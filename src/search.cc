// © 2014 the PBNF Authors under the MIT license. See AUTHORS for the list of authors.

/**
 * \file search.cc
 *
 *
 *
 * \author Ethan Burns
 * \date 2008-10-09
 */

#include <vector>

#include "util/atomic_int.h"
#include "state.h"
#include "search.h"

static Search *instance = NULL;

void output_search_stats_on_timeout(void)
{
	if (instance)
		instance->output_stats();
}

Search::Search(void) : expanded(0), generated(0)
{
	instance = this;
}

/**
 * Call the expand method of the given state and track stats on number
 * of generated/expanded nodes.
 * \param s The state to expand.
 * \return A newly allocated vector of the children states.  This must
 *         be deleted by the caller.
 */
vector<State *> *Search::expand(State *s)
{
	vector<State *> *children;

	children = s->expand();
	expanded.inc();
	generated.add(children->size());

	return children;
}

/**
 * Clear the expanded and generated counters.
 */
void Search::clear_counts(void)
{
	expanded.set(0);
	generated.set(0);
}

/**
 * Get the value of the expanded counter.
 */
unsigned long Search::get_expanded(void) const
{
	return expanded.read();
}

/**
 * Get the value of the generated counter.
 */
unsigned long Search::get_generated(void) const
{
	return generated.read();
}

/**
 * Set the expanded count.
 * \param e The value to set it to.
 */
void Search::set_expanded(unsigned long e)
{
	expanded.set(e);
}

/**
 * Set the generated count.
 * \param g The value to set it to.
 */
void Search::set_generated(unsigned long g)
{
	generated.set(g);
}


/**
 * Do nothing by default.
 */
void Search::output_stats(void)
{
}
