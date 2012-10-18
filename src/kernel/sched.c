/*
 * Copyright (c) 2012 Matthew Iselin, Rich Edelman
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */
#include <sched.h>
#include <types.h>
#include <malloc.h>
#include <string.h>
#include <assert.h>
#include <timer.h>
#include <util.h>
#include <vmem.h>
#include <io.h>
#include <spinlock.h>
#include <multicpu.h>
#include <interrupts.h>

// #define VERBOSE_LOGGING

#define RESCHED_IDLE_RETURN         0
#define RESCHED_IDLE_RUNTHREAD      1

static size_t nextpid = 0;

#define QUEUE_COUNT (THREAD_PRIORITY_LOW + 1)

void *ready_queue = 0;

/// Primary priority queues.
void *prio_queues[QUEUE_COUNT] = {0};

/// Alternate priority queues.
void *prio_already_queues[QUEUE_COUNT] = {0};

/// Zombie thread queue.
static void *zombie_queue = 0;

/// Global scheduler lock - to ensure queue operations are done atomically.
static void *sched_spinlock = 0;

/// Idle thread in the system that we can clone onto new CPUs as they come up.
static struct thread *g_idle_thread = 0;

/// Number of threads currently ready to run (excluding idle thread).
static atomic_t numready = 0;

/// Global priority level.
static atomic_t priolevel = 0;

/** Initialises the architecture-specific context layer (for create_context). */
extern void init_context();

static void reschedule_internal(size_t action_on_idle);

static void **get_prio_queues() {
    return (void **) prio_queues;
}

static void **get_prio_already_queues() {
    return (void **) prio_already_queues;
}

static struct thread *get_current_thread() {
    struct thread **current_thread = (struct current_thread **) multicpu_percpu_at(MULTICPU_PERCPU_CURRTHREAD);
    return *current_thread;
}

static void set_current_thread(struct thread *t) {
    struct thread **current_thread = (struct current_thread **) multicpu_percpu_at(MULTICPU_PERCPU_CURRTHREAD);
    *current_thread = t;
}

static struct thread *get_idle_thread() {
    struct thread **idle_thread = (struct current_thread **) multicpu_percpu_at(MULTICPU_PERCPU_IDLETHREAD);
    return *idle_thread;
}

static void set_idle_thread(struct thread *t) {
    struct thread **idle_thread = (struct current_thread **) multicpu_percpu_at(MULTICPU_PERCPU_IDLETHREAD);
    *idle_thread = t;
}

static size_t get_current_priolevel() {
    return priolevel;
}

static void set_current_priolevel(size_t new) {
    priolevel = new;
}

/// Sets the current priority level, but only if it matches a previous value.
static void set_current_priolevel_if(size_t new, size_t old) {
    atomic_val_compare_and_swap(&priolevel, old, new);
}

static void increment_priolevel() {
    atomic_inc(priolevel);
}

static int sched_timer(uint64_t ticks) {
    if(ticks > get_current_thread()->timeslice)
        ticks = get_current_thread()->timeslice;
    get_current_thread()->timeslice -= ticks;

    int doresched = get_current_thread()->timeslice ? 0 : 1;
    dprintf("sched timer: %d\n", doresched);
    return doresched;
}

static int zombie_reaper(uint64_t ticks) {
    if(zombie_queue == 0)
        return 0;

    // Clean up ALL the threads.
    while(!queue_empty(zombie_queue)) {
        struct thread *thr = (struct thread *) queue_pop(zombie_queue);
        dprintf("reaping zombie thread %p\n", thr);

        destroy_context(thr->ctx);

        free(thr->ctx);
        free(thr);
    }

    return 0;
}

struct thread *sched_current_thread() {
    return get_current_thread();
}

void sched_setidle(struct thread *t) {
    g_idle_thread = t;
    g_idle_thread->isidle = 1;
    set_idle_thread(g_idle_thread);
}

static void install_sched_timer() {
    // Tick for timeslice completion.
    if(install_timer(sched_timer, ((THREAD_DEFAULT_TIMESLICE_MS << TIMERRES_SHIFT) | TIMERRES_MILLI), TIMERFEAT_PERIODIC | TIMERFEAT_PERCPU) < 0) {
        dprintf("scheduler install failed - no useful timer available\n");
        kprintf("scheduler install failed - system will have its usability greatly reduced\n");
    }
}

void sched_cpualive(void *lock) {
    dprintf("scheduler: new cpu (%d) to be registered!\n", multicpu_id());

    // If an idle thread has been installed, start up the scheduler on this core
    if(g_idle_thread) {
        struct thread *t = (struct thread *) malloc(sizeof(struct thread));
        memset(t, 0, sizeof(struct thread));

        t->state = THREAD_STATE_READY;
        t->timeslice = THREAD_DEFAULT_TIMESLICE;
        t->parent = g_idle_thread->parent;
        t->isidle = 1;

        t->base_priority = g_idle_thread->base_priority;
        t->priority = g_idle_thread->priority;
        t->ctx = (context_t *) malloc(sizeof(context_t));
        clone_context(g_idle_thread->ctx, t->ctx);

        list_insert(t->parent->thread_list, t, 0);

        set_idle_thread(t);
        set_current_thread(t);

        install_sched_timer();
        switch_threads(0, t, 1, lock);
    }
}

struct process *create_process(const char *name, struct process *parent) {
    struct process *ret = (struct process *) malloc(sizeof(struct process));
    memset(ret, 0, sizeof(struct process));

    if(name == 0)
        strcpy(ret->name, DEFAULT_PROCESS_NAME);
    else
        strncpy(ret->name, name, PROCESS_NAME_MAX);

    ret->pid = nextpid++;

    ret->child_list = create_list();
    ret->thread_list = create_list();

    if(parent != 0) {
        if(parent->child_list == 0)
            parent->child_list = create_list();

        list_insert(parent->child_list, ret, 0);
    }

    return ret;
}

struct thread *create_thread(struct process *parent, uint32_t prio, thread_entry_t start, uintptr_t stack, size_t stacksz, void *param) {
    struct thread *t = (struct thread *) malloc(sizeof(struct thread));
    memset(t, 0, sizeof(struct thread));

    t->state = THREAD_STATE_SLEEPING;
    t->timeslice = THREAD_DEFAULT_TIMESLICE;
    t->parent = parent;

    t->base_priority = t->priority = prio;

    t->ctx = (context_t *) malloc(sizeof(context_t));
    create_context(t->ctx, start, stack, stacksz, param);

    list_insert(parent->thread_list, t, 0);

    return t;
}

void switch_threads(struct thread *old, struct thread *new, unative_t intstate, void *lock) {
#ifdef VERBOSE_LOGGING
    dprintf("switch_threads: %x -> %x\n", old, new);
#endif
    if(!old) {
        if(!get_current_thread())
            set_current_thread(new);
        new->state = THREAD_STATE_RUNNING;

        switch_context(0, new->ctx, intstate, lock ? spinlock_getatom(lock) : lock);
    }
    else {
        dprintf("cpu %d old ctx %p -> new ctx %p\n", multicpu_id(), old->ctx, new->ctx);
        switch_context(old->ctx, new->ctx, intstate, lock ? spinlock_getatom(lock) : lock);
    }
}

void thread_kill() {
    assert(get_current_thread() != 0);

    // Put the thread into the zombie state and then kill it.
    get_current_thread()->state = THREAD_STATE_ZOMBIE;
    queue_push(zombie_queue, get_current_thread());
    reschedule();

    while(1) panic("thread_kill trying to return\n");
}

void thread_return() {
    dprintf("sched: thread returning - killing\n");
    thread_kill();
}

void thread_sleep() {
    assert(get_current_thread() != 0);

    get_current_thread()->state = THREAD_STATE_SLEEPING;
    reschedule();
}

void thread_wake(struct thread *thr) {
    assert(thr != 0);

    dprintf("waking thread %x\n", thr);

    queue_push(ready_queue, thr);

#if 0

    void **queues = get_prio_queues();

    spinlock_acquire(sched_spinlock);
    if(!queues[thr->priority]) {
        queues[thr->priority] = create_queue();
    }

    queue_push(queues[thr->priority], thr);
    spinlock_release(sched_spinlock);

    atomic_inc(numready);

#endif

    thr->state = THREAD_STATE_READY;
}

uint32_t thread_priority(struct thread *prio) {
    if(prio == NULL) {
        prio = get_current_thread();
    }

    return prio->priority;
}

static void go_next_priolevel() {
    void **queues = get_prio_queues();
    void **already = get_prio_already_queues();
    void **q = 0;

    size_t level = 0, real = 0;
    while(((level = get_current_priolevel()) < (QUEUE_COUNT * 2))) {
        if(level >= QUEUE_COUNT) {
            real = level % QUEUE_COUNT;
            q = already;
        } else {
            q = queues;
        }

        if(q[real]) {
            int empty = queue_empty(q[real]);
            if(!empty) {
                break;
            }
        }

        // Make sure the priority level is only incremented if another core
        // didn't increment it already!
        set_current_priolevel_if(level + 1, level);
    }
}

static int is_idle(size_t action_on_idle, unative_t intstate) {
    int empty = queue_empty(ready_queue);

    // Handle idle.
    if((empty) && ((get_current_thread() == get_idle_thread()) || (action_on_idle == RESCHED_IDLE_RETURN))) {
        return 1;
    } else if(empty) {
        if(action_on_idle == RESCHED_IDLE_RUNTHREAD) {
            dprintf("cpu %d became idle\n", multicpu_id());
            get_idle_thread()->state = THREAD_STATE_RUNNING;
            get_idle_thread()->timeslice = THREAD_DEFAULT_TIMESLICE;

            if(get_current_thread() != get_idle_thread()) {
                struct thread *tmp = get_current_thread();
                set_current_thread(get_idle_thread());
                switch_threads(tmp, get_idle_thread(), intstate, 0);
            }

            return 1;
        }

        dprintf("action: %d\n", action_on_idle);
        panic("bad action on idle to scheduler");
    }

    return 0;

#if 0
    size_t nr_tmp = numready;

    // Handle idle.
    if((!nr_tmp) && ((get_current_thread() == get_idle_thread()) || (action_on_idle == RESCHED_IDLE_RETURN))) {
        return 1;
    } else if(!nr_tmp) {
        if(action_on_idle == RESCHED_IDLE_RUNTHREAD) {
            dprintf("cpu %d became idle\n", multicpu_id());
            get_idle_thread()->state = THREAD_STATE_RUNNING;

            if(get_current_thread() != get_idle_thread()) {
                struct thread *tmp = get_current_thread();
                set_current_thread(get_idle_thread());
                switch_threads(tmp, get_idle_thread(), intstate, 0);
            }

            return 1;
        }

        dprintf("action: %d\n", action_on_idle);
        panic("bad action on idle to scheduler");
    }

    return 0;
#endif
}

static void reschedule_internal(size_t action_on_idle) {
    /// \note Scheduling is performed on each core and refers to the global
    ///       queues to receive threads to execute. Therefore, if the system
    ///       has four cores, and four threads in the ready queue, each core
    ///       will run a thread each.
    ///       Because queues do not require locking to access, we can simply
    ///       disable interrupts to avoid pre-emption and access core-specific
    ///       data as needed. Atomic operations are used on thread state where
    ///       possible.

    // Handle the case where the current thread is NULL (probably on a CPU which
    // isn't yet fully configured).
    if(!get_current_thread()) {
        return;
    }

    // Preserve interrupt state.
    unative_t intstate = interrupts_get();

    // Must run without interruption for the time being...
    interrupts_disable();

#if 0
    if(get_current_thread()->timeslice > 0) {
#ifdef VERBOSE_LOGGING
        dprintf("reschedule before timeslice completes\n");
#endif
        if(get_current_thread()->priority > get_current_thread()->base_priority)
            atomic_dec(get_current_thread()->priority);
    } else {
#ifdef VERBOSE_LOGGING
        dprintf("reschedule due to completed timeslice\n");
#endif
        atomic_inc(get_current_thread()->priority);

        if(get_current_thread()->priority > THREAD_PRIORITY_LOW)
            get_current_thread()->priority = THREAD_PRIORITY_LOW;
    }
#endif

    // RUNNING -> READY transition for the current thread. State could be
    // SLEEPING, in which case this reschedule is to pick a new thread to run,
    // leaving the current thread off the queue.
    // Also, the idle thread is handled specially.
    if(get_current_thread() != get_idle_thread()) {
        if(get_current_thread()->state == THREAD_STATE_RUNNING) {
            get_current_thread()->state = THREAD_STATE_READY;

            queue_push(ready_queue, get_current_thread());
        }
    }

    // Gone to idle state (ie, no threads ready).
    if(is_idle(action_on_idle, intstate)) {
        if(intstate) {
            interrupts_enable();
        }

        return;
    }

    struct thread *thr = (struct thread *) queue_pop(ready_queue);
    assert(thr != 0);

    dprintf("new thread %x current %x\n", thr, get_current_thread());

    // Thread not actually alive?
    if(thr->state != THREAD_STATE_READY) {
        dprintf("reschedule: thread %p in queue wasn't really ready\n", thr);

        // Threads in the zombie state need to be added to the zombie queue here.
        // They cannot be added to the zombie queue if they are 'remotely' killed
        // by another process (as they are already in the queue).
        if(thr->state == THREAD_STATE_ZOMBIE) {
            queue_push(zombie_queue, thr);
        }

        if(intstate) {
            interrupts_enable();
        }

        reschedule();

        return;
    }

    // Reset the timeslice and prepare for context switch.
    thr->timeslice = THREAD_DEFAULT_TIMESLICE;
    thr->state = THREAD_STATE_RUNNING;

#ifdef VERBOSE_LOGGING
    dprintf("reschedule: queues now run: %sempty / already: %sempty\n", queue_empty(queues[get_current_priolevel()]) ? "" : "not ", queue_empty(already_queues[get_current_priolevel()]) ? "" : "not ");
#endif

    // Perform the context switch if this isn't the already-running thread.
    if(thr != get_current_thread()) {
        if(get_current_thread()->parent != thr->parent) {
            /// \todo Process switch - address space and such.
            dprintf("TODO: process switch - address space etc %x %x\n", get_current_thread()->parent, thr->parent);
        }

        struct thread *tmp = get_current_thread();
        set_current_thread(thr);
        switch_threads(tmp, thr, intstate, 0);
    } else if(intstate) {
        interrupts_enable();
    }

#if 0

    // RUNNING -> READY transition for the current thread. State could be
    // SLEEPING, in which case this reschedule is to pick a new thread to run,
    // leaving the current thread off the queue.
    // Also, the idle thread is handled specially.
    if(get_current_thread() != get_idle_thread()) {
        dprintf("old state %d vs %d\n", get_current_thread()->state, THREAD_STATE_RUNNING);
        if(get_current_thread()->state == THREAD_STATE_RUNNING) {
            get_current_thread()->state = THREAD_STATE_READY;

            dprintf("running -> ready transition\n");

            // Create an already queue for this priority level, if one doesn't exist yet.
            if(!already_queues[get_current_thread()->priority]) {
                spinlock_acquire(sched_spinlock);
                // Check to see if the already queue got created during the lock acquire.
                if(!already_queues[get_current_thread()->priority]) {
                    already_queues[get_current_thread()->priority] = create_queue();
                }
                spinlock_release(sched_spinlock);
            }

            // Push onto the already queue.
            queue_push(already_queues[get_current_thread()->priority], get_current_thread());

            // This thread is now ready.
            atomic_inc(numready);
        }
    }

    // Gone to idle state?
    if(is_idle(action_on_idle, intstate)) {
        if(intstate) {
            interrupts_enable();
        }

        return;
    }

    // We switch back to the first priority level if we were running the idle
    // thread, as if we don't we won't find any threads to run (we'll switch to
    // the already queue, and the thread to run is in the already queue!)
    if(get_current_thread() == get_idle_thread()) {
        set_current_priolevel(0);
    }

    // Find the next priority level with items in it.
    go_next_priolevel();

    if(get_current_priolevel() >= QUEUE_COUNT) {
        // Swap the already queues to be the ready queues (and vice versa).
        swap_queues();

        set_current_priolevel(0);
        go_next_priolevel();
    }

    // Current priority level, saved until we are ready to continue.
    size_t level = get_current_priolevel();
    dprintf("level: %d/%d [%d]\n", level, QUEUE_COUNT, numready);

    assert(level < QUEUE_COUNT);

    // Pop a thread off the run queue.
    struct thread *thr = (struct thread *) queue_pop(queues[level]);
    if(thr == 0) {
        dprintf("queue emptied after selecting priority level!\n");

        // Did we become idle as a result?
        if(is_idle(action_on_idle, intstate)) {
            if(intstate) {
                interrupts_enable();
            }

            return;
        }

        // Try the next level, otherwise.
        level = get_current_priolevel();
        thr = (struct thread *) queue_pop(queues[level]);
        if(thr == 0) {
            // Okay, this is hopeless! Return to the original thread.
            if(intstate) {
                interrupts_enable();
            }

            return;
        }
    }

    // Thread not actually alive?
    if(thr->state != THREAD_STATE_READY) {
        dprintf("reschedule: thread %p in queue wasn't really ready\n", thr);

        // Threads in the zombie state need to be added to the zombie queue here.
        // They cannot be added to the zombie queue if they are 'remotely' killed
        // by another process (as they are already in the queue).
        if(thr->state == THREAD_STATE_ZOMBIE) {
            queue_push(zombie_queue, thr);
        }

        if(intstate) {
            interrupts_enable();
        }

        reschedule();

        return;
    }

    // Reset the timeslice and prepare for context switch.
    thr->timeslice = THREAD_DEFAULT_TIMESLICE;
    thr->state = THREAD_STATE_RUNNING;

    // Thread no longer ready - being switched to.
    atomic_dec(numready);

    // Empty run queue?
    if(queue_empty(queues[level])) {
        // Only increment the priority level if it wasn't already.
        set_current_priolevel_if(level + 1, level);
    }

#ifdef VERBOSE_LOGGING
    dprintf("reschedule: queues now run: %sempty / already: %sempty\n", queue_empty(queues[get_current_priolevel()]) ? "" : "not ", queue_empty(already_queues[get_current_priolevel()]) ? "" : "not ");
#endif

    // Perform the context switch if this isn't the already-running thread.
    if(thr != get_current_thread()) {
        if(get_current_thread()->parent != thr->parent) {
            /// \todo Process switch - address space and such.
            dprintf("TODO: process switch - address space etc %x %x\n", get_current_thread()->parent, thr->parent);
        }

        struct thread *tmp = get_current_thread();
        set_current_thread(thr);
        switch_threads(tmp, thr, intstate, 0);
    }

    if(intstate) {
        interrupts_enable();
    }

#endif
}

void sched_yield() {
    reschedule_internal(RESCHED_IDLE_RETURN);
}

void reschedule() {
    reschedule_internal(RESCHED_IDLE_RUNTHREAD);
}

void init_scheduler() {
    zombie_queue = create_queue();

    sched_spinlock = create_spinlock();

    ready_queue = create_queue();

    // Set up the current CPU (other CPUs will be enabled as they come alive)
    sched_cpualive(0);

    init_context();

    numready = 0;

    dprintf("scheduler spinlock is %p\n", sched_spinlock);

    // Timer handler for the zombie reaper.
    install_timer(zombie_reaper, ((1 << TIMERRES_SHIFT) | TIMERRES_SECONDS), TIMERFEAT_PERIODIC);
}

void start_scheduler() {
    // Can't start the scheduler without a thread running!
    assert(get_current_thread() != 0);

    spinlock_acquire(sched_spinlock);

    install_sched_timer();

    spinlock_release(sched_spinlock);
}

