/****************************************************************************

NAME
   rtcm3_json.c - deserialize RTCM3 JSON

DESCRIPTION
   This module uses the generic JSON parser to get data from RTCM3
representations to libgps structures.

PERMISSIONS
   This file is Copyright (c) 2013-2018 by the GPSD project
   SPDX-License-Identifier: BSD-2-clause

***************************************************************************/

#include "gpsd_config.h"  /* must be before all includes */

#include <stdio.h>
#include <math.h>
#include <string.h>
#include <stddef.h>

#include "gpsd.h"

#ifdef SOCKET_EXPORT_ENABLE
#include "gps_json.h"

int json_rtcm3_read(const char *buf,
		    char *path, size_t pathlen, struct rtcm3_t *rtcm3,
		    const char **endptr)
{
    static char *stringptrs[NITEMS(rtcm3->rtcmtypes.data)];
    static char stringstore[sizeof(rtcm3->rtcmtypes.data) * 2];
    static int stringcount;

/* *INDENT-OFF* */
#define RTCM3_HEADER \
	{"class",          t_check,    .dflt.check = "RTCM3"}, \
	{"type",           t_uinteger, .addr.uinteger = &rtcm3->type}, \
	{"device",         t_string,   .addr.string = path, .len = pathlen}, \
	{"length",         t_uinteger, .addr.uinteger = &rtcm3->length},

    int status = 0, satcount = 0;

#define RTCM3FIELD(type, fld)	STRUCTOBJECT(struct rtcm3_ ## type ## _t, fld)
    const struct json_attr_t rtcm1001_satellite[] = {
	{"ident",     t_uinteger, RTCM3FIELD(1001, ident)},
	{"ind",       t_uinteger, RTCM3FIELD(1001, L1.indicator)},
	{"prange",    t_real,     RTCM3FIELD(1001, L1.pseudorange)},
	{"delta",     t_real,     RTCM3FIELD(1001, L1.rangediff)},
	{"lockt",     t_uinteger, RTCM3FIELD(1001, L1.locktime)},
	{NULL},
    };

    const struct json_attr_t rtcm1002_satellite[] = {
	{"ident",     t_uinteger, RTCM3FIELD(1002, ident)},
	{"ind",       t_uinteger, RTCM3FIELD(1002, L1.indicator)},
	{"prange",    t_real,     RTCM3FIELD(1002, L1.pseudorange)},
	{"delta",     t_real,     RTCM3FIELD(1002, L1.rangediff)},
	{"lockt",     t_uinteger, RTCM3FIELD(1002, L1.locktime)},
	{"amb",       t_uinteger, RTCM3FIELD(1002, L1.ambiguity)},
	{"CNR",       t_real,     RTCM3FIELD(1002, L1.CNR)},
	{NULL},
    };

    const struct json_attr_t rtcm1009_satellite[] = {
	{"ident",     t_uinteger, RTCM3FIELD(1009, ident)},
	{"ind",       t_uinteger, RTCM3FIELD(1009, L1.indicator)},
	{"channel",   t_uinteger, RTCM3FIELD(1009, L1.channel)},
	{"prange",    t_real,     RTCM3FIELD(1009, L1.pseudorange)},
	{"delta",     t_real,     RTCM3FIELD(1009, L1.rangediff)},
	{"lockt",     t_uinteger, RTCM3FIELD(1009, L1.locktime)},
	{NULL},
    };

    const struct json_attr_t rtcm1010_satellite[] = {
	{"ident",     t_uinteger, RTCM3FIELD(1010, ident)},
	{"ind",       t_uinteger, RTCM3FIELD(1010, L1.indicator)},
	{"channel",   t_uinteger, RTCM3FIELD(1010, L1.channel)},
	{"prange",    t_real,     RTCM3FIELD(1010, L1.pseudorange)},
	{"delta",     t_real,     RTCM3FIELD(1010, L1.rangediff)},
	{"lockt",     t_uinteger, RTCM3FIELD(1010, L1.locktime)},
	{"amb",       t_uinteger, RTCM3FIELD(1010, L1.ambiguity)},
	{"CNR",       t_real,     RTCM3FIELD(1010, L1.CNR)},
	{NULL},
    };
#undef RTCM3FIELD

#define R1001	&rtcm3->rtcmtypes.rtcm3_1001.header
    const struct json_attr_t json_rtcm1001[] = {
	RTCM3_HEADER
	{"station_id", t_uinteger, .addr.uinteger = R1001.station_id},
	{"tow",        t_uinteger, .addr.uinteger = (unsigned int *)R1001.tow},
        {"sync",       t_boolean,  .addr.boolean = R1001.sync},
        {"smoothing",  t_boolean,  .addr.boolean = R1001.smoothing},
	{"interval",   t_uinteger, .addr.uinteger = R1001.interval},
        {"satellites", t_array,     STRUCTARRAY(rtcm3->rtcmtypes.rtcm3_1001.rtk_data,
					    rtcm1001_satellite, &satcount)},
	{NULL},
    };
#undef R1001

#define R1002	&rtcm3->rtcmtypes.rtcm3_1002.header
    const struct json_attr_t json_rtcm1002[] = {
	RTCM3_HEADER
	{"station_id", t_uinteger, .addr.uinteger = R1002.station_id},
	{"tow",        t_uinteger, .addr.uinteger = (unsigned int *)R1002.tow},
        {"sync",       t_boolean,  .addr.boolean = R1002.sync},
        {"smoothing",  t_boolean,  .addr.boolean = R1002.smoothing},
	{"interval",   t_uinteger, .addr.uinteger = R1002.interval},
        {"satellites", t_array,     STRUCTARRAY(rtcm3->rtcmtypes.rtcm3_1002.rtk_data,
					    rtcm1002_satellite, &satcount)},
	{NULL},
    };
#undef R1002

#define R1007	rtcm3->rtcmtypes.rtcm3_1007
    const struct json_attr_t json_rtcm1007[] = {
	RTCM3_HEADER
	{"station_id", t_uinteger, .addr.uinteger = &R1007.station_id},
	{"desc",       t_string,   .addr.string = R1007.descriptor,
	                                 .len = sizeof(R1007.descriptor)},
	{"setup_id",   t_uinteger, .addr.uinteger = &R1007.setup_id},
	{NULL},
    };
#undef R1002

#define R1008	rtcm3->rtcmtypes.rtcm3_1008
    const struct json_attr_t json_rtcm1008[] = {
	RTCM3_HEADER
	{"station_id", t_uinteger, .addr.uinteger = &R1008.station_id},
	{"desc",       t_string,   .addr.string = R1008.descriptor,
	                                 .len = sizeof(R1008.descriptor)},
	{"setup_id",   t_uinteger, .addr.uinteger = &R1008.setup_id},
	{"serial",     t_string,   .addr.string = R1008.serial,
	                                 .len = sizeof(R1008.serial)},
	{NULL},
    };
#undef R1008

#define R1009	&rtcm3->rtcmtypes.rtcm3_1009.header
    const struct json_attr_t json_rtcm1009[] = {
	RTCM3_HEADER
	{"station_id", t_uinteger, .addr.uinteger = R1009.station_id},
	{"tow",        t_uinteger, .addr.uinteger = (unsigned int *)R1009.tow},
        {"sync",       t_boolean,  .addr.boolean = R1009.sync},
        {"smoothing",  t_boolean,  .addr.boolean = R1009.smoothing},
	{"interval",   t_uinteger, .addr.uinteger = R1009.interval},
        {"satellites", t_array,    STRUCTARRAY(rtcm3->rtcmtypes.rtcm3_1009.rtk_data,
					    rtcm1009_satellite, &satcount)},
	{NULL},
    };
#undef R1010

#define R1010	&rtcm3->rtcmtypes.rtcm3_1010.header
    const struct json_attr_t json_rtcm1010[] = {
	RTCM3_HEADER
	{"station_id", t_uinteger, .addr.uinteger = R1010.station_id},
	{"tow",        t_uinteger, .addr.uinteger = (unsigned int *)R1010.tow},
        {"sync",       t_boolean,  .addr.boolean = R1010.sync},
        {"smoothing",  t_boolean,  .addr.boolean = R1010.smoothing},
	{"interval",   t_uinteger, .addr.uinteger = R1010.interval},
        {"satellites", t_array,     STRUCTARRAY(rtcm3->rtcmtypes.rtcm3_1010.rtk_data,
					    rtcm1010_satellite, &satcount)},
	{NULL},
    };
#undef R1010

#define R1014	&rtcm3->rtcmtypes.rtcm3_1014
    const struct json_attr_t json_rtcm1014[] = {
	RTCM3_HEADER
	{"netid",      t_uinteger, .addr.uinteger = R1014.network_id},
	{"subnetid",   t_uinteger, .addr.uinteger = R1014.subnetwork_id},
	{"statcount",  t_uinteger, .addr.uinteger = R1014.stationcount},
	{"master",     t_uinteger, .addr.uinteger = R1014.master_id},
	{"aux",        t_uinteger, .addr.uinteger = R1014.aux_id},
	{"lat",        t_real,     .addr.real = R1014.d_lat},
	{"lon",        t_real,     .addr.real = R1014.d_lon},
	{"alt",        t_real,     .addr.real = R1014.d_alt},
	{NULL},
    };
#undef R1014

#define R1033	rtcm3->rtcmtypes.rtcm3_1033
    const struct json_attr_t json_rtcm1033[] = {
	RTCM3_HEADER
	{"station_id", t_uinteger, .addr.uinteger = &R1033.station_id},
	{"desc",       t_string,   .addr.string = R1033.descriptor,
	                                 .len = sizeof(R1033.descriptor)},
	{"setup_id",   t_uinteger, .addr.uinteger = &R1033.setup_id},
	{"serial",     t_string,   .addr.string = R1033.serial,
	                                 .len = sizeof(R1033.serial)},
	{"receiver",   t_string,   .addr.string = R1033.receiver,
	                                 .len = sizeof(R1033.receiver)},
	{"firmware",   t_string,   .addr.string = R1033.firmware,
	                                 .len = sizeof(R1033.firmware)},
	{NULL},
    };
#undef R1033

    const struct json_attr_t json_rtcm3_fallback[] = {
	RTCM3_HEADER
	{"data",     t_array, .addr.array.element_type = t_string,
	                             .addr.array.arr.strings.ptrs = stringptrs,
	                             .addr.array.arr.strings.store = stringstore,
	                             .addr.array.arr.strings.storelen = sizeof(stringstore),
	                             .addr.array.count = &stringcount,
	                             .addr.array.maxlen = NITEMS(stringptrs)},
	{NULL},
    };

#undef RTCM3_HEADER
/* *INDENT-ON* */

    memset(rtcm3, '\0', sizeof(struct rtcm3_t));

    if (strstr(buf, "\"type\":1001,") != NULL) {
	status = json_read_object(buf, json_rtcm1001, endptr);
	if (status == 0)
	    rtcm3->rtcmtypes.rtcm3_1001.header.satcount = (unsigned short)satcount;
    } else if (strstr(buf, "\"type\":1002,") != NULL) {
	status = json_read_object(buf, json_rtcm1002, endptr);
	if (status == 0)
	    rtcm3->rtcmtypes.rtcm3_1002.header.satcount = (unsigned short)satcount;
    } else if (strstr(buf, "\"type\":1007,") != NULL) {
	status = json_read_object(buf, json_rtcm1007, endptr);
    } else if (strstr(buf, "\"type\":1008,") != NULL) {
	status = json_read_object(buf, json_rtcm1008, endptr);
    } else if (strstr(buf, "\"type\":1009,") != NULL) {
	status = json_read_object(buf, json_rtcm1009, endptr);
    } else if (strstr(buf, "\"type\":1010,") != NULL) {
	status = json_read_object(buf, json_rtcm1010, endptr);
    } else if (strstr(buf, "\"type\":1014,") != NULL) {
	status = json_read_object(buf, json_rtcm1014, endptr);
    } else if (strstr(buf, "\"type\":1033,") != NULL) {
	status = json_read_object(buf, json_rtcm1033, endptr);
    } else {
	int n;
	status = json_read_object(buf, json_rtcm3_fallback, endptr);
	for (n = 0; n < NITEMS(rtcm3->rtcmtypes.data); n++) {
	    if (n >= stringcount) {
		rtcm3->rtcmtypes.data[n] = '\0';
	    } else {
		unsigned int u;
		int fldcount = sscanf(stringptrs[n], "0x%02x\n", &u);
		if (fldcount != 1)
		    return JSON_ERR_MISC;
		else
		    rtcm3->rtcmtypes.data[n] = (char)u;
	    }
	}
    }
    return status;
}
#endif /* SOCKET_EXPORT_ENABLE */

/* rtcm3_json.c ends here */
