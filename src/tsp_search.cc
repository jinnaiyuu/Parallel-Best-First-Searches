// © 2014 the PBNF Authors under the MIT license. See AUTHORS for the list of authors.

/**
 * \file tiles_search.cc
 *
 *
 *
 * \author Ethan Burns
 * \date 2008-11-11
 */

#include <assert.h>
#include <math.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include <iostream>
#include <vector>

#include "get_search.h"
#include "search.h"
#include "state.h"
#include "h_zero.h"
#include "tsp/tsp.h"
#include "util/timer.h"
#include "util/timeout.h"
#include "div_merge_project.h"

using namespace std;

int main(int argc, char *argv[]) {
	unsigned int timelimit = 10000; // seconds
	vector<State *> *path;
	printf("get_search\n");
	Search *search = get_search(argc, argv);

	if (strcmp(argv[1], "arastar") == 0) { // hack to peal off extra ARA* argument
		argv++;
		argc--;
	}
	//	string cost = argc == 2 ? "unit" : argv[2];
	Tsp g(cin);
	Timer timer;
	Projection *project;

	printf("projection\n");
	// TODO: How can you abstract TSP properly?
	if (nblocks == 0) {
		project = NULL;
	} else if (nblocks == 1 || nblocks == 240) {
		project = new Tsp::RowModProject(&g, nblocks);
	} else if (nblocks == 2 || nblocks == 3360) {
		project = new Tsp::RowModProject(&g, nblocks);
	} else if (nblocks == 3 || nblocks == 43680) {
		project = new Tsp::RowModProject(&g, nblocks);
/*	} else if (nblocks == 123) {
		project = new Tiles::TwoTileNoBlankProject(&g, 1, 2, 3);*/
	} else {
		cerr << "Invalid abstraction size: " << nblocks << endl;
		cerr << "15-puzzle: 240=1tile, 3360=2tile" << endl;
		exit(EXIT_FAILURE);
	}

	g.set_projection(project);
	printf("MSP\n");
	Tsp::MinimumSpanningTree msp(&g);
	g.set_heuristic(&msp);

	printf("heuristic done\n");

#if defined(NDEBUG)
	timeout(timelimit);
#endif	// NDEBUG
	timer.start();
	State* inits = g.initial_state();
	printf("start search\n");
	path = search->search(&timer, inits);
	timer.stop();

#if defined(NDEBUG)
	timeout_stop();
#endif	// NDEBUG
	search->output_stats();

	if (path) {
		printf("cost: %f\n", (double) path->at(0)->get_g() / fp_one);
		cout << "length: " << path->size() << endl;

		for (unsigned int i = 0; i < path->size(); i += 1)
			delete path->at(i);
		delete path;
	} else {
		cout << "# No Solution" << endl;
	}
	cout << "time-limit: " << timelimit << endl;
	cout << "wall_time: " << timer.get_wall_time() << endl;
	cout << "CPU_time: " << timer.get_processor_time() << endl;
	cout << "generated: " << search->get_generated() << endl;
	cout << "expanded: " << search->get_expanded() << endl;

	if (project)
		delete project;

	delete search;

	return EXIT_SUCCESS;
}
