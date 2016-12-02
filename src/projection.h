/* © 2014 the PBNF Authors under the MIT license. See AUTHORS for the list of authors.*/

/**
 * \file projection.h
 *
 *
 *
 * \author Ethan Burns
 * \date 2008-10-15
 */

#if !defined(_PROJECTION_H_)
#define _PROJECTION_H_

#include <vector>

class State;

using namespace std;

/**
 * An abstract projection function class.
 */
class Projection {
public:
	virtual ~Projection();

	/**
	 * Project a state, returning an integer that represents the
	 * NBlock that the state projects into.
	 */
	virtual unsigned int project(State *s) const = 0;

	/**
	 * Get the number of NBlocks that will be used in this
	 * projection.  NBlocks will be numbered from
	 * 0..num_nblocks() - 1
	 */
	virtual unsigned int get_num_nblocks(void) const = 0;

	/**
	 * Get the list of successor NBlock numbers.
	 */
	virtual vector<unsigned int>get_successors(unsigned int b) const = 0;

	/**
	 * Get the list of predecessor NBlock numbers.
	 */
	virtual vector<unsigned int>get_predecessors(unsigned int b) const = 0;
};

#endif	/* !_PROJECTION_H_ */
