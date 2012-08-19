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

#include <system.h>
#include <types.h>
#include <prcm.h>
#include <vmem.h>
#include <io.h>
#include <mmiopool.h>

#define PRCM_BASE 0x48004000

static vaddr_t prcm_virt = 0;

// Offsets from prcm_virt for different sections of the PRCM module.
#define CM_IVA2         0x0000
#define SYSREG          0x0800
#define CM_MPU          0x0900
#define CM_CORE         0x0A00
#define CM_SGX          0x0B00
#define CM_WKUP         0x0C00
#define CLOCKCTL        0x0D00
#define CM_DSS          0x0E00
#define CM_CAM          0x0F00
#define CM_PER          0x1000
#define CM_EMU          0x1100
#define GLOBALREG       0x1200
#define CM_NEON         0x1300
#define CM_USBHOST      0x1400

#define PER_FCLKEN      0x00
#define PER_ICLKEN      0x10
#define PER_IDLEST      0x20
#define PER_AUTOIDLE    0x30
#define PER_CLKSEL      0x40
#define PER_SLEEPDEP    0x44
#define PER_CLKSTCRL    0x48
#define PER_CLKSTST     0x4C

#define CORE_FCLKEN1    0x00
#define CORE_FCLKEN3    0x08
#define CORE_ICLKEN1    0x10
#define CORE_ICLKEN3    0x18
#define CORE_IDLEST1    0x20
#define CORE_IDLEST3    0x28
#define CORE_AUTOIDLE1  0x30
#define CORE_AUTOIDLE3  0x38
#define CORE_CLKSEL     0x40
#define CORE_CLKSTCTRL  0x48
#define CORE_CLKSTST    0x4C

#define PLL_CLKEN       0x00
#define PLL_CLKEN2      0x04
#define PLL_IDLEST      0x20
#define PLL_IDLEST2     0x24
#define PLL_AUTOIDLE    0x30
#define PLL_AUTOIDLE2   0x34
#define PLL_CLKSEL1     0x40
#define PLL_CLKSEL2     0x44
#define PLL_CLKSEL3     0x48
#define PLL_CLKSEL4     0x4C
#define PLL_CLKSEL5     0x50
#define PLL_CLKOUT      0x70

void per_clocksel(size_t clock, int which) {
    if(clock > 0) {
        --clock;
        uint32_t bit = 1 << clock;

        volatile uint32_t *clksel = (volatile uint32_t *) (prcm_virt + CM_PER + PER_CLKSEL);

        uint32_t val = *clksel;
        if(which == CLOCK_32K) {
            val &= ~bit;
        } else {
            val |= bit;
        }
        *clksel = val;
    }
}

void per_funcclock(size_t clock, int enable) {
    if(clock > 0) {
        clock += 2;
        uint32_t bit = 1 << clock;

        volatile uint32_t *clksel = (volatile uint32_t *) (prcm_virt + CM_PER + PER_FCLKEN);

        uint32_t val = *clksel;
        if(enable)
            val |= bit;
        else
            val &= ~bit;
        *clksel = val;
    }
}

void per_ifaceclock(size_t clock, int enable) {
    if(clock > 0) {
        clock += 2;
        uint32_t bit = 1 << clock;

        volatile uint32_t *clksel = (volatile uint32_t *) (prcm_virt + CM_PER + PER_ICLKEN);

        uint32_t val = *clksel;
        if(enable)
            val |= bit;
        else
            val &= ~bit;
        *clksel = val;
    }
}

void core_funcclock(size_t n, size_t clock, int enable) {
    uint32_t bit = 1 << clock;

    vaddr_t base = 0;
    if(n == 1)
        base = CORE_FCLKEN1;
    else if(n == 3)
        base = CORE_FCLKEN3;

    volatile uint32_t *clksel = (volatile uint32_t *) (prcm_virt + CM_CORE + base);

    uint32_t val = *clksel;
    if(enable)
        val |= bit;
    else
        val &= ~bit;
    *clksel = val;
}

void core_ifaceclock(size_t n, size_t clock, int enable) {
    uint32_t bit = 1 << clock;

    vaddr_t base = 0;
    if(n == 1)
        base = CORE_ICLKEN1;
    else if(n == 3)
        base = CORE_ICLKEN3;

    volatile uint32_t *clksel = (volatile uint32_t *) (prcm_virt + CM_CORE + base);

    uint32_t val = *clksel;
    if(enable)
        val |= bit;
    else
        val &= ~bit;
    *clksel = val;
}

void core_waitidle(size_t n, size_t clock, int waitforon) {
    uint32_t bit = 1 << clock;

    vaddr_t base = 0;
    if(n == 1)
        base = CORE_IDLEST1;
    else if(n == 3)
        base = CORE_IDLEST3;

    volatile uint32_t *clksel = (volatile uint32_t *) (prcm_virt + CM_CORE + base);

    if(waitforon)
        while(!(*clksel & bit));
    else
        while(*clksel & bit);
}

void pll_waitidle(size_t n, size_t clock, int waitforon) {
    uint32_t bit = 1 << clock;

    vaddr_t base = 0;
    if(n == 1)
        base = PLL_IDLEST;
    else if(n == 2)
        base = PLL_IDLEST2;

    volatile uint32_t *clksel = (volatile uint32_t *) (prcm_virt + CLOCKCTL + base);

    if(waitforon)
        while(!(*clksel & bit));
    else
        while(*clksel & bit);
}

void core_clocksel(size_t clock, int which) {
    uint32_t bit = 1 << clock;
    uint32_t set = which << clock;

    volatile uint32_t *clksel = (volatile uint32_t *) (prcm_virt + CM_CORE + CORE_CLKSEL);

    uint32_t mask = 0;
    if(clock == CLOCK_L3)
        mask = 0x3;
    else if(clock == CLOCK_L4)
        mask = 0xC;
    else
        mask = bit;

    uint32_t val = *clksel;
    val &= ~mask;
    val |= set;
    *clksel = val;
}

void pll_setclock(size_t n, size_t value) {
    vaddr_t base = 0;
    if(n == 1)
        base = PLL_CLKEN;
    else if(n == 2)
        base = PLL_CLKEN2;

    volatile uint32_t *clksel = (volatile uint32_t *) (prcm_virt + CLOCKCTL + base);
    *clksel = value;
}

void pll_selclock(size_t n, size_t value) {
    vaddr_t base = 0;
    if(n == 1)
        base = PLL_CLKSEL1;
    else if(n == 2)
        base = PLL_CLKSEL2;
    else if(n == 3)
        base = PLL_CLKSEL3;
    else if(n == 4)
        base = PLL_CLKSEL4;
    else if(n == 5)
        base = PLL_CLKSEL5;

    volatile uint32_t *clksel = (volatile uint32_t *) (prcm_virt + CLOCKCTL + base);
    *clksel = value;
}

void init_prcm() {
    prcm_virt = (vaddr_t) mmiopool_alloc(PAGE_SIZE * 2, PRCM_BASE);

    dprintf("ARMv7 omap3 PRCM module - mapping complete.\n");
}
