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
#include <types.h>
#include <system.h>
#include <panic.h>
#include <vmem.h>
#include <pmem.h>
#include <util.h>
#include <io.h>

extern void arm_mach_uart_remap();

static paddr_t g_primedpage = 0;

extern int __end;

#define FIRSTLEVEL_FAULT    0
#define FIRSTLEVEL_PAGETAB  1
#define FIRSTLEVEL_SECTION  2
#define FIRSTLEVEL_RSVD     3

#define SECLEVEL_FAULT      0
#define SECLEVEL_LARGE      1
#define SECLEVEL_SMALL      2
#define SECLEVEL_SMALLNX    3

/** Section B3.3 in the ARM Architecture Reference Manual (ARMv7) */

/// First level descriptor - roughly equivalent to a page directory entry on x86
struct first_level
{
    /// Type field for descriptors
    /// 0 = fault
    /// 1 = page table
    /// 2 = section or supersection
    /// 3 = reserved

    union {
        struct {
            uint32_t type : 2;
            uint32_t ignore : 30;
        } __packed fault;
        struct {
            uint32_t type : 2;
            uint32_t sbz1 : 1;
            uint32_t ns : 1;
            uint32_t sbz2 : 1;
            uint32_t domain : 4;
            uint32_t imp : 1;
            uint32_t baseaddr : 22;
        } __packed pageTable;
        struct {
            uint32_t type : 2;
            uint32_t b : 1;
            uint32_t c : 1;
            uint32_t xn : 1;
            uint32_t domain : 4; /// extended base address for supersection
            uint32_t imp : 1;
            uint32_t ap1 : 2;
            uint32_t tex : 3;
            uint32_t ap2 : 1;
            uint32_t s : 1;
            uint32_t nG : 1;
            uint32_t sectiontype : 1; /// = 0 for section, 1 for supersection
            uint32_t ns : 1;
            uint32_t base : 12;
        } __packed section;

        uint32_t entry;
    } descriptor;
} __packed;

/// Second level descriptor - roughly equivalent to a page table entry on x86
struct second_level
{
    /// Type field for descriptors
    /// 0 = fault
    /// 1 = large page
    /// >2 = small page (NX at bit 0)

    union
    {
        struct {
            uint32_t type : 2;
            uint32_t ignore : 30;
        } __packed fault;
        struct {
            uint32_t type : 2;
            uint32_t b : 1;
            uint32_t c : 1;
            uint32_t ap1 : 2;
            uint32_t sbz : 3;
            uint32_t ap2 : 1;
            uint32_t s : 1;
            uint32_t nG : 1;
            uint32_t tex : 3;
            uint32_t xn : 1;
            uint32_t base : 16;
        } __packed largepage;
        struct {
            uint32_t type : 2;
            uint32_t b : 1;
            uint32_t c : 1;
            uint32_t ap1 : 2;
            uint32_t sbz : 3;
            uint32_t ap2 : 1;
            uint32_t s : 1;
            uint32_t nG : 1;
            uint32_t base : 20;
        } __packed smallpage;

        uint32_t entry;
    } descriptor;
} __packed;

static void tlb_invalall() {
    // Completely invalidate the TLBs and the branch target cache.
    const uint32_t ignore = 0;
    asm volatile("MCR p15, 0, %0, c8, c7, 0" :: "r" (ignore));
    asm volatile("MCR p15, 0, %0, c7, c5, 6" :: "r" (ignore));

    __barrier;
}

static void tlb_invalpage(vaddr_t addr) {
    const uint32_t ignore = 0;
    asm volatile("MCR p15, 0, %0, c8, c7, 1" :: "r" (addr));
    asm volatile("MCR p15, 0, %0, c7, c5, 6" :: "r" (ignore));

    __barrier;
}

void arch_vmem_prime(paddr_t p) {
    g_primedpage = p;
}

int arch_vmem_map(vaddr_t v, paddr_t p, size_t f) {
    // Determine which range of page tables to use
    unative_t page_tables = 0;
    if(v < 0x40000000)
        page_tables = USER_PAGETABS_VIRT;
    else
        page_tables = PAGETABS_VIRT;

    if(p == (paddr_t) ~0) {
        p = pmem_alloc();
        if(p == 0) {
            p = g_primedpage;
            g_primedpage = 0;
        }
        if(p == 0)
            panic("Out of memory.");
    } else if((p & 0xFFF) != 0) {
        p &= (paddr_t) ~0xFFF;
    }

    unative_t vaddr = v;
    unative_t paddr = p;

    uint32_t pdir_offset = vaddr >> 20;
    uint32_t ptab_offset = (vaddr >> 12) & 0xFF;

    struct first_level *pdir = (struct first_level *) PAGEDIR_VIRT;
    if(!pdir[pdir_offset].descriptor.fault.type) {
        // Allocate page table.
        /// \todo implement me!
        panic("unimplemented - map page table that doesn't exist yet");
    }

    struct second_level *ptab = (struct second_level *) (page_tables + (pdir_offset * 0x400));
    if(ptab[ptab_offset].descriptor.fault.type) {
        return -1; // Already mapped!
    } else {
        ptab[ptab_offset].descriptor.entry = paddr;
        ptab[ptab_offset].descriptor.smallpage.type = 2;
        ptab[ptab_offset].descriptor.smallpage.ap1 = 3; /// \todo use flags parameter instead
        ptab[ptab_offset].descriptor.smallpage.nG = 1;
    }

    // Invalidate the TLB entry for this virtual address.
    tlb_invalpage(vaddr);

    return 0;
}

void arch_vmem_unmap(vaddr_t v) {
    // Determine which range of page tables to use
    unative_t page_tables = 0;
    if(v < 0x40000000)
        page_tables = USER_PAGETABS_VIRT;
    else
        page_tables = PAGETABS_VIRT;

    unative_t vaddr = v;

    uint32_t pdir_offset = vaddr >> 20;
    uint32_t ptab_offset = (vaddr >> 12) & 0xFF;

    /// \todo Handle sections/supersections.
    /// \todo Handle large pages.

    struct first_level *pdir = (struct first_level *) PAGEDIR_VIRT;
    if(!pdir[pdir_offset].descriptor.fault.type) {
        // No page table - ignore.
        return;
    }

    struct second_level *ptab = (struct second_level *) (page_tables + (pdir_offset * 0x400));
    if(ptab[ptab_offset].descriptor.fault.type) {
        ptab[ptab_offset].descriptor.fault.type = SECLEVEL_FAULT;
    }

    // Invalidate the TLB entry for this virtual address now that it is unmapped
    tlb_invalpage(vaddr);

    return 0;
}

int arch_vmem_modify(vaddr_t v, size_t nf) {
	return -1;
}

int arch_vmem_ismapped(vaddr_t v) {
    // Same operation as for v2p, except only care about the status bits.
    unative_t pa = 0;
    asm volatile("MCR p15, 0, %0, c7, c8, 3" :: "r" (v));
    asm volatile("MRC p15, 0, %0, c7, c4, 0" : "=r" (pa));

    // Failure?
    return (pa & 0x1) ? 0 : 1;
}

paddr_t arch_vmem_v2p(vaddr_t v) {
    // Lookup the virtual address, return physical address.
    unative_t pa = 0;
    asm volatile("MCR p15, 0, %0, c7, c8, 3" :: "r" (v));
    asm volatile("MRC p15, 0, %0, c7, c4, 0" : "=r" (pa));

    // Failure?
    if(pa & 0x1) {
        dprintf("v2p failed for %x\n", v);
        return (paddr_t) ~0;
    }

    // Return the physical address.
    return (paddr_t) (pa & ~0xFFF);
}

vaddr_t arch_vmem_create() {
	return 0;
}

void arch_vmem_switch(vaddr_t pd) {
}

void arch_vmem_init() {
    int i;
    dprintf("armv7: vmem init\n");

    // Disable the MMU before we start creating mappings and modifying physical
    // memory. We can't trust that the bootloader has setup sensible mappings.
    uint32_t sctlr = 0;
    asm volatile("MRC p15,0,%0,c1,c0,0" : "=r" (sctlr));
    sctlr &= ~1;
    asm volatile("MCR p15,0,%0,c1,c0,0" :: "r" (sctlr));

    // Clear out the TLB now that we have disabled the MMU.
    tlb_invalall();

    /// \note This function is all about setting up a kernel address space in
    ///       which we can create mappings and such. As such there's a fair bit
    ///       involved and it is a bit unwieldy. Half the problem is that I've
    ///       decided to pre-allocate MMU structures for the kernel address
    ///       space, so we initialise them HERE instead of in vmem_map. -Matt

    // Zero the page directory and all page tables.
    memset((void *) PAGEDIR_PHYS, 0, 0x4000);
    memset((void *) PAGETABS_PHYS, 0, 0x400000);

    // We want to map in the page directory to the address space, to make
    // modifying it easier.
    unative_t page_table_addr = PAGETABS_PHYS + (0x400000 - 0x400);
    unative_t paddr = 0;
    uintptr_t vaddr = PAGEDIR_VIRT;
    uint32_t pdir_offset, ptab_offset;
    size_t offset;

    // Map the page directory.
    struct first_level *pdir = (struct first_level *) PAGEDIR_PHYS;
    struct second_level *ptab = (struct second_level *) page_table_addr;
    pdir_offset = vaddr >> 20;

    pdir[pdir_offset].descriptor.entry = page_table_addr;
    pdir[pdir_offset].descriptor.pageTable.type = 1;
    pdir[pdir_offset].descriptor.pageTable.domain = 1; // Domain 1 for paging structures.

    // Page directories are four pages long!
    for(i = 0; i < 4; i++) {
        ptab_offset = ((vaddr + (i * 0x1000)) >> 12) & 0xFF;
        ptab[ptab_offset].descriptor.entry = PAGEDIR_PHYS + (i * 0x1000);
        ptab[ptab_offset].descriptor.smallpage.type = 2;
        ptab[ptab_offset].descriptor.smallpage.ap1 = 3; /// \todo Flags should be configured!
        ptab[ptab_offset].descriptor.smallpage.s = 1; // Shareable.
    }

    // Identity-map the kernel.
    /// \todo Relocate to 0xC0000000 somehow!

    dprintf("armv7: identity mapping the kernel\n");

    size_t kernelSize = ((uintptr_t) &__end) - RAM_START;
    for(offset = 0; offset < kernelSize; offset += 0x100000) {
        vaddr = RAM_START + offset;
        pdir_offset = vaddr >> 20;

        pdir[pdir_offset].descriptor.entry = vaddr;
        pdir[pdir_offset].descriptor.section.type = 2;
        pdir[pdir_offset].descriptor.section.domain = 2; // Domain 2 is the kernel.
        pdir[pdir_offset].descriptor.section.ap1 = 3;
        pdir[pdir_offset].descriptor.section.s = 1;
    }

    // Pre-allocate remaining page tables.
    dprintf("armv7: pre-allocating page tables\n");

    for(offset = 0; offset < 0x400000; offset += 0x100000) {
        vaddr = PAGETABS_VIRT + offset;
        paddr = PAGETABS_PHYS + offset;
        pdir_offset = vaddr >> 20;

        pdir[pdir_offset].descriptor.entry = paddr;
        pdir[pdir_offset].descriptor.section.type = 2;
        pdir[pdir_offset].descriptor.section.domain = 1;
        pdir[pdir_offset].descriptor.section.ap1 = 3;
        pdir[pdir_offset].descriptor.section.s = 1;

        // 1024 page tables in this region...
        for(i = 0; i < 1024; i++, paddr += 0x400) {
            vaddr = (offset << 10) + (i * 0x100000);
            pdir_offset = vaddr >> 20;

            // Existing mappings shouldn't be overwritten.
            if(pdir[pdir_offset].descriptor.entry)
                continue;
            pdir[pdir_offset].descriptor.entry = paddr;
            pdir[pdir_offset].descriptor.pageTable.type = 1;
            pdir[pdir_offset].descriptor.pageTable.domain = 1;
        }
    }

    dprintf("armv7: enabling mmu\n");

    __barrier;

    // Write TTBR0 with null - user part of address space split
    asm volatile("MCR p15,0,%0,c2,c0,0" :: "r" (0));

    // Write TTBR1 with the kernel page directory
    asm volatile("MCR p15,0,%0,c2,c0,1" :: "r" (PAGEDIR_PHYS));

    // Write TTBCR to configure a 4K directory, and a 1/3 GB split of the address
    // space - user/kernel.
    asm volatile("MCR p15,0,%0,c2,c0,2" :: "r" (2));

    // Give manager access to all domains
    /// \todo temporary.
    asm volatile("MCR p15,0,%0,c3,c0,0" :: "r" (0xFFFFFFFF));

    // Enable the MMU.
    sctlr = 0;
    asm volatile("MRC p15,0,%0,c1,c0,0" : "=r" (sctlr));
    sctlr |= 1;
    asm volatile("MCR p15,0,%0,c1,c0,0" :: "r" (sctlr));

    // The UART code refers to the actual physical addresses of the UARTs...
    // The machine implementation exposes this function so we can notify it that
    // vmem_map is callable.
    arm_mach_uart_remap();

    dprintf("armv7: mmu enabled\n");
}

