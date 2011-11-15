
#ifndef _PMEM_H
#define _PMEM_H

#include <stdint.h>
#include <multiboot.h>

/// Initialise the physical memory allocator
#define pmem_init		mach_phys_init

/// De-initialise the physical memory allocator.
#define pmem_deinit		mach_phys_deinit

/// Allocate a single page from the physical allocator.
extern paddr_t	pmem_alloc();

/// Deallocate a single page, returning it to the physical allocator.
extern void		pmem_dealloc(paddr_t p);

/// Pin a particular physical page, making it impossible to allocate.
extern void		pmem_pin(paddr_t p);

// Machine-specific physical memory management
extern int		mach_phys_init(struct multiboot_info *mbi);
extern int		mach_phys_deinit();

#endif
