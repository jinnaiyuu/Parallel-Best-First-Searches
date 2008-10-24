/* -*- mode:linux -*- */
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

#include "psdd/projection.h"
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

	virtual void set_heuristic(const Heuristic *h);
	virtual const Heuristic* get_heuristic(void) const;

	/* Abstract methods */
	virtual State *initial_state(void) = 0;
	virtual vector<const State*> *expand(const State *s) = 0;
	virtual void set_projection(Projection *p) = 0;
	virtual Projection *get_projection(void) const = 0;
private:
	const Heuristic *heuristic;
};

#endif	/* !_SEARCH_DOMAIN_H_ */
