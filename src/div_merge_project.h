/* © 2014 the PBNF Authors under the MIT license. See AUTHORS for the list of authors.*/

/**
 * \file div_merge_project.h
 *
 *
 *
 * \author Ethan Burns
 * \date 2008-12-19
 */

#if !defined(_DIV_MERGE_PROJECT_H_)
#define _DIV_MERGE_PROJECT_H_

#include <vector>

#include "projection.h"

class State;

using namespace std;

/**
 * An abstract projection function class.
 */
class DivMergeProject : public Projection {
public:
	DivMergeProject(unsigned int div, Projection *p);

	unsigned int project(State *s) const;

	unsigned int get_num_nblocks(void) const;

	vector<unsigned int> get_successors(unsigned int b) const;

	vector<unsigned int> get_predecessors(unsigned int b) const;
private:
	unsigned int div;
	Projection *projection;
};

#endif	/* !_DIV_MERGE_PROJECT_H_ */
