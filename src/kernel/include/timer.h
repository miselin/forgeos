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

#ifndef _TIMER_H
#define _TIMER_H

#include <types.h>
#include <compiler.h>

/// Defines the type for a timer handler.
typedef void (*timer_handler)(uint64_t ticks);

/// Type for the table of available timers in the system.
struct timer_table_entry {
	struct timer *tmr;
};

/// Exports a timer entry in the timer table.
#define EXPORT_TIMER(name, ent) \
	struct timer_table_entry _tmr_##name SECTION(".table.timers") UNUSED = {&ent}

// Supported timer resolutions.
#define TIMERRES_TERRIBLE		0x0 // > second resolution
#define TIMERRES_SECONDS		0x1
#define TIMERRES_MILLI			0x2
#define TIMERRES_MICRO			0x4
#define TIMERRES_NANO			0x8

/// Shift the frequency this many bits LEFT, eg, if your timer can do, say,
/// only 2 millisecond resolution, do ((2 << TIMERRES_SHIFT) | TIMERRES_MILLI)
#define TIMERRES_SHIFT			12
#define TIMERRES_MASK			0xFFF

// Supported timer features.
#define TIMERFEAT_USELESS		0x0 // supports nothing
#define TIMERFEAT_ONESHOT		0x1
#define TIMERFEAT_PERIODIC		0x2
#define TIMERFEAT_UNIXTS		0x4 // can give a UNIX timestamp, shiny!
#define TIMERFEAT_COUNTS		0x8 // counts ticks

// Defines a timer.
struct timer {
	uint32_t timer_res;
	uint32_t timer_feat;

	const char *name;

	/// Initialises the timer. Installs interrupt handlers and such.
	/// \return -1 if hardware not present, or not usable. 0 otherwise.
	int (*timer_init)();

	/// De-initialises the timer. Removes interrupt handlers and resets
	/// hardware to a pre-defined state if required.
	int (*timer_deinit)();

	/// Only valid if timer_feat & TIMERFEAT_COUNTS
	uint64_t (*timer_ticks)();

	/// Pause the timer.
	void (*pause)();

	/// Resume the timer.
	void (*resume)();
};

/**
 * \brief Called by a timer to indicate a tick has taken place.
 *
 * Timers should call this with their structure containing resolution
 * and function pointers, to aid the implementation in determining what
 * action to perform for this tick.
 *
 * The @ticks parameter is similarly formatted to timer::timer_res, but
 * indicates the amount of time that has passed since the last call to
 * timer_ticked.
 *
 * That format is ((UNIT_COUNT << TIMERRES_SHIFT) | TIMER_RES).
 */
extern void timer_ticked(struct timer *tim, uint32_t ticks);

/// Initialises the timer framework and initialises timer hardware.
extern void timers_init();

/**
 * \brief Installs a new timer with the given semantics.
 *
 * For one-shot timers, the @ticks parameter refers to the number of ticks
 * from the time of installing into the future at which the timer should
 * fire. Upon firing, the one-shot timer is removed from the system.
 *
 * For periodic timers, the @ticks parameter indicates the number of ticks
 * between each call to timer_ticked.
 *
 * The format for @ticks is the same as that for timer::timer_res, or
 * for timer_ticked.
 *
 * That format is ((UNIT_COUNT << TIMERRES_SHIFT) | TIMER_RES).
 * \todo Turn that into a macro. For the love of all things good.
 *
 * \return -1 if no timer supporting the given features is available, and
 *		   emulation is not possible. 0 otherwise.
 */
extern int install_timer(timer_handler th, uint32_t ticks, uint32_t feat);

/**
 * Removes the given timer handler from the system.
 */
extern void remove_timer(timer_handler th);

#endif
