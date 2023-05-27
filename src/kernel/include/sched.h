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

#ifndef _SCHED_H
#define _SCHED_H

#include <types.h>

// Should be in the arch-specific includes directory.
#include_next <sched.h>

#include <annotate.h>

#ifndef _CONTEXT_T_DEFINED
#define _CONTEXT_T_DEFINED
#warning This architecture does not define a context_t!
typedef void context_t;
#endif

#define PROCESS_NAME_MAX    64

#define DEFAULT_PROCESS_NAME    "<untitled process>"

#define THREAD_PRIORITY_REALTIME       0
#define THREAD_PRIORITY_VERYHIGH       20
#define THREAD_PRIORITY_HIGH           40
#define THREAD_PRIORITY_MEDIUM         60
#define THREAD_PRIORITY_LOW            79

#define THREAD_STATE_RUNNING            0
#define THREAD_STATE_READY              1
#define THREAD_STATE_ZOMBIE             2
#define THREAD_STATE_SLEEPING           3
#define THREAD_STATE_ALREADY            100

/// Default timeslice in nanoseconds: this is 10 ms.
#define THREAD_DEFAULT_TIMESLICE_MS     10
#define THREAD_DEFAULT_TIMESLICE        (THREAD_DEFAULT_TIMESLICE_MS*1000000)

typedef void (*thread_entry_t)(void*);

struct thread {
    context_t *ctx;

    uint32_t state;

    uint64_t timeslice;

    /**
     * Base priority: the priority of the thread cannot exceed this.
     * This avoids threads with high I/O managing to achieve realtime
     * priority when they give up their timeslice all the time.
     */
    uint32_t base_priority;

    /// Actual, current, priority of the thread.
    uint32_t priority;

    /// Is this the idle thread?
    uint8_t isidle;

    struct process *parent;
};

/** A process. */
struct process {
    char name[PROCESS_NAME_MAX];

    struct process *next;

    void *child_list;
    void *thread_list;

    /// Process ID.
    size_t pid;
};

/** Creates a process and returns a pointer to it. */
extern struct process *create_process(const char *name, struct process *parent);

/**
 * Creates a new thread under a given process.
 * A zero stack or stacksz parameter will allocate a new stack for this thread.
 */
extern struct thread *create_thread(struct process *parent, uint32_t prio, thread_entry_t start, uintptr_t stack, size_t stacksz, void *param);

/** Performs a context switch to a new context. */
extern void switch_context(context_t *oldctx, context_t *newctx, unative_t wasints, void *lock);

/** Saves the given thread's context. Returns zero when restored from the saved context. */
extern __returns_twice int save_thread_context(context_t *ctx);

/** Restores the given thread's context. */
extern __noreturn int restore_thread_context(context_t *ctx, void *lock);

/** Creates a new context (archictecture-specific). */
extern void create_context(context_t *ctx, thread_entry_t start, uintptr_t stack, size_t stacksz, void *param);

/** Clones the given old context into the new context. */
void clone_context(context_t *old, context_t *new);

/** Destroys the given context (architecture-specific). */
extern void destroy_context(context_t *ctx);

/**
 * Kills the given thread.
 * Technically, adds the thread to the zombie queue for future reaping.
 */
extern void thread_kill() __noreturn;

/** Puts the current thread to sleep (MUST be woken, no time for this one). */
extern void thread_sleep();

/** Wakes up a thread (threads begin in the SLEEPING state). */
extern void thread_wake(struct thread *thr);

/** Reads the current priority of a thread. Pass NULL for the current thread. */
extern uint32_t thread_priority(struct thread *prio);

/** Performs a reschedule. */
extern void reschedule();

/** Initialises the scheduler. */
extern void init_scheduler();

/** Begins the scheduler proper: pre-emption rather than co-operative multitasking. */
extern void start_scheduler();

/** Get the currently running thread. */
extern struct thread *sched_current_thread();

/** Yields the current timeslice immediately. */
extern void sched_yield();

/** Defines the thread which is the 'idle' thread in the system. */
extern void sched_setidle(struct thread *t);

/** Notifies the scheduler of a new CPU being active. */
extern void sched_cpualive(void *lock);

/** Kickstarts the cooperative phase of the scheduler to begin scheduling. */
extern void sched_kickstart();

/** Function to be called when a thread returns from its entry point. */
extern void thread_return() __noreturn;

#endif
