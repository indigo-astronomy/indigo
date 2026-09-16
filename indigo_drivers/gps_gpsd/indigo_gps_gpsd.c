// Copyright (c) 2019-2026 Thomas Stibor
// All rights reserved.

// You may use this software under the terms of 'INDIGO Astronomy
// open-source license' (see LICENSE.md).

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

// This file generated from indigo_gps_gpsd.driver

#pragma mark - Includes

#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <assert.h>

//+ include

#include <gps.h>

//- include

#include <indigo/indigo_driver_xml.h>
#include <indigo/indigo_gps_driver.h>
#include <indigo/indigo_uni_io.h>

#include "indigo_gps_gpsd.h"

#pragma mark - Common definitions

#define DRIVER_VERSION       0x03000005
#define DRIVER_NAME          "indigo_gps_gpsd"
#define DRIVER_LABEL         "GPS Sevice Daemon (GPSD) Client"
#define GPS_DEVICE_NAME      "GPSD Client"
#define PRIVATE_DATA         ((gpsd_private_data *)device->private_data)

//+ define

#define GPSD_DEFAULT_PORT    "2947"
#define GPSD_MAX_MESSAGES    32
#define GPSD_POLL_DELAY      0.1

//- define

#pragma mark - Private data definition

typedef struct {
	indigo_uni_handle *handle;
	//+ data
	struct gps_data_t gps_data;
	//- data
} gpsd_private_data;

#pragma mark - Low level code

//+ code

static bool gpsd_open(indigo_device *device) {
	char *text = DEVICE_PORT_ITEM->text.value;
	char host_name[INDIGO_NAME_SIZE] = { 0 };
	char port[15] = { 0 };
	if (!strncmp(text, "gpsd://", 7)) {
		text += 7;
	}
	char *colon = strchr(text, ':');
	if (colon == NULL) {
		if (strlen(text) >= sizeof(host_name)) {
			INDIGO_DRIVER_ERROR(DRIVER_NAME, "Host name too long");
			return false;
		}
		snprintf(host_name, sizeof(host_name), "%s", text);
		snprintf(port, sizeof(port), "%s", GPSD_DEFAULT_PORT);
	} else {
		if (colon - text >= (ptrdiff_t)sizeof(host_name)) {
			INDIGO_DRIVER_ERROR(DRIVER_NAME, "Host name too long");
			return false;
		}
		if (strlen(colon + 1) >= sizeof(port)) {
			INDIGO_DRIVER_ERROR(DRIVER_NAME, "Port value too long");
			return false;
		}
		snprintf(host_name, sizeof(host_name), "%.*s", (int)(colon - text), text);
		snprintf(port, sizeof(port), "%s", colon + 1);
	}
	if (gps_open(host_name, port, &PRIVATE_DATA->gps_data)) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "Failed to connect to gpsd://%s:%s", host_name, port);
		return false;
	}
	(void)gps_stream(&PRIVATE_DATA->gps_data, WATCH_ENABLE | WATCH_JSON, NULL);
	INDIGO_DRIVER_LOG(DRIVER_NAME, "Connected to gpsd://%s:%s", host_name, port);
	return true;
}

static void gpsd_close(indigo_device *device) {
	(void)gps_stream(&PRIVATE_DATA->gps_data, WATCH_DISABLE, NULL);
	if (gps_close(&PRIVATE_DATA->gps_data)) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "Failed to disconnect from gpsd.");
	} else {
		INDIGO_DRIVER_LOG(DRIVER_NAME, "Disconnected from gpsd.");
	}
}

static void gps_connection_handler(indigo_device *device);

//- code

//+ gps.code

static void gpsd_publish(indigo_device *device) {
	struct gps_data_t *gps_data = &PRIVATE_DATA->gps_data;
	INDIGO_DRIVER_DEBUG(DRIVER_NAME, "gps_read(): set: %llx", (unsigned long long)gps_data->set);
	GPS_UTC_TIME_PROPERTY->state = INDIGO_BUSY_STATE;
	GPS_GEOGRAPHIC_COORDINATES_PROPERTY->state = INDIGO_BUSY_STATE;
	GPS_STATUS_PROPERTY->state = INDIGO_BUSY_STATE;
	GPS_ADVANCED_STATUS_PROPERTY->state = INDIGO_OK_STATE;
	GPS_STATUS_NO_FIX_ITEM->light.value = INDIGO_IDLE_STATE;
	GPS_STATUS_2D_FIX_ITEM->light.value = INDIGO_IDLE_STATE;
	GPS_STATUS_3D_FIX_ITEM->light.value = INDIGO_IDLE_STATE;
	if (gps_data->set & TIME_SET) {
		char isotime[INDIGO_VALUE_SIZE] = { 0 };
		indigo_timetoisogm(gps_data->fix.time.tv_sec, isotime, sizeof(isotime));
		INDIGO_COPY_VALUE(GPS_UTC_ITEM->text.value, isotime);
		GPS_UTC_TIME_PROPERTY->state = INDIGO_OK_STATE;
	}
	if (gps_data->set & LATLON_SET) {
		GPS_GEOGRAPHIC_COORDINATES_LONGITUDE_ITEM->number.value = gps_data->fix.longitude;
		GPS_GEOGRAPHIC_COORDINATES_LATITUDE_ITEM->number.value = gps_data->fix.latitude;
		GPS_GEOGRAPHIC_COORDINATES_PROPERTY->state = INDIGO_OK_STATE;
	}
	if (gps_data->set & ALTITUDE_SET) {
		/* "alt" is deprecated since gpsd 3.20; fall back to the documented altMSL/altHAE fields */
		double altitude = isfinite(gps_data->fix.altitude) ? gps_data->fix.altitude : (isfinite(gps_data->fix.altMSL) ? gps_data->fix.altMSL : gps_data->fix.altHAE);
		if (isfinite(altitude)) {
			GPS_GEOGRAPHIC_COORDINATES_ELEVATION_ITEM->number.value = altitude;
			GPS_GEOGRAPHIC_COORDINATES_PROPERTY->state = INDIGO_OK_STATE;
		}
	}
	if (gps_data->set & MODE_SET) {
		if (gps_data->fix.mode == MODE_NO_FIX) {
			GPS_STATUS_NO_FIX_ITEM->light.value = INDIGO_ALERT_STATE;
		}
		if (gps_data->fix.mode == MODE_2D) {
			GPS_STATUS_2D_FIX_ITEM->light.value = INDIGO_BUSY_STATE;
		}
		if (gps_data->fix.mode == MODE_3D) {
			GPS_STATUS_3D_FIX_ITEM->light.value = INDIGO_OK_STATE;
		}
		if (gps_data->fix.mode != MODE_NOT_SEEN) {
			GPS_STATUS_PROPERTY->state = INDIGO_OK_STATE;
		}
	}
	/* DOP_SET does not seem to be set even when there is DOP data */
	if (!isnan(gps_data->dop.pdop)) {
		GPS_ADVANCED_STATUS_PDOP_ITEM->number.value = gps_data->dop.pdop;
	}
	if (!isnan(gps_data->dop.hdop)) {
		GPS_ADVANCED_STATUS_HDOP_ITEM->number.value = gps_data->dop.hdop;
	}
	if (!isnan(gps_data->dop.vdop)) {
		GPS_ADVANCED_STATUS_VDOP_ITEM->number.value = gps_data->dop.vdop;
	}
	if (gps_data->set & SATELLITE_SET) {
		GPS_ADVANCED_STATUS_SVS_IN_USE_ITEM->number.value = gps_data->satellites_used;
		GPS_ADVANCED_STATUS_SVS_IN_VIEW_ITEM->number.value = gps_data->satellites_visible;
	}
	indigo_update_property(device, GPS_STATUS_PROPERTY, NULL);
	indigo_update_property(device, GPS_GEOGRAPHIC_COORDINATES_PROPERTY, NULL);
	indigo_update_property(device, GPS_UTC_TIME_PROPERTY, NULL);
	if (GPS_ADVANCED_ENABLED_ITEM->sw.value) {
		indigo_update_property(device, GPS_ADVANCED_STATUS_PROPERTY, NULL);
	}
}

//- gps.code

#pragma mark - High level code (gps)

static void gps_timer_callback(indigo_device *device) {
	if (!IS_CONNECTED) {
		return;
	}
	//+ gps.on_timer
	for (int processed = 0; processed < GPSD_MAX_MESSAGES; processed++) {
		if (!gps_waiting(&PRIVATE_DATA->gps_data, 0)) {
			indigo_execute_handler_in(device, GPSD_POLL_DELAY, gps_timer_callback);
			return;
		}
		int result = gps_read(&PRIVATE_DATA->gps_data, NULL, 0);
		if (result == -1) {
			INDIGO_DRIVER_ERROR(DRIVER_NAME, "gps_read(): %s", gps_errstr(result));
			indigo_set_switch(CONNECTION_PROPERTY, CONNECTION_DISCONNECTED_ITEM, true);
			gps_connection_handler(device);
			CONNECTION_PROPERTY->state = INDIGO_ALERT_STATE;
			indigo_update_property(device, CONNECTION_PROPERTY, "Connection to gpsd lost");
			return;
		}
		if (result == 0) {
			indigo_execute_handler_in(device, GPSD_POLL_DELAY, gps_timer_callback);
			return;
		}
		gpsd_publish(device);
	}
	indigo_execute_handler(device, gps_timer_callback);
	//- gps.on_timer
}

static void gps_connection_handler(indigo_device *device) {
	if (CONNECTION_CONNECTED_ITEM->sw.value) {
		bool connection_result = true;
		connection_result = gpsd_open(device);
		if (connection_result) {
			//+ gps.on_connect
			GPS_GEOGRAPHIC_COORDINATES_PROPERTY->state = INDIGO_BUSY_STATE;
			GPS_GEOGRAPHIC_COORDINATES_LONGITUDE_ITEM->number.value = 0;
			GPS_GEOGRAPHIC_COORDINATES_LATITUDE_ITEM->number.value = 0;
			GPS_GEOGRAPHIC_COORDINATES_ELEVATION_ITEM->number.value = 0;
			GPS_STATUS_NO_FIX_ITEM->light.value = INDIGO_IDLE_STATE;
			GPS_STATUS_2D_FIX_ITEM->light.value = INDIGO_IDLE_STATE;
			GPS_STATUS_3D_FIX_ITEM->light.value = INDIGO_IDLE_STATE;
			GPS_STATUS_PROPERTY->state = INDIGO_BUSY_STATE;
			GPS_UTC_TIME_PROPERTY->state = INDIGO_BUSY_STATE;
			INDIGO_COPY_VALUE(GPS_UTC_ITEM->text.value, "0000-00-00T00:00:00.00");
			GPS_ADVANCED_STATUS_SVS_IN_USE_ITEM->number.value = 0;
			GPS_ADVANCED_STATUS_SVS_IN_VIEW_ITEM->number.value = 0;
			GPS_ADVANCED_STATUS_PDOP_ITEM->number.value = 0;
			GPS_ADVANCED_STATUS_HDOP_ITEM->number.value = 0;
			GPS_ADVANCED_STATUS_VDOP_ITEM->number.value = 0;
			GPS_ADVANCED_STATUS_PROPERTY->state = INDIGO_BUSY_STATE;
			//- gps.on_connect
		}
		if (connection_result) {
			indigo_execute_handler(device, gps_timer_callback);
			CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
			indigo_send_message(device, OK_PROPERTY, "Connected to %s on %s", GPS_DEVICE_NAME, DEVICE_PORT_ITEM->text.value);
		} else {
			indigo_send_message(device, ALERT_PROPERTY, "Failed to connect to %s on %s", GPS_DEVICE_NAME, DEVICE_PORT_ITEM->text.value);
			CONNECTION_PROPERTY->state = INDIGO_ALERT_STATE;
			indigo_set_switch(CONNECTION_PROPERTY, CONNECTION_DISCONNECTED_ITEM, true);
		}
	} else {
		indigo_cancel_pending_handlers(device);
		gpsd_close(device);
		indigo_send_message(device, OK_PROPERTY, "Disconnected from %s", device->name);
		CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
	}
	indigo_gps_change_property(device, NULL, CONNECTION_PROPERTY);
}

#pragma mark - Device API (gps)

static indigo_result gps_enumerate_properties(indigo_device *device, indigo_client *client, indigo_property *property);

static indigo_result gps_attach(indigo_device *device) {
	if (indigo_gps_attach(device, DRIVER_NAME, DRIVER_VERSION) == INDIGO_OK) {
		DEVICE_PORT_PROPERTY->hidden = false;
		//+ gps.on_attach
		DEVICE_PORT_PROPERTY->state = INDIGO_OK_STATE;
		INDIGO_COPY_VALUE(DEVICE_PORT_PROPERTY->label, "GPS daemon host");
		INDIGO_COPY_VALUE(DEVICE_PORT_ITEM->label, "Hostname (host:port)");
		INDIGO_COPY_VALUE(DEVICE_PORT_ITEM->text.value, "gpsd://localhost:2947");
		GPS_ADVANCED_PROPERTY->hidden = false;
		GPS_GEOGRAPHIC_COORDINATES_PROPERTY->hidden = false;
		GPS_GEOGRAPHIC_COORDINATES_PROPERTY->count = 3;
		GPS_UTC_TIME_PROPERTY->hidden = false;
		GPS_UTC_TIME_PROPERTY->count = 1;
		//- gps.on_attach
		INDIGO_DEVICE_ATTACH_LOG(DRIVER_NAME, device->name);
		return gps_enumerate_properties(device, NULL, NULL);
	}
	return INDIGO_FAILED;
}

static indigo_result gps_enumerate_properties(indigo_device *device, indigo_client *client, indigo_property *property) {
	return indigo_gps_enumerate_properties(device, client, property);
}

static indigo_result gps_change_property(indigo_device *device, indigo_client *client, indigo_property *property) {
	if (indigo_property_match_changeable(CONNECTION_PROPERTY, property)) {
		if (!indigo_ignore_connection_change(device, property)) {
			indigo_property_copy_values(CONNECTION_PROPERTY, property, false);
			INDIGO_UPDATE_PROPERTY_STATE(CONNECTION_PROPERTY, INDIGO_BUSY_STATE, NULL);
			indigo_execute_handler(device, gps_connection_handler);
		}
		return INDIGO_OK;
	}
	return indigo_gps_change_property(device, client, property);
}

static indigo_result gps_detach(indigo_device *device) {
	if (IS_CONNECTED) {
		indigo_set_switch(CONNECTION_PROPERTY, CONNECTION_DISCONNECTED_ITEM, true);
		gps_connection_handler(device);
	}
	INDIGO_DEVICE_DETACH_LOG(DRIVER_NAME, device->name);
	return indigo_gps_detach(device);
}

#pragma mark - Device templates

static indigo_device gps_template = INDIGO_DEVICE_INITIALIZER(GPS_DEVICE_NAME, gps_attach, gps_enumerate_properties, gps_change_property, NULL, gps_detach);

#pragma mark - Main code

indigo_result indigo_gps_gpsd(indigo_driver_action action, indigo_driver_info *info) {
	static indigo_driver_action last_action = INDIGO_DRIVER_SHUTDOWN;
	static gpsd_private_data *private_data = NULL;
	static indigo_device *gps = NULL;

	SET_DRIVER_INFO(info, DRIVER_LABEL, __FUNCTION__, DRIVER_VERSION, false, last_action);

	if (action == last_action) {
		return INDIGO_OK;
	}

	switch (action) {
		case INDIGO_DRIVER_INIT: {
			last_action = action;
			private_data = (gpsd_private_data *)indigo_safe_malloc(sizeof(gpsd_private_data));
			gps = (indigo_device *)indigo_safe_malloc_copy(sizeof(indigo_device), &gps_template);
			gps->private_data = private_data;
			indigo_attach_device(gps);
			break;

		}
		case INDIGO_DRIVER_SHUTDOWN: {
			VERIFY_NOT_CONNECTED(gps);
			last_action = action;
			if (gps != NULL) {
				indigo_detach_device(gps);
				indigo_safe_free(gps);
				gps = NULL;
			}
			if (private_data != NULL) {
				indigo_safe_free(private_data);
				private_data = NULL;
			}
			break;

		}
		case INDIGO_DRIVER_INFO:
			break;
	}

	return INDIGO_OK;
}
