/*
 * gpsdclient.h -- common functions for GPSD clients
 *
 * This file is Copyright (c) 2010 by the GPSD project
 * SPDX-License-Identifier: BSD-2-clause
 *
 */

#ifndef _GPSD_GPSDCLIENT_H_
#define _GPSD_GPSDCLIENT_H_

struct exportmethod_t
/* describe an export method */
{
    const char *name;
    const char *magic;
    const char *description;
};

struct fixsource_t
/* describe a data source */
{
    char *spec;		/* pointer to actual storage */
    char *server;
    char *port;
    char *device;
};

struct exportmethod_t *export_lookup(const char *);
struct exportmethod_t *export_default(void);
void export_list(FILE *);
enum unit {unspecified, imperial, nautical, metric};
enum unit gpsd_units(void);
enum deg_str_type { deg_dd, deg_ddmm, deg_ddmmss };

/* Warning: deg_to_str() not thread safe */
extern char *deg_to_str(enum deg_str_type type, double f);
extern char *deg_to_str2(enum deg_str_type type, double f,
                         char *buf, unsigned int buf_size,
                         const char *suffix_pos, const char *suffix_neg);

extern void gpsd_source_spec(const char *fromstring,
			     struct fixsource_t *source);

char *maidenhead(double n,double e);

/* this needs to match JSON_DATE_MAX in gpsd.h */
#define CLIENT_DATE_MAX	24

#endif /* _GPSDCLIENT_H_ */
/* gpsdclient.h ends here */
