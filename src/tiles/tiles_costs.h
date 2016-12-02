/* © 2014 the PBNF Authors under the MIT license. See AUTHORS for the list of authors.*/

/**
 * \file tiles_costs.h
 *
 * Some cost functions for the tiles domain.
 *
 * \author eaburns
 * \date 05-01-2010
 */
#if !defined(_TILES_COSTS_H_)
#define _TILES_COSTS_H_

#include <string>
using namespace std;

#include <math.h>

/**
 * The parent class for cost functions for tiles.  There is just one
 * method which is the 'operator()' method that gets the cost of
 * moving a tile.
 */
class TilesCostFunction {
public:
	/**
	 * This operator gets the cost of moving a tile with the
	 * number [tile_number].
	 */
	virtual double operator()(unsigned int tile_number) = 0;

	virtual ~TilesCostFunction() {}

};

/**
 * The default unit cost function.
 */
class TilesUnitCost : public TilesCostFunction {
public:
	double operator()(unsigned int tile_number) {
		return 1.0;
	}
};

/**
 * Square root cost function makes the cost of moving a tile the
 * square root of its number.
 */
class TilesSqrtCost  : public TilesCostFunction {
public:
	double operator()(unsigned int tile_number) {
		return sqrt(tile_number);
	}
};

/**
 * Inverse cost function makes the cost of moving a tile the
 * inverse of its number.
 */
class TilesInverseCost  : public TilesCostFunction {
public:
	double operator()(unsigned int tile_number) {
		return 1.0 / tile_number;
	}
};

/**
 * Gets a cost function given its name.
 */
TilesCostFunction& get_cost_function_by_name(string name);


#endif /* !_TILES_COSTS_H_ */
