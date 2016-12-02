// © 2014 the PBNF Authors under the MIT license. See AUTHORS for the list of authors.

/**
 * \file open_list.cc
 *
 *
 *
 * \author Ethan Burns
 * \date 2008-10-13
 */

#include "open_list.h"

OpenList::OpenList()
{
	best.set(fp_infinity);
	cur_size = 0;
	max_size = 0;
}

OpenList::~OpenList() {}

/**
 * Track the best open state.
 */
void OpenList::set_best_val(fp_type f)
{
	best.set(f);
}

fp_type OpenList::get_best_val(void)
{
	return best.read();
}

unsigned int OpenList::size()
{
	return cur_size;
}

void OpenList::set_size(unsigned int s)
{
#if defined(QUEUE_SIZES)
	cur_size = s;
	if (s > max_size)
		max_size = s;
	sum += s;
	num += 1;
#endif	// QUEUE_SIZES
}

void OpenList::change_size(unsigned int delta)
{
#if defined(QUEUE_SIZES)
	cur_size += delta;
	if (cur_size > max_size)
		max_size = cur_size;
	sum += cur_size;
	num += 1;
#endif	// QUEUE_SIZES
}

#if defined(QUEUE_SIZES)
void OpenList::print_stats(ostream &o)
{
	o << "average-open-size: " << sum / num << endl;
	o << "max-open-size: " << sum / num << endl;
}

unsigned int OpenList::get_max_size()
{
	return max_size;
}

#else  // QUEUE_SIZES
void OpenList::print_stats(ostream &o)
{
}
#endif	// QUEUE_SIZES
