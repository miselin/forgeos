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
#include <kboot.h>

#include <io.h>
#include <pmem.h>
#include <vmem.h>
#include <devices.h>
#include <timer.h>
#include <interrupts.h>
#include <mmiopool.h>
#include <dlmalloc.h>
#include <powerman.h>
#include <multicpu.h>
#include <system.h>
#include <timer.h>
#include <assert.h>
#include <sched.h>
#include <pool.h>
#include <test.h>
#include <malloc.h>
#include <sleep.h>

extern void init_serial();
extern void _start();

KBOOT_IMAGE(0);

void idle(void *p __unused) {
    while(1) {
        // Always confirm interrupts are enabled before halting.
        // The halt should put the CPU into a low power state until an IRQ or
        // something comes through.
        if(!interrupts_get())
            interrupts_enable();
        __halt;

        // sched_yield won't reschedule unless there is a thread to switch to.
        sched_yield();
    }
}

void banner(void *p __unused) {
    char idlebuf[81];
    size_t n = 0;
    while(1) {
        interrupts_disable();
        sprintf(idlebuf, "%-79s", "");
        puts_at(idlebuf, 0, 24);

        sprintf(idlebuf, "FORGE: %d cpus [curr %d], mem %d/%d KiB used, heap ends at %x // %d", multicpu_count(), multicpu_id(), (uintptr_t) (pmem_size() - pmem_freek()), (uintptr_t) pmem_size(), dlmalloc_sbrk(0), n++);
        puts_at(idlebuf, 0, 24);
        interrupts_enable();

        sleep_ms(1000);
    }
}

void init2(void *p __unused) {
    if(multicpu_count() > 1) {
        kprintf("Starting %d additional processors...\n", multicpu_count() - 1);
        for(size_t i = 0; i < multicpu_count(); i++) {
            multicpu_start(i);
        }
    }

    dprintf("Starting the scheduler...\n");
    start_scheduler();

    kprintf("Initialising power management...\n");
    powerman_init();

    dprintf("FORGE initialisation complete.\n");
    kprintf("FORGE initialisation complete.\n");

    while(1) __halt;
}

void _kmain(uint32_t magic, phys_ptr_t tags) {
#ifdef X86 // KBoot is only enabled for X86 at the moment.
	assert(magic == KBOOT_MAGIC);
	_start(); // _start() sets up a page directory that isn't KBoot's - known state!
	clrscr();
#endif

#ifdef MACH_REQUIRES_EARLY_DEVINIT
    // Initialise the machine to a state where we can do MMU stuff.
    init_devices_early();

    // The machines that define this also require virtual memory to be configured
    // before it's used. On X86 in particular we actually use vmem_map before
    // vmem_init - that's just not reasonable for non-x86 (as we don't have a
    // pre-existing page directory from the loader!)
	kprintf("Initialising virtual memory...\n");
	vmem_init();
#endif

    // This will make sure there's about 4K of space for malloc to use until physical
    // memory management is available for proper virtual memory.
    kprintf("Initialising sbrk()...\n");
    dlmalloc_sbrk(0);

	kprintf("Initialising physical memory manager...\n");
	pmem_init(tags);

#ifdef MACH_REQUIRES_EARLY_DEVINIT
    // Start up the MMIO pool implementation.
    kprintf("Configuring MMIO pools...\n");
    init_mmiopool(MMIO_BASE, MMIO_LENGTH);

    // Virtual memory and physical memory management are now working fine.
    // Do secondary early init.
    init_devices_early2();
#else
	kprintf("Completing virtual memory initialisation...\n");
	vmem_init();

    // Start up the MMIO pool implementation.
    kprintf("Configuring MMIO pools...\n");
    init_mmiopool(MMIO_BASE, MMIO_LENGTH);
#endif

    // Early power management features can be enabled early (eg, ACPI on x86-pc
    // so we can find things like the HPET).
    kprintf("Initialising initial power management...\n");
    powerman_earlyinit();

    kprintf("Initialising malloc...\n");
    init_malloc();

	kprintf("Configuring software and hardware interrupts...\n");
	interrupts_init();

    kprintf("Initialising multi-CPU layer...\n");
    multicpu_init();

	kprintf("Initialising machine devices...\n");
	init_devices();

	kprintf("Initialising timers...\n");
	timers_init();

    kprintf("Configuring memory pools...\n");
    init_pool();

    kprintf("Initialising scheduler...\n");
    init_scheduler();

    kprintf("Finalizing virtual memory initialization...\n");
    vmem_final_init();

#ifdef _TESTING
	perform_tests();
	kprintf("Tests complete.\n");
	while(1) __halt;
#endif

    // Leave the bottom line of the screen for a memory and information display.
    scrextents(80, 24);

    kprintf("Startup complete.\n");

    struct process *initproc = create_process("init", 0);
    struct thread *init_thread = create_thread(initproc, THREAD_PRIORITY_HIGH, init2, 0, 0, 0);
    struct thread *idle_thread = create_thread(initproc, THREAD_PRIORITY_LOW, 0, 0, 0, 0);
    struct thread *banner_thread = create_thread(initproc, THREAD_PRIORITY_LOW, banner, 0, 0, 0);

    dprintf("banner thread is %x\n", banner_thread);

    sched_setidle(idle_thread);
    thread_wake(banner_thread);
    sched_kickstart();  // kick off the first thread

    // kickstart kicks into the idle thread, so we become that now.
    idle(0);
}

