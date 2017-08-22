/*
 * engine.c
 *
 *  Created on: Dec 23, 2016
 *      Author: jnevens
 */
#include <stdio.h>

#include <eu/log.h>

#include "action.h"
#include "condition.h"
#include "rule_list.h"
#include "rule.h"
#include "event.h"
#include "engine.h"

static void trigger_action_iter_cb(action_t *action, void *arg)
{
	event_t *event = arg;
	action_execute(action, event);
}

static void check_condition_iter_cb(condition_t *condition, void *arg)
{
	bool *condition_ok = arg;
	if (condition_check(condition)) {
		eu_log_debug("Check condition OK");
	} else {
		*condition_ok = false;
		eu_log_debug("Check condition NOK");
	}
}

static void trigger_event_iter_cb(rule_t *rule, void *arg)
{
	event_t *event = arg;
	bool conditions_ok = true;
	rule_print(rule);
	// event OK
	rule_foreach_condition(rule, check_condition_iter_cb, &conditions_ok);
	// execute all actions
	if (conditions_ok) {
		rule_foreach_action(rule, trigger_action_iter_cb, event);
	}
}

void engine_trigger_event(event_t *event)
{
	event_print(event);
	rule_list_foreach_event(trigger_event_iter_cb, event, event);
}
