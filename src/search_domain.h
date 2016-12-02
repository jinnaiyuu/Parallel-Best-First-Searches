/* © 2014 the PBNF Authors under the MIT license. See AUTHORS for the list of authors.*/

/**
 * \file search_domain.h
 *
 * A search domain.
 *
 * \author Ethan Burns
 * \date 2008-10-08
 */
#if !defined(_SEARCH_DOMAIN_H_)
#define _SEARCH_DOMAIN_H_

#include <vector>
#include <iostream>

#include "projection.h"
#include "heuristic.h"

using namespace std;

class state;

/**
 * An abstract search domain.
 */
class SearchDomain {
public:
	SearchDomain();

	virtual ~SearchDomain();

	virtual void set_heuristic(Heuristic *h);
	virtual Heuristic* get_heuristic(void) const;
	virtual void set_projection(const Projection *p);
	virtual const Projection *get_projection(void) const;

	/* Abstract methods */
	virtual State *initial_state(void) = 0;
	virtual vector<State*> *expand(State *s) = 0;
private:
	Heuristic *heuristic;
	const Projection *project;
};

#endif	/* !_SEARCH_DOMAIN_H_ */
