/* © 2014 the PBNF Authors under the MIT license. See AUTHORS for the list of authors.*/

/**
 * \file atomic_sun4v.c
 *
 *
 *
 * \author Ethan Burns
 * \date 2009-02-12
 */

#include <atomic.h>
#include "atomic.h"

int test_and_set(void *v)
{
	int old;
	old = atomic_swap(v, 1);
	return old != 0;
}

void fetch_and_add(void *v, intptr_t n)
{
	atomic_add(v, n);
}

int compare_and_swap(void *v, intptr_t old, intptr_t new)
{
	return atomic_cas(v, old, new) == old;
}
