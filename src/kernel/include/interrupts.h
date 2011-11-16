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

#ifndef _INTERRUPTS_H
#define _INTERRUPTS_H

#include <stdint.h>
#include <stack.h> // Pull in the architecture interrupt stack struct.

typedef int (*inthandler_t)(struct intr_stack *);

/// Initialises interrupts for this system.
#define interrupts_init		arch_interrupts_init

/// Enables interrupts
#define interrupts_enable	arch_interrupts_enable

/// Disables interrupts
#define interrupts_disable	arch_interrupts_disable

/// Registers a new software interrupt
#define interrupts_trap_reg	arch_interrupts_reg

/// Registers a new hardware interrupt
#define interrupts_irq_reg	mach_interrupts_reg

extern void arch_interrupts_init();

extern void arch_interrupts_enable();
extern void arch_interrupts_disable();

extern void arch_interrupts_reg(int n, inthandler_t handler);
extern void mach_interrupts_reg(int n, int leveltrig, inthandler_t handler);

#endif
