// © 2014 the PBNF Authors under the MIT license. See AUTHORS for the list of authors.

/*
 * \file sparc64_atomic_int.cc
 *
 * \author Ethan Burns
 * \date 2008-10-22
 */

#include <assert.h>
#include <atomic.h>
#include <stdint.h>

#include "atomic_int.h"

AtomicInt::AtomicInt(void)
	: value(0)
{
}

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
	assert(((int64_t)i) > 0);
	atomic_add_64(&value, i);
}

void AtomicInt::sub(uint64_t i)
{
	assert(((int64_t)i) > 0);
	atomic_add_64(&value, -((int64_t)i));
}

void AtomicInt::inc(void)
{
	atomic_inc_64(&value);
}

void AtomicInt::dec(void)
{
	atomic_dec_64(&value);
}

uint64_t AtomicInt::swap(uint64_t n)
{
	return atomic_swap_64(&value, n);
}

uint64_t AtomicInt::cmp_and_swap(uint64_t o, uint64_t n)
{
	return atomic_cas_64(&value, o, n);
}

