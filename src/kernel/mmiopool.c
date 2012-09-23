/*
 * Copyright (c) 2012 Matthew Iselin, Rich Edelman
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
#include <mmiopool.h>
#include <malloc.h>
#include <util.h>
#include <vmem.h>
#include <io.h>

struct region {
    vaddr_t base;
    size_t len;

    int allocated;
};

static void *freelist = 0;

void init_mmiopool(vaddr_t mmiobase, size_t maxsz) {
    freelist = create_list();

    struct region *r = (struct region *) malloc(sizeof(struct region));
    r->base = mmiobase;
    r->len = maxsz;
    r->allocated = 0;

    list_insert(freelist, r, 0);
}

void *mmiopool_alloc(size_t len, paddr_t tophys) {
    if(!freelist) {
        return 0;
    }

    vaddr_t ret = 0;

    // Handle offsets within the physical address.
    if(tophys & (PAGE_SIZE - 1)) {
        len += tophys & (PAGE_SIZE - 1);

        // Now, page-align the physical address.
        tophys &= ~(PAGE_SIZE - 1);
    }

    if(len < PAGE_SIZE)
        len = PAGE_SIZE;

    // Round up to the next page size.
    if(len % PAGE_SIZE)
        len = (len + PAGE_SIZE) & ~(PAGE_SIZE - 1);

    // Find an unallocated region big enough for us.
    size_t i = 0;
    struct region *p = 0;
    while((p = list_at(freelist, i++))) {
        // Already allocated, or not big enough?
        if(p->allocated || (p->len < len)) {
            continue;
        }

        // No exact match in size?
        if(p->len != len) {
            // Okay, we can split this region now.
            struct region *new_region = (struct region *) malloc(sizeof(struct region));
            new_region->allocated = 0;
            new_region->len = p->len - len;
            new_region->base = p->base + len;

            list_insert(freelist, new_region, i);
        }

        // Update this region.
        p->len = len;
        p->allocated = 1;

        ret = p->base;

        for(i = 0; i < (len / PAGE_SIZE); i++) {
            vmem_map(ret + (i * PAGE_SIZE), tophys + (i * PAGE_SIZE), VMEM_READWRITE | VMEM_DEVICE | VMEM_SUPERVISOR | VMEM_GLOBAL);
        }

        break;
    }

    return (void *) ret;
}

void mmiopool_dealloc(void *p __unused) {
    // Find where this address is, in the list, merge with surrounding unallocated
    // regions if they exist.

    /// \todo Write me :D
}
