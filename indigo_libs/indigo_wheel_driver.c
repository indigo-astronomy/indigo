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

/** INDIGO Wheel Driver base
 \file indigo_wheel_driver.c
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

#include "indigo_wheel_driver.h"

indigo_result indigo_wheel_attach(indigo_device *device, unsigned version) {
	assert(device != NULL);
	if (WHEEL_CONTEXT == NULL) {
		device->device_context = malloc(sizeof(indigo_wheel_context));
		assert(device->device_context);
		memset(device->device_context, 0, sizeof(indigo_wheel_context));
	}
	if (WHEEL_CONTEXT != NULL) {
		if (indigo_device_attach(device, version, INDIGO_INTERFACE_WHEEL) == INDIGO_OK) {
			// -------------------------------------------------------------------------------- WHEEL_SLOT
			WHEEL_SLOT_PROPERTY = indigo_init_number_property(NULL, device->name, WHEEL_SLOT_PROPERTY_NAME, WHEEL_MAIN_GROUP, "Current slot", INDIGO_IDLE_STATE, INDIGO_RW_PERM, 1);
			if (WHEEL_SLOT_PROPERTY == NULL)
				return INDIGO_FAILED;
			indigo_init_number_item(WHEEL_SLOT_ITEM, WHEEL_SLOT_ITEM_NAME, "Slot number", 1, 9, 1, 0);
			// -------------------------------------------------------------------------------- WHEEL_SLOT_NAME
			WHEEL_SLOT_NAME_PROPERTY = indigo_init_text_property(NULL, device->name, WHEEL_SLOT_NAME_PROPERTY_NAME, WHEEL_MAIN_GROUP, "Slot names", INDIGO_IDLE_STATE, INDIGO_RW_PERM, WHEEL_SLOT_ITEM->number.max);
			if (WHEEL_SLOT_NAME_PROPERTY == NULL)
				return INDIGO_FAILED;
			for (int i = 0; i < WHEEL_SLOT_NAME_PROPERTY->count; i++) {
				char name[16];
				char label[16];
				snprintf(name, 16, WHEEL_SLOT_NAME_ITEM_NAME, i + 1);
				snprintf(label, 16, "Slot #%d", i + 1);
				indigo_init_text_item(WHEEL_SLOT_NAME_1_ITEM + i, name, label, "Filter #%d", i + 1);
			}
			// --------------------------------------------------------------------------------
			return INDIGO_OK;
		}
	}
	return INDIGO_FAILED;
}

indigo_result indigo_wheel_enumerate_properties(indigo_device *device, indigo_client *client, indigo_property *property) {
	assert(device != NULL);
	assert(DEVICE_CONTEXT != NULL);
	indigo_result result = INDIGO_OK;
	if ((result = indigo_device_enumerate_properties(device, client, property)) == INDIGO_OK) {
		if (IS_CONNECTED) {
			if (indigo_property_match(WHEEL_SLOT_PROPERTY, property))
				indigo_define_property(device, WHEEL_SLOT_PROPERTY, NULL);
			if (indigo_property_match(WHEEL_SLOT_NAME_PROPERTY, property))
				indigo_define_property(device, WHEEL_SLOT_NAME_PROPERTY, NULL);
		}
	}
	return result;
}

indigo_result indigo_wheel_change_property(indigo_device *device, indigo_client *client, indigo_property *property) {
	assert(device != NULL);
	assert(DEVICE_CONTEXT != NULL);
	assert(property != NULL);
	if (indigo_property_match(CONNECTION_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- CONNECTION
		if (IS_CONNECTED) {
			indigo_define_property(device, WHEEL_SLOT_PROPERTY, NULL);
			indigo_define_property(device, WHEEL_SLOT_NAME_PROPERTY, NULL);
		} else {
			indigo_delete_property(device, WHEEL_SLOT_PROPERTY, NULL);
			indigo_delete_property(device, WHEEL_SLOT_NAME_PROPERTY, NULL);
		}
		// -------------------------------------------------------------------------------- WHEEL_SLOT_NAME
	} else if (indigo_property_match(WHEEL_SLOT_NAME_PROPERTY, property)) {
		indigo_property_copy_values(WHEEL_SLOT_NAME_PROPERTY, property, false);
		WHEEL_SLOT_NAME_PROPERTY->state = INDIGO_OK_STATE;
		indigo_update_property(device, WHEEL_SLOT_NAME_PROPERTY, NULL);
		return INDIGO_OK;
	} else if (indigo_property_match(CONFIG_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- CONFIG
		if (indigo_switch_match(CONFIG_SAVE_ITEM, property)) {
			indigo_save_property(device, NULL, WHEEL_SLOT_NAME_PROPERTY);
		}
		// --------------------------------------------------------------------------------
	}
	return indigo_device_change_property(device, client, property);
}

indigo_result indigo_wheel_detach(indigo_device *device) {
	assert(device != NULL);
	indigo_release_property(WHEEL_SLOT_PROPERTY);
	indigo_release_property(WHEEL_SLOT_NAME_PROPERTY);
	return indigo_device_detach(device);
}
