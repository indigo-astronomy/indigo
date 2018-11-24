/*
 * 1394-Based Digital Camera Control Library
 *
 * Error logging functions
 *
 * Written by Damien Douxchamps <ddouxchamps@users.sf.net> and
 *            Rudolf Leitgeb
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307 USA
 */

#include "log.h"
#include <stdarg.h>
#include <stdlib.h>

static void
default_errorlog_handler(dc1394log_t type, const char *message, void* user)
{
    fprintf(stderr, "libdc1394 error: %s\n", message);
}

static void
default_warninglog_handler(dc1394log_t type, const char *message, void* user)
{
    fprintf(stderr, "libdc1394 warning: %s\n", message);
}

static void
default_debuglog_handler(dc1394log_t type, const char *message, void* user)
{
    static int log_enabled = -1;
    if (log_enabled == -1) {
        if (getenv("DC1394_DEBUG") == NULL )
            log_enabled = 0;
        else
            log_enabled = 1;
    }

    if (log_enabled == 1)
        fprintf(stderr, "libdc1394 debug: %s\n", message);
}

static void(*system_errorlog_handler)(dc1394log_t type, const char *message, void* user) = default_errorlog_handler;
static void(*system_warninglog_handler)(dc1394log_t type, const char *message, void* user) = default_warninglog_handler;
static void(*system_debuglog_handler)(dc1394log_t type, const char *message, void* user) = default_debuglog_handler;
static void *errorlog_data = NULL;
static void *warninglog_data = NULL;
static void *debuglog_data = NULL;

dc1394error_t
dc1394_log_register_handler(dc1394log_t type, void(*log_handler)(dc1394log_t type, const char *message, void* user), void* user) {
    switch (type) {
    case DC1394_LOG_ERROR:
        system_errorlog_handler = log_handler;
        errorlog_data=user;
        return DC1394_SUCCESS;
    case DC1394_LOG_WARNING:
        system_warninglog_handler = log_handler;
        warninglog_data=user;
        return DC1394_SUCCESS;
    case DC1394_LOG_DEBUG:
        system_debuglog_handler = log_handler;
        debuglog_data=user;
        return DC1394_SUCCESS;
    default:
        return DC1394_INVALID_LOG_TYPE;
    }
}

dc1394error_t
dc1394_log_set_default_handler(dc1394log_t type) {
    switch (type) {
    case DC1394_LOG_ERROR:
        system_errorlog_handler = default_errorlog_handler;
        errorlog_data=NULL;
        return DC1394_SUCCESS;
    case DC1394_LOG_WARNING:
        system_warninglog_handler = default_warninglog_handler;
        warninglog_data=NULL;
        return DC1394_SUCCESS;
    case DC1394_LOG_DEBUG:
        system_debuglog_handler = default_debuglog_handler;
        debuglog_data=NULL;
        return DC1394_SUCCESS;
    default:
        return DC1394_INVALID_LOG_TYPE;
    }
}


void dc1394_log_error(const char *format,...)
{
    char string[1024];
    if (system_errorlog_handler != NULL) {
        va_list args;
        va_start(args, format);
        vsnprintf(string, sizeof(string), format, args);
        system_errorlog_handler(DC1394_LOG_ERROR, string, errorlog_data);
    }
}


void dc1394_log_warning(const char *format,...)
{
    char string[1024];
    if (system_warninglog_handler != NULL) {
        va_list args;
        va_start(args, format);
        vsnprintf(string, sizeof(string), format, args);
        system_warninglog_handler(DC1394_LOG_WARNING, string, warninglog_data);
    }
}


void dc1394_log_debug(const char *format,...)
{
    char string[1024];
    if (system_debuglog_handler != NULL) {
        va_list args;
        va_start(args, format);
        vsnprintf(string, sizeof(string), format, args);
        system_debuglog_handler(DC1394_LOG_DEBUG, string, debuglog_data);
    }
}

