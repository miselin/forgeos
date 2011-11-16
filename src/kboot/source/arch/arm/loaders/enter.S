/*
 * Copyright (C) 2011 Alex Smith
 *
 * Permission to use, copy, modify, and/or distribute this software for any
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

/**
 * @file
 * @brief		ARM kernel entry code.
 */

#include <arm/asm.h>
#include <kboot.h>

/** Enter an ARM KBoot kernel.
 * @param tags		Tag list address.
 * @param ttbr0		Address to load into TTBR0.
 * @param entry		Kernel entry point. */
FUNCTION_START(kboot_arch_enter_real)
	/* Ensure that IRQs/FIQs are disabled. */
	cpsid	if

	/* Write the first level descriptor table address to TTBR0. */
	mcr	p15, 0, r1, c2, c0, 0

	/* Ensure that TTBCR is configured to always use TTBR0. */
	mov	r4, #0
	mcr	p15, 0, r4, c2, c0, 2

	/* Enable full access to all domains. */
	mvn	r4, #0
	mcr	p15, 0, r4, c3, c0, 0

	/* Enable the I/D caches (SCTLR.I and SCTLR.C). */
	mrc	p15, 0, r4, c1, c0, 0
	orr	r4, r4, #(1<<12)
	orr	r4, r4, #(1<<2)
	mcr	p15, 0, r4, c1, c0, 0

	/* Invalidate I/D TLB. */
	mcr	p15, 0, r0, c8, c7, 0

	/* Enable the MMU by setting SCTLR.M to 1. */
	orr	r4, r4, #(1<<0)
	mcr	p15, 0, r4, c1, c0, 0

	/* Ensure that changes made are visible to any following instructions. */
	isb

	/* Call the kernel. */
	mov	r1, r0
	ldr	r0, =KBOOT_MAGIC
	mov	pc, r2
FUNCTION_END(kboot_arch_enter_real)
