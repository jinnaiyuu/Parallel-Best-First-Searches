// © 2014 the PBNF Authors under the MIT license. See AUTHORS for the list of authors.

/**
 * \file pastar.cc
 *
 *
 *
 * \author Sofia Lemons
 * \date 2008-11-02
 */

#include "util/thread.h"
#include "state.h"
#include "pastar.h"

class PAStarThread : public Thread {
public:
	PAStarThread() {}
	PAStarThread(PAStar *p) : p(p) {}
	PAStarThread(PAStar *p, pthread_mutex_t* mut, CompletionCounter* cc) : p(p), mut(mut), cc(cc) {}

	virtual void run(void){
		vector<State *> *children = NULL;
		State *s;

		while(!p->is_done()){
			pthread_mutex_lock(mut);
			if(!p->open.empty()){
				s = p->open.take();
				pthread_mutex_unlock(mut);
			}
			else{
				cc->complete();
				if (cc->is_complete()){
					p->set_done();
				}
				if (p->done==true){
					pthread_mutex_unlock(mut);
					if (children)
						delete children;
					return;
				}
				pthread_mutex_unlock(mut);
				while(p->open.empty() && !p->is_done()){
				}
				pthread_mutex_lock(mut);
				cc->uncomplete();
				pthread_mutex_unlock(mut);
				continue;
			}

			if (s->get_f() >= p->bound.read()) {
				p->open.prune();
				continue;
			}

			if (s->is_goal())
				p->set_path(s->get_path());

			children = p->expand(s);
			for (unsigned int i = 0; i < children->size(); i += 1) {
				State *c = children->at(i);
				State *dup = p->closed.lookup(c);
				if (dup){
					pthread_mutex_lock(mut);
					if (dup->get_g() > c->get_g()) {
						dup->update(c->get_parent(), c->get_c(), c->get_g());
						if (dup->is_open())
							p->open.see_update(dup);
						else
							p->open.add(dup);
					}
					pthread_mutex_unlock(mut);
					delete c;
				}
				else{
					p->open.add(c);
					p->closed.add(c);
				}
			}
		}

		delete children;
	}

private:
	PAStar *p;
	pthread_mutex_t* mut;
	friend class PAStar;
	CompletionCounter *cc;
};


PAStar::PAStar(unsigned int n_threads) : n_threads(n_threads),
					 path(NULL),
					 bound(fp_infinity){
	done = false;
					 }

void PAStar::set_done()
{
        pthread_mutex_lock(&mutex);
        done = true;
        pthread_mutex_unlock(&mutex);
}

bool PAStar::is_done()
{
        bool ret;
        pthread_mutex_lock(&mutex);
 	ret = done;
        pthread_mutex_unlock(&mutex);
        return ret;
}

void PAStar::set_path(vector<State *> *p)
{
        pthread_mutex_lock(&mutex);
        if (this->path == NULL ||
	    this->path->at(0)->get_g() > p->at(0)->get_g()){
		this->path = p;
		bound.set(p->at(0)->get_g());
        }
        pthread_mutex_unlock(&mutex);
}

bool PAStar::has_path()
{
        bool ret;
        pthread_mutex_lock(&mutex);
 	ret = (path != NULL);
        pthread_mutex_unlock(&mutex);
        return ret;
}


/**
 * Perform a Parallel A* search.
 */
vector<State *> *PAStar::search(Timer *t, State *init)
{
 	open.add(init);
        pthread_mutex_init(&mutex, NULL);

        CompletionCounter cc = CompletionCounter(n_threads);
	pthread_mutex_t m;
        pthread_mutex_init(&m, NULL);

        unsigned int worker;
        vector<PAStarThread *> threads;
	vector<PAStarThread *>::iterator iter;
        for (worker=0; worker<n_threads; worker++) {
		PAStarThread *t = new PAStarThread(this, &m, &cc);
		threads.push_back(t);
		t->start();
        }
	for (iter = threads.begin(); iter != threads.end(); iter++) {
		(*iter)->join();
		delete *iter;
	}

 	return path;
}
