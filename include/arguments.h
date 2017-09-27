/*
 * arguments.h
 *
 *  Created on: Dec 18, 2016
 *      Author: jnevens
 */

#ifndef ARGUMENTS_H_
#define ARGUMENTS_H_

/* Used by main to communicate with parse_opt. */
typedef struct arguments {
	bool daemonize;
	bool config_test;
	char *pidfile;
	char *rulesdir;
} arguments_t;

arguments_t *arguments_get(void);
int arguments_parse(int argc, char *argv[]);

#endif /* ARGUMENTS_H_ */
