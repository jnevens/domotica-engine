/*
 * utils_http.h
 *
 *  Created on: Jun 12, 2018
 *      Author: jnevens
 */

#ifndef SRC_UTLS_HTTP_H_
#define SRC_UTLS_HTTP_H_

#include <stdbool.h>

bool http_request(const char *url, char **response);

#endif /* SRC_UTLS_HTTP_H_ */
