/*
 * eltako.c
 *
 *  Created on: Dec 21, 2016
 *      Author: jnevens
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/stat.h>
#include <sys/types.h>

#include <eu/log.h>
#include <eu/event.h>
#include <libeltako/frame.h>
#include <libeltako/message.h>
#include <libeltako/serial.h>
#include <libeltako/frame_receiver.h>

#include "device.h"
#include "device_list.h"
#include "utils_time.h"
#include "eltako.h"
#include "eltako_fsr14.h"
#include "eltako_fsb14.h"
#include "eltako_fts14em.h"
#include "eltako_fud14.h"

static eltako_frame_receiver_t *eltako_receiver = NULL;
static int eltako_fd = -1;
static const char *scriptsdir = "/etc/domotica-engine/eltako.d";
static eu_list_t *eltako_send_queue = NULL;
static uint64_t eltako_send_last = 0;

/* function prototypes */
static bool eltako_send_real(eltako_message_t *msg);


void eltakod_execute_scripts(eltako_frame_t *frame, const char *prefix)
{
	DIR *dir;
	struct dirent *ent;

	if (access(scriptsdir, F_OK) != 0)
		return;

	if ((dir = opendir(scriptsdir)) != NULL) {
		/* print all the files and directories within directory */
		while ((ent = readdir(dir)) != NULL) {
			char path[512];
			char cmd[512 + 128];

			if (ent->d_name[0] == '.')
				continue;

			snprintf(path, sizeof(path), "%s/%s", scriptsdir, ent->d_name);
			if (access(path, X_OK) == 0) {
				eltako_message_t *msg = eltako_message_create_from_frame(frame);
				const uint8_t *data = eltako_message_get_data(msg);
				printf("Execute script: %s\n", path);

				snprintf(cmd, sizeof(cmd), "%s %s %d 0x%08X 0x%02X%02X%02X%02X %d",
						path,
						prefix,
						eltako_message_get_rorg(msg),
						eltako_message_get_address(msg),
						data[0], data[1], data[2], data[3],
						eltako_message_get_status(msg));
				system(cmd);
			}
		}
		closedir(dir);
	} else {
		/* could not open directory */
		printf("Failed to open dir: %m");
	}
}

static bool eltako_check_send_queue(void * arg)
{
	uint64_t now = get_current_time_ms();

	/* check anything in queue */
	if (eu_list_count(eltako_send_queue) == 0)
		return true;

	/* check if we are not sending too fast */
	if ((now - eltako_send_last) < (200 - 10))
		return true;

	/* retrieve last message from list */
	eu_list_node_t *node = eu_list_first(eltako_send_queue);
	eltako_message_t *msg = eu_list_node_data(node);
	eu_list_remove_node(eltako_send_queue, node);

	/* send message to eltako system */
	eltako_send_real(msg);

	return true;
}

bool eltako_send(eltako_message_t *msg)
{
	eu_list_append(eltako_send_queue, msg);

	/* try to send directly if possible */
	eltako_check_send_queue(NULL);

	return true;
}

static bool eltako_send_real(eltako_message_t *msg)
{
	if (msg == NULL) {
		return false;
	}

	eltako_frame_t *frame = eltako_message_to_frame(msg);
	eltako_frame_print(frame);
	eltako_frame_send(frame, eltako_fd);
	eltako_send_last = get_current_time_ms();

	eltakod_execute_scripts(frame, "TX");

	eltako_frame_destroy(frame);
	eltako_message_destroy(msg);
	return true;
}

static bool eltako_device_list_find_cb(device_t *device, void *arg) {
	eltako_message_t *msg = arg;
	if (strcmp(device_get_type(device), "FTS14EM") == 0) {
		uint32_t address = eltako_fts14em_get_address(device);
		if (address == eltako_message_get_address(msg)) {
			return true;
		}
	}
	return false;
}

static void incoming_eltako_msg(eltako_message_t *msg)
{
	eltako_message_print(msg);

	device_t *device = device_list_find(eltako_device_list_find_cb, (void *)msg);
	if (device) {
		eu_log_debug("Input triggered: %s (%s)", device_get_name(device), device_get_type(device));
		if (strcmp(device_get_type(device), "FTS14EM") == 0) {
			eltako_fts14em_incoming_data(msg, device);
		}
	} else {
		eu_log_debug("ignore eltako message from 0x%x", eltako_message_get_address(msg));
	}
}

static void incoming_eltako_data(int fd, short revents, void *arg)
{
	uint8_t buf[1024];
	ssize_t rval;

	bzero(buf, sizeof(buf));
	while (1) {
		if ((rval = eltako_serial_read(fd, buf, sizeof(buf))) < 0) {
			if (errno == EWOULDBLOCK) {
				break;
			}
			perror("reading stream message");
		} else if (rval > 0) { // get data
			eltako_frame_receiver_add_data(eltako_receiver, buf, (size_t) rval);

			eltako_frame_t *frame = NULL;
			while ((frame = eltako_frame_receiver_parse_buffer(eltako_receiver)) != NULL) {
				eltako_message_t *msg = eltako_message_create_from_frame(frame);
				if (msg) {
					incoming_eltako_msg(msg);
					eltako_message_destroy(msg);
				}

				eltakod_execute_scripts(frame, "RX");
				eltako_frame_destroy(frame);
			}
		}
	}
}

static bool eltako_open_serial_connection(const char *port)
{
	eltako_fd = 0;

	if ((eltako_fd = eltako_serial_port_init(port)) <= 0) {
		printf("serial port setup failed!\n");
		return -1;
	}

	eltako_serial_port_set_blocking(eltako_fd, false);
	eltako_receiver = eltako_frame_receiver_init();

	eu_event_add(eltako_fd, POLLIN, incoming_eltako_data, NULL, NULL);

	return true;
}

bool eltako_technology_init(void)
{
	if (!eltako_open_serial_connection("/dev/ttyUSB0")) {
		eu_log_err("Failed to connect to eltako series 14!");
		return false;
	}

	eltako_send_queue = eu_list_create();
	eltako_fts14em_init();
	eltako_fud14_init();
	eltako_fsr14_init();
	eltako_fsb14_init();

	eu_event_timer_create(100, eltako_check_send_queue, NULL);

	eu_log_info("Succesfully initialized: eltako!");
	return true;
}
