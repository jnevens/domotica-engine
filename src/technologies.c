/*
 * technologies.c
 *
 *  Created on: Dec 20, 2016
 *      Author: jnevens
 */
#include <stdio.h>

#include <bus/log.h>

#include "eltako.h"
#include "gpio.h"
#include "technologies.h"

typedef struct {
	technology_init_fn_t init_fn;
	char *name;
} technology_t;

static technology_t technologies[] = {
		{
				.init_fn = eltako_technology_init,
				.name = "Eltako Series 14",
		},
		{
				.init_fn = gpio_technology_init,
				.name = "GPIO",
		},
		{}
};

bool technologies_init(void)
{
	bool ret = false;
	int i = 0;

	while(technologies[i].init_fn) {
		ret = technologies[i].init_fn();
		if (ret == true) {
			log_info("Successfully initialize technology '%s'!", technologies[i].name);
		} else {
			log_fatal("Failed to initialize technology '%s'!", technologies[i].name);
			break;
		}
		i++;
	}

	return ret;
}
