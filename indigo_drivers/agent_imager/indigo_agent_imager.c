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

/** INDIGO Imager agent
 \file indigo_agent_imager.c
 */

#define DRIVER_VERSION 0x0001
#define DRIVER_NAME	"indigo_agent_imager"

#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <math.h>
#include <assert.h>
#include <pthread.h>

#include "indigo_driver_xml.h"
#include "indigo_agent_imager.h"

#define MAX_DEVICES							32
#define SEQUENCE_SIZE						16

#define DEVICE_PRIVATE_DATA			((agent_private_data *)device->private_data)
#define CLIENT_PRIVATE_DATA			((agent_private_data *)client->client_context)

#define AGENT_EXPOSURE_PROPERTY					(DEVICE_PRIVATE_DATA->agent_exposure_property)
#define AGENT_EXECUTE_BATCH_0_ITEM      (AGENT_EXPOSURE_PROPERTY->items+0)
#define AGENT_EXECUTE_SEQUENCE_ITEM     (AGENT_EXPOSURE_PROPERTY->items+1)

#define AGENT_ABORT_EXPOSURE_PROPERTY		(DEVICE_PRIVATE_DATA->agent_abort_exposure_property)
#define AGENT_ABORT_ITEM         				(AGENT_ABORT_EXPOSURE_PROPERTY->items+0)

#define AGENT_CCD_LIST_PROPERTY					(DEVICE_PRIVATE_DATA->agent_ccd_list_property)
#define AGENT_WHEEL_LIST_PROPERTY				(DEVICE_PRIVATE_DATA->agent_wheel_list_property)
#define AGENT_FOCUSER_LIST_PROPERTY			(DEVICE_PRIVATE_DATA->agent_focuser_list_property)

#define AGENT_BATCH_ENABLED_PROPERTY		(DEVICE_PRIVATE_DATA->agent_batch_enabled_property)
#define AGENT_BATCH_COUNT_PROPERTY			(DEVICE_PRIVATE_DATA->agent_batch_count_property)
#define AGENT_BATCH_DURATION_PROPERTY		(DEVICE_PRIVATE_DATA->agent_batch_duration_property)
#define AGENT_BATCH_TIMELAPSE_PROPERTY	(DEVICE_PRIVATE_DATA->agent_batch_timelapse_property)
#define AGENT_BATCH_OFFSET_PROPERTY			(DEVICE_PRIVATE_DATA->agent_batch_offset_property)
#define AGENT_BATCH_GAIN_PROPERTY				(DEVICE_PRIVATE_DATA->agent_batch_gain_property)
#define AGENT_BATCH_GAMMA_PROPERTY			(DEVICE_PRIVATE_DATA->agent_batch_gamma_property)
#define AGENT_BATCH_NAME_PROPERTY				(DEVICE_PRIVATE_DATA->agent_batch_name_property)
#define AGENT_BATCH_MODE_PROPERTY				(DEVICE_PRIVATE_DATA->agent_batch_mode_property)
#define AGENT_BATCH_FILTER_PROPERTY			(DEVICE_PRIVATE_DATA->agent_batch_filter_property)
#define AGENT_BATCH_STATE_PROPERTY			(DEVICE_PRIVATE_DATA->agent_batch_state_property)

typedef struct {
	indigo_device *device;
	indigo_client *client;
	indigo_property *agent_exposure_property;
	indigo_property *agent_abort_exposure_property;
	indigo_property *agent_ccd_list_property;
	indigo_property *agent_wheel_list_property;
	indigo_property *agent_focuser_list_property;
	indigo_property *agent_batch_enabled_property;
	indigo_property *agent_batch_count_property;
	indigo_property *agent_batch_duration_property;
	indigo_property *agent_batch_timelapse_property;
	indigo_property *agent_batch_offset_property;
	indigo_property *agent_batch_gain_property;
	indigo_property *agent_batch_gamma_property;
	indigo_property *agent_batch_name_property;
	indigo_property *agent_batch_mode_property;
	indigo_property *agent_batch_filter_property;
	indigo_property *agent_batch_state_property;
} agent_private_data;

// -------------------------------------------------------------------------------- INDIGO agent device implementation

static indigo_result agent_enumerate_properties(indigo_device *device, indigo_client *client, indigo_property *property);

static indigo_result agent_device_attach(indigo_device *device) {
	assert(device != NULL);
	assert(DEVICE_PRIVATE_DATA != NULL);
	if (indigo_agent_attach(device, DRIVER_VERSION) == INDIGO_OK) {
		// -------------------------------------------------------------------------------- Exposure properties
		AGENT_EXPOSURE_PROPERTY = indigo_init_switch_property(NULL, device->name, AGENT_EXPOSURE_PROPERTY_NAME, "Main", "Start exposure", INDIGO_OK_STATE, INDIGO_RW_PERM, INDIGO_ANY_OF_MANY_RULE, 2);
		if (AGENT_EXPOSURE_PROPERTY == NULL)
			return INDIGO_FAILED;
		indigo_init_switch_item(AGENT_EXECUTE_BATCH_0_ITEM, AGENT_EXECUTE_BATCH_0_ITEM_NAME, "Execute batch #0", false);
		indigo_init_switch_item(AGENT_EXECUTE_SEQUENCE_ITEM, AGENT_EXECUTE_SEQUENCE_ITEM_NAME, "Execute sequence #1...", false);
		AGENT_ABORT_EXPOSURE_PROPERTY = indigo_init_switch_property(NULL, device->name, AGENT_ABORT_EXPOSURE_PROPERTY_NAME, "Main", "Abort exposure", INDIGO_OK_STATE, INDIGO_RW_PERM, INDIGO_ANY_OF_MANY_RULE, 1);
		if (AGENT_EXPOSURE_PROPERTY == NULL)
			return INDIGO_FAILED;
		indigo_init_switch_item(AGENT_ABORT_ITEM, AGENT_ABORT_ITEM_NAME, "Abort exposure", false);
		// -------------------------------------------------------------------------------- Device properties
		AGENT_CCD_LIST_PROPERTY = indigo_init_switch_property(NULL, device->name, AGENT_CCD_LIST_PROPERTY_NAME, "Devices", "Camera list", INDIGO_OK_STATE, INDIGO_RW_PERM, INDIGO_ONE_OF_MANY_RULE, MAX_DEVICES);
		if (AGENT_CCD_LIST_PROPERTY == NULL)
			return INDIGO_FAILED;
		AGENT_CCD_LIST_PROPERTY->count = 1;
		indigo_init_switch_item(AGENT_CCD_LIST_PROPERTY->items, AGENT_DEVICE_LIST_NONE_ITEM_NAME, "None", true);
		AGENT_WHEEL_LIST_PROPERTY = indigo_init_switch_property(NULL, device->name, AGENT_WHEEL_LIST_PROPERTY_NAME, "Devices", "Filter wheel list", INDIGO_OK_STATE, INDIGO_RW_PERM, INDIGO_ONE_OF_MANY_RULE, MAX_DEVICES);
		if (AGENT_WHEEL_LIST_PROPERTY == NULL)
			return INDIGO_FAILED;
		AGENT_WHEEL_LIST_PROPERTY->count = 1;
		indigo_init_switch_item(AGENT_WHEEL_LIST_PROPERTY->items, AGENT_DEVICE_LIST_NONE_ITEM_NAME, "None", true);
		AGENT_FOCUSER_LIST_PROPERTY = indigo_init_switch_property(NULL, device->name, AGENT_FOCUSER_LIST_PROPERTY_NAME, "Devices", "Focuser list", INDIGO_OK_STATE, INDIGO_RW_PERM, INDIGO_ONE_OF_MANY_RULE, MAX_DEVICES);
		if (AGENT_FOCUSER_LIST_PROPERTY == NULL)
			return INDIGO_FAILED;
		AGENT_FOCUSER_LIST_PROPERTY->count = 1;
		indigo_init_switch_item(AGENT_FOCUSER_LIST_PROPERTY->items, AGENT_DEVICE_LIST_NONE_ITEM_NAME, "None", true);
		// -------------------------------------------------------------------------------- Sequencer properties
		AGENT_BATCH_ENABLED_PROPERTY = indigo_init_switch_property(NULL, device->name, AGENT_BATCH_ENABLED_PROPERTY_NAME, "Sequencer", "Enabled", INDIGO_OK_STATE, INDIGO_RW_PERM, INDIGO_ANY_OF_MANY_RULE, SEQUENCE_SIZE);
		if (AGENT_BATCH_ENABLED_PROPERTY == NULL)
			return INDIGO_FAILED;
		AGENT_BATCH_COUNT_PROPERTY = indigo_init_number_property(NULL, device->name, AGENT_BATCH_COUNT_PROPERTY_NAME, "Sequencer", "Count", INDIGO_OK_STATE, INDIGO_RW_PERM, SEQUENCE_SIZE);
		if (AGENT_BATCH_COUNT_PROPERTY == NULL)
			return INDIGO_FAILED;
		AGENT_BATCH_DURATION_PROPERTY = indigo_init_number_property(NULL, device->name, AGENT_BATCH_DURATION_PROPERTY_NAME, "Sequencer", "Duration", INDIGO_OK_STATE, INDIGO_RW_PERM, SEQUENCE_SIZE);
		if (AGENT_BATCH_DURATION_PROPERTY == NULL)
			return INDIGO_FAILED;
		AGENT_BATCH_TIMELAPSE_PROPERTY = indigo_init_number_property(NULL, device->name, AGENT_BATCH_TIMELAPSE_PROPERTY_NAME, "Sequencer", "Timelapse", INDIGO_OK_STATE, INDIGO_RW_PERM, SEQUENCE_SIZE);
		if (AGENT_BATCH_TIMELAPSE_PROPERTY == NULL)
			return INDIGO_FAILED;
		AGENT_BATCH_OFFSET_PROPERTY = indigo_init_number_property(NULL, device->name, AGENT_BATCH_OFFSET_PROPERTY_NAME, "Sequencer", "Offset", INDIGO_OK_STATE, INDIGO_RW_PERM, SEQUENCE_SIZE);
		if (AGENT_BATCH_OFFSET_PROPERTY == NULL)
			return INDIGO_FAILED;
		AGENT_BATCH_GAIN_PROPERTY = indigo_init_number_property(NULL, device->name, AGENT_BATCH_GAIN_PROPERTY_NAME, "Sequencer", "Gain", INDIGO_OK_STATE, INDIGO_RW_PERM, SEQUENCE_SIZE);
		if (AGENT_BATCH_GAIN_PROPERTY == NULL)
			return INDIGO_FAILED;
		AGENT_BATCH_GAMMA_PROPERTY = indigo_init_number_property(NULL, device->name, AGENT_BATCH_GAMMA_PROPERTY_NAME, "Sequencer", "Gamma", INDIGO_OK_STATE, INDIGO_RW_PERM, SEQUENCE_SIZE);
		if (AGENT_BATCH_GAMMA_PROPERTY == NULL)
			return INDIGO_FAILED;
		AGENT_BATCH_NAME_PROPERTY = indigo_init_text_property(NULL, device->name, AGENT_BATCH_NAME_PROPERTY_NAME, "Sequencer", "File name pattern", INDIGO_OK_STATE, INDIGO_RW_PERM, SEQUENCE_SIZE);
		if (AGENT_BATCH_NAME_PROPERTY == NULL)
			return INDIGO_FAILED;
		AGENT_BATCH_MODE_PROPERTY = indigo_init_text_property(NULL, device->name, AGENT_BATCH_MODE_PROPERTY_NAME, "Sequencer", "Camera mode", INDIGO_OK_STATE, INDIGO_RW_PERM, SEQUENCE_SIZE);
		if (AGENT_BATCH_MODE_PROPERTY == NULL)
			return INDIGO_FAILED;
		AGENT_BATCH_FILTER_PROPERTY = indigo_init_text_property(NULL, device->name, AGENT_BATCH_FILTER_PROPERTY_NAME, "Sequencer", "Filter name", INDIGO_OK_STATE, INDIGO_RW_PERM, SEQUENCE_SIZE);
		if (AGENT_BATCH_FILTER_PROPERTY == NULL)
			return INDIGO_FAILED;
		AGENT_BATCH_STATE_PROPERTY = indigo_init_light_property(NULL, device->name, AGENT_BATCH_STATE_PROPERTY_NAME, "Sequencer", "Status", INDIGO_OK_STATE, SEQUENCE_SIZE);
		if (AGENT_BATCH_STATE_PROPERTY == NULL)
			return INDIGO_FAILED;
		char name[INDIGO_NAME_SIZE];
		for (int i = 0; i < SEQUENCE_SIZE; i++) {
			sprintf(name, AGENT_BATCH_ITEM_FORMAT, i);
			indigo_init_switch_item(AGENT_BATCH_ENABLED_PROPERTY->items + i, name, name, false);
			indigo_init_number_item(AGENT_BATCH_COUNT_PROPERTY->items + i, name, name, 0, 10000, 1, 0);
			indigo_init_number_item(AGENT_BATCH_DURATION_PROPERTY->items + i, name, name, 0, 10000, 1, 0);
			indigo_init_number_item(AGENT_BATCH_TIMELAPSE_PROPERTY->items + i, name, name, 0, 10000, 1, 0);
			indigo_init_number_item(AGENT_BATCH_OFFSET_PROPERTY->items + i, name, name, 0, 10000, 1, 0);
			indigo_init_number_item(AGENT_BATCH_GAIN_PROPERTY->items + i, name, name, 0, 10000, 1, 0);
			indigo_init_number_item(AGENT_BATCH_GAMMA_PROPERTY->items + i, name, name, 0, 10000, 1, 0);
			indigo_init_text_item(AGENT_BATCH_NAME_PROPERTY->items + i, name, name, "");
			indigo_init_text_item(AGENT_BATCH_MODE_PROPERTY->items + i, name, name, "");
			indigo_init_text_item(AGENT_BATCH_FILTER_PROPERTY->items + i, name, name, "");
			indigo_init_light_item(AGENT_BATCH_STATE_PROPERTY->items + i, name, name, INDIGO_IDLE_STATE);
		}
		// --------------------------------------------------------------------------------
		INDIGO_DEVICE_ATTACH_LOG(DRIVER_NAME, device->name);
		return agent_enumerate_properties(device, NULL, NULL);
	}
	return INDIGO_FAILED;
}

static indigo_result agent_enumerate_properties(indigo_device *device, indigo_client *client, indigo_property *property) {
	if (client!= NULL && client == DEVICE_PRIVATE_DATA->client)
		return INDIGO_OK;
	if (indigo_property_match(AGENT_EXPOSURE_PROPERTY, property))
		indigo_define_property(device, AGENT_EXPOSURE_PROPERTY, NULL);
	if (indigo_property_match(AGENT_ABORT_EXPOSURE_PROPERTY, property))
		indigo_define_property(device, AGENT_ABORT_EXPOSURE_PROPERTY, NULL);
	if (indigo_property_match(AGENT_CCD_LIST_PROPERTY, property))
		indigo_define_property(device, AGENT_CCD_LIST_PROPERTY, NULL);
	if (indigo_property_match(AGENT_WHEEL_LIST_PROPERTY, property))
		indigo_define_property(device, AGENT_WHEEL_LIST_PROPERTY, NULL);
	if (indigo_property_match(AGENT_FOCUSER_LIST_PROPERTY, property))
		indigo_define_property(device, AGENT_FOCUSER_LIST_PROPERTY, NULL);
	if (indigo_property_match(AGENT_BATCH_ENABLED_PROPERTY, property))
		indigo_define_property(device, AGENT_BATCH_ENABLED_PROPERTY, NULL);
	if (indigo_property_match(AGENT_BATCH_COUNT_PROPERTY, property))
		indigo_define_property(device, AGENT_BATCH_COUNT_PROPERTY, NULL);
	if (indigo_property_match(AGENT_BATCH_DURATION_PROPERTY, property))
		indigo_define_property(device, AGENT_BATCH_DURATION_PROPERTY, NULL);
	if (indigo_property_match(AGENT_BATCH_TIMELAPSE_PROPERTY, property))
		indigo_define_property(device, AGENT_BATCH_TIMELAPSE_PROPERTY, NULL);
	if (indigo_property_match(AGENT_BATCH_OFFSET_PROPERTY, property))
		indigo_define_property(device, AGENT_BATCH_OFFSET_PROPERTY, NULL);
	if (indigo_property_match(AGENT_BATCH_GAIN_PROPERTY, property))
		indigo_define_property(device, AGENT_BATCH_GAIN_PROPERTY, NULL);
	if (indigo_property_match(AGENT_BATCH_GAMMA_PROPERTY, property))
		indigo_define_property(device, AGENT_BATCH_GAMMA_PROPERTY, NULL);
	if (indigo_property_match(AGENT_BATCH_NAME_PROPERTY, property))
		indigo_define_property(device, AGENT_BATCH_NAME_PROPERTY, NULL);
	if (indigo_property_match(AGENT_BATCH_MODE_PROPERTY, property))
		indigo_define_property(device, AGENT_BATCH_MODE_PROPERTY, NULL);
	if (indigo_property_match(AGENT_BATCH_FILTER_PROPERTY, property))
		indigo_define_property(device, AGENT_BATCH_FILTER_PROPERTY, NULL);
	if (indigo_property_match(AGENT_BATCH_STATE_PROPERTY, property))
		indigo_define_property(device, AGENT_BATCH_STATE_PROPERTY, NULL);	return indigo_device_enumerate_properties(device, client, property);
}

static indigo_result select_device(indigo_device *device, indigo_client *client, indigo_property *property, indigo_property *list) {
	indigo_property *ccd_connection = indigo_init_switch_property(NULL, "*", CONNECTION_PROPERTY_NAME, NULL, NULL, INDIGO_OK_STATE, INDIGO_RW_PERM, INDIGO_ONE_OF_MANY_RULE, 2);
	indigo_init_switch_item(ccd_connection->items + 0, CONNECTION_CONNECTED_ITEM_NAME, NULL, false);
	indigo_init_switch_item(ccd_connection->items + 1, CONNECTION_DISCONNECTED_ITEM_NAME, NULL, true);
	for (int i = 1; i < list->count; i++) {
		if (list->items[i].sw.value) {
			list->items[i].sw.value = false;
			strncpy(ccd_connection->device, list->items[i].name, INDIGO_NAME_SIZE);
			indigo_change_property(client, ccd_connection);
			break;
		}
	}
	indigo_property_copy_values(list, property, false);
	for (int i = 1; i < list->count; i++) {
		if (list->items[i].sw.value) {
			indigo_set_switch(ccd_connection, ccd_connection->items, true);
			list->state = INDIGO_BUSY_STATE;
			indigo_update_property(device, list, NULL);
			strncpy(ccd_connection->device, list->items[i].name, INDIGO_NAME_SIZE);
			indigo_change_property(client, ccd_connection);
			return INDIGO_OK;
		}
	}
	list->state = INDIGO_OK_STATE;
	indigo_update_property(device, list, NULL);
	return INDIGO_OK;
}

static void execute_batch_0(indigo_device *device) {
	// TBD
	char message[INDIGO_VALUE_SIZE];
	for (int i = 0; i < AGENT_BATCH_COUNT_PROPERTY->items[0].number.target; i++) {
		AGENT_BATCH_STATE_PROPERTY->items[0].light.value = INDIGO_BUSY_STATE;
		sprintf(message, "Exposure %d/%d in progress", i + 1, (int)AGENT_BATCH_COUNT_PROPERTY->items[0].number.target);
		indigo_update_property(device, AGENT_BATCH_STATE_PROPERTY, message);
		usleep(AGENT_BATCH_DURATION_PROPERTY->items[0].number.target * 1000000);
		sprintf(message, "Timelapse %d/%d in progress", i + 1, (int)AGENT_BATCH_COUNT_PROPERTY->items[0].number.target);
		indigo_update_property(device, AGENT_BATCH_STATE_PROPERTY, message);
		usleep(AGENT_BATCH_TIMELAPSE_PROPERTY->items[0].number.target * 1000000);
		AGENT_BATCH_STATE_PROPERTY->items[0].light.value = INDIGO_OK_STATE;
		sprintf(message, "Exposure %d/%d done", i + 1, (int)AGENT_BATCH_COUNT_PROPERTY->items[0].number.target);
		indigo_update_property(device, AGENT_BATCH_STATE_PROPERTY, message);
	}
	// TBD

	AGENT_EXPOSURE_PROPERTY->state = INDIGO_OK_STATE;
	indigo_update_property(device, AGENT_EXPOSURE_PROPERTY, NULL);
}

static void execute_sequence(indigo_device *device) {
	// TBD
	char message[INDIGO_VALUE_SIZE];
	for (int j = 1; j < SEQUENCE_SIZE; j++) {
		if (AGENT_BATCH_ENABLED_PROPERTY->items[j].sw.value) {
			for (int i = 0; i < AGENT_BATCH_COUNT_PROPERTY->items[j].number.target; i++) {
				AGENT_BATCH_STATE_PROPERTY->items[j].light.value = INDIGO_BUSY_STATE;
				sprintf(message, "Exposure %d/%d batch %d in progress", i + 1, (int)AGENT_BATCH_COUNT_PROPERTY->items[j].number.target, j);
				indigo_update_property(device, AGENT_BATCH_STATE_PROPERTY, message);
				usleep(AGENT_BATCH_DURATION_PROPERTY->items[j].number.target * 1000000);
				sprintf(message, "Timelapse %d/%d batch %d in progress", i + 1, (int)AGENT_BATCH_COUNT_PROPERTY->items[j].number.target, j);
				indigo_update_property(device, AGENT_BATCH_STATE_PROPERTY, message);
				usleep(AGENT_BATCH_TIMELAPSE_PROPERTY->items[j].number.target * 1000000);
				AGENT_BATCH_STATE_PROPERTY->items[j].light.value = INDIGO_OK_STATE;
				sprintf(message, "Exposure %d/%d batch %d done", i + 1, (int)AGENT_BATCH_COUNT_PROPERTY->items[j].number.target, j);
				indigo_update_property(device, AGENT_BATCH_STATE_PROPERTY, message);
			}
		}
	}
	// TBD
	AGENT_EXPOSURE_PROPERTY->state = INDIGO_OK_STATE;
	indigo_update_property(device, AGENT_EXPOSURE_PROPERTY, NULL);
}


static indigo_result agent_change_property(indigo_device *device, indigo_client *client, indigo_property *property) {
	assert(device != NULL);
	assert(DEVICE_CONTEXT != NULL);
	assert(property != NULL);
	if (client == DEVICE_PRIVATE_DATA->client)
		return INDIGO_OK;
	if (indigo_property_match(AGENT_EXPOSURE_PROPERTY, property)) {
		indigo_property_copy_values(AGENT_EXPOSURE_PROPERTY, property, false);
		if (AGENT_EXPOSURE_PROPERTY->state != INDIGO_BUSY_STATE && AGENT_ABORT_EXPOSURE_PROPERTY->state != INDIGO_BUSY_STATE) {
			if (AGENT_EXECUTE_BATCH_0_ITEM->sw.value) {
				AGENT_EXPOSURE_PROPERTY->state = INDIGO_BUSY_STATE;
				indigo_set_timer(device, 0, execute_batch_0);
			} else if (AGENT_EXECUTE_SEQUENCE_ITEM->sw.value) {
				AGENT_EXPOSURE_PROPERTY->state = INDIGO_BUSY_STATE;
				indigo_set_timer(device, 0, execute_sequence);
			}
		}
		AGENT_EXECUTE_BATCH_0_ITEM->sw.value = AGENT_EXECUTE_SEQUENCE_ITEM->sw.value = false;
		indigo_update_property(device, AGENT_EXPOSURE_PROPERTY, NULL);
	} else if (indigo_property_match(AGENT_ABORT_EXPOSURE_PROPERTY, property)) {
		indigo_property_copy_values(AGENT_ABORT_EXPOSURE_PROPERTY, property, false);
		if (AGENT_ABORT_ITEM->sw.value && AGENT_EXPOSURE_PROPERTY->state == INDIGO_BUSY_STATE) {
			AGENT_EXPOSURE_PROPERTY->state = INDIGO_ALERT_STATE;
			indigo_update_property(device, AGENT_EXPOSURE_PROPERTY, NULL);
			AGENT_ABORT_EXPOSURE_PROPERTY->state = INDIGO_BUSY_STATE;
		}
		AGENT_ABORT_ITEM->sw.value = false;
		indigo_update_property(device, AGENT_ABORT_EXPOSURE_PROPERTY, NULL);
	} else if (indigo_property_match(AGENT_CCD_LIST_PROPERTY, property)) {
		return select_device(device, client, property, AGENT_CCD_LIST_PROPERTY);
	} else if (indigo_property_match(AGENT_WHEEL_LIST_PROPERTY, property)) {
		return select_device(device, client, property, AGENT_WHEEL_LIST_PROPERTY);
	} else if (indigo_property_match(AGENT_FOCUSER_LIST_PROPERTY, property)) {
		return select_device(device, client, property, AGENT_FOCUSER_LIST_PROPERTY);
	} else if (indigo_property_match(AGENT_BATCH_ENABLED_PROPERTY, property)) {
		indigo_property_copy_values(AGENT_BATCH_ENABLED_PROPERTY, property, false);
		AGENT_BATCH_ENABLED_PROPERTY->state = INDIGO_OK_STATE;
		indigo_update_property(device, AGENT_BATCH_ENABLED_PROPERTY, NULL);
	} else if (indigo_property_match(AGENT_BATCH_COUNT_PROPERTY, property)) {
		indigo_property_copy_values(AGENT_BATCH_COUNT_PROPERTY, property, false);
		AGENT_BATCH_COUNT_PROPERTY->state = INDIGO_OK_STATE;
		indigo_update_property(device, AGENT_BATCH_COUNT_PROPERTY, NULL);
	} else if (indigo_property_match(AGENT_BATCH_DURATION_PROPERTY, property)) {
		indigo_property_copy_values(AGENT_BATCH_DURATION_PROPERTY, property, false);
		AGENT_BATCH_DURATION_PROPERTY->state = INDIGO_OK_STATE;
		indigo_update_property(device, AGENT_BATCH_DURATION_PROPERTY, NULL);
	} else if (indigo_property_match(AGENT_BATCH_TIMELAPSE_PROPERTY, property)) {
		indigo_property_copy_values(AGENT_BATCH_TIMELAPSE_PROPERTY, property, false);
		AGENT_BATCH_TIMELAPSE_PROPERTY->state = INDIGO_OK_STATE;
		indigo_update_property(device, AGENT_BATCH_TIMELAPSE_PROPERTY, NULL);
	} else if (indigo_property_match(AGENT_BATCH_OFFSET_PROPERTY, property)) {
		indigo_property_copy_values(AGENT_BATCH_OFFSET_PROPERTY, property, false);
		AGENT_BATCH_OFFSET_PROPERTY->state = INDIGO_OK_STATE;
		indigo_update_property(device, AGENT_BATCH_OFFSET_PROPERTY, NULL);
	} else if (indigo_property_match(AGENT_BATCH_GAIN_PROPERTY, property)) {
		indigo_property_copy_values(AGENT_BATCH_GAIN_PROPERTY, property, false);
		AGENT_BATCH_GAIN_PROPERTY->state = INDIGO_OK_STATE;
		indigo_update_property(device, AGENT_BATCH_GAIN_PROPERTY, NULL);
	} else if (indigo_property_match(AGENT_BATCH_GAMMA_PROPERTY, property)) {
		indigo_property_copy_values(AGENT_BATCH_GAMMA_PROPERTY, property, false);
		AGENT_BATCH_GAMMA_PROPERTY->state = INDIGO_OK_STATE;
		indigo_update_property(device, AGENT_BATCH_GAMMA_PROPERTY, NULL);
	} else if (indigo_property_match(AGENT_BATCH_NAME_PROPERTY, property)) {
		indigo_property_copy_values(AGENT_BATCH_NAME_PROPERTY, property, false);
		AGENT_BATCH_NAME_PROPERTY->state = INDIGO_OK_STATE;
		indigo_update_property(device, AGENT_BATCH_NAME_PROPERTY, NULL);
	} else if (indigo_property_match(AGENT_BATCH_MODE_PROPERTY, property)) {
		indigo_property_copy_values(AGENT_BATCH_MODE_PROPERTY, property, false);
		AGENT_BATCH_MODE_PROPERTY->state = INDIGO_OK_STATE;
		indigo_update_property(device, AGENT_BATCH_MODE_PROPERTY, NULL);
	} else if (indigo_property_match(AGENT_BATCH_FILTER_PROPERTY, property)) {
		indigo_property_copy_values(AGENT_BATCH_FILTER_PROPERTY, property, false);
		AGENT_BATCH_FILTER_PROPERTY->state = INDIGO_OK_STATE;
		indigo_update_property(device, AGENT_BATCH_FILTER_PROPERTY, NULL);
	} else if (indigo_property_match(AGENT_BATCH_STATE_PROPERTY, property)) {
		indigo_property_copy_values(AGENT_BATCH_STATE_PROPERTY, property, false);
		AGENT_BATCH_STATE_PROPERTY->state = INDIGO_OK_STATE;
		indigo_update_property(device, AGENT_BATCH_STATE_PROPERTY, NULL);	}
	return indigo_agent_change_property(device, client, property);
}

static indigo_result agent_device_detach(indigo_device *device) {
	assert(device != NULL);
	indigo_release_property(AGENT_EXPOSURE_PROPERTY);
	indigo_release_property(AGENT_ABORT_EXPOSURE_PROPERTY);
	indigo_release_property(AGENT_CCD_LIST_PROPERTY);
	indigo_release_property(AGENT_WHEEL_LIST_PROPERTY);
	indigo_release_property(AGENT_FOCUSER_LIST_PROPERTY);
	indigo_release_property(AGENT_BATCH_ENABLED_PROPERTY);
	indigo_release_property(AGENT_BATCH_COUNT_PROPERTY);
	indigo_release_property(AGENT_BATCH_DURATION_PROPERTY);
	indigo_release_property(AGENT_BATCH_TIMELAPSE_PROPERTY);
	indigo_release_property(AGENT_BATCH_OFFSET_PROPERTY);
	indigo_release_property(AGENT_BATCH_GAIN_PROPERTY);
	indigo_release_property(AGENT_BATCH_GAMMA_PROPERTY);
	indigo_release_property(AGENT_BATCH_NAME_PROPERTY);
	indigo_release_property(AGENT_BATCH_MODE_PROPERTY);
	indigo_release_property(AGENT_BATCH_FILTER_PROPERTY);
	indigo_release_property(AGENT_BATCH_STATE_PROPERTY);	return indigo_agent_detach(device);
}

static indigo_result agent_client_attach(indigo_client *client) {
	indigo_enumerate_properties(client, &INDIGO_ALL_PROPERTIES);
	return INDIGO_OK;
}

static indigo_result add_device(struct indigo_client *client, struct indigo_device *device, indigo_property *property, indigo_property *device_list) {
	int count = device_list->count;
	for (int i = 1; i < count; i++) {
		if (!strcmp(property->device, device_list->items[i].name)) {
			return INDIGO_OK;
		}
	}
	if (count < MAX_DEVICES) {
		indigo_delete_property(CLIENT_PRIVATE_DATA->device, device_list, NULL);
		indigo_init_switch_item(device_list->items + count, property->device, property->device, false);
		device_list->count++;
		indigo_define_property(CLIENT_PRIVATE_DATA->device, device_list, NULL);
	}
	return INDIGO_OK;
}

static indigo_result agent_define_property(struct indigo_client *client, struct indigo_device *device, indigo_property *property, const char *message) {
	if (device == CLIENT_PRIVATE_DATA->device)
		return INDIGO_OK;
	if (!strcmp(property->name, INFO_PROPERTY_NAME)) {
		indigo_item *interface = indigo_get_item(property, INFO_DEVICE_INTERFACE_ITEM_NAME);
		if (interface) {
			int mask = atoi(interface->text.value);
			if (mask & INDIGO_INTERFACE_CCD) {
				add_device(client, device, property, CLIENT_PRIVATE_DATA->agent_ccd_list_property);
			}
			if (mask & INDIGO_INTERFACE_WHEEL) {
				add_device(client, device, property, CLIENT_PRIVATE_DATA->agent_wheel_list_property);
			}
			if (mask & INDIGO_INTERFACE_FOCUSER) {
				add_device(client, device, property, CLIENT_PRIVATE_DATA->agent_focuser_list_property);
			}
		}
	}
	return INDIGO_OK;
}

static indigo_result connect_device(struct indigo_client *client, struct indigo_device *device, indigo_property *property, indigo_property *device_list) {
	indigo_item *connected_device = indigo_get_item(property, CONNECTION_CONNECTED_ITEM_NAME);
	for (int i = 1; i < device_list->count; i++) {
		if (!strcmp(property->device, device_list->items[i].name) && device_list->items[i].sw.value) {
			if (device_list->state == INDIGO_BUSY_STATE) {
				if (property->state == INDIGO_ALERT_STATE) {
					device_list->state = INDIGO_ALERT_STATE;
				} else if (connected_device->sw.value && property->state == INDIGO_OK_STATE) {
					device_list->state = INDIGO_OK_STATE;
				}
				indigo_update_property(CLIENT_PRIVATE_DATA->device, device_list, NULL);
				return INDIGO_OK;
			} else if (device_list->state == INDIGO_OK_STATE && !connected_device->sw.value) {
				device_list->state = INDIGO_ALERT_STATE;
				indigo_update_property(CLIENT_PRIVATE_DATA->device, device_list, NULL);
				return INDIGO_OK;
			}
		}
	}
	return INDIGO_OK;
}

static indigo_result agent_update_property(struct indigo_client *client, struct indigo_device *device, indigo_property *property, const char *message) {
	if (device == CLIENT_PRIVATE_DATA->device)
		return INDIGO_OK;
	if (!strcmp(property->name, CONNECTION_PROPERTY_NAME)) {
		if (property->state != INDIGO_BUSY_STATE) {
			connect_device(client, device, property, CLIENT_PRIVATE_DATA->agent_ccd_list_property);
			connect_device(client, device, property, CLIENT_PRIVATE_DATA->agent_wheel_list_property);
			connect_device(client, device, property, CLIENT_PRIVATE_DATA->agent_focuser_list_property);
		}
	}
	return INDIGO_OK;
}

static indigo_result delete_device(struct indigo_client *client, struct indigo_device *device, indigo_property *property, indigo_property *device_list) {
	for (int i = 1; i < device_list->count; i++) {
		if (!strcmp(property->device, device_list->items[i].name)) {
			int size = (device_list->count - i - 1) * sizeof(indigo_item);
			if (size > 0) {
				memcpy(device_list->items + i, device_list->items + i + 1, size);
			}
			indigo_delete_property(CLIENT_PRIVATE_DATA->device, device_list, NULL);
			device_list->count--;
			indigo_define_property(CLIENT_PRIVATE_DATA->device, device_list, NULL);
			return INDIGO_OK;
		}
	}
	return INDIGO_OK;
}

static indigo_result agent_delete_property(indigo_client *client, struct indigo_device *device, indigo_property *property, const char *message) {
	if (device == CLIENT_PRIVATE_DATA->device)
		return INDIGO_OK;
	if (*property->name == 0 || !strcmp(property->name, INFO_PROPERTY_NAME)) {
		delete_device(client, device, property, CLIENT_PRIVATE_DATA->agent_ccd_list_property);
		delete_device(client, device, property, CLIENT_PRIVATE_DATA->agent_wheel_list_property);
		delete_device(client, device, property, CLIENT_PRIVATE_DATA->agent_focuser_list_property);
	}
	return INDIGO_OK;
}

static indigo_result agent_client_detach(indigo_client *client) {
	// TBD
	return INDIGO_OK;
}

// --------------------------------------------------------------------------------

static agent_private_data *private_data = NULL;

static indigo_device *agent_device = NULL;
static indigo_client *agent_client = NULL;

indigo_result indigo_agent_imager(indigo_driver_action action, indigo_driver_info *info) {
	static indigo_device agent_device_template = INDIGO_DEVICE_INITIALIZER(
		IMAGER_AGENT_NAME,
		agent_device_attach,
		agent_enumerate_properties,
		agent_change_property,
		NULL,
		agent_device_detach
	);

	static indigo_client agent_client_template = {
		IMAGER_AGENT_NAME, false, NULL, INDIGO_OK, INDIGO_VERSION_CURRENT, NULL,
		agent_client_attach,
		agent_define_property,
		agent_update_property,
		agent_delete_property,
		NULL,
		agent_client_detach
	};

	static indigo_driver_action last_action = INDIGO_DRIVER_SHUTDOWN;

	SET_DRIVER_INFO(info, "Imager agent", __FUNCTION__, DRIVER_VERSION, false, last_action);

	if (action == last_action)
		return INDIGO_OK;

	switch(action) {
		case INDIGO_DRIVER_INIT:
			last_action = action;
			private_data = malloc(sizeof(agent_private_data));
			assert(private_data != NULL);
			memset(private_data, 0, sizeof(agent_private_data));
			agent_device = malloc(sizeof(indigo_device));
			assert(agent_device != NULL);
			private_data->device = agent_device;
			memcpy(agent_device, &agent_device_template, sizeof(indigo_device));
			agent_device->private_data = private_data;
			indigo_attach_device(agent_device);

			agent_client = malloc(sizeof(indigo_client));
			assert(agent_client != NULL);
			private_data->client = agent_client;
			memcpy(agent_client, &agent_client_template, sizeof(indigo_client));
			agent_client->client_context = private_data;
			indigo_attach_client(agent_client);
			break;

		case INDIGO_DRIVER_SHUTDOWN:
			last_action = action;
			if (agent_device != NULL) {
				indigo_detach_device(agent_device);
				free(agent_device);
				agent_device = NULL;
			}
			if (agent_client != NULL) {
				indigo_detach_client(agent_client);
				free(agent_client);
				agent_client = NULL;
			}
			if (private_data != NULL) {
				free(private_data);
				private_data = NULL;
			}
			break;

		case INDIGO_DRIVER_INFO:
			break;
	}
	return INDIGO_OK;
}
