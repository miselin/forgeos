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

#ifndef _STDARG_H
#define _STDARG_H

#ifdef __GNUC__
typedef __builtin_va_list	va_list;
#define va_start(ap, pN)	__builtin_va_start(ap, pN)
#define va_end(ap)			__builtin_va_end(ap)
#define va_arg(ap, pN)		__builtin_va_arg(ap, pN)
#define va_copy(d, s)		__builtin_va_copy(d, s)
#else
typedef char *va_list;
#define __va_argsize(t) (((sizeof(t) + sizeof(int) - 1) / sizeof(int)) * sizeof(int))
#define va_start(ap, pN) ((ap) = ((va_list) (*pN) + __va_argsize(pN)))
#define va_end(ap) ((void) 0)
#define va_arg(ap, t) (((ap) + __va_argsize(t)), *(((t*) (void*) ((ap) - __va_argsize(t)))))
#endif

#endif

