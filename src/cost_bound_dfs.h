/* © 2014 the PBNF Authors under the MIT license. See AUTHORS for the list of authors.*/

/**
 * \file cost_bound_dfs.h
 *
 *
 *
 * \author Ethan Burns
 * \date 2008-10-10
 */

#if !defined(_COST_BOUND_DFS_H_)
#define _COST_BOUND_DFS_H_

#include <vector>

#include "state.h"
#include "search.h"

using namespace std;

/**
 * A Depth First Search that is bounded on node cost.
 */
class CostBoundDFS : public Search {
public:
	CostBoundDFS(fp_type bound);

	virtual vector<State *> *search(Timer *, State *);

	fp_type get_min_pruned(void) const;
private:
	bool is_cycle(State *) const;

	fp_type bound;
	fp_type min_pruned;
};

#endif	/* !_COST_BOUND_DFS_H_ */
