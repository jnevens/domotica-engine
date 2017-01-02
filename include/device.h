/*
 * device.h
 *
 *  Created on: Jan 2, 2017
 *      Author: jnevens
 */

#ifndef DEVICE_H_
#define DEVICE_H_

#include <stdbool.h>

#include "types.h"

#define SHORT_PRESS_MAX_TIME_MS		600
#define PRESS_DIM_INTERVAL_TIME_MS	200

typedef bool (*device_parse_fn_t)(device_t *device, char *options[]);
typedef bool (*device_exec_fn_t)(device_t *device, action_t *action);

device_t *device_create(char *name, char *options[]);
void device_destroy(device_t *device);

void device_set_userdata(device_t *device, void *userdata);
void *device_get_userdata(device_t *device);

bool device_set(device_t *device, action_t *action);
void device_trigger_event(device_t *device, event_type_e event);

bool device_register_type(const char *name, event_type_e events, action_type_e actions,
		device_parse_fn_t parse_cb, device_exec_fn_t exec_cb);

const char *device_get_name(device_t *device);
const char *device_get_type(device_t *device);
action_type_e device_get_supported_action(device_t *device);
action_type_e device_get_supported_events(device_t *device);





#endif /* DEVICE_H_ */
