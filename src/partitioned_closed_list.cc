// © 2014 the PBNF Authors under the MIT license. See AUTHORS for the list of authors.

/**
 * \file synch_closed_list.h
 *
 *
 *
 * \author Sofia Lemons
 * \date 2008-10-13
 */

#include <pthread.h>
#include <stdio.h>
#include "partitioned_closed_list.h"

PartitionedClosedList::PartitionedClosedList(unsigned int closedlistsize,
		unsigned int closedlistdivision) :
		ClosedList(closedlistsize), mutex(closedlistdivision), closedlistdivision(
				closedlistdivision) {
	for (unsigned int i = 0; i < closedlistdivision; ++i) {
		pthread_mutex_init(&(mutex[i]), NULL);
	}
}

void PartitionedClosedList::add(State *s) {
	unsigned dist_hash = s->hash() % closedlistdivision;
//	printf("dist_hash = %u\n", dist_hash);
	pthread_mutex_lock(&mutex[dist_hash]);
	ClosedList::add(s);
	pthread_mutex_unlock(&mutex[dist_hash]);
}

// Hey, you don't need a lock for lookup i guess.
State *PartitionedClosedList::lookup(State *c) {
//	pthread_mutex_lock(&mutex);
	State *s = ClosedList::lookup(c);
//	pthread_mutex_unlock(&mutex);
	return s;
}

void PartitionedClosedList::delete_all_states(void) {
	for (unsigned int i = 0; i < closedlistdivision; ++i) {
		pthread_mutex_lock(&mutex[i]);
	}
	ClosedList::delete_all_states();
	for (unsigned int i = 0; i < closedlistdivision; ++i) {
		pthread_mutex_unlock(&mutex[i]);
	}
}
