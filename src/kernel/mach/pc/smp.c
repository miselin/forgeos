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

#define PERCPU_NUM_ENTRIES          (PAGE_SIZE / sizeof(unative_t))

struct percpu_data {
    unative_t data[PERCPU_NUM_ENTRIES];
};

struct percpu_data_handle {
    unative_t addr;
    size_t block;
    size_t bit;
};

static void *init_slock = 0;

extern void *pc_ap_entry;

static int ap_lowmem_init = 0;
static uint8_t ap_startup_vec = 0;

extern void *pc_ap_pdir;

#define WARM_RESET_VECTOR       0x0469

extern void idle(void *p __unused);

/// Called by an AP after it completes initial startup.
void ap_startup() {
    multicpu_cpuinit();

    idle(0);
}

void multicpu_cpuinit() {
    extern void vmem_multicpu_init();
    extern void ints_multicpu_init();

    // Configure per-CPU data storage.
    struct percpu_data *percpu = (struct percpu_data *) malloc(sizeof(struct percpu_data));
    memset(percpu, 0, sizeof(struct percpu_data));
    __asm__ volatile("mov %0, %%dr3" :: "r" ((unative_t) percpu));

    // Configure our Local APIC.
    init_lapic();

    // Switch to the correct GDT (now that paging is on and such).
    vmem_multicpu_init();

    // Enable interrupts for this CPU.
    ints_multicpu_init();

    // Initialisation complete - release the lock to let the system continue.
    dprintf("AP %d has started\n", multicpu_id());

    // Set up this CPU for scheduling (will not return).
    sched_cpualive(init_slock);
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

    // Set shutdown code to 'warm reset', install AP startup function in the
    // warm reset vector.
    cmos_write(0x0F, 0x0A);

    vmem_map((vaddr_t) 0, 0, VMEM_SUPERVISOR | VMEM_READWRITE);
    *((uint16_t *) (WARM_RESET_VECTOR)) = ap_startup_vec;
    *((uint16_t *) (WARM_RESET_VECTOR + 2)) = 0;
    vmem_unmap((vaddr_t) 0);

    // Setup per-CPU data for the BSP.
    struct percpu_data *percpu = (struct percpu_data *) malloc(sizeof(struct percpu_data));
    memset(percpu, 0, sizeof(struct percpu_data));
    __asm__ volatile("mov %0, %%dr3" :: "r" ((unative_t) percpu));

    return 0;
}

int start_processor(uint8_t id) NO_THREAD_SAFETY_ANALYSIS {
    dprintf("x86 AP startup - id %d\n", id);

    // Don't start APs if init hasn't been done yet.
    if(!ap_lowmem_init) {
        return -1;
    }
    assert(init_slock != NULL);

    // Prepare to start the AP.
    spinlock_acquire(init_slock);

    /// \todo This is very 'fire and forget' - would be nice to use the ICR
    ///       delivery status bit to verify IPIs have been received.

    // Init IPI
    lapic_ipi(id, ap_startup_vec, 5 /* INIT */, 1, 1);

    // 10 ms delay, without putting the thread to sleep.
    sleep_micro(10000);

    if(lapic_ver != LAPIC_VERSION_INTEGRATED) {
        // Startup IPI
        lapic_ipi(id, ap_startup_vec, 6 /* Startup */, 1, 0);

        // Sleep 200 microseconds.
        sleep_micro(200);

        // Startup IPI
        lapic_ipi(id, ap_startup_vec, 6 /* Startup */, 1, 0);

        // Sleep 200 microseconds.
        sleep_micro(200);
    }

    // Wait until the other processor has started.
    spinlock_acquire(init_slock);
    spinlock_release(init_slock);

    return 0;
}

void *multicpu_percpu_at(size_t n) {
    unative_t dr3 = 0;
    __asm__ volatile("mov %%dr3, %0" : "=r" (dr3));

    // Not set up yet?
    if(!dr3) {
        return 0;
    }

    if(n >= PERCPU_NUM_ENTRIES) {
        dprintf("percpu_alloc: index out of range\n");
        return 0;
    }

    struct percpu_data *data = (struct percpu_data *) dr3;
    return (void *) &data->data[n];
}
