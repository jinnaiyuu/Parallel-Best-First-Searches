// © 2014 the PBNF Authors under the MIT license. See AUTHORS for the list of authors.

/**
 * \file optimistic.cc
 *
 *
 *
 * \author eaburns
 * \date 2009-06-02
 */
#include <assert.h>

#include <limits>

#include "state.h"
#include "pq_open_list.h"
#include "closed_list.h"
#include "optimistic.h"
#include "util/fixed_point.h"

Optimistic::Optimistic(double b) : bound(b) {}

Optimistic::~Optimistic(void)
{
	closed.delete_all_states();
}

/**
 * Get the next state from either open_fp or open_f following the
 * rules of optimistic search:
 *
 *	- If there is no incumbent, use open_fp
 *	- If there is an incumbent:
 *		- if [open_fp.get_best_val() < incumbent_cost]
 *			take from open_fp
 *		- else take from open_f
 */
State *Optimistic::get_next_state(void)
{
	if (!incumbent)
		return open_fp.take();

	if (open_fp.get_best_val() < incumbent_cost) {
		State *s = open_fp.take();
		open_f.remove(s);
		return s;
	}

	State *s = open_f.take();
	open_fp.remove(s);
	return s;
}

/**
 * Add the child to the open list(s).  If there is an incumbent, then
 * add the child to both open_fp and open_f, if not then just add it
 * to open_fp.
 */
void Optimistic::open_add_node(State *s)
{
	if (incumbent)
		open_f.add(s);
	open_fp.add(s);
}

/**
 * Update the open list.  If there is no incumbent then just update
 * the open_fp else update both open_fp and open_f.
 */
void Optimistic::open_see_update(State *s)
{
	if (incumbent)
		open_f.see_update(s);
	open_fp.see_update(s);
}

/**
 * Set the incumbent solution.  If this is the first incumbent then we
 * must populate open_f.
 */
void Optimistic::set_incumbent(State *s)
{
	assert(s->is_goal());
	assert(s->get_h() == 0);

	if (!incumbent) {
		// Initialize the open_f queue.
		list<State*> *states = open_fp.states();
		list<State*>::iterator i;

		for (i = states->begin(); i != states->end(); i++)
			open_f.add(*i);

		delete states;
	}

	incumbent = s->get_path();
	incumbent_cost = DOUBLE(s->get_g());
}

/**
 * Test if the search is complete (if our bound has been proved).
 */
bool Optimistic::search_complete(void)
{
	double f_min;

	if (!incumbent)
		return false;

	f_min = DOUBLE(open_f.get_best_val());
	return bound * f_min >= incumbent_cost;
}

vector<State *> *Optimistic::search(Timer *t, State *init)
{
	incumbent = NULL;
	incumbent_cost = numeric_limits<double>::infinity();

	open_fp.add(init);

	while (!open_fp.empty() && !search_complete()) {
		State *s = get_next_state();
		if (s->is_goal()) {
			set_incumbent(s);
			continue;
		}

		vector<State *> *children = expand(s);
		for (unsigned int i = 0; i < children->size(); i += 1) {
			State *c = children->at(i);
			State *dup = closed.lookup(c);
			if (dup) {
				if (dup->get_g() > c->get_g()) {
					dup->update(c->get_parent(), c->get_c(), c->get_g());
					if (dup->is_open())
						open_see_update(dup);
					else
						open_add_node(dup);
				}
				delete c;
			} else {
				open_add_node(c);
				closed.add(c);
			}

		}
		delete children;
	}

	return incumbent;
}
