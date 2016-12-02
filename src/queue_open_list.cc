// © 2014 the PBNF Authors under the MIT license. See AUTHORS for the list of authors.

/**
 * \file queue_open_list.cc
 *
 *
 *
 * \author Ethan Burns
 * \date 2008-10-20
 */

#include <limits>
#include <list>

#include "queue_open_list.h"
#include "state.h"

using namespace std;

QueueOpenList::QueueOpenList()
 :OpenList()
{
}

void QueueOpenList::add(State *s)
{
	s->set_open(true);
	q.push_front(s);
	set_best_val(q.front()->get_f());
}

State *QueueOpenList::take(void)
{
	State *s = q.front();

	s->set_open(false);
	q.pop_front();
	if (q.empty())
		set_best_val(fp_infinity);
	else
		set_best_val(q.front()->get_f());


	return s;
}

State *QueueOpenList::peek(void)
{
	return q.front();
}

bool QueueOpenList::empty(void)
{
	return q.empty();
}

void QueueOpenList::delete_all_states(void)
{
	State *s;

	while (!q.empty()) {
		s = q.front();
		q.pop_front();
		delete s;
	}
}

void QueueOpenList::prune(void)
{
	while (!q.empty())
		q.pop_front();
}

unsigned int QueueOpenList::size(void)
{
	return q.size();
}

list<State*> *QueueOpenList::states(void)
{
	list<State*> *lst = new list<State*>(q.begin(), q.end());
	return lst;
}
