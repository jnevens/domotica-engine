/*
 * eltako.c
 *
 *  Created on: Dec 21, 2016
 *      Author: jnevens
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <eu/log.h>
#include <eu/event.h>
#include <libvsb/client.h>
#include <libeltako/frame.h>
#include <libeltako/message.h>

#include "device.h"
#include "device_list.h"
#include "eltako.h"
#include "eltako_fsr14.h"
#include "eltako_fsb14.h"
#include "eltako_fts14em.h"
#include "eltako_fud14.h"

static vsb_client_t *vsb_client;

bool eltako_send(eltako_message_t *msg)
{
	if (msg == NULL) {
		return false;
	}
	eltako_frame_t *frame = eltako_message_to_frame(msg);
	eltako_frame_print(frame);
	vsb_client_send_data(vsb_client, eltako_frame_get_data(frame), eltako_frame_get_raw_size(frame));
	eltako_message_destroy(msg);
	eltako_frame_destroy(frame);
}

static bool device_list_find_cb(device_t *device, void *arg) {
	eltako_message_t *msg = arg;
	if (strcmp(device_get_type(device), "FTS14EM") == 0) {
		uint32_t address = eltako_fts14em_get_address(device);
		if (address == eltako_message_get_address(msg)) {
			return true;
		}
	}
	return false;
}

static void incoming_data(void *data, size_t len, void *arg)
{
	eltako_frame_t *frame = eltako_frame_create_from_buffer(data, len);
	eltako_message_t *msg = eltako_message_create_from_frame(frame);
	//eltako_frame_print(frame);

	eltako_message_print(msg);
	device_t *device = device_list_find(device_list_find_cb, (void *)msg);
	if (device) {
		eu_log_debug("Input triggered: %s (%s)", device_get_name(device), device_get_type(device));
		if (strcmp(device_get_type(device), "FTS14EM") == 0) {
			eltako_fts14em_incoming_data(msg, device);
		}
	} else {
		eu_log_debug("ignore eltako message from 0x%x", eltako_message_get_address(msg));
	}

	eltako_message_destroy(msg);
	eltako_frame_destroy(frame);
}

static void handle_incoming_event(int fd, short int revents, void *arg)
{
	vsb_client_t *vsb_client = (vsb_client_t *)arg;
	vsb_client_handle_incoming_event(vsb_client);
}

void handle_connection_disconnect(void *arg)
{
	eu_log_err("Connection lost with server!\n");
}

static bool eltako_connect_with_eltakod(void)
{
	vsb_client = vsb_client_init("/var/run/eltako.socket", "domotica-engine");
	if (vsb_client == NULL) {
		eu_log_err("Failed connection to eltakod! Cannot open /var/run/eltako.socket");
		return false;
	}
	int eltako_fd = vsb_client_get_fd(vsb_client);
	vsb_client_register_incoming_data_cb(vsb_client, incoming_data, NULL);
	eu_event_add(eltako_fd, POLLIN, handle_incoming_event, NULL, vsb_client);
	vsb_client_register_disconnect_cb(vsb_client, handle_connection_disconnect, NULL);

	eu_log_info("Eltako registered (fd = %d)", eltako_fd);

	return true;
}

bool eltako_technology_init(void)
{
	if(!eltako_connect_with_eltakod()) {
		eu_log_err("Failed to connect to eltakod!");
		return false;
	}

	eltako_fts14em_init();
	eltako_fud14_init();
	eltako_fsr14_init();
	eltako_fsb14_init();

	eu_log_info("Succesfully initialized: eltako!");
	return true;
}
