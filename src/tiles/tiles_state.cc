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
}

TilesState::TilesState(SearchDomain *d, State *parent, fp_type c, fp_type g,
		fp_type h_val, vector<unsigned int> tiles, unsigned int blank) :
		State(d, parent, c, g), tiles(tiles), blank(blank) {
	this->h = h_val;
	compute_hash();
//	init_zbrhash();
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
//	init_zbrhash();
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

unsigned int TilesState::dist_hash(int dist, int n_threads) {
	switch (dist) {
	case distribution::Zobrist:
		return zbrhash() % n_threads;
		break;
	case distribution::AbstractZobrist:
		return abstzbrhash() % n_threads;
		break;
	case distribution::Permutation:
		// PHDA* in Jinnai&Fukunaga 2017. Referred as ``HDA*'' by Burns et al. 2010.
		return hash_val % n_threads;
		break;
	case distribution::Random:
		return random_dist() % n_threads;
		break;
	case distribution::GOHA:
		// Mahapatra, Nihar R., and Shantanu Dutt. 1997.
		return goha(n_threads);
		break;
	default:
		assert(false);
		return 0;
		break;
	}
}

//void TilesState::init_zbrhash() {
//	for (int num = 0; num < 16; ++num) {
//		for (int pos = 0; pos < 16; ++pos) {
//			if (num == 0) {
//				zbr_table[num][pos] = 0;
//			} else {
//				zbr_table[num][pos] = rand();
//			}
//		}
//	}
//}

unsigned int TilesState::zbrhash() {
	const Tiles *t = static_cast<const Tiles *>(domain);

	unsigned int zbr = 0;
	for (int pos = 0; pos < 16; ++pos) {
		zbr = zbr ^ t->zbr_table[tiles[pos]][pos];
	}
	//  printf("zbr = %d\n", zbr);
	return zbr;
}

unsigned int TilesState::abstzbrhash() {
	const Tiles *t = static_cast<const Tiles *>(domain);

	unsigned int zbr = 0;
	for (int pos = 0; pos < 16; ++pos) {
		zbr = zbr ^ t->zbr_table[tiles[pos]][pos/8];
	}
	//  printf("zbr = %d\n", zbr);
	return zbr;
}

unsigned int TilesState::random_dist() {
	return rand();
}
// Mahapatra, Nihar R., and Shantanu Dutt. 1997.
unsigned int TilesState::goha(int n_threads) {
	double g = 0.6180339887;
	unsigned int kappa = 0;
	for (int pos = 0; pos < 16; ++pos) {
		kappa = tiles[pos];
		kappa = kappa << 4;
	}
	unsigned int d = floor(
			(double) n_threads * ((double) kappa * g - (double) floor(kappa * g)));
	return d;
}
