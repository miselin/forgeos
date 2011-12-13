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
	struct timer *tim;
	timer_handler th;
	uint64_t ticks;
	uint64_t orig_ticks;
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

/// Convert ticks to the highest possible resolution (nanoseconds)
inline uint64_t conv_ticks(uint32_t ticks) {
	uint64_t ret = ticks;

	if((ret & TIMERRES_SECONDS) != 0) {
		ret = (ret >> TIMERRES_SHIFT) * 1000000000;
	} else if((ret & TIMERRES_MILLI) != 0) {
		ret = (ret >> TIMERRES_SHIFT) * 1000000;
	} else if((ret & TIMERRES_MICRO) != 0) {
		ret = (ret >> TIMERRES_SHIFT) * 1000;
	} else if((ret & TIMERRES_NANO) != 0) {
		ret = ret >> TIMERRES_SHIFT;
	} else {
		ret = 0;
	}

	return ret;
}

void timer_ticked(struct timer *tim, uint32_t in_ticks) {
	dprintf("timer %s tick: %x\n", tim->name, in_ticks);

	struct timer_handler_meta *p = 0;
	size_t index = 0;

	// Convert the ticks.
	uint64_t ticks = conv_ticks(in_ticks);

	/// \todo This doesn't work if there's ever more than one timer in the system. We should be selecting
	///		  the best timer when a handler is installed, based on resolution and such.

	while((p = (struct timer_handler_meta *) list_at(timer_list, index++))) {
		// Ignore if not for this timer.
		if(p->tim != tim)
			continue;

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

	dprintf("installing timer handler %x with %x ticks, looking for features %x\n", th, ticks, feat);

	struct timer_handler_meta *p = (struct timer_handler_meta *) malloc(sizeof(struct timer_handler_meta));
	p->tim = 0;

	// Find a timer that is most effective for these features.
	// Also, try and match the resolution if at all possible.
	for(size_t i = 0; i < HW_TIMER_COUNT; i++) {
		struct timer_table_entry ent = GET_HW_TIMER(i);

		// Match features.
		if((ent.tmr->timer_feat & feat) != 0) {
			// Ensure that the timer has a higher resolution than we require here.
			// A lower-resolution timer may be acceptable as a last resort, but we
			// want the best possible option!
			if((ent.tmr->timer_res & TIMERRES_MASK) >= (ticks & TIMERRES_MASK)) {
				dprintf("timer %s is acceptable for this timer handler\n", ent.tmr->name);
				p->tim = ent.tmr;
				break;
			} else {
				dprintf("timer %s matches requested features, but does not have an acceptable resolution.\n", ent.tmr->name);
			}
		}
	}

	// If no exact match can be found, we will need to:
	// a) Do oneshot emulation, or
	// b) Convert the resolution.
	if(p->tim == 0) {
		for(size_t i = 0; (i < HW_TIMER_COUNT) && (p->tim == 0); i++) {
			struct timer_table_entry ent = GET_HW_TIMER(i);

			dprintf("timer %s with feat %x\n", ent.tmr->name, ent.tmr->timer_feat);

			// Feature match?
			if((ent.tmr->timer_feat & feat) != 0) {
				dprintf("accepting timer %s because it's features match, but the resolution may cause unexpected behaviour.\n", ent.tmr->name);
				p->tim = ent.tmr;
			} else if(((feat & TIMERFEAT_ONESHOT) != 0) && ((ent.tmr->timer_feat & TIMERFEAT_PERIODIC) != 0)) {
				dprintf("accepting timer %s as it can be used for one-shot emulation.\n", ent.tmr->name);
				p->tim = ent.tmr; // One-shot emulation.
			}
		}
	}

	if(p->tim == 0) {
		dprintf("could not find an acceptable timer for this timer handler.\n");
		free(p);
		return -1;
	}

	p->th = th;
	p->ticks = p->orig_ticks = conv_ticks(ticks);
	p->feat = feat;

	list_insert(timer_list, p, 0);

	return 0;
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
