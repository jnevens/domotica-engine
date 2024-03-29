/*
 * technologies.c
 *
 *  Created on: Dec 20, 2016
 *      Author: jnevens
 */
#include <stdio.h>
#include <stdbool.h>

#include <eu/log.h>

#include "config.h"
#include "eltako.h"
#include "velux_klf200.h"
#include "soltracker.h"
#include "gpio.h"
#include "yamaha_mc.h"
#include "timer.h"
#include "sunriset.h"
#include "dht22.h"
#include "bool.h"

#include "technologies.h"

typedef struct {
	technology_init_fn_t init_fn;
	technology_cleanup_fn_t cleanup_fn;
	char *name;
	bool exit_on_fail;
} technology_t;

static technology_t technologies[] = {
		{
				.init_fn = timer_technology_init,
				.name = "Timer",
				.exit_on_fail = true,
		},
#ifdef ENABLE_MOD_ELTAKO
		{
				.init_fn = eltako_technology_init,
				.name = "Eltako Series 14",
				.exit_on_fail = false,
		},
#endif
#ifdef ENABLE_MOD_SOLTRACK
		{
				.init_fn = soltracker_technology_init,
				.cleanup_fn = soltracker_technology_cleanup,
				.name = "Soltracker",
				.exit_on_fail = true,
		},
#endif
#ifdef ENABLE_MOD_MUSICCAST
		{
				.init_fn = musiccast_technology_init,
				.cleanup_fn = musiccast_technology_cleanup,
				.name = "Yamaha MusicCast",
				.exit_on_fail = true,
		},
#endif
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
		{
				.init_fn = dht22_technology_init,
				.name = "DHT22",
				.exit_on_fail = true,
		},
		{
				.init_fn = bool_technology_init,
				.name = "Booleans",
				.exit_on_fail = true,
		},
		{
				.init_fn = velux_klf200_technology_init,
				.name = "VELUX KLF200",
				.exit_on_fail = true,
		}
};

bool technologies_init(void)
{
	bool fail = false;

	for(int i = 0; i < (sizeof(technologies) / sizeof(technology_t)); i++) {
		bool ret = technologies[i].init_fn();
		if (ret == true) {
			eu_log_info("Successfully initialize technology '%s'!", technologies[i].name);
		} else {
			if (technologies[i].exit_on_fail == true) {
				eu_log_fatal("Failed to initialize technology '%s'!", technologies[i].name);
				fail = true;
				break;
			} else {
				eu_log_err("Failed to initialize technology '%s'!", technologies[i].name);
			}
		}
	}
	return !fail;
}

void technologies_cleanup(void)
{
	for(int i = 0; i < (sizeof(technologies) / sizeof(technology_t)); i++) {
		if (technologies[i].cleanup_fn) {
			technologies[i].cleanup_fn();
		}
	}
}
