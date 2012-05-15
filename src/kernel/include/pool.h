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
#ifndef POOL_H
#define POOL_H

#include <types.h>

/** Initialises the memory pool implementation. */
extern void init_pool();

/** Creates a new memory pool with a certain number of buffers of a given size. */
extern void *create_pool(size_t buffsz, size_t buffcnt);

/**
 * Creates a new memory pool with a certain number of buffers of a given size,
 * at a particular memory address.
 */
extern void *create_pool_at(size_t buffsz, size_t buffcnt, uintptr_t addr);

/** Allocates a buffer from a pool. */
extern void *pool_alloc(void *pool);

/** Returns a given buffer to a pool. */
extern void pool_dealloc(void *pool, void *p);

/** Returns a given buffer to a pool, and releases the memory for it. */
extern void pool_dealloc_and_free(void *pool, void *p);

/** Returns the number of buffers allocated for a pool. */
extern size_t pool_count(void *pool);

#endif

