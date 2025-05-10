// Copyright (c) 2025 Rumen G. Bogdanovski
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

// This file generated from indigo_rotator_simulator.driver

// version history
// 3.0 by Rumen G. Bogdanovski

#pragma mark - Includes

#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <assert.h>
#include <pthread.h>
#include <indigo/indigo_driver_xml.h>
#include <indigo/indigo_rotator_driver.h>
#include <indigo/indigo_uni_io.h>

#include "indigo_rotator_simulator.h"

#pragma mark - Common definitions

#define DRIVER_VERSION       0x03000003
#define DRIVER_NAME          "indigo_rotator_simulator"
#define DRIVER_LABEL         "Field Rotator Simulator"
#define ROTATOR_DEVICE_NAME  "Field Rotator Simulator"
#define PRIVATE_DATA         ((simulator_private_data *)device->private_data)

//+ define

#define ROTATOR_SPEED        1

//- define

#pragma mark - Private data definition

typedef struct {
	pthread_mutex_t mutex;
	indigo_timer *rotator_timer;
	indigo_timer *rotator_connection_handler_timer;
	indigo_timer *rotator_position_handler_timer;
	indigo_timer *rotator_abort_motion_handler_timer;
	//+ data
	double target_position, current_position;
	//- data
} simulator_private_data;

#pragma mark - High level code (rotator)

static void rotator_timer_callback(indigo_device *device) {
	if (!IS_CONNECTED) {
		return;
	}
	pthread_mutex_lock(&PRIVATE_DATA->mutex);
	//+ rotator.on_timer
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
			indigo_reschedule_timer(device, 0.1, &PRIVATE_DATA->rotator_timer);
		} else if (PRIVATE_DATA->current_position > PRIVATE_DATA->target_position) {
			ROTATOR_POSITION_PROPERTY->state = INDIGO_BUSY_STATE;
			if (PRIVATE_DATA->current_position - PRIVATE_DATA->target_position > ROTATOR_SPEED)
				ROTATOR_POSITION_ITEM->number.value = PRIVATE_DATA->current_position = (PRIVATE_DATA->current_position - ROTATOR_SPEED);
			else
				ROTATOR_POSITION_ITEM->number.value = PRIVATE_DATA->current_position = PRIVATE_DATA->target_position;
			indigo_update_property(device, ROTATOR_POSITION_PROPERTY, NULL);
			indigo_reschedule_timer(device, 0.1, &PRIVATE_DATA->rotator_timer);
		} else {
			ROTATOR_POSITION_PROPERTY->state = INDIGO_OK_STATE;
			ROTATOR_POSITION_ITEM->number.value = PRIVATE_DATA->current_position;
			indigo_update_property(device, ROTATOR_POSITION_PROPERTY, NULL);
		}
	}
	//- rotator.on_timer
	pthread_mutex_unlock(&PRIVATE_DATA->mutex);
}

static void rotator_connection_handler(indigo_device *device) {
	indigo_lock_master_device(device);
	if (CONNECTION_CONNECTED_ITEM->sw.value) {
		pthread_mutex_lock(&PRIVATE_DATA->mutex);
		indigo_set_timer(device, 0, rotator_timer_callback, &PRIVATE_DATA->rotator_timer);
		CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
		indigo_send_message(device, "Connected to %s", device->name);
		pthread_mutex_unlock(&PRIVATE_DATA->mutex);
	} else {
		indigo_cancel_timer_sync(device, &PRIVATE_DATA->rotator_timer);
		indigo_cancel_timer_sync(device, &PRIVATE_DATA->rotator_position_handler_timer);
		indigo_cancel_timer_sync(device, &PRIVATE_DATA->rotator_abort_motion_handler_timer);
		indigo_send_message(device, "Disconnected from %s", device->name);
		CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
	}
	indigo_rotator_change_property(device, NULL, CONNECTION_PROPERTY);
	indigo_unlock_master_device(device);
}

static void rotator_position_handler(indigo_device *device) {
	pthread_mutex_lock(&PRIVATE_DATA->mutex);
	ROTATOR_POSITION_PROPERTY->state = INDIGO_OK_STATE;
	//+ rotator.ROTATOR_POSITION.on_change
	if (ROTATOR_ON_POSITION_SET_SYNC_ITEM->sw.value) {
		PRIVATE_DATA->target_position = ROTATOR_POSITION_ITEM->number.target;
		PRIVATE_DATA->current_position = ROTATOR_POSITION_ITEM->number.value;
	} else {
		ROTATOR_POSITION_PROPERTY->state = INDIGO_BUSY_STATE;
		ROTATOR_POSITION_ITEM->number.value = PRIVATE_DATA->current_position;
		PRIVATE_DATA->target_position = ROTATOR_POSITION_ITEM->number.target;
		indigo_set_timer(device, 0.1, rotator_timer_callback, &PRIVATE_DATA->rotator_timer);
	}
	//- rotator.ROTATOR_POSITION.on_change
	indigo_update_property(device, ROTATOR_POSITION_PROPERTY, NULL);
	pthread_mutex_unlock(&PRIVATE_DATA->mutex);
}

static void rotator_abort_motion_handler(indigo_device *device) {
	pthread_mutex_lock(&PRIVATE_DATA->mutex);
	ROTATOR_ABORT_MOTION_PROPERTY->state = INDIGO_OK_STATE;
	//+ rotator.ROTATOR_ABORT_MOTION.on_change
	if (ROTATOR_ABORT_MOTION_ITEM->sw.value && ROTATOR_POSITION_PROPERTY->state == INDIGO_BUSY_STATE) {
		ROTATOR_POSITION_PROPERTY->state = INDIGO_ALERT_STATE;
		ROTATOR_POSITION_ITEM->number.value = PRIVATE_DATA->current_position;
		indigo_update_property(device, ROTATOR_POSITION_PROPERTY, NULL);
	}
	ROTATOR_ABORT_MOTION_ITEM->sw.value = false;
	//- rotator.ROTATOR_ABORT_MOTION.on_change
	indigo_update_property(device, ROTATOR_ABORT_MOTION_PROPERTY, NULL);
	pthread_mutex_unlock(&PRIVATE_DATA->mutex);
}

#pragma mark - Device API (rotator)

static indigo_result rotator_enumerate_properties(indigo_device *device, indigo_client *client, indigo_property *property);

static indigo_result rotator_attach(indigo_device *device) {
	if (indigo_rotator_attach(device, DRIVER_NAME, DRIVER_VERSION) == INDIGO_OK) {
		ADDITIONAL_INSTANCES_PROPERTY->hidden = DEVICE_CONTEXT->base_device != NULL;
		ROTATOR_POSITION_PROPERTY->hidden = false;
		ROTATOR_ABORT_MOTION_PROPERTY->hidden = false;
		INDIGO_DEVICE_ATTACH_LOG(DRIVER_NAME, device->name);
		pthread_mutex_init(&PRIVATE_DATA->mutex, NULL);
		return rotator_enumerate_properties(device, NULL, NULL);
	}
	return INDIGO_FAILED;
}

static indigo_result rotator_enumerate_properties(indigo_device *device, indigo_client *client, indigo_property *property) {
	return indigo_rotator_enumerate_properties(device, NULL, NULL);
}

static indigo_result rotator_change_property(indigo_device *device, indigo_client *client, indigo_property *property) {
	if (indigo_property_match_changeable(CONNECTION_PROPERTY, property)) {
		if (!indigo_ignore_connection_change(device, property)) {
			indigo_property_copy_values(CONNECTION_PROPERTY, property, false);
			CONNECTION_PROPERTY->state = INDIGO_BUSY_STATE;
			indigo_update_property(device, CONNECTION_PROPERTY, NULL);
			indigo_set_timer(device, 0, rotator_connection_handler, &PRIVATE_DATA->rotator_connection_handler_timer);
		}
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(ROTATOR_POSITION_PROPERTY, property)) {
		INDIGO_COPY_VALUES_PROCESS_CHANGE(ROTATOR_POSITION_PROPERTY, rotator_position_handler, rotator_position_handler_timer);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(ROTATOR_ABORT_MOTION_PROPERTY, property)) {
		INDIGO_COPY_VALUES_PROCESS_CHANGE(ROTATOR_ABORT_MOTION_PROPERTY, rotator_abort_motion_handler, rotator_abort_motion_handler_timer);
		return INDIGO_OK;
	}
	return indigo_rotator_change_property(device, client, property);
}

static indigo_result rotator_detach(indigo_device *device) {
	if (IS_CONNECTED) {
		indigo_set_switch(CONNECTION_PROPERTY, CONNECTION_DISCONNECTED_ITEM, true);
		rotator_connection_handler(device);
	}
	INDIGO_DEVICE_DETACH_LOG(DRIVER_NAME, device->name);
	pthread_mutex_destroy(&PRIVATE_DATA->mutex);
	return indigo_rotator_detach(device);
}

#pragma mark - Device templates

static indigo_device rotator_template = INDIGO_DEVICE_INITIALIZER(ROTATOR_DEVICE_NAME, rotator_attach, rotator_enumerate_properties, rotator_change_property, NULL, rotator_detach);

#pragma mark - Main code

indigo_result indigo_rotator_simulator(indigo_driver_action action, indigo_driver_info *info) {
	static indigo_driver_action last_action = INDIGO_DRIVER_SHUTDOWN;
	static simulator_private_data *private_data = NULL;
	static indigo_device *rotator = NULL;

	SET_DRIVER_INFO(info, DRIVER_LABEL, __FUNCTION__, DRIVER_VERSION, false, last_action);

	if (action == last_action) {
		return INDIGO_OK;
	}

	switch (action) {
		case INDIGO_DRIVER_INIT:
			last_action = action;
			private_data = indigo_safe_malloc(sizeof(simulator_private_data));
			rotator = indigo_safe_malloc_copy(sizeof(indigo_device), &rotator_template);
			rotator->private_data = private_data;
			indigo_attach_device(rotator);
			break;

		case INDIGO_DRIVER_SHUTDOWN:
			VERIFY_NOT_CONNECTED(rotator);
			last_action = action;
			if (rotator != NULL) {
				indigo_detach_device(rotator);
				free(rotator);
				rotator = NULL;
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
