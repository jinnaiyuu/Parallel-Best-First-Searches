// © 2014 the PBNF Authors under the MIT license. See AUTHORS for the list of authors.

/**
 * \file serial_solution_stream.cc
 *
 *
 *
 * \author eaburns
 * \date 2009-04-20
 */
#include <iostream>

using namespace std;

#include <signal.h>
#include <stdlib.h>

#include "../state.h"
#include "solution_stream.h"
#include "serial_solution_stream.h"
#include "timer.h"

static void restore_sigs(sigset_t *oldmask)
{
        if (sigprocmask(SIG_SETMASK, oldmask, NULL) == -1)
                cerr << "restore_sigs: sigprocmask failed" << endl;
}

static void block_sigs(sigset_t *oldmask)
{
        sigset_t mask;
        sigfillset(&mask);
        if (sigprocmask(SIG_SETMASK, &mask, oldmask) == -1)
                cerr << "block_sigs: sigprocmask failed" << endl;
}

SerialSolutionStream::SerialSolutionStream(Timer *t, double g)
	: SolutionStream(t, g), best(NULL), lst(NULL),
		solution_found(false)
{
}

void SerialSolutionStream::see_solution(vector<State *> *path,
					 unsigned int gen,
					 unsigned int exp)
{
	sigset_t mask;
	double time = timer->get_lap_time();

	block_sigs(&mask);

	solution_found = true;

	if (!better(path, best))
		return;

	Solution *t = new Solution(path, gen, exp, time);
	best = t;

	if (within_gran(path, lst)) {
		t->next = lst;
		lst = t;
	}

	restore_sigs(&mask);
}

vector<State *> *SerialSolutionStream::get_best_path(void)
{
	if (!best)
		return NULL;

	return best->path;
}

/**
 * Not meant to be called atomically.
 */
void SerialSolutionStream::output(ostream &o)
{
	if (!best) {
		if (solution_found) {
			cerr << "Solution found, but best is NULL"
	        	     << endl;
			abort();
		}
		return;
	}

	if (best->next == NULL && lst != best)
		best->next = lst;

	SolutionStream::do_output(o, best);
}
