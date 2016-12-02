/* © 2014 the PBNF Authors under the MIT license. See AUTHORS for the list of authors.*/

/**
 * \file sync_wastar.h
 *
 *
 *
 * \author Sofia Lemons
 * \date 2009-04-16
 */

#if !defined(_SYNC_WASTAR_H_)
#define _SYNC_WASTAR_H_

#include "state.h"
#include "search.h"
#include "util/sync_solution_stream.h"
#include "util/atomic_int.h"
#include "pq_open_list.h"

/**
 * This search just performs a bunch of wA* searches, each in its own
 * thread, sharing solution cost and lowest weight value finished yet.
 */
class SyncWAStar : public Search {
public:
  SyncWAStar(unsigned int n_threads, vector<State*> *inits, bool dd);
	virtual ~SyncWAStar(void);
	virtual vector<State *> *search(Timer *t, State *init);
	friend class SyncWAStarThread;
	//virtual void output_stats(void);
        void set_done();
        bool is_done();
        void set_path(vector<State *> *path);
	State *get_next_init();

	void output_stats(void);
private:
	unsigned int n_threads;
        bool done;
	pthread_mutex_t mutex;
	AtomicInt bound;
	SolutionStream *solutions;

	vector<State*> *inits;
	unsigned int next_init;
	bool final_weight;
	bool dd;
};

#endif	/* !_SYNC_WASTAR_H_ */
