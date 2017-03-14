/*
 * device.c
 *
 *  Created on: Jan 2, 2017
 *      Author: jnevens
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <bus/log.h>
#include <bus/list.h>

#include "action.h"
#include "event.h"
#include "engine.h"
#include "device.h"

typedef struct {
	char *name;
	device_parse_fn_t parse_cb;
	device_exec_fn_t exec_cb;
	action_type_e actions;
	event_type_e events;
} device_type_t;

struct device_s {
	char *name;
	device_type_t *device_type;
	void *userdata;
};

static list_t *device_types;

device_t *device_create(char *name, const char *devtype, char *options[])
{
	device_t *device = calloc(1, sizeof(device_t));
	device->name = strdup(name);

	// lookup device type
	if(options[0] != NULL) {
		list_node_t *node;
		list_for_each(node, device_types) {
			device_type_t *device_type = list_node_data(node);
			if (strcmp(device_type->name, devtype) == 0) {
				if(device_type->parse_cb(device, options)) {
					device->device_type = device_type;
					goto finish;
				}
				log_err("Failed to parse device: %s", name);
				break;
			}
		}
	}

	device_destroy(device);
	return NULL;
finish:
	return device;
}

void device_destroy(device_t *device)
{
	free(device->name);
	free(device);

}

void device_set_userdata(device_t *device, void *userdata)
{
	device->userdata = userdata;
}

void *device_get_userdata(device_t *device)
{
	return device->userdata;
}

bool device_set(device_t *device, action_t *action)
{
	list_node_t *node;

	list_for_each(node, device_types) {
		device_type_t *device_type = list_node_data(node);
		if (device_type == device->device_type) {
			if (action_get_type(action) & device_type->actions) {
				device_type->exec_cb(device, action);
			} else {
				log_err("Action type: %s not supported for device: %s",
						action_type_to_char(action_get_type(action)), device_get_name(device));
			}
		}
	}
}

void device_trigger_event(device_t *device, event_type_e event_type)
{
	event_t *event = event_create(device, event_type);
	log_info("Event: device: %s, event: %s", device_get_name(device), event_get_name(event));
	engine_trigger_event(event);
	event_delete(event);
}

bool device_register_type(const char *name, event_type_e events, action_type_e actions,
		device_parse_fn_t parse_cb, device_exec_fn_t exec_cb)
{
	if (!device_types) {
		device_types = list_create();
	}

	device_type_t *type = calloc(1, sizeof(device_type_t));
	type->name = strdup(name);
	type->actions = actions;
	type->parse_cb = parse_cb;
	type->exec_cb = exec_cb;

	log_info("Register device type: %s", name);

	list_append(device_types, type);
	return true;
}

const char *device_get_name(device_t *device)
{
	return device->name;
}

const char *device_get_type(device_t *device)
{
	return device->device_type->name;
}

action_type_e device_get_supported_action(device_t *device)
{
	return device->device_type->actions;
}

action_type_e device_get_supported_events(device_t *device)
{
	return device->device_type->events;
}

