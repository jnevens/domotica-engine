#include <stdio.h>
#include <signal.h>
#include <unistd.h>
#include <time.h>

#include <eu/event.h>
#include <eu/log.h>
#include <eu/daemon.h>

#include "arguments.h"
#include "rule_parser.h"
#include "technologies.h"
#include "rule_list.h"
#include "device_list.h"
#include "utils_time.h"
#include "schedule.h"

static void termination_handler(int signum)
{
	eu_log_err("Terminate event loop!");
	eu_event_loop_stop();
}

static void fix_time(void)
{
	// wait for system time
	while (get_current_time_s() < 47 * 365 * 24 * 3600) {
		eu_log_info("No system time set... waiting");
		sleep(1);
	}

	time_t t = time(NULL);
	struct tm lt = { 0 };

	localtime_r(&t, &lt);

	eu_log_info("System time set (TZ=%s GMT%s%lds)", lt.tm_zone, (lt.tm_gmtoff >= 0) ? "+" : "", lt.tm_gmtoff);
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
	// initialize time
	fix_time();
	// parse config file
	rule_list_init();
	device_list_init();
	schedule_init();
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
