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
#include <dlmalloc.h>
#include <timer.h>
#include <assert.h>
#include <sched.h>
#include <pool.h>
#include <test.h>

extern void init_serial();
extern void _start();

KBOOT_IMAGE(0);

void idle() {
    dprintf("idle thread has started...\n");

    char idlebuf[81];
    while(1) {
        interrupts_disable();
        sprintf(idlebuf, "%-79s", "");
        puts_at(idlebuf, 0, 24);

        sprintf(idlebuf, "FORGE Operating System: mem %d/%d KiB used, heap ends at %x", (uintptr_t) (pmem_size() - pmem_freek()), (uintptr_t) pmem_size(), dlmalloc_sbrk(0));
        puts_at(idlebuf, 0, 24);
        interrupts_enable();
        __halt;
    }
}

void init2() {
    dprintf("Starting the scheduler...\n");
    start_scheduler();

    dprintf("FORGE initialisation complete.\n");
    while(1) {
        interrupts_enable();
        __halt;
    }
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
	kprintf("Initialising malloc()...\n");
	dlmalloc_sbrk(0);

	kprintf("Initialising physical memory manager...\n");
	pmem_init(tags);

#ifndef MACH_REQUIRES_EARLY_DEVINIT
	kprintf("Completing virtual memory initialisation...\n");
	vmem_init();
#endif

	kprintf("Configuring software and hardware interrupts...\n");
	interrupts_init();

	kprintf("Initialising machine devices...\n");
	init_devices();

	kprintf("Initialising timers...\n");
	timers_init();

    kprintf("Configuring memory pools...\n");
    init_pool();

    kprintf("Initialising scheduler...\n");
    init_scheduler();

#ifdef _TESTING
	perform_tests();
	kprintf("Tests complete.\n");
	while(1) __halt;
#endif

    // Leave the bottom line of the screen for a memory and information display.
    scrextents(80, 24);

    kprintf("Startup complete.\n");

    struct process *initproc = create_process("init", PROCESS_PRIORITY_HIGH, 0);
    struct thread *init_thread = create_thread(initproc, init2, 0, 0);
    struct thread *idle_thread = create_thread(initproc, idle, 0, 0);

    thread_wake(idle_thread);
    switch_threads(0, init_thread);

    while(1)
        __halt;
}

