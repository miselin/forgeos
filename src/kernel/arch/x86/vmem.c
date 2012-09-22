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

#include <types.h>
#include <system.h>
#include <panic.h>
#include <vmem.h>
#include <pmem.h>
#include <util.h>
#include <io.h>
#include <powerman.h>

// #define VMEM_VERBOSE

#ifndef VMEM_VERBOSE
#undef dprintf
#define dprintf(a, ...)
#endif

struct gdt_entry {
	uint16_t	limit_low;
	uint16_t	base_low;
	uint8_t		base_mid;
	uint8_t		access;
	uint8_t		gran;
	uint8_t		base_high;
} __packed __aligned(4);
static struct gdt_entry gdt[16];

struct gdt_ptr {
	uint16_t	limit;
	uintptr_t	base;
} __packed __aligned(4);
static struct gdt_ptr gdtr;

#define FLAGS_PRESENT		0x01
#define FLAGS_WRITEABLE		0x02
#define FLAGS_USER			0x04
#define FLAGS_GLOBAL		0x100

#define PDIR_VIRT			0xFFFFF000
#define PDIR_BASE			0xFFC00000
#define PDIR_OFFSET(a)		(((a) >> 22) & 0x3FFUL)
#define PTAB_OFFSET(a)		(((a) >> 12) & 0x3FFUL)
#define PTAB_FROM_VADDR(a)	(PDIR_BASE + (PDIR_OFFSET(a) << 12))

// From start-x86.s
extern int init, init_end;
extern int tmpstack_base;

static paddr_t g_primedpage = 0;

void invlpg(char *p) {
	__asm__ volatile("invlpg %0" : : "m" (*p));
}

/// Translate input flags to x86 MMU flags.
size_t flags_to_x86(size_t f) {
	size_t flags = FLAGS_PRESENT;
	if(f & VMEM_READWRITE)
		flags |= FLAGS_WRITEABLE;
	if(f & VMEM_USERMODE)
		flags |= FLAGS_USER;
	if(f & VMEM_GLOBAL)
		flags |= FLAGS_GLOBAL;
	return flags;
}

void arch_vmem_prime(paddr_t p) {
    dprintf("vmem_prime: primed with %x\n", p);
    g_primedpage = p;
}

int arch_vmem_map(vaddr_t v, paddr_t p, size_t f) {
	uint32_t flags = flags_to_x86(f);
	if(p == (paddr_t) ~0) {
		p = g_primedpage;
		if(p == 0) {
		    p = pmem_alloc();
		} else
		    g_primedpage = 0;

		if(p == 0)
			panic("Out of memory.");
	} else if((p & 0xFFF) != 0) {
		p &= (paddr_t) ~0xFFF;
	}

	dprintf("vmem: map(%x -> %x)\n", v, p);

	// Load the page directory so we can figure out if a page table is present.
	uint32_t *pdir = (uint32_t *) PDIR_VIRT;

	// Is there an entry for the page table?
	vaddr_t entry = pdir[PDIR_OFFSET(v)] & (vaddr_t) ~0xFFF;
	uint32_t *ptab = (uint32_t *) PTAB_FROM_VADDR(v);
	if(entry == 0) {
		// No page table yet - allocate one.
		paddr_t ptab_phys = g_primedpage;
		if(ptab_phys == 0) {
		    ptab_phys = pmem_alloc();
		} else
		    g_primedpage = 0;
		if(ptab_phys == 0)
		    return -1;
		pdir[PDIR_OFFSET(v)] = ((vaddr_t) ptab_phys) | FLAGS_PRESENT | FLAGS_WRITEABLE; // Non-user, Present

		dprintf("vmem: allocated a new page table for %x at %x\n", v, ptab_phys);

	    // Invalidate the TLB cache for this page table
	    invlpg((char *) ptab);

		memset(ptab, 0, PAGE_SIZE - 1);
	}

	// Complete the mapping.
	ptab[PTAB_OFFSET(v)] = ((unative_t) (p & PADDR_MASK)) | flags;

	// Invalidate the TLB cache for this newly mapped page
	invlpg((char *) v);

	return 0;
}

void arch_vmem_unmap(vaddr_t v) {
	if(vmem_ismapped(v) == 0) {
		return;
	}

	dprintf("vmem: unmap(%x)\n", v);

	// Unmap the page by marking it not present
	uint32_t *ptab = (uint32_t *) PTAB_FROM_VADDR(v);
	ptab[PTAB_OFFSET(v)] &= (uint32_t) ~FLAGS_PRESENT;

	// Invalidate the TLB cache for this page
	invlpg((char *) v);
}

int arch_vmem_modify(vaddr_t v, size_t nf) {
	if(vmem_ismapped(v) == 0) {
		return -1;
	}

	// Unmap the page by marking it not present
	uint32_t *ptab = (uint32_t *) PTAB_FROM_VADDR(v);
	ptab[PTAB_OFFSET(v)] &= (uint32_t) ~0xFFF;
	ptab[PTAB_OFFSET(v)] |= flags_to_x86(nf);

	// Invalidate the TLB cache for this page
	invlpg((char *) v);

	return 0;
}

int arch_vmem_ismapped(vaddr_t v) {
	dprintf("vmem: is %x mapped?\n", v);

	// Load the page directory so we can figure out if a page table is present.
	uint32_t *pdir = (uint32_t *) PDIR_VIRT;

	// Is there an entry for the page table?
	vaddr_t entry = pdir[v >> 22] & (paddr_t) ~0xFFF;
	if(entry != 0) {
		// Complete the mapping.
		uint32_t *ptab = (uint32_t *) PTAB_FROM_VADDR(v);
		if((ptab[PTAB_OFFSET(v)] & FLAGS_PRESENT) != 0) {
			dprintf("vmem: %x is mapped\n", v);
			return 1;
		}
	}

	dprintf("vmem: %x is not mapped\n", v);

	return 0;
}

paddr_t arch_vmem_v2p(vaddr_t v) {
	dprintf("vmem: v2p %x\n", v);

	uint32_t *ptab = (uint32_t *) PTAB_FROM_VADDR(v);
	if(ptab[PTAB_OFFSET(v)] & FLAGS_PRESENT) {
		return (ptab[PTAB_OFFSET(v)] & (paddr_t) ~0xFFF) | (v & 0xFFF);
	}

	return 0;
}

vaddr_t arch_vmem_create() {
	return 0;
}

void arch_vmem_switch(vaddr_t pd __unused) {
}

void gdt_set(int n, uintptr_t base, uint32_t limit, uint8_t access, uint8_t gran) {
	if(n > 16)
		return;

	gdt[n].base_low = base & 0xFFFF;
	gdt[n].base_mid = (uint8_t) ((base >> 16) & 0xFF);
	gdt[n].base_high = (uint8_t) ((base >> 24) & 0xFF);

	gdt[n].limit_low = limit & 0xFFFF;
	gdt[n].gran = (uint8_t) (((limit >> 16) & 0x0F) | (gran & 0xF0));

	gdt[n].access = access;
}

static __noinline void reload_gdt() {
	__asm__ volatile("lgdt %0; \
					  jmp $0x08, $.flush;\n \
					  .flush:\n \
					  movw $0x10, %%ax; \
					  movw %%ax, %%ds; \
					  movw %%ax, %%es; \
					  movw %%ax, %%fs; \
					  movw %%ax, %%gs; \
					  movw %%ax, %%ss" :: "m" (gdtr) : "eax");
}

void arch_vmem_init() {
	// We can clear out the .init section, freeing some pages.
	uintptr_t init_start_phys = log2phys((uintptr_t) &init);
	uintptr_t c = 0, n = 0;
	for(c = (uintptr_t) &init; c < (uintptr_t) &init_end; c += PAGE_SIZE) {
		vmem_unmap(c);

		pmem_dealloc(init_start_phys);
		init_start_phys += 0x1000;
		n++;
	}

	kprintf("vmem: cleared %ld KB of RAM used by kernel init\n", (n * PAGE_SIZE) / 1024);

	// We can also relocate the stack.
	uintptr_t stack_phys = log2phys((uintptr_t) &tmpstack_base);
	uintptr_t stack_base = STACK_TOP - STACK_SIZE;
	uintptr_t stack_virt = stack_base;
	for(c = stack_phys; c < (stack_phys + STACK_SIZE); c += 0x1000, stack_virt += 0x1000) {
		vmem_map(stack_virt, c, VMEM_READWRITE);
	}

	// We need to copy the existing KBoot stack, and then we need to switch stacks.
	// Because I'm masochistic, I'm doing this in C.

	uint32_t esp = 0;
	__asm__ volatile("mov %%esp, %0" : "=r" (esp));

	dprintf("current stack is %x, moving to base %x\n", esp, stack_base);

	// Assume it's page-aligned, so we can figure out how much to copy.
	size_t stacksz = 0x1000 - (esp & 0xFFF);

	// Copy.
	memcpy((void *) (STACK_TOP - stacksz), (void *) esp, stacksz);

	// This is tricky. Update the stack pointer, live.
	uintptr_t ebp = 0;
	__asm__ volatile("mov %%ebp, %0" : "=r" (ebp));
	__asm__ volatile("mov %0, %%esp" :: "r" (STACK_TOP - stacksz) : "esp");

	// Just moved the stack, a barrier is definitely necessary.
	__barrier;

	// And now we can knock out the big page mapping the first 4 MB of RAM,
	// essentially completely unmapping the first 4 MB of RAM entirely.
	// By now, things like KBoot tags should be used and ACPI stuff should be
	// mapped in, or about to be mapped in.
	/// \todo move kboot tags to high memory.
	uint32_t *pdir = (uint32_t *) PDIR_VIRT;
	pdir[0] = 0;

	// Just completely flush the TLB.
	__asm__ volatile("mov %%cr3, %%eax; mov %%eax, %%cr3" ::: "eax", "memory");

	/// \todo massive hack
	vmem_map(0xB8000, 0xB8000, VMEM_SUPERVISOR | VMEM_GLOBAL | VMEM_READWRITE);

	dprintf("stack is now at %x ebp is %x\n", (STACK_TOP - stacksz), ebp);

	// Set up the GDT now.
	/// \todo Provide an API for adding new segments.

	// Null GDT entry
	gdt_set(0, 0, 0, 0, 0);

	// Kernel CS/DS (0x08/0x10)
	gdt_set(1, 0, 0xFFFFFFFF, 0x98, 0xCF);
	gdt_set(2, 0, 0xFFFFFFFF, 0x92, 0xCF);

	// User CS/DS (0x18/0x20)
	gdt_set(3, 0, 0xFFFFFFFF, 0xF8, 0xCF);
	gdt_set(4, 0, 0xFFFFFFFF, 0xF2, 0xCF);

	// Set up the GDTR
	gdtr.limit = (sizeof(struct gdt_entry) * 5) - 1;
	gdtr.base = (uintptr_t) gdt;

	// Make sure GCC doesn't attempt to reorder instructions here
	__barrier;

	// Flush the GDT
	reload_gdt();

	dprintf("gdtr limit: %x, base: %x\n", gdtr.limit, gdtr.base);
}

extern void *pc_acpi_gdt;

int vmem_powerstate_change(int new_state) {
	uint32_t *pdir = (uint32_t *) PDIR_VIRT;
	static uint32_t persist_pdir = 0;

	if(new_state == POWERMAN_STATE_WORKING) {
		// Handle return to working state by reloading GDTR.
		dprintf("pc: power state changed to working, reloading GDT\n");
		reload_gdt();

		// Restore the old 0 - 4 MB page table.
		pdir[0] = persist_pdir;
	} else if(new_state < POWERMAN_STATE_OFF) {
		// Copy our current kernel code/data segments so the wakeup code can
		// load a GDT with minimal effort.
		dprintf("pc: copying gdt for wakeup to %p\n", &pc_acpi_gdt);
		memcpy(&pc_acpi_gdt, gdt, sizeof(gdt[0]) * 3);

		// Map in the first 4 MB with a large page again.
		persist_pdir = pdir[0];
		pdir[0] = 0x83;
	}

	return 0;
}

void vmem_powerman_init() {
	powerman_installcallback(vmem_powerstate_change);
}
