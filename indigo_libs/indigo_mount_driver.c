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
// 2.0 by Peter Polakovic <peter.polakovic@cloudmakers.eu>

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
#include "indigo_io.h"
#include "indigo_novas.h"
#include "indigo_agent.h"

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
			MOUNT_GEOGRAPHIC_COORDINATES_PROPERTY = indigo_init_number_property(NULL, device->name, GEOGRAPHIC_COORDINATES_PROPERTY_NAME, MOUNT_SITE_GROUP, "Location", INDIGO_IDLE_STATE, INDIGO_RW_PERM, 3);
			if (MOUNT_GEOGRAPHIC_COORDINATES_PROPERTY == NULL)
				return INDIGO_FAILED;
			indigo_init_number_item(MOUNT_GEOGRAPHIC_COORDINATES_LATITUDE_ITEM, GEOGRAPHIC_COORDINATES_LATITUDE_ITEM_NAME, "Latitude (-90 to +90° +N)", -90, 90, 0, 0);
			indigo_init_number_item(MOUNT_GEOGRAPHIC_COORDINATES_LONGITUDE_ITEM, GEOGRAPHIC_COORDINATES_LONGITUDE_ITEM_NAME, "Longitude (0 to 360° +E)", -180, 360, 0, 0);
			indigo_init_number_item(MOUNT_GEOGRAPHIC_COORDINATES_ELEVATION_ITEM, GEOGRAPHIC_COORDINATES_ELEVATION_ITEM_NAME, "Elevation (m)", 0, 8000, 0, 0);
			// -------------------------------------------------------------------------------- MOUNT_LST_TIME
			MOUNT_LST_TIME_PROPERTY = indigo_init_number_property(NULL, device->name, MOUNT_LST_TIME_PROPERTY_NAME, MOUNT_SITE_GROUP, "LST Time", INDIGO_IDLE_STATE, INDIGO_RW_PERM, 1);
			if (MOUNT_LST_TIME_PROPERTY == NULL)
				return INDIGO_FAILED;
			MOUNT_LST_TIME_PROPERTY->hidden = true;
			indigo_init_number_item(MOUNT_LST_TIME_ITEM, MOUNT_LST_TIME_ITEM_NAME, "LST Time", 0, 24, 0, 0);
			// -------------------------------------------------------------------------------- MOUNT_UTC_TIME
			MOUNT_UTC_TIME_PROPERTY = indigo_init_text_property(NULL, device->name, UTC_TIME_PROPERTY_NAME, MOUNT_SITE_GROUP, "UTC time", INDIGO_IDLE_STATE, INDIGO_RW_PERM, 2);
			if (MOUNT_UTC_TIME_PROPERTY == NULL)
				return INDIGO_FAILED;
			MOUNT_UTC_TIME_PROPERTY->hidden = true;
			indigo_init_text_item(MOUNT_UTC_ITEM, UTC_TIME_ITEM_NAME, "UTC Time", "0000-00-00T00:00:00");
			indigo_init_text_item(MOUNT_UTC_OFFEST_ITEM, UTC_OFFSET_ITEM_NAME, "UTC Offset", "0"); /* step is 0.5 as there are timezones at 30 min */
			// -------------------------------------------------------------------------------- MOUNT_UTC_FROM_HOST
			MOUNT_SET_HOST_TIME_PROPERTY = indigo_init_switch_property(NULL, device->name, MOUNT_SET_HOST_TIME_PROPERTY_NAME, MOUNT_SITE_GROUP, "Set UTC", INDIGO_IDLE_STATE, INDIGO_RW_PERM, INDIGO_AT_MOST_ONE_RULE, 1);
			if (MOUNT_SET_HOST_TIME_PROPERTY == NULL)
				return INDIGO_FAILED;
			MOUNT_SET_HOST_TIME_PROPERTY->hidden = true;
			indigo_init_switch_item(MOUNT_SET_HOST_TIME_ITEM  , MOUNT_SET_HOST_TIME_ITEM_NAME, "From host", false);
			// -------------------------------------------------------------------------------- MOUNT_PARK
			MOUNT_PARK_PROPERTY = indigo_init_switch_property(NULL, device->name, MOUNT_PARK_PROPERTY_NAME, MOUNT_MAIN_GROUP, "Park", INDIGO_IDLE_STATE, INDIGO_RW_PERM, INDIGO_ONE_OF_MANY_RULE, 2);
			if (MOUNT_PARK_PROPERTY == NULL)
				return INDIGO_FAILED;
			indigo_init_switch_item(MOUNT_PARK_PARKED_ITEM, MOUNT_PARK_PARKED_ITEM_NAME, "Mount parked", true);
			indigo_init_switch_item(MOUNT_PARK_UNPARKED_ITEM, MOUNT_PARK_UNPARKED_ITEM_NAME, "Mount unparked", false);
			// -------------------------------------------------------------------------------- MOUNT_PARK_SET
			MOUNT_PARK_SET_PROPERTY = indigo_init_switch_property(NULL, device->name, MOUNT_PARK_SET_PROPERTY_NAME, MOUNT_MAIN_GROUP, "Set park position", INDIGO_IDLE_STATE, INDIGO_RW_PERM, INDIGO_AT_MOST_ONE_RULE, 2);
			if (MOUNT_PARK_SET_PROPERTY == NULL)
				return INDIGO_FAILED;
			MOUNT_PARK_SET_PROPERTY->hidden = true;
			indigo_init_switch_item(MOUNT_PARK_SET_DEFAULT_ITEM, MOUNT_PARK_SET_DEFAULT_ITEM_NAME, "Set default position", false);
			indigo_init_switch_item(MOUNT_PARK_SET_CURRENT_ITEM, MOUNT_PARK_SET_CURRENT_ITEM_NAME, "set current position", false);
			// -------------------------------------------------------------------------------- MOUNT_PARK_POSITION
			MOUNT_PARK_POSITION_PROPERTY = indigo_init_number_property(NULL, device->name, MOUNT_PARK_POSITION_PROPERTY_NAME, MOUNT_MAIN_GROUP, "Park position", INDIGO_IDLE_STATE, INDIGO_RW_PERM, 2);
			if (MOUNT_PARK_POSITION_PROPERTY == NULL)
				return INDIGO_FAILED;
			MOUNT_PARK_SET_PROPERTY->hidden = true;
			indigo_init_number_item(MOUNT_PARK_POSITION_RA_ITEM, MOUNT_PARK_POSITION_RA_ITEM_NAME, "Right ascension (0 to 24 hrs)", 0, 24, 0, 0);
			indigo_init_number_item(MOUNT_PARK_POSITION_DEC_ITEM, MOUNT_PARK_POSITION_DEC_ITEM_NAME, "Declination (-90 to 90°)", -90, 90, 0, 90);
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
			MOUNT_MOTION_DEC_PROPERTY = indigo_init_switch_property(NULL, device->name, MOUNT_MOTION_DEC_PROPERTY_NAME, MOUNT_MAIN_GROUP, "Move N/S", INDIGO_IDLE_STATE, INDIGO_RW_PERM, INDIGO_AT_MOST_ONE_RULE, 2);
			if (MOUNT_MOTION_DEC_PROPERTY == NULL)
				return INDIGO_FAILED;
			indigo_init_switch_item(MOUNT_MOTION_NORTH_ITEM, MOUNT_MOTION_NORTH_ITEM_NAME, "North", false);
			indigo_init_switch_item(MOUNT_MOTION_SOUTH_ITEM, MOUNT_MOTION_SOUTH_ITEM_NAME, "South", false);
			// -------------------------------------------------------------------------------- MOUNT_MOTION_WE
			MOUNT_MOTION_RA_PROPERTY = indigo_init_switch_property(NULL, device->name, MOUNT_MOTION_RA_PROPERTY_NAME, MOUNT_MAIN_GROUP, "Move W/E", INDIGO_IDLE_STATE, INDIGO_RW_PERM, INDIGO_AT_MOST_ONE_RULE, 2);
			if (MOUNT_MOTION_RA_PROPERTY == NULL)
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
			indigo_init_switch_item(MOUNT_TRACKING_ON_ITEM, MOUNT_TRACKING_ON_ITEM_NAME, "Tracking", true);
			indigo_init_switch_item(MOUNT_TRACKING_OFF_ITEM, MOUNT_TRACKING_OFF_ITEM_NAME, "Stopped" , false);
			// -------------------------------------------------------------------------------- MOUNT_GUIDE_RATE
			MOUNT_GUIDE_RATE_PROPERTY = indigo_init_number_property(NULL, device->name, MOUNT_GUIDE_RATE_PROPERTY_NAME, MOUNT_MAIN_GROUP, "Guide rate", INDIGO_IDLE_STATE, INDIGO_RW_PERM, 2);
			if (MOUNT_GUIDE_RATE_PROPERTY == NULL)
				return INDIGO_FAILED;
			indigo_init_number_item(MOUNT_GUIDE_RATE_RA_ITEM, MOUNT_GUIDE_RATE_RA_ITEM_NAME, "RA (% of sidereal)", 1, 100, 1, 50);
			indigo_init_number_item(MOUNT_GUIDE_RATE_DEC_ITEM, MOUNT_GUIDE_RATE_DEC_ITEM_NAME, "Dec (% of sidereal)", 1, 100, 1, 50);
			// -------------------------------------------------------------------------------- MOUNT_EQUATORIAL_COORDINATES
			MOUNT_EQUATORIAL_COORDINATES_PROPERTY = indigo_init_number_property(NULL, device->name, MOUNT_EQUATORIAL_COORDINATES_PROPERTY_NAME, MOUNT_MAIN_GROUP, "Equatorial EOD coordinates", INDIGO_IDLE_STATE, INDIGO_RW_PERM, 2);
			if (MOUNT_EQUATORIAL_COORDINATES_PROPERTY == NULL)
				return INDIGO_FAILED;
			indigo_init_number_item(MOUNT_EQUATORIAL_COORDINATES_RA_ITEM, MOUNT_EQUATORIAL_COORDINATES_RA_ITEM_NAME, "Right ascension (0 to 24 hrs)", 0, 24, 0, 0);
			indigo_init_number_item(MOUNT_EQUATORIAL_COORDINATES_DEC_ITEM, MOUNT_EQUATORIAL_COORDINATES_DEC_ITEM_NAME, "Declination (-90 to 90°)", -90, 90, 0, 90);
			// -------------------------------------------------------------------------------- MOUNT_HORIZONTAL_COORDINATES
			MOUNT_HORIZONTAL_COORDINATES_PROPERTY = indigo_init_number_property(NULL, device->name, MOUNT_HORIZONTAL_COORDINATES_PROPERTY_NAME, MOUNT_MAIN_GROUP, "Horizontal coordinates", INDIGO_IDLE_STATE, INDIGO_RO_PERM, 2);
			if (MOUNT_HORIZONTAL_COORDINATES_PROPERTY == NULL)
				return INDIGO_FAILED;
			indigo_init_number_item(MOUNT_HORIZONTAL_COORDINATES_AZ_ITEM, MOUNT_HORIZONTAL_COORDINATES_AZ_ITEM_NAME, "Azimuth (0 to 360°)", 0, 360, 0, 0);
			indigo_init_number_item(MOUNT_HORIZONTAL_COORDINATES_ALT_ITEM, MOUNT_HORIZONTAL_COORDINATES_ALT_ITEM_NAME, "Altitude (0 to 90°)", 0, 90, 0, 0);
			// -------------------------------------------------------------------------------- MOUNT_ABORT_MOTION
			MOUNT_ABORT_MOTION_PROPERTY = indigo_init_switch_property(NULL, device->name, MOUNT_ABORT_MOTION_PROPERTY_NAME, MOUNT_MAIN_GROUP, "Abort motion", INDIGO_IDLE_STATE, INDIGO_RW_PERM, INDIGO_ONE_OF_MANY_RULE, 1);
			if (MOUNT_ABORT_MOTION_PROPERTY == NULL)
				return INDIGO_FAILED;
			indigo_init_switch_item(MOUNT_ABORT_MOTION_ITEM, MOUNT_ABORT_MOTION_ITEM_NAME, "Abort motion", false);
			// -------------------------------------------------------------------------------- MOUNT_ALIGNMENT_MODE
			MOUNT_ALIGNMENT_MODE_PROPERTY = indigo_init_switch_property(NULL, device->name, MOUNT_ALIGNMENT_MODE_PROPERTY_NAME, MOUNT_ALIGNMENT_GROUP, "Alignment mode", INDIGO_IDLE_STATE, INDIGO_RW_PERM, INDIGO_ONE_OF_MANY_RULE, 3);
			if (MOUNT_ALIGNMENT_MODE_PROPERTY == NULL)
				return INDIGO_FAILED;
			indigo_init_switch_item(MOUNT_ALIGNMENT_MODE_CONTROLLER_ITEM, MOUNT_ALIGNMENT_MODE_CONTROLLER_ITEM_NAME, "Mount controller", true); // check MOUNT_ALIGNMENT_SELECT_POINTS and MOUNT_ALIGNMENT_DELETE_POINTS if default is changed
			indigo_init_switch_item(MOUNT_ALIGNMENT_MODE_SINGLE_POINT_ITEM, MOUNT_ALIGNMENT_MODE_SINGLE_POINT_ITEM_NAME, "Single point", false);
			indigo_init_switch_item(MOUNT_ALIGNMENT_MODE_MULTI_POINT_ITEM, MOUNT_ALIGNMENT_MODE_MULTI_POINT_ITEM_NAME, "Multi point", false);
			// -------------------------------------------------------------------------------- MOUNT_RAW_COORDINATES
			MOUNT_RAW_COORDINATES_PROPERTY = indigo_init_number_property(NULL, device->name, MOUNT_RAW_COORDINATES_PROPERTY_NAME, MOUNT_ALIGNMENT_GROUP, "Raw coordinates", INDIGO_IDLE_STATE, INDIGO_RO_PERM, 2);
			if (MOUNT_RAW_COORDINATES_PROPERTY == NULL)
				return INDIGO_FAILED;
			MOUNT_RAW_COORDINATES_PROPERTY->hidden = true;
			indigo_init_number_item(MOUNT_RAW_COORDINATES_RA_ITEM, MOUNT_RAW_COORDINATES_RA_ITEM_NAME, "Raw right ascension (0 to 24 hrs)", 0, 24, 0, 0);
			indigo_init_number_item(MOUNT_RAW_COORDINATES_DEC_ITEM, MOUNT_RAW_COORDINATES_DEC_ITEM_NAME, "Raw declination (-90 to 90°)", -90, 90, 0, 90);
			// -------------------------------------------------------------------------------- MOUNT_ALIGNMENT_SELECT_POINTS
			MOUNT_ALIGNMENT_SELECT_POINTS_PROPERTY = indigo_init_switch_property(NULL, device->name, MOUNT_ALIGNMENT_SELECT_POINTS_PROPERTY_NAME, MOUNT_ALIGNMENT_GROUP, "Select alignment points", INDIGO_IDLE_STATE, INDIGO_RW_PERM, INDIGO_ONE_OF_MANY_RULE, MOUNT_MAX_ALIGNMENT_POINTS);
			if (MOUNT_ALIGNMENT_SELECT_POINTS_PROPERTY == NULL)
				return INDIGO_FAILED;
			MOUNT_ALIGNMENT_SELECT_POINTS_PROPERTY->hidden = true;
			MOUNT_ALIGNMENT_SELECT_POINTS_PROPERTY->count = 0;
			// -------------------------------------------------------------------------------- MOUNT_ALIGNMENT_DELETE_POINTS
			MOUNT_ALIGNMENT_DELETE_POINTS_PROPERTY = indigo_init_switch_property(NULL, device->name, MOUNT_ALIGNMENT_DELETE_POINTS_PROPERTY_NAME, MOUNT_ALIGNMENT_GROUP, "Delete alignment points", INDIGO_IDLE_STATE, INDIGO_RW_PERM, INDIGO_ANY_OF_MANY_RULE, MOUNT_MAX_ALIGNMENT_POINTS);
			if (MOUNT_ALIGNMENT_DELETE_POINTS_PROPERTY == NULL)
				return INDIGO_FAILED;
			MOUNT_ALIGNMENT_DELETE_POINTS_PROPERTY->hidden = true;
			MOUNT_ALIGNMENT_DELETE_POINTS_PROPERTY->count = 0;
			// -------------------------------------------------------------------------------- SNOOP_DEVICES
			MOUNT_SNOOP_DEVICES_PROPERTY = indigo_init_text_property(NULL, device->name, SNOOP_DEVICES_PROPERTY_NAME, MAIN_GROUP, "Snoop devices", INDIGO_IDLE_STATE, INDIGO_RW_PERM, 2);
			if (MOUNT_SNOOP_DEVICES_PROPERTY == NULL)
				return INDIGO_FAILED;
			indigo_init_text_item(MOUNT_SNOOP_JOYSTICK_ITEM, SNOOP_JOYSTICK_ITEM_NAME, "Joystick", "");
			indigo_init_text_item(MOUNT_SNOOP_GPS_ITEM, SNOOP_GPS_ITEM_NAME, "GPS", "");
			// --------------------------------------------------------------------------------
			return INDIGO_OK;
		}
	}
	return INDIGO_FAILED;
}

indigo_result indigo_mount_enumerate_properties(indigo_device *device, indigo_client *client, indigo_property *property) {
	assert(device != NULL);
	assert(DEVICE_CONTEXT != NULL);
	indigo_result result = INDIGO_OK;
	if ((result = indigo_device_enumerate_properties(device, client, property)) == INDIGO_OK) {
		if (IS_CONNECTED) {
			if (indigo_property_match(MOUNT_INFO_PROPERTY, property))
				indigo_define_property(device, MOUNT_INFO_PROPERTY, NULL);
			if (indigo_property_match(MOUNT_GEOGRAPHIC_COORDINATES_PROPERTY, property))
				indigo_define_property(device, MOUNT_GEOGRAPHIC_COORDINATES_PROPERTY, NULL);
			if (indigo_property_match(MOUNT_LST_TIME_PROPERTY, property))
				indigo_define_property(device, MOUNT_LST_TIME_PROPERTY, NULL);
			if (indigo_property_match(MOUNT_UTC_TIME_PROPERTY, property))
				indigo_define_property(device, MOUNT_UTC_TIME_PROPERTY, NULL);
			if (indigo_property_match(MOUNT_SET_HOST_TIME_PROPERTY, property))
				indigo_define_property(device, MOUNT_SET_HOST_TIME_PROPERTY, NULL);
			if (indigo_property_match(MOUNT_PARK_PROPERTY, property))
				indigo_define_property(device, MOUNT_PARK_PROPERTY, NULL);
			if (indigo_property_match(MOUNT_PARK_SET_PROPERTY, property))
				indigo_define_property(device, MOUNT_PARK_SET_PROPERTY, NULL);
			if (indigo_property_match(MOUNT_PARK_POSITION_PROPERTY, property))
				indigo_define_property(device, MOUNT_PARK_POSITION_PROPERTY, NULL);
			if (indigo_property_match(MOUNT_SLEW_RATE_PROPERTY, property))
				indigo_define_property(device, MOUNT_SLEW_RATE_PROPERTY, NULL);
			if (indigo_property_match(MOUNT_MOTION_DEC_PROPERTY, property))
				indigo_define_property(device, MOUNT_MOTION_DEC_PROPERTY, NULL);
			if (indigo_property_match(MOUNT_MOTION_RA_PROPERTY, property))
				indigo_define_property(device, MOUNT_MOTION_RA_PROPERTY, NULL);
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
			if (indigo_property_match(MOUNT_HORIZONTAL_COORDINATES_PROPERTY, property))
				indigo_define_property(device, MOUNT_HORIZONTAL_COORDINATES_PROPERTY, NULL);
			if (indigo_property_match(MOUNT_ABORT_MOTION_PROPERTY, property))
				indigo_define_property(device, MOUNT_ABORT_MOTION_PROPERTY, NULL);
			if (indigo_property_match(MOUNT_ALIGNMENT_MODE_PROPERTY, property))
				indigo_define_property(device, MOUNT_ALIGNMENT_MODE_PROPERTY, NULL);
			if (indigo_property_match(MOUNT_RAW_COORDINATES_PROPERTY, property))
				indigo_define_property(device, MOUNT_RAW_COORDINATES_PROPERTY, NULL);
			if (indigo_property_match(MOUNT_ALIGNMENT_SELECT_POINTS_PROPERTY, property))
				indigo_define_property(device, MOUNT_ALIGNMENT_SELECT_POINTS_PROPERTY, NULL);
			if (indigo_property_match(MOUNT_ALIGNMENT_DELETE_POINTS_PROPERTY, property))
				indigo_define_property(device, MOUNT_ALIGNMENT_DELETE_POINTS_PROPERTY, NULL);
			if (indigo_property_match(MOUNT_SNOOP_DEVICES_PROPERTY, property))
				indigo_define_property(device, MOUNT_SNOOP_DEVICES_PROPERTY, NULL);
		}
	}
	return result;
}

indigo_result indigo_mount_change_property(indigo_device *device, indigo_client *client, indigo_property *property) {
	assert(device != NULL);
	assert(DEVICE_CONTEXT != NULL);
	assert(property != NULL);
	if (indigo_property_match(CONNECTION_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- CONNECTION
		if (IS_CONNECTED) {
			indigo_eq2hor(MOUNT_GEOGRAPHIC_COORDINATES_LATITUDE_ITEM->number.value, MOUNT_GEOGRAPHIC_COORDINATES_LONGITUDE_ITEM->number.value, MOUNT_GEOGRAPHIC_COORDINATES_ELEVATION_ITEM->number.value, MOUNT_EQUATORIAL_COORDINATES_RA_ITEM->number.value, MOUNT_EQUATORIAL_COORDINATES_DEC_ITEM->number.value, &MOUNT_HORIZONTAL_COORDINATES_ALT_ITEM->number.value, &MOUNT_HORIZONTAL_COORDINATES_AZ_ITEM->number.value);
			indigo_define_property(device, MOUNT_INFO_PROPERTY, NULL);
			indigo_define_property(device, MOUNT_GEOGRAPHIC_COORDINATES_PROPERTY, NULL);
			indigo_define_property(device, MOUNT_LST_TIME_PROPERTY, NULL);
			indigo_define_property(device, MOUNT_UTC_TIME_PROPERTY, NULL);
			indigo_define_property(device, MOUNT_SET_HOST_TIME_PROPERTY, NULL);
			indigo_define_property(device, MOUNT_PARK_PROPERTY, NULL);
			indigo_define_property(device, MOUNT_PARK_SET_PROPERTY, NULL);
			indigo_define_property(device, MOUNT_PARK_POSITION_PROPERTY, NULL);
			indigo_define_property(device, MOUNT_SLEW_RATE_PROPERTY, NULL);
			indigo_define_property(device, MOUNT_MOTION_DEC_PROPERTY, NULL);
			indigo_define_property(device, MOUNT_MOTION_RA_PROPERTY, NULL);
			indigo_define_property(device, MOUNT_TRACK_RATE_PROPERTY, NULL);
			indigo_define_property(device, MOUNT_TRACKING_PROPERTY, NULL);
			indigo_define_property(device, MOUNT_GUIDE_RATE_PROPERTY, NULL);
			indigo_define_property(device, MOUNT_ON_COORDINATES_SET_PROPERTY, NULL);
			indigo_define_property(device, MOUNT_EQUATORIAL_COORDINATES_PROPERTY, NULL);
			indigo_define_property(device, MOUNT_HORIZONTAL_COORDINATES_PROPERTY, NULL);
			indigo_define_property(device, MOUNT_ABORT_MOTION_PROPERTY, NULL);
			indigo_define_property(device, MOUNT_ALIGNMENT_MODE_PROPERTY, NULL);
			indigo_define_property(device, MOUNT_RAW_COORDINATES_PROPERTY, NULL);
			indigo_define_property(device, MOUNT_ALIGNMENT_SELECT_POINTS_PROPERTY, NULL);
			indigo_define_property(device, MOUNT_ALIGNMENT_DELETE_POINTS_PROPERTY, NULL);
			indigo_define_property(device, MOUNT_SNOOP_DEVICES_PROPERTY, NULL);
			indigo_add_snoop_rule(MOUNT_PARK_PROPERTY, MOUNT_SNOOP_JOYSTICK_ITEM->text.value, MOUNT_PARK_PROPERTY_NAME);
			indigo_add_snoop_rule(MOUNT_SLEW_RATE_PROPERTY, MOUNT_SNOOP_JOYSTICK_ITEM->text.value, MOUNT_SLEW_RATE_PROPERTY_NAME);
			indigo_add_snoop_rule(MOUNT_TRACKING_PROPERTY, MOUNT_SNOOP_JOYSTICK_ITEM->text.value, MOUNT_TRACKING_PROPERTY_NAME);
			indigo_add_snoop_rule(MOUNT_MOTION_DEC_PROPERTY, MOUNT_SNOOP_JOYSTICK_ITEM->text.value, MOUNT_MOTION_DEC_PROPERTY_NAME);
			indigo_add_snoop_rule(MOUNT_MOTION_RA_PROPERTY, MOUNT_SNOOP_JOYSTICK_ITEM->text.value, MOUNT_MOTION_RA_PROPERTY_NAME);
			indigo_add_snoop_rule(MOUNT_ABORT_MOTION_PROPERTY, MOUNT_SNOOP_JOYSTICK_ITEM->text.value, MOUNT_ABORT_MOTION_PROPERTY_NAME);
			indigo_add_snoop_rule(MOUNT_GEOGRAPHIC_COORDINATES_PROPERTY, MOUNT_SNOOP_GPS_ITEM->text.value, GEOGRAPHIC_COORDINATES_PROPERTY_NAME);
			indigo_add_snoop_rule(MOUNT_UTC_TIME_PROPERTY, MOUNT_SNOOP_GPS_ITEM->text.value, UTC_TIME_PROPERTY_NAME);
		} else {
			indigo_remove_snoop_rule(MOUNT_PARK_PROPERTY, MOUNT_SNOOP_JOYSTICK_ITEM->text.value, MOUNT_PARK_PROPERTY_NAME);
			indigo_remove_snoop_rule(MOUNT_SLEW_RATE_PROPERTY, MOUNT_SNOOP_JOYSTICK_ITEM->text.value, MOUNT_SLEW_RATE_PROPERTY_NAME);
			indigo_remove_snoop_rule(MOUNT_TRACKING_PROPERTY, MOUNT_SNOOP_JOYSTICK_ITEM->text.value, MOUNT_TRACKING_PROPERTY_NAME);
			indigo_remove_snoop_rule(MOUNT_MOTION_DEC_PROPERTY, MOUNT_SNOOP_JOYSTICK_ITEM->text.value, MOUNT_MOTION_DEC_PROPERTY_NAME);
			indigo_remove_snoop_rule(MOUNT_MOTION_RA_PROPERTY, MOUNT_SNOOP_JOYSTICK_ITEM->text.value, MOUNT_MOTION_RA_PROPERTY_NAME);
			indigo_remove_snoop_rule(MOUNT_ABORT_MOTION_PROPERTY, MOUNT_SNOOP_JOYSTICK_ITEM->text.value, MOUNT_ABORT_MOTION_PROPERTY_NAME);
			indigo_remove_snoop_rule(MOUNT_GEOGRAPHIC_COORDINATES_PROPERTY, MOUNT_SNOOP_GPS_ITEM->text.value, GEOGRAPHIC_COORDINATES_PROPERTY_NAME);
			indigo_remove_snoop_rule(MOUNT_UTC_TIME_PROPERTY, MOUNT_SNOOP_GPS_ITEM->text.value, UTC_TIME_PROPERTY_NAME);
			indigo_delete_property(device, MOUNT_INFO_PROPERTY, NULL);
			indigo_delete_property(device, MOUNT_GEOGRAPHIC_COORDINATES_PROPERTY, NULL);
			indigo_delete_property(device, MOUNT_LST_TIME_PROPERTY, NULL);
			indigo_delete_property(device, MOUNT_UTC_TIME_PROPERTY, NULL);
			indigo_delete_property(device, MOUNT_SET_HOST_TIME_PROPERTY, NULL);
			indigo_delete_property(device, MOUNT_PARK_PROPERTY, NULL);
			indigo_delete_property(device, MOUNT_PARK_SET_PROPERTY, NULL);
			indigo_delete_property(device, MOUNT_PARK_POSITION_PROPERTY, NULL);
			indigo_delete_property(device, MOUNT_SLEW_RATE_PROPERTY, NULL);
			indigo_delete_property(device, MOUNT_MOTION_DEC_PROPERTY, NULL);
			indigo_delete_property(device, MOUNT_MOTION_RA_PROPERTY, NULL);
			indigo_delete_property(device, MOUNT_TRACK_RATE_PROPERTY, NULL);
			indigo_delete_property(device, MOUNT_TRACKING_PROPERTY, NULL);
			indigo_delete_property(device, MOUNT_GUIDE_RATE_PROPERTY, NULL);
			indigo_delete_property(device, MOUNT_ON_COORDINATES_SET_PROPERTY, NULL);
			indigo_delete_property(device, MOUNT_EQUATORIAL_COORDINATES_PROPERTY, NULL);
			indigo_delete_property(device, MOUNT_HORIZONTAL_COORDINATES_PROPERTY, NULL);
			indigo_delete_property(device, MOUNT_ABORT_MOTION_PROPERTY, NULL);
			indigo_delete_property(device, MOUNT_ALIGNMENT_MODE_PROPERTY, NULL);
			indigo_delete_property(device, MOUNT_RAW_COORDINATES_PROPERTY, NULL);
			indigo_delete_property(device, MOUNT_ALIGNMENT_SELECT_POINTS_PROPERTY, NULL);
			indigo_delete_property(device, MOUNT_ALIGNMENT_DELETE_POINTS_PROPERTY, NULL);
			indigo_delete_property(device, MOUNT_SNOOP_DEVICES_PROPERTY, NULL);
		}
	} else if (indigo_property_match(MOUNT_GEOGRAPHIC_COORDINATES_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- MOUNT_GEOGRAPHIC_COORDINATES
		indigo_property_copy_values(MOUNT_GEOGRAPHIC_COORDINATES_PROPERTY, property, false);
		if (MOUNT_GEOGRAPHIC_COORDINATES_LONGITUDE_ITEM->number.value < 0)
			MOUNT_GEOGRAPHIC_COORDINATES_LONGITUDE_ITEM->number.value += 360;
		indigo_update_coordinates(device, "Geographic coordinates changed");
		MOUNT_GEOGRAPHIC_COORDINATES_PROPERTY->state = INDIGO_OK_STATE;
		indigo_update_property(device, MOUNT_GEOGRAPHIC_COORDINATES_PROPERTY, NULL);
		return INDIGO_OK;
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
	} else if (indigo_property_match(MOUNT_PARK_SET_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- MOUNT_PARK_SET
		indigo_property_copy_values(MOUNT_PARK_SET_PROPERTY, property, false);
		if (MOUNT_PARK_SET_DEFAULT_ITEM->sw.value) {
			MOUNT_PARK_POSITION_RA_ITEM->number.value = 0;
			MOUNT_PARK_POSITION_DEC_ITEM->number.value = 90;
			MOUNT_PARK_POSITION_PROPERTY->state = INDIGO_OK_STATE;
			indigo_update_property(device, MOUNT_PARK_POSITION_PROPERTY, NULL);
			MOUNT_PARK_SET_DEFAULT_ITEM->sw.value = false;
		} else if (MOUNT_PARK_SET_CURRENT_ITEM->sw.value) {
			MOUNT_PARK_POSITION_RA_ITEM->number.value = MOUNT_EQUATORIAL_COORDINATES_RA_ITEM->number.value;
			MOUNT_PARK_POSITION_DEC_ITEM->number.value = MOUNT_EQUATORIAL_COORDINATES_DEC_ITEM->number.value;
			MOUNT_PARK_POSITION_PROPERTY->state = INDIGO_OK_STATE;
			indigo_update_property(device, MOUNT_PARK_POSITION_PROPERTY, NULL);
			MOUNT_PARK_SET_CURRENT_ITEM->sw.value = false;
		}
		MOUNT_PARK_SET_PROPERTY->state = INDIGO_OK_STATE;
		indigo_update_property(device, MOUNT_PARK_SET_PROPERTY, NULL);
		return INDIGO_OK;
	} else if (indigo_property_match(MOUNT_PARK_POSITION_PROPERTY, property)) {
			// -------------------------------------------------------------------------------- MOUNT_PARK_POSITION
		indigo_property_copy_values(MOUNT_PARK_POSITION_PROPERTY, property, false);
		MOUNT_PARK_POSITION_PROPERTY->state = INDIGO_OK_STATE;
		indigo_update_property(device, MOUNT_PARK_POSITION_PROPERTY, NULL);
		return INDIGO_OK;
	} else if (indigo_property_match(CONFIG_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- CONFIG
		if (indigo_switch_match(CONFIG_SAVE_ITEM, property)) {
			indigo_save_property(device, NULL, MOUNT_GEOGRAPHIC_COORDINATES_PROPERTY);
			indigo_save_property(device, NULL, MOUNT_SLEW_RATE_PROPERTY);
			indigo_save_property(device, NULL, MOUNT_TRACK_RATE_PROPERTY);
			indigo_save_property(device, NULL, MOUNT_TRACKING_PROPERTY);
			indigo_save_property(device, NULL, MOUNT_GUIDE_RATE_PROPERTY);
			indigo_save_property(device, NULL, MOUNT_ALIGNMENT_MODE_PROPERTY);
			indigo_save_property(device, NULL, MOUNT_PARK_POSITION_PROPERTY);
			indigo_save_property(device, NULL, MOUNT_SNOOP_DEVICES_PROPERTY);
			int handle = indigo_open_config_file(device->name, 0, O_WRONLY | O_CREAT | O_TRUNC, ".alignment");
			if (handle > 0) {
				int count = MOUNT_CONTEXT->alignment_point_count;
				indigo_printf(handle, "%d\n", count);
				for (int i = 0; i < count; i++) {
					indigo_alignment_point *point =  MOUNT_CONTEXT->alignment_points + i;
					indigo_printf(handle, "%d %g %g %g %g\n", point->used, point->ra, point->dec, point->raw_ra, point->raw_dec);
				}
				close(handle);
			}
		} else if (indigo_switch_match(CONFIG_LOAD_ITEM, property)) {
			int handle = indigo_open_config_file(device->name, 0, O_RDONLY, ".alignment");
			if (handle > 0) {
				int count;
				char buffer[1024], name[INDIGO_NAME_SIZE], label[INDIGO_VALUE_SIZE];
				indigo_read_line(handle, buffer, sizeof(buffer));
				sscanf(buffer, "%d", &count);
				MOUNT_CONTEXT->alignment_point_count = count;
				MOUNT_ALIGNMENT_SELECT_POINTS_PROPERTY->count = count;
				MOUNT_ALIGNMENT_DELETE_POINTS_PROPERTY->count = count;
				for (int i = 0; i < count; i++) {
					indigo_alignment_point *point =  MOUNT_CONTEXT->alignment_points + i;
					indigo_read_line(handle, buffer, sizeof(buffer));
					sscanf(buffer, "%d %lg %lg %lg %lg", (int *)&point->used, &point->ra, &point->dec, &point->raw_ra, &point->raw_dec);
					snprintf(name, INDIGO_NAME_SIZE, "%d", i);
					snprintf(label, INDIGO_VALUE_SIZE, "RA %.2f / Dec %.2f", point->ra, point->dec);
					indigo_init_switch_item(MOUNT_ALIGNMENT_SELECT_POINTS_PROPERTY->items + i, name, label, point->used);
					indigo_init_switch_item(MOUNT_ALIGNMENT_DELETE_POINTS_PROPERTY->items + i, name, label, false);
				}
				close(handle);
				MOUNT_ALIGNMENT_SELECT_POINTS_PROPERTY->state = INDIGO_OK_STATE;
				indigo_update_property(device, MOUNT_ALIGNMENT_SELECT_POINTS_PROPERTY, NULL);
				MOUNT_ALIGNMENT_DELETE_POINTS_PROPERTY->state = INDIGO_OK_STATE;
				indigo_update_property(device, MOUNT_ALIGNMENT_DELETE_POINTS_PROPERTY, NULL);
			}
		}
	} else if (indigo_property_match(MOUNT_EQUATORIAL_COORDINATES_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- MOUNT_EQUATORIAL_COORDINATES
		if (MOUNT_ON_COORDINATES_SET_SYNC_ITEM->sw.value) {
			if (MOUNT_ALIGNMENT_MODE_CONTROLLER_ITEM->sw.value) {
				MOUNT_EQUATORIAL_COORDINATES_PROPERTY->state = INDIGO_ALERT_STATE;
				indigo_update_coordinates(device, "SYNC in CONTROLLER mode passed to indigo_mount_change_property");
			} else if (MOUNT_CONTEXT->alignment_point_count >= MOUNT_MAX_ALIGNMENT_POINTS) {
				MOUNT_EQUATORIAL_COORDINATES_PROPERTY->state = INDIGO_ALERT_STATE;
				indigo_update_coordinates(device, "Too many alignment points");
			} else {
				indigo_property_copy_values(MOUNT_EQUATORIAL_COORDINATES_PROPERTY, property, false);
				int index = MOUNT_CONTEXT->alignment_point_count++;
				indigo_alignment_point *point = MOUNT_CONTEXT->alignment_points + index;
				point->ra = MOUNT_EQUATORIAL_COORDINATES_RA_ITEM->number.value;
				point->dec = MOUNT_EQUATORIAL_COORDINATES_DEC_ITEM->number.value;
				point->raw_ra = MOUNT_RAW_COORDINATES_RA_ITEM->number.value;
				point->raw_dec = MOUNT_RAW_COORDINATES_DEC_ITEM->number.value;
				char name[INDIGO_NAME_SIZE], label[INDIGO_VALUE_SIZE];
				snprintf(name, INDIGO_NAME_SIZE, "%d", index);
				snprintf(label, INDIGO_VALUE_SIZE, "RA %.2f / Dec %.2f", point->ra, point->dec);
				indigo_init_switch_item(MOUNT_ALIGNMENT_SELECT_POINTS_PROPERTY->items + index, name, label, true);
				point->used = true;
				if (MOUNT_ALIGNMENT_MODE_SINGLE_POINT_ITEM->sw.value) {
					for (int i = 0; i < MOUNT_ALIGNMENT_SELECT_POINTS_PROPERTY->count; i++) {
						MOUNT_ALIGNMENT_SELECT_POINTS_PROPERTY->items[i].sw.value = false;
						MOUNT_CONTEXT->alignment_points[i].used = false;
					}
				}
				MOUNT_ALIGNMENT_SELECT_POINTS_PROPERTY->count = MOUNT_CONTEXT->alignment_point_count;
				MOUNT_ALIGNMENT_SELECT_POINTS_PROPERTY->state = INDIGO_OK_STATE;
				indigo_delete_property(device, MOUNT_ALIGNMENT_SELECT_POINTS_PROPERTY, NULL);
				indigo_define_property(device, MOUNT_ALIGNMENT_SELECT_POINTS_PROPERTY, NULL);
				indigo_init_switch_item(MOUNT_ALIGNMENT_DELETE_POINTS_PROPERTY->items + index, name, label, false);
				MOUNT_ALIGNMENT_DELETE_POINTS_PROPERTY->count = MOUNT_CONTEXT->alignment_point_count;
				MOUNT_ALIGNMENT_DELETE_POINTS_PROPERTY->state = INDIGO_OK_STATE;
				indigo_delete_property(device, MOUNT_ALIGNMENT_DELETE_POINTS_PROPERTY, NULL);
				indigo_define_property(device, MOUNT_ALIGNMENT_DELETE_POINTS_PROPERTY, NULL);
				MOUNT_EQUATORIAL_COORDINATES_PROPERTY->state = INDIGO_OK_STATE;
				indigo_update_coordinates(device, NULL);
			}
		}
		return INDIGO_OK;
	} else if (indigo_property_match(MOUNT_ALIGNMENT_MODE_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- MOUNT_ALIGNMENT_MODE
		indigo_property_copy_values(MOUNT_ALIGNMENT_MODE_PROPERTY, property, false);
		indigo_delete_property(device, MOUNT_RAW_COORDINATES_PROPERTY, NULL);
		indigo_delete_property(device, MOUNT_ALIGNMENT_SELECT_POINTS_PROPERTY, NULL);
		indigo_delete_property(device, MOUNT_ALIGNMENT_DELETE_POINTS_PROPERTY, NULL);
		if (MOUNT_ALIGNMENT_MODE_SINGLE_POINT_ITEM->sw.value) {
			MOUNT_RAW_COORDINATES_PROPERTY->hidden = false;
			MOUNT_ALIGNMENT_SELECT_POINTS_PROPERTY->hidden = false;
			MOUNT_ALIGNMENT_SELECT_POINTS_PROPERTY->rule = INDIGO_ONE_OF_MANY_RULE;
			MOUNT_ALIGNMENT_DELETE_POINTS_PROPERTY->hidden = false;
		} else if (MOUNT_ALIGNMENT_MODE_MULTI_POINT_ITEM->sw.value) {
			MOUNT_RAW_COORDINATES_PROPERTY->hidden = false;
			MOUNT_ALIGNMENT_SELECT_POINTS_PROPERTY->hidden = false;
			MOUNT_ALIGNMENT_SELECT_POINTS_PROPERTY->rule = INDIGO_ANY_OF_MANY_RULE;
			MOUNT_ALIGNMENT_DELETE_POINTS_PROPERTY->hidden = false;
		} else {
			MOUNT_RAW_COORDINATES_PROPERTY->hidden = true;
			MOUNT_ALIGNMENT_SELECT_POINTS_PROPERTY->hidden = true;
			MOUNT_ALIGNMENT_DELETE_POINTS_PROPERTY->hidden = true;
		}
		indigo_define_property(device, MOUNT_RAW_COORDINATES_PROPERTY, NULL);
		indigo_define_property(device, MOUNT_ALIGNMENT_SELECT_POINTS_PROPERTY, NULL);
		indigo_define_property(device, MOUNT_ALIGNMENT_DELETE_POINTS_PROPERTY, NULL);
		indigo_raw_to_translated(device, MOUNT_RAW_COORDINATES_RA_ITEM->number.value, MOUNT_RAW_COORDINATES_DEC_ITEM->number.value, &MOUNT_EQUATORIAL_COORDINATES_RA_ITEM->number.value, &MOUNT_EQUATORIAL_COORDINATES_DEC_ITEM->number.value);
		indigo_raw_to_translated(device, MOUNT_RAW_COORDINATES_RA_ITEM->number.target, MOUNT_RAW_COORDINATES_DEC_ITEM->number.target, &MOUNT_EQUATORIAL_COORDINATES_RA_ITEM->number.target, &MOUNT_EQUATORIAL_COORDINATES_DEC_ITEM->number.target);
		MOUNT_EQUATORIAL_COORDINATES_PROPERTY->state = INDIGO_OK_STATE;
		indigo_update_coordinates(device, NULL);
		MOUNT_ALIGNMENT_MODE_PROPERTY->state = INDIGO_OK_STATE;
		indigo_update_property(device, MOUNT_ALIGNMENT_MODE_PROPERTY, NULL);
		return INDIGO_OK;
	} else if (indigo_property_match(MOUNT_ALIGNMENT_SELECT_POINTS_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- MOUNT_ALIGNMENT_SELECT_POINTS
		indigo_property_copy_values(MOUNT_ALIGNMENT_SELECT_POINTS_PROPERTY, property, false);
		for (int i = 0; i < MOUNT_ALIGNMENT_SELECT_POINTS_PROPERTY->count; i++) {
			int index = atoi(MOUNT_ALIGNMENT_SELECT_POINTS_PROPERTY->items[i].name);
			if (index < MOUNT_CONTEXT->alignment_point_count) {
				bool used = MOUNT_ALIGNMENT_SELECT_POINTS_PROPERTY->items[i].sw.value;
				MOUNT_CONTEXT->alignment_points[index].used = used;
			}
		}
		indigo_raw_to_translated(device, MOUNT_RAW_COORDINATES_RA_ITEM->number.value, MOUNT_RAW_COORDINATES_DEC_ITEM->number.value, &MOUNT_EQUATORIAL_COORDINATES_RA_ITEM->number.value, &MOUNT_EQUATORIAL_COORDINATES_DEC_ITEM->number.value);
		indigo_raw_to_translated(device, MOUNT_RAW_COORDINATES_RA_ITEM->number.target, MOUNT_RAW_COORDINATES_DEC_ITEM->number.target, &MOUNT_EQUATORIAL_COORDINATES_RA_ITEM->number.target, &MOUNT_EQUATORIAL_COORDINATES_DEC_ITEM->number.target);
		MOUNT_EQUATORIAL_COORDINATES_PROPERTY->state = INDIGO_OK_STATE;
		indigo_update_coordinates(device, NULL);
		MOUNT_ALIGNMENT_MODE_PROPERTY->state = INDIGO_OK_STATE;
		indigo_update_property(device, MOUNT_ALIGNMENT_SELECT_POINTS_PROPERTY, NULL);
		return INDIGO_OK;
	} else if (indigo_property_match(MOUNT_ALIGNMENT_DELETE_POINTS_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- MOUNT_ALIGNMENT_DELETE_POINTS
		for (int i = 0; i < property->count; i++) {
			int index = atoi(property->items[i].name);
			if (index < MOUNT_CONTEXT->alignment_point_count) {
				if (property->items[i].sw.value) {
					for (int j = index + 1; j < MOUNT_CONTEXT->alignment_point_count; j++) {
						char name[INDIGO_NAME_SIZE];
						snprintf(name, INDIGO_NAME_SIZE, "%d", j - 1);
						MOUNT_CONTEXT->alignment_points[j - 1] = MOUNT_CONTEXT->alignment_points[j];
						MOUNT_ALIGNMENT_SELECT_POINTS_PROPERTY->items[j - 1] = MOUNT_ALIGNMENT_SELECT_POINTS_PROPERTY->items[j];
						strncpy(MOUNT_ALIGNMENT_SELECT_POINTS_PROPERTY->items[j - 1].name, name, INDIGO_NAME_SIZE);
						MOUNT_ALIGNMENT_DELETE_POINTS_PROPERTY->items[j - 1] = MOUNT_ALIGNMENT_DELETE_POINTS_PROPERTY->items[j];
						strncpy(MOUNT_ALIGNMENT_DELETE_POINTS_PROPERTY->items[j - 1].name, name, INDIGO_NAME_SIZE);
					}
					MOUNT_ALIGNMENT_SELECT_POINTS_PROPERTY->count = MOUNT_ALIGNMENT_DELETE_POINTS_PROPERTY->count = --MOUNT_CONTEXT->alignment_point_count;
				}
			}
		}
		indigo_raw_to_translated(device, MOUNT_RAW_COORDINATES_RA_ITEM->number.value, MOUNT_RAW_COORDINATES_DEC_ITEM->number.value, &MOUNT_EQUATORIAL_COORDINATES_RA_ITEM->number.value, &MOUNT_EQUATORIAL_COORDINATES_DEC_ITEM->number.value);
		indigo_raw_to_translated(device, MOUNT_RAW_COORDINATES_RA_ITEM->number.target, MOUNT_RAW_COORDINATES_DEC_ITEM->number.target, &MOUNT_EQUATORIAL_COORDINATES_RA_ITEM->number.target, &MOUNT_EQUATORIAL_COORDINATES_DEC_ITEM->number.target);
		MOUNT_EQUATORIAL_COORDINATES_PROPERTY->state = INDIGO_OK_STATE;
		indigo_update_coordinates(device, NULL);
		indigo_delete_property(device, MOUNT_ALIGNMENT_SELECT_POINTS_PROPERTY, NULL);
		MOUNT_ALIGNMENT_SELECT_POINTS_PROPERTY->state = INDIGO_OK_STATE;
		indigo_define_property(device, MOUNT_ALIGNMENT_SELECT_POINTS_PROPERTY, NULL);
		indigo_delete_property(device, MOUNT_ALIGNMENT_DELETE_POINTS_PROPERTY, NULL);
		MOUNT_ALIGNMENT_DELETE_POINTS_PROPERTY->state = INDIGO_OK_STATE;
		indigo_define_property(device, MOUNT_ALIGNMENT_DELETE_POINTS_PROPERTY, NULL);
		return INDIGO_OK;
		// -------------------------------------------------------------------------------- SNOOP_DEVICES
	} else if (indigo_property_match(MOUNT_SNOOP_DEVICES_PROPERTY, property)) {
		indigo_remove_snoop_rule(MOUNT_PARK_PROPERTY, MOUNT_SNOOP_JOYSTICK_ITEM->text.value, MOUNT_PARK_PROPERTY_NAME);
		indigo_remove_snoop_rule(MOUNT_SLEW_RATE_PROPERTY, MOUNT_SNOOP_JOYSTICK_ITEM->text.value, MOUNT_SLEW_RATE_PROPERTY_NAME);
		indigo_remove_snoop_rule(MOUNT_TRACKING_PROPERTY, MOUNT_SNOOP_JOYSTICK_ITEM->text.value, MOUNT_TRACKING_PROPERTY_NAME);
		indigo_remove_snoop_rule(MOUNT_MOTION_DEC_PROPERTY, MOUNT_SNOOP_JOYSTICK_ITEM->text.value, MOUNT_MOTION_DEC_PROPERTY_NAME);
		indigo_remove_snoop_rule(MOUNT_MOTION_RA_PROPERTY, MOUNT_SNOOP_JOYSTICK_ITEM->text.value, MOUNT_MOTION_RA_PROPERTY_NAME);
		indigo_remove_snoop_rule(MOUNT_ABORT_MOTION_PROPERTY, MOUNT_SNOOP_JOYSTICK_ITEM->text.value, MOUNT_ABORT_MOTION_PROPERTY_NAME);
		indigo_remove_snoop_rule(MOUNT_GEOGRAPHIC_COORDINATES_PROPERTY, MOUNT_SNOOP_GPS_ITEM->text.value, GEOGRAPHIC_COORDINATES_PROPERTY_NAME);
		indigo_remove_snoop_rule(MOUNT_UTC_TIME_PROPERTY, MOUNT_SNOOP_GPS_ITEM->text.value, UTC_TIME_PROPERTY_NAME);
		indigo_property_copy_values(MOUNT_SNOOP_DEVICES_PROPERTY, property, false);
		indigo_trim_local_service(MOUNT_SNOOP_JOYSTICK_ITEM->text.value);
		indigo_trim_local_service(MOUNT_SNOOP_GPS_ITEM->text.value);
		indigo_add_snoop_rule(MOUNT_PARK_PROPERTY, MOUNT_SNOOP_JOYSTICK_ITEM->text.value, MOUNT_PARK_PROPERTY_NAME);
		indigo_add_snoop_rule(MOUNT_SLEW_RATE_PROPERTY, MOUNT_SNOOP_JOYSTICK_ITEM->text.value, MOUNT_SLEW_RATE_PROPERTY_NAME);
		indigo_add_snoop_rule(MOUNT_TRACKING_PROPERTY, MOUNT_SNOOP_JOYSTICK_ITEM->text.value, MOUNT_TRACKING_PROPERTY_NAME);
		indigo_add_snoop_rule(MOUNT_MOTION_DEC_PROPERTY, MOUNT_SNOOP_JOYSTICK_ITEM->text.value, MOUNT_MOTION_DEC_PROPERTY_NAME);
		indigo_add_snoop_rule(MOUNT_MOTION_RA_PROPERTY, MOUNT_SNOOP_JOYSTICK_ITEM->text.value, MOUNT_MOTION_RA_PROPERTY_NAME);
		indigo_add_snoop_rule(MOUNT_ABORT_MOTION_PROPERTY, MOUNT_SNOOP_JOYSTICK_ITEM->text.value, MOUNT_ABORT_MOTION_PROPERTY_NAME);
		indigo_add_snoop_rule(MOUNT_GEOGRAPHIC_COORDINATES_PROPERTY, MOUNT_SNOOP_GPS_ITEM->text.value, GEOGRAPHIC_COORDINATES_PROPERTY_NAME);
		indigo_add_snoop_rule(MOUNT_UTC_TIME_PROPERTY, MOUNT_SNOOP_GPS_ITEM->text.value, UTC_TIME_PROPERTY_NAME);
		MOUNT_SNOOP_DEVICES_PROPERTY->state = INDIGO_OK_STATE;
		indigo_update_property(device, MOUNT_SNOOP_DEVICES_PROPERTY, NULL);
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
	indigo_release_property(MOUNT_SET_HOST_TIME_PROPERTY);
	indigo_release_property(MOUNT_PARK_PROPERTY);
	indigo_release_property(MOUNT_PARK_SET_PROPERTY);
	indigo_release_property(MOUNT_PARK_POSITION_PROPERTY);
	indigo_release_property(MOUNT_SLEW_RATE_PROPERTY);
	indigo_release_property(MOUNT_MOTION_DEC_PROPERTY);
	indigo_release_property(MOUNT_MOTION_RA_PROPERTY);
	indigo_release_property(MOUNT_TRACK_RATE_PROPERTY);
	indigo_release_property(MOUNT_TRACKING_PROPERTY);
	indigo_release_property(MOUNT_GUIDE_RATE_PROPERTY);
	indigo_release_property(MOUNT_ON_COORDINATES_SET_PROPERTY);
	indigo_release_property(MOUNT_EQUATORIAL_COORDINATES_PROPERTY);
	indigo_release_property(MOUNT_HORIZONTAL_COORDINATES_PROPERTY);
	indigo_release_property(MOUNT_ABORT_MOTION_PROPERTY);
	indigo_release_property(MOUNT_ALIGNMENT_MODE_PROPERTY);
	indigo_release_property(MOUNT_RAW_COORDINATES_PROPERTY);
	indigo_release_property(MOUNT_ALIGNMENT_SELECT_POINTS_PROPERTY);
	indigo_release_property(MOUNT_ALIGNMENT_DELETE_POINTS_PROPERTY);
	indigo_release_property(MOUNT_SNOOP_DEVICES_PROPERTY);
	return indigo_device_detach(device);
}

indigo_result indigo_translated_to_raw(indigo_device *device, double ra, double dec, double *raw_ra, double *raw_dec) {
	if (MOUNT_ALIGNMENT_MODE_CONTROLLER_ITEM->sw.value) {
		*raw_ra = ra;
		*raw_dec = dec;
		return INDIGO_OK;
	} else if (MOUNT_ALIGNMENT_MODE_SINGLE_POINT_ITEM->sw.value) {
		for (int i = 0; i < MOUNT_CONTEXT->alignment_point_count; i++) {
			indigo_alignment_point *point = MOUNT_CONTEXT->alignment_points + i;
			if (point->used) {
				*raw_ra = ra + (point->raw_ra - point->ra);
				*raw_dec = dec + (point->raw_dec - point->dec);
				return INDIGO_OK;
			}
		}
		*raw_ra = ra;
		*raw_dec = dec;
		return INDIGO_OK;
	} else if (MOUNT_ALIGNMENT_MODE_MULTI_POINT_ITEM->sw.value) {

		// TBD Rumen

		return INDIGO_OK;
	}
	return INDIGO_FAILED;
}

indigo_result indigo_raw_to_translated(indigo_device *device, double raw_ra, double raw_dec, double *ra, double *dec) {
	if (MOUNT_ALIGNMENT_MODE_CONTROLLER_ITEM->sw.value) {
		*ra = raw_ra;
		*dec = raw_dec;
		return INDIGO_OK;
	} else if (MOUNT_ALIGNMENT_MODE_SINGLE_POINT_ITEM->sw.value) {
		for (int i = 0; i < MOUNT_CONTEXT->alignment_point_count; i++) {
			indigo_alignment_point *point = MOUNT_CONTEXT->alignment_points + i;
			if (point->used) {
				*ra = raw_ra + (point->ra - point->raw_ra);
				*dec = raw_dec + (point->dec - point->raw_dec);
				return INDIGO_OK;
			}
		}
		*ra = raw_ra;
		*dec = raw_dec;
		return INDIGO_OK;
	} else if (MOUNT_ALIGNMENT_MODE_MULTI_POINT_ITEM->sw.value) {

		// TBD Rumen

		return INDIGO_OK;
	}
	return INDIGO_FAILED;
}

void indigo_update_coordinates(indigo_device *device, const char *message) {
	indigo_update_property(device, MOUNT_EQUATORIAL_COORDINATES_PROPERTY, message);
	if (!MOUNT_GEOGRAPHIC_COORDINATES_PROPERTY->hidden && !MOUNT_HORIZONTAL_COORDINATES_PROPERTY->hidden) {
		indigo_eq2hor(MOUNT_GEOGRAPHIC_COORDINATES_LATITUDE_ITEM->number.value, MOUNT_GEOGRAPHIC_COORDINATES_LONGITUDE_ITEM->number.value, MOUNT_GEOGRAPHIC_COORDINATES_ELEVATION_ITEM->number.value, MOUNT_EQUATORIAL_COORDINATES_RA_ITEM->number.value, MOUNT_EQUATORIAL_COORDINATES_DEC_ITEM->number.value, &MOUNT_HORIZONTAL_COORDINATES_ALT_ITEM->number.value, &MOUNT_HORIZONTAL_COORDINATES_AZ_ITEM->number.value);
		MOUNT_HORIZONTAL_COORDINATES_PROPERTY->state = MOUNT_EQUATORIAL_COORDINATES_PROPERTY->state;
		indigo_update_property(device, MOUNT_HORIZONTAL_COORDINATES_PROPERTY, NULL);
	}
}
