// © 2014 the PBNF Authors under the MIT license. See AUTHORS for the list of authors.

/**
 * \file breadth_first_search.cc
 *
 *
 *
 * \author Ethan Burns
 * \date 2008-10-25
 */

#include <assert.h>

#include <limits>

#include "breadth_first_search.h"
#include "state.h"
#include "closed_list.h"
#include "queue_open_list.h"

using namespace std;

BreadthFirstSearch::BreadthFirstSearch(void)
	: bound(fp_infinity) {}

BreadthFirstSearch::BreadthFirstSearch(fp_type bound)
	: bound(bound) {}

BreadthFirstSearch::~BreadthFirstSearch(void) {}

vector<State *> *BreadthFirstSearch::search(Timer *t, State *init)
{
	vector<State *> *path = NULL;
	QueueOpenList open;
	ClosedList closed;

	open.add(init);

	while (!open.empty() && !path) {
		State *s = open.take();
		State *dup = closed.lookup(s);

		if (s->get_f() > bound) {
			cout << "Deleting out of bound" << endl;
			delete s;
			continue;
		}

		if (dup) {
			delete s;
			continue;
		}

		closed.add(s);

		if (s->is_goal()) {
			path = s->get_path();
			break;
		}

		vector<State *> *children = expand(s);
		for (unsigned int i = 0; i < children->size(); i += 1) {
			open.add(children->at(i));
		}
		delete children;
	}

	closed.delete_all_states();
	open.delete_all_states();

	return path;
}
