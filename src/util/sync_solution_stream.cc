// © 2014 the PBNF Authors under the MIT license. See AUTHORS for the list of authors.

/**
 * \file sync_solution_stream.cc
 *
 *
 *
 * \author eaburns
 * \date 2009-04-14
 */

#include <iostream>

using namespace std;

#include <stdlib.h>

#include <pthread.h>

#include "../state.h"
#include "sync_solution_stream.h"
#include "solution_stream.h"
#include "timer.h"

SyncSolutionStream::SyncSolutionStream(Timer *t, double g)
	: SerialSolutionStream(t, g)
{
	pthread_mutex_init(&mutex, NULL);
}

void SyncSolutionStream::see_solution(vector<State *> *path,
					 unsigned int gen,
					 unsigned int exp)
{
	pthread_mutex_lock(&mutex);
	SerialSolutionStream::see_solution(path, gen, exp);
	pthread_mutex_unlock(&mutex);
}

vector<State *> *SyncSolutionStream::get_best_path(void)
{
	vector<State*> *p;
	pthread_mutex_lock(&mutex);
	p = SerialSolutionStream::get_best_path();
	pthread_mutex_unlock(&mutex);
	return p;
}
