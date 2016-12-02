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

#include <arpa/inet.h>

#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../util/atomic_int.h"
#include "../util/eps.h"

#include "grid_state.h"
#include "grid_world.h"

using namespace std;

/**
 * Create a new GridWorld.
 * \param s The istream to read the world file from.
 */
GridWorld::GridWorld(istream &s)
#if defined(ENABLE_IMAGES)
	: expanded(0)
#endif	// ENABLE_IMAGES
{
	char line[100];
	char c;

	s >> height;
	s >> width;

	s >> line;
	if(strcmp(line, "Board:") != 0) {
		cerr << "Parse error: expected \"Board:\"" << endl;
		exit(EXIT_FAILURE);
	}
	c = s.get();		// new-line
	if (c != '\n') {
		cerr << endl << "Parse error: [" << c << "]" << endl;
		exit(EXIT_FAILURE);
	}

	obstacles.resize(width * height);
	for (int h = 0; h < height; h += 1) {
		for (int w = 0; w < width; w += 1) {
			c = s.get();
			if (c == '#')
				obstacles[width * h + w] = true;
			else
				obstacles[width * h + w] = false;
		}
		c = s.get();	// new-line
		if (c != '\n') {
			cerr << endl << "Parse error: [" << c << "]" << endl;
			exit(EXIT_FAILURE);
		}
	}

	// Cost (Unit/Life)
	s >> line;
	if (strcmp(line, "Unit") == 0) {
		cost_type = UNIT_COST;
	} else {
		assert(strcmp(line, "Life") == 0);
		cost_type = LIFE_COST;
	}

	// Movement (Four-way/Eight-way)
	s >> line;
	if (strcmp(line, "Four-way") == 0) {
		move_type = FOUR_WAY_MOVES;
	} else {
		assert(strcmp(line, "Eight-way") == 0);
		move_type = EIGHT_WAY_MOVES;
	}


	// NOTE: *our* y-coordinates are the opposite of the ones
	// stored in the map file.  The map file uses the bottom of
	// the grid as y=0, we use the top.
	s >> start_x;
	s >> start_y;
	start_y = height - 1 - start_y;
	s >> goal_x;
	s >> goal_y;
	goal_y = height - 1 - goal_y;

#if defined(ENABLE_IMAGES)
	states.resize(width * height, AtomicInt(0));
#endif	// ENABLE_IMAGES
}

/**
 * Get the initial state.
 * \return A new state (that must be deleted by the caller) that
 *         represents the initial state.
 */
State *GridWorld::initial_state(void)
{
	return new GridState(this, NULL, 0, 0, start_x, start_y);
}

/**
 * Four-way movement expand.
 */
vector<State*> *GridWorld::expand4(GridState *s)
{
	vector<State*> *children;
	int x = s->get_x();
	int y = s->get_y();
	fp_type g = s->get_g();
	fp_type cost = this->cost_type == UNIT_COST ? fp_one : y * fp_one;

	children = new vector<State*>();

	if (x > 0 && !is_obstacle(x - 1, y)) {
		children->push_back(new GridState(this, s,
						  cost,
						  g + cost, x - 1, y));
	}
	if (x < width - 1 && !is_obstacle(x + 1, y)) {
		children->push_back(new GridState(this, s,
						  cost,
						  g + cost, x + 1, y));
	}
	if (y > 0 && !is_obstacle(x, y- 1)) {
		children->push_back(new GridState(this, s,
						  cost,
						  g + cost, x, y - 1));
	}
	if (y < height - 1 && !is_obstacle(x, y+ 1)) {
		children->push_back(new GridState(this, s,
						  cost,
						  g + cost, x, y + 1));
	}

	return children;
}

/**
 * Eight-way movement expand.
 */
vector<State*> *GridWorld::expand8(GridState *s)
{
	vector<State*> *children;
	int x = s->get_x();
	int y = s->get_y();
	fp_type g = s->get_g();
	fp_type cost = this->cost_type == UNIT_COST ? fp_sqrt2 : y * fp_sqrt2;

	children = expand4(s);

	if (x > 0 && y > 0 && !is_obstacle(x - 1, y - 1)) {
		children->push_back(new GridState(this, s, cost, g + cost,
						  x - 1, y - 1));
	}
	if (x < width - 1 && y > 0 && !is_obstacle(x + 1, y - 1)) {
		children->push_back(new GridState(this, s, cost, g + cost,
						  x + 1, y - 1));
	}
	if (x < width - 1 && y < height - 1 && !is_obstacle(x + 1, y + 1)) {
		children->push_back(new GridState(this, s, cost, g + cost,
						  x + 1, y + 1));
	}
	if (x > 0 && y < height - 1 && !is_obstacle(x - 1, y+ 1)) {
		children->push_back(new GridState(this, s, cost, g + cost,
						  x - 1, y + 1));
	}

	return children;
}

/**
 * Expand a gridstate.
 * \param state The state to expand.
 * \return A newly allocated vector of newly allocated children
 *         states.  All of this must be deleted by the caller.
 */
vector<State*> *GridWorld::expand(State *state)
{
	GridState *s = static_cast<GridState*>(state);

#if defined(ENABLE_IMAGES)
	expanded.inc();
	expanded_state(s);
#endif	// ENABLE_IMAGSE

	if (move_type == FOUR_WAY_MOVES)
		return expand4(s);
	else
		return expand8(s);

}

/**
 * Get the x-coordinate of the goal.
 * \return the x-coordinate of the goal.
 */
int GridWorld::get_goal_x(void) const
{
	return goal_x;
}

/**
 * Get the y-coordinate of the goal.
 * \return the y-coordinate of the goal.
 */
int GridWorld::get_goal_y(void) const
{
	return goal_y;
}

/**
 * Get the world width.
 * \return The world width.
 */
int GridWorld::get_width(void) const
{
	return width;
}

/**
 * Get the world height.
 * \return The world height.
 */
int GridWorld::get_height(void) const
{
	return height;
}

/**
 * Get the cost type for this world.
 * \return UNIT_COST or LIFE_COST
 */
enum GridWorld::cost_type GridWorld::get_cost_type(void) const
{
	return cost_type;
}

/**
 * Get the movement type.
 */
enum GridWorld::move_type GridWorld::get_move_type(void) const
{
	return move_type;
}

/**
 * Test if there is an obstacle at the given location.
 */
bool GridWorld::is_obstacle(int x, int y) const
{
	return obstacles[width * y + x];
}

/**
 * Prints the grid world to the given stream.
 * \param o The output stream to print to.
 * \param path An optional parameter that is a vector of states that
 *             form a path in the world.  If given, this path will be displayed.
 */
void GridWorld::print(ostream &o, const vector<State *> *path = NULL) const
{
	o << height << " " << width << endl;
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
	o << start_x << " " << start_y << "\t" << goal_x << " " << goal_y << endl;
}

/**
 * Test whether or not a location is on the given path.
 * \param path The path.
 * \param x The x-coordinate to test.
 * \param y The y-coordinate to test.
 */
bool GridWorld::on_path(const vector<State *> *path, int x, int y) const
{
	if (!path)
		return false;

	for (unsigned int i = 0; i < path->size(); i += 1) {
		GridState *s = static_cast<GridState *>(path->at(i));
		if (s->get_x() == x && s->get_y() == y)
			return true;
	}

	return false;
}

#if defined(ENABLE_IMAGES)

/**
 * Mark a state as expanded, if this was the first time that the state
 * was expanded, it is marked in the states field so that we can make
 * a display of when each state was expanded.
 * \param s The state that was expanded.
 */
void GridWorld::expanded_state(GridState *s)
{
	int index = s->get_y() * width + s->get_x();

	// this should be made atomic
	states[index].set(expanded.read());
}

/**
 * Export an image of the order of state expansions to an encapsulated
 * post script file.
 * \param file The name of the file to export to.
 */
void GridWorld::export_eps(string file) const
{
	const int min_size = 500;
	const int max_size = 1000;
	int granularity =
	        expanded.read() == 0     ? 0
		: (expanded.read() < 500 ? 500 / expanded.read()
	                                 : expanded.read() / 500);
	string data;
	EPS image(width, height);

	if (width < min_size && height < min_size) {
		fp_type min_side = width < height ? width : height;
		image.set_scale(min_size / min_side);
	}

	if (width > max_size || height > max_size) {
		fp_type max_side = width > max_size ? width : height;
		image.set_scale(max_size / max_side);
	}

	// Image red data
	for (int y = height - 1; y >= 0; y -= 1) {
		for (int x = 0; x < width; x += 1) {
			int i = y * width + x;
			char red, green, blue;
			if (is_obstacle(x, y)) {
				// black
				red = green = blue = 0;
			} else if (states[i].read() == 0) {
				// white
				red = green = blue = (char) 255;
			} else {
				// A certain shade of orange
				int number = states[i].read();
				int x = expanded.read() < 500
				        ? number * granularity
					: number / granularity;
				if (x < 125) {
					red = (char) 255;
					green = (char) 255;
					blue = 125 - x;
				} else if (x < 380) {
					red = (char) 255;
					green = 255 - (x - 125);
					blue = 0;
				} else {
					red = 255 - (x - 380);
					green = blue = 0;
				}

			}
			data += red;
			data += green;
			data += blue;
		}
	}

	image.set_data(data);
	image.output(file);
}

#endif	// ENABLE_IMAGES

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/

GridWorld::ManhattanDist::ManhattanDist(const SearchDomain *d)
	: Heuristic(d) {}

/**
 * Compute the costs between two y values.
 */
fp_type GridWorld::ManhattanDist::cost_from(int a, int b) const
{
	fp_type last = b;

	if (a > b)
		last += 1;
	else if (a < b)
		last -= 1;

	return (a + last) * (abs(a - b)) / 2;
}

/**
 * Compute the up-and-over path cost for four-way movement
 *
 * Move up to the y-value of the goal then across to the goal, *or*
 * move across to the goal then down to the y-value of the goal.
 */
fp_type GridWorld::ManhattanDist::compute_up_over4(int x, int y,
						int gx, int gy) const
{
	fp_type min_y = y < gy ? y : gy;
	fp_type dx = abs(gx - x);

	return (dx * min_y) + cost_from(y, gy);
}

/**
 * Compute the up-over-and-down path cost for four-way movement
 *
 * This moves up to y=0, moves across for free to the correct x-value
 * then down to the goal.
 *
 * cost from state to y=0: (y^2 + y) / 2
 * cost from y=0 to goal:  (gy^2 - gy) / 2
 */
fp_type GridWorld::ManhattanDist::compute_up_over_down4(int x, int y,
						     int gx, int gy) const
{
	return ((y * y) + y + (gy * gy) - gy) / 2;
}

/**
 * Compute the 4-way movement heuristic
 */
fp_type GridWorld::ManhattanDist::compute4(const GridWorld *w,
					   GridState *s) const
{
	int x = s->get_x();
	int y = s->get_y();
	int gx = w->get_goal_x();
	int gy = w->get_goal_y();

	fp_type dx = abs(gx - x);

	if (w->get_cost_type() == GridWorld::UNIT_COST) {
		return (dx + abs(gy - y)) * fp_one;

	} else {		// Life-cost
		fp_type cost_up_over_down = compute_up_over_down4(x, y, gx, gy);
		fp_type cost_up_over = compute_up_over4(x, y, gx, gy);

		return cost_up_over_down < cost_up_over
			? cost_up_over_down * fp_one
			: cost_up_over * fp_one;
	}
}

/**
 * Compute the 8-way movement heuristic
 */
fp_type GridWorld::ManhattanDist::compute8(const GridWorld *w,
					 GridState *s) const
{
	int x = s->get_x();
	int y = s->get_y();
	int gx = w->get_goal_x();
	int gy = w->get_goal_y();
	int dx = abs(gx - x);
	int dy = abs(gy - y);

	if (w->get_cost_type() == GridWorld::UNIT_COST) {
		fp_type total = dx > dy ? dx : dy;
		fp_type diag = dx < dy ? dx : dy;
		return (diag * fp_sqrt2) + ((total - diag) * fp_one);
	} else {
		if (dx > dy) {
			// I *think* we do an arc with 2 diagonal lines.
			int max_up = y < gy ? y : gy;
			int extra = dx - dy;
			int up = max_up < (extra / 2) ? max_up : (extra / 2);
			int high_y = (y < gy ? y : gy) - up;
			int across = extra - (2 * up);
			return (cost_from(y, high_y)
				+ across * high_y
				+ cost_from(high_y, gy)) * fp_one;
		} else {
			return (cost_from(y, gy) * fp_one);
		}
	}
}

/**
 * Compute the Manhattan distance heuristic.
 * \param state The state to compute the heuristic for.
 * \return The Manhattan distance from the given state to the goal.
 */
fp_type GridWorld::ManhattanDist::compute(State *state) const
{
	GridState *s = static_cast<GridState *>(state);
	const GridWorld *w = static_cast<const GridWorld *>(domain);

	if (w->get_move_type() == FOUR_WAY_MOVES)
		return compute4(w, s);
	else
		return compute8(w, s);
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/

/**
 * Create a new row modulos projection function.
 * \param d The search domain (a GridWorld)
 * \param mod_val The mod value to use (this will be the number of NBlocks).
 */
GridWorld::RowModProject::RowModProject(const SearchDomain *d,
					unsigned int mod_val)
	: mod_val(mod_val)
{
	const GridWorld *g;

	g = static_cast<const GridWorld *>(d);

	max_row = g->get_height();
}


/**
 * Destructor (because we have virtual functions).
 */
GridWorld::RowModProject::~RowModProject() {}

unsigned int GridWorld::RowModProject::project(State *s) const
{
	GridState *g;

	g = static_cast<GridState *>(s);

	return g->get_y() % mod_val;
}

/**
 * Get the number of nblocks.
 * \return The number of NBlocks.
 */
unsigned int GridWorld::RowModProject::get_num_nblocks(void) const
{
	return mod_val;
}

/**
 * Get the successors of an NBlock with the given hash value.
 * \param b The hash value of the NBlock.
 * \return The successor NBlocks of the given NBlock.
 */
vector<unsigned int> GridWorld::RowModProject::get_successors(unsigned int b) const
{
	return get_neighbors(b);
}

/**
 * Get the predecessors of an NBlock with the givev hash value.
 * \param b The hash value of the NBlock.
 * \return The predecessor NBlocks of the given NBlock.
 */
vector<unsigned int> GridWorld::RowModProject::get_predecessors(unsigned int b) const
{
	return get_neighbors(b);
}

/**
 * Get the neighboring NBlock numbers.
 */
vector<unsigned int> GridWorld::RowModProject::get_neighbors(unsigned int b) const
{
	vector<unsigned int> p;

	if (b > 0)
		p.push_back((b - 1) % mod_val);
	else
		p.push_back(mod_val - 1);

	p.push_back((b + 1) % mod_val);

	return p;
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/

/**
 * Create a new row modulos projection function.
 * \param d The search domain (a GridWorld)
 * \param mod_val The mod value to use (this will be the number of NBlocks).
 */
GridWorld::CoarseProject::CoarseProject(const SearchDomain *d,
					unsigned int c,
					unsigned int r)
{
	const GridWorld *g;

	g = static_cast<const GridWorld *>(d);

	if (g->width % c)
		c += 1;

	if (g->height % r)
		r += 1;

	this->cols = c;
	this->rows = r;
	this->cols_div = g->width / c;
	this->rows_div = g->height / r;
	this->move_type = g->move_type;

/*
	cerr << "Creating coarse projection, c="
	     << this->cols << ", rows=" << this->rows
	     << ", cols_div=" << cols_div << ", rows_div=" << rows_div
	     << endl;
*/
}


/**
 * Destructor (because we have virtual functions).
 */
GridWorld::CoarseProject::~CoarseProject() {}

unsigned int GridWorld::CoarseProject::get_id(unsigned int x, unsigned int y) const
{
	return (y * cols) + x;
}

unsigned int GridWorld::CoarseProject::project(State *s) const
{
	GridState *g;

	g = static_cast<GridState *>(s);

	unsigned int id =  get_id(g->get_x() / cols_div, g->get_y() / rows_div);
#if !defined(NDEBUG)
	if (id >= (rows * cols)) {
		cerr << "rows=" << rows << ", cols=" << cols << endl
		     << "x=" << g->get_x() << ", y=" << g->get_y() << endl
		     << "id=" << id << endl;
	}
#endif	// !NDEBUG
	assert(id < (rows * cols));

/*
	cerr << "Projecting, x=" << g->get_x() << ", y=" << g->get_y()
	     << " to x=" << g->get_x() / cols_div
	     << ", y=" << g->get_y() / rows_div
	     << ", id=" << id << endl;
*/
	return id;
}

/**
 * Get the number of nblocks.
 * \return The number of NBlocks.
 */
unsigned int GridWorld::CoarseProject::get_num_nblocks(void) const
{
	return rows * cols;
}

/**
 * Get the successors of an NBlock with the given hash value.
 * \param b The hash value of the NBlock.
 * \return The successor NBlocks of the given NBlock.
 */
vector<unsigned int> GridWorld::CoarseProject::get_successors(unsigned int b) const
{
	return get_neighbors(b);
}

/**
 * Get the predecessors of an NBlock with the givev hash value.
 * \param b The hash value of the NBlock.
 * \return The predecessor NBlocks of the given NBlock.
 */
vector<unsigned int> GridWorld::CoarseProject::get_predecessors(unsigned int b) const
{
	return get_neighbors(b);
}

/**
 * Get the neighboring NBlock numbers.
 */
vector<unsigned int> GridWorld::CoarseProject::get_neighbors(unsigned int b) const
{
	vector<unsigned int> p;
	unsigned int x = b % cols;
	unsigned int y = b / cols;

	if (x > 0)
		p.push_back(get_id(x - 1, y));
	if (x < cols - 1)
		p.push_back(get_id(x + 1, y));
	if (y > 0)
		p.push_back(get_id(x, y - 1));
	if (y < rows - 1)
		p.push_back(get_id(x, y + 1));

	if (move_type == GridWorld::EIGHT_WAY_MOVES) {
		if (x > 0 && y > 0)
			p.push_back(get_id(x - 1, y - 1));
		if (x < cols - 1 && y > 0)
			p.push_back(get_id(x + 1, y - 1));
		if (x < cols - 1 && y < rows - 1)
			p.push_back(get_id(x + 1, y + 1));
		if (x > 0 && y < rows - 1)
			p.push_back(get_id(x - 1, y + 1));
	}

	return p;
}
