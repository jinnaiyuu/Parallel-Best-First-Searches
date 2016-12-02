// © 2014 the PBNF Authors under the MIT license. See AUTHORS for the list of authors.

/*
 * \file atomic_int_test.cc
 *
 * \author Ethan Burns
 * \date 2008-10-22
 */

#include <stdio.h>
#include "atomic_int.h"

using namespace std;

#if defined(_64_BIT_LONG)
#define _64BIT_X "%lx"
#else
#define _64BIT_X "%llx"
#endif

int main(void)
{
  uint64_t ret;
  AtomicInt i(0xffffffff1LL);

  printf("initial i (0xffffffff1): " _64BIT_X "\n", i.read());

  i.inc();
  printf("inc i (0xffffffff2): " _64BIT_X "\n", i.read());

  i.dec();
  printf("dec i (0xffffffff1): " _64BIT_X "\n", i.read());

  i.set(0xfffffff42LL);
  printf("set i (0xfffffff42): " _64BIT_X "\n", i.read());

  i.sub(2);
  printf("sub 2 i (0xfffffff40): " _64BIT_X "\n", i.read());

  i.add(2);
  printf("add 2 i (0xfffffff42): " _64BIT_X "\n", i.read());

  ret = i.swap(0x50);
  printf("swap (50) i ret (0xfffffff42): " _64BIT_X ", " _64BIT_X "\n",
	 i.read(), ret);

  ret = i.cmp_and_swap(0x42, 1);
  printf("bad cmp_and_swap (50) i, ret (50): " _64BIT_X ", " _64BIT_X "\n",
	 i.read(), ret);

  ret = i.cmp_and_swap(0x50, 0xeeeeeeeeLL);
  printf("good cmp_and_swap (0xeeeeeeee) i, ret (50): " _64BIT_X ", " _64BIT_X "\n",
	 i.read(), ret);

  return 0;
}
