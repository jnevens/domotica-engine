/*
 * timer.c
 *
 *  Created on: Jan 2, 2017
 *      Author: jnevens
 */
#include <stdio.h>
#include <stdlib.h>

#include <eu/log.h>
#include <eu/event.h>
#include <eu/event.h>

#include "timer.h"
#include "device.h"
#include "event.h"
#include "action.h"

typedef struct {
	uint64_t timeout_ms;
	eu_event_timer_t *timer;
} tmr_t;

static bool timer_device_parser(device_t *device, char *options[])
{
	int timeout = 0;
	eu_log_debug("parse timer %s!", device_get_name(device));

	if (options[0] == NULL)
		return false;

	// check timeout
	if (sscanf(options[0], "%ds", &timeout) == 1) {
		timeout *= 1;
	} else if (sscanf(options[0], "%dm", &timeout) == 1) {
		timeout *= 60;
	} else if (sscanf(options[0], "%dh", &timeout) == 1) {
		timeout *= 60 * 60;
	} else if (sscanf(options[0], "%d", &timeout) == 1) {
		timeout *= 1;
	} else {
		return false;
	}

	// create timer
	tmr_t *tmr = calloc(1, sizeof(tmr_t));
	if (tmr) {
		eu_log_info("create timer %s (timeout:%ds)",
				device_get_name(device),
				timeout);
		tmr->timer = NULL;
		tmr->timeout_ms = timeout * 1000;
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

static bool timer_device_exec(device_t *device, action_t *action, event_t *event)
{
	tmr_t *tmr = device_get_userdata(device);
	switch (action_get_type(action)) {
	case ACTION_SET:
		if (tmr->timer) {
			eu_event_timer_destroy(tmr->timer);
			tmr->timer = NULL;
		}
		tmr->timer = eu_event_timer_create(tmr->timeout_ms, tmr_timeout, device);
		break;
	case ACTION_UNSET:
		if (tmr->timer) {
			eu_event_timer_destroy(tmr->timer);
			tmr->timer = NULL;
		}
		break;
	default:
		break;
	}

	return false;
}

static void timer_device_cleanup(device_t *device)
{
	tmr_t *tmr = device_get_userdata(device);
	if (tmr->timer)
		eu_event_timer_destroy(tmr->timer);
	free(tmr);
}

static device_type_info_t timer_info = {
	.name = "TIMER",
	.events = EVENT_TIMEOUT,
	.actions = ACTION_SET | ACTION_UNSET,
	.conditions = 0,
	.check_cb = NULL,
	.parse_cb = timer_device_parser,
	.exec_cb = timer_device_exec,
	.cleanup_cb = timer_device_cleanup,
};

bool timer_technology_init(void)
{
	device_type_register(&timer_info);

	eu_log_info("Succesfully initialized: Timers!");
	return true;
}
