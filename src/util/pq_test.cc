// © 2014 the PBNF Authors under the MIT license. See AUTHORS for the list of authors.

/**
 * \file pq_test.c
 *
 *
 *
 * \author Ethan Burns
 * \date 2009-02-11
 */

#include "priority_queue.h"

#include <iostream>

using namespace std;

class Int {
public:
	class IntPQOps {
	public:
		int operator()(Int *a, Int *b)
		{
			return a->value > b->value;
		}
		int get_value(Int *a) {
			return a->value;
		}
		void operator()(Int *a, int ind)
		{
			a->index = ind;
		}
		int operator()(Int *a)
		{
			return a->index;
		}
	};

	Int(int v) {
		value = v;
		index = -1;
	}

	int index;
	int value;
};

void print(PriorityQueue<Int*, Int::IntPQOps> &pq)
{
	for (int i = 0; i < pq.get_fill(); i += 1) {
		cout << "[" << pq.get_vec()[i]->value << "]";
	}
	cout << endl;
}

/*
void print (PriorityQueue<Int*, Int::IntPQOps> &pq) {}
*/


int main(void)
{
	PriorityQueue<Int*, Int::IntPQOps> pq;
	Int *one = new Int(1);
	Int *three = new Int(3);
	Int *four = new Int(4);

	pq.add(one);
	pq.add(new Int(2));
	pq.add(three);
	pq.add(new Int(6));
	pq.add(new Int(8));
	pq.add(new Int(-5));
	pq.add(new Int(-1));
	pq.add(four);

	print(pq);
	cout << "front: " << pq.front()->value << endl;

	cout << endl;

	cout << "remove 4" << endl;
	pq.remove(four->index);
	print(pq);
	cout << "front: " << pq.front()->value << endl;

	cout << endl;

	cout << "take" << endl;
	delete pq.take();
	print(pq);
	cout << "front: " << pq.front()->value << endl;

	cout << endl;


	cout << "change 1 to -2" << endl;
	one->value = -2;
	pq.see_update(one->index);
	print(pq);
	cout << "front: " << pq.front()->value << endl;

	cout << endl;

	cout << "take" << endl;
	delete pq.take();
	print(pq);
	cout << "front: " << pq.front()->value << endl;

	cout << endl;

	cout << "change 3 to 5" << endl;
	three->value = 5;
	pq.see_update(three->index);
	print(pq);
	cout << "front: " << pq.front()->value << endl;

	cout << endl;

	cout << "take" << endl;
	delete pq.take();
	print(pq);
	cout << "front: " << pq.front()->value << endl;

	cout << endl;

	cout << "take" << endl;
	delete pq.take();
	print(pq);
	cout << "front: " << pq.front()->value << endl;

	cout << endl;

	cout << "take" << endl;
	delete pq.take();
	print(pq);
	cout << "front: " << pq.front()->value << endl;

	cout << endl;


	cout << "take" << endl;
	delete pq.take();
	print(pq);
	cout << "front: " << pq.front()->value << endl;

	cout << endl;

	return 0;
}
