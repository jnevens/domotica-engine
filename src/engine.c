/*
 * engine.c
 *
 *  Created on: Dec 23, 2016
 *      Author: jnevens
 */
#include <stdio.h>

#include <bus/log.h>

#include "action.h"
#include "rule_list.h"
#include "rule.h"
#include "event.h"
#include "engine.h"

static void trigger_action_iter_cb(action_t *action, void *arg)
{
	event_t *event = arg;
	log_debug("Execute action!");
	action_execute(action, event);
}

static void trigger_event_iter_cb(rule_t *rule, void *arg)
{
	event_t *event = arg;
	rule_print(rule);
	// event OK
	// TODO: check all conditions
	// execute all actions
	rule_foreach_action(rule, trigger_action_iter_cb, event);
}

void engine_trigger_event(event_t *event)
{
	event_print(event);
	rule_list_foreach_event(trigger_event_iter_cb, event, event);
}
