/****************************************************************************

NAME
   ais_json.c - deserialize AIS JSON

DESCRIPTION
   This module uses the generic JSON parser to get data from AIS
representations to libgps structures.

This file is Copyright (c)2010-2018 by the GPSD project
SPDX-License-Identifier: BSD-2-clause
***************************************************************************/

#include "gpsd_config.h"  /* must be before all includes */

#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stddef.h>
#include <time.h>

#include "gps.h"
#include "json.h"
#ifdef SOCKET_EXPORT_ENABLE
#include "libgps.h"

/* kluge because we don't want to include gpsd.h here */
extern int gpsd_hexpack(const char *, char *, size_t);

static void lenhex_unpack(const char *from,
			  size_t * plen, char *to, size_t maxlen)
{
    char *colon = strchr(from, ':');

    *plen = (size_t) atoi(from);
    if (colon != NULL)
	(void)gpsd_hexpack(colon + 1, to, maxlen);
}


int json_ais_read(const char *buf,
		  char *path, size_t pathlen, struct ais_t *ais,
		  const char **endptr)
{
    /* collected but not actually used yet */
    bool scaled;

#define AIS_HEADER \
	{"class",          t_check,    .dflt.check = "AIS"}, \
	{"type",           t_uinteger, .addr.uinteger = &ais->type}, \
	{"device",         t_string,   .addr.string = path, \
	                                  .len = pathlen}, \
	{"repeat",         t_uinteger, .addr.uinteger = &ais->repeat}, \
	{"scaled",         t_boolean,  .addr.boolean = &scaled, \
		                          .dflt.boolean = false}, \
	{"mmsi",           t_uinteger, .addr.uinteger = &ais->mmsi},

#define AIS_TYPE6 \
	{"seqno",         t_uinteger,  .addr.uinteger = &ais->type6.seqno,\
                                       .dflt.uinteger = 0},\
	{"dest_mmsi",     t_uinteger,  .addr.uinteger = &ais->type6.dest_mmsi,\
                                       .dflt.uinteger = 0},\
	{"retransmit",    t_boolean,   .addr.boolean = &ais->type6.retransmit,\
                                       .dflt.boolean = false},\
	{"dac",           t_uinteger,  .addr.uinteger = &ais->type6.dac,\
                                       .dflt.uinteger = 0},\
	{"fid",           t_uinteger,  .addr.uinteger = &ais->type6.fid,\
                                       .dflt.uinteger = 0},
#define AIS_TYPE8 \
	{"dac",           t_uinteger,  .addr.uinteger = &ais->type8.dac,\
                                       .dflt.uinteger = 0},\
	{"fid",           t_uinteger,  .addr.uinteger = &ais->type8.fid,\
                                       .dflt.uinteger = 0},

    int status;

#include "ais_json.i"		/* JSON parser template structures */

#undef AIS_HEADER

    memset(ais, '\0', sizeof(struct ais_t));

    if (strstr(buf, "\"type\":1,") != NULL
	|| strstr(buf, "\"type\":2,") != NULL
	|| strstr(buf, "\"type\":3,") != NULL) {
	status = json_read_object(buf, json_ais1, endptr);
    } else if (strstr(buf, "\"type\":4,") != NULL
	       || strstr(buf, "\"type\":11,") != NULL) {
	status = json_read_object(buf, json_ais4, endptr);
	if (status == 0) {
	    ais->type4.year = AIS_YEAR_NOT_AVAILABLE;
	    ais->type4.month = AIS_MONTH_NOT_AVAILABLE;
	    ais->type4.day = AIS_DAY_NOT_AVAILABLE;
	    ais->type4.hour = AIS_HOUR_NOT_AVAILABLE;
	    ais->type4.minute = AIS_MINUTE_NOT_AVAILABLE;
	    ais->type4.second = AIS_SECOND_NOT_AVAILABLE;
	    // We use %09u for the date to allow for dodgy years (>9999) to go through
	    // cppcheck-suppress uninitvar
	    (void)sscanf(timestamp, "%09u-%02u-%02uT%02u:%02u:%02uZ",
			 &ais->type4.year,
			 &ais->type4.month,
			 &ais->type4.day,
			 &ais->type4.hour,
			 &ais->type4.minute,
			 &ais->type4.second);
	}
    } else if (strstr(buf, "\"type\":5,") != NULL) {
	status = json_read_object(buf, json_ais5, endptr);
	if (status == 0) {
	    ais->type5.month = AIS_MONTH_NOT_AVAILABLE;
	    ais->type5.day = AIS_DAY_NOT_AVAILABLE;
	    ais->type5.hour = AIS_HOUR_NOT_AVAILABLE;
	    ais->type5.minute = AIS_MINUTE_NOT_AVAILABLE;
	    // cppcheck-suppress uninitvar
	    (void)sscanf(eta, "%02u-%02uT%02u:%02uZ",
			 &ais->type5.month,
			 &ais->type5.day,
			 &ais->type5.hour,
			 &ais->type5.minute);
	}
    } else if (strstr(buf, "\"type\":6,") != NULL) {
	bool structured = false;
	if (strstr(buf, "\"dac\":1,") != NULL) {
	    if (strstr(buf, "\"fid\":12,") != NULL) {
		status = json_read_object(buf, json_ais6_fid12, endptr);
		if (status == 0) {
		    ais->type6.dac1fid12.lmonth = AIS_MONTH_NOT_AVAILABLE;
		    ais->type6.dac1fid12.lday = AIS_DAY_NOT_AVAILABLE;
		    ais->type6.dac1fid12.lhour = AIS_HOUR_NOT_AVAILABLE;
		    ais->type6.dac1fid12.lminute = AIS_MINUTE_NOT_AVAILABLE;
		    // cppcheck-suppress uninitvar
		    (void)sscanf(departure, "%02u-%02uT%02u:%02uZ",
				 &ais->type6.dac1fid12.lmonth,
				 &ais->type6.dac1fid12.lday,
				 &ais->type6.dac1fid12.lhour,
				 &ais->type6.dac1fid12.lminute);
		    ais->type6.dac1fid12.nmonth = AIS_MONTH_NOT_AVAILABLE;
		    ais->type6.dac1fid12.nday = AIS_DAY_NOT_AVAILABLE;
		    ais->type6.dac1fid12.nhour = AIS_HOUR_NOT_AVAILABLE;
		    ais->type6.dac1fid12.nminute = AIS_MINUTE_NOT_AVAILABLE;
		    // cppcheck-suppress uninitvar
		    (void)sscanf(eta, "%02u-%02uT%02u:%02uZ",
				 &ais->type6.dac1fid12.nmonth,
				 &ais->type6.dac1fid12.nday,
				 &ais->type6.dac1fid12.nhour,
				 &ais->type6.dac1fid12.nminute);
		}
		structured = true;
	    }
	    else if (strstr(buf, "\"fid\":15,") != NULL) {
		status = json_read_object(buf, json_ais6_fid15, endptr);
		structured = true;
	    }
	    else if (strstr(buf, "\"fid\":16,") != NULL) {
		status = json_read_object(buf, json_ais6_fid16, endptr);
		structured = true;
	    }
	    else if (strstr(buf, "\"fid\":18,") != NULL) {
		status = json_read_object(buf, json_ais6_fid18, endptr);
		if (status == 0) {
		    ais->type6.dac1fid18.day = AIS_DAY_NOT_AVAILABLE;
		    ais->type6.dac1fid18.hour = AIS_HOUR_NOT_AVAILABLE;
		    ais->type6.dac1fid18.minute = AIS_MINUTE_NOT_AVAILABLE;
		    // cppcheck-suppress uninitvar
		    (void)sscanf(arrival, "%02u-%02uT%02u:%02uZ",
				 &ais->type6.dac1fid18.month,
				 &ais->type6.dac1fid18.day,
				 &ais->type6.dac1fid18.hour,
				 &ais->type6.dac1fid18.minute);
		}
		structured = true;
	    }
	    else if (strstr(buf, "\"fid\":20,") != NULL) {
		status = json_read_object(buf, json_ais6_fid20, endptr);
		if (status == 0) {
		    ais->type6.dac1fid20.month = AIS_MONTH_NOT_AVAILABLE;
		    ais->type6.dac1fid20.day = AIS_DAY_NOT_AVAILABLE;
		    ais->type6.dac1fid20.hour = AIS_HOUR_NOT_AVAILABLE;
		    ais->type6.dac1fid20.minute = AIS_MINUTE_NOT_AVAILABLE;
		    // cppcheck-suppress uninitvar
		    (void)sscanf(arrival, "%02u-%02uT%02u:%02uZ",
				 &ais->type6.dac1fid20.month,
				 &ais->type6.dac1fid20.day,
				 &ais->type6.dac1fid20.hour,
				 &ais->type6.dac1fid20.minute);
		}
		structured = true;
	    }
	    else if (strstr(buf, "\"fid\":25,") != NULL) {
		status = json_read_object(buf, json_ais6_fid25, endptr);
		structured = true;
	    }
	    else if (strstr(buf, "\"fid\":28,") != NULL) {
		status = json_read_object(buf, json_ais6_fid28, endptr);
		if (status == 0) {
		    ais->type6.dac1fid28.month = AIS_MONTH_NOT_AVAILABLE;
		    ais->type6.dac1fid28.day = AIS_DAY_NOT_AVAILABLE;
		    ais->type6.dac1fid28.hour = AIS_HOUR_NOT_AVAILABLE;
		    ais->type6.dac1fid28.minute = AIS_MINUTE_NOT_AVAILABLE;
		    // cppcheck-suppress uninitvar
		    (void)sscanf(start, "%02u-%02uT%02u:%02uZ",
				 &ais->type6.dac1fid28.month,
				 &ais->type6.dac1fid28.day,
				 &ais->type6.dac1fid28.hour,
				 &ais->type6.dac1fid28.minute);
		}
		structured = true;
	    }
	    else if (strstr(buf, "\"fid\":30,") != NULL) {
		status = json_read_object(buf, json_ais6_fid30, endptr);
		structured = true;
	    }
	    else if (strstr(buf, "\"fid\":32,") != NULL || strstr(buf, "\"fid\":14,") != NULL) {
		status = json_read_object(buf, json_ais6_fid32, endptr);
		structured = true;
	    }
	}
	else if (strstr(buf, "\"dac\":235,") != NULL || strstr(buf, "\"dac\":250,") != NULL) {
	    if (strstr(buf, "\"fid\":10,") != NULL) {
		status = json_read_object(buf, json_ais6_fid10, endptr);
		structured = true;
	    }
	}
	else if (strstr(buf, "\"dac\":200,") != NULL) {
	    if (strstr(buf, "\"fid\":21,") != NULL) {
		status = json_read_object(buf, json_ais6_fid21, endptr);
		structured = true;
		if (status == 0) {
		    ais->type6.dac200fid21.month = AIS_MONTH_NOT_AVAILABLE;
		    ais->type6.dac200fid21.day = AIS_DAY_NOT_AVAILABLE;
		    ais->type6.dac200fid21.hour = AIS_HOUR_NOT_AVAILABLE;
		    ais->type6.dac200fid21.minute = AIS_MINUTE_NOT_AVAILABLE;
		    // cppcheck-suppress uninitvar
		    (void)sscanf(eta, "%02u-%02uT%02u:%02u",
				 &ais->type6.dac200fid21.month,
				 &ais->type6.dac200fid21.day,
				 &ais->type6.dac200fid21.hour,
				 &ais->type6.dac200fid21.minute);
		}
	    }
	    else if (strstr(buf, "\"fid\":22,") != NULL) {
		status = json_read_object(buf, json_ais6_fid22, endptr);
		structured = true;
		if (status == 0) {
		    ais->type6.dac200fid22.month = AIS_MONTH_NOT_AVAILABLE;
		    ais->type6.dac200fid22.day = AIS_DAY_NOT_AVAILABLE;
		    ais->type6.dac200fid22.hour = AIS_HOUR_NOT_AVAILABLE;
		    ais->type6.dac200fid22.minute = AIS_MINUTE_NOT_AVAILABLE;
		    // cppcheck-suppress uninitvar
		    (void)sscanf(rta, "%02u-%02uT%02u:%02u",
				 &ais->type6.dac200fid22.month,
				 &ais->type6.dac200fid22.day,
				 &ais->type6.dac200fid22.hour,
				 &ais->type6.dac200fid22.minute);
		}
	    }
	    else if (strstr(buf, "\"fid\":55,") != NULL) {
		status = json_read_object(buf, json_ais6_fid55, endptr);
		structured = true;
	    }
	}
	if (!structured) {
	    status = json_read_object(buf, json_ais6, endptr);
	    if (status == 0)
		lenhex_unpack(data, &ais->type6.bitcount,
			      ais->type6.bitdata, sizeof(ais->type6.bitdata));
	}
	ais->type6.structured = structured;
    } else if (strstr(buf, "\"type\":7,") != NULL
	       || strstr(buf, "\"type\":13,") != NULL) {
	status = json_read_object(buf, json_ais7, endptr);
    } else if (strstr(buf, "\"type\":8,") != NULL) {
	bool structured = false;
	if (strstr(buf, "\"dac\":1,") != NULL) {
	    if (strstr(buf, "\"fid\":11,") != NULL) {
		status = json_read_object(buf, json_ais8_fid11, endptr);
		if (status == 0) {
		    ais->type8.dac1fid11.day = AIS_DAY_NOT_AVAILABLE;
		    ais->type8.dac1fid11.hour = AIS_HOUR_NOT_AVAILABLE;
		    ais->type8.dac1fid11.minute = AIS_MINUTE_NOT_AVAILABLE;
		    // cppcheck-suppress uninitvar
		    (void)sscanf(timestamp, "%02uT%02u:%02uZ",
				 &ais->type8.dac1fid11.day,
				 &ais->type8.dac1fid11.hour,
				 &ais->type8.dac1fid11.minute);
		}
		structured = true;
	    }
	    else if (strstr(buf, "\"fid\":13,") != NULL) {
		status = json_read_object(buf, json_ais8_fid13, endptr);
		if (status == 0) {
		    ais->type8.dac1fid13.fmonth = AIS_MONTH_NOT_AVAILABLE;
		    ais->type8.dac1fid13.fday = AIS_DAY_NOT_AVAILABLE;
		    ais->type8.dac1fid13.fhour = AIS_HOUR_NOT_AVAILABLE;
		    ais->type8.dac1fid13.fminute = AIS_MINUTE_NOT_AVAILABLE;
		    // cppcheck-suppress uninitvar
		    (void)sscanf(departure, "%02u-%02uT%02u:%02uZ",
				 &ais->type8.dac1fid13.fmonth,
				 &ais->type8.dac1fid13.fday,
				 &ais->type8.dac1fid13.fhour,
				 &ais->type8.dac1fid13.fminute);
		    ais->type8.dac1fid13.tmonth = AIS_MONTH_NOT_AVAILABLE;
		    ais->type8.dac1fid13.tday = AIS_DAY_NOT_AVAILABLE;
		    ais->type8.dac1fid13.thour = AIS_HOUR_NOT_AVAILABLE;
		    ais->type8.dac1fid13.tminute = AIS_MINUTE_NOT_AVAILABLE;
		    // cppcheck-suppress uninitvar
		    (void)sscanf(eta, "%02u-%02uT%02u:%02uZ",
				 &ais->type8.dac1fid13.tmonth,
				 &ais->type8.dac1fid13.tday,
				 &ais->type8.dac1fid13.thour,
				 &ais->type8.dac1fid13.tminute);
		}
		structured = true;
	    }
	    else if (strstr(buf, "\"fid\":15,") != NULL) {
		status = json_read_object(buf, json_ais8_fid15, endptr);
		structured = true;
	    }
	    else if (strstr(buf, "\"fid\":16,") != NULL) {
		status = json_read_object(buf, json_ais8_fid16, endptr);
		if (status == 0) {
			structured = true;
		}
	    }
	    else if (strstr(buf, "\"fid\":17,") != NULL) {
		status = json_read_object(buf, json_ais8_fid17, endptr);
		structured = true;
	    }
	    else if (strstr(buf, "\"fid\":19,") != NULL) {
		status = json_read_object(buf, json_ais8_fid19, endptr);
		structured = true;
	    }
	    else if (strstr(buf, "\"fid\":23,") != NULL) {
		status = json_read_object(buf, json_ais8_fid23, endptr);
		ais->type8.dac200fid23.start_year = AIS_YEAR_NOT_AVAILABLE;
		ais->type8.dac200fid23.start_month = AIS_MONTH_NOT_AVAILABLE;
		ais->type8.dac200fid23.start_day = AIS_DAY_NOT_AVAILABLE;
		ais->type8.dac200fid23.start_hour = AIS_HOUR_NOT_AVAILABLE;
		ais->type8.dac200fid23.start_minute = AIS_MINUTE_NOT_AVAILABLE;
		ais->type8.dac200fid23.end_year = AIS_YEAR_NOT_AVAILABLE;
		ais->type8.dac200fid23.end_month = AIS_MONTH_NOT_AVAILABLE;
		ais->type8.dac200fid23.end_day = AIS_DAY_NOT_AVAILABLE;
		ais->type8.dac200fid23.end_hour = AIS_HOUR_NOT_AVAILABLE;
		ais->type8.dac200fid23.end_minute = AIS_MINUTE_NOT_AVAILABLE;
		// cppcheck-suppress uninitvar
		(void)sscanf(start, "%09u-%02u-%02uT%02u:%02u",
			 &ais->type8.dac200fid23.start_year,
			 &ais->type8.dac200fid23.start_month,
			 &ais->type8.dac200fid23.start_day,
			 &ais->type8.dac200fid23.start_hour,
			 &ais->type8.dac200fid23.start_minute);
		// cppcheck-suppress uninitvar
		(void)sscanf(end, "%09u-%02u-%02uT%02u:%02u",
			 &ais->type8.dac200fid23.end_year,
			 &ais->type8.dac200fid23.end_month,
			 &ais->type8.dac200fid23.end_day,
			 &ais->type8.dac200fid23.end_hour,
			 &ais->type8.dac200fid23.end_minute);
		structured = true;
	    }
	    else if (strstr(buf, "\"fid\":24,") != NULL) {
		status = json_read_object(buf, json_ais8_fid24, endptr);
		structured = true;
	    }
	    else if (strstr(buf, "\"fid\":27,") != NULL) {
		status = json_read_object(buf, json_ais8_fid27, endptr);
		if (status == 0) {
		    ais->type8.dac1fid27.month = AIS_MONTH_NOT_AVAILABLE;
		    ais->type8.dac1fid27.day = AIS_DAY_NOT_AVAILABLE;
		    ais->type8.dac1fid27.hour = AIS_HOUR_NOT_AVAILABLE;
		    ais->type8.dac1fid27.minute = AIS_MINUTE_NOT_AVAILABLE;
		    // cppcheck-suppress uninitvar
		    (void)sscanf(start, "%02u-%02uT%02u:%02uZ",
				 &ais->type8.dac1fid27.month,
				 &ais->type8.dac1fid27.day,
				 &ais->type8.dac1fid27.hour,
				 &ais->type8.dac1fid27.minute);
		}
		structured = true;
	    }
	    else if (strstr(buf, "\"fid\":29,") != NULL) {
		status = json_read_object(buf, json_ais8_fid29, endptr);
		structured = true;
	    }
	    else if (strstr(buf, "\"fid\":31,") != NULL) {
		status = json_read_object(buf, json_ais8_fid31, endptr);
		if (status == 0) {
		    ais->type8.dac1fid31.day = AIS_DAY_NOT_AVAILABLE;
		    ais->type8.dac1fid31.hour = AIS_HOUR_NOT_AVAILABLE;
		    ais->type8.dac1fid31.minute = AIS_MINUTE_NOT_AVAILABLE;
		    // cppcheck-suppress uninitvar
		    (void)sscanf(timestamp, "%02uT%02u:%02uZ",
				 &ais->type8.dac1fid31.day,
				 &ais->type8.dac1fid31.hour,
				 &ais->type8.dac1fid31.minute);
		}
		structured = true;
	    }
	}
	else if (strstr(buf, "\"dac\":200,") != NULL && strstr(buf,"data")==NULL) {
	    if (strstr(buf, "\"fid\":10,") != NULL) {
		status = json_read_object(buf, json_ais8_fid10, endptr);
		structured = true;
	    }
	    if (strstr(buf, "\"fid\":40,") != NULL) {
		status = json_read_object(buf, json_ais8_fid40, endptr);
		structured = true;
	    }
	}
	if (!structured) {
	    status = json_read_object(buf, json_ais8, endptr);
	    if (status == 0)
		lenhex_unpack(data, &ais->type8.bitcount,
			      ais->type8.bitdata, sizeof(ais->type8.bitdata));
	}
	ais->type8.structured = structured;
    } else if (strstr(buf, "\"type\":9,") != NULL) {
	status = json_read_object(buf, json_ais9, endptr);
    } else if (strstr(buf, "\"type\":10,") != NULL) {
	status = json_read_object(buf, json_ais10, endptr);
    } else if (strstr(buf, "\"type\":12,") != NULL) {
	status = json_read_object(buf, json_ais12, endptr);
    } else if (strstr(buf, "\"type\":14,") != NULL) {
	status = json_read_object(buf, json_ais14, endptr);
    } else if (strstr(buf, "\"type\":15,") != NULL) {
	status = json_read_object(buf, json_ais15, endptr);
    } else if (strstr(buf, "\"type\":16,") != NULL) {
	status = json_read_object(buf, json_ais16, endptr);
    } else if (strstr(buf, "\"type\":17,") != NULL) {
	status = json_read_object(buf, json_ais17, endptr);
	if (status == 0)
	    lenhex_unpack(data, &ais->type17.bitcount,
			  ais->type17.bitdata, sizeof(ais->type17.bitdata));
    } else if (strstr(buf, "\"type\":18,") != NULL) {
	status = json_read_object(buf, json_ais18, endptr);
    } else if (strstr(buf, "\"type\":19,") != NULL) {
	status = json_read_object(buf, json_ais19, endptr);
    } else if (strstr(buf, "\"type\":20,") != NULL) {
	status = json_read_object(buf, json_ais20, endptr);
    } else if (strstr(buf, "\"type\":21,") != NULL) {
	status = json_read_object(buf, json_ais21, endptr);
    } else if (strstr(buf, "\"type\":22,") != NULL) {
	status = json_read_object(buf, json_ais22, endptr);
    } else if (strstr(buf, "\"type\":23,") != NULL) {
	status = json_read_object(buf, json_ais23, endptr);
    } else if (strstr(buf, "\"type\":24,") != NULL) {
	status = json_read_object(buf, json_ais24, endptr);
    } else if (strstr(buf, "\"type\":25,") != NULL) {
	status = json_read_object(buf, json_ais25, endptr);
	if (status == 0)
	    lenhex_unpack(data, &ais->type25.bitcount,
			  ais->type25.bitdata, sizeof(ais->type25.bitdata));
    } else if (strstr(buf, "\"type\":26,") != NULL) {
	status = json_read_object(buf, json_ais26, endptr);
	if (status == 0)
	    lenhex_unpack(data, &ais->type26.bitcount,
			  ais->type26.bitdata, sizeof(ais->type26.bitdata));
    } else if (strstr(buf, "\"type\":27,") != NULL) {
	status = json_read_object(buf, json_ais27, endptr);
    } else {
	if (endptr != NULL)
	    *endptr = NULL;
	return JSON_ERR_MISC;
    }
    return status;
}
#endif /* SOCKET_EXPORT_ENABLE */

/* ais_json.c ends here */
