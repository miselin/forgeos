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

#ifndef _VMEM_H
#define _VMEM_H

#include <stdint.h>

// Zero #defines here are for readability, they serve no purpose as flags.

#define VMEM_READONLY		0
#define VMEM_READWRITE		0x01

#define VMEM_SUPERVISOR		0
#define VMEM_USERMODE		0x02

#define VMEM_GLOBAL			0x100

/// Maps in a single page.
#define vmem_map		arch_vmem_map

/// Unmaps a single page.
#define vmem_unmap		arch_vmem_unmap

/// Changes the flags on an existing mapping.
#define vmem_modify		arch_vmem_modify

/// Is the given address mapped?
#define vmem_ismapped	arch_vmem_ismapped

/// Creates a new address space context.
#define vmem_create		arch_vmem_create

/// Switches to a new address space.
#define vmem_switch		arch_vmem_switch

/// Completes initialisation of the virtual memory manager by cleaning up
/// areas of the kernel that are not needed anymore, and also by moving the
/// stack to the correct area for the implementation.
#define vmem_init		arch_vmem_init

extern int arch_vmem_map(vaddr_t, paddr_t, size_t);
extern void arch_vmem_unmap(vaddr_t);
extern int arch_vmem_modify(vaddr_t, size_t);
extern int arch_vmem_ismapped(vaddr_t);
extern vaddr_t arch_vmem_create();
extern void arch_vmem_switch(vaddr_t);
extern void arch_vmem_init();

#endif
