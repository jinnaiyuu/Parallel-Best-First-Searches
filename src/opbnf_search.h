/* © 2014 the PBNF Authors under the MIT license. See AUTHORS for the list of authors.*/

/**
 * \file wpbnf_search.h
 *
 *
 *
 * \author Sofia Lemons
 * \date 2009-04-18
 */

#if !defined(_OPBNF_SEARCH_H_)
#define _OPBNF_SEARCH_H_

#include <vector>

#include "opbnf/nblock_graph.h"
#include "opbnf/nblock.h"
#include "util/thread.h"
#include "util/atomic_int.h"
#include "util/cumulative_ave.h"
#include "projection.h"
#include "search.h"
#include "state.h"

using namespace std;

namespace OPBNF {

	class OPBNFSearch : public Search {
	public:
		OPBNFSearch(unsigned int n_threads, unsigned int min_expansions);

		virtual ~OPBNFSearch(void);

		virtual vector<State *> *search(Timer *t, State *initial);
		virtual void output_stats(void);
	private:
		void set_path(vector<State *> *path);

		class PBNFThread : public Thread {
		public:
			PBNFThread(NBlockGraph *graph, OPBNFSearch *search);
			~PBNFThread(void);
			void run(void);
		private:
			vector<State *> *search_nblock(NBlock *n);
			bool should_switch(NBlock *n);

			unsigned int expansions; /* for testing switch */
			NBlockGraph *graph;
			OPBNFSearch *search;
			bool set_hot;
			fp_type next_best;
		};

		unsigned int n_threads;
		const Projection *project;
		pthread_mutex_t path_mutex;
		vector<State *> *path;
		AtomicInt bound;
		bool done;

		NBlockGraph *graph;
		unsigned int min_expansions;
		Timer graph_timer;

		fp_type b;
	};
}

#endif	/* !_PBNF_SEARCH_H_ */
