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
#include "indigo_io.h"
#include "indigo_novas.h"
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
	if (PRIVATE_DATA->handle > 0) {
		//  Longitude needed for LST
		double lng = MOUNT_GEOGRAPHIC_COORDINATES_LONGITUDE_ITEM->number.value;
		double lat = MOUNT_GEOGRAPHIC_COORDINATES_LATITUDE_ITEM->number.value * M_PI / 180.0;

		//  Get the raw coords
		synscan_get_coords(device);
		
		//  Get the LST as quickly as we can after the HA
		double lst = indigo_lst(lng);

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

			if (aligned_ha < 0.0)
				aligned_ha += 24.0;
			if (aligned_ha >= 24.0)
				aligned_ha -= 24.0;
			aligned_ha = aligned_ha * M_PI / 12.0;

			sla_de2h(aligned_ha, aligned_dec, lat, &az, &alt);

			MOUNT_HORIZONTAL_COORDINATES_ALT_ITEM->number.value = alt * 180.0 / M_PI;
			MOUNT_HORIZONTAL_COORDINATES_AZ_ITEM->number.value = az * 180.0 / M_PI;
			MOUNT_HORIZONTAL_COORDINATES_PROPERTY->state = MOUNT_EQUATORIAL_COORDINATES_PROPERTY->state;
			indigo_update_property(device, MOUNT_HORIZONTAL_COORDINATES_PROPERTY, NULL);
		}
	}
	indigo_reschedule_timer(device, 0.5, &PRIVATE_DATA->position_timer);
}

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
		indigo_send_message(device, "Failed to connect to %s", name);
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

static void synscan_connect_timer_callback(indigo_device* device) {
	//  Lock the driver
	pthread_mutex_lock(&PRIVATE_DATA->driver_mutex);
	
	//assert(CONNECTION_PROPERTY->state == INDIGO_BUSY_STATE);
	
	//  Open and configure the mount
	//  FIXME - master device
	bool result = synscan_open(device->master_device) && synscan_configure(device->master_device);
	if (result) {
		PRIVATE_DATA->device_count++;
		CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
		indigo_update_property(device, CONNECTION_PROPERTY, "connected to mount!");
		
		indigo_set_switch(MOUNT_TRACKING_PROPERTY, MOUNT_TRACKING_OFF_ITEM, true);

		//  Here I need to invoke the code in indigo_mount_driver.c on lines 270-334 to define the properties that should now be present.
		indigo_mount_change_property(device, NULL, CONNECTION_PROPERTY);
		indigo_define_property(device, MOUNT_POLARSCOPE_PROPERTY, NULL);
		indigo_define_property(device, MOUNT_OPERATING_MODE_PROPERTY, NULL);

		//  Start position timer
		PRIVATE_DATA->position_timer = indigo_set_timer(device->master_device, 0, position_timer_callback);
	}
	else {
		CONNECTION_PROPERTY->state = INDIGO_ALERT_STATE;
		indigo_set_switch(CONNECTION_PROPERTY, CONNECTION_DISCONNECTED_ITEM, true);
		indigo_update_property(device, CONNECTION_PROPERTY, "Failed to connect to mount");
		indigo_delete_property(device, MOUNT_POLARSCOPE_PROPERTY, NULL);
		indigo_delete_property(device, MOUNT_OPERATING_MODE_PROPERTY, NULL);
	}
	
	//  Need to define and delete the 2 custom properties on connect/disconnect

	//  Unlock the driver
	pthread_mutex_unlock(&PRIVATE_DATA->driver_mutex);
}

void synscan_mount_connect(indigo_device* device) {
	//  Ignore if we are already processing a connection change
	if (CONNECTION_PROPERTY->state == INDIGO_BUSY_STATE)
		return;
	
	//  Handle connect/disconnect commands
	if (CONNECTION_CONNECTED_ITEM->sw.value) {
		//  CONNECT to the mount
		if (PRIVATE_DATA->device_count == 0) {
			CONNECTION_PROPERTY->state = INDIGO_BUSY_STATE;
			//  No need to store the timer as this is a one-shot
			indigo_set_timer(device, 0, &synscan_connect_timer_callback);
			return;
		}
		else
			PRIVATE_DATA->device_count++;
	}
	else if (CONNECTION_DISCONNECTED_ITEM->sw.value) {
		//  DISCONNECT from mount
		if (PRIVATE_DATA->device_count > 0) {
			PRIVATE_DATA->device_count--;
			if (PRIVATE_DATA->device_count == 0) {
				synscan_close(device);
				indigo_cancel_timer(device, &PRIVATE_DATA->position_timer);
				indigo_delete_property(device, MOUNT_POLARSCOPE_PROPERTY, NULL);
				indigo_delete_property(device, MOUNT_OPERATING_MODE_PROPERTY, NULL);
			}
		}
	}
	CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
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

	//**  Perform preliminary slew on both axes
	//  Compute initial target positions
	double ra, dec;
	indigo_translated_to_raw(device, MOUNT_EQUATORIAL_COORDINATES_RA_ITEM->number.target, MOUNT_EQUATORIAL_COORDINATES_DEC_ITEM->number.target, &ra, &dec);
	ra = ra * M_PI / 12.0;
	dec = dec * M_PI / 180.0;
	double lng = MOUNT_GEOGRAPHIC_COORDINATES_LONGITUDE_ITEM->number.value;
	double lst = indigo_lst(lng) * M_PI / 12.0;
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
	double target_lst = indigo_lst(lng) + (5 / 3600.0);
	bool lst_wrap = false;
	if (target_lst >= 24.0) {
		target_lst -= 24.0;
		lst_wrap = true;
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
		lst = indigo_lst(lng);

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
		usleep(100000);
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
	//double lst = indigo_lst(lng) * M_PI / 12.0;
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
//	double target_lst = indigo_lst(lng) + (5 / 3600.0);
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
//		lst = indigo_lst(lng);
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
//		usleep(100000);
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
		indigo_set_timer(device, 0, mount_slew_timer_callback);
	}
}

void mount_handle_aa_coordinates(indigo_device *device) {
//	if (MOUNT_ON_COORDINATES_SET_SLEW_ITEM->sw.value || MOUNT_ON_COORDINATES_SET_TRACK_ITEM->sw.value) {
//		MOUNT_HORIZONTAL_COORDINATES_PROPERTY->state = INDIGO_BUSY_STATE;
//		indigo_update_coordinates(device, "Slewing...");
//		
//		//  Start slew timer thread
//		PRIVATE_DATA->globalMode = kGlobalModeSlewing;
//		indigo_set_timer(device, 0, mount_slew_aa_timer_callback);
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
		indigo_set_timer(device, 0, mount_update_tracking_rate_timer_callback);
	}
	else {
		MOUNT_TRACK_RATE_PROPERTY->state = INDIGO_OK_STATE;
		indigo_update_property(device, MOUNT_TRACK_RATE_PROPERTY, NULL);
	}
}

static void mount_tracking_timer_callback(indigo_device* device) {
	pthread_mutex_lock(&PRIVATE_DATA->driver_mutex);
	char* message;
	if (MOUNT_TRACKING_ON_ITEM->sw.value) {
		//  Start tracking at the configured rate
		double axisRate = synscan_tracking_rate(device);
		synscan_slew_axis_at_rate(device, kAxisRA, axisRate);
		PRIVATE_DATA->raAxisMode = kAxisModeTracking;
		message = "Tracking started.";
	}
	else if (MOUNT_TRACKING_OFF_ITEM->sw.value) {
		synscan_stop_axis(device, kAxisRA);
		synscan_wait_for_axis_stopped(device, kAxisRA, NULL);
		PRIVATE_DATA->raAxisMode = kAxisModeIdle;
		message = "Tracking stopped.";
	}
	MOUNT_TRACKING_PROPERTY->state = INDIGO_OK_STATE;
	indigo_update_property(device, MOUNT_TRACKING_PROPERTY, message);
	pthread_mutex_unlock(&PRIVATE_DATA->driver_mutex);
}

void mount_handle_tracking(indigo_device *device) {
	MOUNT_TRACKING_PROPERTY->state = INDIGO_BUSY_STATE;
	indigo_update_property(device, MOUNT_TRACKING_PROPERTY, NULL);
	indigo_set_timer(device, 0, mount_tracking_timer_callback);
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
		indigo_set_timer(device, 0, manual_slew_west_timer_callback);
	} else if (MOUNT_MOTION_EAST_ITEM->sw.value) {
		MOUNT_MOTION_RA_PROPERTY->state = INDIGO_BUSY_STATE;
		indigo_update_property(device, MOUNT_MOTION_RA_PROPERTY, NULL);
		indigo_set_timer(device, 0, manual_slew_east_timer_callback);
	} else {
		indigo_set_timer(device, 0, manual_slew_ra_stop_timer_callback);
	}
}

void mount_handle_motion_dec(indigo_device *device) {
	if(MOUNT_MOTION_NORTH_ITEM->sw.value) {
		MOUNT_MOTION_DEC_PROPERTY->state = INDIGO_BUSY_STATE;
		indigo_update_property(device, MOUNT_MOTION_DEC_PROPERTY, NULL);
		indigo_set_timer(device, 0, manual_slew_north_timer_callback);
	} else if (MOUNT_MOTION_SOUTH_ITEM->sw.value) {
		MOUNT_MOTION_DEC_PROPERTY->state = INDIGO_BUSY_STATE;
		indigo_update_property(device, MOUNT_MOTION_DEC_PROPERTY, NULL);
		indigo_set_timer(device, 0, manual_slew_south_timer_callback);
	} else {
		indigo_set_timer(device, 0, manual_slew_dec_stop_timer_callback);
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
	double ha = MOUNT_PARK_POSITION_HA_ITEM->number.value * M_PI / 12.0;
	double dec = MOUNT_PARK_POSITION_DEC_ITEM->number.value * M_PI / 180.0;
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
	MOUNT_PARK_PARKED_ITEM->sw.value = true;
	MOUNT_PARK_PROPERTY->state = INDIGO_OK_STATE;
	indigo_update_property(device, MOUNT_PARK_PROPERTY, "Mount parked.");
	PRIVATE_DATA->globalMode = kGlobalModeIdle;
	pthread_mutex_unlock(&PRIVATE_DATA->driver_mutex);
}

void mount_handle_park(indigo_device* device) {
	if (MOUNT_PARK_PARKED_ITEM->sw.value) {
		if (PRIVATE_DATA->globalMode == kGlobalModeIdle) {
			MOUNT_PARK_PROPERTY->state = INDIGO_BUSY_STATE;
			indigo_update_property(device, MOUNT_PARK_PROPERTY, "Parking...");

			PRIVATE_DATA->globalMode = kGlobalModeParking;
			indigo_set_timer(device, 0, mount_park_timer_callback);
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
		//  Cancel any park or slew in progress
		PRIVATE_DATA->abort_motion = true;

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
