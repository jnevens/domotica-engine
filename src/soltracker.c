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
#include <time.h>


#include <eu/log.h>
#include <eu/list.h>
#include <eu/event.h>

#include <SolTrack.h>

#include "device.h"
#include "device_list.h"
#include "engine.h"
#include "utils_time.h"

#include "soltracker.h"

typedef struct {
	double lat;
	double lon;
	double azimuth;
	double altitude;
	double angle;
	double view_angle;
	device_t *device;
	bool in_sun;
} soltracker_t;

static eu_list_t *soltracker_list = NULL;
static eu_event_timer_t *soltrack_timer = NULL;

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

static void soltracker_calculate_position(double lon, double lat, double *azi, double *alt)
{
	time_t t = time(NULL);
	struct tm *ts = gmtime(&t);
	struct Position pos;
	struct Time time = {
			.year = ts->tm_year + 1900,
			.month = ts->tm_mon + 1,
			.day = ts->tm_mday,
			.hour = ts->tm_hour,
			.minute = ts->tm_min,
			.second = ts->tm_sec,
		};
	struct Location loc = {
			.longitude = lon / R2D,
			.latitude = lat / R2D,
			.pressure = 101.0,
			.temperature = 283.0,
		};

	SolTrack(time, loc, &pos, 0, 1, 0, 0);

	eu_log_debug("Soltracker: %04d-%02d-%02d %02d:%02d:%02d %10.6lf %10.6lf",
			time.year, time.month, time.day,
			(int) time.hour + 2, (int) time.minute, (int) time.second,
			pos.azimuthRefract * R2D, pos.altitudeRefract * R2D);

	*azi = pos.azimuthRefract * R2D;
	*alt = pos.altitudeRefract * R2D;
}

static void soltracker_check_tracker(soltracker_t *st)
{
	device_t *device = st->device;
	bool in_sun = false;
	double azi = 0.0;
	double alt = 0.0;

	soltracker_calculate_position(st->lon, st->lat, &azi, &alt);

	st->azimuth = azi;
	st->altitude = alt;

	double angle_on = st->angle - st->view_angle/2;
	double angle_off = st->angle + st->view_angle/2;

	if ((alt >= 0.0) && (azi > angle_on) && (azi < angle_off)) {
		in_sun = true;
	}

	eu_log_info("soltracker %s (on/off: %lf/%lf) in %s",
			device_get_name(device),
			angle_on, angle_off,
			(in_sun) ? "SUN" : "SHADOW");

	if (in_sun != st->in_sun) {
		st->in_sun = in_sun;
		event_t *event = event_create(device, (in_sun) ? EVENT_SET : EVENT_UNSET);
		engine_trigger_event(event);
		event_destroy(event);
	}

	event_t *event = event_create(device, EVENT_SUN_POSITION);
	event_option_set_double(event, "azimuth", azi);
	event_option_set_double(event, "altitude", alt);
	engine_trigger_event(event);
	event_destroy(event);
}

static void soltracker_check_all_trackers(void)
{
	eu_list_for_each_declare(node, soltracker_list) {
		soltracker_t *st = eu_list_node_data(node);
		soltracker_check_tracker(st);
	}
}

static bool soltracker_parser(device_t *device, char *options[])
{
	eu_log_info("Parse soltracker %s", device_get_name(device));

	double temp, lat, lon, angle, view_angle = 180;
	int hemisphere;
	eu_log_debug("lat: %s, lon: %s", options[0], options[1]);

	if ((options[0]) && (2 == sscanf(options[0], "%lf%1[Nn]", &temp, &hemisphere))) {
		lat = temp;
	} else if (2 == sscanf(options[0], "%lf%1[Ss]", &temp, &hemisphere)) {
		lat = -temp;
	} else {
		eu_log_err("Cannot parse latitude!");
		return false;
	}
	if ((options[1]) && (2 == sscanf(options[1], "%lf%1[Ww]", &temp, &hemisphere))) {
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
	if ((options[2]) && (1 != sscanf(options[2], "%lf", &angle))) {
		eu_log_err("Cannot parse tracker angle!");
		return false;
	}

	if (options[3]) {
		sscanf(options[3], "%lf", &view_angle);
	}

	eu_log_info("Suntracker: lat: %f lon: %f, angle: %f, view_angle: %f", lat, lon, angle, view_angle);

	soltracker_t *st = calloc(1, sizeof(soltracker_t));
	if (st) {
		double azi = 0.0;
		double alt = 0.0;

		soltracker_calculate_position(lon, lat, &azi, &alt);

		st->lon = lon;
		st->lat = lat;
		st->azimuth = azi;
		st->altitude = alt;
		st->angle = angle;
		st->view_angle = view_angle;

		eu_list_append(soltracker_list, st);
		st->device = device;
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
	eu_variant_map_set_double(varmap, "azimuth", st->azimuth);
	eu_variant_map_set_double(varmap, "altitude", st->altitude);
	eu_variant_map_set_double(varmap, "angle", st->angle);
	eu_variant_map_set_double(varmap, "view_angle", st->view_angle);
	eu_variant_map_set_bool(varmap, "in_sun", st->in_sun);

	return true;
}

static bool soltrack_timer_cb(void *arg)
{
	soltracker_check_all_trackers();

	return eu_event_timer_continue;
}

static device_type_info_t soltracker_info = {
	.name = "SOLTRACKER",
	.events = EVENT_SET | EVENT_UNSET,
	.actions = 0,
	.conditions = CONDITION_SET | CONDITION_UNSET,
	.check_cb = soltracker_check,
	.parse_cb = soltracker_parser,
	.exec_cb = NULL,
	.state_cb = soltracker_state,
};

bool soltracker_technology_init(void)
{
	soltracker_list = eu_list_create();

	device_type_register(&soltracker_info);

	soltrack_timer = eu_event_timer_create(60 * 1000, soltrack_timer_cb, NULL);

	eu_log_info("Succesfully initialized: soltracker!");
	return true;
}

void soltracker_technology_cleanup(void)
{
	eu_event_timer_destroy(soltrack_timer);
}
