/* © 2014 the PBNF Authors under the MIT license. See AUTHORS for the list of authors.*/

/**
 * \file prio_queue.c
 *
 * A lock-free priority queue from: ``Fast and Lock-Free Concurrent
 * Priority Queues for Multi-Thread Systems'' -- By H. Sundell and
 * P. Tsigas
 *
 * \author eaburns
 * \date 2009-03-27
 */

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "backoff.h"
#include "atomic.h"
#include "mem.h"

/**
 * The max number of levels in the skiplist.
 */
#define MAX_LEVEL 24

/**
 * Get the next node at the given level.
 */
#define NEXT(node, level) (((struct lf_pq_node *)(node))->n.links[(level) + 1])

/**
 * Get the previous node (used in deletion).
 */
#define PREV(node) (((struct lf_pq_node *)(node))->n.links[0])

struct lf_pq_node {
	struct node n;
	int level;
	int valid_level;
	void *value;
	void *key;
};

struct lf_pq {
	int (*pred)(void *a, void *b);
	struct freelist *freelist;
	struct lf_pq_node *head;
	struct lf_pq_node *tail;
	size_t num_nodes;
	unsigned int seed;
};

static struct lf_pq_node *help_delete(struct lf_pq *pq,
				      struct lf_pq_node *node,
				      int level);
/**
 * Release a reference to a node.
 *
 * This is just a more convenient way of calling mem_release and it is
 * consistent with the function name used in the Sundell's paper.
 */
static inline void RELEASE_NODE(struct lf_pq *pq, void *_n)
{
	struct lf_pq_node *n = _n;

	mem_release(pq->freelist, n);
}

/**
 * Read a node from a node pointer pointer.
 *
 * This is just a more convenient way of calling mem_safe_read and it
 * is consistent with the function name used in the Sundell's paper.
 */
static inline struct lf_pq_node *READ_NODE(struct lf_pq *pq, void *n)
{
	return mem_safe_read_null_on_mark(pq->freelist, n);
}


/**
 * Increase the reference counter of the given node.
 *
 * This is just a more convenient way of calling mem_incr_ref and it
 * is consistent with the function name used in the Sundell's paper.
 */
static inline struct lf_pq_node *COPY_NODE(void *_n)
{
	struct lf_pq_node *n = _n;

	mem_incr_ref((struct node *) n);
	return n;
}

/**
 * A predecessor function with tie breaking on the node pointer
 * addresses.
 */
static int is_pred(struct lf_pq *pq, struct lf_pq_node *a, struct lf_pq_node *b)
{
	int p = pq->pred(GET_UNMARKED(a->key), GET_UNMARKED(b->key));

	if (p == pq->pred(GET_UNMARKED(b->key), GET_UNMARKED(a->key))) {
		/* Nodes have equal priority. */
		return a > b;
	}

	return p;
}

/**
 * Traverse to the next element in the skiplist at the given level,
 * deleting things as we pass (if they need it).
 *
 * \param pq The prio queue.
 *
 * \param node1 A pointer to a pointer to the node forwhich we want to
 *              get the following node.  This node is also a return
 *              parameter in which the previous node of the return
 *              value of this function is stored.  If node1 is not
 *              marked for deletion then it will just be the same as
 *              the node passed in, but if deletion happens, it may be
 *              a new node.
 *
 * \param level The level.
 *
 * \return The next node.
 *
 * \note This function assumes that node1 will never get marked (the
 *       node1 parameter is dereferenced freely in this function).
 *       This means that node1 should be the address of a local
 *       variable, *not* a location within a node in the structure
 *       that may get marked.
 */
static struct lf_pq_node *read_next(struct lf_pq *pq,
				    struct lf_pq_node **node1,
				    int level)
{
	struct lf_pq_node *node2;

	assert(pq->head->level == MAX_LEVEL);
	assert(pq->tail->level == MAX_LEVEL);

	assert(!IS_MARKED(*node1));

	if (IS_MARKED((*node1)->value))
		*node1 = help_delete(pq, *node1, level);

	node2 = READ_NODE(pq, &NEXT(*node1, level));
	while (!node2) {
		*node1 = help_delete(pq, *node1, level);
		node2 = READ_NODE(pq, &NEXT((*node1), level));
	}

	assert(!IS_MARKED(node2));

	return node2;
}

/**
 * Scan for the node with the key or a greater key.
 *
 * \param pq The prio queue.
 *
 * \param node1 A pointer to a pointer to the node forwhich we want to
 *              get the following node.  This node is also a return
 *              parameter in which the previous node of the return
 *              value of this function is stored.  If node1 is not
 *              marked for deletion then it will just be the same as
 *              the node passed in, but if deletion happens, it may be
 *              a new node.
 *
 * \param level The level.
 *
 * \param key The value of a node forwhich we want to find the
 *            insertion spot.
 *
 * \return The node with the key greater than the given key, or the
 *         tail node.  The returned node will be reference and *node1
 *         will point to the node before the returned node and it will
 *         have a reference too.
 *
 * \note node1 should be referenced coming into this function.
 *
 * \note This function assumes that node1 will never get marked (the
 *       node1 parameter is dereferenced freely in this function).
 *       This means that node1 should be the address of a local
 *       variable, *not* a location within a node in the structure
 *       that may get marked.
 */
static struct lf_pq_node *scan_key(struct lf_pq *pq,
				   struct lf_pq_node **node1,
				   int level,
				   struct lf_pq_node *key)
{
	struct lf_pq_node *node2;

	assert(!IS_MARKED(*node1));

	node2 = read_next(pq, node1, level);
	while (node2 != pq->tail && is_pred(pq, node2, key)) {
		/* Releasing a marked node is okay. */
		RELEASE_NODE(pq, *node1);
		*node1 = node2;
		node2 = read_next(pq, node1, level);
	}

	assert(!IS_MARKED(node2));
	assert(node2 == pq->tail || node2 == key || is_pred(pq, key, node2));

	return node2;
}

/**
 * Remove a node from the structure at the given level.
 *
 * \note **prev should be referenced coming into this function and the
 *       reference is still held upon the return.

 * \note node should be referenced coming into this function and the
 *       reference is still held upon the return.
 */
static void remove_node(struct lf_pq *pq, struct lf_pq_node *node,
			struct lf_pq_node **prev, int level)
{
	struct lf_pq_node *last;

	assert(IS_MARKED(node->value));

	backoff_decrease();

	for ( ; ; ) {
		if (NEXT(node, level) == DELETED_REFERENCE)
			break;
		last = scan_key(pq, prev, level, node);
		assert(last == pq->tail || last == node || is_pred(pq, node, last));
		assert (*prev != pq->tail);
		RELEASE_NODE(pq, last);

		if (last != node || NEXT(node, level) == DELETED_REFERENCE)
			break;

		if (compare_and_swap(&NEXT(*prev, level), (intptr_t) node,
				     (intptr_t) GET_UNMARKED(NEXT(node,
								  level)))) {
			/* If the CAS succeeded, we lost a reference
			 * to node from NEXT(prev, level), but we
			 * don't gain a reference to NEXT(node,level)
			 * since we flag it as a deleted reference in
			 * NEXT(node, level). */
			NEXT(node, level) = DELETED_REFERENCE;
			RELEASE_NODE(pq, node);
			break;
		}

		backoff();
	}
}

/**
 * Help delete a node.
 *
 * \note node must be referenced coming into this function and the
 *       reference is released before this function returns.
 */
static struct lf_pq_node *help_delete(struct lf_pq *pq,
				      struct lf_pq_node *node,
				      int level)
{
	int i;
	struct lf_pq_node *prev, *node2;

	assert(level <= node->level);
	assert(IS_MARKED(node->value));

	for (i = level; i < node->level; i += 1) {
		assert(i < MAX_LEVEL);
		do {
			node2 = (struct lf_pq_node *) NEXT(node, i);
		} while (!IS_MARKED(node2)
			 && !compare_and_swap(&NEXT(node, i), (intptr_t) node2,
					      (intptr_t) GET_MARKED(node2)));
	}

	prev = GET_UNMARKED(PREV(node));
	if (!prev || level >= prev->valid_level) {
		prev = COPY_NODE(pq->head);
		for (i = MAX_LEVEL - 1; i >= level; i -= 1) {
			node2 = scan_key(pq, &prev, i, node);
			RELEASE_NODE(pq, node2);
		}
	} else {
		COPY_NODE(prev);
	}

	remove_node(pq, node, &prev, level);
	RELEASE_NODE(pq, node);

	return prev;
}

/**
 * Get a random level number.
 *
 * NOTE0: This may be re-entrant, but it is not thread safe!!!!
 * NOTE1: This is used in an unsafe manor by many threads :)
 */
static int random_level(struct lf_pq *pq)
{
/*
	int flip;
*/
	int count = 0;
	long result;

/*
	do {
		lrand48_r(&pq->rand_buffer, &result);
		flip = result % 2;
		count += 1;
	} while (flip && count < MAX_LEVEL - 1);
*/

	/* From lockfree-lib... apparently has 0.5 drop off rate
	 * per-level.  Looks more efficient than the previous
	 * implementation.  Gets a number between 1 <= level <=
	 * MAX_LEVEL. */
	result = rand_r(&pq->seed);
	result = (result >> 4) & ((1 << (MAX_LEVEL - 1)) - 1);
	while (result & 0x1) {
		result >>= 1;
		count += 1;
	}

	return count;
}

/**
 * Create a new node, with one reference to it.
 */
static struct lf_pq_node *create_node(struct lf_pq *pq, int level, void *value, void *key)
{
	struct lf_pq_node *n = mem_new(pq->freelist);
	if (!n)
		return NULL;

	n->level = level;
	n->value = value;
	n->key = key;
	n->valid_level = 0;
	PREV(n) = DELETED_REFERENCE;

	return n;
}

/**
 * This is in no way thread safe.
 */
int lf_pq_print(FILE *f, struct lf_pq *pq)
{
	int i;
	struct lf_pq_node *n;

	n = pq->head;
	while (n) {
		fprintf(f, "%p: val=%-10p refs=%-3d ", n, n->value, (int) n->n.refct_claim / 2);
		for (i = 0; i < n->level; i += 1) {
			fprintf(f, " %10p", n->n.links[i]);
		}
		fprintf(f, "\n");
		n = (struct lf_pq_node *) NEXT(n, 0);
	}

	return 0;
}

/**
 * Test that the priority queue holds.  If there is an error, some
 * info about it is printed to the given file.
 *
 * This is in no way thread safe.
 *
 * This should not be called if there are marked nodes... it doesn't
 * check for that and will dereference them.
 */
int lf_pq_property_holds(FILE *f, struct lf_pq *pq)
{
	int i, pred;
	struct lf_pq_node *n, *p;

	n = pq->head;
	for (i = MAX_LEVEL - 1; i >= 0; i -= 1) {
		n = pq->head;
		while (n) {
			p = n;
			do {
				p = (struct lf_pq_node *) NEXT(p, i);
			} while (IS_MARKED(p));
			if (n != pq->head && p && p != pq->tail) {
				if (n->n.refct_claim & 0x1) {
					fprintf(f, "Error at level %d: "
						"freed node %p\n", i, n);
					return 0;
				}
				if (n != pq->tail
				    && (n->n.refct_claim / 2) != n->level) {
					fprintf(f, "Error at level %d: "
						"refs are wrong in node %p "
						" expected %d, has %d\n",
						i, n, n->level,
						(int) n->n.refct_claim / 2);
					return 0;
				}
				pred = is_pred(pq, p, n);
				if (pred && f) {
					fprintf(f, "Error at level %d: "
						"%p (%p) > %p (%p)\n",
						i, n, n->value, p, p->value);
				}
				if (pred)
					return 0;
			}
			n = p;
		}
	}

	return 1;
}

int lf_pq_insert(struct lf_pq *pq, void *val, void *key)
{
	int i, level;
	struct lf_pq_node *new_node;
	struct lf_pq_node *node1, *node2;
	struct lf_pq_node *saved_nodes[MAX_LEVEL];

	backoff_decrease();

	level = random_level(pq);
	new_node = create_node(pq, level, val, key);
	if (!new_node)
		return -1;

	assert(new_node->level == level);

	/*
	 * Find the node which the new node will be inserted before.
	 * Save the previous nodes at each level for later use.
	 */
	node1 = COPY_NODE(pq->head);
	for (i = MAX_LEVEL - 1; i >= 1; i -= 1) {
		node2 = scan_key(pq, &node1, i, new_node);
		RELEASE_NODE(pq, node2);
		if (i < level)
			saved_nodes[i] = COPY_NODE(node1);
	}

	assert(new_node->level == level);

	/*
	 * Insert the new node at level 0.
	 */
	for ( ; ; ) {
		node2 = scan_key(pq, &node1, 0, new_node);

		/* I left out a chunk of code here that overwrites the
		 * value of a node with the same key as the node being
		 * inserted.  We don't want this behavior. */

		/* new_node gets node2's reference from scan_key.  */
		NEXT(new_node, 0) = (struct node *) node2;

		/* Either the CAS will fail and NEXT(new_node, 0)
		 * won't point to node2 on the next try, or the CAS
		 * will succeed and NEXT(new_node,0) will get
		 * NEXT(node1,0)'s reference to node2. */
		RELEASE_NODE(pq, node2);

		COPY_NODE(new_node); /* For the CAS */
		if (compare_and_swap(&NEXT(node1, 0), (intptr_t) node2,
				     (intptr_t) new_node)) {
			/* The CAS released a reference to node2. */
			/* We are done with the reference from
			 * scan_key now. */
			RELEASE_NODE(pq, node1);
			break;
		}

		/* The reference to new_node (from the CAS) never
		 * stuck. */
		RELEASE_NODE(pq, new_node);

		backoff();
	}

	assert(new_node->level == level);

	/*
	 * Insert the new node at the rest of its levels.
	 */
	for (i = 1; i < level; i += 1) {
		new_node->valid_level = i;
		assert(new_node->valid_level <= level);

		/* We have a reference to this node from before.
		 * scan_key needs this to be referenced going in, so
		 * this is fine. */
		node1 = saved_nodes[i];

		for ( ; ; ) {
			int success;
			node2 = scan_key(pq, &node1, i, new_node);

			/* new_node gets node2's reference from scan_key. */
			NEXT(new_node, i) = (struct node *) node2;

			COPY_NODE(new_node); /* For the CAS. */
			success = 0;

			/* The following 'if' is *terrible* but it is
			 * what the alg calls for. */
			if (IS_MARKED(new_node->value)
			    || (success =
				compare_and_swap(&NEXT(node1, i),
						 (intptr_t) node2,
						 (intptr_t) new_node))) {
				if (success) {
					/* NEXT(node1, i) doesn't ref
					 * node2. */
					RELEASE_NODE(pq, node2);
				} else {
					/* CAS failed, NEXT(node1, i)
					 * doesn't reference
					 * new_node. */
					RELEASE_NODE(pq, new_node);
				}


				/* We are done with the reference from
				 * scan_key now. */
				RELEASE_NODE(pq, node1);
				break;
			}
			assert(!success);

			/* The CAS failed, so the reference to
			 * new_node never stuck. */
			RELEASE_NODE(pq, new_node);
			/* new_node won't point to node2 on the
			 * retry. */
#if !defined(NDEBUG)
			/* This *should* be unnecessary. */
			NEXT(new_node, i) = NULL;
#endif	/* !NDEBUG */
			RELEASE_NODE(pq, node2);

			backoff();
		}

	}

	assert(new_node->level == level);
/* #if !defined(NDEBUG) */
/* 	for (i = new_node->level; i < MAX_LEVEL; i += 1) */
/* 		assert(!NEXT(new_node, i)); */
/* #endif	/\* !NDEBUG *\/ */

	new_node->valid_level = level;
	if (IS_MARKED(new_node->value))
		new_node = help_delete(pq, new_node, 0);
	RELEASE_NODE(pq, new_node);

	return 0;
}

void *lf_pq_peek_min(struct lf_pq *pq)
{
	struct lf_pq_node *n;

	n = (struct lf_pq_node *) NEXT(pq->head, 0);

	if (n == pq->tail)
		return NULL;

	return n->value;
}

void *lf_pq_delete_min(struct lf_pq *pq)
{
	int i;
	void *value;
	struct lf_pq_node *prev, *node1, *node2;

	backoff_decrease();

	prev = COPY_NODE(pq->head);
	for ( ; ; ) {
		node1 = read_next(pq, &prev, 0);
		if (node1 == pq->tail) {
			RELEASE_NODE(pq, prev);
			RELEASE_NODE(pq, node1);
			return NULL;
		}
	retry:
		value = node1->value;
		if (node1 != (struct lf_pq_node *) NEXT(prev, 0)) {
			RELEASE_NODE(pq, node1);
			continue;
		}

		if (!IS_MARKED(value)) {
			if (compare_and_swap(&node1->value, (intptr_t) value,
					     (intptr_t) GET_MARKED(value))) {
				/* Takes our reference to prev. */
				assert(!GET_UNMARKED(PREV(node1)));

/**************************************************************
Commenting _in_ the "PREV(node1) = ..." line is what the acutal
algorithm calls for.  This allows threads to have shorter searches
when removing the node.  This appears to leak a reference somewhere,
so instead I just leave the PREV field NULL and release the node.
Deleting threads will have to search, but it is better than leaking a
reference.
*/
/*
				PREV(node1) = (struct node*) prev;
*/
				RELEASE_NODE(pq, prev);
/**************************************************************/

				break;
			} else {
				/* Since we don't replace values, this
				 * will only happen if someone marked
				 * node1->value. */
				assert(IS_MARKED(node1->value));

				backoff();
				goto retry;
			}
		}

		node1 = help_delete(pq, node1, 0);
		RELEASE_NODE(pq, prev);
		prev = node1;

		backoff();
	}

	for (i = 0; i < node1->level; i += 1 ) {
		assert(i < MAX_LEVEL);
		do {
			node2 = (struct lf_pq_node *) NEXT(node1, i);
		} while (!IS_MARKED(node2)
			 && !compare_and_swap(&NEXT(node1, i), (intptr_t) node2,
					      (intptr_t) GET_MARKED(node2)));
	}

	prev = COPY_NODE(pq->head);
	for (i = node1->level - 1; i >= 0; i -= 1)
		remove_node(pq, node1, &prev, i);

	RELEASE_NODE(pq, prev);
	RELEASE_NODE(pq, node1);

	return value;
}

int lf_pq_empty(struct lf_pq *pq)
{
	return ((struct lf_pq_node* ) NEXT(pq->head, 0)) == pq->tail;
}

struct lf_pq *lf_pq_create(size_t nbrelm, int (*pred)(void*, void*))
{
	int i;
	struct lf_pq *pq;

	pq = calloc(1, sizeof(*pq));
	if (!pq)
		return NULL;

	nbrelm += 2;		/* head and tail dummy nodes. */
	pq->num_nodes = nbrelm;
	pq->pred = pred;

	pq->seed = time(NULL);

	/* MAX_LEVEL + 1 for the PREV link. */
	pq->freelist = mem_freelist_create(nbrelm, MAX_LEVEL + 1,
					   sizeof(struct lf_pq_node));
	if (!pq->freelist)
		goto err_fl;

	pq->head = create_node(pq, MAX_LEVEL, NULL, NULL);
	if (!pq->head)
		goto err_head;
	pq->tail = create_node(pq, MAX_LEVEL, NULL, NULL);
	if (!pq->tail)
		goto err_tail;

	for (i = 0; i < MAX_LEVEL; i += 1)
		NEXT(pq->head, i) = (struct node *) COPY_NODE(pq->tail);

	return pq;

err_tail:
	mem_release(pq->freelist, pq->head);
err_head:
	mem_freelist_destroy(pq->freelist);
err_fl:
	free(pq);
	return NULL;
}

void lf_pq_destroy(struct lf_pq *pq)
{
	size_t nfreed;

	mem_release(pq->freelist, pq->head);
	mem_release(pq->freelist, pq->tail);

	nfreed = mem_freelist_destroy(pq->freelist);
	if (nfreed != pq->num_nodes) {
		fprintf(stderr, "nfreed=%d, pq->num_nodes=%d\n",
			(int) nfreed, (int) pq->num_nodes);
	}
	assert(nfreed == pq->num_nodes);

	free(pq);
}
