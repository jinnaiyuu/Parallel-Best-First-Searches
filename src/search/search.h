/* -*- mode:linux -*- */
/**
 * \file search.h
 *
 *
 *
 * \author Ethan Burns
 * \date 2008-10-09
 */

#if !defined(_SEARCH_H_)
#define _SEARCH_H_

#include <vector>

#include "state.h"

using namespace std;

class Search {
public:
	Search(void);

	virtual vector<const State *> *search(const State *) = 0;

	void clear_counts(void);
	unsigned long get_expanded(void) const;
	unsigned long get_generated(void) const;

protected:
	vector<const State *> *expand(const State *);

private:
	unsigned long expanded;
	unsigned long generated;
};

#endif	/* !_SEARCH_H_ */