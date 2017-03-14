/*
 * sun.c
 *
 *  Created on: Jan 10, 2017
 *      Author: jnevens
 */
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <math.h>
#include <time.h>

#include <bus/log.h>
#include <bus/event.h>
#include <bus/timer.h>

#include "utils_sunriset.h"
#include "device.h"
#include "event.h"
#include "action.h"
#include "sunriset.h"

typedef enum {
	SUNRISET_EVENT_RISE= 0,
	SUNRISET_EVENT_SET,
	SUNRISET_EVENT_RISE_CIV,
	SUNRISET_EVENT_SET_CIV,
	SUNRISET_EVENT_RISE_NAUT,
	SUNRISET_EVENT_SET_NAUT,
	SUNRISET_EVENT_RISE_ASTR,
	SUNRISET_EVENT_SET_ASTR,
	SUNRISET_EVENT_COUNT
} sunriset_event_e;

typedef struct {
	double lat;
	double lon;
	event_timer_t *midnight;
	event_timer_t *sunriset_timers[SUNRISET_EVENT_COUNT];
} sunriset_t;

typedef struct {
	sunriset_event_e se;
	device_t *device;
} sunriset_event_t;

static void sunriset_calculate_next_run(device_t *device);

bool sunriset_event_callback(void *arg)
{
	sunriset_event_t *sunevent = arg;
	sunriset_t *sr = device_get_userdata(sunevent->device);

	switch (sunevent->se) {
	case SUNRISET_EVENT_RISE:
		device_trigger_event(sunevent->device, EVENT_SUNRISE);
		break;
	case SUNRISET_EVENT_RISE_CIV:
		device_trigger_event(sunevent->device, EVENT_SUNRISE_CIV);
		break;
	case SUNRISET_EVENT_RISE_NAUT:
		device_trigger_event(sunevent->device, EVENT_SUNRISE_NAUT);
		break;
	case SUNRISET_EVENT_RISE_ASTR:
		device_trigger_event(sunevent->device, EVENT_SUNRISE_ASTRO);
		break;
	case SUNRISET_EVENT_SET:
		device_trigger_event(sunevent->device, EVENT_SUNSET);
		break;
	case SUNRISET_EVENT_SET_CIV:
		device_trigger_event(sunevent->device, EVENT_SUNSET_CIV);
		break;
	case SUNRISET_EVENT_SET_NAUT:
		device_trigger_event(sunevent->device, EVENT_SUNSET_NAUT);
		break;
	case SUNRISET_EVENT_SET_ASTR:
		device_trigger_event(sunevent->device, EVENT_SUNSET_ASTRO);
		break;
	default:
		break;
	}

	sr->sunriset_timers[sunevent->se] = NULL;
	free(sunevent);
	return false;
}

static void sunriset_create_event_timer(device_t *device, sunriset_event_e se, int hour, int min)
{
	sunriset_t *sr = device_get_userdata(device);
	time_t ct = time(NULL), et;
	ctime(&ct);
	struct tm *tm = localtime(&ct);
	tm->tm_sec = 0;
	tm->tm_min = min;
	tm->tm_hour = hour;
	et = mktime(tm);

	log_debug("Schedule event: %d @%02d:%02dh UTC in %ds", se, hour, min, et - ct);

	if (et > ct) {
		log_debug("Event in future: %ds", et - ct);
		if (sr->sunriset_timers[se]) {
			free(event_timer_get_userdata(sr->sunriset_timers[se]));
			event_timer_destroy(sr->sunriset_timers[se]);
		}
		sunriset_event_t *sunevent = calloc(1, sizeof(sunriset_event_t));
		sunevent->device = device;
		sunevent->se = se;
		sr->sunriset_timers[se] = event_timer_create((et - ct) * 1000, sunriset_event_callback, sunevent);
	} else {
		log_notice("Event already in history!");
	}
}

static void sunriset_calculate(device_t *device)
{
	sunriset_t *sr = device_get_userdata(device);
	int rs;
	int year,month,day;
	double rise, set;
	double rise_civ, set_civ;
	double rise_naut, set_naut;
	double rise_astr, set_astr;
	time_t tt;
	struct tm *tm;

	tt = time(NULL);
	ctime(&tt);
	tm = localtime(&tt);

	year = 1900 + tm->tm_year;
	month = 1 + tm->tm_mon;
	day = 1 + tm->tm_mday;

	log_debug("date: %d-%d-%d", year, month, day);

	sun_rise_set(year, month, day, sr->lon, sr->lat, &rise, &set);
	civil_twilight(year, month, day, sr->lon, sr->lat, &rise_civ, &set_civ);
	nautical_twilight(year, month, day, sr->lon, sr->lat, &rise_naut, &set_naut);
	astronomical_twilight(year, month, day, sr->lon, sr->lat, &rise_astr, &set_astr);

	log_info( "sunrise:              %2.2d:%2.2d UTC, sunset %2.2d:%2.2d UTC",
		TMOD(HOURS(rise)), MINUTES(rise),
		TMOD(HOURS(set)), MINUTES(set));

	log_info( "sunrise civilian:     %2.2d:%2.2d UTC, sunset %2.2d:%2.2d UTC",
		TMOD(HOURS(rise_civ)), MINUTES(rise_civ),
		TMOD(HOURS(set_civ)), MINUTES(set_civ));
	log_info( "sunrise nautical:     %2.2d:%2.2d UTC, sunset %2.2d:%2.2d UTC",
		TMOD(HOURS(rise_naut)), MINUTES(rise_naut),
		TMOD(HOURS(set_naut)), MINUTES(set_naut));
	log_info( "sunrise astronomical: %2.2d:%2.2d UTC, sunset %2.2d:%2.2d UTC",
		TMOD(HOURS(rise_astr)), MINUTES(rise_astr),
		TMOD(HOURS(set_astr)), MINUTES(set_astr));

	sunriset_create_event_timer(device, SUNRISET_EVENT_RISE, TMOD(HOURS(rise)), MINUTES(rise));
	sunriset_create_event_timer(device, SUNRISET_EVENT_SET, TMOD(HOURS(set)), MINUTES(set));
	sunriset_create_event_timer(device, SUNRISET_EVENT_RISE_CIV, TMOD(HOURS(rise_civ)), MINUTES(rise_civ));
	sunriset_create_event_timer(device, SUNRISET_EVENT_SET_CIV, TMOD(HOURS(set_civ)), MINUTES(set_civ));
	sunriset_create_event_timer(device, SUNRISET_EVENT_RISE_NAUT, TMOD(HOURS(rise_naut)), MINUTES(rise_naut));
	sunriset_create_event_timer(device, SUNRISET_EVENT_SET_NAUT, TMOD(HOURS(set_naut)), MINUTES(set_naut));
	sunriset_create_event_timer(device, SUNRISET_EVENT_RISE_ASTR, TMOD(HOURS(rise_astr)), MINUTES(rise_astr));
	sunriset_create_event_timer(device, SUNRISET_EVENT_SET_ASTR, TMOD(HOURS(set_astr)), MINUTES(set_astr));
}

static bool sunriset_next_run_callback(void *arg)
{
	device_t *device = arg;
	sunriset_t *sr = device_get_userdata(device);
	sunriset_calculate(device);
	sr->midnight = NULL;
	sunriset_calculate_next_run(device);
	return false;
}

static void sunriset_calculate_next_run(device_t *device)
{
	sunriset_t *sr = device_get_userdata(device);
	time_t ct = time(NULL), nrt;
	ctime(&ct);
	struct tm *tm = localtime(&ct);
	tm->tm_sec = 0;
	tm->tm_min = 0;
	tm->tm_hour = 0;
	tm->tm_mday++;
	nrt = mktime(tm);
	log_debug("Next day start in: %d seconds", nrt - ct);

	sr->midnight = event_timer_create((nrt - ct) * 1000, sunriset_next_run_callback, device);
}

static bool sunriset_parser(device_t *device, char *options[])
{
	log_debug("parse sunriset %s!", device_get_name(device));
	sunriset_t *sunriset = calloc(1, sizeof(sunriset_t));
	if (sunriset) {
		double temp;
		int hemisphere;
		log_debug("%s %s", options[0], options[1]);

		if (2 == sscanf(options[0], "%lf%1[Nn]", &temp, &hemisphere)) {
			sunriset->lat = temp;
			//coords_set |= LAT_SET;
		}
		if (2 == sscanf(options[0], "%lf%1[Ss]", &temp, &hemisphere)) {
			sunriset->lat = -temp;
			//coords_set |= LAT_SET;
		}
		if (2 == sscanf(options[1], "%lf%1[Ww]", &temp, &hemisphere)) {
			sunriset->lon = -temp;
			//coords_set |= LON_SET;
		}
		/* this looks different from the others because 77E
		 parses as scientific notation */
		if (1 == sscanf(options[1], "%lf%", &temp, &hemisphere)
				&& (options[1][strlen(options[1]) - 1] == 'E'
					|| options[1][strlen(options[1]) - 1] == 'e')) {
			sunriset->lon = temp;
			//coords_set |= LON_SET;
		}

		log_info("sunriset: lat: %f lon: %f", sunriset->lat, sunriset->lon);

		device_set_userdata(device, sunriset);
		sunriset_calculate(device);
		sunriset_calculate_next_run(device);
		return true;
	}
	return false;
}

bool sunriset_technology_init(void)
{
	event_type_e events = EVENT_SUNRISE | EVENT_SUNSET;
	action_type_e actions = 0;
	device_register_type("SUNRISET", events, actions, sunriset_parser, NULL);

	log_info("Succesfully initialized: Sunriset!");
	return true;
}
