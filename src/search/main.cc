/* -*- mode:linux -*- */
/**
 * \file main.cc
 *
 *
 *
 * \author Ethan Burns
 * \date 2008-10-08
 */

#include <iostream>
#include <vector>
#include <string.h>
#include <stdlib.h>

#include "state.h"
#include "a_star.h"
#include "cost_bound_dfs.h"
#include "ida_star.h"
#include "kbfs.h"
#include "psdd/psdd.h"
#include "h_zero.h"
#include "grid/grid_world.h"

using namespace std;

Search *get_search(int argc, char *argv[])
{
	unsigned int threads;

	if (argc > 1 && strcmp(argv[1], "astar") == 0) {
		return new AStar();
	} else if (argc > 1 && strcmp(argv[1], "idastar") == 0) {
		return new IDAStar();
	} else if (argc > 2 && strcmp(argv[1], "costbounddfs") == 0) {
		return new CostBoundDFS(atoi(argv[2]));
	} else if (argc > 1 && strcmp(argv[1], "kbfs") == 0) {
		return new KBFS();
	} else if (argc > 1 && sscanf(argv[1], "psdd-%u", &threads) == 1) {
		return new PSDD(threads);
	} else {
		cout << "Must supply a search algorithm:" << endl;
		cout << "\tastar, idastar, costbounddfs, kbfs, psdd-<threads>"
		     << endl;
		exit(EXIT_FAILURE);
	}
}

int main(int argc, char *argv[])
{
	vector<const State *> *path;
	Search *search = get_search(argc, argv);
	GridWorld g(cin);
	GridWorld::ManhattanDist manhattan(&g);
	GridWorld::RowModProject project(&g, g.get_height());
	HZero hzero(&g);

	g.set_heuristic(&manhattan);
//	g.set_heuristic(&hzero);
	g.set_projection(&project);
	path = search->search(g.initial_state());
//	g.print(cout, path);

	cout << "generated: " << search->get_generated() << endl;
	cout << "expanded: " << search->get_expanded() << endl;
	if (path) {
		cout << "cost: " << path->at(0)->get_g() << endl;
		for (unsigned int i = 0; i < path->size(); i += 1)
			delete path->at(i);
		delete path;
	} else {
		cout << "No Solution" << endl;
	}

	delete search;

#if ENABLE_IMAGES
	g.export_eps("output.eps");
#endif	// ENABLE_IMAGES

	return EXIT_SUCCESS;
}
