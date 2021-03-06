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

typedef int (*crosscpu_func_t)(void *p);

#define MULTICPU_PERCPU_CURRTHREAD      0
#define MULTICPU_PERCPU_IDLETHREAD      1
#define MULTICPU_PERCPU_CPUTIMER        2

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
 * \brief Run a function call on another CPU.
 *
 * Sometimes it may be necessary for a function call to be run in a context that
 * is not on the current CPU. For example, a timer handler may need to run, but
 * it was installed on a different core to the currently executing CPU.
 *
 * This function will block until the receiving CPU begins executing the given
 * code.
 *
 * \param cpu Machine-specific CPU ID.
 */
extern void multicpu_call(uint32_t cpu, crosscpu_func_t func, void *param);

/**
 * \brief Request all other CPUs to reschedule immediately.
 *
 * In FORGE, when one CPU reschedules, all other CPUs must reschedule at the
 * same time. This allows the BSP to continue being the primary destination for
 * IRQs and system events, while APs can handle the user load.
 *
 * The mechanics of how other CPUs are notified are machine-specific.
 */
extern void multicpu_doresched();

/**
 * Get the machine-specific ID of the currently executing processor.
 */
extern uint32_t multicpu_id();

/**
 * Get the machine-specific ID of a processor based on an index.
 */
extern uint32_t multicpu_idxtoid(uint32_t idx);

/**
 * Get the number of CPUs in the system (logical + physical).
 */
extern uint32_t multicpu_count();

/**
 * \brief Gets a pointer into the per-CPU data area at the given index.
 *
 * There are a lot of data structures in the kernel that rely on data that can't
 * be shared across the entire system. For example, in the scheduler, data about
 * the threads running on the core is not relevant to any other core.
 *
 * This API provides a means for modules such as the scheduler to set aside
 * per-CPU variables without having to use, for example, a tree with the CPU ID
 * as a key.
 *
 * \param n index in the data area to use (< PAGE_SIZE / sizeof unative_t)
 * \return pointer to the data area indexed
 */
extern void *multicpu_percpu_at(size_t n);

#endif

