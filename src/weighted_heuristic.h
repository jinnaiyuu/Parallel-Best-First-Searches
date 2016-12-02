/* © 2014 the PBNF Authors under the MIT license. See AUTHORS for the list of authors.*/

/**
 * \file weighted_heuristic.h
 *
 *
 *
 * \author Ethan Burns
 * \date 2008-10-27
 */

#if !defined(_WEIGHTED_HEURISTIC_H_)
#define _WEIGHTED_HEURISTIC_H_

#include "state.h"
#include "heuristic.h"

class WeightedHeuristic : public Heuristic {
public:
	WeightedHeuristic(const SearchDomain *d, const Heuristic *h, fp_type w);
	virtual ~WeightedHeuristic(void);

	virtual fp_type compute(State *s) const;
private:
	fp_type weight;
	const Heuristic *heuristic;
};

#endif	/* !_WEIGHTED_HEURISTIC_H_ */
