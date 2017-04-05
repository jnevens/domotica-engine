/*
 * schedule.c
 *
 *  Created on: Mar 19, 2017
 *      Author: jnevens
 */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <bus/log.h>

#include "schedule.h"

#define X(a, b) b,
char *schedule_type_names[] = {
	SCHEDULE_TYPES_TABLE
	NULL
};
#undef X


struct schedule_s {
	schedule_type_e type;
	char *name;
};

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
		log_err("invalid schedule type!");
		goto error;
	}

	schedule->name = strdup(name);

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

	log_debug("day: %d, hour: %d, min: %d, event %s", day, hour, min, event);
	return true;
}
