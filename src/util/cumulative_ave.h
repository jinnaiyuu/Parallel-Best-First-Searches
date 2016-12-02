/* © 2014 the PBNF Authors under the MIT license. See AUTHORS for the list of authors.*/

/**
 * \file cumulative_ave.h
 *
 *
 *
 * \author Ethan Burns
 * \date 2008-12-24
 */

#if !defined(_CUMULATIVE_AVE_H_)
#define _CUMULATIVE_AVE_H_

/**
 * Maybe this is overkill... but I like it
 */
class CumulativeAverage {
public:
	CumulativeAverage(void);
	void add_val(unsigned long val);
	float read(void);
	unsigned long get_num(void);
	void reset(void);
private:
	float ave;
	unsigned long num;
};

#endif	/* _CUMULATIVE_AVE_H_ */
