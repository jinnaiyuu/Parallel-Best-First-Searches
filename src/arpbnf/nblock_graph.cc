// © 2014 the PBNF Authors under the MIT license. See AUTHORS for the list of authors.

/**
 * \file nblock_graph.cc
 *
 *
 *
 * \author Ethan Burns
 * \date 2008-10-20
 */

#include <assert.h>
#include <pthread.h>
#include <errno.h>
#include <stdio.h>

#include <limits>
#include <iostream>
#include <list>
#include <map>
#include <vector>

#include "../closed_list.h"
#include "../pq_open_list.h"
#include "../open_list.h"
#include "../projection.h"
#include "../arpbnf_search.h"
#include "nblock.h"
#include "nblock_graph.h"

using namespace std;
using namespace ARPBNF;

/**
 * Create the nblock with the given ID.
 */
NBlock *NBlockGraph::create_nblock(unsigned int id)
{
	NBlock *n = new NBlock(project, id);

	nblocks_created += 1;

	return n;
}

/**
 * Apparently gdb can't single step inside a c++ constructor... so we
 * just call this function in the constructor so that we can see what
 * is going on.
 */
void NBlockGraph::cpp_is_a_bad_language(const Projection *p, State *initial)
{
	unsigned int init_nblock = p->project(initial);

	nblocks_created = 0;
	project = p;
	num_sigma_zero = num_nblocks = p->get_num_nblocks();
	assert(init_nblock < num_nblocks);

	resort_q = lf_queue_create(num_nblocks);

	map.set_observer(this);
	NBlock *n = map.get(init_nblock);
	n->open.add(initial);
	n->closed.add(initial);
	free_list.add(n);

	done = false;

	pthread_mutex_init(&mutex, NULL);
	pthread_cond_init(&cond, NULL);
}

/**
 * Create a new NBlock graph.
 * \param p The projection function.
 */
NBlockGraph::NBlockGraph(const Projection *p, State *initial, AtomicInt *b)
	: map(p),
	  resort_flag(false),
	  resort_start(false),
	  bound(b)
{
	cpp_is_a_bad_language(p, initial);
}

/**
 * Destroy an NBlock graph.
 */
NBlockGraph::~NBlockGraph()
{
	lf_queue_destroy(resort_q);
}


/**
 * Get the next nblock for expansion, possibly release a finished
 * nblock.  The next nblock is the one with the minimum f-value.
 *
 * \note This call will block if there are currently no free nblocks.
 * \param finished If non-NULL, the finished nblock will be released
 *        into the next level's free_list.
 * \param trylock Set to true if a trylock should be attempted instead
 *                of a lock.
 * \return The next NBlock to expand or NULL if there is nothing left
 *         to do.
 */
NBlock *NBlockGraph::next_nblock(NBlock *finished, bool trylock, bool final_wt)
{
	NBlock *n = NULL;

	// Take the lock, but if someone else already has it, just
	// keep going.
	if (!resort_flag && trylock && finished && !finished->open.empty()) {
		if (pthread_mutex_trylock(&mutex) == EBUSY)
			return finished;
	} else {
		pthread_mutex_lock(&mutex);
	}

	if (finished) {		// Release an NBlock
		if (finished->sigma != 0) {
			cerr << "A proc had an NBlock with sigma != 0" << endl;
			finished->print(cerr);
			cerr << endl << endl << endl;
			__print(cerr);
		}
		assert(finished->sigma == 0);

		if (!resort_flag && !finished->open.empty()) {
			fp_type cur_f = finished->open.get_best_val();
			fp_type new_f;
			if (free_list.empty())
				new_f = fp_infinity;
			else
				new_f = free_list.front()->open.get_best_val();
			if (cur_f <= new_f && cur_f <= bound->read()) {
				n = finished;
				goto out;
			}
		}

		free_nblock(finished);
	}

again:
	if (resort_flag) {
		pthread_mutex_unlock(&mutex);
		resort();
		pthread_mutex_lock(&mutex);
	}

	if (num_sigma_zero.read() == num_nblocks
	    && !resort_flag
	    && (free_list.empty()
		|| free_list.front()->open.get_best_val() > bound->read())) {
		list<NBlock*>::iterator iter;

#if !defined(NDEBUG)
		for (iter = nblocks.begin(); iter != nblocks.end(); iter++) {
			assert((*iter)->open.empty() || (*iter)->open.get_best_val() > bound->read());
		}
#endif	// !NDEBUG

		n = NULL;
		goto out;
	}


	while (!resort_flag && !done && free_list.empty()) {
		pthread_cond_wait(&cond, &mutex);
	}

	if (done)
		goto out;

	if (resort_flag)
		goto again;


	n = free_list.take();

	if (n->open.get_best_val() > bound->read()) {

		free_if_free(n);

		// Lets let some other threads progress before we try
		// again.
		pthread_cond_wait(&cond, &mutex);
		goto again;
	}

	n->inuse = true;
	update_scope_sigmas(n->id, 1);

/*
  for (set<NBlock *>::iterator iter = n->interferes.begin();
  iter != n->interferes.end();
  iter++)
  assert((*iter)->sigma > 0);
*/
out:
	if (n == NULL && final_wt) {
#if !defined(NDEBUG)
		cerr << "Exiting with NULL:" << endl;
		cerr << "\tnum_sigma_zero: " << num_sigma_zero.read() << endl;
		cerr << "\tnum_nblocks: " << num_nblocks << endl;
#endif	// !NDEBUG
		set_done();
	}

	pthread_mutex_unlock(&mutex);

	return n;
}

void NBlockGraph::free_if_free(NBlock *finished)
{
	if (is_free(finished)) {
		free_list.add(finished);
		pthread_cond_broadcast(&cond);
	}
}

void NBlockGraph::free_nblock(NBlock *finished)
{
	finished->inuse = false;
	free_if_free(finished);
	update_scope_sigmas(finished->id, -1);
}

/**
 * Get the best NBlock in the interference scope of b which is not free.
 */
NBlock *NBlockGraph::best_in_scope(NBlock *b)
{
	NBlock *best_b = NULL;
	fp_type best_val = fp_infinity;
	set<unsigned int>::iterator i;

	for (i = b->interferes.begin(); i != b->interferes.end(); i++) {
		NBlock *b = map.find(*i);
		if (!b)
			continue;
		if (b->open.empty())
			continue;
		if (!best_b || b->open.get_best_val() < best_val) {
			best_val = b->open.get_best_val();
			best_b = b;
		}
	}

	return best_b;
}

/**
 * Get the NBlock given by the hash value.
 */
NBlock *NBlockGraph::get_nblock(unsigned int hash)
{
	return map.get(hash);
}

/**
 * Get the value of the best nblock.
 */
fp_type NBlockGraph::best_val(void)
{
	NBlock *b = NULL;
	b = free_list.front();
	if (b)
		return b->open.get_best_val();
	return fp_infinity;
}


/**
 * Print an NBlock, but don't take the lock.
 */
void NBlockGraph::__print(ostream &o)
{

	o << "Number of NBlocks: " << num_nblocks << endl;
	o << "Number of NBlocks with sigma zero: "
	  << num_sigma_zero.read() << endl;
	o << "All Blocks:" << endl;
	for (unsigned int i = 0; i < num_nblocks; i += 1)
		if (map.find(i))
			map.find(i)->print(o);
}

/**
 * Print an NBlockGraph to the given stream.
 */
void NBlockGraph::print(ostream &o)
{
	pthread_mutex_lock(&mutex);
	__print(o);
	pthread_mutex_unlock(&mutex);
}

/**
 * Update the sigmas by delta for all of the predecessors of y and all
 * of the predecessors of the successors of y.
 */
void NBlockGraph::update_scope_sigmas(unsigned int y, int delta)
{
	set<unsigned int>::iterator iter;
	NBlock *n = map.get(y);

	assert(n->sigma == 0);

	/*
	  \A y' \in predecessors(y) /\ y' /= y,
	  \sigma(y') <- \sigma(y') +- 1

	  \A y' \in successors(y),
	  \A y'' \in predecessors(y'), /\ y'' /= y,
	  \sigma(y'') <- \sigma(y'') +- 1
	*/

	for (iter = n->interferes.begin();
	     iter != n->interferes.end();
	     iter++) {
		NBlock *m = map.get(*iter);
		if (m->sigma == 0) {
			assert(delta > 0);
			if (is_free(m) && m->pq_index != -1)
				free_list.remove(m->pq_index);
			num_sigma_zero.dec();
		}
		m->sigma += delta;
		if (m->sigma == 0) {
			if (m->hot)
				set_cold(m);
			free_if_free(m);
			num_sigma_zero.inc();
		}
	}
}

/**
 * Sets the done flag with out taking the lock.
 */
void NBlockGraph::set_done(void)
{
	list<NBlock*>::iterator iter;

	if (done)
		return;

#if !defined(NDEBUG)
	cerr << endl;

	if (!free_list.empty())
		cerr << "Free list is empty" << endl;
	else
		cerr << "Free list is _NOT_ empty" << endl;

	cerr << "num_sigma_zero: " << num_sigma_zero.read() << endl;
	cerr << "num_nblocks: " << num_nblocks << endl;

	for (iter = nblocks.begin(); iter != nblocks.end(); iter++) {
		if (!(!(*iter)->hot)) {
			cerr << "Done, but there is a hot block" << endl;
		}

		if (!((*iter)->incons.empty())) {
			cerr << "Done, but there is a non-empty incons list" << endl;
		}

		if (!((*iter)->open.empty() || (*iter)->open.get_best_val() >= bound->read())) {
			cerr << "Done, but there is an openlist with in-bound values" << endl;
			(*iter)->print(cerr);
		}

		(*iter)->open.verify();
	}
#endif	// !NDEBUG

	done = true;
	pthread_cond_broadcast(&cond);
}

/**
 * Test if an NBlock is free.
 */
bool NBlockGraph::is_free(NBlock *b)
{
	return !b->inuse
		&& b->sigma == 0
		&& b->sigma_hot == 0
		&& !b->open.empty();
}

/**
 * Mark an NBlock as hot, we want this one.
 */
void NBlockGraph::set_hot(NBlock *b)
{
	set<unsigned int>::iterator i;
	fp_type val = b->open.get_best_val();

	pthread_mutex_lock(&mutex);

	if (!b->hot && b->sigma > 0) {
		for (i = b->interferes.begin(); i != b->interferes.end(); i++) {
			assert(b->id != *i);
			NBlock *m = map.get(*i);
			if (m->hot && m->open.get_best_val() < val)
				goto out;
		}

		b->hot = true;
		for (i = b->interferes.begin(); i != b->interferes.end(); i++) {
			assert(b->id != *i);
			NBlock *m = map.get(*i);
			if (is_free(m) && m->pq_index != -1)
				free_list.remove(m->pq_index);
			if (m->hot)
				set_cold(m);
			m->sigma_hot += 1;
		}
	}
out:
	pthread_mutex_unlock(&mutex);
}

/**
 * Mark an NBlock as cold.  The mutex must be held *before* this
 * function is called.n
 */
void NBlockGraph::set_cold(NBlock *b)
{
	set<unsigned int>::iterator i;

	b->hot = false;
	for (i = b->interferes.begin(); i != b->interferes.end(); i++) {
		assert(b->id != *i);
		NBlock *m = map.get(*i);
		m->sigma_hot -= 1;
		free_if_free(m);
	}
}

/**
 * We won't release block b, so set all hot blocks in its interference
 * scope back to cold.
 */
void NBlockGraph::wont_release(NBlock *b)
{
	set<unsigned int>::iterator iter;

	pthread_mutex_lock(&mutex);

	for (iter = b->interferes.begin();
	     iter != b->interferes.end();
	     iter++) {
		NBlock *m = map.find(*iter);
		if (!m)
			continue;
		if (m->hot)
			set_cold(m);
	}

	pthread_mutex_unlock(&mutex);
}

/**
 * Get the number of nblocks which were actually created.
 */
unsigned int NBlockGraph::get_ncreated_nblocks(void)
{
	return map.get_num_created();;
}


/*
 * The NBlockMap calls this when an nblock is lazily created.
 */
void NBlockGraph::observe(NBlock *b)
{
	nblocks.push_front(b);
}

bool NBlockGraph::needs_resort()
{
	return resort_flag;
}

void NBlockGraph::call_for_resort(NBlock *finished, bool final_weight, ARPBNFSearch *s)
{
	int n = 0;
#if !defined(NDEBUG)
	unsigned long nodes_before, nodes_after;
#endif	// NDEBUG
	list<NBlock *>::iterator iter;

	// don't bother resorting if we are done.
	if (done || final_weight)
		return;

	/*
	 * Try to set the resort flag.  If it is successfully set,
	 * then we are the master thread.
	 */
	pthread_mutex_lock(&mutex);
	if (resort_flag) {
		// someone else got here first, just resort and leave
		pthread_mutex_unlock(&mutex);
		resort();
		return;
	}
	resort_flag = true;
	if (finished)
		free_nblock(finished);
	pthread_cond_broadcast(&cond);
	pthread_mutex_unlock(&mutex);

retry:
	/*
	 * wait for everything to release
	 */
	while (num_sigma_zero.read() != num_nblocks)
		;

	pthread_mutex_lock(&mutex);
	if (num_sigma_zero.read() != num_nblocks) {
		pthread_mutex_unlock(&mutex);
		goto retry;
	}

	n_to_sort.set(0);

	s->move_to_next_weight();

	/*
	 * At this point, all nblocks are released... let's initiate
	 * the resort.
	 */
#if !defined(NDEBUG)
	nodes_before = 0;
#endif	// !NDEBUG
	for (iter = nblocks.begin(); iter != nblocks.end(); iter++) {
		int err;
#if !defined(NDEBUG)
		nodes_before += (*iter)->open.size();
		nodes_before += (*iter)->incons.size();
#endif	// !NDEBUG
		err = lf_queue_enqueue(resort_q, *iter);
		if (err) {
			perror(__func__);
			exit(1);
		}
		n += 1;
	}
	n_to_sort.set(n);

	// setting this signals the slave threads to begin sorting.
	resort_start = true;

	// master thread begins sorting.
	resort();

	while (n_to_sort.read() > 0)
		;

	/*
	 * The blocks should now be resorted.  The master resorts the
	 * free_list then clears the flag so everyone can continue.
	 */

	free_list.reset();

	bool all_empty = true;
	bool in_bound = false;
#if !defined(NDEBUG)
	nodes_after = 0;
#endif	// !NDEBUG
	for (iter = nblocks.begin(); iter != nblocks.end(); iter++) {
#if !defined(NDEBUG)
		nodes_after += (*iter)->open.size();
		nodes_after += (*iter)->incons.size();
#endif	// !NDEBUG
		assert((*iter)->incons.empty());
		if ((*iter)->open.get_best_val() <= bound->read())
			in_bound = true;
		if (!(*iter)->open.empty())
			all_empty = false;
		assert((*iter)->incons.empty());
		assert((*iter)->pq_index == -1);
		free_if_free(*iter);
	}

	assert(nodes_before == nodes_after);

	if (all_empty || (!in_bound && final_weight))
		set_done();

	resort_flag = false;
	resort_start = false;
	pthread_mutex_unlock(&mutex);
}

void NBlockGraph::resort(void)
{
	/** Spin until the master thread declares the resort has started. */
	while(!resort_start && resort_flag)
		;

	if (!resort_flag)
		return;

	while (!lf_queue_empty(resort_q)) {
		NBlock *b = (NBlock*) lf_queue_dequeue(resort_q);
		if (b) {
			assert(b->sigma == 0);
			assert(b->sigma_hot == 0);
			b->resort();
			n_to_sort.dec();
		}
	}

	// at this point, the master has the lock, so we won't get any
	// searching done until the master is finished.
}

bool NBlockGraph::is_done(void)
{
	return done;
}

void NBlockGraph::new_bound(void)
{
	pthread_cond_signal(&cond);
}
