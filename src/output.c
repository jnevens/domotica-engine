/*
 * output.c
 *
 *  Created on: Dec 20, 2016
 *      Author: jnevens
 */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <bus/log.h>
#include <bus/list.h>

#include "output.h"

typedef struct {
	char *name;
	output_parse_fn_t parse_cb;
	output_exec_fn_t exec_cb;
	action_type_e actions;
} output_type_t;

struct output_s {
	char *name;
	void *userdata;
	output_type_t *output_type;
};

static list_t *output_types;

output_t *output_create(char *name, char *options[]) {
	log_info("Output create: %s", name);
	output_t *output = calloc(1, sizeof(output_t));
	output->name = strdup(name);

	// lookup output type
	if(options[0] != NULL) {
		list_node_t *node;
		list_for_each(node, output_types) {
			output_type_t *output_type = list_node_data(node);
			if (strcmp(output_type->name, options[0]) == 0) {
				log_info("Output create: %s", name);
				if(output_type->parse_cb(output, options)) {
					output->output_type = output_type;
					goto finish;
				}
				log_err("Failed to parse output: %s", name);
				break;
			}
		}
	}

	output_destroy(output);
	return NULL;
finish:
	return output;
}

void output_destroy(output_t *output)
{
	free(output->name);
	free(output);
}

void output_set_userdata(output_t *output, void *userdata)
{
	output->userdata = userdata;
}

void *output_get_userdata(output_t *output)
{
	return output->userdata;
}

const char *output_get_name(output_t *output)
{
	return output->name;
}

bool output_register_type(const char *name, action_type_e actions, output_parse_fn_t parse_cb, output_exec_fn_t exec_cb)
{
	if (!output_types) {
		output_types = list_create();
	}

	output_type_t *type = calloc(1, sizeof(output_type_t));
	type->name = strdup(name);
	type->actions = actions;
	type->parse_cb = parse_cb;
	type->exec_cb = exec_cb;

	log_info("Register output type: %s", name);

	list_append(output_types, type);
	return true;
}

bool output_set(output_t *output, action_t *action)
{
	list_node_t *node;

	list_for_each(node, output_types) {
		output_type_t *output_type = list_node_data(node);
		if (output_type == output->output_type) {
			if (action_get_type(action) & output_type->actions) {
				output_type->exec_cb(output, action);
			} else {
				log_err("Action type: %s not supported for output: %s",
						action_type_to_char(action_get_type(action)), output_get_name(output));
			}
		}
	}
}
