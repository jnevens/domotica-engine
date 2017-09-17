/*
 * device.c
 *
 *  Created on: Jan 2, 2017
 *      Author: jnevens
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <eu/log.h>
#include <eu/list.h>

#include "action.h"
#include "condition.h"
#include "event.h"
#include "engine.h"
#include "device.h"

typedef struct {
	char *name;
	device_parse_fn_t parse_cb;
	device_exec_fn_t exec_cb;
	device_check_fn_t check_cb;
	action_type_e actions;
	event_type_e events;
	condition_type_e conditions;
} device_type_t;

struct device_s {
	char *name;
	device_type_t *device_type;
	void *userdata;
};

static eu_list_t *device_types;

device_t *device_create(char *name, const char *devtype, char *options[])
{
	device_t *device = calloc(1, sizeof(device_t));
	device->name = strdup(name);

	// lookup device type
	if (device && device->name) {
		if (options[0] != NULL) {
			eu_list_node_t *node;
			eu_list_for_each(node, device_types)
			{
				device_type_t *device_type = eu_list_node_data(node);
				if (strcmp(device_type->name, devtype) == 0) {
					if (device_type->parse_cb(device, options)) {
						device->device_type = device_type;
						goto finish;
					}
					eu_log_err("Failed to parse device: %s", name);
					break;
				}
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
	eu_list_node_t *node;

	eu_list_for_each(node, device_types) {
		device_type_t *device_type = eu_list_node_data(node);
		if (device_type == device->device_type) {
			if (action_get_type(action) & device_type->actions) {
				device_type->exec_cb(device, action);
			} else {
				eu_log_err("Action type: %s not supported for device: %s",
						action_type_to_char(action_get_type(action)), device_get_name(device));
			}
		}
	}
	return true;
}

void device_trigger_event(device_t *device, event_type_e event_type)
{
	event_t *event = event_create(device, event_type);
	eu_log_info("Event: device: %s, event: %s", device_get_name(device), event_get_name(event));
	engine_trigger_event(event);
	event_delete(event);
}

bool device_check(device_t *device, condition_t *condition)
{
	eu_list_node_t *node;

	eu_list_for_each(node, device_types) {
		device_type_t *device_type = eu_list_node_data(node);
		if (device_type == device->device_type) {

			if (condition_get_type(condition) & device_type->conditions) {
				if (device_type->check_cb) {
					return device_type->check_cb(device, condition);
				} else {
					eu_log_err("No Condition check callback for device type: %s", device_type->name);
				}
			} else {
				eu_log_err("Condition type: %s not supported for device type: %s",
						condition_type_to_char(condition_get_type(condition)), device_type->name);
			}
		}
	}

	return false;
}

bool device_type_register(device_type_info_t *device_type_info)
{
	if (!device_types) {
		device_types = eu_list_create();
	}

	device_type_t *type = calloc(1, sizeof(device_type_t));
	type->name = device_type_info->name;
	type->actions = device_type_info->actions;
	type->events= device_type_info->events;
	type->conditions = device_type_info->conditions;
	type->parse_cb = device_type_info->parse_cb;
	type->exec_cb = device_type_info->exec_cb;
	type->check_cb = device_type_info->check_cb;

	eu_log_info("Register device type: %s", device_type_info->name);

	eu_list_append(device_types, type);
	return true;
}

const char *device_get_name(device_t *device)
{
	if (!device)
		return NULL;

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

