/*
 * input.c
 *
 *  Created on: Dec 20, 2016
 *      Author: jnevens
 */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <bus/log.h>
#include <bus/list.h>

#include "input.h"
#include "event.h"
#include "engine.h"

typedef struct {
	char *name;
	input_parse_fn_t parse_cb;
	input_exec_fn_t exec_cb;
	event_type_e events;
} input_type_t;

struct input_s {
	char *name;
	input_type_t *input_type;
	void *userdata;
};

static list_t *input_types = NULL;


bool input_register_type(const char *name, event_type_e events, input_parse_fn_t parse_cb, input_exec_fn_t exec_cb)
{
	if (!input_types) {
		input_types = list_create();
	}

	input_type_t *type = calloc(1, sizeof(input_type_t));
	type->name = strdup(name);
	type->events = events;
	type->parse_cb = parse_cb;
	type->exec_cb = exec_cb;

	log_info("Register input type: %s", name);

	list_append(input_types, type);
	return true;
}

input_t *input_create(char *name, char *options[]) {
	input_t *input = calloc(1, sizeof(input_t));
	input->name = strdup(name);

	// lookup input type
	if(options[0] != NULL) {
		list_node_t *node;
		list_for_each(node, input_types) {
			input_type_t *input_type = list_node_data(node);
			if (strcmp(input_type->name, options[0]) == 0) {
				log_info("Input create: %s", name);
				if(input_type->parse_cb(input, options)) {
					input->input_type = input_type;
					goto finish;
				}
				log_err("Failed to parse input: %s", name);
				break;
			}
		}
	}

	input_destroy(input);
	return NULL;
finish:
	return input;
}

void input_destroy(input_t *input)
{
	free(input->name);
	free(input);
}

void input_set_userdata(input_t *input, void *userdata)
{
	input->userdata = userdata;
}

void *input_get_userdata(input_t *input)
{
	return input->userdata;
}

void input_trigger_event(input_t *input, event_type_e event_type)
{
	event_t *event = event_create_input(input, event_type);
	log_info("Event: input: %s, event: %s", input_get_name(input), event_get_name(event));
	engine_trigger_event(event);
	event_delete(event);
}

const char *input_get_name(input_t *input)
{
	return input->name;
}

const char *input_get_type(input_t *input)
{
	return input->input_type->name;
}
