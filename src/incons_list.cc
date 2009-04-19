/**
 * \file incons_list.cc
 *
 *
 *
 * \author eaburns
 * \date 2009-04-19
 */
#include <assert.h>
#include <stdlib.h>

#include "util/hash_table.h"
#include "state.h"
#include "incons_list.h"

InconsList::InconsList(void)
{
}

InconsList::InconsList(unsigned long s)
	: tbl(s)
{
}

InconsList::~InconsList(void)
{
	tbl.prune();
}

/**
 * Add to the closed list.
 * \param s The state to add.
 */
void InconsList::add(State *s)
{
	if (s->incons)
		return;
	tbl.add(s);
	s->incons = true;
}

/**
 * Lookup a state in the closed list.
 * \param c The state to look up.
 * \return The state if it was found, NULL on error.
 */
State *InconsList::lookup(State *c)
{
	return tbl.lookup(c);
}

bool InconsList::empty()
{
	return tbl.empty();
}

/**
 * Drop all states, but don't delete them.
 */
void InconsList::re_open(OpenList *o)
{
	tbl.for_each(__do_re_open, (void*) o);
	tbl.prune();
}

void InconsList::__do_re_open(void *aux, State *s)
{
	OpenList *o = (OpenList*)aux;
	s->incons = false;
	o->add(s);
}