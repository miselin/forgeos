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

#include <stdint.h>

uint8_t inb(uint16_t port) {
	uint8_t ret = 0;
	__asm__ volatile("inb %1, %0" : "=a" (ret) : "Nd" (port));
	return ret;
}

uint16_t inw(uint16_t port) {
	uint16_t ret = 0;
	__asm__ volatile("inw %1, %0" : "=a" (ret) : "Nd" (port));
	return ret;
}

uint32_t inl(uint16_t port) {
	uint32_t ret = 0;
	__asm__ volatile("inl %1, %0" : "=a" (ret) : "Nd" (port));
	return ret;
}

void outb(uint16_t port, uint8_t val) {
	__asm__ volatile("outb %0, %1" :: "a" (val), "Nd" (port));
}

void outw(uint16_t port, uint16_t val) {
	__asm__ volatile("outw %0, %1" :: "a" (val), "Nd" (port));
}

void outl(uint16_t port, uint32_t val) {
	__asm__ volatile("outl %0, %1" :: "a" (val), "Nd" (port));
}
