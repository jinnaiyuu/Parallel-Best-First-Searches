// © 2014 the PBNF Authors under the MIT license. See AUTHORS for the list of authors.

/**
 * \file thread_test.cc
 *
 * Test the Thread class.
 *
 * \author Ethan Burns
 * \date 2008-10-21
 */

#include <iostream>

#include "thread.h"

using namespace std;

/**
 * An example Thread class.  This class takes two numbers, a unique ID
 * value and a number to count till.  When this thread is started, it
 * counts from 0 to num-1 and prints each number it counts along with
 * its given id value and its pthread thread_id value.
 */
class TestThread : public Thread {
public:
	/**
	 * Create a new test thread class with the given ID and number
	 * to count to.
	 */
	TestThread(int id, int num) : id(id), num(num) {}

	~TestThread() {}

	/**
	 * The run function actually does the work of the thread.
	 */
	virtual void run(void) {
		for (int i = 0; i < num; i += 1) {
			cout << "thread " << get_id()
			     << "(" << id << "): " << i << endl;
		}
	}

private:
	int id;
	int num;
};


int main(void)
{
	TestThread one(1, 1000);
	TestThread two(2, 2000);

	one.start();
	two.start();

	one.join();		// wait for thread `one' to complete
	cout << "one finished" << endl;

	two.join();		// wait for thread `two' to complete
	cout << "two finished" << endl;

	return 0;
}
