/* © 2014 the PBNF Authors under the MIT license. See AUTHORS for the list of authors.*/

/**
 * \file pq_open_list.h
 *
 * An open list class.
 *
 * \author Ethan Burns
 * \date 2008-10-09
 */

/*
#include <assert.h>

#include <list>
#include <limits>

#include "state.h"
#include "open_list.h"
#include "util/priority_queue.h"
#include "util/cpu_timer.h"
*/

#include "pq_multiheap_open_list.h"

using namespace std;

template<class PQCompare>
PQMultiheapOpenList<PQCompare>::PQMultiheapOpenList(unsigned int n_threads,
		unsigned int n_heaps) :
		OpenList(), n_threads(n_threads), n_heaps(n_heaps) {
	printf("PQMultiheap constructor\n");
	printf("n_threads, n_heaps = %u, %u\n", this->n_threads, this->n_heaps);
	set_best_val(fp_infinity);
	pq.resize(n_heaps);

	for (unsigned int i = 0; i < n_heaps; ++i) {
		pq[i] = new PriorityQueue<State *, PQCompare>();
	}
	printf("pq[0]->fill = %u\n", pq[0]->get_fill());
}

/**
 * Add a state to the OpenList.
 * \param s The state to add.
 */
template<class PQCompare>
void PQMultiheapOpenList<PQCompare>::add(State *s) {
	printf("PQMultiheap::add\n");
	unsigned int which_heap = (s->dist_hash() / n_threads) % n_heaps;
	printf("which_heap = %u\n", which_heap);
	start_queue_timer();
	s->set_open(true);
	stop_queue_timer();

	pq[which_heap]->add(s);
	change_size(1);

	// TODO: check this
	fp_type best = fp_infinity;
	for (unsigned int i = 0; i < n_heaps; ++i) {
		if (!pq[i]->empty() && (best > comp.get_value(pq[i]->front()))) {
			set_best_val(comp.get_value(pq[i]->front()));
			best = comp.get_value(pq[i]->front());
			best_heap = i;
		}
	}
	/*
	if (get_best_val() > comp.get_value(s)) {
		set_best_val(comp.get_value(s));
		best_heap = which_heap;
	}
*/
	printf("best_heap = %u\n", best_heap);
}

/**
 * Remove and return the state with the lowest f-value.
 * \return The front of the priority queue.
 */
template<class PQCompare>
State *PQMultiheapOpenList<PQCompare>::take(void) {
	printf("PQMultiheap::take\n");
	printf("best_heap = %u\n", best_heap);
	State *s;
	printf("pq size = %u\n", pq[best_heap]->get_fill());
	start_queue_timer();
//	for (unsigned int i = 0; i < n_heaps; ++i) {
	s = pq[best_heap]->take(); // TODO: check
//	}
	if (!s) {
		printf("s is NULL\n");

		return s;
	}
	printf("taken\n");
//	stop_queue_timer();
	s->set_open(false);
//	change_size(-1);
	printf("iterate over heaps\n");
	fp_type best = fp_infinity;
	for (unsigned int i = 0; i < n_heaps; ++i) {
		if (!pq[i]->empty() && best > comp.get_value(pq[i]->front())) {
			set_best_val(comp.get_value(pq[i]->front()));
			best = comp.get_value(pq[i]->front());
			best_heap = i;
		}
	}
	printf("return take\n");
	return s;
}

/**
 * Peek at the top element.
 */
template<class PQCompare>
State * PQMultiheapOpenList<PQCompare>::peek(void) {
	printf("PQMultiheap::peek\n");
	return pq[best_heap]->front(); // TODO chk
}

/**
 * Test if the OpenList is empty.
 * \return True if the open list is empty, false if not.
 */
template<class PQCompare>
bool PQMultiheapOpenList<PQCompare>::empty(void) {
	printf("PQMultiheap::empty\n");
	for (unsigned int i = 0; i < n_heaps; ++i) { // TODO chk
		if (!pq[i]->empty()) {
			return false;
		}
	}
	return true;
}

/**
 * Delete all of the states on the open list.
 */
template<class PQCompare>
void PQMultiheapOpenList<PQCompare>::delete_all_states(void) {
	printf("PQMultiheap::delete_all_states\n");
	for (unsigned int i = 0; i < n_heaps; ++i) { // TODO chk
		while (!pq[i]->empty()) {
			delete pq[i]->take();
		}
		pq[i]->reset();
	}
	set_size(0);
}

/**
 * Prune all of the states.
 */
template<class PQCompare>
void PQMultiheapOpenList<PQCompare>::prune(void) {
	printf("PQMultiheapOpenList::prune\n");

	for (unsigned int i = 0; i < n_heaps; ++i) {
		int fill = pq[i]->get_fill(); //TODO

		for (int j = 0; j < fill; j += 1)
			pq[i]->get_elem(j)->set_open(false);
		pq[i]->reset();
	}
	set_size(0);

}

template<class PQCompare>
unsigned int PQMultiheapOpenList<PQCompare>::size(void) {
	printf("PQMultiheap::size\n");
	unsigned int fill = 0;
	for (unsigned int i = 0; i < n_heaps; ++i) {
		fill += pq[i]->get_fill();
	}
	return fill; //TODO
}

/**
 * Ensure that the heap propert holds.  This should be called after
 * updating states which are open.
 */
template<class PQCompare>
void PQMultiheapOpenList<PQCompare>::see_update(State *s) {
	printf("PQMultiheap::see_update\n");
/*
	start_queue_timer();
	pq[best_heap]->see_update(get_index(s)); // TODO
	stop_queue_timer();
*/

	fp_type best = fp_infinity;
	for (unsigned int i = 0; i < n_heaps; ++i) {
		if (!pq[i]->empty() && best > comp.get_value(pq[i]->front())) {
			best = comp.get_value(pq[i]->front());
			best_heap = i;
		}
	}
	set_best_val(best);

//	set_best_val(comp.get_value(pq[best_heap]->front()));
}

/**
 * Remove the given state from the PQ.
 */
template<class PQCompare>
void PQMultiheapOpenList<PQCompare>::remove(State *s) {
	printf("PQMultiheap::remove\n");
	/*	start_queue_timer(); // TODO
	 for (unsigned int i = 0; i < n_heaps; ++i) {
	 pq[i].remove(get_index(s));
	 }
	 stop_queue_timer();

	 s->set_open(false);
	 change_size(-1);
	 if (pq.empty())
	 set_best_val(fp_infinity);
	 else
	 set_best_val(comp.get_value(pq.front()));*/
}

/**
 * Resort the whole thing.
 */
template<class PQCompare>
void PQMultiheapOpenList<PQCompare>::resort(void) {
	printf("PQMultiheap::resort\n");
	/*	start_queue_timer(); // TODO
	 pq.resort();
	 stop_queue_timer();

	 if (pq.empty())
	 set_best_val(fp_infinity);
	 else
	 set_best_val(comp.get_value(pq.front()));*/
}

template<class PQCompare>
void PQMultiheapOpenList<PQCompare>::verify(void) {
	assert(pq->heap_holds(0, pq->get_fill() - 1));
	assert(pq->indexes_match());
}

template<class PQCompare>
list<State*> *PQMultiheapOpenList<PQCompare>::states(void) {
	printf("PQMultiheap::states\n");
//	for (unsigned int i = 0; i < n_heaps; ++i) {
	return pq[0]->to_list(); // TODO
//	}
}
