/* © 2014 the PBNF Authors under the MIT license. See AUTHORS for the list of authors.*/

/**
 * \file thread.h
 *
 *
 *
 * \author Ethan Burns
 * \date 2008-10-21
 */

#if !defined(_THREAD_H_)
#define _THREAD_H_

#include <pthread.h>

class Thread {
public:
	Thread(void);
	virtual ~Thread(void);

	int join(void);
	int start(void);
	pthread_t get_sys_id(void) const;
	unsigned int get_id(void) const;
        void signal(void);
        void wait(void);

	virtual void run(void) = 0;

	/* Get the current thread. */
	static Thread *current(void);

protected:
        pthread_mutex_t mutex; //cond and mutex for thread waits
        pthread_cond_t cond;
        bool do_exit; //set to true in join()

private:
	friend void *pthread_call_run(void *);
	friend void init_current_key(void);

	pthread_t pthread_id;
	unsigned int id;

        unsigned int signalled;

	/* The next available ID number */
	static unsigned int next_id;

	/* Thread-local storage for the current() function return. */
	static pthread_once_t init_current_key_once;
	static pthread_key_t current_key;
};

#endif	/* !_THREAD_H_ */
