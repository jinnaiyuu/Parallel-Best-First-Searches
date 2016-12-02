// © 2014 the PBNF Authors under the MIT license. See AUTHORS for the list of authors.

/**
 * \file nblock.cc
 *
 *
 *
 * \author Ethan Burns
 * \date 2008-10-21
 */

#include <assert.h>

#include <limits>
#include <vector>
#include <set>

#include "nblock.h"
#include "../open_list.h"
#include "../queue_open_list.h"
#include "../closed_list.h"

using namespace std;
using namespace PBNF;

/**
 * Create a new NBlock structure.
 */
NBlock::NBlock(const Projection *project, unsigned int ident)
	: id(ident),
	  sigma(0),
	  closed(4000),
	  sigma_hot(0),
	  hot(false),
	  inuse(false),
	  pq_index(-1)
{
	set<unsigned int> iset;
	assert(id < project->get_num_nblocks());

	vector<unsigned int>::iterator i, j;
	preds = project->get_predecessors(id);
	succs = project->get_successors(id);

	// predecessors, successors and the predecessors of the successors.
	ninterferes = 0;
	iset.insert(preds.begin(), preds.end());
	for (i = succs.begin(); i != succs.end(); i++) {
		iset.insert(*i);
		ninterferes += 1;
		vector<unsigned int> spreds = project->get_predecessors(*i);
		for (j = spreds.begin(); j != spreds.end(); j++) {
			iset.insert(*j);
			ninterferes += 1;
		}
	}

	// this over-allocates the array a bit because we may have
	// counted duplicate elements.
	interferes = new unsigned int[ninterferes];

	unsigned int next = 0;
	set<unsigned int>::iterator k;
	for (k = iset.begin(); k != iset.end(); k++) {
		if ((*k) != id) {
			interferes[next] = *k;
			next += 1;
		}
	}

	ninterferes = next;
}

/**
 * Destroy an NBlock and all of its states.
 */
NBlock::~NBlock(void)
{
	closed.delete_all_states();
	delete[] interferes;
}

/**
 * Print an NBlock to the given stream.
 */
void NBlock::print(ostream &o)
{
	fp_type best_f;
	vector<unsigned int>::const_iterator i;

	best_f = open.empty() ? fp_infinity : open.peek()->get_f();

	o << "nblock " << id << endl;
	o << "\tsigma: " << sigma << endl;
	o << "\tsigma_hot: " << sigma_hot << endl;
	o << "\thot: " << hot << endl;
	o << "\tinuse: " << inuse << endl;
	o << "\topen: " << (open.empty() ? "false" : "true") << endl;
	o << "\tbest f(n): " << best_f << endl;

	o << "\tinterferes with: ";
	for (unsigned int i = 0; i < ninterferes; i++)
		o << (interferes[i]) << " ";
	o << endl;

	o << "\tpreds: ";
	for (i = preds.begin(); i != preds.end(); i++)
		o << (*i) << " ";
	o << endl;

	o << "\tsuccs: ";
	for (i = succs.begin(); i != succs.end(); i++)
		o << (*i) << " ";
	o << endl;

}

fp_type NBlock::best_value(void)
{
	return open.get_best_val();
}
