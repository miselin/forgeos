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

#ifndef _UTIL_H
#define _UTIL_H

#include <types.h>

extern void memset(void *p, char c, size_t len);
extern void *memcpy(void *dest, void *src, size_t len);

extern void *create_stack();
extern void delete_stack(void *s);

extern void stack_push(void *s, void *p);
extern void *stack_pop(void *s);

extern void *create_list();
extern void delete_list(void *p);

extern void list_insert(void *list, void *data, size_t index);
extern void *list_at(void *list, size_t index);
extern size_t list_len(void *list);
extern void list_remove(void *list, size_t index);

extern void *create_queue();
extern void delete_queue(void *queue);

extern void queue_push(void *queue, void *data);
extern int queue_empty(void *queue);
extern void *queue_pop(void *queue);

#endif
