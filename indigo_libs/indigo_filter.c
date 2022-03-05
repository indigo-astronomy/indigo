// Copyright (c) 2018 CloudMakers, s. r. o.
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

/** INDIGO Filter agent base
 \file indigo_filter.c
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

#include <indigo/indigo_filter.h>

static int interface_mask[INDIGO_FILTER_LIST_COUNT] = { INDIGO_INTERFACE_CCD, INDIGO_INTERFACE_WHEEL, INDIGO_INTERFACE_FOCUSER, INDIGO_INTERFACE_MOUNT, INDIGO_INTERFACE_GUIDER, INDIGO_INTERFACE_DOME, INDIGO_INTERFACE_GPS, INDIGO_INTERFACE_AUX_JOYSTICK, INDIGO_INTERFACE_AUX, INDIGO_INTERFACE_AUX, INDIGO_INTERFACE_AUX, INDIGO_INTERFACE_AUX };
static char *property_name_prefix[INDIGO_FILTER_LIST_COUNT] = { "CCD_", "WHEEL_", "FOCUSER_", "MOUNT_", "GUIDER_", "DOME_", "GPS_", "JOYSTICK_", "AUX_1_", "AUX_2_", "AUX_3_", "AUX_4_" };
static int property_name_prefix_len[INDIGO_FILTER_LIST_COUNT] = { 4, 6, 8, 6, 7, 5, 4, 9, 6, 6, 6, 6 };
static char *property_name_label[INDIGO_FILTER_LIST_COUNT] = { "CCD ", "Wheel ", "Focuser ", "Mount ", "Guider ", "Dome ", "GPS ", "Joystick", "AUX #1 ", "AUX #2 ", "AUX #3 ", "AUX #4 " };

indigo_result indigo_filter_device_attach(indigo_device *device, const char* driver_name, unsigned version, indigo_device_interface device_interface) {
	assert(device != NULL);
	if (FILTER_DEVICE_CONTEXT == NULL) {
		device->device_context = indigo_safe_malloc(sizeof(indigo_filter_context));
	}
	FILTER_DEVICE_CONTEXT->device = device;
	if (FILTER_DEVICE_CONTEXT != NULL) {
		if (indigo_device_attach(device, driver_name, version, INDIGO_INTERFACE_AGENT | device_interface) == INDIGO_OK) {
			CONNECTION_PROPERTY->hidden = true;
			// -------------------------------------------------------------------------------- CCD property
			FILTER_CCD_LIST_PROPERTY = indigo_init_switch_property(NULL, device->name, FILTER_CCD_LIST_PROPERTY_NAME, "Main", "Camera list", INDIGO_OK_STATE, INDIGO_RW_PERM, INDIGO_ONE_OF_MANY_RULE, INDIGO_FILTER_MAX_DEVICES);
			if (FILTER_CCD_LIST_PROPERTY == NULL)
				return INDIGO_FAILED;
			FILTER_CCD_LIST_PROPERTY->hidden = true;
			FILTER_CCD_LIST_PROPERTY->count = 1;
			indigo_init_switch_item(FILTER_CCD_LIST_PROPERTY->items, FILTER_DEVICE_LIST_NONE_ITEM_NAME, "No camera", true);
			// -------------------------------------------------------------------------------- wheel property
			FILTER_WHEEL_LIST_PROPERTY = indigo_init_switch_property(NULL, device->name, FILTER_WHEEL_LIST_PROPERTY_NAME, "Main", "Wheel list", INDIGO_OK_STATE, INDIGO_RW_PERM, INDIGO_ONE_OF_MANY_RULE, INDIGO_FILTER_MAX_DEVICES);
			if (FILTER_WHEEL_LIST_PROPERTY == NULL)
				return INDIGO_FAILED;
			FILTER_WHEEL_LIST_PROPERTY->hidden = true;
			FILTER_WHEEL_LIST_PROPERTY->count = 1;
			indigo_init_switch_item(FILTER_WHEEL_LIST_PROPERTY->items, FILTER_DEVICE_LIST_NONE_ITEM_NAME, "No wheel", true);
			// -------------------------------------------------------------------------------- focuser property
			FILTER_FOCUSER_LIST_PROPERTY = indigo_init_switch_property(NULL, device->name, FILTER_FOCUSER_LIST_PROPERTY_NAME, "Main", "Focuser list", INDIGO_OK_STATE, INDIGO_RW_PERM, INDIGO_ONE_OF_MANY_RULE, INDIGO_FILTER_MAX_DEVICES);
			if (FILTER_FOCUSER_LIST_PROPERTY == NULL)
				return INDIGO_FAILED;
			FILTER_FOCUSER_LIST_PROPERTY->hidden = true;
			FILTER_FOCUSER_LIST_PROPERTY->count = 1;
			indigo_init_switch_item(FILTER_FOCUSER_LIST_PROPERTY->items, FILTER_DEVICE_LIST_NONE_ITEM_NAME, "No focuser", true);
			// -------------------------------------------------------------------------------- mount property
			FILTER_MOUNT_LIST_PROPERTY = indigo_init_switch_property(NULL, device->name, FILTER_MOUNT_LIST_PROPERTY_NAME, "Main", "Mount list", INDIGO_OK_STATE, INDIGO_RW_PERM, INDIGO_ONE_OF_MANY_RULE, INDIGO_FILTER_MAX_DEVICES);
			if (FILTER_MOUNT_LIST_PROPERTY == NULL)
				return INDIGO_FAILED;
			FILTER_MOUNT_LIST_PROPERTY->hidden = true;
			FILTER_MOUNT_LIST_PROPERTY->count = 1;
			indigo_init_switch_item(FILTER_MOUNT_LIST_PROPERTY->items, FILTER_DEVICE_LIST_NONE_ITEM_NAME, "No mount", true);
			// -------------------------------------------------------------------------------- guider property
			FILTER_GUIDER_LIST_PROPERTY = indigo_init_switch_property(NULL, device->name, FILTER_GUIDER_LIST_PROPERTY_NAME, "Main", "Guider list", INDIGO_OK_STATE, INDIGO_RW_PERM, INDIGO_ONE_OF_MANY_RULE, INDIGO_FILTER_MAX_DEVICES);
			if (FILTER_GUIDER_LIST_PROPERTY == NULL)
				return INDIGO_FAILED;
			FILTER_GUIDER_LIST_PROPERTY->hidden = true;
			FILTER_GUIDER_LIST_PROPERTY->count = 1;
			indigo_init_switch_item(FILTER_GUIDER_LIST_PROPERTY->items, FILTER_DEVICE_LIST_NONE_ITEM_NAME, "No guider", true);
			// -------------------------------------------------------------------------------- dome property
			FILTER_DOME_LIST_PROPERTY = indigo_init_switch_property(NULL, device->name, FILTER_DOME_LIST_PROPERTY_NAME, "Main", "Dome list", INDIGO_OK_STATE, INDIGO_RW_PERM, INDIGO_ONE_OF_MANY_RULE, INDIGO_FILTER_MAX_DEVICES);
			if (FILTER_DOME_LIST_PROPERTY == NULL)
				return INDIGO_FAILED;
			FILTER_DOME_LIST_PROPERTY->hidden = true;
			FILTER_DOME_LIST_PROPERTY->count = 1;
			indigo_init_switch_item(FILTER_DOME_LIST_PROPERTY->items, FILTER_DEVICE_LIST_NONE_ITEM_NAME, "No dome", true);
			// -------------------------------------------------------------------------------- GPS property
			FILTER_GPS_LIST_PROPERTY = indigo_init_switch_property(NULL, device->name, FILTER_GPS_LIST_PROPERTY_NAME, "Main", "GPS list", INDIGO_OK_STATE, INDIGO_RW_PERM, INDIGO_ONE_OF_MANY_RULE, INDIGO_FILTER_MAX_DEVICES);
			if (FILTER_GPS_LIST_PROPERTY == NULL)
				return INDIGO_FAILED;
			FILTER_GPS_LIST_PROPERTY->hidden = true;
			FILTER_GPS_LIST_PROPERTY->count = 1;
			indigo_init_switch_item(FILTER_GPS_LIST_PROPERTY->items, FILTER_DEVICE_LIST_NONE_ITEM_NAME, "No GPS", true);
			// -------------------------------------------------------------------------------- Joystick property
			FILTER_JOYSTICK_LIST_PROPERTY = indigo_init_switch_property(NULL, device->name, FILTER_JOYSTICK_LIST_PROPERTY_NAME, "Main", "Joystick list", INDIGO_OK_STATE, INDIGO_RW_PERM, INDIGO_ONE_OF_MANY_RULE, INDIGO_FILTER_MAX_DEVICES);
			if (FILTER_JOYSTICK_LIST_PROPERTY == NULL)
				return INDIGO_FAILED;
			FILTER_JOYSTICK_LIST_PROPERTY->hidden = true;
			FILTER_JOYSTICK_LIST_PROPERTY->count = 1;
			indigo_init_switch_item(FILTER_JOYSTICK_LIST_PROPERTY->items, FILTER_DEVICE_LIST_NONE_ITEM_NAME, "No joystick", true);
			// -------------------------------------------------------------------------------- AUX #1 property
			FILTER_AUX_1_LIST_PROPERTY = indigo_init_switch_property(NULL, device->name, FILTER_AUX_1_LIST_PROPERTY_NAME, "Main", "AUX #1 list", INDIGO_OK_STATE, INDIGO_RW_PERM, INDIGO_ONE_OF_MANY_RULE, INDIGO_FILTER_MAX_DEVICES);
			if (FILTER_AUX_1_LIST_PROPERTY == NULL)
				return INDIGO_FAILED;
			FILTER_AUX_1_LIST_PROPERTY->hidden = true;
			FILTER_AUX_1_LIST_PROPERTY->count = 1;
			indigo_init_switch_item(FILTER_AUX_1_LIST_PROPERTY->items, FILTER_DEVICE_LIST_NONE_ITEM_NAME, "No AUX device #1", true);
			// -------------------------------------------------------------------------------- AUX #2 property
			FILTER_AUX_2_LIST_PROPERTY = indigo_init_switch_property(NULL, device->name, FILTER_AUX_2_LIST_PROPERTY_NAME, "Main", "AUX #2 list", INDIGO_OK_STATE, INDIGO_RW_PERM, INDIGO_ONE_OF_MANY_RULE, INDIGO_FILTER_MAX_DEVICES);
			if (FILTER_AUX_2_LIST_PROPERTY == NULL)
				return INDIGO_FAILED;
			FILTER_AUX_2_LIST_PROPERTY->hidden = true;
			FILTER_AUX_2_LIST_PROPERTY->count = 1;
			indigo_init_switch_item(FILTER_AUX_2_LIST_PROPERTY->items, FILTER_DEVICE_LIST_NONE_ITEM_NAME, "No AUX device #2", true);
			// -------------------------------------------------------------------------------- AUX #3 property
			FILTER_AUX_3_LIST_PROPERTY = indigo_init_switch_property(NULL, device->name, FILTER_AUX_3_LIST_PROPERTY_NAME, "Main", "AUX #3 list", INDIGO_OK_STATE, INDIGO_RW_PERM, INDIGO_ONE_OF_MANY_RULE, INDIGO_FILTER_MAX_DEVICES);
			if (FILTER_AUX_3_LIST_PROPERTY == NULL)
				return INDIGO_FAILED;
			FILTER_AUX_3_LIST_PROPERTY->hidden = true;
			FILTER_AUX_3_LIST_PROPERTY->count = 1;
			indigo_init_switch_item(FILTER_AUX_3_LIST_PROPERTY->items, FILTER_DEVICE_LIST_NONE_ITEM_NAME, "No AUX device #3", true);
			// -------------------------------------------------------------------------------- AUX #4 property
			FILTER_AUX_4_LIST_PROPERTY = indigo_init_switch_property(NULL, device->name, FILTER_AUX_4_LIST_PROPERTY_NAME, "Main", "AUX #4 list", INDIGO_OK_STATE, INDIGO_RW_PERM, INDIGO_ONE_OF_MANY_RULE, INDIGO_FILTER_MAX_DEVICES);
			if (FILTER_AUX_4_LIST_PROPERTY == NULL)
				return INDIGO_FAILED;
			FILTER_AUX_4_LIST_PROPERTY->hidden = true;
			FILTER_AUX_4_LIST_PROPERTY->count = 1;
			indigo_init_switch_item(FILTER_AUX_4_LIST_PROPERTY->items, FILTER_DEVICE_LIST_NONE_ITEM_NAME, "No AUX device #4", true);
			// -------------------------------------------------------------------------------- Related CCD property
			FILTER_RELATED_CCD_LIST_PROPERTY = indigo_init_switch_property(NULL, device->name, FILTER_RELATED_CCD_LIST_PROPERTY_NAME, "Main", "Related CCD list", INDIGO_OK_STATE, INDIGO_RW_PERM, INDIGO_ONE_OF_MANY_RULE, INDIGO_FILTER_MAX_DEVICES);
			if (FILTER_RELATED_CCD_LIST_PROPERTY == NULL)
				return INDIGO_FAILED;
			FILTER_RELATED_CCD_LIST_PROPERTY->hidden = true;
			FILTER_RELATED_CCD_LIST_PROPERTY->count = 1;
			indigo_init_switch_item(FILTER_RELATED_CCD_LIST_PROPERTY->items, FILTER_DEVICE_LIST_NONE_ITEM_NAME, "No camera", true);
			// -------------------------------------------------------------------------------- Related wheel property
			FILTER_RELATED_WHEEL_LIST_PROPERTY = indigo_init_switch_property(NULL, device->name, FILTER_RELATED_WHEEL_LIST_PROPERTY_NAME, "Main", "Related wheel list", INDIGO_OK_STATE, INDIGO_RW_PERM, INDIGO_ONE_OF_MANY_RULE, INDIGO_FILTER_MAX_DEVICES);
			if (FILTER_RELATED_WHEEL_LIST_PROPERTY == NULL)
				return INDIGO_FAILED;
			FILTER_RELATED_WHEEL_LIST_PROPERTY->hidden = true;
			FILTER_RELATED_WHEEL_LIST_PROPERTY->count = 1;
			indigo_init_switch_item(FILTER_RELATED_WHEEL_LIST_PROPERTY->items, FILTER_DEVICE_LIST_NONE_ITEM_NAME, "No wheel", true);
			// -------------------------------------------------------------------------------- Related focuser property
			FILTER_RELATED_FOCUSER_LIST_PROPERTY = indigo_init_switch_property(NULL, device->name, FILTER_RELATED_FOCUSER_LIST_PROPERTY_NAME, "Main", "Related focuser list", INDIGO_OK_STATE, INDIGO_RW_PERM, INDIGO_ONE_OF_MANY_RULE, INDIGO_FILTER_MAX_DEVICES);
			if (FILTER_RELATED_FOCUSER_LIST_PROPERTY == NULL)
				return INDIGO_FAILED;
			FILTER_RELATED_FOCUSER_LIST_PROPERTY->hidden = true;
			FILTER_RELATED_FOCUSER_LIST_PROPERTY->count = 1;
			indigo_init_switch_item(FILTER_RELATED_FOCUSER_LIST_PROPERTY->items, FILTER_DEVICE_LIST_NONE_ITEM_NAME, "No focuser", true);
			// -------------------------------------------------------------------------------- Related mount property
			FILTER_RELATED_MOUNT_LIST_PROPERTY = indigo_init_switch_property(NULL, device->name, FILTER_RELATED_MOUNT_LIST_PROPERTY_NAME, "Main", "Related mount list", INDIGO_OK_STATE, INDIGO_RW_PERM, INDIGO_ONE_OF_MANY_RULE, INDIGO_FILTER_MAX_DEVICES);
			if (FILTER_RELATED_MOUNT_LIST_PROPERTY == NULL)
				return INDIGO_FAILED;
			FILTER_RELATED_MOUNT_LIST_PROPERTY->hidden = true;
			FILTER_RELATED_MOUNT_LIST_PROPERTY->count = 1;
			indigo_init_switch_item(FILTER_RELATED_MOUNT_LIST_PROPERTY->items, FILTER_DEVICE_LIST_NONE_ITEM_NAME, "No mount", true);
			// -------------------------------------------------------------------------------- Related guider property
			FILTER_RELATED_GUIDER_LIST_PROPERTY = indigo_init_switch_property(NULL, device->name, FILTER_RELATED_GUIDER_LIST_PROPERTY_NAME, "Main", "Related guider list", INDIGO_OK_STATE, INDIGO_RW_PERM, INDIGO_ONE_OF_MANY_RULE, INDIGO_FILTER_MAX_DEVICES);
			if (FILTER_RELATED_GUIDER_LIST_PROPERTY == NULL)
				return INDIGO_FAILED;
			FILTER_RELATED_GUIDER_LIST_PROPERTY->hidden = true;
			FILTER_RELATED_GUIDER_LIST_PROPERTY->count = 1;
			indigo_init_switch_item(FILTER_RELATED_GUIDER_LIST_PROPERTY->items, FILTER_DEVICE_LIST_NONE_ITEM_NAME, "No guider", true);
			// -------------------------------------------------------------------------------- Related dome property
			FILTER_RELATED_DOME_LIST_PROPERTY = indigo_init_switch_property(NULL, device->name, FILTER_RELATED_DOME_LIST_PROPERTY_NAME, "Main", "Related dome list", INDIGO_OK_STATE, INDIGO_RW_PERM, INDIGO_ONE_OF_MANY_RULE, INDIGO_FILTER_MAX_DEVICES);
			if (FILTER_RELATED_DOME_LIST_PROPERTY == NULL)
				return INDIGO_FAILED;
			FILTER_RELATED_DOME_LIST_PROPERTY->hidden = true;
			FILTER_RELATED_DOME_LIST_PROPERTY->count = 1;
			indigo_init_switch_item(FILTER_RELATED_DOME_LIST_PROPERTY->items, FILTER_DEVICE_LIST_NONE_ITEM_NAME, "No dome", true);
			// -------------------------------------------------------------------------------- Related GPS property
			FILTER_RELATED_GPS_LIST_PROPERTY = indigo_init_switch_property(NULL, device->name, FILTER_RELATED_GPS_LIST_PROPERTY_NAME, "Main", "Related GPS list", INDIGO_OK_STATE, INDIGO_RW_PERM, INDIGO_ONE_OF_MANY_RULE, INDIGO_FILTER_MAX_DEVICES);
			if (FILTER_RELATED_GPS_LIST_PROPERTY == NULL)
				return INDIGO_FAILED;
			FILTER_RELATED_GPS_LIST_PROPERTY->hidden = true;
			FILTER_RELATED_GPS_LIST_PROPERTY->count = 1;
			indigo_init_switch_item(FILTER_RELATED_GPS_LIST_PROPERTY->items, FILTER_DEVICE_LIST_NONE_ITEM_NAME, "No GPS", true);
			// -------------------------------------------------------------------------------- Related joystick property
			FILTER_RELATED_JOYSTICK_LIST_PROPERTY = indigo_init_switch_property(NULL, device->name, FILTER_RELATED_JOYSTICK_LIST_PROPERTY_NAME, "Main", "Related joystick", INDIGO_OK_STATE, INDIGO_RW_PERM, INDIGO_ONE_OF_MANY_RULE, INDIGO_FILTER_MAX_DEVICES);
			if (FILTER_RELATED_JOYSTICK_LIST_PROPERTY == NULL)
				return INDIGO_FAILED;
			FILTER_RELATED_JOYSTICK_LIST_PROPERTY->hidden = true;
			FILTER_RELATED_JOYSTICK_LIST_PROPERTY->count = 1;
			indigo_init_switch_item(FILTER_RELATED_JOYSTICK_LIST_PROPERTY->items, FILTER_DEVICE_LIST_NONE_ITEM_NAME, "No joystick", true);
			// -------------------------------------------------------------------------------- Related AUX #1 property
			FILTER_RELATED_AUX_1_LIST_PROPERTY = indigo_init_switch_property(NULL, device->name, FILTER_RELATED_AUX_1_LIST_PROPERTY_NAME, "Main", "Related AUX #1 list", INDIGO_OK_STATE, INDIGO_RW_PERM, INDIGO_ONE_OF_MANY_RULE, INDIGO_FILTER_MAX_DEVICES);
			if (FILTER_RELATED_AUX_1_LIST_PROPERTY == NULL)
				return INDIGO_FAILED;
			FILTER_RELATED_AUX_1_LIST_PROPERTY->hidden = true;
			FILTER_RELATED_AUX_1_LIST_PROPERTY->count = 1;
			indigo_init_switch_item(FILTER_RELATED_AUX_1_LIST_PROPERTY->items, FILTER_DEVICE_LIST_NONE_ITEM_NAME, "No AUX device #1", true);
			// -------------------------------------------------------------------------------- Related AUX #2 property
			FILTER_RELATED_AUX_2_LIST_PROPERTY = indigo_init_switch_property(NULL, device->name, FILTER_RELATED_AUX_2_LIST_PROPERTY_NAME, "Main", "Related AUX #2 list", INDIGO_OK_STATE, INDIGO_RW_PERM, INDIGO_ONE_OF_MANY_RULE, INDIGO_FILTER_MAX_DEVICES);
			if (FILTER_RELATED_AUX_2_LIST_PROPERTY == NULL)
				return INDIGO_FAILED;
			FILTER_RELATED_AUX_2_LIST_PROPERTY->hidden = true;
			FILTER_RELATED_AUX_2_LIST_PROPERTY->count = 1;
			indigo_init_switch_item(FILTER_RELATED_AUX_2_LIST_PROPERTY->items, FILTER_DEVICE_LIST_NONE_ITEM_NAME, "No AUX device #2", true);
			// -------------------------------------------------------------------------------- Related AUX #3 property
			FILTER_RELATED_AUX_3_LIST_PROPERTY = indigo_init_switch_property(NULL, device->name, FILTER_RELATED_AUX_3_LIST_PROPERTY_NAME, "Main", "Related AUX #3 list", INDIGO_OK_STATE, INDIGO_RW_PERM, INDIGO_ONE_OF_MANY_RULE, INDIGO_FILTER_MAX_DEVICES);
			if (FILTER_RELATED_AUX_3_LIST_PROPERTY == NULL)
				return INDIGO_FAILED;
			FILTER_RELATED_AUX_3_LIST_PROPERTY->hidden = true;
			FILTER_RELATED_AUX_3_LIST_PROPERTY->count = 1;
			indigo_init_switch_item(FILTER_RELATED_AUX_3_LIST_PROPERTY->items, FILTER_DEVICE_LIST_NONE_ITEM_NAME, "No AUX device #3", true);
			// -------------------------------------------------------------------------------- Related AUX #4 property
			FILTER_RELATED_AUX_4_LIST_PROPERTY = indigo_init_switch_property(NULL, device->name, FILTER_RELATED_AUX_4_LIST_PROPERTY_NAME, "Main", "Related AUX #4 list", INDIGO_OK_STATE, INDIGO_RW_PERM, INDIGO_ONE_OF_MANY_RULE, INDIGO_FILTER_MAX_DEVICES);
			if (FILTER_RELATED_AUX_4_LIST_PROPERTY == NULL)
				return INDIGO_FAILED;
			FILTER_RELATED_AUX_4_LIST_PROPERTY->hidden = true;
			FILTER_RELATED_AUX_4_LIST_PROPERTY->count = 1;
			indigo_init_switch_item(FILTER_RELATED_AUX_4_LIST_PROPERTY->items, FILTER_DEVICE_LIST_NONE_ITEM_NAME, "No AUX device #4", true);
			// -------------------------------------------------------------------------------- Related agents property
			FILTER_RELATED_AGENT_LIST_PROPERTY = indigo_init_switch_property(NULL, device->name, FILTER_RELATED_AGENT_LIST_PROPERTY_NAME, "Main", "Related agent list", INDIGO_OK_STATE, INDIGO_RW_PERM, INDIGO_ANY_OF_MANY_RULE, INDIGO_FILTER_MAX_DEVICES);
			if (FILTER_RELATED_AGENT_LIST_PROPERTY == NULL)
				return INDIGO_FAILED;
			FILTER_RELATED_AGENT_LIST_PROPERTY->hidden = true;
			FILTER_RELATED_AGENT_LIST_PROPERTY->count = 0;
			// --------------------------------------------------------------------------------
			CONFIG_PROPERTY->hidden = true;
			PROFILE_PROPERTY->hidden = true;
			return INDIGO_OK;
		}
	}
	return INDIGO_FAILED;
}

indigo_result indigo_filter_enumerate_properties(indigo_device *device, indigo_client *client, indigo_property *property) {
	assert(device != NULL);
	assert(DEVICE_CONTEXT != NULL);
	for (int i = 0; i < INDIGO_FILTER_LIST_COUNT; i++) {
		indigo_property *device_list = FILTER_DEVICE_CONTEXT->filter_device_list_properties[i];
		if (indigo_property_match(device_list, property))
			indigo_define_property(device, device_list, NULL);
		device_list = FILTER_DEVICE_CONTEXT->filter_related_device_list_properties[i];
		if (indigo_property_match(device_list, property))
			indigo_define_property(device, device_list, NULL);
	}
	if (indigo_property_match(FILTER_DEVICE_CONTEXT->filter_related_agent_list_property, property))
		indigo_define_property(device, FILTER_DEVICE_CONTEXT->filter_related_agent_list_property, NULL);
	for (int i = 0; i < INDIGO_FILTER_MAX_CACHED_PROPERTIES; i++) {
		indigo_property *cached_property = FILTER_DEVICE_CONTEXT->agent_property_cache[i];
		if (cached_property && indigo_property_match(cached_property, property))
			indigo_define_property(device, cached_property, NULL);
	}
	return indigo_device_enumerate_properties(device, client, property);
}

static void update_additional_instances(indigo_device *device) {
	int count = ADDITIONAL_INSTANCES_COUNT_ITEM->number.value;
	for (int i = 0; i < count; i++) {
		if (DEVICE_CONTEXT->additional_device_instances[i] == NULL) {
			indigo_device *additional_device = indigo_safe_malloc_copy(sizeof(indigo_device), device);
			snprintf(additional_device->name, INDIGO_NAME_SIZE, "%s #%d", device->name, i + 2);
			additional_device->lock = -1;
			additional_device->is_remote = false;
			additional_device->gp_bits = 0;
			additional_device->master_device = NULL;
			additional_device->last_result = 0;
			additional_device->access_token = 0;
			additional_device->device_context = indigo_safe_malloc(MALLOCED_SIZE(device->device_context));
			((indigo_device_context *)additional_device->device_context)->base_device = device;
			additional_device->private_data = indigo_safe_malloc(MALLOCED_SIZE(device->private_data));
			indigo_attach_device(additional_device);
			DEVICE_CONTEXT->additional_device_instances[i] = additional_device;
			indigo_client *agent_client = FILTER_DEVICE_CONTEXT->client;
			indigo_client *additional_client = indigo_safe_malloc_copy(sizeof(indigo_client), agent_client);
			snprintf(additional_client->name, INDIGO_NAME_SIZE, "%s #%d", agent_client->name, i + 2);
			additional_client->is_remote = false;
			additional_client->last_result = 0;
			additional_client->enable_blob_mode_records = NULL;
			additional_client->client_context = additional_device->device_context;
			indigo_attach_client(additional_client);
			FILTER_DEVICE_CONTEXT->additional_client_instances[i] = additional_client;
		}
	}
	for (int i = count; i < MAX_ADDITIONAL_INSTANCES; i++) {
		indigo_device *additional_device = DEVICE_CONTEXT->additional_device_instances[i];
		if (additional_device != NULL) {
			indigo_client *additional_client = FILTER_DEVICE_CONTEXT->additional_client_instances[i];
			indigo_detach_client(additional_client);
			free(additional_client);
			indigo_detach_device(additional_device);
			free(additional_device->private_data);
			free(additional_device);
			DEVICE_CONTEXT->additional_device_instances[i] = NULL;
		}
	}
	ADDITIONAL_INSTANCES_PROPERTY->state = INDIGO_OK_STATE;
	indigo_update_property(device, ADDITIONAL_INSTANCES_PROPERTY, NULL);
}

static indigo_result update_device_list(indigo_device *device, indigo_client *client, indigo_property *device_list, indigo_property *property, char *device_name) {
	for (int i = 0; i < property->count; i++) {
		if (property->items[i].sw.value) {
			for (int j = 0; j < device_list->count; j++) {
				if (device_list->items[j].sw.value) {
					if (!strcmp(property->items[i].name, device_list->items[j].name))
						return INDIGO_OK;
					break;
				}
			}
		}
	}
	device_list->state = INDIGO_BUSY_STATE;
	indigo_update_property(device, device_list, NULL);
	*device_name = 0;
	indigo_property *connection_property = indigo_init_switch_property(NULL, "", CONNECTION_PROPERTY_NAME, NULL, NULL, INDIGO_OK_STATE, INDIGO_RW_PERM, INDIGO_ONE_OF_MANY_RULE, 1);
	for (int i = 1; i < device_list->count; i++) {
		if (device_list->items[i].sw.value) {
			device_list->items[i].sw.value = false;
			strcpy(connection_property->device, device_list->items[i].name);
			indigo_property **device_cache = FILTER_DEVICE_CONTEXT->device_property_cache;
			indigo_property **agent_cache = FILTER_DEVICE_CONTEXT->agent_property_cache;
			for (int i = 0; i < INDIGO_FILTER_MAX_CACHED_PROPERTIES; i++) {
				indigo_property *device_property = device_cache[i];
				if (device_property && !strcmp(connection_property->device, device_property->device)) {
					device_cache[i] = NULL;
					if (agent_cache[i]) {
						indigo_delete_property(device, agent_cache[i], NULL);
						indigo_release_property(agent_cache[i]);
						agent_cache[i] = NULL;
					}
				}
			}
			indigo_init_switch_item(connection_property->items, CONNECTION_DISCONNECTED_ITEM_NAME, NULL, true);
			connection_property->access_token = indigo_get_device_or_master_token(connection_property->device);
			indigo_change_property(client, connection_property);
			break;
		}
	}
	indigo_property_copy_values(device_list, property, false);
	for (int i = 1; i < device_list->count; i++) {
		if (device_list->items[i].sw.value) {
			char *name = device_list->items[i].name;
			bool disconnected = true;
			for (int j = 0; j < INDIGO_FILTER_MAX_DEVICES; j++) {
				indigo_property *cached_connection_property = FILTER_DEVICE_CONTEXT->connection_property_cache[j];
				if (cached_connection_property != NULL && !strcmp(cached_connection_property->device, name)) {
					disconnected = cached_connection_property->state == INDIGO_OK_STATE || cached_connection_property->state == INDIGO_ALERT_STATE;
					if (disconnected) {
						disconnected = false;
						for (int k = 0; k < cached_connection_property->count; k++) {
							indigo_item *item = cached_connection_property->items + k;
							if (!strcmp(item->name, CONNECTION_DISCONNECTED_ITEM_NAME) && item->sw.value) {
								disconnected = true;
								break;
							}
						}
					}
					break;
				}
			}
			if (disconnected) {
				device_list->state = INDIGO_BUSY_STATE;
				indigo_update_property(device, device_list, NULL);
				strcpy(connection_property->device, name);
				indigo_init_switch_item(connection_property->items, CONNECTION_CONNECTED_ITEM_NAME, NULL, true);
				indigo_enumerate_properties(client, connection_property);
				connection_property->access_token = indigo_get_device_or_master_token(connection_property->device);
				indigo_change_property(client, connection_property);
			} else {
				indigo_set_switch(device_list, device_list->items, true);
				device_list->state = INDIGO_ALERT_STATE;
				indigo_update_property(device, device_list, "'%s' is already connected and maybe in use, please disconnect it first.", name);
			}
			indigo_release_property(connection_property);
			return INDIGO_OK;
		}
	}
	device_list->state = INDIGO_OK_STATE;
	indigo_update_property(device, device_list, NULL);
	indigo_release_property(connection_property);
	return INDIGO_OK;
}

static indigo_result update_related_device_list(indigo_device *device, indigo_property *device_list, indigo_property *property) {
	indigo_property_copy_values(device_list, property, false);
	for (int i = 1; i < device_list->count; i++) {
		if (device_list->items[i].sw.value) {
			device_list->state = INDIGO_OK_STATE;
			indigo_property all_properties;
			memset(&all_properties, 0, sizeof(all_properties));
			strcpy(all_properties.device, device_list->items[i].name);
			indigo_enumerate_properties(FILTER_DEVICE_CONTEXT->client, &all_properties);
			indigo_update_property(device, device_list, NULL);
			return INDIGO_OK;
		}
	}
	indigo_update_property(device, device_list, NULL);
	return INDIGO_OK;
}

static indigo_result update_related_agent_list(indigo_device *device, indigo_property *property) {
	indigo_property *device_list = FILTER_DEVICE_CONTEXT->filter_related_agent_list_property;
	indigo_property_copy_values(device_list, property, false);
	for (int i = 0; i < device_list->count; i++) {
		if (device_list->items[i].sw.value) {
			device_list->state = INDIGO_OK_STATE;
			indigo_property all_properties;
			memset(&all_properties, 0, sizeof(all_properties));
			strcpy(all_properties.device, device_list->items[i].name);
			indigo_enumerate_properties(FILTER_DEVICE_CONTEXT->client, &all_properties);
		}
	}
	indigo_update_property(device, device_list, NULL);
	return INDIGO_OK;
}


indigo_result indigo_filter_change_property(indigo_device *device, indigo_client *client, indigo_property *property) {
	assert(device != NULL);
	assert(DEVICE_CONTEXT != NULL);
	assert(property != NULL);
	for (int i = 0; i < INDIGO_FILTER_LIST_COUNT; i++) {
		indigo_property *device_list = FILTER_DEVICE_CONTEXT->filter_device_list_properties[i];
		if (indigo_property_match(device_list, property)) {
			if (FILTER_DEVICE_CONTEXT->running_process) {
				indigo_update_property(device, device_list, "You can't change selection now!");
				return INDIGO_OK;
			}
			return update_device_list(device, client, device_list, property, FILTER_DEVICE_CONTEXT->device_name[i]);
		}
		device_list = FILTER_DEVICE_CONTEXT->filter_related_device_list_properties[i];
		if (indigo_property_match(device_list, property))
			return update_related_device_list(device, device_list, property);
	}
	if (indigo_property_match(FILTER_DEVICE_CONTEXT->filter_related_agent_list_property, property))
		return update_related_agent_list(device, property);
	indigo_property **agent_cache = FILTER_DEVICE_CONTEXT->agent_property_cache;
	for (int i = 0; i < INDIGO_FILTER_MAX_CACHED_PROPERTIES; i++) {
		if (agent_cache[i] && indigo_property_match(agent_cache[i], property)) {
			int size = sizeof(indigo_property) + property->count * sizeof(indigo_item);
			indigo_property *copy = (indigo_property *)malloc(size);
			memcpy(copy, property, size);
			strcpy(copy->device, FILTER_DEVICE_CONTEXT->device_property_cache[i]->device);
			strcpy(copy->name, FILTER_DEVICE_CONTEXT->device_property_cache[i]->name);
			if (copy->type == INDIGO_TEXT_VECTOR) {
				for (int k = 0; k < copy->count; k++) {
					indigo_item *item = copy->items + k;
					if (item->text.long_value) {
						item->text.long_value = NULL;
						indigo_set_text_item_value(item, property->items[k].text.long_value);
					}
				}
			}
			copy->access_token = indigo_get_device_or_master_token(copy->device);
			indigo_change_property(client, copy);
			indigo_release_property(copy);
			return INDIGO_OK;
		}
	}
	if (indigo_property_match(ADDITIONAL_INSTANCES_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- ADDITIONAL_INSTANCES
		assert(DEVICE_CONTEXT->base_device == NULL);
		indigo_property_copy_values(ADDITIONAL_INSTANCES_PROPERTY, property, false);
		if (FILTER_DEVICE_CONTEXT->client != NULL)
			update_additional_instances(device);
		return INDIGO_OK;
		// --------------------------------------------------------------------------------
	}
	return indigo_device_change_property(device, client, property);
}

indigo_result indigo_filter_device_detach(indigo_device *device) {
	assert(device != NULL);
	for (int i = 0; i < INDIGO_FILTER_LIST_COUNT; i++) {
		indigo_release_property(FILTER_DEVICE_CONTEXT->filter_device_list_properties[i]);
		indigo_release_property(FILTER_DEVICE_CONTEXT->filter_related_device_list_properties[i]);
	}
	indigo_release_property(FILTER_DEVICE_CONTEXT->filter_related_agent_list_property);
	for (int i = 0; i < MAX_ADDITIONAL_INSTANCES; i++) {
		indigo_client *additional_client = FILTER_DEVICE_CONTEXT->additional_client_instances[i];
		if (additional_client != NULL) {
			indigo_detach_client(additional_client);
			free(additional_client);
			FILTER_DEVICE_CONTEXT->additional_client_instances[i] = NULL;
		}
	}
	return indigo_device_detach(device);
}

indigo_result indigo_filter_client_attach(indigo_client *client) {
	assert(client != NULL);
	assert (FILTER_CLIENT_CONTEXT != NULL);
	FILTER_CLIENT_CONTEXT->client = client;
	indigo_property **device_cache = FILTER_CLIENT_CONTEXT->device_property_cache;
	indigo_property **agent_cache = FILTER_CLIENT_CONTEXT->device_property_cache;
	for (int i = 0; i < INDIGO_FILTER_MAX_CACHED_PROPERTIES; i++) {
		device_cache[i] = NULL;
		agent_cache[i] = NULL;
	}
	indigo_property all_properties;
	memset(&all_properties, 0, sizeof(all_properties));
	indigo_enumerate_properties(client, &all_properties);
	update_additional_instances(FILTER_CLIENT_CONTEXT->device);
	return INDIGO_OK;
}

static bool device_in_list(indigo_property *device_list, indigo_property *property) {
	int count = device_list->count;
	for (int i = 0; i < count; i++) {
		if (!strcmp(property->device, device_list->items[i].name)) {
			return true;
		}
	}
	return false;
}

static void add_to_list(indigo_device *device, indigo_property *device_list, indigo_property *property) {
	int count = device_list->count;
	if (count < INDIGO_FILTER_MAX_DEVICES) {
		indigo_delete_property(device, device_list, NULL);
		indigo_init_switch_item(device_list->items + count, property->device, property->device, false);
		device_list->count++;
		indigo_define_property(device, device_list, NULL);
	} else {
		indigo_error("[%s:%d] Max device count reached", __FUNCTION__, __LINE__);
	}
}

indigo_result indigo_filter_define_property(indigo_client *client, indigo_device *device, indigo_property *property, const char *message) {
	if (device == FILTER_CLIENT_CONTEXT->device)
		return INDIGO_OK;
	device = FILTER_CLIENT_CONTEXT->device;
	indigo_property **device_cache = FILTER_CLIENT_CONTEXT->device_property_cache;
	indigo_property **agent_cache = FILTER_CLIENT_CONTEXT->agent_property_cache;
	if (property->type == INDIGO_BLOB_VECTOR) {
		indigo_enable_blob(client, property, INDIGO_ENABLE_BLOB_URL);
	}
	if (!strcmp(property->name, INFO_PROPERTY_NAME)) {
		indigo_item *interface = indigo_get_item(property, INFO_DEVICE_INTERFACE_ITEM_NAME);
		if (interface) {
			int mask = atoi(interface->text.value);
			indigo_property *tmp;
			if ((mask & INDIGO_INTERFACE_AGENT) == INDIGO_INTERFACE_AGENT) {
				tmp = FILTER_CLIENT_CONTEXT->filter_related_agent_list_property;
				if (!tmp->hidden && !device_in_list(tmp, property) && (FILTER_CLIENT_CONTEXT->validate_related_agent == NULL || FILTER_CLIENT_CONTEXT->validate_related_agent(FILTER_CLIENT_CONTEXT->device, property, mask)))
					add_to_list(device, tmp, property);
			} else {
				for (int i = 0; i < INDIGO_FILTER_LIST_COUNT; i++) {
					if ((mask & interface_mask[i]) == interface_mask[i]) {
						tmp = FILTER_CLIENT_CONTEXT->filter_device_list_properties[i];
						if (!tmp->hidden && !device_in_list(tmp, property) && (FILTER_CLIENT_CONTEXT->validate_device == NULL || FILTER_CLIENT_CONTEXT->validate_device(FILTER_CLIENT_CONTEXT->device, i, property, mask)))
							add_to_list(device, tmp, property);
						tmp = FILTER_CLIENT_CONTEXT->filter_related_device_list_properties[i];
						if (!tmp->hidden && !device_in_list(tmp, property) && (FILTER_CLIENT_CONTEXT->validate_related_device == NULL || FILTER_CLIENT_CONTEXT->validate_related_device(FILTER_CLIENT_CONTEXT->device, i, property, mask)))
							add_to_list(device, tmp, property);
					}
				}
			}
			return INDIGO_OK;
		}
	} else if (!strcmp(property->name, CONNECTION_PROPERTY_NAME)) {
		int free_index = -1;
		for (int i = 0; i < INDIGO_FILTER_MAX_DEVICES; i++) {
			indigo_property *connection_property = FILTER_CLIENT_CONTEXT->connection_property_cache[i];
			if (connection_property == NULL)
				free_index = i;
			else if (connection_property == property) {
				free_index = i;
				break;
			}
		}
		if (free_index == -1 && property == NULL) {
			indigo_error("[%s:%d] Max cached device count reached", __FUNCTION__, __LINE__);
		}
		if (free_index >= 0 && FILTER_CLIENT_CONTEXT->connection_property_cache[free_index] == NULL) {
			FILTER_CLIENT_CONTEXT->connection_property_cache[free_index] = property;
			FILTER_CLIENT_CONTEXT->connection_property_device_cache[free_index] = strdup(property->device);
		}
		if (property->state != INDIGO_BUSY_STATE) {
			for (int i = 0; i < INDIGO_FILTER_LIST_COUNT; i++) {
				indigo_item *connected_device = indigo_get_item(property, CONNECTION_CONNECTED_ITEM_NAME);
				indigo_property *device_list = FILTER_CLIENT_CONTEXT->filter_device_list_properties[i];
				for (int j = 1; j < device_list->count; j++) {
					if (!strcmp(property->device, device_list->items[j].name) && device_list->items[j].sw.value) {
						if (device_list->state == INDIGO_BUSY_STATE) {
							if (connected_device->sw.value && property->state == INDIGO_OK_STATE) {
								indigo_property *configuration_property = indigo_init_switch_property(NULL, property->device, CONFIG_PROPERTY_NAME, NULL, NULL, INDIGO_OK_STATE, INDIGO_RW_PERM, INDIGO_ANY_OF_MANY_RULE, 1);
								indigo_init_switch_item(configuration_property->items, CONFIG_LOAD_ITEM_NAME, NULL, true);
								configuration_property->access_token = indigo_get_device_or_master_token(configuration_property->device);
								indigo_change_property(client, configuration_property);
								indigo_release_property(configuration_property);
								strcpy(FILTER_CLIENT_CONTEXT->device_name[i], property->device);
								device_list->state = INDIGO_OK_STATE;
								indigo_property all_properties;
								memset(&all_properties, 0, sizeof(all_properties));
								strcpy(all_properties.device, property->device);
								indigo_enumerate_properties(client, &all_properties);
							}
							indigo_update_property(device, device_list, NULL);
							return INDIGO_OK;
						} else if (device_list->state == INDIGO_OK_STATE && !connected_device->sw.value) {
							indigo_set_switch(device_list, device_list->items, true);
							device_list->state = INDIGO_ALERT_STATE;
							strcpy(FILTER_CLIENT_CONTEXT->device_name[i], "");
							indigo_update_property(device, device_list, NULL);
							return INDIGO_OK;
						}
					}
				}
			}
		}
	} else if (!strcmp(property->group, MAIN_GROUP)) {
		return INDIGO_OK;
	} else {
		for (int i = 0; i < INDIGO_FILTER_LIST_COUNT; i++) {
			char *name_prefix = property_name_prefix[i];
			int name_prefix_length = property_name_prefix_len[i];
			if (strcmp(property->device, FILTER_CLIENT_CONTEXT->device_name[i]))
				continue;
			bool found = false;
			for (int j = 0; j < INDIGO_FILTER_MAX_CACHED_PROPERTIES; j++) {
				if (device_cache[j] == property) {
					found = true;
					break;
				}
			}
			if (!found) {
				int free_index;
				for (free_index = 0; free_index < INDIGO_FILTER_MAX_CACHED_PROPERTIES; free_index++) {
					if (device_cache[free_index] == NULL) {
						device_cache[free_index] = property;
						int size = sizeof(indigo_property) + property->count * sizeof(indigo_item);
						indigo_property *copy = (indigo_property *)malloc(size);
						memcpy(copy, property, size);
						strcpy(copy->device, device->name);
						bool translate = strncmp(name_prefix, copy->name, name_prefix_length);
						if (translate && !strcmp(name_prefix, "CCD_") && !strncmp(copy->name, "DSLR_", 5))
							translate = false;
						if (translate) {
							strcpy(copy->name, name_prefix);
							strcat(copy->name, property->name);
							strcpy(copy->label, property_name_label[i]);
							strcat(copy->label, property->label);
						}
						if (copy->type == INDIGO_TEXT_VECTOR) {
							for (int k = 0; k < copy->count; k++) {
								indigo_item *item = copy->items + k;
								if (item->text.long_value) {
									item->text.long_value = NULL;
									indigo_set_text_item_value(item, property->items[k].text.long_value);
								}
							}
						}
						agent_cache[free_index] = copy;
						indigo_define_property(device, copy, message);
						break;
					}
				}
				if (free_index == INDIGO_FILTER_MAX_CACHED_PROPERTIES) {
					indigo_error("[%s:%d] Max cached properties count reached", __FUNCTION__, __LINE__);
				}
			}
			return INDIGO_OK;
		}
	}
	return INDIGO_OK;
}

indigo_result indigo_filter_update_property(indigo_client *client, indigo_device *device, indigo_property *property, const char *message) {
	if (device == FILTER_CLIENT_CONTEXT->device)
		return INDIGO_OK;
	device = FILTER_CLIENT_CONTEXT->device;
	indigo_property **device_cache = FILTER_CLIENT_CONTEXT->device_property_cache;
	indigo_property **agent_cache = FILTER_CLIENT_CONTEXT->agent_property_cache;
	for (int i = 0; i < INDIGO_FILTER_LIST_COUNT; i++) {
		if (!strcmp(property->name, CONNECTION_PROPERTY_NAME) && property->state != INDIGO_BUSY_STATE) {
			indigo_item *connected_device = indigo_get_item(property, CONNECTION_CONNECTED_ITEM_NAME);
			indigo_property *device_list = FILTER_CLIENT_CONTEXT->filter_device_list_properties[i];
			for (int j = 1; j < device_list->count; j++) {
				if (!strcmp(property->device, device_list->items[j].name) && device_list->items[j].sw.value) {
					if (device_list->state == INDIGO_BUSY_STATE) {
						if (property->state == INDIGO_ALERT_STATE) {
							indigo_set_switch(device_list, device_list->items, true);
							device_list->state = INDIGO_ALERT_STATE;
							strcpy(FILTER_CLIENT_CONTEXT->device_name[i], "");
						} else if (connected_device->sw.value && property->state == INDIGO_OK_STATE) {
							indigo_property *configuration_property = indigo_init_switch_property(NULL, property->device, CONFIG_PROPERTY_NAME, NULL, NULL, INDIGO_OK_STATE, INDIGO_RW_PERM, INDIGO_ANY_OF_MANY_RULE, 1);
							indigo_init_switch_item(configuration_property->items, CONFIG_LOAD_ITEM_NAME, NULL, true);
							configuration_property->access_token = indigo_get_device_or_master_token(configuration_property->device);
							indigo_change_property(client, configuration_property);
							indigo_release_property(configuration_property);
							strcpy(FILTER_CLIENT_CONTEXT->device_name[i], property->device);
							device_list->state = INDIGO_OK_STATE;
							indigo_property all_properties;
							memset(&all_properties, 0, sizeof(all_properties));
							strcpy(all_properties.device, property->device);
							indigo_enumerate_properties(client, &all_properties);
						}
						indigo_update_property(device, device_list, NULL);
						return INDIGO_OK;
					} else if (device_list->state == INDIGO_OK_STATE && !connected_device->sw.value) {
						indigo_set_switch(device_list, device_list->items, true);
						device_list->state = INDIGO_ALERT_STATE;
						strcpy(FILTER_CLIENT_CONTEXT->device_name[i], "");
						indigo_update_property(device, device_list, NULL);
						return INDIGO_OK;
					}
				}
			}
		} else {
			if (strcmp(property->device, FILTER_CLIENT_CONTEXT->device_name[i]))
				continue;
			for (int i = 0; i < INDIGO_FILTER_MAX_CACHED_PROPERTIES; i++) {
				if (device_cache[i] == property) {
					if (agent_cache[i]) {
						indigo_property *copy = agent_cache[i];
						if (copy->type == INDIGO_TEXT_VECTOR) {
							for (int k = 0; k < copy->count; k++) {
								indigo_set_text_item_value(copy->items + k, indigo_get_text_item_value(property->items + k));
							}
						} else {
							memcpy(agent_cache[i]->items, property->items, property->count * sizeof(indigo_item));
						}
						agent_cache[i]->state = device_cache[i]->state;
						indigo_update_property(device, agent_cache[i], message);
					}
					return INDIGO_OK;
				}
			}
		}
	}
	return INDIGO_OK;
}

static void remove_from_list(indigo_device *device, indigo_property *device_list, indigo_property *property, char *device_name) {
	for (int i = 1; i < device_list->count; i++) {
		if (!strcmp(property->device, device_list->items[i].name)) {
			if (device_list->items[i].sw.value && device_name) {
				device_list->items[0].sw.value = true;
				if (device_name)
					*device_name = 0;
				device_list->state = INDIGO_ALERT_STATE;
			}
			int size = (device_list->count - i - 1) * sizeof(indigo_item);
			if (size > 0) {
				char buffer[size];
				memcpy(buffer, device_list->items + i + 1, size);
				memcpy(device_list->items + i, buffer, size);
			}
			indigo_delete_property(device, device_list, NULL);
			device_list->count--;
			indigo_define_property(device, device_list, NULL);
			break;
		}
	}
}

indigo_result indigo_filter_delete_property(indigo_client *client, indigo_device *device, indigo_property *property, const char *message) {
	if (device == FILTER_CLIENT_CONTEXT->device)
		return INDIGO_OK;
	device = FILTER_CLIENT_CONTEXT->device;
	indigo_property **device_cache = FILTER_CLIENT_CONTEXT->device_property_cache;
	indigo_property **agent_cache = FILTER_CLIENT_CONTEXT->agent_property_cache;
	if (*property->name) {
		for (int i = 0; i < INDIGO_FILTER_MAX_CACHED_PROPERTIES; i++) {
			if (device_cache[i] == property) {
				// this is the list of "fragile" properties used by various filter agents
				// if any of them is removed, any background process should abort asap
				FILTER_CLIENT_CONTEXT->property_removed =
					!strcmp(property->name, CCD_EXPOSURE_PROPERTY_NAME) ||
					!strcmp(property->name, CCD_STREAMING_PROPERTY_NAME) ||
					!strcmp(property->name, CCD_IMAGE_FORMAT_PROPERTY_NAME) ||
					!strcmp(property->name, CCD_UPLOAD_MODE_PROPERTY_NAME) ||
					!strcmp(property->name, CCD_TEMPERATURE_PROPERTY_NAME) ||
					!strcmp(property->name, GUIDER_GUIDE_RA_PROPERTY_NAME) ||
					!strcmp(property->name, GUIDER_GUIDE_DEC_PROPERTY_NAME) ||
					!strcmp(property->name, FOCUSER_DIRECTION_PROPERTY_NAME) ||
					!strcmp(property->name, FOCUSER_STEPS_PROPERTY_NAME);
				device_cache[i] = NULL;
				if (agent_cache[i]) {
					indigo_delete_property(device, agent_cache[i], NULL);
					indigo_release_property(agent_cache[i]);
					agent_cache[i] = NULL;
				}
				break;
			}
		}
		if (!strcmp(property->name, CONNECTION_PROPERTY_NAME)) {
			for (int i = 0; i < INDIGO_FILTER_MAX_DEVICES; i++) {
				if (FILTER_CLIENT_CONTEXT->connection_property_cache[i] == property) {
					free(FILTER_CLIENT_CONTEXT->connection_property_device_cache[i]);
					FILTER_CLIENT_CONTEXT->connection_property_cache[i] = NULL;
					FILTER_CLIENT_CONTEXT->connection_property_device_cache[i] = NULL;
					break;
				}
			}
		}
	} else {
		for (int i = 0; i < INDIGO_FILTER_MAX_CACHED_PROPERTIES; i++) {
			if (device_cache[i] && !strcmp(device_cache[i]->device, property->device)) {
				FILTER_CLIENT_CONTEXT->property_removed = true;
				device_cache[i] = NULL;
				if (agent_cache[i]) {
					indigo_delete_property(device, agent_cache[i], message);
					indigo_release_property(agent_cache[i]);
					agent_cache[i] = NULL;
				}
			}
		}
		for (int i = 0; i < INDIGO_FILTER_MAX_DEVICES; i++) {
			if (FILTER_CLIENT_CONTEXT->connection_property_device_cache[i] && !strcmp(FILTER_CLIENT_CONTEXT->connection_property_device_cache[i], property->device)) {
				free(FILTER_CLIENT_CONTEXT->connection_property_device_cache[i]);
				FILTER_CLIENT_CONTEXT->connection_property_cache[i] = NULL;
				FILTER_CLIENT_CONTEXT->connection_property_device_cache[i] = NULL;
				break;
			}
		}
	}
	if (*property->name == 0 || !strcmp(property->name, INFO_PROPERTY_NAME)) {
		for (int i = 0; i < INDIGO_FILTER_LIST_COUNT; i++) {
			remove_from_list(device, FILTER_CLIENT_CONTEXT->filter_device_list_properties[i], property, FILTER_CLIENT_CONTEXT->device_name[i]);
			remove_from_list(device, FILTER_CLIENT_CONTEXT->filter_related_device_list_properties[i], property, NULL);
			remove_from_list(device, FILTER_CLIENT_CONTEXT->filter_related_agent_list_property, property, NULL);
		}
	}
	return INDIGO_OK;
}

indigo_result indigo_filter_client_detach(indigo_client *client) {
	for (int i = 0; i < INDIGO_FILTER_LIST_COUNT; i++) {
		indigo_property *list = FILTER_CLIENT_CONTEXT->filter_device_list_properties[i];
		for (int j = 1; j < list->count; j++) {
			indigo_item *item = list->items + j;
			if (item->sw.value) {
				indigo_change_switch_property_1(client, item->name, CONNECTION_PROPERTY_NAME, CONNECTION_DISCONNECTED_ITEM_NAME, true);
				break;
			}
		}
	}	
	indigo_property **agent_cache = FILTER_CLIENT_CONTEXT->agent_property_cache;
	for (int i = 0; i < INDIGO_FILTER_MAX_CACHED_PROPERTIES; i++) {
		if (agent_cache[i])
			indigo_release_property(agent_cache[i]);
	}
	for (int i = 0; i < INDIGO_FILTER_MAX_DEVICES; i++) {
		if (FILTER_CLIENT_CONTEXT->connection_property_device_cache[i]) {
			free(FILTER_CLIENT_CONTEXT->connection_property_device_cache[i]);
		}
	}
	return INDIGO_OK;
}

bool indigo_filter_cached_property(indigo_device *device, int index, char *name, indigo_property **device_property, indigo_property **agent_property) {
	indigo_property **cache = FILTER_DEVICE_CONTEXT->device_property_cache;
	char *device_name = FILTER_DEVICE_CONTEXT->device_name[index];
	indigo_property *property;
	for (int j = 0; j < INDIGO_FILTER_MAX_CACHED_PROPERTIES; j++) {
		if ((property = cache[j])) {
			if (!strcmp(property->device, device_name) && !strcmp(property->name, name)) {
				if (device_property)
					*device_property = cache[j];
				if (agent_property)
					*agent_property = FILTER_DEVICE_CONTEXT->agent_property_cache[j];
				return true;
			}
		}
	}
	return false;
}

indigo_result indigo_filter_forward_change_property(indigo_client *client, indigo_property *property, char *device_name) {
	int size = sizeof(indigo_property) + property->count * (sizeof(indigo_item));
	indigo_property *copy = indigo_safe_malloc_copy(size, property);
	strcpy(copy->device, device_name);
	if (copy->type == INDIGO_TEXT_VECTOR) {
		for (int k = 0; k < copy->count; k++) {
			indigo_item *item = copy->items + k;
			if (item->text.long_value) {
				item->text.long_value = NULL;
				indigo_set_text_item_value(item, property->items[k].text.long_value);
			}
		}
	}
	copy->access_token = indigo_get_device_or_master_token(copy->device);
	property->perm = INDIGO_RW_PERM;
	indigo_result result = indigo_change_property(client, copy);
	indigo_release_property(copy);
	return result;
}
