/* © 2014 the PBNF Authors under the MIT license. See AUTHORS for the list of authors.*/

/**
 * \file lpastar.h
 *
 * Contains the PAStar class.
 *
 * \author Ethan Burns (modified from Sofia Lemons' pastar.h)
 * \date 2009-04-12
 */

#if !defined(_LPASTAR_H_)
#define _LPASTAR_H_

#include "state.h"
#include "search.h"
#include "lf_openlist.h"
#include "lf_closedlist.h"
#include "util/completion_counter.h"

/**
 * A Parallel A* search class.
 */
class LPAStar : public Search {
public:
	LPAStar(unsigned int);
	virtual vector<State *> *search(Timer *, State *);
        void set_done();
        bool is_done();
        void set_path(vector<State *> *path);
private:
	LF_OpenList open;
	LF_ClosedList closed;
        volatile bool done;
        friend class LPAStarThread;
        const unsigned int n_threads;

        vector<State *> * volatile path;
	AtomicInt bound;
};

#endif	/* !_LPASTAR_H_ */
