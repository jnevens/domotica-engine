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

#include <eu/log.h>
#include <eu/event.h>
#include <eu/list.h>

#include "utils_sunriset.h"
#include "device.h"
#include "event.h"
#include "action.h"
#include "sunriset.h"

typedef enum
{
	SUNRISET_EVENT_RISE = 0,
	SUNRISET_EVENT_SET,
	SUNRISET_EVENT_RISE_CIV,
	SUNRISET_EVENT_SET_CIV,
	SUNRISET_EVENT_RISE_NAUT,
	SUNRISET_EVENT_SET_NAUT,
	SUNRISET_EVENT_RISE_ASTR,
	SUNRISET_EVENT_SET_ASTR,
	SUNRISET_EVENT_COUNT
} sunriset_event_e;

typedef struct
{
	double lat;
	double lon;
	double rise;
	double set;
	double rise_civ;
	double set_civ;
	double rise_naut;
	double set_naut;
	double rise_astr;
	double set_astr;
	bool rised;
	bool rised_civ;
	bool rised_naut;
	bool rised_astr;
	eu_event_timer_t *midnight;
	eu_event_timer_t *sunriset_timers[SUNRISET_EVENT_COUNT];
} sunriset_t;

typedef struct
{
	sunriset_event_e se;
	device_t *device;
} sunriset_event_t;

static void sunriset_calculate_next_run(device_t *device);
static void sunriset_current_state(device_t *device);

bool sunriset_event_callback(void *arg)
{
	sunriset_event_t *sunevent = arg;
	sunriset_t *sr = device_get_userdata(sunevent->device);
	device_t *device = sunevent->device;

	switch (sunevent->se)
	{
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

	sunriset_current_state(device);
	return false;
}

static void sunriset_create_event_timer(device_t *device, sunriset_event_e se, int hour, int min)
{
	sunriset_t *sr = device_get_userdata(device);
	time_t ct = time(NULL), et;

	if (ctime(&ct) == NULL) {
		eu_log_err("Failed creating time!");
		return;
	}

	struct tm *tm = localtime(&ct);
	tm->tm_sec = 0;
	tm->tm_min = min;
	tm->tm_hour = hour;
	et = mktime(tm);

	eu_log_debug("Schedule event: %d @%02d:%02dh UTC in %ds", se, hour, min, et - ct);

	if (sr->sunriset_timers[se]) {
		free(eu_event_timer_get_userdata(sr->sunriset_timers[se]));
		eu_event_timer_destroy(sr->sunriset_timers[se]);
		sr->sunriset_timers[se] = NULL;
	}

	if (et > ct) {
		eu_log_debug("Event in future: %ds", et - ct);
		sunriset_event_t *sunevent = calloc(1, sizeof(sunriset_event_t));
		sunevent->device = device;
		sunevent->se = se;
		sr->sunriset_timers[se] = eu_event_timer_create((et - ct) * 1000, sunriset_event_callback, sunevent);
	} else {
		eu_log_notice("Event already in history!");
	}
}

static void sunriset_current_state(device_t *device)
{
	sunriset_t *sr = device_get_userdata(device);

	if (sr->sunriset_timers[SUNRISET_EVENT_RISE_ASTR]) {
		sr->rised_astr = false;
	} else if (sr->sunriset_timers[SUNRISET_EVENT_SET_ASTR]) {
		sr->rised_astr = true;
	}

	if (sr->sunriset_timers[SUNRISET_EVENT_RISE_NAUT]) {
		sr->rised_naut = false;
	} else if (sr->sunriset_timers[SUNRISET_EVENT_SET_NAUT]) {
		sr->rised_naut = true;
	}

	if (sr->sunriset_timers[SUNRISET_EVENT_RISE_CIV]) {
		sr->rised_civ = false;
	} else if (sr->sunriset_timers[SUNRISET_EVENT_SET_CIV]) {
		sr->rised_civ = true;
	}
	
	if (sr->sunriset_timers[SUNRISET_EVENT_RISE]) {
		sr->rised = false;
	} else if (sr->sunriset_timers[SUNRISET_EVENT_SET]) {
		sr->rised = true;
	}

	eu_log_info("sunrised:              %d", sr->rised);
	eu_log_info("sunrised civilian:     %d", sr->rised_civ);
	eu_log_info("sunrised nautical:     %d", sr->rised_naut);
	eu_log_info("sunrised astronomical: %d", sr->rised_astr);
}

static void sunriset_calculate(device_t *device)
{
	sunriset_t *sr = device_get_userdata(device);
	int year, month, day;
	double rise, set;
	double rise_civ, set_civ;
	double rise_naut, set_naut;
	double rise_astr, set_astr;
	double tz_corr = 0.0;
	time_t ct;
	struct tm *tm;

	ct = time(NULL);
	if (ctime(&ct) == NULL) {
		eu_log_err("Failed creating time!");
		return;
	}
	tm = localtime(&ct);

	year = 1900 + tm->tm_year;
	month = 1 + tm->tm_mon;
	day = 1 + tm->tm_mday;

	eu_log_debug("date: %d-%d-%d %d:%d:%d %s GMT%s%ds", year, month, day, tm->tm_hour, tm->tm_min, tm->tm_sec,
				 tm->tm_zone, (tm->tm_gmtoff >= 0) ? "+" : "", tm->tm_gmtoff);

	tz_corr = (tm->tm_gmtoff * 1.0) / 3600.0;

	sun_rise_set(year, month, day, sr->lon, sr->lat, &rise, &set);
	civil_twilight(year, month, day, sr->lon, sr->lat, &rise_civ, &set_civ);
	nautical_twilight(year, month, day, sr->lon, sr->lat, &rise_naut, &set_naut);
	astronomical_twilight(year, month, day, sr->lon, sr->lat, &rise_astr, &set_astr);

	rise += tz_corr;
	set += tz_corr;
	rise_civ += tz_corr;
	set_civ += tz_corr;
	rise_naut += tz_corr;
	set_naut += tz_corr;
	rise_astr += tz_corr;
	set_astr += tz_corr;

	eu_log_info("sunrise:              %2.2d:%2.2d %s, sunset %2.2d:%2.2d %s",
				TMOD(HOURS(rise)), MINUTES(rise), tm->tm_zone,
				TMOD(HOURS(set)), MINUTES(set), tm->tm_zone);
	eu_log_info("sunrise civilian:     %2.2d:%2.2d %s, sunset %2.2d:%2.2d %s",
				TMOD(HOURS(rise_civ)), MINUTES(rise_civ), tm->tm_zone,
				TMOD(HOURS(set_civ)), MINUTES(set_civ), tm->tm_zone);
	eu_log_info("sunrise nautical:     %2.2d:%2.2d %s, sunset %2.2d:%2.2d %s",
				TMOD(HOURS(rise_naut)), MINUTES(rise_naut), tm->tm_zone,
				TMOD(HOURS(set_naut)), MINUTES(set_naut), tm->tm_zone);
	eu_log_info("sunrise astronomical: %2.2d:%2.2d %s, sunset %2.2d:%2.2d %s",
				TMOD(HOURS(rise_astr)), MINUTES(rise_astr), tm->tm_zone,
				TMOD(HOURS(set_astr)), MINUTES(set_astr), tm->tm_zone);

	sr->rise = rise;
	sr->set = set;
	sr->rise_civ = rise_civ;
	sr->set_civ = set_civ;
	sr->rise_naut = rise_naut;
	sr->set_naut = set_naut;
	sr->rise_astr = rise_astr;
	sr->set_astr = set_astr;

	sunriset_create_event_timer(device, SUNRISET_EVENT_RISE, TMOD(HOURS(rise)), MINUTES(rise));
	sunriset_create_event_timer(device, SUNRISET_EVENT_SET, TMOD(HOURS(set)), MINUTES(set));
	sunriset_create_event_timer(device, SUNRISET_EVENT_RISE_CIV, TMOD(HOURS(rise_civ)), MINUTES(rise_civ));
	sunriset_create_event_timer(device, SUNRISET_EVENT_SET_CIV, TMOD(HOURS(set_civ)), MINUTES(set_civ));
	sunriset_create_event_timer(device, SUNRISET_EVENT_RISE_NAUT, TMOD(HOURS(rise_naut)), MINUTES(rise_naut));
	sunriset_create_event_timer(device, SUNRISET_EVENT_SET_NAUT, TMOD(HOURS(set_naut)), MINUTES(set_naut));
	sunriset_create_event_timer(device, SUNRISET_EVENT_RISE_ASTR, TMOD(HOURS(rise_astr)), MINUTES(rise_astr));
	sunriset_create_event_timer(device, SUNRISET_EVENT_SET_ASTR, TMOD(HOURS(set_astr)), MINUTES(set_astr));

	sunriset_current_state(device);
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
	if (ctime(&ct) == NULL) {
		eu_log_err("Failed creating time!");
		return;
	}
	struct tm *tm = localtime(&ct);
	tm->tm_sec = 0;
	tm->tm_min = 0;
	tm->tm_hour = 0;
	tm->tm_mday++;
	nrt = mktime(tm);
	eu_log_debug("Next day start in: %d seconds", nrt - ct);

	sr->midnight = eu_event_timer_create((nrt - ct) * 1000, sunriset_next_run_callback, device);
}

static bool sunriset_parser(device_t *device, char *options[])
{
	eu_log_debug("parse sunriset %s!", device_get_name(device));
	sunriset_t *sunriset = calloc(1, sizeof(sunriset_t));
	if (sunriset) {
		double temp;
		int hemisphere;
		eu_log_debug("%s %s", options[0], options[1]);

		if (2 == sscanf(options[0], "%lf%1[Nn]", &temp, &hemisphere)) {
			sunriset->lat = temp;
		}
		if (2 == sscanf(options[0], "%lf%1[Ss]", &temp, &hemisphere)) {
			sunriset->lat = -temp;
		}
		if (2 == sscanf(options[1], "%lf%1[Ww]", &temp, &hemisphere)) {
			sunriset->lon = -temp;
		}
		/* this looks different from the others because 77E
		 parses as scientific notation */
		if (1 == sscanf(options[1], "%lf%", &temp)
				&& (options[1][strlen(options[1]) - 1] == 'E'
				|| options[1][strlen(options[1]) - 1] == 'e')) {
			sunriset->lon = temp;
		}

		eu_log_info("sunriset: lat: %f lon: %f", sunriset->lat, sunriset->lon);

		device_set_userdata(device, sunriset);
		sunriset_calculate(device);
		sunriset_calculate_next_run(device);
		return true;
	}
	return false;
}

static bool sunriset_device_check(device_t *device, condition_t *condition)
{
	sunriset_t *sr = device_get_userdata(device);

	switch (condition_get_type(condition))
	{
	case CONDITION_SUNRISE:
		return sr->rised;
	case CONDITION_SUNRISE_CIV:
		return sr->rised_civ;
	case CONDITION_SUNRISE_ASTRO:
		return sr->rised_astr;
	case CONDITION_SUNRISE_NAUT:
		return sr->rised_naut;
	case CONDITION_SUNSET:
		return !sr->rised;
	case CONDITION_SUNSET_CIV:
		return !sr->rised_civ;
	case CONDITION_SUNSET_ASTRO:
		return !sr->rised_astr;
	case CONDITION_SUNSET_NAUT:
		return !sr->rised_naut;
	}
	return false;
}

static bool sunriset_device_state(device_t *device, eu_variant_map_t *varmap)
{
	char buf[32];
	sunriset_t *sr = device_get_userdata(device);

	eu_variant_map_set_float(varmap, "latitude", sr->lat);
	eu_variant_map_set_float(varmap, "longitude", sr->lon);

	sprintf(buf, "%2.2d:%2.2d", TMOD(HOURS(sr->rise_astr)), MINUTES(sr->rise_astr));
	eu_variant_map_set_char(varmap, "sunrise astronomical", buf);
	sprintf(buf, "%2.2d:%2.2d", TMOD(HOURS(sr->rise_naut)), MINUTES(sr->rise_naut));
	eu_variant_map_set_char(varmap, "sunrise nautical", buf);
	sprintf(buf, "%2.2d:%2.2d", TMOD(HOURS(sr->rise_civ)), MINUTES(sr->rise_civ));
	eu_variant_map_set_char(varmap, "sunrise civilian", buf);
	sprintf(buf, "%2.2d:%2.2d", TMOD(HOURS(sr->rise)), MINUTES(sr->rise));
	eu_variant_map_set_char(varmap, "sunrise", buf);
	sprintf(buf, "%2.2d:%2.2d", TMOD(HOURS(sr->set)), MINUTES(sr->set));
	eu_variant_map_set_char(varmap, "sunset", buf);
	sprintf(buf, "%2.2d:%2.2d", TMOD(HOURS(sr->set_civ)), MINUTES(sr->set_civ));
	eu_variant_map_set_char(varmap, "sunset civilian", buf);
	sprintf(buf, "%2.2d:%2.2d", TMOD(HOURS(sr->set_naut)), MINUTES(sr->set_naut));
	eu_variant_map_set_char(varmap, "sunset nautical", buf);
	sprintf(buf, "%2.2d:%2.2d", TMOD(HOURS(sr->set_astr)), MINUTES(sr->set_astr));
	eu_variant_map_set_char(varmap, "sunset astronomical", buf);

	return true;
}

static void sunriset_device_cleanup(device_t *device)
{
	sunriset_t *sr = device_get_userdata(device);
	for (int se = 0; se < SUNRISET_EVENT_COUNT; se++) {
		eu_event_timer_t *event = sr->sunriset_timers[se];
		if (event) {
			free(eu_event_timer_get_userdata(event));
			eu_event_timer_destroy(event);
		}
	}
	eu_event_timer_destroy(sr->midnight);
	free(sr);
}

static device_type_info_t sunriset_info = {
	.name = "SUNRISET",
	.events = EVENT_SUNRISE | EVENT_SUNSET |
			  EVENT_SUNRISE_CIV | EVENT_SUNSET_CIV |
			  EVENT_SUNRISE_NAUT | EVENT_SUNSET_NAUT |
			  EVENT_SUNRISE_ASTRO | EVENT_SUNSET_ASTRO,
	.actions = 0,
	.conditions =
		CONDITION_SUNRISE | CONDITION_SUNSET |
		CONDITION_SUNRISE_CIV | CONDITION_SUNSET_CIV |
		CONDITION_SUNRISE_NAUT | CONDITION_SUNSET_NAUT |
		CONDITION_SUNRISE_ASTRO | CONDITION_SUNSET_ASTRO,
	.check_cb = sunriset_device_check,
	.parse_cb = sunriset_parser,
	.exec_cb = NULL,
	.state_cb = sunriset_device_state,
	.cleanup_cb = sunriset_device_cleanup,
};

bool sunriset_technology_init(void)
{
	device_type_register(&sunriset_info);

	eu_log_info("Succesfully initialized: Sunriset!");
	return true;
}
