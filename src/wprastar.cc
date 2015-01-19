// © 2014 the PBNF Authors under the MIT license. See AUTHORS for the list of authors.

 /**
 * \file wprastar.cc
 *
 *
 *
 * \author Sofia Lemons
 * \date 2009-03-19
 */

#include <assert.h>
#include <math.h>
#include <errno.h>

#include <vector>
#include <limits>

#include "util/sync_solution_stream.h"
#include "wprastar.h"
#include "projection.h"
#include "search.h"
#include "state.h"

/*
 * Test if we can drop this dupe
 * og -- old g,
 * pg -- parent g,
 * c  -- cost
 *
 * assumes that there is a pointer 'p' to the wPRAStar search.
 */
#define CANT_DROP(og, pg, c) ((og) > (pg) + ((p->weight * (c)) / fp_one))

using namespace std;

wPRAStar::wPRAStarThread::wPRAStarThread(wPRAStar *p,
					 vector<wPRAStarThread *> *threads,
					 CompletionCounter* cc)
	: p(p),
	  threads(threads),
	  cc(cc),
	  q_empty(true)
{
	expansions = 0;
	out_qs.resize(threads->size(), NULL);
        completed = false;
}


wPRAStar::wPRAStarThread::~wPRAStarThread(void) {
}

vector<State*> *wPRAStar::wPRAStarThread::get_queue(void)
{
	return &q;
}

Mutex *wPRAStar::wPRAStarThread::get_mutex(void)
{
	return &mutex;
}

void wPRAStar::wPRAStarThread::post_send(void *t)
{
	wPRAStarThread *thr = (wPRAStarThread*) t;
	if (thr->completed) {
		thr->cc->uncomplete();
		thr->completed = false;
	}
	thr->q_empty = false;
}

bool wPRAStar::wPRAStarThread::flush_sends(void)
{
	unsigned int i;
	bool has_sends = false;

	expansions = 0;
	for (i = 0; i < threads->size(); i += 1) {
		if (!out_qs[i])
			continue;
		if (out_qs[i]) {
			out_qs[i]->try_flush();
			if (!out_qs[i]->is_empty())
				has_sends = true;
		}
	}

	return has_sends;
}


/**
 * Flush the queue
 */
void wPRAStar::wPRAStarThread::flush_receives(bool has_sends)
{
	// wait for either completion or more nodes to expand
	if (open.empty() || !p->async)
		mutex.lock();
	else if (!mutex.try_lock())
		return;

	if (q_empty && !has_sends && open.empty()) {
		completed = true;
		cc->complete();

		// busy wait
		mutex.unlock();
		while (q_empty && !cc->is_complete() && !p->is_done())
			;
		mutex.lock();

		// we are done, just return
		if (cc->is_complete()) {
			assert(q_empty);
			mutex.unlock();
			return;
		}
	}

	// got some stuff on the queue, lets do duplicate detection
	// and add stuff to the open list
	for (unsigned int i = 0; i < q.size(); i += 1) {
		State *c = q[i];
		State *dup = closed.lookup(c);
		if (dup){
			if (dup->get_g() > c->get_g()) {
				fp_type old_g = dup->get_g();
				fp_type parent_g = c->get_g() - c->get_c();
				dup->update(c->get_parent(),
					    c->get_c(),
					    c->get_g());
				if (dup->is_open())
					open.see_update(dup);
				else if (!p->dd || CANT_DROP(old_g,
							     parent_g,
							     c->get_c())) {
/* old_g > parent_g + ((p->weight * c->get_c()) / fp_one)*/
					// Wheeler's dup dropping
					open.add(dup);
				}
			}
			delete c;
		}
		else{
			open.add(c);
			closed.add(c);
		}
	}
	q.clear();
	q_empty = true;
	mutex.unlock();
}

void wPRAStar::wPRAStarThread::do_async_send(unsigned int dest_tid, State *c)
{
	if (!out_qs[dest_tid]) {
		Mutex *lk = threads->at(dest_tid)->get_mutex();
		vector<State*> *qu = threads->at(dest_tid)->get_queue();
		out_qs[dest_tid] =
			new MsgBuffer<State*>(lk, qu,
					      post_send,
					      threads->at(dest_tid));
	}

	out_qs[dest_tid]->try_send(c);
}

void wPRAStar::wPRAStarThread::do_sync_send(unsigned int dest_tid, State *c)
{
	wPRAStarThread *dest = threads->at(dest_tid);

	dest->get_mutex()->lock();
	dest->get_queue()->push_back(c);
	post_send(dest);
	dest->get_mutex()->unlock();
}

void wPRAStar::wPRAStarThread::send_state(State *c)
{
	unsigned long hash =
		p->use_abstraction
		? p->project->project(c)
//		: c->hash();
				: c->zbrhash();
	unsigned int dest_tid = threads->at(hash % p->n_threads)->get_id();
	bool self_add = dest_tid == this->get_id();

	assert (p->n_threads != 1 || self_add);

	if (self_add) {
		State *dup = closed.lookup(c);
		if (dup){
			if (dup->get_g() > c->get_g()) {
				fp_type old_g = dup->get_g();
				fp_type parent_g = c->get_g() - c->get_c();
				dup->update(c->get_parent(),
					    c->get_c(),
					    c->get_g());
				if (dup->is_open())
					open.see_update(dup);
				else if (!p->dd || CANT_DROP(old_g,
							     parent_g,
							     c->get_c()))
					open.add(dup);
			}
			delete c;
		}
		else{
			open.add(c);
			closed.add(c);
		}
		return;
	}

	// not a self add
	if (p->async)
		do_async_send(dest_tid, c);
	else
		do_sync_send(dest_tid, c);

}

State *wPRAStar::wPRAStarThread::take(void)
{
	bool has_sends = true;

	expansions += 1;
	if (p->max_exp == 0 || expansions > p->max_exp)
		has_sends = flush_sends();

	while (open.empty() || !q_empty) {
		if (has_sends)
			has_sends = flush_sends();
		flush_receives(has_sends);

		if (cc->is_complete()){
			p->set_done();
			return NULL;
		}
        }

	State *ret = NULL;
	if (!p->is_done())
		ret = open.take();

        return ret;
}

/**
 * Run the search thread.
 */
void wPRAStar::wPRAStarThread::run(void){
        vector<State *> *children = NULL;

        while(!p->is_done()){
		State *s = take();
		if (s == NULL)
			continue;

		if (s->get_f_prime() >= p->bound.read()) {
			open.prune();
		}
		if ((p->weight * s->get_f()) / fp_one >= p->bound.read()) {
			continue;
		}
		if (s->is_goal()) {
			p->set_path(s->get_path());
		}

		children = p->expand(s);
		for (unsigned int i = 0; i < children->size(); i += 1) {
			State *c = children->at(i);
			if (c->get_f_prime() < p->bound.read()
			    && ((c->get_f() * p->weight) / fp_one
				< p->bound.read()))
				send_state(c);
			else
				delete c;
		}
		delete children;
        }
}


/************************************************************/


wPRAStar::wPRAStar(unsigned int n_threads, bool d, bool abst, bool as,
		   unsigned int max_e)
	: n_threads(n_threads),
	  bound(fp_infinity),
	  project(NULL),
	  dd(d),
	  use_abstraction(abst),
	  async(as)
{
	max_exp = max_e;
	if (max_exp != 0 && !async) {
		cerr << "Max expansions must be zero for synchronous sends"
		     << endl;
		abort();
	}
        done = false;
}


wPRAStar::wPRAStar(unsigned int n_threads,
		   fp_type bound,
		   bool d,
		   bool abst,
		   bool as,
		   unsigned int max_e)
	: n_threads(n_threads),
          bound(bound),
	  project(NULL),
	  dd(d),
	  use_abstraction(abst),
	  async(as)
{
	max_exp = max_e;
	if (max_exp != 0 && !async) {
		cerr << "Max expansions must be zero for synchronous sends"
		     << endl;
		abort();
	}
        done = false;
}

wPRAStar::~wPRAStar(void) {
}

void wPRAStar::set_done()
{
        done = true;
}

bool wPRAStar::is_done()
{
        return done;
}

void wPRAStar::set_path(vector<State *> *p)
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

vector<State *> *wPRAStar::search(Timer *t, State *init)
{
	solutions = new SyncSolutionStream(t, 0.0001);
	project = init->get_domain()->get_projection();
	weight = init->get_domain()->get_heuristic()->get_weight();

        CompletionCounter cc = CompletionCounter(n_threads);

	threads.resize(n_threads, NULL);
        for (unsigned int i = 0; i < n_threads; i += 1)
		threads.at(i) = new wPRAStarThread(this, &threads, &cc);

	if (use_abstraction)
		threads.at(project->project(init)%n_threads)->open.add(init);
	else
		threads.at(init->hash() % n_threads)->open.add(init);

        for (iter = threads.begin(); iter != threads.end(); iter++) {
		(*iter)->start();
        }

        for (iter = threads.begin(); iter != threads.end(); iter++) {
		(*iter)->join();
		delete *iter;
        }

        return solutions->get_best_path();
}
