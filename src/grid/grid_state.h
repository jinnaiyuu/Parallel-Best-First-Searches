/* © 2014 the PBNF Authors under the MIT license. See AUTHORS for the list of authors.*/

/**
 * \file grid_state.h
 *
 *
 *
 * \author Ethan Burns
 * \date 2008-10-08
 */

#if !defined(_GRID_STATE_H_)
#define _GRID_STATE_H_

#include <vector>

#include "grid_world.h"
#include "../state.h"
#include "../search_domain.h"

using namespace std;

class GridState : public State {
public:
	GridState(GridWorld *d, State *parent, fp_type c, fp_type g, int x, int y);

	virtual bool is_goal(void);
	virtual uint64_t hash(void) const;
	virtual State *clone(void) const;
	virtual void print(ostream &o) const;
	virtual bool equals(State *s) const;

	virtual int get_x(void) const;
	virtual int get_y(void) const;
	void init_zbrhash(void) {;};
	unsigned int dist_hash(int dist, int n_threads);

private:
	int x, y;
};

#endif	/* !_GRID_STATE_H_ */
