// Copyright 2012 Ethan Burns. All rights reserved.
// Use of this source code is governed by an MIT-style
// license that can be found in the LICENSE file.
#include "../search.hpp"
#include "../fatal.hpp"
#include "../hashtbl.hpp"

#include <cstdio>
#include <cstdlib>
#include <cassert>
#include <stdint.h>
#include <vector>
#include <iostream>

struct MSA {

	struct State {
		char tiles[1];
		char blank;
		std::vector<unsigned int> sequence;
		int h;
	};

	struct PackedState {
		// TODO: max number of sequence = 8.
		uint64_t word;

		unsigned long hash() const {
			return word;
		}

		bool eq(const PackedState &h) const {
			return word == h.word;
//			for (unsigned int i = 0; i < num_of_sequences; ++i) {
//				if (sequence[i] != h.sequence[i]) {
//					return false;
//				}
//			}
//			return true;
		}
	};

	// Tiles constructs a new instance by reading
	// the initial state from the given file which is
	// expected to be in Wheeler's tiles instance
	// format.
	MSA(std::istream &pam, std::istream &sequence);
	~MSA() {
		delete pam;
		for (unsigned int seq1 = 0; seq1 < num_of_sequences; ++seq1) {
			for (unsigned int seq2 = seq1 + 1; seq2 < num_of_sequences;
					++seq2) {
				delete pairwise_tables[seq1 + seq2 * num_of_sequences];
			}
		}
		delete pairwise_tables;
	}

	State initial() const {
		State s;
		s.sequence.resize(num_of_sequences);
//		printf("init %u\n", num_of_sequences);
		for (unsigned int i = 0; i < num_of_sequences; ++i) {
			s.sequence[i] = 0;
		}
		s.h = heuristic(s);
		return s;
	}

	int h(const State &s) const {
		return s.h;
	}

	bool isgoal(const State &s) const {
//		printf("isgoal\n");
		for (unsigned int i = 0; i < num_of_sequences; ++i) {
			if (s.sequence[i] != sequences[i].size()) {
				return false;
			}
		}
//		printf("goal: %d %d\n", s.sequence[0], s.sequence[1]);
		return true;
	}

	int nops(const State &s) const {
		int ops = num_of_sequences;

		for (unsigned int i = 0; i < num_of_sequences; ++i) {
			if (s.sequence[i] == sequences[i].size()) {
				--ops;
			}
		}

		return (1 << ops) - 1;
	}

	// Potentially cause problem not to excluding when any of the positions exceed the length of the sequence,
	// but reducing the efforts on nops & nthop will significantly improve the performance.
	int nthop(const State &s, int n) const {
		int op = n + 1;
//		printf("n = %d -> ", n);
		for (unsigned int i = 0; i < num_of_sequences; ++i) {
			if (s.sequence[i] == sequences[i].size()) {
//				printf("X");
				int upper = op / (1 << i);
				int lower = op % (1 << i);
				op = upper * (1 << (i + 1)) + lower;
			} else {
//				printf("O");
			}
		}
//		printf(": op = %d\n", op);

//		printf("\n");
//		int r = n;
//		for (unsigned int i = 0; i < num_of_sequences; ++i) {
//			if (s.sequence[i] == sequences[i].size()) {
//				int rup = r & (1 << i);
//			}
//		}
//		return optab[(int) s.blank].ops[n];
		return op;
	}

	struct Undo {
		int newb;
		int h;
	};

	Edge<MSA> apply(State &s, int newb) const {
//		printf("apply\n");
//		for (unsigned int i = 0; i < num_of_sequences; ++i) {
//			printf("%u ", s.sequence[i]);
//		}
//		printf("\n");
//		printf("BEFORE:\n");
		heuristic(s);

		// TODO: apply incrementing seqs;
		int path = newb;
//		printf("path = %d\n", path);

		// TODO: test this bit hacking.
//		printf("edge: ");
		int* incs = new int[num_of_sequences];
		for (int i = 0; i < num_of_sequences; ++i) {
			incs[i] = path & 0x1;
			path >>= 1;
//			printf("%u", incs[i]);
			s.sequence[i] += incs[i];
		}
//		printf("\n");


//		printf("\ncosts:\n");
		int cost = 0;
		for (unsigned int i = 0; i < num_of_sequences; ++i) {
			for (unsigned int j = i + 1; j < num_of_sequences; ++j) {
				int amino0, amino1;
				if (incs[i] == 0) {
					amino0 = 24;
				} else {
					amino0 = sequences[i][s.sequence[i]];
				}
				if (incs[j] == 0) {
					amino1 = 24;
				} else {
					amino1 = sequences[j][s.sequence[j]];
				}
//				printf("%ux%u: %u\n", i, j, pam[amino0 * 25 + amino1]);
				cost += pam[amino0 * 25 + amino1];
			}
		}

		Edge<MSA> e(cost, newb, -100);
		e.undo.newb = newb;
		e.undo.h = s.h;

//		printf("AFTER:\n");
		s.h = heuristic(s);
//		printf("\n\n");

		return e;
	}

	void undo(State &s, const Edge<MSA> &e) const {
		s.h = e.undo.h;
		int path = e.undo.newb;
		for (int i = 0; i < num_of_sequences; ++i) {
			int inc = path & 0x1;
			path >>= 1;
			s.sequence[i] -= inc;
		}

	}

	// pack packes state s into the packed state dst.
	void pack(PackedState &dst, State &s) const {
		dst.word = 0; // to make g++ shut up about uninitialized usage.
		for (int i = 0; i < num_of_sequences; i++) {
			dst.word = (dst.word << 8) | s.sequence[i];
		}
//		printf("hash = %lu\n", dst.word);
	}

	// unpack unpacks the packed state s into the state dst.
	void unpack(State &dst, PackedState s) const {
		dst.h = 0;
		dst.sequence.resize(num_of_sequences);
//		printf("t = ");
		for (int i = num_of_sequences - 1; i >= 0; i--) {
			unsigned int t = s.word & 0xFF;
//			printf("%d ", t);
			s.word >>= 8;
			dst.sequence[i] = t;
		}
//		printf("\n");

	}

	void print_alignment(std::vector<MSA::State> solution) {
		printf("alignment:\n");
		for (int sq = 0; sq < num_of_sequences; ++sq) {
			char l0[solution.size()];
			unsigned int b0 = 100;
			for (int i = 0; i < solution.size(); ++i) {
				MSA::State s = solution[i];
				if (b0 == s.sequence[sq]) {
					l0[i] = '.';
				} else {
					l0[i] = pamcode[sequences[sq][s.sequence[sq]]];
					b0 = s.sequence[sq];
				}
			}
			for (int i = solution.size() - 1; i > 0; --i) {
				printf("%c", l0[i]);
			}
			printf("\n");
		}
	}

	unsigned int calc_cost(std::vector<MSA::State> solution) {

		unsigned int cost = 0;
		for (int sq = 0; sq < solution.size() - 1; ++sq) {
			MSA::State s = solution[sq];
			bool* isgap = new bool[num_of_sequences];
			if (sq == 0) {
				for (unsigned int i = 0; i < num_of_sequences; ++i) {
					isgap[i] = false;
				}
			} else {
				for (unsigned int i = 0; i < num_of_sequences; ++i) {
					isgap[i] = (s.sequence[i] == solution[sq - 1].sequence[i]);
				}
			}

//			printf("\ncosts = \n");

			for (unsigned int i = 0; i < num_of_sequences; ++i) {
				for (unsigned int j = i + 1; j < num_of_sequences; ++j) {
					int amino0, amino1;
					if (isgap[i] == true) {
						amino0 = 24;
					} else {
						amino0 = sequences[i][s.sequence[i]];
					}
					if (isgap[j] == true) {
						amino1 = 24;
					} else {
						amino1 = sequences[j][s.sequence[j]];
					}
//					printf("%ux%u: %u\n", i, j, pam[amino0 * 25 + amino1]);
					cost += pam[amino0 * 25 + amino1];
				}
			}
			delete isgap;
		}
		return cost;
	}

	unsigned int num_of_sequences = 0;

	std::vector<std::vector<unsigned int> > sequences;

private:

//	int edgecost() {
//
//	}

	int heuristic(const State& s) const {
		int cost = 0;
//		printf("heuristic\n");

		for (unsigned int i = 0; i < num_of_sequences; ++i) {
			for (unsigned int j = i + 1; j < num_of_sequences; ++j) {
//				if (s.sequence[i] > sequences[i].size()
//						|| s.sequence[j] > sequences[j].size()) {
//					cost += 10000; // to eliminate exceeding sequence.
//				} else {
//				printf("%ux%u: %u\n", i, j,
//						pairwise_tables[i + j * num_of_sequences][s.sequence[i]
//								+ s.sequence[j] * (sequences[i].size() + 1)]);
				cost += pairwise_tables[i + j * num_of_sequences][s.sequence[i]
						+ s.sequence[j] * (sequences[i].size() + 1)];
//				cost += pairwise_heuristic(i, s.sequence[i], j, s.sequence[j]);
//				}
			}
		}

//		for (int i = 0; i < num_of_sequences; i++) {
//			printf("%d ", s.sequence[i]);
//			if (s.sequence[i] > sequences[i].size()) {
//				cost += 10000; // to eliminate exceeding sequence.
//			} else {
//				cost = cost + (sequences[i].size() - s.sequence[i]) * gapcost;
//			}
//		}

//		printf("heuristic = %d\n", cost);

//		return 0;
		return cost;
	}

	void init_pairwise_heuristic() {
		// allocates a bit more than needed. not an issue.
		pairwise_tables = new int*[num_of_sequences * num_of_sequences];
		for (unsigned int seq1 = 0; seq1 < num_of_sequences; ++seq1) {
			for (unsigned int seq2 = seq1 + 1; seq2 < num_of_sequences;
					++seq2) {

				unsigned int num = seq1 + seq2 * num_of_sequences;
				unsigned int length1 = sequences[seq1].size() + 1;
				unsigned int length2 = sequences[seq2].size() + 1;

				pairwise_tables[num] = new int[length1 * length2];

				////////////////////////////////
				// Dynamic Programming
				////////////////////////////////

				// Initialization of Dynamic Programming.
				for (int i = 0; i < length1; ++i) {
					pairwise_tables[num][i + length1 * (length2 - 1)] = gapcost
							* (length1 - 1 - i);
				}
				for (int j = 0; j < length2; ++j) {
					pairwise_tables[num][j * length1 + length1 - 1] = gapcost
							* (length2 - 1 - j);
				}

				for (int pos1 = length1 - 2; pos1 >= 0; --pos1) {
					for (int pos2 = length2 - 2; pos2 >= 0; --pos2) {
						int diagonal, gap1, gap2;

						unsigned int amino1 = sequences[seq1][pos1];
						unsigned int amino2 = sequences[seq2][pos2];
						diagonal = pairwise_tables[num][(pos1 + 1)
								+ (pos2 + 1) * length1]
								+ pam[amino1 * 25 + amino2];
						gap1 = pairwise_tables[num][(pos1 + 1) + pos2 * length1]
								+ pam[24 * 25 + amino2];
						gap2 = pairwise_tables[num][pos1 + (pos2 + 1) * length1]
								+ pam[amino1 * 25 + 24];

						pairwise_tables[num][pos1 + pos2 * length1] = min(
								diagonal, gap1, gap2);
					}
				}

//				cost += pairwise_tables[i + j * num_of_sequences]
//				                        [s.sequence[i]+ s.sequence[j] * (sequences[i].size() + 1)];

//				printf("TABLE %ux%u:\n", seq1, seq2);
//				for (int j = 0; j < length2; ++j) {
//					for (int i = 0; i < length1; ++i) {
//						printf("%3d ",
//								pairwise_tables[seq1 + seq2 * num_of_sequences][i
//										+ j * length1]);
//					}
//					printf("\n");
//				}
//				printf("\n");
			}
		}
	}

// TODO: PAM 250
	int* pam;
	int gapcost;

	int** pairwise_tables; // Just a pointer would be faster than multiple pointers.

	char pamcode[25] =
			{ 'A', 'R', 'N', 'D', 'C', 'Q', 'E', 'G', 'H', 'I', 'L', 'K', 'M',
					'F', 'P', 'S', 'T', 'W', 'Y', 'V', 'B', 'J', 'Z', 'X', '*' };

	int min(int a, int b, int c) const {
		int min;
		if (a < b) {
			min = a;
		} else {
			min = b;
		}
		if (min > c) {
			min = c;
		}
		return min;
	}

	unsigned int encode(char amino) {
		// A  R  N  D  C  Q  E  G  H  I  L  K  M  F  P  S  T  W  Y  V  B  J  Z  X  *
		switch (amino) {
		case 'A':
			return 0;
		case 'R':
			return 1;
		case 'N':
			return 2;
		case 'D':
			return 3;
		case 'C':
			return 4;
		case 'Q':
			return 5;
		case 'E':
			return 6;
		case 'G':
			return 7;
		case 'H':
			return 8;
		case 'I':
			return 9;
		case 'L':
			return 10;
		case 'K':
			return 11;
		case 'M':
			return 12;
		case 'F':
			return 13;
		case 'P':
			return 14;
		case 'S':
			return 15;
		case 'T':
			return 16;
		case 'W':
			return 17;
		case 'Y':
			return 18;
		case 'V':
			return 19;
		case 'B':
			return 20;
		case 'J':
			return 21;
		case 'Z':
			return 22;
		case 'X':
			return 23;
		case '*':
			return 24;
		default:
			printf("ERROR");
			break;
		}
		return 0;
	}
};
