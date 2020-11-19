/*
 * domctl.c
 *
 *  Created on: Nov 26, 2017
 *      Author: jnevens
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <eu/log.h>
#include <eu/socket.h>

void usage(void)
{
	fprintf(stderr, "domctl [command]\n");
	exit(1);
}

int main(int argc, char *argv[])
{
	char buf[1024];
	int rv = 0;

	if (argc < 1) {
		usage();
	}

	eu_log_init("domctl");
	eu_log_set_print_level(EU_LOG_NOTICE);

	eu_socket_t *socket = eu_socket_create_unix();
	if (!socket) {
		eu_log_fatal("Failed to create socket!");
	}
	if(!eu_socket_connect_unix(socket, "/var/run/domotica-engine.sock")) {
		eu_log_fatal("Failed to connect to deamon");
	}
	eu_socket_write(socket, argv[1], strlen(argv[1]) + 1);

	while((rv = eu_socket_read(socket, buf, sizeof(buf))) > 0) {
		eu_socket_set_blocking(socket, false);
		printf("%.*s", rv, buf);
	}
	eu_socket_destroy(socket);

	return 0;
}
