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

#include <types.h>
#include <kboot.h>
#include <system.h>
#include <panic.h>
#include <pmem.h>
#include <io.h>

extern int end;

int mach_phys_init(phys_ptr_t tags) {
	paddr_t kernel_end = log2phys((uintptr_t) &end);
	size_t n = 0; phys_ptr_t base = 0;
	
    kboot_tag_t *taglist = (kboot_tag_t *) tags;
    int found = 0;
    do {
        if(taglist->type == KBOOT_TAG_MEMORY) {
            kboot_tag_memory_t *memtag = (kboot_tag_memory_t *) taglist;
            
            if((memtag->type == KBOOT_MEMORY_FREE) && (((paddr_t) memtag->end) > kernel_end)) {
                base = memtag->start;
                if(base < kernel_end)
                    base = kernel_end;
                
                for(; base < memtag->end; base += PAGE_SIZE) {
                    pmem_dealloc(base);
                    n++;
                }
            }
            
            found = 1;
        }
        taglist = (kboot_tag_t *) taglist->next;
    } while(taglist);
    
    if(!found)
        panic("No memory map has been provided, cannot continue.");

    kprintf("pmem: %d pages ready for use - ~ %d MB\n", n, (n * 4096) / 0x100000);

	return 0;
}

int mach_phys_deinit() {
	return 0;
}
