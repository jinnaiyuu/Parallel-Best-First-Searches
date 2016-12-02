// Copyright 2012 Ethan Burns. All rights reserved.
// Use of this source code is governed by an MIT-style
// license that can be found in the LICENSE file.
#include "msa.hpp"
#include <string>

MSA::MSA(std::istream &pam, std::istream &sequence) {
	// Read PAM250 scores for edge cost.

	// keys
	// A  R  N  D  C  Q  E  G  H  I  L  K  M  F  P  S  T  W  Y  V  B  J  Z  X  *
	this->pam = new int[25 * 25];
	for (unsigned int i = 0; i < 25 * 25; ++i) {
		pam >> this->pam[i];
		this->pam[i] = 17 - this->pam[i];
	}
	gapcost = this->pam[24];

//	printf("PAM 250\n");
//	printf("   A  R  N  D  C  Q  E  G  H  I  L  K  M  F  P  S  T  W  Y  V  B  J  Z  X  *\n");
//	for (unsigned int i = 0; i < 25; ++i) {
//		printf("%c ", pamcode[i]);
//		for (unsigned int j = 0; j < 25; ++j) {
//			printf("%2d ", this->pam[i * 25 + j]);
//		}
//		printf("\n");
//	}
//	printf("gapcost = %d\n", gapcost);


	//>1aab_
	// GKGDPKKPRGKMSSYAFFVQTSREEHKKKHPDASVNFSEFSKKCSERWKT
	// MSAKEKGKFEDMAKADKARYEREMKTYIPPKGE

	// Read sequences to align

	unsigned int seq = 0;
	std::string name;
//	while (sequence >> name) {

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

	init_pairwise_heuristic();
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
