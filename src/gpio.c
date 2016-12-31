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

#include <bus/log.h>
#include <bus/event.h>
#include <bus/timer.h>

#include "input.h"
#include "input_list.h"
#include "output.h"
#include "utils_gpio.h"
#include "utils_time.h"
#include "gpio.h"

typedef struct {
	int	pin;
	bool value;
	int fd;
	uint64_t last_time_ms;
	uint64_t press_time;
	event_timer_t *timer;
} gpio_t;

bool gpio_input_dim_press_timer_cb(void *arg)
{
	input_t *input = arg;
	gpio_t *gpio_input = input_get_userdata(input);

	uint64_t press_time = gpio_input->press_time;
	uint64_t release_time = get_current_time_ms();
	uint64_t press_duration = release_time - press_time;

	input_trigger_event(input, EVENT_DIM);

	return event_timer_continue;
}

bool gpio_input_short_press_timer_cb(void *arg)
{
	input_t *input = arg;
	gpio_t *gpio_input = input_get_userdata(input);


	input_trigger_event(input, EVENT_DIM);

	gpio_input->timer = NULL;
	gpio_input->timer = event_timer_create(PRESS_DIM_INTERVAL_TIME_MS, gpio_input_dim_press_timer_cb, input);

	return event_timer_stop;
}

void gpio_input_handle_edge(input_t *input)
{
	gpio_t *input_gpio = input_get_userdata(input);

	if (input_gpio->value == 1) {
		input_gpio->press_time = get_current_time_ms();

		input_trigger_event(input, EVENT_PRESS);

		if (input_gpio->timer) {
			event_timer_destroy(input_gpio->timer);
		}
		input_gpio->timer = event_timer_create(SHORT_PRESS_MAX_TIME_MS, gpio_input_short_press_timer_cb, input);
	} else {
		uint64_t press_time = input_gpio->press_time;
		uint64_t release_time = get_current_time_ms();
		uint64_t press_duration = release_time - press_time;
		log_debug("press_duration: %dms", press_duration);

		input_trigger_event(input, EVENT_RELEASE);

		if (input_gpio->timer) {
			event_timer_destroy(input_gpio->timer);
		}
		input_gpio->timer = NULL;

		if (press_duration <= SHORT_PRESS_MAX_TIME_MS) {
			input_trigger_event(input, EVENT_SHORT_PRESS);
		} else {
			input_trigger_event(input, EVENT_LONG_PRESS);
		}
	}
}

bool gpio_input_find_by_fd_cb(input_t *input, void *arg)
{
	if (strcmp(input_get_type(input), "GPIO") == 0) {
		int fd = (int)arg;
		gpio_t *input_gpio = input_get_userdata(input);
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
	int rv = read(fd, buf, sizeof(buf));
	int edge = (buf[0] == '1') ? 1 : 0;
	log_info("edge detected! rv: %d edge: %d", rv, edge);

	input_t *input = input_list_find(gpio_input_find_by_fd_cb, (void *)fd);
	if (input) {
		gpio_t *input_gpio = input_get_userdata(input);
		if (input_gpio->value != edge && ((get_current_time_ms() - input_gpio->last_time_ms) > 50)) {
			input_gpio->value = edge;
			input_gpio->last_time_ms = get_current_time_ms();

			log_info("gpio event: %s, %d", input_get_name(input), edge);
			gpio_input_handle_edge(input);
		}
	}
}

static bool gpio_input_parser(input_t *input, char *options[])
{
	gpio_t *gpio_input = calloc(1, sizeof(gpio_t));
	if(options[1] != NULL) {
		gpio_input->pin = strtoll(options[1], NULL, 10);
		input_set_userdata(input, gpio_input);
		gpio_export(gpio_input->pin);
		gpio_set_direction(gpio_input->pin, GPIO_DIR_IN);
		gpio_set_edge(gpio_input->pin, GPIO_EDGE_BOTH);
		gpio_input->value = gpio_get_value(gpio_input->pin);

		gpio_input->fd = gpio_fd_open(gpio_input->pin);
		event_add(gpio_input->fd, POLLPRI, gpio_input_edge_detected_cb, NULL,input);

		log_info("GPIO input created (name: %s, pin: %d)", input_get_name(input), gpio_input->pin);
		return true;
	}
	return false;
}

static bool gpio_input_exec(input_t *input)
{
	return false;
}

static bool gpio_output_parser(output_t *output, char *options[])
{
	gpio_t *gpio_output = calloc(1, sizeof(gpio_t));
	if(options[1] != NULL) {
		gpio_output->pin = strtoll(options[1], NULL, 10);
		output_set_userdata(output, gpio_output);
		gpio_export(gpio_output->pin);
		gpio_set_direction(gpio_output->pin, GPIO_DIR_OUT);
		gpio_set_value(gpio_output->pin, GPIO_VAL_LOW);
		gpio_output->value = GPIO_VAL_LOW;
		log_info("GPIO output created (name: %s, pin: %d)", output_get_name(output), gpio_output->pin);
		return true;
	}
	return false;
}

static bool gpio_output_exec(output_t *output, action_t *action)
{
	gpio_t *output_gpio = output_get_userdata(output);
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
		gpio_set_value(output_gpio->pin, !output_gpio->value);
		output_gpio->value = !output_gpio->value;
		break;
	}
	log_debug("gpio %s(%d) set to: %d", output_get_name(output), output_gpio->pin, output_gpio->value);

	return false;
}

bool gpio_technology_init(void)
{
	event_type_e events = EVENT_PRESS | EVENT_RELEASE | EVENT_SHORT_PRESS | EVENT_LONG_PRESS | EVENT_DIM;
	input_register_type("GPIO", events, gpio_input_parser, gpio_input_exec);
	action_type_e actions = ACTION_SET | ACTION_UNSET | ACTION_TOGGLE;
	output_register_type("GPIO", actions, gpio_output_parser, gpio_output_exec);

	log_info("Succesfully initialized: gpio!");
	return true;
}