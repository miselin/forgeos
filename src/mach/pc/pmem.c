
#include <stdint.h>
#include <multiboot.h>
#include <system.h>
#include <pmem.h>
#include <io.h>

extern int end;

int mach_phys_init(struct multiboot_info *mbi) {
	struct multiboot_mmap *mmap = (struct multiboot_mmap *) mbi->mi_mmap_addr;
	size_t len = 0; int n = 0;
	uintptr_t kernel_end = log2phys((uintptr_t) &end);

	kprintf("pmem: memory map:\n");
	len = mbi->mi_mmap_length;
	while(len) {
		uintptr_t base = (uintptr_t) mmap->mm_base_addr;
		uintptr_t top = (uintptr_t) (mmap->mm_base_addr + mmap->mm_length);
		kprintf("    %x -> %x [%d]\n", base, top, mmap->mm_type);

		// Don't use kernel pages.
		if(top > kernel_end) {
			if(base < kernel_end)
				base = kernel_end;

			// Available? Use 'dealloc' to push onto the stack.
			if(mmap->mm_type == 1) {
				for(; base < top; base += PAGE_SIZE) {
					pmem_dealloc(base);
					n++;
				}
			}
		}

		len -= mmap->mm_size + sizeof(mmap->mm_size);
		mmap = (struct multiboot_mmap *)
					(((uintptr_t) mmap) + (mmap->mm_size + sizeof(mmap->mm_size)));
	}
	kprintf("pmem: %d pages ready for use - ~ %d MB\n", n, (n * 4096) / 0x100000);
	return 0;
}

int mach_phys_deinit() {
	return 0;
}
