// © 2014 the PBNF Authors under the MIT license. See AUTHORS for the list of authors.

/**
 * \file prastar.cc
 *
 *
 *
 * \author Sofia Lemons
 * \date 2008-11-19
 */

#include <assert.h>
#include <math.h>
#include <errno.h>

#include <vector>
#include <limits>

extern "C" {
#include "lockfree/include/atomic.h"
}

#include "util/timer.h"
#include "util/mutex.h"
#include "util/msg_buffer.h"
#include "util/sync_solution_stream.h"
#include "ppastar.h"
#include "projection.h"
#include "search.h"
#include "state.h"

using namespace std;

#if defined(COUNT_FS)
F_hist PPAStar::fs;
#endif // COUNT_FS


PPAStar::PPAStarThread::PPAStarThread(PPAStar *p,
		vector<PPAStarThread *> *threads, CompletionCounter* cc) :
		p(p), threads(threads), cc(cc), q_empty(true),
		total_expansion(0), duplicate(0){
	expansions = 0;
	time_spinning = 0;
//	out_qs.resize(threads->size(), NULL);
	completed = false;
}


PPAStar::PPAStarThread::~PPAStarThread(void) {
//	vector<MsgBuffer<State*> *>::iterator i;
//	for (i = out_qs.begin(); i != out_qs.end(); i++)
//		if (*i)
//			delete *i;
	printf("expd = %u\n", total_expansion);
	printf("duplicate = %u\n", duplicate);
}

void PPAStar::PPAStarThread::run(void) {
//	vector<State *> *children = NULL;
	printf("start!\n");
	while (!p->is_done()) {
//		printf("loop\n");
		if (p->open.empty()) {
			cc->complete();
			if (cc->is_complete()) {
				p->set_done();
			}
			continue;
		}
		State *s = p->open.take();

		// Prune.
		if (s == NULL || (s->get_f() >= p->bound.read())) {
			continue;
		}
		cc->uncomplete();

		if (s->is_goal()) {
			p->set_path(s->get_path());
		}

//		printf("expd\n");
		vector<State *> *children = p->expand(s);
		for (unsigned int i = 0; i < children->size(); i += 1) {
			State *c = children->at(i);
			State *dup = p->closed.lookup(c);
			if (dup) {
				if (dup->get_g() > c->get_g()) {
				  // TODO: count how much duplication occurs
					dup->update(c->get_parent(), c->get_c(), c->get_g());
					if (dup->is_open())
						p->open.see_update(dup);
				}
				delete c;
			} else {
				p->open.add(c);
				p->closed.add(c);
			}

		}
		delete children;
	}

}

/************************************************************/


PPAStar::PPAStar(unsigned int n_threads, bool use_abst, bool a_send,
		bool a_recv, unsigned int max_e,
		unsigned int open_list_division,
		unsigned int closed_list_size,
		unsigned int closed_list_division) :
		open(open_list_division),
		closed(closed_list_size, closed_list_division),
		n_threads(n_threads), bound(fp_infinity), project(NULL), use_abstraction(
				use_abst), async_send(a_send), async_recv(a_recv), max_exp(
				max_e) {
	if (max_e != 0 && !async_send) {
		cerr << "Max expansions must be zero for synchronous sends" << endl;
		abort();
	}
	done = false;
}


PPAStar::~PPAStar(void) {
	for (iter = threads.begin(); iter != threads.end(); iter++) {
		if (*iter)
			delete (*iter);
	}
}


void PPAStar::set_done() {
	done = true;
}


bool PPAStar::is_done() {
	return done;
}


void PPAStar::set_path(vector<State *> *p) {
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


vector<State *> *PPAStar::search(Timer *timer, State *init) {
	solutions = new SyncSolutionStream(timer, 0.0001);
	project = init->get_domain()->get_projection();

	CompletionCounter cc = CompletionCounter(n_threads);

	threads.resize(n_threads, NULL);
	for (unsigned int i = 0; i < n_threads; i += 1)
		threads.at(i) = new PPAStarThread(this, &threads, &cc);

//	if (use_abstraction)
//		threads.at(project->project(init) % n_threads)->open.add(init);
//	else
//		threads.at(init->hash() % n_threads)->
	open.add(init);

	for (iter = threads.begin(); iter != threads.end(); iter++) {
		(*iter)->start();
	}

	for (iter = threads.begin(); iter != threads.end(); iter++)
		(*iter)->join();

	return solutions->get_best_path();
}


void PPAStar::output_stats(void) {
#if defined(QUEUE_SIZES)
	max_open_size = 0;
	avg_open_size = 0;
	unsigned long sum = 0;
	unsigned long num = 0;
	for (iter = threads.begin(); iter != threads.end(); iter++) {
		sum += (*iter)->open.get_sum();
		num += (*iter)->open.get_num();
		if ((*iter)->open.get_max_size() > max_open_size)
		max_open_size = (*iter)->open.get_max_size();
	}
	avg_open_size = (double) sum / num;
	cout << "average-open-size: " << avg_open_size << endl;
	cout << "max-open-size: " << max_open_size << endl;
#endif	// QUEUE_SIZES
#if defined(TIME_QUEUES)
	double cpu_time_sum = 0;
	unsigned long time_count = 0;
	for (iter = threads.begin(); iter != threads.end(); iter++) {
		cpu_time_sum += (*iter)->open.get_cpu_sum();
		time_count += (*iter)->open.get_time_count();
	}
	cout << "mean-pq-cpu-time: " << cpu_time_sum / time_count << endl;
#endif

#if defined(INSTRUMENTED)
	time_spinning = 0.0;
	max_spinning = 0.0;
	ThreadSpecific<double> lock_times = Mutex::get_lock_times();
	for (iter = threads.begin(); iter != threads.end(); iter++) {
		PPAStarThread *thr = *iter;
		double t = thr->time_spinning;
		double l = lock_times.get_value_for(thr->get_id());
		if (t > max_spinning)
		max_spinning = t;
		if (t > 0.)
		cout << "wait-time: " << t << endl;
		if (l > 0.)
		cout << "lock-time: " << l << endl;
		if (t + l > 0.)
		cout << "coord-time: " << t + l << endl;

		time_spinning += t;
	}
#endif	// QUEUE_SIZES
#if defined(COUNT_FS)
	ofstream o;
	std::cout << "Outputting to prastar-fs.dat" << std::endl;
	o.open("prastar-fs.dat", std::ios_base::app);
	fs.output_above(o, bound.read());
	o.close();
#endif	// COUNT_FS

	if (solutions)
		solutions->output(cout);

#if defined(INSTRUMENTED)
	cout << "total-time-acquiring-locks: "
	<< Mutex::get_total_lock_acquisition_time() << endl;
	cout << "average-time-acquiring-locks: "
	<< Mutex::get_total_lock_acquisition_time() / n_threads
	<< endl;
	cout << "max-time-acquiring-locks: "
	<< Mutex::get_max_lock_acquisition_time() << endl;
	cout << "total-time-waiting: " << time_spinning << endl;
	cout << "average-time-waiting: "
	<< time_spinning / n_threads << endl;
	cout << "max-time-waiting: " << max_spinning << endl;
#endif	// INSTRUMENTED
}
