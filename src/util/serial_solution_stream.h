/* © 2014 the PBNF Authors under the MIT license. See AUTHORS for the list of authors.*/

/**
 * \file serial_solution_stream.h
 *
 *
 *
 * \author eaburns
 * \date 2009-04-20
 */
#if !defined(_SERIAL_SOLUTION_STREAM_H_)
#define _SERIAL_SOLUTION_STREAM_H_

#include "../state.h"
#include "solution_stream.h"
#include "timer.h"
#include "fixed_point.h"

using namespace std;

/**
 * Holds a stream of solutions along with the time at which the
 * solution was arrived.
 */
class SerialSolutionStream : public SolutionStream {
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
	SerialSolutionStream(Timer *t, double g);

	/**
	 * The destructor does nothing, but this gets rid of a
	 * warning.
	 */
	~SerialSolutionStream() {}

	/**
	 * See an incumbent solution.
	 *
	 * \param gen Nodes generated at the time the solution was found.
	 *
	 * \param exp Nodes expanded at the time the solution was found.
	 */
	void see_solution(vector<State *> *path,
				  unsigned int gen,
				  unsigned int exp);

	/**
	 * Get the best solution path.
	 */
	vector<State *> *get_best_path(void);

	/**
	 * Output the solution stream to the given output stream.
	 */
	void output(ostream &o);

private:
	Solution *best;
	Solution *lst;

	bool solution_found;
};

#endif /* !_SERIAL_SOLUTION_STREAM_H_ */
