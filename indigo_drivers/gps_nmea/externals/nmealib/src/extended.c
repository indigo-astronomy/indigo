/*
 *
 * NMEA library
 * URL: http://nmea.sourceforge.net
 * Author: Marc Chalain <marc.chalain@gmail.com>
 * Licence: http://www.gnu.org/licenses/lgpl.html
 * $Id: extended.c 4 2007-08-27 $
 *
 */

#ifdef __GNUC__
#define _GNU_SOURCE
#include <dlfcn.h>
#endif

#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>

#include <pthread.h>

#include "nmea/parse.h"
#include "nmea/parser.h"
#include "nmea/extended.h"

static int nmea_gpsfd = -1;
static nmeaPARSER nmea_parser;
static nmeaINFO nmea_info;
static struct timespec nmea_tp;
static struct timespec nmea_deltatp = {0, 0};
static int (*pclock_gettime)(clockid_t, struct timespec *);
static pthread_t nmea_pthread;
static pthread_attr_t nmea_attr;
static int nmea_thread_running = 0;

static void _nmea_readdatetime(nmeaINFO *info, struct timespec *tp, int satinuse);

static void *_thread_readdatetime(void *);

__attribute__((constructor)) void nmea_init()
{
	const char defaultdevicename[] = "/dev/ttyS0";
	char *devicename;
	void *launchpthread = 0;

#ifdef _GNU_SOURCE
	pclock_gettime = (int (*)(clockid_t, struct timespec *))dlsym(RTLD_NEXT, "clock_gettime");
	launchpthread = (void *)dlsym(RTLD_NEXT, "pthread_create");
#else
	pclock_gettime = clock_gettime;
#endif

	devicename = getenv("NMEA_TTYGPS");
	if (!devicename)
		devicename = (char *)defaultdevicename;

	nmea_gpsfd = open(devicename, O_RDONLY);
	if (nmea_gpsfd > 2 )
	{
		nmea_parser_init(&nmea_parser);
		nmea_zero_INFO(&nmea_info);

		if (launchpthread)
		{
			pthread_attr_init(&nmea_attr);
			pthread_attr_setdetachstate(&nmea_attr, PTHREAD_CREATE_JOINABLE);

			nmea_thread_running = 1;
			if (pthread_create(&nmea_pthread, &nmea_attr, _thread_readdatetime, NULL) != 0)
			{
				close(nmea_gpsfd);
				nmea_gpsfd = -1;
			}
		}
		else
		{
			_nmea_readdatetime(&nmea_info, &nmea_tp, 3);
		}
	}

}

__attribute__((destructor)) void nmea_deinit()
{
	if (nmea_gpsfd > 2 )
		close(nmea_gpsfd);
}

static void *_thread_readdatetime(void *unused)
{
	_nmea_readdatetime(&nmea_info, &nmea_tp, 2);
	nmea_thread_running = 0;
	return NULL;
}

static void _nmea_readdatetime(nmeaINFO *info, struct timespec *tp, int satinuse)
{
	int size = 100;
	char buff[101];

	do
	{
		size = read(nmea_gpsfd, buff, size);
		if (size > 0)
		{
			nmea_parse(&nmea_parser, &buff[0], size, info);
			if (nmea_info.satinfo.inuse > satinuse)
			{
				struct tm gpstime;

				gpstime.tm_year = nmea_info.utc.year;
				gpstime.tm_mon = nmea_info.utc.mon;
				gpstime.tm_mday = nmea_info.utc.day;
				gpstime.tm_hour = nmea_info.utc.hour;
				gpstime.tm_min = nmea_info.utc.min;
				gpstime.tm_sec = nmea_info.utc.sec;
				gpstime.tm_isdst = 0;
				/*
				 * mktime espects a local time and not UTC
				 * timezone is the global variable from <time.h>
				 */
				gpstime.tm_hour -= (timezone / 3600);
				tp->tv_sec = mktime(&gpstime);
				tp->tv_nsec = nmea_info.utc.hsec * 10000000;
				if (info->smask & TP_ZDA) {
					pclock_gettime(CLOCK_REALTIME, &nmea_deltatp);
					nmea_info.smask &= ~TP_ZDA;
				}
			}
		}
		else if ((errno != EINTR || nmea_thread_running) && errno != EAGAIN)
		{
			close(nmea_gpsfd);
			nmea_gpsfd = -1;
			break;
		}
	} while (nmea_thread_running || !(info->smask & (TP_ZDA | TP_GGA | TP_RMC)));
}

int nmea_gettime(clockid_t clk_id, struct timespec *tp)
{
	if (CLOCK_GPS == clk_id && nmea_gpsfd > 2)
	{
		if (! nmea_thread_running)
			_nmea_readdatetime(&nmea_info, &nmea_tp, 2);
		if (nmea_info.satinfo.inuse > 2)
		{
			struct timespec now = {0, 0};
			if (nmea_deltatp.tv_sec > 0)
			{
				pclock_gettime(CLOCK_REALTIME, &now);
				now.tv_sec -= nmea_deltatp.tv_sec;
				now.tv_nsec -= nmea_deltatp.tv_nsec;
				now.tv_nsec += now.tv_sec * 1000000000;
			}
			tp->tv_sec = nmea_tp.tv_sec + now.tv_sec;
			tp->tv_nsec = nmea_tp.tv_nsec + now.tv_nsec;
			nmea_info.smask &= ~(TP_ZDA | TP_GGA | TP_RMC);
			return 0;
		}
		else if (CLOCK_GPS == clk_id)
		{
			errno = EINVAL;
			return -1;
		}
	}
	return pclock_gettime(clk_id, tp);
}

int nmea_getpos(struct gpsposition *po)
{
	if (nmea_gpsfd > 2)
	{
		po->latitude = nmea_info.lat;
		po->longitude = nmea_info.lon;
		return 0;
	}
	return -1;
}

#ifdef _GNU_SOURCE
int clock_gettime(clockid_t clk_id, struct timespec *tp) __attribute__ ((weak, alias ("nmea_gettime")));
#endif
