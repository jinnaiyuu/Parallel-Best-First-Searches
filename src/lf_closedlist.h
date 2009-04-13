/**
 * \file lf_closedlist.h
 *
 *
 *
 * \author eaburns
 * \date 2009-04-12
 */

#if !defined(_LF_CLOSEDLIST_H_)
#define _LF_CLOSEDLIST_H_

#include "state.h"
#include "closed_list.h"

using namespace std;

extern "C" {
#include "map.h"
#include "hashtable.h"
}

/**
 * A simple closed list class.
 */
class LF_ClosedList : public ClosedList {
public:
	LF_ClosedList(void);
	~LF_ClosedList(void);

	void add(State *);
	State *lookup(State *);
	void delete_all_states(void);

private:
	map_t *map;
};

#endif	/* !_LF_CLOSEDLIST_H_ */