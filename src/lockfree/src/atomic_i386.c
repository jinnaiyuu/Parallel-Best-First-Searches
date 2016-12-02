/* © 2014 the PBNF Authors under the MIT license. See AUTHORS for the list of authors.*/

/**
 * \file atomic_i386.c
 *
 *
 *
 * \author Ethan Burns
 * \date 2009-02-12
 */

#include "atomic.h"

int test_and_set(void *_v)
{
	int ret = 1;
	void **v = _v;

	__asm__("lock; xchgl %0, %1"
		: "=r" (ret)
		: "m" (*v), "0"(ret)
		: "memory");

	return ret == 0;
}

void fetch_and_add(void *_v, intptr_t n)
{
	void **v = _v;
	__asm__("lock; addl %1, %0"
		: "=m"(*v)
		: "r"(n), "m" (*v)
		: "memory");
}

int compare_and_swap(void *_v, intptr_t old, intptr_t new)
{
	intptr_t prev = old;
	void **v = _v;

	__asm__("lock; cmpxchgl %1, %2"
		: "=a" (prev)
		: "r" (new), "m"(*v), "0"(old)
		: "memory");

	return old == prev;
}
