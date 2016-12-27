/*
 * output.h
 *
 *  Created on: Dec 20, 2016
 *      Author: jnevens
 */

#ifndef OUTPUT_H_
#define OUTPUT_H_

#include <stdbool.h>
#include "action.h"
#include "types.h"

typedef bool (*output_parse_fn_t)(output_t *output, char *options[]);
typedef bool (*output_exec_fn_t)(output_t *output, action_t *action);

output_t *output_create(char *name, char *options[]);
void output_destroy(output_t *output);

void output_set_userdata(output_t *output, void *userdata);
void *output_get_userdata(output_t *output);

bool output_set(output_t *output, action_t *action);

bool output_register_type(const char *name, action_type_e actions, output_parse_fn_t parse_cb, output_exec_fn_t exec_cb);

const char *output_get_name(output_t *output);

#endif /* OUTPUT_H_ */
