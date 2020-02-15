/*
 * dht22_private.h
 *
 *  Created on: Feb 15, 2020
 *      Author: jnevens
 */

#ifndef INCLUDE_DHT22_PRIVATE_H_
#define INCLUDE_DHT22_PRIVATE_H_

typedef struct {
	int gpio;
	time_t last_measurement;
	float temperature;
	float humidity;
	size_t fail_count;
	bool error;
	time_t error_time;
} dht22_t;

#endif /* INCLUDE_DHT22_PRIVATE_H_ */
