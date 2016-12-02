/* © 2014 the PBNF Authors under the MIT license. See AUTHORS for the list of authors.*/

/**
 * \file optimistic.h
 *
 *
 *
 * \author eaburns
 * \date 2009-06-02
 */

#if !defined(_OPTIMISTIC_H_)
#define _OPTIMISTIC_H_

#include "state.h"
#include "search.h"
#include "closed_list.h"

/**
 * An A* search class.
 */
class Optimistic : public Search {
public:
	Optimistic(double bound);
	virtual ~Optimistic(void);
	virtual vector<State *> *search(Timer *, State *);
private:
	State *get_next_state(void);
	void open_add_node(State *s);
	void open_see_update(State *s);
	void set_incumbent(State *s);
	bool search_complete(void);

	ClosedList closed;
	PQOpenList<State::PQOpsF> open_f;
	PQOpenList<State::PQOpsFPrime> open_fp;

	vector<State *> *incumbent;
	double incumbent_cost;
	double bound;
};

#endif	/* !_OPTIMISTIC_H_ */
