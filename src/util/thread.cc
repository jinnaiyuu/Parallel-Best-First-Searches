// © 2014 the PBNF Authors under the MIT license. See AUTHORS for the list of authors.

/**
 * \file thread.cc
 *
 *
 *
 * \author Ethan Burns
 * \date 2008-10-21
 */

#include <pthread.h>
#include <limits.h>
#include <string.h>

#include <iostream>

#include "thread.h"

using namespace std;

#if !defined(LINE_MAX)
#if !defined(_POSIX2_LINE_MAX)
#define LINE_MAX 2048
#else  // !_POSIX2_LINE_MAX
#define LINE_MAX _POSIX2_LINE_MAX
#endif	// _POSIX2_LINE_MAX
#endif	// LINE_MAX

unsigned int Thread::next_id = 0;

pthread_once_t Thread::init_current_key_once = PTHREAD_ONCE_INIT;
pthread_key_t Thread::current_key;

/**
 * Initializes the current_key field.
 */
void init_current_key(void)
{
	pthread_key_create(&Thread::current_key, NULL);
}

/**
 * Create a new thread.
 */
Thread::Thread(void)
{
	id = next_id;
	next_id += 1;
        pthread_mutex_init(&mutex, NULL);
        pthread_cond_init(&cond, NULL);
        do_exit = false;
        signalled=0;

	// initialize the current_key field... only do it once.
	pthread_once(&init_current_key_once, init_current_key);
}

Thread* Thread::current(void)
{
	Thread *current = (Thread*) pthread_getspecific(current_key);
	return current;
}

/**
 * Pthreads can't call methods of a class directly.  This function
 * just calls the run method of the class that is passed in as the
 * parameter.
 */
void *pthread_call_run(void *void_t)
{
	Thread *t = (Thread *) void_t;

	pthread_setspecific(t->current_key, void_t);
	t->run();

	return NULL;
}

/**
 * This gets rid of a cast warning with pthread_create
 */
extern "C" void *__pthread_call_run(void *void_t)
{
	return pthread_call_run(void_t);
}

/**
 * Virtual destructor... apparently.
 */
Thread::~Thread(void) {}

/**
 * Joins the thread.  This means that it waits for the thread to exit.
 * \return 0 on success, a negative error value on error.
 */
int Thread::join(void)
{
        pthread_mutex_lock(&mutex);
        int ret;
	do_exit = true;
	pthread_cond_signal(&cond);
        signalled++;
        pthread_mutex_unlock(&mutex);
	ret = pthread_join(pthread_id, NULL);
        return ret;
}

/**
 * Signal thread to start working again.
 */
void Thread::signal(void)
{
	pthread_mutex_lock(&mutex);
	pthread_cond_signal(&cond);
        signalled++;
	pthread_mutex_unlock(&mutex);
}

/**
 * Signal thread to wait on a condition.
 */
void Thread::wait(void)
{
	pthread_mutex_lock(&mutex);
        while(!signalled){
          pthread_cond_wait(&cond, &mutex);
        }
        signalled--;
	pthread_mutex_unlock(&mutex);
}

/**
 * Get the ID that the system uses to identfy the thread.
 */
pthread_t Thread::get_sys_id(void) const
{
	return pthread_id;
}

/**
 * Get the ID of this thread.
 */
unsigned int Thread::get_id(void) const
{
	return id;
}

/**
 * Start the new thread.
 */
int Thread::start(void)
{
	int ret;
	char buf[LINE_MAX];
	pthread_attr_t attr;

	pthread_attr_init(&attr);
	//pthread_attr_setscope(&attr, PTHREAD_SCOPE_SYSTEM);

	ret = pthread_create(&pthread_id, &attr, __pthread_call_run, this);
	if (ret < 0) {
		char* err_str = strerror_r(ret, buf, LINE_MAX);
		cerr << "Error starting thread: " << err_str << endl;
	}

	return ret;
}
