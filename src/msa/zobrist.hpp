/*
 * zobrist.h
 *
 *  Created on: Jun 10, 2014
 *      Author: yuu
 */

#ifndef MSAZOBRIST_H_
#define MSAZOBRIST_H_

#include <cmath>
#include <climits>
#include <cstdlib>
#include <math.h>

template<typename D>
class MSAZobrist {
public:
	enum ABST {
		SINGLE = 1, FIVE = 5
//		PAIR = 1,
//		LINE = 2,
//		BLOCK = 3,
//		TWO = 4,
//		ABSTRACTION = 123,
//		FAR_TILES = 1712,
//		FOURBLOCK_24 = 4024,
//		FOURABSTRACTION = 1234
	};

// Should delete compatibility for performance.
	MSAZobrist(D &d, ABST abst = SINGLE) :
			d(d), structure(abst) {
		initZobrist(structure);
//		dump_table();
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
		unsigned int c = 0;
		for (unsigned int i = 0; i < map.size(); ++i) {
			c = c ^ map[i][s.sequence[i]];
		}
//		printf("zbr = %u\n", c);
		return c;
	}

	unsigned int inc_hash(typename D::State s) const {
		return 0;
	}

//#define RANDOM_ZOBRIST_INITIALIZATION
private:
	void initZobrist(unsigned int abst) {
		map.resize(d.num_of_sequences);
		for (unsigned int i = 0; i < map.size(); ++i) {
			map[i].resize(d.sequences[i].size());
		}
		gen = std::mt19937(rd());
//		unsigned int max = std::numeric_limits<hashlength>::max();
//		unsigned int max = UINT_MAX;
		dis = std::uniform_int_distribution<>(INT_MIN, INT_MAX);
// Not sure I should initialize it by time as it randomize the results for each run.
#ifdef RANDOM_ZOBRIST_INITIALIZATION
		srand(time(NULL));
#endif

		if (abst < 1000) {
			// ###### Abstraction
			abstraction(abst);
		} else {
			abstraction(abst % 1000, abst / 1000);
		}
	}

	void abstraction(unsigned int abst, unsigned int features = 100) {
		printf("abst = %u\n", abst);
		printf("features = %u\n", features);
		if (abst == 0) {
			++abst;
		}
		for (unsigned int i = 0; i < map.size(); ++i) {
			if (i < features) {
				for (unsigned int j = 0; j < map[i].size(); j += abst) {
					unsigned int r = random();
					for (unsigned int ab = 0;
							(ab < abst) && ((j + ab) < map[i].size()); ++ab) {
						map[i][j + ab] = r;
					}
				}
			} else {
				for (unsigned int j = 0; j < map[i].size(); ++j) {
					map[i][j] = 0;
				}
			}
		}
	}

	unsigned int random() {
		return dis(gen) + INT_MAX;
	}

	D& d;
	unsigned int structure;

	std::vector<std::vector<unsigned int> > map;

	std::random_device rd;
	std::mt19937 gen;
	std::uniform_int_distribution<> dis;
};

#endif /* ZOBRIST_H_ */
