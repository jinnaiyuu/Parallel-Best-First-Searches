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

// So far only single hash.
void TilesState::compute_closed_hash(void) {
	Tiles * t = static_cast<Tiles*>(domain);
	switch (t->get_closed_hash()) {
	case 0: // Perfect Hash
//		printf("perf closed\n");
		hash_val = perfect_hash(t->get_n_threads());
		break;
	default:
//		printf("perfect closed\n");
//		assert(true);
		hash_val = perfect_hash(1);
		break;
	}
//	printf("closed = %u\n", hash_val);
}
//
// This technique is from Korf, R.E. and Schultze P, "Large-Scale
// Parallel Breadth-First Search, AAAI-05.
//
uint64_t TilesState::perfect_hash(unsigned int n_threads) {
	unsigned int bits = 0;
	const Tiles *t = static_cast<const Tiles *>(domain);
	const vector<uint64_t> *ones = t->get_ones();
	const vector<uint64_t> *fact_ary = t->get_fact_ary();

	uint64_t hash = 0;
//	hash_val = 0;
	for (int i = tiles.size() - 1; i >= 0; i -= 1) {
		uint64_t k = tiles[i];
		uint64_t mask = ~((~0) << k);
		uint64_t v = mask & bits;
		uint64_t d = k - ones->at(v);
		hash += d * fact_ary->at(i);
		bits |= 1 << k;
	}

	assert(n_threads > 0);
//	printf("n_threads = %u\n", n_threads);
	hash = hash / static_cast<uint64_t>(n_threads);
//	printf("perfect h = %lu\n", hash);
	return hash;
}

TilesState::TilesState(SearchDomain *d, State *parent, fp_type c, fp_type g,
		fp_type h_val, vector<unsigned int> tiles, unsigned int blank) :
		State(d, parent, c, g), tiles(tiles), blank(blank) {
	this->h = h_val;
	compute_closed_hash();
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
	compute_closed_hash();
}

/**
 * Test if the tiles are in order.
 */
bool TilesState::is_goal(void) {
	Tiles * t = static_cast<Tiles*>(domain);

	return t->is_goal(this);
}

uint128_t TilesState::hash(void) const {
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
	/*	size = tiles.size();
	 for (unsigned int num = 0; num < size; ++num) {
	 for (unsigned int pos = 0; pos < size; ++pos) {
	 if (num == 0) {
	 zbr_table[num][pos] = 0;
	 } else {
	 zbr_table[num][pos] = rand(); // 0 to 255
	 }
	 }
	 }*/
	init_zbrhash_block();
	printf("block zbrhash\n");
	/*
	 for (int num = 0; num < 16; ++num) {
	 for (int pos = 0; pos < 16; ++pos) {
	 printf("(%d,%d) = %d\n", num, pos, zbr_table[num][pos]);
	 }
	 }
	 */
}

void TilesState::init_zbrhash_block() {
	Tiles * t = static_cast<Tiles*>(domain);
	unsigned int h = t->get_dist_hash();

	size = tiles.size();

	if (h == 0) {
		for (int i = 1; i < size; ++i) {
			for (int j = 0; j < size; ++j) {
				int r = random();
				zbr_table[i][j] = r; // zbr[number][place]
			}
		}
	} else if (h == 2 || h == 3) {
		if (size == 16) {
			int js[4] = { 0, 2, 8, 10 };
			for (int i = 1; i < size; ++i) {
				for (int j = 0; j < 4; ++j) {
					int r = random();
					zbr_table[i][js[j]] = r; // zbr[number][place]
					zbr_table[i][js[j] + 1] = r;
					zbr_table[i][js[j] + 4] = r;
					zbr_table[i][js[j] + 4 + 1] = r;
				}
			}

		} else {
			int a[5] = { 0, 1, 5, 6, 10 };
			int b[5] = { 15, 16, 20, 21, 22 };
			int c[5] = { 14, 18, 19, 23, 24 };
			int d[5] = { 2, 3, 4, 8, 9 };
			int e[5] = { 7, 11, 12, 13, 17 };

			for (int i = 1; i < size; ++i) {
				int r = random();
				for (int j = 0; j < 5; ++j) {
					zbr_table[i][a[j]] = r; // zbr[number][place]
				}
				r = random();
				for (int j = 0; j < 5; ++j) {
					zbr_table[i][b[j]] = r; // zbr[number][place]
				}
				r = random();
				for (int j = 0; j < 5; ++j) {
					zbr_table[i][c[j]] = r; // zbr[number][place]
				}
				r = random();
				for (int j = 0; j < 5; ++j) {
					zbr_table[i][d[j]] = r; // zbr[number][place]
				}
				r = random();
				for (int j = 0; j < 5; ++j) {
					zbr_table[i][d[j]] = r; // zbr[number][place]
				}
			}

		}
	} else {
		assert(false);
	}
}

unsigned int TilesState::dist_hash(void) {
	Tiles * t = static_cast<Tiles*>(domain);
	unsigned int hash = 0;
	switch (t->get_dist_hash()) {
	case 0:
//		printf("zobrist hash\n");
		hash = zobrist_hash();
		break;
	case 1:
//		printf("residual\n");
		hash = perf_residual_hash(); // Permutation Hash
		break;
	case 2:
		// Abstract Zobrist Hashing
//		printf("zobrist hash\n");
		hash = zobrist_hash();
		break;
	case 3:
		// Abstract Zobrist Hashing for 24 Puzzle
//		printf("zobrist hash\n");
		hash = zobrist_hash();
		break;
	case 9:
		hash = 0;
		break;
	default:
		assert(false);
		break;
	}
//	printf("dist = %u\n", hash % t->get_n_threads());
	return hash;
}

unsigned int TilesState::zobrist_hash(void) {
	unsigned int zbr = 0;

	for (unsigned int pos = 0; pos < size; ++pos) {
		zbr = zbr ^ zbr_table[tiles[pos]][pos];
	}
//	printf("zbr = %d\n", zbr);
	return zbr;
}

// This is actually just a perfect hash returning.
unsigned int TilesState::perf_residual_hash(void) {
	return perfect_hash(1);
}

unsigned int TilesState::size;
unsigned int TilesState::zbr_table[25][25];
