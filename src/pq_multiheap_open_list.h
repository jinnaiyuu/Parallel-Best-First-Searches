/* © 2014 the PBNF Authors under the MIT license. See AUTHORS for the list of authors.*/

/**
 * \file pq_open_list.h
 *
 * An open list class.
 *
 * \author Ethan Burns
 * \date 2008-10-09
 */

#if !defined(_PQ_MULTIHEAP_OPEN_LIST_H_)
#define _PQ_MULTIHEAP_OPEN_LIST_H_

#include <assert.h>

#include <list>
#include <limits>

#include "state.h"
#include "open_list.h"
#include "util/priority_queue.h"
#include "util/cpu_timer.h"

using namespace std;

#if defined(TIME_QUEUES)
#define start_queue_timer() do { t.start(); } while (0)
#define stop_queue_timer()				\
	do {						\
		t.stop();				\
		time_count += 1;			\
		cpu_time_sum += t.get_time(); \
	} while (0)
#else
#define start_queue_timer()
#define stop_queue_timer()
#endif // TIME_QUEUES
/**
 * A priority queue for states based on their f(s) = g(s) + h(s)
 * value.
 *
 */
template<class PQCompare>
class PQMultiheapOpenList: public OpenList {
public:
	PQMultiheapOpenList(unsigned int n_threads, unsigned int n_heaps);
	~PQMultiheapOpenList(void);
	void add(State *s);
	State *take(void);
	State *peek(void);
	bool empty(void);
	void delete_all_states(void);
	void prune(void);

	list<State*> *states(void);

	unsigned int size(void);
	void remove(State *s);
	void see_update(State *s);
	void resort();

	/* Verify the heap property holds */
	void verify();

#if defined(TIME_QUEUES)
	double get_cpu_sum(void) {return cpu_time_sum;}
	unsigned long get_time_count(void) {return time_count;}
#endif
private:
	fp_type get_priority(State* s);
	unsigned int n_threads;
	unsigned int n_heaps;
	vector<PriorityQueue<State *, PQCompare>*> pq;
	fp_type best_priority;
	unsigned int best_heap;
	PQCompare get_index;
	PQCompare comp;

	vector<fp_type> fronts;
	vector<unsigned int> distribution;

#if defined(TIME_QUEUES)
	CPU_timer t;
	double cpu_time_sum;
	unsigned long time_count;
#endif
};

template<class PQCompare>
PQMultiheapOpenList<PQCompare>::PQMultiheapOpenList(unsigned int n_threads,
		unsigned int n_heaps) :
		OpenList(), n_threads(n_threads), n_heaps(n_heaps), best_priority(
				fp_infinity) {
//	printf("PQMultiheap constructor\n");
//	printf("n_threads, n_heaps = %u, %u\n", this->n_threads, this->n_heaps);
	set_best_val(fp_infinity);
	pq.resize(n_heaps);

	for (unsigned int i = 0; i < n_heaps; ++i) {
		pq[i] = new PriorityQueue<State *, PQCompare>();
	}

	fronts.resize(n_heaps);
	for (unsigned int i = 0; i < n_heaps; ++i) {
		fronts[i] = fp_infinity;
	}

	distribution.resize(n_heaps);
	for (unsigned int i = 0; i < n_heaps; ++i) {
		distribution[i] = 0;
	}
//	printf("pq[0]->fill = %u\n", pq[0]->get_fill());
}

template<class PQCompare>
PQMultiheapOpenList<PQCompare>::~PQMultiheapOpenList(void) {
	for (unsigned int i = 0; i < n_heaps; ++i) {
		printf("%u ", distribution[i]);
	}
	printf("\n");
}

/**
 * Add a state to the OpenList.
 * \param s The state to add.
 */
template<class PQCompare>
void PQMultiheapOpenList<PQCompare>::add(State *s) {
//	printf("PQMultiheap::add\n");
	unsigned int which_heap = (s->dist_hash() / n_threads) % n_heaps;
//	printf("which_heap = %u\n", which_heap);
	start_queue_timer();
	s->set_open(true);
	stop_queue_timer();

	pq[which_heap]->add(s);
	change_size(1);

//	printf("f,h = %lu, %lu\n", s->get_h(), s->get_f());
	/*	printf("f+h = %lu\n", get_priority(s));
	 printf("best priority = %lu\n", best_priority);
	 printf("best = %lu\n", get_best_val());
	 printf("best heap = %u\n", best_heap);*/


/*	if (get_priority(s) < best_priority) {
		best_priority = get_priority(s);
		set_best_val(comp.get_value(s));
		best_heap = which_heap;
	}*/

	// TODO: Why this don't work?
/*	fp_type best = fp_infinity;
	unsigned int recal_best_heap = -1;
	for (unsigned int i = 0; i < n_heaps; ++i) {
		if (!pq[i]->empty() && best > get_priority(pq[i]->front())) {
			best = get_priority(pq[i]->front());
			recal_best_heap = i;
			set_best_val(comp.get_value(pq[i]->front()));
		}
	}
	best_priority = best;
	best_heap = recal_best_heap;*/


	/*	if (best_heap != recal_best_heap) {
		printf("best_heap, actual_best = %u, %u\n", best_heap, recal_best_heap);
	}*/
	/*
	 fp_type best = fp_infinity;
	 for (unsigned int i = 0; i < n_heaps; ++i) {
	 if (!pq[i]->empty() && (best > get_priority(pq[i]->front()))) {
	 best = get_priority(pq[i]->front());
	 best_heap = i;
	 }
	 }
	 set_best_val(best);
	 */

	/*
	 if (get_best_val() > comp.get_value(s)) {
	 set_best_val(comp.get_value(s));
	 best_heap = which_heap;
	 }
	 */
//	printf("best_heap = %u\n", best_heap);
}

/**
 * Remove and return the state with the lowest f-value.
 * \return The front of the priority queue.
 */
template<class PQCompare>
State *PQMultiheapOpenList<PQCompare>::take(void) {
//	printf("PQMultiheap::take\n");
//	printf("best_heap = %u\n", best_heap);
	State *s;
//	printf("pq size = %u\n", pq[best_heap]->get_fill());
	start_queue_timer();
//	for (unsigned int i = 0; i < n_heaps; ++i) {

	fp_type best = fp_infinity;
//	unsigned int recal_best_heap = -1;
	for (unsigned int i = 0; i < n_heaps; ++i) {
		if (!pq[i]->empty() && best > get_priority(pq[i]->front())) {
			best = get_priority(pq[i]->front());
			best_heap = i;
		}
	}-
/*	if (best_heap != recal_best_heap) {
		printf("best_heap, actual_best = %u, %u\n", best_heap, recal_best_heap);
	}*/
	s = pq[best_heap]->take(); // TODO: check
//	}
	if (!s) {
//		printf("s is NULL\n");
		return s;
	}

//	printf("taken\n");
//	stop_queue_timer();
	s->set_open(false);
//	change_size(-1);
//	printf("iterate over heaps\n");
	if (get_priority(s) <= best_priority) {
		fp_type best = fp_infinity;
		for (unsigned int i = 0; i < n_heaps; ++i) {
			if (!pq[i]->empty() && (best > get_priority(pq[i]->front()))) {
				best = get_priority(pq[i]->front());
				best_heap = i;
			}
		}
		best_priority = best;
		if (empty()) {
			set_best_val(fp_infinity);
		} else {
			set_best_val(comp.get_value(pq[best_heap]->front()));
		}
	}
//	printf("return take\n");
	++distribution[best_heap];
	return s;
}

/**
 * Peek at the top element.
 */
template<class PQCompare>
State * PQMultiheapOpenList<PQCompare>::peek(void) {
//	printf("PQMultiheap::peek\n");
	return pq[best_heap]->front(); // TODO chk
}

/**
 * Test if the OpenList is empty.
 * \return True if the open list is empty, false if not.
 */
template<class PQCompare>
bool PQMultiheapOpenList<PQCompare>::empty(void) {
//	printf("PQMultiheap::empty\n");
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
//	printf("PQMultiheap::delete_all_states\n");
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
//	printf("PQMultiheapOpenList::prune\n");

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
///	printf("PQMultiheap::size\n");
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
//	printf("PQMultiheap::see_update\n");
	/*
	 start_queue_timer();
	 pq[best_heap]->see_update(get_index(s)); // TODO
	 stop_queue_timer();
	 */

	fp_type best = fp_infinity;
	for (unsigned int i = 0; i < n_heaps; ++i) {
		if (!pq[i]->empty() && best > get_priority(pq[i]->front())) {
			best = get_priority(pq[i]->front());
			best_heap = i;
		}
	}
	best_priority = best;
	set_best_val(comp.get_value(pq[best_heap]->front()));

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

template<class PQCompare>
fp_type PQMultiheapOpenList<PQCompare>::get_priority(State* s) {
	return s->get_f() * 10 + s->get_h() / 10000;
}

#endif	/* !_PQ_OPEN_LIST_H_ */
