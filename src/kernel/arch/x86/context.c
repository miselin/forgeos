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
#include <io.h>

void create_context(context_t *ctx, thread_entry_t start, uintptr_t stack, size_t stacksz) {
    memset(ctx, 0, sizeof(context_t));
    
    /// \todo Don't use malloc!
    void *stack_ptr = 0;
    if(stack)
        stack_ptr = (void *) stack;
    else
        stack_ptr = (void *) malloc(stacksz);
    
    ctx->eip = (uint32_t) start;
    ctx->esp = ctx->ebp = (uint32_t) ((char *) stack_ptr) + (stacksz - 4);
    
    dprintf("new x86 context %x: eip=%x, esp=%x-%x\n", ctx, ctx->eip, ctx->esp - stacksz + 4, ctx->esp);
}

