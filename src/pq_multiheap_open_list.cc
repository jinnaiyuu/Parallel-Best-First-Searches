/* © 2014 the PBNF Authors under the MIT license. See AUTHORS for the list of authors.*/

/**
 * \file pq_open_list.h
 *
 * An open list class.
 *
 * \author Ethan Burns
 * \date 2008-10-09
 */

#if !defined(_PQ_OPEN_LIST_H_)
#define _PQ_OPEN_LIST_H_

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
 * \todo make this a bit more general.
 */
template<class PQCompare>
	class PQOpenList : public OpenList {
public:
	PQOpenList(void);
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
	double get_cpu_sum(void) { return cpu_time_sum; }
	unsigned long get_time_count(void) { return time_count; }
#endif
private:
	PriorityQueue<State *, PQCompare> pq;
	PQCompare get_index;
	PQCompare comp;

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
	PQOpenList<PQCompare>::PQOpenList(void)
:OpenList()
{
}

/**
 * Add a state to the OpenList.
 * \param s The state to add.
 */
template<class PQCompare>
void PQOpenList<PQCompare>::add(State *s)
{
	start_queue_timer();
	s->set_open(true);
	stop_queue_timer();

	pq.add(s);
	change_size(1);
	set_best_val(comp.get_value(pq.front()));
}

/**
 * Remove and return the state with the lowest f-value.
 * \return The front of the priority queue.
 */
template<class PQCompare>
State *PQOpenList<PQCompare>::take(void)
{
	State *s;

	start_queue_timer();
	s = pq.take();
	stop_queue_timer();

	s->set_open(false);
	change_size(-1);

	if (pq.empty())
		set_best_val(fp_infinity);
	else
		set_best_val(comp.get_value(pq.front()));

	return s;
}

/**
 * Peek at the top element.
 */
template<class PQCompare>
 State * PQOpenList<PQCompare>::peek(void)
{
	return pq.front();
}

/**
 * Test if the OpenList is empty.
 * \return True if the open list is empty, false if not.
 */
template<class PQCompare>
 bool PQOpenList<PQCompare>::empty(void)
{
	return pq.empty();
}

/**
 * Delete all of the states on the open list.
 */
template<class PQCompare>
 void PQOpenList<PQCompare>::delete_all_states(void)
{
	while (!pq.empty())
		delete pq.take();

	pq.reset();
	set_size(0);
}

/**
 * Prune all of the states.
 */
template<class PQCompare>
 void PQOpenList<PQCompare>::prune(void)
{
	int fill = pq.get_fill();

	for (int i = 0; i < fill; i += 1)
		pq.get_elem(i)->set_open(false);

	pq.reset();
	set_size(0);
}

template<class PQCompare>
 unsigned int PQOpenList<PQCompare>::size(void)
{
	return pq.get_fill();
}

/**
 * Ensure that the heap propert holds.  This should be called after
 * updating states which are open.
 */
template<class PQCompare>
	void PQOpenList<PQCompare>::see_update(State *s)
{
	start_queue_timer();
	pq.see_update(get_index(s));
	stop_queue_timer();

	set_best_val(comp.get_value(pq.front()));
}

/**
 * Remove the given state from the PQ.
 */
template<class PQCompare>
	void PQOpenList<PQCompare>::remove(State *s)
{
	start_queue_timer();
	pq.remove(get_index(s));
	stop_queue_timer();

	s->set_open(false);
	change_size(-1);
	if (pq.empty())
		set_best_val(fp_infinity);
	else
		set_best_val(comp.get_value(pq.front()));
}

/**
 * Resort the whole thing.
 */
template<class PQCompare>
void PQOpenList<PQCompare>::resort(void)
{
	start_queue_timer();
	pq.resort();
	stop_queue_timer();

	if (pq.empty())
		set_best_val(fp_infinity);
	else
		set_best_val(comp.get_value(pq.front()));
}

template<class PQCompare>
void PQOpenList<PQCompare>::verify(void)
{
	assert(pq.heap_holds(0, pq.get_fill() - 1));
	assert(pq.indexes_match());
}

template<class PQCompare>
list<State*> *PQOpenList<PQCompare>::states(void)
{
	return pq.to_list();
}

#endif	/* !_PQ_OPEN_LIST_H_ */
