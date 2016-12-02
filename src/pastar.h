/* © 2014 the PBNF Authors under the MIT license. See AUTHORS for the list of authors.*/

/**
 * \file pastar.h
 *
 * Contains the PAStar class.
 *
 * \author Sofia Lemons
 * \date 2008-11-02
 */

#if !defined(_PASTAR_H_)
#define _PASTAR_H_

#include "state.h"
#include "search.h"
#include "synch_pq_olist.h"
#include "synch_closed_list.h"
#include "util/completion_counter.h"

/**
 * A Parallel A* search class.
 */
class PAStar : public Search {
public:
	PAStar(unsigned int);
	virtual vector<State *> *search(Timer *, State *);
        void set_done();
        bool is_done();
        void set_path(vector<State *> *path);
        bool has_path();
private:
	SynchPQOList<State::PQOpsF> open;
	SynchClosedList closed;
        bool done;
	pthread_mutex_t mutex;
        friend class PAStarThread;
        const unsigned int n_threads;
        vector<State *> *path;
	AtomicInt bound;
};

#endif	/* !_PASTAR_H_ */
