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

#include <stdint.h>
#include <multiboot.h>

/// Initialise the physical memory allocator
#define pmem_init		mach_phys_init

/// De-initialise the physical memory allocator.
#define pmem_deinit		mach_phys_deinit

/// Allocate a single page from the physical allocator.
extern paddr_t	pmem_alloc();

/// Deallocate a single page, returning it to the physical allocator.
extern void		pmem_dealloc(paddr_t p);

/// Pin a particular physical page, making it impossible to allocate.
extern void		pmem_pin(paddr_t p);

// Machine-specific physical memory management
extern int		mach_phys_init(struct multiboot_info *mbi);
extern int		mach_phys_deinit();

#endif
