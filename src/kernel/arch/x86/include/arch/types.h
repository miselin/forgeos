/*
 * Copyright (c) 2012 Rich Edelman <redelman at gmail dot com>
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

#ifndef __ARCH_TYPES_H__
#define __ARCH_TYPES_H__

/*
 * X86 architecture dependent type definitions. All typedefs here are meant
 * to be used by both the kernel and userspace.
 */

/* Basic integral types */
typedef signed char             __forge_int8_t;
typedef unsigned char           __forge_uint8_t;
typedef signed short            __forge_int16_t;
typedef unsigned short          __forge_uint16_t;
typedef signed int              __forge_int32_t;
typedef unsigned int            __forge_uint32_t;

#ifdef __x86_64__
typedef signed long             __forge_int64_t;
typedef unsigned long           __forge_uint64_t;
#else
typedef signed long long        __forge_int64_t;
typedef unsigned long long      __forge_uint64_t;
#endif

typedef signed long             __forge_ptrdiff_t;
typedef signed long             __forge_intptr_t;
typedef unsigned long           __forge_uintptr_t;

typedef __forge_int64_t       __forge_intmax_t;
typedef __forge_uint64_t      __forge_uintmax_t;


/* Format character definitions for printf() */
#define PRId8       "d"
#define PRIi8       "i"
#define PRIo8       "o"
#define PRIu8       "u"
#define PRIx8       "x"

#define PRId16      "d"
#define PRIi16      "i"
#define PRIo16      "o"
#define PRIu16      "u"
#define PRIx16      "x"

#define PRId32      "d"
#define PRIi32      "i"
#define PRIo32      "o"
#define PRIu32      "u"
#define PRIx32      "x"

#ifdef __x86_64__
#define PRId64      "ld"
#define PRIi64      "li"
#define PRIo64      "lo"
#define PRIu64      "lu"
#define PRIx64      "lx"
#else
#define PRId64      "lld"
#define PRIi64      "lli"
#define PRIo64      "llo"
#define PRIu64      "llu"
#define PRIx64      "llx"
#endif /* __x86_64__ */

#define PTRdPTR     "ld"
#define PRIiPTR     "li"
#define PRIoPTR     "lo"
#define PRIuPTR     "lu"
#define PRIxPTR     "lx"

#define PRIdMAX     "jd"
#define PRIiMAX     "ji"
#define PRIoMAX     "ji"
#define PRIuMAX     "ju"
#define PRIxMAX     "jx"

/*
 * X86 Atomics
 */
#define atomic_inc(m) __asm__ volatile("lock incl %0" : "+m" ((m)))
#define atomic_dec(m) __asm__ volatile("lock decl %0" : "+m" ((m)))

#endif /* __ARCH_TYPES_H__ */
