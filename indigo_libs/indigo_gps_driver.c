// Copyright (c) 2017 Rumen G.Bogdanovski.
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
// 2.0 by Rumen G.Bogdanovski <rumen@skyarchive.org>

/** INDIGO GPS Driver base
 \file indigo_gps_driver.c
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <assert.h>
#include <string.h>
#include <errno.h>
#include <time.h>
#include <math.h>
#include <fcntl.h>
#include <sys/stat.h>

#include "indigo_gps_driver.h"
#include "indigo_io.h"

indigo_result indigo_gps_attach(indigo_device *device, unsigned version) {
	assert(device != NULL);
	assert(device != NULL);
	if (GPS_CONTEXT == NULL) {
		device->device_context = malloc(sizeof(indigo_gps_context));
		assert(device->device_context);
		memset(device->device_context, 0, sizeof(indigo_gps_context));
	}
	if (GPS_CONTEXT != NULL) {
		if (indigo_device_attach(device, version, INDIGO_INTERFACE_GPS) == INDIGO_OK) {
			// -------------------------------------------------------------------------------- GPS_HAVE_FIX
			GPS_STATUS_PROPERTY = indigo_init_light_property(NULL, device->name, GPS_STATUS_PROPERTY_NAME, GPS_SITE_GROUP, "Status", INDIGO_IDLE_STATE, 3);
			if (GPS_STATUS_PROPERTY == NULL)
				return INDIGO_FAILED;
			indigo_init_light_item(GPS_STATUS_NO_FIX_ITEM, GPS_STATUS_NO_FIX_ITEM_NAME, "No Fix", INDIGO_IDLE_STATE);
			indigo_init_light_item(GPS_STATUS_2D_FIX_ITEM, GPS_STATUS_2D_FIX_ITEM_NAME, "2D Fix", INDIGO_IDLE_STATE);
			indigo_init_light_item(GPS_STATUS_3D_FIX_ITEM, GPS_STATUS_3D_FIX_ITEM_NAME, "3D Fix", INDIGO_IDLE_STATE);
			// -------------------------------------------------------------------------------- GPS_GEOGRAPHIC_COORDINATES
			GPS_GEOGRAPHIC_COORDINATES_PROPERTY = indigo_init_number_property(NULL, device->name, GPS_GEOGRAPHIC_COORDINATES_PROPERTY_NAME, GPS_SITE_GROUP, "Location", INDIGO_IDLE_STATE, INDIGO_RO_PERM, 4);
			if (GPS_GEOGRAPHIC_COORDINATES_PROPERTY == NULL)
				return INDIGO_FAILED;
			indigo_init_number_item(GPS_GEOGRAPHIC_COORDINATES_LATITUDE_ITEM, GPS_GEOGRAPHIC_COORDINATES_LATITUDE_ITEM_NAME, "Latitude (-S / +N)", -90, 90, 0, 0);
			indigo_init_number_item(GPS_GEOGRAPHIC_COORDINATES_LONGITUDE_ITEM, GPS_GEOGRAPHIC_COORDINATES_LONGITUDE_ITEM_NAME, "Longitude (-W / +E)", -180, 360, 0, 0);
			indigo_init_number_item(GPS_GEOGRAPHIC_COORDINATES_ELEVATION_ITEM, GPS_GEOGRAPHIC_COORDINATES_ELEVATION_ITEM_NAME, "Elevation (m)", 0, 9000, 0, 0);
			indigo_init_number_item(GPS_GEOGRAPHIC_COORDINATES_ACCURACY_ITEM, GPS_GEOGRAPHIC_COORDINATES_ACCURACY_ITEM_NAME, "Position accuracy (+/-m)", 0, 200, 0, 0);
			// -------------------------------------------------------------------------------- GPS_UTC_TIME
			GPS_UTC_TIME_PROPERTY = indigo_init_text_property(NULL, device->name, GPS_UTC_TIME_PROPERTY_NAME, GPS_SITE_GROUP, "UTC time", INDIGO_IDLE_STATE, INDIGO_RO_PERM, 2);
			if (GPS_UTC_TIME_PROPERTY == NULL)
				return INDIGO_FAILED;
			GPS_UTC_TIME_PROPERTY->hidden = true;
			indigo_init_text_item(GPS_UTC_ITEM, GPS_UTC_TIME_ITEM_NAME, "UTC Time", "0000-00-00T00:00:00");
			indigo_init_text_item(GPS_UTC_OFFEST_ITEM, GPS_UTC_OFFSET_ITEM_NAME, "UTC Offset", "0"); /* step is 0.5 as there are timezones at 30 min */

			return INDIGO_OK;
		}
	}
	return INDIGO_FAILED;
}

indigo_result indigo_gps_enumerate_properties(indigo_device *device, indigo_client *client, indigo_property *property) {
	assert(device != NULL);
	assert(DEVICE_CONTEXT != NULL);
	indigo_result result = INDIGO_OK;
	if ((result = indigo_device_enumerate_properties(device, client, property)) == INDIGO_OK) {
		if (IS_CONNECTED) {
			if (indigo_property_match(GPS_GEOGRAPHIC_COORDINATES_PROPERTY, property))
				indigo_define_property(device, GPS_GEOGRAPHIC_COORDINATES_PROPERTY, NULL);
			if (indigo_property_match(GPS_UTC_TIME_PROPERTY, property))
				indigo_define_property(device, GPS_UTC_TIME_PROPERTY, NULL);
			if (indigo_property_match(GPS_STATUS_PROPERTY, property))
				indigo_define_property(device, GPS_STATUS_PROPERTY, NULL);
		}
	}
	return result;
}

indigo_result indigo_gps_change_property(indigo_device *device, indigo_client *client, indigo_property *property) {
	assert(device != NULL);
	assert(DEVICE_CONTEXT != NULL);
	assert(property != NULL);
	if (indigo_property_match(CONNECTION_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- CONNECTION
		if (IS_CONNECTED) {
			indigo_define_property(device, GPS_GEOGRAPHIC_COORDINATES_PROPERTY, NULL);
			indigo_define_property(device, GPS_UTC_TIME_PROPERTY, NULL);
			indigo_define_property(device, GPS_STATUS_PROPERTY, NULL);
		} else {
			indigo_delete_property(device, GPS_GEOGRAPHIC_COORDINATES_PROPERTY, NULL);
			indigo_delete_property(device, GPS_UTC_TIME_PROPERTY, NULL);
			indigo_delete_property(device, GPS_STATUS_PROPERTY, NULL);
		}
	} else if (indigo_property_match(GPS_GEOGRAPHIC_COORDINATES_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- GPS_GEOGRAPHIC_COORDINATES
		indigo_property_copy_values(GPS_GEOGRAPHIC_COORDINATES_PROPERTY, property, false);
		if (GPS_GEOGRAPHIC_COORDINATES_LONGITUDE_ITEM->number.value < 0)
			GPS_GEOGRAPHIC_COORDINATES_LONGITUDE_ITEM->number.value += 360;
		GPS_GEOGRAPHIC_COORDINATES_PROPERTY->state = INDIGO_OK_STATE;
		indigo_update_property(device, GPS_GEOGRAPHIC_COORDINATES_PROPERTY, NULL);
		return INDIGO_OK;
	} else if (indigo_property_match(CONFIG_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- CONFIG
		if (indigo_switch_match(CONFIG_SAVE_ITEM, property)) {
			indigo_save_property(device, NULL, GPS_GEOGRAPHIC_COORDINATES_PROPERTY);
		}
	}
	return indigo_device_change_property(device, client, property);
}

indigo_result indigo_gps_detach(indigo_device *device) {
	assert(device != NULL);
	indigo_release_property(GPS_GEOGRAPHIC_COORDINATES_PROPERTY);
	indigo_release_property(GPS_UTC_TIME_PROPERTY);
	indigo_release_property(GPS_STATUS_PROPERTY);
	return indigo_device_detach(device);
}
