// © 2014 the PBNF Authors under the MIT license. See AUTHORS for the list of authors.

/**
 * \file tiles_state.cc
 *
 *
 *
 * \author Ethan Burns
 * \date 2008-11-03
 */

#include <assert.h>
#include <math.h>

#include <vector>
#include <iostream>
#include <stdlib.h>
#include <stdio.h>

#include "tiles.h"
#include "tiles_state.h"

using namespace std;

//
// This technique is from Korf, R.E. and Schultze P, "Large-Scale
// Parallel Breadth-First Search, AAAI-05.
//
void TilesState::compute_hash(void) {
	unsigned int bits = 0;
	const Tiles *t = static_cast<const Tiles *>(domain);
	const vector<uint64_t> *ones = t->get_ones();
	const vector<uint64_t> *fact_ary = t->get_fact_ary();

	hash_val = 0;
	for (int i = tiles.size() - 1; i >= 0; i -= 1) {
		uint64_t k = tiles[i];
		uint64_t mask = ~((~0) << k);
		uint64_t v = mask & bits;
		uint64_t d = k - ones->at(v);
		hash_val += d * fact_ary->at(i);
		bits |= 1 << k;
	}
//	printf("h = %lu\n", this->h);

}

TilesState::TilesState(SearchDomain *d, State *parent, fp_type c, fp_type g,
		fp_type h_val, vector<unsigned int> tiles, unsigned int blank) :
		State(d, parent, c, g), tiles(tiles), blank(blank) {
	this->h = h_val;
	compute_hash();
}

TilesState::TilesState(SearchDomain *d, State *parent, fp_type c, fp_type g,
		vector<unsigned int> t, unsigned int b) :
		State(d, parent, c, g), tiles(t), blank(b) {
	assert(t[b] == 0);
	if (domain->get_heuristic())
		this->h = domain->get_heuristic()->compute(this);
	else
		this->h = 0;
	assert(this->h == 0 ? is_goal() : 1);
	compute_hash();
}

/**
 * Test if the tiles are in order.
 */
bool TilesState::is_goal(void) {
	Tiles * t = static_cast<Tiles*>(domain);

	return t->is_goal(this);
}

uint64_t TilesState::hash(void) const {
	return hash_val;
}

State *TilesState::clone(void) const {
	return new TilesState(domain, parent, c, g, tiles, blank);
}

void TilesState::print(ostream &o) const {
	Tiles *t = static_cast<Tiles*>(domain);
	unsigned int i = 0;

	o << "Hash: " << hash_val << endl;
	for (unsigned int y = 0; y < t->get_height(); y += 1) {
		for (unsigned int x = 0; x < t->get_width(); x += 1) {
			o << tiles[i];
			if (x < t->get_width() - 1)
				o << "\t";
			i += 1;
		}
		o << endl;
	}
}

/**
 * Test if two states are the same configuration.
 */
bool TilesState::equals(State *s) const {
	TilesState *other = static_cast<TilesState *>(s);

	for (unsigned int i = 0; i < tiles.size(); i += 1)
		if (tiles[i] != other->tiles[i])
			return false;

	return true;
}

/**
 * Get the tile vector.
 */
const vector<unsigned int> *TilesState::get_tiles(void) const {
	return &tiles;
}

/**
 * Get the blank position.
 */
unsigned int TilesState::get_blank(void) const {
	return blank;
}

void TilesState::init_zbrhash() {
	size = tiles.size();
	for (unsigned int num = 0; num < size; ++num) {
		for (unsigned int pos = 0; pos < size; ++pos) {
			if (num == 0) {
				zbr_table[num][pos] = 0;
			} else {
				zbr_table[num][pos] = rand(); // 0 to 255
			}
		}
	}
	/*
 	for (int num = 0; num < 16; ++num) {
		for (int pos = 0; pos < 16; ++pos) {
			printf("(%d,%d) = %d\n", num, pos, zbr_table[num][pos]);
		}
	}
	*/
}

unsigned int TilesState::zbrhash(void) {
	unsigned int zbr = 0;

	for (unsigned int pos = 0; pos < size; ++pos) {
		zbr = zbr ^ zbr_table[tiles[pos]][pos];
	}
//	printf("zbr = %d\n", zbr % 8);
	return zbr;
}

unsigned int TilesState::size;
unsigned int TilesState::zbr_table[25][25];
