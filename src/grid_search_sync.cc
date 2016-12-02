// © 2014 the PBNF Authors under the MIT license. See AUTHORS for the list of authors.

/**
 * \file grid_search.cc
 *
 *
 *
 * \author Ethan Burns
 * \date 2008-10-08
 */

#include <assert.h>
#include <math.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include <iostream>
#include <vector>

#include "get_search.h"
#include "state.h"
#include "search.h"
#include "h_zero.h"
#include "grid/grid_world.h"
#include "util/cpu_timer.h"
#include "util/timeout.h"
#include "util/fixed_point.h"
#include "sync_wastar.h"

using namespace std;

int main(int argc, char *argv[])
{
	unsigned int timelimit = 300;	// seconds
	vector<State *> *path;
	SyncWAStar *search;
	GridWorld *g_orig = new GridWorld(cin);
	Timer timer;

	unsigned int root = (unsigned int) sqrt((double)nblocks);
	GridWorld::CoarseProject project(g_orig, root, root);



	vector<double> *weights;
	vector<State *> *inits = new vector<State *>();
	unsigned int threads=1;
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
		GridWorld *g = (GridWorld*)malloc(sizeof(*g_orig));
		memcpy(g, g_orig, sizeof(*g_orig));
		g->set_projection(&project);

		GridWorld::ManhattanDist *manhattan = new GridWorld::ManhattanDist(g);
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

	delete search;

	return EXIT_SUCCESS;
}
