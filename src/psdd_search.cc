// © 2014 the PBNF Authors under the MIT license. See AUTHORS for the list of authors.

/**
 * \file psdd.cc
 *
 *
 *
 * \author Ethan Burns
 * \date 2008-10-24
 */

#include <pthread.h>

#include <limits>
#include <iostream>
#include <vector>

#include "psdd_search.h"
#include "psdd/nblock_graph.h"
#include "psdd/nblock.h"
#include "util/thread.h"
#include "util/timer.h"
#include "projection.h"
#include "open_list.h"
#include "closed_list.h"

using namespace std;
using namespace PSDD;

/**
 * Create a new PSDD Search thread.
 */
PSDDSearch::PSDDThread::PSDDThread(NBlockGraph *graph, PSDDSearch *search)
	: graph(graph), search(search), lowest_out_of_bounds(fp_infinity) {}


PSDDSearch::PSDDThread::~PSDDThread() {}


/**
 * The thread work method for a PSDD search.
 */
void PSDDSearch::PSDDThread::run(void)
{
	vector<State *> *path;
	NBlock *n = NULL;

	do {
		n = graph->next_nblock(n);
		if (!n)		// no solution
			break;

		exp_this_block = 0;

		path = search_nblock(n);

		if (path) {
			search->set_path(path);
			graph->set_path_found();
		}

		ave_exp_per_nblock.add_val(exp_this_block);

	} while(!search->path_found());
}

fp_type PSDDSearch::PSDDThread::get_ave_exp_per_nblock(void)
{
	return ave_exp_per_nblock.read();
}

/**
 * Search a single NBlock.
 * \param n The NBlock.
 * \return NULL, or a path to a goal.
 */
vector<State *> *PSDDSearch::PSDDThread::search_nblock(NBlock *n)
{
	vector<State *> *path = NULL;
	OpenList *cur_open = &n->open[graph->get_cur_layer()];

	while (!cur_open->empty()) {
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

			if ((*iter)->get_f() > search->bound.read()) {
				if ((*iter)->get_f() < lowest_out_of_bounds)
					lowest_out_of_bounds = (*iter)->get_f();

				delete *iter;
				continue;
			}

			unsigned int block = search->project->project(*iter);
			NBlock *b = graph->get_nblock(block);
			OpenList *next_open = &b->open[graph->get_next_layer()];
			ClosedList *next_closed = &graph->get_nblock(block)->closed;
			State *dup = next_closed->lookup(*iter);
			if (dup) {
				if (dup->get_g() > (*iter)->get_g()) {
					dup->update((*iter)->get_parent(),
						    (*iter)->get_c(),
						    (*iter)->get_g());
					if (!dup->is_open())
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

fp_type PSDDSearch::PSDDThread::get_lowest_out_of_bounds(void)
{
	return lowest_out_of_bounds;
}


/**********************************************************************/


/**
 * Create a new Parallel Structured Duplicate Detection search.
 */
PSDDSearch::PSDDSearch(unsigned int n_threads)
	: bound(fp_infinity),
	  n_threads(n_threads),
	  project(NULL),
	  path(NULL),
	  graph(NULL),
	  lowest_out_of_bounds(fp_infinity),
	  print(true)
{
	pthread_mutex_init(&path_mutex, NULL);
}

/**
 * Create a new Parallel Structured Duplicate Detection search with a
 * given bound.
 */
PSDDSearch::PSDDSearch(unsigned int n_threads, fp_type bound)
	: bound(bound),
	  n_threads(n_threads),
	  project(NULL),
	  path(NULL),
	  graph(NULL),
	  lowest_out_of_bounds(fp_infinity)
{
	pthread_mutex_init(&path_mutex, NULL);
}


/**
 * Destructor.
 */
PSDDSearch::~PSDDSearch(void)
{
	if (graph)
		delete graph;
}


/**
 * Set the path to the goal.
 */
void PSDDSearch::set_path(vector<State *> *p)
{
	pthread_mutex_lock(&path_mutex);
	if (path) {
		vector<State *>::iterator iter;

		for (iter = p->begin(); iter != p->end(); iter++)
			delete *iter;

		delete p;
	} else {
		path = p;
	}
	pthread_mutex_unlock(&path_mutex);
}


/**
 * Test if there has been a path to the goal found yet.
 */
bool PSDDSearch::path_found(void) const
{
	return path != NULL;
}

/**
 * Perform the search.
 */
vector<State *> *PSDDSearch::search(Timer *timer, State *initial)
{
	project = initial->get_domain()->get_projection();

	vector<PSDDThread *> threads;
	vector<PSDDThread *>::iterator iter;
	fp_type sum = 0.0;
	unsigned int num = 0;
	Timer graph_timer;

	if (!graph) {
		graph_timer.start();
		graph = new NBlockGraph(project, initial);
		graph_timer.stop();
	} else {
		graph->reset(project, initial);
	}

	for (unsigned int i = 0; i < n_threads; i += 1) {
		PSDDThread *t = new PSDDThread(graph, this);
		threads.push_back(t);
		t->start();
	}

	for (iter = threads.begin(); iter != threads.end(); iter++) {
		(*iter)->join();
		if ((*iter)->get_lowest_out_of_bounds()
		    < lowest_out_of_bounds) {
			lowest_out_of_bounds =
				(*iter)->get_lowest_out_of_bounds();
		}

		fp_type ave = (*iter)->get_ave_exp_per_nblock();
		if (ave != 0.0) {
			sum += ave;
			num += 1;
		}

		delete *iter;
	}

/* This output should not happen here inside the search function */
//	if (num == 0 )
// 		cout << "expansions-per-nblock: -1" << endl;
// 	else
// 		cout << "expansions-per-nblock: " << sum / num << endl;

// 	cout << "nblock-graph-creation-time: " << t.get_wall_time() << endl;

// 	cout << "total-nblocks: " << project->get_num_nblocks() << endl;
// 	cout << "created-nblocks: " << graph->get_ncreated_nblocks() << endl;

	return path;
}

/**
 * Set the bound.
 */
void PSDDSearch::set_bound(fp_type b)
{
	this->bound.set(b);
}

fp_type PSDDSearch::get_lowest_out_of_bounds(void)
{
	return lowest_out_of_bounds;
}

void PSDDSearch::reset(void)
{
	lowest_out_of_bounds = fp_infinity;
	path = NULL;
}

void PSDDSearch::do_not_print(void)
{
	print = false;
}
