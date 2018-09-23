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
// libnmea dependency removed by Peter Polakovic, new implementation based on https://www.gpsinformation.org/dale/nmea.htm

/** INDIGO GPS NMEA 0183 driver
 \file indigo_gps_nmea.c
 */

#define DRIVER_VERSION 0x0004
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

#define PRIVATE_DATA        ((nmea_private_data *)device->private_data)

typedef struct {
	int handle;
	int count_open;
	pthread_mutex_t serial_mutex;
} nmea_private_data;

// -------------------------------------------------------------------------------- INDIGO GPS device implementation

static bool gps_open(indigo_device *device) {
	pthread_mutex_lock(&PRIVATE_DATA->serial_mutex);
	if (PRIVATE_DATA->count_open == 0) {
		char *name = DEVICE_PORT_ITEM->text.value;
		if (strncmp(name, "gps://", 6)) {
			PRIVATE_DATA->handle = indigo_open_serial(name);
		} else {
			char *host = name + 8;
			char *colon = strchr(host, ':');
			if (colon == NULL) {
				PRIVATE_DATA->handle = indigo_open_tcp(host, 9999);
			} else {
				char host_name[INDIGO_NAME_SIZE];
				strncpy(host_name, host, colon - host);
				int port = atoi(colon + 1);
				PRIVATE_DATA->handle = indigo_open_tcp(host_name, port);
			}
		}
		if (PRIVATE_DATA->handle >= 0) {
			INDIGO_DRIVER_LOG(DRIVER_NAME, "Connected to %s", name);
			pthread_mutex_unlock(&PRIVATE_DATA->serial_mutex);
			PRIVATE_DATA->count_open++;
			return true;
		} else {
			INDIGO_DRIVER_ERROR(DRIVER_NAME, "Failed to connect to %s", name);
			pthread_mutex_unlock(&PRIVATE_DATA->serial_mutex);
			return false;
		}
	}
	PRIVATE_DATA->count_open++;
	pthread_mutex_unlock(&PRIVATE_DATA->serial_mutex);
	return true;
}


static void gps_close(indigo_device *device) {
	pthread_mutex_lock(&PRIVATE_DATA->serial_mutex);
	if (--PRIVATE_DATA->count_open == 0) {
		close(PRIVATE_DATA->handle);
		PRIVATE_DATA->handle = -1;
		INDIGO_DRIVER_LOG(DRIVER_NAME, "Disconnected from %s", DEVICE_PORT_ITEM->text.value);
	}
	pthread_mutex_unlock(&PRIVATE_DATA->serial_mutex);
}

static char **parse(char *buffer) {
	INDIGO_DRIVER_DEBUG(DRIVER_NAME, "%s", buffer);
	if (strncmp("$GP", buffer, 3))
		return NULL;
	char *index = strchr(buffer, '*');
	if (index) {
		*index++ = 0;
		int c1 = (int)strtol(index, NULL, 16);
		int c2 = 0;
		index = buffer + 1;
		while (*index)
			c2 ^= *index++;
		if (c1 != c2)
			return NULL;
	}
	static char *tokens[32];
	int token = 0;
	memset(tokens, 0, sizeof(tokens));
	index = buffer + 3;
	while (index) {
		tokens[token++] = index;
		index = strchr(index, ',');
		if (index)
			*index++ = 0;
	}
	return tokens;
}

static void gps_refresh_callback(indigo_device *device) {
	char buffer[128];
	char **tokens;
	INDIGO_DRIVER_LOG(DRIVER_NAME, "NMEA reader started");
	while (PRIVATE_DATA->handle > 0) {
		pthread_mutex_lock(&PRIVATE_DATA->serial_mutex);
		if (indigo_read_line(PRIVATE_DATA->handle, buffer, sizeof(buffer)) > 0 && (tokens = parse(buffer))) {
			if (!strcmp(tokens[0], "RMC")) { // Recommended Minimum sentence C
				int time = atoi(tokens[1]);
				int date = atoi(tokens[9]);
				sprintf(GPS_UTC_ITEM->text.value, "20%02d-%02d-%02dT%02d:%02d:%02d", date % 100, (date / 100) % 100, date / 10000, time / 10000, (time / 100) % 100, time % 100);
				GPS_UTC_TIME_PROPERTY->state = INDIGO_OK_STATE;
				indigo_update_property(device, GPS_UTC_TIME_PROPERTY, NULL);
				double lat = atof(tokens[3]);
				lat = floor(lat / 100) + fmod(lat, 100) / 60;
				if (!strcmp(tokens[4], "S"))
					lat = -lat;
				lat = round(lat * 10000) / 10000;
				double lon = atof(tokens[5]);
				lon = floor(lon / 100) + fmod(lon, 100) / 60;
				if (!strcmp(tokens[6], "W"))
					lon = -lon;
				lon = round(lon * 10000) / 10000;
				if (GPS_GEOGRAPHIC_COORDINATES_LONGITUDE_ITEM->number.value != lon || GPS_GEOGRAPHIC_COORDINATES_LATITUDE_ITEM->number.value != lat) {
					GPS_GEOGRAPHIC_COORDINATES_LONGITUDE_ITEM->number.value = lon;
					GPS_GEOGRAPHIC_COORDINATES_LATITUDE_ITEM->number.value = lat;
					GPS_GEOGRAPHIC_COORDINATES_PROPERTY->state = INDIGO_OK_STATE;
					indigo_update_property(device, GPS_GEOGRAPHIC_COORDINATES_PROPERTY, NULL);
				}
			} else if (!strcmp(tokens[0], "GGA")) { // Global Positioning System Fix Data
				double lat = atof(tokens[2]);
				lat = floor(lat / 100) + fmod(lat, 100) / 60;
				if (!strcmp(tokens[3], "S"))
					lat = -lat;
				lat = round(lat * 10000) / 10000;
				double lon = atof(tokens[4]);
				lon = floor(lon / 100) + fmod(lon, 100) / 60;
				if (!strcmp(tokens[5], "W"))
					lon = -lon;
				lon = round(lon * 10000) / 10000;
				double elv = round(atof(tokens[9]));
				if (GPS_GEOGRAPHIC_COORDINATES_LONGITUDE_ITEM->number.value != lon || GPS_GEOGRAPHIC_COORDINATES_LATITUDE_ITEM->number.value != lat || GPS_GEOGRAPHIC_COORDINATES_ELEVATION_ITEM->number.value != elv) {
					GPS_GEOGRAPHIC_COORDINATES_LONGITUDE_ITEM->number.value = lon;
					GPS_GEOGRAPHIC_COORDINATES_LATITUDE_ITEM->number.value = lat;
					GPS_GEOGRAPHIC_COORDINATES_ELEVATION_ITEM->number.value = elv;
					GPS_GEOGRAPHIC_COORDINATES_PROPERTY->state = INDIGO_OK_STATE;
					indigo_update_property(device, GPS_GEOGRAPHIC_COORDINATES_PROPERTY, NULL);
				}
				int in_use = atoi(tokens[7]);
				if (GPS_ADVANCED_STATUS_SVS_IN_USE_ITEM->number.value != in_use) {
					GPS_ADVANCED_STATUS_SVS_IN_USE_ITEM->number.value = in_use;
					GPS_ADVANCED_STATUS_PROPERTY->state = INDIGO_OK_STATE;
					if (GPS_ADVANCED_ENABLED_ITEM->sw.value) {
						indigo_update_property(device, GPS_ADVANCED_STATUS_PROPERTY, NULL);
					}
				}
			} else if (!strcmp(tokens[0], "GSV")) { // Satellites in view
				int in_view = atoi(tokens[3]);
				if (GPS_ADVANCED_STATUS_SVS_IN_VIEW_ITEM->number.value != in_view) {
					GPS_ADVANCED_STATUS_SVS_IN_VIEW_ITEM->number.value = in_view;
					GPS_ADVANCED_STATUS_PROPERTY->state = INDIGO_OK_STATE;
					if (GPS_ADVANCED_ENABLED_ITEM->sw.value) {
						indigo_update_property(device, GPS_ADVANCED_STATUS_PROPERTY, NULL);
					}
				}
			} else if (!strcmp(tokens[0], "GSA")) { // Satellite status
				char fix = *tokens[2] - '0';
				if (fix == 1 && GPS_STATUS_NO_FIX_ITEM->light.value != INDIGO_ALERT_STATE) {
					GPS_STATUS_NO_FIX_ITEM->light.value = INDIGO_ALERT_STATE;
					GPS_STATUS_2D_FIX_ITEM->light.value = INDIGO_IDLE_STATE;
					GPS_STATUS_3D_FIX_ITEM->light.value = INDIGO_IDLE_STATE;
					GPS_STATUS_PROPERTY->state = INDIGO_OK_STATE;
					if (GPS_GEOGRAPHIC_COORDINATES_PROPERTY->state != INDIGO_BUSY_STATE) {
						GPS_GEOGRAPHIC_COORDINATES_PROPERTY->state = INDIGO_BUSY_STATE;
						indigo_update_property(device, GPS_GEOGRAPHIC_COORDINATES_PROPERTY, NULL);
					}
					if (GPS_UTC_TIME_PROPERTY->state != INDIGO_BUSY_STATE) {
						GPS_UTC_TIME_PROPERTY->state = INDIGO_BUSY_STATE;
						indigo_update_property(device, GPS_UTC_TIME_PROPERTY, NULL);
					}
					indigo_update_property(device, GPS_STATUS_PROPERTY, NULL);
				} else if (fix == 2 && GPS_STATUS_2D_FIX_ITEM->light.value != INDIGO_BUSY_STATE) {
					GPS_STATUS_NO_FIX_ITEM->light.value = INDIGO_IDLE_STATE;
					GPS_STATUS_2D_FIX_ITEM->light.value = INDIGO_BUSY_STATE;
					GPS_STATUS_3D_FIX_ITEM->light.value = INDIGO_IDLE_STATE;
					GPS_STATUS_PROPERTY->state = INDIGO_OK_STATE;
					indigo_update_property(device, GPS_STATUS_PROPERTY, NULL);
					if (GPS_GEOGRAPHIC_COORDINATES_PROPERTY->state != INDIGO_BUSY_STATE) {
						GPS_GEOGRAPHIC_COORDINATES_PROPERTY->state = INDIGO_BUSY_STATE;
						indigo_update_property(device, GPS_GEOGRAPHIC_COORDINATES_PROPERTY, NULL);
					}
					if (GPS_UTC_TIME_PROPERTY->state != INDIGO_BUSY_STATE) {
						GPS_UTC_TIME_PROPERTY->state = INDIGO_BUSY_STATE;
						indigo_update_property(device, GPS_UTC_TIME_PROPERTY, NULL);
					}
				} else if (fix == 3 && GPS_STATUS_3D_FIX_ITEM->light.value != INDIGO_OK_STATE) {
					GPS_STATUS_NO_FIX_ITEM->light.value = INDIGO_IDLE_STATE;
					GPS_STATUS_2D_FIX_ITEM->light.value = INDIGO_IDLE_STATE;
					GPS_STATUS_3D_FIX_ITEM->light.value = INDIGO_OK_STATE;
					GPS_STATUS_PROPERTY->state = INDIGO_OK_STATE;
					if (GPS_GEOGRAPHIC_COORDINATES_PROPERTY->state != INDIGO_OK_STATE) {
						GPS_GEOGRAPHIC_COORDINATES_PROPERTY->state = INDIGO_OK_STATE;
						indigo_update_property(device, GPS_GEOGRAPHIC_COORDINATES_PROPERTY, NULL);
					}
					if (GPS_UTC_TIME_PROPERTY->state != INDIGO_OK_STATE) {
						GPS_UTC_TIME_PROPERTY->state = INDIGO_OK_STATE;
						indigo_update_property(device, GPS_UTC_TIME_PROPERTY, NULL);
					}
					indigo_update_property(device, GPS_STATUS_PROPERTY, NULL);
				}
				double pdop = atof(tokens[15]);
				double hdop = atof(tokens[16]);
				double vdop = atof(tokens[17]);
				if (GPS_ADVANCED_STATUS_PDOP_ITEM->number.value != pdop || GPS_ADVANCED_STATUS_HDOP_ITEM->number.value != hdop || GPS_ADVANCED_STATUS_VDOP_ITEM->number.value != vdop) {
					GPS_ADVANCED_STATUS_PDOP_ITEM->number.value = pdop;
					GPS_ADVANCED_STATUS_HDOP_ITEM->number.value = hdop;
					GPS_ADVANCED_STATUS_VDOP_ITEM->number.value = vdop;
					GPS_ADVANCED_STATUS_PROPERTY->state = INDIGO_OK_STATE;
					if (GPS_ADVANCED_ENABLED_ITEM->sw.value) {
						indigo_update_property(device, GPS_ADVANCED_STATUS_PROPERTY, NULL);
					}
				}
			}
		}
		pthread_mutex_unlock(&PRIVATE_DATA->serial_mutex);
	}
	INDIGO_DRIVER_LOG(DRIVER_NAME, "NMEA reader finished");
}


static indigo_result gps_attach(indigo_device *device) {
	assert(device != NULL);
	assert(PRIVATE_DATA != NULL);
	if (indigo_gps_attach(device, DRIVER_VERSION) == INDIGO_OK) {
		pthread_mutex_init(&PRIVATE_DATA->serial_mutex, NULL);
		SIMULATION_PROPERTY->hidden = true;
		DEVICE_PORT_PROPERTY->hidden = false;
		DEVICE_PORTS_PROPERTY->hidden = false;
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


static indigo_result gps_change_property(indigo_device *device, indigo_client *client, indigo_property *property) {
	assert(device != NULL);
	assert(DEVICE_CONTEXT != NULL);
	assert(property != NULL);
	// -------------------------------------------------------------------------------- CONNECTION
	if (indigo_property_match(CONNECTION_PROPERTY, property)) {
		indigo_property_copy_values(CONNECTION_PROPERTY, property, false);
		if (CONNECTION_CONNECTED_ITEM->sw.value) {
			if (PRIVATE_DATA->handle == -1) {
				if (gps_open(device)) {
					GPS_GEOGRAPHIC_COORDINATES_PROPERTY->state = INDIGO_BUSY_STATE;
					GPS_GEOGRAPHIC_COORDINATES_LONGITUDE_ITEM->number.value = 0;
					GPS_GEOGRAPHIC_COORDINATES_LATITUDE_ITEM->number.value = 0;
					GPS_GEOGRAPHIC_COORDINATES_ELEVATION_ITEM->number.value = 0;
					GPS_STATUS_NO_FIX_ITEM->light.value = INDIGO_ALERT_STATE;
					GPS_STATUS_2D_FIX_ITEM->light.value = INDIGO_IDLE_STATE;
					GPS_STATUS_3D_FIX_ITEM->light.value = INDIGO_IDLE_STATE;
					GPS_STATUS_PROPERTY->state = INDIGO_OK_STATE;
					GPS_UTC_TIME_PROPERTY->state = INDIGO_BUSY_STATE;
					sprintf(GPS_UTC_ITEM->text.value, "0000-00-00T00:00:00.00");
					indigo_set_timer(device, 0, gps_refresh_callback);
					CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
				} else {
					indigo_set_switch(CONNECTION_PROPERTY, CONNECTION_DISCONNECTED_ITEM, true);
					CONNECTION_PROPERTY->state = INDIGO_ALERT_STATE;
				}
			}
		} else {
			if (PRIVATE_DATA->handle != -1) {
				gps_close(device);
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
	INDIGO_DEVICE_DETACH_LOG(DRIVER_NAME, device->name);
	return indigo_gps_detach(device);
}

// --------------------------------------------------------------------------------

static nmea_private_data *private_data = NULL;

static indigo_device *gps = NULL;

indigo_result indigo_gps_nmea(indigo_driver_action action, indigo_driver_info *info) {
	static indigo_device gps_template = INDIGO_DEVICE_INITIALIZER(
		GPS_NMEA_NAME,
		gps_attach,
		indigo_gps_enumerate_properties,
		gps_change_property,
		NULL,
		gps_detach
	);

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
