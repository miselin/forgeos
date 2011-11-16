/*
 * Copyright (C) 2011 Alex Smith
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
 * @brief		Test kernel console functions.
 */

#include <arch/io.h>
#include <system.h>

/** Serial port to use. */
#define SERIAL_PORT		0x3F8

extern void console_putc(char ch);
static bool have_inited = false;

/** Initialise the serial port. */
static inline void init_serial_port(void) {
	out8(SERIAL_PORT + 1, 0x00);  /* Disable all interrupts */
	out8(SERIAL_PORT + 3, 0x80);  /* Enable DLAB (set baud rate divisor) */
	out8(SERIAL_PORT + 0, 0x03);  /* Set divisor to 3 (lo byte) 38400 baud */
	out8(SERIAL_PORT + 1, 0x00);  /*                  (hi byte) */
	out8(SERIAL_PORT + 3, 0x03);  /* 8 bits, no parity, one stop bit */
	out8(SERIAL_PORT + 2, 0xC7);  /* Enable FIFO, clear them, with 14-byte threshold */
	out8(SERIAL_PORT + 4, 0x0B);  /* IRQs enabled, RTS/DSR set */
	have_inited = true;
}

/** Print a character.
 * @param ch		Character to print. */
void console_putc(char ch) {
	if(!have_inited) {
		init_serial_port();
	}

	while(!(in8(SERIAL_PORT + 5) & 0x20));
	if(ch == '\n') {
		while(!(in8(SERIAL_PORT + 5) & 0x20));
		out8(SERIAL_PORT, '\r');
	}
	out8(SERIAL_PORT, ch);
}
