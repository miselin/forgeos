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

#ifndef _STDINT_H
#define _STDINT_H

typedef signed char		int8_t;
typedef signed short	int16_t;
typedef signed long		int32_t;
typedef signed int		int64_t __attribute__((mode (__DI__)));

typedef unsigned char	uint8_t;
typedef unsigned short	uint16_t;
typedef unsigned long	uint32_t;
typedef unsigned int	uint64_t __attribute__((mode (__DI__)));

typedef unsigned long	size_t;

typedef signed long		intptr_t;
typedef unsigned long	uintptr_t;

typedef uintptr_t		paddr_t;
typedef uintptr_t		vaddr_t;

typedef int64_t			intmax_t;
typedef uint64_t		uintmax_t;

#endif
