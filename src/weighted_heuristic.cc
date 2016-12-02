// © 2014 the PBNF Authors under the MIT license. See AUTHORS for the list of authors.

/**
 * \file weighted_heuristic.cc
 *
 *
 *
 * \author Ethan Burns
 * \date 2008-10-27
 */

#include "weighted_heuristic.h"
#include "state.h"

WeightedHeuristic::WeightedHeuristic(const SearchDomain *d,
				     const Heuristic *h,
				     fp_type w)
	: Heuristic(d), weight(w), heuristic(h) {}


WeightedHeuristic::~WeightedHeuristic(void) {}


fp_type WeightedHeuristic::compute(State *s) const
{
	return weight * heuristic->compute(s);
}
