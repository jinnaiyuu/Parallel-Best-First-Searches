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

#define LOCK_PREFIX    "lock;"

AtomicInt::AtomicInt(unsigned long val)
{
	assert(sizeof(uint32_t) == sizeof(unsigned long));
	value = val;
}

unsigned long AtomicInt::read(void) const
{
	return value;
}

void AtomicInt::set(unsigned long i)
{
	value = i;
}

void AtomicInt::add(unsigned long i)
{
	__asm__ __volatile__(LOCK_PREFIX "addl %1,%0"
			     :"=m"(value)
			     :"ir"(i), "m"(value));
}

void AtomicInt::sub(unsigned long i)
{
	__asm__ __volatile__(LOCK_PREFIX "subl %1,%0"
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
	__asm__ __volatile__(LOCK_PREFIX "decl %0"
			     :"=m"(value)
			     :"m"(value));
}

unsigned long AtomicInt::swap(unsigned long n)
{
	__asm__ __volatile__("xchgl %0,%1"
			     : "=r" (n)
			     : "m" (value), "0" (n)
			     : "memory");

	return n;
}

unsigned long AtomicInt::cmp_and_swap(unsigned long o, unsigned long n)
{
	unsigned long prev;

	__asm__ __volatile__("cmpxchgl %1,%2"
			     : "=a"(prev)
			     : "r"(n), "m"(value), "0"(o)
			     : "memory");
	return prev;
}
