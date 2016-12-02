// © 2014 the PBNF Authors under the MIT license. See AUTHORS for the list of authors.

 /**
 * \file nblock.cc
 *
 *
 *
 * \author eaburns
 * \date 2009-03-13
 */

#include "nblock.h"

using namespace std;
using namespace WBFPSDD;

/**
 * Create a new NBlock structure.
 */
NBlock::NBlock(const Projection *project, unsigned int ident)
  : id(ident),
    sigma(0),
    closed(1000),
    fp_pq_index(-1),
    inuse(false),
    inlayer(false)
{
	assert(id < project->get_num_nblocks());

	vector<unsigned int>::iterator i, j;
	vector<unsigned int> preds_vec = project->get_predecessors(id);
	vector<unsigned int> succs_vec = project->get_successors(id);
	// predecessors, successors and the predecessors of the successors.
	vector<unsigned int> interferes_vec = preds_vec;
	for (i = succs_vec.begin(); i != succs_vec.end(); i++) {
		interferes_vec.push_back(*i);
		vector<unsigned int> spreds = project->get_predecessors(*i);
		for (j = spreds.begin(); j != spreds.end(); j++) {
			interferes_vec.push_back(*j);
		}
	}
	for (i = preds_vec.begin(); i != preds_vec.end(); i++)
		if (*i != id)
			preds.insert(*i);
	for (i = succs_vec.begin(); i != succs_vec.end(); i++)
		if (*i != id)
			succs.insert(*i);
	for (i = interferes_vec.begin(); i != interferes_vec.end(); i++)
		if (*i != id)
			interferes.insert(*i);
}



/**
 * Destroy an NBlock and all of its states.
 */
NBlock::~NBlock(void)
{
	closed.delete_all_states();
}


/**
 * Print an NBlock to the given stream.
 */
void NBlock::print(ostream &o) const
{
        set<unsigned int>::const_iterator iter;

	o << "nblock " << id << endl;
	o << "\tsigma: " << sigma << endl;

	o << "\tinterferes with: ";
	for (iter = interferes.begin(); iter != interferes.end(); iter++)
		o << *iter << " ";
	o << endl;

	o << "\tpreds: ";
	for (iter = preds.begin(); iter != preds.end(); iter++)
		o << *iter << " ";
	o << endl;

	o << "\tsuccs: ";
	for (iter = succs.begin(); iter != succs.end(); iter++)
		o << *iter << " ";
	o << endl;

}
