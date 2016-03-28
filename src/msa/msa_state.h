/* © 2014 the PBNF Authors under the MIT license. See AUTHORS for the list of authors.*/

/**
 * \file msa_state.h
 *
 *
 *
 * \author Ethan Burns
 * \date 2008-10-08
 */

#if !defined(_MSA_STATE_H_)
#define _MSA_STATE_H_

#include <vector>

#include "msa_world.h"
#include "../state.h"
#include "../search_domain.h"

using namespace std;

class MSAState : public State {
public:
	MSAState(MSAWorld *d, State *parent, fp_type c, fp_type g, vector<unsigned int> sequence);

	virtual bool is_goal(void);
	virtual uint128_t hash(void) const;
	virtual State *clone(void) const;
	virtual void print(ostream &o) const;
	virtual bool equals(State *s) const;

	void init_zbrhash(void);
	unsigned int dist_hash(void);
	std::vector<unsigned int> sequence;

private:
	unsigned int zobrist_hash(void); // disth = 0


	static unsigned int zbr_table[10][200];
//	int h;
};

#endif	/* !_GRID_STATE_H_ */
