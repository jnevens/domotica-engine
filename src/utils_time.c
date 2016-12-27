/*
 * utils_time.c
 *
 *  Created on: Dec 23, 2016
 *      Author: jnevens
 */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/time.h>

#include "utils_time.h"

uint64_t get_current_time_ms(void)
{
	uint64_t ret;
	struct timeval now;
	gettimeofday(&now,NULL);

	ret = now.tv_sec;
	ret *= 1000;
	ret += now.tv_usec / 1000;

	return  ret;
}

uint64_t get_current_time_us(void)
{
	uint64_t ret;
	struct timeval now;
	gettimeofday(&now,NULL);

	ret = now.tv_sec;
	ret *= 1000000;
	ret += now.tv_usec;

	return  ret;
}
