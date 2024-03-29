/*
 * condition.h
 *
 *  Created on: Jun 27, 2017
 *      Author: jnevens
 */

#ifndef CONDITION_H_
#define CONDITION_H_

#include "types.h"

#define CONDITION_TYPE_TABLE \
	X(0,	CONDITION_UNSET,			"UNSET") \
	X(1,	CONDITION_SET,				"SET") \
	X(2,	CONDITION_UP,				"UP") \
	X(3,	CONDITION_DOWN,				"DOWN") \
	X(4,	CONDITION_STOPPED,			"STOPPED") \
	X(5,	CONDITION_RISING,			"RISING") \
	X(6,	CONDITION_DESCENDING,		"DESCENDING") \
	X(7,	CONDITION_LOCKED,			"LOCKED") \
	X(8,	CONDITION_UNLOCKED,			"UNLOCKED") \
	X(9,	CONDITION_SUNRISE,			"SUNRISE") \
	X(10,	CONDITION_SUNRISE_CIV,		"SUNRISE_CIVIL") \
	X(11,	CONDITION_SUNRISE_NAUT,		"SUNRISE_NAUTICAL") \
	X(12,	CONDITION_SUNRISE_ASTRO,	"SUNRISE_ASTRONOMICAL") \
	X(13,	CONDITION_SUNSET,			"SUNSET") \
	X(14,	CONDITION_SUNSET_CIV,		"SUNSET_CIVIL") \
	X(15,	CONDITION_SUNSET_NAUT,		"SUNSET_NAUTICAL") \
	X(16,	CONDITION_SUNSET_ASTRO,		"SUNSET_ASTRONOMICAL") \

#define X(a,b,c) b = (1 << a),
enum condition_type {
	CONDITION_TYPE_TABLE
};
#undef X

condition_t *condition_create(device_t *device, condition_type_e condition_type, char *options[]);
void condition_destroy(condition_t *condition);

condition_type_e condition_type_from_char(const char *condition_str);
const char *condition_type_to_char(condition_type_e type);
condition_type_e condition_get_type(condition_t *condition);

bool condition_check(condition_t *condition);



#endif /* CONDITION_H_ */
