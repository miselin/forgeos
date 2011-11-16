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
 * @brief		ARM MMU functions.
 */

#include <arch/page.h>

#include <lib/string.h>

#include <assert.h>
#include <memory.h>
#include <mmu.h>
#include <system.h>

/** Allocate a paging structure. */
static phys_ptr_t allocate_structure(size_t size) {
	phys_ptr_t addr = phys_memory_alloc(size, size, true);
	memset((void *)addr, 0, size);
	return addr;
}

/** Map a section in a context.
 * @param ctx		Context to map in.
 * @param virt		Virtual address to map.
 * @param phys		Physical address to map to. */
static void map_section(mmu_context_t *ctx, ptr_t virt, phys_ptr_t phys) {
	uint32_t *l1;
	int l1e;

	assert(!(virt % LARGE_PAGE_SIZE));
	assert(!(phys % LARGE_PAGE_SIZE));

	l1 = (uint32_t *)ctx->ttbr0;
	l1e = virt / LARGE_PAGE_SIZE;
	l1[l1e] = phys | (1<<1) | (1<<10);
}

/** Map a small page in a context.
 * @param ctx		Context to map in.
 * @param virt		Virtual address to map.
 * @param phys		Physical address to map to. */
static void map_small(mmu_context_t *ctx, ptr_t virt, phys_ptr_t phys) {
	uint32_t *l1, *l2;
	phys_ptr_t addr;
	int l1e, l2e;

	l1 = (uint32_t *)ctx->ttbr0;
	l1e = virt / LARGE_PAGE_SIZE;
	if(!(l1[l1e] & (1<<0))) {
		/* FIXME: Second level tables are actually 1KB. Should probably
		 * split up these pages and use them fully. */
		addr = allocate_structure(PAGE_SIZE);
		l1[l1e] = addr | (1<<0);
	}

	l2 = (uint32_t *)(l1[l1e] & 0xFFFFFC00);
	l2e = (virt % LARGE_PAGE_SIZE) / PAGE_SIZE;
	l2[l2e] = phys | (1<<1) | (1<<4);
}

/** Create a mapping in an MMU context.
 * @param ctx		Context to map in.
 * @param virt		Virtual address to map.
 * @param phys		Physical address to map to.
 * @param size		Size of the mapping to create.
 * @return		Whether created successfully. */
bool mmu_map(mmu_context_t *ctx, target_ptr_t virt, phys_ptr_t phys, target_size_t size) {
	uint32_t i;

	if(virt % PAGE_SIZE || phys % PAGE_SIZE || size % PAGE_SIZE) {
		return false;
	}

	/* Map using sections where possible. To do this, align up to a 1MB
	 * boundary using small pages, map anything possible with sections,
	 * then do the rest using small pages. If virtual and physical addresses
	 * are at different offsets from a section boundary, we cannot map
	 * using sections. */
	if((virt % LARGE_PAGE_SIZE) == (phys % LARGE_PAGE_SIZE)) {
		while(virt % LARGE_PAGE_SIZE && size) {
			map_small(ctx, virt, phys);
			virt += PAGE_SIZE;
			phys += PAGE_SIZE;
			size -= PAGE_SIZE;
		}
		while(size / LARGE_PAGE_SIZE) {
			map_section(ctx, virt, phys);
			virt += LARGE_PAGE_SIZE;
			phys += LARGE_PAGE_SIZE;
			size -= LARGE_PAGE_SIZE;
		}
	}

	/* Map whatever remains. */
	for(i = 0; i < size; i += PAGE_SIZE) {
		map_small(ctx, virt + i, phys + i);
	}

	return true;
}

/** Create a new MMU context.
 * @return		Pointer to context. */
mmu_context_t *mmu_context_create(void) {
	mmu_context_t *ctx;

	ctx = kmalloc(sizeof(*ctx));
	ctx->ttbr0 = allocate_structure(0x4000);
	return ctx;
}
