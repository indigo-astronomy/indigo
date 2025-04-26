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
#include <assert.h>
#include <string.h>
#include <errno.h>
#include <time.h>
#include <math.h>
#include <sys/stat.h>

#include <indigo/indigo_mount_driver.h>
#include <indigo/indigo_io.h>
#include <indigo/indigo_align.h>
#include <indigo/indigo_agent.h>

# define DEG2RAD (M_PI/180.0)

static double indigo_range24(double ha) {
	return fmod(ha + (24000), 24);
}

indigo_result indigo_mount_attach(indigo_device *device, const char* driver_name, unsigned version) {
	assert(device != NULL);
	assert(device != NULL);
	if (MOUNT_CONTEXT == NULL) {
		device->device_context = indigo_safe_malloc(sizeof(indigo_mount_context));
	}
	if (MOUNT_CONTEXT != NULL) {
		if (indigo_device_attach(device, driver_name, version, INDIGO_INTERFACE_MOUNT) == INDIGO_OK) {
			// -------------------------------------------------------------------------------- MOUNT_INFO
			MOUNT_INFO_PROPERTY = indigo_init_text_property(NULL, device->name, MOUNT_INFO_PROPERTY_NAME, MOUNT_MAIN_GROUP, "Info", INDIGO_OK_STATE, INDIGO_RO_PERM, 3);
			if (MOUNT_INFO_PROPERTY == NULL) {
				return INDIGO_FAILED;
			}
			indigo_init_text_item(MOUNT_INFO_VENDOR_ITEM, MOUNT_INFO_VENDOR_ITEM_NAME, "Vendor", "Unknown");
			indigo_init_text_item(MOUNT_INFO_MODEL_ITEM, MOUNT_INFO_MODEL_ITEM_NAME, "Model", "Unknown");
			indigo_init_text_item(MOUNT_INFO_FIRMWARE_ITEM, MOUNT_INFO_FIRMWARE_ITEM_NAME, "Firmware", "N/A");
			// -------------------------------------------------------------------------------- MOUNT_GEOGRAPHIC_COORDINATES
			MOUNT_GEOGRAPHIC_COORDINATES_PROPERTY = indigo_init_number_property(NULL, device->name, GEOGRAPHIC_COORDINATES_PROPERTY_NAME, MOUNT_SITE_GROUP, "Location", INDIGO_OK_STATE, INDIGO_RW_PERM, 3);
			if (MOUNT_GEOGRAPHIC_COORDINATES_PROPERTY == NULL) {
				return INDIGO_FAILED;
			}
			indigo_init_sexagesimal_number_item(MOUNT_GEOGRAPHIC_COORDINATES_LATITUDE_ITEM, GEOGRAPHIC_COORDINATES_LATITUDE_ITEM_NAME, "Latitude (-90° to +90° +N)", -90, 90, 0, 0);
			indigo_init_sexagesimal_number_item(MOUNT_GEOGRAPHIC_COORDINATES_LONGITUDE_ITEM, GEOGRAPHIC_COORDINATES_LONGITUDE_ITEM_NAME, "Longitude (0° to 360° +E)", -180, 360, 0, 0);
			indigo_init_number_item(MOUNT_GEOGRAPHIC_COORDINATES_ELEVATION_ITEM, GEOGRAPHIC_COORDINATES_ELEVATION_ITEM_NAME, "Elevation (m)", -400, 8000, 0, 0);
			// -------------------------------------------------------------------------------- MOUNT_LST_TIME
			MOUNT_LST_TIME_PROPERTY = indigo_init_number_property(NULL, device->name, MOUNT_LST_TIME_PROPERTY_NAME, MOUNT_SITE_GROUP, "LST Time", INDIGO_OK_STATE, INDIGO_RO_PERM, 1);
			if (MOUNT_LST_TIME_PROPERTY == NULL) {
				return INDIGO_FAILED;
			}
			indigo_init_sexagesimal_number_item(MOUNT_LST_TIME_ITEM, MOUNT_LST_TIME_ITEM_NAME, "LST Time", 0, 24, 0, 0);
			// -------------------------------------------------------------------------------- MOUNT_UTC_TIME
			MOUNT_UTC_TIME_PROPERTY = indigo_init_text_property(NULL, device->name, UTC_TIME_PROPERTY_NAME, MOUNT_SITE_GROUP, "UTC time", INDIGO_OK_STATE, INDIGO_RW_PERM, 2);
			if (MOUNT_UTC_TIME_PROPERTY == NULL) {
				return INDIGO_FAILED;
			}
			MOUNT_UTC_TIME_PROPERTY->hidden = true;
			indigo_init_text_item(MOUNT_UTC_ITEM, UTC_TIME_ITEM_NAME, "UTC Time", "0000-00-00T00:00:00");
			indigo_init_text_item(MOUNT_UTC_OFFSET_ITEM, UTC_OFFSET_ITEM_NAME, "UTC Offset", "0"); /* step is 0.5 as there are timezones at 30 min */
			// -------------------------------------------------------------------------------- MOUNT_UTC_FROM_HOST
			MOUNT_SET_HOST_TIME_PROPERTY = indigo_init_switch_property(NULL, device->name, MOUNT_SET_HOST_TIME_PROPERTY_NAME, MOUNT_SITE_GROUP, "Set UTC", INDIGO_OK_STATE, INDIGO_RW_PERM, INDIGO_AT_MOST_ONE_RULE, 1);
			if (MOUNT_SET_HOST_TIME_PROPERTY == NULL) {
				return INDIGO_FAILED;
			}
			MOUNT_SET_HOST_TIME_PROPERTY->hidden = true;
			indigo_init_switch_item(MOUNT_SET_HOST_TIME_ITEM, MOUNT_SET_HOST_TIME_ITEM_NAME, "From host", false);
			// -------------------------------------------------------------------------------- MOUNT_PARK
			MOUNT_PARK_PROPERTY = indigo_init_switch_property(NULL, device->name, MOUNT_PARK_PROPERTY_NAME, MOUNT_MAIN_GROUP, "Park", INDIGO_OK_STATE, INDIGO_RW_PERM, INDIGO_ONE_OF_MANY_RULE, 2);
			if (MOUNT_PARK_PROPERTY == NULL) {
				return INDIGO_FAILED;
			}
			indigo_init_switch_item(MOUNT_PARK_PARKED_ITEM, MOUNT_PARK_PARKED_ITEM_NAME, "Mount parked", true);
			indigo_init_switch_item(MOUNT_PARK_UNPARKED_ITEM, MOUNT_PARK_UNPARKED_ITEM_NAME, "Mount unparked", false);
			// -------------------------------------------------------------------------------- MOUNT_PARK_SET
			MOUNT_PARK_SET_PROPERTY = indigo_init_switch_property(NULL, device->name, MOUNT_PARK_SET_PROPERTY_NAME, MOUNT_MAIN_GROUP, "Set park position", INDIGO_OK_STATE, INDIGO_RW_PERM, INDIGO_AT_MOST_ONE_RULE, 2);
			if (MOUNT_PARK_SET_PROPERTY == NULL) {
				return INDIGO_FAILED;
			}
			MOUNT_PARK_SET_PROPERTY->hidden = true;
			indigo_init_switch_item(MOUNT_PARK_SET_CURRENT_ITEM, MOUNT_PARK_SET_CURRENT_ITEM_NAME, "Set current position", false);
			indigo_init_switch_item(MOUNT_PARK_SET_DEFAULT_ITEM, MOUNT_PARK_SET_DEFAULT_ITEM_NAME, "Set default position", false);
			// -------------------------------------------------------------------------------- MOUNT_PARK_POSITION
			MOUNT_PARK_POSITION_PROPERTY = indigo_init_number_property(NULL, device->name, MOUNT_PARK_POSITION_PROPERTY_NAME, MOUNT_MAIN_GROUP, "Park position", INDIGO_OK_STATE, INDIGO_RW_PERM, 2);
			if (MOUNT_PARK_POSITION_PROPERTY == NULL) {
				return INDIGO_FAILED;
			}
			MOUNT_PARK_POSITION_PROPERTY->hidden = true;
			indigo_init_sexagesimal_number_item(MOUNT_PARK_POSITION_HA_ITEM, MOUNT_PARK_POSITION_HA_ITEM_NAME, "Hour Angle (-12 to 12 hrs)", -12, 12, 0, 6);
			indigo_init_sexagesimal_number_item(MOUNT_PARK_POSITION_DEC_ITEM, MOUNT_PARK_POSITION_DEC_ITEM_NAME, "Declination (-90 to 90°)", -90, 90, 0, 90);
			// -------------------------------------------------------------------------------- MOUNT_HOME
			MOUNT_HOME_PROPERTY = indigo_init_switch_property(NULL, device->name, MOUNT_HOME_PROPERTY_NAME, MOUNT_MAIN_GROUP, "Home", INDIGO_OK_STATE, INDIGO_RW_PERM, INDIGO_ONE_OF_MANY_RULE, 1);
			if (MOUNT_HOME_PROPERTY == NULL) {
				return INDIGO_FAILED;
			}
			MOUNT_HOME_PROPERTY->hidden = true;
			indigo_init_switch_item(MOUNT_HOME_ITEM, MOUNT_HOME_ITEM_NAME, "Slew to home position and stop", false);
			// -------------------------------------------------------------------------------- MOUNT_HOME_SET
			MOUNT_HOME_SET_PROPERTY = indigo_init_switch_property(NULL, device->name, MOUNT_HOME_SET_PROPERTY_NAME, MOUNT_MAIN_GROUP, "Set home position", INDIGO_OK_STATE, INDIGO_RW_PERM, INDIGO_AT_MOST_ONE_RULE, 2);
			if (MOUNT_HOME_SET_PROPERTY == NULL) {
				return INDIGO_FAILED;
			}
			MOUNT_HOME_SET_PROPERTY->hidden = true;
			indigo_init_switch_item(MOUNT_HOME_SET_CURRENT_ITEM, MOUNT_HOME_SET_CURRENT_ITEM_NAME, "Set current position", false);
			indigo_init_switch_item(MOUNT_HOME_SET_DEFAULT_ITEM, MOUNT_HOME_SET_DEFAULT_ITEM_NAME, "Set default position", false);
			// -------------------------------------------------------------------------------- MOUNT_HOME_POSITION
			MOUNT_HOME_POSITION_PROPERTY = indigo_init_number_property(NULL, device->name, MOUNT_HOME_POSITION_PROPERTY_NAME, MOUNT_MAIN_GROUP, "Home position", INDIGO_OK_STATE, INDIGO_RW_PERM, 2);
			if (MOUNT_HOME_POSITION_PROPERTY == NULL) {
				return INDIGO_FAILED;
			}
			MOUNT_HOME_POSITION_PROPERTY->hidden = true;
			indigo_init_sexagesimal_number_item(MOUNT_HOME_POSITION_HA_ITEM, MOUNT_HOME_POSITION_HA_ITEM_NAME, "Hour Angle (-12 to 12 hrs)", -12, 12, 0, 6);
			indigo_init_sexagesimal_number_item(MOUNT_HOME_POSITION_DEC_ITEM, MOUNT_HOME_POSITION_DEC_ITEM_NAME, "Declination (-90 to 90°)", -90, 90, 0, 90);
			// -------------------------------------------------------------------------------- MOUNT_ON_COORDINATES_SET
			MOUNT_ON_COORDINATES_SET_PROPERTY = indigo_init_switch_property(NULL, device->name,MOUNT_ON_COORDINATES_SET_PROPERTY_NAME, MOUNT_MAIN_GROUP, "On coordinates set", INDIGO_OK_STATE, INDIGO_RW_PERM, INDIGO_ONE_OF_MANY_RULE, 3);
			if (MOUNT_ON_COORDINATES_SET_PROPERTY == NULL) {
				return INDIGO_FAILED;
			}
			indigo_init_switch_item(MOUNT_ON_COORDINATES_SET_TRACK_ITEM, MOUNT_ON_COORDINATES_SET_TRACK_ITEM_NAME, "Slew to target and track", true);
			indigo_init_switch_item(MOUNT_ON_COORDINATES_SET_SYNC_ITEM, MOUNT_ON_COORDINATES_SET_SYNC_ITEM_NAME, "Sync to target", false);
			indigo_init_switch_item(MOUNT_ON_COORDINATES_SET_SLEW_ITEM, MOUNT_ON_COORDINATES_SET_SLEW_ITEM_NAME, "Slew to target and stop", false);
			// -------------------------------------------------------------------------------- MOUNT_SLEW_RATE
			MOUNT_SLEW_RATE_PROPERTY = indigo_init_switch_property(NULL, device->name, MOUNT_SLEW_RATE_PROPERTY_NAME, MOUNT_MAIN_GROUP, "Slew rate", INDIGO_OK_STATE, INDIGO_RW_PERM, INDIGO_ONE_OF_MANY_RULE, 4);
			if (MOUNT_SLEW_RATE_PROPERTY == NULL) {
				return INDIGO_FAILED;
			}
			indigo_init_switch_item(MOUNT_SLEW_RATE_GUIDE_ITEM, MOUNT_SLEW_RATE_GUIDE_ITEM_NAME, "Guide rate", true);
			indigo_init_switch_item(MOUNT_SLEW_RATE_CENTERING_ITEM, MOUNT_SLEW_RATE_CENTERING_ITEM_NAME, "Centering rate", false);
			indigo_init_switch_item(MOUNT_SLEW_RATE_FIND_ITEM, MOUNT_SLEW_RATE_FIND_ITEM_NAME, "Find rate", false);
			indigo_init_switch_item(MOUNT_SLEW_RATE_MAX_ITEM, MOUNT_SLEW_RATE_MAX_ITEM_NAME, "Max rate", false);
			// -------------------------------------------------------------------------------- MOUNT_MOTION_NS
			MOUNT_MOTION_DEC_PROPERTY = indigo_init_switch_property(NULL, device->name, MOUNT_MOTION_DEC_PROPERTY_NAME, MOUNT_MAIN_GROUP, "Move N/S", INDIGO_OK_STATE, INDIGO_RW_PERM, INDIGO_AT_MOST_ONE_RULE, 2);
			if (MOUNT_MOTION_DEC_PROPERTY == NULL) {
				return INDIGO_FAILED;
			}
			indigo_init_switch_item(MOUNT_MOTION_NORTH_ITEM, MOUNT_MOTION_NORTH_ITEM_NAME, "North", false);
			indigo_init_switch_item(MOUNT_MOTION_SOUTH_ITEM, MOUNT_MOTION_SOUTH_ITEM_NAME, "South", false);
			// -------------------------------------------------------------------------------- MOUNT_MOTION_WE
			MOUNT_MOTION_RA_PROPERTY = indigo_init_switch_property(NULL, device->name, MOUNT_MOTION_RA_PROPERTY_NAME, MOUNT_MAIN_GROUP, "Move W/E", INDIGO_OK_STATE, INDIGO_RW_PERM, INDIGO_AT_MOST_ONE_RULE, 2);
			if (MOUNT_MOTION_RA_PROPERTY == NULL) {
				return INDIGO_FAILED;
			}
			indigo_init_switch_item(MOUNT_MOTION_WEST_ITEM, MOUNT_MOTION_WEST_ITEM_NAME, "West", false);
			indigo_init_switch_item(MOUNT_MOTION_EAST_ITEM, MOUNT_MOTION_EAST_ITEM_NAME, "East", false);
			// -------------------------------------------------------------------------------- MOUNT_TRACK_RATE
			MOUNT_TRACK_RATE_PROPERTY = indigo_init_switch_property(NULL, device->name, MOUNT_TRACK_RATE_PROPERTY_NAME, MOUNT_MAIN_GROUP, "Track rate", INDIGO_OK_STATE, INDIGO_RW_PERM, INDIGO_ONE_OF_MANY_RULE, 5);
			if (MOUNT_TRACK_RATE_PROPERTY == NULL) {
				return INDIGO_FAILED;
			}
			indigo_init_switch_item(MOUNT_TRACK_RATE_SIDEREAL_ITEM, MOUNT_TRACK_RATE_SIDEREAL_ITEM_NAME, "Sidereal rate", true);
			indigo_init_switch_item(MOUNT_TRACK_RATE_SOLAR_ITEM, MOUNT_TRACK_RATE_SOLAR_ITEM_NAME, "Solar rate", false);
			indigo_init_switch_item(MOUNT_TRACK_RATE_LUNAR_ITEM, MOUNT_TRACK_RATE_LUNAR_ITEM_NAME, "Lunar rate", false);
			indigo_init_switch_item(MOUNT_TRACK_RATE_KING_ITEM, MOUNT_TRACK_RATE_KING_ITEM_NAME, "King rate", false);
			indigo_init_switch_item(MOUNT_TRACK_RATE_CUSTOM_ITEM, MOUNT_TRACK_RATE_CUSTOM_ITEM_NAME, "Custom rate", false);
			MOUNT_TRACK_RATE_PROPERTY->count = 3;
			// -------------------------------------------------------------------------------- MOUNT_CUSTOM_TRACKING_RATE
			MOUNT_CUSTOM_TRACKING_RATE_PROPERTY = indigo_init_number_property(NULL, device->name, MOUNT_CUSTOM_TRACKING_RATE_PROPERTY_NAME, MOUNT_MAIN_GROUP, "Custom tracking rate", INDIGO_OK_STATE, INDIGO_RW_PERM, 1);
			if (MOUNT_CUSTOM_TRACKING_RATE_PROPERTY == NULL) {
				return INDIGO_FAILED;
			}
			MOUNT_CUSTOM_TRACKING_RATE_PROPERTY->hidden = true;
			indigo_init_number_item(MOUNT_CUSTOM_TRACKING_RATE_ITEM, MOUNT_CUSTOM_TRACKING_RATE_ITEM_NAME, "Custom tracking rate", 0, 0, 0, 0);
			// -------------------------------------------------------------------------------- MOUNT_TRACKING
			MOUNT_TRACKING_PROPERTY = indigo_init_switch_property(NULL, device->name, MOUNT_TRACKING_PROPERTY_NAME, MOUNT_MAIN_GROUP, "Tracking", INDIGO_OK_STATE, INDIGO_RW_PERM, INDIGO_ONE_OF_MANY_RULE, 2);
			if (MOUNT_TRACKING_PROPERTY == NULL) {
				return INDIGO_FAILED;
			}
			indigo_init_switch_item(MOUNT_TRACKING_ON_ITEM, MOUNT_TRACKING_ON_ITEM_NAME, "Tracking", true);
			indigo_init_switch_item(MOUNT_TRACKING_OFF_ITEM, MOUNT_TRACKING_OFF_ITEM_NAME, "Stopped" , false);
			// -------------------------------------------------------------------------------- MOUNT_GUIDE_RATE
			MOUNT_GUIDE_RATE_PROPERTY = indigo_init_number_property(NULL, device->name, MOUNT_GUIDE_RATE_PROPERTY_NAME, MOUNT_MAIN_GROUP, "Guide rate", INDIGO_OK_STATE, INDIGO_RW_PERM, 2);
			if (MOUNT_GUIDE_RATE_PROPERTY == NULL) {
				return INDIGO_FAILED;
			}
			indigo_init_number_item(MOUNT_GUIDE_RATE_RA_ITEM, MOUNT_GUIDE_RATE_RA_ITEM_NAME, "Guiding rate (% of sidereal)", 1, 100, 1, 50);
			indigo_init_number_item(MOUNT_GUIDE_RATE_DEC_ITEM, MOUNT_GUIDE_RATE_DEC_ITEM_NAME, "DEC Guiding rate (% of sidereal)", 1, 100, 1, 50);
			// -------------------------------------------------------------------------------- MOUNT_EQUATORIAL_COORDINATES
			MOUNT_EQUATORIAL_COORDINATES_PROPERTY = indigo_init_number_property(NULL, device->name, MOUNT_EQUATORIAL_COORDINATES_PROPERTY_NAME, MOUNT_MAIN_GROUP, "Equatorial coordinates", INDIGO_OK_STATE, INDIGO_RW_PERM, 2);
			if (MOUNT_EQUATORIAL_COORDINATES_PROPERTY == NULL) {
				return INDIGO_FAILED;
			}
			indigo_init_sexagesimal_number_item(MOUNT_EQUATORIAL_COORDINATES_RA_ITEM, MOUNT_EQUATORIAL_COORDINATES_RA_ITEM_NAME, "Right ascension (0 to 24 hrs)", 0, 24, 0, 0);
			indigo_init_sexagesimal_number_item(MOUNT_EQUATORIAL_COORDINATES_DEC_ITEM, MOUNT_EQUATORIAL_COORDINATES_DEC_ITEM_NAME, "Declination (-90 to 90°)", -90, 90, 0, 90);
			// -------------------------------------------------------------------------------- MOUNT_HORIZONTAL_COORDINATES
			MOUNT_HORIZONTAL_COORDINATES_PROPERTY = indigo_init_number_property(NULL, device->name, MOUNT_HORIZONTAL_COORDINATES_PROPERTY_NAME, MOUNT_MAIN_GROUP, "Horizontal coordinates", INDIGO_OK_STATE, INDIGO_RO_PERM, 2);
			if (MOUNT_HORIZONTAL_COORDINATES_PROPERTY == NULL) {
				return INDIGO_FAILED;
			}
			indigo_init_sexagesimal_number_item(MOUNT_HORIZONTAL_COORDINATES_AZ_ITEM, MOUNT_HORIZONTAL_COORDINATES_AZ_ITEM_NAME, "Azimuth (0 to 360°)", 0, 360, 0, 0);
			indigo_init_sexagesimal_number_item(MOUNT_HORIZONTAL_COORDINATES_ALT_ITEM, MOUNT_HORIZONTAL_COORDINATES_ALT_ITEM_NAME, "Altitude (0 to 90°)", 0, 90, 0, 0);
			// -------------------------------------------------------------------------------- MOUNT_TARGET_INFO
			MOUNT_TARGET_INFO_PROPERTY = indigo_init_number_property(NULL, device->name, MOUNT_TARGET_INFO_PROPERTY_NAME, MOUNT_MAIN_GROUP, "Target info", INDIGO_OK_STATE, INDIGO_RO_PERM, 4);
			if (MOUNT_TARGET_INFO_PROPERTY == NULL) {
				return INDIGO_FAILED;
			}
			indigo_init_sexagesimal_number_item(MOUNT_TARGET_INFO_RISE_TIME_ITEM, MOUNT_TARGET_INFO_RISE_TIME_ITEM_NAME, "Rise time (GMT)", 0, 24, 0, 0);
			indigo_init_sexagesimal_number_item(MOUNT_TARGET_INFO_TRANSIT_TIME_ITEM, MOUNT_TARGET_INFO_TRANSIT_TIME_ITEM_NAME, "Transit time (GMT)", 0, 24, 0, 0);
			indigo_init_sexagesimal_number_item(MOUNT_TARGET_INFO_SET_TIME_ITEM, MOUNT_TARGET_INFO_SET_TIME_ITEM_NAME, "Set time (GMT)", 0, 24, 0, 0);
			indigo_init_sexagesimal_number_item(MOUNT_TARGET_INFO_TIME_TO_TRANSIT_ITEM, MOUNT_TARGET_INFO_TIME_TO_TRANSIT_ITEM_NAME, "Time to next transit (hrs)", 0, 24, 0, 0);
			// -------------------------------------------------------------------------------- MOUNT_ABORT_MOTION
			MOUNT_ABORT_MOTION_PROPERTY = indigo_init_switch_property(NULL, device->name, MOUNT_ABORT_MOTION_PROPERTY_NAME, MOUNT_MAIN_GROUP, "Abort motion", INDIGO_OK_STATE, INDIGO_RW_PERM, INDIGO_AT_MOST_ONE_RULE, 1);
			if (MOUNT_ABORT_MOTION_PROPERTY == NULL) {
				return INDIGO_FAILED;
			}
			indigo_init_switch_item(MOUNT_ABORT_MOTION_ITEM, MOUNT_ABORT_MOTION_ITEM_NAME, "Abort motion", false);
			// -------------------------------------------------------------------------------- MOUNT_ALIGNMENT_MODE
			MOUNT_ALIGNMENT_MODE_PROPERTY = indigo_init_switch_property(NULL, device->name, MOUNT_ALIGNMENT_MODE_PROPERTY_NAME, MOUNT_ALIGNMENT_GROUP, "Alignment mode", INDIGO_OK_STATE, INDIGO_RW_PERM, INDIGO_ONE_OF_MANY_RULE, 4);
			if (MOUNT_ALIGNMENT_MODE_PROPERTY == NULL) {
				return INDIGO_FAILED;
			}
			MOUNT_ALIGNMENT_MODE_PROPERTY->hidden = true;
			indigo_init_switch_item(MOUNT_ALIGNMENT_MODE_SINGLE_POINT_ITEM, MOUNT_ALIGNMENT_MODE_SINGLE_POINT_ITEM_NAME, "Single point", false);
			indigo_init_switch_item(MOUNT_ALIGNMENT_MODE_NEAREST_POINT_ITEM, MOUNT_ALIGNMENT_MODE_NEAREST_POINT_ITEM_NAME, "Nearest point", false);
			indigo_init_switch_item(MOUNT_ALIGNMENT_MODE_MULTI_POINT_ITEM, MOUNT_ALIGNMENT_MODE_MULTI_POINT_ITEM_NAME, "Multi point", false);
			indigo_init_switch_item(MOUNT_ALIGNMENT_MODE_CONTROLLER_ITEM, MOUNT_ALIGNMENT_MODE_CONTROLLER_ITEM_NAME, "Mount controller", true); // check MOUNT_ALIGNMENT_SELECT_POINTS and MOUNT_ALIGNMENT_DELETE_POINTS if default is changed
			// -------------------------------------------------------------------------------- MOUNT_RAW_COORDINATES
			MOUNT_RAW_COORDINATES_PROPERTY = indigo_init_number_property(NULL, device->name, MOUNT_RAW_COORDINATES_PROPERTY_NAME, MOUNT_ALIGNMENT_GROUP, "Raw coordinates", INDIGO_OK_STATE, INDIGO_RO_PERM, 2);
			if (MOUNT_RAW_COORDINATES_PROPERTY == NULL) {
				return INDIGO_FAILED;
			}
			MOUNT_RAW_COORDINATES_PROPERTY->hidden = true;
			indigo_init_sexagesimal_number_item(MOUNT_RAW_COORDINATES_RA_ITEM, MOUNT_RAW_COORDINATES_RA_ITEM_NAME, "Raw right ascension (0 to 24 hrs)", 0, 24, 0, 0);
			indigo_init_sexagesimal_number_item(MOUNT_RAW_COORDINATES_DEC_ITEM, MOUNT_RAW_COORDINATES_DEC_ITEM_NAME, "Raw declination (-90 to 90°)", -90, 90, 0, 90);
			// -------------------------------------------------------------------------------- MOUNT_ALIGNMENT_SELECT_POINTS
			MOUNT_ALIGNMENT_SELECT_POINTS_PROPERTY = indigo_init_switch_property(NULL, device->name, MOUNT_ALIGNMENT_SELECT_POINTS_PROPERTY_NAME, MOUNT_ALIGNMENT_GROUP, "Select alignment points", INDIGO_OK_STATE, INDIGO_RW_PERM, INDIGO_ONE_OF_MANY_RULE, MOUNT_MAX_ALIGNMENT_POINTS);
			if (MOUNT_ALIGNMENT_SELECT_POINTS_PROPERTY == NULL) {
				return INDIGO_FAILED;
			}
			MOUNT_ALIGNMENT_SELECT_POINTS_PROPERTY->hidden = MOUNT_ALIGNMENT_MODE_CONTROLLER_ITEM->sw.value;
			MOUNT_ALIGNMENT_SELECT_POINTS_PROPERTY->count = 0;
			// -------------------------------------------------------------------------------- MOUNT_ALIGNMENT_DELETE_POINTS
			MOUNT_ALIGNMENT_DELETE_POINTS_PROPERTY = indigo_init_switch_property(NULL, device->name, MOUNT_ALIGNMENT_DELETE_POINTS_PROPERTY_NAME, MOUNT_ALIGNMENT_GROUP, "Delete alignment point", INDIGO_OK_STATE, INDIGO_RW_PERM, INDIGO_ONE_OF_MANY_RULE, MOUNT_MAX_ALIGNMENT_POINTS + 1);
			if (MOUNT_ALIGNMENT_DELETE_POINTS_PROPERTY == NULL) {
				return INDIGO_FAILED;
			}
			indigo_init_switch_item(MOUNT_ALIGNMENT_DELETE_POINTS_PROPERTY->items, MOUNT_ALIGNMENT_DELETE_ALL_POINTS_ITEM_NAME, "All points", false);
			sprintf(MOUNT_ALIGNMENT_DELETE_POINTS_PROPERTY->items->hints, "warn_on_set:\"Clear all alignment points?\";");
			MOUNT_ALIGNMENT_DELETE_POINTS_PROPERTY->hidden = MOUNT_ALIGNMENT_MODE_CONTROLLER_ITEM->sw.value;
			MOUNT_ALIGNMENT_DELETE_POINTS_PROPERTY->count = 0;
			// -------------------------------------------------------------------------------- MOUNT_ALIGNMENT_RESET
			MOUNT_ALIGNMENT_RESET_PROPERTY = indigo_init_switch_property(NULL, device->name, MOUNT_ALIGNMENT_RESET_PROPERTY_NAME, MOUNT_ALIGNMENT_GROUP, "Reset alignment data", INDIGO_OK_STATE, INDIGO_RW_PERM, INDIGO_ONE_OF_MANY_RULE, 1);
			if (MOUNT_ALIGNMENT_RESET_PROPERTY == NULL) {
				return INDIGO_FAILED;
			}
			indigo_init_switch_item(MOUNT_ALIGNMENT_RESET_ITEM, MOUNT_ALIGNMENT_RESET_ITEM_NAME, "Reset", false);
			sprintf(MOUNT_ALIGNMENT_RESET_ITEM->hints, "warn_on_set:\"Reset alignment data?\";");
			MOUNT_ALIGNMENT_RESET_PROPERTY->hidden = MOUNT_ALIGNMENT_MODE_CONTROLLER_ITEM->sw.value;
			// -------------------------------------------------------------------------------- MOUNT_EPOCH
			MOUNT_EPOCH_PROPERTY = indigo_init_number_property(NULL, device->name, MOUNT_EPOCH_PROPERTY_NAME, MOUNT_ALIGNMENT_GROUP, "Current epoch", INDIGO_OK_STATE, INDIGO_RW_PERM, 1);
			if (MOUNT_EPOCH_PROPERTY == NULL) {
				return INDIGO_FAILED;
			}
			indigo_init_number_item(MOUNT_EPOCH_ITEM, MOUNT_EPOCH_ITEM_NAME, "Epoch (0, 1900-2050)", 0, 2050, 0, 2000);
			// -------------------------------------------------------------------------------- MOUNT_ALIGNMENT_MODE
			MOUNT_SIDE_OF_PIER_PROPERTY = indigo_init_switch_property(NULL, device->name, MOUNT_SIDE_OF_PIER_PROPERTY_NAME, MOUNT_MAIN_GROUP, "Side of pier", INDIGO_OK_STATE, INDIGO_RW_PERM, INDIGO_ONE_OF_MANY_RULE, 2);
			if (MOUNT_SIDE_OF_PIER_PROPERTY == NULL) {
				return INDIGO_FAILED;
			}
			MOUNT_SIDE_OF_PIER_PROPERTY->hidden = true;
			indigo_init_switch_item(MOUNT_SIDE_OF_PIER_EAST_ITEM, MOUNT_SIDE_OF_PIER_EAST_ITEM_NAME, "East", true);
			indigo_init_switch_item(MOUNT_SIDE_OF_PIER_WEST_ITEM, MOUNT_SIDE_OF_PIER_WEST_ITEM_NAME, "West", false);
			// -------------------------------------------------------------------------------- SNOOP_DEVICES
			MOUNT_SNOOP_DEVICES_PROPERTY = indigo_init_text_property(NULL, device->name, SNOOP_DEVICES_PROPERTY_NAME, MAIN_GROUP, "Snoop devices", INDIGO_OK_STATE, INDIGO_RW_PERM, 2);
			if (MOUNT_SNOOP_DEVICES_PROPERTY == NULL) {
				return INDIGO_FAILED;
			}
			indigo_init_text_item(MOUNT_SNOOP_JOYSTICK_ITEM, SNOOP_JOYSTICK_ITEM_NAME, "Joystick", "");
			indigo_init_text_item(MOUNT_SNOOP_GPS_ITEM, SNOOP_GPS_ITEM_NAME, "GPS", "");
			// -------------------------------------------------------------------------------- MOUNT_USE_PPEC
			MOUNT_PEC_PROPERTY = indigo_init_switch_property(NULL, device->name, MOUNT_PEC_PROPERTY_NAME, MOUNT_MAIN_GROUP, "Use PEC", INDIGO_OK_STATE, INDIGO_RW_PERM, INDIGO_ONE_OF_MANY_RULE, 2);
			if (MOUNT_PEC_PROPERTY == NULL) {
				return INDIGO_FAILED;
			}
			MOUNT_PEC_PROPERTY->hidden = true;
			indigo_init_switch_item(MOUNT_PEC_ENABLED_ITEM, MOUNT_PEC_ENABLED_ITEM_NAME, "Enabled", false);
			indigo_init_switch_item(MOUNT_PEC_DISABLED_ITEM, MOUNT_PEC_DISABLED_ITEM_NAME, "Disabled", true);

			// -------------------------------------------------------------------------------- MOUNT_TRAIN_PPEC
			MOUNT_PEC_TRAINING_PROPERTY = indigo_init_switch_property(NULL, device->name, MOUNT_PEC_TRAINING_PROPERTY_NAME, MOUNT_MAIN_GROUP, "Train PEC", INDIGO_OK_STATE, INDIGO_RW_PERM, INDIGO_ONE_OF_MANY_RULE, 2);
			if (MOUNT_PEC_TRAINING_PROPERTY == NULL) {
				return INDIGO_FAILED;
			}
			MOUNT_PEC_TRAINING_PROPERTY->hidden = true;
			indigo_init_switch_item(MOUNT_PEC_TRAINIG_STARTED_ITEM, MOUNT_PEC_TRAINIG_STARTED_ITEM_NAME, "Started", false);
			indigo_init_switch_item(MOUNT_PEC_TRAINIG_STOPPED_ITEM, MOUNT_PEC_TRAINIG_STOPPED_ITEM_NAME, "Stopped", true);
			// --------------------------------------------------------------------------------
			return INDIGO_OK;
		}
	}
	return INDIGO_FAILED;
}

void indigo_mount_load_alignment_points(indigo_device *device) {
	indigo_uni_handle *handle = indigo_open_config_file(device->name, 0, false, ".alignment");
	if (handle != NULL) {
		int count;
		char buffer[1024], name[INDIGO_NAME_SIZE], label[INDIGO_VALUE_SIZE];
		indigo_uni_read_line(handle, buffer, sizeof(buffer));
		sscanf(buffer, "%d", &count);
		MOUNT_CONTEXT->alignment_point_count = count;
		MOUNT_ALIGNMENT_SELECT_POINTS_PROPERTY->count = count;
		MOUNT_ALIGNMENT_DELETE_POINTS_PROPERTY->count = count > 0 ? count + 1 : 0;
		for (int i = 0; i < count; i++) {
			indigo_alignment_point *point =  MOUNT_CONTEXT->alignment_points + i;
			indigo_uni_read_line(handle, buffer, sizeof(buffer));
			point->used = false;
			sscanf(buffer, "%d %lg %lg %lg %lg %lg %d", (int *)&point->used, &point->ra, &point->dec, &point->raw_ra, &point->raw_dec, &point->lst, &point->side_of_pier);
			snprintf(name, INDIGO_NAME_SIZE, "%d", i);
			snprintf(label, INDIGO_VALUE_SIZE, "%s %s %c", indigo_dtos(point->ra, "%2d:%02d:%02d"), indigo_dtos(point->dec, "%2d:%02d:%02d"), point->side_of_pier == MOUNT_SIDE_EAST ? 'E' : 'W');
			indigo_init_switch_item(MOUNT_ALIGNMENT_SELECT_POINTS_PROPERTY->items + i, name, label, point->used);
			indigo_init_switch_item(MOUNT_ALIGNMENT_DELETE_POINTS_PROPERTY->items + i + 1, name, label, false);
		}
		indigo_uni_close(&handle);
		MOUNT_ALIGNMENT_SELECT_POINTS_PROPERTY->state = INDIGO_OK_STATE;
		indigo_update_property(device, MOUNT_ALIGNMENT_SELECT_POINTS_PROPERTY, NULL);
		MOUNT_ALIGNMENT_DELETE_POINTS_PROPERTY->state = INDIGO_OK_STATE;
		indigo_update_property(device, MOUNT_ALIGNMENT_DELETE_POINTS_PROPERTY, NULL);
	}
}

void indigo_mount_save_alignment_points(indigo_device *device) {
	indigo_uni_handle *handle = indigo_open_config_file(device->name, 0, true, ".alignment");
	if (handle != NULL) {
		int count = MOUNT_CONTEXT->alignment_point_count;
		char b1[32], b2[32], b3[32], b4[32], b5[32];
		indigo_uni_printf(handle, "%d\n", count);
		for (int i = 0; i < count; i++) {
			indigo_alignment_point *point =  MOUNT_CONTEXT->alignment_points + i;
			indigo_uni_printf(handle, "%d %s %s %s %s %s %d\n", point->used, indigo_dtoa(point->ra, b1), indigo_dtoa(point->dec, b2), indigo_dtoa(point->raw_ra, b3), indigo_dtoa(point->raw_dec, b4), indigo_dtoa(point->lst, b5), point->side_of_pier);
		}
		indigo_uni_close(&handle);
	}
}

void indigo_mount_update_alignment_points(indigo_device *device) {
	indigo_mount_save_alignment_points(device);
	char label[INDIGO_VALUE_SIZE];
	for (int i = 0; i < MOUNT_CONTEXT->alignment_point_count; i++) {
		indigo_alignment_point *point =  MOUNT_CONTEXT->alignment_points + i;
		snprintf(label, INDIGO_VALUE_SIZE, "%s %s %c", indigo_dtos(point->ra, "%2d:%02d:%02d"), indigo_dtos(point->dec, "%2d:%02d:%02d"), point->side_of_pier == MOUNT_SIDE_EAST ? 'E' : 'W');
		strcpy(MOUNT_ALIGNMENT_SELECT_POINTS_PROPERTY->items[i].label, label);
		strcpy(MOUNT_ALIGNMENT_DELETE_POINTS_PROPERTY->items[i + 1].label, label);
	}
	indigo_raw_to_translated(device, MOUNT_RAW_COORDINATES_RA_ITEM->number.value, MOUNT_RAW_COORDINATES_DEC_ITEM->number.value, &MOUNT_EQUATORIAL_COORDINATES_RA_ITEM->number.value, &MOUNT_EQUATORIAL_COORDINATES_DEC_ITEM->number.value);
	indigo_raw_to_translated(device, MOUNT_RAW_COORDINATES_RA_ITEM->number.target, MOUNT_RAW_COORDINATES_DEC_ITEM->number.target, &MOUNT_EQUATORIAL_COORDINATES_RA_ITEM->number.target, &MOUNT_EQUATORIAL_COORDINATES_DEC_ITEM->number.target);
	MOUNT_EQUATORIAL_COORDINATES_PROPERTY->state = INDIGO_OK_STATE;
	indigo_update_coordinates(device, NULL);
	indigo_delete_property(device, MOUNT_ALIGNMENT_SELECT_POINTS_PROPERTY, NULL);
	MOUNT_ALIGNMENT_SELECT_POINTS_PROPERTY->count = MOUNT_CONTEXT->alignment_point_count;
	MOUNT_ALIGNMENT_SELECT_POINTS_PROPERTY->state = INDIGO_OK_STATE;
	indigo_define_property(device, MOUNT_ALIGNMENT_SELECT_POINTS_PROPERTY, NULL);
	indigo_delete_property(device, MOUNT_ALIGNMENT_DELETE_POINTS_PROPERTY, NULL);
	MOUNT_ALIGNMENT_DELETE_POINTS_PROPERTY->count = MOUNT_CONTEXT->alignment_point_count ? MOUNT_CONTEXT->alignment_point_count + 1 : 0;
	MOUNT_ALIGNMENT_DELETE_POINTS_PROPERTY->state = INDIGO_OK_STATE;
	indigo_define_property(device, MOUNT_ALIGNMENT_DELETE_POINTS_PROPERTY, NULL);
}

indigo_result indigo_mount_enumerate_properties(indigo_device *device, indigo_client *client, indigo_property *property) {
	assert(device != NULL);
	assert(DEVICE_CONTEXT != NULL);
	if (IS_CONNECTED) {
		indigo_define_matching_property(MOUNT_INFO_PROPERTY);
		indigo_define_matching_property(MOUNT_GEOGRAPHIC_COORDINATES_PROPERTY);
		indigo_define_matching_property(MOUNT_LST_TIME_PROPERTY);
		indigo_define_matching_property(MOUNT_UTC_TIME_PROPERTY);
		indigo_define_matching_property(MOUNT_SET_HOST_TIME_PROPERTY);
		indigo_define_matching_property(MOUNT_PARK_PROPERTY);
		indigo_define_matching_property(MOUNT_PARK_SET_PROPERTY);
		indigo_define_matching_property(MOUNT_PARK_POSITION_PROPERTY);
		indigo_define_matching_property(MOUNT_HOME_PROPERTY);
		indigo_define_matching_property(MOUNT_HOME_SET_PROPERTY);
		indigo_define_matching_property(MOUNT_HOME_POSITION_PROPERTY);
		indigo_define_matching_property(MOUNT_SLEW_RATE_PROPERTY);
		indigo_define_matching_property(MOUNT_MOTION_DEC_PROPERTY);
		indigo_define_matching_property(MOUNT_MOTION_RA_PROPERTY);
		indigo_define_matching_property(MOUNT_TRACK_RATE_PROPERTY);
		indigo_define_matching_property(MOUNT_CUSTOM_TRACKING_RATE_PROPERTY);
		indigo_define_matching_property(MOUNT_TRACKING_PROPERTY);
		indigo_define_matching_property(MOUNT_GUIDE_RATE_PROPERTY);
		indigo_define_matching_property(MOUNT_ON_COORDINATES_SET_PROPERTY);
		indigo_define_matching_property(MOUNT_EQUATORIAL_COORDINATES_PROPERTY);
		indigo_define_matching_property(MOUNT_HORIZONTAL_COORDINATES_PROPERTY);
		indigo_define_matching_property(MOUNT_TARGET_INFO_PROPERTY);
		indigo_define_matching_property(MOUNT_ABORT_MOTION_PROPERTY);
		indigo_define_matching_property(MOUNT_ALIGNMENT_MODE_PROPERTY);
		indigo_define_matching_property(MOUNT_RAW_COORDINATES_PROPERTY);
		indigo_define_matching_property(MOUNT_ALIGNMENT_SELECT_POINTS_PROPERTY);
		indigo_define_matching_property(MOUNT_ALIGNMENT_DELETE_POINTS_PROPERTY);
		indigo_define_matching_property(MOUNT_ALIGNMENT_RESET_PROPERTY);
		indigo_define_matching_property(MOUNT_EPOCH_PROPERTY);
		indigo_define_matching_property(MOUNT_SIDE_OF_PIER_PROPERTY);
		indigo_define_matching_property(MOUNT_SNOOP_DEVICES_PROPERTY);
		indigo_define_matching_property(MOUNT_PEC_PROPERTY);
		indigo_define_matching_property(MOUNT_PEC_TRAINING_PROPERTY);
	}
	return indigo_device_enumerate_properties(device, client, property);
}

indigo_result indigo_mount_change_property(indigo_device *device, indigo_client *client, indigo_property *property) {
	assert(device != NULL);
	assert(DEVICE_CONTEXT != NULL);
	assert(property != NULL);
	if (indigo_property_match_changeable(CONNECTION_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- CONNECTION
		if (IS_CONNECTED) {
			indigo_mount_load_alignment_points(device);
			double ra = MOUNT_EQUATORIAL_COORDINATES_RA_ITEM->number.value;
			double dec = MOUNT_EQUATORIAL_COORDINATES_DEC_ITEM->number.value;
			indigo_j2k_to_jnow(&ra, &dec);
			indigo_radec_to_altaz(ra, dec, NULL, MOUNT_GEOGRAPHIC_COORDINATES_LATITUDE_ITEM->number.value, MOUNT_GEOGRAPHIC_COORDINATES_LONGITUDE_ITEM->number.value, MOUNT_GEOGRAPHIC_COORDINATES_ELEVATION_ITEM->number.value, &MOUNT_HORIZONTAL_COORDINATES_ALT_ITEM->number.value, &MOUNT_HORIZONTAL_COORDINATES_AZ_ITEM->number.value);
			indigo_define_property(device, MOUNT_INFO_PROPERTY, NULL);
			indigo_define_property(device, MOUNT_GEOGRAPHIC_COORDINATES_PROPERTY, NULL);
			indigo_define_property(device, MOUNT_LST_TIME_PROPERTY, NULL);
			indigo_define_property(device, MOUNT_UTC_TIME_PROPERTY, NULL);
			indigo_define_property(device, MOUNT_SET_HOST_TIME_PROPERTY, NULL);
			indigo_define_property(device, MOUNT_PARK_PROPERTY, NULL);
			indigo_define_property(device, MOUNT_PARK_SET_PROPERTY, NULL);
			indigo_define_property(device, MOUNT_PARK_POSITION_PROPERTY, NULL);
			indigo_define_property(device, MOUNT_HOME_PROPERTY, NULL);
			indigo_define_property(device, MOUNT_HOME_SET_PROPERTY, NULL);
			indigo_define_property(device, MOUNT_HOME_POSITION_PROPERTY, NULL);
			indigo_define_property(device, MOUNT_SLEW_RATE_PROPERTY, NULL);
			indigo_define_property(device, MOUNT_MOTION_DEC_PROPERTY, NULL);
			indigo_define_property(device, MOUNT_MOTION_RA_PROPERTY, NULL);
			indigo_define_property(device, MOUNT_TRACK_RATE_PROPERTY, NULL);
			indigo_define_property(device, MOUNT_CUSTOM_TRACKING_RATE_PROPERTY, NULL);
			indigo_define_property(device, MOUNT_TRACKING_PROPERTY, NULL);
			indigo_define_property(device, MOUNT_GUIDE_RATE_PROPERTY, NULL);
			indigo_define_property(device, MOUNT_ON_COORDINATES_SET_PROPERTY, NULL);
			indigo_define_property(device, MOUNT_EQUATORIAL_COORDINATES_PROPERTY, NULL);
			indigo_define_property(device, MOUNT_HORIZONTAL_COORDINATES_PROPERTY, NULL);
			indigo_define_property(device, MOUNT_TARGET_INFO_PROPERTY, NULL);
			indigo_define_property(device, MOUNT_ABORT_MOTION_PROPERTY, NULL);
			indigo_define_property(device, MOUNT_ALIGNMENT_MODE_PROPERTY, NULL);
			indigo_define_property(device, MOUNT_RAW_COORDINATES_PROPERTY, NULL);
			indigo_define_property(device, MOUNT_ALIGNMENT_SELECT_POINTS_PROPERTY, NULL);
			indigo_define_property(device, MOUNT_ALIGNMENT_DELETE_POINTS_PROPERTY, NULL);
			indigo_define_property(device, MOUNT_ALIGNMENT_RESET_PROPERTY, NULL);
			indigo_define_property(device, MOUNT_EPOCH_PROPERTY, NULL);
			indigo_define_property(device, MOUNT_SIDE_OF_PIER_PROPERTY, NULL);
			indigo_define_property(device, MOUNT_SNOOP_DEVICES_PROPERTY, NULL);
			indigo_define_property(device, MOUNT_PEC_PROPERTY, NULL);
			indigo_define_property(device, MOUNT_PEC_TRAINING_PROPERTY, NULL);
			indigo_add_snoop_rule(MOUNT_PARK_PROPERTY, MOUNT_SNOOP_JOYSTICK_ITEM->text.value, MOUNT_PARK_PROPERTY_NAME);
			indigo_add_snoop_rule(MOUNT_SLEW_RATE_PROPERTY, MOUNT_SNOOP_JOYSTICK_ITEM->text.value, MOUNT_SLEW_RATE_PROPERTY_NAME);
			indigo_add_snoop_rule(MOUNT_TRACKING_PROPERTY, MOUNT_SNOOP_JOYSTICK_ITEM->text.value, MOUNT_TRACKING_PROPERTY_NAME);
			indigo_add_snoop_rule(MOUNT_MOTION_DEC_PROPERTY, MOUNT_SNOOP_JOYSTICK_ITEM->text.value, MOUNT_MOTION_DEC_PROPERTY_NAME);
			indigo_add_snoop_rule(MOUNT_MOTION_RA_PROPERTY, MOUNT_SNOOP_JOYSTICK_ITEM->text.value, MOUNT_MOTION_RA_PROPERTY_NAME);
			indigo_add_snoop_rule(MOUNT_ABORT_MOTION_PROPERTY, MOUNT_SNOOP_JOYSTICK_ITEM->text.value, MOUNT_ABORT_MOTION_PROPERTY_NAME);
			indigo_add_snoop_rule(MOUNT_GEOGRAPHIC_COORDINATES_PROPERTY, MOUNT_SNOOP_GPS_ITEM->text.value, GEOGRAPHIC_COORDINATES_PROPERTY_NAME);
			indigo_add_snoop_rule(MOUNT_UTC_TIME_PROPERTY, MOUNT_SNOOP_GPS_ITEM->text.value, UTC_TIME_PROPERTY_NAME);
		} else {
			MOUNT_PARK_PROPERTY->state = INDIGO_OK_STATE;
			MOUNT_HOME_PROPERTY->state = INDIGO_OK_STATE;
			MOUNT_MOTION_DEC_PROPERTY->state = INDIGO_OK_STATE;
			MOUNT_MOTION_RA_PROPERTY->state = INDIGO_OK_STATE;
			MOUNT_EQUATORIAL_COORDINATES_PROPERTY->state = INDIGO_OK_STATE;
			MOUNT_HORIZONTAL_COORDINATES_PROPERTY->state = INDIGO_OK_STATE;
			MOUNT_TARGET_INFO_PROPERTY->state = INDIGO_OK_STATE;
			MOUNT_RAW_COORDINATES_PROPERTY->state = INDIGO_OK_STATE;
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
			indigo_delete_property(device, MOUNT_HOME_PROPERTY, NULL);
			indigo_delete_property(device, MOUNT_HOME_SET_PROPERTY, NULL);
			indigo_delete_property(device, MOUNT_HOME_POSITION_PROPERTY, NULL);
			indigo_delete_property(device, MOUNT_SLEW_RATE_PROPERTY, NULL);
			indigo_delete_property(device, MOUNT_MOTION_DEC_PROPERTY, NULL);
			indigo_delete_property(device, MOUNT_MOTION_RA_PROPERTY, NULL);
			indigo_delete_property(device, MOUNT_TRACK_RATE_PROPERTY, NULL);
			indigo_delete_property(device, MOUNT_CUSTOM_TRACKING_RATE_PROPERTY, NULL);
			indigo_delete_property(device, MOUNT_TRACKING_PROPERTY, NULL);
			indigo_delete_property(device, MOUNT_GUIDE_RATE_PROPERTY, NULL);
			indigo_delete_property(device, MOUNT_ON_COORDINATES_SET_PROPERTY, NULL);
			indigo_delete_property(device, MOUNT_EQUATORIAL_COORDINATES_PROPERTY, NULL);
			indigo_delete_property(device, MOUNT_HORIZONTAL_COORDINATES_PROPERTY, NULL);
			indigo_delete_property(device, MOUNT_TARGET_INFO_PROPERTY, NULL);
			indigo_delete_property(device, MOUNT_ABORT_MOTION_PROPERTY, NULL);
			indigo_delete_property(device, MOUNT_ALIGNMENT_MODE_PROPERTY, NULL);
			indigo_delete_property(device, MOUNT_RAW_COORDINATES_PROPERTY, NULL);
			indigo_delete_property(device, MOUNT_ALIGNMENT_SELECT_POINTS_PROPERTY, NULL);
			indigo_delete_property(device, MOUNT_ALIGNMENT_DELETE_POINTS_PROPERTY, NULL);
			indigo_delete_property(device, MOUNT_ALIGNMENT_RESET_PROPERTY, NULL);
			indigo_delete_property(device, MOUNT_EPOCH_PROPERTY, NULL);
			indigo_delete_property(device, MOUNT_SIDE_OF_PIER_PROPERTY, NULL);
			indigo_delete_property(device, MOUNT_SNOOP_DEVICES_PROPERTY, NULL);
			indigo_delete_property(device, MOUNT_PEC_PROPERTY, NULL);
			indigo_delete_property(device, MOUNT_PEC_TRAINING_PROPERTY, NULL);
		}
	} else if (indigo_property_match_changeable(MOUNT_GEOGRAPHIC_COORDINATES_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- MOUNT_GEOGRAPHIC_COORDINATES
		indigo_property_copy_values(MOUNT_GEOGRAPHIC_COORDINATES_PROPERTY, property, false);
		if (MOUNT_GEOGRAPHIC_COORDINATES_LONGITUDE_ITEM->number.value < 0) {
			MOUNT_GEOGRAPHIC_COORDINATES_LONGITUDE_ITEM->number.value += 360;
		}
		if (MOUNT_GEOGRAPHIC_COORDINATES_LATITUDE_ITEM->number.value < 0) {
			if (MOUNT_PARK_POSITION_DEC_ITEM->number.value == 90) {
				MOUNT_PARK_POSITION_DEC_ITEM->number.value = MOUNT_PARK_POSITION_DEC_ITEM->number.target = -90;
				indigo_update_property(device, MOUNT_PARK_POSITION_PROPERTY, NULL);
			}
			if (MOUNT_HOME_POSITION_DEC_ITEM->number.value == 90) {
				MOUNT_HOME_POSITION_DEC_ITEM->number.value = MOUNT_HOME_POSITION_DEC_ITEM->number.target = -90;
				indigo_update_property(device, MOUNT_HOME_POSITION_PROPERTY, NULL);
			}
		} else {
			if (MOUNT_PARK_POSITION_DEC_ITEM->number.value == -90) {
				MOUNT_PARK_POSITION_DEC_ITEM->number.value = MOUNT_PARK_POSITION_DEC_ITEM->number.target = 90;
				indigo_update_property(device, MOUNT_PARK_POSITION_PROPERTY, NULL);
			}
			if (MOUNT_HOME_POSITION_DEC_ITEM->number.value == -90) {
				MOUNT_HOME_POSITION_DEC_ITEM->number.value = MOUNT_HOME_POSITION_DEC_ITEM->number.target = 90;
				indigo_update_property(device, MOUNT_HOME_POSITION_PROPERTY, NULL);
			}
		}
		indigo_update_coordinates(device, NULL);
		MOUNT_GEOGRAPHIC_COORDINATES_PROPERTY->state = INDIGO_OK_STATE;
		indigo_update_property(device, MOUNT_GEOGRAPHIC_COORDINATES_PROPERTY, NULL);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(MOUNT_ON_COORDINATES_SET_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- MOUNT_ON_COORDINATES_SET
		indigo_property_copy_values(MOUNT_ON_COORDINATES_SET_PROPERTY, property, false);
		MOUNT_ON_COORDINATES_SET_PROPERTY->state = INDIGO_OK_STATE;
		indigo_update_property(device, MOUNT_ON_COORDINATES_SET_PROPERTY, NULL);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(MOUNT_TRACK_RATE_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- MOUNT_TRACK_RATE
		indigo_property_copy_values(MOUNT_TRACK_RATE_PROPERTY, property, false);
		MOUNT_TRACK_RATE_PROPERTY->state = INDIGO_OK_STATE;
		indigo_update_property(device, MOUNT_TRACK_RATE_PROPERTY, NULL);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(MOUNT_TRACKING_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- MOUNT_TRACKING
		indigo_property_copy_values(MOUNT_TRACKING_PROPERTY, property, false);
		MOUNT_TRACKING_PROPERTY->state = INDIGO_OK_STATE;
		indigo_update_property(device, MOUNT_TRACKING_PROPERTY, NULL);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(MOUNT_SLEW_RATE_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- MOUNT_SLEW_RATE
		indigo_property_copy_values(MOUNT_SLEW_RATE_PROPERTY, property, false);
		MOUNT_SLEW_RATE_PROPERTY->state = INDIGO_OK_STATE;
		indigo_update_property(device, MOUNT_SLEW_RATE_PROPERTY, NULL);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(MOUNT_GUIDE_RATE_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- MOUNT_GUIDE_RATE
		indigo_property_copy_values(MOUNT_GUIDE_RATE_PROPERTY, property, false);
		MOUNT_GUIDE_RATE_PROPERTY->state = INDIGO_OK_STATE;
		indigo_update_property(device, MOUNT_GUIDE_RATE_PROPERTY, NULL);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(MOUNT_PARK_SET_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- MOUNT_PARK_SET
		indigo_property_copy_values(MOUNT_PARK_SET_PROPERTY, property, false);
		if (MOUNT_PARK_SET_DEFAULT_ITEM->sw.value) {
			double lat = MOUNT_GEOGRAPHIC_COORDINATES_LATITUDE_ITEM->number.value;
			MOUNT_PARK_POSITION_HA_ITEM->number.value = 6;
			MOUNT_PARK_POSITION_DEC_ITEM->number.value = lat > 0 ? 90 : -90;
			MOUNT_PARK_POSITION_PROPERTY->state = INDIGO_OK_STATE;
			indigo_update_property(device, MOUNT_PARK_POSITION_PROPERTY, NULL);
			MOUNT_PARK_SET_DEFAULT_ITEM->sw.value = false;
		} else if (MOUNT_PARK_SET_CURRENT_ITEM->sw.value) {
			double lng = MOUNT_GEOGRAPHIC_COORDINATES_LONGITUDE_ITEM->number.value;
			time_t utc = indigo_get_mount_utc(device);
			MOUNT_PARK_POSITION_HA_ITEM->number.value = indigo_lst(&utc, lng) - MOUNT_EQUATORIAL_COORDINATES_RA_ITEM->number.value;
			MOUNT_PARK_POSITION_DEC_ITEM->number.value = MOUNT_EQUATORIAL_COORDINATES_DEC_ITEM->number.value;
			MOUNT_PARK_POSITION_PROPERTY->state = INDIGO_OK_STATE;
			indigo_update_property(device, MOUNT_PARK_POSITION_PROPERTY, NULL);
			MOUNT_PARK_SET_CURRENT_ITEM->sw.value = false;
		}
		MOUNT_PARK_SET_PROPERTY->state = INDIGO_OK_STATE;
		indigo_update_property(device, MOUNT_PARK_SET_PROPERTY, NULL);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(MOUNT_PARK_POSITION_PROPERTY, property)) {
			// -------------------------------------------------------------------------------- MOUNT_PARK_POSITION
		indigo_property_copy_values(MOUNT_PARK_POSITION_PROPERTY, property, false);
		MOUNT_PARK_POSITION_PROPERTY->state = INDIGO_OK_STATE;
		indigo_update_property(device, MOUNT_PARK_POSITION_PROPERTY, NULL);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(MOUNT_HOME_SET_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- MOUNT_HOME_SET
		indigo_property_copy_values(MOUNT_HOME_SET_PROPERTY, property, false);
		if (MOUNT_HOME_SET_DEFAULT_ITEM->sw.value) {
			double lat = MOUNT_GEOGRAPHIC_COORDINATES_LATITUDE_ITEM->number.value;
			MOUNT_HOME_POSITION_HA_ITEM->number.value = 6;
			MOUNT_HOME_POSITION_DEC_ITEM->number.value = lat > 0 ? 90 : -90;
			MOUNT_HOME_POSITION_PROPERTY->state = INDIGO_OK_STATE;
			indigo_update_property(device, MOUNT_HOME_POSITION_PROPERTY, NULL);
			MOUNT_HOME_SET_DEFAULT_ITEM->sw.value = false;
		} else if (MOUNT_HOME_SET_CURRENT_ITEM->sw.value) {
			double lng = MOUNT_GEOGRAPHIC_COORDINATES_LONGITUDE_ITEM->number.value;
			time_t utc = indigo_get_mount_utc(device);
			MOUNT_HOME_POSITION_HA_ITEM->number.value = indigo_lst(&utc, lng) - MOUNT_EQUATORIAL_COORDINATES_RA_ITEM->number.value;
			MOUNT_HOME_POSITION_DEC_ITEM->number.value = MOUNT_EQUATORIAL_COORDINATES_DEC_ITEM->number.value;
			MOUNT_HOME_POSITION_PROPERTY->state = INDIGO_OK_STATE;
			indigo_update_property(device, MOUNT_HOME_POSITION_PROPERTY, NULL);
			MOUNT_HOME_SET_CURRENT_ITEM->sw.value = false;
		}
		MOUNT_HOME_SET_PROPERTY->state = INDIGO_OK_STATE;
		indigo_update_property(device, MOUNT_HOME_SET_PROPERTY, NULL);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(MOUNT_HOME_POSITION_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- MOUNT_HOME_POSITION
		indigo_property_copy_values(MOUNT_HOME_POSITION_PROPERTY, property, false);
		MOUNT_HOME_POSITION_PROPERTY->state = INDIGO_OK_STATE;
		indigo_update_property(device, MOUNT_HOME_POSITION_PROPERTY, NULL);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(CONFIG_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- CONFIG
		if (indigo_switch_match(CONFIG_SAVE_ITEM, property)) {
			indigo_save_property(device, NULL, MOUNT_GEOGRAPHIC_COORDINATES_PROPERTY);
			indigo_save_property(device, NULL, MOUNT_SLEW_RATE_PROPERTY);
			indigo_save_property(device, NULL, MOUNT_TRACK_RATE_PROPERTY);
			indigo_save_property(device, NULL, MOUNT_CUSTOM_TRACKING_RATE_PROPERTY);
			indigo_save_property(device, NULL, MOUNT_GUIDE_RATE_PROPERTY);
			indigo_save_property(device, NULL, MOUNT_ALIGNMENT_MODE_PROPERTY);
			indigo_save_property(device, NULL, MOUNT_PARK_POSITION_PROPERTY);
			indigo_save_property(device, NULL, MOUNT_HOME_POSITION_PROPERTY);
			indigo_save_property(device, NULL, MOUNT_EPOCH_PROPERTY);
			indigo_save_property(device, NULL, MOUNT_PEC_PROPERTY);
			indigo_mount_save_alignment_points(device);
		} else if (indigo_switch_match(CONFIG_LOAD_ITEM, property)) {
			indigo_mount_load_alignment_points(device);
		}
	} else if (indigo_property_match_changeable(MOUNT_EQUATORIAL_COORDINATES_PROPERTY, property)) {
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
				time_t utc = indigo_get_mount_utc(device);
				point->lst = indigo_lst(&utc, MOUNT_GEOGRAPHIC_COORDINATES_LONGITUDE_ITEM->number.value);
				point->ra = MOUNT_EQUATORIAL_COORDINATES_RA_ITEM->number.value;
				point->dec = MOUNT_EQUATORIAL_COORDINATES_DEC_ITEM->number.value;
				point->raw_ra = MOUNT_RAW_COORDINATES_RA_ITEM->number.value;
				point->raw_dec = MOUNT_RAW_COORDINATES_DEC_ITEM->number.value;

				if (MOUNT_SIDE_OF_PIER_PROPERTY->hidden) {
					double ha = indigo_range24(point->lst - point->ra);
					if (ha > 12.0) {
						ha -= 24.0;
					}
					point->side_of_pier = (ha >= 0) ? MOUNT_SIDE_WEST : MOUNT_SIDE_EAST;
				} else {
					point->side_of_pier = MOUNT_SIDE_OF_PIER_EAST_ITEM->sw.value ? MOUNT_SIDE_EAST : MOUNT_SIDE_WEST;
				}

				char name[INDIGO_NAME_SIZE], label[INDIGO_VALUE_SIZE];
				snprintf(name, INDIGO_NAME_SIZE, "%d", index);
				snprintf(label, INDIGO_VALUE_SIZE, "%s %s %c", indigo_dtos(point->ra, "%2d:%02d:%02d"), indigo_dtos(point->dec, "%2d:%02d:%02d"), point->side_of_pier == MOUNT_SIDE_EAST ? 'E' : 'W');
				indigo_init_switch_item(MOUNT_ALIGNMENT_SELECT_POINTS_PROPERTY->items + index, name, label, true);
				point->used = true;

				//  Deselect other points if using single point mode
				if (MOUNT_ALIGNMENT_MODE_SINGLE_POINT_ITEM->sw.value) {
					for (int i = 0; i < MOUNT_ALIGNMENT_SELECT_POINTS_PROPERTY->count; i++) {
						MOUNT_ALIGNMENT_SELECT_POINTS_PROPERTY->items[i].sw.value = false;
						MOUNT_CONTEXT->alignment_points[i].used = false;
					}
				}

				indigo_mount_save_alignment_points(device);
				MOUNT_ALIGNMENT_SELECT_POINTS_PROPERTY->count = MOUNT_CONTEXT->alignment_point_count;
				MOUNT_ALIGNMENT_SELECT_POINTS_PROPERTY->state = INDIGO_OK_STATE;
				indigo_delete_property(device, MOUNT_ALIGNMENT_SELECT_POINTS_PROPERTY, NULL);
				indigo_define_property(device, MOUNT_ALIGNMENT_SELECT_POINTS_PROPERTY, NULL);
				indigo_init_switch_item(MOUNT_ALIGNMENT_DELETE_POINTS_PROPERTY->items + index + 1, name, label, false);
				MOUNT_ALIGNMENT_DELETE_POINTS_PROPERTY->count = MOUNT_CONTEXT->alignment_point_count + 1;
				MOUNT_ALIGNMENT_DELETE_POINTS_PROPERTY->state = INDIGO_OK_STATE;
				indigo_delete_property(device, MOUNT_ALIGNMENT_DELETE_POINTS_PROPERTY, NULL);
				indigo_define_property(device, MOUNT_ALIGNMENT_DELETE_POINTS_PROPERTY, NULL);
				MOUNT_EQUATORIAL_COORDINATES_PROPERTY->state = INDIGO_OK_STATE;
				indigo_update_coordinates(device, NULL);
			}
		}
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(MOUNT_ALIGNMENT_MODE_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- MOUNT_ALIGNMENT_MODE
		indigo_property_copy_values(MOUNT_ALIGNMENT_MODE_PROPERTY, property, false);
		indigo_delete_property(device, MOUNT_RAW_COORDINATES_PROPERTY, NULL);
		indigo_delete_property(device, MOUNT_ALIGNMENT_SELECT_POINTS_PROPERTY, NULL);
		indigo_delete_property(device, MOUNT_ALIGNMENT_DELETE_POINTS_PROPERTY, NULL);
		indigo_delete_property(device, MOUNT_ALIGNMENT_RESET_PROPERTY, NULL);
		if (MOUNT_ALIGNMENT_MODE_SINGLE_POINT_ITEM->sw.value) {
			MOUNT_RAW_COORDINATES_PROPERTY->hidden = false;
			MOUNT_ALIGNMENT_SELECT_POINTS_PROPERTY->hidden = false;
			MOUNT_ALIGNMENT_SELECT_POINTS_PROPERTY->rule = INDIGO_ONE_OF_MANY_RULE;
			MOUNT_ALIGNMENT_DELETE_POINTS_PROPERTY->hidden = false;
			MOUNT_ALIGNMENT_RESET_PROPERTY->hidden = false;
			if (strcmp(client->name, CONFIG_READER)) {
				indigo_set_switch(MOUNT_ALIGNMENT_SELECT_POINTS_PROPERTY, MOUNT_ALIGNMENT_SELECT_POINTS_PROPERTY->items + MOUNT_ALIGNMENT_SELECT_POINTS_PROPERTY->count - 1, true);
			}
		} else if (MOUNT_ALIGNMENT_MODE_NEAREST_POINT_ITEM->sw.value) {
			MOUNT_RAW_COORDINATES_PROPERTY->hidden = false;
			MOUNT_ALIGNMENT_SELECT_POINTS_PROPERTY->hidden = false;
			MOUNT_ALIGNMENT_SELECT_POINTS_PROPERTY->rule = INDIGO_ANY_OF_MANY_RULE;
			MOUNT_ALIGNMENT_DELETE_POINTS_PROPERTY->hidden = false;
			MOUNT_ALIGNMENT_RESET_PROPERTY->hidden = false;
			if (strcmp(client->name, CONFIG_READER)) {
				for (int i = 0; i < MOUNT_ALIGNMENT_SELECT_POINTS_PROPERTY->count; i++) {
					MOUNT_ALIGNMENT_SELECT_POINTS_PROPERTY->items[i].sw.value = true;
				}
			}
		} else if (MOUNT_ALIGNMENT_MODE_MULTI_POINT_ITEM->sw.value) {
			MOUNT_RAW_COORDINATES_PROPERTY->hidden = false;
			MOUNT_ALIGNMENT_SELECT_POINTS_PROPERTY->hidden = false;
			MOUNT_ALIGNMENT_SELECT_POINTS_PROPERTY->rule = INDIGO_ANY_OF_MANY_RULE;
			MOUNT_ALIGNMENT_DELETE_POINTS_PROPERTY->hidden = false;
			MOUNT_ALIGNMENT_RESET_PROPERTY->hidden = false;
			if (strcmp(client->name, CONFIG_READER)) {
				for (int i = 0; i < MOUNT_ALIGNMENT_SELECT_POINTS_PROPERTY->count; i++) {
					MOUNT_ALIGNMENT_SELECT_POINTS_PROPERTY->items[i].sw.value = true;
				}
			}
		} else {
			MOUNT_RAW_COORDINATES_PROPERTY->hidden = true;
			MOUNT_ALIGNMENT_SELECT_POINTS_PROPERTY->hidden = true;
			MOUNT_ALIGNMENT_DELETE_POINTS_PROPERTY->hidden = true;
			MOUNT_ALIGNMENT_RESET_PROPERTY->hidden = true;
		}
		MOUNT_ALIGNMENT_MODE_PROPERTY->state = INDIGO_OK_STATE;
		indigo_define_property(device, MOUNT_RAW_COORDINATES_PROPERTY, NULL);
		indigo_define_property(device, MOUNT_ALIGNMENT_SELECT_POINTS_PROPERTY, NULL);
		indigo_define_property(device, MOUNT_ALIGNMENT_DELETE_POINTS_PROPERTY, NULL);
		indigo_define_property(device, MOUNT_ALIGNMENT_RESET_PROPERTY, NULL);
		indigo_raw_to_translated(device, MOUNT_RAW_COORDINATES_RA_ITEM->number.value, MOUNT_RAW_COORDINATES_DEC_ITEM->number.value, &MOUNT_EQUATORIAL_COORDINATES_RA_ITEM->number.value, &MOUNT_EQUATORIAL_COORDINATES_DEC_ITEM->number.value);
		indigo_raw_to_translated(device, MOUNT_RAW_COORDINATES_RA_ITEM->number.target, MOUNT_RAW_COORDINATES_DEC_ITEM->number.target, &MOUNT_EQUATORIAL_COORDINATES_RA_ITEM->number.target, &MOUNT_EQUATORIAL_COORDINATES_DEC_ITEM->number.target);
		MOUNT_EQUATORIAL_COORDINATES_PROPERTY->state = INDIGO_OK_STATE;
		indigo_update_coordinates(device, NULL);
		indigo_update_property(device, MOUNT_ALIGNMENT_MODE_PROPERTY, NULL);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(MOUNT_ALIGNMENT_SELECT_POINTS_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- MOUNT_ALIGNMENT_SELECT_POINTS
		indigo_property_copy_values(MOUNT_ALIGNMENT_SELECT_POINTS_PROPERTY, property, false);
		for (int i = 0; i < MOUNT_ALIGNMENT_SELECT_POINTS_PROPERTY->count; i++) {
			int index = atoi(MOUNT_ALIGNMENT_SELECT_POINTS_PROPERTY->items[i].name);
			if (index < MOUNT_CONTEXT->alignment_point_count) {
				bool used = MOUNT_ALIGNMENT_SELECT_POINTS_PROPERTY->items[i].sw.value;
				MOUNT_CONTEXT->alignment_points[index].used = used;
			}
		}
		indigo_mount_save_alignment_points(device);
		indigo_raw_to_translated(device, MOUNT_RAW_COORDINATES_RA_ITEM->number.value, MOUNT_RAW_COORDINATES_DEC_ITEM->number.value, &MOUNT_EQUATORIAL_COORDINATES_RA_ITEM->number.value, &MOUNT_EQUATORIAL_COORDINATES_DEC_ITEM->number.value);
		indigo_raw_to_translated(device, MOUNT_RAW_COORDINATES_RA_ITEM->number.target, MOUNT_RAW_COORDINATES_DEC_ITEM->number.target, &MOUNT_EQUATORIAL_COORDINATES_RA_ITEM->number.target, &MOUNT_EQUATORIAL_COORDINATES_DEC_ITEM->number.target);
		MOUNT_EQUATORIAL_COORDINATES_PROPERTY->state = INDIGO_OK_STATE;
		indigo_update_coordinates(device, NULL);
		MOUNT_ALIGNMENT_MODE_PROPERTY->state = INDIGO_OK_STATE;
		indigo_update_property(device, MOUNT_ALIGNMENT_SELECT_POINTS_PROPERTY, NULL);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(MOUNT_ALIGNMENT_DELETE_POINTS_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- MOUNT_ALIGNMENT_DELETE_POINTS
		for (int i = 0; i < property->count; i++) {
			if (property->items[i].sw.value) {
				if (!strcmp(property->items[i].name, MOUNT_ALIGNMENT_DELETE_ALL_POINTS_ITEM_NAME)) {
					MOUNT_CONTEXT->alignment_point_count = 0;
				} else {
					int index = atoi(property->items[i].name);
					if (index < MOUNT_CONTEXT->alignment_point_count) {
						memcpy(MOUNT_CONTEXT->alignment_points + index, MOUNT_CONTEXT->alignment_points + index + 1, (MOUNT_CONTEXT->alignment_point_count - index - 1) * sizeof(indigo_alignment_point));
						MOUNT_CONTEXT->alignment_point_count--;
					}
				}
				break;
			}
		}
		indigo_mount_update_alignment_points(device);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(MOUNT_ALIGNMENT_RESET_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- MOUNT_ALIGNMENT_RESET
		indigo_property_copy_values(MOUNT_ALIGNMENT_RESET_PROPERTY, property, false);
		if (MOUNT_ALIGNMENT_RESET_ITEM->sw.value) {
			MOUNT_CONTEXT->alignment_point_count = 0;
			indigo_mount_update_alignment_points(device);
		}
		MOUNT_ALIGNMENT_RESET_PROPERTY->state = INDIGO_OK_STATE;
		MOUNT_ALIGNMENT_RESET_ITEM->sw.value = false;
		indigo_update_property(device, MOUNT_ALIGNMENT_RESET_PROPERTY, NULL);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(MOUNT_EPOCH_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- MOUNT_EPOCH
		indigo_property_copy_values(MOUNT_EPOCH_PROPERTY, property, false);
		if (MOUNT_EPOCH_ITEM->number.target != 0 && MOUNT_EPOCH_ITEM->number.target != 1900 && MOUNT_EPOCH_ITEM->number.target != 1950 && MOUNT_EPOCH_ITEM->number.target != 2000 && MOUNT_EPOCH_ITEM->number.target != 2050) {
			MOUNT_EPOCH_ITEM->number.value = MOUNT_EPOCH_ITEM->number.target = 2000;
			indigo_send_message(device, "Warning! Valid values are 0, 1900, 1950, 2000 or 2050 only, value adjusted to 2000");
		}
		MOUNT_EPOCH_PROPERTY->state = INDIGO_OK_STATE;
		indigo_update_property(device, MOUNT_EPOCH_PROPERTY, NULL);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(MOUNT_SIDE_OF_PIER_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- MOUNT_SIDE_OF_PIER_PROPERTY
		indigo_property_copy_values(MOUNT_SIDE_OF_PIER_PROPERTY, property, false);
		MOUNT_SIDE_OF_PIER_PROPERTY->state = INDIGO_OK_STATE;
		indigo_update_property(device, MOUNT_SIDE_OF_PIER_PROPERTY, NULL);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(MOUNT_CUSTOM_TRACKING_RATE_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- MOUNT_CUSTOM_TRACKING_RATE
		indigo_property_copy_values(MOUNT_CUSTOM_TRACKING_RATE_PROPERTY, property, false);
		MOUNT_CUSTOM_TRACKING_RATE_PROPERTY->state = INDIGO_OK_STATE;
		indigo_update_property(device, MOUNT_CUSTOM_TRACKING_RATE_PROPERTY, NULL);
		return INDIGO_OK;
		// -------------------------------------------------------------------------------- SNOOP_DEVICES
	} else if (indigo_property_match_changeable(MOUNT_SNOOP_DEVICES_PROPERTY, property)) {
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
	indigo_release_property(MOUNT_HOME_PROPERTY);
	indigo_release_property(MOUNT_HOME_SET_PROPERTY);
	indigo_release_property(MOUNT_HOME_POSITION_PROPERTY);
	indigo_release_property(MOUNT_SLEW_RATE_PROPERTY);
	indigo_release_property(MOUNT_MOTION_DEC_PROPERTY);
	indigo_release_property(MOUNT_MOTION_RA_PROPERTY);
	indigo_release_property(MOUNT_TRACK_RATE_PROPERTY);
	indigo_release_property(MOUNT_CUSTOM_TRACKING_RATE_PROPERTY);
	indigo_release_property(MOUNT_TRACKING_PROPERTY);
	indigo_release_property(MOUNT_GUIDE_RATE_PROPERTY);
	indigo_release_property(MOUNT_ON_COORDINATES_SET_PROPERTY);
	indigo_release_property(MOUNT_EQUATORIAL_COORDINATES_PROPERTY);
	indigo_release_property(MOUNT_HORIZONTAL_COORDINATES_PROPERTY);
	indigo_release_property(MOUNT_TARGET_INFO_PROPERTY);
	indigo_release_property(MOUNT_ABORT_MOTION_PROPERTY);
	indigo_release_property(MOUNT_ALIGNMENT_MODE_PROPERTY);
	indigo_release_property(MOUNT_EPOCH_PROPERTY);
	indigo_release_property(MOUNT_SIDE_OF_PIER_PROPERTY);
	indigo_release_property(MOUNT_RAW_COORDINATES_PROPERTY);
	indigo_release_property(MOUNT_ALIGNMENT_SELECT_POINTS_PROPERTY);
	indigo_release_property(MOUNT_ALIGNMENT_DELETE_POINTS_PROPERTY);
	indigo_release_property(MOUNT_ALIGNMENT_RESET_PROPERTY);
	indigo_release_property(MOUNT_SNOOP_DEVICES_PROPERTY);
	indigo_release_property(MOUNT_PEC_PROPERTY);
	indigo_release_property(MOUNT_PEC_TRAINING_PROPERTY);
	return indigo_device_detach(device);
}

/*
static void indigo_eq_to_encoder(indigo_device* device, double ha, double dec, int side_of_pier, double* enc_ha, double* enc_dec) {
	//  Get hemisphere
	bool south = MOUNT_GEOGRAPHIC_COORDINATES_LATITUDE_ITEM->number.value < 0;

	//  Convert DEC to Degrees of encoder rotation relative to 0 position
	//  [-90,90]
	//
	//  NORTHERN HEMISPHERE
	//
	//  DEC 90              Degrees = 90 W or 90 E
	//  DEC 75              Degrees = 105 W or 75 E
	//  DEC 45              Degrees = 135 W or 45 E
	//  DEC 20              Degrees = 160 W or 20 E
	//  DEC 0               Degrees = 180 W or 0 E
	//  DEC -20             Degrees = 200 W or 340 E
	//  DEC -45             Degrees = 225 W or 315 E
	//  DEC -75             Degrees = 255 W or 285 E
	//  DEC -90             Degrees = 270
	//
	//
	//  SOUTHERN HEMISPHERE
	//
	//  DEC -90             Degrees = 90
	//  DEC -75             Degrees = 105 E or 75 W
	//  DEC -45             Degrees = 135 E or 45 W
	//  DEC -20             Degrees = 160 E or 20 W
	//  DEC 0               Degrees = 180 E or 0 W
	//  DEC  20             Degrees = 200 E or 340 W
	//  DEC  45             Degrees = 225 E or 315 W
	//  DEC  75             Degrees = 255 E or 285 W
	//  DEC  90             Degrees = 270

	//  HA represents hours since the transit of the object
	//  < 0 means object has yet to transit
	//
	//  convert to [-12,12]
	if (ha > 12.0) {
		ha -= 24.0;
	}
	if (ha < -12.0) {
		ha += 24.0;
	}
	assert(ha >= -12.0 && ha <= 12.0);

	//  Generate the DEC positions (east or west of meridian)
	if (side_of_pier == MOUNT_SIDE_WEST) {
		*enc_dec = 180.0 - dec;   //  NORTH variant by default
		if (south) {
			if (dec < 0) {
				*enc_dec = -dec;
			} else {
				*enc_dec = 360.0 - dec;
			}
		}

		if (ha >= 0) {
			*enc_ha = ha;
		} else {
			*enc_ha = ha + 24.0;
		}
		if (south) {
			*enc_ha = 12.0 - ha;
		}
	} else {
		*enc_dec = 180.0 + dec;   //  SOUTH variant by default
		if (!south) {
			if (dec < 0) {
				*enc_dec = 360.0 + dec;
			} else {
				*enc_dec = dec;
			}
		}

		*enc_ha = ha + 12.0;
		if (south) {
			if (ha >= 0) {
				*enc_ha = 24.0 - ha;
			} else {
				*enc_ha = -ha;
			}
		}
	}

	//  Rebase the angle to optimise the DEC slew relative to HOME
	if (*enc_dec > 270.0) {
		*enc_dec -= 360.0;
	}

	//  Remove units
	*enc_ha /= 24.0;
	*enc_dec /= 360.0;
}
*/

static indigo_alignment_point* indigo_find_single_alignment_point(indigo_device* device) {
	for (int i = 0; i < MOUNT_CONTEXT->alignment_point_count; i++) {
		//  Return first used point
		indigo_alignment_point *point = MOUNT_CONTEXT->alignment_points + i;
		if (point->used) {
			return point;
		}
	}

	//  Return NULL if there are no used points
	return NULL;
}

/*
static indigo_alignment_point* indigo_nearest_alignment_point(indigo_device* device, double lst, double ra, double dec, int side_of_pier, int raw) {
	//  Compute virtual encoder angles for RA/DEC
	double enc_ra, enc_dec;
	indigo_eq_to_encoder(device, indigo_range24(lst - ra), dec, side_of_pier, &enc_ra, &enc_dec);

	//  Find nearest alignment point
	double min_d = 10.0;   //  Larger than 2.0
	indigo_alignment_point* nearest_point = 0;
	for (int i = 0; i < MOUNT_CONTEXT->alignment_point_count; i++) {
		//  Skip unused points
		indigo_alignment_point *point = MOUNT_CONTEXT->alignment_points + i;
		if (!point->used) {
			continue;
		}

		//  Compute virtual encoder angles for alignment point
		double enc_p_ra, enc_p_dec;
		if (raw) {
			indigo_eq_to_encoder(device, indigo_range24(point->lst - point->raw_ra), point->raw_dec, point->side_of_pier, &enc_p_ra, &enc_p_dec);
		} else {
			indigo_eq_to_encoder(device, indigo_range24(point->lst - point->ra), point->dec, point->side_of_pier, &enc_p_ra, &enc_p_dec);
		}
		//  FIXME!!! for closest point, GCD should be used Euclieadean distance is meaningless in spherical coordinates
		//  Compute separation of encoder angles of RA/DEC and alignment point
		//  Determine nearest point
		double delta_ra = enc_p_ra - enc_ra;
		double delta_dec = enc_p_dec - enc_dec;
		double d = (delta_ra * delta_ra) + (delta_dec * delta_dec);
		if (d < min_d) {
			nearest_point = point;
			min_d = d;
		}
	}

	//  Return nearest point
	return nearest_point;
}
*/

static indigo_alignment_point* indigo_find_nearest_alignment_point(indigo_device* device, double lst, double ra, double dec, bool raw) {
	//  Find nearest alignment point
	double sin_dec = sin(dec * DEG2RAD);
	double cos_dec = cos(dec * DEG2RAD);
	double min_d = 180.0;   //  Larger than 2.0
	indigo_alignment_point* nearest_point = 0;
	double ha = 15 * indigo_range24(lst - ra) * DEG2RAD;

	for (int i = 0; i < MOUNT_CONTEXT->alignment_point_count; i++) {
		//  Skip unused points
		indigo_alignment_point *point = MOUNT_CONTEXT->alignment_points + i;
		if (!point->used) {
			continue;
		}

		//  Compute virtual encoder angles for alignment point
		double p_ha, p_dec;
		if (raw) {
			p_ha = 15 * indigo_range24(point->lst - point->raw_ra) * DEG2RAD;
			p_dec = point->raw_dec * DEG2RAD;
		} else {
			p_ha = 15 * indigo_range24(point->lst - point->ra) * DEG2RAD;
			p_dec = point->dec * DEG2RAD;
		}
		double d = acos(sin_dec * sin(p_dec) + cos_dec * cos(p_dec) * cos(ha - p_ha)) / DEG2RAD;
		if (d < min_d) {
			nearest_point = point;
			min_d = d;
		}
	}

	//  Return nearest point
	return nearest_point;
}

//  Called to transform an observed position into a position for mount
indigo_result indigo_translated_to_raw(indigo_device *device, double ra, double dec, double *raw_ra, double *raw_dec) {
	if (MOUNT_ALIGNMENT_MODE_CONTROLLER_ITEM->sw.value) {
		*raw_ra = ra;
		*raw_dec = dec;
		return INDIGO_OK;
	} else if (MOUNT_ALIGNMENT_MODE_NEAREST_POINT_ITEM->sw.value || MOUNT_ALIGNMENT_MODE_SINGLE_POINT_ITEM->sw.value) {
		time_t utc = indigo_get_mount_utc(device);
		double lst = indigo_lst(&utc, MOUNT_GEOGRAPHIC_COORDINATES_LONGITUDE_ITEM->number.value);
		double ha = indigo_range24(lst - ra);
		if (ha > 12.0) {
			ha -= 24.0;
		}
		int side_of_pier = (ha >= 0.0) ? MOUNT_SIDE_WEST : MOUNT_SIDE_EAST;
		return indigo_translated_to_raw_with_lst(device, lst, ra, dec, side_of_pier, raw_ra, raw_dec);
	} else if (MOUNT_ALIGNMENT_MODE_MULTI_POINT_ITEM->sw.value) {

		// TBD Rumen

		return INDIGO_OK;
	}
	return INDIGO_FAILED;
}

indigo_result indigo_translated_to_raw_with_lst(indigo_device *device, double lst, double ra, double dec, int side_of_pier, double *raw_ra, double *raw_dec) {
	if (MOUNT_ALIGNMENT_MODE_CONTROLLER_ITEM->sw.value) {
		*raw_ra = ra;
		*raw_dec = dec;
		return INDIGO_OK;
	} else if (MOUNT_ALIGNMENT_MODE_NEAREST_POINT_ITEM->sw.value || MOUNT_ALIGNMENT_MODE_SINGLE_POINT_ITEM->sw.value) {
		indigo_alignment_point* point;
		if (MOUNT_ALIGNMENT_MODE_SINGLE_POINT_ITEM->sw.value) {
			point = indigo_find_single_alignment_point(device);
		} else {
		  point = indigo_find_nearest_alignment_point(device, lst, ra, dec, false);
		}
		if (point) {
			// Transform coordinates
			*raw_ra = ra + (point->raw_ra - point->ra);
			*raw_dec = dec + (point->raw_dec - point->dec);

			//**  Re-normalize coordinates to ensure they are in range
			//  RA
			if (*raw_ra < 0.0) {
				*raw_ra += 24.0;
			}
			if (*raw_ra >= 24.0) {
				*raw_ra -= 24.0;
			}

			//  DEC
			if (*raw_dec > 90.0) {
				*raw_dec = 180.0 - *raw_dec;
				*raw_ra += 12.0;
				if (*raw_ra >= 24.0) {
					*raw_ra -= 24.0;
				}
			}
			if (*raw_dec < -90.0) {
				*raw_dec = -180.0 - *raw_dec;
				*raw_ra += 12.0;
				if (*raw_ra >= 24.0) {
					*raw_ra -= 24.0;
				}
			}
		} else {
			*raw_ra = ra;
			*raw_dec = dec;
		}
		return INDIGO_OK;
	} else if (MOUNT_ALIGNMENT_MODE_MULTI_POINT_ITEM->sw.value) {

		// TBD Rumen

		return INDIGO_OK;
	}
	return INDIGO_FAILED;
}

//  Called to transform from position reported by mount to observed position
indigo_result indigo_raw_to_translated(indigo_device *device, double raw_ra, double raw_dec, double *ra, double *dec) {
	if (MOUNT_ALIGNMENT_MODE_CONTROLLER_ITEM->sw.value) {
		*ra = raw_ra;
		*dec = raw_dec;
		return INDIGO_OK;
	} else if (MOUNT_ALIGNMENT_MODE_NEAREST_POINT_ITEM->sw.value || MOUNT_ALIGNMENT_MODE_SINGLE_POINT_ITEM->sw.value) {
		time_t utc = indigo_get_mount_utc(device);
		double lst = indigo_lst(&utc, MOUNT_GEOGRAPHIC_COORDINATES_LONGITUDE_ITEM->number.value);
		double ha = indigo_range24(lst - raw_ra);
		if (ha > 12.0) {
			ha -= 24.0;
		}
		int side_of_pier = (ha >= 0.0) ? MOUNT_SIDE_WEST : MOUNT_SIDE_EAST;
		return indigo_raw_to_translated_with_lst(device, lst, raw_ra, raw_dec, side_of_pier, ra, dec);
	} else if (MOUNT_ALIGNMENT_MODE_MULTI_POINT_ITEM->sw.value) {

		// TBD Rumen

		return INDIGO_OK;
	}
	return INDIGO_FAILED;
}

//  Called to transform from position reported by mount to observed position
indigo_result indigo_raw_to_translated_with_lst(indigo_device *device, double lst, double raw_ra, double raw_dec, int side_of_pier, double *ra, double *dec) {
	if (MOUNT_ALIGNMENT_MODE_CONTROLLER_ITEM->sw.value) {
		*ra = raw_ra;
		*dec = raw_dec;
		return INDIGO_OK;
	} else if (MOUNT_ALIGNMENT_MODE_NEAREST_POINT_ITEM->sw.value || MOUNT_ALIGNMENT_MODE_SINGLE_POINT_ITEM->sw.value) {
		indigo_alignment_point* point;
		if (MOUNT_ALIGNMENT_MODE_SINGLE_POINT_ITEM->sw.value) {
			point = indigo_find_single_alignment_point(device);
		} else {
			point = indigo_find_nearest_alignment_point(device, lst, raw_ra, raw_dec, true);
		}
		if (point) {
			// Transform coordinates
			*ra = raw_ra + (point->ra - point->raw_ra);
			*dec = raw_dec + (point->dec - point->raw_dec);

			//**  Re-normalize coordinates to ensure they are in range
			//  RA
			if (*ra < 0.0) {
				*ra += 24.0;
			}
			if (*ra >= 24.0) {
				*ra -= 24.0;
			}

			//  DEC
			if (*dec > 90.0) {
				*dec = 180.0 - *dec;
				*ra += 12.0;
				if (*ra >= 24.0) {
					*ra -= 24.0;
				}
			}
			if (*dec < -90.0) {
				*dec = -180.0 - *dec;
				*ra += 12.0;
				if (*ra >= 24.0) {
					*ra -= 24.0;
				}
			}
		} else {
			*ra = raw_ra;
			*dec = raw_dec;
		}
		return INDIGO_OK;
	} else if (MOUNT_ALIGNMENT_MODE_MULTI_POINT_ITEM->sw.value) {

		// TBD Rumen

		return INDIGO_OK;
	}
	return INDIGO_FAILED;
}

time_t indigo_get_mount_utc(indigo_device *device) {
	if (MOUNT_UTC_TIME_PROPERTY->hidden == false) {
		return indigo_isogmtotime(MOUNT_UTC_ITEM->text.value);
	} else {
		return time(NULL);
	}
}

void indigo_update_coordinates(indigo_device *device, const char *message) {
	time_t utc = indigo_get_mount_utc(device);

	if (!MOUNT_GEOGRAPHIC_COORDINATES_PROPERTY->hidden && !MOUNT_HORIZONTAL_COORDINATES_PROPERTY->hidden) {
		double ra = MOUNT_EQUATORIAL_COORDINATES_RA_ITEM->number.value;
		double dec = MOUNT_EQUATORIAL_COORDINATES_DEC_ITEM->number.value;
		indigo_j2k_to_jnow(&ra, &dec);
		indigo_radec_to_altaz(ra, dec, &utc, MOUNT_GEOGRAPHIC_COORDINATES_LATITUDE_ITEM->number.value, MOUNT_GEOGRAPHIC_COORDINATES_LONGITUDE_ITEM->number.value, MOUNT_GEOGRAPHIC_COORDINATES_ELEVATION_ITEM->number.value, &MOUNT_HORIZONTAL_COORDINATES_ALT_ITEM->number.value, &MOUNT_HORIZONTAL_COORDINATES_AZ_ITEM->number.value);
		MOUNT_HORIZONTAL_COORDINATES_PROPERTY->state = MOUNT_EQUATORIAL_COORDINATES_PROPERTY->state;
		indigo_update_property(device, MOUNT_HORIZONTAL_COORDINATES_PROPERTY, NULL);

		// DEPRECATED, moved to Mount Agent and AGENT_MOUNT_DISPLAY_COORDINATES
		MOUNT_LST_TIME_ITEM->number.value = indigo_lst(&utc, MOUNT_GEOGRAPHIC_COORDINATES_LONGITUDE_ITEM->number.value);
		indigo_raise_set(
			UT2JD(utc),
			MOUNT_GEOGRAPHIC_COORDINATES_LATITUDE_ITEM->number.value,
			MOUNT_GEOGRAPHIC_COORDINATES_LONGITUDE_ITEM->number.value,
			ra,
			dec,
			&MOUNT_TARGET_INFO_RISE_TIME_ITEM->number.value,
			&MOUNT_TARGET_INFO_TRANSIT_TIME_ITEM->number.value,
			&MOUNT_TARGET_INFO_SET_TIME_ITEM->number.value
		);
		MOUNT_TARGET_INFO_TIME_TO_TRANSIT_ITEM->number.value = indigo_time_to_transit(ra, MOUNT_LST_TIME_ITEM->number.value, false);
		/*
		indigo_error(
			"time = %s, ttr = %s, raise = %s, transit = %s, set = %s",
			asctime(localtime(&utc)),
			indigo_dtos(MOUNT_TARGET_INFO_TIME_TO_TRANSIT_ITEM->number.value, NULL),
			indigo_dtos(MOUNT_TARGET_INFO_RISE_TIME_ITEM->number.value, NULL),
			indigo_dtos(MOUNT_TARGET_INFO_TRANSIT_TIME_ITEM->number.value, NULL),
			indigo_dtos(MOUNT_TARGET_INFO_SET_TIME_ITEM->number.value, NULL)
		);
		*/
	}
	indigo_update_property(device, MOUNT_TARGET_INFO_PROPERTY, NULL);
	indigo_update_property(device, MOUNT_LST_TIME_PROPERTY, NULL);
	indigo_update_property(device, MOUNT_EQUATORIAL_COORDINATES_PROPERTY, message);
}
