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
#ifndef CACHE_H
#define CACHE_H

#include <types.h>

/** Creates a new cache with the given size. */
extern void *create_cache(size_t cachesz);

/** Evicts a number of pages from the cache, if possible. */
extern void evict_cache(void *cache, size_t numpages);

/**
 * Starts work with a cache block at the given offset. This block can be read
 * from or written to. This is, however, merely a block in memory. Developers
 * must implement a mechanism for writing data from a completed block to disk.
 *
 * Calling this function ensures the cache block at the given offset will not
 * be evicted until cache_doneblock is called on it.
 *
 * The hash of the block will be updated upon calling this function.
 */
extern void *cache_startblock(void *cache, unative_t offset);

/**
 * Completes work with a cache block. The block address should be assumed to be
 * unmapped after this function is called; at any time after this function is
 * called on a block, the cache can evict that block.
 */
extern void cache_doneblock(void *cache, void *block);

/**
 * Gets the address of the given block. Parameter is the return value from
 * cache_startblock.
 */
extern void *cache_blockaddr(void *block);

/**
 * Determines if a given block is available in cache.
 */
extern int cache_iscached(void *cache, unative_t offset);


/**
 * Determines if a block has changed by comparing the current hash of the data
 * at its address with the hash created when the block was last used in
 * cache_startblock.
 *
 * The purpose of this function is to be called immediately before
 * cache_doneblock, in order to determine if the block changed since calling
 * cache_startblock. The canonical example of this function is working with a
 * slow medium such as disk, or a network-backed file.
 *
 * The user may provide a buffer to be written to disk. This function can be
 * used after updating the cache to determine whether or not to complete a slow
 * disk request.
 */
extern int cache_blockchanged(void *block);

/**
 * Destroys the given cache, freeing all memory consumed.
 */
extern void destroy_cache(void *cache);

#endif


