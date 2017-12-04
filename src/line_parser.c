/*
 * line_parser.c
 *
 *  Created on: Nov 26, 2017
 *      Author: jnevens
 */
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include "line_parser.h"

#define X(a, b, c) b,
char *statement_names[] = {
	STATEMENT_TABLE
};
#undef X

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

void line_destroy(line_t *line)
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

line_t *line_parse(char *strline)
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
	int minoptions = (line->statement == STATEMENT_STATE) ? 0 : 1;
	if (n < (1 + minoptions)) {
		line_destroy(line);
		line = NULL;
	}

	return line;
}
