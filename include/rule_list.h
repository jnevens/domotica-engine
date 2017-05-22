/*
 * rules_list.h
 *
 *  Created on: Dec 20, 2016
 *      Author: jnevens
 */

#ifndef RULE_LIST_H_
#define RULE_LIST_H_

#include <stdbool.h>
#include <eu/list.h>
#include "types.h"

typedef void (*rule_list_iter_fn_t)(rule_t *rule, void *arg);

bool rule_list_init(void);
eu_list_t *rule_list_get(void);
void rule_list_destroy(void);

bool rule_list_add(rule_t *rule);

void rule_list_foreach(rule_list_iter_fn_t iter_cb, void *arg);
void rule_list_foreach_event(rule_list_iter_fn_t iter_cb, event_t *event, void *arg);

#endif /* RULE_LIST_H_ */
