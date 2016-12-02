/* © 2014 the PBNF Authors under the MIT license. See AUTHORS for the list of authors.*/

/**
 * \file incons_list.h
 *
 *
 *
 * \author eaburns
 * \date 2009-04-19
 */
#if !defined(_INCONS_LIST_H_)
#define _INCONS_LIST_H_

#include "util/hash_table.h"
#include "state.h"
#include "open_list.h"

using namespace std;

/**
 * A simple inconsintency list class.
 */
class InconsList {
public:
	InconsList(void);
	InconsList(unsigned long size);
	virtual ~InconsList(void);
	virtual void add(State *);
	virtual State *lookup(State *);
	virtual bool empty();

	unsigned long size(void);

	virtual void re_open(OpenList *);

private:
	static void __do_re_open(void *, State *);

	HashTable<State> tbl;
};

#endif	/* !_INCONS_LIST_H_ */

