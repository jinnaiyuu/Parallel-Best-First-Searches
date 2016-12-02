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
using namespace WPBNF;

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
void NBlockGraph::cpp_is_a_bad_language(const Projection *p,
					State *initial,
					AtomicInt *b /* bound */,
					double w /* weight */)
{
	unsigned int init_nblock = p->project(initial);

	nblocks_created = 0;
	project = p;
	num_sigma_zero = num_nblocks = p->get_num_nblocks();
	assert(init_nblock < num_nblocks);

	NBlock *n = map.get(init_nblock);
	n->open_fp.add(initial);
	n->closed.add(initial);
	free_list.add(n);

	done = false;
	bound = b;
	weight = w;

	nblocks_assigned = 0;
	nblocks_assigned_max = 0;
}

/**
 * Create a new NBlock graph.
 * \param p The projection function.
 */
NBlockGraph::NBlockGraph(const Projection *p,
			 State *initial,
			 AtomicInt *b /* bound */,
			 double w /* weight */)
	: map(p)
{
	cpp_is_a_bad_language(p, initial, b, w);
}

/**
 * Destroy an NBlock graph.
 */
NBlockGraph::~NBlockGraph()
{
}


/**
 * Get the next nblock for expansion, possibly release a finished
 * nblock.  The next nblock is the one with the minimum f-value.
 *
 * \note This call will block if there are currently no free nblocks.
 * \param finished If non-NULL, the finished nblock will be released
 *        into the next level's free_list.
 * \return The next NBlock to expand or NULL if there is nothing left
 *         to do.
 */
NBlock *NBlockGraph::next_nblock(NBlock *finished)
{
	NBlock *n = NULL;

	// Take the lock, but if someone else already has it, just
	// keep going.
	if (finished && !finished->open_fp.empty()) {
		if (!mutex.try_lock())
			return finished;
	} else {
		mutex.lock();
	}

	if (finished) {		// Release an NBlock
		assert(finished->sigma == 0);

		nblocks_assigned -= 1;

		// Possibly add this block back to the free list.
		finished->inuse = false;
		if (is_free(finished)) {
			free_list.add(finished);
			mutex.cond_signal();
		}

		update_scope_sigmas(finished, -1);
	}

retry:

	if (num_sigma_zero == num_nblocks && free_list.empty()) {
		set_done();
		goto out;
	}

	while (!done && free_list.empty())
		mutex.cond_wait();

	if (done)
		goto out;

	if (free_list.front()->best_value() >= (bound->read() * weight)) {
		prune_free_list();
		goto retry;
	}

	n = free_list.take();
	nblocks_assigned += 1;
	if (nblocks_assigned > nblocks_assigned_max)
		nblocks_assigned_max = nblocks_assigned;
	n->inuse = true;
	update_scope_sigmas(n, 1);

/*
  for (set<NBlock *>::iterator iter = n->interferes.begin();
  iter != n->interferes.end();
  iter++)
  assert((*iter)->sigma > 0);
*/
out:
	mutex.unlock();
	return n;
}


/**
 * Get the best NBlock in the interference scope of b which is not free.
 */
NBlock *NBlockGraph::best_in_scope(NBlock *block)
{
	NBlock *best_b = NULL;
	fp_type best_val = fp_infinity;

	for (unsigned int i = 0; i < block->ninterferes; i++) {
		NBlock *b = map.find(block->interferes[i]);
		if (!b)
			continue;
		if (b->open_fp.empty())
			continue;
		if (!best_b || b->open_fp.get_best_val() < best_val) {
			best_val = b->open_fp.get_best_val();
			best_b = b;
		}
	}

	return best_b;
}

/*
 * Remove all nblocks from the free_list.  The nodes in these nblocks
 * will remain in their open lists, for pruning later... if they ever
 * are added back to the free_list.  They can be added back to the
 * free_list if a processor searches one of their neighbors and they
 * gain a node worth looking at.
 */
void NBlockGraph::prune_free_list(void)
{
	free_list.reset();
}


/**
 * Get the NBlock given by the hash value.
 */
NBlock *NBlockGraph::get_nblock(unsigned int hash)
{
	return map.get(hash);
}

/**
 * Get the statistics on the maximum number of NBlocks assigned at one time.
 */
unsigned int NBlockGraph::get_max_assigned_nblocks(void) const
{

	return nblocks_assigned_max;
}

/**
 * Get the value of the best nblock on the free list.
 */
fp_type NBlockGraph::best_free_val(void)
{
	NBlock *b = NULL;
	b = free_list.front();
	if (b)
		return b->open_fp.get_best_val();
	return fp_infinity;
}


/**
 * Print an NBlock, but don't take the lock.
 */
void NBlockGraph::__print(ostream &o)
{

	o << "Number of NBlocks: " << num_nblocks << endl;
	o << "Number of NBlocks with sigma zero: " << num_sigma_zero << endl;
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
	mutex.lock();
	__print(o);
	mutex.unlock();
}

/**
 * Update the sigmas by delta for all of the predecessors of y and all
 * of the predecessors of the successors of y.
 */
void NBlockGraph::update_scope_sigmas(NBlock *n, int delta)
{
	bool broadcast = false;
	assert(n->sigma == 0);

	/*
	  \A y' \in predecessors(y) /\ y' /= y,
	  \sigma(y') <- \sigma(y') +- 1

	  \A y' \in successors(y),
	  \A y'' \in predecessors(y'), /\ y'' /= y,
	  \sigma(y'') <- \sigma(y'') +- 1
	*/

	for (unsigned int i = 0; i < n->ninterferes; i++) {
		NBlock *m = map.get(n->interferes[i]);
		if (m->sigma == 0) {
			assert(delta > 0);
			if (is_free(m) && m->fp_pq_index != -1)
				free_list.remove(m->fp_pq_index);
			num_sigma_zero -= 1;
		}
		m->sigma += delta;
		if (m->sigma == 0) {
			if (m->hot)
				broadcast |= set_cold(m);
			if (is_free(m)) {
				free_list.add(m);
				broadcast = true;
			}
			num_sigma_zero += 1;
		}
	}

	if (broadcast)
		mutex.cond_broadcast();
}

/**
 * Sets the done flag.
 */
void NBlockGraph::set_done(void)
{
	done = true;
	mutex.cond_broadcast();
}

/**
 * Test if an NBlock is free.
 */
bool NBlockGraph::is_free(NBlock *b)
{
	return !b->inuse
		&& b->sigma == 0
		&& b->sigma_hot == 0
		&& !b->open_fp.empty();
}

/**
 * Mark an NBlock as hot, we want this one.
 */
bool NBlockGraph::set_hot(NBlock *b)
{
	fp_type f = b->open_fp.get_best_val();
	bool broadcast = false;

	if (!mutex.try_lock())
		return false;

	if (!b->hot && b->sigma > 0) {
		for (unsigned int i = 0; i < b->ninterferes; i++) {
			assert(b->id != b->interferes[i]);
			NBlock *m = map.get(b->interferes[i]);
			if (m->hot && m->open_fp.get_best_val() < f)
				goto out;
		}

		b->hot = true;
		for (unsigned int i = 0; i < b->ninterferes; i++) {
			assert(b->id != b->interferes[i]);
			NBlock *m = map.get(b->interferes[i]);
			if (is_free(m) && m->fp_pq_index != -1)
				free_list.remove(m->fp_pq_index);
			if (m->hot)
				broadcast |= set_cold(m);
			m->sigma_hot += 1;
		}
	}
out:
	if (broadcast)
		mutex.cond_broadcast();
	mutex.unlock();
	return true;
}

/**
 * Mark an NBlock as cold.  The mutex must be held *before* this
 * function is called.n
 */
bool NBlockGraph::set_cold(NBlock *b)
{
	bool broadcast = false;
	b->hot = false;
	for (unsigned int i = 0; i < b->ninterferes; i++) {
		assert(b->id != b->interferes[i]);
		NBlock *m = map.get(b->interferes[i]);
		m->sigma_hot -= 1;
		if (is_free(m)) {
			free_list.add(m);
			broadcast = true;
		}
	}

	return broadcast;
}

/**
 * We won't release block b, so set all hot blocks in its interference
 * scope back to cold.
 */
void NBlockGraph::wont_release(NBlock *b)
{
	bool broadcast = false;

	// Don't bother locking if nothing is hot anyway.
	if (b->sigma_hot == 0)
		return;

	if (!mutex.try_lock())
		return;

	for (unsigned int i = 0; i < b->ninterferes; i++) {
		NBlock *m = map.find(b->interferes[i]);
		if (!m)
			continue;
		if (m->hot)
			broadcast |= set_cold(m);
	}

	if (broadcast)
		mutex.cond_broadcast();

	mutex.unlock();
}

/**
 * Get the number of nblocks which were actually created.
 */
unsigned int NBlockGraph::get_ncreated_nblocks(void)
{
	return map.get_num_created();;
}
