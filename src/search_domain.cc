// © 2014 the PBNF Authors under the MIT license. See AUTHORS for the list of authors.

/**
 * \file search_domain.cc
 *
 *
 *
 * \author Ethan Burns
 * \date 2008-10-08
 */

#include <assert.h>

#include "search_domain.h"

/**
 * Create a new search domain.
 */
SearchDomain::SearchDomain()
	: heuristic(NULL),
	  project(NULL) {}

SearchDomain::~SearchDomain() {}

/**
 * Set the heuristic that will be used in this domain.
 * \param h The heuristic to add.
 */
void SearchDomain::set_heuristic(Heuristic *h)
{
	this->heuristic = h;
}

/**
 * Get the heuristic class.
 */
Heuristic *SearchDomain::get_heuristic(void) const
{
	return heuristic;
}

/**
 * Set the projection function.
 */
void SearchDomain::set_projection(const Projection *p)
{
	project = p;
}

/**
 * Get the current projection function.
 */
const Projection *SearchDomain::get_projection(void) const
{
	return project;
}

