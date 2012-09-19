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

#ifndef _SEMAPHORE_H
#define _SEMAPHORE_H

#include <types.h>

/// Creates a new semaphore, with the given semantics.
extern void *create_semaphore(size_t max, size_t initial);

/// Destroys the given semaphore, unblocking all blocked threads.
extern void delete_semaphore(void *sem);

/// Acquire the given count on the given semaphore
extern void semaphore_acquire(void *sem, size_t count);

/// Try and acquire the given count on the given semaphore.
extern int semaphore_tryacquire(void *sem, size_t count);

/// Release the given count on the given semaphore.
extern void semaphore_release(void *sem, size_t count);

#endif
