/* © 2014 the PBNF Authors under the MIT license. See AUTHORS for the list of authors.*/

/**
 * \file synch_pq_olist.h
 *
 *
 *
 * \author Ethan Burns
 * \date 2008-10-10
 */

#if !defined(_SYNCH_PQ_OLIST_H_)
#define _SYNCH_PQ_OLIST_H_

#include <pthread.h>

#include "pq_open_list.h"

/**
 * A thread safe PQ OpenList implementation.
 */
template<class PQCompare>
	class SynchPQOList : public PQOpenList<PQCompare> {
public:
	SynchPQOList(void);

	virtual void add(State *s);
	virtual State *take(void);
	virtual State *peek(void);
	virtual bool empty(void);
	virtual void delete_all_states(void);
	virtual void prune(void);
	virtual fp_type get_best_val(void);

	virtual void see_update(State *s);
private:
	pthread_mutex_t mutex;
};

template<class PQCompare>
	SynchPQOList<PQCompare>::SynchPQOList(void) {
	pthread_mutex_init(&mutex, NULL);
}

template<class PQCompare>
	void SynchPQOList<PQCompare>::add(State *s)
{
	pthread_mutex_lock(&mutex);
	PQOpenList<PQCompare>::add(s);
	pthread_mutex_unlock(&mutex);
}

template<class PQCompare>
State *SynchPQOList<PQCompare>::take(void)
{
	State *ret;

	pthread_mutex_lock(&mutex);
	ret = PQOpenList<PQCompare>::take();
	pthread_mutex_unlock(&mutex);

	return ret;
}

template<class PQCompare>
State *SynchPQOList<PQCompare>::peek(void)
{
	State *ret;

	pthread_mutex_lock(&mutex);
	ret = PQOpenList<PQCompare>::peek();
	pthread_mutex_unlock(&mutex);

	return ret;
}

template<class PQCompare>
bool SynchPQOList<PQCompare>::empty(void)
{
	bool ret;

	pthread_mutex_lock(&mutex);
	ret = PQOpenList<PQCompare>::empty();
	pthread_mutex_unlock(&mutex);

	return ret;
}

template<class PQCompare>
void SynchPQOList<PQCompare>::delete_all_states(void)
{
	pthread_mutex_lock(&mutex);
	PQOpenList<PQCompare>::delete_all_states();
	pthread_mutex_unlock(&mutex);
}

template<class PQCompare>
void SynchPQOList<PQCompare>::prune(void)
{
	pthread_mutex_lock(&mutex);
	PQOpenList<PQCompare>::prune();
	pthread_mutex_unlock(&mutex);
}

template<class PQCompare>
fp_type SynchPQOList<PQCompare>::get_best_val(void)
{
	fp_type ret;

	pthread_mutex_lock(&mutex);
	ret = PQOpenList<PQCompare>::get_best_val();
	pthread_mutex_unlock(&mutex);

	return ret;
}

template<class PQCompare>
void SynchPQOList<PQCompare>::see_update(State *s)

{
	pthread_mutex_lock(&mutex);
	if (!s->is_open())
		/* it is possible that someone removed this before we
		 * were able to update it... just re-add it. */
		PQOpenList<PQCompare>::add(s);
	else
		PQOpenList<PQCompare>::see_update(s);
	pthread_mutex_unlock(&mutex);
}

#endif	/* !_SYNCH_PQ_OLIST_H_ */
