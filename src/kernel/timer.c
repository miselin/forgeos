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
#include <timer.h>
#include <io.h>

extern int __begin_timer_table, __end_timer_table;

void timers_init() {
	uintptr_t begin = (uintptr_t) &__begin_timer_table;
	uintptr_t end = (uintptr_t) &__end_timer_table;
	struct timer_table_entry *ent = (struct timer_table_entry *) begin;

	for(size_t i = 0; i < (end - begin) / sizeof(struct timer_table_entry); i++) {
		if(ent->tmr && (ent->tmr->timer_init != 0)) {
			kprintf("init timer %s: ", ent->tmr->name);
			int rc = ent->tmr->timer_init();
			if(!rc)
				kprintf("OK\n");
			else
				kprintf("FAIL\n");
		}
	}
}

void timer_ticked(struct timer *tim, uint32_t ticks) {
	kprintf("timer %s tick: %x\n", tim->name, ticks);
}

int install_timer(timer_handler th, uint32_t ticks, uint32_t feat) {
	return -1;
}

void remove_timer(timer_handler th) {
}
