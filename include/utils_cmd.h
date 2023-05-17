/*
 * utils_time.h
 *
 *  Created on: Dec 23, 2016
 *      Author: jnevens
 */

#ifndef UTILS_CMD_H_
#define UTILS_CMD_H_

#include <stdint.h>

typedef struct cmd_s cmd_t;

char *cmd_execute(const char *cmd);

cmd_t* cmd_create(const char *console_cmd, void (*cmd_resp_cb)(int rv, const char *resp, ssize_t resp_size, void *arg) ,void *arg);
bool cmd_exec(cmd_t *cmd);
bool cmd_is_done(cmd_t *cmd);
void cmd_destroy(cmd_t *cmd);


#endif /* UTILS_CMD_H_ */
