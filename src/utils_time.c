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
#include <time.h>
#include <errno.h>

#include "utils_time.h"



void busy_wait_microseconds(uint32_t micros)
{
	// Set delay time period.
	struct timeval deltatime;
	deltatime.tv_sec = micros / 1000000;
	deltatime.tv_usec = (micros % 1000000);
	struct timeval walltime;
	// Get current time and add delay to find end time.
	gettimeofday(&walltime, NULL);
	struct timeval endtime;
	timeradd(&walltime, &deltatime, &endtime);
	// Tight loop to waste time (and CPU) until enough time as elapsed.
	while (timercmp(&walltime, &endtime, <)) {
		gettimeofday(&walltime, NULL);
	}
}

void busy_wait_milliseconds(uint32_t millis)
{
	busy_wait_microseconds(millis * 1000);
}

void sleep_milliseconds(uint32_t millis)
{
	struct timespec sleep;
	sleep.tv_sec = millis / 1000;
	sleep.tv_nsec = (millis % 1000) * 1000000L;
	while (clock_nanosleep(CLOCK_MONOTONIC, 0, &sleep, &sleep) && errno == EINTR)
		;
}

uint64_t get_current_time_s(void)
{
	struct timeval now;
	gettimeofday(&now, NULL);

	return now.tv_sec;
}

uint64_t get_current_time_ms(void)
{
	uint64_t ret;
	struct timeval now;
	gettimeofday(&now, NULL);

	ret = now.tv_sec;
	ret *= 1000;
	ret += now.tv_usec / 1000;

	return ret;
}

uint64_t get_current_time_us(void)
{
	uint64_t ret;
	struct timeval now;
	gettimeofday(&now, NULL);

	ret = now.tv_sec;
	ret *= 1000000;
	ret += now.tv_usec;

	return ret;
}

const char *get_current_timezone(void)
{
	static char *timezone = NULL;
	time_t t = time(NULL);
	struct tm lt = { 0 };

	localtime_r(&t, &lt);

	timezone = strdup(lt.tm_zone);
	return timezone;
}

int32_t get_current_timezone_offset(void)
{
	time_t t = time(NULL);
	struct tm lt = { 0 };

	localtime_r(&t, &lt);

	return lt.tm_gmtoff;
}
