// Copyright (c) 2020 Rumen G.Bogdanovski
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

/** INDIGO Field Rotator Simulator driver
 \file indigo_rotator_simulator.c
 */

#define DRIVER_VERSION 0x0002
#define DRIVER_NAME	"indigo_rotator_simulator"

#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <math.h>
#include <assert.h>
#include <pthread.h>

#include <indigo/indigo_driver_xml.h>

#include "indigo_rotator_simulator.h"

#define PRIVATE_DATA								((simulator_private_data *)device->private_data)

#define ROTATOR_SPEED 0.9

typedef struct {
	double target_position, current_position;
	indigo_timer *rotator_timer;
} simulator_private_data;

// -------------------------------------------------------------------------------- INDIGO rotator device implementation

static void rotator_timer_callback(indigo_device *device) {
	if (ROTATOR_POSITION_PROPERTY->state == INDIGO_ALERT_STATE) {
		ROTATOR_POSITION_ITEM->number.value = PRIVATE_DATA->target_position = PRIVATE_DATA->current_position;
		indigo_update_property(device, ROTATOR_POSITION_PROPERTY, NULL);
	} else {
		if (PRIVATE_DATA->current_position < PRIVATE_DATA->target_position) {
			ROTATOR_POSITION_PROPERTY->state = INDIGO_BUSY_STATE;
			if (PRIVATE_DATA->target_position - PRIVATE_DATA->current_position > ROTATOR_SPEED)
				ROTATOR_POSITION_ITEM->number.value = PRIVATE_DATA->current_position = (PRIVATE_DATA->current_position + ROTATOR_SPEED);
			else
				ROTATOR_POSITION_ITEM->number.value = PRIVATE_DATA->current_position = PRIVATE_DATA->target_position;
			indigo_update_property(device, ROTATOR_POSITION_PROPERTY, NULL);
			indigo_reschedule_timer(device, 0.2, &PRIVATE_DATA->rotator_timer);
		} else if (PRIVATE_DATA->current_position > PRIVATE_DATA->target_position){
			ROTATOR_POSITION_PROPERTY->state = INDIGO_BUSY_STATE;
			if (PRIVATE_DATA->current_position - PRIVATE_DATA->target_position > ROTATOR_SPEED)
				ROTATOR_POSITION_ITEM->number.value = PRIVATE_DATA->current_position = (PRIVATE_DATA->current_position - ROTATOR_SPEED);
			else
				ROTATOR_POSITION_ITEM->number.value = PRIVATE_DATA->current_position = PRIVATE_DATA->target_position;
			indigo_update_property(device, ROTATOR_POSITION_PROPERTY, NULL);
			indigo_reschedule_timer(device, 0.2, &PRIVATE_DATA->rotator_timer);
		} else {
			ROTATOR_POSITION_PROPERTY->state = INDIGO_OK_STATE;
			ROTATOR_POSITION_ITEM->number.value = PRIVATE_DATA->current_position;
			indigo_update_property(device, ROTATOR_POSITION_PROPERTY, NULL);
		}
	}
}

static indigo_result rotator_attach(indigo_device *device) {
	assert(device != NULL);
	assert(PRIVATE_DATA != NULL);
	if (indigo_rotator_attach(device, DRIVER_NAME, DRIVER_VERSION) == INDIGO_OK) {
		// --------------------------------------------------------------------------------
		ADDITIONAL_INSTANCES_PROPERTY->hidden = DEVICE_CONTEXT->base_device != NULL;
		INDIGO_DEVICE_ATTACH_LOG(DRIVER_NAME, device->name);
		return indigo_rotator_enumerate_properties(device, NULL, NULL);
	}
	return INDIGO_FAILED;
}

static void rotator_connect_callback(indigo_device *device) {
	CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
	if (!CONNECTION_CONNECTED_ITEM->sw.value) {
		indigo_cancel_timer_sync(device, &PRIVATE_DATA->rotator_timer);
	}
	indigo_rotator_change_property(device, NULL, CONNECTION_PROPERTY);
}

static indigo_result rotator_change_property(indigo_device *device, indigo_client *client, indigo_property *property) {
	assert(device != NULL);
	assert(DEVICE_CONTEXT != NULL);
	assert(property != NULL);
	if (indigo_property_match(CONNECTION_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- CONNECTION
		if (indigo_ignore_connection_change(device, property))
			return INDIGO_OK;
		indigo_property_copy_values(CONNECTION_PROPERTY, property, false);
		CONNECTION_PROPERTY->state = INDIGO_BUSY_STATE;
		indigo_update_property(device, CONNECTION_PROPERTY, NULL);
		indigo_set_timer(device, 0, rotator_connect_callback, NULL);
		return INDIGO_OK;		
	} else if (indigo_property_match(ROTATOR_POSITION_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- ROTATOR_POSITION
		indigo_property_copy_values(ROTATOR_POSITION_PROPERTY, property, false);
		if (ROTATOR_ON_POSITION_SET_SYNC_ITEM->sw.value) {
			ROTATOR_POSITION_PROPERTY->state = INDIGO_OK_STATE;
			PRIVATE_DATA->target_position = ROTATOR_POSITION_ITEM->number.target;
			PRIVATE_DATA->current_position = ROTATOR_POSITION_ITEM->number.value;
			indigo_update_property(device, ROTATOR_POSITION_PROPERTY, NULL);
		} else {
			ROTATOR_POSITION_PROPERTY->state = INDIGO_BUSY_STATE;
			ROTATOR_POSITION_ITEM->number.value = PRIVATE_DATA->current_position;
			PRIVATE_DATA->target_position = ROTATOR_POSITION_ITEM->number.target;
			indigo_update_property(device, ROTATOR_POSITION_PROPERTY, NULL);
			indigo_set_timer(device, 0.2, rotator_timer_callback, &PRIVATE_DATA->rotator_timer);
		}
		return INDIGO_OK;
	} else if (indigo_property_match(ROTATOR_ABORT_MOTION_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- ROTATOR_ABORT_MOTION
		indigo_property_copy_values(ROTATOR_ABORT_MOTION_PROPERTY, property, false);
		if (ROTATOR_ABORT_MOTION_ITEM->sw.value && ROTATOR_POSITION_PROPERTY->state == INDIGO_BUSY_STATE) {
			ROTATOR_POSITION_PROPERTY->state = INDIGO_ALERT_STATE;
			ROTATOR_POSITION_ITEM->number.value = PRIVATE_DATA->current_position;
			indigo_update_property(device, ROTATOR_POSITION_PROPERTY, NULL);
		}
		ROTATOR_ABORT_MOTION_PROPERTY->state = INDIGO_OK_STATE;
		ROTATOR_ABORT_MOTION_ITEM->sw.value = false;
		indigo_update_property(device, ROTATOR_ABORT_MOTION_PROPERTY, NULL);
		return INDIGO_OK;
		// --------------------------------------------------------------------------------
	}
	return indigo_rotator_change_property(device, client, property);
}

static indigo_result rotator_detach(indigo_device *device) {
	assert(device != NULL);
	if (IS_CONNECTED) {
		indigo_set_switch(CONNECTION_PROPERTY, CONNECTION_DISCONNECTED_ITEM, true);
		rotator_connect_callback(device);
	}
	INDIGO_DEVICE_DETACH_LOG(DRIVER_NAME, device->name);
	return indigo_rotator_detach(device);
}

// --------------------------------------------------------------------------------

static simulator_private_data *private_data = NULL;
static indigo_device *imager_focuser = NULL;

indigo_result indigo_rotator_simulator(indigo_driver_action action, indigo_driver_info *info) {
	static indigo_device imager_rotator_template = INDIGO_DEVICE_INITIALIZER(
		SIMULATOR_ROTATOR_NAME,
		rotator_attach,
		indigo_rotator_enumerate_properties,
		rotator_change_property,
		NULL,
		rotator_detach
	);

	static indigo_driver_action last_action = INDIGO_DRIVER_SHUTDOWN;

	SET_DRIVER_INFO(info, "Field Rotator Simulator", __FUNCTION__, DRIVER_VERSION, true, last_action);

	if (action == last_action)
		return INDIGO_OK;

	switch(action) {
		case INDIGO_DRIVER_INIT:
			last_action = action;
			private_data = indigo_safe_malloc(sizeof(simulator_private_data));
			imager_focuser = indigo_safe_malloc_copy(sizeof(indigo_device), &imager_rotator_template);
			imager_focuser->private_data = private_data;
			indigo_attach_device(imager_focuser);
			break;

		case INDIGO_DRIVER_SHUTDOWN:
			VERIFY_NOT_CONNECTED(imager_focuser);
			last_action = action;
			if (imager_focuser != NULL) {
				indigo_detach_device(imager_focuser);
				free(imager_focuser);
				imager_focuser = NULL;
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
