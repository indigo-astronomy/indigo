// Copyright (c) 2017-2025 Rumen G. Bogdanovski
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

// This file generated from indigo_gps_nmea.driver

#pragma mark - Includes

#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <assert.h>
#include <pthread.h>
#include <indigo/indigo_driver_xml.h>
#include <indigo/indigo_gps_driver.h>
#include <indigo/indigo_uni_io.h>

#include "indigo_gps_nmea.h"

#pragma mark - Common definitions

#define DRIVER_VERSION       0x03000011
#define DRIVER_NAME          "indigo_gps_nmea"
#define DRIVER_LABEL         "Generic NMEA 0183 GPS"
#define GPS_DEVICE_NAME      "NMEA GPS"
#define PRIVATE_DATA         ((nmea_private_data *)device->private_data)

//+ define

#define MAX_NB_OF_SYSTEMS    26

//- define

#pragma mark - Property definitions

#define GPS_SELECTED_SYSTEM_PROPERTY      (PRIVATE_DATA->gps_selected_system_property)
#define AUTOMATIC_SYSTEM_ITEM             (GPS_SELECTED_SYSTEM_PROPERTY->items + 0)
#define MULTIPLE_SYSTEM_ITEM              (GPS_SELECTED_SYSTEM_PROPERTY->items + 1)
#define GPS_SYSTEM_ITEM                   (GPS_SELECTED_SYSTEM_PROPERTY->items + 2)
#define GALILEO_SYSTEM_ITEM               (GPS_SELECTED_SYSTEM_PROPERTY->items + 3)
#define GLONASS_SYSTEM_ITEM               (GPS_SELECTED_SYSTEM_PROPERTY->items + 4)
#define BEIDOU_SYSTEM_ITEM                (GPS_SELECTED_SYSTEM_PROPERTY->items + 5)
#define NAVIC_SYSTEM_ITEM                 (GPS_SELECTED_SYSTEM_PROPERTY->items + 6)
#define QZSS_SYSTEM_ITEM                  (GPS_SELECTED_SYSTEM_PROPERTY->items + 7)

#define GPS_SELECTED_SYSTEM_PROPERTY_NAME "X_GPS_SELECTED_SYSTEM"
#define AUTOMATIC_SYSTEM_ITEM_NAME        "AUTO"
#define MULTIPLE_SYSTEM_ITEM_NAME         "MULTIPLE"
#define GPS_SYSTEM_ITEM_NAME              "GPS"
#define GALILEO_SYSTEM_ITEM_NAME          "GALILEO"
#define GLONASS_SYSTEM_ITEM_NAME          "GLONASS"
#define BEIDOU_SYSTEM_ITEM_NAME           "BEIDOU"
#define NAVIC_SYSTEM_ITEM_NAME            "NAVIC"
#define QZSS_SYSTEM_ITEM_NAME             "QZSS"

#pragma mark - Private data definition

typedef struct {
	indigo_uni_handle *handle;
	indigo_property *gps_selected_system_property;
	//+ data
	int satellites_in_view[MAX_NB_OF_SYSTEMS];
	char selected_system;
	//- data
} nmea_private_data;

#pragma mark - Low level code

//+ code

static bool nmea_open(indigo_device *device) {
	char *name = DEVICE_PORT_ITEM->text.value;
	if (indigo_uni_is_url(name, "gps")) {
		PRIVATE_DATA->handle = indigo_uni_client_tcp_socket(name, 9999, INDIGO_LOG_DEBUG);
	} else {
		PRIVATE_DATA->handle = indigo_uni_open_serial_with_config(name, DEVICE_BAUDRATE_ITEM->text.value, INDIGO_LOG_DEBUG);
	}
	return PRIVATE_DATA->handle != NULL;
}

static void nmea_close(indigo_device *device) {
	indigo_uni_close(&PRIVATE_DATA->handle);
}

static char **nmea_parse(char *buffer) {
	INDIGO_DRIVER_DEBUG(DRIVER_NAME, "%s", buffer);
	if (strncmp("$G", buffer, 2)) {// Disregard "non positioning" sentences
		return NULL;
	}
	char *index = strchr(buffer, '*');
	if (index) {
		*index++ = 0;
		int c1 = (int)strtol(index, NULL, 16);
		int c2 = 0;
		index = buffer + 1;
		while (*index) {
			c2 ^= *index++;
		}
		if (c1 != c2) {
			return NULL;
		}
	}
	char **tokens = indigo_safe_malloc(32 * sizeof(char *));
	int token = 0;
	memset(tokens, 0, 32 * sizeof(char *));
	index = buffer + 3;
	while (index) {
		tokens[token++] = index;
		index = strchr(index, ',');
		if (index) {
			*index++ = 0;
		}
	}
	return tokens;
}

static void nmea_reset(indigo_device *device) {
	if (AUTOMATIC_SYSTEM_ITEM->sw.value) {
		PRIVATE_DATA->selected_system = 0;
	} else if (MULTIPLE_SYSTEM_ITEM->sw.value) {
		PRIVATE_DATA->selected_system = 'N';
	} else if (GPS_SYSTEM_ITEM->sw.value) {
		PRIVATE_DATA->selected_system = 'P';
	} else if (GALILEO_SYSTEM_ITEM->sw.value) {
		PRIVATE_DATA->selected_system = 'A';
	} else if (GLONASS_SYSTEM_ITEM->sw.value) {
		PRIVATE_DATA->selected_system = 'L';
	} else if (BEIDOU_SYSTEM_ITEM->sw.value) {
		PRIVATE_DATA->selected_system = 'B';
	} else if (NAVIC_SYSTEM_ITEM->sw.value) {
		PRIVATE_DATA->selected_system = 'I';
	} else if (QZSS_SYSTEM_ITEM->sw.value) {
		PRIVATE_DATA->selected_system = 'Q';
	}
	GPS_GEOGRAPHIC_COORDINATES_PROPERTY->state = INDIGO_BUSY_STATE;
	GPS_UTC_TIME_PROPERTY->state = INDIGO_BUSY_STATE;
	GPS_STATUS_NO_FIX_ITEM->light.value = INDIGO_IDLE_STATE;
	GPS_STATUS_2D_FIX_ITEM->light.value = INDIGO_IDLE_STATE;
	GPS_STATUS_3D_FIX_ITEM->light.value = INDIGO_IDLE_STATE;
	GPS_STATUS_PROPERTY->state = INDIGO_BUSY_STATE;
	GPS_ADVANCED_STATUS_SVS_IN_USE_ITEM->number.value = 0;
	GPS_ADVANCED_STATUS_SVS_IN_VIEW_ITEM->number.value = 0;
	GPS_ADVANCED_STATUS_PDOP_ITEM->number.value = 0.0;
	GPS_ADVANCED_STATUS_HDOP_ITEM->number.value = 0.0;
	GPS_ADVANCED_STATUS_VDOP_ITEM->number.value = 0.0;
	GPS_ADVANCED_STATUS_PROPERTY->state = INDIGO_BUSY_STATE;
	indigo_update_property(device, GPS_GEOGRAPHIC_COORDINATES_PROPERTY, NULL);
	indigo_update_property(device, GPS_STATUS_PROPERTY, NULL);
	indigo_update_property(device, GPS_UTC_TIME_PROPERTY, NULL);
	indigo_update_property(device, GPS_ADVANCED_STATUS_PROPERTY, NULL);
	memset(PRIVATE_DATA->satellites_in_view, 0, sizeof(int) * MAX_NB_OF_SYSTEMS);
}

static void gps_connection_handler(indigo_device *device);

//- code

#pragma mark - High level code (gps)

static void gps_timer_callback(indigo_device *device) {
	if (!IS_CONNECTED) {
		return;
	}
	//+ gps.on_timer
	char buffer[128];
	long length = indigo_uni_read_line(PRIVATE_DATA->handle, buffer, sizeof(buffer));
	char **tokens = nmea_parse(buffer);

	if (length > 0 && tokens) {
		char nmea_system = buffer[2];
		if (!strcmp(tokens[0], "RMC") && (PRIVATE_DATA->selected_system == 0 ||  PRIVATE_DATA->selected_system == nmea_system)) {
			// Recommended Minimum sentence C
			bool hasFix = (*tokens[2] == 'A');
			int time = atoi(tokens[1]);
			int date = atoi(tokens[9]);
			sprintf(GPS_UTC_ITEM->text.value, "20%02d-%02d-%02dT%02d:%02d:%02d", date % 100, (date / 100) % 100, date / 10000, time / 10000, (time / 100) % 100, time % 100);
			GPS_UTC_TIME_PROPERTY->state = hasFix ? INDIGO_OK_STATE : INDIGO_ALERT_STATE;
			indigo_update_property(device, GPS_UTC_TIME_PROPERTY, NULL);
			double lat = indigo_atod(tokens[3]);
			lat = floor(lat / 100) + fmod(lat, 100) / 60;
			if (!strcmp(tokens[4], "S")) {
				lat = -lat;
			}
			lat = round(lat * 10000) / 10000;
			double lon = indigo_atod(tokens[5]);
			lon = floor(lon / 100) + fmod(lon, 100) / 60;
			if (!strcmp(tokens[6], "W")) {
				lon = -lon;
			}
			lon = round(lon * 10000) / 10000;
			if (GPS_GEOGRAPHIC_COORDINATES_LONGITUDE_ITEM->number.value != lon || GPS_GEOGRAPHIC_COORDINATES_LATITUDE_ITEM->number.value != lat) {
				GPS_GEOGRAPHIC_COORDINATES_LONGITUDE_ITEM->number.value = lon;
				GPS_GEOGRAPHIC_COORDINATES_LATITUDE_ITEM->number.value = lat;
				GPS_GEOGRAPHIC_COORDINATES_PROPERTY->state = hasFix ? INDIGO_OK_STATE : INDIGO_ALERT_STATE;
				indigo_update_property(device, GPS_GEOGRAPHIC_COORDINATES_PROPERTY, NULL);
			}
			if (hasFix && PRIVATE_DATA->selected_system == 0) {
				PRIVATE_DATA->selected_system = nmea_system;
				memset(PRIVATE_DATA->satellites_in_view, 0, sizeof(int) * MAX_NB_OF_SYSTEMS);
			}
		} else if (!strcmp(tokens[0], "GGA") && (PRIVATE_DATA->selected_system == 0 ||  PRIVATE_DATA->selected_system == nmea_system)) {
			// Global Positioning System Fix Data
			bool hasFix = (*tokens[6] != 0 && *tokens[6] != '0');
			double lat = indigo_atod(tokens[2]);
			lat = floor(lat / 100) + fmod(lat, 100) / 60;
			if (!strcmp(tokens[3], "S")) {
				lat = -lat;
			}
			lat = round(lat * 10000) / 10000;
			double lon = indigo_atod(tokens[4]);
			lon = floor(lon / 100) + fmod(lon, 100) / 60;
			if (!strcmp(tokens[5], "W")) {
				lon = -lon;
			}
			lon = round(lon * 10000) / 10000;
			if (GPS_GEOGRAPHIC_COORDINATES_LONGITUDE_ITEM->number.value != lon || GPS_GEOGRAPHIC_COORDINATES_LATITUDE_ITEM->number.value != lat) {
				GPS_GEOGRAPHIC_COORDINATES_LONGITUDE_ITEM->number.value = lon;
				GPS_GEOGRAPHIC_COORDINATES_LATITUDE_ITEM->number.value = lat;
			}
			double elv = round(indigo_atod(tokens[9]));
			if (GPS_GEOGRAPHIC_COORDINATES_ELEVATION_ITEM->number.value != elv) {
				GPS_GEOGRAPHIC_COORDINATES_ELEVATION_ITEM->number.value = elv;
			}
			GPS_GEOGRAPHIC_COORDINATES_PROPERTY->state = hasFix ? INDIGO_OK_STATE : INDIGO_ALERT_STATE;
			indigo_update_property(device, GPS_GEOGRAPHIC_COORDINATES_PROPERTY, NULL);
			int in_use = atoi(tokens[7]);
			if (GPS_ADVANCED_STATUS_SVS_IN_USE_ITEM->number.value != in_use) {
				GPS_ADVANCED_STATUS_SVS_IN_USE_ITEM->number.value = in_use;
				GPS_ADVANCED_STATUS_PROPERTY->state = hasFix ? INDIGO_OK_STATE : INDIGO_ALERT_STATE;
				if (GPS_ADVANCED_ENABLED_ITEM->sw.value) {
					indigo_update_property(device, GPS_ADVANCED_STATUS_PROPERTY, NULL);
				}
			}
			if (hasFix && PRIVATE_DATA->selected_system == 0) {
				PRIVATE_DATA->selected_system = nmea_system;
				memset(PRIVATE_DATA->satellites_in_view, 0, sizeof(int) * MAX_NB_OF_SYSTEMS);
			}
		} else if (!strcmp(tokens[0], "GSV") && (PRIVATE_DATA->selected_system == 0 || PRIVATE_DATA->selected_system == 'N' || PRIVATE_DATA->selected_system == nmea_system)) {
			// Satellites in view
			int in_view = atoi(tokens[3]), total_in_view = 0;
			if ((PRIVATE_DATA->selected_system == 0 || PRIVATE_DATA->selected_system == 'N') && nmea_system != 'N') {
				// When the nmea system is not yet selected, or if using a multi-constellation fix
				// count satellites from all constellations
				// As an exception, if the GSV is sent with the GNSS talker id 'N', it is assumed to comprise all SVs
				if (nmea_system >= 'A' && nmea_system <= 'Z') {
					PRIVATE_DATA->satellites_in_view[nmea_system - 'A'] = in_view;
					for (int i = 0; i < MAX_NB_OF_SYSTEMS; i++)
						total_in_view += PRIVATE_DATA->satellites_in_view[i];
				}
			} else {
				// Otherwise, count only satellites from the selected system
				total_in_view = in_view;
			}
			if (GPS_ADVANCED_STATUS_SVS_IN_VIEW_ITEM->number.value != total_in_view) {
				GPS_ADVANCED_STATUS_SVS_IN_VIEW_ITEM->number.value = total_in_view;
				GPS_ADVANCED_STATUS_PROPERTY->state = INDIGO_OK_STATE;
				if (GPS_ADVANCED_ENABLED_ITEM->sw.value) {
					indigo_update_property(device, GPS_ADVANCED_STATUS_PROPERTY, NULL);
				}
			}
		} else if (!strcmp(tokens[0], "GSA") && (PRIVATE_DATA->selected_system == 0 || PRIVATE_DATA->selected_system == 'N' || PRIVATE_DATA->selected_system == nmea_system)) {
			// Satellite status
			// When providing a multiple GNSS fix, the receiver may provide a GSA for each constellation, and none with 'N' id
			// As the fix is a combined fix, the values should be the same in each GSA, as observed with the receiver I have in hand.
			// We therefore process equally all GSA in this specific case
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
			double pdop = indigo_atod(tokens[15]);
			double hdop = indigo_atod(tokens[16]);
			double vdop = indigo_atod(tokens[17]);
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
		indigo_safe_free(tokens);
		indigo_execute_handler_in(device, 0, gps_timer_callback);
	} else {
		indigo_send_message(device, "Failed to read from GPS unit");
		indigo_set_switch(CONNECTION_PROPERTY, CONNECTION_DISCONNECTED_ITEM, true);
		indigo_set_timer(device, 0, gps_connection_handler, NULL);
	}
	//- gps.on_timer
}

static void gps_connection_handler(indigo_device *device) {
	if (CONNECTION_CONNECTED_ITEM->sw.value) {
		bool connection_result = true;
		connection_result = nmea_open(device);
		if (connection_result) {
			//+ gps.on_connect
			GPS_GEOGRAPHIC_COORDINATES_LONGITUDE_ITEM->number.value = 0;
			GPS_GEOGRAPHIC_COORDINATES_LATITUDE_ITEM->number.value = 0;
			GPS_GEOGRAPHIC_COORDINATES_ELEVATION_ITEM->number.value = 0;
			sprintf(GPS_UTC_ITEM->text.value, "0000-00-00T00:00:00.00");
			nmea_reset(device);
			//- gps.on_connect
			indigo_execute_handler(device, gps_timer_callback);
			CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
			indigo_send_message(device, "Connected to %s on %s", GPS_DEVICE_NAME, DEVICE_PORT_ITEM->text.value);
		} else {
			indigo_send_message(device, "Failed to connect to %s on %s", GPS_DEVICE_NAME, DEVICE_PORT_ITEM->text.value);
			CONNECTION_PROPERTY->state = INDIGO_ALERT_STATE;
			indigo_set_switch(CONNECTION_PROPERTY, CONNECTION_DISCONNECTED_ITEM, true);
		}
	} else {
		nmea_close(device);
		indigo_send_message(device, "Disconnected from %s", device->name);
		CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
	}
	indigo_gps_change_property(device, NULL, CONNECTION_PROPERTY);
}

static void gps_selected_system_handler(indigo_device *device) {
	GPS_SELECTED_SYSTEM_PROPERTY->state = INDIGO_OK_STATE;
	//+ gps.GPS_SELECTED_SYSTEM.on_change
	nmea_reset(device);
	//- gps.GPS_SELECTED_SYSTEM.on_change
	indigo_update_property(device, GPS_SELECTED_SYSTEM_PROPERTY, NULL);
}

#pragma mark - Device API (gps)

static indigo_result gps_enumerate_properties(indigo_device *device, indigo_client *client, indigo_property *property);

static indigo_result gps_attach(indigo_device *device) {
	if (indigo_gps_attach(device, DRIVER_NAME, DRIVER_VERSION) == INDIGO_OK) {
		ADDITIONAL_INSTANCES_PROPERTY->hidden = DEVICE_CONTEXT->base_device != NULL;
		DEVICE_PORT_PROPERTY->hidden = false;
		DEVICE_PORTS_PROPERTY->hidden = false;
		indigo_enumerate_serial_ports(device, DEVICE_PORTS_PROPERTY);
		//+ gps.on_attach
		GPS_ADVANCED_PROPERTY->hidden = false;
		GPS_GEOGRAPHIC_COORDINATES_PROPERTY->hidden = false;
		GPS_GEOGRAPHIC_COORDINATES_PROPERTY->count = 3;
		GPS_UTC_TIME_PROPERTY->hidden = false;
		GPS_UTC_TIME_PROPERTY->count = 1;
		//- gps.on_attach
		GPS_SELECTED_SYSTEM_PROPERTY = indigo_init_switch_property(NULL, device->name, GPS_SELECTED_SYSTEM_PROPERTY_NAME, MAIN_GROUP, "Selected positioning system", INDIGO_OK_STATE, INDIGO_RW_PERM, INDIGO_ONE_OF_MANY_RULE, 8);
		if (GPS_SELECTED_SYSTEM_PROPERTY == NULL) {
			return INDIGO_FAILED;
		}
		indigo_init_switch_item(AUTOMATIC_SYSTEM_ITEM, AUTOMATIC_SYSTEM_ITEM_NAME, "Autodetect", true);
		indigo_init_switch_item(MULTIPLE_SYSTEM_ITEM, MULTIPLE_SYSTEM_ITEM_NAME, "Multiple", false);
		indigo_init_switch_item(GPS_SYSTEM_ITEM, GPS_SYSTEM_ITEM_NAME, "GPS", false);
		indigo_init_switch_item(GALILEO_SYSTEM_ITEM, GALILEO_SYSTEM_ITEM_NAME, "Galileo", false);
		indigo_init_switch_item(GLONASS_SYSTEM_ITEM, GLONASS_SYSTEM_ITEM_NAME, "GLONASS", false);
		indigo_init_switch_item(BEIDOU_SYSTEM_ITEM, BEIDOU_SYSTEM_ITEM_NAME, "BeiDou", false);
		indigo_init_switch_item(NAVIC_SYSTEM_ITEM, NAVIC_SYSTEM_ITEM_NAME, "NavIC", false);
		indigo_init_switch_item(QZSS_SYSTEM_ITEM, QZSS_SYSTEM_ITEM_NAME, "QZSS1", false);
		INDIGO_DEVICE_ATTACH_LOG(DRIVER_NAME, device->name);
		return gps_enumerate_properties(device, NULL, NULL);
	}
	return INDIGO_FAILED;
}

static indigo_result gps_enumerate_properties(indigo_device *device, indigo_client *client, indigo_property *property) {
	INDIGO_DEFINE_MATCHING_PROPERTY(GPS_SELECTED_SYSTEM_PROPERTY);
	return indigo_gps_enumerate_properties(device, NULL, NULL);
}

static indigo_result gps_change_property(indigo_device *device, indigo_client *client, indigo_property *property) {
	if (indigo_property_match_changeable(CONNECTION_PROPERTY, property)) {
		if (!indigo_ignore_connection_change(device, property)) {
			indigo_property_copy_values(CONNECTION_PROPERTY, property, false);
			CONNECTION_PROPERTY->state = INDIGO_BUSY_STATE;
			indigo_update_property(device, CONNECTION_PROPERTY, NULL);
			indigo_execute_handler(device, gps_connection_handler);
		}
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(GPS_SELECTED_SYSTEM_PROPERTY, property)) {
		INDIGO_COPY_VALUES_PROCESS_CHANGE(GPS_SELECTED_SYSTEM_PROPERTY, gps_selected_system_handler);
		return INDIGO_OK;
	}
	return indigo_gps_change_property(device, client, property);
}

static indigo_result gps_detach(indigo_device *device) {
	if (IS_CONNECTED) {
		indigo_set_switch(CONNECTION_PROPERTY, CONNECTION_DISCONNECTED_ITEM, true);
		gps_connection_handler(device);
	}
	indigo_release_property(GPS_SELECTED_SYSTEM_PROPERTY);
	INDIGO_DEVICE_DETACH_LOG(DRIVER_NAME, device->name);
	return indigo_gps_detach(device);
}

#pragma mark - Device templates

static indigo_device gps_template = INDIGO_DEVICE_INITIALIZER(GPS_DEVICE_NAME, gps_attach, gps_enumerate_properties, gps_change_property, NULL, gps_detach);

#pragma mark - Main code

indigo_result indigo_gps_nmea(indigo_driver_action action, indigo_driver_info *info) {
	static indigo_driver_action last_action = INDIGO_DRIVER_SHUTDOWN;
	static nmea_private_data *private_data = NULL;
	static indigo_device *gps = NULL;

	SET_DRIVER_INFO(info, DRIVER_LABEL, __FUNCTION__, DRIVER_VERSION, false, last_action);

	if (action == last_action) {
		return INDIGO_OK;
	}

	switch (action) {
		case INDIGO_DRIVER_INIT:
			last_action = action;
			static indigo_device_match_pattern patterns[2] = { 0 };
			strcpy(patterns[0].product_string, "GPS");
			strcpy(patterns[1].product_string, "GNSS");
			INDIGO_REGISER_MATCH_PATTERNS(gps_template, patterns, 2);
			private_data = indigo_safe_malloc(sizeof(nmea_private_data));
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
