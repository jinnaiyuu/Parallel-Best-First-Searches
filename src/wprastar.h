/* © 2014 the PBNF Authors under the MIT license. See AUTHORS for the list of authors.*/

/**
 * \file wprastar.h
 *
 *
 *
 * \author Sofia Lemons
 * \date 2009-03-19
 */

#if !defined(_WPRASTAR_H_)
#define _WPRASTAR_H_

#include <vector>

#include "state.h"
#include "search.h"
#include "pbnf/nblock_graph.h"
#include "pbnf/nblock.h"
#include "util/msg_buffer.h"
#include "util/atomic_int.h"
#include "util/thread.h"
#include "synch_pq_olist.h"
#include "synch_closed_list.h"
#include "util/completion_counter.h"

using namespace std;

class wPRAStar : public Search {
public:
        wPRAStar(unsigned int n_threads, bool dd, bool abst, bool async,
		 unsigned int max_e);
        wPRAStar(unsigned int n_threads,
		 fp_type bound,
		 bool dd,
		 bool abst,
		 bool async,
		 unsigned int max_e);

        virtual ~wPRAStar(void);

        virtual vector<State *> *search(Timer *t, State *init);
        void set_done();
        bool is_done();
        void set_path(vector<State *> *path);

	void set_bound(fp_type bound);

private:
        class wPRAStarThread : public Thread {
        public:
                wPRAStarThread(wPRAStar *p,
			       vector<wPRAStarThread *> *threads,
			       CompletionCounter* cc);
                virtual ~wPRAStarThread(void);
                virtual void run(void);

		vector<State*> *get_queue(void);
		Mutex *get_mutex();
		static void post_send(void *thr);

                State *take(void);


        private:
		/* Flushes the send queues. */
		bool flush_sends(void);

                void flush_receives(bool has_sends);

		void do_async_send(unsigned int dest_tid, State *c);
		void do_sync_send(unsigned int dest_tid, State *c);

		/* sends the state to the appropriate thread (possibly
		 * this thread). */
		void send_state(State *c);

                wPRAStar *p;
                vector<wPRAStarThread *> *threads;

		vector<State *> q;
		Mutex mutex;

		/* The outgoing message queues (allocated lazily). */
		vector<MsgBuffer<State*>* > out_qs;

                bool completed;
                CompletionCounter *cc;

		unsigned int expansions;

                friend class wPRAStar;
                PQOpenList<State::PQOpsFPrime> open;
                ClosedList closed;
		bool q_empty;
        };

        const unsigned int n_threads;
	fp_type weight;
	AtomicInt bound;
        bool done;
	const Projection *project;
	vector<wPRAStarThread *> threads;
	vector<wPRAStarThread *>::iterator iter;

	SolutionStream *solutions;

	bool dd;		/* Use duplicate detection? */
	bool use_abstraction;	/* Use abstraction for hashing? */
	bool async;		/* Asynchronous send/recv? */

	unsigned int max_exp;
};

#endif	/* !_WPRASTAR_H_ */
