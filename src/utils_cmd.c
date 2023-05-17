/*
 * utils_cmd.c
 *
 *  Created on: Jan 5, 2023
 *      Author: jnevens
 */
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>

#include <eu/log.h>
#include <eu/event.h>

#include "utils_cmd.h"

#define MAX_RES_SIZE (128*1042)

struct cmd_s {
    char *cmd;
    char *resp;
    ssize_t resp_buf_size;
    ssize_t resp_size;
    int rv;
    bool done;
    FILE *pp;
    eu_event_t *event;
    void (*resp_cb)(int rv, const char *resp, ssize_t resp_size, void *arg);
    void *resp_cb_arg;
};

cmd_t* cmd_create(const char *console_cmd, void (*cmd_resp_cb)(int rv, const char *resp, ssize_t resp_size, void *arg) ,void *arg)
{
    cmd_t *cmd = calloc(1, sizeof(cmd_t));
    cmd->cmd = strdup(console_cmd);
    cmd->resp_cb = cmd_resp_cb;
    cmd->resp_cb_arg = arg;

    return cmd;
}

bool cmd_is_done(cmd_t *cmd)
{
    return cmd->done;
}

static void cmd_close(cmd_t *cmd)
{
    if (cmd->event) {
        eu_event_destroy(cmd->event);
        cmd->event = NULL;
    }
    if (cmd->pp) {
        pclose(cmd->pp);
        cmd->pp = NULL;
    }
}

static void cmd_incoming_data(int fd, short revents, void *arg)
{
    cmd_t *cmd = (cmd_t *)arg;

    /* check pipe is close (hang up) */
    if (revents & POLLHUP) {
        eu_log_err("cmd finished (0x%x)", revents);
        cmd_close(cmd);
        cmd->done = true;

        if (cmd->resp_cb)
            cmd->resp_cb(0, cmd->resp, cmd->resp_size, cmd->resp_cb_arg);
        return;
    }

    int c = fgetc(cmd->pp);
    if (c == EOF) {
        eu_log_info("cmd EOF");

        cmd->done = true;
        cmd_close(cmd);
        return;
    }
    if ((cmd->resp_size + 1) >= cmd->resp_buf_size) {
        cmd->resp_buf_size += 1024;
        if (cmd->resp_buf_size >= MAX_RES_SIZE) {
            eu_log_warn("cmd response too big");
            return;
        }
        cmd->resp = realloc(cmd->resp, cmd->resp_buf_size);
    }
    cmd->resp[cmd->resp_size] = c;
    cmd->resp[cmd->resp_size + 1] = '\0';
    cmd->resp_size++;
}

bool cmd_exec(cmd_t *cmd)
{
    eu_log_debug("Executing: %s", cmd->cmd);
    cmd->done = false;
    cmd->rv = -1;

    cmd->pp = popen(cmd->cmd, "r");
    if (!cmd->pp) {
        eu_log_err("Failed to run command");
        free(cmd->resp);
        cmd->resp_buf_size = 0;
        return false;
    }

    /* get file descriptor */
    int fd = fileno(cmd->pp);

    /* add to scheduler */
	cmd->event = eu_event_add(fd, POLLIN | POLLHUP | POLLERR, cmd_incoming_data, NULL, cmd);

    return true;
}

void cmd_destroy(cmd_t *cmd)
{
    cmd_close(cmd);
    free(cmd->cmd);
    free(cmd->resp);
    free(cmd);
}

char* cmd_execute(const char *cmd)
{
    eu_log_debug("Executing: %s", cmd);
    ssize_t buff_size = 1024;
    char *result = malloc(buff_size);

    FILE *pp = popen(cmd, "r");
    if (!pp) {
        eu_log_err("Failed to run command");
        free(result);
        return NULL;
    }

    int size = 0;
    int c;
    do {
        c = fgetc(pp);
        if (c == EOF) {
            break;
        }
        size++;
        if (size >= buff_size) {
            buff_size += 1024;
            if (buff_size >= MAX_RES_SIZE) {
                eu_log_warn("Too big");
                break;
            }
            result = realloc(result, buff_size+1);
        }
        result[size-1] = c;
    } while(true);

    if (fclose(pp) == -1) {
        eu_log_err("Failed to execute command");
        free(result);
        return NULL;
    }
    result[size] = '\0';
    return result;
}
