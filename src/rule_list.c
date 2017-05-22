/*
 * rules_list.c
 *
 *  Created on: Dec 20, 2016
 *      Author: jnevens
 */
#include "rule_list.h"

#include <stdio.h>

#include <eu/log.h>
#include <eu/list.h>

#include "event.h"
#include "rule.h"
#include "rule_list.h"

eu_list_t *rules = NULL;

bool rule_list_init(void)
{
	rules = eu_list_create();
}

eu_list_t *rule_list_get(void)
{
	return rules;
}

void rule_list_destroy(void)
{
	eu_list_destroy_with_data(rules, (eu_list_destroy_element_fn_t)rule_destroy);
	rules = NULL;
}

bool rule_list_add(rule_t *rule)
{
	eu_list_append(rules, rule);
}

void rule_list_foreach(rule_list_iter_fn_t iter_cb, void *arg)
{
	eu_list_node_t *node = NULL;
	eu_list_for_each(node, rules)
	{
		rule_t *rule = eu_list_node_data(node);
		iter_cb(rule, arg);
	}
}

void rule_list_foreach_event(rule_list_iter_fn_t iter_cb, event_t *event, void *arg)
{
	eu_list_node_t *node = NULL;
	eu_list_for_each(node, rules)
	{
		rule_t *rule = eu_list_node_data(node);
		event_t *rule_event = rule_get_event(rule);

		if (event_equal(rule_event, event)) {
			iter_cb(rule, arg);
		}
	}
}
