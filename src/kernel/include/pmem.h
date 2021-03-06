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

#ifndef _PMEM_H
#define _PMEM_H

#include <types.h>

#define PMEM_SPECIAL_STANDARD       0
#define PMEM_SPECIAL_FIRMWARE       1 // eg, < 1 MB on x86

/// Initialise the physical memory allocator
#define pmem_init		mach_phys_init

/// De-initialise the physical memory allocator.
#define pmem_deinit		mach_phys_deinit

/// Allocate a single page from the physical allocator.
extern paddr_t	pmem_alloc();

/// Allocate a page from a special region. Must be implemented by an
/// architecture or machine - simply return pmem_alloc if no special regions.
extern paddr_t pmem_alloc_special(size_t how);

/// Deallocate a page from a special region.
extern void pmem_dealloc_special(size_t how, paddr_t p);

/// Deallocate a single page, returning it to the physical allocator.
extern void		pmem_dealloc(paddr_t p);

/// Pin a particular physical page, making it impossible to allocate.
extern void		pmem_pin(paddr_t p);

// Machine-specific physical memory management
extern int      mach_phys_init(phys_ptr_t tags);
extern int		mach_phys_deinit();

// Full physical memory size (or rather, available pages - not mmio etc) in KiB
extern paddr_t	pmem_size();

// Free physical memory, available to be allocated in KiB.
extern paddr_t	pmem_freek();

/// Structure defining a physical address.
struct phys_page {
    paddr_t addr;
};

#endif
