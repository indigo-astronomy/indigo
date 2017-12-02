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
#include <unistd.h>
#include <assert.h>
#include <string.h>
#include <errno.h>
#include <time.h>
#include <math.h>
#include <fcntl.h>
#include <sys/stat.h>

#include "indigo_dome_driver.h"
#include "indigo_novas.h"

#define SYNC_INTERAL 2.0  /* in seconds */

static void sync_timer_callback(indigo_device *device) {
	static int count = 0;
	count++;
	char msg[100];
	sprintf(msg, "Timer updated property %d", count);
	INDIGO_DRIVER_ERROR("indigo_dome_driver", "SYNC");
	//DOME_EQUATORIAL_COORDINATES_PROPERTY->state=INDIGO_BUSY_STATE;
	//DOME_EQUATORIAL_COORDINATES_RA_ITEM->number.value += 0.01;
	indigo_update_property(device, DOME_EQUATORIAL_COORDINATES_PROPERTY, msg);
	indigo_reschedule_timer(device, SYNC_INTERAL, &DOME_CONTEXT->sync_timer);
}

indigo_result indigo_dome_attach(indigo_device *device, unsigned version) {
	assert(device != NULL);
	assert(device != NULL);
	if (DOME_CONTEXT == NULL) {
		device->device_context = malloc(sizeof(indigo_dome_context));
		assert(device->device_context);
		memset(device->device_context, 0, sizeof(indigo_dome_context));
	}
	if (DOME_CONTEXT != NULL) {
		if (indigo_device_attach(device, version, INDIGO_INTERFACE_DOME) == INDIGO_OK) {
			// -------------------------------------------------------------------------------- DOME_SPEED
			DOME_SPEED_PROPERTY = indigo_init_number_property(NULL, device->name, DOME_SPEED_PROPERTY_NAME, DOME_MAIN_GROUP, "Dome speed", INDIGO_IDLE_STATE, INDIGO_RW_PERM, 1);
			if (DOME_SPEED_PROPERTY == NULL)
				return INDIGO_FAILED;
			indigo_init_number_item(DOME_SPEED_ITEM, DOME_SPEED_ITEM_NAME, "Speed", 1, 100, 1, 1);
			DOME_DIRECTION_PROPERTY = indigo_init_switch_property(NULL, device->name, DOME_DIRECTION_PROPERTY_NAME, DOME_MAIN_GROUP, "Movement direction", INDIGO_IDLE_STATE, INDIGO_RW_PERM, INDIGO_ONE_OF_MANY_RULE, 2);
			if (DOME_DIRECTION_PROPERTY == NULL)
				return INDIGO_FAILED;
			indigo_init_switch_item(DOME_DIRECTION_MOVE_CLOCKWISE_ITEM, DOME_DIRECTION_MOVE_CLOCKWISE_ITEM_NAME, "Move clockwise", true);
			indigo_init_switch_item(DOME_DIRECTION_MOVE_COUNTERCLOCKWISE_ITEM, DOME_DIRECTION_MOVE_COUNTERCLOCKWISE_ITEM_NAME, "Move counterclockwise", false);
			// -------------------------------------------------------------------------------- DOME_STEPS
			DOME_STEPS_PROPERTY = indigo_init_number_property(NULL, device->name, DOME_STEPS_PROPERTY_NAME, DOME_MAIN_GROUP, "Relative move", INDIGO_IDLE_STATE, INDIGO_RW_PERM, 1);
			if (DOME_STEPS_PROPERTY == NULL)
				return INDIGO_FAILED;
			indigo_init_number_item(DOME_STEPS_ITEM, DOME_STEPS_ITEM_NAME, "Relative move (steps/ms)", 0, 65535, 1, 0);
			// -------------------------------------------------------------------------------- DOME_EQUATORIAL_COORDINATES
			DOME_EQUATORIAL_COORDINATES_PROPERTY = indigo_init_number_property(NULL, device->name, DOME_EQUATORIAL_COORDINATES_PROPERTY_NAME, DOME_MAIN_GROUP, "Equatorial EOD coordinates", INDIGO_IDLE_STATE, INDIGO_RW_PERM, 2);
			if (DOME_EQUATORIAL_COORDINATES_PROPERTY == NULL)
				return INDIGO_FAILED;
			indigo_init_number_item(DOME_EQUATORIAL_COORDINATES_RA_ITEM, DOME_EQUATORIAL_COORDINATES_RA_ITEM_NAME, "Right ascension (0 to 24 hrs)", 0, 24, 0, 0);
			indigo_init_number_item(DOME_EQUATORIAL_COORDINATES_DEC_ITEM, DOME_EQUATORIAL_COORDINATES_DEC_ITEM_NAME, "Declination (-180 to 180°)", -180, 180, 0, 90);
			// -------------------------------------------------------------------------------- DOME_HORIZONTAL_COORDINATES
			DOME_HORIZONTAL_COORDINATES_PROPERTY = indigo_init_number_property(NULL, device->name, DOME_HORIZONTAL_COORDINATES_PROPERTY_NAME, DOME_MAIN_GROUP, "Absolute position", INDIGO_IDLE_STATE, INDIGO_RW_PERM, 2);
			if (DOME_HORIZONTAL_COORDINATES_PROPERTY == NULL)
				return INDIGO_FAILED;
			indigo_init_number_item(DOME_HORIZONTAL_COORDINATES_AZ_ITEM, DOME_HORIZONTAL_COORDINATES_AZ_ITEM_NAME, "Azimuth (0 to 360°)", 0, 360, 0, 0);
			indigo_init_number_item(DOME_HORIZONTAL_COORDINATES_ALT_ITEM, DOME_HORIZONTAL_COORDINATES_ALT_ITEM_NAME, "Altitude (0 to 90°)", 0, 90, 0, 0);
			DOME_HORIZONTAL_COORDINATES_PROPERTY->count = 1;
			// -------------------------------------------------------------------------------- DOME_SYNC
			DOME_SYNC_PROPERTY = indigo_init_number_property(NULL, device->name, DOME_SYNC_PROPERTY_NAME, DOME_MAIN_GROUP, "Synchronize with mount", INDIGO_IDLE_STATE, INDIGO_RW_PERM, 1);
			if (DOME_SYNC_PROPERTY == NULL)
				return INDIGO_FAILED;
			indigo_init_number_item(DOME_SYNC_THRESHOLD_ITEM, DOME_SYNC_THRESHOLD_ITEM_NAME, "Sync threshold (0 to 20°)", 0, 20, 0, 0);
			// -------------------------------------------------------------------------------- DOME_ABORT_MOTION
			DOME_ABORT_MOTION_PROPERTY = indigo_init_switch_property(NULL, device->name, DOME_ABORT_MOTION_PROPERTY_NAME, DOME_MAIN_GROUP, "Abort motion", INDIGO_IDLE_STATE, INDIGO_RW_PERM, INDIGO_ONE_OF_MANY_RULE, 1);
			if (DOME_ABORT_MOTION_PROPERTY == NULL)
				return INDIGO_FAILED;
			indigo_init_switch_item(DOME_ABORT_MOTION_ITEM, DOME_ABORT_MOTION_ITEM_NAME, "Abort motion", false);
			// -------------------------------------------------------------------------------- DOME_SHUTTER
			DOME_SHUTTER_PROPERTY = indigo_init_switch_property(NULL, device->name, DOME_SHUTTER_PROPERTY_NAME, DOME_MAIN_GROUP, "Shutter", INDIGO_IDLE_STATE, INDIGO_RW_PERM, INDIGO_ONE_OF_MANY_RULE, 2);
			if (DOME_SHUTTER_PROPERTY == NULL)
				return INDIGO_FAILED;
			indigo_init_switch_item(DOME_SHUTTER_OPENED_ITEM, DOME_SHUTTER_OPENED_ITEM_NAME, "Shutter opened", false);
			indigo_init_switch_item(DOME_SHUTTER_CLOSED_ITEM, DOME_SHUTTER_CLOSED_ITEM_NAME, "Shutter closed", true);
			// -------------------------------------------------------------------------------- DOME_PARK
			DOME_PARK_PROPERTY = indigo_init_switch_property(NULL, device->name, DOME_PARK_PROPERTY_NAME, DOME_MAIN_GROUP, "Park", INDIGO_IDLE_STATE, INDIGO_RW_PERM, INDIGO_ONE_OF_MANY_RULE, 2);
			if (DOME_PARK_PROPERTY == NULL)
				return INDIGO_FAILED;
			indigo_init_switch_item(DOME_PARK_PARKED_ITEM, DOME_PARK_PARKED_ITEM_NAME, "Dome parked", true);
			indigo_init_switch_item(DOME_PARK_UNPARKED_ITEM, DOME_PARK_UNPARKED_ITEM_NAME, "Dome unparked", false);
			// -------------------------------------------------------------------------------- DOME_MEASUREMENT
			DOME_DIMENSION_PROPERTY = indigo_init_number_property(NULL, device->name, DOME_DIMENSION_PROPERTY_NAME, DOME_MAIN_GROUP, "Dome dimension", INDIGO_IDLE_STATE, INDIGO_RW_PERM, 6);
			if (DOME_DIMENSION_PROPERTY == NULL)
				return INDIGO_FAILED;
			indigo_init_number_item(DOME_RADIUS_ITEM, DOME_RADIUS_ITEM_NAME, "Dome radius (m)", 0, 50, 0, 0);
			indigo_init_number_item(DOME_SHUTTER_WIDTH_ITEM, DOME_SHUTTER_WIDTH_ITEM_NAME, "Dome shutter width (m)", 0, 50, 0, 0);
			indigo_init_number_item(DOME_MOUNT_PIVOT_OFFSET_NS_ITEM, DOME_MOUNT_PIVOT_OFFSET_NS_ITEM_NAME, "Mount Pivot Offset N/S (m, +N/-S)", -30, 30, 0, 0);
			indigo_init_number_item(DOME_MOUNT_PIVOT_OFFSET_EW_ITEM, DOME_MOUNT_PIVOT_OFFSET_EW_ITEM_NAME, "Mount Pivot Offset E/W (m, +E/-W)", -30, 30, 0, 0);
			indigo_init_number_item(DOME_MOUNT_PIVOT_VERTICAL_OFFSET_ITEM, DOME_MOUNT_PIVOT_VERTICAL_OFFSET_ITEM_NAME, "Mount Pivot Vertical Offset (m)", -10, 10, 0, 0);
			indigo_init_number_item(DOME_MOUNT_PIVOT_OTA_OFFSET_ITEM, DOME_MOUNT_PIVOT_OTA_OFFSET_ITEM_NAME, "Optical axis offset from the RA axis (m)", 0, 10, 0, 0);
			// -------------------------------------------------------------------------------- DOME_GEOGRAPHIC_COORDINATES
			DOME_GEOGRAPHIC_COORDINATES_PROPERTY = indigo_init_number_property(NULL, device->name, DOME_GEOGRAPHIC_COORDINATES_PROPERTY_NAME, DOME_MAIN_GROUP, "Location", INDIGO_IDLE_STATE, INDIGO_RW_PERM, 3);
			if (DOME_GEOGRAPHIC_COORDINATES_PROPERTY == NULL)
				return INDIGO_FAILED;
			indigo_init_number_item(DOME_GEOGRAPHIC_COORDINATES_LATITUDE_ITEM, DOME_GEOGRAPHIC_COORDINATES_LATITUDE_ITEM_NAME, "Latitude (-90 to +90° +N)", -90, 90, 0, 0);
			indigo_init_number_item(DOME_GEOGRAPHIC_COORDINATES_LONGITUDE_ITEM, DOME_GEOGRAPHIC_COORDINATES_LONGITUDE_ITEM_NAME, "Longitude (0 to 360° +E)", -180, 360, 0, 0);
			indigo_init_number_item(DOME_GEOGRAPHIC_COORDINATES_ELEVATION_ITEM, DOME_GEOGRAPHIC_COORDINATES_ELEVATION_ITEM_NAME, "Elevation (m)", 0, 8000, 0, 0);
			// --------------------------------------------------------------------------------
			return INDIGO_OK;
		}
	}
	return INDIGO_FAILED;
}

indigo_result indigo_dome_enumerate_properties(indigo_device *device, indigo_client *client, indigo_property *property) {
	assert(device != NULL);
	assert(DEVICE_CONTEXT != NULL);
	indigo_result result = INDIGO_OK;
	if ((result = indigo_device_enumerate_properties(device, client, property)) == INDIGO_OK) {
		if (IS_CONNECTED) {
			if (indigo_property_match(DOME_SPEED_PROPERTY, property))
				indigo_define_property(device, DOME_SPEED_PROPERTY, NULL);
			if (indigo_property_match(DOME_DIRECTION_PROPERTY, property))
				indigo_define_property(device, DOME_DIRECTION_PROPERTY, NULL);
			if (indigo_property_match(DOME_STEPS_PROPERTY, property))
				indigo_define_property(device, DOME_STEPS_PROPERTY, NULL);
			if (indigo_property_match(DOME_EQUATORIAL_COORDINATES_PROPERTY, property))
				indigo_define_property(device, DOME_EQUATORIAL_COORDINATES_PROPERTY, NULL);
			if (indigo_property_match(DOME_HORIZONTAL_COORDINATES_PROPERTY, property))
				indigo_define_property(device, DOME_HORIZONTAL_COORDINATES_PROPERTY, NULL);
			if (indigo_property_match(DOME_SYNC_PROPERTY, property))
				indigo_define_property(device, DOME_SYNC_PROPERTY, NULL);
			if (indigo_property_match(DOME_ABORT_MOTION_PROPERTY, property))
				indigo_define_property(device, DOME_ABORT_MOTION_PROPERTY, NULL);
			if (indigo_property_match(DOME_SHUTTER_PROPERTY, property))
				indigo_define_property(device, DOME_SHUTTER_PROPERTY, NULL);
			if (indigo_property_match(DOME_PARK_PROPERTY, property))
				indigo_define_property(device, DOME_PARK_PROPERTY, NULL);
			if (indigo_property_match(DOME_DIMENSION_PROPERTY, property))
				indigo_define_property(device, DOME_DIMENSION_PROPERTY, NULL);
			if (indigo_property_match(DOME_GEOGRAPHIC_COORDINATES_PROPERTY, property))
				indigo_define_property(device, DOME_GEOGRAPHIC_COORDINATES_PROPERTY, NULL);
		}
	}
	return result;
}

indigo_result indigo_dome_change_property(indigo_device *device, indigo_client *client, indigo_property *property) {
	assert(device != NULL);
	assert(DEVICE_CONTEXT != NULL);
	assert(property != NULL);
	if (indigo_property_match(CONNECTION_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- CONNECTION
		if (IS_CONNECTED) {
			indigo_define_property(device, DOME_SPEED_PROPERTY, NULL);
			indigo_define_property(device, DOME_DIRECTION_PROPERTY, NULL);
			indigo_define_property(device, DOME_STEPS_PROPERTY, NULL);
			indigo_define_property(device, DOME_EQUATORIAL_COORDINATES_PROPERTY, NULL);
			indigo_define_property(device, DOME_HORIZONTAL_COORDINATES_PROPERTY, NULL);
			indigo_define_property(device, DOME_SYNC_PROPERTY, NULL);
			indigo_define_property(device, DOME_ABORT_MOTION_PROPERTY, NULL);
			indigo_define_property(device, DOME_SHUTTER_PROPERTY, NULL);
			indigo_define_property(device, DOME_PARK_PROPERTY, NULL);
			indigo_define_property(device, DOME_DIMENSION_PROPERTY, NULL);
			indigo_define_property(device, DOME_GEOGRAPHIC_COORDINATES_PROPERTY, NULL);
			DOME_CONTEXT->sync_timer = indigo_set_timer(device, SYNC_INTERAL, sync_timer_callback);
		} else {
			indigo_cancel_timer(device, &DOME_CONTEXT->sync_timer);
			indigo_delete_property(device, DOME_SPEED_PROPERTY, NULL);
			indigo_delete_property(device, DOME_DIRECTION_PROPERTY, NULL);
			indigo_delete_property(device, DOME_STEPS_PROPERTY, NULL);
			indigo_delete_property(device, DOME_EQUATORIAL_COORDINATES_PROPERTY, NULL);
			indigo_delete_property(device, DOME_HORIZONTAL_COORDINATES_PROPERTY, NULL);
			indigo_delete_property(device, DOME_SYNC_PROPERTY, NULL);
			indigo_delete_property(device, DOME_ABORT_MOTION_PROPERTY, NULL);
			indigo_delete_property(device, DOME_SHUTTER_PROPERTY, NULL);
			indigo_delete_property(device, DOME_PARK_PROPERTY, NULL);
			indigo_delete_property(device, DOME_DIMENSION_PROPERTY, NULL);
			indigo_delete_property(device, DOME_GEOGRAPHIC_COORDINATES_PROPERTY, NULL);
		}
		// -------------------------------------------------------------------------------- DOME_SPEED
	} else if (indigo_property_match(DOME_SPEED_PROPERTY, property)) {
		indigo_property_copy_values(DOME_SPEED_PROPERTY, property, false);
		DOME_SPEED_PROPERTY->state = INDIGO_OK_STATE;
		indigo_update_property(device, DOME_SPEED_PROPERTY, NULL);
		return INDIGO_OK;
		// -------------------------------------------------------------------------------- DOME_DIRECTION
	} else if (indigo_property_match(DOME_DIRECTION_PROPERTY, property)) {
		indigo_property_copy_values(DOME_DIRECTION_PROPERTY, property, false);
		DOME_DIRECTION_PROPERTY->state = INDIGO_OK_STATE;
		indigo_update_property(device, DOME_DIRECTION_PROPERTY, NULL);
		return INDIGO_OK;
		// -------------------------------------------------------------------------------- DOME_GEOGRAPHIC_COORDINATES
	} else if (indigo_property_match(DOME_GEOGRAPHIC_COORDINATES_PROPERTY, property)) {
		indigo_property_copy_values(DOME_GEOGRAPHIC_COORDINATES_PROPERTY, property, false);
		DOME_GEOGRAPHIC_COORDINATES_PROPERTY->state = INDIGO_OK_STATE;
		indigo_update_property(device, DOME_GEOGRAPHIC_COORDINATES_PROPERTY, NULL);
		return INDIGO_OK;
		// -------------------------------------------------------------------------------- DOME_SYNC
	} else if (indigo_property_match(DOME_SYNC_PROPERTY, property)) {
		indigo_property_copy_values(DOME_SYNC_PROPERTY, property, false);
		DOME_SYNC_PROPERTY->state = INDIGO_OK_STATE;
		indigo_update_property(device, DOME_SYNC_PROPERTY, NULL);
		return INDIGO_OK;
		// -------------------------------------------------------------------------------- DOME_DIMENSION
	} else if (indigo_property_match(DOME_DIMENSION_PROPERTY, property)) {
		indigo_property_copy_values(DOME_DIMENSION_PROPERTY, property, false);
		DOME_DIMENSION_PROPERTY->state = INDIGO_OK_STATE;
		indigo_update_property(device, DOME_DIMENSION_PROPERTY, NULL);
		return INDIGO_OK;
		// -------------------------------------------------------------------------------- CONFIG
	} else if (indigo_property_match(CONFIG_PROPERTY, property)) {
		if (indigo_switch_match(CONFIG_SAVE_ITEM, property)) {
			indigo_save_property(device, NULL, DOME_SPEED_PROPERTY);
			indigo_save_property(device, NULL, DOME_DIRECTION_PROPERTY);
			indigo_save_property(device, NULL, DOME_SYNC_PROPERTY);
		}
	}
	return indigo_device_change_property(device, client, property);
}

indigo_result indigo_dome_detach(indigo_device *device) {
	assert(device != NULL);
	indigo_release_property(DOME_SPEED_PROPERTY);
	indigo_release_property(DOME_DIRECTION_PROPERTY);
	indigo_release_property(DOME_STEPS_PROPERTY);
	indigo_release_property(DOME_EQUATORIAL_COORDINATES_PROPERTY);
	indigo_release_property(DOME_HORIZONTAL_COORDINATES_PROPERTY);
	indigo_release_property(DOME_SYNC_PROPERTY);
	indigo_release_property(DOME_ABORT_MOTION_PROPERTY);
	indigo_release_property(DOME_SHUTTER_PROPERTY);
	indigo_release_property(DOME_PARK_PROPERTY);
	indigo_release_property(DOME_DIMENSION_PROPERTY);
	indigo_release_property(DOME_GEOGRAPHIC_COORDINATES_PROPERTY);
	return indigo_device_detach(device);
}

indigo_result indigo_fix_dome_coordinates(indigo_device *device, double ra, double dec, double *alt, double *az) {
	if (!DOME_GEOGRAPHIC_COORDINATES_PROPERTY->hidden && !DOME_HORIZONTAL_COORDINATES_PROPERTY->hidden) {
		indigo_eq2hor(DOME_GEOGRAPHIC_COORDINATES_LATITUDE_ITEM->number.value, DOME_GEOGRAPHIC_COORDINATES_LONGITUDE_ITEM->number.value, DOME_GEOGRAPHIC_COORDINATES_ELEVATION_ITEM->number.value, ra, dec, alt, az);
		double lst = indigo_lst(DOME_GEOGRAPHIC_COORDINATES_LONGITUDE_ITEM->number.value);
		double ha = map24(lst - ra);
		INDIGO_DRIVER_ERROR("dome_driver","ha = %f, lst = %f", ha, lst);
		DOME_HORIZONTAL_COORDINATES_AZ_ITEM->number.value = indigo_dome_solve_azimuth (
			ha,
			dec,
			DOME_GEOGRAPHIC_COORDINATES_LATITUDE_ITEM->number.value,
			DOME_RADIUS_ITEM->number.value,
			DOME_MOUNT_PIVOT_VERTICAL_OFFSET_ITEM->number.value,
			DOME_MOUNT_PIVOT_OTA_OFFSET_ITEM->number.value,
			DOME_MOUNT_PIVOT_OFFSET_NS_ITEM->number.value,
			DOME_MOUNT_PIVOT_OFFSET_EW_ITEM->number.value
		);
		indigo_update_property(device, DOME_HORIZONTAL_COORDINATES_PROPERTY, NULL);
		return INDIGO_OK;
	}
	return INDIGO_FAILED;
}
