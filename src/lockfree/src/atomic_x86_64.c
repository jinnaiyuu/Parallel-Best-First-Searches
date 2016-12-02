/* © 2014 the PBNF Authors under the MIT license. See AUTHORS for the list of authors.*/

/**
 * \file atomic_x86_64.c
 *
 *
 *
 * \author Ethan Burns
 * \date 2009-02-12
 */

#include "atomic.h"

int test_and_set(void *_v)
{
	intptr_t ret = 1;
	void **v = (void*) _v;

	__asm__("lock; xchgq %0, %1"
		: "=r" (ret)
		: "m" (*v), "0"(ret)
		: "memory");

	return ret == 0;
}

void fetch_and_add(void *_v, intptr_t n)
{
	void **v = (void*) _v;
	__asm__("lock; addq %1, %0"
		: "=m"(*v)
		: "r"(n), "m" (*v)
		: "memory");
}

int compare_and_swap(void *_v, intptr_t old, intptr_t new)
{
	intptr_t prev = old;
	void **v = (void*) _v;

	__asm__("lock; cmpxchgq %1, %2"
		: "=a" (prev)
		: "r" (new), "m"(*v), "0"(old)
		: "memory");

	return old == prev;
}
