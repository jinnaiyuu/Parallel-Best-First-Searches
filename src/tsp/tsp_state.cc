// © 2014 the PBNF Authors under the MIT license. See AUTHORS for the list of authors.

/**
 * \file grid_state.cc
 *
 *
 *
 * \author Ethan Burns
 * \date 2008-10-08
 */

#include <iostream>
#include <vector>
#include <algorithm>
#include <stdio.h>

#include "tsp.h"
#include "tsp_state.h"

using namespace std;

/**
 * Create a new grid state.
 * \param d The search domain.
 * \param parent The parent of this state.
 * \param g The g-value of this state.
 * \param x The x-coordinate of this state.
 * \param y The y-coordinate of this state.
 */
TspState::TspState(Tsp *d, State *parent, fp_type c, fp_type g,
		vector<unsigned int>* visited_) :
		State(d, parent, c, g) {

//	printf("TspState\n");
	visited = *visited_;
	if (domain->get_heuristic()) {
//		printf("compute\n");
		this->h = domain->get_heuristic()->compute(this);
	} else {
		this->h = 0;
	}
	this->zbr_val = dist_hash();
//	printf("zbr_val = %u", zbr_val);
}

TspState::TspState(Tsp *d, State *parent, fp_type c, fp_type g,
		vector<unsigned int>* visited_, unsigned int zbr_val_) :
		State(d, parent, c, g) {
//	printf("TspState\n");
	visited = *visited_;
	if (domain->get_heuristic()) {
//		printf("compute\n");
		this->h = domain->get_heuristic()->compute(this);
	} else {
		this->h = 0;
	}
	this->zbr_val = zbr_val_;
	/*	unsigned int n = this->hash();
	 unsigned int cities = d->get_number_of_cities();
	 for (unsigned int i = 0; i < cities - 1; ++i) {
	 n <<= 1;
	 if (n & (1 << cities - 1))
	 printf("1");
	 else
	 printf("0");
	 }
	 printf("\n");*/
}

/**
 * Test if this state is the goal.
 * \return True if this is a goal, false if not.
 */
bool TspState::is_goal(void) {
	const Tsp* d = static_cast<const Tsp *>(domain);
	bool ret = visited.size() == d->get_number_of_cities();
	if (ret) {
		printf("cities = %u\n", d->get_number_of_cities());
		for (unsigned int i = 0; i < visited.size(); ++i) {
			printf("%u ", visited[i]);
		}
		printf("\n");
	}
	return ret;
//	return visited.size() == d->get_number_of_cities(); // may need to make more safe
}

/**
 * Get the hash value of this state.
 * \return A UNIQUE hash value for this state.
 */
// may need to use uint128_t. Even so, would be hard.
uint128_t TspState::hash(void) const {
	const Tsp *d = static_cast<const Tsp *>(domain);
	uint128_t hash = 0;
	unsigned int cities = d->get_number_of_cities();

	for (unsigned int i = 0; i < visited.size(); ++i) {
		hash += (1 << (visited[i] - 1));
	}

	// The city currently on is different from other perspectives.
	if (visited.size() > 0) {
		hash += (visited.back() << (cities + 1));
	}
//
//	for (unsigned int i = 0; i < cities; ++i) {
//		if (hash & (1 << (i - 1)) ){
//			printf("1");
//		} else {
//			printf("0");
//		}
//	}
//	printf(": ");
//	printf("hash = %llu\n", hash);
	return hash;
}

/**
 * Create a copy of this state.
 * \return A copy of this state.
 */
State *TspState::clone(void) const {
	Tsp *d = static_cast<Tsp *>(domain);
	// copy visited and then pass the pointer to the new node.
	vector<unsigned int> perm(visited);
	return new TspState(d, parent, c, g, &perm);
}

/**
 * Print this state.
 * \param o The ostream to print to.
 */
void TspState::print(ostream &o) const {
	/*	o << "x=" << x << ", y=" << y << ", g=" << g << ", h=" << h << ", f="
	 << g + h << endl;*/
}

/**
 * TspState equality.
 */
bool TspState::equals(State *state) const {
	printf("eq: ");
	TspState *s;
	s = static_cast<TspState *>(state);
	vector<unsigned int> svisited = s->get_visited();
	if (svisited.size() != visited.size()) {
		printf("dif size\n");
		return false;
	}
	for (unsigned int i = 0; i < visited.size(); ++i) {
		bool exist = false;
		for (unsigned int j = 0; j < svisited.size(); ++i) {
			if (visited[i] == svisited[j]) {
				exist = true;
				break;
			}
		}
		if (exist == false) {
			printf("dif num\n");
			return false;
		}
	}
	printf("equal\n");
	for (unsigned int i = 0; i < visited.size(); ++i) {
		printf("%u ", visited[i]);
	}
	printf("\n");
	for (unsigned int i = 0; i < svisited.size(); ++i) {
		printf("%u ", svisited[i]);
	}
	printf("\n");

	return true;
}

vector<unsigned int> TspState::get_visited(void) const {
	return visited;
}

void TspState::init_zbrhash(unsigned int abstraction, bool is_structure /* = false*/) {
	const Tsp* d = static_cast<const Tsp *>(domain);
	if (is_structure) {

	} else {
		for (unsigned int i = 0; i < d->get_number_of_cities(); ++i) {
			zbr_table[i] = 0;
		}
		for (unsigned int i = 0; i < d->get_number_of_cities(); i +=
				abstraction) {
			zbr_table[i] = rand();
//		printf("zbr[%u] = %u\n", i, zbr_table[i]);
		}
	}
}

unsigned int TspState::dist_hash(void) {
	return zbr_val;
	/*	vector<unsigned int> visited = get_visited();
	 unsigned int zbr = 0;
	 for (unsigned int i = 0; i < visited.size(); ++i) {
	 zbr = zbr ^ zbr_table[visited[i]];
	 }
	 //	printf("zbr = %u\n", zbr%8);
	 return zbr;*/
}

unsigned int TspState::zbr_table[100];
