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

/// Maximum width of the text console (regardless of extents)
#define PC_MAX_W 80

/// Maximum height of the text console
#define PC_MAX_H 25

static uint8_t screenX = 0;
static uint8_t screenY = 0;

static uint8_t screenW = 80;
static uint8_t screenH = 25;

static uint8_t *vmem = (uint8_t *) 0xB8000;

void machine_clear_screen() {
	memset(vmem, 0, screenW * screenH * sizeof(uint16_t));
}

void machine_putc(char c) {
	serial_write((uint8_t) c);

	if(c == '\n') {
		screenX = 0;
		screenY++;
	} else {
		int offset = ((screenY * screenW) + screenX) * 2;
		vmem[offset] = (unsigned char) c;
		vmem[offset + 1] = 0x07;

		screenX++;
	}

	if(screenX >= screenW) {
		screenX = 0;
		screenY++;
	}

	if(screenY >= screenH) {
		screenY--;

		// Scroll.
		memcpy(vmem, &vmem[screenW * 2], (screenH - 1) * screenW * 2);
		memset(&vmem[(screenW * 2) * (screenH - 1)], 0, screenW * 2);
	}
}

void machine_putc_at(char c, int x, int y) {
	if(x < 0)
		x = 0;
	if(y < 0)
		y = 0;

	if(x > PC_MAX_W)
		x = PC_MAX_W;
	if(y > PC_MAX_H)
		y = PC_MAX_H;

	uint8_t sx = screenX, sy = screenY;
	uint8_t sw = screenW, sh = screenH; // Saved because we don't want to scroll the screen.

	screenX = (uint8_t) x & 0xFF; screenY = (uint8_t) y & 0xFF;
	screenW = PC_MAX_W; screenH = PC_MAX_H + 1;
	machine_putc(c);

	screenX = sx;
	screenY = sy;
}

void machine_define_screen_extents(int x, int y) {
	if(x < 0)
		x = 0;
	if(y < 0)
		y = 0;

	if(x > PC_MAX_W)
		x = PC_MAX_W;
	if(y > PC_MAX_H)
		y = PC_MAX_H;

	screenW = (uint8_t) x & 0xFF;
	screenH = (uint8_t) y & 0xFF;
}
