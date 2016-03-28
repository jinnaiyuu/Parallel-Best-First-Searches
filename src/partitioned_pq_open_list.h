/* © 2014 the PBNF Authors under the MIT license. See AUTHORS for the list of authors.*/

/**
 * \file pq_open_list.h
 *
 * An open list class.
 *
 * \author Ethan Burns
 * \date 2008-10-09
 */

#if !defined(_PARTITIONED_PQ_OPEN_LIST_H_)
#define _PARTITIONED_PQ_OPEN_LIST_H_

#include <assert.h>

#include <list>
#include <limits>
#include <pthread.h>

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
 * \todo make this a bit more general.
 */

//#define unsinged unsigned
template<class PQCompare>
class PartitionedPQOpenList: public OpenList {
public:
	PartitionedPQOpenList(unsigned int openlistdivision);
	~PartitionedPQOpenList(void);

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
//	std::vector<PriorityQueue<State *, PQCompare> > pqs;
	std::vector<PQVectorOpenList> pqs;
	std::vector<pthread_mutex_t> ms;

//	PriorityQueue<PQVectorOpenList *, PQCompare> pqs;

	// Priority queue implemented in term of vector.
	std::vector<unsigned int> priorities;

	unsigned int openlistdivision;
	PQCompare get_index;
	PQCompare comp;

	unsigned int fill;


// Stats
	unsigned int add_locked;
	unsigned int take_locked;

#if defined(TIME_QUEUES)
	CPU_timer t;
	double cpu_time_sum;
	unsigned long time_count;
#endif
};

/**
 * Create a new PQ open list.
 */
template<class PQCompare>
PartitionedPQOpenList<PQCompare>::PartitionedPQOpenList(
		unsigned int openlistdivision) :
		OpenList(), pqs(openlistdivision), ms(openlistdivision), priorities(
				openlistdivision, fp_infinity), openlistdivision(
				openlistdivision), fill(0),
				add_locked(0), take_locked(0) {
	for (unsigned int i = 0; i < openlistdivision; ++i) {
		pthread_mutex_init(&(ms[i]), NULL);
//		pqs[i].changeSize(100);
	}

}

template<class PQCompare>
PartitionedPQOpenList<PQCompare>::~PartitionedPQOpenList(void) {
	printf("locked = %u, %u\n", add_locked, take_locked);
}

/**
 * Add a state to the OpenList.
 * \param s The state to add.
 */
template<class PQCompare>
void PartitionedPQOpenList<PQCompare>::add(State *s) {
	start_queue_timer();
	s->set_open(true);
	stop_queue_timer();
	unsigned int dist = s->dist_hash() % openlistdivision;
//	printf("add: %u\n", dist);

	unsigned int old_f = fp_infinity;
	if (!pqs[dist].empty()) {
		old_f = pqs[dist].peek()->get_f();
	}
	if (pthread_mutex_trylock(&(ms[dist])) != 0) {
		++add_locked;
		pthread_mutex_lock(&(ms[dist]));
	}
//	printf("locked\n");
	pqs[dist].add(s);
//	printf("added\n");
	++fill;
	priorities[dist] = pqs[dist].peek()->get_priority();

	if (get_best_val() != pqs[dist].peek()->get_f()) {
		unsigned int min = fp_infinity;
		for (unsigned int i = 0; i < openlistdivision; ++i) {
//			printf("priorites = %u\n", priorities[i]);
			if (min > priorities[i]) {
				min = priorities[i];
				dist = i;
			}
		}
	}
	set_best_val((pqs[dist]).peek()->get_f());
	pthread_mutex_unlock(&(ms[dist]));
//	printf("unlocked\n");

//	change_size(1);

//	printf("dist = %u\n", dist);
//	if (pqs[dist].empty()) {
////		printf("empty\n");
//		set_best_val(fp_infinity);
//	} else {
////		printf("not empty\n");
//		State* f = (pqs[dist]).peek();
////		printf("front\n");
//		if (f == NULL) {
////			printf("f is NULL\n");
//		}
////		printf("%u\n", f->get_priority());
//		set_best_val(f->get_f());
//	}
//	set_best_val(comp.get_value(pq.front()));
//	printf("add end\n");
}

/**
 * Remove and return the state with the lowest f-value.
 * \return The front of the priority queue.
 */
template<class PQCompare>
State *PartitionedPQOpenList<PQCompare>::take(void) {

	State *s;

	start_queue_timer();
	unsigned int dist = 0;
	fp_type min = fp_infinity;

	// Going to be improved.
	for (unsigned int i = 0; i < openlistdivision; ++i) {
		if (min > priorities[i]) {
			min = priorities[i];
			dist = i;
		}
	}

	printf("take: %u, priority = %u\n", dist, pqs[dist].peek()->get_priority());

	if (pthread_mutex_trylock(&ms[dist]) != 0) {
		++take_locked;
		pthread_mutex_lock(&ms[dist]);
	}
	s = pqs[dist].take();
	--fill;
	if (pqs[dist].empty()) {
		priorities[dist] = fp_infinity;
	} else {
		priorities[dist] = pqs[dist].peek()->get_priority();
	}

	// Set best f value.
	if (empty()) {
		set_best_val(fp_infinity);
	} else {
		set_best_val(comp.get_value(peek()));
	}

	pthread_mutex_unlock(&ms[dist]);
	stop_queue_timer();

	s->set_open(false);

//	change_size(-1);

// TODO: set best val

	return s;
}

/**
 * Peek at the top element.
 */
template<class PQCompare>
State * PartitionedPQOpenList<PQCompare>::peek(void) {
//	printf("peek\n");
	unsigned int dist = 0;
	fp_type min = fp_infinity;
	for (unsigned int i = 0; i < openlistdivision; ++i) {
		if (!(pqs[i].empty()) && (min > priorities[i])) {
			min = priorities[i];
			dist = i;
		}
	}
	printf("peek dist = %u\n", dist);

	// If no node
	if (min == fp_infinity && pqs[dist].empty()) {
		printf("empty peek\n");
		return NULL;
	}
//	printf("front\n");
	return pqs[dist].peek();
}

/**
 * Test if the OpenList is empty.
 * \return True if the open list is empty, false if not.
 */
template<class PQCompare>
bool PartitionedPQOpenList<PQCompare>::empty(void) {
//	printf("empty\n");
	for (unsigned int i = 0; i < openlistdivision; ++i) {
		if (!pqs[i].empty()) {
			return false;
		}
	}
	return true;
}

/**
 * Delete all of the states on the open list.
 */
template<class PQCompare>
void PartitionedPQOpenList<PQCompare>::delete_all_states(void) {
	for (unsigned int i = 0; i < openlistdivision; ++i) {
		while (!pqs[i].empty())
			delete pqs[i].take();
//		pqs[i].reset();
	}
	set_size(0);
}

/**
 * Prune all of the states.
 */
template<class PQCompare>
void PartitionedPQOpenList<PQCompare>::prune(void) {
	printf("prune\n");
//	int fill = pq.get_fill();
//
//	for (int i = 0; i < fill; i += 1)
//		pq.get_elem(i)->set_open(false);
//
//	pq.reset();
//	set_size(0);
}

template<class PQCompare>
unsigned int PartitionedPQOpenList<PQCompare>::size(void) {
	return fill;
}

/**
 * Ensure that the heap propert holds.  This should be called after
 * updating states which are open.
 */
template<class PQCompare>
void PartitionedPQOpenList<PQCompare>::see_update(State *s) {
	printf("see_update\n");
//	start_queue_timer();
//	pq.see_update(get_index(s));
//	stop_queue_timer();
//
//	set_best_val(comp.get_value(pq.front()));
}

/**
 * Remove the given state from the PQ.
 */
template<class PQCompare>
void PartitionedPQOpenList<PQCompare>::remove(State *s) {
	printf("remove\n");
//	start_queue_timer();
//	pq.remove(get_index(s));
//	stop_queue_timer();
//
//	s->set_open(false);
//	change_size(-1);
//	if (pq.empty())
//		set_best_val(fp_infinity);
//	else
//		set_best_val(comp.get_value(pq.front()));
}

/**
 * Resort the whole thing.
 */
template<class PQCompare>
void PartitionedPQOpenList<PQCompare>::resort(void) {
	printf("resort\n");
//	start_queue_timer();
//	pq.resort();
//	stop_queue_timer();
//
//	if (pq.empty())
//		set_best_val(fp_infinity);
//	else
//		set_best_val(comp.get_value(pq.front()));
}

template<class PQCompare>
void PartitionedPQOpenList<PQCompare>::verify(void) {
	printf("verify\n");
//	assert(pq.heap_holds(0, pq.get_fill() - 1));
//	assert(pq.indexes_match());
}

template<class PQCompare>
list<State*> *PartitionedPQOpenList<PQCompare>::states(void) {
//	printf("states\n");
//	return pqs[0].to_list();
}

#endif	/* !_PQ_OPEN_LIST_H_ */
