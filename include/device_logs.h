/*
 * device_logs.h
 *
 *  Created on: Aug 1, 2019
 *      Author: jnevens
 */

#ifndef INCLUDE_DEVICE_LOGS_H_
#define INCLUDE_DEVICE_LOGS_H_

typedef enum {
	LOG_TEMPERATURE,
	LOG_HUMIDITY,
} device_log_type_e;

bool device_log_add(device_t *device, device_log_type_e type, time_t ts, double value);
char *device_logs_get_json(device_t *device, time_t interval);

bool device_has_logs(device_t *device);

bool device_logs_store(void);
bool device_logs_load(void);

#endif /* INCLUDE_DEVICE_LOGS_H_ */
