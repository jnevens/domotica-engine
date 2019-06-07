/*
 * store.h
 *
 *  Created on: 18 Dec 2018
 *      Author: jnevens
 */

#ifndef INCLUDE_STORE_H_
#define INCLUDE_STORE_H_

int store_init(void);

int store_devices_state(void);
int restore_devices_state(void);

#endif /* INCLUDE_STORE_H_ */
