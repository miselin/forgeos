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

#define POOL_STACK_SZ       0x4000
#define POOL_STACK_COUNT    0x1000

#define EFLAGS_INT_ENBALE   (1UL << 9)

static void *stackpool = 0;

void init_context() {
    // Create a pool for stacks - 4096, 4KB stacks = 16 MB maximum pool size.
    stackpool = create_pool_at(POOL_STACK_SZ, POOL_STACK_COUNT, (STACK_TOP - STACK_SIZE) - (POOL_STACK_SZ * POOL_STACK_COUNT));
}

void create_context(context_t *ctx, thread_entry_t start, uintptr_t stack, size_t stacksz, void *param) {
    assert(stackpool != 0);

    memset(ctx, 0, sizeof(context_t));

    if(!stacksz) {
        stacksz = POOL_STACK_SZ;
    }

    void *stack_ptr = 0;
    if(stack) {
        stack_ptr = (void *) stack;
        ctx->stackispool = 0;
    } else {
        stack_ptr = pool_alloc(stackpool);
        ctx->stackispool = 1;
    }

    assert(stack_ptr != 0);

    ctx->eip = (uint32_t) start;
    ctx->stackbase = (uint32_t) stack_ptr;
    ctx->eflags = EFLAGS_INT_ENBALE;

    uintptr_t stack_top = ((uintptr_t) stack_ptr) + (stacksz - sizeof(unative_t));
    ctx->ebp = stack_top;

    uintptr_t *stackp = (uintptr_t *) stack_top;
    *stackp = (uintptr_t) thread_return;
    stackp--;
    *stackp = (uintptr_t) param;

    ctx->esp = (uint32_t) stackp;

    dprintf("new x86 context %p: eip=%x, esp=%x, ebp=%x (stack: %x-%x)\n", ctx, ctx->eip, ctx->esp, ctx->ebp, ctx->stackbase, stack_top + sizeof(unative_t));
}

void clone_context(context_t *old, context_t *new) {
    memcpy(new, old, sizeof(*old));
    assert(old->stackispool);

    void *stack = pool_alloc(stackpool);
    memcpy(stack, (void *) old->stackbase, POOL_STACK_SZ);

    new->stackbase = (uint32_t) stack;

    unative_t ebp_diff = old->esp - old->stackbase;
    unative_t esp_diff = old->esp - old->stackbase;

    new->ebp = new->stackbase + ebp_diff;
    new->esp = new->stackbase + esp_diff;

    dprintf("cloned x86 context %p -> %p: eip=%x, esp=%x\n", old, new, new->eip, new->esp);
}

void destroy_context(context_t *ctx) {
    assert(ctx != 0);

    if(ctx->stackispool)
        pool_dealloc_and_free(stackpool, (void *) ctx->stackbase);
}
