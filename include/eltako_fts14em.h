/*
 * eltako_fts14em.h
 *
 *  Created on: Dec 22, 2016
 *      Author: jnevens
 */

#ifndef ELTAKO_FTS14EM_H_
#define ELTAKO_FTS14EM_H_

#include <stdbool.h>
#include <stdint.h>
#include <libeltako/message.h>

#include "types.h"

bool eltako_fts14em_init(void);

void eltako_fts14em_incoming_data(eltako_message_t *msg, device_t *device);

uint32_t eltako_fts14em_get_address(device_t *device);

#endif /* ELTAKO_FTS14EM_H_ */
