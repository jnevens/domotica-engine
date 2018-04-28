/*
 * rules_parser.c
 *
 *  Created on: Dec 18, 2016
 *      Author: jnevens
 */
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <sys/types.h>
#include <dirent.h>
#include <unistd.h>
#include <errno.h>

#include <eu/log.h>

#include "event.h"
#include "action.h"
#include "condition.h"
#include "rule.h"
#include "device.h"
#include "device_list.h"
#include "line_parser.h"
#include "rule_list.h"
#include "schedule.h"



static void action_input_handler(char *line);
static void action_output_handler(char *line);

// todo rule_t input_t event_t output_t action_t



int rules_read_file_declarations(const char *file)
{
	FILE *fp;
	int line_nr = 1, ret = 0;
	eu_log_info("read rules file: %s", file);

	if (NULL == (fp = fopen(file, "r"))) {
		eu_log_err("Failed to open file: %s", strerror(errno));
		return -1;
	}

	char line[256];
	while (fgets(line, sizeof(line), fp) != NULL) {
		static rule_t *rule = NULL;
		static schedule_t *schedule = NULL;
		line_t *ln = line_parse(line);

		if (ln) {
			if (schedule) {
				if (schedule_parse_line(schedule, ln->raw)) {
					continue;
				} else {
					eu_log_fatal("Failed to parse device (invalid action): %s (%s:%d)", ln->name, file, line_nr);
					ret = -1;
					break;
				}
			}

			switch (ln->statement) {
			/* declarations */
			case STATEMENT_INPUT:
			case STATEMENT_OUTPUT:
			case STATEMENT_SUNRISET:
			case STATEMENT_SOLTRACKER:
			case STATEMENT_TIMER:
			case STATEMENT_BOOL:
			case STATEMENT_SCHEDULE: {
				const char *devtype = ln->options[0];
				if (ln->statement == STATEMENT_TIMER)
					devtype = "TIMER";
				if (ln->statement == STATEMENT_SUNRISET)
					devtype = "SUNRISET";
				if (ln->statement == STATEMENT_SOLTRACKER)
					devtype = "SOLTRACKER";
				if (ln->statement == STATEMENT_SCHEDULE)
					devtype = "SCHEDULE";
				if (ln->statement == STATEMENT_BOOL)
					devtype = "BOOL";
				device_t *device = device_create(ln->name, devtype, ln->options);
				if (device) {
					device_list_add(device);
					if (ln->statement == STATEMENT_SCHEDULE) {
						schedule = device_get_userdata(device);
					}
				} else {
					eu_log_fatal("Failed to parse device: %s (%s:%d)", ln->name, file, line_nr);
					ret = -1;
				}
				break;
			}
			/* rules */
			case STATEMENT_IF:
			case STATEMENT_AND:
			case STATEMENT_DO: {
				break;
			}
			default: {
				eu_log_fatal("Failed to parse device (invalid action): %s (%s:%d)", ln->name, file, line_nr);
				ret = -1;
				break;
			}
			}
			if (ret)
				break;
			//eu_log_info("%s", line);
		} else {
			rule = NULL;
			if (schedule) {
				schedule_parsing_finished(schedule);
				schedule = NULL;
			}
		}
		line_nr++;
		line_destroy(ln);
		ln = NULL;
	}

	fclose(fp);
	return ret;
}

int rules_read_file_rules(const char *file)
{
	FILE *fp;
	int line_nr = 1, ret = 0;
	eu_log_info("read rules file: %s", file);

	if (NULL == (fp = fopen(file, "r"))) {
		eu_log_err("Failed to open file: %s", strerror(errno));
		return -1;
	}

	char line[256];
	while (fgets(line, sizeof(line), fp) != NULL) {
		static rule_t *rule = NULL;
		static bool schedule = false;
		line_t *ln = line_parse(line);

		if (ln) {
			if (schedule)
				continue;

			switch (ln->statement) {
			/* declarations */
			case STATEMENT_INPUT:
			case STATEMENT_OUTPUT:
			case STATEMENT_SUNRISET:
			case STATEMENT_SOLTRACKER:
			case STATEMENT_TIMER:
			case STATEMENT_BOOL:
			case STATEMENT_SCHEDULE: {
				if (ln->statement == STATEMENT_SCHEDULE)
					schedule = true;
				break;
			}
			/* rules */
			case STATEMENT_IF: {
				eu_log_debug("Create rule");
				rule = rule_create();
				device_t *device = device_list_find_by_name(ln->name);
				if (!device) {
					eu_log_err("Cannot find device: %s", ln->name);
					ret = -1;
					break;
				}
				event_type_e event_type = event_type_from_char(ln->options[0]);
				if (event_type == -1) {
					eu_log_err("Event '%s' does not exist!", ln->options[0]);
					ret = -1;
					break;
				}
				eu_log_debug("Rule add event: %s %s", ln->name, ln->options[0]);
				event_t *event = event_create(device, event_type);
				rule_add_event(rule, event);
				rule_list_add(rule);
				break;
			}
			case STATEMENT_AND: {
				eu_log_debug("Rule add condition: %s %s", ln->name, ln->options[0]);
				if (!rule) {
					eu_log_fatal("No IF before AND! (%s:%d)", file, line_nr);
				}
				device_t *device = device_list_find_by_name(ln->name);
				if (!device) {
					eu_log_err("Cannot find device: %s", ln->name);
					ret = -1;
					break;
				}
				condition_type_e condition_type = condition_type_from_char(ln->options[0]);
				if (condition_type == -1) {
					eu_log_err("Action '%s' does not exist!", ln->options[0]);
					ret = -1;
					break;
				}
				condition_t *condition = condition_create(device, condition_type, ln->options);
				rule_add_condition(rule, condition);
				break;
			}
			case STATEMENT_DO: {
				eu_log_debug("Rule add action: %s %s", ln->name, ln->options[0]);
				if (!rule) {
					eu_log_fatal("No IF before DO! (%s:%d)", file, line_nr);
				}
				device_t *device = device_list_find_by_name(ln->name);
				if (!device) {
					eu_log_err("Cannot find device: %s", ln->name);
					ret = -1;
					break;
				}
				action_type_e action_type = action_type_from_char(ln->options[0]);
				if (action_type == -1) {
					eu_log_err("Action '%s' does not exist!", ln->options[0]);
					ret = -1;
					break;
				}
				action_t *action = action_create(device, action_type, ln->options);
				rule_add_action(rule, action);
				break;
			}
			default: {
				eu_log_fatal("Failed to parse device (invalid action): %s (%s:%d)", ln->name, file, line_nr);
				ret = -1;
				break;
			}
			}
			if (ret)
				break;
			//eu_log_info("%s", line);
		} else {
			rule = NULL;
			schedule = false;
		}
		line_nr++;
		line_destroy(ln);
		ln = NULL;
	}

	fclose(fp);
	return ret;
}

int rules_read_file(const char *file_path, int pass)
{
	int ret = -1;

	if (pass == 1) {
		ret = rules_read_file_declarations(file_path);
	} else {
		ret = rules_read_file_rules(file_path);
	}

	return ret;
}

int rules_read_dir(const char *dir, int pass)
{
	DIR* FD = NULL;
	struct dirent* in_file = NULL;
	int ret = -1, files_read = 0;

	eu_log_info("read rules directory: %s", dir);

	/* Scanning the in directory */
	if (NULL == (FD = opendir(dir))) {
		eu_log_err("Failed to open rules directory: %s", strerror(errno));
		return -1;
	}

	while ((in_file = readdir(FD))) {
		char *file_path = NULL;
		size_t len = strlen(in_file->d_name);
		if (!strcmp(in_file->d_name, "."))
			continue;
		if (!strcmp(in_file->d_name, ".."))
			continue;
		if((len < 7) || strcmp(&in_file->d_name[len - 6],".rules"))
			continue;

		if(asprintf(&file_path, "%s/%s", dir, in_file->d_name) <= 0) {
			eu_log_err("asprintf failed: %s", strerror(errno));
			break;
		}

		ret = rules_read_file(file_path, pass);

		free(file_path);

		if(ret != 0)
			break;
		files_read++;
	}

	if(ret == 0 && files_read > 0)
		ret = 0;

	closedir(FD);

	return ret;
}

