/* © 2014 the PBNF Authors under the MIT license. See AUTHORS for the list of authors.*/

/**
 * \file timer.h
 *
 * Just computes CPU time
 *
 * \author Ethan Burns
 * \date 2008-11-03
 */

#if !defined(_CPU_TIMER_H_)
#define _CPU_TIMER_H_

#include <time.h>

class CPU_timer {
public:
	CPU_timer(void);
	void start(void);
	void stop(void);
	double get_time(void);
private:
	double compute_processor_elapsed(void);

	clock_t processor_start_time;
	double processor_elapsed;
	bool running;
};

#endif	/* !_CPU_TIMER_H_ */
