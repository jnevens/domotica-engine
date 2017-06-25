/*
 * rules_parser.c
 *
 *  Created on: Dec 18, 2016
 *      Author: jnevens
 */
#define _GNU_SOURCE
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
#include "rule.h"
#include "device.h"
#include "device_list.h"
#include "rule_list.h"
#include "schedule.h"

#define LINE_MAX_OPTIONS 10

static void action_input_handler(char *line);
static void action_output_handler(char *line);

// todo rule_t input_t event_t output_t action_t

#define STATEMENT_TABLE \
	X(STATEMENT_INPUT,		"INPUT",	action_input_parser) \
	X(STATEMENT_OUTPUT,		"OUTPUT",	action_output_parser) \
	X(STATEMENT_TIMER,		"TIMER",	NULL) \
	X(STATEMENT_IF,			"IF",		NULL) \
	X(STATEMENT_AND,		"AND",		NULL) \
	X(STATEMENT_DO,			"DO",		NULL) \
	X(STATEMENT_SUNRISET,	"SUNRISET",	NULL) \
	X(STATEMENT_SCHEDULE,	"SCHEDULE",	NULL) \
	X(STATEMENT_INVALID,	NULL,		NULL)

#define X(a, b, c) a,
typedef enum {
	STATEMENT_TABLE
} statement_e;
#undef X

#define X(a, b, c) b,
char *statement_names[] = {
	STATEMENT_TABLE
};
#undef X

static void action_input_parser(rule_t *rule, char *line)
{

}

static void action_output_parser(rule_t *rule, char *line)
{

}

typedef struct {
	statement_e statement;
	char *raw;
	char *name;
	char *options[LINE_MAX_OPTIONS];
	int	options_count;
} line_t;

static void line_cleanup(char *line) {
	if(line[strlen(line)-1] == '\n')
		line[strlen(line) - 1] = '\0';
}

static bool line_is_comment(const char *line)
{
	if (strlen(line) == 0)
		return true;
	if (line[0] == '#')
		return true;
	return false;
}

static statement_e line_get_statement(const char *line)
{
	int i;

	for(i = 0; statement_names[i] != NULL; i++) {
		if(strncmp(line, statement_names[i], strlen(statement_names[i])) == 0)
			break;
	}
	return i;
}

static void line_destroy(line_t *line)
{
	if (line) {
		int i;
		for (i = 0; i < LINE_MAX_OPTIONS; i++) {
			free(line->options[i]);
		}
		free(line->name);
		free(line->raw);
		free(line);
	}
}

static line_t *line_parse(char *strline)
{
	line_cleanup(strline);
	if(line_is_comment(strline))
		return NULL;

	int i, n = 0;
	char *ln = strdup(strline);
	line_t *line = calloc(1, sizeof(line_t));
	line->raw = strdup(strline);
	line->statement = line_get_statement(strline);

	size_t len = strlen(ln);
	for(i = 0; i < len; i++) {
		if(ln[i] == ' ' || ln[i] == '\t')
			ln[i] = '\0';
	}
	for(i = 0; i < len; i++) {
		if (ln[i] == '\0') {
			if (n == 0) {
				line->name = strdup(&ln[i + 1]);
			} else if (n > 0) {
				line->options[n-1] = strdup(&ln[i + 1]);
				line->options_count++;
			}
			n++;
		}
	}

	free(ln);
	if (n < 2) {

		line = NULL;
	}

	return line;
}

int rules_read_file(const char *file)
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
			case STATEMENT_INPUT:
			case STATEMENT_OUTPUT:
			case STATEMENT_SUNRISET:
			case STATEMENT_TIMER: {
				const char *devtype = ln->options[0];
				if (ln->statement == STATEMENT_TIMER)
					devtype = "TIMER";
				if (ln->statement == STATEMENT_SUNRISET)
					devtype = "SUNRISET";
				device_t *device = device_create(ln->name, devtype, ln->options);
				if (device) {
					device_list_add(device);
				} else {
					eu_log_fatal("Failed to parse device: %s (%s:%d)", ln->name, file, line_nr);
					ret = -1;
				}
				break;
			}
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
				rule_add_condition(rule, ln->name, ln->options[0]);
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
			case STATEMENT_SCHEDULE: {
				eu_log_debug("Create schedule: %s %s", ln->name, ln->options[0]);
				schedule = schedule_create(ln->name, ln->options[0]);
				if (!schedule) {
					eu_log_fatal("Failed to parse schedule: %s (%s:%d)", ln->name, file, line_nr);
					ret = -1;
				}
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
			schedule = NULL;
		}
		line_nr++;
		line_destroy(ln);
		ln = NULL;
	}

	fclose(fp);
	return ret;
}

int rules_read_dir(const char *dir)
{
	DIR* FD;
	struct dirent* in_file;
	FILE *entry_file;
	char buffer[BUFSIZ];
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

		ret = rules_read_file(file_path);
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

