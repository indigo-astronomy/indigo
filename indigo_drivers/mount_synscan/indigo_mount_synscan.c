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

static const double raRates[] = { 1.25, 2, 8, 16, 32, 70, 100, 625, 725, 825 };
static const double decRates[] = { 0.5, 1, 8, 16, 32, 70, 100, 625, 725, 825 };

// -------------------------------------------------------------------------------- INDIGO MOUNT device implementation

static bool synscan_open(indigo_device *device) {
	char *name = "/dev/cu.usbserial";//DEVICE_PORT_ITEM->text.value;
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
	if (PRIVATE_DATA->globalMode != kGlobalModeIdle) {
		MOUNT_EQUATORIAL_COORDINATES_PROPERTY->state = INDIGO_ALERT_STATE;
	}
	else {
		MOUNT_EQUATORIAL_COORDINATES_PROPERTY->state = INDIGO_BUSY_STATE;

		/* GOTO requested */
		if(MOUNT_ON_COORDINATES_SET_SLEW_ITEM->sw.value || MOUNT_ON_COORDINATES_SET_TRACK_ITEM->sw.value) {
			PRIVATE_DATA->slew_state = SLEW_PHASE0;
			PRIVATE_DATA->globalMode = kGlobalModeSlewing;
		}
		/* SYNC requested */
		else if (MOUNT_ON_COORDINATES_SET_SYNC_ITEM->sw.value) {
	//		res = tc_sync_rade_p(PRIVATE_DATA->dev_id, h2d(MOUNT_EQUATORIAL_COORDINATES_RA_ITEM->number.value), MOUNT_EQUATORIAL_COORDINATES_DEC_ITEM->number.value);
	//		if (res != RC_OK) {
				MOUNT_EQUATORIAL_COORDINATES_PROPERTY->state = INDIGO_ALERT_STATE;
	//			INDIGO_DRIVER_ERROR(DRIVER_NAME, "tc_sync_rade_p(%d) = %d", PRIVATE_DATA->dev_id, res);
	//		}
		}
	}
	indigo_update_coordinates(device, NULL);
}

static void mount_handle_tracking(indigo_device *device) {
	if (PRIVATE_DATA->globalMode != kGlobalModeIdle) {
		MOUNT_TRACKING_PROPERTY->state = INDIGO_ALERT_STATE;
	}
	else {
		MOUNT_TRACKING_PROPERTY->state = INDIGO_OK_STATE;
		if (MOUNT_TRACKING_ON_ITEM->sw.value) {
			//  Get the desired tracking rate
			enum TrackingMode trackingMode = kTrackingModeOff;
			if (MOUNT_TRACK_RATE_SIDEREAL_ITEM->sw.value)
				trackingMode = kTrackingModeSidereal;
			else if (MOUNT_TRACK_RATE_SOLAR_ITEM->sw.value)
				trackingMode = kTrackingModeSolar;
			else if (MOUNT_TRACK_RATE_LUNAR_ITEM->sw.value)
				trackingMode = kTrackingModeLunar;

			//  Start tracking
			if (!synscan_start_tracking_mode(device, trackingMode)) {
				MOUNT_TRACKING_PROPERTY->state = INDIGO_ALERT_STATE;
			}
		}
		else if (MOUNT_TRACKING_OFF_ITEM->sw.value) {
			if (!synscan_start_tracking_mode(device, kTrackingModeOff)) {
				MOUNT_TRACKING_PROPERTY->state = INDIGO_ALERT_STATE;
			}
		}
	}
	indigo_update_property(device, MOUNT_TRACKING_PROPERTY, NULL);
}

static int mount_slew_rate(indigo_device* device) {
	if (MOUNT_SLEW_RATE_GUIDE_ITEM->sw.value)
		return 1;
	else if (MOUNT_SLEW_RATE_CENTERING_ITEM->sw.value)
		return 4;
	else if (MOUNT_SLEW_RATE_FIND_ITEM->sw.value)
		return 6;
	else if (MOUNT_SLEW_RATE_MAX_ITEM->sw.value)
		return 9;
	else
		return 1;
}

static void mount_handle_slew_rate(indigo_device *device) {
	MOUNT_SLEW_RATE_PROPERTY->state = INDIGO_OK_STATE;
	indigo_update_property(device, MOUNT_SLEW_RATE_PROPERTY, NULL);
}

static void mount_handle_motion_dec(indigo_device *device) {
	double axisRate = decRates[mount_slew_rate(device)] * SIDEREAL_RATE;
	if(MOUNT_MOTION_NORTH_ITEM->sw.value) {
		synscan_slew_axis_at_rate(device, kAxisDEC, -axisRate);
		MOUNT_MOTION_DEC_PROPERTY->state = INDIGO_BUSY_STATE;
	} else if (MOUNT_MOTION_SOUTH_ITEM->sw.value) {
		synscan_slew_axis_at_rate(device, kAxisDEC, axisRate);
		MOUNT_MOTION_DEC_PROPERTY->state = INDIGO_BUSY_STATE;
	} else {
		synscan_stop_and_resume_tracking_for_axis(device, kAxisDEC);
		MOUNT_MOTION_DEC_PROPERTY->state = INDIGO_OK_STATE;
	}
	indigo_update_property(device, MOUNT_MOTION_DEC_PROPERTY, NULL);
}

static void mount_handle_motion_ra(indigo_device *device) {
	double axisRate = raRates[mount_slew_rate(device)] * SIDEREAL_RATE;
	if (MOUNT_MOTION_WEST_ITEM->sw.value) {
		synscan_slew_axis_at_rate(device, kAxisRA, axisRate);
		MOUNT_MOTION_RA_PROPERTY->state = INDIGO_BUSY_STATE;
	} else if (MOUNT_MOTION_EAST_ITEM->sw.value) {
		synscan_slew_axis_at_rate(device, kAxisRA, -axisRate);
		MOUNT_MOTION_RA_PROPERTY->state = INDIGO_BUSY_STATE;
	}
	else {
		synscan_stop_and_resume_tracking_for_axis(device, kAxisRA);
		MOUNT_MOTION_RA_PROPERTY->state = INDIGO_OK_STATE;
	}
	indigo_update_property(device, MOUNT_MOTION_RA_PROPERTY, NULL);
}

#if 0
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
#endif

static bool mount_cancel_slew(indigo_device *device) {
	if (PRIVATE_DATA->globalMode == kGlobalModeParking || PRIVATE_DATA->globalMode == kGlobalModeSlewing) {
		PRIVATE_DATA->globalMode = kGlobalModeIdle;
	}
	PRIVATE_DATA->raDesiredAxisMode = kAxisModeIdle;
	PRIVATE_DATA->decDesiredAxisMode = kAxisModeIdle;

//	MOUNT_MOTION_NORTH_ITEM->sw.value = false;
//	MOUNT_MOTION_SOUTH_ITEM->sw.value = false;
//	MOUNT_MOTION_DEC_PROPERTY->state = INDIGO_OK_STATE;
//	indigo_update_property(device, MOUNT_MOTION_DEC_PROPERTY, NULL);
//
//	MOUNT_MOTION_WEST_ITEM->sw.value = false;
//	MOUNT_MOTION_EAST_ITEM->sw.value = false;
//	MOUNT_MOTION_RA_PROPERTY->state = INDIGO_OK_STATE;
//	indigo_update_property(device, MOUNT_MOTION_RA_PROPERTY, NULL);
//
//	MOUNT_EQUATORIAL_COORDINATES_RA_ITEM->number.target = MOUNT_EQUATORIAL_COORDINATES_RA_ITEM->number.value;
//	MOUNT_EQUATORIAL_COORDINATES_DEC_ITEM->number.target = MOUNT_EQUATORIAL_COORDINATES_DEC_ITEM->number.value;
//	MOUNT_EQUATORIAL_COORDINATES_PROPERTY->state = INDIGO_OK_STATE;
//	indigo_update_coordinates(device, NULL);

	MOUNT_ABORT_MOTION_ITEM->sw.value = false;
	MOUNT_ABORT_MOTION_PROPERTY->state = INDIGO_OK_STATE;
	indigo_update_property(device, MOUNT_ABORT_MOTION_PROPERTY, "Aborted.");
	return true;
}

static void park_timer_callback(indigo_device *device) {
	if (PRIVATE_DATA->globalMode == kGlobalModeParking) {
		switch (PRIVATE_DATA->park_state) {
			case PARK_NONE:
				break;
			case PARK_PHASE0:
				{
					//  Compute the axis positions for parking
					double ha = MOUNT_PARK_POSITION_HA_ITEM->number.value * M_PI / 12.0;
					double dec = MOUNT_PARK_POSITION_DEC_ITEM->number.value * M_PI / 180.0;
					double haPos[2], decPos[2];
					coords_eq_to_encoder2(device, ha, dec, haPos, decPos);

					//  Start the axes moving
					PRIVATE_DATA->raTargetPosition = haPos[0];
					PRIVATE_DATA->decTargetPosition = decPos[0];
					PRIVATE_DATA->raDesiredAxisMode = kAxisModeSlewing;
					PRIVATE_DATA->decDesiredAxisMode = kAxisModeSlewing;

					//  Move to next state
					PRIVATE_DATA->park_state = PARK_PHASE1;
				}
				break;

			case PARK_PHASE1:
				{
					//  Check for HA parked
					if (PRIVATE_DATA->raAxisMode != kAxisModeSlewIdle)
						break;

					//  Set desired mode to Idle
					PRIVATE_DATA->raDesiredAxisMode = kAxisModeIdle;

					//  Move to next state
					PRIVATE_DATA->park_state = PARK_PHASE2;
				}
				break;

			case PARK_PHASE2:
				{
					//  Check for DEC parked
					if (PRIVATE_DATA->decAxisMode != kAxisModeSlewIdle)
						break;

					//  Set desired mode to Idle
					PRIVATE_DATA->decDesiredAxisMode = kAxisModeIdle;

					//  Update state
					PRIVATE_DATA->park_state = PARK_NONE;
					PRIVATE_DATA->globalMode = kGlobalModeParked;
					PRIVATE_DATA->parked = true;
					MOUNT_PARK_PARKED_ITEM->sw.value = true;
					MOUNT_PARK_PROPERTY->state = INDIGO_OK_STATE;
				}
				break;
		}
	}
	if (PRIVATE_DATA->globalMode == kGlobalModeParking) {
		indigo_reschedule_timer(device, REFRESH_SECONDS, &PRIVATE_DATA->park_timer);
	}
	else {
		PRIVATE_DATA->park_timer = NULL;
		indigo_update_property(device, MOUNT_PARK_PROPERTY, "Mount Parked.");
	}
}

// -------------------------------------------------------------------------------- INDIGO MOUNT device implementation

static void position_timer_callback(indigo_device *device) {
	if (PRIVATE_DATA->handle > 0 && !PRIVATE_DATA->parked) {
		//  Longitude needed for LST
		double lng = MOUNT_GEOGRAPHIC_COORDINATES_LONGITUDE_ITEM->number.value;

		//  Get the raw coords
		synscan_get_coords(device);

		//  Get the LST as quickly as we can after the HA
		double lst = indigo_lst(lng) * M_PI / 12.0;

		//  Convert Encoder position to EQ
		coords_encoder_to_eq(device, PRIVATE_DATA->raPosition, PRIVATE_DATA->decPosition, &PRIVATE_DATA->currentHA, &PRIVATE_DATA->currentDec);

		//  Add in LST to get RA
		PRIVATE_DATA->currentRA = lst - PRIVATE_DATA->currentHA;
		PRIVATE_DATA->currentRA *= 12.0 / M_PI;
		PRIVATE_DATA->currentDec *= 180.0 / M_PI;
		if (PRIVATE_DATA->currentRA < 0)
			PRIVATE_DATA->currentRA += 24.0;
		if (PRIVATE_DATA->currentRA >= 24.0)
			PRIVATE_DATA->currentRA -= 24.0;

		printf("LST: %g\nHA: %g\n", lst * 12.0 / M_PI, PRIVATE_DATA->currentHA);

		//		double diffRA = fabs(MOUNT_EQUATORIAL_COORDINATES_RA_ITEM->number.target - PRIVATE_DATA->currentRA);
		//		double diffDec = fabs(MOUNT_EQUATORIAL_COORDINATES_DEC_ITEM->number.target - PRIVATE_DATA->currentDec);
		if (PRIVATE_DATA->slew_state == SLEW_NONE)
			MOUNT_EQUATORIAL_COORDINATES_PROPERTY->state = INDIGO_OK_STATE;
		else
			MOUNT_EQUATORIAL_COORDINATES_PROPERTY->state = INDIGO_BUSY_STATE;


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
		// -------------------------------------------------------------------------------- MOUNT_RAW_COORDINATES
		MOUNT_RAW_COORDINATES_PROPERTY->hidden = false;
		// -------------------------------------------------------------------------------- DEVICE_PORTS
		DEVICE_PORTS_PROPERTY->hidden = false;
		// -------------------------------------------------------------------------------- DEVICE_PORT
		DEVICE_PORT_PROPERTY->hidden = false;
		// -------------------------------------------------------------------------------- MOUNT_ALIGNMENT_MODE
		MOUNT_ALIGNMENT_MODE_PROPERTY->count = 2;
		indigo_set_switch(MOUNT_ALIGNMENT_MODE_PROPERTY, MOUNT_ALIGNMENT_MODE_SINGLE_POINT_ITEM, true);
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

		//  Do we need to support ST4 port and pulse guiding?

		pthread_mutex_init(&PRIVATE_DATA->port_mutex, NULL);
		PRIVATE_DATA->mountConfigured = false;

		INDIGO_DEVICE_ATTACH_LOG(DRIVER_NAME, device->name);

		indigo_define_property(device, MOUNT_POLARSCOPE_PROPERTY, NULL);
		indigo_define_property(device, OPERATING_MODE_PROPERTY, NULL);
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
				}
				else {
					strcpy(MOUNT_INFO_VENDOR_ITEM->text.value, "Sky-Watcher");
					strcpy(MOUNT_INFO_MODEL_ITEM->text.value, "SynScan");

					//  Start timers
					PRIVATE_DATA->ha_axis_timer = indigo_set_timer(device, 0, ha_axis_timer_callback);
					PRIVATE_DATA->dec_axis_timer = indigo_set_timer(device, 0, dec_axis_timer_callback);
					PRIVATE_DATA->position_timer = indigo_set_timer(device, 0, position_timer_callback);
					PRIVATE_DATA->slew_timer = indigo_set_timer(device, 0, slew_timer_callback);
					CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
				}
			}
			else {
				PRIVATE_DATA->device_count--;
				CONNECTION_PROPERTY->state = INDIGO_ALERT_STATE;
				indigo_set_switch(CONNECTION_PROPERTY, CONNECTION_DISCONNECTED_ITEM, true);
			}
		}
		else {
			indigo_cancel_timer(device, &PRIVATE_DATA->ha_axis_timer);
			indigo_cancel_timer(device, &PRIVATE_DATA->dec_axis_timer);
			indigo_cancel_timer(device, &PRIVATE_DATA->position_timer);
			indigo_cancel_timer(device, &PRIVATE_DATA->slew_timer);
			if (--PRIVATE_DATA->device_count == 0) {
				synscan_close(device);
			}
			CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
		}
		indigo_update_property(device, CONNECTION_PROPERTY, "");
	}
	else if (indigo_property_match(MOUNT_PARK_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- MOUNT_PARK
#if 0
		if(PRIVATE_DATA->park_in_progress) {
			indigo_update_property(device, MOUNT_PARK_PROPERTY, WARN_PARKING_PROGRESS_MSG);
			return INDIGO_OK;
		}
#endif
		indigo_property_copy_values(MOUNT_PARK_PROPERTY, property, false);
		if (MOUNT_PARK_PARKED_ITEM->sw.value) {
			if (PRIVATE_DATA->globalMode == kGlobalModeIdle) {
				MOUNT_PARK_PROPERTY->state = INDIGO_BUSY_STATE;
				indigo_update_property(device, MOUNT_PARK_PROPERTY, "Parking...");
				PRIVATE_DATA->globalMode = kGlobalModeParking;
				PRIVATE_DATA->park_state = PARK_PHASE0;
				PRIVATE_DATA->park_timer = indigo_set_timer(device, 0, park_timer_callback);
			}
			else {
				//  Can't park while mount is doing something else
				MOUNT_PARK_PROPERTY->state = INDIGO_ALERT_STATE;
				indigo_update_property(device, MOUNT_PARK_PROPERTY, "Mount busy!");
			}
		} else {
			MOUNT_PARK_PROPERTY->state = INDIGO_BUSY_STATE;
			indigo_update_property(device, MOUNT_PARK_PROPERTY, "Unparking...");

			//  We should try to move to an unpark position here - but we don't have one

			PRIVATE_DATA->parked = false;
			PRIVATE_DATA->globalMode = kGlobalModeIdle;
			MOUNT_PARK_PROPERTY->state = INDIGO_OK_STATE;
			indigo_update_property(device, MOUNT_PARK_PROPERTY, "Mount unparked.");
		}
		return INDIGO_OK;
	}
	else if (indigo_property_match(MOUNT_EQUATORIAL_COORDINATES_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- MOUNT_EQUATORIAL_COORDINATES
		if(PRIVATE_DATA->parked) {
			indigo_update_coordinates(device, WARN_PARKED_MSG);
			return INDIGO_OK;
		}
		int ra = MOUNT_EQUATORIAL_COORDINATES_RA_ITEM->number.value;
		int dec = MOUNT_EQUATORIAL_COORDINATES_DEC_ITEM->number.value;
		indigo_property_copy_values(MOUNT_EQUATORIAL_COORDINATES_PROPERTY, property, false);
		MOUNT_EQUATORIAL_COORDINATES_RA_ITEM->number.value = ra;
		MOUNT_EQUATORIAL_COORDINATES_DEC_ITEM->number.value = dec;
		mount_handle_coordinates(device);
		return INDIGO_OK;
	}
	else if (indigo_property_match(MOUNT_TRACK_RATE_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- MOUNT_TRACK_RATE
		indigo_property_copy_values(MOUNT_TRACK_RATE_PROPERTY, property, false);

		//  UPDATE THE TRACKING RATE OF THE AXIS IF WE ARE TRACKING

		MOUNT_TRACK_RATE_PROPERTY->state = INDIGO_OK_STATE;
		indigo_update_property(device, MOUNT_TRACK_RATE_PROPERTY, NULL);
		return INDIGO_OK;
	}
	else if (indigo_property_match(MOUNT_TRACKING_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- MOUNT_TRACKING (on/off)
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
		//  ADJUST SLEW RATE IF MANUAL SLEWING
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
		if (MOUNT_ABORT_MOTION_ITEM->sw.value)
			mount_cancel_slew(device);
		return INDIGO_OK;
		// --------------------------------------------------------------------------------
	}

//  Consider if we want other data saved/loaded in config

#ifdef NOT_IMPLEMENTED_YET
	else if (indigo_property_match(MOUNT_GUIDE_RATE_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- MOUNT_GUIDING
		indigo_property_copy_values(MOUNT_GUIDE_RATE_PROPERTY, property, false);
		//mount_handle_st4_guiding_rate(device);
		//MOUNT_GUIDE_RATE_PROPERTY->state = INDIGO_OK_STATE;
		//indigo_update_property(device, MOUNT_GUIDE_RATE_PROPERTY, NULL);
		return INDIGO_OK;
	}
#endif
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

static indigo_result synscan_guider_enumerate_properties(indigo_device *device, indigo_client *client, indigo_property *property) {
	if (IS_CONNECTED) {
//		if (indigo_property_match(COMMAND_GUIDE_RATE_PROPERTY, property))
//			indigo_define_property(device, COMMAND_GUIDE_RATE_PROPERTY, NULL);
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
		COMMAND_GUIDE_RATE_PROPERTY = indigo_init_switch_property(NULL, device->name, COMMAND_GUIDE_RATE_PROPERTY_NAME, GUIDER_MAIN_GROUP, "Guide rate", INDIGO_OK_STATE, INDIGO_RW_PERM, INDIGO_ONE_OF_MANY_RULE, 2);
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
