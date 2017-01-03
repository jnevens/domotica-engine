/*
 * timer.c
 *
 *  Created on: Jan 2, 2017
 *      Author: jnevens
 */
#include <stdio.h>
#include <stdlib.h>

#include <bus/log.h>
#include <bus/event.h>
#include <bus/timer.h>

#include "timer.h"
#include "device.h"
#include "event.h"
#include "action.h"

typedef struct {
	uint64_t timeout_ms;
	event_timer_t *timer;
} tmr_t;

static bool timer_device_parser(device_t *device, char *options[])
{
	log_debug("parse timer %s!", device_get_name(device));
	tmr_t *tmr = calloc(1, sizeof(tmr_t));
	if (tmr) {
		tmr->timeout_ms = 10000;
		tmr->timer = NULL;
		device_set_userdata(device, tmr);
		return true;
	}
	return false;
}

static bool tmr_timeout(void *arg)
{
	device_t *device = arg;
	tmr_t *tmr = device_get_userdata(device);
	tmr->timer = NULL;
	device_trigger_event(device, EVENT_TIMEOUT);

	return false;
}

static bool timer_device_exec(device_t *device, action_t *action)
{
	tmr_t *tmr = device_get_userdata(device);
	switch (action_get_type(action)) {
	case ACTION_SET:
		if (tmr->timer) {
			event_timer_destroy(tmr->timer);
			tmr->timer = NULL;
		}
		tmr->timer = event_timer_create(tmr->timeout_ms, tmr_timeout, device);
		break;
	case ACTION_UNSET:
		if (tmr->timer) {
			event_timer_destroy(tmr->timer);
			tmr->timer = NULL;
		}
		break;
	}

	return false;
}

bool timer_technology_init(void)
{
	event_type_e events = EVENT_TIMEOUT;
	action_type_e actions = ACTION_SET | ACTION_UNSET;
	device_register_type("TIMER", events, actions, timer_device_parser, timer_device_exec);

	log_info("Succesfully initialized: Timers!");
	return true;
}
