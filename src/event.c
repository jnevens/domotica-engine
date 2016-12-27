/*
 * event.c
 *
 *  Created on: Dec 23, 2016
 *      Author: jnevens
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <bus/log.h>

#include "input.h"
#include "event.h"

#define X(a, b) b,
static char *event_type_name[] = {
		EVENT_TYPE_TABLE
		NULL
};
#undef X

struct event_s {
	enum event_src_type src_type;
	void *src;
	event_type_e type;
	// event options ...
};

event_t *event_create_input(input_t *input, event_type_e event_type)
{
	event_t *event = calloc(1, sizeof(event_t));
	event->src_type = EVENT_SRC_INPUT;
	event->src = input;
	event->type = event_type;

	return event;
}

void event_delete(event_t *event)
{
	free(event);
}

const char *event_get_name(event_t *event)
{
	return event_type_name[event->type];
}

input_t *event_get_input(event_t *event)
{
	return (input_t *) event->src;
}

bool event_equal(event_t *event1, event_t *event2)
{
	if (event1->src_type != event2->src_type) {
		return false;
	}
	if (event1->src != event2->src) {
		return false;
	}
	if (event1->type != event2->type) {
		return false;
	}
	return true;
}

event_type_e event_type_from_char(const char *name) {
	int i = 0;

	if (name == NULL)
		return -1;

	while (event_type_name[i]) {
		if (strcmp(event_type_name[i], name) == 0) {
			return i;
		}
		i++;
	}
	log_err("Cannot find event type: %s", name);
	return -1;
}

void event_print(event_t *event)
{
	input_t *input = (input_t *) event->src;
	log_info("event: input: %s, type: %s", input_get_name(input), event_get_name(event));
}
