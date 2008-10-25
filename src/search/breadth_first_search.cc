/* -*- mode:linux -*- */
/**
 * \file breadth_first_search.cc
 *
 *
 *
 * \author Ethan Burns
 * \date 2008-10-25
 */

#include <assert.h>
#include <math.h>		// INFINITY

#include "breadth_first_search.h"
#include "state.h"
#include "closed_list.h"
#include "queue_open_list.h"

BreadthFirstSearch::BreadthFirstSearch(void)
	: bound(INFINITY) {}

BreadthFirstSearch::BreadthFirstSearch(float bound)
	: bound(bound) {}

BreadthFirstSearch::~BreadthFirstSearch(void) {}

vector<const State *> *BreadthFirstSearch::search(const State *init)
{
	vector<const State *> *path = NULL;
	QueueOpenList open;
	ClosedList closed;

	open.add(init);

	while (!open.empty() && !path) {
		const State *s = open.take();
		const State *dup = closed.lookup(s);

		if (s->get_f() > bound) {
			cout << "Deleting out of bound" << endl;
			delete s;
			continue;
		}

		if (dup) {
			assert(dup->get_g() <= s->get_g());
			delete s;
			continue;
		}

		closed.add(s);

		if (s->is_goal()) {
			path = s->get_path();
			break;
		}

		vector<const State *> *children = expand(s);
		for (unsigned int i = 0; i < children->size(); i += 1) {
			open.add(children->at(i));
		}
		delete children;
	}

	closed.delete_all_states();
	open.delete_all_states();

	return path;
}