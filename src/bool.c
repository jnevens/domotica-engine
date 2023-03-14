/*
 * bool.c
 *
 *  Created on: Jun 25, 2017
 *      Author: jnevens
 */
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include <eu/log.h>

#include "device.h"
#include "event.h"
#include "action.h"

#include "bool.h"

typedef struct {
	int32_t	val;
} bool_t;

static bool bool_parser(device_t *device, char *options[])
{
	int val_default = -1;
	if (options[0]) {
		if (strcmp(options[0], "0") == 0) {
			val_default = 0;
		} else if (strcmp(options[0], "1") == 0) {
			val_default = 1;
		}
	}
	eu_log_debug("bool: %s : %d", device_get_name(device), val_default);

	bool_t *b = calloc(1, sizeof(bool_t));
	if (b) {
		b->val = val_default;
		device_set_userdata(device, b);
		return true;
	}

	return false;
}

static bool bool_exec(device_t *device, action_t *action, event_t *event)
{
	bool_t *b = device_get_userdata(device);
	int32_t val_priv = b->val;
	action_print(action);

	switch (action_get_type(action)) {
	case ACTION_SET:
		b->val = true;
		break;
	case ACTION_UNSET:
		b->val = false;
		break;
	case ACTION_TOGGLE:
		b->val = (b->val) ? 0 : 1;
		break;
	default:
		break;
	}

	eu_log_debug("bool %s set to: %d (prev %d)", device_get_name(device), (b->val) ? 1 : 0, (val_priv) ? 1 : 0);

	if (val_priv != b->val) {
		device_trigger_event(device, (b->val == 1) ? EVENT_SET : EVENT_UNSET );
	}

	return false;
}

static bool bool_check(device_t *device, condition_t *condition)
{
	bool_t *b = device_get_userdata(device);
	switch (condition_get_type(condition)) {
	case CONDITION_SET:
		return (b->val) ? true : false;
		break;
	case CONDITION_UNSET:
		return (!b->val) ? true : false;
		break;
	default:
		break;
	}
	return false;
}

static bool bool_state(device_t *device, eu_variant_map_t *state)
{
	bool_t *b = device_get_userdata(device);

	eu_variant_map_set_int32(state, "value", b->val);

	return true;
}

static bool bool_store(device_t *device, eu_variant_map_t *state)
{
	bool_t *b = device_get_userdata(device);
	eu_variant_map_set_int32(state, "value", b->val);
	return true;
}

static bool bool_restore(device_t *device, eu_variant_map_t *state)
{
	bool_t *b = device_get_userdata(device);
	b->val = eu_variant_map_get_int32(state, "value");
	return true;
}

static void bool_cleanup(device_t *device)
{
	bool_t *b = device_get_userdata(device);
	free(b);
}

static device_type_info_t bool_info = {
	.name = "BOOL",
	.events = EVENT_SET | EVENT_UNSET,
	.actions = ACTION_SET | ACTION_UNSET | ACTION_TOGGLE,
	.conditions = CONDITION_SET | CONDITION_UNSET,
	.check_cb = bool_check,
	.parse_cb = bool_parser,
	.exec_cb = bool_exec,
	.state_cb = bool_state,
	.store_cb = bool_store,
	.restore_cb = bool_restore,
	.cleanup_cb = bool_cleanup,
};

bool bool_technology_init(void)
{
	device_type_register(&bool_info);

	eu_log_info("Succesfully initialized: Booleans!");
	return true;
}
