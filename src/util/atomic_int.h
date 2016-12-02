/* © 2014 the PBNF Authors under the MIT license. See AUTHORS for the list of authors.*/

/**
 * \file atomic_int.h
 *
 * Atomic integer operations.  This code uses code from
 * liblinuxkernel, which means that it was indirrectly taken from the
 * Linux kernel.
 *
 * \author Ethan Burns
 * \date 2008-10-14
 */

#if !defined(_ATOMIC_INT_H_)
#define _ATOMIC_INT_H_

#include <stdint.h>

/**
 * This class defines an atomic integer.  The operations provided as
 * methods of this class can be used concurrently by multiple
 * processors to access this interger value.
 *
 * \note A these methods are currently x86 specific.  Unfortunately we
 *       will need to add pre-processor macros to make them portable.
 */
class AtomicInt {
public:
	AtomicInt(void);

	AtomicInt(uint64_t val);

	uint64_t read(void) const;

	void set(uint64_t i);

	void add(uint64_t i);

	void sub(uint64_t i);

	void inc(void);

	void dec(void);

	uint64_t swap(uint64_t n);

	/**
	 * Atomic compare and swap.  This operation takes an old value
	 * and a new value.  If the AtomicInt currently contains the
	 * old value passed as a parameter here, the new value is
	 * atomically swapped in.  If the AtomicInt contains a value
	 * that is not the old value given as a parameter, the new
	 * value is *not* swapped in.
	 * \param o The expected old value.
	 * \param n The new value to swap if the current value is 'o'.
	 * \return The value before this operation.  If the return
	 *         value is equivilant to 'o', then the operation was
	 *         a success, otherwise it was not.
	 */
	uint64_t cmp_and_swap(uint64_t o, uint64_t n);
private:

	/* If this ever gets changed, change the type of fp_type in
	 * the fixed_point.h file. */
	volatile uint64_t value;
};

#endif	/* !_ATOMIC_INT_H_ */
