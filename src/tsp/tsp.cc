// © 2014 the PBNF Authors under the MIT license. See AUTHORS for the list of authors.

/**
 * \file grid_world.cc
 *
 *
 *
 * \author Ethan Burns
 * \date 2008-10-08
 */

#include <iostream>
#include <fstream>
#include <map>
#include <string>
#include <vector>
#include <cmath>

#include <arpa/inet.h>

#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../util/atomic_int.h"
#include "../util/eps.h"

#include "tsp_state.h"
#include "tsp.h"

using namespace std;

/**
 * Create a new GridWorld.
 * \param s The istream to read the world file from.
 */
Tsp::Tsp(istream &s)
#if defined(ENABLE_IMAGES)
		: expanded(0)
#endif	// ENABLE_IMAGES
		{
	// Input data structure
	// <number of city>
	// (x, y)    # coordination of city number 0
	// (x, y)    # coordination of city number 1
	// .... until the last city.

	s >> number_of_cities;
	miles = new double[number_of_cities * number_of_cities]; // a bit more than enough

	// coordination of the city. Divided by x & y.
	double* xcoords = new double[number_of_cities];
	double* ycoords = new double[number_of_cities];

	// Read input from istream and set them to coords.
	// Not sure this works.
	for (unsigned int i = 0; i < number_of_cities; ++i) {
		s >> xcoords[i];
		s >> ycoords[i];
	}

	// Calculate the distance for each cities.
	for (unsigned int from = 0; from < number_of_cities; ++from) {
		for (unsigned int to = 0; to < number_of_cities; ++to) {
			if (from == to) {
				continue;
			} else {
				double tri = (xcoords[from] - xcoords[to])
						* (xcoords[from] - xcoords[to])
						+ (ycoords[from] - ycoords[to])
								* (ycoords[from] - ycoords[to]);
				miles[from * number_of_cities + to] = sqrt(tri);
			}
		}
	}
	/*	printf("domain\n");
	 printf("cities = %u\n", number_of_cities);
	 for (unsigned int i = 0; i < number_of_cities; ++i) {
	 printf("%lf %lf\n", xcoords[i], ycoords[i]);
	 }
	 for (unsigned int i = 0; i < number_of_cities * number_of_cities; ++i) {
	 printf("%lf\n", miles[i]);
	 }*/

}

/**
 * Get the initial state.
 * \return A new state (that must be deleted by the caller) that
 *         represents the initial state.
 */
State *Tsp::initial_state(void) {
	vector<unsigned int>* empty = new vector<unsigned int>();
	printf("initial_state\n");
	State* state = new TspState(this, NULL, 0, 0, empty);
	TspState *s = static_cast<TspState*>(state);
	s->init_zbrhash();
	return state;
}

/**
 * Expand a gridstate.
 * \param state The state to expand.
 * \return A newly allocated vector of newly allocated children
 *         states.  All of this must be deleted by the caller.
 */
vector<State*> *Tsp::expand(State *state) {
	TspState *s = static_cast<TspState*>(state);
//	printf("Tsp::expand\n");
	/*
	 vector<unsigned int> visited = s->get_visited();
	 for (unsigned int i = 0; i < visited.size(); ++i) {
	 printf("%u",visited[i]);
	 }
	 printf("\n");
	 */
	if (s->get_visited().size() == number_of_cities - 1) {
		return expand_to_goal(s);
	} else {
		return expand_usual(s);
	}
}

vector<State*> *Tsp::expand_usual(TspState *state) {
	vector<State*> *children = new vector<State*>();
//	printf("Tsp::expand_usual\n");

	vector<unsigned int> visited = state->get_visited();
//	printf("visited.size = %lu\n", visited.size());
	fp_type g = state->get_g();

	vector<bool> not_visited(number_of_cities);

	for (unsigned int i = 0; i < number_of_cities; ++i) {
		not_visited[i] = true;
	}
	for (unsigned int i = 0; i < visited.size(); ++i) {
		not_visited[visited[i]] = false;
	}

	// i from 1 because 0 is the home town.
	unsigned int from = 0;
	unsigned int zbr_val = state->get_zbr();

	if (visited.size() > 0) {
		from = visited.back();
	}
	for (unsigned int i = 1; i < number_of_cities; ++i) {
		if (not_visited[i] == true) {
			vector<unsigned int> new_visited(visited);
			new_visited.push_back(i);
			double cost = miles[from * number_of_cities + new_visited.back()]
					* 10000; // TODO: not sure how this works.
			unsigned int new_zbr_val = zbr_val ^ state->zbr_table[i];
			children->push_back(
					new TspState(this, state, cost, g + cost, &new_visited, new_zbr_val));
		}
	}
	return children;
}

// Just need to expand toward 0, hometown.
vector<State*> *Tsp::expand_to_goal(TspState *state) {
	fp_type g = state->get_g();
	vector<State*> *children = new vector<State*>();
	vector<unsigned int> visited = state->get_visited();
	double cost = miles[visited.back() * number_of_cities + 0] * 10000;
	vector<unsigned int> new_visited(visited);
	new_visited.push_back(0);
	children->push_back(
			new TspState(this, state, cost, g + cost, &new_visited, state->get_zbr()));
	return children;
}
/**
 * Prints the grid world to the given stream.
 * \param o The output stream to print to.
 * \param path An optional parameter that is a vector of states that
 *             form a path in the world.  If given, this path will be displayed.
 */
void Tsp::print(ostream &o, const vector<State *> *path = NULL) const {

	/*	o << height << " " << width << endl;
	 o << "Board:" << endl;

	 for (int h = 0; h < height; h += 1) {
	 for (int w = 0; w < width; w += 1) {
	 if (is_obstacle(w, h))
	 o << "#";
	 else if (path && on_path(path, w, h))
	 o << "o";
	 else
	 o << " ";
	 }
	 o << endl;;
	 }

	 o << "Unit" << endl;
	 o << "Four-way" << endl;
	 o << start_x << " " << start_y << "\t" << goal_x << " " << goal_y << endl;*/
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/

Tsp::MinimumSpanningTree::MinimumSpanningTree(const SearchDomain *d) :
		Heuristic(d) {
}

/**
 * Compute the 4-way movement heuristic
 */
/*fp_type Tsp::ManhattanDist::compute4(const Tsp *w, GridState *s) const {
 int x = s->get_x();
 int y = s->get_y();
 int gx = w->get_goal_x();
 int gy = w->get_goal_y();

 fp_type dx = abs(gx - x);

 if (w->get_cost_type() == Tsp::UNIT_COST) {
 return (dx + abs(gy - y)) * fp_one;

 } else { // Life-cost
 fp_type cost_up_over_down = compute_up_over_down4(x, y, gx, gy);
 fp_type cost_up_over = compute_up_over4(x, y, gx, gy);

 return cost_up_over_down < cost_up_over ?
 cost_up_over_down * fp_one : cost_up_over * fp_one;
 }
 }*/

/**
 * Compute the Manhattan distance heuristic.
 * \param state The state to compute the heuristic for.
 * \return The Manhattan distance from the given state to the goal.
 */
fp_type Tsp::MinimumSpanningTree::compute(State *state) const {
	TspState *s = static_cast<TspState*>(state);
	vector<unsigned int> visited = s->get_visited(); // copy constructor
	// 1. List the cities unvisited and the city currently in.
	vector<bool> not_visited(number_of_cities, true);
	// -1 is to include city currently visiting
	if (visited.size() > 0) {
		for (unsigned int i = 0; i < visited.size(); ++i) {
			not_visited[visited[i]] = false;
		}
	}

	double min = 10000.0;

	unsigned int from = 0;
	if (visited.size() > 0) {
		from = visited.back();
	}
	for (unsigned int i = 0; i < number_of_cities; ++i) {
		if (not_visited[i] && min > miles[from * number_of_cities + i]) {
			min = miles[from * number_of_cities + i];
		}
	}
//	printf("min = %f\ndist = %u\n", min, dist);

	if (visited.size() == number_of_cities) {
		return 0;
	}
	if (visited.size() == number_of_cities - 1) {
//		printf("goal: cost = %f\n", min/10000.0);
		return min;
	}
//	printf("mst\n");
//	printf("not_visited.size = %lu\n", not_visited.size());
	fp_type mst_cost = mst(&not_visited);
//	printf("mst_cost = %lu\n", mst_cost);
	return mst_cost + min;

}

// Prim's Algorithm
fp_type Tsp::MinimumSpanningTree::mst(vector<bool> *not_visited) const {
	vector<unsigned int> vertices; // vertices are the nodes for the MSP.
	vector<unsigned int> t; // building
	vector<unsigned int> g_minus_t; // The rest of the graph

//	printf("not_visited = ");
	double cost = 0;

	for (unsigned int i = 0; i < not_visited->size(); ++i) {
		if (not_visited->at(i) == true) {
//			printf("X");
			vertices.push_back(i);
			g_minus_t.push_back(i);
		} else {
//			printf("O");
		}
	}
//	printf("\n");

	t.push_back(vertices.back()); // initial node
	g_minus_t.pop_back(); // delete from g_minus_t.
//	printf("t.size = %lu\n", t.size());
//	printf("vertices.size = %lu\n", vertices.size());
	while (t.size() < vertices.size()) {
		double min_edge = 100000.0; // max edge of this domain is 1.412 (or sqrt(2))
		unsigned int new_vertex = -1;

		// Get the minimum edge to a new vertex.
		for (unsigned int i = 0; i < t.size(); ++i) {
			for (unsigned int j = 0; j < g_minus_t.size(); ++j) {
				if (miles[t[i] * number_of_cities + g_minus_t[j]] < min_edge) {
					min_edge = miles[t[i] * number_of_cities + g_minus_t[j]];
//					new_vertex = g_minus_t[j];
					new_vertex = j;
				}
			}
		}
//		printf("new vertex = %u\n", g_minus_t[new_vertex]);
		t.push_back(g_minus_t[new_vertex]);
		g_minus_t.erase(g_minus_t.begin() + new_vertex); // This won't be a issue as the size of vector is <100.
		cost += min_edge;
	}
//	printf("cost = %f\n", cost);
	return cost * 10000;
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/


Tsp::Blind::Blind(const SearchDomain *d) :
		Heuristic(d) {
}

fp_type Tsp::Blind::compute(State *s) const {
	return 0;
}


/****************************************************************************/
/****************************************************************************/
/****************************************************************************/

//#define MOD 16

/**
 * Create a new row modulos projection function.
 * \param d The search domain (a GridWorld)
 * \param mod_val The mod value to use (this will be the number of NBlocks).
 */
Tsp::RowModProject::RowModProject(const SearchDomain *d, unsigned int mod_val) :
		mod_val(mod_val) {
//	const Tsp *g;

//	g = static_cast<const Tsp *>(d);

//	max_row = g->get_height();
}

/**
 * Destructor (because we have virtual functions).
 */
Tsp::RowModProject::~RowModProject() {
}

// TODO: ad hoc. need to come up with smart way to abstract.
unsigned int Tsp::RowModProject::project(State *s) const {
//	printf("RowModProject::project\n");
	TspState *g;
	g = static_cast<TspState *>(s);
	vector<unsigned int> visited = g->get_visited();

//	printf("size = %lu\n", visited.size());

	if (visited.size() == 0) {
		return 0;
	}

//	hash = visited[0];

	/*	while (visited.size() > modulo) {
	 hash = hash * MOD + (visited[modulo] + 1); // ad hoc. won't work for cities > 16.
	 modulo += mod_val;
	 }*/
	if (visited.size() < mod_val) {
		return visited[0];
	} else {
		unsigned int hash = visited[0] + 1;
		for (unsigned int i = 1; i < 1; ++i) {
			unsigned int p = 1;
			for (unsigned int j = 0; j < i; ++j) {
				p *= mod_val;
			}
			hash = hash + (visited[i] + 1) * p;
		}
		return hash;
	}
//	printf("front = %u\n", visited.front());

}

/**
 * Get the number of nblocks.
 * \return The number of NBlocks.
 */
unsigned int Tsp::RowModProject::get_num_nblocks(void) const {
	return mod_val * mod_val + 1;
}

/**
 * Get the successors of an NBlock with the given hash value.
 * \param b The hash value of the NBlock.
 * \return The successor NBlocks of the given NBlock.
 */
// TODO: ad hoc
vector<unsigned int> Tsp::RowModProject::get_successors(unsigned int b) const {
	vector<unsigned int> s;
	/*	for (unsigned int i = 0; i < number_of_cities; ++i) {
	 s.push_back(b * MOD + i + 1);
	 }*/
	/*if (b == 0) {
	 for (unsigned int i = 1; i <= number_of_cities; ++i) {
	 s.push_back(i);
	 }
	 } else {
	 for (unsigned int i = 1; i <= number_of_cities; ++i) {
	 s.push_back(b * MOD + i);
	 }
	 }*/
	if (b == 0) {
		for (unsigned int i = 1; i <= number_of_cities; ++i) {
			s.push_back(i);
		}
//	} else if (b < MOD) { // visited[0]
	} else if (b < mod_val){
		for (unsigned int i = 1; i <= number_of_cities; ++i) {
			s.push_back(b + i * mod_val);
		}
	} else if (b < mod_val * mod_val){
		for (unsigned int i = 1; i <= number_of_cities; ++i) {
			s.push_back(b + i * mod_val * mod_val);
		}
	}
	return s;
}

/**
 * Get the predecessors of an NBlock with the givev hash value.
 * \param b The hash value of the NBlock.
 * \return The predecessor NBlocks of the given NBlock.
 */
// TODO: ad hoc
vector<unsigned int> Tsp::RowModProject::get_predecessors(
		unsigned int b) const {
	vector<unsigned int> s;
	/*
	 if (b < MOD) {
	 s.push_back(0);
	 return s;
	 }
	 */
	if (b == 0) {
		return s;
	} else if (b < mod_val) {
		s.push_back(0);
	} else { // b < MOD * MOD
//		printf("b = %u, b/MOD = %u\n", b, b % MOD);
		s.push_back(b % mod_val);
	}

	return s;
}

/**
 * Get the neighboring NBlock numbers.
 */
/*vector<unsigned int> Tsp::RowModProject::get_neighbors(unsigned int b) const {
 vector<unsigned int> p;
 if (b == 0) {
 for (unsigned int i = 1; i < number_of_cities; ++i) {
 p.push_back(i);
 }
 } else {
 // TODO: ad hoc
 p.push_back(0);
 }
 if (b > 0)
 p.push_back((b - 1) % mod_val);
 else
 p.push_back(mod_val - 1);

 p.push_back((b + 1) % mod_val);

 return p;
 }*/

unsigned int Tsp::number_of_cities;
double* Tsp::miles;
