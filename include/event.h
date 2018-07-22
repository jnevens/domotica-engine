/*
 * event.h
 *
 *  Created on: Dec 23, 2016
 *      Author: jnevens
 */

#ifndef EVENT_H_
#define EVENT_H_

#include <stdbool.h>

#include <eu/variant.h>

#include "types.h"

#define EVENT_TYPE_TABLE \
	X(0,	EVENT_PRESS,			"PRESS" ) \
	X(1,	EVENT_SET,				"SET" ) \
	X(2,	EVENT_UNSET,			"UNSET" ) \
	X(3,	EVENT_RESET,			"RESET" ) \
	X(4,	EVENT_RELEASE,			"RELEASE" ) \
	X(5,	EVENT_SHORT_PRESS,		"SHORT_PRESS" ) \
	X(6,	EVENT_LONG_PRESS,		"LONG_PRESS" ) \
	X(7,	EVENT_DIM,				"DIM" ) \
	X(8,	EVENT_TIMEOUT,			"TIMEOUT" ) \
	X(9,	EVENT_SUNRISE,			"SUNRISE" ) \
	X(10,	EVENT_SUNRISE_CIV,		"SUNRISE_CIVIL" ) \
	X(11,	EVENT_SUNRISE_NAUT,		"SUNRISE_NAUTICAL" ) \
	X(12,	EVENT_SUNRISE_ASTRO,	"SUNRISE_ASTRONOMICAL" ) \
	X(13,	EVENT_SUNSET,			"SUNSET" ) \
	X(14,	EVENT_SUNSET_CIV,		"SUNSET_CIVIL" ) \
	X(15,	EVENT_SUNSET_NAUT,		"SUNSET_NAUTICAL" ) \
	X(16,	EVENT_SUNSET_ASTRO,		"SUNSET_ASTRONOMICAL" ) \
	X(17,	EVENT_SUN_POSITION,		"SUN_POSITION" ) \
	X(18,	EVENT_TEMPERATURE,		"TEMPERATURE" ) \
	X(19,	EVENT_HUMIDITY,			"HUMIDITY" ) \

#define X(a,b,c) b = (1 << a),
enum event_type {
	EVENT_TYPE_TABLE
};
#undef X

enum event_src_type {
	EVENT_SRC_INPUT
};

const char *event_get_name(event_t *event);
device_t *event_get_device(event_t *event);
event_type_e event_get_type(event_t *event);

bool event_equal(event_t *event1, event_t *event2);

event_t *event_create(device_t *device, event_type_e event);
void event_destroy(event_t *event); // event_destroy exist in bus/event.h

void event_option_set(event_t *event, const char *name, eu_variant_t *var);
void event_option_set_int32(event_t *event, const char *name, int val);
void event_option_set_double(event_t *event, const char *name, double val);

eu_variant_t *event_option_get(event_t *event, const char *name);
int event_option_get_int32(event_t *event, const char *name);
double event_option_get_double(event_t *event, const char *name);

event_type_e event_type_from_char(const char *name);

void event_print(event_t *event);

#endif /* EVENT_H_ */
