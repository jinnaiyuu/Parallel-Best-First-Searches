/* © 2014 the PBNF Authors under the MIT license. See AUTHORS for the list of authors.*/

/**
 * \file grid_state.h
 *
 *
 *
 * \author Ethan Burns
 * \date 2008-10-08
 */

#if !defined(_TSP_STATE_H_)
#define _TSP_STATE_H_

#include <vector>

#include "tsp.h"
#include "../state.h"
#include "../search_domain.h"

using namespace std;

// TspState contains information of 
// where already visited (bool[], visited) and where it is now (int, visiting).
// Heuristic is given by the solution of Minimum spanning tree which is relaxed problem of Tsp.
// Zobrist hashing would be given by bool[] and int.
class TspState : public State {
public:
	TspState(Tsp *d, State *parent, fp_type c, fp_type g, vector<unsigned int>* visited_);

	virtual bool is_goal(void); // All city already visited and in the home city.
	virtual uint64_t hash(void) const; // Can make hash with two integer, visited[] and visiting.
	virtual State *clone(void) const; 
	virtual void print(ostream &o) const;
	virtual bool equals(State *s) const; // visited and visiting

	// as the number of the cities in 
	virtual vector<unsigned int> get_visited(void) const;
//	virtual int get_visiting(void) const;

	// zobrist hash can be given by visited and visiting.
//	void init_zbrhash(void) {;};
	unsigned int zbrhash(void);
private:
	vector<unsigned int> visited;
};

#endif	/* !_Tsp_STATE_H_ */
