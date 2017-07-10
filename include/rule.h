/*
 * rule.h
 *
 *  Created on: Dec 19, 2016
 *      Author: jnevens
 */

#ifndef RULE_H_
#define RULE_H_

#include "types.h"

typedef void (*rule_action_iter_fn_t)(action_t *action, void *arg);
typedef void (*rule_condition_iter_fn_t)(condition_t *condition, void *arg);

rule_t *rule_create(void);
void rule_destroy(rule_t *rule);

bool rule_add_event(rule_t *rule, event_t *event);
bool rule_add_condition(rule_t *rule, condition_t *condition);
bool rule_add_action(rule_t *rule, action_t *action);

event_t *rule_get_event(rule_t *rule);

void rule_foreach_action(rule_t *rule, rule_action_iter_fn_t action_cb, void *arg);
void rule_foreach_condition(rule_t *rule, rule_condition_iter_fn_t condition_cb, void *arg);

void rule_print(rule_t *rule);

#endif /* RULE_H_ */
