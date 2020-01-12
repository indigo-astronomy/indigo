/* libgps_core.c -- client interface library for the gpsd daemon
 *
 * Core portion of client library.  Cals helpers to handle different eports.
 *
 * This file is Copyright (c) 2010-2018 by the GPSD project
 * SPDX-License-Identifier: BSD-2-clause
 */

#include "gpsd_config.h"  /* must be before all includes */

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdbool.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>
#include <stdarg.h>

#include "gpsd.h"
#include "libgps.h"
#include "gps_json.h"
#include "strfuncs.h"

#ifdef LIBGPS_DEBUG
int libgps_debuglevel = 0;

static FILE *debugfp;

void gps_enable_debug(int level, FILE * fp)
/* control the level and destination of debug trace messages */
{
    libgps_debuglevel = level;
    debugfp = fp;
#if defined(CLIENTDEBUG_ENABLE) && defined(SOCKET_EXPORT_ENABLE)
    json_enable_debug(level - DEBUG_JSON, fp);
#endif
}

void libgps_trace(int errlevel, const char *fmt, ...)
/* assemble command in printf(3) style */
{
    if (errlevel <= libgps_debuglevel) {
	char buf[BUFSIZ];
	va_list ap;

	(void)strlcpy(buf, "libgps: ", sizeof(buf));
	va_start(ap, fmt);
	str_vappendf(buf, sizeof(buf), fmt, ap);
	va_end(ap);

	(void)fputs(buf, debugfp);
    }
}
#else
// Functions defined as so to furfil the API but otherwise do nothing when built with debug capability turned off
void gps_enable_debug(int level UNUSED, FILE * fp UNUSED) {}
void libgps_trace(int errlevel UNUSED, const char *fmt UNUSED, ...){}
#endif /* LIBGPS_DEBUG */

#ifdef SOCKET_EXPORT_ENABLE
#define CONDITIONALLY_UNUSED
#else
#define CONDITIONALLY_UNUSED UNUSED
#endif /* SOCKET_EXPORT_ENABLE */

int gps_open(const char *host,
	     const char *port CONDITIONALLY_UNUSED,
	     struct gps_data_t *gpsdata)
{
    int status = -1;

    if (!gpsdata)
	return -1;

#ifdef SHM_EXPORT_ENABLE
    if (host != NULL && strcmp(host, GPSD_SHARED_MEMORY) == 0) {
	status = gps_shm_open(gpsdata);
	if (status == -1)
	    status = SHM_NOSHARED;
	else if (status == -2)
	    status = SHM_NOATTACH;
    }
#define USES_HOST
#endif /* SHM_EXPORT_ENABLE */

#ifdef DBUS_EXPORT_ENABLE
    if (host != NULL && strcmp(host, GPSD_DBUS_EXPORT) == 0) {
	status = gps_dbus_open(gpsdata);
	if (status != 0)
	    status = DBUS_FAILURE;
    }
#define USES_HOST
#endif /* DBUS_EXPORT_ENABLE */

#ifdef SOCKET_EXPORT_ENABLE
    if (status == -1) {
        status = gps_sock_open(host, port, gpsdata);
    }
#define USES_HOST
#endif /* SOCKET_EXPORT_ENABLE */

#ifndef USES_HOST
    (void)fprintf(stderr,
                  "No methods available for connnecting to %s!\n",
                  host);
#endif /* USES_HOST */
#undef USES_HOST

    gpsdata->set = 0;
    gpsdata->status = STATUS_NO_FIX;
    gpsdata->satellites_used = 0;
    gps_clear_att(&(gpsdata->attitude));
    gps_clear_dop(&(gpsdata->dop));
    gps_clear_fix(&(gpsdata->fix));

    return status;
}

#if defined(SHM_EXPORT_ENABLE) || defined(SOCKET_EXPORT_ENABLE)
#define CONDITIONALLY_UNUSED
#else
#define CONDITIONALLY_UNUSED	UNUSED
#endif

int gps_close(struct gps_data_t *gpsdata CONDITIONALLY_UNUSED)
/* close a gpsd connection */
{
    int status = -1;

    libgps_debug_trace((DEBUG_CALLS, "gps_close()\n"));

#ifdef SHM_EXPORT_ENABLE
    if (BAD_SOCKET((intptr_t)(gpsdata->gps_fd))) {
	gps_shm_close(gpsdata);
	status = 0;
    }
#endif /* SHM_EXPORT_ENABLE */

#ifdef SOCKET_EXPORT_ENABLE
    if (status == -1) {
        status = gps_sock_close(gpsdata);
    }
#endif /* SOCKET_EXPORT_ENABLE */

	return status;
}

/* read from a gpsd connection
 *
 * parameters:
 *    gps_data_t *gpsdata   -- structure for GPS data
 *    char *message         -- NULL, or optional buffer for received JSON
 *    int message_len       -- zero, or sizeof(message)
 */
int gps_read(struct gps_data_t *gpsdata CONDITIONALLY_UNUSED,
             char *message, int message_len)
{
    int status = -1;

    libgps_debug_trace((DEBUG_CALLS, "gps_read() begins\n"));
    if ((NULL != message) && (0 < message_len)) {
        /* be sure message is zero length */
        /* we do not memset() as this is time critical input path */
        *message = '\0';
    }

#ifdef SHM_EXPORT_ENABLE
    if (BAD_SOCKET((intptr_t)(gpsdata->gps_fd))) {
	status = gps_shm_read(gpsdata);
    }
#endif /* SHM_EXPORT_ENABLE */

#ifdef SOCKET_EXPORT_ENABLE
    if (status == -1 && !BAD_SOCKET((intptr_t)(gpsdata->gps_fd))) {
        status = gps_sock_read(gpsdata, message, message_len);
    }
#endif /* SOCKET_EXPORT_ENABLE */

    libgps_debug_trace((DEBUG_CALLS, "gps_read() -> %d (%s)\n",
			status, gps_maskdump(gpsdata->set)));

    return status;
}

int gps_send(struct gps_data_t *gpsdata CONDITIONALLY_UNUSED, const char *fmt CONDITIONALLY_UNUSED, ...)
/* send a command to the gpsd instance */
{
    int status = -1;
    char buf[BUFSIZ];
    va_list ap;

    va_start(ap, fmt);
    (void)vsnprintf(buf, sizeof(buf) - 2, fmt, ap);
    va_end(ap);
    if (buf[strlen(buf) - 1] != '\n')
	(void)strlcat(buf, "\n", sizeof(buf));

#ifdef SOCKET_EXPORT_ENABLE
    status = gps_sock_send(gpsdata, buf);
#endif /* SOCKET_EXPORT_ENABLE */

    return status;
}

int gps_stream(struct gps_data_t *gpsdata CONDITIONALLY_UNUSED,
	unsigned int flags CONDITIONALLY_UNUSED,
	void *d CONDITIONALLY_UNUSED)
{
    int status = -1;

#ifdef SOCKET_EXPORT_ENABLE
    /* cppcheck-suppress redundantAssignment */
    status = gps_sock_stream(gpsdata, flags, d);
#endif /* SOCKET_EXPORT_ENABLE */

    return status;
}

const char *gps_data(const struct gps_data_t *gpsdata CONDITIONALLY_UNUSED)
/* return the contents of the client data buffer */
{
    const char *bufp = NULL;

#ifdef SOCKET_EXPORT_ENABLE
    bufp = gps_sock_data(gpsdata);
#endif /* SOCKET_EXPORT_ENABLE */

    return bufp;
}

bool gps_waiting(const struct gps_data_t *gpsdata CONDITIONALLY_UNUSED, int timeout CONDITIONALLY_UNUSED)
/* is there input waiting from the GPS? */
/* timeout is in uSec */
{
    /* this is bogus, but I can't think of a better solution yet */
    bool waiting = true;

#ifdef SHM_EXPORT_ENABLE
    if ((intptr_t)(gpsdata->gps_fd) == SHM_PSEUDO_FD)
	waiting = gps_shm_waiting(gpsdata, timeout);
#endif /* SHM_EXPORT_ENABLE */

#ifdef SOCKET_EXPORT_ENABLE
    // cppcheck-suppress pointerPositive
    if ((intptr_t)(gpsdata->gps_fd) >= 0)
	waiting = gps_sock_waiting(gpsdata, timeout);
#endif /* SOCKET_EXPORT_ENABLE */

    return waiting;
}

int gps_mainloop(struct gps_data_t *gpsdata CONDITIONALLY_UNUSED,
		 int timeout CONDITIONALLY_UNUSED,
		 void (*hook)(struct gps_data_t *gpsdata) CONDITIONALLY_UNUSED)
{
    int status = -1;

    libgps_debug_trace((DEBUG_CALLS, "gps_mainloop() begins\n"));

#ifdef SHM_EXPORT_ENABLE
    if ((intptr_t)(gpsdata->gps_fd) == SHM_PSEUDO_FD)
	status = gps_shm_mainloop(gpsdata, timeout, hook);
#endif /* SHM_EXPORT_ENABLE */
#ifdef DBUS_EXPORT_ENABLE
    if ((intptr_t)(gpsdata->gps_fd) == DBUS_PSEUDO_FD)
	status = gps_dbus_mainloop(gpsdata, timeout, hook);
#endif /* DBUS_EXPORT_ENABLE */
#ifdef SOCKET_EXPORT_ENABLE
    if ((intptr_t)(gpsdata->gps_fd) >= 0)
	status = gps_sock_mainloop(gpsdata, timeout, hook);
#endif /* SOCKET_EXPORT_ENABLE */

    libgps_debug_trace((DEBUG_CALLS, "gps_mainloop() -> %d (%s)\n",
			status, gps_maskdump(gpsdata->set)));

    return status;
}

extern const char *gps_errstr(const int err)
{
    /*
     * We might add our own error codes in the future, e.g for
     * protocol compatibility checks
     */
#ifndef USE_QT
#ifdef SHM_EXPORT_ENABLE
    if (err == SHM_NOSHARED)
	return "no shared-memory segment or daemon not running";
    else if (err == SHM_NOATTACH)
	return "attach failed for unknown reason";
#endif /* SHM_EXPORT_ENABLE */
#ifdef DBUS_EXPORT_ENABLE
    if (err == DBUS_FAILURE)
	return "DBUS initialization failure";
#endif /* DBUS_EXPORT_ENABLE */
    return netlib_errstr(err);
#else
    static char buf[32];
    (void)snprintf(buf, sizeof(buf), "Qt error %d", err);
    return buf;
#endif
}

#ifdef LIBGPS_DEBUG
void libgps_dump_state(struct gps_data_t *collect)
{
    char ts_buf[TIMESPEC_LEN];

    /* no need to dump the entire state, this is a sanity check */
#ifndef USE_QT
    (void)fprintf(debugfp, "flags: (0x%04x) %s\n",
		  (unsigned int)collect->set, gps_maskdump(collect->set));
#endif
    if (collect->set & ONLINE_SET)
	(void)fprintf(debugfp, "ONLINE: %s\n",
                      timespec_str(&collect->online, ts_buf, sizeof(ts_buf)));
    if (collect->set & TIME_SET)
	(void)fprintf(debugfp, "TIME: %s\n",
                     timespec_str(&collect->fix.time, ts_buf, sizeof(ts_buf)));
    /* NOTE: %.7f needed for cm level accurate GPS */
    if (collect->set & LATLON_SET)
	(void)fprintf(debugfp, "LATLON: lat/lon: %.7lf %.7lf\n",
		      collect->fix.latitude, collect->fix.longitude);
    if (collect->set & ALTITUDE_SET)
	(void)fprintf(debugfp, "ALTITUDE: altHAE: %lf  U: climb: %lf\n",
		      collect->fix.altHAE, collect->fix.climb);
    if (collect->set & SPEED_SET)
	(void)fprintf(debugfp, "SPEED: %lf\n", collect->fix.speed);
    if (collect->set & TRACK_SET)
	(void)fprintf(debugfp, "TRACK: track: %lf\n", collect->fix.track);
    if (collect->set & MAGNETIC_TRACK_SET)
        (void)fprintf(debugfp, "MAGNETIC_TRACK: magtrack: %lf\n",
                      collect->fix.magnetic_track);
    if (collect->set & CLIMB_SET)
	(void)fprintf(debugfp, "CLIMB: climb: %lf\n", collect->fix.climb);
    if (collect->set & STATUS_SET) {
	const char *status_values[] = { "NO_FIX", "FIX", "DGPS_FIX" };
	(void)fprintf(debugfp, "STATUS: status: %d (%s)\n",
		      collect->status, status_values[collect->status]);
    }
    if (collect->set & MODE_SET) {
	const char *mode_values[] = { "", "NO_FIX", "MODE_2D", "MODE_3D" };
	(void)fprintf(debugfp, "MODE: mode: %d (%s)\n",
		      collect->fix.mode, mode_values[collect->fix.mode]);
    }
    if (collect->set & DOP_SET)
	(void)fprintf(debugfp,
		      "DOP: satellites %d, pdop=%lf, hdop=%lf, vdop=%lf\n",
		      collect->satellites_used, collect->dop.pdop,
		      collect->dop.hdop, collect->dop.vdop);
    if (collect->set & VERSION_SET)
	(void)fprintf(debugfp, "VERSION: release=%s rev=%s proto=%d.%d\n",
		      collect->version.release,
		      collect->version.rev,
		      collect->version.proto_major,
		      collect->version.proto_minor);
    if (collect->set & POLICY_SET)
	(void)fprintf(debugfp,
		      "POLICY: watcher=%s nmea=%s raw=%d scaled=%s timing=%s, split24=%s pps=%s, devpath=%s\n",
		      collect->policy.watcher ? "true" : "false",
		      collect->policy.nmea ? "true" : "false",
		      collect->policy.raw,
		      collect->policy.scaled ? "true" : "false",
		      collect->policy.timing ? "true" : "false",
		      collect->policy.split24 ? "true" : "false",
		      collect->policy.pps ? "true" : "false",
		      collect->policy.devpath);
    if (collect->set & SATELLITE_SET) {
	struct satellite_t *sp;

	(void)fprintf(debugfp, "SKY: satellites in view: %d\n",
		      collect->satellites_visible);
	for (sp = collect->skyview;
	     sp < collect->skyview + collect->satellites_visible;
	     sp++) {
	    (void)fprintf(debugfp, "  %2.2d: %4.1f %5.1f %3.0f %c\n",
			  sp->PRN, sp->elevation,
			  sp->azimuth, sp->ss,
			  sp->used ? 'Y' : 'N');
	}
    }
    if (collect->set & RAW_SET)
	(void)fprintf(debugfp, "RAW: got raw data\n");
    if (collect->set & DEVICE_SET)
	(void)fprintf(debugfp, "DEVICE: Device is '%s', driver is '%s'\n",
		      collect->dev.path, collect->dev.driver);
    if (collect->set & DEVICELIST_SET) {
	int i;
	(void)fprintf(debugfp, "DEVICELIST:%d devices:\n",
		      collect->devices.ndevices);
	for (i = 0; i < collect->devices.ndevices; i++) {
	    (void)fprintf(debugfp, "%d: path='%s' driver='%s'\n",
			  collect->devices.ndevices,
			  collect->devices.list[i].path,
			  collect->devices.list[i].driver);
	}
    }

}
#endif /* LIBGPS_DEBUG */

// end
