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
 * @brief		OMAP3 core definitions.
 */

#ifndef __OMAP3_OMAP3_H
#define __OMAP3_OMAP3_H

/** Physical memory region definitions. */
#define OMAP3_GPRAM_BASE	0x00000000	/**< GPMC (Q0) base. */
#define OMAP3_GPRAM_SIZE	0x40000000	/**< GPMC size (1GB). */
#define OMAP3_L4_BASE		0x48000000	/**< L4-Core base. */
#define OMAP3_L4_SIZE		0x01000000	/**< L4-Core size (16MB). */
#define OMAP3_L4_WAKEUP_BASE	0x48300000	/**< L4-Wakeup base. */
#define OMAP3_L4_WAKEUP_SIZE	0x00040000	/**< L4-Wakeup size (256KB). */
#define OMAP3_L4_PER_BASE	0x49000000	/**< L4-Peripheral base. */
#define OMAP3_L4_PER_SIZE	0x00100000	/**< L4-Peripheral size (1MB). */
#define OMAP3_L4_EMU_BASE	0x54000000	/**< L4-Emulation base. */
#define OMAP3_L4_EMU_SIZE	0x00800000	/**< L4-Emulation size (8MB). */
#define OMAP3_SDRAM_BASE	0x80000000	/**< SDMC (Q2) base. */
#define OMAP3_SDRAM_SIZE	0x40000000	/**< SDMC size (1GB). */

/** UART base addresses. */
#define OMAP3_UART1_BASE	(OMAP3_L4_BASE + 0x6A000)
#define OMAP3_UART2_BASE	(OMAP3_L4_BASE + 0x6C000)
#define OMAP3_UART3_BASE	(OMAP3_L4_PER_BASE + 0x20000)

#endif /* __OMAP3_OMAP3_H */
