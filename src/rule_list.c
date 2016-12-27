/*
 * rules_list.c
 *
 *  Created on: Dec 20, 2016
 *      Author: jnevens
 */
#include "rule_list.h"

#include <stdio.h>

#include <bus/log.h>
#include <bus/list.h>

#include "event.h"
#include "rule.h"
#include "rule_list.h"

list_t *rules = NULL;

bool rule_list_init(void)
{
	rules = list_create();
}

list_t *rule_list_get(void)
{
	return rules;
}

void rule_list_destroy(void)
{
	list_destroy_with_data(rules, (list_destroy_element_fn_t)rule_destroy);
	rules = NULL;
}

bool rule_list_add(rule_t *rule)
{
	list_append(rules, rule);
}

void rule_list_foreach(rule_list_iter_fn_t iter_cb, void *arg)
{
	list_node_t *node = NULL;
	list_for_each(node, rules)
	{
		rule_t *rule = list_node_data(node);
		iter_cb(rule, arg);
	}
}

void rule_list_foreach_event(rule_list_iter_fn_t iter_cb, event_t *event, void *arg)
{
	list_node_t *node = NULL;
	list_for_each(node, rules)
	{
		rule_t *rule = list_node_data(node);
		event_t *rule_event = rule_get_event(rule);

		if (event_equal(rule_event, event)) {
			iter_cb(rule, arg);
		}
	}
}
