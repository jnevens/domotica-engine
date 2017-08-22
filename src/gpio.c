/*
 * gpio.c
 *
 *  Created on: Dec 27, 2016
 *      Author: jnevens
 */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <error.h>
#include <errno.h>

#include <eu/log.h>
#include <eu/event.h>
#include <eu/timer.h>

#include "event.h"
#include "action.h"
#include "device.h"
#include "device_list.h"
#include "utils_gpio.h"
#include "utils_time.h"
#include "gpio.h"

typedef struct {
	int	pin;
	bool value;
	int fd;
	uint64_t last_time_ms;
	uint64_t press_time;
	eu_event_timer_t *timer;
} gpio_t;

bool gpio_input_dim_press_timer_cb(void *arg)
{
	device_t *device = arg;

	device_trigger_event(device, EVENT_DIM);

	return eu_event_timer_continue;
}

bool gpio_input_short_press_timer_cb(void *arg)
{
	device_t *device = arg;
	gpio_t *gpio_input = device_get_userdata(device);


	device_trigger_event(device, EVENT_DIM);

	gpio_input->timer = NULL;
	gpio_input->timer = eu_event_timer_create(PRESS_DIM_INTERVAL_TIME_MS, gpio_input_dim_press_timer_cb, device);

	return eu_event_timer_stop;
}

void gpio_input_handle_edge(device_t *device)
{
	gpio_t *input_gpio = device_get_userdata(device);

	if (input_gpio->value == 1) {
		input_gpio->press_time = get_current_time_ms();

		device_trigger_event(device, EVENT_PRESS);

		if (input_gpio->timer) {
			eu_event_timer_destroy(input_gpio->timer);
		}
		input_gpio->timer = eu_event_timer_create(SHORT_PRESS_MAX_TIME_MS, gpio_input_short_press_timer_cb, device);
	} else {
		uint64_t press_time = input_gpio->press_time;
		uint64_t release_time = get_current_time_ms();
		uint64_t press_duration = release_time - press_time;
		eu_log_debug("press_duration: %dms", press_duration);

		device_trigger_event(device, EVENT_RELEASE);

		if (input_gpio->timer) {
			eu_event_timer_destroy(input_gpio->timer);
		}
		input_gpio->timer = NULL;

		if (press_duration <= SHORT_PRESS_MAX_TIME_MS) {
			device_trigger_event(device, EVENT_SHORT_PRESS);
		} else {
			device_trigger_event(device, EVENT_LONG_PRESS);
		}
	}
}

bool gpio_input_find_by_fd_cb(device_t *device, void *arg)
{
	if (strcmp(device_get_type(device), "GPIO") == 0) {
		int fd = (int)arg;
		gpio_t *input_gpio = device_get_userdata(device);
		if (input_gpio->fd == fd) {
			return true;
		}
	}
	return false;
}

void gpio_input_edge_detected_cb(int fd, short int revents, void *arg)
{
	uint8_t buf[1024] = {};
	lseek(fd, 0, SEEK_SET);
	read(fd, buf, sizeof(buf));
	int edge = (buf[0] == '1') ? 1 : 0;

	device_t *device = device_list_find(gpio_input_find_by_fd_cb, (void *)fd);
	if (device) {
		gpio_t *input_gpio = device_get_userdata(device);
		if (input_gpio->value != edge && ((get_current_time_ms() - input_gpio->last_time_ms) > 50)) {
			input_gpio->value = edge;
			input_gpio->last_time_ms = get_current_time_ms();

			gpio_input_handle_edge(device);
		}
	}
}

static bool gpio_input_parser(device_t *device, char *options[])
{
	if(options[1] != NULL) {
		gpio_t *gpio_input = calloc(1, sizeof(gpio_t));
		gpio_input->pin = strtoll(options[1], NULL, 10);
		device_set_userdata(device, gpio_input);
		gpio_export(gpio_input->pin);
		gpio_set_direction(gpio_input->pin, GPIO_DIR_IN);
		gpio_set_edge(gpio_input->pin, GPIO_EDGE_BOTH);
		gpio_input->value = gpio_get_value(gpio_input->pin);

		gpio_input->fd = gpio_fd_open(gpio_input->pin);
		eu_event_add(gpio_input->fd, POLLPRI, gpio_input_edge_detected_cb, NULL,device);

		eu_log_info("GPIO input created (name: %s, pin: %d)", device_get_name(device), gpio_input->pin);
		return true;
	}
	return false;
}

static bool gpio_input_exec(device_t *device, action_t *action)
{
	return false;
}

static bool gpio_output_parser(device_t *device, char *options[])
{
	if(options[1] != NULL) {
		gpio_t *gpio_output = calloc(1, sizeof(gpio_t));
		gpio_output->pin = strtoll(options[1], NULL, 10);
		device_set_userdata(device, gpio_output);
		gpio_export(gpio_output->pin);
		gpio_set_direction(gpio_output->pin, GPIO_DIR_OUT);
		gpio_set_value(gpio_output->pin, GPIO_VAL_LOW);
		gpio_output->value = GPIO_VAL_LOW;
		eu_log_info("GPIO output created (name: %s, pin: %d)", device_get_name(device), gpio_output->pin);
		return true;
	}
	return false;
}

static bool gpio_output_exec(device_t *device, action_t *action)
{
	gpio_t *output_gpio = device_get_userdata(device);
	action_print(action);

	switch (action_get_type(action)) {
	case ACTION_SET:
		gpio_set_value(output_gpio->pin, GPIO_VAL_HIGH);
		output_gpio->value = GPIO_VAL_HIGH;
		break;
	case ACTION_UNSET:
		gpio_set_value(output_gpio->pin, GPIO_VAL_LOW);
		output_gpio->value = GPIO_VAL_LOW;
		break;
	case ACTION_TOGGLE:
		output_gpio->value = !output_gpio->value;
		gpio_set_value(output_gpio->pin, output_gpio->value);
		break;
	}
	eu_log_debug("gpio %s(%d) set to: %d", device_get_name(device), output_gpio->pin, output_gpio->value);

	return false;
}

static device_type_info_t gpi_info = {
	.name = "GPI",
	.events = EVENT_PRESS | EVENT_RELEASE | EVENT_SHORT_PRESS | EVENT_LONG_PRESS | EVENT_DIM,
	.actions = 0,
	.conditions = 0,
	.check_cb = NULL,
	.parse_cb = gpio_input_parser,
	.exec_cb = gpio_input_exec,
};

static device_type_info_t gpo_info = {
	.name = "GPO",
	.events = 0,
	.actions = ACTION_SET | ACTION_UNSET | ACTION_TOGGLE,
	.conditions = 0,
	.check_cb = NULL,
	.parse_cb = gpio_output_parser,
	.exec_cb = gpio_output_exec,
};

bool gpio_technology_init(void)
{
	device_type_register(&gpi_info);
	device_type_register(&gpo_info);

	eu_log_info("Succesfully initialized: gpio!");
	return true;
}
