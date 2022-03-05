// Copyright (c) 2018 CloudMakers, s. r. o.
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
// 2.0 by Peter Polakovic <peter.polakovic@cloudmakers.eu>

/** INDIGO Mount control agent
 \file indigo_agent_mount.c
 */

#define DRIVER_VERSION 0x000A
#define DRIVER_NAME	"indigo_agent_mount"

#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <math.h>
#include <assert.h>
#include <pthread.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <netinet/in.h>

#include <indigo/indigo_driver_xml.h>
#include <indigo/indigo_filter.h>
#include <indigo/indigo_io.h>
#include <indigo/indigo_mount_driver.h>

#include "indigo_agent_mount.h"

#define DEVICE_PRIVATE_DATA														((agent_private_data *)device->private_data)
#define CLIENT_PRIVATE_DATA														((agent_private_data *)FILTER_CLIENT_CONTEXT->device->private_data)

#define AGENT_GEOGRAPHIC_COORDINATES_PROPERTY					(DEVICE_PRIVATE_DATA->agent_geographic_property)
#define AGENT_GEOGRAPHIC_COORDINATES_LATITUDE_ITEM  	(AGENT_GEOGRAPHIC_COORDINATES_PROPERTY->items+0)
#define AGENT_GEOGRAPHIC_COORDINATES_LONGITUDE_ITEM  	(AGENT_GEOGRAPHIC_COORDINATES_PROPERTY->items+1)
#define AGENT_GEOGRAPHIC_COORDINATES_ELEVATION_ITEM  	(AGENT_GEOGRAPHIC_COORDINATES_PROPERTY->items+2)

#define AGENT_SITE_DATA_SOURCE_PROPERTY								(DEVICE_PRIVATE_DATA->agent_site_data_source_property)
#define AGENT_SITE_DATA_SOURCE_HOST_ITEM  						(AGENT_SITE_DATA_SOURCE_PROPERTY->items+0)
#define AGENT_SITE_DATA_SOURCE_MOUNT_ITEM  						(AGENT_SITE_DATA_SOURCE_PROPERTY->items+1)
#define AGENT_SITE_DATA_SOURCE_DOME_ITEM  						(AGENT_SITE_DATA_SOURCE_PROPERTY->items+2)
#define AGENT_SITE_DATA_SOURCE_GPS_ITEM  							(AGENT_SITE_DATA_SOURCE_PROPERTY->items+3)

#define AGENT_SET_HOST_TIME_PROPERTY									(DEVICE_PRIVATE_DATA->agent_set_host_time_property)
#define AGENT_SET_HOST_TIME_MOUNT_ITEM  							(AGENT_SET_HOST_TIME_PROPERTY->items+0)
#define AGENT_SET_HOST_TIME_DOME_ITEM  								(AGENT_SET_HOST_TIME_PROPERTY->items+1)

#define AGENT_LX200_SERVER_PROPERTY										(DEVICE_PRIVATE_DATA->agent_lx200_server_property)
#define AGENT_LX200_SERVER_STOPPED_ITEM								(AGENT_LX200_SERVER_PROPERTY->items+0)
#define AGENT_LX200_SERVER_STARTED_ITEM								(AGENT_LX200_SERVER_PROPERTY->items+1)

#define AGENT_LX200_CONFIGURATION_PROPERTY						(DEVICE_PRIVATE_DATA->agent_lx200_configuration_property)
#define AGENT_LX200_CONFIGURATION_PORT_ITEM						(AGENT_LX200_CONFIGURATION_PROPERTY->items+0)

#define AGENT_LIMITS_PROPERTY													(DEVICE_PRIVATE_DATA->agent_limits_property)
#define AGENT_HA_TRACKING_LIMIT_ITEM									(AGENT_LIMITS_PROPERTY->items+0)
#define AGENT_LOCAL_TIME_LIMIT_ITEM										(AGENT_LIMITS_PROPERTY->items+1)
#define AGENT_COORDINATES_PROPAGATE_THESHOLD_ITEM			(AGENT_LIMITS_PROPERTY->items+2)

#define AGENT_MOUNT_FOV_PROPERTY											(DEVICE_PRIVATE_DATA->agent_fov_property)
#define AGENT_MOUNT_FOV_ANGLE_ITEM										(AGENT_MOUNT_FOV_PROPERTY->items+0)
#define AGENT_MOUNT_FOV_WIDTH_ITEM										(AGENT_MOUNT_FOV_PROPERTY->items+1)
#define AGENT_MOUNT_FOV_HEIGHT_ITEM										(AGENT_MOUNT_FOV_PROPERTY->items+2)


typedef struct {
	indigo_property *agent_geographic_property;
	indigo_property *agent_site_data_source_property;
	indigo_property *agent_set_host_time_property;
	indigo_property *agent_lx200_server_property;
	indigo_property *agent_lx200_configuration_property;
	indigo_property *agent_limits_property;
	indigo_property *agent_fov_property;
	double mount_latitude, mount_longitude, mount_elevation;
	double dome_latitude, dome_longitude, dome_elevation;
	double gps_latitude, gps_longitude, gps_elevation;
	double mount_ra, mount_dec;
	double mount_target_ra, mount_target_dec;
	int server_socket;
	bool dome_unparked;
	pthread_mutex_t mutex;
} agent_private_data;

static void save_config(indigo_device *device) {
	if (pthread_mutex_trylock(&DEVICE_CONTEXT->config_mutex) == 0) {
		pthread_mutex_unlock(&DEVICE_CONTEXT->config_mutex);
		pthread_mutex_lock(&DEVICE_PRIVATE_DATA->mutex);
		indigo_save_property(device, NULL, AGENT_GEOGRAPHIC_COORDINATES_PROPERTY);
		indigo_save_property(device, NULL, AGENT_SITE_DATA_SOURCE_PROPERTY);
		indigo_save_property(device, NULL, AGENT_LX200_CONFIGURATION_PROPERTY);
		indigo_save_property(device, NULL, AGENT_MOUNT_FOV_PROPERTY);
		indigo_save_property(device, NULL, AGENT_SET_HOST_TIME_PROPERTY);
		indigo_save_property(device, NULL, ADDITIONAL_INSTANCES_PROPERTY);
		double tmp_ha_tracking_limit = AGENT_HA_TRACKING_LIMIT_ITEM->number.value;
		AGENT_HA_TRACKING_LIMIT_ITEM->number.value = AGENT_HA_TRACKING_LIMIT_ITEM->number.target;
		double tmp_local_time_limit = AGENT_LOCAL_TIME_LIMIT_ITEM->number.value;
		AGENT_LOCAL_TIME_LIMIT_ITEM->number.value = AGENT_LOCAL_TIME_LIMIT_ITEM->number.target;
		indigo_save_property(device, NULL, AGENT_LIMITS_PROPERTY);
		AGENT_HA_TRACKING_LIMIT_ITEM->number.value = tmp_ha_tracking_limit;
		 AGENT_LOCAL_TIME_LIMIT_ITEM->number.value = tmp_local_time_limit;
		if (DEVICE_CONTEXT->property_save_file_handle) {
			CONFIG_PROPERTY->state = INDIGO_OK_STATE;
			close(DEVICE_CONTEXT->property_save_file_handle);
			DEVICE_CONTEXT->property_save_file_handle = 0;
		} else {
			CONFIG_PROPERTY->state = INDIGO_ALERT_STATE;
		}
		CONFIG_SAVE_ITEM->sw.value = false;
		indigo_update_property(device, CONFIG_PROPERTY, NULL);
		pthread_mutex_unlock(&DEVICE_PRIVATE_DATA->mutex);
	}
}

static void set_site_coordinates2(indigo_device *device, int index, double latitude, double longitude, double elevation) {
	if (*FILTER_DEVICE_CONTEXT->device_name[index]) {
		static const char *names[] = { GEOGRAPHIC_COORDINATES_LATITUDE_ITEM_NAME, GEOGRAPHIC_COORDINATES_LONGITUDE_ITEM_NAME, GEOGRAPHIC_COORDINATES_ELEVATION_ITEM_NAME };
		double values[] = { latitude, longitude, elevation };
		indigo_change_number_property(FILTER_DEVICE_CONTEXT->client, FILTER_DEVICE_CONTEXT->device_name[index], GEOGRAPHIC_COORDINATES_PROPERTY_NAME, 3, names, values);
	}
}

static void set_site_coordinates3(indigo_device *device) {
	indigo_property *list = FILTER_DEVICE_CONTEXT->filter_related_agent_list_property;
	for (int i = 0; i < list->count; i++) {
		indigo_item *item = list->items + i;
		if (item->sw.value && !strncmp("Imager Agent", item->name, 12)) {
			indigo_property *property = indigo_init_text_property(NULL, item->name, CCD_FITS_HEADERS_PROPERTY_NAME, NULL, NULL, INDIGO_OK_STATE, INDIGO_RW_PERM, 2);
			indigo_init_text_item(property->items + 0, "HEADER_5", NULL, "SITELAT='%d %02d %02d'", (int)(AGENT_GEOGRAPHIC_COORDINATES_LATITUDE_ITEM->number.value), ((int)(fabs(AGENT_GEOGRAPHIC_COORDINATES_LATITUDE_ITEM->number.value) * 60)) % 60, ((int)(fabs(AGENT_GEOGRAPHIC_COORDINATES_LATITUDE_ITEM->number.value) * 3600)) % 60);
			indigo_init_text_item(property->items + 1, "HEADER_6", NULL,  "SITELONG='%d %02d %02d'", (int)(AGENT_GEOGRAPHIC_COORDINATES_LONGITUDE_ITEM->number.value), ((int)(fabs(AGENT_GEOGRAPHIC_COORDINATES_LONGITUDE_ITEM->number.value) * 60)) % 60, ((int)(fabs(AGENT_GEOGRAPHIC_COORDINATES_LONGITUDE_ITEM->number.value) * 3600)) % 60);
			property->access_token = indigo_get_device_or_master_token(property->device);
			indigo_change_property(FILTER_DEVICE_CONTEXT->client, property);
			indigo_release_property(property);
		}
	}
}

static void set_eq_coordinates(indigo_device *device) {
	indigo_property *list = FILTER_DEVICE_CONTEXT->filter_related_agent_list_property;
	for (int i = 0; i < list->count; i++) {
		indigo_item *item = list->items + i;
		if (item->sw.value && !strncmp("Imager Agent", item->name, 12)) {
			indigo_property *property = indigo_init_text_property(NULL, item->name, CCD_FITS_HEADERS_PROPERTY_NAME, NULL, NULL, INDIGO_OK_STATE, INDIGO_RW_PERM, 2);
			indigo_init_text_item(property->items + 0, "HEADER_7", NULL, "OBJCTRA='%d %02d %02d'", (int)(DEVICE_PRIVATE_DATA->mount_ra), ((int)(fabs(DEVICE_PRIVATE_DATA->mount_ra) * 60)) % 60, ((int)(fabs(DEVICE_PRIVATE_DATA->mount_ra) * 3600)) % 60);
			indigo_init_text_item(property->items + 1, "HEADER_8", NULL,  "OBJCTDEC='%d %02d %02d'", (int)(DEVICE_PRIVATE_DATA->mount_dec), ((int)(fabs(DEVICE_PRIVATE_DATA->mount_dec) * 60)) % 60, ((int)(fabs(DEVICE_PRIVATE_DATA->mount_dec) * 3600)) % 60);
			property->access_token = indigo_get_device_or_master_token(property->device);
			indigo_change_property(FILTER_DEVICE_CONTEXT->client, property);
			indigo_release_property(property);
		}
	}
}

static void abort_capture(indigo_device *device) {
	indigo_property *list = FILTER_DEVICE_CONTEXT->filter_related_agent_list_property;
	for (int i = 0; i < list->count; i++) {
		indigo_item *item = list->items + i;
		if (item->sw.value && !strncmp("Imager Agent", item->name, 12)) {
			indigo_change_switch_property_1(FILTER_DEVICE_CONTEXT->client, item->name, AGENT_ABORT_PROCESS_PROPERTY_NAME, AGENT_ABORT_PROCESS_ITEM_NAME, true);
		}
	}
}

static void abort_guiding(indigo_device *device) {
	indigo_property *list = FILTER_DEVICE_CONTEXT->filter_related_agent_list_property;
	for (int i = 0; i < list->count; i++) {
		indigo_item *item = list->items + i;
		if (item->sw.value && !strncmp("Guider Agent", item->name, 12)) {
			indigo_change_switch_property_1(FILTER_DEVICE_CONTEXT->client, item->name, AGENT_ABORT_PROCESS_PROPERTY_NAME, AGENT_ABORT_PROCESS_ITEM_NAME, true);
		}
	}
}

static void set_site_coordinates(indigo_device *device) {
	double latitude = 0, longitude = 0, elevation = 0;
	if (AGENT_SITE_DATA_SOURCE_HOST_ITEM->sw.value) {
		latitude = AGENT_GEOGRAPHIC_COORDINATES_LATITUDE_ITEM->number.target;
		longitude = AGENT_GEOGRAPHIC_COORDINATES_LONGITUDE_ITEM->number.target;
		elevation = AGENT_GEOGRAPHIC_COORDINATES_ELEVATION_ITEM->number.target;
		set_site_coordinates2(device, INDIGO_FILTER_MOUNT_INDEX, latitude, longitude, elevation);
		set_site_coordinates2(device, INDIGO_FILTER_DOME_INDEX, latitude, longitude, elevation);
	} else if (AGENT_SITE_DATA_SOURCE_MOUNT_ITEM->sw.value) {
		latitude = DEVICE_PRIVATE_DATA->mount_latitude;
		longitude = DEVICE_PRIVATE_DATA->mount_longitude;
		elevation = DEVICE_PRIVATE_DATA->mount_elevation;
		set_site_coordinates2(device, INDIGO_FILTER_DOME_INDEX, latitude, longitude, elevation);
	} else if (AGENT_SITE_DATA_SOURCE_DOME_ITEM->sw.value) {
		latitude = DEVICE_PRIVATE_DATA->dome_latitude;
		longitude = DEVICE_PRIVATE_DATA->dome_longitude;
		elevation = DEVICE_PRIVATE_DATA->dome_elevation;
		set_site_coordinates2(device, INDIGO_FILTER_MOUNT_INDEX, latitude, longitude, elevation);
	} else if (AGENT_SITE_DATA_SOURCE_GPS_ITEM->sw.value) {
		latitude = DEVICE_PRIVATE_DATA->gps_latitude;
		longitude = DEVICE_PRIVATE_DATA->gps_longitude;
		elevation = DEVICE_PRIVATE_DATA->gps_elevation;
		set_site_coordinates2(device, INDIGO_FILTER_MOUNT_INDEX, latitude, longitude, elevation);
		set_site_coordinates2(device, INDIGO_FILTER_DOME_INDEX, latitude, longitude, elevation);
	}
	AGENT_GEOGRAPHIC_COORDINATES_LATITUDE_ITEM->number.value = latitude;
	AGENT_GEOGRAPHIC_COORDINATES_LONGITUDE_ITEM->number.value = longitude;
	AGENT_GEOGRAPHIC_COORDINATES_ELEVATION_ITEM->number.value = elevation;
	AGENT_GEOGRAPHIC_COORDINATES_PROPERTY->state = INDIGO_OK_STATE;
	indigo_update_property(device, AGENT_GEOGRAPHIC_COORDINATES_PROPERTY, NULL);
	set_site_coordinates3(device);
	if (AGENT_SET_HOST_TIME_MOUNT_ITEM->sw.value)
		indigo_change_switch_property_1(FILTER_DEVICE_CONTEXT->client, FILTER_DEVICE_CONTEXT->device_name[INDIGO_FILTER_MOUNT_INDEX], MOUNT_SET_HOST_TIME_PROPERTY_NAME, MOUNT_SET_HOST_TIME_ITEM_NAME, true);
	if (AGENT_SET_HOST_TIME_DOME_ITEM->sw.value)
		indigo_change_switch_property_1(FILTER_DEVICE_CONTEXT->client, FILTER_DEVICE_CONTEXT->device_name[INDIGO_FILTER_DOME_INDEX], DOME_SET_HOST_TIME_PROPERTY_NAME, DOME_SET_HOST_TIME_ITEM_NAME, true);

}

// -------------------------------------------------------------------------------- INDIGO agent device implementation

static indigo_result agent_enumerate_properties(indigo_device *device, indigo_client *client, indigo_property *property);

static indigo_result agent_device_attach(indigo_device *device) {
	assert(device != NULL);
	assert(DEVICE_PRIVATE_DATA != NULL);
	if (indigo_filter_device_attach(device, DRIVER_NAME, DRIVER_VERSION, INDIGO_INTERFACE_MOUNT | INDIGO_INTERFACE_DOME | INDIGO_INTERFACE_GPS | INDIGO_INTERFACE_AUX_JOYSTICK) == INDIGO_OK) {
		// -------------------------------------------------------------------------------- Device properties
		FILTER_MOUNT_LIST_PROPERTY->hidden = false;
		FILTER_DOME_LIST_PROPERTY->hidden = false;
		FILTER_GPS_LIST_PROPERTY->hidden = false;
		FILTER_JOYSTICK_LIST_PROPERTY->hidden = false;
		FILTER_RELATED_AGENT_LIST_PROPERTY->hidden = false;
		// -------------------------------------------------------------------------------- GEOGRAPHIC_COORDINATES
		AGENT_GEOGRAPHIC_COORDINATES_PROPERTY = indigo_init_number_property(NULL, device->name, GEOGRAPHIC_COORDINATES_PROPERTY_NAME, "Agent", "Location", INDIGO_OK_STATE, INDIGO_RW_PERM, 3);
		if (AGENT_GEOGRAPHIC_COORDINATES_PROPERTY == NULL)
		return INDIGO_FAILED;
		indigo_init_sexagesimal_number_item(AGENT_GEOGRAPHIC_COORDINATES_LATITUDE_ITEM, GEOGRAPHIC_COORDINATES_LATITUDE_ITEM_NAME, "Latitude (-90 to +90° +N)", -90, 90, 0, 0);
		indigo_init_sexagesimal_number_item(AGENT_GEOGRAPHIC_COORDINATES_LONGITUDE_ITEM, GEOGRAPHIC_COORDINATES_LONGITUDE_ITEM_NAME, "Longitude (0 to 360° +E)", -180, 360, 0, 0);
		indigo_init_number_item(AGENT_GEOGRAPHIC_COORDINATES_ELEVATION_ITEM, GEOGRAPHIC_COORDINATES_ELEVATION_ITEM_NAME, "Elevation (m)", -400, 8000, 0, 0);
		// -------------------------------------------------------------------------------- AGENT_SITE_DATA_SOURCE
		AGENT_SITE_DATA_SOURCE_PROPERTY = indigo_init_switch_property(NULL, device->name, AGENT_SITE_DATA_SOURCE_PROPERTY_NAME, "Agent", "Location coordinates source", INDIGO_OK_STATE, INDIGO_RW_PERM, INDIGO_ONE_OF_MANY_RULE, 4);
		if (AGENT_SITE_DATA_SOURCE_PROPERTY == NULL)
			return INDIGO_FAILED;
		indigo_init_switch_item(AGENT_SITE_DATA_SOURCE_HOST_ITEM, AGENT_SITE_DATA_SOURCE_HOST_ITEM_NAME, "Use agent coordinates", true);
		indigo_init_switch_item(AGENT_SITE_DATA_SOURCE_MOUNT_ITEM, AGENT_SITE_DATA_SOURCE_MOUNT_ITEM_NAME, "Use mount coordinates", false);
		indigo_init_switch_item(AGENT_SITE_DATA_SOURCE_DOME_ITEM, AGENT_SITE_DATA_SOURCE_DOME_ITEM_NAME, "Use dome coordinates", false);
		indigo_init_switch_item(AGENT_SITE_DATA_SOURCE_GPS_ITEM, AGENT_SITE_DATA_SOURCE_GPS_ITEM_NAME, "Use GPS coordinates", false);
		// -------------------------------------------------------------------------------- AGENT_SET_HOST_TIME
		AGENT_SET_HOST_TIME_PROPERTY = indigo_init_switch_property(NULL, device->name, AGENT_SET_HOST_TIME_PROPERTY_NAME, "Agent", "Set host time", INDIGO_OK_STATE, INDIGO_RW_PERM, INDIGO_ANY_OF_MANY_RULE, 2);
		if (AGENT_SET_HOST_TIME_PROPERTY == NULL)
			return INDIGO_FAILED;
		indigo_init_switch_item(AGENT_SET_HOST_TIME_MOUNT_ITEM, AGENT_SET_HOST_TIME_MOUNT_ITEM_NAME, "Set host time to mount", true);
		indigo_init_switch_item(AGENT_SET_HOST_TIME_DOME_ITEM, AGENT_SET_HOST_TIME_DOME_ITEM_NAME, "Set host time to dome", true);
		// -------------------------------------------------------------------------------- AGENT_LX200_SERVER
		AGENT_LX200_SERVER_PROPERTY = indigo_init_switch_property(NULL, device->name, AGENT_LX200_SERVER_PROPERTY_NAME, "Agent", "LX200 Server state", INDIGO_OK_STATE, INDIGO_RW_PERM, INDIGO_ONE_OF_MANY_RULE, 2);
		if (AGENT_LX200_SERVER_PROPERTY == NULL)
			return INDIGO_FAILED;
		indigo_init_switch_item(AGENT_LX200_SERVER_STARTED_ITEM, AGENT_LX200_SERVER_STARTED_ITEM_NAME, "Start LX200 server", false);
		indigo_init_switch_item(AGENT_LX200_SERVER_STOPPED_ITEM, AGENT_LX200_SERVER_STOPPED_ITEM_NAME, "Stop LX200 server", true);
		AGENT_LX200_CONFIGURATION_PROPERTY = indigo_init_number_property(NULL, device->name, AGENT_LX200_CONFIGURATION_PROPERTY_NAME, "Agent", "LX200 Server configuration", INDIGO_OK_STATE, INDIGO_RW_PERM, 1);
		if (AGENT_LX200_CONFIGURATION_PROPERTY == NULL)
			return INDIGO_FAILED;
		indigo_init_number_item(AGENT_LX200_CONFIGURATION_PORT_ITEM, AGENT_LX200_CONFIGURATION_PORT_ITEM_NAME, "Server port", 0, 0xFFFF, 0, 4030);
		// -------------------------------------------------------------------------------- AGENT_LIMITS
		AGENT_LIMITS_PROPERTY = indigo_init_number_property(NULL, device->name, AGENT_LIMITS_PROPERTY_NAME, "Agent", "Limits", INDIGO_OK_STATE, INDIGO_RW_PERM, 3);
		if (AGENT_LIMITS_PROPERTY == NULL)
			return INDIGO_FAILED;
		indigo_init_sexagesimal_number_item(AGENT_HA_TRACKING_LIMIT_ITEM, AGENT_HA_TRACKING_LIMIT_ITEM_NAME, "HA tracking limit (0 to 24)", 0, 24, 0, 24);
		indigo_init_sexagesimal_number_item(AGENT_LOCAL_TIME_LIMIT_ITEM, AGENT_LOCAL_TIME_LIMIT_ITEM_NAME, "Time limit (0 to 24)", 0, 24, 0, 12);
		indigo_init_sexagesimal_number_item(AGENT_COORDINATES_PROPAGATE_THESHOLD_ITEM, AGENT_COORDINATES_PROPAGATE_THESHOLD_ITEM_NAME, "Change threshold (°)", 0, 360, 0, 5.0/3600.0);
		// -------------------------------------------------------------------------------- AGENT_MOUNT_FOV
		AGENT_MOUNT_FOV_PROPERTY = indigo_init_number_property(NULL, device->name, AGENT_MOUNT_FOV_PROPERTY_NAME, "Agent", "FOV", INDIGO_OK_STATE, INDIGO_RW_PERM, 3);
		if (AGENT_MOUNT_FOV_PROPERTY == NULL)
			return INDIGO_FAILED;
		indigo_init_sexagesimal_number_item(AGENT_MOUNT_FOV_ANGLE_ITEM, AGENT_MOUNT_FOV_ANGLE_ITEM_NAME, "Angle (°)", -360, 360, 0, 0);
		indigo_init_sexagesimal_number_item(AGENT_MOUNT_FOV_WIDTH_ITEM, AGENT_MOUNT_FOV_WIDTH_ITEM_NAME, "Width (°)", 0, 360, 0, 0);
		indigo_init_sexagesimal_number_item(AGENT_MOUNT_FOV_HEIGHT_ITEM, AGENT_MOUNT_FOV_HEIGHT_ITEM_NAME, "Height (°)", 0, 360, 0, 0);
		// --------------------------------------------------------------------------------
		CONNECTION_PROPERTY->hidden = true;
		ADDITIONAL_INSTANCES_PROPERTY->hidden = DEVICE_CONTEXT->base_device != NULL;
		pthread_mutex_init(&DEVICE_PRIVATE_DATA->mutex, NULL);
		indigo_load_properties(device, false);
		INDIGO_DEVICE_ATTACH_LOG(DRIVER_NAME, device->name);
		return agent_enumerate_properties(device, NULL, NULL);
	}
	return INDIGO_FAILED;
}

static indigo_result agent_enumerate_properties(indigo_device *device, indigo_client *client, indigo_property *property) {
	if (client != NULL && client == FILTER_DEVICE_CONTEXT->client)
		return INDIGO_OK;
	if (indigo_property_match(AGENT_GEOGRAPHIC_COORDINATES_PROPERTY, property))
		indigo_define_property(device, AGENT_GEOGRAPHIC_COORDINATES_PROPERTY, NULL);
	if (indigo_property_match(AGENT_SITE_DATA_SOURCE_PROPERTY, property))
		indigo_define_property(device, AGENT_SITE_DATA_SOURCE_PROPERTY, NULL);
	if (indigo_property_match(AGENT_SET_HOST_TIME_PROPERTY, property))
		indigo_define_property(device, AGENT_SET_HOST_TIME_PROPERTY, NULL);
	if (indigo_property_match(AGENT_LX200_SERVER_PROPERTY, property))
		indigo_define_property(device, AGENT_LX200_SERVER_PROPERTY, NULL);
	if (indigo_property_match(AGENT_LX200_CONFIGURATION_PROPERTY, property))
		indigo_define_property(device, AGENT_LX200_CONFIGURATION_PROPERTY, NULL);
	if (indigo_property_match(AGENT_LIMITS_PROPERTY, property))
		indigo_define_property(device, AGENT_LIMITS_PROPERTY, NULL);
	if (indigo_property_match(AGENT_MOUNT_FOV_PROPERTY, property))
		indigo_define_property(device, AGENT_MOUNT_FOV_PROPERTY, NULL);
	return indigo_filter_enumerate_properties(device, client, property);
}

typedef struct {
	int client_socket;
	indigo_device *device;
} handler_data;

static char * doubleToSexa(double value, char *format) {
	static char buffer[128];
	double d = fabs(value);
	double m = 60.0 * (d - floor(d));
	double s = round(60.0 * (m - floor(m)));
	if (s == 60) {
		s = 0;
		++m;
	}
	if (m == 60) {
		m = 0;
		++d;
	}
	if (value < 0) {
		d = -d;
	}
	snprintf(buffer, sizeof(buffer), format, (int)d, (int)m, (int)s);
	return buffer;
}

static const char *COORDINATE_NAMES[] = { MOUNT_EQUATORIAL_COORDINATES_RA_ITEM_NAME, MOUNT_EQUATORIAL_COORDINATES_DEC_ITEM_NAME };

static void worker_thread(handler_data *data) {
	indigo_device *device = data->device;
	int client_socket = data->client_socket;
	char buffer_in[128];
	char buffer_out[128];
	long result = 1;
	char mount_name[INDIGO_NAME_SIZE];
	struct timeval tv;
	tv.tv_sec = 0;
	tv.tv_usec = 500000;
	setsockopt(client_socket, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv, sizeof tv);
	INDIGO_DRIVER_TRACE(MOUNT_AGENT_NAME, "%d: CONNECTED", client_socket);
	while (true) {
		strcpy(mount_name, FILTER_DEVICE_CONTEXT->device_name[INDIGO_FILTER_MOUNT_INDEX]);
		if (*mount_name == 0)
			break;
		*buffer_in = 0;
		*buffer_out = 0;
		result = read(client_socket, buffer_in, 1);
		if (result <= 0)
			break;
		if (*buffer_in == 6) {
			strcpy(buffer_out, "P");
		} else if (*buffer_in == '#') {
			continue;
		} else if (*buffer_in == ':') {
			int i = 0;
			while (i < sizeof(buffer_in)) {
				result = read(client_socket, buffer_in + i, 1);
				if (result <= 0)
					break;
				if (buffer_in[i] == '#') {
					buffer_in[i] = 0;
					break;
				}
				i++;
			}
			if (result == -1)
				break;
			if (strcmp(buffer_in, "GVP") == 0) {
				strcpy(buffer_out, "indigo#");
			} else if (strcmp(buffer_in, "GR") == 0) {
				strcpy(buffer_out, doubleToSexa(DEVICE_PRIVATE_DATA->mount_ra, "%02d:%02d:%02d#"));
			} else if (strcmp(buffer_in, "GD") == 0) {
				strcpy(buffer_out, doubleToSexa(DEVICE_PRIVATE_DATA->mount_dec, "%+03d*%02d'%02d#"));
			} else if (strncmp(buffer_in, "Sr", 2) == 0) {
				int h = 0, m = 0;
				double s = 0;
				char c;
				if (sscanf(buffer_in + 2, "%d%c%d%c%lf", &h, &c, &m, &c, &s) == 5) {
					DEVICE_PRIVATE_DATA->mount_target_ra = h + m/60.0 + s/3600.0;
					strcpy(buffer_out, "1");
				} else if (sscanf(buffer_in + 2, "%d%c%d", &h, &c, &m) == 3) {
					DEVICE_PRIVATE_DATA->mount_target_ra = h + m/60.0;
					strcpy(buffer_out, "1");
				} else {
					strcpy(buffer_out, "0");
				}
			} else if (strncmp(buffer_in, "Sd", 2) == 0) {
				int d = 0, m = 0;
				double s = 0;
				char c;
				if (sscanf(buffer_in + 2, "%d%c%d%c%lf", &d, &c, &m, &c, &s) == 5) {
					DEVICE_PRIVATE_DATA->mount_target_dec = d > 0 ? d + m/60.0 + s/3600.0 : d - m/60.0 - s/3600.0;
					strcpy(buffer_out, "1");
				} else if (sscanf(buffer_in + 2, "%d%c%d", &d, &c, &m) == 3) {
					DEVICE_PRIVATE_DATA->mount_target_dec = d > 0 ? d + m/60.0 : d - m/60.0;
					strcpy(buffer_out, "1");
				} else {
					strcpy(buffer_out, "0");
				}
			} else if (strncmp(buffer_in, "MS", 2) == 0) {
				indigo_change_switch_property_1(FILTER_DEVICE_CONTEXT->client, mount_name, MOUNT_PARK_PROPERTY_NAME, MOUNT_PARK_UNPARKED_ITEM_NAME, true);
				indigo_change_switch_property_1(FILTER_DEVICE_CONTEXT->client, mount_name, MOUNT_ON_COORDINATES_SET_PROPERTY_NAME, MOUNT_ON_COORDINATES_SET_TRACK_ITEM_NAME, true);
				double values[] = { DEVICE_PRIVATE_DATA->mount_target_ra, DEVICE_PRIVATE_DATA->mount_target_dec };
				indigo_change_number_property(FILTER_DEVICE_CONTEXT->client, mount_name, MOUNT_EQUATORIAL_COORDINATES_PROPERTY_NAME, 2, COORDINATE_NAMES, values);
				strcpy(buffer_out, "0");
			} else if (strncmp(buffer_in, "CM", 2) == 0) {
				indigo_change_switch_property_1(FILTER_DEVICE_CONTEXT->client, mount_name, MOUNT_PARK_PROPERTY_NAME, MOUNT_PARK_UNPARKED_ITEM_NAME, true);
				indigo_change_switch_property_1(FILTER_DEVICE_CONTEXT->client, mount_name, MOUNT_ON_COORDINATES_SET_PROPERTY_NAME, MOUNT_ON_COORDINATES_SET_SYNC_ITEM_NAME, true);
				double values[] = { DEVICE_PRIVATE_DATA->mount_target_ra, DEVICE_PRIVATE_DATA->mount_target_dec };
				indigo_change_number_property(FILTER_DEVICE_CONTEXT->client, mount_name, MOUNT_EQUATORIAL_COORDINATES_PROPERTY_NAME, 2, COORDINATE_NAMES, values);
				strcpy(buffer_out, "OK#");
			} else if (strcmp(buffer_in, "RG") == 0) {
				indigo_change_switch_property_1(FILTER_DEVICE_CONTEXT->client, mount_name, MOUNT_SLEW_RATE_PROPERTY_NAME, MOUNT_SLEW_RATE_GUIDE_ITEM_NAME, true);
			} else if (strcmp(buffer_in, "RC") == 0) {
				indigo_change_switch_property_1(FILTER_DEVICE_CONTEXT->client, mount_name, MOUNT_SLEW_RATE_PROPERTY_NAME, MOUNT_SLEW_RATE_CENTERING_ITEM_NAME, true);
			} else if (strcmp(buffer_in, "RM") == 0) {
				indigo_change_switch_property_1(FILTER_DEVICE_CONTEXT->client, mount_name, MOUNT_SLEW_RATE_PROPERTY_NAME, MOUNT_SLEW_RATE_FIND_ITEM_NAME, true);
			} else if (strcmp(buffer_in, "RS") == 0) {
				indigo_change_switch_property_1(FILTER_DEVICE_CONTEXT->client, mount_name, MOUNT_SLEW_RATE_PROPERTY_NAME, MOUNT_SLEW_RATE_MAX_ITEM_NAME, true);
			} else if (strcmp(buffer_in, "Sw1") == 0) {
				indigo_change_switch_property_1(FILTER_DEVICE_CONTEXT->client, mount_name, MOUNT_SLEW_RATE_PROPERTY_NAME, MOUNT_SLEW_RATE_GUIDE_ITEM_NAME, true);
				strcpy(buffer_out, "1");
			} else if (strcmp(buffer_in, "Sw2") == 0) {
				indigo_change_switch_property_1(FILTER_DEVICE_CONTEXT->client, mount_name, MOUNT_SLEW_RATE_PROPERTY_NAME, MOUNT_SLEW_RATE_CENTERING_ITEM_NAME, true);
				strcpy(buffer_out, "1");
			} else if (strcmp(buffer_in, "Sw3") == 0) {
				indigo_change_switch_property_1(FILTER_DEVICE_CONTEXT->client, mount_name, MOUNT_SLEW_RATE_PROPERTY_NAME, MOUNT_SLEW_RATE_FIND_ITEM_NAME, true);
				strcpy(buffer_out, "1");
			} else if (strcmp(buffer_in, "Sw4") == 0) {
				indigo_change_switch_property_1(FILTER_DEVICE_CONTEXT->client, mount_name, MOUNT_SLEW_RATE_PROPERTY_NAME, MOUNT_SLEW_RATE_MAX_ITEM_NAME, true);
				strcpy(buffer_out, "1");
			} else if (strcmp(buffer_in, "Mn") == 0) {
				indigo_change_switch_property_1(FILTER_DEVICE_CONTEXT->client, mount_name, MOUNT_PARK_PROPERTY_NAME, MOUNT_PARK_UNPARKED_ITEM_NAME, true);
				indigo_change_switch_property_1(FILTER_DEVICE_CONTEXT->client, mount_name, MOUNT_MOTION_DEC_PROPERTY_NAME, MOUNT_MOTION_NORTH_ITEM_NAME, true);
			} else if (strcmp(buffer_in, "Qn") == 0) {
				indigo_change_switch_property_1(FILTER_DEVICE_CONTEXT->client, mount_name, MOUNT_MOTION_DEC_PROPERTY_NAME, MOUNT_MOTION_NORTH_ITEM_NAME, false);
			} else if (strcmp(buffer_in, "Ms") == 0) {
				indigo_change_switch_property_1(FILTER_DEVICE_CONTEXT->client, mount_name, MOUNT_PARK_PROPERTY_NAME, MOUNT_PARK_UNPARKED_ITEM_NAME, true);
				indigo_change_switch_property_1(FILTER_DEVICE_CONTEXT->client, mount_name, MOUNT_MOTION_DEC_PROPERTY_NAME, MOUNT_MOTION_SOUTH_ITEM_NAME, true);
			} else if (strcmp(buffer_in, "Qs") == 0) {
				indigo_change_switch_property_1(FILTER_DEVICE_CONTEXT->client, mount_name, MOUNT_MOTION_DEC_PROPERTY_NAME, MOUNT_MOTION_SOUTH_ITEM_NAME, false);
			} else if (strcmp(buffer_in, "Mw") == 0) {
				indigo_change_switch_property_1(FILTER_DEVICE_CONTEXT->client, mount_name, MOUNT_PARK_PROPERTY_NAME, MOUNT_PARK_UNPARKED_ITEM_NAME, true);
				indigo_change_switch_property_1(FILTER_DEVICE_CONTEXT->client, mount_name, MOUNT_MOTION_RA_PROPERTY_NAME, MOUNT_MOTION_WEST_ITEM_NAME, true);
			} else if (strcmp(buffer_in, "Qw") == 0) {
				indigo_change_switch_property_1(FILTER_DEVICE_CONTEXT->client, mount_name, MOUNT_MOTION_RA_PROPERTY_NAME, MOUNT_MOTION_WEST_ITEM_NAME, false);
			} else if (strcmp(buffer_in, "Me") == 0) {
				indigo_change_switch_property_1(FILTER_DEVICE_CONTEXT->client, mount_name, MOUNT_PARK_PROPERTY_NAME, MOUNT_PARK_UNPARKED_ITEM_NAME, true);
				indigo_change_switch_property_1(FILTER_DEVICE_CONTEXT->client, mount_name, MOUNT_MOTION_RA_PROPERTY_NAME, MOUNT_MOTION_EAST_ITEM_NAME, true);
			} else if (strcmp(buffer_in, "Qe") == 0) {
				indigo_change_switch_property_1(FILTER_DEVICE_CONTEXT->client, mount_name, MOUNT_MOTION_RA_PROPERTY_NAME, MOUNT_MOTION_EAST_ITEM_NAME, false);
			} else if (strcmp(buffer_in, "Q") == 0) {
				indigo_change_switch_property_1(FILTER_DEVICE_CONTEXT->client, mount_name, MOUNT_ABORT_MOTION_PROPERTY_NAME, MOUNT_ABORT_MOTION_ITEM_NAME, true);
			} else if (strncmp(buffer_in, "SC", 2) == 0) {
				strcpy(buffer_out, "1Updating        planetary data. #                              #");
			} else if (strncmp(buffer_in, "S", 1) == 0) {
				strcpy(buffer_out, "1");
			}
			if (*buffer_out) {
				indigo_write(client_socket, buffer_out, strlen(buffer_out));
				if (*buffer_in == 'G')
					INDIGO_DRIVER_DEBUG(MOUNT_AGENT_NAME, "%d: '%s' -> '%s'", client_socket, buffer_in, buffer_out);
				else
					INDIGO_DRIVER_DEBUG(MOUNT_AGENT_NAME, "%d: '%s' -> '%s'", client_socket, buffer_in, buffer_out);
			} else {
				INDIGO_DRIVER_DEBUG(MOUNT_AGENT_NAME, "%d: '%s' -> ", client_socket, buffer_in);
			}
		}
	}
	INDIGO_DRIVER_TRACE(MOUNT_AGENT_NAME, "%d: DISCONNECTED", client_socket);
	close(client_socket);
	free(data);
}

static void start_lx200_server(indigo_device *device) {
	struct sockaddr_in client_name;
	unsigned int name_len = sizeof(client_name);

	int port = (int)AGENT_LX200_CONFIGURATION_PORT_ITEM->number.value;
	int server_socket = socket(PF_INET, SOCK_STREAM, 0);
	if (server_socket == -1) {
		AGENT_LX200_SERVER_PROPERTY->state = INDIGO_ALERT_STATE;
		indigo_update_property(device, AGENT_LX200_SERVER_PROPERTY, "%s: socket() failed (%s)", MOUNT_AGENT_NAME, strerror(errno));
		return;
	}
	int reuse = 1;
	if (setsockopt(server_socket, SOL_SOCKET,SO_REUSEADDR, &reuse, sizeof(reuse)) < 0) {
		close(server_socket);
		indigo_set_switch(AGENT_LX200_SERVER_PROPERTY, AGENT_LX200_SERVER_STOPPED_ITEM, true);
		AGENT_LX200_SERVER_PROPERTY->state = INDIGO_ALERT_STATE;
		indigo_update_property(device, AGENT_LX200_SERVER_PROPERTY, "%s: setsockopt() failed (%s)", MOUNT_AGENT_NAME, strerror(errno));
		return;
	}
	struct sockaddr_in server_address;
	unsigned int server_address_length = sizeof(server_address);
	server_address.sin_family = AF_INET;
	server_address.sin_port = htons(port);
	server_address.sin_addr.s_addr = htonl(INADDR_ANY);
	if (bind(server_socket, (struct sockaddr *)&server_address, server_address_length) < 0) {
		close(server_socket);
		indigo_set_switch(AGENT_LX200_SERVER_PROPERTY, AGENT_LX200_SERVER_STOPPED_ITEM, true);
		AGENT_LX200_SERVER_PROPERTY->state = INDIGO_ALERT_STATE;
		indigo_update_property(device, AGENT_LX200_SERVER_PROPERTY, "%s: bind() failed (%s)", MOUNT_AGENT_NAME, strerror(errno));
		return;
	}
	if (getsockname(server_socket, (struct sockaddr *)&(server_address), &server_address_length) < 0) {
		close(server_socket);
		indigo_set_switch(AGENT_LX200_SERVER_PROPERTY, AGENT_LX200_SERVER_STOPPED_ITEM, true);
		AGENT_LX200_SERVER_PROPERTY->state = INDIGO_ALERT_STATE;
		indigo_update_property(device, AGENT_LX200_SERVER_PROPERTY, "%s: getsockname() failed (%s)", MOUNT_AGENT_NAME, strerror(errno));
		return;
	}
	if (listen(server_socket, 5) < 0) {
		close(server_socket);
		indigo_set_switch(AGENT_LX200_SERVER_PROPERTY, AGENT_LX200_SERVER_STOPPED_ITEM, true);
		AGENT_LX200_SERVER_PROPERTY->state = INDIGO_ALERT_STATE;
		indigo_update_property(device, AGENT_LX200_SERVER_PROPERTY, "%s: Can't listen on server socket (%s)", MOUNT_AGENT_NAME, strerror(errno));
		return;
	}
	if (port == 0) {
		AGENT_LX200_CONFIGURATION_PORT_ITEM->number.value = port = ntohs(server_address.sin_port);
		AGENT_LX200_CONFIGURATION_PROPERTY->state = INDIGO_OK_STATE;
		indigo_update_property(device, AGENT_LX200_CONFIGURATION_PROPERTY, NULL);
	}
	DEVICE_PRIVATE_DATA->server_socket = server_socket;
	AGENT_LX200_SERVER_PROPERTY->state = INDIGO_OK_STATE;
	indigo_update_property(device, AGENT_LX200_SERVER_PROPERTY, "Server started on %d", (int)AGENT_LX200_CONFIGURATION_PORT_ITEM->number.value);
	while (DEVICE_PRIVATE_DATA->server_socket) {
		int client_socket = accept(DEVICE_PRIVATE_DATA->server_socket, (struct sockaddr *)&client_name, &name_len);
		if (client_socket != -1) {
			handler_data *data = indigo_safe_malloc(sizeof(handler_data));
			data->client_socket = client_socket;
			data->device = device;
			if (!indigo_async((void *(*)(void *))worker_thread, data))
				INDIGO_DRIVER_ERROR(MOUNT_AGENT_NAME, "Can't create worker thread for connection (%s)", strerror(errno));
		}
	}
	AGENT_LX200_SERVER_PROPERTY->state = INDIGO_OK_STATE;
	indigo_set_switch(AGENT_LX200_SERVER_PROPERTY, AGENT_LX200_SERVER_STOPPED_ITEM, true);
	indigo_update_property(device, AGENT_LX200_SERVER_PROPERTY, "Server finished");
}

static void stop_lx200_server(indigo_device *device) {
	int server_socket = DEVICE_PRIVATE_DATA->server_socket;
	if (server_socket) {
		AGENT_LX200_SERVER_PROPERTY->state = INDIGO_BUSY_STATE;
		indigo_update_property(device, AGENT_LX200_SERVER_PROPERTY, NULL);
		DEVICE_PRIVATE_DATA->server_socket = 0;
		shutdown(server_socket, SHUT_RDWR);
		close(server_socket);
	}
}

static indigo_result agent_change_property(indigo_device *device, indigo_client *client, indigo_property *property) {
	assert(device != NULL);
	assert(DEVICE_CONTEXT != NULL);
	assert(property != NULL);
	if (client == FILTER_DEVICE_CONTEXT->client)
		return INDIGO_OK;
	if (indigo_property_match(AGENT_SITE_DATA_SOURCE_PROPERTY, property)) {
			// -------------------------------------------------------------------------------- AGENT_SITE_DATA_SOURCE
			indigo_property_copy_values(AGENT_SITE_DATA_SOURCE_PROPERTY, property, false);
			set_site_coordinates(device);
			AGENT_SITE_DATA_SOURCE_PROPERTY->state = INDIGO_OK_STATE;
			save_config(device);
			indigo_update_property(device, AGENT_SITE_DATA_SOURCE_PROPERTY, NULL);
			return INDIGO_OK;
	} else if (indigo_property_match(AGENT_SET_HOST_TIME_PROPERTY, property)) {
	// -------------------------------------------------------------------------------- AGENT_SET_HOST_TIME
		indigo_property_copy_values(AGENT_SET_HOST_TIME_PROPERTY, property, false);
		if (AGENT_SET_HOST_TIME_MOUNT_ITEM->sw.value && *FILTER_DEVICE_CONTEXT->device_name[INDIGO_FILTER_MOUNT_INDEX])
			indigo_change_switch_property_1(FILTER_DEVICE_CONTEXT->client, FILTER_DEVICE_CONTEXT->device_name[INDIGO_FILTER_MOUNT_INDEX], MOUNT_SET_HOST_TIME_PROPERTY_NAME, MOUNT_SET_HOST_TIME_ITEM_NAME, true);
		if (AGENT_SET_HOST_TIME_DOME_ITEM->sw.value && *FILTER_DEVICE_CONTEXT->device_name[INDIGO_FILTER_DOME_INDEX])
			indigo_change_switch_property_1(FILTER_DEVICE_CONTEXT->client, FILTER_DEVICE_CONTEXT->device_name[INDIGO_FILTER_DOME_INDEX], DOME_SET_HOST_TIME_PROPERTY_NAME, DOME_SET_HOST_TIME_ITEM_NAME, true);
		AGENT_SET_HOST_TIME_PROPERTY->state = INDIGO_OK_STATE;
		save_config(device);
		indigo_update_property(device, AGENT_SET_HOST_TIME_PROPERTY, NULL);
		return INDIGO_OK;
	 } else if (indigo_property_match(AGENT_GEOGRAPHIC_COORDINATES_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- AGENT_GEOGRAPHIC_COORDINATES
		indigo_property_copy_values(AGENT_GEOGRAPHIC_COORDINATES_PROPERTY, property, false);
		set_site_coordinates(device);
		AGENT_GEOGRAPHIC_COORDINATES_PROPERTY->state = INDIGO_OK_STATE;
		save_config(device);
		indigo_update_property(device, AGENT_GEOGRAPHIC_COORDINATES_PROPERTY, NULL);
		return INDIGO_OK;
	} else if (indigo_property_match(AGENT_LX200_SERVER_PROPERTY, property)) {
			// -------------------------------------------------------------------------------- LX200_SERVER
		indigo_property_copy_values(AGENT_LX200_SERVER_PROPERTY, property, false);
		AGENT_LX200_SERVER_PROPERTY->state = INDIGO_BUSY_STATE;
		if (AGENT_LX200_SERVER_STARTED_ITEM->sw.value) {
			indigo_set_timer(device, 0, start_lx200_server, NULL);
		} else {
			indigo_set_timer(device, 0, stop_lx200_server, NULL);
		}
		indigo_update_property(device, AGENT_LX200_SERVER_PROPERTY, NULL);
		return INDIGO_OK;
	} else if (indigo_property_match(AGENT_LX200_CONFIGURATION_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- AGENT_LX200_CONFIGURATION
		indigo_property_copy_values(AGENT_LX200_CONFIGURATION_PROPERTY, property, false);
		AGENT_LX200_CONFIGURATION_PROPERTY->state = INDIGO_OK_STATE;
		save_config(device);
		indigo_update_property(device, AGENT_LX200_SERVER_PROPERTY, NULL);
		return INDIGO_OK;
	} else if (indigo_property_match(AGENT_LIMITS_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- AGENT_LIMITS
		indigo_property_copy_values(AGENT_LIMITS_PROPERTY, property, false);
		AGENT_LIMITS_PROPERTY->state = INDIGO_OK_STATE;
		save_config(device);
		indigo_update_property(device, AGENT_LIMITS_PROPERTY, NULL);
		return INDIGO_OK;
	} else if (indigo_property_match(AGENT_MOUNT_FOV_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- AGENT_MOUNT_FOV
		indigo_property_copy_values(AGENT_MOUNT_FOV_PROPERTY, property, false);
		AGENT_MOUNT_FOV_PROPERTY->state = INDIGO_OK_STATE;
		save_config(device);
		indigo_update_property(device, AGENT_MOUNT_FOV_PROPERTY, NULL);
		return INDIGO_OK;
		// -------------------------------------------------------------------------------- ADDITIONAL_INSTANCES
	} else if (indigo_property_match(ADDITIONAL_INSTANCES_PROPERTY, property)) {
		if (indigo_filter_change_property(device, client, property) == INDIGO_OK) {
			save_config(device);
		}
		return INDIGO_OK;
	}
	return indigo_filter_change_property(device, client, property);
}

static indigo_result agent_device_detach(indigo_device *device) {
	assert(device != NULL);
	save_config(device);
	indigo_release_property(AGENT_GEOGRAPHIC_COORDINATES_PROPERTY);
	indigo_release_property(AGENT_SITE_DATA_SOURCE_PROPERTY);
	indigo_release_property(AGENT_SET_HOST_TIME_PROPERTY);
	indigo_release_property(AGENT_LX200_SERVER_PROPERTY);
	indigo_release_property(AGENT_LX200_CONFIGURATION_PROPERTY);
	indigo_release_property(AGENT_LIMITS_PROPERTY);
	indigo_release_property(AGENT_MOUNT_FOV_PROPERTY);
	pthread_mutex_destroy(&DEVICE_PRIVATE_DATA->mutex);
	return indigo_filter_device_detach(device);
}

// -------------------------------------------------------------------------------- INDIGO agent client implementation

static void process_snooping(indigo_client *client, indigo_device *device, indigo_property *property) {
	if (*FILTER_CLIENT_CONTEXT->device_name[INDIGO_FILTER_MOUNT_INDEX] && !strcmp(property->device, FILTER_CLIENT_CONTEXT->device_name[INDIGO_FILTER_MOUNT_INDEX])) {
		if (!strcmp(property->name, CONNECTION_PROPERTY_NAME)) {
			if (property->state == INDIGO_OK_STATE) {
				indigo_set_timer(FILTER_CLIENT_CONTEXT->device, 1, set_site_coordinates, NULL);
			}
		} else if (!strcmp(property->name, GEOGRAPHIC_COORDINATES_PROPERTY_NAME)) {
			if (property->state == INDIGO_OK_STATE) {
				bool changed = false;
				for (int i = 0; i < property->count; i++) {
					if (!strcmp(property->items[i].name, GEOGRAPHIC_COORDINATES_LATITUDE_ITEM_NAME)) {
						changed = changed || fabs(CLIENT_PRIVATE_DATA->mount_latitude - property->items[i].number.value) > CLIENT_PRIVATE_DATA->agent_limits_property->items[2].number.value;
						CLIENT_PRIVATE_DATA->mount_latitude = property->items[i].number.value;
					} else if (!strcmp(property->items[i].name, GEOGRAPHIC_COORDINATES_LONGITUDE_ITEM_NAME)) {
						changed = changed || fabs(CLIENT_PRIVATE_DATA->mount_longitude - property->items[i].number.value) > CLIENT_PRIVATE_DATA->agent_limits_property->items[2].number.value;
						CLIENT_PRIVATE_DATA->mount_longitude = property->items[i].number.value;
					} else if (!strcmp(property->items[i].name, GEOGRAPHIC_COORDINATES_ELEVATION_ITEM_NAME)) {
						CLIENT_PRIVATE_DATA->mount_elevation = property->items[i].number.value;
					}
				}
				if (changed && CLIENT_PRIVATE_DATA->agent_site_data_source_property->items[1].sw.value)
					indigo_set_timer(FILTER_CLIENT_CONTEXT->device, 1, set_site_coordinates, NULL);
			}
		} else if (!strcmp(property->name, MOUNT_EQUATORIAL_COORDINATES_PROPERTY_NAME)) {
			for (int i = 0; i < property->count; i++) {
				if (!strcmp(property->items[i].name, MOUNT_EQUATORIAL_COORDINATES_RA_ITEM_NAME))
					CLIENT_PRIVATE_DATA->mount_ra = property->items[i].number.value;
				else if (!strcmp(property->items[i].name, MOUNT_EQUATORIAL_COORDINATES_DEC_ITEM_NAME))
					CLIENT_PRIVATE_DATA->mount_dec = property->items[i].number.value;
			}
			if (property->state != INDIGO_ALERT_STATE) {
				if (*FILTER_CLIENT_CONTEXT->device_name[INDIGO_FILTER_DOME_INDEX] && CLIENT_PRIVATE_DATA->dome_unparked) {
					static const char *names[] = { DOME_EQUATORIAL_COORDINATES_RA_ITEM_NAME, DOME_EQUATORIAL_COORDINATES_DEC_ITEM_NAME };
					double values[] = { CLIENT_PRIVATE_DATA->mount_ra, CLIENT_PRIVATE_DATA->mount_dec };
					indigo_change_number_property(FILTER_CLIENT_CONTEXT->client, FILTER_CLIENT_CONTEXT->device_name[INDIGO_FILTER_DOME_INDEX], DOME_EQUATORIAL_COORDINATES_PROPERTY_NAME, 2, names, values);
				}
			}
			if (property->state == INDIGO_OK_STATE) {
				set_eq_coordinates(FILTER_CLIENT_CONTEXT->device);
			} else {
				abort_capture(FILTER_CLIENT_CONTEXT->device);
				abort_guiding(FILTER_CLIENT_CONTEXT->device);
			}
		} else if (!strcmp(property->name, MOUNT_PARK_PROPERTY_NAME)) {
			if (property->state == INDIGO_OK_STATE) {
				if (*FILTER_CLIENT_CONTEXT->device_name[INDIGO_FILTER_DOME_INDEX]) {
					char *dome_name = FILTER_CLIENT_CONTEXT->device_name[INDIGO_FILTER_DOME_INDEX];
					for (int i = 0; i < property->count; i++) {
						indigo_item *item = property->items + i;
						if (!strcmp(item->name, MOUNT_PARK_PARKED_ITEM_NAME) && item->sw.value) {
							indigo_change_switch_property_1(FILTER_CLIENT_CONTEXT->client, dome_name, DOME_PARK_PROPERTY_NAME, DOME_PARK_PARKED_ITEM_NAME, true);
							indigo_change_switch_property_1(FILTER_CLIENT_CONTEXT->client, dome_name, DOME_SHUTTER_PROPERTY_NAME, DOME_SHUTTER_CLOSED_ITEM_NAME, true);
						} else if (!strcmp(property->items[i].name, MOUNT_PARK_UNPARKED_ITEM_NAME)) {
							indigo_change_switch_property_1(FILTER_CLIENT_CONTEXT->client, dome_name, DOME_PARK_PROPERTY_NAME, DOME_PARK_UNPARKED_ITEM_NAME, true);
							indigo_change_switch_property_1(FILTER_CLIENT_CONTEXT->client, dome_name, DOME_SHUTTER_PROPERTY_NAME, DOME_SHUTTER_OPENED_ITEM_NAME, true);
						}
					}
				}
			}
			for (int i = 0; i < property->count; i++) {
				if (!strcmp(property->items[i].name, MOUNT_PARK_PARKED_ITEM_NAME)) {
					if (property->items[i].sw.value) {
						abort_capture(FILTER_CLIENT_CONTEXT->device);
						abort_guiding(FILTER_CLIENT_CONTEXT->device);
					}
				}
			}
		}
	} else if (!strcmp(property->name, MOUNT_LST_TIME_PROPERTY_NAME)) {
		for (int i = 0; i < property->count; i++) {
			if (!strcmp(property->items[i].name, MOUNT_LST_TIME_ITEM_NAME)) {
				double lst = property->items[i].number.value;
				double ha = fmod(lst - CLIENT_PRIVATE_DATA->mount_ra + 24, 24);
				time_t timer;
				time(&timer);
				struct tm *info = localtime(&timer);
				double now = info->tm_hour + info->tm_min / 60.0 + info->tm_sec / 3600.0;
				CLIENT_PRIVATE_DATA->agent_limits_property->items[0].number.value = ha;
				CLIENT_PRIVATE_DATA->agent_limits_property->items[1].number.value = now;
				indigo_update_property(FILTER_CLIENT_CONTEXT->device, CLIENT_PRIVATE_DATA->agent_limits_property, NULL);
				if (property->state == INDIGO_OK_STATE) {
					indigo_property *agent_park_property;
					if (indigo_filter_cached_property(FILTER_CLIENT_CONTEXT->device, INDIGO_FILTER_MOUNT_INDEX, MOUNT_PARK_PROPERTY_NAME, NULL, &agent_park_property) && agent_park_property->state == INDIGO_OK_STATE) {
						for (int j = 0; j < agent_park_property->count; j++) {
							if (!strcmp(agent_park_property->items[j].name, MOUNT_PARK_UNPARKED_ITEM_NAME)) {
								if (agent_park_property->items[j].sw.value) {
									bool park = false;
									if (ha > CLIENT_PRIVATE_DATA->agent_limits_property->items[0].number.target) {
										park = true;
										indigo_send_message(FILTER_CLIENT_CONTEXT->device, "Hour angle tracking limit reached");
									}
									double target = CLIENT_PRIVATE_DATA->agent_limits_property->items[1].number.target;
									if (now < 12 && target < 12 && now > target) {
										park = true;
										indigo_send_message(FILTER_CLIENT_CONTEXT->device, "Time limit reached");
									}
									if (now > 12 && target > 12 && now > target) {
										park = true;
										indigo_send_message(FILTER_CLIENT_CONTEXT->device, "Time limit reached");
									}
									if (park) {
										abort_capture(FILTER_CLIENT_CONTEXT->device);
										abort_guiding(FILTER_CLIENT_CONTEXT->device);
										indigo_change_switch_property_1(FILTER_CLIENT_CONTEXT->client, FILTER_CLIENT_CONTEXT->device_name[INDIGO_FILTER_MOUNT_INDEX], MOUNT_PARK_PROPERTY_NAME, MOUNT_PARK_PARKED_ITEM_NAME, true);
									}
									break;
								}
							}
						}
					}
				}
				break;
			}
		}
	} else if (*FILTER_CLIENT_CONTEXT->device_name[INDIGO_FILTER_DOME_INDEX] && !strcmp(property->device, FILTER_CLIENT_CONTEXT->device_name[INDIGO_FILTER_DOME_INDEX])) {
		if (!strcmp(property->name, CONNECTION_PROPERTY_NAME)) {
			if (property->state == INDIGO_OK_STATE) {
				indigo_set_timer(FILTER_CLIENT_CONTEXT->device, 1, set_site_coordinates, NULL);
			}
		} else if (!strcmp(property->name, GEOGRAPHIC_COORDINATES_PROPERTY_NAME)) {
			if (property->state == INDIGO_OK_STATE) {
				bool changed = false;
				for (int i = 0; i < property->count; i++) {
					if (!strcmp(property->items[i].name, GEOGRAPHIC_COORDINATES_LATITUDE_ITEM_NAME)) {
						changed = changed || fabs(CLIENT_PRIVATE_DATA->dome_latitude - property->items[i].number.value) > CLIENT_PRIVATE_DATA->agent_limits_property->items[2].number.value;
						CLIENT_PRIVATE_DATA->dome_latitude = property->items[i].number.value;
					} else if (!strcmp(property->items[i].name, GEOGRAPHIC_COORDINATES_LONGITUDE_ITEM_NAME)) {
						changed = changed || fabs(CLIENT_PRIVATE_DATA->dome_longitude - property->items[i].number.value) > CLIENT_PRIVATE_DATA->agent_limits_property->items[2].number.value;
						CLIENT_PRIVATE_DATA->dome_longitude = property->items[i].number.value;
					} else if (!strcmp(property->items[i].name, GEOGRAPHIC_COORDINATES_ELEVATION_ITEM_NAME)) {
						CLIENT_PRIVATE_DATA->dome_elevation = property->items[i].number.value;
					}
				}
				if (changed && CLIENT_PRIVATE_DATA->agent_site_data_source_property->items[2].sw.value)
					indigo_set_timer(FILTER_CLIENT_CONTEXT->device, 1, set_site_coordinates, NULL);
			}
		} else if (!strcmp(property->name, DOME_PARK_PROPERTY_NAME)) {
			CLIENT_PRIVATE_DATA->dome_unparked = false;
			if (property->state == INDIGO_OK_STATE) {
				for (int i = 0; i < property->count; i++) {
					if (!strcmp(property->items[i].name, DOME_PARK_UNPARKED_ITEM_NAME)) {
						CLIENT_PRIVATE_DATA->dome_unparked = property->items[i].sw.value;
						break;
					}
				}
			}
		}
	} else if (*FILTER_CLIENT_CONTEXT->device_name[INDIGO_FILTER_GPS_INDEX] && !strcmp(property->device, FILTER_CLIENT_CONTEXT->device_name[INDIGO_FILTER_GPS_INDEX])) {
		if (!strcmp(property->name, GEOGRAPHIC_COORDINATES_PROPERTY_NAME)) {
			if (property->state == INDIGO_OK_STATE) {
				bool changed = false;
				for (int i = 0; i < property->count; i++) {
					if (!strcmp(property->items[i].name, GEOGRAPHIC_COORDINATES_LATITUDE_ITEM_NAME)) {
						changed = changed || fabs(CLIENT_PRIVATE_DATA->gps_latitude - property->items[i].number.value) > CLIENT_PRIVATE_DATA->agent_limits_property->items[2].number.value;
						CLIENT_PRIVATE_DATA->gps_latitude = property->items[i].number.value;
					} else if (!strcmp(property->items[i].name, GEOGRAPHIC_COORDINATES_LONGITUDE_ITEM_NAME)) {
						changed = changed || fabs(CLIENT_PRIVATE_DATA->gps_longitude - property->items[i].number.value) > CLIENT_PRIVATE_DATA->agent_limits_property->items[2].number.value;
						CLIENT_PRIVATE_DATA->gps_longitude = property->items[i].number.value;
					} else if (!strcmp(property->items[i].name, GEOGRAPHIC_COORDINATES_ELEVATION_ITEM_NAME)) {
						CLIENT_PRIVATE_DATA->gps_elevation = property->items[i].number.value;
					}
				}
				if (changed && CLIENT_PRIVATE_DATA->agent_site_data_source_property->items[3].sw.value)
					indigo_set_timer(FILTER_CLIENT_CONTEXT->device, 1, set_site_coordinates, NULL);
			}
		}
	} else if (*FILTER_CLIENT_CONTEXT->device_name[INDIGO_FILTER_JOYSTICK_INDEX] && !strcmp(property->device, FILTER_CLIENT_CONTEXT->device_name[INDIGO_FILTER_JOYSTICK_INDEX])) {
		if (*FILTER_CLIENT_CONTEXT->device_name[INDIGO_FILTER_MOUNT_INDEX] && property->state == INDIGO_OK_STATE && (!strcmp(property->name, MOUNT_PARK_PROPERTY_NAME) || !strcmp(property->name, MOUNT_SLEW_RATE_PROPERTY_NAME) || !strcmp(property->name, MOUNT_MOTION_DEC_PROPERTY_NAME) || !strcmp(property->name, MOUNT_MOTION_RA_PROPERTY_NAME) || !strcmp(property->name, MOUNT_ABORT_MOTION_PROPERTY_NAME) || !strcmp(property->name, MOUNT_TRACKING_PROPERTY_NAME))) {
			indigo_filter_forward_change_property(client, property, FILTER_CLIENT_CONTEXT->device_name[INDIGO_FILTER_MOUNT_INDEX]);
		}
	}
}

static indigo_result agent_define_property(indigo_client *client, indigo_device *device, indigo_property *property, const char *message) {
	if (!strncmp(property->device, "Imager Agent", 12) && !strcmp(property->name, CCD_FITS_HEADERS_PROPERTY_NAME)) {
		set_site_coordinates3(FILTER_CLIENT_CONTEXT->device);
	} else {
		process_snooping(client, device, property);
	}
	return indigo_filter_define_property(client, device, property, message);
}

static indigo_result agent_update_property(indigo_client *client, indigo_device *device, indigo_property *property, const char *message) {
	if (!strcmp(property->device, MOUNT_AGENT_NAME) && !strcmp(property->name, FILTER_MOUNT_LIST_PROPERTY_NAME)) {
		if (!property->items->sw.value && property->state == INDIGO_OK_STATE) {
			indigo_property *device_selection_property;
			if (indigo_filter_cached_property(FILTER_CLIENT_CONTEXT->device, INDIGO_FILTER_MOUNT_INDEX, MOUNT_ALIGNMENT_SELECT_POINTS_PROPERTY_NAME, &device_selection_property, NULL)) {
				for (int i = 0; i < device_selection_property->count; i++) {
					if (device_selection_property->items[i].sw.value) {
						indigo_send_message(FILTER_CLIENT_CONTEXT->device, "There are active saved alignment points. Make sure you you want to use them.");
						break;
					}
				}
			}
		}
	} else {
		process_snooping(client, device, property);
	}
	return indigo_filter_update_property(client, device, property, message);
}

// -------------------------------------------------------------------------------- Initialization

static agent_private_data *private_data = NULL;

static indigo_device *agent_device = NULL;
static indigo_client *agent_client = NULL;

indigo_result indigo_agent_mount(indigo_driver_action action, indigo_driver_info *info) {
	static indigo_device agent_device_template = INDIGO_DEVICE_INITIALIZER(
		MOUNT_AGENT_NAME,
		agent_device_attach,
		agent_enumerate_properties,
		agent_change_property,
		NULL,
		agent_device_detach
	);

	static indigo_client agent_client_template = {
		MOUNT_AGENT_NAME, false, NULL, INDIGO_OK, INDIGO_VERSION_CURRENT, NULL,
		indigo_filter_client_attach,
		agent_define_property,
		agent_update_property,
		indigo_filter_delete_property,
		NULL,
		indigo_filter_client_detach
	};

	static indigo_driver_action last_action = INDIGO_DRIVER_SHUTDOWN;

	SET_DRIVER_INFO(info, MOUNT_AGENT_NAME, __FUNCTION__, DRIVER_VERSION, false, last_action);

	if (action == last_action)
		return INDIGO_OK;

	switch(action) {
		case INDIGO_DRIVER_INIT:
			last_action = action;
			private_data = indigo_safe_malloc(sizeof(agent_private_data));
			agent_device = indigo_safe_malloc_copy(sizeof(indigo_device), &agent_device_template);
			agent_device->private_data = private_data;
			indigo_attach_device(agent_device);

			agent_client = indigo_safe_malloc_copy(sizeof(indigo_client), &agent_client_template);
			agent_client->client_context = agent_device->device_context;
			indigo_attach_client(agent_client);
			break;

		case INDIGO_DRIVER_SHUTDOWN:
			last_action = action;
			if (agent_client != NULL) {
				indigo_detach_client(agent_client);
				free(agent_client);
				agent_client = NULL;
			}
			if (agent_device != NULL) {
				indigo_detach_device(agent_device);
				free(agent_device);
				agent_device = NULL;
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
