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

struct node {
	struct node *next;
	void *p;
};

struct stack {
	struct node *head;
	uint32_t flags;
};

void *create_stack() {
	void *ret = malloc(sizeof(struct stack));
	struct stack *s = (struct stack *) ret;
	s->head = 0;
	s->flags = 0;
	return ret;
}

void stack_flags(void *p, uint32_t flags) {
	if(!p)
		return;
	struct stack *s = (struct stack *) p;
	s->flags = flags;
}

void delete_stack(void *p) {
	if(!p)
		return;

	struct stack *s = (struct stack *) p;
	struct node *n = s->head, *tmp;

	/// \note Will not clean up any data in the stack
	while(n) {
		tmp = n;
		n = n->next;

		if(s->flags & STACK_FLAGS_NOMEMLOCK)
			free_nolock(tmp);
		else
			free(tmp);
	}
}

void stack_push(void *stack, void *data) {
	if(!stack)
		return;

	struct stack *s = (struct stack *) stack;
	struct node *n = 0;
	if(s->flags & STACK_FLAGS_NOMEMLOCK)
		n = (struct node *) malloc_nolock(sizeof(struct node));
	else
		n = (struct node *) malloc(sizeof(struct node));
	n->p = data;

	// Replace the head of the stack with n, but only if our new node next
	// pointer is still the head of the stack.
	atomic_compare_and_swap(&s->head, n, n->next, n->next, n->next = s->head);
}

void *stack_pop(void *stack) {
	if(!stack)
		return 0;

	struct stack *s = (struct stack *) stack;
	struct node *top = s->head;

	// Remove the first item from the stack by updating the head to point to the
	// next item from the head. Atomically.
	atomic_compare_and_swap(&s->head, s->head->next, void * _a __unused, top, top = s->head);

	void *ret = top->p;

	if(s->flags & STACK_FLAGS_NOMEMLOCK)
		free_nolock(top);
	else
		free(top);

	return ret;
}
