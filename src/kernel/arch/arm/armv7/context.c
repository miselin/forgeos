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
#include <util.h>
#include <malloc.h>
#include <pool.h>
#include <io.h>
#include <system.h>
#include <assert.h>

#define POOL_STACK_SZ       0x1000
#define POOL_STACK_COUNT    0x1000

static void *stackpool = 0;

struct initial_data {
    void *param;
    thread_entry_t start;
};

/// Entry point for ARM threads - handles return from the threads properly.
void arm_thread(void *data) {
    struct initial_data *d = (struct initial_data *) data;

    thread_entry_t start = d->start;
    void *param = d->param;

    free(d);

    start(param);

    thread_return();
}

void init_context() {
    // Create a pool for stacks - 4096, 4KB stacks = 16 MB maximum pool size.
    stackpool = create_pool_at(POOL_STACK_SZ, POOL_STACK_COUNT, (STACK_TOP - STACK_SIZE) - (POOL_STACK_SZ * POOL_STACK_COUNT));
}

void create_context(context_t *ctx, thread_entry_t start, uintptr_t stack, size_t stacksz, void *param) {
    assert(stackpool != 0);

    memset(ctx, 0, sizeof(context_t));

    void *stack_ptr = 0;
    if(stack) {
        stack_ptr = (void *) stack;
    } else {
        stack_ptr = (void *) pool_alloc(stackpool);
        ctx->stackispool = 1;
    }

    assert(stack_ptr != 0);

    struct initial_data *d = (struct initial_data *) malloc(sizeof(struct initial_data));
    d->start = start;
    d->param = param;

    ctx->lr = (unative_t) arm_thread;
    ctx->usersp = ctx->stackbase = (unative_t) stack_ptr;
    ctx->usersp += stacksz - 4;

    ctx->r0 = (unative_t) d;

    dprintf("new arm context %p\n", ctx);
}

void destroy_context(context_t *ctx) {
    assert(ctx != 0);

    if(ctx->stackispool) {
        pool_dealloc_and_free(stackpool, (void *) ctx->stackbase);
    }
}
