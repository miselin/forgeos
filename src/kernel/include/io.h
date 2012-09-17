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

#ifndef _IO_H
#define _IO_H

#include <types.h>

/// Print a single character to the screen abstraction provided by the machine.
#define putc(c)	machine_putc(c)

/// Clear the screen (abstracted by the machine).
#define clrscr	machine_clear_screen

/**
 * Define the extents of the console screen, where one is available.
 * Use I/O functions with the _at suffix to write to areas outside these extents.
 */
#define scrextents	machine_define_screen_extents

/**
 * Print a single character to the screen abstraction provided by the machine, at the
 * given x/y co-ordinate.
 */
#define putc_at(c, x, y) machine_putc_at(c, x, y)

extern void machine_putc(char c);
extern void machine_putc_at(char c, int x, int y);
extern void machine_clear_screen();
extern void machine_define_screen_extents(int x, int y);

extern void puts(const char *s);
extern void puts_at(const char *s, int x, int y);

extern void serial_puts(const char *s);

#if defined(DEBUG) || defined(_TESTING)
extern int dprintf(const char *fmt, ...) __attribute__((format(printf, 1, 2)));
#else
#define dprintf(a, ...)
#endif

extern int kprintf(const char *fmt, ...) __attribute__((format(printf, 1, 2)));
extern int sprintf(char * s, const char *fmt, ...)  __attribute__((format(printf, 2, 3)));

// I don't like doing this, but x86 just has to be different...
#ifdef X86
extern uint8_t inb(uint16_t port);
extern uint16_t inw(uint16_t port);
extern uint32_t inl(uint16_t port);

extern void outb(uint16_t port, uint8_t val);
extern void outw(uint16_t port, uint16_t val);
extern void outl(uint16_t port, uint32_t val);
#endif

#endif
