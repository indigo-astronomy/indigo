//  Copyright (c) 2016 CloudMakers, s. r. o.
//  All rights reserved.
//
//	You can use this software under the terms of 'INDIGO Astronomy
//  open-source license' (see LICENSE.md).
//
//	THIS SOFTWARE IS PROVIDED BY THE AUTHORS 'AS IS' AND ANY EXPRESS
//	OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
//	WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
//	ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY
//	DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
//	DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
//	GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
//	INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
//	WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
//	NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
//	SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

//  version history
//  2.0 Build 0 - PoC by Peter Polakovic <peter.polakovic@cloudmakers.eu>

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

indigo_result indigo_focuser_device_attach(indigo_device *device, indigo_version version) {
	assert(device != NULL);
	assert(device != NULL);
	if (FOCUSER_DEVICE_CONTEXT == NULL) {
		device->device_context = malloc(sizeof(indigo_focuser_device_context));
		assert(device->device_context);
		memset(device->device_context, 0, sizeof(indigo_focuser_device_context));
	}
	if (FOCUSER_DEVICE_CONTEXT != NULL) {
		if (indigo_device_attach(device, version, INDIGO_INTERFACE_FOCUSER) == INDIGO_OK) {
			// -------------------------------------------------------------------------------- FOCUSER_SPEED
			FOCUSER_SPEED_PROPERTY = indigo_init_number_property(NULL, device->name, "FOCUSER_SPEED", FOCUSER_MAIN_GROUP, "Focuser speed", INDIGO_IDLE_STATE, INDIGO_RW_PERM, 1);
			if (FOCUSER_SPEED_PROPERTY == NULL)
				return INDIGO_FAILED;
			indigo_init_number_item(FOCUSER_SPEED_ITEM, "SPEED", "Speed", 0, 100, 0, 0);
			// -------------------------------------------------------------------------------- FOCUSER_DIRECTION
			FOCUSER_DIRECTION_PROPERTY = indigo_init_switch_property(NULL, device->name, "FOCUSER_DIRECTION", FOCUSER_MAIN_GROUP, "Movement direction", INDIGO_IDLE_STATE, INDIGO_RW_PERM, INDIGO_ONE_OF_MANY_RULE, 2);
			if (FOCUSER_DIRECTION_PROPERTY == NULL)
				return INDIGO_FAILED;
			indigo_init_switch_item(FOCUSER_DIRECTION_MOVE_INWARD_ITEM, "MOVE_INWARD", "Move inward", true);
			indigo_init_switch_item(FOCUSER_DIRECTION_MOVE_OUTWARD_ITEM, "MOVE_OUTWARD", "Move outward", false);
			// -------------------------------------------------------------------------------- FOCUSER_STEPS
			FOCUSER_STEPS_PROPERTY = indigo_init_number_property(NULL, device->name, "FOCUSER_STEPS", FOCUSER_MAIN_GROUP, "Relative move", INDIGO_IDLE_STATE, INDIGO_RW_PERM, 1);
			if (FOCUSER_STEPS_PROPERTY == NULL)
				return INDIGO_FAILED;
			indigo_init_number_item(FOCUSER_STEPS_ITEM, "STEPS", "Relative move (steps/ms)", 0, 10000, 1, 0);
				// -------------------------------------------------------------------------------- FOCUSER_POSITION
			FOCUSER_POSITION_PROPERTY = indigo_init_number_property(NULL, device->name, "FOCUSER_POSITION", FOCUSER_MAIN_GROUP, "Absolute position", INDIGO_IDLE_STATE, INDIGO_RW_PERM, 1);
			if (FOCUSER_POSITION_PROPERTY == NULL)
				return INDIGO_FAILED;
			indigo_init_number_item(FOCUSER_POSITION_ITEM, "POSITION", "Absolute position", -10000, 10000, 1, 0);
			// -------------------------------------------------------------------------------- FOCUSER_ABORT_MOTION
			FOCUSER_ABORT_MOTION_PROPERTY = indigo_init_switch_property(NULL, device->name, "FOCUSER_ABORT_MOTION", FOCUSER_MAIN_GROUP, "Abort motion", INDIGO_IDLE_STATE, INDIGO_RW_PERM, INDIGO_ONE_OF_MANY_RULE, 1);
			if (FOCUSER_ABORT_MOTION_PROPERTY == NULL)
				return INDIGO_FAILED;
			indigo_init_switch_item(FOCUSER_ABORT_MOTION_ITEM, "ABORT_MOTION", "Abort motion", false);
			// --------------------------------------------------------------------------------
			return INDIGO_OK;
		}
	}
	return INDIGO_FAILED;
}

indigo_result indigo_focuser_device_enumerate_properties(indigo_device *device, indigo_client *client, indigo_property *property) {
	assert(device != NULL);
	assert(device->device_context != NULL);
	indigo_result result = INDIGO_OK;
	if ((result = indigo_device_enumerate_properties(device, client, property)) == INDIGO_OK) {
		if (CONNECTION_CONNECTED_ITEM->sw.value) {
			if (indigo_property_match(FOCUSER_SPEED_PROPERTY, property))
				indigo_define_property(device, FOCUSER_SPEED_PROPERTY, NULL);
			if (indigo_property_match(FOCUSER_DIRECTION_PROPERTY, property))
				indigo_define_property(device, FOCUSER_DIRECTION_PROPERTY, NULL);
			if (indigo_property_match(FOCUSER_STEPS_PROPERTY, property))
				indigo_define_property(device, FOCUSER_STEPS_PROPERTY, NULL);
			if (indigo_property_match(FOCUSER_POSITION_PROPERTY, property))
				indigo_define_property(device, FOCUSER_POSITION_PROPERTY, NULL);
			if (indigo_property_match(FOCUSER_ABORT_MOTION_PROPERTY, property))
				indigo_define_property(device, FOCUSER_ABORT_MOTION_PROPERTY, NULL);
		}
	}
	return result;
}

indigo_result indigo_focuser_change_property(indigo_device *device, indigo_client *client, indigo_property *property) {
	assert(device != NULL);
	assert(device->device_context != NULL);
	assert(property != NULL);
	if (indigo_property_match(CONNECTION_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- CONNECTION
		if (CONNECTION_CONNECTED_ITEM->sw.value) {
			indigo_define_property(device, FOCUSER_SPEED_PROPERTY, NULL);
			indigo_define_property(device, FOCUSER_DIRECTION_PROPERTY, NULL);
			indigo_define_property(device, FOCUSER_STEPS_PROPERTY, NULL);
			indigo_define_property(device, FOCUSER_POSITION_PROPERTY, NULL);
			indigo_define_property(device, FOCUSER_ABORT_MOTION_PROPERTY, NULL);
		} else {
			indigo_delete_property(device, FOCUSER_SPEED_PROPERTY, NULL);
			indigo_delete_property(device, FOCUSER_DIRECTION_PROPERTY, NULL);
			indigo_delete_property(device, FOCUSER_STEPS_PROPERTY, NULL);
			indigo_delete_property(device, FOCUSER_POSITION_PROPERTY, NULL);
			indigo_delete_property(device, FOCUSER_ABORT_MOTION_PROPERTY, NULL);
		}
	// -------------------------------------------------------------------------------- FOCUSER_SPEED
	} else if (indigo_property_match(FOCUSER_SPEED_PROPERTY, property)) {
		indigo_property_copy_values(FOCUSER_SPEED_PROPERTY, property, false);
		FOCUSER_SPEED_PROPERTY->state = INDIGO_OK_STATE;
		indigo_update_property(device, FOCUSER_SPEED_PROPERTY, NULL);
	// -------------------------------------------------------------------------------- FOCUSER_DIRECTION
	} else if (indigo_property_match(FOCUSER_DIRECTION_PROPERTY, property)) {
		indigo_property_copy_values(FOCUSER_DIRECTION_PROPERTY, property, false);
		FOCUSER_DIRECTION_PROPERTY->state = INDIGO_OK_STATE;
		indigo_update_property(device, FOCUSER_DIRECTION_PROPERTY, NULL);
	}
	return indigo_device_change_property(device, client, property);
}

indigo_result indigo_focuser_device_detach(indigo_device *device) {
	assert(device != NULL);
	if (CONNECTION_CONNECTED_ITEM->sw.value) {
		indigo_delete_property(device, FOCUSER_SPEED_PROPERTY, NULL);
		indigo_delete_property(device, FOCUSER_DIRECTION_PROPERTY, NULL);
		indigo_delete_property(device, FOCUSER_STEPS_PROPERTY, NULL);
		indigo_delete_property(device, FOCUSER_POSITION_PROPERTY, NULL);
		indigo_delete_property(device, FOCUSER_ABORT_MOTION_PROPERTY, NULL);
	}
	free(FOCUSER_SPEED_PROPERTY);
	free(FOCUSER_DIRECTION_PROPERTY);
	free(FOCUSER_STEPS_PROPERTY);
	free(FOCUSER_POSITION_PROPERTY);
	free(FOCUSER_ABORT_MOTION_PROPERTY);
	return indigo_device_detach(device);
}

