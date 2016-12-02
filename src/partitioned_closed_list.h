/* © 2014 the PBNF Authors under the MIT license. See AUTHORS for the list of authors.*/

/**
 * \file synch_closed_list.h
 *
 *
 *
 * \author Sofia Lemons
 * \date 2008-10-13
 */

#if !defined(_PARTITIONED_CLOSED_LIST_H_)
#define _PARTITIONED_CLOSED_LIST_H_

#include <pthread.h>
#include <vector>

#include "closed_list.h"

/**
 * A thread safe ClosedList implementation.
 */

class PartitionedClosedList : public ClosedList {
public:
	PartitionedClosedList(unsigned int closedlistsize, unsigned int closedlistdivision);

	void add(State *);
	State *lookup(State *);
	void delete_all_states(void);

private:
	std::vector<pthread_mutex_t> mutex;
	unsigned int closedlistdivision;
};

#endif
