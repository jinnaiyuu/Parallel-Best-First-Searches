// © 2014 the PBNF Authors under the MIT license. See AUTHORS for the list of authors.

/**
 * \file h_zero.cc
 *
 *
 *
 * \author Ethan Burns
 * \date 2008-10-13
 */

#include "h_zero.h"

HZero::HZero(const SearchDomain *d) : Heuristic(d) {}

fp_type HZero::compute(State *s) const
{
	return 0.0;
}
