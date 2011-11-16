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

#ifndef _COMPILER_H
#define _COMPILER_H

#define PACKED		__attribute__((packed))
#define ALIGN(n)	__attribute__((align(n)))
#define ALIGNED(n)	__attribute__((aligned(n)))

#define SECTION(s)	__attribute__((section(s)))
#define UNUSED		__attribute__((unused))

#define NOP			__asm__ volatile("nop")

#define MEMORY_BARRIER __sync_synchronize()

// Atomicity.
#define atomic_fetch_and_add	__sync_fetch_and_add
#define atomic_fetch_and_sub	__sync_fetch_and_sub
#define atomic_fetch_and_or		__sync_fetch_and_or
#define atomic_fetch_and_and	__sync_fetch_and_and
#define atomic_fetch_and_xor	__sync_fetch_and_xor
#define atomic_fetch_and_nand	__sync_fetch_and_nand

#define atomic_add_and_fetch	__sync_add_and_fetch
#define atomic_sub_and_fetch	__sync_sub_and_fetch
#define atomic_or_and_fetch		__sync_or_and_fetch
#define atomic_and_and_fetch	__sync_and_and_fetch
#define atomic_xor_and_fetch	__sync_xor_and_fetch
#define atomic_nand_and_fetch	__sync_nand_and_fetch

#define atomic_bool_compare_and_swap	__sync_bool_compare_and_swap
#define atomic_val_compare_and_swap		__sync_val_compare_and_swap

#define atomic_lock_test_and_set		__sync_lock_test_and_set
#define atomic_lock_release				__sync_lock_release

#endif
