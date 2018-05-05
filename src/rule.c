/*
 * rule.c
 *
 *  Created on: Dec 19, 2016
 *      Author: jnevens
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <eu/log.h>
#include <eu/list.h>

#include "device.h"
#include "rule.h"
#include "event.h"
#include "action.h"

struct rule_s {
	event_t *event;
	eu_list_t *conditions;
	eu_list_t *actions;
	int count;
};

rule_t *rule_create(void)
{
	rule_t *rule = calloc(1, sizeof(rule_t));
	rule->conditions = eu_list_create();
	rule->actions = eu_list_create();

	return rule;
}

void rule_destroy(rule_t *rule)
{
	if (rule) {
		eu_list_destroy_with_data(rule->conditions, free);
		eu_list_destroy_with_data(rule->actions, free);
		free(rule);
	}
}

bool rule_add_event(rule_t *rule, event_t *event)
{
	if (rule->event)
		event_destroy(rule->event);

	rule->event = event;

	return true;
}

bool rule_add_condition(rule_t *rule, condition_t *condition)
{
	if (!rule || !condition) {
		return false;
	}

	eu_list_append(rule->conditions, condition);
	return true;
}

bool rule_add_action(rule_t *rule, action_t *action)
{
	if (!rule || !action) {
		return false;
	}

	eu_list_append(rule->actions, action);
	return true;
}

event_t *rule_get_event(rule_t *rule)
{
	return rule->event;
}

void rule_foreach_action(rule_t *rule, rule_action_iter_fn_t action_cb, void *arg)
{
	eu_list_node_t *node;

	eu_list_for_each(node, rule->actions)
	{
		action_t *action = eu_list_node_data(node);
		action_cb(action, arg);
	}
}

void rule_foreach_condition(rule_t *rule, rule_condition_iter_fn_t condition_cb, void *arg)
{
	eu_list_node_t *node;

	eu_list_for_each(node, rule->conditions)
	{
		condition_t *condition = eu_list_node_data(node);
		condition_cb(condition, arg);
	}
}

void rule_print(rule_t *rule)
{
	eu_list_node_t *node;
	event_t *event = rule->event;

	eu_log_debug("print rule:");
	event_print(event);

	eu_list_for_each(node, rule->actions)
	{
		action_t *action = eu_list_node_data(node);
		action_print(action);
	}

}
