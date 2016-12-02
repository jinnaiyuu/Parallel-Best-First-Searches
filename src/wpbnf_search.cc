// © 2014 the PBNF Authors under the MIT license. See AUTHORS for the list of authors.

/**
 * \file wpbnf_search.cc
 *
 *
 *
 * \author Ethan Burns
 * \date 2008-10-29
 */

#include <assert.h>
#include <math.h>

#include <limits>
#include <vector>
#include <algorithm>

#include "wpbnf_search.h"
#include "search.h"
#include "state.h"
#include "util/timer.h"
#include "util/cumulative_ave.h"
#include "util/sync_solution_stream.h"

using namespace std;
using namespace WPBNF;

#define MIN_M 1
#define MAX_INT std::numeric_limits<int>::max()

WPBNFSearch::PBNFThread::PBNFThread(NBlockGraph *graph, WPBNFSearch *search)
	: graph(graph), search(search)
{
}


WPBNFSearch::PBNFThread::~PBNFThread(void) {}


/**
 * Run the search thread.
 */
void WPBNFSearch::PBNFThread::run(void)
{
	vector<State *> *path;
	NBlock *n = NULL;

	do {
		n = graph->next_nblock(n);

		if (n) {
			expansions = 0;
			path = search_nblock(n);

			if (path)
				search->set_path(path);
		}
	} while (!search->done && n);

	search->done = true;
	graph->set_done();
}

/**
 * Search a single NBlock.
 */
vector<State *> *WPBNFSearch::PBNFThread::search_nblock(NBlock *n)
{
	vector<State *> *path = NULL;
	OpenList *open_fp = &n->open_fp;
//	ClosedList *closed = &n->closed;

	while (!search->done && !open_fp->empty() && !should_switch(n)) {
		State *s = open_fp->take();

		// If the best f' value in this nblock is out of
		// bounds, prune everything.
		if (s->get_f_prime() >= search->bound.read()) {
			open_fp->prune();
			break;
		}

		// If the individual f value is bad, prune the single node.
		if ((search->weight * s->get_f()) / fp_one >= search->bound.read())
			continue;

		if (s->is_goal()) {
			path = s->get_path();
			break;
		}

		expansions += 1;

		vector<State *> *children = search->expand(s);
		vector<State *>::iterator iter;

 		for (iter = children->begin(); iter != children->end(); iter++) {
			if ((search->weight * (*iter)->get_f()) / fp_one >= search->bound.read()) {
				delete *iter;
				continue;
			}
			State *ch = *iter;
			unsigned int block = search->project->project(ch);
			PQOpenList<State::PQOpsFPrime> *next_open_fp = &graph->get_nblock(block)->open_fp;
			ClosedList *next_closed = &graph->get_nblock(block)->closed;
			State *dup = next_closed->lookup(ch);
			if (dup) {
				if (dup->get_g() > ch->get_g()) {
					fp_type old_g = dup->get_g();
					dup->update(ch->get_parent(), ch->get_c(), ch->get_g());
					if (dup->is_open()) {
						next_open_fp->see_update(dup);
					} else if (!search->dd || (old_g > s->get_g() + ((search->weight * ch->get_c()) / fp_one))) {
						// this is Wheeler's duplicate dropping test.
						next_open_fp->add(dup);
					}
				}
				delete ch;
			} else {
				next_closed->add(ch);
				if (ch->is_goal()) {
					path = ch->get_path();
					delete children;
					return path;
				}
				next_open_fp->add(ch);
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
bool WPBNFSearch::PBNFThread::should_switch(NBlock *n)
{
	bool ret;

	if (expansions < search->min_expansions)
		return false;

	expansions = 0;

	fp_type free = graph->best_free_val();
	fp_type cur = n->open_fp.get_best_val();

	NBlock *best_scope = graph->best_in_scope(n);
	if (best_scope) {
		fp_type scope = best_scope->open_fp.get_best_val();

		ret = free < cur || scope < cur;
		if (!ret)
			graph->wont_release(n);
		else if (scope < free) {
			ret = graph->set_hot(best_scope);
		}
	} else {
		ret = free < cur;
	}

	return ret;
}


/************************************************************/
/************************************************************/
/************************************************************/


WPBNFSearch::WPBNFSearch(unsigned int n_threads,
			 unsigned int min_e, bool dup_drop)
	: n_threads(n_threads),
	  project(NULL),
	  bound(fp_infinity),
	  done(false),
	  graph(NULL),
	  dd(dup_drop)

{
	min_expansions = min_e;

#if defined(INSTRUMENTED)
	sum = num = osum = onum = 0;
#endif	// INSTRUMENTED
}


WPBNFSearch::~WPBNFSearch(void)
{
	if (graph)
		delete graph;
}


vector<State *> *WPBNFSearch::search(Timer *t, State *initial)
{
	project = initial->get_domain()->get_projection();
	solutions = new SyncSolutionStream(t, 0.0001);

	vector<PBNFThread *> threads;
	vector<PBNFThread *>::iterator iter;
#if !defined(NDEBUG)
	Heuristic *h = initial->get_domain()->get_heuristic();

	cout << "weight: " << h->get_weight() << endl;
#endif	// !NDEBUG

	weight = initial->get_domain()->get_heuristic()->get_weight();
	graph = new NBlockGraph(project, initial, &bound, weight);

	for (unsigned int i = 0; i < n_threads; i += 1) {
		PBNFThread *t = new PBNFThread(graph, this);
		threads.push_back(t);
		t->start();
	}

	for (iter = threads.begin(); iter != threads.end(); iter++) {
		(*iter)->join();

		delete *iter;
	}

	return solutions->get_best_path();
}

void WPBNFSearch::output_stats(void)
{
#if defined(INSTRUMENTED)
	if (num == 0)
		cout << "expansions-per-nblock: -1" << endl;
	else
		cout << "expansions-per-nblock: " << sum / num << endl;
	if (onum == 0)
		cout << "avg-open-list-size: -1" << endl;
	else
		cout << "avg-open-list-size: " << osum / onum << endl;

	cout << "total-nblocks: " << project->get_num_nblocks() << endl;
	cout << "created-nblocks: " << graph->get_ncreated_nblocks() << endl;
#endif	// INSTRUMENTED

	if (solutions)
		solutions->output(cout);

}

/**
 * Set an incumbant solution.
 */
void WPBNFSearch::set_path(vector<State *> *p)
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

// 	pthread_mutex_lock(&path_mutex);
// 	assert(p->at(0)->get_g() == p->at(0)->get_f());
// 	if (p && bound.read() > p->at(0)->get_g()) {
// 		this->path = p;
// 		bound.set(p->at(0)->get_g());
// 	}
//	pthread_mutex_unlock(&path_mutex);
}
