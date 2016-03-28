/*
 * zobrist.h
 *
 *  Created on: Jun 10, 2014
 *      Author: yuu
 */

#ifndef HYPERPLANE_H_
#define HYPERPLANE_H_

#include <cmath>
//#include <tgmath.h>
#include <climits>
#include <cstdlib>
#include <math.h>

#include "zobrist.hpp"

template<typename D>
class Hyperplane {
public:
	enum ABST {
		SINGLE = 0, FIVE = 5
//		PAIR = 1,
//		LINE = 2,
//		BLOCK = 3,
//		TWO = 4,
//		ABSTRACTION = 123,
//		FAR_TILES = 1712,
//		FOURBLOCK_24 = 4024,
//		FOURABSTRACTION = 1234
	};

	Hyperplane(D &d, ABST abst) :
			d(d), structure(abst), zbr(d) {
		// give abst a number of processors.
		init(structure);
		if (is_d_int) {
			printf("d = %d\n", param_d);
		} else {
			printf("d = 1 / %d\n", param_d);
		}
	}

	/**
	 * @param number: The number slided
	 * @param from : Where the number is in parent node
	 * @param to : Where the number is in child node
	 * @return the value to XOR to the Zobrist value of parent.
	 */
	unsigned int inc_hash(const unsigned int previous, const int number,
			const int from, const int to, const char* const newBoard,
			const typename D::State s) const {
//		printf("inc_hash hwd\n");
		int c = 0;
		for (unsigned int i = 0; i < d.num_of_sequences; ++i) {
			c += s.sequence[i];
		}
//		printf("added c = %d\n", c);
		if (is_d_int) {
			c = c / param_d;
			c = floor(c);
		} else {
			c = c * param_d;
			c += zbr.inc_hash(previous, number, from, to, newBoard, s)
					% param_d;
		}
//		printf("hash = %u\n", c);
		return c;
	}

	unsigned int inc_hash(typename D::State s) const {
		return 0;
	}

//#define RANDOM_ZOBRIST_INITIALIZATION
private:
	void init(unsigned int abst) {
		double total_length = 0.0;
		for (unsigned int i = 0; i < d.num_of_sequences; ++i) {
			total_length += d.sequences[i].size();
		}
		double p = 0.03 * total_length / log2(static_cast<double>(abst));

		if (p >= 1.0) {
			is_d_int = true;
			param_d = p;
		} else {
			is_d_int = false;
			param_d = 1.0 / p;
		}
		printf("abst = %u\n", abst);
		printf("log(abst) = %f\n", log2(static_cast<double>(abst)));
		printf("total_length = %f\n", total_length);
		printf("is_d_int = %d\n", is_d_int);
		printf("p = %f\n", p);
		printf("param_d = %d\n", param_d);

	}

	D& d;
	int structure;

	bool is_d_int;
	int param_d;

	MSAZobrist<D> zbr;

};

#endif /* ZOBRIST_H_ */
