/*
 * velux_klf200.c
 *
 *  Created on: Jan 5, 2023
 *      Author: jnevens
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include <math.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>

#include <eu/log.h>
#include <eu/list.h>
#include <eu/event.h>

#include "arguments.h"
#include "device.h"
#include "device_list.h"
#include "utils_cmd.h"
#include "velux_klf200.h"

typedef struct {
	uint32_t address;
	char hostname[256];
	char password[64];
	int32_t actual_value;
	bool changed;
	bool locked;
} device_velux_klf200_t;

static eu_list_t *klf200_devices = NULL;
static pthread_t worker_thread = 0;

static bool velux_klf200_device_parser(device_t *device, char *options[])
{
	if(options[1] != NULL) {
		device_velux_klf200_t *velux_klf200 = calloc(1, sizeof(device_velux_klf200_t));
		// hostname
		snprintf(velux_klf200->hostname, sizeof(velux_klf200->hostname),"%s", options[1]);
		// password
		snprintf(velux_klf200->password, sizeof(velux_klf200->password),"%s", options[2]);
		// device address
		velux_klf200->address = strtoll(options[3], NULL, 16);
		device_set_userdata(device, velux_klf200);
		eu_log_debug("VELUX-KLF200 device created (name: %s, klf:%s, pw:%s, n:%lu)",
				device_get_name(device),
				velux_klf200->hostname,
				velux_klf200->password,
				velux_klf200->address);

		// add to devices list
		eu_list_append(klf200_devices, device);
		return true;
	}
	return false;
}

static bool velux_klf200_device_exec(device_t *device, action_t *action, event_t *event)
{
	device_velux_klf200_t *velux_klf200 = device_get_userdata(device);
	//eltako_message_t *msg = NULL;
	eu_log_info("set velux_klf200 device: %s, type: %s", device_get_name(device), action_type_to_char(action_get_type(action)));

	switch (action_get_type(action)) {
	case ACTION_TOGGLE_DOWN: {
		velux_klf200->actual_value = 1;
		velux_klf200->changed = true;
		break;
	}
	case ACTION_TOGGLE_UP: {
		velux_klf200->actual_value = 0;
		velux_klf200->changed = true;
		break;
	}
	case ACTION_DOWN: {
		velux_klf200->actual_value = 1;
		velux_klf200->changed = true;
		break;
	}
	case ACTION_UP: {
		velux_klf200->actual_value = 0;
		velux_klf200->changed = true;
		break;
	}
	case ACTION_TOGGLE: {
		velux_klf200->actual_value = (velux_klf200->actual_value) ? 0 : 1;
		velux_klf200->changed = true;
		break;
	}
	case ACTION_LOCK: {
		velux_klf200->locked = true;
		break;
	}
	case ACTION_UNLOCK: {
		velux_klf200->locked = false;
		break;
	}
	default:
		return false;
	}
	eu_log_info("set velux_klf200 device: %s, type: %s, new value: %d",
			device_get_name(device),
			action_type_to_char(action_get_type(action)),
			velux_klf200->actual_value);

	bool rv = true;
	return rv;
}

static bool velux_klf200_device_check(device_t *device, condition_t *condition)
{
	device_velux_klf200_t *velux_klf200 = device_get_userdata(device);
	switch (condition_get_type(condition)) {
	case CONDITION_SET:
		return (velux_klf200->actual_value == 1) ? true : false;
		break;
	case CONDITION_UNSET:
		return (velux_klf200->actual_value == 0) ? true : false;
		break;
	default:
		break;
	}
	return false;
}

static bool velux_klf200_device_state(device_t *device, eu_variant_map_t *varmap)
{
	device_velux_klf200_t *velux_klf200 = device_get_userdata(device);

	eu_variant_map_set_char(varmap, "value", "unknown");
	if (velux_klf200->changed == 0) {
		if (velux_klf200->actual_value == 1) {
			eu_variant_map_set_char(varmap, "value", "closed");
		} else {
			eu_variant_map_set_char(varmap, "value", "open");
		}
	} else {
		if (velux_klf200->actual_value == 1) {
			eu_variant_map_set_char(varmap, "value", "closing");
		} else {
			eu_variant_map_set_char(varmap, "value", "opening");
		}
	}

	return true;
}

static void velux_klf200_device_cleanup(device_t *device)
{
	device_velux_klf200_t *velux_klf200 = device_get_userdata(device);
	free(velux_klf200);
}

static bool velux_klf200_device_store(device_t *device, eu_variant_map_t *state)
{
	device_velux_klf200_t *velux_klf200 = device_get_userdata(device);
	eu_variant_map_set_int32(state, "value", velux_klf200->actual_value);
	return true;
}

static bool velux_klf200_device_restore(device_t *device, eu_variant_map_t *state)
{
	device_velux_klf200_t *velux_klf200 = device_get_userdata(device);
	if (eu_variant_map_has(state, "value")) {
		velux_klf200->actual_value = eu_variant_map_get_int32(state, "value");
		if ((velux_klf200->actual_value != 0) && (velux_klf200->actual_value != 1))
			velux_klf200->actual_value = 0;
		velux_klf200->actual_value = velux_klf200->actual_value;
	}
	return true;
}

static bool velux_klf200_worker_check_velux(device_t *device)
{
	device_velux_klf200_t *klf200 = device_get_userdata(device);
	char *cmd = NULL;
	bool rv = true;

	if (!klf200->changed) {
		return false;
	}

	// value 0 = open
	// value 100 = close
	asprintf(&cmd, "%s/velux-klf200.sh %s %s %d %d",
			arguments_get()->execdir,
			klf200->hostname,
			klf200->password,
			klf200->address,
			(klf200->actual_value == 0) ? 0 : 100);

	char *resp = cmd_execute(cmd);
	if (!resp)
		rv = false;

	eu_log_debug("velux klf200 resp: %s", resp);

	klf200->changed = false;

	free(cmd);
	free(resp);

	return rv;
}

static void* velux_klf200_worker_main(void *arguments)
{
	for (;;) {
		bool has_work_done = false;
		eu_list_for_each_declare(node, klf200_devices) {
			device_t *device = eu_list_node_data(node);
			if (velux_klf200_worker_check_velux(device))
				has_work_done = true;
		}

		if (has_work_done)
			continue;
		
		sleep(1);
	}

	return NULL;
}

static bool velux_klf200_technology_init_after_fork(void *arg)
{
	int result_code = pthread_create(&worker_thread, NULL, velux_klf200_worker_main, NULL);
	if (result_code) {
		eu_log_err("Failed to create VELUX KLF200 worker thread! %m");
	}

	return false;
}

static device_type_info_t velux_klf200_info = {
	.name = "VELUX-KLF200",
	.events = EVENT_UP, EVENT_DOWN,
	.actions = ACTION_UP | ACTION_DOWN | ACTION_STOP | ACTION_TOGGLE_UP | ACTION_TOGGLE_DOWN | ACTION_LOCK | ACTION_UNLOCK,
	.conditions = CONDITION_UP | CONDITION_DOWN | CONDITION_RISING | CONDITION_DESCENDING,
	.check_cb = velux_klf200_device_check,
	.parse_cb = velux_klf200_device_parser,
	.exec_cb = velux_klf200_device_exec,
	.state_cb = velux_klf200_device_state,
	.cleanup_cb = velux_klf200_device_cleanup,
	.store_cb = velux_klf200_device_store,
	.restore_cb = velux_klf200_device_restore,
};

bool velux_klf200_technology_init(void)
{
	device_type_register(&velux_klf200_info);

	klf200_devices = eu_list_create();
	eu_event_timer_create(100, velux_klf200_technology_init_after_fork, NULL);

	eu_log_info("Successfully initialized: VELUX KLF200!");

	return true;
}
