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
	printf("get_search\n");
	unsigned int timelimit = 10000; // seconds
	vector<State *> *path;
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
	} else if (nblocks) { // TODO: ad hoc
		project = new Tsp::RowModProject(&g, nblocks);
	} else {
		cerr << "Invalid abstraction size: " << nblocks << endl;
		cerr << "15-puzzle: 240=1tile, 3360=2tile" << endl;
		exit(EXIT_FAILURE);
	}

	g.set_projection(project);

//	printf("MSP\n");
	Tsp::MinimumSpanningTree mst(&g, 0);
	Tsp::MinimumSpanningTree onetree(&g, 1);
	Tsp::RoundTripDistance rtd(&g);
	Tsp::Blind blind(&g);
// If parameter given as blind, then run blind heuristic.
	if (argc >= 3 && (strcmp(argv[2], "blind") == 0)) {
//		printf("blind\n");
		g.set_heuristic(&blind);
	} else if (argc >= 3 &&(strcmp(argv[2], "round") == 0)){
		g.set_heuristic(&rtd);
	} else if (argc >= 3 &&(strcmp(argv[2], "onetree") == 0)){
		g.set_heuristic(&onetree);
	} else {
		g.set_heuristic(&mst);
	}

	unsigned int abstraction = 1;
	for (int i = 0; i < argc; ++i){
		if(sscanf(argv[i], "abstraction-%u", &abstraction) == 1) {

		}
	}
	printf("heuristic done\n");

#if defined(NDEBUG)
	timeout(timelimit);
#endif	// NDEBUG
	timer.start();
	State* inits = g.initial_state();
	g.init_zbrhash(abstraction);

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
