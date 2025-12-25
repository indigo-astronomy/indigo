// Copyright (c) 2021-2025 CloudMakers, s. r. o.
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
#include <assert.h>
#include <string.h>
#include <errno.h>
#include <time.h>
#include <math.h>
#include <sys/stat.h>

#include <indigo/indigo_agent.h>
#include <indigo/indigo_filter.h>
#include <indigo/indigo_align.h>
#include <indigo/indigo_platesolver.h>

// TODO: Remove obsoleted AGENT_PLATESOLVER_SYNC and AGENT_PLATESOLVER_ABORT properties in future

// -------------------------------------------------------------------------------- INDIGO agent device implementation

static bool validate_related_agent(indigo_device *device, indigo_property *info_property, int mask) {
	if (!strncmp(info_property->device, "Imager Agent", 12))
		return true;
	if (!strncmp(info_property->device, "Guider Agent", 12))
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
		indigo_save_property(device, NULL, AGENT_PLATESOLVER_MOUNT_SETTLE_TIME_PROPERTY);
		indigo_save_property(device, NULL, AGENT_PLATESOLVER_HINTS_PROPERTY);
		indigo_save_property(device, NULL, AGENT_PLATESOLVER_SYNC_PROPERTY);
		indigo_save_property(device, NULL, AGENT_PLATESOLVER_PA_SETTINGS_PROPERTY);
		indigo_save_property(device, NULL, AGENT_PLATESOLVER_EXPOSURE_SETTINGS_PROPERTY);
		if (DEVICE_CONTEXT->property_save_file_handle != NULL) {
			CONFIG_PROPERTY->state = INDIGO_OK_STATE;
			indigo_uni_close(&DEVICE_CONTEXT->property_save_file_handle);
		} else {
			CONFIG_PROPERTY->state = INDIGO_ALERT_STATE;
		}
		CONFIG_SAVE_ITEM->sw.value = false;
		indigo_update_property(device, CONFIG_PROPERTY, NULL);
		pthread_mutex_unlock(&INDIGO_PLATESOLVER_DEVICE_PRIVATE_DATA->mutex);
	}
}

static bool set_fov(indigo_device *device, double angle, double width, double height) {
	char *related_agent_name = indigo_filter_first_related_agent(FILTER_DEVICE_CONTEXT->device, "Mount Agent");
	if (related_agent_name != NULL) {
		const char *item_names[] = { AGENT_MOUNT_FOV_ANGLE_ITEM_NAME, AGENT_MOUNT_FOV_WIDTH_ITEM_NAME, AGENT_MOUNT_FOV_HEIGHT_ITEM_NAME };
		double item_values[] = { angle, width, height };
		indigo_change_number_property(FILTER_DEVICE_CONTEXT->client, related_agent_name, AGENT_MOUNT_FOV_PROPERTY_NAME, 3, item_names, item_values);
		return true;
	}
	return false;
}

static bool abort_mount_move(indigo_device *device) {
	char *related_agent_name = indigo_filter_first_related_agent(FILTER_DEVICE_CONTEXT->device, "Mount Agent");
	if (related_agent_name != NULL) {
		indigo_change_switch_property_1(FILTER_DEVICE_CONTEXT->client, related_agent_name, MOUNT_ABORT_MOTION_PROPERTY_NAME, MOUNT_ABORT_MOTION_ITEM_NAME, true);
		return true;
	}
	return false;
}

static bool abort_exposure(indigo_device *device) {
	char *related_agent_name = indigo_filter_first_related_agent_2(FILTER_DEVICE_CONTEXT->device, "Imager Agent", "Guider Agent");
	if (related_agent_name != NULL) {
		indigo_change_switch_property_1(FILTER_DEVICE_CONTEXT->client, related_agent_name, CCD_ABORT_EXPOSURE_PROPERTY_NAME, CCD_ABORT_EXPOSURE_ITEM_NAME, true);
		return true;
	}
	return false;
}

#define mount_sync(device, ra, dec, settle_time) mount_control(device, AGENT_MOUNT_START_SYNC_ITEM_NAME, ra, dec, settle_time)
#define mount_slew(device, ra, dec, settle_time) mount_control(device, AGENT_MOUNT_START_SLEW_ITEM_NAME, ra, dec, settle_time)

static bool mount_control(indigo_device *device, char *operation, double ra, double dec, double settle_time) {
	ra = fmod(ra + 24, 24.0);
	char *related_agent_name = indigo_filter_first_related_agent(FILTER_DEVICE_CONTEXT->device, "Mount Agent");
	if (related_agent_name != NULL) {
		if (INDIGO_PLATESOLVER_DEVICE_PRIVATE_DATA->mount_process_state == INDIGO_BUSY_STATE) {
			indigo_error("Mount Agent is busy");
			return false;
		}
		const char *item_names[] = { AGENT_MOUNT_TARGET_COORDINATES_RA_ITEM_NAME, AGENT_MOUNT_TARGET_COORDINATES_DEC_ITEM_NAME };
		double item_values[] = { ra, dec };
		indigo_change_number_property(FILTER_DEVICE_CONTEXT->client, related_agent_name, AGENT_MOUNT_TARGET_COORDINATES_PROPERTY_NAME, 2, item_names, item_values);
		INDIGO_PLATESOLVER_DEVICE_PRIVATE_DATA->mount_process_state = INDIGO_IDLE_STATE;
		indigo_change_switch_property_1(FILTER_DEVICE_CONTEXT->client, related_agent_name, AGENT_START_PROCESS_PROPERTY_NAME, operation, true);
		indigo_debug("'%s'.'TARGET_COORDINATES' requested RA=%g, DEC=%g", related_agent_name, ra, dec);
		for (int i = 0; i < 100; i++) { // wait 1s to become BUSY or ALERT
			if (INDIGO_PLATESOLVER_DEVICE_PRIVATE_DATA->abort_process_requested) {
				INDIGO_PLATESOLVER_DEVICE_PRIVATE_DATA->abort_process_requested = false;
				abort_mount_move(device);
				return false;
			}
			if (INDIGO_PLATESOLVER_DEVICE_PRIVATE_DATA->mount_process_state == INDIGO_BUSY_STATE || INDIGO_PLATESOLVER_DEVICE_PRIVATE_DATA->mount_process_state == INDIGO_ALERT_STATE) {
				break;
			}
			indigo_usleep(10000);
		}
		if (INDIGO_PLATESOLVER_DEVICE_PRIVATE_DATA->mount_process_state != INDIGO_BUSY_STATE) {
			indigo_error("Mount Agent didn't become busy in 1s");
			return false;
		}
		for (int i = 0; i < 6000; i++) { // wait 60s to become not BUSY
			if (INDIGO_PLATESOLVER_DEVICE_PRIVATE_DATA->abort_process_requested) {
				INDIGO_PLATESOLVER_DEVICE_PRIVATE_DATA->abort_process_requested = false;
				abort_mount_move(device);
				return false;
			}
			if (INDIGO_PLATESOLVER_DEVICE_PRIVATE_DATA->mount_process_state != INDIGO_BUSY_STATE) {
				break;
			}
			indigo_usleep(10000);
		}
		if (INDIGO_PLATESOLVER_DEVICE_PRIVATE_DATA->mount_process_state != INDIGO_OK_STATE) {
			indigo_error("Mount Agent didn't finish in 60s");
			return false;
		}
		indigo_sleep(settle_time);
		return true;
	}
	indigo_send_message(device, "No mount agent selected");
	return false;
}

static bool start_exposure(indigo_device *device, double exposure) {
	char *related_agent_name = indigo_filter_first_related_agent(FILTER_DEVICE_CONTEXT->device, "Imager Agent");
	if (related_agent_name != NULL) {
		if (INDIGO_PLATESOLVER_DEVICE_PRIVATE_DATA->imager_capture_state == INDIGO_BUSY_STATE) {
			indigo_error("Imager Agent is busy");
			return false;
		}
		indigo_change_number_property_1(FILTER_DEVICE_CONTEXT->client, related_agent_name, AGENT_IMAGER_CAPTURE_PROPERTY_NAME, AGENT_IMAGER_CAPTURE_ITEM_NAME, exposure);
		for (int i = 0; i < 6000; i++) { // wait 60s to become BUSY or ALERT
			if (INDIGO_PLATESOLVER_DEVICE_PRIVATE_DATA->abort_process_requested) {
				INDIGO_PLATESOLVER_DEVICE_PRIVATE_DATA->abort_process_requested = false;
				abort_exposure(device);
				return false;
			}
			if (INDIGO_PLATESOLVER_DEVICE_PRIVATE_DATA->imager_capture_state == INDIGO_BUSY_STATE || INDIGO_PLATESOLVER_DEVICE_PRIVATE_DATA->imager_capture_state == INDIGO_ALERT_STATE) {
				break;
			}
			indigo_usleep(10000);
		}
		if (INDIGO_PLATESOLVER_DEVICE_PRIVATE_DATA->imager_capture_state == INDIGO_ALERT_STATE) {
			indigo_error("Frame capture on Imager Agent failed");
			return false;
		}
		return true;
	}
	related_agent_name = indigo_filter_first_related_agent(FILTER_DEVICE_CONTEXT->device, "Guider Agent");
	if (related_agent_name != NULL) {
		if (INDIGO_PLATESOLVER_DEVICE_PRIVATE_DATA->guider_process_state == INDIGO_BUSY_STATE) {
			indigo_error("Guider Agent is busy");
			return false;
		}
		indigo_change_number_property_1(FILTER_DEVICE_CONTEXT->client, related_agent_name, AGENT_GUIDER_SETTINGS_PROPERTY_NAME, AGENT_GUIDER_SETTINGS_EXPOSURE_ITEM_NAME, exposure);
		indigo_change_switch_property_1(FILTER_DEVICE_CONTEXT->client, related_agent_name, AGENT_START_PROCESS_PROPERTY_NAME, AGENT_GUIDER_START_PREVIEW_1_ITEM_NAME, true);
		for (int i = 0; i < 100; i++) { // wait 1s to become BUSY or ALERT
			if (INDIGO_PLATESOLVER_DEVICE_PRIVATE_DATA->abort_process_requested) {
				INDIGO_PLATESOLVER_DEVICE_PRIVATE_DATA->abort_process_requested = false;
				abort_exposure(device);
				return false;
			}
			if (INDIGO_PLATESOLVER_DEVICE_PRIVATE_DATA->guider_process_state == INDIGO_BUSY_STATE || INDIGO_PLATESOLVER_DEVICE_PRIVATE_DATA->guider_process_state == INDIGO_ALERT_STATE) {
				break;
			}
			indigo_usleep(10000);
		}
		if (INDIGO_PLATESOLVER_DEVICE_PRIVATE_DATA->guider_process_state == INDIGO_ALERT_STATE) {
			indigo_error("Frame capture on Guider Agent failed");
			return false;
		}
		return true;
	}
	indigo_send_message(device, "Failed to start exposure - no image source agent selected");
	return false;
}

static void reset_pa_state(indigo_device * device, bool force) {
	if (force || AGENT_PLATESOLVER_PA_STATE_ITEM->number.value == INDIGO_POLAR_ALIGN_IN_PROGRESS) {
		if (AGENT_PLATESOLVER_PA_STATE_ITEM->number.value == INDIGO_POLAR_ALIGN_IN_PROGRESS || AGENT_PLATESOLVER_PA_STATE_ITEM->number.value == INDIGO_POLAR_ALIGN_IDLE) {
			AGENT_PLATESOLVER_PA_STATE_PROPERTY->state = INDIGO_OK_STATE;
		} else {
			AGENT_PLATESOLVER_PA_STATE_PROPERTY->state = INDIGO_ALERT_STATE;
		}
		AGENT_PLATESOLVER_PA_STATE_ITEM->number.value = INDIGO_POLAR_ALIGN_IDLE;
		AGENT_PLATESOLVER_PA_STATE_DEC_DRIFT_2_ITEM->number.value =
		AGENT_PLATESOLVER_PA_STATE_DEC_DRIFT_3_ITEM->number.value =
		AGENT_PLATESOLVER_PA_STATE_TARGET_RA_ITEM->number.value =
		AGENT_PLATESOLVER_PA_STATE_TARGET_DEC_ITEM->number.value =
		AGENT_PLATESOLVER_PA_STATE_CURRENT_RA_ITEM->number.value =
		AGENT_PLATESOLVER_PA_STATE_CURRENT_DEC_ITEM->number.value =
		AGENT_PLATESOLVER_PA_STATE_ALT_ERROR_ITEM->number.value =
		AGENT_PLATESOLVER_PA_STATE_AZ_ERROR_ITEM->number.value =
		AGENT_PLATESOLVER_PA_STATE_POLAR_ERROR_ITEM->number.value =
		AGENT_PLATESOLVER_PA_STATE_ALT_CORRECTION_UP_ITEM->number.value =
		AGENT_PLATESOLVER_PA_STATE_AZ_CORRECTION_CW_ITEM->number.value =
		AGENT_PLATESOLVER_PA_STATE_ACCURACY_WARNING_ITEM->number.value = 0;
		indigo_update_property(device, AGENT_PLATESOLVER_PA_STATE_PROPERTY, NULL);
	}
}

static void populate_pa_state(indigo_device * device) {
	AGENT_PLATESOLVER_PA_STATE_AZ_ERROR_ITEM->number.value = INDIGO_PLATESOLVER_DEVICE_PRIVATE_DATA->pa_az_error * RAD2DEG;
	AGENT_PLATESOLVER_PA_STATE_ALT_ERROR_ITEM->number.value = INDIGO_PLATESOLVER_DEVICE_PRIVATE_DATA->pa_alt_error * RAD2DEG;
	AGENT_PLATESOLVER_PA_STATE_POLAR_ERROR_ITEM->number.value = sqrt(
		AGENT_PLATESOLVER_PA_STATE_AZ_ERROR_ITEM->number.value * AGENT_PLATESOLVER_PA_STATE_AZ_ERROR_ITEM->number.value +
		AGENT_PLATESOLVER_PA_STATE_ALT_ERROR_ITEM->number.value * AGENT_PLATESOLVER_PA_STATE_ALT_ERROR_ITEM->number.value
	);

	if (INDIGO_PLATESOLVER_DEVICE_PRIVATE_DATA->geo_coordinates.d > 0) {
		/* Northern hemisphere */
		AGENT_PLATESOLVER_PA_STATE_ALT_CORRECTION_UP_ITEM->number.value = (INDIGO_PLATESOLVER_DEVICE_PRIVATE_DATA->pa_alt_error > 0) ? 1 : 0;
	} else {
		/* Southen hemisphere */
		AGENT_PLATESOLVER_PA_STATE_ALT_CORRECTION_UP_ITEM->number.value = (INDIGO_PLATESOLVER_DEVICE_PRIVATE_DATA->pa_alt_error > 0) ? 0 : 1;
	}
	/* for azimuth northern or southern does not matter as long as we use CW and CCW*/
	AGENT_PLATESOLVER_PA_STATE_AZ_CORRECTION_CW_ITEM->number.value = (INDIGO_PLATESOLVER_DEVICE_PRIVATE_DATA->pa_az_error > 0) ? 1 : 0;

	AGENT_PLATESOLVER_PA_STATE_TARGET_RA_ITEM->number.value = INDIGO_PLATESOLVER_DEVICE_PRIVATE_DATA->pa_target_ra;
	AGENT_PLATESOLVER_PA_STATE_TARGET_DEC_ITEM->number.value = INDIGO_PLATESOLVER_DEVICE_PRIVATE_DATA->pa_target_dec;

	AGENT_PLATESOLVER_PA_STATE_CURRENT_RA_ITEM->number.value = INDIGO_PLATESOLVER_DEVICE_PRIVATE_DATA->pa_current_ra;
	AGENT_PLATESOLVER_PA_STATE_CURRENT_DEC_ITEM->number.value = INDIGO_PLATESOLVER_DEVICE_PRIVATE_DATA->pa_current_dec;

	indigo_debug(
		"POLAR_ALIGN: Site lon = %f rad, lat = %f rad ",
		INDIGO_PLATESOLVER_DEVICE_PRIVATE_DATA->geo_coordinates.a,
		INDIGO_PLATESOLVER_DEVICE_PRIVATE_DATA->geo_coordinates.d
	);

	indigo_debug(
		"POLAR_ALIGN: targetRA = %.10f deg, targetDec = %.10f deg, currentRA = %.10f, currentDec = %.10f",
		AGENT_PLATESOLVER_PA_STATE_TARGET_RA_ITEM->number.value,
		AGENT_PLATESOLVER_PA_STATE_TARGET_DEC_ITEM->number.value,
		AGENT_PLATESOLVER_PA_STATE_CURRENT_RA_ITEM->number.value,
		AGENT_PLATESOLVER_PA_STATE_CURRENT_DEC_ITEM->number.value
	);

	indigo_debug(
		"POLAR_ALIGN: drift2 = %.10f deg, drift3 = %.10f deg, errorALT = %.10f', errorAZ = %.10f'",
		AGENT_PLATESOLVER_PA_STATE_DEC_DRIFT_2_ITEM->number.value,
		AGENT_PLATESOLVER_PA_STATE_DEC_DRIFT_3_ITEM->number.value,
		AGENT_PLATESOLVER_PA_STATE_ALT_ERROR_ITEM->number.value * 60,
		AGENT_PLATESOLVER_PA_STATE_AZ_ERROR_ITEM->number.value * 60
	);

	char message[256];
	if(INDIGO_PLATESOLVER_DEVICE_PRIVATE_DATA->pa_initial_error * 60.0 >= PA_MAX_INITIAL_ERROR) {
		AGENT_PLATESOLVER_PA_STATE_ACCURACY_WARNING_ITEM->number.value = 1;
		snprintf(message, sizeof(message), "Polar error: %.2f' (Warning: Initial polar error is loo large, align within 1-2° and restart polar alignment)", AGENT_PLATESOLVER_PA_STATE_POLAR_ERROR_ITEM->number.value * 60);
	} else {
		AGENT_PLATESOLVER_PA_STATE_ACCURACY_WARNING_ITEM->number.value = 0;
		snprintf(message, sizeof(message), "Polar error: %.2f'", AGENT_PLATESOLVER_PA_STATE_POLAR_ERROR_ITEM->number.value * 60);
	}
	indigo_send_message( device, message);

	indigo_send_message(
		device,
		"Azimuth error: %+.2f', move %s (use azimuth adjustment knob)",
		AGENT_PLATESOLVER_PA_STATE_AZ_ERROR_ITEM->number.value * 60,
		(AGENT_PLATESOLVER_PA_STATE_AZ_CORRECTION_CW_ITEM->number.value > 0) ? "C.W." : "C.C.W."
	);
	indigo_send_message(
		device,
		"Altitude error: %+.2f', move %s (use altitude adjustment knob)",
		AGENT_PLATESOLVER_PA_STATE_ALT_ERROR_ITEM->number.value * 60,
		(AGENT_PLATESOLVER_PA_STATE_ALT_CORRECTION_UP_ITEM->number.value > 0) ? "Up" : "Down"
	);
}

static void to_jnow_if_not(indigo_device *device, double *ra, double *dec) {
	/* if coordinates are in J2000 precess transform to JNow */
	if (AGENT_PLATESOLVER_WCS_EPOCH_ITEM->number.value != 0) {
		indigo_j2k_to_jnow(ra, dec);
	}
}

static void process_failed(indigo_device *device, char *message) {
	if (AGENT_PLATESOLVER_WCS_PROPERTY->state == INDIGO_BUSY_STATE) {
		AGENT_PLATESOLVER_WCS_PROPERTY->state = INDIGO_ALERT_STATE;
		AGENT_PLATESOLVER_WCS_STATE_ITEM->number.value = INDIGO_SOLVER_STATE_IDLE;
		indigo_update_property(device, AGENT_PLATESOLVER_WCS_PROPERTY, NULL);
	}
	if (AGENT_START_PROCESS_PROPERTY->state == INDIGO_BUSY_STATE) {
		indigo_set_switch(AGENT_PLATESOLVER_SYNC_PROPERTY, AGENT_PLATESOLVER_SYNC_PROPERTY->items + INDIGO_PLATESOLVER_DEVICE_PRIVATE_DATA->saved_sync_mode, true);
		indigo_update_property(device, AGENT_PLATESOLVER_SYNC_PROPERTY, NULL);
		AGENT_START_PROCESS_PROPERTY->state = INDIGO_ALERT_STATE;
		AGENT_PLATESOLVER_START_SOLVE_ITEM->sw.value =
		AGENT_PLATESOLVER_START_SYNC_ITEM->sw.value =
		AGENT_PLATESOLVER_START_CENTER_ITEM->sw.value =
		AGENT_PLATESOLVER_START_PRECISE_GOTO_ITEM->sw.value =
		AGENT_PLATESOLVER_START_CALCULATE_PA_ERROR_ITEM->sw.value =
		AGENT_PLATESOLVER_START_RECALCULATE_PA_ERROR_ITEM->sw.value = false;
		indigo_update_property(device, AGENT_START_PROCESS_PROPERTY, NULL);
	}
	indigo_send_message(device, message);
}

static void abort_process(indigo_device *device) {
	INDIGO_PLATESOLVER_DEVICE_PRIVATE_DATA->abort_process_requested = true;
	abort_exposure(device);
	abort_mount_move(device);
	INDIGO_PLATESOLVER_DEVICE_PRIVATE_DATA->abort(device);
	reset_pa_state(device, true);
	process_failed(device, "Process aborted");
}

static void start_process(indigo_device *device) {
	INDIGO_PLATESOLVER_DEVICE_PRIVATE_DATA->abort_process_requested = false;
	for (int i = 0; i < AGENT_PLATESOLVER_SYNC_PROPERTY->count; i++) {
		if (AGENT_PLATESOLVER_SYNC_PROPERTY->items[i].sw.value) {
			INDIGO_PLATESOLVER_DEVICE_PRIVATE_DATA->saved_sync_mode = i;
			break;
		}
	}
	if (AGENT_PLATESOLVER_START_SOLVE_ITEM->sw.value) {
		indigo_set_switch(AGENT_PLATESOLVER_SYNC_PROPERTY, AGENT_PLATESOLVER_SYNC_DISABLED_ITEM, true);
	} else if (AGENT_PLATESOLVER_START_SYNC_ITEM->sw.value) {
		indigo_set_switch(AGENT_PLATESOLVER_SYNC_PROPERTY, AGENT_PLATESOLVER_SYNC_SYNC_ITEM, true);
	} else if (AGENT_PLATESOLVER_START_CENTER_ITEM->sw.value) {
		indigo_set_switch(AGENT_PLATESOLVER_SYNC_PROPERTY, AGENT_PLATESOLVER_SYNC_CENTER_ITEM, true);
	} else if (AGENT_PLATESOLVER_START_PRECISE_GOTO_ITEM->sw.value) {
		indigo_set_switch(AGENT_PLATESOLVER_SYNC_PROPERTY, AGENT_PLATESOLVER_SYNC_CENTER_ITEM, true);
	} else if (AGENT_PLATESOLVER_START_CALCULATE_PA_ERROR_ITEM->sw.value) {
		indigo_set_switch(AGENT_PLATESOLVER_SYNC_PROPERTY, AGENT_PLATESOLVER_SYNC_CALCULATE_PA_ERROR_ITEM, true);
	} else if (AGENT_PLATESOLVER_START_RECALCULATE_PA_ERROR_ITEM->sw.value) {
		indigo_set_switch(AGENT_PLATESOLVER_SYNC_PROPERTY, AGENT_PLATESOLVER_SYNC_RECALCULATE_PA_ERROR_ITEM, true);
	}
	indigo_update_property(device, AGENT_PLATESOLVER_SYNC_PROPERTY, NULL);
	if (AGENT_PLATESOLVER_START_PRECISE_GOTO_ITEM->sw.value) {
		AGENT_PLATESOLVER_WCS_STATE_ITEM->number.value = INDIGO_SOLVER_STATE_GOTO;
		AGENT_PLATESOLVER_WCS_PROPERTY->state = INDIGO_BUSY_STATE;
		indigo_update_property(device, AGENT_PLATESOLVER_WCS_PROPERTY, NULL);
		if (!mount_slew(device, AGENT_PLATESOLVER_GOTO_SETTINGS_RA_ITEM->number.target,AGENT_PLATESOLVER_GOTO_SETTINGS_DEC_ITEM->number.target, AGENT_PLATESOLVER_MOUNT_SETTLE_TIME_ITEM->number.value)) {
			process_failed(device, "Slew failed");
			return;
		}
	}
	if (!start_exposure(device, AGENT_PLATESOLVER_EXPOSURE_SETTINGS_EXPOSURE_ITEM->number.value))
		process_failed(device, NULL);
}

static void solve(indigo_platesolver_task *task) {
	indigo_device *device = task->device;
	double recenter_ra = AGENT_PLATESOLVER_HINTS_RA_ITEM->number.value;
	double recenter_dec = AGENT_PLATESOLVER_HINTS_DEC_ITEM->number.value;
	INDIGO_PLATESOLVER_DEVICE_PRIVATE_DATA->abort_process_requested = false;

	if (AGENT_PLATESOLVER_SYNC_CALCULATE_PA_ERROR_ITEM->sw.value) {
		if (AGENT_PLATESOLVER_PA_STATE_ITEM->number.value == INDIGO_POLAR_ALIGN_REFERENCE_1) {
			INDIGO_PLATESOLVER_DEVICE_PRIVATE_DATA->eq_start_coordinates.a = INDIGO_PLATESOLVER_DEVICE_PRIVATE_DATA->eq_coordinates.a;
			INDIGO_PLATESOLVER_DEVICE_PRIVATE_DATA->eq_start_coordinates.d = INDIGO_PLATESOLVER_DEVICE_PRIVATE_DATA->eq_coordinates.d;
		}
	}

	// Solve with a particular plate solver

	if (task->image == NULL) {
		indigo_send_message(device, "Downloading image");
		if (!indigo_download_blob(task->image_url, &task->image, &task->size, NULL)) {
			process_failed(device, "Image download failed");
			return;
		}
	}
	AGENT_PLATESOLVER_IMAGE_OUTPUT_ITEM->blob.value = NULL;
	AGENT_PLATESOLVER_IMAGE_OUTPUT_ITEM->blob.size = 0;
	AGENT_PLATESOLVER_IMAGE_OUTPUT_ITEM->blob.format[0] = 0;
	AGENT_PLATESOLVER_IMAGE_OUTPUT_PROPERTY->state = INDIGO_BUSY_STATE;
	indigo_update_property(device, AGENT_PLATESOLVER_IMAGE_OUTPUT_PROPERTY, NULL);
	bool success = INDIGO_PLATESOLVER_DEVICE_PRIVATE_DATA->solve(device, task);
	if (success) {
		AGENT_PLATESOLVER_IMAGE_OUTPUT_ITEM->blob.value = task->image;
		AGENT_PLATESOLVER_IMAGE_OUTPUT_ITEM->blob.size = task->size;
		INDIGO_COPY_NAME(AGENT_PLATESOLVER_IMAGE_OUTPUT_ITEM->blob.format, task->format);
		AGENT_PLATESOLVER_IMAGE_OUTPUT_PROPERTY->state = INDIGO_OK_STATE;
		indigo_update_property(device, AGENT_PLATESOLVER_IMAGE_OUTPUT_PROPERTY, NULL);
	} else {
		AGENT_PLATESOLVER_IMAGE_OUTPUT_PROPERTY->state = INDIGO_ALERT_STATE;
		indigo_update_property(device, AGENT_PLATESOLVER_IMAGE_OUTPUT_PROPERTY, NULL);
	}
	indigo_safe_free(task->image);
	indigo_safe_free(task);
	if (!success) {
		if (AGENT_PLATESOLVER_PA_STATE_ITEM->number.value != INDIGO_POLAR_ALIGN_IDLE) {
			AGENT_PLATESOLVER_PA_STATE_PROPERTY->state = INDIGO_ALERT_STATE;
			if (AGENT_PLATESOLVER_PA_STATE_ITEM->number.value == INDIGO_POLAR_ALIGN_RECALCULATE) {
				AGENT_PLATESOLVER_PA_STATE_ITEM->number.value = INDIGO_POLAR_ALIGN_IN_PROGRESS;
			} else {
				AGENT_PLATESOLVER_PA_STATE_ITEM->number.value = INDIGO_POLAR_ALIGN_IDLE;
			}
			indigo_update_property(device, AGENT_PLATESOLVER_PA_STATE_PROPERTY, NULL);
		}
		process_failed(device, "Solving failed");
		return;
	}

	// Continue with a generic process
	set_fov(device, AGENT_PLATESOLVER_WCS_ANGLE_ITEM->number.value, AGENT_PLATESOLVER_WCS_WIDTH_ITEM->number.value, AGENT_PLATESOLVER_WCS_HEIGHT_ITEM->number.value);

	if (AGENT_PLATESOLVER_SYNC_SYNC_ITEM->sw.value || AGENT_PLATESOLVER_SYNC_CENTER_ITEM->sw.value) {
		AGENT_PLATESOLVER_WCS_STATE_ITEM->number.value = INDIGO_SOLVER_STATE_SYNCING;
		indigo_update_property(device, AGENT_PLATESOLVER_WCS_PROPERTY, NULL);
		if (!mount_sync(device, AGENT_PLATESOLVER_WCS_RA_ITEM->number.value, AGENT_PLATESOLVER_WCS_DEC_ITEM->number.value, 2)) {
			process_failed(device, "Sync failed");
			return;
		}
		indigo_send_message(device, "Synced");
	}

	if (AGENT_PLATESOLVER_SYNC_CENTER_ITEM->sw.value) {
		AGENT_PLATESOLVER_WCS_STATE_ITEM->number.value = INDIGO_SOLVER_STATE_CENTERING;
		indigo_update_property(device, AGENT_PLATESOLVER_WCS_PROPERTY, NULL);
		if (!mount_slew(device, recenter_ra, recenter_dec, AGENT_PLATESOLVER_MOUNT_SETTLE_TIME_ITEM->number.value)) {
			process_failed(device, "Slew failed");
			return;
		}
		indigo_send_message(device, "Centered");
	}

	if (AGENT_PLATESOLVER_SYNC_CALCULATE_PA_ERROR_ITEM->sw.value) {
		if (AGENT_PLATESOLVER_PA_STATE_ITEM->number.value == INDIGO_POLAR_ALIGN_REFERENCE_1) {
			indigo_debug("%s(): state INDIGO_POLAR_ALIGN_REFERENCE_1 -> INDIGO_POLAR_ALIGN_REFERENCE_2", __FUNCTION__);
			AGENT_PLATESOLVER_PA_STATE_PROPERTY->state = INDIGO_BUSY_STATE;
			AGENT_PLATESOLVER_PA_STATE_ITEM->number.value = INDIGO_POLAR_ALIGN_REFERENCE_2;
			double lst_now = indigo_lst(NULL, INDIGO_PLATESOLVER_DEVICE_PRIVATE_DATA->geo_coordinates.a * RAD2DEG);
			double ra = AGENT_PLATESOLVER_WCS_RA_ITEM->number.value;
			double dec = AGENT_PLATESOLVER_WCS_DEC_ITEM->number.value;
			to_jnow_if_not(device, &ra, &dec);
			indigo_ra_dec_to_point(
				ra,
				dec,
				lst_now,
				&INDIGO_PLATESOLVER_DEVICE_PRIVATE_DATA->pa_reference1
			);

			indigo_debug(
				"%s(): REFERECE 1: LST=%f h, HA=%f rad, Dec=%f rad",
				__FUNCTION__,
				lst_now * DEG2RAD * 15,
				INDIGO_PLATESOLVER_DEVICE_PRIVATE_DATA->pa_reference1.a,
				INDIGO_PLATESOLVER_DEVICE_PRIVATE_DATA->pa_reference1.d
			);

			indigo_update_property(device, AGENT_PLATESOLVER_PA_STATE_PROPERTY, NULL);
			AGENT_PLATESOLVER_WCS_STATE_ITEM->number.value = INDIGO_SOLVER_STATE_CENTERING;
			indigo_update_property(device, AGENT_PLATESOLVER_WCS_PROPERTY, NULL);
			bool ok = mount_slew(
				device,
				(INDIGO_PLATESOLVER_DEVICE_PRIVATE_DATA->eq_start_coordinates.a * RAD2DEG - AGENT_PLATESOLVER_PA_SETTINGS_HA_MOVE_ITEM->number.value) / 15,
				INDIGO_PLATESOLVER_DEVICE_PRIVATE_DATA->eq_start_coordinates.d * RAD2DEG,
				AGENT_PLATESOLVER_MOUNT_SETTLE_TIME_ITEM->number.value
			);
			if (ok) {
				ok = start_exposure(device, AGENT_PLATESOLVER_EXPOSURE_SETTINGS_EXPOSURE_ITEM->number.value);
			}
			if (!ok) {
				AGENT_PLATESOLVER_PA_STATE_PROPERTY->state = INDIGO_ALERT_STATE;
				AGENT_PLATESOLVER_PA_STATE_ITEM->number.value = INDIGO_POLAR_ALIGN_IDLE;
				indigo_update_property(device, AGENT_PLATESOLVER_PA_STATE_PROPERTY, NULL);
				process_failed(device, NULL);
				return;
			}
		} else if (AGENT_PLATESOLVER_PA_STATE_ITEM->number.value == INDIGO_POLAR_ALIGN_REFERENCE_2) {
			indigo_debug("%s(): state INDIGO_POLAR_ALIGN_REFERENCE_2 -> INDIGO_POLAR_ALIGN_REFERENCE_3", __FUNCTION__);
			AGENT_PLATESOLVER_PA_STATE_PROPERTY->state = INDIGO_BUSY_STATE;
			AGENT_PLATESOLVER_PA_STATE_ITEM->number.value = INDIGO_POLAR_ALIGN_REFERENCE_3;
			double lst_now = indigo_lst(NULL, INDIGO_PLATESOLVER_DEVICE_PRIVATE_DATA->geo_coordinates.a * RAD2DEG);
			double ra = AGENT_PLATESOLVER_WCS_RA_ITEM->number.value;
			double dec = AGENT_PLATESOLVER_WCS_DEC_ITEM->number.value;
			to_jnow_if_not(device, &ra, &dec);
			indigo_ra_dec_to_point(
				ra,
				dec,
				lst_now,
				&INDIGO_PLATESOLVER_DEVICE_PRIVATE_DATA->pa_reference2
			);

			indigo_debug(
				"%s(): REFERECE 2: LST=%f h, HA=%f rad, Dec=%f rad",
				__FUNCTION__,
				lst_now * DEG2RAD * 15,
				INDIGO_PLATESOLVER_DEVICE_PRIVATE_DATA->pa_reference2.a,
				INDIGO_PLATESOLVER_DEVICE_PRIVATE_DATA->pa_reference2.d
			);

			indigo_update_property(device, AGENT_PLATESOLVER_PA_STATE_PROPERTY, NULL);
			AGENT_PLATESOLVER_WCS_STATE_ITEM->number.value = INDIGO_SOLVER_STATE_CENTERING;
			indigo_update_property(device, AGENT_PLATESOLVER_WCS_PROPERTY, NULL);
			bool ok = mount_slew(
				device,
				(INDIGO_PLATESOLVER_DEVICE_PRIVATE_DATA->eq_start_coordinates.a * RAD2DEG - 2 * AGENT_PLATESOLVER_PA_SETTINGS_HA_MOVE_ITEM->number.value) / 15,
				INDIGO_PLATESOLVER_DEVICE_PRIVATE_DATA->eq_start_coordinates.d * RAD2DEG,
				AGENT_PLATESOLVER_MOUNT_SETTLE_TIME_ITEM->number.value
			);
			if (ok) {
				ok = start_exposure(device, AGENT_PLATESOLVER_EXPOSURE_SETTINGS_EXPOSURE_ITEM->number.value);
			}
			if (!ok) {
				AGENT_PLATESOLVER_PA_STATE_PROPERTY->state = INDIGO_ALERT_STATE;
				AGENT_PLATESOLVER_PA_STATE_ITEM->number.value = INDIGO_POLAR_ALIGN_IDLE;
				indigo_update_property(device, AGENT_PLATESOLVER_PA_STATE_PROPERTY, NULL);
				process_failed(device, NULL);
				return;
			}
		} else if (AGENT_PLATESOLVER_PA_STATE_ITEM->number.value == INDIGO_POLAR_ALIGN_REFERENCE_3) {
			double lst_now = indigo_lst(NULL, INDIGO_PLATESOLVER_DEVICE_PRIVATE_DATA->geo_coordinates.a * RAD2DEG);
			INDIGO_PLATESOLVER_DEVICE_PRIVATE_DATA->pa_current_ra = AGENT_PLATESOLVER_WCS_RA_ITEM->number.value;
			INDIGO_PLATESOLVER_DEVICE_PRIVATE_DATA->pa_current_dec = AGENT_PLATESOLVER_WCS_DEC_ITEM->number.value;
			to_jnow_if_not(device, &INDIGO_PLATESOLVER_DEVICE_PRIVATE_DATA->pa_current_ra, &INDIGO_PLATESOLVER_DEVICE_PRIVATE_DATA->pa_current_dec);
			indigo_ra_dec_to_point(
				INDIGO_PLATESOLVER_DEVICE_PRIVATE_DATA->pa_current_ra,
				INDIGO_PLATESOLVER_DEVICE_PRIVATE_DATA->pa_current_dec,
				lst_now,
				&INDIGO_PLATESOLVER_DEVICE_PRIVATE_DATA->pa_reference3
			);

			indigo_debug(
				"%s(): REFERECE 3: LST=%f h, HA=%f rad, Dec=%f rad",
				__FUNCTION__,
				lst_now,
				INDIGO_PLATESOLVER_DEVICE_PRIVATE_DATA->pa_reference3.a,
				INDIGO_PLATESOLVER_DEVICE_PRIVATE_DATA->pa_reference3.d
			);

			indigo_spherical_point_t reference1 = {0,0,0};
			indigo_spherical_point_t reference2 = {0,0,0};
			indigo_spherical_point_t reference3 = {0,0,0};

			if (AGENT_PLATESOLVER_PA_SETTINGS_COMPENSATE_REFRACTION_ITEM->number.value != 0) {
				indigo_compensate_refraction(&INDIGO_PLATESOLVER_DEVICE_PRIVATE_DATA->pa_reference1, INDIGO_PLATESOLVER_DEVICE_PRIVATE_DATA->geo_coordinates.d, &reference1);
				indigo_compensate_refraction(&INDIGO_PLATESOLVER_DEVICE_PRIVATE_DATA->pa_reference2, INDIGO_PLATESOLVER_DEVICE_PRIVATE_DATA->geo_coordinates.d, &reference2);
				indigo_compensate_refraction(&INDIGO_PLATESOLVER_DEVICE_PRIVATE_DATA->pa_reference3, INDIGO_PLATESOLVER_DEVICE_PRIVATE_DATA->geo_coordinates.d, &reference3);
			} else {
				reference1 = INDIGO_PLATESOLVER_DEVICE_PRIVATE_DATA->pa_reference1;
				reference2 = INDIGO_PLATESOLVER_DEVICE_PRIVATE_DATA->pa_reference2;
				reference3 = INDIGO_PLATESOLVER_DEVICE_PRIVATE_DATA->pa_reference3;
			}

			indigo_polar_alignment_error_3p(
				&reference1,
				&reference2,
				&reference3,
				&AGENT_PLATESOLVER_PA_STATE_DEC_DRIFT_2_ITEM->number.value,
				&AGENT_PLATESOLVER_PA_STATE_DEC_DRIFT_3_ITEM->number.value,
				&INDIGO_PLATESOLVER_DEVICE_PRIVATE_DATA->pa_alt_error,
				&INDIGO_PLATESOLVER_DEVICE_PRIVATE_DATA->pa_az_error
			);
			AGENT_PLATESOLVER_PA_STATE_DEC_DRIFT_2_ITEM->number.value *= RAD2DEG;
			AGENT_PLATESOLVER_PA_STATE_DEC_DRIFT_3_ITEM->number.value *= RAD2DEG;

			INDIGO_PLATESOLVER_DEVICE_PRIVATE_DATA->pa_initial_error = sqrt(
				INDIGO_PLATESOLVER_DEVICE_PRIVATE_DATA->pa_alt_error * INDIGO_PLATESOLVER_DEVICE_PRIVATE_DATA->pa_alt_error +
				INDIGO_PLATESOLVER_DEVICE_PRIVATE_DATA->pa_az_error * INDIGO_PLATESOLVER_DEVICE_PRIVATE_DATA->pa_az_error
			);

			indigo_debug("Initial polar error: %.2f degrees", INDIGO_PLATESOLVER_DEVICE_PRIVATE_DATA->pa_initial_error * 60);

			// here we do not care about the refraction since we work with real coordinates
			indigo_spherical_point_t target_position = {0,0,0};
			indigo_polar_alignment_target_position(
				&INDIGO_PLATESOLVER_DEVICE_PRIVATE_DATA->pa_reference3,
				INDIGO_PLATESOLVER_DEVICE_PRIVATE_DATA->pa_alt_error,
				INDIGO_PLATESOLVER_DEVICE_PRIVATE_DATA->pa_az_error,
				&target_position
			);
			indigo_point_to_ra_dec(
				&target_position,
				lst_now,
				&INDIGO_PLATESOLVER_DEVICE_PRIVATE_DATA->pa_target_ra,
				&INDIGO_PLATESOLVER_DEVICE_PRIVATE_DATA->pa_target_dec
			);

			populate_pa_state(device);

			indigo_debug("%s(): state INDIGO_POLAR_ALIGN_REFERENCE_3 -> INDIGO_POLAR_ALIGN_IN_PROGRESS", __FUNCTION__);
			AGENT_PLATESOLVER_PA_STATE_PROPERTY->state = INDIGO_OK_STATE;
			AGENT_PLATESOLVER_PA_STATE_ITEM->number.value = INDIGO_POLAR_ALIGN_IN_PROGRESS;
			indigo_update_property(device, AGENT_PLATESOLVER_PA_STATE_PROPERTY, NULL);
		}
	}

	if (AGENT_PLATESOLVER_SYNC_RECALCULATE_PA_ERROR_ITEM->sw.value) {
		if (AGENT_PLATESOLVER_PA_STATE_ITEM->number.value == INDIGO_POLAR_ALIGN_RECALCULATE) {
			indigo_spherical_point_t position_raw = {0,0,0};
			indigo_spherical_point_t reference_position_raw = {0,0,0};
			indigo_spherical_point_t position = {0,0,0};
			indigo_spherical_point_t reference_position = {0,0,0};
			double lst_now = indigo_lst(NULL, INDIGO_PLATESOLVER_DEVICE_PRIVATE_DATA->geo_coordinates.a * RAD2DEG);
			INDIGO_PLATESOLVER_DEVICE_PRIVATE_DATA->pa_current_ra = AGENT_PLATESOLVER_WCS_RA_ITEM->number.value;
			INDIGO_PLATESOLVER_DEVICE_PRIVATE_DATA->pa_current_dec = AGENT_PLATESOLVER_WCS_DEC_ITEM->number.value;
			to_jnow_if_not(device, &INDIGO_PLATESOLVER_DEVICE_PRIVATE_DATA->pa_current_ra, &INDIGO_PLATESOLVER_DEVICE_PRIVATE_DATA->pa_current_dec);
			indigo_ra_dec_to_point(
				INDIGO_PLATESOLVER_DEVICE_PRIVATE_DATA->pa_current_ra,
				INDIGO_PLATESOLVER_DEVICE_PRIVATE_DATA->pa_current_dec,
				lst_now,
				&position_raw
			);
			indigo_ra_dec_to_point(
				INDIGO_PLATESOLVER_DEVICE_PRIVATE_DATA->pa_target_ra,
				INDIGO_PLATESOLVER_DEVICE_PRIVATE_DATA->pa_target_dec,
				lst_now,
				&reference_position_raw
			);

			if (AGENT_PLATESOLVER_PA_SETTINGS_COMPENSATE_REFRACTION_ITEM->number.value != 0) {
				indigo_compensate_refraction(&position_raw, INDIGO_PLATESOLVER_DEVICE_PRIVATE_DATA->geo_coordinates.d, &position);
				indigo_compensate_refraction(&reference_position_raw, INDIGO_PLATESOLVER_DEVICE_PRIVATE_DATA->geo_coordinates.d, &reference_position);
			} else {
				position = position_raw;
				reference_position = reference_position_raw;
			}

			bool ok = indigo_reestimate_polar_error(
				&position,
				&reference_position,
				&INDIGO_PLATESOLVER_DEVICE_PRIVATE_DATA->pa_alt_error,
				&INDIGO_PLATESOLVER_DEVICE_PRIVATE_DATA->pa_az_error
			);
			if (!ok) {
				AGENT_PLATESOLVER_PA_STATE_PROPERTY->state = INDIGO_ALERT_STATE;
				AGENT_PLATESOLVER_PA_STATE_ITEM->number.value = INDIGO_POLAR_ALIGN_IDLE;
				indigo_update_property(device, AGENT_PLATESOLVER_PA_STATE_PROPERTY, NULL);
				char message[256];
				snprintf(message, sizeof(message), "Polar error exceeds the maximal error of %.0f°, align better and restart polar alignment", PA_MAX_ERROR);
				process_failed(device, message);
				return;
			}

			indigo_debug(
				"%s(): CURRENT: LST=%f h",
				__FUNCTION__,
				lst_now
			);

			populate_pa_state(device);

			indigo_debug("%s(): state INDIGO_POLAR_ALIGN_RECALCULATE -> INDIGO_POLAR_ALIGN_IN_PROGRESS", __FUNCTION__);
			AGENT_PLATESOLVER_PA_STATE_PROPERTY->state = INDIGO_OK_STATE;
			AGENT_PLATESOLVER_PA_STATE_ITEM->number.value = INDIGO_POLAR_ALIGN_IN_PROGRESS;
			indigo_update_property(device, AGENT_PLATESOLVER_PA_STATE_PROPERTY, NULL);
		} else {
			indigo_debug("%s(): state INDIGO_POLAR_ALIGN_RECALCULATE -> INDIGO_POLAR_ALIGN_IDLE", __FUNCTION__);
			AGENT_PLATESOLVER_PA_STATE_PROPERTY->state = INDIGO_ALERT_STATE;
			AGENT_PLATESOLVER_PA_STATE_ITEM->number.value = INDIGO_POLAR_ALIGN_IDLE;
			indigo_update_property(device, AGENT_PLATESOLVER_PA_STATE_PROPERTY, NULL);
			process_failed(device, "Alignment process is not in progress");
			return;
		}
	}
	AGENT_PLATESOLVER_WCS_PROPERTY->state = INDIGO_OK_STATE;
	AGENT_PLATESOLVER_WCS_STATE_ITEM->number.value = INDIGO_SOLVER_STATE_IDLE;
	indigo_update_property(device, AGENT_PLATESOLVER_WCS_PROPERTY, NULL);

	if (AGENT_START_PROCESS_PROPERTY->state == INDIGO_BUSY_STATE && AGENT_PLATESOLVER_PA_STATE_PROPERTY->state != INDIGO_BUSY_STATE) {
		indigo_set_switch(AGENT_PLATESOLVER_SYNC_PROPERTY, AGENT_PLATESOLVER_SYNC_PROPERTY->items + INDIGO_PLATESOLVER_DEVICE_PRIVATE_DATA->saved_sync_mode, true);
		indigo_update_property(device, AGENT_PLATESOLVER_SYNC_PROPERTY, NULL);
		AGENT_START_PROCESS_PROPERTY->state = INDIGO_OK_STATE;
		AGENT_PLATESOLVER_START_SOLVE_ITEM->sw.value =
		AGENT_PLATESOLVER_START_SYNC_ITEM->sw.value =
		AGENT_PLATESOLVER_START_CENTER_ITEM->sw.value =
		AGENT_PLATESOLVER_START_PRECISE_GOTO_ITEM->sw.value =
		AGENT_PLATESOLVER_START_CALCULATE_PA_ERROR_ITEM->sw.value =
		AGENT_PLATESOLVER_START_RECALCULATE_PA_ERROR_ITEM->sw.value = false;
		indigo_update_property(device, AGENT_START_PROCESS_PROPERTY, NULL);
	}
}

indigo_result indigo_platesolver_device_attach(indigo_device *device, const char* driver_name, unsigned version, indigo_device_interface device_interface) {
	assert(device != NULL);
	if (indigo_filter_device_attach(device, driver_name, version, device_interface) == INDIGO_OK) {
		// -------------------------------------------------------------------------------- FILTER_RELATED_AGENT_LIST
		FILTER_RELATED_AGENT_LIST_PROPERTY->hidden = false;
		FILTER_DEVICE_CONTEXT->validate_related_agent = validate_related_agent;
		// -------------------------------------------------------------------------------- AGENT_PLATESOLVER_USE_INDEX
		AGENT_PLATESOLVER_USE_INDEX_PROPERTY = indigo_init_switch_property(NULL, device->name, AGENT_PLATESOLVER_USE_INDEX_PROPERTY_NAME, PLATESOLVER_MAIN_GROUP, "Use indexes", INDIGO_OK_STATE, INDIGO_RW_PERM, INDIGO_ANY_OF_MANY_RULE, 33);
		if (AGENT_PLATESOLVER_USE_INDEX_PROPERTY == NULL) {
			return INDIGO_FAILED;
		}
		AGENT_PLATESOLVER_USE_INDEX_PROPERTY->count = 0;
		// -------------------------------------------------------------------------------- Hints property
		AGENT_PLATESOLVER_HINTS_PROPERTY = indigo_init_number_property(NULL, device->name, AGENT_PLATESOLVER_HINTS_PROPERTY_NAME, PLATESOLVER_MAIN_GROUP, "Hints", INDIGO_OK_STATE, INDIGO_RW_PERM, 9);
		if (AGENT_PLATESOLVER_HINTS_PROPERTY == NULL) {
			return INDIGO_FAILED;
		}
		indigo_init_number_item(AGENT_PLATESOLVER_HINTS_RADIUS_ITEM, AGENT_PLATESOLVER_HINTS_RADIUS_ITEM_NAME, "Search radius (°)", 0, 360, 2, 0);
		indigo_init_sexagesimal_number_item(AGENT_PLATESOLVER_HINTS_RA_ITEM, AGENT_PLATESOLVER_HINTS_RA_ITEM_NAME, "RA (hours)", 0, 24, 0, 0);
		indigo_init_sexagesimal_number_item(AGENT_PLATESOLVER_HINTS_DEC_ITEM, AGENT_PLATESOLVER_HINTS_DEC_ITEM_NAME, "Dec (°)", -90, 90, 0, 0);
		indigo_init_number_item(AGENT_PLATESOLVER_HINTS_EPOCH_ITEM, AGENT_PLATESOLVER_HINTS_EPOCH_ITEM_NAME, "J2000 (2000=J2000, 0=JNow)", 0, 2050, 1, 2000);
		indigo_init_number_item(AGENT_PLATESOLVER_HINTS_SCALE_ITEM, AGENT_PLATESOLVER_HINTS_SCALE_ITEM_NAME, "Pixel scale ( < 0: camera scale) (°/pixel)", -1, 5, -1, 0);
		indigo_init_number_item(AGENT_PLATESOLVER_HINTS_PARITY_ITEM, AGENT_PLATESOLVER_HINTS_PARITY_ITEM_NAME, "Parity (-1,0,1)", -1, 1, 1, 0);
		indigo_init_number_item(AGENT_PLATESOLVER_HINTS_DOWNSAMPLE_ITEM, AGENT_PLATESOLVER_HINTS_DOWNSAMPLE_ITEM_NAME, "Downsample", 1, 16, 1, 2);
		indigo_init_number_item(AGENT_PLATESOLVER_HINTS_DEPTH_ITEM, AGENT_PLATESOLVER_HINTS_DEPTH_ITEM_NAME, "Depth", 0, 1000, 5, 0);
		indigo_init_number_item(AGENT_PLATESOLVER_HINTS_CPU_LIMIT_ITEM, AGENT_PLATESOLVER_HINTS_CPU_LIMIT_ITEM_NAME, "CPU Limit (seconds)", 0, 600, 10, 180);
		strcpy(AGENT_PLATESOLVER_HINTS_RADIUS_ITEM->number.format, "%m");
		strcpy(AGENT_PLATESOLVER_HINTS_RA_ITEM->number.format, "%m");
		strcpy(AGENT_PLATESOLVER_HINTS_DEC_ITEM->number.format, "%m");
		strcpy(AGENT_PLATESOLVER_HINTS_SCALE_ITEM->number.format, "%m");
		// -------------------------------------------------------------------------------- WCS property
		AGENT_PLATESOLVER_WCS_PROPERTY = indigo_init_number_property(NULL, device->name, AGENT_PLATESOLVER_WCS_PROPERTY_NAME, PLATESOLVER_MAIN_GROUP, "WCS solution", INDIGO_OK_STATE, INDIGO_RO_PERM, 10);
		if (AGENT_PLATESOLVER_WCS_PROPERTY == NULL) {
			return INDIGO_FAILED;
		}
		indigo_init_number_item(AGENT_PLATESOLVER_WCS_STATE_ITEM, AGENT_PLATESOLVER_WCS_STATE_ITEM_NAME, "WCS solution state", 0, 5, 0, 0);
		indigo_init_sexagesimal_number_item(AGENT_PLATESOLVER_WCS_RA_ITEM, AGENT_PLATESOLVER_WCS_RA_ITEM_NAME, "Frame center RA (hours)", 0, 24, 0, 0);
		indigo_init_sexagesimal_number_item(AGENT_PLATESOLVER_WCS_DEC_ITEM, AGENT_PLATESOLVER_WCS_DEC_ITEM_NAME, "Frame center Dec (°)", 0, 360, 0, 0);
		indigo_init_number_item(AGENT_PLATESOLVER_WCS_EPOCH_ITEM, AGENT_PLATESOLVER_WCS_EPOCH_ITEM_NAME, "J2000 (2000=J2000, 0=JNow)", 0, 2000, 0, 2000);
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
		// -------------------------------------------------------------------------------- AGENT_PLATESOLVER_SYNC property /* OBSOLETED */
		AGENT_PLATESOLVER_SYNC_PROPERTY = indigo_init_switch_property(NULL, device->name, AGENT_PLATESOLVER_SYNC_PROPERTY_NAME, PLATESOLVER_MAIN_GROUP, "Sync mode (obsolete)", INDIGO_OK_STATE, INDIGO_RW_PERM, INDIGO_ONE_OF_MANY_RULE, 5);
		if (AGENT_PLATESOLVER_SYNC_PROPERTY == NULL) {
			return INDIGO_FAILED;
		}
		indigo_init_switch_item(AGENT_PLATESOLVER_SYNC_DISABLED_ITEM, AGENT_PLATESOLVER_SYNC_DISABLED_ITEM_NAME, "Disabled", true);
		indigo_init_switch_item(AGENT_PLATESOLVER_SYNC_SYNC_ITEM, AGENT_PLATESOLVER_SYNC_SYNC_ITEM_NAME, "Sync only", false);
		indigo_init_switch_item(AGENT_PLATESOLVER_SYNC_CENTER_ITEM, AGENT_PLATESOLVER_SYNC_CENTER_ITEM_NAME, "Sync and center", false);
		indigo_init_switch_item(AGENT_PLATESOLVER_SYNC_CALCULATE_PA_ERROR_ITEM, AGENT_PLATESOLVER_SYNC_CALCULATE_PA_ERROR_ITEM_NAME, "Calclulate polar alignment error", false);
		indigo_init_switch_item(AGENT_PLATESOLVER_SYNC_RECALCULATE_PA_ERROR_ITEM, AGENT_PLATESOLVER_SYNC_RECALCULATE_PA_ERROR_ITEM_NAME, "Recalclulate polar alignment error", false);
		// -------------------------------------------------------------------------------- AGENT_START_PROCESS property /* replaces AGENT_PLATESOLVER_SYNC */
		AGENT_START_PROCESS_PROPERTY = indigo_init_switch_property(NULL, device->name, AGENT_START_PROCESS_PROPERTY_NAME, PLATESOLVER_MAIN_GROUP, "Start process", INDIGO_OK_STATE, INDIGO_RW_PERM, INDIGO_AT_MOST_ONE_RULE, 6);
		if (AGENT_PLATESOLVER_SYNC_PROPERTY == NULL) {
			return INDIGO_FAILED;
		}
		indigo_init_switch_item(AGENT_PLATESOLVER_START_SOLVE_ITEM, AGENT_PLATESOLVER_START_SOLVE_ITEM_NAME, "Solve only", false);
		indigo_init_switch_item(AGENT_PLATESOLVER_START_SYNC_ITEM, AGENT_PLATESOLVER_START_SYNC_ITEM_NAME, "Solve and sync", false);
		indigo_init_switch_item(AGENT_PLATESOLVER_START_CENTER_ITEM, AGENT_PLATESOLVER_START_CENTER_ITEM_NAME, "Solve, sync and center", false);
		indigo_init_switch_item(AGENT_PLATESOLVER_START_PRECISE_GOTO_ITEM, AGENT_PLATESOLVER_START_PRECISE_GOTO_ITEM_NAME, "Precise GOTO", false);
		indigo_init_switch_item(AGENT_PLATESOLVER_START_CALCULATE_PA_ERROR_ITEM, AGENT_PLATESOLVER_START_CALCULATE_PA_ERROR_ITEM_NAME, "Calclulate polar alignment error", false);
		indigo_init_switch_item(AGENT_PLATESOLVER_START_RECALCULATE_PA_ERROR_ITEM, AGENT_PLATESOLVER_START_RECALCULATE_PA_ERROR_ITEM_NAME, "Recalclulate polar alignment error", false);
		// -------------------------------------------------------------------------------- AGENT_ABORT_PROCESS property /* replaces AGENT_PLATESOLVER_ABORT */
		AGENT_ABORT_PROCESS_PROPERTY = indigo_init_switch_property(NULL, device->name, AGENT_ABORT_PROCESS_PROPERTY_NAME, "Agent", "Abort process", INDIGO_OK_STATE, INDIGO_RW_PERM, INDIGO_AT_MOST_ONE_RULE, 1);
		if (AGENT_ABORT_PROCESS_PROPERTY == NULL) {
			return INDIGO_FAILED;
		}
		indigo_init_switch_item(AGENT_ABORT_PROCESS_ITEM, AGENT_ABORT_PROCESS_ITEM_NAME, "Abort process", false);
		// -------------------------------------------------------------------------------- AGENT_PLATESOLVER_SOLVE_IMAGES
		AGENT_PLATESOLVER_SOLVE_IMAGES_PROPERTY = indigo_init_switch_property(NULL, device->name, AGENT_PLATESOLVER_SOLVE_IMAGES_PROPERTY_NAME, PLATESOLVER_MAIN_GROUP, "Solve images from related agents", INDIGO_OK_STATE, INDIGO_RW_PERM, INDIGO_ONE_OF_MANY_RULE, 2);
		if (AGENT_PLATESOLVER_SOLVE_IMAGES_PROPERTY == NULL) {
			return INDIGO_FAILED;
		}
		indigo_init_switch_item(AGENT_PLATESOLVER_SOLVE_IMAGES_ENABLED_ITEM, AGENT_PLATESOLVER_SOLVE_IMAGES_ENABLED_ITEM_NAME, "Enabled", true);
		indigo_init_switch_item(AGENT_PLATESOLVER_SOLVE_IMAGES_DISABLED_ITEM, AGENT_PLATESOLVER_SOLVE_IMAGES_DISABLED_ITEM_NAME, "Disabled", false);
		// -------------------------------------------------------------------------------- SETTINGS property
		AGENT_PLATESOLVER_EXPOSURE_SETTINGS_PROPERTY = indigo_init_number_property(NULL, device->name, AGENT_PLATESOLVER_EXPOSURE_SETTINGS_PROPERTY_NAME, PLATESOLVER_MAIN_GROUP, "Exposure settings", INDIGO_OK_STATE, INDIGO_RW_PERM, 1);
		if (AGENT_PLATESOLVER_EXPOSURE_SETTINGS_PROPERTY == NULL) {
			return INDIGO_FAILED;
		}
		indigo_init_number_item(AGENT_PLATESOLVER_EXPOSURE_SETTINGS_EXPOSURE_ITEM, AGENT_PLATESOLVER_EXPOSURE_SETTINGS_EXPOSURE_ITEM_NAME, "Exposure time (s)", 0, 60, 1, 1);
		// -------------------------------------------------------------------------------- POLAR_ALIGNMENT_SETTINGS property
		AGENT_PLATESOLVER_PA_SETTINGS_PROPERTY = indigo_init_number_property(NULL, device->name, AGENT_PLATESOLVER_PA_SETTINGS_PROPERTY_NAME, PLATESOLVER_MAIN_GROUP, "Polar alignment settings", INDIGO_OK_STATE, INDIGO_RW_PERM, 3);
		if (AGENT_PLATESOLVER_PA_SETTINGS_PROPERTY == NULL) {
			return INDIGO_FAILED;
		}
		indigo_init_number_item(AGENT_PLATESOLVER_PA_SETTINGS_EXPOSURE_ITEM, AGENT_PLATESOLVER_PA_SETTINGS_EXPOSURE_ITEM_NAME, "Exposure time (s) (obsolete)", 0, 60, 1, 1);
		indigo_init_number_item(AGENT_PLATESOLVER_PA_SETTINGS_HA_MOVE_ITEM, AGENT_PLATESOLVER_PA_SETTINGS_HA_MOVE_ITEM_NAME, "Hour angle move (°)", -50, 50, 5, 20);
		indigo_init_number_item(AGENT_PLATESOLVER_PA_SETTINGS_COMPENSATE_REFRACTION_ITEM, AGENT_PLATESOLVER_PA_SETTINGS_COMPENSATE_REFRACTION_ITEM_NAME, "Compensate refraction (1=On, 0=Off)", 0, 1, 0, 0);
		strcpy(AGENT_PLATESOLVER_PA_SETTINGS_HA_MOVE_ITEM->number.format, "%m");
		strcpy(AGENT_PLATESOLVER_PA_SETTINGS_COMPENSATE_REFRACTION_ITEM->number.format, "%.0f");
		// -------------------------------------------------------------------------------- POLAR_ALIGNMENT_ERROR property
		AGENT_PLATESOLVER_PA_STATE_PROPERTY = indigo_init_number_property(NULL, device->name, AGENT_PLATESOLVER_PA_STATE_PROPERTY_NAME, PLATESOLVER_MAIN_GROUP, "Polar alignment state", INDIGO_OK_STATE, INDIGO_RO_PERM, 13);
		if (AGENT_PLATESOLVER_PA_STATE_PROPERTY == NULL) {
			return INDIGO_FAILED;
		}
		indigo_init_number_item(AGENT_PLATESOLVER_PA_STATE_ITEM, AGENT_PLATESOLVER_PA_STATE_ITEM_NAME, "Polar alignment state", 0, 10, 0, 0);
		indigo_init_number_item(AGENT_PLATESOLVER_PA_STATE_DEC_DRIFT_2_ITEM, AGENT_PLATESOLVER_PA_STATE_DEC_DRIFT_2_ITEM_NAME, "Declination drift at point 2 (°)", -45, 45, 0, 0);
		indigo_init_number_item(AGENT_PLATESOLVER_PA_STATE_DEC_DRIFT_3_ITEM, AGENT_PLATESOLVER_PA_STATE_DEC_DRIFT_3_ITEM_NAME, "Declination drift at point 3 (°)", -45, 45, 0, 0);
		indigo_init_number_item(AGENT_PLATESOLVER_PA_STATE_TARGET_RA_ITEM, AGENT_PLATESOLVER_PA_STATE_TARGET_RA_ITEM_NAME, "Target position RA (h)", 0, 24, 0, 0);
		indigo_init_number_item(AGENT_PLATESOLVER_PA_STATE_TARGET_DEC_ITEM, AGENT_PLATESOLVER_PA_STATE_TARGET_DEC_ITEM_NAME, "Target position DEC (°)", -90, +90, 0, 0);
		indigo_init_number_item(AGENT_PLATESOLVER_PA_STATE_CURRENT_RA_ITEM, AGENT_PLATESOLVER_PA_STATE_CURRENT_RA_ITEM_NAME, "Current position RA (h)", 0, 24, 0, 0);
		indigo_init_number_item(AGENT_PLATESOLVER_PA_STATE_CURRENT_DEC_ITEM, AGENT_PLATESOLVER_PA_STATE_CURRENT_DEC_ITEM_NAME, "Current position DEC (°)", -90, +90, 0, 0);
		indigo_init_number_item(AGENT_PLATESOLVER_PA_STATE_AZ_ERROR_ITEM, AGENT_PLATESOLVER_PA_STATE_AZ_ERROR_ITEM_NAME, "Azimuth error (°)", -45, 45, 0, 0);
		indigo_init_number_item(AGENT_PLATESOLVER_PA_STATE_ALT_ERROR_ITEM, AGENT_PLATESOLVER_PA_STATE_ALT_ERROR_ITEM_NAME, "Altitude error (°)", -45, 45, 0, 0);
		indigo_init_number_item(AGENT_PLATESOLVER_PA_STATE_ALT_CORRECTION_UP_ITEM, AGENT_PLATESOLVER_PA_STATE_ALT_CORRECTION_UP_ITEM_NAME, "Altitude correction (1=Up, 0=Down)", 0, 1, 0, 0);
		indigo_init_number_item(AGENT_PLATESOLVER_PA_STATE_AZ_CORRECTION_CW_ITEM, AGENT_PLATESOLVER_PA_STATE_AZ_CORRECTION_CW_ITEM_NAME, "Azimuth correction (1=C.W., 0=C.C.W.)", 0, 1, 0, 0);
		indigo_init_number_item(AGENT_PLATESOLVER_PA_STATE_POLAR_ERROR_ITEM, AGENT_PLATESOLVER_PA_STATE_POLAR_ERROR_ITEM_NAME, "Polar error (°)", -45, 45, 0, 0);
		indigo_init_number_item(AGENT_PLATESOLVER_PA_STATE_ACCURACY_WARNING_ITEM, AGENT_PLATESOLVER_PA_STATE_ACCURACY_WARNING_ITEM_NAME, "Accuracy warning", 0, 1, 0, 0);
		strcpy(AGENT_PLATESOLVER_PA_STATE_ITEM->number.format, "%.0f");
		strcpy(AGENT_PLATESOLVER_PA_STATE_DEC_DRIFT_2_ITEM->number.format, "%m");
		strcpy(AGENT_PLATESOLVER_PA_STATE_DEC_DRIFT_3_ITEM->number.format, "%m");
		strcpy(AGENT_PLATESOLVER_PA_STATE_TARGET_RA_ITEM->number.format, "%m");
		strcpy(AGENT_PLATESOLVER_PA_STATE_TARGET_DEC_ITEM->number.format, "%m");
		strcpy(AGENT_PLATESOLVER_PA_STATE_CURRENT_RA_ITEM->number.format, "%m");
		strcpy(AGENT_PLATESOLVER_PA_STATE_CURRENT_DEC_ITEM->number.format, "%m");
		strcpy(AGENT_PLATESOLVER_PA_STATE_AZ_ERROR_ITEM->number.format, "%m");
		strcpy(AGENT_PLATESOLVER_PA_STATE_ALT_ERROR_ITEM->number.format, "%m");
		strcpy(AGENT_PLATESOLVER_PA_STATE_ALT_CORRECTION_UP_ITEM->number.format, "%.0f");
		strcpy(AGENT_PLATESOLVER_PA_STATE_AZ_CORRECTION_CW_ITEM->number.format, "%.0f");
		strcpy(AGENT_PLATESOLVER_PA_STATE_POLAR_ERROR_ITEM->number.format, "%m");
		strcpy(AGENT_PLATESOLVER_PA_STATE_ACCURACY_WARNING_ITEM->number.format, "%.0f");
		// -------------------------------------------------------------------------------- AGENT_PLATESOLVER_GOTO_SETTINGS
		AGENT_PLATESOLVER_GOTO_SETTINGS_PROPERTY = indigo_init_number_property(NULL, device->name, AGENT_PLATESOLVER_GOTO_SETTINGS_PROPERTY_NAME, PLATESOLVER_MAIN_GROUP, "GOTO Settings", INDIGO_OK_STATE, INDIGO_RW_PERM, 2);
		if (AGENT_PLATESOLVER_GOTO_SETTINGS_PROPERTY == NULL) {
			return INDIGO_FAILED;
		}
		indigo_init_sexagesimal_number_item(AGENT_PLATESOLVER_GOTO_SETTINGS_RA_ITEM, AGENT_PLATESOLVER_GOTO_SETTINGS_RA_ITEM_NAME, "Right ascension (0 to 24 hrs)", 0, 24, 0, 0);
		indigo_init_sexagesimal_number_item(AGENT_PLATESOLVER_GOTO_SETTINGS_DEC_ITEM, AGENT_PLATESOLVER_GOTO_SETTINGS_DEC_ITEM_NAME, "Declination (-90 to 90°)", -90, 90, 0, 90);
		// -------------------------------------------------------------------------------- MOUNT_SETTLE_TIME property
		AGENT_PLATESOLVER_MOUNT_SETTLE_TIME_PROPERTY = indigo_init_number_property(NULL, device->name, AGENT_PLATESOLVER_MOUNT_SETTLE_TIME_PROPERTY_NAME, PLATESOLVER_MAIN_GROUP, "Mount settle time", INDIGO_OK_STATE, INDIGO_RW_PERM, 1);
		if (AGENT_PLATESOLVER_MOUNT_SETTLE_TIME_PROPERTY == NULL) {
			return INDIGO_FAILED;
		}
		indigo_init_number_item(AGENT_PLATESOLVER_MOUNT_SETTLE_TIME_ITEM, AGENT_PLATESOLVER_MOUNT_SETTLE_TIME_ITEM_NAME, "Settle time (s)", 0, 60, 1, 3);
		// -------------------------------------------------------------------------------- AGENT_PLATESOLVER_ABORT property /* OBSOLETED */
		AGENT_PLATESOLVER_ABORT_PROPERTY = indigo_init_switch_property(NULL, device->name, AGENT_PLATESOLVER_ABORT_PROPERTY_NAME, PLATESOLVER_MAIN_GROUP, "Abort", INDIGO_OK_STATE, INDIGO_RW_PERM, INDIGO_AT_MOST_ONE_RULE, 1);
		if (AGENT_PLATESOLVER_ABORT_PROPERTY == NULL) {
			return INDIGO_FAILED;
		}
		indigo_init_switch_item(AGENT_PLATESOLVER_ABORT_ITEM, AGENT_PLATESOLVER_ABORT_ITEM_NAME, "Abort", false);
		// -------------------------------------------------------------------------------- AGENT_PLATESOLVER_IMAGE property
		AGENT_PLATESOLVER_IMAGE_PROPERTY = indigo_init_blob_property_p(NULL, device->name, AGENT_PLATESOLVER_IMAGE_PROPERTY_NAME, PLATESOLVER_MAIN_GROUP, "Image upload", INDIGO_OK_STATE, INDIGO_WO_PERM, 1);
		if (AGENT_PLATESOLVER_IMAGE_PROPERTY == NULL) {
			return INDIGO_FAILED;
		}
		indigo_init_blob_item(AGENT_PLATESOLVER_IMAGE_ITEM, AGENT_PLATESOLVER_IMAGE_ITEM_NAME, "Image");
		// -------------------------------------------------------------------------------- AGENT_PLATESOLVER_IMAGE_OUTPUT property
		AGENT_PLATESOLVER_IMAGE_OUTPUT_PROPERTY = indigo_init_blob_property(NULL, device->name, AGENT_PLATESOLVER_IMAGE_OUTPUT_PROPERTY_NAME, PLATESOLVER_MAIN_GROUP, "Image mirror", INDIGO_OK_STATE, 1);
		if (AGENT_PLATESOLVER_IMAGE_OUTPUT_PROPERTY == NULL) {
			return INDIGO_FAILED;
		}
		indigo_init_blob_item(AGENT_PLATESOLVER_IMAGE_OUTPUT_ITEM, AGENT_PLATESOLVER_IMAGE_OUTPUT_ITEM_NAME, "Image");
		// --------------------------------------------------------------------------------
		CONFIG_PROPERTY->hidden = true;
		PROFILE_PROPERTY->hidden = true;
		CONNECTION_PROPERTY->hidden = true;
		// --------------------------------------------------------------------------------
		INDIGO_PLATESOLVER_DEVICE_PRIVATE_DATA->mount_process_state = INDIGO_IDLE_STATE;
		INDIGO_PLATESOLVER_DEVICE_PRIVATE_DATA->guider_process_state = INDIGO_IDLE_STATE;
		INDIGO_PLATESOLVER_DEVICE_PRIVATE_DATA->imager_capture_state = INDIGO_IDLE_STATE;

		pthread_mutex_init(&INDIGO_PLATESOLVER_DEVICE_PRIVATE_DATA->mutex, NULL);
		return INDIGO_OK;
	}
	return INDIGO_FAILED;
}

indigo_result indigo_platesolver_enumerate_properties(indigo_device *device, indigo_client *client, indigo_property *property) {
	INDIGO_DEFINE_MATCHING_PROPERTY(AGENT_PLATESOLVER_USE_INDEX_PROPERTY);
	INDIGO_DEFINE_MATCHING_PROPERTY(AGENT_PLATESOLVER_HINTS_PROPERTY);
	INDIGO_DEFINE_MATCHING_PROPERTY(AGENT_PLATESOLVER_WCS_PROPERTY);
	INDIGO_DEFINE_MATCHING_PROPERTY(AGENT_PLATESOLVER_SYNC_PROPERTY);
	INDIGO_DEFINE_MATCHING_PROPERTY(AGENT_START_PROCESS_PROPERTY);
	INDIGO_DEFINE_MATCHING_PROPERTY(AGENT_ABORT_PROCESS_PROPERTY);
	INDIGO_DEFINE_MATCHING_PROPERTY(AGENT_PLATESOLVER_SOLVE_IMAGES_PROPERTY);
	INDIGO_DEFINE_MATCHING_PROPERTY(AGENT_PLATESOLVER_EXPOSURE_SETTINGS_PROPERTY);
	INDIGO_DEFINE_MATCHING_PROPERTY(AGENT_PLATESOLVER_PA_SETTINGS_PROPERTY);
	INDIGO_DEFINE_MATCHING_PROPERTY(AGENT_PLATESOLVER_GOTO_SETTINGS_PROPERTY);
	INDIGO_DEFINE_MATCHING_PROPERTY(AGENT_PLATESOLVER_MOUNT_SETTLE_TIME_PROPERTY);
	INDIGO_DEFINE_MATCHING_PROPERTY(AGENT_PLATESOLVER_PA_STATE_PROPERTY);
	INDIGO_DEFINE_MATCHING_PROPERTY(AGENT_PLATESOLVER_ABORT_PROPERTY);
	INDIGO_DEFINE_MATCHING_PROPERTY(AGENT_PLATESOLVER_IMAGE_PROPERTY);
	INDIGO_DEFINE_MATCHING_PROPERTY(AGENT_PLATESOLVER_IMAGE_OUTPUT_PROPERTY);
	return indigo_filter_enumerate_properties(device, client, property);
}

indigo_result indigo_platesolver_change_property(indigo_device *device, indigo_client *client, indigo_property *property) {
	assert(device != NULL);
	assert(DEVICE_CONTEXT != NULL);
	assert(property != NULL);
	if (client == FILTER_DEVICE_CONTEXT->client) {
		return INDIGO_OK;
	}
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
		if (AGENT_PLATESOLVER_HINTS_EPOCH_ITEM->number.target != 0 && AGENT_PLATESOLVER_HINTS_EPOCH_ITEM->number.target != 2000) {
			AGENT_PLATESOLVER_HINTS_EPOCH_ITEM->number.value = AGENT_PLATESOLVER_HINTS_EPOCH_ITEM->number.target = 2000;
			indigo_send_message(device, "Warning! Valid values are 0 or 2000 only, value adjusted to 2000");
		}
		AGENT_PLATESOLVER_HINTS_PROPERTY->state = INDIGO_OK_STATE;
		indigo_update_property(device, AGENT_PLATESOLVER_HINTS_PROPERTY, NULL);
		INDIGO_PLATESOLVER_DEVICE_PRIVATE_DATA->save_config(device);
		return INDIGO_OK;
	} else if (indigo_property_match(AGENT_PLATESOLVER_EXPOSURE_SETTINGS_PROPERTY, property)) {
	// -------------------------------------------------------------------------------- AGENT_PLATESOLVER_EXPOSURE_SETTINGS
		indigo_property_copy_values(AGENT_PLATESOLVER_EXPOSURE_SETTINGS_PROPERTY, property, false);
		AGENT_PLATESOLVER_EXPOSURE_SETTINGS_PROPERTY->state = INDIGO_OK_STATE;
		AGENT_PLATESOLVER_PA_SETTINGS_PROPERTY->state = INDIGO_OK_STATE;
		AGENT_PLATESOLVER_PA_SETTINGS_EXPOSURE_ITEM->number.value =
		AGENT_PLATESOLVER_PA_SETTINGS_EXPOSURE_ITEM->number.target =
		AGENT_PLATESOLVER_EXPOSURE_SETTINGS_EXPOSURE_ITEM->number.target;
		indigo_update_property(device, AGENT_PLATESOLVER_PA_SETTINGS_PROPERTY, NULL);
		indigo_update_property(device, AGENT_PLATESOLVER_EXPOSURE_SETTINGS_PROPERTY, NULL);
		INDIGO_PLATESOLVER_DEVICE_PRIVATE_DATA->save_config(device);
		return INDIGO_OK;
	} else if (indigo_property_match(AGENT_PLATESOLVER_PA_SETTINGS_PROPERTY, property)) {
	// -------------------------------------------------------------------------------- AGENT_PLATESOLVER_PA_SETTINGS
		indigo_property_copy_values(AGENT_PLATESOLVER_PA_SETTINGS_PROPERTY, property, false);
		AGENT_PLATESOLVER_PA_SETTINGS_PROPERTY->state = INDIGO_OK_STATE;
		AGENT_PLATESOLVER_EXPOSURE_SETTINGS_PROPERTY->state = INDIGO_OK_STATE;
		AGENT_PLATESOLVER_EXPOSURE_SETTINGS_EXPOSURE_ITEM->number.value =
		AGENT_PLATESOLVER_EXPOSURE_SETTINGS_EXPOSURE_ITEM->number.target =
		AGENT_PLATESOLVER_PA_SETTINGS_EXPOSURE_ITEM->number.target;
		indigo_update_property(device, AGENT_PLATESOLVER_EXPOSURE_SETTINGS_PROPERTY, NULL);
		indigo_update_property(device, AGENT_PLATESOLVER_PA_SETTINGS_PROPERTY, NULL);
		INDIGO_PLATESOLVER_DEVICE_PRIVATE_DATA->save_config(device);
		return INDIGO_OK;
	} else if (indigo_property_match(AGENT_PLATESOLVER_GOTO_SETTINGS_PROPERTY, property)) {
	// -------------------------------------------------------------------------------- AGENT_PLATESOLVER_GOTO_SETTINGS
		indigo_property_copy_values(AGENT_PLATESOLVER_GOTO_SETTINGS_PROPERTY, property, false);
		AGENT_PLATESOLVER_PA_SETTINGS_PROPERTY->state = INDIGO_OK_STATE;
		indigo_update_property(device, AGENT_PLATESOLVER_GOTO_SETTINGS_PROPERTY, NULL);
		return INDIGO_OK;
	} else if (indigo_property_match(AGENT_PLATESOLVER_MOUNT_SETTLE_TIME_PROPERTY, property)) {
	// -------------------------------------------------------------------------------- AGENT_PLATESOLVER_MOUNT_SETTLE_TIME
		indigo_property_copy_values(AGENT_PLATESOLVER_MOUNT_SETTLE_TIME_PROPERTY, property, false);
		AGENT_PLATESOLVER_MOUNT_SETTLE_TIME_PROPERTY->state = INDIGO_OK_STATE;
		indigo_update_property(device, AGENT_PLATESOLVER_MOUNT_SETTLE_TIME_PROPERTY, NULL);
		INDIGO_PLATESOLVER_DEVICE_PRIVATE_DATA->save_config(device);
		return INDIGO_OK;
	} else if (indigo_property_match(AGENT_PLATESOLVER_SYNC_PROPERTY, property)) {
	// -------------------------------------------------------------------------------- AGENT_PLATESOLVER_SYNC
		indigo_property_copy_values(AGENT_PLATESOLVER_SYNC_PROPERTY, property, false);
		AGENT_PLATESOLVER_SYNC_PROPERTY->state = INDIGO_OK_STATE;
		indigo_update_property(device, AGENT_PLATESOLVER_SYNC_PROPERTY, NULL);
		INDIGO_PLATESOLVER_DEVICE_PRIVATE_DATA->save_config(device);
		return INDIGO_OK;
	} else if (indigo_property_match(AGENT_PLATESOLVER_SOLVE_IMAGES_PROPERTY, property)) {
	// -------------------------------------------------------------------------------- AGENT_PLATESOLVER_SOLVE_IMAGES
		indigo_property_copy_values(AGENT_PLATESOLVER_SOLVE_IMAGES_PROPERTY, property, false);
		AGENT_PLATESOLVER_SOLVE_IMAGES_PROPERTY->state = INDIGO_OK_STATE;
		indigo_update_property(device, AGENT_PLATESOLVER_SOLVE_IMAGES_PROPERTY, NULL);
		//INDIGO_PLATESOLVER_DEVICE_PRIVATE_DATA->save_config(device);
		return INDIGO_OK;
	} else if (indigo_property_match(AGENT_START_PROCESS_PROPERTY, property)) {
	// -------------------------------------------------------------------------------- AGENT_START_PROCESS
		indigo_property_copy_values(AGENT_START_PROCESS_PROPERTY, property, false);
		if (AGENT_START_PROCESS_PROPERTY->state != INDIGO_BUSY_STATE && AGENT_PLATESOLVER_WCS_PROPERTY->state != INDIGO_BUSY_STATE) {
			indigo_property_copy_values(AGENT_START_PROCESS_PROPERTY, property, false);
			AGENT_START_PROCESS_PROPERTY->state = INDIGO_BUSY_STATE;
			indigo_update_property(device, AGENT_START_PROCESS_PROPERTY, NULL);
			indigo_set_timer(device, 0, start_process, NULL);
		}
		return INDIGO_OK;
	} else if (indigo_property_match(AGENT_ABORT_PROCESS_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- AGENT_ABORT_PROCESS
		indigo_property_copy_values(AGENT_ABORT_PROCESS_PROPERTY, property, false);
		if (AGENT_ABORT_PROCESS_ITEM->sw.value) {
			indigo_async((void *(*)(void *))abort_process, device);
			AGENT_ABORT_PROCESS_ITEM->sw.value = false;
			AGENT_ABORT_PROCESS_PROPERTY->state = INDIGO_OK_STATE;
			indigo_update_property(device, AGENT_ABORT_PROCESS_PROPERTY, NULL);
		}
	} else if (indigo_property_match(AGENT_PLATESOLVER_IMAGE_PROPERTY, property)) {
	// -------------------------------------------------------------------------------- AGENT_PLATESOLVER_IMAGE
		indigo_property_copy_values(AGENT_PLATESOLVER_IMAGE_PROPERTY, property, false);
		if (AGENT_PLATESOLVER_IMAGE_ITEM->blob.size > 0 && AGENT_PLATESOLVER_IMAGE_ITEM->blob.value) {
			indigo_platesolver_task *task = indigo_safe_malloc(sizeof(indigo_platesolver_task));
			task->device = device;
			INDIGO_COPY_VALUE(task->image_url, AGENT_PLATESOLVER_IMAGE_ITEM->blob.url);
			if (AGENT_PLATESOLVER_IMAGE_ITEM->blob.value != NULL) {
				task->image = indigo_safe_malloc_copy(task->size = AGENT_PLATESOLVER_IMAGE_ITEM->blob.size, AGENT_PLATESOLVER_IMAGE_ITEM->blob.value);
				INDIGO_COPY_NAME(task->format, AGENT_PLATESOLVER_IMAGE_ITEM->blob.format);
			} else {
				task->image = NULL;
			}
			// uploaded files should not use camera pixel scale
			INDIGO_PLATESOLVER_DEVICE_PRIVATE_DATA->pixel_scale = 0;
			indigo_async((void *(*)(void *))solve, task);
			AGENT_PLATESOLVER_IMAGE_PROPERTY->state = INDIGO_OK_STATE;
		} else {
			AGENT_PLATESOLVER_IMAGE_PROPERTY->state = INDIGO_ALERT_STATE;
		}
		indigo_update_property(device, AGENT_PLATESOLVER_IMAGE_PROPERTY, NULL);
		return INDIGO_OK;
	} else if (indigo_property_match(AGENT_PLATESOLVER_ABORT_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- AGENT_PLATESOLVER_ABORT
		indigo_property_copy_values(AGENT_PLATESOLVER_ABORT_PROPERTY, property, false);
		if (AGENT_PLATESOLVER_ABORT_ITEM->sw.value) {
			indigo_async((void *(*)(void *))abort_process, device);
			AGENT_PLATESOLVER_ABORT_ITEM->sw.value = false;
			AGENT_PLATESOLVER_ABORT_PROPERTY->state = INDIGO_OK_STATE;
			indigo_update_property(device, AGENT_PLATESOLVER_ABORT_PROPERTY, NULL);
		}
	}
	return indigo_filter_change_property(device, client, property);
}

indigo_result indigo_platesolver_device_detach(indigo_device *device) {
	assert(device != NULL);
	indigo_release_property(AGENT_PLATESOLVER_USE_INDEX_PROPERTY);
	indigo_release_property(AGENT_PLATESOLVER_HINTS_PROPERTY);
	indigo_release_property(AGENT_PLATESOLVER_WCS_PROPERTY);
	indigo_release_property(AGENT_PLATESOLVER_SYNC_PROPERTY);
	indigo_release_property(AGENT_START_PROCESS_PROPERTY);
	indigo_release_property(AGENT_ABORT_PROCESS_PROPERTY);
	indigo_release_property(AGENT_PLATESOLVER_SOLVE_IMAGES_PROPERTY);
	indigo_release_property(AGENT_PLATESOLVER_EXPOSURE_SETTINGS_PROPERTY);
	indigo_release_property(AGENT_PLATESOLVER_PA_SETTINGS_PROPERTY);
	indigo_release_property(AGENT_PLATESOLVER_PA_STATE_PROPERTY);
	indigo_release_property(AGENT_PLATESOLVER_GOTO_SETTINGS_PROPERTY);
	indigo_release_property(AGENT_PLATESOLVER_MOUNT_SETTLE_TIME_PROPERTY);
	indigo_release_property(AGENT_PLATESOLVER_ABORT_PROPERTY);
	indigo_release_property(AGENT_PLATESOLVER_IMAGE_PROPERTY);
	indigo_release_property(AGENT_PLATESOLVER_IMAGE_OUTPUT_PROPERTY);
	pthread_mutex_destroy(&INDIGO_PLATESOLVER_DEVICE_PRIVATE_DATA->mutex);
	return indigo_filter_device_detach(device);
}

// -------------------------------------------------------------------------------- INDIGO agent client implementation

static void indigo_platesolver_handle_property(indigo_client *client, indigo_device *device, indigo_property *property, const char *message) {
	char *related_agent_name = indigo_filter_first_related_agent(FILTER_CLIENT_CONTEXT->device, "Mount Agent");
	if (related_agent_name && !strcmp(related_agent_name, property->device)) {
		indigo_device *device = FILTER_CLIENT_CONTEXT->device;
		if (!strcmp(property->name, MOUNT_EQUATORIAL_COORDINATES_PROPERTY_NAME)) {
			bool update = false;
			double ra = NAN, dec = NAN;
			if (property->state == INDIGO_BUSY_STATE) {
				/* if mount is moved and polar alignment is in progress stop polar alignment and invalidate values */
				reset_pa_state(device, false);
			}
			if (property->state == INDIGO_OK_STATE || property->state == INDIGO_BUSY_STATE) {
				for (int i = 0; i < property->count; i++) {
					indigo_item *item = property->items + i;
					if (!strcmp(item->name, MOUNT_EQUATORIAL_COORDINATES_RA_ITEM_NAME)) {
						ra = item->number.value;
						INDIGO_PLATESOLVER_CLIENT_PRIVATE_DATA->eq_coordinates.a = 15 * DEG2RAD * ra;
						if (AGENT_PLATESOLVER_HINTS_RA_ITEM->number.value != ra) {
							AGENT_PLATESOLVER_HINTS_RA_ITEM->number.value = AGENT_PLATESOLVER_HINTS_RA_ITEM->number.target = ra;
							update = true;
						}
					} else if (!strcmp(item->name, MOUNT_EQUATORIAL_COORDINATES_DEC_ITEM_NAME)) {
						dec = item->number.value;
						INDIGO_PLATESOLVER_CLIENT_PRIVATE_DATA->eq_coordinates.d = DEG2RAD * dec;
						if (AGENT_PLATESOLVER_HINTS_DEC_ITEM->number.value != dec) {
							AGENT_PLATESOLVER_HINTS_DEC_ITEM->number.value = AGENT_PLATESOLVER_HINTS_DEC_ITEM->number.target = dec;
							update = true;
						}
					}
				}
				if (update) {
					AGENT_PLATESOLVER_HINTS_PROPERTY->state = INDIGO_OK_STATE;
					indigo_update_property(device, AGENT_PLATESOLVER_HINTS_PROPERTY, NULL);
				}
			}
		} else if (!strcmp(property->name, MOUNT_GEOGRAPHIC_COORDINATES_PROPERTY_NAME)) {
			//INDIGO_PLATESOLVER_CLIENT_PRIVATE_DATA->geo_coordinates_state = property->state;
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
		} else if (!strcmp(property->name, AGENT_START_PROCESS_PROPERTY_NAME)) {
			INDIGO_PLATESOLVER_CLIENT_PRIVATE_DATA->mount_process_state = property->state;
		}
	}
	related_agent_name = indigo_filter_first_related_agent(FILTER_CLIENT_CONTEXT->device, "Imager Agent");
	if (related_agent_name && !strcmp(related_agent_name, property->device)) {
		if (!strcmp(property->name, AGENT_IMAGER_CAPTURE_PROPERTY_NAME)) {
			INDIGO_PLATESOLVER_CLIENT_PRIVATE_DATA->imager_capture_state = property->state;
		}
	}
	related_agent_name = indigo_filter_first_related_agent(FILTER_CLIENT_CONTEXT->device, "Guider Agent");
	if (related_agent_name && !strcmp(related_agent_name, property->device)) {
		if (!strcmp(property->name, AGENT_START_PROCESS_PROPERTY_NAME)) {
			INDIGO_PLATESOLVER_CLIENT_PRIVATE_DATA->guider_process_state = property->state;
		}
	}
	related_agent_name = indigo_filter_first_related_agent_2(FILTER_CLIENT_CONTEXT->device, "Imager Agent", "Guider Agent");
	if (related_agent_name && !strcmp(related_agent_name, property->device)) {
		indigo_device *device = FILTER_CLIENT_CONTEXT->device;
		if (!strcmp(property->name, CCD_LENS_FOV_PROPERTY_NAME)) {
			INDIGO_DRIVER_DEBUG("SOLVER", "%s.%s: state %d", property->device, property->name, property->state);
			if (property->state == INDIGO_OK_STATE) {
				indigo_item *item = indigo_get_item(property, CCD_LENS_FOV_PIXEL_SCALE_HEIGHT_ITEM_NAME);
				if (item) {
					INDIGO_PLATESOLVER_DEVICE_PRIVATE_DATA->pixel_scale = item->number.value;
					INDIGO_DRIVER_DEBUG("SOLVER", "%s.%s: pixel_scale = %f", property->device, property->name, INDIGO_PLATESOLVER_DEVICE_PRIVATE_DATA->pixel_scale);
				}
			} else {
				INDIGO_PLATESOLVER_DEVICE_PRIVATE_DATA->pixel_scale = 0;
				INDIGO_DRIVER_DEBUG("SOLVER", "%s.%s not in OK state, pixel_scale = %f", property->device, property->name, INDIGO_PLATESOLVER_DEVICE_PRIVATE_DATA->pixel_scale);
			}
		}
	}
}

void handle_polar_align_failure(indigo_device *device) {
	if ((AGENT_PLATESOLVER_SYNC_CALCULATE_PA_ERROR_ITEM->sw.value || AGENT_PLATESOLVER_SYNC_RECALCULATE_PA_ERROR_ITEM->sw.value) && AGENT_PLATESOLVER_PA_STATE_ITEM->number.value != INDIGO_POLAR_ALIGN_IDLE && AGENT_PLATESOLVER_PA_STATE_ITEM->number.value != INDIGO_POLAR_ALIGN_IN_PROGRESS) {
		INDIGO_DRIVER_DEBUG("SOLVER", "Exposure failed in AGENT_PLATESOLVER_PA_STATE = %d", (int)AGENT_PLATESOLVER_PA_STATE_ITEM->number.value);
		AGENT_PLATESOLVER_PA_STATE_PROPERTY->state = INDIGO_ALERT_STATE;
		AGENT_PLATESOLVER_PA_STATE_ITEM->number.value = INDIGO_POLAR_ALIGN_IDLE;
		indigo_update_property(device, AGENT_PLATESOLVER_PA_STATE_PROPERTY, NULL);
		process_failed(device, "Polar alignment failed");
	} else {
		process_failed(device, "Image capture failed");
	}
}

indigo_result indigo_platesolver_update_property(indigo_client *client, indigo_device *device, indigo_property *property, const char *message) {
	char *related_agent_name = indigo_filter_first_related_agent_2(FILTER_CLIENT_CONTEXT->device, "Imager Agent", "Guider Agent");
	if (related_agent_name && !strcmp(related_agent_name, property->device)) {
		indigo_device *device = FILTER_CLIENT_CONTEXT->device;
		if (!strcmp(property->name, CCD_IMAGE_PROPERTY_NAME)) {
			if ((AGENT_START_PROCESS_PROPERTY->state == INDIGO_BUSY_STATE || AGENT_PLATESOLVER_SOLVE_IMAGES_ENABLED_ITEM->sw.value)) {
				if (property->state == INDIGO_OK_STATE) {
					for (int i = 0; i < property->count; i++) {
						indigo_item *item = property->items + i;
						if (!strcmp(item->name, CCD_IMAGE_ITEM_NAME)) {
							indigo_platesolver_task *task = indigo_safe_malloc(sizeof(indigo_platesolver_task));
							task->device = FILTER_CLIENT_CONTEXT->device;
							INDIGO_COPY_VALUE(task->image_url, item->blob.url);
							if (item->blob.value != NULL) {
								task->image = indigo_safe_malloc_copy(task->size = item->blob.size, item->blob.value);
								INDIGO_COPY_NAME(task->format, item->blob.format);
							} else {
								task->image = NULL;
							}
							indigo_async((void *(*)(void *))solve, task);
						}
					}
				} else if (property->state == INDIGO_BUSY_STATE) {
					if ((AGENT_PLATESOLVER_SYNC_CALCULATE_PA_ERROR_ITEM->sw.value || AGENT_PLATESOLVER_SYNC_RECALCULATE_PA_ERROR_ITEM->sw.value) && (AGENT_PLATESOLVER_PA_STATE_ITEM->number.value == INDIGO_POLAR_ALIGN_IDLE || AGENT_PLATESOLVER_PA_STATE_ITEM->number.value == INDIGO_POLAR_ALIGN_IN_PROGRESS)) {
						if (AGENT_PLATESOLVER_SYNC_CALCULATE_PA_ERROR_ITEM->sw.value) {
							if (AGENT_PLATESOLVER_PA_STATE_ITEM->number.value == INDIGO_POLAR_ALIGN_IDLE || AGENT_PLATESOLVER_PA_STATE_ITEM->number.value == INDIGO_POLAR_ALIGN_IN_PROGRESS) {
								INDIGO_DRIVER_DEBUG("SOLVER", "state INDIGO_POLAR_ALIGN_IDLE -> INDIGO_POLAR_ALIGN_REFERENCE_1");
								AGENT_PLATESOLVER_PA_STATE_PROPERTY->state = INDIGO_BUSY_STATE;
								AGENT_PLATESOLVER_PA_STATE_ITEM->number.value = INDIGO_POLAR_ALIGN_REFERENCE_1;
								indigo_update_property(device, AGENT_PLATESOLVER_PA_STATE_PROPERTY, NULL);
							}
						} else if (AGENT_PLATESOLVER_SYNC_RECALCULATE_PA_ERROR_ITEM->sw.value) {
							if (AGENT_PLATESOLVER_PA_STATE_ITEM->number.value == INDIGO_POLAR_ALIGN_IN_PROGRESS) {
								INDIGO_DRIVER_DEBUG("SOLVER", "state INDIGO_POLAR_ALIGN_IN_PROGRESS -> INDIGO_POLAR_ALIGN_RECALCULATE");
								AGENT_PLATESOLVER_PA_STATE_PROPERTY->state = INDIGO_BUSY_STATE;
								AGENT_PLATESOLVER_PA_STATE_ITEM->number.value = INDIGO_POLAR_ALIGN_RECALCULATE;
								indigo_update_property(device, AGENT_PLATESOLVER_PA_STATE_PROPERTY, NULL);
							} else {
								INDIGO_DRIVER_DEBUG("SOLVER", "can not transit to INDIGO_POLAR_ALIGN_RECALCULATE from the current state (%d)", (int)AGENT_PLATESOLVER_PA_STATE_ITEM->number.value);
								AGENT_PLATESOLVER_PA_STATE_PROPERTY->state = INDIGO_ALERT_STATE;
								AGENT_PLATESOLVER_PA_STATE_ITEM->number.value = INDIGO_POLAR_ALIGN_IDLE;
								indigo_update_property(device, AGENT_PLATESOLVER_PA_STATE_PROPERTY, NULL);
								abort_exposure(device);
								process_failed(device, "Polar alignment is not in progress");
							}
						}
					} else if (AGENT_PLATESOLVER_WCS_PROPERTY->state != INDIGO_BUSY_STATE) {
						AGENT_PLATESOLVER_WCS_PROPERTY->state = INDIGO_BUSY_STATE;
						AGENT_PLATESOLVER_WCS_STATE_ITEM->number.value = INDIGO_SOLVER_STATE_WAITING_FOR_IMAGE;
						indigo_update_property(device, AGENT_PLATESOLVER_WCS_PROPERTY, NULL);
					}
				} else if (property->state == INDIGO_ALERT_STATE) {
					handle_polar_align_failure(device);
				}
			}
		} else if (!strcmp(property->name, CCD_EXPOSURE_PROPERTY_NAME)) {
			if (AGENT_PLATESOLVER_SOLVE_IMAGES_ENABLED_ITEM->sw.value) {
				INDIGO_DRIVER_DEBUG("SOLVER", "%s.%s: state %d", related_agent_name, property->name, property->state);
				if (property->state == INDIGO_ALERT_STATE) {
					handle_polar_align_failure(device);
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
