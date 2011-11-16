/*
 * Copyright (C) 2010-2011 Alex Smith
 *
 * Permission to use, copy, modify, and/or distribute this software for any
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

/**
 * @file
 * @brief		Core definitions.
 */

#ifndef __SYSTEM_H
#define __SYSTEM_H

#include <arch/system.h>

#include <platform/system.h>

#include <types.h>

/** Builtin object definition structure. */
typedef struct builtin {
	/** Type of the builtin. */
	enum {
		BUILTIN_TYPE_COMMAND,
		BUILTIN_TYPE_FS,
		BUILTIN_TYPE_PARTITION_MAP,
	} type;

	/** Pointer to object. */
	void *object;
} builtin_t;

extern builtin_t __builtins_start[], __builtins_end[];

/** Define a builtin object. */
#define DEFINE_BUILTIN(type, object) \
	static builtin_t __builtin_##name __section(".builtins") __used = { \
		type, \
		&object \
	}

/** Iterate over builtin objects. */
#define BUILTIN_ITERATE(btype, otype, vname) \
	int __iter_##vname = 0; \
	for(otype *vname = (otype *)__builtins_start[0].object; \
	    __iter_##vname < (__builtins_end - __builtins_start); \
	    vname = (otype *)__builtins_start[++__iter_##vname].object) \
		if(__builtins_start[__iter_##vname].type == btype)

extern int kvprintf(const char *fmt, va_list args);
extern int kprintf(const char *fmt, ...) __printf(1, 2);
extern int dvprintf(const char *fmt, va_list args);
extern int dprintf(const char *fmt, ...) __printf(1, 2);

extern void backtrace(int (*printfn)(const char *fmt, ...));

extern void internal_error(const char *fmt, ...) __printf(1, 2) __noreturn;
extern void boot_error(const char *fmt, ...) __printf(1, 2) __noreturn;

#endif /* __SYSTEM_H */
