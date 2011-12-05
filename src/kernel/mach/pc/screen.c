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

#include <types.h>
#include <serial.h>
#include <util.h>

static uint8_t screenX = 0;
static uint8_t screenY = 0;

static uint8_t *vmem = (uint8_t *) 0xB8000;

void machine_clear_screen() {
	memset(vmem, 0, 80 * 25 * sizeof(uint16_t));
}

void machine_putc(char c) {
	serial_write((uint8_t) c);

	if(c == '\n') {
		screenX = 0;
		screenY++;
	} else {
		int offset = ((screenY * 80) + screenX) * 2;
		vmem[offset] = (unsigned char) c;
		vmem[offset + 1] = 0x07;

		screenX++;
	}

	if(screenX >= 80) {
		screenX = 0;
		screenY++;
	}

	if(screenY >= 25) {
		screenY--;

		// Scroll.
		memcpy(vmem, &vmem[80 * 2], 24 * 80 * 2);
		memset(&vmem[(80 * 2) * 24], 0, 80 * 2);
	}
}
