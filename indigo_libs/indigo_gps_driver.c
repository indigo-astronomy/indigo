// Copyright (c) 2017-2025 Rumen G. Bogdanovski.
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
// 2.0 by Rumen G. Bogdanovski <rumenastro@gmail.com>

/** INDIGO GPS Driver base
 \file indigo_gps_driver.c
 */

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <errno.h>
#include <time.h>
#include <math.h>
#include <sys/stat.h>

#include <indigo/indigo_gps_driver.h>
#include <indigo/indigo_io.h>

indigo_result indigo_gps_attach(indigo_device *device, const char* driver_name, unsigned version) {
	assert(device != NULL);
	assert(device != NULL);
	if (GPS_CONTEXT == NULL) {
		device->device_context = indigo_safe_malloc(sizeof(indigo_gps_context));
	}
	if (GPS_CONTEXT != NULL) {
		if (indigo_device_attach(device, driver_name, version, INDIGO_INTERFACE_GPS) == INDIGO_OK) {
			// -------------------------------------------------------------------------------- GPS_STATUS
			GPS_STATUS_PROPERTY = indigo_init_light_property(NULL, device->name, GPS_STATUS_PROPERTY_NAME, GPS_SITE_GROUP, "Status", INDIGO_OK_STATE, 3);
			if (GPS_STATUS_PROPERTY == NULL) {
				return INDIGO_FAILED;
			}
			indigo_init_light_item(GPS_STATUS_NO_FIX_ITEM, GPS_STATUS_NO_FIX_ITEM_NAME, "No Fix", INDIGO_IDLE_STATE);
			indigo_init_light_item(GPS_STATUS_2D_FIX_ITEM, GPS_STATUS_2D_FIX_ITEM_NAME, "2D Fix", INDIGO_IDLE_STATE);
			indigo_init_light_item(GPS_STATUS_3D_FIX_ITEM, GPS_STATUS_3D_FIX_ITEM_NAME, "3D Fix", INDIGO_IDLE_STATE);
			// -------------------------------------------------------------------------------- GPS_GEOGRAPHIC_COORDINATES
			GPS_GEOGRAPHIC_COORDINATES_PROPERTY = indigo_init_number_property(NULL, device->name, GEOGRAPHIC_COORDINATES_PROPERTY_NAME, GPS_SITE_GROUP, "Location", INDIGO_OK_STATE, INDIGO_RO_PERM, 4);
			if (GPS_GEOGRAPHIC_COORDINATES_PROPERTY == NULL) {
				return INDIGO_FAILED;
			}
			indigo_init_sexagesimal_number_item(GPS_GEOGRAPHIC_COORDINATES_LATITUDE_ITEM, GEOGRAPHIC_COORDINATES_LATITUDE_ITEM_NAME, "Latitude (-S / +N)", -90, 90, 0, 0);
			indigo_init_sexagesimal_number_item(GPS_GEOGRAPHIC_COORDINATES_LONGITUDE_ITEM, GEOGRAPHIC_COORDINATES_LONGITUDE_ITEM_NAME, "Longitude (-W / +E)", -180, 360, 0, 0);
			indigo_init_number_item(GPS_GEOGRAPHIC_COORDINATES_ELEVATION_ITEM, GEOGRAPHIC_COORDINATES_ELEVATION_ITEM_NAME, "Elevation (m)", 0, 9000, 0, 0);
			indigo_init_number_item(GPS_GEOGRAPHIC_COORDINATES_ACCURACY_ITEM, GEOGRAPHIC_COORDINATES_ACCURACY_ITEM_NAME, "Position accuracy (+/-m)", 0, 200, 0, 0);
			// -------------------------------------------------------------------------------- GPS_UTC_TIME
			GPS_UTC_TIME_PROPERTY = indigo_init_text_property(NULL, device->name, UTC_TIME_PROPERTY_NAME, GPS_SITE_GROUP, "UTC time", INDIGO_OK_STATE, INDIGO_RO_PERM, 2);
			if (GPS_UTC_TIME_PROPERTY == NULL) {
				return INDIGO_FAILED;
			}
			GPS_UTC_TIME_PROPERTY->hidden = true;
			indigo_init_text_item(GPS_UTC_ITEM, UTC_TIME_ITEM_NAME, "UTC Time", "0000-00-00T00:00:00");
			indigo_init_text_item(GPS_UTC_OFFEST_ITEM, UTC_OFFSET_ITEM_NAME, "UTC Offset", "0"); /* step is 0.5 as there are timezones at 30 min */
			// -------------------------------------------------------------------------------- GPS_ADVANCED
			GPS_ADVANCED_PROPERTY = indigo_init_switch_property(NULL, device->name, GPS_ADVANCED_PROPERTY_NAME, GPS_ADVANCED_GROUP, "Show advanced status", INDIGO_OK_STATE, INDIGO_RW_PERM, INDIGO_ONE_OF_MANY_RULE, 2);
			if (GPS_ADVANCED_PROPERTY == NULL) {
				return INDIGO_FAILED;
			}
			GPS_ADVANCED_PROPERTY->hidden = true;
			indigo_init_switch_item(GPS_ADVANCED_ENABLED_ITEM, GPS_ADVANCED_ENABLED_ITEM_NAME, "Enable", false);
			indigo_init_switch_item(GPS_ADVANCED_DISABLED_ITEM, GPS_ADVANCED_DISABLED_ITEM_NAME, "Disable", true);
			// -------------------------------------------------------------------------------- GPS_ADVANCED_STATUS
			GPS_ADVANCED_STATUS_PROPERTY = indigo_init_number_property(NULL, device->name, GPS_ADVANCED_STATUS_PROPERTY_MANE, GPS_ADVANCED_GROUP, "Advanced status", INDIGO_OK_STATE, INDIGO_RO_PERM, 5);
			if (GPS_ADVANCED_STATUS_PROPERTY == NULL) {
				return INDIGO_FAILED;
			}
			GPS_ADVANCED_STATUS_PROPERTY->hidden = true;
			indigo_init_number_item(GPS_ADVANCED_STATUS_SVS_IN_USE_ITEM, GPS_ADVANCED_STATUS_SVS_IN_USE_ITEM_NAME, "SVs in use", 0, 36, 0, 0);
			indigo_init_number_item(GPS_ADVANCED_STATUS_SVS_IN_VIEW_ITEM, GPS_ADVANCED_STATUS_SVS_IN_VIEW_ITEM_NAME, "SVs in view", 0, 36, 0, 0);
			indigo_init_number_item(GPS_ADVANCED_STATUS_PDOP_ITEM, GPS_ADVANCED_STATUS_PDOP_ITEM_NAME, "Position DOP", 0, 200, 0, 0);
			indigo_init_number_item(GPS_ADVANCED_STATUS_HDOP_ITEM, GPS_ADVANCED_STATUS_HDOP_ITEM_NAME, "Horizontal DOP ", 0, 200, 0, 0);
			indigo_init_number_item(GPS_ADVANCED_STATUS_VDOP_ITEM, GPS_ADVANCED_STATUS_VDOP_ITEM_NAME, "Vertical DOP", 0, 200, 0, 0);

			return INDIGO_OK;
		}
	}
	return INDIGO_FAILED;
}

indigo_result indigo_gps_enumerate_properties(indigo_device *device, indigo_client *client, indigo_property *property) {
	assert(device != NULL);
	assert(DEVICE_CONTEXT != NULL);
	GPS_ADVANCED_STATUS_PROPERTY->hidden = GPS_ADVANCED_PROPERTY->hidden;
	if (IS_CONNECTED) {
		INDIGO_DEFINE_MATCHING_PROPERTY(GPS_GEOGRAPHIC_COORDINATES_PROPERTY);
		INDIGO_DEFINE_MATCHING_PROPERTY(GPS_UTC_TIME_PROPERTY);
		INDIGO_DEFINE_MATCHING_PROPERTY(GPS_STATUS_PROPERTY);
		INDIGO_DEFINE_MATCHING_PROPERTY(GPS_ADVANCED_PROPERTY);
		if (indigo_property_match(GPS_ADVANCED_STATUS_PROPERTY, property) && (GPS_ADVANCED_ENABLED_ITEM->sw.value))
			indigo_define_property(device, GPS_ADVANCED_STATUS_PROPERTY, NULL);
	}
	return indigo_device_enumerate_properties(device, client, property);
}

indigo_result indigo_gps_change_property(indigo_device *device, indigo_client *client, indigo_property *property) {
	assert(device != NULL);
	assert(DEVICE_CONTEXT != NULL);
	assert(property != NULL);
	if (indigo_property_match_changeable(CONNECTION_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- CONNECTION
		if (IS_CONNECTED) {
			indigo_define_property(device, GPS_GEOGRAPHIC_COORDINATES_PROPERTY, NULL);
			indigo_define_property(device, GPS_UTC_TIME_PROPERTY, NULL);
			indigo_define_property(device, GPS_STATUS_PROPERTY, NULL);
			indigo_define_property(device, GPS_ADVANCED_PROPERTY, NULL);
			if (GPS_ADVANCED_ENABLED_ITEM->sw.value) {
				indigo_define_property(device, GPS_ADVANCED_STATUS_PROPERTY, NULL);
			}
		} else {
			indigo_delete_property(device, GPS_GEOGRAPHIC_COORDINATES_PROPERTY, NULL);
			indigo_delete_property(device, GPS_UTC_TIME_PROPERTY, NULL);
			indigo_delete_property(device, GPS_STATUS_PROPERTY, NULL);
			indigo_delete_property(device, GPS_ADVANCED_PROPERTY, NULL);
			indigo_delete_property(device, GPS_ADVANCED_STATUS_PROPERTY, NULL);
			if (GPS_ADVANCED_ENABLED_ITEM->sw.value) {
				indigo_delete_property(device, GPS_ADVANCED_STATUS_PROPERTY, NULL);
			}
		}
	} else if (indigo_property_match_changeable(GPS_GEOGRAPHIC_COORDINATES_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- GPS_GEOGRAPHIC_COORDINATES
		indigo_property_copy_values(GPS_GEOGRAPHIC_COORDINATES_PROPERTY, property, false);
		if (GPS_GEOGRAPHIC_COORDINATES_LONGITUDE_ITEM->number.value < 0) {
			GPS_GEOGRAPHIC_COORDINATES_LONGITUDE_ITEM->number.value += 360;
		}
		GPS_GEOGRAPHIC_COORDINATES_PROPERTY->state = INDIGO_OK_STATE;
		indigo_update_property(device, GPS_GEOGRAPHIC_COORDINATES_PROPERTY, NULL);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(GPS_ADVANCED_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- GPS_ADVANCED
		indigo_property_copy_values(GPS_ADVANCED_PROPERTY, property, false);
		GPS_ADVANCED_PROPERTY->state = INDIGO_OK_STATE;
		if (GPS_ADVANCED_ENABLED_ITEM->sw.value) {
			indigo_define_property(device, GPS_ADVANCED_STATUS_PROPERTY, NULL);
		} else {
			indigo_delete_property(device, GPS_ADVANCED_STATUS_PROPERTY, NULL);
		}
		indigo_update_property(device, GPS_ADVANCED_PROPERTY, NULL);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(CONFIG_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- CONFIG
		if (indigo_switch_match(CONFIG_SAVE_ITEM, property)) {
			indigo_save_property(device, NULL, GPS_GEOGRAPHIC_COORDINATES_PROPERTY);
			indigo_save_property(device, NULL, GPS_ADVANCED_PROPERTY);
		}
	}
	return indigo_device_change_property(device, client, property);
}

indigo_result indigo_gps_detach(indigo_device *device) {
	assert(device != NULL);
	indigo_release_property(GPS_GEOGRAPHIC_COORDINATES_PROPERTY);
	indigo_release_property(GPS_UTC_TIME_PROPERTY);
	indigo_release_property(GPS_STATUS_PROPERTY);
	indigo_release_property(GPS_ADVANCED_PROPERTY);
	indigo_release_property(GPS_ADVANCED_STATUS_PROPERTY);
	return indigo_device_detach(device);
}
