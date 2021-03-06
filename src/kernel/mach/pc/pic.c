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
#include <interrupts.h>
#include <stack.h>
#include <io.h>

#define IRQ_INT_BASE		32

struct irqhandler {
	inthandler_t	handler;
	int				leveltrig;
	void			*param;
};

static struct irqhandler handlers[16];

static void eoi(size_t irqnum) {
	outb(0x20, 0x20);
	if(irqnum > 7)
		outb(0xA0, 0x20);
}

static void disable(size_t irqnum) {
	if(irqnum <= 7) {
		outb(0x21, inb(0x21) | (uint8_t) (1 << irqnum));
	} else {
		outb(0xA1, inb(0xA1) | (uint8_t) (1 << (irqnum - 8)));
	}
}

static void enable(size_t irqnum) {
	if(irqnum <= 7) {
		outb(0x21, inb(0x21) & (uint8_t) ~(1 << irqnum));
	} else {
		outb(0xA1, inb(0xA1) & (uint8_t) ~(1 << (irqnum - 8)));
	}
}

static int irq_stub(struct intr_stack *stack, void *p __unused) {
	size_t irqnum = stack->intnum - IRQ_INT_BASE;
	int ret = 0;

	// Read the interrupt status register for master and slave
	outb(0x20, 0x0b);
	outb(0xa0, 0x0b);
	uint16_t isr = (inb(0xa0) << 8) | inb(0x20);

	// Handle spurious IRQs.
	if(irqnum == 7) {
		// Spurious?
		if((isr & (1 << 7)) == 0) {
			dprintf("pic: spurious irq 7\n");
			return 0;
		}
	} else if(irqnum == 15) {
		// Spurious?
		if((isr & (1 << 15)) == 0) {
			dprintf("pic: spurious irq 15\n");
			eoi(7);
			return 0;
		}
	}

	// Legit call into the IRQ stub?
	if((isr & (1 << irqnum)) == 0) {
		dprintf("pic: irq %d came from nowhere!?\n", irqnum);
		return 0;
	}

	if(handlers[irqnum].handler == 0) {
		dprintf("Unhandled IRQ %d\n", irqnum);
		eoi(irqnum);
		return 0;
	}

	// EOI is sent first if not level triggered.
	if(handlers[irqnum].leveltrig == 0)
		eoi(irqnum);

	// Call the handler.
	/// \todo Allow multiple IRQ handlers for one IRQ.
	ret = handlers[irqnum].handler(stack, handlers[irqnum].param);

	// Send an EOI if level triggered.
	if(handlers[irqnum].leveltrig > 0)
		eoi(irqnum);

	return ret;
}

void init_pic() {
	size_t i = 0;

	// Remap the PIC
	outb(0x20, 0x11);
	outb(0xA0, 0x11);
	outb(0x21, 0x20);
	outb(0xA1, 0x28);
	outb(0x21, 0x04);
	outb(0xA1, 0x02);
	outb(0x21, 0x01);
	outb(0xA1, 0x01);
	outb(0x21, 0x0);
	outb(0xA1, 0x0);

	// Install IRQ handlers.
	for(i = 0 ; i < 16; i++) {
		handlers[i].handler = 0;
		handlers[i].leveltrig = 0;
		disable(i);

		arch_interrupts_reg((int) i + IRQ_INT_BASE, irq_stub);
	}
}

void pic_interrupt_reg(int n, int leveltrig, inthandler_t handler, void *p) {
	handlers[n].handler = handler;
	handlers[n].leveltrig = leveltrig;
	handlers[n].param = p;
	enable((size_t) n);
}
