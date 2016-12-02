/* © 2014 the PBNF Authors under the MIT license. See AUTHORS for the list of authors.*/

/**
 * \file atomic_int.h
 *
 *
 *
 * \author Ethan Burns
 * \date 2008-11-10
 */

#if !defined(_ATOMIC_FLOAT_H_)
#define _ATOMIC_FLOAT_H_

#include "atomic_int.h"

/**
 * Assumes that an unsigned long is the size of a fp_type.
 */
class AtomicFloat {
public:
	AtomicFloat(void);
	AtomicFloat(double v);
	double read(void);
	void set(double);
	void add(double);
private:
	AtomicInt value;
};

#endif	/* !_ATOMIC_FLOAT_H_ */
