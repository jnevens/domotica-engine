/*
 * eltako.c
 *
 *  Created on: Dec 21, 2016
 *      Author: jnevens
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <bus/log.h>
#include <bus/event.h>
#include <libvsb/client.h>
#include <libeltako/frame.h>
#include <libeltako/message.h>

#include "input.h"
#include "input_list.h"
#include "eltako.h"
#include "eltako_fts14em.h"
#include "eltako_fud14.h"

static vsb_client_t *vsb_client;

bool eltako_send(eltako_message_t *msg)
{
	eltako_frame_t *frame = eltako_message_to_frame(msg);
	eltako_frame_print(frame);
	vsb_client_send_data(vsb_client, eltako_frame_get_data(frame), eltako_frame_get_raw_size(frame));
}

static bool input_list_find_cb(input_t *input, void *arg) {
	eltako_message_t *msg = arg;
	if (strcmp(input_get_type(input), "FTS14EM") == 0) {
		uint32_t address = eltako_fts14em_get_address(input);
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
	input_t *input = input_list_find(input_list_find_cb, (void *)msg);
	if (input) {
		log_debug("Input triggered: %s (%s)", input_get_name(input), input_get_type(input));
		if (strcmp(input_get_type(input), "FTS14EM") == 0) {
			eltako_fts14em_incoming_data(msg, input);
		}
	} else {
		log_debug("ignore eltako message from 0x%x", eltako_message_get_address(msg));
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
	log_err("Connection lost with server!\n");
}

static bool eltako_connect_with_eltakod(void)
{
	vsb_client = vsb_client_init("/var/run/eltako.socket", "domotica-engine");
	int eltako_fd = vsb_client_get_fd(vsb_client);
	vsb_client_register_incoming_data_cb(vsb_client, incoming_data, NULL);
	event_add(eltako_fd, POLLIN, handle_incoming_event, NULL, vsb_client);
	vsb_client_register_disconnect_cb(vsb_client, handle_connection_disconnect, NULL);
}

bool eltako_technology_init(void)
{
	if(eltako_connect_with_eltakod()) {
		log_err("Failed to connect to eltakod!");
		return false;
	}

	eltako_fts14em_init();
	eltako_fud14_init();
	//eltako_fsr14-2x_init();
	//eltako_fsr14-4x_init();
	//eltako_fsb14_init();

	log_info("Succesfully initialized: eltako!");
	return true;
}
