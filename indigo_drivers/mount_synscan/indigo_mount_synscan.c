// Copyright (c) 2018 David Hulse
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
// 2.0 by David Hulse

/** INDIGO MOUNT SynScan (skywatcher) driver
 \file indigo_mount_synscan.c
 */

#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <math.h>
#include <assert.h>

#include <indigo/indigo_driver_xml.h>
#include <indigo/indigo_io.h>
#include <indigo/indigo_novas.h>

#include "indigo_mount_synscan.h"
#include "indigo_mount_synscan.h"
#include "indigo_mount_synscan_private.h"
#include "indigo_mount_synscan_mount.h"
#include "indigo_mount_synscan_guider.h"

//#define COMMAND_GUIDE_RATE_PROPERTY     (PRIVATE_DATA->command_guide_rate_property)
//#define GUIDE_50_ITEM                   (COMMAND_GUIDE_RATE_PROPERTY->items+0)
//#define GUIDE_100_ITEM                  (COMMAND_GUIDE_RATE_PROPERTY->items+1)
//
//#define COMMAND_GUIDE_RATE_PROPERTY_NAME   "COMMAND_GUIDE_RATE"
//#define GUIDE_50_ITEM_NAME                 "GUIDE_50"
//#define GUIDE_100_ITEM_NAME                "GUIDE_100"

#define WARN_PARKED_MSG                    "Mount is parked, please unpark!"
#define WARN_PARKING_PROGRESS_MSG          "Mount parking is in progress, please wait until complete!"

// gp_bits is used as boolean
#define is_connected                   gp_bits

// -------------------------------------------------------------------------------- INDIGO MOUNT device implementation

static indigo_result mount_attach(indigo_device *device) {
	assert(device != NULL);
	assert(PRIVATE_DATA != NULL);
	if (indigo_mount_attach(device, DRIVER_NAME, DRIVER_VERSION) == INDIGO_OK) {
		// -------------------------------------------------------------------------------- MOUNT_PARK
		MOUNT_PARK_PARKED_ITEM->sw.value = false;
		MOUNT_PARK_UNPARKED_ITEM->sw.value = true;
		// -------------------------------------------------------------------------------- MOUNT_PARK_SET
		MOUNT_PARK_SET_PROPERTY->hidden = false;
		// -------------------------------------------------------------------------------- MOUNT_PARK_POSITION
		MOUNT_PARK_POSITION_PROPERTY->hidden = false;
		// -------------------------------------------------------------------------------- MOUNT_HOME
		MOUNT_HOME_PROPERTY->hidden = false;
		// -------------------------------------------------------------------------------- MOUNT_HOME_SET
		MOUNT_HOME_SET_PROPERTY->hidden = false;
		// -------------------------------------------------------------------------------- MOUNT_HOME_POSITION
		MOUNT_HOME_POSITION_PROPERTY->hidden = false;
		// -------------------------------------------------------------------------------- MOUNT_TRACKING
		MOUNT_TRACKING_ON_ITEM->sw.value = false;
		MOUNT_TRACKING_OFF_ITEM->sw.value = true;
		// -------------------------------------------------------------------------------- MOUNT_GUIDE_RATE
		strncpy(MOUNT_GUIDE_RATE_PROPERTY->label,"ST4 guide rate", INDIGO_VALUE_SIZE);
		// -------------------------------------------------------------------------------- MOUNT_RAW_COORDINATES
		MOUNT_RAW_COORDINATES_PROPERTY->hidden = false;
		// -------------------------------------------------------------------------------- DEVICE_PORTS
		DEVICE_PORTS_PROPERTY->hidden = false;
		// -------------------------------------------------------------------------------- DEVICE_PORT
		DEVICE_PORT_PROPERTY->hidden = false;
		// -------------------------------------------------------------------------------- DEVICE_BAUDRATE
		DEVICE_BAUDRATE_PROPERTY->hidden = false;
		// -------------------------------------------------------------------------------- MOUNT_ALIGNMENT_MODE
		indigo_set_switch(MOUNT_ALIGNMENT_MODE_PROPERTY, MOUNT_ALIGNMENT_MODE_NEAREST_POINT_ITEM, true);
		MOUNT_ALIGNMENT_MODE_PROPERTY->hidden = false;
		MOUNT_ALIGNMENT_MODE_PROPERTY->count = 2;
		MOUNT_ALIGNMENT_SELECT_POINTS_PROPERTY->hidden = false;
		MOUNT_ALIGNMENT_SELECT_POINTS_PROPERTY->rule = INDIGO_ANY_OF_MANY_RULE;
		MOUNT_ALIGNMENT_DELETE_POINTS_PROPERTY->hidden = false;
		// -------------------------------------------------------------------------------- MOUNT_EPOCH
		MOUNT_EPOCH_ITEM->number.value = 0;
		// -------------------------------------------------------------------------------- MOUNT_SIDE_OF_PIER
		MOUNT_SIDE_OF_PIER_PROPERTY->hidden = false;
		MOUNT_SIDE_OF_PIER_PROPERTY->perm = INDIGO_RO_PERM;
		// -------------------------------------------------------------------------------- MOUNT_POLARSCOPE
		MOUNT_POLARSCOPE_PROPERTY = indigo_init_number_property(NULL, device->name, MOUNT_POLARSCOPE_PROPERTY_NAME, MOUNT_MAIN_GROUP, "Polarscope", INDIGO_OK_STATE, INDIGO_RW_PERM, 1);
		if (MOUNT_POLARSCOPE_PROPERTY == NULL)
			return INDIGO_FAILED;
		MOUNT_POLARSCOPE_PROPERTY->hidden = true;
		indigo_init_number_item(MOUNT_POLARSCOPE_BRIGHTNESS_ITEM, MOUNT_POLARSCOPE_BRIGHTNESS_ITEM_NAME, "Polarscope Brightness", 0, 255, 0, 0);
		// -------------------------------------------------------------------------------- MOUNT_OPERATING_MODE
		MOUNT_OPERATING_MODE_PROPERTY = indigo_init_switch_property(NULL, device->name, MOUNT_OPERATING_MODE_PROPERTY_NAME, MOUNT_MAIN_GROUP, "Operating mode", INDIGO_OK_STATE, INDIGO_RW_PERM, INDIGO_ONE_OF_MANY_RULE, 2);
		if (MOUNT_OPERATING_MODE_PROPERTY == NULL)
			return INDIGO_FAILED;
		MOUNT_OPERATING_MODE_PROPERTY->hidden = true;
		indigo_init_switch_item(POLAR_MODE_ITEM, POLAR_MODE_ITEM_NAME, "Polar mode", true);
		indigo_init_switch_item(ALTAZ_MODE_ITEM, ALTAZ_MODE_ITEM_NAME, "Alt/Az mode", false);

		// -------------------------------------------------------------------------------- MOUNT_USE_ENCODERS
		MOUNT_USE_ENCODERS_PROPERTY = indigo_init_switch_property(NULL, device->name, MOUNT_USE_ENCODERS_PROPERTY_NAME, MOUNT_MAIN_GROUP, "Use encoders", INDIGO_OK_STATE, INDIGO_RW_PERM, INDIGO_ANY_OF_MANY_RULE, 2);
		if (MOUNT_USE_ENCODERS_PROPERTY == NULL)
			return INDIGO_FAILED;
		MOUNT_USE_ENCODERS_PROPERTY->hidden = true;
		indigo_init_switch_item(MOUNT_USE_RA_ENCODER_ITEM, MOUNT_USE_RA_ENCODER_ITEM_NAME, "Use RA encoder", false);
		indigo_init_switch_item(MOUNT_USE_DEC_ENCODER_ITEM, MOUNT_USE_DEC_ENCODER_ITEM_NAME, "Use Dec encoder", false);

		// -------------------------------------------------------------------------------- MOUNT_AUTOHOME
		MOUNT_AUTOHOME_PROPERTY = indigo_init_switch_property(NULL, device->name, MOUNT_AUTOHOME_PROPERTY_NAME, MOUNT_MAIN_GROUP, "Auto home", INDIGO_OK_STATE, INDIGO_RW_PERM, INDIGO_ANY_OF_MANY_RULE, 1);
		if (MOUNT_AUTOHOME_PROPERTY == NULL)
			return INDIGO_FAILED;
		MOUNT_AUTOHOME_PROPERTY->hidden = true;
		indigo_init_switch_item(MOUNT_AUTOHOME_ITEM, MOUNT_AUTOHOME_ITEM_NAME, "Start auto home procedure", false);
		// -------------------------------------------------------------------------------- MOUNT_POLARSCOPE
		MOUNT_AUTOHOME_SETTINGS_PROPERTY = indigo_init_number_property(NULL, device->name, MOUNT_AUTOHOME_SETTINGS_PROPERTY_NAME, MOUNT_MAIN_GROUP, "Auto home settings", INDIGO_OK_STATE, INDIGO_RW_PERM, 1);
		if (MOUNT_AUTOHOME_SETTINGS_PROPERTY == NULL)
			return INDIGO_FAILED;
		MOUNT_AUTOHOME_SETTINGS_PROPERTY->hidden = true;
		indigo_init_number_item(MOUNT_AUTOHOME_DEC_OFFSET_ITEM, MOUNT_AUTOHOME_DEC_OFFSET_ITEM_NAME, "Dec offset", -90, 90, 0, 0);

		//  FURTHER INITIALISATION
		pthread_mutex_init(&PRIVATE_DATA->port_mutex, NULL);
		pthread_mutex_init(&PRIVATE_DATA->driver_mutex, NULL);
		PRIVATE_DATA->mountConfigured = false;
		ADDITIONAL_INSTANCES_PROPERTY->hidden = DEVICE_CONTEXT->base_device != NULL;
		INDIGO_DEVICE_ATTACH_LOG(DRIVER_NAME, device->name);
		
		return indigo_mount_enumerate_properties(device, NULL, NULL);
	}
	return INDIGO_FAILED;
}

//  This gets called when each client attaches to discover device properties
static indigo_result mount_enumerate_properties(indigo_device *device, indigo_client *client, indigo_property *property) {
	assert(device != NULL);
	assert(DEVICE_CONTEXT != NULL);
	if (IS_CONNECTED) {
		if (indigo_property_match(MOUNT_POLARSCOPE_PROPERTY, property))
			indigo_define_property(device, MOUNT_POLARSCOPE_PROPERTY, NULL);
		if (indigo_property_match(MOUNT_OPERATING_MODE_PROPERTY, property))
			indigo_define_property(device, MOUNT_OPERATING_MODE_PROPERTY, NULL);
		if (indigo_property_match(MOUNT_USE_ENCODERS_PROPERTY, property))
			indigo_define_property(device, MOUNT_USE_ENCODERS_PROPERTY, NULL);
		if (indigo_property_match(MOUNT_AUTOHOME_PROPERTY, property))
			indigo_define_property(device, MOUNT_AUTOHOME_PROPERTY, NULL);
		if (indigo_property_match(MOUNT_AUTOHOME_SETTINGS_PROPERTY, property))
			indigo_define_property(device, MOUNT_AUTOHOME_SETTINGS_PROPERTY, NULL);
	}
	return indigo_mount_enumerate_properties(device, client, property);
}

static indigo_result mount_change_property(indigo_device *device, indigo_client *client, indigo_property *property) {
	assert(device != NULL);
	assert(DEVICE_CONTEXT != NULL);
	assert(property != NULL);

	if (indigo_property_match(CONNECTION_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- CONNECTION
		//  This gets invoked from the network threads receiving the change requests, and so we may get more than one overlapping.
		//  This will be a problem since we are updating shared state. Last request wins the race and the preceding requests will see
		//  changing data and might experience issues.

		if (indigo_ignore_connection_change(device, property))
			return INDIGO_OK;
		indigo_property_copy_values(CONNECTION_PROPERTY, property, false);
		CONNECTION_PROPERTY->state = INDIGO_BUSY_STATE;
		indigo_update_property(device, CONNECTION_PROPERTY, NULL);

		//  Talk to the mount - see if its there - read configuration data
		//  Once we have it all, update the property to OK or ALERT state
		//  This can be done by a background timer thread
		return synscan_mount_connect(device);
	} else if (indigo_property_match(MOUNT_PARK_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- MOUNT_PARK
		indigo_property_copy_values(MOUNT_PARK_PROPERTY, property, false);
		mount_handle_park(device);
		return INDIGO_OK;
	} else if (indigo_property_match(MOUNT_HOME_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- MOUNT_HOME
		indigo_property_copy_values(MOUNT_HOME_PROPERTY, property, false);
		mount_handle_home(device);
		return INDIGO_OK;
	} else if (indigo_property_match(MOUNT_EQUATORIAL_COORDINATES_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- MOUNT_EQUATORIAL_COORDINATES
		if (IS_CONNECTED) {
			if (MOUNT_PARK_PARKED_ITEM->sw.value) {
				MOUNT_EQUATORIAL_COORDINATES_PROPERTY->state = INDIGO_ALERT_STATE;
				indigo_update_property(device, MOUNT_EQUATORIAL_COORDINATES_PROPERTY, "Mount is parked.");
			} else if (MOUNT_ON_COORDINATES_SET_SYNC_ITEM->sw.value) {
				if (PRIVATE_DATA->globalMode == kGlobalModeIdle) {
					//  Pass sync requests through to indigo common code
					return indigo_mount_change_property(device, client, property);
				} else {
					MOUNT_EQUATORIAL_COORDINATES_PROPERTY->state = INDIGO_ALERT_STATE;
					indigo_update_coordinates(device, "Sync not performed - mount is busy.");
					return INDIGO_OK;
				}
			} else {
				if (PRIVATE_DATA->globalMode == kGlobalModeIdle) {
					double ra = MOUNT_EQUATORIAL_COORDINATES_RA_ITEM->number.value;
					double dec = MOUNT_EQUATORIAL_COORDINATES_DEC_ITEM->number.value;
					indigo_property_copy_values(MOUNT_EQUATORIAL_COORDINATES_PROPERTY, property, false);
					MOUNT_EQUATORIAL_COORDINATES_RA_ITEM->number.value = ra;
					MOUNT_EQUATORIAL_COORDINATES_DEC_ITEM->number.value = dec;
					mount_handle_coordinates(device);
				} else {
					MOUNT_EQUATORIAL_COORDINATES_PROPERTY->state = INDIGO_ALERT_STATE;
					indigo_update_coordinates(device, "Slew not started - mount is busy.");
				}
				return INDIGO_OK;
			}
		}
#if 0
	} else if (indigo_property_match(MOUNT_HORIZONTAL_COORDINATES_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- MOUNT_HORIZONTAL_COORDINATES
		if (MOUNT_ON_COORDINATES_SET_SYNC_ITEM->sw.value) {
//			if (PRIVATE_DATA->globalMode == kGlobalModeIdle) {
//				//  Pass sync requests through to indigo common code
//				return indigo_mount_change_property(device, client, property);
//			}
//			else {
//				MOUNT_EQUATORIAL_COORDINATES_PROPERTY->state = INDIGO_ALERT_STATE;
//				indigo_update_coordinates(device, "Sync not performed - mount is busy.");
//				return INDIGO_OK;
//			}
		} else {
			if (PRIVATE_DATA->globalMode == kGlobalModeIdle) {
				double az = MOUNT_HORIZONTAL_COORDINATES_AZ_ITEM->number.value;
				double alt = MOUNT_HORIZONTAL_COORDINATES_ALT_ITEM->number.value;
				indigo_property_copy_values(MOUNT_HORIZONTAL_COORDINATES_PROPERTY, property, false);
				MOUNT_HORIZONTAL_COORDINATES_AZ_ITEM->number.value = az;
				MOUNT_HORIZONTAL_COORDINATES_ALT_ITEM->number.value = alt;
				mount_handle_aa_coordinates(device);
			}
			else {
				MOUNT_HORIZONTAL_COORDINATES_PROPERTY->state = INDIGO_ALERT_STATE;
				//  FIXME - is there an assumption about coordinates here?
				//indigo_update_coordinates(device, "Slew not started - mount is busy.");
			}
			return INDIGO_OK;
		}
#endif
	} else if (indigo_property_match(MOUNT_TRACK_RATE_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- MOUNT_TRACK_RATE
		if (IS_CONNECTED) {
			indigo_property_copy_values(MOUNT_TRACK_RATE_PROPERTY, property, false);
			mount_handle_tracking_rate(device);
		}
		return INDIGO_OK;
	} else if (indigo_property_match(MOUNT_TRACKING_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- MOUNT_TRACKING (on/off)
		if (IS_CONNECTED) {
			if (MOUNT_PARK_PARKED_ITEM->sw.value) {
				MOUNT_TRACKING_PROPERTY->state = INDIGO_ALERT_STATE;
				indigo_update_property(device, MOUNT_TRACKING_PROPERTY, "Mount is parked.");
			} else {
				indigo_property_copy_values(MOUNT_TRACKING_PROPERTY, property, false);
				mount_handle_tracking(device);
			}
			return INDIGO_OK;
		}
	} else if (indigo_property_match(MOUNT_SLEW_RATE_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- MOUNT_SLEW_RATE
		//  We dont even need to do thsi - just let common code handle it
		//indigo_property_copy_values(MOUNT_SLEW_RATE_PROPERTY, property, false);
		//MOUNT_SLEW_RATE_PROPERTY->state = INDIGO_OK_STATE;
	} else if (indigo_property_match(MOUNT_MOTION_RA_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- MOUNT_MOTION_RA
		if (MOUNT_PARK_PARKED_ITEM->sw.value) {
			MOUNT_MOTION_RA_PROPERTY->state = INDIGO_ALERT_STATE;
			indigo_update_property(device, MOUNT_MOTION_RA_PROPERTY, "Mount is parked.");
		} else {
			indigo_property_copy_values(MOUNT_MOTION_RA_PROPERTY, property, false);
			mount_handle_motion_ra(device);
		}
		return INDIGO_OK;
	} else if (indigo_property_match(MOUNT_MOTION_DEC_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- MOUNT_MOTION_DEC
		if (MOUNT_PARK_PARKED_ITEM->sw.value) {
			MOUNT_MOTION_DEC_PROPERTY->state = INDIGO_ALERT_STATE;
			indigo_update_property(device, MOUNT_MOTION_DEC_PROPERTY, "Mount is parked.");
		} else {
			indigo_property_copy_values(MOUNT_MOTION_DEC_PROPERTY, property, false);
			mount_handle_motion_dec(device);
		}
		return INDIGO_OK;
	} else if (indigo_property_match(MOUNT_ABORT_MOTION_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- MOUNT_ABORT_MOTION
		indigo_property_copy_values(MOUNT_ABORT_MOTION_PROPERTY, property, false);
		mount_handle_abort(device);
		return INDIGO_OK;
	}
	else if (indigo_property_match(MOUNT_GUIDE_RATE_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- MOUNT_GUIDE_RATE
		indigo_property_copy_values(MOUNT_GUIDE_RATE_PROPERTY, property, false);
		if (IS_CONNECTED) {
			mount_handle_st4_guiding_rate(device);
		}
		return INDIGO_OK;
	} else if (indigo_property_match(MOUNT_POLARSCOPE_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- MOUNT_POLARSCOPE
		indigo_property_copy_values(MOUNT_POLARSCOPE_PROPERTY, property, false);
		if (IS_CONNECTED) {
			mount_handle_polarscope(device);
		}
		return INDIGO_OK;
	} else if (indigo_property_match(MOUNT_USE_ENCODERS_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- MOUNT_USE_ENCODERS
		indigo_property_copy_values(MOUNT_USE_ENCODERS_PROPERTY, property, false);
		if (IS_CONNECTED) {
			mount_handle_encoders(device);
		}
		return INDIGO_OK;
	} else if (indigo_property_match(MOUNT_PEC_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- MOUNT_USE_PPEC
		indigo_property_copy_values(MOUNT_PEC_PROPERTY, property, false);
		if (IS_CONNECTED) {
			mount_handle_use_ppec(device);
		}
		return INDIGO_OK;
	} else if (indigo_property_match(MOUNT_PEC_TRAINING_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- MOUNT_TRAIN_PPEC
		indigo_property_copy_values(MOUNT_PEC_TRAINING_PROPERTY, property, false);
		if (IS_CONNECTED) {
			mount_handle_train_ppec(device);
		}
		return INDIGO_OK;
	} else if (indigo_property_match(MOUNT_AUTOHOME_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- MOUNT_AUTOHOME
		if (MOUNT_PARK_PARKED_ITEM->sw.value) {
			MOUNT_AUTOHOME_PROPERTY->state = INDIGO_ALERT_STATE;
			indigo_update_property(device, MOUNT_AUTOHOME_PROPERTY, "Mount is parked.");
		} else {
			indigo_property_copy_values(MOUNT_AUTOHOME_PROPERTY, property, false);
			if (IS_CONNECTED) {
				mount_handle_autohome(device);
			}
		}
		return INDIGO_OK;
	} else if (indigo_property_match(MOUNT_AUTOHOME_SETTINGS_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- MOUNT_AUTOHOME_SETTINGS
		indigo_property_copy_values(MOUNT_AUTOHOME_SETTINGS_PROPERTY, property, false);
		indigo_update_property(device, MOUNT_AUTOHOME_SETTINGS_PROPERTY, NULL);
		return INDIGO_OK;
	} else if (indigo_property_match(MOUNT_OPERATING_MODE_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- MOUNT_OPERATING_MODE
		indigo_property_copy_values(MOUNT_OPERATING_MODE_PROPERTY, property, false);
		MOUNT_OPERATING_MODE_PROPERTY->state = INDIGO_OK_STATE;
		if (IS_CONNECTED) {
			indigo_update_property(device, MOUNT_OPERATING_MODE_PROPERTY, "Switched mount operating mode");
		}
		return INDIGO_OK;
	} else if (indigo_property_match(CONFIG_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- CONFIG
		if (indigo_switch_match(CONFIG_SAVE_ITEM, property)) {
			indigo_save_property(device, NULL, MOUNT_POLARSCOPE_PROPERTY);
			indigo_save_property(device, NULL, MOUNT_OPERATING_MODE_PROPERTY);
			indigo_save_property(device, NULL, MOUNT_USE_ENCODERS_PROPERTY);
			indigo_save_property(device, NULL, MOUNT_AUTOHOME_SETTINGS_PROPERTY);
		}
		// --------------------------------------------------------------------------------
	}
	return indigo_mount_change_property(device, client, property);
}

static indigo_result mount_detach(indigo_device *device) {
	assert(device != NULL);
	if (IS_CONNECTED) {
		indigo_set_switch(CONNECTION_PROPERTY, CONNECTION_DISCONNECTED_ITEM, true);
		synscan_mount_connect(device);
	}
	indigo_delete_property(device, MOUNT_POLARSCOPE_PROPERTY, NULL);
	indigo_delete_property(device, MOUNT_OPERATING_MODE_PROPERTY, NULL);
	indigo_release_property(MOUNT_POLARSCOPE_PROPERTY);
	indigo_release_property(MOUNT_OPERATING_MODE_PROPERTY);
	indigo_release_property(MOUNT_USE_ENCODERS_PROPERTY);
	indigo_release_property(MOUNT_AUTOHOME_PROPERTY);
	indigo_release_property(MOUNT_AUTOHOME_SETTINGS_PROPERTY);
	INDIGO_DEVICE_DETACH_LOG(DRIVER_NAME, device->name);
	return indigo_mount_detach(device);
}

// -------------------------------------------------------------------------------- INDIGO guider device implementation

static indigo_result guider_attach(indigo_device *device) {
	assert(device != NULL);
	assert(PRIVATE_DATA != NULL);
	if (indigo_guider_attach(device, DRIVER_NAME, DRIVER_VERSION) == INDIGO_OK) {
		// -------------------------------------------------------------------------------- GUIDER_RATE
		GUIDER_RATE_PROPERTY->hidden = false;
		GUIDER_RATE_PROPERTY->count = 2;
		strncpy(GUIDER_RATE_PROPERTY->label,"Pulse-Guide Rate", INDIGO_VALUE_SIZE);
		indigo_copy_value(GUIDER_RATE_ITEM->label, "RA Guiding rate (% of sidereal)");

		INDIGO_DEVICE_ATTACH_LOG(DRIVER_NAME, device->name);

		//  Init sync primitives
		pthread_mutex_init(&PRIVATE_DATA->ha_mutex, NULL);
		pthread_mutex_init(&PRIVATE_DATA->dec_mutex, NULL);
		pthread_cond_init(&PRIVATE_DATA->ha_pulse_cond, NULL);
		pthread_cond_init(&PRIVATE_DATA->dec_pulse_cond, NULL);
		PRIVATE_DATA->guiding_thread_exit = false;
		PRIVATE_DATA->ha_pulse_ms = PRIVATE_DATA->dec_pulse_ms = 0;

		return indigo_guider_enumerate_properties(device, NULL, NULL);
	}
	return INDIGO_FAILED;
}

static indigo_result guider_enumerate_properties(indigo_device *device, indigo_client *client, indigo_property *property) {
	if (IS_CONNECTED) {
	}
	return indigo_guider_enumerate_properties(device, NULL, NULL);
}

static indigo_result guider_change_property(indigo_device *device, indigo_client *client, indigo_property *property) {
	assert(device != NULL);
	assert(DEVICE_CONTEXT != NULL);
	assert(property != NULL);
	// -------------------------------------------------------------------------------- CONNECTION
	if (indigo_property_match(CONNECTION_PROPERTY, property)) {
		if (indigo_ignore_connection_change(device, property))
			return INDIGO_OK;
		indigo_property_copy_values(CONNECTION_PROPERTY, property, false);
		CONNECTION_PROPERTY->state = INDIGO_BUSY_STATE;
		indigo_update_property(device, CONNECTION_PROPERTY, NULL);
		return synscan_guider_connect(device);
	}
	else if (indigo_property_match(GUIDER_GUIDE_RA_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- GUIDER_GUIDE_RA
		indigo_property_copy_values(GUIDER_GUIDE_RA_PROPERTY, property, false);
		GUIDER_GUIDE_RA_PROPERTY->state = INDIGO_OK_STATE;
		int duration = 0;
		if (GUIDER_GUIDE_EAST_ITEM->number.value > 0) {
			GUIDER_GUIDE_RA_PROPERTY->state = INDIGO_BUSY_STATE;
			duration = -GUIDER_GUIDE_EAST_ITEM->number.value;
			INDIGO_DRIVER_DEBUG(DRIVER_NAME, "Guiding %dms EAST", -duration);
			indigo_update_property(device, GUIDER_GUIDE_RA_PROPERTY, NULL);
		}
		else if (GUIDER_GUIDE_WEST_ITEM->number.value > 0) {
			GUIDER_GUIDE_RA_PROPERTY->state = INDIGO_BUSY_STATE;
			duration = GUIDER_GUIDE_WEST_ITEM->number.value;
			INDIGO_DRIVER_DEBUG(DRIVER_NAME, "Guiding %dms WEST", duration);
			indigo_update_property(device, GUIDER_GUIDE_RA_PROPERTY, NULL);
		}
		else {
			indigo_update_property(device, GUIDER_GUIDE_RA_PROPERTY, NULL);
		}

		//  Check if tracking is enabled on master device
		//  Scope hack to make the MOUNT_CONTEXT macro work
		bool tracking_enabled = false;
		{
			indigo_device* d = device;
			indigo_device* device = d->master_device;
			tracking_enabled = MOUNT_TRACKING_ON_ITEM->sw.value;
		}
		
		//  Start a pulse if the mount is tracking
		if (duration != 0 && tracking_enabled) {
			pthread_mutex_lock(&PRIVATE_DATA->ha_mutex);
			PRIVATE_DATA->ha_pulse_ms = duration;
			pthread_cond_signal(&PRIVATE_DATA->ha_pulse_cond);
			pthread_mutex_unlock(&PRIVATE_DATA->ha_mutex);
		}
		else if (duration != 0 && !tracking_enabled) {
			GUIDER_GUIDE_RA_PROPERTY->state = INDIGO_ALERT_STATE;
			indigo_update_property(device, GUIDER_GUIDE_RA_PROPERTY, "Ignoring RA guide pulse - mount is not tracking.");
		}
		return INDIGO_OK;
	}
	else if (indigo_property_match(GUIDER_GUIDE_DEC_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- GUIDER_GUIDE_DEC
		indigo_property_copy_values(GUIDER_GUIDE_DEC_PROPERTY, property, false);
		GUIDER_GUIDE_DEC_PROPERTY->state = INDIGO_OK_STATE;
		int duration = 0;
		if (GUIDER_GUIDE_NORTH_ITEM->number.value > 0) {
			GUIDER_GUIDE_DEC_PROPERTY->state = INDIGO_BUSY_STATE;
			duration = GUIDER_GUIDE_NORTH_ITEM->number.value;
			INDIGO_DRIVER_DEBUG(DRIVER_NAME, "Guiding %dms NORTH", duration);
			indigo_update_property(device, GUIDER_GUIDE_DEC_PROPERTY, NULL);
		}
		else if (GUIDER_GUIDE_SOUTH_ITEM->number.value > 0) {
			GUIDER_GUIDE_DEC_PROPERTY->state = INDIGO_BUSY_STATE;
			duration = -GUIDER_GUIDE_SOUTH_ITEM->number.value;
			INDIGO_DRIVER_DEBUG(DRIVER_NAME, "Guiding %dms SOUTH", -duration);
			indigo_update_property(device, GUIDER_GUIDE_DEC_PROPERTY, NULL);
		}
		else {
			indigo_update_property(device, GUIDER_GUIDE_DEC_PROPERTY, NULL);
		}

		//  Start a pulse
		if (duration != 0) {
			pthread_mutex_lock(&PRIVATE_DATA->dec_mutex);
			PRIVATE_DATA->dec_pulse_ms = duration;
			pthread_cond_signal(&PRIVATE_DATA->dec_pulse_cond);
			pthread_mutex_unlock(&PRIVATE_DATA->dec_mutex);
		}
		return INDIGO_OK;
	}
	else if (indigo_property_match(GUIDER_RATE_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- GUIDE_RATE
		indigo_property_copy_values(GUIDER_RATE_PROPERTY, property, false);
		if (IS_CONNECTED) {
			indigo_update_property(device, GUIDER_RATE_PROPERTY, "Guide rate updated.");
		}
		return INDIGO_OK;
	}
	else if (indigo_property_match(CONFIG_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- CONFIG
		if (indigo_switch_match(CONFIG_SAVE_ITEM, property))
			indigo_save_property(device, NULL, GUIDER_RATE_PROPERTY);
	}
	// --------------------------------------------------------------------------------
	return indigo_guider_change_property(device, client, property);
}

static indigo_result guider_detach(indigo_device *device) {
	assert(device != NULL);
	if (IS_CONNECTED) {
		indigo_set_switch(CONNECTION_PROPERTY, CONNECTION_DISCONNECTED_ITEM, true);
		synscan_guider_connect(device);
	}

	//  Wake up the pulse timer threads to exit
	PRIVATE_DATA->guiding_thread_exit = true;
	pthread_cond_signal(&PRIVATE_DATA->ha_pulse_cond);
	pthread_cond_signal(&PRIVATE_DATA->dec_pulse_cond);

	INDIGO_DEVICE_DETACH_LOG(DRIVER_NAME, device->name);
	return indigo_guider_detach(device);
}

// --------------------------------------------------------------------------------

static synscan_private_data *private_data = NULL;

static indigo_device *mount = NULL;
static indigo_device *mount_guider = NULL;

indigo_result indigo_mount_synscan(indigo_driver_action action, indigo_driver_info *info) {
	static indigo_device mount_template = INDIGO_DEVICE_INITIALIZER(
		MOUNT_SYNSCAN_NAME,
		mount_attach,
		mount_enumerate_properties,
		mount_change_property,
		NULL,
		mount_detach
	);
	static indigo_device mount_guider_template = INDIGO_DEVICE_INITIALIZER(
		MOUNT_SYNSCAN_GUIDER_NAME,
		guider_attach,
		guider_enumerate_properties,
		guider_change_property,
		NULL,
		guider_detach
	);
	static indigo_driver_action last_action = INDIGO_DRIVER_SHUTDOWN;

	SET_DRIVER_INFO(info, "SynScan Mount", __FUNCTION__, DRIVER_VERSION, false, last_action);

	if (action == last_action)
		return INDIGO_OK;

	switch (action) {
		case INDIGO_DRIVER_INIT:
			last_action = action;
			private_data = indigo_safe_malloc(sizeof(synscan_private_data));
			mount = indigo_safe_malloc_copy(sizeof(indigo_device), &mount_template);
			mount->master_device = mount;
			mount->private_data = private_data;
			indigo_attach_device(mount);

			mount_guider = indigo_safe_malloc_copy(sizeof(indigo_device), &mount_guider_template);
			mount_guider->master_device = mount;
			mount_guider->private_data = private_data;
			indigo_attach_device(mount_guider);
			break;

		case INDIGO_DRIVER_SHUTDOWN:
			VERIFY_NOT_CONNECTED(mount);
			VERIFY_NOT_CONNECTED(mount_guider);
			last_action = action;
			if (mount_guider != NULL) {
				indigo_detach_device(mount_guider);
			}
			if (mount != NULL) {
				indigo_detach_device(mount);
			}
			while (private_data->timer_count)
				indigo_usleep(100000);
			if (mount_guider != NULL) {
				free(mount_guider);
				mount_guider = NULL;
			}
			if (mount != NULL) {
				free(mount);
				mount = NULL;
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
