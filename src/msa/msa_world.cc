// © 2014 the PBNF Authors under the MIT license. See AUTHORS for the list of authors.

/**
 * \file msa_world.cc
 *
 *
 *
 * \author Ethan Burns
 * \date 2008-10-08
 */

#include <iostream>
#include <fstream>
#include <map>
#include <string>
#include <vector>

#include <arpa/inet.h>

#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <string>

#include "../util/atomic_int.h"
#include "../util/eps.h"

#include "msa_state.h"
#include "msa_world.h"

using namespace std;

/**
 * Create a new MSAWorld.
 * \param s The istream to read the world file from.
 */
MSAWorld::MSAWorld(istream &pam, istream &sequence) {
	// Read PAM250 scores for edge cost.

	// keys
	// A  R  N  D  C  Q  E  G  H  I  L  K  M  F  P  S  T  W  Y  V  B  J  Z  X  *
	this->pam = new int[25 * 25];
	for (unsigned int i = 0; i < 25 * 25; ++i) {
		pam >> this->pam[i];
		this->pam[i] = 17 - this->pam[i];
	}
	gapcost = this->pam[24];

	printf("PAM 250\n");
	printf("   A  R  N  D  C  Q  E  G  H  I  L  K  M  F  P  S  T  W  Y  V  B  J  Z  X  *\n");
	for (unsigned int i = 0; i < 25; ++i) {
		printf("%c ", pamcode[i]);
		for (unsigned int j = 0; j < 25; ++j) {
			printf("%2d ", this->pam[i * 25 + j]);
		}
		printf("\n");
	}
	printf("gapcost = %d\n", gapcost);

	//>1aab_
	// GKGDPKKPRGKMSSYAFFVQTSREEHKKKHPDASVNFSEFSKKCSERWKT
	// MSAKEKGKFEDMAKADKARYEREMKTYIPPKGE

	// Read sequences to align

	unsigned int seq = 0;
	std::string name;
//	while (sequence >> name) {

	num_of_sequences = 0;

//	for (int i = 0; i < 4; ++i) {
	while (getline(sequence, name)) {
		++num_of_sequences;
		sequences.resize(sequences.size() + 1);
		char amino;
		while (sequence >> amino && 'A' <= amino && amino <= 'Z') {
//			printf("%c", amino);
			if (amino == '\n') {
			} else {
				sequences[seq].push_back(encode(amino));
			}
		}
//		printf("\n");
		++seq;
	}
	printf("number of sequences = %u\n", num_of_sequences);

//	printf("sequences:\n");
//	printf("length = ");
	printf("length of the sequences: ");
	for (unsigned int i = 0; i < sequences.size(); ++i) {
		printf("%u ", sequences[i].size());
	}
	printf("\n");
//	for (unsigned int i = 0; i < sequences.size(); ++i) {
//		printf("seq %u:\n", i);
//		for (unsigned int j = 0; j < sequences[i].size(); ++j) {
//			printf("%u ", sequences[i][j]);
//		}
//		printf("\n");
//	}

}

/**
 * Get the initial state.
 * \return A new state (that must be deleted by the caller) that
 *         represents the initial state.
 */
State *MSAWorld::initial_state(void) {
	vector<unsigned int> a(num_of_sequences, 0);
	MSAState* m = new MSAState(this, NULL, 0, 0, a);
	m->init_zbrhash();
	return m;
}

/**
 * Expand a msastate.
 * \param state The state to expand.
 * \return A newly allocated vector of newly allocated children
 *         states.  All of this must be deleted by the caller.
 */
vector<State*> *MSAWorld::expand(State *state) {
//	printf("expd\n");
	MSAState *s = static_cast<MSAState*>(state);


	int ops = nops(s);

	vector<State *> *children = new vector<State *>;

	for (int i = 0; i < ops; ++i) {
		int path = nthop(s, i);
		vector<unsigned int> incs(num_of_sequences);

		for (int i = 0; i < num_of_sequences; ++i) {
			incs[i] = path & 0x1;
			path >>= 1;
		}

		fp_type	cost = calc_cost(s, &incs);

		for (int i = 0; i < num_of_sequences; ++i) {
			incs[i] += s->sequence[i];
		}

		MSAState* child = new MSAState(this, s, cost, s->get_g() + cost, incs);
		children->push_back(child);
	}
//	printf("expd done\n");

	return children;
}

int MSAWorld::nops(MSAState* s) const {
	int ops = num_of_sequences;

	for (unsigned int i = 0; i < num_of_sequences; ++i) {
		if (s->sequence[i] == sequences[i].size()) {
			--ops;
		}
	}

	return (1 << ops) - 1;
}

int MSAWorld::nthop(MSAState* s, int n) const {
	int op = n + 1;
//		printf("n = %d -> ", n);
	for (unsigned int i = 0; i < num_of_sequences; ++i) {
		if (s->sequence[i] == sequences[i].size()) {
//				printf("X");
			int upper = op / (1 << i);
			int lower = op % (1 << i);
			op = upper * (1 << (i + 1)) + lower;
		}
	}
	return op;
}

int MSAWorld::calc_cost(MSAState* s, vector<unsigned int>* incs) const {
	int cost = 0;
	for (unsigned int i = 0; i < num_of_sequences; ++i) {
		for (unsigned int j = i + 1; j < num_of_sequences; ++j) {
			int amino0, amino1;
			if (incs->at(i) == 0) {
				amino0 = 24;
			} else {
				amino0 = sequences[i][s->sequence[i]];
			}
			if (incs->at(j) == 0) {
				amino1 = 24;
			} else {
				amino1 = sequences[j][s->sequence[j]];
			}
//				printf("%ux%u: %u\n", i, j, pam[amino0 * 25 + amino1]);
			cost += pam[amino0 * 25 + amino1];
		}
	}
	return cost;
}

MSAWorld::Pairwise::Pairwise(const SearchDomain *d)
	: Heuristic(d) {
	MSAWorld* w = (MSAWorld *) domain;

	// allocates a bit more than needed. not an issue.
	pairwise_tables = new int*[w->num_of_sequences * w->num_of_sequences];
	for (unsigned int seq1 = 0; seq1 < w->num_of_sequences; ++seq1) {
		for (unsigned int seq2 = seq1 + 1; seq2 < w->num_of_sequences; ++seq2) {

			unsigned int num = seq1 + seq2 * w->num_of_sequences;
			unsigned int length1 = w->sequences[seq1].size() + 1;
			unsigned int length2 = w->sequences[seq2].size() + 1;

			pairwise_tables[num] = new int[length1 * length2];

			////////////////////////////////
			// Dynamic Programming
			////////////////////////////////

			// Initialization of Dynamic Programming.
			for (int i = 0; i < length1; ++i) {
				pairwise_tables[num][i + length1 * (length2 - 1)] = w->gapcost
						* (length1 - 1 - i);
			}
			for (int j = 0; j < length2; ++j) {
				pairwise_tables[num][j * length1 + length1 - 1] = w->gapcost
						* (length2 - 1 - j);
			}

			for (int pos1 = length1 - 2; pos1 >= 0; --pos1) {
				for (int pos2 = length2 - 2; pos2 >= 0; --pos2) {
					int diagonal, gap1, gap2;

					unsigned int amino1 = w->sequences[seq1][pos1];
					unsigned int amino2 = w->sequences[seq2][pos2];
					diagonal = pairwise_tables[num][(pos1 + 1)
							+ (pos2 + 1) * length1] + w->pam[amino1 * 25 + amino2];
					gap1 = pairwise_tables[num][(pos1 + 1) + pos2 * length1]
							+ w->pam[24 * 25 + amino2];
					gap2 = pairwise_tables[num][pos1 + (pos2 + 1) * length1]
							+ w->pam[amino1 * 25 + 24];

					pairwise_tables[num][pos1 + pos2 * length1] = min(diagonal,
							gap1, gap2);
				}
			}

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

fp_type MSAWorld::Pairwise::compute(State *s) const {
	MSAState* state = (MSAState *) s;
	MSAWorld* w = (MSAWorld *) domain;

	int cost = 0;
//		printf("heuristic\n");

	for (unsigned int i = 0; i < w->num_of_sequences; ++i) {
		for (unsigned int j = i + 1; j < w->num_of_sequences; ++j) {
//				printf("%ux%u: %u\n", i, j,
//						pairwise_tables[i + j * num_of_sequences][s.sequence[i]
//								+ s.sequence[j] * (sequences[i].size() + 1)]);
			cost += pairwise_tables[i + j * w->num_of_sequences][state->sequence[i]
					+ state->sequence[j] * (w->sequences[i].size() + 1)];
		}
	}

//		printf("heuristic = %d\n", cost);
//		return 0;
	return cost;

}


MSAWorld::RowModProject::RowModProject(const SearchDomain *d, unsigned int block_size) :
	block_size(block_size) {
	MSAWorld * w = (MSAWorld *) d;

	number_of_sequences = w->num_of_sequences;
	for (int i = 0; i < number_of_sequences; ++i) {
		lengths.push_back(w->sequences[i].size());
	}

	// may need each sequence length.
}
MSAWorld::RowModProject::~RowModProject() {}

unsigned int MSAWorld::RowModProject::project(State *s) const {
	MSAState* m = static_cast<MSAState *>(s);

	unsigned int ret = 0;
	for (int i = 0; i < number_of_sequences; ++i) {
//		printf("seq: %d %u\n", i, (m->sequence[i] / block_size));
		ret = (ret << 8) | (m->sequence[i] / block_size);
	}
//	printf("project ret = %u\n", ret);

//	vector<unsigned int> b = get_successors(ret);
//	printf("successors = ");
//	for (int i = 0; i < b.size(); ++i) {
//		printf("%u, ", b[i]);
//	}
//	printf("\n");
	return ret;
}
unsigned int MSAWorld::RowModProject::get_num_nblocks(void) const {
//	unsigned int ret = 1;
//	for (int i = 0; i < number_of_sequences; ++i) {
//		ret *= ((lengths[i] - 1) / block_size) + 1;
//	}
//	printf("get num nblock = %u\n", ret);
	// TODO: this
	return 9999999999;
}
vector<unsigned int> MSAWorld::RowModProject::get_successors(unsigned int b) const {
	vector<unsigned int> ret;

	int ops = (1 << number_of_sequences);

	for (int op = 1; op < ops; ++op) {
		int path = op;
		unsigned int child = b;

		for (int i = 0; i < number_of_sequences; ++i) {
			if (path & 0x1) {
				child = child + (1 << (8 * i));
			}
			path >>= 1;
		}
//		printf("suc = %u\n", child);
		ret.push_back(child);
	}
	return ret;
}
vector<unsigned int> MSAWorld::RowModProject::get_predecessors(unsigned int b) const {
	vector<unsigned int> ret;

	int ops = (1 << number_of_sequences);

	for (int op = 1; op < ops; ++op) {
		int path = op;
		unsigned int child = b;

		for (int i = 0; i < number_of_sequences; ++i) {
			if (path & 0x1) {
				// TODO:　if
				unsigned int p = child & (0xFF << (8 * i));
				if (p > 0) {
					child = child - (1 << (8 * i));
				} else {
					//					printf("seq %d is zero: %u. hash = %u\n", i, p, child);
				}
			}
			path >>= 1;
		}
		//		printf("pred = %u\n", child);

		ret.push_back(child);
	}
	return ret;
}


int MSAWorld::Pairwise::min(int a, int b, int c) const {
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

unsigned int MSAWorld::encode(char amino) {
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
