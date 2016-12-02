/* © 2014 the PBNF Authors under the MIT license. See AUTHORS for the list of authors.*/

/**
 * \file nblock.h
 *
 *
 *
 * \author Ethan Burns
 * \date 2008-10-21
 */

#if !defined(_WPBNF_NBLOCK_H_)
#define _WPBNF_NBLOCK_H_

#include <pthread.h>
#include <iostream>
#include <set>

#include "../open_list.h"
#include "../pq_open_list.h"
#include "../closed_list.h"
#include "../projection.h"

using namespace std;

namespace WPBNF {
	struct NBlock {


		struct NBlockPQFuncsFprime {
			/* Predecessor operator. */
			int inline operator()(NBlock *a, NBlock *b) {
				fp_type afp = a->open_fp.get_best_val();
				fp_type bfp = b->open_fp.get_best_val();

				if (afp == bfp) {
					fp_type ag = 0;
					fp_type bg = 0;
					if (!a->open_fp.empty())
						ag = a->open_fp.peek()->get_g();
					if (!b->open_fp.empty())
						bg = b->open_fp.peek()->get_g();
					return ag > bg;
				}
				return afp < bfp;
			}
			/* Set the prio queue index. */
			void inline operator()(NBlock *a, int i) {
				a->fp_pq_index = i;
			}
			/* Set the prio queue index. */
			int inline operator()(NBlock *a) {
				return a->fp_pq_index;
			}
			/* Set the prio queue index. */
			fp_type inline get_value(NBlock *a) {
				return a->open_fp.get_best_val();
			}
		};

		NBlock(const Projection *p, unsigned int id);
		~NBlock(void);
		void next_iteration(void);
		bool operator<(NBlock *a);
		void print(ostream &s);

		/* Get the value of teh best open node in this nblock. */
		fp_type best_value(void);

		unsigned int id;
		unsigned int sigma;
		ClosedList closed;
		PQOpenList<State::PQOpsFPrime> open_fp;
		unsigned int sigma_hot;
		int hot;
		int inuse;
		int pq_index; // Index into the f_min PQ
		int fp_pq_index; // Index into the free list PQ


		unsigned int *interferes;
		unsigned int ninterferes;
		vector<unsigned int> preds;
		vector<unsigned int> succs;
	};
}	/* PBNF */
#endif	/* !_WPBNF_NBLOCK_H_ */
