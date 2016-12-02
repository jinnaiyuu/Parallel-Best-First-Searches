// © 2014 the PBNF Authors under the MIT license. See AUTHORS for the list of authors.

/**
 * \file x86_64_atomic_int.cc
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

#define LOCK_PREFIX    "lock;"

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
	__asm__ __volatile__(LOCK_PREFIX "addq %1,%0"
			     :"=m"(value)
			     :"ir"(i), "m"(value));
}

void AtomicInt::sub(uint64_t i)
{
	__asm__ __volatile__(LOCK_PREFIX "subq %1,%0"
			     :"=m"(value)
			     :"ir"(i), "m"(value));
}

void AtomicInt::inc(void)
{
	__asm__ __volatile__(LOCK_PREFIX "incl %0"
			     :"=m"(value)
			     :"m"(value));
}

void AtomicInt::dec(void)
{
	__asm__ __volatile__(LOCK_PREFIX "decq %0"
			     :"=m"(value)
			     :"m"(value));
}

uint64_t AtomicInt::swap(uint64_t n)
{
	__asm__ __volatile__("xchgq %0,%1"
			     : "=r" (n)
			     : "m" (value), "0" (n)
			     : "memory");

	return n;
}

uint64_t AtomicInt::cmp_and_swap(uint64_t o, uint64_t n)
{
	uint64_t prev;

	__asm__ __volatile__("cmpxchgq %1,%2"
			     : "=a"(prev)
			     : "r"(n), "m"(value), "0"(o)
			     : "memory");
	return prev;
}
