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
    
    struct node *parent;
    
    struct node *left;
    struct node *right;
};

struct iterator {
    struct node *n;
    size_t idx;
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
    
    return (void *) it;
}

void *tree_next(void *t, void *i) {
    if(!t || !i)
        return 0;
    
    struct tree *meta = (struct tree *) t;
    struct iterator *it = (struct iterator *) i;
    
    assert(it->n != 0);
    
    size_t n = it->idx++;
    if(it->idx > meta->len)
        it->idx = meta->len;
    
    if(n == 0) {
        return it->n->key;
    } else if(n == meta->len) {
        return 0;
    }
    
    // Traverse to the right (assume came from the left)
    if(it->n->right) {
        it->n = it->n->right;
    } else if(it->n->parent) {
        it->n = it->n->parent;
    }
    
    return it->n->key;

/*
          if((pPreviousNode == pNode->parent) && pNode->leftChild)
          {
            pPreviousNode = pNode;
            pNode = pNode->leftChild;
            traverseNext();
          }
          else if((((pNode->leftChild) && (pPreviousNode == pNode->leftChild)) || ((!pNode->leftChild) && (pPreviousNode != pNode))) && (pPreviousNode != pNode->rightChild))
          {
            pPreviousNode = pNode;
          }
          else if((pPreviousNode == pNode) && pNode->rightChild)
          {
            pPreviousNode = pNode;
            pNode = pNode->rightChild;
            traverseNext();
          }
          else
          {
            pPreviousNode = pNode;
            pNode = pNode->parent;
            traverseNext();
          }
*/
}

void *tree_prev(void *t, void *i) {
    if(!t || !i)
        return 0;
    
    struct tree *meta = (struct tree *) t;
    struct iterator *it = (struct iterator *) i;
    
    assert(it->n != 0);
    
    size_t n = it->idx;
    
    if(it->idx)
        it->idx--;
    
    if(n == 0) {
        return 0;
    } else if(n == meta->len) {
        return it->n->key;
    }
    
    // Traverse to the left (assume we came from the right)
    if(it->n->left) {
        it->n = it->n->left;
    } else if(it->n->parent) {
        it->n = it->n->parent;
    }
    
    return it->n->key;
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
    if(tree_search(t, key) != 0)
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
    
    if(tree_search(t, key) == 0)
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

void *tree_search(void *t, void *search_key) {
    if(!t)
        return 0;
    
    struct tree *meta = (struct tree *) t;
    if(!meta->len)
        return 0;
    
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
    
    return 0;
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

