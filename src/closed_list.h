/**
 * \file closed_list.h
 *
 * A simple closed list class.
 *
 * \author Ethan Burns
 * \date 2008-10-09
 */

#if !defined(_CLOSED_LIST_H_)
#define _CLOSED_LIST_H_

#include "state.h"

using namespace std;

/**
 * A simple closed list class.
 */
class ClosedList {
public:
	ClosedList(void);
	ClosedList(unsigned long size);
	~ClosedList(void);
	void add(State *);
	State *lookup(State *);
	void delete_all_states(void);

private:
	void init(unsigned long size);
	void new_table(void);
	void resize(void);
	void do_add(State *s);
	unsigned long get_ind(State *s);

	class Bucket {
	public:
		Bucket(State *data, Bucket *next);
		~Bucket(void);

		State *lookup(State *s);
		Bucket *add(State *s);

		State *data;
		Bucket *next;
		unsigned int size;
	};

	Bucket **table;
	unsigned long size;
	unsigned long fill;
};

#endif	/* !_CLOSED_LIST_H_ */