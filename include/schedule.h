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

void schedule_init(void);

bool schedule_parse_line(schedule_t *schedule, const char *line);
bool schedule_parsing_finished(schedule_t *schedule);

#endif /* SCHEDULE_H_ */
