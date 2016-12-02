/* © 2014 the PBNF Authors under the MIT license. See AUTHORS for the list of authors.*/

/**
 * \file wpbnf_search.h
 *
 *
 *
 * \author Ethan Burns
 * \date 2008-10-29
 */

#if !defined(_WPBNF_SEARCH_H_)
#define _WPBNF_SEARCH_H_

#include <vector>

#include "wpbnf/nblock_graph.h"
#include "wpbnf/nblock.h"
#include "util/thread.h"
#include "util/atomic_int.h"
#include "util/cumulative_ave.h"
#include "util/solution_stream.h"
#include "projection.h"
#include "search.h"
#include "state.h"

using namespace std;

namespace WPBNF {

	class WPBNFSearch : public Search {
	public:
		WPBNFSearch(unsigned int n_threads, unsigned int min_expansions, bool dd);

		virtual ~WPBNFSearch(void);

		virtual vector<State *> *search(Timer *t, State *initial);
		void output_stats(void);

	private:
		void set_path(vector<State *> *path);

		class PBNFThread : public Thread {
		public:
			PBNFThread(NBlockGraph *graph, WPBNFSearch *search);
			~PBNFThread(void);
			void run(void);
			fp_type get_ave_exp_per_nblock(void);
			fp_type get_ave_open_size(void);
		private:
			vector<State *> *search_nblock(NBlock *n);
			bool should_switch(NBlock *n);

			unsigned int expansions; /* for testing switch */
			NBlockGraph *graph;
			WPBNFSearch *search;
			bool set_hot;
		};

		unsigned int n_threads;
		const Projection *project;
		SolutionStream *solutions;
		AtomicInt bound;
		bool done;

		bool dynamic_m;

		NBlockGraph *graph;
		unsigned int min_expansions;

		fp_type weight;


#if defined(INSTRUMENTED)
		/* stats tracking */
		fp_type sum;
		unsigned int num;
		fp_type osum;
		unsigned int onum;
#endif	/* INSTRUMENTED */

		/* duplicate dropping? */
		bool dd;
	};
}

#endif	/* !_PBNF_SEARCH_H_ */
