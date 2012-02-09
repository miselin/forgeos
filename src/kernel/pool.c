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
#include <pool.h>
#include <types.h>
#include <system.h>
#include <assert.h>
#include <malloc.h>
#include <util.h>
#include <vmem.h>
#include <io.h>

struct pool {
    uintptr_t base;
    uint32_t *bitmap;
    size_t buffer_size;
    size_t buffer_count;
    size_t alloc_count;
};

static uintptr_t pool_base = POOL_BASE;

void init_pool() {
    dprintf("will be creating pools at %x\n", pool_base);
}

void *create_pool(size_t buffsz, size_t buffcnt) {
    /// \todo Atomicity.
    void *ret = create_pool_at(buffsz, buffcnt, pool_base);
    pool_base += (buffsz * buffcnt);
    
    return ret;
}

void *create_pool_at(size_t buffsz, size_t buffcnt, uintptr_t addr) {
    struct pool *ret = (struct pool *) malloc(sizeof(struct pool));
    ret->base = addr;
    ret->alloc_count = 0;
    ret->buffer_count = buffcnt;
    ret->buffer_size = buffsz;
    
    size_t wordcount = buffcnt / 32;
    if(!wordcount) wordcount = 1;
    ret->bitmap = (uint32_t*) malloc(wordcount * sizeof(uint32_t));
    memset(ret->bitmap, 0, wordcount * sizeof(uint32_t));
    
    dprintf("creating pool at %x: %d buffers, each %d bytes (%d dwords in bitmap)\n", addr, buffcnt, buffsz, wordcount);
    
    return (void *) ret;
}

void *pool_alloc(void *pool) {
    if(!pool)
        return 0;
    struct pool *p = (struct pool *) pool;
    
    if(p->alloc_count == p->buffer_count) {
        dprintf("pool %x is exhausted\n", p);
        return 0;
    }
    
    // Find a free bit.
    size_t buffer_idx = 0;
    size_t word = p->alloc_count / 32;
    for(buffer_idx = 0; buffer_idx < 32; buffer_idx++) {
        if((p->bitmap[word] & (1UL << buffer_idx)) == 0) {
            p->bitmap[word] |= (1UL << buffer_idx);
            break;
        }
    }
    
    buffer_idx += word * 32;
    uintptr_t addr = p->base + (buffer_idx * p->buffer_size);
    
    if(!vmem_ismapped(addr)) {
        for(size_t i = 0; i < ((p->buffer_size + 0xFFF) / 0x1000); i++)
            vmem_map(addr + (i * 0x1000), (paddr_t) ~0, VMEM_READWRITE | VMEM_SUPERVISOR);
    }
    
    p->alloc_count++;
    
    dprintf("pool_alloc returning buffer %d (0x%x)\n", buffer_idx, addr);
    return (void *) addr;
}

void pool_dealloc(void *pool, void *p) {
    if(!pool)
        return;
    struct pool *s = (struct pool *) pool;
    
    uintptr_t addr = (uintptr_t) p;
    if((addr < s->base) || (addr > (s->base + (s->buffer_size * s->buffer_count))))
        return;
    
    size_t buffer_idx = (addr - s->base) / s->buffer_size;
    size_t word = buffer_idx / 32;
    size_t bit = buffer_idx % 32;
    s->bitmap[word] &= ~(1UL << bit);
    
    for(size_t i = 0; i < ((s->buffer_size + 0xFFF) / 0x1000); i++)
        vmem_unmap(addr + (i * 0x1000));
}

size_t pool_count(void *pool) {
    if(!pool)
        return 0;
    struct pool *p = (struct pool *) pool;
    return p->alloc_count;
}

