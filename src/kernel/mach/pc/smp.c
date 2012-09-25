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

#include <system.h>
#include <compiler.h>
#include <spinlock.h>
#include <multicpu.h>
#include <assert.h>
#include <types.h>
#include <sched.h>
#include <pmem.h>
#include <vmem.h>
#include <util.h>
#include <io.h>

#include <apic.h>

static void *init_slock = 0;

extern void *pc_ap_entry;

static int ap_lowmem_init = 0;
static uint8_t ap_startup_vec = 0;

extern void *pc_ap_pdir;

/// Called by an AP after it completes initial startup.
void ap_startup() {
    multicpu_cpuinit();
    while(1) __halt;
}

void multicpu_cpuinit() {
    extern void vmem_multicpu_init();
    extern void ints_multicpu_init();

    // Configure our Local APIC.
    init_lapic();

    // Switch to the correct GDT (now that paging is on and such).
    vmem_multicpu_init();

    // Enable interrupts for this CPU.
    ints_multicpu_init();

    // Initialisation complete - release the lock to let the system continue.
    dprintf("AP %d has started\n", multicpu_id());
    spinlock_release(init_slock);

    // Set up this CPU for scheduling (will not return).
    sched_cpualive();
}

int multicpu_init() {
    // Most init done by init_apic, but we do want our spinlock to be live.
    init_slock = create_spinlock();

    // Store the current page directory so APs can pick it up.
    uint32_t cr3 = 0;
    __asm__ volatile("mov %%cr3, %0" : "=r" (cr3));
    *((uint32_t *) &pc_ap_pdir) = cr3;

    // Map and copy the low memory region APs boot at.
    paddr_t p = pmem_alloc_special(PMEM_SPECIAL_FIRMWARE);
    vmem_map((vaddr_t) p, p, VMEM_SUPERVISOR | VMEM_READWRITE);
    memcpy((void *) p, (void *) &pc_ap_entry, PAGE_SIZE);

    // Copied - set the vector so APs are ready to go.
    ap_startup_vec = p >> 12;

    // Initialisation is done!
    ap_lowmem_init = 1;

    return 0;
}

int start_processor(uint8_t id) {
    /// \todo Not really compatible with the Intel MP Spec.

    dprintf("x86 AP startup - id %d\n", id);

    // Don't start APs if init hasn't been done yet.
    if(!ap_lowmem_init) {
        return -1;
    }
    assert(init_slock != NULL);

    // Prepare to start the AP.
    spinlock_acquire(init_slock);

    // Init IPI
    lapic_ipi(id, ap_startup_vec, 5 /* INIT */, 1, 1);

    for(int z = 0; z < 0x10000; z++);

    // Startup IPI
    lapic_ipi(id, ap_startup_vec, 6 /* Startup */, 1, 0);

    // Wait until the other processor has started.
    spinlock_acquire(init_slock);
    spinlock_release(init_slock);

    return 0;
}
