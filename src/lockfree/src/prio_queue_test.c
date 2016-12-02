/* © 2014 the PBNF Authors under the MIT license. See AUTHORS for the list of authors.*/

/**
 * \file prio_queue_test.c
 *
 * Tests the functionality of the priority queue.
 *
 * \author eaburns
 * \date 2009-03-29
 */

#include <sys/time.h>
#include <sys/times.h>

#include <assert.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <time.h>
#include <unistd.h>

#include "lockfree.h"

static int nodes_per_thread;
static unsigned int seed;

void *producer_thread_fun(void *arg)
{
	int i, err;
	long rand;

	struct lf_pq *pq = arg;

	for (i = 0; i < nodes_per_thread; i += 1) {
		do {
			rand = rand_r(&seed);
			rand %= 100;
			/* 0 is reserved and the lowest bit must not be set. */
		} while (rand == 0 || rand & 0x1);
		err = lf_pq_insert(pq, (void*)rand, (void*)rand);
		if (err) {
			perror(__func__);
			exit(EXIT_FAILURE);
		}
	}

	return NULL;
}

void *consumer_thread_fun(void *arg)
{
	struct lf_pq *pq = arg;
	void *min;
	intptr_t count = 0;

	while (count < nodes_per_thread) {
		min = lf_pq_delete_min(pq);
		if (min)
			count += 1;
	}

	return NULL;
}

int pred_fun(void *a, void *b)
{
	return a < b;
}

int main(int argc, char *argv[])
{
	int i;
	int ret = EXIT_FAILURE;
	int nthreads = 0;
	int nnodes = 0;
	pthread_t *consumers;
	pthread_t *producers;
	struct lf_pq *pq;
	long seed;
	struct timeval tv;
	double stime, etime;

	if (argc != 3 && argc != 4) {
		printf("Usage: prio_queue_test <npairs> <n-per-thread>\n");
		return EXIT_FAILURE;
	}
	sscanf(argv[1], "%d", &nthreads);
	if (nthreads <= 0) {
		printf("Must have at least one producer/consumer pair thread.\n");
		return EXIT_FAILURE;
	}
	sscanf(argv[2], "%d", &nodes_per_thread);
	if (nodes_per_thread <= 0) {
		printf("Must have at least one node-per-thread.\n");
		return EXIT_FAILURE;
	}
	if (argc == 4)
		sscanf(argv[3], "%ld", &seed);
	else
		seed = time(NULL);

	printf("Seeding the random number generator: %ld\n", seed);
	seed = time(NULL);

	nnodes = nthreads * nodes_per_thread;
	printf("Creating a priority queue with %d nodes\n", nnodes);

	pq = lf_pq_create(nnodes, pred_fun);
	if (!pq) {
		perror(__func__);
		return EXIT_FAILURE;
	}

	printf("Creating %d consumers, and %d producers\n", nthreads, nthreads);
	printf("Writing %d elements each producer\n", nodes_per_thread);

	consumers = calloc(nthreads, sizeof(*consumers));
	if (!consumers) {
		perror(__func__);
		goto err_consumers;
	}

	producers = calloc(nthreads, sizeof(*producers));
	if (!producers) {
		perror(__func__);
		goto err_producers;
	}

	gettimeofday(&tv, NULL);
	stime = tv.tv_usec;
	stime /= 1000000;
	stime += tv.tv_sec;

	/* Create all of the consumers */
	for (i = 0; i < nthreads; i += 1)
		pthread_create(consumers + i, NULL, consumer_thread_fun, pq);

	/* Create all of the producers. */
	for (i = 0; i < nthreads; i += 1)
		pthread_create(producers + i, NULL, producer_thread_fun, pq);

	/* Wait for it. */
	for (i = 0; i < nthreads; i += 1)
		pthread_join(*(producers + i), NULL);

	for (i = 0; i < nthreads; i += 1)
		pthread_join(*(consumers + i), NULL);

	gettimeofday(&tv, NULL);
	etime = tv.tv_usec;
	etime /= 1000000;
	etime += tv.tv_sec;

	printf("%d values written in %f seconds\n", nnodes, etime - stime);

/*
	lf_pq_print(stdout, pq);
	assert(lf_pq_property_holds(stdout, pq));
*/

	ret = EXIT_SUCCESS;
	free(producers);

err_producers:
	free(consumers);

err_consumers:
	lf_pq_destroy(pq);

	return ret;
}

