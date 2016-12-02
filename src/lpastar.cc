// © 2014 the PBNF Authors under the MIT license. See AUTHORS for the list of authors.

/**
 * \file lpastar.cc
 *
 *
 *
 * \author Ethan Burns (modified from Sofia Lemons' pastar.cc)
 * \date 2009-04-12
 */

#include <assert.h>

#include "util/thread.h"
#include "util/atomic_int.h"
#include "state.h"
#include "lpastar.h"

extern "C" {
#include "lockfree/include/atomic.h"
}

class LPAStarThread : public Thread {
public:
	LPAStarThread() {}
	LPAStarThread(LPAStar *p) : p(p) {}
	LPAStarThread(LPAStar *p, AtomicInt* working) : p(p), working(working) {}

	virtual void run(void){
		vector<State *> *children = NULL;
		State *s;

		while(!p->is_done()){
			if(!p->open.empty()){
				s = p->open.take();
				if (!s)
					continue;
			} else{
				working->dec();
				if (p->open.empty() && working->read() == 0) {
					p->set_done();
					break;
				}

				while(p->open.empty() && working->read() != 0)
					;

				if (p->open.empty() && working->read() == 0)
					break;

				working->inc();
				continue;
			}

			if (s->get_f_prime() >= p->bound.read()) {
				continue;
			}

			State *dup = p->closed.lookup(s);
			if (dup && dup->get_g() < s->get_g()) {
				continue;
			}

			if (s->is_goal())
				p->set_path(s->get_path());

			children = p->expand(s);
			for (unsigned int i = 0; i < children->size(); i += 1) {
				State *c = children->at(i);
				State *dup = p->closed.lookup(c);
				if (dup) {
					if (dup->get_g() > c->get_g()) {
						p->closed.cond_update(c);
						p->open.add(c);
					} else {
						delete c;
					}
				} else {
					p->closed.cond_update(c);
					p->open.add(c);
				}
			}
		}

		delete children;
	}

private:
	LPAStar *p;
	friend class LPAStar;
	AtomicInt *working;
};


LPAStar::LPAStar(unsigned int n_threads)
	: n_threads(n_threads),
	  path(NULL),
	  bound(fp_infinity)
{
	done = false;
}

void LPAStar::set_done()
{
        done = true;
}

bool LPAStar::is_done()
{
        return done;
}

void LPAStar::set_path(vector<State *> *p)
{
	fp_type cost = p->at(0)->get_g();
	fp_type old_cost;
	vector<State *> *old_path;

	old_path = path;
	while (old_path == NULL
	       || old_path->at(0)->get_g() > cost) {
		if (compare_and_swap((void*) &path,
				     (intptr_t) old_path,
				     (intptr_t) p)) {
			break;
		}
		old_path = path;
	}

	old_cost = bound.read();
	while (old_cost > cost)
		old_cost = bound.cmp_and_swap(old_cost, cost);

	assert(bound.read() <= path->at(0)->get_g());
}

/**
 * Perform a Parallel A* search.
 */
vector<State *> *LPAStar::search(Timer *t, State *init)
{
 	open.add(init);

	AtomicInt working(n_threads);

        unsigned int worker;
        vector<LPAStarThread *> threads;
	vector<LPAStarThread *>::iterator iter;
        for (worker=0; worker<n_threads; worker++) {
		LPAStarThread *t = new LPAStarThread(this, &working);
		threads.push_back(t);
		t->start();
        }
	for (iter = threads.begin(); iter != threads.end(); iter++) {
		(*iter)->join();
		delete *iter;
	}

 	return path;
}
