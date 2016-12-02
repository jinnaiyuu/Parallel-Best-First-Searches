/* © 2014 the PBNF Authors under the MIT license. See AUTHORS for the list of authors.*/

/**
 * \file nblock_map.h
 *
 * A map from unsigned int ids to nblocks where the nblocks are
 * created lazily upon request.
 *
 * \author Ethan Burns
 * \date 2009-02-18
 */

#if !defined(_NBLOCK_MAP_H_)
#define _NBLOCK_MAP_H_

// If this is defined then lazy nblock creation will not happen.
//#define UNLAZY

extern "C" {
#include "../lockfree/include/atomic.h"
}

#include "../projection.h"

template<class NB> class NBlockMap {
public:

	/**
	 * Observes the creation of an nblock.
	 */
	class CreationObserver {
	public:
		virtual ~CreationObserver() {}
		virtual void observe(NB *b) = 0;
	};

	NBlockMap(const Projection *p);
	~NBlockMap(void);

	NB *get(unsigned int);

	NB *find(unsigned int);

	unsigned int get_num_created(void);


	void set_observer(CreationObserver *o);

private:
	const Projection *project;
	NB **blocks;
	unsigned int num_nblocks;
	unsigned int num_created;

	CreationObserver *observer;
};

/**
 * Create a new NBlockMap that creates nblocks using the given
 * projection function.
 */
template<class NB>
NBlockMap<NB>::NBlockMap(const Projection *p)
{
	observer = NULL;
	project = p;
	num_nblocks = p->get_num_nblocks();
	blocks = new NB*[num_nblocks];

	for (unsigned int i = 0; i < num_nblocks; i += 1) {
#if defined(UNLAZY)
		blocks[i] = new NB(project, i);
#else
		blocks[i] = NULL;
#endif
	}

#if defined(UNLAZY)
	num_created = num_nblocks;
#else
	num_created = 0;
#endif
}

/**
 * Destruct the nblock map and delete all of its nblocks.
 */
template<class NB>
NBlockMap<NB>::~NBlockMap(void)
{
	for (unsigned int i = 0; i < num_nblocks; i += 1)
		if (blocks[i])
			delete blocks[i];
	delete[] blocks;
}

/**
 * Get the NBlock with the given ID.  If the NBlock is not yet
 * created, then create it.
 */
template<class NB>
NB *NBlockMap<NB>::get(unsigned int id)
{
#if defined(UNLAZY)
	return blocks[id];
#else
	if (!blocks[id]) {
		NB *b = new NB(project, id);
		if (compare_and_swap(&blocks[id], (intptr_t) NULL, (intptr_t) b)) {
			if (observer)
				observer->observe(blocks[id]);
			num_created += 1;
		} else {
			delete b;
		}
	}

	return blocks[id];
#endif
}

/**
 * Get the NBlock with the given ID if it has already been created, if
 * not return NULL.
 */
template<class NB>
NB *NBlockMap<NB>::find(unsigned int id)
{
	return blocks[id];
}

/**
 * Get the number of nblocks that were actually created.
 */
template<class NB>
unsigned int NBlockMap<NB>::get_num_created(void)
{
	return num_created;
}

/**
 * Set a class that observes the creation of new nblocks.
 */
template<class NB>
void NBlockMap<NB>::set_observer(CreationObserver *o)
{
	observer = o;
}


#endif /* !_NBLOCK_MAP_H_ */
