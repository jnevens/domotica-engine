/*
 * line_parser.h
 *
 *  Created on: Nov 26, 2017
 *      Author: jnevens
 */

#ifndef LINE_PARSER_H_
#define LINE_PARSER_H_

#define LINE_MAX_OPTIONS 10

#define STATEMENT_TABLE \
	X(STATEMENT_INPUT,		"INPUT",		NULL) \
	X(STATEMENT_OUTPUT,		"OUTPUT",		NULL) \
	X(STATEMENT_TIMER,		"TIMER",		NULL) \
	X(STATEMENT_IF,			"IF",			NULL) \
	X(STATEMENT_AND,		"AND",			NULL) \
	X(STATEMENT_DO,			"DO",			NULL) \
	X(STATEMENT_SUNRISET,	"SUNRISET",		NULL) \
	X(STATEMENT_SOLTRACKER,	"SOLTRACKER",	NULL) \
	X(STATEMENT_SCHEDULE,	"SCHEDULE",		NULL) \
	X(STATEMENT_BOOL,		"BOOL",			NULL) \
	X(STATEMENT_STATE,		"STATE",		NULL) \
	X(STATEMENT_LOGS,		"LOGS",			NULL) \
	X(STATEMENT_INVALID,	NULL,			NULL)

#define X(a, b, c) a,
typedef enum {
	STATEMENT_TABLE
} statement_e;
#undef X

typedef struct {
	statement_e statement;
	char *raw;
	char *name;
	char *options[LINE_MAX_OPTIONS];
	int	options_count;
} line_t;

line_t *line_parse(char *strline);
void line_destroy(line_t *line);

#endif /* LINE_PARSER_H_ */
