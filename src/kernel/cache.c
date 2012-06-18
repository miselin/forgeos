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
#include <pool.h>
#include <io.h>

/** Cache block. All blocks are 4096 bytes in size. */
struct block {
    void *addr;

    /**
     * A meaningful offset that this block represents. For example, a sector
     * number might be here for a disk cache.
     */
    unative_t offset;

    /**
     * When retrieving a cache block for use, the refcount is incremented. When
     * the system has memory pressure and cache needs to be evicted, blocks with
     * a refcount of zero will be freed. Blocks with a positive refcount cannot
     * be freed.
     */
    size_t refcount;

    /**
     * Simple hash of the data in the block. Developers can use this to avoid
     * writing an unchanged cache block back to a slow medium, by comparing
     * a block about to be written to the block stored in cache.
     */
    uint32_t hash;
};

struct cache {
    /**
     * Pool containing all blocks for the cache. This essentially means caches
     * have a static size. Blocks can be freed from this pool quite happily,
     * so erring on the side of a larger cache is always recommended.
     */
    void *pool;

    /** Metadata for the blocks in the pool, as a tree (based on offset). */
    void *blockdata;
};

static uint32_t dohash(uint32_t *p) {
    /// \todo Ridiculously naive hash here, fixme!
    uint32_t hash = 0;
    for(size_t i = 0; i < 1024; i++) {
        hash += p[i];
    }
    return hash;
}

void *create_cache(size_t cachesz) {
    void *ret = (void *) malloc(sizeof(struct cache));
    struct cache *meta = (struct cache *) ret;

    if(cachesz < 0x1000)
        cachesz = 0x1000;

    if(cachesz & 0xFFF)
        cachesz = (cachesz & (size_t) ~0xFFF) + 0x1000;

    meta->pool = create_pool(0x1000, cachesz / 0x1000);
    meta->blockdata = create_tree();

    return ret;
}

void evict_cache(void *cache, size_t numpages) {
    if(!cache || !numpages)
        return;

    struct cache *meta = (struct cache *) cache;
    struct block *blockdata = (struct block *) tree_min(meta->blockdata);

    size_t numevicted = 0;
    void *it = tree_iterator(meta->blockdata);
    while(!tree_iterator_eof(meta->blockdata, it)) {
        if(blockdata->refcount == 0) {
            pool_dealloc_and_free(meta->pool, (void *) blockdata->addr);
            numevicted++;

            tree_delete(meta->blockdata, (void *) blockdata->offset);
            free(blockdata);
        }

        tree_next(meta->blockdata, it);
    }

    dprintf("cache: evicted %d pages from the cache (out of %d in evict call)\n", numevicted, numpages);
}

void *cache_startblock(void *cache, unative_t offset) {
    if(!cache)
        return 0;

    struct cache *meta = (struct cache *) cache;
    struct block *blockdata = (struct block *) tree_search(meta->blockdata, (void *) offset);

    // Allocate a block if one doesn't exist.
    if(!blockdata) {
        blockdata = (struct block *) malloc(sizeof(struct block));
        memset(blockdata, 0, sizeof(*blockdata));

        blockdata->addr = pool_alloc(meta->pool);
        blockdata->offset = offset;

        memset(blockdata->addr, 0, 0x1000);

        tree_insert(meta->blockdata, (void *) offset, blockdata);
    }

    dprintf("cache: block for offset %x is now started\n", offset);

    blockdata->refcount++;

    // Update the hash.
    blockdata->hash = dohash(blockdata->addr);

    return (void *) blockdata;
}

void cache_doneblock(void *cache, void *block) {
    if(!cache || !block)
        return;

    struct block *blockdata = (struct block *) block;

    dprintf("cache: block for offset %x is now finished\n", blockdata->offset);

    blockdata->refcount--;
}

void *cache_blockaddr(void *block) {
    if(!block)
        return 0;

    struct block *blockdata = (struct block *) block;

    return blockdata->addr;
}

int cache_blockchanged(void *block) {
    if(!block)
        return 0;

    struct block *blockdata = (struct block *) block;

    // Calculate the new hash.
    uint32_t hash = dohash(blockdata->addr);

    if(hash != blockdata->hash) {
        blockdata->hash = hash;
        return 1;
    } else {
        return 0;
    }
}

int cache_iscached(void *cache, unative_t offset) {
    if(!cache)
        return 0;

    struct cache *meta = (struct cache *) cache;
    struct block *blockdata = (struct block *) tree_search(meta->blockdata, (void *) offset);
    if(blockdata)
        return 1;
    else
        return 0;
}

void destroy_cache(void *cache) {
    /// \todo Implement me! :)
}

