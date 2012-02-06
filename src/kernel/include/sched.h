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

#ifndef _CONTEXT_T_DEFINED
#define _CONTEXT_T_DEFINED
#warning This architecture does not define a context_t!
typedef void context_t;
#endif

#define PROCESS_NAME_MAX    64

#define DEFAULT_PROCESS_NAME    "<untitled process>"

typedef void (*thread_entry_t)();

struct thread {
    context_t *ctx;
};

/** A process. */
struct process {
    char name[PROCESS_NAME_MAX];
    
    struct process *next;
    
    void *child_list;
    void *thread_list;
};

/** Creates a process and returns a pointer to it. */
extern struct process *create_process(const char *name, struct process *parent);

/**
 * Creates a new thread under a given process.
 * A zero stack or stacksz parameter will allocate a new stack for this thread.
 */
extern struct thread *create_thread(struct process *parent, thread_entry_t start, uintptr_t stack, size_t stacksz);

/** Performs a context switch to a new context. */
extern void switch_context(context_t **oldctx, context_t *newctx);

/** Performs a context switch between two threads. */
extern void switch_threads(struct thread *old, struct thread *new);

/** Creates a new context (archictecture-specific). */
extern void create_context(context_t *ctx, thread_entry_t start, uintptr_t stack, size_t stacksz);

#endif
