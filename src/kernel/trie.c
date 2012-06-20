/*
 * Copyright (c) 2012 Matthew Iselin, Rich Edelman
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

#include <types.h>
#include <util.h>
#include <string.h>
#include <assert.h>
#include <malloc.h>
#include <test.h>

/// An empty string - used to define the root of the trie.
#define EMPTY_STRING		""

struct node {
	/// Prefix of this particular node in the tree. (eg, 't', 'tr', 'tre', 'tree')
	char *prefix;

	/// Length of the prefix (to avoid a strlen every time the node is passed).
	size_t prefixlen;

	/**
	 * Does this prefix have a value? Some prefixes are part of a lookup, and do not
	 * have a value.
	 */
	int isvalue;

	/// Value assigned to the prefix, if there is one.
	void *value;

	/// Number of child nodes to this particular node.
	size_t numchildren;

	/// Array of child nodes.
	struct node **children;

	/// Parent node.
	struct node *parent;
};

struct trie {
	struct node *root;
};

void *create_trie() {
	struct trie *t = (struct trie *) malloc(sizeof(struct trie));
	memset(t, 0, sizeof(struct trie));

	t->root = (struct node *) malloc(sizeof(struct node));
	memset(t->root, 0, sizeof(struct node));

	t->root->prefix = (char *) malloc(1);
	t->root->prefixlen = 0;
	strcpy(t->root->prefix, EMPTY_STRING);

	return (void *) t;
}

void delete_trie(void *t) {
	/// \todo implement me! :)
}

static void add_child(struct node *n, struct node *c) {
	n->numchildren++;
	struct node **newchildren = 0;
	if(n->numchildren == 1)
		newchildren = malloc(n->numchildren * sizeof(struct node *));
	else
		newchildren = realloc(n->children, n->numchildren * sizeof(struct node *));
	assert(newchildren != 0);

	newchildren[n->numchildren - 1] = c;
	n->children = newchildren;

	c->parent = n;
}

static size_t commonprefix(const char *s1, const char *s2) {
	size_t n = 0;
	while((*s1 == *s2) && (*s1 && *s2)) {
		n++;
		s1++;
		s2++;
	}

	return n;
}

void trie_insert(void *t, const char *s, void *val) {
	if(!t)
		return;

	struct trie *meta = (struct trie *) t;
	struct node *n = meta->root, *child = 0;

	// Search for the prefix, see if we can insert.
	size_t i = 0;
	int found = 0, common = 0;
	while(1) {
		for(; i < n->numchildren; i++) {
			child = n->children[i];

			// Exact match.
			if(strcmp(child->prefix, s) == 0) {
				found = 1;
				break;
			}

			// No exact match - can we find a common prefix to keep iterating with?
			else if(commonprefix(child->prefix, s)) {
				n = child;
				child = 0;
				common = 1;
				break;
			}

			// No common prefix - keep searching.
			else
				child = 0;
		}

		// A match was found!
		if(found)
			break;

		// No common prefixes found here - have to insert the full prefix.
		if(!common)
			break;

		common = 0;
	}

	// If the entire prefix was found, we can simply return - the value is set.
	if(found) {
		assert(child != 0);
		child->isvalue = 1;
		child->value = val;
		return;
	}

	// Key doesn't already exist - add it.
	if(!child) {
		child = (struct node *) malloc(sizeof(struct node));

		memset(child, 0, sizeof(struct node));

		child->prefixlen = strlen(s);
		child->prefix = (char *) malloc(child->prefixlen + 1);
		strcpy(child->prefix, s);

		child->isvalue = 1;
		child->value = val;
	}

	// Okay, now that we've linked this in as a child, do we need to split the parent
	// prefix in order to make this work?
	size_t commonlen = 0;
	if((commonlen = commonprefix(n->prefix, child->prefix)) < n->prefixlen) {
		// Okay, so we need to create a new node with the smaller prefix, with
		// 'n' and 'child' as its children.
		struct node *parent = (struct node *) malloc(sizeof(struct node));
		memset(parent, 0, sizeof(struct node));

		parent->prefixlen = commonlen;
		parent->prefix = (char *) malloc(commonlen + 1);
		strncpy(parent->prefix, n->prefix, parent->prefixlen);

		// Now, the child in the parent needs to be replaced.
		for(i = 0; i < n->parent->numchildren; i++) {
			if(n->parent->children[i] == n) {
				n->parent->children[i] = parent;
				parent->parent = n->parent;
			}
		}

		// And now we can link in the children of this node.
		add_child(parent, n);
		add_child(parent, child);
	} else {
		add_child(n, child);
	}
}

void trie_delete(void *t, const char *s) {
	/// \todo implement me! :)
}

void *trie_search(void *t, const char *s) {
	if(!t)
		return 0;

	struct trie *meta = (struct trie *) t;
	struct node *n = meta->root, *child = 0;

	if(!strcmp(s, ""))
		return n->value;

	int found = 0;
	size_t i = 0;
	while(!found) {
		child = 0;

		// No children - fail!
		if(!n->numchildren)
			break;

		// Check each child for the right prefix.
		for(i = 0; i < n->numchildren; i++) {
			child = n->children[i];
			if(strcmp(s, child->prefix) == 0) {
				// Direct match!
				found = 1;
				break;
			} else if(strncmp(s, child->prefix, child->prefixlen) == 0)
				break;
		}

		// Prefix not found?
		if(i >= n->numchildren) {
			child = 0;
			break;
		}

		// Iterate to the next child.
		n = child;
	}

	if(child)
		return child->value;
	else
		return 0;
}

DEFINE_TEST(trie_empty, ORDER_SECONDARY, 0, void *t = create_trie(), trie_search(t, "nothing"))
DEFINE_TEST(trie_single, ORDER_SECONDARY, (void *) 0xbeef, void *t = create_trie(), trie_insert(t, "hello", (void*) 0xbeef), trie_search(t, "hello"))
DEFINE_TEST(trie_deep, ORDER_SECONDARY, (void *) 0xbeef, void *t = create_trie(),
			trie_insert(t, "h", (void*) 0),
			trie_insert(t, "he", (void*) 0),
			trie_insert(t, "hel", (void*) 0),
			trie_insert(t, "hell", (void*) 0),
			trie_insert(t, "hello", (void*) 0),
			trie_insert(t, "helloworld", (void*) 0xbeef),
			trie_search(t, "helloworld"))
DEFINE_TEST(trie_variety, ORDER_SECONDARY, (void *) 0xbeef, void *t = create_trie(),
			trie_insert(t, "h", (void*) 0),
			trie_insert(t, "i", (void*) 0),
			trie_insert(t, "j", (void*) 0),
			trie_insert(t, "k", (void*) 0),
			trie_insert(t, "information", (void*) 0xbeef),
			trie_search(t, "information"))
