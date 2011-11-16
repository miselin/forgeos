/*
 * Copyright (C) 2011 Alex Smith
 *
 * Permission to use, copy, modify, and/or distribute this software for any
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

/**
 * @file
 * @brief		OMAP3 memory management.
 */

#include <arm/atag.h>

#include <lib/utility.h>

#include <omap3/omap3.h>

#include <memory.h>
#include <system.h>

extern char __start[];

/** Detect memory regions. */
void platform_memory_detect(void) {
	phys_ptr_t start, end;

	/* Iterate through all ATAG_MEM tags and add the regions they describe. */
	ATAG_ITERATE(tag, ATAG_MEM) {
		if(tag->mem.size) {
			/* Cut the region short if it is not page-aligned. */
			start = ROUND_UP(tag->mem.start, PAGE_SIZE);
			end = ROUND_DOWN(tag->mem.start + tag->mem.size, PAGE_SIZE);
			phys_memory_add(start, end, PHYS_MEMORY_FREE);
		}
	}

	/* Mark any supplied boot image as internal, the memory taken by it is
	 * no longer used once the kernel is entered. */
	ATAG_ITERATE(tag, ATAG_INITRD2) {
		if(tag->initrd.size) {
			/* Ensure the whole region is covered if it is not
			 * page-aligned. */
			start = ROUND_DOWN(tag->initrd.start, PAGE_SIZE);
			end = ROUND_UP(tag->initrd.start + tag->initrd.size, PAGE_SIZE);
			phys_memory_add(start, end, PHYS_MEMORY_INTERNAL);
		}
	}

	/* Mark the region between the start of SDRAM and our load address as
	 * internal, as U-Boot puts things like the ATAG list here. */
	phys_memory_add(OMAP3_SDRAM_BASE, ROUND_DOWN((ptr_t)__start, PAGE_SIZE),
	                PHYS_MEMORY_INTERNAL);
}
