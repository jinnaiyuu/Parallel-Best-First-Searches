/* © 2014 the PBNF Authors under the MIT license. See AUTHORS for the list of authors.*/

/**
 * \file nblock.h
 *
 *
 *
 * \author Ethan Burns
 * \date 2008-10-21
 */

#if !defined(_ARPBNF_NBLOCK_H_)
#define _ARPBNF_NBLOCK_H_

#include <pthread.h>
#include <iostream>
#include <set>

#include "../open_list.h"
#include "../pq_open_list.h"
#include "../closed_list.h"
#include "../incons_list.h"
#include "../projection.h"

using namespace std;

namespace ARPBNF {
/**
 * An NBlock
 */
	struct NBlock {
		NBlock(const Projection *p, unsigned int id);

		~NBlock(void);

		void next_iteration(void);
		bool operator<(NBlock *a);
		void print(ostream &s);

		/**
		 * Resort the nodes in this nblock.  This means that
		 * the nodes will need to recompute their heuristic
		 * values and the PQ will need to resort.
		 */
		void resort(void);

		struct NBlockPQFuncsFprime {
			/* Predecessor operator. */
			int inline operator()(NBlock *a, NBlock *b) {
				fp_type afp = a->open.get_best_val();
				fp_type bfp = b->open.get_best_val();

				if (afp == bfp) {
					fp_type af = a->open.get_best_val();
					fp_type bf = b->open.get_best_val();
					if (af == bf) {
						fp_type ag = fp_infinity;
						fp_type bg = fp_infinity;
						if (!a->open.empty())
							ag = a->open.peek()->get_g();
						if (!b->open.empty())
							bg = b->open.peek()->get_g();
						return ag > bg;
					}
					return af < bf;
				}
				return afp < bfp;
			}
			/* Set the prio queue index. */
			void inline operator()(NBlock *a, int i) {
				a->pq_index = i;
			}
			/* Set the prio queue index. */
			int inline operator()(NBlock *a) {
				return a->pq_index;
			}
			/* Set the prio queue index. */
			fp_type inline get_value(NBlock *a) {
				return a->open.get_best_val();
			}
		};

		unsigned int id;
		unsigned int sigma;
		ClosedList closed;
		InconsList incons;
		PQOpenList<State::PQOpsFPrime> open;

		unsigned int sigma_hot;
		int hot;
		int inuse;
		int pq_index;

		set<unsigned int> interferes;
		set<unsigned int> preds;
		set<unsigned int> succs;
	};
}	/* ARPBNF */
#endif	/* !_ARPBNF_NBLOCK_H_ */
