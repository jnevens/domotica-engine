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
#include "output.h"
#include "eltako_fud14.h"

typedef struct {
	uint32_t address;
	uint8_t dim_value;
	uint8_t dim_direction;
	uint8_t dim_min_value;
	uint8_t dim_max_value;
	uint8_t dim_interval;
} output_fud14_t;

static bool fud14_output_parser(output_t *output, char *options[])
{
	output_fud14_t *fud14 = calloc(1, sizeof(output_fud14_t));
	if(options[1] != NULL) {
		fud14->address = strtoll(options[1], NULL, 16);
		fud14->dim_value = 0;
		fud14->dim_direction = 1;
		fud14->dim_min_value = 0;
		fud14->dim_max_value = 100;
		fud14->dim_interval = 3;
		output_set_userdata(output, fud14);
		log_debug("FUD14 output created (name: %s, address: 0x%x)", output_get_name(output), fud14->address);
		return true;
	}
	return false;
}

static bool fud14_output_exec(output_t *output, action_t *action)
{
	output_fud14_t *fud14 = output_get_userdata(output);
	eltako_message_t *msg = NULL;
	log_info("set fud14 output: %s, type: %s", output_get_name(output), action_type_to_char(action_get_type(action)));

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
	log_info("set fud14 output: %s, type: %s, new value: %d", output_get_name(output),
			action_type_to_char(action_get_type(action)), fud14->dim_value);
	return eltako_send(msg);
}


bool eltako_fud14_init(void)
{
	action_type_e supported_actions = ACTION_SET | ACTION_UNSET | ACTION_TOGGLE | ACTION_DIM;
	output_register_type("FUD14", supported_actions, fud14_output_parser, fud14_output_exec);
	log_debug("supported actions: 0x%x", supported_actions);

	return true;
}
