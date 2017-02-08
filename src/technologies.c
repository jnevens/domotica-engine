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
#include "timer.h"
#include "sunriset.h"

typedef struct {
	technology_init_fn_t init_fn;
	char *name;
	bool exit_on_fail;
} technology_t;

static technology_t technologies[] = {
		{
				.init_fn = timer_technology_init,
				.name = "Timer",
				.exit_on_fail = true,
		},
		{
				.init_fn = eltako_technology_init,
				.name = "Eltako Series 14",
				.exit_on_fail = false,
		},
		{
				.init_fn = gpio_technology_init,
				.name = "GPIO",
				.exit_on_fail = true,
		},
		{
				.init_fn = sunriset_technology_init,
				.name = "Sunriset",
				.exit_on_fail = true,
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
			if (technologies[i].exit_on_fail)
				break;
		}
		i++;
	}

	return ret;
}
