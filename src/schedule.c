/*
 * schedule.c
 *
 *  Created on: Mar 19, 2017
 *      Author: jnevens
 */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>

#include <eu/log.h>
#include <eu/list.h>
#include <eu/timer.h>

#include "event.h"
#include "schedule.h"
#include "utils_time.h"

#define X(a, b) b,
char *schedule_type_names[] = {
	SCHEDULE_TYPES_TABLE
	NULL
};
#undef X

eu_list_t *schedules = NULL;

struct schedule_entry_s {
	int day;
	int hour;
	int min;
	event_t *event;
};

struct schedule_s {
	schedule_type_e type;
	eu_list_t *entries;
	char *name;
};

schedule_entry_t *schedule_entry_create(int day, int hour, int min, event_t *event)
{
	schedule_entry_t *entry = calloc(1, sizeof(schedule_entry_t));

	entry->day = day;
	entry->hour = hour;
	entry->min = min;
	entry->event = event;

	return entry;
}

bool schedule_add_entry(schedule_t *schedule, schedule_entry_t *entry)
{
	eu_list_append(schedule->entries, entry);
	return true;
}

schedule_t *schedule_create(const char *name, const char *type)
{
	int i;
	schedule_t *schedule = calloc(1, sizeof(schedule_t));

	if (schedule) {
		for (i = 0; schedule_type_names[i]; i++) {
			if (strcmp(schedule_type_names[i], type) == 0) {
				schedule->type = i;
				break;
			}
		}
	}

	if (schedule_type_names[i] == NULL) {
		eu_log_err("invalid schedule type!");
		goto error;
	}

	schedule->entries = eu_list_create();
	schedule->name = strdup(name);

	eu_list_append(schedules, schedule);

	return schedule;
error: free(schedule);
	return NULL;
}


bool schedule_parse_line(schedule_t *schedule, const char *line)
{
	int rv = 0, day = 1, hour = 0, min = 0;
	char event[50];

	if (schedule->type == SCHEDULE_DAY) {
		if (sscanf(line, "%d %d %s %*c", &hour, &min, event) != 3) {
			return false;
		}
	} else if (schedule->type == SCHEDULE_WEEK) {
		if (sscanf(line, "%d %d %d %s %*c", &day, &hour, &min, event) != 4) {
			return false;
		}
	}

	event_type_e event_type = event_type_from_char(event);
	event_t *e = event_create(NULL, event_type);
	schedule_entry_t *entry = schedule_entry_create(day, hour, min, e);
	schedule_add_entry(schedule, entry);
	eu_log_debug("day: %d, hour: %d, min: %d, event %s", day, hour, min, event);
	return true;
}

static bool calculate_next_timing_event(void *arg)
{
	uint64_t until_next = (1000 * 60) - (get_current_time_ms() % (1000 * 60));
	eu_log_debug("schedule event in %d!", until_next);
	eu_event_timer_create(until_next, calculate_next_timing_event, NULL);
	return false;
}

void schedule_init(void)
{
	eu_log_info("Schedules init!");
	schedules = eu_list_create();
	calculate_next_timing_event(NULL);
}
