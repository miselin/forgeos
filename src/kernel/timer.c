/*
 * Copyright (c) 2011 Matthew Iselin, Rich Edelman
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
#include <assert.h>
#include <timer.h>
#include <util.h>
#include <malloc.h>
#include <io.h>

extern int __begin_timer_table, __end_timer_table;

static void * volatile timer_list = 0;

struct timer_handler_meta {
	timer_handler th;
	uint32_t ticks;
	uint32_t orig_ticks;
	uint32_t feat;
};

#define GET_HW_TIMER(n) ((struct timer_table_entry *) &__begin_timer_table)[(n)]
#define HW_TIMER_COUNT	((((uintptr_t) &__end_timer_table) - ((uintptr_t) &__begin_timer_table)) / sizeof(struct timer_table_entry))

void timers_init() {
	for(size_t i = 0; i < HW_TIMER_COUNT; i++) {
		struct timer_table_entry ent = GET_HW_TIMER(i);
		if(ent.tmr && (ent.tmr->timer_init != 0)) {
			kprintf("init timer %s: ", ent.tmr->name);
			int rc = ent.tmr->timer_init();
			if(!rc)
				kprintf("OK\n");
			else
				kprintf("FAIL\n");
		}
	}
}

void timer_ticked(struct timer *tim, uint32_t ticks) {
	dprintf("timer %s tick: %x\n", tim->name, ticks);

	struct timer_handler_meta *p = 0;
	size_t index = 0;

	/// \todo This doesn't work if there's ever more than one timer in the system. We should be selecting
	///		  the best timer when a handler is installed, based on resolution and such.

	while((p = (struct timer_handler_meta *) list_at(timer_list, index++))) {
		// Expire one-shot timers, if possible.
		if((tim->timer_feat & TIMERFEAT_ONESHOT) != 0) {
			if(((p->feat & TIMERFEAT_ONESHOT) != 0) && (p->ticks == ticks)) {
				p->th(ticks);
				list_remove(timer_list, index--);
			}
		}

		// Handle periodic timers.
		if((tim->timer_feat & TIMERFEAT_PERIODIC) != 0) {
			if((p->feat & TIMERFEAT_PERIODIC) != 0) {
				if(ticks >= p->ticks)
					ticks = p->ticks;
				p->ticks -= ticks;

				if(!p->ticks) {
					p->th(p->orig_ticks);
					p->ticks = p->orig_ticks; // Reload the timer.
				}
			}
		}

		// One-shot emulation.
		if(((tim->timer_feat & TIMERFEAT_ONESHOT) == 0) && ((tim->timer_feat & TIMERFEAT_PERIODIC) != 0)) {
			if((p->feat & TIMERFEAT_ONESHOT) != 0) {
				if(ticks >= p->ticks)
					ticks = p->ticks;
				p->ticks -= ticks;

				if(!p->ticks) {
					p->th(p->orig_ticks);
					list_remove(timer_list, index--);
				}
			}
		}
	}
}

int install_timer(timer_handler th, uint32_t ticks, uint32_t feat) {
	if(timer_list == 0) {
		timer_list = create_list();
	}

	assert(timer_list != 0);

	struct timer_handler_meta *p = (struct timer_handler_meta *) malloc(sizeof(struct timer_handler_meta));
	p->th = th;
	p->ticks = p->orig_ticks = ticks;
	p->feat = feat;

	list_insert(timer_list, p, 0);

	return -1;
}

void remove_timer(timer_handler th) {
	if(timer_list == 0)
		return;

	size_t index = 0;
	struct timer_handler_meta *p = 0;
	while((p = (struct timer_handler_meta *) list_at(timer_list, index++))) {
		if(p->th == th)
			list_remove(timer_list, index--);
	}
}
