/* © 2014 the PBNF Authors under the MIT license. See AUTHORS for the list of authors.*/

/**
 * \file msa_world.h
 *
 *
 *
 * \author Ethan Burns
 * \date 2008-10-08
 */

#if !defined(_MSA_WORLD_H_)
#define _MSA_WORLD_H_

#include <iostream>
#include <map>
#include <string>
#include <vector>

#include "../util/atomic_int.h"

#include "../state.h"
#include "../search_domain.h"
#include "../projection.h"

class MSAState;

using namespace std;



class MSAWorld : public SearchDomain {
public:
	MSAWorld(istream &pam, istream &inst);


	State *initial_state(void);
	vector<State*> *expand(State *s);

	void set_abstraction(int abst){abstraction = abst;};

	class Pairwise : public Heuristic {
	public:
		Pairwise(const SearchDomain *d);
		fp_type compute(State *s) const;
		int min(int a, int b, int c) const;
		int** pairwise_tables; // Just a pointer would be faster than multiple pointers.

	};

	/*
	 * Projection function that uses the row number mod a value.
	 */
	class RowModProject : public Projection {
	public:
		RowModProject(const SearchDomain *d, unsigned int block_size);
		~RowModProject();
		unsigned int project(State *s) const ;
		unsigned int get_num_nblocks(void) const ;
		vector<unsigned int> get_successors(unsigned int b) const;
		vector<unsigned int> get_predecessors(unsigned int b) const;
	private:
//		vector<unsigned int> get_neighbors(unsigned int b) const;
		unsigned int block_size;
		int number_of_sequences;
		vector<unsigned int> lengths;
	};

	unsigned int num_of_sequences;
	int abstraction;
	std::vector<std::vector<unsigned int> > sequences;

	int* pam;
	int gapcost;

private:

	int nops(MSAState* s) const;
	int nthop(MSAState* s, int n) const;

	int calc_cost(MSAState* s, vector<unsigned int>* incs) const;

	unsigned int encode(char amino);


	char pamcode[25] =
			{ 'A', 'R', 'N', 'D', 'C', 'Q', 'E', 'G', 'H', 'I', 'L', 'K', 'M',
					'F', 'P', 'S', 'T', 'W', 'Y', 'V', 'B', 'J', 'Z', 'X', '*' };



};

#endif	/* !_GRID_WORLD_H_ */
