/*
 * musiccast.c
 *
 *  Created on: May 26, 2018
 *      Author: jnevens
 */

#include <stdbool.h>
#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <musiccast.h>
#include <eu/log.h>

#include "device.h"
#include "yamaha_mc.h"

typedef struct {
	struct in_addr ip;
	char *zone;
	bool power;
	int32_t volume;
	int32_t volume_max;
	int32_t volume_step;
	musiccast_conn_t *mcc;
} device_musiccast_t;

typedef struct {
	char **response;
	size_t len;
} musiccast_request_download_arg;

static size_t musiccast_request_download_cb(void *ptr, size_t size, size_t nmemb, FILE *stream)
{
	eu_log_debug("data received: %uB", size * nmemb);
	musiccast_request_download_arg *arg = (musiccast_request_download_arg *)stream;
	size_t new_len = arg->len + size * nmemb;
	*arg->response = realloc(*arg->response, new_len + 1);
	memcpy(&((*arg->response)[arg->len]), ptr, size * nmemb);
	(*arg->response)[new_len] = '\0';
	arg->len = new_len;
	return size * nmemb;
}

static bool musiccast_device_parser(device_t *device, char *options[])
{
	if (options[1] != NULL) {
		struct in_addr ip;
		char *zone = "main";
		musiccast_conn_t *mcc = NULL;

		if (inet_aton(options[1], &ip) == 0) {
			fprintf(stderr, "Invalid address\n");
			eu_log_err("Failed to convert ip address: %s : %m", options[1]);
			return false;
		}

		if (options[2] != NULL) {
			zone = strdup(options[2]);
		}

		if ((mcc = musiccast_init(inet_ntoa(ip))) == NULL) {
			eu_log_err("Failed to connect with MUSICCAST device: %s", ip);
			return false;
		}

		device_musiccast_t *dmc = calloc(1, sizeof(device_musiccast_t));
		dmc->ip = ip;
		dmc->zone = zone;
		dmc->mcc = mcc;
		dmc->volume = 0;
		dmc->volume_max = 10;
		dmc->volume_step = 1;
		device_set_userdata(device, dmc);
		eu_log_debug("Yamaha MusicCast device created (name: %s, IP: %s, zone: %s)",
				device_get_name(device), inet_ntoa(dmc->ip), dmc->zone);

		musiccast_zone_status(dmc->mcc, dmc->zone);

		return true;
	}
	return false;
}

static bool musiccast_device_exec(device_t *device, action_t *action, event_t *event)
{
	device_musiccast_t *dmc = device_get_userdata(device);
	action_print(action);

	switch (action_get_type(action)) {
	case ACTION_SET:
		musiccast_zone_power_set(dmc->mcc, dmc->zone, MC_POWER_STATE_ON);
		break;
	case ACTION_UNSET:
		musiccast_zone_power_set(dmc->mcc, dmc->zone, MC_POWER_STATE_STANDBY);
		break;
	case ACTION_TOGGLE:
		musiccast_zone_power_set(dmc->mcc, dmc->zone, MC_POWER_STATE_TOGGLE);
		break;
	case ACTION_UP:
		musiccast_zone_volume_set(dmc->mcc, dmc->zone, MC_ZONE_VOLUME_SET_UP, 1);
		break;
	case ACTION_DOWN:
		musiccast_zone_volume_set(dmc->mcc, dmc->zone, MC_ZONE_VOLUME_SET_DOWN, 1);
		break;
	}

	return false;
}

static bool musiccast_device_state(device_t *device, eu_variant_map_t *state)
{
	device_musiccast_t *dmc = device_get_userdata(device);

	eu_variant_map_set_bool(state, "power", dmc->power);
	eu_variant_map_set_int32(state, "volume", dmc->volume);
	eu_variant_map_set_int32(state, "volume_max", dmc->volume_max);
	eu_variant_map_set_int32(state, "volume_step", dmc->volume_step);

	return true;
}

void musiccast_device_cleanup(device_t *device)
{
	device_musiccast_t *mc = device_get_userdata(device);
	free(mc);
}

static device_type_info_t musiccast_info = {
	.name = "MUSICCAST",
	.events = 0,
	.actions =  ACTION_SET | ACTION_UNSET | ACTION_TOGGLE | ACTION_UP | ACTION_DOWN,
	.conditions = 0,
	.check_cb = NULL,
	.parse_cb = musiccast_device_parser,
	.exec_cb = musiccast_device_exec,
	.state_cb = musiccast_device_state,
	.cleanup_cb = musiccast_device_cleanup,
};

bool musiccast_technology_init(void)
{
	device_type_register(&musiccast_info);
	return true;
}

void musiccast_technology_cleanup(void)
{

}

