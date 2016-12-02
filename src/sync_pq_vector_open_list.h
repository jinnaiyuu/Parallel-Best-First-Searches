/* © 2014 the PBNF Authors under the MIT license. See AUTHORS for the list of authors.*/

/**
 * \file pq_open_list.h
 *
 * An open list class.
 *
 * \author Ethan Burns
 * \date 2008-10-09
 */

#if !defined(_SYNC_PQ_VECTOR_OPEN_LIST_H_)
#define _SYNC_PQ_VECTOR_OPEN_LIST_H_

#include <assert.h>

#include <list>
#include <limits>
#include <pthread.h>


#include "state.h"
#include "open_list.h"
#include "util/priority_queue.h"
#include "util/cpu_timer.h"

using namespace std;

#if defined(TIME_QUEUES)
#define start_queue_timer() do { t.start(); } while (0)
#define stop_queue_timer()				\
	do {						\
		t.stop();				\
		time_count += 1;			\
		cpu_time_sum += t.get_time(); \
	} while (0)
#else
#define start_queue_timer()
#define stop_queue_timer()
#endif // TIME_QUEUES


/**
 * A priority queue for states based on their f(s) = g(s) + h(s)
 * value.
 *
 * \todo make this a bit more general.
 */
class SyncPQVectorOpenList : public OpenList {

		struct Maxq {
			Maxq(void) : fill(0), max(0) { }

			// p is h value here.
			void push(State *n, int p) {
//				printf("Maxq::push\n");
				assert (p >= 0);
				if (bins.size() <= (unsigned int) p)
					bins.resize(p+1);

				if (p > max)
					max = p;

				// TODO: What is this openind for?
				// If you put p to 0, No h tie-breaking.
//				n->openind = bins[p].size();
				bins[p].push_back(n);
				fill++;
			}

			State *pop(void) {
//				printf("Maxq::pop\n");
				for ( ; bins[max].empty(); max--) {
					if (max == 0) {
						printf("max==0\n");
						break;
					}
				}
//				dbgprintf("g = %d\n", max);
				State *n = bins[max].back();
				bins[max].pop_back();
	//			n->openind = -1;
				fill--;
				return n;
			}
			bool empty(void) { return fill == 0; }

			int getsize() {
				int sum = 0;
				for (unsigned int i = 0; i < bins.size(); ++i) {
					sum += bins[i].size();
				}
				return sum;
			}
			int fill, max;
			std::vector< std::vector<State*> > bins;
		};

public:
	SyncPQVectorOpenList(void);
	void changeSize(unsigned int size);
	void add(State *s);
	State *take(void);
	State *peek(void);
	bool empty(void);
	void delete_all_states(void);
	void prune(void);

	list<State*> *states(void);

	unsigned int size(void);
	void remove(State *s);
	void see_update(State *s);
	void resort();

	/* Verify the heap property holds */
	void verify();

#if defined(TIME_QUEUES)
	double get_cpu_sum(void) { return cpu_time_sum; }
	unsigned long get_time_count(void) { return time_count; }
#endif
private:
//	PriorityQueue<State *, PQCompare> pq;
	int fill, min;
	std::vector<Maxq> pq;
	pthread_mutex_t mutex;

//	PQCompare get_index;
//	PQCompare comp;

#if defined(TIME_QUEUES)
	CPU_timer t;
	double cpu_time_sum;
	unsigned long time_count;
#endif
};


#endif	/* !_PQ_OPEN_LIST_H_ */
