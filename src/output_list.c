/*
 * output_list.c
 *
 *  Created on: Dec 20, 2016
 *      Author: jnevens
 */
#include <stdio.h>
#include <string.h>

#include <bus/list.h>

#include "output_list.h"
#include "output.h"

static list_t *outputs = NULL;

bool output_list_init(void)
{
	outputs = list_create();
}

list_t *output_list_get(void)
{
	return outputs;
}

void output_list_destroy(void)
{
	list_destroy_with_data(outputs, (list_destroy_element_fn_t)output_destroy);
	outputs = NULL;
}

bool output_list_add(output_t *output)
{
	list_append(outputs, output);
}

output_t *output_list_find_by_name(const char *name)
{
	list_node_t *node = NULL;
	list_for_each(node, outputs) {
		output_t *output = list_node_data(node);
		if(strcmp(output_get_name(output), name) == 0) {
			return output;
		}
	}
	return NULL;
}

