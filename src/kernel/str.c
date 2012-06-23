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
    assert(s1 != NULL && s2 != NULL);
    return strncpy(s1, s2, strlen(s2));
}

char *strncpy(char *s1, const char *s2, size_t max) {
    assert(s1 != NULL && s2 != NULL);
    char *ret = s1;

    while(*s2 && max) {
        *s1 = *s2;
        max--; s1++; s2++;
    }

    *s1 = 0;

    return ret;
}

// Unlink from __builtin_str(n)cmp
#undef strcmp
#undef strncmp

int strcmp(const char *s1, const char *s2) {
    assert(s1 != NULL && s2 != NULL);
	// do/while means one string being empty returns the correct result
	do {
		if(*s1 < *s2)
			return -1;
		else if(*s1 > *s2)
			return 1;
	} while(*s1++ && *s2++);

	// catchall.
	return 0;
}

int strncmp(const char *s1, const char *s2, size_t n) {
    assert(s1 != NULL && s2 != NULL);
	// do/while means one string being empty returns the correct result
	do {
		if(*s1 < *s2)
			return -1;
		else if(*s1 > *s2)
			return 1;
	} while(*s1++ && *s2++ && --n);

	// catchall.
	return 0;
}

const char *strsearch(const char *s, const char c) {
    int found = 0;
    while(*s != 0) {
        if(*s == c) {
            found = 1;
            break;
        }

        s++;
    }

    if(found)
        return s;
    else
        return NULL;
}

DEFINE_TEST(strlen_zero, ORDER_SECONDARY, 0, NOP, strlen(""))
DEFINE_TEST(strlen_hello, ORDER_SECONDARY, 5, NOP, strlen("Hello"))
DEFINE_TEST(strlen_midnull, ORDER_SECONDARY, 3, NOP, strlen("Hel\0lo"))
DEFINE_TEST(strlen_isnull, ORDER_SECONDARY, 0, NOP, strlen(0))

DEFINE_TEST(strcmp_empty, ORDER_SECONDARY, 0, NOP, strcmp("", ""))
DEFINE_TEST(strcmp_nomatch, ORDER_SECONDARY, -1, NOP, strcmp("hello", "world"))
DEFINE_TEST(strcmp_match, ORDER_SECONDARY, 0, NOP, strcmp("hello", "hello"))
DEFINE_TEST(strcmp_posret, ORDER_SECONDARY, 1, NOP, strcmp("abc", "a"))
DEFINE_TEST(strcmp_negret, ORDER_SECONDARY, -1, NOP, strcmp("a", "abc"))

DEFINE_TEST(strncmp_empty, ORDER_SECONDARY, 0, NOP, strncmp("", "", 10))
DEFINE_TEST(strncmp_submatch, ORDER_SECONDARY, 0, NOP, strncmp("hello", "hellish", 3))
DEFINE_TEST(strncmp_subnotmatch, ORDER_SECONDARY, 1, NOP, strncmp("hello", "hellish", 6))
DEFINE_TEST(strncmp_posret, ORDER_SECONDARY, 1, NOP, strncmp("abc", "a", 2))

DEFINE_TEST(search_found, ORDER_SECONDARY, "abc" + 2, NOP, strsearch("abc", 'c'))
DEFINE_TEST(search_notfound, ORDER_SECONDARY, 0, NOP, strsearch("abc", 'd'))
