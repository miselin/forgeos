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

#include <arch/page.h>

#include <omap3/omap3.h>
#include <omap3/uart.h>

#include <kboot.h>
#include <system.h>

extern void console_putc(char ch);
static bool have_inited = false;

KBOOT_MAPPING(0xC1000000, OMAP3_UART1_BASE, PAGE_SIZE);
KBOOT_MAPPING(0xC1001000, OMAP3_UART2_BASE, PAGE_SIZE);
KBOOT_MAPPING(0xC1002000, OMAP3_UART3_BASE, PAGE_SIZE);

/** UART port definitions. */
static volatile uint8_t *uarts[] = {
	(volatile uint8_t *)0xC1000000,
	(volatile uint8_t *)0xC1001000,
	(volatile uint8_t *)0xC1002000,
};

/** Read a register from a UART.
 * @param port		Port to read from.
 * @param reg		Register to read.
 * @return		Value read. */
static inline uint8_t uart_read_reg(int port, int reg) {
	return uarts[port][reg << 2];
}

/** Write a register to a UART.
 * @param port		Port to read from.
 * @param reg		Register to read.
 * @param value		Value to write. */
static inline void uart_write_reg(int port, int reg, uint8_t value) {
	uarts[port][reg << 2] = value;
}

/** Initialise a UART port.
 * @param port		Port number to initialise.
 * @param baud		Baud rate. */
static void uart_init_port(int port, int baud) {
	uint16_t divisor;

	/* Disable UART. */
	uart_write_reg(port, UART_MDR1_REG, 0x7);

	/* Enable access to IER_REG (configuration mode B for EFR_REG). */
	uart_write_reg(port, UART_LCR_REG, 0xBF);
	uart_write_reg(port, UART_EFR_REG, uart_read_reg(port, UART_EFR_REG) | (1<<4));

	/* Disable interrupts and sleep mode (operational mode). */
	uart_write_reg(port, UART_LCR_REG, 0);
	uart_write_reg(port, UART_IER_REG, 0);

	/* Clear and enable FIFOs. Must be done when clock is not running, so
	 * set DLL_REG and DLH_REG to 0. All done in configuration mode A. */
	uart_write_reg(port, UART_LCR_REG, (1<<7));
	uart_write_reg(port, UART_DLL_REG, 0);
	uart_write_reg(port, UART_DLH_REG, 0);
	uart_write_reg(port, UART_FCR_REG, (1<<0) | (1<<1) | (1<<2));

	/* Now program the divisor to set the baud rate.
	 *  Baud rate = (functional clock / 16) / N */
	divisor = (UART_CLOCK / 16) / baud;
	uart_write_reg(port, UART_DLL_REG, divisor & 0xff);
	uart_write_reg(port, UART_DLH_REG, (divisor >> 8) & 0x3f);

	/* Configure for 8N1 (8-bit, no parity, 1 stop-bit), and switch to
	 * operational mode. */
	uart_write_reg(port, UART_LCR_REG, 0x3);

	/* Enable RTS/DTR. */
	uart_write_reg(port, UART_MCR_REG, (1<<0) | (1<<1));

	/* Enable UART in 16x mode. */
	uart_write_reg(port, UART_MDR1_REG, 0);
}

/** Write a character to a UART.
 * @param port		Port to write.
 * @param ch		Character to write. */
static void uart_putch(int port, unsigned char ch) {
	/* Wait for the TX FIFO to be empty. */
	while(!(uart_read_reg(port, UART_LSR_REG) & (1<<6))) {}

	/* Write the character. */
	uart_write_reg(port, UART_THR_REG, ch);
}

/** Write a character to the UART console.
 * @param ch		Character to write. */
void console_putc(char ch) {
	if(!have_inited) {
		uart_init_port(DEBUG_UART, 115200);
		uart_putch(DEBUG_UART, '\n');
		have_inited = true;
	}
	uart_putch(DEBUG_UART, ch);
}
