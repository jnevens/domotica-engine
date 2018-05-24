/*
 * device.h
 *
 *  Created on: Jan 2, 2017
 *      Author: jnevens
 */

#ifndef DEVICE_H_
#define DEVICE_H_

#include <stdbool.h>

#include <eu/variant_map.h>

#include "types.h"

#include "event.h"
#include "action.h"
#include "condition.h"

#define SHORT_PRESS_MAX_TIME_MS		600
#define PRESS_DIM_INTERVAL_TIME_MS	200

typedef bool (*device_parse_fn_t)(device_t *device, char *options[]);
typedef bool (*device_exec_fn_t)(device_t *device, action_t *action, event_t *event);
typedef bool (*device_check_fn_t)(device_t *device, condition_t *condition);
typedef bool (*device_state_fn_t)(device_t *device, eu_variant_map_t *state);
typedef void (*device_cleanup_fn_t)(device_t *device);

typedef struct {
	char *name;
	event_type_e events;
	action_type_e actions;
	condition_type_e conditions;
	device_parse_fn_t parse_cb;
	device_exec_fn_t exec_cb;
	device_check_fn_t check_cb;
	device_state_fn_t state_cb;
	device_cleanup_fn_t cleanup_cb;
} device_type_info_t;

device_t *device_create(char *name, const char *devtype, char *options[]);
void device_destroy(device_t *device);

void device_set_userdata(device_t *device, void *userdata);
void *device_get_userdata(device_t *device);

bool device_set(device_t *device, action_t *action, event_t *event);
void device_trigger_event(device_t *device, event_type_e event);
bool device_check(device_t *device, condition_t *condition);
eu_variant_map_t *device_state(device_t *device);

bool device_type_register(device_type_info_t *device_type_info);
bool device_type_exists(const char *dev_type_name);

const char *device_get_name(device_t *device);
const char *device_get_type(device_t *device);
action_type_e device_get_supported_action(device_t *device);
action_type_e device_get_supported_events(device_t *device);





#endif /* DEVICE_H_ */
