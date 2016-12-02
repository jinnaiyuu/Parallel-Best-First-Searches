// © 2014 the PBNF Authors under the MIT license. See AUTHORS for the list of authors.

/**
 * \file timer.cc
 *
 *
 *
 * \author Ethan Burns
 * \date 2008-11-03
 */

#include <sys/time.h>
#include <sys/times.h>
#include <time.h>
#include <unistd.h>

#include "cpu_timer.h"

CPU_timer::CPU_timer(void)
	: processor_start_time(0),
	  processor_elapsed(0),
	  running(false){}


void CPU_timer::start(void)
{
	struct tms tms;

	processor_elapsed = 0;

	times(&tms);
	processor_start_time = tms.tms_utime + tms.tms_stime;
	running = true;
}

double CPU_timer::compute_processor_elapsed(void)
{
	struct tms tms;
	clock_t diff;

	times(&tms);
	diff = (tms.tms_utime + tms.tms_stime) - processor_start_time;

	return (double) diff / sysconf(_SC_CLK_TCK);
}

void CPU_timer::stop(void)
{
	processor_elapsed = compute_processor_elapsed();
	running = false;
}

double CPU_timer::get_time(void)
{
	if (running)
		processor_elapsed = compute_processor_elapsed();

	return processor_elapsed;
}
