/* © 2014 the PBNF Authors under the MIT license. See AUTHORS for the list of authors.*/

/**
 * \file a_star.h
 *
 * Contains the AStar class.
 *
 * \author Ethan Burns
 * \date 2008-10-09
 */

#if !defined(_A_STAR_VECTOR_H_)
#define _A_STAR_VECTOR_H_

#include "state.h"
#include "search.h"
#include "closed_list.h"
#include "pq_open_list.h"
#include "pq_vector_open_list.h"

/**
 * An A* search class.
 */
class AStarVector : public Search {
public:
	AStarVector(unsigned int openlistsize);
	AStarVector(unsigned int openlistsize, unsigned int closedlistsize);
//	AStarVector(bool dd);
	virtual ~AStarVector(void);
	virtual vector<State *> *search(Timer *, State *);

	void output_stats(void);
private:
	PQVectorOpenList open;
	ClosedList closed;
	bool dd;		/* dup dropping */
};

#endif	/* !_A_STAR_H_ */
