/* © 2014 the PBNF Authors under the MIT license. See AUTHORS for the list of authors.*/

/**
 * \file nblock_graph.h
 *
 *
 *
 * \author Ethan Burns
 * \date 2008-10-20
 */

#if !defined(_WBFPSDD_NBLOCK_GRAPH_H_)
#define _WBFPSDD_NBLOCK_GRAPH_H_

#include <assert.h>
#include <pthread.h>

#include <iostream>
#include <map>
#include <list>
#include <vector>

#include "../state.h"
#include "../closed_list.h"
#include "../queue_open_list.h"
#include "../open_list.h"
#include "../projection.h"
#include "../util/nblock_map.h"
#include "../util/atomic_int.h"
#include "nblock.h"

using namespace std;

namespace WBFPSDD {

	class NBlockGraph {
	public:
		enum layer { LAYERA = 0, LAYERB };

		NBlockGraph(const Projection *p, unsigned int nthreads, fp_type multiplier, State *init);

		~NBlockGraph();

		NBlock *next_nblock(NBlock *finished);
		NBlock *get_nblock(unsigned int hash);

		void set_done(void);
		void print(ostream &o);
		unsigned int get_max_assigned_nblocks(void) const;
		unsigned int get_ncreated_nblocks(void);

		fp_type get_layer_value(void) const;

	private:
		void __print(ostream &o);
		void update_scope_sigmas(unsigned int y, int delta);
		void update_sigma(NBlock *yblk, int delta);

		/* NBlocks. */
//		map<unsigned int, NBlock *> blocks;
		NBlockMap<NBlock> map;

		/* The total number of NBlocks. */
		unsigned int num_nblocks;

		/* The number of NBlocks with sigma values of zero. */
		unsigned int num_sigma_zero;

		/* list of free nblock numbers */
		list<NBlock *> free_list;

		/* prio-queue of NBlocks used to populate the next
		 * layer.  Nblocks are added and removed from this pq
		 * as they are acquired by processors. */
		PriorityQueue<NBlock*, NBlock::PQOpsFPrime> nblock_pq_fp;

		/* The value of this layer. */
		fp_type layer_value;


		pthread_mutex_t mutex;
		pthread_cond_t cond;

		/* This flag is set when a thread finds a path so that other
		 * threads do not continue to wait for a new NBlock. */
		bool done;


		/*
		 * Statistics
		 */
		unsigned int nblocks_assigned;
		unsigned int nblocks_assigned_max;

		unsigned int nthreads;
		fp_type multiplier;
	};

} /* WBFPSDD */

#endif	/* !_WBFPSDD_NBLOCK_GRAPH_H_ */
