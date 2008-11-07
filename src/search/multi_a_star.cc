/* -*- mode:linux -*- */
/**
 * \file multi_a_star.cc
 *
 *
 *
 * \author Ethan Burns
 * \date 2008-11-07
 */

#include "util/thread.h"
#include "state.h"
#include "a_star.h"
#include "multi_a_star.h"


MultiAStar::MultiAStar(unsigned int n_threads)
	: n_threads(n_threads) {}

MultiAStar::~MultiAStar(void) {}

class MultiAStarThread : public Thread {
public:
	MultiAStarThread(const State *init)
		: init(init) {}

	vector<const State *> *get_path(void) {
		return path;
	}

	unsigned int get_expanded(void) {
		return search.get_expanded();
	}

	unsigned int get_generated(void) {
		return search.get_generated();
	}

	void run(void) {
		path = search.search(init->clone());
	}

private:
	AStar search;
	const State *init;
	vector<const State *> *path;
};

vector<const State *> *MultiAStar::search(const State *init)
{
	vector<MultiAStarThread *> threads;
	vector<const State *> *path = NULL;

	for (unsigned int i = 0; i < n_threads; i += 1)
		threads.push_back(new MultiAStarThread(init));

	for(vector<MultiAStarThread *>::iterator i = threads.begin();
	    i != threads.end(); i++)
		(*i)->start();

	for(vector<MultiAStarThread *>::iterator i = threads.begin();
	    i != threads.end(); i++) {
		if (!path)
			path = (*i)->get_path();
		else
			delete (*i)->get_path();
		set_expanded(get_expanded() + (*i)->get_expanded());
		set_generated(get_generated() + (*i)->get_generated());
		(*i)->join();
	}

	return path;
}