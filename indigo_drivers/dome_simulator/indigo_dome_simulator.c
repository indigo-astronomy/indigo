// Copyright (c) 2017-2025 CloudMakers, s. r. o.
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

// This file generated from indigo_dome_simulator.driver

#pragma mark - Includes

#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <assert.h>
#include <pthread.h>
#include <indigo/indigo_driver_xml.h>
#include <indigo/indigo_dome_driver.h>
#include <indigo/indigo_uni_io.h>

#include "indigo_dome_simulator.h"

#pragma mark - Common definitions

#define DRIVER_VERSION       0x03000006
#define DRIVER_NAME          "indigo_dome_simulator"
#define DRIVER_LABEL         "Dome Simulator"
#define DOME_DEVICE_NAME     DRIVER_LABEL
#define PRIVATE_DATA         ((simulator_private_data *)device->private_data)

#pragma mark - Private data definition

typedef struct {
	pthread_mutex_t mutex;
	indigo_timer *dome_timer;
	indigo_timer *dome_connection_handler_timer;
	indigo_timer *dome_speed_handler_timer;
	indigo_timer *dome_horizontal_coordinates_handler_timer;
	indigo_timer *dome_slaving_parameters_handler_timer;
	indigo_timer *dome_steps_handler_timer;
	indigo_timer *dome_equatorial_coordinates_handler_timer;
	indigo_timer *dome_abort_motion_handler_timer;
	indigo_timer *dome_shutter_handler_timer;
	indigo_timer *dome_park_handler_timer;
	//+ data
	int target_position, current_position;
	//- data
} simulator_private_data;

#pragma mark - High level code (dome)

static void dome_timer_callback(indigo_device *device) {
	if (!IS_CONNECTED) {
		return;
	}
	pthread_mutex_lock(&PRIVATE_DATA->mutex);
	//+ dome.on_timer
	if (DOME_HORIZONTAL_COORDINATES_PROPERTY->state == INDIGO_ALERT_STATE) {
		DOME_HORIZONTAL_COORDINATES_AZ_ITEM->number.value = PRIVATE_DATA->target_position = PRIVATE_DATA->current_position;
		indigo_update_property(device, DOME_HORIZONTAL_COORDINATES_PROPERTY, NULL);
		DOME_STEPS_PROPERTY->state = INDIGO_ALERT_STATE;
		indigo_update_property(device, DOME_STEPS_PROPERTY, NULL);
	} else {
		if (DOME_DIRECTION_MOVE_CLOCKWISE_ITEM->sw.value && PRIVATE_DATA->current_position != PRIVATE_DATA->target_position) {
			DOME_HORIZONTAL_COORDINATES_PROPERTY->state = INDIGO_BUSY_STATE;
			int dif = (int)(PRIVATE_DATA->target_position - PRIVATE_DATA->current_position + 360) % 360;
			if (dif > DOME_SPEED_ITEM->number.value) {
				DOME_HORIZONTAL_COORDINATES_AZ_ITEM->number.value = PRIVATE_DATA->current_position = (int)(PRIVATE_DATA->current_position + DOME_SPEED_ITEM->number.value + 360) % 360;
			} else {
				DOME_HORIZONTAL_COORDINATES_AZ_ITEM->number.value = PRIVATE_DATA->current_position = PRIVATE_DATA->target_position;
			}
			indigo_update_property(device, DOME_HORIZONTAL_COORDINATES_PROPERTY, NULL);
			DOME_STEPS_PROPERTY->state = INDIGO_BUSY_STATE;
			indigo_update_property(device, DOME_STEPS_PROPERTY, NULL);
			indigo_set_timer(device, 0.1, dome_timer_callback, NULL);
		} else if (DOME_DIRECTION_MOVE_COUNTERCLOCKWISE_ITEM->sw.value && PRIVATE_DATA->current_position != PRIVATE_DATA->target_position) {
			DOME_HORIZONTAL_COORDINATES_PROPERTY->state = INDIGO_BUSY_STATE;
			int dif = (int)(PRIVATE_DATA->current_position - PRIVATE_DATA->target_position + 360) % 360;
			if (dif > DOME_SPEED_ITEM->number.value) {
				DOME_HORIZONTAL_COORDINATES_AZ_ITEM->number.value = PRIVATE_DATA->current_position = (int)(PRIVATE_DATA->current_position - DOME_SPEED_ITEM->number.value + 360) % 360;
			} else {
				DOME_HORIZONTAL_COORDINATES_AZ_ITEM->number.value = PRIVATE_DATA->current_position = PRIVATE_DATA->target_position;
			}
			indigo_update_property(device, DOME_HORIZONTAL_COORDINATES_PROPERTY, NULL);
			DOME_STEPS_PROPERTY->state = INDIGO_BUSY_STATE;
			indigo_update_property(device, DOME_STEPS_PROPERTY, NULL);
			indigo_set_timer(device, 0.1, dome_timer_callback, NULL);
		} else {
			DOME_HORIZONTAL_COORDINATES_PROPERTY->state = INDIGO_OK_STATE;
			DOME_HORIZONTAL_COORDINATES_AZ_ITEM->number.value = PRIVATE_DATA->current_position;
			indigo_update_property(device, DOME_HORIZONTAL_COORDINATES_PROPERTY, NULL);
			DOME_STEPS_PROPERTY->state = INDIGO_OK_STATE;
			indigo_update_property(device, DOME_STEPS_PROPERTY, NULL);
			if (DOME_PARK_PROPERTY->state == INDIGO_BUSY_STATE) {
				DOME_PARK_PROPERTY->state = INDIGO_OK_STATE;
				indigo_update_property(device, DOME_PARK_PROPERTY, "Parked");
			}
		}
	}
	//- dome.on_timer
	pthread_mutex_unlock(&PRIVATE_DATA->mutex);
}

static void dome_connection_handler(indigo_device *device) {
	indigo_lock_master_device(device);
	if (CONNECTION_CONNECTED_ITEM->sw.value) {
		pthread_mutex_lock(&PRIVATE_DATA->mutex);
		indigo_set_timer(device, 0, dome_timer_callback, &PRIVATE_DATA->dome_timer);
		CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
		indigo_send_message(device, "Connected to %s", device->name);
		pthread_mutex_unlock(&PRIVATE_DATA->mutex);
	} else {
		indigo_cancel_timer_sync(device, &PRIVATE_DATA->dome_timer);
		indigo_cancel_timer_sync(device, &PRIVATE_DATA->dome_speed_handler_timer);
		indigo_cancel_timer_sync(device, &PRIVATE_DATA->dome_horizontal_coordinates_handler_timer);
		indigo_cancel_timer_sync(device, &PRIVATE_DATA->dome_slaving_parameters_handler_timer);
		indigo_cancel_timer_sync(device, &PRIVATE_DATA->dome_steps_handler_timer);
		indigo_cancel_timer_sync(device, &PRIVATE_DATA->dome_equatorial_coordinates_handler_timer);
		indigo_cancel_timer_sync(device, &PRIVATE_DATA->dome_abort_motion_handler_timer);
		indigo_cancel_timer_sync(device, &PRIVATE_DATA->dome_shutter_handler_timer);
		indigo_cancel_timer_sync(device, &PRIVATE_DATA->dome_park_handler_timer);
		indigo_send_message(device, "Disconnected from %s", device->name);
		CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
	}
	indigo_dome_change_property(device, NULL, CONNECTION_PROPERTY);
	indigo_unlock_master_device(device);
}

static void dome_speed_handler(indigo_device *device) {
	pthread_mutex_lock(&PRIVATE_DATA->mutex);
	DOME_SPEED_PROPERTY->state = INDIGO_OK_STATE;
	indigo_update_property(device, DOME_SPEED_PROPERTY, NULL);
	pthread_mutex_unlock(&PRIVATE_DATA->mutex);
}

static void dome_horizontal_coordinates_handler(indigo_device *device) {
	pthread_mutex_lock(&PRIVATE_DATA->mutex);
	DOME_HORIZONTAL_COORDINATES_PROPERTY->state = INDIGO_OK_STATE;
	//+ dome.DOME_HORIZONTAL_COORDINATES.on_change
	if (DOME_PARK_PARKED_ITEM->sw.value) {
		DOME_HORIZONTAL_COORDINATES_PROPERTY->state = INDIGO_ALERT_STATE;
		indigo_send_message(device, "Dome is parked");
	} else {
		PRIVATE_DATA->target_position = (int)DOME_HORIZONTAL_COORDINATES_AZ_ITEM->number.target;
		int dif = (int)(PRIVATE_DATA->target_position - PRIVATE_DATA->current_position + 360) % 360;
		if (dif < 180) {
			indigo_set_switch(DOME_DIRECTION_PROPERTY, DOME_DIRECTION_MOVE_CLOCKWISE_ITEM, true);
			DOME_STEPS_ITEM->number.value = dif;
		} else if (dif > 180) {
			indigo_set_switch(DOME_DIRECTION_PROPERTY, DOME_DIRECTION_MOVE_COUNTERCLOCKWISE_ITEM, true);
			DOME_STEPS_ITEM->number.value = 360 - dif;
		}
		DOME_DIRECTION_PROPERTY->state = INDIGO_OK_STATE;
		indigo_update_property(device, DOME_DIRECTION_PROPERTY, NULL);
		DOME_STEPS_PROPERTY->state = INDIGO_BUSY_STATE;
		indigo_update_property(device, DOME_STEPS_PROPERTY, NULL);
		DOME_HORIZONTAL_COORDINATES_PROPERTY->state = INDIGO_BUSY_STATE;
		indigo_update_property(device, DOME_HORIZONTAL_COORDINATES_PROPERTY, NULL);
		DOME_EQUATORIAL_COORDINATES_PROPERTY->state = INDIGO_OK_STATE;
		indigo_update_property(device, DOME_EQUATORIAL_COORDINATES_PROPERTY, NULL);
		indigo_set_timer(device, 0.5, dome_timer_callback, NULL);
	}
	//- dome.DOME_HORIZONTAL_COORDINATES.on_change
	indigo_update_property(device, DOME_HORIZONTAL_COORDINATES_PROPERTY, NULL);
	pthread_mutex_unlock(&PRIVATE_DATA->mutex);
}

static void dome_slaving_parameters_handler(indigo_device *device) {
	pthread_mutex_lock(&PRIVATE_DATA->mutex);
	DOME_SLAVING_PARAMETERS_PROPERTY->state = INDIGO_OK_STATE;
	indigo_update_property(device, DOME_SLAVING_PARAMETERS_PROPERTY, NULL);
	pthread_mutex_unlock(&PRIVATE_DATA->mutex);
}

static void dome_steps_handler(indigo_device *device) {
	pthread_mutex_lock(&PRIVATE_DATA->mutex);
	DOME_STEPS_PROPERTY->state = INDIGO_OK_STATE;
	//+ dome.DOME_STEPS.on_change
	if (DOME_PARK_PARKED_ITEM->sw.value) {
		DOME_STEPS_PROPERTY->state = INDIGO_ALERT_STATE;
		indigo_send_message(device, "Dome is parked");
	} else {
		DOME_STEPS_ITEM->number.value = (int)DOME_STEPS_ITEM->number.value;
		if (DOME_DIRECTION_MOVE_COUNTERCLOCKWISE_ITEM->sw.value) {
			PRIVATE_DATA->target_position = (int)(PRIVATE_DATA->current_position - DOME_STEPS_ITEM->number.value + 360) % 360;
		} else if (DOME_DIRECTION_MOVE_CLOCKWISE_ITEM->sw.value) {
			PRIVATE_DATA->target_position = (int)(PRIVATE_DATA->current_position + DOME_STEPS_ITEM->number.value + 360) % 360;
		}
		DOME_HORIZONTAL_COORDINATES_PROPERTY->state = INDIGO_BUSY_STATE;
		DOME_HORIZONTAL_COORDINATES_AZ_ITEM->number.value = PRIVATE_DATA->current_position;
		indigo_update_property(device, DOME_HORIZONTAL_COORDINATES_PROPERTY, NULL);
		DOME_STEPS_PROPERTY->state = INDIGO_BUSY_STATE;
		indigo_set_timer(device, 0.5, dome_timer_callback, NULL);
	}
	//- dome.DOME_STEPS.on_change
	indigo_update_property(device, DOME_STEPS_PROPERTY, NULL);
	pthread_mutex_unlock(&PRIVATE_DATA->mutex);
}

static void dome_equatorial_coordinates_handler(indigo_device *device) {
	pthread_mutex_lock(&PRIVATE_DATA->mutex);
	DOME_EQUATORIAL_COORDINATES_PROPERTY->state = INDIGO_OK_STATE;
	//+ dome.DOME_EQUATORIAL_COORDINATES.on_change
	double az;
	if ((DOME_SLAVING_ENABLE_ITEM->sw.value) && indigo_fix_dome_azimuth(device, DOME_EQUATORIAL_COORDINATES_RA_ITEM->number.value, DOME_EQUATORIAL_COORDINATES_DEC_ITEM->number.value, DOME_HORIZONTAL_COORDINATES_AZ_ITEM->number.value, &az)) {
		if (DOME_PARK_PARKED_ITEM->sw.value) {
			if (DOME_EQUATORIAL_COORDINATES_PROPERTY->state != INDIGO_ALERT_STATE) {
				DOME_EQUATORIAL_COORDINATES_PROPERTY->state = INDIGO_ALERT_STATE;
				indigo_send_message(device, "Can not Synchronize. Dome is parked.");
			}
		} else {
			PRIVATE_DATA->target_position = (int)(DOME_HORIZONTAL_COORDINATES_AZ_ITEM->number.target = az);
			int dif = (int)(PRIVATE_DATA->target_position - PRIVATE_DATA->current_position + 360) % 360;
			if (dif < 180) {
				indigo_set_switch(DOME_DIRECTION_PROPERTY, DOME_DIRECTION_MOVE_CLOCKWISE_ITEM, true);
				DOME_STEPS_ITEM->number.value = dif;
			} else if (dif > 180) {
				indigo_set_switch(DOME_DIRECTION_PROPERTY, DOME_DIRECTION_MOVE_COUNTERCLOCKWISE_ITEM, true);
				DOME_STEPS_ITEM->number.value = 360 - dif;
			}
			DOME_DIRECTION_PROPERTY->state = INDIGO_OK_STATE;
			indigo_update_property(device, DOME_DIRECTION_PROPERTY, NULL);
			DOME_STEPS_PROPERTY->state = INDIGO_BUSY_STATE;
			indigo_update_property(device, DOME_STEPS_PROPERTY, NULL);
			DOME_HORIZONTAL_COORDINATES_PROPERTY->state = INDIGO_BUSY_STATE;
			indigo_update_property(device, DOME_HORIZONTAL_COORDINATES_PROPERTY, NULL);
			indigo_set_timer(device, 0.5, dome_timer_callback, NULL);
		}
	}
	//- dome.DOME_EQUATORIAL_COORDINATES.on_change
	indigo_update_property(device, DOME_EQUATORIAL_COORDINATES_PROPERTY, NULL);
	pthread_mutex_unlock(&PRIVATE_DATA->mutex);
}

static void dome_abort_motion_handler(indigo_device *device) {
	pthread_mutex_lock(&PRIVATE_DATA->mutex);
	DOME_ABORT_MOTION_PROPERTY->state = INDIGO_OK_STATE;
	//+ dome.DOME_ABORT_MOTION.on_change
	if (DOME_ABORT_MOTION_ITEM->sw.value && DOME_HORIZONTAL_COORDINATES_PROPERTY->state == INDIGO_BUSY_STATE) {
		DOME_HORIZONTAL_COORDINATES_PROPERTY->state = INDIGO_ALERT_STATE;
		DOME_HORIZONTAL_COORDINATES_AZ_ITEM->number.value = PRIVATE_DATA->current_position;
		indigo_update_property(device, DOME_HORIZONTAL_COORDINATES_PROPERTY, NULL);
	}
	DOME_ABORT_MOTION_ITEM->sw.value = false;
	//- dome.DOME_ABORT_MOTION.on_change
	indigo_update_property(device, DOME_ABORT_MOTION_PROPERTY, NULL);
	pthread_mutex_unlock(&PRIVATE_DATA->mutex);
}

static void dome_shutter_handler(indigo_device *device) {
	pthread_mutex_lock(&PRIVATE_DATA->mutex);
	//+ dome.DOME_SHUTTER.on_change
	DOME_SHUTTER_PROPERTY->state = INDIGO_BUSY_STATE;
	indigo_usleep(INDIGO_DELAY(6));
	DOME_SHUTTER_PROPERTY->state = INDIGO_OK_STATE;
	//- dome.DOME_SHUTTER.on_change
	indigo_update_property(device, DOME_SHUTTER_PROPERTY, NULL);
	pthread_mutex_unlock(&PRIVATE_DATA->mutex);
}

static void dome_park_handler(indigo_device *device) {
	pthread_mutex_lock(&PRIVATE_DATA->mutex);
	DOME_PARK_PROPERTY->state = INDIGO_OK_STATE;
	//+ dome.DOME_PARK.on_change
	if (DOME_PARK_PARKED_ITEM->sw.value) {
		DOME_PARK_PROPERTY->state = INDIGO_BUSY_STATE;
		if (PRIVATE_DATA->current_position > 180) {
			DOME_DIRECTION_PROPERTY->state = INDIGO_OK_STATE;
			indigo_set_switch(DOME_DIRECTION_PROPERTY, DOME_DIRECTION_MOVE_CLOCKWISE_ITEM, true);
			DOME_STEPS_ITEM->number.value = 360 - PRIVATE_DATA->current_position;
		} else if (PRIVATE_DATA->current_position < 180) {
			DOME_DIRECTION_PROPERTY->state = INDIGO_OK_STATE;
			indigo_set_switch(DOME_DIRECTION_PROPERTY, DOME_DIRECTION_MOVE_COUNTERCLOCKWISE_ITEM, true);
			DOME_STEPS_ITEM->number.value = PRIVATE_DATA->current_position;
		}
		PRIVATE_DATA->target_position = 0;
		DOME_DIRECTION_PROPERTY->state = INDIGO_OK_STATE;
		indigo_update_property(device, DOME_DIRECTION_PROPERTY, NULL);
		DOME_STEPS_PROPERTY->state = INDIGO_BUSY_STATE;
		indigo_update_property(device, DOME_STEPS_PROPERTY, NULL);
		DOME_HORIZONTAL_COORDINATES_PROPERTY->state = INDIGO_BUSY_STATE;
		indigo_update_property(device, DOME_HORIZONTAL_COORDINATES_PROPERTY, NULL);
		indigo_set_timer(device, 0.5, dome_timer_callback, NULL);
	}
	//- dome.DOME_PARK.on_change
	indigo_update_property(device, DOME_PARK_PROPERTY, NULL);
	pthread_mutex_unlock(&PRIVATE_DATA->mutex);
}

#pragma mark - Device API (dome)

static indigo_result dome_enumerate_properties(indigo_device *device, indigo_client *client, indigo_property *property);

static indigo_result dome_attach(indigo_device *device) {
	if (indigo_dome_attach(device, DRIVER_NAME, DRIVER_VERSION) == INDIGO_OK) {
		ADDITIONAL_INSTANCES_PROPERTY->hidden = DEVICE_CONTEXT->base_device != NULL;
		DOME_SPEED_PROPERTY->hidden = false;
		//+ dome.DOME_SPEED.on_attach
		DOME_SPEED_ITEM->number.value = 1;
		//- dome.DOME_SPEED.on_attach
		DOME_HORIZONTAL_COORDINATES_PROPERTY->hidden = false;
		//+ dome.DOME_HORIZONTAL_COORDINATES.on_attach
		DOME_HORIZONTAL_COORDINATES_PROPERTY->perm = INDIGO_RW_PERM;
		//- dome.DOME_HORIZONTAL_COORDINATES.on_attach
		DOME_SLAVING_PARAMETERS_PROPERTY->hidden = false;
		DOME_STEPS_PROPERTY->hidden = false;
		DOME_EQUATORIAL_COORDINATES_PROPERTY->hidden = false;
		DOME_ABORT_MOTION_PROPERTY->hidden = false;
		DOME_SHUTTER_PROPERTY->hidden = false;
		DOME_PARK_PROPERTY->hidden = false;
		INDIGO_DEVICE_ATTACH_LOG(DRIVER_NAME, device->name);
		pthread_mutex_init(&PRIVATE_DATA->mutex, NULL);
		return dome_enumerate_properties(device, NULL, NULL);
	}
	return INDIGO_FAILED;
}

static indigo_result dome_enumerate_properties(indigo_device *device, indigo_client *client, indigo_property *property) {
	return indigo_dome_enumerate_properties(device, NULL, NULL);
}

static indigo_result dome_change_property(indigo_device *device, indigo_client *client, indigo_property *property) {
	if (indigo_property_match_changeable(CONNECTION_PROPERTY, property)) {
		if (!indigo_ignore_connection_change(device, property)) {
			indigo_property_copy_values(CONNECTION_PROPERTY, property, false);
			CONNECTION_PROPERTY->state = INDIGO_BUSY_STATE;
			indigo_update_property(device, CONNECTION_PROPERTY, NULL);
			indigo_set_timer(device, 0, dome_connection_handler, &PRIVATE_DATA->dome_connection_handler_timer);
		}
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(DOME_SPEED_PROPERTY, property)) {
		INDIGO_COPY_VALUES_PROCESS_CHANGE(DOME_SPEED_PROPERTY, dome_speed_handler, dome_speed_handler_timer);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(DOME_HORIZONTAL_COORDINATES_PROPERTY, property)) {
		INDIGO_COPY_TARGETS_PROCESS_CHANGE(DOME_HORIZONTAL_COORDINATES_PROPERTY, dome_horizontal_coordinates_handler, dome_horizontal_coordinates_handler_timer);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(DOME_SLAVING_PARAMETERS_PROPERTY, property)) {
		INDIGO_COPY_VALUES_PROCESS_CHANGE(DOME_SLAVING_PARAMETERS_PROPERTY, dome_slaving_parameters_handler, dome_slaving_parameters_handler_timer);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(DOME_STEPS_PROPERTY, property)) {
		INDIGO_COPY_VALUES_PROCESS_CHANGE(DOME_STEPS_PROPERTY, dome_steps_handler, dome_steps_handler_timer);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(DOME_EQUATORIAL_COORDINATES_PROPERTY, property)) {
		INDIGO_COPY_VALUES_PROCESS_CHANGE(DOME_EQUATORIAL_COORDINATES_PROPERTY, dome_equatorial_coordinates_handler, dome_equatorial_coordinates_handler_timer);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(DOME_ABORT_MOTION_PROPERTY, property)) {
		INDIGO_COPY_VALUES_PROCESS_CHANGE(DOME_ABORT_MOTION_PROPERTY, dome_abort_motion_handler, dome_abort_motion_handler_timer);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(DOME_SHUTTER_PROPERTY, property)) {
		INDIGO_COPY_VALUES_PROCESS_CHANGE(DOME_SHUTTER_PROPERTY, dome_shutter_handler, dome_shutter_handler_timer);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(DOME_PARK_PROPERTY, property)) {
		INDIGO_COPY_VALUES_PROCESS_CHANGE(DOME_PARK_PROPERTY, dome_park_handler, dome_park_handler_timer);
		return INDIGO_OK;
	}
	return indigo_dome_change_property(device, client, property);
}

static indigo_result dome_detach(indigo_device *device) {
	if (IS_CONNECTED) {
		indigo_set_switch(CONNECTION_PROPERTY, CONNECTION_DISCONNECTED_ITEM, true);
		dome_connection_handler(device);
	}
	INDIGO_DEVICE_DETACH_LOG(DRIVER_NAME, device->name);
	pthread_mutex_destroy(&PRIVATE_DATA->mutex);
	return indigo_dome_detach(device);
}

#pragma mark - Device templates

static indigo_device dome_template = INDIGO_DEVICE_INITIALIZER(DOME_DEVICE_NAME, dome_attach, dome_enumerate_properties, dome_change_property, NULL, dome_detach);

#pragma mark - Main code

indigo_result indigo_dome_simulator(indigo_driver_action action, indigo_driver_info *info) {
	static indigo_driver_action last_action = INDIGO_DRIVER_SHUTDOWN;
	static simulator_private_data *private_data = NULL;
	static indigo_device *dome = NULL;

	SET_DRIVER_INFO(info, DRIVER_LABEL, __FUNCTION__, DRIVER_VERSION, false, last_action);

	if (action == last_action) {
		return INDIGO_OK;
	}

	switch (action) {
		case INDIGO_DRIVER_INIT:
			last_action = action;
			private_data = indigo_safe_malloc(sizeof(simulator_private_data));
			dome = indigo_safe_malloc_copy(sizeof(indigo_device), &dome_template);
			dome->private_data = private_data;
			indigo_attach_device(dome);
			break;

		case INDIGO_DRIVER_SHUTDOWN:
			VERIFY_NOT_CONNECTED(dome);
			last_action = action;
			if (dome != NULL) {
				indigo_detach_device(dome);
				free(dome);
				dome = NULL;
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
