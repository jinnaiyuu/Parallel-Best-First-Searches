/* © 2014 the PBNF Authors under the MIT license. See AUTHORS for the list of authors.*/

/**
 * \file dynamic_bounded_psdd.h
 *
 *
 *
 * \author Ethan Burns
 * \date 2008-10-27
 */

#if !defined(_DYNAMIC_BOUNDED_PSDD_H_)
#define _DYNAMIC_BOUNDED_PSDD_H_

#include "search.h"
#include "state.h"

class DynamicBoundedPSDD : public Search {
public:
        DynamicBoundedPSDD(unsigned int n_threads, fp_type weight);

	virtual ~DynamicBoundedPSDD(void);
	virtual vector<State *> *search(Timer *t, State *s);
private:
	unsigned int n_threads;
	fp_type weight;
};

#endif	/* !_DYNAMIC_BOUNDED_PSDD_H_ */
