// © 2014 the PBNF Authors under the MIT license. See AUTHORS for the list of authors.

/**
 * \file cumulative_ave.cc
 *
 *
 *
 * \author Ethan Burns
 * \date 2008-12-24
 */

#include "cumulative_ave.h"

CumulativeAverage::CumulativeAverage(void)
{
	reset();
}

/**
 * Add a value to the running average.
 */
void CumulativeAverage::add_val(unsigned long val)
{
	// from Wikipedia "moving average"
	ave = ave + ((val - ave) / (num + 1));
	num += 1;
}

/**
 * Get the number of entries
 */
unsigned long CumulativeAverage::get_num(void)
{
	return num;
}

/**
 * Read the average
 */
float CumulativeAverage::read(void)
{
	return ave;
}


void CumulativeAverage::reset(void)
{
	ave = 0;
	num = 0;
}
