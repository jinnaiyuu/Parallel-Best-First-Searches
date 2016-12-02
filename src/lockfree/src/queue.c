/* © 2014 the PBNF Authors under the MIT license. See AUTHORS for the list of authors.*/

/**
 * \file queue.c
 *
 *
 *
 * \author eaburns
 * \date 2009-03-18
 */

#define _POSIX_C_SOURCE 200112L

#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>

#include "atomic.h"
#include "mem.h"

/**
 * A node in the lock free queue.
 */
struct lf_queue_node {
	struct node n;		/* links[0] used as next pointer */
	void *value;
};

/**
 * A lock free queue implementation.
 */
struct lf_queue {
	struct lf_queue_node *head; /* The head of the queue. */
	struct lf_queue_node *tail; /* The tail of the queue. */
	struct freelist *freelist; /* The freelist of queue nodes. */
	size_t num_nodes;
};


/** links[0] is the 'next' link in the queue. */
#define NEXT(node) ((node)->n.links[0])

int lf_queue_enqueue(struct lf_queue *queue, void *elm)
{
	struct lf_queue_node *p, *q, *r, *old;

	if (!elm) {
		errno = EINVAL;
		return -1;
	}

	/* This ref counts for the prev node's next ptr too. */
	q = mem_new(queue->freelist);
	if (!q)
		return -1;

	q->value = elm;
	NEXT(q) = NULL;
	mem_incr_ref(q);	/* Either tail ref, or this is released */

	p = mem_safe_read(queue->freelist, &queue->tail);
	old = mem_safe_read(queue->freelist, &p);

	do {
		while (NEXT(p)) {
			r = p;
			p = mem_safe_read(queue->freelist, &NEXT(p));
			mem_release(queue->freelist, r);
		}
	} while (!compare_and_swap((intptr_t*) &NEXT(p),
				  (intptr_t) NULL,
				  (intptr_t) q));

	mem_release(queue->freelist, p);
	if (compare_and_swap(&queue->tail, (intptr_t) old, (intptr_t) q)) {
		mem_release(queue->freelist, old);
		mem_release(queue->freelist, old);
	} else {
		mem_release(queue->freelist, old);
		mem_release(queue->freelist, q);
	}

	return 0;
}

void *lf_queue_dequeue(struct lf_queue *queue)
{
	int success;
	void *value;

	struct lf_queue_node *next, *p;

	do {
		p = mem_safe_read(queue->freelist, &queue->head);
		next = (struct lf_queue_node *) NEXT(p);
		if (!next) {
			mem_release(queue->freelist, p);
			return NULL;
		}

		mem_incr_ref(next);

		success = compare_and_swap(&queue->head, (intptr_t) p,
					   (intptr_t) next);
		if (!success)
			mem_release(queue->freelist, next);

		mem_release(queue->freelist, p);
	} while (!success);

	value = ((struct lf_queue_node *) NEXT(p))->value;
	assert((struct lf_queue_node *) NEXT(p) == next);
	mem_release(queue->freelist, p);

	return value;
}

int lf_queue_empty(struct lf_queue *queue)
{
	return NEXT(queue->head) == NULL;
}

struct lf_queue *lf_queue_create(size_t nbrelm)
{
	struct lf_queue *q;

	q = calloc(1, sizeof(*q));
	if (!q) {
		errno = ENOMEM;
		return NULL;
	}

	/* Over allocate by 1 for the dummy node. */
	q->num_nodes = nbrelm + 1;

	q->freelist = mem_freelist_create(q->num_nodes, 1,
					  sizeof(struct lf_queue_node));
	if (!q->freelist) {	/* errno set by mem_create_freelist() */
		free(q);
		return NULL;
	}

	q->head = q->tail = mem_new(q->freelist);
	if (!q->head) {
		mem_freelist_destroy(q->freelist);
		free(q);
		return NULL;
	}
	mem_incr_ref(q->head);
	NEXT(q->tail) = NULL;

	return q;
}

void lf_queue_destroy(struct lf_queue *queue)
{
/*
	while (!lf_queue_empty(queue))
		lf_queue_dequeue(queue);

	assert(!NEXT(queue->head));
	assert(queue->head = queue->tail);
*/
	mem_release(queue->freelist, queue->head);
	mem_release(queue->freelist, queue->tail);
	mem_freelist_destroy(queue->freelist);

	free(queue);
}
