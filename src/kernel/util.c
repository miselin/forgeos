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
#include <test.h>

void memset(void *p, char c, size_t len)
{
	char *s = (char *) p;
	size_t i = 0;
	for(; i < len; i++)
		s[i] = c;
}

#ifdef memcpy
#undef memcpy
#endif

void *memcpy(void *dest, void *src, size_t len) {
	char *s1 = (char *) src, *s2 = (char *) dest;
	while(len--) *s2++ = *s1++;
	return dest;
}

/*
 * Tests for the above functions. Using the ',' operator and macros to
 * create some truly convoluted things here. Note that none of this exists
 * in the output file unless _TESTING is defined.
 */

DEFINE_TEST(memset_std, ORDER_PRIMARY, 1,
			TEST_INIT_VAR(char, buf[4], {1, 2, 3, 4}),
			memset(buf, 0, 4),
			(buf[0] == 0 && buf[1] == 0 &&
			 buf[2] == 0 && buf[3] == 0) ? 1 : 0)

DEFINE_TEST(memset_nonzero, ORDER_PRIMARY, 1,
			TEST_INIT_VAR(char, buf[4], {1, 2, 3, 4}),
			memset(buf, 3, 4),
			(buf[0] == 3 && buf[1] == 3 &&
			 buf[2] == 3 && buf[3] == 3) ? 1 : 0)

DEFINE_TEST(memset_empty, ORDER_PRIMARY, 1,
			TEST_INIT_VAR(char, buf[4], {1, 2, 3, 4}),
			memset(buf, 0, 0),
			(buf[0] == 1 && buf[1] == 2 &&
			 buf[2] == 3 && buf[3] == 4) ? 1 : 0)
