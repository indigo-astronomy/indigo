// Copyright (c) 2019 Thomas Stibor
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
// 2.0 by Thomas Stibor

/** INDIGO GPS GPSD driver
 \file indigo_gps_gpsd.c
 */

#define DRIVER_VERSION	0x0004
#define DRIVER_NAME	"indigo_gps_gpsd"

#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <math.h>
#include <assert.h>
#include <gps.h>
#include <indigo/indigo_driver_xml.h>
#include <indigo/indigo_io.h>
#include "indigo_gps_gpsd.h"

#define PRIVATE_DATA    ((gpsd_private_data *)device->private_data)

typedef struct {
	struct gps_data_t gps_data;
} gpsd_private_data;


static bool gpsd_open(indigo_device *device) {
	int rc;
	char *text = DEVICE_PORT_ITEM->text.value;
	char host_name[INDIGO_NAME_SIZE] = {0};
	char port[15] = {0};

	if (!strncmp(text, "gpsd://", 7)) {
		text += 7; // just skip 'gpsd://' prefix
	}
	char *colon = strchr(text, ':');
	if (colon == NULL) {
		strcpy(host_name, text);
		strcpy(port, "2947");
	} else {
		strncpy(host_name, text, colon - text);
		strcpy(port, colon + 1);
	}

	rc = gps_open(host_name, port, &PRIVATE_DATA->gps_data);
	if (rc) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "Failed to connect to gpsd://%s:%s", host_name, port);
		return false;
	}
	(void)gps_stream(&PRIVATE_DATA->gps_data, WATCH_ENABLE | WATCH_JSON, NULL);

	INDIGO_DRIVER_LOG(DRIVER_NAME, "Connected to gpsd://%s:%s", host_name, port);
	return true;
}


static void gpsd_close(indigo_device *device) {
	int rc;

	(void)gps_stream(&PRIVATE_DATA->gps_data, WATCH_DISABLE, NULL);
	rc = gps_close(&PRIVATE_DATA->gps_data);
	if (rc) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "Failed to disconnect from gpsd.");
	} else {
		INDIGO_DRIVER_LOG(DRIVER_NAME, "Disconnected from gpsd.");
	}
}


static indigo_result gps_attach(indigo_device *device) {
	assert(device != NULL);
	assert(PRIVATE_DATA != NULL);

	if (indigo_gps_attach(device, DRIVER_NAME, DRIVER_VERSION) == INDIGO_OK) {
		DEVICE_PORT_PROPERTY->hidden = false;
		DEVICE_PORT_PROPERTY->state = INDIGO_OK_STATE;
		strcpy(DEVICE_PORT_PROPERTY->label, "GPS daemon host");
		strcpy(DEVICE_PORT_ITEM->label, "Hostname (host:port)");
		strcpy(DEVICE_PORT_ITEM->text.value, "gpsd://localhost:2947");
		DEVICE_PORTS_PROPERTY->hidden = true;
		DEVICE_BAUDRATE_PROPERTY->hidden = true;
		GPS_ADVANCED_PROPERTY->hidden = false;
		GPS_GEOGRAPHIC_COORDINATES_PROPERTY->hidden = false;
		GPS_GEOGRAPHIC_COORDINATES_PROPERTY->count = 3;
		GPS_UTC_TIME_PROPERTY->hidden = false;
		GPS_UTC_TIME_PROPERTY->count = 1;
		INDIGO_DEVICE_ATTACH_LOG(DRIVER_NAME, device->name);

		return indigo_gps_enumerate_properties(device, NULL, NULL);
	}
	return INDIGO_FAILED;
}


static void gps_refresh_callback(indigo_device *device) {
	assert(device != NULL);
	assert(PRIVATE_DATA != NULL);

	bool recv_data;
	int rc;

	while (IS_CONNECTED) {
		recv_data = gps_waiting(&PRIVATE_DATA->gps_data, 200000);
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "gps_waiting(): %d", recv_data);
		if (!recv_data) {
			GPS_STATUS_PROPERTY->state = INDIGO_BUSY_STATE;
			/* Waiting for too long will result in buffereing and reading old
			   messages thus the the displayed time will progressively fall behind
			 */
			indigo_usleep(100);
			continue;
		}

		rc = gps_read(&PRIVATE_DATA->gps_data, NULL, 0);
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "gps_read(): bytes read %d, set: %lu", rc, PRIVATE_DATA->gps_data.set);
		if (rc == -1) {
			INDIGO_DRIVER_ERROR(DRIVER_NAME, "gps_read(): %s", gps_errstr(rc));
			GPS_STATUS_PROPERTY->state = INDIGO_ALERT_STATE;
			indigo_sleep(1);
			continue;
		}

		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "set TIME_SET: %d", PRIVATE_DATA->gps_data.set & TIME_SET);
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "set LATLON_SET: %d", PRIVATE_DATA->gps_data.set & LATLON_SET);
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "set ALTITUDE_SET: %d", PRIVATE_DATA->gps_data.set & ALTITUDE_SET);
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "set MODE_SET: %d", PRIVATE_DATA->gps_data.set & MODE_SET);
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "set DOP_SET: %d", PRIVATE_DATA->gps_data.set & DOP_SET);
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "set STATUS_SET: %d", PRIVATE_DATA->gps_data.set & STATUS_SET);
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "set SATELLITE_SET: %d", PRIVATE_DATA->gps_data.set & SATELLITE_SET);

		GPS_UTC_TIME_PROPERTY->state = INDIGO_BUSY_STATE;
		GPS_GEOGRAPHIC_COORDINATES_PROPERTY->state = INDIGO_BUSY_STATE;
		GPS_STATUS_PROPERTY->state = INDIGO_BUSY_STATE;
		GPS_ADVANCED_STATUS_PROPERTY->state = INDIGO_OK_STATE;

		GPS_STATUS_NO_FIX_ITEM->light.value = INDIGO_IDLE_STATE;
		GPS_STATUS_2D_FIX_ITEM->light.value = INDIGO_IDLE_STATE;
		GPS_STATUS_3D_FIX_ITEM->light.value = INDIGO_IDLE_STATE;

		if (PRIVATE_DATA->gps_data.set & TIME_SET) {
			char isotime[INDIGO_VALUE_SIZE] = {0};

			indigo_timetoisogm(PRIVATE_DATA->gps_data.fix.time.tv_sec,
					   isotime, sizeof(isotime));
			indigo_copy_value(GPS_UTC_ITEM->text.value, isotime);
			GPS_UTC_TIME_PROPERTY->state = INDIGO_OK_STATE;
		}
		if (PRIVATE_DATA->gps_data.set & LATLON_SET) {
			GPS_GEOGRAPHIC_COORDINATES_LONGITUDE_ITEM->number.value = PRIVATE_DATA->gps_data.fix.longitude;
			GPS_GEOGRAPHIC_COORDINATES_LATITUDE_ITEM->number.value = PRIVATE_DATA->gps_data.fix.latitude;
			GPS_GEOGRAPHIC_COORDINATES_PROPERTY->state = INDIGO_OK_STATE;
		}
		if (PRIVATE_DATA->gps_data.set & ALTITUDE_SET) {
			GPS_GEOGRAPHIC_COORDINATES_ELEVATION_ITEM->number.value = PRIVATE_DATA->gps_data.fix.altitude;
			GPS_GEOGRAPHIC_COORDINATES_PROPERTY->state = INDIGO_OK_STATE;
		}
		if (PRIVATE_DATA->gps_data.set & MODE_SET) {
			if (PRIVATE_DATA->gps_data.fix.mode == MODE_NO_FIX) {
				GPS_STATUS_NO_FIX_ITEM->light.value = INDIGO_ALERT_STATE;
			}
			if (PRIVATE_DATA->gps_data.fix.mode == MODE_2D) {
				GPS_STATUS_2D_FIX_ITEM->light.value = INDIGO_BUSY_STATE;
			}
			if (PRIVATE_DATA->gps_data.fix.mode == MODE_3D) {
				GPS_STATUS_3D_FIX_ITEM->light.value = INDIGO_OK_STATE;
			}

			if (PRIVATE_DATA->gps_data.fix.mode == MODE_NOT_SEEN) {
				GPS_STATUS_PROPERTY->state = INDIGO_BUSY_STATE;
			} else {
				GPS_STATUS_PROPERTY->state = INDIGO_OK_STATE;
			}
		}
		/* DOP_SET does not seem to be set even when there is DOP data */
		if (!isnan(PRIVATE_DATA->gps_data.dop.pdop))
			GPS_ADVANCED_STATUS_PDOP_ITEM->number.value = PRIVATE_DATA->gps_data.dop.pdop;
		if (!isnan(PRIVATE_DATA->gps_data.dop.hdop))
			GPS_ADVANCED_STATUS_HDOP_ITEM->number.value = PRIVATE_DATA->gps_data.dop.hdop;
		if (!isnan(PRIVATE_DATA->gps_data.dop.vdop))
			GPS_ADVANCED_STATUS_VDOP_ITEM->number.value = PRIVATE_DATA->gps_data.dop.vdop;

		if (PRIVATE_DATA->gps_data.set & SATELLITE_SET) {
			GPS_ADVANCED_STATUS_SVS_IN_USE_ITEM->number.value = PRIVATE_DATA->gps_data.satellites_used;
			GPS_ADVANCED_STATUS_SVS_IN_VIEW_ITEM->number.value = PRIVATE_DATA->gps_data.satellites_visible;
			if (PRIVATE_DATA->gps_data.set & DOP_SET) {
				GPS_ADVANCED_STATUS_PROPERTY->state = INDIGO_OK_STATE;
			}
		}

		indigo_update_property(device, GPS_STATUS_PROPERTY, NULL);
		indigo_update_property(device, GPS_GEOGRAPHIC_COORDINATES_PROPERTY, NULL);
		indigo_update_property(device, GPS_UTC_TIME_PROPERTY, NULL);
		if (GPS_ADVANCED_ENABLED_ITEM->sw.value) {
			indigo_update_property(device, GPS_ADVANCED_STATUS_PROPERTY, NULL);
		}
	}
}

static void gps_connect_callback(indigo_device *device) {
	if (CONNECTION_CONNECTED_ITEM->sw.value) {
		if (gpsd_open(device)) {
			GPS_GEOGRAPHIC_COORDINATES_PROPERTY->state = INDIGO_BUSY_STATE;
			GPS_GEOGRAPHIC_COORDINATES_LONGITUDE_ITEM->number.value = 0;
			GPS_GEOGRAPHIC_COORDINATES_LATITUDE_ITEM->number.value = 0;
			GPS_GEOGRAPHIC_COORDINATES_ELEVATION_ITEM->number.value = 0;
			GPS_STATUS_NO_FIX_ITEM->light.value = INDIGO_IDLE_STATE;
			GPS_STATUS_2D_FIX_ITEM->light.value = INDIGO_IDLE_STATE;
			GPS_STATUS_3D_FIX_ITEM->light.value = INDIGO_IDLE_STATE;
			GPS_STATUS_PROPERTY->state = INDIGO_BUSY_STATE;
			GPS_UTC_TIME_PROPERTY->state = INDIGO_BUSY_STATE;
			sprintf(GPS_UTC_ITEM->text.value, "0000-00-00T00:00:00.00");
			indigo_set_timer(device, 0, gps_refresh_callback, NULL);
			CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
		} else {
			indigo_set_switch(CONNECTION_PROPERTY, CONNECTION_DISCONNECTED_ITEM, true);
			CONNECTION_PROPERTY->state = INDIGO_ALERT_STATE;
		}
	} else {
		gpsd_close(device);
		CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
	}
	indigo_gps_change_property(device, NULL, CONNECTION_PROPERTY);
}



static indigo_result gps_change_property(indigo_device *device, indigo_client *client, indigo_property *property) {
	assert(device != NULL);
	assert(DEVICE_CONTEXT != NULL);
	assert(property != NULL);

	if (indigo_property_match_changeable(CONNECTION_PROPERTY, property)) {
		if (indigo_ignore_connection_change(device, property))
			return INDIGO_OK;
		indigo_property_copy_values(CONNECTION_PROPERTY, property, false);
		CONNECTION_PROPERTY->state = INDIGO_BUSY_STATE;
		indigo_update_property(device, CONNECTION_PROPERTY, NULL);
		indigo_set_timer(device, 0, gps_connect_callback, NULL);
		return INDIGO_OK;
	}

	return indigo_gps_change_property(device, client, property);
}

static indigo_result gps_detach(indigo_device *device) {
	assert(device != NULL);

	if (IS_CONNECTED) {
		indigo_set_switch(CONNECTION_PROPERTY, CONNECTION_DISCONNECTED_ITEM, true);
		gps_connect_callback(device);
	}
	INDIGO_DEVICE_DETACH_LOG(DRIVER_NAME, device->name);

	return indigo_gps_detach(device);
}

static gpsd_private_data *private_data = NULL;
static indigo_device *gps = NULL;

indigo_result indigo_gps_gpsd(indigo_driver_action action, indigo_driver_info *info) {
	static indigo_device gps_template = INDIGO_DEVICE_INITIALIZER(
		GPS_GPSD_DEVICE_NAME,
		gps_attach,
		indigo_gps_enumerate_properties,
		gps_change_property,
		NULL,
		gps_detach
	);

	static indigo_driver_action last_action = INDIGO_DRIVER_SHUTDOWN;

	SET_DRIVER_INFO(info, GPS_GPSD_DRIVER_DESCRIPTION, __FUNCTION__, DRIVER_VERSION, false, last_action);

	if (action == last_action) {
		return INDIGO_OK;
	}

	switch (action) {
		case INDIGO_DRIVER_INIT:
			last_action = action;
			private_data = indigo_safe_malloc(sizeof(gpsd_private_data));
			gps = indigo_safe_malloc_copy(sizeof(indigo_device), &gps_template);
			gps->private_data = private_data;
			indigo_attach_device(gps);
			break;

		case INDIGO_DRIVER_SHUTDOWN:
			VERIFY_NOT_CONNECTED(gps);
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
