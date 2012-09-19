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

#ifndef _STRING_H
#define _STRING_H

#include <types.h>

extern size_t strlen(const char *s);

extern char *strcpy(char *s1, const char *s2);
extern char *strncpy(char *s1, const char *s2, size_t max);

#define strcmp __builtin_strcmp
#define strncmp __builtin_strncmp

extern unsigned long int strtoul(char *str, char **end, int base);

extern char *strcat(char *a, char *b);

#define islower(c) (c >= 'a' && c <= 'z')
#define isupper(c) (c >= 'A' && c <= 'Z')
#define isalpha(c) (isupper(c) || islower(c))
#define isdigit(c) (c >= '0' && c <= '9')
#define isxdigit(c) (isdigit(c) || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F'))

#define isprint(c) (isdigit(c) || isalpha(c))
#define isspace(c) ((c == ' ') || (c == '\t') || (c == '\n') || \
                    (c == '\v') || (c == '\f') || (c == '\r'))

#define tolower(c) (isupper(c) ? ((c) - 'A') + 'a' : c)
#define toupper(c) (isupper(c) ? c : ((c) - 'a') + 'A')

/// Find 'c' in s, return a pointer to the first location found or NULL if not found.
extern const char *strsearch(const char *s, const char c);

#endif
