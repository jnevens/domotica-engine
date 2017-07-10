/*
 * eltako_fsr14.c
 *
 *  Created on: Dec 24, 2016
 *      Author: jnevens
 */
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>

#include <eu/log.h>
#include <libeltako/message.h>
#include <libeltako/dimmer.h>

#include "eltako.h"
#include "action.h"
#include "event.h"
#include "device.h"
#include "eltako_fsr14.h"

typedef struct {
	uint32_t address;
	int32_t val;
} device_fsr14_t;

static bool fsr14_device_parser(device_t *device, char *options[])
{
	device_fsr14_t *fsr14 = calloc(1, sizeof(device_fsr14_t));
	if(options[1] != NULL) {
		fsr14->address = strtoll(options[1], NULL, 16);
		device_set_userdata(device, fsr14);
		eu_log_debug("FSR14 device created (name: %s, address: 0x%x)", device_get_name(device), fsr14->address);
		return true;
	}
	return false;
}

static bool fsr14_device_exec(device_t *device, action_t *action)
{
	device_fsr14_t *fsr14 = device_get_userdata(device);
	eltako_message_t *msg = NULL;
	eu_log_info("set fsr14 device: %s, type: %s", device_get_name(device), action_type_to_char(action_get_type(action)));

	switch (action_get_type(action)) {
	case ACTION_SET: {
		fsr14->val = 1;
		uint8_t data[4] = { 0x09, 0x00, 0x00, 0x01 };
		msg = eltako_message_create(0, 0x07, data, fsr14->address, 0);
		break;
	}
	case ACTION_UNSET: {
		fsr14->val = 0;
		uint8_t data[4] = { 0x08, 0x00, 0x00, 0x01 };
		msg = eltako_message_create(0, 0x07, data, fsr14->address, 0);
		break;
	}
	case ACTION_TOGGLE: {
		fsr14->val = (fsr14->val) ? 0 : 1;
		uint8_t data[4] = { (uint8_t)(0x08 | fsr14->val), 0x00, 0x00, 0x01 };
		msg = eltako_message_create(0, 0x07, data, fsr14->address, 0);
		break;
	}
	default:
		return false;
	}
	eu_log_info("set fsr14 device: %s, type: %s, new value: %d", device_get_name(device),
			action_type_to_char(action_get_type(action)), fsr14->val);
	return eltako_send(msg);
}


bool eltako_fsr14_init(void)
{
	action_type_e actions = ACTION_SET | ACTION_UNSET | ACTION_TOGGLE;
	event_type_e events = 0;
	device_register_type("FSR14", events, actions, fsr14_device_parser, fsr14_device_exec);

	return true;
}
