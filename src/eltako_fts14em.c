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

#include <bus/log.h>
#include <bus/event.h>
#include <bus/timer.h>

#include "input.h"
#include "utils_time.h"
#include "eltako_fts14em.h"

typedef struct {
	uint32_t address;
	uint64_t press_time;
	event_timer_t *timer;
} fts14em_input_t;

static bool fts14em_input_parser(input_t *input, char *options[])
{
	fts14em_input_t *eltako_input = calloc(1, sizeof(fts14em_input_t));
	if(options[1] != NULL) {
		eltako_input->address = strtoll(options[1], NULL, 16);
		input_set_userdata(input, eltako_input);
		log_info("FTS14EM input created (name: %s, address: 0x%x)", input_get_name(input), eltako_input->address);
		return true;
	}
	return false;
}

bool eltako_fts14em_dim_press_timer_cb(void *arg)
{
	input_t *input = arg;
	fts14em_input_t *fts14em_input = input_get_userdata(input);

	uint64_t press_time = fts14em_input->press_time;
	uint64_t release_time = get_current_time_ms();
	uint64_t press_duration = release_time - press_time;

	input_trigger_event(input, EVENT_DIM);

	return event_timer_continue;
}

bool eltako_fts14em_short_press_timer_cb(void *arg)
{
	input_t *input = arg;
	fts14em_input_t *fts14em_input = input_get_userdata(input);

	fts14em_input->timer = event_timer_create(PRESS_DIM_INTERVAL_TIME_MS, eltako_fts14em_dim_press_timer_cb, input);

	input_trigger_event(input, EVENT_DIM);

	return event_timer_stop;
}

void eltako_fts14em_incoming_data(eltako_message_t *msg, input_t *input)
{
	int status = eltako_message_get_status(msg);
	fts14em_input_t *fts14em_input = input_get_userdata(input);

	if(status == 0x30) { // press
		input_trigger_event(input, EVENT_PRESS);
		fts14em_input->press_time = get_current_time_ms();

		if(fts14em_input->timer) {
			event_timer_destroy(fts14em_input->timer);
		}
		fts14em_input->timer = event_timer_create(SHORT_PRESS_MAX_TIME_MS, eltako_fts14em_short_press_timer_cb, input);
	} else if (status == 0x20) { // release
		uint64_t press_time = fts14em_input->press_time;
		uint64_t release_time = get_current_time_ms();
		uint64_t press_duration = release_time - press_time;
		log_debug("press duration: %llums", press_duration);

		input_trigger_event(input, EVENT_RELEASE);

		event_timer_destroy(fts14em_input->timer);
		fts14em_input->timer = NULL;

		if(press_duration <= SHORT_PRESS_MAX_TIME_MS) {
			input_trigger_event(input, EVENT_SHORT_PRESS);
		} else {
			input_trigger_event(input, EVENT_LONG_PRESS);
		}
	}
}

static bool fts14em_input_exec(input_t *input)
{
	return false;
}

uint32_t eltako_fts14em_get_address(input_t *input)
{
	fts14em_input_t *eltako_input = input_get_userdata(input);
	return eltako_input->address;
}

bool eltako_fts14em_init(void)
{
	event_type_e events = EVENT_PRESS | EVENT_RELEASE | EVENT_SHORT_PRESS | EVENT_LONG_PRESS | EVENT_DIM;
	input_register_type("FTS14EM", events, fts14em_input_parser, fts14em_input_exec);

	return false;
}
