/*
 * eltako_fud14.c
 *
 *  Created on: Dec 24, 2016
 *      Author: jnevens
 */
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>

#include <bus/log.h>
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
	device_fud14_t *fud14 = calloc(1, sizeof(device_fud14_t));
	if(options[1] != NULL) {
		fud14->address = strtoll(options[1], NULL, 16);
		fud14->dim_value = 0;
		fud14->dim_direction = 1;
		fud14->dim_min_value = 0;
		fud14->dim_max_value = 100;
		fud14->dim_interval = 3;
		device_set_userdata(device, fud14);
		log_debug("FUD14 device created (name: %s, address: 0x%x)", device_get_name(device), fud14->address);
		return true;
	}
	return false;
}

static bool fud14_device_exec(device_t *device, action_t *action)
{
	device_fud14_t *fud14 = device_get_userdata(device);
	eltako_message_t *msg = NULL;
	log_info("set fud14 device: %s, type: %s", device_get_name(device), action_type_to_char(action_get_type(action)));

	switch (action_get_type(action)) {
	case ACTION_SET:
		msg = eltako_dimmer_create_message(fud14->address, DIMMER_EVENT_ON, 100, 1, false);
		fud14->dim_value = 100;
		fud14->dim_direction = 0;
		break;
	case ACTION_UNSET:
		msg = eltako_dimmer_create_message(fud14->address, DIMMER_EVENT_OFF, 0, 1, false);
		fud14->dim_value = 0;
		fud14->dim_direction = 1;
		break;
	case ACTION_TOGGLE:
		fud14->dim_value = (fud14->dim_value > 0) ? 0 : 100;
		msg = eltako_dimmer_create_message(fud14->address, (fud14->dim_value > 0) ? DIMMER_EVENT_ON : DIMMER_EVENT_OFF,
				fud14->dim_value, 1, false);
		fud14->dim_direction = !!!fud14->dim_value;
		break;
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
	log_info("set fud14 device: %s, type: %s, new value: %d", device_get_name(device),
			action_type_to_char(action_get_type(action)), fud14->dim_value);
	return eltako_send(msg);
}


bool eltako_fud14_init(void)
{
	action_type_e actions = ACTION_SET | ACTION_UNSET | ACTION_TOGGLE | ACTION_DIM;
	event_type_e events = 0;
	device_register_type("FUD14", events, actions, fud14_device_parser, fud14_device_exec);

	return true;
}
