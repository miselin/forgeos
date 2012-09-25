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

#ifndef _SYSTEM_H
#define _SYSTEM_H

#include <types.h>

#define PHYS_ADDR		0x400000UL

#define KERNEL_BASE		0xC0000000UL

/// Mapping for the Local APIC in the address space. Each CPU can access this
/// address which maps to its Local APIC.
#define KERNEL_LAPIC    0xCFFFF000UL

#define HEAP_BASE		0xD0000000UL
#define POOL_BASE       0xE0000000UL
#define MMIO_BASE       0xF0000000UL
#define STACK_TOP		0xFFC00000UL
#define STACK_SIZE		0x4000UL // 16 KB

#define MMIO_LENGTH     0xF0000000UL

#define PAGE_SIZE		0x1000UL

/// Mask to be applied against a paddr_t to get a full physical address.
#define PADDR_MASK      0xFFFFFFFFUL

#define log2phys(x)		(((x) - KERNEL_BASE) + PHYS_ADDR)
#define phys2log(x)		(((x) - PHYS_ADDR) + KERNEL_BASE)

inline void x86_cpuid(int code, uint32_t *a, uint32_t *b, uint32_t * c, uint32_t *d) {
    __asm__ volatile("cpuid" : "=a" (*a), "=b" (*b), "=c" (*c), "=d" (*d) : "a" (code));
}

inline void x86_get_msr(uint32_t msr, uint32_t *l, uint32_t *h) {
    __asm__ volatile("rdmsr" : "=a" (*l), "=d" (*h) : "c" (msr));
}

inline void x86_set_msr(uint32_t msr, uint32_t l, uint32_t h) {
    __asm__ volatile("wrmsr" :: "a" (l), "d" (h), "c" (msr));
}

#endif
