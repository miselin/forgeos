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

/*
 * This file is a bit... interesting. It's all the support for built-in
 * runtime testing, so there's some fun C magic in here.
 */

#define ORDER_PRIMARY		"0"
#define ORDER_SECONDARY		"1"
#define ORDER_TERTIARY		"2"

#ifdef _TESTING

#ifndef _TEST_H
#define _TEST_H

#include <compiler.h>

struct __test {
	int (*routine)();
	const char *name;
} PACKED;

#define TEST_INIT_VAR(type, name, ...) type name = __VA_ARGS__

/// Defines a new test to be performed. Name must be unique.
/// croutine can be any C code that can be on the LHS of operator ==
#define DEFINE_TEST(name, order, expected, init, ...) \
	int __test_##name() { \
		init; \
		if((__VA_ARGS__) == (expected)) \
			return 0; \
		else \
			return 1; \
	} \
	struct __test __test_dat_##name __section(".__test." order) __unused = {__test_##name, #name};

/// Perform all configured tests in the system.
extern int perform_tests();

#endif

#else

#define TEST_INIT_VAR(a, b, ...)
#define DEFINE_TEST(a, b, c, d, ...)

#endif
