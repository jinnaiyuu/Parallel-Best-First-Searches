/* © 2014 the PBNF Authors under the MIT license. See AUTHORS for the list of authors.*/

/**
 * \file grid_world.h
 *
 *
 *
 * \author Ethan Burns
 * \date 2008-10-08
 */

#if !defined(_TSP_H_)
#define _TSP_H_

#include <iostream>
#include <map>
#include <string>
#include <vector>

#include "../util/atomic_int.h"

#include "../state.h"
#include "../search_domain.h"
#include "../projection.h"

class TspState;

using namespace std;



class Tsp : public SearchDomain {
public:
	Tsp(istream &s); // pairs of (x, y). Each is a coordination of the city.

	State *initial_state(void); // all not visited, visiting = 0 (home town).

	// Go to all cities other than visited ones and home.
	// If all cities other than home already visited, then go home town.
	vector<State*> *expand(State *s);

	unsigned int get_number_of_cities() const{
		return number_of_cities;
	};

	void print(ostream &o, const vector<State *> *path) const;
#if defined(ENABLE_IMAGES)
	void export_eps(string file) const;
#endif	/* ENABLE_IMAGES */

	/*
	 * The Manhattan Distance heuristic.
	 */
	class MinimumSpanningTree : public Heuristic {
	public:
		MinimumSpanningTree(const SearchDomain *d);
		fp_type compute(State *s) const;
	private:
		// min(miles(visiting, i))
//		fp_type visiting_to_mst(vector<bool> *not_visited, unsigned int current) const;
		// mst in cities not visited or visiting.
		fp_type mst(vector<bool> *not_visited) const;
	};

	class Blind : public Heuristic {
	public:
		Blind(const SearchDomain *d);
		fp_type compute(State *s) const;
	private:
	};

	/*
	 * Projection function that uses the row number mod a value.
	 */
	class RowModProject : public Projection {
	public:
		RowModProject(const SearchDomain *d, unsigned int mod_val);
		~RowModProject();
		unsigned int project(State *s) const ;
		unsigned int get_num_nblocks(void) const ;
		vector<unsigned int> get_successors(unsigned int b) const;
		vector<unsigned int> get_predecessors(unsigned int b) const;
	private:
//		vector<unsigned int> get_neighbors(unsigned int b) const;
		unsigned int mod_val;
		unsigned int max_row;
	};


private:

	static unsigned int number_of_cities;
	static double* miles; // miles between cities.

	vector<State*> *expand_usual(TspState *state);
	vector<State*> *expand_to_goal(TspState *state);

	#if defined(ENABLE_IMAGES)
	void expanded_state(GridState *s);

	AtomicInt expanded;
	vector<AtomicInt> states;
#endif	// ENABLE_IMAGES 
	

};

#endif	/* !_TSP_H_ */
