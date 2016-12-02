// © 2014 the PBNF Authors under the MIT license. See AUTHORS for the list of authors.

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
	next:
		if (graph->is_done())
			break;
		n = graph->next_nblock(n, !set_hot, search->final_weight);
		set_hot = false;
		if (n) {
			expansions = 0;
			path = search_nblock(n);

			if (path) {
				if (search->set_path(path) && !search->final_weight) {
					graph->call_for_resort(n, search->final_weight, search);
					n = NULL;
					goto next;
				}
			}
		} else if (!search->final_weight) {
			graph->call_for_resort(NULL, search->final_weight, search);
#if !defined(NDEBUG) && 0
			cout << "No solution found at weight "
			     << search->weights->at(search->next_weight - 1)
			     << endl;
#endif	// !NDEBUG
			goto next;
		} else {
#if !defined(NDEBUG) && 0
			cout << "Done" << endl;
#endif	// !NDEBUG
		}
	} while (!graph->is_done());
}

/**
 * Search a single NBlock.
 */
vector<State *> *ARPBNFSearch::ARPBNFThread::search_nblock(NBlock *n)
{
	vector<State *> *path = NULL;
	OpenList *open = &n->open;

	while (!open->empty() && !should_switch(n) && !graph->needs_resort()) {

		if (n->open.peek()->get_f_prime() > search->bound.read())
			return NULL;

		State *s = open->take();

#if !defined(NDEBUG)
		State *dup = n->closed.lookup(s);
		assert (!dup || dup == s);
#endif	// !NDEBUG

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
	NBlock *cblock = graph->get_nblock(block);
	PQOpenList<State::PQOpsFPrime> *copen = &cblock->open;
	ClosedList *cclosed = &cblock->closed;
	InconsList *cincons = &cblock->incons;
	State *dup = cclosed->lookup(ch);

	if (dup) {
		if (dup->get_g() > ch->get_g()) {
			dup->update(ch->get_parent(), ch->get_c(), ch->get_g());
			assert(!dup->is_incons() || !dup->is_open());
			if (dup->is_open()) {
				copen->see_update(dup);
			} else {
				// Since we aren't searching in
				// strict-f' order, when the weight is
				// 1.0 we must re-open these
				// inconsistent states.
				if (search->final_weight)
					copen->add(dup);
				else if (!dup->is_incons())
					cincons->add(dup);
#if !defined(NDEBUG)
				else
					assert(cincons->lookup(dup) == dup);
#endif // !NDEBUG
			}
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
	  next_weight(1),
	  domain(NULL),
	  final_weight(false)
{
	pthread_mutex_init(&wmutex, NULL);
}


ARPBNFSearch::~ARPBNFSearch(void)
{
	if (graph)
		delete graph;
}


vector<State *> *ARPBNFSearch::search(Timer *timer, State *initial)
{
	solutions = new SyncSolutionStream(timer, 0.0001);
	domain = initial->get_domain();
	project = domain->get_projection();

	vector<ARPBNFSearch::ARPBNFThread*> threads;
	vector<ARPBNFSearch::ARPBNFThread*>::iterator iter;

	graph = new NBlockGraph(project, initial, &bound);

	if (domain->get_heuristic()->get_weight() == fp_one)
		final_weight = true;
	final_sol_weight = domain->get_heuristic()->get_weight();

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

void ARPBNFSearch::move_to_next_weight(void)
{
	if (next_weight < weights->size())
		final_sol_weight = weights->at(next_weight);

	if (weights->at(next_weight - 1) != 1.0) {
		double nw = 1.0;
		if (next_weight < weights->size()) {
			nw = weights->at(next_weight);
			next_weight += 1;
		} else {
			cerr << "Final weight is not 1.0, using 1.0" << endl;
		}

		domain->get_heuristic()->set_weight(nw);
		if (nw == 1.0 || next_weight == weights->size())
			final_weight = true;
	}
}

/**
 * Set an incumbant solution.
 *
 * If this function returns true than the calling thread must call for
 * a resort.
 */
bool ARPBNFSearch::set_path(vector<State *> *p)
{
	assert(solutions);

	pthread_mutex_lock(&wmutex);

	if (bound.read() <= p->at(0)->get_g()) {
		pthread_mutex_unlock(&wmutex);
		return false;
	}

	solutions->see_solution(p, get_generated(), get_expanded());
	bound.set(p->at(0)->get_g());
	graph->new_bound();

#if !defined(NDEBUG) && 0
	cout << "Solution of cost " << p->at(0)->get_g() / fp_one
	     << " found at weight " << weights->at(next_weight - 1)
	     << endl;
#endif	// !NDEBUG

	pthread_mutex_unlock(&wmutex);

	return true;
}

/**
 * Output extra "key: value" pairs.
 * keys should not have spaces in their names!
 */
void ARPBNFSearch::output_stats(void)
{
	cout << "total-nblocks: " << project->get_num_nblocks() << endl;
	cout << "created-nblocks: " << graph->get_ncreated_nblocks() << endl;
	cout << "final-sol-weight: " << final_sol_weight << endl;

	if (solutions)
		solutions->output(cout);
}
