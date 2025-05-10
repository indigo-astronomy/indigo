// Copyright (c) 2017 CloudMakers, s. r. o.
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

/** INDIGO Dome Driver base
 \file indigo_dome_driver.c
 */

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <errno.h>
#include <time.h>
#include <math.h>
#include <sys/stat.h>

#include <indigo/indigo_dome_driver.h>
#include <indigo/indigo_agent.h>
#include <indigo/indigo_align.h>

#define SYNC_INTERAL 15.0  /* in seconds */

static indigo_client dummy_client = { "Client", false, NULL, INDIGO_OK, INDIGO_VERSION_CURRENT, NULL, NULL, NULL, NULL, NULL, NULL, NULL };

static void sync_timer_callback(indigo_device *device) {
	if (!DOME_PARK_PARKED_ITEM->sw.value) {
		indigo_change_property(&dummy_client, DOME_EQUATORIAL_COORDINATES_PROPERTY);
	}
	indigo_reschedule_timer(device, SYNC_INTERAL, &DOME_CONTEXT->sync_timer);
}

indigo_result indigo_dome_attach(indigo_device *device, const char* driver_name, unsigned version) {
	assert(device != NULL);
	assert(device != NULL);
	if (DOME_CONTEXT == NULL) {
		device->device_context = indigo_safe_malloc(sizeof(indigo_dome_context));
	}
	if (DOME_CONTEXT != NULL) {
		if (indigo_device_attach(device, driver_name, version, INDIGO_INTERFACE_DOME) == INDIGO_OK) {
			// -------------------------------------------------------------------------------- DOME_SPEED
			DOME_SPEED_PROPERTY = indigo_init_number_property(NULL, device->name, DOME_SPEED_PROPERTY_NAME, DOME_MAIN_GROUP, "Dome speed", INDIGO_OK_STATE, INDIGO_RW_PERM, 1);
			if (DOME_SPEED_PROPERTY == NULL) {
				return INDIGO_FAILED;
			}
			indigo_init_number_item(DOME_SPEED_ITEM, DOME_SPEED_ITEM_NAME, "Speed", 1, 100, 1, 1);
			// -------------------------------------------------------------------------------- DOME_DIRECTION
			DOME_DIRECTION_PROPERTY = indigo_init_switch_property(NULL, device->name, DOME_DIRECTION_PROPERTY_NAME, DOME_MAIN_GROUP, "Movement direction", INDIGO_OK_STATE, INDIGO_RW_PERM, INDIGO_ONE_OF_MANY_RULE, 2);
			if (DOME_DIRECTION_PROPERTY == NULL) {
				return INDIGO_FAILED;
			}
			indigo_init_switch_item(DOME_DIRECTION_MOVE_CLOCKWISE_ITEM, DOME_DIRECTION_MOVE_CLOCKWISE_ITEM_NAME, "Move clockwise", true);
			indigo_init_switch_item(DOME_DIRECTION_MOVE_COUNTERCLOCKWISE_ITEM, DOME_DIRECTION_MOVE_COUNTERCLOCKWISE_ITEM_NAME, "Move counterclockwise", false);
			// -------------------------------------------------------------------------------- DOME_ON_HORIZONTAL_COORDINATES_SET
			DOME_ON_HORIZONTAL_COORDINATES_SET_PROPERTY = indigo_init_switch_property(NULL, device->name, DOME_ON_HORIZONTAL_COORDINATES_SET_PROPERTY_NAME, DOME_MAIN_GROUP, "On absolute position set", INDIGO_OK_STATE, INDIGO_RW_PERM, INDIGO_ONE_OF_MANY_RULE, 2);
			if (DOME_ON_HORIZONTAL_COORDINATES_SET_PROPERTY == NULL) {
				return INDIGO_FAILED;
			}
			DOME_ON_HORIZONTAL_COORDINATES_SET_PROPERTY->hidden = true;
			indigo_init_switch_item(DOME_ON_HORIZONTAL_COORDINATES_SET_GOTO_ITEM, DOME_ON_HORIZONTAL_COORDINATES_SET_GOTO_ITEM_NAME, "Go to position", true);
			indigo_init_switch_item(DOME_ON_HORIZONTAL_COORDINATES_SET_SYNC_ITEM, DOME_ON_HORIZONTAL_COORDINATES_SET_SYNC_ITEM_NAME, "Sync to position", false);
			// -------------------------------------------------------------------------------- DOME_STEPS
			DOME_STEPS_PROPERTY = indigo_init_number_property(NULL, device->name, DOME_STEPS_PROPERTY_NAME, DOME_MAIN_GROUP, "Relative move", INDIGO_OK_STATE, INDIGO_RW_PERM, 1);
			if (DOME_STEPS_PROPERTY == NULL) {
				return INDIGO_FAILED;
			}
			indigo_init_number_item(DOME_STEPS_ITEM, DOME_STEPS_ITEM_NAME, "Relative move (steps/ms)", 0, 65535, 1, 0);
			// -------------------------------------------------------------------------------- DOME_EQUATORIAL_COORDINATES
			DOME_EQUATORIAL_COORDINATES_PROPERTY = indigo_init_number_property(NULL, device->name, DOME_EQUATORIAL_COORDINATES_PROPERTY_NAME, DOME_MAIN_GROUP, "Equatorial coordinates", INDIGO_OK_STATE, INDIGO_RW_PERM, 2);
			if (DOME_EQUATORIAL_COORDINATES_PROPERTY == NULL) {
				return INDIGO_FAILED;
			}
			indigo_init_sexagesimal_number_item(DOME_EQUATORIAL_COORDINATES_RA_ITEM, DOME_EQUATORIAL_COORDINATES_RA_ITEM_NAME, "Right ascension (0 to 24 hrs)", 0, 24, 0, 0);
			indigo_init_sexagesimal_number_item(DOME_EQUATORIAL_COORDINATES_DEC_ITEM, DOME_EQUATORIAL_COORDINATES_DEC_ITEM_NAME, "Declination (-90 to 90°)", -90, 90, 0, 90);
			// -------------------------------------------------------------------------------- DOME_HORIZONTAL_COORDINATES
			DOME_HORIZONTAL_COORDINATES_PROPERTY = indigo_init_number_property(NULL, device->name, DOME_HORIZONTAL_COORDINATES_PROPERTY_NAME, DOME_MAIN_GROUP, "Absolute position", INDIGO_OK_STATE, INDIGO_RW_PERM, 2);
			if (DOME_HORIZONTAL_COORDINATES_PROPERTY == NULL) {
				return INDIGO_FAILED;
			}
			indigo_init_sexagesimal_number_item(DOME_HORIZONTAL_COORDINATES_AZ_ITEM, DOME_HORIZONTAL_COORDINATES_AZ_ITEM_NAME, "Azimuth (0 to 360°)", 0, 360, 0, 0);
			indigo_init_sexagesimal_number_item(DOME_HORIZONTAL_COORDINATES_ALT_ITEM, DOME_HORIZONTAL_COORDINATES_ALT_ITEM_NAME, "Altitude (0 to 90°)", 0, 90, 0, 0);
			DOME_HORIZONTAL_COORDINATES_PROPERTY->count = 1;
			// -------------------------------------------------------------------------------- DOME_SLAVING
			DOME_SLAVING_PROPERTY = indigo_init_switch_property(NULL, device->name, DOME_SLAVING_PROPERTY_NAME, DOME_MAIN_GROUP, "Slave dome to mount", INDIGO_OK_STATE, INDIGO_RW_PERM, INDIGO_ONE_OF_MANY_RULE, 2);;
			if (DOME_SLAVING_PROPERTY == NULL) {
				return INDIGO_FAILED;
			}
			indigo_init_switch_item(DOME_SLAVING_ENABLE_ITEM, DOME_SLAVING_ENABLE_ITEM_NAME, "Enable", false);
			indigo_init_switch_item(DOME_SLAVING_DISABLE_ITEM, DOME_SLAVING_DISABLE_ITEM_NAME, "Disable", true);
			// -------------------------------------------------------------------------------- DOME_SYNC
			DOME_SLAVING_PARAMETERS_PROPERTY = indigo_init_number_property(NULL, device->name, DOME_SLAVING_PARAMETERS_PROPERTY_NAME, DOME_MAIN_GROUP, "Slaving parameteres", INDIGO_OK_STATE, INDIGO_RW_PERM, 1);
			if (DOME_SLAVING_PARAMETERS_PROPERTY == NULL) {
				return INDIGO_FAILED;
			}
			DOME_SLAVING_PARAMETERS_PROPERTY->hidden = true;
			indigo_init_number_item(DOME_SLAVING_THRESHOLD_ITEM, DOME_SLAVING_THRESHOLD_ITEM_NAME, "Minimal move threshold (0 to 20°)", 0, 20, 0, 1);
			// -------------------------------------------------------------------------------- DOME_ABORT_MOTION
			DOME_ABORT_MOTION_PROPERTY = indigo_init_switch_property(NULL, device->name, DOME_ABORT_MOTION_PROPERTY_NAME, DOME_MAIN_GROUP, "Abort motion", INDIGO_OK_STATE, INDIGO_RW_PERM, INDIGO_AT_MOST_ONE_RULE, 1);
			if (DOME_ABORT_MOTION_PROPERTY == NULL) {
				return INDIGO_FAILED;
			}
			indigo_init_switch_item(DOME_ABORT_MOTION_ITEM, DOME_ABORT_MOTION_ITEM_NAME, "Abort motion", false);
			// -------------------------------------------------------------------------------- DOME_SHUTTER
			DOME_SHUTTER_PROPERTY = indigo_init_switch_property(NULL, device->name, DOME_SHUTTER_PROPERTY_NAME, DOME_MAIN_GROUP, "Shutter", INDIGO_OK_STATE, INDIGO_RW_PERM, INDIGO_ONE_OF_MANY_RULE, 2);
			if (DOME_SHUTTER_PROPERTY == NULL) {
				return INDIGO_FAILED;
			}
			indigo_init_switch_item(DOME_SHUTTER_CLOSED_ITEM, DOME_SHUTTER_CLOSED_ITEM_NAME, "Shutter closed", true);
			indigo_init_switch_item(DOME_SHUTTER_OPENED_ITEM, DOME_SHUTTER_OPENED_ITEM_NAME, "Shutter opened", false);
			// -------------------------------------------------------------------------------- DOME_FLAP
			DOME_FLAP_PROPERTY = indigo_init_switch_property(NULL, device->name, DOME_FLAP_PROPERTY_NAME, DOME_MAIN_GROUP, "Flap", INDIGO_OK_STATE, INDIGO_RW_PERM, INDIGO_ONE_OF_MANY_RULE, 2);
			if (DOME_FLAP_PROPERTY == NULL) {
				return INDIGO_FAILED;
			}
			DOME_FLAP_PROPERTY->hidden = true;
			indigo_init_switch_item(DOME_FLAP_CLOSED_ITEM, DOME_FLAP_CLOSED_ITEM_NAME, "Flap closed", true);
			indigo_init_switch_item(DOME_FLAP_OPENED_ITEM, DOME_FLAP_OPENED_ITEM_NAME, "Flap opened", false);
			// -------------------------------------------------------------------------------- DOME_PARK
			DOME_PARK_PROPERTY = indigo_init_switch_property(NULL, device->name, DOME_PARK_PROPERTY_NAME, DOME_MAIN_GROUP, "Park", INDIGO_OK_STATE, INDIGO_RW_PERM, INDIGO_ONE_OF_MANY_RULE, 2);
			if (DOME_PARK_PROPERTY == NULL) {
				return INDIGO_FAILED;
			}
			indigo_init_switch_item(DOME_PARK_PARKED_ITEM, DOME_PARK_PARKED_ITEM_NAME, "Dome parked", true);
			indigo_init_switch_item(DOME_PARK_UNPARKED_ITEM, DOME_PARK_UNPARKED_ITEM_NAME, "Dome unparked", false);
			// -------------------------------------------------------------------------------- DOME_PARK_POSITION
			DOME_PARK_POSITION_PROPERTY = indigo_init_number_property(NULL, device->name, DOME_PARK_POSITION_PROPERTY_NAME, DOME_MAIN_GROUP, "Park position", INDIGO_OK_STATE, INDIGO_RW_PERM, 2);
			if (DOME_PARK_POSITION_PROPERTY == NULL) {
				return INDIGO_FAILED;
			}
			DOME_PARK_POSITION_PROPERTY->hidden = true;
			indigo_init_sexagesimal_number_item(DOME_PARK_POSITION_AZ_ITEM, DOME_PARK_POSITION_AZ_ITEM_NAME, "Azimuth (0 to 360°)", 0, 360, 0, 0);
			indigo_init_sexagesimal_number_item(DOME_PARK_POSITION_ALT_ITEM, DOME_PARK_POSITION_ALT_ITEM_NAME, "Altitude (0 to 90°)", 0, 90, 0, 0);
			DOME_PARK_POSITION_PROPERTY->count = 1;
			// -------------------------------------------------------------------------------- DOME_HOME
			DOME_HOME_PROPERTY = indigo_init_switch_property(NULL, device->name, DOME_HOME_PROPERTY_NAME, DOME_MAIN_GROUP, "Home", INDIGO_OK_STATE, INDIGO_RW_PERM, INDIGO_AT_MOST_ONE_RULE, 1);
			if (DOME_HOME_PROPERTY == NULL) {
				return INDIGO_FAILED;
			}
			DOME_HOME_PROPERTY->hidden = true;
			indigo_init_switch_item(DOME_HOME_ITEM, DOME_HOME_ITEM_NAME, "Go to home position", false);
			// -------------------------------------------------------------------------------- DOME_MEASUREMENT
			DOME_DIMENSION_PROPERTY = indigo_init_number_property(NULL, device->name, DOME_DIMENSION_PROPERTY_NAME, DOME_MAIN_GROUP, "Dome dimension", INDIGO_OK_STATE, INDIGO_RW_PERM, 6);
			if (DOME_DIMENSION_PROPERTY == NULL) {
				return INDIGO_FAILED;
			}
			indigo_init_number_item(DOME_RADIUS_ITEM, DOME_RADIUS_ITEM_NAME, "Dome radius (m)", 0.1, 50, 0, 0.1);
			indigo_init_number_item(DOME_SHUTTER_WIDTH_ITEM, DOME_SHUTTER_WIDTH_ITEM_NAME, "Dome shutter width (m)", 0.1, 50, 0, 0.1);
			indigo_init_number_item(DOME_MOUNT_PIVOT_OFFSET_NS_ITEM, DOME_MOUNT_PIVOT_OFFSET_NS_ITEM_NAME, "Mount Pivot Offset N/S (m, +N/-S)", -30, 30, 0, 0);
			indigo_init_number_item(DOME_MOUNT_PIVOT_OFFSET_EW_ITEM, DOME_MOUNT_PIVOT_OFFSET_EW_ITEM_NAME, "Mount Pivot Offset E/W (m, +E/-W)", -30, 30, 0, 0);
			indigo_init_number_item(DOME_MOUNT_PIVOT_VERTICAL_OFFSET_ITEM, DOME_MOUNT_PIVOT_VERTICAL_OFFSET_ITEM_NAME, "Mount Pivot Vertical Offset (m)", -10, 10, 0, 0);
			indigo_init_number_item(DOME_MOUNT_PIVOT_OTA_OFFSET_ITEM, DOME_MOUNT_PIVOT_OTA_OFFSET_ITEM_NAME, "Optical axis offset from the RA axis (m)", 0, 10, 0, 0);
			// -------------------------------------------------------------------------------- DOME_GEOGRAPHIC_COORDINATES
			DOME_GEOGRAPHIC_COORDINATES_PROPERTY = indigo_init_number_property(NULL, device->name, GEOGRAPHIC_COORDINATES_PROPERTY_NAME, DOME_SITE_GROUP, "Location", INDIGO_OK_STATE, INDIGO_RW_PERM, 3);
			if (DOME_GEOGRAPHIC_COORDINATES_PROPERTY == NULL) {
				return INDIGO_FAILED;
			}
			indigo_init_sexagesimal_number_item(DOME_GEOGRAPHIC_COORDINATES_LATITUDE_ITEM, GEOGRAPHIC_COORDINATES_LATITUDE_ITEM_NAME, "Latitude (-90 to +90° +N)", -90, 90, 0, 0);
			indigo_init_sexagesimal_number_item(DOME_GEOGRAPHIC_COORDINATES_LONGITUDE_ITEM, GEOGRAPHIC_COORDINATES_LONGITUDE_ITEM_NAME, "Longitude (0 to 360° +E)", -180, 360, 0, 0);
			indigo_init_number_item(DOME_GEOGRAPHIC_COORDINATES_ELEVATION_ITEM, GEOGRAPHIC_COORDINATES_ELEVATION_ITEM_NAME, "Elevation (m)", 0, 8000, 0, 0);
			// --------------------------------------------------------------------------------DOME_UTC_TIME
			DOME_UTC_TIME_PROPERTY = indigo_init_text_property(NULL, device->name, UTC_TIME_PROPERTY_NAME, DOME_SITE_GROUP, "UTC time", INDIGO_OK_STATE, INDIGO_RW_PERM, 2);
			if (DOME_UTC_TIME_PROPERTY == NULL) {
			return INDIGO_FAILED;
		}
			DOME_UTC_TIME_PROPERTY->hidden = true;
			indigo_init_text_item(DOME_UTC_ITEM, UTC_TIME_ITEM_NAME, "UTC Time", "0000-00-00T00:00:00");
			indigo_init_text_item(DOME_UTC_OFFSET_ITEM, UTC_OFFSET_ITEM_NAME, "UTC Offset", "0"); /* step is 0.5 as there are timezones at 30 min */
			// -------------------------------------------------------------------------------- DOME_UTC_FROM_HOST
			DOME_SET_HOST_TIME_PROPERTY = indigo_init_switch_property(NULL, device->name, DOME_SET_HOST_TIME_PROPERTY_NAME, DOME_SITE_GROUP, "Set UTC", INDIGO_OK_STATE, INDIGO_RW_PERM, INDIGO_AT_MOST_ONE_RULE, 1);
			if (DOME_SET_HOST_TIME_PROPERTY == NULL) {
				return INDIGO_FAILED;
			}
			DOME_SET_HOST_TIME_PROPERTY->hidden = true;
			indigo_init_switch_item(DOME_SET_HOST_TIME_ITEM, DOME_SET_HOST_TIME_ITEM_NAME, "From host", false);
			// -------------------------------------------------------------------------------- SNOOP_DEVICES
			DOME_SNOOP_DEVICES_PROPERTY = indigo_init_text_property(NULL, device->name, SNOOP_DEVICES_PROPERTY_NAME, MAIN_GROUP, "Snoop devices", INDIGO_OK_STATE, INDIGO_RW_PERM, 2);
			if (DOME_SNOOP_DEVICES_PROPERTY == NULL) {
				return INDIGO_FAILED;
			}
			indigo_init_text_item(DOME_SNOOP_MOUNT_ITEM, SNOOP_MOUNT_ITEM_NAME, "Mount", "");
			indigo_init_text_item(DOME_SNOOP_GPS_ITEM, SNOOP_GPS_ITEM_NAME, "GPS", "");
			// --------------------------------------------------------------------------------
			return INDIGO_OK;
		}
	}
	return INDIGO_FAILED;
}

indigo_result indigo_dome_enumerate_properties(indigo_device *device, indigo_client *client, indigo_property *property) {
	assert(device != NULL);
	assert(DEVICE_CONTEXT != NULL);
	if (IS_CONNECTED) {
		INDIGO_DEFINE_MATCHING_PROPERTY(DOME_SPEED_PROPERTY);
		INDIGO_DEFINE_MATCHING_PROPERTY(DOME_DIRECTION_PROPERTY);
		INDIGO_DEFINE_MATCHING_PROPERTY(DOME_ON_HORIZONTAL_COORDINATES_SET_PROPERTY);
		INDIGO_DEFINE_MATCHING_PROPERTY(DOME_STEPS_PROPERTY);
		INDIGO_DEFINE_MATCHING_PROPERTY(DOME_EQUATORIAL_COORDINATES_PROPERTY);
		INDIGO_DEFINE_MATCHING_PROPERTY(DOME_HORIZONTAL_COORDINATES_PROPERTY);
		INDIGO_DEFINE_MATCHING_PROPERTY(DOME_SLAVING_PROPERTY);
		INDIGO_DEFINE_MATCHING_PROPERTY(DOME_SLAVING_PARAMETERS_PROPERTY);
		INDIGO_DEFINE_MATCHING_PROPERTY(DOME_ABORT_MOTION_PROPERTY);
		INDIGO_DEFINE_MATCHING_PROPERTY(DOME_SHUTTER_PROPERTY);
		INDIGO_DEFINE_MATCHING_PROPERTY(DOME_FLAP_PROPERTY);
		INDIGO_DEFINE_MATCHING_PROPERTY(DOME_PARK_PROPERTY);
		INDIGO_DEFINE_MATCHING_PROPERTY(DOME_PARK_POSITION_PROPERTY);
		INDIGO_DEFINE_MATCHING_PROPERTY(DOME_HOME_PROPERTY);
		INDIGO_DEFINE_MATCHING_PROPERTY(DOME_DIMENSION_PROPERTY);
		INDIGO_DEFINE_MATCHING_PROPERTY(DOME_GEOGRAPHIC_COORDINATES_PROPERTY);
		INDIGO_DEFINE_MATCHING_PROPERTY(DOME_UTC_TIME_PROPERTY);
		INDIGO_DEFINE_MATCHING_PROPERTY(DOME_SET_HOST_TIME_PROPERTY);
		INDIGO_DEFINE_MATCHING_PROPERTY(DOME_SNOOP_DEVICES_PROPERTY);
	}
	return indigo_device_enumerate_properties(device, client, property);
}

indigo_result indigo_dome_change_property(indigo_device *device, indigo_client *client, indigo_property *property) {
	assert(device != NULL);
	assert(DEVICE_CONTEXT != NULL);
	assert(property != NULL);
	if (indigo_property_match_changeable(CONNECTION_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- CONNECTION
		if (IS_CONNECTED) {
			indigo_define_property(device, DOME_SPEED_PROPERTY, NULL);
			indigo_define_property(device, DOME_DIRECTION_PROPERTY, NULL);
			indigo_define_property(device, DOME_ON_HORIZONTAL_COORDINATES_SET_PROPERTY, NULL);
			indigo_define_property(device, DOME_STEPS_PROPERTY, NULL);
			indigo_define_property(device, DOME_EQUATORIAL_COORDINATES_PROPERTY, NULL);
			indigo_define_property(device, DOME_HORIZONTAL_COORDINATES_PROPERTY, NULL);
			indigo_define_property(device, DOME_SLAVING_PROPERTY, NULL);
			indigo_define_property(device, DOME_SLAVING_PARAMETERS_PROPERTY, NULL);
			indigo_define_property(device, DOME_ABORT_MOTION_PROPERTY, NULL);
			indigo_define_property(device, DOME_SHUTTER_PROPERTY, NULL);
			indigo_define_property(device, DOME_FLAP_PROPERTY, NULL);
			indigo_define_property(device, DOME_PARK_PROPERTY, NULL);
			indigo_define_property(device, DOME_PARK_POSITION_PROPERTY, NULL);
			indigo_define_property(device, DOME_HOME_PROPERTY, NULL);
			indigo_define_property(device, DOME_DIMENSION_PROPERTY, NULL);
			indigo_define_property(device, DOME_GEOGRAPHIC_COORDINATES_PROPERTY, NULL);
			indigo_define_property(device, DOME_UTC_TIME_PROPERTY, NULL);
			indigo_define_property(device, DOME_SET_HOST_TIME_PROPERTY, NULL);
			indigo_define_property(device, DOME_SNOOP_DEVICES_PROPERTY, NULL);
			if (DOME_SLAVING_ENABLE_ITEM->sw.value) {
				indigo_add_snoop_rule(DOME_EQUATORIAL_COORDINATES_PROPERTY, DOME_SNOOP_MOUNT_ITEM->text.value, MOUNT_EQUATORIAL_COORDINATES_PROPERTY_NAME);
				indigo_add_snoop_rule(DOME_GEOGRAPHIC_COORDINATES_PROPERTY, DOME_SNOOP_GPS_ITEM->text.value, GEOGRAPHIC_COORDINATES_PROPERTY_NAME);
			}
			indigo_set_timer(device, SYNC_INTERAL, sync_timer_callback, &DOME_CONTEXT->sync_timer);
		} else {
			indigo_cancel_timer(device, &DOME_CONTEXT->sync_timer);
			DOME_STEPS_PROPERTY->state = INDIGO_OK_STATE;
			DOME_EQUATORIAL_COORDINATES_PROPERTY->state = INDIGO_OK_STATE;
			DOME_HORIZONTAL_COORDINATES_PROPERTY->state = INDIGO_OK_STATE;
			DOME_SHUTTER_PROPERTY->state = INDIGO_OK_STATE;
			DOME_FLAP_PROPERTY->state = INDIGO_OK_STATE;
			DOME_PARK_PROPERTY->state = INDIGO_OK_STATE;
			DOME_PARK_POSITION_PROPERTY->state = INDIGO_OK_STATE;
			DOME_HOME_PROPERTY->state = INDIGO_OK_STATE;
			indigo_remove_snoop_rule(DOME_EQUATORIAL_COORDINATES_PROPERTY, DOME_SNOOP_MOUNT_ITEM->text.value, MOUNT_EQUATORIAL_COORDINATES_PROPERTY_NAME);
			indigo_remove_snoop_rule(DOME_GEOGRAPHIC_COORDINATES_PROPERTY, DOME_SNOOP_GPS_ITEM->text.value, GEOGRAPHIC_COORDINATES_PROPERTY_NAME);
			indigo_delete_property(device, DOME_SPEED_PROPERTY, NULL);
			indigo_delete_property(device, DOME_DIRECTION_PROPERTY, NULL);
			indigo_delete_property(device, DOME_ON_HORIZONTAL_COORDINATES_SET_PROPERTY, NULL);
			indigo_delete_property(device, DOME_STEPS_PROPERTY, NULL);
			indigo_delete_property(device, DOME_EQUATORIAL_COORDINATES_PROPERTY, NULL);
			indigo_delete_property(device, DOME_HORIZONTAL_COORDINATES_PROPERTY, NULL);
			indigo_delete_property(device, DOME_SLAVING_PROPERTY, NULL);
			indigo_delete_property(device, DOME_SLAVING_PARAMETERS_PROPERTY, NULL);
			indigo_delete_property(device, DOME_ABORT_MOTION_PROPERTY, NULL);
			indigo_delete_property(device, DOME_SHUTTER_PROPERTY, NULL);
			indigo_delete_property(device, DOME_FLAP_PROPERTY, NULL);
			indigo_delete_property(device, DOME_PARK_PROPERTY, NULL);
			indigo_delete_property(device, DOME_PARK_POSITION_PROPERTY, NULL);
			indigo_delete_property(device, DOME_HOME_PROPERTY, NULL);
			indigo_delete_property(device, DOME_DIMENSION_PROPERTY, NULL);
			indigo_delete_property(device, DOME_GEOGRAPHIC_COORDINATES_PROPERTY, NULL);
			indigo_delete_property(device, DOME_UTC_TIME_PROPERTY, NULL);
			indigo_delete_property(device, DOME_SET_HOST_TIME_PROPERTY, NULL);
			indigo_delete_property(device, DOME_SNOOP_DEVICES_PROPERTY, NULL);
		}
		// -------------------------------------------------------------------------------- DOME_SPEED
	} else if (indigo_property_match_changeable(DOME_SPEED_PROPERTY, property)) {
		indigo_property_copy_values(DOME_SPEED_PROPERTY, property, false);
		DOME_SPEED_PROPERTY->state = INDIGO_OK_STATE;
		indigo_update_property(device, DOME_SPEED_PROPERTY, NULL);
		return INDIGO_OK;
		// -------------------------------------------------------------------------------- DOME_DIRECTION
	} else if (indigo_property_match_changeable(DOME_DIRECTION_PROPERTY, property)) {
		indigo_property_copy_values(DOME_DIRECTION_PROPERTY, property, false);
		DOME_DIRECTION_PROPERTY->state = INDIGO_OK_STATE;
		indigo_update_property(device, DOME_DIRECTION_PROPERTY, NULL);
		return INDIGO_OK;
		// -------------------------------------------------------------------------------- DOME_ON_HORIZONTAL_COORDINATES_SET
	} else if (indigo_property_match_changeable(DOME_ON_HORIZONTAL_COORDINATES_SET_PROPERTY, property)) {
		indigo_property_copy_values(DOME_ON_HORIZONTAL_COORDINATES_SET_PROPERTY, property, false);
		DOME_ON_HORIZONTAL_COORDINATES_SET_PROPERTY->state = INDIGO_OK_STATE;
		if (DOME_SLAVING_ENABLE_ITEM->sw.value) {
			DOME_ON_HORIZONTAL_COORDINATES_SET_PROPERTY->state = INDIGO_ALERT_STATE;
			indigo_set_switch(DOME_ON_HORIZONTAL_COORDINATES_SET_PROPERTY, DOME_ON_HORIZONTAL_COORDINATES_SET_GOTO_ITEM, true);
			indigo_update_property(device, DOME_ON_HORIZONTAL_COORDINATES_SET_PROPERTY, "Can not SYNC position while folowing the mount.");
		} else {
			indigo_update_property(device, DOME_ON_HORIZONTAL_COORDINATES_SET_PROPERTY, NULL);
		}
		return INDIGO_OK;
		// -------------------------------------------------------------------------------- DOME_GEOGRAPHIC_COORDINATES
	} else if (indigo_property_match_changeable(DOME_GEOGRAPHIC_COORDINATES_PROPERTY, property)) {
		indigo_property_copy_values(DOME_GEOGRAPHIC_COORDINATES_PROPERTY, property, false);
		DOME_GEOGRAPHIC_COORDINATES_PROPERTY->state = INDIGO_OK_STATE;
		indigo_update_property(device, DOME_GEOGRAPHIC_COORDINATES_PROPERTY, NULL);
		return INDIGO_OK;
		// -------------------------------------------------------------------------------- DOME_SLAVING
	} else if (indigo_property_match_changeable(DOME_SLAVING_PROPERTY, property)) {
		indigo_property_copy_values(DOME_SLAVING_PROPERTY, property, false);
		DOME_SLAVING_PROPERTY->state = INDIGO_OK_STATE;
		indigo_remove_snoop_rule(DOME_EQUATORIAL_COORDINATES_PROPERTY, DOME_SNOOP_MOUNT_ITEM->text.value, MOUNT_EQUATORIAL_COORDINATES_PROPERTY_NAME);
		indigo_remove_snoop_rule(DOME_GEOGRAPHIC_COORDINATES_PROPERTY, DOME_SNOOP_GPS_ITEM->text.value, GEOGRAPHIC_COORDINATES_PROPERTY_NAME);
		if (DOME_SLAVING_ENABLE_ITEM->sw.value) {
			if (!DOME_ON_HORIZONTAL_COORDINATES_SET_PROPERTY->hidden && !DOME_ON_HORIZONTAL_COORDINATES_SET_GOTO_ITEM->sw.value) {
				DOME_ON_HORIZONTAL_COORDINATES_SET_PROPERTY->state = INDIGO_OK_STATE;
				indigo_set_switch(DOME_ON_HORIZONTAL_COORDINATES_SET_PROPERTY, DOME_ON_HORIZONTAL_COORDINATES_SET_GOTO_ITEM, true);
				indigo_update_property(device, DOME_ON_HORIZONTAL_COORDINATES_SET_PROPERTY, "Switching to GOTO mode." );
			}
			indigo_add_snoop_rule(DOME_EQUATORIAL_COORDINATES_PROPERTY, DOME_SNOOP_MOUNT_ITEM->text.value, MOUNT_EQUATORIAL_COORDINATES_PROPERTY_NAME);
			indigo_add_snoop_rule(DOME_GEOGRAPHIC_COORDINATES_PROPERTY, DOME_SNOOP_GPS_ITEM->text.value, GEOGRAPHIC_COORDINATES_PROPERTY_NAME);
		}
		indigo_update_property(device, DOME_SLAVING_PROPERTY, NULL);
		return INDIGO_OK;
		// -------------------------------------------------------------------------------- DOME_SLAVING_PARAMETERS
	} else if (indigo_property_match_changeable(DOME_SLAVING_PARAMETERS_PROPERTY, property)) {
		indigo_property_copy_values(DOME_SLAVING_PARAMETERS_PROPERTY, property, false);
		DOME_SLAVING_PARAMETERS_PROPERTY->state = INDIGO_OK_STATE;
		indigo_update_property(device, DOME_SLAVING_PARAMETERS_PROPERTY, NULL);
		return INDIGO_OK;
		// -------------------------------------------------------------------------------- DOME_DIMENSION
	} else if (indigo_property_match_changeable(DOME_DIMENSION_PROPERTY, property)) {
		indigo_property_copy_values(DOME_DIMENSION_PROPERTY, property, false);
		DOME_DIMENSION_PROPERTY->state = INDIGO_OK_STATE;
		indigo_update_property(device, DOME_DIMENSION_PROPERTY, NULL);
		return INDIGO_OK;
		// -------------------------------------------------------------------------------- CONFIG
	} else if (indigo_property_match_changeable(CONFIG_PROPERTY, property)) {
		if (indigo_switch_match(CONFIG_SAVE_ITEM, property)) {
			indigo_save_property(device, NULL, DOME_SPEED_PROPERTY);
			indigo_save_property(device, NULL, DOME_DIRECTION_PROPERTY);
			indigo_save_property(device, NULL, DOME_SLAVING_PROPERTY);
			indigo_save_property(device, NULL, DOME_SLAVING_PARAMETERS_PROPERTY);
			indigo_save_property(device, NULL, DOME_GEOGRAPHIC_COORDINATES_PROPERTY);
			indigo_save_property(device, NULL, DOME_DIMENSION_PROPERTY);
		}
		// -------------------------------------------------------------------------------- SNOOP_DEVICES
	} else if (indigo_property_match_changeable(DOME_SNOOP_DEVICES_PROPERTY, property)) {
		indigo_remove_snoop_rule(DOME_EQUATORIAL_COORDINATES_PROPERTY, DOME_SNOOP_MOUNT_ITEM->text.value, MOUNT_EQUATORIAL_COORDINATES_PROPERTY_NAME);
		indigo_remove_snoop_rule(DOME_GEOGRAPHIC_COORDINATES_PROPERTY, DOME_SNOOP_GPS_ITEM->text.value, GEOGRAPHIC_COORDINATES_PROPERTY_NAME);
		indigo_property_copy_values(DOME_SNOOP_DEVICES_PROPERTY, property, false);
		indigo_trim_local_service(DOME_SNOOP_MOUNT_ITEM->text.value);
		indigo_trim_local_service(DOME_SNOOP_GPS_ITEM->text.value);
		indigo_add_snoop_rule(DOME_EQUATORIAL_COORDINATES_PROPERTY, DOME_SNOOP_MOUNT_ITEM->text.value, MOUNT_EQUATORIAL_COORDINATES_PROPERTY_NAME);
		indigo_add_snoop_rule(DOME_GEOGRAPHIC_COORDINATES_PROPERTY, DOME_SNOOP_GPS_ITEM->text.value, GEOGRAPHIC_COORDINATES_PROPERTY_NAME);
		DOME_SNOOP_DEVICES_PROPERTY->state = INDIGO_OK_STATE;
		indigo_update_property(device, DOME_SNOOP_DEVICES_PROPERTY, NULL);
	}
	return indigo_device_change_property(device, client, property);
}

indigo_result indigo_dome_detach(indigo_device *device) {
	assert(device != NULL);
	indigo_release_property(DOME_SPEED_PROPERTY);
	indigo_release_property(DOME_DIRECTION_PROPERTY);
	indigo_release_property(DOME_ON_HORIZONTAL_COORDINATES_SET_PROPERTY);
	indigo_release_property(DOME_STEPS_PROPERTY);
	indigo_release_property(DOME_EQUATORIAL_COORDINATES_PROPERTY);
	indigo_release_property(DOME_HORIZONTAL_COORDINATES_PROPERTY);
	indigo_release_property(DOME_SLAVING_PROPERTY);
	indigo_release_property(DOME_SLAVING_PARAMETERS_PROPERTY);
	indigo_release_property(DOME_ABORT_MOTION_PROPERTY);
	indigo_release_property(DOME_SHUTTER_PROPERTY);
	indigo_release_property(DOME_FLAP_PROPERTY);
	indigo_release_property(DOME_PARK_PROPERTY);
	indigo_release_property(DOME_PARK_POSITION_PROPERTY);
	indigo_release_property(DOME_HOME_PROPERTY);
	indigo_release_property(DOME_DIMENSION_PROPERTY);
	indigo_release_property(DOME_GEOGRAPHIC_COORDINATES_PROPERTY);
	indigo_release_property(DOME_UTC_TIME_PROPERTY);
	indigo_release_property(DOME_SET_HOST_TIME_PROPERTY);
	indigo_release_property(DOME_SNOOP_DEVICES_PROPERTY);
	return indigo_device_detach(device);
}

time_t indigo_get_dome_utc(indigo_device *device) {
	if (DOME_UTC_TIME_PROPERTY->hidden == false) {
		return indigo_isogmtotime(DOME_UTC_ITEM->text.value);
	} else {
		return time(NULL);
	}
}

bool indigo_fix_dome_azimuth(indigo_device *device, double ra, double dec, double az_prev, double *az) {
	bool update_needed = false;
	if (!DOME_GEOGRAPHIC_COORDINATES_PROPERTY->hidden && !DOME_HORIZONTAL_COORDINATES_PROPERTY->hidden) {
		double threshold = DOME_SLAVING_THRESHOLD_ITEM->number.value;
		time_t utc = indigo_get_dome_utc(device);
		double lst = indigo_lst(&utc, DOME_GEOGRAPHIC_COORDINATES_LONGITUDE_ITEM->number.value);
		double ha = map24(lst - ra);
		*az = indigo_dome_solve_azimuth (
			ha,
			dec,
			DOME_GEOGRAPHIC_COORDINATES_LATITUDE_ITEM->number.value,
			DOME_RADIUS_ITEM->number.value,
			DOME_MOUNT_PIVOT_VERTICAL_OFFSET_ITEM->number.value,
			DOME_MOUNT_PIVOT_OTA_OFFSET_ITEM->number.value,
			DOME_MOUNT_PIVOT_OFFSET_NS_ITEM->number.value,
			DOME_MOUNT_PIVOT_OFFSET_EW_ITEM->number.value
		);
		double diff = indigo_azimuth_distance(az_prev, *az);
		if (diff >= threshold) {
			INDIGO_DRIVER_DEBUG("dome_driver", "Update dome Az diff = %.4f, threshold = %.4f", diff, threshold);
			update_needed = true;
		} else {
			INDIGO_DRIVER_DEBUG("dome_driver", "No dome Az update needed diff = %.4f, threshold = %.4f", diff, threshold);
			update_needed = false;
		}
		*az = round(*az * 100) / 100;
		INDIGO_DRIVER_DEBUG("dome_driver","ha = %.5f, lst = %.5f, dec = %.5f, az = %.4f, az_prev = %.4f", ha, lst, dec, *az, az_prev);
	}
	return update_needed;
}
