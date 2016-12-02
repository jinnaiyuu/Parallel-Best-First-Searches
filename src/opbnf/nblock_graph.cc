// © 2014 the PBNF Authors under the MIT license. See AUTHORS for the list of authors.

/**
 * \file nblock_graph.cc
 *
 *
 *
 * \author Sofia Lemons
 * \date 2009-04-18
 */

#include <assert.h>
#include <pthread.h>
#include <errno.h>

#include <limits>
#include <iostream>
#include <list>
#include <map>
#include <vector>

#include "../closed_list.h"
#include "../pq_open_list.h"
#include "../open_list.h"
#include "../projection.h"
#include "nblock.h"
#include "nblock_graph.h"

using namespace std;
using namespace OPBNF;

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

	f_min.set(0);
	map.set_observer(this);

	NBlock *n = map.get(init_nblock);
	n->add(initial);
	n->closed.add(initial);
	free_list_fp.add(n);
	free_list_f.add(n);

	done = false;

	pthread_mutex_init(&mutex, NULL);
	pthread_cond_init(&cond, NULL);
}

/**
 * Create a new NBlock graph.
 * \param p The projection function.
 */
NBlockGraph::NBlockGraph(const Projection *p, State *initial)
	: map(p)
{
	cpp_is_a_bad_language(p, initial);
}

/**
 * Destroy an NBlock graph.
 */
NBlockGraph::~NBlockGraph()
{
}


/**
 * Get the next nblock for expansion, possibly release a finished
 * nblock.  The next nblock is the one with the minimum f'-value.
 *
 * \note This call will block if there are currently no free nblocks.
 * \param finished If non-NULL, the finished nblock will be released
 *        into the next level's free_list.
 * \param trylock Set to true if a trylock should be attempted instead
 *                of a lock.
 * \return The next NBlock to expand or NULL if there is nothing left
 *         to do.
 */
NBlock *NBlockGraph::next_nblock(NBlock *finished, bool trylock, fp_type bound)
{
	NBlock *n = NULL;

	// Take the lock, but if someone else already has it, just
	// keep going.
	if (trylock && finished && !finished->empty()) {
		if (pthread_mutex_trylock(&mutex) == EBUSY)
			return finished;
	} else if(pthread_mutex_trylock(&mutex) == EBUSY)
		pthread_mutex_lock(&mutex);

	if (finished) {		// Release an NBlock
		// Sanity check.
		if (finished->sigma != 0) {
			cerr << "A proc had an NBlock with sigma != 0" << endl;
			finished->print(cerr);
			cerr << endl << endl << endl;
			__print(cerr);
		}
		assert(finished->sigma == 0);

		// Re-sort the nblock PQ so that we can get the f_min value.
		set<unsigned int>::iterator iter;
		for (iter = finished->succs.begin(); iter != finished->succs.end(); iter++) {
			NBlock *n = map.find(*iter);
			if (n) {
				n->update_local_f_min();
				nblock_pq.see_update(n->pq_index);
			}
		}
		finished->update_local_f_min();
		nblock_pq.see_update(finished->pq_index);

		/* don't get_best_f() here, since this could've
		 * changed out from under us.  Instead,
		 * get_local_f_min() which only changes when an nblock
		 * is released.  This is going to be an underestimate.
		 * The issue is that, if a processor is searching the
		 * f_min nblock, it may have emptied it... we don't
		 * want to read fp_inifinity here because the best
		 * nblock was emptied, better to read an old
		 * local_f_min value. */
		f_min.set(nblock_pq.front()->get_local_f_min());

//		cout << "f_{min}: " << f_min.read() << endl;

		// Test if this nblock is still worse than the front
		// of the free list.  If not, then just keep searching
		// it.
		if (!finished->empty()) {
			if (!NBlock::better(free_list_fp.front(), finished, bound)
			    && !NBlock::better(free_list_f.front(), finished, bound)) {
				n = finished;
				goto out;
			}
		}

		// Possibly add this block back to the free list.
		finished->inuse = false;
		if (is_free(finished)) {
			free_list_fp.add(finished);
			free_list_f.add(finished);
			pthread_cond_broadcast(&cond);
		}
		update_scope_sigmas(finished->id, -1);

		if (free_list_fp.empty() && num_sigma_zero == num_nblocks) {
			__set_done();
//			__print(cerr);
			goto out;
		}

	}

	while (!done && free_list_fp.empty())
		pthread_cond_wait(&cond, &mutex);

	if (done)
		goto out;

	if(best_free_fp_val() < bound){
		n = free_list_fp.take();
		free_list_f.remove(n->index_f);
	} else {
		n = free_list_f.take();
		free_list_fp.remove(n->index_fp);
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
	pthread_mutex_unlock(&mutex);

	return n;
}

/**
 * Get the best NBlock in the interference scope of b which is not free.
 */
NBlock *NBlockGraph::best_in_scope(NBlock *b, fp_type bound)
{
	NBlock *best_b = NULL;
	set<unsigned int>::iterator i;

//	pthread_mutex_lock(&mutex);

	for (i = b->interferes.begin(); i != b->interferes.end(); i++) {
		NBlock *b = map.find(*i);
		if (!b)
			continue;
		if (b->empty())
			continue;
		if (!best_b || NBlock::better(b, best_b, bound)) {
			best_b = b;
		}
	}

//	pthread_mutex_unlock(&mutex);

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
 * Get the value of the best nblock on the free list.
 */
fp_type NBlockGraph::best_free_fp_val(void){
	NBlock *b = NULL;
	b = free_list_fp.front();
	if (b)
		return b->get_best_fp();
	return fp_infinity;
}

/**
 * Get the value of the best nblock on the free list.
 */
fp_type NBlockGraph::best_free_f_val(void){
	NBlock *b = NULL;
	b = free_list_f.front();
	if (b)
		return b->get_best_f();
	return fp_infinity;
}

/**
 * Get the global f_min value.
 */
fp_type NBlockGraph::get_f_min(void)
{
	return f_min.read();
}


/**
 * Print an NBlock, but don't take the lock.
 */
void NBlockGraph::__print(ostream &o)
{

	o << "Number of NBlocks: " << num_nblocks << endl;
	o << "Number of NBlocks with sigma zero: " << num_sigma_zero << endl;
	o << "--------------------" << endl;
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
			if (is_free(m) && m->index_fp != -1){
				free_list_fp.remove(m->index_fp);
				free_list_f.remove(m->index_f);
			}
			num_sigma_zero -= 1;
		}
		m->sigma += delta;
		if (m->sigma == 0) {
			if (m->hot)
				set_cold(m);
			if (is_free(m)) {
				free_list_fp.add(m);
				free_list_f.add(m);
				pthread_cond_broadcast(&cond);
			}
			num_sigma_zero += 1;
		}
	}
}

/**
 * Sets the done flag with out taking the lock.
 */
void NBlockGraph::__set_done(void)
{
	done = true;
	pthread_cond_broadcast(&cond);
}

/**
 * Sets the done flag.
 */
void NBlockGraph::set_done(void)
{
	if (done)
		return;
	pthread_mutex_lock(&mutex);
	if (done) {
		pthread_mutex_unlock(&mutex);
		return;
	}
	__set_done();
	pthread_mutex_unlock(&mutex);
}

/**
 * Test if an NBlock is free.
 */
bool NBlockGraph::is_free(NBlock *b)
{
	return !b->inuse
		&& b->sigma == 0
		&& b->sigma_hot == 0
		&& !b->empty();
}

/**
 * Mark an NBlock as hot, we want this one.
 */
void NBlockGraph::set_hot(NBlock *b, fp_type bound)
{
	set<unsigned int>::iterator i;

	if(pthread_mutex_trylock(&mutex) == EBUSY)
		pthread_mutex_lock(&mutex);

	if (!b->hot && b->sigma > 0) {
		for (i = b->interferes.begin(); i != b->interferes.end(); i++) {
			assert(b->id != *i);
			NBlock *m = map.get(*i);
			if (m->hot && NBlock::better(m, b, bound)){
				goto out;
			}
		}

		b->hot = true;
		for (i = b->interferes.begin(); i != b->interferes.end(); i++) {
			assert(b->id != *i);
			NBlock *m = map.get(*i);
			if (is_free(m) && m->index_fp != -1){
				free_list_fp.remove(m->index_fp);
				free_list_f.remove(m->index_f);
			}
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
		if (is_free(m)) {
			free_list_fp.add(m);
			free_list_f.add(m);
			pthread_cond_broadcast(&cond);
		}
	}
}

/**
 * We won't release block b, so set all hot blocks in its interference
 * scope back to cold.
 */
void NBlockGraph::wont_release(NBlock *b)
{
	set<unsigned int>::iterator iter;

	if(pthread_mutex_trylock(&mutex) == EBUSY)
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

/**
 * This is where the NBlockMap notifies us of the creation of a new nblock.
 */
void NBlockGraph::observe(NBlock *b)
{
	nblock_pq.add(b);
#if !defined(NDEBUG)
	nblocks.push_front(b);
#endif	// !NDEBUG
}

NBlock *NBlockGraph::next_nblock_fp_peek()
{
	NBlock *b = NULL;
	b = free_list_fp.front();
	return b;
}

NBlock *NBlockGraph::next_nblock_f_peek()
{
	NBlock *b = NULL;
	b = free_list_f.front();
	return b;
}
