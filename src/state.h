/* © 2014 the PBNF Authors under the MIT license. See AUTHORS for the list of authors.*/

/**
 * \file state.h
 *
 *
 *
 * \author Ethan Burns
 * \date 2008-10-08
 */
#if !defined(_STATE_H_)
#define _STATE_H_

#include <iostream>
#include <vector>

#include "util/fixed_point.h"
#include "search_domain.h"

using namespace std;

/**
 * An abstract search state class.
 */
class State {
public:

	class PQOpsF {
	public:
		/* The predecessor operator. */
		int inline operator()(State *a, State *b) const {
			fp_type fa = a->get_f();
			fp_type fb = b->get_f();

			if (fa < fb)
				return true;
			else if (fa > fb)
				return false;
			else {
				fp_type ga = a->get_g();
				fp_type gb = b->get_g();

				return ga > gb;
			}
		}
		fp_type inline get_value(State *s) const {
			return s->get_f();
		}
		void inline operator()(State *a, int i) {
			a->f_pq_index = i;
		}

		int inline operator()(State *a) {
			return a->f_pq_index;
		}
	};

	class PQOpsFPrime {
	public:
		int inline operator()(State *a, State *b) const {
			fp_type fa = a->get_f_prime();
			fp_type fb = b->get_f_prime();
			if (fa < fb)
				return true;
			else if (fa > fb)
				return false;
			else
				return f_cmp(a, b);
		}

		fp_type inline get_value(State *s) const {
			return s->get_f_prime();
		}
		void inline operator()(State *a, int i) {
			a->f_prime_pq_index = i;
		}

		int inline operator()(State *a) {
			return a->f_prime_pq_index;
		}
	private:
		PQOpsF f_cmp;
	};

	class PQOpsFPrimeSilviaTieBreaking {
		/** This class is the prio-queue operations for a
		 * queue sorted on f' which uses Silvia Richter's
		 * tie-breaking rules. */
	public:
		int inline operator()(State *a, State *b) const {
			fp_type fa = a->get_f_prime();
			fp_type fb = b->get_f_prime();
			if (fa < fb)
				return true;
			else if (fa > fb)
				return false;
			else
				/* If there is a tie then [a], the new
				 * element, is not the predecessor. */
				return false;
		}

		fp_type inline get_value(State *s) const {
			return s->get_f_prime();
		}
		void inline operator()(State *a, int i) {
			a->f_prime_pq_index = i;
		}

		int inline operator()(State *a) {
			return a->f_prime_pq_index;
		}
	private:
//		PQOpsF f_cmp;
	};

	class PQOpsFPrimeGreaterFTieBreaking {
		/** Tie breaking on the greater f-value being
		 * preferred.  This is counter-intuitive, but Silvia
		 * thinks it may make our search perform like hers. */
	public:
		int inline operator()(State *a, State *b) const {
			fp_type fa = a->get_f_prime();
			fp_type fb = b->get_f_prime();
			if (fa < fb)
				return true;
			else if (fa > fb)
				return false;
			else
				return !f_cmp(a, b);
		}

		fp_type inline get_value(State *s) const {
			return s->get_f_prime();
		}
		void inline operator()(State *a, int i) {
			a->f_prime_pq_index = i;
		}

		int inline operator()(State *a) {
			return a->f_prime_pq_index;
		}
	private:
		PQOpsF f_cmp;
	};

	State(SearchDomain *d, State *parent, fp_type c, fp_type g);

	virtual ~State();

	virtual SearchDomain *get_domain(void) const;

	virtual uint64_t hash(void) const = 0;
	virtual bool is_goal(void) = 0;
	virtual State *clone(void) const = 0;
	virtual void print(ostream &o) const = 0;
	virtual bool equals(State *s) const = 0;

	virtual vector<State*> *expand(void);
	virtual vector<State*> *expand(int thread_id);

	fp_type get_f(void) const;
	fp_type get_f_prime(void) const;
	fp_type get_c(void) const;
	fp_type get_g(void) const;
	void update(State *parent, fp_type c, fp_type g);
	fp_type get_h(void) const;
	State *get_parent(void) const;
	vector<State *> *get_path(void);
	void set_open(bool b);
	bool is_open(void) const;
	bool is_incons(void) const;
//protected:
	State *parent;
	SearchDomain *domain;
	fp_type c;
	fp_type g;
	fp_type h;
	bool open;
	bool incons;

	/* indexes into the pq open lists. */
	int f_pq_index;
	int f_prime_pq_index;

	virtual unsigned int dist_hash(int dist, int n_threads) = 0;
};

#endif	/* !_STATE_H_ */
