/* © 2014 the PBNF Authors under the MIT license. See AUTHORS for the list of authors.*/

/**
 * \file get_search.h
 *
 *
 *
 * \author Ethan Burns
 * \date 2008-11-13
 */

#if !defined(_GET_SEARCH_H_)
#define _GET_SEARCH_H_

#include "util/fixed_point.h"
#include <vector>

using namespace std;

class Search;

extern unsigned int threads;
extern fp_type cost_bound;
extern unsigned int nblocks;
extern double weight;

Search *get_search(int argc, char *argv[]);
vector<double> *parse_weights(char *str);

#endif	/* !_GET_SEARCH_H_ */
