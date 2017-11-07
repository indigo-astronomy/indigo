#ifndef __NMEA_EXTENDED_H__
#define __NMEA_EXTENDED_H__

#define CLOCK_GPS 17

#include <time.h>

struct gpsposition
{
	int latitude;
	int longitude;
};

#ifdef  __cplusplus
extern "C" {
#endif

int nmea_gettime(clockid_t clk_id, struct timespec *tp);
int nmea_getpos(struct gpsposition *po);

#ifdef  __cplusplus
}
#endif

#endif /* __NMEA_EXTENDED_H__ */
