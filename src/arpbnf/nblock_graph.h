/* © 2014 the PBNF Authors under the MIT license. See AUTHORS for the list of authors.*/

/**
 * \file nblock_graph.h
 *
 *
 *
 * \author Ethan Burns
 * \date 2008-10-20
 */

#if !defined(_ARPBNF_NBLOCK_GRAPH_H_)
#define _ARPBNF_NBLOCK_GRAPH_H_

#include <pthread.h>

#include <iostream>
#include <list>
#include <vector>

#include "nblock.h"
#include "../state.h"
#include "../closed_list.h"
#include "../queue_open_list.h"
#include "../open_list.h"
#include "../projection.h"
#include "../util/nblock_map.h"

using namespace std;

extern "C" {
#include "../lockfree/include/lockfree.h"
}

namespace ARPBNF {
	class ARPBNFSearch;


	class NBlockGraph : public NBlockMap<NBlock>::CreationObserver {
	public:
		NBlockGraph(const Projection *p, State *init, AtomicInt *bound);

		~NBlockGraph();

		NBlock *next_nblock(NBlock *finished, bool trylock, bool final_wt);
		NBlock *get_nblock(unsigned int hash);
		NBlock *__get_nblock(unsigned int hash);
		void print(ostream &o);
		bool is_done(void);
		NBlock *best_in_scope(NBlock *b);
		void wont_release(NBlock *b);
		void set_hot(NBlock *b);

		unsigned int get_ncreated_nblocks(void);

		fp_type best_val(void);

		/**
		 * Signal all threads to stop and resort the nblocks.
		 */
		void call_for_resort(NBlock *finished, bool final_weight, ARPBNFSearch *s);

		/**
		 * Test if the resort bit is set.
		 */
		bool needs_resort(void);

		/**
		 * Observe the creation of a new nblock (add it to the
		 * 'nblocks' list).
		 */
		void observe(NBlock *b);

		/**
		 * There is a new bound... check if there are any good
		 * nblocks.
		 */
		void new_bound(void);

	private:
		void cpp_is_a_bad_language(const Projection *p, State *initial);
		NBlock *create_nblock(unsigned int id);
		NBlock *get_nblock_if_created(unsigned int hash);
		void set_done(void);
		void __print(ostream &o);
		bool is_free(NBlock *b);
		void set_cold(NBlock *b);
		void update_scope_sigmas(unsigned int y, int delta);

		/**
		 * Free an nblock
		 */
		void free_nblock(NBlock *finished);

		/**
		 * Actually add the nblock to the freelist (if it is free)
		 */
		void free_if_free(NBlock *finished);

		/**
		 * Resort all of the nblocks.
		 */
		void resort(void);

		const Projection *project;

		/**
		 * NBlocks (this may be incomplete because nblocks are created lazily).
		 */
		NBlockMap<NBlock> map;

		/* The total number of NBlocks. */
		unsigned int num_nblocks;
		unsigned int nblocks_created;

		/* The number of NBlocks with sigma values of zero. */
		AtomicInt num_sigma_zero;

		/**
		 * List of free nblock numbers
		 */
		PriorityQueue<NBlock*, NBlock::NBlockPQFuncsFprime> free_list;

		/**
		 * List of all created nblocks.  When an nblock is
		 * lazily created, it is added to this list so that it
		 * can be resorted when the time comes.
		 */
		list<NBlock*> nblocks;

		/* This flag is set when the search is completed to
		 * signal to all waiting processess that the search
		 * has completed. */
		volatile bool done;

		/**
		 * Locks the nblock graph.
		 */
		pthread_mutex_t mutex;

		/**
		 * Signals the condition that an nblock has become
		 * free for searching.
		 */
		pthread_cond_t cond;

		/**
		 * When this is set, all of the threads get together
		 * and resort the nblocks.
		 */
		volatile bool resort_flag;

		/**
		 * Set to tell threads to begin sorting.
		 */
		volatile bool resort_start;

		/**
		 * Queue of nblocks which need to be resorted.
		 */
		lf_queue *resort_q;

		/**
		 * The number of nblocks that need to be sorted.
		 */
		AtomicInt n_to_sort;

		/** The current search bound. */
		AtomicInt *bound;
	};
} /* ARPBNF */

#endif	/* !_ARPBNF_NBLOCK_GRAPH_H_ */
