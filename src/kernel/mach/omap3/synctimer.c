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

#define SYNCTIMER_VIRT  (MMIO_BASE + 0xF000)
#define SYNCTIMER_PHYS  0x48320000

int init_omap3_synctimer() {
    // Map in the MMIO region, display version.
    vmem_map(SYNCTIMER_VIRT, SYNCTIMER_PHYS, VMEM_READWRITE | VMEM_SUPERVISOR | VMEM_DEVICE | VMEM_GLOBAL);
    volatile uint32_t *synctimer = (volatile uint32_t *) SYNCTIMER_VIRT;

    // Display information.
    uint8_t rev = synctimer[0] & 0xFF;
    dprintf("ARMv7 OMAP3 Sync Timer Revision %d.%d\n", (rev >> 4), (rev & 0xF));

    return 0;
}

uint64_t omap3_synctimer_ticks() {
    volatile uint32_t *synctimer = (volatile uint32_t *) SYNCTIMER_VIRT;
    return (uint64_t) synctimer[4];
}

static struct timer t = {
    ((32 << TIMERRES_SHIFT) | TIMERRES_MICRO),
    TIMERFEAT_COUNTS,
    "32kHZ Sync Timer",
    init_omap3_synctimer,
    0,
    omap3_synctimer_ticks,
    0,
    0
};

EXPORT_TIMER(omap3synctimer, t);
