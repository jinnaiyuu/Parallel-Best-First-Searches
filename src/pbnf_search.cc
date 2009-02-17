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

#include <limits>
#include <vector>
#include <algorithm>

#include "pbnf_search.h"
#include "search.h"
#include "state.h"
#include "util/timer.h"
#include "util/cumulative_ave.h"

using namespace std;
using namespace PBNF;

#define MIN_M 1
#define MAX_INT std::numeric_limits<int>::max()

AtomicInt PBNFSearch::min_expansions(MIN_M);

PBNFSearch::PBNFThread::PBNFThread(NBlockGraph *graph, PBNFSearch *search)
	: graph(graph), search(search), set_hot(false) {
	next_best = 0.0;
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
		n = graph->next_nblock(n, !set_hot, false);

		if (n && search->dynamic_m){
			next_best = graph->best_f();
			//if (next_best == n->open.get_best_f()){
			//	next_best = 0.0;
			//}
		}

		set_hot = false;
		if (n) {
			expansions = 0;
			exp_this_block = 0;
			path = search_nblock(n);

			if (path)
				search->set_path(path);
			ave_exp_per_nblock.add_val(exp_this_block);
		}
	} while (n);

	graph->set_done();
}

/**
 * Get the average number of expansions per-nblock.
 */
fp_type PBNFSearch::PBNFThread::get_ave_exp_per_nblock(void)
{
	return ave_exp_per_nblock.read();
}

/**
 * Get the average size of open lists.
 */
fp_type PBNFSearch::PBNFThread::get_ave_open_size(void)
{
	return ave_open_size.read();
}

/**
 * Search a single NBlock.
 */
vector<State *> *PBNFSearch::PBNFThread::search_nblock(NBlock *n)
{
	vector<State *> *path = NULL;
	OpenList *open = &n->open;
//	ClosedList *closed = &n->closed;

	while (!open->empty() && !should_switch(n)) {
		State *s = open->take();
		ave_open_size.add_val(open->size());

		if (s->get_f() >= search->bound.read())
			continue;

		if (s->is_goal()) {
			path = s->get_path();
			break;
		}

		expansions += 1;
		exp_this_block += 1;

		vector<State *> *children = search->expand(s);
		vector<State *>::iterator iter;

 		for (iter = children->begin(); iter != children->end(); iter++) {
			if ((*iter)->get_f() >= search->bound.read()) {
				delete *iter;
				continue;
			}
			unsigned int block = search->project->project(*iter);
			PQOpenList<State::CompareOnFPrime> *next_open = &graph->get_nblock(block)->open;
			ClosedList *next_closed = &graph->get_nblock(block)->closed;
			State *dup = next_closed->lookup(*iter);
			if (dup) {
				if (dup->get_g() > (*iter)->get_g()) {
					dup->update((*iter)->get_parent(),
						    (*iter)->get_g());
					if (dup->is_open())
						next_open->resort(dup);
					else
						next_open->add(dup);
					ave_open_size.add_val(open->size());
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
				ave_open_size.add_val(open->size());
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

	if (next_best == 0.0 || graph->best_f() != 0.0){
		if (expansions < search->min_expansions.read())
			return false;
	}
	else{
		return n->open.get_best_f() > next_best;
	}

	expansions = 0;

	fp_type free = graph->next_nblock_f_value();
	fp_type cur = n->open.peek()->get_f();

	if (search->detect_livelocks) {
		NBlock *best_scope = graph->best_in_scope(n);
		if (best_scope) {
			fp_type scope = best_scope->open.get_best_f();

			ret = free < cur || scope < cur;
			if (!ret)
				graph->wont_release(n, false);
			else if (scope < free) {
				graph->set_hot(best_scope, false);
				set_hot = true;
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


PBNFSearch::PBNFSearch(unsigned int n_threads,
		       unsigned int min_e,
		       bool detect_livelocks)
	: n_threads(n_threads),
	  project(NULL),
	  path(NULL),
	  bound(fp_infinity),
	  detect_livelocks(detect_livelocks),
	  graph(NULL)

{
	pthread_mutex_init(&path_mutex, NULL);
	if (min_e == 0){
		dynamic_m = true;
		PBNFSearch::min_expansions = AtomicInt(MIN_M);
	}
	else{
		dynamic_m = false;
		PBNFSearch::min_expansions = AtomicInt(min_e);
	}
}


PBNFSearch::~PBNFSearch(void)
{
	if (graph)
		delete graph;
}


vector<State *> *PBNFSearch::search(State *initial)
{
	project = initial->get_domain()->get_projection();

	vector<PBNFThread *> threads;
	vector<PBNFThread *>::iterator iter;
	fp_type sum = 0.0;
	unsigned int num = 0;
	fp_type osum = 0.0;
	unsigned int onum = 0;
	Timer t;

	t.start();
	graph = new NBlockGraph(project, initial);
	t.stop();

	for (unsigned int i = 0; i < n_threads; i += 1) {
		PBNFThread *t = new PBNFThread(graph, this);
		threads.push_back(t);
		t->start();
	}
	
	for (iter = threads.begin(); iter != threads.end(); iter++) {
		(*iter)->join();

		fp_type ave = (*iter)->get_ave_exp_per_nblock();
		if (ave != 0) {
			sum += ave;
			num += 1;
		}
		fp_type oave = (*iter)->get_ave_open_size();
		if (oave != 0) {
			osum += oave;
			onum += 1;
		}

		delete *iter;
	}
	if (num == 0)
		cout << "expansions-per-nblock: -1" << endl;
	else
		cout << "expansions-per-nblock: " << sum / num << endl;
	if (onum == 0)
		cout << "avg-open-list-size: -1" << endl;
	else
		cout << "avg-open-list-size: " << osum / onum << endl;

	cout << "nblock-graph-creation-time: " << t.get_wall_time() << endl;

	return path;
}


/**
 * Set an incumbant solution.
 */
void PBNFSearch::set_path(vector<State *> *p)
{
	pthread_mutex_lock(&path_mutex);
	assert(p->at(0)->get_g() == p->at(0)->get_f());
	if (p && bound.read() > p->at(0)->get_g()) {
		this->path = p;
		bound.set(p->at(0)->get_g());
	}
	pthread_mutex_unlock(&path_mutex);
}

void PBNFSearch::inc_m()
{
        unsigned int old = PBNFSearch::min_expansions.read();
	unsigned int o, n;
	do { o = old; n = min((unsigned int)(o * 2), (unsigned int)((MAX_INT/2)-1)); old = PBNFSearch::min_expansions.cmp_and_swap(o, n);
	} while (old != o);
}

void PBNFSearch::dec_m()
{
        unsigned int old = PBNFSearch::min_expansions.read();
	unsigned int o, n;
	do { o = old; n = max((unsigned int)(o*.8), (unsigned int)MIN_M); old = PBNFSearch::min_expansions.cmp_and_swap(o, n);
	} while (old != o);
}
