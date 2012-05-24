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

extern int init, end;

int mach_phys_init(phys_ptr_t tags) {
	unative_t kernel_start = (unative_t) &init;
	unative_t kernel_end = (unative_t) &end;
	
	size_t n = 0; unative_t base = 0;
	for(base = RAM_START; base < RAM_FINISH; base += 0x1000) {
	    if((base < kernel_end) && (base > kernel_start))
	        continue;
        pmem_dealloc(base);
        n++;
    }

    kprintf("pmem: %d pages ready for use - ~ %d MB\n", n, (n * 4096) / 0x100000);

	return 0;
}

int mach_phys_deinit() {
	return 0;
}

