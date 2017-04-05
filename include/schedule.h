/*
 * schedule.h
 *
 *  Created on: Mar 19, 2017
 *      Author: jnevens
 */

#ifndef SCHEDULE_H_
#define SCHEDULE_H_

#include <stdbool.h>
#include "types.h"

#define SCHEDULE_TYPES_TABLE \
	X(SCHEDULE_DAY, "DAYLY") \
	X(SCHEDULE_WEEK, "WEEKLY") \

#define X(a, b) a,
typedef enum {
	SCHEDULE_TYPES_TABLE
} schedule_type_e;
#undef X

schedule_t *schedule_create(const char *name, const char *type);
bool schedule_parse_line(schedule_t *schedule, const char *line);

#endif /* SCHEDULE_H_ */
