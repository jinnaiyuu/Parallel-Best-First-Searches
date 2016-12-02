/* © 2014 the PBNF Authors under the MIT license. See AUTHORS for the list of authors.*/

/**
 * \file backoff.h
 *
 * This performs delaying using an exponential scheme based on the one
 * used in ``A Methodology for Implementing Highly Concurrent Data
 * Objects'' -- M. Herlihy.
 *
 * \author eaburns
 * \date 2009-04-10
 */
#if !defined(_BACKOFF_H_)
#define _BACKOFF_H_

#include <pthread.h>
#include <stdlib.h>
#include <time.h>

#include "atomic.h"

/** The max delay. */
#define DELAY_MAX 100000

/** The max delay is multiplied by this value via a loop so that we
 * never run out of bits in our 'long' datatype (even on small 32-bit
 * machines). */
#define DELAY_MULT 100

static pthread_key_t key_buffer;
static pthread_once_t once_buffer = PTHREAD_ONCE_INIT;

static void init_keys(void)
{
	pthread_key_create(&key_buffer, NULL);
}

static __inline__ void backoff_decrease(void)
{
	long max;

	pthread_once(&once_buffer, init_keys);

	max = (long) pthread_getspecific(key_buffer);
	if (max == 0)
		max = DELAY_MAX;

	if (max > 1)
		max = max / 2;
	if (max == 0)
		max = 1;

	pthread_setspecific(key_buffer, (void*) max);
}

/**
 * Delays for a random amount (less than [max]) and returns the new
 * max value.
 */
static __inline__ void backoff(void)
{
	static int seeded = 0;
	static unsigned int seed;
	long max;

	int j;
	long i, delay;

	if (!seeded) {
		seed = time(NULL);
		seeded = 1;
	}

	max = (long) pthread_getspecific(key_buffer);

	delay = rand_r(&seed);
	assert(max != 0);
	delay %= max;

	for (j = 0; j < DELAY_MULT; j += 1) {
		for (i = 0; i < delay; i += 1)
			__asm__ __volatile__ ("");
	}

	max = max < DELAY_MAX ? max * 2 : max;
	pthread_setspecific(key_buffer, (void*) max);
}

#endif	/* !_BACKOFF_H_ */
