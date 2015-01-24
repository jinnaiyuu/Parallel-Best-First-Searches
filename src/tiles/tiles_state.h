/* © 2014 the PBNF Authors under the MIT license. See AUTHORS for the list of authors.*/

/**
 * \file tiles_state.h
 *
 *
 *
 * \author Ethan Burns
 * \date 2008-11-03
 */

#if !defined(_TILES_STATE_H_)
#define _TILES_STATE_H_

#include <vector>

#include "../search_domain.h"
#include "../state.h"

using namespace std;

class TilesState : public State {
public:
	TilesState(SearchDomain *d, State *parent, fp_type c, fp_type g,
		   vector<unsigned int> tiles, unsigned int blank);

	TilesState(SearchDomain *d, State *parent, fp_type c, fp_type g,
		   fp_type h, vector<unsigned int> tiles,
		   unsigned int blank);

	virtual bool is_goal(void);
	virtual uint64_t hash(void) const;
	virtual State *clone(void) const;
	virtual void print(ostream &o) const;
	virtual bool equals(State *s) const;

	const vector<unsigned int> *get_tiles(void) const;
	unsigned int get_blank(void) const;
	void init_zbrhash(void);

	unsigned int dist_hash(void); // compute hash for distribution

private:
	// Hash for closed list
	void compute_closed_hash(void);
	uint64_t perfect_hash(unsigned int n_threads); // closedh = 0


	// Hash for distribution
	unsigned int zobrist_hash(void); // disth = 0
	unsigned int perf_residual_hash(void); // disth = 1

	vector<unsigned int> tiles;
	unsigned int blank;
	uint64_t hash_val;

	// This should be put to tiles.h.
	static unsigned int size;
	static unsigned int zbr_table[25][25]; // The size isn't that problem.
};


#endif	/* !_TILES_STATE_H_ */
