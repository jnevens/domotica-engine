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

#include "utils_cmd.h"

#define MAX_RES_SIZE (128*1042)

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
