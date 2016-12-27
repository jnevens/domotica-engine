/*
 * input_list.h
 *
 *  Created on: Dec 20, 2016
 *      Author: jnevens
 */

#ifndef INPUT_LIST_H_
#define INPUT_LIST_H_

#include <stdbool.h>
#include <bus/list.h>
#include "types.h"

typedef bool (*input_list_find_fn_t)(input_t *input, void *arg);

bool input_list_init(void);
list_t *input_list_get(void);
void input_list_destroy(void);

bool input_list_add(input_t *input);
input_t *input_list_find_by_name(const char *name);
input_t *input_list_find(input_list_find_fn_t find_cb, void *arg);

#endif /* INPUT_LIST_H_ */
