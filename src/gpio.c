/*
 * gpio.c
 *
 *  Created on: Dec 27, 2016
 *      Author: jnevens
 */
#include <stdio.h>

#include <bus/log.h>

#include "gpio.h"

bool gpio_technology_init(void)
{
	log_info("Succesfully initialized: gpio!");
	return true;
}
