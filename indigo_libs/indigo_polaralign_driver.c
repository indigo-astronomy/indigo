// Copyright (c) 2025 by Rumen G.Bogdanovski 
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
// 2.0 by Rumen G.Bogdanovski <rumenastro@gmail.com>

/** INDIGO Polar Aligner Driver base
 \file indigo_polaralign_driver.c
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

#include <indigo/indigo_polaralign_driver.h>
#include <indigo/indigo_io.h>

indigo_result indigo_polaralign_attach(indigo_device *device, const char *driver_name, unsigned version) {
	assert(device != NULL);
	if (POLARALIGN_CONTEXT == NULL) {
		device->device_context = indigo_safe_malloc(sizeof(indigo_polaralign_context));
	}
	if (POLARALIGN_CONTEXT != NULL) {
		if (indigo_device_attach(device, driver_name, version, INDIGO_INTERFACE_POLARALIGN) == INDIGO_OK) {
			// -------------------------------------------------------------------------------- POLARALIGN_OFFSET
			POLARALIGN_OFFSET_PROPERTY = indigo_init_number_property(NULL, device->name, POLARALIGN_OFFSET_PROPERTY_NAME, POLARALIGN_MAIN_GROUP, "Polar alignment offsets", INDIGO_OK_STATE, INDIGO_RW_PERM, 2);
			if (POLARALIGN_OFFSET_PROPERTY == NULL)
				return INDIGO_FAILED;
			indigo_init_number_item(POLARALIGN_OFFSET_ALT_ITEM, POLARALIGN_OFFSET_ALT_ITEM_NAME, "Altitude offset [arcmin]", -900, 900, 0.1, 0);
			indigo_init_number_item(POLARALIGN_OFFSET_AZ_ITEM, POLARALIGN_OFFSET_AZ_ITEM_NAME, "Azimuth offset [arcmin]", -900, 900, 0.1, 0);
			// -------------------------------------------------------------------------------- POLARALIGN_ABORT_MOTION
			POLARALIGN_ABORT_MOTION_PROPERTY = indigo_init_switch_property(NULL, device->name, POLARALIGN_ABORT_MOTION_PROPERTY_NAME, POLARALIGN_MAIN_GROUP, "Abort motion", INDIGO_OK_STATE, INDIGO_RW_PERM, INDIGO_AT_MOST_ONE_RULE, 1);
			if (POLARALIGN_ABORT_MOTION_PROPERTY == NULL)
				return INDIGO_FAILED;
			indigo_init_switch_item(POLARALIGN_ABORT_MOTION_ITEM, POLARALIGN_ABORT_MOTION_ITEM_NAME, "Abort motion", false);
			// -------------------------------------------------------------------------------- POLARALIGN_STEPS_PER_DEGREE
			POLARALIGN_STEPS_PER_DEGREE_PROPERTY = indigo_init_number_property(NULL, device->name, POLARALIGN_STEPS_PER_DEGREE_PROPERTY_NAME, POLARALIGN_MAIN_GROUP, "Steps per degree", INDIGO_OK_STATE, INDIGO_RW_PERM, 2);
			if (POLARALIGN_STEPS_PER_DEGREE_PROPERTY == NULL)
				return INDIGO_FAILED;
			indigo_init_number_item(POLARALIGN_STEPS_PER_DEGREE_ALT_ITEM, POLARALIGN_STEPS_PER_DEGREE_ALT_ITEM_NAME, "Altitude axis steps/degree", 0, 3600, 1, 0);
			indigo_init_number_item(POLARALIGN_STEPS_PER_DEGREE_AZ_ITEM, POLARALIGN_STEPS_PER_DEGREE_AZ_ITEM_NAME, "Azimuth axis steps/degree", 0, 3600, 1, 0);
			// -------------------------------------------------------------------------------- POLARALIGN_DIRECTION_ALT
			POLARALIGN_DIRECTION_ALT_PROPERTY = indigo_init_switch_property(NULL, device->name, POLARALIGN_DIRECTION_ALT_PROPERTY_NAME, POLARALIGN_MAIN_GROUP, "Altitude axis direction", INDIGO_OK_STATE, INDIGO_RW_PERM, INDIGO_ONE_OF_MANY_RULE, 2);
			if (POLARALIGN_DIRECTION_ALT_PROPERTY == NULL)
				return INDIGO_FAILED;
			indigo_init_switch_item(POLARALIGN_DIRECTION_ALT_NORMAL_ITEM, POLARALIGN_DIRECTION_NORMAL_ITEM_NAME, "Normal", true);
			indigo_init_switch_item(POLARALIGN_DIRECTION_ALT_REVERSED_ITEM, POLARALIGN_DIRECTION_REVERSED_ITEM_NAME, "Reversed", false);
			// -------------------------------------------------------------------------------- POLARALIGN_DIRECTION_AZ
			POLARALIGN_DIRECTION_AZ_PROPERTY = indigo_init_switch_property(NULL, device->name, POLARALIGN_DIRECTION_AZ_PROPERTY_NAME, POLARALIGN_MAIN_GROUP, "Azimuth axis direction", INDIGO_OK_STATE, INDIGO_RW_PERM, INDIGO_ONE_OF_MANY_RULE, 2);
			if (POLARALIGN_DIRECTION_AZ_PROPERTY == NULL)
				return INDIGO_FAILED;
			indigo_init_switch_item(POLARALIGN_DIRECTION_AZ_NORMAL_ITEM, POLARALIGN_DIRECTION_NORMAL_ITEM_NAME, "Normal", true);
			indigo_init_switch_item(POLARALIGN_DIRECTION_AZ_REVERSED_ITEM, POLARALIGN_DIRECTION_REVERSED_ITEM_NAME, "Reversed", false);
			// -------------------------------------------------------------------------------- POLARALIGN_RESET_POSITION_ALT
			POLARALIGN_RESET_POSITION_ALT_PROPERTY = indigo_init_switch_property(NULL, device->name, POLARALIGN_RESET_POSITION_ALT_PROPERTY_NAME, POLARALIGN_MAIN_GROUP, "Reset altitude position to zero", INDIGO_OK_STATE, INDIGO_RW_PERM, INDIGO_AT_MOST_ONE_RULE, 1);
			if (POLARALIGN_RESET_POSITION_ALT_PROPERTY == NULL)
				return INDIGO_FAILED;
			indigo_init_switch_item(POLARALIGN_RESET_POSITION_ALT_ITEM, POLARALIGN_RESET_POSITION_ITEM_NAME, "Reset altitude position", false);
			// -------------------------------------------------------------------------------- POLARALIGN_RESET_POSITION_AZ
			POLARALIGN_RESET_POSITION_AZ_PROPERTY = indigo_init_switch_property(NULL, device->name, POLARALIGN_RESET_POSITION_AZ_PROPERTY_NAME, POLARALIGN_MAIN_GROUP, "Reset azimuth position to zero", INDIGO_OK_STATE, INDIGO_RW_PERM, INDIGO_AT_MOST_ONE_RULE, 1);
			if (POLARALIGN_RESET_POSITION_AZ_PROPERTY == NULL)
				return INDIGO_FAILED;
			indigo_init_switch_item(POLARALIGN_RESET_POSITION_AZ_ITEM, POLARALIGN_RESET_POSITION_ITEM_NAME, "Reset azimuth position", false);
			// -------------------------------------------------------------------------------- POLARALIGN_LIMITS
			POLARALIGN_LIMITS_PROPERTY = indigo_init_number_property(NULL, device->name, POLARALIGN_LIMITS_PROPERTY_NAME, POLARALIGN_MAIN_GROUP, "Position limits [arcmin]", INDIGO_OK_STATE, INDIGO_RW_PERM, 4);
			if (POLARALIGN_LIMITS_PROPERTY == NULL)
				return INDIGO_FAILED;
			indigo_init_number_item(POLARALIGN_LIMITS_MIN_POSITION_ALT_ITEM, POLARALIGN_LIMITS_MIN_POSITION_ALT_ITEM_NAME, "Min altitude position [arcmin]", -900, 0, 1, -900);
			indigo_init_number_item(POLARALIGN_LIMITS_MAX_POSITION_ALT_ITEM, POLARALIGN_LIMITS_MAX_POSITION_ALT_ITEM_NAME, "Max altitude position [arcmin]", 0, 900, 1, 900);
			indigo_init_number_item(POLARALIGN_LIMITS_MIN_POSITION_AZ_ITEM, POLARALIGN_LIMITS_MIN_POSITION_AZ_ITEM_NAME, "Min azimuth position [arcmin]", -900, 0, 1, -900);
			indigo_init_number_item(POLARALIGN_LIMITS_MAX_POSITION_AZ_ITEM, POLARALIGN_LIMITS_MAX_POSITION_AZ_ITEM_NAME, "Max azimuth position [arcmin]", 0, 900, 1, 900);
			// --------------------------------------------------------------------------------
			return INDIGO_OK;
		}
	}
	return INDIGO_FAILED;
}

indigo_result indigo_polaralign_enumerate_properties(indigo_device *device, indigo_client *client, indigo_property *property) {
	assert(device != NULL);
	assert(DEVICE_CONTEXT != NULL);
	if (IS_CONNECTED) {
		INDIGO_DEFINE_MATCHING_PROPERTY(POLARALIGN_OFFSET_PROPERTY);
		INDIGO_DEFINE_MATCHING_PROPERTY(POLARALIGN_ABORT_MOTION_PROPERTY);
		INDIGO_DEFINE_MATCHING_PROPERTY(POLARALIGN_STEPS_PER_DEGREE_PROPERTY);
		INDIGO_DEFINE_MATCHING_PROPERTY(POLARALIGN_DIRECTION_ALT_PROPERTY);
		INDIGO_DEFINE_MATCHING_PROPERTY(POLARALIGN_DIRECTION_AZ_PROPERTY);
		INDIGO_DEFINE_MATCHING_PROPERTY(POLARALIGN_RESET_POSITION_ALT_PROPERTY);
		INDIGO_DEFINE_MATCHING_PROPERTY(POLARALIGN_RESET_POSITION_AZ_PROPERTY);
		INDIGO_DEFINE_MATCHING_PROPERTY(POLARALIGN_LIMITS_PROPERTY);
	}
	return indigo_device_enumerate_properties(device, client, property);
}

indigo_result indigo_polaralign_change_property(indigo_device *device, indigo_client *client, indigo_property *property) {
	assert(device != NULL);
	assert(DEVICE_CONTEXT != NULL);
	assert(property != NULL);
	if (indigo_property_match_changeable(CONNECTION_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- CONNECTION
		if (IS_CONNECTED) {
			indigo_define_property(device, POLARALIGN_OFFSET_PROPERTY, NULL);
			indigo_define_property(device, POLARALIGN_ABORT_MOTION_PROPERTY, NULL);
			indigo_define_property(device, POLARALIGN_STEPS_PER_DEGREE_PROPERTY, NULL);
			indigo_define_property(device, POLARALIGN_DIRECTION_ALT_PROPERTY, NULL);
			indigo_define_property(device, POLARALIGN_DIRECTION_AZ_PROPERTY, NULL);
			indigo_define_property(device, POLARALIGN_RESET_POSITION_ALT_PROPERTY, NULL);
			indigo_define_property(device, POLARALIGN_RESET_POSITION_AZ_PROPERTY, NULL);
			indigo_define_property(device, POLARALIGN_LIMITS_PROPERTY, NULL);
		} else {
			POLARALIGN_OFFSET_PROPERTY->state = INDIGO_OK_STATE;
			indigo_delete_property(device, POLARALIGN_OFFSET_PROPERTY, NULL);
			indigo_delete_property(device, POLARALIGN_ABORT_MOTION_PROPERTY, NULL);
			indigo_delete_property(device, POLARALIGN_STEPS_PER_DEGREE_PROPERTY, NULL);
			indigo_delete_property(device, POLARALIGN_DIRECTION_ALT_PROPERTY, NULL);
			indigo_delete_property(device, POLARALIGN_DIRECTION_AZ_PROPERTY, NULL);
			indigo_delete_property(device, POLARALIGN_RESET_POSITION_ALT_PROPERTY, NULL);
			indigo_delete_property(device, POLARALIGN_RESET_POSITION_AZ_PROPERTY, NULL);
			indigo_delete_property(device, POLARALIGN_LIMITS_PROPERTY, NULL);
		}
	// -------------------------------------------------------------------------------- POLARALIGN_OFFSET
	} else if (indigo_property_match_changeable(POLARALIGN_OFFSET_PROPERTY, property)) {
		indigo_property_copy_values(POLARALIGN_OFFSET_PROPERTY, property, false);
		POLARALIGN_OFFSET_PROPERTY->state = INDIGO_OK_STATE;
		indigo_update_property(device, POLARALIGN_OFFSET_PROPERTY, NULL);
		return INDIGO_OK;
	// -------------------------------------------------------------------------------- POLARALIGN_ABORT_MOTION
	} else if (indigo_property_match_changeable(POLARALIGN_ABORT_MOTION_PROPERTY, property)) {
		indigo_property_copy_values(POLARALIGN_ABORT_MOTION_PROPERTY, property, false);
		POLARALIGN_ABORT_MOTION_PROPERTY->state = INDIGO_OK_STATE;
		POLARALIGN_ABORT_MOTION_ITEM->sw.value = false;
		indigo_update_property(device, POLARALIGN_ABORT_MOTION_PROPERTY, NULL);
		return INDIGO_OK;
	// -------------------------------------------------------------------------------- POLARALIGN_STEPS_PER_DEGREE
	} else if (indigo_property_match_changeable(POLARALIGN_STEPS_PER_DEGREE_PROPERTY, property)) {
		indigo_property_copy_values(POLARALIGN_STEPS_PER_DEGREE_PROPERTY, property, false);
		POLARALIGN_STEPS_PER_DEGREE_PROPERTY->state = INDIGO_OK_STATE;
		indigo_update_property(device, POLARALIGN_STEPS_PER_DEGREE_PROPERTY, NULL);
		return INDIGO_OK;
	// -------------------------------------------------------------------------------- POLARALIGN_DIRECTION_ALT
	} else if (indigo_property_match_changeable(POLARALIGN_DIRECTION_ALT_PROPERTY, property)) {
		indigo_property_copy_values(POLARALIGN_DIRECTION_ALT_PROPERTY, property, false);
		POLARALIGN_DIRECTION_ALT_PROPERTY->state = INDIGO_OK_STATE;
		indigo_update_property(device, POLARALIGN_DIRECTION_ALT_PROPERTY, NULL);
		return INDIGO_OK;
	// -------------------------------------------------------------------------------- POLARALIGN_DIRECTION_AZ
	} else if (indigo_property_match_changeable(POLARALIGN_DIRECTION_AZ_PROPERTY, property)) {
		indigo_property_copy_values(POLARALIGN_DIRECTION_AZ_PROPERTY, property, false);
		POLARALIGN_DIRECTION_AZ_PROPERTY->state = INDIGO_OK_STATE;
		indigo_update_property(device, POLARALIGN_DIRECTION_AZ_PROPERTY, NULL);
		return INDIGO_OK;
	// -------------------------------------------------------------------------------- POLARALIGN_RESET_POSITION_ALT
	} else if (indigo_property_match_changeable(POLARALIGN_RESET_POSITION_ALT_PROPERTY, property)) {
		indigo_property_copy_values(POLARALIGN_RESET_POSITION_ALT_PROPERTY, property, false);
		POLARALIGN_RESET_POSITION_ALT_PROPERTY->state = INDIGO_OK_STATE;
		POLARALIGN_RESET_POSITION_ALT_ITEM->sw.value = false;
		indigo_update_property(device, POLARALIGN_RESET_POSITION_ALT_PROPERTY, NULL);
		return INDIGO_OK;
	// -------------------------------------------------------------------------------- POLARALIGN_RESET_POSITION_AZ
	} else if (indigo_property_match_changeable(POLARALIGN_RESET_POSITION_AZ_PROPERTY, property)) {
		indigo_property_copy_values(POLARALIGN_RESET_POSITION_AZ_PROPERTY, property, false);
		POLARALIGN_RESET_POSITION_AZ_PROPERTY->state = INDIGO_OK_STATE;
		POLARALIGN_RESET_POSITION_AZ_ITEM->sw.value = false;
		indigo_update_property(device, POLARALIGN_RESET_POSITION_AZ_PROPERTY, NULL);
		return INDIGO_OK;
	// -------------------------------------------------------------------------------- POLARALIGN_LIMITS
	} else if (indigo_property_match_changeable(POLARALIGN_LIMITS_PROPERTY, property)) {
		indigo_property_copy_values(POLARALIGN_LIMITS_PROPERTY, property, false);
		POLARALIGN_LIMITS_PROPERTY->state = INDIGO_OK_STATE;
		indigo_update_property(device, POLARALIGN_LIMITS_PROPERTY, NULL);
		return INDIGO_OK;
	}
	return indigo_device_change_property(device, client, property);
}

indigo_result indigo_polaralign_detach(indigo_device *device) {
	assert(device != NULL);
	indigo_release_property(POLARALIGN_OFFSET_PROPERTY);
	indigo_release_property(POLARALIGN_ABORT_MOTION_PROPERTY);
	indigo_release_property(POLARALIGN_STEPS_PER_DEGREE_PROPERTY);
	indigo_release_property(POLARALIGN_DIRECTION_ALT_PROPERTY);
	indigo_release_property(POLARALIGN_DIRECTION_AZ_PROPERTY);
	indigo_release_property(POLARALIGN_RESET_POSITION_ALT_PROPERTY);
	indigo_release_property(POLARALIGN_RESET_POSITION_AZ_PROPERTY);
	indigo_release_property(POLARALIGN_LIMITS_PROPERTY);
	return indigo_device_detach(device);
}
