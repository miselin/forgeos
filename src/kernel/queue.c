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

#define QUEUE_MAGIC		0xDEADBEEF

struct node {
	void *p;
	struct node *next;
};

struct queue {
	struct node *head;
	struct node *tail;
	struct node *base;
	size_t len __aligned(4);
};

void *create_queue() {
	void *ret = malloc(sizeof(struct queue));
	struct queue *q = (struct queue *) ret;

	struct node *base = (struct node *) malloc(sizeof(struct node));
	base->p = (void *) QUEUE_MAGIC;
	base->next = NULL;

	q->base = base;
	q->head = q->tail = base;

	q->len = 0;
	return ret;
}

void delete_queue(void *queue) {
	if(!queue)
		return;

	struct queue *q = (struct queue *) queue;
	struct node *n = q->head, *tmp;
	while(n) {
		tmp = n;
		n = n->next;
		free(tmp);

		if(tmp == q->base)
			q->base = NULL;
	}

	if(q->base != NULL)
		free(q->base);

	free(q);
}

void queue_push(void *queue, void *data) {
	if(!queue)
		return;

	struct queue *q = (struct queue *) queue;
	struct node *n = (struct node *) malloc(sizeof(struct node));
	n->p = data;
	n->next = 0;

	struct node *tail, *next;
	while(1) {
		tail = q->tail;
		next = tail->next;
		if(tail == q->tail) {
			if(next == NULL) {
				if(atomic_bool_compare_and_swap(&tail->next, next, n)) {
					break;
				}
			} else {
				(void) atomic_val_compare_and_swap(&q->tail, tail, next);
			}
		}
	}

	(void) atomic_val_compare_and_swap(&q->tail, tail, n);
	atomic_inc(q->len);
}

void *queue_pop(void *queue) {
	if(!queue)
		return 0;

	struct queue *q = (struct queue *) queue;

	void *ret = 0;
	struct node *head, *tail, *next;
	while(1) {
		head = q->head;
		tail = q->tail;
		next = head->next;

		if(head == q->head) {
			if(head == tail) {
				if(next == NULL) {
					return 0;
				} else {
					(void) atomic_val_compare_and_swap(&q->tail, tail, next);
				}
			} else {
				ret = next->p;
				if(atomic_bool_compare_and_swap(&q->head, head, next)) {
					break;
				}
			}
		}
	}

	free(head);

	atomic_dec(q->len);
	return ret;
}

int queue_empty(void *queue) {
	if(!queue)
		return 0;

	struct queue *q = (struct queue *) queue;
	if((q->head == q->tail) && (q->head->next == NULL)) {
		return 1;
	} else {
		return 0;
	}
}
