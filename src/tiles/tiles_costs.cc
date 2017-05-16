// © 2014 the PBNF Authors under the MIT license. See AUTHORS for the list of authors.

/**
 * \file tiles_costs.cpp
 *
 *
 *
 * \author eaburns
 * \date 05-01-2010
 */

#include <string>
#include <iostream>
using namespace std;

#include <stdlib.h>
#include <string.h>

#include "tiles_costs.h"

struct cost_tab_ent {
	string name;
	TilesCostFunction &fun;
};


static TilesUnitCost unit_cost = TilesUnitCost();
static TilesSqrtCost sqrt_cost = TilesSqrtCost();
static TilesInverseCost inverse_cost = TilesInverseCost();

// Table of cost functions.
static cost_tab_ent cost_functions[] = {
	{ "", unit_cost }, // default value
	{ "unit", unit_cost },
	{ "sqrt", sqrt_cost },
	{ "inverse", inverse_cost },
};


TilesCostFunction &get_cost_function_by_name(string name)
{
	unsigned int n = sizeof(cost_functions) / sizeof(*cost_functions);
	unsigned int i;

	for (i = 0; i < n; i += 1)
		if (cost_functions[i].name == name)
			return cost_functions[i].fun;

	std::cerr << "Bad cost function: " << name << std::endl;
	exit(EXIT_FAILURE);
}
