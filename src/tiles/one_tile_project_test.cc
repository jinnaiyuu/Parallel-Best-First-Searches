// © 2014 the PBNF Authors under the MIT license. See AUTHORS for the list of authors.

/**
 * \file one_tile_project_test.cc
 *
 *
 *
 * \author Ethan Burns
 * \date 2008-11-18
 */

#include <math.h>

#include <iostream>

#include "tiles.h"

using namespace std;

int main(void)
{
	vector<unsigned int>::iterator iter;
	vector<unsigned int> succs;

	Tiles t(cin);
	Tiles::OneTileProject p(&t);

	cout << "nnblocks: " << p.get_num_nblocks() << endl;
	unsigned int id = t.get_width()
		* t.get_height()
		* t.get_width()
		+ t.get_width()
		+ t.get_height();

	p.print(id, cout);
	succs = p.get_successors(id);

	cout << "Successors: " << endl;
	for (iter = succs.begin(); iter != succs.end(); iter++) {
		cout << '\t';
		p.print(*iter, cout);
	}
}
