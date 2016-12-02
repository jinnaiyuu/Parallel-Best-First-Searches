/* © 2014 the PBNF Authors under the MIT license. See AUTHORS for the list of authors.*/

/**
 * \file ida_star.h
 *
 *
 *
 * \author Ethan Burns
 * \date 2008-10-10
 */

#if !defined(_IDA_STAR_H_)
#define _IDA_STAR_H_

#include <vector>
#include <list>

#include "state.h"
#include "search.h"

using namespace std;

class IDAStar : public Search {
public:
	virtual vector<State *> *search(Timer *, State *);
	virtual void output_stats(void);
private:
	struct iteration {
		unsigned int no;
		fp_type bound;
		unsigned long exp;
		unsigned long gen;
		unsigned long new_exp;
	};

	list<iteration> iters;
};

#endif	/* !_IDA_STAR_H_ */
