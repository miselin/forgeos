/*
 * Copyright (c) 2012 Matthew Iselin, Rich Edelman, Jörg Pfähler, James Molloy
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

#include <util.h>
#include <malloc.h>
#include <assert.h>

/**
 * \note Lifted much of this from Pedigree, to be honest. The iterator stuff
 * is mine, as is the lookup function. Everything else should be attributed to
 * Jörg Pfähler and/or James Molloy.
 *
 * This is just a standard balanced AVL tree implementation that uses void* keys
 * and permits a custom compare function (for the situations where objects are
 * used as keys).
 */

struct node {
    void *key;
    void *val;

    size_t height;

    /// Reference count - the number of iterators currently referencing this node.
    size_t refcount;

    /// Deletion flag - should this node be freed if the refcount hits zero?
    int delete;

    struct node *parent;

    struct node *left;
    struct node *right;
};

struct iterator {
    struct node *n;
    struct node *prev;
};

struct tree {
    struct node *root;

    tree_comparer cmp_func;

    size_t len;
};

static void rotate_left(struct tree *meta, struct node *n);
static void rotate_right(struct tree *meta, struct node *n);

static size_t node_height(struct node *n);
static int node_bfactor(struct node *n);

static void node_rebalance(struct tree *meta, struct node *n);

static int default_comparer(void *a, void *b) {
    /// \note YES THIS MEANS 64-BIT SYSTEMS HAVE MORE KEY SPACE. Less than ideal.
    unative_t ai = (unative_t) a, bi = (unative_t) b;
    if(ai < bi)
        return -1;
    else if(ai == bi)
        return 0;
    else
        return 1;
}

static void remove_node(struct tree *meta, struct node *n) {
    meta->len--;

    struct node *o = n;

    while(n->left || n->right) {
        size_t hl = node_height(n->left);
        size_t hr = node_height(n->right);

        if(!hl)
            rotate_left(meta, n);
        else if(!hr)
            rotate_right(meta, n);
        else if(hl <= hr) {
            rotate_right(meta, n);
            rotate_left(meta, n);
        } else {
            rotate_left(meta, n);
            rotate_right(meta, n);
        }
    }

    if(!n->parent)
        meta->root = 0;
    else {
        if(n->parent->left == n)
            n->parent->left = 0;
        else
            n->parent->right = 0;
    }


    while(n) {
        int b = node_bfactor(n);
        if((b < -1) || (b > 1))
            node_rebalance(meta, n);
        n = n->parent;
    }

    free(o);
}

static int free_node_if_needed(struct tree *meta, struct node *n) {
	assert(n != 0);

	if((!n->refcount) && (n->delete)) {
    	remove_node(meta, n);
		free(n);

		return 1;
	}

	return 0;
}

void *create_tree() {
    return create_tree_cmp(default_comparer);
}

void *create_tree_cmp(tree_comparer cmp) {
    struct tree *meta = (struct tree *) malloc(sizeof(struct tree));
    meta->root = 0;
    meta->cmp_func = cmp;

    return (void *) meta;
}

void delete_tree(void *t) {
    if(!t)
        return;

    struct tree *meta __unused = (struct tree *) t;

    /// \todo Implement me!
}

void *tree_iterator(void *t) {
    if(!t)
        return 0;

    struct tree *meta = (struct tree *) t;
    struct iterator *it = (struct iterator *) malloc(sizeof(struct iterator));
    memset(it, 0, sizeof(*it));
    it->n = meta->root;

    while(it->n->left) {
        it->n = it->n->left;
    }

    it->n->refcount++;
    return (void *) it;
}

void *tree_key(void *n) {
	if(!n)
		return TREE_NOTFOUND;

	struct node *p = (struct node *) n;
	return p->key;
}

void *tree_val(void *n) {
	if(!n)
		return TREE_NOTFOUND;

	struct node *p = (struct node *) n;
	return p->val;
}

int tree_iterator_bof(void *t, void *it) {
	if(!t || !it)
		return 0;

    struct iterator *i = (struct iterator *) it;

    return i->n == tree_min(t) ? 1 : 0;
}

int tree_iterator_eof(void *t, void *it) {
	if(!t || !it)
		return 0;

    struct iterator *i = (struct iterator *) it;

    return i->n ? 0 : 1;
}

void *tree_min(void *t) {
    if(!t)
        return 0;

    struct tree *meta = (struct tree *) t;
    struct node *p = meta->root;
    while(p->left) {
    	p = p->left;
    }

    return (void *) p;
}

void *tree_max(void *t) {
    if(!t)
        return 0;

    struct tree *meta = (struct tree *) t;
    struct node *p = meta->root;
    while(p->right) {
    	p = p->right;
    }

    return (void *) p;
}

void *tree_next(void *t, void *i) {
    if(!t || !i)
        return 0;

    struct tree *meta = (struct tree *) t;
    struct iterator *it = (struct iterator *) i;
    struct node *p = it->n;

	// If at the end of the tree, we can't iterate further.
    if(!it->n) {
    	return it->prev;
    }

    if(it->n->right) {
    	it->n = it->n->right;
    	while(it->n->left)
    		it->n = it->n->left;
    } else {
    	while(it->n->parent && (it->n == it->n->parent->right))
    		it->n = it->n->parent;
    	it->n = it->n->parent;
    }

	/// \todo Atomicity.

	// Free the node we were at before if it was to be deleted.
	p->refcount--;
	if(!free_node_if_needed(meta, p)) {
	    // Previous node is now the current node.
    	it->prev = p;
    }

	if(it->n)
		it->n->refcount++;
    return it->n;
}

void *tree_prev(void *t, void *i) {
    if(!t || !i)
        return 0;

    struct tree *meta = (struct tree *) t;
    struct iterator *it = (struct iterator *) i;
    struct node *p = it->n;

    if(it->n == 0) {
    	it->n = it->prev;
    	it->n->refcount++;
    	return it->n;
    }

    if(it->n->left) {
    	it->n = it->n->left;
    	while(it->n->right)
    		it->n = it->n->right;
    } else {
    	while(it->n->parent && (it->n == it->n->parent->left))
    		it->n = it->n->parent;
	    it->n = it->n->parent;
    }

	/// \todo Atomicity.
	// Free the node we were at before if it was to be deleted.
	p->refcount--;
	free_node_if_needed(meta, p);

	if(it->n) {
		it->prev = p;
		it->n->refcount++;
	}
    return it->n;
}

void tree_deliterator(void *t, void *i) {
    if(!t || !i)
        return;

    free(i);
}

void tree_insert(void *t, void *key, void *val) {
    if(!t)
        return;
    struct tree *meta = (struct tree *) t;

    // Avoid insertion if the key is already in the tree.
    if(tree_search(t, key) != TREE_NOTFOUND)
        return;

    struct node *new_node = (struct node *) malloc(sizeof(struct node));
    memset(new_node, 0, sizeof(*new_node));
    new_node->key = key;
    new_node->val = val;

    meta->len++;

    if(!meta->root) {
        meta->root = new_node;
        return;
    }

    struct node *n = meta->root;
    while(1) {
        int c = meta->cmp_func(key, n->key);
        if(c < 0) {
            if(!n->left) {
                n->left = new_node;
                new_node->parent = n;
                break;
            } else
                n = n->left;
        } else {
            if(!n->right) {
                n->right = new_node;
                new_node->parent = n;
                break;
            } else
                n = n->right;
        }
    }

    while(n) {
        int b = node_bfactor(n);
        if((b < -1) || (b > 1))
            node_rebalance(meta, n);
        n = n->parent;
    }
}

void tree_delete(void *t, void *key) {
    if(!t)
        return;

    struct tree *meta = (struct tree *) t;
    if(!meta->len)
        return;

    if(tree_search(t, key) == TREE_NOTFOUND)
        return;

    struct node *n = meta->root;
    while(n) {
        int c = meta->cmp_func(key, n->key);
        if(!c) {
            break;
        }

        if(c < 0)
            n = n->left;
        else
            n = n->right;
    }

    if(!n) return;

	n->delete = 1;
    if(!n->refcount) {
    	remove_node(meta, n);
    }
}

void *tree_search(void *t, void *search_key) {
    if(!t)
        return TREE_NOTFOUND;

    struct tree *meta = (struct tree *) t;
    if(!meta->len)
        return TREE_NOTFOUND;

    struct node *n = meta->root;
    while(n) {
        int c = meta->cmp_func(search_key, n->key);
        if(!c) {
            return n->val;
        }

        if(c < 0)
            n = n->left;
        else
            n = n->right;
    }

    return TREE_NOTFOUND;
}

void rotate_left(struct tree *meta, struct node *n) {
    struct node *y = n->right;

    n->right = y->left;
    if(y->left)
        y->left->parent = n;

    y->parent = n->parent;
    if(!n->parent)
        meta->root = y;
    else if(n == n->parent->left)
        n->parent->left = y;
    else
        n->parent->right = y;

    y->left = n;
    n->parent = y;
}

void rotate_right(struct tree *meta, struct node *n) {
    struct node *y = n->left;

    n->left = y->right;
    if(y->right)
        y->right->parent = n;

    y->parent = n->parent;
    if(!n->parent)
        meta->root = y;
    else if(n == n->parent->left)
        n->parent->left = y;
    else
        n->parent->right = y;

    y->right = n;
    n->parent = y;
}

size_t node_height(struct node *n) {
    if(!n)
        return 0;

    size_t tempL = 0, tempR = 0;

    if(n->left)
        tempL = n->left->height;
    if(n->right)
        tempR = n->right->height;

    tempL++; tempR++;

    if(tempL > tempR) {
        n->height = tempL;
        return tempL;
    } else {
        n->height = tempR;
        return tempR;
    }
}

int node_bfactor(struct node *n) {
    return ((int) node_height(n->right)) - ((int) node_height(n->left));
}

void node_rebalance(struct tree *meta, struct node *n) {
    int b = node_bfactor(n);
    if(b < -1) {
        if(node_bfactor(n->left) > 0) {
            rotate_left(meta, n->left);
            rotate_right(meta, n);
        } else {
            rotate_right(meta, n);
        }
    } else if(b > 1) {
        if(node_bfactor(n->right) < 0) {
            rotate_right(meta, n->right);
            rotate_left(meta, n);
        } else {
            rotate_left(meta, n);
        }
    }
}

