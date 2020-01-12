/****************************************************************************

NAME
   shared_json.c - move data between in-core and JSON structures

DESCRIPTION
   This module uses the generic JSON parser to get data from JSON
representations to gps.h structures. These functions are used in both
the daemon and the client library.

PERMISSIONS
  Written by Eric S. Raymond, 2009
  This file is Copyright (c) 2009-2018 The GPSD project
  SPDX-License-Identifier: BSD-2-clause

***************************************************************************/

#include "gpsd_config.h"  /* must be before all includes */

#include <math.h>
#include <stdbool.h>

#include "gpsd.h"
#ifdef SOCKET_EXPORT_ENABLE
#include "gps_json.h"
#include "timespec.h"

int json_device_read(const char *buf,
		     struct devconfig_t *dev,
		     const char **endptr)
{
    double d_cycle, d_mincycle;
    /* *INDENT-OFF* */
    const struct json_attr_t json_attrs_device[] = {
	{"class",      t_check,      .dflt.check = "DEVICE"},

        {"path",       t_string,     .addr.string  = dev->path,
	                                .len = sizeof(dev->path)},
        // odd, device->gpsdata.online is sent, but put in dev->activated?
	{"activated",  t_time,       .addr.ts = &dev->activated,
				        .dflt.ts = {0, 0}},
	{"flags",      t_integer,    .addr.integer = &dev->flags},
	{"driver",     t_string,     .addr.string  = dev->driver,
	                                .len = sizeof(dev->driver)},
	{"subtype",    t_string,     .addr.string  = dev->subtype,
	                                .len = sizeof(dev->subtype)},
	{"subtype1",   t_string,     .addr.string  = dev->subtype1,
	                                .len = sizeof(dev->subtype1)},
        {"hexdata",    t_string,     .addr.string  = dev->hexdata,
	                                .len = sizeof(dev->hexdata)},
	{"native",     t_integer,    .addr.integer = &dev->driver_mode,
				        .dflt.integer = DEVDEFAULT_NATIVE},
	{"bps",	       t_uinteger,   .addr.uinteger = &dev->baudrate,
				        .dflt.uinteger = DEVDEFAULT_BPS},
	{"parity",     t_character,  .addr.character = &dev->parity,
                                        .dflt.character = DEVDEFAULT_PARITY},
	{"stopbits",   t_uinteger,   .addr.uinteger = &dev->stopbits,
				        .dflt.uinteger = DEVDEFAULT_STOPBITS},
	{"cycle",      t_real,       .addr.real = &d_cycle,
				        .dflt.real = NAN},
	{"mincycle",   t_real,       .addr.real = &d_mincycle,
				        .dflt.real = NAN},
	{NULL},
    };
    /* *INDENT-ON* */
    int status;

    status = json_read_object(buf, json_attrs_device, endptr);
    if (status != 0)
	return status;

    if (0 == isfinite(d_cycle)) {
	dev->cycle.tv_sec = 0;
	dev->cycle.tv_nsec = 0;
    } else {
	DTOTS(&dev->cycle, d_cycle);
    }
    if (0 == isfinite(d_mincycle)) {
	dev->mincycle.tv_sec = 0;
	dev->mincycle.tv_nsec = 0;
    } else {
	DTOTS(&dev->cycle, d_mincycle);
    }

    return 0;
}

int json_watch_read(const char *buf,
		    struct gps_policy_t *ccp,
		    const char **endptr)
{
    bool dummy_pps_flag;
    /* *INDENT-OFF* */
    struct json_attr_t chanconfig_attrs[] = {
	{"class",          t_check,    .dflt.check = "WATCH"},

	{"enable",         t_boolean,  .addr.boolean = &ccp->watcher,
                                          .dflt.boolean = true},
	{"json",           t_boolean,  .addr.boolean = &ccp->json,
                                          .nodefault = true},
	{"raw",	           t_integer,  .addr.integer = &ccp->raw,
	                                  .nodefault = true},
	{"nmea",	   t_boolean,  .addr.boolean = &ccp->nmea,
	                                  .nodefault = true},
	{"scaled",         t_boolean,  .addr.boolean = &ccp->scaled},
	{"timing",         t_boolean,  .addr.boolean = &ccp->timing},
	{"split24",        t_boolean,  .addr.boolean = &ccp->split24},
	{"pps",            t_boolean,  .addr.boolean = &ccp->pps},
	{"device",         t_string,   .addr.string = ccp->devpath,
	                                  .len = sizeof(ccp->devpath)},
	{"remote",         t_string,   .addr.string = ccp->remote,
	                                  .len = sizeof(ccp->remote)},
	{"pps",            t_boolean,  .addr.boolean = &dummy_pps_flag,
                                          .nodefault = false},
	{NULL},
    };
    /* *INDENT-ON* */
    int status;

    status = json_read_object(buf, chanconfig_attrs, endptr);
    return status;
}

#endif /* SOCKET_EXPORT_ENABLE */

/* shared_json.c ends here */
