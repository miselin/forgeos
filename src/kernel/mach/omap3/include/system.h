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

#define PHYS_ADDR		0x80008000UL

#define KERNEL_BASE		0xC0000000UL
#define HEAP_BASE		0x60000000UL
#define POOL_BASE       0xB0000000UL
#define MMIO_BASE       0xD0000000UL
#define STACK_TOP		0xFFC00000UL
#define STACK_SIZE		0x4000UL // 16 KB

#define MMIO_LENGTH     0x10000000UL

#define PAGEDIR_VIRT    0xFFFB0000UL
#define PAGETABS_VIRT   0xFF000000UL

#define USER_PAGEDIR_VIRT   0x3FFFF000UL
#define USER_PAGETABS_VIRT  0x3FE00000UL

#define PAGEDIR_PHYS    0x8FAFC000UL
#define PAGETABS_PHYS   0x8FB00000UL

#define TRAP_VECTORS    0xFFFF0000UL

#define RAM_START       0x80000000UL
#define RAM_FINISH      (0x9FFFFFFFUL + 1UL)

#define PAGE_SIZE		0x1000UL

/// Mask to be applied against a paddr_t to get a full physical address.
#define PADDR_MASK      0xFFFFFFFFUL

#define log2phys(x)		(x)
#define phys2log(x)		(x)

#endif
