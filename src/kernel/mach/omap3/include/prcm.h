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
#ifndef _PRCM_H
#define _PRCM_H

#include <types.h>

#define CLOCK_32K       0
#define CLOCK_SYS       1
#define CLOCK_L3        0
#define CLOCK_L4        2

#define CLOCK_DIV1      1
#define CLOCK_DIV2      2

/// PER clock selection.
extern void per_clocksel(size_t clock, int which);

/// Enable/disable functional clock for PER
extern void per_funcclock(size_t clock, int enable);

/// Enable/disable interface clock for PER
extern void per_ifaceclock(size_t clock, int enable);

/// Enable/disable functional clock for CORE.
extern void core_funcclock(size_t n, size_t clock, int enable);

/// Enable/disable interface clock for CORE.
extern void core_ifaceclock(size_t n, size_t clock, int enable);

/// Wait for CORE idle.
extern void core_waitidle(size_t n, size_t clock, int waitforon);

/// Wait for PLL idle.
extern void pll_waitidle(size_t n, size_t clock, int waitforon);

/// Core clock selection.
extern void core_clocksel(size_t clock, int which);

/// Enable/disable a PLL clock.
extern void pll_setclock(size_t n, size_t value);

/// PLL clock selection.
extern void pll_selclock(size_t n, size_t value);

/// Map in the PCRM MMIO regions, etc...
extern void init_prcm();

#endif
