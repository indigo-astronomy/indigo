/* libgpsd_core.c -- manage access to sensors
 *
 * Access to the driver layer goes through the entry points in this file.
 * The idea is to present a session as an abstraction from which you get
 * fixes (and possibly other data updates) by calling gpsd_multipoll(). The
 * rest is setup and teardown. (For backward compatibility the older gpsd_poll()
 * entry point has been retained.)
 *
 * This file is Copyright (c) 2010-2018 by the GPSD project
 * SPDX-License-Identifier: BSD-2-clause
 */

#include "gpsd_config.h"  /* must be before all includes */

#include <assert.h>
#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <libgen.h>
#include <math.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <syslog.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

#include "gpsd.h"
#include "matrix.h"
#include "strfuncs.h"
#include "timespec.h"
#if defined(NMEA2000_ENABLE)
#include "driver_nmea2000.h"
#endif /* defined(NMEA2000_ENABLE) */

ssize_t gpsd_write(struct gps_device_t *session,
		   const char *buf,
		   const size_t len)
/* pass low-level data to devices straight through */
{
    return session->context->serial_write(session, buf, len);
}

static void basic_report(const char *buf)
{
    (void)fputs(buf, stderr);
}

void errout_reset(struct gpsd_errout_t *errout)
{
    errout->debug = LOG_SHOUT;
    errout->report = basic_report;
}

static pthread_mutex_t report_mutex;

void gpsd_acquire_reporting_lock(void)
{
    int err;
    err = pthread_mutex_lock(&report_mutex);
    if ( 0 != err ) {
        /* POSIX says pthread_mutex_lock() should only fail if the
        thread holding the lock has died.  Best for gppsd to just die
        because things are FUBAR. */

	(void) fprintf(stderr,"pthread_mutex_lock() failed: %s\n",
            strerror(errno));
	exit(EXIT_FAILURE);
    }
}

void gpsd_release_reporting_lock(void)
{
    int err;
    err = pthread_mutex_unlock(&report_mutex);
    if ( 0 != err ) {
        /* POSIX says pthread_mutex_unlock() should only fail when
        trying to unlock a lock that does not exist, or is not owned by
        this thread.  This should never happen, so best for gpsd to die
        because things are FUBAR. */

	(void) fprintf(stderr,"pthread_mutex_unlock() failed: %s\n",
            strerror(errno));
	exit(EXIT_FAILURE);
    }
}

#ifndef SQUELCH_ENABLE
static void visibilize(char *outbuf, size_t outlen,
		       const char *inbuf, size_t inlen)
{
    const char *sp;

    outbuf[0] = '\0';
    for (sp = inbuf; sp < inbuf + inlen && strlen(outbuf)+6 < outlen; sp++)
	if (isprint((unsigned char) *sp) || (sp[0] == '\n' && sp[1] == '\0')
	  || (sp[0] == '\r' && sp[2] == '\0'))
	    (void)snprintf(outbuf + strlen(outbuf), 2, "%c", *sp);
	else
	    (void)snprintf(outbuf + strlen(outbuf), 6, "\\x%02x",
			   0x00ff & (unsigned)*sp);
}
#endif /* !SQUELCH_ENABLE */


static void gpsd_vlog(const int errlevel,
                      const struct gpsd_errout_t *errout,
                      char *outbuf, size_t outlen,
                      const char *fmt, va_list ap)
/* assemble msg in vprintf(3) style, use errout hook or syslog for delivery */
{
#ifdef SQUELCH_ENABLE
    (void)errout;
    (void)errlevel;
    (void)fmt;
#else
    char buf[BUFSIZ];
    char *err_str;

    // errout should never be NULL, but some code analyzers complain anyway
    if (NULL == errout ||
        errout->debug < errlevel) {
        return;
    }

    gpsd_acquire_reporting_lock();
    switch ( errlevel ) {
    case LOG_ERROR:
            err_str = "ERROR: ";
            break;
    case LOG_SHOUT:
            err_str = "SHOUT: ";
            break;
    case LOG_WARN:
            err_str = "WARN: ";
            break;
    case LOG_CLIENT:
            err_str = "CLIENT: ";
            break;
    case LOG_INF:
            err_str = "INFO: ";
            break;
    case LOG_DATA:
            err_str = "DATA: ";
            break;
    case LOG_PROG:
            err_str = "PROG: ";
            break;
    case LOG_IO:
            err_str = "IO: ";
            break;
    case LOG_SPIN:
            err_str = "SPIN: ";
            break;
    case LOG_RAW:
            err_str = "RAW: ";
            break;
    default:
            err_str = "UNK: ";
    }

    assert(errout->label != NULL);
    (void)strlcpy(buf, errout->label, sizeof(buf));
    (void)strlcat(buf, ":", sizeof(buf));
    (void)strlcat(buf, err_str, sizeof(buf));
    str_vappendf(buf, sizeof(buf), fmt, ap);

    visibilize(outbuf, outlen, buf, strlen(buf));

    if (getpid() == getsid(getpid()))
        syslog((errlevel <= LOG_SHOUT) ? LOG_ERR : LOG_NOTICE, "%s", outbuf);
    else if (errout->report != NULL)
        errout->report(outbuf);
    else
        (void)fputs(outbuf, stderr);
    gpsd_release_reporting_lock();
#endif /* !SQUELCH_ENABLE */
}

/* assemble msg in printf(3) style, use errout hook or syslog for delivery */
void gpsd_log(const int errlevel, const struct gpsd_errout_t *errout,
              const char *fmt, ...)
{
    char buf[BUFSIZ];
    va_list ap;

    buf[0] = '\0';
    va_start(ap, fmt);
    gpsd_vlog(errlevel, errout, buf, sizeof(buf), fmt, ap);
    va_end(ap);
}

const char *gpsd_prettydump(struct gps_device_t *session)
/* dump the current packet in a form optimised for eyeballs */
{
    return gpsd_packetdump(session->msgbuf, sizeof(session->msgbuf),
			   (char *)session->lexer.outbuffer,
			   session->lexer.outbuflen);
}



/* Define the possible hook strings here so we can get the length */
#define HOOK_ACTIVATE "ACTIVATE"
#define HOOK_DEACTIVATE "DEACTIVATE"

#define HOOK_CMD_MAX (sizeof(DEVICEHOOKPATH) + GPS_PATH_MAX \
                      + sizeof(HOOK_DEACTIVATE))

static void gpsd_run_device_hook(struct gpsd_errout_t *errout,
				 char *device_name, char *hook)
{
    struct stat statbuf;

    if (stat(DEVICEHOOKPATH, &statbuf) == -1)
	GPSD_LOG(LOG_PROG, errout,
		 "no %s present, skipped running %s hook\n",
		 DEVICEHOOKPATH, hook);
    else {
	int status;
	char buf[HOOK_CMD_MAX];
	(void)snprintf(buf, sizeof(buf), "%s %s %s",
		       DEVICEHOOKPATH, device_name, hook);
	GPSD_LOG(LOG_INF, errout, "running %s\n", buf);
	status = system(buf);
	if (status == -1)
	    GPSD_LOG(LOG_ERROR, errout, "error running %s\n", buf);
	else
	    GPSD_LOG(LOG_INF, errout,
		     "%s returned %d\n", DEVICEHOOKPATH,
		     WEXITSTATUS(status));
    }
}

int gpsd_switch_driver(struct gps_device_t *session, char *type_name)
{
    const struct gps_type_t **dp;
    bool first_sync = (session->device_type != NULL);
    unsigned int i;

    if (first_sync && strcmp(session->device_type->type_name, type_name) == 0)
	return 0;

    GPSD_LOG(LOG_PROG, &session->context->errout,
	     "switch_driver(%s) called...\n", type_name);
    for (dp = gpsd_drivers, i = 0; *dp; dp++, i++)
	if (strcmp((*dp)->type_name, type_name) == 0) {
	    GPSD_LOG(LOG_PROG, &session->context->errout,
		     "selecting %s driver...\n",
		     (*dp)->type_name);
	    gpsd_assert_sync(session);
	    session->device_type = *dp;
	    session->driver_index = i;
#ifdef RECONFIGURE_ENABLE
	    session->gpsdata.dev.mincycle = session->device_type->min_cycle;
#endif /* RECONFIGURE_ENABLE */
	    /* reconfiguration might be required */
	    if (first_sync && session->device_type->event_hook != NULL)
		session->device_type->event_hook(session,
						 event_driver_switch);
#ifdef RECONFIGURE_ENABLE
	    if (STICKY(*dp))
		session->last_controller = *dp;
#endif /* RECONFIGURE_ENABLE */
	    return 1;
	}
    GPSD_LOG(LOG_ERROR, &session->context->errout,
	     "invalid GPS type \"%s\".\n", type_name);
    return 0;
}

void gps_context_init(struct gps_context_t *context,
		      const char *label)
{
    (void)memset(context, '\0', sizeof(struct gps_context_t));
    //context.readonly = false;
    context->leap_notify    = LEAP_NOWARNING;
    context->serial_write = gpsd_serial_write;

    errout_reset(&context->errout);
    context->errout.label = (char *)label;

    (void)pthread_mutex_init(&report_mutex, NULL);
}

void gpsd_init(struct gps_device_t *session, struct gps_context_t *context,
	       const char *device)
/* initialize GPS polling */
{
    if (device != NULL)
	(void)strlcpy(session->gpsdata.dev.path, device,
		      sizeof(session->gpsdata.dev.path));
    session->device_type = NULL;	/* start by hunting packets */
#ifdef RECONFIGURE_ENABLE
    session->last_controller = NULL;
#endif /* RECONFIGURE_ENABLE */
    session->observed = 0;
    session->sourcetype = source_unknown;	/* gpsd_open() sets this */
    session->servicetype = service_unknown;	/* gpsd_open() sets this */
    session->context = context;
    memset(session->subtype, 0, sizeof(session->subtype));
    memset(session->subtype1, 0, sizeof(session->subtype1));
#ifdef NMEA0183_ENABLE
    memset(&(session->nmea), 0, sizeof(session->nmea));
#endif /* NMEA0183_ENABLE */
    gps_clear_fix(&session->gpsdata.fix);
    gps_clear_fix(&session->newdata);
    gps_clear_fix(&session->lastfix);
    gps_clear_fix(&session->oldfix);
    session->gpsdata.set = 0;
    gps_clear_att(&session->gpsdata.attitude);
    gps_clear_dop(&session->gpsdata.dop);
    session->gpsdata.dev.mincycle.tv_sec = 1;
    session->gpsdata.dev.mincycle.tv_nsec = 0;
    session->gpsdata.dev.cycle.tv_sec = 1;
    session->gpsdata.dev.cycle.tv_nsec = 0;
    session->sor.tv_sec = 0;
    session->sor.tv_nsec = 0;
    session->chars = 0;
    /* tty-level initialization */
    gpsd_tty_init(session);
    /* necessary in case we start reading in the middle of a GPGSV sequence */
    gpsd_zero_satellites(&session->gpsdata);

    /* initialize things for the packet parser */
    packet_reset(&session->lexer);
}

/* temporarily release the GPS device */
void gpsd_deactivate(struct gps_device_t *session)
{
#ifdef RECONFIGURE_ENABLE
    if (!session->context->readonly
	&& session->device_type != NULL
	&& session->device_type->event_hook != NULL) {
	session->device_type->event_hook(session, event_deactivate);
    }
#endif /* RECONFIGURE_ENABLE */
    GPSD_LOG(LOG_INF, &session->context->errout,
	     "closing GPS=%s (%d)\n",
	     session->gpsdata.dev.path, session->gpsdata.gps_fd);
#if defined(NMEA2000_ENABLE)
    if (session->sourcetype == source_can)
        (void)nmea2000_close(session);
    else
#endif /* of defined(NMEA2000_ENABLE) */
        (void)gpsd_close(session);
    if (session->mode == O_OPTIMIZE)
	gpsd_run_device_hook(&session->context->errout,
			     session->gpsdata.dev.path,
			     HOOK_DEACTIVATE);
    /* tell any PPS-watcher thread to die */
    session->pps_thread.report_hook = NULL;
    /* mark it inactivated */
    session->gpsdata.online.tv_sec = 0;
    session->gpsdata.online.tv_nsec = 0;
}

static void ppsthread_log(volatile struct pps_thread_t *pps_thread,
			  int loglevel, const char *fmt, ...)
/* shim function to decouple PPS monitor code from the session structure */
{
    struct gps_device_t *device = (struct gps_device_t *)pps_thread->context;
    char buf[BUFSIZ];
    va_list ap;

    switch (loglevel) {
    case THREAD_ERROR:
	loglevel = LOG_ERROR;
	break;
    case THREAD_WARN:
	loglevel = LOG_WARN;
	break;
    case THREAD_INF:
	loglevel = LOG_INF;
	break;
    case THREAD_PROG:
	loglevel = LOG_PROG;
	break;
    case THREAD_RAW:
	loglevel = LOG_RAW;
	break;
    }

    buf[0] = '\0';
    va_start(ap, fmt);
    gpsd_vlog(loglevel, &device->context->errout, buf, sizeof(buf), fmt, ap);
    va_end(ap);
}


void gpsd_clear(struct gps_device_t *session)
/* device has been opened - clear its storage for use */
{
    (void)clock_gettime(CLOCK_REALTIME, &session->gpsdata.online);
    lexer_init(&session->lexer);
    session->lexer.errout = session->context->errout;
    // session->gpsdata.online = 0;
    gps_clear_att(&session->gpsdata.attitude);
    gps_clear_dop(&session->gpsdata.dop);
    gps_clear_fix(&session->gpsdata.fix);
    session->gpsdata.status = STATUS_NO_FIX;
    session->releasetime = (time_t)0;
    session->badcount = 0;

    /* clear the private data union */
    memset( (void *)&session->driver, '\0', sizeof(session->driver));
    /* set up the context structure for the PPS thread monitor */
    memset((void *)&session->pps_thread, 0, sizeof(session->pps_thread));
    session->pps_thread.devicefd = session->gpsdata.gps_fd;
    session->pps_thread.devicename = session->gpsdata.dev.path;
    session->pps_thread.log_hook = ppsthread_log;
    session->pps_thread.context = (void *)session;

    session->opentime = time(NULL);
}

static int parse_uri_dest(struct gps_device_t *session, char *s,
	char **host, char **service)
/* split s into host and service parts
 * if service is not specified, *service is assigned to NULL
 * return: -1 on error, 0 otherwise
 */
{
	if (s[0] != '[') {
		*host = s;
		s = strchr(s, ':');
	} else { /* IPv6 literal */
		char *cb = strchr(s, ']');
		if (!cb || (cb[1] && cb[1] != ':')) {
			GPSD_LOG(LOG_ERROR, &session->context->errout,
				"Malformed URI specified.\n");
			return -1;
		}
		*cb = '\0';
		*host = s + 1;
		s = cb + 1;
	}
	if (s && s[0] && s[1]) {
		*s = '\0';
		*service = s + 1;
	} else
		*service = NULL;
	return 0;
}

int gpsd_open(struct gps_device_t *session)
/* open a device for access to its data *
 * return: the opened file descriptor
 *         PLACEHOLDING_FD - for /dev/ppsX
 *         UNALLOCATED_FD - for open failure
 *         -1 - for open failure
 */
{
#ifdef NETFEED_ENABLE
    /* special case: source may be a URI to a remote GNSS or DGPS service */
    if (netgnss_uri_check(session->gpsdata.dev.path)) {
	session->gpsdata.gps_fd = netgnss_uri_open(session,
						   session->gpsdata.dev.path);
	session->sourcetype = source_tcp;
	GPSD_LOG(LOG_SPIN, &session->context->errout,
		 "netgnss_uri_open(%s) returns socket on fd %d\n",
		 session->gpsdata.dev.path, session->gpsdata.gps_fd);
	return session->gpsdata.gps_fd;
    /* otherwise, could be an TCP data feed */
    } else if (str_starts_with(session->gpsdata.dev.path, "tcp://")) {
	char server[GPS_PATH_MAX], *host, *port;
	socket_t dsock;
	(void)strlcpy(server, session->gpsdata.dev.path + 6, sizeof(server));
	INVALIDATE_SOCKET(session->gpsdata.gps_fd);
	if (parse_uri_dest(session, server, &host, &port) == -1 || !port) {
	    GPSD_LOG(LOG_ERROR, &session->context->errout,
		     "Missing service in TCP feed spec.\n");
	    return -1;
	}
	GPSD_LOG(LOG_INF, &session->context->errout,
		 "opening TCP feed at %s, port %s.\n", host,
		 port);
	if ((dsock = netlib_connectsock(AF_UNSPEC, host, port, "tcp")) < 0) {
	    GPSD_LOG(LOG_ERROR, &session->context->errout,
		     "TCP device open error %s.\n",
		     netlib_errstr(dsock));
	    return -1;
	} else
	    GPSD_LOG(LOG_SPIN, &session->context->errout,
		     "TCP device opened on fd %d\n", dsock);
	session->gpsdata.gps_fd = dsock;
	session->sourcetype = source_tcp;
	return session->gpsdata.gps_fd;
    /* or could be UDP */
    } else if (str_starts_with(session->gpsdata.dev.path, "udp://")) {
	char server[GPS_PATH_MAX], *host, *port;
	socket_t dsock;
	(void)strlcpy(server, session->gpsdata.dev.path + 6, sizeof(server));
	INVALIDATE_SOCKET(session->gpsdata.gps_fd);
	if (parse_uri_dest(session, server, &host, &port) == -1 || !port) {
	    GPSD_LOG(LOG_ERROR, &session->context->errout,
		     "Missing service in UDP feed spec.\n");
	    return -1;
	}
	GPSD_LOG(LOG_INF, &session->context->errout,
		 "opening UDP feed at %s, port %s.\n", host,
		 port);
	if ((dsock = netlib_connectsock(AF_UNSPEC, host, port, "udp")) < 0) {
	    GPSD_LOG(LOG_ERROR, &session->context->errout,
		     "UDP device open error %s.\n",
		     netlib_errstr(dsock));
	    return -1;
	} else
	    GPSD_LOG(LOG_SPIN, &session->context->errout,
		     "UDP device opened on fd %d\n", dsock);
	session->gpsdata.gps_fd = dsock;
	session->sourcetype = source_udp;
	return session->gpsdata.gps_fd;
    }
#endif /* NETFEED_ENABLE */
#ifdef PASSTHROUGH_ENABLE
    if (str_starts_with(session->gpsdata.dev.path, "gpsd://")) {
	char server[GPS_PATH_MAX], *host, *port;
	socket_t dsock;
	(void)strlcpy(server, session->gpsdata.dev.path + 7, sizeof(server));
	INVALIDATE_SOCKET(session->gpsdata.gps_fd);
	if (parse_uri_dest(session, server, &host, &port) == -1)
		return -1;
	if (!port)
	    port = DEFAULT_GPSD_PORT;
	GPSD_LOG(LOG_INF, &session->context->errout,
		 "opening remote gpsd feed at %s, port %s.\n",
		 host, port);
	if ((dsock = netlib_connectsock(AF_UNSPEC, host, port, "tcp")) < 0) {
	    GPSD_LOG(LOG_ERROR, &session->context->errout,
		     "remote gpsd device open error %s.\n",
		     netlib_errstr(dsock));
	    return -1;
	} else
	    GPSD_LOG(LOG_SPIN, &session->context->errout,
		     "remote gpsd feed opened on fd %d\n", dsock);
	/* watch to remote is issued when WATCH is */
	session->gpsdata.gps_fd = dsock;
	session->sourcetype = source_gpsd;
	return session->gpsdata.gps_fd;
    }
#endif /* PASSTHROUGH_ENABLE */
#if defined(NMEA2000_ENABLE)
    if (str_starts_with(session->gpsdata.dev.path, "nmea2000://")) {
        return nmea2000_open(session);
    }
#endif /* defined(NMEA2000_ENABLE) */
    /* fall through to plain serial open */
    /* could be a naked /dev/ppsX */
    return gpsd_serial_open(session);
}

int gpsd_activate(struct gps_device_t *session, const int mode)
/* acquire a connection to the GPS device */
{
    if (mode == O_OPTIMIZE)
	gpsd_run_device_hook(&session->context->errout,
			     session->gpsdata.dev.path, HOOK_ACTIVATE);
    session->gpsdata.gps_fd = gpsd_open(session);
    if (mode != O_CONTINUE)
	session->mode = mode;

    // cppcheck-suppress pointerLessThanZero
    if (session->gpsdata.gps_fd < 0) {
        /* return could be -1, PLACEHOLDING_FD, of UNALLOCATED_FD */
        if ( PLACEHOLDING_FD == session->gpsdata.gps_fd ) {
            /* it is /dev/ppsX, need to set devicename, etc. */
	    gpsd_clear(session);
        }
	return session->gpsdata.gps_fd;
    }

#ifdef NON_NMEA0183_ENABLE
    /* if it's a sensor, it must be probed */
    if ((session->servicetype == service_sensor) &&
	(session->sourcetype != source_can)) {
	const struct gps_type_t **dp;

	for (dp = gpsd_drivers; *dp; dp++) {
	    if ((*dp)->probe_detect != NULL) {
		GPSD_LOG(LOG_PROG, &session->context->errout,
			 "Probing \"%s\" driver...\n",
			 (*dp)->type_name);
		/* toss stale data */
		(void)tcflush(session->gpsdata.gps_fd, TCIOFLUSH);
		if ((*dp)->probe_detect(session) != 0) {
		    GPSD_LOG(LOG_PROG, &session->context->errout,
			     "Probe found \"%s\" driver...\n",
			     (*dp)->type_name);
		    session->device_type = *dp;
		    gpsd_assert_sync(session);
		    goto foundit;
		} else
		    GPSD_LOG(LOG_PROG, &session->context->errout,
			     "Probe not found \"%s\" driver...\n",
			     (*dp)->type_name);
	    }
	}
	GPSD_LOG(LOG_PROG, &session->context->errout,
		 "no probe matched...\n");
    }
foundit:
#endif /* NON_NMEA0183_ENABLE */

    gpsd_clear(session);
    GPSD_LOG(LOG_INF, &session->context->errout,
	     "gpsd_activate(%d): activated GPS (fd %d)\n",
	     session->mode, session->gpsdata.gps_fd);
    /*
     * We might know the device's type, but we shouldn't assume it has
     * retained its settings.  A revert hook might well have undone
     * them on the previous close.  Fire a reactivate event so drivers
     * can do something about this if they choose.
     */
    if (session->device_type != NULL
	&& session->device_type->event_hook != NULL)
	session->device_type->event_hook(session, event_reactivate);
    return session->gpsdata.gps_fd;
}


/*****************************************************************************

Carl Carter of SiRF supplied this algorithm for computing DOPs from
a list of visible satellites (some typos corrected)...

For satellite n, let az(n) = azimuth angle from North and el(n) be elevation.
Let:

    a(k, 1) = sin az(k) * cos el(k)
    a(k, 2) = cos az(k) * cos el(k)
    a(k, 3) = sin el(k)

Then form the line-of-sight matrix A for satellites used in the solution:

    | a(1,1) a(1,2) a(1,3) 1 |
    | a(2,1) a(2,2) a(2,3) 1 |
    |   :       :      :   : |
    | a(n,1) a(n,2) a(n,3) 1 |

And its transpose A~:

    |a(1, 1) a(2, 1) .  .  .  a(n, 1) |
    |a(1, 2) a(2, 2) .  .  .  a(n, 2) |
    |a(1, 3) a(2, 3) .  .  .  a(n, 3) |
    |    1       1   .  .  .     1    |

Compute the covariance matrix (A~*A)^-1, which is guaranteed symmetric:

    | s(x)^2    s(x)*s(y)  s(x)*s(z)  s(x)*s(t) |
    | s(y)*s(x) s(y)^2     s(y)*s(z)  s(y)*s(t) |
    | s(z)*s(x) s(z)*s(y)  s(z)^2     s(z)*s(t) |
    | s(t)*s(x) s(t)*s(y)  s(t)*s(z)  s(t)^2    |

Then:

GDOP = sqrt(s(x)^2 + s(y)^2 + s(z)^2 + s(t)^2)
TDOP = sqrt(s(t)^2)
PDOP = sqrt(s(x)^2 + s(y)^2 + s(z)^2)
HDOP = sqrt(s(x)^2 + s(y)^2)
VDOP = sqrt(s(z)^2)

Here's how we implement it...

First, each compute element P(i,j) of the 4x4 product A~*A.
If S(k=1,k=n): f(...) is the sum of f(...) as k varies from 1 to n, then
applying the definition of matrix product tells us:

P(i,j) = S(k=1,k=n): B(i, k) * A(k, j)

But because B is the transpose of A, this reduces to

P(i,j) = S(k=1,k=n): A(k, i) * A(k, j)

This is not, however, the entire algorithm that SiRF uses.  Carl writes:

> As you note, with rounding accounted for, most values agree exactly, and
> those that don't agree our number is higher.  That is because we
> deweight some satellites and account for that in the DOP calculation.
> If a satellite is not used in a solution at the same weight as others,
> it should not contribute to DOP calculation at the same weight.  So our
> internal algorithm does a compensation for that which you would have no
> way to duplicate on the outside since we don't output the weighting
> factors.  In fact those are not even available to API users.

Queried about the deweighting, Carl says:

> In the SiRF tracking engine, each satellite track is assigned a quality
> value based on the tracker's estimate of that signal.  It includes C/No
> estimate, ability to hold onto the phase, stability of the I vs. Q phase
> angle, etc.  The navigation algorithm then ranks all the tracks into
> quality order and selects which ones to use in the solution and what
> weight to give those used in the solution.  The process is actually a
> bit of a "trial and error" method -- we initially use all available
> tracks in the solution, then we sequentially remove the lowest quality
> ones until the solution stabilizes.  The weighting is inherent in the
> Kalman filter algorithm.  Once the solution is stable, the DOP is
> computed from those SVs used, and there is an algorithm that looks at
> the quality ratings and determines if we need to deweight any.
> Likewise, if we use altitude hold mode for a 3-SV solution, we deweight
> the phantom satellite at the center of the Earth.

So we cannot exactly duplicate what SiRF does internally.  We'll leave
HDOP alone and use our computed values for VDOP and PDOP.  Note, this
may have to change in the future if this code is used by a non-SiRF
driver.

******************************************************************************/


static gps_mask_t fill_dop(const struct gpsd_errout_t *errout,
			   const struct gps_data_t * gpsdata,
			   struct dop_t * dop)
{
    double prod[4][4];
    double inv[4][4];
    double satpos[MAXCHANNELS][4];
    double xdop, ydop, hdop, vdop, pdop, tdop, gdop;
    int i, j, k, n;

    memset(satpos, 0, sizeof(satpos));

    for (n = k = 0; k < gpsdata->satellites_visible; k++) {
        if (!gpsdata->skyview[k].used) {
             /* skip unused sats */
             continue;
        }
        if (1 > gpsdata->skyview[k].PRN) {
             /* skip bad PRN */
             continue;
        }
	if (0 == isfinite(gpsdata->skyview[k].azimuth) ||
            0 > gpsdata->skyview[k].azimuth ||
            359 < gpsdata->skyview[k].azimuth) {
             /* skip bad azimuth */
             continue;
        }
	if (0 == isfinite(gpsdata->skyview[k].elevation) ||
            90 < fabs(gpsdata->skyview[k].elevation)) {
             /* skip bad elevation */
             continue;
        }
        const struct satellite_t *sp = &gpsdata->skyview[k];
        satpos[n][0] = sin(sp->azimuth * DEG_2_RAD)
            * cos(sp->elevation * DEG_2_RAD);
        satpos[n][1] = cos(sp->azimuth * DEG_2_RAD)
            * cos(sp->elevation * DEG_2_RAD);
        satpos[n][2] = sin(sp->elevation * DEG_2_RAD);
        satpos[n][3] = 1;
        GPSD_LOG(LOG_INF, errout, "PRN=%3d az=%.1f ael%.1f (%f, %f, %f)\n",
                 gpsdata->skyview[k].PRN,
                 gpsdata->skyview[k].azimuth,
                 gpsdata->skyview[k].elevation,
                 satpos[n][0], satpos[n][1], satpos[n][2]);
        n++;
    }
    /* can't use gpsdata->satellites_used as that is a counter for xxGSA,
     * and gets cleared at odd times */
    GPSD_LOG(LOG_INF, errout, "Sats used (%d):\n", n);

    /* If we don't have 4 satellites then we don't have enough information to calculate DOPS */
    if (n < 4) {
#ifdef __UNUSED__
	GPSD_LOG(LOG_RAW, errout,
		 "Not enough satellites available %d < 4:\n",
		 n);
#endif /* __UNUSED__ */
	return 0;		/* Is this correct return code here? or should it be ERROR_SET */
    }

    memset(prod, 0, sizeof(prod));
    memset(inv, 0, sizeof(inv));

#ifdef __UNUSED__
    GPSD_LOG(LOG_INF, errout, "Line-of-sight matrix:\n");
    for (k = 0; k < n; k++) {
	GPSD_LOG(LOG_INF, errout, "%f %f %f %f\n",
		 satpos[k][0], satpos[k][1], satpos[k][2], satpos[k][3]);
    }
#endif /* __UNUSED__ */

    for (i = 0; i < 4; ++i) {	//< rows
	for (j = 0; j < 4; ++j) {	//< cols
	    prod[i][j] = 0.0;
	    for (k = 0; k < n; ++k) {
		prod[i][j] += satpos[k][i] * satpos[k][j];
	    }
	}
    }

#ifdef __UNUSED__
    GPSD_LOG(LOG_INF, errout, "product:\n");
    for (k = 0; k < 4; k++) {
	GPSD_LOG(LOG_INF, errout, "%f %f %f %f\n",
		 prod[k][0], prod[k][1], prod[k][2], prod[k][3]);
    }
#endif /* __UNUSED__ */

    if (matrix_invert(prod, inv)) {
#ifdef __UNUSED__
	/*
	 * Note: this will print garbage unless all the subdeterminants
	 * are computed in the invert() function.
	 */
	GPSD_LOG(LOG_RAW, errout, "inverse:\n");
	for (k = 0; k < 4; k++) {
	    GPSD_LOG(LOG_RAW, errout,
		     "%f %f %f %f\n",
		     inv[k][0], inv[k][1], inv[k][2], inv[k][3]);
	}
#endif /* __UNUSED__ */
    } else {
#ifndef USE_QT
	GPSD_LOG(LOG_DATA, errout,
		 "LOS matrix is singular, can't calculate DOPs - source '%s'\n",
		 gpsdata->dev.path);
#endif
	return 0;
    }

    xdop = sqrt(inv[0][0]);
    ydop = sqrt(inv[1][1]);
    hdop = sqrt(inv[0][0] + inv[1][1]);
    vdop = sqrt(inv[2][2]);
    pdop = sqrt(inv[0][0] + inv[1][1] + inv[2][2]);
    tdop = sqrt(inv[3][3]);
    gdop = sqrt(inv[0][0] + inv[1][1] + inv[2][2] + inv[3][3]);

#ifndef USE_QT
    GPSD_LOG(LOG_DATA, errout,
	     "DOPS computed/reported: X=%f/%f, Y=%f/%f, H=%f/%f, V=%f/%f, "
	     "P=%f/%f, T=%f/%f, G=%f/%f\n",
	     xdop, dop->xdop, ydop, dop->ydop, hdop, dop->hdop, vdop,
	     dop->vdop, pdop, dop->pdop, tdop, dop->tdop, gdop, dop->gdop);
#endif

    /* Check to see which DOPs we already have.  Save values if no value
     * from the GPS.  Do not overwrite values which came from the GPS */
    if (isfinite(dop->xdop) == 0) {
	dop->xdop = xdop;
    }
    if (isfinite(dop->ydop) == 0) {
	dop->ydop = ydop;
    }
    if (isfinite(dop->hdop) == 0) {
	dop->hdop = hdop;
    }
    if (isfinite(dop->vdop) == 0) {
	dop->vdop = vdop;
    }
    if (isfinite(dop->pdop) == 0) {
	dop->pdop = pdop;
    }
    if (isfinite(dop->tdop) == 0) {
	dop->tdop = tdop;
    }
    if (isfinite(dop->gdop) == 0) {
	dop->gdop = gdop;
    }

    return DOP_SET;
}

/* compute errors and derived quantities
 * also a handy place to do final sanity checking */
static void gpsd_error_model(struct gps_device_t *session)
{
    struct gps_fix_t *fix;           /* current fix */
    struct gps_fix_t *lastfix;       /* last fix, maybe same time stamp */
    struct gps_fix_t *oldfix;        /* old fix, previsou time stamp */
    double deltatime = -1.0;         /* time span to compute rates */

    /*
     * Now we compute derived quantities.  This is where the tricky error-
     * modeling stuff goes. Presently we don't know how to derive
     * time error.
     *
     * Some drivers set the position-error fields.  Only the Zodiacs
     * report speed error.  No NMEA 183 reports climb error. GPXTE
     * and PSRFEPE can report track error, but are rare.
     *
     * The UERE constants are our assumption about the base error of
     * GPS fixes in different directions.
     */
#define H_UERE_NO_DGPS		15.0	/* meters, 95% confidence */
#define H_UERE_WITH_DGPS	3.75	/* meters, 95% confidence */
#define V_UERE_NO_DGPS		23.0	/* meters, 95% confidence */
#define V_UERE_WITH_DGPS	5.75	/* meters, 95% confidence */
#define P_UERE_NO_DGPS		19.0	/* meters, 95% confidence */
#define P_UERE_WITH_DGPS	4.75	/* meters, 95% confidence */
    double h_uere, v_uere, p_uere;

    if (NULL == session)
	return;

    fix = &session->gpsdata.fix;
    lastfix = &session->lastfix;
    oldfix = &session->oldfix;

    if (0 < fix->time.tv_sec) {
        /* we have a time for this merge data */

        deltatime = TS_SUB_D(&fix->time, &lastfix->time);

        if (0.0099 < fabs(deltatime)) {
	    /* Time just moved, probably forward at least 10 ms.
             * Lastfix is now the previous (old) fix. */
	    *oldfix = *lastfix;
        } else {
            // compute delta from old fix
	    deltatime = TS_SUB_D(&fix->time, &oldfix->time);
        }
    }
    /* Sanity check for negative delta? */

    h_uere =
	(session->gpsdata.status ==
	 STATUS_DGPS_FIX ? H_UERE_WITH_DGPS : H_UERE_NO_DGPS);
    v_uere =
	(session->gpsdata.status ==
	 STATUS_DGPS_FIX ? V_UERE_WITH_DGPS : V_UERE_NO_DGPS);
    p_uere =
	(session->gpsdata.status ==
	 STATUS_DGPS_FIX ? P_UERE_WITH_DGPS : P_UERE_NO_DGPS);

    if (0 == isfinite(fix->latitude) ||
        0 == isfinite(fix->longitude) ||  /* both lat/lon, or none */
        90.0 < fabs(fix->latitude) ||     /* lat out of range */
        180.0 < fabs(fix->longitude)) {   /* lon out of range */
	fix->latitude = fix->longitude = NAN;
    }
    /* validate ECEF */
    if (0 == isfinite(fix->ecef.x) ||
        0 == isfinite(fix->ecef.y) ||
        0 == isfinite(fix->ecef.z) ||
        10.0 >= (fabs(fix->ecef.x) +
                 fabs(fix->ecef.y) +
                 fabs(fix->ecef.z))) { /* all zeros */
        fix->ecef.x = fix->ecef.y = fix->ecef.z = NAN;
    }

    /* if we have not lat/lon, but do have ECEF, calculate lat/lon */
    if ((0 == isfinite(fix->longitude) ||
         0 == isfinite(fix->latitude)) &&
        0 != isfinite(fix->ecef.x)) {
	session->gpsdata.set |= ecef_to_wgs84fix(fix,
			                         fix->ecef.x, fix->ecef.y,
			                         fix->ecef.z, fix->ecef.vx,
			                         fix->ecef.vy, fix->ecef.vz);
    }

    /* If you are in a rocket, and your GPS is ITAR unlocked, then
     * triple check these sanity checks.
     *
     * u-blox 8: Max altitude: 50,000m
     *           Max horizontal speed: 250 m/s
     *           Max climb: 100 m/s
     *
     * u-blox ZED-F9P: Max Velocity: 500 m/s
     */

    /* sanity check the speed, 10,000 m/s should be a nice max
     * Low Earth Orbit (LEO) is about 7,800 m/s */
    if (9999.9 < fabs(fix->speed))
	fix->speed = NAN;

    if (9999.9 < fabs(fix->NED.velN))
	fix->NED.velN = NAN;
    if (9999.9 < fabs(fix->NED.velE))
	fix->NED.velE = NAN;
    if (9999.9 < fabs(fix->NED.velD))
	fix->NED.velD = NAN;

    /* sanity check the climb, 10,000 m/s should be a nice max */
    if (9999.9 < fabs(fix->climb))
	fix->climb = NAN;
    if (0 != isfinite(fix->NED.velD) &&
        0 == isfinite(fix->climb)) {
        /* have good velD, use it for climb */
        fix->climb = -fix->NED.velD;
    }

    /* compute speed and track from velN and velE if needed and possible */
    if (0 != isfinite(fix->NED.velN) &&
        0 != isfinite(fix->NED.velE)) {
	if (0 == isfinite(fix->speed)) {
	    fix->speed = hypot(fix->NED.velN, fix->NED.velE);
        }
	if (0 == isfinite(fix->track)) {
	    fix->track = atan2(fix->NED.velE, fix->NED.velN) * RAD_2_DEG;
            // normalized later
        }
    }

    /*
     * OK, this is not an error computation, but we're at the right
     * place in the architecture for it.  Compute geoid separation
     * and altHAE and altMSL in the simplest possible way.
     */

    /* geoid (ellipsoid) separation and variation */
    if (0 != isfinite(fix->latitude) &&
        0 != isfinite(fix->longitude)) {
        if (0 == isfinite(fix->geoid_sep)) {
            fix->geoid_sep = wgs84_separation(fix->latitude,
                                              fix->longitude);
        }
        if (0 == isfinite(fix->magnetic_var) ||
            0.09 >= fabs(fix->magnetic_var)) {
            /* some GPS set 0.0,E, or 0,W instead of blank */
            fix->magnetic_var = mag_var(fix->latitude,
                                        fix->longitude);
        }
    }

    if (0 != isfinite(fix->magnetic_var)) {
        if (0 == isfinite(fix->magnetic_track) &&
            0 != isfinite(fix->track)) {

            // calculate mag track, normalized later
            fix->magnetic_track = fix->track + fix->magnetic_var;
        } else if (0 != isfinite(fix->magnetic_track) &&
                   0 == isfinite(fix->track)) {

            // calculate true track, normalized later
            fix->track = fix->magnetic_track - fix->magnetic_var;
        }
    }
    if (0 != isfinite(fix->track)) {
            // normalize true track
            DEG_NORM(fix->track);
    }

    if (0 != isfinite(fix->magnetic_track)) {
            // normalize mag track
            DEG_NORM(fix->magnetic_track);
    }

    if (0 != isfinite(fix->geoid_sep)) {
	if (0 != isfinite(fix->altHAE) &&
	    0 == isfinite(fix->altMSL)) {
	    /* compute missing altMSL */
	    fix->altMSL = fix->altHAE - fix->geoid_sep;
	} else if (0 == isfinite(fix->altHAE) &&
		   0 != isfinite(fix->altMSL)) {
	    /* compute missing altHAE */
	    fix->altHAE = fix->altMSL + fix->geoid_sep;
	}
    }

    /*
     * OK, this is not an error computation, but we're at the right
     * place in the architecture for it.  Compute speed over ground
     * and climb/sink in the simplest possible way.
     */

#ifdef  __UNUSED__
    // debug code
    {
	char tbuf[JSON_DATE_MAX+1];
	GPSD_LOG(LOG_SHOUT, &session->context->errout,
		 "time %s deltatime %f\n",
		 timespec_to_iso8601(fix->time, tbuf, sizeof(tbuf)),
		 deltatime);
    }
#endif // __UNUSED__

    if (0 < deltatime) {
        /* have a valid time duration */
        /* FIXME! ignore if large.  maybe > 1 hour? */

	if (MODE_2D <= fix->mode &&
	    MODE_2D <= oldfix->mode) {

	    if (0 == isfinite(fix->speed)) {
		fix->speed = earth_distance(fix->latitude, fix->longitude,
				            oldfix->latitude, oldfix->longitude)
		                            / deltatime;
                /* sanity check */
		if (9999.9 < fabs(fix->speed))
		    fix->speed = NAN;
	    }

	    if (MODE_3D <= fix->mode &&
		MODE_3D <= oldfix->mode &&
		0 == isfinite(fix->climb) &&
		0 != isfinite(fix->altHAE) &&
		0 != isfinite(oldfix->altHAE)) {
		    fix->climb = (fix->altHAE - oldfix->altHAE) / deltatime;

		    /* sanity check the climb */
		    if (9999.9 < fabs(fix->climb))
			fix->climb = NAN;
	    }
	}
    }

    /*
     * Field reports match the theoretical prediction that
     * expected time error should be half the resolution of
     * the GPS clock, so we put the bound of the error
     * in as a constant pending getting it from each driver.
     *
     * In an ideal world, we'd increase this if no leap-second has
     * been seen and it's less than 750s (one almanac load cycle) from
     * device powerup. Alas, we have no way to know when device
     * powerup occurred - depending on the receiver design it could be
     * when the hardware was first powered up or when it was first
     * opened.  Also, some devices (notably plain NMEA0183 receivers)
     * never ship an indication of when they have valid leap second.
     */
    if (0 < fix->time.tv_sec &&
        0 == isfinite(fix->ept)) {
        /* can we compute ept from tdop? */
	fix->ept = 0.005;
    }

    /* Other error computations depend on having a valid fix */
    if (MODE_2D <= fix->mode) {
	if (0 == isfinite(fix->epx) &&
            0 != isfinite(session->gpsdata.dop.hdop)) {
	    fix->epx = session->gpsdata.dop.xdop * h_uere;
        }

	if (0 == isfinite(fix->epy) &&
            0 != isfinite(session->gpsdata.dop.hdop)) {
	    fix->epy = session->gpsdata.dop.ydop * h_uere;
        }

	if (MODE_3D <= fix->mode &&
	    0 == isfinite(fix->epv) &&
	    0 != isfinite(session->gpsdata.dop.vdop)) {
	    fix->epv = session->gpsdata.dop.vdop * v_uere;
        }

        /* 2D error */
	if (0 == isfinite(fix->eph) &&
	    0 != isfinite(session->gpsdata.dop.hdop)) {
	    fix->eph = session->gpsdata.dop.hdop * p_uere;
	}

        /* 3D error */
	if (0 == isfinite(fix->sep) &&
	    0 != isfinite(session->gpsdata.dop.pdop)) {
	    fix->sep = session->gpsdata.dop.pdop * p_uere;
	}

	/*
	 * If we have a current fix and an old fix, and the packet handler
	 * didn't set the speed error, climb error or track error members
         * itself, try to compute them now.
	 */

#define EMAX(x, y)     (((x) > (y)) ? (x) : (y))

	if (0 < deltatime &&
	    MODE_2D <= oldfix->mode) {

	    if (0 == isfinite(fix->eps) &&
		0 != isfinite(oldfix->epx) &&
		0 != isfinite(oldfix->epy)) {
		    fix->eps = (EMAX(oldfix->epx, oldfix->epy) +
				EMAX(fix->epx, fix->epy)) / deltatime;
	    }

	    if (0 == isfinite(fix->epd)) {
		/*
		 * We compute a track error estimate solely from the
		 * position of this fix and the last one.  The maximum
		 * track error, as seen from the position of last fix, is
		 * the angle subtended by the two most extreme possible
		 * error positions of the current fix; the expected track
		 * error is half that.  Let the position of the old fix be
		 * A and of the new fix B.  We model the view from A as
		 * two right triangles ABC and ABD with BC and BD both
		 * having the length of the new fix's estimated error.
		 * adj = len(AB), opp = len(BC) = len(BD), hyp = len(AC) =
		 * len(AD). This leads to spurious uncertainties
		 * near 180 when we're moving slowly; to avoid reporting
		 * garbage, throw back NaN if the distance from the previous
		 * fix is less than the error estimate.
		 */
		double adj = earth_distance(oldfix->latitude, oldfix->longitude,
					    fix->latitude, fix->longitude);
		double opp = EMAX(fix->epx, fix->epy);
		if (isfinite(adj) != 0 && adj > opp) {
		    double hyp = sqrt(adj * adj + opp * opp);
		    fix->epd = RAD_2_DEG * 2 * asin(opp / hyp);
		}
            }

	    if (0 == isfinite(fix->epc) &&
	        0 != isfinite(fix->epv) &&
	        0 != isfinite(oldfix->epv)) {
		    /* Is this really valid? */
		    /* if vertical uncertainties are zero this will be too */
		    fix->epc = (oldfix->epv + fix->epv) / deltatime;
	    }
	}
    }

#ifdef  __UNUSED__
    {
        // Debug code.
	char tbuf[JSON_DATE_MAX+1];
	GPSD_LOG(&session->context->errout, 0,
		 "DEBUG: %s deltatime %.3f, speed %0.3f climb %.3f "
                 "epc %.3f fixHAE %.3f oldHAE %.3f\n",
		 timespec_to_iso8601(fix->time, tbuf, sizeof(tbuf)),
		 deltatime, fix->speed, fix->climb, fix->epc,
                 fix->altHAE, oldfix->altHAE);
    }
#endif // __UNUSED__

    if (0 < fix->time.tv_sec) {
	/* save lastfix, not yet oldfix, for later error computations */
	*lastfix = *fix;
    }
}

int gpsd_await_data(fd_set *rfds,
		    fd_set *efds,
		     const int maxfd,
		     fd_set *all_fds,
		     struct gpsd_errout_t *errout)
/* await data from any socket in the all_fds set */
{
    int status;

    FD_ZERO(efds);
    *rfds = *all_fds;
    GPSD_LOG(LOG_RAW + 1, errout, "select waits\n");
    /*
     * Poll for user commands or GPS data.  The timeout doesn't
     * actually matter here since select returns whenever one of
     * the file descriptors in the set goes ready.  The point
     * of tracking maxfd is to keep the set of descriptors that
     * pselect(2) has to poll here as small as possible (for
     * low-clock-rate SBCs and the like).
     *
     * pselect(2) is preferable to vanilla select, to eliminate
     * the once-per-second wakeup when no sensors are attached.
     * This cuts power consumption.
     */
    errno = 0;

    status = pselect(maxfd + 1, rfds, NULL, NULL, NULL, NULL);
    if (status == -1) {
	if (errno == EINTR)
	    return AWAIT_NOT_READY;
	else if (errno == EBADF) {
	    int fd;
	    for (fd = 0; fd < (int)FD_SETSIZE; fd++)
		/*
		 * All we care about here is a cheap, fast, uninterruptible
		 * way to check if a file descriptor is valid.
		 */
		if (FD_ISSET(fd, all_fds) && fcntl(fd, F_GETFL, 0) == -1) {
		    FD_CLR(fd, all_fds);
		    FD_SET(fd, efds);
		}
	    return AWAIT_NOT_READY;
	} else {
	    GPSD_LOG(LOG_ERROR, errout, "select: %s\n", strerror(errno));
	    return AWAIT_FAILED;
	}
    }

    if (errout->debug >= LOG_SPIN) {
	int i;
	char dbuf[BUFSIZ];
        timespec_t ts_now;
        char ts_str[TIMESPEC_LEN];

	dbuf[0] = '\0';
	for (i = 0; i < (int)FD_SETSIZE; i++)
	    if (FD_ISSET(i, all_fds))
		str_appendf(dbuf, sizeof(dbuf), "%d ", i);
	str_rstrip_char(dbuf, ' ');
	(void)strlcat(dbuf, "} -> {", sizeof(dbuf));
	for (i = 0; i < (int)FD_SETSIZE; i++)
	    if (FD_ISSET(i, rfds))
		str_appendf(dbuf, sizeof(dbuf), " %d ", i);

	(void)clock_gettime(CLOCK_REALTIME, &ts_now);
	GPSD_LOG(LOG_SPIN, errout,
		 "pselect() {%s} at %s (errno %d)\n",
		 dbuf,
                 timespec_str(&ts_now, ts_str, sizeof(ts_str)),
                 errno);
    }

    return AWAIT_GOT_INPUT;
}

static bool hunt_failure(struct gps_device_t *session)
/* after a bad packet, what should cue us to go to next autobaud setting? */
{
    /*
     * We have tried three different tests here.
     *
     * The first was session->badcount++>1.  This worked very well on
     * ttys for years and years, but caused failure to sync on TCP/IP
     * sources, which have I/O boundaries in mid-packet more often
     * than RS232 ones.  There's a test for this at
     * test/daemon/tcp-torture.log.
     *
     * The second was session->badcount++>1 && session->lexer.state==0.
     * Fail hunt only if we get a second consecutive bad packet
     * and the lexer is in ground state.  We don't want to fail on
     * a first bad packet because the source might have a burst of
     * leading garbage after open.  We don't want to fail if the
     * lexer is not in ground state, because that means the read
     * might have picked up a valid partial packet - better to go
     * back around the loop and pick up more data.
     *
     * The "&& session->lexer.state==0" guard causes an intermittent
     * hang while autobauding on SiRF IIIs (but not on SiRF-IIs, oddly
     * enough).  Removing this conjunct resurrected the failure
     * of test/daemon/tcp-torture.log.
     *
     * Our third attempt, isatty(session->gpsdata.gps_fd) != 0
     * && session->badcount++>1, reverts to the old test that worked
     * well on ttys for ttys and prevents non-tty devices from *ever*
     * having hunt failures. This has the cost that non-tty devices
     * will never get kicked off for presenting bad packets.
     *
     * This test may need further revision.
     */
    return isatty(session->gpsdata.gps_fd) != 0 && session->badcount++>1;
}

gps_mask_t gpsd_poll(struct gps_device_t *session)
/* update the stuff in the scoreboard structure */
{
    ssize_t newlen;
    bool driver_change = false;
    timespec_t ts_now;
    timespec_t delta;
    char ts_buf[TIMESPEC_LEN];

    gps_clear_fix(&session->newdata);

    /*
     * Input just became available from a sensor, but no read from the
     * device has yet been done.
     *
     * What we actually do here is trickier.  For latency-timing
     * purposes, we want to know the time at the start of the current
     * recording cycle. We rely on the fact that even at 4800bps
     * there's a quiet time perceptible to the human eye in gpsmon
     * between when the last character of the last packet in a
     * 1-second cycle ships and when the next reporting cycle
     * ships. Because the cycle time is fixed, higher baud rates will
     * make this gap larger.
     *
     * Thus, we look for an inter-character delay much larger than an
     * average 4800bps sentence time.  How should this delay be set?  Well,
     * counting framing bits and erring on the side of caution, it's
     * about 480 characters per second or 2083 microeconds per character;
     * that's almost exactly 0.125 seconds per average 60-char sentence.
     * Doubling this to avoid false positives, we look for an inter-character
     * delay of greater than 0.250s.
     *
     * The above assumes a cycle time of 1 second.  To get the minimum size of
     * the quiet period, we multiply by the device cycle time.
     *
     * We can sanity-check these calculation by watching logs. If we have set
     * MINIMUM_QUIET_TIME correctly, the "transmission pause" message below
     * will consistently be emitted just before the sentence that shows up
     * as start-of-cycle in gpsmon, and never emitted at any other point
     * in the cycle.
     *
     * In practice, it seems that edge detection succeeds at 9600bps but
     * fails at 4800bps.  This is not surprising, as previous profiling has
     * indicated that at 4800bps some devices overrun a 1-second cycle time
     * with the data they transmit.
     */
#define MINIMUM_QUIET_TIME	0.25
    if (session->lexer.outbuflen == 0) {
	/* beginning of a new packet */
	(void)clock_gettime(CLOCK_REALTIME, &ts_now);
	if (NULL != session->device_type &&
            (0 < session->lexer.start_time.tv_sec ||
             0 < session->lexer.start_time.tv_nsec)) {
#ifdef RECONFIGURE_ENABLE
	    const double min_cycle = TSTONS(&session->device_type->min_cycle);
#else
            // Assume that all GNSS receivers are 1Hz
	    const double min_cycle = 1;
#endif /* RECONFIGURE_ENABLE */
	    double quiet_time = (MINIMUM_QUIET_TIME * min_cycle);
	    double gap;

            gap = TS_SUB_D(&ts_now, &session->lexer.start_time);

	    if (gap > min_cycle)
		GPSD_LOG(LOG_WARN, &session->context->errout,
			 "cycle-start detector failed.\n");
	    else if (gap > quiet_time) {
		GPSD_LOG(LOG_PROG, &session->context->errout,
			 "transmission pause of %f\n", gap);
		session->sor = ts_now;
		session->lexer.start_char = session->lexer.char_counter;
	    }
	}
	session->lexer.start_time = ts_now;
    }

    if (session->lexer.type >= COMMENT_PACKET) {
	session->observed |= PACKET_TYPEMASK(session->lexer.type);
    }

    /* can we get a full packet from the device? */
    if (session->device_type != NULL) {
	newlen = session->device_type->get_packet(session);
	/* coverity[deref_ptr] */
	GPSD_LOG(LOG_RAW, &session->context->errout,
		 "%s is known to be %s\n",
		 session->gpsdata.dev.path,
		 session->device_type->type_name);
    } else {
	newlen = generic_get(session);
    }

    /* update the scoreboard structure from the GPS */
    GPSD_LOG(LOG_RAW + 1, &session->context->errout,
	     "%s sent %zd new characters\n",
	     session->gpsdata.dev.path, newlen);

    (void)clock_gettime(CLOCK_REALTIME, &ts_now);
    TS_SUB(&delta, &ts_now, &session->gpsdata.online);
    if (newlen < 0) {		/* read error */
	GPSD_LOG(LOG_INF, &session->context->errout,
		 "GPS on %s returned error %zd (%s sec since data)\n",
		 session->gpsdata.dev.path, newlen,
                 timespec_str(&delta, ts_buf, sizeof(ts_buf)));
	session->gpsdata.online.tv_sec = 0;
	session->gpsdata.online.tv_nsec = 0;
	return ERROR_SET;
    } else if (newlen == 0) {		/* zero length read, possible EOF */
	/*
	 * Multiplier is 2 to avoid edge effects due to sampling at the exact
	 * wrong time...
	 */
	if (0 < session->gpsdata.online.tv_sec &&
            // FIXME: do this with integer math...
            TSTONS(&delta) >= (TSTONS(&session->gpsdata.dev.cycle) * 2)) {
	    GPSD_LOG(LOG_INF, &session->context->errout,
		     "GPS on %s is offline (%s sec since data)\n",
		     session->gpsdata.dev.path,
                     timespec_str(&delta, ts_buf, sizeof(ts_buf)));
	    session->gpsdata.online.tv_sec = 0;
	    session->gpsdata.online.tv_nsec = 0;
	}
	return NODATA_IS;
    } else /* (newlen > 0) */ {
	GPSD_LOG(LOG_RAW, &session->context->errout,
		 "packet sniff on %s finds type %d\n",
		 session->gpsdata.dev.path, session->lexer.type);
	if (session->lexer.type == COMMENT_PACKET) {
	    if (strcmp((const char *)session->lexer.outbuffer, "# EOF\n") == 0) {
		GPSD_LOG(LOG_PROG, &session->context->errout,
			 "synthetic EOF\n");
		return EOF_IS;
	    }
	    else
		GPSD_LOG(LOG_PROG, &session->context->errout,
			 "comment, sync lock deferred\n");
	    /* FALL THROUGH */
	} else if (session->lexer.type > COMMENT_PACKET) {
	    if (session->device_type == NULL)
		driver_change = true;
	    else {
		int newtype = session->lexer.type;
		/*
		 * Are we seeing a new packet type? Then we probably
		 * want to change drivers.
		 */
		bool new_packet_type =
		    (newtype != session->device_type->packet_type);
		/*
		 * Possibly the old driver has a mode-switcher method, in
		 * which case we know it can handle NMEA itself and may
		 * want to do special things (like tracking whether a
		 * previous mode switch to binary succeeded in suppressing
		 * NMEA).
		 */
#ifdef RECONFIGURE_ENABLE
		bool dependent_nmea = (newtype == NMEA_PACKET &&
				   session->device_type->mode_switcher != NULL);
#else
		bool dependent_nmea = false;
#endif /* RECONFIGURE_ENABLE */

		/*
		 * Compute whether to switch drivers.
		 * If the previous driver type was sticky and this one
		 * isn't, we'll revert after processing the packet.
		 */
		driver_change = new_packet_type && !dependent_nmea;
	    }
	    if (driver_change) {
		const struct gps_type_t **dp;

		for (dp = gpsd_drivers; *dp; dp++)
		    if (session->lexer.type == (*dp)->packet_type) {
			GPSD_LOG(LOG_PROG, &session->context->errout,
				 "switching to match packet type %d: %s\n",
				 session->lexer.type, gpsd_prettydump(session));
			(void)gpsd_switch_driver(session, (*dp)->type_name);
			break;
		    }
	    }
	    session->badcount = 0;
	    session->gpsdata.dev.driver_mode =
                (session->lexer.type > NMEA_PACKET) ? MODE_BINARY : MODE_NMEA;
	    /* FALL THROUGH */
	} else if (hunt_failure(session) && !gpsd_next_hunt_setting(session)) {
	    (void)clock_gettime(CLOCK_REALTIME, &ts_now);
	    TS_SUB(&delta, &ts_now, &session->gpsdata.online);
	    GPSD_LOG(LOG_INF, &session->context->errout,
		     "hunt on %s failed (%s sec since data)\n",
		     session->gpsdata.dev.path,
                     timespec_str(&delta, ts_buf, sizeof(ts_buf)));
	    return ERROR_SET;
	}
    }

    if (session->lexer.outbuflen == 0) {      /* got new data, but no packet */
	GPSD_LOG(LOG_RAW + 1, &session->context->errout,
		 "New data on %s, not yet a packet\n",
		 session->gpsdata.dev.path);
	return ONLINE_SET;
    } else {			/* we have recognized a packet */
	gps_mask_t received = PACKET_SET;
	(void)clock_gettime(CLOCK_REALTIME, &session->gpsdata.online);

	GPSD_LOG(LOG_RAW + 1, &session->context->errout,
		 "Accepted packet on %s.\n",
		 session->gpsdata.dev.path);

	/* track the packet count since achieving sync on the device */
	if (driver_change &&
            (session->drivers_identified & (1 << session->driver_index)) == 0) {
	    speed_t speed = gpsd_get_speed(session);

	    /* coverity[var_deref_op] */
	    GPSD_LOG(LOG_INF, &session->context->errout,
		     "%s identified as type %s, %ld sec @ %ubps\n",
		     session->gpsdata.dev.path,
		     session->device_type->type_name,
		     (long)(time(NULL) - session->opentime),
		     (unsigned int)speed);

	    /* fire the init_query method */
	    if (session->device_type != NULL
		&& session->device_type->init_query != NULL) {
		/*
		 * We can force readonly off knowing this method does
		 * not alter device state.
		 */
		bool saved = session->context->readonly;
		session->context->readonly = false;
		session->device_type->init_query(session);
		session->context->readonly = saved;
	    }

	    /* fire the identified hook */
	    if (session->device_type != NULL
		&& session->device_type->event_hook != NULL)
		session->device_type->event_hook(session, event_identified);
	    session->lexer.counter = 0;

	    /* let clients know about this. */
	    received |= DRIVER_IS;

	    /* mark the fact that this driver has been seen */
	    session->drivers_identified |= (1 << session->driver_index);
	} else
	    session->lexer.counter++;

	/* fire the configure hook, on every packet.  Seems excessive... */
	if (session->device_type != NULL
	    && session->device_type->event_hook != NULL)
	    session->device_type->event_hook(session, event_configure);

	/*
	 * The guard looks superfluous, but it keeps the rather expensive
	 * gpsd_packetdump() function from being called even when the debug
	 * level does not actually require it.
	 */
	if (session->context->errout.debug >= LOG_RAW)
	    GPSD_LOG(LOG_RAW, &session->context->errout,
		     "raw packet of type %d, %zd:%s\n",
		     session->lexer.type,
		     session->lexer.outbuflen,
		     gpsd_prettydump(session));


	/* Get data from current packet into the fix structure */
	if (session->lexer.type != COMMENT_PACKET)
	    if (session->device_type != NULL
		&& session->device_type->parse_packet != NULL)
		received |= session->device_type->parse_packet(session);

#ifdef RECONFIGURE_ENABLE
	/*
	 * We may want to revert to the last driver that was marked
	 * sticky.  What this accomplishes is that if we've just
	 * processed something like AIVDM, but a driver with control
	 * methods or an event hook had been active before that, we
	 * keep the information about those capabilities.
	 */
	if (!STICKY(session->device_type)
	    && session->last_controller != NULL
	    && STICKY(session->last_controller))
	{
	    session->device_type = session->last_controller;
	    GPSD_LOG(LOG_PROG, &session->context->errout,
		     "reverted to %s driver...\n",
		     session->device_type->type_name);
	}
#endif /* RECONFIGURE_ENABLE */

	/* are we going to generate a report? if so, count characters */
	if ((received & REPORT_IS) != 0) {
	    session->chars = session->lexer.char_counter - session->lexer.start_char;
	}

	session->gpsdata.set = ONLINE_SET | received;

	/*
	 * Compute fix-quality data from the satellite positions.
	 * These will not overwrite any DOPs reported from the packet
	 * we just got.
	 */
	if ((received & SATELLITE_SET) != 0
	    && session->gpsdata.satellites_visible > 0) {
	    session->gpsdata.set |= fill_dop(&session->context->errout,
					     &session->gpsdata,
					     &session->gpsdata.dop);
	}

	/* copy/merge device data into staging buffers */
	if ((session->gpsdata.set & CLEAR_IS) != 0) {
            /* CLEAR_IS should only be set on first sentence of cycle */
	    gps_clear_fix(&session->gpsdata.fix);
            gps_clear_att(&session->gpsdata.attitude);
        }

	/* GPSD_LOG(LOG_PROG, &session->context->errout,
	                 "transfer mask: %s\n",
                         gps_maskdump(session->gpsdata.set)); */
	gps_merge_fix(&session->gpsdata.fix,
		      session->gpsdata.set, &session->newdata);

	gpsd_error_model(session);

	/*
	 * Count good fixes. We used to check
	 *      session->gpsdata.status > STATUS_NO_FIX
	 * here, but that wasn't quite right.  That tells us whether
	 * we think we have a valid fix for the current cycle, but remains
	 * true while following non-fix packets are received.  What we
	 * really want to know is whether the last packet received was a
	 * fix packet AND held a valid fix. We must ignore non-fix packets
	 * AND packets which have fix data but are flagged as invalid. Some
	 * devices output fix packets on a regular basis, even when unable
	 * to derive a good fix. Such packets should set STATUS_NO_FIX.
	 */
	if (0 != (session->gpsdata.set & (LATLON_SET|ECEF_SET))) {
	    if ( session->gpsdata.status > STATUS_NO_FIX) {
		session->context->fixcnt++;
		session->fixcnt++;
            } else {
		session->context->fixcnt = 0;
		session->fixcnt = 0;
            }
	}

	/*
	 * Sanity check.  This catches a surprising number of port and
	 * driver errors, including 32-vs.-64-bit problems.
	 */
	if ((session->gpsdata.set & TIME_SET) != 0) {
	    if (session->newdata.time.tv_sec >
                (time(NULL) + (60 * 60 * 24 * 365))) {
		GPSD_LOG(LOG_WARN, &session->context->errout,
			 "date (%lld) more than a year in the future!\n",
			 (long long)session->newdata.time.tv_sec);
	    } else if (session->newdata.time.tv_sec < 0) {
		GPSD_LOG(LOG_ERROR, &session->context->errout,
			 "date (%lld) is negative!\n",
			 (long long)session->newdata.time.tv_sec);
            }
	}

	return session->gpsdata.set;
    }
    /* Should never get here */
    GPSD_LOG(LOG_EMERG, &session->context->errout,
             "fell out of gps_poll()!\n");
    return 0;
}

int gpsd_multipoll(const bool data_ready,
		   struct gps_device_t *device,
		   void (*handler)(struct gps_device_t *, gps_mask_t),
		   float reawake_time)
/* consume and handle packets from a specified device */
{
    if (data_ready)
    {
	int fragments;

	GPSD_LOG(LOG_RAW + 1, &device->context->errout,
		 "polling %d\n", device->gpsdata.gps_fd);

#ifdef NETFEED_ENABLE
	/*
	 * Strange special case - the opening transaction on an NTRIP connection
	 * may not yet be completed.  Try to ratchet things forward.
	 */
	if (device->servicetype == service_ntrip
	    && device->ntrip.conn_state != ntrip_conn_established) {

	    (void)ntrip_open(device, "");
	    if (device->ntrip.conn_state == ntrip_conn_err) {
		GPSD_LOG(LOG_WARN, &device->context->errout,
			 "connection to ntrip server failed\n");
		device->ntrip.conn_state = ntrip_conn_init;
		return DEVICE_ERROR;
	    } else {
		return DEVICE_READY;
	    }
	}
#endif /* NETFEED_ENABLE */

	for (fragments = 0; ; fragments++) {
	    gps_mask_t changed = gpsd_poll(device);

	    if (changed == EOF_IS) {
		GPSD_LOG(LOG_WARN, &device->context->errout,
			 "device signed off %s\n",
			 device->gpsdata.dev.path);
		return DEVICE_EOF;
	    } else if (changed == ERROR_SET) {
		GPSD_LOG(LOG_WARN, &device->context->errout,
			 "device read of %s returned error or "
                         "packet sniffer failed sync (flags %s)\n",
			 device->gpsdata.dev.path,
			 gps_maskdump(changed));
		return DEVICE_ERROR;
	    } else if (changed == NODATA_IS) {
		/*
		 * No data on the first fragment read means the device
		 * fd may have been in an end-of-file condition on select.
		 */
		if (fragments == 0) {
		    GPSD_LOG(LOG_DATA, &device->context->errout,
			     "%s returned zero bytes\n",
			     device->gpsdata.dev.path);
		    if (device->zerokill) {
			/* failed timeout-and-reawake, kill it */
			gpsd_deactivate(device);
			if (device->ntrip.works) {
			    device->ntrip.works = false; // reset so we try this once only
			    if (gpsd_activate(device, O_CONTINUE) < 0) {
				GPSD_LOG(LOG_WARN, &device->context->errout,
					 "reconnect to ntrip server failed\n");
				return DEVICE_ERROR;
			    } else {
				GPSD_LOG(LOG_INF, &device->context->errout,
					 "reconnecting to ntrip server\n");
				return DEVICE_READY;
			    }
			}
		    } else if (reawake_time == 0) {
			return DEVICE_ERROR;
		    } else {
			/*
			 * Disable listening to this fd for long enough
			 * that the buffer can fill up again.
			 */
			GPSD_LOG(LOG_DATA, &device->context->errout,
				 "%s will be repolled in %f seconds\n",
				 device->gpsdata.dev.path, reawake_time);
			device->reawake = time(NULL) + reawake_time;
			return DEVICE_UNREADY;
		    }
		}
		/*
		 * No data on later fragment reads just means the
		 * input buffer is empty.  In this case break out
		 * of the fragment-processing loop but consider
		 * the device still good.
		 */
		break;
	    }

	    /* we got actual data, head off the reawake special case */
	    device->zerokill = false;
	    device->reawake = (time_t)0;

	    /* must have a full packet to continue */
	    if ((changed & PACKET_SET) == 0)
		break;

	    /* conditional prevents mask dumper from eating CPU */
	    if (device->context->errout.debug >= LOG_DATA) {
		if (device->lexer.type == BAD_PACKET)
		    GPSD_LOG(LOG_DATA, &device->context->errout,
			     "packet with bad checksum from %s\n",
			     device->gpsdata.dev.path);
		else
		    GPSD_LOG(LOG_DATA, &device->context->errout,
			     "packet type %d from %s with %s\n",
			     device->lexer.type,
			     device->gpsdata.dev.path,
			     gps_maskdump(device->gpsdata.set));
	    }


	    /* handle data contained in this packet */
	    if (device->lexer.type != BAD_PACKET)
		handler(device, changed);

#ifdef __future__
            // this breaks: test/daemon/passthrough.log ??
	    /*
	     * Bernd Ocklin suggests:
	     * Exit when a full packet was received and parsed.
	     * This allows other devices to be serviced even if
	     * this device delivers a full packet at every single
	     * read.
	     * Otherwise we can sit here for a long time without
	     * any for-loop exit condition being met.
             * It might also reduce the latency from a received packet to
             * it being output by gpsd.
	     */
	    if ((changed & PACKET_SET) != 0)
               break;
#endif /* __future__ */
	}
    }
    else if (device->reawake>0 && time(NULL) >device->reawake) {
	/* device may have had a zero-length read */
	GPSD_LOG(LOG_DATA, &device->context->errout,
		 "%s reawakened after zero-length read\n",
		 device->gpsdata.dev.path);
	device->reawake = (time_t)0;
	device->zerokill = true;
	return DEVICE_READY;
    }

    /* no change in device descriptor state */
    return DEVICE_UNCHANGED;
}

void gpsd_wrap(struct gps_device_t *session)
/* end-of-session wrapup */
{
    if (!BAD_SOCKET(session->gpsdata.gps_fd))
	gpsd_deactivate(session);
}

void gpsd_zero_satellites( struct gps_data_t *out)
{
    int sat;

    (void)memset(out->skyview, '\0', sizeof(out->skyview));
    out->satellites_visible = 0;
    /* zero is good inbound data for ss, elevation, and azimuth.  */
    /* we need to set them to invalid values */
    for ( sat = 0; sat < MAXCHANNELS; sat++ ) {
        out->skyview[sat].azimuth = NAN;
        out->skyview[sat].elevation = NAN;
        out->skyview[sat].ss = NAN;
        out->skyview[sat].freqid = -1;
    }
#if 0
    /*
     * We used to clear DOPs here, but this causes misbehavior on some
     * combined GPS/GLONASS/QZSS receivers like the Telit SL869; the
     * symptom is that the "satellites_used" field in a struct gps_data_t
     * filled in by gps_read() is always zero.
     */
    gps_clear_dop(&out->dop);
#endif
}

/* Latch the fact that we've saved a fix.
 * And add in the device fudge */
void ntp_latch(struct gps_device_t *device, struct timedelta_t *td)
{

    /* this should be an invariant of the way this function is called */
    if (0 >= device->newdata.time.tv_sec) {
        return;
    }

    (void)clock_gettime(CLOCK_REALTIME, &td->clock);
    /* structure copy of time from GPS */
    td->real = device->newdata.time;

    /* is there an offset method? */
    if (NULL != device->device_type &&
	NULL != device->device_type->time_offset) {
	double integral;
        double offset = device->device_type->time_offset(device);

	/* add in offset which is double */
	td->real.tv_nsec += (long)(modf(offset, &integral) * 1e9);
	td->real.tv_sec += (time_t)integral;
        TS_NORM(&td->real);
    }

    /* thread-safe update */
    pps_thread_fixin(&device->pps_thread, td);
}

/* end */
