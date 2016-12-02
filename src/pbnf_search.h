/* © 2014 the PBNF Authors under the MIT license. See AUTHORS for the list of authors.*/

/**
 * \file pbnf_search.h
 *
 *
 *
 * \author Ethan Burns
 * \date 2008-10-29
 */

#if !defined(_PBNF_SEARCH_H_)
#define _PBNF_SEARCH_H_

#include <vector>

#include "pbnf/nblock_graph.h"
#include "pbnf/nblock.h"
#include "util/thread.h"
#include "util/atomic_int.h"
#include "util/cumulative_ave.h"
#include "util/sync_solution_stream.h"
#include "projection.h"
#include "search.h"
#include "state.h"

using namespace std;

namespace PBNF {

	class PBNFSearch : public Search {
	public:
		PBNFSearch(unsigned int n_threads,
			   unsigned int min_expansions,
			   bool detect_livelocks);

		virtual ~PBNFSearch(void);

		virtual vector<State *> *search(Timer *t, State *initial);
		virtual void output_stats(void);

	private:
		void set_path(vector<State *> *path);

		class PBNFThread : public Thread {
		public:
			PBNFThread(NBlockGraph *graph, PBNFSearch *search);
			~PBNFThread(void);
			void run(void);
			fp_type get_ave_exp_per_nblock(void);
			fp_type get_ave_open_size(void);
		private:
			vector<State *> *search_nblock(NBlock *n);
			bool should_switch(NBlock *n);

			unsigned int expansions; /* for testing switch */
			NBlockGraph *graph;
			PBNFSearch *search;
#if defined(INSTRUMENTED)
			unsigned long exp_this_block;
			CumulativeAverage ave_exp_per_nblock;
#endif	/* INSTRUMENTED */

		};

		unsigned int n_threads;
		const Projection *project;

		SolutionStream *solutions;
		AtomicInt bound;

		bool detect_livelocks;

		NBlockGraph *graph;
		unsigned int min_expansions;

#if defined(INSTRUMENTED)
		/* For tracking avg nodes expanded per-nblock. */
		fp_type sum;
		unsigned int num;
#endif	/* INSTRUMENTED */

#if defined(COUNT_FS)
		static F_hist fs;
#endif	/* COUNT_FS */
	};
}

#endif	/* !_PBNF_SEARCH_H_ */
