// Copyright (c) 2021 CloudMakers, s. r. o.
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

/** INDIGO Generic platesolver base
 \file indigo_platesolver_driver.c
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <assert.h>
#include <string.h>
#include <errno.h>
#include <time.h>
#include <math.h>
#include <fcntl.h>
#include <sys/stat.h>

#include <indigo/indigo_agent.h>
#include <indigo/indigo_filter.h>
#include <indigo/indigo_novas.h>
#include <indigo/indigo_platesolver.h>

//#define HIDE_POLAR_ALIGN

// -------------------------------------------------------------------------------- INDIGO agent device implementation

static bool validate_related_agent(indigo_device *device, indigo_property *info_property, int mask) {
	if ((mask & INDIGO_INTERFACE_CCD) == INDIGO_INTERFACE_CCD)
		return true;
	if (!strncmp(info_property->device, "Mount Agent", 11))
		return true;
	return false;
}

bool indigo_platesolver_validate_executable(const char *executable) {
	char command[128];
	snprintf(command, sizeof(command), "command -v %s", executable);
	FILE *output = NULL;
	char *line = NULL;
	size_t size = 0;
	output = popen(command, "r");
	ssize_t result = getline(&line, &size, output);
	if (result > 1) {
		INDIGO_DEBUG(indigo_debug("indigo_platesolver_validate_executable: %s", line));
	} else {
		indigo_error("indigo_platesolver_validate_executable: %s not found", executable);
	}
	pclose(output);
	free(line);
	return result > 1;
}

void indigo_platesolver_save_config(indigo_device *device) {
	if (pthread_mutex_trylock(&DEVICE_CONTEXT->config_mutex) == 0) {
		pthread_mutex_unlock(&DEVICE_CONTEXT->config_mutex);
		pthread_mutex_lock(&INDIGO_PLATESOLVER_DEVICE_PRIVATE_DATA->mutex);
		indigo_save_property(device, NULL, AGENT_PLATESOLVER_USE_INDEX_PROPERTY);
		indigo_save_property(device, NULL, AGENT_PLATESOLVER_HINTS_PROPERTY);
		indigo_save_property(device, NULL, AGENT_PLATESOLVER_SYNC_PROPERTY);
#ifdef HIDE_POLAR_ALIGN
		indigo_save_property(device, NULL, AGENT_PLATESOLVER_PA_SETTINGS_PROPERTY);
#endif
		if (DEVICE_CONTEXT->property_save_file_handle) {
			CONFIG_PROPERTY->state = INDIGO_OK_STATE;
			close(DEVICE_CONTEXT->property_save_file_handle);
			DEVICE_CONTEXT->property_save_file_handle = 0;
		} else {
			CONFIG_PROPERTY->state = INDIGO_ALERT_STATE;
		}
		CONFIG_SAVE_ITEM->sw.value = false;
		indigo_update_property(device, CONFIG_PROPERTY, NULL);
		pthread_mutex_unlock(&INDIGO_PLATESOLVER_DEVICE_PRIVATE_DATA->mutex);
	}
}

static void set_pa_reference(indigo_device *device) {
	double lst_now = indigo_lst(NULL, INDIGO_PLATESOLVER_DEVICE_PRIVATE_DATA->geo_coordinates.a * RAD2DEG);
	indigo_ra_dec_to_point(
		AGENT_PLATESOLVER_WCS_RA_ITEM->number.value,
		AGENT_PLATESOLVER_WCS_DEC_ITEM->number.value,
		lst_now,
		&INDIGO_PLATESOLVER_DEVICE_PRIVATE_DATA->pa_reference1
	);

	INDIGO_PLATESOLVER_DEVICE_PRIVATE_DATA->pa_error_at_reference2.a =
	INDIGO_PLATESOLVER_DEVICE_PRIVATE_DATA->pa_error_at_reference2.d =
	INDIGO_PLATESOLVER_DEVICE_PRIVATE_DATA->pa_error_at_reference2.r =
	INDIGO_PLATESOLVER_DEVICE_PRIVATE_DATA->pa_reference2.a =
	INDIGO_PLATESOLVER_DEVICE_PRIVATE_DATA->pa_reference2.d =
	INDIGO_PLATESOLVER_DEVICE_PRIVATE_DATA->pa_reference2.r =
	INDIGO_PLATESOLVER_DEVICE_PRIVATE_DATA->pa_lst_at_reference2 = 0;

	if (AGENT_PLATESOLVER_PA_STATE_ITEM->number.value == POLAR_ALIGN_IDLE) {
		AGENT_PLATESOLVER_PA_STATE_PROPERTY->state = INDIGO_IDLE_STATE;
	}
	AGENT_PLATESOLVER_PA_STATE_AZ_ERROR_ITEM->number.value =
	AGENT_PLATESOLVER_PA_STATE_ALT_ERROR_ITEM->number.value =
	AGENT_PLATESOLVER_PA_STATE_HA_DRIFT_ITEM->number.value =
	AGENT_PLATESOLVER_PA_STATE_DEC_DRIFT_ITEM->number.value =
	AGENT_PLATESOLVER_PA_STATE_AZ_CORRECTION_CW_ITEM->number.value =
	AGENT_PLATESOLVER_PA_STATE_ALT_CORRECTION_UP_ITEM->number.value =
	AGENT_PLATESOLVER_PA_STATE_TOTAL_ERROR_ITEM->number.value = 0;
	indigo_update_property(device, AGENT_PLATESOLVER_PA_STATE_PROPERTY, NULL);

	indigo_log("%s(): Polar align: Reference LST = %f, Lon = %f, Lat = %f", __FUNCTION__, lst_now,  INDIGO_PLATESOLVER_DEVICE_PRIVATE_DATA->geo_coordinates.a * RAD2DEG, INDIGO_PLATESOLVER_DEVICE_PRIVATE_DATA->geo_coordinates.d * RAD2DEG);
	indigo_log("%s(): Polar align: Reference HA = %f, Dec = %f", __FUNCTION__, INDIGO_PLATESOLVER_DEVICE_PRIVATE_DATA->pa_reference1.a * RAD2DEG / 15, INDIGO_PLATESOLVER_DEVICE_PRIVATE_DATA->pa_reference1.d * RAD2DEG);
}

static bool set_fov(indigo_device *device, double angle, double width, double height) {
	for (int i = 0; i < FILTER_RELATED_AGENT_LIST_PROPERTY->count; i++) {
		indigo_item *item = FILTER_RELATED_AGENT_LIST_PROPERTY->items + i;
		if (item->sw.value && !strncmp(item->name, "Mount Agent", 11)) {
			const char *item_names[] = { AGENT_MOUNT_FOV_ANGLE_ITEM_NAME, AGENT_MOUNT_FOV_WIDTH_ITEM_NAME, AGENT_MOUNT_FOV_HEIGHT_ITEM_NAME };
			double item_values[] = { angle, width, height };
			indigo_change_number_property(FILTER_DEVICE_CONTEXT->client, item->name, AGENT_MOUNT_FOV_PROPERTY_NAME, 3, item_names, item_values);
			return true;
		}
	}
	indigo_send_message(device, "No mount agent selected");
	return false;
}

#define mount_sync(device, ra, dec, settle_time) mount_control(device, MOUNT_ON_COORDINATES_SET_SYNC_ITEM_NAME, ra, dec, settle_time)
#define mount_slew(device, ra, dec, settle_time) mount_control(device, MOUNT_ON_COORDINATES_SET_TRACK_ITEM_NAME, ra, dec, settle_time)

static bool mount_control(indigo_device *device, char *operation, double ra, double dec, double settle_time) {
	for (int i = 0; i < FILTER_RELATED_AGENT_LIST_PROPERTY->count; i++) {
		indigo_item *item = FILTER_RELATED_AGENT_LIST_PROPERTY->items + i;
		if (item->sw.value && !strncmp(item->name, "Mount Agent", 11)) {
			//indigo_debug("'%s'.'MOUNT_ON_COORDINATES_SET' requested %s", item->name, operation);
			INDIGO_PLATESOLVER_DEVICE_PRIVATE_DATA->on_coordinates_set_state = INDIGO_IDLE_STATE;
			indigo_change_switch_property_1(FILTER_DEVICE_CONTEXT->client, item->name, MOUNT_ON_COORDINATES_SET_PROPERTY_NAME, operation, true);
			for (int i = 0; i < 300; i++) { // wait 3s to become OK
				if (INDIGO_PLATESOLVER_DEVICE_PRIVATE_DATA->on_coordinates_set_state == INDIGO_OK_STATE)
					break;
				indigo_usleep(10000);
			}
			if (INDIGO_PLATESOLVER_DEVICE_PRIVATE_DATA->on_coordinates_set_state != INDIGO_OK_STATE) {
				indigo_error("MOUNT_ON_COORDINATES_SET didn't become OK in 3s");
				return false;
			}
			const char *item_names[] = { MOUNT_EQUATORIAL_COORDINATES_RA_ITEM_NAME, MOUNT_EQUATORIAL_COORDINATES_DEC_ITEM_NAME };
			double item_values[] = { ra, dec };
			INDIGO_PLATESOLVER_DEVICE_PRIVATE_DATA->eq_coordinates_state = INDIGO_IDLE_STATE;
			indigo_log("'%s'.'MOUNT_EQUATORIAL_COORDINATES' requested RA=%g, DEC=%g", item->name, ra, dec);
			indigo_change_number_property(FILTER_DEVICE_CONTEXT->client, item->name, MOUNT_EQUATORIAL_COORDINATES_PROPERTY_NAME, 2, item_names, item_values);
			for (int i = 0; i < 300; i++) { // wait 3 s to become BUSY
				if (INDIGO_PLATESOLVER_DEVICE_PRIVATE_DATA->eq_coordinates_state == INDIGO_BUSY_STATE)
					break;
				indigo_usleep(10000);
			}
			if (INDIGO_PLATESOLVER_DEVICE_PRIVATE_DATA->eq_coordinates_state != INDIGO_BUSY_STATE) {
				indigo_debug("MOUNT_EQUATORIAL_COORDINATES didn't become BUSY in 3s");
			}
			for (int i = 0; i < 6000; i++) { // wait 60s to become not BUSY
				if (INDIGO_PLATESOLVER_DEVICE_PRIVATE_DATA->eq_coordinates_state != INDIGO_BUSY_STATE)
					break;
				indigo_usleep(10000);
			}
			if (INDIGO_PLATESOLVER_DEVICE_PRIVATE_DATA->eq_coordinates_state != INDIGO_OK_STATE) {
				indigo_error("MOUNT_EQUATORIAL_COORDINATES didn't become OK in 60s");
				return false;
			}
			indigo_usleep(ONE_SECOND_DELAY * settle_time);
			return true;
		}
	}
	indigo_send_message(device, "No mount agent selected");
	return false;
}

static bool start_exposure(indigo_device *device, double exposure) {
	for (int i = 0; i < FILTER_RELATED_AGENT_LIST_PROPERTY->count; i++) {
		indigo_item *item = FILTER_RELATED_AGENT_LIST_PROPERTY->items + i;
		if (item->sw.value && (!strncmp(item->name, "Imager Agent", 12) || !strncmp(item->name, "Guider Agent", 12))) {
			indigo_change_number_property_1(FILTER_DEVICE_CONTEXT->client, item->name, CCD_EXPOSURE_PROPERTY_NAME, CCD_EXPOSURE_ITEM_NAME, exposure);
			return true;
		}
	}
	indigo_send_message(device, "No image source agent selected");
	return false;
}

static bool calculate_pa_error(indigo_device *device) {
	bool compensate_refraction = (bool)AGENT_PLATESOLVER_PA_SETTINGS_COMPENSATE_REFRACTION_ITEM->number.value;
	if (INDIGO_PLATESOLVER_DEVICE_PRIVATE_DATA->pa_reference1.r == 0) {
		AGENT_PLATESOLVER_PA_STATE_ITEM->number.value =
		AGENT_PLATESOLVER_PA_STATE_AZ_ERROR_ITEM->number.value =
		AGENT_PLATESOLVER_PA_STATE_ALT_ERROR_ITEM->number.value =
		AGENT_PLATESOLVER_PA_STATE_HA_DRIFT_ITEM->number.value =
		AGENT_PLATESOLVER_PA_STATE_DEC_DRIFT_ITEM->number.value =
		AGENT_PLATESOLVER_PA_STATE_AZ_CORRECTION_CW_ITEM->number.value =
		AGENT_PLATESOLVER_PA_STATE_ALT_CORRECTION_UP_ITEM->number.value =
		AGENT_PLATESOLVER_PA_STATE_TOTAL_ERROR_ITEM->number.value = 0;
		indigo_send_message(device, "The initial position for polar error calculation is not specified");
		return false;
	} else {
		indigo_spherical_point_t equatorial_error = {0}, horizontal_error = {0};
		double solved_altitude = 0;
		if (INDIGO_PLATESOLVER_DEVICE_PRIVATE_DATA->pa_error_at_reference2.r != 0) {
			indigo_spherical_point_t position;
			indigo_spherical_point_t position_h;
			indigo_spherical_point_t position_ref2;
			indigo_spherical_point_t position_ref2_h;

			indigo_ra_dec_to_point(
				AGENT_PLATESOLVER_WCS_RA_ITEM->number.value,
				AGENT_PLATESOLVER_WCS_DEC_ITEM->number.value,
				INDIGO_PLATESOLVER_DEVICE_PRIVATE_DATA->pa_lst_at_reference2,
				&position
			);

			position_ref2 = INDIGO_PLATESOLVER_DEVICE_PRIVATE_DATA->pa_reference2;

			indigo_log("%s(): Polar align: Residual correction (Phase II). Refraction %s", __FUNCTION__, compensate_refraction ? "ENABLED" : "DISABLED");
			indigo_log("%s(): Polar align: Use reference LST = %f", __FUNCTION__, INDIGO_PLATESOLVER_DEVICE_PRIVATE_DATA->pa_lst_at_reference2);
			indigo_log("%s(): Polar align: Solved HA = %f, Dec = %f", __FUNCTION__, position.a * RAD2DEG / 15, position.d * RAD2DEG);

			indigo_equatorial_to_hotizontal(
				&position,
				INDIGO_PLATESOLVER_DEVICE_PRIVATE_DATA->geo_coordinates.d,
				&position_h
			);
			indigo_equatorial_to_hotizontal(
				&position_ref2,
				INDIGO_PLATESOLVER_DEVICE_PRIVATE_DATA->geo_coordinates.d,
				&position_ref2_h
			);

			indigo_log("%s(): Polar align: Solved Az = %f, Alt = %f", __FUNCTION__, position_h.a * RAD2DEG, position_h.d * RAD2DEG);
			indigo_log("%s(): Polar align: Reference2 HA = %f, Dec = %f", __FUNCTION__, position_ref2.a * RAD2DEG / 15, position_ref2.d * RAD2DEG);
			indigo_log("%s(): Polar align: Reference2 Az = %f, Alt = %f", __FUNCTION__, position_ref2_h.a * RAD2DEG, position_ref2_h.d * RAD2DEG);

			if (compensate_refraction) {
				double refraction_ref2 = indigo_calculate_refraction(position_ref2_h.d);
				double refraction_pos = indigo_calculate_refraction(position_h.d);

				position_ref2_h.d -= refraction_ref2;
				position_h.d -= refraction_pos;

				indigo_log("%s(): Polar align: Refraction @Reference2 = %f', @Solved = %f', diffR = %f'", __FUNCTION__, refraction_ref2 * RAD2DEG * 60, refraction_pos * RAD2DEG * 60, (refraction_ref2 - refraction_pos) * RAD2DEG * 60);
				indigo_log("%s(): Polar align: Solved Az_r = %f, Alt_r = %f", __FUNCTION__, position_h.a * RAD2DEG, position_h.d * RAD2DEG);
				indigo_log("%s(): Polar align: Reference2 Az_r = %f, Alt_r = %f", __FUNCTION__, position_ref2_h.a * RAD2DEG, position_ref2_h.d * RAD2DEG);
			}

			equatorial_error.a = position.a - position_ref2.a;
			equatorial_error.d = position.d - position_ref2.d;
			equatorial_error.r = 1;

			horizontal_error.d = position_ref2_h.d + INDIGO_PLATESOLVER_DEVICE_PRIVATE_DATA->pa_error_at_reference2.d - position_h.d;
			horizontal_error.a = position_ref2_h.a + INDIGO_PLATESOLVER_DEVICE_PRIVATE_DATA->pa_error_at_reference2.a - position_h.a;
			horizontal_error.r = 1;

			solved_altitude = position_h.d;
		} else {
			indigo_log("%s(): Polar align: Initial correction (Phase I). Refraction %s", __FUNCTION__, compensate_refraction ? "ENABLED" : "DISABLED");
			double lst_now = indigo_lst(NULL, INDIGO_PLATESOLVER_DEVICE_PRIVATE_DATA->geo_coordinates.a * RAD2DEG);
			indigo_spherical_point_t position;
			indigo_spherical_point_t position_h;
			indigo_spherical_point_t position_observed;
			indigo_ra_dec_to_point(
				AGENT_PLATESOLVER_WCS_RA_ITEM->number.value,
				AGENT_PLATESOLVER_WCS_DEC_ITEM->number.value,
				lst_now,
				&position
			);

			indigo_ra_dec_to_point(
				INDIGO_PLATESOLVER_DEVICE_PRIVATE_DATA->eq_coordinates.a * RAD2DEG / 15,
				INDIGO_PLATESOLVER_DEVICE_PRIVATE_DATA->eq_coordinates.d * RAD2DEG,
				lst_now,
				&position_observed
			);

			indigo_equatorial_to_hotizontal(
				&position,
				INDIGO_PLATESOLVER_DEVICE_PRIVATE_DATA->geo_coordinates.d,
				&position_h
			);
			solved_altitude = position_h.d;

			indigo_log("%s(): Polar align: LST = %f, Telescope RA = %f, Dec = %f", __FUNCTION__, lst_now, INDIGO_PLATESOLVER_DEVICE_PRIVATE_DATA->eq_coordinates.a * RAD2DEG / 15, INDIGO_PLATESOLVER_DEVICE_PRIVATE_DATA->eq_coordinates.d * RAD2DEG);
			indigo_log("%s(): Polar align: Solved HA = %f, Dec = %f", __FUNCTION__, position.a * RAD2DEG / 15, position.d * RAD2DEG);
			indigo_log("%s(): Polar align: Telescope HA = %f, Dec = %f", __FUNCTION__, position_observed.a * RAD2DEG / 15, position_observed.d * RAD2DEG);

			bool success = indigo_polar_alignment_error(
				&INDIGO_PLATESOLVER_DEVICE_PRIVATE_DATA->pa_reference1,
				&position,
				&position_observed,
				INDIGO_PLATESOLVER_DEVICE_PRIVATE_DATA->geo_coordinates.d,
				compensate_refraction,
				&equatorial_error,
				&horizontal_error
			);
			if (!success) {
				indigo_send_message(device, "No solution for polar correction is found");
				return false;
			}
			INDIGO_PLATESOLVER_DEVICE_PRIVATE_DATA->pa_error_at_reference2 = horizontal_error;
			INDIGO_PLATESOLVER_DEVICE_PRIVATE_DATA->pa_lst_at_reference2 = lst_now;
			INDIGO_PLATESOLVER_DEVICE_PRIVATE_DATA->pa_reference2 = position;
		}
		AGENT_PLATESOLVER_PA_STATE_AZ_ERROR_ITEM->number.value = horizontal_error.a * RAD2DEG;
		AGENT_PLATESOLVER_PA_STATE_ALT_ERROR_ITEM->number.value = horizontal_error.d * RAD2DEG;
		AGENT_PLATESOLVER_PA_STATE_HA_DRIFT_ITEM->number.value = equatorial_error.a * RAD2DEG;
		AGENT_PLATESOLVER_PA_STATE_DEC_DRIFT_ITEM->number.value = equatorial_error.d * RAD2DEG;
		AGENT_PLATESOLVER_PA_STATE_ALT_CORRECTION_UP_ITEM->number.value = (horizontal_error.d >= 0) ? 1 : 0;
		AGENT_PLATESOLVER_PA_STATE_AZ_CORRECTION_CW_ITEM->number.value = (horizontal_error.a >= 0) ? 1 : 0;
		/* The great circle distance can be simplified as follows, because these are the errors at the given position !!! */
		AGENT_PLATESOLVER_PA_STATE_TOTAL_ERROR_ITEM->number.value = sqrt(horizontal_error.a * horizontal_error.a + horizontal_error.d * horizontal_error.d) * RAD2DEG;
		double delta_az = horizontal_error.a * RAD2DEG * 60;
		double delta_alt = horizontal_error.d * RAD2DEG * 60;
		double total_delta = AGENT_PLATESOLVER_PA_STATE_TOTAL_ERROR_ITEM->number.value * 60;

		indigo_log("%s(): Polar align: Altitude = %.2f", __FUNCTION__, solved_altitude * RAD2DEG);
		indigo_log("%s(): Polar align: Correction deltaHA = %f', deltaDec = %f'", __FUNCTION__, equatorial_error.a * RAD2DEG * 60, equatorial_error.d * RAD2DEG * 60);
		indigo_log("%s(): Polar align: Correction deltaAz = %f', deltaAlt = %f'", __FUNCTION__, delta_az, delta_alt);
		indigo_log("%s(): Polar align: Total offset = %f'", __FUNCTION__, total_delta);
		if (solved_altitude * RAD2DEG < 30) {
			indigo_send_message(device, "Calculated offset: %.2f' (Polar alignment may not be accurate at %.2f° altitide)", total_delta, solved_altitude * RAD2DEG);
		} else {
			indigo_send_message(device, "Calculated offset: %.2f'", total_delta);
		}
		indigo_send_message(device, "Azimuth correction: %+.2f', move %s (use azimuth adjustment knob)" , delta_az, (delta_az > 0) ? "C.W." : "C.C.W.");
		indigo_send_message(device, "Altitude correction: %+.2f', move %s (use altitude adjustment knob)", delta_alt, (delta_alt > 0) ? "Up" : "Down");
	}
	return true;
}

static void solve(indigo_platesolver_task *task) {
	indigo_device *device = task->device;
	double recenter_ra = AGENT_PLATESOLVER_HINTS_RA_ITEM->number.value;
	double recenter_dec = AGENT_PLATESOLVER_HINTS_DEC_ITEM->number.value;

	if (AGENT_PLATESOLVER_PA_STATE_ITEM->number.value == POLAR_ALIGN_IDLE) {
		if (AGENT_PLATESOLVER_SYNC_CALCULATE_PA_ERROR_ITEM->sw.value) {
			indigo_log("%s(): state POLAR_ALIGN_IDLE -> POLAR_ALIGN_REFERENCE_1", __FUNCTION__);
			AGENT_PLATESOLVER_PA_STATE_PROPERTY->state = INDIGO_BUSY_STATE;
			AGENT_PLATESOLVER_PA_STATE_ITEM->number.value = POLAR_ALIGN_REFERENCE_1;
			indigo_update_property(device, AGENT_PLATESOLVER_PA_STATE_PROPERTY, NULL);
		} else if (AGENT_PLATESOLVER_SYNC_RECALCULATE_PA_ERROR_ITEM->sw.value) {
			indigo_log("%s(): state POLAR_ALIGN_IDLE -> POLAR_ALIGN_RECALCULATE", __FUNCTION__);
			AGENT_PLATESOLVER_PA_STATE_PROPERTY->state = INDIGO_BUSY_STATE;
			AGENT_PLATESOLVER_PA_STATE_ITEM->number.value = POLAR_ALIGN_RECALCULATE;
			indigo_update_property(device, AGENT_PLATESOLVER_PA_STATE_PROPERTY, NULL);
		}
	}

	// Solve with a particular plate solver
	if (!INDIGO_PLATESOLVER_DEVICE_PRIVATE_DATA->solve(device, task->image, task->size))
		return;

	// Continue with a generic process
	set_fov(device, AGENT_PLATESOLVER_WCS_ANGLE_ITEM->number.value, AGENT_PLATESOLVER_WCS_WIDTH_ITEM->number.value, AGENT_PLATESOLVER_WCS_HEIGHT_ITEM->number.value);

	if (
		AGENT_PLATESOLVER_SYNC_SYNC_ITEM->sw.value ||
		AGENT_PLATESOLVER_SYNC_CENTER_ITEM->sw.value //||
		//(
			//AGENT_PLATESOLVER_SYNC_CALCULATE_PA_ERROR_ITEM->sw.value &&
			//AGENT_PLATESOLVER_PA_STATE_ITEM->number.value == POLAR_ALIGN_START
		//)
	) {
		set_pa_reference(device);
		mount_sync(device, AGENT_PLATESOLVER_WCS_RA_ITEM->number.value, AGENT_PLATESOLVER_WCS_DEC_ITEM->number.value, 2);
	}

	if (AGENT_PLATESOLVER_SYNC_CENTER_ITEM->sw.value) {
		mount_slew(device, recenter_ra, recenter_dec, 3);
	}

	if (AGENT_PLATESOLVER_SYNC_CALCULATE_PA_ERROR_ITEM->sw.value) {
		if(AGENT_PLATESOLVER_PA_STATE_ITEM->number.value == POLAR_ALIGN_REFERENCE_1) {
			indigo_log("%s(): state POLAR_ALIGN_REFERENCE_1 -> POLAR_ALIGN_REFERENCE_2", __FUNCTION__);
			AGENT_PLATESOLVER_PA_STATE_PROPERTY->state = INDIGO_BUSY_STATE;
			AGENT_PLATESOLVER_PA_STATE_ITEM->number.value = POLAR_ALIGN_REFERENCE_2;
			INDIGO_PLATESOLVER_DEVICE_PRIVATE_DATA->pa_lst_at_reference1 = indigo_lst(NULL, INDIGO_PLATESOLVER_DEVICE_PRIVATE_DATA->geo_coordinates.a * RAD2DEG);
			indigo_ra_dec_to_point(
				AGENT_PLATESOLVER_WCS_RA_ITEM->number.value,
				AGENT_PLATESOLVER_WCS_DEC_ITEM->number.value,
				INDIGO_PLATESOLVER_DEVICE_PRIVATE_DATA->pa_lst_at_reference1,
				&INDIGO_PLATESOLVER_DEVICE_PRIVATE_DATA->pa_reference1
			);
			indigo_update_property(device, AGENT_PLATESOLVER_PA_STATE_PROPERTY, NULL);
			mount_slew(
				device,
				(INDIGO_PLATESOLVER_DEVICE_PRIVATE_DATA->eq_coordinates.a * RAD2DEG - AGENT_PLATESOLVER_PA_SETTINGS_HA_MOVE_ITEM->number.value) / 15,
				INDIGO_PLATESOLVER_DEVICE_PRIVATE_DATA->pa_reference1.d * RAD2DEG,
				3
			);
			start_exposure(device, AGENT_PLATESOLVER_PA_SETTINGS_EXPOSURE_ITEM->number.value);
		} else if (AGENT_PLATESOLVER_PA_STATE_ITEM->number.value == POLAR_ALIGN_REFERENCE_2) {
			indigo_log("%s(): state POLAR_ALIGN_REFERENCE_2 -> POLAR_ALIGN_REFERENCE_3", __FUNCTION__);
			AGENT_PLATESOLVER_PA_STATE_PROPERTY->state = INDIGO_BUSY_STATE;
			AGENT_PLATESOLVER_PA_STATE_ITEM->number.value = POLAR_ALIGN_REFERENCE_3;
			INDIGO_PLATESOLVER_DEVICE_PRIVATE_DATA->pa_lst_at_reference2 = indigo_lst(NULL, INDIGO_PLATESOLVER_DEVICE_PRIVATE_DATA->geo_coordinates.a * RAD2DEG);
			indigo_ra_dec_to_point(
				AGENT_PLATESOLVER_WCS_RA_ITEM->number.value,
				AGENT_PLATESOLVER_WCS_DEC_ITEM->number.value,
				INDIGO_PLATESOLVER_DEVICE_PRIVATE_DATA->pa_lst_at_reference2,
				&INDIGO_PLATESOLVER_DEVICE_PRIVATE_DATA->pa_reference2
			);
			indigo_update_property(device, AGENT_PLATESOLVER_PA_STATE_PROPERTY, NULL);
			mount_slew(
				device,
				(INDIGO_PLATESOLVER_DEVICE_PRIVATE_DATA->eq_coordinates.a * RAD2DEG - AGENT_PLATESOLVER_PA_SETTINGS_HA_MOVE_ITEM->number.value) / 15,
				INDIGO_PLATESOLVER_DEVICE_PRIVATE_DATA->pa_reference1.d * RAD2DEG,
				3
			);
			start_exposure(device, AGENT_PLATESOLVER_PA_SETTINGS_EXPOSURE_ITEM->number.value);
		} else if (AGENT_PLATESOLVER_PA_STATE_ITEM->number.value == POLAR_ALIGN_REFERENCE_3) {
			indigo_log("%s(): state POLAR_ALIGN_REFERENCE_3 -> POLAR_ALIGN_IDLE", __FUNCTION__);
			AGENT_PLATESOLVER_PA_STATE_PROPERTY->state = INDIGO_OK_STATE;
			AGENT_PLATESOLVER_PA_STATE_ITEM->number.value = POLAR_ALIGN_IDLE;
			INDIGO_PLATESOLVER_DEVICE_PRIVATE_DATA->pa_lst_at_reference3 = indigo_lst(NULL, INDIGO_PLATESOLVER_DEVICE_PRIVATE_DATA->geo_coordinates.a * RAD2DEG);
			indigo_ra_dec_to_point(
				AGENT_PLATESOLVER_WCS_RA_ITEM->number.value,
				AGENT_PLATESOLVER_WCS_DEC_ITEM->number.value,
				INDIGO_PLATESOLVER_DEVICE_PRIVATE_DATA->pa_lst_at_reference3,
				&INDIGO_PLATESOLVER_DEVICE_PRIVATE_DATA->pa_reference3
			);
			indigo_update_property(device, AGENT_PLATESOLVER_PA_STATE_PROPERTY, NULL);

			double d2, d3, u, v;
			indigo_polar_alignment_error_3p(
				&INDIGO_PLATESOLVER_DEVICE_PRIVATE_DATA->pa_reference1,
				&INDIGO_PLATESOLVER_DEVICE_PRIVATE_DATA->pa_reference2,
				&INDIGO_PLATESOLVER_DEVICE_PRIVATE_DATA->pa_reference3,
				&d2,
				&d3,
				&u,
				&v
			);
			indigo_log("POLAR_ALIGN: d2 = %.10f d3 = %.10f u = %.10f v = %.10f", d2, d3, u * RAD2DEG * 60 , v * RAD2DEG * 60);
		}  else if (AGENT_PLATESOLVER_PA_STATE_ITEM->number.value == POLAR_ALIGN_GOTO_MERIDIAN){
			indigo_log("%s(): state POLAR_ALIGN_GOTO_MERIDIAN -> POLAR_ALIGN_IDLE", __FUNCTION__);
			AGENT_PLATESOLVER_PA_STATE_PROPERTY->state = INDIGO_OK_STATE;
			AGENT_PLATESOLVER_PA_STATE_ITEM->number.value = POLAR_ALIGN_IDLE;
			INDIGO_PLATESOLVER_DEVICE_PRIVATE_DATA->pa_lst_at_reference2 = indigo_lst(NULL, INDIGO_PLATESOLVER_DEVICE_PRIVATE_DATA->geo_coordinates.a * RAD2DEG);
			indigo_spherical_point_t position;
			indigo_ra_dec_to_point(
				AGENT_PLATESOLVER_WCS_RA_ITEM->number.value,
				AGENT_PLATESOLVER_WCS_DEC_ITEM->number.value,
				INDIGO_PLATESOLVER_DEVICE_PRIVATE_DATA->pa_lst_at_reference2,
				&INDIGO_PLATESOLVER_DEVICE_PRIVATE_DATA->pa_reference2
			);
			indigo_log(
				"%s(): Polar align: Reference at meridian LST = %f", __FUNCTION__,
				INDIGO_PLATESOLVER_DEVICE_PRIVATE_DATA->pa_lst_at_reference2
			);
			indigo_log(
				"%s(): Polar align: Reference at meridian HA = %f, Dec = %f", __FUNCTION__,
				INDIGO_PLATESOLVER_DEVICE_PRIVATE_DATA->pa_reference2.a * RAD2DEG,
				INDIGO_PLATESOLVER_DEVICE_PRIVATE_DATA->pa_reference2.d * RAD2DEG
			);
			indigo_update_property(device, AGENT_PLATESOLVER_PA_STATE_PROPERTY, NULL);
		}
	}

	if (AGENT_PLATESOLVER_SYNC_RECALCULATE_PA_ERROR_ITEM->sw.value) {
		if (calculate_pa_error(device)) {
			AGENT_PLATESOLVER_PA_STATE_PROPERTY->state = INDIGO_OK_STATE;
		} else {
			AGENT_PLATESOLVER_PA_STATE_PROPERTY->state = INDIGO_ALERT_STATE;
		}
		indigo_log("%s(): state POLAR_ALIGN_RECALCULATE -> POLAR_ALIGN_IDLE", __FUNCTION__);
		AGENT_PLATESOLVER_PA_STATE_ITEM->number.value = POLAR_ALIGN_IDLE;
		indigo_update_property(device, AGENT_PLATESOLVER_PA_STATE_PROPERTY, NULL);
	}

	AGENT_PLATESOLVER_WCS_PROPERTY->state = INDIGO_OK_STATE;
	indigo_update_property(device, AGENT_PLATESOLVER_WCS_PROPERTY, NULL);
}

indigo_result indigo_platesolver_device_attach(indigo_device *device, const char* driver_name, unsigned version, indigo_device_interface device_interface) {
	assert(device != NULL);
	if (indigo_filter_device_attach(device, driver_name, version, device_interface) == INDIGO_OK) {
		// -------------------------------------------------------------------------------- FILTER_RELATED_AGENT_LIST
		FILTER_RELATED_AGENT_LIST_PROPERTY->hidden = false;
		FILTER_DEVICE_CONTEXT->validate_related_agent = validate_related_agent;
		// -------------------------------------------------------------------------------- AGENT_PLATESOLVER_USE_INDEX
		AGENT_PLATESOLVER_USE_INDEX_PROPERTY = indigo_init_switch_property(NULL, device->name, AGENT_PLATESOLVER_USE_INDEX_PROPERTY_NAME, PLATESOLVER_MAIN_GROUP, "Use indexes", INDIGO_OK_STATE, INDIGO_RW_PERM, INDIGO_ANY_OF_MANY_RULE, 33);
		if (AGENT_PLATESOLVER_USE_INDEX_PROPERTY == NULL)
			return INDIGO_FAILED;
		AGENT_PLATESOLVER_USE_INDEX_PROPERTY->count = 0;
		// -------------------------------------------------------------------------------- Hints property
		AGENT_PLATESOLVER_HINTS_PROPERTY = indigo_init_number_property(NULL, device->name, AGENT_PLATESOLVER_HINTS_PROPERTY_NAME, PLATESOLVER_MAIN_GROUP, "Hints", INDIGO_OK_STATE, INDIGO_RW_PERM, 8);
		if (AGENT_PLATESOLVER_HINTS_PROPERTY == NULL)
			return INDIGO_FAILED;
		indigo_init_number_item(AGENT_PLATESOLVER_HINTS_RADIUS_ITEM, AGENT_PLATESOLVER_HINTS_RADIUS_ITEM_NAME, "Search radius (°)", 0, 360, 2, 0);
		indigo_init_number_item(AGENT_PLATESOLVER_HINTS_RA_ITEM, AGENT_PLATESOLVER_HINTS_RA_ITEM_NAME, "RA (hours)", 0, 24, 0, 0);
		indigo_init_number_item(AGENT_PLATESOLVER_HINTS_DEC_ITEM, AGENT_PLATESOLVER_HINTS_DEC_ITEM_NAME, "Dec (°)", -90, 90, 0, 0);
		indigo_init_number_item(AGENT_PLATESOLVER_HINTS_SCALE_ITEM, AGENT_PLATESOLVER_HINTS_SCALE_ITEM_NAME, "Pixel scale (°/pixel)", 0, 1000, 0, 0);
		indigo_init_number_item(AGENT_PLATESOLVER_HINTS_PARITY_ITEM, AGENT_PLATESOLVER_HINTS_PARITY_ITEM_NAME, "Parity (-1,0,1)", -1, 1, 1, 0);
		indigo_init_number_item(AGENT_PLATESOLVER_HINTS_DOWNSAMPLE_ITEM, AGENT_PLATESOLVER_HINTS_DOWNSAMPLE_ITEM_NAME, "Downsample", 1, 16, 1, 2);
		indigo_init_number_item(AGENT_PLATESOLVER_HINTS_DEPTH_ITEM, AGENT_PLATESOLVER_HINTS_DEPTH_ITEM_NAME, "Depth", 0, 1000, 5, 0);
		indigo_init_number_item(AGENT_PLATESOLVER_HINTS_CPU_LIMIT_ITEM, AGENT_PLATESOLVER_HINTS_CPU_LIMIT_ITEM_NAME, "CPU Limit (seconds)", 0, 600, 10, 180);
		strcpy(AGENT_PLATESOLVER_HINTS_RADIUS_ITEM->number.format, "%m");
		strcpy(AGENT_PLATESOLVER_HINTS_RA_ITEM->number.format, "%m");
		strcpy(AGENT_PLATESOLVER_HINTS_DEC_ITEM->number.format, "%m");
		strcpy(AGENT_PLATESOLVER_HINTS_SCALE_ITEM->number.format, "%m");
		// -------------------------------------------------------------------------------- WCS property
		AGENT_PLATESOLVER_WCS_PROPERTY = indigo_init_number_property(NULL, device->name, AGENT_PLATESOLVER_WCS_PROPERTY_NAME, PLATESOLVER_MAIN_GROUP, "WCS solution", INDIGO_OK_STATE, INDIGO_RO_PERM, 8);
		if (AGENT_PLATESOLVER_WCS_PROPERTY == NULL)
			return INDIGO_FAILED;
		indigo_init_number_item(AGENT_PLATESOLVER_WCS_RA_ITEM, AGENT_PLATESOLVER_WCS_RA_ITEM_NAME, "Frame center RA (hours)", 0, 24, 0, 0);
		indigo_init_number_item(AGENT_PLATESOLVER_WCS_DEC_ITEM, AGENT_PLATESOLVER_WCS_DEC_ITEM_NAME, "Frame center Dec (°)", 0, 360, 0, 0);
		indigo_init_number_item(AGENT_PLATESOLVER_WCS_ANGLE_ITEM, AGENT_PLATESOLVER_WCS_ANGLE_ITEM_NAME, "Rotation angle (° E of N)", 0, 360, 0, 0);
		indigo_init_number_item(AGENT_PLATESOLVER_WCS_WIDTH_ITEM, AGENT_PLATESOLVER_WCS_WIDTH_ITEM_NAME, "Frame width (°)", 0, 360, 0, 0);
		indigo_init_number_item(AGENT_PLATESOLVER_WCS_HEIGHT_ITEM, AGENT_PLATESOLVER_WCS_HEIGHT_ITEM_NAME, "Frame height (°)", 0, 360, 0, 0);
		indigo_init_number_item(AGENT_PLATESOLVER_WCS_SCALE_ITEM, AGENT_PLATESOLVER_WCS_SCALE_ITEM_NAME, "Pixel scale (°/pixel)", 0, 1000, 0, 0);
		indigo_init_number_item(AGENT_PLATESOLVER_WCS_PARITY_ITEM, AGENT_PLATESOLVER_WCS_PARITY_ITEM_NAME, "Parity (-1,1)", -1, 1, 0, 0);
		indigo_init_number_item(AGENT_PLATESOLVER_WCS_INDEX_ITEM, AGENT_PLATESOLVER_WCS_INDEX_ITEM_NAME, "Used index file", 0, 10000, 0, 0);
		strcpy(AGENT_PLATESOLVER_WCS_RA_ITEM->number.format, "%m");
		strcpy(AGENT_PLATESOLVER_WCS_DEC_ITEM->number.format, "%m");
		strcpy(AGENT_PLATESOLVER_WCS_ANGLE_ITEM->number.format, "%m");
		strcpy(AGENT_PLATESOLVER_WCS_WIDTH_ITEM->number.format, "%m");
		strcpy(AGENT_PLATESOLVER_WCS_HEIGHT_ITEM->number.format, "%m");
		strcpy(AGENT_PLATESOLVER_WCS_SCALE_ITEM->number.format, "%m");
		// -------------------------------------------------------------------------------- SYNC property
		AGENT_PLATESOLVER_SYNC_PROPERTY = indigo_init_switch_property(NULL, device->name, AGENT_PLATESOLVER_SYNC_PROPERTY_NAME, PLATESOLVER_MAIN_GROUP, "Sync mode", INDIGO_OK_STATE, INDIGO_RW_PERM, INDIGO_ONE_OF_MANY_RULE, 5);
		if (AGENT_PLATESOLVER_SYNC_PROPERTY == NULL)
			return INDIGO_FAILED;
		indigo_init_switch_item(AGENT_PLATESOLVER_SYNC_DISABLED_ITEM, AGENT_PLATESOLVER_SYNC_DISABLED_ITEM_NAME, "Disabled", true);
		indigo_init_switch_item(AGENT_PLATESOLVER_SYNC_SYNC_ITEM, AGENT_PLATESOLVER_SYNC_SYNC_ITEM_NAME, "Sync only", false);
		indigo_init_switch_item(AGENT_PLATESOLVER_SYNC_CENTER_ITEM, AGENT_PLATESOLVER_SYNC_CENTER_ITEM_NAME, "Sync and center", false);
		indigo_init_switch_item(AGENT_PLATESOLVER_SYNC_CALCULATE_PA_ERROR_ITEM, AGENT_PLATESOLVER_SYNC_CALCULATE_PA_ERROR_ITEM_NAME, "Calclulate polar alignment error", false);
		indigo_init_switch_item(AGENT_PLATESOLVER_SYNC_RECALCULATE_PA_ERROR_ITEM, AGENT_PLATESOLVER_SYNC_RECALCULATE_PA_ERROR_ITEM_NAME, "Recalclulate polar alignment error", false);
#ifdef HIDE_POLAR_ALIGN
		AGENT_PLATESOLVER_SYNC_PROPERTY->count = 3;
#endif
		// -------------------------------------------------------------------------------- POLAR_ALIGNMENT_SETTINGS property
		AGENT_PLATESOLVER_PA_SETTINGS_PROPERTY = indigo_init_number_property(NULL, device->name, AGENT_PLATESOLVER_PA_SETTINGS_PROPERTY_NAME, PLATESOLVER_MAIN_GROUP, "Polar alignment settings", INDIGO_OK_STATE, INDIGO_RW_PERM, 4);
		if (AGENT_PLATESOLVER_PA_SETTINGS_PROPERTY == NULL)
			return INDIGO_FAILED;
		indigo_init_number_item(AGENT_PLATESOLVER_PA_SETTINGS_EXPOSURE_ITEM, AGENT_PLATESOLVER_PA_SETTINGS_EXPOSURE_ITEM_NAME, "Exposure time (s)", 0, 60, 1, 1);
		indigo_init_number_item(AGENT_PLATESOLVER_PA_SETTINGS_HA_MOVE_ITEM, AGENT_PLATESOLVER_PA_SETTINGS_HA_MOVE_ITEM_NAME, "Hour angle move (°)", -180, 180, 5, 30);
		indigo_init_number_item(AGENT_PLATESOLVER_PA_SETTINGS_DEC_MOVE_ITEM, AGENT_PLATESOLVER_PA_SETTINGS_DEC_MOVE_ITEM_NAME, "Declination move (°)", -45, 45, 1, 0);
		indigo_init_number_item(AGENT_PLATESOLVER_PA_SETTINGS_COMPENSATE_REFRACTION_ITEM, AGENT_PLATESOLVER_PA_SETTINGS_COMPENSATE_REFRACTION_ITEM_NAME, "Compensate refraction (1=On/0=Off)", 0, 1, 0, 0);
		strcpy(AGENT_PLATESOLVER_PA_SETTINGS_HA_MOVE_ITEM->number.format, "%m");
		strcpy(AGENT_PLATESOLVER_PA_SETTINGS_DEC_MOVE_ITEM->number.format, "%m");
		strcpy(AGENT_PLATESOLVER_PA_SETTINGS_COMPENSATE_REFRACTION_ITEM->number.format, "%.0f");
#ifdef HIDE_POLAR_ALIGN
		AGENT_PLATESOLVER_PA_SETTINGS_PROPERTY->hidden = true;
#endif
		// -------------------------------------------------------------------------------- POLAR_ALIGNMENT_ERROR property
		AGENT_PLATESOLVER_PA_STATE_PROPERTY = indigo_init_number_property(NULL, device->name, AGENT_PLATESOLVER_PA_STATE_PROPERTY_NAME, PLATESOLVER_MAIN_GROUP, "Polar alignment state", INDIGO_IDLE_STATE, INDIGO_RO_PERM, 8);
		if (AGENT_PLATESOLVER_PA_STATE_PROPERTY == NULL)
			return INDIGO_FAILED;
		indigo_init_number_item(AGENT_PLATESOLVER_PA_STATE_ITEM, AGENT_PLATESOLVER_PA_STATE_ITEM_NAME, "Polar alignment state", 0, 10, 0, 0);
		indigo_init_number_item(AGENT_PLATESOLVER_PA_STATE_HA_DRIFT_ITEM, AGENT_PLATESOLVER_PA_STATE_HA_DRIFT_ITEM_NAME, "Hour angle drift (°)", -45, 45, 0, 0);
		indigo_init_number_item(AGENT_PLATESOLVER_PA_STATE_DEC_DRIFT_ITEM, AGENT_PLATESOLVER_PA_STATE_DEC_DRIFT_ITEM_NAME, "Declination drift(°)", -45, 45, 0, 0);
		indigo_init_number_item(AGENT_PLATESOLVER_PA_STATE_ALT_ERROR_ITEM, AGENT_PLATESOLVER_PA_STATE_ALT_ERROR_ITEM_NAME, "Altitude error (°)", -45, 45, 0, 0);
		indigo_init_number_item(AGENT_PLATESOLVER_PA_STATE_AZ_ERROR_ITEM, AGENT_PLATESOLVER_PA_STATE_AZ_ERROR_ITEM_NAME, "Azimuth error (°)", -45, 45, 0, 0);
		indigo_init_number_item(AGENT_PLATESOLVER_PA_STATE_ALT_CORRECTION_UP_ITEM, AGENT_PLATESOLVER_PA_STATE_ALT_CORRECTION_UP_ITEM_NAME, "Altitude correction (1=Up/0=Down)", 0, 1, 0, 0);
		indigo_init_number_item(AGENT_PLATESOLVER_PA_STATE_AZ_CORRECTION_CW_ITEM, AGENT_PLATESOLVER_PA_STATE_AZ_CORRECTION_CW_ITEM_NAME, "Azimuth correction (1=C.W./0=C.C.W.)", 0, 1, 0, 0);
		indigo_init_number_item(AGENT_PLATESOLVER_PA_STATE_TOTAL_ERROR_ITEM, AGENT_PLATESOLVER_PA_STATE_TOTAL_ERROR_ITEM_NAME, "Total error (°)", -45, 45, 0, 0);
		strcpy(AGENT_PLATESOLVER_PA_STATE_ITEM->number.format, "%.0f");
		strcpy(AGENT_PLATESOLVER_PA_STATE_HA_DRIFT_ITEM->number.format, "%m");
		strcpy(AGENT_PLATESOLVER_PA_STATE_DEC_DRIFT_ITEM->number.format, "%m");
		strcpy(AGENT_PLATESOLVER_PA_STATE_ALT_ERROR_ITEM->number.format, "%m");
		strcpy(AGENT_PLATESOLVER_PA_STATE_AZ_ERROR_ITEM->number.format, "%m");
		strcpy(AGENT_PLATESOLVER_PA_STATE_ALT_CORRECTION_UP_ITEM->number.format, "%.0f");
		strcpy(AGENT_PLATESOLVER_PA_STATE_AZ_CORRECTION_CW_ITEM->number.format, "%.0f");
		strcpy(AGENT_PLATESOLVER_PA_STATE_TOTAL_ERROR_ITEM->number.format, "%m");
#ifdef HIDE_POLAR_ALIGN
		AGENT_PLATESOLVER_PA_STATE_PROPERTY->hidden = true;
#endif
		// -------------------------------------------------------------------------------- ABORT property
		AGENT_PLATESOLVER_ABORT_PROPERTY = indigo_init_switch_property(NULL, device->name, AGENT_PLATESOLVER_ABORT_PROPERTY_NAME, PLATESOLVER_MAIN_GROUP, "Abort", INDIGO_OK_STATE, INDIGO_RW_PERM, INDIGO_ANY_OF_MANY_RULE, 1);
		if (AGENT_PLATESOLVER_ABORT_PROPERTY == NULL)
			return INDIGO_FAILED;
		indigo_init_switch_item(AGENT_PLATESOLVER_ABORT_ITEM, AGENT_PLATESOLVER_ABORT_ITEM_NAME, "Abort", false);
		// -------------------------------------------------------------------------------- IMAGE property
		AGENT_PLATESOLVER_IMAGE_PROPERTY = indigo_init_blob_property_p(NULL, device->name, AGENT_PLATESOLVER_IMAGE_PROPERTY_NAME, PLATESOLVER_MAIN_GROUP, "Image", INDIGO_OK_STATE, INDIGO_WO_PERM, 1);
		if (AGENT_PLATESOLVER_IMAGE_PROPERTY == NULL)
			return INDIGO_FAILED;
		indigo_init_blob_item(AGENT_PLATESOLVER_IMAGE_ITEM, AGENT_PLATESOLVER_IMAGE_ITEM_NAME, "Image");
		// --------------------------------------------------------------------------------
		CONFIG_PROPERTY->hidden = true;
		PROFILE_PROPERTY->hidden = true;
		CONNECTION_PROPERTY->hidden = true;
		// --------------------------------------------------------------------------------
		INDIGO_PLATESOLVER_DEVICE_PRIVATE_DATA->on_coordinates_set_state = INDIGO_IDLE_STATE;
		INDIGO_PLATESOLVER_DEVICE_PRIVATE_DATA->eq_coordinates_state = INDIGO_IDLE_STATE;

		pthread_mutex_init(&INDIGO_PLATESOLVER_DEVICE_PRIVATE_DATA->mutex, NULL);
		return INDIGO_OK;
	}
	return INDIGO_FAILED;
}

indigo_result indigo_platesolver_enumerate_properties(indigo_device *device, indigo_client *client, indigo_property *property) {
	if (indigo_property_match(AGENT_PLATESOLVER_USE_INDEX_PROPERTY, property))
		indigo_define_property(device, AGENT_PLATESOLVER_USE_INDEX_PROPERTY, NULL);
	if (indigo_property_match(AGENT_PLATESOLVER_HINTS_PROPERTY, property))
		indigo_define_property(device, AGENT_PLATESOLVER_HINTS_PROPERTY, NULL);
	if (indigo_property_match(AGENT_PLATESOLVER_WCS_PROPERTY, property))
		indigo_define_property(device, AGENT_PLATESOLVER_WCS_PROPERTY, NULL);
	if (indigo_property_match(AGENT_PLATESOLVER_SYNC_PROPERTY, property))
		indigo_define_property(device, AGENT_PLATESOLVER_SYNC_PROPERTY, NULL);
	if (indigo_property_match(AGENT_PLATESOLVER_PA_SETTINGS_PROPERTY, property))
		indigo_define_property(device, AGENT_PLATESOLVER_PA_SETTINGS_PROPERTY, NULL);
	if (indigo_property_match(AGENT_PLATESOLVER_PA_STATE_PROPERTY, property))
		indigo_define_property(device, AGENT_PLATESOLVER_PA_STATE_PROPERTY, NULL);
	if (indigo_property_match(AGENT_PLATESOLVER_ABORT_PROPERTY, property))
		indigo_define_property(device, AGENT_PLATESOLVER_ABORT_PROPERTY, NULL);
	if (indigo_property_match(AGENT_PLATESOLVER_IMAGE_PROPERTY, property))
		indigo_define_property(device, AGENT_PLATESOLVER_IMAGE_PROPERTY, NULL);
	return indigo_filter_enumerate_properties(device, client, property);
}

indigo_result indigo_platesolver_change_property(indigo_device *device, indigo_client *client, indigo_property *property) {
	assert(device != NULL);
	assert(DEVICE_CONTEXT != NULL);
	assert(property != NULL);
	if (client == FILTER_DEVICE_CONTEXT->client)
		return INDIGO_OK;
	if (indigo_property_match(AGENT_PLATESOLVER_USE_INDEX_PROPERTY, property)) {
	// -------------------------------------------------------------------------------- AGENT_PLATESOLVER_USE_INDEX
		indigo_property_copy_values(AGENT_PLATESOLVER_USE_INDEX_PROPERTY, property, false);
		AGENT_PLATESOLVER_USE_INDEX_PROPERTY->state = INDIGO_OK_STATE;
		indigo_update_property(device, AGENT_PLATESOLVER_USE_INDEX_PROPERTY, NULL);
		INDIGO_PLATESOLVER_DEVICE_PRIVATE_DATA->save_config(device);
		return INDIGO_OK;
	} else if (indigo_property_match(AGENT_PLATESOLVER_HINTS_PROPERTY, property)) {
	// -------------------------------------------------------------------------------- AGENT_PLATESOLVER_HINTS
		indigo_property_copy_values(AGENT_PLATESOLVER_HINTS_PROPERTY, property, false);
		AGENT_PLATESOLVER_HINTS_PROPERTY->state = INDIGO_OK_STATE;
		indigo_update_property(device, AGENT_PLATESOLVER_HINTS_PROPERTY, NULL);
		INDIGO_PLATESOLVER_DEVICE_PRIVATE_DATA->save_config(device);
		return INDIGO_OK;
	} else if (indigo_property_match(AGENT_PLATESOLVER_PA_SETTINGS_PROPERTY, property)) {
	// -------------------------------------------------------------------------------- AGENT_PLATESOLVER_PA_SETTINGS
		indigo_property_copy_values(AGENT_PLATESOLVER_PA_SETTINGS_PROPERTY, property, false);
		AGENT_PLATESOLVER_PA_SETTINGS_PROPERTY->state = INDIGO_OK_STATE;
		indigo_update_property(device, AGENT_PLATESOLVER_PA_SETTINGS_PROPERTY, NULL);
		INDIGO_PLATESOLVER_DEVICE_PRIVATE_DATA->save_config(device);
		return INDIGO_OK;
	} else if (indigo_property_match(AGENT_PLATESOLVER_SYNC_PROPERTY, property)) {
	// -------------------------------------------------------------------------------- AGENT_PLATESOLVER_SYNC
		indigo_property_copy_values(AGENT_PLATESOLVER_SYNC_PROPERTY, property, false);
		AGENT_PLATESOLVER_SYNC_PROPERTY->state = INDIGO_OK_STATE;
		indigo_update_property(device, AGENT_PLATESOLVER_SYNC_PROPERTY, NULL);
		INDIGO_PLATESOLVER_DEVICE_PRIVATE_DATA->save_config(device);
		return INDIGO_OK;
	} else if (indigo_property_match(AGENT_PLATESOLVER_IMAGE_PROPERTY, property)) {
	// -------------------------------------------------------------------------------- AGENT_PLATESOLVER_IMAGE
		indigo_property_copy_values(AGENT_PLATESOLVER_IMAGE_PROPERTY, property, false);
		if (AGENT_PLATESOLVER_IMAGE_ITEM->blob.size > 0 && AGENT_PLATESOLVER_IMAGE_ITEM->blob.value) {
			indigo_platesolver_task *task = indigo_safe_malloc(sizeof(indigo_platesolver_task));
			task->device = device;
			task->image = indigo_safe_malloc_copy(task->size = AGENT_PLATESOLVER_IMAGE_ITEM->blob.size, AGENT_PLATESOLVER_IMAGE_ITEM->blob.value);
			indigo_async((void *(*)(void *))solve, task);
			AGENT_PLATESOLVER_IMAGE_PROPERTY->state = INDIGO_OK_STATE;
		} else {
			AGENT_PLATESOLVER_IMAGE_PROPERTY->state = INDIGO_ALERT_STATE;
		}
		indigo_update_property(device, AGENT_PLATESOLVER_IMAGE_PROPERTY, NULL);
		return INDIGO_OK;
	}
	return indigo_filter_change_property(device, client, property);
}

indigo_result indigo_platesolver_device_detach(indigo_device *device) {
	assert(device != NULL);
	indigo_release_property(AGENT_PLATESOLVER_USE_INDEX_PROPERTY);
	indigo_release_property(AGENT_PLATESOLVER_HINTS_PROPERTY);
	indigo_release_property(AGENT_PLATESOLVER_WCS_PROPERTY);
	indigo_release_property(AGENT_PLATESOLVER_SYNC_PROPERTY);
	indigo_release_property(AGENT_PLATESOLVER_PA_SETTINGS_PROPERTY);
	indigo_release_property(AGENT_PLATESOLVER_PA_STATE_PROPERTY);
	indigo_release_property(AGENT_PLATESOLVER_ABORT_PROPERTY);
	indigo_release_property(AGENT_PLATESOLVER_IMAGE_PROPERTY);
	pthread_mutex_destroy(&INDIGO_PLATESOLVER_DEVICE_PRIVATE_DATA->mutex);
	return indigo_filter_device_detach(device);
}

// -------------------------------------------------------------------------------- INDIGO agent client implementation

static void indigo_platesolver_handle_property(indigo_client *client, indigo_device *device, indigo_property *property, const char *message) {
	{ char *device_name = property->device;
		indigo_device *device = FILTER_CLIENT_CONTEXT->device;
		if (!strcmp(property->name, MOUNT_EQUATORIAL_COORDINATES_PROPERTY_NAME)) {
			indigo_property *agents = FILTER_CLIENT_CONTEXT->filter_related_agent_list_property;
			for (int j = 0; j < agents->count; j++) {
				indigo_item *item = agents->items + j;
				if (item->sw.value && !strcmp(item->name, device_name)) {
					bool update = false;
					double ra = NAN, dec = NAN;
					INDIGO_PLATESOLVER_CLIENT_PRIVATE_DATA->eq_coordinates_state = property->state;
					if (property->state == INDIGO_OK_STATE) {
						for (int i = 0; i < property->count; i++) {
							indigo_item *item = property->items + i;
							if (!strcmp(item->name, MOUNT_EQUATORIAL_COORDINATES_RA_ITEM_NAME)) {
								ra = item->number.value;
								if (AGENT_PLATESOLVER_HINTS_RA_ITEM->number.value != ra) {
									AGENT_PLATESOLVER_HINTS_RA_ITEM->number.value = AGENT_PLATESOLVER_HINTS_RA_ITEM->number.target = ra;
									INDIGO_PLATESOLVER_CLIENT_PRIVATE_DATA->eq_coordinates.a = 15 * DEG2RAD * ra;
									update = true;
								}
							} else if (!strcmp(item->name, MOUNT_EQUATORIAL_COORDINATES_DEC_ITEM_NAME)) {
								dec = item->number.value;
								if (AGENT_PLATESOLVER_HINTS_DEC_ITEM->number.value != dec) {
									AGENT_PLATESOLVER_HINTS_DEC_ITEM->number.value = AGENT_PLATESOLVER_HINTS_DEC_ITEM->number.target = dec;
									INDIGO_PLATESOLVER_CLIENT_PRIVATE_DATA->eq_coordinates.d = DEG2RAD * dec;
									update = true;
								}
							}
						}
						//indigo_debug("'%s'.'MOUNT_EQUATORIAL_COORDINATES' state %s, RA=%g, DEC=%g", device_name, indigo_property_state_text[property->state], ra, dec);
						if (update) {
							AGENT_PLATESOLVER_HINTS_PROPERTY->state = INDIGO_OK_STATE;
							indigo_update_property(device, AGENT_PLATESOLVER_HINTS_PROPERTY, NULL);
						}
					}
					break;
				}
			}
		} else if (!strcmp(property->name, MOUNT_GEOGRAPHIC_COORDINATES_PROPERTY_NAME)) {
			indigo_property *agents = FILTER_CLIENT_CONTEXT->filter_related_agent_list_property;
			for (int j = 0; j < agents->count; j++) {
				indigo_item *item = agents->items + j;
				if (item->sw.value && !strcmp(item->name, device_name)) {
					INDIGO_PLATESOLVER_CLIENT_PRIVATE_DATA->geo_coordinates_state = property->state;
					INDIGO_PLATESOLVER_CLIENT_PRIVATE_DATA->geo_coordinates.r = 1;
					double lat = NAN, lon = NAN;
					for (int i = 0; i < property->count; i++) {
						indigo_item *item = property->items + i;
						if (!strcmp(item->name, MOUNT_GEOGRAPHIC_COORDINATES_LATITUDE_ITEM_NAME)) {
							lat = item->number.value;
							INDIGO_PLATESOLVER_CLIENT_PRIVATE_DATA->geo_coordinates.d = DEG2RAD * lat;
						} else if (!strcmp(item->name, MOUNT_GEOGRAPHIC_COORDINATES_LONGITUDE_ITEM_NAME)) {
							lon = item->number.value;
							INDIGO_PLATESOLVER_CLIENT_PRIVATE_DATA->geo_coordinates.a = DEG2RAD * lon;
						}
					}
					//indigo_debug("'%s'.'MOUNT_GEOGRAPHIC_COORDINATES' state %s, LAT=%g, LONG=%g", device_name, indigo_property_state_text[property->state], lat, lon);
					break;
				}
			}
		} else if (!strcmp(property->name, MOUNT_ON_COORDINATES_SET_PROPERTY_NAME)) {
			indigo_property *agents = FILTER_CLIENT_CONTEXT->filter_related_agent_list_property;
			for (int j = 0; j < agents->count; j++) {
				indigo_item *item = agents->items + j;
				if (item->sw.value && !strcmp(item->name, device_name)) {
					INDIGO_PLATESOLVER_CLIENT_PRIVATE_DATA->on_coordinates_set_state = property->state;
					//indigo_debug("'%s'.'MOUNT_ON_COORDINATES_SET' state %s", device_name, indigo_property_state_text[property->state]);
					break;
				}
			}
		}
	}
}

indigo_result indigo_platesolver_update_property(indigo_client *client, indigo_device *device, indigo_property *property, const char *message) {
	{ char *device_name = property->device;
		if (property->state == INDIGO_OK_STATE && !strcmp(property->name, CCD_IMAGE_PROPERTY_NAME)) {
			indigo_property *related_agents = FILTER_CLIENT_CONTEXT->filter_related_agent_list_property;
			for (int j = 0; j < related_agents->count; j++) {
				indigo_item *item = related_agents->items + j;
				if (item->sw.value && !strcmp(item->name, device_name)) {
					for (int i = 0; i < property->count; i++) {
						indigo_item *item = property->items + i;
						if (!strcmp(item->name, CCD_IMAGE_ITEM_NAME)) {
							indigo_platesolver_task *task = indigo_safe_malloc(sizeof(indigo_platesolver_task));
							task->device = FILTER_CLIENT_CONTEXT->device;
							task->image = indigo_safe_malloc_copy(task->size = item->blob.size, item->blob.value);
							indigo_async((void *(*)(void *))solve, task);
						}
					}
					break;
				}
			}
		}
	}
	indigo_platesolver_handle_property(client, device, property, message);
	return indigo_filter_update_property(client, device, property, message);
}

indigo_result indigo_platesolver_define_property(indigo_client *client, indigo_device *device, indigo_property *property, const char *message) {
	indigo_platesolver_handle_property(client, device, property, message);
	return indigo_filter_define_property(client, device, property, message);
}
