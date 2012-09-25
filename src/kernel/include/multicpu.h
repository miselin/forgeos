/*
 * Copyright (c) 2012 Matthew Iselin
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

#ifndef _MULTICPU_H
#define _MULTICPU_H

#include <types.h>

/**
 * \brief Initialise multi-CPU support in the system.
 *
 * This is implemented by the machine layer, which should detect available CPUs,
 * if that hasn't been done already. Any housekeeping work should also be done
 * in this call.
 *
 * After this call, the system must be able to get a valid value from
 * multicpu_count() - any other call is permitted to fail gracefully.
 *
 * Actual CPU startup is done later.
 */
extern int multicpu_init();

/**
 * \brief Start the CPU referred to by the given index.
 *
 * The machine layer may decide how to interpret this index however it chooses.
 * It is merely a generic abstraction that the kernel can use to work with CPUs
 * without knowing their hardware-specific IDs.
 */
extern int multicpu_start(uint32_t cpu);

/**
 * \brief Stop/halt the given CPU.
 *
 * The machine layer may implement this in the way that makes the most sense for
 * the machine.
 */
extern int multicpu_halt(uint32_t cpu);

/**
 * \brief Code run to initialise a newly started CPU to be ready to go.
 *
 * The machine layer knows what is important and what is not important, and it
 * is up to the machine layer to perform this initialisation.
 */
extern void multicpu_cpuinit();

/**
 * Get the machine-specific ID of the currently executing processor.
 */
extern uint32_t multicpu_id();

/**
 * Get the number of CPUs in the system (logical + physical).
 */
extern uint32_t multicpu_count();

#endif

