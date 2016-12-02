/* © 2014 the PBNF Authors under the MIT license. See AUTHORS for the list of authors.*/

/**
 * \file ordlist_test.c
 *
 *
 *
 * \author eaburns
 * \date 2009-03-19
 */

#define _POSIX_C_SOURCE 200112L

#include <assert.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#include "atomic.h"
#include "lockfree.h"

static int nodes_per_thread;

static intptr_t value;
static intptr_t rvalue;
static int done_producing = 0;

void *producer_thread_fun(void *arg)
{
	int i;
	intptr_t old, new;

	struct lf_ordlist *lst = arg;

	for (i = 0; i < nodes_per_thread; i += 1) {
		do {
			old = value;
			new = old - 1;
		} while (!compare_and_swap(&value, old, new));

		lf_ordlist_add(lst, (void*) new);
	}

	return NULL;
}

void *consumer_thread_fun(void *arg)
{
	int success;
	intptr_t old, new;
	struct lf_ordlist *lst = arg;


	while (!done_producing || !lf_ordlist_empty(lst)) {
		if (lf_ordlist_find(lst, (void*) rvalue)) {
			success = lf_ordlist_remove(lst, (void*) rvalue);
			if (success) {
				do {
					old = rvalue;
					new = rvalue - 1;
				} while (!compare_and_swap(&rvalue, old, new));
			}
		}
	}

	return NULL;
}

int cmp_elms(void *a, void *b)
{
	return b - a;
}

int main(int argc, char *argv[])
{
	int ret = EXIT_FAILURE;
	int i;
	int nconsumers = 0;
	int nproducers = 0;
	int nnodes = 0;
	pthread_t *consumers, *producers;
	struct lf_ordlist *ordlist;

	if (argc != 4) {
		printf("Usage: ordlist_test <nconsumers> <nproducers> <n-per-thread>\n");
		return EXIT_FAILURE;
	}
	sscanf(argv[1], "%d", &nconsumers);
	if (nconsumers <= 0) {
		printf("Must have at least one consumer.\n");
		return EXIT_FAILURE;
	}
	sscanf(argv[2], "%d", &nproducers);
	if (nproducers <= 0) {
		printf("Must have at least one producer.\n");
		return EXIT_FAILURE;
	}
	sscanf(argv[3], "%d", &nodes_per_thread);
	if (nodes_per_thread <= 0) {
		printf("Must have at least one node-per-thread.\n");
		return EXIT_FAILURE;
	}

	nnodes = nproducers * nodes_per_thread;
	value = nnodes + 1;
	rvalue = nnodes;
	printf("Creating a ordlist with %d nodes\n", nnodes);

	ordlist = lf_ordlist_create(nnodes, cmp_elms);
	if (!ordlist) {
		perror("");
		return EXIT_FAILURE;
	}

	printf("Creating %d consumers, and %d producers\n", nconsumers, nproducers);
	printf("Writing %d elements each producer\n", nodes_per_thread);

	consumers = calloc(nconsumers, sizeof(*consumers));
	if (!consumers) {
		perror("");
		goto err_consumers;
	}

	producers = calloc(nproducers, sizeof(*producers));
	if (!producers) {
		perror("");
		goto err_producers;
	}

	for (i = 0; i < nproducers; i += 1)
		pthread_create(producers + i, NULL, producer_thread_fun, ordlist);

	for (i = 0; i < nconsumers; i += 1)
		pthread_create(consumers + i, NULL, consumer_thread_fun, ordlist);

	for (i = 0; i < nproducers; i += 1)
		pthread_join(*(producers + i), NULL);
	done_producing = 1;

/*
	fprintf(stdout, "Printing:\n");
	lf_ordlist_print(stdout, ordlist);
*/

	for (i = 0; i < nconsumers; i += 1)
		pthread_join(*(consumers + i), NULL);

	printf("%d values written\n", nnodes);
	ret = EXIT_SUCCESS;
	free(producers);

err_producers:
	free(consumers);

err_consumers:
	lf_ordlist_destroy(ordlist);

	return ret;
}

