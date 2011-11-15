
#include <io.h>
#include <pmem.h>
#include <vmem.h>
#include <devices.h>
#include <interrupts.h>
#include <multiboot.h>
#include <dlmalloc.h>
#include <assert.h>

void _kmain(struct multiboot_info *mboot) {
	clrscr();

	// This will make sure there's about 4K of space for malloc to use until physical
	// memory management is available for proper virtual memory.
	kprintf("Initialising malloc()...\n");
	dlmalloc_sbrk(0);

	kprintf("Initialising physical memory manager...\n");
	pmem_init(mboot);

	kprintf("Completing virtual memory initialisation...\n");
	vmem_init();

	kprintf("Configuring software and hardware interrupts...\n");
	interrupts_init();

	kprintf("Initialising machine devices...\n");
	init_devices();

	kprintf("Enabling interrupts...\n");
	interrupts_enable();

	kprintf("Startup complete!\n");

	while(1) __asm__ volatile("hlt");
}
