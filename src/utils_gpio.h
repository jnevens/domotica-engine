/*
 * utils_gpio.h
 *
 *  Created on: Dec 28, 2016
 *      Author: jnevens
 */

#ifndef UTILS_GPIO__H_
#define UTILS_GPIO__H_

typedef enum {
	GPIO_VAL_LOW = 0,
	GPIO_VAL_HIGH
} gpio_value_e;

typedef enum {
	GPIO_DIR_IN = 0,
	GPIO_DIR_OUT,
} gpio_direction_e;

typedef enum {
	GPIO_EDGE_NONE = 0,
	GPIO_EDGE_RISING,
	GPIO_EDGE_FALLING,
	GPIO_EDGE_BOTH,
} gpio_edge_e;

int gpio_export(int pin);
int gpio_unexport(int pin);
int gpio_fd_open(int pin);
void gpio_fd_close(int fd);
int gpio_set_direction(int pin, gpio_direction_e dir);
int gpio_set_edge(int pin, gpio_edge_e edge);
int gpio_get_value(int pin);
int gpio_set_value(int pin, gpio_value_e value);

#endif /* UTILS_GPIO__H_ */
