/* © 2014 the PBNF Authors under the MIT license. See AUTHORS for the list of authors.*/

/**
 * \file awastar.h
 *
 *
 *
 * \author eaburns
 * \date 2009-04-20
 */
#if !defined(_AWASTAR_H_)
#define _AWASTAR_H_

#include "util/solution_stream.h"
#include "state.h"
#include "search.h"
#include "closed_list.h"

/**
 * An A* search class.
 */
class AwAStar : public Search {
public:
	AwAStar(void);
	virtual ~AwAStar(void);
	virtual vector<State *> *search(Timer *, State *);
	void output_stats(void);
private:
	SolutionStream *solutions;
	ClosedList closed;
};

#endif	/* !_AWASTAR_H_ */

