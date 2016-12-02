/* © 2014 the PBNF Authors under the MIT license. See AUTHORS for the list of authors.*/

/**
 * \file heuristic.h
 *
 * A heuristic function.
 *
 * \author Ethan Burns
 * \date 2008-10-08
 */

#if !defined(_HEURISTIC_H_)
#define _HEURISTIC_H_

#include "util/fixed_point.h"

class SearchDomain;
class State;

/**
 * An abstract heuristic function.
 */
class Heuristic {
public:
	Heuristic(const SearchDomain *d);
	virtual ~Heuristic();

	virtual fp_type compute(State *s) const = 0;
	void set_weight(float w);
	fp_type get_weight(void) const;
protected:
	const SearchDomain *domain;
	fp_type weight;
};

#endif	/* !_HEURISTIC_H_ */
