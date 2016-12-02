/* © 2014 the PBNF Authors under the MIT license. See AUTHORS for the list of authors.*/

/**
 * \file timer.h
 *
 *
 *
 * \author Ethan Burns
 * \date 2008-11-03
 */

#if !defined(_TIMER_H_)
#define _TIMER_H_

#include <time.h>

class Timer {
public:
	Timer(void);
	void start(void);
	void stop(void);
	double get_wall_time(void);
	double get_processor_time(void);

	/**
	 * Get the elapsed time, but without stopping the timer.
	 * (like on a stop watch... the lap button).
	 */
	double get_lap_time(void);
private:
	double compute_wall_elapsed(void);
	double compute_processor_elapsed(void);

	double wall_start_time;
	clock_t processor_start_time;
	double wall_elapsed;
	double processor_elapsed;
	bool running;
};

#endif	/* !_TIMER_H_ */
