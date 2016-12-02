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
#include "sync_wastar.h"

using namespace std;

int main(int argc, char *argv[])
{
	// 90 seconds is too small to solve the 96 instance optimally in a consistent manner.
	unsigned int timelimit = 120;	// seconds
	vector<State *> *path;
	SyncWAStar *search;
	Tiles *g_orig = new Tiles(cin, "");
	Timer timer;

	Projection *project;
	if (nblocks == 0) {
		project = NULL;
	} else if (nblocks == 1 || nblocks == 240) {
		project = new Tiles::OneTileProject(g_orig);
	} else if (nblocks == 2 || nblocks == 3360) {
		project = new Tiles::TwoTileProject(g_orig);
	} else {
		cerr << "Invalid abstraction size: " << nblocks << endl;
		cerr << "15-puzzle: 240=1tile, 3360=2tile" << endl;
		exit(EXIT_FAILURE);
	}

	vector<double> *weights;
	vector<State *> *inits = new vector<State *>();
	unsigned int threads;
	bool dd;
	if (argc > 2
	    && sscanf(argv[1], "syncwastar-%u", &threads) == 1) {
		weights = parse_weights(argv[2]);
		dd = false;
	}else if (argc > 2
	    && sscanf(argv[1], "syncwastardd-%u", &threads) == 1) {
		weights = parse_weights(argv[2]);
		dd = true;
	}
	else{
		cerr << "bad input" << endl;
		exit(EXIT_FAILURE);
	}

	for(vector<double>::iterator i = weights->begin();
	    i != weights->end(); i++){
		Tiles *g = (Tiles*)malloc(sizeof(*g_orig));
		memcpy(g, g_orig, sizeof(*g_orig));
		g->set_projection(project);

		Tiles::ManhattanDist *manhattan = new Tiles::ManhattanDist(g);
		manhattan->set_weight(*i);
		g->set_heuristic(manhattan);
		inits->push_back(g->initial_state());
	}

	search = new SyncWAStar(threads, inits, dd);

#if defined(NDEBUG)
	timeout(timelimit);
#endif	// NDEBUG

	timer.start();
	path = search->search(&timer, NULL);
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
