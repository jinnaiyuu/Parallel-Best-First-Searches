// © 2014 the PBNF Authors under the MIT license. See AUTHORS for the list of authors.

/**
 * \file timeout.cc
 *
 *
 *
 * \author Ethan Burns
 * \date 2008-12-16
 */

#include <iostream>

#include <string.h>
#include <signal.h>
#include <stdlib.h>
#include <unistd.h>

#include "../search.h"

using namespace std;

static volatile bool timeout_stopped = false;

static unsigned int timelimit = 0;

extern "C" void alarm_action(int sig)
{
	if (timeout_stopped)
		return;

	cout << "# Timeout" << endl;

	output_search_stats_on_timeout();

	cout << "time-limit: " << timelimit << endl
	     << "cost: infinity" << endl
	     << "length: infinity" << endl
	     << "wall_time: infinity" << endl
	     << "CPU_time: infinity" << endl
	     << "generated: infinity" << endl
	     << "expanded: infinity" << endl;

	cout.flush();
	cerr.flush();

	_exit(EXIT_SUCCESS);
	exit(EXIT_SUCCESS);
}

void timeout(unsigned int sec)
{
	struct sigaction sa;

	timelimit = sec;

	memset(&sa, '\0', sizeof(sa));
	sa.sa_handler = alarm_action;
	sigfillset(&sa.sa_mask);
	sigaction(SIGALRM, &sa, NULL);

	timeout_stopped = false;
	unsigned int err = alarm(sec);
	if (err) {
		cerr << "Failed to set the alarm: " << err << endl;
		abort();
	}
}

void timeout_stop(void)
{
	timeout_stopped = true;
}
