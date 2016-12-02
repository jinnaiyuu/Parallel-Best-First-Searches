/* © 2014 the PBNF Authors under the MIT license. See AUTHORS for the list of authors.*/

/**
 * \file h_zero.h
 *
 *
 *
 * \author Ethan Burns
 * \date 2008-10-13
 */

#if !defined(_H_ZERO_H_)
#define _H_ZERO_H_

#include "heuristic.h"

class State;

/**
 * An abstract heuristic function.
 */
class HZero : public Heuristic {
public:
	HZero(const SearchDomain *d);
	virtual fp_type compute(State *s) const;
};


#endif	/* !_H_ZERO_H_ */
