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

#include "device.h"
#include "action.h"

struct action_s {
	action_type_e type;
	device_t *device;
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
	if (!(device_get_supported_action(device) & action_type)) {
		eu_log_fatal("device %s does not support action: %s", device_get_name(device), action_type_to_char(action_type));
		return NULL;
	}

	action_t *action = calloc(1, sizeof(action_t));
	action->device = device;
	action->type = action_type;

	return action;
}

void action_destroy(action_t *action)
{
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

	while (action_type_name[i]) {
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
	for(i = 0; i< 32; i++) {
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
