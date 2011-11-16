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

#include <stdint.h>
#include <interrupts.h>
#include <compiler.h>
#include <stack.h>
#include <pit.h>
#include <io.h>

/// Frequency at which we want the timer to tick.
#define TIMER_TICK_HZ	100

#define PIT_PORT		0x40

volatile size_t ticks = 0;

/// Timer IRQ handler
static int pic_irq(struct intr_stack *p) {
	ticks += 1;
	if((ticks % TIMER_TICK_HZ) == 0)
		kprintf("PIC: one second has passed.\n");
	return 0;
}

void init_pit() {
	// Set the divisor to achieve the frequency we want.
	size_t div = 1193180 / TIMER_TICK_HZ;

	// Configure the divisor
	outb(PIT_PORT + 3, 0x36);
	outb(PIT_PORT + 0, div & 0xFF);
	outb(PIT_PORT + 0, (div >> 8) & 0xFF);

	// Install the IRQ handler
	interrupts_irq_reg(0, 0, pic_irq);
}
