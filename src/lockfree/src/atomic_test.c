/* © 2014 the PBNF Authors under the MIT license. See AUTHORS for the list of authors.*/

/**
 * \file atomic_test.c
 *
 * Test the basic atomic operations.
 *
 * \author eaburns
 * \date 2009-03-18
 */

#include <stdio.h>
#include <stdlib.h>
#include "atomic.h"

int main(void)
{
	int i = 0;

	printf("1: test_and_set, i = 0\n");
	if (!test_and_set(&i) || i != 1) {
		printf("\tfailed, i = %d\n", i);
		return EXIT_FAILURE;
	}
	printf("\tsuccess i = %d\n\n", i);

	printf("2: test_and_set, i != 0\n");
	if (test_and_set(&i) || i != 1) {
		printf("\tfailed, i = %d\n", i);
		return EXIT_FAILURE;
	}
	printf("\tsuccess i = %d\n\n", i);

	printf("3: fetch_and_add, i = 1 add 41\n");
	fetch_and_add(&i, 41);
	if (i != 42) {
		printf("\tfailed, i = %d\n", i);
		return EXIT_FAILURE;
	}
	printf("\tsuccess i = %d\n\n", i);

	printf("4: fetch_and_add, i = 42 add -41\n");
	fetch_and_add(&i, -41);
	if (i != 1) {
		printf("\tfailed, i = %d\n", i);
		return EXIT_FAILURE;
	}
	printf("\tsuccess i = %d\n\n", i);

	printf("5: compare_and_swap, i = 1 old = 1 new = 42\n");
	if (!compare_and_swap(&i, 1, 42) || i != 42) {
		printf("\tfailed, i = %d\n", i);
		return EXIT_FAILURE;
	}
	printf("\tsuccess i = %d\n\n", i);

	printf("6: compare_and_swap, i = 42 old = 1 new = 5\n");
	if (compare_and_swap(&i, 1, 5) || i != 42) {
		printf("\tfailed, i = %d\n", i);
		return EXIT_FAILURE;
	}
	printf("\tsuccess i = %d\n\n", i);

	return EXIT_SUCCESS;
}
