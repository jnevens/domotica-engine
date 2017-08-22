/*
 * condition.c
 *
 *  Created on: Jun 27, 2017
 *      Author: jnevens
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <eu/log.h>
#include <eu/list.h>

#include "device.h"

#include "condition.h"

struct condition_s {
	condition_type_e type;
	device_t *device;
};

#define X(a, b, c) c,
static char *condition_type_name[] = {
		CONDITION_TYPE_TABLE
		NULL
};
#undef X

condition_t *condition_create(device_t *device, condition_type_e condition_type, char *options[])
{
	condition_t *condition = calloc(1, sizeof(condition_t));
	if (condition) {
		condition->device = device;
		condition->type = condition_type;
	}

	return condition;
}

condition_type_e condition_type_from_char(const char *condition_str)
{
	int i = 0;

	if (condition_str == NULL)
		return -1;

	while (condition_type_name[i]) {
		if (strcmp(condition_type_name[i], condition_str) == 0) {
			return (1 << i);
		}
		i++;
	}
	eu_log_err("Cannot find action type: %s", condition_str);
	return -1;
}

const char *condition_type_to_char(condition_type_e type)
{
	int i;
	const char *type_name = NULL;
	for(i = 0; condition_type_name[i] != NULL; i++) {
		if (type & 0x1) {
			type_name = condition_type_name[i];
			break;
		}
		type >>= 1;
	}
	return type_name;
}

condition_type_e condition_get_type(condition_t *condition)
{
	return condition->type;
}

void condition_destroy(condition_t *condition)
{
	free(condition);
}

bool condition_check(condition_t *condition)
{
	return device_check(condition->device, condition);
}
