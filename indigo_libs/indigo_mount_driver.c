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

indigo_result indigo_mount_attach(indigo_device *device, indigo_version version) {
	assert(device != NULL);
	assert(device != NULL);
	if (MOUNT_CONTEXT == NULL) {
		device->device_context = malloc(sizeof(indigo_mount_context));
		assert(device->device_context);
		memset(device->device_context, 0, sizeof(indigo_mount_context));
	}
	if (MOUNT_CONTEXT != NULL) {
		if (indigo_device_attach(device, version, INDIGO_INTERFACE_MOUNT) == INDIGO_OK) {
			// -------------------------------------------------------------------------------- MOUNT_GEOGRAPHIC_COORDINATES
			MOUNT_GEOGRAPHIC_COORDINATES_PROPERTY = indigo_init_number_property(NULL, device->name, "MOUNT_GEOGRAPHIC_COORDINATES", MOUNT_SITE_GROUP, "Geographical coordinates", INDIGO_IDLE_STATE, INDIGO_RW_PERM, 3);
			if (MOUNT_GEOGRAPHIC_COORDINATES_PROPERTY == NULL)
				return INDIGO_FAILED;
			indigo_init_number_item(MOUNT_GEOGRAPHIC_COORDINATES_LATITUDE_ITEM, "LATITUDE", "Site latitude (-90 to +90° +N)", -90, 90, 0, 0);
			indigo_init_number_item(MOUNT_GEOGRAPHIC_COORDINATES_LONGITUDE_ITEM, "LONGITUDE", "Site longitude (0 to 360° +E)", 0, 360, 0, 0);
			indigo_init_number_item(MOUNT_GEOGRAPHIC_COORDINATES_ELEVATION_ITEM, "ELEVATION", "Site elevation, meters", 0, 8000, 0, 0);
			// -------------------------------------------------------------------------------- MOUNT_LST_TIME
			MOUNT_LST_TIME_PROPERTY = indigo_init_number_property(NULL, device->name, "MOUNT_LST_TIME", MOUNT_SITE_GROUP, "Local sidereal time", INDIGO_IDLE_STATE, INDIGO_RW_PERM, 3);
			if (MOUNT_LST_TIME_PROPERTY == NULL)
				return INDIGO_FAILED;
			MOUNT_LST_TIME_PROPERTY->hidden = true;
			indigo_init_number_item(MOUNT_LST_TIME_ITEM, "TIME", "LST Time", 0, 24, 0, 0);
			// -------------------------------------------------------------------------------- MOUNT_PARK
			MOUNT_PARK_PROPERTY = indigo_init_switch_property(NULL, device->name, "MOUNT_PARK", MOUNT_MAIN_GROUP, "Park", INDIGO_IDLE_STATE, INDIGO_RW_PERM, INDIGO_ONE_OF_MANY_RULE, 2);
			if (MOUNT_PARK_PROPERTY == NULL)
				return INDIGO_FAILED;
			indigo_init_switch_item(MOUNT_PARK_PARKED_ITEM, "PARKED", "Mount parked", true);
			indigo_init_switch_item(MOUNT_PARK_UNPARKED_ITEM, "UNPARKED", "Mount unparked", false);
			// -------------------------------------------------------------------------------- MOUNT_ON_COORDINATES_SET
			MOUNT_ON_COORDINATES_SET_PROPERTY = indigo_init_switch_property(NULL, device->name, "MOUNT_ON_COORDINATES_SET", MOUNT_MAIN_GROUP, "On coordinates set", INDIGO_IDLE_STATE, INDIGO_RW_PERM, INDIGO_ONE_OF_MANY_RULE, 3);
			if (MOUNT_ON_COORDINATES_SET_PROPERTY == NULL)
				return INDIGO_FAILED;
			indigo_init_switch_item(MOUNT_ON_COORDINATES_SET_TRACK_ITEM, "TRACK", "Slew to target and track", true);
			indigo_init_switch_item(MOUNT_ON_COORDINATES_SET_SYNC_ITEM, "SYNC", "Sync to target", false);
			indigo_init_switch_item(MOUNT_ON_COORDINATES_SET_SLEW_ITEM, "SLEW", "Slew to target and stop", false);
			// -------------------------------------------------------------------------------- MOUNT_SLEW_RATE
			MOUNT_SLEW_RATE_PROPERTY = indigo_init_switch_property(NULL, device->name, "MOUNT_SLEW_RATE", MOUNT_MAIN_GROUP, "Slew rate", INDIGO_IDLE_STATE, INDIGO_RW_PERM, INDIGO_ONE_OF_MANY_RULE, 4);
			if (MOUNT_SLEW_RATE_PROPERTY == NULL)
				return INDIGO_FAILED;
			indigo_init_switch_item(MOUNT_SLEW_RATE_GUIDE_ITEM, "GUIDE", "Guide rate", true);
			indigo_init_switch_item(MOUNT_SLEW_RATE_CENTERING_ITEM, "CENTERING", "Centering rate", false);
			indigo_init_switch_item(MOUNT_SLEW_RATE_FIND_ITEM, "FIND", "Find rate", false);
			indigo_init_switch_item(MOUNT_SLEW_RATE_MAX_ITEM, "MAX", "Max rate", false);
			// -------------------------------------------------------------------------------- MOUNT_EQUATORIAL_COORDINATES
			MOUNT_EQUATORIAL_COORDINATES_PROPERTY = indigo_init_number_property(NULL, device->name, "MOUNT_EQUATORIAL_COORDINATES", MOUNT_MAIN_GROUP, "Equatorial EOD coordinates", INDIGO_IDLE_STATE, INDIGO_RW_PERM, 2);
			if (MOUNT_EQUATORIAL_COORDINATES_PROPERTY == NULL)
				return INDIGO_FAILED;
			indigo_init_number_item(MOUNT_EQUATORIAL_COORDINATES_RA_ITEM, "RA", "Right ascension (0 to 24 hrs)", 0, 24, 0, 0);
			indigo_init_number_item(MOUNT_EQUATORIAL_COORDINATES_DEC_ITEM, "DEC", "Declination (-180 to 180°)", -180, 180, 0, 90);
			// -------------------------------------------------------------------------------- MOUNT_HORIZONTAL_COORDINATES
			MOUNT_HORIZONTAL_COORDINATES_PROPERTY = indigo_init_number_property(NULL, device->name, "MOUNT_HORIZONTAL_COORDINATES", MOUNT_MAIN_GROUP, "Horizontal coordinates", INDIGO_IDLE_STATE, INDIGO_RW_PERM, 2);
			if (MOUNT_HORIZONTAL_COORDINATES_PROPERTY == NULL)
				return INDIGO_FAILED;
			MOUNT_HORIZONTAL_COORDINATES_PROPERTY->hidden = true;
			indigo_init_number_item(MOUNT_HORIZONTAL_COORDINATES_ALT_ITEM, "ALT", "Altitude (0 to 90°)", 0, 90, 0, 0);
			indigo_init_number_item(MOUNT_HORIZONTAL_COORDINATES_AZ_ITEM, "AZ", "Azimuth (0 to 360°)", 0, 360, 0, 0);
			// -------------------------------------------------------------------------------- MOUNT_ABORT_MOTION
			MOUNT_ABORT_MOTION_PROPERTY = indigo_init_switch_property(NULL, device->name, "MOUNT_ABORT_MOTION", MOUNT_MAIN_GROUP, "Abort motion", INDIGO_IDLE_STATE, INDIGO_RW_PERM, INDIGO_ONE_OF_MANY_RULE, 1);
			if (MOUNT_ABORT_MOTION_PROPERTY == NULL)
				return INDIGO_FAILED;
			indigo_init_switch_item(MOUNT_ABORT_MOTION_ITEM, "ABORT_MOTION", "Abort motion", false);
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
			if (indigo_property_match(MOUNT_GEOGRAPHIC_COORDINATES_PROPERTY, property))
				indigo_define_property(device, MOUNT_GEOGRAPHIC_COORDINATES_PROPERTY, NULL);
			if (indigo_property_match(MOUNT_LST_TIME_PROPERTY, property) && !MOUNT_LST_TIME_PROPERTY->hidden)
				indigo_define_property(device, MOUNT_LST_TIME_PROPERTY, NULL);
			if (indigo_property_match(MOUNT_PARK_PROPERTY, property))
				indigo_define_property(device, MOUNT_PARK_PROPERTY, NULL);
			if (indigo_property_match(MOUNT_ON_COORDINATES_SET_PROPERTY, property))
				indigo_define_property(device, MOUNT_ON_COORDINATES_SET_PROPERTY, NULL);
			if (indigo_property_match(MOUNT_SLEW_RATE_PROPERTY, property))
				indigo_define_property(device, MOUNT_SLEW_RATE_PROPERTY, NULL);
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
			indigo_define_property(device, MOUNT_GEOGRAPHIC_COORDINATES_PROPERTY, NULL);
			if (!MOUNT_LST_TIME_PROPERTY->hidden)
				indigo_define_property(device, MOUNT_LST_TIME_PROPERTY, NULL);
			indigo_define_property(device, MOUNT_PARK_PROPERTY, NULL);
			indigo_define_property(device, MOUNT_ON_COORDINATES_SET_PROPERTY, NULL);
			indigo_define_property(device, MOUNT_SLEW_RATE_PROPERTY, NULL);
			indigo_define_property(device, MOUNT_EQUATORIAL_COORDINATES_PROPERTY, NULL);
			if (!MOUNT_HORIZONTAL_COORDINATES_PROPERTY->hidden)
				indigo_define_property(device, MOUNT_HORIZONTAL_COORDINATES_PROPERTY, NULL);
			indigo_define_property(device, MOUNT_ABORT_MOTION_PROPERTY, NULL);
		} else {
			indigo_delete_property(device, MOUNT_GEOGRAPHIC_COORDINATES_PROPERTY, NULL);
			if (!MOUNT_LST_TIME_PROPERTY->hidden)
			indigo_delete_property(device, MOUNT_LST_TIME_PROPERTY, NULL);
			indigo_delete_property(device, MOUNT_PARK_PROPERTY, NULL);
			indigo_delete_property(device, MOUNT_ON_COORDINATES_SET_PROPERTY, NULL);
			indigo_delete_property(device, MOUNT_SLEW_RATE_PROPERTY, NULL);
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
		// --------------------------------------------------------------------------------
	}
	return indigo_device_change_property(device, client, property);
}

indigo_result indigo_mount_detach(indigo_device *device) {
	assert(device != NULL);
	free(MOUNT_GEOGRAPHIC_COORDINATES_PROPERTY);
	free(MOUNT_LST_TIME_PROPERTY);
	free(MOUNT_PARK_PROPERTY);
	free(MOUNT_ON_COORDINATES_SET_PROPERTY);
	free(MOUNT_SLEW_RATE_PROPERTY);
	free(MOUNT_EQUATORIAL_COORDINATES_PROPERTY);
	free(MOUNT_HORIZONTAL_COORDINATES_PROPERTY);
	free(MOUNT_ABORT_MOTION_PROPERTY);
	return indigo_device_detach(device);
}

