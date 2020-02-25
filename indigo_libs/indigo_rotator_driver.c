// Copyright (c) 2020 by Rumen G.Bogdanovski
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

/** INDIGO Rotator Driver base
 \file indigo_rotator_driver.c
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

#include <indigo/indigo_rotator_driver.h>

indigo_result indigo_rotator_attach(indigo_device *device, unsigned version) {
	assert(device != NULL);
	assert(device != NULL);
	if (ROTATOR_CONTEXT == NULL) {
		device->device_context = malloc(sizeof(indigo_rotator_context));
		assert(device->device_context);
		memset(device->device_context, 0, sizeof(indigo_rotator_context));
	}
	if (ROTATOR_CONTEXT != NULL) {
		if (indigo_device_attach(device, version, INDIGO_INTERFACE_ROTATOR) == INDIGO_OK) {
			// -------------------------------------------------------------------------------- ROTATOR_STEPS_PER_REVOLUTION
			ROTATOR_STEPS_PER_REVOLUTION_PROPERTY = indigo_init_number_property(NULL, device->name, ROTATOR_STEPS_PER_REVOLUTION_PROPERTY_NAME, ROTATOR_MAIN_GROUP, "Steps Per Revolution", INDIGO_OK_STATE, INDIGO_RW_PERM, 1);
			if (ROTATOR_STEPS_PER_REVOLUTION_PROPERTY == NULL)
				return INDIGO_FAILED;
			ROTATOR_STEPS_PER_REVOLUTION_PROPERTY->hidden = true;
			indigo_init_number_item(ROTATOR_STEPS_PER_REVOLUTION_ITEM, ROTATOR_STEPS_PER_REVOLUTION_ITEM_NAME, "Steps/360°", 1, 3600, 1, 3600);
			// -------------------------------------------------------------------------------- ROTATOR_DIRECTION
			ROTATOR_DIRECTION_PROPERTY = indigo_init_switch_property(NULL, device->name, ROTATOR_DIRECTION_PROPERTY_NAME, ROTATOR_MAIN_GROUP, "Invert motion", INDIGO_OK_STATE, INDIGO_RW_PERM, INDIGO_ONE_OF_MANY_RULE, 2);
			if (ROTATOR_DIRECTION_PROPERTY == NULL)
				return INDIGO_FAILED;
			ROTATOR_DIRECTION_PROPERTY->hidden = true;
			indigo_init_switch_item(ROTATOR_DIRECTION_NORMAL_ITEM, ROTATOR_DIRECTION_NORMAL_ITEM_NAME, "Normal", true);
			indigo_init_switch_item(ROTATOR_DIRECTION_REVERSED_ITEM, ROTATOR_DIRECTION_REVERSED_ITEM_NAME, "Reversed", false);
			// -------------------------------------------------------------------------------- ROTATOR_ON_POSITION_SET
			ROTATOR_ON_POSITION_SET_PROPERTY = indigo_init_switch_property(NULL, device->name, ROTATOR_ON_POSITION_SET_PROPERTY_NAME, ROTATOR_MAIN_GROUP, "On position set", INDIGO_OK_STATE, INDIGO_RW_PERM, INDIGO_ONE_OF_MANY_RULE, 2);
			if (ROTATOR_ON_POSITION_SET_PROPERTY == NULL)
				return INDIGO_FAILED;
			indigo_init_switch_item(ROTATOR_ON_POSITION_SET_GOTO_ITEM, ROTATOR_ON_POSITION_SET_GOTO_ITEM_NAME, "Goto to position", true);
			indigo_init_switch_item(ROTATOR_ON_POSITION_SET_SYNC_ITEM, ROTATOR_ON_POSITION_SET_SYNC_ITEM_NAME, "Sync to position", false);
			// -------------------------------------------------------------------------------- ROTATOR_POSITION
			ROTATOR_POSITION_PROPERTY = indigo_init_number_property(NULL, device->name, ROTATOR_POSITION_PROPERTY_NAME, ROTATOR_MAIN_GROUP, "Absolute position", INDIGO_OK_STATE, INDIGO_RW_PERM, 1);
			if (ROTATOR_POSITION_PROPERTY == NULL)
				return INDIGO_FAILED;
			indigo_init_number_item(ROTATOR_POSITION_ITEM, ROTATOR_POSITION_ITEM_NAME, "Absolute position (°)", -90, 360, 1, 0);
			// -------------------------------------------------------------------------------- ROTATOR_ABORT_MOTION
			ROTATOR_ABORT_MOTION_PROPERTY = indigo_init_switch_property(NULL, device->name, ROTATOR_ABORT_MOTION_PROPERTY_NAME, ROTATOR_MAIN_GROUP, "Abort motion", INDIGO_OK_STATE, INDIGO_RW_PERM, INDIGO_AT_MOST_ONE_RULE, 1);
			if (ROTATOR_ABORT_MOTION_PROPERTY == NULL)
				return INDIGO_FAILED;
			indigo_init_switch_item(ROTATOR_ABORT_MOTION_ITEM, ROTATOR_ABORT_MOTION_ITEM_NAME, "Abort motion", false);
				// -------------------------------------------------------------------------------- ROTATOR_BACKLASH
			ROTATOR_BACKLASH_PROPERTY = indigo_init_number_property(NULL, device->name, ROTATOR_BACKLASH_PROPERTY_NAME, ROTATOR_MAIN_GROUP, "Backlash compensation", INDIGO_OK_STATE, INDIGO_RW_PERM, 1);
			if (ROTATOR_BACKLASH_PROPERTY == NULL)
				return INDIGO_FAILED;
			ROTATOR_BACKLASH_PROPERTY->hidden = true;
			indigo_init_number_item(ROTATOR_BACKLASH_ITEM, ROTATOR_BACKLASH_ITEM_NAME, "Backlash (steps)", 0, 999, 0, 0);
			// -------------------------------------------------------------------------------- ROTAOTR_LIMITS
			ROTATOR_LIMITS_PROPERTY = indigo_init_number_property(NULL, device->name, ROTATOR_LIMITS_PROPERTY_NAME, ROTATOR_MAIN_GROUP, "Rotator limits", INDIGO_OK_STATE, INDIGO_RW_PERM, 2);
			if (ROTATOR_LIMITS_PROPERTY == NULL)
				return INDIGO_FAILED;
			ROTATOR_LIMITS_PROPERTY->hidden = true;
			indigo_init_number_item(ROTATOR_LIMITS_MIN_POSITION_ITEM, ROTATOR_LIMITS_MIN_POSITION_ITEM_NAME, "Minimum position (°)", -90, 360, 1, -90);
			indigo_init_number_item(ROTATOR_LIMITS_MAX_POSITION_ITEM, ROTATOR_LIMITS_MAX_POSITION_ITEM_NAME, "Maximum (°)", -90, 360, 1, 360);
			// --------------------------------------------------------------------------------
			return INDIGO_OK;
		}
	}
	return INDIGO_FAILED;
}

indigo_result indigo_rotator_enumerate_properties(indigo_device *device, indigo_client *client, indigo_property *property) {
	assert(device != NULL);
	assert(DEVICE_CONTEXT != NULL);
	if (IS_CONNECTED) {
		if (indigo_property_match(ROTATOR_DIRECTION_PROPERTY, property))
			indigo_define_property(device, ROTATOR_DIRECTION_PROPERTY, NULL);
		if (indigo_property_match(ROTATOR_STEPS_PER_REVOLUTION_PROPERTY, property))
			indigo_define_property(device, ROTATOR_STEPS_PER_REVOLUTION_PROPERTY, NULL);
		if (indigo_property_match(ROTATOR_ABORT_MOTION_PROPERTY, property))
			indigo_define_property(device, ROTATOR_ABORT_MOTION_PROPERTY, NULL);
		if (indigo_property_match(ROTATOR_BACKLASH_PROPERTY, property))
			indigo_define_property(device, ROTATOR_BACKLASH_PROPERTY, NULL);
		if (indigo_property_match(ROTATOR_POSITION_PROPERTY, property))
			indigo_define_property(device, ROTATOR_POSITION_PROPERTY, NULL);
		if (indigo_property_match(ROTATOR_LIMITS_PROPERTY, property))
			indigo_define_property(device, ROTATOR_LIMITS_PROPERTY, NULL);
		if (indigo_property_match(ROTATOR_ON_POSITION_SET_PROPERTY, property))
			indigo_define_property(device, ROTATOR_ON_POSITION_SET_PROPERTY, NULL);
	}
	return indigo_device_enumerate_properties(device, client, property);
}

indigo_result indigo_rotator_change_property(indigo_device *device, indigo_client *client, indigo_property *property) {
	assert(device != NULL);
	assert(DEVICE_CONTEXT != NULL);
	assert(property != NULL);
	if (indigo_property_match(CONNECTION_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- CONNECTION
		if (IS_CONNECTED) {
			indigo_define_property(device, ROTATOR_LIMITS_PROPERTY, NULL);
			indigo_define_property(device, ROTATOR_DIRECTION_PROPERTY, NULL);
			indigo_define_property(device, ROTATOR_STEPS_PER_REVOLUTION_PROPERTY, NULL);
			indigo_define_property(device, ROTATOR_ON_POSITION_SET_PROPERTY, NULL);
			indigo_define_property(device, ROTATOR_ABORT_MOTION_PROPERTY, NULL);
			indigo_define_property(device, ROTATOR_BACKLASH_PROPERTY, NULL);
			indigo_define_property(device, ROTATOR_POSITION_PROPERTY, NULL);
		} else {
			indigo_delete_property(device, ROTATOR_LIMITS_PROPERTY, NULL);
			indigo_delete_property(device, ROTATOR_DIRECTION_PROPERTY, NULL);
			indigo_delete_property(device, ROTATOR_STEPS_PER_REVOLUTION_PROPERTY, NULL);
			indigo_delete_property(device, ROTATOR_ON_POSITION_SET_PROPERTY, NULL);
			indigo_delete_property(device, ROTATOR_ABORT_MOTION_PROPERTY, NULL);
			indigo_delete_property(device, ROTATOR_BACKLASH_PROPERTY, NULL);
			indigo_delete_property(device, ROTATOR_POSITION_PROPERTY, NULL);
		}
	// -------------------------------------------------------------------------------- ROTATOR_ON_POSITION_SET
	} else if (indigo_property_match(ROTATOR_ON_POSITION_SET_PROPERTY, property)) {
		indigo_property_copy_values(ROTATOR_ON_POSITION_SET_PROPERTY, property, false);
		ROTATOR_ON_POSITION_SET_PROPERTY->state = INDIGO_OK_STATE;
		if (IS_CONNECTED) {
			indigo_update_property(device, ROTATOR_ON_POSITION_SET_PROPERTY, NULL);
		}
		return INDIGO_OK;
	// -------------------------------------------------------------------------------- ROTATOR_DIRECTION
	} else if (indigo_property_match(ROTATOR_DIRECTION_PROPERTY, property)) {
		indigo_property_copy_values(ROTATOR_DIRECTION_PROPERTY, property, false);
		ROTATOR_DIRECTION_PROPERTY->state = INDIGO_OK_STATE;
		if (IS_CONNECTED) {
			indigo_update_property(device, ROTATOR_DIRECTION_PROPERTY, NULL);
		}
		return INDIGO_OK;
	// -------------------------------------------------------------------------------- ROTATOR_STEPS_PER_REVOLUTION
} else if (indigo_property_match(ROTATOR_STEPS_PER_REVOLUTION_PROPERTY, property)) {
			indigo_property_copy_values(ROTATOR_STEPS_PER_REVOLUTION_PROPERTY, property, false);
			ROTATOR_STEPS_PER_REVOLUTION_PROPERTY->state = INDIGO_OK_STATE;
			if (IS_CONNECTED) {
				indigo_update_property(device, ROTATOR_STEPS_PER_REVOLUTION_PROPERTY, NULL);
			}
			return INDIGO_OK;
	} else if (indigo_property_match(CONFIG_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- CONFIG
		if (indigo_switch_match(CONFIG_SAVE_ITEM, property)) {
			indigo_save_property(device, NULL, ROTATOR_DIRECTION_PROPERTY);
			indigo_save_property(device, NULL, ROTATOR_STEPS_PER_REVOLUTION_PROPERTY);
			indigo_save_property(device, NULL, ROTATOR_BACKLASH_PROPERTY);
			indigo_save_property(device, NULL, ROTATOR_LIMITS_PROPERTY);
		}
	}
	return indigo_device_change_property(device, client, property);
}

indigo_result indigo_rotator_detach(indigo_device *device) {
	assert(device != NULL);
	indigo_release_property(ROTATOR_DIRECTION_PROPERTY);
	indigo_release_property(ROTATOR_ABORT_MOTION_PROPERTY);
	indigo_release_property(ROTATOR_BACKLASH_PROPERTY);
	indigo_release_property(ROTATOR_LIMITS_PROPERTY);
	indigo_release_property(ROTATOR_STEPS_PER_REVOLUTION_PROPERTY);
	indigo_release_property(ROTATOR_ON_POSITION_SET_PROPERTY);
	indigo_release_property(ROTATOR_POSITION_PROPERTY);
	return indigo_device_detach(device);
}
