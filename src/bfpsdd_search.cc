// © 2014 the PBNF Authors under the MIT license. See AUTHORS for the list of authors.

/**
 * \file bfpsdd_search.cc
 *
 *
 *
 * \author Ethan Burns
 * \date 2008-11-20
 */


#include <pthread.h>

#include <limits>
#include <iostream>
#include <vector>

#include "bfpsdd_search.h"
#include "bfpsdd/real_val_nblock_pq.h"
#include "bfpsdd/nblock_graph.h"
#include "bfpsdd/nblock.h"
#include "util/thread.h"
#include "util/timer.h"
#include "util/sync_solution_stream.h"
#include "projection.h"
#include "open_list.h"
#include "closed_list.h"

using namespace std;
using namespace BFPSDD;

/**
 * Create a new PSDD Search thread.
 */
BFPSDDSearch::BFPSDDThread::BFPSDDThread(NBlockGraph<BFPSDD::RealValNBlockPQ<State::PQOpsFPrime>,State::PQOpsFPrime> *graph,
					 BFPSDDSearch *search)
	: graph(graph), search(search) {}


BFPSDDSearch::BFPSDDThread::~BFPSDDThread() {}


/**
 * The thread work method for a PSDD search.
 */
void BFPSDDSearch::BFPSDDThread::run(void)
{
	vector<State *> *path;
	NBlock<State::PQOpsFPrime> *n = NULL;

	do {
		n = graph->next_nblock(n);

		if (n) {
			exp_this_block = 0;
			path = search_nblock(n);

			if (path)
				search->set_path(path);
			ave_exp_per_nblock.add_val(exp_this_block);
		}

	} while(n);
}

fp_type BFPSDDSearch::BFPSDDThread::get_ave_exp_per_nblock(void)
{
	return ave_exp_per_nblock.read();
}

/**
 * Search a single NBlock.
 * \param n The NBlock.
 * \return NULL, or a path to a goal.
 */
vector<State *> *BFPSDDSearch::BFPSDDThread::search_nblock(NBlock<State::PQOpsFPrime> *n)
{
	vector<State *> *path = NULL;
	PQOpenList<State::PQOpsFPrime> *cur_open = &n->open;

	while (!cur_open->empty()) {
		if (exp_this_block > search->min_expansions
		    && cur_open->get_best_val() > graph->get_layer_value())
			break;

		if (cur_open->get_best_val() >= search->bound.read()) {
			cur_open->prune();
			break;
		}

		State *s = cur_open->take();

		if (s->is_goal()) {
			path = s->get_path();
			break;
		}

		exp_this_block += 1;

		vector<State *> *children = search->expand(s);
		vector<State *>::iterator iter;
		for (iter = children->begin();
		     iter != children->end();
		     iter++) {
			unsigned int block = search->project->project(*iter);
			NBlock<State::PQOpsFPrime> *b = graph->get_nblock(block);
			PQOpenList<State::PQOpsFPrime> *next_open = &b->open;
			ClosedList *next_closed = &graph->get_nblock(block)->closed;
			if ((*iter)->get_f() >= search->bound.read()) {
				delete *iter;
				continue;
			}
			State *dup = next_closed->lookup(*iter);
			if (dup) {
				if (dup->get_g() > (*iter)->get_g()) {
					dup->update((*iter)->get_parent(),
						    (*iter)->get_c(),
						    (*iter)->get_g());
					if (dup->is_open())
						next_open->see_update(dup);
					else
						next_open->add(dup);
				}
				delete *iter;
			} else {
				next_closed->add(*iter);
				if ((*iter)->is_goal()) {
					path = (*iter)->get_path();
					delete children;
					return path;
				}
				next_open->add(*iter);
			}
		}
		delete children;

	}

	return path;
}


/**********************************************************************/


/**
 * Create a new Parallel Structured Duplicate Detection search.
 */
BFPSDDSearch::BFPSDDSearch(unsigned int n_threads, fp_type mult, unsigned int min_expansions)
	: bound(fp_infinity),
	  n_threads(n_threads),
	  project(NULL),
	  path(NULL),
	  graph(NULL),
	  min_expansions(min_expansions),
	  multiplier(mult)
{
}

/**
 * Create a new Parallel Structured Duplicate Detection search with a
 * given bound.
 */
BFPSDDSearch::BFPSDDSearch(unsigned int n_threads, fp_type mult, unsigned int min_expansions, fp_type bound)
	: bound(bound),
	  n_threads(n_threads),
	  project(NULL),
	  path(NULL),
	  graph(NULL),
	  min_expansions(min_expansions),
	  multiplier(mult),
	  sum(0),
	  num(0)
{
}


/**
 * Destructor.
 */
BFPSDDSearch::~BFPSDDSearch(void)
{
	if (graph)
		delete graph;
}


/**
 * Set the path to the goal.
 */
void BFPSDDSearch::set_path(vector<State *> *p)
{
	fp_type b, oldb;

	assert(solutions);

	solutions->see_solution(p, get_generated(), get_expanded());
	b = solutions->get_best_path()->at(0)->get_g();

	// CAS in our new solution bound if it is still indeed better
	// than the previous bound.
	do {
		oldb = bound.read();
		if (oldb <= b)
			return;
	} while (bound.cmp_and_swap(oldb, b) != oldb);
}


/**
 * Test if there has been a path to the goal found yet.
 */
bool BFPSDDSearch::path_found(void) const
{
	return path != NULL;
}

/**
 * Perform the search.
 */
vector<State *> *BFPSDDSearch::search(Timer *timer, State *initial)
{
	solutions = new SyncSolutionStream(timer, 0.0001);
	project = initial->get_domain()->get_projection();

	vector<BFPSDDThread *> threads;
	vector<BFPSDDThread *>::iterator iter;

	graph_timer.start();
	graph = new NBlockGraph<RealValNBlockPQ<State::PQOpsFPrime>,
		State::PQOpsFPrime>(project,
				    n_threads,
				    multiplier,
				    initial);
	graph_timer.stop();

	for (unsigned int i = 0; i < n_threads; i += 1) {
		BFPSDDThread *t = new BFPSDDThread(graph, this);
		threads.push_back(t);
		t->start();
	}

	for (iter = threads.begin(); iter != threads.end(); iter++) {
		(*iter)->join();

		fp_type ave = (*iter)->get_ave_exp_per_nblock();
		if (ave != 0.0) {
			sum += ave;
			num += 1;
		}

		delete *iter;
	}

	return solutions->get_best_path();
}


void BFPSDDSearch::output_stats(void)
{
	if (num == 0)
		cout << "expansions-per-nblock: -1" << endl;
	else
		cout << "expansions-per-nblock: " << sum / num << endl;

	cout << "nblock-graph-creation-time: "
	     << graph_timer.get_wall_time() << endl;

	cout << "total-nblocks: " << project->get_num_nblocks() << endl;
	cout << "created-nblocks: " << graph->get_ncreated_nblocks() << endl;

	Mutex::print_pre_thread_stats(cout);

	Mutex::print_stats(cout);

#if defined(INSTRUMENTED)
	cout << "average-time-acquiring-locks: "
	     << Mutex::get_total_lock_acquisition_time() / n_threads
	     << endl;
	cout << "average-time-waiting: "
	     << Mutex::get_total_cond_wait_time() / n_threads << endl;
#endif	// INSTRUMENTED
	graph->print_stats(cout);
}

/**
 * Set the bound.
 */
void BFPSDDSearch::set_bound(fp_type b)
{
	this->bound.set(b);
}
