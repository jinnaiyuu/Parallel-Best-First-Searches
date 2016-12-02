/* © 2014 the PBNF Authors under the MIT license. See AUTHORS for the list of authors.*/

/**
 * \file solution_stream.h
 *
 *
 *
 * \author eaburns
 * \date 2009-04-14
 */

#if !defined(_SOLUTION_STREAM_H_)
#define _SOLUTION_STREAM_H_

#include "../state.h"
#include "timer.h"
#include "fixed_point.h"

using namespace std;

/**
 * Holds a stream of solutions along with the time at which the
 * solution was arrived.
 */
class SolutionStream {
public:
	/**
	 * Create a new solution stream that uses the given timer and
	 * that tracks solutions that are at least the given
	 * granularity better than the previous solution.
	 *
	 * \param g The solution granularity.  A new solution is added
	 *          to the stream if it is at least [g] times better
	 *          than the current best solution.
	 */
	SolutionStream(Timer *t, double g);

	/**
	 * The destructor does nothing, but this gets rid of a
	 * warning.
	 */
	virtual ~SolutionStream() {}

	/**
	 * See an incumbent solution.
	 *
	 * \param gen Nodes generated at the time the solution was found.
	 *
	 * \param exp Nodes expanded at the time the solution was found.
	 */
	virtual void see_solution(vector<State *> *path,
				  unsigned int gen,
				  unsigned int exp) = 0;

	/**
	 * Get the best solution path.
	 */
	virtual vector<State *> *get_best_path(void) = 0;

	/**
	 * Output the solution stream to the given output stream.
	 */
	virtual void output(ostream &o) = 0;

protected:

	/**
	 * A solution cost and time pair.
	 */
	struct Solution {
		Solution(vector<State *> *p, unsigned int gen,
			 unsigned int exp, double t)
			{
				path = p;
				generated = gen;
				expanded = exp;
				time = t;
				next = NULL;
			}
		vector<State *> *path;
		unsigned int generated;
		unsigned int expanded;
		double time;

		Solution *next;
	};


	void do_output_recur(ostream &o, Solution *s);
	void do_output(ostream &o, Solution *s);

	bool better(vector<State *> *path, Solution *cur);
	bool within_gran(vector<State *> *path, Solution *cur);

	/** The search timer */
	Timer *timer;

	/** The solution granularity. */
	double granularity;
};

#endif /* !_SOLUTION_STREAM_H_ */
