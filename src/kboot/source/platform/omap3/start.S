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
 * @brief		OMAP3 loader entry point.
 */

#include <arch/page.h>
#include <arm/asm.h>
#include <platform/system.h>

/* Ensure that this code is linked in first. */
.section .init.text, "ax", %progbits

/** Main entry point for the loader. */
FUNCTION_START(_start)
	/* We are loaded using the ARM Linux boot procedure.
	 * Processor state upon entry:
	 *  - SVC (supervisor) mode with IRQ and FIQ interrupts disabled.
	 *  - MMU off.
	 *  - Data cache off.
	 *  - Instruction cache on or off.
	 * Registers:
	 *  R0 - 0
	 *  R1 - Machine type.
	 *  R2 - Physical address of ATAG list.
	 */

	/* Load the stack address. We don't care about setting stacks for other
	 * modes here, we shouldn't get any exceptions or interrupts while
	 * we're in the loader. */
	ldr	sp, =loader_stack + PAGE_SIZE

	/* Save the ATAG list pointer (R4 is callee-save). */
	mov	r4, r2

	/* Clear out the BSS section. */
	ldr	r0, =__bss_start
	mov	r1, #0
	ldr	r2, =__bss_end
	sub	r2, r2, r0
	bl	memset

	/* Store the ATAG list address. */
	ldr	r0, =atag_list
	str	r4, [r0]

	/* Call the main function. */
	b	loader_main
FUNCTION_END(_start)

.section .data, "aw", %progbits

/** Stack.
 * @note		Must not be placed in .bss, as it is cleared in
 *			loader_main and the stack would be overwritten. */
.align PAGE_WIDTH
SYMBOL(loader_stack)
	.fill	PAGE_SIZE
