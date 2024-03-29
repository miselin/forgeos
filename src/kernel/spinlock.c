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

#include <types.h>
#include <malloc.h>
#include <system.h>
#include <interrupts.h>
#include <spinlock.h>
#include <multicpu.h>
#include <assert.h>
#include <panic.h>
#include <util.h>
#include <sched.h>
#include <io.h>

struct spinlock {
	uint32_t locked;
	uint8_t wasints;
	uint8_t isstatic;
};

spinlock_t create_spinlock() {
	void *r = malloc(sizeof(struct spinlock));
	memset(r, 0, sizeof(struct spinlock));
	return r;
}

spinlock_t create_spinlock_at(void *static_region, size_t static_region_size) {
	assert(static_region_size >= sizeof(struct spinlock));
	memset(static_region, 0, sizeof(struct spinlock));
	return (spinlock_t) static_region;
}

void delete_spinlock(spinlock_t s) {
	if(!s)
		return;

	struct spinlock *sl = (struct spinlock *) s;
	sl->locked = 0;
	free(s);
}

void *spinlock_getatom(spinlock_t s) {
	if(!s)
		return NULL;

	struct spinlock *sl = (struct spinlock *) s;
	return (void *) &sl->locked;
}

uint8_t spinlock_intstate(spinlock_t s) {
	if(!s)
		return 0;

	struct spinlock *sl = (struct spinlock *) s;
	return sl->wasints;
}

void spinlock_acquire(spinlock_t s) {
	if(!s)
		return;

	struct spinlock *sl = (struct spinlock *) s;

	uint8_t wasints = interrupts_get();
	interrupts_disable();
	atomic_compare_and_swap(&sl->locked, 1, void * _a __unused, 0, if(multicpu_count() == 1) { dprintf("deadlock in spinlock %p\n", s); panic("deadlock"); } __spin;);

	sl->wasints = wasints;

	assert(sl->locked);
}

void spinlock_release(spinlock_t s) {
	if(!s)
		return;

	uint8_t wasints = interrupts_get();

	// If uniprocessor and interrupts were enabled, then it is an error condition.
	// Multiprocessors systems can acquire on one CPU and release on another.
	if(wasints && (multicpu_count() == 1))
		panic("spinlock released with interrupts enabled!");

	struct spinlock *sl = (struct spinlock *) s;
	assert(sl->locked);

	wasints = sl->wasints;
	atomic_compare_and_swap(&sl->locked, 0, void * _a __unused, 1, if(multicpu_count() == 1) { dprintf("deadlock in spinlock %p\n", s); panic("deadlock"); } __spin;);

	if(wasints)
		interrupts_enable();
	else
		interrupts_disable();
}
