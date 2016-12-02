// © 2014 the PBNF Authors under the MIT license. See AUTHORS for the list of authors.

/**
 * \file eps.cc
 *
 *
 *
 * \author Ethan Burns
 * \date 2008-10-13
 */

#include <iostream>
#include <fstream>
#include <string>

#include "eps.h"

using namespace std;

EPS::EPS(int width, int height) : width(width), height(height), scale(1.0) {}

EPS::EPS(int width, int height, double scale)
  : width(width), height(height), scale(scale) {}

EPS::EPS(int width, int height, double scale, string data)
	: width(width), height(height), scale(scale), data(data) {}

/**
 * Get the RunLengthEncode'd version of the given input string.
 */
string EPS::run_length_encode(string in) const
{
	string buffer, out;

	for (unsigned int i = 0; i < in.size(); i += 1) {
		buffer += in[i];

		// Buffer Full
		if (buffer.size() == 127) {
			out += buffer.size() - 1;
			out += buffer;
			buffer.clear();
		}

		// Run found
		if (buffer.size() >= 3
		    && (buffer[buffer.size() - 3]
			== buffer[buffer.size() - 2])
		    && (buffer[buffer.size() - 3]
			== buffer[buffer.size() - 1])) {
			if (buffer.size() > 3) {
				out += buffer.size() - 4;
				out += buffer.substr(0, buffer.size() - 3);
			}

			buffer.clear();

			int run = 3;
			char c = in[i];
			do {
				run += 1;
				i += 1;
			} while(run != 129 && in[i] == c && i < in.size());
			i -= 1;
			out += 2 - run;
			out += c;
		}
	}

	if (buffer.size()) {
		out += buffer.size() - 1;
		out += buffer;
	}

	return out;
}

/**
 * Convert a pixel location to a post script point.
 */
double EPS::point_of_pixel(int px) const
{
	return ((double) px) * 0.72;
}

/**
 * Set the scale.
 */
void EPS::set_scale(double s)
{
	scale = s;
}

/**
 * Set the image data.
 */
void EPS::set_data(const string d)
{
	data = d;
}

/**
 * Output the .eps to the given file.
 */
void EPS::output(string file)
{
	ofstream o(file.c_str());

	// PS and EPS header comments
	o << "%!PS-Adobe-3.0" << endl;
	o << "%%Creator: GridWorld::export_eps" << endl;
	o << "%%Title: " << file << endl;
	o << "%%BoundingBox: 0 0 "
	  << point_of_pixel(width) * scale << " "
	  << point_of_pixel(height) * scale << endl;
	o << "%%EndComments" << endl;

	// scale the image properly
	o << point_of_pixel(width) * scale << " "
	  << point_of_pixel(height) * scale << " scale" << endl;

	// The image
	o << width << endl;	// width
	o << height << endl;	// height
	o << "8" << endl;	// color depth
	o << "[" << width << " 0 0 " << height << " 0 0]" << endl;
	o << "/datasource currentfile /RunLengthDecode filter def" << endl;
	o << "/datastring " << width * 3 << " string def" << endl;
	o << "{datasource datastring readstring pop}" << endl;
	o << "false" << endl;
	o << "3" << endl;
	o << "colorimage" << endl;

	o << run_length_encode(data) << endl;
	o << "showpage" << endl;

	o.close();
}
