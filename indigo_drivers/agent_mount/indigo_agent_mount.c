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

#define DRIVER_VERSION 0x0001
#define DRIVER_NAME	"indigo_agent_mount"

#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <math.h>
#include <assert.h>
#include <pthread.h>

#include "indigo_driver_xml.h"
#include "indigo_filter.h"
#include "indigo_mount_driver.h"
#include "indigo_agent_mount.h"

#define DEVICE_PRIVATE_DATA												((agent_private_data *)device->private_data)
#define CLIENT_PRIVATE_DATA												((agent_private_data *)FILTER_CLIENT_CONTEXT->device->private_data)

#define AGENT_GEOGRAPHIC_COORDINATES_PROPERTY					(DEVICE_PRIVATE_DATA->agent_geographic_property)
#define AGENT_GEOGRAPHIC_COORDINATES_LATITUDE_ITEM  	(AGENT_GEOGRAPHIC_COORDINATES_PROPERTY->items+0)
#define AGENT_GEOGRAPHIC_COORDINATES_LONGITUDE_ITEM  	(AGENT_GEOGRAPHIC_COORDINATES_PROPERTY->items+1)
#define AGENT_GEOGRAPHIC_COORDINATES_ELEVATION_ITEM  	(AGENT_GEOGRAPHIC_COORDINATES_PROPERTY->items+2)

#define AGENT_SITE_DATA_SOURCE_PROPERTY								(DEVICE_PRIVATE_DATA->agent_site_data_source_property)
#define AGENT_SITE_DATA_SOURCE_HOST_ITEM  						(AGENT_SITE_DATA_SOURCE_PROPERTY->items+0)
#define AGENT_SITE_DATA_SOURCE_MOUNT_ITEM  						(AGENT_SITE_DATA_SOURCE_PROPERTY->items+1)
#define AGENT_SITE_DATA_SOURCE_DOME_ITEM  						(AGENT_SITE_DATA_SOURCE_PROPERTY->items+2)
#define AGENT_SITE_DATA_SOURCE_GPS_ITEM  							(AGENT_SITE_DATA_SOURCE_PROPERTY->items+3)

typedef struct {
	indigo_property *agent_geographic_property;
	indigo_property *agent_site_data_source_property;
	double mount_latitude, mount_longitude, mount_elevation;
	double dome_latitude, dome_longitude, dome_elevation;
	double gps_latitude, gps_longitude, gps_elevation;
	bool do_sync;
} agent_private_data;

static void set_site_coordinates2(indigo_device *device, int index, double latitude, double longitude, double elevation) {
	if (*FILTER_DEVICE_CONTEXT->device_name[index]) {
		indigo_property *property = indigo_init_number_property(NULL, FILTER_DEVICE_CONTEXT->device_name[index], GEOGRAPHIC_COORDINATES_PROPERTY_NAME, NULL, NULL, INDIGO_OK_STATE, INDIGO_RW_PERM, 3);
		indigo_init_number_item(property->items + 0, GEOGRAPHIC_COORDINATES_LATITUDE_ITEM_NAME, NULL, 0, 0, 0, latitude);
		indigo_init_number_item(property->items + 1, GEOGRAPHIC_COORDINATES_LONGITUDE_ITEM_NAME, NULL, 0, 0, 0, longitude);
		indigo_init_number_item(property->items + 2, GEOGRAPHIC_COORDINATES_ELEVATION_ITEM_NAME, NULL, 0, 0, 0, elevation);
		indigo_change_property(FILTER_DEVICE_CONTEXT->client, property);
		indigo_release_property(property);
	}
}

static void set_site_coordinates(indigo_device *device) {
	if (AGENT_SITE_DATA_SOURCE_HOST_ITEM->sw.value) {
		double latitude = AGENT_GEOGRAPHIC_COORDINATES_LATITUDE_ITEM->number.target;
		double longitude = AGENT_GEOGRAPHIC_COORDINATES_LONGITUDE_ITEM->number.target;
		double elevation = AGENT_GEOGRAPHIC_COORDINATES_ELEVATION_ITEM->number.target;
		AGENT_GEOGRAPHIC_COORDINATES_LATITUDE_ITEM->number.value = latitude;
		AGENT_GEOGRAPHIC_COORDINATES_LONGITUDE_ITEM->number.value = longitude;
		AGENT_GEOGRAPHIC_COORDINATES_ELEVATION_ITEM->number.value = elevation;
		AGENT_GEOGRAPHIC_COORDINATES_PROPERTY->state = INDIGO_OK_STATE;
		indigo_update_property(device, AGENT_GEOGRAPHIC_COORDINATES_PROPERTY, NULL);
		set_site_coordinates2(device, INDIGO_FILTER_MOUNT_INDEX, latitude, longitude, elevation);
		set_site_coordinates2(device, INDIGO_FILTER_DOME_INDEX, latitude, longitude, elevation);
		return;
	}
	if (AGENT_SITE_DATA_SOURCE_MOUNT_ITEM->sw.value) {
		double latitude = DEVICE_PRIVATE_DATA->mount_latitude;
		double longitude = DEVICE_PRIVATE_DATA->mount_longitude;
		double elevation = DEVICE_PRIVATE_DATA->mount_elevation;
		AGENT_GEOGRAPHIC_COORDINATES_LATITUDE_ITEM->number.value = latitude;
		AGENT_GEOGRAPHIC_COORDINATES_LONGITUDE_ITEM->number.value = longitude;
		AGENT_GEOGRAPHIC_COORDINATES_ELEVATION_ITEM->number.value = elevation;
		AGENT_GEOGRAPHIC_COORDINATES_PROPERTY->state = INDIGO_OK_STATE;
		indigo_update_property(device, AGENT_GEOGRAPHIC_COORDINATES_PROPERTY, NULL);
		set_site_coordinates2(device, INDIGO_FILTER_DOME_INDEX, latitude, longitude, elevation);
		return;
	}
	if (AGENT_SITE_DATA_SOURCE_DOME_ITEM->sw.value) {
		double latitude = DEVICE_PRIVATE_DATA->dome_latitude;
		double longitude = DEVICE_PRIVATE_DATA->dome_longitude;
		double elevation = DEVICE_PRIVATE_DATA->dome_elevation;
		AGENT_GEOGRAPHIC_COORDINATES_LATITUDE_ITEM->number.value = latitude;
		AGENT_GEOGRAPHIC_COORDINATES_LONGITUDE_ITEM->number.value = longitude;
		AGENT_GEOGRAPHIC_COORDINATES_ELEVATION_ITEM->number.value = elevation;
		AGENT_GEOGRAPHIC_COORDINATES_PROPERTY->state = INDIGO_OK_STATE;
		indigo_update_property(device, AGENT_GEOGRAPHIC_COORDINATES_PROPERTY, NULL);
		set_site_coordinates2(device, INDIGO_FILTER_MOUNT_INDEX, latitude, longitude, elevation);
		return;
	}
	if (AGENT_SITE_DATA_SOURCE_GPS_ITEM->sw.value) {
		double latitude = DEVICE_PRIVATE_DATA->gps_latitude;
		double longitude = DEVICE_PRIVATE_DATA->gps_longitude;
		double elevation = DEVICE_PRIVATE_DATA->gps_elevation;
		AGENT_GEOGRAPHIC_COORDINATES_LATITUDE_ITEM->number.value = latitude;
		AGENT_GEOGRAPHIC_COORDINATES_LONGITUDE_ITEM->number.value = longitude;
		AGENT_GEOGRAPHIC_COORDINATES_ELEVATION_ITEM->number.value = elevation;
		AGENT_GEOGRAPHIC_COORDINATES_PROPERTY->state = INDIGO_OK_STATE;
		indigo_update_property(device, AGENT_GEOGRAPHIC_COORDINATES_PROPERTY, NULL);
		set_site_coordinates2(device, INDIGO_FILTER_MOUNT_INDEX, latitude, longitude, elevation);
		set_site_coordinates2(device, INDIGO_FILTER_DOME_INDEX, latitude, longitude, elevation);
		return;
	}
}

// -------------------------------------------------------------------------------- INDIGO agent device implementation

static indigo_result agent_enumerate_properties(indigo_device *device, indigo_client *client, indigo_property *property);

static indigo_result agent_device_attach(indigo_device *device) {
	assert(device != NULL);
	assert(DEVICE_PRIVATE_DATA != NULL);
	if (indigo_filter_device_attach(device, DRIVER_VERSION, INDIGO_INTERFACE_CCD) == INDIGO_OK) {
		// -------------------------------------------------------------------------------- Device properties
		FILTER_MOUNT_LIST_PROPERTY->hidden = false;
		FILTER_DOME_LIST_PROPERTY->hidden = false;
		FILTER_GPS_LIST_PROPERTY->hidden = false;
		// -------------------------------------------------------------------------------- AGENT_GEOGRAPHIC_COORDINATES
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
		indigo_init_switch_item(AGENT_SITE_DATA_SOURCE_HOST_ITEM, AGENT_SITE_DATA_SOURCE_HOST_ITEM_NAME, "Host", true);
		indigo_init_switch_item(AGENT_SITE_DATA_SOURCE_MOUNT_ITEM, AGENT_SITE_DATA_SOURCE_MOUNT_ITEM_NAME, "Mount", false);
		indigo_init_switch_item(AGENT_SITE_DATA_SOURCE_DOME_ITEM, AGENT_SITE_DATA_SOURCE_DOME_ITEM_NAME, "Dome", false);
		indigo_init_switch_item(AGENT_SITE_DATA_SOURCE_GPS_ITEM, AGENT_SITE_DATA_SOURCE_GPS_ITEM_NAME, "GPS", false);
		// --------------------------------------------------------------------------------
		CONNECTION_PROPERTY->hidden = true;
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
	return indigo_filter_enumerate_properties(device, client, property);
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
		indigo_update_property(device, AGENT_SITE_DATA_SOURCE_PROPERTY, NULL);
		return INDIGO_OK;
	} else if (indigo_property_match(AGENT_GEOGRAPHIC_COORDINATES_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- AGENT_GEOGRAPHIC_COORDINATES
		indigo_property_copy_values(AGENT_GEOGRAPHIC_COORDINATES_PROPERTY, property, false);
		set_site_coordinates(device);
		AGENT_GEOGRAPHIC_COORDINATES_PROPERTY->state = INDIGO_OK_STATE;
		indigo_update_property(device, AGENT_GEOGRAPHIC_COORDINATES_PROPERTY, NULL);
		return INDIGO_OK;
	} else if (indigo_property_match(CONFIG_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- CONFIG
		if (indigo_switch_match(CONFIG_SAVE_ITEM, property)) {
			indigo_save_property(device, NULL, AGENT_SITE_DATA_SOURCE_PROPERTY);
		}
	}
	return indigo_filter_change_property(device, client, property);
}

static indigo_result agent_device_detach(indigo_device *device) {
	assert(device != NULL);
	indigo_release_property(AGENT_GEOGRAPHIC_COORDINATES_PROPERTY);
	indigo_release_property(AGENT_SITE_DATA_SOURCE_PROPERTY);
	return indigo_filter_device_detach(device);
}

// -------------------------------------------------------------------------------- INDIGO agent client implementation

static void process_snooping(indigo_client *client, indigo_device *device, indigo_property *property) {
	if (*FILTER_CLIENT_CONTEXT->device_name[INDIGO_FILTER_MOUNT_INDEX] && !strcmp(property->device, FILTER_CLIENT_CONTEXT->device_name[INDIGO_FILTER_MOUNT_INDEX])) {
		if (!strcmp(property->name, GEOGRAPHIC_COORDINATES_PROPERTY_NAME) && property->state == INDIGO_OK_STATE) {
			for (int i = 0; i < property->count; i++) {
				if (!strcmp(property->items[i].name, GEOGRAPHIC_COORDINATES_LATITUDE_ITEM_NAME))
				CLIENT_PRIVATE_DATA->mount_latitude = property->items[i].number.value;
				else if (!strcmp(property->items[i].name, GEOGRAPHIC_COORDINATES_LONGITUDE_ITEM_NAME))
				CLIENT_PRIVATE_DATA->mount_longitude = property->items[i].number.value;
				else if (!strcmp(property->items[i].name, GEOGRAPHIC_COORDINATES_ELEVATION_ITEM_NAME))
				CLIENT_PRIVATE_DATA->mount_elevation = property->items[i].number.value;
			}
			if (CLIENT_PRIVATE_DATA->agent_site_data_source_property->items[1].sw.value)
			set_site_coordinates(FILTER_CLIENT_CONTEXT->device);
		} else if (!strcmp(property->name, MOUNT_EQUATORIAL_COORDINATES_PROPERTY_NAME) && property->state != INDIGO_ALERT_STATE) {
			if (CLIENT_PRIVATE_DATA->do_sync && *FILTER_CLIENT_CONTEXT->device_name[INDIGO_FILTER_DOME_INDEX]) {
				indigo_property *eq_property = indigo_init_number_property(NULL, FILTER_CLIENT_CONTEXT->device_name[INDIGO_FILTER_DOME_INDEX], DOME_EQUATORIAL_COORDINATES_PROPERTY_NAME, NULL, NULL, INDIGO_OK_STATE, INDIGO_RW_PERM, 2);
				for (int i = 0; i < property->count; i++) {
					if (!strcmp(property->items[i].name, MOUNT_EQUATORIAL_COORDINATES_RA_ITEM_NAME))
					indigo_init_number_item(eq_property->items + 0, MOUNT_EQUATORIAL_COORDINATES_RA_ITEM_NAME, NULL, 0, 0, 0,  property->items[i].number.value);
					else if (!strcmp(property->items[i].name, MOUNT_EQUATORIAL_COORDINATES_DEC_ITEM_NAME))
					indigo_init_number_item(eq_property->items + 1, MOUNT_EQUATORIAL_COORDINATES_DEC_ITEM_NAME, NULL, 0, 0, 0,  property->items[i].number.value);
				}
				indigo_change_property(FILTER_CLIENT_CONTEXT->client, eq_property);
				indigo_release_property(eq_property);
			}
		}
	}
	if (*FILTER_CLIENT_CONTEXT->device_name[INDIGO_FILTER_DOME_INDEX] && !strcmp(property->device, FILTER_CLIENT_CONTEXT->device_name[INDIGO_FILTER_DOME_INDEX])) {
		if (!strcmp(property->name, GEOGRAPHIC_COORDINATES_PROPERTY_NAME) && property->state == INDIGO_OK_STATE) {
			for (int i = 0; i < property->count; i++) {
				if (!strcmp(property->items[i].name, GEOGRAPHIC_COORDINATES_LATITUDE_ITEM_NAME))
				CLIENT_PRIVATE_DATA->dome_latitude = property->items[i].number.value;
				else if (!strcmp(property->items[i].name, GEOGRAPHIC_COORDINATES_LONGITUDE_ITEM_NAME))
				CLIENT_PRIVATE_DATA->dome_longitude = property->items[i].number.value;
				else if (!strcmp(property->items[i].name, GEOGRAPHIC_COORDINATES_ELEVATION_ITEM_NAME))
				CLIENT_PRIVATE_DATA->dome_elevation = property->items[i].number.value;
			}
			if (CLIENT_PRIVATE_DATA->agent_site_data_source_property->items[2].sw.value)
			set_site_coordinates(FILTER_CLIENT_CONTEXT->device);
		} else if (!strcmp(property->name, DOME_AUTO_SYNC_PROPERTY_NAME) && property->state == INDIGO_OK_STATE) {
			for (int i = 0; i < property->count; i++) {
				if (!strcmp(property->items[i].name, DOME_AUTO_SYNC_ENABLE_ITEM_NAME))
				CLIENT_PRIVATE_DATA->do_sync = property->items[i].sw.value;
			}
		}
	}
	if (*FILTER_CLIENT_CONTEXT->device_name[INDIGO_FILTER_GPS_INDEX] && !strcmp(property->device, FILTER_CLIENT_CONTEXT->device_name[INDIGO_FILTER_GPS_INDEX])) {
		if (!strcmp(property->name, GEOGRAPHIC_COORDINATES_PROPERTY_NAME) && property->state == INDIGO_OK_STATE) {
			for (int i = 0; i < property->count; i++) {
				if (!strcmp(property->items[i].name, GEOGRAPHIC_COORDINATES_LATITUDE_ITEM_NAME))
				CLIENT_PRIVATE_DATA->gps_latitude = property->items[i].number.value;
				else if (!strcmp(property->items[i].name, GEOGRAPHIC_COORDINATES_LONGITUDE_ITEM_NAME))
				CLIENT_PRIVATE_DATA->gps_longitude = property->items[i].number.value;
				else if (!strcmp(property->items[i].name, GEOGRAPHIC_COORDINATES_ELEVATION_ITEM_NAME))
				CLIENT_PRIVATE_DATA->gps_elevation = property->items[i].number.value;
			}
			if (CLIENT_PRIVATE_DATA->agent_site_data_source_property->items[3].sw.value)
			set_site_coordinates(FILTER_CLIENT_CONTEXT->device);
		}
	}
}

static indigo_result agent_define_property(indigo_client *client, indigo_device *device, indigo_property *property, const char *message) {
	process_snooping(client, device, property);
	return indigo_filter_define_property(client, device, property, message);
}

static indigo_result agent_update_property(indigo_client *client, indigo_device *device, indigo_property *property, const char *message) {
	process_snooping(client, device, property);
	return indigo_filter_update_property(client, device, property, message);
}

static indigo_result agent_delete_property(indigo_client *client, indigo_device *device, indigo_property *property, const char *message) {
	if (*FILTER_CLIENT_CONTEXT->device_name[INDIGO_FILTER_GPS_INDEX] && !strcmp(property->device, FILTER_CLIENT_CONTEXT->device_name[INDIGO_FILTER_GPS_INDEX])) {
		if (!strcmp(property->name, DOME_AUTO_SYNC_PROPERTY_NAME) && property->state == INDIGO_OK_STATE) {
			CLIENT_PRIVATE_DATA->do_sync = false;
		}
	}
	return indigo_filter_delete_property(client, device, property, message);
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
		agent_delete_property,
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
			private_data = malloc(sizeof(agent_private_data));
			assert(private_data != NULL);
			memset(private_data, 0, sizeof(agent_private_data));
			agent_device = malloc(sizeof(indigo_device));
			assert(agent_device != NULL);
			memcpy(agent_device, &agent_device_template, sizeof(indigo_device));
			agent_device->private_data = private_data;
			indigo_attach_device(agent_device);

			agent_client = malloc(sizeof(indigo_client));
			assert(agent_client != NULL);
			memcpy(agent_client, &agent_client_template, sizeof(indigo_client));
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
