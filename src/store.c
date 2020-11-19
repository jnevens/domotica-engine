/*
 * store.c
 *
 *  Created on: 18 Dec 2018
 *      Author: jnevens
 */
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>
#include <time.h>

#include <json-c/json.h>

#include <eu/log.h>
#include <eu/variant.h>
#include <eu/variant_map.h>
#include <eu/event.h>

#include "device.h"
#include "device_list.h"
#include "store.h"

#define STORE_DEVICES_STATE_FILE "/data/domotica-engines.states.json"

static eu_event_timer_t *store_timer = NULL;

/*
 * Hash function for strings: http://www.cse.yorku.ca/~oz/hash.html djb2
 * */
static unsigned long hash_djb2(unsigned char *str)
{
	unsigned long hash = 5381;
	int c;

	while (c = *str++)
		hash = ((hash << 5) + hash) + c; /* hash * 33 + c */

	return hash;
}

int restore_devices_state(void)
{
	if (access(STORE_DEVICES_STATE_FILE, R_OK | W_OK) != 0)
		return 0;

	json_object *obj = json_object_from_file(STORE_DEVICES_STATE_FILE);
	time_t time = (time_t) json_object_get_int(json_object_object_get(obj, "time"));
	json_object *obj_devices = json_object_object_get(obj, "devices");
	eu_variant_map_t *devices_state = eu_variant_map_deserialize(obj_devices);

	if (eu_variant_map_count(devices_state) > 0) {
		eu_variant_map_pair_t *pair = NULL;
		eu_variant_map_for_each_pair(pair, devices_state) {
			const char *device_name = eu_variant_map_pair_get_key(pair);
			eu_log_info("Restore state of device: %s", device_name);
			eu_variant_t *var = eu_variant_map_pair_get_val(pair);
			eu_variant_map_t *device_state = eu_variant_da_map(var);
			device_t *device = device_list_find_by_name(device_name);
			if (device) {
				device_restore(device, device_state);
			}
		}
	}

	eu_variant_map_destroy(devices_state);
	json_object_put(obj);
	return 0;
}

static bool store_device_cb(device_t *device, void *arg)
{
	eu_variant_map_t *devices_state  = (eu_variant_map_t *)arg;
	eu_variant_map_t *device_state = device_store(device);
	if (device_state) {
		eu_log_info("store state of device: %s", device_get_name(device));
		eu_variant_t *var = eu_variant_create(EU_VARIANT_TYPE_MAP);
		eu_variant_set_map(var, device_state);
		eu_variant_map_set_variant(devices_state, device_get_name(device), var);
	}

	return true;
}

int store_devices_state(void)
{
	eu_variant_map_t *devices_state = eu_variant_map_create();

	device_list_foreach(store_device_cb, devices_state);

	json_object *obj = json_object_new_object();
	json_object_object_add(obj, "time", json_object_new_int((int) time(NULL)));
	json_object_object_add(obj, "devices", eu_variant_map_serialize(devices_state));
	json_object_to_file(STORE_DEVICES_STATE_FILE, obj);

	eu_variant_map_destroy(devices_state);
	json_object_put(obj);
	return 0;
}

bool store_device_states_timer_cb(void *arg)
{
	eu_log_info("Store state of all devices!");
	store_devices_state();
	return true;
}

int store_init(void)
{
	restore_devices_state();
	store_devices_state();
	store_timer = eu_event_timer_create(300 * 1000,  store_device_states_timer_cb, NULL);
	return restore_devices_state();
}
