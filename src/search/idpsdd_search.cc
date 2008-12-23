/* -*- mode:linux -*- */
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

vector<const State *> *IDPSDDSearch::search(const State *init)
{
	vector <const State *> *path = NULL;
	float old_bound, bound = init->get_f();

	do {
		PSDDSearch psdd(n_threads, bound);
//		cout << "bound: " << bound << endl;

		path = psdd.search(init->clone());
		set_expanded(get_expanded() + psdd.get_expanded());
		set_generated(get_generated() + psdd.get_generated());
		old_bound = bound;
		bound = psdd.get_lowest_out_of_bounds();
	} while ((!path || path->at(0)->get_f() > old_bound) && bound != -1);

	delete init;

	return path;
}