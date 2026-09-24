// Copyright (c) 2016-2026 CloudMakers, s. r. o.
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

// This file generated from indigo_mount_simulator.driver

#pragma mark - Includes

#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <assert.h>
#include <indigo/indigo_driver_xml.h>
#include <indigo/indigo_mount_driver.h>
#include <indigo/indigo_align.h>
#include <indigo/indigo_guider_driver.h>
#include <indigo/indigo_uni_io.h>

#include "indigo_mount_simulator.h"

#pragma mark - Common definitions

#define DRIVER_VERSION       0x03000011
#define DRIVER_NAME          "indigo_mount_simulator"
#define DRIVER_LABEL         "Mount Simulator"
#define MOUNT_DEVICE_NAME    DRIVER_LABEL
#define GUIDER_DEVICE_NAME   "Mount Simulator (guider)"
#define PRIVATE_DATA         ((simulator_private_data *)device->private_data)

#pragma mark - Private data definition

typedef struct {
	int count;
	//+ data
	bool parking, parked, going_home, at_home;
	double ha;
	bool slew_in_progress;
	//- data
} simulator_private_data;

#pragma mark - Low level code

//+ code

/* Device-specific queue handlers are emitted below. */

//- code

//+ mount.code

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
			} else if (MOUNT_TRACKING_OFF_ITEM->sw.value) {
				indigo_set_switch(MOUNT_TRACKING_PROPERTY, MOUNT_TRACKING_ON_ITEM, true);
				indigo_update_property(device, MOUNT_TRACKING_PROPERTY, NULL);
				MOUNT_STATE_TRACKING_ITEM->light.value = INDIGO_OK_STATE;
				indigo_update_property(device, MOUNT_STATE_PROPERTY, NULL);
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

//- mount.code

//+ guider.code

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

//- guider.code

#pragma mark - High level code (mount)

static void mount_connection_handler(indigo_device *device) {
	if (CONNECTION_CONNECTED_ITEM->sw.value) {
		//+ mount.on_connect
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
		//- mount.on_connect
		CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
		indigo_send_message(device, OK_PROPERTY, "Connected to %s", device->name);
	} else {
		indigo_cancel_pending_handlers(device);
		//+ mount.on_disconnect
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
		//- mount.on_disconnect
		indigo_send_message(device, OK_PROPERTY, "Disconnected from %s", device->name);
		CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
	}
	indigo_mount_change_property(device, NULL, CONNECTION_PROPERTY);
}

static void mount_park_handler(indigo_device *device) {
	//+ mount.MOUNT_PARK.on_change
	MOUNT_PARK_PROPERTY->state = INDIGO_BUSY_STATE;
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
	//- mount.MOUNT_PARK.on_change
	indigo_update_property(device, MOUNT_PARK_PROPERTY, NULL);
}

static void mount_home_handler(indigo_device *device) {
	//+ mount.MOUNT_HOME.on_change
	MOUNT_HOME_PROPERTY->state = INDIGO_BUSY_STATE;
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
	}
	//- mount.MOUNT_HOME.on_change
	indigo_update_property(device, MOUNT_HOME_PROPERTY, NULL);
}

static void mount_equatorial_coordinates_handler(indigo_device *device) {
	if (!MOUNT_PARK_PROPERTY->hidden && MOUNT_PARK_PARKED_ITEM->sw.value) {
		indigo_send_message(device, MOUNT_EQUATORIAL_COORDINATES_PROPERTY, "Mount is parked!");
		INDIGO_UPDATE_PROPERTY_STATE(MOUNT_EQUATORIAL_COORDINATES_PROPERTY, INDIGO_ALERT_STATE, NULL);
		return;
	}
	MOUNT_EQUATORIAL_COORDINATES_PROPERTY->state = INDIGO_OK_STATE;
	//+ mount.MOUNT_EQUATORIAL_COORDINATES.on_change
	if (MOUNT_ON_COORDINATES_SET_SYNC_ITEM->sw.value) {
		MOUNT_EQUATORIAL_COORDINATES_RA_ITEM->number.value = MOUNT_EQUATORIAL_COORDINATES_RA_ITEM->number.target;
		MOUNT_EQUATORIAL_COORDINATES_DEC_ITEM->number.value = MOUNT_EQUATORIAL_COORDINATES_DEC_ITEM->number.target;
		MOUNT_RAW_COORDINATES_RA_ITEM->number.target = MOUNT_RAW_COORDINATES_RA_ITEM->number.value = MOUNT_EQUATORIAL_COORDINATES_RA_ITEM->number.value;
		MOUNT_RAW_COORDINATES_DEC_ITEM->number.target = MOUNT_RAW_COORDINATES_DEC_ITEM->number.value = MOUNT_EQUATORIAL_COORDINATES_DEC_ITEM->number.value;
		MOUNT_EQUATORIAL_COORDINATES_PROPERTY->state = MOUNT_RAW_COORDINATES_PROPERTY->state = INDIGO_OK_STATE;
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
		// MOUNT_SIDE_OF_PIER is the side of the pier the OTA is on: a target in the western sky is reached from the east side
		bool west = az <= 180;
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
		indigo_cancel_pending_handler(device, position_handler);
		indigo_execute_handler(device, position_handler);
	}
	//- mount.MOUNT_EQUATORIAL_COORDINATES.on_change
	indigo_update_coordinates(device, NULL);
}

static void mount_motion_dec_handler(indigo_device *device) {
	if (!MOUNT_PARK_PROPERTY->hidden && MOUNT_PARK_PARKED_ITEM->sw.value) {
		indigo_send_message(device, MOUNT_MOTION_DEC_PROPERTY, "Mount is parked!");
		INDIGO_UPDATE_PROPERTY_STATE(MOUNT_MOTION_DEC_PROPERTY, INDIGO_ALERT_STATE, NULL);
		return;
	}
	//+ mount.MOUNT_MOTION_DEC.on_change
	MOUNT_MOTION_DEC_PROPERTY->state = INDIGO_OK_STATE;
	indigo_update_property(device, MOUNT_MOTION_DEC_PROPERTY, NULL);
	indigo_cancel_pending_handler(device, manual_motion_finalizer);
	indigo_execute_handler(device, manual_motion_finalizer);
	//- mount.MOUNT_MOTION_DEC.on_change
}

static void mount_motion_ra_handler(indigo_device *device) {
	if (!MOUNT_PARK_PROPERTY->hidden && MOUNT_PARK_PARKED_ITEM->sw.value) {
		indigo_send_message(device, MOUNT_MOTION_RA_PROPERTY, "Mount is parked!");
		INDIGO_UPDATE_PROPERTY_STATE(MOUNT_MOTION_RA_PROPERTY, INDIGO_ALERT_STATE, NULL);
		return;
	}
	//+ mount.MOUNT_MOTION_RA.on_change
	MOUNT_MOTION_RA_PROPERTY->state = INDIGO_OK_STATE;
	indigo_update_property(device, MOUNT_MOTION_RA_PROPERTY, NULL);
	indigo_cancel_pending_handler(device, manual_motion_finalizer);
	indigo_execute_handler(device, manual_motion_finalizer);
	//- mount.MOUNT_MOTION_RA.on_change
}

static void mount_abort_motion_handler(indigo_device *device) {
	//+ mount.MOUNT_ABORT_MOTION.on_change
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
	//- mount.MOUNT_ABORT_MOTION.on_change
}

static void mount_tracking_handler(indigo_device *device) {
	if (!MOUNT_PARK_PROPERTY->hidden && MOUNT_PARK_PARKED_ITEM->sw.value) {
		indigo_send_message(device, MOUNT_TRACKING_PROPERTY, "Mount is parked!");
		INDIGO_UPDATE_PROPERTY_STATE(MOUNT_TRACKING_PROPERTY, INDIGO_ALERT_STATE, NULL);
		return;
	}
	MOUNT_TRACKING_PROPERTY->state = INDIGO_OK_STATE;
	//+ mount.MOUNT_TRACKING.on_change
	time_t utc = indigo_get_mount_utc(device);
	PRIVATE_DATA->ha = indigo_lst(&utc, MOUNT_GEOGRAPHIC_COORDINATES_LONGITUDE_ITEM->number.value) - MOUNT_RAW_COORDINATES_RA_ITEM->number.value;
	MOUNT_STATE_TRACKING_ITEM->light.value = MOUNT_TRACKING_ON_ITEM->sw.value ? INDIGO_OK_STATE : INDIGO_IDLE_STATE;
	indigo_update_property(device, MOUNT_STATE_PROPERTY, NULL);
	//- mount.MOUNT_TRACKING.on_change
	indigo_update_property(device, MOUNT_TRACKING_PROPERTY, NULL);
}

#pragma mark - Device API (mount)

static indigo_result mount_enumerate_properties(indigo_device *device, indigo_client *client, indigo_property *property);

static indigo_result mount_attach(indigo_device *device) {
	if (indigo_mount_attach(device, DRIVER_NAME, DRIVER_VERSION) == INDIGO_OK) {
		ADDITIONAL_INSTANCES_PROPERTY->hidden = device->base_device != NULL;
		//+ mount.on_attach
		SIMULATION_PROPERTY->hidden = false;
		SIMULATION_PROPERTY->perm = INDIGO_RO_PERM;
		SIMULATION_ENABLED_ITEM->sw.value = true;
		SIMULATION_DISABLED_ITEM->sw.value = false;
		DEVICE_PORT_PROPERTY->hidden = true;
		MOUNT_STATE_PROPERTY->hidden = false;
		MOUNT_PARK_SET_PROPERTY->hidden = false;
		MOUNT_PARK_POSITION_PROPERTY->hidden = false;
		PRIVATE_DATA->parked = true;
		MOUNT_HOME_SET_PROPERTY->hidden = false;
		MOUNT_HOME_POSITION_PROPERTY->hidden = false;
		MOUNT_HOME_PROPERTY->hidden = false;
		MOUNT_SIDE_OF_PIER_PROPERTY->hidden = false;
		MOUNT_EPOCH_PROPERTY->perm = INDIGO_RO_PERM;
		MOUNT_ON_COORDINATES_SET_PROPERTY->count = 2;
		MOUNT_CUSTOM_TRACKING_RATE_PROPERTY->hidden = false;
		MOUNT_RAW_COORDINATES_RA_ITEM->number.value = MOUNT_RAW_COORDINATES_RA_ITEM->number.target = 0;
		MOUNT_RAW_COORDINATES_DEC_ITEM->number.value = MOUNT_RAW_COORDINATES_DEC_ITEM->number.target = 90;
		MOUNT_EQUATORIAL_COORDINATES_RA_ITEM->number.value = MOUNT_EQUATORIAL_COORDINATES_RA_ITEM->number.target = MOUNT_RAW_COORDINATES_RA_ITEM->number.value;
		MOUNT_EQUATORIAL_COORDINATES_DEC_ITEM->number.value = MOUNT_EQUATORIAL_COORDINATES_DEC_ITEM->number.target = MOUNT_RAW_COORDINATES_DEC_ITEM->number.value;
		MOUNT_ALIGNMENT_MODE_PROPERTY->hidden = false;
		MOUNT_TRACK_RATE_PROPERTY->count = 5;
		indigo_set_switch(MOUNT_TRACKING_PROPERTY, MOUNT_TRACKING_OFF_ITEM, true);
		AUTHENTICATION_PROPERTY->hidden = false;
		AUTHENTICATION_PROPERTY->count = 1;
		//- mount.on_attach
		MOUNT_PARK_PROPERTY->hidden = false;
		MOUNT_HOME_PROPERTY->hidden = false;
		MOUNT_EQUATORIAL_COORDINATES_PROPERTY->hidden = false;
		MOUNT_MOTION_DEC_PROPERTY->hidden = false;
		MOUNT_MOTION_RA_PROPERTY->hidden = false;
		MOUNT_ABORT_MOTION_PROPERTY->hidden = false;
		MOUNT_TRACKING_PROPERTY->hidden = false;
		INDIGO_DEVICE_ATTACH_LOG(DRIVER_NAME, device->name);
		return mount_enumerate_properties(device, NULL, NULL);
	}
	return INDIGO_FAILED;
}

static indigo_result mount_enumerate_properties(indigo_device *device, indigo_client *client, indigo_property *property) {
	return indigo_mount_enumerate_properties(device, client, property);
}

static indigo_result mount_change_property(indigo_device *device, indigo_client *client, indigo_property *property) {
	if (indigo_property_match_changeable(CONNECTION_PROPERTY, property)) {
		INDIGO_PROCESS_CONNECT(mount_connection_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(MOUNT_PARK_PROPERTY, property)) {
		INDIGO_COPY_VALUES_PROCESS_CHANGE(MOUNT_PARK_PROPERTY, mount_park_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(MOUNT_HOME_PROPERTY, property)) {
		INDIGO_COPY_VALUES_PROCESS_CHANGE(MOUNT_HOME_PROPERTY, mount_home_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(MOUNT_EQUATORIAL_COORDINATES_PROPERTY, property)) {
		INDIGO_REJECT_CHANGE_IF(!MOUNT_PARK_PROPERTY->hidden && MOUNT_PARK_PARKED_ITEM->sw.value, MOUNT_EQUATORIAL_COORDINATES_PROPERTY, "Mount is parked!");
		//+ mount.MOUNT_EQUATORIAL_COORDINATES.on_change_request
		if (!MOUNT_PARK_PARKED_ITEM->sw.value && MOUNT_ON_COORDINATES_SET_SYNC_ITEM->sw.value && !MOUNT_ALIGNMENT_MODE_CONTROLLER_ITEM->sw.value) {
			return indigo_mount_change_property(device, client, property);
		}
		//- mount.MOUNT_EQUATORIAL_COORDINATES.on_change_request
		INDIGO_COPY_TARGETS_PROCESS_CHANGE(MOUNT_EQUATORIAL_COORDINATES_PROPERTY, mount_equatorial_coordinates_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(MOUNT_MOTION_DEC_PROPERTY, property)) {
		INDIGO_REJECT_CHANGE_IF(!MOUNT_PARK_PROPERTY->hidden && MOUNT_PARK_PARKED_ITEM->sw.value, MOUNT_MOTION_DEC_PROPERTY, "Mount is parked!");
		INDIGO_COPY_VALUES_PROCESS_CHANGE_ANYTIME(MOUNT_MOTION_DEC_PROPERTY, mount_motion_dec_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(MOUNT_MOTION_RA_PROPERTY, property)) {
		INDIGO_REJECT_CHANGE_IF(!MOUNT_PARK_PROPERTY->hidden && MOUNT_PARK_PARKED_ITEM->sw.value, MOUNT_MOTION_RA_PROPERTY, "Mount is parked!");
		INDIGO_COPY_VALUES_PROCESS_CHANGE_ANYTIME(MOUNT_MOTION_RA_PROPERTY, mount_motion_ra_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(MOUNT_ABORT_MOTION_PROPERTY, property)) {
		INDIGO_REJECT_CHANGE_IF(MOUNT_PARK_PARKED_ITEM->sw.value, MOUNT_ABORT_MOTION_PROPERTY, "Mount is parked");
		INDIGO_COPY_VALUES_PROCESS_URGENT_CHANGE(MOUNT_ABORT_MOTION_PROPERTY, mount_abort_motion_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(MOUNT_TRACKING_PROPERTY, property)) {
		INDIGO_REJECT_CHANGE_IF(!MOUNT_PARK_PROPERTY->hidden && MOUNT_PARK_PARKED_ITEM->sw.value, MOUNT_TRACKING_PROPERTY, "Mount is parked!");
		INDIGO_COPY_VALUES_PROCESS_CHANGE(MOUNT_TRACKING_PROPERTY, mount_tracking_handler);
		return INDIGO_OK;
	}
	return indigo_mount_change_property(device, client, property);
}

static indigo_result mount_detach(indigo_device *device) {
	if (IS_CONNECTED) {
		indigo_set_switch(CONNECTION_PROPERTY, CONNECTION_DISCONNECTED_ITEM, true);
		mount_connection_handler(device);
	}
	INDIGO_DEVICE_DETACH_LOG(DRIVER_NAME, device->name);
	return indigo_mount_detach(device);
}

#pragma mark - High level code (guider)

static void guider_connection_handler(indigo_device *device) {
	if (CONNECTION_CONNECTED_ITEM->sw.value) {
		CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
		indigo_send_message(device, OK_PROPERTY, "Connected to %s", device->name);
	} else {
		indigo_cancel_pending_handlers(device);
		//+ guider.on_disconnect
		GUIDER_GUIDE_EAST_ITEM->number.value = GUIDER_GUIDE_EAST_ITEM->number.target = 0;
		GUIDER_GUIDE_WEST_ITEM->number.value = GUIDER_GUIDE_WEST_ITEM->number.target = 0;
		GUIDER_GUIDE_NORTH_ITEM->number.value = GUIDER_GUIDE_NORTH_ITEM->number.target = 0;
		GUIDER_GUIDE_SOUTH_ITEM->number.value = GUIDER_GUIDE_SOUTH_ITEM->number.target = 0;
		GUIDER_GUIDE_RA_PROPERTY->state = GUIDER_GUIDE_DEC_PROPERTY->state = INDIGO_OK_STATE;
		//- guider.on_disconnect
		indigo_send_message(device, OK_PROPERTY, "Disconnected from %s", device->name);
		CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
	}
	indigo_guider_change_property(device, NULL, CONNECTION_PROPERTY);
}

static void guider_guide_dec_handler(indigo_device *device) {
	//+ guider.GUIDER_GUIDE_DEC.on_change
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
	//- guider.GUIDER_GUIDE_DEC.on_change
}

static void guider_guide_ra_handler(indigo_device *device) {
	//+ guider.GUIDER_GUIDE_RA.on_change
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
	//- guider.GUIDER_GUIDE_RA.on_change
}

static void guider_rate_handler(indigo_device *device) {
	//+ guider.GUIDER_RATE.on_change
	GUIDER_RATE_PROPERTY->state = INDIGO_OK_STATE;
	//- guider.GUIDER_RATE.on_change
	indigo_update_property(device, GUIDER_RATE_PROPERTY, NULL);
}

#pragma mark - Device API (guider)

static indigo_result guider_enumerate_properties(indigo_device *device, indigo_client *client, indigo_property *property);

static indigo_result guider_attach(indigo_device *device) {
	if (indigo_guider_attach(device, DRIVER_NAME, DRIVER_VERSION) == INDIGO_OK) {
		GUIDER_GUIDE_DEC_PROPERTY->hidden = false;
		GUIDER_GUIDE_RA_PROPERTY->hidden = false;
		GUIDER_RATE_PROPERTY->hidden = false;
		//+ guider.GUIDER_RATE.on_attach
		GUIDER_RATE_PROPERTY->count = 2;
		//- guider.GUIDER_RATE.on_attach
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
		INDIGO_PROCESS_CONNECT(guider_connection_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(GUIDER_GUIDE_DEC_PROPERTY, property)) {
		//+ guider.GUIDER_GUIDE_DEC.on_change_request
		GUIDER_GUIDE_NORTH_ITEM->number.value = GUIDER_GUIDE_NORTH_ITEM->number.target = 0;
		GUIDER_GUIDE_SOUTH_ITEM->number.value = GUIDER_GUIDE_SOUTH_ITEM->number.target = 0;
		//- guider.GUIDER_GUIDE_DEC.on_change_request
		INDIGO_COPY_VALUES_PROCESS_PRIORITY_CHANGE_ANYTIME(GUIDER_GUIDE_DEC_PROPERTY, guider_guide_dec_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(GUIDER_GUIDE_RA_PROPERTY, property)) {
		//+ guider.GUIDER_GUIDE_RA.on_change_request
		GUIDER_GUIDE_EAST_ITEM->number.value = GUIDER_GUIDE_EAST_ITEM->number.target = 0;
		GUIDER_GUIDE_WEST_ITEM->number.value = GUIDER_GUIDE_WEST_ITEM->number.target = 0;
		//- guider.GUIDER_GUIDE_RA.on_change_request
		INDIGO_COPY_VALUES_PROCESS_PRIORITY_CHANGE_ANYTIME(GUIDER_GUIDE_RA_PROPERTY, guider_guide_ra_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(GUIDER_RATE_PROPERTY, property)) {
		INDIGO_COPY_VALUES_PROCESS_CHANGE(GUIDER_RATE_PROPERTY, guider_rate_handler);
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

static indigo_device mount_template = INDIGO_DEVICE_INITIALIZER(MOUNT_DEVICE_NAME, mount_attach, mount_enumerate_properties, mount_change_property, NULL, mount_detach);

static indigo_device guider_template = INDIGO_DEVICE_INITIALIZER(GUIDER_DEVICE_NAME, guider_attach, guider_enumerate_properties, guider_change_property, NULL, guider_detach);

#pragma mark - Main code

indigo_result indigo_mount_simulator(indigo_driver_action action, indigo_driver_info *info) {
	static indigo_driver_action last_action = INDIGO_DRIVER_SHUTDOWN;
	static simulator_private_data *private_data = NULL;
	static indigo_device *mount = NULL;
	static indigo_device *guider = NULL;

	SET_DRIVER_INFO(info, DRIVER_LABEL, __FUNCTION__, DRIVER_VERSION, false, last_action);

	if (action == last_action) {
		return INDIGO_OK;
	}

	switch (action) {
		case INDIGO_DRIVER_INIT: {
			last_action = action;
			private_data = (simulator_private_data *)indigo_safe_malloc(sizeof(simulator_private_data));
			mount = (indigo_device *)indigo_safe_malloc_copy(sizeof(indigo_device), &mount_template);
			mount->private_data = private_data;
			indigo_attach_device(mount);
			guider = (indigo_device *)indigo_safe_malloc_copy(sizeof(indigo_device), &guider_template);
			guider->private_data = private_data;
			guider->master_device = mount;
			indigo_attach_device(guider);
			break;

		}
		case INDIGO_DRIVER_SHUTDOWN: {
			VERIFY_NOT_CONNECTED(mount);
			VERIFY_NOT_CONNECTED(guider);
			last_action = action;
			if (guider != NULL) {
				indigo_detach_device(guider);
				indigo_safe_free(guider);
				guider = NULL;
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

		}
		case INDIGO_DRIVER_INFO:
			break;
	}

	return INDIGO_OK;
}
