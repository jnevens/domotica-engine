/*
 * action.h
 *
 *  Created on: Dec 24, 2016
 *      Author: jnevens
 */

#ifndef ACTION_H_
#define ACTION_H_

#include <stdbool.h>
#include "types.h"

#define ACTION_TYPE_TABLE \
	X(0,	ACTION_SET,		"SET") \
	X(1,	ACTION_UNSET,	"UNSET") \
	X(2,	ACTION_TOGGLE,	"TOGGLE") \
	X(3,	ACTION_DIM,		"DIM")

#define X(a,b,c) b = (1 << a),
enum action_type {
	ACTION_TYPE_TABLE
};
#undef X

action_t *action_create(device_t *device, action_type_e action_type, char *options[]);
void action_destroy(action_t *action);

action_type_e action_get_type(action_t *action);

bool action_execute(action_t *action, event_t *event);

void action_print(action_t *action);

action_type_e action_type_from_char(const char *type_str);
const char *action_type_to_char(action_type_e type);


#endif /* ACTION_H_ */
