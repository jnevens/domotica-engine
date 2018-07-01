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

#include <curl/curl.h>
#include <eu/log.h>

#include "device.h"

typedef struct {
	struct in_addr ip;
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

const bool musiccast_request(const char *url, char **response)
{
	CURL *curl;
	CURLcode res;
	bool rv = false;
	musiccast_request_download_arg download_arg = {
			.response = response,
			.len = 0,
	};

	curl = curl_easy_init();
	if (!curl) {
		return false;
	}

	curl_easy_setopt(curl, CURLOPT_URL, url);
	// curl_easy_setopt(curl, CURLOPT_POSTFIELDS, "name=daniel&project=curl");
	curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1);
	curl_easy_setopt(curl, CURLOPT_WRITEDATA, &download_arg);
	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, musiccast_request_download_cb);
	/* Perform the request, res will get the return code */
	res = curl_easy_perform(curl);
	/* Check for errors */
	if (res != CURLE_OK) {
		eu_log_err("curl_easy_perform() failed: %s", curl_easy_strerror(res));
		goto cleanup;
	}

	rv = true;
cleanup:
	curl_easy_cleanup(curl);
	return rv;
}

static void json_parse(const char *json)
{
	json_object *jobj = json_tokener_parse(json);

	if (!jobj) {
		eu_log_debug("no json: %s", json);
		return;
	}

	json_object_object_foreach(jobj, key, val) {
		int val_type = json_object_get_type(val);
		switch (val_type) {
		case json_type_null:
			eu_log_debug("val is NULL");
			break;
		case json_type_boolean:
			eu_log_debug("%s = %s", key, (json_object_get_boolean(val)) ? "true" : "false");
			break;
		case json_type_double:
			eu_log_debug("%s = %.2lf", key, json_object_get_double(val));
			break;
		case json_type_int:
			eu_log_debug("%s = %d", key, json_object_get_int(val));
			break;
		case json_type_string:
			eu_log_debug("%s = %s", key, (char *)json_object_get_string(val));
			break;
		case json_type_object:
			eu_log_debug("val is an object");
			break;
		case json_type_array:
			eu_log_debug("val is an array");
			break;
		}
	}
	json_object_put(jobj);
}

static bool musiccast_get_device_info(device_musiccast_t *mc)
{
	bool rv;
	char *url = NULL;
	char *response = NULL;
	asprintf(&url, "http://%s/YamahaExtendedControl/v1/system/getDeviceInfo", inet_ntoa(mc->ip));
	rv = musiccast_request(url, &response);
	eu_log_debug("response:\n%s", response);

	json_parse(response);

	free(url);
	free(response);
	return rv;
}

static bool musiccast_get_features(device_musiccast_t *mc)
{
	bool rv;
	char *url = NULL;
	char *response = NULL;
	asprintf(&url, "http://%s/YamahaExtendedControl/v1/system/getFeatures", inet_ntoa(mc->ip));
	rv = musiccast_request(url, &response);
	eu_log_debug("response:\n%s", response);

	json_parse(response);

	free(url);
	free(response);
	return rv;
}

static bool musiccast_get_network_status(device_musiccast_t *mc)
{
	bool rv;
	char *url = NULL;
	char *response = NULL;
	asprintf(&url, "http://%s/YamahaExtendedControl/v1/system/getNetworkStatus", inet_ntoa(mc->ip));
	rv = musiccast_request(url, &response);
	eu_log_debug("response:\n%s", response);

	json_parse(response);

	free(url);
	free(response);
	return rv;
}

static bool musiccast_get_func_status(device_musiccast_t *mc)
{
	bool rv;
	char *url = NULL;
	char *response = NULL;
	asprintf(&url, "http://%s/YamahaExtendedControl/v1/system/getFuncStatus", inet_ntoa(mc->ip));
	rv = musiccast_request(url, &response);
	eu_log_debug("response:\n%s", response);

	json_parse(response);

	free(url);
	free(response);
	return rv;
}

static bool musiccast_get_location_info(device_musiccast_t *mc)
{
	bool rv;
	char *url = NULL;
	char *response = NULL;
	asprintf(&url, "http://%s/YamahaExtendedControl/v1/system/getLocationInfo", inet_ntoa(mc->ip));
	rv = musiccast_request(url, &response);
	eu_log_debug("response:\n%s", response);

	json_parse(response);

	free(url);
	free(response);
	return rv;
}

static bool musiccast_get_status(device_musiccast_t *mc)
{
	bool rv;
	char *url = NULL;
	char *response = NULL;
	asprintf(&url, "http://%s/YamahaExtendedControl/v1/system/getStatus", inet_ntoa(mc->ip));
	rv = musiccast_request(url, &response);
	eu_log_debug("response:\n%s", response);

	json_parse(response);

	free(url);
	free(response);
	return rv;
}

static bool musiccast_get_sound_program_list(device_musiccast_t *mc)
{
	bool rv;
	char *url = NULL;
	char *response = NULL;
	asprintf(&url, "http://%s/YamahaExtendedControl/v1/system/getSoundProgramList", inet_ntoa(mc->ip));
	rv = musiccast_request(url, &response);
	eu_log_debug("response:\n%s", response);

	json_parse(response);

	free(url);
	free(response);
	return rv;
}

static bool musiccast_device_parser(device_t *device, char *options[])
{
	if (options[1] != NULL) {
		struct in_addr ip;

		if (inet_aton(options[1], &ip) == 0) {
			fprintf(stderr, "Invalid address\n");
			eu_log_err("Failed to convert ip address: %s : %m", options[1]);
			return false;
		}

		device_musiccast_t *mc = calloc(1, sizeof(device_musiccast_t));
		mc->ip = ip;
		device_set_userdata(device, mc);
		eu_log_debug("Yamaha MusicCast device created (name: %s, IP: %s)", device_get_name(device), inet_ntoa(mc->ip));

		musiccast_get_device_info(mc);
		musiccast_get_features(mc);
		musiccast_get_network_status(mc);
		musiccast_get_func_status(mc);
		musiccast_get_location_info(mc);
		musiccast_get_status(mc);
		musiccast_get_sound_program_list(mc);
		return true;
	}
	return false;
}

void musiccast_device_cleanup(device_t *device)
{
	device_musiccast_t *mc = device_get_userdata(device);
	free(mc);
}

static device_type_info_t musiccast_info = {
	.name = "MUSICCAST",
	.events = 0,
	.actions = 0,
	.conditions = 0,
	.check_cb = NULL,
	.parse_cb = musiccast_device_parser,
	.exec_cb = NULL,
	.state_cb = NULL,
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

