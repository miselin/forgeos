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

#include <io.h>
#include <test.h>
#include <string.h>
#include <assert.h>

size_t strlen(const char *s) {
	size_t ret = 0;
	if(s == 0)
		return ret;
	while(*s++) {
		ret++;
	}
	
	// Plenty of code will go ahead and treat our return value as a signed int.
	// If you find a 2 billion character long string, chances are using strlen
	// on it isn't exactly the brightest idea anyway.
	assert(((int) ret) >= 0);
	
	return ret;
}

char *strcpy(char *s1, const char *s2) {
    return strncpy(s1, s2, strlen(s2));
}

char *strncpy(char *s1, const char *s2, size_t max) {
    char *ret = s1;
    
    while(*s2 && max) {
        *s1 = *s2;
        max--; s1++; s2++;
    }
    
    *s1 = 0;
    
    return ret;
}

DEFINE_TEST(strlen_zero, ORDER_SECONDARY, 0, NOP, strlen(""))
DEFINE_TEST(strlen_hello, ORDER_SECONDARY, 5, NOP, strlen("Hello"))
DEFINE_TEST(strlen_midnull, ORDER_SECONDARY, 3, NOP, strlen("Hel\0lo"))
DEFINE_TEST(strlen_isnull, ORDER_SECONDARY, 0, NOP, strlen(0))
