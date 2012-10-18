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

#include <multicpu.h>

// #define SPAM_THE_LOGS

extern int __begin_timer_table, __end_timer_table;

static void * volatile timer_list = 0;

static void * hwtimer_list = 0;

struct timer_handler_meta {
	struct timer *tim;
	timer_handler th;
	uint64_t ticks;
	uint64_t orig_ticks;
	uint32_t feat;

	uint32_t cpu;
};

struct crosscpu_th {
	timer_handler th;
	uint64_t ticks;
};

#define HW_TIMER_COUNT		(list_len(hwtimer_list))
#define GET_HW_TIMER(n)		((struct timer *) list_at(hwtimer_list, (n)))
#define GET_TIMER_RES(n)	(GET_HW_TIMER(n)->timer_res & TIMERRES_MASK)

#define GET_STATIC_TIMER(n) ((struct timer_table_entry *) &__begin_timer_table)[(n)]
#define STATIC_TIMER_COUNT	((((uintptr_t) &__end_timer_table) - ((uintptr_t) &__begin_timer_table)) / sizeof(struct timer_table_entry))

void timers_init() {
	// Initialise all timers now.
	size_t i;
	dprintf("timers_init: %d static timers\n", STATIC_TIMER_COUNT);
	for(i = 0; i < STATIC_TIMER_COUNT; i++) {
		struct timer_table_entry ent = GET_STATIC_TIMER(i);
		dprintf("registering %s!?\n", ent.tmr->name);
		timer_register(ent.tmr);
	}
}

void timer_register(struct timer *tim) {
	if(!hwtimer_list) {
		hwtimer_list = create_list();
	}

	if(tim && (tim->timer_init != 0)) {
		kprintf("init timer %s: ", tim->name);
		int rc = tim->timer_init();
		if(!rc) {
			kprintf("OK\n");

			tim->cpu = multicpu_id();

			list_insert(hwtimer_list, tim, 0);
		} else {
			kprintf("FAIL\n");

			// Failed - timer can't offer anything.
			tim->timer_feat = 0;
		}
	}
}

/// Convert ticks to the highest possible resolution (nanoseconds)
static uint64_t conv_ticks(uint32_t ticks) {
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

static void timer_crosscpu_stub(struct crosscpu_th *meta) {
	meta->th(meta->ticks);
	free(meta);
}

static int do_th(struct timer_handler_meta *p, uint64_t ticks) {
	if(p->cpu == multicpu_id())
		return p->th(ticks);
	else {
		struct crosscpu_th *crossmeta = (struct crosscpu_th *) malloc(sizeof(struct crosscpu_th));
		crossmeta->th = p->th;
		crossmeta->ticks = ticks;
		multicpu_call(p->cpu, (crosscpu_func_t) timer_crosscpu_stub, (void *) crossmeta);
	}
	return 0;
}

int timer_ticked(struct timer *tim, uint32_t in_ticks) {
#ifdef SPAM_THE_LOGS
	dprintf("timer %s tick: %x\n", tim->name, in_ticks);
#endif

	struct timer_handler_meta *p = 0;
	size_t index = 0;
	int ret = 0;

	// Convert the ticks.
	uint64_t ticks = conv_ticks(in_ticks);

#ifdef SPAM_THE_LOGS
	dprintf("%d %d ns\n", (uint32_t) (ticks >> 32), (uint32_t) ticks);
#endif

	while((p = (struct timer_handler_meta *) list_at(timer_list, index++))) {
		// Ignore if not for this timer.
		if(p->tim != tim)
			continue;

		// Expire one-shot timers, if possible.
		if((tim->timer_feat & TIMERFEAT_ONESHOT) != 0) {
			if(((p->feat & TIMERFEAT_ONESHOT) != 0) && (p->ticks == ticks)) {
				ret += do_th(p, ticks);
				list_remove(timer_list, --index);
			}
		}

		// Handle periodic timers.
		if((tim->timer_feat & TIMERFEAT_PERIODIC) != 0) {
			if((p->feat & TIMERFEAT_PERIODIC) != 0) {
				if(p->ticks < ticks)
					p->ticks = 0;
				else
					p->ticks -= ticks;

				if(!p->ticks) {
					ret += do_th(p, ticks > p->orig_ticks ? ticks : p->orig_ticks);
					p->ticks = p->orig_ticks; // Reload the timer.
				}
			}
		}

		// One-shot emulation.
		if(((tim->timer_feat & TIMERFEAT_ONESHOT) == 0) && ((tim->timer_feat & TIMERFEAT_PERIODIC) != 0)) {
			if((p->feat & TIMERFEAT_ONESHOT) != 0) {
				if(p->ticks < ticks)
					p->ticks = 0;
				else
					p->ticks -= ticks;

				if(!p->ticks) {
					ret += do_th(p, ticks > p->orig_ticks ? ticks : p->orig_ticks);
					list_remove(timer_list, --index);
				}
			}
		}
	}

	return ret ? 1 : 0;
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
		struct timer *ent = GET_HW_TIMER(i);

		// Ignore per-CPU timers that aren't for this CPU.
		if((ent->timer_feat & TIMERFEAT_PERCPU) && (ent->cpu != multicpu_id())) {
			continue;
		}

		// If this is a per-CPU timer and that is not being requested, skip this
		// timer. This ensures that global timers are used with preference to
		// local timers.
		if((ent->timer_feat & TIMERFEAT_PERCPU) && (!(feat & TIMERFEAT_PERCPU))) {
			continue;
		}

		// Match features.
		if((ent->timer_feat & feat) != 0) {
			// Ensure that the timer has a higher resolution than we require here.
			// A lower-resolution timer may be acceptable as a last resort, but we
			// want the best possible option!
			if((ent->timer_res & TIMERRES_MASK) <= (ticks & TIMERRES_MASK)) {
				dprintf("timer %s is acceptable for this timer handler\n", ent->name);
				p->tim = ent;
				break;
			} else {
				dprintf("timer %s matches requested features, but does not have an acceptable resolution.\n", ent->name);
			}
		}
	}

	// If no exact match can be found, we will need to:
	// a) Do oneshot emulation, or
	// b) Convert the resolution.
	if(p->tim == 0) {
		for(size_t i = 0; (i < HW_TIMER_COUNT) && (p->tim == 0); i++) {
			struct timer *ent = GET_HW_TIMER(i);

			// Ignore per-CPU timers that aren't for this CPU.
			if((ent->timer_feat & TIMERFEAT_PERCPU) && (ent->cpu != multicpu_id())) {
				continue;
			}

			// Feature match?
			if((ent->timer_feat & feat) != 0) {
				dprintf("accepting timer %s because its features match, but the resolution may cause unexpected behaviour.\n", ent->name);
				p->tim = ent;
			} else if(((feat & TIMERFEAT_ONESHOT) != 0) && ((ent->timer_feat & TIMERFEAT_PERIODIC) != 0)) {
				dprintf("accepting timer %s as it can be used for one-shot emulation.\n", ent->name);
				p->tim = ent; // One-shot emulation.
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
	p->cpu = multicpu_id();

	// Insert in order - lowest ticks first, highest last. This allows us to always
	// handle the closest timer to completion first.
	size_t i = 0;
	struct timer_handler_meta *tmp = 0;
	while((tmp = (struct timer_handler_meta *) list_at(timer_list, i++))) {
		if(tmp->ticks >= p->ticks)
			break;
	}

	list_insert(timer_list, p, i);

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
