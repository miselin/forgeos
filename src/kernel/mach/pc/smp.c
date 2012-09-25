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
#include <types.h>
#include <sched.h>
#include <pmem.h>
#include <vmem.h>
#include <io.h>

#include <apic.h>

extern void *pc_ap_entry;

static int ap_lowmem_init = 0;
static uint8_t ap_startup_vec = 0;

/// Called by an AP after it completes initial startup.
void ap_startup() {
    dprintf("AP has started up\n");
    while(1) __halt;
}

int start_processor(uint8_t id) {
    /// \todo Not really compatible with the Intel MP Spec.

    dprintf("x86 AP startup - id %d\n", id);

    // Relocate the AP entry point to low memory, if it hasn't already been moved.
    if(!ap_lowmem_init) {
        paddr_t p = pmem_alloc_special(PMEM_SPECIAL_FIRMWARE);
        vmem_map((vaddr_t) p, p, VMEM_SUPERVISOR | VMEM_READWRITE);
        memcpy((void *) p, (void *) &pc_ap_entry, PAGE_SIZE);

        // Copied - set the vector.
        ap_startup_vec = p >> 12;

        // Initialisation is done!
        ap_lowmem_init = 1;
    }

    // Init IPI
    lapic_ipi(id, ap_startup_vec, 5 /* INIT */, 1, 1);

    for(int z = 0; z < 0x10000; z++);

    // Startup IPI
    lapic_ipi(id, ap_startup_vec, 6 /* Startup */, 1, 0);

    return 0;
}
