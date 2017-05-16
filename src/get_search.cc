// © 2014 the PBNF Authors under the MIT license. See AUTHORS for the list of authors.

/**
 * \file get_search.cc
 *
 *
 *
 * \author Ethan Burns
 * \date 2008-11-13
 */

#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include <limits>
#include <vector>
#include <iostream>

// this has to come first because other includes may include the C++
// STL map.
#include "lpastar.h"

#include "astar.h"
#include "arastar.h"
#include "awastar.h"
#include "multi_a_star.h"
#include "breadth_first_search.h"
#include "cost_bound_dfs.h"
#include "ida_star.h"
#include "kbfs.h"
#include "psdd_search.h"
#include "arpbnf_search.h"
#include "opbnf_search.h"
#include "bfpsdd_search.h"
#include "wbfpsdd_search.h"
#include "idpsdd_search.h"
#include "dynamic_bounded_psdd.h"
#include "pbnf_search.h"
#include "wpbnf_search.h"
#include "pastar.h"
#include "prastar.h"
#include "wprastar.h"
#include "optimistic.h"

#include "get_search.h"

#include <limits.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>
#if !defined(LINE_MAX)
#define LINE_MAX 255
#endif

using namespace std;

unsigned int threads = 1;
fp_type cost_bound = fp_infinity;
unsigned int nblocks = 1;
double weight = 1.0;
double bound = -1.0;

/**
 * Parse a weight schedule string.
 */
vector<double> *parse_weights(char *str)
{
	unsigned int i, j;
	bool decimal_seen = false;
	char buf[LINE_MAX];
	vector<double> *weights = new vector<double>();

	j = 0;
	for (i = 0; i < strlen(str); i += 1) {
		char c = str[i];
		if (isdigit(c) || (c == '.' && !decimal_seen)) {
			if (c == '.')
				decimal_seen = true;
			assert(j < LINE_MAX); // just bomb out if we overflow :(
			buf[j] = c;
			j += 1;
		} else if (c == ',') {
			double d;

			assert(j < LINE_MAX); // just bomb out if we overflow :(
			buf[j] = '\0';
			j = 0;
			decimal_seen = false;

			sscanf(buf, "%lf", &d);
			weights->push_back(d);
		} else {
			cerr << "Invalid weight list [" << str
			     << "] at char: " << i << endl;
			exit(1);
		}
	}
	if (j > 0) {
		double d;
		buf[j] = '\0';
		j = 0;
		sscanf(buf, "%lf", &d);
		weights->push_back(d);
	}

	return weights;
}

Search *get_search(int argc, char *argv[])
{
	unsigned int min_expansions = 0;
	unsigned int max_e = 0;
	unsigned int multiplier;
	double bound;
	unsigned int dist = 0;

	if (argc > 1 && strcmp(argv[1], "astar") == 0) {
		return new AStar();
	} else if (argc > 1 && sscanf(argv[1], "wastar-%lf", &weight) == 1) {
		return new AStar(false);
	} else if (argc > 1 && sscanf(argv[1], "optimistic-%lf", &bound) == 1) {
		weight = ((bound - 1) * 2) + 1;
		return new Optimistic(bound);
	} else if (argc > 1 && sscanf(argv[1], "wastardd-%lf", &weight) == 1) {
		return new AStar(true);
	} else if (argc > 1 && sscanf(argv[1], "awastar-%lf", &weight) == 1) {
		return new AwAStar();
	} else if (argc > 2 && strcmp(argv[1], "arastar") == 0) {
		vector<double> *weights = parse_weights(argv[2]);
		weight = weights->at(0);
		return new ARAStar(weights);
	} else if (argc > 1 && strcmp(argv[1], "idastar") == 0) {
		return new IDAStar();
	} else if (argc > 1 && strcmp(argv[1], "bfs") == 0) {
		return new BreadthFirstSearch();
/*
	} else if (argc > 1
		   && sscanf(argv[1], "costbounddfs-%lf", &cost_bound) == 1) {
		return new CostBoundDFS(cost_bound);
*/
	} else if (argc > 1 && sscanf(argv[1], "kbfs-%u", &threads) == 1) {
		return new KBFS(threads);
	} else if (argc > 1 && sscanf(argv[1], "pastar-%u", &threads) == 1) {
		return new PAStar(threads);
	} else if (argc > 1 && sscanf(argv[1], "lpastar-%lf-%u", &weight, &threads) == 2) {
		return new LPAStar(threads);
	} else if (argc > 1 && sscanf(argv[1], "prastar-%u", &threads) == 1) {
		return new PRAStar(threads, false, false, false, 0);
	} else if (argc > 1 && sscanf(argv[1], "aprastar-%u-%u", &threads, &nblocks) == 2) {
		return new PRAStar(threads, true, false, false, 0);
	} else if (argc > 1 && sscanf(argv[1], "hdastar-%u-%u-%u", &max_e, &threads, &dist) == 3) {
		return new PRAStar(threads, false, true, true, max_e, dist);
	} else if (argc > 1 && sscanf(argv[1], "hdastar-%u-%u", &max_e, &threads) == 2) {
		return new PRAStar(threads, false, true, true, max_e);
	} else if (argc > 1 && sscanf(argv[1], "ahdastar-%u-%u-%u", &max_e, &threads, &nblocks) == 3) {
		return new PRAStar(threads, true, true, true, max_e);

	} else if (argc > 1 && sscanf(argv[1], "ahdastar-%lf-%u-%u-%u", &weight, &max_e, &threads, &nblocks) == 4) {
		return new PRAStar(threads, true, true, true, max_e);

	} else if (argc > 1 && sscanf(argv[1], "hdastar-syncsends-%u", &threads) == 1) {
		return new PRAStar(threads, false,
				   false, // async_send
				   true,  // async_recv
				   max_e);
	} else if (argc > 1 && sscanf(argv[1], "hdastar-syncrecvs-%u-%u", &max_e, &threads) == 2) {
		return new PRAStar(threads, false,
				   true,   // async_send
				   false,  // async_recv
				   max_e);
	} else if (argc > 1 && sscanf(argv[1], "ahdastar-syncsends-%u-%u", &threads, &nblocks) == 2) {
		return new PRAStar(threads, true,
				   false, // async_send
				   true,  // async_recv
				   max_e);
	} else if (argc > 1 && sscanf(argv[1], "ahdastar-syncrecvs-%u-%u-%u", &max_e, &threads, &nblocks) == 3) {
		return new PRAStar(threads, true,
				   true,   // async_send
				   false,  // async_recv
				   max_e);

	} else if (argc > 1 && sscanf(argv[1], "waprastar-%lf-%u-%u", &weight, &threads, &nblocks) == 3) {
		return new wPRAStar(threads,
				    false /* dd */,
				    true /* abst */,
				    false /* async */,
				    0);
	} else if (argc > 1 && sscanf(argv[1], "waprastardd-%lf-%u-%u", &weight, &threads, &nblocks) == 3) {
		return new wPRAStar(threads,
				    true /* dd */,
				    false /* abst */,
				    false /* async */,
				    0);
	} else if (argc > 1 && sscanf(argv[1], "wprastar-%lf-%u", &weight, &threads) == 2) {
		return new wPRAStar(threads,
				    false, // dd
				    false, // abst
				    false, // async
				    0);
	} else if (argc > 1 && sscanf(argv[1], "wahdastar-%lf-%u-%u-%u",
				      &weight, &max_e, &threads, &nblocks) == 4) {
		return new wPRAStar(threads,
				    false, // dd
				    true,  // abst
				    true,  // async
				    max_e);
	} else if (argc > 1 && sscanf(argv[1], "wahdastardd-%lf-%u-%u-%u",
				      &weight, &max_e, &threads, &nblocks) == 4) {
		return new wPRAStar(threads,
				    true, // dd
				    true, // abst
				    true, // async
				    max_e);
	} else if (argc > 1 && sscanf(argv[1], "whdastar-%lf-%u-%u",
				      &weight, &max_e, &threads) == 3) {
		return new wPRAStar(threads,
				    false, // dd
				    false, // abst
				    true,  // async
				    max_e);
	} else if (argc > 1 && sscanf(argv[1], "whdastardd-%lf-%u-%u",
				      &weight, &max_e, &threads) == 3) {
		return new wPRAStar(threads,
				    true,  // dd
				    false, // abst
				    true,  // async
				    max_e);
	} else if (argc > 1
		   && sscanf(argv[1], "psdd-%u-%u-%lf", &threads, &nblocks,
			     &bound) == 3) {
		return new PSDDSearch(threads, FIXED_OF_DOUBLE(bound));
	} else if (argc > 1
		   && sscanf(argv[1], "psdd-%u-%u", &threads, &nblocks) == 2) {
		return new PSDDSearch(threads);
	} else if (argc > 1
		   && sscanf(argv[1], "bfpsdd-%u-%u-%u-%u", &multiplier,
			     &min_expansions, &threads, &nblocks) == 4) {
		return new BFPSDDSearch(threads, multiplier, min_expansions);
	} else if (argc > 1
		   && sscanf(argv[1], "wbfpsdd-%lf-%u-%u-%u-%u", &weight,
			     &multiplier, &min_expansions,
			     &threads, &nblocks) == 5) {
		return new WBFPSDDSearch(threads, multiplier, min_expansions, false);
	} else if (argc > 1
		   && sscanf(argv[1], "wbfpsdddd-%lf-%u-%u-%u-%u", &weight,
			     &multiplier, &min_expansions,
			     &threads, &nblocks) == 5) {
		return new WBFPSDDSearch(threads, multiplier, min_expansions, true);
	} else if (argc > 1
		   && sscanf(argv[1], "idpsdd-%u-%u", &threads, &nblocks) == 2) {
		return new IDPSDDSearch(threads);
	} else if (argc > 1
		   && sscanf(argv[1], "dynpsdd-%lf-%u-%u",
			     &weight, &threads, &nblocks) == 3) {
		return new DynamicBoundedPSDD(threads, weight);
	} else if (argc > 1
		   && sscanf(argv[1], "pbnf-%lf-%u-%u-%u",
			     &weight, &min_expansions, &threads, &nblocks) == 4) {
		return new PBNF::PBNFSearch(threads, min_expansions, false);
	} else if (argc > 1
		   && sscanf(argv[1], "safepbnf-%lf-%u-%u-%u", &weight, &min_expansions, &threads, &nblocks) == 4) {
		return new PBNF::PBNFSearch(threads, min_expansions, true);
	} else if (argc > 2
		   && sscanf(argv[1], "arpbnf-%u-%u-%u", &min_expansions, &threads, &nblocks) == 3) {
		vector<double> *weights = parse_weights(argv[2]);
		weight = weights->at(0);
		return new ARPBNF::ARPBNFSearch(threads, min_expansions, weights);
	} else if (argc > 1
		   && sscanf(argv[1], "wpbnf-%lf-%u-%u-%u", &weight, &min_expansions, &threads, &nblocks) == 4) {
		return new WPBNF::WPBNFSearch(threads, min_expansions, false);
	} else if (argc > 1
		   && sscanf(argv[1], "wpbnfdd-%lf-%u-%u-%u", &weight, &min_expansions, &threads, &nblocks) == 4) {
		return new WPBNF::WPBNFSearch(threads, min_expansions, true);
	} else if (argc > 1
		   && sscanf(argv[1], "opbnf-%lf-%u-%u-%u", &weight, &min_expansions, &threads, &nblocks) == 4) {
		return new OPBNF::OPBNFSearch(threads, min_expansions);
	} else if (argc > 1 && sscanf(argv[1], "multiastar-%u", &threads) == 1) {
		return new MultiAStar(threads);
	} else if (argc > 1 && sscanf(argv[1], "multiwastar-%lf-%u", &weight, &threads) == 2) {
		return new MultiAStar(threads);
	} else {
		cout << "Must supply a search algorithm:" << endl;
		cout << "\tastar" << endl
		     << "\twastar-<weight>" << endl
		     << "\tidastar" << endl
		     << "\tbfs" << endl
		     << "\tcostbounddfs-<cost>" << endl
		     << "\tkbfs-<threads>" << endl
		     << "\tawastar-<weight>" << endl
		     << "\tarastar <weight-list>" << endl
		     << "\tpastar-<threads>" << endl
		     << "\tlpastar-<weight>-<threads>" << endl
		     << "\tprastar-<threads>" << endl
		     << "\taprastar-<threads>-<nblocks>" << endl
		     << "\thdastar-<max-expansions>-<threads>" << endl
		     << "\tahdastar-<max-expansions>-<threads>-<nblocks>" << endl
		     << "\tahdastar-<max-expansions>-<weight>-<threads>-<nblocks>" << endl
		     << "\thdastar-syncsends-<threads>" << endl
		     << "\thdastar-syncrecvs-<max-expansions>-<threads>" << endl
		     << "\tahdastar-syncsends-<threads>-<nblocks>" << endl
		     << "\tahdastar-syncrecvs-<max-expansions>-<threads>-<nblocks>" << endl
		     << "\twahdastar-<weight>-<max-expansions>-<threads>-<nblocks>" << endl
		     << "\twahdastardd-<weight>-<max-expansions>-<threads>-<nblocks>" << endl
		     << "\twhdastar-<weight>-<max-expansions>-<threads>" << endl
		     << "\twhdastardd-<weight>-<max-expansions>-<threads>" << endl
		     << "\twprastar-<weight>-<threads>" << endl
		     << "\twaprastar-<weight>-<threads>-<nblocks>" << endl
		     << "\twaprastardd-<weight>-<threads>-<nblocks>" << endl
		     << "\tpsdd-<threads>-<nblocks>" << endl
		     << "\tdynpsdd-<weight>-<threads>-<nblocks>" << endl
		     << "\tbfpsdd-<multiplier>-<min-expansions>-<threads>-<nblocks>" << endl
		     << "\twbfpsdd-<weight>-<multiplier>-<min-expansions>-<threads>-<nblocks>" << endl
		     << "\twbfpsdddd-<weight>-<multiplier>-<min-expansions>-<threads>-<nblocks>" << endl
		     << "\tidpsdd-<threads>-<nblocks>" << endl
		     << "\tpbnf-<weight>-<min_expansions>-<threads>-<nblocks>" << endl
		     << "\twpbnf-<weight>-<min_expansions>-<threads>-<nblocks>" << endl
		     << "\twpbnfdd-<weight>-<min_expansions>-<threads>-<nblocks>" << endl
		     << "\tsafepbnf-<min-expansions>-<threads>-<nblocks>" << endl
		     << "\tsafepbnf-<weight>-<min-expansions>-<threads>-<nblocks>" << endl
		     << "\tarpbnf-<min-expansions>-<threads>-<nblocks> <weight-list>" << endl
		     << "\topbnf-<bound>-<min-expansions>-<threads>-<nblocks>" << endl
		     << "\tmultiastar-<threads>" << endl
		     << "\tmultiwastar-<weight>-<threads>" << endl
		     << endl;
		exit(EXIT_FAILURE);
	}
}
