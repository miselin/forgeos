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
#include <mmiopool.h>
#include <serial.h>
#include <system.h>
#include <vmem.h>
#include <util.h>
#include <io.h>

#define UART1_PHYS      0x4806A000
#define UART2_PHYS      0x4806C000
#define UART3_PHYS      0x49020000

/// OMAP3 UART addresses.
volatile uint8_t *uart1 = (volatile uint8_t*) UART1_PHYS;
volatile uint8_t *uart2 = (volatile uint8_t*) UART2_PHYS;
volatile uint8_t *uart3 = (volatile uint8_t*) UART3_PHYS;

/** UART/IrDA/CIR Registers */
#define DLL_REG         0x00 // R/W
#define RHR_REG         0x00 // R
#define THR_REG         0x00 // W
#define DLH_REG         0x04 // R/W
#define IER_REG         0x04 // R/W
#define IIR_REG         0x08 // R
#define FCR_REG         0x08 // W
#define EFR_REG         0x08 // RW
#define LCR_REG         0x0C // RW
#define MCR_REG         0x10 // RW
#define XON1_ADDR1_REG  0x10 // RW
#define LSR_REG         0x14 // R
#define XON2_ADDR2_REG  0x14 // RW
#define MSR_REG         0x18 // R
#define TCR_REG         0x18 // RW
#define XOFF1_REG       0x18 // RW
#define SPR_REG         0x1C // RW
#define TLR_REG         0x1C // RW
#define XOFF2_REG       0x1C // RW
#define MDR1_REG        0x20 // RW
#define MDR2_REG        0x24 // RW

#define USAR_REG        0x38 // R

#define SCR_REG         0x40 // RW
#define SSR_REG         0x44 // R

#define MVR_REG         0x50 // R
#define SYSC_REG        0x54 // RW
#define SYSS_REG        0x58 // R
#define WER_REG         0x5C // RW

void machine_clear_screen() {
    int i;

    // Naive clear screen. Can we assume a terminal emulator is present??
    machine_putc('\r');
    for(i = 0; i < 100; i++)
        machine_putc('\n');
}

/// Gets a UART MMIO block given a number
static volatile uint8_t *uart_get(int n)
{
    if(n == 1)
        return uart1;
    else if(n == 2)
        return uart2;
    else if(n == 3)
        return uart3;
    else
        return 0;
}

/// Perform a software reset of a given UART
static int uart_softreset(int n)
{
    volatile uint8_t *uart = uart_get(n);
    if(!uart)
        return 0;

    /** Reset the UART. Page 2677, section 17.5.1.1.1 **/

    // 1. Initiate a software reset
    uart[SYSC_REG] |= 0x2;

    // 2. Wait for the end of the reset operation
    while(!(uart[SYSS_REG] & 0x1));

    return 1;
}

/// Configure FIFOs and DMA to default values
static int uart_fifodefaults(int n)
{
    volatile uint8_t *uart = uart_get(n);
    if(!uart)
        return 0;

    /** Configure FIFOs and DMA **/

    // 1. Switch to configuration mode B to access the EFR_REG register
    uint8_t old_lcr_reg = uart[LCR_REG];
    uart[LCR_REG] = 0xBF;

    // 2. Enable submode TCR_TLR to access TLR_REG (part 1 of 2)
    uint8_t efr_reg = uart[EFR_REG];
    uint8_t old_enhanced_en = efr_reg & 0x8;
    if(!(efr_reg & 0x8)) // Set ENHANCED_EN (bit 4) if not set
        efr_reg |= 0x8;
    uart[EFR_REG] = efr_reg; // Write back to the register

    // 3. Switch to configuration mode A
    uart[LCR_REG] = 0x80;

    // 4. Enable submode TCL_TLR to access TLR_REG (part 2 of 2)
    uint8_t mcr_reg = uart[MCR_REG];
    uint8_t old_tcl_tlr = mcr_reg & 0x20;
    if(!(mcr_reg & 0x20))
        mcr_reg |= 0x20;
    uart[MCR_REG] = mcr_reg;

    // 5. Enable FIFO, load new FIFO triggers (part 1 of 3), and the new DMA mode (part 1 of 2)
    uart[FCR_REG] = 1; // TX and RX FIFO triggers at 8 characters, no DMA mode

    // 6. Switch to configuration mode B to access EFR_REG
    uart[LCR_REG] = 0xBF;

    // 7. Load new FIFO triggers (part 2 of 3)
    uart[TLR_REG] = 0;

    // 8. Load new FIFO triggers (part 3 of 3) and the new DMA mode (part 2 of 2)
    uart[SCR_REG] = 0;

    // 9. Restore the ENHANCED_EN value saved in step 2
    if(!old_enhanced_en)
        uart[EFR_REG] = uart[EFR_REG] ^ 0x8;

    // 10. Switch to configuration mode A to access the MCR_REG register
    uart[LCR_REG] = 0x80;

    // 11. Restore the MCR_REG TCR_TLR value saved in step 4
    if(!old_tcl_tlr)
        uart[MCR_REG] = uart[MCR_REG] ^ 0x20;

    // 12. Restore the LCR_REG value stored in step 1
    uart[LCR_REG] = old_lcr_reg;

    return 1;
}

/// Configure the UART protocol (to defaults - 115200 baud, 8 character bits,
/// no paruart_protoconfigity, 1 stop bit). Will also enable the UART for output as a side
/// effect.
static int uart_protoconfig(int n)
{
    volatile uint8_t *uart = uart_get(n);
    if(!uart)
        return 0;

    /** Configure protocol, baud and interrupts **/

    // 1. Disable UART to access DLL_REG and DLH_REG
    uart[MDR1_REG] = (uart[MDR1_REG] & ~0x7) | 0x7;

    // 2. Switch to configuration mode B to access the EFR_REG register
    uart[LCR_REG] = 0xBF;

    // 3. Enable access to IER_REG
    uint8_t efr_reg = uart[EFR_REG];
    uint8_t old_enhanced_en = efr_reg & 0x8;
    if(!(efr_reg & 0x8)) // Set ENHANCED_EN (bit 4) if not set
        efr_reg |= 0x8;
    uart[EFR_REG] = efr_reg; // Write back to the register

    // 4. Switch to operational mode to access the IER_REG register
    uart[LCR_REG] = 0;

    // 5. Clear IER_REG
    uart[IER_REG] = 0;

    // 6. Switch to configuration mode B to access DLL_REG and DLH_REG
    uart[LCR_REG] = 0xBF;

    // 7. Load the new divisor value (looking for 115200 baud)
    uart[0x0] = 0x1A; // divisor low byte
    uart[0x4] = 0; // divisor high byte

    // 8. Switch to operational mode to access the IER_REG register
    uart[LCR_REG] = 0;

    // 9. Load new interrupt configuration
    uart[IER_REG] = 0; // No interrupts wanted at this stage

    // 10. Switch to configuration mode B to access the EFR_REG register
    uart[LCR_REG] = 0xBF;

    // 11. Restore the ENHANCED_EN value saved in step 3
    if(old_enhanced_en)
        uart[EFR_REG] = uart[EFR_REG] ^ 0x8;

    // 12. Load the new protocol formatting (parity, stop bit, character length)
    // and enter operational mode
    uart[LCR_REG] = 0x3; // 8 bit characters, no parity, one stop bit

    // 13. Load the new UART mode
    uart[MDR1_REG] = 0;

    return 1;
}

/// Completely disable flow control on the UART
static int uart_disableflowctl(int n)
{
    volatile uint8_t *uart = uart_get(n);
    if(!uart)
        return 0;

    /** Configure hardware flow control */

    // 1. Switch to configuration mode A to access the MCR_REG register
    uint8_t old_lcr_reg = uart[LCR_REG];
    uart[LCR_REG] = 0x80;

    // 2. Enable submode TCR_TLR to access TCR_REG (part 1 of 2)
    uint8_t mcr_reg = uart[MCR_REG];
    uint8_t old_tcl_tlr = mcr_reg & 0x20;
    if(!(mcr_reg & 0x20))
        mcr_reg |= 0x20;
    uart[MCR_REG] = mcr_reg;

    // 3. Switch to configuration mode B to access the EFR_REG register
    uart[LCR_REG] = 0xBF;

    // 4. Enable submode TCR_TLR to access the TCR_REG register (part 2 of 2)
    uint8_t efr_reg = uart[EFR_REG];
    if(!(efr_reg & 0x8)) // Set ENHANCED_EN (bit 4) if not set
        efr_reg |= 0x8;
    uart[EFR_REG] = efr_reg; // Write back to the register

    // 5. Load new start and halt trigger values
    uart[TCR_REG] = 0;

    // 6. Disable RX/TX hardware flow control mode, and restore the ENHANCED_EN
    // values stored in step 4
    uart[EFR_REG] = 0;

    // 7. Switch to configuration mode A to access MCR_REG
    uart[LCR_REG] = 0x80;

    // 8. Restore the MCR_REG TCR_TLR value stored in step 2
    if(!old_tcl_tlr)
        uart[MCR_REG] = uart[MCR_REG] ^ 0x20;

    // 9. Restore the LCR_REG value saved in step 1
    uart[LCR_REG] = old_lcr_reg;

    return 1;
}

static void uart_write(int n, char c)
{
    volatile uint8_t *uart = uart_get(n);
    if(!uart)
        return;

    // Wait until the hold register is empty
    while(!(uart[LSR_REG] & 0x20));
    uart[THR_REG] = c;
}

static char uart_read(int n)
{
    volatile uint8_t *uart = uart_get(n);
    if(!uart)
        return 0;

    // Wait for data in the receive FIFO
    while(!(uart[LSR_REG] & 0x1));
    return uart[RHR_REG];
}


void machine_putc(char c) {
    serial_write(c);
}

void machine_putc_at(char c __unused, int x __unused, int y __unused) {
    // Use VT-100 codes to make this work.
    /// \todo implement
}

void machine_define_screen_extents(int x __unused, int y __unused) {
}

/// Initialises the UARTs. I know they're technically not serial ports, but this
/// is an easy abstraction.
void init_serial() {
    int i;
    for(i = 0; i < 3; i++) {
        uart_softreset(i + 1);
        uart_fifodefaults(i + 1);
        uart_protoconfig(i + 1);
        uart_disableflowctl(i + 1);
    }

#ifdef DEBUG
    // Wait for one character to be transmitted before continuing.
    // This makes debugging a little easier.
    serial_puts("Push a key to continue startup...\n");
    serial_read();
#endif
}

void serial_write(uint8_t c) {
    // Convert linefeed to carriage return/linefeed.
    if(c == '\n')
        uart_write(3, '\r');
    uart_write(3, c);
}

uint8_t serial_read() {
    return uart_read(3);
}

void arm_mach_uart_disable() {
    // Disable uart_* functions until after allocation is done.
    // All functions will gracefully fail (and dprintf et al will simply return
    // without actually writing any data).
    uart1 = uart2 = uart3 = (volatile uint8_t*) 0;
}

void arm_mach_uart_remap() {
    // Allocate from the MMIO pool for all UARTs.
    uart1 = (volatile uint8_t*) mmiopool_alloc(PAGE_SIZE, UART1_PHYS);
    uart2 = (volatile uint8_t*) mmiopool_alloc(PAGE_SIZE, UART2_PHYS);
    uart3 = (volatile uint8_t*) mmiopool_alloc(PAGE_SIZE, UART3_PHYS);

    dprintf("omap3: mapped UART in virtual memory %x %x %x\n", uart1, uart2, uart3);
}
