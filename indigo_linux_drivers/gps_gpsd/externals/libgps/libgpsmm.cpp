/*
 * Copyright (C) 2005 Alfredo Pironti
 *
 * SPDX-License-Identifier: BSD-2-clause
 */

#include "gpsd_config.h"  /* must be before all includes */

#include <cstdlib>
#include "libgpsmm.h"

struct gps_data_t* gpsmm::gps_inner_open(const char *host, const char *port)
{
    const bool err = (gps_open(host, port, gps_state()) != 0);
    if ( err ) {
	to_user = NULL;
	return NULL;
    }
    else { // connection successfully opened
	to_user= new struct gps_data_t;
	return backup(); //we return the backup of our internal structure
    }
}

struct gps_data_t* gpsmm::stream(int flags)
{
    if (to_user == NULL)
	return NULL;
    else if (gps_stream(gps_state(),flags, NULL)==-1) {
	return NULL;
    }
    else {
	return backup();
    }
}

struct gps_data_t* gpsmm::send(const char *request)
{
    if (gps_send(gps_state(),request)==-1) {
	return NULL;
    }
    else {
	return backup();
    }
}

struct gps_data_t* gpsmm::read(void)
{
    if (gps_read(gps_state(), NULL, 0)<=0) {
	// we return null if there was a read() error, if no
	// data was ready in POLL_NOBLOCK mode, or if the
	// connection is closed by gpsd
	return NULL;
    }
    else {
	return backup();
    }
}

bool gpsmm::waiting(int timeout)
{
    return gps_waiting(gps_state(), timeout);
}

const char *gpsmm::data(void)
{
    return gps_data(gps_state());
}

// cppcheck-suppress unusedFunction
void gpsmm::clear_fix(void)
{
    gps_clear_fix(&(gps_state()->fix));
}

// cppcheck-suppress unusedFunction
void gpsmm::enable_debug(int level, FILE *fp)
{
#ifdef CLIENTDEBUG_ENABLE
    gps_enable_debug(level, fp);
#endif /* CLIENTDEBUG_ENABLE */
}

// cppcheck-suppress unusedFunction
bool gpsmm::is_open(void)
{
	return to_user != NULL;
}

gpsmm::~gpsmm()
{
    if ( to_user != NULL ) {
	(void)gps_close(gps_state());
	delete to_user;
    }
}
