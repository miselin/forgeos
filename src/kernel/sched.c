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
#include <io.h>

static void *runqueue = 0;
static void *alreadyqueue = 0;
size_t runqueue_n = 0;

static struct thread *current_thread = 0;

static size_t nextpid = 0;

/** Initialises the architecture-specific context layer (for create_context). */
extern void init_context();

static int sched_timer(uint64_t ticks) {
    if(ticks > current_thread->timeslice)
        ticks = current_thread->timeslice;
    current_thread->timeslice -= ticks;

    return current_thread->timeslice ? 0 : 1;
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
    dprintf("switch_threads: %x -> %x\n", old, new);
    if(!old) {
        if(!current_thread)
            current_thread = new;
        new->state = THREAD_STATE_RUNNING;

        switch_context(0, new->ctx);
    }
    else
        switch_context(old->ctx, new->ctx);
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
    queue_push(runqueue, thr);
}

uint32_t thread_priority(struct thread *prio) {
    if(prio == NULL) {
        prio = current_thread;
    }

    return prio->priority;
}

void reschedule() {
    if(current_thread->timeslice > 0) {
        dprintf("reschedule before timeslice completes\n");
    } else {
        /// \todo Multilevel feedback scheduler will use this to identify
        ///       that we need to drop priority on the thread/process.
    }

    // RUNNING -> READY transition for the current thread. State could be
    // SLEEPING, in which case this reschedule is to pick a new thread to run,
    // leaving the current thread off the queue.
    if(current_thread->state == THREAD_STATE_RUNNING) {
        current_thread->state = THREAD_STATE_READY;
        queue_push(alreadyqueue, current_thread);
    }

    assert(!queue_empty(runqueue) || !queue_empty(alreadyqueue));

    // Pop a thread off the run queue.
    struct thread *thr = (struct thread *) queue_pop(runqueue);
    assert(thr != 0);

    // Reset the timeslice and prepare for context switch.
    thr->timeslice = THREAD_DEFAULT_TIMESLICE;
    thr->state = THREAD_STATE_RUNNING;

    // Empty run queue?
    if(queue_empty(runqueue)) {
        dprintf("Empty runqueue, swapping queues\n");
        void *tmp = runqueue;
        runqueue = alreadyqueue;
        alreadyqueue = tmp;
    }

    dprintf("reschedule: queues now run: %sempty / already: %sempty\n", queue_empty(runqueue) ? "" : "not ", queue_empty(alreadyqueue) ? "" : "not ");

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
    runqueue = create_queue();
    alreadyqueue = create_queue();

    init_context();
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

