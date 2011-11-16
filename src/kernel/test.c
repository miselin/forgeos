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

#ifdef _TESTING

#include <test.h>
#include <io.h>

extern int __begin_tests_0, __end_tests;

int perform_tests() {
	uintptr_t begin = (uintptr_t) &__begin_tests_0;
	uintptr_t end = (uintptr_t) &__end_tests;
	size_t testcount = (end - begin) / sizeof(struct __test), n = 0;

	kprintf("==== Performing Tests... ====\n");

	kprintf("Tests begin at %x, end at %x [%d tests]\n", begin, end, testcount);

	while(begin < end) {
		struct __test *test = (struct __test *) begin;
		kprintf("Test %d of %d: %s ", n++, testcount, test->name);
		if(!test->routine())
			kprintf("PASS\n");
		else {
			kprintf("FAIL\n");
			return -1;
		}
		begin += sizeof(struct __test);
	}

	return 0;
}

#endif
