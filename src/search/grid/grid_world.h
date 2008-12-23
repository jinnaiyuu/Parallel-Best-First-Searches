/* -*- mode:linux -*- */
/**
 * \file grid_world.h
 *
 *
 *
 * \author Ethan Burns
 * \date 2008-10-08
 */

#if !defined(_GRID_WORLD_H_)
#define _GRID_WORLD_H_

#include <iostream>
#include <map>
#include <string>
#include <vector>

#include "../util/atomic_int.h"

#include "../state.h"
#include "../search_domain.h"
#include "../projection.h"

class GridState;

using namespace std;



class GridWorld : public SearchDomain {
public:
	enum cost_type { UNIT_COST, LIFE_COST };
	enum move_type { FOUR_WAY_MOVES, EIGHT_WAY_MOVES };

	GridWorld(istream &s);

	const State *initial_state(void);
	vector<const State*> *expand(const State *s);

	int get_goal_x(void) const;
	int get_goal_y(void) const;
	int get_width(void) const;
	int get_height(void) const;
	enum cost_type get_cost_type(void) const;
	enum move_type get_move_type(void) const;
	void print(ostream &o, const vector<const State *> *path) const;
#if defined(ENABLE_IMAGES)
	void export_eps(string file) const;
#endif	/* ENABLE_IMAGES */

	/*
	 * The Manhattan Distance heuristic.
	 */
	class ManhattanDist : public Heuristic {
	public:
		ManhattanDist(const SearchDomain *d);
		float compute(const State *s) const;
	private:
		float compute_up_over4(int x, int y,
				       int gx, int gy) const;
		float compute_up_over_down4(int x, int y,
					    int gx, int gy) const;
		float comupte4(const GridWorld *w, const GridState *s) const;
		float comupte8(const GridWorld *w, const GridState *s) const;
	};

	/*
	 * Projection function that uses the row number mod a value.
	 */
	class RowModProject : public Projection {
	public:
		RowModProject(const SearchDomain *d, unsigned int mod_val);
		~RowModProject();
		unsigned int project(const State *s) const ;
		unsigned int get_num_nblocks(void) const ;
		vector<unsigned int> get_successors(unsigned int b) const;
		vector<unsigned int> get_predecessors(unsigned int b) const;
	private:
		vector<unsigned int> get_neighbors(unsigned int b) const;
		unsigned int mod_val;
		unsigned int max_row;
	};

	/*
	 * Overlay the grid with a more coarse grid of abstract states.
	 */
	class CoarseProject : public Projection {
	public:
		CoarseProject(const SearchDomain *d, unsigned int cols, unsigned int rows);
		~CoarseProject();
		unsigned int project(const State *s) const ;
		unsigned int get_num_nblocks(void) const ;
		vector<unsigned int> get_successors(unsigned int b) const;
		vector<unsigned int> get_predecessors(unsigned int b) const;
	private:
		unsigned int get_id(unsigned int x, unsigned int y) const;
		vector<unsigned int> get_neighbors(unsigned int b) const;
		unsigned int cols, cols_div;
		unsigned int rows, rows_div;
	};

private:
	bool on_path(const vector<const State *> *path, int x, int y) const;
	bool is_obstacle(int x, int y) const;
	vector<const State*> *expand4(const GridState *s);
	vector<const State*> *expand8(const GridState *s);

	enum cost_type cost_type;
	enum move_type move_type;

	int width, height;
	int start_x, start_y;
	int goal_x, goal_y;
	map<int, bool> obstacles;

#if defined(ENABLE_IMAGES)
	void expanded_state(const GridState *s);

	AtomicInt expanded;
	vector<AtomicInt> states;
#endif	/* ENABLE_IMAGES */

};

#endif	/* !_GRID_WORLD_H_ */
