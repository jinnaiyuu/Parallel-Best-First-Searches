// © 2014 the PBNF Authors under the MIT license. See AUTHORS for the list of authors.

/**
 * \file arastar.cc
 *
 *
 *
 * \author eaburns
 * \date 2009-04-20
 */
#include <assert.h>

#include <vector>
#include <iostream>

using namespace std;

#include "util/fixed_point.h"
#include "util/serial_solution_stream.h"
#include "state.h"
#include "pq_open_list.h"
#include "closed_list.h"
#include "arastar.h"

ARAStar::ARAStar(vector<double> *ws)
	: solutions(NULL),
	  weights(ws),
	  heuristic(NULL),
	  next_weight(1),
	  cur_weight(ws->at(0))
{
}

ARAStar::~ARAStar(void)
{
	closed.delete_all_states();
}


void ARAStar::output_stats(void)
{
	if (solutions)
		solutions->output(cout);
}

void ARAStar::move_to_next_weight()
{
	double wt = 1.0;
	if (next_weight < weights->size()) {
		wt = weights->at(next_weight);
		next_weight += 1;
	}

	cur_weight = wt;
	heuristic->set_weight(wt);

	open.resort();
	incons.re_open(&open);
	closed.remove_closed_nodes();
}

#define MIN(x, y) ((x) < (y) ? (x) : (y))

double ARAStar::get_eprime()
{
	double eprime;

	fp_type best_incons_fixed = min_f_queue.empty()
		? fp_infinity
		: min_f_queue.front()->get_f();
	double best_incons = (double) best_incons_fixed / (double) fp_one;
	double goal_cost = (double) incumbent_cost / (double) fp_one;

	if (min_f_queue.empty())
		cout << "min_f_queue.empty()" << endl;


	eprime = MIN(cur_weight, goal_cost / best_incons);

#if !defined(NDEBUG)
	cout << "cur_weight=" << cur_weight
	     << " eprime=" << eprime
	     << " goal_cost=" << goal_cost
	     << " best_incons=" << best_incons << endl;
#endif	// !NDEBUG

	return eprime;
}

vector<State *> *ARAStar::search(Timer *t, State *init)
{
	double eprime;
	incumbent_cost = fp_infinity;
	next_weight = 1;
 	heuristic = init->get_domain()->get_heuristic();
	cur_weight = heuristic->get_weight();
	solutions = new SerialSolutionStream(t, 0.0001);

	open.add(init);
	min_f_queue.add(init);

	improve_path();
	eprime = get_eprime();
	while (eprime > 1.0) {
		move_to_next_weight();
		improve_path();
		eprime = get_eprime();
	}

	return solutions->get_best_path();
}

void ARAStar::improve_path(void)
{
	while (!open.empty() && incumbent_cost > open.get_best_val()) {
		State *s = open.take();
		min_f_queue.remove(s->f_pq_index);

		// Prune based on f-value
		if (s->get_f() >= incumbent_cost)
			continue;

		if (s->is_goal()) {
			vector<State*> *path = s->get_path();
			solutions->see_solution(path,
						get_generated(),
						get_expanded());
#if !defined(NDEBUG)
			cout << "Solution of cost " << path->at(0)->get_g() / fp_one
			     << " found at weight " << weights->at(next_weight - 1)
		     << endl;
#endif	// !NDEBUG
			move_to_next_weight();
			incumbent_cost = s->get_g();
		}

		vector<State *> *children = expand(s);
		for (unsigned int i = 0; i < children->size(); i += 1) {
			State *c = children->at(i);
			State *dup = closed.lookup(c);
			if (dup) {
				if (dup->get_g() > c->get_g()) {
					dup->update(c->get_parent(), c->get_c(), c->get_g());
					if (dup->is_open()) {
						open.see_update(dup);
						min_f_queue.see_update(dup->f_pq_index);
					} else if (dup->is_incons()) {
						min_f_queue.see_update(dup->f_pq_index);
					} else {
						min_f_queue.add(dup);
						incons.add(dup);
					}
				}
				delete c;
			} else {
				open.add(c);
				min_f_queue.add(c);
				closed.add(c);
			}

		}
		delete children;
	}
}
