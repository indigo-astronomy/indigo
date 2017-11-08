// Copyright (c) 2017 Rumen G. Bogdanovski
// All rights reserved.
//
// You can use this software under the terms of 'INDIGO Astronomy
// open-source license' (see LICENSE.md).
//
// THIS SOFTWARE IS PROVIDED BY THE AUTHORS 'AS IS' AND ANY EXPRESS
// OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
// WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY
// DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
// DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
// GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
// WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
// NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
// SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.


// version history
// 2.0 by Rumen G. Bogdanovski

/** INDIGO GPS NMEA 0183 driver
 \file indigo_gps_nmea.c
 */

#define DRIVER_VERSION 0x0002
#define DRIVER_NAME	"idnigo_gps_nmea"

#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <math.h>
#include <assert.h>

#include "indigo_driver_xml.h"
#include "indigo_io.h"
#include "indigo_gps_nmea.h"
#include "nmea/nmea.h"

const double rad2deg = 180/M_PI;

#define PRIVATE_DATA        ((nmea_private_data *)device->private_data)

// gp_bits is used as boolean
#define is_connected                   gp_bits

typedef struct {
	int handle;
	int count_open;
	nmeaINFO info;
    nmeaPARSER parser;
	pthread_mutex_t serial_mutex;
	indigo_timer *gps_timer;
} nmea_private_data;

void nmea_trace_callback(const char *str, int str_size) {
	INDIGO_DRIVER_DEBUG(DRIVER_NAME, "%s", str);
}

void nmea_error_callback(const char *str, int str_size) {
    INDIGO_DRIVER_ERROR(DRIVER_NAME, "%s", str);
}


// -------------------------------------------------------------------------------- INDIGO GPS device implementation

static bool gps_open(indigo_device *device) {
	if (device->is_connected) return false;
	pthread_mutex_lock(&PRIVATE_DATA->serial_mutex);
	if (PRIVATE_DATA->count_open++ == 0) {
		char *name = DEVICE_PORT_ITEM->text.value;
		if (strncmp(name, "gps://", 6)) {
			PRIVATE_DATA->handle = indigo_open_serial(name);
		} else {
			char *host = name + 8;
			char *colon = strchr(host, ':');
			if (colon == NULL) {
				PRIVATE_DATA->handle = indigo_open_tcp(host, 4030);
			} else {
				char host_name[INDIGO_NAME_SIZE];
				strncpy(host_name, host, colon - host);
				int port = atoi(colon + 1);
				PRIVATE_DATA->handle = indigo_open_tcp(host_name, port);
			}
		}
		if (PRIVATE_DATA->handle >= 0) {
			INDIGO_DRIVER_LOG(DRIVER_NAME, "connected to %s", name);
			pthread_mutex_unlock(&PRIVATE_DATA->serial_mutex);
			return true;
		} else {
			INDIGO_DRIVER_ERROR(DRIVER_NAME, "failed to connect to %s", name);
			pthread_mutex_unlock(&PRIVATE_DATA->serial_mutex);
			return false;
		}
	}
	pthread_mutex_unlock(&PRIVATE_DATA->serial_mutex);
	return true;
}


static void gps_close(indigo_device *device) {
	if (!device->is_connected) return;
	pthread_mutex_lock(&PRIVATE_DATA->serial_mutex);
	if (--PRIVATE_DATA->count_open == 0) {
		device->is_connected = false;
		close(PRIVATE_DATA->handle);
		PRIVATE_DATA->handle = -1;
		INDIGO_DRIVER_LOG(DRIVER_NAME, "disconnected from %s", DEVICE_PORT_ITEM->text.value);
	}
	pthread_mutex_unlock(&PRIVATE_DATA->serial_mutex);
}


static void gps_refresh_callback(indigo_device *device) {
	int prev_sec = -1, prev_fix = 0, size;
	char buff[100];
	nmeaPOS dpos;

	nmea_property()->trace_func = &nmea_trace_callback;
	nmea_property()->error_func = &nmea_error_callback;
	nmea_zero_INFO(&PRIVATE_DATA->info);
	nmea_parser_init(&PRIVATE_DATA->parser);

	while (device->is_connected) {
		pthread_mutex_lock(&PRIVATE_DATA->serial_mutex);
		size = indigo_read(PRIVATE_DATA->handle, &buff[0], 100);

        nmea_parse(&PRIVATE_DATA->parser, &buff[0], size, &PRIVATE_DATA->info);
		pthread_mutex_unlock(&PRIVATE_DATA->serial_mutex);
        nmea_info2pos(&PRIVATE_DATA->info, &dpos);

		if(prev_sec != PRIVATE_DATA->info.utc.sec) {
			prev_sec = PRIVATE_DATA->info.utc.sec;

			if (PRIVATE_DATA->info.fix != NMEA_FIX_BAD) {
				GPS_GEOGRAPHIC_COORDINATES_LONGITUDE_ITEM->number.value = rad2deg*dpos.lon;
				GPS_GEOGRAPHIC_COORDINATES_LATITUDE_ITEM->number.value = rad2deg*dpos.lat;
				GPS_GEOGRAPHIC_COORDINATES_ELEVATION_ITEM->number.value = PRIVATE_DATA->info.elv;
			}

			sprintf(
				GPS_UTC_ITEM->text.value,
				"%04d-%02d-%02dT%02d:%02d:%02d.%02d",
				PRIVATE_DATA->info.utc.year+1900,
				PRIVATE_DATA->info.utc.mon,
				PRIVATE_DATA->info.utc.day,
				PRIVATE_DATA->info.utc.hour,
				PRIVATE_DATA->info.utc.min,
				PRIVATE_DATA->info.utc.sec,
				PRIVATE_DATA->info.utc.hsec
			);

			if (PRIVATE_DATA->info.fix == NMEA_FIX_BAD) {
				GPS_STATUS_NO_FIX_ITEM->light.value = INDIGO_ALERT_STATE;
				GPS_STATUS_2D_FIX_ITEM->light.value = INDIGO_IDLE_STATE;
				GPS_STATUS_3D_FIX_ITEM->light.value = INDIGO_IDLE_STATE;
				GPS_STATUS_PROPERTY->state = INDIGO_ALERT_STATE;
				GPS_GEOGRAPHIC_COORDINATES_PROPERTY->state = INDIGO_ALERT_STATE;
				GPS_UTC_TIME_PROPERTY->state = INDIGO_ALERT_STATE;
			}

			if ( PRIVATE_DATA->info.fix == NMEA_FIX_2D) {
				GPS_STATUS_NO_FIX_ITEM->light.value = INDIGO_IDLE_STATE;
				GPS_STATUS_2D_FIX_ITEM->light.value = INDIGO_BUSY_STATE;
				GPS_STATUS_3D_FIX_ITEM->light.value = INDIGO_IDLE_STATE;
				GPS_STATUS_PROPERTY->state = INDIGO_BUSY_STATE;
				GPS_GEOGRAPHIC_COORDINATES_PROPERTY->state = INDIGO_BUSY_STATE;
				GPS_UTC_TIME_PROPERTY->state = INDIGO_OK_STATE;
			}

			if (PRIVATE_DATA->info.fix == NMEA_FIX_3D) {
				GPS_STATUS_NO_FIX_ITEM->light.value = INDIGO_IDLE_STATE;
				GPS_STATUS_2D_FIX_ITEM->light.value = INDIGO_IDLE_STATE;
				GPS_STATUS_3D_FIX_ITEM->light.value = INDIGO_OK_STATE;
				GPS_STATUS_PROPERTY->state = INDIGO_OK_STATE;
				GPS_GEOGRAPHIC_COORDINATES_PROPERTY->state = INDIGO_OK_STATE;
				GPS_UTC_TIME_PROPERTY->state = INDIGO_OK_STATE;
			}

			indigo_update_property(device, GPS_GEOGRAPHIC_COORDINATES_PROPERTY, NULL);
			indigo_update_property(device, GPS_UTC_TIME_PROPERTY, NULL);
			if (prev_fix != PRIVATE_DATA->info.fix) {
				prev_fix = PRIVATE_DATA->info.fix;
				indigo_update_property(device, GPS_STATUS_PROPERTY, NULL);
			}
		}
	}
	nmea_parser_destroy(&PRIVATE_DATA->parser);
}


static indigo_result gps_attach(indigo_device *device) {
	assert(device != NULL);
	assert(PRIVATE_DATA != NULL);
	if (indigo_gps_attach(device, DRIVER_VERSION) == INDIGO_OK) {
		pthread_mutex_init(&PRIVATE_DATA->serial_mutex, NULL);
		SIMULATION_PROPERTY->hidden = true;
		DEVICE_PORT_PROPERTY->hidden = false;
		DEVICE_PORTS_PROPERTY->hidden = false;
		GPS_GEOGRAPHIC_COORDINATES_PROPERTY->hidden = false;
		GPS_GEOGRAPHIC_COORDINATES_PROPERTY->count = 3;
		GPS_UTC_TIME_PROPERTY->hidden = false;
		GPS_UTC_TIME_PROPERTY->count = 1;

		INDIGO_DRIVER_LOG(DRIVER_NAME, "'%s' attached.", device->name);
		return indigo_gps_enumerate_properties(device, NULL, NULL);
	}
	return INDIGO_FAILED;
}


static indigo_result gps_change_property(indigo_device *device, indigo_client *client, indigo_property *property) {
	assert(device != NULL);
	assert(DEVICE_CONTEXT != NULL);
	assert(property != NULL);
	// -------------------------------------------------------------------------------- CONNECTION
	if (indigo_property_match(CONNECTION_PROPERTY, property)) {
		indigo_property_copy_values(CONNECTION_PROPERTY, property, false);
		if (CONNECTION_CONNECTED_ITEM->sw.value) {
			if (!device->is_connected) {
				if (gps_open(device)) {
					device->is_connected = true;
					CONNECTION_PROPERTY->state = INDIGO_OK_STATE;

					GPS_GEOGRAPHIC_COORDINATES_LONGITUDE_ITEM->number.value = 0;
					GPS_GEOGRAPHIC_COORDINATES_LATITUDE_ITEM->number.value = 0;
					GPS_GEOGRAPHIC_COORDINATES_ELEVATION_ITEM->number.value = 0;
					sprintf(GPS_UTC_ITEM->text.value, "0000-00-00T00:00:00.00");
					/* start updates */
					PRIVATE_DATA->gps_timer = indigo_set_timer(device, 0, gps_refresh_callback);
				} else {
					CONNECTION_PROPERTY->state = INDIGO_ALERT_STATE;
					indigo_set_switch(CONNECTION_PROPERTY, CONNECTION_DISCONNECTED_ITEM, true);
				}
			}
		} else {
			if (device->is_connected) {
				indigo_cancel_timer(device, &PRIVATE_DATA->gps_timer);
				gps_close(device);
				device->is_connected = false;
				CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
			}
		}
	}
	return indigo_gps_change_property(device, client, property);
}


static indigo_result gps_detach(indigo_device *device) {
	assert(device != NULL);
	if (CONNECTION_CONNECTED_ITEM->sw.value)
		indigo_device_disconnect(NULL, device->name);
	indigo_cancel_timer(device, &PRIVATE_DATA->gps_timer);

	INDIGO_DRIVER_LOG(DRIVER_NAME, "'%s' detached.", device->name);
	return indigo_gps_detach(device);
}

// --------------------------------------------------------------------------------

static nmea_private_data *private_data = NULL;

static indigo_device *gps = NULL;

indigo_result indigo_gps_nmea(indigo_driver_action action, indigo_driver_info *info) {
	static indigo_device gps_template = {
		GPS_NMEA_NAME, false, 0, NULL, NULL, INDIGO_OK, INDIGO_VERSION_CURRENT,
		gps_attach,
		indigo_gps_enumerate_properties,
		gps_change_property,
		NULL,
		gps_detach
	};

	static indigo_driver_action last_action = INDIGO_DRIVER_SHUTDOWN;

	SET_DRIVER_INFO(info, GPS_NMEA_NAME, __FUNCTION__, DRIVER_VERSION, last_action);

	if (action == last_action)
		return INDIGO_OK;

	switch (action) {
	case INDIGO_DRIVER_INIT:
		last_action = action;
		private_data = malloc(sizeof(nmea_private_data));
		assert(private_data != NULL);
		memset(private_data, 0, sizeof(nmea_private_data));
		private_data->count_open = 0;
		private_data->handle = -1;
		gps = malloc(sizeof(indigo_device));
		assert(gps != NULL);
		memcpy(gps, &gps_template, sizeof(indigo_device));
		gps->private_data = private_data;
		indigo_attach_device(gps);
		break;

	case INDIGO_DRIVER_SHUTDOWN:
		last_action = action;
		if (gps != NULL) {
			indigo_detach_device(gps);
			free(gps);
			gps = NULL;
		}
		if (private_data != NULL) {
			free(private_data);
			private_data = NULL;
		}
		break;

	case INDIGO_DRIVER_INFO:
		break;
	}

	return INDIGO_OK;
}
