/* © 2014 the PBNF Authors under the MIT license. See AUTHORS for the list of authors.*/

/**
 * \file eps.h
 *
 *
 *
 * \author Ethan Burns
 * \date 2008-10-13
 */

#if !defined(_EPS_H_)
#define _EPS_H_

#include <string>

using namespace std;

/**
 * An encapsulated post script image.
 */
class EPS {
public:
	EPS(int width, int height);
	EPS(int width, int height, double scale);
	EPS(int width, int height, double scale, string data);
	void set_scale(double s);
	void set_data(const string d);
	void output(string file);
private:
	double point_of_pixel(int px) const;
	string run_length_encode(string in) const;

	int width, height;
	double scale;
	string data;
};

#endif	/* !_EPS_H_ */
