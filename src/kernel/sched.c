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

// #define VERBOSE_LOGGING

static struct thread *current_thread = 0;

static size_t nextpid = 0;

static size_t current_priolevel = 0;

#define QUEUE_COUNT (THREAD_PRIORITY_LOW + 1)

/// Run queues for each priority level.
/// \todo Group threads with a common parent process.
static void *prio_queues[THREAD_PRIORITY_LOW + 1] = {0};

/// Already queues for the above.
static void *prio_already_queues[THREAD_PRIORITY_LOW + 1] = {0};

/// Zombie thread queue.
static void *zombie_queue = 0;

/** Initialises the architecture-specific context layer (for create_context). */
extern void init_context();

static int sched_timer(uint64_t ticks) {
    if(ticks > current_thread->timeslice)
        ticks = current_thread->timeslice;
    current_thread->timeslice -= ticks;

    return current_thread->timeslice ? 0 : 1;
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
    return current_thread;
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

struct thread *create_thread(struct process *parent, uint32_t prio, thread_entry_t start, uintptr_t stack, size_t stacksz) {
    if(stacksz == 0)
        stacksz = 0x1000;

    struct thread *t = (struct thread *) malloc(sizeof(struct thread));
    memset(t, 0, sizeof(struct thread));

    t->state = THREAD_STATE_SLEEPING;
    t->timeslice = THREAD_DEFAULT_TIMESLICE;
    t->parent = parent;

    t->base_priority = t->priority = prio;

    t->ctx = (context_t *) malloc(sizeof(context_t));
    create_context(t->ctx, start, stack, stacksz);

    list_insert(parent->thread_list, t, 0);

    return t;
}

void switch_threads(struct thread *old, struct thread *new) {
#ifdef VERBOSE_LOGGING
    dprintf("switch_threads: %x -> %x\n", old, new);
#endif
    if(!old) {
        if(!current_thread)
            current_thread = new;
        new->state = THREAD_STATE_RUNNING;

        switch_context(0, new->ctx);
    }
    else
        switch_context(old->ctx, new->ctx);
}

void thread_kill() {
    assert(current_thread != 0);

    // Put the thread into the zombie state and then kill it.
    current_thread->state = THREAD_STATE_ZOMBIE;
    queue_push(zombie_queue, current_thread);
    reschedule();
}

void thread_sleep() {
    assert(current_thread != 0);

    /// TODO: extra parameter for reschedule to make this atomic? Maybe.
    current_thread->state = THREAD_STATE_SLEEPING;
    reschedule();
}

void thread_wake(struct thread *thr) {
    assert(thr != 0);

    thr->state = THREAD_STATE_READY;
    if(!prio_queues[thr->priority])
        prio_queues[thr->priority] = create_queue();
    queue_push(prio_queues[thr->priority], thr);
}

uint32_t thread_priority(struct thread *prio) {
    if(prio == NULL) {
        prio = current_thread;
    }

    return prio->priority;
}

static void go_next_priolevel() {
    while((current_priolevel < QUEUE_COUNT)) {
        if(prio_queues[current_priolevel]) {
            if(!queue_empty(prio_queues[current_priolevel])) {
                break;
            }
        }

        current_priolevel = current_priolevel + 1;
    }
}

void reschedule() {
    if(current_thread->timeslice > 0) {
#ifdef VERBOSE_LOGGING
        dprintf("reschedule before timeslice completes\n");
#endif
        if(current_thread->priority > current_thread->base_priority)
            current_thread->priority--;
    } else {
#ifdef VERBOSE_LOGGING
        dprintf("reschedule due to completed timeslice\n");
#endif
        current_thread->priority++;

        if(current_thread->priority > THREAD_PRIORITY_LOW)
            current_thread->priority = THREAD_PRIORITY_LOW;
    }

    // RUNNING -> READY transition for the current thread. State could be
    // SLEEPING, in which case this reschedule is to pick a new thread to run,
    // leaving the current thread off the queue.
    if(current_thread->state == THREAD_STATE_RUNNING) {
        current_thread->state = THREAD_STATE_READY;

        if(!prio_already_queues[current_thread->priority])
            prio_already_queues[current_thread->priority] = create_queue();
        queue_push(prio_already_queues[current_thread->priority], current_thread);
    }

    // Find the next priority level with items in it.
    go_next_priolevel();

    if(current_priolevel >= QUEUE_COUNT) {
        for(size_t i = 0; i < QUEUE_COUNT; i++) {
            void *tmp = prio_queues[i];
            prio_queues[i] = prio_already_queues[i];
            prio_already_queues[i] = tmp;
        }

        current_priolevel = 0;
        go_next_priolevel();
    }

    assert(!queue_empty(prio_queues[current_priolevel]));

    // Pop a thread off the run queue.
    struct thread *thr = (struct thread *) queue_pop(prio_queues[current_priolevel]);
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

        reschedule();
    }

    // Reset the timeslice and prepare for context switch.
    thr->timeslice = THREAD_DEFAULT_TIMESLICE;
    thr->state = THREAD_STATE_RUNNING;

    // Empty run queue?
    if(queue_empty(prio_queues[current_priolevel])) {
        current_priolevel++;
    }

#ifdef VERBOSE_LOGGING
    dprintf("reschedule: queues now run: %sempty / already: %sempty\n", queue_empty(prio_queues[current_priolevel]) ? "" : "not ", queue_empty(prio_already_queues[current_priolevel]) ? "" : "not ");
#endif

    // Perform the context switch if this isn't the already-running thread.
    if(thr != current_thread) {
        if(current_thread->parent != thr->parent) {
            /// \todo Process switch - address space and such.
            dprintf("TODO: process switch - address space etc %x %x\n", current_thread->parent, thr->parent);
        }

        struct thread *tmp = current_thread;
        current_thread = thr;
        switch_threads(tmp, thr);
    }
}

void init_scheduler() {
    for(size_t i = 0; i < QUEUE_COUNT; i++) {
        prio_queues[i] = 0;
        prio_already_queues[i] = 0;
    }

    init_context();

    zombie_queue = create_queue();

    // Timer handler for the zombie reaper.
    install_timer(zombie_reaper, ((1 << TIMERRES_SHIFT) | TIMERRES_SECONDS), TIMERFEAT_PERIODIC);
}


void start_scheduler() {
    // Can't start the scheduler without a thread running!
    assert(current_thread != 0);

    // Install a timer handler - tick for timeslice completion.
    if(install_timer(sched_timer, ((THREAD_DEFAULT_TIMESLICE_MS << TIMERRES_SHIFT) | TIMERRES_MILLI), TIMERFEAT_PERIODIC) < 0) {
        dprintf("scheduler install failed - no useful timer available\n");
        kprintf("scheduler install failed - system will have its usability greatly reduced\n");
    }
}

