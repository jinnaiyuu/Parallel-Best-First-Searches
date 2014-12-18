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
#include "pq_vector_open_list.h"

using namespace std;

/**
 * Create a new PQ open list.
 */
// template<class PQCompare>
	PQVectorOpenList::PQVectorOpenList(void)
: fill(0), min(0), pq(120)
{
}

/**
 * Add a state to the OpenList.
 * \param s The state to add.
 */
// template<class PQCompare>
void PQVectorOpenList::add(State *s)
{
//	printf("PQVectorOpenList::add\n");
//	printf("f,h = %lu, %lu\n", s->get_f()/10000, s->get_h()/10000);
	int p0 = s->get_f()/10000;
	assert(p0<120 && p0>=0);

	if (p0 >= static_cast<int>(pq.size())) {
//		printf("f going crazy.\n");
		return;
	}

	if (p0 < min)
		min = p0;

	pq[p0].push(s, static_cast<int>(s->get_g()/10000));
	fill++;
}

/**
 * Remove and return the state with the lowest f-value.
 * \return The front of the priority queue.
 */
// template<class PQCompare>
State *PQVectorOpenList::take(void)
{
//	printf("PQVectorOpenList::take\n");

	for ( ; (unsigned int) min < pq.size() && pq[min].empty() ; min++) {
		;
	}
	fill--;
	return pq[min].pop();
}

/**
 * Peek at the top element.
 */
// template<class PQCompare>
 State * PQVectorOpenList::peek(void)
{
	 // TODO: this function would not work well.
//	printf("PQVectorOpenList::peek\n");
	return pq[min].pop();
}

/**
 * Test if the OpenList is empty.
 * \return True if the open list is empty, false if not.
 */
// template<class PQCompare>
 bool PQVectorOpenList::empty(void)
{
	return (fill==0);
}

/**
 * Delete all of the states on the open list.
 */
// template<class PQCompare>
 void PQVectorOpenList::delete_all_states(void)
{
		printf("PQVectorOpenList::delete_all_states\n");
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
 void PQVectorOpenList::prune(void)
{
		printf("PQVectorOpenList::prune\n");
	// TODO: not sure what this function is for.
/*
	for (int i = 0; i < fill; i += 1)
		pq.get_elem(i)->set_open(false);

	pq.reset();
	set_size(0);
	*/
}

// template<class PQCompare>
 unsigned int PQVectorOpenList::size(void)
{
	return fill;
}

/**
 * Ensure that the heap propert holds.  This should be called after
 * updating states which are open.
 */
// template<class PQCompare>
	void PQVectorOpenList::see_update(State *s)
{
		printf("PQVectorOpenList::see_update\n");
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
	void PQVectorOpenList::remove(State *s)
{
	printf("PQVectorOpenList::remove\n");
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
void PQVectorOpenList::resort(void)
{
	printf("PQVectorOpenList::resort\n");
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
void PQVectorOpenList::verify(void)
{
	printf("PQVectorOpenList::verify\n");
	/*
	assert(pq.heap_holds(0, pq.get_fill() - 1));
	assert(pq.indexes_match());
	*/
}

// template<class PQCompare>
list<State*> *PQVectorOpenList::states(void)
{
	printf("PQVectorOpenList::states\n");
	return NULL;
//	return pq.to_list();
}
