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

#ifndef _POWERMAN_H
#define _POWERMAN_H

#include <types.h>

#define POWERMAN_STATE_WORKING      0
#define POWERMAN_STATE_HALT         1
#define POWERMAN_STATE_CPUOFF       2
#define POWERMAN_STATE_STANDBY      3
#define POWERMAN_STATE_HIBERNATE    4
#define POWERMAN_STATE_OFF          5
#define POWERMAN_STATE_MAX          5

#define POWERMAN_EVENT_RTC          0x1000
#define POWERMAN_EVENT_POWERBUTTON  0x1001
#define POWERMAN_EVENT_SLEEPBUTTON  0x1002

typedef int (*powerman_callback_t)(int);

/**
 * \brief Early initialisation for power management.
 *
 * Some platforms won't need to do anything for early initialisation, but others
 * may want to load metadata and tables and such before too much of the system
 * is loaded, perhaps because those platforms mix power management and device
 * enumeration.
 *
 * Platforms need to offer platform_powerman_earlyinit(). This can do nothing if
 * that makes sense for the platform.
 *
 * This function alone is not enough for power management to be ready to use.
 */
extern int powerman_earlyinit();

/**
 * \brief Full initialisation of power management.
 *
 * Performs initialisation of the power management subsystem. Platforms do this
 * in different ways.
 *
 * Platforms offer platform_powerman_init() for this purpose.
 */
extern int powerman_init();

/**
 * \brief Install a callback function for a power state change.
 *
 * This function allows code in the kernel to install a callback to be notified
 * if the power state of the system changes for some reason. The callback is
 * called for all power state changes, but the callback does not need to handle
 * all of them (it can choose to handle only "OFF", for example).
 *
 * The intent of this function is for device drivers and other important modules
 * to be able to find out when power state transitions happen and react
 * accordingly, perhaps by shutting down a device, or flushing a disk cache.
 *
 * The parameter to the callback is always the pending new power state for the
 * system.
 */
extern void powerman_installcallback(powerman_callback_t cb);

/**
 * Remove the given callback function.
 */
extern void powerman_removecallback(powerman_callback_t cb);

/**
 * \brief Enter a new power state, calling callbacks along the way.
 *
 * It can be safely assumed that if this function returns zero, that the system
 * has successfully entered the new power state and then returned back to the
 * previous state. That is, if you call powerman_enter(POWERMAN_STATE_STANDBY),
 * and the return value is zero, the system has been put into standby and then
 * resumed during the function call.
 */
extern int powerman_enter(int new_state);

/**
 * \brief Notify the power management subsystem of a particular event.
 *
 * There are various events that can take place during the runtime of the kernel
 * including external inputs and internal requests. This provides a central
 * place to request the power management system respond to a particular event.
 */
extern int powerman_event(int event);

/**
 * Platform-specific early initialisation.
 */
extern int platform_powerman_earlyinit();

/**
 * Platform-specific initialisation.
 */
extern int platform_powerman_init();

/**
 * Platform-specific preparation to enter a new power state.
 */
extern int platform_powerman_prep(int new_state);

/**
 * Platform-specific entry to a new power state.
 */
extern int platform_powerman_enter(int new_state);

#endif
