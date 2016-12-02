// © 2014 the PBNF Authors under the MIT license. See AUTHORS for the list of authors.

/**
 * \file dynamic_bounded_psdd.cc
 *
 *
 *
 * \author Ethan Burns
 * \date 2008-10-27
 */

#include <vector>

#include "search_domain.h"
#include "state.h"
#include "heuristic.h"
#include "astar.h"
#include "weighted_heuristic.h"
#include "psdd_search.h"
#include "dynamic_bounded_psdd.h"

using namespace std;
using namespace PSDD;

DynamicBoundedPSDD::DynamicBoundedPSDD(unsigned int n_threads, fp_type weight)
	: n_threads(n_threads), weight(weight) {}


DynamicBoundedPSDD::~DynamicBoundedPSDD(void) {}


/**
 * Perform two searches.  First, a weighted A* to get a bound, then a
 * PSDD search using the weighted solution cost as the bound.
 */
vector<State *> *DynamicBoundedPSDD::search(Timer *t, State *init)
{
	SearchDomain *d = init->get_domain();
	Heuristic *h = d->get_heuristic();
	WeightedHeuristic wh(d, h, weight);
	AStar astar;
	PSDDSearch psdd(n_threads);
	vector<State *> *path;

	cerr << "Starting wA*" << endl;

	d->set_heuristic(&wh);
	path = astar.search(t, init->clone());
	d->set_heuristic(h);

	cerr << "wA* Finished" << endl;

	if (!path)
		return NULL;

	psdd.set_bound(path->at(0)->get_g());
	cerr << "Bound: " << path->at(0)->get_g() << endl;

	for (unsigned int i = 0; i < path->size(); i += 1)
		delete path->at(i);
	delete path;

	path = psdd.search(t, init);

	set_expanded(psdd.get_expanded() + astar.get_expanded());
	set_generated(psdd.get_generated() + astar.get_generated());

	return path;
}
