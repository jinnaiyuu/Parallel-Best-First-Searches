// © 2014 the PBNF Authors under the MIT license. See AUTHORS for the list of authors.

/**
 * \file nblock_graph.cc
 *
 *
 *
 * \author eaburns
 * \date 2009-03-13
 */

#include "nblock.h"
#include "nblock_graph.h"

using namespace std;
using namespace WBFPSDD;

/**
 * Create a new NBlock graph.
 * \param p The projection function.
 */
NBlockGraph::NBlockGraph(const Projection *p,
			 unsigned int nt,
			 fp_type mult,
			 State *initial)
	: map(p)
{
	unsigned int init_nblock = p->project(initial);

	this->nthreads = nt;
	num_sigma_zero = num_nblocks = p->get_num_nblocks();
	this->multiplier = mult;
	assert(init_nblock < num_nblocks);

	//map.set_observer(this);

	NBlock *n = map.get(init_nblock);
	n->open_fp.add(initial);
	n->closed.add(initial);
	free_list.push_back(n);
	layer_value = n->open_fp.get_best_val();

	pthread_mutex_init(&mutex, NULL);
	pthread_cond_init(&cond, NULL);
	done = false;

	nblocks_assigned = 0;
	nblocks_assigned_max = 0;
}

/**
 * Destroy an NBlock graph.
 */
NBlockGraph::~NBlockGraph()
{}


/**
 * Get the next nblock for expansion, possibly release a finished
 * nblock.
 * \note This call will block if there are currently no free nblocks.
 * \param finished If non-NULL, the finished nblock will be released
 *        into the next level's free_list.
 * \return The next NBlock to expand or NULL if there is nothing left
 *         to do.
 */
NBlock *
NBlockGraph::next_nblock(NBlock *finished)
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
		finished->inlayer = false;
		if (!finished->open_fp.empty())
			nblock_pq_fp.add(finished);

		update_scope_sigmas(finished->id, -1);
		finished->inuse = false;

		if (free_list.size() == 0
		    && num_sigma_zero == num_nblocks) {
			// If there are no NBlocks that are currently
			// free with nodes on them, and all of the
			// NBlocks have sigma values of zero:
			// Switch to the next iteration


			// Switch layers -- fill the freelist with the
			// nblocks that have the next layer's value
			if (nblock_pq_fp.empty())
				layer_value = fp_infinity;
			else
				layer_value = nblock_pq_fp.front()->open_fp.get_best_val();
			unsigned int added = 0;
			while (!nblock_pq_fp.empty()
			       && (added < multiplier*nthreads
				   || nblock_pq_fp.front()->open_fp.get_best_val() == layer_value)) {
				NBlock *nb = nblock_pq_fp.take();
				added += 1;
				nb->inlayer = true;
				free_list.push_back(nb);
			}

			if (free_list.empty())
				done = true;

			// Wake up everyone...
			pthread_cond_broadcast(&cond);
		}
	}

	while (free_list.empty() && !done)
		pthread_cond_wait(&cond, &mutex);

	if (done)
		goto out;

	n = free_list.front();
	n->inuse = true;
	free_list.pop_front();
	nblocks_assigned += 1;
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
 * Get the statistics on the maximum number of NBlocks assigned at one time.
 */
unsigned int
NBlockGraph::get_max_assigned_nblocks(void) const
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
	for (fiter = free_list.begin();
	     fiter != free_list.end();
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
void
NBlockGraph::update_sigma(NBlock *yblk,
			  int delta)
{
	if (yblk->sigma == 0) {
		assert(delta > 0);
		if (!yblk->open_fp.empty())
			free_list.remove(yblk);
		assert(!yblk->inuse);
		if (yblk->fp_pq_index != -1)
			nblock_pq_fp.remove(yblk->fp_pq_index);
		num_sigma_zero -= 1;
	}

	yblk->sigma += delta;

	if (yblk->sigma == 0) {
		if (!yblk->open_fp.empty()) {
			assert(!yblk->inuse);
			if (yblk->open_fp.get_best_val() == layer_value
			    && yblk->inlayer) {
				free_list.push_back(yblk);
				pthread_cond_signal(&cond);
			} else {
				nblock_pq_fp.add(yblk);
			}
		}

		num_sigma_zero += 1;
	}
}

/**
 * Update the sigmas by delta for all of the predecessors of y and all
 * of the predecessors of the successors of y.
 */
void
NBlockGraph::update_scope_sigmas(unsigned int y,
				 int delta)
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
		update_sigma(map.get(*iter), delta);
	}
}


// Get the value of the current layer.
fp_type NBlockGraph::get_layer_value(void) const
{
	return layer_value;
}


// Get the number of nblocks that were actually created.
unsigned int NBlockGraph::get_ncreated_nblocks(void)
{
	return map.get_num_created();
}

void NBlockGraph::set_done(void)
{
	done = true;
	pthread_cond_broadcast(&cond);
}
