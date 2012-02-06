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

#include <io.h>
#include <stdarg.h>

extern int vsprintf(char *buf, const char *fmt, va_list args);

/// \todo Code duplication.

/// \todo Figure out a decent way to tag dprintf output with kernel, driver,
///       user, libc, whatever so we can figure out where each debug line comes
///       from in big logs.

#ifdef DEBUG
int dprintf(const char *fmt, ...) {
	int len = 0;
	char buf[512];
	va_list args;
	va_start(args, fmt);
	len = vsprintf(buf, fmt, args);
    va_end(args);
    serial_puts("[kernel] ");
	serial_puts(buf);
	return len;
}
#endif

int kprintf(const char *fmt, ...) {
	int len = 0;
	char buf[512];
	va_list args;
	va_start(args, fmt);
	len = vsprintf(buf, fmt, args);
    va_end(args);
	puts(buf);
	return len;
}
