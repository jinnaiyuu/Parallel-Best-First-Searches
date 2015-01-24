// © 2014 the PBNF Authors under the MIT license. See AUTHORS for the list of authors.

/**
 * \file closed_list.cc
 *
 *
 *
 * \author Ethan Burns
 * \date 2008-10-09
 */

#include <assert.h>
#include <stdlib.h>

#include "util/hash_table.h"
#include "state.h"
#include "closed_list.h"

ClosedList::ClosedList(void)
{
}

ClosedList::ClosedList(unsigned long s)
	: tbl(s)
{
}


ClosedList::~ClosedList(void)
{
	prune();
}

/**
 * Add to the closed list.
 * \param s The state to add.
 */
void ClosedList::add(State *s)
{
	tbl.add(s);
}

/**
 * Lookup a state in the closed list.
 * \param c The state to look up.
 * \return The state if it was found, NULL on error.
 */
State *ClosedList::lookup(State *c)
{
	return tbl.lookup(c);
}

/**
 * Delete (free up the memory for) all states in the closed list.
 */
void ClosedList::delete_all_states(void)
{
	list<State*>::iterator iter;
	tbl.delete_all();

	for (iter = nodes.begin(); iter != nodes.end(); iter++)
		delete *iter;
}

/**
 * Drop all states, but don't delete them.
 */
void ClosedList::prune(void)
{
	tbl.prune();
}

bool ClosedList::empty()
{
	return tbl.empty();
}

/**
 * Removes all nodes that do not have their 'open' flag or 'incons'
 * flag set.
 */
void ClosedList::remove_closed_nodes()
{
	list<State*>::iterator iter;
	list<State*> closed;

	tbl.for_each(__remove_closed, (void*) &closed);

	for (iter = closed.begin(); iter != closed.end(); iter++)
		tbl.remove(*iter);

	nodes.splice(nodes.begin(), closed);
}

void ClosedList::__remove_closed(void *aux, State *s)
{
	list<State*> *nodes = (list<State*>*) aux;

	if (s->open || s->incons)
		return;
	else
		nodes->push_front(s);
}
