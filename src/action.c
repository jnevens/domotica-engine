/*
 * action.c
 *
 *  Created on: Dec 24, 2016
 *      Author: jnevens
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <eu/log.h>
#include <eu/list.h>

#include "device.h"
#include "action.h"

struct action_s {
	action_type_e type;
	device_t *device;
	eu_list_t *options;
	// state
};

#define X(a, b, c) c,
static char *action_type_name[] = {
		ACTION_TYPE_TABLE
		NULL
};
#undef X

action_t *action_create(device_t *device, action_type_e action_type, char *options[])
{
	int i = 1;

	if (!(device_get_supported_action(device) & action_type)) {
		eu_log_fatal("device %s does not support action: %s", device_get_name(device), action_type_to_char(action_type));
		return NULL;
	}

	action_t *action = calloc(1, sizeof(action_t));
	action->device = device;
	action->type = action_type;
	action->options = eu_list_create();
	while(options[i] != NULL) {
		eu_log_debug("action add %02d option: %s", i, options[i]);
		eu_list_append(action->options, strdup(options[i]));
		i++;
	}

	return action;
}

void action_destroy(action_t *action)
{
	eu_list_destroy_with_data(action->options, free);
	free(action);
}

action_type_e action_get_type(action_t *action)
{
	return action->type;
}

void action_print(action_t *action)
{
	device_t *device = action->device;
	eu_log_info("action: device: %s, type: %s", device_get_name(device), action_type_to_char(action->type));
}

action_type_e action_type_from_char(const char *type_str)
{
	int i = 0;

	if (type_str == NULL)
		return -1;

	while (action_type_name[i] != NULL) {
		if (strcmp(action_type_name[i], type_str) == 0) {
			return (1 << i);
		}
		i++;
	}
	eu_log_err("Cannot find action type: %s", type_str);
	return -1;
}

const char *action_type_to_char(action_type_e type)
{
	int i;
	const char *type_name = NULL;
	for(i = 0; action_type_name[i] != NULL; i++) {
		if (type & 0x1) {
			type_name = action_type_name[i];
			break;
		}
		type >>= 1;
	}
	return type_name;
}

bool action_execute(action_t *action, event_t *event)
{
	device_set(action->device, action);
	return true;
}

const char *action_get_option(action_t *action, int option) {
	if (eu_list_count(action->options) > 0) {
		int i = 1;
		eu_list_node_t *node = NULL;
		eu_list_for_each(node, action->options)
		{
			if (option == i) {
				return (const char *) eu_list_node_data(node);
			}
			i++;
		}
	}
	return NULL;
}
