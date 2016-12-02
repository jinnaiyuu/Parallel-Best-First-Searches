/* © 2014 the PBNF Authors under the MIT license. See AUTHORS for the list of authors.*/

/**
 * \file hashtbl.c
 *
 * A simple fixed size hash table using an array of lf_ordlist
 * buckets.
 *
 * \author eaburns
 * \date 2009-04-10
 */

#define _POSIX_C_SOURCE 200112L

#include <assert.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>

#include "atomic.h"
#include "lockfree.h"		/* for lf_ordlist */

struct lf_hashtbl {
	uint64_t (*hash)(void*);
	int (*cmp)(void *, void*);

	size_t e_per_bucket;
	size_t nbuckets;
	struct lf_ordlist **buckets;
};

struct lf_hashtbl *lf_hashtbl_create(size_t nbuckets,
				     size_t e_per_bucket,
				     int (*cmp)(void *, void*),
				     uint64_t (*hash)(void*))
{
	struct lf_hashtbl *tbl;

	tbl = calloc(1, sizeof(*tbl));
	if (!tbl)
		return NULL;
	tbl->hash = hash;
	tbl->cmp = cmp;
	tbl->e_per_bucket = e_per_bucket;
	tbl->nbuckets = nbuckets;
	tbl->buckets = calloc(nbuckets, sizeof(*tbl->buckets));
	if (!tbl->buckets)
		goto err_buckets;

	return tbl;

err_buckets:
	free(tbl);
	return NULL;
}

void lf_hashtbl_destroy(struct lf_hashtbl *tbl)
{
	int i;

	for (i = 0; i < tbl->nbuckets; i += 1) {
		if (tbl->buckets[i])
			lf_ordlist_destroy(tbl->buckets[i]);
	}

	free(tbl->buckets);
	free(tbl);
}

void *lf_hashtbl_add(struct lf_hashtbl *tbl, void *elm)
{
/* 	int b = tbl->hash(elm) % tbl->nbuckets; */

/* 	assert(elm); */

/* 	if (!tbl->buckets[b]) { */
/* 		struct lf_ordlist *l = lf_ordlist_create(tbl->e_per_bucket, */
/* 							 tbl->cmp); */

/* 		if (!compare_and_swap(&tbl->buckets[b], */
/* 				      (intptr_t) NULL, */
/* 				      (intptr_t) l)) */
/* 			lf_ordlist_destroy(l); */
/* 	} */

/* 	return lf_ordlist_add(tbl->buckets[b], elm); */
	return lf_hashtbl_cond_update(tbl, NULL, elm);
}

void *lf_hashtbl_cond_update(struct lf_hashtbl *tbl, int (*f)(void *, void *),
			     void *elm)
{
	int b = tbl->hash(elm) % tbl->nbuckets;

	assert(elm);

	if (!tbl->buckets[b]) {
		struct lf_ordlist *l = lf_ordlist_create(tbl->e_per_bucket,
							 tbl->cmp);

		if (!compare_and_swap(&tbl->buckets[b],
				      (intptr_t) NULL,
				      (intptr_t) l))
			lf_ordlist_destroy(l);
	}

	return lf_ordlist_cond_update(tbl->buckets[b], f, elm);
}

void *lf_hashtbl_lookup(struct lf_hashtbl *tbl, void *elm)
{
	int b = tbl->hash(elm) % tbl->nbuckets;
	void *e = NULL;

	if (tbl->buckets[b])
		e = lf_ordlist_find(tbl->buckets[b], elm);

	return e;
}
