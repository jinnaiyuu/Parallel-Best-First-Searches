/* © 2014 the PBNF Authors under the MIT license. See AUTHORS for the list of authors.*/

/**
 * \file bfpsdd_search.h
 *
 *
 *
 * \author Ethan Burns
 * \date 2008-11-20
 */

#if !defined(_BFPSDD_H_)
#define _BFPSDD_H_

#include <vector>

#include "bfpsdd/nblock.h"
#include "bfpsdd/nblock_graph.h"
#include "bfpsdd/real_val_nblock_pq.h"
#include "util/thread.h"
#include "util/mutex.h"
#include "util/solution_stream.h"
#include "util/cumulative_ave.h"
#include "projection.h"
#include "search.h"
#include "state.h"

using namespace std;

class BFPSDDSearch : public Search {
public:
	BFPSDDSearch(unsigned int n_threads, fp_type multiplier, unsigned int min_expansions);
	BFPSDDSearch(unsigned int n_threads, fp_type multiplier, unsigned int min_expansions, fp_type bound);
	virtual ~BFPSDDSearch(void);

	virtual vector<State *> *search(Timer *t, State *s);

	void set_bound(fp_type bound);
	virtual void output_stats(void);
private:

	void set_path(vector<State *> *path);
	bool path_found(void) const;

	class BFPSDDThread : public Thread {
	public:
		BFPSDDThread(BFPSDD::NBlockGraph<BFPSDD::RealValNBlockPQ<State::PQOpsFPrime>, State::PQOpsFPrime> *graph,
			   BFPSDDSearch *search);
		~BFPSDDThread();
		void run(void);
		fp_type get_ave_exp_per_nblock(void);
	private:
		vector<State *> *search_nblock(BFPSDD::NBlock<State::PQOpsFPrime> *n);
		BFPSDD::NBlockGraph<BFPSDD::RealValNBlockPQ<State::PQOpsFPrime>, State::PQOpsFPrime> *graph;
		BFPSDDSearch *search;
		unsigned long exp_this_block;
		CumulativeAverage ave_exp_per_nblock;
	};

	AtomicInt bound;
	unsigned int n_threads;
	const Projection *project;
	vector<State *> *path;
	BFPSDD::NBlockGraph<BFPSDD::RealValNBlockPQ<State::PQOpsFPrime>, State::PQOpsFPrime> *graph;
	SolutionStream *solutions;
	unsigned int min_expansions;
	fp_type multiplier;

	/* stats tracking */
	fp_type sum;
	unsigned int num;
	Timer graph_timer;
};

#endif	/* !_BFPSDD_H_ */
