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
	char buf[256];

	if (argc < 1) {
		usage();
	}

	eu_socket_t *socket = eu_socket_create_unix();
	if (!socket) {
		eu_log_fatal("Failed to create socket!");
	}
	if(!eu_socket_connect_unix(socket, "/var/run/domotica-engine.sock")) {
		eu_log_fatal("Failed to connect to deamon");
	}
	eu_socket_write(socket, argv[1], strlen(argv[1]) + 1);
	int rv = eu_socket_read(socket, buf, sizeof(buf));
	if (rv > 0) {
		printf("response:\n%s\n", buf);
	}
	eu_socket_destroy(socket);

	return 0;
}
