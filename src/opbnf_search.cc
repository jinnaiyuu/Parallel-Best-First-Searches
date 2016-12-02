// © 2014 the PBNF Authors under the MIT license. See AUTHORS for the list of authors.

/**
 * \file wpbnf_search.cc
 *
 *
 *
 * \author Sofia Lemons
 * \date 2009-04-18
 */

#include <assert.h>
#include <math.h>

#include <limits>
#include <vector>
#include <algorithm>

#include "opbnf_search.h"
#include "search.h"
#include "state.h"
#include "util/timer.h"
#include "util/cumulative_ave.h"

using namespace std;
using namespace OPBNF;

#define MIN_M 1
#define MAX_INT std::numeric_limits<int>::max()

OPBNFSearch::PBNFThread::PBNFThread(NBlockGraph *graph, OPBNFSearch *search)
	: graph(graph), search(search), set_hot(false) {
	next_best = 0.0;
}


OPBNFSearch::PBNFThread::~PBNFThread(void) {}


/**
 * Run the search thread.
 */
void OPBNFSearch::PBNFThread::run(void)
{
	vector<State *> *path;
	NBlock *n = NULL;

	do {
		n = graph->next_nblock(n, !set_hot, search->bound.read());

		if ((search->b * graph->get_f_min()) / fp_one > search->bound.read()) {
/*
			cout << "Exiting because we have hit the bound: ("
			     << search->b << " * " << graph->get_f_min()
			     << ") / " << fp_one << " > "
			     << search->bound.read() << endl;
*/
			break;
		}

		set_hot = false;
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
vector<State *> *OPBNFSearch::PBNFThread::search_nblock(NBlock *n)
{
	vector<State *> *path = NULL;
//	ClosedList *closed = &n->closed;

	while (!search->done && !n->empty() && !should_switch(n)) {
		// If the best f value in this nblock is bad, prune everything.
		if ((search->b * n->get_best_f()) / fp_one >= search->bound.read()) {
			n->prune();
			break;
		}

		State *s;
		if(n->get_best_fp() < search->bound.read())
			s = n->take_fp();
		else
			s = n->take_f();

		// If the individual f value is bad, prune the single node.
		if ((search->b * s->get_f()) / fp_one >= search->bound.read())
			continue;

 		if (s->is_goal()) {
			path = s->get_path();
			break;
		}

		expansions += 1;

		vector<State *> *children = search->expand(s);
		vector<State *>::iterator iter;

 		for (iter = children->begin(); iter != children->end(); iter++) {
			State *c = *iter;
			if ((search->b * c->get_f()) / fp_one >= search->bound.read()) {
				delete c;
				continue;
			}
			unsigned int blocknum = search->project->project(c);
			NBlock *cblock = graph->get_nblock(blocknum);
			ClosedList *next_closed = &cblock->closed;
			State *dup = next_closed->lookup(c);
			if (dup) {
				if (dup->get_g() > c->get_g()) {
					dup->update(c->get_parent(),
						    c->get_c(),
						    c->get_g());
					if (dup->is_open())
						cblock->see_update(dup);
					else
						cblock->add(dup);
				}


				delete c;
			} else {
				next_closed->add(c);
				if (c->is_goal()) {
					path = c->get_path();
					delete children;
					return path;
				}
				cblock->add(c);
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
bool OPBNFSearch::PBNFThread::should_switch(NBlock *n)
{
	bool ret;

	if (expansions < search->min_expansions)
		return false;

	expansions = 0;

	fp_type bound = search->bound.read();

	NBlock *best_scope = graph->best_in_scope(n, bound);
	NBlock *free_fp = graph->next_nblock_fp_peek();
	NBlock *free_f = graph->next_nblock_f_peek();
	bool free_exist = free_fp != NULL;
	bool free = free_exist && (NBlock::better(free_fp, n, bound)
				   || NBlock::better(free_f, n, bound));
	if (best_scope) {
		bool scope = NBlock::better(best_scope, n, bound);
		ret = free || scope;
		if (!ret)
			graph->wont_release(n);
		else if (scope
			 && (!free_exist
			     || (NBlock::better(best_scope, free_fp, bound)
				 && NBlock::better(best_scope, free_f, bound)))) {
			graph->set_hot(best_scope, search->bound.read());
			set_hot = true;
		}
	} else {
		ret = free;
	}

	return ret;
}


/************************************************************/
/************************************************************/
/************************************************************/


OPBNFSearch::OPBNFSearch(unsigned int n_threads,
			 unsigned int min_e)
	: n_threads(n_threads),
	  project(NULL),
	  path(NULL),
	  bound(fp_infinity),
	  done(false),
	  graph(NULL),
	  min_expansions(min_e)
{
	pthread_mutex_init(&path_mutex, NULL);
}


OPBNFSearch::~OPBNFSearch(void)
{
	if (graph)
		delete graph;
}


vector<State *> *OPBNFSearch::search(Timer *t, State *initial)
{
	project = initial->get_domain()->get_projection();
	Heuristic *h = initial->get_domain()->get_heuristic();

	double b_float = (double) h->get_weight() / (double) fp_one;
	h->set_weight(((b_float - 1.0) * 2.0) + 1.0);
	b = b_float * fp_one;

#if !defined(NDEBUG)
	cout << "b_float: " << b_float << endl;
	cout << "bound: " << b << endl;
	cout << "weight: " << h->get_weight() << endl;
#endif	// !NDEBUG

	vector<PBNFThread *> threads;
	vector<PBNFThread *>::iterator iter;


	graph_timer.start();
	graph = new NBlockGraph(project, initial);
	graph_timer.stop();

	for (unsigned int i = 0; i < n_threads; i += 1) {
		PBNFThread *t = new PBNFThread(graph, this);
		threads.push_back(t);
		t->start();
	}

	for (iter = threads.begin(); iter != threads.end(); iter++) {
		(*iter)->join();
		delete *iter;
	}
	return path;
}


/**
 * Set an incumbant solution.
 */
void OPBNFSearch::set_path(vector<State *> *p)
{
	pthread_mutex_lock(&path_mutex);
	assert(p->at(0)->get_g() == p->at(0)->get_f());
	if (p && bound.read() > p->at(0)->get_g()) {
		this->path = p;
		bound.set(p->at(0)->get_g());

		if ((b * graph->get_f_min()) / fp_one > p->at(0)->get_g()){
			done = true;
		}
	}
	pthread_mutex_unlock(&path_mutex);
}

void OPBNFSearch::output_stats(void)
{
	cout << "nblock-graph-creation-time: " << graph_timer.get_wall_time() << endl;
	cout << "total-nblocks: " << project->get_num_nblocks() << endl;
	cout << "created-nblocks: " << graph->get_ncreated_nblocks() << endl;
}
