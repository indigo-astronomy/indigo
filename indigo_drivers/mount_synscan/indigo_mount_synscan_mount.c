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
 \file indigo_mount_synscan_mount.c
 */

#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <indigo/indigo_novas.h>
#include "indigo_mount_synscan_mount.h"
#include "indigo_mount_synscan_private.h"


#define h2d(h) (h * 15.0)
#define d2h(d) (d / 15.0)
static const double raRates[] = { 1.25, 2, 8, 16, 32, 70, 100, 625, 725, 825 };
static const double decRates[] = { 0.5, 1, 8, 16, 32, 70, 100, 625, 725, 825 };


#define REFRESH_SECONDS (0.5)

static int mount_manual_slew_rate(indigo_device* device) {
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

double synscan_tracking_rate(indigo_device* device) {
	if (MOUNT_TRACK_RATE_SIDEREAL_ITEM->sw.value)
		return SIDEREAL_RATE;
	else if (MOUNT_TRACK_RATE_SOLAR_ITEM->sw.value)
		return SOLAR_RATE;
	else if (MOUNT_TRACK_RATE_LUNAR_ITEM->sw.value)
		return LUNAR_RATE;
	else
		return 0.0;
}

static void
sla_de2h(double ha, double dec, double phi, double* az, double* el) {
	const double D2PI = 6.283185307179586476925286766559;

	//*  Useful trig functions
	double sh = sin(ha);
	double ch = cos(ha);
	double sd = sin(dec);
	double cd = cos(dec);
	double sp = sin(phi);
	double cp = cos(phi);

	//*  Az,El as x,y,z
	double x = -ch*cd*sp + sd*cp;
	double y = -sh*cd;
	double z = ch*cd*cp + sd*sp;

	//*  To spherical
	double r = hypot(x, y);
	double a;
	if (r == 0.0)
		a = 0.0;
	else
		a = atan2(y, x);
	if (fabs(a) < 0.00001 /*DBL_EPSILON*/)
		a = 0.0;
	if (a < 0.0) a += D2PI;
	*az = a;
	*el = atan2(z, r);
}

static void position_timer_callback(indigo_device *device) {
	PRIVATE_DATA->timer_count++;
	if (PRIVATE_DATA->handle > 0) {
		if (MOUNT_AUTOHOME_PROPERTY->state != INDIGO_BUSY_STATE) {
			//  Longitude needed for LST
			double lng = MOUNT_GEOGRAPHIC_COORDINATES_LONGITUDE_ITEM->number.value;
			double lat = MOUNT_GEOGRAPHIC_COORDINATES_LATITUDE_ITEM->number.value * M_PI / 180.0;

			//  Get the raw coords
			synscan_get_coords(device);

			//  Get the LST as quickly as we can after the HA
			double lst = indigo_lst(NULL, lng);

			//  Convert Encoder position to Raw HA/DEC/SideOfPier
			double raw_ha, raw_dec;
			int raw_sop;
			coords_encoder_to_eq(device, PRIVATE_DATA->raPosition, PRIVATE_DATA->decPosition, &raw_ha, &raw_dec, &raw_sop);
			//INDIGO_DRIVER_DEBUG(DRIVER_NAME, "RAW ENCODER:    HA %g, DEC: %g", PRIVATE_DATA->raPosition, PRIVATE_DATA->decPosition);

			//  Add in LST to get RA
			MOUNT_RAW_COORDINATES_RA_ITEM->number.value = (lst - (raw_ha * 12.0 / M_PI));
			MOUNT_RAW_COORDINATES_DEC_ITEM->number.value = raw_dec * 180.0 / M_PI;
			if (MOUNT_RAW_COORDINATES_RA_ITEM->number.value < 0)
				MOUNT_RAW_COORDINATES_RA_ITEM->number.value += 24.0;
			if (MOUNT_RAW_COORDINATES_RA_ITEM->number.value >= 24.0)
				MOUNT_RAW_COORDINATES_RA_ITEM->number.value -= 24.0;
			if (raw_sop == MOUNT_SIDE_EAST)
				indigo_set_switch(MOUNT_SIDE_OF_PIER_PROPERTY, MOUNT_SIDE_OF_PIER_EAST_ITEM, true);
			else
				indigo_set_switch(MOUNT_SIDE_OF_PIER_PROPERTY, MOUNT_SIDE_OF_PIER_WEST_ITEM, true);

			//INDIGO_DRIVER_DEBUG(DRIVER_NAME, "LST: %g, HA: %g, RA: %g, DEC: %g", lst, ha, (lst-(ha*12.0 / M_PI)), dec);
			indigo_update_property(device, MOUNT_RAW_COORDINATES_PROPERTY, NULL);
			indigo_update_property(device, MOUNT_SIDE_OF_PIER_PROPERTY, NULL);

			indigo_raw_to_translated_with_lst(device, lst, MOUNT_RAW_COORDINATES_RA_ITEM->number.value, MOUNT_RAW_COORDINATES_DEC_ITEM->number.value, raw_sop, &MOUNT_EQUATORIAL_COORDINATES_RA_ITEM->number.value, &MOUNT_EQUATORIAL_COORDINATES_DEC_ITEM->number.value);

			//  In here we want to transform from observed to mean coordinates

			indigo_update_property(device, MOUNT_EQUATORIAL_COORDINATES_PROPERTY, NULL);

			//  Note that this code normally belongs in indigo_update_coordinates(), but that routine causes numerical
			//  instability in derived quantities due to not having a way to use a fixed LST value for derived values.
			if (!MOUNT_GEOGRAPHIC_COORDINATES_PROPERTY->hidden && !MOUNT_HORIZONTAL_COORDINATES_PROPERTY->hidden) {
				double az, alt;
				double aligned_ha = lst - MOUNT_EQUATORIAL_COORDINATES_RA_ITEM->number.value;
				double aligned_dec = MOUNT_EQUATORIAL_COORDINATES_DEC_ITEM->number.value * M_PI / 180.0;

	//			if (aligned_ha < 0.0)
	//				aligned_ha += 24.0;
	//			if (aligned_ha >= 24.0)
	//				aligned_ha -= 24.0;
				aligned_ha = aligned_ha * M_PI / 12.0;

				sla_de2h(aligned_ha, aligned_dec, lat, &az, &alt);

				MOUNT_HORIZONTAL_COORDINATES_ALT_ITEM->number.value = alt * 180.0 / M_PI;
				MOUNT_HORIZONTAL_COORDINATES_AZ_ITEM->number.value = az * 180.0 / M_PI;
				MOUNT_HORIZONTAL_COORDINATES_PROPERTY->state = MOUNT_EQUATORIAL_COORDINATES_PROPERTY->state;
				indigo_update_property(device, MOUNT_HORIZONTAL_COORDINATES_PROPERTY, NULL);
				MOUNT_LST_TIME_ITEM->number.value = lst;
				indigo_update_property(device, MOUNT_LST_TIME_PROPERTY, NULL);
			}
		}
		indigo_reschedule_timer(device, 0.5, &PRIVATE_DATA->position_timer);
	}
	PRIVATE_DATA->timer_count--;
}

static void synscan_connect_timer_callback(indigo_device* device) {
	//  Lock the driver
	pthread_mutex_lock(&PRIVATE_DATA->driver_mutex);
	bool result = true;
	//  Open and configure the mount
	if (PRIVATE_DATA->device_count == 0) {
		result = synscan_open(device);
		if (result) {
			result = synscan_configure(device);
			if (!result && !PRIVATE_DATA->udp) {
				synscan_close(device);
				if (strcmp(DEVICE_BAUDRATE_ITEM->text.value, "9600-8N1"))
					strcpy(DEVICE_BAUDRATE_ITEM->text.value, "9600-8N1");
				else
					strcpy(DEVICE_BAUDRATE_ITEM->text.value, "115200-8N1");
				indigo_update_property(device, DEVICE_BAUDRATE_PROPERTY, "Trying to change baudrate");
				result = synscan_open(device);
				if (result)
					result = synscan_configure(device);
			}
		}
	}
	if (result) {
		PRIVATE_DATA->device_count++;
		indigo_set_switch(MOUNT_TRACKING_PROPERTY, MOUNT_TRACKING_OFF_ITEM, true);
		indigo_define_property(device, MOUNT_POLARSCOPE_PROPERTY, NULL);
		indigo_define_property(device, MOUNT_OPERATING_MODE_PROPERTY, NULL);
		indigo_define_property(device, MOUNT_USE_ENCODERS_PROPERTY, NULL);
		indigo_define_property(device, MOUNT_PEC_PROPERTY, NULL);
		indigo_define_property(device, MOUNT_PEC_TRAINING_PROPERTY, NULL);
		indigo_define_property(device, MOUNT_AUTOHOME_PROPERTY, NULL);
		indigo_define_property(device, MOUNT_AUTOHOME_SETTINGS_PROPERTY, NULL);
		CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
		indigo_mount_change_property(device, NULL, CONNECTION_PROPERTY);
		//  Start position timer
		indigo_set_timer(device, 1, position_timer_callback, &PRIVATE_DATA->position_timer);
	} else {
		synscan_close(device);
		CONNECTION_PROPERTY->state = INDIGO_ALERT_STATE;
		indigo_set_switch(CONNECTION_PROPERTY, CONNECTION_DISCONNECTED_ITEM, true);
		indigo_update_property(device, CONNECTION_PROPERTY, "Failed to connect to mount");
	}
	indigo_mount_change_property(device, NULL, CONNECTION_PROPERTY);
	//  Unlock the driver
	pthread_mutex_unlock(&PRIVATE_DATA->driver_mutex);
}

static void synscan_disconnect_timer_callback(indigo_device* device) {
	indigo_cancel_timer(device, &PRIVATE_DATA->position_timer);
	indigo_delete_property(device, MOUNT_POLARSCOPE_PROPERTY, NULL);
	indigo_delete_property(device, MOUNT_OPERATING_MODE_PROPERTY, NULL);
	indigo_delete_property(device, MOUNT_USE_ENCODERS_PROPERTY, NULL);
	indigo_delete_property(device, MOUNT_PEC_PROPERTY, NULL);
	indigo_delete_property(device, MOUNT_PEC_TRAINING_PROPERTY, NULL);
	indigo_delete_property(device, MOUNT_AUTOHOME_PROPERTY, NULL);
	indigo_delete_property(device, MOUNT_AUTOHOME_SETTINGS_PROPERTY, NULL);
	if (PRIVATE_DATA->device_count > 0 && --PRIVATE_DATA->device_count == 0) {
		synscan_close(device);
	}
	CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
	indigo_mount_change_property(device, NULL, CONNECTION_PROPERTY);
}

indigo_result synscan_mount_connect(indigo_device* device) {
	//  Handle connect/disconnect commands
	if (CONNECTION_CONNECTED_ITEM->sw.value) {
		//  CONNECT to the mount
		indigo_set_timer(device, 0, &synscan_connect_timer_callback, NULL);
	} else if (CONNECTION_DISCONNECTED_ITEM->sw.value) {
		indigo_set_timer(device, 0, &synscan_disconnect_timer_callback, NULL);
	}
	return INDIGO_OK;
}

static void mount_slew_timer_callback(indigo_device* device) {
	char* message = "Slew aborted.";

	pthread_mutex_lock(&PRIVATE_DATA->driver_mutex);

	//**  Stop both axes
	synscan_stop_axis(device, kAxisRA);
	synscan_stop_axis(device, kAxisDEC);
	synscan_wait_for_axis_stopped(device, kAxisRA, &PRIVATE_DATA->abort_motion);
	PRIVATE_DATA->raAxisMode = kAxisModeIdle;
	synscan_wait_for_axis_stopped(device, kAxisDEC, &PRIVATE_DATA->abort_motion);
	PRIVATE_DATA->decAxisMode = kAxisModeIdle;

	//  The coordinates that we have been given SHOULD be expressed in the epoch specified in MOUNT_EPOCH
	//  If this is not JNOW, then we need to transform them to JNOW and then compute the observed place coordinates

	//**  Perform preliminary slew on both axes
	//  Compute initial target positions
	double ra, dec;
	indigo_translated_to_raw(device, MOUNT_EQUATORIAL_COORDINATES_RA_ITEM->number.target, MOUNT_EQUATORIAL_COORDINATES_DEC_ITEM->number.target, &ra, &dec);
	ra = ra * M_PI / 12.0;
	dec = dec * M_PI / 180.0;
	double lng = MOUNT_GEOGRAPHIC_COORDINATES_LONGITUDE_ITEM->number.value;
	double lst = indigo_lst(NULL, lng) * M_PI / 12.0;
	double ha = lst - ra;
	double haPos[2], decPos[2];
	coords_eq_to_encoder2(device, ha, dec, haPos, decPos);

	//  Select best encoder point based on limits
	int idx = synscan_select_best_encoder_point(device, haPos, decPos);

	//  Abort slew if requested
	if (PRIVATE_DATA->abort_motion)
		goto finish;

	//  Start the first slew
	INDIGO_DRIVER_DEBUG(DRIVER_NAME, "1ST SLEW TO:  %g / %g     (HA %g / DEC %g)", haPos[idx], decPos[idx], ha, dec);
	synscan_slew_axis_to_position(device, kAxisRA, haPos[idx]);
	synscan_slew_axis_to_position(device, kAxisDEC, decPos[idx]);
	//indigo_update_property(device, MOUNT_EQUATORIAL_COORDINATES_PROPERTY, "Slewing...");

	//**  Wait for HA slew to complete
	synscan_wait_for_axis_stopped(device, kAxisRA, &PRIVATE_DATA->abort_motion);
	PRIVATE_DATA->raAxisMode = kAxisModeIdle;

	//**  Compute precise HA slew for LST + 5 seconds
	indigo_translated_to_raw(device, MOUNT_EQUATORIAL_COORDINATES_RA_ITEM->number.target, MOUNT_EQUATORIAL_COORDINATES_DEC_ITEM->number.target, &ra, &dec);
	ra = ra * M_PI / 12.0;
	dec = dec * M_PI / 180.0;
	double target_lst = indigo_lst(NULL, lng) + (5 / 3600.0);
	if (target_lst >= 24.0) {
		target_lst -= 24.0;
	}
	ha = (target_lst * M_PI / 12.0) - ra;
	coords_eq_to_encoder2(device, ha, dec, haPos, decPos);

	//  Abort slew if requested
	if (PRIVATE_DATA->abort_motion)
		goto finish;

	//  We use the same index as the preliminary slew to avoid issues with target crossing meridian during first slew
	INDIGO_DRIVER_DEBUG(DRIVER_NAME, "2ND SLEW TO:  %g / %g     (HA %g / DEC %g)", haPos[idx], decPos[idx], ha, dec);

	//  Start new HA slew
	synscan_slew_axis_to_position(device, kAxisRA, haPos[idx]);
	indigo_update_property(device, MOUNT_EQUATORIAL_COORDINATES_PROPERTY, "Performing accurate HA slew...");

	//**  Wait for precise HA slew to complete
	synscan_wait_for_axis_stopped(device, kAxisRA, &PRIVATE_DATA->abort_motion);
	PRIVATE_DATA->raAxisMode = kAxisModeIdle;

	//  Abort slew if requested
	if (PRIVATE_DATA->abort_motion)
		goto finish;

	//**  Wait for LST to match target LST
	indigo_update_property(device, MOUNT_EQUATORIAL_COORDINATES_PROPERTY, "HA slew complete.");
	while (!PRIVATE_DATA->abort_motion) {
		//  Get current LST
		lst = indigo_lst(NULL, lng);

		//  Wait for LST to reach target LST
		//  Ensure difference is not too large to account for LST wrap
		if (lst >= target_lst && (lst - target_lst <= 5/3600.0)) {
			//    Start tracking if we need to
			if (MOUNT_ON_COORDINATES_SET_TRACK_ITEM->sw.value) {
				synscan_slew_axis_at_rate(device, kAxisRA, synscan_tracking_rate(device));
				PRIVATE_DATA->raAxisMode = kAxisModeTracking;

				//  Update the tracking property to be ON if not already so it is reflected in UI
				indigo_set_switch(MOUNT_TRACKING_PROPERTY, MOUNT_TRACKING_ON_ITEM, true);
				MOUNT_TRACKING_PROPERTY->state = INDIGO_OK_STATE;
				indigo_update_property(device, MOUNT_TRACKING_PROPERTY, "Tracking started.");
			}
			break;
		}

		//  Brief delay
		indigo_usleep(100000);
	}

	//**  Wait for DEC slew to complete
	synscan_wait_for_axis_stopped(device, kAxisDEC, &PRIVATE_DATA->abort_motion);
	PRIVATE_DATA->decAxisMode = kAxisModeIdle;
	indigo_update_property(device, MOUNT_EQUATORIAL_COORDINATES_PROPERTY, "DEC slew complete.");

	//  Abort slew if requested
	if (PRIVATE_DATA->abort_motion)
		goto finish;

	//**  Do precise DEC slew correction
	//  Start new DEC slew
	synscan_slew_axis_to_position(device, kAxisDEC, decPos[idx]);
	indigo_update_property(device, MOUNT_EQUATORIAL_COORDINATES_PROPERTY, "Performing accurate DEC slew...");

	//**  Wait for DEC slew to complete
	synscan_wait_for_axis_stopped(device, kAxisDEC, &PRIVATE_DATA->abort_motion);
	PRIVATE_DATA->decAxisMode = kAxisModeIdle;

	//  Abort slew if requested
	if (PRIVATE_DATA->abort_motion)
		goto finish;

	indigo_update_property(device, MOUNT_EQUATORIAL_COORDINATES_PROPERTY, "DEC slew complete.");
	message = "Slew complete.";

finish:
	//**  Finalise slew coordinates
	PRIVATE_DATA->abort_motion = false;
	MOUNT_EQUATORIAL_COORDINATES_PROPERTY->state = INDIGO_OK_STATE;
	indigo_update_property(device, MOUNT_EQUATORIAL_COORDINATES_PROPERTY, message);
	PRIVATE_DATA->globalMode = kGlobalModeIdle;
	pthread_mutex_unlock(&PRIVATE_DATA->driver_mutex);
}

#if 0
static void mount_slew_aa_timer_callback(indigo_device* device) {
	char* message = "Slew aborted.";

	pthread_mutex_lock(&PRIVATE_DATA->driver_mutex);

	//**  Stop both axes
	synscan_stop_axis(device, kAxisRA);
	synscan_stop_axis(device, kAxisDEC);
	synscan_wait_for_axis_stopped(device, kAxisRA, &PRIVATE_DATA->abort_motion);
	PRIVATE_DATA->raAxisMode = kAxisModeIdle;
	synscan_wait_for_axis_stopped(device, kAxisDEC, &PRIVATE_DATA->abort_motion);
	PRIVATE_DATA->decAxisMode = kAxisModeIdle;

	//**  Perform preliminary slew on both axes
	//  Compute initial target positions
	double az, alt;
	//indigo_translated_to_raw(device, MOUNT_EQUATORIAL_COORDINATES_RA_ITEM->number.target, MOUNT_EQUATORIAL_COORDINATES_DEC_ITEM->number.target, &ra, &dec);
	az = az * M_PI / 12.0;
	alt = alt * M_PI / 180.0;
	//double lng = MOUNT_GEOGRAPHIC_COORDINATES_LONGITUDE_ITEM->number.value;
	//double lst = indigo_lst(NULL, lng) * M_PI / 12.0;
	//double ha = lst - ra;
	double azPos[2], altPos[2];
	coords_aa_to_encoder2(device, az, alt, azPos, altPos);

	//  Select best encoder point based on limits
	int idx = synscan_select_best_encoder_point_aa(device, azPos, altPos);

	//  Abort slew if requested
	if (PRIVATE_DATA->abort_motion)
		goto finish;

	//  Start the first slew
	INDIGO_DRIVER_DEBUG(DRIVER_NAME, "1ST SLEW TO:  %g / %g     (AZ %g / ALT %g)", azPos[idx], altPos[idx], az, alt);
	synscan_slew_axis_to_position(device, kAxisRA, azPos[idx]);
	synscan_slew_axis_to_position(device, kAxisDEC, altPos[idx]);

	//**  Wait for HA slew to complete
	synscan_wait_for_axis_stopped(device, kAxisRA, &PRIVATE_DATA->abort_motion);
	PRIVATE_DATA->raAxisMode = kAxisModeIdle;

	//**  Compute precise HA slew for LST + 5 seconds
//	indigo_translated_to_raw(device, MOUNT_EQUATORIAL_COORDINATES_RA_ITEM->number.target, MOUNT_EQUATORIAL_COORDINATES_DEC_ITEM->number.target, &ra, &dec);
//	ra = ra * M_PI / 12.0;
//	dec = dec * M_PI / 180.0;
//	double target_lst = indigo_lst(NULL, lng) + (5 / 3600.0);
//	bool lst_wrap = false;
//	if (target_lst >= 24.0) {
//		target_lst -= 24.0;
//		lst_wrap = true;
//	}
//	ha = (target_lst * M_PI / 12.0) - ra;
//	coords_eq_to_encoder2(device, ha, dec, haPos, decPos);

	//  Abort slew if requested
	if (PRIVATE_DATA->abort_motion)
		goto finish;

	//  We use the same index as the preliminary slew to avoid issues with target crossing meridian during first slew
	INDIGO_DRIVER_DEBUG(DRIVER_NAME, "2ND SLEW TO:  %g / %g     (HA %g / DEC %g)", azPos[idx], altPos[idx], az, alt);

	//  Start new HA slew
	synscan_slew_axis_to_position(device, kAxisRA, azPos[idx]);
	indigo_update_property(device, MOUNT_HORIZONTAL_COORDINATES_PROPERTY, "Performing accurate AZ slew...");

	//**  Wait for precise HA slew to complete
	synscan_wait_for_axis_stopped(device, kAxisRA, &PRIVATE_DATA->abort_motion);
	PRIVATE_DATA->raAxisMode = kAxisModeIdle;

	//  Abort slew if requested
	if (PRIVATE_DATA->abort_motion)
		goto finish;

	//**  Wait for LST to match target LST
	indigo_update_property(device, MOUNT_HORIZONTAL_COORDINATES_PROPERTY, "AZ slew complete.");
//	while (!PRIVATE_DATA->abort_motion) {
//		//  Get current LST
//		lst = indigo_lst(NULL, lng);
//
//		//  Wait for LST to reach target LST
//		//  Ensure difference is not too large to account for LST wrap
//		if (lst >= target_lst && (lst - target_lst <= 5/3600.0)) {
//			//    Start tracking if we need to
//			if (MOUNT_ON_COORDINATES_SET_TRACK_ITEM->sw.value) {
//				synscan_slew_axis_at_rate(device, kAxisRA, synscan_tracking_rate(device));
//				PRIVATE_DATA->raAxisMode = kAxisModeTracking;
//
//				//  Update the tracking property to be ON if not already so it is reflected in UI
//				indigo_set_switch(MOUNT_TRACKING_PROPERTY, MOUNT_TRACKING_ON_ITEM, true);
//				MOUNT_TRACKING_PROPERTY->state = INDIGO_OK_STATE;
//				indigo_update_property(device, MOUNT_TRACKING_PROPERTY, "Tracking started.");
//			}
//			break;
//		}
//
//		//  Brief delay
//		indigo_usleep(100000);
//	}

	//**  Wait for DEC slew to complete
	synscan_wait_for_axis_stopped(device, kAxisDEC, &PRIVATE_DATA->abort_motion);
	PRIVATE_DATA->decAxisMode = kAxisModeIdle;
	indigo_update_property(device, MOUNT_HORIZONTAL_COORDINATES_PROPERTY, "ALT slew complete.");

	//  Abort slew if requested
	if (PRIVATE_DATA->abort_motion)
		goto finish;

	//**  Do precise DEC slew correction
	//  Start new DEC slew
	synscan_slew_axis_to_position(device, kAxisDEC, altPos[idx]);
	indigo_update_property(device, MOUNT_HORIZONTAL_COORDINATES_PROPERTY, "Performing accurate ALT slew...");

	//**  Wait for DEC slew to complete
	synscan_wait_for_axis_stopped(device, kAxisDEC, &PRIVATE_DATA->abort_motion);
	PRIVATE_DATA->decAxisMode = kAxisModeIdle;

	//  Abort slew if requested
	if (PRIVATE_DATA->abort_motion)
		goto finish;

	indigo_update_property(device, MOUNT_HORIZONTAL_COORDINATES_PROPERTY, "ALT slew complete.");
	message = "Slew complete.";

finish:
	//**  Finalise slew coordinates
	PRIVATE_DATA->abort_motion = false;
	MOUNT_HORIZONTAL_COORDINATES_PROPERTY->state = INDIGO_OK_STATE;
	indigo_update_property(device, MOUNT_HORIZONTAL_COORDINATES_PROPERTY, message);
	PRIVATE_DATA->globalMode = kGlobalModeIdle;
	pthread_mutex_unlock(&PRIVATE_DATA->driver_mutex);
}
#endif

void mount_handle_coordinates(indigo_device *device) {
	if (MOUNT_ON_COORDINATES_SET_SLEW_ITEM->sw.value || MOUNT_ON_COORDINATES_SET_TRACK_ITEM->sw.value) {
		MOUNT_EQUATORIAL_COORDINATES_PROPERTY->state = INDIGO_BUSY_STATE;
		indigo_update_coordinates(device, "Slewing...");

		//  Start slew timer thread
		PRIVATE_DATA->globalMode = kGlobalModeSlewing;
		indigo_set_timer(device, 0, mount_slew_timer_callback, NULL);
	}
}

void mount_handle_aa_coordinates(indigo_device *device) {
//	if (MOUNT_ON_COORDINATES_SET_SLEW_ITEM->sw.value || MOUNT_ON_COORDINATES_SET_TRACK_ITEM->sw.value) {
//		MOUNT_HORIZONTAL_COORDINATES_PROPERTY->state = INDIGO_BUSY_STATE;
//		indigo_update_coordinates(device, "Slewing...");
//
//		//  Start slew timer thread
//		PRIVATE_DATA->globalMode = kGlobalModeSlewing;
//		indigo_set_timer(device, 0, mount_slew_aa_timer_callback, NULL);
//	}
}

static void mount_update_tracking_rate_timer_callback(indigo_device* device) {
	//  Update the tracking rate
	pthread_mutex_lock(&PRIVATE_DATA->driver_mutex);
	double axisRate = synscan_tracking_rate(device);

	//  Try a simple rate update on the axis
	//  If a simple rate adjustment isn't possible, stop the axis first
	bool result;
	synscan_update_axis_to_rate(device, kAxisRA, axisRate, &result);
	if (!result) {
		synscan_stop_axis(device, kAxisRA);
		synscan_wait_for_axis_stopped(device, kAxisRA, NULL);
		synscan_slew_axis_at_rate(device, kAxisRA, axisRate);
	}
	PRIVATE_DATA->raAxisMode = kAxisModeTracking;

	//  Finished updating
	MOUNT_TRACK_RATE_PROPERTY->state = INDIGO_OK_STATE;
	indigo_update_property(device, MOUNT_TRACK_RATE_PROPERTY, NULL);
	pthread_mutex_unlock(&PRIVATE_DATA->driver_mutex);
}

void mount_handle_tracking_rate(indigo_device* device) {
	//  Update the tracking rate of the axis if we are tracking
	if (MOUNT_TRACKING_ON_ITEM->sw.value) {
		MOUNT_TRACK_RATE_PROPERTY->state = INDIGO_BUSY_STATE;
		indigo_update_property(device, MOUNT_TRACK_RATE_PROPERTY, NULL);
		indigo_set_timer(device, 0, mount_update_tracking_rate_timer_callback, NULL);
	}
	else {
		MOUNT_TRACK_RATE_PROPERTY->state = INDIGO_OK_STATE;
		indigo_update_property(device, MOUNT_TRACK_RATE_PROPERTY, NULL);
	}
}

static void mount_tracking_timer_callback(indigo_device* device) {
	pthread_mutex_lock(&PRIVATE_DATA->driver_mutex);
	MOUNT_TRACKING_PROPERTY->state = INDIGO_OK_STATE;
	if (MOUNT_TRACKING_ON_ITEM->sw.value) {
		//  Start tracking at the configured rate
		double axisRate = synscan_tracking_rate(device);
		synscan_slew_axis_at_rate(device, kAxisRA, axisRate);
		PRIVATE_DATA->raAxisMode = kAxisModeTracking;
		indigo_update_property(device, MOUNT_TRACKING_PROPERTY, "Tracking started");
	} else if (MOUNT_TRACKING_OFF_ITEM->sw.value) {
		synscan_stop_axis(device, kAxisRA);
		synscan_wait_for_axis_stopped(device, kAxisRA, NULL);
		PRIVATE_DATA->raAxisMode = kAxisModeIdle;
		indigo_send_message(device, "Tracking stopped");
		indigo_update_property(device, MOUNT_TRACKING_PROPERTY, NULL);
	}
	pthread_mutex_unlock(&PRIVATE_DATA->driver_mutex);
}

void mount_handle_tracking(indigo_device *device) {
	MOUNT_TRACKING_PROPERTY->state = INDIGO_BUSY_STATE;
	indigo_update_property(device, MOUNT_TRACKING_PROPERTY, NULL);
	indigo_set_timer(device, 0, mount_tracking_timer_callback, NULL);
}

static void manual_slew_west_timer_callback(indigo_device* device) {
	pthread_mutex_lock(&PRIVATE_DATA->driver_mutex);
	double axisRate = raRates[mount_manual_slew_rate(device)] * SIDEREAL_RATE;
	synscan_slew_axis_at_rate(device, kAxisRA, axisRate);
	PRIVATE_DATA->raAxisMode = kAxisModeManualSlewing;
	pthread_mutex_unlock(&PRIVATE_DATA->driver_mutex);
}

static void manual_slew_east_timer_callback(indigo_device* device) {
	pthread_mutex_lock(&PRIVATE_DATA->driver_mutex);
	double axisRate = raRates[mount_manual_slew_rate(device)] * SIDEREAL_RATE;
	synscan_slew_axis_at_rate(device, kAxisRA, -axisRate);
	PRIVATE_DATA->raAxisMode = kAxisModeManualSlewing;
	pthread_mutex_unlock(&PRIVATE_DATA->driver_mutex);
}

static void manual_slew_north_timer_callback(indigo_device* device) {
	pthread_mutex_lock(&PRIVATE_DATA->driver_mutex);
	double axisRate = decRates[mount_manual_slew_rate(device)] * SIDEREAL_RATE;
	synscan_slew_axis_at_rate(device, kAxisDEC, -axisRate);
	PRIVATE_DATA->decAxisMode = kAxisModeManualSlewing;
	pthread_mutex_unlock(&PRIVATE_DATA->driver_mutex);
}

static void manual_slew_south_timer_callback(indigo_device* device) {
	pthread_mutex_lock(&PRIVATE_DATA->driver_mutex);
	double axisRate = decRates[mount_manual_slew_rate(device)] * SIDEREAL_RATE;
	synscan_slew_axis_at_rate(device, kAxisDEC, axisRate);
	PRIVATE_DATA->decAxisMode = kAxisModeManualSlewing;
	pthread_mutex_unlock(&PRIVATE_DATA->driver_mutex);
}

static void manual_slew_ra_stop_timer_callback(indigo_device* device) {
	pthread_mutex_lock(&PRIVATE_DATA->driver_mutex);
	synscan_stop_axis(device, kAxisRA);
	synscan_wait_for_axis_stopped(device, kAxisRA, NULL);
	PRIVATE_DATA->raAxisMode = kAxisModeIdle;

	//  Resume tracking if tracking is currently on
	if (MOUNT_TRACKING_ON_ITEM->sw.value) {
		double axisRate = synscan_tracking_rate(device);
		synscan_slew_axis_at_rate(device, kAxisRA, axisRate);
		PRIVATE_DATA->raAxisMode = kAxisModeTracking;
	}

	//  Update property to OK
	MOUNT_MOTION_RA_PROPERTY->state = INDIGO_OK_STATE;
	indigo_update_property(device, MOUNT_MOTION_RA_PROPERTY, NULL);
	pthread_mutex_unlock(&PRIVATE_DATA->driver_mutex);
}

static void manual_slew_dec_stop_timer_callback(indigo_device* device) {
	pthread_mutex_lock(&PRIVATE_DATA->driver_mutex);
	synscan_stop_axis(device, kAxisDEC);
	synscan_wait_for_axis_stopped(device, kAxisDEC, NULL);
	PRIVATE_DATA->decAxisMode = kAxisModeIdle;

	//  Update property to OK
	MOUNT_MOTION_DEC_PROPERTY->state = INDIGO_OK_STATE;
	indigo_update_property(device, MOUNT_MOTION_DEC_PROPERTY, NULL);
	pthread_mutex_unlock(&PRIVATE_DATA->driver_mutex);
}

void mount_handle_motion_ra(indigo_device *device) {
	if (MOUNT_MOTION_WEST_ITEM->sw.value) {
		MOUNT_MOTION_RA_PROPERTY->state = INDIGO_BUSY_STATE;
		indigo_update_property(device, MOUNT_MOTION_RA_PROPERTY, NULL);
		indigo_set_timer(device, 0, manual_slew_west_timer_callback, NULL);
	} else if (MOUNT_MOTION_EAST_ITEM->sw.value) {
		MOUNT_MOTION_RA_PROPERTY->state = INDIGO_BUSY_STATE;
		indigo_update_property(device, MOUNT_MOTION_RA_PROPERTY, NULL);
		indigo_set_timer(device, 0, manual_slew_east_timer_callback, NULL);
	} else {
		indigo_set_timer(device, 0, manual_slew_ra_stop_timer_callback, NULL);
	}
}

void mount_handle_motion_dec(indigo_device *device) {
	if(MOUNT_MOTION_NORTH_ITEM->sw.value) {
		MOUNT_MOTION_DEC_PROPERTY->state = INDIGO_BUSY_STATE;
		indigo_update_property(device, MOUNT_MOTION_DEC_PROPERTY, NULL);
		indigo_set_timer(device, 0, manual_slew_north_timer_callback, NULL);
	} else if (MOUNT_MOTION_SOUTH_ITEM->sw.value) {
		MOUNT_MOTION_DEC_PROPERTY->state = INDIGO_BUSY_STATE;
		indigo_update_property(device, MOUNT_MOTION_DEC_PROPERTY, NULL);
		indigo_set_timer(device, 0, manual_slew_south_timer_callback, NULL);
	} else {
		indigo_set_timer(device, 0, manual_slew_dec_stop_timer_callback, NULL);
	}
}

static void mount_park_timer_callback(indigo_device* device) {
	pthread_mutex_lock(&PRIVATE_DATA->driver_mutex);

	//  Stop both axes
	synscan_stop_axis(device, kAxisRA);
	synscan_stop_axis(device, kAxisDEC);
	synscan_wait_for_axis_stopped(device, kAxisRA, &PRIVATE_DATA->abort_motion);
	PRIVATE_DATA->raAxisMode = kAxisModeIdle;
	synscan_wait_for_axis_stopped(device, kAxisDEC, &PRIVATE_DATA->abort_motion);
	PRIVATE_DATA->raAxisMode = kAxisModeIdle;

	//  Stop tracking if enabled
	indigo_set_switch(MOUNT_TRACKING_PROPERTY, MOUNT_TRACKING_OFF_ITEM, true);
	MOUNT_TRACKING_PROPERTY->state = INDIGO_OK_STATE;
	indigo_update_property(device, MOUNT_TRACKING_PROPERTY, "Tracking stopped.");

	//  Compute the axis positions for parking
	double ha = (PRIVATE_DATA->globalMode == kGlobalModeGoingHome ? MOUNT_HOME_POSITION_HA_ITEM->number.value : MOUNT_PARK_POSITION_HA_ITEM->number.value) * M_PI / 12.0;
	double dec = (PRIVATE_DATA->globalMode == kGlobalModeGoingHome ? MOUNT_HOME_POSITION_DEC_ITEM->number.value : MOUNT_PARK_POSITION_DEC_ITEM->number.value) * M_PI / 180.0;
	double haPos[2], decPos[2];
	coords_eq_to_encoder2(device, ha, dec, haPos, decPos);
	int idx = synscan_select_best_encoder_point(device, haPos, decPos);

	//  Abort parking if requested
	if (PRIVATE_DATA->abort_motion) {
		PRIVATE_DATA->abort_motion = false;
		pthread_mutex_unlock(&PRIVATE_DATA->driver_mutex);
		return;
	}

	//  Start the axes moving
	synscan_slew_axis_to_position(device, kAxisRA, haPos[idx]);
	synscan_slew_axis_to_position(device, kAxisDEC, decPos[idx]);

	//  Check for HA parked
	synscan_wait_for_axis_stopped(device, kAxisRA, &PRIVATE_DATA->abort_motion);
	PRIVATE_DATA->raAxisMode = kAxisModeIdle;

	//  Check for DEC parked
	synscan_wait_for_axis_stopped(device, kAxisDEC, &PRIVATE_DATA->abort_motion);
	PRIVATE_DATA->decAxisMode = kAxisModeIdle;

	//  Abort parking if requested
	if (PRIVATE_DATA->abort_motion) {
		PRIVATE_DATA->abort_motion = false;
		pthread_mutex_unlock(&PRIVATE_DATA->driver_mutex);
		return;
	}

	//  Update state
	if (PRIVATE_DATA->globalMode == kGlobalModeGoingHome) {
		MOUNT_HOME_PROPERTY->state = INDIGO_OK_STATE;
		indigo_update_property(device, MOUNT_HOME_PROPERTY, "Mount at home.");
	} else {
		synscan_save_position(device);
		MOUNT_PARK_PARKED_ITEM->sw.value = true;
		MOUNT_PARK_PROPERTY->state = INDIGO_OK_STATE;
		indigo_update_property(device, MOUNT_PARK_PROPERTY, "Mount parked.");
	}
	PRIVATE_DATA->globalMode = kGlobalModeIdle;
	pthread_mutex_unlock(&PRIVATE_DATA->driver_mutex);
}

void mount_handle_park(indigo_device* device) {
	if (MOUNT_PARK_PARKED_ITEM->sw.value) {
		if (PRIVATE_DATA->globalMode == kGlobalModeIdle) {
			MOUNT_PARK_PROPERTY->state = INDIGO_BUSY_STATE;
			indigo_update_property(device, MOUNT_PARK_PROPERTY, "Parking...");

			PRIVATE_DATA->globalMode = kGlobalModeParking;
			indigo_set_timer(device, 0, mount_park_timer_callback, NULL);
		} else {
			//  Can't park while mount is doing something else
			MOUNT_PARK_PROPERTY->state = INDIGO_ALERT_STATE;
			indigo_update_property(device, MOUNT_PARK_PROPERTY, "Parking not started - mount is busy.");
		}
	}
	else {
		MOUNT_PARK_PROPERTY->state = INDIGO_OK_STATE;
		indigo_update_property(device, MOUNT_PARK_PROPERTY, "Mount unparked.");
	}
}

void mount_handle_home(indigo_device* device) {
	if (MOUNT_HOME_ITEM->sw.value) {
		MOUNT_HOME_ITEM->sw.value = false;
		if (PRIVATE_DATA->globalMode == kGlobalModeIdle) {
			MOUNT_HOME_PROPERTY->state = INDIGO_BUSY_STATE;
			indigo_update_property(device, MOUNT_HOME_PROPERTY, "Going home...");
			PRIVATE_DATA->globalMode = kGlobalModeGoingHome;
			indigo_set_timer(device, 0, mount_park_timer_callback, NULL);
		} else {
			//  Can't go home while mount is doing something else
			MOUNT_PARK_PROPERTY->state = INDIGO_ALERT_STATE;
			indigo_update_property(device, MOUNT_PARK_PROPERTY, "Going home not started - mount is busy.");
		}
	}
}

static enum GuideRate mount_guide_rate(int rate) {
	if (rate < 19)
		return kGuideRate_x0_125;
	else if (rate < 38)
		return kGuideRate_x0_25;
	else if (rate < 63)
		return kGuideRate_x0_50;
	else if (rate < 88)
		return kGuideRate_x0_75;
	else
		return kGuideRate_x1_00;
}

static int mount_guide_rate_percent(enum GuideRate rate) {
	switch (rate) {
		case kGuideRate_x0_125:
			return 12;
		case kGuideRate_x0_25:
			return 25;
		case kGuideRate_x0_50:
			return 50;
		case kGuideRate_x0_75:
			return 75;
		case kGuideRate_x1_00:
			return 100;
	}
}

void mount_handle_st4_guiding_rate(indigo_device *device) {
	//  Convert RA/DEC rate to supportable rate
	int ra_rate = mount_guide_rate(MOUNT_GUIDE_RATE_RA_ITEM->number.value);
	int dec_rate = mount_guide_rate(MOUNT_GUIDE_RATE_DEC_ITEM->number.value);

	//  Update the rates
	synscan_set_st4_guide_rate(device, kAxisRA, ra_rate);
	synscan_set_st4_guide_rate(device, kAxisDEC, dec_rate);

	//  Update rate properties to reflect what happened
	MOUNT_GUIDE_RATE_RA_ITEM->number.value = mount_guide_rate_percent(ra_rate);
	MOUNT_GUIDE_RATE_DEC_ITEM->number.value = mount_guide_rate_percent(dec_rate);
	MOUNT_GUIDE_RATE_PROPERTY->state = INDIGO_OK_STATE;
	indigo_update_property(device, MOUNT_GUIDE_RATE_PROPERTY, "Updated ST4 guide rate.");
}

void mount_handle_abort(indigo_device *device) {
	if (MOUNT_ABORT_MOTION_ITEM->sw.value) {
		if (PRIVATE_DATA->globalMode != kGlobalModeIdle) {
			//  Cancel any park or slew in progress
			PRIVATE_DATA->abort_motion = true;
		}

		//  Unconditionally stop motors in case mount is out of control for any reason
		synscan_stop_axis(device, kAxisRA);
		synscan_stop_axis(device, kAxisDEC);
		synscan_wait_for_axis_stopped(device, kAxisRA, NULL);
		synscan_wait_for_axis_stopped(device, kAxisDEC, NULL);
		PRIVATE_DATA->raAxisMode = kAxisModeIdle;
		PRIVATE_DATA->decAxisMode = kAxisModeIdle;

		//  Cancel any HA manual slew
		MOUNT_MOTION_WEST_ITEM->sw.value = false;
		MOUNT_MOTION_EAST_ITEM->sw.value = false;
		MOUNT_MOTION_RA_PROPERTY->state = INDIGO_OK_STATE;
		indigo_update_property(device, MOUNT_MOTION_RA_PROPERTY, NULL);

		//  Cancel any DEC manual slew
		MOUNT_MOTION_NORTH_ITEM->sw.value = false;
		MOUNT_MOTION_SOUTH_ITEM->sw.value = false;
		MOUNT_MOTION_DEC_PROPERTY->state = INDIGO_OK_STATE;
		indigo_update_property(device, MOUNT_MOTION_DEC_PROPERTY, NULL);

		//  Update coordinates in case we aborted a slew
		MOUNT_EQUATORIAL_COORDINATES_RA_ITEM->number.target = MOUNT_EQUATORIAL_COORDINATES_RA_ITEM->number.value;
		MOUNT_EQUATORIAL_COORDINATES_DEC_ITEM->number.target = MOUNT_EQUATORIAL_COORDINATES_DEC_ITEM->number.value;
		MOUNT_EQUATORIAL_COORDINATES_PROPERTY->state = INDIGO_OK_STATE;
		indigo_update_coordinates(device, NULL);

		//  Disable tracking
		indigo_set_switch(MOUNT_TRACKING_PROPERTY, MOUNT_TRACKING_OFF_ITEM, true);
		MOUNT_TRACKING_PROPERTY->state = INDIGO_OK_STATE;
		indigo_update_property(device, MOUNT_TRACKING_PROPERTY, NULL);

		//  Reset the abort switch
		MOUNT_ABORT_MOTION_ITEM->sw.value = false;
		MOUNT_ABORT_MOTION_PROPERTY->state = INDIGO_OK_STATE;
		indigo_update_property(device, MOUNT_ABORT_MOTION_PROPERTY, "Aborted.");

		//  Set global mode to idle
		PRIVATE_DATA->globalMode = kGlobalModeIdle;
	}
}

void mount_handle_polarscope(indigo_device *device) {
	unsigned char brightness = MOUNT_POLARSCOPE_BRIGHTNESS_ITEM->number.value;
	synscan_set_polarscope_brightness(device, brightness);

	MOUNT_POLARSCOPE_PROPERTY->state = INDIGO_OK_STATE;
	indigo_update_property(device, MOUNT_POLARSCOPE_PROPERTY, "Changed polarscope LED brightness.");
}

void mount_handle_encoders(indigo_device *device) {
	synscan_ext_setting(device, kAxisRA,  MOUNT_USE_RA_ENCODER_ITEM->sw.value ? kTurnEncoderOn : kTurnEncoderOff);
	synscan_ext_setting(device, kAxisDEC,  MOUNT_USE_DEC_ENCODER_ITEM->sw.value ? kTurnEncoderOn : kTurnEncoderOff);
	MOUNT_USE_ENCODERS_PROPERTY->state = INDIGO_OK_STATE;
	indigo_update_property(device, MOUNT_USE_ENCODERS_PROPERTY, "Updated encoders usage");
}

void mount_handle_use_ppec(indigo_device *device) {
	if (!synscan_ext_setting(device, kAxisRA,  MOUNT_PEC_ENABLED_ITEM->sw.value ? kTurnPECCOn : kTurnPECCOff)) {
		MOUNT_PEC_PROPERTY->state = INDIGO_ALERT_STATE;
		indigo_update_property(device, MOUNT_PEC_PROPERTY, "Failed to update PPEC state");
	} else {
		MOUNT_PEC_PROPERTY->state = INDIGO_OK_STATE;
		indigo_update_property(device, MOUNT_PEC_PROPERTY, "Updated PPEC state");
	}
}

static void mount_train_ppec_callback(indigo_device* device) {
	if (!synscan_ext_inquiry(device, kAxisRA, kGetFeatures, &PRIVATE_DATA->raFeatures)) {
		MOUNT_PEC_TRAINING_PROPERTY->state = INDIGO_ALERT_STATE;
		indigo_update_property(device, MOUNT_PEC_TRAINING_PROPERTY, "Failed to read PPEC training state");
	} else if ((PRIVATE_DATA->raFeatures & kInPPECTraining) || (PRIVATE_DATA->decFeatures & kInPPECTraining)) {
		indigo_set_timer(device, 1, mount_train_ppec_callback, NULL);
	} else {
		indigo_set_switch(MOUNT_PEC_TRAINING_PROPERTY, MOUNT_PEC_TRAINIG_STOPPED_ITEM, true);
		indigo_update_property(device, MOUNT_PEC_TRAINING_PROPERTY, "Cleared PPEC training state");
	}
}

void mount_handle_train_ppec(indigo_device *device) {
	if (!synscan_ext_setting(device, kAxisRA,  MOUNT_PEC_TRAINIG_STARTED_ITEM->sw.value ? kStarPECCTtraining : kStopPECCTtraining)) {
		MOUNT_PEC_TRAINING_PROPERTY->state = INDIGO_ALERT_STATE;
		indigo_update_property(device, MOUNT_PEC_TRAINING_PROPERTY, "Failed to update PPEC training state");
	} else {
		MOUNT_PEC_TRAINING_PROPERTY->state = INDIGO_OK_STATE;
		indigo_update_property(device, MOUNT_PEC_TRAINING_PROPERTY, "Updated PPEC training state");
	}
	if (MOUNT_PEC_TRAINIG_STARTED_ITEM->sw.value || MOUNT_PEC_TRAINIG_STARTED_ITEM->sw.value)
		indigo_set_timer(device, 1, mount_train_ppec_callback, NULL);
}

// Code based on: https://stargazerslounge.com/topic/238364-eq8-tools-autohome-without-handset/

static void mount_autohome_timer_callback(indigo_device* device) {
	pthread_mutex_lock(&PRIVATE_DATA->driver_mutex);
	long value, position_ra, position_dec;
	INDIGO_DRIVER_DEBUG(DRIVER_NAME, "********** Auto home procedure started");
	INDIGO_DRIVER_DEBUG(DRIVER_NAME, "Steps/Revolution RA = %06lx, DEC = %06lx", PRIVATE_DATA->raTotalSteps & 0xFFFFFF, PRIVATE_DATA->decTotalSteps & 0xFFFFFF);

	INDIGO_DRIVER_DEBUG(DRIVER_NAME, "Turn encoders off");
	MOUNT_USE_RA_ENCODER_ITEM->sw.value = MOUNT_USE_DEC_ENCODER_ITEM->sw.value = false;
	mount_handle_encoders(device);
	INDIGO_DRIVER_DEBUG(DRIVER_NAME, "Make sure axes are stopped");
	synscan_stop_axis(device, kAxisRA);
	synscan_stop_axis(device, kAxisDEC);
	INDIGO_DRIVER_DEBUG(DRIVER_NAME, "Wait until both axis stops");
	synscan_wait_for_axis_stopped(device, kAxisRA, NULL);
	synscan_wait_for_axis_stopped(device, kAxisDEC, NULL);

	INDIGO_DRIVER_DEBUG(DRIVER_NAME, "Reset coordinates");
	synscan_init_axis_position(device, kAxisRA, PRIVATE_DATA->raHomePosition);
	synscan_init_axis_position(device, kAxisDEC, PRIVATE_DATA->decHomePosition);

	INDIGO_DRIVER_DEBUG(DRIVER_NAME, "********** Try to get reliable home index signal");
	INDIGO_DRIVER_DEBUG(DRIVER_NAME, "Reset home position index");
	synscan_ext_setting(device, kAxisRA, kResetHomeIndexer);
	synscan_ext_setting(device, kAxisDEC, kResetHomeIndexer);
	INDIGO_DRIVER_DEBUG(DRIVER_NAME, "Read home position index data");
	synscan_ext_inquiry(device, kAxisRA, kGetHomeIndex, &value);
	bool slewing_up_ra = value == 0;
	synscan_ext_inquiry(device, kAxisDEC, kGetHomeIndex, &value);
	bool slewing_up_dec = value == 0;
	INDIGO_DRIVER_DEBUG(DRIVER_NAME, "Slewing up RA = %d, DEC = %d", slewing_up_ra, slewing_up_dec);
	INDIGO_DRIVER_DEBUG(DRIVER_NAME, "Set slewing mode");
	synscan_set_axis_gearing(device, kAxisRA, kAxisDirectionFwd, kAxisSpeedAbsSlew);
	synscan_set_axis_gearing(device, kAxisDEC, kAxisDirectionFwd, kAxisSpeedAbsSlew);
	INDIGO_DRIVER_DEBUG(DRIVER_NAME, "Aquire current position");
	synscan_axis_position(device, kAxisRA, &position_ra);
	synscan_axis_position(device, kAxisDEC, &position_dec);
	INDIGO_DRIVER_DEBUG(DRIVER_NAME, "RA = %06lx, DEC = %06lx", position_ra & 0xFFFFFF, position_dec & 0xFFFFFF);
	INDIGO_DRIVER_DEBUG(DRIVER_NAME, "Calculate target position");
	value = 5.0 / 360.0 *  PRIVATE_DATA->raTotalSteps;
	position_ra = position_ra + (slewing_up_ra ? -value : value);
	value = 5.0 / 360.0 *  PRIVATE_DATA->decTotalSteps;
	position_dec = position_dec + (slewing_up_dec ? -value : value);
	INDIGO_DRIVER_DEBUG(DRIVER_NAME, "RA = %06lx, DEC = %06lx", position_ra & 0xFFFFFF, position_dec & 0xFFFFFF);
	INDIGO_DRIVER_DEBUG(DRIVER_NAME, "Set target position");
	synscan_set_goto_target(device, kAxisRA, position_ra);
	synscan_set_goto_target(device, kAxisDEC, position_dec);
	INDIGO_DRIVER_DEBUG(DRIVER_NAME, "Slew away from current position");
	synscan_slew_axis(device, kAxisRA);
	synscan_slew_axis(device, kAxisDEC);
	INDIGO_DRIVER_DEBUG(DRIVER_NAME, "Wait until both axis stops");
	synscan_wait_for_axis_stopped(device, kAxisRA, NULL);
	synscan_wait_for_axis_stopped(device, kAxisDEC, NULL);
	INDIGO_DRIVER_DEBUG(DRIVER_NAME, "Check home index status");
	synscan_ext_inquiry(device, kAxisRA, kGetHomeIndex, &value);
	bool index_changed_ra = !((value == 0) || (value == 0xFFFFFF));
	synscan_ext_inquiry(device, kAxisDEC, kGetHomeIndex, &value);
	bool index_changed_dec = !((value == 0) || (value == 0xFFFFFF));
	INDIGO_DRIVER_DEBUG(DRIVER_NAME, "RA changed = %d, DEC changed = %d", index_changed_ra, index_changed_dec);
	if (index_changed_ra) {
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "Make extra movement RA");
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "Set slewing mode");
		synscan_set_axis_gearing(device, kAxisRA, kAxisDirectionFwd, kAxisSpeedAbsSlew);
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "Aquire current position");
		synscan_axis_position(device, kAxisRA, &position_ra);
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "RA = %06lx", position_ra & 0xFFFFFF);
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "Calculate target position");
		value = 5.0 / 360.0 *  PRIVATE_DATA->raTotalSteps;
		position_ra = position_ra + (slewing_up_ra ? -value : value);
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "RA = %06lx", position_ra & 0xFFFFFF);
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "Set target position");
		synscan_set_goto_target(device, kAxisRA, position_ra);
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "Slew away from current position");
		synscan_slew_axis(device, kAxisRA);
	}
	if (index_changed_dec) {
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "Make extra movement DEC");
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "Set slewing mode");
		synscan_set_axis_gearing(device, kAxisDEC, kAxisDirectionFwd, kAxisSpeedAbsSlew);
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "Aquire current position");
		synscan_axis_position(device, kAxisDEC, &position_dec);
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "Calculate target position");
		value = 5.0 / 360.0 *  PRIVATE_DATA->raTotalSteps;
		position_dec = position_dec + (slewing_up_dec ? -value : value);
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "DEC = %06lx", position_dec & 0xFFFFFF);
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "Set target position");
		synscan_set_goto_target(device, kAxisDEC, position_dec);
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "Slew 5 deg away from current position");
		synscan_slew_axis(device, kAxisDEC);
	}
	INDIGO_DRIVER_DEBUG(DRIVER_NAME, "Wait until both axis stops");
	synscan_wait_for_axis_stopped(device, kAxisRA, NULL);
	synscan_wait_for_axis_stopped(device, kAxisDEC, NULL);
	INDIGO_DRIVER_DEBUG(DRIVER_NAME, "Reset home position index if needed");
	if (index_changed_ra) {
		synscan_ext_setting(device, kAxisRA, kResetHomeIndexer);
		synscan_ext_inquiry(device, kAxisRA, kGetHomeIndex, &value);
		slewing_up_ra = value == 0;
	}
	if (index_changed_dec) {
		synscan_ext_setting(device, kAxisDEC, kResetHomeIndexer);
		synscan_ext_inquiry(device, kAxisDEC, kGetHomeIndex, &value);
		slewing_up_dec = value == 0;
	}
	INDIGO_DRIVER_DEBUG(DRIVER_NAME, "Slewing up RA = %d, DEC = %d", slewing_up_ra, slewing_up_dec);

	INDIGO_DRIVER_DEBUG(DRIVER_NAME, "********** Move the axis to negative position");
	if (!slewing_up_ra) {
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "Set RA slewing mode");
		synscan_set_axis_gearing(device, kAxisRA, kAxisDirectionRev, kAxisSpeedHigh);
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "Set RA slewing speed");
		synscan_set_axis_slew_rate(device, kAxisRA, 0x6);
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "Start RA slewing");
		synscan_slew_axis(device, kAxisRA);
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "Wait RA home position index change");
		while (true) {
			synscan_ext_inquiry(device, kAxisRA, kGetHomeIndex, &value);
			if (value != 0xFFFFFF)
				break;
		}
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "Wait 3s");
		indigo_usleep(3 * ONE_SECOND_DELAY);
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "Stop RA slewing");
		synscan_stop_axis(device, kAxisRA);
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "Wait until RA stops");
		synscan_wait_for_axis_stopped(device, kAxisRA, NULL);
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "Reset RA home position index");
		synscan_ext_setting(device, kAxisRA, kResetHomeIndexer);
		slewing_up_ra = true;
	}
	if (!slewing_up_dec) {
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "Set DEC slewing mode");
		synscan_set_axis_gearing(device, kAxisDEC, kAxisDirectionRev, kAxisSpeedHigh);
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "Set DEC slewing speed");
		synscan_set_axis_slew_rate(device, kAxisDEC, 0x6);
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "Start DEC slewing");
		synscan_slew_axis(device, kAxisDEC);
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "Wait DEC home position index change");
		while (true) {
			synscan_ext_inquiry(device, kAxisDEC, kGetHomeIndex, &value);
			if (value != 0xFFFFFF)
				break;
		}
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "Wait 3s");
		indigo_usleep(3 * ONE_SECOND_DELAY);
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "Stop DEC slewing");
		synscan_stop_axis(device, kAxisDEC);
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "Wait until DEC stops");
		synscan_wait_for_axis_stopped(device, kAxisDEC, NULL);
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "Reset DEC home position index");
		synscan_ext_setting(device, kAxisDEC, kResetHomeIndexer);
		slewing_up_dec = true;
	}

	INDIGO_DRIVER_DEBUG(DRIVER_NAME, "********** Find the home position index");
	INDIGO_DRIVER_DEBUG(DRIVER_NAME, "Set slewing mode");
	synscan_set_axis_gearing(device, kAxisRA, kAxisDirectionFwd, kAxisSpeedHigh);
	synscan_set_axis_gearing(device, kAxisDEC, kAxisDirectionFwd, kAxisSpeedHigh);
	INDIGO_DRIVER_DEBUG(DRIVER_NAME, "Set slewing speed");
	synscan_set_axis_slew_rate(device, kAxisRA, 6);
	synscan_set_axis_slew_rate(device, kAxisDEC, 6);
	INDIGO_DRIVER_DEBUG(DRIVER_NAME, "Start slewing");
	synscan_slew_axis(device, kAxisRA);
	synscan_slew_axis(device, kAxisDEC);
	index_changed_ra = index_changed_dec = false;
	long home_position_ra = 0, home_position_dec = 0;
	while (!index_changed_ra || !index_changed_dec) {
		if (!index_changed_ra) {
			synscan_ext_inquiry(device, kAxisRA, kGetHomeIndex, &value);
			if (value != 0) {
				position_ra = home_position_ra = value;
				index_changed_ra = true;
				synscan_stop_axis(device, kAxisRA);
			}
		}
		if (!index_changed_dec) {
			synscan_ext_inquiry(device, kAxisDEC, kGetHomeIndex, &value);
			if (value != 0) {
				position_dec = home_position_dec = value;
				index_changed_dec = true;
				synscan_stop_axis(device, kAxisDEC);
			}
		}
	}
	INDIGO_DRIVER_DEBUG(DRIVER_NAME, "Wait until both axis stops");
	synscan_wait_for_axis_stopped(device, kAxisRA, NULL);
	synscan_wait_for_axis_stopped(device, kAxisDEC, NULL);

	INDIGO_DRIVER_DEBUG(DRIVER_NAME, "********** Move back");
	INDIGO_DRIVER_DEBUG(DRIVER_NAME, "Set slewing mode");
	synscan_set_axis_gearing(device, kAxisRA, kAxisDirectionFwd, kAxisSpeedAbsSlew);
	synscan_set_axis_gearing(device, kAxisDEC, kAxisDirectionFwd, kAxisSpeedAbsSlew);
	INDIGO_DRIVER_DEBUG(DRIVER_NAME, "Calculate target position");
	position_ra -= 10.0 / 360.0 *  PRIVATE_DATA->raTotalSteps;
	position_dec -= 10.0 / 360.0 *  PRIVATE_DATA->decTotalSteps;
	INDIGO_DRIVER_DEBUG(DRIVER_NAME, "%06lx %06lx", position_ra & 0xFFFFFF, position_dec & 0xFFFFFF);
	INDIGO_DRIVER_DEBUG(DRIVER_NAME, "Set target position");
	synscan_set_goto_target(device, kAxisRA, position_ra);
	synscan_set_goto_target(device, kAxisDEC, position_dec);
	INDIGO_DRIVER_DEBUG(DRIVER_NAME, "Slew 10deg away from current position");
	synscan_slew_axis(device, kAxisRA);
	synscan_slew_axis(device, kAxisDEC);
	INDIGO_DRIVER_DEBUG(DRIVER_NAME, "Wait until both axis stops");
	synscan_wait_for_axis_stopped(device, kAxisRA, NULL);
	synscan_wait_for_axis_stopped(device, kAxisDEC, NULL);

	INDIGO_DRIVER_DEBUG(DRIVER_NAME, "********** Goto home position");
	INDIGO_DRIVER_DEBUG(DRIVER_NAME, "Set slewing mode");
	synscan_set_axis_gearing(device, kAxisRA, kAxisDirectionFwd, kAxisSpeedAbsSlew);
	synscan_set_axis_gearing(device, kAxisDEC, kAxisDirectionFwd, kAxisSpeedAbsSlew);
	INDIGO_DRIVER_DEBUG(DRIVER_NAME, "Set target position");
	synscan_set_goto_target(device, kAxisRA, home_position_ra);
	synscan_set_goto_target(device, kAxisDEC, home_position_dec);
	INDIGO_DRIVER_DEBUG(DRIVER_NAME, "Slew home");
	synscan_slew_axis(device, kAxisRA);
	synscan_slew_axis(device, kAxisDEC);
	INDIGO_DRIVER_DEBUG(DRIVER_NAME, "Wait until both axis stops");
	synscan_wait_for_axis_stopped(device, kAxisRA, NULL);
	synscan_wait_for_axis_stopped(device, kAxisDEC, NULL);

	INDIGO_DRIVER_DEBUG(DRIVER_NAME, "********** Reset mount positions");
	synscan_init_axis_position(device, kAxisRA, PRIVATE_DATA->raHomePosition);
	synscan_init_axis_position(device, kAxisDEC, PRIVATE_DATA->decHomePosition + MOUNT_AUTOHOME_DEC_OFFSET_ITEM->number.target / 360.0 *  PRIVATE_DATA->decTotalSteps);

	PRIVATE_DATA->globalMode = kGlobalModeIdle;
	MOUNT_AUTOHOME_PROPERTY->state = INDIGO_OK_STATE;
	indigo_update_property(device, MOUNT_AUTOHOME_PROPERTY, NULL);
	pthread_mutex_unlock(&PRIVATE_DATA->driver_mutex);
}

void mount_handle_autohome(indigo_device *device) {
	if (MOUNT_AUTOHOME_ITEM->sw.value) {
		MOUNT_AUTOHOME_ITEM->sw.value = false;
		if (PRIVATE_DATA->globalMode == kGlobalModeIdle) {
			MOUNT_AUTOHOME_PROPERTY->state = INDIGO_BUSY_STATE;
			indigo_update_property(device, MOUNT_AUTOHOME_PROPERTY, "Starting auto home procedure...");
			PRIVATE_DATA->globalMode = kGlobalModeGoingHome;
			indigo_set_timer(device, 0, mount_autohome_timer_callback, NULL);
		} else {
			MOUNT_AUTOHOME_PROPERTY->state = INDIGO_ALERT_STATE;
			indigo_update_property(device, MOUNT_AUTOHOME_PROPERTY, "Auto home not started - mount is busy.");
		}
	}
}
