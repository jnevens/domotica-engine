/*
 * soltracker.c
 *
 *  Created on: Apr 20, 2018
 *      Author: jnevens
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#include <eu/log.h>
#include <eu/list.h>
#include <eu/timer.h>

#include <SolTrack.h>

#include "device.h"
#include "device_list.h"
#include "utils_time.h"

#include "soltracker.h"

typedef struct {
	double lat;
	double lon;
	double orientation;
	eu_event_timer_t *midnight;
} soltracker_t;

static eu_list_t *soltracker_list = NULL;

static double orientation_norm(double orientation)
{
	if (orientation > 360.0) {
		orientation -= 360.0;
	}
	if (orientation < 0.0) {
		orientation += 360.0;
	}

	if (orientation < 0.0 || orientation > 360.0) {
		orientation = orientation_norm(orientation);
	}

	return orientation;
}

static void soltracker_check_tracker(soltracker_t *st)
{
	struct Position pos;
	struct Time time = { .year = 2018, .month = 4, .day = 19, .hour = 14, .minute = 0, .second = 0, };
	struct Location loc;

	loc.longitude = st->lon / R2D;
	loc.latitude = st->lat / R2D;
	loc.pressure = 101.0;
	loc.temperature = 283.0;

	SolTrack(time, loc, &pos, 0, 1, 0, 0);

	eu_log_debug("Soltracker: %04d-%02d-%02d %02d:%02d:%02d %10.6lf %10.6lf\n",
			time.year, time.month, time.day,
			(int) time.hour + 2, (int) time.minute, (int) time.second,
			pos.azimuthRefract * R2D, pos.altitudeRefract * R2D);

}

static void soltracker_check_all_trackers(void)
{
	eu_list_for_each_declare(node, soltracker_list) {
		soltracker_t *st = eu_list_node_data(node);
		soltracker_check_tracker(st);
	}
}

static bool soltracker_calculate_next_timing_event(void *arg)
{
	uint64_t until_next = (1000 * 60) - (get_current_time_ms() % (1000 * 60) + (30 * 1000));
	// eu_log_debug("schedule event in %d!", until_next);
	eu_event_timer_create(until_next, soltracker_calculate_next_timing_event, (void *)0xdeadbeef);

	if (arg != NULL) {
		soltracker_check_all_trackers();
	}
	return false;
}

static bool soltracker_parser(device_t *device, char *options[])
{
	eu_log_info("Parse soltracker %s", device_get_name(device));

	double temp, lat, lon, orientation;
	int hemisphere;
	eu_log_debug("lat: %s, lon: %s, orientation: %s", options[0], options[1], options[2]);

	if (2 == sscanf(options[0], "%lf%1[Nn]", &temp, &hemisphere)) {
		lat = temp;
	} else if (2 == sscanf(options[0], "%lf%1[Ss]", &temp, &hemisphere)) {
		lat = -temp;
	} else {
		eu_log_err("Cannot parse latitude!");
		return false;
	}
	if (2 == sscanf(options[1], "%lf%1[Ww]", &temp, &hemisphere)) {
		lon = -temp;
	/* this looks different from the others because 77E
	 parses as scientific notation */
	} else if (1 == sscanf(options[1], "%lf%", &temp)
			&& (options[1][strlen(options[1]) - 1] == 'E'
				|| options[1][strlen(options[1]) - 1] == 'e')) {
		lon = temp;
	} else {
		eu_log_err("Cannot parse longitude!");
		return false;
	}
	if (1 == sscanf(options[1], "%lf", &orientation)) {
		eu_log_err("Cannot parse orientation!");
		return false;
	}
	orientation = orientation_norm(orientation);

	eu_log_info("Suntracker: lat: %f lon: %f, orientation: %f", lat, lon, orientation);

	soltracker_t *st = calloc(1, sizeof(soltracker_t));
	if (st) {
		st->lon = lon;
		st->lat = lat;
		st->orientation = orientation;

		eu_list_append(soltracker_list, st);
		device_set_userdata(device, st);
		return true;
	}

	return false;
}

static bool soltracker_check(device_t *device, condition_t *condition)
{
	soltracker_t *st = device_get_userdata(device);
	switch (condition_get_type(condition)) {
	case CONDITION_SET:
		break;
	case CONDITION_UNSET:
		break;
	}
	return false;
}

static bool soltracker_state(device_t *device, eu_variant_map_t *varmap)
{
	soltracker_t *st = device_get_userdata(device);

	eu_variant_map_set_double(varmap, "latitude", st->lat);
	eu_variant_map_set_double(varmap, "longitude", st->lon);
	eu_variant_map_set_double(varmap, "orientation", st->orientation);

	return true;
}

static device_type_info_t soltracker_info = {
	.name = "SUNTRACKER",
	.events = 0,
	.actions = 0,
	.conditions = 0,
	.check_cb = soltracker_check,
	.parse_cb = soltracker_parser,
	.exec_cb = NULL,
	.state_cb = soltracker_state,
};

bool soltracker_technology_init(void)
{
	soltracker_list = eu_list_create();

	device_type_register(&soltracker_info);

	soltracker_calculate_next_timing_event(NULL);

	eu_log_info("Succesfully initialized: soltracker!");
	return true;
}
