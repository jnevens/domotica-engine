/*
 * input.h
 *
 *  Created on: Dec 20, 2016
 *      Author: jnevens
 */

#ifndef INPUT_H_
#define INPUT_H_

#include <stdbool.h>

#include "types.h"
#include "event.h"

#define SHORT_PRESS_MAX_TIME_MS		600
#define PRESS_DIM_INTERVAL_TIME_MS	200

typedef bool (*input_parse_fn_t)(input_t *input, char *options[]);
typedef bool (*input_exec_fn_t)(input_t *input);

input_t *input_create(char *name, char *options[]);
void input_destroy(input_t *input);

void input_set_userdata(input_t *input, void *userdata);
void *input_get_userdata(input_t *input);

bool input_register_type(const char *name, event_type_e events, input_parse_fn_t parse_cb, input_exec_fn_t exec_cb);

void input_trigger_event(input_t *input, event_type_e event);

const char *input_get_name(input_t *input);
const char *input_get_type(input_t *input);

#endif /* INPUT_H_ */
