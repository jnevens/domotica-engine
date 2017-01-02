/*
 * device_list.c
 *
 *  Created on: Jan 2, 2017
 *      Author: jnevens
 */
#include <stdio.h>
#include <string.h>

#include <bus/list.h>

#include "device_list.h"
#include "device.h"

static list_t *devices = NULL;

bool device_list_init(void)
{
	devices = list_create();
}

list_t *device_list_get(void)
{
	return devices;
}

void device_list_destroy(void)
{
	list_destroy_with_data(devices, (list_destroy_element_fn_t)device_destroy);
	devices = NULL;
}

bool device_list_add(device_t *device)
{
	list_append(devices, device);
}

device_t *device_list_find_by_name(const char *name)
{
	list_node_t *node = NULL;
	list_for_each(node, devices) {
		device_t *device = list_node_data(node);
		if(strcmp(device_get_name(device), name) == 0) {
			return device;
		}
	}
	return NULL;
}

device_t *device_list_find(device_list_find_fn_t find_cb, void *arg) {
	list_node_t *node = NULL;
	list_for_each(node, devices)
	{
		device_t *device = list_node_data(node);
		if (find_cb(device, arg)) {
			return device;
		}
	}
	return NULL;
}