/* © 2014 the PBNF Authors under the MIT license. See AUTHORS for the list of authors.*/

/**
 * \file thread_specific.h
 *
 * A thread specific type.
 *
 * \author eaburns
 * \date 2009-07-23
 */

#if !defined(_THREAD_SPECIFIC_H_)
#define _THREAD_SPECIFIC_H_

#include <limits.h>

#include <vector>
using namespace std;

#include "thread.h"

/* To attempt to prevent cache ping-ponging? */
#define PADDING 24 		/* bytes */


template <class T>
class ThreadSpecific {
public:
	ThreadSpecific(T iv)
	{
		struct padded_entry init_val;
		init_val.data = iv;
		entries.resize(_POSIX_THREAD_THREADS_MAX, init_val);
	}

	T get_value(void)
	{
		unsigned int tid = Thread::current()->get_id();
		return entries[tid].data;
	}

	T get_value_for(unsigned int tid)
	{
		return entries[tid].data;
	}

	void set_value(T v)
	{
		unsigned int tid = Thread::current()->get_id();
		entries[tid].data = v;
	}

	/**
	 * Get all entries.
	 */
	vector<T> get_all_entries(void)
	{
		typename vector<struct padded_entry>::iterator iter;
		vector<T> ret;

		for (iter = entries.begin(); iter != entries.end(); iter++)
			ret.push_back((*iter).data);

		return ret;
	}

private:
	/* We want to try to get each element of the array on a
	 * seperate cache line. */
	struct padded_entry {
		T data;
		char padding[PADDING];
	};

	vector<struct padded_entry> entries;
};

#endif	/* !_THREAD_SPECIFIC_H_ */
