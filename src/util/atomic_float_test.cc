// © 2014 the PBNF Authors under the MIT license. See AUTHORS for the list of authors.

/**
 * \file atomic_int_test.cc
 *
 *
 *
 * \author Ethan Burns
 * \date 2008-11-10
 */

#include <float.h>
#include <stdlib.h>

#include <iostream>

#include "atomic_float.h"

using namespace std;

int main(void)
{
	AtomicFloat f(4.2);

	cout << "4.2: " << f.read() << endl;
	f.set(12312.123123);
	cout << "12312.123123: " << f.read() << endl;
	f.set(FLT_MAX);
	cout << FLT_MAX << ": " << f.read() << endl;;

	return EXIT_SUCCESS;
}
