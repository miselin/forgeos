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

#ifndef _SPINLOCK_H
#define _SPINLOCK_H

#include "annotate.h"

typedef void *spinlock_t CAPABILITY("mutex");

/**
 * Create a spinlock.
 */
extern spinlock_t create_spinlock();

/**
 * Create a spinlock in the given buffer. Useful if memory allocation is not yet available.
 */
extern spinlock_t create_spinlock_at(void *static_region, size_t static_region_size);

/**
 * Destroy a spinlock.
 */
extern void delete_spinlock(spinlock_t s);

/**
 * Get a pointer to the spinlock's atomic state (ie, the 'locked'/'unlocked' state.
 */
extern void *spinlock_getatom(spinlock_t s);

/**
 * Get the interrupt state before the spinlock was acquired.
 */
uint8_t spinlock_intstate(spinlock_t s);

/**
 * Acquire the lock. Blocks if the lock is already attained.
 * On single-processor systems, if the lock is already attained and interrupts are not
 * enabled, a kernel panic will occur due to a probable deadlock.
 * Disables interrupts as a side-effect.
 */
extern void spinlock_acquire(spinlock_t s) ACQUIRE(s) NO_THREAD_SAFETY_ANALYSIS;

/**
 * Releases the lock.
 */
extern void spinlock_release(spinlock_t s) RELEASE(s) NO_THREAD_SAFETY_ANALYSIS;

#endif
