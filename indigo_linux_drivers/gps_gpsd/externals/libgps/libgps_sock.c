/* libgps_sock.c -- client interface library for the gpsd daemon
 *
 * This file is Copyright (c) 2010-2018 by the GPSD project
 * SPDX-License-Identifier: BSD-2-clause
 */

#include "gpsd_config.h"  /* must be before all includes */

#include <assert.h>
#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <locale.h>
#include <math.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/select.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#ifndef USE_QT
#ifdef HAVE_SYS_SOCKET_H
#include <sys/socket.h>
#endif /* HAVE_SYS_SOCKET_H */
#ifdef HAVE_WINSOCK2_H
#include <winsock2.h>
#endif /* HAVE_WINSOCK2_H */
#else
#include <QTcpSocket>
#endif /* USE_QT */

#include "gps.h"
#include "gpsd.h"
#include "libgps.h"
#include "strfuncs.h"
#include "timespec.h"      /* for NS_IN_SEC */
#ifdef SOCKET_EXPORT_ENABLE
#include "gps_json.h"

struct privdata_t
{
    bool newstyle;
    /* data buffered from the last read */
    ssize_t waiting;
    char buffer[GPS_JSON_RESPONSE_MAX * 2];
#ifdef LIBGPS_DEBUG
    int waitcount;
#endif /* LIBGPS_DEBUG */
};

#ifdef HAVE_WINSOCK2_H
static bool need_init = TRUE;
static bool need_finish = TRUE;

static bool windows_init(void)
/* Ensure socket networking is initialized for Windows. */
{
    WSADATA wsadata;
    /* request access to Windows Sockets API version 2.2 */
    int res = WSAStartup(MAKEWORD(2, 2), &wsadata);
    if (res != 0) {
      libgps_debug_trace((DEBUG_CALLS, "WSAStartup returns error %d\n", res));
    }
    return (res == 0);
}

static bool windows_finish(void)
/* Shutdown Windows Sockets. */
{
    int res = WSACleanup();
    if (res != 0) {
        libgps_debug_trace((DEBUG_CALLS, "WSACleanup returns error %d\n", res));
    }
    return (res == 0);
}
#endif /* HAVE_WINSOCK2_H */

int gps_sock_open(const char *host, const char *port,
		  struct gps_data_t *gpsdata)
{
    if (!host)
	host = "localhost";
    if (!port)
	port = DEFAULT_GPSD_PORT;

    libgps_debug_trace((DEBUG_CALLS, "gps_sock_open(%s, %s)\n", host, port));

#ifndef USE_QT
#ifdef HAVE_WINSOCK2_H
	if (need_init) {
	  need_init != windows_init();
	}
#endif
	if ((gpsdata->gps_fd =
	    netlib_connectsock(AF_UNSPEC, host, port, "tcp")) < 0) {
	    errno = gpsdata->gps_fd;
	    libgps_debug_trace((DEBUG_CALLS,
                               "netlib_connectsock() returns error %d\n",
                               errno));
	    return -1;
        } else
	    libgps_debug_trace((DEBUG_CALLS,
                "netlib_connectsock() returns socket on fd %d\n",
                gpsdata->gps_fd));
#else /* HAVE_WINSOCK2_H */
	QTcpSocket *sock = new QTcpSocket();
	gpsdata->gps_fd = sock;
	sock->connectToHost(host, QString(port).toInt());
	if (!sock->waitForConnected())
	    qDebug() << "libgps::connect error: " << sock->errorString();
	else
	    qDebug() << "libgps::connected!";
#endif /* USE_QT */

    /* set up for line-buffered I/O over the daemon socket */
    gpsdata->privdata = (void *)malloc(sizeof(struct privdata_t));
    if (gpsdata->privdata == NULL)
	return -1;
    PRIVATE(gpsdata)->newstyle = false;
    PRIVATE(gpsdata)->waiting = 0;
    PRIVATE(gpsdata)->buffer[0] = 0;

#ifdef LIBGPS_DEBUG
    PRIVATE(gpsdata)->waitcount = 0;
#endif /* LIBGPS_DEBUG */
    return 0;
}

bool gps_sock_waiting(const struct gps_data_t *gpsdata, int timeout)
/* is there input waiting from the GPS? */
/* timeout is in uSec */
{
#ifndef USE_QT
    libgps_debug_trace((DEBUG_CALLS, "gps_waiting(%d): %d\n",
                       timeout, PRIVATE(gpsdata)->waitcount++));
    if (PRIVATE(gpsdata)->waiting > 0)
	return true;

    /* all error conditions return "not waiting" -- crude but effective */
    return nanowait(gpsdata->gps_fd, timeout * 1000);
#else
    return ((QTcpSocket *) (gpsdata->gps_fd))->waitForReadyRead(timeout / 1000);
#endif
}

int gps_sock_close(struct gps_data_t *gpsdata)
/* close a gpsd connection */
{
    free(PRIVATE(gpsdata));
    gpsdata->privdata = NULL;
#ifndef USE_QT
    int status;
#ifdef HAVE_WINSOCK2_H
    status = closesocket(gpsdata->gps_fd);
    if (need_finish) {
      need_finish != windows_finish();
    }
#else
    status = close(gpsdata->gps_fd);
#endif /* HAVE_WINSOCK2_H */
    gpsdata->gps_fd = -1;
    return status;
#else
    QTcpSocket *sock = (QTcpSocket *) gpsdata->gps_fd;
    sock->disconnectFromHost();
    delete sock;
    gpsdata->gps_fd = NULL;
    return 0;
#endif
}

int gps_sock_read(struct gps_data_t *gpsdata, char *message, int message_len)
/* wait for and read data being streamed from the daemon */
{
    char *eol;
    ssize_t response_length;
    int status = -1;

    errno = 0;
    gpsdata->set &= ~PACKET_SET;

    /* scan to find end of message (\n), or end of buffer */
    for (eol = PRIVATE(gpsdata)->buffer;
	 eol < (PRIVATE(gpsdata)->buffer + PRIVATE(gpsdata)->waiting);
         eol++) {
        if ('\n' == *eol)
            break;
    }

    if (*eol != '\n') {
	/* no full message found, try to fill buffer */

#ifndef USE_QT
	/* read data: return -1 if no data waiting or buffered, 0 otherwise */
	status = (int)recv(gpsdata->gps_fd,
               PRIVATE(gpsdata)->buffer + PRIVATE(gpsdata)->waiting,
               sizeof(PRIVATE(gpsdata)->buffer) - PRIVATE(gpsdata)->waiting, 0);
#else
	status =
	    ((QTcpSocket *) (gpsdata->gps_fd))->read(PRIVATE(gpsdata)->buffer +
                 PRIVATE(gpsdata)->waiting,
                 sizeof(PRIVATE(gpsdata)->buffer) - PRIVATE(gpsdata)->waiting);
#endif
#ifdef HAVE_WINSOCK2_H
	int wserr = WSAGetLastError();
#endif /* HAVE_WINSOCK2_H */

#ifdef USE_QT
	if (status < 0) {
            /* All negative statuses are error for QT
             *
             * read: https://doc.qt.io/qt-5/qiodevice.html#read
             *
             * Reads at most maxSize bytes from the device into data,
             * and returns the number of bytes read.
             * If an error occurs, such as when attempting to read from
             * a device opened in WriteOnly mode, this function returns -1.
             *
             * 0 is returned when no more data is available for reading.
             * However, reading past the end of the stream is considered
             * an error, so this function returns -1 in those cases
             * (that is, reading on a closed socket or after a process
             * has died).
             */
            return -1;
	}

#else  /* not USE_QT */
	if (status <= 0) {
            /* 0 or negative
             *
             * read:
             *  https://pubs.opengroup.org/onlinepubs/007908775/xsh/read.html
             *
             * If nbyte is 0, read() will return 0 and have no other results.
             * ...
             * When attempting to read a file (other than a pipe or FIFO)
             * that supports non-blocking reads and has no data currently
             * available:
             *    - If O_NONBLOCK is set,
             *            read() will return a -1 and set errno to [EAGAIN].
             *    - If O_NONBLOCK is clear,
             *            read() will block the calling thread until some
             *            data becomes available.
             *    - The use of the O_NONBLOCK flag has no effect if there
             *       is some data available.
             * ...
             * If a read() is interrupted by a signal before it reads any
             * data, it will return -1 with errno set to [EINTR].
             * If a read() is interrupted by a signal after it has
             * successfully read some data, it will return the number of
             * bytes read.
             *
             * recv:
             *   https://pubs.opengroup.org/onlinepubs/007908775/xns/recv.html
             *
             * If no messages are available at the socket and O_NONBLOCK
             * is not set on the socket's file descriptor, recv() blocks
             * until a message arrives.
             * If no messages are available at the socket and O_NONBLOCK
             * is set on the socket's file descriptor, recv() fails and
             * sets errno to [EAGAIN] or [EWOULDBLOCK].
             * ...
             * Upon successful completion, recv() returns the length of
             * the message in bytes. If no messages are available to be
             * received and the peer has performed an orderly shutdown,
             * recv() returns 0. Otherwise, -1 is returned and errno is
             * set to indicate the error.
             *
             * Summary:
             * if nbytes 0 and read return 0 -> out of the free buffer
             * space but still didn't get correct json -> report an error
             * -> return -1
             * if read return 0 but requested some bytes to read -> other
             *side disconnected -> report an error -> return -1
             * if read return -1 and errno is in [EAGAIN, EINTR, EWOULDBLOCK]
             * -> not an error, we'll retry later -> return 0
             * if read return -1 and errno is not in [EAGAIN, EINTR,
             * EWOULDBLOCK] -> error -> return -1
             *
             */

            /*
             * check for not error cases first: EAGAIN, EINTR, etc
             */
             if (status < 0) {
#ifdef HAVE_WINSOCK2_H
                if (wserr == WSAEINTR || wserr == WSAEWOULDBLOCK)
                    return 0;
#else
                if (errno == EINTR || errno == EAGAIN || errno == EWOULDBLOCK)
                    return 0;
#endif /* HAVE_WINSOCK2_H */
             }

             /* disconnect or error */
             return -1;
	}
#endif /* USE_QT */

	/* if we just received data from the socket, it's in the buffer */
	PRIVATE(gpsdata)->waiting += status;

	/* there's new buffered data waiting, check for full message */
	for (eol = PRIVATE(gpsdata)->buffer;
	     eol < (PRIVATE(gpsdata)->buffer + PRIVATE(gpsdata)->waiting);
             eol++) {
	    if ('\n' == *eol)
		break;
	}
	if (*eol != '\n')
            /* still no full message, give up for now */
	    return 0;
    }

    /* eol now points to trailing \n in a full message */
    *eol = '\0';
    if (NULL != message) {
        strlcpy(message, PRIVATE(gpsdata)->buffer, message_len);
    }
    (void)clock_gettime(CLOCK_REALTIME, &gpsdata->online);
    /* unpack the JSON message */
    status = gps_unpack(PRIVATE(gpsdata)->buffer, gpsdata);

    /* why the 1? */
    response_length = eol - PRIVATE(gpsdata)->buffer + 1;

    /* calculate length of good data still in buffer */
    PRIVATE(gpsdata)->waiting -= response_length;

    if (1 > PRIVATE(gpsdata)->waiting) {
        /* no waiting data, or overflow, clear the buffer, just in case */
        *PRIVATE(gpsdata)->buffer = '\0';
        PRIVATE(gpsdata)->waiting = 0;
    } else {
	memmove(PRIVATE(gpsdata)->buffer,
		PRIVATE(gpsdata)->buffer + response_length,
		PRIVATE(gpsdata)->waiting);
    }
    gpsdata->set |= PACKET_SET;

    return (status == 0) ? (int)response_length : status;
}

int gps_unpack(char *buf, struct gps_data_t *gpsdata)
/* unpack a gpsd response into a status structure, buf must be writeable.
 * gps_unpack() currently returns 0 in all cases, but should it ever need to
 * return an error status, it must be < 0.
 */
{
    libgps_debug_trace((DEBUG_CALLS, "gps_unpack(%s)\n", buf));

    /* detect and process a JSON response */
    if (buf[0] == '{') {
	const char *jp = buf, **next = &jp;
	while (next != NULL && *next != NULL && next[0][0] != '\0') {
	    libgps_debug_trace((DEBUG_CALLS,
                               "gps_unpack() segment parse '%s'\n",
                               *next));
	    if (libgps_json_unpack(*next, gpsdata, next) == -1)
		break;
#ifdef LIBGPS_DEBUG
	    if (libgps_debuglevel >= 1)
		libgps_dump_state(gpsdata);
#endif /* LIBGPS_DEBUG */

	}
    }

#ifndef USE_QT
    libgps_debug_trace((DEBUG_CALLS,
                        "final flags: (0x%04x) %s\n",
                        gpsdata->set,gps_maskdump(gpsdata->set)));
#endif
    return 0;
}

const char *gps_sock_data(const struct gps_data_t *gpsdata)
/* return the contents of the client data buffer */
{
    /* no length data, so pretty useless... */
    return PRIVATE(gpsdata)->buffer;
}

int gps_sock_send(struct gps_data_t *gpsdata, const char *buf)
/* send a command to the gpsd instance */
{
#ifndef USE_QT
#ifdef HAVE_WINSOCK2_H
    if (send(gpsdata->gps_fd, buf, strlen(buf), 0) == (ssize_t) strlen(buf))
#else
    if (write(gpsdata->gps_fd, buf, strlen(buf)) == (ssize_t) strlen(buf))
#endif /* HAVE_WINSOCK2_H */
	return 0;
    else
	return -1;
#else
    QTcpSocket *sock = (QTcpSocket *) gpsdata->gps_fd;
    sock->write(buf, strlen(buf));
    if (sock->waitForBytesWritten())
	return 0;
    else {
	qDebug() << "libgps::send error: " << sock->errorString();
	return -1;
    }
#endif
}

int gps_sock_stream(struct gps_data_t *gpsdata, unsigned int flags, void *d)
/* ask gpsd to stream reports at you, hiding the command details */
{
    char buf[GPS_JSON_COMMAND_MAX];

    if ((flags & (WATCH_JSON | WATCH_NMEA | WATCH_RAW)) == 0) {
	flags |= WATCH_JSON;
    }
    if ((flags & WATCH_DISABLE) != 0) {
	(void)strlcpy(buf, "?WATCH={\"enable\":false,", sizeof(buf));
	if (flags & WATCH_JSON)
	    (void)strlcat(buf, "\"json\":false,", sizeof(buf));
	if (flags & WATCH_NMEA)
	    (void)strlcat(buf, "\"nmea\":false,", sizeof(buf));
	if (flags & WATCH_RAW)
	    (void)strlcat(buf, "\"raw\":1,", sizeof(buf));
	if (flags & WATCH_RARE)
	    (void)strlcat(buf, "\"raw\":0,", sizeof(buf));
	if (flags & WATCH_SCALED)
	    (void)strlcat(buf, "\"scaled\":false,", sizeof(buf));
	if (flags & WATCH_TIMING)
	    (void)strlcat(buf, "\"timing\":false,", sizeof(buf));
	if (flags & WATCH_SPLIT24)
	    (void)strlcat(buf, "\"split24\":false,", sizeof(buf));
	if (flags & WATCH_PPS)
	    (void)strlcat(buf, "\"pps\":false,", sizeof(buf));
	str_rstrip_char(buf, ',');
	(void)strlcat(buf, "};", sizeof(buf));
	libgps_debug_trace((DEBUG_CALLS,
                           "gps_stream() disable command: %s\n", buf));
	return gps_send(gpsdata, buf);
    } else {			/* if ((flags & WATCH_ENABLE) != 0) */
	(void)strlcpy(buf, "?WATCH={\"enable\":true,", sizeof(buf));
	if (flags & WATCH_JSON)
	    (void)strlcat(buf, "\"json\":true,", sizeof(buf));
	if (flags & WATCH_NMEA)
	    (void)strlcat(buf, "\"nmea\":true,", sizeof(buf));
	if (flags & WATCH_RARE)
	    (void)strlcat(buf, "\"raw\":1,", sizeof(buf));
	if (flags & WATCH_RAW)
	    (void)strlcat(buf, "\"raw\":2,", sizeof(buf));
	if (flags & WATCH_SCALED)
	    (void)strlcat(buf, "\"scaled\":true,", sizeof(buf));
	if (flags & WATCH_TIMING)
	    (void)strlcat(buf, "\"timing\":true,", sizeof(buf));
	if (flags & WATCH_SPLIT24)
	    (void)strlcat(buf, "\"split24\":true,", sizeof(buf));
	if (flags & WATCH_PPS)
	    (void)strlcat(buf, "\"pps\":true,", sizeof(buf));
	if (flags & WATCH_DEVICE)
	    str_appendf(buf, sizeof(buf), "\"device\":\"%s\",", (char *)d);
	str_rstrip_char(buf, ',');
	(void)strlcat(buf, "};", sizeof(buf));
	libgps_debug_trace((DEBUG_CALLS,
                           "gps_stream() enable command: %s\n", buf));
	return gps_send(gpsdata, buf);
    }
}

int gps_sock_mainloop(struct gps_data_t *gpsdata, int timeout,
			 void (*hook)(struct gps_data_t *gpsdata))
/* run a socket main loop with a specified handler */
{
    for (;;) {
	if (!gps_waiting(gpsdata, timeout)) {
	    return -1;
	} else {
	    int status = gps_read(gpsdata, NULL, 0);

	    if (status == -1)
		return -1;
	    if (status > 0)
		(*hook)(gpsdata);
	}
    }
    //return 0;
}

#endif /* SOCKET_EXPORT_ENABLE */

/* end */
