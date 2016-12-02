// © 2014 the PBNF Authors under the MIT license. See AUTHORS for the list of authors.

/**
 * \file sync_wastar.cc
 *
 *
 *
 * \author Sofia Lemons
 * \date 2009-04-16
 */

#include "util/thread.h"
#include "state.h"
#include "sync_wastar.h"
#include "closed_list.h"
#include "util/timer.h"

SyncWAStar::SyncWAStar(unsigned int n_threads, vector<State*> *inits, bool dd)
	: n_threads(n_threads), inits(inits), dd(dd) {}

SyncWAStar::~SyncWAStar(void) {}

class SyncWAStarThread : public Thread {
public:
	SyncWAStarThread(SyncWAStar *search, bool dd)
		: search(search), dd(dd) {}

	void run(void) {
		PQOpenList<State::PQOpsFPrime> open;
		State* init = search->get_next_init();

		while(!search->is_done() && init != NULL){
			open.add(init);

			while (!open.empty() && !search->is_done()) {
				State *s = open.take();

				if(s->get_f_prime() >= search->bound.read()){
					if (s->domain->get_heuristic()->get_weight() == fp_one){
						search->set_done();
					}
					break;
				}

				if (s->is_goal()) {
					search->set_path(s->get_path());
					if (s->domain->get_heuristic()->get_weight() == fp_one){
						search->set_done();
					}
					break;
				}

				vector<State *> *children = search->expand(s);
				for (unsigned int i = 0; i < children->size(); i += 1) {
					State *c = children->at(i);
					State *dup = closed.lookup(c);
					if (dup) {
						if (dup->get_g() > c->get_g()) {
							dup->update(c->get_parent(), c->get_c(), c->get_g());
							if (dup->is_open())
								open.see_update(dup);
							else if (!dd)
								open.add(dup);
						}
						delete c;
					} else {
						open.add(c);
						closed.add(c);
					}

				}
				delete children;
			}

			open.prune();
			closed.delete_all_states();
			init = search->get_next_init();
		}
	}

private:
	SyncWAStar *search;
	ClosedList closed;
	bool dd;
};

void SyncWAStar::set_done()
{
        pthread_mutex_lock(&mutex);
        done = true;
        pthread_mutex_unlock(&mutex);
}

bool SyncWAStar::is_done()
{
        bool ret;
        pthread_mutex_lock(&mutex);
 	ret = done;
        pthread_mutex_unlock(&mutex);
        return ret;
}

void SyncWAStar::set_path(vector<State *> *p)
{
        pthread_mutex_lock(&mutex);
        if (bound.read() > p->at(0)->get_g()){
		solutions->see_solution(p, get_generated(), get_expanded());
		bound.set(p->at(0)->get_g());
        }
        pthread_mutex_unlock(&mutex);
}

State *SyncWAStar::get_next_init(){
	State *ret;

	pthread_mutex_lock(&mutex);
	if (!final_weight) {
		ret = inits->at(next_init);
		next_init += 1;

		if (next_init == inits->size())
			final_weight = true;
	}
	else{
		ret = NULL;
	}
	pthread_mutex_unlock(&mutex);

	return ret;
}

vector<State *> *SyncWAStar::search(Timer *t, State *init)
{
	vector<SyncWAStarThread *> threads;
	next_init = 0;
	done = false;
	final_weight = false;
	bound.set(fp_infinity);
	solutions = new SyncSolutionStream(t, 0.0001);

	for (unsigned int i = 0; i < n_threads; i += 1)
		threads.push_back(new SyncWAStarThread(this, dd));

	for(vector<SyncWAStarThread *>::iterator i = threads.begin();
	    i != threads.end(); i++)
		(*i)->start();

	for(vector<SyncWAStarThread *>::iterator i = threads.begin();
	    i != threads.end(); i++) {
		(*i)->join();
	}

	return solutions->get_best_path();
}

void SyncWAStar::output_stats(void)
{
	solutions->output(cout);
}
