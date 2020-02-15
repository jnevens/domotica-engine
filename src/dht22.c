/*
 * dht22.c
 *
 *  Created on: Feb 8, 2017
 *      Author: jnevens
 */
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <math.h>
#include <fcntl.h>
#include <time.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/time.h>

#include <eu/log.h>
#include <eu/list.h>
#include <eu/event.h>

#include <sqlite3.h>

#include "utils_sched.h"
#include "utils_time.h"
#include "utils_http.h"
#include "device.h"
#include "device_logs.h"
#include "event.h"
#include "action.h"
#include "dht22_private.h"
#include "dht22_worker.h"
#include "dht22.h"



// Defines
#define MEASURMENT_INTERVAL 300

static eu_list_t *dht22_devices = NULL;

static time_t dht22_get_current_log_time_rounded(void)
{
	time_t now, ltime, diff;
	now = time(NULL);

	diff = now % MEASURMENT_INTERVAL;
	if (diff < (MEASURMENT_INTERVAL / 2)) {
		ltime = now - (now % MEASURMENT_INTERVAL);
	} else {
		ltime = now + (MEASURMENT_INTERVAL - (now % MEASURMENT_INTERVAL));
	}

	return ltime;
}

static void dth22_store_measurement_value_remote(const char *name, const char *type, time_t mtime, double value)
{
	char *response = NULL;
	char *url = NULL;
	char ltime_str[32];
	struct tm *ltime_tm = gmtime(&mtime);
	strftime(ltime_str, sizeof(ltime_str), "%Y%m%dT%H:%M:%SZ", ltime_tm);

	asprintf(&url, "http://synweb.local/v1/measurement.php?sensor=%s&type=%s&timestamp=%s&value=%lf&createchannel",
			name,
			type,
			ltime_str,
			value);

	if(!http_request(url, &response)) {
		eu_log_err("Failed storing measurement values: '%s'", url);
	}

	eu_log_info("Logs stored: '%s'", response);

	free(response);
	free(url);
}

static void dht22_store_measurement_values_remote(device_t *device, time_t log_time)
{
	dht22_t *dht22 = device_get_userdata(device);

	dth22_store_measurement_value_remote(device_get_name(device), "temperature", log_time, dht22->temperature);
	dth22_store_measurement_value_remote(device_get_name(device), "humidity", log_time, dht22->humidity);
}

static void dht22_store_measurement_values_sqlite(device_t *device, time_t log_time)
{
	dht22_t *dht22 = device_get_userdata(device);
	struct tm *ltime_tm = gmtime(&log_time);
	sqlite3 *db;
	char ltime_str[32];
	char *err_msg = 0;
	char *sql = NULL;

	strftime(ltime_str, sizeof(ltime_str), "%Y-%m-%d %H:%M:%S", ltime_tm);

	int rc = sqlite3_open("/data/dbs/measurements.sqlite", &db);

	if (rc != SQLITE_OK) {
		eu_log_err("Cannot open database: %s\n", sqlite3_errmsg(db));
		sqlite3_close(db);

		return;
	}

	asprintf(&sql,
			"INSERT INTO measurements (timestamp, value, sensor_id, type_id) VALUES"
			"('%s', '%f', (SELECT id FROM sensors WHERE domname='%s'), (SELECT id FROM types WHERE name='temperature')),"
			"('%s', '%f', (SELECT id FROM sensors WHERE domname='%s'), (SELECT id FROM types WHERE name='humidity'));",
			ltime_str, dht22->temperature, device_get_name(device),
			ltime_str, dht22->humidity, device_get_name(device));

	eu_log_info("DHT Query: %s", sql);

	rc = sqlite3_exec(db, sql, 0, 0, &err_msg);
	free(sql);

	if (rc != SQLITE_OK) {
		eu_log_err("SQL error: %s\n", err_msg);
		sqlite3_free(err_msg);
		sqlite3_close(db);
		return;
	}

	sqlite3_close(db);
}

static void dht22_store_measurement_values(device_t *device)
{
	dht22_t *dht22 = device_get_userdata(device);
	time_t log_time = dht22_get_current_log_time_rounded();
	device_log_add(device, LOG_TEMPERATURE, log_time, dht22->temperature);
	device_log_add(device, LOG_HUMIDITY, log_time, dht22->humidity);
	dht22_store_measurement_values_sqlite(device, log_time);
	//dht22_store_measurement_values_remote(device, log_time);
}

static void dht22_measure_device(device_t *device)
{
	time_t now = time(NULL);
	dht22_t *dht22 = device_get_userdata(device);

	if (dht22->error)
		return;

	device_trigger_event(device, EVENT_TEMPERATURE);
	device_trigger_event(device, EVENT_HUMIDITY);
	dht22_store_measurement_values(device);
}

static void dht22_measure_devices(void)
{
	eu_list_for_each_declare(node, dht22_devices) {
		device_t *device = eu_list_node_data(node);
		dht22_t *dht22 = device_get_userdata(device);

		dht22_measure_device(device);
	}
}

static bool dht22_calculate_next_measurement_moment(void *arg)
{
	uint64_t until_next = (1000 * MEASURMENT_INTERVAL) - (get_current_time_ms() % (1000 * MEASURMENT_INTERVAL));
	eu_event_timer_create(until_next, dht22_calculate_next_measurement_moment, (void *)0xdeadbeef);

	if (arg != NULL) {
		dht22_measure_devices();
	}
	return false;
}

static bool dht22_parser(device_t *device, char *options[])
{
	bool rv = false;
	int gpio = atoi(options[1]);
	eu_log_debug("dht22 gpio: %d", gpio);

	dht22_t *dht22 = calloc(1, sizeof(dht22_t));
	dht22->gpio = gpio;
	dht22->fail_count = 0;
	dht22->error = false;
	dht22->error_time = 0;
	device_set_userdata(device, dht22);

	if (gpio >= 0 && gpio <= 1024)
		rv = true;

	eu_list_append(dht22_devices, device);

	return rv;
}

static bool dht22_device_state(device_t *device, eu_variant_map_t *state)
{
	dht22_t *dht22 = device_get_userdata(device);

	eu_variant_map_set_float(state, "temperature", dht22->temperature);
	eu_variant_map_set_float(state, "humidity", dht22->humidity);
	eu_variant_map_set_bool(state, "error", dht22->error);

	return true;
}

static void dht22_device_cleanup(device_t *device)
{
	dht22_t *dht22 = device_get_userdata(device);
	free(dht22);
}

static bool dht22_technology_init_after_fork(void *arg)
{
	dht22_calculate_next_measurement_moment(NULL);
	dht22_worker_init(dht22_devices);
	return false;
}

static device_type_info_t dht22_info = {
	.name = "DHT22",
	.events = EVENT_TEMPERATURE | EVENT_HUMIDITY,
	.actions = 0,
	.conditions = 0,
	.check_cb = NULL,
	.parse_cb = dht22_parser,
	.exec_cb = NULL,
	.state_cb = dht22_device_state,
	.cleanup_cb = dht22_device_cleanup,
};

bool dht22_technology_init(void)
{
	device_type_register(&dht22_info);

	dht22_devices = eu_list_create();

	eu_log_info("Succesfully initialized: Sunriset!");
	eu_event_timer_create(1, dht22_technology_init_after_fork, NULL);
	return true;
}
