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

#include "grid_world.h"
#include "grid_state.h"

using namespace std;

/**
 * Create a new grid state.
 * \param d The search domain.
 * \param parent The parent of this state.
 * \param g The g-value of this state.
 * \param x The x-coordinate of this state.
 * \param y The y-coordinate of this state.
 */
GridState::GridState(GridWorld *d, State *parent, fp_type c, fp_type g, int x, int y)
	: State(d, parent, c, g), x(x), y(y)
{
  //  printf("new state\n");
	if (domain->get_heuristic())
		this->h = domain->get_heuristic()->compute(this);
	else
		this->h = 0;
}

/**
 * Test if this state is the goal.
 * \return True if this is a goal, false if not.
 */
bool GridState::is_goal(void)
{
	const GridWorld *d;

	d = static_cast<const GridWorld *>(domain);

	return d->get_goal_x() == x && d->get_goal_y() == y;
}

/**
 * Get the hash value of this state.
 * \return A unique hash value for this state.
 */
uint64_t GridState::hash(void) const
{
	const GridWorld *d;

	d = static_cast<const GridWorld *>(domain);

	return x * d->get_height() + y;
}

/**
 * Create a copy of this state.
 * \return A copy of this state.
 */
State *GridState::clone(void) const
{
	GridWorld *d = static_cast<GridWorld *>(domain);

	return new GridState(d, parent, c, g, x, y);
}

/**
 * Print this state.
 * \param o The ostream to print to.
 */
void GridState::print(ostream &o) const
{
	o << "x=" << x << ", y=" << y << ", g=" << g << ", h=" << h
	  << ", f=" << g + h << endl;
}

/**
 * GridState equality.
 */
bool GridState::equals(State *state) const
{
	GridState *s;

	s = static_cast<GridState *>(state);

	return s->x == x && s->y == y;
}

/**
 * Get the x-coordinate.
 * \return The x-coordinate.
 */
int GridState::get_x(void) const
{
	return x;
}

/**
 * Get the x-coordinate.
 * \return The x-coordinate.
 */
int GridState::get_y(void) const
{
	return y;
}

unsigned int GridState::dist_hash(void){
  return hash();
}
