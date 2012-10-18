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
    return *((size_t *) multicpu_percpu_at(MULTICPU_PERCPU_PRIOLEVEL));
}

static void set_current_priolevel(size_t new) {
    *((size_t *) multicpu_percpu_at(MULTICPU_PERCPU_PRIOLEVEL)) = new;
}

static int sched_timer(uint64_t ticks) {
    if(ticks > get_current_thread()->timeslice)
        ticks = get_current_thread()->timeslice;
    get_current_thread()->timeslice -= ticks;

    // Don't pre-empt the idle thread (it will do that itself).
    int doresched = 0;
    if(!get_current_thread()->isidle) {
        doresched = get_current_thread()->timeslice ? 0 : 1;
    }
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

    spinlock_acquire(sched_spinlock);
    set_current_priolevel(THREAD_PRIORITY_REALTIME);
    spinlock_release(sched_spinlock);

    // If an idle thread has been installed, start up the scheduler on this core
    if(g_idle_thread) {
        struct thread *t = (struct thread *) malloc(sizeof(struct thread));
        memset(t, 0, sizeof(struct thread));

        t->state = THREAD_STATE_READY;
        t->timeslice = THREAD_DEFAULT_TIMESLICE;
        t->parent = g_idle_thread->parent;

        t->base_priority = g_idle_thread->base_priority;
        t->priority = g_idle_thread->priority;
        t->ctx = (context_t *) malloc(sizeof(context_t));
        clone_context(g_idle_thread->ctx, t->ctx);

        list_insert(t->parent->thread_list, t, 0);

        set_idle_thread(t);

        install_sched_timer();
        switch_threads(0, get_idle_thread(), lock);
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

void switch_threads(struct thread *old, struct thread *new, void *lock) {
#ifdef VERBOSE_LOGGING
    dprintf("switch_threads: %x -> %x\n", old, new);
#endif
    if(!old) {
        if(!get_current_thread())
            set_current_thread(new);
        new->state = THREAD_STATE_RUNNING;

        switch_context(0, new->ctx, spinlock_getatom(lock), interrupts_get());
    }
    else {
        dprintf("cpu %d old ctx %p -> new ctx %p\n", multicpu_id(), old->ctx, new->ctx);
        switch_context(old->ctx, new->ctx, spinlock_getatom(lock), spinlock_intstate(lock));
    }
}

void thread_kill() {
    assert(get_current_thread() != 0);

    // Put the thread into the zombie state and then kill it.
    get_current_thread()->state = THREAD_STATE_ZOMBIE;
    queue_push(zombie_queue, get_current_thread());
    reschedule();
}

void thread_return() {
    dprintf("sched: thread returning - killing\n");
    thread_kill();
}

void thread_sleep() {
    assert(get_current_thread() != 0);

    /// TODO: extra parameter for reschedule to make this atomic? Maybe.
    get_current_thread()->state = THREAD_STATE_SLEEPING;
    reschedule();
}

void thread_wake(struct thread *thr) {
    assert(thr != 0);

    thr->state = THREAD_STATE_READY;

    spinlock_acquire(sched_spinlock);
    void **queues = get_prio_queues();
    if(!queues[thr->priority]) {
        queues[thr->priority] = create_queue();
    }
    queue_push(queues[thr->priority], thr);
    spinlock_release(sched_spinlock);
}

uint32_t thread_priority(struct thread *prio) {
    if(prio == NULL) {
        prio = get_current_thread();
    }

    return prio->priority;
}

static void go_next_priolevel() {
    void **queues = get_prio_queues();
    while((get_current_priolevel() < QUEUE_COUNT)) {
        if(queues[get_current_priolevel()]) {
            if(!queue_empty(queues[get_current_priolevel()])) {
                break;
            }
        }

        set_current_priolevel(get_current_priolevel() + 1);
    }
}

static void reschedule_internal(size_t action_on_idle) {
    spinlock_acquire(sched_spinlock);

    // Handle the case where the current thread is NULL (probably on a CPU which
    // isn't yet fully configured).
    if(!get_current_thread()) {
        spinlock_release(sched_spinlock);
        return;
    }

    void **queues = get_prio_queues();
    void **already_queues = get_prio_already_queues();

    if(get_current_thread()->timeslice > 0) {
#ifdef VERBOSE_LOGGING
        dprintf("reschedule before timeslice completes\n");
#endif
        if(get_current_thread()->priority > get_current_thread()->base_priority)
            get_current_thread()->priority--;
    } else {
#ifdef VERBOSE_LOGGING
        dprintf("reschedule due to completed timeslice\n");
#endif
        get_current_thread()->priority++;

        if(get_current_thread()->priority > THREAD_PRIORITY_LOW)
            get_current_thread()->priority = THREAD_PRIORITY_LOW;
    }

    // Create an already queue for this priority level, if one doesn't exist yet.
    if(!already_queues[get_current_thread()->priority]) {
        already_queues[get_current_thread()->priority] = create_queue();
    }

    // RUNNING -> READY transition for the current thread. State could be
    // SLEEPING, in which case this reschedule is to pick a new thread to run,
    // leaving the current thread off the queue.
    // Also, the idle thread is handled specially.
    if((get_current_thread()->state == THREAD_STATE_RUNNING) &&
        (get_current_thread() != get_idle_thread())) {
        get_current_thread()->state = THREAD_STATE_READY;

        queue_push(already_queues[get_current_thread()->priority], get_current_thread());
    }

    // Find the next priority level with items in it.
    go_next_priolevel();

    if(get_current_priolevel() >= QUEUE_COUNT) {
        for(size_t i = 0; i < QUEUE_COUNT; i++) {
            void *tmp = queues[i];
            queues[i] = already_queues[i];
            already_queues[i] = tmp;
        }

        set_current_priolevel(0);
        go_next_priolevel();
    }

    // Detect idle.
    if(get_current_priolevel() >= QUEUE_COUNT) {
        // Reset priority level.
        set_current_priolevel(0);

        if(action_on_idle == RESCHED_IDLE_RETURN) {
            spinlock_release(sched_spinlock);
            return;
        } else if(action_on_idle == RESCHED_IDLE_RUNTHREAD) {
            dprintf("cpu %d: idle\n", multicpu_id());

            get_idle_thread()->timeslice = THREAD_DEFAULT_TIMESLICE;
            get_idle_thread()->state = THREAD_STATE_RUNNING;

            struct thread *tmp = get_current_thread();
            set_current_thread(get_idle_thread());
            switch_threads(tmp, get_idle_thread(), sched_spinlock);
            return;
        } else {
            dprintf("unsupported internal reschedule action on idle\n");
            spinlock_release(sched_spinlock);
            return;
        }
    }

    // Pop a thread off the run queue.
    struct thread *thr = (struct thread *) queue_pop(queues[get_current_priolevel()]);
    assert(thr != 0);

    // Thread not actually alive?
    if(thr->state != THREAD_STATE_READY) {
        dprintf("reschedule: thread %p in queue wasn't really ready\n", thr);

        // Threads in the zombie state need to be added to the zombie queue here.
        // They cannot be added to the zombie queue if they are 'remotely' killed
        // by another process (as they are already in the queue).
        if(thr->state == THREAD_STATE_ZOMBIE) {
            queue_push(zombie_queue, thr);
        }

        spinlock_release(sched_spinlock);
        reschedule();

        return;
    }

    // Reset the timeslice and prepare for context switch.
    thr->timeslice = THREAD_DEFAULT_TIMESLICE;
    thr->state = THREAD_STATE_RUNNING;

    // Empty run queue?
    if(queue_empty(queues[get_current_priolevel()])) {
        set_current_priolevel(get_current_priolevel() + 1);
    }

#ifdef VERBOSE_LOGGING
    dprintf("reschedule: queues now run: %sempty / already: %sempty\n", queue_empty(queues[current_priolevel]) ? "" : "not ", queue_empty(already_queues[current_priolevel]) ? "" : "not ");
#endif

    // Perform the context switch if this isn't the already-running thread.
    if(thr != get_current_thread()) {
        if(get_current_thread()->parent != thr->parent) {
            /// \todo Process switch - address space and such.
            dprintf("TODO: process switch - address space etc %x %x\n", get_current_thread()->parent, thr->parent);
        }

        struct thread *tmp = get_current_thread();
        set_current_thread(thr);
        switch_threads(tmp, thr, sched_spinlock);
    } else {
        spinlock_release(sched_spinlock);
    }
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

    // Set up the current CPU (other CPUs will be enabled as they come alive)
    sched_cpualive(0);

    init_context();

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

