/*
 * eltako.h
 *
 *  Created on: Dec 21, 2016
 *      Author: jnevens
 */

#ifndef ELTAKO_H_
#define ELTAKO_H_

#include <stdbool.h>
#include <libeltako/message.h>

bool eltako_technology_init(void);
bool eltako_send(eltako_message_t *msg);

#endif /* ELTAKO_H_ */
