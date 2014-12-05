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
#include <stdio.h>

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
vector<State *> *Search::expand(State *s, int thread_id)
{
	vector<State *> *children;
	//	printf("Search::expd %d\n", thread_id);
	// This function is run in all situation.
	children = s->expand(thread_id);

	// Intentional Needless operation for delaying the search.
	useless_counter += useless_calc(useless_counter);

	// These atomic operation can be a performance issue.
       	expanded.inc();
	generated.add(children->size());

	return children;
}

vector<State *> *Search::expand(State *s)
{
  expand(s, -1);
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

int Search::useless_calc(int useless) {
  int test = 0;
  int uselessLocal = 0;
  for (int i = 0; i < delay; ++i) {
    ++test;
    int l = 0;
    l += useless;
    l = 2 * l - uselessLocal;
    if (l != 0) {
      uselessLocal = l % useless;
    } else {
      uselessLocal = useless * 2;
    }
  }
  //  printf("uselessLocal = %d\n", uselessLocal);
  return uselessLocal;
}

int Search::get_useless() {
  return useless_counter;
}
