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
#include <malloc.h>
#include <assert.h>
#include <util.h>
#include <io.h>

struct phys_page {
	paddr_t addr;
};

static void *page_stack = 0;

static paddr_t freeKiB = 0;

paddr_t pmem_freek() {
	return freeKiB;
}

paddr_t pmem_alloc() {
	struct phys_page *page = 0;
	paddr_t ret = 0;

	if(page_stack == 0)
		return 0;

	page = (struct phys_page *) stack_pop(page_stack);
	if(!page)
		return 0;
	ret = page->addr;
	free(page);

	freeKiB -= PAGE_SIZE / 1024;

	return ret;
}

void pmem_dealloc(paddr_t p) {
	struct phys_page *page = 0;

	if(page_stack == 0)
		page_stack = create_stack();

	page = (struct phys_page *) malloc(sizeof(struct phys_page));
	page->addr = p;

	stack_push(page_stack, page);

	freeKiB += PAGE_SIZE / 1024;
}

void pmem_pin(paddr_t p) {
	/// \note Do we even need this function?
	assert(0);
}

