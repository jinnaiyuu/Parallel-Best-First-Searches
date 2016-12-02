/* © 2014 the PBNF Authors under the MIT license. See AUTHORS for the list of authors.*/

/**
 * \file queue_test.c
 *
 * Test the lock free queue.
 *
 * \author eaburns
 * \date 2009-03-18
 */

#define _POSIX_C_SOURCE 200112L

#include <assert.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>

#include "atomic.h"
#include "lockfree.h"

static int nodes_per_thread;

static intptr_t value;

void *producer_thread_fun(void *arg)
{
	int i, err;
	intptr_t old, new;

	struct lf_queue *queue = arg;

	for (i = 0; i < nodes_per_thread; i += 1) {
		do {
			old = value;
			new = old - 1;
		} while (!compare_and_swap(&value, old, new));

		err = lf_queue_enqueue(queue, (void*) value);
		if (err) {
			perror(__func__);
			exit(EXIT_FAILURE);
		}
	}

	return NULL;
}

void *consumer_thread_fun(void *arg)
{
	struct lf_queue *queue = arg;
/*
	intptr_t val;
*/


	while (value > 1 || !lf_queue_empty(queue)) {
/*
		val = (intptr_t) lf_queue_dequeue(queue);
		if (val)
			printf("%lu\n", (unsigned long) val);
*/
		lf_queue_dequeue(queue);
	}


	return NULL;
}

int main(int argc, char *argv[])
{
	int ret = EXIT_FAILURE;
	int i;
	int nconsumers = 0;
	int nproducers = 0;
	int nnodes = 0;
	pthread_t *consumers, *producers;
	struct lf_queue *queue;

	if (argc != 4) {
		printf("Usage: queue_test <nconsumers> <nproducers> <n-per-thread>\n");
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
	printf("Creating a queue with %d nodes\n", nnodes);

	queue = lf_queue_create(nnodes);
	if (!queue) {
		perror(__func__);
		return EXIT_FAILURE;
	}

	printf("Creating %d consumers, and %d producers\n", nconsumers, nproducers);
	printf("Writing %d elements each producer\n", nodes_per_thread);

	consumers = calloc(nconsumers, sizeof(*consumers));
	if (!consumers) {
		perror(__func__);
		goto err_consumers;
	}

	producers = calloc(nproducers, sizeof(*producers));
	if (!producers) {
		perror(__func__);
		goto err_producers;
	}

	for (i = 0; i < nconsumers; i += 1)
		pthread_create(consumers + i, NULL, consumer_thread_fun, queue);

	for (i = 0; i < nproducers; i += 1)
		pthread_create(producers + i, NULL, producer_thread_fun, queue);

	for (i = 0; i < nproducers; i += 1)
		pthread_join(*(producers + i), NULL);

	for (i = 0; i < nconsumers; i += 1)
		pthread_join(*(consumers + i), NULL);

	printf("%d values written\n", nnodes);
	ret = EXIT_SUCCESS;
	free(producers);

err_producers:
	free(consumers);

err_consumers:
	lf_queue_destroy(queue);

	return ret;
}
