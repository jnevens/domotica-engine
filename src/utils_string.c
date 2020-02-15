/*
 * utils_string.c
 *
 *  Created on: Aug 17, 2019
 *      Author: jnevens
 */
#include <string.h>
#include <stdlib.h>

#include "utils_string.h"

void strappend(char **base_ptr, const char *append)
{
	char *base = NULL;
	size_t len_base = strlen(*base_ptr);
	size_t len_app = strlen(append);
	size_t len_new = len_base + len_app + 1;

	base = realloc(*base_ptr, len_new);

	memcpy(base + len_base, append, len_app);
	base[len_base + len_app] = '\0';

	*base_ptr = base;
}
