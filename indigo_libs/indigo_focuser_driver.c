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

/** INDIGO Focuser Driver base
 \file indigo_focuser_driver.c
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

#include "indigo_focuser_driver.h"

indigo_result indigo_focuser_attach(indigo_device *device, unsigned version) {
	assert(device != NULL);
	assert(device != NULL);
	if (FOCUSER_CONTEXT == NULL) {
		device->device_context = malloc(sizeof(indigo_focuser_context));
		assert(device->device_context);
		memset(device->device_context, 0, sizeof(indigo_focuser_context));
	}
	if (FOCUSER_CONTEXT != NULL) {
		if (indigo_device_attach(device, version, INDIGO_INTERFACE_FOCUSER) == INDIGO_OK) {
			// -------------------------------------------------------------------------------- FOCUSER_SPEED
			FOCUSER_SPEED_PROPERTY = indigo_init_number_property(NULL, device->name, FOCUSER_SPEED_PROPERTY_NAME, FOCUSER_MAIN_GROUP, "Focuser speed", INDIGO_OK_STATE, INDIGO_RW_PERM, 1);
			if (FOCUSER_SPEED_PROPERTY == NULL)
				return INDIGO_FAILED;
			indigo_init_number_item(FOCUSER_SPEED_ITEM, FOCUSER_SPEED_ITEM_NAME, "Speed", 1, 100, 1, 1);
			// -------------------------------------------------------------------------------- FOCUSER_REVERSE_MOTION
			FOCUSER_REVERSE_MOTION_PROPERTY = indigo_init_switch_property(NULL, device->name, FOCUSER_REVERSE_MOTION_PROPERTY_NAME, FOCUSER_MAIN_GROUP, "Invert on and out motion", INDIGO_OK_STATE, INDIGO_RW_PERM, INDIGO_ONE_OF_MANY_RULE, 2);
			if (FOCUSER_REVERSE_MOTION_PROPERTY == NULL)
				return INDIGO_FAILED;
			FOCUSER_REVERSE_MOTION_PROPERTY->hidden = true;
			indigo_init_switch_item(FOCUSER_REVERSE_MOTION_DISABLED_ITEM, FOCUSER_REVERSE_MOTION_DISABLED_ITEM_NAME, "Disabled", true);
			indigo_init_switch_item(FOCUSER_REVERSE_MOTION_ENABLED_ITEM, FOCUSER_REVERSE_MOTION_ENABLED_ITEM_NAME, "Enabled", false);
			// -------------------------------------------------------------------------------- FOCUSER_DIRECTION
			FOCUSER_DIRECTION_PROPERTY = indigo_init_switch_property(NULL, device->name, FOCUSER_DIRECTION_PROPERTY_NAME, FOCUSER_MAIN_GROUP, "Movement direction", INDIGO_OK_STATE, INDIGO_RW_PERM, INDIGO_ONE_OF_MANY_RULE, 2);
			if (FOCUSER_DIRECTION_PROPERTY == NULL)
				return INDIGO_FAILED;
			indigo_init_switch_item(FOCUSER_DIRECTION_MOVE_INWARD_ITEM, FOCUSER_DIRECTION_MOVE_INWARD_ITEM_NAME, "Move inward", true);
			indigo_init_switch_item(FOCUSER_DIRECTION_MOVE_OUTWARD_ITEM, FOCUSER_DIRECTION_MOVE_OUTWARD_ITEM_NAME, "Move outward", false);
			// -------------------------------------------------------------------------------- FOCUSER_STEPS
			FOCUSER_STEPS_PROPERTY = indigo_init_number_property(NULL, device->name, FOCUSER_STEPS_PROPERTY_NAME, FOCUSER_MAIN_GROUP, "Relative move", INDIGO_OK_STATE, INDIGO_RW_PERM, 1);
			if (FOCUSER_STEPS_PROPERTY == NULL)
				return INDIGO_FAILED;
			indigo_init_number_item(FOCUSER_STEPS_ITEM, FOCUSER_STEPS_ITEM_NAME, "Relative move (steps/ms)", 0, 65535, 1, 0);
			// -------------------------------------------------------------------------------- FOCUSER_ON_POSITION_SET
			FOCUSER_ON_POSITION_SET_PROPERTY = indigo_init_switch_property(NULL, device->name,FOCUSER_ON_POSITION_SET_PROPERTY_NAME, FOCUSER_MAIN_GROUP, "On position set", INDIGO_OK_STATE, INDIGO_RW_PERM, INDIGO_ONE_OF_MANY_RULE, 2);
			if (FOCUSER_ON_POSITION_SET_PROPERTY == NULL)
				return INDIGO_FAILED;
			FOCUSER_ON_POSITION_SET_PROPERTY->hidden = true;
			indigo_init_switch_item(FOCUSER_ON_POSITION_SET_GOTO_ITEM, FOCUSER_ON_POSITION_SET_GOTO_ITEM_NAME, "Goto to position", true);
			indigo_init_switch_item(FOCUSER_ON_POSITION_SET_SYNC_ITEM, FOCUSER_ON_POSITION_SET_SYNC_ITEM_NAME, "Sync to position", false);
			// -------------------------------------------------------------------------------- FOCUSER_POSITION
			FOCUSER_POSITION_PROPERTY = indigo_init_number_property(NULL, device->name, FOCUSER_POSITION_PROPERTY_NAME, FOCUSER_MAIN_GROUP, "Absolute position", INDIGO_OK_STATE, INDIGO_RW_PERM, 1);
			if (FOCUSER_POSITION_PROPERTY == NULL)
				return INDIGO_FAILED;
			indigo_init_number_item(FOCUSER_POSITION_ITEM, FOCUSER_POSITION_ITEM_NAME, "Absolute position", -10000, 10000, 1, 0);
			// -------------------------------------------------------------------------------- FOCUSER_ABORT_MOTION
			FOCUSER_ABORT_MOTION_PROPERTY = indigo_init_switch_property(NULL, device->name, FOCUSER_ABORT_MOTION_PROPERTY_NAME, FOCUSER_MAIN_GROUP, "Abort motion", INDIGO_OK_STATE, INDIGO_RW_PERM, INDIGO_AT_MOST_ONE_RULE, 1);
			if (FOCUSER_ABORT_MOTION_PROPERTY == NULL)
				return INDIGO_FAILED;
			indigo_init_switch_item(FOCUSER_ABORT_MOTION_ITEM, FOCUSER_ABORT_MOTION_ITEM_NAME, "Abort motion", false);
				// -------------------------------------------------------------------------------- CCD_TEMPERATURE
			FOCUSER_BACKLASH_PROPERTY = indigo_init_number_property(NULL, device->name, FOCUSER_BACKLASH_PROPERTY_NAME, FOCUSER_MAIN_GROUP, "Backlash compensation", INDIGO_OK_STATE, INDIGO_RW_PERM, 1);
			if (FOCUSER_BACKLASH_PROPERTY == NULL)
				return INDIGO_FAILED;
			FOCUSER_BACKLASH_PROPERTY->hidden = true;
			indigo_init_number_item(FOCUSER_BACKLASH_ITEM, FOCUSER_BACKLASH_ITEM_NAME, "Backlash", 0, 999, 0, 0);
			// -------------------------------------------------------------------------------- CCD_TEMPERATURE
			FOCUSER_TEMPERATURE_PROPERTY = indigo_init_number_property(NULL, device->name, FOCUSER_TEMPERATURE_PROPERTY_NAME, FOCUSER_MAIN_GROUP, "Temperature", INDIGO_OK_STATE, INDIGO_RO_PERM, 1);
			if (FOCUSER_TEMPERATURE_PROPERTY == NULL)
				return INDIGO_FAILED;
			FOCUSER_TEMPERATURE_PROPERTY->hidden = true;
			indigo_init_number_item(FOCUSER_TEMPERATURE_ITEM, FOCUSER_TEMPERATURE_ITEM_NAME, "Temperature (°C)", -50, 50, 1, 0);
			// -------------------------------------------------------------------------------- CCD_COMPENSATION
			FOCUSER_COMPENSATION_PROPERTY = indigo_init_number_property(NULL, device->name, FOCUSER_COMPENSATION_PROPERTY_NAME, FOCUSER_MAIN_GROUP, "Temperature compensation", INDIGO_OK_STATE, INDIGO_RW_PERM, 1);
			if (FOCUSER_COMPENSATION_PROPERTY == NULL)
				return INDIGO_FAILED;
			FOCUSER_COMPENSATION_PROPERTY->hidden = true;
			indigo_init_number_item(FOCUSER_COMPENSATION_ITEM, FOCUSER_COMPENSATION_ITEM_NAME, "Compensation (steps/°C)", -50, 50, 1, 0);
			// -------------------------------------------------------------------------------- FOCUSER_MODE
			FOCUSER_MODE_PROPERTY = indigo_init_switch_property(NULL, device->name, FOCUSER_MODE_PROPERTY_NAME, FOCUSER_MAIN_GROUP, "Compensation mode", INDIGO_OK_STATE, INDIGO_RW_PERM, INDIGO_ONE_OF_MANY_RULE, 2);
			if (FOCUSER_MODE_PROPERTY == NULL)
				return INDIGO_FAILED;
			FOCUSER_MODE_PROPERTY->hidden = true;
			indigo_init_switch_item(FOCUSER_MODE_MANUAL_ITEM, FOCUSER_MODE_MANUAL_ITEM_NAME, "Manual control", true);
			indigo_init_switch_item(FOCUSER_MODE_AUTOMATIC_ITEM, FOCUSER_MODE_AUTOMATIC_ITEM_NAME, "Automatic control", false);
			// -------------------------------------------------------------------------------- CCD_LIMITS
			FOCUSER_LIMITS_PROPERTY = indigo_init_number_property(NULL, device->name, FOCUSER_LIMITS_PROPERTY_NAME, FOCUSER_MAIN_GROUP, "Focuser Limits", INDIGO_OK_STATE, INDIGO_RW_PERM, 2);
			if (FOCUSER_LIMITS_PROPERTY == NULL)
				return INDIGO_FAILED;
			FOCUSER_LIMITS_PROPERTY->hidden = true;
			indigo_init_number_item(FOCUSER_LIMITS_MIN_POSITION_ITEM, FOCUSER_LIMITS_MIN_POSITION_ITEM_NAME, "Minimum (steps)", 0, 1000, 1, 0);
			indigo_init_number_item(FOCUSER_LIMITS_MAX_POSITION_ITEM, FOCUSER_LIMITS_MAX_POSITION_ITEM_NAME, "Maximum (steps)", 0, 1000, 1, 0);
			// --------------------------------------------------------------------------------
			return INDIGO_OK;
		}
	}
	return INDIGO_FAILED;
}

indigo_result indigo_focuser_enumerate_properties(indigo_device *device, indigo_client *client, indigo_property *property) {
	assert(device != NULL);
	assert(DEVICE_CONTEXT != NULL);
	if (IS_CONNECTED) {
		if (FOCUSER_MODE_MANUAL_ITEM->sw.value) {
			if (indigo_property_match(FOCUSER_SPEED_PROPERTY, property))
				indigo_define_property(device, FOCUSER_SPEED_PROPERTY, NULL);
			if (indigo_property_match(FOCUSER_REVERSE_MOTION_PROPERTY, property))
				indigo_define_property(device, FOCUSER_REVERSE_MOTION_PROPERTY, NULL);
			if (indigo_property_match(FOCUSER_DIRECTION_PROPERTY, property))
				indigo_define_property(device, FOCUSER_DIRECTION_PROPERTY, NULL);
			if (indigo_property_match(FOCUSER_STEPS_PROPERTY, property))
				indigo_define_property(device, FOCUSER_STEPS_PROPERTY, NULL);
			if (indigo_property_match(FOCUSER_ABORT_MOTION_PROPERTY, property))
				indigo_define_property(device, FOCUSER_ABORT_MOTION_PROPERTY, NULL);
			if (indigo_property_match(FOCUSER_BACKLASH_PROPERTY, property))
				indigo_define_property(device, FOCUSER_BACKLASH_PROPERTY, NULL);
			if (indigo_property_match(FOCUSER_POSITION_PROPERTY, property)) {
				FOCUSER_POSITION_PROPERTY->perm = INDIGO_RW_PERM;
				indigo_define_property(device, FOCUSER_POSITION_PROPERTY, NULL);
			}
		} else {
			if (indigo_property_match(FOCUSER_POSITION_PROPERTY, property)) {
				FOCUSER_POSITION_PROPERTY->perm = INDIGO_RO_PERM;
				indigo_define_property(device, FOCUSER_POSITION_PROPERTY, NULL);
			}
		}
		if (indigo_property_match(FOCUSER_LIMITS_PROPERTY, property))
			indigo_define_property(device, FOCUSER_LIMITS_PROPERTY, NULL);
		if (indigo_property_match(FOCUSER_ON_POSITION_SET_PROPERTY, property))
			indigo_define_property(device, FOCUSER_ON_POSITION_SET_PROPERTY, NULL);
		if (indigo_property_match(FOCUSER_TEMPERATURE_PROPERTY, property))
			indigo_define_property(device, FOCUSER_TEMPERATURE_PROPERTY, NULL);
		if (indigo_property_match(FOCUSER_COMPENSATION_PROPERTY, property))
			indigo_define_property(device, FOCUSER_COMPENSATION_PROPERTY, NULL);
		if (indigo_property_match(FOCUSER_MODE_PROPERTY, property))
			indigo_define_property(device, FOCUSER_MODE_PROPERTY, NULL);
	}
	return indigo_device_enumerate_properties(device, client, property);
}

indigo_result indigo_focuser_change_property(indigo_device *device, indigo_client *client, indigo_property *property) {
	assert(device != NULL);
	assert(DEVICE_CONTEXT != NULL);
	assert(property != NULL);
	if (indigo_property_match(CONNECTION_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- CONNECTION
		if (IS_CONNECTED) {
			indigo_define_property(device, FOCUSER_MODE_PROPERTY, NULL);
			indigo_define_property(device, FOCUSER_LIMITS_PROPERTY, NULL);
			indigo_define_property(device, FOCUSER_ON_POSITION_SET_PROPERTY, NULL);
			indigo_define_property(device, FOCUSER_TEMPERATURE_PROPERTY, NULL);
			indigo_define_property(device, FOCUSER_COMPENSATION_PROPERTY, NULL);
			if (FOCUSER_MODE_MANUAL_ITEM->sw.value) {
				indigo_define_property(device, FOCUSER_SPEED_PROPERTY, NULL);
				indigo_define_property(device, FOCUSER_REVERSE_MOTION_PROPERTY, NULL);
				indigo_define_property(device, FOCUSER_DIRECTION_PROPERTY, NULL);
				indigo_define_property(device, FOCUSER_STEPS_PROPERTY, NULL);
				indigo_define_property(device, FOCUSER_ABORT_MOTION_PROPERTY, NULL);
				indigo_define_property(device, FOCUSER_BACKLASH_PROPERTY, NULL);
				FOCUSER_POSITION_PROPERTY->perm = INDIGO_RW_PERM;
				indigo_define_property(device, FOCUSER_POSITION_PROPERTY, NULL);
			} else {
				FOCUSER_POSITION_PROPERTY->perm = INDIGO_RO_PERM;
				indigo_define_property(device, FOCUSER_POSITION_PROPERTY, NULL);
			}
		} else {
			indigo_delete_property(device, FOCUSER_LIMITS_PROPERTY, NULL);
			indigo_delete_property(device, FOCUSER_ON_POSITION_SET_PROPERTY, NULL);
			indigo_delete_property(device, FOCUSER_POSITION_PROPERTY, NULL);
			indigo_delete_property(device, FOCUSER_TEMPERATURE_PROPERTY, NULL);
			indigo_delete_property(device, FOCUSER_COMPENSATION_PROPERTY, NULL);
			indigo_delete_property(device, FOCUSER_MODE_PROPERTY, NULL);
			if (FOCUSER_MODE_MANUAL_ITEM->sw.value) {
				indigo_delete_property(device, FOCUSER_SPEED_PROPERTY, NULL);
				indigo_delete_property(device, FOCUSER_REVERSE_MOTION_PROPERTY, NULL);
				indigo_delete_property(device, FOCUSER_DIRECTION_PROPERTY, NULL);
				indigo_delete_property(device, FOCUSER_STEPS_PROPERTY, NULL);
				indigo_delete_property(device, FOCUSER_ABORT_MOTION_PROPERTY, NULL);
				indigo_delete_property(device, FOCUSER_BACKLASH_PROPERTY, NULL);
			}
		}
	// -------------------------------------------------------------------------------- FOCUSER_SPEED
	} else if (indigo_property_match(FOCUSER_SPEED_PROPERTY, property)) {
		indigo_property_copy_values(FOCUSER_SPEED_PROPERTY, property, false);
		FOCUSER_SPEED_PROPERTY->state = INDIGO_OK_STATE;
		if (IS_CONNECTED) {
			indigo_update_property(device, FOCUSER_SPEED_PROPERTY, NULL);
		}
		return INDIGO_OK;
	// -------------------------------------------------------------------------------- FOCUSER_REVERSE_MOTION
	} else if (indigo_property_match(FOCUSER_REVERSE_MOTION_PROPERTY, property)) {
		indigo_property_copy_values(FOCUSER_REVERSE_MOTION_PROPERTY, property, false);
		FOCUSER_REVERSE_MOTION_PROPERTY->state = INDIGO_OK_STATE;
		if (IS_CONNECTED) {
			indigo_update_property(device, FOCUSER_REVERSE_MOTION_PROPERTY, NULL);
		}
		return INDIGO_OK;
	// -------------------------------------------------------------------------------- FOCUSER_ON_POSITION_SET
	} else if (indigo_property_match(FOCUSER_ON_POSITION_SET_PROPERTY, property)) {
		indigo_property_copy_values(FOCUSER_ON_POSITION_SET_PROPERTY, property, false);
		FOCUSER_ON_POSITION_SET_PROPERTY->state = INDIGO_OK_STATE;
		indigo_update_property(device, FOCUSER_ON_POSITION_SET_PROPERTY, NULL);
		return INDIGO_OK;
		// -------------------------------------------------------------------------------- FOCUSER_DIRECTION
	} else if (indigo_property_match(FOCUSER_DIRECTION_PROPERTY, property)) {
		indigo_property_copy_values(FOCUSER_DIRECTION_PROPERTY, property, false);
		FOCUSER_DIRECTION_PROPERTY->state = INDIGO_OK_STATE;
		if (IS_CONNECTED) {
			indigo_update_property(device, FOCUSER_DIRECTION_PROPERTY, NULL);
		}
		return INDIGO_OK;
		// -------------------------------------------------------------------------------- FOCUSER_LIMITS
	} else if (indigo_property_match(FOCUSER_LIMITS_PROPERTY, property)) {
		indigo_property_copy_values(FOCUSER_LIMITS_PROPERTY, property, false);
		FOCUSER_LIMITS_PROPERTY->state = INDIGO_OK_STATE;
		if (IS_CONNECTED) {
			indigo_update_property(device, FOCUSER_LIMITS_PROPERTY, NULL);
		}
		return INDIGO_OK;
	// -------------------------------------------------------------------------------- FOCUSER_MODE
	} else if (indigo_property_match(FOCUSER_MODE_PROPERTY, property)) {
		indigo_property_copy_values(FOCUSER_MODE_PROPERTY, property, false);
		if (FOCUSER_MODE_MANUAL_ITEM->sw.value) {
			indigo_define_property(device, FOCUSER_SPEED_PROPERTY, NULL);
			indigo_define_property(device, FOCUSER_REVERSE_MOTION_PROPERTY, NULL);
			indigo_define_property(device, FOCUSER_DIRECTION_PROPERTY, NULL);
			indigo_define_property(device, FOCUSER_STEPS_PROPERTY, NULL);
			indigo_define_property(device, FOCUSER_ABORT_MOTION_PROPERTY, NULL);
			indigo_define_property(device, FOCUSER_BACKLASH_PROPERTY, NULL);
			indigo_delete_property(device, FOCUSER_POSITION_PROPERTY, NULL);
			FOCUSER_POSITION_PROPERTY->perm = INDIGO_RW_PERM;
			indigo_define_property(device, FOCUSER_POSITION_PROPERTY, NULL);
		} else {
			indigo_delete_property(device, FOCUSER_SPEED_PROPERTY, NULL);
			indigo_delete_property(device, FOCUSER_REVERSE_MOTION_PROPERTY, NULL);
			indigo_delete_property(device, FOCUSER_DIRECTION_PROPERTY, NULL);
			indigo_delete_property(device, FOCUSER_STEPS_PROPERTY, NULL);
			indigo_delete_property(device, FOCUSER_ABORT_MOTION_PROPERTY, NULL);
			indigo_delete_property(device, FOCUSER_BACKLASH_PROPERTY, NULL);
			indigo_delete_property(device, FOCUSER_POSITION_PROPERTY, NULL);
			FOCUSER_POSITION_PROPERTY->perm = INDIGO_RO_PERM;
			indigo_define_property(device, FOCUSER_POSITION_PROPERTY, NULL);
		}
		FOCUSER_MODE_PROPERTY->state = INDIGO_OK_STATE;
		indigo_update_property(device, FOCUSER_MODE_PROPERTY, NULL);
		return INDIGO_OK;
	} else if (indigo_property_match(CONFIG_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- CONFIG
		if (indigo_switch_match(CONFIG_SAVE_ITEM, property)) {
			indigo_save_property(device, NULL, FOCUSER_SPEED_PROPERTY);
			indigo_save_property(device, NULL, FOCUSER_REVERSE_MOTION_PROPERTY);
			indigo_save_property(device, NULL, FOCUSER_DIRECTION_PROPERTY);
			indigo_save_property(device, NULL, FOCUSER_COMPENSATION_PROPERTY);
			indigo_save_property(device, NULL, FOCUSER_BACKLASH_PROPERTY);
			indigo_save_property(device, NULL, FOCUSER_LIMITS_PROPERTY);
		}
	}
	return indigo_device_change_property(device, client, property);
}

indigo_result indigo_focuser_detach(indigo_device *device) {
	assert(device != NULL);
	indigo_release_property(FOCUSER_SPEED_PROPERTY);
	indigo_release_property(FOCUSER_REVERSE_MOTION_PROPERTY);
	indigo_release_property(FOCUSER_DIRECTION_PROPERTY);
	indigo_release_property(FOCUSER_STEPS_PROPERTY);
	indigo_release_property(FOCUSER_ABORT_MOTION_PROPERTY);
	indigo_release_property(FOCUSER_BACKLASH_PROPERTY);
	indigo_release_property(FOCUSER_LIMITS_PROPERTY);
	indigo_release_property(FOCUSER_ON_POSITION_SET_PROPERTY);
	indigo_release_property(FOCUSER_POSITION_PROPERTY);
	indigo_release_property(FOCUSER_TEMPERATURE_PROPERTY);
	indigo_release_property(FOCUSER_COMPENSATION_PROPERTY);
	indigo_release_property(FOCUSER_MODE_PROPERTY);
	return indigo_device_detach(device);
}
