// © 2014 the PBNF Authors under the MIT license. See AUTHORS for the list of authors.

/**
 * \file div_merge_project_test.cc
 *
 *
 *
 * \author Ethan Burns
 * \date 2009-01-09
 */

#include <math.h>

#include <iostream>

#include "../div_merge_project.h"
#include "tiles.h"

using namespace std;

int main(void)
{
	vector<unsigned int>::iterator iter;
	vector<unsigned int> succs;

	Tiles t(cin);
	Tiles::OneTileProject one_tile(&t);
	DivMergeProject p(2, &one_tile);

	cout << "nnblocks: " << p.get_num_nblocks() << endl;

	cout << "Successors: " << endl;
	for (iter = succs.begin(); iter != succs.end(); iter++) {
		cout << '\t';
		cout << *iter << endl;
	}
}
