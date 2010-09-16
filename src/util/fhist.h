/**
 * \file fhist.h
 *
 * A simple histogram of integer valued f values.
 *
 * \author eaburns
 * \date 08-09-2010
 */

#include "fixed_point.h"
#include "mutex.h"

#include <fstream>
#include <vector>

class F_hist {
public:
	/*
	 * Add a count of 1 to the histogram for 'f'
	 */
	void see_f(fp_type f);

	/*
	 * Output counts for all values above 'opt' to 'o'.
	 */
	void output_above(std::ostream &o, fp_type opt);

private:
	std::vector<unsigned long> counts;

	Mutex mutex;
};