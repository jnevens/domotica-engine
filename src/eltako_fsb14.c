/*
 * eltako_fsb14.c
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
#include "eltako_fsb14.h"

typedef struct {
	uint32_t address;
	uint32_t duration;
} device_fsb14_t;

static bool fsb14_device_parser(device_t *device, char *options[])
{
	device_fsb14_t *fsb14 = calloc(1, sizeof(device_fsb14_t));
	if (options[1] != NULL) {
		fsb14->address = strtoll(options[1], NULL, 16);
		fsb14->duration = strtoll(options[2], NULL, 10);
		device_set_userdata(device, fsb14);
		log_debug("FSB14 device created (name: %s, address: 0x%x, duration: %d)", device_get_name(device),
				fsb14->address, fsb14->duration);
		return true;
	}
	return false;
}

static bool fsb14_device_exec(device_t *device, action_t *action)
{
	device_fsb14_t *fsb14 = device_get_userdata(device);
	eltako_message_t *msg = NULL;

	switch (action_get_type(action)) {
	case ACTION_UP: {
		uint8_t data[4] = { 0x08, 0x01, fsb14->duration, 0x00 };
		msg = eltako_message_create(0, 0x07, data, fsb14->address, 0);
		break;
	}
	case ACTION_DOWN: {
		uint8_t data[4] = { 0x08, 0x02, fsb14->duration, 0x00 };
		msg = eltako_message_create(0, 0x07, data, fsb14->address, 0);
		break;
	}
	case ACTION_STOP: {
		uint8_t data[4] = { 0x08, 0x00, fsb14->duration, 0x00 };
		msg = eltako_message_create(0, 0x07, data, fsb14->address, 0);
		//msg = eltako_dimmer_create_message(fsb14->address, DIMMER_EVENT_OFF, 0, 1, false);
		break;
	}
	default:
		return false;
	}
	log_info("set fsb14 device: %s, type: %s", device_get_name(device), action_type_to_char(action_get_type(action)));
	return eltako_send(msg);
}

bool eltako_fsb14_init(void)
{
	action_type_e actions = ACTION_UP | ACTION_DOWN | ACTION_STOP;
	event_type_e events = 0;
	device_register_type("FSB14", events, actions, fsb14_device_parser, fsb14_device_exec);

	return true;
}
