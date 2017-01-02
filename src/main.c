#include <stdio.h>
#include <signal.h>

#include <bus/event.h>
#include <bus/log.h>
#include <bus/daemon.h>

#include "arguments.h"
#include "rule_parser.h"
#include "technologies.h"
#include "rule_list.h"
#include "device_list.h"

static void termination_handler(int signum)
{
	log_err("Terminate event loop!");
	event_loop_stop();
}

int main(int argc, char *argv[])
{
	// handle signals
	signal(SIGTERM, termination_handler);
	signal(SIGINT, termination_handler);

	// init event loop
	event_loop_init();
	// init logging
	log_init("domotica-engine");
	log_set_syslog_level(BLOG_WARNING);
	log_set_print_level(BLOG_DEBUG);
	// parse arguments
	arguments_parse(argc, argv);
	// parse config file
	rule_list_init();
	device_list_init();
	if(!technologies_init())
		goto stop;

	// parse rules
	if(rules_read_dir(arguments_get()->rulesdir))
		goto stop;

	// deamonize
	if(arguments_get()->daemonize) {
		log_info("Daemonize...");
		daemonize(arguments_get()->pidfile);
	}

	// initialize Domotica engine

	log_info("Domotica engine started...");

	// run baby, run!
	event_loop();
stop:
	event_loop_cleanup();

	return 0;
}
