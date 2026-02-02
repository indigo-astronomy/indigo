// Copyright (c) 2016-2025 CloudMakers, s. r. o.
// All rights reserved.

// You may use this software under the terms of 'INDIGO Astronomy
// open-source license' (see LICENSE.md).

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

// This file generated from indigo_ao_sx.driver

#pragma mark - Includes

#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <assert.h>
#include <pthread.h>
#include <indigo/indigo_driver_xml.h>
#include <indigo/indigo_ao_driver.h>
#include <indigo/indigo_guider_driver.h>
#include <indigo/indigo_uni_io.h>

#include "indigo_ao_sx.h"

#pragma mark - Common definitions

#define DRIVER_VERSION       0x0300000B
#define DRIVER_NAME          "indigo_ao_sx"
#define DRIVER_LABEL         "StarlightXpress AO"
#define AO_DEVICE_NAME       "SX AO"
#define GUIDER_DEVICE_NAME   "SX AO (guider)"
#define PRIVATE_DATA         ((sx_private_data *)device->private_data)

#pragma mark - Private data definition

typedef struct {
	int count;
	indigo_uni_handle *handle;
	//+ data
	char response[6];
	//- data
} sx_private_data;

#pragma mark - Low level code

//+ code

static bool sx_command(indigo_device *device, char *command, int response, ...) {
	long result = indigo_uni_discard(PRIVATE_DATA->handle);
	if (result >= 0) {
		va_list args;
		va_start(args, response);
		result = indigo_uni_vprintf(PRIVATE_DATA->handle, command, args);
		va_end(args);
		if (result > 0 && response > 0) {
			result = indigo_uni_read_section(PRIVATE_DATA->handle, PRIVATE_DATA->response, response, "", "", INDIGO_DELAY((*command == 'K' || *command == 'R') ? 15 : 1));
		}
	}
	return result > 0;
}

static bool sx_open(indigo_device *device) {
	PRIVATE_DATA->handle = indigo_uni_open_serial(DEVICE_PORT_ITEM->text.value, INDIGO_LOG_DEBUG);
	if (PRIVATE_DATA->handle != NULL) {
		if (sx_command(device, "X", 1) && PRIVATE_DATA->response[0] == 'Y') {
			if (sx_command(device, "V", 4) && PRIVATE_DATA->response[0] == 'V') {
				INDIGO_COPY_VALUE(INFO_DEVICE_MODEL_ITEM->text.value, DRIVER_LABEL);
				INDIGO_COPY_VALUE(INFO_DEVICE_FW_REVISION_ITEM->text.value, PRIVATE_DATA->response + 1);
				indigo_update_property(device, INFO_PROPERTY, NULL);
				return true;
			}
		}
		indigo_uni_close(&PRIVATE_DATA->handle);
	}
	return false;
}

static void sx_close(indigo_device *device) {
	INDIGO_COPY_VALUE(INFO_DEVICE_MODEL_ITEM->text.value, "Unknown");
	INDIGO_COPY_VALUE(INFO_DEVICE_FW_REVISION_ITEM->text.value, "Unknown");
	indigo_update_property(device, INFO_PROPERTY, NULL);
	indigo_uni_close(&PRIVATE_DATA->handle);
}

//- code

#pragma mark - High level code (ao)

static void ao_connection_handler(indigo_device *device) {
	if (CONNECTION_CONNECTED_ITEM->sw.value) {
		bool connection_result = true;
		if (PRIVATE_DATA->count++ == 0) {
			connection_result = sx_open(device);
		}
		if (connection_result) {
			//+ ao.on_connect
			if (sx_command(device, "L", 1)) {
				AO_GUIDE_DEC_PROPERTY->state = AO_GUIDE_RA_PROPERTY->state = INDIGO_OK_STATE;
				if (PRIVATE_DATA->response[0] & 0x05)
					AO_GUIDE_DEC_PROPERTY->state = INDIGO_ALERT_STATE;
				if (PRIVATE_DATA->response[0] & 0x0A)
					AO_GUIDE_RA_PROPERTY->state = INDIGO_ALERT_STATE;
			}
			//- ao.on_connect
			CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
			indigo_send_message(device, "Connected to %s on %s", AO_DEVICE_NAME, DEVICE_PORT_ITEM->text.value);
		} else {
			indigo_send_message(device, "Failed to connect to %s on %s", AO_DEVICE_NAME, DEVICE_PORT_ITEM->text.value);
			PRIVATE_DATA->count--;
			CONNECTION_PROPERTY->state = INDIGO_ALERT_STATE;
			indigo_set_switch(CONNECTION_PROPERTY, CONNECTION_DISCONNECTED_ITEM, true);
		}
	} else {
		indigo_cancel_pending_handlers(device);
		if (--PRIVATE_DATA->count == 0) {
			sx_close(device);
		}
		indigo_send_message(device, "Disconnected from %s", device->name);
		CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
	}
	indigo_ao_change_property(device, NULL, CONNECTION_PROPERTY);
}

static void ao_guide_dec_handler(indigo_device *device) {
	AO_GUIDE_DEC_PROPERTY->state = INDIGO_OK_STATE;
	//+ ao.AO_GUIDE_DEC.on_change
	if (AO_GUIDE_NORTH_ITEM->number.value > 0) {
		sx_command(device, "GN%05d", 1, (int)AO_GUIDE_NORTH_ITEM->number.value);
	} else if (AO_GUIDE_SOUTH_ITEM->number.value > 0) {
		sx_command(device, "GS%05d", 1, (int)AO_GUIDE_SOUTH_ITEM->number.value);
	}
	AO_GUIDE_NORTH_ITEM->number.value = AO_GUIDE_SOUTH_ITEM->number.value = 0;
	if (PRIVATE_DATA->response[0] != 'G') {
		AO_GUIDE_DEC_PROPERTY->state = INDIGO_ALERT_STATE;
	}
	//- ao.AO_GUIDE_DEC.on_change
	indigo_update_property(device, AO_GUIDE_DEC_PROPERTY, NULL);
}

static void ao_guide_ra_handler(indigo_device *device) {
	AO_GUIDE_RA_PROPERTY->state = INDIGO_OK_STATE;
	//+ ao.AO_GUIDE_RA.on_change
	if (AO_GUIDE_WEST_ITEM->number.value > 0) {
		sx_command(device, "GW%05d", 1, (int)AO_GUIDE_WEST_ITEM->number.value);
	} else if (AO_GUIDE_EAST_ITEM->number.value > 0) {
		sx_command(device, "GT%05d", 1, (int)AO_GUIDE_EAST_ITEM->number.value);
	}
	AO_GUIDE_WEST_ITEM->number.value = AO_GUIDE_EAST_ITEM->number.value = 0;
	if (PRIVATE_DATA->response[0] != 'G') {
		AO_GUIDE_RA_PROPERTY->state = INDIGO_ALERT_STATE;
	}
	//- ao.AO_GUIDE_RA.on_change
	indigo_update_property(device, AO_GUIDE_RA_PROPERTY, NULL);
}

static void ao_reset_handler(indigo_device *device) {
	AO_RESET_PROPERTY->state = INDIGO_OK_STATE;
	//+ ao.AO_RESET.on_change
	if (AO_CENTER_ITEM->sw.value) {
		sx_command(device, "K", 1);
		AO_GUIDE_DEC_PROPERTY->state = INDIGO_OK_STATE;
		indigo_update_property(device, AO_GUIDE_DEC_PROPERTY, NULL);
		AO_GUIDE_RA_PROPERTY->state = INDIGO_OK_STATE;
		indigo_update_property(device, AO_GUIDE_RA_PROPERTY, NULL);
	} else if (AO_UNJAM_ITEM->sw.value) {
		sx_command(device, "R", 1);
		AO_GUIDE_DEC_PROPERTY->state = INDIGO_OK_STATE;
		indigo_update_property(device, AO_GUIDE_DEC_PROPERTY, NULL);
		AO_GUIDE_RA_PROPERTY->state = INDIGO_OK_STATE;
		indigo_update_property(device, AO_GUIDE_RA_PROPERTY, NULL);
	}
	AO_CENTER_ITEM->sw.value = AO_UNJAM_ITEM->sw.value = false;
	if (PRIVATE_DATA->response[0] != 'K') {
		AO_RESET_PROPERTY->state = INDIGO_ALERT_STATE;
	}
	//- ao.AO_RESET.on_change
	indigo_update_property(device, AO_RESET_PROPERTY, NULL);
}

#pragma mark - Device API (ao)

static indigo_result ao_enumerate_properties(indigo_device *device, indigo_client *client, indigo_property *property);

static indigo_result ao_attach(indigo_device *device) {
	if (indigo_ao_attach(device, DRIVER_NAME, DRIVER_VERSION) == INDIGO_OK) {
		ADDITIONAL_INSTANCES_PROPERTY->hidden = DEVICE_CONTEXT->base_device != NULL;
		DEVICE_PORT_PROPERTY->hidden = false;
		DEVICE_PORTS_PROPERTY->hidden = false;
		indigo_enumerate_serial_ports(device, DEVICE_PORTS_PROPERTY);
		//+ ao.on_attach
		AO_GUIDE_NORTH_ITEM->number.max = AO_GUIDE_SOUTH_ITEM->number.max = AO_GUIDE_EAST_ITEM->number.max = AO_GUIDE_WEST_ITEM->number.max = 50;
		INFO_PROPERTY->count = 6;
		INDIGO_COPY_VALUE(INFO_DEVICE_MODEL_ITEM->text.value, "Unknown");
		INDIGO_COPY_VALUE(INFO_DEVICE_FW_REVISION_ITEM->text.value, "Unknown");
		//- ao.on_attach
		AO_GUIDE_DEC_PROPERTY->hidden = false;
		AO_GUIDE_RA_PROPERTY->hidden = false;
		AO_RESET_PROPERTY->hidden = false;
		INDIGO_DEVICE_ATTACH_LOG(DRIVER_NAME, device->name);
		return ao_enumerate_properties(device, NULL, NULL);
	}
	return INDIGO_FAILED;
}

static indigo_result ao_enumerate_properties(indigo_device *device, indigo_client *client, indigo_property *property) {
	return indigo_ao_enumerate_properties(device, client, property);
}

static indigo_result ao_change_property(indigo_device *device, indigo_client *client, indigo_property *property) {
	if (indigo_property_match_changeable(CONNECTION_PROPERTY, property)) {
		if (!indigo_ignore_connection_change(device, property)) {
			indigo_property_copy_values(CONNECTION_PROPERTY, property, false);
			CONNECTION_PROPERTY->state = INDIGO_BUSY_STATE;
			indigo_update_property(device, CONNECTION_PROPERTY, NULL);
			indigo_execute_handler(device, ao_connection_handler);
		}
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(AO_GUIDE_DEC_PROPERTY, property)) {
		INDIGO_COPY_VALUES_PROCESS_CHANGE(AO_GUIDE_DEC_PROPERTY, ao_guide_dec_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(AO_GUIDE_RA_PROPERTY, property)) {
		INDIGO_COPY_VALUES_PROCESS_CHANGE(AO_GUIDE_RA_PROPERTY, ao_guide_ra_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(AO_RESET_PROPERTY, property)) {
		INDIGO_COPY_VALUES_PROCESS_CHANGE(AO_RESET_PROPERTY, ao_reset_handler);
		return INDIGO_OK;
	}
	return indigo_ao_change_property(device, client, property);
}

static indigo_result ao_detach(indigo_device *device) {
	if (IS_CONNECTED) {
		indigo_set_switch(CONNECTION_PROPERTY, CONNECTION_DISCONNECTED_ITEM, true);
		ao_connection_handler(device);
	}
	INDIGO_DEVICE_DETACH_LOG(DRIVER_NAME, device->name);
	return indigo_ao_detach(device);
}

#pragma mark - High level code (guider)

static void guider_connection_handler(indigo_device *device) {
	if (CONNECTION_CONNECTED_ITEM->sw.value) {
		bool connection_result = true;
		if (PRIVATE_DATA->count++ == 0) {
			connection_result = sx_open(device->master_device);
		}
		if (connection_result) {
			CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
			indigo_send_message(device, "Connected to %s on %s", GUIDER_DEVICE_NAME, DEVICE_PORT_ITEM->text.value);
		} else {
			indigo_send_message(device, "Failed to connect to %s on %s", GUIDER_DEVICE_NAME, DEVICE_PORT_ITEM->text.value);
			PRIVATE_DATA->count--;
			CONNECTION_PROPERTY->state = INDIGO_ALERT_STATE;
			indigo_set_switch(CONNECTION_PROPERTY, CONNECTION_DISCONNECTED_ITEM, true);
		}
	} else {
		indigo_cancel_pending_handlers(device);
		if (--PRIVATE_DATA->count == 0) {
			sx_close(device);
		}
		indigo_send_message(device, "Disconnected from %s", device->name);
		CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
	}
	indigo_guider_change_property(device, NULL, CONNECTION_PROPERTY);
}

static void guider_guide_dec_handler(indigo_device *device) {
	GUIDER_GUIDE_DEC_PROPERTY->state = INDIGO_OK_STATE;
	//+ guider.GUIDER_GUIDE_DEC.on_change
	if (GUIDER_GUIDE_NORTH_ITEM->number.value > 0) {
		sx_command(device, "MN%05d", 1, (int)GUIDER_GUIDE_NORTH_ITEM->number.value / 10);
	} else if (GUIDER_GUIDE_SOUTH_ITEM->number.value > 0) {
		sx_command(device, "MS%05d", 1, (int)GUIDER_GUIDE_SOUTH_ITEM->number.value / 10);
	}
	GUIDER_GUIDE_NORTH_ITEM->number.value = GUIDER_GUIDE_SOUTH_ITEM->number.value = 0;
	if (PRIVATE_DATA->response[0] != 'M') {
		GUIDER_GUIDE_DEC_PROPERTY->state = INDIGO_ALERT_STATE;
	}
	//- guider.GUIDER_GUIDE_DEC.on_change
	indigo_update_property(device, GUIDER_GUIDE_DEC_PROPERTY, NULL);
}

static void guider_guide_ra_handler(indigo_device *device) {
	GUIDER_GUIDE_RA_PROPERTY->state = INDIGO_OK_STATE;
	//+ guider.GUIDER_GUIDE_RA.on_change
	if (AO_GUIDE_WEST_ITEM->number.value > 0) {
		sx_command(device, "MW%05d", 1, (int)AO_GUIDE_WEST_ITEM->number.value);
	} else if (AO_GUIDE_EAST_ITEM->number.value > 0) {
		sx_command(device, "MT%05d", 1, (int)AO_GUIDE_EAST_ITEM->number.value);
	}
	AO_GUIDE_WEST_ITEM->number.value = AO_GUIDE_EAST_ITEM->number.value = 0;
	if (PRIVATE_DATA->response[0] != 'M') {
		AO_GUIDE_RA_PROPERTY->state = INDIGO_ALERT_STATE;
	}
	//- guider.GUIDER_GUIDE_RA.on_change
	indigo_update_property(device, GUIDER_GUIDE_RA_PROPERTY, NULL);
}

#pragma mark - Device API (guider)

static indigo_result guider_enumerate_properties(indigo_device *device, indigo_client *client, indigo_property *property);

static indigo_result guider_attach(indigo_device *device) {
	if (indigo_guider_attach(device, DRIVER_NAME, DRIVER_VERSION) == INDIGO_OK) {
		GUIDER_GUIDE_DEC_PROPERTY->hidden = false;
		GUIDER_GUIDE_RA_PROPERTY->hidden = false;
		INDIGO_DEVICE_ATTACH_LOG(DRIVER_NAME, device->name);
		return guider_enumerate_properties(device, NULL, NULL);
	}
	return INDIGO_FAILED;
}

static indigo_result guider_enumerate_properties(indigo_device *device, indigo_client *client, indigo_property *property) {
	return indigo_guider_enumerate_properties(device, client, property);
}

static indigo_result guider_change_property(indigo_device *device, indigo_client *client, indigo_property *property) {
	if (indigo_property_match_changeable(CONNECTION_PROPERTY, property)) {
		if (!indigo_ignore_connection_change(device, property)) {
			indigo_property_copy_values(CONNECTION_PROPERTY, property, false);
			CONNECTION_PROPERTY->state = INDIGO_BUSY_STATE;
			indigo_update_property(device, CONNECTION_PROPERTY, NULL);
			indigo_execute_handler(device, guider_connection_handler);
		}
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(GUIDER_GUIDE_DEC_PROPERTY, property)) {
		INDIGO_COPY_VALUES_PROCESS_CHANGE(GUIDER_GUIDE_DEC_PROPERTY, guider_guide_dec_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(GUIDER_GUIDE_RA_PROPERTY, property)) {
		INDIGO_COPY_VALUES_PROCESS_CHANGE(GUIDER_GUIDE_RA_PROPERTY, guider_guide_ra_handler);
		return INDIGO_OK;
	}
	return indigo_guider_change_property(device, client, property);
}

static indigo_result guider_detach(indigo_device *device) {
	if (IS_CONNECTED) {
		indigo_set_switch(CONNECTION_PROPERTY, CONNECTION_DISCONNECTED_ITEM, true);
		guider_connection_handler(device);
	}
	INDIGO_DEVICE_DETACH_LOG(DRIVER_NAME, device->name);
	return indigo_guider_detach(device);
}

#pragma mark - Device templates

static indigo_device ao_template = INDIGO_DEVICE_INITIALIZER(AO_DEVICE_NAME, ao_attach, ao_enumerate_properties, ao_change_property, NULL, ao_detach);

static indigo_device guider_template = INDIGO_DEVICE_INITIALIZER(GUIDER_DEVICE_NAME, guider_attach, guider_enumerate_properties, guider_change_property, NULL, guider_detach);

#pragma mark - Main code

indigo_result indigo_ao_sx(indigo_driver_action action, indigo_driver_info *info) {
	static indigo_driver_action last_action = INDIGO_DRIVER_SHUTDOWN;
	static sx_private_data *private_data = NULL;
	static indigo_device *ao = NULL;
	static indigo_device *guider = NULL;

	SET_DRIVER_INFO(info, DRIVER_LABEL, __FUNCTION__, DRIVER_VERSION, false, last_action);

	if (action == last_action) {
		return INDIGO_OK;
	}

	switch (action) {
		case INDIGO_DRIVER_INIT:
			last_action = action;
			private_data = indigo_safe_malloc(sizeof(sx_private_data));
			ao = indigo_safe_malloc_copy(sizeof(indigo_device), &ao_template);
			ao->private_data = private_data;
			indigo_attach_device(ao);
			guider = indigo_safe_malloc_copy(sizeof(indigo_device), &guider_template);
			guider->private_data = private_data;
			guider->master_device = ao;
			indigo_attach_device(guider);
			break;

		case INDIGO_DRIVER_SHUTDOWN:
			VERIFY_NOT_CONNECTED(ao);
			VERIFY_NOT_CONNECTED(guider);
			last_action = action;
			if (ao != NULL) {
				indigo_detach_device(ao);
				free(ao);
				ao = NULL;
			}
			if (guider != NULL) {
				indigo_detach_device(guider);
				free(guider);
				guider = NULL;
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
