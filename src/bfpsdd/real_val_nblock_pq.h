/* © 2014 the PBNF Authors under the MIT license. See AUTHORS for the list of authors.*/

/**
 * \file real_val_nblock_pq.h
 *
 *
 *
 * \author Ethan Burns
 * \date 2008-11-20
 */

#if !defined(_REAL_VAL_NBLOCK_PQ_H_)
#define _REAL_VAL_NBLOCK_PQ_H_

#include <assert.h>

#include <queue>
#include <algorithm>

#include "nblock.h"

using namespace std;

namespace BFPSDD {

	template <class StateCompare>
		class RealValNBlockPQ {
	public:
		RealValNBlockPQ(void);
		void add(NBlock<StateCompare> *b);
		void remove(NBlock<StateCompare> *b);
		NBlock<StateCompare> *take(void);
		NBlock<StateCompare> *top(void);
		bool empty(void);
	private:
		class NBlockCompare {
		public:
			bool operator()(NBlock<StateCompare> *a,
					NBlock<StateCompare> *b) {
				return a->open.get_best_val()
				       > b->open.get_best_val();
			}
		};

		NBlockCompare compare;

		vector <NBlock<StateCompare> *> heap;
	};

	template<class StateCompare>
		RealValNBlockPQ<StateCompare>::RealValNBlockPQ(void)
	{
		make_heap(heap.begin(), heap.end(), compare);
	}

	template<class StateCompare>
		void RealValNBlockPQ<StateCompare>::add(NBlock<StateCompare> *b)
	{
/*
		typename vector <NBlock<StateCompare> *>::iterator i;

		for (i = heap.begin(); i != heap.end(); i++)
			assert(*i != b);
*/
		heap.push_back(b);
		push_heap(heap.begin(), heap.end(), compare);
	}

	template<class StateCompare>
		void RealValNBlockPQ<StateCompare>::remove(NBlock<StateCompare> *b)
	{
		typename vector<NBlock<StateCompare> *>::iterator iter;

		for (iter = heap.begin(); iter != heap.end(); iter++) {
			if (*iter == b) {
				heap.erase(iter);
				make_heap(heap.begin(), heap.end(), compare);
				return;
			}
		}
	}

	template<class StateCompare>
		NBlock<StateCompare> *RealValNBlockPQ<StateCompare>::take(void)
	{
		NBlock<StateCompare> *b;

		b = heap.front();
		pop_heap(heap.begin(), heap.end(), compare);
		heap.pop_back();

		return b;
	}

	template<class StateCompare>
		NBlock<StateCompare> *RealValNBlockPQ<StateCompare>::top(void)
	{
		return heap.front();
	}

	template<class StateCompare>
		bool RealValNBlockPQ<StateCompare>::empty(void)
	{
		return heap.size() == 0;
	}

}

#endif	/* !_REAL_VAL_NBLOCK_PQ_H_ */
