/* © 2014 the PBNF Authors under the MIT license. See AUTHORS for the list of authors.*/

/**
 * \file completion_counter.h
 *
 *
 *
 * \author Ethan Burns
 * \date 2008-10-24
 */

#if !defined(_COMPLETION_COUNTER_H_)
#define _COMPLETION_COUNTER_H_

#include "atomic_int.h"
#include <pthread.h>

class CompletionCounter {
public:
	CompletionCounter(void);
	CompletionCounter(unsigned int max);

	void set_max(unsigned int max);
        unsigned int get_count();

	void complete(void);
	void uncomplete(void);
	void wait(void);
	void reset(void);
        bool is_complete(void);

private:
	AtomicInt counter;
	AtomicInt max;
};

#endif	/* !_COMPLETION_COUNTER_H_ */
