/*
 * device_logs.c
 *
 *  Created on: Aug 1, 2019
 *      Author: jnevens
 */
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include <eu/log.h>

#include "utils_string.h"
#include "types.h"
#include "device.h"
#include "device_logs.h"

typedef struct device_log_entry_s
{
	time_t ts;
	double value;
} device_log_entry_t;

#define LOG_ENTRIES 12*24 // every 5min, 24h

typedef struct device_logs_s
{
	device_t *device;
	device_log_type_e type;
	size_t entries_count;
	device_log_entry_t entries[LOG_ENTRIES];
} device_log_t;

static device_log_t **device_logs = NULL;
static size_t device_logs_count = 0;

static device_log_t *device_logs_lookup(device_t *device, device_log_type_e type)
{
	device_log_t *device_log = NULL;

	for (size_t i = 0; i < device_logs_count; i++) {
		if (device_logs[i]->device == device && device_logs[i]->type == type)
			return device_logs[i];
	}

	return device_log;
}

static device_log_t *device_log_create(device_t *device, device_log_type_e type)
{
	device_log_t *device_log = calloc(1, sizeof(device_log_t));
	device_log->device = device;
	device_log->type = type;
	device_logs = realloc(device_logs, (device_logs_count + 1) * sizeof(device_log_t));
	device_logs[device_logs_count] = device_log;
	device_logs_count++;
	return device_log;
}

bool device_log_add(device_t *device, device_log_type_e type, time_t ts, double value)
{
	// check device log exists
	device_log_t *device_log = device_logs_lookup(device, type);
	if (!device_log)
		device_log = device_log_create(device, type);

	eu_log_info("log entry: %s, type:%d, ts:%u, value:%f", device_get_name(device), type, ts, value);

	// move old data
	memmove(&device_log->entries[1], &device_log->entries[0], sizeof(device_log_entry_t) * (LOG_ENTRIES - 1));

	// add measurement
	device_log->entries[0].ts = ts;
	device_log->entries[0].value = value;
	if (device_log->entries_count < LOG_ENTRIES)
		device_log->entries_count++;

	return true;
}

bool device_has_logs(device_t *device)
{
	for (int i = 0; i < device_logs_count; i++) {
		if (device_logs[i]->device == device) {
			if (device_logs[i]->entries_count)
				return true;
		}
	}

	return false;
}

char *device_logs_get_json(device_t *device, time_t interval)
{
	char *json = strdup("{\n");
	for (int i = 0; i < device_logs_count; i++) {
		if (device_logs[i]->device == device) {
			size_t entry_count = 0;

			if (strlen(json) > 5)
				strappend(&json, "\t,\n");

			if (device_logs[i]->type == LOG_HUMIDITY)
				strappend(&json, "\t\"humidity\":\n\t[");
			else if (device_logs[i]->type == LOG_TEMPERATURE)
				strappend(&json, "\t\"temperature\":\n\t[");

			for (size_t j = 0; j < device_logs[i]->entries_count; j++) {
				char buf[64];
				device_log_entry_t *entry = &device_logs[i]->entries[j];
				char ltime_str[32];
				struct tm *ltime_tm = gmtime(&entry->ts);

				if (entry->ts < (time(NULL) - interval))
					break;

				strftime(ltime_str, sizeof(ltime_str), "%Y%m%dT%H%M%SZ", ltime_tm);

				if (entry_count > 0)
					strappend(&json, ",");

				snprintf(buf, sizeof(buf), "{\"%s\":%f}", ltime_str, entry->value);
				strappend(&json, buf);
				entry_count++;
			}
			strappend(&json, "\t]\n");
		}
	}
	strappend(&json, "}\n");
	return json;
}

bool device_logs_store(void)
{
	return false;
}

bool device_logs_load(void)
{
	return false;
}

