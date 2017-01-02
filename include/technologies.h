/*
 * technologies.h
 *
 *  Created on: Dec 20, 2016
 *      Author: jnevens
 */

#ifndef TECHNOLOGIES_H_
#define TECHNOLOGIES_H_


typedef bool (*technology_init_fn_t)(void);

bool technologies_init(void);

#endif /* TECHNOLOGIES_H_ */
