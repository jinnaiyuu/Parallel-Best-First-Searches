/* © 2014 the PBNF Authors under the MIT license. See AUTHORS for the list of authors.*/

/**
 * \file atomic.h
 *
 * Atomic pointer operations.
 *
 * \author Ethan Burns
 * \date 2009-02-12
 */

#include <stdint.h>

/**
 * Test if the value pointed to by the argument is 0, if so, set it to
 * one and return true.  If not, return false.
 *
 * The first param is a pointer to a intptr_t sized value cast into a
 * void*
 */
int test_and_set(void *);

/**
 * Atomically add the second argument to the value pointed to by the
 * first argument.
 *
 * The first param is a pointer to a intptr_t sized value cast into a
 * void*
 */
void fetch_and_add(void *, intptr_t);


/**
 * Compare the 2nd argument with the value pointed to by the first, if
 * they are the same, swap in the value of the 3rd argument and return
 * true else return false.
 *
 * The first param is a pointer to a intptr_t sized value cast into a
 * void*
 */
int compare_and_swap(void *, intptr_t, intptr_t);
