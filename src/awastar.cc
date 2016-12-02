// © 2014 the PBNF Authors under the MIT license. See AUTHORS for the list of authors.

/**
 * \file awastar.cc
 *
 *
 *
 * \author eaburns
 * \date 2009-04-20
 */
#include <assert.h>

#include <iostream>

using namespace std;

#include "util/fixed_point.h"
#include "util/serial_solution_stream.h"
#include "state.h"
#include "pq_open_list.h"
#include "closed_list.h"
#include "awastar.h"

AwAStar::AwAStar(void)
	: solutions(NULL)
{
}

AwAStar::~AwAStar(void)
{
	closed.delete_all_states();
}


/**
 * Perform an A* search.
 */
vector<State *> *AwAStar::search(Timer *t, State *init)
{
	fp_type incumbent_cost = fp_infinity;
	PQOpenList<State::PQOpsFPrime> open;

	solutions = new SerialSolutionStream(t, 0.01);
	open.add(init);

	while (!open.empty()) {
		State *s = open.take();

		if (s->get_f() >= incumbent_cost)
			continue;

		if (s->is_goal()) {
			vector<State*> *path = s->get_path();
			solutions->see_solution(path,
						get_generated(),
						get_expanded());
			incumbent_cost = s->get_g();
		}

		vector<State *> *children = expand(s);
		for (unsigned int i = 0; i < children->size(); i += 1) {
			State *c = children->at(i);
			if (c->get_f() >= incumbent_cost) {
				delete c;
				continue;
			}
			State *dup = closed.lookup(c);
			if (dup) {
				if (dup->get_g() > c->get_g()) {
					dup->update(c->get_parent(), c->get_c(), c->get_g());
					if (dup->is_open())
						open.see_update(dup);
					else
						open.add(dup);
				}
				delete c;
			} else {
				open.add(c);
				closed.add(c);
			}

		}
		delete children;
	}

	return solutions->get_best_path();
}

void AwAStar::output_stats(void)
{
	if (solutions)
		solutions->output(cout);
}

