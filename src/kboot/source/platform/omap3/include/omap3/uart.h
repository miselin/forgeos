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
 * @brief		OMAP3 UART definitions.
 */

#ifndef __OMAP3_UART_H
#define __OMAP3_UART_H

/** Port to use for debug output. FIXME: This is only correct for BeagleBoard. */
#define DEBUG_UART	2

/** UART port definitions. */
#define UART_RHR_REG		0	/**< Receive Holding Register. */
#define UART_THR_REG		0	/**< Transmit Holding Register. */
#define UART_DLL_REG		0	/**< Divisor Latches Low. */
#define UART_DLH_REG		1	/**< Divisor Latches High. */
#define UART_IER_REG		1	/**< Interrupt Enable Register. */
#define UART_EFR_REG		2	/**< Enhanced Feature Register. */
#define UART_FCR_REG		2	/**< FIFO Control Register. */
#define UART_LCR_REG		3	/**< Line Control Register. */
#define UART_MCR_REG		4	/**< Modem Control Register. */
#define UART_LSR_REG		5	/**< Line Status Register. */
#define UART_MDR1_REG		8	/**< Mode Definition Register 1. */

/** Base clock rate (48MHz). */
#define UART_CLOCK		48000000

#endif /* __OMAP3_UART_H */
