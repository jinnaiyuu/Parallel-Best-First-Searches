/* © 2014 the PBNF Authors under the MIT license. See AUTHORS for the list of authors.*/

/**
 * \file search.h
 *
 * Abstract search classe
 *
 * \author Ethan Burns
 * \date 2008-10-09
 */

#if !defined(_SEARCH_H_)
#define _SEARCH_H_

#include <vector>

#include "util/atomic_int.h"
#include "util/timer.h"
#include "state.h"

using namespace std;

void output_search_stats_on_timeout(void);

/**
 * An abstract search class that collects some statistics.
 */
class Search {
public:
	Search(void);
	virtual ~Search() {}

	virtual vector<State *> *search(Timer *t, State *) = 0;
	virtual void output_stats(void);

	void clear_counts(void);
	unsigned long get_expanded(void) const;
	unsigned long get_generated(void) const;

protected:
	vector<State *> *expand(State *);

	void set_expanded(unsigned long e);
	void set_generated(unsigned long g);

private:
	AtomicInt expanded;
	AtomicInt generated;
};

#endif	/* !_SEARCH_H_ */
