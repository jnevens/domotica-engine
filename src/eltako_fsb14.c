/*
 * eltako_fsb14.c
 *
 *  Created on: Dec 24, 2016
 *      Author: jnevens
 */
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>

#include <eu/log.h>
#include <eu/event.h>
#include <libeltako/message.h>
#include <libeltako/dimmer.h>

#include "eltako.h"
#include "action.h"
#include "event.h"
#include "condition.h"
#include "device.h"
#include "eltako_fsb14.h"

typedef struct {
	uint32_t address;
	uint32_t duration;
	eu_event_timer_t *timer;
	condition_type_e condition;
	action_type_e last_action;
	bool locked;
} device_fsb14_t;

static bool fsb14_device_parser(device_t *device, char *options[])
{
	if (options[1] != NULL) {
		device_fsb14_t *fsb14 = calloc(1, sizeof(device_fsb14_t));
		fsb14->address = strtoll(options[1], NULL, 16);
		fsb14->duration = strtoll(options[2], NULL, 10);
		fsb14->condition = CONDITION_STOPPED;
		device_set_userdata(device, fsb14);
		eu_log_debug("FSB14 device created (name: %s, address: 0x%x, duration: %d)", device_get_name(device),
				fsb14->address, fsb14->duration);
		return true;
	}
	return false;
}

static bool fsb14_timer_callback(void *arg)
{
	device_t *device = (device_t *)arg;
	device_fsb14_t *fsb14 = device_get_userdata(device);

	fsb14->condition = CONDITION_STOPPED;
	if (fsb14->last_action == ACTION_UP) {
		fsb14->condition = CONDITION_UP;
		device_trigger_event(device, EVENT_UP);
	} else if (fsb14->last_action == ACTION_DOWN) {
		fsb14->condition = CONDITION_DOWN;
		device_trigger_event(device, EVENT_DOWN);
	}
	fsb14->last_action = ACTION_STOP;
	fsb14->timer = NULL;

	return false;
}

static bool fsb14_device_exec(device_t *device, action_t *action, event_t *event)
{
	bool rv = true;
	device_fsb14_t *fsb14 = device_get_userdata(device);
	eltako_message_t *msg = NULL;
	uint8_t data[4] = { 0x08, 0x01, fsb14->duration, 0x00 };

	switch (action_get_type(action)) {
	case ACTION_TOGGLE_UP: {
		if (fsb14->condition == CONDITION_STOPPED || fsb14->condition == CONDITION_DOWN) {
			data[1] = 0x01; // open
		} else {
			data[1] = 0x00; // stop
		}
		break;
	}
	case ACTION_TOGGLE_DOWN: {
		if (fsb14->condition == CONDITION_STOPPED || fsb14->condition == CONDITION_UP) {
			data[1] = 0x02; // close
		} else {
			data[1] = 0x00; // stop
		}
		break;
	}
	case ACTION_UP: {
		data[1] = 0x01; // open
		break;
	}
	case ACTION_DOWN: {
		data[1] = 0x02; // close
		break;
	}
	case ACTION_STOP: {
		data[1] = 0x00; // stop
		break;
	}
	case ACTION_LOCK: {
		fsb14->locked = 1;
		return true;
		break;
	}
	case ACTION_UNLOCK: {
		fsb14->locked = 0;
		return true;
		break;
	}
	default:
		return false;
	}

	if (fsb14->timer) {
		eu_event_timer_destroy(fsb14->timer);
		fsb14->timer = NULL;
	}

	if (!fsb14->locked) {
		if (data[1] == 0x01) { // rise
			fsb14->condition = CONDITION_RISING;
			fsb14->last_action = ACTION_UP;
			fsb14->timer = eu_event_timer_create(fsb14->duration * 1000, fsb14_timer_callback, device);
		} else if (data[1] == 0x02) { // descend
			fsb14->condition = CONDITION_DESCENDING;
			fsb14->last_action = ACTION_DOWN;
			fsb14->timer = eu_event_timer_create(fsb14->duration * 1000, fsb14_timer_callback, device);
		} else { // stop
			fsb14->condition = CONDITION_STOPPED;
			fsb14->last_action = ACTION_STOP;
		}

		if (action_get_option(action, 1) != NULL) {
			data[2] = atoi(action_get_option(action, 1));
		}

		msg = eltako_message_create(0, 0x07, data, fsb14->address, 0);
		eu_log_info("set fsb14 device: %s, type: %s", device_get_name(device),
				action_type_to_char(action_get_type(action)));
		rv = eltako_send(msg);
	} else {
		eu_log_info("fsb14 device: %s, LOCKED! ignoring action: %s", device_get_name(device),
				action_type_to_char(action_get_type(action)));
	}

	return rv;
}

static bool fsb14_device_check(device_t *device, condition_t *condition)
{
	device_fsb14_t *fsb14 = device_get_userdata(device);

	eu_log_debug("condition: %s, current value: %d",
			condition_type_to_char(condition_get_type(condition)),
			condition_type_to_char(fsb14->condition));

	if (condition_get_type(condition) == CONDITION_LOCKED) {
		return fsb14->locked;
	} else if (condition_get_type(condition) == CONDITION_UNLOCKED) {
		return !fsb14->locked;
	} else if (condition_get_type(condition) == fsb14->condition) {
		return true;
	} else {
		return false;
	}
}

static bool fsb14_device_state(device_t *device, eu_variant_map_t *varmap)
{
	device_fsb14_t *fsb14 = device_get_userdata(device);

	eu_variant_map_set_int32(varmap, "duration", fsb14->duration);
	if (fsb14->condition == CONDITION_DOWN ) {
		eu_variant_map_set_char(varmap, "value", "closed");
	} else if (fsb14->condition == CONDITION_UP ) {
		eu_variant_map_set_char(varmap , "value", "open");
	} else if (fsb14->condition == CONDITION_RISING ) {
		eu_variant_map_set_char(varmap, "value", "opening");
	} else if (fsb14->condition == CONDITION_DESCENDING ) {
		eu_variant_map_set_char(varmap, "value", "closing");
	} else if (fsb14->condition == CONDITION_STOPPED ) {
		eu_variant_map_set_char(varmap, "value", "stopped");
	} else {
		eu_variant_map_set_char(varmap, "value", "unknown");
	}
	eu_variant_map_set_int32(varmap, "locked", fsb14->locked);

	return true;
}

static void fsb14_device_cleanup(device_t *device)
{
	device_fsb14_t *fsb14 = device_get_userdata(device);
	free(fsb14);
}

static device_type_info_t fsb14_info = {
	.name = "FSB14",
	.events = EVENT_UP, EVENT_DOWN,
	.actions = ACTION_UP | ACTION_DOWN | ACTION_STOP | ACTION_TOGGLE_UP | ACTION_TOGGLE_DOWN | ACTION_LOCK | ACTION_UNLOCK,
	.conditions = CONDITION_UP | CONDITION_DOWN | CONDITION_RISING | CONDITION_DESCENDING,
	.check_cb = fsb14_device_check,
	.parse_cb = fsb14_device_parser,
	.exec_cb = fsb14_device_exec,
	.state_cb = fsb14_device_state,
	.cleanup_cb = fsb14_device_cleanup,
};

bool eltako_fsb14_init(void)
{
	device_type_register(&fsb14_info);

	return true;
}
