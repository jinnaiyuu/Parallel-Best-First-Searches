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

#include "synch_closed_list.h"

SynchClosedList::SynchClosedList(void) {
	pthread_mutex_init(&mutex, NULL);
}

void SynchClosedList::add(State *s)
{
	pthread_mutex_lock(&mutex);
	ClosedList::add(s);
	pthread_mutex_unlock(&mutex);
}

State *SynchClosedList::lookup(State *c)
{
        pthread_mutex_lock(&mutex);
	State *s = ClosedList::lookup(c);
        pthread_mutex_unlock(&mutex);
        return s;
}

void SynchClosedList::delete_all_states(void)
{
	pthread_mutex_lock(&mutex);
	ClosedList::delete_all_states();
	pthread_mutex_unlock(&mutex);
}
