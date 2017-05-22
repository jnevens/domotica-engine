/*
 * utils_time.h
 *
 *  Created on: Dec 23, 2016
 *      Author: jnevens
 */

#ifndef UTILS_TIME_H_
#define UTILS_TIME_H_

#include <stdint.h>

uint64_t get_current_time_s(void);
uint64_t get_current_time_ms(void);
uint64_t get_current_time_us(void);

// Busy wait delay for most accurate timing, but high CPU usage.
// Only use this for short periods of time (a few hundred milliseconds at most)!
void busy_wait_milliseconds(uint32_t millis);

// General delay that sleeps so CPU usage is low, but accuracy is potentially bad.
void sleep_milliseconds(uint32_t millis);

#endif /* UTILS_TIME_H_ */
