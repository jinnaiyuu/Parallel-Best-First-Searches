// © 2014 the PBNF Authors under the MIT license. See AUTHORS for the list of authors.

/**
 * \file msa_state.cc
 *
 *
 *
 * \author Ethan Burns
 * \date 2008-10-08
 */

#include <iostream>
#include <vector>
#include <math.h>
#include <stdlib.h>
#include <stdio.h>

#include "msa_world.h"
#include "msa_state.h"

using namespace std;

/**
 * Create a new msa state.
 * \param d The search domain.
 * \param parent The parent of this state.
 * \param g The g-value of this state.
 * \param x The x-coordinate of this state.
 * \param y The y-coordinate of this state.
 */
MSAState::MSAState(MSAWorld *d, State *parent, fp_type c, fp_type g, vector<unsigned int> sequence)
	: State(d, parent, c, g)
{
	this->sequence = sequence;

	if (domain->get_heuristic())
		this->h = domain->get_heuristic()->compute(this);
	else
		this->h = 0;
}

/**
 * Test if this state is the goal.
 * \return True if this is a goal, false if not.
 */
bool MSAState::is_goal(void)
{
	const MSAWorld *d;

	d = static_cast<const MSAWorld *>(domain);

	for (unsigned int i = 0; i < d->num_of_sequences; ++i) {
		if (sequence[i] != d->sequences[i].size()) {
			return false;
		}
	}
	return true;
}

/**
 * Get the hash value of this state.
 * \return A unique hash value for this state.
 */
uint128_t MSAState::hash(void) const
{
	const MSAWorld *d;

	d = static_cast<const MSAWorld *>(domain);

	uint128_t h = 0;
	for (int i = 0; i < d->num_of_sequences; i++) {
		h = (h << 8) | sequence[i];
	}

	return h;
}

/**
 * Create a copy of this state.
 * \return A copy of this state.
 */
State *MSAState::clone(void) const
{
	MSAWorld *d = static_cast<MSAWorld *>(domain);

	vector<unsigned int> cln(sequence);

	return new MSAState(d, parent, c, g, sequence);
}

/**
 * Print this state.
 * \param o The ostream to print to.
 */
void MSAState::print(ostream &o) const
{
	o << "not implemented";
}

/**
 * MSAState equality.
 */
bool MSAState::equals(State *state) const
{
	MSAState *s;

	s = static_cast<MSAState *>(state);

	const MSAWorld *d;

	d = static_cast<const MSAWorld *>(domain);

	for (int i = 0; i < d->num_of_sequences; i++) {
		if (sequence[i] != s->sequence[i]) {
			return false;
		}
	}
	return true;
}


void MSAState::init_zbrhash(void) {
	const MSAWorld *d;
	d = static_cast<const MSAWorld *>(domain);

	for (unsigned int i = 0; i < d->num_of_sequences; ++i) {
		for (unsigned int j = 0; j < d->sequences[i].size();) {
			int r = random();
			for (int k = 0; k < d->abstraction; ++k, ++j) {
				zbr_table[i][j] = r;
			}
		}
	}

//	printf("zbr table\n");
//	for (unsigned int i = 0; i < d->num_of_sequences; ++i) {
//		for (unsigned int j = 0; j < d->sequences[i].size(); ++j) {
//			printf("%u ", zbr_table[i][j]);
//		}
//		printf("\n");
//	}

}

// Zobrist Hash
unsigned int MSAState::dist_hash(void){
	return zobrist_hash();
}

unsigned int MSAState::zobrist_hash(void) {
	const MSAWorld *d;
	d = static_cast<const MSAWorld *>(domain);

	unsigned int zbr = 0;

	for (unsigned int pos = 0; pos < d->num_of_sequences; ++pos) {
		zbr = zbr ^ zbr_table[pos][sequence[pos]];
	}
//	printf("zbr = %u\n", zbr);
	return zbr;
}

unsigned int MSAState::zbr_table[10][200];
