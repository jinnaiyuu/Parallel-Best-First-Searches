/* © 2014 the PBNF Authors under the MIT license. See AUTHORS for the list of authors.*/

/**
 * \file mem.c
 *
 * Implementation for lock-free memory management.  See header file for
 * more verbose comments.
 *
 * \author eaburns
 * \date 2009-03-17
 */

#define _POSIX_C_SOURCE 200112L

#include <assert.h>
#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include "atomic.h"
#include "mem.h"

/**
 * The maximum number of recursive calls to mem_release_recursive
 * before switching over to iterative releasing.  Switching to
 * iterative releasing prevents stack overflows when there are a lot
 * of cascading releases.
 */
#define MEM_RELEASE_DEPTH_MAX 100

/**
 * A simple stack for releasing memory in the iterative
 * implementation.
 */
struct mem_rel_stack {
	struct mem_rel_stack *next;
	struct node *node;
};

static void reclaim(struct freelist *fl, struct node *p);

static void clear_lowest_bit(intptr_t *p);

static int decrement_and_test_and_set(intptr_t *p);

static int mem_rel_stack_push(struct mem_rel_stack **head,
			      struct node *node);

static __attribute__((__unused__)) void mem_release_iterative(struct freelist *fl,
				  struct node *p);

static void mem_release_recursive(struct freelist *fl,
				  struct node *p,
				  int depth);

static void mem_release_recursive(struct freelist *fl,
				  struct node *p,
				  int depth);

struct freelist *mem_freelist_create(size_t nbrelm,
				     size_t nlinks,
				     size_t elmsize)
{
	size_t i;
	struct node *p;
	struct freelist *fl;

	if (nbrelm == 0) {
		errno = EINVAL;
		return NULL;
	}
	if (elmsize < sizeof(*p)) {
		errno = EINVAL;
		return NULL;
	}
	if (nlinks == 0)
		nlinks = 1;

	fl = calloc(1, sizeof(*fl));
	if (!fl) {
		errno = ENOMEM;
		return NULL;
	}
	fl->nlinks = nlinks;
	fl->nalloced = nbrelm;
	fl->nfreed = nbrelm;
	fl->head = NULL;

	for (i = 0; i < nbrelm; i += 1) {
		p = calloc(1, elmsize);
		assert(!IS_MARKED(p));
		if (!p)
			goto err;
		p->refct_claim = 0x1;
		p->links = calloc(nlinks, sizeof(*p->links));
		if (!p->links) {
			free(p);
			goto err;
		}

		p->links[0] = fl->head;
		fl->head = p;
	}

	return fl;

err:
	while (fl->head) {
		p = fl->head;
		fl->head = fl->head->links[0];
		free(p);
	}
	free(fl);
	errno = ENOMEM;
	return NULL;
}

size_t mem_freelist_destroy(struct freelist *fl)
{
	size_t ret = 0;
	struct node *p, *head;

#if !defined(NDEBUG)
	if (fl->nalloced != fl->nfreed) {
		fprintf(stderr, "WARNING: not all nodes freed "
			"alloced=%d, nfreed=%d\n", (int) fl->nalloced,
			(int) fl->nfreed);
	}
#endif	/* !NDEBUG */

	p = head = fl->head;

	while (head) {
		p = head;
		head = head->links[0];
		free(p->links);
		free(p);
		ret += 1;
	}
	free(fl);

	return ret;
}

void *mem_new(struct freelist *fl)
{
	struct node *p;
	int i;

	for ( ; ; ) {
		p = mem_safe_read(fl, &fl->head);
		if (!p) {
			errno = ENOMEM;
			assert(p);
			return NULL;
		}

		if (compare_and_swap(&fl->head,
				     (intptr_t) p,
				     (intptr_t) p->links[0])) {
			clear_lowest_bit(&p->refct_claim);
			for (i = 0; i < fl->nlinks; i += 1)
				p->links[i] = NULL;
			fetch_and_add(&fl->nfreed, -1);
			return p;
		} else {
			mem_release(fl, p);
		}
	}
}

void *mem_safe_read(struct freelist *fl, void *_p)
{
	struct node *q;
	struct node **p = _p;

	for ( ; ; ) {
		q = *p;
		if (!q)
			return NULL;
		mem_incr_ref(q);
		if (q == *p)
			return q;
		else mem_release(fl, q);
	}
}

void *mem_safe_read_null_on_mark(struct freelist *fl, void *_p)
{
	struct node *q;
	struct node **p = _p;

	for ( ; ; ) {
		q = *p;
		if (!q || IS_MARKED(q))
			return NULL;
		mem_incr_ref(q);
		if (q == *p)
			return q;
		else mem_release(fl, q);
	}
}

void mem_incr_ref(void *_p)
{
	struct node *p;

	p = GET_UNMARKED(_p);

	fetch_and_add((intptr_t*) &p->refct_claim, (intptr_t) 2);
}

void mem_release(struct freelist *fl, void *_p)
{
	struct node *p = GET_UNMARKED(_p);

	mem_release_recursive(fl, p, 0);
/*
	mem_release_iterative(fl, p);
*/

}

static void reclaim(struct freelist *fl, struct node *p)
{
	struct node *q;

	assert(!IS_MARKED(p));
	do {
		q = fl->head;
		assert(!IS_MARKED(q));
		p->links[0] = q;
	} while (!compare_and_swap(&fl->head,
				   (intptr_t) q,
				   (intptr_t) p));
	fetch_and_add(&fl->nfreed, 1);
}

static void clear_lowest_bit(intptr_t *p)
{
	intptr_t old, new;

	do {
		old = *p;
		new = old - 1;
	} while (!compare_and_swap(p, old, new));
}

static int decrement_and_test_and_set(intptr_t *p)
{
	intptr_t old, new;

	do {
		old = *p;
		new = old - 2;
		if (new == 0)
			new = 1;
	} while (!compare_and_swap(p, old, new));

	assert(*p != (intptr_t) -1);
	assert(*p < (intptr_t) 10000);

	return (old - new) & 0x1;
}

/**
 * Add an element to a mem_rel_stack and return the new stack head via
 * the 'head' argument.
 *
 * \return 0 on success and !0 on failure (out of memory).
 */
static int mem_rel_stack_push(struct mem_rel_stack **head,
			      struct node *node)
{
	struct mem_rel_stack *elm = calloc(1, sizeof(*elm));
	if (!elm)
		return 1;

	elm->node = node;
	elm->next = *head;
	*head = elm;

	return 0;
}

/**
 * Pop the top element off of the stack.  If the stack is empty, NULL
 * is returned.
 */
static struct node *mem_rel_stack_pop(struct mem_rel_stack **head)
{
	struct mem_rel_stack *elm;
	struct node *node;

	if (!(*head))
		return NULL;

	elm = *head;
	*head = (*head)->next;

	node = elm->node;
	free(elm);

	return node;
}

/**
 * The interative implementation of mem_release.  A stack is used to
 * track cascading releases.  This version is probably slower than the
 * recursive implementation, but it shouldn't cause stack overflows.
 */
static void mem_release_iterative(struct freelist *fl,
				  struct node *p)
{
	int i;
	struct mem_rel_stack *stack = NULL;

	while (p) {
		if (decrement_and_test_and_set(&p->refct_claim) != 0) {
			for (i = 0; i < fl->nlinks; i += 1) {
				struct node *l = p->links[i];
				if (!l)
					break;
				if (IS_MARKED(l))
					l = GET_UNMARKED(l);
				p->links[i] = NULL;
				if (!l)
					continue;

				assert(l != p);

				if (mem_rel_stack_push(&stack, l))
					mem_release_recursive(fl, l, 0);
			}

#if !defined(NDEBUG)
			for (i = 0; i < fl->nlinks; i += 1)
				assert(p->links[i] == NULL);
#endif /* !NDEBUG */

			reclaim(fl, p);
		}

		p = mem_rel_stack_pop(&stack);
	}
}

/**
 * The recursive implementation of mem_release.  This function does
 * the release, but it tracks the depth too.
 */
static void mem_release_recursive(struct freelist *fl,
				  struct node *p,
				  int depth)
{
	int i;

	if (p == NULL)
		return;
	if (decrement_and_test_and_set(&p->refct_claim) == 0)
		return;

	for (i = 0; i < fl->nlinks; i += 1) {
		struct node *l = p->links[i];
		if (!l)
			break;
		if (IS_MARKED(l))
			l = GET_UNMARKED(l);
		p->links[i] = NULL;
		if (!l)
			continue;

		assert(l != p);

		if (depth == MEM_RELEASE_DEPTH_MAX) {
			mem_release_iterative(fl, l);
		} else {
			/* This is a non-tail recursive call and may
			 * cause stack overflow... hence the auxiliary
			 * iterative implementation. */
			mem_release_recursive(fl, l, depth + 1);
		}
	}

#if !defined(NDEBUG)
	for (i = 0; i < fl->nlinks; i += 1)
		assert(p->links[i] == NULL);
#endif /* !NDEBUG */

	reclaim(fl, p);
}

