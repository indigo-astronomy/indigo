/*
 * This file contains functions to deal with compatibility issues across OSes.
 *
 * The initial version of this file is a near-verbatim concatenation of the
 * following three source files:
 *     clock_gettime.c
 *     daemon.c
 *     strl.c
 * History of this code prior to the creation of this file can be found
 * in the histories of those files.
 *
 * This file is Copyright (c)2017-2019 by the GPSD project
 * SPDX-License-Identifier: BSD-2-clause
 */

#include "gpsd_config.h"  /* must be before all includes */

#include "os_compat.h"

#ifndef HAVE_CLOCK_GETTIME

/* Simulate ANSI/POSIX clock_gettime() on platforms that don't have it */

#include <time.h>
#include <sys/time.h>

/*
 * Note that previous versions of this code made use of clock_get_time()
 * on OSX, as a way to get time of day with nanosecond resolution.  But
 * it turns out that clock_get_time() only has microsecond resolution,
 * in spite of the data format, and it's also substantially slower than
 * gettimeofday().  Thus, it makes no sense to do anything special for OSX.
 */

int clock_gettime(clockid_t clk_id, struct timespec *ts)
{
    (void) clk_id;
    struct timeval tv;
    if (gettimeofday(&tv, NULL) < 0)
	return -1;
    ts->tv_sec = tv.tv_sec;
    ts->tv_nsec = tv.tv_usec * 1000;
    return 0;
}
#endif /* HAVE_CLOCK_GETTIME */

/* End of clock_gettime section */

#ifndef HAVE_DAEMON
/* Simulate Linux/BSD daemon() on platforms that don't have it */

#ifdef HAVE_FORK

#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#if defined (HAVE_PATH_H)
#include <paths.h>
#else
#if !defined (_PATH_DEVNULL)
#define _PATH_DEVNULL    "/dev/null"
#endif
#endif

int os_daemon(int nochdir, int noclose)
/* compatible with the daemon(3) found on Linuxes and BSDs */
{
    int fd;

    switch (fork()) {
    case -1:
	return -1;
    case 0:			/* child side */
	break;
    default:			/* parent side */
	exit(EXIT_SUCCESS);
    }

    if (setsid() == -1)
	return -1;
    if ((nochdir==0) && (chdir("/") == -1))
	return -1;
    if ((noclose==0) && (fd = open(_PATH_DEVNULL, O_RDWR, 0)) != -1) {
	(void)dup2(fd, STDIN_FILENO);
	(void)dup2(fd, STDOUT_FILENO);
	(void)dup2(fd, STDERR_FILENO);
	if (fd > 2)
	    (void)close(fd);
    }
    /* coverity[leaked_handle] Intentional handle duplication */
    return 0;
}
#else /* !HAVE_FORK */

#include <errno.h>

int os_daemon(int nochdir, int noclose)
{
    (void) nochdir; (void) noclose;
    errno = EPERM;
    return -1;
}

#endif /* !HAVE_FORK */

#else /* HAVE_DAEMON */

#if defined (__linux__) || defined (__GLIBC__)

#include <unistd.h>      /* for daemon() */

#elif defined(__APPLE__) /* !__linux__ */

/*
 * Avoid the OSX deprecation warning.
 *
 * Note that on OSX, real daemons like gpsd should run via launchd rather than
 * self-daemonizing, but we use daemon() for other tools as well.
 *
 * There doesn't seem to be an easy way to avoid the warning other than by
 * providing our own declaration to override the deprecation-flagged version.
 * Fortunately, the function signature is pretty well frozen at this point.
 *
 * Until we fix the kludge where all this code has to be additionally compiled
 * as C++ for the Qt library, we need this to be compilable as C++ as well.
 */
#ifdef __cplusplus
extern "C" {
#endif

int daemon(int nochdir, int noclose);

#ifdef __cplusplus
}
#endif

#else /* !__linux__ && !__APPLE__ */

#include <stdlib.h>

#endif /* !__linux__ && !__APPLE__ */

int os_daemon(int nochdir, int noclose)
{
    return daemon(nochdir, noclose);
}

#endif /* HAVE_DAEMON */

/* End of daemon section */

/* Provide syslog() on platforms that don't have it */

#ifndef HAVE_SYSLOG_H
#include "compiler.h"
#include <stdarg.h>
#include <stdio.h>
/*
 * Minimal syslog() fallback to print to stderr
 *
 */
PRINTF_FUNC(2, 3) void syslog(int priority UNUSED, const char *format, ...)
{
  /* ATM ignore priority (i.e. don't even both prepending to output) */
  char buf[BUFSIZ];
  va_list ap;
  va_start(ap, format);
  /* Always append a new line to the message */
  (void)vsnprintf(buf, sizeof(buf) - 2, format, ap);
  (void)fprintf(stderr, "%s\n", buf);
  va_end(ap);
}

void openlog (const char *__ident UNUSED, int __option UNUSED, int __facility UNUSED)
{
  (void)fprintf(stderr, "Warning openlog() not available\n");
}

void closelog (void)
{
}
#endif /* !HAVE_SYSLOG_H */

/* End of syslog section */

/*
 * Provide BSD strlcat()/strlcpy() on platforms that don't have it
 *
 * These versions use memcpy and strlen() because they are often
 * heavily optimized down to assembler level. Thus, likely to be
 * faster even with the function call overhead.
 */

#ifndef HAVE_STRLCAT

#include <string.h>

/*
 * Appends src to string dst of size siz (unlike strncat, siz is the
 * full size of dst, not space left).  At most siz-1 characters
 * will be copied.  Always NUL terminates (unless siz <= strlen(dst)).
 * Returns strlen(src) + MIN(siz, strlen(initial dst)).
 * If retval >= siz, truncation occurred.
 */
size_t strlcat(char *dst, const char *src, size_t siz)
{
    size_t slen = strlen(src);
    size_t dlen = strlen(dst);
    if (siz != 0) {
	if (dlen + slen < siz)
	    memcpy(dst + dlen, src, slen + 1);
	else {
	    memcpy(dst + dlen, src, siz - dlen - 1);
	    dst[siz - 1] = '\0';
	}
    }
    return dlen + slen;
}

#ifdef __UNUSED__
/*	$OpenBSD: strlcat.c,v 1.13 2005/08/08 08:05:37 espie Exp $	*/

/*
 * Copyright (c) 1998 Todd C. Miller <Todd.Miller@courtesan.com>
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

size_t strlcat(char *dst, const char *src, size_t siz)
{
    char *d = dst;
    const char *s = src;
    size_t n = siz;
    size_t dlen;

    /* Find the end of dst and adjust bytes left but don't go past end */
    while (n-- != 0 && *d != '\0')
	d++;
    dlen = (size_t) (d - dst);
    n = siz - dlen;

    if (n == 0)
	return (dlen + strlen(s));
    while (*s != '\0') {
	if (n != 1) {
	    *d++ = *s;
	    n--;
	}
	s++;
    }
    *d = '\0';

    return (dlen + (s - src));	/* count does not include NUL */
}
#endif /* __UNUSED__ */
#endif /* !HAVE_STRLCAT */

#ifndef HAVE_STRLCPY

#include <string.h>

/*
 * Copy src to string dst of size siz.  At most siz-1 characters
 * will be copied.  Always NUL terminates (unless siz == 0).
 * Returns strlen(src); if retval >= siz, truncation occurred.
 */
size_t strlcpy(char *dst, const char *src, size_t siz)
{
    size_t len = strlen(src);
    if (siz != 0) {
	if (len >= siz) {
	    memcpy(dst, src, siz - 1);
	    dst[siz - 1] = '\0';
	} else
	    memcpy(dst, src, len + 1);
    }
    return len;
}

#endif /* !HAVE_STRLCPY */

/* End of strlcat()/strlcpy() section */

/*
 * Provide sincos() on platforms that don't have it.
 * This just uses the usual sin() and cos(), with no speed benefit.
 * It doesn't worry about the corner case where the +-infinity argument
 * may raise the "invalid" exception before storing both NaN results.
 */

#ifndef HAVE_SINCOS

#include <math.h>

void sincos(double x, double *sinp, double *cosp)
{
    *sinp = sin(x);
    *cosp = cos(x);
}

#endif /* !HAVE_SINCOS */

/* End of sincos() section. */
