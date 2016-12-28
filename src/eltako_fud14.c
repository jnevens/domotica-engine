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
} output_fud14_t;

static bool fud14_output_parser(output_t *output, char *options[])
{
	output_fud14_t *output_fud14 = calloc(1, sizeof(output_fud14_t));
	if(options[1] != NULL) {
		output_fud14->address = strtoll(options[1], NULL, 16);
		output_set_userdata(output, output_fud14);
		log_debug("FUD14 output created (name: %s, address: 0x%x)", output_get_name(output), output_fud14->address);
		return true;
	}
	return false;
}

static bool fud14_output_exec(output_t *output, action_t *action)
{
	output_fud14_t *output_fud14 = output_get_userdata(output);
	eltako_message_t *msg = NULL;
	log_info("set fud14 output: %s, type: %s", output_get_name(output), action_type_to_char(action_get_type(action)));

	switch (action_get_type(action)) {
	case ACTION_SET :
		msg = eltako_dimmer_create_message(output_fud14->address, DIMMER_EVENT_ON,
				100, 1, false);
		output_fud14->dim_value = 100;
		break;
	case ACTION_UNSET :
		msg = eltako_dimmer_create_message(output_fud14->address, DIMMER_EVENT_OFF,
				0, 1, false);
		output_fud14->dim_value = 0;
		break;
	case ACTION_TOGGLE :
		output_fud14->dim_value = (output_fud14->dim_value > 0) ? 0 : 100;
		msg = eltako_dimmer_create_message(output_fud14->address, (output_fud14->dim_value > 0) ?
				DIMMER_EVENT_ON : DIMMER_EVENT_OFF, output_fud14->dim_value, 1, false);
		break;
	default:
		return false;
	}
	log_debug("new output dim value: %d", output_fud14->dim_value);
	return eltako_send(msg);
}


bool eltako_fud14_init(void)
{
	action_type_e supported_actions = ACTION_SET | ACTION_UNSET | ACTION_TOGGLE | ACTION_DIM;
	output_register_type("FUD14", supported_actions, fud14_output_parser, fud14_output_exec);
	log_debug("supported actions: 0x%x", supported_actions);

	return true;
}
