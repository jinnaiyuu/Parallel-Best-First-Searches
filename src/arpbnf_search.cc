/**
 * \file arpbnf_search.cc
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

#include "arpbnf_search.h"
#include "search.h"
#include "state.h"
#include "util/timer.h"
#include "util/sync_solution_stream.h"

using namespace std;
using namespace ARPBNF;

ARPBNFSearch::ARPBNFThread::ARPBNFThread(NBlockGraph *graph, ARPBNFSearch *search)
	: graph(graph), search(search), set_hot(false)
{
}


ARPBNFSearch::ARPBNFThread::~ARPBNFThread(void) {}

/**
 * Run the search thread.
 */
void ARPBNFSearch::ARPBNFThread::run(void)
{
	vector<State *> *path;
	NBlock *n = NULL;

	do {
		n = graph->next_nblock(n, !set_hot);

		set_hot = false;
		if (n) {
			expansions = 0;
			path = search_nblock(n);

			if (path)
				search->set_path(path);
		}
	} while (n);

	graph->set_done();
}

/**
 * Search a single NBlock.
 */
vector<State *> *ARPBNFSearch::ARPBNFThread::search_nblock(NBlock *n)
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

		vector<State *> *children = search->expand(s);
		vector<State *>::iterator iter;

 		for (iter = children->begin(); iter != children->end(); iter++) {
			State *ch = *iter;
			// First check if we can prune!
			if (ch->get_f() >= search->bound.read()) {
				delete ch;
				continue;
			}
			vector<State *> *path = process_child(ch);
			if (path) {
				delete children;
				return path;
			}
		}
		delete children;
	}

	return path;
}

/**
 * Process a child state.
 *
 * \return A path if the child was a goal state... maybe we can prune
 *         more on this.
 */
vector<State *> *ARPBNFSearch::ARPBNFThread::process_child(State *ch)
{
	unsigned int block = search->project->project(ch);
	PQOpenList<State::PQOpsFPrime> *copen = &graph->get_nblock(block)->open;
	ClosedList *cclosed = &graph->get_nblock(block)->closed;
	State *dup = cclosed->lookup(ch);

	if (dup) {
		if (dup->get_g() > ch->get_g()) {
			dup->update(ch->get_parent(), ch->get_g());
			if (dup->is_open())
				copen->see_update(dup);
			else
				copen->add(dup);
		}
		delete ch;
	} else {
		cclosed->add(ch);
		if (ch->is_goal())
			return ch->get_path();
		copen->add(ch);
	}

	return NULL;
}


/**
 * Test the graph to see if we should switch to a new NBlock.
 * \param n The current NBlock.
 *
 * \note We should make this more complex... we should also check our
 *       successor NBlocks.
 */
bool ARPBNFSearch::ARPBNFThread::should_switch(NBlock *n)
{
	bool ret;

	if (expansions < search->min_expansions)
		return false;

	expansions = 0;

	fp_type free = graph->best_val();
	fp_type cur = n->open.get_best_val();
	NBlock *best_scope = graph->best_in_scope(n);

	if (best_scope) {
		fp_type scope = best_scope->open.get_best_val();

		ret = free < cur || scope < cur;
		if (!ret) {
			graph->wont_release(n);
		} else if (scope < free) {
			graph->set_hot(best_scope);
			set_hot = true;
		}
	} else {
		ret = free < cur;
	}

	return ret;
}


/*****************************************************************************/
/*****************************************************************************/


ARPBNFSearch::ARPBNFSearch(unsigned int n_threads,
			   unsigned int min_e,
			   vector<double> *w)
	: n_threads(n_threads),
	  project(NULL),
	  bound(fp_infinity),
	  graph(NULL),
	  min_expansions(min_e),
	  weights(w),
	  next_weight(1)
{
}


ARPBNFSearch::~ARPBNFSearch(void)
{
	if (graph)
		delete graph;
}


vector<State *> *ARPBNFSearch::search(Timer *timer, State *initial)
{
	solutions = new SyncSolutionStream(timer, 0.0001);
	project = initial->get_domain()->get_projection();

	vector<ARPBNFSearch::ARPBNFThread*> threads;
	vector<ARPBNFSearch::ARPBNFThread*>::iterator iter;

	graph = new NBlockGraph(project, initial);

	for (unsigned int i = 0; i < n_threads; i += 1) {
		ARPBNFThread *t = new ARPBNFThread(graph, this);
		threads.push_back(t);
		t->start();
	}

	for (iter = threads.begin(); iter != threads.end(); iter++) {
		(*iter)->join();
		delete *iter;
	}

	return solutions->get_best_path();
}


/**
 * Set an incumbant solution.
 */
void ARPBNFSearch::set_path(vector<State *> *p)
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
void ARPBNFSearch::output_stats(void)
{
	cout << "total-nblocks: " << project->get_num_nblocks() << endl;
	cout << "created-nblocks: " << graph->get_ncreated_nblocks() << endl;

	if (solutions)
		solutions->output(cout);
}