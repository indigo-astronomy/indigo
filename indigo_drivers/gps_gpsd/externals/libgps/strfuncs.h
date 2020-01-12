/*
 * strfuncs.h - string functions
 *
 * This software is distributed under a BSD-style license. See the
 * file "COPYING" in the toop-level directory of the distribution for details.
 */
#ifndef _GPSD_STRFUNCS_H_
#define _GPSD_STRFUNCS_H_

#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include "compiler.h"


static inline bool str_starts_with(const char *str, const char *prefix)
{
    return strncmp(str, prefix, strlen(prefix)) == 0;
}


PRINTF_FUNC(3, 4)
static inline void str_appendf(char *str, size_t alloc_size, const char *format, ...)
{
    va_list ap;
    va_start(ap, format);
    (void) vsnprintf(str + strlen(str), alloc_size - strlen(str), format, ap);
    va_end(ap);
}


static inline void str_vappendf(char *str, size_t alloc_size, const char *format, va_list ap)
{
    (void) vsnprintf(str + strlen(str), alloc_size - strlen(str), format, ap);
}


static inline void str_rstrip_char(char *str, char ch)
{
    if (strlen(str) != 0 && str[strlen(str) - 1] == ch) {
        str[strlen(str) - 1] = '\0';
    }
}

#endif /* _GPSD_STRFUNCS_H_ */
