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

KBOOT_IMAGE(0);

void idle() {
    kprintf("Idle thread created.\n");
    while(1) {
        kprintf("A");
        asm volatile("sti; hlt");
    }
}

void init2() {
    dprintf("Starting the scheduler...\n");
    start_scheduler();
    
    dprintf("Mattise initialisation complete.\n");
    while(1) {
        kprintf("B");
        asm volatile("sti; hlt");
    }
}

void _kmain(uint32_t magic, phys_ptr_t tags) {
	clrscr();

	assert(magic == KBOOT_MAGIC);

	// This will make sure there's about 4K of space for malloc to use until physical
	// memory management is available for proper virtual memory.
	kprintf("Initialising malloc()...\n");
	dlmalloc_sbrk(0);

	kprintf("Initialising physical memory manager...\n");
	pmem_init(tags);

	kprintf("Completing virtual memory initialisation...\n");
	vmem_init();

	kprintf("Configuring software and hardware interrupts...\n");
	interrupts_init();

	kprintf("Initialising machine devices...\n");
	init_devices();

	kprintf("Initialising timers...\n");
	timers_init();
    
    dprintf("Configuring memory pools...\n");
    init_pool();

    kprintf("Initialising scheduler...\n");
    init_scheduler();

#ifdef _TESTING
	perform_tests();
#else
	kprintf("Enabling interrupts...\n");
	interrupts_enable();
#endif
    
    kprintf("Startup complete.\n");
    
    struct process *initproc = create_process("init", PROCESS_PRIORITY_HIGH, 0);
    struct thread *init_thread = create_thread(initproc, init2, 0, 0);
    struct thread *idle_thread = create_thread(initproc, idle, 0, 0);
    
    thread_wake(idle_thread);
    switch_threads(0, init_thread);
    
    while(1)
        asm volatile("hlt");
}

