/* © 2014 the PBNF Authors under the MIT license. See AUTHORS for the list of authors.*/

/**
 * \file nblock.h
 *
 *
 *
 * \author Ethan Burns
 * \date 2008-10-21
 */

#if !defined(_NBLOCK_H_)
#define _NBLOCK_H_

#include <iostream>
#include <set>

#include "../open_list.h"
#include "../queue_open_list.h"
#include "../closed_list.h"

#include "../pq_vector_open_list.h"

using namespace std;

namespace PSDD {

/**
 * An NBlock
 */
	struct NBlock {
		NBlock(const Projection *p, unsigned int id);

		~NBlock(void);

		void print(ostream &s) const;

		unsigned int id;
		unsigned int sigma;
		ClosedList closed;
		QueueOpenList open[2];
	//	PQVectorOpenList open[2];
		bool inuse;

		set<unsigned int> interferes;
		set<unsigned int> preds;
		set<unsigned int> succs;
	};

} /* PSDD */

#endif	/* !_NBLOCK_H_ */
