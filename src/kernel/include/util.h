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

#define STACK_FLAGS_NOMEMLOCK       1

typedef int (*tree_comparer)(void *, void *);

#ifndef NO_BUILTIN_MEMFUNCS
#define memset __builtin_memset
#define memcpy __builtin_memcpy
#else
extern void memset(void *p, int c, size_t len);
extern void *memcpy(void *dst, void *src, size_t len);
#endif

extern int memcmp(void *a, void *b, size_t len);

extern void *create_stack();
extern void delete_stack(void *s);

extern void stack_flags(void *p, uint32_t flags);
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

extern void *create_tree();
extern void *create_tree_cmp(tree_comparer cmp);
extern void delete_tree(void *t);

extern void tree_insert(void *t, void *key, void *val);
extern void tree_delete(void *t, void *key);
extern void *tree_search(void *t, void *search_key);

extern void *tree_min(void *t);
extern void *tree_max(void *t);

extern int tree_iterator_bof(void *t, void *it);
extern int tree_iterator_eof(void *t, void *it);

extern void *tree_key(void *n);
extern void *tree_val(void *n);

extern void *tree_iterator(void *t);
extern void *tree_next(void *t, void *i);
extern void *tree_prev(void *t, void *i);
extern void tree_deliterator(void *t, void *i);

extern void *create_trie();
extern void delete_trie(void *t);

extern void trie_insert(void *t, const char *s, void *val);
extern void trie_delete(void *t, const char *s);
extern void *trie_search(void *t, const char *s);

#endif
