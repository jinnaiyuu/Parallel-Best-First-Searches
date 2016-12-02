/* © 2014 the PBNF Authors under the MIT license. See AUTHORS for the list of authors.*/

/**
 * \file msg_buffer.h
 *
 * Async communication between a producer and a consumer thread.
 *
 * The way this works is that the producer thread attempts to take the
 * lock on the recive queue of the consumer thread.  If the lock is
 * taken successfully the messages are put directly on the consumer's
 * receive queue.  If the lock fails to be taken, instead of waiting,
 * the message is buffered until a later time when it can be flushed.
 *
 * \author eaburns
 * \date 2009-07-14
 */

#if !defined(_MSG_BUFFER_H_)
#define _MSG_BUFFER_H_

#include <errno.h>

#include <vector>

#include "mutex.h"

using namespace std;

template <class Msg>
class MsgBuffer {
private:

	/**
	 * Flushes the buffer to the remote queue.  This assumes that
	 * the caller will handle the locking/unlocking of the mutex.
	 */
	void __flush(void) {
		for (unsigned int i = 0; i < buffer.size(); i += 1) {
			Msg m = buffer[i];
			queue->push_back(m);
		}
		if (!buffer.empty())
			post_send(data);
		buffer.clear();
	}

public:

	/**
	 * Creates a new message buffer
	 */
	MsgBuffer(Mutex *m, vector<Msg> *q,
		  void (*ps)(void*), void *d) {
		mutex = m;
		queue = q;
		post_send = ps;
		data = d;
	}

	/**
	 * Send a message to the remote queue.  If the lock is not
	 * available, the message is buffered until the next send
	 * occurs, or a flush operation is performed.
	 */
	bool try_send(Msg m) {
		buffer.push_back(m);
		if (!mutex->try_lock()) {
			return false;
		} else {
			__flush();
			mutex->unlock();
			return true;
		}
	}

	/**
	 * Force a send to the remote queue.  This will wait on the
	 * lock until it is available for the send.
	 */
	void force_send(Msg m) {
		mutex->lock();
		buffer.push_back(m);
		__flush();
		mutex->unlock();
	}

	/**
	 * Tries to flush the queue to the remote peer.
	 */
	bool try_flush(void) {

		if (buffer.empty())
			return false;

		if (mutex->try_lock()) {
			__flush();
			mutex->unlock();
			return true;
		}

		return false;
	}

	/**
	 * Forces a flush to the remote peer.
	 */
	void force_flush(void) {
		mutex->lock();
		__flush();
		mutex->unlock();
	}

	/**
	 * Test if the buffer is empty.
	 */
	bool is_empty(void) {
		return buffer.empty();
	}

private:
	/* The lock on the message queue. */
	Mutex *mutex;

	/* The queue to send messages */
	vector<Msg> *queue;

	/* Called when a message is sent (with the mutex held). */
	void (*post_send)(void *);

	/* Data passed to post_send. */
	void *data;

	/* A local buffer for messages. */
	vector<Msg> buffer;
};

#endif /* _MSG_BUFFER_H_ */
