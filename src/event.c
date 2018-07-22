/*
 * event.c
 *
 *  Created on: Dec 23, 2016
 *      Author: jnevens
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <eu/log.h>
#include <eu/variant.h>
#include <eu/variant_map.h>

#include "device.h"
#include "event.h"

#define X(a, b, c) c,
static char *event_type_name[] = {
		EVENT_TYPE_TABLE
		NULL
};
#undef X

struct event_s {
	enum event_src_type src_type;
	device_t *device;
	event_type_e type;
	eu_variant_map_t *options;
};

event_t *event_create(device_t *device, event_type_e event_type)
{
	event_t *event = calloc(1, sizeof(event_t));
	event->src_type = EVENT_SRC_INPUT;
	event->device = device;
	event->type = event_type;
	event->options = eu_variant_map_create();

	return event;
}

void event_destroy(event_t *event)
{
	eu_variant_map_destroy(event->options);
	free(event);
}

const char *event_get_name(event_t *event)
{
	int i;
	event_type_e type = event->type;
	const char *type_name = NULL;
	for(i = 0; event_type_name[i] != NULL; i++) {
		if (type & 0x1) {
			type_name = event_type_name[i];
			break;
		}
		type >>= 1;
	}
	return type_name;
}

device_t *event_get_device(event_t *event)
{
	return (device_t *) event->device;
}

void event_option_set(event_t *event, const char *name, eu_variant_t *var)
{
	eu_variant_map_set_variant(event->options, name, var);
}

void event_option_set_int32(event_t *event, const char *name, int val)
{
	eu_variant_map_set_int32(event->options, name, val);
}

void event_option_set_double(event_t *event, const char *name, double val)
{
	eu_variant_map_set_double(event->options, name, val);
}

eu_variant_t *event_option_get(event_t *event, const char *name)
{
	return eu_variant_map_get_variant(event->options, name);
}

int event_option_get_int32(event_t *event, const char *name)
{
	return eu_variant_map_get_int32(event->options, name);
}

double event_option_get_double(event_t *event, const char *name)
{
	return eu_variant_map_get_double(event->options, name);
}

event_type_e event_get_type(event_t *event)
{
	return event->type;
}

bool event_equal(event_t *event1, event_t *event2)
{
	if (event1->src_type != event2->src_type) {
		return false;
	}
	if (event1->device != event2->device) {
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

	while (event_type_name[i] != NULL) {
		if (strcmp(event_type_name[i], name) == 0) {
			return (1 << i);
		}
		i++;
	}
	eu_log_err("Cannot find event type: %s", name);
	return -1;
}

void event_print(event_t *event)
{
	device_t *device = (device_t *) event->device;


	eu_log_info("event: device: %s, type: %s", device_get_name(device), event_get_name(event));

	if (eu_variant_map_count(event->options) > 0) {
		eu_variant_map_pair_t *pair = NULL;
		eu_variant_map_for_each_pair(pair, event->options) {
			const char *key = eu_variant_map_pair_get_key(pair);
			char *val = eu_variant_print_char(eu_variant_map_pair_get_val(pair));
			eu_log_info("   option: %s = %s", key, val);
			free(val);
		}
	}
}
