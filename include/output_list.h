/*
 * output_list.h
 *
 *  Created on: Dec 20, 2016
 *      Author: jnevens
 */

#ifndef OUTPUT_LIST_H_
#define OUTPUT_LIST_H_

#include <stdbool.h>
#include <bus/list.h>
#include "types.h"

bool output_list_init(void);
list_t *output_list_get(void);
void output_list_destroy(void);

bool output_list_add(output_t *output);
output_t *output_list_find_by_name(const char *name);


#endif /* OUTPUT_LIST_H_ */
