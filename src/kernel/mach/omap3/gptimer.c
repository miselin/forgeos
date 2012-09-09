/*
 * Copyright (c) 2012 Matthew Iselin, Rich Edelman
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
#include <system.h>
#include <interrupts.h>
#include <compiler.h>
#include <io.h>
#include <timer.h>
#include <vmem.h>
#include <assert.h>
#include <prcm.h>
#include <mmiopool.h>

#define GPTIMER_COUNT       11

static volatile uint32_t *gptimer[GPTIMER_COUNT] = {0};

static paddr_t gptimer_phys[] = {
    0x48318000,
    0x49032000,
    0x49034000,
    0x49036000,
    0x49038000,
    0x4903A000,
    0x4903C000,
    0x4903E000,
    0x49040000,
    0x48086000,
    0x48088000
};

#define TIDR        (0x00 / 4)
#define TIOCP_CFG   (0x10 / 4)
#define TISTAT      (0x14 / 4)
#define TISR        (0x18 / 4)
#define TIER        (0x1C / 4)
#define TWER        (0x20 / 4)
#define TCLR        (0x24 / 4)
#define TCRR        (0x28 / 4)
#define TLDR        (0x2C / 4)
#define TTGR        (0x30 / 4)
#define TWPS        (0x34 / 4)
#define TMAR        (0x38 / 4)
#define TCAR1       (0x3C / 4)
#define TSICR       (0x40 / 4)
#define TCAR2       (0x44 / 4)
#define TPIR        (0x48 / 4)
#define TNIR        (0x4C / 4)
#define TCVR        (0x50 / 4)
#define TOCR        (0x54 / 4)
#define TOWR        (0x58 / 4)

static struct timer gp0, gp1, gp2, gp3, gp4, gp5, gp6, gp7, gp8, gp9, gp10;
static struct timer *timers[] = {
    &gp0, &gp1, &gp2, &gp3, &gp4, &gp5, &gp6, &gp7, &gp8, &gp9, &gp10
};

/// GP Timer IRQ
int irq_omap3_gptimer(size_t n, struct intr_stack *s __unused) {
    int ret = timer_ticked(timers[n], ((1 << TIMERRES_SHIFT) | TIMERRES_MILLI));

    // ACK the interrupt.
    gptimer[n][TISR] = gptimer[n][TISR];

    return ret;
}

/// All init_omap3_gpn functions end up here.
int init_omap3_gptimer(size_t n, inthandler_t irqhandler) {
    assert(n < sizeof gptimer_phys);

    // Configure clock sources and enable clocks for timers other than GPTIMER1.
    if(n > 0) {
        // During reset, we want to use the SYS_CLK.
        per_clocksel(n, CLOCK_SYS);

        // Enable interface and functional clocks.
        per_funcclock(n, 1);
        per_ifaceclock(n, 1);
    }

    // Map in the timer memory.
    gptimer[n] = (volatile uint32_t *) mmiopool_alloc(PAGE_SIZE, gptimer_phys[n]);

    // Display revision.
    uint8_t rev = gptimer[n][TIDR] & 0xFF;
    dprintf("ARMv7 OMAP3 GP Timer %d Revision %d.%d\n", n, (rev >> 4), (rev & 0xF));

    // Reset the timer.
    gptimer[n][TIOCP_CFG] = 0x2;
    gptimer[n][TSICR] = 0x2;
    dprintf("ptr: %x\n", gptimer[n]);
    dprintf("status: %x\n", gptimer[n][TISTAT]);
    while((gptimer[n][TISTAT] & 1) == 0);

    dprintf("reset complete\n");

    // Use the 32 kHZ functional clock now that reset is complete.
    if(n > 0) {
        per_clocksel(n, CLOCK_32K);
    }

    dprintf("1\n");

    // Set up for 1 ms ticks. 16.2.4.2.1, OMAP35xx manual.
    gptimer[n][TPIR] = 232000;
    gptimer[n][TNIR] = (uint32_t) -768000;
    gptimer[n][TLDR] = 0xFFFFFFE0;
    gptimer[n][TTGR] = 1;

    dprintf("2\n");

    // Clear any existing interrupts pending.
    gptimer[n][TISR] = 0x7;

    dprintf("3\n");

    // Install our IRQ handler.
    interrupts_irq_reg(37 + (int) n, 1, irqhandler);

    dprintf("4\n");

    // Enable overflow interrupt - will fire an IRQ when the tick counter overflows.
    gptimer[n][TIER] = 0x2;

    dprintf("5\n");

    // Start the timer!
    gptimer[n][TCLR] = 0x3;

    dprintf("timer init done\n");

    return 0;
}

#define INIT_GP(n) \
    int init_omap3_gp##n () { \
        return init_omap3_gptimer(n, irq_omap3_gp##n); \
    }

#define IRQ_GP(n) \
    int irq_omap3_gp##n (struct intr_stack *s) { \
        return irq_omap3_gptimer(n, s); \
    }

#define DEFINE_OMAP3_GPTIMER(n) \
    static struct timer gp##n = { \
        ((1 << TIMERRES_SHIFT) | TIMERRES_MILLI), \
        TIMERFEAT_PERIODIC, \
        "General Purpose Timer #" #n, \
        init_omap3_gp##n, \
        0, \
        0, \
        0, \
        0 \
    }; \
    EXPORT_TIMER(gptimer##n, gp##n);

IRQ_GP(0)
IRQ_GP(1)
IRQ_GP(2)
IRQ_GP(3)
IRQ_GP(4)
IRQ_GP(5)
IRQ_GP(6)
IRQ_GP(7)
IRQ_GP(8)
IRQ_GP(9)
IRQ_GP(10)

INIT_GP(0)
INIT_GP(1)
INIT_GP(2)
INIT_GP(3)
INIT_GP(4)
INIT_GP(5)
INIT_GP(6)
INIT_GP(7)
INIT_GP(8)
INIT_GP(9)
INIT_GP(10)

DEFINE_OMAP3_GPTIMER(0)
/*
DEFINE_OMAP3_GPTIMER(1)
DEFINE_OMAP3_GPTIMER(2)
DEFINE_OMAP3_GPTIMER(3)
DEFINE_OMAP3_GPTIMER(4)
DEFINE_OMAP3_GPTIMER(5)
DEFINE_OMAP3_GPTIMER(6)
DEFINE_OMAP3_GPTIMER(7)
DEFINE_OMAP3_GPTIMER(8)
DEFINE_OMAP3_GPTIMER(9)
DEFINE_OMAP3_GPTIMER(10)
*/
