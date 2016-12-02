/* © 2014 the PBNF Authors under the MIT license. See AUTHORS for the list of authors.*/

/**
 * \file breadth_first_search.h
 *
 *
 *
 * \author Ethan Burns
 * \date 2008-10-25
 */

#if !defined(_BREADTH_FIRST_SEARCH_H_)
#define _BREADTH_FIRST_SEARCH_H_

#include <vector>

#include "state.h"
#include "search.h"

using namespace std;

class BreadthFirstSearch : public Search {
public:
	BreadthFirstSearch(void);

	BreadthFirstSearch(fp_type bound);

	virtual ~BreadthFirstSearch(void);

	virtual vector<State *> *search(Timer *, State *);

private:
	fp_type bound;
};

#endif	/* !_BREADTH_FIRST_SEARCH_H_ */
