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

#define __packed          __attribute__((packed))
#define __aligned(n)      __attribute__((aligned(n)))

#define __barrier         __asm__ volatile("" ::: "memory")

#define __section(s)      __attribute__((section(s)))

#define __unused          __attribute__((unused))
#define __used            __attribute__((used))

#if defined(__clang__) && !defined(NO_CLANG_BUILTINS)
#define atomic_compare_and_swap(old_val, new_val, out_val, ...)    out_val = __sync_swap((old_val), (new_val))
#else

#ifdef ARM
#define atomic_bool_compare_and_swap __arm_bool_compare_and_swap
extern int __arm_bool_compare_and_swap(void **d, void *o, void *n);
#else
#define atomic_bool_compare_and_swap __sync_bool_compare_and_swap
#endif

#define atomic_compare_and_swap(old_val, new_val, out_val, cmp_val, stmt) do { stmt; } while(!atomic_bool_compare_and_swap((old_val), (cmp_val), (new_val)))

#endif

#define STRINGIFY(val)          #val
#define XSTRINGIFY(val)         STRINGIFY(val)

#endif /* _COMPILER_H */

