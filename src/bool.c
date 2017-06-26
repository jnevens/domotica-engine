/*
 * bool.c
 *
 *  Created on: Jun 25, 2017
 *      Author: jnevens
 */
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>

#include <eu/log.h>

#include "device.h"
#include "event.h"
#include "action.h"

#include "bool.h"

typedef struct {
	bool	val;
} bool_t;

static bool bool_parser(device_t *device, char *options[])
{
	int val_default = (options[0] && options[0][0] == '0') ? 0 : 1;
	eu_log_debug("bool: %s : %d", device_get_name(device), val_default);

	bool_t *b = calloc(1, sizeof(bool_t));
	if (b) {
		b->val = val_default;
		device_set_userdata(device, b);
		return true;
	}

	return false;
}

static bool bool_exec(device_t *device, action_t *action)
{
	bool_t *b = device_get_userdata(device);
	bool val_priv = b->val;
	action_print(action);

	switch (action_get_type(action)) {
	case ACTION_SET:
		b->val = true;
		break;
	case ACTION_UNSET:
		b->val = false;
		break;
	case ACTION_TOGGLE:
		b->val = !b->val;
		break;
	}

	eu_log_debug("bool %s set to: %d (prev %d)", device_get_name(device), (b->val) ? 1 : 0, (val_priv) ? 1 : 0);

	if (val_priv != b->val) {
		device_trigger_event(device, (b->val) ? EVENT_SET : EVENT_UNSET );
	}

	return false;
}

bool bool_technology_init(void)
{
	event_type_e events = EVENT_SET | EVENT_UNSET;
	action_type_e actions = ACTION_SET | ACTION_UNSET | ACTION_TOGGLE;
	device_register_type("BOOL", events, actions, bool_parser, bool_exec);

	eu_log_info("Succesfully initialized: Booleans!");
	return true;
}
