/* © 2014 the PBNF Authors under the MIT license. See AUTHORS for the list of authors.*/

/**
 * \file ordlist_valois.c
 *
 * An implementation of a lock free ordered ordlist by J. Valois.
 *
 * \author eaburns
 * \date 2009-03-19
 */

#define _POSIX_C_SOURCE 200112L

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

#include "atomic.h"
#include "mem.h"

struct lf_ordlist_node {
	struct node n;
	struct lf_ordlist_node *back; /* not ref counted. */
	int aux;
	void *value;
};

struct lf_ordlist {
	struct lf_ordlist_node *first;
	struct lf_ordlist_node *last;
	struct freelist *freelist;	/* The freelist of queue nodes. */

	size_t num_nodes;	/* Number of nodes alloced for this queue */

	/* Comparison: <0 (a < b)
         *              0 (a == b)
         *             >0 (a > b) */
	int (*cmp)(void *a, void *b);
};

struct lf_cursor {
	struct lf_ordlist_node *pre_cell;
	struct lf_ordlist_node *pre_aux;
	struct lf_ordlist_node *target;
};

/** Nodes with NULL values are the aux nodes. */
#define IS_AUX_NODE(node) ((node)->aux)

/** links[0] is the 'next' link in the ordlist. */
#define NEXT(node) ((node)->n.links[0])

/** links[1] is the 'back' link in the ordlist. */
#define BACK(node) ((node)->n.links[1])

/**
 * This is in no way atomic...
 *
 * Print the given list to the given file.  This does some very
 * *basic* error checking in the list.
 */
static int __attribute__((__unused__))
lf_print_list(FILE *f, struct lf_ordlist *lst)
{
	struct lf_ordlist_node *p;

	p = lst->first;

	do {
		fprintf(f, "%p: refs=%lu, NEXT=%p, BACK=%p, aux=%d, val=%p\n",
			p, (unsigned long) p->n.refct_claim / 2,
			NEXT(p), BACK(p), p->aux, p->value);

		if (p->n.refct_claim & 0x1) {
			fprintf(f, "Bad node\n");
			break;
		}
		if (p == (struct lf_ordlist_node*) NEXT(p)) {
			/* Only tests for single element loops. */
			fprintf(f, "Loop detected\n");
			break;
		}

		p = (struct lf_ordlist_node *) NEXT(p);
	} while (p);
	return 0;
}

/**
 * This is in no way atomic...
 *
 * Print the addresses of the given cursor to the given file.
 */
static int __attribute__((__unused__))
lf_print_cursor(FILE *f, struct lf_cursor *c)
{
	fprintf(f, "pre_cell=%p, pre_aux=%p, target=%p\n",
		c->pre_cell, c->pre_aux, c->target);
	return 0;
}

/**
 * Create a new aux node.
 */
static struct lf_ordlist_node *new_aux(struct lf_ordlist *lst)
{
	struct lf_ordlist_node *n;

	n = mem_new(lst->freelist);
	if (!n)
		return NULL;
	BACK(n) = NULL;
	n->aux = 1;
	n->value = NULL;

	return n;
}


/**
 * Create a new normal node with the given value.
 */
static struct lf_ordlist_node *new_node(struct lf_ordlist *lst, void *v)
{
	struct lf_ordlist_node *n;

	n = mem_new(lst->freelist);
	if (!n)
		return NULL;
	BACK(n) = NULL;
	n->aux = 0;
	n->value = v;

	return n;
}


/**
 * Update the cursor to point to the target properly.
 */
static void lf_update(struct lf_ordlist *lst, struct lf_cursor *c)
{
	int success;
	struct lf_ordlist_node *n, *p;

	if (NEXT(c->pre_aux) == (struct node *) c->target)
		return;

	p = c->pre_aux;
	n = mem_safe_read(lst->freelist, &NEXT(p));
	mem_release(lst->freelist, c->target);

	while (n != lst->last && IS_AUX_NODE(n)) {
		assert(c->pre_cell != n);

		mem_incr_ref(n); /* Incase the CAS succeeds. */
		success = compare_and_swap((intptr_t*) &NEXT(c->pre_cell),
					   (intptr_t) p,
					   (intptr_t) n);

		/* If this succeeds, p is no longer pointed to by
		 * NEXT(c->pre_cell), and n is now pointed to by
		 * NEXT(c->pre_cell) and p (which will be immediately
		 * released). */
		if (success)
			mem_release(lst->freelist, p);
		else
			mem_release(lst->freelist, n);

		mem_release(lst->freelist, p);

		p = n;
		n = mem_safe_read(lst->freelist, &NEXT(p));
	}

	c->pre_aux = p;
	assert(c->pre_aux->aux);
	c->target = n;
	assert(!c->target->aux);
}

/**
 * Release the nodes that the cursor is pointing to (if any).
 */
static void lf_release_cursor(struct lf_ordlist *lst, struct lf_cursor *c)
{
	if (c->pre_cell)
		mem_release(lst->freelist, c->pre_cell);
	if (c->pre_aux)
		mem_release(lst->freelist, c->pre_aux);
	if (c->target)
		mem_release(lst->freelist, c->target);
}

/**
 * Get a cursor to the first element in the ordlist.
 */
static void lf_first(struct lf_ordlist *lst, struct lf_cursor *c)
{
	c->pre_cell = mem_safe_read(lst->freelist, &lst->first);
	c->pre_aux = mem_safe_read(lst->freelist, &NEXT(lst->first));
	c->target = NULL;

	lf_update(lst, c);
}

/**
 * Move the cursor down the ordlist.
 *
 * \return !0 on success and 0 on failure (at the end of the ordlist).
 */
static int lf_next(struct lf_ordlist *lst, struct lf_cursor *c)
{
	if (c->target == lst->last)
		return 0;

	assert(NEXT(c->target));

	mem_release(lst->freelist, c->pre_cell);
	c->pre_cell = mem_safe_read(lst->freelist, &c->target);

	mem_release(lst->freelist, c->pre_aux);
	c->pre_aux = mem_safe_read(lst->freelist, &NEXT(c->target));

	assert(!(c->pre_aux->n.refct_claim & 0x1));
	assert(c->pre_aux);
	assert(c->pre_aux->aux);

	lf_update(lst, c);

	return 1;
}

/**
 * Insert an element into the ordlist before the cursor.
 *
 * \return !0 on success, 0 on failure, -1 on error (errno is set).
 *         An error will be an allocation error, as opposed to failure
 *         where the element is merely not inserted.
 */
static int lf_insert(struct lf_ordlist *lst, struct lf_cursor *c, void *value)
{
	int success = -1;
	struct lf_ordlist_node *a, *q;

	assert(c->pre_aux->aux == 1);
	assert(!(c->pre_aux->n.refct_claim & 0x1));

	q = new_node(lst, value);
	if (!q)
		return -1;
	a = new_aux(lst);
	if (!a)
		goto fail_a;

	NEXT(q) = (struct node *) a;
	NEXT(a) = (struct node *) c->target;

	assert(c->pre_aux != q);
	success = compare_and_swap((intptr_t*) &NEXT(c->pre_aux),
				   (intptr_t) c->target,
				   (intptr_t) q);
	if (!success)
		goto fail;

/*
  printf("added:   (pre_aux=%p, cell=%p) val=%p\n", c->pre_aux, q, value);
*/
	return 1;

fail:
	NEXT(a) = NULL;
	mem_release(lst->freelist, a);
fail_a:
	NEXT(q) = NULL;
	mem_release(lst->freelist, q);
	return success;
}

/**
 * Tries to find the insertion place for an element in the ordlist.
 *
 * \return !0 if an equivalant value is found (the cursor points to
 *         it), 0 if the key is not found (the cursor points to the
 *         smallest key greater than the searched key).
 */
static int lf_find_from(struct lf_ordlist *lst, struct lf_cursor *c, void *value)
{
	while (c->target != lst->last) {
		if (lst->cmp(c->target->value, value) == 0)
			return 1;
		else if (lst->cmp(c->target->value, value) > 0)
			return 0;
		else
			lf_next(lst, c);
	}

	return 0;
}

/**
 * Delete the node at the cursor.  Try to remove adjacent aux cells.
 *
 * \note This operation uses the references from the cursor.  DO NOT
 *       release the cursor after this operation.
 *
 * \return 0 on failure, !0 on success.
 */
int lf_delete(struct lf_ordlist *lst, struct lf_cursor *c)
{
	int success;
	struct lf_ordlist_node *d, *n, *p, *s;

	assert(c->target != lst->first);
	assert(c->target != lst->last);

	/*
	 * Try to remove the node.
	 */
	d = c->target;
	n = mem_safe_read(lst->freelist, &NEXT(c->target));
	assert(c->pre_aux != n);
	mem_incr_ref(n);	/*  For the CAS. */
	success = compare_and_swap((intptr_t*) &NEXT(c->pre_aux),
				   (intptr_t) d,
				   (intptr_t) n);

	if (!success) {
		mem_release(lst->freelist, n); /* The ref from the CAS. */
		mem_release(lst->freelist, n); /* Our ref from the safe_read */
		return 0;
	}
	/* On success, NEXT(c->pre_aux) will reference n and not d. */
	mem_release(lst->freelist, d);	/* NEXT(c->pre_aux) drops d. */

/*
  printf("deleted: (pre_aux=%p, cell=%p) val=%p\n",
  c->pre_aux, d, d->value);
*/

	/* If NEXT(d) == NULL, the memory manager will not free
	 * BACK(d).  Anyway, NEXT(d) == NULL would mean that we are
	 * deleting the dummy node at 'last'. */
	assert(NEXT(d));
	assert(NEXT(d) != (struct node*) c->pre_aux);
	assert(NEXT(d) != (struct node*) c->pre_cell);
	assert(NEXT(d) != (struct node*) c->target);

#if !defined(NDEBUG)
	/* This CAS is unnecessary, we should be the only thread who
	 * has d with the intent to set the BACK(d) pointer. */
	success = compare_and_swap(&BACK(d), (intptr_t) NULL,
				   (intptr_t) c->pre_cell);
	assert(success);
#else
	BACK(d) = (struct node *) c->pre_cell;
#endif	/* !NDEBUG */
	mem_incr_ref(c->pre_cell); /* BACK(d) points to c->pre_cell. */

	/*
	 * Find the previous undeleted cell.
	 */
	p = c->pre_cell;
	mem_incr_ref(c->pre_cell); /* Track this reference to p. */
	while (BACK(p) != NULL) {
		struct lf_ordlist_node *q;
		q = mem_safe_read(lst->freelist, &BACK(p));
		mem_release(lst->freelist, p);
		p = q;
	}

	/*
	 * Get the aux cell that follows the deleted cell.
	 */
	while (IS_AUX_NODE((struct lf_ordlist_node *) NEXT(n))) {
		struct lf_ordlist_node *q;
		q = mem_safe_read(lst->freelist, &NEXT(n));
		mem_release(lst->freelist, n);
		n = q;
	}

	/*
	 * Try to swap into the next pointer of the previous undeleted
	 * cell the aux cell that followed the node we deleted.
	 */
	s = mem_safe_read(lst->freelist, &NEXT(p));
	do {
		assert(p != n);

		/* if this succeeds, NEXT(p) gets our reference to n. */
		success = compare_and_swap(&NEXT(p), (intptr_t) s,
					   (intptr_t) n);
		if (!success) {
			mem_release(lst->freelist, s);
			s = mem_safe_read(lst->freelist, &NEXT(p));
		}
	} while (!success
		 && BACK(p) == NULL
		 && !IS_AUX_NODE((struct lf_ordlist_node*) NEXT(n)));

	/* If the CAS succeeded, it got our reference to n, and we
	 * need to release NEXT(p)'s reference to s.  If it failed, we
	 * need to release our reference to n. */
	if (success)
		mem_release(lst->freelist, s);
	else
		mem_release(lst->freelist, n);

	mem_release(lst->freelist, p);
	mem_release(lst->freelist, s);

	return 1;
}

void *lf_ordlist_find(struct lf_ordlist *lst, void *value)
{
	void *val = NULL;
	struct lf_cursor c;
	lf_first(lst, &c);

	if (lf_find_from(lst, &c, value))
		val =  c.target->value;

	lf_release_cursor(lst, &c);

	return val;
}


int lf_ordlist_add(struct lf_ordlist *lst, void *value)
{
	int success;
	struct lf_cursor c;

	lf_first(lst, &c);
	for( ; ; ) {
		success = lf_find_from(lst, &c, value);
		if (success)
			break;
		success = lf_insert(lst, &c, value);
		if (success)
			break;
		lf_update(lst, &c);
	}

	lf_release_cursor(lst, &c);
	return success;
}

int lf_ordlist_remove(struct lf_ordlist *lst, void *value)
{
	int success;
	struct lf_cursor c;

	lf_first(lst, &c);

	for ( ; ; ) {
		success = lf_find_from(lst, &c, value);
		if (!success) {
			break;
		}

		success = lf_delete(lst, &c);
		if (success)
			break;

		lf_update(lst, &c);
	}
	lf_release_cursor(lst, &c);
	return success;
}

int lf_ordlist_empty(struct lf_ordlist *lst)
{
	int ret;
	struct lf_cursor c;

	lf_first(lst, &c);
	ret = (c.target == lst->last);
	lf_release_cursor(lst, &c);

	return ret;
}

struct lf_ordlist *lf_ordlist_create(size_t nbrelm,
				     int (*cmp)(void *a, void *b))
{
	struct lf_ordlist *lst;

	lst = calloc(1, sizeof(*lst));
	if (!lst)
		return NULL;

	lst->cmp = cmp;

	/* 2 dummy nodes, one aux node in between each regular node. */
	lst->num_nodes = (nbrelm * 2) + 3;

	lst->freelist = mem_freelist_create(lst->num_nodes, 2,
					    sizeof(struct lf_ordlist_node));
	if (!lst->freelist)
		goto err_lst;

	lst->first = new_node(lst, NULL);
	if (!lst->first)
		goto err_first;

	NEXT(lst->first) = (struct node *) new_aux(lst);
	if (!NEXT(lst->first))
		goto err_aux;

	lst->last = new_node(lst, NULL);
	if (!lst->last)
		goto err_last;

	NEXT((struct lf_ordlist_node *) NEXT(lst->first)) =
		(struct node *) lst->last;
	mem_incr_ref(lst->last);

	return lst;

err_last:
	mem_release(lst->freelist, NEXT(lst->first));
err_aux:
	mem_release(lst->freelist, lst->first);
err_first:
	mem_freelist_destroy(lst->freelist);
err_lst:
	free(lst);
	return NULL;
}

void lf_ordlist_destroy(struct lf_ordlist *lst)
{
	size_t nfreed;

	mem_release(lst->freelist, lst->first);
	mem_release(lst->freelist, lst->last);

	nfreed = mem_freelist_destroy(lst->freelist);
	assert(nfreed == lst->num_nodes);

	free(lst);
}
