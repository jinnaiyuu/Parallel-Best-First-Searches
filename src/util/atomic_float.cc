// © 2014 the PBNF Authors under the MIT license. See AUTHORS for the list of authors.

/**
 * \file atomic_int.cc
 *
 *
 *
 * \author Ethan Burns
 * \date 2008-11-10
 */

#include <assert.h>

#include "fixed_point.h"

#include "atomic_int.h"
#include "atomic_float.h"

union cast_union {
	uint64_t l;
	double f;
};

AtomicFloat::AtomicFloat(void)
{
	assert(sizeof(uint64_t) == sizeof(double));
	set(0);
}

AtomicFloat::AtomicFloat(double v)
{
	assert(sizeof(uint64_t) == sizeof(double));
	set(v);
}

double AtomicFloat::read(void)
{
	union cast_union c;
	c.l = value.read();
	return c.f;
}

void AtomicFloat::set(double v)
{
	union cast_union c;
	c.f = v;
	value.set(c.l);
}

void AtomicFloat::add(double v)
{
	union cast_union o, n;

	do {
		o.l = value.read();
		n.f = o.f + v;
	} while (value.cmp_and_swap(o.l, n.l) != o.l);
}
