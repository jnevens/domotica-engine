/*
 * eltako_fts14em.c
 *
 *  Created on: Dec 22, 2016
 *      Author: jnevens
 */
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>

#include <eu/log.h>
#include <eu/event.h>
#include <eu/timer.h>

#include "event.h"
#include "action.h"
#include "device.h"
#include "utils_time.h"
#include "eltako_fts14em.h"

typedef struct {
	uint32_t address;
	uint64_t press_time;
} fts14em_device_t;

static bool fts14em_device_parser(device_t *device, char *options[])
{
	if(options[1] != NULL) {
		fts14em_device_t *eltako_device = calloc(1, sizeof(fts14em_device_t));
		eltako_device->address = strtoll(options[1], NULL, 16);
		device_set_userdata(device, eltako_device);
		eu_log_info("FTS14EM device created (name: %s, address: 0x%x)", device_get_name(device), eltako_device->address);
		return true;
	}
	return false;
}

void eltako_fts14em_incoming_data(eltako_message_t *msg, device_t *device)
{
	int status = eltako_message_get_status(msg);
	fts14em_device_t *fts14em_device = device_get_userdata(device);

	if(status == 0x30) { // press
		device_trigger_event(device, EVENT_PRESS);
		fts14em_device->press_time = get_current_time_ms();
	} else if (status == 0x20) { // release
		uint64_t press_time = fts14em_device->press_time;
		if (press_time > 0) {
			uint64_t release_time = get_current_time_ms();
			uint64_t press_duration = release_time - press_time;
			eu_log_debug("press duration: %llums", press_duration);

			device_trigger_event(device, EVENT_RELEASE);
			fts14em_device->press_time = 0;

			if (press_duration <= SHORT_PRESS_MAX_TIME_MS) {
				device_trigger_event(device, EVENT_SHORT_PRESS);
			} else {
				device_trigger_event(device, EVENT_LONG_PRESS);
			}
		} else {
			eu_log_warn("Ignoring release event without press time! %s", device_get_name(device));
		}
	}
}

static bool fts14em_device_exec(device_t *device, action_t *action)
{
	return false;
}

uint32_t eltako_fts14em_get_address(device_t *device)
{
	fts14em_device_t *eltako_device = device_get_userdata(device);
	return eltako_device->address;
}

static device_type_info_t fts14em_info = {
	.name = "FTS14EM",
	.events = EVENT_PRESS | EVENT_RELEASE | EVENT_SHORT_PRESS | EVENT_LONG_PRESS,
	.actions = 0,
	.conditions = 0,
	.check_cb = NULL,
	.parse_cb = fts14em_device_parser,
	.exec_cb = fts14em_device_exec,
};

bool eltako_fts14em_init(void)
{
	device_type_register(&fts14em_info);

	return false;
}
