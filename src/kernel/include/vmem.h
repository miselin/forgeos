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

#include <types.h>

// Zero #defines here are for readability, they serve no purpose as flags.

#define VMEM_READONLY		0
#define VMEM_READWRITE		0x01

#define VMEM_SUPERVISOR		0
#define VMEM_USERMODE		0x02

#define VMEM_GLOBAL			0x100
#define VMEM_DEVICE         0x200

/// Executable region of memory.
#define VMEM_EXEC         0x400

/// Maps in a single page.
#define vmem_map		arch_vmem_map

/// Unmaps a single page.
#define vmem_unmap		arch_vmem_unmap

/// Gets a physical address from a virtual address in the current address space.
#define vmem_v2p        arch_vmem_v2p

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

/// Finalizes any remaining virtual memory initialization such as removing
/// large page mappings that were needed for kernel init.
#define vmem_final_init		arch_vmem_final_init

/// "Primes" the virtual memory allocator in a case where the physical memory
/// allocator may be less-than-functional, by providing a page to be used as
/// a replacement if @pmem_alloc() fails.
#define vmem_prime      arch_vmem_prime

extern int arch_vmem_map(vaddr_t, paddr_t, size_t);
extern void arch_vmem_unmap(vaddr_t);
extern int arch_vmem_modify(vaddr_t, size_t);
extern int arch_vmem_ismapped(vaddr_t);
extern paddr_t arch_vmem_v2p(vaddr_t);
extern vaddr_t arch_vmem_create();
extern void arch_vmem_switch(vaddr_t);
extern void arch_vmem_init();
extern void arch_vmem_final_init();

extern void arch_vmem_prime(paddr_t);

#endif
