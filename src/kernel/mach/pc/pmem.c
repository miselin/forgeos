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
#include <malloc.h>
#include <panic.h>
#include <util.h>
#include <pmem.h>
#include <io.h>

extern int end;

static paddr_t totalKiB = 0;

static void *firmware_stack = 0;

#define FIRMWARE_END        ((paddr_t) 0x100000)

paddr_t pmem_size() {
	return totalKiB;
}

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

	totalKiB = (n * PAGE_SIZE) / 1024;
    kprintf("pmem: %d pages ready for use - ~ %d MB\n", n, totalKiB / 1024);

    // Firmware stack for < 1 MB RAM (for things like ACPI and such)
    firmware_stack = create_stack();
    stack_flags(firmware_stack, STACK_FLAGS_NOMEMLOCK);

    // Find regions under 1 MB.
    n = 0;
    taglist = (kboot_tag_t *) tags;
    do {
        if(taglist->type == KBOOT_TAG_MEMORY) {
            kboot_tag_memory_t *memtag = (kboot_tag_memory_t *) taglist;

            if((memtag->type == KBOOT_MEMORY_FREE) && (((paddr_t) memtag->end) <= FIRMWARE_END)) {
                dprintf("firmware %llx -> %llx\n", memtag->start, memtag->end);

                // Don't overwrite the first page of memory.
                if(base == 0)
                    base += PAGE_SIZE;

                for(base = memtag->start; base < memtag->end; base += PAGE_SIZE) {
                    /// \todo HACK!
                    if(base == 0xB8000)
                        continue;

                    pmem_dealloc_special(PMEM_SPECIAL_FIRMWARE, base);
                    n++;
                }
            }
        }

        taglist = (kboot_tag_t *) taglist->next;
    } while(taglist);

    kprintf("pmem: %d pages < 1 MB ready for use - ~ %d KiB\n", n, (n * PAGE_SIZE) / 1024);

	return 0;
}

paddr_t pmem_alloc_special(size_t how) {
    struct phys_page *page = 0;
    paddr_t ret = 0;

    if(how == PMEM_SPECIAL_STANDARD) {
        return pmem_alloc();
    } else if(how == PMEM_SPECIAL_FIRMWARE) {
        if(firmware_stack == 0)
            return 0;

        page = (struct phys_page *) stack_pop(firmware_stack);
        if(!page)
            return 0;

        ret = page->addr;
        free_nolock(page);
    }

    return ret;
}

void pmem_dealloc_special(size_t how, paddr_t p) {
    struct phys_page *page = 0;

    if(how == PMEM_SPECIAL_STANDARD) {
        pmem_dealloc(p);
    } else if(how == PMEM_SPECIAL_FIRMWARE) {
        if(firmware_stack == 0)
            return;

        page = (struct phys_page *) malloc_nolock(sizeof(struct phys_page));
        page->addr = p;

        stack_push(firmware_stack, page);
    }
}

int mach_phys_deinit() {
	return 0;
}
