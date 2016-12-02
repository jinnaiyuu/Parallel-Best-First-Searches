// © 2014 the PBNF Authors under the MIT license. See AUTHORS for the list of authors.

/**
 * \file idpsdd_search.cc
 *
 *
 *
 * \author Ethan Burns
 * \date 2008-12-22
 */

#include "idpsdd_search.h"
#include "psdd_search.h"
#include "state.h"

IDPSDDSearch::IDPSDDSearch(unsigned int n_threads)
	: n_threads(n_threads) {}

vector<State *> *IDPSDDSearch::search(Timer *t, State *init)
{
	vector <State *> *path = NULL;
	fp_type old_bound, bound = init->get_f();

	PSDDSearch psdd(n_threads, bound);
	psdd.do_not_print();
	do {
//		cout << "bound: " << bound << endl;

		path = psdd.search(t, init->clone());
		set_expanded(get_expanded() + psdd.get_expanded());
		set_generated(get_generated() + psdd.get_generated());
		old_bound = bound;
		bound = psdd.get_lowest_out_of_bounds();
		psdd.reset();
		psdd.set_bound(bound);
	} while ((!path || path->at(0)->get_f() > old_bound) && bound != 0);

	delete init;

	return path;
}
