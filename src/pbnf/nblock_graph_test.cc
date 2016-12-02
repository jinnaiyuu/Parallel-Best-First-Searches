// © 2014 the PBNF Authors under the MIT license. See AUTHORS for the list of authors.

/**
 * \file nblock_graph_test.cc
 *
 *
 *
 * \author Ethan Burns
 * \date 2008-10-24
 */

#include <iostream>
#include <vector>

#include "nblock_graph.h"
#include "nblock.h"

#include "../projection.h"
#include "../open_list.h"
#include "../closed_list.h"
#include "../grid/grid_world.h"

using namespace std;
using namespace PBNF;

int main(void)
{
	GridWorld w(cin);
	GridWorld::ManhattanDist heur(&w);
	w.set_heuristic(&heur);
	GridWorld::RowModProject p(&w, w.get_height());
	NBlockGraph g(&p, w.initial_state());
	NBlock *n;
	State *s;
	vector<State *> *children;
	vector<State *>::iterator iter;

	g.print(cout);

	cout << "------------------------------------------------------------" << endl;
	cout << endl << endl << endl << endl;
	n = g.next_nblock(NULL, false);
	cout << "Got NBlock:" << endl;
	n->print(cout);
	cout << endl << endl << endl << endl;
	cout << "------------------------------------------------------------" << endl;
	cout << endl;
	g.print(cout);

	s = n->open.take();
	children = s->expand();
	cout << endl << endl << endl << endl;
	cout << "------------------------------------------------------------" << endl;
	cout << "Children:" << endl;
	for (iter = children->begin(); iter != children->end(); iter++) {
		NBlock *blk = g.get_nblock(p.project(*iter));

		(*iter)->print(cout);
		blk->open.add(*iter);
	}
	cout << "------------------------------------------------------------" << endl;
	cout << endl << endl << endl << endl;
	n = g.next_nblock(n, false);
	cout << "Got NBlock:" << endl;
	n->print(cout);
	cout << endl << endl << endl << endl;
	cout << "------------------------------------------------------------" << endl;
	cout << endl;
	g.print(cout);

	return 0;
}
