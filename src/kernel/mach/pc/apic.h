/*
 * Copyright (c) 2012 Matthew Iselin
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

#ifndef _APIC_H
#define _APIC_H

#include <interrupts.h>
#include <types.h>

#define LAPIC_VERSION_82489DX       0
#define LAPIC_VERSION_INTEGRATED    1

extern int init_apic();
extern void init_lapic();

extern void apic_interrupt_reg(int n, int leveltrig, inthandler_t handler, void *p);

/// Perform an Inter-Processor Interrupt
extern void lapic_ipi(uint8_t dest_proc, uint8_t vector, uint32_t delivery, uint8_t bassert, uint8_t level);

/// Perform a broadcast Inter-Processor Interrupt
void lapic_bipi(uint8_t vector, uint32_t delivery);

extern uint32_t lapic_ver();

#endif
