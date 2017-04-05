/*
 * arguments.c
 *
 *  Created on: Dec 18, 2016
 *      Author: jnevens
 */
#include <argp.h>
#include <string.h>
#include <stdbool.h>

#include "arguments.h"

/* Default arguments */
arguments_t arguments = {
	.daemonize = false,
	.pidfile = "/var/run/domotica-engine.pid",
	.rulesdir = "/etc/domotica-engine/rules.d",
};

/* Program documentation. */
static char doc[] = "Domoitia-Engine, switch all your lights/dimmers/blinds...";

/* A description of the arguments we accept. */
static char args_doc[] = "";

/* The options we understand. */
static struct argp_option options[] = {
		{ "deamonize",	'D',	0,	0,	"Deamonize application" },
		{ "pidfile",	'p',	"Pidfile", 0, "PID file (default: /var/run/eltakod.pid)" },
		{ 0 }
};

/* Parse a single option. */
static error_t parse_opt(int key, char *arg, struct argp_state *state) {
	/* Get the input argument from argp_parse, which we
	 know is a pointer to our arguments structure. */
	struct arguments *arguments = state->input;

	switch (key) {
	case 'D':
		arguments->daemonize = true;
		break;
	case 'p':
		arguments->pidfile = arg;
		break;
	case ARGP_KEY_ARG:
		break;
	case ARGP_KEY_END:
		if (state->arg_num >= 1)
			/* Not enough arguments. */
			argp_usage(state);
		break;
	default:
		return ARGP_ERR_UNKNOWN;
	}
	return 0;
}

static struct argp argp = { options, parse_opt, args_doc, doc };

arguments_t *arguments_get(void)
{
	return &arguments;
}

int arguments_parse(int argc, char *argv[])
{
	int ret = argp_parse(&argp, argc, argv, 0, 0, &arguments);
	return (ret == ARGP_NO_ERRS) ? 0 : ret;
}
