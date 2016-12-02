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

#include "timer.h"

Timer::Timer(void)
	: wall_start_time(0),
	  processor_start_time(0),
	  wall_elapsed(0),
	  processor_elapsed(0),
	  running(false){}


void Timer::start(void)
{
	struct timeval t;
	struct tms tms;

	wall_elapsed = processor_elapsed = 0;

	times(&tms);
	processor_start_time = tms.tms_utime + tms.tms_stime;

	gettimeofday(&t, NULL);
	wall_start_time = t.tv_usec;
	wall_start_time /= 1000000;
	wall_start_time += t.tv_sec;

	running = true;
}

double Timer::get_lap_time(void)
{
	return compute_wall_elapsed();
}

double Timer::compute_wall_elapsed(void)
{
	struct timeval t;
	double stop;

	gettimeofday(&t, NULL);
	stop = t.tv_usec;
	stop /= 1000000;
	stop += t.tv_sec;

	return stop - wall_start_time;
}

double Timer::compute_processor_elapsed(void)
{
	struct tms tms;
	clock_t diff;

	times(&tms);
	diff = (tms.tms_utime + tms.tms_stime) - processor_start_time;

	return (double) diff / sysconf(_SC_CLK_TCK);
}

void Timer::stop(void)
{
	wall_elapsed = compute_wall_elapsed();
	processor_elapsed = compute_processor_elapsed();
	running = false;
}

double Timer::get_wall_time(void)
{
	if (running)
		wall_elapsed = compute_wall_elapsed();

	return wall_elapsed;
}

double Timer::get_processor_time(void)
{
	if (running)
		processor_elapsed = compute_processor_elapsed();

	return processor_elapsed;
}
