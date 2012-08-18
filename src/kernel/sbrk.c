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
#include <system.h>
#include <panic.h>
#include <vmem.h>
#include <pmem.h>
#include <util.h>
#include <io.h>

// #define VERBOSE_SBRK

static uintptr_t base = 0;

static char first_page[PAGE_SIZE] __attribute__((aligned(PAGE_SIZE)));
static char prime_page[PAGE_SIZE] __attribute__((aligned(PAGE_SIZE)));

#define PAGE_ALIGNED(x) ((x) & (uintptr_t) ~(PAGE_SIZE - 1))

void *dlmalloc_sbrk(intptr_t incr) {
	uintptr_t old = base;

#ifdef VERBOSE_SBRK
	dprintf("sbrk(0x%x) - base at %x\n", incr, base);
#endif

	if(base == 0) {
		// Assume there is no heap yet. Note that physical memory management depends on
		// malloc, so we need a little bit of static space to kick things off.
		memset(first_page, 0, PAGE_SIZE);
		vmem_prime(log2phys((paddr_t) prime_page));
		vmem_map(HEAP_BASE, log2phys((paddr_t) first_page), VMEM_READWRITE);
		base = old = HEAP_BASE;
	}

	if(incr == 0)
		return (void *) base;

	if(incr < 0) {
		incr = -incr;
		base -= (uintptr_t) incr;

		// Unmap.
		vaddr_t v = old;
		if(PAGE_ALIGNED(old) != PAGE_ALIGNED(base)) {
			for(; v > base; v -= PAGE_SIZE) {
				if(vmem_ismapped(v)) {
					paddr_t p = vmem_v2p(v);
					vmem_unmap(v);
					pmem_dealloc(p);
				}
			}
		}

		// Return the new top of the heap.
		old = base;
	} else {
		base += (uintptr_t) incr;
		if(PAGE_ALIGNED(old) != PAGE_ALIGNED(base)) {
			vaddr_t v = old;
			for(; v <= base; v += PAGE_SIZE) {
				if(vmem_ismapped(v) == 0) {
					vmem_map(v, (paddr_t) ~0, 0);
				}
			}
		}
	}

#ifdef VERBOSE_SBRK
	dprintf("sbrk returning %x\n", old);
#endif

	return (void *) old;
}

