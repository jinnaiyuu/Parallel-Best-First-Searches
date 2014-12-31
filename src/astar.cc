// © 2014 the PBNF Authors under the MIT license. See AUTHORS for the list of authors.

/**
 * \file a_star.cc
 *
 *
 *
 * \author Ethan Burns
 * \date 2008-10-09
 */

#include <assert.h>

#include "astar.h"

AStar::AStar(void) : dd(false) { }

AStar::AStar(bool d) : dd(d) { }

AStar::~AStar(void)
{
	closed.delete_all_states();
}


/**
 * Perform an A* search.
 */
vector<State *> *AStar::search(Timer *t, State *init)
{
	vector<State *> *path = NULL;

	open.add(init);

	while (!open.empty() && !path) {
//		printf("astar::search loop\n");

		State *s = open.take();

		if (s->is_goal()) {
//			printf("is_goal");
			path = s->get_path();
			break;
		}

		vector<State *> *children = expand(s);
		for (unsigned int i = 0; i < children->size(); i += 1) {
			State *c = children->at(i);
			State *dup = closed.lookup(c);
			if (dup) {
				if (dup->get_g() > c->get_g()) {
					dup->update(c->get_parent(), c->get_c(), c->get_g());
					if (dup->is_open())
						open.see_update(dup);
					else if (!dd)
						open.add(dup);
				}
				delete c;
			} else {
				open.add(c);
				closed.add(c);
			}

		}
		delete children;
	}

	return path;
}

void AStar::output_stats(void)
{
	open.print_stats(cout);
	cout << "total-time-acquiring-locks: 0" << endl;
	cout << "average-time-acquiring-locks: 0" << endl;
	cout << "total-time-waiting: 0" << endl;
	cout << "average-time-waiting: 0" << endl;
}
