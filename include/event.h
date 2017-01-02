/*
 * event.h
 *
 *  Created on: Dec 23, 2016
 *      Author: jnevens
 */

#ifndef EVENT_H_
#define EVENT_H_

#include <stdbool.h>
#include "types.h"

#define EVENT_TYPE_TABLE \
	X(EVENT_PRESS, "PRESS" ) \
	X(EVENT_RELEASE, "RELEASE" ) \
	X(EVENT_SHORT_PRESS, "SHORT_PRESS" ) \
	X(EVENT_LONG_PRESS, "LONG_PRESS" ) \
	X(EVENT_DIM, "DIM" )

#define X(a,b) a,
enum event_type {
	EVENT_TYPE_TABLE
};
#undef X

enum event_src_type {
	EVENT_SRC_INPUT
};

const char *event_get_name(event_t *event);
device_t *event_get_device(event_t *event);

bool event_equal(event_t *event1, event_t *event2);

event_t *event_create(device_t *device, event_type_e event);
void event_delete(event_t *event); // event_destroy exist in bus/event.h

event_type_e event_type_from_char(const char *name);

void event_print(event_t *event);

#endif /* EVENT_H_ */
