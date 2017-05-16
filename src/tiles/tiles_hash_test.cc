// © 2014 the PBNF Authors under the MIT license. See AUTHORS for the list of authors.

/**
 * \file tiles_hash_test.cc
 *
 *
 *
 * \author Ethan Burns
 * \date 2008-11-03
 */

#include <stdlib.h>

#include <vector>
#include <map>
#include <iostream>

#include "../queue_open_list.h"
#include "../closed_list.h"
#include "tiles.h"
#include "tiles_state.h"

using namespace std;

int main(void)
{
	map<unsigned int, State *> seen; // check for duplicate hashes
	QueueOpenList open;
	ClosedList closed;
	Tiles dom(cin);
	Tiles::ManhattanDist m(&dom);
	State *s;
	dom.set_heuristic(&m);

	vector<unsigned int> vec(9);
	vec[0] = 0;
	vec[1] = 1;
	vec[2] = 2;
	vec[3] = 3;
	vec[4] = 4;
	vec[5] = 5;
	vec[6] = 6;
	vec[7] = 7;
	vec[8] = 8;

	open.add(dom.initial_state());

	unsigned int expanded = 0;
	while (!open.empty()) {
		s = open.take();

		if (closed.lookup(s) != NULL) {
			delete s;
			continue;
		}

		expanded += 1;
		closed.add(s);
		cout << "--------------------" << endl;
		cout << "Expanding: " << endl;
		s->print(cout);

		if (seen.find(s->hash()) != seen.end()) {
			cerr << "Duplicate hash: " << endl;
			s->print(cerr);
			seen.find(s->hash())->second->print(cerr);
			return EXIT_FAILURE;
		} else {
			seen[s->hash()] = s;
		}

		vector<State *> *children = s->expand();
		vector<State *>::iterator iter;

		for (iter = children->begin();
		     iter != children->end();
		     iter++) {
			open.add(*iter);
			cout << endl;
			(*iter)->print(cout);
		}
		delete children;
	}

	closed.delete_all_states();

	cout << "Expanded " << expanded << endl;

	return EXIT_SUCCESS;
}
