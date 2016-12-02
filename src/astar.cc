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
#include "tsp/tsp_state.h"

#include "astar.h"

AStar::AStar(void) : dd(false) { }

AStar::AStar(bool d) : dd(d) { }

AStar::AStar(unsigned int closedlistsize) : dd(false), closed(closedlistsize) { }

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
		State *s = open.take();

		if (s->is_goal()) {
//			printf("is_goal");
			path = s->get_path();
			break;
		}

		vector<State *> *children = expand(s);
//		printf("%u children\n", children->size());
		for (unsigned int i = 0; i < children->size(); i += 1) {
//			printf("i = %u\n", i);
			State *c = children->at(i);
//			printf("c->hash = %u\n", c->hash());
			State *dup = closed.lookup(c);
//			if (!dup) {
//				printf("dup NULL\n");
//			} else {
//				printf("dup ok");
//			}
			if (dup) {
//				printf("duped\n");

				if (dup->get_g() > c->get_g()) {
				  // TODO: count how much duplication occurs
					dup->update(c->get_parent(), c->get_c(), c->get_g());
					if (dup->is_open())
						open.see_update(dup);
					else if (!dd)
						open.add(dup);
				}
//				printf("del\n");

				delete c;
//				printf("deleted\n");
			} else {
				open.add(c);
				closed.add(c);
			}

		}
//		printf("expddone\n");
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
