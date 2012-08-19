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

#ifndef _DEVICES_H
#define _DEVICES_H

/// Initialises basic machine-specific devices required early on. This includes
/// things such as the serial port, and timers.
#define init_devices mach_init_devices

extern void mach_init_devices();

#ifdef MACH_REQUIRES_EARLY_DEVINIT
/// Initialises the machine to a state where we can assume serial_write will
/// work and as such we can continue to initialise the MMU and such before
/// configuring additional devices.
#define init_devices_early mach_init_devices_early

/// Performs any extra initialisation that may be required early, but that
/// requires the MMU and/or physical memory to be functional.
#define init_devices_early2 mach_init_devices_early2

extern void mach_init_devices_early();
extern void mach_init_devices_early2();
#endif

#endif
