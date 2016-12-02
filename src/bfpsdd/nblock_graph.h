/* © 2014 the PBNF Authors under the MIT license. See AUTHORS for the list of authors.*/

/**
 * \file nblock_graph.h
 *
 *
 *
 * \author Ethan Burns
 * \date 2008-10-20
 */

#if !defined(_BFPSDD_NBLOCK_GRAPH_H_)
#define _BFPSDD_NBLOCK_GRAPH_H_

#include <assert.h>

#include <iostream>
#include <map>
#include <list>
#include <vector>

#include "../state.h"
#include "../closed_list.h"
#include "../queue_open_list.h"
#include "../open_list.h"
#include "../projection.h"
#include "../util/nblock_map.h"
#include "../util/mutex.h"
#include "nblock.h"

using namespace std;

namespace BFPSDD {

	template<class NBlockPQ, class StateCompare>
		class NBlockGraph
		: NBlockMap<NBlock<StateCompare> >::CreationObserver {
	public:
		enum layer { LAYERA = 0, LAYERB };

		NBlockGraph(const Projection *p, unsigned int nthreads, fp_type multiplier, State *init);

		~NBlockGraph();

		NBlock<StateCompare> *next_nblock(NBlock<StateCompare> *finished);
		NBlock<StateCompare> *get_nblock(unsigned int hash);

		void print(ostream &o);
		unsigned int get_max_assigned_nblocks(void) const;
		unsigned int get_ncreated_nblocks(void);

		fp_type get_layer_value(void) const;

		void observe(NBlock<StateCompare> *b);
		void print_stats(ostream &o);
	private:
		void __print(ostream &o);
		void update_scope_sigmas(unsigned int y, int delta);
		void update_sigma(NBlock<StateCompare> *yblk, int delta);

		/* NBlocks. */
//		map<unsigned int, NBlock<StateCompare> *> blocks;
		NBlockMap<NBlock<StateCompare> > map;

		/* The total number of NBlocks. */
		unsigned int num_nblocks;

		/* The number of NBlocks with sigma values of zero. */
		unsigned int num_sigma_zero;

		/* list of free nblock numbers */
		list<NBlock<StateCompare> *> free_list;

		/* list of all nblocks. */
		list<NBlock<StateCompare> *> nblocks;

		/* prio-queue of NBlocks. */
		NBlockPQ nblock_pq;

		/* The value of this layer */
		fp_type layer_value;

		Mutex mutex;

		/* This flag is set when a thread finds a path so that other
		 * threads do not continue to wait for a new NBlock. */
		bool path_found;


		/*
		 * Statistics
		 */
		unsigned int nblocks_assigned;
		unsigned int nblocks_assigned_max;

		unsigned int nthreads;
		fp_type multiplier;
	};




/**
 * Create a new NBlock graph.
 * \param p The projection function.
 */
	template<class NBlockPQ, class StateCompare>
		NBlockGraph<NBlockPQ, StateCompare>::NBlockGraph(const Projection *p,
								 unsigned int nt,
								 fp_type mult,
								 State *initial)
		: map(p)
	{
		unsigned int init_nblock = p->project(initial);

		map.set_observer(this);
		this->nthreads = nt;
		num_sigma_zero = num_nblocks = p->get_num_nblocks();
		this->multiplier = mult;
		assert(init_nblock < num_nblocks);

		NBlock<StateCompare> *n = map.get(init_nblock);
		n->open.add(initial);
		n->closed.add(initial);
		free_list.push_back(n);
		layer_value = n->open.get_best_val();

		path_found = false;

		nblocks_assigned = 0;
		nblocks_assigned_max = 0;
	}

/**
 * Destroy an NBlock graph.
 */
	template<class NBlockPQ, class StateCompare>
		NBlockGraph<NBlockPQ, StateCompare>::~NBlockGraph()
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
	template<class NBlockPQ, class StateCompare>
		NBlock<StateCompare> *
		NBlockGraph<NBlockPQ, StateCompare>::next_nblock(NBlock<StateCompare> *finished)
	{
		NBlock<StateCompare> *n = NULL;

		mutex.lock();

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
			if (!finished->open.empty())
				nblock_pq.add(finished);

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
				if (nblock_pq.empty())
					layer_value = fp_infinity;
				else
					layer_value = nblock_pq.top()->open.get_best_val();
				unsigned int added = 0;
				while (!nblock_pq.empty()
				       && (added < multiplier*nthreads
					   || nblock_pq.top()->open.get_best_val() == layer_value)) {
					NBlock<StateCompare> *nb = nblock_pq.take();
					added += 1;
					nb->inlayer = true;
					free_list.push_back(nb);
				}

				if (free_list.empty())
					path_found = true;

				// Wake up everyone...
				mutex.cond_broadcast();
			}
		}

		while (free_list.empty() && !path_found)
			mutex.cond_wait();

		if (path_found)
			goto out;

		n = free_list.front();
		n->inuse = true;
		free_list.pop_front();
		nblocks_assigned += 1;
		if (nblocks_assigned > nblocks_assigned_max)
			nblocks_assigned_max = nblocks_assigned;
		update_scope_sigmas(n->id, 1);
	out:
		mutex.unlock();

		return n;
	}


/**
 * Get the NBlock given by the hash value.
 * This shouldn't be called unless the calling thread has this block
 * assigned to it.
 */
	template<class NBlockPQ, class StateCompare>
		NBlock<StateCompare> *NBlockGraph<NBlockPQ, StateCompare>::get_nblock(unsigned int hash)
	{
		return map.get(hash);
	}

/**
 * Get the statistics on the maximum number of NBlocks assigned at one time.
 */
	template<class NBlockPQ, class StateCompare>
		unsigned int
		NBlockGraph<NBlockPQ, StateCompare>::get_max_assigned_nblocks(void) const
	{

		return nblocks_assigned_max;
	}


/**
 * Print an NBlock, but don't take the lock.
 */
	template<class NBlockPQ, class StateCompare>
		void NBlockGraph<NBlockPQ, StateCompare>::__print(ostream &o)
	{
		typename list<NBlock<StateCompare> *>::iterator fiter;

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
	template<class NBlockPQ, class StateCompare>
		void NBlockGraph<NBlockPQ, StateCompare>::print(ostream &o)
	{
		mutex.lock();
		__print(o);
		mutex.unlock();
	}

/**
 * Update the sigma value of a block, add it to the freelist if need
 * be, or remove it from the freelist.
 */
	template<class NBlockPQ, class StateCompare>
		void
		NBlockGraph<NBlockPQ, StateCompare>::update_sigma(NBlock<StateCompare> *yblk,
								  int delta)
	{
		if (yblk->sigma == 0) {
			assert(delta > 0);
			if (!yblk->open.empty())
				free_list.remove(yblk);
			assert(!yblk->inuse);
			nblock_pq.remove(yblk);
			num_sigma_zero -= 1;
		}

		yblk->sigma += delta;

		if (yblk->sigma == 0) {
			if (!yblk->open.empty()) {
				assert(!yblk->inuse);
				if (yblk->open.get_best_val() == layer_value
				    && yblk->inlayer) {
					free_list.push_back(yblk);
					mutex.cond_broadcast();
				} else {
					nblock_pq.add(yblk);
				}
			}
			num_sigma_zero += 1;
		}
	}

/**
 * Update the sigmas by delta for all of the predecessors of y and all
 * of the predecessors of the successors of y.
 */
	template<class NBlockPQ, class StateCompare>
		void
		NBlockGraph<NBlockPQ, StateCompare>::update_scope_sigmas(unsigned int y,
									 int delta)
	{
		typename set<unsigned int>::iterator iter;
		NBlock<StateCompare> *n = map.get(y);

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

	template<class NBlockPQ, class StateCompare>
		fp_type NBlockGraph<NBlockPQ, StateCompare>::get_layer_value(void) const
	{
		return layer_value;
	}

	template<class NBlockPQ, class StateCompare>
	unsigned int NBlockGraph<NBlockPQ, StateCompare>::get_ncreated_nblocks(void)
	{
		return map.get_num_created();
	}

	template<class NBlockPQ, class StateCompare>
	void NBlockGraph<NBlockPQ, StateCompare>::observe(NBlock<StateCompare> *b)
	{
		nblocks.push_back(b);
	}

	template<class NBlockPQ, class StateCompare>
	void NBlockGraph<NBlockPQ, StateCompare>::print_stats(ostream &o)
	{
		//
		// Print open list statistics.
		//
#if defined(QUEUE_SIZES)
		typename list<NBlock<StateCompare>* >::iterator iter;
		unsigned int max = 0;
		double sum = 0;
		double num = 0;
		for (iter = nblocks.begin(); iter != nblocks.end(); iter++) {
			NBlock<StateCompare> *n = *iter;

			if (n->open.get_max_size() > max)
				max = n->open.get_max_size();
			sum += n->open.get_avg_size();
			num += 1;
		}
		if (num > 0) {
			cout << "average-open-size: " << (sum / num) << endl;
			cout << "max-open-size: " << max << endl;
		} else {
			cout << "average-open-size: -1" << endl;
			cout << "max-open-size: -1" << endl;
		}
#endif	/* QUEUE_SIZES */
	}

} /* BFPSDD */

#endif	/* !_BFPSDD_NBLOCK_GRAPH_H_ */
