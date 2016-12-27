/*
 * input_list.c
 *
 *  Created on: Dec 20, 2016
 *      Author: jnevens
 */
#include <stdio.h>
#include <string.h>

#include <bus/list.h>

#include "input_list.h"
#include "input.h"

static list_t *inputs = NULL;

bool input_list_init(void)
{
	inputs = list_create();
}

list_t *input_list_get(void)
{
	return inputs;
}

void input_list_destroy(void)
{
	list_destroy_with_data(inputs, (list_destroy_element_fn_t)input_destroy);
	inputs = NULL;
}

bool input_list_add(input_t *input)
{
	list_append(inputs, input);
}

input_t *input_list_find_by_name(const char *name)
{
	list_node_t *node = NULL;
	list_for_each(node, inputs) {
		input_t *input = list_node_data(node);
		if(strcmp(input_get_name(input), name) == 0) {
			return input;
		}
	}
	return NULL;
}

input_t *input_list_find(input_list_find_fn_t find_cb, void *arg) {
	list_node_t *node = NULL;
	list_for_each(node, inputs)
	{
		input_t *input = list_node_data(node);
		if (find_cb(input, arg)) {
			return input;
		}
	}
	return NULL;
}
