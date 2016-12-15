// Copyright (c) 2016 Rumen G. Bogdanovski
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
// 2.0 Build 0 - PoC by Rumen G. Bogdanovski

/** INDIGO MOUNT Nexstar (celestron & skywatcher) driver
 \file indigo_mount_nexstar.c
 */

#define DRIVER_VERSION 0x0001

#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <math.h>
#include <assert.h>

#include "indigo_driver_xml.h"

#include "nexstar.h"
#include "indigo_mount_nexstar.h"

#define h2d(h) (h * 15.0)
#define d2h(d) (d / 15.0)

#define REFRESH_SECONDS (0.5)

#undef PRIVATE_DATA
#define PRIVATE_DATA        ((nexstar_private_data *)DEVICE_CONTEXT->private_data)

#define RA_MIN_DIFF         (1/24/60/10)
#define DEC_MIN_DIFF        (1/60/60)

#define SET_UTC_PROPERTY    (PRIVATE_DATA->set_utc_property)
#define SET_UTC_ITEM		(SET_UTC_PROPERTY->items+0)

typedef struct {
	int dev_id;
	bool parked;
	char tty_name[INDIGO_VALUE_SIZE];
	int count_open;
	int slew_rate;
	pthread_mutex_t serial_mutex;
	indigo_timer *position_timer, *guider_timer_ra, *guider_timer_dec;
	int guide_rate;
	indigo_property *set_utc_property;
} nexstar_private_data;

// -------------------------------------------------------------------------------- INDIGO MOUNT device implementation

static indigo_result nexstar_enumerate_properties(indigo_device *device, indigo_client *client, indigo_property *property) {
	if (CONNECTION_CONNECTED_ITEM->sw.value) {
		if (indigo_property_match(SET_UTC_PROPERTY, property))
			indigo_define_property(device, SET_UTC_PROPERTY, NULL);
	}
	return indigo_mount_enumerate_properties(device, NULL, NULL);
}


static bool mount_open(indigo_device *device) {
	pthread_mutex_lock(&PRIVATE_DATA->serial_mutex);
	if (PRIVATE_DATA->count_open++ == 0) {
		int dev_id = open_telescope(DEVICE_PORT_ITEM->text.value);
		if (dev_id == -1) {
			pthread_mutex_unlock(&PRIVATE_DATA->serial_mutex);
			INDIGO_LOG(indigo_log("indigo_mount_nexstar: open_telescope(%s) = %d", DEVICE_PORT_ITEM->text.value, dev_id));
			return false;
		} else {
			PRIVATE_DATA->dev_id = dev_id;
		}
	}
	pthread_mutex_unlock(&PRIVATE_DATA->serial_mutex);
	return true;
}


static bool mount_handle_coordinates(indigo_device *device) {
	int res = RC_OK;
	pthread_mutex_lock(&PRIVATE_DATA->serial_mutex);
	// GOTO requested
	if(MOUNT_ON_COORDINATES_SET_TRACK_ITEM->sw.value) {
		res = tc_goto_rade_p(PRIVATE_DATA->dev_id, h2d(MOUNT_EQUATORIAL_COORDINATES_RA_ITEM->number.value), MOUNT_EQUATORIAL_COORDINATES_DEC_ITEM->number.value);
		if (res != RC_OK) {
			INDIGO_LOG(indigo_log("indigo_mount_nexstar: tc_goto_rade_p(%d) = %d", PRIVATE_DATA->dev_id, res));
		}
	}
	// SYNC requested
	else if (MOUNT_ON_COORDINATES_SET_SYNC_ITEM->sw.value) {
		res = tc_sync_rade_p(PRIVATE_DATA->dev_id, h2d(MOUNT_EQUATORIAL_COORDINATES_RA_ITEM->number.value), MOUNT_EQUATORIAL_COORDINATES_DEC_ITEM->number.value);
		if (res != RC_OK) {
			INDIGO_LOG(indigo_log("indigo_mount_nexstar: tc_sync_rade_p(%d) = %d", PRIVATE_DATA->dev_id, res));
		}
	}
	pthread_mutex_unlock(&PRIVATE_DATA->serial_mutex);
	if (res) return false;
	else return true;
}


static void mount_handle_slew_rate(indigo_device *device) {
	if(MOUNT_SLEW_RATE_GUIDE_ITEM->sw.value) {
		PRIVATE_DATA->slew_rate = PRIVATE_DATA->guide_rate;
	} else if (MOUNT_SLEW_RATE_CENTERING_ITEM->sw.value) {
		PRIVATE_DATA->slew_rate = 4;
	} else if (MOUNT_SLEW_RATE_FIND_ITEM->sw.value) {
		PRIVATE_DATA->slew_rate = 6;
	} else if (MOUNT_SLEW_RATE_MAX_ITEM->sw.value) {
		PRIVATE_DATA->slew_rate = 9;
	} else {
		MOUNT_SLEW_RATE_GUIDE_ITEM->sw.value = true;
		PRIVATE_DATA->slew_rate = PRIVATE_DATA->guide_rate;
	}
	MOUNT_SLEW_RATE_PROPERTY->state = INDIGO_OK_STATE;
	indigo_update_property(device, MOUNT_SLEW_RATE_PROPERTY, "slew speed = %d", PRIVATE_DATA->slew_rate);
}


static void mount_handle_motion_ns(indigo_device *device) {
	int dev_id = PRIVATE_DATA->dev_id;
	int res = RC_OK;
	char message[INDIGO_VALUE_SIZE];

	if (PRIVATE_DATA->slew_rate == 0) mount_handle_slew_rate(device);

	pthread_mutex_lock(&PRIVATE_DATA->serial_mutex);
	if(MOUNT_MOTION_NORTH_ITEM->sw.value) {
		res = tc_slew_fixed(dev_id, TC_AXIS_DE, TC_DIR_POSITIVE, PRIVATE_DATA->slew_rate);
		strncpy(message,"Moving North...",sizeof(message));
		MOUNT_MOTION_NS_PROPERTY->state = INDIGO_BUSY_STATE;
	} else if (MOUNT_MOTION_SOUTH_ITEM->sw.value) {
		res = tc_slew_fixed(dev_id, TC_AXIS_DE, TC_DIR_NEGATIVE, PRIVATE_DATA->slew_rate);
		strncpy(message,"Moving South...",sizeof(message));
		MOUNT_MOTION_NS_PROPERTY->state = INDIGO_BUSY_STATE;
	} else {
		res = tc_slew_fixed(dev_id, TC_AXIS_DE, TC_DIR_POSITIVE, 0); // STOP move
		strncpy(message,"Stopped moving",sizeof(message));
		MOUNT_MOTION_NS_PROPERTY->state = INDIGO_OK_STATE;
	}
	pthread_mutex_unlock(&PRIVATE_DATA->serial_mutex);

	if (res != RC_OK) {
		INDIGO_LOG(indigo_log("indigo_mount_nexstar: tc_slew_fixed(%d) = %d", dev_id, res));
		MOUNT_MOTION_NS_PROPERTY->state = INDIGO_ALERT_STATE;
	}

	indigo_update_property(device, MOUNT_MOTION_NS_PROPERTY, message);
}


static void mount_handle_motion_ne(indigo_device *device) {
	int dev_id = PRIVATE_DATA->dev_id;
	int res = RC_OK;
	char message[INDIGO_VALUE_SIZE];

	pthread_mutex_lock(&PRIVATE_DATA->serial_mutex);
	if(MOUNT_MOTION_EAST_ITEM->sw.value) {
		res = tc_slew_fixed(dev_id, TC_AXIS_RA, TC_DIR_POSITIVE, PRIVATE_DATA->slew_rate);
		strncpy(message,"Moving East...",sizeof(message));
		MOUNT_MOTION_WE_PROPERTY->state = INDIGO_BUSY_STATE;
	} else if (MOUNT_MOTION_WEST_ITEM->sw.value) {
		res = tc_slew_fixed(dev_id, TC_AXIS_RA, TC_DIR_NEGATIVE, PRIVATE_DATA->slew_rate);
		strncpy(message,"Moving West...",sizeof(message));
		MOUNT_MOTION_WE_PROPERTY->state = INDIGO_BUSY_STATE;
	} else {
		res = tc_slew_fixed(dev_id, TC_AXIS_RA, TC_DIR_POSITIVE, 0); // STOP move
		strncpy(message,"Stopped moving",sizeof(message));
		MOUNT_MOTION_WE_PROPERTY->state = INDIGO_OK_STATE;
	}
	pthread_mutex_unlock(&PRIVATE_DATA->serial_mutex);

	if (res != RC_OK) {
		INDIGO_LOG(indigo_log("indigo_mount_nexstar: tc_slew_fixed(%d) = %d", dev_id, res));
		MOUNT_MOTION_WE_PROPERTY->state = INDIGO_ALERT_STATE;
	}

	indigo_update_property(device, MOUNT_MOTION_WE_PROPERTY, message);
}


static bool mount_set_location(indigo_device *device) {
	int res;
	pthread_mutex_lock(&PRIVATE_DATA->serial_mutex);
	res = tc_set_location(PRIVATE_DATA->dev_id, MOUNT_GEOGRAPHIC_COORDINATES_LONGITUDE_ITEM->number.value, MOUNT_GEOGRAPHIC_COORDINATES_LATITUDE_ITEM->number.value);
	pthread_mutex_unlock(&PRIVATE_DATA->serial_mutex);
	if (res != RC_OK) {
		INDIGO_LOG(indigo_log("indigo_mount_nexstar: tc_set_location(%d) = %d", PRIVATE_DATA->dev_id, res));
		return false;
	}
	return true;
}


static bool mount_set_utc_from_host(indigo_device *device) {
	return true;
}


static bool mount_cancel_slew(indigo_device *device) {
	pthread_mutex_lock(&PRIVATE_DATA->serial_mutex);

	int res = tc_goto_cancel(PRIVATE_DATA->dev_id);
	if (res != RC_OK) {
		INDIGO_LOG(indigo_log("indigo_mount_nexstar: tc_goto_cancel(%d) = %d", PRIVATE_DATA->dev_id, res));
	}

	MOUNT_MOTION_NORTH_ITEM->sw.value = false;
	MOUNT_MOTION_SOUTH_ITEM->sw.value = false;
	MOUNT_MOTION_NS_PROPERTY->state = INDIGO_OK_STATE;
	indigo_update_property(device, MOUNT_MOTION_NS_PROPERTY, NULL);
	MOUNT_MOTION_WEST_ITEM->sw.value = false;
	MOUNT_MOTION_EAST_ITEM->sw.value = false;
	MOUNT_MOTION_WE_PROPERTY->state = INDIGO_OK_STATE;
	indigo_update_property(device, MOUNT_MOTION_WE_PROPERTY, NULL);
	MOUNT_EQUATORIAL_COORDINATES_RA_ITEM->number.target = MOUNT_EQUATORIAL_COORDINATES_RA_ITEM->number.value;
	MOUNT_EQUATORIAL_COORDINATES_DEC_ITEM->number.target = MOUNT_EQUATORIAL_COORDINATES_DEC_ITEM->number.value;
	MOUNT_EQUATORIAL_COORDINATES_PROPERTY->state = INDIGO_OK_STATE;
	indigo_update_property(device, MOUNT_EQUATORIAL_COORDINATES_PROPERTY, NULL);

	MOUNT_ABORT_MOTION_ITEM->sw.value = false;
	MOUNT_ABORT_MOTION_PROPERTY->state = INDIGO_OK_STATE;
	indigo_update_property(device, MOUNT_ABORT_MOTION_PROPERTY, "Aborted");

	pthread_mutex_unlock(&PRIVATE_DATA->serial_mutex);
	return true;
}


static void mount_close(indigo_device *device) {
	pthread_mutex_lock(&PRIVATE_DATA->serial_mutex);

	if (--PRIVATE_DATA->count_open == 0) {
		close_telescope(PRIVATE_DATA->dev_id);
		PRIVATE_DATA->dev_id = -1;
	}

	pthread_mutex_unlock(&PRIVATE_DATA->serial_mutex);
}


static void position_timer_callback(indigo_device *device) {
	int res;
	double ra, dec, lon, lat;
	int dev_id = PRIVATE_DATA->dev_id;

	pthread_mutex_lock(&PRIVATE_DATA->serial_mutex);
	if (tc_goto_in_progress(dev_id)) {
		MOUNT_EQUATORIAL_COORDINATES_PROPERTY->state = INDIGO_BUSY_STATE;
	} else {
		MOUNT_EQUATORIAL_COORDINATES_PROPERTY->state = INDIGO_OK_STATE;
	}

	res = tc_get_rade_p(dev_id, &ra, &dec);
	if (res != RC_OK) {
		INDIGO_LOG(indigo_log("indigo_mount_nexstar: tc_get_rade_p(%d) = %d", dev_id, res));
	}

	res = tc_get_location(dev_id, &lon, &lat);
	if (res != RC_OK) {
		INDIGO_LOG(indigo_log("indigo_mount_nexstar: tc_get_location(%d) = %d", dev_id, res));
	}
	pthread_mutex_unlock(&PRIVATE_DATA->serial_mutex);

	MOUNT_EQUATORIAL_COORDINATES_RA_ITEM->number.value = d2h(ra);
	MOUNT_EQUATORIAL_COORDINATES_DEC_ITEM->number.value = dec;
	indigo_update_property(device, MOUNT_EQUATORIAL_COORDINATES_PROPERTY, NULL);

	MOUNT_GEOGRAPHIC_COORDINATES_LONGITUDE_ITEM->number.value = lon;
	MOUNT_GEOGRAPHIC_COORDINATES_LATITUDE_ITEM->number.value = lat;
	indigo_update_property(device, MOUNT_GEOGRAPHIC_COORDINATES_PROPERTY, NULL);

	PRIVATE_DATA->position_timer = indigo_set_timer(device, REFRESH_SECONDS, position_timer_callback);
}

static indigo_result mount_attach(indigo_device *device) {
	assert(device != NULL);
	assert(device->device_context != NULL);
	nexstar_private_data *private_data = device->device_context;
	device->device_context = NULL;

	if (indigo_mount_attach(device, DRIVER_VERSION) == INDIGO_OK) {
		DEVICE_CONTEXT->private_data = private_data;
		pthread_mutex_init(&PRIVATE_DATA->serial_mutex, NULL);

		SET_UTC_PROPERTY = indigo_init_switch_property(NULL, device->name, "SET_UTC", MOUNT_MAIN_GROUP, "Set mount UTC", INDIGO_IDLE_STATE, INDIGO_RW_PERM, INDIGO_AT_MOST_ONE_RULE, 1);
		if (SET_UTC_PROPERTY == NULL)
			return INDIGO_FAILED;
		indigo_init_switch_item(SET_UTC_PROPERTY->items+0, "Copy from host", "Copy from host", false);

		// -------------------------------------------------------------------------------- SIMULATION
		SIMULATION_PROPERTY->hidden = true;
		// -------------------------------------------------------------------------------- MOUNT_ON_COORDINATES_SET
		MOUNT_ON_COORDINATES_SET_PROPERTY->count = 2;
		// -------------------------------------------------------------------------------- DEVICE_PORT
		DEVICE_PORT_PROPERTY->hidden = false;
		// -------------------------------------------------------------------------------- DEVICE_PORTS
		DEVICE_PORTS_PROPERTY->hidden = false;
		// --------------------------------------------------------------------------------
		
		MOUNT_GEOGRAPHIC_COORDINATES_PROPERTY->hidden = false;
		MOUNT_GEOGRAPHIC_COORDINATES_PROPERTY->count = 2; // we can not set elevation from the protocol
		MOUNT_LST_TIME_PROPERTY->hidden = true;
		MOUNT_UTC_TIME_PROPERTY->perm = INDIGO_RO_PERM;
		MOUNT_SLEW_RATE_PROPERTY->hidden = false;

		INDIGO_LOG(indigo_log("%s attached", device->name));
		return indigo_mount_enumerate_properties(device, NULL, NULL);
	}
	return INDIGO_FAILED;
}

static indigo_result mount_change_property(indigo_device *device, indigo_client *client, indigo_property *property) {
	assert(device != NULL);
	assert(device->device_context != NULL);
	assert(property != NULL);
	// -------------------------------------------------------------------------------- CONNECTION
	if (indigo_property_match(CONNECTION_PROPERTY, property)) {
		indigo_property_copy_values(CONNECTION_PROPERTY, property, false);
		if (CONNECTION_CONNECTED_ITEM->sw.value) {
			if (mount_open(device)) {
				indigo_define_property(device, SET_UTC_PROPERTY, NULL);
				CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
				position_timer_callback(device);
			} else {
				CONNECTION_PROPERTY->state = INDIGO_ALERT_STATE;
				indigo_set_switch(CONNECTION_PROPERTY, CONNECTION_DISCONNECTED_ITEM, true);
			}
		} else {
			indigo_cancel_timer(device, PRIVATE_DATA->position_timer);
			PRIVATE_DATA->position_timer = NULL;
			indigo_delete_property(device, SET_UTC_PROPERTY, NULL);
			mount_close(device);
			CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
		}
	} else if (indigo_property_match(MOUNT_PARK_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- MOUNT_PARK
		indigo_property_copy_values(MOUNT_PARK_PROPERTY, property, false);
		if (MOUNT_PARK_PARKED_ITEM->sw.value) {
			MOUNT_PARK_PROPERTY->state = INDIGO_BUSY_STATE;
			indigo_update_property(device, MOUNT_PARK_PROPERTY, "Parking...");

			// TODO PARK HERE!

			MOUNT_PARK_PROPERTY->state = INDIGO_OK_STATE;
			indigo_update_property(device, MOUNT_PARK_PROPERTY, "Parked");
			PRIVATE_DATA->parked = true;
		} else {
			MOUNT_PARK_PROPERTY->state = INDIGO_BUSY_STATE;
			indigo_update_property(device, MOUNT_PARK_PROPERTY, "Unparking...");

			// TODO UNPARK HERE!

			MOUNT_PARK_PROPERTY->state = INDIGO_OK_STATE;
			indigo_update_property(device, MOUNT_PARK_PROPERTY, "Unparked");
			PRIVATE_DATA->parked = false;
		}
		return INDIGO_OK;
	} else if (indigo_property_match(MOUNT_GEOGRAPHIC_COORDINATES_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- MOUNT_GEOGRAPTHIC_COORDINATES
		indigo_property_copy_values(MOUNT_GEOGRAPHIC_COORDINATES_PROPERTY, property, false);
		if (mount_set_location(device)) {
			MOUNT_GEOGRAPHIC_COORDINATES_PROPERTY->state = INDIGO_OK_STATE;
		} else {
			MOUNT_GEOGRAPHIC_COORDINATES_PROPERTY->state = INDIGO_ALERT_STATE;
		}
		indigo_update_property(device, MOUNT_GEOGRAPHIC_COORDINATES_PROPERTY, NULL);
		return INDIGO_OK;
	} else if (indigo_property_match(SET_UTC_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- SET_UTC_PROPERTY
		indigo_property_copy_values(SET_UTC_PROPERTY, property, false);
		if(SET_UTC_ITEM->sw.value) {
			if(mount_set_utc_from_host(device)) {
				SET_UTC_PROPERTY->state = INDIGO_OK_STATE;
			} else {
				SET_UTC_PROPERTY->state = INDIGO_ALERT_STATE;
			}
		}
		SET_UTC_ITEM->sw.value = false;
		SET_UTC_PROPERTY->state = INDIGO_OK_STATE;
		indigo_update_property(device, SET_UTC_PROPERTY, NULL);
		return INDIGO_OK;
	} else if (indigo_property_match(MOUNT_EQUATORIAL_COORDINATES_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- MOUNT_EQUATORIAL_COORDINATES
		indigo_property_copy_values(MOUNT_EQUATORIAL_COORDINATES_PROPERTY, property, false);
		if(!mount_handle_coordinates(device)) {
			MOUNT_EQUATORIAL_COORDINATES_PROPERTY->state = INDIGO_ALERT_STATE;
			indigo_update_property(device, MOUNT_EQUATORIAL_COORDINATES_PROPERTY, NULL);
		}
		return INDIGO_OK;
	} else if (indigo_property_match(MOUNT_SLEW_RATE_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- MOUNT_SLEW_RATE
		indigo_property_copy_values(MOUNT_SLEW_RATE_PROPERTY, property, false);
		mount_handle_slew_rate(device);
		return INDIGO_OK;
	} else if (indigo_property_match(MOUNT_MOTION_NS_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- MOUNT_MOTION_NS
		indigo_property_copy_values(MOUNT_MOTION_NS_PROPERTY, property, false);
		mount_handle_motion_ns(device);
		return INDIGO_OK;
	} else if (indigo_property_match(MOUNT_MOTION_WE_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- MOUNT_MOTION_WE
		indigo_property_copy_values(MOUNT_MOTION_WE_PROPERTY, property, false);
		mount_handle_motion_ne(device);
		return INDIGO_OK;
	} else if (indigo_property_match(MOUNT_ABORT_MOTION_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- MOUNT_ABORT_MOTION
		indigo_property_copy_values(MOUNT_ABORT_MOTION_PROPERTY, property, false);
		if (PRIVATE_DATA->position_timer != NULL) {
			mount_cancel_slew(device);
			MOUNT_EQUATORIAL_COORDINATES_PROPERTY->state = INDIGO_ALERT_STATE;
			indigo_update_property(device, MOUNT_EQUATORIAL_COORDINATES_PROPERTY, NULL);
		}
		return INDIGO_OK;
		// --------------------------------------------------------------------------------
	}
	return indigo_mount_change_property(device, client, property);
}

static indigo_result mount_detach(indigo_device *device) {
	assert(device != NULL);
	if (CONNECTION_CONNECTED_ITEM->sw.value)
		indigo_device_disconnect(NULL, device);
	if(PRIVATE_DATA->position_timer) {
		indigo_cancel_timer(device, PRIVATE_DATA->position_timer);
		PRIVATE_DATA->position_timer = NULL;
	}

	indigo_release_property(SET_UTC_PROPERTY);
	if (PRIVATE_DATA->dev_id > 0) mount_close(device);
	INDIGO_LOG(indigo_log("%s detached", device->name));
	return indigo_mount_detach(device);
}


// -------------------------------------------------------------------------------- INDIGO guider device implementation


static void guider_timer_callback_ra(indigo_device *device) {
	PRIVATE_DATA->guider_timer_ra = NULL;
	int dev_id = PRIVATE_DATA->dev_id;
	int res;

	pthread_mutex_lock(&PRIVATE_DATA->serial_mutex);
	res = tc_slew_fixed(dev_id, TC_AXIS_RA, TC_DIR_POSITIVE, 0); // STOP move
	pthread_mutex_unlock(&PRIVATE_DATA->serial_mutex);
	if (res != RC_OK) {
		INDIGO_LOG(indigo_log("indigo_mount_nexstar: tc_slew_fixed(%d) = %d", dev_id, res));
	}

	GUIDER_GUIDE_EAST_ITEM->number.value = 0;
	GUIDER_GUIDE_WEST_ITEM->number.value = 0;
	GUIDER_GUIDE_RA_PROPERTY->state = INDIGO_OK_STATE;
	indigo_update_property(device, GUIDER_GUIDE_RA_PROPERTY, NULL);
}


static void guider_timer_callback_dec(indigo_device *device) {
	PRIVATE_DATA->guider_timer_dec = NULL;
	int dev_id = PRIVATE_DATA->dev_id;
	int res;

	pthread_mutex_lock(&PRIVATE_DATA->serial_mutex);
	res = tc_slew_fixed(dev_id, TC_AXIS_DE, TC_DIR_POSITIVE, 0); // STOP move
	pthread_mutex_unlock(&PRIVATE_DATA->serial_mutex);
	if (res != RC_OK) {
		INDIGO_LOG(indigo_log("indigo_mount_nexstar: tc_slew_fixed(%d) = %d", dev_id, res));
	}

	GUIDER_GUIDE_NORTH_ITEM->number.value = 0;
	GUIDER_GUIDE_SOUTH_ITEM->number.value = 0;
	GUIDER_GUIDE_DEC_PROPERTY->state = INDIGO_OK_STATE;
	indigo_update_property(device, GUIDER_GUIDE_DEC_PROPERTY, NULL);
}


static indigo_result guider_attach(indigo_device *device) {
	assert(device != NULL);
	assert(device->device_context != NULL);
	nexstar_private_data *private_data = device->device_context;
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
	// -------------------------------------------------------------------------------- CONNECTION
	if (indigo_property_match(CONNECTION_PROPERTY, property)) {
		indigo_property_copy_values(CONNECTION_PROPERTY, property, false);
		if (CONNECTION_CONNECTED_ITEM->sw.value) {
			if (mount_open(device)) {
				PRIVATE_DATA->guider_timer_ra = NULL;
				PRIVATE_DATA->guider_timer_dec = NULL;
				CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
				GUIDER_GUIDE_DEC_PROPERTY->hidden = false;
				GUIDER_GUIDE_RA_PROPERTY->hidden = false;
			} else {
				CONNECTION_PROPERTY->state = INDIGO_ALERT_STATE;
				indigo_set_switch(CONNECTION_PROPERTY, CONNECTION_DISCONNECTED_ITEM, true);
			}
		} else {
			mount_close(device);
			CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
		}
	} else if (indigo_property_match(GUIDER_GUIDE_DEC_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- GUIDER_GUIDE_DEC
		if (PRIVATE_DATA->guider_timer_dec != NULL)
			indigo_cancel_timer(device, PRIVATE_DATA->guider_timer_dec);
		indigo_property_copy_values(GUIDER_GUIDE_DEC_PROPERTY, property, false);
		GUIDER_GUIDE_DEC_PROPERTY->state = INDIGO_OK_STATE;
		int duration = GUIDER_GUIDE_NORTH_ITEM->number.value;
		if (duration > 0) {
			pthread_mutex_lock(&PRIVATE_DATA->serial_mutex);
			int res = tc_slew_fixed(PRIVATE_DATA->dev_id, TC_AXIS_DE, TC_DIR_POSITIVE, PRIVATE_DATA->guide_rate);
			pthread_mutex_unlock(&PRIVATE_DATA->serial_mutex);
			if (res != RC_OK) {
				INDIGO_LOG(indigo_log("indigo_mount_nexstar: tc_slew_fixed(%d) = %d", PRIVATE_DATA->dev_id, res));
			}
			GUIDER_GUIDE_DEC_PROPERTY->state = INDIGO_BUSY_STATE;
			PRIVATE_DATA->guider_timer_dec = indigo_set_timer(device, duration/1000.0, guider_timer_callback_dec);
		} else {
			int duration = GUIDER_GUIDE_SOUTH_ITEM->number.value;
			if (duration > 0) {
				pthread_mutex_lock(&PRIVATE_DATA->serial_mutex);
				int res = tc_slew_fixed(PRIVATE_DATA->dev_id, TC_AXIS_DE, TC_DIR_NEGATIVE, PRIVATE_DATA->guide_rate);
				pthread_mutex_unlock(&PRIVATE_DATA->serial_mutex);
				if (res != RC_OK) {
					INDIGO_LOG(indigo_log("indigo_mount_nexstar: tc_slew_fixed(%d) = %d", PRIVATE_DATA->dev_id, res));
				}
				GUIDER_GUIDE_DEC_PROPERTY->state = INDIGO_BUSY_STATE;
				PRIVATE_DATA->guider_timer_dec = indigo_set_timer(device, duration/1000.0, guider_timer_callback_dec);
			}
		}
		indigo_update_property(device, GUIDER_GUIDE_DEC_PROPERTY, NULL);
		return INDIGO_OK;
	} else if (indigo_property_match(GUIDER_GUIDE_RA_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- GUIDER_GUIDE_RA
		if (PRIVATE_DATA->guider_timer_ra != NULL)
			indigo_cancel_timer(device, PRIVATE_DATA->guider_timer_ra);
		indigo_property_copy_values(GUIDER_GUIDE_RA_PROPERTY, property, false);
		GUIDER_GUIDE_RA_PROPERTY->state = INDIGO_OK_STATE;
		int duration = GUIDER_GUIDE_EAST_ITEM->number.value;
		if (duration > 0) {
			pthread_mutex_lock(&PRIVATE_DATA->serial_mutex);
			int res = tc_slew_fixed(PRIVATE_DATA->dev_id, TC_AXIS_RA, TC_DIR_POSITIVE, PRIVATE_DATA->guide_rate);
			pthread_mutex_unlock(&PRIVATE_DATA->serial_mutex);
			if (res != RC_OK) {
				INDIGO_LOG(indigo_log("indigo_mount_nexstar: tc_slew_fixed(%d) = %d", PRIVATE_DATA->dev_id, res));
			}
			GUIDER_GUIDE_RA_PROPERTY->state = INDIGO_BUSY_STATE;
			PRIVATE_DATA->guider_timer_ra = indigo_set_timer(device, duration/1000.0, guider_timer_callback_ra);
		} else {
			int duration = GUIDER_GUIDE_WEST_ITEM->number.value;
			if (duration > 0) {
				pthread_mutex_lock(&PRIVATE_DATA->serial_mutex);
				int res = tc_slew_fixed(PRIVATE_DATA->dev_id, TC_AXIS_RA, TC_DIR_NEGATIVE, PRIVATE_DATA->guide_rate);
				pthread_mutex_unlock(&PRIVATE_DATA->serial_mutex);
				if (res != RC_OK) {
					INDIGO_LOG(indigo_log("indigo_mount_nexstar: tc_slew_fixed(%d) = %d", PRIVATE_DATA->dev_id, res));
				}
				GUIDER_GUIDE_RA_PROPERTY->state = INDIGO_BUSY_STATE;
				PRIVATE_DATA->guider_timer_ra = indigo_set_timer(device, duration/1000.0, guider_timer_callback_ra);
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

static nexstar_private_data *private_data = NULL;

static indigo_device *mount = NULL;
static indigo_device *mount_guider = NULL;

indigo_result indigo_mount_nexstar(indigo_driver_action action, indigo_driver_info *info) {
	static indigo_device mount_template = {
		MOUNT_NEXSTAR_NAME, NULL, INDIGO_OK, INDIGO_VERSION_CURRENT,
		mount_attach,
		nexstar_enumerate_properties,
		mount_change_property,
		mount_detach
	};
	static indigo_device mount_guider_template = {
		MOUNT_NEXSTAR_GUIDER_NAME, NULL, INDIGO_OK, INDIGO_VERSION_CURRENT,
		guider_attach,
		indigo_guider_enumerate_properties,
		guider_change_property,
		guider_detach
	};

	static indigo_driver_action last_action = INDIGO_DRIVER_SHUTDOWN;

	SET_DRIVER_INFO(info, MOUNT_NEXSTAR_NAME, __FUNCTION__, DRIVER_VERSION, last_action);

	if (action == last_action)
		return INDIGO_OK;

	switch (action) {
	case INDIGO_DRIVER_INIT:
		last_action = action;
		private_data = malloc(sizeof(nexstar_private_data));
		assert(private_data != NULL);
		memset(private_data, 0, sizeof(nexstar_private_data));
		private_data->dev_id = -1;
		private_data->guide_rate = 1; /* 1 -> 0.5 siderial rate , 2 -> siderial rate */
		private_data->count_open = 0;
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

