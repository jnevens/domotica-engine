/*
 * eltako_fud14.c
 *
 *  Created on: Dec 24, 2016
 *      Author: jnevens
 */
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>

#include <eu/log.h>
#include <eu/variant_map.h>
#include <libeltako/message.h>
#include <libeltako/dimmer.h>

#include "eltako.h"
#include "action.h"
#include "event.h"
#include "device.h"
#include "eltako_fud14.h"

typedef struct {
	uint32_t address;
	uint8_t dim_value;
	uint8_t dim_direction;
	uint8_t dim_min_value;
	uint8_t dim_max_value;
	uint8_t dim_interval;
} device_fud14_t;

static bool fud14_device_parser(device_t *device, char *options[])
{
	if(options[1] != NULL) {
		device_fud14_t *fud14 = calloc(1, sizeof(device_fud14_t));
		fud14->address = strtoll(options[1], NULL, 16);
		fud14->dim_value = 0;
		fud14->dim_direction = 1;
		fud14->dim_min_value = 0;
		fud14->dim_max_value = 100;
		fud14->dim_interval = 3;
		device_set_userdata(device, fud14);
		eu_log_debug("FUD14 device created (name: %s, address: 0x%x)", device_get_name(device), fud14->address);
		return true;
	}
	return false;
}

static bool fud14_device_exec(device_t *device, action_t *action, event_t *event)
{
	device_fud14_t *fud14 = device_get_userdata(device);
	eltako_message_t *msg = NULL;
	eu_log_info("set fud14 device: %s, type: %s", device_get_name(device), action_type_to_char(action_get_type(action)));

	switch (action_get_type(action)) {
	case ACTION_SET:
		fud14->dim_value = (action_get_option(action, 1) != NULL) ? atoi(action_get_option(action, 1)) :  100;
		fud14->dim_direction = 0;
		msg = eltako_dimmer_create_message(fud14->address, DIMMER_EVENT_ON, fud14->dim_value, 1, false);
		break;
	case ACTION_UNSET:
		msg = eltako_dimmer_create_message(fud14->address, DIMMER_EVENT_OFF, 0, 1, false);
		fud14->dim_value = 0;
		fud14->dim_direction = 1;
		break;
	case ACTION_TOGGLE:
	{
		int max_dim_value = (action_get_option(action, 1) != NULL) ? atoi(action_get_option(action, 1)) :  100;
		fud14->dim_value = (fud14->dim_value > 0) ? 0 : max_dim_value;
		msg = eltako_dimmer_create_message(fud14->address, (fud14->dim_value > 0) ? DIMMER_EVENT_ON : DIMMER_EVENT_OFF,
				fud14->dim_value, 1, false);
		fud14->dim_direction = !!!fud14->dim_value;
		break;
	}
	case ACTION_DIM:
		if (fud14->dim_direction == 1) {
			fud14->dim_value += fud14->dim_interval;
		} else {
			fud14->dim_value -= fud14->dim_interval;
		}
		if (fud14->dim_value >= fud14->dim_max_value) {
			fud14->dim_value = fud14->dim_max_value;
			fud14->dim_direction = 0;
		}
		if (fud14->dim_value <= fud14->dim_min_value) {
			fud14->dim_value = fud14->dim_min_value;
			fud14->dim_direction = 1;
		}
		msg = eltako_dimmer_create_message(fud14->address, DIMMER_EVENT_ON, 100, 1, false);
		break;
	default:
		return false;
	}
	eu_log_info("set fud14 device: %s, type: %s, new value: %d", device_get_name(device),
			action_type_to_char(action_get_type(action)), fud14->dim_value);
	return eltako_send(msg);
}

static bool fud14_device_check(device_t *device, condition_t *condition)
{
	device_fud14_t *fud14 = device_get_userdata(device);
	switch (condition_get_type(condition)) {

	eu_log_debug("condition: %s, current value: %d",
			condition_type_to_char(condition_get_type(condition)),
			fud14->dim_value);

	case CONDITION_SET:
		return (fud14->dim_value > 0) ? true : false;
		break;
	case CONDITION_UNSET:
		return (fud14->dim_value == 0) ? true : false;
		break;
	}
	return false;
}

static bool fud14_device_state(device_t *device, eu_variant_map_t *state)
{
	device_fud14_t *fud14 = device_get_userdata(device);

	eu_variant_map_set_int32(state, "value", fud14->dim_value);

	return true;
}

static void fud14_device_cleanup(device_t *device)
{
	device_fud14_t *fud14 = device_get_userdata(device);
	free(fud14);
}


static device_type_info_t fud14_info = {
	.name = "FUD14",
	.events = 0,
	.actions = ACTION_SET | ACTION_UNSET | ACTION_TOGGLE | ACTION_DIM,
	.conditions = CONDITION_SET | CONDITION_UNSET,
	.check_cb = fud14_device_check,
	.parse_cb = fud14_device_parser,
	.exec_cb = fud14_device_exec,
	.state_cb = fud14_device_state,
	.cleanup_cb = fud14_device_cleanup,
};

bool eltako_fud14_init(void)
{
	device_type_register(&fud14_info);

	return true;
}
