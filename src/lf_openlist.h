/* © 2014 the PBNF Authors under the MIT license. See AUTHORS for the list of authors.*/

/**
 * \file lf_openlist.h
 *
 *
 *
 * \author eaburns
 * \date 2009-04-12
 */

#if !defined(_LF_OPENLIST_H_)
#define _LF_OPENLIST_H_

#include "open_list.h"

extern "C" {
#include "lockfree/include/lockfree.h"
}

class LF_OpenList : public OpenList {
public:
	LF_OpenList(void);
	~LF_OpenList(void);

	void add(State *s);
	State *take(void);
	State *peek(void);
	bool empty(void);
	void delete_all_states(void);
	void prune(void);
	unsigned int size(void);

	list<State*> *states(void);
private:
	struct lf_pq *pq;
	AtomicInt fill;
};

#endif	/* !_LF_OPENLIST_H_ */
