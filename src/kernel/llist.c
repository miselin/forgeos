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

#include <compiler.h>
#include <malloc.h>
#include <util.h>
#include <io.h>

struct node {
	void *p;
	struct node *next, *prev;
};

struct llist {
	size_t len;
	struct node *head;
	struct node *tail;
};

void *create_list() {
	void *ret = malloc(sizeof(struct llist));
	struct llist *l = (struct llist *) ret;
	l->head = l->tail = 0;
	l->len = 0;
	return ret;
}

void delete_list(void *p) {
	if(!p)
		return;

	struct llist *s = (struct llist *) p;
	struct node *n = s->head, *tmp;

	/// \note Will not clean up any data in the list
	while(n) {
		tmp = n;
		n = n->next;
		free(tmp);
	}
}

void list_insert(void *list, void *data, size_t index) {
	if(!list)
		return;

	dprintf("list_insert %x: idx %d\n", list, index);

	struct llist *l = (struct llist *) list;
	struct node *n = (struct node *) malloc(sizeof(struct node)), *tmp;
	n->p = data;

	// No existing nodes, yet.
	if(l->len == 0) {
		l->head = l->tail = n;
	}

	// Special cases.
	else if(index == 0) {
		n->next = l->head;
		n->prev = 0;
		l->head = n;
	} else if(index >= (l->len - 1)) {
		l->tail->next = n;
		n->prev = l->tail;
		n->next = 0;
		l->tail = n;
	} else {
		// Some magic to only ever traverse half the list.
		size_t i = 0;
		int half = 0;
		if(index > (l->len / 2)) {
			half = 1;
			tmp = l->tail;
			i = l->len;
		} else {
			tmp = l->head;
		}

		while(i != index) {
			if(half) {
				tmp = tmp->prev;
				i--;
			} else {
				tmp = tmp->next;
				i++;
			}
		}

		n->prev = tmp;
		n->next = tmp->next;
		tmp->next = n;
	}

	if(l->tail == 0)
		l->tail = l->head;

	l->len++;
}

void *list_at(void *list, size_t index) {
	if(!list)
		return 0;

	struct llist *l = (struct llist *) list;
	struct node *ret;

	// Identifies the end of the list.
	if(index >= l->len)
		return 0;

	if(index == 0) {
		ret = l->head;
	} else if(index >= (l->len - 1)) {
		ret = l->tail;
	} else {
		size_t i = 0;
		ret = l->head;
		while(i < index) {
			ret = ret->next;
			i++;
		}
	}

	return ret->p;
}

size_t list_len(void *list) {
    if(!list)
        return 0;

    struct llist *l = (struct llist *) list;
    return l->len;
}

void list_remove(void *list, size_t index) {
	if(!list)
		return;

	struct llist *l = (struct llist *) list;
	struct node *n = 0;

	dprintf("list_remove %x: idx %d\n", list, index);

	if(index == 0) {
		n = l->head;
		l->head = l->head->next;
		l->head->prev = 0;
	} else if(index >= (l->len - 1)) {
		n = l->tail;
		l->tail = n->prev;
		l->tail->next = 0;
	} else {
		size_t i = 0;
		n = l->head;
		while(i < index) {
			n = n->next;
			i++;
		}

		n->prev->next = n->next;
		n->next->prev = n->prev;
	}

	free(n);

	l->len--;
}
