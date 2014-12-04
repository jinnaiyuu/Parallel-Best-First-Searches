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
#include "tiles/tiles.h"
#include "util/timer.h"
#include "util/timeout.h"
#include "div_merge_project.h"


using namespace std;

int main(int argc, char *argv[])
{
	unsigned int timelimit = 10000000;	// seconds
	vector<State *> *path;
	Search *search = get_search(argc, argv);

	if (strcmp(argv[1], "arastar") == 0) {	// hack to peal off extra ARA* argument
		argv++;
		argc--;
	}
	string cost = argc == 2 ? "unit" : argv[2];
	Tiles g(cin, cost);
	Timer timer;

	Projection *project;
	if (nblocks == 0) {
		project = NULL;
	} else if (nblocks == 1 || nblocks == 240) {
		project = new Tiles::OneTileProject(&g);
	} else if (nblocks == 2 || nblocks == 3360) {
		project = new Tiles::TwoTileProject(&g);
	} else if (nblocks == 3 || nblocks == 43680) {
		project = new Tiles::ThreeTileProject(&g);
	} else if (nblocks == 123) {
		project = new Tiles::TwoTileNoBlankProject(&g, 1, 2, 3);
	} else {
		cerr << "Invalid abstraction size: " << nblocks << endl;
		cerr << "15-puzzle: 240=1tile, 3360=2tile" << endl;
		exit(EXIT_FAILURE);
	}

//	DivMergeProject project(4, &__project);
	g.set_projection(project);

//	HZero hzero(&g);
//	g.set_heuristic(&hzero);
	Tiles::ManhattanDist manhattan(&g);
	manhattan.set_weight(weight);
	g.set_heuristic(&manhattan);

#if defined(NDEBUG)
	timeout(timelimit);
#endif	// NDEBUG

	timer.start();
	path = search->search(&timer, g.initial_state());
	timer.stop();

#if defined(NDEBUG)
	timeout_stop();
#endif	// NDEBUG

	search->output_stats();

	/* Print the graph to the terminal */
//	g.print(cout, path);

	if (path) {
		printf("cost: %f\n", (double) path->at(0)->get_g() / fp_one);
		cout << "length: " << path->size() << endl;

		// Make sure that the heuristic was actually admissible!
		/*
		for (unsigned int i = path->size() - 1; i >= 0; i -= 1) {
#if !defined(NDEBUG)
			State *s = path->at(i);
			fp_type togo = path->at(0)->get_g() - s->get_g();
			assert(s->get_h() <= togo);
#endif
			if (i > 0)
				assert(s->get_h() > 0);
			if (i == 0)
				break;
		}
		*/
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

//#ifdef ANALYZE_ORDER
       	g.dumpOrder(32); // Ad hoc
//#endif

	if (project)
		delete project;

	delete search;

	return EXIT_SUCCESS;
}
