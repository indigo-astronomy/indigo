/*
 * This file is Copyright (c)2010-2018 by the GPSD project
 * SPDX-License-Identifier: BSD-2-clause
 */

#include "gpsd_config.h"  /* must be before all includes */

#include <errno.h>
#include <libgen.h>
#include <math.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include "gps.h"
#include "libgps.h"

#if defined(DBUS_EXPORT_ENABLE)
#include <syslog.h>

struct privdata_t
{
    void (*handler)(struct gps_data_t *);
};

#include <dbus/dbus.h>

/*
 * Unpleasant that we have to declare a static context pointer here - means
 * you can't have multiple DBUS sessions open (not that this matters
 * much in practice). The problem is the DBUS API lacks some hook
 * arguments that it ought to have.
 */
static struct gps_data_t *share_gpsdata;
static DBusConnection *connection;

static DBusHandlerResult handle_gps_fix(DBusMessage * message)
{
    DBusError error;
    const char *gpsd_devname = NULL;

    dbus_error_init(&error);

    dbus_message_get_args(message,
			  &error,
			  DBUS_TYPE_DOUBLE, &share_gpsdata->fix.time,
			  DBUS_TYPE_INT32, &share_gpsdata->fix.mode,
			  DBUS_TYPE_DOUBLE, &share_gpsdata->fix.ept,
			  DBUS_TYPE_DOUBLE, &share_gpsdata->fix.latitude,
			  DBUS_TYPE_DOUBLE, &share_gpsdata->fix.longitude,
			  DBUS_TYPE_DOUBLE, &share_gpsdata->fix.eph,
                          /* The dbus doc does not seem to specify
                           * altHAE or altMSL */
			  DBUS_TYPE_DOUBLE, &share_gpsdata->fix.altHAE,
			  DBUS_TYPE_DOUBLE, &share_gpsdata->fix.epv,
			  DBUS_TYPE_DOUBLE, &share_gpsdata->fix.track,
			  DBUS_TYPE_DOUBLE, &share_gpsdata->fix.epd,
			  DBUS_TYPE_DOUBLE, &share_gpsdata->fix.speed,
			  DBUS_TYPE_DOUBLE, &share_gpsdata->fix.eps,
			  DBUS_TYPE_DOUBLE, &share_gpsdata->fix.climb,
			  DBUS_TYPE_DOUBLE, &share_gpsdata->fix.epc,
			  DBUS_TYPE_STRING, &gpsd_devname, DBUS_TYPE_INVALID);

    if (share_gpsdata->fix.mode > MODE_NO_FIX )
	share_gpsdata->status = STATUS_FIX;
    else
	share_gpsdata->status = STATUS_NO_FIX;

    dbus_error_free(&error);

    PRIVATE(share_gpsdata)->handler(share_gpsdata);
    return DBUS_HANDLER_RESULT_HANDLED;
}

/*
 * Message dispatching function
 *
 */
static DBusHandlerResult signal_handler(DBusConnection * connection UNUSED,
					DBusMessage * message,
                                        void *user_data UNUSED)
{
    if (dbus_message_is_signal(message, "org.gpsd", "fix"))
	return handle_gps_fix(message);
    /*
     * ignore all other messages
     */

    return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
}

int gps_dbus_open(struct gps_data_t *gpsdata)
{
    DBusError error;

    gpsdata->privdata = (void *)malloc(sizeof(struct privdata_t));
    if (gpsdata->privdata == NULL)
	return -1;

    dbus_error_init(&error);
    connection = dbus_bus_get(DBUS_BUS_SYSTEM, &error);
    if (dbus_error_is_set(&error)) {
	syslog(LOG_CRIT, "%s: %s", error.name, error.message);
	dbus_error_free(&error);
	return 3;
    }

    dbus_bus_add_match(connection, "type='signal'", &error);
    if (dbus_error_is_set(&error)) {
	syslog(LOG_CRIT, "unable to add match for signals %s: %s", error.name,
	       error.message);
	dbus_error_free(&error);
	return 4;
    }

    if (!dbus_connection_add_filter
	(connection, (DBusHandleMessageFunction) signal_handler, NULL,
	 NULL)) {
	syslog(LOG_CRIT, "unable to register filter with the connection");
	return 5;
    }

#ifndef USE_QT
    gpsdata->gps_fd = DBUS_PSEUDO_FD;
#else
    gpsdata->gps_fd = (void *)(intptr_t)DBUS_PSEUDO_FD;
#endif /* USE_QT */
    share_gpsdata = gpsdata;
    return 0;
}

int gps_dbus_mainloop(struct gps_data_t *gpsdata,
		       int timeout,
		       void (*hook)(struct gps_data_t *))
/* run a DBUS main loop with a specified handler */
{
    share_gpsdata = gpsdata;
    PRIVATE(share_gpsdata)->handler = (void (*)(struct gps_data_t *))hook;
    for (;;)
	if (dbus_connection_read_write_dispatch(connection, (int)(timeout/1000)) != TRUE)
	    return -1;
    return 0;
}

#endif /* defined(DBUS_EXPORT_ENABLE) */
