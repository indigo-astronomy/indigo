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

#include "indigo_driver_xml.h"
#include "indigo_io.h"
#include "indigo_novas.h"
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
	if (indigo_mount_attach(device, DRIVER_VERSION) == INDIGO_OK) {
		// -------------------------------------------------------------------------------- MOUNT_PARK
		MOUNT_PARK_PARKED_ITEM->sw.value = false;
		MOUNT_PARK_UNPARKED_ITEM->sw.value = true;
		// -------------------------------------------------------------------------------- MOUNT_PARK_SET
		MOUNT_PARK_SET_PROPERTY->hidden = false;
		// -------------------------------------------------------------------------------- MOUNT_PARK_POSITION
		MOUNT_PARK_POSITION_PROPERTY->hidden = false;
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
		// -------------------------------------------------------------------------------- MOUNT_ALIGNMENT_MODE
		indigo_set_switch(MOUNT_ALIGNMENT_MODE_PROPERTY, MOUNT_ALIGNMENT_MODE_NEAREST_POINT_ITEM, true);
		MOUNT_ALIGNMENT_MODE_PROPERTY->hidden = false;
		MOUNT_ALIGNMENT_MODE_PROPERTY->count = 2;
		MOUNT_ALIGNMENT_SELECT_POINTS_PROPERTY->hidden = false;
		MOUNT_ALIGNMENT_SELECT_POINTS_PROPERTY->rule = INDIGO_ANY_OF_MANY_RULE;
		MOUNT_ALIGNMENT_DELETE_POINTS_PROPERTY->hidden = false;
		// -------------------------------------------------------------------------------- MOUNT_EPOCH
		MOUNT_EPOCH_PROPERTY->hidden = false;
		// -------------------------------------------------------------------------------- MOUNT_SIDE_OF_PIER
		MOUNT_SIDE_OF_PIER_PROPERTY->hidden = false;
		// -------------------------------------------------------------------------------- MOUNT_POLARSCOPE
		MOUNT_POLARSCOPE_PROPERTY = indigo_init_number_property(NULL, device->name, MOUNT_POLARSCOPE_PROPERTY_NAME, MOUNT_MAIN_GROUP, "Polarscope", INDIGO_OK_STATE, INDIGO_RW_PERM, 1);
		if (MOUNT_POLARSCOPE_PROPERTY == NULL)
			return INDIGO_FAILED;
		MOUNT_POLARSCOPE_PROPERTY->hidden = true;
		indigo_init_number_item(MOUNT_POLARSCOPE_BRIGHTNESS_ITEM, MOUNT_POLARSCOPE_BRIGHTNESS_ITEM_NAME, "Polarscope Brightness", 0, 255, 0, 0);
		// -------------------------------------------------------------------------------- OPERATING_MODE
		OPERATING_MODE_PROPERTY = indigo_init_switch_property(NULL, device->name, OPERATING_MODE_PROPERTY_NAME, MOUNT_MAIN_GROUP, "Operating mode", INDIGO_OK_STATE, INDIGO_RW_PERM, INDIGO_ONE_OF_MANY_RULE, 2);
		if (OPERATING_MODE_PROPERTY == NULL)
			return INDIGO_FAILED;
		indigo_init_switch_item(POLAR_MODE_ITEM, POLAR_MODE_ITEM_NAME, "Polar mode", true);
		indigo_init_switch_item(ALTAZ_MODE_ITEM, ALTAZ_MODE_ITEM_NAME, "Alt/Az mode", false);
		OPERATING_MODE_PROPERTY->hidden = true;

		//  FURTHER INITIALISATION
		pthread_mutex_init(&PRIVATE_DATA->port_mutex, NULL);
		pthread_mutex_init(&PRIVATE_DATA->driver_mutex, NULL);
		pthread_mutex_init(&PRIVATE_DATA->ha_mutex, NULL);
		pthread_mutex_init(&PRIVATE_DATA->dec_mutex, NULL);
		PRIVATE_DATA->mountConfigured = false;
		INDIGO_DEVICE_ATTACH_LOG(DRIVER_NAME, device->name);
		
		indigo_define_property(device, MOUNT_POLARSCOPE_PROPERTY, NULL);
		indigo_define_property(device, OPERATING_MODE_PROPERTY, NULL);
		return indigo_mount_enumerate_properties(device, NULL, NULL);
	}
	return INDIGO_FAILED;
}

//  This gets called when each client attaches to discover device properties
static indigo_result mount_enumerate_properties(indigo_device *device, indigo_client *client, indigo_property *property) {
	assert(device != NULL);
	assert(DEVICE_CONTEXT != NULL);
	indigo_result result = INDIGO_OK;
	if ((result = indigo_mount_enumerate_properties(device, client, property)) == INDIGO_OK) {
		if (IS_CONNECTED) {
			if (indigo_property_match(MOUNT_POLARSCOPE_PROPERTY, property))
				indigo_define_property(device, MOUNT_POLARSCOPE_PROPERTY, NULL);
			if (indigo_property_match(OPERATING_MODE_PROPERTY, property))
				indigo_define_property(device, OPERATING_MODE_PROPERTY, NULL);
		}
	}
	return result;
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

		indigo_property_copy_values(CONNECTION_PROPERTY, property, false);

		//  Talk to the mount - see if its there - read configuration data
		//  Once we have it all, update the property to OK or ALERT state
		//  This can be done by a background timer thread
		synscan_mount_connect(device);
		//  Falls through to base code to complete connection
	} else if (indigo_property_match(MOUNT_PARK_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- MOUNT_PARK
		indigo_property_copy_values(MOUNT_PARK_PROPERTY, property, false);
		mount_handle_park(device);
		return INDIGO_OK;
	} else if (indigo_property_match(MOUNT_EQUATORIAL_COORDINATES_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- MOUNT_EQUATORIAL_COORDINATES
		if (MOUNT_ON_COORDINATES_SET_SYNC_ITEM->sw.value) {
			if (PRIVATE_DATA->globalMode == kGlobalModeIdle) {
				//  Pass sync requests through to indigo common code
				return indigo_mount_change_property(device, client, property);
			}
			else {
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
			}
			else {
				MOUNT_EQUATORIAL_COORDINATES_PROPERTY->state = INDIGO_ALERT_STATE;
				indigo_update_coordinates(device, "Slew not started - mount is busy.");
			}
			return INDIGO_OK;
		}
	} else if (indigo_property_match(MOUNT_TRACK_RATE_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- MOUNT_TRACK_RATE
		indigo_property_copy_values(MOUNT_TRACK_RATE_PROPERTY, property, false);
		mount_handle_tracking_rate(device);
		return INDIGO_OK;
	} else if (indigo_property_match(MOUNT_TRACKING_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- MOUNT_TRACKING (on/off)
		indigo_property_copy_values(MOUNT_TRACKING_PROPERTY, property, false);
		mount_handle_tracking(device);
		return INDIGO_OK;
	} else if (indigo_property_match(MOUNT_SLEW_RATE_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- MOUNT_SLEW_RATE
		indigo_property_copy_values(MOUNT_SLEW_RATE_PROPERTY, property, false);
		MOUNT_SLEW_RATE_PROPERTY->state = INDIGO_OK_STATE;
	} else if (indigo_property_match(MOUNT_MOTION_RA_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- MOUNT_MOTION_RA
		indigo_property_copy_values(MOUNT_MOTION_RA_PROPERTY, property, false);
		mount_handle_motion_ra(device);
		return INDIGO_OK;
	} else if (indigo_property_match(MOUNT_MOTION_DEC_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- MOUNT_MOTION_DEC
		indigo_property_copy_values(MOUNT_MOTION_DEC_PROPERTY, property, false);
		mount_handle_motion_dec(device);
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
		mount_handle_st4_guiding_rate(device);
		return INDIGO_OK;
	} else if (indigo_property_match(MOUNT_POLARSCOPE_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- MOUNT_POLARSCOPE
		indigo_property_copy_values(MOUNT_POLARSCOPE_PROPERTY, property, false);
		mount_handle_polarscope(device);
		return INDIGO_OK;
		// --------------------------------------------------------------------------------
	}
	return indigo_mount_change_property(device, client, property);
}

static indigo_result mount_detach(indigo_device *device) {
	assert(device != NULL);
	if (CONNECTION_CONNECTED_ITEM->sw.value)
		indigo_device_disconnect(NULL, device->name);
	indigo_delete_property(device, MOUNT_POLARSCOPE_PROPERTY, NULL);
	indigo_delete_property(device, OPERATING_MODE_PROPERTY, NULL);
	indigo_release_property(MOUNT_POLARSCOPE_PROPERTY);
	indigo_release_property(OPERATING_MODE_PROPERTY);
	indigo_cancel_timer(device, &PRIVATE_DATA->position_timer);
	INDIGO_DEVICE_DETACH_LOG(DRIVER_NAME, device->name);
	return indigo_mount_detach(device);
}

// -------------------------------------------------------------------------------- INDIGO guider device implementation

static indigo_result guider_attach(indigo_device *device) {
	assert(device != NULL);
	assert(PRIVATE_DATA != NULL);
	if (indigo_guider_attach(device, DRIVER_VERSION) == INDIGO_OK) {
		GUIDER_RATE_PROPERTY->hidden = false;
		INDIGO_DEVICE_ATTACH_LOG(DRIVER_NAME, device->name);
		return indigo_guider_enumerate_properties(device, NULL, NULL);
	}
	return INDIGO_FAILED;
}

static indigo_result guider_enumerate_properties(indigo_device *device, indigo_client *client, indigo_property *property) {
	if (IS_CONNECTED) {
		//  Placeholder in case we need custom properties
	}
	return indigo_guider_enumerate_properties(device, NULL, NULL);
}

static indigo_result guider_change_property(indigo_device *device, indigo_client *client, indigo_property *property) {
	assert(device != NULL);
	assert(DEVICE_CONTEXT != NULL);
	assert(property != NULL);
	// -------------------------------------------------------------------------------- CONNECTION
	if (indigo_property_match(CONNECTION_PROPERTY, property)) {
		indigo_property_copy_values(CONNECTION_PROPERTY, property, false);
		synscan_mount_connect(device);
//		if (CONNECTION_CONNECTED_ITEM->sw.value) {
//			if (!device->is_connected) {
//				if (mount_open(device)) {
//					device->is_connected = true;
//					indigo_define_property(device, COMMAND_GUIDE_RATE_PROPERTY, NULL);
//					PRIVATE_DATA->guider_timer_ra = NULL;
//					PRIVATE_DATA->guider_timer_dec = NULL;
//					CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
//					GUIDER_GUIDE_DEC_PROPERTY->hidden = false;
//					GUIDER_GUIDE_RA_PROPERTY->hidden = false;
//				} else {
//					CONNECTION_PROPERTY->state = INDIGO_ALERT_STATE;
//					indigo_set_switch(CONNECTION_PROPERTY, CONNECTION_DISCONNECTED_ITEM, true);
//				}
//			}
//		} else {
//			if (device->is_connected) {
//				mount_close(device);
//				indigo_delete_property(device, COMMAND_GUIDE_RATE_PROPERTY, NULL);
//				device->is_connected = false;
//				CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
//			}
//		}
	}
	else if (indigo_property_match(GUIDER_GUIDE_RA_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- GUIDER_GUIDE_RA
		//indigo_cancel_timer(device, &PRIVATE_DATA->guider_timer_ra);
		indigo_property_copy_values(GUIDER_GUIDE_RA_PROPERTY, property, false);
		GUIDER_GUIDE_RA_PROPERTY->state = INDIGO_OK_STATE;
		int duration = GUIDER_GUIDE_EAST_ITEM->number.value;
		if (duration > 0) {
			GUIDER_GUIDE_RA_PROPERTY->state = INDIGO_BUSY_STATE;
			PRIVATE_DATA->guider_timer_ra = indigo_set_timer(device, 0, guider_timer_callback_ra);
//			int res = tc_slew_fixed(PRIVATE_DATA->dev_id, TC_AXIS_RA, TC_DIR_POSITIVE, PRIVATE_DATA->guide_rate);
//			PRIVATE_DATA->guider_timer_ra = indigo_set_timer(device, duration/1000.0, guider_timer_callback_ra);
		}
		else {
			int duration = GUIDER_GUIDE_WEST_ITEM->number.value;
			if (duration > 0) {
				GUIDER_GUIDE_RA_PROPERTY->state = INDIGO_BUSY_STATE;
				PRIVATE_DATA->guider_timer_ra = indigo_set_timer(device, 0, guider_timer_callback_ra);
//				int res = tc_slew_fixed(PRIVATE_DATA->dev_id, TC_AXIS_RA, TC_DIR_NEGATIVE, PRIVATE_DATA->guide_rate);
//				PRIVATE_DATA->guider_timer_ra = indigo_set_timer(device, duration/1000.0, guider_timer_callback_ra);
			}
		}
		indigo_update_property(device, GUIDER_GUIDE_RA_PROPERTY, NULL);
		return INDIGO_OK;
	}
	else if (indigo_property_match(GUIDER_GUIDE_DEC_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- GUIDER_GUIDE_DEC
		//indigo_cancel_timer(device, &PRIVATE_DATA->guider_timer_dec);
		indigo_property_copy_values(GUIDER_GUIDE_DEC_PROPERTY, property, false);
		GUIDER_GUIDE_DEC_PROPERTY->state = INDIGO_OK_STATE;
		int duration = GUIDER_GUIDE_NORTH_ITEM->number.value;
		if (duration > 0) {
			GUIDER_GUIDE_DEC_PROPERTY->state = INDIGO_BUSY_STATE;
			PRIVATE_DATA->guider_timer_dec = indigo_set_timer(device, 0, guider_timer_callback_dec);
//			int res = tc_slew_fixed(PRIVATE_DATA->dev_id, TC_AXIS_DE, TC_DIR_POSITIVE, PRIVATE_DATA->guide_rate);
//			PRIVATE_DATA->guider_timer_dec = indigo_set_timer(device, duration/1000.0, guider_timer_callback_dec);
		} else {
			int duration = GUIDER_GUIDE_SOUTH_ITEM->number.value;
			if (duration > 0) {
				GUIDER_GUIDE_DEC_PROPERTY->state = INDIGO_BUSY_STATE;
				PRIVATE_DATA->guider_timer_dec = indigo_set_timer(device, 0, guider_timer_callback_dec);
//				int res = tc_slew_fixed(PRIVATE_DATA->dev_id, TC_AXIS_DE, TC_DIR_NEGATIVE, PRIVATE_DATA->guide_rate);
//				PRIVATE_DATA->guider_timer_dec = indigo_set_timer(device, duration/1000.0, guider_timer_callback_dec);
			}
		}
		indigo_update_property(device, GUIDER_GUIDE_DEC_PROPERTY, NULL);
		return INDIGO_OK;
	}
	else if (indigo_property_match(GUIDER_RATE_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- COMMAND_GUIDE_RATE
		indigo_property_copy_values(GUIDER_RATE_PROPERTY, property, false);
		indigo_update_property(device, GUIDER_RATE_PROPERTY, "Guide rate updated.");
		return INDIGO_OK;
	}
	// --------------------------------------------------------------------------------
	return indigo_guider_change_property(device, client, property);
}

static indigo_result guider_detach(indigo_device *device) {
	assert(device != NULL);
	if (CONNECTION_CONNECTED_ITEM->sw.value)
		indigo_device_disconnect(NULL, device->name);
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
			private_data = malloc(sizeof(synscan_private_data));
			assert(private_data != NULL);
			memset(private_data, 0, sizeof(synscan_private_data));
			mount = malloc(sizeof(indigo_device));
			assert(mount != NULL);
			memcpy(mount, &mount_template, sizeof(indigo_device));
			mount->master_device = mount;
			mount->private_data = private_data;
			indigo_attach_device(mount);

			mount_guider = malloc(sizeof(indigo_device));
			assert(mount_guider != NULL);
			memcpy(mount_guider, &mount_guider_template, sizeof(indigo_device));
			mount_guider->master_device = mount;
			mount_guider->private_data = private_data;
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
