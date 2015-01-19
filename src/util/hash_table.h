/* © 2014 the PBNF Authors under the MIT license. See AUTHORS for the list of authors.*/

/**
 * \file hash_table.h
 *
 *
 *
 * \author eaburns
 * \date 2009-04-19
 */
#if !defined(_HASH_TABLE_H_)
#define _HASH_TABLE_H_

#include <list>
#include <stdlib.h>
#include <stdint.h>

using namespace std;

/**
 * A simple hash table class.
 */
template <class Elm> class HashTable {
public:
	HashTable(void);
	HashTable(unsigned long size);
	~HashTable(void);
	void add(Elm *);
	void remove(Elm *);
	Elm *lookup(Elm *);
	void delete_all(void);
	bool empty();

	unsigned long get_fill(void);

	void prune(void);
	void for_each(void (*f)(void*, Elm*), void*);

private:
	void init(unsigned long size);
	void new_table(void);
	void resize(void);
	void do_add(Elm *s);
	unsigned long get_ind(Elm *s);

	class Bucket {
	public:
		Bucket(Elm *data, Bucket *next);
		~Bucket(void);

		Elm *lookup(Elm *s);

		/*
		 * This really returns a Bucket*, but I couldn't get
		 * the templates to actually do that.
		 */
		void *add(Elm *s);

		Elm *data;
		Bucket *next;
		unsigned int size;
	};

	Bucket **table;
	unsigned long size;
	unsigned long fill;
};

template<class Elm>
HashTable<Elm>::Bucket::Bucket(Elm *d, HashTable<Elm>::Bucket *n)
{
	data = d;
	next = n;
	if (!next)
		size = 0;
	else
		size = next->size + 1;
}

template<class Elm>
HashTable<Elm>::Bucket::~Bucket(void)
{
}

template<class Elm>
Elm *HashTable<Elm>::Bucket::lookup(Elm *s)
{
	if (s->hash() == data->hash())
		return data;
	else if (data->hash() > s->hash())
		return NULL;
	else if (!next)
		return NULL;

	return next->lookup(s);
}

template<class Elm>
void* HashTable<Elm>::Bucket::add(Elm *s)
{
	if (s->hash() < data->hash())
		return new Bucket(s, this);
	else if (s->hash () == data->hash())
		data = s;
	else if (!next)
		next = new Bucket(s, NULL);
	else {
		next = (Bucket*) next->add(s);
		size += 1;
	}

	return this;
}

template<class Elm>
void HashTable<Elm>::init(unsigned long s)
{
	this->table = NULL;
	this->size = s;
	fill = 0;
}

template<class Elm>
HashTable<Elm>::HashTable(void)
{
	// TODO: This is way too naive. Should adjust to the domain, at least take prime number.
	init(1000000); //9999943
}

template<class Elm>
HashTable<Elm>::HashTable(unsigned long s)
{
	init(s);
}

template<class Elm>
HashTable<Elm>::~HashTable(void)
{
	prune();
}

/**
 * Initializes the table if it hasn't been created yet
 */
template<class Elm>
void HashTable<Elm>::new_table(void)
{
	if (!table)
		table = (Bucket **) calloc(size, sizeof(Bucket *));
}

/**
 * Add to the hash table.
 * \param s The state to add.
 */
template<class Elm>
void HashTable<Elm>::add(Elm *s)
{
	new_table();
	if (fill + 1 >= size / 3)
		resize();

	do_add(s);
	fill += 1;
}

template<class Elm>
void HashTable<Elm>::remove(Elm *s)
{
	unsigned long i = get_ind(s);
	Bucket *b = table[i];
	if (!b)
		return;

	uint64_t hash = s->hash();
	Bucket *prev = b;
	while (b) {
		if (b->data->hash() == hash)
			break;
		prev = b;
		b = b->next;
	}

	if (b) {
		if (table[i] == b)
			table[i] = b->next;
		else
			prev->next = b->next;
		delete b;
		fill -= 1;
	}

	assert(lookup(s) == NULL);
}

/**
 * Grow the table and rehash everything.
 */
template<class Elm>
void HashTable<Elm>::resize(void)
{
	Bucket *b;
	Bucket**old_table = table;
	unsigned int old_size;


	old_size = size;
	size = size * 10;
	table = NULL;
	new_table();

	for (unsigned int i = 0; i < old_size; i += 1) {
		for (b = old_table[i]; b; b = b->next)
			do_add(b->data);
		if (old_table[i])
			delete old_table[i];
	}

	free(old_table);
}

/**
 * Get the table index for the given state.
 */
template<class Elm>
unsigned long HashTable<Elm>::get_ind(Elm *s)
{
	return s->hash() % size;
}

/**
 * Add a state to the table.
 */
template<class Elm>
void HashTable<Elm>::do_add(Elm *s)
{
	Bucket *old = table[get_ind(s)];
	if (!old)
		table[get_ind(s)] = new Bucket(s, NULL);
	else
		table[get_ind(s)] = (Bucket*) old->add(s);

}

/**
 * Lookup a state in the hash table.
 * \param c The state to look up.
 * \return The state if it was found, NULL on error.
 */
template<class Elm>
Elm *HashTable<Elm>::lookup(Elm *c)
{
	if (!table)
		return NULL;

	if (table[get_ind(c)])
		return table[get_ind(c)]->lookup(c);
	else
		return NULL;
}

/**
 * Delete (free up the memory for) all entries in the hash table.
 */
template<class Elm>
void HashTable<Elm>::delete_all(void)
{
	Bucket *b;

	if (!table)
		return;

	for (unsigned int i = 0; i < size; i += 1) {
		for (b = table[i]; b; b = b->next)
			delete b->data;
		Bucket *b = table[i];
		while (b) {
			Bucket *c = b;
			b = b->next;
			delete c;
		}
		table[i] = NULL;
	}
}

/**
 * Drop all entries, but don't delete them.
 */
template<class Elm>
void HashTable<Elm>::prune(void)
{
	if (!table)
		return;

	for (unsigned int i = 0; i < size; i += 1) {
		if (table[i]) {
			Bucket *b = table[i];
			while (b) {
				Bucket *c = b;
				b = b->next;
				delete c;
			}
			table[i] = NULL;
		}
	}
	free(table);

	this->table = NULL;
	fill = 0;
}

/**
 * Test if the hash table is empty
 */
template<class Elm>
bool HashTable<Elm>::empty()
{
	return fill == 0;
}

template<class Elm>
void HashTable<Elm>::for_each(void (*f)(void*, Elm*), void *aux)
{
	if (!table)
		return;

	for (unsigned int i = 0; i < size; i += 1) {
		Bucket *b;
		for (b = table[i]; b; b = b->next)
			f(aux, b->data);
	}
}

template<class Elm>
unsigned long HashTable<Elm>::get_fill(void)
{
	return fill;
}

#endif	/* !_HASH_TABLE_H_ */
