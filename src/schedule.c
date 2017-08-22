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
#include <time.h>

#include <eu/log.h>
#include <eu/list.h>
#include <eu/timer.h>

#include "device.h"
#include "event.h"
#include "action.h"
#include "engine.h"
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
	int day;	/* sun 0 -> sat 6*/
	int hour;
	int min;
	event_t *event;
};

struct schedule_s {
	schedule_type_e type;
	eu_list_t *entries;
	char *name;
	device_t *device;
};

static schedule_entry_t *schedule_entry_create(int day, int hour, int min, event_t *event)
{
	schedule_entry_t *entry = calloc(1, sizeof(schedule_entry_t));

	entry->day = (day != 7) ? day : 0;
	entry->hour = hour;
	entry->min = min;
	entry->event = event;

	return entry;
}

static bool schedule_add_entry(schedule_t *schedule, schedule_entry_t *entry)
{
	eu_list_append(schedule->entries, entry);
	return true;
}

static schedule_t *schedule_create(const char *name, const char *type)
{
	int i = 0;
	schedule_t *schedule = calloc(1, sizeof(schedule_t));

	if (schedule) {
		for (i = 0; schedule_type_names[i]; i++) {
			if (strcmp(schedule_type_names[i], type) == 0) {
				schedule->type = i;
				break;
			}
		}

		if (schedule_type_names[i] == NULL) {
			eu_log_err("invalid schedule type!");
			goto error;
		}

		schedule->entries = eu_list_create();
		schedule->name = strdup(name);

		eu_list_append(schedules, schedule);
	}

	return schedule;
error:
	free(schedule);
	return NULL;
}


bool schedule_parse_line(schedule_t *schedule, const char *line)
{
	int day = 1, hour = 0, min = 0;
	char event[50];

	if (schedule->type == SCHEDULE_DAY) {
		if (sscanf(line, "%d %d %49s %*c", &hour, &min, event) != 3) {
			return false;
		}
	} else if (schedule->type == SCHEDULE_WEEK) {
		if (sscanf(line, "%d %d %d %49s %*c", &day, &hour, &min, event) != 4) {
			return false;
		}
	}

	event_type_e event_type = event_type_from_char(event);
	event_t *e = event_create(schedule->device, event_type);
	schedule_entry_t *entry = schedule_entry_create(day, hour, min, e);
	schedule_add_entry(schedule, entry);
	eu_log_debug("day: %d, hour: %d, min: %d, event %s", day, hour, min, event);
	return true;
}

static schedule_entry_t *schedule_search_entry(schedule_t *schedule, int day, int hour, int min)
{
	eu_list_node_t *node;

	eu_list_for_each(node, schedule->entries) {
		schedule_entry_t *entry = eu_list_node_data(node);
		if (schedule->type == SCHEDULE_DAY) {
			if (entry->hour == hour && entry->min == min) {
				return entry;
			}
		} else if (schedule->type == SCHEDULE_WEEK) {
			if (entry->day == day && entry->hour == hour && entry->min == min) {
				return entry;
			}
		}
	}

	return NULL;
}

static void schedule_check_event(schedule_t *schedule)
{
	time_t t = time(NULL);
	struct tm *ts = localtime(&t);

	//eu_log_debug("schedule: %s %d %d:%d", schedule->name, ts->tm_wday, ts->tm_hour, ts->tm_min);

	schedule_entry_t *entry = schedule_search_entry(schedule, ts->tm_wday, ts->tm_hour, ts->tm_min);
	if (entry) {
		engine_trigger_event(entry->event);
	}
}

static void schedule_check_events(void)
{
	eu_list_node_t *node;
	eu_list_for_each(node, schedules) {
		schedule_check_event(eu_list_node_data(node));
	}
}

static bool calculate_next_timing_event(void *arg)
{
	uint64_t until_next = (1000 * 60) - (get_current_time_ms() % (1000 * 60));
	// eu_log_debug("schedule event in %d!", until_next);
	eu_event_timer_create(until_next, calculate_next_timing_event, (void *)0xdeadbeef);

	if (arg != NULL) {
		schedule_check_events();
	}
	return false;
}

static bool schedule_parser(device_t *device, char *options[])
{
	eu_log_debug("Create schedule: %s %s", device_get_name(device), options[0]);
	schedule_t *schedule = schedule_create(device_get_name(device), options[0]);
	if (schedule) {
		device_set_userdata(device, schedule);
		schedule->device = device;
		return true;
	}
	return false;
}

static bool schedule_exec(device_t *device, action_t *action)
{
	return false;
}

static device_type_info_t schedule_info = {
	.name = "SCHEDULE",
	.events = 0,
	.actions = 0,
	.conditions = 0,
	.check_cb = NULL,
	.parse_cb = schedule_parser,
	.exec_cb = schedule_exec,
};

void schedule_init(void)
{
	eu_log_info("Schedules init!");

	device_type_register(&schedule_info);

	schedules = eu_list_create();
	calculate_next_timing_event(NULL);
}
