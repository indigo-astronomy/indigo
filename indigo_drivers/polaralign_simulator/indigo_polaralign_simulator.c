// Copyright (c) 2026 by Rumen G.Bogdanovski
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

// This file generated from indigo_polaralign_simulator.driver

#pragma mark - Includes

#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <assert.h>
#include <indigo/indigo_driver_xml.h>
#include <indigo/indigo_polaralign_driver.h>
#include <indigo/indigo_uni_io.h>

#include "indigo_polaralign_simulator.h"

#pragma mark - Common definitions

#define DRIVER_VERSION       0x03000004
#define DRIVER_NAME          "indigo_polaralign_simulator"
#define DRIVER_LABEL         "Polar Aligner Simulator"
#define POLARALIGN_DEVICE_NAME DRIVER_LABEL
#define PRIVATE_DATA         ((simulator_private_data *)device->private_data)

//+ define

#define POLARALIGN_SPEED     0.5

//- define

#pragma mark - Private data definition

typedef struct {
	//+ data
	double target_altitude, current_altitude;
	double target_azimuth, current_azimuth;
	//- data
} simulator_private_data;

#pragma mark - Low level code

//+ code

static void polaralign_offset_handler(indigo_device *device);

static void polaralign_motion_finalizer(indigo_device *device) {
	bool altitude_done = false;
	bool azimuth_done = false;
	double altitude_delta = PRIVATE_DATA->target_altitude - PRIVATE_DATA->current_altitude;
	if (fabs(altitude_delta) > POLARALIGN_SPEED) {
		PRIVATE_DATA->current_altitude += copysign(POLARALIGN_SPEED, altitude_delta);
	} else {
		PRIVATE_DATA->current_altitude = PRIVATE_DATA->target_altitude;
		altitude_done = true;
	}
	POLARALIGN_OFFSET_ALT_ITEM->number.value = PRIVATE_DATA->current_altitude;
	double azimuth_delta = PRIVATE_DATA->target_azimuth - PRIVATE_DATA->current_azimuth;
	if (fabs(azimuth_delta) > POLARALIGN_SPEED) {
		PRIVATE_DATA->current_azimuth += copysign(POLARALIGN_SPEED, azimuth_delta);
	} else {
		PRIVATE_DATA->current_azimuth = PRIVATE_DATA->target_azimuth;
		azimuth_done = true;
	}
	POLARALIGN_OFFSET_AZ_ITEM->number.value = PRIVATE_DATA->current_azimuth;
	if (altitude_done && azimuth_done) {
		POLARALIGN_OFFSET_PROPERTY->state = INDIGO_OK_STATE;
		indigo_update_property(device, POLARALIGN_OFFSET_PROPERTY, NULL);
	} else {
		POLARALIGN_OFFSET_PROPERTY->state = INDIGO_BUSY_STATE;
		indigo_update_property(device, POLARALIGN_OFFSET_PROPERTY, NULL);
		indigo_execute_handler_in(device, 0.2, polaralign_motion_finalizer);
	}
}

//- code

#pragma mark - High level code (polaralign)

static void polaralign_connection_handler(indigo_device *device) {
	if (CONNECTION_CONNECTED_ITEM->sw.value) {
		CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
		indigo_send_message(device, OK_PROPERTY, "Connected to %s", device->name);
	} else {
		indigo_cancel_pending_handlers(device);
		//+ polaralign.on_disconnect
		PRIVATE_DATA->target_altitude = PRIVATE_DATA->current_altitude;
		PRIVATE_DATA->target_azimuth = PRIVATE_DATA->current_azimuth;
		POLARALIGN_OFFSET_ALT_ITEM->number.value = POLARALIGN_OFFSET_ALT_ITEM->number.target = PRIVATE_DATA->current_altitude;
		POLARALIGN_OFFSET_AZ_ITEM->number.value = POLARALIGN_OFFSET_AZ_ITEM->number.target = PRIVATE_DATA->current_azimuth;
		POLARALIGN_OFFSET_PROPERTY->state = INDIGO_OK_STATE;
		//- polaralign.on_disconnect
		indigo_send_message(device, OK_PROPERTY, "Disconnected from %s", device->name);
		CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
	}
	indigo_polaralign_change_property(device, NULL, CONNECTION_PROPERTY);
}

static void polaralign_offset_handler(indigo_device *device) {
	//+ polaralign.POLARALIGN_OFFSET.on_change
	double target_altitude = POLARALIGN_OFFSET_ALT_ITEM->number.target;
	double target_azimuth = POLARALIGN_OFFSET_AZ_ITEM->number.target;
	bool altitude_beyond = target_altitude < POLARALIGN_LIMITS_MIN_POSITION_ALT_ITEM->number.value || target_altitude > POLARALIGN_LIMITS_MAX_POSITION_ALT_ITEM->number.value;
	bool azimuth_beyond = target_azimuth < POLARALIGN_LIMITS_MIN_POSITION_AZ_ITEM->number.value || target_azimuth > POLARALIGN_LIMITS_MAX_POSITION_AZ_ITEM->number.value;
	POLARALIGN_OFFSET_ALT_ITEM->number.value = PRIVATE_DATA->current_altitude;
	POLARALIGN_OFFSET_AZ_ITEM->number.value = PRIVATE_DATA->current_azimuth;
	if (altitude_beyond || azimuth_beyond) {
		POLARALIGN_OFFSET_PROPERTY->state = INDIGO_ALERT_STATE;
		if (altitude_beyond && azimuth_beyond) {
			indigo_update_property(device, POLARALIGN_OFFSET_PROPERTY, "Both altitude (%.2f) and azimuth (%.2f) targets are beyond limits", target_altitude, target_azimuth);
		} else if (altitude_beyond) {
			indigo_update_property(device, POLARALIGN_OFFSET_PROPERTY, "Altitude target (%.2f) is beyond limits [%.2f, %.2f]", target_altitude, POLARALIGN_LIMITS_MIN_POSITION_ALT_ITEM->number.value, POLARALIGN_LIMITS_MAX_POSITION_ALT_ITEM->number.value);
		} else {
			indigo_update_property(device, POLARALIGN_OFFSET_PROPERTY, "Azimuth target (%.2f) is beyond limits [%.2f, %.2f]", target_azimuth, POLARALIGN_LIMITS_MIN_POSITION_AZ_ITEM->number.value, POLARALIGN_LIMITS_MAX_POSITION_AZ_ITEM->number.value);
		}
	} else {
		PRIVATE_DATA->target_altitude = target_altitude;
		PRIVATE_DATA->target_azimuth = target_azimuth;
		if (PRIVATE_DATA->target_altitude == PRIVATE_DATA->current_altitude && PRIVATE_DATA->target_azimuth == PRIVATE_DATA->current_azimuth) {
			POLARALIGN_OFFSET_PROPERTY->state = INDIGO_OK_STATE;
			indigo_update_property(device, POLARALIGN_OFFSET_PROPERTY, NULL);
		} else {
			POLARALIGN_OFFSET_PROPERTY->state = INDIGO_BUSY_STATE;
			indigo_update_property(device, POLARALIGN_OFFSET_PROPERTY, NULL);
			indigo_execute_handler_in(device, 0.2, polaralign_motion_finalizer);
		}
	}
	//- polaralign.POLARALIGN_OFFSET.on_change
}

static void polaralign_abort_motion_handler(indigo_device *device) {
	//+ polaralign.POLARALIGN_ABORT_MOTION.on_change
	if (POLARALIGN_ABORT_MOTION_ITEM->sw.value && POLARALIGN_OFFSET_PROPERTY->state == INDIGO_BUSY_STATE) {
		indigo_cancel_pending_handler(device, polaralign_offset_handler);
		indigo_cancel_pending_handler(device, polaralign_motion_finalizer);
		PRIVATE_DATA->target_altitude = PRIVATE_DATA->current_altitude;
		PRIVATE_DATA->target_azimuth = PRIVATE_DATA->current_azimuth;
		POLARALIGN_OFFSET_ALT_ITEM->number.value = POLARALIGN_OFFSET_ALT_ITEM->number.target = PRIVATE_DATA->current_altitude;
		POLARALIGN_OFFSET_AZ_ITEM->number.value = POLARALIGN_OFFSET_AZ_ITEM->number.target = PRIVATE_DATA->current_azimuth;
		POLARALIGN_OFFSET_PROPERTY->state = INDIGO_ALERT_STATE;
		indigo_update_property(device, POLARALIGN_OFFSET_PROPERTY, NULL);
	}
	POLARALIGN_ABORT_MOTION_ITEM->sw.value = false;
	POLARALIGN_ABORT_MOTION_PROPERTY->state = INDIGO_OK_STATE;
	indigo_update_property(device, POLARALIGN_ABORT_MOTION_PROPERTY, NULL);
	//- polaralign.POLARALIGN_ABORT_MOTION.on_change
}

static void polaralign_reset_position_alt_handler(indigo_device *device) {
	POLARALIGN_RESET_POSITION_ALT_PROPERTY->state = INDIGO_OK_STATE;
	//+ polaralign.POLARALIGN_RESET_POSITION_ALT.on_change
	if (POLARALIGN_RESET_POSITION_ALT_ITEM->sw.value) {
		PRIVATE_DATA->current_altitude = PRIVATE_DATA->target_altitude = 0;
		POLARALIGN_OFFSET_ALT_ITEM->number.value = POLARALIGN_OFFSET_ALT_ITEM->number.target = 0;
		indigo_update_property(device, POLARALIGN_OFFSET_PROPERTY, NULL);
	}
	POLARALIGN_RESET_POSITION_ALT_ITEM->sw.value = false;
	//- polaralign.POLARALIGN_RESET_POSITION_ALT.on_change
	indigo_update_property(device, POLARALIGN_RESET_POSITION_ALT_PROPERTY, NULL);
}

static void polaralign_reset_position_az_handler(indigo_device *device) {
	POLARALIGN_RESET_POSITION_AZ_PROPERTY->state = INDIGO_OK_STATE;
	//+ polaralign.POLARALIGN_RESET_POSITION_AZ.on_change
	if (POLARALIGN_RESET_POSITION_AZ_ITEM->sw.value) {
		PRIVATE_DATA->current_azimuth = PRIVATE_DATA->target_azimuth = 0;
		POLARALIGN_OFFSET_AZ_ITEM->number.value = POLARALIGN_OFFSET_AZ_ITEM->number.target = 0;
		indigo_update_property(device, POLARALIGN_OFFSET_PROPERTY, NULL);
	}
	POLARALIGN_RESET_POSITION_AZ_ITEM->sw.value = false;
	//- polaralign.POLARALIGN_RESET_POSITION_AZ.on_change
	indigo_update_property(device, POLARALIGN_RESET_POSITION_AZ_PROPERTY, NULL);
}

#pragma mark - Device API (polaralign)

static indigo_result polaralign_enumerate_properties(indigo_device *device, indigo_client *client, indigo_property *property);

static indigo_result polaralign_attach(indigo_device *device) {
	if (indigo_polaralign_attach(device, DRIVER_NAME, DRIVER_VERSION) == INDIGO_OK) {
		ADDITIONAL_INSTANCES_PROPERTY->hidden = device->base_device != NULL;
		POLARALIGN_OFFSET_PROPERTY->hidden = false;
		POLARALIGN_ABORT_MOTION_PROPERTY->hidden = false;
		POLARALIGN_STEPS_PER_DEGREE_PROPERTY->hidden = false;
		//+ polaralign.POLARALIGN_STEPS_PER_DEGREE.on_attach
		POLARALIGN_STEPS_PER_DEGREE_ALT_ITEM->number.value = POLARALIGN_STEPS_PER_DEGREE_ALT_ITEM->number.target = 360;
		POLARALIGN_STEPS_PER_DEGREE_AZ_ITEM->number.value = POLARALIGN_STEPS_PER_DEGREE_AZ_ITEM->number.target = 360;
		//- polaralign.POLARALIGN_STEPS_PER_DEGREE.on_attach
		POLARALIGN_RESET_POSITION_ALT_PROPERTY->hidden = false;
		POLARALIGN_RESET_POSITION_AZ_PROPERTY->hidden = false;
		INDIGO_DEVICE_ATTACH_LOG(DRIVER_NAME, device->name);
		return polaralign_enumerate_properties(device, NULL, NULL);
	}
	return INDIGO_FAILED;
}

static indigo_result polaralign_enumerate_properties(indigo_device *device, indigo_client *client, indigo_property *property) {
	return indigo_polaralign_enumerate_properties(device, client, property);
}

static indigo_result polaralign_change_property(indigo_device *device, indigo_client *client, indigo_property *property) {
	if (indigo_property_match_changeable(CONNECTION_PROPERTY, property)) {
		if (!indigo_ignore_connection_change(device, property)) {
			indigo_property_copy_values(CONNECTION_PROPERTY, property, false);
			INDIGO_UPDATE_PROPERTY_STATE(CONNECTION_PROPERTY, INDIGO_BUSY_STATE, NULL);
			indigo_execute_handler(device, polaralign_connection_handler);
		}
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(POLARALIGN_OFFSET_PROPERTY, property)) {
		INDIGO_COPY_TARGETS_PROCESS_CHANGE(POLARALIGN_OFFSET_PROPERTY, polaralign_offset_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(POLARALIGN_ABORT_MOTION_PROPERTY, property)) {
		INDIGO_COPY_VALUES_PROCESS_URGENT_CHANGE(POLARALIGN_ABORT_MOTION_PROPERTY, polaralign_abort_motion_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(POLARALIGN_RESET_POSITION_ALT_PROPERTY, property)) {
		INDIGO_COPY_VALUES_PROCESS_CHANGE(POLARALIGN_RESET_POSITION_ALT_PROPERTY, polaralign_reset_position_alt_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(POLARALIGN_RESET_POSITION_AZ_PROPERTY, property)) {
		INDIGO_COPY_VALUES_PROCESS_CHANGE(POLARALIGN_RESET_POSITION_AZ_PROPERTY, polaralign_reset_position_az_handler);
		return INDIGO_OK;
	}
	return indigo_polaralign_change_property(device, client, property);
}

static indigo_result polaralign_detach(indigo_device *device) {
	if (IS_CONNECTED) {
		indigo_set_switch(CONNECTION_PROPERTY, CONNECTION_DISCONNECTED_ITEM, true);
		polaralign_connection_handler(device);
	}
	INDIGO_DEVICE_DETACH_LOG(DRIVER_NAME, device->name);
	return indigo_polaralign_detach(device);
}

#pragma mark - Device templates

static indigo_device polaralign_template = INDIGO_DEVICE_INITIALIZER(POLARALIGN_DEVICE_NAME, polaralign_attach, polaralign_enumerate_properties, polaralign_change_property, NULL, polaralign_detach);

#pragma mark - Main code

indigo_result indigo_polaralign_simulator(indigo_driver_action action, indigo_driver_info *info) {
	static indigo_driver_action last_action = INDIGO_DRIVER_SHUTDOWN;
	static simulator_private_data *private_data = NULL;
	static indigo_device *polaralign = NULL;

	SET_DRIVER_INFO(info, DRIVER_LABEL, __FUNCTION__, DRIVER_VERSION, true, last_action);

	if (action == last_action) {
		return INDIGO_OK;
	}

	switch (action) {
		case INDIGO_DRIVER_INIT: {
			last_action = action;
			private_data = (simulator_private_data *)indigo_safe_malloc(sizeof(simulator_private_data));
			polaralign = (indigo_device *)indigo_safe_malloc_copy(sizeof(indigo_device), &polaralign_template);
			polaralign->private_data = private_data;
			indigo_attach_device(polaralign);
			break;

		}
		case INDIGO_DRIVER_SHUTDOWN: {
			VERIFY_NOT_CONNECTED(polaralign);
			last_action = action;
			if (polaralign != NULL) {
				indigo_detach_device(polaralign);
				indigo_safe_free(polaralign);
				polaralign = NULL;
			}
			if (private_data != NULL) {
				indigo_safe_free(private_data);
				private_data = NULL;
			}
			break;

		}
		case INDIGO_DRIVER_INFO:
			break;
	}

	return INDIGO_OK;
}
