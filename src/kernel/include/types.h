/*
 * Copyright (c) 2011 Rich Edelman <redelman at gmail dot com>
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

#ifndef __TYPES_H__
#define __TYPES_H__

#include <arch/types.h>
#include <compiler.h>

/* Define NULL as appropriate for C and C++ */
#ifndef NULL
#   ifdef __cplusplus
#       ifdef __LP64__
#           define NULL (0L)
#       else
#           define NULL 0
#       endif /* __LP64__ */
#   else
#       define NULL ((void *)0)
#   endif  /* __cplusplus */
#endif /* NULL */

#ifndef __bool_true_false_are_defined
    /* bool is already defined in C++ */
#   ifndef __cplusplus
        typedef _Bool           bool;
#       define true            1
#       define false           0
#   else /* __cplusplus */
        typedef bool            bool;
#       define true            true
#       define false           false
#   endif /* __cplusplus */

#   define __bool_true_false_are_defined    1

#endif /* __bool_true_false_are_defined */

#ifndef offsetof
#   define offsetof(type, member)   __builtin_offsetof(type, member)
#endif /* offsetof */

typedef unsigned long           size_t;
typedef signed long             ssize_t;

typedef __mattise_int8_t        int8_t;
typedef __mattise_uint8_t       uint8_t;

typedef __mattise_int16_t       int16_t;
typedef __mattise_uint16_t      uint16_t;

typedef __mattise_int32_t       int32_t;
typedef __mattise_uint32_t      uint32_t;

typedef __mattise_int64_t       int64_t;
typedef __mattise_uint64_t      uint64_t;

typedef __mattise_ptrdiff_t     ptrdiff_t;
typedef __mattise_intptr_t      intptr_t;
typedef __mattise_uintptr_t     uintptr_t;

typedef __mattise_intmax_t      intmax_t;
typedef __mattise_uintmax_t     uintmax_t;

typedef __mattise_uint64_t      offset_t;

/* Misc -- Actually are kernel specific */
typedef __mattise_uintptr_t     register_t;

typedef __mattise_intptr_t      native_t;
typedef __mattise_uintptr_t     unative_t;

typedef __mattise_uint64_t      phys_ptr_t;
typedef __mattise_uintptr_t     ptr_t;

typedef __mattise_uint64_t      paddr_t;
typedef __mattise_uintptr_t     vaddr_t;

#endif /* __TYPES_H__ */
