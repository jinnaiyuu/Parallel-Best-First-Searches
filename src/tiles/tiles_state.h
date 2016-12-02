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


	unsigned int dist_hash(int dist, int n_threads);

	void init_zbrhash(void);
	unsigned int zbrhash(void);
	unsigned int random_dist(void);
	unsigned int goha(int n_threads);

private:
	void compute_hash(void);
	vector<unsigned int> tiles;
	unsigned int blank;
	uint64_t hash_val;
	
	unsigned int zbr_table[16][16]; // adhoc
};

#endif	/* !_TILES_STATE_H_ */
