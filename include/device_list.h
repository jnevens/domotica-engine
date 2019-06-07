/*
 * device_list.h
 *
 *  Created on: Jan 2, 2017
 *      Author: jnevens
 */

#ifndef DEVICE_LIST_H_
#define DEVICE_LIST_H_

#include <stdbool.h>
#include <eu/list.h>
#include "types.h"

typedef bool (*device_list_iter_fn_t)(device_t *device, void *arg);

bool device_list_init(void);
eu_list_t *device_list_get(void);
void device_list_destroy(void);

bool device_list_add(device_t *device);
device_t *device_list_find_by_name(const char *name);
device_t *device_list_find(device_list_iter_fn_t find_cb, void *arg);
void device_list_foreach(device_list_iter_fn_t iter_cb, void *arg);

#endif /* DEVICE_LIST_H_ */
