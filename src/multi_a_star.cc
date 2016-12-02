// © 2014 the PBNF Authors under the MIT license. See AUTHORS for the list of authors.

/**
 * \file multi_a_star.cc
 *
 *
 *
 * \author Ethan Burns
 * \date 2008-11-07
 */

#include "util/thread.h"
#include "util/timer.h"
#include "state.h"
#include "astar.h"
#include "multi_a_star.h"


MultiAStar::MultiAStar(unsigned int n_threads)
	: n_threads(n_threads) {}

MultiAStar::~MultiAStar(void) {}

class MultiAStarThread : public Thread {
public:
	MultiAStarThread(Timer *t, State *init)
		: timer(t), init(init), path(NULL) {}

	vector<State *> *get_path(void) {
		return path;
	}

	unsigned int get_expanded(void) {
		return search.get_expanded();
	}

	unsigned int get_generated(void) {
		return search.get_generated();
	}

	void run(void) {
		path = search.search(timer, init->clone());
	}

private:
	AStar search;
	Timer *timer;
	State *init;
	vector<State *> *path;
};

vector<State *> *MultiAStar::search(Timer *t, State *init)
{
	vector<MultiAStarThread *> threads;
	vector<State *> *path = NULL;

	for (unsigned int i = 0; i < n_threads; i += 1)
		threads.push_back(new MultiAStarThread(t, init));

	for(vector<MultiAStarThread *>::iterator i = threads.begin();
	    i != threads.end(); i++)
		(*i)->start();

	for(vector<MultiAStarThread *>::iterator i = threads.begin();
	    i != threads.end(); i++) {
		(*i)->join();
		if (!path)
			path = (*i)->get_path();
		else
			delete (*i)->get_path();
		set_expanded(get_expanded() + (*i)->get_expanded());
		set_generated(get_generated() + (*i)->get_generated());
	}

	return path;
}
