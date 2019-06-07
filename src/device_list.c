/*
 * device_list.c
 *
 *  Created on: Jan 2, 2017
 *      Author: jnevens
 */
#include <stdio.h>
#include <string.h>

#include <eu/list.h>

#include "device_list.h"
#include "device.h"

static eu_list_t *devices = NULL;

bool device_list_init(void)
{
	devices = eu_list_create();
}

eu_list_t *device_list_get(void)
{
	return devices;
}

void device_list_destroy(void)
{
	eu_list_destroy_with_data(devices, (eu_list_destroy_element_fn_t)device_destroy);
	devices = NULL;
}

bool device_list_add(device_t *device)
{
	eu_list_append(devices, device);
}

device_t *device_list_find_by_name(const char *name)
{
	eu_list_node_t *node = NULL;
	eu_list_for_each(node, devices) {
		device_t *device = eu_list_node_data(node);
		const char *device_name = device_get_name(device);
		if(device_name && (strcmp(device_name, name) == 0)) {
			return device;
		}
	}
	return NULL;
}

device_t *device_list_find(device_list_iter_fn_t find_cb, void *arg) {
	eu_list_node_t *node = NULL;
	eu_list_for_each(node, devices)
	{
		device_t *device = eu_list_node_data(node);
		if (find_cb(device, arg)) {
			return device;
		}
	}
	return NULL;
}

void device_list_foreach(device_list_iter_fn_t iter_cb, void *arg) {
	eu_list_node_t *node = NULL;
	eu_list_for_each(node, devices)
	{
		device_t *device = eu_list_node_data(node);
		iter_cb(device, arg);
	}
}
