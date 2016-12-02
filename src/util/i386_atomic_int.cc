// © 2014 the PBNF Authors under the MIT license. See AUTHORS for the list of authors.

/**
 * \file i386_atomic_int.cc
 *
 * Atomic integer operations.  This code uses code from
 * liblinuxkernel, which means that it was indirrectly taken from the
 * Linux kernel.
 *
 * \author Ethan Burns
 * \date 2008-10-21
 */

#include <assert.h>

#include "atomic_int.h"

// defined in cmpxchg_i386.s
extern "C" int cmpxchg_64(volatile uint64_t *v, uint64_t o, uint64_t n);

AtomicInt::AtomicInt(void)
	: value(0) {}

AtomicInt::AtomicInt(uint64_t val)
	: value(val)
{
}

uint64_t AtomicInt::read(void) const
{
	return value;
}

void AtomicInt::set(uint64_t i)
{
	value = i;
}

void AtomicInt::add(uint64_t i)
{
	uint64_t old = value;

	while (!cmpxchg_64(&value, old, value + i))
		old = value;
}

void AtomicInt::sub(uint64_t i)
{
	uint64_t old = value;

	while (!cmpxchg_64(&value, old, value - i))
		old = value;
}

void AtomicInt::inc(void)
{
	add(1);
}

void AtomicInt::dec(void)
{
	sub(1);
}

uint64_t AtomicInt::swap(uint64_t n)
{
	unsigned long old = value;

	while(!cmpxchg_64(&value, old, n))
		old = value;

	return old;
}

uint64_t AtomicInt::cmp_and_swap(uint64_t o, uint64_t n)
{
	if (cmpxchg_64(&value, o, n))
		return o;
	else
		return value;
}
