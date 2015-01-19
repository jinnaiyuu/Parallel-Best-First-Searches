// © 2014 the PBNF Authors under the MIT license. See AUTHORS for the list of authors.

/**
 * \file pbnf_search.cc
 *
 *
 *
 * \author Ethan Burns
 * \date 2008-10-29
 */

#include <assert.h>
#include <math.h>

#include <fstream>
#include <limits>
#include <vector>
#include <algorithm>

#include "pbnf_search.h"
#include "search.h"
#include "state.h"
#include "util/timer.h"
#include "util/fhist.h"
#include "util/cumulative_ave.h"
#include "util/sync_solution_stream.h"

using namespace std;
using namespace PBNF;

#define MIN_M 1
#define MAX_INT std::numeric_limits<int>::max()

PBNFSearch::PBNFThread::PBNFThread(NBlockGraph *graph, PBNFSearch *search)
	: graph(graph), search(search)
{
}


PBNFSearch::PBNFThread::~PBNFThread(void) {}


/**
 * Run the search thread.
 */
void PBNFSearch::PBNFThread::run(void)
{
	vector<State *> *path;
	NBlock *n = NULL;

	do {
		n = graph->next_nblock(n);

		if (n) {
			expansions = 0;
#if defined(INSTRUMENTED)
			exp_this_block = 0;
#endif	// INSTRUMENTED
			path = search_nblock(n);

			if (path)
				search->set_path(path);
#if defined(INSTRUMENTED)
			ave_exp_per_nblock.add_val(exp_this_block);
#endif	// INSTRUMENTED
		}
	} while (n);

	graph->set_done();
}

#if defined(INSTRUMENTED)
/**
 * Get the average number of expansions per-nblock.
 */
fp_type PBNFSearch::PBNFThread::get_ave_exp_per_nblock(void)
{
	return ave_exp_per_nblock.read();
}
#endif	// INSTRUMENTED

/**
 * Search a single NBlock.
 */
vector<State *> *PBNFSearch::PBNFThread::search_nblock(NBlock *n)
{
	vector<State *> *path = NULL;
	OpenList *open = &n->open;

	while (!open->empty() && !should_switch(n)) {
		State *s = open->take();

		if (s->get_f() >= search->bound.read())
			continue;

		if (s->is_goal()) {
			path = s->get_path();
			break;
		}

		expansions += 1;
#if defined(INSTRUMENTED)
		exp_this_block += 1;
#endif	// INSTRUMENTED

#if defined(COUNT_FS)
		fs.see_f(s->get_f());
#endif // COUNT_FS
		vector<State *> *children = search->expand(s);
		vector<State *>::iterator iter;

 		for (iter = children->begin(); iter != children->end(); iter++) {
			if ((*iter)->get_f() >= search->bound.read()) {
				delete *iter;
				continue;
			}
			unsigned int block = search->project->project(*iter);
			// TODO:
//			PQOpenList<State::PQOpsFPrime> *next_open =
			PQVectorOpenList *next_open =
			&graph->get_nblock(block)->open;

			ClosedList *next_closed =
				&graph->get_nblock(block)->closed;
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


/**
 * Test the graph to see if we should switch to a new NBlock.
 * \param n The current NBlock.
 *
 * \note We should make this more complex... we should also check our
 *       successor NBlocks.
 */
bool PBNFSearch::PBNFThread::should_switch(NBlock *n)
{
	bool ret;

	if (expansions < search->min_expansions)
		return false;

	expansions = 0;

	fp_type free = graph->best_val();
	fp_type cur = n->open.get_best_val();

	if (search->detect_livelocks) {
		NBlock *best_scope = graph->best_in_scope(n);
		if (best_scope) {
			fp_type scope = best_scope->open.get_best_val();

			ret = free < cur || scope < cur;
			if (!ret)
				graph->wont_release(n);
			else if (scope < free) {
				ret = graph->set_hot(best_scope);
			}
		} else {
			ret = free < cur;
		}
	} else {
		ret = free < cur;
	}

	return ret;
}


/************************************************************/
/************************************************************/
/************************************************************/

#if defined(COUNT_FS)
F_hist PBNFSearch::fs;
#endif // COUNT_FS

PBNFSearch::PBNFSearch(unsigned int n_threads,
		       unsigned int min_e,
		       bool detect_livelocks)
	: n_threads(n_threads),
	  project(NULL),
	  bound(fp_infinity),
	  detect_livelocks(detect_livelocks),
	  graph(NULL),
	  min_expansions(min_e)
{
	printf("vector\n");

#if defined(INSTRUMENTED)
	sum = 0.0;
	num = 0;
#endif	// INSTRUMENTED
}


PBNFSearch::~PBNFSearch(void)
{
	if (graph)
		delete graph;
}

vector<State *> *PBNFSearch::search(Timer *timer, State *initial)
{
	double weight;
	solutions = new SyncSolutionStream(timer, 0.0001);
	project = initial->get_domain()->get_projection();

	vector<PBNFThread *> threads;
	vector<PBNFThread *>::iterator iter;

	weight = initial->get_domain()->get_heuristic()->get_weight();
	graph = new NBlockGraph(project, initial, &bound, weight);

	for (unsigned int i = 0; i < n_threads; i += 1) {
		PBNFThread *t = new PBNFThread(graph, this);
		threads.push_back(t);
		t->start();
	}

	// Accumulate statistics
#if defined(INSTRUMENTED)
	sum = num = 0;
#endif	// INSTRUMENTED
	for (iter = threads.begin(); iter != threads.end(); iter++) {
		(*iter)->join();

#if defined(INSTRUMENTED)
		fp_type ave = (*iter)->get_ave_exp_per_nblock();
		if (ave != 0) {
			sum += ave;
			num += 1;
		}
#endif	// INSTRUMENTED
	}

#if defined(COUNT_FS)
	ofstream o;
	std::cout << "Outputting to pbnf-fs.dat" << std::endl;
	o.open("pbnf-fs.dat", std::ios_base::app);
	fs.output_above(o, bound.read());
	o.close();
#endif	// COUNT_FS

	return solutions->get_best_path();
}


/**
 * Set an incumbant solution.
 */
void PBNFSearch::set_path(vector<State *> *p)
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
 * Output extra "key: value" pairs.
 * keys should not have spaces in their names!
 */
void PBNFSearch::output_stats(void)
{
#if defined(INSTRUMENTED)
	if (num == 0)
		cout << "expansions-per-nblock: -1" << endl;
	else
		cout << "expansions-per-nblock: " << sum / num << endl;
#endif	// INSTRUMENTED

	cout << "total-nblocks: " << project->get_num_nblocks() << endl;
	cout << "created-nblocks: " << graph->get_ncreated_nblocks() << endl;

	if (graph)
		graph->print_stats(cout);

	if (solutions)
		solutions->output(cout);

#if defined(INSTRUMENTED)
	Mutex::print_pre_thread_stats(cout);
	cout << "average-time-acquiring-locks: "
	     << Mutex::get_total_lock_acquisition_time() / n_threads
	     << endl;
	cout << "max-time-acquiring-locks: "
	     << Mutex::get_max_lock_acquisition_time() << endl;
	cout << "average-time-waiting: "
	     << Mutex::get_total_cond_wait_time() / n_threads << endl;
	cout << "max-time-waiting: "
	     << Mutex::get_max_cond_wait_time() << endl;
#endif	// INSTRUMENTED
}
