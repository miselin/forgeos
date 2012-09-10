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

static int set(int n) {
    uint32_t cpsr = 0;
    __asm__ __volatile__("MRS %0, cpsr" : "=r" (cpsr));
    if(n)
        cpsr &= ~0x80UL;
    else
        cpsr |= 0x80;
    __asm__ __volatile__("MSR cpsr_c, %0" :: "r" (cpsr));

    return n;
}

void arch_interrupts_enable() {
	set(1);
}

void arch_interrupts_disable() {
	set(0);
}

int arch_interrupts_get() {
    uint32_t cpsr = 0;
    __asm__ __volatile__("MRS %0, cpsr" : "=r" (cpsr));
    return (cpsr & 0x80UL) == 0 ? 1 : 0;
}

/// \note Not actually atomic!
int __arm_bool_compare_and_swap(void **d, void *o, void *n) {
    if(*d == o) {
        *d = n;
        return 1;
    }
    return 0;
}

