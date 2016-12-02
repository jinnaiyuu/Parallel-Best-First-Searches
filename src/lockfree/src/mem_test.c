/* © 2014 the PBNF Authors under the MIT license. See AUTHORS for the list of authors.*/

/**
 * \file mem_test.c
 *
 * "Test" the memory manager by spawning threads which allocate and
 * reclaim memory.
 *
 * \author eaburns
 * \date 2009-03-18
 */

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>

#define MAX_LINKS 10000
#include "mem.h"

static int num_allocs;
static int iterations;

void *thread_fun(void *arg)
{
	struct freelist *fl = arg;
	struct node **n;
	char errstr[255];
	int iter, i, j;

	n = calloc(num_allocs, sizeof(*n));
	if (!n) {
		perror("");
		exit(EXIT_FAILURE);
	}

	for (iter = 0; iter < iterations; iter += 1) {
		for (i = 0; i < num_allocs; i += 1) {
			n[i] = mem_new(fl);
			if (!n[i]) {
				snprintf(errstr, 255, "alloc %d", i);
				perror(errstr);
			}
			for (j = 0; j < MAX_LINKS; j += 1)
				n[i]->links[j] = (void*) pthread_self();
		}
		for (i = 0; i < num_allocs; i += 1) {
			for (j = 0; j < MAX_LINKS; j += 1) {
				if (n[i]->links[j] != (void*) pthread_self()) {
					printf("Race condition found by thread %#lx saw %#lx\n",
					       (unsigned long) pthread_self(),
					       (unsigned long) n[i]->links[j]);
					exit(EXIT_FAILURE);
				} else {
					n[i]->links[j] = NULL;
				}
			}
			mem_release(fl, n[i]);
		}
	}

	free(n);

	return NULL;
}

int main(int argc, char *argv[])
{
	int i;
	int nthreads = 0;
	int nnodes;
	pthread_t *threads;
	struct freelist *fl;

	if (argc != 4) {
		printf("Usage: mem_test <threads> <iters> <allocations>\n");
		return EXIT_FAILURE;
	}

	sscanf(argv[1], "%d", &nthreads);
	if (nthreads <= 0) {
		printf("Must have at least one thread.\n");
		return EXIT_FAILURE;
	}
	sscanf(argv[2], "%d", &iterations);
	if (iterations <= 0) {
		printf("Must have at least one iteration.\n");
		return EXIT_FAILURE;
	}
	sscanf(argv[3], "%d", &num_allocs);
	if (num_allocs <= 0) {
		printf("Must have at least allocation\n"); 
		return EXIT_FAILURE;
	}

	nnodes = nthreads * num_allocs;
	printf("Creating a free list with %d nodes\n", nnodes);

	fl = mem_freelist_create(nnodes, MAX_LINKS, sizeof(*fl));
	if (!fl) {
		perror("");
		return EXIT_FAILURE;
	}

	printf("Creating %d threads each performing %d "
	       "allocations and releases\n",
	       nthreads, num_allocs);

	threads = calloc(nthreads, sizeof(*threads));
	if (!threads) {
		perror("");
		mem_freelist_destroy(fl);
		return EXIT_FAILURE;
	}

	for (i = 0; i < nthreads; i += 1) {
		pthread_create(threads + i, NULL, thread_fun, fl);
		printf("Thread %#lx started\n", (unsigned long) (threads + i));
	}

	for (i = 0; i < nthreads; i += 1)
		pthread_join(*(threads + i), NULL);

	free(threads);
	mem_freelist_destroy(fl);

	return EXIT_SUCCESS;
}
