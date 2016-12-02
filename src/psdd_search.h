/* © 2014 the PBNF Authors under the MIT license. See AUTHORS for the list of authors.*/

/**
 * \file psdd.h
 *
 *
 *
 * \author Ethan Burns
 * \date 2008-10-24
 */

#if !defined(_PSDD_H_)
#define _PSDD_H_

#include <pthread.h>

#include <vector>

#include "psdd/nblock.h"
#include "psdd/nblock_graph.h"
#include "util/thread.h"
#include "util/cumulative_ave.h"
#include "projection.h"
#include "search.h"
#include "state.h"

using namespace std;
using namespace PSDD;

class PSDDSearch : public Search {
public:
	PSDDSearch(unsigned int n_threads);
	PSDDSearch(unsigned int n_threads, fp_type bound);
	virtual ~PSDDSearch(void);

	vector<State *> *search(Timer *t, State *s);
	vector<State *> *search(Timer *t, State *s, NBlockGraph *g);

	void set_bound(fp_type bound);
	fp_type get_lowest_out_of_bounds(void);
	void do_not_print(void);
	void reset(void);
private:

	void set_path(vector<State *> *path);
	bool path_found(void) const;

	class PSDDThread : public Thread {
	public:
		PSDDThread(NBlockGraph *graph, PSDDSearch *search);
		virtual ~PSDDThread();
		virtual void run(void);
		fp_type get_lowest_out_of_bounds(void);
		fp_type get_ave_exp_per_nblock(void);
	private:
		vector<State *> *search_nblock(NBlock *n);
		NBlockGraph *graph;
		PSDDSearch *search;
		fp_type lowest_out_of_bounds;
		unsigned long exp_this_block;
		CumulativeAverage ave_exp_per_nblock;
	};

	AtomicInt bound;
	unsigned int n_threads;
	const Projection *project;
	vector<State *> *path;
	pthread_mutex_t path_mutex;

	NBlockGraph *graph;
	fp_type lowest_out_of_bounds;
	bool print;
};

#endif	/* !_PSDD_H_ */
