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

struct queue {
	struct node *head;
	struct node *tail;
};

void *create_queue() {
	void *ret = malloc(sizeof(struct queue));
	struct queue *q = (struct queue *) ret;
	q->head = q->tail = 0;
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
	}
}

void queue_push(void *queue, void *data) {
	if(!queue)
		return;

	struct queue *q = (struct queue *) queue;
	struct node *n = (struct node *) malloc(sizeof(struct node));
	n->p = data;
	n->prev = 0;

	n->next = q->head;
	if(q->head)
		q->head->prev = n;
	q->head = n;

	if(q->tail == 0)
		q->tail = q->head;
}

void *queue_pop(void *queue) {
	if(!queue)
		return 0;

	struct queue *q = (struct queue *) queue;
	struct node *n;

	if(!q->tail)
		return 0;

	n = q->tail;
	q->tail = n->prev;
	if(q->tail)
		q->tail->next = 0;
	if(n == q->head) { // Popped the front of the queue.
		q->head = q->tail = 0;
	}

	void *ret = n->p;
	free(n);

	return ret;
}

int queue_empty(void *queue) {
	if(!queue)
		return 0;

	struct queue *q = (struct queue *) queue;
    if(q->tail)
        return 0;
    else
        return 1;
}

