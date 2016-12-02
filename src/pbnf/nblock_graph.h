/* © 2014 the PBNF Authors under the MIT license. See AUTHORS for the list of authors.*/

/**
 * \file nblock_graph.h
 *
 *
 *
 * \author Ethan Burns
 * \date 2008-10-20
 */

#if !defined(_NBLOCK_GRAPH_H_)
#define _NBLOCK_GRAPH_H_

#include <iostream>
#include <map>
#include <list>
#include <vector>

#include "nblock.h"
#include "../state.h"
#include "../closed_list.h"
#include "../queue_open_list.h"
#include "../open_list.h"
#include "../projection.h"
#include "../util/nblock_map.h"
#include "../util/mutex.h"
#include "../util/atomic_int.h"

using namespace std;

namespace PBNF {
	class NBlockGraph : public NBlockMap<NBlock>::CreationObserver {
	public:
		NBlockGraph(const Projection *p, State *init,
			    AtomicInt *b, double w);

		~NBlockGraph();

		NBlock *next_nblock(NBlock *finished);
		NBlock *get_nblock(unsigned int hash);
		void print(ostream &o);
#if defined(INSTRUMENTED)
		unsigned int get_max_assigned_nblocks(void) const;
#endif	/* INSTRUMENTED */
		void set_done(void);
		NBlock *best_in_scope(NBlock *b);
		void wont_release(NBlock *b);
		bool set_hot(NBlock *b);

		unsigned int get_ncreated_nblocks(void);

		fp_type best_val(void);

		/* For NBlockMap::CreationObserver */
		void observe(NBlock *b);

		/* Print statistics. */
		void print_stats(ostream &o);

	private:
		void cpp_is_a_bad_language(const Projection *p,
					   State *initial,
					   AtomicInt *b /* bound */,
					   double w /* weight */);

		/* Remove all nblocks from the free_list.  The nodes in
		 * these nblocks will remain in their open lists, for
		 * pruning later... if they ever are added back to the
		 * free_list.  They can be added back to the free_list
		 * if a processor searches one of their neighbors and
		 * they gain a node worth looking at. */
		void prune_free_list(void);

		void __print(ostream &o);
		bool is_free(NBlock *b);

		/* Set an nblock to cold.  Returns true if the caller
		 * needs to pthread_broadcast. */
		bool set_cold(NBlock *b);

		void update_scope_sigmas(NBlock *n, int delta);

		const Projection *project;

		/* NBlocks (this may be incomplete because nblocks are
		 * created lazily). */
		NBlockMap<NBlock> map;

		/* The total number of NBlocks. */
		unsigned int num_nblocks;

		/* The number of NBlocks with sigma values of zero. */
		unsigned int num_sigma_zero;

		/* list of free nblock numbers */
		PriorityQueue<NBlock*, NBlock::NBlockPQFuncsFprime> free_list;

		/* This flag is set when the search is completed to
		 * signal to all waiting processess that the search
		 * has completed. */
		bool done;

		Mutex mutex;

		/* A pointer to the bound used in the search.  For
		 * early termination and free_list pruning. */
		AtomicInt *bound;
		double weight;

#if defined(INSTRUMENTED)
		/*
		 * Statistics
		 */
		/* total times a lock was taken without trylock on a
		 * switch. */
		unsigned int switch_locks_forced;
		/* the block was empty on the force. */
		unsigned int switch_locks_forced_empty;
		/* finished was NULL on the force. */
		unsigned int switch_locks_forced_finished;
		/* total number of times nblocks were switched. */
		unsigned int total_switches;
		unsigned int total_waits;
		unsigned int nblocks_assigned;
		unsigned int nblocks_assigned_max;
#endif	/* INSTRUMENTED */

#if defined(INSTRUMENTED) || defined(QUEUE_SIZES)
		/* list of created nblocks. */
		list<NBlock*> nblocks;
#endif	/* INSTRUMENTED || QUEUE_SIZES*/
	};
} /* PBNF */

#endif	/* !_NBLOCK_GRAPH_H_ */
