/* © 2014 the PBNF Authors under the MIT license. See AUTHORS for the list of authors.*/

/**
 * \file mem.h
 *
 * Implementation for lock-free memory management from ``Correction of
 * a Memory Management Method for Lock-Free Data Structures'' -- By
 * M. Michael and M. Scott. (refer to Figure 6)
 *
 * \author eaburns
 * \date 2009-03-17
 */
#if !defined(_MEM_H_)
#define _MEM_H_

#include <stdint.h>

/**
 * Test if a pointer is marked
 */
#define IS_MARKED(p) (((intptr_t) (p)) & 0x1)

/**
 * Get the marked version of a pointer
 */
#define GET_MARKED(p) ((void*) (((intptr_t) (p)) | 0x1))

/**
 * Get the marked version of a pointer
 */
#define GET_UNMARKED(p) ((void*) (((intptr_t) (p)) & (~0x1)))

/**
 * A reference which has been deleted
 */
#define DELETED_REFERENCE (GET_MARKED(NULL))

struct node {
	intptr_t refct_claim;

	/* Links that can be used when nodes must have pointers to
	 * other nodes in data structure implementations. */
	struct node **links;
};

struct freelist {
	struct node *head;

	/* Number of links per element. */
	size_t nlinks;

	/* Number of nodes which are currently free. */
	size_t nfreed;

	/* Number of allocated nodes. */
	size_t nalloced;
};

/**
 * Create a new free list of nodes.  Nodes can be over-allocated using
 * the elmsize parameter.  This allows the user to have extra data in
 * each node using casting tricks:
 *
 * \param nbrelm The number of elements in the list.
 *
 * \param nlinks The number of links in each node (allocated
 *               seperately from the node structure itself).
 *
 * \param elmsize The size of each element (should be at least
 *                sizeof(struct node).
 *
 * \return A freelist pointer on success or NULL on error and errno is
 *         set:
 *         - ENOMEM: Unable to allocate memory for the free list.
 *         - EINVAL: The elmsize parameter is too small.
 *         - EINVAL: The nbrelm parameter is zero.
 */
struct freelist *mem_freelist_create(size_t nbrelm,
				     size_t nlinks,
				     size_t elmsize);

/**
 * Destroy the free list, deallocating all of the memory for the nodes
 * which are currently on it.
 *
 * \return The number of elements that were freed.
 */
size_t mem_freelist_destroy(struct freelist *fl);

/**
 * Allocate a new element off of the free list.
 *
 * \return A new node on success or NULL on error and errno is set.
 *         - ENOMEM: No nodes left on the free list.
 */
void *mem_new(struct freelist *fl);

/**
 * Increment the reference counter of a node and get a pointer to it.
 *
 * \param p Must be a (struct node **) cast into a (void *).
 */
void *mem_safe_read(struct freelist *fl, void *p);

/**
 * Same as mem_safe_read, but returns NULL if the node that is read is
 * marked.
 */
void *mem_safe_read_null_on_mark(struct freelist *fl, void *p);

/**
 * Increment the reference counter of a node.  You should only call
 * this if you already have a pointer to the node that you know won't
 * change.
 *
 * This function can be safely called on a marked reference.
 *
 * \param p Must be a (struct node *) cast into a (void *).
 */
void mem_incr_ref(void *p);

/**
 * Release a node (decrementing the ref-counter and possibly
 * reclaiming it). If the node is reclaimed the links[] array is
 * released recursively starting from links[0] until a NULL link is
 * found.  If a link is marked as a DELETED_REFERENCE it is not
 * considered NULL.
 *
 * This function can be safely called on a marked reference.
 *
 * \param p Must be a (struct node *) cast into a (void *).
 */
void mem_release(struct freelist *fl, void *p);

#endif	/* !_MEM_H_ */
