// Copyright (c) 2020 Rumen G. Bogdanovski
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

/** INDIGO Astromi.ch MGBox driver
 \file indigo_aux_mgbox.c
 */

#define DRIVER_VERSION 0x0003
#define DRIVER_NAME	"idnigo_aux_mgbox"

#define DEFAULT_BAUDRATE "38400"

// gp_bits is used as boolean
#define is_connected               gp_bits

#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <math.h>
#include <assert.h>

#include <indigo/indigo_driver_xml.h>
#include <indigo/indigo_io.h>

#include "indigo_aux_mgbox.h"

#define PRIVATE_DATA                          ((mg_private_data *)device->private_data)

#define SETTINGS_GROUP	                      "Settings"
#define WEATHER_GROUP                         "Weather"
#define SWITCH_GROUP                          "Switch Control"


// GPS
#define X_SEND_GPS_MOUNT_PROPERTY_NAME        "X_SEND_GPS_DATA_TO_MOUNT"
#define X_SEND_GPS_MOUNT_ITEM_NAME            "ENABLED"

#define X_SEND_GPS_MOUNT_PROPERTY             (PRIVATE_DATA->gps_to_mount_property)
#define X_SEND_GPS_MOUNT_ITEM                 (X_SEND_GPS_MOUNT_PROPERTY->items + 0)

#define X_REBOOT_GPS_PROPERTY_NAME            "X_REBOOT_GPS"
#define X_REBOOT_GPS_ITEM_NAME                "REBOOT"

#define X_REBOOT_GPS_PROPERTY                 (PRIVATE_DATA->reboot_gps_property)
#define X_REBOOT_GPS_ITEM                     (X_REBOOT_GPS_PROPERTY->items + 0)

// Switch
#define AUX_OUTLET_NAMES_PROPERTY             (PRIVATE_DATA->outlet_names_property)
#define AUX_OUTLET_NAME_1_ITEM                (AUX_OUTLET_NAMES_PROPERTY->items + 0)

#define AUX_GPIO_OUTLET_PROPERTY              (PRIVATE_DATA->gpio_outlet_property)
#define AUX_GPIO_OUTLET_1_ITEM                (AUX_GPIO_OUTLET_PROPERTY->items + 0)

#define AUX_OUTLET_PULSE_LENGTHS_PROPERTY     (PRIVATE_DATA->gpio_outlet_pulse_property)
#define AUX_OUTLET_PULSE_LENGTHS_1_ITEM       (AUX_OUTLET_PULSE_LENGTHS_PROPERTY->items + 0)

// Weather
#define X_SEND_WEATHER_MOUNT_PROPERTY_NAME    "X_SEND_WEATHER_DATA_TO_MOUNT"
#define X_SEND_WEATHER_MOUNT_ITEM_NAME        "ENABLED"

#define X_SEND_WEATHER_MOUNT_PROPERTY         (PRIVATE_DATA->weather_to_mount_property)
#define X_SEND_WEATHER_MOUNT_ITEM             (X_SEND_WEATHER_MOUNT_PROPERTY->items + 0)

#define X_REBOOT_PROPERTY_NAME                 "X_REBOOT_DEVICE"
#define X_REBOOT_ITEM_NAME                     "REBOOT"

#define X_REBOOT_PROPERTY                      (PRIVATE_DATA->reboot_device_property)
#define X_REBOOT_ITEM                          (X_REBOOT_PROPERTY->items + 0)

#define X_CALIBRATION_PROPERTY_NAME           "X_WEATHER_CALIBRATION"

#define X_CALIBRATION_PROPERTY                (PRIVATE_DATA->sky_calibration_property)
#define X_CALIBRATION_TEMPERATURE_ITEM        (X_CALIBRATION_PROPERTY->items + 0)
#define X_CALIBRATION_HUMIDIDTY_ITEM          (X_CALIBRATION_PROPERTY->items + 1)
#define X_CALIBRATION_PRESSURE_ITEM           (X_CALIBRATION_PROPERTY->items + 2)

#define AUX_WEATHER_PROPERTY                  (PRIVATE_DATA->weather_property)
#define AUX_WEATHER_TEMPERATURE_ITEM          (AUX_WEATHER_PROPERTY->items + 0)
#define AUX_WEATHER_DEWPOINT_ITEM             (AUX_WEATHER_PROPERTY->items + 1)
#define AUX_WEATHER_HUMIDITY_ITEM             (AUX_WEATHER_PROPERTY->items + 2)
#define AUX_WEATHER_PRESSURE_ITEM             (AUX_WEATHER_PROPERTY->items + 3)

// DEW
#define AUX_DEW_THRESHOLD_PROPERTY            (PRIVATE_DATA->dew_threshold_property)
#define AUX_DEW_THRESHOLD_SENSOR_1_ITEM       (AUX_DEW_THRESHOLD_PROPERTY->items + 0)

#define AUX_DEW_WARNING_PROPERTY              (PRIVATE_DATA->dew_warning_property)
#define AUX_DEW_WARNING_SENSOR_1_ITEM         (AUX_DEW_WARNING_PROPERTY->items + 0)

typedef struct {
	int handle;
	int count_open;
	pthread_mutex_t serial_mutex,
	                reset_mutex;
	char firmware[INDIGO_VALUE_SIZE];
	char device_type[INDIGO_VALUE_SIZE];
	indigo_property *outlet_names_property,
	                *gpio_outlet_property,
	                *gpio_outlet_pulse_property,
	                *sky_calibration_property,
	                *weather_property,
	                *dew_threshold_property,
	                *dew_warning_property,
	                *weather_to_mount_property,
	                *gps_to_mount_property,
	                *reboot_gps_property,
	                *reboot_device_property;
} mg_private_data;

static mg_private_data *private_data = NULL;
static indigo_device *gps = NULL;
static indigo_device *aux_weather = NULL;
static indigo_timer *global_timer = NULL;

// ---------------------------- Common Stuff ----------------------------------
static char **parse(char *buffer) {
	int offset = 3;

	INDIGO_DRIVER_DEBUG(DRIVER_NAME, "%s", buffer);

	if (strncmp("$GP", buffer, 3) && strncmp("$P", buffer, 2) && strncmp("$LOG", buffer, 4)) return NULL;
	else if (buffer[1] == 'G') offset = 3;
	else if (buffer[1] == 'P') offset = 2;
	else offset = 1;

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
	static char *tokens[128];
	int token = 0;
	memset(tokens, 0, sizeof(tokens));
	index = buffer + offset;
	while (index) {
		tokens[token++] = index;
		index = strchr(index, ',');
		if (index)
			*index++ = 0;
	}
	return tokens;
}


#define update_property_if_connected(device, property, message)\
	 {\
		 if (device->is_connected) indigo_update_property(device, property, message);\
	 }


static void mg_send_command(int handle, char *command) {
	/* This device does not respond to frequent commands, so wait 1/2 seconds */
	indigo_usleep(ONE_SECOND_DELAY / 2);
	INDIGO_DRIVER_DEBUG(DRIVER_NAME, "Command -> %s", command);
	indigo_write(handle, command, strlen(command));
}


static void data_refresh_callback(indigo_device *gdevice) {
	char buffer[INDIGO_VALUE_SIZE];
	char **tokens;
	indigo_device* device;

	device = gps;
	INDIGO_DRIVER_DEBUG(DRIVER_NAME, "NMEA reader started");
	while (PRIVATE_DATA->handle >= 0) {
		pthread_mutex_lock(&PRIVATE_DATA->reset_mutex);
		int result = indigo_read_line(PRIVATE_DATA->handle, buffer, sizeof(buffer));
		pthread_mutex_unlock(&PRIVATE_DATA->reset_mutex);
		buffer[INDIGO_VALUE_SIZE-1] = '\0';
		if (result > 0 && (tokens = parse(buffer))) {
			// GPS Update
			device = gps;
			if (!strcmp(tokens[0], "RMC")) { // Recommended Minimum sentence C
				int time = atoi(tokens[1]);
				int date = atoi(tokens[9]);
				sprintf(GPS_UTC_ITEM->text.value, "20%02d-%02d-%02dT%02d:%02d:%02d", date % 100, (date / 100) % 100, date / 10000, time / 10000, (time / 100) % 100, time % 100);
				GPS_UTC_TIME_PROPERTY->state = INDIGO_OK_STATE;
				update_property_if_connected(device, GPS_UTC_TIME_PROPERTY, NULL);
				double lat = indigo_atod(tokens[3]);
				lat = floor(lat / 100) + fmod(lat, 100) / 60;
				if (!strcmp(tokens[4], "S"))
					lat = -lat;
				lat = round(lat * 10000) / 10000;
				double lon = indigo_atod(tokens[5]);
				lon = floor(lon / 100) + fmod(lon, 100) / 60;
				if (!strcmp(tokens[6], "W"))
					lon = -lon;
				lon = round(lon * 10000) / 10000;
				if (GPS_GEOGRAPHIC_COORDINATES_LONGITUDE_ITEM->number.value != lon || GPS_GEOGRAPHIC_COORDINATES_LATITUDE_ITEM->number.value != lat) {
					GPS_GEOGRAPHIC_COORDINATES_LONGITUDE_ITEM->number.value = lon;
					GPS_GEOGRAPHIC_COORDINATES_LATITUDE_ITEM->number.value = lat;
					GPS_GEOGRAPHIC_COORDINATES_PROPERTY->state = INDIGO_OK_STATE;
					update_property_if_connected(device, GPS_GEOGRAPHIC_COORDINATES_PROPERTY, NULL);
				}
			} else if (!strcmp(tokens[0], "GGA")) { // Global Positioning System Fix Data
				double lat = indigo_atod(tokens[2]);
				lat = floor(lat / 100) + fmod(lat, 100) / 60;
				if (!strcmp(tokens[3], "S"))
					lat = -lat;
				lat = round(lat * 10000) / 10000;
				double lon = indigo_atod(tokens[4]);
				lon = floor(lon / 100) + fmod(lon, 100) / 60;
				if (!strcmp(tokens[5], "W"))
					lon = -lon;
				lon = round(lon * 10000) / 10000;
				double elv = round(indigo_atod(tokens[9]));
				if (GPS_GEOGRAPHIC_COORDINATES_LONGITUDE_ITEM->number.value != lon || GPS_GEOGRAPHIC_COORDINATES_LATITUDE_ITEM->number.value != lat || GPS_GEOGRAPHIC_COORDINATES_ELEVATION_ITEM->number.value != elv) {
					GPS_GEOGRAPHIC_COORDINATES_LONGITUDE_ITEM->number.value = lon;
					GPS_GEOGRAPHIC_COORDINATES_LATITUDE_ITEM->number.value = lat;
					GPS_GEOGRAPHIC_COORDINATES_ELEVATION_ITEM->number.value = elv;
					GPS_GEOGRAPHIC_COORDINATES_PROPERTY->state = INDIGO_OK_STATE;
					update_property_if_connected(device, GPS_GEOGRAPHIC_COORDINATES_PROPERTY, NULL);
				}
				int in_use = atoi(tokens[7]);
				if (GPS_ADVANCED_STATUS_SVS_IN_USE_ITEM->number.value != in_use) {
					GPS_ADVANCED_STATUS_SVS_IN_USE_ITEM->number.value = in_use;
					GPS_ADVANCED_STATUS_PROPERTY->state = INDIGO_OK_STATE;
					if (GPS_ADVANCED_ENABLED_ITEM->sw.value) {
						update_property_if_connected(device, GPS_ADVANCED_STATUS_PROPERTY, NULL);
					}
				}
			} else if (!strcmp(tokens[0], "GSV")) { // Satellites in view
				int in_view = atoi(tokens[3]);
				if (GPS_ADVANCED_STATUS_SVS_IN_VIEW_ITEM->number.value != in_view) {
					GPS_ADVANCED_STATUS_SVS_IN_VIEW_ITEM->number.value = in_view;
					GPS_ADVANCED_STATUS_PROPERTY->state = INDIGO_OK_STATE;
					if (GPS_ADVANCED_ENABLED_ITEM->sw.value) {
						update_property_if_connected(device, GPS_ADVANCED_STATUS_PROPERTY, NULL);
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
						update_property_if_connected(device, GPS_GEOGRAPHIC_COORDINATES_PROPERTY, NULL);
					}
					if (GPS_UTC_TIME_PROPERTY->state != INDIGO_BUSY_STATE) {
						GPS_UTC_TIME_PROPERTY->state = INDIGO_BUSY_STATE;
						update_property_if_connected(device, GPS_UTC_TIME_PROPERTY, NULL);
					}
					update_property_if_connected(device, GPS_STATUS_PROPERTY, NULL);
				} else if (fix == 2 && GPS_STATUS_2D_FIX_ITEM->light.value != INDIGO_BUSY_STATE) {
					GPS_STATUS_NO_FIX_ITEM->light.value = INDIGO_IDLE_STATE;
					GPS_STATUS_2D_FIX_ITEM->light.value = INDIGO_BUSY_STATE;
					GPS_STATUS_3D_FIX_ITEM->light.value = INDIGO_IDLE_STATE;
					GPS_STATUS_PROPERTY->state = INDIGO_OK_STATE;
					update_property_if_connected(device, GPS_STATUS_PROPERTY, NULL);
					if (GPS_GEOGRAPHIC_COORDINATES_PROPERTY->state != INDIGO_BUSY_STATE) {
						GPS_GEOGRAPHIC_COORDINATES_PROPERTY->state = INDIGO_BUSY_STATE;
						update_property_if_connected(device, GPS_GEOGRAPHIC_COORDINATES_PROPERTY, NULL);
					}
					if (GPS_UTC_TIME_PROPERTY->state != INDIGO_BUSY_STATE) {
						GPS_UTC_TIME_PROPERTY->state = INDIGO_BUSY_STATE;
						update_property_if_connected(device, GPS_UTC_TIME_PROPERTY, NULL);
					}
				} else if (fix == 3 && GPS_STATUS_3D_FIX_ITEM->light.value != INDIGO_OK_STATE) {
					GPS_STATUS_NO_FIX_ITEM->light.value = INDIGO_IDLE_STATE;
					GPS_STATUS_2D_FIX_ITEM->light.value = INDIGO_IDLE_STATE;
					GPS_STATUS_3D_FIX_ITEM->light.value = INDIGO_OK_STATE;
					GPS_STATUS_PROPERTY->state = INDIGO_OK_STATE;
					if (GPS_GEOGRAPHIC_COORDINATES_PROPERTY->state != INDIGO_OK_STATE) {
						GPS_GEOGRAPHIC_COORDINATES_PROPERTY->state = INDIGO_OK_STATE;
						update_property_if_connected(device, GPS_GEOGRAPHIC_COORDINATES_PROPERTY, NULL);
					}
					if (GPS_UTC_TIME_PROPERTY->state != INDIGO_OK_STATE) {
						GPS_UTC_TIME_PROPERTY->state = INDIGO_OK_STATE;
						update_property_if_connected(device, GPS_UTC_TIME_PROPERTY, NULL);
					}
					indigo_update_property(device, GPS_STATUS_PROPERTY, NULL);
				}
				double pdop = indigo_atod(tokens[15]);
				double hdop = indigo_atod(tokens[16]);
				double vdop = indigo_atod(tokens[17]);
				if (GPS_ADVANCED_STATUS_PDOP_ITEM->number.value != pdop || GPS_ADVANCED_STATUS_HDOP_ITEM->number.value != hdop || GPS_ADVANCED_STATUS_VDOP_ITEM->number.value != vdop) {
					GPS_ADVANCED_STATUS_PDOP_ITEM->number.value = pdop;
					GPS_ADVANCED_STATUS_HDOP_ITEM->number.value = hdop;
					GPS_ADVANCED_STATUS_VDOP_ITEM->number.value = vdop;
					GPS_ADVANCED_STATUS_PROPERTY->state = INDIGO_OK_STATE;
					if (GPS_ADVANCED_ENABLED_ITEM->sw.value) {
						update_property_if_connected(device, GPS_ADVANCED_STATUS_PROPERTY, NULL);
					}
				}
			}
			/*
			// Test code to be removed when in production
			static bool inject = false;
			if (inject) {
				strcpy(buffer, "$PXDR,P,96816.0,P,0,C,24.9,C,1,H,34.0,P,2,C,8.0,C,3,1.1*04");
				tokens = parse(buffer);
				inject = false;
			} else {
				strcpy(buffer, "$LOG: Device Type: MGPBox*6B");
				tokens = parse(buffer);
				inject = true;
				if (!tokens) {
					INDIGO_DRIVER_LOG(DRIVER_NAME, "TOKEN: NULL");
				} else {
					INDIGO_DRIVER_LOG(DRIVER_NAME, "TOKEN: %s", tokens[0]);
				}
			}
			*/
			// Weather update
			device = aux_weather;
			if (!strcmp(tokens[0], "XDR")) { // Astromi.ch custom weather data
				AUX_WEATHER_PRESSURE_ITEM->number.value = indigo_atod(tokens[2]) / 100.0; // We need hPa
				AUX_WEATHER_TEMPERATURE_ITEM->number.value = indigo_atod(tokens[6]);
				AUX_WEATHER_HUMIDITY_ITEM->number.value = indigo_atod(tokens[10]);
				AUX_WEATHER_DEWPOINT_ITEM->number.value = indigo_atod(tokens[14]);
				AUX_WEATHER_PROPERTY->state = INDIGO_OK_STATE;
				update_property_if_connected(device, AUX_WEATHER_PROPERTY, NULL);
				if (PRIVATE_DATA->firmware[0] == '\0') {
					indigo_copy_value(PRIVATE_DATA->firmware, tokens[17]);
					indigo_copy_value(INFO_DEVICE_FW_REVISION_ITEM->text.value, PRIVATE_DATA->firmware);
					indigo_update_property(device, INFO_PROPERTY, NULL);
					device = gps;
					indigo_copy_value(INFO_DEVICE_FW_REVISION_ITEM->text.value, PRIVATE_DATA->firmware);
					indigo_update_property(device, INFO_PROPERTY, NULL);
					device = aux_weather;
				}
				// Dew point warning
				if (AUX_WEATHER_TEMPERATURE_ITEM->number.value - AUX_DEW_THRESHOLD_SENSOR_1_ITEM->number.value <= AUX_WEATHER_DEWPOINT_ITEM->number.value) {
					AUX_DEW_WARNING_PROPERTY->state = INDIGO_OK_STATE;
					AUX_DEW_WARNING_SENSOR_1_ITEM->light.value = INDIGO_ALERT_STATE;
				} else {
					AUX_DEW_WARNING_PROPERTY->state = INDIGO_OK_STATE;
					AUX_DEW_WARNING_SENSOR_1_ITEM->light.value = INDIGO_OK_STATE;
				}
				update_property_if_connected(device, AUX_DEW_WARNING_PROPERTY, NULL);

			} else if (!strcmp(tokens[0], "CAL")) {  // Calibratin Data
				X_CALIBRATION_PRESSURE_ITEM->number.value = indigo_atod(tokens[2]) / 10.0;
				X_CALIBRATION_TEMPERATURE_ITEM->number.value = indigo_atod(tokens[4]) / 10.0;
				X_CALIBRATION_HUMIDIDTY_ITEM->number.value = indigo_atod(tokens[6]) / 10.0;
				X_CALIBRATION_PROPERTY->state = INDIGO_OK_STATE;
				update_property_if_connected(device, X_CALIBRATION_PROPERTY, NULL);

				int i = 1;
				while (tokens[i] != NULL) {
					if (!strcmp(tokens[i++], "MM")) {
						X_SEND_WEATHER_MOUNT_ITEM->sw.value = atoi(tokens[i]);
						X_SEND_WEATHER_MOUNT_PROPERTY->state = INDIGO_OK_STATE;
						update_property_if_connected(device, X_SEND_WEATHER_MOUNT_PROPERTY, NULL);
						break;
					}
				}
				i = 1;
				while (tokens[i] != NULL) {
					if (!strcmp(tokens[i++], "MG")) {
						X_SEND_GPS_MOUNT_ITEM->sw.value = atoi(tokens[i]);
						X_SEND_GPS_MOUNT_PROPERTY->state = INDIGO_OK_STATE;
						device = gps;
						update_property_if_connected(device, X_SEND_GPS_MOUNT_PROPERTY, NULL);
						device = aux_weather;
						break;
					}
				}
			} else if (!strncmp(tokens[0], "LOG:", 4)) { // LOG messages are not coma separated
				char device_type[INDIGO_VALUE_SIZE];
				if (sscanf(tokens[0], "LOG: Device Type: %s", device_type) == 1) {
					if (PRIVATE_DATA->device_type[0] == '\0') {
						indigo_copy_value(PRIVATE_DATA->device_type, device_type);
						indigo_copy_value(INFO_DEVICE_MODEL_ITEM->text.value, PRIVATE_DATA->device_type);
						indigo_update_property(device, INFO_PROPERTY, NULL);
						device = gps;
						indigo_copy_value(INFO_DEVICE_MODEL_ITEM->text.value, PRIVATE_DATA->device_type);
						indigo_update_property(device, INFO_PROPERTY, NULL);
						device = aux_weather;
					}
				} else { // Show the log messages drom the device without the LOG: prefix
					char *message = tokens[0] + 4;
					indigo_send_message(device, message);
					INDIGO_DRIVER_DEBUG(DRIVER_NAME, "%s", message);
				}
			}
		}
	}
	INDIGO_DRIVER_DEBUG(DRIVER_NAME, "NMEA reader finished");
}

// -------------------------------------------------------------------------------- INDIGO GPS device implementation
static bool mgbox_open(indigo_device *device) {
	pthread_mutex_lock(&PRIVATE_DATA->serial_mutex);
	if (PRIVATE_DATA->count_open++ == 0) {
		char *name = DEVICE_PORT_ITEM->text.value;
		if (!indigo_is_device_url(name, "mgbox")) {
			INDIGO_DRIVER_DEBUG(DRIVER_NAME, "Opening local device on port: '%s', baudrate = %s", DEVICE_PORT_ITEM->text.value, DEVICE_BAUDRATE_ITEM->text.value);
			PRIVATE_DATA->handle = indigo_open_serial_with_speed(name, atoi(DEVICE_BAUDRATE_ITEM->text.value));
		} else {
			INDIGO_DRIVER_DEBUG(DRIVER_NAME, "Opening netwotk device on host: %s", DEVICE_PORT_ITEM->text.value);
			indigo_network_protocol proto = INDIGO_PROTOCOL_TCP;
			PRIVATE_DATA->handle = indigo_open_network_device(name, 9999, &proto);
		}
		if (PRIVATE_DATA->handle >= 0) {
			INDIGO_DRIVER_LOG(DRIVER_NAME, "Connected to %s", name);
			indigo_set_timer(gps, 0, data_refresh_callback, &global_timer);
			// To be on the safe side wait a bit after connect some arduino devices reset at connect
			indigo_usleep(ONE_SECOND_DELAY);
			// request devicetype (the device is reluctant to answer commands for some reason so try 3 times)
			int retry = 3;
			while (retry--) {
				INDIGO_DRIVER_DEBUG(DRIVER_NAME, "Trying yo identify defice (will retry %d more times)...", retry);
				mg_send_command(PRIVATE_DATA->handle, ":devicetype*");
				// wait for 2.5 seconds for a response, handled in data_refresh_callback()
				for (int i=1; i <= 25; i++) {
					indigo_usleep(ONE_SECOND_DELAY / 10);
					if (PRIVATE_DATA->device_type[0] != '\0') {
						INDIGO_DRIVER_DEBUG(DRIVER_NAME, "Device identified as '%s' in %.1f sec.", PRIVATE_DATA->device_type, i / 10.0);
						retry = 0;
						break;
					}
				}
			}

			// no responce to ":devicetype*"
			if (PRIVATE_DATA->device_type[0] == '\0') {
				close(PRIVATE_DATA->handle);
				PRIVATE_DATA->handle = -1;
				indigo_cancel_timer_sync(gps, &global_timer);
				PRIVATE_DATA->count_open--;
				PRIVATE_DATA->firmware[0] = '\0';
				PRIVATE_DATA->device_type[0] = '\0';
				INDIGO_DRIVER_ERROR(DRIVER_NAME, "Could not identify device at %s", name);
				pthread_mutex_unlock(&PRIVATE_DATA->serial_mutex);
				return false;
			}
		} else {
			INDIGO_DRIVER_ERROR(DRIVER_NAME, "Failed to connect to %s", name);
			PRIVATE_DATA->count_open--;
			pthread_mutex_unlock(&PRIVATE_DATA->serial_mutex);
			return false;
		}
	}
	pthread_mutex_unlock(&PRIVATE_DATA->serial_mutex);
	return true;
}


static void mgbox_close(indigo_device *device) {
	pthread_mutex_lock(&PRIVATE_DATA->serial_mutex);
	if (--PRIVATE_DATA->count_open == 0) {
		close(PRIVATE_DATA->handle);
		PRIVATE_DATA->handle = -1;
		indigo_cancel_timer_sync(gps, &global_timer);
		PRIVATE_DATA->firmware[0] = '\0';
		PRIVATE_DATA->device_type[0] = '\0';
		INDIGO_DRIVER_LOG(DRIVER_NAME, "Disconnected from %s", DEVICE_PORT_ITEM->text.value);
	}
	pthread_mutex_unlock(&PRIVATE_DATA->serial_mutex);
}


static void mg_reset_gps(indigo_device *device) {
	if (X_REBOOT_GPS_ITEM->sw.value) {
		pthread_mutex_lock(&PRIVATE_DATA->serial_mutex);
		pthread_mutex_lock(&PRIVATE_DATA->reset_mutex);
		mg_send_command(PRIVATE_DATA->handle, ":rebootgps*");
		indigo_usleep(2 * ONE_SECOND_DELAY);
		pthread_mutex_unlock(&PRIVATE_DATA->reset_mutex);
		pthread_mutex_unlock(&PRIVATE_DATA->serial_mutex);
		X_REBOOT_GPS_ITEM->sw.value = false;
	}
	X_REBOOT_GPS_PROPERTY->state = INDIGO_OK_STATE;
	indigo_update_property(device, X_REBOOT_GPS_PROPERTY, NULL);
}


static indigo_result gps_enumerate_properties(indigo_device *device, indigo_client *client, indigo_property *property) {
	if (IS_CONNECTED) {
		if (indigo_property_match(X_SEND_GPS_MOUNT_PROPERTY, property))
			indigo_define_property(device, X_SEND_GPS_MOUNT_PROPERTY, NULL);
		if (indigo_property_match(X_REBOOT_GPS_PROPERTY, property))
			indigo_define_property(device, X_REBOOT_GPS_PROPERTY, NULL);

	}
	return indigo_gps_enumerate_properties(device, NULL, NULL);
}


static indigo_result gps_attach(indigo_device *device) {
	assert(device != NULL);
	assert(PRIVATE_DATA != NULL);
	if (indigo_gps_attach(device, DRIVER_NAME, DRIVER_VERSION) == INDIGO_OK) {
		SIMULATION_PROPERTY->hidden = true;
		DEVICE_PORT_PROPERTY->hidden = false;
		DEVICE_PORTS_PROPERTY->hidden = false;
		DEVICE_BAUDRATE_PROPERTY->hidden = false;
		GPS_ADVANCED_PROPERTY->hidden = false;
		indigo_copy_value(DEVICE_BAUDRATE_ITEM->text.value, DEFAULT_BAUDRATE);
		GPS_GEOGRAPHIC_COORDINATES_PROPERTY->hidden = false;
		GPS_GEOGRAPHIC_COORDINATES_PROPERTY->count = 3;
		GPS_UTC_TIME_PROPERTY->hidden = false;
		GPS_UTC_TIME_PROPERTY->count = 1;
		INFO_PROPERTY->count = 6;
		//#ifdef INDIGO_LINUX
		//for (int i = 0; i < DEVICE_PORTS_PROPERTY->count; i++) {
		//	if (strstr(DEVICE_PORTS_PROPERTY->items[i].name, "ttyGPS")) {
		//		indigo_copy_value(DEVICE_PORT_ITEM->text.value, DEVICE_PORTS_PROPERTY->items[i].name);
		//		break;
		//	}
		//}
		//#endif
		//--------------------------------------------------------------------------- X_SEND_GPS_MOUNT_PROPERTY
		X_SEND_GPS_MOUNT_PROPERTY = indigo_init_switch_property(NULL, device->name, X_SEND_GPS_MOUNT_PROPERTY_NAME, SETTINGS_GROUP, "Send GPS data to mount", INDIGO_OK_STATE, INDIGO_RW_PERM, INDIGO_ANY_OF_MANY_RULE, 1);
		if (X_SEND_GPS_MOUNT_PROPERTY == NULL)
			return INDIGO_FAILED;
		indigo_init_switch_item(X_SEND_GPS_MOUNT_ITEM, X_SEND_GPS_MOUNT_ITEM_NAME, "Enable", false);
		//--------------------------------------------------------------------------- X_REBOOT_GPS_PROPERTY
		X_REBOOT_GPS_PROPERTY = indigo_init_switch_property(NULL, device->name, X_REBOOT_GPS_PROPERTY_NAME, SETTINGS_GROUP, "Reboot GPS", INDIGO_OK_STATE, INDIGO_RW_PERM, INDIGO_ANY_OF_MANY_RULE, 1);
		if (X_REBOOT_GPS_PROPERTY == NULL)
			return INDIGO_FAILED;
		indigo_init_switch_item(X_REBOOT_GPS_ITEM, X_REBOOT_GPS_ITEM_NAME, "Reboot!", false);
		//--------------------------------------------------------------------------
		INDIGO_DEVICE_ATTACH_LOG(DRIVER_NAME, device->name);
		return gps_enumerate_properties(device, NULL, NULL);
	}
	return INDIGO_FAILED;
}


static void gps_connect_callback(indigo_device *device) {
	if (CONNECTION_CONNECTED_ITEM->sw.value) {
		if (!device->is_connected) {
			if (mgbox_open(device)) {
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
				if (PRIVATE_DATA->device_type[0] != '\0') {
					indigo_copy_value(INFO_DEVICE_MODEL_ITEM->text.value, PRIVATE_DATA->device_type);
				}
				if (PRIVATE_DATA->firmware[0] != '\0') {
					indigo_copy_value(INFO_DEVICE_FW_REVISION_ITEM->text.value, PRIVATE_DATA->firmware);
				}
				if (!strchr(PRIVATE_DATA->device_type, 'G')) {
					char message[INDIGO_VALUE_SIZE];
					snprintf(message, INDIGO_VALUE_SIZE, "Model '%s' does not have GPS device", PRIVATE_DATA->device_type);
					mgbox_close(device);
					indigo_set_switch(CONNECTION_PROPERTY, CONNECTION_DISCONNECTED_ITEM, true);
					device->is_connected = false;
					CONNECTION_PROPERTY->state = INDIGO_ALERT_STATE;
					indigo_update_property(device, CONNECTION_PROPERTY, message);
					INDIGO_DRIVER_ERROR(DRIVER_NAME, "%s", message);
					return;
				}
				indigo_define_property(device, X_SEND_GPS_MOUNT_PROPERTY, NULL);
				indigo_define_property(device, X_REBOOT_GPS_PROPERTY, NULL);
				device->is_connected = true;
				CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
			} else {
				indigo_set_switch(CONNECTION_PROPERTY, CONNECTION_DISCONNECTED_ITEM, true);
				device->is_connected = false;
				CONNECTION_PROPERTY->state = INDIGO_ALERT_STATE;
			}
		}
	} else {
		if (device->is_connected) {
			indigo_delete_property(device, X_SEND_GPS_MOUNT_PROPERTY, NULL);
			indigo_delete_property(device, X_REBOOT_GPS_PROPERTY, NULL);
			mgbox_close(device);
			device->is_connected = false;
			CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
		}
	}
	indigo_gps_change_property(device, NULL, CONNECTION_PROPERTY);
}


static indigo_result gps_change_property(indigo_device *device, indigo_client *client, indigo_property *property) {
	assert(device != NULL);
	assert(DEVICE_CONTEXT != NULL);
	assert(property != NULL);
	if (indigo_property_match(CONNECTION_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- CONNECTION
		if (indigo_ignore_connection_change(device, property))
			return INDIGO_OK;
		indigo_property_copy_values(CONNECTION_PROPERTY, property, false);
		CONNECTION_PROPERTY->state = INDIGO_BUSY_STATE;
		indigo_update_property(device, CONNECTION_PROPERTY, NULL);
		indigo_set_timer(device, 0, gps_connect_callback, NULL);
		return INDIGO_OK;
	} else if (indigo_property_match(X_SEND_GPS_MOUNT_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- X_SEND_GPS_DATA_TO_MOUNT
		indigo_property_copy_values(X_SEND_GPS_MOUNT_PROPERTY, property, false);
		if (!device->is_connected) return INDIGO_OK;
		char command[INDIGO_VALUE_SIZE];
		X_SEND_GPS_MOUNT_PROPERTY->state = INDIGO_BUSY_STATE;
		indigo_update_property(device, X_SEND_GPS_MOUNT_PROPERTY, NULL);
		if (X_SEND_GPS_MOUNT_ITEM->sw.value) {
			strcpy(command,":mg,1*");
		} else {
			strcpy(command,":mg,0*");
		}
		pthread_mutex_lock(&PRIVATE_DATA->serial_mutex);
		mg_send_command(PRIVATE_DATA->handle, command);
		pthread_mutex_unlock(&PRIVATE_DATA->serial_mutex);
		return INDIGO_OK;
	} else if (indigo_property_match(X_REBOOT_GPS_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- X_REBOOT_GPS
		indigo_property_copy_values(X_REBOOT_GPS_PROPERTY, property, false);
		if (!device->is_connected) return INDIGO_OK;
		X_REBOOT_GPS_PROPERTY->state = INDIGO_BUSY_STATE;
		indigo_update_property(device, X_REBOOT_GPS_PROPERTY, NULL);
		indigo_set_timer(device, 0, mg_reset_gps, NULL);
		return INDIGO_OK;
	}
	// -------------------------------------------------------------------------------------
	return indigo_gps_change_property(device, client, property);
}


static indigo_result gps_detach(indigo_device *device) {
	assert(device != NULL);
	if (IS_CONNECTED) {
		indigo_set_switch(CONNECTION_PROPERTY, CONNECTION_DISCONNECTED_ITEM, true);
		gps_connect_callback(device);
	}
	indigo_release_property(X_SEND_GPS_MOUNT_PROPERTY);
	indigo_release_property(X_REBOOT_GPS_PROPERTY);
	INDIGO_DEVICE_DETACH_LOG(DRIVER_NAME, device->name);
	return indigo_gps_detach(device);
}


// ------------------------------------------------------------------------------ AUX device implementation
static void mg_set_callibration(indigo_device *device) {
	char command[INDIGO_VALUE_SIZE];
	pthread_mutex_lock(&PRIVATE_DATA->serial_mutex);
	sprintf(command, ":calp,%d*", (int)(X_CALIBRATION_PRESSURE_ITEM->number.target * 10));
	mg_send_command(PRIVATE_DATA->handle, command);
	sprintf(command, ":calt,%d*", (int)(X_CALIBRATION_TEMPERATURE_ITEM->number.target * 10));
	mg_send_command(PRIVATE_DATA->handle, command);
	sprintf(command, ":calh,%d*", (int)(X_CALIBRATION_HUMIDIDTY_ITEM->number.target * 10));
	mg_send_command(PRIVATE_DATA->handle, command);
	pthread_mutex_unlock(&PRIVATE_DATA->serial_mutex);
}


static void mg_pulse(indigo_device *device) {
	if (AUX_GPIO_OUTLET_1_ITEM->sw.value) {
		char command[INDIGO_VALUE_SIZE];
		sprintf(command, ":pulse,%d*", (int)(AUX_OUTLET_PULSE_LENGTHS_1_ITEM->number.target));
		pthread_mutex_lock(&PRIVATE_DATA->serial_mutex);
		mg_send_command(PRIVATE_DATA->handle, command);
		pthread_mutex_unlock(&PRIVATE_DATA->serial_mutex);
		indigo_usleep((int)(AUX_OUTLET_PULSE_LENGTHS_1_ITEM->number.target * 1000));
		AUX_GPIO_OUTLET_1_ITEM->sw.value = false;
		AUX_GPIO_OUTLET_PROPERTY->state = INDIGO_OK_STATE;
		indigo_update_property(device, AUX_GPIO_OUTLET_PROPERTY, NULL);
	}
}


static void mg_reset_device(indigo_device *device) {
	if (X_REBOOT_ITEM->sw.value) {
		pthread_mutex_lock(&PRIVATE_DATA->serial_mutex);
		pthread_mutex_lock(&PRIVATE_DATA->reset_mutex);
		mg_send_command(PRIVATE_DATA->handle, ":reboot*");
		indigo_usleep(2 * ONE_SECOND_DELAY);
		pthread_mutex_unlock(&PRIVATE_DATA->reset_mutex);
		pthread_mutex_unlock(&PRIVATE_DATA->serial_mutex);
		X_REBOOT_ITEM->sw.value = false;
	}
	X_REBOOT_PROPERTY->state = INDIGO_OK_STATE;
	indigo_update_property(device, X_REBOOT_PROPERTY, NULL);
}


static int aux_init_properties(indigo_device *device) {
	// -------------------------------------------------------------------------------- SIMULATION
	SIMULATION_PROPERTY->hidden = true;
	// -------------------------------------------------------------------------------- DEVICE_PORT
	DEVICE_PORT_PROPERTY->hidden = false;
	// -------------------------------------------------------------------------------- DEVICE_PORTS
	DEVICE_PORTS_PROPERTY->hidden = false;
	// -------------------------------------------------------------------------------- DEVICE_BAUDRATE
	DEVICE_BAUDRATE_PROPERTY->hidden = false;
	indigo_copy_value(DEVICE_BAUDRATE_ITEM->text.value, DEFAULT_BAUDRATE);
	// --------------------------------------------------------------------------------
	INFO_PROPERTY->count = 6;
	// -------------------------------------------------------------------------------- GPIO OUTLETS
	AUX_GPIO_OUTLET_PROPERTY = indigo_init_switch_property(NULL, device->name, AUX_GPIO_OUTLETS_PROPERTY_NAME, SWITCH_GROUP, "Switch outlet", INDIGO_OK_STATE, INDIGO_RW_PERM, INDIGO_ANY_OF_MANY_RULE, 1);
	if (AUX_GPIO_OUTLET_PROPERTY == NULL)
		return INDIGO_FAILED;
	indigo_init_switch_item(AUX_GPIO_OUTLET_1_ITEM, AUX_GPIO_OUTLETS_OUTLET_1_ITEM_NAME, "Pulse switch", false);
	// -------------------------------------------------------------------------------- OUTLET_NAMES
	AUX_OUTLET_NAMES_PROPERTY = indigo_init_text_property(NULL, device->name, AUX_OUTLET_NAMES_PROPERTY_NAME, SETTINGS_GROUP, "Switch name", INDIGO_OK_STATE, INDIGO_RW_PERM, 1);
	if (AUX_OUTLET_NAMES_PROPERTY == NULL)
		return INDIGO_FAILED;
	indigo_init_text_item(AUX_OUTLET_NAME_1_ITEM, AUX_GPIO_OUTLET_NAME_1_ITEM_NAME, "Switch name", "Pulse switch");
	// -------------------------------------------------------------------------------- AUX_OUTLET_PULSE_LENGTHS
	AUX_OUTLET_PULSE_LENGTHS_PROPERTY = indigo_init_number_property(NULL, device->name, "AUX_OUTLET_PULSE_LENGTHS", SWITCH_GROUP, "Switch pulse length (ms)", INDIGO_OK_STATE, INDIGO_RW_PERM, 1);
	if (AUX_OUTLET_PULSE_LENGTHS_PROPERTY == NULL)
		return INDIGO_FAILED;
	indigo_init_number_item(AUX_OUTLET_PULSE_LENGTHS_1_ITEM, AUX_GPIO_OUTLETS_OUTLET_1_ITEM_NAME, "Pulse switch", 1, 10000, 100, 1000);
	// -------------------------------------------------------------------------------- DEW_THRESHOLD
	AUX_DEW_THRESHOLD_PROPERTY = indigo_init_number_property(NULL, device->name, AUX_DEW_THRESHOLD_PROPERTY_NAME, SETTINGS_GROUP, "Dew warning threshold", INDIGO_OK_STATE, INDIGO_RW_PERM, 1);
	if (AUX_DEW_THRESHOLD_PROPERTY == NULL)
		return INDIGO_FAILED;
	indigo_init_number_item(AUX_DEW_THRESHOLD_SENSOR_1_ITEM, AUX_DEW_THRESHOLD_SENSOR_1_ITEM_NAME, "Temperature difference (°C)", 0, 9, 0, 2);
	// -------------------------------------------------------------------------------- DEW_WARNING
	AUX_DEW_WARNING_PROPERTY = indigo_init_light_property(NULL, device->name, AUX_DEW_WARNING_PROPERTY_NAME, WEATHER_GROUP, "Dew warning", INDIGO_BUSY_STATE, 1);
	if (AUX_DEW_WARNING_PROPERTY == NULL)
		return INDIGO_FAILED;
	indigo_init_light_item(AUX_DEW_WARNING_SENSOR_1_ITEM, AUX_DEW_WARNING_SENSOR_1_ITEM_NAME, "Dew warning", INDIGO_IDLE_STATE);
	// -------------------------------------------------------------------------------- X_CALIBRATION
	X_CALIBRATION_PROPERTY = indigo_init_number_property(NULL, device->name, X_CALIBRATION_PROPERTY_NAME, SETTINGS_GROUP, "Weather calibration factors", INDIGO_OK_STATE, INDIGO_RW_PERM, 3);
	if (X_CALIBRATION_PROPERTY == NULL)
		return INDIGO_FAILED;
	indigo_init_number_item(X_CALIBRATION_TEMPERATURE_ITEM, AUX_WEATHER_TEMPERATURE_ITEM_NAME, "Temperature (°C)", -200, 200, 0, 0);
	indigo_init_number_item(X_CALIBRATION_HUMIDIDTY_ITEM, AUX_WEATHER_HUMIDITY_ITEM_NAME, "Relative Humidity (%)", -99, 99, 0, 0);
	indigo_init_number_item(X_CALIBRATION_PRESSURE_ITEM, AUX_WEATHER_PRESSURE_ITEM_NAME, "Atmospheric Pressure (Pa)", -999, 999, 0, 0);
	// -------------------------------------------------------------------------------- AUX_WEATHER
	AUX_WEATHER_PROPERTY = indigo_init_number_property(NULL, device->name, AUX_WEATHER_PROPERTY_NAME, WEATHER_GROUP, "Weather conditions", INDIGO_BUSY_STATE, INDIGO_RO_PERM, 4);
	if (AUX_WEATHER_PROPERTY == NULL)
		return INDIGO_FAILED;
	indigo_init_number_item(AUX_WEATHER_TEMPERATURE_ITEM, AUX_WEATHER_TEMPERATURE_ITEM_NAME, "Ambient temperature (°C)", -200, 80, 0, 0);
	indigo_copy_value(AUX_WEATHER_TEMPERATURE_ITEM->number.format, "%.1f");
	indigo_init_number_item(AUX_WEATHER_DEWPOINT_ITEM, AUX_WEATHER_DEWPOINT_ITEM_NAME, "Dewpoint (°C)", -200, 80, 1, 0);
	indigo_copy_value(AUX_WEATHER_DEWPOINT_ITEM->number.format, "%.1f");
	indigo_init_number_item(AUX_WEATHER_HUMIDITY_ITEM, AUX_WEATHER_HUMIDITY_ITEM_NAME, "Relative humidity (%)", 0, 100, 0, 0);
	indigo_copy_value(AUX_WEATHER_HUMIDITY_ITEM->number.format, "%.1f");
	indigo_init_number_item(AUX_WEATHER_PRESSURE_ITEM, AUX_WEATHER_PRESSURE_ITEM_NAME, "Atmospheric Pressure (hPa)", 0, 10000, 0, 0);
	indigo_copy_value(AUX_WEATHER_PRESSURE_ITEM->number.format, "%.2f");
	//--------------------------------------------------------------------------- X_SEND_WEATHER_MOUNT
	X_SEND_WEATHER_MOUNT_PROPERTY = indigo_init_switch_property(NULL, device->name, X_SEND_WEATHER_MOUNT_PROPERTY_NAME, SETTINGS_GROUP, "Send weather data to mount", INDIGO_OK_STATE, INDIGO_RW_PERM, INDIGO_ANY_OF_MANY_RULE, 1);
	if (X_SEND_WEATHER_MOUNT_PROPERTY == NULL)
		return INDIGO_FAILED;
	indigo_init_switch_item(X_SEND_WEATHER_MOUNT_ITEM, X_SEND_WEATHER_MOUNT_ITEM_NAME, "Enable", false);
	//--------------------------------------------------------------------------- X_REBOOT_DEVICE
	X_REBOOT_PROPERTY = indigo_init_switch_property(NULL, device->name, X_REBOOT_PROPERTY_NAME, SETTINGS_GROUP, "Reboot device", INDIGO_OK_STATE, INDIGO_RW_PERM, INDIGO_ANY_OF_MANY_RULE, 1);
	if (X_REBOOT_PROPERTY == NULL)
		return INDIGO_FAILED;
	indigo_init_switch_item(X_REBOOT_ITEM, X_REBOOT_ITEM_NAME, "Reboot!", false);
	//---------------------------------------------------------------------------
	return INDIGO_OK;
}


static indigo_result aux_enumerate_properties(indigo_device *device, indigo_client *client, indigo_property *property) {
	if (IS_CONNECTED) {
		if (indigo_property_match(AUX_GPIO_OUTLET_PROPERTY, property))
			indigo_define_property(device, AUX_GPIO_OUTLET_PROPERTY, NULL);
		if (indigo_property_match(AUX_OUTLET_PULSE_LENGTHS_PROPERTY, property))
			indigo_define_property(device, AUX_OUTLET_PULSE_LENGTHS_PROPERTY, NULL);
		if (indigo_property_match(AUX_WEATHER_PROPERTY, property))
			indigo_define_property(device, AUX_WEATHER_PROPERTY, NULL);
		if (indigo_property_match(AUX_DEW_WARNING_PROPERTY, property))
			indigo_define_property(device, AUX_DEW_WARNING_PROPERTY, NULL);
		if (indigo_property_match(X_CALIBRATION_PROPERTY, property))
			indigo_define_property(device, X_CALIBRATION_PROPERTY, NULL);
		if (indigo_property_match(X_SEND_WEATHER_MOUNT_PROPERTY, property))
			indigo_define_property(device, X_SEND_WEATHER_MOUNT_PROPERTY, NULL);
		if (indigo_property_match(X_REBOOT_PROPERTY, property))
			indigo_define_property(device, X_REBOOT_PROPERTY, NULL);
	}
	if (indigo_property_match(AUX_OUTLET_NAMES_PROPERTY, property))
		indigo_define_property(device, AUX_OUTLET_NAMES_PROPERTY, NULL);
	if (indigo_property_match(AUX_DEW_THRESHOLD_PROPERTY, property))
		indigo_define_property(device, AUX_DEW_THRESHOLD_PROPERTY, NULL);

	return indigo_aux_enumerate_properties(device, NULL, NULL);
}


static indigo_result aux_attach(indigo_device *device) {
	assert(device != NULL);
	assert(PRIVATE_DATA != NULL);
	if (indigo_aux_attach(device, DRIVER_NAME, DRIVER_VERSION, INDIGO_INTERFACE_AUX_WEATHER | INDIGO_INTERFACE_AUX_GPIO) == INDIGO_OK) {
		if (aux_init_properties(device) != INDIGO_OK) return INDIGO_FAILED;
		PRIVATE_DATA->handle = -1;
		pthread_mutex_init(&PRIVATE_DATA->serial_mutex, NULL);
		pthread_mutex_init(&PRIVATE_DATA->reset_mutex, NULL);
		ADDITIONAL_INSTANCES_PROPERTY->hidden = DEVICE_CONTEXT->base_device != NULL;
		INDIGO_DEVICE_ATTACH_LOG(DRIVER_NAME, device->name);
		return aux_enumerate_properties(device, NULL, NULL);
	}
	return INDIGO_FAILED;
}


static void handle_aux_connect_property(indigo_device *device) {
	if (CONNECTION_CONNECTED_ITEM->sw.value) {
		if (!device->is_connected) {
			if (mgbox_open(device)) {
				if (PRIVATE_DATA->device_type[0] != '\0') {
					indigo_copy_value(INFO_DEVICE_MODEL_ITEM->text.value, PRIVATE_DATA->device_type);
				}
				if (PRIVATE_DATA->firmware[0] != '\0') {
					indigo_copy_value(INFO_DEVICE_FW_REVISION_ITEM->text.value, PRIVATE_DATA->firmware);
				}
				// request callibration data at connect
				mg_send_command(PRIVATE_DATA->handle, ":calget*");
				indigo_define_property(device, AUX_GPIO_OUTLET_PROPERTY, NULL);
				indigo_define_property(device, AUX_OUTLET_PULSE_LENGTHS_PROPERTY, NULL);
				indigo_define_property(device, AUX_WEATHER_PROPERTY, NULL);
				indigo_define_property(device, AUX_DEW_WARNING_PROPERTY, NULL);
				indigo_define_property(device, X_CALIBRATION_PROPERTY, NULL);
				indigo_define_property(device, X_SEND_WEATHER_MOUNT_PROPERTY, NULL);
				indigo_define_property(device, X_REBOOT_PROPERTY, NULL);
				device->is_connected = true;
				CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
			} else {
				CONNECTION_PROPERTY->state = INDIGO_ALERT_STATE;
				device->is_connected = false;
				indigo_set_switch(CONNECTION_PROPERTY, CONNECTION_DISCONNECTED_ITEM, false);
			}
		}
	} else {
		if (device->is_connected) {
			indigo_delete_property(device, AUX_GPIO_OUTLET_PROPERTY, NULL);
			indigo_delete_property(device, AUX_OUTLET_PULSE_LENGTHS_PROPERTY, NULL);
			indigo_delete_property(device, AUX_WEATHER_PROPERTY, NULL);
			indigo_delete_property(device, AUX_DEW_WARNING_PROPERTY, NULL);
			indigo_delete_property(device, X_CALIBRATION_PROPERTY, NULL);
			indigo_delete_property(device, X_SEND_WEATHER_MOUNT_PROPERTY, NULL);
			indigo_delete_property(device, X_REBOOT_PROPERTY, NULL);
			mgbox_close(device);
			device->is_connected = false;
			CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
		}
	}
	indigo_aux_change_property(device, NULL, CONNECTION_PROPERTY);
}


static indigo_result aux_change_property(indigo_device *device, indigo_client *client, indigo_property *property) {
	assert(device != NULL);
	assert(DEVICE_CONTEXT != NULL);
	assert(property != NULL);
	if (indigo_property_match(CONNECTION_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- CONNECTION
		if (indigo_ignore_connection_change(device, property))
			return INDIGO_OK;
		indigo_property_copy_values(CONNECTION_PROPERTY, property, false);
		CONNECTION_PROPERTY->state = INDIGO_BUSY_STATE;
		indigo_update_property(device, CONNECTION_PROPERTY, NULL);
		indigo_set_timer(device, 0, handle_aux_connect_property, NULL);
		return INDIGO_OK;
	} else if (indigo_property_match(AUX_OUTLET_NAMES_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- X_AUX_OUTLET_NAMES
		indigo_property_copy_values(AUX_OUTLET_NAMES_PROPERTY, property, false);
		if (IS_CONNECTED) {
			indigo_delete_property(device, AUX_GPIO_OUTLET_PROPERTY, NULL);
			indigo_delete_property(device, AUX_OUTLET_PULSE_LENGTHS_PROPERTY, NULL);
		}
		snprintf(AUX_GPIO_OUTLET_1_ITEM->label, INDIGO_NAME_SIZE, "%s", AUX_OUTLET_NAME_1_ITEM->text.value);
		snprintf(AUX_OUTLET_PULSE_LENGTHS_1_ITEM->label, INDIGO_NAME_SIZE, "%s", AUX_OUTLET_NAME_1_ITEM->text.value);
		if (IS_CONNECTED) {
			indigo_define_property(device, AUX_GPIO_OUTLET_PROPERTY, NULL);
			indigo_define_property(device, AUX_OUTLET_PULSE_LENGTHS_PROPERTY, NULL);
		}
		AUX_OUTLET_NAMES_PROPERTY->state = INDIGO_OK_STATE;
		indigo_update_property(device, AUX_OUTLET_NAMES_PROPERTY, NULL);
		return INDIGO_OK;
	} else if (indigo_property_match(AUX_OUTLET_PULSE_LENGTHS_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- AUX_OUTLET_PULSE_LENGTHS
		indigo_property_copy_values(AUX_OUTLET_PULSE_LENGTHS_PROPERTY, property, false);
		if (!IS_CONNECTED) return INDIGO_OK;
		indigo_update_property(device, AUX_OUTLET_PULSE_LENGTHS_PROPERTY, NULL);
		return INDIGO_OK;
	} else if (indigo_property_match(AUX_GPIO_OUTLET_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- AUX_GPIO_OUTLET
		indigo_property_copy_values(AUX_GPIO_OUTLET_PROPERTY, property, false);
		if (!IS_CONNECTED) return INDIGO_OK;
		if (!strchr(PRIVATE_DATA->device_type, 'P')) {
			AUX_GPIO_OUTLET_1_ITEM->sw.value = false;
			AUX_GPIO_OUTLET_PROPERTY->state = INDIGO_ALERT_STATE;
			char message[INDIGO_VALUE_SIZE];
			snprintf(message, INDIGO_VALUE_SIZE, "Model '%s' does not have a pulse switch", PRIVATE_DATA->device_type);
			indigo_update_property(device, AUX_GPIO_OUTLET_PROPERTY, message);
			INDIGO_DRIVER_ERROR(DRIVER_NAME, "%s", message);
			return INDIGO_OK;
		}
		AUX_GPIO_OUTLET_PROPERTY->state = INDIGO_OK_STATE;
		indigo_update_property(device, AUX_GPIO_OUTLET_PROPERTY, NULL);
		indigo_set_timer(device, 0, mg_pulse, NULL);
		return INDIGO_OK;
	} else if (indigo_property_match(X_CALIBRATION_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- X_CALIBRATION
		indigo_property_copy_values(X_CALIBRATION_PROPERTY, property, false);
		if (!device->is_connected) return INDIGO_OK;
		X_CALIBRATION_PROPERTY->state = INDIGO_BUSY_STATE;
		indigo_update_property(device, X_CALIBRATION_PROPERTY, NULL);
		indigo_set_timer(device, 0, mg_set_callibration, NULL);
		return INDIGO_OK;
	} else if (indigo_property_match(X_SEND_WEATHER_MOUNT_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- X_SEND_WEATHER_DATA_TO_MOUNT
		indigo_property_copy_values(X_SEND_WEATHER_MOUNT_PROPERTY, property, false);
		if (!device->is_connected) return INDIGO_OK;
		char command[INDIGO_VALUE_SIZE];
		X_SEND_WEATHER_MOUNT_PROPERTY->state = INDIGO_BUSY_STATE;
		indigo_update_property(device, X_SEND_WEATHER_MOUNT_PROPERTY, NULL);
		if (X_SEND_WEATHER_MOUNT_ITEM->sw.value) {
			strcpy(command,":mm,1*");
		} else {
			strcpy(command,":mm,0*");
		}
		pthread_mutex_lock(&PRIVATE_DATA->serial_mutex);
		mg_send_command(PRIVATE_DATA->handle, command);
		pthread_mutex_unlock(&PRIVATE_DATA->serial_mutex);
		return INDIGO_OK;
	} else if (indigo_property_match(X_REBOOT_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- X_REBOOT_DEVICE
		indigo_property_copy_values(X_REBOOT_PROPERTY, property, false);
		if (!device->is_connected) return INDIGO_OK;
		X_REBOOT_PROPERTY->state = INDIGO_BUSY_STATE;
		indigo_update_property(device, X_REBOOT_PROPERTY, NULL);
		indigo_set_timer(device, 0, mg_reset_device, NULL);
		return INDIGO_OK;
	} else if (indigo_property_match(AUX_DEW_THRESHOLD_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- AUX_DEW_THRESHOLD
		indigo_property_copy_values(AUX_DEW_THRESHOLD_PROPERTY, property, false);
		AUX_DEW_THRESHOLD_PROPERTY->state = INDIGO_OK_STATE;
		indigo_update_property(device, AUX_DEW_THRESHOLD_PROPERTY, NULL);
		return INDIGO_OK;
	} else if (indigo_property_match(CONFIG_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- CONFIG
		if (indigo_switch_match(CONFIG_SAVE_ITEM, property)) {
			indigo_save_property(device, NULL, AUX_OUTLET_NAMES_PROPERTY);
			indigo_save_property(device, NULL, AUX_DEW_THRESHOLD_PROPERTY);
		}
	}
	// --------------------------------------------------------------------------------
	return indigo_aux_change_property(device, client, property);
}


static indigo_result aux_detach(indigo_device *device) {
	assert(device != NULL);
	if (IS_CONNECTED) {
		indigo_set_switch(CONNECTION_PROPERTY, CONNECTION_DISCONNECTED_ITEM, true);
		handle_aux_connect_property(device);
	}
	indigo_release_property(AUX_GPIO_OUTLET_PROPERTY);
	indigo_release_property(AUX_OUTLET_PULSE_LENGTHS_PROPERTY);
	indigo_release_property(AUX_WEATHER_PROPERTY);
	indigo_release_property(AUX_DEW_WARNING_PROPERTY);
	indigo_release_property(X_CALIBRATION_PROPERTY);
	indigo_release_property(X_SEND_WEATHER_MOUNT_PROPERTY);
	indigo_release_property(X_REBOOT_PROPERTY);

	INDIGO_DEVICE_DETACH_LOG(DRIVER_NAME, device->name);

	indigo_delete_property(device, AUX_OUTLET_NAMES_PROPERTY, NULL);
	indigo_release_property(AUX_OUTLET_NAMES_PROPERTY);

	indigo_delete_property(device, AUX_DEW_THRESHOLD_PROPERTY, NULL);
	indigo_release_property(AUX_DEW_THRESHOLD_PROPERTY);

	return indigo_aux_detach(device);
}


// --------------------------------------------------------------------------------
indigo_result indigo_aux_mgbox(indigo_driver_action action, indigo_driver_info *info) {
	static indigo_device gps_template = INDIGO_DEVICE_INITIALIZER(
		GPS_MGBOX_NAME,
		gps_attach,
		gps_enumerate_properties,
		gps_change_property,
		NULL,
		gps_detach
	);

	static indigo_device aux_template = INDIGO_DEVICE_INITIALIZER(
		WEATHER_MGBOX_NAME,
		aux_attach,
		aux_enumerate_properties,
		aux_change_property,
		NULL,
		aux_detach
	);

	static indigo_driver_action last_action = INDIGO_DRIVER_SHUTDOWN;

	SET_DRIVER_INFO(info, "Astromi.ch MGBox", __FUNCTION__, DRIVER_VERSION, false, last_action);

	if (action == last_action)
		return INDIGO_OK;

	switch (action) {
	case INDIGO_DRIVER_INIT:
		last_action = action;

		private_data = indigo_safe_malloc(sizeof(mg_private_data));
		aux_weather = indigo_safe_malloc_copy(sizeof(indigo_device), &aux_template);
		aux_weather->private_data = private_data;
		indigo_attach_device(aux_weather);

		gps = indigo_safe_malloc_copy(sizeof(indigo_device), &gps_template);
		gps->private_data = private_data;
		gps->master_device = aux_weather;
		indigo_attach_device(gps);

		break;

	case INDIGO_DRIVER_SHUTDOWN:
		VERIFY_NOT_CONNECTED(gps);
		VERIFY_NOT_CONNECTED(aux_weather);
		last_action = action;
		if (aux_weather != NULL) {
			indigo_detach_device(aux_weather);
			free(aux_weather);
			aux_weather = NULL;
		}
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
