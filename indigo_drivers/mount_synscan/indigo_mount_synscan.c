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
#include "indigo_mount_synscan_protocol.h"
#include "indigo_mount_synscan_driver.h"

#define h2d(h) (h * 15.0)
#define d2h(d) (d / 15.0)

#define REFRESH_SECONDS (0.5)

#define COMMAND_GUIDE_RATE_PROPERTY     (PRIVATE_DATA->command_guide_rate_property)
#define GUIDE_50_ITEM                   (COMMAND_GUIDE_RATE_PROPERTY->items+0)
#define GUIDE_100_ITEM                  (COMMAND_GUIDE_RATE_PROPERTY->items+1)

#define COMMAND_GUIDE_RATE_PROPERTY_NAME   "COMMAND_GUIDE_RATE"
#define GUIDE_50_ITEM_NAME                 "GUIDE_50"
#define GUIDE_100_ITEM_NAME                "GUIDE_100"

#define WARN_PARKED_MSG                    "Mount is parked, please unpark!"
#define WARN_PARKING_PROGRESS_MSG          "Mount parking is in progress, please wait until complete!"

// gp_bits is used as boolean
#define is_connected                   gp_bits

static const double raRates[] = { 1.25, 2, 8, 16, 32, 70, 100, 625, 725, 825 };
static const double decRates[] = { 0.5, 1, 8, 16, 32, 70, 100, 625, 725, 825 };

// -------------------------------------------------------------------------------- INDIGO MOUNT device implementation

static bool synscan_open(indigo_device *device) {
	char *name = DEVICE_PORT_ITEM->text.value;
	if (strncmp(name, "synscan://", 10)) {
		PRIVATE_DATA->handle = indigo_open_serial(name);
	} else {
		char *host = name + 10;
		char *colon = strchr(host, ':');
		if (colon == NULL) {
			PRIVATE_DATA->handle = indigo_open_tcp(host, 4030);
		} else {
			char host_name[INDIGO_NAME_SIZE];
			strncpy(host_name, host, colon - host);
			int port = atoi(colon + 1);
			PRIVATE_DATA->handle = indigo_open_tcp(host_name, port);
		}
	}
	if (PRIVATE_DATA->handle >= 0) {
		INDIGO_DRIVER_LOG(DRIVER_NAME, "connected to %s", name);
		return true;
	} else {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "failed to connect to %s", name);
		return false;
	}
}

static void synscan_close(indigo_device *device) {
	if (PRIVATE_DATA->handle > 0) {
		close(PRIVATE_DATA->handle);
		PRIVATE_DATA->handle = 0;
		INDIGO_DRIVER_LOG(DRIVER_NAME, "disconnected from %s", DEVICE_PORT_ITEM->text.value);
	}
}

static void mount_handle_coordinates(indigo_device *device) {
	MOUNT_EQUATORIAL_COORDINATES_PROPERTY->state = INDIGO_BUSY_STATE;

	/* GOTO requested */
	if(MOUNT_ON_COORDINATES_SET_SLEW_ITEM->sw.value || MOUNT_ON_COORDINATES_SET_TRACK_ITEM->sw.value) {
		PRIVATE_DATA->slew_state = SLEW_START;
		PRIVATE_DATA->slew_timer = indigo_set_timer(device, 0, slew_timer_callback);
	}
	/* SYNC requested */
	else if (MOUNT_ON_COORDINATES_SET_SYNC_ITEM->sw.value) {
//		res = tc_sync_rade_p(PRIVATE_DATA->dev_id, h2d(MOUNT_EQUATORIAL_COORDINATES_RA_ITEM->number.value), MOUNT_EQUATORIAL_COORDINATES_DEC_ITEM->number.value);
//		if (res != RC_OK) {
			MOUNT_EQUATORIAL_COORDINATES_PROPERTY->state = INDIGO_ALERT_STATE;
//			INDIGO_DRIVER_ERROR(DRIVER_NAME, "tc_sync_rade_p(%d) = %d", PRIVATE_DATA->dev_id, res);
//		}
	}
	indigo_update_coordinates(device, NULL);
}

static void mount_handle_tracking(indigo_device *device) {
	MOUNT_TRACKING_PROPERTY->state = INDIGO_OK_STATE;
	if (MOUNT_TRACKING_ON_ITEM->sw.value) {

		//  Set the correct tracking rate
		//			if (MOUNT_TRACK_RATE_SIDEREAL_ITEM->sw.value && PRIVATE_DATA->lastTrackRate != 'q') {
		//				meade_command(device, ":TQ#", NULL, 0, 0);
		//				PRIVATE_DATA->lastTrackRate = 'q';
		//			} else if (MOUNT_TRACK_RATE_SOLAR_ITEM->sw.value && PRIVATE_DATA->lastTrackRate != 's') {
		//				meade_command(device, ":TS#", NULL, 0, 0);
		//				PRIVATE_DATA->lastTrackRate = 's';
		//			} else if (MOUNT_TRACK_RATE_LUNAR_ITEM->sw.value && PRIVATE_DATA->lastTrackRate != 'l') {
		//				meade_command(device, ":TL#", NULL, 0, 0);
		//				PRIVATE_DATA->lastTrackRate = 'l';


		if (!synscan_start_tracking_mode(device, PRIVATE_DATA->trackingMode)) {
			MOUNT_TRACKING_PROPERTY->state = INDIGO_ALERT_STATE;
		}
	}
	else if (MOUNT_TRACKING_OFF_ITEM->sw.value) {
		if (!synscan_start_tracking_mode(device, kTrackingModeOff)) {
			MOUNT_TRACKING_PROPERTY->state = INDIGO_ALERT_STATE;
		}
	}

	indigo_update_property(device, MOUNT_TRACKING_PROPERTY, NULL);
}

static void mount_handle_slew_rate(indigo_device *device) {
	if (MOUNT_SLEW_RATE_GUIDE_ITEM->sw.value) {
		PRIVATE_DATA->raSlewRate = PRIVATE_DATA->decSlewRate = PRIVATE_DATA->guide_rate;
	} else if (MOUNT_SLEW_RATE_CENTERING_ITEM->sw.value) {
		PRIVATE_DATA->raSlewRate = PRIVATE_DATA->decSlewRate = 4;
	} else if (MOUNT_SLEW_RATE_FIND_ITEM->sw.value) {
		PRIVATE_DATA->raSlewRate = PRIVATE_DATA->decSlewRate = 6;
	} else if (MOUNT_SLEW_RATE_MAX_ITEM->sw.value) {
		PRIVATE_DATA->raSlewRate = PRIVATE_DATA->decSlewRate = 9;
	} else {
		MOUNT_SLEW_RATE_GUIDE_ITEM->sw.value = true;
		PRIVATE_DATA->raSlewRate = PRIVATE_DATA->decSlewRate = PRIVATE_DATA->guide_rate;
	}
	MOUNT_SLEW_RATE_PROPERTY->state = INDIGO_OK_STATE;
	indigo_update_property(device, MOUNT_SLEW_RATE_PROPERTY, "slew speed = %d", PRIVATE_DATA->raSlewRate);
}

static void mount_handle_motion_dec(indigo_device *device) {
	char message[INDIGO_VALUE_SIZE];
	bool ok;

	if (PRIVATE_DATA->slew_rate == 0)
		mount_handle_slew_rate(device);

	double axisRate = decRates[PRIVATE_DATA->decSlewRate] * SIDEREAL_RATE;
	if(MOUNT_MOTION_NORTH_ITEM->sw.value) {
		ok = synscan_slew_axis_at_rate(device, kAxisDEC, -axisRate);
		strncpy(message,"Moving North...",sizeof(message));
		MOUNT_MOTION_DEC_PROPERTY->state = INDIGO_BUSY_STATE;
	} else if (MOUNT_MOTION_SOUTH_ITEM->sw.value) {
		ok = synscan_slew_axis_at_rate(device, kAxisDEC, axisRate);
		strncpy(message,"Moving South...",sizeof(message));
		MOUNT_MOTION_DEC_PROPERTY->state = INDIGO_BUSY_STATE;
	} else {
		synscan_stop_and_resume_tracking_for_axis(device, kAxisRA);
		strncpy(message,"Stopped moving.",sizeof(message));
		MOUNT_MOTION_DEC_PROPERTY->state = INDIGO_OK_STATE;
	}
	indigo_update_property(device, MOUNT_MOTION_DEC_PROPERTY, message);
}

static void mount_handle_motion_ra(indigo_device *device) {
	char message[INDIGO_VALUE_SIZE];
	bool ok;

	if (PRIVATE_DATA->slew_rate == 0)
		mount_handle_slew_rate(device);

	double axisRate = raRates[PRIVATE_DATA->raSlewRate] * SIDEREAL_RATE;
	if (MOUNT_MOTION_WEST_ITEM->sw.value) {
		ok = synscan_slew_axis_at_rate(device, kAxisRA, axisRate);
		strncpy(message,"Moving West...",sizeof(message));
		MOUNT_MOTION_RA_PROPERTY->state = INDIGO_BUSY_STATE;
	} else if (MOUNT_MOTION_EAST_ITEM->sw.value) {
		ok = synscan_slew_axis_at_rate(device, kAxisRA, -axisRate);
		strncpy(message,"Moving East...",sizeof(message));
		MOUNT_MOTION_DEC_PROPERTY->state = INDIGO_BUSY_STATE;
	}
	else {
		synscan_stop_and_resume_tracking_for_axis(device, kAxisRA);
		strncpy(message,"Stopped moving.",sizeof(message));
		MOUNT_MOTION_DEC_PROPERTY->state = INDIGO_OK_STATE;
	}
	indigo_update_property(device, MOUNT_MOTION_RA_PROPERTY, NULL);
}

#if 0
static bool mount_set_location(indigo_device *device) {
	int res;
	double lon = MOUNT_GEOGRAPHIC_COORDINATES_LONGITUDE_ITEM->number.value;
	if (lon > 180) lon -= 360.0;
	pthread_mutex_lock(&PRIVATE_DATA->serial_mutex);
	res = tc_set_location(PRIVATE_DATA->dev_id, lon, MOUNT_GEOGRAPHIC_COORDINATES_LATITUDE_ITEM->number.value);
	pthread_mutex_unlock(&PRIVATE_DATA->serial_mutex);
	if (res != RC_OK) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "tc_set_location(%d) = %d", PRIVATE_DATA->dev_id, res);
		return false;
	}
	return true;
}


static void mount_handle_st4_guiding_rate(indigo_device *device) {
	int dev_id = PRIVATE_DATA->dev_id;
	int res = RC_OK;

	int offset = 1;                                             /* for Ceslestron 0 is 1% and 99 is 100% */
	if (PRIVATE_DATA->vendor_id == VNDR_SKYWATCHER) offset = 0; /* there is no offset for Sky-Watcher */

	MOUNT_GUIDE_RATE_PROPERTY->state = INDIGO_OK_STATE;

	pthread_mutex_lock(&PRIVATE_DATA->serial_mutex);
	/* reset only if input value is changed - better begaviour for Sky-Watcher as there are no separate RA and DEC rates */
	if ((int)(MOUNT_GUIDE_RATE_RA_ITEM->number.value) != PRIVATE_DATA->st4_ra_rate) {
		res = tc_set_autoguide_rate(dev_id, TC_AXIS_RA, (int)(MOUNT_GUIDE_RATE_RA_ITEM->number.value)-1);
		if (res != RC_OK) {
			INDIGO_DRIVER_ERROR(DRIVER_NAME, "tc_set_autoguide_rate(%d) = %d", dev_id, res);
			MOUNT_GUIDE_RATE_PROPERTY->state = INDIGO_ALERT_STATE;
		} else {
			PRIVATE_DATA->st4_ra_rate = (int)(MOUNT_GUIDE_RATE_RA_ITEM->number.value);
		}
	}

	/* reset only if input value is changed - better begaviour for Sky-Watcher as there are no separate RA and DEC rates */
	if ((int)(MOUNT_GUIDE_RATE_DEC_ITEM->number.value) != PRIVATE_DATA->st4_dec_rate) {
		res = tc_set_autoguide_rate(dev_id, TC_AXIS_DE, (int)(MOUNT_GUIDE_RATE_DEC_ITEM->number.value)-1);
		if (res != RC_OK) {
			INDIGO_DRIVER_ERROR(DRIVER_NAME, "tc_set_autoguide_rate(%d) = %d", dev_id, res);
			MOUNT_GUIDE_RATE_PROPERTY->state = INDIGO_ALERT_STATE;
		} else {
			PRIVATE_DATA->st4_dec_rate = (int)(MOUNT_GUIDE_RATE_DEC_ITEM->number.value);
		}
	}

	/* read set values as Sky-Watcher rounds to 12, 25 ,50, 75 and 100 % */
	int st4_ra_rate = tc_get_autoguide_rate(dev_id, TC_AXIS_RA);
	if (st4_ra_rate < 0) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "tc_get_autoguide_rate(%d) = %d", dev_id, st4_ra_rate);
	} else {
		MOUNT_GUIDE_RATE_RA_ITEM->number.value = st4_ra_rate + offset;
	}

	int st4_dec_rate = tc_get_autoguide_rate(dev_id, TC_AXIS_DE);
	if (st4_dec_rate < 0) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "tc_get_autoguide_rate(%d) = %d", dev_id, st4_dec_rate);
	} else {
		MOUNT_GUIDE_RATE_DEC_ITEM->number.value = st4_dec_rate + offset;
	}

	pthread_mutex_unlock(&PRIVATE_DATA->serial_mutex);

	indigo_update_property(device, MOUNT_GUIDE_RATE_PROPERTY, NULL);
}


static void mount_handle_utc(indigo_device *device) {
	time_t utc_time = indigo_isototime(MOUNT_UTC_ITEM->text.value);
	if (utc_time == -1) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "Wrong date/time format!");
		MOUNT_UTC_TIME_PROPERTY->state = INDIGO_ALERT_STATE;
		indigo_update_property(device, MOUNT_UTC_TIME_PROPERTY, "Wrong date/time format!");
		return;
	}

	pthread_mutex_lock(&PRIVATE_DATA->serial_mutex);
	/* set mount time to UTC */
	int res = tc_set_time(PRIVATE_DATA->dev_id, utc_time, 0, 0);
	pthread_mutex_unlock(&PRIVATE_DATA->serial_mutex);

	if (res != RC_OK) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "tc_set_time(%d) = %d", PRIVATE_DATA->dev_id, res);
		MOUNT_UTC_TIME_PROPERTY->state = INDIGO_ALERT_STATE;
		indigo_update_property(device, MOUNT_UTC_TIME_PROPERTY, "Can not set mount date/time.");
		return;
	}

	MOUNT_UTC_TIME_PROPERTY->state = INDIGO_OK_STATE;
	indigo_update_property(device, MOUNT_UTC_TIME_PROPERTY, NULL);
	return;
}

static bool mount_set_utc_from_host(indigo_device *device) {
	time_t utc_time = indigo_utc(NULL);
	if (utc_time == -1) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "Can not get host UT");
		return false;
	}

	pthread_mutex_lock(&PRIVATE_DATA->serial_mutex);
	/* set mount time to UTC */
	int res = tc_set_time(PRIVATE_DATA->dev_id, utc_time, 0, 0);
	pthread_mutex_unlock(&PRIVATE_DATA->serial_mutex);

	if (res != RC_OK) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "tc_set_time(%d) = %d", PRIVATE_DATA->dev_id, res);
		return false;
	}
	return true;
}
#endif

static bool mount_cancel_slew(indigo_device *device) {


//	int res = tc_goto_cancel(PRIVATE_DATA->dev_id);
//	if (res != RC_OK) {
//		INDIGO_DRIVER_ERROR(DRIVER_NAME, "tc_goto_cancel(%d) = %d", PRIVATE_DATA->dev_id, res);
//	}

	MOUNT_MOTION_NORTH_ITEM->sw.value = false;
	MOUNT_MOTION_SOUTH_ITEM->sw.value = false;
	MOUNT_MOTION_DEC_PROPERTY->state = INDIGO_OK_STATE;
	indigo_update_property(device, MOUNT_MOTION_DEC_PROPERTY, NULL);

	MOUNT_MOTION_WEST_ITEM->sw.value = false;
	MOUNT_MOTION_EAST_ITEM->sw.value = false;
	MOUNT_MOTION_RA_PROPERTY->state = INDIGO_OK_STATE;
	indigo_update_property(device, MOUNT_MOTION_RA_PROPERTY, NULL);

	MOUNT_EQUATORIAL_COORDINATES_RA_ITEM->number.target = MOUNT_EQUATORIAL_COORDINATES_RA_ITEM->number.value;
	MOUNT_EQUATORIAL_COORDINATES_DEC_ITEM->number.target = MOUNT_EQUATORIAL_COORDINATES_DEC_ITEM->number.value;
	MOUNT_EQUATORIAL_COORDINATES_PROPERTY->state = INDIGO_OK_STATE;
	indigo_update_coordinates(device, NULL);

	MOUNT_ABORT_MOTION_ITEM->sw.value = false;
	MOUNT_ABORT_MOTION_PROPERTY->state = INDIGO_OK_STATE;
	indigo_update_property(device, MOUNT_ABORT_MOTION_PROPERTY, "Aborted.");
	return true;
}

#if 0
static void park_timer_callback(indigo_device *device) {
	int res;
	int dev_id = PRIVATE_DATA->dev_id;

	if (dev_id < 0) return;

	pthread_mutex_lock(&PRIVATE_DATA->serial_mutex);
	if (tc_goto_in_progress(dev_id)) {
		MOUNT_PARK_PROPERTY->state = INDIGO_BUSY_STATE;
		PRIVATE_DATA->park_in_progress = true;
	} else {
		res = tc_set_tracking_mode(dev_id, TC_TRACK_OFF);
		if (res != RC_OK) {
			INDIGO_DRIVER_ERROR(DRIVER_NAME, "tc_set_tracking_mode(%d) = %d", dev_id, res);
		} else {
			MOUNT_TRACKING_OFF_ITEM->sw.value = true;
			MOUNT_TRACKING_ON_ITEM->sw.value = false;
			indigo_update_property(device, MOUNT_TRACKING_PROPERTY, NULL);
		}
		MOUNT_PARK_PROPERTY->state = INDIGO_OK_STATE;
		PRIVATE_DATA->park_in_progress = false;
	}
	pthread_mutex_unlock(&PRIVATE_DATA->serial_mutex);

	if (PRIVATE_DATA->park_in_progress) {
		indigo_reschedule_timer(device, REFRESH_SECONDS, &PRIVATE_DATA->park_timer);
	} else {
		PRIVATE_DATA->park_timer = NULL;
		indigo_update_property(device, MOUNT_PARK_PROPERTY, "Mount Parked.");
	}
}
#endif

// -------------------------------------------------------------------------------- INDIGO MOUNT device implementation

static void position_timer_callback(indigo_device *device) {
	if (PRIVATE_DATA->handle > 0 && !PRIVATE_DATA->parked && PRIVATE_DATA->slew_state == SLEW_NONE) {
		//  Get the raw coords
		synscan_get_coords(device);
//		double diffRA = fabs(MOUNT_EQUATORIAL_COORDINATES_RA_ITEM->number.target - PRIVATE_DATA->currentRA);
//		double diffDec = fabs(MOUNT_EQUATORIAL_COORDINATES_DEC_ITEM->number.target - PRIVATE_DATA->currentDec);
//		if (diffRA <= RA_MIN_DIF && diffDec <= DEC_MIN_DIF)
			MOUNT_EQUATORIAL_COORDINATES_PROPERTY->state = INDIGO_OK_STATE;
//		else
//			MOUNT_EQUATORIAL_COORDINATES_PROPERTY->state = INDIGO_BUSY_STATE;

		//  Convert Encoder position to EQ
		coords_encoder_to_eq(device, PRIVATE_DATA->raPosition, PRIVATE_DATA->decPosition, &PRIVATE_DATA->currentHA, &PRIVATE_DATA->currentDec);

		//  Add in LST to get RA
		double lng = MOUNT_GEOGRAPHIC_COORDINATES_LONGITUDE_ITEM->number.value;
		double lst = indigo_lst(lng) * M_PI / 12.0;
		PRIVATE_DATA->currentRA = lst - PRIVATE_DATA->currentHA;
		if (PRIVATE_DATA->currentRA < 0)
			PRIVATE_DATA->currentRA += 24.0;
		PRIVATE_DATA->currentRA *= 12.0 / M_PI;
		PRIVATE_DATA->currentDec *= 180.0 / M_PI;

		printf("LST: %g\nHA: %g\n", lst * 12.0 / M_PI, PRIVATE_DATA->currentHA);

		MOUNT_EQUATORIAL_COORDINATES_RA_ITEM->number.value = PRIVATE_DATA->currentRA;
		MOUNT_EQUATORIAL_COORDINATES_DEC_ITEM->number.value = PRIVATE_DATA->currentDec;
		indigo_update_coordinates(device, NULL);

		//meade_get_utc(device);
		//indigo_update_property(device, MOUNT_UTC_TIME_PROPERTY, NULL);
	}
	indigo_reschedule_timer(device, 0.5, &PRIVATE_DATA->position_timer);
}

static indigo_result mount_attach(indigo_device *device) {
	assert(device != NULL);
	assert(PRIVATE_DATA != NULL);
	if (indigo_mount_attach(device, DRIVER_VERSION) == INDIGO_OK) {
		// -------------------------------------------------------------------------------- SIMULATION
		SIMULATION_PROPERTY->hidden = true;
		// -------------------------------------------------------------------------------- MOUNT_PARK
		MOUNT_PARK_PROPERTY->count = 2;
		MOUNT_PARK_PARKED_ITEM->sw.value = false;
		MOUNT_PARK_UNPARKED_ITEM->sw.value = true;
		// -------------------------------------------------------------------------------- MOUNT_ON_COORDINATES_SET
		MOUNT_ON_COORDINATES_SET_PROPERTY->count = 2;
		// -------------------------------------------------------------------------------- DEVICE_PORT
		DEVICE_PORT_PROPERTY->hidden = false;
		// -------------------------------------------------------------------------------- DEVICE_PORTS
		DEVICE_PORTS_PROPERTY->hidden = false;
		// -------------------------------------------------------------------------------- MOUNT_TRACK_RATE
		MOUNT_TRACK_RATE_PROPERTY->hidden = false;
		// -------------------------------------------------------------------------------- MOUNT_GEOGRAPHIC_COORDINATES
		MOUNT_GEOGRAPHIC_COORDINATES_PROPERTY->hidden = false;
		MOUNT_GEOGRAPHIC_COORDINATES_PROPERTY->count = 3; // we can not set elevation from the protocol
		// -------------------------------------------------------------------------------- ALIGNMENT_MODE
		//		ALIGNMENT_MODE_PROPERTY = indigo_init_switch_property(NULL, device->name, ALIGNMENT_MODE_PROPERTY_NAME, MOUNT_MAIN_GROUP, "Alignment mode", INDIGO_IDLE_STATE, INDIGO_RW_PERM, INDIGO_ONE_OF_MANY_RULE, 2);
//		if (ALIGNMENT_MODE_PROPERTY == NULL)
//			return INDIGO_FAILED;
//		indigo_init_switch_item(POLAR_MODE_ITEM, POLAR_MODE_ITEM_NAME, "Polar mode", false);
//		indigo_init_switch_item(ALTAZ_MODE_ITEM, ALTAZ_MODE_ITEM_NAME, "Alt/Az mode", false);
//		ALIGNMENT_MODE_PROPERTY->hidden = true;
		// --------------------------------------------------------------------------------
		MOUNT_LST_TIME_PROPERTY->hidden = true;
		MOUNT_UTC_TIME_PROPERTY->hidden = true;
		//MOUNT_UTC_TIME_PROPERTY->count = 1;
		//MOUNT_UTC_TIME_PROPERTY->perm = INDIGO_RO_PERM;
		MOUNT_SET_HOST_TIME_PROPERTY->hidden = true;

		// -------------------------------------------------------------------------------- MOUNT_POLARSCOPE
		MOUNT_POLARSCOPE_PROPERTY = indigo_init_number_property(NULL, device->name, MOUNT_POLARSCOPE_PROPERTY_NAME, MOUNT_MAIN_GROUP, "Polarscope", INDIGO_IDLE_STATE, INDIGO_RW_PERM, 1);
		if (MOUNT_POLARSCOPE_PROPERTY == NULL)
			return INDIGO_FAILED;
		MOUNT_POLARSCOPE_PROPERTY->hidden = true;
		indigo_init_number_item(MOUNT_POLARSCOPE_BRIGHTNESS_ITEM, MOUNT_POLARSCOPE_BRIGHTNESS_ITEM_NAME, "Polarscope Brightness", 0, 255, 0, 0);

		//strncpy(MOUNT_GUIDE_RATE_PROPERTY->label,"ST4 guide rate", INDIGO_VALUE_SIZE);

		PRIVATE_DATA->mountConfigured = false;

		pthread_mutex_init(&PRIVATE_DATA->port_mutex, NULL);
		INDIGO_DEVICE_ATTACH_LOG(DRIVER_NAME, device->name);
		return indigo_mount_enumerate_properties(device, NULL, NULL);
	}
	return INDIGO_FAILED;
}

static indigo_result mount_change_property(indigo_device *device, indigo_client *client, indigo_property *property) {
	assert(device != NULL);
	assert(DEVICE_CONTEXT != NULL);
	assert(property != NULL);

	if (indigo_property_match(CONNECTION_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- CONNECTION
		indigo_property_copy_values(CONNECTION_PROPERTY, property, false);
		if (CONNECTION_CONNECTED_ITEM->sw.value) {
			bool result = true;
			if (PRIVATE_DATA->device_count++ == 0) {
				CONNECTION_PROPERTY->state = INDIGO_BUSY_STATE;
				indigo_update_property(device, CONNECTION_PROPERTY, NULL);
				result = synscan_open(device);
			}
			if (result) {
				if (!synscan_configure(device)) {
					PRIVATE_DATA->device_count--;
					CONNECTION_PROPERTY->state = INDIGO_ALERT_STATE;
					indigo_set_switch(CONNECTION_PROPERTY, CONNECTION_DISCONNECTED_ITEM, true);
					return INDIGO_OK;
				}
//				char response[20];
//				if (synscan_command(device, SYNSCAN_FIRMWARE_VERSION, response, 19, 0)) {
//					INDIGO_DRIVER_LOG(DRIVER_NAME, "firmware:  %s", response);
//					//strncpy(PRIVATE_DATA->product, response, 64);
//				}
//				ALIGNMENT_MODE_PROPERTY->hidden = true;
//				if (!strcmp(PRIVATE_DATA->product, "EQMac")) {
//					MOUNT_SET_HOST_TIME_PROPERTY->hidden = true;
//					MOUNT_UTC_TIME_PROPERTY->hidden = true;
//					MOUNT_TRACKING_PROPERTY->hidden = true;
//					MOUNT_PARK_PROPERTY->count = 2; // Can unpark!
//					meade_get_coords(device);
//					if (PRIVATE_DATA->currentRA == 0 && PRIVATE_DATA->currentDec == 0) {
//						MOUNT_PARK_PARKED_ITEM->sw.value = true;
//						MOUNT_PARK_UNPARKED_ITEM->sw.value = false;
//						PRIVATE_DATA->parked = true;
//					} else {
//						MOUNT_PARK_PARKED_ITEM->sw.value = false;
//						MOUNT_PARK_UNPARKED_ITEM->sw.value = true;
//						PRIVATE_DATA->parked = false;
//					}
//					strcpy(MOUNT_INFO_VENDOR_ITEM->text.value, "Generic");
//					strcpy(MOUNT_INFO_MODEL_ITEM->text.value, "EQMac");
//					strcpy(MOUNT_INFO_FIRMWARE_ITEM->text.value, "N/A");
//				} else {
					//MOUNT_SET_HOST_TIME_PROPERTY->hidden = true;
					//MOUNT_UTC_TIME_PROPERTY->hidden = false;
					//MOUNT_PARK_PARKED_ITEM->sw.value = false;
					MOUNT_TRACKING_PROPERTY->hidden = false;
					PRIVATE_DATA->trackingMode = kTrackingModeSidereal;
					PRIVATE_DATA->parked = false;
					PRIVATE_DATA->raSlewRate = PRIVATE_DATA->decSlewRate = 4;
					strcpy(MOUNT_INFO_VENDOR_ITEM->text.value, "Sky-Watcher");
				strcpy(MOUNT_INFO_MODEL_ITEM->text.value, "SynScan");
//					if (meade_command(device, ":GVF#", response, 127, 0)) {
//						INDIGO_DRIVER_LOG(DRIVER_NAME, "version:  %s", response);
//						char *sep = strchr(response, '|');
//						if (sep != NULL)
//							*sep = 0;
//						strncpy(MOUNT_INFO_MODEL_ITEM->text.value, response, INDIGO_VALUE_SIZE);
//					} else {
//						strncpy(MOUNT_INFO_MODEL_ITEM->text.value, PRIVATE_DATA->product, INDIGO_VALUE_SIZE);
//					}
//					if (synscan_command(device, SYNSCAN_FIRMWARE_VERSION, response, 19, 0)) {
//						INDIGO_DRIVER_LOG(DRIVER_NAME, "firmware: %s", response);
//						strncpy(MOUNT_INFO_FIRMWARE_ITEM->text.value, response, INDIGO_VALUE_SIZE);
//					}
//					if (meade_command(device, ":GW#", response, 127, 0)) {
//						INDIGO_DRIVER_LOG(DRIVER_NAME, "status:   %s", response);
//						ALIGNMENT_MODE_PROPERTY->hidden = false;
//						if (*response == 'P') {
//							POLAR_MODE_ITEM->sw.value = true;
//							MOUNT_TRACKING_ON_ITEM->sw.value = true;
//							MOUNT_TRACKING_OFF_ITEM->sw.value = false;
//						} else if (*response == 'A') {
//							ALTAZ_MODE_ITEM->sw.value = true;
//							MOUNT_TRACKING_ON_ITEM->sw.value = true;
//							MOUNT_TRACKING_OFF_ITEM->sw.value = false;
//						} else {
//							ALTAZ_MODE_ITEM->sw.value = true;
//							MOUNT_TRACKING_ON_ITEM->sw.value = false;
//							MOUNT_TRACKING_OFF_ITEM->sw.value = true;
//						}
//
//						indigo_define_property(device, ALIGNMENT_MODE_PROPERTY, NULL);
//					}
//					if (meade_command(device, ":Gt#", response, 127, 0))
//						MOUNT_GEOGRAPHIC_COORDINATES_LATITUDE_ITEM->number.target = MOUNT_GEOGRAPHIC_COORDINATES_LATITUDE_ITEM->number.value = indigo_stod(response);
//					if (meade_command(device, ":Gg#", response, 127, 0))
//						MOUNT_GEOGRAPHIC_COORDINATES_LONGITUDE_ITEM->number.target = MOUNT_GEOGRAPHIC_COORDINATES_LONGITUDE_ITEM->number.value = indigo_stod(response);
//					meade_get_coords(device);
//					meade_get_utc(device);
//				}
				if (!PRIVATE_DATA->parked)
					PRIVATE_DATA->position_timer = indigo_set_timer(device, 0, position_timer_callback);
				CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
			} else {
				PRIVATE_DATA->device_count--;
				CONNECTION_PROPERTY->state = INDIGO_ALERT_STATE;
				indigo_set_switch(CONNECTION_PROPERTY, CONNECTION_DISCONNECTED_ITEM, true);
			}
		} else {
			indigo_cancel_timer(device, &PRIVATE_DATA->position_timer);
			PRIVATE_DATA->position_timer = NULL;
			if (--PRIVATE_DATA->device_count == 0) {
				synscan_close(device);
			}
//			if (!ALIGNMENT_MODE_PROPERTY->hidden)
//				indigo_delete_property(device, ALIGNMENT_MODE_PROPERTY, NULL);
			CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
		}
	}
#if 0
	if (indigo_property_match(CONNECTION_PROPERTY, property)) {
		indigo_property_copy_values(CONNECTION_PROPERTY, property, false);
		if (CONNECTION_CONNECTED_ITEM->sw.value) {
			if (!device->is_connected) {
				if (mount_open(device)) {
					int dev_id = PRIVATE_DATA->dev_id;
					CONNECTION_PROPERTY->state = INDIGO_OK_STATE;

					/* initialize info prop */
					int vendor_id = guess_mount_vendor(dev_id);
					if (vendor_id < 0) {
						INDIGO_DRIVER_ERROR(DRIVER_NAME, "guess_mount_vendor(%d) = %d", dev_id, vendor_id);
					} else if (vendor_id == VNDR_SKYWATCHER) {
						strncpy(MOUNT_INFO_VENDOR_ITEM->text.value, "Sky-Watcher", INDIGO_VALUE_SIZE);
					} else if (vendor_id == VNDR_CELESTRON) {
						strncpy(MOUNT_INFO_VENDOR_ITEM->text.value, "Celestron", INDIGO_VALUE_SIZE);
					}
					PRIVATE_DATA->vendor_id = vendor_id;

					int model_id = tc_get_model(dev_id);
					if (model_id < 0) {
						INDIGO_DRIVER_ERROR(DRIVER_NAME, "tc_get_model(%d) = %d", dev_id, model_id);
					} else {
						get_model_name(model_id,MOUNT_INFO_MODEL_ITEM->text.value,  INDIGO_VALUE_SIZE);
					}

					int firmware = tc_get_version(dev_id, NULL, NULL);
					if (firmware < 0) {
						INDIGO_DRIVER_ERROR(DRIVER_NAME, "tc_get_version(%d) = %d", dev_id, firmware);
					} else {
						if (vendor_id == VNDR_SKYWATCHER) {
							snprintf(MOUNT_INFO_FIRMWARE_ITEM->text.value, INDIGO_VALUE_SIZE, "%2d.%02d.%02d", GET_RELEASE(firmware), GET_REVISION(firmware), GET_PATCH(firmware));
						} else {
							snprintf(MOUNT_INFO_FIRMWARE_ITEM->text.value, INDIGO_VALUE_SIZE, "%2d.%02d", GET_RELEASE(firmware), GET_REVISION(firmware));
						}
					}

					/* initialize guidingrate prop */
					int offset = 1;                                             /* for Ceslestron 0 is 1% and 99 is 100% */
					if (PRIVATE_DATA->vendor_id == VNDR_SKYWATCHER) offset = 0; /* there is no offset for Sky-Watcher */

					int st4_ra_rate = tc_get_autoguide_rate(dev_id, TC_AXIS_RA);
					if (st4_ra_rate < 0) {
						INDIGO_DRIVER_ERROR(DRIVER_NAME, "tc_get_autoguide_rate(%d) = %d", dev_id, st4_ra_rate);
					} else {
						MOUNT_GUIDE_RATE_RA_ITEM->number.value = st4_ra_rate + offset;
						PRIVATE_DATA->st4_ra_rate = st4_ra_rate + offset;
					}

					int st4_dec_rate = tc_get_autoguide_rate(dev_id, TC_AXIS_DE);
					if (st4_dec_rate < 0) {
						INDIGO_DRIVER_ERROR(DRIVER_NAME, "tc_get_autoguide_rate(%d) = %d", dev_id, st4_dec_rate);
					} else {
						MOUNT_GUIDE_RATE_DEC_ITEM->number.value = st4_dec_rate + offset;
						PRIVATE_DATA->st4_dec_rate = st4_dec_rate + offset;
					}

					/* initialize tracking prop */
					int mode = tc_get_tracking_mode(PRIVATE_DATA->dev_id);
					if (mode < 0) {
						INDIGO_DRIVER_ERROR(DRIVER_NAME, "tc_get_tracking_mode(%d) = %d", PRIVATE_DATA->dev_id, mode);
					} else if (mode == TC_TRACK_OFF) {
						MOUNT_TRACKING_OFF_ITEM->sw.value = true;
						MOUNT_TRACKING_ON_ITEM->sw.value = false;
						MOUNT_TRACKING_PROPERTY->state = INDIGO_OK_STATE;
						indigo_update_property(device, MOUNT_TRACKING_PROPERTY, NULL);
					} else {
						MOUNT_TRACKING_OFF_ITEM->sw.value = false;
						MOUNT_TRACKING_ON_ITEM->sw.value = true;
						MOUNT_TRACKING_PROPERTY->state = INDIGO_OK_STATE;
						indigo_update_property(device, MOUNT_TRACKING_PROPERTY, NULL);
					}

					device->is_connected = true;
					/* start updates */
					PRIVATE_DATA->position_timer = indigo_set_timer(device, 0, position_timer_callback);
				} else {
					CONNECTION_PROPERTY->state = INDIGO_ALERT_STATE;
					indigo_set_switch(CONNECTION_PROPERTY, CONNECTION_DISCONNECTED_ITEM, true);
				}
			}
		} else {
			if (device->is_connected) {
				indigo_cancel_timer(device, &PRIVATE_DATA->position_timer);
				mount_close(device);
				device->is_connected = false;
				CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
			}
		}
	}
#endif
#if 0
	else if (indigo_property_match(MOUNT_PARK_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- MOUNT_PARK
		if(PRIVATE_DATA->park_in_progress) {
			indigo_update_property(device, MOUNT_PARK_PROPERTY, WARN_PARKING_PROGRESS_MSG);
			return INDIGO_OK;
		}
		indigo_property_copy_values(MOUNT_PARK_PROPERTY, property, false);
		if (MOUNT_PARK_PARKED_ITEM->sw.value) {
			PRIVATE_DATA->parked = true;  /* a but premature but need to cancel other movements from now on until unparked */
			PRIVATE_DATA->park_in_progress = true;

			pthread_mutex_lock(&PRIVATE_DATA->serial_mutex);
			int res = tc_goto_azalt_p(PRIVATE_DATA->dev_id, 0, 90);
			pthread_mutex_unlock(&PRIVATE_DATA->serial_mutex);
			if (res != RC_OK) {
				INDIGO_DRIVER_ERROR(DRIVER_NAME, "tc_goto_azalt_p(%d) = %d", PRIVATE_DATA->dev_id, res);
			}

			MOUNT_PARK_PROPERTY->state = INDIGO_BUSY_STATE;
			indigo_update_property(device, MOUNT_PARK_PROPERTY, "Parking...");
			PRIVATE_DATA->park_timer = indigo_set_timer(device, 2, park_timer_callback);
		} else {
			MOUNT_PARK_PROPERTY->state = INDIGO_BUSY_STATE;
			indigo_update_property(device, MOUNT_PARK_PROPERTY, "Unparking...");

			pthread_mutex_lock(&PRIVATE_DATA->serial_mutex);
			int res = tc_set_tracking_mode(PRIVATE_DATA->dev_id, TC_TRACK_EQ);
			pthread_mutex_unlock(&PRIVATE_DATA->serial_mutex);
			if (res != RC_OK) {
				INDIGO_DRIVER_ERROR(DRIVER_NAME, "tc_set_tracking_mode(%d) = %d", PRIVATE_DATA->dev_id, res);
			} else {
				MOUNT_TRACKING_OFF_ITEM->sw.value = false;
				MOUNT_TRACKING_ON_ITEM->sw.value = true;
				indigo_update_property(device, MOUNT_TRACKING_PROPERTY, NULL);
			}

			PRIVATE_DATA->parked = false;
			MOUNT_PARK_PROPERTY->state = INDIGO_OK_STATE;
			indigo_update_property(device, MOUNT_PARK_PROPERTY, "Mount unparked.");
		}
		return INDIGO_OK;
	}
#endif
#if 0
	else if (indigo_property_match(MOUNT_SET_HOST_TIME_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- MOUNT_SET_HOST_TIME_PROPERTY
		indigo_property_copy_values(MOUNT_SET_HOST_TIME_PROPERTY, property, false);
		if(MOUNT_SET_HOST_TIME_ITEM->sw.value) {
			if(mount_set_utc_from_host(device)) {
				MOUNT_SET_HOST_TIME_PROPERTY->state = INDIGO_OK_STATE;
			} else {
				MOUNT_SET_HOST_TIME_PROPERTY->state = INDIGO_ALERT_STATE;
			}
		}
		MOUNT_SET_HOST_TIME_ITEM->sw.value = false;
		MOUNT_SET_HOST_TIME_PROPERTY->state = INDIGO_OK_STATE;
		indigo_update_property(device, MOUNT_SET_HOST_TIME_PROPERTY, NULL);
		return INDIGO_OK;
	}
	else if (indigo_property_match(MOUNT_UTC_TIME_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- MOUNT_UTC_TIME_PROPERTY
		indigo_property_copy_values(MOUNT_UTC_TIME_PROPERTY, property, false);
		mount_handle_utc(device);
		return INDIGO_OK;
	}
#endif
	else if (indigo_property_match(MOUNT_GEOGRAPHIC_COORDINATES_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- MOUNT_GEOGRAPTHIC_COORDINATES
		indigo_property_copy_values(MOUNT_GEOGRAPHIC_COORDINATES_PROPERTY, property, false);
		if (MOUNT_GEOGRAPHIC_COORDINATES_LONGITUDE_ITEM->number.value < 0)
			MOUNT_GEOGRAPHIC_COORDINATES_LONGITUDE_ITEM->number.value += 360;
		MOUNT_GEOGRAPHIC_COORDINATES_PROPERTY->state = INDIGO_OK_STATE;
		indigo_update_property(device, MOUNT_GEOGRAPHIC_COORDINATES_PROPERTY, NULL);
		return INDIGO_OK;
	}
	else if (indigo_property_match(MOUNT_EQUATORIAL_COORDINATES_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- MOUNT_EQUATORIAL_COORDINATES
		if(PRIVATE_DATA->parked) {
			indigo_update_coordinates(device, WARN_PARKED_MSG);
			return INDIGO_OK;
		}
		indigo_property_copy_values(MOUNT_EQUATORIAL_COORDINATES_PROPERTY, property, false);
		mount_handle_coordinates(device);
		return INDIGO_OK;
	}
	else if (indigo_property_match(MOUNT_TRACKING_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- MOUNT_TRACKING
		if(PRIVATE_DATA->parked) {
			indigo_update_property(device, MOUNT_TRACKING_PROPERTY, WARN_PARKED_MSG);
			return INDIGO_OK;
		}
		indigo_property_copy_values(MOUNT_TRACKING_PROPERTY, property, false);
		mount_handle_tracking(device);
		return INDIGO_OK;
	}
	else if (indigo_property_match(MOUNT_SLEW_RATE_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- MOUNT_SLEW_RATE
		indigo_property_copy_values(MOUNT_SLEW_RATE_PROPERTY, property, false);
		mount_handle_slew_rate(device);
		return INDIGO_OK;
	}
	else if (indigo_property_match(MOUNT_MOTION_DEC_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- MOUNT_MOTION_DEC
		if(PRIVATE_DATA->parked) {
			MOUNT_MOTION_DEC_PROPERTY->state = INDIGO_ALERT_STATE;
			indigo_update_property(device, MOUNT_MOTION_DEC_PROPERTY, WARN_PARKED_MSG);
			return INDIGO_OK;
		}
		indigo_property_copy_values(MOUNT_MOTION_DEC_PROPERTY, property, false);
		mount_handle_motion_dec(device);
		return INDIGO_OK;
	}
	else if (indigo_property_match(MOUNT_MOTION_RA_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- MOUNT_MOTION_RA
		if (PRIVATE_DATA->parked) {
			MOUNT_MOTION_RA_PROPERTY->state = INDIGO_ALERT_STATE;
			indigo_update_property(device, MOUNT_MOTION_RA_PROPERTY, WARN_PARKED_MSG);
			return INDIGO_OK;
		}
		indigo_property_copy_values(MOUNT_MOTION_RA_PROPERTY, property, false);
		mount_handle_motion_ra(device);
		return INDIGO_OK;
	}
	else if (indigo_property_match(MOUNT_ABORT_MOTION_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- MOUNT_ABORT_MOTION
		indigo_property_copy_values(MOUNT_ABORT_MOTION_PROPERTY, property, false);
		mount_cancel_slew(device);
		return INDIGO_OK;
		// --------------------------------------------------------------------------------
	}
#ifdef NOT_IMPLEMENTED_YET
	else if (indigo_property_match(MOUNT_GUIDE_RATE_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- MOUNT_GUIDING
		indigo_property_copy_values(MOUNT_GUIDE_RATE_PROPERTY, property, false);
		//mount_handle_st4_guiding_rate(device);
		return INDIGO_OK;
	}
#endif
	return indigo_mount_change_property(device, client, property);
}

static indigo_result mount_detach(indigo_device *device) {
	assert(device != NULL);
	if (CONNECTION_CONNECTED_ITEM->sw.value)
		indigo_device_disconnect(NULL, device->name);
	//indigo_release_property(ALIGNMENT_MODE_PROPERTY);
	indigo_cancel_timer(device, &PRIVATE_DATA->position_timer);
	INDIGO_DEVICE_DETACH_LOG(DRIVER_NAME, device->name);
	return indigo_mount_detach(device);
}

// -------------------------------------------------------------------------------- INDIGO guider device implementation

static indigo_result synscan_guider_enumerate_properties(indigo_device *device, indigo_client *client, indigo_property *property) {
	if (IS_CONNECTED) {
		if (indigo_property_match(COMMAND_GUIDE_RATE_PROPERTY, property))
			indigo_define_property(device, COMMAND_GUIDE_RATE_PROPERTY, NULL);
	}
	return indigo_guider_enumerate_properties(device, NULL, NULL);
}

#if 0
static void guider_timer_callback_ra(indigo_device *device) {
	PRIVATE_DATA->guider_timer_ra = NULL;
	int dev_id = PRIVATE_DATA->dev_id;
	int res;

	pthread_mutex_lock(&PRIVATE_DATA->serial_mutex);
	res = tc_slew_fixed(dev_id, TC_AXIS_RA, TC_DIR_POSITIVE, 0); // STOP move
	pthread_mutex_unlock(&PRIVATE_DATA->serial_mutex);
	if (res != RC_OK) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "tc_slew_fixed(%d) = %d", dev_id, res);
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
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "tc_slew_fixed(%d) = %d", dev_id, res);
	}

	GUIDER_GUIDE_NORTH_ITEM->number.value = 0;
	GUIDER_GUIDE_SOUTH_ITEM->number.value = 0;
	GUIDER_GUIDE_DEC_PROPERTY->state = INDIGO_OK_STATE;
	indigo_update_property(device, GUIDER_GUIDE_DEC_PROPERTY, NULL);
}


static void guider_handle_guide_rate(indigo_device *device) {
	if(GUIDE_50_ITEM->sw.value) {
		PRIVATE_DATA->guide_rate = 1;
	} else if (GUIDE_100_ITEM->sw.value) {
		PRIVATE_DATA->guide_rate = 2;
	}
	COMMAND_GUIDE_RATE_PROPERTY->state = INDIGO_OK_STATE;
	if (PRIVATE_DATA->guide_rate == 1)
		indigo_update_property(device, COMMAND_GUIDE_RATE_PROPERTY, "Command guide rate set to 7.5\"/s (50%% sidereal).");
	else if (PRIVATE_DATA->guide_rate == 2)
		indigo_update_property(device, COMMAND_GUIDE_RATE_PROPERTY, "Command guide rate set to 15\"/s (100%% sidereal).");
	else
		indigo_update_property(device, COMMAND_GUIDE_RATE_PROPERTY, "Command guide rate set.");
}


static indigo_result guider_attach(indigo_device *device) {
	assert(device != NULL);
	assert(PRIVATE_DATA != NULL);
	if (indigo_guider_attach(device, DRIVER_VERSION) == INDIGO_OK) {
		PRIVATE_DATA->guide_rate = 1; /* 1 -> 0.5 siderial rate , 2 -> siderial rate */
		COMMAND_GUIDE_RATE_PROPERTY = indigo_init_switch_property(NULL, device->name, COMMAND_GUIDE_RATE_PROPERTY_NAME, GUIDER_MAIN_GROUP, "Guide rate", INDIGO_IDLE_STATE, INDIGO_RW_PERM, INDIGO_ONE_OF_MANY_RULE, 2);
		if (COMMAND_GUIDE_RATE_PROPERTY == NULL)
			return INDIGO_FAILED;
		indigo_init_switch_item(GUIDE_50_ITEM, GUIDE_50_ITEM_NAME, "50% sidereal", true);
		indigo_init_switch_item(GUIDE_100_ITEM, GUIDE_100_ITEM_NAME, "100% sidereal", false);

		INDIGO_DEVICE_ATTACH_LOG(DRIVER_NAME, device->name);
		return indigo_guider_enumerate_properties(device, NULL, NULL);
	}
	return INDIGO_FAILED;
}

static indigo_result guider_change_property(indigo_device *device, indigo_client *client, indigo_property *property) {
	assert(device != NULL);
	assert(DEVICE_CONTEXT != NULL);
	assert(property != NULL);
	// -------------------------------------------------------------------------------- CONNECTION
	if (indigo_property_match(CONNECTION_PROPERTY, property)) {
		indigo_property_copy_values(CONNECTION_PROPERTY, property, false);
		if (CONNECTION_CONNECTED_ITEM->sw.value) {
			if (!device->is_connected) {
				if (mount_open(device)) {
					device->is_connected = true;
					indigo_define_property(device, COMMAND_GUIDE_RATE_PROPERTY, NULL);
					PRIVATE_DATA->guider_timer_ra = NULL;
					PRIVATE_DATA->guider_timer_dec = NULL;
					CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
					GUIDER_GUIDE_DEC_PROPERTY->hidden = false;
					GUIDER_GUIDE_RA_PROPERTY->hidden = false;
				} else {
					CONNECTION_PROPERTY->state = INDIGO_ALERT_STATE;
					indigo_set_switch(CONNECTION_PROPERTY, CONNECTION_DISCONNECTED_ITEM, true);
				}
			}
		} else {
			if (device->is_connected) {
				mount_close(device);
				indigo_delete_property(device, COMMAND_GUIDE_RATE_PROPERTY, NULL);
				device->is_connected = false;
				CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
			}
		}
	} else if (indigo_property_match(GUIDER_GUIDE_DEC_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- GUIDER_GUIDE_DEC
		indigo_cancel_timer(device, &PRIVATE_DATA->guider_timer_dec);
		indigo_property_copy_values(GUIDER_GUIDE_DEC_PROPERTY, property, false);
		GUIDER_GUIDE_DEC_PROPERTY->state = INDIGO_OK_STATE;
		int duration = GUIDER_GUIDE_NORTH_ITEM->number.value;
		if (duration > 0) {
			pthread_mutex_lock(&PRIVATE_DATA->serial_mutex);
			int res = tc_slew_fixed(PRIVATE_DATA->dev_id, TC_AXIS_DE, TC_DIR_POSITIVE, PRIVATE_DATA->guide_rate);
			pthread_mutex_unlock(&PRIVATE_DATA->serial_mutex);
			if (res != RC_OK) {
				INDIGO_DRIVER_ERROR(DRIVER_NAME, "tc_slew_fixed(%d) = %d", PRIVATE_DATA->dev_id, res);
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
					INDIGO_DRIVER_ERROR(DRIVER_NAME, "tc_slew_fixed(%d) = %d", PRIVATE_DATA->dev_id, res);
				}
				GUIDER_GUIDE_DEC_PROPERTY->state = INDIGO_BUSY_STATE;
				PRIVATE_DATA->guider_timer_dec = indigo_set_timer(device, duration/1000.0, guider_timer_callback_dec);
			}
		}
		indigo_update_property(device, GUIDER_GUIDE_DEC_PROPERTY, NULL);
		return INDIGO_OK;
	} else if (indigo_property_match(GUIDER_GUIDE_RA_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- GUIDER_GUIDE_RA
		indigo_cancel_timer(device, &PRIVATE_DATA->guider_timer_ra);
		indigo_property_copy_values(GUIDER_GUIDE_RA_PROPERTY, property, false);
		GUIDER_GUIDE_RA_PROPERTY->state = INDIGO_OK_STATE;
		int duration = GUIDER_GUIDE_EAST_ITEM->number.value;
		if (duration > 0) {
			pthread_mutex_lock(&PRIVATE_DATA->serial_mutex);
			int res = tc_slew_fixed(PRIVATE_DATA->dev_id, TC_AXIS_RA, TC_DIR_POSITIVE, PRIVATE_DATA->guide_rate);
			pthread_mutex_unlock(&PRIVATE_DATA->serial_mutex);
			if (res != RC_OK) {
				INDIGO_DRIVER_ERROR(DRIVER_NAME, "tc_slew_fixed(%d) = %d", PRIVATE_DATA->dev_id, res);
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
					INDIGO_DRIVER_ERROR(DRIVER_NAME, "tc_slew_fixed(%d) = %d", PRIVATE_DATA->dev_id, res);
				}
				GUIDER_GUIDE_RA_PROPERTY->state = INDIGO_BUSY_STATE;
				PRIVATE_DATA->guider_timer_ra = indigo_set_timer(device, duration/1000.0, guider_timer_callback_ra);
			}
		}
		indigo_update_property(device, GUIDER_GUIDE_RA_PROPERTY, NULL);
		return INDIGO_OK;
	} else if (indigo_property_match(COMMAND_GUIDE_RATE_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- COMMAND_GUIDE_RATE
		indigo_property_copy_values(COMMAND_GUIDE_RATE_PROPERTY, property, false);
		guider_handle_guide_rate(device);
		return INDIGO_OK;
	}
	// --------------------------------------------------------------------------------
	return indigo_guider_change_property(device, client, property);
}

static indigo_result guider_detach(indigo_device *device) {
	assert(device != NULL);
	if (CONNECTION_CONNECTED_ITEM->sw.value)
		indigo_device_disconnect(NULL, device->name);

	indigo_release_property(COMMAND_GUIDE_RATE_PROPERTY);
	INDIGO_DEVICE_DETACH_LOG(DRIVER_NAME, device->name);
	return indigo_guider_detach(device);
}
#endif

// --------------------------------------------------------------------------------

static synscan_private_data *private_data = NULL;

static indigo_device *mount = NULL;
static indigo_device *mount_guider = NULL;

indigo_result indigo_mount_synscan(indigo_driver_action action, indigo_driver_info *info) {
	static indigo_device mount_template = INDIGO_DEVICE_INITIALIZER(
		MOUNT_SYNSCAN_NAME,
		mount_attach,
		indigo_mount_enumerate_properties,
		mount_change_property,
		NULL,
		mount_detach
	);
#if 0
	static indigo_device mount_guider_template = INDIGO_DEVICE_INITIALIZER(
		MOUNT_SYNSCAN_GUIDER_NAME,
		guider_attach,
		synscan_guider_enumerate_properties,
		guider_change_property,
		NULL,
		guider_detach
	);
#endif
	static indigo_driver_action last_action = INDIGO_DRIVER_SHUTDOWN;

	SET_DRIVER_INFO(info, "SynScan Mount", __FUNCTION__, DRIVER_VERSION, last_action);

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
			mount->private_data = private_data;
			indigo_attach_device(mount);

			//mount_guider = malloc(sizeof(indigo_device));
			//assert(mount_guider != NULL);
			//memcpy(mount_guider, &mount_guider_template, sizeof(indigo_device));
			//mount_guider->private_data = private_data;
			//indigo_attach_device(mount_guider);
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
