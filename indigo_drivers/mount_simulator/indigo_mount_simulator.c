// Copyright (c) 2016-2026 CloudMakers, s. r. o.
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
// 3.0 refactoring by Peter Polakovic <peter.polakovic@cloudmakers.eu>

/** INDIGO MOUNT Simulator driver
 \file indigo_mount_simulator.c
 */

#define DRIVER_VERSION 0x0300000C
#define DRIVER_NAME "indigo_mount_simulator"

#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <assert.h>

#include <indigo/indigo_driver_xml.h>
#include <indigo/indigo_align.h>

#include "indigo_mount_simulator.h"

#define PRIVATE_DATA        ((simulator_private_data *)device->private_data)

typedef struct {
	bool parking, parked, going_home, at_home;
	double ha;
	bool slew_in_progress;
} simulator_private_data;

	// -------------------------------------------------------------------------------- INDIGO MOUNT device implementation

static void position_handler(indigo_device *device) {
	if (!IS_CONNECTED) {
		return;
	}
	double diffRA = MOUNT_RAW_COORDINATES_RA_ITEM->number.target - MOUNT_RAW_COORDINATES_RA_ITEM->number.value;
	if (diffRA > 12) {
		diffRA = -(24 - diffRA);
	} else if (diffRA < -12) {
		diffRA = (24 - diffRA);
	}
	double diffDec = MOUNT_RAW_COORDINATES_DEC_ITEM->number.target - MOUNT_RAW_COORDINATES_DEC_ITEM->number.value;
	if (PRIVATE_DATA->slew_in_progress) {
		if (diffRA == 0 && diffDec == 0) {
			if (MOUNT_EQUATORIAL_COORDINATES_PROPERTY->state == INDIGO_BUSY_STATE) {
				MOUNT_EQUATORIAL_COORDINATES_PROPERTY->state = MOUNT_RAW_COORDINATES_PROPERTY->state = INDIGO_OK_STATE;
				MOUNT_STATE_SLEW_ITEM->light.value = INDIGO_IDLE_STATE;
				indigo_update_property(device, MOUNT_STATE_PROPERTY, NULL);
			}
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
				indigo_update_property(device, MOUNT_PARK_PROPERTY, NULL);
				MOUNT_STATE_PARK_ITEM->light.value = INDIGO_OK_STATE;
				MOUNT_STATE_TRACKING_ITEM->light.value = INDIGO_IDLE_STATE;
				indigo_update_property(device, MOUNT_STATE_PROPERTY, NULL);
			} else if (PRIVATE_DATA->going_home) {
				PRIVATE_DATA->going_home = false;
				PRIVATE_DATA->at_home = true;
				indigo_set_switch(MOUNT_TRACKING_PROPERTY, MOUNT_TRACKING_OFF_ITEM, true);
				indigo_update_property(device, MOUNT_TRACKING_PROPERTY, NULL);
				MOUNT_HOME_PROPERTY->state = INDIGO_OK_STATE;
				indigo_update_property(device, MOUNT_HOME_PROPERTY, NULL);
				MOUNT_STATE_HOME_ITEM->light.value = INDIGO_OK_STATE;
				MOUNT_STATE_TRACKING_ITEM->light.value = INDIGO_IDLE_STATE;
				indigo_update_property(device, MOUNT_STATE_PROPERTY, NULL);
			} else {
				if (MOUNT_TRACKING_OFF_ITEM->sw.value) {
					indigo_set_switch(MOUNT_TRACKING_PROPERTY, MOUNT_TRACKING_ON_ITEM, true);
					indigo_update_property(device, MOUNT_TRACKING_PROPERTY, NULL);
					MOUNT_STATE_TRACKING_ITEM->light.value = INDIGO_OK_STATE;
					indigo_update_property(device, MOUNT_STATE_PROPERTY, NULL);
				}
			}
		} else {
			double speedRA = 0.2;
			double speedDec = 1.5;
			if (fabs(diffRA) < speedRA) {
				MOUNT_RAW_COORDINATES_RA_ITEM->number.value = MOUNT_RAW_COORDINATES_RA_ITEM->number.target;
			} else if (diffRA > 0) {
				MOUNT_RAW_COORDINATES_RA_ITEM->number.value += speedRA;
				if (MOUNT_RAW_COORDINATES_RA_ITEM->number.value > 24) {
					MOUNT_RAW_COORDINATES_RA_ITEM->number.value -= 24;
				}
			} else if (diffRA < 0) {
				MOUNT_RAW_COORDINATES_RA_ITEM->number.value -= speedRA;
				if (MOUNT_RAW_COORDINATES_RA_ITEM->number.value < 0) {
					MOUNT_RAW_COORDINATES_RA_ITEM->number.value += 24;
				}
			}
			if (fabs(diffDec) < speedDec) {
				MOUNT_RAW_COORDINATES_DEC_ITEM->number.value = MOUNT_RAW_COORDINATES_DEC_ITEM->number.target;
			} else if (diffDec > 0) {
				MOUNT_RAW_COORDINATES_DEC_ITEM->number.value += speedDec;
			} else if (diffDec < 0) {
				MOUNT_RAW_COORDINATES_DEC_ITEM->number.value -= speedDec;
			}
			MOUNT_EQUATORIAL_COORDINATES_PROPERTY->state = INDIGO_BUSY_STATE;
		}
		indigo_execute_handler_in(device, 0.2, position_handler);
	} else {
		if (PRIVATE_DATA->parked) {
			MOUNT_RAW_COORDINATES_RA_ITEM->number.value = fmod(indigo_lst(NULL, MOUNT_GEOGRAPHIC_COORDINATES_LONGITUDE_ITEM->number.value) - PRIVATE_DATA->ha + 24, 24);
			MOUNT_RAW_COORDINATES_DEC_ITEM->number.value = MOUNT_PARK_POSITION_DEC_ITEM->number.value;
		} else if (MOUNT_EQUATORIAL_COORDINATES_PROPERTY->state == INDIGO_OK_STATE && MOUNT_TRACKING_OFF_ITEM->sw.value) {
			MOUNT_RAW_COORDINATES_RA_ITEM->number.value = fmod(indigo_lst(NULL, MOUNT_GEOGRAPHIC_COORDINATES_LONGITUDE_ITEM->number.value) - PRIVATE_DATA->ha + 24, 24);
		}
		indigo_execute_handler_in(device, 1.0, position_handler);
	}
	indigo_raw_to_translated(device, MOUNT_RAW_COORDINATES_RA_ITEM->number.value, MOUNT_RAW_COORDINATES_DEC_ITEM->number.value, &MOUNT_EQUATORIAL_COORDINATES_RA_ITEM->number.value, &MOUNT_EQUATORIAL_COORDINATES_DEC_ITEM->number.value);
	indigo_update_coordinates(device, NULL);
	indigo_update_property(device, MOUNT_RAW_COORDINATES_PROPERTY, NULL);
}

static void manual_motion_finalizer(indigo_device *device) {
	if (!IS_CONNECTED) {
		return;
	}
	double speed = 0;
	if (MOUNT_SLEW_RATE_GUIDE_ITEM->sw.value) {
		speed = 0.01;
	} else if (MOUNT_SLEW_RATE_CENTERING_ITEM->sw.value) {
		speed = 0.025;
	} else if (MOUNT_SLEW_RATE_FIND_ITEM->sw.value) {
		speed = 0.1;
	} else if (MOUNT_SLEW_RATE_MAX_ITEM->sw.value) {
		speed = 0.5;
	}
	double decStep = 0;
	if (MOUNT_MOTION_NORTH_ITEM->sw.value) {
		decStep = speed * 15;
	} else if (MOUNT_MOTION_SOUTH_ITEM->sw.value) {
		decStep = -speed * 15;
	}
	double raStep = 0;
	if (MOUNT_MOTION_WEST_ITEM->sw.value) {
		raStep = speed;
	} else if (MOUNT_MOTION_EAST_ITEM->sw.value) {
		raStep = -speed;
	}
	if (raStep == 0 && decStep == 0) {
		MOUNT_EQUATORIAL_COORDINATES_PROPERTY->state = MOUNT_RAW_COORDINATES_PROPERTY->state = INDIGO_OK_STATE;
	} else {
		MOUNT_RAW_COORDINATES_RA_ITEM->number.target = MOUNT_RAW_COORDINATES_RA_ITEM->number.value = fmod(MOUNT_RAW_COORDINATES_RA_ITEM->number.value + raStep * speed + 24, 24);
		MOUNT_RAW_COORDINATES_DEC_ITEM->number.target = MOUNT_RAW_COORDINATES_DEC_ITEM->number.value = fmod(MOUNT_RAW_COORDINATES_DEC_ITEM->number.value + decStep * speed + 360 + 180, 360) - 180;
		MOUNT_EQUATORIAL_COORDINATES_PROPERTY->state = INDIGO_BUSY_STATE;
		indigo_execute_handler_in(device, 0.5, manual_motion_finalizer);
	}
	indigo_raw_to_translated(device, MOUNT_RAW_COORDINATES_RA_ITEM->number.value, MOUNT_RAW_COORDINATES_DEC_ITEM->number.value, &MOUNT_EQUATORIAL_COORDINATES_RA_ITEM->number.value, &MOUNT_EQUATORIAL_COORDINATES_DEC_ITEM->number.value);
	indigo_update_coordinates(device, NULL);
	indigo_update_property(device, MOUNT_RAW_COORDINATES_PROPERTY, NULL);
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
		// -------------------------------------------------------------------------------- MOUNT_STATE
		MOUNT_STATE_PROPERTY->hidden = false;
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
		// -------------------------------------------------------------------------------- MOUNT_SIDE_OF_PIER
		MOUNT_SIDE_OF_PIER_PROPERTY->hidden = false;
		// -------------------------------------------------------------------------------- MOUNT_EPOCH
		MOUNT_EPOCH_PROPERTY->perm = INDIGO_RO_PERM;
		// -------------------------------------------------------------------------------- MOUNT_ON_COORDINATES_SET
		MOUNT_ON_COORDINATES_SET_PROPERTY->count = 2;
		// -------------------------------------------------------------------------------- MOUNT_CUSTOM_TRACKING_RATE
		MOUNT_CUSTOM_TRACKING_RATE_PROPERTY->hidden = false;
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
		ADDITIONAL_INSTANCES_PROPERTY->hidden = device->base_device != NULL;
		INDIGO_DEVICE_ATTACH_LOG(DRIVER_NAME, device->name);
		return indigo_mount_enumerate_properties(device, NULL, NULL);
	}
	return INDIGO_FAILED;
}

static void mount_connection_handler(indigo_device *device) {
	if (CONNECTION_CONNECTED_ITEM->sw.value) {
		if (MOUNT_PARK_PARKED_ITEM->sw.value) {
			MOUNT_EQUATORIAL_COORDINATES_RA_ITEM->number.value = MOUNT_EQUATORIAL_COORDINATES_RA_ITEM->number.target = fmod(indigo_lst(NULL, MOUNT_GEOGRAPHIC_COORDINATES_LONGITUDE_ITEM->number.value) - (PRIVATE_DATA->ha = MOUNT_PARK_POSITION_HA_ITEM->number.value) + 24, 24);
			MOUNT_RAW_COORDINATES_DEC_ITEM->number.value = MOUNT_RAW_COORDINATES_DEC_ITEM->number.target = MOUNT_PARK_POSITION_DEC_ITEM->number.value;
			indigo_translated_to_raw(device, MOUNT_EQUATORIAL_COORDINATES_RA_ITEM->number.target, MOUNT_EQUATORIAL_COORDINATES_DEC_ITEM->number.target, &MOUNT_RAW_COORDINATES_RA_ITEM->number.target, &MOUNT_RAW_COORDINATES_DEC_ITEM->number.target);
			MOUNT_RAW_COORDINATES_RA_ITEM->number.value = MOUNT_RAW_COORDINATES_RA_ITEM->number.target;
			MOUNT_RAW_COORDINATES_DEC_ITEM->number.value = MOUNT_RAW_COORDINATES_DEC_ITEM->number.target;
			MOUNT_STATE_PARK_ITEM->light.value = INDIGO_OK_STATE;
		} else {
			MOUNT_STATE_PARK_ITEM->light.value = INDIGO_IDLE_STATE;
		}
		indigo_raw_to_translated(device, MOUNT_RAW_COORDINATES_RA_ITEM->number.value, MOUNT_RAW_COORDINATES_DEC_ITEM->number.value, &MOUNT_EQUATORIAL_COORDINATES_RA_ITEM->number.value, &MOUNT_EQUATORIAL_COORDINATES_DEC_ITEM->number.value);
		indigo_raw_to_translated(device, MOUNT_RAW_COORDINATES_RA_ITEM->number.target, MOUNT_RAW_COORDINATES_DEC_ITEM->number.target, &MOUNT_EQUATORIAL_COORDINATES_RA_ITEM->number.target, &MOUNT_EQUATORIAL_COORDINATES_DEC_ITEM->number.target);
		indigo_execute_handler_in(device, 1, position_handler);
		indigo_send_message(device, OK_PROPERTY, "Connected to %s", device->name);
	} else {
		indigo_cancel_pending_handlers(device);
		PRIVATE_DATA->slew_in_progress = PRIVATE_DATA->parking = PRIVATE_DATA->going_home = false;
		if (MOUNT_PARK_PROPERTY->state == INDIGO_BUSY_STATE) {
			indigo_set_switch(MOUNT_PARK_PROPERTY, MOUNT_PARK_UNPARKED_ITEM, true);
			MOUNT_PARK_PROPERTY->state = INDIGO_ALERT_STATE;
		}
		if (MOUNT_HOME_PROPERTY->state == INDIGO_BUSY_STATE) {
			MOUNT_HOME_ITEM->sw.value = false;
			MOUNT_HOME_PROPERTY->state = INDIGO_ALERT_STATE;
		}
		MOUNT_MOTION_NORTH_ITEM->sw.value = MOUNT_MOTION_SOUTH_ITEM->sw.value = false;
		MOUNT_MOTION_EAST_ITEM->sw.value = MOUNT_MOTION_WEST_ITEM->sw.value = false;
		MOUNT_RAW_COORDINATES_RA_ITEM->number.target = MOUNT_RAW_COORDINATES_RA_ITEM->number.value;
		MOUNT_RAW_COORDINATES_DEC_ITEM->number.target = MOUNT_RAW_COORDINATES_DEC_ITEM->number.value;
		MOUNT_EQUATORIAL_COORDINATES_PROPERTY->state = MOUNT_RAW_COORDINATES_PROPERTY->state = INDIGO_OK_STATE;
		indigo_send_message(device, OK_PROPERTY, "Disconnected from %s", device->name);
	}
	CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
	indigo_mount_change_property(device, NULL, CONNECTION_PROPERTY);
}

static void mount_park_handler(indigo_device *device) {
	if (MOUNT_PARK_PARKED_ITEM->sw.value && !(PRIVATE_DATA->parking || PRIVATE_DATA->parked)) {
		MOUNT_EQUATORIAL_COORDINATES_RA_ITEM->number.target = fmod(indigo_lst(NULL, MOUNT_GEOGRAPHIC_COORDINATES_LONGITUDE_ITEM->number.value) - (PRIVATE_DATA->ha = MOUNT_PARK_POSITION_HA_ITEM->number.value) + 24, 24);
		MOUNT_EQUATORIAL_COORDINATES_DEC_ITEM->number.target = MOUNT_PARK_POSITION_DEC_ITEM->number.value;
		indigo_translated_to_raw(device, MOUNT_EQUATORIAL_COORDINATES_RA_ITEM->number.target, MOUNT_EQUATORIAL_COORDINATES_DEC_ITEM->number.target, &MOUNT_RAW_COORDINATES_RA_ITEM->number.target, &MOUNT_RAW_COORDINATES_DEC_ITEM->number.target);
		indigo_set_switch(MOUNT_TRACKING_PROPERTY, MOUNT_TRACKING_OFF_ITEM, true);
		MOUNT_TRACKING_PROPERTY->state = INDIGO_OK_STATE;
		indigo_update_property(device, MOUNT_TRACKING_PROPERTY, NULL);
		indigo_set_switch(MOUNT_SIDE_OF_PIER_PROPERTY, MOUNT_SIDE_OF_PIER_EAST_ITEM, true);
		MOUNT_SIDE_OF_PIER_PROPERTY->state = INDIGO_OK_STATE;
		indigo_update_property(device, MOUNT_SIDE_OF_PIER_PROPERTY, NULL);
		MOUNT_EQUATORIAL_COORDINATES_PROPERTY->state = MOUNT_RAW_COORDINATES_PROPERTY->state = INDIGO_BUSY_STATE;
		indigo_update_coordinates(device, NULL);
		PRIVATE_DATA->parking = true;
		PRIVATE_DATA->parked = false;
		PRIVATE_DATA->slew_in_progress = true;
		MOUNT_STATE_SLEW_ITEM->light.value = INDIGO_BUSY_STATE;
		MOUNT_STATE_PARK_ITEM->light.value = INDIGO_BUSY_STATE;
		MOUNT_STATE_TRACKING_ITEM->light.value = INDIGO_IDLE_STATE;
		indigo_update_property(device, MOUNT_STATE_PROPERTY, NULL);
		indigo_cancel_pending_handler(device, position_handler);
		indigo_execute_handler(device, position_handler);
		return;
	} else if (MOUNT_PARK_UNPARKED_ITEM->sw.value && (PRIVATE_DATA->parking || PRIVATE_DATA->parked)) {
		indigo_set_switch(MOUNT_TRACKING_PROPERTY, MOUNT_TRACKING_ON_ITEM, true);
		MOUNT_TRACKING_PROPERTY->state = INDIGO_OK_STATE;
		indigo_update_property(device, MOUNT_TRACKING_PROPERTY, NULL);
		PRIVATE_DATA->parking = false;
		PRIVATE_DATA->parked = false;
		PRIVATE_DATA->slew_in_progress = false;
		MOUNT_STATE_SLEW_ITEM->light.value = MOUNT_STATE_PARK_ITEM->light.value = INDIGO_IDLE_STATE;
		MOUNT_STATE_TRACKING_ITEM->light.value = INDIGO_OK_STATE;
		indigo_update_property(device, MOUNT_STATE_PROPERTY, NULL);
	}
	MOUNT_PARK_PROPERTY->state = INDIGO_OK_STATE;
	indigo_update_property(device, MOUNT_PARK_PROPERTY, NULL);
}

static void mount_home_handler(indigo_device *device) {
	if (MOUNT_HOME_ITEM->sw.value) {
		MOUNT_HOME_ITEM->sw.value = false;
		PRIVATE_DATA->at_home = false;
		time_t utc = indigo_get_mount_utc(device);
		MOUNT_EQUATORIAL_COORDINATES_RA_ITEM->number.target = fmod(indigo_lst(&utc, MOUNT_GEOGRAPHIC_COORDINATES_LONGITUDE_ITEM->number.value) - (PRIVATE_DATA->ha = MOUNT_HOME_POSITION_HA_ITEM->number.value) + 24, 24);
		MOUNT_EQUATORIAL_COORDINATES_DEC_ITEM->number.target = MOUNT_HOME_POSITION_DEC_ITEM->number.value;
		indigo_translated_to_raw(device, MOUNT_EQUATORIAL_COORDINATES_RA_ITEM->number.target, MOUNT_EQUATORIAL_COORDINATES_DEC_ITEM->number.target, &MOUNT_RAW_COORDINATES_RA_ITEM->number.target, &MOUNT_RAW_COORDINATES_DEC_ITEM->number.target);
		indigo_set_switch(MOUNT_TRACKING_PROPERTY, MOUNT_TRACKING_OFF_ITEM, true);
		MOUNT_TRACKING_PROPERTY->state = INDIGO_OK_STATE;
		indigo_update_property(device, MOUNT_TRACKING_PROPERTY, NULL);
		indigo_set_switch(MOUNT_SIDE_OF_PIER_PROPERTY, MOUNT_SIDE_OF_PIER_EAST_ITEM, true);
		MOUNT_SIDE_OF_PIER_PROPERTY->state = INDIGO_OK_STATE;
		indigo_update_property(device, MOUNT_SIDE_OF_PIER_PROPERTY, NULL);
		MOUNT_EQUATORIAL_COORDINATES_PROPERTY->state = MOUNT_RAW_COORDINATES_PROPERTY->state = INDIGO_BUSY_STATE;
		indigo_update_coordinates(device, NULL);
		PRIVATE_DATA->going_home = true;
		PRIVATE_DATA->slew_in_progress = true;
		MOUNT_STATE_SLEW_ITEM->light.value = INDIGO_BUSY_STATE;
		MOUNT_STATE_HOME_ITEM->light.value = INDIGO_BUSY_STATE;
		MOUNT_STATE_TRACKING_ITEM->light.value = INDIGO_IDLE_STATE;
		indigo_update_property(device, MOUNT_STATE_PROPERTY, NULL);
		indigo_cancel_pending_handler(device, position_handler);
		indigo_execute_handler(device, position_handler);
	} else {
		MOUNT_HOME_PROPERTY->state = INDIGO_OK_STATE;
		indigo_update_property(device, MOUNT_HOME_PROPERTY, NULL);
	}
}

static void mount_equatorial_coordinates_handler(indigo_device *device) {
	if (MOUNT_ON_COORDINATES_SET_SYNC_ITEM->sw.value) {
		MOUNT_EQUATORIAL_COORDINATES_RA_ITEM->number.value = MOUNT_EQUATORIAL_COORDINATES_RA_ITEM->number.target;
		MOUNT_EQUATORIAL_COORDINATES_DEC_ITEM->number.value = MOUNT_EQUATORIAL_COORDINATES_DEC_ITEM->number.target;
		MOUNT_RAW_COORDINATES_RA_ITEM->number.target = MOUNT_RAW_COORDINATES_RA_ITEM->number.value = MOUNT_EQUATORIAL_COORDINATES_RA_ITEM->number.value;
		MOUNT_RAW_COORDINATES_DEC_ITEM->number.target = MOUNT_RAW_COORDINATES_DEC_ITEM->number.value = MOUNT_EQUATORIAL_COORDINATES_DEC_ITEM->number.value;
		MOUNT_EQUATORIAL_COORDINATES_PROPERTY->state = MOUNT_RAW_COORDINATES_PROPERTY->state = INDIGO_OK_STATE;
		indigo_update_coordinates(device, NULL);
		indigo_update_property(device, MOUNT_RAW_COORDINATES_PROPERTY, NULL);
	} else if (MOUNT_ON_COORDINATES_SET_TRACK_ITEM->sw.value) {
		PRIVATE_DATA->at_home = false;
		indigo_translated_to_raw(device, MOUNT_EQUATORIAL_COORDINATES_RA_ITEM->number.target, MOUNT_EQUATORIAL_COORDINATES_DEC_ITEM->number.target, &MOUNT_RAW_COORDINATES_RA_ITEM->number.target, &MOUNT_RAW_COORDINATES_DEC_ITEM->number.target);
		MOUNT_EQUATORIAL_COORDINATES_PROPERTY->state = MOUNT_RAW_COORDINATES_PROPERTY->state = INDIGO_BUSY_STATE;
		time_t utc = indigo_get_mount_utc(device);
		double ra = MOUNT_EQUATORIAL_COORDINATES_RA_ITEM->number.target;
		double dec = MOUNT_EQUATORIAL_COORDINATES_DEC_ITEM->number.target;
		indigo_j2k_to_jnow(&ra, &dec);
		double alt, az;
		indigo_radec_to_altaz(ra, dec, &utc, MOUNT_GEOGRAPHIC_COORDINATES_LATITUDE_ITEM->number.value, MOUNT_GEOGRAPHIC_COORDINATES_LONGITUDE_ITEM->number.value, MOUNT_GEOGRAPHIC_COORDINATES_ELEVATION_ITEM->number.value, &alt, &az);
		bool west = az > 180;
		if (MOUNT_GEOGRAPHIC_COORDINATES_LATITUDE_ITEM->number.value < 0) {
			west = !west;
		}
		MOUNT_STATE_SLEW_ITEM->light.value = INDIGO_BUSY_STATE;
		MOUNT_STATE_HOME_ITEM->light.value = INDIGO_IDLE_STATE;
		indigo_update_property(device, MOUNT_STATE_PROPERTY, NULL);
		indigo_set_switch(MOUNT_SIDE_OF_PIER_PROPERTY, west ? MOUNT_SIDE_OF_PIER_WEST_ITEM : MOUNT_SIDE_OF_PIER_EAST_ITEM, true);
		MOUNT_SIDE_OF_PIER_PROPERTY->state = INDIGO_OK_STATE;
		indigo_update_property(device, MOUNT_SIDE_OF_PIER_PROPERTY, NULL);
		PRIVATE_DATA->slew_in_progress = true;
		indigo_update_coordinates(device, NULL);
		indigo_cancel_pending_handler(device, position_handler);
		indigo_execute_handler(device, position_handler);
	}
}

static void mount_motion_dec_handler(indigo_device *device) {
	MOUNT_MOTION_DEC_PROPERTY->state = INDIGO_OK_STATE;
	indigo_update_property(device, MOUNT_MOTION_DEC_PROPERTY, NULL);
	indigo_cancel_pending_handler(device, manual_motion_finalizer);
	indigo_execute_handler(device, manual_motion_finalizer);
}

static void mount_motion_ra_handler(indigo_device *device) {
	MOUNT_MOTION_RA_PROPERTY->state = INDIGO_OK_STATE;
	indigo_update_property(device, MOUNT_MOTION_RA_PROPERTY, NULL);
	indigo_cancel_pending_handler(device, manual_motion_finalizer);
	indigo_execute_handler(device, manual_motion_finalizer);
}

static void mount_abort_motion_handler(indigo_device *device) {
	bool manual_motion_in_progress = MOUNT_MOTION_NORTH_ITEM->sw.value || MOUNT_MOTION_SOUTH_ITEM->sw.value || MOUNT_MOTION_EAST_ITEM->sw.value || MOUNT_MOTION_WEST_ITEM->sw.value;
	bool coordinate_operation_pending = MOUNT_EQUATORIAL_COORDINATES_PROPERTY->state == INDIGO_BUSY_STATE;
	indigo_cancel_pending_handler(device, mount_park_handler);
	indigo_cancel_pending_handler(device, mount_home_handler);
	indigo_cancel_pending_handler(device, mount_equatorial_coordinates_handler);
	indigo_cancel_pending_handler(device, mount_motion_dec_handler);
	indigo_cancel_pending_handler(device, mount_motion_ra_handler);
	indigo_cancel_pending_handler(device, manual_motion_finalizer);
	if (PRIVATE_DATA->slew_in_progress || manual_motion_in_progress || coordinate_operation_pending) {
		PRIVATE_DATA->slew_in_progress = PRIVATE_DATA->parking = PRIVATE_DATA->going_home = false;
		MOUNT_RAW_COORDINATES_RA_ITEM->number.target = MOUNT_RAW_COORDINATES_RA_ITEM->number.value;
		MOUNT_RAW_COORDINATES_DEC_ITEM->number.target = MOUNT_RAW_COORDINATES_DEC_ITEM->number.value;
		MOUNT_EQUATORIAL_COORDINATES_PROPERTY->state = MOUNT_RAW_COORDINATES_PROPERTY->state = INDIGO_ALERT_STATE;
		indigo_update_coordinates(device, NULL);
		indigo_update_property(device, MOUNT_RAW_COORDINATES_PROPERTY, NULL);
	}
	if (MOUNT_PARK_PROPERTY->state == INDIGO_BUSY_STATE) {
		indigo_set_switch(MOUNT_PARK_PROPERTY, MOUNT_PARK_UNPARKED_ITEM, true);
		MOUNT_PARK_PROPERTY->state = INDIGO_ALERT_STATE;
		indigo_update_property(device, MOUNT_PARK_PROPERTY, NULL);
	}
	if (MOUNT_HOME_PROPERTY->state == INDIGO_BUSY_STATE) {
		MOUNT_HOME_ITEM->sw.value = false;
		MOUNT_HOME_PROPERTY->state = INDIGO_ALERT_STATE;
		indigo_update_property(device, MOUNT_HOME_PROPERTY, NULL);
	}
	MOUNT_MOTION_NORTH_ITEM->sw.value = MOUNT_MOTION_SOUTH_ITEM->sw.value = false;
	MOUNT_MOTION_EAST_ITEM->sw.value = MOUNT_MOTION_WEST_ITEM->sw.value = false;
	MOUNT_MOTION_DEC_PROPERTY->state = MOUNT_MOTION_RA_PROPERTY->state = INDIGO_OK_STATE;
	indigo_update_property(device, MOUNT_MOTION_DEC_PROPERTY, NULL);
	indigo_update_property(device, MOUNT_MOTION_RA_PROPERTY, NULL);
	MOUNT_STATE_SLEW_ITEM->light.value = INDIGO_IDLE_STATE;
	MOUNT_STATE_PARK_ITEM->light.value = PRIVATE_DATA->parked ? INDIGO_OK_STATE : INDIGO_IDLE_STATE;
	MOUNT_STATE_HOME_ITEM->light.value = PRIVATE_DATA->at_home ? INDIGO_OK_STATE : INDIGO_IDLE_STATE;
	indigo_update_property(device, MOUNT_STATE_PROPERTY, NULL);
	MOUNT_ABORT_MOTION_ITEM->sw.value = false;
	MOUNT_ABORT_MOTION_PROPERTY->state = INDIGO_OK_STATE;
	indigo_update_property(device, MOUNT_ABORT_MOTION_PROPERTY, "Aborted");
}

static void mount_tracking_handler(indigo_device *device) {
	time_t utc = indigo_get_mount_utc(device);
	PRIVATE_DATA->ha = indigo_lst(&utc, MOUNT_GEOGRAPHIC_COORDINATES_LONGITUDE_ITEM->number.value) - MOUNT_RAW_COORDINATES_RA_ITEM->number.value;
	MOUNT_STATE_TRACKING_ITEM->light.value = MOUNT_TRACKING_ON_ITEM->sw.value ? INDIGO_OK_STATE : INDIGO_IDLE_STATE;
	indigo_update_property(device, MOUNT_STATE_PROPERTY, NULL);
	MOUNT_TRACKING_PROPERTY->state = INDIGO_OK_STATE;
	indigo_update_property(device, MOUNT_TRACKING_PROPERTY, NULL);
}

static indigo_result mount_change_property(indigo_device *device, indigo_client *client, indigo_property *property) {
	assert(device != NULL);
	assert(DEVICE_CONTEXT != NULL);
	assert(property != NULL);
	if (indigo_property_match_changeable(CONNECTION_PROPERTY, property)) {
			// -------------------------------------------------------------------------------- CONNECTION
		if (indigo_ignore_connection_change(device, property)) {
			return INDIGO_OK;
		}
		indigo_property_copy_values(CONNECTION_PROPERTY, property, false);
		CONNECTION_PROPERTY->state = INDIGO_BUSY_STATE;
		indigo_update_property(device, CONNECTION_PROPERTY, NULL);
		indigo_execute_handler(device, mount_connection_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(MOUNT_PARK_PROPERTY, property)) {
			// -------------------------------------------------------------------------------- MOUNT_PARK
		INDIGO_COPY_VALUES_PROCESS_CHANGE(MOUNT_PARK_PROPERTY, mount_park_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(MOUNT_HOME_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- MOUNT_HOME
		INDIGO_COPY_VALUES_PROCESS_CHANGE(MOUNT_HOME_PROPERTY, mount_home_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(MOUNT_EQUATORIAL_COORDINATES_PROPERTY, property)) {
			// -------------------------------------------------------------------------------- MOUNT_EQUATORIAL_COORDINATES
		if (MOUNT_PARK_PARKED_ITEM->sw.value) {
			MOUNT_EQUATORIAL_COORDINATES_PROPERTY->state = INDIGO_ALERT_STATE;
			indigo_update_property(device, MOUNT_EQUATORIAL_COORDINATES_PROPERTY, "Mount is parked");
		} else if (MOUNT_ON_COORDINATES_SET_SYNC_ITEM->sw.value && !MOUNT_ALIGNMENT_MODE_CONTROLLER_ITEM->sw.value) {
			indigo_mount_change_property(device, client, property);
		} else {
			INDIGO_COPY_TARGETS_PROCESS_CHANGE(MOUNT_EQUATORIAL_COORDINATES_PROPERTY, mount_equatorial_coordinates_handler);
		}
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(MOUNT_MOTION_DEC_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- MOUNT_MOTION_DEC
		if (MOUNT_PARK_PARKED_ITEM->sw.value) {
			MOUNT_MOTION_DEC_PROPERTY->state = INDIGO_ALERT_STATE;
			indigo_update_property(device, MOUNT_MOTION_DEC_PROPERTY, "Mount is parked");
		} else {
			INDIGO_COPY_VALUES_PROCESS_CHANGE_ANYTIME(MOUNT_MOTION_DEC_PROPERTY, mount_motion_dec_handler);
		}
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(MOUNT_MOTION_RA_PROPERTY, property)) {
			// -------------------------------------------------------------------------------- MOUNT_MOTION_RA
		if (MOUNT_PARK_PARKED_ITEM->sw.value) {
			MOUNT_MOTION_RA_PROPERTY->state = INDIGO_ALERT_STATE;
			indigo_update_property(device, MOUNT_MOTION_RA_PROPERTY, "Mount is parked");
		} else {
			INDIGO_COPY_VALUES_PROCESS_CHANGE_ANYTIME(MOUNT_MOTION_RA_PROPERTY, mount_motion_ra_handler);
		}
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(MOUNT_ABORT_MOTION_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- MOUNT_ABORT_MOTION
		if (MOUNT_PARK_PARKED_ITEM->sw.value) {
			MOUNT_ABORT_MOTION_PROPERTY->state = INDIGO_ALERT_STATE;
			indigo_update_property(device, MOUNT_ABORT_MOTION_PROPERTY, "Mount is parked");
		} else {
			INDIGO_COPY_VALUES_PROCESS_URGENT_CHANGE(MOUNT_ABORT_MOTION_PROPERTY, mount_abort_motion_handler);
		}
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(MOUNT_TRACKING_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- MOUNT_TRACKING
		if (MOUNT_PARK_PARKED_ITEM->sw.value) {
			MOUNT_TRACKING_PROPERTY->state = INDIGO_ALERT_STATE;
			indigo_update_property(device, MOUNT_TRACKING_PROPERTY, "Mount is parked");
		} else {
			INDIGO_COPY_VALUES_PROCESS_CHANGE(MOUNT_TRACKING_PROPERTY, mount_tracking_handler);
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
		mount_connection_handler(device);
	}
	INDIGO_DEVICE_DETACH_LOG(DRIVER_NAME, device->name);
	return indigo_mount_detach(device);
}

	// -------------------------------------------------------------------------------- INDIGO guider device implementation

static void guider_guide_ra_finalizer(indigo_device *device) {
	if (GUIDER_GUIDE_EAST_ITEM->number.value != 0 || GUIDER_GUIDE_WEST_ITEM->number.value != 0) {
		GUIDER_GUIDE_EAST_ITEM->number.value = GUIDER_GUIDE_EAST_ITEM->number.target = 0;
		GUIDER_GUIDE_WEST_ITEM->number.value = GUIDER_GUIDE_WEST_ITEM->number.target = 0;
		GUIDER_GUIDE_RA_PROPERTY->state = INDIGO_OK_STATE;
		indigo_update_property(device, GUIDER_GUIDE_RA_PROPERTY, NULL);
	}
}

static void guider_guide_dec_finalizer(indigo_device *device) {
	if (GUIDER_GUIDE_NORTH_ITEM->number.value != 0 || GUIDER_GUIDE_SOUTH_ITEM->number.value != 0) {
		GUIDER_GUIDE_NORTH_ITEM->number.value = GUIDER_GUIDE_NORTH_ITEM->number.target = 0;
		GUIDER_GUIDE_SOUTH_ITEM->number.value = GUIDER_GUIDE_SOUTH_ITEM->number.target = 0;
		GUIDER_GUIDE_DEC_PROPERTY->state = INDIGO_OK_STATE;
		indigo_update_property(device, GUIDER_GUIDE_DEC_PROPERTY, NULL);
	}
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

static void guider_connection_handler(indigo_device *device) {
	CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
	if (!CONNECTION_CONNECTED_ITEM->sw.value) {
		indigo_cancel_pending_handlers(device);
		GUIDER_GUIDE_EAST_ITEM->number.value = GUIDER_GUIDE_EAST_ITEM->number.target = 0;
		GUIDER_GUIDE_WEST_ITEM->number.value = GUIDER_GUIDE_WEST_ITEM->number.target = 0;
		GUIDER_GUIDE_NORTH_ITEM->number.value = GUIDER_GUIDE_NORTH_ITEM->number.target = 0;
		GUIDER_GUIDE_SOUTH_ITEM->number.value = GUIDER_GUIDE_SOUTH_ITEM->number.target = 0;
		GUIDER_GUIDE_RA_PROPERTY->state = GUIDER_GUIDE_DEC_PROPERTY->state = INDIGO_OK_STATE;
	}
	indigo_guider_change_property(device, NULL, CONNECTION_PROPERTY);
}

static void guider_guide_ra_handler(indigo_device *device) {
	indigo_cancel_pending_handler(device, guider_guide_ra_finalizer);
	double duration = GUIDER_GUIDE_EAST_ITEM->number.value > 0 ? GUIDER_GUIDE_EAST_ITEM->number.value : GUIDER_GUIDE_WEST_ITEM->number.value;
	if (duration > 0) {
		GUIDER_GUIDE_RA_PROPERTY->state = INDIGO_BUSY_STATE;
		indigo_update_property(device, GUIDER_GUIDE_RA_PROPERTY, NULL);
		indigo_execute_priority_handler_in(device, INDIGO_TASK_PRIORITY_TIME, duration / 1000.0, guider_guide_ra_finalizer);
	} else {
		GUIDER_GUIDE_RA_PROPERTY->state = INDIGO_OK_STATE;
		indigo_update_property(device, GUIDER_GUIDE_RA_PROPERTY, NULL);
	}
}

static void guider_guide_dec_handler(indigo_device *device) {
	indigo_cancel_pending_handler(device, guider_guide_dec_finalizer);
	double duration = GUIDER_GUIDE_NORTH_ITEM->number.value > 0 ? GUIDER_GUIDE_NORTH_ITEM->number.value : GUIDER_GUIDE_SOUTH_ITEM->number.value;
	if (duration > 0) {
		GUIDER_GUIDE_DEC_PROPERTY->state = INDIGO_BUSY_STATE;
		indigo_update_property(device, GUIDER_GUIDE_DEC_PROPERTY, NULL);
		indigo_execute_priority_handler_in(device, INDIGO_TASK_PRIORITY_TIME, duration / 1000.0, guider_guide_dec_finalizer);
	} else {
		GUIDER_GUIDE_DEC_PROPERTY->state = INDIGO_OK_STATE;
		indigo_update_property(device, GUIDER_GUIDE_DEC_PROPERTY, NULL);
	}
}

static void guider_rate_handler(indigo_device *device) {
	GUIDER_RATE_PROPERTY->state = INDIGO_OK_STATE;
	indigo_update_property(device, GUIDER_RATE_PROPERTY, NULL);
}

static indigo_result guider_change_property(indigo_device *device, indigo_client *client, indigo_property *property) {
	assert(device != NULL);
	assert(DEVICE_CONTEXT != NULL);
	assert(property != NULL);
	if (indigo_property_match_changeable(CONNECTION_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- CONNECTION
		if (indigo_ignore_connection_change(device, property)) {
			return INDIGO_OK;
		}
		indigo_property_copy_values(CONNECTION_PROPERTY, property, false);
		CONNECTION_PROPERTY->state = INDIGO_BUSY_STATE;
		indigo_update_property(device, CONNECTION_PROPERTY, NULL);
		indigo_execute_handler(device, guider_connection_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(GUIDER_GUIDE_DEC_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- GUIDER_GUIDE_DEC
		GUIDER_GUIDE_NORTH_ITEM->number.value = GUIDER_GUIDE_NORTH_ITEM->number.target = 0;
		GUIDER_GUIDE_SOUTH_ITEM->number.value = GUIDER_GUIDE_SOUTH_ITEM->number.target = 0;
		indigo_property_copy_values(GUIDER_GUIDE_DEC_PROPERTY, property, false);
		GUIDER_GUIDE_DEC_PROPERTY->state = INDIGO_BUSY_STATE;
		indigo_update_property(device, GUIDER_GUIDE_DEC_PROPERTY, NULL);
		indigo_execute_priority_handler(device, INDIGO_TASK_PRIORITY_TIME, guider_guide_dec_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(GUIDER_GUIDE_RA_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- GUIDER_GUIDE_RA
		GUIDER_GUIDE_EAST_ITEM->number.value = GUIDER_GUIDE_EAST_ITEM->number.target = 0;
		GUIDER_GUIDE_WEST_ITEM->number.value = GUIDER_GUIDE_WEST_ITEM->number.target = 0;
		indigo_property_copy_values(GUIDER_GUIDE_RA_PROPERTY, property, false);
		GUIDER_GUIDE_RA_PROPERTY->state = INDIGO_BUSY_STATE;
		indigo_update_property(device, GUIDER_GUIDE_RA_PROPERTY, NULL);
		indigo_execute_priority_handler(device, INDIGO_TASK_PRIORITY_TIME, guider_guide_ra_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(GUIDER_RATE_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- GUIDER_RATE
		INDIGO_COPY_VALUES_PROCESS_CHANGE(GUIDER_RATE_PROPERTY, guider_rate_handler);
		return INDIGO_OK;
			// --------------------------------------------------------------------------------
	}
	return indigo_guider_change_property(device, client, property);
}

static indigo_result guider_detach(indigo_device *device) {
	assert(device != NULL);
	if (IS_CONNECTED) {
		indigo_set_switch(CONNECTION_PROPERTY, CONNECTION_DISCONNECTED_ITEM, true);
		guider_connection_handler(device);
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

	if (action == last_action) {
		return INDIGO_OK;
	}

	switch (action) {
		case INDIGO_DRIVER_INIT:
			last_action = action;
			private_data = indigo_safe_malloc(sizeof(simulator_private_data));
			mount = indigo_safe_malloc_copy(sizeof(indigo_device), &mount_template);
			mount->private_data = private_data;
			indigo_attach_device(mount);
			mount_guider = indigo_safe_malloc_copy(sizeof(indigo_device), &mount_guider_template);
			mount_guider->private_data = private_data;
			mount_guider->master_device = mount;
			indigo_attach_device(mount_guider);
			break;

		case INDIGO_DRIVER_SHUTDOWN:
			VERIFY_NOT_CONNECTED(mount);
			VERIFY_NOT_CONNECTED(mount_guider);
			last_action = action;
			if (mount_guider != NULL) {
				indigo_detach_device(mount_guider);
				indigo_safe_free(mount_guider);
				mount_guider = NULL;
			}
			if (mount != NULL) {
				indigo_detach_device(mount);
				indigo_safe_free(mount);
				mount = NULL;
			}
			if (private_data != NULL) {
				indigo_safe_free(private_data);
				private_data = NULL;
			}
			break;

		case INDIGO_DRIVER_INFO:
			break;
	}

	return INDIGO_OK;
}
