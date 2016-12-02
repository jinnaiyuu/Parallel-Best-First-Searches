// © 2014 the PBNF Authors under the MIT license. See AUTHORS for the list of authors.

/**
 * \file kbfs.cc
 *
 *
 *
 * \author Sofia Lemons
 * \date 2008-10-09
 */

#include "util/thread.h"
#include "state.h"
#include "kbfs.h"

class KBFSThread : public Thread {
public:
	KBFSThread(){}
	KBFSThread(KBFS *k) : k(k) {}
	KBFSThread(State *s, KBFS *k) : s(s), k(k) {}

	~KBFSThread() {
          delete children;
	}

	virtual void run(void){
          k->cc.complete();
          wait();
                
          while(!do_exit){
            children = k->expand(s);
            k->cc.complete();
            wait();
          }
        }

private:
	State *s;
	KBFS *k;
        vector<State *> *children;
        friend class KBFS;
};


KBFS::KBFS(unsigned int n_th)
	: n_threads(n_th),
	  bound(fp_infinity)
{
	cc = CompletionCounter(n_th);
}


/**
 * Perform a KBFS search.
 */
vector<State *> *KBFS::search(Timer *t, State *init)
{
 	vector<State *> *path = NULL;
        unsigned int worker, i;
        vector<KBFSThread *> threads;
	vector<KBFSThread *>::iterator iter;
        for (worker=0; worker<n_threads; worker++) {
        	KBFSThread *t = new KBFSThread(this);
		threads.push_back(t);
		t->start();
        }
        cc.wait();

 	open.add(init);

 	while (!open.empty()) {
         	for (worker=0; (worker<n_threads) && !open.empty(); worker++) {
                    State *s = open.take();
                    if (s->is_goal()) {
			    if (path == NULL || 
				path->at(0)->get_g() > path->at(0)->get_g()){
				    path = s->get_path();
				    bound.set(path->at(0)->get_g());
			    }
			    worker--;
			    continue;
                    }

		    if (s->get_f() >= bound.read()) {
			    worker--;
			    closed.add(s);
			    continue;
		    }

		    closed.add(s);

                    threads[worker]->s = s;
                }

                cc.reset();
                cc.set_max(worker);

                for(i=0; i<worker; i++){
                    threads[i]->signal();
                }
                cc.wait();
		for(i=0; i<worker; i++){
		    for (unsigned int j = 0; 
			 j < threads[i]->children->size(); j += 1) {
		      State *c = threads[i]->children->at(j);
		      State *dup = closed.lookup(c);
			if (dup){
			  if (dup->get_g() > c->get_g()) {
				  dup->update(c->get_parent(), c->get_c(), c->get_g());
			    if (dup->is_open())
			      open.see_update(dup);
			    else
			      open.add(dup);
			  }
			  delete c;
			}
			else{
			  open.add(c);
			  closed.add(c);
			}
		    }
		}
 	}

	for (iter = threads.begin(); iter != threads.end(); iter++) {
		(*iter)->join();
		delete *iter;
	}

 	closed.delete_all_states();

 	return path;
}
