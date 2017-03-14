/*
 * utils_sched.c
 *
 *  Created on: Feb 8, 2017
 *      Author: jnevens
 */
#include <errno.h>
#include <sched.h>
#include <string.h>

#include "utils_sched.h"

void sched_set_max_priority(void)
{
	struct sched_param sched;
	memset(&sched, 0, sizeof(sched));
	// Use FIFO scheduler with highest priority for the lowest chance of the kernel context switching.
	sched.sched_priority = sched_get_priority_max(SCHED_FIFO);
	sched_setscheduler(0, SCHED_FIFO, &sched);
}

void sched_set_default_priority(void)
{
	struct sched_param sched;
	memset(&sched, 0, sizeof(sched));
	// Go back to default scheduler with default 0 priority.
	sched.sched_priority = 0;
	sched_setscheduler(0, SCHED_OTHER, &sched);
}
