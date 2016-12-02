/* © 2014 the PBNF Authors under the MIT license. See AUTHORS for the list of authors.*/

/**
 * \file nblock.h
 *
 *
 *
 * \author Sofia Lemons
 * \date 2009-04-18
 */

#if !defined(_NBLOCK_H_)
#define _NBLOCK_H_

#include <pthread.h>
#include <iostream>
#include <set>

#include "../open_list.h"
#include "../pq_open_list.h"
#include "../closed_list.h"
#include "../projection.h"

using namespace std;

namespace OPBNF {
	struct NBlock {

		/**
		 * Operations for the nblock PQ for the f' free list.
		 */
		struct NBlockPQFuncsFP {
			/* Predecessor operator. */
			int inline operator()(NBlock *a, NBlock *b) {
				return a->open_fp.get_best_val() < b->open_fp.get_best_val();
			}
			/* Set the prio queue index. */
			void inline operator()(NBlock *a, int i) {
				a->index_fp = i;
			}
			/* Get the prio queue index. */
			int inline operator()(NBlock *a) {
				return a->index_fp;
			}
			/* Get the value. */
			fp_type inline get_value(NBlock *a) {
				return a->open_fp.get_best_val();
			}
		};

		/**
		 * Operations for the nblock PQ for the f free list.
		 */
		struct NBlockPQFuncsF {
			/* Predecessor operator. */
			int inline operator()(NBlock *a, NBlock *b) {
				return a->open_f.get_best_val() < b->open_f.get_best_val();
			}
			/* Set the prio queue index. */
			void inline operator()(NBlock *a, int i) {
				a->index_f = i;
			}
			/* Get the prio queue index. */
			int inline operator()(NBlock *a) {
				return a->index_f;
			}
			/* Get the value. */
			fp_type inline get_value(NBlock *a) {
				return a->open_f.get_best_val();
			}
		};

		/**
		 * Operations for the nblock PQ used for trakcing f_min.
		 */
		struct NBlockPQFuncs {
			/* Predecessor operator. */
			int inline operator()(NBlock *a, NBlock *b) {
				return a->get_local_f_min() < b->get_local_f_min();
			}
			/* Set the prio queue index. */
			void inline operator()(NBlock *a, int i) {
				a->pq_index = i;
			}
			/* Get the prio queue index. */
			int inline operator()(NBlock *a) {
				return a->pq_index;
			}
			/* Get the value. */
			fp_type inline get_value(NBlock *a) {
				return a->get_local_f_min();
			}
		};


		static bool better(NBlock* a, NBlock* b, fp_type bound);

		NBlock(const Projection *p, unsigned int id);
		~NBlock(void);
		bool operator<(NBlock *a);
		void print(ostream &s);

		/* Add a state to the nblock. */
		void add(State *s);

		/* Update the value of the given state in the OPEN lists. */
		void see_update(State *s);

		/* Take the state with the best f-value. */
		State *take_f(void);

		/* Take the state with the best f'-value. */
		State *take_fp(void);

		/* Prune all states. */
		void prune(void);

		/* Get the best f-valued state. */
		fp_type get_best_f(void);

		/* Get the best f'-valued state. */
		fp_type get_best_fp(void);

		/* Test if this nblock is empty. */
		bool empty();

		/* Read the f_min value local to this nblock. */
		fp_type get_local_f_min();

		/* Update the local local_f_min of this nblock for
		 * tracking the global f_min.  This should only be
		 * called when the nblock graph's mutex is held so
		 * that it doesn't change on us after the nblock PQ
		 * has been updated. */
		void update_local_f_min();

		unsigned int id;
		unsigned int sigma;
		ClosedList closed;
		unsigned int sigma_hot;
		int hot;
		int inuse;

		/**
		 * Index into the PQ tracking f_min.
		 */
		int pq_index;

		/**
		 * Index into the f' freelist.
		 */
		int index_fp;

		/**
		 * Index into the f freelist.
		 */
		int index_f;

		set<unsigned int> interferes;
		set<unsigned int> preds;
		set<unsigned int> succs;

	private:
		void set_best_values(void);

		PQOpenList<State::PQOpsFPrime> open_fp;
		PQOpenList<State::PQOpsF> open_f;

		AtomicInt best_f;
		AtomicInt best_fp;

		/**
		 * This is set only when the nblock graph's mutex is
		 * held (when the block is released).  This is used
		 * for getting a lower bound on the global f_min.
		 */
		fp_type local_f_min;
	};
}	/* PBNF */
#endif	/* !_NBLOCK_H_ */
