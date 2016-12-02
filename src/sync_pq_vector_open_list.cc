/* © 2014 the PBNF Authors under the MIT license. See AUTHORS for the list of authors.*/

/**
 * \file pq_open_list.h
 *
 * An open list class.
 *
 * \author Ethan Burns
 * \date 2008-10-09
 */

//#include <assert.h>
/*
#include <list>
#include <limits>

#include "state.h"
#include "open_list.h"
#include "util/priority_queue.h"
#include "util/cpu_timer.h"
*/
#include <pthread.h>

#include "sync_pq_vector_open_list.h"

using namespace std;

/**
 * Create a new PQ open list.
 */
// template<class PQCompare>
	SyncPQVectorOpenList::SyncPQVectorOpenList(void)
: fill(0), min(0), pq(120)
{
		pthread_mutex_init(&mutex, NULL);
}

void SyncPQVectorOpenList::changeSize(unsigned int size)
{
	// This instruction would take a bit of time.
	pq.resize(size);
}

/**
 * Add a state to the OpenList.
 * \param s The state to add.
 */
// template<class PQCompare>
void SyncPQVectorOpenList::add(State *s)
{
	pthread_mutex_lock(&mutex);
//	printf("SyncPQVectorOpenList::add\n");
//	printf("f,h = %lu, %lu\n", s->get_f()/10000, s->get_h()/10000);
	int p0 = s->get_f()/10000;
	assert(p0<120 && p0>=0);

	if (p0 >= static_cast<int>(pq.size())) {
//		printf("f going crazy.\n");
		pthread_mutex_unlock(&mutex);
		return;
	}

	if (p0 < min)
		min = p0;

	pq[p0].push(s, static_cast<int>(s->get_g()/10000));
	fill++;
	pthread_mutex_unlock(&mutex);

}

/**
 * Remove and return the state with the lowest f-value.
 * \return The front of the priority queue.
 */
// template<class PQCompare>
State *SyncPQVectorOpenList::take(void)
{
	pthread_mutex_lock(&mutex);

//	printf("SyncPQVectorOpenList::take\n");

	for ( ; (unsigned int) min < pq.size() && pq[min].empty() ; min++) {
		;
	}
	fill--;
	State* ret = pq[min].pop();
	pthread_mutex_unlock(&mutex);

	return ret;
}

/**
 * Peek at the top element.
 */
// template<class PQCompare>
State * SyncPQVectorOpenList::peek(void)
{
	pthread_mutex_lock(&mutex);
	State* ret = pq[min].pop();
	 // TODO: this function would not work well.
//	printf("SyncPQVectorOpenList::peek\n");
	pthread_mutex_unlock(&mutex);
	return ret;
}

/**
 * Test if the OpenList is empty.
 * \return True if the open list is empty, false if not.
 */
// template<class PQCompare>
bool SyncPQVectorOpenList::empty(void)
{
	pthread_mutex_lock(&mutex);
	bool ret = (fill==0);
	pthread_mutex_unlock(&mutex);
	return ret;
}

/**
 * Delete all of the states on the open list.
 */
// template<class PQCompare>
 void SyncPQVectorOpenList::delete_all_states(void)
{
		printf("SyncPQVectorOpenList::delete_all_states\n");
	// TODO: Need to implement in term of vector<vector<state*>>
//	while (!pq.empty())
//		delete pq.take();

//	pq.reset();
	set_size(0);
}

/**
 * Prune all of the states.
 */
// template<class PQCompare>
 void SyncPQVectorOpenList::prune(void)
{
	//printf("SyncPQVectorOpenList::prune\n");
	// TODO: not sure what this function is for.
/*
	for (int i = 0; i < fill; i += 1)
		pq.get_elem(i)->set_open(false);

	pq.reset();
	set_size(0);
	*/
}

// template<class PQCompare>
 unsigned int SyncPQVectorOpenList::size(void)
{
	return fill;
}

/**
 * Ensure that the heap propert holds.  This should be called after
 * updating states which are open.
 */
// template<class PQCompare>
	void SyncPQVectorOpenList::see_update(State *s)
{
		printf("SyncPQVectorOpenList::see_update\n");
	/*
	start_queue_timer();
	pq.see_update(get_index(s));
	stop_queue_timer();

	set_best_val(comp.get_value(pq.front()));
	*/
}

/**
 * Remove the given state from the PQ.
 */
// template<class PQCompare>
	void SyncPQVectorOpenList::remove(State *s)
{
	printf("SyncPQVectorOpenList::remove\n");
	// TODO: Why would you need this function?
	/*
	start_queue_timer();
	pq.remove(get_index(s));
	stop_queue_timer();

	s->set_open(false);
	change_size(-1);
	if (pq.empty())
		set_best_val(fp_infinity);
	else
		set_best_val(comp.get_value(pq.front()));
		*/
}

/**
 * Resort the whole thing.
 */
// template<class PQCompare>
void SyncPQVectorOpenList::resort(void)
{
	printf("SyncPQVectorOpenList::resort\n");
	// TODO: What is this function for?
	/*
	start_queue_timer();
	pq.resort();
	stop_queue_timer();

	if (pq.empty())
		set_best_val(fp_infinity);
	else
		set_best_val(comp.get_value(pq.front()));
	 */
}

// template<class PQCompare>
void SyncPQVectorOpenList::verify(void)
{
	printf("SyncPQVectorOpenList::verify\n");
	/*
	assert(pq.heap_holds(0, pq.get_fill() - 1));
	assert(pq.indexes_match());
	*/
}

// template<class PQCompare>
list<State*> *SyncPQVectorOpenList::states(void)
{
	printf("SyncPQVectorOpenList::states\n");
	return NULL;
//	return pq.to_list();
}
