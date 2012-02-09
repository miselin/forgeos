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

#ifndef _SYSTEM_H
#define _SYSTEM_H

#define PHYS_ADDR		0x100000

#define KERNEL_BASE		0xC0000000
#define HEAP_BASE		0xD0000000
#define POOL_BASE       0xE0000000
#define STACK_TOP		0xFFC00000
#define STACK_SIZE		0x4000 // 16 KB

#define PAGE_SIZE		0x1000

#define log2phys(x)		(((x) - KERNEL_BASE) + PHYS_ADDR)
#define phys2log(x)		(((x) - PHYS_ADDR) + KERNEL_BASE)

#endif
