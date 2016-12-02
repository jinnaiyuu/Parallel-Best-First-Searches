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

#include <iostream>
#include <list>
#include <map>
#include <vector>

#include "../closed_list.h"
#include "../queue_open_list.h"
#include "../open_list.h"
#include "../projection.h"
#include "nblock.h"
#include "nblock_graph.h"

using namespace std;
using namespace PSDD;

/**
 * Create a new NBlock graph.
 * \param p The projection function.
 */
NBlockGraph::NBlockGraph(const Projection *p, State *initial)
	: map(p)
{
	unsigned int init_nblock = p->project(initial);

	layer = LAYERA;

	num_sigma_zero = num_nblocks = p->get_num_nblocks();
	assert(init_nblock < num_nblocks);

	NBlock *n = map.get(init_nblock);
	n->open[layer].add(initial);
	n->closed.add(initial);
	free_list[layer].push_back(n);

	pthread_mutex_init(&mutex, NULL);
	pthread_cond_init(&cond, NULL);
	path_found = false;

	nblocks_assigned = 0;
	nblocks_assigned_max = 0;
}

/**
 * Destroy an NBlock graph.
 */
NBlockGraph::~NBlockGraph() {}


/**
 * Get the next nblock for expansion, possibly release a finished
 * nblock.
 * \note This call will block if there are currently no free nblocks.
 * \param finished If non-NULL, the finished nblock will be released
 *        into the next level's free_list.
 * \return The next NBlock to expand or NULL if there is nothing left
 *         to do.
 */
NBlock *NBlockGraph::next_nblock(NBlock *finished)
{
	NBlock *n = NULL;

	pthread_mutex_lock(&mutex);

	if (finished) {		// Release an NBlock
		if (finished->sigma != 0) {
			cerr << "A proc had an NBlock with sigma != 0" << endl;
			finished->print(cerr);
			cerr << endl << endl << endl;
			__print(cerr);
		}
		assert(finished->sigma == 0);

		nblocks_assigned -= 1;
		update_scope_sigmas(finished->id, -1);
		finished->inuse = false;

		if (!finished->open[get_next_layer()].empty())
			free_list[get_next_layer()].push_back(finished);

		if (free_list[layer].size() == 0
		    && num_sigma_zero == num_nblocks) {
			// If there are no NBlocks that are currently
			// free with nodes on them, and all of the
			// NBlocks have sigma values of zero:
			// Switch to the next iteration

			// Switch layers
			layer = get_next_layer();
			if (free_list[layer].empty()) {
				path_found = true;
				pthread_cond_broadcast(&cond);
				goto out;
			}

			// Wake up everyone...
			pthread_cond_broadcast(&cond);
		}
	}

	while (free_list[layer].empty() && !path_found)
		pthread_cond_wait(&cond, &mutex);

	if (path_found)
		goto out;

	n = free_list[layer].front();
	free_list[layer].pop_front();
	nblocks_assigned += 1;
	assert(n->sigma == 0);
	assert(!n->open[layer].empty());
	assert(!n->inuse);
	n->inuse = true;

	if (nblocks_assigned > nblocks_assigned_max)
		nblocks_assigned_max = nblocks_assigned;
	update_scope_sigmas(n->id, 1);

out:
	pthread_mutex_unlock(&mutex);

	return n;
}


/**
 * Get the NBlock given by the hash value.
 * This shouldn't be called unless the calling thread has this block
 * assigned to it.
 */
NBlock *NBlockGraph::get_nblock(unsigned int hash)
{
	return map.get(hash);
}


/**
 * Signal anyone else that is waiting that a path has been found and
 * there is no need to get a new NBlock.
 */
void NBlockGraph::set_path_found(void)
{
	pthread_mutex_lock(&mutex);
	path_found = true;
	pthread_cond_broadcast(&cond);
	pthread_mutex_unlock(&mutex);
}


/**
 * Get the statistics on the maximum number of NBlocks assigned at one time.
 */
unsigned int NBlockGraph::get_max_assigned_nblocks(void) const
{

	return nblocks_assigned_max;
}


/**
 * Print an NBlock, but don't take the lock.
 */
void NBlockGraph::__print(ostream &o)
{
	list<NBlock *>::iterator fiter;

	o << "Number of NBlocks: " << num_nblocks << endl;
	o << "Number of NBlocks with sigma zero: " << num_sigma_zero << endl;
	o << "--------------------" << endl;
	o << "Current Free Blocks:" << endl;
	for (fiter = free_list[layer].begin();
	     fiter != free_list[layer].end();
	     fiter++)
		(*fiter)->print(o);
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
 * Update the sigma value of a block, add it to the freelist if need
 * be, or remove it from the freelist.
 */
void NBlockGraph::update_sigma(NBlock *yblk, int delta)
{
	if (yblk->sigma == 0) {
		assert(delta > 0);
		if (!yblk->open[layer].empty())
			free_list[layer].remove(yblk);
		else if (!yblk->open[get_next_layer()].empty())
			free_list[get_next_layer()].remove(yblk);
		num_sigma_zero -= 1;
	}

	yblk->sigma += delta;

	if (yblk->sigma == 0) {
		if (!yblk->open[layer].empty()) {
			free_list[layer].push_back(yblk);
			pthread_cond_signal(&cond);
		} else if (!yblk->open[get_next_layer()].empty()) {
			free_list[get_next_layer()].push_back(yblk);
			pthread_cond_signal(&cond);
		}

		num_sigma_zero += 1;
	}
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
		assert(*iter != n->id);
		update_sigma(map.get(*iter), delta);
	}
}

enum NBlockGraph::layer NBlockGraph::get_next_layer(void) const
{
	return layer == LAYERA ? LAYERB : LAYERA;
}

enum NBlockGraph::layer NBlockGraph::get_cur_layer(void) const
{
	return layer;
}

/**
 * Remove all old states.
 */
void NBlockGraph::reset(const Projection *p, State *initial)
{
	unsigned int init_nblock = p->project(initial);

	free_list[0].clear();
	free_list[1].clear();
	num_sigma_zero = num_nblocks;
	layer = NBlockGraph::LAYERA;
	path_found = false;
	nblocks_assigned = 0;

	for (unsigned int i = 0; i < num_nblocks; i += 1) {
		NBlock *n = map.find(i);
		if (n) {
			n->closed.delete_all_states();
			n->open[0].delete_all_states();
			n->open[1].delete_all_states();
			n->inuse = false;
			n->sigma = 0;
			if (i == init_nblock) {
				n->open[layer].add(initial);
				n->closed.add(initial);

				free_list[layer].push_back(n);
			}
		}
	}
}

unsigned int NBlockGraph::get_ncreated_nblocks(void)
{
	return map.get_num_created();
}
