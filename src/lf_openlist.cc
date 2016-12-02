// © 2014 the PBNF Authors under the MIT license. See AUTHORS for the list of authors.

/**
 * \file lf_openlist.cc
 *
 *
 *
 * \author eaburns
 * \date 2009-04-12
 */

#include <assert.h>
#include <stdlib.h>


#include "util/fixed_point.h"
#include "lf_openlist.h"
#include "open_list.h"

extern "C" {
#include "lockfree/include/lockfree.h"
}

#define MAX_FILL 5000000

static int state_pred_fun(void *a, void *b)
{
	State *sa = (State*) a;
	State *sb = (State*) b;
	fp_type afp = sa->get_f_prime();
	fp_type bfp = sb->get_f_prime();

	if (afp == bfp) {
		fp_type af = sa->get_f();
		fp_type bf = sb->get_f();
		if (af == bf) {
			fp_type ag = sa->get_g();
			fp_type bg = sb->get_g();
			return ag > bg;
		}
		return af < bf;
	}

	return afp < bfp;
}

LF_OpenList::LF_OpenList(void)
 :OpenList(),
  fill(0)
{
	pq = lf_pq_create(MAX_FILL, state_pred_fun);
}

LF_OpenList::~LF_OpenList(void)
{
/*
	while (!lf_pq_empty(pq))
		lf_pq_delete_min(pq);
*/

	lf_pq_destroy(pq);
}

void LF_OpenList::add(State *s)
{
	int err;

	err = lf_pq_insert(pq, s, s);

	assert(!err);

	if (err)
		abort();

	fill.inc();
}

State *LF_OpenList::take(void)
{
	State *s;

	s = (State *) lf_pq_delete_min(pq);
	if (s)
		fill.dec();

	return s;
}

State *LF_OpenList::peek(void)
{
	return (State*) lf_pq_peek_min(pq);
}

bool LF_OpenList::empty(void)
{
	return lf_pq_empty(pq);
}

void LF_OpenList::delete_all_states(void)
{
	State *s;

	for ( ; ; ) {
		s = take();
		if (s)
			delete s;
		else
			break;
	}
}

void LF_OpenList::prune(void)
{
	State *s;

	do {
		s = take();
	} while (s);
}

unsigned int LF_OpenList::size(void)
{
	return fill.read();
}

list<State*> *LF_OpenList::states(void)
{
	assert("unimplemented" == NULL);
	return NULL;
}
