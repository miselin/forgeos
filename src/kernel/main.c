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
#include <test.h>

KBOOT_IMAGE(0);

context_t *new1, *new2;

void ctx1() {
	while(1) {
		uint32_t esp = 0; __asm__ volatile("mov %%esp, %0" : "=r" (esp));
		dprintf("1 %x\n", esp);
		switch_context(&new1, new2);

		while(1) __asm__ volatile("hlt");
	}
}

void ctx2() {
	while(1) {
		uint32_t esp = 0; __asm__ volatile("mov %%esp, %0" : "=r" (esp));
		dprintf("2 %x\n", esp);
		switch_context(&new2, new1);

		while(1) __asm__ volatile("hlt");
	}
}

char stack1[32], stack2[32];

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

#ifdef _TESTING
	perform_tests();
#else
	kprintf("Enabling interrupts...\n");
	interrupts_enable();

	kprintf("Startup complete!\n");
#endif

	new1 = (context_t *) malloc(sizeof(context_t));
	new2 = (context_t *) malloc(sizeof(context_t));

	memset(new1, 0, sizeof(*new1));
	memset(new2, 0, sizeof(*new2));

	new1->esp = stack1 + 32;
	new2->esp = stack2 + 32;

	new1->eip = (uint32_t) ctx1;
	new2->eip = (uint32_t) ctx2;

	switch_context(0, new1);

	while(1) __asm__ volatile("hlt");
}
