/*
 * remote.c
 *
 *  Created on: Nov 25, 2017
 *      Author: jnevens
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <stdint.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <error.h>

#include <eu/log.h>
#include <eu/socket.h>
#include <eu/event.h>
#include <eu/variant.h>

#include "utils_string.h"
#include "device.h"
#include "device_list.h"
#include "device_logs.h"
#include "action.h"
#include "line_parser.h"

static eu_event_t *event_tcp = NULL;
static eu_event_t *event_unix = NULL;

static bool handle_command(char *buf, eu_variant_map_t **map, char **json)
{
	bool rv = false;
	line_t *ln = line_parse(buf);

	if (ln) {
		switch (ln->statement) {
		case STATEMENT_DO: {
			device_t *device = device_list_find_by_name(ln->name);
			if (!device) {
				eu_log_err("Cannot find device: %s", ln->name);
				goto end;
			}
			action_type_e action_type = action_type_from_char(ln->options[0]);
			if (action_type == -1) {
				eu_log_err("Action '%s' does not exist!", ln->options[0]);
				goto end;
			}
			action_t *action = action_create(device, action_type, ln->options);
			if (action == NULL) {
				eu_log_err("Failed to create action!");
				goto end;
			}
			if(action_execute(action, NULL)) {
				rv = true;
			}
			action_destroy(action);
			break;
		}
		case STATEMENT_STATE: {
			device_t *device = device_list_find_by_name(ln->name);
			if (!device) {
				eu_log_err("Cannot find device: %s", ln->name);
				goto end;
			}
			*map = device_state(device);
			if (map == NULL) {
				eu_log_err("Cannot get state of device: %s", ln->name);
				goto end;
			} else {
				rv = true;
			}
			break;
		}
		case STATEMENT_LOGS: {
			device_t *device = device_list_find_by_name(ln->name);
			if (!device) {
				eu_log_err("Cannot find device: %s", ln->name);
				goto end;
			}
			*json = device_logs_get_json(device, 3600*24);
			if (*json == NULL) {
				eu_log_err("Cannot get logs of device: %s", ln->name);
				goto end;
			} else {
				rv = true;
			}
			break;
		}
		default:
			eu_log_err("unsupported type of command!");
			break;
		}
	} else {
		eu_log_err("failed to parse line!");
	}
	end: line_destroy(ln);
	return rv;
}

static void remote_connection_close(eu_socket_t *sock)
{
	eu_event_t *event = eu_socket_get_userdata(sock);
	eu_event_destroy(event);
	eu_socket_destroy(sock);
}

static void handle_incoming_conn_event(int fd, short int revents, void *arg)
{
	char buf[128];
	eu_socket_t *conn_sock = arg;
	eu_variant_map_t *map = NULL;
	char *json = NULL;

	memset(buf, '\0', sizeof(buf));

	int rv = eu_socket_read(conn_sock, buf, sizeof(buf) - 1);
	if (rv > 0) {
		char *response = NULL;
		if (handle_command(buf, &map, &json) == true) {
			if (map) {
				response = strdup("{\n");
				int map_count = eu_variant_map_count(map);
				int count = 1;
				eu_variant_map_for_each_pair(pair, map) {
					char *tmp = NULL;
					const char *key = eu_variant_map_pair_get_key(pair);
					eu_variant_t *var = eu_variant_map_pair_get_val(pair);

					if (eu_variant_type(var) == EU_VARIANT_TYPE_INT32) {
						asprintf(&tmp, "\t\"%s\" : \"%d\"", key, eu_variant_int32(var));
					} else if (eu_variant_type(var) == EU_VARIANT_TYPE_CHAR) {
						asprintf(&tmp, "\t\"%s\" : \"%s\"", key, eu_variant_da_char(var));
					} else if (eu_variant_type(var) == EU_VARIANT_TYPE_FLOAT) {
						asprintf(&tmp, "\t\"%s\" : \"%f\"", key, eu_variant_float(var));
					} else if (eu_variant_type(var) == EU_VARIANT_TYPE_DOUBLE) {
						asprintf(&tmp, "\t\"%s\" : \"%lf\"", key, eu_variant_double(var));
					} else if (eu_variant_type(var) == EU_VARIANT_TYPE_BOOL) {
						asprintf(&tmp, "\t\"%s\" : \"%d\"", key, eu_variant_bool(var));
					} else if (eu_variant_type(var) == EU_VARIANT_TYPE_MAP) {
						asprintf(&tmp, "\t\"%s\" : \"%s\"", key, "MAP-TODO");
					}

					strappend(&response, tmp);
					if (count < map_count)
						strappend(&response, ",");
					strappend(&response, "\n");
					count++;
					free(tmp);
				}
				strappend(&response, "}\n");
			} else if (json) {
				response = strdup(json);
			} else {
				response = strdup("OK");
			}
		} else {
			response = strdup("NOK");
		}

		if (response) {
			if (eu_socket_write(conn_sock, response, strlen(response) + 1) <=0 ) {
				eu_log_err("send response [%d]: '%s' ERR %d:%s", strlen(response) + 1, response, errno, strerror(errno));
			}
		}
		free(response);
	} else {
		remote_connection_close(conn_sock);
	}

	if (map)
		eu_variant_map_destroy(map);
	if (json)
		free(json);
}

static void handle_incoming_conn_error(int fd, short int revents, void *arg)
{
	eu_socket_t *conn_sock = arg;
	eu_log_info("remote, incoming conn event error, remove!");

	remote_connection_close(conn_sock);
}

static void handle_incoming_event(int fd, short int revents, void *arg)
{
	eu_socket_t *socket = arg;

	eu_socket_t *new = eu_socket_accept(socket);
	eu_socket_set_blocking(new, 0);
	eu_event_t *event = eu_event_add(eu_socket_get_fd(new), POLLIN, handle_incoming_conn_event,
			handle_incoming_conn_error, new);
	eu_socket_set_userdata(new, event);
}

static void handle_incoming_error(int fd, short int revents, void *arg)
{
	eu_log_info("remote, incoming event error");
}

static eu_event_t *remote_connection_init_unix(const char *unix_path)
{
	eu_socket_t *socket = eu_socket_create_unix();
	if (socket == NULL) {
		eu_log_err("Failed to create socket: %s", strerror(errno));
		return NULL;
	}

	if (!eu_socket_bind_unix(socket, unix_path)) {
		eu_log_err("Failed to bind socket: %s", strerror(errno));
		eu_socket_destroy(socket);
		return NULL;
	}

	eu_socket_listen(socket, 10);

	int fd = eu_socket_get_fd(socket);
	eu_event_t *event = eu_event_add(fd, POLLIN, handle_incoming_event, handle_incoming_error, socket);
	if (event == NULL) {
		eu_socket_destroy(socket);
	}

	return event;
}

static eu_event_t *remote_connection_init_tcp(uint16_t port)
{
	eu_socket_t *socket = eu_socket_create_tcp();
	if (socket == NULL) {
		eu_log_err("Failed to create socket: %s", strerror(errno));
		return NULL;
	}

	if (!eu_socket_bind_tcp(socket, port)) {
		eu_log_err("Failed to bind socket: %s", strerror(errno));
		eu_socket_destroy(socket);
		return NULL;
	}

	eu_socket_listen(socket, 10);

	int fd = eu_socket_get_fd(socket);
	eu_event_t *event = eu_event_add(fd, POLLIN, handle_incoming_event, handle_incoming_error, socket);
	if (event == NULL) {
		eu_socket_destroy(socket);
	}

	return event;
}

static void remote_connection_cleanup_tcp(eu_event_t *event)
{
	if (event) {
		eu_socket_t *socket = eu_event_get_userdata(event);
		eu_socket_destroy(socket);
		eu_event_destroy(event);
	}
}

static void remote_connection_cleanup_unix(eu_event_t *event)
{
	if (event) {
		eu_socket_t *socket = eu_event_get_userdata(event);
		eu_socket_destroy(socket);
		eu_event_destroy(event);
	}
}

void remote_connection_cleanup(void)
{
	remote_connection_cleanup_tcp(event_tcp);
	remote_connection_cleanup_unix(event_unix);

	event_tcp = NULL;
	event_unix = NULL;
}

bool remote_connection_init(void)
{
	event_unix = remote_connection_init_unix("/var/run/domotica-engine.sock");
	event_tcp = remote_connection_init_tcp(17922);

	if (event_unix == NULL || event_tcp == NULL) {
		remote_connection_cleanup();
		return false;
	}

	return true;
}
