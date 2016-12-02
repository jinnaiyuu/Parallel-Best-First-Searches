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
#include "prastar_vector.h"
#include "projection.h"
#include "search.h"
#include "state.h"

using namespace std;

#if defined(COUNT_FS)
F_hist PRAStar::fs;
#endif // COUNT_FS
PRAStarVector::PRAStarVectorThread::PRAStarVectorThread(PRAStarVector *p,
		vector<PRAStarVectorThread *> *threads, CompletionCounter* cc) :
		p(p), threads(threads), cc(cc), closed(p->closed_list_size), q_empty(
				true), expanded(0), self_push(0) {
	expansions = 0;
	time_spinning = 0;
	out_qs.resize(threads->size(), NULL);
	completed = false;
	open.changeSize(p->openlistsize);
//	printf("resized to %u\n", p->openlistsize);
}

PRAStarVector::PRAStarVectorThread::~PRAStarVectorThread(void) {
	p->self_pushes += self_push;
	vector<MsgBuffer<State*> *>::iterator i;
	for (i = out_qs.begin(); i != out_qs.end(); i++)
		if (*i)
			delete *i;
	open.print_quality();
}

vector<State*> *PRAStarVector::PRAStarVectorThread::get_queue(void) {
	return &q;
}

Mutex *PRAStarVector::PRAStarVectorThread::get_mutex(void) {
	return &mutex;
}

void PRAStarVector::PRAStarVectorThread::post_send(void *t) {
	PRAStarVectorThread *thr = (PRAStarVectorThread*) t;
	if (thr->completed) {
		thr->cc->uncomplete();
		thr->completed = false;
	}
	thr->q_empty = false;
}

bool PRAStarVector::PRAStarVectorThread::flush_sends(void) {
	unsigned int i;
	bool has_sends = false;

	if (!p->async_send)
		return false;

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

void PRAStarVector::PRAStarVectorThread::flush_receives(bool has_sends) {
#if defined(INSTRUMENTED)
	Timer t;
	bool timer_started = false;
#endif // INSTRUMENTED
	// wait for either completion or more nodes to expand
	if (open.empty() || !p->async_recv)
		mutex.lock();
	else if (!mutex.try_lock()) // asynchronous
		return;

	if (q_empty && !has_sends && open.empty()) {
		completed = true;
		cc->complete();

#if defined(INSTRUMENTED)
		if (!timer_started) {
			timer_started = true;
			t.start();
		}
#endif	// INSTRUMENTED
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
#if defined(INSTRUMENTED)
	if (timer_started) {
		t.stop();
		time_spinning += t.get_wall_time();
	}
#endif	// INSTRUMENTED
	// got some stuff on the queue, lets do duplicate detection
	// and add stuff to the open list
	for (unsigned int i = 0; i < q.size(); i += 1) {
		State *c = q[i];
		if (c->get_f() >= p->bound.read()) {
			delete c;
			continue;
		}
		State *dup = closed.lookup(c);
		if (dup) {
			if (dup->get_g() > c->get_g()) {
				dup->update(c->get_parent(), c->get_c(), c->get_g());
				if (dup->is_open())
					open.see_update(dup);
				else
					open.add(dup);
			}
			delete c;
		} else {
			open.add(c);
			closed.add(c);
		}
	}
	q.clear();
	q_empty = true;
	mutex.unlock();
}

void PRAStarVector::PRAStarVectorThread::do_async_send(unsigned int dest_tid,
		State *c) {
	if (!out_qs[dest_tid]) {
		Mutex *lk = threads->at(dest_tid)->get_mutex();
		vector<State*> *qu = threads->at(dest_tid)->get_queue();
		out_qs[dest_tid] = new MsgBuffer<State*>(lk, qu, post_send,
				threads->at(dest_tid));
	}

	out_qs[dest_tid]->try_send(c);
}

void PRAStarVector::PRAStarVectorThread::do_sync_send(unsigned int dest_tid,
		State *c) {
	PRAStarVectorThread *dest = threads->at(dest_tid);

	dest->get_mutex()->lock();
	dest->get_queue()->push_back(c);
	post_send(dest);
	dest->get_mutex()->unlock();
}

void PRAStarVector::PRAStarVectorThread::send_state(State *c) {
	unsigned long hash =
			p->use_abstraction ? p->project->project(c) : c->dist_hash();
//	: c->zbrhash();
	unsigned int dest_tid = threads->at(hash % p->n_threads)->get_id();
	bool self_add = dest_tid == this->get_id();

	assert(p->n_threads != 1 || self_add);

	if (self_add) {
		++self_push;
		State *dup = closed.lookup(c);
		if (dup) {
			if (dup->get_g() > c->get_g()) {
				dup->update(c->get_parent(), c->get_c(), c->get_g());
				if (dup->is_open())
					open.see_update(dup);
				else
					open.add(dup);
			}
			delete c;
		} else {
			open.add(c);
			closed.add(c);
		}
		return;
	}

	// not a self add
	if (p->async_send)
		do_async_send(dest_tid, c);
	else
		do_sync_send(dest_tid, c);
}

State *PRAStarVector::PRAStarVectorThread::take(void) {
	bool has_sends = true;

	expansions += 1;
	if (p->max_exp == 0 || expansions > p->max_exp)
		has_sends = flush_sends();
	if (!q_empty)
		flush_receives(has_sends);

	while (open.empty()) {

		if (has_sends)
			has_sends = flush_sends();

		flush_receives(has_sends);

		if (cc->is_complete()) {
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

void PRAStarVector::PRAStarVectorThread::run(void) {
	vector<State *> *children = NULL;

	while (!p->is_done()) {

		// Double lock HDA*
//		if (mutex.try_lock()) {
////			printf("locked\n");
//			mutex.unlock();
//		}

		State *s = take();
		if (s == NULL)
			continue;

		if (s->get_f() >= p->bound.read()) {
			/*
			 open.prune();
			 */
			continue;
		}
		if (s->is_goal()) {
			p->set_path(s->get_path());
		}

#if defined(COUNT_FS)
		fs.see_f(s->get_f());
#endif // COUNT_FS
		children = p->expand(s);

		// Delete if serious serious comparison.
		++expanded;

		for (unsigned int i = 0; i < children->size(); i += 1) {
			State *c = children->at(i);
			if (c->get_f() < p->bound.read())
				send_state(c);
			else
				delete c;
		}
		delete children;
	}
}

/************************************************************/

PRAStarVector::PRAStarVector(unsigned int n_threads, bool use_abst, bool a_send,
		bool a_recv, unsigned int max_e, unsigned int openlistsize_) :
		n_threads(n_threads), bound(fp_infinity), project(NULL), use_abstraction(
				use_abst), async_send(a_send), async_recv(a_recv), max_exp(
				max_e), closed_list_size(1000000), openlistsize(openlistsize_),
				self_pushes(0){
	if (max_e != 0 && !async_send) {
		cerr << "Max expansions must be zero for synchronous sends" << endl;
		abort();
	}
	done = false;
	printf("closedlistsize = %u\n", closed_list_size);
}

PRAStarVector::PRAStarVector(unsigned int n_threads, bool use_abst, bool a_send,
		bool a_recv, unsigned int max_e, unsigned int openlistsize_,
		unsigned int closed_list_size_) :
		n_threads(n_threads), bound(fp_infinity), project(NULL), use_abstraction(
				use_abst), async_send(a_send), async_recv(a_recv), max_exp(
				max_e), closed_list_size(closed_list_size_), openlistsize(
				openlistsize_), self_pushes(0) {
	if (max_e != 0 && !async_send) {
		cerr << "Max expansions must be zero for synchronous sends" << endl;
		abort();
	}
	done = false;
	printf("closedlistsize = %u\n", closed_list_size);
}

PRAStarVector::~PRAStarVector(void) {
	vector<unsigned int> expds(threads.size(), 0);
//	expds.resize(threads.size());
	unsigned int i = 0;
	for (iter = threads.begin(); iter != threads.end(); iter++) {
		if (*iter) {
//			printf("%u ", (*iter)->expanded);
			expds[i] = (*iter)->expanded;
			++i;
			delete (*iter);
		}
	}
	double max = 0;
	double sum = 0;
	for (unsigned int j = 0; j < expds.size(); ++j) {
		if (max < expds[j]) {
			max = expds[j];
		}
		sum += expds[j];
	}
	printf("load_balance: %f\n", max / (sum / expds.size()) );
	printf("self_push: %u\n", self_pushes);
}

void PRAStarVector::set_done() {
	done = true;
}

bool PRAStarVector::is_done() {
	return done;
}

void PRAStarVector::set_path(vector<State *> *p) {
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

vector<State *> *PRAStarVector::search(Timer *timer, State *init) {
	solutions = new SyncSolutionStream(timer, 0.0001);
	project = init->get_domain()->get_projection();

	CompletionCounter cc = CompletionCounter(n_threads);

	threads.resize(n_threads, NULL);
	for (unsigned int i = 0; i < n_threads; i += 1)
		threads.at(i) = new PRAStarVectorThread(this, &threads, &cc);

	if (use_abstraction)
		threads.at(project->project(init) % n_threads)->open.add(init);
	else
		threads.at(init->hash() % n_threads)->open.add(init);

	for (iter = threads.begin(); iter != threads.end(); iter++) {
		(*iter)->start();
	}

	for (iter = threads.begin(); iter != threads.end(); iter++)
		(*iter)->join();

	return solutions->get_best_path();
}

void PRAStarVector::output_stats(void) {
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
		PRAStarThread *thr = *iter;
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
