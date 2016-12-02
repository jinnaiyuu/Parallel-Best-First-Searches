// © 2014 the PBNF Authors under the MIT license. See AUTHORS for the list of authors.

/**
 * \file lf_closedlist.cc
 *
 *
 *
 * \author eaburns
 * \date 2009-04-12
 */

#include <assert.h>

#include "lf_closedlist.h"

extern "C" {
#include "lockfree/include/lockfree.h"
}

static int state_cmp_fun(void *a, void *b)
{
	State *sa = (State*)a;
	State *sb = (State*)b;

	if (!sa && !sb)
		return 0;
	if (!sa)
		return 1;
	if (!sb)
		return -1;

	return sa->hash() - sb->hash();
}

static uint64_t state_hash_fun(void *a)
{
	State *sa = (State*)a;

	return sa->hash();
}

LF_ClosedList::LF_ClosedList(void)
{
	tbl = lf_hashtbl_create(50000,
				500,
				state_cmp_fun,
				state_hash_fun);
}

LF_ClosedList::~LF_ClosedList(void)
{
	lf_hashtbl_destroy(tbl);
}

// STATIC
int LF_ClosedList::better(void *a, void *b)
{
	State *sa = (State*) a;
	State *sb = (State*) b;
	assert(a);
	assert(b);

	return sa->get_g() < sb->get_g();
}

State *LF_ClosedList::cond_update(State *s)
{
	return (State*) lf_hashtbl_cond_update(tbl, better, (void*) s);
}

void LF_ClosedList::add(State *s)
{
	lf_hashtbl_add(tbl, (void*) s);
}

State *LF_ClosedList::lookup(State *s)
{
	return (State*) lf_hashtbl_lookup(tbl, (void*) s);
}

void LF_ClosedList::delete_all_states(void)
{
	// Not implemented yet...
}
