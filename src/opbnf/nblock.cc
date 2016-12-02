// © 2014 the PBNF Authors under the MIT license. See AUTHORS for the list of authors.

/**
 * \file nblock.cc
 *
 *
 *
 * \author Sofia Lemons
 * \date 2009-04-18
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
using namespace OPBNF;

/**
 * Create a new NBlock structure.
 */
NBlock::NBlock(const Projection *project, unsigned int ident)
	: id(ident),
	  sigma(0),
	  closed(1000),
	  sigma_hot(0),
	  hot(false),
	  inuse(false),
	  best_f(fp_infinity),
	  best_fp(fp_infinity),
	  local_f_min(fp_infinity)
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
void NBlock::print(ostream &o)
{
	fp_type best_fp;
	set<unsigned int>::const_iterator iter;

	best_fp = open_fp.empty() ? fp_infinity : open_fp.peek()->get_f_prime();

	o << "nblock " << id << endl;
	o << "\tsigma: " << sigma << endl;
	o << "\tsigma_hot: " << sigma_hot << endl;
	o << "\thot: " << hot << endl;
	o << "\tinuse: " << inuse << endl;
	o << "\topen: " << (open_fp.empty() ? "false" : "true") << endl;
	o << "\tbest f(n): " << best_fp << endl;

	o << "\tinterferes with: ";
	for (iter = interferes.begin(); iter != interferes.end(); iter++)
		o << (*iter) << " ";
	o << endl;

	o << "\tpreds: ";
	for (iter = preds.begin(); iter != preds.end(); iter++)
		o << (*iter) << " ";
	o << endl;

	o << "\tsuccs: ";
	for (iter = succs.begin(); iter != succs.end(); iter++)
		o << (*iter) << " ";
	o << endl;

}

bool NBlock::better(NBlock* a, NBlock* b, fp_type bound){
	fp_type a_f, a_fp, b_f, b_fp;

	if (!a)
		return false;
	if (!b)
		return true;

	a_fp = a->open_fp.get_best_val();
	b_fp = b->open_fp.get_best_val();

	if (a_fp < bound)
		return a_fp < b_fp;

	if (b_fp < bound)	// (a_fp >= bound) and is therefore worse
		return false;

	a_f = a->open_f.get_best_val();
	b_f = b->open_f.get_best_val();

	return a_f < b_f;

}

void NBlock::add(State *s)
{
	open_fp.add(s);
	open_f.add(s);
	set_best_values();
}

void NBlock::see_update(State *s)
{
	open_fp.see_update(s);
	open_f.see_update(s);
	set_best_values();
}

State *NBlock::take_f(void)
{
	State *s = open_f.take();
	if (!s)
		return NULL;

	open_fp.remove(s);
	set_best_values();

	return s;
}

State *NBlock::take_fp(void)
{
	State *s = open_fp.take();
	if (!s)
		return NULL;

	open_f.remove(s);
	set_best_values();

	return s;
}

void NBlock::prune(void)
{
	open_fp.prune();
	open_f.prune();
}

fp_type NBlock::get_best_f(void)
{
	return best_f.read();
}

fp_type NBlock::get_best_fp(void)
{
	return best_fp.read();
}

void NBlock::set_best_values(void)
{
	if (!empty()) {
		best_f.set(open_f.get_best_val());
		best_fp.set(open_fp.get_best_val());
	} else {
		best_f.set(fp_infinity);
		best_fp.set(fp_infinity);
	}
}


bool NBlock::empty(void)
{
	assert(!open_f.empty() || open_fp.empty());
	assert(!open_fp.empty() || open_f.empty());

	return open_f.empty();
}

fp_type NBlock::get_local_f_min()
{
	return local_f_min;
}

void NBlock::update_local_f_min()
{
	local_f_min = best_f.read();
}
