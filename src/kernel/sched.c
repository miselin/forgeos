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

// #define VERBOSE_LOGGING

static void *current_thread = 0;

static size_t nextpid = 0;

static void *current_priolevel = 0;

#define QUEUE_COUNT (THREAD_PRIORITY_LOW + 1)

/// Run queues for each priority level.
/// \todo Group threads with a common parent process.
static void *prio_queues = 0;

/// Already queues for the above.
static void *prio_already_queues = 0;

/// Zombie thread queue.
static void *zombie_queue = 0;

/// Global scheduler lock - to ensure queue operations are done atomically.
static void *sched_spinlock = 0;

/// Idle thread in the system that we can clone onto new CPUs as they come up.
static struct thread *idle_thread = 0;

/** Initialises the architecture-specific context layer (for create_context). */
extern void init_context();

static void **get_prio_queues() {
    if(!prio_queues)
        return NULL;

    void **arr = tree_search(prio_queues, (void *) multicpu_id());
    if(arr == TREE_NOTFOUND) {
        return NULL;
    } else {
        return arr;
    }
}

static void **get_prio_already_queues() {
    if(!prio_already_queues)
        return NULL;

    void **arr = tree_search(prio_already_queues, (void *) multicpu_id());
    if(arr == TREE_NOTFOUND) {
        return NULL;
    } else {
        return arr;
    }
}

static struct thread *get_current_thread() {
    void *r = tree_search(current_thread, (void *) multicpu_id());
    if(r == TREE_NOTFOUND)
        return 0;
    else
        return (struct thread *) r;
}

static void set_current_thread(struct thread *t) {
    tree_delete(current_thread, (void *) multicpu_id());
    tree_insert(current_thread, (void *) multicpu_id(), t);
}

static size_t get_current_priolevel() {
    void *r = tree_search(current_priolevel, (void *) multicpu_id());
    if(r == TREE_NOTFOUND)
        return 0;
    else
        return (size_t) r;
}

static void set_current_priolevel(size_t new) {
    tree_delete(current_priolevel, (void *) multicpu_id());
    tree_insert(current_priolevel, (void *) multicpu_id(), (void *) new);
}

static int sched_timer(uint64_t ticks) {
    if(ticks > get_current_thread()->timeslice)
        ticks = get_current_thread()->timeslice;
    get_current_thread()->timeslice -= ticks;

    int do_resched = get_current_thread()->timeslice ? 0 : 1;
    if(do_resched) {
        multicpu_doresched();
    }
    return do_resched;
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
    idle_thread = t;
    thread_wake(t);
}

void sched_cpualive() {
    dprintf("scheduler: new cpu (%d) to be registered!\n", multicpu_id());

    spinlock_acquire(sched_spinlock);
    void **prio = (void **) malloc(sizeof(void *) * QUEUE_COUNT);
    void **prio_already = (void **) malloc(sizeof(void *) * QUEUE_COUNT);

    size_t i;
    for(i = 0; i < QUEUE_COUNT; i++) {
        prio[i] = 0;
        prio_already[i] = 0;
    }

    tree_insert(prio_queues, (void *) multicpu_id(), prio);
    tree_insert(prio_already_queues, (void *) multicpu_id(), prio_already);

    set_current_priolevel(THREAD_PRIORITY_REALTIME);
    spinlock_release(sched_spinlock);

    // Clone the idle thread and then switch to it, if it exists.
    if(idle_thread) {
        struct thread *t = (struct thread *) malloc(sizeof(struct thread));
        memset(t, 0, sizeof(struct thread));

        t->state = THREAD_STATE_SLEEPING;
        t->timeslice = THREAD_DEFAULT_TIMESLICE;
        t->parent = idle_thread->parent;

        t->base_priority = idle_thread->base_priority;
        t->priority = idle_thread->priority;
        t->ctx = (context_t *) malloc(sizeof(context_t));
        clone_context(idle_thread->ctx, t->ctx);

        list_insert(idle_thread->parent->thread_list, t, 0);

        thread_wake(t);
        switch_threads(0, t);
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

void switch_threads(struct thread *old, struct thread *new) {
#ifdef VERBOSE_LOGGING
    dprintf("switch_threads: %x -> %x\n", old, new);
#endif
    if(!old) {
        if(!get_current_thread())
            set_current_thread(new);
        new->state = THREAD_STATE_RUNNING;

        switch_context(0, new->ctx, 0);
    }
    else {
        dprintf("old ctx %p -> new ctx %p\n", old->ctx, new->ctx);
        switch_context(old->ctx, new->ctx, spinlock_getatom(sched_spinlock));
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
        dprintf("creating queue for level %d\n", thr->priority);
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

void sched_yield() {
    reschedule();
}

void reschedule() {
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

    // RUNNING -> READY transition for the current thread. State could be
    // SLEEPING, in which case this reschedule is to pick a new thread to run,
    // leaving the current thread off the queue.
    if(get_current_thread()->state == THREAD_STATE_RUNNING) {
        get_current_thread()->state = THREAD_STATE_READY;

        if(!already_queues[get_current_thread()->priority]) {
            already_queues[get_current_thread()->priority] = create_queue();
        }
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

    assert(!queue_empty(queues[get_current_priolevel()]));

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
        switch_threads(tmp, thr);
    } else {
        if(thr == idle_thread) {
            // System is idle (switched from idle thread to idle thread).
        }
        spinlock_release(sched_spinlock);
    }
}

void init_scheduler() {
    // Set up the per-CPU queue.
    prio_queues = create_tree();
    prio_already_queues = create_tree();

    current_thread = create_tree();
    current_priolevel = create_tree();

    zombie_queue = create_queue();

    sched_spinlock = create_spinlock();

    // Set up the current CPU (other CPUs will be enabled as they come alive)
    sched_cpualive();

    init_context();

    dprintf("scheduler spinlock is %p\n", sched_spinlock);

    // Timer handler for the zombie reaper.
    install_timer(zombie_reaper, ((1 << TIMERRES_SHIFT) | TIMERRES_SECONDS), TIMERFEAT_PERIODIC);
}

void start_scheduler() {
    // Can't start the scheduler without a thread running!
    assert(get_current_thread() != 0);

    spinlock_acquire(sched_spinlock);

    // Install a timer handler - tick for timeslice completion.
    if(install_timer(sched_timer, ((THREAD_DEFAULT_TIMESLICE_MS << TIMERRES_SHIFT) | TIMERRES_MILLI), TIMERFEAT_PERIODIC) < 0) {
        dprintf("scheduler install failed - no useful timer available\n");
        kprintf("scheduler install failed - system will have its usability greatly reduced\n");
    }

    spinlock_release(sched_spinlock);
}

