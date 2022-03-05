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

/** INDIGO MOUNT Simulator driver
 \file indigo_mount_simulator.c
 */

#define DRIVER_VERSION 0x0007
#define DRIVER_NAME "indigo_mount_simulator"

#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <math.h>
#include <assert.h>

#include <indigo/indigo_driver_xml.h>
#include <indigo/indigo_novas.h>

#include "indigo_mount_simulator.h"

#define PRIVATE_DATA        ((simulator_private_data *)device->private_data)

typedef struct {
	bool parking, parked, going_home, at_home;
	indigo_timer *position_timer, *move_timer, *guider_timer;
	double ha;
	bool slew_in_progress;
	pthread_mutex_t position_mutex;
} simulator_private_data;

	// -------------------------------------------------------------------------------- INDIGO MOUNT device implementation

static void position_timer_callback(indigo_device *device) {
	pthread_mutex_lock(&PRIVATE_DATA->position_mutex);
	if (IS_CONNECTED) {
		double diffRA = MOUNT_RAW_COORDINATES_RA_ITEM->number.target - MOUNT_RAW_COORDINATES_RA_ITEM->number.value;
		if (diffRA > 12)
			diffRA = -(24 - diffRA);
		else if (diffRA < -12) {
			diffRA = (24 - diffRA);
		}
		double diffDec = MOUNT_RAW_COORDINATES_DEC_ITEM->number.target - MOUNT_RAW_COORDINATES_DEC_ITEM->number.value;
		if (PRIVATE_DATA->slew_in_progress) {
			if (diffRA == 0 && diffDec == 0) {
				MOUNT_EQUATORIAL_COORDINATES_PROPERTY->state = MOUNT_RAW_COORDINATES_PROPERTY->state = INDIGO_OK_STATE;
				if (MOUNT_TRACKING_OFF_ITEM->sw.value) {
					PRIVATE_DATA->ha = indigo_lst(NULL, MOUNT_GEOGRAPHIC_COORDINATES_LONGITUDE_ITEM->number.value) - MOUNT_RAW_COORDINATES_RA_ITEM->number.value;
				}
				PRIVATE_DATA->slew_in_progress = false;
				if (PRIVATE_DATA->parking) {
					PRIVATE_DATA->parking = false;
					PRIVATE_DATA->parked = true;
					indigo_set_switch(MOUNT_TRACKING_PROPERTY, MOUNT_TRACKING_OFF_ITEM, true);
					MOUNT_TRACKING_PROPERTY->state = INDIGO_OK_STATE;
					indigo_update_property(device, MOUNT_TRACKING_PROPERTY, NULL);
					MOUNT_PARK_PROPERTY->state = INDIGO_OK_STATE;
					indigo_update_property(device, MOUNT_PARK_PROPERTY, "Parked");
				}
				if (PRIVATE_DATA->going_home) {
					PRIVATE_DATA->going_home = false;
					indigo_set_switch(MOUNT_TRACKING_PROPERTY, MOUNT_TRACKING_OFF_ITEM, true);
					indigo_update_property(device, MOUNT_TRACKING_PROPERTY, NULL);
					MOUNT_HOME_PROPERTY->state = INDIGO_OK_STATE;
					indigo_update_property(device, MOUNT_HOME_PROPERTY, NULL);
				}
			} else {
				double speedRA = 0.2;
				double speedDec = 1.5;
				if (fabs(diffRA) < speedRA) {
					MOUNT_RAW_COORDINATES_RA_ITEM->number.value = MOUNT_RAW_COORDINATES_RA_ITEM->number.target;
				} else if (diffRA > 0) {
					MOUNT_RAW_COORDINATES_RA_ITEM->number.value += speedRA;
					if (MOUNT_RAW_COORDINATES_RA_ITEM->number.value > 24)
						MOUNT_RAW_COORDINATES_RA_ITEM->number.value -= 24;
				} else if (diffRA < 0) {
					MOUNT_RAW_COORDINATES_RA_ITEM->number.value -= speedRA;
					if (MOUNT_RAW_COORDINATES_RA_ITEM->number.value < 0)
						MOUNT_RAW_COORDINATES_RA_ITEM->number.value += 24;
				}
				if (fabs(diffDec) < speedDec)
					MOUNT_RAW_COORDINATES_DEC_ITEM->number.value = MOUNT_RAW_COORDINATES_DEC_ITEM->number.target;
				else if (diffDec > 0)
					MOUNT_RAW_COORDINATES_DEC_ITEM->number.value += speedDec;
				else if (diffDec < 0)
					MOUNT_RAW_COORDINATES_DEC_ITEM->number.value -= speedDec;
				MOUNT_EQUATORIAL_COORDINATES_PROPERTY->state = INDIGO_BUSY_STATE;
			}
			indigo_reschedule_timer(device, 0.2, &PRIVATE_DATA->position_timer);
		} else {
			if (PRIVATE_DATA->parked || (MOUNT_EQUATORIAL_COORDINATES_PROPERTY->state == INDIGO_OK_STATE && MOUNT_TRACKING_OFF_ITEM->sw.value)) {
				MOUNT_RAW_COORDINATES_RA_ITEM->number.value = fmod(indigo_lst(NULL, MOUNT_GEOGRAPHIC_COORDINATES_LONGITUDE_ITEM->number.value) - PRIVATE_DATA->ha + 24, 24);
			}
			indigo_reschedule_timer(device, 1.0, &PRIVATE_DATA->position_timer);
		}
		indigo_raw_to_translated(device, MOUNT_RAW_COORDINATES_RA_ITEM->number.value, MOUNT_RAW_COORDINATES_DEC_ITEM->number.value, &MOUNT_EQUATORIAL_COORDINATES_RA_ITEM->number.value, &MOUNT_EQUATORIAL_COORDINATES_DEC_ITEM->number.value);
		indigo_update_coordinates(device, NULL);
		indigo_update_property(device, MOUNT_RAW_COORDINATES_PROPERTY, NULL);
	}
	pthread_mutex_unlock(&PRIVATE_DATA->position_mutex);
}

static void move_timer_callback(indigo_device *device) {
	pthread_mutex_lock(&PRIVATE_DATA->position_mutex);
	double speed = 0;
	if (MOUNT_SLEW_RATE_GUIDE_ITEM->sw.value)
		speed = 0.01;
	else if (MOUNT_SLEW_RATE_CENTERING_ITEM->sw.value)
		speed = 0.025;
	else if (MOUNT_SLEW_RATE_FIND_ITEM->sw.value)
		speed = 0.1;
	else if (MOUNT_SLEW_RATE_MAX_ITEM->sw.value)
		speed = 0.5;
	double decStep = 0;
	if (MOUNT_MOTION_NORTH_ITEM->sw.value)
		decStep = speed * 15;
	else if (MOUNT_MOTION_SOUTH_ITEM->sw.value)
		decStep = -speed * 15;
	double raStep = 0;
	if (MOUNT_MOTION_WEST_ITEM->sw.value)
		raStep = speed;
	else if (MOUNT_MOTION_EAST_ITEM->sw.value)
		raStep = -speed;
	if (raStep == 0 && decStep == 0) {
		MOUNT_EQUATORIAL_COORDINATES_PROPERTY->state = MOUNT_RAW_COORDINATES_PROPERTY->state = INDIGO_OK_STATE;
		PRIVATE_DATA->move_timer = NULL;
	} else {
		MOUNT_RAW_COORDINATES_RA_ITEM->number.target = MOUNT_RAW_COORDINATES_RA_ITEM->number.value = fmod(MOUNT_RAW_COORDINATES_RA_ITEM->number.value + raStep * speed + 24, 24);
		MOUNT_RAW_COORDINATES_DEC_ITEM->number.target = MOUNT_RAW_COORDINATES_DEC_ITEM->number.value = fmod(MOUNT_RAW_COORDINATES_DEC_ITEM->number.value + decStep * speed + 360 + 180, 360) - 180;
		MOUNT_EQUATORIAL_COORDINATES_PROPERTY->state = INDIGO_BUSY_STATE;
		indigo_reschedule_timer(device, 0.5, &PRIVATE_DATA->move_timer);
	}
	indigo_raw_to_translated(device, MOUNT_RAW_COORDINATES_RA_ITEM->number.value, MOUNT_RAW_COORDINATES_DEC_ITEM->number.value, &MOUNT_EQUATORIAL_COORDINATES_RA_ITEM->number.value, &MOUNT_EQUATORIAL_COORDINATES_DEC_ITEM->number.value);
	indigo_update_coordinates(device, NULL);
	indigo_update_property(device, MOUNT_RAW_COORDINATES_PROPERTY, NULL);
	pthread_mutex_unlock(&PRIVATE_DATA->position_mutex);
}

static indigo_result mount_attach(indigo_device *device) {
	assert(device != NULL);
	assert(PRIVATE_DATA != NULL);
	if (indigo_mount_attach(device, DRIVER_NAME, DRIVER_VERSION) == INDIGO_OK) {
		// -------------------------------------------------------------------------------- SIMULATION
		SIMULATION_PROPERTY->hidden = false;
		SIMULATION_PROPERTY->perm = INDIGO_RO_PERM;
		SIMULATION_ENABLED_ITEM->sw.value = true;
		SIMULATION_DISABLED_ITEM->sw.value = false;
		// -------------------------------------------------------------------------------- DEVICE_PORT
		DEVICE_PORT_PROPERTY->hidden = true;
		// -------------------------------------------------------------------------------- MOUNT_PARK_SET
		MOUNT_PARK_SET_PROPERTY->hidden = false;
		// -------------------------------------------------------------------------------- MOUNT_PARK_POSITION
		MOUNT_PARK_POSITION_PROPERTY->hidden = false;
		// -------------------------------------------------------------------------------- MOUNT_PARK
		PRIVATE_DATA->parked = true;
		// -------------------------------------------------------------------------------- MOUNT_HOME_SET
		MOUNT_HOME_SET_PROPERTY->hidden = false;
		// -------------------------------------------------------------------------------- MOUNT_HOME_POSITION
		MOUNT_HOME_POSITION_PROPERTY->hidden = false;
		// -------------------------------------------------------------------------------- MOUNT_HOME
		MOUNT_HOME_PROPERTY->hidden = false;
		// -------------------------------------------------------------------------------- MOUNT_ON_COORDINATES_SET
		MOUNT_ON_COORDINATES_SET_PROPERTY->count = 2;
		// -------------------------------------------------------------------------------- MOUNT_EQUATORIAL_COORDINATES
		MOUNT_RAW_COORDINATES_RA_ITEM->number.value = MOUNT_RAW_COORDINATES_RA_ITEM->number.target = 0;
		MOUNT_RAW_COORDINATES_DEC_ITEM->number.value = MOUNT_RAW_COORDINATES_DEC_ITEM->number.target = 90;
		MOUNT_EQUATORIAL_COORDINATES_RA_ITEM->number.value = MOUNT_EQUATORIAL_COORDINATES_RA_ITEM->number.target = MOUNT_RAW_COORDINATES_RA_ITEM->number.value;
		MOUNT_EQUATORIAL_COORDINATES_DEC_ITEM->number.value = MOUNT_EQUATORIAL_COORDINATES_DEC_ITEM->number.target = MOUNT_RAW_COORDINATES_DEC_ITEM->number.value;
		// -------------------------------------------------------------------------------- MOUNT_ALIGNMENT_MODE
		MOUNT_ALIGNMENT_MODE_PROPERTY->hidden = false;
		// -------------------------------------------------------------------------------- MOUNT_TRACK_RATE
		MOUNT_TRACK_RATE_PROPERTY->count = 5;
		indigo_set_switch(MOUNT_TRACKING_PROPERTY, MOUNT_TRACKING_OFF_ITEM, true);
		// -------------------------------------------------------------------------------- AUTHENTICATION
		AUTHENTICATION_PROPERTY->hidden = false;
		AUTHENTICATION_PROPERTY->count = 1;
		// --------------------------------------------------------------------------------
		ADDITIONAL_INSTANCES_PROPERTY->hidden = DEVICE_CONTEXT->base_device != NULL;
		INDIGO_DEVICE_ATTACH_LOG(DRIVER_NAME, device->name);
		return indigo_mount_enumerate_properties(device, NULL, NULL);
	}
	return INDIGO_FAILED;
}

static void mount_connect_callback(indigo_device *device) {
	if (CONNECTION_CONNECTED_ITEM->sw.value) {
		if (MOUNT_PARK_PARKED_ITEM->sw.value) {
			MOUNT_EQUATORIAL_COORDINATES_RA_ITEM->number.value = MOUNT_EQUATORIAL_COORDINATES_RA_ITEM->number.target = fmod(indigo_lst(NULL, MOUNT_GEOGRAPHIC_COORDINATES_LONGITUDE_ITEM->number.value) - (PRIVATE_DATA->ha = MOUNT_PARK_POSITION_HA_ITEM->number.value) + 24, 24);
			MOUNT_RAW_COORDINATES_DEC_ITEM->number.value = MOUNT_RAW_COORDINATES_DEC_ITEM->number.target = MOUNT_PARK_POSITION_DEC_ITEM->number.value;
			indigo_translated_to_raw(device, MOUNT_EQUATORIAL_COORDINATES_RA_ITEM->number.target, MOUNT_EQUATORIAL_COORDINATES_DEC_ITEM->number.target, &MOUNT_RAW_COORDINATES_RA_ITEM->number.target, &MOUNT_RAW_COORDINATES_DEC_ITEM->number.target);
			MOUNT_RAW_COORDINATES_RA_ITEM->number.value = MOUNT_RAW_COORDINATES_RA_ITEM->number.target;
			MOUNT_RAW_COORDINATES_DEC_ITEM->number.value = MOUNT_RAW_COORDINATES_DEC_ITEM->number.target;
		}
		indigo_raw_to_translated(device, MOUNT_RAW_COORDINATES_RA_ITEM->number.value, MOUNT_RAW_COORDINATES_DEC_ITEM->number.value, &MOUNT_EQUATORIAL_COORDINATES_RA_ITEM->number.value, &MOUNT_EQUATORIAL_COORDINATES_DEC_ITEM->number.value);
		indigo_raw_to_translated(device, MOUNT_RAW_COORDINATES_RA_ITEM->number.target, MOUNT_RAW_COORDINATES_DEC_ITEM->number.target, &MOUNT_EQUATORIAL_COORDINATES_RA_ITEM->number.target, &MOUNT_EQUATORIAL_COORDINATES_DEC_ITEM->number.target);
		indigo_set_timer(device, 1, position_timer_callback, &PRIVATE_DATA->position_timer);
	} else {
		indigo_cancel_timer_sync(device, &PRIVATE_DATA->position_timer);
	}
	CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
	indigo_mount_change_property(device, NULL, CONNECTION_PROPERTY);
}

static indigo_result mount_change_property(indigo_device *device, indigo_client *client, indigo_property *property) {
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
		indigo_set_timer(device, 0, mount_connect_callback, NULL);
		return INDIGO_OK;
	} else if (indigo_property_match(MOUNT_PARK_PROPERTY, property)) {
			// -------------------------------------------------------------------------------- MOUNT_PARK
		indigo_property_copy_values(MOUNT_PARK_PROPERTY, property, false);
		if (MOUNT_PARK_PARKED_ITEM->sw.value) {
			MOUNT_PARK_PROPERTY->state = INDIGO_BUSY_STATE;
			indigo_update_property(device, MOUNT_PARK_PROPERTY, "Parking...");
			MOUNT_EQUATORIAL_COORDINATES_RA_ITEM->number.target = fmod(indigo_lst(NULL, MOUNT_GEOGRAPHIC_COORDINATES_LONGITUDE_ITEM->number.value) - (PRIVATE_DATA->ha = MOUNT_PARK_POSITION_HA_ITEM->number.value) + 24, 24);
			MOUNT_EQUATORIAL_COORDINATES_DEC_ITEM->number.target = MOUNT_PARK_POSITION_DEC_ITEM->number.value;
			indigo_translated_to_raw(device, MOUNT_EQUATORIAL_COORDINATES_RA_ITEM->number.target, MOUNT_EQUATORIAL_COORDINATES_DEC_ITEM->number.target, &MOUNT_RAW_COORDINATES_RA_ITEM->number.target, &MOUNT_RAW_COORDINATES_DEC_ITEM->number.target);
			PRIVATE_DATA->parking = true;
			PRIVATE_DATA->parked = false;
			PRIVATE_DATA->slew_in_progress = true;
			MOUNT_EQUATORIAL_COORDINATES_PROPERTY->state = MOUNT_RAW_COORDINATES_PROPERTY->state = INDIGO_BUSY_STATE;
			indigo_update_coordinates(device, NULL);
		} else {
			indigo_set_switch(MOUNT_TRACKING_PROPERTY, MOUNT_TRACKING_ON_ITEM, true);
			MOUNT_TRACKING_PROPERTY->state = INDIGO_OK_STATE;
			indigo_update_property(device, MOUNT_TRACKING_PROPERTY, NULL);
			MOUNT_PARK_PROPERTY->state = INDIGO_OK_STATE;
			indigo_update_property(device, MOUNT_PARK_PROPERTY, "Unparked");
			PRIVATE_DATA->parking = false;
			PRIVATE_DATA->parked = false;
		}
		return INDIGO_OK;
	} else if (indigo_property_match(MOUNT_HOME_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- MOUNT_HOME
		indigo_property_copy_values(MOUNT_HOME_PROPERTY, property, false);
		if (MOUNT_HOME_ITEM->sw.value) {
			MOUNT_HOME_ITEM->sw.value = false;
			MOUNT_HOME_PROPERTY->state = INDIGO_BUSY_STATE;
			indigo_update_property(device, MOUNT_HOME_PROPERTY, "Going home...");
			time_t utc = indigo_get_mount_utc(device);
			MOUNT_EQUATORIAL_COORDINATES_RA_ITEM->number.target = fmod(indigo_lst(&utc, MOUNT_GEOGRAPHIC_COORDINATES_LONGITUDE_ITEM->number.value) - (PRIVATE_DATA->ha = MOUNT_HOME_POSITION_HA_ITEM->number.value) + 24, 24);
			MOUNT_EQUATORIAL_COORDINATES_DEC_ITEM->number.target = MOUNT_HOME_POSITION_DEC_ITEM->number.value;
			indigo_translated_to_raw(device, MOUNT_EQUATORIAL_COORDINATES_RA_ITEM->number.target, MOUNT_EQUATORIAL_COORDINATES_DEC_ITEM->number.target, &MOUNT_RAW_COORDINATES_RA_ITEM->number.target, &MOUNT_RAW_COORDINATES_DEC_ITEM->number.target);
			PRIVATE_DATA->going_home = true;
			PRIVATE_DATA->slew_in_progress = true;
			MOUNT_EQUATORIAL_COORDINATES_PROPERTY->state = MOUNT_RAW_COORDINATES_PROPERTY->state = INDIGO_BUSY_STATE;
			indigo_update_coordinates(device, NULL);
		}
		return INDIGO_OK;
	} else if (indigo_property_match(MOUNT_EQUATORIAL_COORDINATES_PROPERTY, property)) {
			// -------------------------------------------------------------------------------- MOUNT_EQUATORIAL_COORDINATES
		if (MOUNT_PARK_PARKED_ITEM->sw.value) {
			MOUNT_EQUATORIAL_COORDINATES_PROPERTY->state = INDIGO_ALERT_STATE;
			indigo_update_property(device, MOUNT_EQUATORIAL_COORDINATES_PROPERTY, "Mount is parked");
		} else if (MOUNT_ON_COORDINATES_SET_SYNC_ITEM->sw.value) {
			if (MOUNT_ALIGNMENT_MODE_CONTROLLER_ITEM->sw.value) {
				indigo_property_copy_values(MOUNT_EQUATORIAL_COORDINATES_PROPERTY, property, false);
				MOUNT_RAW_COORDINATES_RA_ITEM->number.target = MOUNT_RAW_COORDINATES_RA_ITEM->number.value = MOUNT_EQUATORIAL_COORDINATES_RA_ITEM->number.value;
				MOUNT_RAW_COORDINATES_DEC_ITEM->number.target = MOUNT_RAW_COORDINATES_DEC_ITEM->number.value = MOUNT_EQUATORIAL_COORDINATES_DEC_ITEM->number.value;
				MOUNT_EQUATORIAL_COORDINATES_PROPERTY->state = INDIGO_OK_STATE;
				indigo_update_coordinates(device, NULL);
			} else {
				indigo_mount_change_property(device, client, property);
				MOUNT_EQUATORIAL_COORDINATES_PROPERTY->state = INDIGO_OK_STATE;
				indigo_update_coordinates(device, NULL);
			}
		} else if (MOUNT_ON_COORDINATES_SET_TRACK_ITEM->sw.value) {
			double ra = MOUNT_EQUATORIAL_COORDINATES_RA_ITEM->number.value;
			double dec = MOUNT_EQUATORIAL_COORDINATES_DEC_ITEM->number.value;
			indigo_property_copy_values(MOUNT_EQUATORIAL_COORDINATES_PROPERTY, property, false);
			indigo_translated_to_raw(device, MOUNT_EQUATORIAL_COORDINATES_RA_ITEM->number.target, MOUNT_EQUATORIAL_COORDINATES_DEC_ITEM->number.target, &MOUNT_RAW_COORDINATES_RA_ITEM->number.target, &MOUNT_RAW_COORDINATES_DEC_ITEM->number.target);
			MOUNT_EQUATORIAL_COORDINATES_RA_ITEM->number.value = ra;
			MOUNT_EQUATORIAL_COORDINATES_DEC_ITEM->number.value = dec;
			MOUNT_EQUATORIAL_COORDINATES_PROPERTY->state = MOUNT_RAW_COORDINATES_PROPERTY->state = INDIGO_BUSY_STATE;
			PRIVATE_DATA->slew_in_progress = true;
			indigo_update_coordinates(device, NULL);
		}
		return INDIGO_OK;
	} else if (indigo_property_match(MOUNT_MOTION_DEC_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- MOUNT_MOTION_DEC
		if (MOUNT_PARK_PARKED_ITEM->sw.value) {
			MOUNT_MOTION_DEC_PROPERTY->state = INDIGO_ALERT_STATE;
			indigo_update_property(device, MOUNT_MOTION_DEC_PROPERTY, "Mount is parked");
		} else {
			indigo_property_copy_values(MOUNT_MOTION_DEC_PROPERTY, property, false);
			MOUNT_MOTION_DEC_PROPERTY->state = INDIGO_OK_STATE;
			indigo_update_property(device, MOUNT_MOTION_DEC_PROPERTY, NULL);
			if (PRIVATE_DATA->move_timer == NULL)
				indigo_set_timer(device, 0, move_timer_callback, &PRIVATE_DATA->move_timer);
		}
		return INDIGO_OK;
	} else if (indigo_property_match(MOUNT_MOTION_RA_PROPERTY, property)) {
			// -------------------------------------------------------------------------------- MOUNT_MOTION_RA
		if (MOUNT_PARK_PARKED_ITEM->sw.value) {
			MOUNT_MOTION_RA_PROPERTY->state = INDIGO_ALERT_STATE;
			indigo_update_property(device, MOUNT_MOTION_RA_PROPERTY, "Mount is parked");
		} else {
			indigo_property_copy_values(MOUNT_MOTION_RA_PROPERTY, property, false);
			MOUNT_MOTION_RA_PROPERTY->state = INDIGO_OK_STATE;
			indigo_update_property(device, MOUNT_MOTION_RA_PROPERTY, NULL);
			if (PRIVATE_DATA->move_timer == NULL)
				indigo_set_timer(device, 0, move_timer_callback, &PRIVATE_DATA->move_timer);
		}
		return INDIGO_OK;
	} else if (indigo_property_match(MOUNT_ABORT_MOTION_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- MOUNT_ABORT_MOTION
		if (MOUNT_PARK_PARKED_ITEM->sw.value) {
			MOUNT_ABORT_MOTION_PROPERTY->state = INDIGO_ALERT_STATE;
			indigo_update_property(device, MOUNT_ABORT_MOTION_PROPERTY, "Mount is parked");
		} else {
			indigo_property_copy_values(MOUNT_ABORT_MOTION_PROPERTY, property, false);
			if (PRIVATE_DATA->slew_in_progress) {
				PRIVATE_DATA->slew_in_progress = false;
				MOUNT_EQUATORIAL_COORDINATES_PROPERTY->state = INDIGO_ALERT_STATE;
				indigo_update_coordinates(device, NULL);
			}
			if (indigo_cancel_timer(device, &PRIVATE_DATA->move_timer)) {
				MOUNT_MOTION_DEC_PROPERTY->state = INDIGO_ALERT_STATE;
				MOUNT_MOTION_NORTH_ITEM->sw.value = false;
				MOUNT_MOTION_SOUTH_ITEM->sw.value = false;
				indigo_update_property(device, MOUNT_MOTION_DEC_PROPERTY, NULL);
				MOUNT_MOTION_RA_PROPERTY->state = INDIGO_ALERT_STATE;
				MOUNT_MOTION_EAST_ITEM->sw.value = false;
				MOUNT_MOTION_WEST_ITEM->sw.value = false;
				indigo_update_property(device, MOUNT_MOTION_RA_PROPERTY, NULL);
				MOUNT_EQUATORIAL_COORDINATES_PROPERTY->state = INDIGO_ALERT_STATE;
				indigo_update_coordinates(device, NULL);
			}
			MOUNT_ABORT_MOTION_ITEM->sw.value = false;
			MOUNT_ABORT_MOTION_PROPERTY->state = INDIGO_OK_STATE;
			indigo_update_property(device, MOUNT_ABORT_MOTION_PROPERTY, "Aborted");
		}
		return INDIGO_OK;
	} else if (indigo_property_match(MOUNT_TRACKING_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- MOUNT_TRACKING
		if (MOUNT_PARK_PARKED_ITEM->sw.value) {
			MOUNT_TRACKING_PROPERTY->state = INDIGO_ALERT_STATE;
			indigo_update_property(device, MOUNT_TRACKING_PROPERTY, "Mount is parked");
		} else {
			indigo_property_copy_values(MOUNT_TRACKING_PROPERTY, property, false);
			time_t utc = indigo_get_mount_utc(device);
			PRIVATE_DATA->ha = indigo_lst(&utc, MOUNT_GEOGRAPHIC_COORDINATES_LONGITUDE_ITEM->number.value) - MOUNT_RAW_COORDINATES_RA_ITEM->number.value;
			MOUNT_TRACKING_PROPERTY->state = INDIGO_OK_STATE;
			indigo_update_property(device, MOUNT_TRACKING_PROPERTY, NULL);
		}
		return INDIGO_OK;
		// --------------------------------------------------------------------------------
	}
	return indigo_mount_change_property(device, client, property);
}

static indigo_result mount_detach(indigo_device *device) {
	assert(device != NULL);
	if (IS_CONNECTED) {
		indigo_set_switch(CONNECTION_PROPERTY, CONNECTION_DISCONNECTED_ITEM, true);
		mount_connect_callback(device);
	}
	INDIGO_DEVICE_DETACH_LOG(DRIVER_NAME, device->name);
	return indigo_mount_detach(device);
}

	// -------------------------------------------------------------------------------- INDIGO guider device implementation

static void guider_timer_callback(indigo_device *device) {
	pthread_mutex_lock(&PRIVATE_DATA->position_mutex);
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
	pthread_mutex_unlock(&PRIVATE_DATA->position_mutex);
}

static indigo_result guider_attach(indigo_device *device) {
	assert(device != NULL);
	assert(PRIVATE_DATA != NULL);
	if (indigo_guider_attach(device, DRIVER_NAME, DRIVER_VERSION) == INDIGO_OK) {
		GUIDER_RATE_PROPERTY->hidden = false;
		GUIDER_RATE_PROPERTY->count = 2;
		INDIGO_DEVICE_ATTACH_LOG(DRIVER_NAME, device->name);
		return indigo_guider_enumerate_properties(device, NULL, NULL);
	}
	return INDIGO_FAILED;
}

static void guider_connect_callback(indigo_device *device) {
	CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
	if (CONNECTION_CONNECTED_ITEM->sw.value) {
		indigo_cancel_timer_sync(device, &PRIVATE_DATA->guider_timer);
	}
	indigo_guider_change_property(device, NULL, CONNECTION_PROPERTY);
}

static indigo_result guider_change_property(indigo_device *device, indigo_client *client, indigo_property *property) {
	assert(device != NULL);
	assert(DEVICE_CONTEXT != NULL);
	assert(property != NULL);
	if (indigo_property_match(CONNECTION_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- CONNECTION
		indigo_property_copy_values(CONNECTION_PROPERTY, property, false);
		CONNECTION_PROPERTY->state = INDIGO_BUSY_STATE;
		indigo_update_property(device, CONNECTION_PROPERTY, NULL);
		indigo_set_timer(device, 0, guider_connect_callback, NULL);
		return INDIGO_OK;
	} else if (indigo_property_match(GUIDER_GUIDE_DEC_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- GUIDER_GUIDE_DEC
		indigo_cancel_timer(device, &PRIVATE_DATA->guider_timer);
		indigo_property_copy_values(GUIDER_GUIDE_DEC_PROPERTY, property, false);
		GUIDER_GUIDE_DEC_PROPERTY->state = INDIGO_OK_STATE;
		int duration = GUIDER_GUIDE_NORTH_ITEM->number.value;
		if (duration > 0) {
			GUIDER_GUIDE_DEC_PROPERTY->state = INDIGO_BUSY_STATE;
			indigo_set_timer(device, duration/1000.0, guider_timer_callback, &PRIVATE_DATA->guider_timer);
		} else {
			int duration = GUIDER_GUIDE_SOUTH_ITEM->number.value;
			if (duration > 0) {
				GUIDER_GUIDE_DEC_PROPERTY->state = INDIGO_BUSY_STATE;
				indigo_set_timer(device, duration/1000.0, guider_timer_callback, &PRIVATE_DATA->guider_timer);
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
			indigo_set_timer(device, duration/1000.0, guider_timer_callback, &PRIVATE_DATA->guider_timer);
		} else {
			int duration = GUIDER_GUIDE_WEST_ITEM->number.value;
			if (duration > 0) {
				GUIDER_GUIDE_RA_PROPERTY->state = INDIGO_BUSY_STATE;
				indigo_set_timer(device, duration/1000.0, guider_timer_callback, &PRIVATE_DATA->guider_timer);
			}
		}
		indigo_update_property(device, GUIDER_GUIDE_RA_PROPERTY, NULL);
		return INDIGO_OK;
	} else if (indigo_property_match(GUIDER_RATE_PROPERTY, property)) {
			// -------------------------------------------------------------------------------- GUIDER_RATE
		indigo_property_copy_values(GUIDER_RATE_PROPERTY, property, false);
		GUIDER_RATE_PROPERTY->state = INDIGO_OK_STATE;
		indigo_update_property(device, GUIDER_RATE_PROPERTY, NULL);
		return INDIGO_OK;
			// --------------------------------------------------------------------------------
	}
	return indigo_guider_change_property(device, client, property);
}

static indigo_result guider_detach(indigo_device *device) {
	assert(device != NULL);
	if (IS_CONNECTED) {
		indigo_set_switch(CONNECTION_PROPERTY, CONNECTION_DISCONNECTED_ITEM, true);
		guider_connect_callback(device);
	}
	INDIGO_DEVICE_DETACH_LOG(DRIVER_NAME, device->name);
	return indigo_guider_detach(device);
}

	// --------------------------------------------------------------------------------

static simulator_private_data *private_data = NULL;

static indigo_device *mount = NULL;
static indigo_device *mount_guider = NULL;

indigo_result indigo_mount_simulator(indigo_driver_action action, indigo_driver_info *info) {
	static indigo_device mount_template = INDIGO_DEVICE_INITIALIZER(
		MOUNT_SIMULATOR_NAME,
		mount_attach,
		indigo_mount_enumerate_properties,
		mount_change_property,
		NULL,
		mount_detach
	);
	static indigo_device mount_guider_template = INDIGO_DEVICE_INITIALIZER(
		MOUNT_SIMULATOR_GUIDER_NAME,
		guider_attach,
		indigo_guider_enumerate_properties,
		guider_change_property,
		NULL,
		guider_detach
	);

	static indigo_driver_action last_action = INDIGO_DRIVER_SHUTDOWN;

	SET_DRIVER_INFO(info, "Mount Simulator", __FUNCTION__, DRIVER_VERSION, false, last_action);

	if (action == last_action)
		return INDIGO_OK;

	switch (action) {
		case INDIGO_DRIVER_INIT:
			last_action = action;
			private_data = indigo_safe_malloc(sizeof(simulator_private_data));
			pthread_mutex_init(&private_data->position_mutex, NULL);
			mount = indigo_safe_malloc_copy(sizeof(indigo_device), &mount_template);
			mount->private_data = private_data;
			indigo_attach_device(mount);
			mount_guider = indigo_safe_malloc_copy(sizeof(indigo_device), &mount_guider_template);
			mount_guider->private_data = private_data;
			indigo_attach_device(mount_guider);
			break;

		case INDIGO_DRIVER_SHUTDOWN:
			VERIFY_NOT_CONNECTED(mount);
			VERIFY_NOT_CONNECTED(mount_guider);
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
				pthread_mutex_destroy(&private_data->position_mutex);
				free(private_data);
				private_data = NULL;
			}
			break;

		case INDIGO_DRIVER_INFO:
			break;
	}

	return INDIGO_OK;
}
