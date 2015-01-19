/* © 2014 the PBNF Authors under the MIT license. See AUTHORS for the list of authors.*/

/**
 * \file prastar.h
 *
 *
 *
 * \author Sofia Lemons
 * \date 2008-11-19
 */

#if !defined(_PRASTAR_MULTIHEAP_H_)
#define _PRASTAR_MULTIHEAP_H_

#include <vector>

#include "state.h"
#include "search.h"
#include "pbnf/nblock_graph.h"
#include "pbnf/nblock.h"
#include "util/mutex.h"
#include "util/msg_buffer.h"
#include "util/atomic_int.h"
#include "util/thread.h"
#include "synch_pq_olist.h"
#include "synch_closed_list.h"
#include "util/completion_counter.h"

#include "pq_multiheap_open_list.h"

using namespace std;

class PRAStarMultiheap: public Search {
public:
	PRAStarMultiheap(unsigned int n_threads, bool use_abst, bool async_send,
			bool async_recv, unsigned int max_exp, unsigned int heap_per_threads);

	virtual ~PRAStarMultiheap(void);

	virtual vector<State *> *search(Timer *t, State *init);
	void set_done();
	bool is_done();
	void set_path(vector<State *> *path);
	bool has_path();

	virtual void output_stats(void);

private:
	class PRAStarMultiheapThread: public Thread {
	public:
		PRAStarMultiheapThread(PRAStarMultiheap *p, vector<PRAStarMultiheapThread *> *threads,
				CompletionCounter* cc);
		virtual ~PRAStarMultiheapThread(void);
		virtual void run(void);

		/**
		 * Gets a pointer to the message queue.
		 */
		vector<State*> *get_queue(void);

		/**
		 * Gets the lock on the message queue.
		 */
		Mutex *get_mutex(void);

		/**
		 * Should be called when the message queue has had
		 * things added to it.
		 */
		static void post_send(void *thr);

		State *take(void);

	private:
		/* Flushes the send queues. */
		bool flush_sends(void);

		/* flushes the queue into the open list. */
		void flush_receives(bool has_sends);

		/* sends the state to the appropriate thread (possibly
		 * this thread). */
		void send_state(State *c);

		/* Perform an asynchronous send to another thread (not called
		 * for self-sends). */
		void do_async_send(unsigned int dest_tid, State *c);

		/* Perform an synchronous send to another thread (not called
		 * for self-sends). */
		void do_sync_send(unsigned int dest_tid, State *c);

		PRAStarMultiheap *p;

		/* List of the other threads */
		vector<PRAStarMultiheapThread *> *threads;

		/* The incoming message queue. */
		vector<State *> q;
		Mutex mutex;

		/* The outgoing message queues (allocated lazily). */
		vector<MsgBuffer<State*>*> out_qs;

		/* Marks whether or not this thread has posted completion */
		bool completed;

		/* expansions between flushes of the send queue. */
		unsigned int expansions;

		/* A pointer to the completion counter. */
		CompletionCounter *cc;

		friend class PRAStarMultiheap;

		/* Search queues */
		// TODO: Implement this multiheap.
		PQMultiheapOpenList<State::PQOpsFPrime> open;

		ClosedList closed;
		bool q_empty;

		/* statistics */
		double time_spinning;

		unsigned int total_expansion;
	};

	bool done;
	pthread_cond_t cond;
	const unsigned int n_threads;
	AtomicInt bound;
	const Projection *project;
	vector<PRAStarMultiheapThread *> threads;
	typename vector<PRAStarMultiheapThread *>::iterator iter;

	SolutionStream *solutions;

	/* true: use abstraction for hashing, false: use basic
	 * hashing. */
	bool use_abstraction;

	/* Level of asynchronousity. */
	bool async_send;
	bool async_recv;

	/* Max expansions before flushing the send queue.  If we are
	 * doing synchronous sends, this will be zero. */
	unsigned int max_exp;

	/* Statistics */
	double time_spinning;
	double max_spinning;
	double avg_open_size;
	unsigned int max_open_size;

	unsigned heap_per_thread;

#if defined(COUNT_FS)
	static F_hist fs;
#endif	/* COUNT_FS */
};

#endif	/* !_PRASTAR_H_ */
