/* © 2014 the PBNF Authors under the MIT license. See AUTHORS for the list of authors.*/

/**
 * \file ordlist_harris.c
 *
 * Timothy Harris' ordered list implementation: ``A Pragmatic
 * Implementation of Non-blocking Linked-Lists.''
 *
 * \author eaburns
 * \date 2009-04-10
 */

#define _POSIX_C_SOURCE 200112L

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

#include "atomic.h"
#include "mem.h"

struct lf_ordlist_node {
	struct node n;
	void *value;
};

struct lf_ordlist {
	struct lf_ordlist_node *head;
	struct lf_ordlist_node *tail;
	struct freelist *fl;

	/* Comparison: <0 (a < b)
         *              0 (a == b)
         *             >0 (a > b) */
	int (*cmp)(void *a, void *b);

	/* number of elements that this list can have. */
	size_t nelms;
};

/*
#define mem_release(fl, elm) \
	do {		     \
		mem_release((fl), (elm));	\
		assert(lst->tail->n.refct_claim >= 4);	\
	} while (0)
*/

/** The next field of a node is the first (and only) link field. */
#define NEXT(node) (((struct lf_ordlist_node*) (node))->n.links[0])


/**
 * Search for a key's position in the ordered list.  Upon return,
 * *left_node points to the left node (and it has a reference) and the
 * return value points to a node that is referenced too.
 */
static struct lf_ordlist_node *search(struct lf_ordlist *lst,
				      void *key,
				      struct lf_ordlist_node **left_node)
{
	struct lf_ordlist_node *left_node_next, *right_node;
	struct lf_ordlist_node *t, *t_next;

search_again:
	for ( ; ; ) {
		*left_node = left_node_next = NULL;
		t = mem_safe_read(lst->fl, &lst->head);
		t_next = mem_safe_read(lst->fl, &NEXT(lst->head));

		/* 1: Find left_node and right_node
		 *
		 * Entering this loop: t and t_next are referenced.
		 *
		 * Leaving this loop: t is referenced, and if t_next
		 *                    is not marked then *left_node
		 *                    and left_node_next are set and
		 *                    have their own references.  If t
		 *                    != lst->tail, then t_next is
		 *                    also referenced.
		 */
		do {
			if (!IS_MARKED(t_next)) {
				/* these refs may have been copied
				 * before, but we had to loop again */
				if (*left_node) {
					mem_release(lst->fl, *left_node);
					*left_node = NULL;
				}
				if (left_node_next) {
					mem_release(lst->fl, left_node_next);
					left_node_next = NULL;
				}

				/* copy both t and t_next's refs */
				mem_incr_ref(t);
				(*left_node) = t;
				mem_incr_ref(t_next);
				left_node_next = t_next;
			}

			mem_release(lst->fl, t);
			t = GET_UNMARKED(t_next); /* take t_next's ref */

			if (t == lst->tail)
				break;

			t_next = mem_safe_read(lst->fl, &NEXT(t));
		} while (IS_MARKED(t_next) || lst->cmp(t->value, key) < 0);
		if (t != lst->tail)
			mem_release(lst->fl, t_next); /* done with t_next */
		right_node = t;	/* takes t's reference */

		/*
		 * At this point, right_node, *left_node and
		 * left_node_next all have references.
		 */

		/* 2: Check nodes are adjacent */
		if (left_node_next == right_node) {
			mem_release(lst->fl, left_node_next);
			if (right_node != lst->tail
			    && IS_MARKED(NEXT(right_node))) {
				mem_release(lst->fl, right_node);
				mem_release(lst->fl, *left_node);
				goto search_again;
			} else {
				return right_node;
			}
		}

		/* 3: Remove one or more marked nodes
		 *
		 * Here, left_node_next, right_node and *left_node are
		 * referenced. */

		/* in case CAS succeeds. */
		mem_incr_ref(right_node);
		assert(*left_node != lst->tail);
		if (compare_and_swap(&NEXT(*left_node),
				     (intptr_t) left_node_next,
				     (intptr_t) right_node)) {

			/* one for NEXT(*left_node), one for
			 * 'left_node_next' */
			mem_release(lst->fl, left_node_next);
			mem_release(lst->fl, left_node_next);

			if ((right_node != lst->tail)
			    && IS_MARKED(NEXT(right_node))) {
				mem_release(lst->fl, right_node);
				mem_release(lst->fl, *left_node);
				goto search_again;
			} else {
				return right_node;
			}
		}
		/* one for the CAS prep. ref and one for
		 * 'right_node' */
		mem_release(lst->fl, right_node);
		mem_release(lst->fl, right_node);

		mem_release(lst->fl, *left_node);
		mem_release(lst->fl, left_node_next);

	} /* for( ; ; ) */

	/* should not reach here */
	assert(0);
}

int lf_ordlist_empty(struct lf_ordlist *lst)
{
	return ((struct lf_ordlist_node *) NEXT(lst->head)) == lst->tail;
}

void *lf_ordlist_find(struct lf_ordlist *lst, void *value)
{
	void *ret = NULL;
	struct lf_ordlist_node *right_node, *left_node;

	right_node = search(lst, value, &left_node);
	if (right_node != lst->tail
	    && lst->cmp(right_node->value, value) == 0)
		ret = right_node->value;

	mem_release(lst->fl, right_node);
	mem_release(lst->fl, left_node);

	return ret;
}

int lf_ordlist_remove(struct lf_ordlist *lst, void *value)
{
	struct lf_ordlist_node *right_node, *right_node_next, *left_node;

	for ( ; ; ) {
		right_node = search(lst, value, &left_node);
		if ((right_node == lst->tail)
		    || lst->cmp(right_node->value, value) != 0) {
			mem_release(lst->fl, right_node);
			mem_release(lst->fl, left_node);
			return 0;
		}

		right_node_next = mem_safe_read(lst->fl, &NEXT(right_node));

		if (!IS_MARKED(right_node_next)) {
			assert(right_node != lst->tail);
			if (compare_and_swap(&NEXT(right_node),
					     (intptr_t) right_node_next,
					     (intptr_t)
					     GET_MARKED(right_node_next)))
				break;
		}
		mem_release(lst->fl, right_node_next);
		mem_release(lst->fl, right_node);
		mem_release(lst->fl, left_node);
	}

	/* if the CAS succeeds, NEXT(left_node) gets our ref to
	 * 'right_node_next' */
	assert(left_node != lst->tail);
	if (!compare_and_swap(&NEXT(left_node),
			      (intptr_t) right_node,
			      (intptr_t) right_node_next)) {

		mem_release(lst->fl, right_node_next);
		mem_release(lst->fl, right_node);
		mem_release(lst->fl, left_node);

		/* delete it via a search. */
		right_node = search(lst, value, &left_node);
		mem_release(lst->fl, right_node);
		mem_release(lst->fl, left_node);
	} else {
		/* safely deleted. */
		mem_release(lst->fl, right_node); /* our ref */
		mem_release(lst->fl, right_node); /* NEXT(left_node) ref */

		mem_release(lst->fl, left_node);
	}
	return 1;
}

void *lf_ordlist_cond_update(struct lf_ordlist *lst,
			     int (*f)(void *, void*),
			     void *value)
{
	struct lf_ordlist_node *new_node;
	struct lf_ordlist_node *right_node, *left_node;

	assert(value);

	new_node = mem_new(lst->fl);
	new_node->value = value;

	for ( ; ; ) {
		right_node = search(lst, value, &left_node);
		if (right_node != lst->tail
		    && lst->cmp(right_node->value, value) == 0) {

			mem_release(lst->fl, left_node);
			mem_release(lst->fl, new_node);


			void *ret = right_node->value;
			while (f /* Never changes */) {
				void *v = right_node->value;

 				if (!f(v, value)) {
					ret = v;
					break;
				}

				if (compare_and_swap(&right_node->value,
						     (intptr_t) v,
						     (intptr_t) value)) {
					ret = value;
					break;
				}
			}


			mem_release(lst->fl, right_node);

			return ret;
		}

		/* NEXT(left_node) gets our reference if CAS succeeds,
		 * and NEXT(new_node) gets the ref to right_node if
		 * CAS succeeds */
		NEXT(new_node) = (struct node *) right_node;

		assert(left_node != lst->tail);
		if (compare_and_swap(&NEXT(left_node),
				     (intptr_t) right_node,
				     (intptr_t) new_node)) {
			mem_release(lst->fl, right_node);
			mem_release(lst->fl, left_node);
			return value;
		}
		mem_release(lst->fl, right_node);
		mem_release(lst->fl, left_node);

		/* If we release 'new_node' later, don't also release
		 * right_node. */
		NEXT(new_node) = NULL;
	}
}

void *lf_ordlist_add(struct lf_ordlist *lst, void *value)
{
/* 	struct lf_ordlist_node *new_node; */
/* 	struct lf_ordlist_node *right_node, *left_node; */

/* 	assert(value); */

/* 	new_node = mem_new(lst->fl); */
/* 	new_node->value = value; */

/* 	for ( ; ; ) { */
/* 		right_node = search(lst, value, &left_node); */
/* 		if (right_node != lst->tail */
/* 		    && lst->cmp(right_node->value, value) == 0) { */
/* 			mem_release(lst->fl, right_node); */
/* 			mem_release(lst->fl, left_node); */
/* 			mem_release(lst->fl, new_node); */
/* 			return right_node->value; */
/* 		} */

/* 		/\* NEXT(left_node) gets our reference if CAS succeeds, */
/* 		 * and NEXT(new_node) gets the ref to right_node if */
/* 		 * CAS succeeds *\/ */
/* 		NEXT(new_node) = (struct node *) right_node; */

/* 		assert(left_node != lst->tail); */
/* 		if (compare_and_swap(&NEXT(left_node), */
/* 				     (intptr_t) right_node, */
/* 				     (intptr_t) new_node)) { */
/* 			mem_release(lst->fl, right_node); */
/* 			mem_release(lst->fl, left_node); */
/* 			return value; */
/* 		} */
/* 		mem_release(lst->fl, right_node); */
/* 		mem_release(lst->fl, left_node); */
/* 	} */

	return lf_ordlist_cond_update(lst, NULL, value);
}

void lf_ordlist_print(FILE *f, struct lf_ordlist *lst)
{
	struct lf_ordlist_node *n;

	fprintf(f, "head=%p\n", lst->head);
	for (n = (struct lf_ordlist_node *) NEXT(lst->head);
	     n != lst->tail;
	     n = (struct lf_ordlist_node *) NEXT(n)) {
		fprintf(f, " elm=%p, value=%p, next=%p\n",
			n, n->value, NEXT(n));
	}
	fprintf(f, "tail=%p\n", lst->tail);
}

struct lf_ordlist *lf_ordlist_create(size_t nbrelm,
				     int (*cmp)(void *a, void *b))
{
	struct lf_ordlist *lst;

	lst = calloc(1, sizeof(*lst));
	if (!lst)
		return NULL;
	lst->cmp = cmp;
	lst->nelms = nbrelm + 2;

	lst->fl = mem_freelist_create(lst->nelms, 1,
				      sizeof(struct lf_ordlist_node));
	if (!lst->fl)
		goto err_fl;

	lst->head = mem_new(lst->fl);
	if (!lst->head)
		goto err_head;

	lst->tail = mem_new(lst->fl);
	if (!lst->tail)
		goto err_tail;

	mem_incr_ref(lst->tail);
	NEXT(lst->head) = (struct node *) lst->tail;

	return lst;

err_tail:
	mem_release(lst->fl, lst->head);

err_head:
	mem_freelist_destroy(lst->fl);

err_fl:
	free(lst);
	return NULL;
}

void lf_ordlist_destroy(struct lf_ordlist *lst)
{
	if (lst->tail->n.refct_claim != 4)
		lf_ordlist_print(stdout, lst);
	assert(lst->tail->n.refct_claim == 4);
	mem_release(lst->fl, lst->head);
	assert(lst->tail->n.refct_claim == 2);
	mem_release(lst->fl, lst->tail);
	mem_freelist_destroy(lst->fl);

	free(lst);
}
