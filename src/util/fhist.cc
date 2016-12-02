// © 2014 the PBNF Authors under the MIT license. See AUTHORS for the list of authors.

/**
 * \file fhist.cc
 *
 * A histogram of f values.
 *
 * \author eaburns
 * \date 08-09-2010
 */

#include "fhist.h"
#include "mutex.h"
#include "fixed_point.h"

#include <iostream>
#include <vector>

void F_hist::see_f(fp_type f)
{
	unsigned long fint = (unsigned long) DOUBLE(f);

	if (Thread::current())
		mutex.lock();
	if (counts.size() < fint + 1)
		counts.resize(fint + 1);

	counts[fint] += 1;
	if (Thread::current())
		mutex.unlock();
}

void F_hist::output_above(std::ostream &o, fp_type opt)
{
	for (unsigned int i = 0; i < counts.size(); i += 1) {
		if (counts[i] > 0) {
			o << (i / (double) DOUBLE(opt)) << '\t'
			  << counts[i] << std::endl;
		}
	}
}
