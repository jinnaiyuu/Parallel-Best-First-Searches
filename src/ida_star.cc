// © 2014 the PBNF Authors under the MIT license. See AUTHORS for the list of authors.

/**
 * \file ida_star.cc
 *
 *
 *
 * \author Ethan Burns
 * \date 2008-10-10
 */

#include <vector>

#include "ida_star.h"
#include "cost_bound_dfs.h"

using namespace std;

void IDAStar::output_stats(void)
{
	list<iteration>::iterator i;

	cout << "cols: "
	     << "\"iter no\" "
	     << "\"cost bound\" "
	     << "\"nodes expanded\" "
	     << "\"nodes generated\" "
	     << "\"new expanded\""
	     << endl;
	for (i = iters.begin(); i != iters.end(); i++)
		cout << "row:"
		     << " " << (*i).no
		     << " " << DOUBLE((*i).bound)
		     << " " << (*i).exp
		     << " " << (*i).gen
		     << " " << (*i).new_exp << endl;
}

vector<State *> *IDAStar::search(Timer *t, State *init)
{
	vector<State *> *path = NULL;
	fp_type old_bound, bound = init->get_f();
	struct iteration iteration;
	unsigned long prev_iter_exp = 0;
	unsigned int iter_no = 0;

	do {
		CostBoundDFS dfs(bound);
#if !defined(NDEBUG)
		cout << "# bound = " << DOUBLE(bound) << endl;
#endif	// !NDEBUG

		path = dfs.search(t, init->clone());
		set_expanded(get_expanded() + dfs.get_expanded());
		set_generated(get_generated() + dfs.get_generated());

		if (!path) {
#if !defined(NDEBUG)
			cout << "# \t expanded = "
			     << dfs.get_expanded() << endl
			     << "# \tgenerated = "
			     << dfs.get_generated() << endl
			     << "# \t      new = "
			     << dfs.get_expanded() - prev_iter_exp << endl;
#endif	// !NDEBUG
			iter_no += 1;
			iteration.no = iter_no;
			iteration.bound = bound;
			iteration.exp = dfs.get_expanded();
			iteration.gen = dfs.get_generated();
			iteration.new_exp = iteration.exp - prev_iter_exp;
			iters.push_back(iteration);
			prev_iter_exp = iteration.exp;
		}

		old_bound = bound;
		bound = dfs.get_min_pruned();
	} while ((!path || path->at(0)->get_g() > old_bound) && bound != 0);

	delete init;

	return path;
}
