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
 * @brief		ARM KBoot kernel loader.
 */

#include <arch/page.h>

#include <platform/system.h>

#include <elf_load.h>
#include <kboot.h>
#include <loader.h>
#include <mmu.h>

extern mmu_context_t *kboot_arch_load(file_handle_t *handle, phys_ptr_t *physp);
extern void kboot_arch_enter(mmu_context_t *ctx, phys_ptr_t tags) __noreturn;
extern void kboot_arch_enter_real(phys_ptr_t tags, ptr_t ttbr0, ptr_t entry) __noreturn;

/** Kernel loader function. */
DEFINE_ELF_LOADER(load_elf_kernel, 32, LARGE_PAGE_SIZE);

/** Information on the loaded kernel. */
static Elf32_Addr kernel_entry;

/** Load a KBoot image into memory.
 * @param handle	Handle to image.
 * @param physp		Where to store physical address of kernel image.
 * @return		Created MMU context for kernel. */
mmu_context_t *kboot_arch_load(file_handle_t *handle, phys_ptr_t *physp) {
	mmu_context_t *ctx;

	if(!elf_check(handle, ELFCLASS32, ELF_EM_ARM)) {
		boot_error("Kernel image is not for this architecture");
	}

	/* Create the MMU context. */
	ctx = mmu_context_create();

	/* Load the kernel. */
	load_elf_kernel(handle, ctx, &kernel_entry, physp);
	dprintf("kboot: 32-bit kernel entry point is 0x%lx, TTBR0 is %p\n",
	        kernel_entry, ctx->ttbr0);

	/* Identity map the loader. */
	mmu_map(ctx, LOADER_LOAD_ADDR, LOADER_LOAD_ADDR, 0x100000);
	return ctx;
}

/** Enter a loaded KBoot kernel.
 * @param ctx		MMU context.
 * @param tags		Tag list address. */
void kboot_arch_enter(mmu_context_t *ctx, phys_ptr_t tags) {
	kboot_arch_enter_real(tags, ctx->ttbr0, kernel_entry);
}
