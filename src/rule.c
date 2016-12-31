/*
 * rule.c
 *
 *  Created on: Dec 19, 2016
 *      Author: jnevens
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <bus/log.h>
#include <bus/list.h>

#include "input.h"
#include "rule.h"
#include "event.h"
#include "action.h"

struct rule_s {
	event_t *event;
	list_t *actions;
	int count;
};

rule_t *rule_create(void)
{
	rule_t *rule = calloc(1, sizeof(rule_t));
	rule->actions = list_create();

	return rule;
}

void rule_destroy(rule_t *rule)
{
	free(rule);
}

bool rule_add_event(rule_t *rule, event_t *event)
{
	if (rule->event)
		event_delete(rule->event);

	rule->event = event;

	return true;
}

bool rule_add_condition(rule_t *rule, const char *name, const char *condition)
{
	return false;
}

bool rule_add_action(rule_t *rule, action_t *action)
{
	if (!rule || !action) {
		return false;
	}

	list_append(rule->actions, action);
	return true;
}

event_t *rule_get_event(rule_t *rule)
{
	return rule->event;
}

void rule_foreach_action(rule_t *rule, rule_action_iter_fn_t action_cb, void *arg)
{
	list_node_t *node;

	list_for_each(node, rule->actions)
	{
		action_t *action = list_node_data(node);
		action_cb(action, arg);
	}
}

void rule_print(rule_t *rule)
{
	list_node_t *node;
	event_t *event = rule->event;

	log_debug("print rule:");
	event_print(event);

	list_for_each(node, rule->actions)
	{
		action_t *action = list_node_data(node);
		action_print(action);
	}

}