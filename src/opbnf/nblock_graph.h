/* © 2014 the PBNF Authors under the MIT license. See AUTHORS for the list of authors.*/

/**
 * \file nblock_graph.h
 *
 *
 *
 * \author Sofia Lemons
 * \date 2009-04-18
 */

#if !defined(_NBLOCK_GRAPH_H_)
#define _NBLOCK_GRAPH_H_

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
#include "../util/priority_queue.h"

using namespace std;

namespace OPBNF {
	class NBlockGraph : public NBlockMap<NBlock>::CreationObserver {
	public:
		NBlockGraph(const Projection *p, State *init);

		~NBlockGraph();

		NBlock *next_nblock(NBlock *finished, bool trylock, fp_type bound);
		NBlock *get_nblock(unsigned int hash);
		NBlock *__get_nblock(unsigned int hash);
		void print(ostream &o);
		unsigned int get_max_assigned_nblocks(void) const;
		void set_done(void);
		NBlock *best_in_scope(NBlock *b, fp_type bound);
		void wont_release(NBlock *b);
		void set_hot(NBlock *b, fp_type bound);
		NBlock *next_nblock_fp_peek();
		NBlock *next_nblock_f_peek();

		void observe(NBlock *b);

		unsigned int get_ncreated_nblocks(void);
		fp_type best_free_f_val(void);
		fp_type best_free_fp_val(void);
		fp_type get_f_min(void);

	private:
		void cpp_is_a_bad_language(const Projection *p, State *initial);
		NBlock *create_nblock(unsigned int id);
		NBlock *get_nblock_if_created(unsigned int hash);
		void __set_done(void);
		void __print(ostream &o);
		bool is_free(NBlock *b);
		void set_cold(NBlock *b);
		void update_scope_sigmas(unsigned int y, int delta);

		const Projection *project;

		/* NBlocks (this may be incomplete because nblocks are created lazily). */
		NBlockMap<NBlock> map;

		/* A PQ to sort all of the nblocks on f value.  This is for quickly finding f-min. */
		PriorityQueue<NBlock*, NBlock::NBlockPQFuncs> nblock_pq;
		AtomicInt f_min;

		/* The total number of NBlocks. */
		unsigned int num_nblocks;
		unsigned int nblocks_created;

		/* The number of NBlocks with sigma values of zero. */
		unsigned int num_sigma_zero;

		/* list of free nblock numbers */
		PriorityQueue<NBlock *, NBlock::NBlockPQFuncsFP> free_list_fp;
		PriorityQueue<NBlock *, NBlock::NBlockPQFuncsF> free_list_f;

		/* This flag is set when the search is completed to
		 * signal to all waiting processess that the search
		 * has completed. */
		bool done;

		pthread_mutex_t mutex;
		pthread_cond_t cond;

#if !defined(NDEBUG)
		/* List of all created nblocks. */
		list<NBlock*> nblocks;
#endif	/* !NDEBUG */

	};
} /* PBNF */

#endif	/* !_NBLOCK_GRAPH_H_ */
