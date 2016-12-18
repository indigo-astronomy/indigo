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
// 2.0 Build 0 - PoC by Peter Polakovic <peter.polakovic@cloudmakers.eu>

/** INDIGO MOUNT Simulator driver
 \file indigo_mount_simulator.c
 */

#define DRIVER_VERSION 0x0001

#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <math.h>
#include <assert.h>

#include "indigo_driver_xml.h"

#include "indigo_mount_simulator.h"

#undef PRIVATE_DATA
#define PRIVATE_DATA        ((simulator_private_data *)DEVICE_CONTEXT->private_data)

typedef struct {
	bool parked;
	indigo_timer *slew_timer, *guider_timer;
} simulator_private_data;

// -------------------------------------------------------------------------------- INDIGO MOUNT device implementation

static void slew_timer_callback(indigo_device *device) {
	double diffRA = MOUNT_EQUATORIAL_COORDINATES_RA_ITEM->number.target - MOUNT_EQUATORIAL_COORDINATES_RA_ITEM->number.value;
	double diffDec = MOUNT_EQUATORIAL_COORDINATES_DEC_ITEM->number.target - MOUNT_EQUATORIAL_COORDINATES_DEC_ITEM->number.value;
	if (diffRA == 0 && diffDec == 0) {
		MOUNT_EQUATORIAL_COORDINATES_PROPERTY->state = INDIGO_OK_STATE;
		PRIVATE_DATA->slew_timer = NULL;
	} else {
		double speedRA = 0.1;
		double speedDec = 1.5;
		if (fabs(diffRA) < speedRA)
			MOUNT_EQUATORIAL_COORDINATES_RA_ITEM->number.value = MOUNT_EQUATORIAL_COORDINATES_RA_ITEM->number.target;
		else if (diffRA > 0)
			MOUNT_EQUATORIAL_COORDINATES_RA_ITEM->number.value += speedRA;
		else if (diffRA < 0)
			MOUNT_EQUATORIAL_COORDINATES_RA_ITEM->number.value -= speedRA;
		if (fabs(diffDec) < speedDec)
			MOUNT_EQUATORIAL_COORDINATES_DEC_ITEM->number.value = MOUNT_EQUATORIAL_COORDINATES_DEC_ITEM->number.target;
		else if (diffDec > 0)
			MOUNT_EQUATORIAL_COORDINATES_DEC_ITEM->number.value += speedDec;
		else if (diffDec < 0)
			MOUNT_EQUATORIAL_COORDINATES_DEC_ITEM->number.value -= speedDec;
		MOUNT_EQUATORIAL_COORDINATES_PROPERTY->state = INDIGO_BUSY_STATE;
		indigo_reschedule_timer(device, 0.2, &PRIVATE_DATA->slew_timer);
	}
	indigo_update_property(device, MOUNT_EQUATORIAL_COORDINATES_PROPERTY, NULL);
}

static indigo_result mount_attach(indigo_device *device) {
	assert(device != NULL);
	assert(device->device_context != NULL);
	
	simulator_private_data *private_data = device->device_context;
	device->device_context = NULL;
	
	if (indigo_mount_attach(device, DRIVER_VERSION) == INDIGO_OK) {
		DEVICE_CONTEXT->private_data = private_data;
		// -------------------------------------------------------------------------------- SIMULATION
		SIMULATION_PROPERTY->hidden = false;
		SIMULATION_PROPERTY->perm = INDIGO_RO_PERM;
		SIMULATION_ENABLED_ITEM->sw.value = true;
		SIMULATION_DISABLED_ITEM->sw.value = false;
		// -------------------------------------------------------------------------------- DEVICE_PORT
		DEVICE_PORT_PROPERTY->hidden = false;
		// -------------------------------------------------------------------------------- MOUNT_PARK
		PRIVATE_DATA->parked = true;
		// -------------------------------------------------------------------------------- MOUNT_ON_COORDINATES_SET
		MOUNT_ON_COORDINATES_SET_PROPERTY->count = 2;
		// -------------------------------------------------------------------------------- MOUNT_EQUATORIAL_COORDINATES
		MOUNT_EQUATORIAL_COORDINATES_RA_ITEM->number.value = MOUNT_EQUATORIAL_COORDINATES_RA_ITEM->number.value = MOUNT_EQUATORIAL_COORDINATES_RA_ITEM->number.target = 0;
		MOUNT_EQUATORIAL_COORDINATES_DEC_ITEM->number.value = MOUNT_EQUATORIAL_COORDINATES_DEC_ITEM->number.value = MOUNT_EQUATORIAL_COORDINATES_DEC_ITEM->number.target = 90;
		// --------------------------------------------------------------------------------
		INDIGO_LOG(indigo_log("%s attached", device->name));
		return indigo_mount_enumerate_properties(device, NULL, NULL);
	}
	return INDIGO_FAILED;
}

static indigo_result mount_change_property(indigo_device *device, indigo_client *client, indigo_property *property) {
	assert(device != NULL);
	assert(device->device_context != NULL);
	assert(property != NULL);
	if (indigo_property_match(CONNECTION_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- CONNECTION
		indigo_property_copy_values(CONNECTION_PROPERTY, property, false);
		CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
	} else if (indigo_property_match(MOUNT_PARK_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- MOUNT_PARK
		indigo_property_copy_values(MOUNT_PARK_PROPERTY, property, false);
		if (MOUNT_PARK_PARKED_ITEM->sw.value) {
			MOUNT_PARK_PROPERTY->state = INDIGO_BUSY_STATE;
			indigo_update_property(device, MOUNT_PARK_PROPERTY, "Parking...");
			sleep(1);
			MOUNT_PARK_PROPERTY->state = INDIGO_OK_STATE;
			indigo_update_property(device, MOUNT_PARK_PROPERTY, "Parked");
			PRIVATE_DATA->parked = true;
		} else {
			MOUNT_PARK_PROPERTY->state = INDIGO_BUSY_STATE;
			indigo_update_property(device, MOUNT_PARK_PROPERTY, "Unparking...");
			sleep(1);
			MOUNT_PARK_PROPERTY->state = INDIGO_OK_STATE;
			indigo_update_property(device, MOUNT_PARK_PROPERTY, "Unparked");
			PRIVATE_DATA->parked = false;
		}
		return INDIGO_OK;
	} else if (indigo_property_match(MOUNT_EQUATORIAL_COORDINATES_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- MOUNT_EQUATORIAL_COORDINATES
		double ra = MOUNT_EQUATORIAL_COORDINATES_RA_ITEM->number.value;
		double dec = MOUNT_EQUATORIAL_COORDINATES_DEC_ITEM->number.value;
		indigo_property_copy_values(MOUNT_EQUATORIAL_COORDINATES_PROPERTY, property, false);
		MOUNT_EQUATORIAL_COORDINATES_RA_ITEM->number.value = ra;
		MOUNT_EQUATORIAL_COORDINATES_DEC_ITEM->number.value = dec;
		if (MOUNT_ON_COORDINATES_SET_TRACK_ITEM->sw.value) {
			slew_timer_callback(device);
		}
		return INDIGO_OK;
	} else if (indigo_property_match(MOUNT_ABORT_MOTION_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- MOUNT_ABORT_MOTION
		indigo_property_copy_values(MOUNT_ABORT_MOTION_PROPERTY, property, false);
		if (indigo_cancel_timer(device, &PRIVATE_DATA->slew_timer)) {
			MOUNT_EQUATORIAL_COORDINATES_PROPERTY->state = INDIGO_ALERT_STATE;
			indigo_update_property(device, MOUNT_EQUATORIAL_COORDINATES_PROPERTY, NULL);
		}
		MOUNT_ABORT_MOTION_PROPERTY->state = INDIGO_OK_STATE;
		indigo_update_property(device, MOUNT_ABORT_MOTION_PROPERTY, "Aborted");
		return INDIGO_OK;
		// --------------------------------------------------------------------------------
	}
	return indigo_mount_change_property(device, client, property);
}

static indigo_result mount_detach(indigo_device *device) {
	assert(device != NULL);
	if (CONNECTION_CONNECTED_ITEM->sw.value)
		indigo_device_disconnect(NULL, device);
	INDIGO_LOG(indigo_log("%s detached", device->name));
	return indigo_mount_detach(device);
}

// -------------------------------------------------------------------------------- INDIGO guider device implementation

static void guider_timer_callback(indigo_device *device) {
	PRIVATE_DATA->guider_timer = NULL;
	if (GUIDER_GUIDE_NORTH_ITEM->number.value != 0 || GUIDER_GUIDE_SOUTH_ITEM->number.value != 0) {
		GUIDER_GUIDE_NORTH_ITEM->number.value = 0;
		GUIDER_GUIDE_SOUTH_ITEM->number.value = 0;
		GUIDER_GUIDE_DEC_PROPERTY->state = INDIGO_OK_STATE;
		indigo_update_property(device, GUIDER_GUIDE_DEC_PROPERTY, NULL);
	}
	if (GUIDER_GUIDE_EAST_ITEM->number.value != 0 || GUIDER_GUIDE_WEST_ITEM->number.value != 0) {
		GUIDER_GUIDE_EAST_ITEM->number.value = 0;
		GUIDER_GUIDE_WEST_ITEM->number.value = 0;
		GUIDER_GUIDE_RA_PROPERTY->state = INDIGO_OK_STATE;
		indigo_update_property(device, GUIDER_GUIDE_RA_PROPERTY, NULL);
	}
}

static indigo_result guider_attach(indigo_device *device) {
	assert(device != NULL);
	assert(device->device_context != NULL);
	simulator_private_data *private_data = device->device_context;
	device->device_context = NULL;
	if (indigo_guider_attach(device, DRIVER_VERSION) == INDIGO_OK) {
		DEVICE_CONTEXT->private_data = private_data;
		INDIGO_LOG(indigo_log("%s attached", device->name));
		return indigo_guider_enumerate_properties(device, NULL, NULL);
	}
	return INDIGO_FAILED;
}

static indigo_result guider_change_property(indigo_device *device, indigo_client *client, indigo_property *property) {
	assert(device != NULL);
	assert(device->device_context != NULL);
	assert(property != NULL);
	if (indigo_property_match(CONNECTION_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- CONNECTION
		indigo_property_copy_values(CONNECTION_PROPERTY, property, false);
		CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
	} else if (indigo_property_match(GUIDER_GUIDE_DEC_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- GUIDER_GUIDE_DEC
		indigo_cancel_timer(device, &PRIVATE_DATA->guider_timer);
		indigo_property_copy_values(GUIDER_GUIDE_DEC_PROPERTY, property, false);
		GUIDER_GUIDE_DEC_PROPERTY->state = INDIGO_OK_STATE;
		int duration = GUIDER_GUIDE_NORTH_ITEM->number.value;
		if (duration > 0) {
			GUIDER_GUIDE_DEC_PROPERTY->state = INDIGO_BUSY_STATE;
			PRIVATE_DATA->guider_timer = indigo_set_timer(device, duration/1000.0, guider_timer_callback);
		} else {
			int duration = GUIDER_GUIDE_SOUTH_ITEM->number.value;
			if (duration > 0) {
				GUIDER_GUIDE_DEC_PROPERTY->state = INDIGO_BUSY_STATE;
				PRIVATE_DATA->guider_timer = indigo_set_timer(device, duration/1000.0, guider_timer_callback);
			}
		}
		indigo_update_property(device, GUIDER_GUIDE_DEC_PROPERTY, NULL);
		return INDIGO_OK;
	} else if (indigo_property_match(GUIDER_GUIDE_RA_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- GUIDER_GUIDE_RA
		indigo_cancel_timer(device, &PRIVATE_DATA->guider_timer);
		indigo_property_copy_values(GUIDER_GUIDE_RA_PROPERTY, property, false);
		GUIDER_GUIDE_RA_PROPERTY->state = INDIGO_OK_STATE;
		int duration = GUIDER_GUIDE_EAST_ITEM->number.value;
		if (duration > 0) {
			GUIDER_GUIDE_RA_PROPERTY->state = INDIGO_BUSY_STATE;
			PRIVATE_DATA->guider_timer = indigo_set_timer(device, duration/1000.0, guider_timer_callback);
		} else {
			int duration = GUIDER_GUIDE_WEST_ITEM->number.value;
			if (duration > 0) {
				GUIDER_GUIDE_RA_PROPERTY->state = INDIGO_BUSY_STATE;
				PRIVATE_DATA->guider_timer = indigo_set_timer(device, duration/1000.0, guider_timer_callback);
			}
		}
		indigo_update_property(device, GUIDER_GUIDE_RA_PROPERTY, NULL);
		return INDIGO_OK;
		// --------------------------------------------------------------------------------
	}
	return indigo_guider_change_property(device, client, property);
}

static indigo_result guider_detach(indigo_device *device) {
	assert(device != NULL);
	if (CONNECTION_CONNECTED_ITEM->sw.value)
		indigo_device_disconnect(NULL, device);
	INDIGO_LOG(indigo_log("%s detached", device->name));
	return indigo_guider_detach(device);
}

// --------------------------------------------------------------------------------

static simulator_private_data *private_data = NULL;

static indigo_device *mount = NULL;
static indigo_device *mount_guider = NULL;

indigo_result indigo_mount_simulator(indigo_driver_action action, indigo_driver_info *info) {
	static indigo_device mount_template = {
		MOUNT_SIMULATOR_NAME, NULL, INDIGO_OK, INDIGO_VERSION_CURRENT,
		mount_attach,
		indigo_mount_enumerate_properties,
		mount_change_property,
		mount_detach
	};
	static indigo_device mount_guider_template = {
		MOUNT_SIMULATOR_GUIDER_NAME, NULL, INDIGO_OK, INDIGO_VERSION_CURRENT,
		guider_attach,
		indigo_guider_enumerate_properties,
		guider_change_property,
		guider_detach
	};

	static indigo_driver_action last_action = INDIGO_DRIVER_SHUTDOWN;

	SET_DRIVER_INFO(info, "Mount Simulator", __FUNCTION__, DRIVER_VERSION, last_action);

	if (action == last_action)
		return INDIGO_OK;

	switch (action) {
	case INDIGO_DRIVER_INIT:
		last_action = action;
		private_data = malloc(sizeof(simulator_private_data));
		assert(private_data != NULL);
		mount = malloc(sizeof(indigo_device));
		assert(mount != NULL);
		memcpy(mount, &mount_template, sizeof(indigo_device));
		mount->device_context = private_data;
		indigo_attach_device(mount);
		mount_guider = malloc(sizeof(indigo_device));
		assert(mount_guider != NULL);
		memcpy(mount_guider, &mount_guider_template, sizeof(indigo_device));
		mount_guider->device_context = private_data;
		indigo_attach_device(mount_guider);
		break;

	case INDIGO_DRIVER_SHUTDOWN:
		last_action = action;
		if (mount != NULL) {
			indigo_detach_device(mount);
			free(mount);
			mount = NULL;
		}
		if (mount_guider != NULL) {
			indigo_detach_device(mount_guider);
			free(mount_guider);
			mount_guider = NULL;
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

