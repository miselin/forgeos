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
#include <assert.h>
#include <panic.h>
#include <util.h>

struct spinlock {
	uint8_t locked;
};

void *create_spinlock() {
	void *r = malloc(sizeof(struct spinlock));
	memset(r, 0, sizeof(struct spinlock));
	return r;
}

void delete_spinlock(void *s) {
	if(!s)
		return;

	struct spinlock *sl = (struct spinlock *) s;
	sl->locked = 0;
	free(s);
}

void spinlock_acquire(void *s) {
	if(!s)
		return;

	struct spinlock *sl = (struct spinlock *) s;

	/// \todo Check for multiple processors.
	if(sl->locked)
		panic("spinlock already acquired - deadlock");

	interrupts_disable();
	atomic_compare_and_swap(&sl->locked, 1, sl->locked, __asm__ (""));


	assert(sl->locked);
}

void spinlock_release(void *s) {
	if(!s)
		return;

	struct spinlock *sl = (struct spinlock *) s;
	sl->locked = 0;

	/// \todo panic if interrupts are enabled.
	/// \todo restore old interrupt state (don't assume 'enabled').

	interrupts_enable();
}
