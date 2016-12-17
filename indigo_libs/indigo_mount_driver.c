// Copyright (c) 2016 CloudMakers, s. r. o.
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
// 2.0 Build 0 - PoC by Peter Polakovic <peter.polakovic@cloudmakers.eu>

/** INDIGO Mount Driver base
 \file indigo_mount_driver.c
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

#include "indigo_mount_driver.h"

indigo_result indigo_mount_attach(indigo_device *device, unsigned version) {
	assert(device != NULL);
	assert(device != NULL);
	if (MOUNT_CONTEXT == NULL) {
		device->device_context = malloc(sizeof(indigo_mount_context));
		assert(device->device_context);
		memset(device->device_context, 0, sizeof(indigo_mount_context));
	}
	if (MOUNT_CONTEXT != NULL) {
		if (indigo_device_attach(device, version, INDIGO_INTERFACE_MOUNT) == INDIGO_OK) {
			// -------------------------------------------------------------------------------- MOUNT_INFO
			MOUNT_INFO_PROPERTY = indigo_init_text_property(NULL, device->name, MOUNT_INFO_PROPERTY_NAME, MOUNT_MAIN_GROUP, "Info", INDIGO_IDLE_STATE, INDIGO_RO_PERM, 3);
			if (MOUNT_INFO_PROPERTY == NULL)
				return INDIGO_FAILED;
			indigo_init_text_item(MOUNT_INFO_VENDOR_ITEM, MOUNT_INFO_VENDOR_ITEM_NAME, "Vendor", "Unkwnown");
			indigo_init_text_item(MOUNT_INFO_MODEL_ITEM, MOUNT_INFO_MODEL_ITEM_NAME, "Model", "Unkwnown");
			indigo_init_text_item(MOUNT_INFO_FIRMWARE_ITEM, MOUNT_INFO_FIRMWARE_ITEM_NAME, "Firmware", "N/A");
			// -------------------------------------------------------------------------------- MOUNT_GEOGRAPHIC_COORDINATES
			MOUNT_GEOGRAPHIC_COORDINATES_PROPERTY = indigo_init_number_property(NULL, device->name, MOUNT_GEOGRAPHIC_COORDINATES_PROPERTY_NAME, MOUNT_SITE_GROUP, "Location", INDIGO_IDLE_STATE, INDIGO_RW_PERM, 3);
			if (MOUNT_GEOGRAPHIC_COORDINATES_PROPERTY == NULL)
				return INDIGO_FAILED;
			indigo_init_number_item(MOUNT_GEOGRAPHIC_COORDINATES_LATITUDE_ITEM, MOUNT_GEOGRAPHIC_COORDINATES_LATITUDE_ITEM_NAME, "Latitude (-90 to +90° +N)", -90, 90, 0, 0);
			indigo_init_number_item(MOUNT_GEOGRAPHIC_COORDINATES_LONGITUDE_ITEM, MOUNT_GEOGRAPHIC_COORDINATES_LONGITUDE_ITEM_NAME, "Longitude (0 to 360° +E)", 0, 360, 0, 0);
			indigo_init_number_item(MOUNT_GEOGRAPHIC_COORDINATES_ELEVATION_ITEM, MOUNT_GEOGRAPHIC_COORDINATES_ELEVATION_ITEM_NAME, "Elevation (m)", 0, 8000, 0, 0);
			// -------------------------------------------------------------------------------- MOUNT_LST_TIME
			MOUNT_LST_TIME_PROPERTY = indigo_init_number_property(NULL, device->name, MOUNT_LST_TIME_PROPERTY_NAME, MOUNT_SITE_GROUP, "LST Time", INDIGO_IDLE_STATE, INDIGO_RW_PERM, 1);
			if (MOUNT_LST_TIME_PROPERTY == NULL)
				return INDIGO_FAILED;
			MOUNT_LST_TIME_PROPERTY->hidden = true;
			indigo_init_number_item(MOUNT_LST_TIME_ITEM, MOUNT_LST_TIME_ITEM_NAME, "LST Time", 0, 24, 0, 0);
			// -------------------------------------------------------------------------------- MOUNT_UTC_TIME
			MOUNT_UTC_TIME_PROPERTY = indigo_init_number_property(NULL, device->name, MOUNT_UTC_TIME_PROPERTY_NAME, MOUNT_SITE_GROUP, "UTC time", INDIGO_IDLE_STATE, INDIGO_RW_PERM, 2);
			if (MOUNT_UTC_TIME_PROPERTY == NULL)
				return INDIGO_FAILED;
			MOUNT_UTC_TIME_PROPERTY->hidden = true;
			/* to be fixed to proper time/date format */
			indigo_init_number_item(MOUNT_UTC_ITEM, MOUNT_UTC_TIME_ITEM_NAME, "UTC Time", 0, 0, 0, 1);
			indigo_init_number_item(MOUNT_UTC_OFFEST_ITEM, MOUNT_UTC_OFFSET_ITEM_NAME, "UTC Offset", -12, 12, 0.5, 0); /* step is 0.5 as there are timezones at 30 min */
			// -------------------------------------------------------------------------------- MOUNT_UTC_FROM_HOST
			MOUNT_UTC_FROM_HOST_PROPERTY = indigo_init_switch_property(NULL, device->name, MOUNT_UTC_FROM_HOST_PROPERTY_NAME, MOUNT_SITE_GROUP, "Set UTC", INDIGO_IDLE_STATE, INDIGO_RW_PERM, INDIGO_AT_MOST_ONE_RULE, 1);
			if (MOUNT_UTC_FROM_HOST_PROPERTY == NULL)
				return INDIGO_FAILED;
			MOUNT_UTC_FROM_HOST_PROPERTY->hidden = true;
            indigo_init_switch_item(MOUNT_SET_UTC_ITEM  , MOUNT_SET_UTC_ITEM_NAME, "From host", false);
			// -------------------------------------------------------------------------------- MOUNT_PARK
			MOUNT_PARK_PROPERTY = indigo_init_switch_property(NULL, device->name, MOUNT_PARK_PROPERTY_NAME, MOUNT_MAIN_GROUP, "Park", INDIGO_IDLE_STATE, INDIGO_RW_PERM, INDIGO_ONE_OF_MANY_RULE, 2);
			if (MOUNT_PARK_PROPERTY == NULL)
				return INDIGO_FAILED;
			indigo_init_switch_item(MOUNT_PARK_PARKED_ITEM, MOUNT_PARK_PARKED_ITEM_NAME, "Mount parked", true);
			indigo_init_switch_item(MOUNT_PARK_UNPARKED_ITEM, MOUNT_PARK_UNPARKED_ITEM_NAME, "Mount unparked", false);
			// -------------------------------------------------------------------------------- MOUNT_ON_COORDINATES_SET
			MOUNT_ON_COORDINATES_SET_PROPERTY = indigo_init_switch_property(NULL, device->name,MOUNT_ON_COORDINATES_SET_PROPERTY_NAME, MOUNT_MAIN_GROUP, "On coordinates set", INDIGO_IDLE_STATE, INDIGO_RW_PERM, INDIGO_ONE_OF_MANY_RULE, 3);
			if (MOUNT_ON_COORDINATES_SET_PROPERTY == NULL)
				return INDIGO_FAILED;
			indigo_init_switch_item(MOUNT_ON_COORDINATES_SET_TRACK_ITEM, MOUNT_ON_COORDINATES_SET_TRACK_ITEM_NAME, "Slew to target and track", true);
			indigo_init_switch_item(MOUNT_ON_COORDINATES_SET_SYNC_ITEM, MOUNT_ON_COORDINATES_SET_SYNC_ITEM_NAME, "Sync to target", false);
			indigo_init_switch_item(MOUNT_ON_COORDINATES_SET_SLEW_ITEM, MOUNT_ON_COORDINATES_SET_SLEW_ITEM_NAME, "Slew to target and stop", false);
			// -------------------------------------------------------------------------------- MOUNT_SLEW_RATE
			MOUNT_SLEW_RATE_PROPERTY = indigo_init_switch_property(NULL, device->name, MOUNT_SLEW_RATE_PROPERTY_NAME, MOUNT_MAIN_GROUP, "Slew rate", INDIGO_IDLE_STATE, INDIGO_RW_PERM, INDIGO_ONE_OF_MANY_RULE, 4);
			if (MOUNT_SLEW_RATE_PROPERTY == NULL)
				return INDIGO_FAILED;
			indigo_init_switch_item(MOUNT_SLEW_RATE_GUIDE_ITEM, MOUNT_SLEW_RATE_GUIDE_ITEM_NAME, "Guide rate", true);
			indigo_init_switch_item(MOUNT_SLEW_RATE_CENTERING_ITEM, MOUNT_SLEW_RATE_CENTERING_ITEM_NAME, "Centering rate", false);
			indigo_init_switch_item(MOUNT_SLEW_RATE_FIND_ITEM, MOUNT_SLEW_RATE_FIND_ITEM_NAME, "Find rate", false);
			indigo_init_switch_item(MOUNT_SLEW_RATE_MAX_ITEM, MOUNT_SLEW_RATE_MAX_ITEM_NAME, "Max rate", false);
			// -------------------------------------------------------------------------------- MOUNT_MOTION_NS
			MOUNT_MOTION_NS_PROPERTY = indigo_init_switch_property(NULL, device->name, MOUNT_MOTION_NS_PROPERTY_NAME, MOUNT_MAIN_GROUP, "Move N/S", INDIGO_IDLE_STATE, INDIGO_RW_PERM, INDIGO_AT_MOST_ONE_RULE, 2);
			if (MOUNT_MOTION_NS_PROPERTY == NULL)
				return INDIGO_FAILED;
			indigo_init_switch_item(MOUNT_MOTION_NORTH_ITEM, MOUNT_MOTION_NORTH_ITEM_NAME, "North", false);
			indigo_init_switch_item(MOUNT_MOTION_SOUTH_ITEM, MOUNT_MOTION_SOUTH_ITEM_NAME, "South", false);
			// -------------------------------------------------------------------------------- MOUNT_MOTION_WE
			MOUNT_MOTION_WE_PROPERTY = indigo_init_switch_property(NULL, device->name, MOUNT_MOTION_WE_PROPERTY_NAME, MOUNT_MAIN_GROUP, "Move W/E", INDIGO_IDLE_STATE, INDIGO_RW_PERM, INDIGO_AT_MOST_ONE_RULE, 2);
			if (MOUNT_MOTION_WE_PROPERTY == NULL)
				return INDIGO_FAILED;
			indigo_init_switch_item(MOUNT_MOTION_WEST_ITEM, MOUNT_MOTION_WEST_ITEM_NAME, "West", false);
			indigo_init_switch_item(MOUNT_MOTION_EAST_ITEM, MOUNT_MOTION_EAST_ITEM_NAME, "East", false);
			// -------------------------------------------------------------------------------- MOUNT_TRACK_RATE
			MOUNT_TRACK_RATE_PROPERTY = indigo_init_switch_property(NULL, device->name, MOUNT_TRACK_RATE_PROPERTY_NAME, MOUNT_MAIN_GROUP, "Track rate", INDIGO_IDLE_STATE, INDIGO_RW_PERM, INDIGO_ONE_OF_MANY_RULE, 3);
			if (MOUNT_TRACK_RATE_PROPERTY == NULL)
				return INDIGO_FAILED;
			indigo_init_switch_item(MOUNT_TRACK_RATE_SIDEREAL_ITEM, MOUNT_TRACK_RATE_SIDEREAL_ITEM_NAME, "Sidereal rate", true);
			indigo_init_switch_item(MOUNT_TRACK_RATE_SOLAR_ITEM, MOUNT_TRACK_RATE_SOLAR_ITEM_NAME, "Solar rate", false);
			indigo_init_switch_item(MOUNT_TRACK_RATE_LUNAR_ITEM, MOUNT_TRACK_RATE_LUNAR_ITEM_NAME, "Lunar rate", false);
			// -------------------------------------------------------------------------------- MOUNT_TRACKING
			MOUNT_TRACKING_PROPERTY = indigo_init_switch_property(NULL, device->name, MOUNT_TRACKING_PROPERTY_NAME, MOUNT_MAIN_GROUP, "Tracking", INDIGO_IDLE_STATE, INDIGO_RW_PERM, INDIGO_ONE_OF_MANY_RULE, 2);
			if (MOUNT_TRACKING_PROPERTY == NULL)
				return INDIGO_FAILED;
			indigo_init_switch_item(MOUNT_TRACKING_ON_ITEM, MOUNT_TRACKING_ON_ITEM_NAME, MOUNT_TRACKING_ON_ITEM_NAME, true);
			indigo_init_switch_item(MOUNT_TRACKING_OFF_ITEM, MOUNT_TRACKING_OFF_ITEM_NAME, MOUNT_TRACKING_OFF_ITEM_NAME , false);
			// -------------------------------------------------------------------------------- MOUNT_GUIDE_RATE
			MOUNT_GUIDE_RATE_PROPERTY = indigo_init_number_property(NULL, device->name, MOUNT_GUIDE_RATE_PROPERTY_NAME, MOUNT_MAIN_GROUP, "Guide rate", INDIGO_IDLE_STATE, INDIGO_RW_PERM, 2);
			if (MOUNT_GUIDE_RATE_PROPERTY == NULL)
				return INDIGO_FAILED;
			indigo_init_number_item(MOUNT_GUIDE_RATE_RA_ITEM, MOUNT_GUIDE_RATE_RA_ITEM_NAME, "RA (% of sidereal)", 0, 100, 0, 50);
			indigo_init_number_item(MOUNT_GUIDE_RATE_DEC_ITEM, MOUNT_GUIDE_RATE_DEC_ITEM_NAME, "Dec (% of sidereal)", 0, 100, 0, 50);
			// -------------------------------------------------------------------------------- MOUNT_EQUATORIAL_COORDINATES
			MOUNT_EQUATORIAL_COORDINATES_PROPERTY = indigo_init_number_property(NULL, device->name, MOUNT_EQUATORIAL_COORDINATES_PROPERTY_NAME, MOUNT_MAIN_GROUP, "Equatorial EOD coordinates", INDIGO_IDLE_STATE, INDIGO_RW_PERM, 2);
			if (MOUNT_EQUATORIAL_COORDINATES_PROPERTY == NULL)
				return INDIGO_FAILED;
			indigo_init_number_item(MOUNT_EQUATORIAL_COORDINATES_RA_ITEM, MOUNT_EQUATORIAL_COORDINATES_RA_ITEM_NAME, "Right ascension (0 to 24 hrs)", 0, 24, 0, 0);
			indigo_init_number_item(MOUNT_EQUATORIAL_COORDINATES_DEC_ITEM, MOUNT_EQUATORIAL_COORDINATES_DEC_ITEM_NAME, "Declination (-180 to 180°)", -180, 180, 0, 90);
			// -------------------------------------------------------------------------------- MOUNT_HORIZONTAL_COORDINATES
			MOUNT_HORIZONTAL_COORDINATES_PROPERTY = indigo_init_number_property(NULL, device->name, MOUNT_HORIZONTAL_COORDINATES_PROPERTY_NAME, MOUNT_MAIN_GROUP, "Horizontal coordinates", INDIGO_IDLE_STATE, INDIGO_RW_PERM, 2);
			if (MOUNT_HORIZONTAL_COORDINATES_PROPERTY == NULL)
				return INDIGO_FAILED;
			MOUNT_HORIZONTAL_COORDINATES_PROPERTY->hidden = true;
			indigo_init_number_item(MOUNT_HORIZONTAL_COORDINATES_ALT_ITEM, MOUNT_HORIZONTAL_COORDINATES_ALT_ITEM_NAME, "Altitude (0 to 90°)", 0, 90, 0, 0);
			indigo_init_number_item(MOUNT_HORIZONTAL_COORDINATES_AZ_ITEM, MOUNT_HORIZONTAL_COORDINATES_AZ_ITEM_NAME, "Azimuth (0 to 360°)", 0, 360, 0, 0);
			// -------------------------------------------------------------------------------- MOUNT_ABORT_MOTION
			MOUNT_ABORT_MOTION_PROPERTY = indigo_init_switch_property(NULL, device->name, MOUNT_ABORT_MOTION_PROPERTY_NAME, MOUNT_MAIN_GROUP, "Abort motion", INDIGO_IDLE_STATE, INDIGO_RW_PERM, INDIGO_ONE_OF_MANY_RULE, 1);
			if (MOUNT_ABORT_MOTION_PROPERTY == NULL)
				return INDIGO_FAILED;
			indigo_init_switch_item(MOUNT_ABORT_MOTION_ITEM, MOUNT_ABORT_MOTION_ITEM_NAME, "Abort motion", false);
			// --------------------------------------------------------------------------------
			return INDIGO_OK;
		}
	}
	return INDIGO_FAILED;
}

indigo_result indigo_mount_enumerate_properties(indigo_device *device, indigo_client *client, indigo_property *property) {
	assert(device != NULL);
	assert(device->device_context != NULL);
	indigo_result result = INDIGO_OK;
	if ((result = indigo_device_enumerate_properties(device, client, property)) == INDIGO_OK) {
		if (CONNECTION_CONNECTED_ITEM->sw.value) {
			if (indigo_property_match(MOUNT_INFO_PROPERTY, property))
				indigo_define_property(device, MOUNT_INFO_PROPERTY, NULL);
			if (indigo_property_match(MOUNT_GEOGRAPHIC_COORDINATES_PROPERTY, property))
				indigo_define_property(device, MOUNT_GEOGRAPHIC_COORDINATES_PROPERTY, NULL);
			if (indigo_property_match(MOUNT_LST_TIME_PROPERTY, property) && !MOUNT_LST_TIME_PROPERTY->hidden)
				indigo_define_property(device, MOUNT_LST_TIME_PROPERTY, NULL);
			if (indigo_property_match(MOUNT_UTC_TIME_PROPERTY, property) && !MOUNT_UTC_TIME_PROPERTY->hidden)
				indigo_define_property(device, MOUNT_UTC_TIME_PROPERTY, NULL);
			if (indigo_property_match(MOUNT_UTC_FROM_HOST_PROPERTY, property) && !MOUNT_UTC_FROM_HOST_PROPERTY->hidden)
				indigo_define_property(device, MOUNT_UTC_FROM_HOST_PROPERTY, NULL);
			if (indigo_property_match(MOUNT_PARK_PROPERTY, property))
				indigo_define_property(device, MOUNT_PARK_PROPERTY, NULL);
			if (indigo_property_match(MOUNT_SLEW_RATE_PROPERTY, property))
				indigo_define_property(device, MOUNT_SLEW_RATE_PROPERTY, NULL);
			if (indigo_property_match(MOUNT_MOTION_NS_PROPERTY, property))
				indigo_define_property(device, MOUNT_MOTION_NS_PROPERTY, NULL);
			if (indigo_property_match(MOUNT_MOTION_WE_PROPERTY, property))
				indigo_define_property(device, MOUNT_MOTION_WE_PROPERTY, NULL);
			if (indigo_property_match(MOUNT_TRACK_RATE_PROPERTY, property))
				indigo_define_property(device, MOUNT_TRACK_RATE_PROPERTY, NULL);
			if (indigo_property_match(MOUNT_TRACKING_PROPERTY, property))
				indigo_define_property(device, MOUNT_TRACKING_PROPERTY, NULL);
			if (indigo_property_match(MOUNT_GUIDE_RATE_PROPERTY, property))
				indigo_define_property(device, MOUNT_GUIDE_RATE_PROPERTY, NULL);
			if (indigo_property_match(MOUNT_ON_COORDINATES_SET_PROPERTY, property))
				indigo_define_property(device, MOUNT_ON_COORDINATES_SET_PROPERTY, NULL);
			if (indigo_property_match(MOUNT_EQUATORIAL_COORDINATES_PROPERTY, property))
				indigo_define_property(device, MOUNT_EQUATORIAL_COORDINATES_PROPERTY, NULL);
			if (indigo_property_match(MOUNT_HORIZONTAL_COORDINATES_PROPERTY, property) && !MOUNT_HORIZONTAL_COORDINATES_PROPERTY->hidden)
				indigo_define_property(device, MOUNT_HORIZONTAL_COORDINATES_PROPERTY, NULL);
			if (indigo_property_match(MOUNT_ABORT_MOTION_PROPERTY, property) && !MOUNT_HORIZONTAL_COORDINATES_PROPERTY->hidden)
				indigo_define_property(device, MOUNT_ABORT_MOTION_PROPERTY, NULL);
		}
	}
	return result;
}

indigo_result indigo_mount_change_property(indigo_device *device, indigo_client *client, indigo_property *property) {
	assert(device != NULL);
	assert(device->device_context != NULL);
	assert(property != NULL);
	if (indigo_property_match(CONNECTION_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- CONNECTION
		if (CONNECTION_CONNECTED_ITEM->sw.value) {
			indigo_define_property(device, MOUNT_INFO_PROPERTY, NULL);
			indigo_define_property(device, MOUNT_GEOGRAPHIC_COORDINATES_PROPERTY, NULL);
			if (!MOUNT_LST_TIME_PROPERTY->hidden)
				indigo_define_property(device, MOUNT_LST_TIME_PROPERTY, NULL);
			if (!MOUNT_UTC_TIME_PROPERTY->hidden)
				indigo_define_property(device, MOUNT_UTC_TIME_PROPERTY, NULL);
			if (!MOUNT_UTC_FROM_HOST_PROPERTY->hidden)
				indigo_define_property(device, MOUNT_UTC_FROM_HOST_PROPERTY, NULL);
			indigo_define_property(device, MOUNT_PARK_PROPERTY, NULL);
			indigo_define_property(device, MOUNT_SLEW_RATE_PROPERTY, NULL);
			indigo_define_property(device, MOUNT_MOTION_NS_PROPERTY, NULL);
			indigo_define_property(device, MOUNT_MOTION_WE_PROPERTY, NULL);
			indigo_define_property(device, MOUNT_TRACK_RATE_PROPERTY, NULL);
			indigo_define_property(device, MOUNT_TRACKING_PROPERTY, NULL);
			indigo_define_property(device, MOUNT_GUIDE_RATE_PROPERTY, NULL);
			indigo_define_property(device, MOUNT_ON_COORDINATES_SET_PROPERTY, NULL);
			indigo_define_property(device, MOUNT_EQUATORIAL_COORDINATES_PROPERTY, NULL);
			if (!MOUNT_HORIZONTAL_COORDINATES_PROPERTY->hidden)
				indigo_define_property(device, MOUNT_HORIZONTAL_COORDINATES_PROPERTY, NULL);
			indigo_define_property(device, MOUNT_ABORT_MOTION_PROPERTY, NULL);
		} else {
			indigo_delete_property(device, MOUNT_INFO_PROPERTY, NULL);
			indigo_delete_property(device, MOUNT_GEOGRAPHIC_COORDINATES_PROPERTY, NULL);
			if (!MOUNT_LST_TIME_PROPERTY->hidden)
				indigo_delete_property(device, MOUNT_LST_TIME_PROPERTY, NULL);
			if (!MOUNT_UTC_TIME_PROPERTY->hidden)
				indigo_delete_property(device, MOUNT_UTC_TIME_PROPERTY, NULL);
			if (!MOUNT_UTC_FROM_HOST_PROPERTY->hidden)
				indigo_delete_property(device, MOUNT_UTC_FROM_HOST_PROPERTY, NULL);
			indigo_delete_property(device, MOUNT_PARK_PROPERTY, NULL);
			indigo_delete_property(device, MOUNT_SLEW_RATE_PROPERTY, NULL);
			indigo_delete_property(device, MOUNT_MOTION_NS_PROPERTY, NULL);
			indigo_delete_property(device, MOUNT_MOTION_WE_PROPERTY, NULL);
			indigo_delete_property(device, MOUNT_TRACK_RATE_PROPERTY, NULL);
			indigo_delete_property(device, MOUNT_TRACKING_PROPERTY, NULL);
			indigo_delete_property(device, MOUNT_GUIDE_RATE_PROPERTY, NULL);
			indigo_delete_property(device, MOUNT_ON_COORDINATES_SET_PROPERTY, NULL);
			indigo_delete_property(device, MOUNT_EQUATORIAL_COORDINATES_PROPERTY, NULL);
			if (!MOUNT_HORIZONTAL_COORDINATES_PROPERTY->hidden)
				indigo_delete_property(device, MOUNT_HORIZONTAL_COORDINATES_PROPERTY, NULL);
			indigo_delete_property(device, MOUNT_ABORT_MOTION_PROPERTY, NULL);
		}
	} else if (indigo_property_match(MOUNT_ON_COORDINATES_SET_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- MOUNT_ON_COORDINATES_SET
		indigo_property_copy_values(MOUNT_ON_COORDINATES_SET_PROPERTY, property, false);
		MOUNT_ON_COORDINATES_SET_PROPERTY->state = INDIGO_OK_STATE;
		indigo_update_property(device, MOUNT_ON_COORDINATES_SET_PROPERTY, NULL);
		return INDIGO_OK;
	} else if (indigo_property_match(MOUNT_TRACK_RATE_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- MOUNT_TRACK_RATE
		indigo_property_copy_values(MOUNT_TRACK_RATE_PROPERTY, property, false);
		MOUNT_TRACK_RATE_PROPERTY->state = INDIGO_OK_STATE;
		indigo_update_property(device, MOUNT_TRACK_RATE_PROPERTY, NULL);
		return INDIGO_OK;
	} else if (indigo_property_match(MOUNT_TRACKING_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- MOUNT_TRACKING
		indigo_property_copy_values(MOUNT_TRACKING_PROPERTY, property, false);
		MOUNT_TRACKING_PROPERTY->state = INDIGO_OK_STATE;
		indigo_update_property(device, MOUNT_TRACKING_PROPERTY, NULL);
		return INDIGO_OK;
	} else if (indigo_property_match(MOUNT_SLEW_RATE_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- MOUNT_SLEW_RATE
		indigo_property_copy_values(MOUNT_SLEW_RATE_PROPERTY, property, false);
		MOUNT_SLEW_RATE_PROPERTY->state = INDIGO_OK_STATE;
		indigo_update_property(device, MOUNT_SLEW_RATE_PROPERTY, NULL);
		return INDIGO_OK;
	} else if (indigo_property_match(MOUNT_GUIDE_RATE_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- MOUNT_GUIDE_RATE
		indigo_property_copy_values(MOUNT_GUIDE_RATE_PROPERTY, property, false);
		MOUNT_GUIDE_RATE_PROPERTY->state = INDIGO_OK_STATE;
		indigo_update_property(device, MOUNT_GUIDE_RATE_PROPERTY, NULL);
		return INDIGO_OK;
	} else if (indigo_property_match(CONFIG_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- CONFIG
		if (indigo_switch_match(CONFIG_SAVE_ITEM, property)) {
			if (MOUNT_SLEW_RATE_PROPERTY->perm == INDIGO_RW_PERM)
				indigo_save_property(device, NULL, MOUNT_SLEW_RATE_PROPERTY);
			if (MOUNT_TRACK_RATE_PROPERTY->perm == INDIGO_RW_PERM)
				indigo_save_property(device, NULL, MOUNT_TRACK_RATE_PROPERTY);
			if (MOUNT_TRACKING_PROPERTY->perm == INDIGO_RW_PERM)
				indigo_save_property(device, NULL, MOUNT_TRACKING_PROPERTY);
			if (MOUNT_GUIDE_RATE_PROPERTY->perm == INDIGO_RW_PERM)
				indigo_save_property(device, NULL, MOUNT_GUIDE_RATE_PROPERTY);
		}
		// --------------------------------------------------------------------------------
	}
	return indigo_device_change_property(device, client, property);
}

indigo_result indigo_mount_detach(indigo_device *device) {
	assert(device != NULL);
	indigo_release_property(MOUNT_INFO_PROPERTY);
	indigo_release_property(MOUNT_GEOGRAPHIC_COORDINATES_PROPERTY);
	indigo_release_property(MOUNT_LST_TIME_PROPERTY);
	indigo_release_property(MOUNT_UTC_TIME_PROPERTY);
	indigo_release_property(MOUNT_UTC_FROM_HOST_PROPERTY);
	indigo_release_property(MOUNT_PARK_PROPERTY);
	indigo_release_property(MOUNT_SLEW_RATE_PROPERTY);
	indigo_release_property(MOUNT_MOTION_NS_PROPERTY);
	indigo_release_property(MOUNT_MOTION_WE_PROPERTY);
	indigo_release_property(MOUNT_TRACK_RATE_PROPERTY);
	indigo_release_property(MOUNT_TRACKING_PROPERTY);
	indigo_release_property(MOUNT_GUIDE_RATE_PROPERTY);
	indigo_release_property(MOUNT_ON_COORDINATES_SET_PROPERTY);
	indigo_release_property(MOUNT_EQUATORIAL_COORDINATES_PROPERTY);
	indigo_release_property(MOUNT_HORIZONTAL_COORDINATES_PROPERTY);
	indigo_release_property(MOUNT_ABORT_MOTION_PROPERTY);
	return indigo_device_detach(device);
}

