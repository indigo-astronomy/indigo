// Copyright (c) 2026 by Rumen G.Bogdanovski
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

/** INDIGO Polar Aligner Simulator driver
 \file indigo_polaralign_simulator.c
 */

#define DRIVER_VERSION 0x0001
#define DRIVER_NAME    "indigo_polaralign_simulator"

#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <math.h>
#include <assert.h>
#include <pthread.h>

#include <indigo/indigo_driver_xml.h>

#include "indigo_polaralign_simulator.h"

/** Slew speed in arcmin per timer tick (0.2 s). */
#define POLARALIGN_SPEED 0.5

#define PRIVATE_DATA	((simulator_private_data *)device->private_data)

typedef struct {
	double target_altitude, current_altitude;
	double target_azimuth,  current_azimuth;
	indigo_timer *motion_timer;
} simulator_private_data;

// -------------------------------------------------------------------------------- INDIGO polar aligner device implementation

static void polaralign_timer_callback(indigo_device *device) {
	bool altitude_done = false;
	bool azimuth_done  = false;

	if (POLARALIGN_OFFSET_PROPERTY->state == INDIGO_ALERT_STATE) {
		/* Aborted – snap to current hardware position */
		POLARALIGN_OFFSET_ALT_ITEM->number.value = PRIVATE_DATA->target_altitude = PRIVATE_DATA->current_altitude;
		POLARALIGN_OFFSET_AZ_ITEM->number.value  = PRIVATE_DATA->target_azimuth  = PRIVATE_DATA->current_azimuth;
		indigo_update_property(device, POLARALIGN_OFFSET_PROPERTY, NULL);
		return;
	}

	/* Altitude axis */
	double alt_diff = PRIVATE_DATA->target_altitude - PRIVATE_DATA->current_altitude;
	if (fabs(alt_diff) > POLARALIGN_SPEED) {
		PRIVATE_DATA->current_altitude += copysign(POLARALIGN_SPEED, alt_diff);
	} else {
		PRIVATE_DATA->current_altitude = PRIVATE_DATA->target_altitude;
		altitude_done = true;
	}
	POLARALIGN_OFFSET_ALT_ITEM->number.value = PRIVATE_DATA->current_altitude;

	/* Azimuth axis */
	double az_diff = PRIVATE_DATA->target_azimuth - PRIVATE_DATA->current_azimuth;
	if (fabs(az_diff) > POLARALIGN_SPEED) {
		PRIVATE_DATA->current_azimuth += copysign(POLARALIGN_SPEED, az_diff);
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
		indigo_reschedule_timer(device, 0.2, &PRIVATE_DATA->motion_timer);
	}
}

static indigo_result polaralign_attach(indigo_device *device) {
	assert(device != NULL);
	assert(PRIVATE_DATA != NULL);
	if (indigo_polaralign_attach(device, DRIVER_NAME, DRIVER_VERSION) == INDIGO_OK) {
		ADDITIONAL_INSTANCES_PROPERTY->hidden = DEVICE_CONTEXT->base_device != NULL;
		POLARALIGN_STEPS_PER_DEGREE_ALT_ITEM->number.value = POLARALIGN_STEPS_PER_DEGREE_ALT_ITEM->number.target = 360;
		POLARALIGN_STEPS_PER_DEGREE_AZ_ITEM->number.value  = POLARALIGN_STEPS_PER_DEGREE_AZ_ITEM->number.target  = 360;
		INDIGO_DEVICE_ATTACH_LOG(DRIVER_NAME, device->name);
		return indigo_polaralign_enumerate_properties(device, NULL, NULL);
	}
	return INDIGO_FAILED;
}

static void polaralign_connect_callback(indigo_device *device) {
	CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
	if (!CONNECTION_CONNECTED_ITEM->sw.value) {
		indigo_cancel_timer_sync(device, &PRIVATE_DATA->motion_timer);
	}
	indigo_polaralign_change_property(device, NULL, CONNECTION_PROPERTY);
}

static indigo_result polaralign_change_property(indigo_device *device, indigo_client *client, indigo_property *property) {
	assert(device != NULL);
	assert(DEVICE_CONTEXT != NULL);
	assert(property != NULL);
	if (indigo_property_match_changeable(CONNECTION_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- CONNECTION
		if (indigo_ignore_connection_change(device, property))
			return INDIGO_OK;
		indigo_property_copy_values(CONNECTION_PROPERTY, property, false);
		CONNECTION_PROPERTY->state = INDIGO_BUSY_STATE;
		indigo_update_property(device, CONNECTION_PROPERTY, NULL);
		indigo_set_timer(device, 0, polaralign_connect_callback, NULL);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(POLARALIGN_OFFSET_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- POLARALIGN_OFFSET
		indigo_property_copy_values(POLARALIGN_OFFSET_PROPERTY, property, false);
		double target_alt = POLARALIGN_OFFSET_ALT_ITEM->number.target;
		double target_az  = POLARALIGN_OFFSET_AZ_ITEM->number.target;
		/* Check limits */
		bool alt_beyond = (target_alt < POLARALIGN_LIMITS_MIN_POSITION_ALT_ITEM->number.value || target_alt > POLARALIGN_LIMITS_MAX_POSITION_ALT_ITEM->number.value);
		bool az_beyond  = (target_az  < POLARALIGN_LIMITS_MIN_POSITION_AZ_ITEM->number.value  || target_az  > POLARALIGN_LIMITS_MAX_POSITION_AZ_ITEM->number.value);
		if (alt_beyond || az_beyond) {
			POLARALIGN_OFFSET_PROPERTY->state = INDIGO_ALERT_STATE;
			/* Restore current readout unchanged */
			POLARALIGN_OFFSET_ALT_ITEM->number.value = PRIVATE_DATA->current_altitude;
			POLARALIGN_OFFSET_AZ_ITEM->number.value  = PRIVATE_DATA->current_azimuth;
			if (alt_beyond && az_beyond)
				indigo_update_property(device, POLARALIGN_OFFSET_PROPERTY, "Both altitude (%.2f) and azimuth (%.2f) targets are beyond limits", target_alt, target_az);
			else if (alt_beyond)
				indigo_update_property(device, POLARALIGN_OFFSET_PROPERTY, "Altitude target (%.2f) is beyond limits [%.2f, %.2f]", target_alt, POLARALIGN_LIMITS_MIN_POSITION_ALT_ITEM->number.value, POLARALIGN_LIMITS_MAX_POSITION_ALT_ITEM->number.value);
			else
				indigo_update_property(device, POLARALIGN_OFFSET_PROPERTY, "Azimuth target (%.2f) is beyond limits [%.2f, %.2f]", target_az, POLARALIGN_LIMITS_MIN_POSITION_AZ_ITEM->number.value, POLARALIGN_LIMITS_MAX_POSITION_AZ_ITEM->number.value);
			return INDIGO_OK;
		}
		/* Restore current readout so movement starts from the actual position. */
		POLARALIGN_OFFSET_ALT_ITEM->number.value = PRIVATE_DATA->current_altitude;
		POLARALIGN_OFFSET_AZ_ITEM->number.value  = PRIVATE_DATA->current_azimuth;
		if (PRIVATE_DATA->target_altitude == PRIVATE_DATA->current_altitude &&
		    PRIVATE_DATA->target_azimuth  == PRIVATE_DATA->current_azimuth) {
			POLARALIGN_OFFSET_PROPERTY->state = INDIGO_OK_STATE;
			indigo_update_property(device, POLARALIGN_OFFSET_PROPERTY, NULL);
		} else {
			POLARALIGN_OFFSET_PROPERTY->state = INDIGO_BUSY_STATE;
			indigo_update_property(device, POLARALIGN_OFFSET_PROPERTY, NULL);
			indigo_set_timer(device, 0.2, polaralign_timer_callback, &PRIVATE_DATA->motion_timer);
		}
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(POLARALIGN_RESET_POSITION_ALT_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- POLARALIGN_RESET_POSITION_ALT
		indigo_property_copy_values(POLARALIGN_RESET_POSITION_ALT_PROPERTY, property, false);
		if (POLARALIGN_RESET_POSITION_ALT_ITEM->sw.value) {
			PRIVATE_DATA->current_altitude = 0;
			PRIVATE_DATA->target_altitude  = 0;
			POLARALIGN_OFFSET_ALT_ITEM->number.value  = 0;
			POLARALIGN_OFFSET_ALT_ITEM->number.target = 0;
			indigo_update_property(device, POLARALIGN_OFFSET_PROPERTY, NULL);
		}
		POLARALIGN_RESET_POSITION_ALT_ITEM->sw.value = false;
		POLARALIGN_RESET_POSITION_ALT_PROPERTY->state = INDIGO_OK_STATE;
		indigo_update_property(device, POLARALIGN_RESET_POSITION_ALT_PROPERTY, NULL);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(POLARALIGN_RESET_POSITION_AZ_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- POLARALIGN_RESET_POSITION_AZ
		indigo_property_copy_values(POLARALIGN_RESET_POSITION_AZ_PROPERTY, property, false);
		if (POLARALIGN_RESET_POSITION_AZ_ITEM->sw.value) {
			PRIVATE_DATA->current_azimuth = 0;
			PRIVATE_DATA->target_azimuth  = 0;
			POLARALIGN_OFFSET_AZ_ITEM->number.value  = 0;
			POLARALIGN_OFFSET_AZ_ITEM->number.target = 0;
			indigo_update_property(device, POLARALIGN_OFFSET_PROPERTY, NULL);
		}
		POLARALIGN_RESET_POSITION_AZ_ITEM->sw.value = false;
		POLARALIGN_RESET_POSITION_AZ_PROPERTY->state = INDIGO_OK_STATE;
		indigo_update_property(device, POLARALIGN_RESET_POSITION_AZ_PROPERTY, NULL);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(POLARALIGN_ABORT_MOTION_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- POLARALIGN_ABORT_MOTION
		indigo_property_copy_values(POLARALIGN_ABORT_MOTION_PROPERTY, property, false);
		if (POLARALIGN_ABORT_MOTION_ITEM->sw.value && POLARALIGN_OFFSET_PROPERTY->state == INDIGO_BUSY_STATE) {
			POLARALIGN_OFFSET_PROPERTY->state = INDIGO_ALERT_STATE;
			indigo_update_property(device, POLARALIGN_OFFSET_PROPERTY, NULL);
		}
		POLARALIGN_ABORT_MOTION_PROPERTY->state = INDIGO_OK_STATE;
		POLARALIGN_ABORT_MOTION_ITEM->sw.value = false;
		indigo_update_property(device, POLARALIGN_ABORT_MOTION_PROPERTY, NULL);
		return INDIGO_OK;
		// --------------------------------------------------------------------------------
	}
	return indigo_polaralign_change_property(device, client, property);
}

static indigo_result polaralign_detach(indigo_device *device) {
	assert(device != NULL);
	if (IS_CONNECTED) {
		indigo_set_switch(CONNECTION_PROPERTY, CONNECTION_DISCONNECTED_ITEM, true);
		polaralign_connect_callback(device);
	}
	INDIGO_DEVICE_DETACH_LOG(DRIVER_NAME, device->name);
	return indigo_polaralign_detach(device);
}

// --------------------------------------------------------------------------------

static simulator_private_data *private_data = NULL;
static indigo_device *polaralign_device = NULL;

indigo_result indigo_polaralign_simulator(indigo_driver_action action, indigo_driver_info *info) {
	static indigo_device polaralign_device_template = INDIGO_DEVICE_INITIALIZER(
		SIMULATOR_POLARALIGN_NAME,
		polaralign_attach,
		indigo_polaralign_enumerate_properties,
		polaralign_change_property,
		NULL,
		polaralign_detach
	);

	static indigo_driver_action last_action = INDIGO_DRIVER_SHUTDOWN;

	SET_DRIVER_INFO(info, "Polar Aligner Simulator", __FUNCTION__, DRIVER_VERSION, true, last_action);

	if (action == last_action)
		return INDIGO_OK;

	switch (action) {
		case INDIGO_DRIVER_INIT:
			last_action = action;
			private_data = indigo_safe_malloc(sizeof(simulator_private_data));
			polaralign_device = indigo_safe_malloc_copy(sizeof(indigo_device), &polaralign_device_template);
			polaralign_device->private_data = private_data;
			indigo_attach_device(polaralign_device);
			break;

		case INDIGO_DRIVER_SHUTDOWN:
			VERIFY_NOT_CONNECTED(polaralign_device);
			last_action = action;
			if (polaralign_device != NULL) {
				indigo_detach_device(polaralign_device);
				free(polaralign_device);
				polaralign_device = NULL;
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
