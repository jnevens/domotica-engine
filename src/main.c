#include <stdio.h>
#include <signal.h>
#include <unistd.h>

#include <eu/event.h>
#include <eu/log.h>
#include <eu/daemon.h>

#include "arguments.h"
#include "rule_parser.h"
#include "technologies.h"
#include "rule_list.h"
#include "device_list.h"

static void termination_handler(int signum)
{
	eu_log_err("Terminate event loop!");
	eu_event_loop_stop();
}

int main(int argc, char *argv[])
{
	// handle signals
	signal(SIGTERM, termination_handler);
	signal(SIGINT, termination_handler);

	// init event loop
	eu_event_loop_init();
	// init logging
	eu_log_init("domotica-engine");
	eu_log_set_syslog_level(EU_LOG_DEBUG);
	eu_log_set_print_level(EU_LOG_DEBUG);
	// parse arguments
	arguments_parse(argc, argv);
	// wait for time
	if (get_current_time_s() < 47*365*24*3600) {

		sleep(1);
	}
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
		eu_log_info("Daemonize...");
		eu_daemonize(arguments_get()->pidfile);
	}

	// initialize Domotica engine

	eu_log_info("Domotica engine started...");

	// run baby, run!
	eu_event_loop();
stop:
	eu_event_loop_cleanup();

	return 0;
}
