// © 2014 the PBNF Authors under the MIT license. See AUTHORS for the list of authors.

/**
 * \file heuristic.cc
 *
 *
 *
 * \author Ethan Burns
 * \date 2008-10-13
 */

#include "util/fixed_point.h"

#include "heuristic.h"

Heuristic::Heuristic(const SearchDomain *d) : domain(d), weight(fp_one) {}

Heuristic::~Heuristic() {}

void Heuristic::set_weight(float wt)
{
	weight = fp_one * wt;
}

fp_type Heuristic::get_weight(void) const
{
	return weight;
}
