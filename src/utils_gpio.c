/*
 * utils_gpio.c
 *
 *  Created on: Dec 28, 2016
 *      Author: jnevens
 */
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include <bus/log.h>

#include "utils_gpio.h"

int gpio_export(int pin)
{
	char buffer[64];
	ssize_t bytes_written;
	int fd;

	fd = open("/sys/class/gpio/export", O_WRONLY);
	if (-1 == fd) {
		fprintf(stderr, "Failed to open export for writing!\n");
		return(-1);
	}

	bytes_written = snprintf(buffer, sizeof(buffer), "%d", pin);
	write(fd, buffer, bytes_written);
	close(fd);
	log_info("gpio exported: %d", pin);
	return(0);
}

int gpio_unexport(int pin)
{
	char buffer[64];
	ssize_t bytes_written;
	int fd;

	fd = open("/sys/class/gpio/unexport", O_WRONLY);
	if (-1 == fd) {
		fprintf(stderr, "Failed to open unexport for writing!\n");
		return(-1);
	}

	bytes_written = snprintf(buffer, sizeof(buffer), "%d", pin);
	write(fd, buffer, bytes_written);
	close(fd);
	return(0);
}

int gpio_fd_open(int pin)
{
	char path[64];
	int fd;

	snprintf(path, sizeof(path), "/sys/class/gpio/gpio%d/value", pin);

	fd = open(path, O_RDONLY | O_NONBLOCK);
	if (-1 == fd) {
		fprintf(stderr, "Failed to open gpio value for polling!\n");
		return (-1);
	}

	log_info("gpio open: fd: %d", fd);

	return fd;
}

void gpio_fd_close(int fd)
{
	close(fd);
}

int gpio_set_direction(int pin, gpio_direction_e dir)
{
	static const char s_directions_str[]  = "in\0out";

	char path[64];
	int fd;

	snprintf(path, sizeof(path), "/sys/class/gpio/gpio%d/direction", pin);
	fd = open(path, O_WRONLY);
	if (-1 == fd) {
		fprintf(stderr, "Failed to open gpio direction for writing!\n");
		return(-1);
	}

	if (-1 == write(fd, &s_directions_str[GPIO_DIR_IN == dir ? 0 : 3], GPIO_DIR_IN == dir ? 2 : 3)) {
		fprintf(stderr, "Failed to set direction!\n");
		return(-1);
	}

	close(fd);
	return(0);
}

static const char *gpio_edges[] = {
		"none",
		"rising",
		"falling",
		"both",
};

int gpio_set_edge(int pin, gpio_edge_e edge)
{
	char path[64];
	int fd;

	snprintf(path, sizeof(path), "/sys/class/gpio/gpio%d/edge", pin);
	fd = open(path, O_WRONLY);
	if (-1 == fd) {
		fprintf(stderr, "Failed to open gpio edge for writing!\n");
		return(-1);
	}

	if (-1 == write(fd, gpio_edges[edge], strlen(gpio_edges[edge]) + 1)) {
		fprintf(stderr, "Failed to set direction!\n");
		return(-1);
	}

	close(fd);
	return(0);
}

int gpio_get_value(int pin)
{
	char path[64];
	char value_str[3];
	int fd;

	snprintf(path, sizeof(path), "/sys/class/gpio/gpio%d/value", pin);
	fd = open(path, O_RDONLY);
	if (-1 == fd) {
		fprintf(stderr, "Failed to open gpio value for reading!\n");
		return(-1);
	}

	if (-1 == read(fd, value_str, 3)) {
		fprintf(stderr, "Failed to read value!\n");
		return(-1);
	}

	close(fd);

	return(atoi(value_str));
}

int gpio_set_value(int pin, gpio_value_e value)
{
	static const char s_values_str[] = "01";

	char path[64];
	int fd;

	snprintf(path, sizeof(path), "/sys/class/gpio/gpio%d/value", pin);
	fd = open(path, O_WRONLY);
	if (-1 == fd) {
		fprintf(stderr, "Failed to open gpio value for writing!\n");
		return(-1);
	}

	if (1 != write(fd, &s_values_str[GPIO_VAL_LOW == value ? 0 : 1], 1)) {
		fprintf(stderr, "Failed to write value!\n");
		return(-1);
	}

	close(fd);
	return(0);
}
