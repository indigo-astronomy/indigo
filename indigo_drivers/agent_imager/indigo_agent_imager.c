// Copyright (c) 2018 CloudMakers, s. r. o.
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

/** INDIGO Imager agent
 \file indigo_agent_imager.c
 */

#define DRIVER_VERSION 0x0032
#define DRIVER_NAME	"indigo_agent_imager"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <assert.h>
#include <pthread.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/stat.h>

#include <indigo/indigo_driver_xml.h>
#include <indigo/indigo_filter.h>
#include <indigo/indigo_ccd_driver.h>
#include <indigo/indigo_io.h>
#include <indigo/indigo_raw_utils.h>
#include <indigo/indigo_align.h>
#include <indigo/indigo_polynomial_fit.h>

#include "indigo_agent_imager.h"

#define DEVICE_PRIVATE_DATA										((imager_agent_private_data *)device->private_data)
#define CLIENT_PRIVATE_DATA										((imager_agent_private_data *)FILTER_CLIENT_CONTEXT->device->private_data)

#define AGENT_IMAGER_BATCH_PROPERTY						(DEVICE_PRIVATE_DATA->agent_imager_batch_property)
#define AGENT_IMAGER_BATCH_COUNT_ITEM    			(AGENT_IMAGER_BATCH_PROPERTY->items+0)
#define AGENT_IMAGER_BATCH_EXPOSURE_ITEM  		(AGENT_IMAGER_BATCH_PROPERTY->items+1)
#define AGENT_IMAGER_BATCH_DELAY_ITEM     		(AGENT_IMAGER_BATCH_PROPERTY->items+2)
#define AGENT_IMAGER_BATCH_FRAMES_TO_SKIP_BEFORE_DITHER_ITEM	(AGENT_IMAGER_BATCH_PROPERTY->items+3)
#define AGENT_IMAGER_BATCH_PAUSE_AFTER_TRANSIT_ITEM     	(AGENT_IMAGER_BATCH_PROPERTY->items+4)

#define AGENT_IMAGER_FOCUS_PROPERTY									(DEVICE_PRIVATE_DATA->agent_imager_focus_property)
#define AGENT_IMAGER_FOCUS_INITIAL_ITEM    					(AGENT_IMAGER_FOCUS_PROPERTY->items+0) // obsolete
#define AGENT_IMAGER_FOCUS_FINAL_ITEM  							(AGENT_IMAGER_FOCUS_PROPERTY->items+1) // obsolete
#define AGENT_IMAGER_FOCUS_ITERATIVE_INITIAL_ITEM 	(AGENT_IMAGER_FOCUS_PROPERTY->items+2)
#define AGENT_IMAGER_FOCUS_ITERATIVE_FINAL_ITEM  		(AGENT_IMAGER_FOCUS_PROPERTY->items+3)
#define AGENT_IMAGER_FOCUS_UCURVE_SAMPLES_ITEM  		(AGENT_IMAGER_FOCUS_PROPERTY->items+4)
#define AGENT_IMAGER_FOCUS_UCURVE_STEP_ITEM  				(AGENT_IMAGER_FOCUS_PROPERTY->items+5)
#define AGENT_IMAGER_FOCUS_BAHTINOV_SIGMA_ITEM  		(AGENT_IMAGER_FOCUS_PROPERTY->items+6)
#define AGENT_IMAGER_FOCUS_BRACKETING_STEP_ITEM  		(AGENT_IMAGER_FOCUS_PROPERTY->items+7)
#define AGENT_IMAGER_FOCUS_BACKLASH_ITEM     				(AGENT_IMAGER_FOCUS_PROPERTY->items+8)
#define AGENT_IMAGER_FOCUS_BACKLASH_OVERSHOOT_ITEM  (AGENT_IMAGER_FOCUS_PROPERTY->items+9)
#define AGENT_IMAGER_FOCUS_STACK_ITEM								(AGENT_IMAGER_FOCUS_PROPERTY->items+10)
#define AGENT_IMAGER_FOCUS_REPEAT_ITEM							(AGENT_IMAGER_FOCUS_PROPERTY->items+11)
#define AGENT_IMAGER_FOCUS_DELAY_ITEM								(AGENT_IMAGER_FOCUS_PROPERTY->items+12)

#define AGENT_IMAGER_FOCUS_FAILURE_PROPERTY		(DEVICE_PRIVATE_DATA->agent_imager_focus_failure_property)
#define AGENT_IMAGER_FOCUS_FAILURE_STOP_ITEM  (AGENT_IMAGER_FOCUS_FAILURE_PROPERTY->items+0)
#define AGENT_IMAGER_FOCUS_FAILURE_RESTORE_ITEM  (AGENT_IMAGER_FOCUS_FAILURE_PROPERTY->items+1)

#define AGENT_IMAGER_FOCUS_ESTIMATOR_PROPERTY		(DEVICE_PRIVATE_DATA->agent_imager_focus_estimator_property)
#define AGENT_IMAGER_FOCUS_ESTIMATOR_UCURVE_ITEM  (AGENT_IMAGER_FOCUS_ESTIMATOR_PROPERTY->items+0)
#define AGENT_IMAGER_FOCUS_ESTIMATOR_HFD_PEAK_ITEM  (AGENT_IMAGER_FOCUS_ESTIMATOR_PROPERTY->items+1)
#define AGENT_IMAGER_FOCUS_ESTIMATOR_RMS_CONTRAST_ITEM  (AGENT_IMAGER_FOCUS_ESTIMATOR_PROPERTY->items+2)
#define AGENT_IMAGER_FOCUS_ESTIMATOR_BAHTINOV_ITEM  (AGENT_IMAGER_FOCUS_ESTIMATOR_PROPERTY->items+3)

#define AGENT_IMAGER_DOWNLOAD_FILE_PROPERTY		(DEVICE_PRIVATE_DATA->agent_imager_download_file_property)
#define AGENT_IMAGER_DOWNLOAD_FILE_ITEM    		(AGENT_IMAGER_DOWNLOAD_FILE_PROPERTY->items+0)

#define AGENT_IMAGER_DOWNLOAD_FILES_PROPERTY	(DEVICE_PRIVATE_DATA->agent_imager_download_files_property)
#define AGENT_IMAGER_DOWNLOAD_FILES_REFRESH_ITEM    (AGENT_IMAGER_DOWNLOAD_FILES_PROPERTY->items+0)
#define DOWNLOAD_MAX_COUNT										(INDIGO_MAX_ITEMS - 1)

#define AGENT_IMAGER_DOWNLOAD_IMAGE_PROPERTY	(DEVICE_PRIVATE_DATA->agent_imager_download_image_property)
#define AGENT_IMAGER_DOWNLOAD_IMAGE_ITEM    	(AGENT_IMAGER_DOWNLOAD_IMAGE_PROPERTY->items+0)

#define AGENT_IMAGER_DELETE_FILE_PROPERTY			(DEVICE_PRIVATE_DATA->agent_imager_delete_file_property)
#define AGENT_IMAGER_DELETE_FILE_ITEM    			(AGENT_IMAGER_DELETE_FILE_PROPERTY->items+0)

#define AGENT_IMAGER_CAPTURE_PROPERTY					(DEVICE_PRIVATE_DATA->agent_imager_capture_property)
#define AGENT_IMAGER_CAPTURE_ITEM  						(AGENT_IMAGER_CAPTURE_PROPERTY->items+0)

#define AGENT_START_PROCESS_PROPERTY					(DEVICE_PRIVATE_DATA->agent_start_process_property)
#define AGENT_IMAGER_START_PREVIEW_1_ITEM  		(AGENT_START_PROCESS_PROPERTY->items+0)
#define AGENT_IMAGER_START_PREVIEW_ITEM  			(AGENT_START_PROCESS_PROPERTY->items+1)
#define AGENT_IMAGER_START_EXPOSURE_ITEM  		(AGENT_START_PROCESS_PROPERTY->items+2)
#define AGENT_IMAGER_START_STREAMING_ITEM 		(AGENT_START_PROCESS_PROPERTY->items+3)
#define AGENT_IMAGER_START_FOCUSING_ITEM 			(AGENT_START_PROCESS_PROPERTY->items+4)
#define AGENT_IMAGER_START_SEQUENCE_ITEM 			(AGENT_START_PROCESS_PROPERTY->items+5)
#define AGENT_IMAGER_CLEAR_SELECTION_ITEM			(AGENT_START_PROCESS_PROPERTY->items+6)

#define AGENT_PAUSE_PROCESS_PROPERTY					(DEVICE_PRIVATE_DATA->agent_pause_process_property)
#define AGENT_PAUSE_PROCESS_ITEM      				(AGENT_PAUSE_PROCESS_PROPERTY->items+0)
#define AGENT_PAUSE_PROCESS_WAIT_ITEM      		(AGENT_PAUSE_PROCESS_PROPERTY->items+1)
#define AGENT_PAUSE_PROCESS_AFTER_TRANSIT_ITEM      	(AGENT_PAUSE_PROCESS_PROPERTY->items+2)

#define AGENT_ABORT_PROCESS_PROPERTY					(DEVICE_PRIVATE_DATA->agent_abort_process_property)
#define AGENT_ABORT_PROCESS_ITEM      				(AGENT_ABORT_PROCESS_PROPERTY->items+0)

#define AGENT_PROCESS_FEATURES_PROPERTY				(DEVICE_PRIVATE_DATA->agent_process_features_property)
#define AGENT_IMAGER_ENABLE_DITHERING_FEATURE_ITEM	(AGENT_PROCESS_FEATURES_PROPERTY->items+0)
#define AGENT_IMAGER_DITHER_AFTER_BATCH_FEATURE_ITEM	(AGENT_PROCESS_FEATURES_PROPERTY->items+1)
#define AGENT_IMAGER_PAUSE_AFTER_TRANSIT_FEATURE_ITEM		(AGENT_PROCESS_FEATURES_PROPERTY->items+2)
#define AGENT_IMAGER_MACRO_MODE_FEATURE_ITEM	(AGENT_PROCESS_FEATURES_PROPERTY->items+3)

#define AGENT_WHEEL_FILTER_PROPERTY						(DEVICE_PRIVATE_DATA->agent_wheel_filter_property)
#define FILTER_SLOT_COUNT											24

#define AGENT_FOCUSER_CONTROL_PROPERTY				(DEVICE_PRIVATE_DATA->agent_focuser_control_property)
#define AGENT_FOCUSER_FOCUS_IN_ITEM      			(AGENT_FOCUSER_CONTROL_PROPERTY->items+0)
#define AGENT_FOCUSER_FOCUS_OUT_ITEM      		(AGENT_FOCUSER_CONTROL_PROPERTY->items+1)

#define AGENT_IMAGER_STATS_PROPERTY						(DEVICE_PRIVATE_DATA->agent_stats_property)
#define AGENT_IMAGER_STATS_EXPOSURE_ITEM      (AGENT_IMAGER_STATS_PROPERTY->items+0)
#define AGENT_IMAGER_STATS_DELAY_ITEM      		(AGENT_IMAGER_STATS_PROPERTY->items+1)
#define AGENT_IMAGER_STATS_FRAME_ITEM      		(AGENT_IMAGER_STATS_PROPERTY->items+2)
#define AGENT_IMAGER_STATS_FRAMES_ITEM      	(AGENT_IMAGER_STATS_PROPERTY->items+3)
#define AGENT_IMAGER_STATS_BATCH_INDEX_ITEM   (AGENT_IMAGER_STATS_PROPERTY->items+4)
#define AGENT_IMAGER_STATS_BATCH_ITEM      		(AGENT_IMAGER_STATS_PROPERTY->items+5)
#define AGENT_IMAGER_STATS_BATCHES_ITEM      	(AGENT_IMAGER_STATS_PROPERTY->items+6)
#define AGENT_IMAGER_STATS_PHASE_ITEM  				(AGENT_IMAGER_STATS_PROPERTY->items+7)
#define AGENT_IMAGER_STATS_DRIFT_X_ITEM      	(AGENT_IMAGER_STATS_PROPERTY->items+8)
#define AGENT_IMAGER_STATS_DRIFT_Y_ITEM      	(AGENT_IMAGER_STATS_PROPERTY->items+9)
#define AGENT_IMAGER_STATS_DITHERING_ITEM     (AGENT_IMAGER_STATS_PROPERTY->items+10)
#define AGENT_IMAGER_STATS_FOCUS_OFFSET_ITEM     		(AGENT_IMAGER_STATS_PROPERTY->items+11)
#define AGENT_IMAGER_STATS_FOCUS_POSITION_ITEM     		(AGENT_IMAGER_STATS_PROPERTY->items+12)
#define AGENT_IMAGER_STATS_RMS_CONTRAST_ITEM     		(AGENT_IMAGER_STATS_PROPERTY->items+13)
#define AGENT_IMAGER_STATS_FOCUS_DEVIATION_ITEM			(AGENT_IMAGER_STATS_PROPERTY->items+14)
#define AGENT_IMAGER_STATS_FRAMES_TO_DITHERING_ITEM (AGENT_IMAGER_STATS_PROPERTY->items+15)
#define AGENT_IMAGER_STATS_BAHTINOV_ITEM      (AGENT_IMAGER_STATS_PROPERTY->items+16)
#define AGENT_IMAGER_STATS_MAX_STARS_TO_USE_ITEM      		(AGENT_IMAGER_STATS_PROPERTY->items+17)
#define AGENT_IMAGER_STATS_PEAK_ITEM      		(AGENT_IMAGER_STATS_PROPERTY->items+18)
#define AGENT_IMAGER_STATS_FWHM_ITEM      		(AGENT_IMAGER_STATS_PROPERTY->items+19)
#define AGENT_IMAGER_STATS_HFD_ITEM      			(AGENT_IMAGER_STATS_PROPERTY->items+20)

#define MAX_STAR_COUNT												50
#define AGENT_IMAGER_STARS_PROPERTY						(DEVICE_PRIVATE_DATA->agent_stars_property)
#define AGENT_IMAGER_STARS_REFRESH_ITEM  			(AGENT_IMAGER_STARS_PROPERTY->items+0)

#define AGENT_IMAGER_SELECTION_PROPERTY							(DEVICE_PRIVATE_DATA->agent_selection_property)
#define AGENT_IMAGER_SELECTION_RADIUS_ITEM  				(AGENT_IMAGER_SELECTION_PROPERTY->items+0)
#define AGENT_IMAGER_SELECTION_SUBFRAME_ITEM				(AGENT_IMAGER_SELECTION_PROPERTY->items+1)
#define AGENT_IMAGER_SELECTION_INCLUDE_LEFT_ITEM		(AGENT_IMAGER_SELECTION_PROPERTY->items+2)
#define AGENT_IMAGER_SELECTION_INCLUDE_TOP_ITEM			(AGENT_IMAGER_SELECTION_PROPERTY->items+3)
#define AGENT_IMAGER_SELECTION_INCLUDE_WIDTH_ITEM		(AGENT_IMAGER_SELECTION_PROPERTY->items+4)
#define AGENT_IMAGER_SELECTION_INCLUDE_HEIGHT_ITEM	(AGENT_IMAGER_SELECTION_PROPERTY->items+5)
#define AGENT_IMAGER_SELECTION_EXCLUDE_LEFT_ITEM		(AGENT_IMAGER_SELECTION_PROPERTY->items+6)
#define AGENT_IMAGER_SELECTION_EXCLUDE_TOP_ITEM			(AGENT_IMAGER_SELECTION_PROPERTY->items+7)
#define AGENT_IMAGER_SELECTION_EXCLUDE_WIDTH_ITEM		(AGENT_IMAGER_SELECTION_PROPERTY->items+8)
#define AGENT_IMAGER_SELECTION_EXCLUDE_HEIGHT_ITEM	(AGENT_IMAGER_SELECTION_PROPERTY->items+9)
#define AGENT_IMAGER_SELECTION_STAR_COUNT_ITEM 			(AGENT_IMAGER_SELECTION_PROPERTY->items+10)
#define AGENT_IMAGER_SELECTION_X_ITEM  							(AGENT_IMAGER_SELECTION_PROPERTY->items+11)
#define AGENT_IMAGER_SELECTION_Y_ITEM  							(AGENT_IMAGER_SELECTION_PROPERTY->items+12)

#define AGENT_IMAGER_SPIKES_PROPERTY					(DEVICE_PRIVATE_DATA->agent_spikes_property)
#define AGENT_IMAGER_SPIKE_1_RHO_ITEM  				(AGENT_IMAGER_SPIKES_PROPERTY->items+0)
#define AGENT_IMAGER_SPIKE_1_THETA_ITEM  			(AGENT_IMAGER_SPIKES_PROPERTY->items+1)
#define AGENT_IMAGER_SPIKE_2_RHO_ITEM  				(AGENT_IMAGER_SPIKES_PROPERTY->items+2)
#define AGENT_IMAGER_SPIKE_2_THETA_ITEM  			(AGENT_IMAGER_SPIKES_PROPERTY->items+3)
#define AGENT_IMAGER_SPIKE_3_RHO_ITEM  				(AGENT_IMAGER_SPIKES_PROPERTY->items+4)
#define AGENT_IMAGER_SPIKE_3_THETA_ITEM  			(AGENT_IMAGER_SPIKES_PROPERTY->items+5)

#define AGENT_IMAGER_SEQUENCE_PROPERTY				(DEVICE_PRIVATE_DATA->agent_sequence)
#define AGENT_IMAGER_SEQUENCE_ITEM						(AGENT_IMAGER_SEQUENCE_PROPERTY->items+0)

#define AGENT_IMAGER_SEQUENCE_SIZE_PROPERTY				(DEVICE_PRIVATE_DATA->agent_sequence_size)
#define AGENT_IMAGER_SEQUENCE_SIZE_ITEM					(AGENT_IMAGER_SEQUENCE_SIZE_PROPERTY->items+0)

#define AGENT_IMAGER_BREAKPOINT_PROPERTY						(DEVICE_PRIVATE_DATA->agent_breakpoint_property)
#define AGENT_IMAGER_BREAKPOINT_PRE_BATCH_ITEM			(AGENT_IMAGER_BREAKPOINT_PROPERTY->items+0)
#define AGENT_IMAGER_BREAKPOINT_PRE_CAPTURE_ITEM		(AGENT_IMAGER_BREAKPOINT_PROPERTY->items+1)
#define AGENT_IMAGER_BREAKPOINT_POST_CAPTURE_ITEM		(AGENT_IMAGER_BREAKPOINT_PROPERTY->items+2)
#define AGENT_IMAGER_BREAKPOINT_PRE_DELAY_ITEM			(AGENT_IMAGER_BREAKPOINT_PROPERTY->items+3)
#define AGENT_IMAGER_BREAKPOINT_POST_DELAY_ITEM			(AGENT_IMAGER_BREAKPOINT_PROPERTY->items+4)
#define AGENT_IMAGER_BREAKPOINT_POST_BATCH_ITEM			(AGENT_IMAGER_BREAKPOINT_PROPERTY->items+5)

#define AGENT_IMAGER_RESUME_CONDITION_PROPERTY			(DEVICE_PRIVATE_DATA->agent_resume_condition_property)
#define AGENT_IMAGER_RESUME_CONDITION_TRIGGER_ITEM	(AGENT_IMAGER_RESUME_CONDITION_PROPERTY->items+0)
#define AGENT_IMAGER_RESUME_CONDITION_BARRIER_ITEM	(AGENT_IMAGER_RESUME_CONDITION_PROPERTY->items+1)

#define AGENT_IMAGER_BARRIER_STATE_PROPERTY					(DEVICE_PRIVATE_DATA->agent_barrier_property)

#define SEQUENCE_SIZE					16
#define MAX_SEQUENCE_SIZE				128

#define BUSY_TIMEOUT 5
#define AF_MOVE_LIMIT_HFD 20
#define AF_MOVE_LIMIT_RMS 40
#define AF_MOVE_LIMIT_UCURVE 10

#define MAX_UCURVE_SAMPLES 24
#define UCURVE_ORDER 4

#define DIGEST_CONVERGE_ITERATIONS 3

#define GRID	32

#define MAX_BAHTINOV_FRAME_SIZE 500

#define MAX(X, Y) (((X) > (Y)) ? (X) : (Y))

typedef struct {
	indigo_property *agent_imager_batch_property;
	indigo_property *agent_imager_focus_property;
	indigo_property *agent_imager_focus_failure_property;
	indigo_property *agent_imager_focus_estimator_property;
	indigo_property *agent_imager_download_file_property;
	indigo_property *agent_imager_download_files_property;
	indigo_property *agent_imager_download_image_property;
	indigo_property *agent_imager_delete_file_property;
	indigo_property *agent_imager_capture_property;
	indigo_property *agent_start_process_property;
	indigo_property *agent_pause_process_property;
	indigo_property *agent_abort_process_property;
	indigo_property *agent_process_features_property;
	indigo_property *agent_wheel_filter_property;
	indigo_property *agent_focuser_control_property;
	indigo_property *agent_stars_property;
	indigo_property *agent_selection_property;
	indigo_property *agent_spikes_property;
	indigo_property *agent_stats_property;
	indigo_property *agent_sequence_size;
	indigo_property *agent_sequence;
	indigo_property *agent_sequence_state;
	indigo_property *agent_breakpoint_property;
	indigo_property *agent_resume_condition_property;
	indigo_property *agent_barrier_property;
	double remaining_exposure_time;
	indigo_property_state exposure_state;
	double remaining_streaming_count;
	indigo_property_state streaming_state;
	int bin_x, bin_y;
	double frame[4];
	double saved_frame[4];
	double saved_include_region[4], saved_exclude_region[4];
	double saved_selection_x, saved_selection_y;
	bool autosubframing;
	bool light_frame;
	indigo_property_state steps_state;
	char current_folder[INDIGO_VALUE_SIZE];
	void *image_buffer;
	size_t image_buffer_size;
	double focuser_position;
	double focuser_temperature;
	double saved_backlash;
	int ucurve_samples_number;
	indigo_star_detection stars[MAX_STAR_COUNT];
	indigo_frame_digest reference;
	double drift_x, drift_y;
	void *last_image;
	long last_image_size;
	char last_image_url[INDIGO_VALUE_SIZE];
	pthread_mutex_t last_image_mutex;
	int last_width;
	int last_height;
	pthread_mutex_t mutex;
	double focus_exposure;
	bool dithering_started, dithering_finished, guiding;
	bool frame_saturated;
	bool focuser_has_backlash;
	bool restore_initial_position;
	bool use_hfd_estimator;
	bool use_rms_estimator;
	bool use_bahtinov_estimator;
	bool use_ucurve_focusing;
	bool use_iterative_focusing;
	bool use_aux_1;
	bool barrier_resume;
	unsigned int dither_num;
	indigo_property_state related_solver_process_state;
	indigo_property_state related_guider_process_state;
	double solver_goto_ra;
	double solver_goto_dec;
	double ra, dec, latitude, longitude, time_to_transit;
	bool has_camera;
} imager_agent_private_data;

// -------------------------------------------------------------------------------- INDIGO agent common code

static void save_config(indigo_device *device) {
	if (pthread_mutex_trylock(&DEVICE_CONTEXT->config_mutex) == 0) {
		pthread_mutex_unlock(&DEVICE_CONTEXT->config_mutex);
		pthread_mutex_lock(&DEVICE_PRIVATE_DATA->mutex);
		indigo_save_property(device, NULL, AGENT_IMAGER_BATCH_PROPERTY);
		indigo_save_property(device, NULL, AGENT_IMAGER_FOCUS_PROPERTY);
		indigo_save_property(device, NULL, AGENT_IMAGER_FOCUS_FAILURE_PROPERTY);
		indigo_save_property(device, NULL, AGENT_IMAGER_FOCUS_ESTIMATOR_PROPERTY);
		indigo_save_property(device, NULL, AGENT_IMAGER_SEQUENCE_SIZE_PROPERTY);
		indigo_save_property(device, NULL, AGENT_IMAGER_SEQUENCE_PROPERTY);
		indigo_save_property(device, NULL, ADDITIONAL_INSTANCES_PROPERTY);
		indigo_save_property(device, NULL, AGENT_PROCESS_FEATURES_PROPERTY);
		char *selection_property_items[] = { AGENT_IMAGER_SELECTION_RADIUS_ITEM_NAME, AGENT_IMAGER_SELECTION_SUBFRAME_ITEM_NAME, AGENT_IMAGER_SELECTION_STAR_COUNT_ITEM_NAME };
		indigo_save_property_items(device, NULL, AGENT_IMAGER_SELECTION_PROPERTY, 3, (const char **)selection_property_items);
		if (DEVICE_CONTEXT->property_save_file_handle != NULL) {
			CONFIG_PROPERTY->state = INDIGO_OK_STATE;
			indigo_uni_close(&DEVICE_CONTEXT->property_save_file_handle);
		} else {
			CONFIG_PROPERTY->state = INDIGO_ALERT_STATE;
		}
		CONFIG_SAVE_ITEM->sw.value = false;
		indigo_update_property(device, CONFIG_PROPERTY, NULL);
		pthread_mutex_unlock(&DEVICE_PRIVATE_DATA->mutex);
	}
}

static void clear_stats(indigo_device *device) {
	AGENT_IMAGER_STATS_EXPOSURE_ITEM->number.value = 0;
	AGENT_IMAGER_STATS_DELAY_ITEM->number.value = 0;
	AGENT_IMAGER_STATS_FRAMES_ITEM->number.value = 0;
	AGENT_IMAGER_STATS_FRAME_ITEM->number.value = 0;
	AGENT_IMAGER_STATS_FWHM_ITEM->number.value = 0;
	AGENT_IMAGER_STATS_PEAK_ITEM->number.value = 0;
	AGENT_IMAGER_STATS_DRIFT_X_ITEM->number.value = 0;
	AGENT_IMAGER_STATS_DRIFT_Y_ITEM->number.value = 0;
	AGENT_IMAGER_STATS_RMS_CONTRAST_ITEM->number.value = 0;
	AGENT_IMAGER_STATS_BAHTINOV_ITEM->number.value = 0;
	AGENT_IMAGER_STATS_FOCUS_DEVIATION_ITEM->number.value = 100;
	for (int i = 0; i < INDIGO_MAX_MULTISTAR_COUNT; i++) {
		AGENT_IMAGER_STATS_HFD_ITEM[i].number.value = 0;
	}
	indigo_update_property(device, AGENT_IMAGER_STATS_PROPERTY, NULL);
}

static void set_headers(indigo_device *device) {
	if (INDIGO_FILTER_WHEEL_SELECTED) {
		for (int i = 0; i < AGENT_WHEEL_FILTER_PROPERTY->count; i++) {
			indigo_item *item = AGENT_WHEEL_FILTER_PROPERTY->items + i;
			if (item->sw.value) {
				indigo_set_fits_header(FILTER_DEVICE_CONTEXT->client, device->name, "FILTER", "'%s'", item->label);
				break;
			}
		}
	} else {
		indigo_remove_fits_header(FILTER_DEVICE_CONTEXT->client, device->name, "FILTER");
	}
	if (INDIGO_FILTER_FOCUSER_SELECTED) {
		if (isnan(DEVICE_PRIVATE_DATA->focuser_position)) {
			indigo_remove_fits_header(FILTER_DEVICE_CONTEXT->client, device->name, "FOCUSPOS");
		} else {
			if (DEVICE_PRIVATE_DATA->focuser_position - rint(DEVICE_PRIVATE_DATA->focuser_position) < 0.00001) {
				indigo_set_fits_header(FILTER_DEVICE_CONTEXT->client, device->name, "FOCUSPOS", "%d", (int)DEVICE_PRIVATE_DATA->focuser_position);
			} else {
				indigo_set_fits_header(FILTER_DEVICE_CONTEXT->client, device->name, "FOCUSPOS", "%.5f", DEVICE_PRIVATE_DATA->focuser_position);
			}
		}
		if (isnan(DEVICE_PRIVATE_DATA->focuser_temperature)) {
			indigo_remove_fits_header(FILTER_DEVICE_CONTEXT->client, device->name, "FOCTEMP");
		} else {
			indigo_set_fits_header(FILTER_DEVICE_CONTEXT->client, device->name, "FOCTEMP", "%.1f", DEVICE_PRIVATE_DATA->focuser_temperature);
		}
	} else {
		indigo_remove_fits_header(FILTER_DEVICE_CONTEXT->client, device->name, "FOCUSPOS");
		indigo_remove_fits_header(FILTER_DEVICE_CONTEXT->client, device->name, "FOCTEMP");
	}
}

static bool capture_frame(indigo_device *device) {
	indigo_property_state state = INDIGO_ALERT_STATE;
	DEVICE_PRIVATE_DATA->frame_saturated = false;
	if (DEVICE_PRIVATE_DATA->last_image) {
		free (DEVICE_PRIVATE_DATA->last_image);
		DEVICE_PRIVATE_DATA->last_image = NULL;
		DEVICE_PRIVATE_DATA->last_image_size = 0;
	}
	for (int exposure_attempt = 0; exposure_attempt < 3; exposure_attempt++) {
		while (AGENT_PAUSE_PROCESS_PROPERTY->state == INDIGO_BUSY_STATE)
			indigo_usleep(200000);
		if (AGENT_ABORT_PROCESS_PROPERTY->state == INDIGO_BUSY_STATE)
			return false;
		if (DEVICE_PRIVATE_DATA->use_aux_1) {
			indigo_change_number_property_1(FILTER_DEVICE_CONTEXT->client, device->name, CCD_EXPOSURE_PROPERTY_NAME, CCD_EXPOSURE_ITEM_NAME, 0);
			indigo_change_number_property_1(FILTER_DEVICE_CONTEXT->client, device->name, "AUX_1_" CCD_EXPOSURE_PROPERTY_NAME, CCD_EXPOSURE_ITEM_NAME, AGENT_IMAGER_BATCH_EXPOSURE_ITEM->number.target);
		} else {
			indigo_change_number_property_1(FILTER_DEVICE_CONTEXT->client, device->name, CCD_EXPOSURE_PROPERTY_NAME, CCD_EXPOSURE_ITEM_NAME, AGENT_IMAGER_BATCH_EXPOSURE_ITEM->number.target);
		}
		for (int i = 0; i < BUSY_TIMEOUT * 1000 && (state = DEVICE_PRIVATE_DATA->exposure_state) != INDIGO_BUSY_STATE && AGENT_ABORT_PROCESS_PROPERTY->state != INDIGO_BUSY_STATE && AGENT_PAUSE_PROCESS_PROPERTY->state != INDIGO_BUSY_STATE; i++)
			indigo_usleep(1000);
		if (AGENT_PAUSE_PROCESS_PROPERTY->state == INDIGO_BUSY_STATE) {
			while (AGENT_PAUSE_PROCESS_PROPERTY->state == INDIGO_BUSY_STATE)
				indigo_usleep(200000);
			if (AGENT_PAUSE_PROCESS_ITEM->sw.value) {
				exposure_attempt--;
				continue;
			}
		}
		if (AGENT_ABORT_PROCESS_PROPERTY->state == INDIGO_BUSY_STATE)
			return false;
		if (state != INDIGO_BUSY_STATE) {
			INDIGO_DRIVER_ERROR(DRIVER_NAME, "CCD_EXPOSURE didn't become busy in %d second(s)", BUSY_TIMEOUT);
			indigo_usleep(ONE_SECOND_DELAY);
			continue;
		}
		double remaining_exposure_time = DEVICE_PRIVATE_DATA->remaining_exposure_time;
		AGENT_IMAGER_STATS_EXPOSURE_ITEM->number.value = remaining_exposure_time;
		indigo_update_property(device, AGENT_IMAGER_STATS_PROPERTY, NULL);
		while ((state = DEVICE_PRIVATE_DATA->exposure_state) == INDIGO_BUSY_STATE) {
			if (AGENT_ABORT_PROCESS_PROPERTY->state == INDIGO_BUSY_STATE)
				return false;
			if (remaining_exposure_time != DEVICE_PRIVATE_DATA->remaining_exposure_time) {
				AGENT_IMAGER_STATS_EXPOSURE_ITEM->number.value = remaining_exposure_time = DEVICE_PRIVATE_DATA->remaining_exposure_time;
				indigo_update_property(device, AGENT_IMAGER_STATS_PROPERTY, NULL);
			}
			if (remaining_exposure_time > 1) {
				indigo_usleep(200000);
			} else {
				indigo_usleep(10000);
			}
		}
		if (AGENT_PAUSE_PROCESS_PROPERTY->state == INDIGO_BUSY_STATE) {
			while (AGENT_PAUSE_PROCESS_PROPERTY->state == INDIGO_BUSY_STATE)
				indigo_usleep(200000);
			if (AGENT_PAUSE_PROCESS_ITEM->sw.value) {
				exposure_attempt--;
				continue;
			}
		}
		if (AGENT_ABORT_PROCESS_PROPERTY->state == INDIGO_BUSY_STATE)
			return false;
		if (state != INDIGO_OK_STATE) {
			INDIGO_DRIVER_ERROR(DRIVER_NAME, "CCD_EXPOSURE_PROPERTY didn't become OK", "");
			indigo_usleep(ONE_SECOND_DELAY);
			continue;
		}
		pthread_mutex_lock(&DEVICE_PRIVATE_DATA->last_image_mutex);
		if (DEVICE_PRIVATE_DATA->last_image == NULL) {
			if (!indigo_download_blob(DEVICE_PRIVATE_DATA->last_image_url, &DEVICE_PRIVATE_DATA->last_image, &DEVICE_PRIVATE_DATA->last_image_size, NULL)) {
				indigo_send_message(device, "Image download failed");
				pthread_mutex_unlock(&DEVICE_PRIVATE_DATA->last_image_mutex);
				return false;
			}
		}
		pthread_mutex_unlock(&DEVICE_PRIVATE_DATA->last_image_mutex);
		indigo_raw_header *header = (indigo_raw_header *)(DEVICE_PRIVATE_DATA->last_image);
		DEVICE_PRIVATE_DATA->last_width = header->width;
		DEVICE_PRIVATE_DATA->last_height = header->height;
		if (header == NULL || (header->signature != INDIGO_RAW_MONO8 && header->signature != INDIGO_RAW_MONO16 && header->signature != INDIGO_RAW_RGB24 && header->signature != INDIGO_RAW_RGB48)) {
			indigo_send_message(device, "Error: RAW image not received");
			return false;
		}
		/* This is potentially bayered image, if so we need to equalize the channels */
		if (indigo_is_bayered_image(header, DEVICE_PRIVATE_DATA->last_image_size)) {
			INDIGO_DRIVER_DEBUG(DRIVER_NAME, "Bayered image detected, equalizing channels", "");
			indigo_equalize_bayer_channels(header->signature, (char *)header + sizeof(indigo_raw_header), header->width, header->height);
		}
		return true;
	}
	INDIGO_DRIVER_ERROR(DRIVER_NAME, "Exposure failed", "");
	return false;
}

static bool capture_plain_frame(indigo_device *device) {
	indigo_property_state state = INDIGO_ALERT_STATE;
	if (DEVICE_PRIVATE_DATA->last_image) {
		free (DEVICE_PRIVATE_DATA->last_image);
		DEVICE_PRIVATE_DATA->last_image = NULL;
		DEVICE_PRIVATE_DATA->last_image_size = 0;
	}
	for (int exposure_attempt = 0; exposure_attempt < 3; exposure_attempt++) {
		if (AGENT_ABORT_PROCESS_PROPERTY->state == INDIGO_BUSY_STATE)
			return false;
		if (DEVICE_PRIVATE_DATA->use_aux_1) {
			indigo_change_number_property_1(FILTER_DEVICE_CONTEXT->client, device->name, CCD_EXPOSURE_PROPERTY_NAME, CCD_EXPOSURE_ITEM_NAME, 0);
			indigo_change_number_property_1(FILTER_DEVICE_CONTEXT->client, device->name, "AUX_1_" CCD_EXPOSURE_PROPERTY_NAME, CCD_EXPOSURE_ITEM_NAME, AGENT_IMAGER_CAPTURE_ITEM->number.target);
		} else {
			indigo_change_number_property_1(FILTER_DEVICE_CONTEXT->client, device->name, CCD_EXPOSURE_PROPERTY_NAME, CCD_EXPOSURE_ITEM_NAME, AGENT_IMAGER_CAPTURE_ITEM->number.target);
		}
		for (int i = 0; i < BUSY_TIMEOUT * 1000 && (state = DEVICE_PRIVATE_DATA->exposure_state) != INDIGO_BUSY_STATE && AGENT_ABORT_PROCESS_PROPERTY->state != INDIGO_BUSY_STATE; i++)
			indigo_usleep(1000);
		if (AGENT_ABORT_PROCESS_PROPERTY->state == INDIGO_BUSY_STATE)
			return false;
		if (state != INDIGO_BUSY_STATE) {
			INDIGO_DRIVER_ERROR(DRIVER_NAME, "CCD_EXPOSURE didn't become busy in %d second(s)", BUSY_TIMEOUT);
			indigo_usleep(ONE_SECOND_DELAY);
			continue;
		}
		double remaining_exposure_time = DEVICE_PRIVATE_DATA->remaining_exposure_time;
		AGENT_IMAGER_CAPTURE_ITEM->number.value = remaining_exposure_time;
		indigo_update_property(device, AGENT_IMAGER_CAPTURE_PROPERTY, NULL);
		while ((state = DEVICE_PRIVATE_DATA->exposure_state) == INDIGO_BUSY_STATE) {
			if (AGENT_ABORT_PROCESS_PROPERTY->state == INDIGO_BUSY_STATE)
				return false;
			if (remaining_exposure_time != DEVICE_PRIVATE_DATA->remaining_exposure_time) {
				AGENT_IMAGER_CAPTURE_ITEM->number.value = remaining_exposure_time = DEVICE_PRIVATE_DATA->remaining_exposure_time;
				indigo_update_property(device, AGENT_IMAGER_CAPTURE_PROPERTY, NULL);
			}
			if (remaining_exposure_time > 1) {
				indigo_usleep(200000);
			} else {
				indigo_usleep(10000);
			}
		}
		if (AGENT_ABORT_PROCESS_PROPERTY->state == INDIGO_BUSY_STATE)
			return false;
		if (state != INDIGO_OK_STATE) {
			INDIGO_DRIVER_ERROR(DRIVER_NAME, "CCD_EXPOSURE_PROPERTY didn't become OK", "");
			indigo_usleep(ONE_SECOND_DELAY);
			continue;
		}
		pthread_mutex_lock(&DEVICE_PRIVATE_DATA->last_image_mutex);
		if (DEVICE_PRIVATE_DATA->last_image == NULL) {
			if (!indigo_download_blob(DEVICE_PRIVATE_DATA->last_image_url, &DEVICE_PRIVATE_DATA->last_image, &DEVICE_PRIVATE_DATA->last_image_size, NULL)) {
				indigo_send_message(device, "Image download failed");
				pthread_mutex_unlock(&DEVICE_PRIVATE_DATA->last_image_mutex);
				return false;
			}
		}
		pthread_mutex_unlock(&DEVICE_PRIVATE_DATA->last_image_mutex);
		indigo_raw_header *header = (indigo_raw_header *)(DEVICE_PRIVATE_DATA->last_image);
		DEVICE_PRIVATE_DATA->last_width = header->width;
		DEVICE_PRIVATE_DATA->last_height = header->height;
		if (header == NULL || (header->signature != INDIGO_RAW_MONO8 && header->signature != INDIGO_RAW_MONO16 && header->signature != INDIGO_RAW_RGB24 && header->signature != INDIGO_RAW_RGB48)) {
			indigo_send_message(device, "Error: RAW image not received");
			return false;
		}
		/* This is potentially bayered image, if so we need to equalize the channels */
		if (indigo_is_bayered_image(header, DEVICE_PRIVATE_DATA->last_image_size)) {
			INDIGO_DRIVER_DEBUG(DRIVER_NAME, "Bayered image detected, equalizing channels", "");
			indigo_equalize_bayer_channels(header->signature, (char *)header + sizeof(indigo_raw_header), header->width, header->height);
		}
		return true;
	}
	INDIGO_DRIVER_ERROR(DRIVER_NAME, "Exposure failed", "");
	return false;
}



static bool find_stars(indigo_device *device) {
	int star_count;
	indigo_raw_header *header = (indigo_raw_header *)(DEVICE_PRIVATE_DATA->last_image);
	indigo_delete_property(device, AGENT_IMAGER_STARS_PROPERTY, NULL);
	indigo_find_stars_precise_clipped(header->signature, (char *)header + sizeof(indigo_raw_header), (int)AGENT_IMAGER_SELECTION_RADIUS_ITEM->number.value, header->width, header->height, MAX_STAR_COUNT, (int)AGENT_IMAGER_SELECTION_INCLUDE_LEFT_ITEM->number.value, (int)AGENT_IMAGER_SELECTION_INCLUDE_TOP_ITEM->number.value, (int)AGENT_IMAGER_SELECTION_INCLUDE_WIDTH_ITEM->number.value, (int)AGENT_IMAGER_SELECTION_INCLUDE_HEIGHT_ITEM->number.value, (int)AGENT_IMAGER_SELECTION_EXCLUDE_LEFT_ITEM->number.value, (int)AGENT_IMAGER_SELECTION_EXCLUDE_TOP_ITEM->number.value, (int)AGENT_IMAGER_SELECTION_EXCLUDE_WIDTH_ITEM->number.value, (int)AGENT_IMAGER_SELECTION_EXCLUDE_HEIGHT_ITEM->number.value, (indigo_star_detection *)&DEVICE_PRIVATE_DATA->stars, &star_count);
	AGENT_IMAGER_STARS_PROPERTY->count = star_count + 1;
	for (int i = 0; i < star_count; i++) {
		char name[8];
		char label[INDIGO_NAME_SIZE];
		snprintf(name, sizeof(name), "%d", i);
		snprintf(label, sizeof(label), "[%d, %d]", (int)DEVICE_PRIVATE_DATA->stars[i].x, (int)DEVICE_PRIVATE_DATA->stars[i].y);
		indigo_init_switch_item(AGENT_IMAGER_STARS_PROPERTY->items + i + 1, name, label, false);
	}
	AGENT_IMAGER_STARS_PROPERTY->state = INDIGO_OK_STATE;
	indigo_define_property(device, AGENT_IMAGER_STARS_PROPERTY, NULL);
	if (star_count == 0) {
		indigo_send_message(device, "Error: No stars detected", "");
		return false;
	}
	return true;
}

static bool select_stars(indigo_device *device) {
	int star_count = 0;
	for (int i = 0; i < AGENT_IMAGER_SELECTION_STAR_COUNT_ITEM->number.value; i++) {
		indigo_item *item_x = AGENT_IMAGER_SELECTION_X_ITEM + 2 * i;
		indigo_item *item_y = AGENT_IMAGER_SELECTION_Y_ITEM + 2 * i;
		if (i == AGENT_IMAGER_STARS_PROPERTY->count - 1) {
			if (DEVICE_PRIVATE_DATA->use_ucurve_focusing) {
				indigo_send_message(device, "Warning: Only %d suitable stars found (%d requested).", star_count, (int)AGENT_IMAGER_SELECTION_STAR_COUNT_ITEM->number.value);
			}
			break;
		}
		item_x->number.target = item_x->number.value = DEVICE_PRIVATE_DATA->stars[i].x;
		item_y->number.target = item_y->number.value = DEVICE_PRIVATE_DATA->stars[i].y;
		star_count++;
	}
	/* In case the number of the stars found is less than AGENT_IMAGER_SELECTION_STAR_COUNT_ITEM
	 set ramaining selections to 0. Otherwise we will have leftover "ghost" stars from the
	 previous search.
	 */
	for (int i = star_count; i < AGENT_IMAGER_SELECTION_STAR_COUNT_ITEM->number.value; i++) {
		indigo_item *item_x = AGENT_IMAGER_SELECTION_X_ITEM + 2 * i;
		indigo_item *item_y = AGENT_IMAGER_SELECTION_Y_ITEM + 2 * i;
		item_x->number.target = item_x->number.value = 0;
		item_y->number.target = item_y->number.value = 0;
	}
	indigo_update_property(device, AGENT_IMAGER_SELECTION_PROPERTY, NULL);
	return star_count > 0;
}

static bool check_selection(indigo_device *device) {
	for (int i = 0; i < AGENT_IMAGER_SELECTION_STAR_COUNT_ITEM->number.value; i++) {
		if ((AGENT_IMAGER_SELECTION_X_ITEM + i)->number.value != 0 && (AGENT_IMAGER_SELECTION_Y_ITEM + i)->number.value != 0) {
			return true;
		}
	}
	bool result;
	if ((result = capture_frame(device)) && AGENT_ABORT_PROCESS_PROPERTY->state != INDIGO_BUSY_STATE) {
		AGENT_IMAGER_STARS_PROPERTY->count = 1;
		result = find_stars(device);
	}
	if (result && AGENT_ABORT_PROCESS_PROPERTY->state != INDIGO_BUSY_STATE) {
		result = select_stars(device);
	}
	return result;
}

static void clear_selection(indigo_device *device) {
	if (AGENT_IMAGER_STARS_PROPERTY->count > 1) {
		indigo_delete_property(device, AGENT_IMAGER_STARS_PROPERTY, NULL);
		AGENT_IMAGER_STARS_PROPERTY->count = 1;
		indigo_define_property(device, AGENT_IMAGER_STARS_PROPERTY, NULL);
	}
	indigo_update_property(device, AGENT_IMAGER_STARS_PROPERTY, NULL);
	for (int i = (int)(AGENT_IMAGER_SELECTION_X_ITEM - AGENT_IMAGER_SELECTION_PROPERTY->items); i < AGENT_IMAGER_SELECTION_PROPERTY->count; i++) {
		indigo_item *item = AGENT_IMAGER_SELECTION_PROPERTY->items + i;
		item->number.value = item->number.target = 0;
	}
	indigo_update_property(device, AGENT_IMAGER_SELECTION_PROPERTY, NULL);
}

static bool select_subframe(indigo_device *device) {
	int selection_x = (int)AGENT_IMAGER_SELECTION_X_ITEM->number.value;
	int selection_y = (int)AGENT_IMAGER_SELECTION_Y_ITEM->number.value;
	if (selection_x == 0 || selection_y == 0) {
		indigo_send_message(device, "Warning: Failed to select subframe.");
		return false;
	}
	if (AGENT_IMAGER_SELECTION_SUBFRAME_ITEM->number.value && DEVICE_PRIVATE_DATA->saved_frame[2] == 0 && DEVICE_PRIVATE_DATA->saved_frame[3] == 0) {
		DEVICE_PRIVATE_DATA->autosubframing = true;
		DEVICE_PRIVATE_DATA->saved_selection_x = selection_x;
		DEVICE_PRIVATE_DATA->saved_selection_y = selection_y;
		memcpy(DEVICE_PRIVATE_DATA->saved_frame, DEVICE_PRIVATE_DATA->frame, 4 * sizeof(double));
		int bin_x = DEVICE_PRIVATE_DATA->bin_x;
		int bin_y = DEVICE_PRIVATE_DATA->bin_y;
		selection_x += (int)(DEVICE_PRIVATE_DATA->frame[0] / bin_x); // left
		selection_y += (int)(DEVICE_PRIVATE_DATA->frame[1] / bin_y); // top
		int window_size = (int)(AGENT_IMAGER_SELECTION_SUBFRAME_ITEM->number.value * AGENT_IMAGER_SELECTION_RADIUS_ITEM->number.value);
		if (window_size < GRID)
			window_size = GRID;
		int frame_left = (int)(rint((selection_x - window_size) / (double)GRID) * GRID);
		int frame_top = (int)(rint((selection_y - window_size) / (double)GRID) * GRID);
		if (selection_x - frame_left < AGENT_IMAGER_SELECTION_RADIUS_ITEM->number.value)
			frame_left -= GRID;
		if (selection_y - frame_top < AGENT_IMAGER_SELECTION_RADIUS_ITEM->number.value)
			frame_top -= GRID;
		int frame_width = (2 * window_size / GRID + 1) * GRID;
		int frame_height = (2 * window_size / GRID + 1) * GRID;
		AGENT_IMAGER_SELECTION_X_ITEM->number.value = selection_x -= frame_left;
		AGENT_IMAGER_SELECTION_Y_ITEM->number.value = selection_y -= frame_top;
		DEVICE_PRIVATE_DATA->saved_include_region[0] = AGENT_IMAGER_SELECTION_INCLUDE_LEFT_ITEM->number.value;
		DEVICE_PRIVATE_DATA->saved_include_region[1] = AGENT_IMAGER_SELECTION_INCLUDE_TOP_ITEM->number.value;
		DEVICE_PRIVATE_DATA->saved_include_region[2] = AGENT_IMAGER_SELECTION_INCLUDE_WIDTH_ITEM->number.value;
		DEVICE_PRIVATE_DATA->saved_include_region[3] = AGENT_IMAGER_SELECTION_INCLUDE_HEIGHT_ITEM->number.value;
		DEVICE_PRIVATE_DATA->saved_exclude_region[0] = AGENT_IMAGER_SELECTION_EXCLUDE_LEFT_ITEM->number.value;
		DEVICE_PRIVATE_DATA->saved_exclude_region[1] = AGENT_IMAGER_SELECTION_EXCLUDE_TOP_ITEM->number.value;
		DEVICE_PRIVATE_DATA->saved_exclude_region[2] = AGENT_IMAGER_SELECTION_EXCLUDE_WIDTH_ITEM->number.value;
		DEVICE_PRIVATE_DATA->saved_exclude_region[3] = AGENT_IMAGER_SELECTION_EXCLUDE_HEIGHT_ITEM->number.value;
		AGENT_IMAGER_SELECTION_INCLUDE_LEFT_ITEM->number.value = AGENT_IMAGER_SELECTION_INCLUDE_TOP_ITEM->number.value = AGENT_IMAGER_SELECTION_INCLUDE_WIDTH_ITEM->number.value = AGENT_IMAGER_SELECTION_INCLUDE_HEIGHT_ITEM->number.value = AGENT_IMAGER_SELECTION_EXCLUDE_LEFT_ITEM->number.value = AGENT_IMAGER_SELECTION_EXCLUDE_TOP_ITEM->number.value = AGENT_IMAGER_SELECTION_EXCLUDE_WIDTH_ITEM->number.value = AGENT_IMAGER_SELECTION_EXCLUDE_HEIGHT_ITEM->number.value = 0;
		indigo_update_property(device, AGENT_IMAGER_SELECTION_PROPERTY, NULL);
		if (frame_width - selection_x < AGENT_IMAGER_SELECTION_RADIUS_ITEM->number.value)
			frame_width += GRID;
		if (frame_height - selection_y < AGENT_IMAGER_SELECTION_RADIUS_ITEM->number.value)
			frame_height += GRID;
		static const char *names[] = { CCD_FRAME_LEFT_ITEM_NAME, CCD_FRAME_TOP_ITEM_NAME, CCD_FRAME_WIDTH_ITEM_NAME, CCD_FRAME_HEIGHT_ITEM_NAME };
		double values[] = { frame_left * bin_x, frame_top * bin_y,  frame_width * bin_x, frame_height * bin_y };
		indigo_change_number_property(FILTER_DEVICE_CONTEXT->client, device->name, CCD_FRAME_PROPERTY_NAME, 4, (const char **)names, values);
		return true;
	}
	return false;
}

static void restore_subframe(indigo_device *device) {
	if (DEVICE_PRIVATE_DATA->autosubframing) {
		static const char *names[] = { CCD_FRAME_LEFT_ITEM_NAME, CCD_FRAME_TOP_ITEM_NAME, CCD_FRAME_WIDTH_ITEM_NAME, CCD_FRAME_HEIGHT_ITEM_NAME };
		indigo_change_number_property(FILTER_DEVICE_CONTEXT->client, device->name, CCD_FRAME_PROPERTY_NAME, 4, (const char **)names, DEVICE_PRIVATE_DATA->saved_frame);
		memset(DEVICE_PRIVATE_DATA->saved_frame, 0, 4 * sizeof(double));
		AGENT_IMAGER_SELECTION_X_ITEM->number.target = AGENT_IMAGER_SELECTION_X_ITEM->number.value = DEVICE_PRIVATE_DATA->saved_selection_x;
		AGENT_IMAGER_SELECTION_Y_ITEM->number.target = AGENT_IMAGER_SELECTION_Y_ITEM->number.value = DEVICE_PRIVATE_DATA->saved_selection_y;
		AGENT_IMAGER_SELECTION_INCLUDE_LEFT_ITEM->number.value = DEVICE_PRIVATE_DATA->saved_include_region[0];
		AGENT_IMAGER_SELECTION_INCLUDE_TOP_ITEM->number.value = DEVICE_PRIVATE_DATA->saved_include_region[1];
		AGENT_IMAGER_SELECTION_INCLUDE_WIDTH_ITEM->number.value = DEVICE_PRIVATE_DATA->saved_include_region[2];
		AGENT_IMAGER_SELECTION_INCLUDE_HEIGHT_ITEM->number.value = DEVICE_PRIVATE_DATA->saved_include_region[3];
		AGENT_IMAGER_SELECTION_EXCLUDE_LEFT_ITEM->number.value = DEVICE_PRIVATE_DATA->saved_exclude_region[0];
		AGENT_IMAGER_SELECTION_EXCLUDE_TOP_ITEM->number.value = DEVICE_PRIVATE_DATA->saved_exclude_region[1];
		AGENT_IMAGER_SELECTION_EXCLUDE_WIDTH_ITEM->number.value = DEVICE_PRIVATE_DATA->saved_exclude_region[2];
		AGENT_IMAGER_SELECTION_EXCLUDE_HEIGHT_ITEM->number.value = DEVICE_PRIVATE_DATA->saved_exclude_region[3];
		indigo_property_state state = AGENT_ABORT_PROCESS_PROPERTY->state;
		AGENT_ABORT_PROCESS_PROPERTY->state = INDIGO_OK_STATE;
		/* capture_frame() should be here in order to have the correct frame and correct selection */
		indigo_usleep((unsigned)(0.5 * ONE_SECOND_DELAY));
		capture_frame(device);
		AGENT_ABORT_PROCESS_PROPERTY->state = state;
		indigo_update_property(device, AGENT_IMAGER_SELECTION_PROPERTY, NULL);
		DEVICE_PRIVATE_DATA->saved_selection_x = 0;
		DEVICE_PRIVATE_DATA->saved_selection_y = 0;
		DEVICE_PRIVATE_DATA->autosubframing = false;
	}
}

static bool capture_and_process_frame(indigo_device *device, uint8_t **saturation_mask) {
	if (!capture_frame(device)) {
		return false;
	}
	if (AGENT_ABORT_PROCESS_PROPERTY->state == INDIGO_BUSY_STATE) {
		return false;
	}
	if (!AGENT_IMAGER_MACRO_MODE_FEATURE_ITEM->sw.value) {
		indigo_raw_header *header = (indigo_raw_header *)(DEVICE_PRIVATE_DATA->last_image);
		/* if frame changes, contrast changes too, so do not change AGENT_IMAGER_STATS_RMS_CONTRAST item if this frame is to restore the full frame */
		if (DEVICE_PRIVATE_DATA->use_rms_estimator) {
			if (saturation_mask) {
				INDIGO_DRIVER_DEBUG(DRIVER_NAME, "focus_saturation_mask = 0x%p", *saturation_mask);
				AGENT_IMAGER_STATS_FOCUS_POSITION_ITEM->number.value = DEVICE_PRIVATE_DATA->focuser_position;
				AGENT_IMAGER_STATS_RMS_CONTRAST_ITEM->number.value = indigo_contrast(header->signature, (char *)header + sizeof(indigo_raw_header), *saturation_mask, header->width, header->height, &DEVICE_PRIVATE_DATA->frame_saturated);
				INDIGO_DRIVER_DEBUG(DRIVER_NAME, "frame contrast = %f %s", AGENT_IMAGER_STATS_RMS_CONTRAST_ITEM->number.value, DEVICE_PRIVATE_DATA->frame_saturated ? "(saturated)" : "");
				if (DEVICE_PRIVATE_DATA->frame_saturated) {
					if (header->signature == INDIGO_RAW_MONO8 || header->signature == INDIGO_RAW_MONO16 || header->signature == INDIGO_RAW_RGB24 || header->signature == INDIGO_RAW_RGB48) {
						indigo_send_message(device, "Warning: Frame saturation detected, masking out saturated areas and resetting statistics");
						if (*saturation_mask == NULL) {
							indigo_init_saturation_mask(header->width, header->height, saturation_mask);
						}
						indigo_update_saturation_mask(header->signature, (char *)header + sizeof(indigo_raw_header), header->width, header->height, *saturation_mask);
						AGENT_IMAGER_STATS_RMS_CONTRAST_ITEM->number.value = indigo_contrast(header->signature, (char *)header + sizeof(indigo_raw_header), *saturation_mask, header->width, header->height, NULL);
						AGENT_IMAGER_STATS_FRAME_ITEM->number.value = 0;
					} else {  // Colour image saturation masking is not supported yet.
						indigo_send_message(device, "Warning: Colour image saturation masking is not supported");
						DEVICE_PRIVATE_DATA->frame_saturated = false;
					}
				}
			}
		} else if (DEVICE_PRIVATE_DATA->use_hfd_estimator) {
			int count = (int)AGENT_IMAGER_SELECTION_STAR_COUNT_ITEM->number.value;
			if (AGENT_IMAGER_STATS_FRAME_ITEM->number.value == 0) {
				indigo_delete_frame_digest(&DEVICE_PRIVATE_DATA->reference);
				DEVICE_PRIVATE_DATA->reference.centroid_x = 0;
				DEVICE_PRIVATE_DATA->reference.centroid_y = 0;
				DEVICE_PRIVATE_DATA->reference.snr = 0;
			}
			for (int i = 0; i < count; i++) {
				indigo_frame_digest reference;
				memset(&reference, 0, sizeof(reference));
				indigo_item *item_x = AGENT_IMAGER_SELECTION_X_ITEM + 2 * i;
				indigo_item *item_y = AGENT_IMAGER_SELECTION_Y_ITEM + 2 * i;
				indigo_item *item_hfd = AGENT_IMAGER_STATS_HFD_ITEM + i;
				if (item_x->number.value != 0 && item_y->number.value != 0) {
					/* It is normal indigo_selection_frame_digest_iterative() to fail during the AF process when the stars are too out of focus.
					 We can still use the previos selection to calculaate the the PSF of the stars. Even with the selection a bit off we still
					 need all PSFs calculated for the current focuser position, otherwise we will mix PSF measurements for the previous focuser
					 position and PSF measurements of the new focuser position and the focus accuracy will be compromised. This is why we do not
					 check the result here.
					 */
					indigo_selection_frame_digest_iterative(header->signature, (char *)header + sizeof(indigo_raw_header), &item_x->number.value, &item_y->number.value, (int)AGENT_IMAGER_SELECTION_RADIUS_ITEM->number.value, header->width, header->height, &reference, DIGEST_CONVERGE_ITERATIONS);
					double fwhm = 0, peak = 0;
					/* here it is the same - we need to continue even if the result is not OK. */
					indigo_selection_psf(header->signature, (char *)header + sizeof(indigo_raw_header), item_x->number.value, item_y->number.value, (int)AGENT_IMAGER_SELECTION_RADIUS_ITEM->number.value, header->width, header->height, &fwhm, &item_hfd->number.value, &peak);
					if (item_hfd->number.value > AGENT_IMAGER_SELECTION_RADIUS_ITEM->number.value * 2) {
						item_hfd->number.value = 0;
					} else if (i == 0) {
						AGENT_IMAGER_STATS_FOCUS_POSITION_ITEM->number.value = DEVICE_PRIVATE_DATA->focuser_position;
						AGENT_IMAGER_STATS_FWHM_ITEM->number.value = fwhm;
						AGENT_IMAGER_STATS_PEAK_ITEM->number.value = peak;
						if (AGENT_IMAGER_STATS_FRAME_ITEM->number.value == 0) {
							memcpy(&DEVICE_PRIVATE_DATA->reference, &reference, sizeof(reference));
						} else if (indigo_calculate_drift(&DEVICE_PRIVATE_DATA->reference, &reference, &DEVICE_PRIVATE_DATA->drift_x, &DEVICE_PRIVATE_DATA->drift_y) == INDIGO_OK) {
							AGENT_IMAGER_STATS_DRIFT_X_ITEM->number.value = round(1000 * DEVICE_PRIVATE_DATA->drift_x) / 1000;
							AGENT_IMAGER_STATS_DRIFT_Y_ITEM->number.value = round(1000 * DEVICE_PRIVATE_DATA->drift_y) / 1000;
							INDIGO_DRIVER_DEBUG(DRIVER_NAME, "Drift %.4gpx, %.4gpx", DEVICE_PRIVATE_DATA->drift_x, DEVICE_PRIVATE_DATA->drift_y);
						}
					}
					indigo_delete_frame_digest(&reference);
				}
			}
			indigo_update_property(device, AGENT_IMAGER_SELECTION_PROPERTY, NULL);
		} else if (DEVICE_PRIVATE_DATA->use_bahtinov_estimator) {
			if (header->width <= MAX_BAHTINOV_FRAME_SIZE && header->height <= MAX_BAHTINOV_FRAME_SIZE) {
				AGENT_IMAGER_STATS_FOCUS_POSITION_ITEM->number.value = DEVICE_PRIVATE_DATA->focuser_position;
				AGENT_IMAGER_STATS_BAHTINOV_ITEM->number.value = indigo_bahtinov_error(header->signature, (char *)header + sizeof(indigo_raw_header), header->width, header->height, AGENT_IMAGER_FOCUS_BAHTINOV_SIGMA_ITEM->number.value, &AGENT_IMAGER_SPIKE_1_RHO_ITEM->number.value, &AGENT_IMAGER_SPIKE_1_THETA_ITEM->number.value, &AGENT_IMAGER_SPIKE_2_RHO_ITEM->number.value, &AGENT_IMAGER_SPIKE_2_THETA_ITEM->number.value, &AGENT_IMAGER_SPIKE_3_RHO_ITEM->number.value, &AGENT_IMAGER_SPIKE_3_THETA_ITEM->number.value);
			} else {
				AGENT_IMAGER_STATS_BAHTINOV_ITEM->number.value = -1;
			}
			indigo_update_property(device, AGENT_IMAGER_SPIKES_PROPERTY, NULL);
		}
	}
	AGENT_IMAGER_STATS_FRAME_ITEM->number.value++;
	indigo_update_property(device, AGENT_IMAGER_STATS_PROPERTY, NULL);
	return true;
}

static void disable_solver(indigo_device *device) {
	char *related_agent_name = indigo_filter_first_related_agent_2(device, "Astrometry Agent", "ASTAP Agent");
	if (related_agent_name) {
		indigo_change_switch_property_1(FILTER_DEVICE_CONTEXT->client, related_agent_name, AGENT_PLATESOLVER_SOLVE_IMAGES_PROPERTY_NAME, AGENT_PLATESOLVER_SOLVE_IMAGES_DISABLED_ITEM_NAME, true);
	}
}

static void allow_abort_by_mount_agent(indigo_device *device, bool state) {
	char *related_agent_name = indigo_filter_first_related_agent(device, "Mount Agent");
	if (related_agent_name) {
		indigo_change_switch_property_1(FILTER_DEVICE_CONTEXT->client, related_agent_name, AGENT_ABORT_RELATED_PROCESS_PROPERTY_NAME, AGENT_ABORT_IMAGER_ITEM_NAME, state);
	}
}

static inline void set_backlash_if_overshoot(indigo_device *device, double backlash) {
	if ((DEVICE_PRIVATE_DATA->focuser_has_backlash) && (AGENT_IMAGER_FOCUS_BACKLASH_OVERSHOOT_ITEM->number.value > 1)) {
		indigo_change_number_property_1(FILTER_DEVICE_CONTEXT->client, device->name, FOCUSER_BACKLASH_PROPERTY_NAME, FOCUSER_BACKLASH_ITEM_NAME, backlash);
	}
}

static bool move_focuser(indigo_device *device, bool moving_out, double steps) {
	if (steps == 0) {
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "Not moving", "");
		return true;
	}
	indigo_property_state state = INDIGO_ALERT_STATE;
	indigo_change_switch_property_1(FILTER_DEVICE_CONTEXT->client, device->name, FOCUSER_DIRECTION_PROPERTY_NAME, moving_out ? FOCUSER_DIRECTION_MOVE_OUTWARD_ITEM_NAME : FOCUSER_DIRECTION_MOVE_INWARD_ITEM_NAME, true);
	indigo_change_number_property_1(FILTER_DEVICE_CONTEXT->client, device->name, FOCUSER_STEPS_PROPERTY_NAME, FOCUSER_STEPS_ITEM_NAME, steps);
	for (int i = 0; i < BUSY_TIMEOUT * 1000 && (state = DEVICE_PRIVATE_DATA->steps_state) != INDIGO_BUSY_STATE && AGENT_ABORT_PROCESS_PROPERTY->state != INDIGO_BUSY_STATE; i++)
		indigo_usleep(1000);
	if (AGENT_ABORT_PROCESS_PROPERTY->state == INDIGO_BUSY_STATE) {
		set_backlash_if_overshoot(device, DEVICE_PRIVATE_DATA->saved_backlash);
		return false;
	}
	if (state != INDIGO_BUSY_STATE) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "FOCUSER_STEPS_PROPERTY didn't become busy in %d second(s)", BUSY_TIMEOUT);
		set_backlash_if_overshoot(device, DEVICE_PRIVATE_DATA->saved_backlash);
		return false;
	}
	while ((state = DEVICE_PRIVATE_DATA->steps_state) == INDIGO_BUSY_STATE) {
		indigo_usleep(200000);
	}
	if (state != INDIGO_OK_STATE) {
		if (AGENT_ABORT_PROCESS_PROPERTY->state != INDIGO_BUSY_STATE)
			INDIGO_DRIVER_ERROR(DRIVER_NAME, "FOCUSER_STEPS_PROPERTY didn't become OK", "");
		set_backlash_if_overshoot(device, DEVICE_PRIVATE_DATA->saved_backlash);
		return false;
	}
	if (AGENT_ABORT_PROCESS_PROPERTY->state == INDIGO_BUSY_STATE) {
		set_backlash_if_overshoot(device, DEVICE_PRIVATE_DATA->saved_backlash);
		return false;
	}
	INDIGO_DRIVER_DEBUG(DRIVER_NAME, "Moving %s %f steps", moving_out ? "OUT" : "IN", steps);
	return true;
}

static bool move_focuser_with_overshoot_if_needed(indigo_device *device, bool move_out, double steps, double approx_backlash, bool apply_backlash) {
	double backlash_overshoot = AGENT_IMAGER_FOCUS_BACKLASH_OVERSHOOT_ITEM->number.value;
	double steps_todo;

	if (apply_backlash && (!DEVICE_PRIVATE_DATA->focuser_has_backlash || backlash_overshoot > 1.0)) {
		steps_todo = steps + approx_backlash * backlash_overshoot;
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "Moving %s %d steps (%d base + %d approx_backlash * %.2f overshoot)", move_out ? "OUT":"IN", (int)steps_todo, (int)steps, (int)approx_backlash, backlash_overshoot);
		if (!move_focuser(device, move_out, steps_todo)) {
			return false;
		}
		if (approx_backlash > 0 && backlash_overshoot > 1) {
			steps_todo = approx_backlash * backlash_overshoot;
			INDIGO_DRIVER_DEBUG(DRIVER_NAME, "Overshot by %d steps, compensating", (int)steps_todo);
			if (!move_focuser(device, !move_out, steps_todo)) {
				return false;
			}
		}
	} else {
		steps_todo = steps;
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "Moving %s %d steps", move_out ? "OUT":"IN", (int)steps_todo);
		if (!move_focuser(device, move_out, steps_todo)) {
			return false;
		}
	}
	return true;
}

static void capture(indigo_device *device) {
	int upload_mode = indigo_save_switch_state(device, CCD_UPLOAD_MODE_PROPERTY_NAME, CCD_UPLOAD_MODE_CLIENT_ITEM_NAME);
	int image_format = indigo_save_switch_state(device, CCD_IMAGE_FORMAT_PROPERTY_NAME, CCD_IMAGE_FORMAT_RAW_ITEM_NAME);
	bool result = capture_plain_frame(device);
	indigo_restore_switch_state(device, CCD_UPLOAD_MODE_PROPERTY_NAME, upload_mode);
	indigo_restore_switch_state(device, CCD_IMAGE_FORMAT_PROPERTY_NAME, image_format);
	if (AGENT_ABORT_PROCESS_PROPERTY->state == INDIGO_BUSY_STATE) {
		AGENT_ABORT_PROCESS_PROPERTY->state = INDIGO_OK_STATE;
		indigo_update_property(device, AGENT_ABORT_PROCESS_PROPERTY, NULL);
	}
	AGENT_IMAGER_CAPTURE_PROPERTY->state = result ? INDIGO_OK_STATE : INDIGO_ALERT_STATE;
	indigo_update_property(device, AGENT_IMAGER_CAPTURE_PROPERTY, NULL);
}

static void preview_1_process(indigo_device *device) {
	uint8_t *saturation_mask = NULL;
	FILTER_DEVICE_CONTEXT->running_process = true;
	clear_stats(device);
	allow_abort_by_mount_agent(device, false);
	disable_solver(device);
	int upload_mode = indigo_save_switch_state(device, CCD_UPLOAD_MODE_PROPERTY_NAME, CCD_UPLOAD_MODE_CLIENT_ITEM_NAME);
	int image_format = indigo_save_switch_state(device, CCD_IMAGE_FORMAT_PROPERTY_NAME, CCD_IMAGE_FORMAT_RAW_ITEM_NAME);
	capture_and_process_frame(device, &saturation_mask);
	indigo_restore_switch_state(device, CCD_UPLOAD_MODE_PROPERTY_NAME, upload_mode);
	indigo_restore_switch_state(device, CCD_IMAGE_FORMAT_PROPERTY_NAME, image_format);
	indigo_safe_free(saturation_mask);
	if (AGENT_ABORT_PROCESS_PROPERTY->state == INDIGO_BUSY_STATE) {
		AGENT_ABORT_PROCESS_PROPERTY->state = INDIGO_OK_STATE;
		indigo_update_property(device, AGENT_ABORT_PROCESS_PROPERTY, NULL);
	}
	AGENT_IMAGER_START_PREVIEW_1_ITEM->sw.value = false;
	AGENT_START_PROCESS_PROPERTY->state = AGENT_IMAGER_STATS_PROPERTY->state = INDIGO_OK_STATE;
	indigo_update_property(device, AGENT_IMAGER_STATS_PROPERTY, NULL);
	indigo_update_property(device, AGENT_START_PROCESS_PROPERTY, NULL);
	FILTER_DEVICE_CONTEXT->running_process = false;
}

static void preview_process(indigo_device *device) {
	uint8_t *saturation_mask = NULL;
	FILTER_DEVICE_CONTEXT->running_process = true;
	clear_stats(device);
	allow_abort_by_mount_agent(device, false);
	disable_solver(device);
	int upload_mode = indigo_save_switch_state(device, CCD_UPLOAD_MODE_PROPERTY_NAME, CCD_UPLOAD_MODE_CLIENT_ITEM_NAME);
	int image_format = indigo_save_switch_state(device, CCD_IMAGE_FORMAT_PROPERTY_NAME, CCD_IMAGE_FORMAT_RAW_ITEM_NAME);
	while (capture_and_process_frame(device, &saturation_mask))
		;
	indigo_restore_switch_state(device, CCD_UPLOAD_MODE_PROPERTY_NAME, upload_mode);
	indigo_restore_switch_state(device, CCD_IMAGE_FORMAT_PROPERTY_NAME, image_format);
	indigo_safe_free(saturation_mask);
	if (AGENT_ABORT_PROCESS_PROPERTY->state == INDIGO_BUSY_STATE) {
		AGENT_ABORT_PROCESS_PROPERTY->state = INDIGO_OK_STATE;
		indigo_update_property(device, AGENT_ABORT_PROCESS_PROPERTY, NULL);
	}
	AGENT_IMAGER_START_PREVIEW_ITEM->sw.value = false;
	AGENT_START_PROCESS_PROPERTY->state = AGENT_IMAGER_STATS_PROPERTY->state = INDIGO_OK_STATE;
	indigo_update_property(device, AGENT_IMAGER_STATS_PROPERTY, NULL);
	indigo_update_property(device, AGENT_START_PROCESS_PROPERTY, NULL);
	FILTER_DEVICE_CONTEXT->running_process = false;
}

static void check_breakpoint(indigo_device *device, indigo_item *breakpoint) {
	if (breakpoint->sw.value) {
		AGENT_PAUSE_PROCESS_PROPERTY->state = INDIGO_BUSY_STATE;
		AGENT_PAUSE_PROCESS_ITEM->sw.value = true;
		indigo_update_property(device, AGENT_PAUSE_PROCESS_PROPERTY, "%s paused on %s breakpoint", device->name, breakpoint->name);
		while (AGENT_PAUSE_PROCESS_PROPERTY->state == INDIGO_BUSY_STATE) {
			if (AGENT_ABORT_PROCESS_PROPERTY->state == INDIGO_BUSY_STATE) {
				indigo_update_property(device, AGENT_PAUSE_PROCESS_PROPERTY, "%s aborted on %s breakpoint", device->name, breakpoint->name);
				return;
			}
			if (AGENT_IMAGER_RESUME_CONDITION_BARRIER_ITEM->sw.value && DEVICE_PRIVATE_DATA->barrier_resume) {
				static const char *names[] = { AGENT_PAUSE_PROCESS_ITEM_NAME };
				const bool values[] = { false };
				for (int i = 0; i < AGENT_IMAGER_BARRIER_STATE_PROPERTY->count; i++) {
					indigo_item *item = AGENT_IMAGER_BARRIER_STATE_PROPERTY->items + i;
					indigo_change_switch_property(FILTER_DEVICE_CONTEXT->client, item->name, AGENT_PAUSE_PROCESS_PROPERTY_NAME, 1, names, values);
				}
				AGENT_PAUSE_PROCESS_ITEM->sw.value = false;
				AGENT_PAUSE_PROCESS_PROPERTY->state = INDIGO_OK_STATE;
				break;
			}
			indigo_usleep(1000);
		}
		indigo_update_property(device, AGENT_PAUSE_PROCESS_PROPERTY, "%s resumed on %s breakpoint", device->name, breakpoint->name);
	}
}

static bool do_dither(indigo_device *device) {
	char *related_agent_name = indigo_filter_first_related_agent(device, "Guider Agent");
	if (!related_agent_name) {
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "Dithering failed, no guider agent selected", "");
		indigo_send_message(device, "Error: Dithering failed, no guider agent selected");
		return true; // do not fail batch if dithering fails - let us keep it for a while
	}
	indigo_change_switch_property_1(FILTER_DEVICE_CONTEXT->client, related_agent_name, AGENT_GUIDER_DITHER_PROPERTY_NAME, AGENT_GUIDER_DITHER_TRIGGER_ITEM_NAME, true);
	DEVICE_PRIVATE_DATA->dithering_started = false;
	DEVICE_PRIVATE_DATA->dithering_finished = false;
	for (int i = 0; i < 15; i++) { // wait up to 3s to start dithering
		if (DEVICE_PRIVATE_DATA->dithering_started) {
			break;
		}
		if (AGENT_ABORT_PROCESS_PROPERTY->state == INDIGO_BUSY_STATE) {
			return false;
		}
		indigo_usleep(200000);
	}
	if (DEVICE_PRIVATE_DATA->dithering_started) {
		AGENT_IMAGER_STATS_PHASE_ITEM->number.value = INDIGO_IMAGER_PHASE_DITHERING;
		indigo_update_property(device, AGENT_IMAGER_STATS_PROPERTY, NULL);
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "Dithering started", "");
		double time_limit = 300 * 5; // 300 * 5 * 200ms = 300s
		for (int i = 0; i < time_limit; i++) { // wait up to time limit to finish dithering
			if (DEVICE_PRIVATE_DATA->dithering_finished) {
				INDIGO_DRIVER_DEBUG(DRIVER_NAME, "Dithering finished", "");
				break;
			}
			if (AGENT_ABORT_PROCESS_PROPERTY->state == INDIGO_BUSY_STATE) {
				return false;
			}
			indigo_usleep(200000);
		}
		if (!DEVICE_PRIVATE_DATA->dithering_finished) {
			INDIGO_DRIVER_DEBUG(DRIVER_NAME, "Dithering failed to settle down", "");
			indigo_send_message(device, "Error: Dithering failed to settle down");
			indigo_usleep(200000);
		}
	}
	return true;
}

static bool exposure_batch(indigo_device *device) {
	double time_to_transit = DEVICE_PRIVATE_DATA->time_to_transit;
	bool pauseOnTTT = AGENT_IMAGER_PAUSE_AFTER_TRANSIT_FEATURE_ITEM->sw.value && time_to_transit < 12;
	indigo_property_state state = INDIGO_ALERT_STATE;
	AGENT_IMAGER_STATS_EXPOSURE_ITEM->number.value = 0;
	AGENT_IMAGER_STATS_DELAY_ITEM->number.value = 0;
	AGENT_IMAGER_STATS_FRAME_ITEM->number.value = 0;
	AGENT_IMAGER_STATS_FRAMES_ITEM->number.value = AGENT_IMAGER_BATCH_COUNT_ITEM->number.target;
	AGENT_IMAGER_STATS_FRAMES_TO_DITHERING_ITEM->number.value = AGENT_IMAGER_BATCH_FRAMES_TO_SKIP_BEFORE_DITHER_ITEM->number.target;
	indigo_update_property(device, AGENT_IMAGER_STATS_PROPERTY, NULL);
	check_breakpoint(device, AGENT_IMAGER_BREAKPOINT_PRE_BATCH_ITEM);
	set_headers(device);
#ifdef __GNUC__
#pragma GCC diagnostic ignored "-Wunreachable-code"
#endif
	for (int remaining_exposures = (int)AGENT_IMAGER_BATCH_COUNT_ITEM->number.target; remaining_exposures != 0; remaining_exposures--) {
		AGENT_IMAGER_STATS_FRAME_ITEM->number.value++;
		AGENT_IMAGER_STATS_PHASE_ITEM->number.value = INDIGO_IMAGER_PHASE_CAPTURING;
		indigo_update_property(device, AGENT_IMAGER_STATS_PROPERTY, NULL);
		if (remaining_exposures < 0)
			remaining_exposures = -1;
		check_breakpoint(device, AGENT_IMAGER_BREAKPOINT_PRE_CAPTURE_ITEM);
		for (int exposure_attempt = 0; exposure_attempt < 3; exposure_attempt++) {
			bool pausedOnTTT = false;
			double exposure_time = AGENT_IMAGER_BATCH_EXPOSURE_ITEM->number.target;
			if (pauseOnTTT && indigo_filter_first_related_agent(device, "Mount Agent")) {
				time_to_transit = DEVICE_PRIVATE_DATA->time_to_transit;
				if (time_to_transit > 12)
					time_to_transit = time_to_transit - 24;
				if (time_to_transit <= exposure_time / 3600 - AGENT_IMAGER_BATCH_PAUSE_AFTER_TRANSIT_ITEM->number.target) {
					pauseOnTTT = false; // pause only once per batch
					clear_selection(device);
					AGENT_PAUSE_PROCESS_AFTER_TRANSIT_ITEM->sw.value = pausedOnTTT = true;
					AGENT_PAUSE_PROCESS_PROPERTY->state = INDIGO_BUSY_STATE;
					indigo_update_property(device, AGENT_PAUSE_PROCESS_PROPERTY, NULL);
					if (DEVICE_PRIVATE_DATA->time_to_transit >= 0) {
						indigo_send_message(device, "Batch paused, transit in %s", indigo_dtos(time_to_transit, NULL));
					} else {
						indigo_send_message(device, "Batch paused, transit %s ago", indigo_dtos(-time_to_transit, NULL));
					}
				}
			}
			if (AGENT_PAUSE_PROCESS_PROPERTY->state == INDIGO_BUSY_STATE) {
				allow_abort_by_mount_agent(device, false);
				while (AGENT_PAUSE_PROCESS_PROPERTY->state == INDIGO_BUSY_STATE) {
					indigo_usleep(200000);
				}
				allow_abort_by_mount_agent(device, true);
			}
			if (AGENT_ABORT_PROCESS_PROPERTY->state == INDIGO_BUSY_STATE) {
				return false;
			}
			if (DEVICE_PRIVATE_DATA->use_aux_1) {
				indigo_change_number_property_1(FILTER_DEVICE_CONTEXT->client, device->name, CCD_EXPOSURE_PROPERTY_NAME, CCD_EXPOSURE_ITEM_NAME, 0);
				indigo_change_number_property_1(FILTER_DEVICE_CONTEXT->client, device->name, "AUX_1_" CCD_EXPOSURE_PROPERTY_NAME, CCD_EXPOSURE_ITEM_NAME, exposure_time);
			} else {
				indigo_change_number_property_1(FILTER_DEVICE_CONTEXT->client, device->name, CCD_EXPOSURE_PROPERTY_NAME, CCD_EXPOSURE_ITEM_NAME, exposure_time);
			}
			for (int i = 0; i < BUSY_TIMEOUT * 1000 && DEVICE_PRIVATE_DATA->exposure_state != INDIGO_BUSY_STATE && DEVICE_PRIVATE_DATA->exposure_state != INDIGO_ALERT_STATE; i++) {
				indigo_usleep(1000);
			}
			if (AGENT_PAUSE_PROCESS_PROPERTY->state == INDIGO_BUSY_STATE) {
				while (AGENT_PAUSE_PROCESS_PROPERTY->state == INDIGO_BUSY_STATE)
					indigo_usleep(200000);
				if (AGENT_PAUSE_PROCESS_ITEM->sw.value) {
					exposure_attempt--;
					continue;
				}
			}
			if (AGENT_ABORT_PROCESS_PROPERTY->state == INDIGO_BUSY_STATE) {
				return false;
			}
			if (DEVICE_PRIVATE_DATA->exposure_state != INDIGO_BUSY_STATE) {
				INDIGO_DRIVER_ERROR(DRIVER_NAME, "CCD_EXPOSURE_PROPERTY didn't become busy in %d second(s)", BUSY_TIMEOUT);
				indigo_usleep(ONE_SECOND_DELAY);
				continue;
			}
			double reported_exposure_time = DEVICE_PRIVATE_DATA->remaining_exposure_time;
			AGENT_IMAGER_STATS_EXPOSURE_ITEM->number.value = reported_exposure_time;
			indigo_update_property(device, AGENT_IMAGER_STATS_PROPERTY, NULL);
			while ((state = DEVICE_PRIVATE_DATA->exposure_state) == INDIGO_BUSY_STATE) {
				if (reported_exposure_time != DEVICE_PRIVATE_DATA->remaining_exposure_time) {
					AGENT_IMAGER_STATS_EXPOSURE_ITEM->number.value = reported_exposure_time = DEVICE_PRIVATE_DATA->remaining_exposure_time;
					indigo_update_property(device, AGENT_IMAGER_STATS_PROPERTY, NULL);
				}
				if (reported_exposure_time > 1) {
					indigo_usleep(200000);
				} else {
					indigo_usleep(10000);
				}
			}
			if (AGENT_PAUSE_PROCESS_PROPERTY->state == INDIGO_BUSY_STATE) {
				while (AGENT_PAUSE_PROCESS_PROPERTY->state == INDIGO_BUSY_STATE)
					indigo_usleep(200000);
				if (AGENT_PAUSE_PROCESS_ITEM->sw.value) {
					exposure_attempt--;
					continue;
				}
			}
			if (AGENT_ABORT_PROCESS_PROPERTY->state == INDIGO_BUSY_STATE) {
				return false;
			}
			if (state != INDIGO_OK_STATE) {
				INDIGO_DRIVER_ERROR(DRIVER_NAME, "CCD_EXPOSURE_PROPERTY didn't become OK", "");
				indigo_usleep(ONE_SECOND_DELAY);
				continue;
			}
			check_breakpoint(device, AGENT_IMAGER_BREAKPOINT_POST_CAPTURE_ITEM);
			bool is_controlled_instance = false;
			if (!AGENT_IMAGER_RESUME_CONDITION_BARRIER_ITEM->sw.value) {
				// Do not dither if any breakpoint is set and resume condition is not set to barrier
				for (int i = 0; i < AGENT_IMAGER_BREAKPOINT_PROPERTY->count; i++) {
					if (AGENT_IMAGER_BREAKPOINT_PROPERTY->items[i].sw.value) {
						is_controlled_instance = true;
						break;
					}
				}
			}
			if (!is_controlled_instance) {
				if (remaining_exposures != 0) {
					if (DEVICE_PRIVATE_DATA->light_frame) {
						if (AGENT_IMAGER_STATS_FRAMES_TO_DITHERING_ITEM->number.value >= 0 && AGENT_IMAGER_ENABLE_DITHERING_FEATURE_ITEM->sw.value && (remaining_exposures > 1 || remaining_exposures == -1 || (remaining_exposures == 1 && AGENT_IMAGER_DITHER_AFTER_BATCH_FEATURE_ITEM->sw.value))) {
							if (AGENT_IMAGER_STATS_FRAMES_TO_DITHERING_ITEM->number.value > 0) {
								AGENT_IMAGER_STATS_FRAMES_TO_DITHERING_ITEM->number.value--;
							} else {
								AGENT_IMAGER_STATS_FRAMES_TO_DITHERING_ITEM->number.value = AGENT_IMAGER_BATCH_FRAMES_TO_SKIP_BEFORE_DITHER_ITEM->number.target;
								if (!do_dither(device)) {
									return false;
								}
							}
						} else {
							AGENT_IMAGER_STATS_FRAMES_TO_DITHERING_ITEM->number.value = AGENT_IMAGER_BATCH_FRAMES_TO_SKIP_BEFORE_DITHER_ITEM->number.target;
						}
					}
					check_breakpoint(device, AGENT_IMAGER_BREAKPOINT_PRE_DELAY_ITEM);
					double remaining_delay_time = AGENT_IMAGER_BATCH_DELAY_ITEM->number.target;
					AGENT_IMAGER_STATS_DELAY_ITEM->number.value = remaining_delay_time;
					AGENT_IMAGER_STATS_PHASE_ITEM->number.value = INDIGO_IMAGER_PHASE_WAITING;
					indigo_update_property(device, AGENT_IMAGER_STATS_PROPERTY, NULL);
					while (remaining_delay_time > 0) {
						while (AGENT_PAUSE_PROCESS_PROPERTY->state == INDIGO_BUSY_STATE) {
							indigo_usleep(200000);
						}
						if (AGENT_ABORT_PROCESS_PROPERTY->state == INDIGO_BUSY_STATE) {
							return false;
						}
						if (remaining_delay_time < floor(AGENT_IMAGER_STATS_DELAY_ITEM->number.value)) {
							double c = ceil(remaining_delay_time);
							if (AGENT_IMAGER_STATS_DELAY_ITEM->number.value > c) {
								AGENT_IMAGER_STATS_DELAY_ITEM->number.value = c;
								indigo_update_property(device, AGENT_IMAGER_STATS_PROPERTY, NULL);
							}
						}
						if (remaining_delay_time > 1) {
							remaining_delay_time -= 0.2;
							indigo_usleep(200000);
						} else {
							remaining_delay_time -= 0.01;
							indigo_usleep(10000);
						}
					}
					AGENT_IMAGER_STATS_DELAY_ITEM->number.value = 0;
					indigo_update_property(device, AGENT_IMAGER_STATS_PROPERTY, NULL);
					check_breakpoint(device, AGENT_IMAGER_BREAKPOINT_POST_DELAY_ITEM);
				}
			}
			break;
		}
	}
	check_breakpoint(device, AGENT_IMAGER_BREAKPOINT_POST_BATCH_ITEM);
	return true;
}

static void exposure_batch_process(indigo_device *device) {
	FILTER_DEVICE_CONTEXT->running_process = true;
	AGENT_IMAGER_STATS_BATCH_ITEM->number.value = 1;
	AGENT_IMAGER_STATS_BATCHES_ITEM->number.value = 1;
	AGENT_IMAGER_STATS_BATCH_INDEX_ITEM->number.value = 0;
	DEVICE_PRIVATE_DATA->dither_num = 0;
	allow_abort_by_mount_agent(device, true);
	disable_solver(device);
	indigo_send_message(device, "Batch started");
	if (AGENT_IMAGER_RESUME_CONDITION_BARRIER_ITEM->sw.value) {
		// Start batch on related imager agents
		// TBD: This is still race condition!
		indigo_property *related_agents_property = FILTER_DEVICE_CONTEXT->filter_related_agent_list_property;
		for (int i = 0; i < related_agents_property->count; i++) {
			indigo_item *item = related_agents_property->items + i;
			if (item->sw.value && !strncmp(item->name, "Imager Agent", 12)) {
				indigo_change_switch_property_1(FILTER_DEVICE_CONTEXT->client, item->name, AGENT_START_PROCESS_PROPERTY_NAME, AGENT_IMAGER_START_EXPOSURE_ITEM_NAME, true);
			}
		}
	}
	if (exposure_batch(device)) {
		AGENT_START_PROCESS_PROPERTY->state = AGENT_IMAGER_STATS_PROPERTY->state = INDIGO_OK_STATE;
		indigo_send_message(device, "Batch finished");
	} else {
		if (AGENT_ABORT_PROCESS_PROPERTY->state == INDIGO_BUSY_STATE) {
			AGENT_ABORT_PROCESS_PROPERTY->state = INDIGO_OK_STATE;
			indigo_update_property(device, AGENT_ABORT_PROCESS_PROPERTY, NULL);
			if (AGENT_IMAGER_BATCH_COUNT_ITEM->number.value == -1) {
				AGENT_START_PROCESS_PROPERTY->state = AGENT_IMAGER_STATS_PROPERTY->state = INDIGO_OK_STATE;
				indigo_send_message(device, "Batch finished");
			} else {
				AGENT_START_PROCESS_PROPERTY->state = INDIGO_ALERT_STATE;
				indigo_send_message(device, "Batch aborted");
			}
		} else {
			AGENT_START_PROCESS_PROPERTY->state = INDIGO_ALERT_STATE;
			indigo_send_message(device, "Batch failed");
		}
	}
	allow_abort_by_mount_agent(device, false);
	AGENT_IMAGER_START_EXPOSURE_ITEM->sw.value = false;
	AGENT_IMAGER_STATS_PHASE_ITEM->number.value = INDIGO_IMAGER_PHASE_IDLE;
	indigo_update_property(device, AGENT_IMAGER_STATS_PROPERTY, NULL);
	indigo_update_property(device, AGENT_START_PROCESS_PROPERTY, NULL);
	FILTER_DEVICE_CONTEXT->running_process = false;
}

static bool bracketing_batch(indigo_device *device) {
	indigo_property_state state = INDIGO_ALERT_STATE;
	AGENT_IMAGER_STATS_EXPOSURE_ITEM->number.value = 0;
	AGENT_IMAGER_STATS_DELAY_ITEM->number.value = 0;
	AGENT_IMAGER_STATS_FRAME_ITEM->number.value = 0;
	AGENT_IMAGER_STATS_FRAMES_ITEM->number.value = AGENT_IMAGER_BATCH_COUNT_ITEM->number.target;
	indigo_update_property(device, AGENT_IMAGER_STATS_PROPERTY, NULL);
	set_headers(device);
	double current_offset = 0;
	double step = fabs(AGENT_IMAGER_FOCUS_BRACKETING_STEP_ITEM->number.value);
	bool moving_out = AGENT_IMAGER_FOCUS_BRACKETING_STEP_ITEM->number.value > 0;
	for (int remaining_exposures = (int)AGENT_IMAGER_BATCH_COUNT_ITEM->number.target; remaining_exposures != 0; remaining_exposures--) {
		AGENT_IMAGER_STATS_FRAME_ITEM->number.value++;
		AGENT_IMAGER_STATS_PHASE_ITEM->number.value = INDIGO_IMAGER_PHASE_CAPTURING;
		indigo_update_property(device, AGENT_IMAGER_STATS_PROPERTY, NULL);
		if (remaining_exposures < 0)
			remaining_exposures = -1;
		for (int exposure_attempt = 0; exposure_attempt < 3; exposure_attempt++) {
			double exposure_time = AGENT_IMAGER_BATCH_EXPOSURE_ITEM->number.target;
			if (AGENT_PAUSE_PROCESS_PROPERTY->state == INDIGO_BUSY_STATE) {
				while (AGENT_PAUSE_PROCESS_PROPERTY->state == INDIGO_BUSY_STATE) {
					indigo_usleep(200000);
				}
			}
			if (AGENT_ABORT_PROCESS_PROPERTY->state == INDIGO_BUSY_STATE) {
				move_focuser(device, !moving_out, current_offset);
				return false;
			}
			indigo_change_number_property_1(FILTER_DEVICE_CONTEXT->client, device->name, CCD_EXPOSURE_PROPERTY_NAME, CCD_EXPOSURE_ITEM_NAME, exposure_time);
			for (int i = 0; i < BUSY_TIMEOUT * 1000 && DEVICE_PRIVATE_DATA->exposure_state != INDIGO_BUSY_STATE && DEVICE_PRIVATE_DATA->exposure_state != INDIGO_ALERT_STATE; i++) {
				indigo_usleep(1000);
			}
			if (AGENT_PAUSE_PROCESS_PROPERTY->state == INDIGO_BUSY_STATE) {
				while (AGENT_PAUSE_PROCESS_PROPERTY->state == INDIGO_BUSY_STATE)
					indigo_usleep(200000);
				if (AGENT_PAUSE_PROCESS_ITEM->sw.value) {
					exposure_attempt--;
					continue;
				}
			}
			if (AGENT_ABORT_PROCESS_PROPERTY->state == INDIGO_BUSY_STATE) {
				move_focuser(device, !moving_out, current_offset);
				return false;
			}
			if (DEVICE_PRIVATE_DATA->exposure_state != INDIGO_BUSY_STATE) {
				INDIGO_DRIVER_ERROR(DRIVER_NAME, "CCD_EXPOSURE_PROPERTY didn't become busy in %d second(s)", BUSY_TIMEOUT);
				indigo_usleep(ONE_SECOND_DELAY);
				continue;
			}
			double reported_exposure_time = DEVICE_PRIVATE_DATA->remaining_exposure_time;
			AGENT_IMAGER_STATS_EXPOSURE_ITEM->number.value = reported_exposure_time;
			indigo_update_property(device, AGENT_IMAGER_STATS_PROPERTY, NULL);
			while ((state = DEVICE_PRIVATE_DATA->exposure_state) == INDIGO_BUSY_STATE) {
				if (reported_exposure_time != DEVICE_PRIVATE_DATA->remaining_exposure_time) {
					AGENT_IMAGER_STATS_EXPOSURE_ITEM->number.value = reported_exposure_time = DEVICE_PRIVATE_DATA->remaining_exposure_time;
					indigo_update_property(device, AGENT_IMAGER_STATS_PROPERTY, NULL);
				}
				if (reported_exposure_time > 1) {
					indigo_usleep(200000);
				} else {
					indigo_usleep(10000);
				}
			}
			if (AGENT_PAUSE_PROCESS_PROPERTY->state == INDIGO_BUSY_STATE) {
				while (AGENT_PAUSE_PROCESS_PROPERTY->state == INDIGO_BUSY_STATE)
					indigo_usleep(200000);
				if (AGENT_PAUSE_PROCESS_ITEM->sw.value) {
					exposure_attempt--;
					continue;
				}
			}
			if (AGENT_ABORT_PROCESS_PROPERTY->state == INDIGO_BUSY_STATE) {
				move_focuser(device, !moving_out, current_offset);
				return false;
			}
			if (state != INDIGO_OK_STATE) {
				INDIGO_DRIVER_ERROR(DRIVER_NAME, "CCD_EXPOSURE_PROPERTY didn't become OK", "");
				indigo_usleep(ONE_SECOND_DELAY);
				continue;
			}
			break;
		}
		if (remaining_exposures != 1) {
			if (move_focuser(device, moving_out, step)) {
				current_offset += step;
				indigo_usleep(500000);
			} else {
				move_focuser(device, !moving_out, current_offset);
				return false;
			}
		}
	}
	move_focuser(device, !moving_out, current_offset);
	return true;
}

static void bracketing_batch_process(indigo_device *device) {
	FILTER_DEVICE_CONTEXT->running_process = true;
	AGENT_IMAGER_STATS_BATCH_ITEM->number.value = 1;
	AGENT_IMAGER_STATS_BATCHES_ITEM->number.value = 1;
	AGENT_IMAGER_STATS_BATCH_INDEX_ITEM->number.value = 0;
	allow_abort_by_mount_agent(device, false);
	disable_solver(device);
	indigo_send_message(device, "Bracketing batch started");
	if (bracketing_batch(device)) {
		AGENT_START_PROCESS_PROPERTY->state = AGENT_IMAGER_STATS_PROPERTY->state = INDIGO_OK_STATE;
		indigo_send_message(device, "Bracketing batch finished");
	} else {
		if (AGENT_ABORT_PROCESS_PROPERTY->state == INDIGO_BUSY_STATE) {
			AGENT_ABORT_PROCESS_PROPERTY->state = INDIGO_OK_STATE;
			indigo_update_property(device, AGENT_ABORT_PROCESS_PROPERTY, NULL);
			if (AGENT_IMAGER_BATCH_COUNT_ITEM->number.value == -1) {
				AGENT_START_PROCESS_PROPERTY->state = AGENT_IMAGER_STATS_PROPERTY->state = INDIGO_OK_STATE;
				indigo_send_message(device, "Bracketing batch finished");
			} else {
				AGENT_START_PROCESS_PROPERTY->state = INDIGO_ALERT_STATE;
				indigo_send_message(device, "Bracketing batch aborted");
			}
		} else {
			AGENT_START_PROCESS_PROPERTY->state = INDIGO_ALERT_STATE;
			indigo_send_message(device, "Bracketing batch failed");
		}
	}
	AGENT_IMAGER_START_EXPOSURE_ITEM->sw.value = false;
	AGENT_IMAGER_STATS_PHASE_ITEM->number.value = INDIGO_IMAGER_PHASE_IDLE;
	indigo_update_property(device, AGENT_IMAGER_STATS_PROPERTY, NULL);
	indigo_update_property(device, AGENT_START_PROCESS_PROPERTY, NULL);
	FILTER_DEVICE_CONTEXT->running_process = false;
}

static bool streaming_batch(indigo_device *device) {
	indigo_property_state state = INDIGO_ALERT_STATE;
	AGENT_IMAGER_STATS_EXPOSURE_ITEM->number.value = 0;
	AGENT_IMAGER_STATS_DELAY_ITEM->number.value = 0;
	AGENT_IMAGER_STATS_FRAME_ITEM->number.value = 0;
	AGENT_IMAGER_STATS_FRAMES_ITEM->number.value = AGENT_IMAGER_BATCH_COUNT_ITEM->number.target;
	indigo_update_property(device, AGENT_IMAGER_STATS_PROPERTY, NULL);
	set_headers(device);
	static char const *names[] = { AGENT_IMAGER_BATCH_COUNT_ITEM_NAME, AGENT_IMAGER_BATCH_EXPOSURE_ITEM_NAME };
	double values[] = { AGENT_IMAGER_BATCH_COUNT_ITEM->number.target, AGENT_IMAGER_BATCH_EXPOSURE_ITEM->number.target };
	indigo_change_number_property(FILTER_DEVICE_CONTEXT->client, device->name, CCD_STREAMING_PROPERTY_NAME, 2, names, values);
	for (int i = 0; i < BUSY_TIMEOUT * 1000 && (state = DEVICE_PRIVATE_DATA->streaming_state) != INDIGO_BUSY_STATE && AGENT_ABORT_PROCESS_PROPERTY->state != INDIGO_BUSY_STATE && AGENT_PAUSE_PROCESS_PROPERTY->state != INDIGO_BUSY_STATE; i++)
		indigo_usleep(1000);
	if (AGENT_PAUSE_PROCESS_PROPERTY->state == INDIGO_BUSY_STATE || AGENT_ABORT_PROCESS_PROPERTY->state == INDIGO_BUSY_STATE)
		return false;
	if (state != INDIGO_BUSY_STATE) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "CCD_STREAMING_PROPERTY didn't become busy in %d second(s)", BUSY_TIMEOUT);
		return false;
	}
	while ((state = DEVICE_PRIVATE_DATA->streaming_state) == INDIGO_BUSY_STATE) {
		indigo_usleep(20000);
		int count = (int)DEVICE_PRIVATE_DATA->remaining_streaming_count;
		if (count != AGENT_IMAGER_STATS_FRAME_ITEM->number.value) {
			AGENT_IMAGER_STATS_FRAME_ITEM->number.value = count;
			indigo_update_property(device, AGENT_IMAGER_STATS_PROPERTY, NULL);
		}
	}
	if (AGENT_PAUSE_PROCESS_PROPERTY->state == INDIGO_BUSY_STATE || AGENT_ABORT_PROCESS_PROPERTY->state == INDIGO_BUSY_STATE)
		return false;
	return true;
}

static void streaming_batch_process(indigo_device *device) {
	FILTER_DEVICE_CONTEXT->running_process = true;
	AGENT_IMAGER_STATS_BATCH_ITEM->number.value = 1;
	AGENT_IMAGER_STATS_BATCHES_ITEM->number.value = 1;
	AGENT_IMAGER_STATS_BATCH_INDEX_ITEM->number.value = 0;
	allow_abort_by_mount_agent(device, true);
	disable_solver(device);
	indigo_send_message(device, "Streaming started");
	if (streaming_batch(device)) {
		AGENT_START_PROCESS_PROPERTY->state = AGENT_IMAGER_STATS_PROPERTY->state = INDIGO_OK_STATE;
		indigo_send_message(device, "Streaming finished");
	} else {
		if (AGENT_ABORT_PROCESS_PROPERTY->state == INDIGO_BUSY_STATE) {
			AGENT_ABORT_PROCESS_PROPERTY->state = INDIGO_OK_STATE;
			indigo_update_property(device, AGENT_ABORT_PROCESS_PROPERTY, NULL);
			if (AGENT_IMAGER_BATCH_COUNT_ITEM->number.value == -1) {
				AGENT_START_PROCESS_PROPERTY->state = AGENT_IMAGER_STATS_PROPERTY->state = INDIGO_OK_STATE;
				indigo_send_message(device, "Streaming finished");
			} else {
				AGENT_START_PROCESS_PROPERTY->state = INDIGO_ALERT_STATE;
				indigo_send_message(device, "Streaming aborted");
			}
		} else {
			AGENT_START_PROCESS_PROPERTY->state = INDIGO_ALERT_STATE;
			indigo_send_message(device, "Streaming failed");
		}
	}
	allow_abort_by_mount_agent(device, false);
	AGENT_IMAGER_START_STREAMING_ITEM->sw.value = false;
	indigo_update_property(device, AGENT_IMAGER_STATS_PROPERTY, NULL);
	indigo_update_property(device, AGENT_START_PROCESS_PROPERTY, NULL);
	FILTER_DEVICE_CONTEXT->running_process = false;
}

static double estimator(indigo_device *device) {
	if (DEVICE_PRIVATE_DATA->use_rms_estimator) {
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "RMS contrast = %g", AGENT_IMAGER_STATS_RMS_CONTRAST_ITEM->number.value);
		if (AGENT_IMAGER_STATS_RMS_CONTRAST_ITEM->number.value == 0) {
			return NAN;
		}
		return AGENT_IMAGER_STATS_RMS_CONTRAST_ITEM->number.value;
	} else if (DEVICE_PRIVATE_DATA->use_hfd_estimator) {
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "Peak = %g, HFD = %g, FWHM = %g", AGENT_IMAGER_STATS_PEAK_ITEM->number.value, AGENT_IMAGER_STATS_HFD_ITEM->number.value, AGENT_IMAGER_STATS_FWHM_ITEM->number.value);
		if (AGENT_IMAGER_STATS_HFD_ITEM->number.value == 0) {
			return NAN;
		}
		return AGENT_IMAGER_STATS_PEAK_ITEM->number.value / AGENT_IMAGER_STATS_HFD_ITEM->number.value;
	} else if (DEVICE_PRIVATE_DATA->use_bahtinov_estimator) {
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "Bahtinov focus error = %g", AGENT_IMAGER_STATS_BAHTINOV_ITEM->number.value);
		if (AGENT_IMAGER_STATS_BAHTINOV_ITEM->number.value < 0 || AGENT_IMAGER_STATS_BAHTINOV_ITEM->number.value > 100) {
			return NAN;
		}
		return 100 - AGENT_IMAGER_STATS_BAHTINOV_ITEM->number.value;
	}
	return NAN;
}

static bool autofocus_iterative(indigo_device *device, uint8_t **saturation_mask) {
	bool repeat = true;
	double min_est = 1e10, max_est = 0;
	double last_quality = 0;
	double steps = AGENT_IMAGER_FOCUS_ITERATIVE_INITIAL_ITEM->number.value;
	int current_offset = 0;
	int limit = (int)(DEVICE_PRIVATE_DATA->use_hfd_estimator ? AF_MOVE_LIMIT_HFD * AGENT_IMAGER_FOCUS_ITERATIVE_INITIAL_ITEM->number.value : AF_MOVE_LIMIT_RMS * AGENT_IMAGER_FOCUS_ITERATIVE_INITIAL_ITEM->number.value);

	INDIGO_DRIVER_DEBUG(DRIVER_NAME, "focuser_has_backlash = %d", DEVICE_PRIVATE_DATA->focuser_has_backlash);

	bool moving_out = true, first_move = true;
	indigo_change_switch_property_1(FILTER_DEVICE_CONTEXT->client, device->name, CCD_UPLOAD_MODE_PROPERTY_NAME, CCD_UPLOAD_MODE_CLIENT_ITEM_NAME, true);
	indigo_change_switch_property_1(FILTER_DEVICE_CONTEXT->client, device->name, FOCUSER_DIRECTION_PROPERTY_NAME, FOCUSER_DIRECTION_MOVE_OUTWARD_ITEM_NAME, true);

	DEVICE_PRIVATE_DATA->saved_backlash = AGENT_IMAGER_FOCUS_BACKLASH_ITEM->number.value;
	set_backlash_if_overshoot(device, 0);
	while (repeat) {
		if (AGENT_PAUSE_PROCESS_PROPERTY->state == INDIGO_BUSY_STATE) {
			while (AGENT_PAUSE_PROCESS_PROPERTY->state == INDIGO_BUSY_STATE)
				indigo_usleep(200000);
			continue;
		}
		if (AGENT_ABORT_PROCESS_PROPERTY->state == INDIGO_BUSY_STATE) {
			set_backlash_if_overshoot(device, DEVICE_PRIVATE_DATA->saved_backlash);
			return false;
		}
		int frame_count = 0;
		double quality = 0;
		for (int i = 0; i < 20 && frame_count < AGENT_IMAGER_FOCUS_STACK_ITEM->number.value; i++) {
			// capture_and_process_frame can fail either because of driver error or because abort, we should repeat also if frame is saturated
			if (!capture_and_process_frame(device, saturation_mask) || DEVICE_PRIVATE_DATA->frame_saturated) {
				if (AGENT_ABORT_PROCESS_PROPERTY->state == INDIGO_BUSY_STATE) {
					set_backlash_if_overshoot(device, DEVICE_PRIVATE_DATA->saved_backlash);
					return false;
				} else {
					continue;
				}
			}
			double current_quality = estimator(device);
			if (isnan(current_quality))
				continue;
			quality = quality > current_quality ? quality : current_quality;
			INDIGO_DRIVER_DEBUG(DRIVER_NAME, "quality = %g, current_quality = %g", quality, current_quality);
			frame_count++;
		}
		if (frame_count == 0 || quality == 0) {
			indigo_send_message(device, "Failed to evaluate quality");
			break;
		}
		min_est = (min_est > quality) ? quality : min_est;
		if (DEVICE_PRIVATE_DATA->frame_saturated) {
			max_est = quality;
		} else {
			max_est = (max_est < quality) ? quality : max_est;
		}
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "Focus Quality = %g %s", quality, DEVICE_PRIVATE_DATA->frame_saturated ? "(saturated)" : "");
		if (quality >= last_quality && abs(current_offset) < limit) {
			if (!move_focuser_with_overshoot_if_needed(device, moving_out, steps, DEVICE_PRIVATE_DATA->saved_backlash, moving_out))
				break;
			if (moving_out) {
				current_offset += (int)steps;
			} else {
				current_offset -= (int)steps;
			}
		} else if (steps <= AGENT_IMAGER_FOCUS_ITERATIVE_FINAL_ITEM->number.value || abs(current_offset) >= limit) {
			INDIGO_DRIVER_DEBUG(DRIVER_NAME, "Current_offset %d steps", (int)current_offset);
			if (
				(AGENT_IMAGER_STATS_HFD_ITEM->number.value > 1.2 * AGENT_IMAGER_SELECTION_RADIUS_ITEM->number.value && DEVICE_PRIVATE_DATA->use_hfd_estimator) ||
				(abs(current_offset) >= limit && DEVICE_PRIVATE_DATA->use_rms_estimator)
			) {
				break;
			} else {
				moving_out = !moving_out;
				if (moving_out) {
					INDIGO_DRIVER_DEBUG(DRIVER_NAME, "Switching and moving OUT %d steps to final position", (int)steps);
					current_offset += (int)steps;
				} else {
					INDIGO_DRIVER_DEBUG(DRIVER_NAME, "Switching and moving IN %d steps to final position", (int)steps);
					current_offset -= (int)steps;
				}
				if (!move_focuser_with_overshoot_if_needed(device, moving_out, steps, DEVICE_PRIVATE_DATA->saved_backlash, moving_out))
					break;
			}
			repeat = false;
		} else {
			moving_out = !moving_out;
			if (!first_move) {
				steps = round(steps / 2);
				if (steps < 1)
					steps = 1;
			}
			first_move = false;
			if (moving_out) {
				INDIGO_DRIVER_DEBUG(DRIVER_NAME, "Switching and moving OUT %d steps", (int)steps);
				current_offset += (int)steps;
			} else {
				INDIGO_DRIVER_DEBUG(DRIVER_NAME, "Switching and moving IN %d steps", (int)steps);
				current_offset -= (int)steps;
			}
			if (!move_focuser_with_overshoot_if_needed(device, moving_out, steps, DEVICE_PRIVATE_DATA->saved_backlash, moving_out)) break;
		}
		AGENT_IMAGER_STATS_FOCUS_OFFSET_ITEM->number.value = current_offset;
		indigo_update_property(device, AGENT_IMAGER_STATS_PROPERTY, NULL);
		last_quality = quality;
	}
	if (!capture_and_process_frame(device, saturation_mask)) {
		return false;
	}
	if (AGENT_ABORT_PROCESS_PROPERTY->state == INDIGO_BUSY_STATE) {
		set_backlash_if_overshoot(device, DEVICE_PRIVATE_DATA->saved_backlash);
		return false;
	}
	// Calculate focus deviation from best
	if (max_est > min_est) {
		AGENT_IMAGER_STATS_FOCUS_DEVIATION_ITEM->number.value = 100 * (max_est - estimator(device)) / (max_est - min_est);
	} else {
		AGENT_IMAGER_STATS_FOCUS_DEVIATION_ITEM->number.value = 100;
	}
	indigo_update_property(device, AGENT_IMAGER_STATS_PROPERTY, NULL);
	bool focus_failed = false;
	if (abs(current_offset) >= limit) {
		indigo_send_message(device, "No focus reached within maximum travel limit per AF run");
		focus_failed = true;
	} else if (DEVICE_PRIVATE_DATA->use_hfd_estimator) {
		if (AGENT_IMAGER_STATS_HFD_ITEM->number.value > 1.2 * AGENT_IMAGER_SELECTION_RADIUS_ITEM->number.value) {
			indigo_send_message(device, "No focus reached, did not converge");
			focus_failed = true;
		} else if (AGENT_IMAGER_STATS_FOCUS_DEVIATION_ITEM->number.value > 20 /* for HFD 20% deviation is ok - tested on realsky */ || AGENT_IMAGER_STATS_FOCUS_DEVIATION_ITEM->number.value > 25 /* for RMS 25% deviation is ok - tested on realsky */) {
			indigo_send_message(device, "Focus does not meet the quality criteria");
			focus_failed = true;
		}
	} else if (isnan(estimator(device))) {
		indigo_send_message(device, "No focus reached, did not converge");
		focus_failed = true;
	}
	INDIGO_DRIVER_DEBUG(DRIVER_NAME, "Focus deviation = %g %%", AGENT_IMAGER_STATS_FOCUS_DEVIATION_ITEM->number.value);
	if (focus_failed) {
		set_backlash_if_overshoot(device, DEVICE_PRIVATE_DATA->saved_backlash);
		if (DEVICE_PRIVATE_DATA->restore_initial_position) {
			indigo_send_message(device, "Focus failed, restoring initial position");
			INDIGO_DRIVER_ERROR(DRIVER_NAME, "Failed to reach focus, moving to initial position %d steps", (int)current_offset);
			if (current_offset > 0) {
				if (moving_out && !DEVICE_PRIVATE_DATA->focuser_has_backlash) {
					current_offset += (int)AGENT_IMAGER_FOCUS_BACKLASH_ITEM->number.value;
				}
				move_focuser(device, false, current_offset);
			} else if (current_offset < 0) {
				current_offset = -current_offset;
				if (!moving_out && !DEVICE_PRIVATE_DATA->focuser_has_backlash) {
					current_offset += (int)AGENT_IMAGER_FOCUS_BACKLASH_ITEM->number.value;
				}
				move_focuser(device, true, current_offset);
			}
			current_offset = 0;
		}
		return false;
	} else {
		set_backlash_if_overshoot(device, DEVICE_PRIVATE_DATA->saved_backlash);
		return true;
	}
}

static int quality_comparator(double *quality1, double *quality2, int count) {
	int comparison = 0;
	for (int i = 0; i < count; i++) {
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "COMAPRE Q1[%d] = %f Q2[%d] = %f", i, quality1[i], i, quality2[i]);
		if (quality1[i] > quality2[i]) {
			comparison ++;
		} else if (quality1[i] < quality2[i]) {
			comparison --;
		}
	}
	INDIGO_DRIVER_DEBUG(DRIVER_NAME, "Quality comparison = %d", comparison);
	return comparison; // ==0 - equal, > 0 - quality1 better, < 0 - quality2 better
}

static bool quality_is_zero(double *quality, int count) {
	for (int i = 0; i < count; i++) {
		if (quality[i] != 0) {
			INDIGO_DRIVER_DEBUG(DRIVER_NAME, "Quality[%d] = %g -> is_zero = false", i, quality[i]);
			return false;
		}
	}
	return true;
}

static bool quality_has_zero(double *quality, int count) {
	for (int i = 0; i < count; i++) {
		if (quality[i] == 0) {
			INDIGO_DRIVER_DEBUG(DRIVER_NAME, "Quality[%d] = %g -> has_zero = true", i, quality[i]);
			return true;
		}
	}
	return false;
}

static int calculate_mode(int arr[], int n) {
	int max_count = 0, mode = arr[0];
	for (int i = 0; i < n; ++i) {
		int count = 0;
		for (int j = 0; j < n; ++j) {
			if (arr[j] == arr[i]) {
				++count;
			}
		}
		if (count > max_count) {
			max_count = count;
			mode = arr[i];
		}
	}
	return mode;
}

static double reduce_ucurve_best_focus(double *best_focuses, int count) {
	double avg_best_focus = 0;
	for(int i = 0; i < count; i++) {
		avg_best_focus += best_focuses[i];
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "UC: Best focus for star #%d is at position %.3f", i, best_focuses[i]);
	}
	avg_best_focus /= count;
	INDIGO_DRIVER_DEBUG(DRIVER_NAME, "UC: Average best focus = %f", avg_best_focus);
	if (count < 4) {
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "UC: Not enough stars to calculate standard deviation using average best focus", "");
		return avg_best_focus;
	}
	double best_focus_stddev = indigo_stddev(best_focuses, count);
	INDIGO_DRIVER_DEBUG(DRIVER_NAME, "UC: Best focus standard deviation = %f", best_focus_stddev);
	double best_focus = 0;
	int used_values = 0;
	for (int i = 0; i < count; i++) {
		double focus_diff = fabs(best_focuses[i] - avg_best_focus);
		if (focus_diff < 3 * best_focus_stddev) {
			best_focus += best_focuses[i];
			used_values++;
			INDIGO_DRIVER_DEBUG(DRIVER_NAME, "UC (+): Using best focus for star #%d at position %.3f", i, best_focuses[i]);
		} else {
			INDIGO_DRIVER_DEBUG(DRIVER_NAME, "UC (-): Ignoring outlier best focus for star #%d at position %.3f", i, best_focuses[i]);
		}
	}
	best_focus /= used_values;
	INDIGO_DRIVER_DEBUG(DRIVER_NAME, "UC: Best focus after outlier removal = %f", best_focus);
	return best_focus;
}

static bool autofocus_ucurve(indigo_device *device) {
	assert(DEVICE_PRIVATE_DATA->use_hfd_estimator);
	double prev_quality[INDIGO_MAX_MULTISTAR_COUNT] = { 0 }, min_est = 1e10;
	double steps = AGENT_IMAGER_FOCUS_UCURVE_STEP_ITEM->number.value;
	int current_offset = 0;
	DEVICE_PRIVATE_DATA->ucurve_samples_number = (int)rint(AGENT_IMAGER_FOCUS_UCURVE_SAMPLES_ITEM->number.value);
	DEVICE_PRIVATE_DATA->saved_backlash = AGENT_IMAGER_FOCUS_BACKLASH_ITEM->number.value;
	int limit = (int)(AF_MOVE_LIMIT_UCURVE * AGENT_IMAGER_FOCUS_UCURVE_STEP_ITEM->number.value * DEVICE_PRIVATE_DATA->ucurve_samples_number);
	bool moving_out = true;
	int sample = 0;
	int sample_index = 0;
	double best_value = 0;
	int best_index = 0;

	indigo_change_switch_property_1(FILTER_DEVICE_CONTEXT->client, device->name, CCD_UPLOAD_MODE_PROPERTY_NAME, CCD_UPLOAD_MODE_CLIENT_ITEM_NAME, true);
	indigo_change_switch_property_1(FILTER_DEVICE_CONTEXT->client, device->name, FOCUSER_DIRECTION_PROPERTY_NAME, FOCUSER_DIRECTION_MOVE_OUTWARD_ITEM_NAME, true);
	set_backlash_if_overshoot(device, 0);

	INDIGO_DRIVER_DEBUG(DRIVER_NAME, "UC: focuser_has_backlash = %d", DEVICE_PRIVATE_DATA->focuser_has_backlash);

	bool repeat = true;
	bool focus_failed = false;
	double hfds[INDIGO_MAX_MULTISTAR_COUNT][MAX_UCURVE_SAMPLES] = {0};
	double focus_pos[MAX_UCURVE_SAMPLES] = {0};
	int star_count = (int)AGENT_IMAGER_SELECTION_STAR_COUNT_ITEM->number.value;
	int mid_index = (int)(rint(DEVICE_PRIVATE_DATA->ucurve_samples_number / 2.0) - 1);
	int ucurve_samples = (int)DEVICE_PRIVATE_DATA->ucurve_samples_number;
	while (repeat) {
		if (AGENT_PAUSE_PROCESS_PROPERTY->state == INDIGO_BUSY_STATE) {
			while (AGENT_PAUSE_PROCESS_PROPERTY->state == INDIGO_BUSY_STATE)
				indigo_usleep(200000);
			continue;
		}
		if (AGENT_ABORT_PROCESS_PROPERTY->state == INDIGO_BUSY_STATE) {
			set_backlash_if_overshoot(device, DEVICE_PRIVATE_DATA->saved_backlash);
			return false;
		}
		double quality[INDIGO_MAX_MULTISTAR_COUNT] = {0};
		int frame_count = 0;
		for (int i = 0; i < 3 && frame_count < AGENT_IMAGER_FOCUS_STACK_ITEM->number.value; i++) {
			if (!capture_and_process_frame(device, NULL)) {
				if (AGENT_ABORT_PROCESS_PROPERTY->state == INDIGO_BUSY_STATE) {
					set_backlash_if_overshoot(device, DEVICE_PRIVATE_DATA->saved_backlash);
					return false;
				} else {
					continue;
				}
			}
			indigo_update_property(device, AGENT_IMAGER_STATS_PROPERTY, NULL);

			if (AGENT_IMAGER_STATS_HFD_ITEM->number.value == 0 || AGENT_IMAGER_STATS_FWHM_ITEM->number.value == 0) {
				INDIGO_DRIVER_DEBUG(
					DRIVER_NAME,
					"UC: Peak = %g, HFD = %g, FWHM = %g",
					AGENT_IMAGER_STATS_PEAK_ITEM->number.value,
					AGENT_IMAGER_STATS_HFD_ITEM->number.value,
					AGENT_IMAGER_STATS_FWHM_ITEM->number.value
				);
				continue;
			}
			double current_quality[INDIGO_MAX_MULTISTAR_COUNT] = {0};
			for (int n = 0; n < star_count; n++) {
				if (AGENT_IMAGER_STATS_HFD_ITEM[n].number.value == 0) {
					current_quality[n] = 0;
				} else {
					current_quality[n] = 1 / AGENT_IMAGER_STATS_HFD_ITEM[n].number.value;
				}
			}
			if (quality_comparator(quality, current_quality, star_count) < 0) {
				memcpy(quality, current_quality, sizeof(double) * star_count);
			}

			INDIGO_DRIVER_DEBUG(DRIVER_NAME, "UC: Peak = %g, HFD = %g, FWHM = %g, current_quality = %g, best_quality = %g", AGENT_IMAGER_STATS_PEAK_ITEM->number.value, AGENT_IMAGER_STATS_HFD_ITEM->number.value, AGENT_IMAGER_STATS_FWHM_ITEM->number.value, current_quality[0], quality[0]);
			frame_count++;
		}
		/* Check if there is at least one star with measured HFD (qiality), if not fail */
		if (quality_is_zero(quality, star_count)){
			indigo_send_message(device, "Error: No stars detected, maybe focus is too far");
			INDIGO_DRIVER_ERROR(DRIVER_NAME, "UC: No stars detected, maybe focus is too far (frame_count = %d)", frame_count);
			focus_failed = true;
			goto ucurve_finish;
		}
		min_est = (min_est > AGENT_IMAGER_STATS_HFD_ITEM->number.value) ? AGENT_IMAGER_STATS_HFD_ITEM->number.value : min_est;
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "UC: sample = %d : Focus Quality = %g (Previous %g)", sample, quality[0], prev_quality[0]);
		if (sample == 0) {
			INDIGO_DRIVER_DEBUG(DRIVER_NAME, "UC: Moving OUT to detect best focus direction", "");
			moving_out = true;
			if (!move_focuser_with_overshoot_if_needed(device, moving_out, steps, DEVICE_PRIVATE_DATA->saved_backlash, true)) break;
			current_offset += (int)steps;
		} else if (sample == 1) {
			double steps_to_move = 0;
			if (quality_comparator(prev_quality, quality, star_count) >= 0) {
				steps_to_move = steps * (mid_index + 1);
				moving_out = true;
				INDIGO_DRIVER_DEBUG(DRIVER_NAME, "UC: Moving OUT %g steps to defocus sufficiently", steps_to_move);
				if (!move_focuser_with_overshoot_if_needed(device, moving_out, steps_to_move, DEVICE_PRIVATE_DATA->saved_backlash, true)) break;
				current_offset += (int)steps_to_move;
			} else {
				steps_to_move = steps * (mid_index + 2);
				moving_out = false;
				INDIGO_DRIVER_DEBUG(DRIVER_NAME, "UC: Moving IN %g steps to defocus sufficiently", steps_to_move);
				if (!move_focuser_with_overshoot_if_needed(device, moving_out, steps_to_move, DEVICE_PRIVATE_DATA->saved_backlash, false)) break;
				current_offset -= (int)steps_to_move;
			}
			moving_out = !moving_out;
		} else {
			if (sample == 2) {
				INDIGO_DRIVER_DEBUG(DRIVER_NAME, "UC: Starting to collect samples", "");
				sample_index = 0;
			}
			/* Copy sample to the U-Curve data */
			focus_pos[sample_index] = DEVICE_PRIVATE_DATA->focuser_position;
			for (int i = 0; i < star_count; i++) {
				hfds[i][sample_index] = AGENT_IMAGER_STATS_HFD_ITEM[i].number.value;
				INDIGO_DRIVER_DEBUG(DRIVER_NAME, "UC: pos[%d][%d] = (%g, %f)", i, sample_index, focus_pos[sample_index], hfds[i][sample_index]);
			}

			/* Find best sample index and value */
			int best_indices[INDIGO_MAX_MULTISTAR_COUNT] = { 0 };
			/* Find the best index for each star */
			int used_stars = 0;
			for (int star = 0; star < star_count; star++) {
				best_value = 0xFFFF;
				best_index = -1;
				for (int i = 0; i <= sample_index; i++) {
					if (hfds[star][i] < best_value && hfds[star][i] > 0) {
						best_value = hfds[star][i];
						best_index = i;
					}
				}
				if (best_index >= 0) {
					best_indices[used_stars++] = best_index;
				}
			}

			if (used_stars == 0) {  /* This should not happen, but just in case */
				indigo_send_message(device, "Error: No usable stars, maybe focus is too far");
				INDIGO_DRIVER_ERROR(DRIVER_NAME, "UC: No usable stars, maybe focus is too far (frame_count = %d)", frame_count);
				focus_failed = true;
				goto ucurve_finish;
			}

			/* Calculate the mode of the best indices */
			best_index = calculate_mode(best_indices, used_stars);
			INDIGO_DRIVER_DEBUG(DRIVER_NAME, "UC: The best sample focus is at position %d (mode), %d for star #0", best_index, best_indices[0]);

			if (sample_index >= ucurve_samples - 1 && best_index > mid_index) {
				/* The best focus is above the middle of the U-Curve.
				   We move all samples to the left and continue collecting.
				 */
				for (int i = 0; i < sample_index; i++) {
					focus_pos[i] = focus_pos[i+1];
					for(int j = 0; j < star_count; j++) {
						hfds[j][i] = hfds[j][i+1];
					}
				}
				sample_index --;
				INDIGO_DRIVER_DEBUG(DRIVER_NAME, "UC: Crossed the mid-point and did not reach best focus - shifting samples to the left", "");
			}
			if (sample_index == ucurve_samples - 1) {
				INDIGO_DRIVER_DEBUG(DRIVER_NAME, "UC: Collected all %d samples", ucurve_samples);
				break;
			}
			if (!move_focuser_with_overshoot_if_needed(device, moving_out, steps, DEVICE_PRIVATE_DATA->saved_backlash, false)) break;
			if (moving_out) {
				current_offset += (int)steps;
			} else {
				current_offset -= (int)steps;
			}
		}
		sample++;
		sample_index++;
		AGENT_IMAGER_STATS_FOCUS_OFFSET_ITEM->number.value = current_offset;
		indigo_update_property(device, AGENT_IMAGER_STATS_PROPERTY, NULL);
		memcpy(prev_quality, quality, sizeof(double) * star_count);
		if (abs(current_offset) >= limit) {
			indigo_send_message(device, "No focus reached within maximum travel limit of %g staps per AF run", limit);
			focus_failed = true;
			goto ucurve_finish;
		}
	}
	if (AGENT_ABORT_PROCESS_PROPERTY->state == INDIGO_BUSY_STATE) {
		set_backlash_if_overshoot(device, DEVICE_PRIVATE_DATA->saved_backlash);
		return false;
	}
	double polynomial[UCURVE_ORDER + 1];
	double best_focuses[INDIGO_MAX_MULTISTAR_COUNT] = {0};
	int stars_used = 0;
	for (int n = 0; n < star_count; n++) {
		if (indigo_get_log_level() >= INDIGO_LOG_DEBUG) {
			INDIGO_DRIVER_DEBUG(DRIVER_NAME, "UC: U-Curve collected %d samples for star #%d (final set):", DEVICE_PRIVATE_DATA->ucurve_samples_number, n);
			for (int i = 0; i < DEVICE_PRIVATE_DATA->ucurve_samples_number; i++) {
				INDIGO_DRIVER_DEBUG(DRIVER_NAME, "UC: point[%d][%d] = (%g, %f)",n , i, focus_pos[i], hfds[n][i]);
			}
		}
		if (quality_has_zero(hfds[n], DEVICE_PRIVATE_DATA->ucurve_samples_number)) {
			INDIGO_DRIVER_DEBUG(DRIVER_NAME, "UC: U-Curve data for star #%d has bad quality data points - skipping", n);
			continue;
		}
		int res = indigo_polynomial_fit(DEVICE_PRIVATE_DATA->ucurve_samples_number, focus_pos, hfds[n], UCURVE_ORDER + 1, polynomial);
		if (res < 0) {
			indigo_send_message(device, "U-Curve failed to fit data points with polynomial");
			INDIGO_DRIVER_ERROR(DRIVER_NAME, "UC: Failed to fit polynomial", "");
			set_backlash_if_overshoot(device, DEVICE_PRIVATE_DATA->saved_backlash);
			focus_failed = true;
			goto ucurve_finish;
		}
		if (indigo_get_log_level() >= INDIGO_LOG_DEBUG) {
			char polynomial_str[1204];
			indigo_polynomial_string(UCURVE_ORDER + 1, polynomial, polynomial_str);
			INDIGO_DRIVER_DEBUG(DRIVER_NAME, "UC: Polynomial fit for star #d: %s", polynomial_str, n);
		}
		if (focus_pos[0] < focus_pos[DEVICE_PRIVATE_DATA->ucurve_samples_number - 1]) {
			best_focuses[stars_used] = indigo_polynomial_min_x(UCURVE_ORDER + 1, polynomial, focus_pos[0], focus_pos[DEVICE_PRIVATE_DATA->ucurve_samples_number - 1], 0.00001);
			if (best_focuses[stars_used] < focus_pos[1] || best_focuses[stars_used] > focus_pos[DEVICE_PRIVATE_DATA->ucurve_samples_number - 2]) {
				focus_failed = true;
			}
		} else {
			best_focuses[stars_used] = indigo_polynomial_min_x(UCURVE_ORDER + 1, polynomial, focus_pos[DEVICE_PRIVATE_DATA->ucurve_samples_number - 1], focus_pos[0], 0.00001);
			if (best_focuses[stars_used] < focus_pos[DEVICE_PRIVATE_DATA->ucurve_samples_number - 2] || best_focuses[stars_used] > focus_pos[1]) {
				focus_failed = true;
			}
		}
		stars_used++;
		if (focus_failed) {
			indigo_send_message(device, "U-Curve failed to find best focus position in the acceptable range");
			INDIGO_DRIVER_ERROR(DRIVER_NAME, "UC: Failed to find best focus position in the acceptable range", "");
			set_backlash_if_overshoot(device, DEVICE_PRIVATE_DATA->saved_backlash);
			goto ucurve_finish;
		}
	}
	if (stars_used == 0) {
		indigo_send_message(device, "U-Curve failed to find the best focus position for any of the selected stars");
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "UC: Failed to find the best focus position for any of the selected stars", "");
		set_backlash_if_overshoot(device, DEVICE_PRIVATE_DATA->saved_backlash);
		focus_failed = true;
		goto ucurve_finish;
	}
	/* Reduce the best focus positions */
	INDIGO_DRIVER_DEBUG(DRIVER_NAME, "UC: Reducing best focus positions for %d of %d stars", stars_used, star_count);
	double best_focus = reduce_ucurve_best_focus(best_focuses, stars_used);
	/* Calculate the steps to best focus */
	double steps_to_focus = fabs(DEVICE_PRIVATE_DATA->focuser_position - best_focus);
	indigo_send_message(device, "U-Curve found best focus at position %.3f", best_focus);
	INDIGO_DRIVER_DEBUG(DRIVER_NAME, "UC: U-Curve found best focus at position %.3f, steps_to_focus = %g", best_focus, steps_to_focus);
	if (!move_focuser_with_overshoot_if_needed(device, !moving_out, steps_to_focus, DEVICE_PRIVATE_DATA->saved_backlash, true)) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "Failed to move to best focus position", "");
		set_backlash_if_overshoot(device, DEVICE_PRIVATE_DATA->saved_backlash);
		return false;
	}

	if (!moving_out) {
		current_offset += (int)steps_to_focus;
	} else {
		current_offset -= (int)steps_to_focus;
	}

	if (!capture_and_process_frame(device, NULL)) {
		return false;
	}

	if (AGENT_ABORT_PROCESS_PROPERTY->state == INDIGO_BUSY_STATE) {
		set_backlash_if_overshoot(device, DEVICE_PRIVATE_DATA->saved_backlash);
		return false;
	}

	/* Calculate focus deviation from the optimal one */
	if (min_est > 0) {
		AGENT_IMAGER_STATS_FOCUS_DEVIATION_ITEM->number.value = 100 * (AGENT_IMAGER_STATS_HFD_ITEM->number.value - min_est) / AGENT_IMAGER_STATS_HFD_ITEM->number.value;
	} else {
		AGENT_IMAGER_STATS_FOCUS_DEVIATION_ITEM->number.value = 100;
	}

	indigo_update_property(device, AGENT_IMAGER_STATS_PROPERTY, NULL);

	if (AGENT_IMAGER_STATS_HFD_ITEM->number.value > 1.2 * AGENT_IMAGER_SELECTION_RADIUS_ITEM->number.value && DEVICE_PRIVATE_DATA->use_hfd_estimator) {
		indigo_send_message(device, "Error: No focus reached, did not converge");
		focus_failed = true;
	} else if (AGENT_IMAGER_STATS_FOCUS_DEVIATION_ITEM->number.value > 20) { /* for HFD 20% deviation is ok - tested on realsky */
		indigo_send_message(device, "Error: Focus does not meet the quality criteria");
		focus_failed = true;
	}
	INDIGO_DRIVER_DEBUG(DRIVER_NAME, "UC: Focus deviation = %g %%", AGENT_IMAGER_STATS_FOCUS_DEVIATION_ITEM->number.value);

	ucurve_finish:
	if (focus_failed) {
		set_backlash_if_overshoot(device, DEVICE_PRIVATE_DATA->saved_backlash);
		if (DEVICE_PRIVATE_DATA->restore_initial_position) {
			indigo_send_message(device, "Focus failed, restoring initial position");
			INDIGO_DRIVER_ERROR(DRIVER_NAME, "Failed to reach focus, moving to initial position %d steps", (int)current_offset);
			if (current_offset > 0) {
				if (moving_out && !DEVICE_PRIVATE_DATA->focuser_has_backlash) {
					current_offset += (int)AGENT_IMAGER_FOCUS_BACKLASH_ITEM->number.value;
				}
				move_focuser(device, false, current_offset);
			} else if (current_offset < 0) {
				current_offset = -current_offset;
				if (!moving_out && !DEVICE_PRIVATE_DATA->focuser_has_backlash) {
					current_offset += (int)AGENT_IMAGER_FOCUS_BACKLASH_ITEM->number.value;
				}
				move_focuser(device, true, current_offset);
			}
			current_offset = 0;
		}
		return false;
	} else {
		set_backlash_if_overshoot(device, DEVICE_PRIVATE_DATA->saved_backlash);
		return true;
	}
}

static bool autofocus(indigo_device *device) {
	bool result = true;
	uint8_t *saturation_mask = NULL;
	clear_stats(device);
	if (DEVICE_PRIVATE_DATA->use_ucurve_focusing) {
		if (DEVICE_PRIVATE_DATA->use_hfd_estimator) {
			result = autofocus_ucurve(device);
		} else {
			indigo_send_message(device, "Error: Unsupported estimator and focusing algorithm combination");
		}
	} else if (DEVICE_PRIVATE_DATA->use_iterative_focusing) {
		if (DEVICE_PRIVATE_DATA->use_hfd_estimator || DEVICE_PRIVATE_DATA->use_rms_estimator || DEVICE_PRIVATE_DATA->use_bahtinov_estimator) {
			result = autofocus_iterative(device, &saturation_mask);
		} else {
			indigo_send_message(device, "Error: Unsupported estimator and focusing algorithm combination");
		}
	}
	indigo_safe_free(saturation_mask);
	return result;
}

static bool autofocus_repeat(indigo_device *device) {
	int focuser_mode = indigo_save_switch_state(device, FOCUSER_MODE_PROPERTY_NAME, FOCUSER_MODE_MANUAL_ITEM_NAME);
	int upload_mode = indigo_save_switch_state(device, CCD_UPLOAD_MODE_PROPERTY_NAME, CCD_UPLOAD_MODE_CLIENT_ITEM_NAME);
	int image_format = indigo_save_switch_state(device, CCD_IMAGE_FORMAT_PROPERTY_NAME, CCD_IMAGE_FORMAT_RAW_ITEM_NAME);
	DEVICE_PRIVATE_DATA->restore_initial_position = AGENT_IMAGER_FOCUS_ESTIMATOR_RMS_CONTRAST_ITEM->sw.value ? false : AGENT_IMAGER_FOCUS_FAILURE_RESTORE_ITEM->sw.value;
	bool result = true;
	int repeat_delay = (int)AGENT_IMAGER_FOCUS_DELAY_ITEM->number.value;
	for (int repeat_count = (int)AGENT_IMAGER_FOCUS_REPEAT_ITEM->number.value; result && repeat_count >= 0; repeat_count--) {
		if (DEVICE_PRIVATE_DATA->use_hfd_estimator || DEVICE_PRIVATE_DATA->use_ucurve_focusing) {
			result = check_selection(device);
			if (result && AGENT_ABORT_PROCESS_PROPERTY->state != INDIGO_BUSY_STATE && (int)AGENT_IMAGER_SELECTION_STAR_COUNT_ITEM->number.value == 1) {
				select_subframe(device);
			}
		}
		result = autofocus(device);
		if (result) {
			break;
		} else if (AGENT_ABORT_PROCESS_PROPERTY->state == INDIGO_BUSY_STATE) {
			result = false;
			break;
		} else if (repeat_count > 0) {
			indigo_send_message(device, "Repeating in %d seconds, %d attempts left", repeat_delay, repeat_count);
			for (int i = repeat_delay * 5; i >= 0; i--) {
				if (AGENT_ABORT_PROCESS_PROPERTY->state == INDIGO_BUSY_STATE) {
					result = false;
					break;
				}
				indigo_usleep(200000);
			}
			repeat_delay *= 2;
			result = true;
		}
	}
	restore_subframe(device);
	indigo_restore_switch_state(device, FOCUSER_MODE_PROPERTY_NAME, focuser_mode);
	indigo_restore_switch_state(device, CCD_UPLOAD_MODE_PROPERTY_NAME, upload_mode);
	indigo_restore_switch_state(device, CCD_IMAGE_FORMAT_PROPERTY_NAME, image_format);
	AGENT_IMAGER_STATS_PROPERTY->state = INDIGO_OK_STATE;
	indigo_update_property(device, AGENT_IMAGER_STATS_PROPERTY, NULL);
	return result;
}

static void autofocus_process(indigo_device *device) {
	FILTER_DEVICE_CONTEXT->running_process = true;
	allow_abort_by_mount_agent(device, true);
	disable_solver(device);
	indigo_send_message(device, "Focusing started");
	if (autofocus_repeat(device)) {
		indigo_send_message(device, "Focusing finished");
		AGENT_START_PROCESS_PROPERTY->state = INDIGO_OK_STATE;
	} else {
		if (AGENT_ABORT_PROCESS_PROPERTY->state == INDIGO_BUSY_STATE) {
			AGENT_ABORT_PROCESS_PROPERTY->state = INDIGO_OK_STATE;
			indigo_update_property(device, AGENT_ABORT_PROCESS_PROPERTY, NULL);
			indigo_send_message(device, "Focusing aborted");
		} else {
			indigo_send_message(device, "Focusing failed");
		}
		AGENT_START_PROCESS_PROPERTY->state = INDIGO_ALERT_STATE;
	}
	allow_abort_by_mount_agent(device, false);
	AGENT_IMAGER_START_FOCUSING_ITEM->sw.value = false;
	indigo_update_property(device, AGENT_START_PROCESS_PROPERTY, NULL);
	FILTER_DEVICE_CONTEXT->running_process = false;
}

static bool validate_include_region(indigo_device *device, bool force) {
	if (!DEVICE_PRIVATE_DATA->autosubframing && DEVICE_PRIVATE_DATA->last_width > 0 && DEVICE_PRIVATE_DATA->last_height > 0) {
		int safety_margin = (int)(DEVICE_PRIVATE_DATA->last_width < DEVICE_PRIVATE_DATA->last_height ? DEVICE_PRIVATE_DATA->last_width * 0.05 : DEVICE_PRIVATE_DATA->last_height * 0.05);
		int safety_limit_left = safety_margin;
		int safety_limit_top = safety_margin;
		int safety_limit_right = DEVICE_PRIVATE_DATA->last_width - safety_margin;
		int safety_limit_bottom = DEVICE_PRIVATE_DATA->last_height - safety_margin;
		int include_left = (int)AGENT_IMAGER_SELECTION_INCLUDE_LEFT_ITEM->number.value;
		int include_top = (int)AGENT_IMAGER_SELECTION_INCLUDE_TOP_ITEM->number.value;
		int include_width = (int)AGENT_IMAGER_SELECTION_INCLUDE_WIDTH_ITEM->number.value;
		int include_height = (int)AGENT_IMAGER_SELECTION_INCLUDE_HEIGHT_ITEM->number.value;
		bool update = false;
		if (include_width > 0 && include_height > 0) {
			if (include_left < safety_limit_left) {
				AGENT_IMAGER_SELECTION_INCLUDE_LEFT_ITEM->number.value = include_left = safety_limit_left;
				update = true;
			}
			int include_right = include_left + include_width;
			if (include_right > safety_limit_right) {
				AGENT_IMAGER_SELECTION_INCLUDE_WIDTH_ITEM->number.value = include_width = safety_limit_right - include_left;
				update = true;
			}
			if (include_top < safety_limit_top) {
				AGENT_IMAGER_SELECTION_INCLUDE_TOP_ITEM->number.value = include_top = safety_limit_top;
				update = true;
			}
			int include_bottom = include_top + include_height;
			if (include_bottom > safety_limit_bottom) {
				AGENT_IMAGER_SELECTION_INCLUDE_HEIGHT_ITEM->number.value = include_height = safety_limit_bottom - include_top;
				update = true;
			}
		} else {
			AGENT_IMAGER_SELECTION_INCLUDE_LEFT_ITEM->number.value = safety_limit_left;
			AGENT_IMAGER_SELECTION_INCLUDE_TOP_ITEM->number.value = safety_limit_top;
			AGENT_IMAGER_SELECTION_INCLUDE_WIDTH_ITEM->number.value = safety_limit_right - safety_limit_left;
			AGENT_IMAGER_SELECTION_INCLUDE_HEIGHT_ITEM->number.value = safety_limit_bottom - safety_limit_top;
			update = true;
		}
		if (update || force) {
			if (AGENT_IMAGER_STARS_PROPERTY->count > 1) {
				indigo_delete_property(device, AGENT_IMAGER_STARS_PROPERTY, NULL);
				AGENT_IMAGER_STARS_PROPERTY->count = 1;
				indigo_define_property(device, AGENT_IMAGER_STARS_PROPERTY, NULL);
			}
			indigo_update_property(device, AGENT_IMAGER_STARS_PROPERTY, NULL);
			for (int i = (int)(AGENT_IMAGER_SELECTION_X_ITEM - AGENT_IMAGER_SELECTION_PROPERTY->items); i < AGENT_IMAGER_SELECTION_PROPERTY->count; i++) {
				indigo_item *item = AGENT_IMAGER_SELECTION_PROPERTY->items + i;
				item->number.value = item->number.target = 0;
			}
		}
		return update;
	}
	return false;
}

static void clear_selection_process(indigo_device *device) {
	FILTER_DEVICE_CONTEXT->running_process = true;
	clear_selection(device);
	AGENT_IMAGER_CLEAR_SELECTION_ITEM->sw.value = false;
	AGENT_START_PROCESS_PROPERTY->state = INDIGO_OK_STATE;
	indigo_update_property(device, AGENT_START_PROCESS_PROPERTY, NULL);
	FILTER_DEVICE_CONTEXT->running_process = false;
}

static void find_stars_process(indigo_device *device) {
	FILTER_DEVICE_CONTEXT->running_process = true;
	int upload_mode = indigo_save_switch_state(device, CCD_UPLOAD_MODE_PROPERTY_NAME, CCD_UPLOAD_MODE_CLIENT_ITEM_NAME);
	int image_format = indigo_save_switch_state(device, CCD_IMAGE_FORMAT_PROPERTY_NAME, CCD_IMAGE_FORMAT_RAW_ITEM_NAME);
	disable_solver(device);
	if (capture_frame(device) && find_stars(device)) {
		AGENT_IMAGER_STARS_PROPERTY->state = INDIGO_OK_STATE;
	} else {
		AGENT_IMAGER_STARS_PROPERTY->state = INDIGO_ALERT_STATE;
	}
	indigo_restore_switch_state(device, CCD_UPLOAD_MODE_PROPERTY_NAME, upload_mode);
	indigo_restore_switch_state(device, CCD_IMAGE_FORMAT_PROPERTY_NAME, image_format);
	if (AGENT_ABORT_PROCESS_PROPERTY->state == INDIGO_BUSY_STATE) {
		AGENT_ABORT_PROCESS_PROPERTY->state = INDIGO_OK_STATE;
		indigo_update_property(device, AGENT_ABORT_PROCESS_PROPERTY, NULL);
	}
	indigo_update_property(device, AGENT_IMAGER_STARS_PROPERTY, NULL);
	FILTER_DEVICE_CONTEXT->running_process = false;
}

static void abort_process(indigo_device *device) {
	if (AGENT_IMAGER_RESUME_CONDITION_BARRIER_ITEM->sw.value) {
		// Stop process on related imager agents
		// TBD: This is still race condition!
		indigo_property *related_agents_property = FILTER_DEVICE_CONTEXT->filter_related_agent_list_property;
		for (int i = 0; i < related_agents_property->count; i++) {
			indigo_item *item = related_agents_property->items + i;
			if (item->sw.value && !strncmp(item->name, "Imager Agent", 12))
				indigo_change_switch_property_1(FILTER_DEVICE_CONTEXT->client, item->name, AGENT_ABORT_PROCESS_PROPERTY_NAME, AGENT_ABORT_PROCESS_ITEM_NAME, true);
		}
	}
	if (DEVICE_PRIVATE_DATA->use_aux_1) {
		indigo_change_switch_property_1(FILTER_DEVICE_CONTEXT->client, device->name, CCD_ABORT_EXPOSURE_PROPERTY_NAME, CCD_ABORT_EXPOSURE_ITEM_NAME, true);
	}
	indigo_change_switch_property_1(FILTER_DEVICE_CONTEXT->client, device->name, CCD_ABORT_EXPOSURE_PROPERTY_NAME, CCD_ABORT_EXPOSURE_ITEM_NAME, true);
	indigo_change_switch_property_1(FILTER_DEVICE_CONTEXT->client, device->name, FOCUSER_ABORT_MOTION_PROPERTY_NAME, FOCUSER_ABORT_MOTION_ITEM_NAME, true);
}

static bool image_filter(const char *name) {
	return strstr(name, ".fits") || strstr(name, ".xisf") || strstr(name, ".raw") || strstr(name, ".jpeg") || strstr(name, ".tiff") || strstr(name, ".avi") || strstr(name, ".ser") || strstr(name, ".nef") || strstr(name, ".cr") || strstr(name, ".sr") || strstr(name, ".arw") || strstr(name, ".raf");
}

static void setup_download(indigo_device *device) {
	if (*DEVICE_PRIVATE_DATA->current_folder) {
		indigo_delete_property(device, AGENT_IMAGER_DOWNLOAD_FILES_PROPERTY, NULL);
		char **list;
		int count = indigo_uni_scandir(DEVICE_PRIVATE_DATA->current_folder, &list, image_filter);
		if (count >= 0) {
			AGENT_IMAGER_DOWNLOAD_FILES_PROPERTY = indigo_resize_property(AGENT_IMAGER_DOWNLOAD_FILES_PROPERTY, count + 1);
			char file_name[PATH_MAX], label[INDIGO_VALUE_SIZE];
			struct stat file_stat;
			int valid_count = 1; /* Refresh item is 0 */
			for (int i = 0; i < count; i++) {
				snprintf(file_name, sizeof(file_name), "%s%s", DEVICE_PRIVATE_DATA->current_folder, list[i]);
				if (stat(file_name, &file_stat) >= 0 && file_stat.st_size > 0) {
					if (file_stat.st_size < 1024) {
						snprintf(label, sizeof(label), "%s (%dB)", list[i], (int)file_stat.st_size);
					} else if (file_stat.st_size < 1048576) {
						snprintf(label, sizeof(label), "%s (%.1fKB)", list[i], file_stat.st_size / 1024.0);
					} else {
						snprintf(label, sizeof(label), "%s (%.1fMB)", list[i], file_stat.st_size / 1048576.0);
					}
					indigo_init_switch_item(AGENT_IMAGER_DOWNLOAD_FILES_PROPERTY->items + valid_count++, list[i], label, false);
				}
				indigo_safe_free(list[i]);
			}
			AGENT_IMAGER_DOWNLOAD_FILES_PROPERTY->count = valid_count;
			indigo_safe_free(list);
		}
		AGENT_IMAGER_DOWNLOAD_FILES_PROPERTY->state = INDIGO_OK_STATE;
		indigo_define_property(device, AGENT_IMAGER_DOWNLOAD_FILES_PROPERTY, NULL);
	}
}

static bool validate_device(indigo_device *device, int index, indigo_property *info_property, int mask) {
	if (index == INDIGO_FILTER_AUX_1_INDEX && mask != INDIGO_INTERFACE_AUX_SHUTTER)
		return false;
	return true;
}

static void adjust_stats_max_stars_to_use(indigo_device *device) {
	if (AGENT_IMAGER_FOCUS_ESTIMATOR_UCURVE_ITEM->sw.value) {
		AGENT_IMAGER_STATS_MAX_STARS_TO_USE_ITEM->number.target = AGENT_IMAGER_STATS_MAX_STARS_TO_USE_ITEM->number.value = AGENT_IMAGER_SELECTION_STAR_COUNT_ITEM->number.value;
	} else if (AGENT_IMAGER_FOCUS_ESTIMATOR_HFD_PEAK_ITEM->sw.value) {
		AGENT_IMAGER_STATS_MAX_STARS_TO_USE_ITEM->number.target = AGENT_IMAGER_STATS_MAX_STARS_TO_USE_ITEM->number.value = 1;
	} else {
		AGENT_IMAGER_STATS_MAX_STARS_TO_USE_ITEM->number.target = AGENT_IMAGER_STATS_MAX_STARS_TO_USE_ITEM->number.value = 0;
	}
}

// ------- Sequencer is deprecated ------------------------------------------------ begin

#ifdef __GNUC__
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
#endif

static void park_mount(indigo_device *device) {
	char *related_agent_name = indigo_filter_first_related_agent(device, "Mount Agent");
	if (related_agent_name) {
		indigo_change_switch_property_1(FILTER_DEVICE_CONTEXT->client, related_agent_name, MOUNT_PARK_PROPERTY_NAME, MOUNT_PARK_PARKED_ITEM_NAME, true);
	}
}

static void unpark_mount(indigo_device *device) {
	char *related_agent_name = indigo_filter_first_related_agent(device, "Mount Agent");
	if (related_agent_name) {
		indigo_change_switch_property_1(FILTER_DEVICE_CONTEXT->client, related_agent_name, MOUNT_PARK_PROPERTY_NAME, MOUNT_PARK_UNPARKED_ITEM_NAME, true);
	}
}

static void solver_precise_goto(indigo_device *device) {
	char *related_agent_name = indigo_filter_first_related_agent_2(device, "Astrometry Agent", "ASTAP Agent");
	if (related_agent_name) {
		static char *names[] = { AGENT_PLATESOLVER_GOTO_SETTINGS_RA_ITEM_NAME, AGENT_PLATESOLVER_GOTO_SETTINGS_DEC_ITEM_NAME };
		double values[] = { DEVICE_PRIVATE_DATA->solver_goto_ra, DEVICE_PRIVATE_DATA->solver_goto_dec };
		indigo_change_number_property(FILTER_DEVICE_CONTEXT->client, related_agent_name, AGENT_PLATESOLVER_GOTO_SETTINGS_PROPERTY_NAME, 2, (const char **)names, values);
		indigo_change_switch_property_1(FILTER_DEVICE_CONTEXT->client, related_agent_name, AGENT_PLATESOLVER_SOLVE_IMAGES_PROPERTY_NAME, AGENT_PLATESOLVER_SOLVE_IMAGES_ENABLED_ITEM_NAME, true);
		indigo_change_switch_property_1(FILTER_DEVICE_CONTEXT->client, related_agent_name, AGENT_START_PROCESS_PROPERTY_NAME, AGENT_PLATESOLVER_START_PRECISE_GOTO_ITEM_NAME, true);
	}
}

static void abort_solver(indigo_device *device) {
	char *related_agent_name = indigo_filter_first_related_agent_2(device, "Astrometry Agent", "ASTAP Agent");
	if (related_agent_name) {
		indigo_change_switch_property_1(FILTER_DEVICE_CONTEXT->client, related_agent_name, AGENT_ABORT_PROCESS_PROPERTY_NAME, AGENT_ABORT_PROCESS_ITEM_NAME, true);
	}
}

static void stop_guider(indigo_device *device) {
	char *related_agent_name = indigo_filter_first_related_agent(device, "Guider Agent");
	if (related_agent_name) {
		indigo_change_switch_property_1(FILTER_DEVICE_CONTEXT->client, related_agent_name, AGENT_ABORT_PROCESS_PROPERTY_NAME, AGENT_ABORT_PROCESS_ITEM_NAME, true);
	}
}

static void calibrate_guider(indigo_device *device, double exposure_time) {
	char *related_agent_name = indigo_filter_first_related_agent(device, "Guider Agent");
	if (related_agent_name) {
		indigo_change_number_property_1(FILTER_DEVICE_CONTEXT->client, related_agent_name, AGENT_GUIDER_SETTINGS_PROPERTY_NAME, AGENT_GUIDER_SETTINGS_EXPOSURE_ITEM_NAME, exposure_time);
		indigo_change_switch_property_1(FILTER_DEVICE_CONTEXT->client, related_agent_name, AGENT_START_PROCESS_PROPERTY_NAME, AGENT_GUIDER_START_CALIBRATION_AND_GUIDING_ITEM_NAME, true);
	}
}

static void start_guider(indigo_device *device, double exposure_time) {
	char *related_agent_name = indigo_filter_first_related_agent(device, "Guider Agent");
	if (related_agent_name) {
		indigo_change_number_property_1(FILTER_DEVICE_CONTEXT->client, related_agent_name, AGENT_GUIDER_SETTINGS_PROPERTY_NAME, AGENT_GUIDER_SETTINGS_EXPOSURE_ITEM_NAME, exposure_time);
		indigo_change_switch_property_1(FILTER_DEVICE_CONTEXT->client, related_agent_name, AGENT_START_PROCESS_PROPERTY_NAME, AGENT_GUIDER_START_GUIDING_ITEM_NAME, true);
	}
}

static bool set_property(indigo_device *device, char *name, char *value) {
	indigo_property *device_property = NULL;
	bool wait_for_solver = false;
	bool wait_for_guider = false;
	int upload_mode = -1;
	int image_format = -1;
	if (!strcasecmp(name, "object")) {
		// NO-OP, for grouping only
	} else if (!strcasecmp(name, "sleep")) {
		// sleep with 0.01s resolution
		double delay = atof(value);
		while (delay > 0 && AGENT_ABORT_PROCESS_PROPERTY->state != INDIGO_BUSY_STATE) {
			indigo_usleep(10000);
			delay -= 0.01;
		}
	} else if (!strcasecmp(name, "focus")) {
		DEVICE_PRIVATE_DATA->focus_exposure = atof(value);
	} else if (!strcasecmp(name, "count")) {
		AGENT_IMAGER_BATCH_COUNT_ITEM->number.target = atoi(value);
		indigo_update_property(device, AGENT_IMAGER_BATCH_PROPERTY, NULL);
	} else if (!strcasecmp(name, "exposure")) {
		AGENT_IMAGER_BATCH_EXPOSURE_ITEM->number.target = AGENT_IMAGER_BATCH_EXPOSURE_ITEM->number.value = atof(value);
		indigo_update_property(device, AGENT_IMAGER_BATCH_PROPERTY, NULL);
	} else if (!strcasecmp(name, "delay")) {
		AGENT_IMAGER_BATCH_DELAY_ITEM->number.target = AGENT_IMAGER_BATCH_DELAY_ITEM->number.value = atof(value);
		indigo_update_property(device, AGENT_IMAGER_BATCH_PROPERTY, NULL);
	} else if (!strcasecmp(name, "filter")) {
		AGENT_IMAGER_STATS_PHASE_ITEM->number.value = INDIGO_IMAGER_PHASE_SETTING_FILTER;
		indigo_update_property(device, AGENT_IMAGER_STATS_PROPERTY, NULL);
		if (indigo_filter_cached_property(device, INDIGO_FILTER_WHEEL_INDEX, WHEEL_SLOT_PROPERTY_NAME, &device_property, NULL)) {
			for (int j = 0; j < AGENT_WHEEL_FILTER_PROPERTY->count; j++) {
				indigo_item *item = AGENT_WHEEL_FILTER_PROPERTY->items + j;
				if (!strcasecmp(value, item->label) || !strcasecmp(value, item->name)) {
					indigo_change_number_property_1(FILTER_DEVICE_CONTEXT->client, device_property->device, device_property->name, WHEEL_SLOT_ITEM_NAME, j + 1);
					break;
				}
			}
		}
	} else if (!strcasecmp(name, "mode")) {
		if (indigo_filter_cached_property(device, INDIGO_FILTER_CCD_INDEX, CCD_MODE_PROPERTY_NAME, &device_property, NULL)) {
			for (int j = 0; j < device_property->count; j++) {
				indigo_item *item = device_property->items + j;
				if (!strcasecmp(item->label, value) || !strcasecmp(item->name, value)) {
					indigo_change_switch_property_1(FILTER_DEVICE_CONTEXT->client, device_property->device, device_property->name, item->name, true);
					break;
				}
			}
		}
	} else if (!strcasecmp(name, "name")) {
		if (indigo_filter_cached_property(device, INDIGO_FILTER_CCD_INDEX, CCD_LOCAL_MODE_PROPERTY_NAME, &device_property, NULL))
			indigo_change_text_property_1(FILTER_DEVICE_CONTEXT->client, device_property->device, device_property->name, CCD_LOCAL_MODE_PREFIX_ITEM_NAME, value);
	} else if (!strcasecmp(name, "gain")) {
		if (indigo_filter_cached_property(device, INDIGO_FILTER_CCD_INDEX, CCD_GAIN_PROPERTY_NAME, &device_property, NULL))
			indigo_change_number_property_1(FILTER_DEVICE_CONTEXT->client, device_property->device, device_property->name, CCD_GAIN_ITEM_NAME, atof(value));
	} else if (!strcasecmp(name, "offset")) {
		if (indigo_filter_cached_property(device, INDIGO_FILTER_CCD_INDEX, CCD_OFFSET_PROPERTY_NAME, &device_property, NULL))
			indigo_change_number_property_1(FILTER_DEVICE_CONTEXT->client, device_property->device, device_property->name, CCD_OFFSET_ITEM_NAME, atof(value));
	} else if (!strcasecmp(name, "gamma")) {
		if (indigo_filter_cached_property(device, INDIGO_FILTER_CCD_INDEX, CCD_GAMMA_PROPERTY_NAME, &device_property, NULL))
			indigo_change_number_property_1(FILTER_DEVICE_CONTEXT->client, device_property->device, device_property->name, CCD_GAMMA_ITEM_NAME, atof(value));
	} else if (!strcasecmp(name, "temperature")) {
		if (indigo_filter_cached_property(device, INDIGO_FILTER_CCD_INDEX, CCD_TEMPERATURE_PROPERTY_NAME, &device_property, NULL))
			indigo_change_number_property_1(FILTER_DEVICE_CONTEXT->client, device_property->device, device_property->name, CCD_TEMPERATURE_ITEM_NAME, atof(value));
	} else if (!strcasecmp(name, "cooler")) {
		if (indigo_filter_cached_property(device, INDIGO_FILTER_CCD_INDEX, CCD_COOLER_PROPERTY_NAME, &device_property, NULL)) {
			for (int j = 0; j < device_property->count; j++) {
				indigo_item *item = device_property->items + j;
				if (!strcasecmp(item->label, value) || !strcasecmp(item->name, value)) {
					indigo_change_switch_property_1(FILTER_DEVICE_CONTEXT->client, device_property->device, device_property->name, item->name, true);
					break;
				}
			}
		}
	} else if (!strcasecmp(name, "frame")) {
		if (indigo_filter_cached_property(device, INDIGO_FILTER_CCD_INDEX, CCD_FRAME_TYPE_PROPERTY_NAME, &device_property, NULL)) {
			for (int j = 0; j < device_property->count; j++) {
				indigo_item *item = device_property->items + j;
				if (!strcasecmp(item->label, value) || !strcasecmp(item->name, value)) {
					indigo_change_switch_property_1(FILTER_DEVICE_CONTEXT->client, device_property->device, device_property->name, item->name, true);
					break;
				}
			}
		}
	} else if (!strcasecmp(name, "aperture")) {
		if (indigo_filter_cached_property(device, INDIGO_FILTER_CCD_INDEX, DSLR_APERTURE_PROPERTY_NAME, &device_property, NULL)) {
			for (int j = 0; j < device_property->count; j++) {
				indigo_item *item = device_property->items + j;
				if (!strcasecmp(item->label, value) || !strcasecmp(item->name, value)) {
					indigo_change_switch_property_1(FILTER_DEVICE_CONTEXT->client, device_property->device, device_property->name, item->name, true);
					break;
				}
			}
		}
	} else if (!strcasecmp(name, "shutter")) {
		if (indigo_filter_cached_property(device, INDIGO_FILTER_CCD_INDEX, DSLR_SHUTTER_PROPERTY_NAME, &device_property, NULL)) {
			for (int j = 0; j < device_property->count; j++) {
				indigo_item *item = device_property->items + j;
				if (!strcasecmp(item->label, value) || !strcasecmp(item->name, value)) {
					indigo_change_switch_property_1(FILTER_DEVICE_CONTEXT->client, device_property->device, device_property->name, item->name, true);
					break;
				}
			}
		}
	} else if (!strcasecmp(name, "iso")) {
		if (indigo_filter_cached_property(device, INDIGO_FILTER_CCD_INDEX, DSLR_ISO_PROPERTY_NAME, &device_property, NULL)) {
			for (int j = 0; j < device_property->count; j++) {
				indigo_item *item = device_property->items + j;
				if (!strcasecmp(item->label, value) || !strcasecmp(item->name, value)) {
					indigo_change_switch_property_1(FILTER_DEVICE_CONTEXT->client, device_property->device, device_property->name, item->name, true);
					break;
				}
			}
		}
	} else if (!strcasecmp(name, "ra")) {
		DEVICE_PRIVATE_DATA->solver_goto_ra = indigo_atod(value);
	} else if (!strcasecmp(name, "dec")) {
		DEVICE_PRIVATE_DATA->solver_goto_dec = indigo_atod(value);
	} else if (!strcasecmp(name, "goto")) {
		upload_mode = indigo_save_switch_state(device, CCD_UPLOAD_MODE_PROPERTY_NAME, CCD_UPLOAD_MODE_CLIENT_ITEM_NAME);
		image_format = indigo_save_switch_state(device, CCD_IMAGE_FORMAT_PROPERTY_NAME, CCD_IMAGE_FORMAT_RAW_ITEM_NAME);
		AGENT_IMAGER_STATS_PHASE_ITEM->number.value = INDIGO_IMAGER_PHASE_SLEWING;
		indigo_update_property(device, AGENT_IMAGER_STATS_PROPERTY, NULL);
		if (!strcmp(value, "precise")) {
			DEVICE_PRIVATE_DATA->related_solver_process_state = INDIGO_IDLE_STATE;
			solver_precise_goto(device);
			wait_for_solver = true;
		} else if (!strcmp(value, "slew")) {
			// TODO: non-precise goto is not implemented in solver agent yet
			wait_for_solver = true;
		}
	} else if (!strcasecmp(name, "calibrate")) {
		AGENT_IMAGER_STATS_PHASE_ITEM->number.value = INDIGO_IMAGER_PHASE_CALIBRATING;
		indigo_update_property(device, AGENT_IMAGER_STATS_PROPERTY, NULL);
		DEVICE_PRIVATE_DATA->related_guider_process_state = INDIGO_IDLE_STATE;
		calibrate_guider(device, atof(value));
		wait_for_guider = true;
	} else if (!strcasecmp(name, "guide")) {
		if (!strcmp(value, "off")) {
			stop_guider(device);
		} else {
			start_guider(device, atof(value));
			wait_for_guider = true;
		}
	} else if (!strcasecmp(name, "start")) {
	} else {
		indigo_send_message(device, "Unknown sequencer command '%s'", name);
		return false;
	}
	if (device_property) {
		indigo_usleep(200000);
		while (device_property->state == INDIGO_BUSY_STATE) {
			indigo_usleep(200000);
		}
		if (device_property->state != INDIGO_OK_STATE) {
			indigo_send_message(device, "Failed to set '%s'", device_property->name);
			return false;
		}
		return true;
	} else if (wait_for_solver) {
		while (DEVICE_PRIVATE_DATA->related_solver_process_state != INDIGO_BUSY_STATE && AGENT_ABORT_PROCESS_PROPERTY->state != INDIGO_BUSY_STATE) {
			indigo_usleep(200000);
		}
		while (DEVICE_PRIVATE_DATA->related_solver_process_state == INDIGO_BUSY_STATE && AGENT_ABORT_PROCESS_PROPERTY->state != INDIGO_BUSY_STATE) {
			indigo_usleep(200000);
		}
		if (DEVICE_PRIVATE_DATA->related_solver_process_state == INDIGO_BUSY_STATE) {
			abort_solver(device);
		}
		disable_solver(device);
		indigo_restore_switch_state(device, CCD_UPLOAD_MODE_PROPERTY_NAME, upload_mode);
		indigo_restore_switch_state(device, CCD_IMAGE_FORMAT_PROPERTY_NAME, image_format);
		return DEVICE_PRIVATE_DATA->related_solver_process_state == INDIGO_OK_STATE;
	} else if (wait_for_guider) { // wait for guider
		DEVICE_PRIVATE_DATA->guiding = false;
		while (!DEVICE_PRIVATE_DATA->guiding && DEVICE_PRIVATE_DATA->related_guider_process_state != INDIGO_ALERT_STATE && AGENT_ABORT_PROCESS_PROPERTY->state != INDIGO_BUSY_STATE) {
			indigo_usleep(200000);
		}
		if (AGENT_ABORT_PROCESS_PROPERTY->state == INDIGO_BUSY_STATE) {
			stop_guider(device);
		}
		return DEVICE_PRIVATE_DATA->guiding;
	}
	return true;
}

static void sequence_process(indigo_device *device) {
	FILTER_DEVICE_CONTEXT->running_process = true;
	char *sequence_text, *sequence_text_pnt, *value;
	AGENT_IMAGER_STATS_BATCH_INDEX_ITEM->number.value = AGENT_IMAGER_STATS_BATCH_ITEM->number.value = AGENT_IMAGER_STATS_PHASE_ITEM->number.value = AGENT_IMAGER_STATS_BATCHES_ITEM->number.value = 0;
	indigo_update_property(device, AGENT_IMAGER_STATS_PROPERTY, NULL);
	DEVICE_PRIVATE_DATA->focus_exposure = 0;
	int sequence_size = AGENT_IMAGER_SEQUENCE_PROPERTY->count - 1;
	sequence_text = indigo_safe_malloc_copy(strlen(indigo_get_text_item_value(AGENT_IMAGER_SEQUENCE_ITEM)) + 1, indigo_get_text_item_value(AGENT_IMAGER_SEQUENCE_ITEM));
	bool focuser_needed = strstr(sequence_text, "focus") != NULL;
	bool wheel_needed = strstr(sequence_text, "filter") != NULL;
	bool rotator_needed = strstr(sequence_text, "angle") != NULL;
	bool mount_needed = strstr(sequence_text, "park") != NULL;
	bool guider_needed = strstr(sequence_text, "guide") != NULL || strstr(sequence_text, "calibrate") != NULL;
	bool solver_needed = strstr(sequence_text, "precise") != NULL;
	for (char *token = strtok_r(sequence_text, ";", &sequence_text_pnt); token; token = strtok_r(NULL, ";", &sequence_text_pnt)) {
		if (strchr(token, '='))
			continue;
		if (!strcmp(token, "park"))
			continue;
		if (!strcmp(token, "unpark"))
			continue;
		int batch_index = atoi(token);
		if (batch_index < 1 || batch_index > sequence_size) {
			continue;
		}
		AGENT_IMAGER_STATS_BATCHES_ITEM->number.value++;
		if (strstr(AGENT_IMAGER_SEQUENCE_PROPERTY->items[batch_index].text.value, "focus") != NULL) {
			focuser_needed = true;
		}
		if (strstr(AGENT_IMAGER_SEQUENCE_PROPERTY->items[batch_index].text.value, "filter") != NULL) {
			wheel_needed = true;
		}
		if (strstr(AGENT_IMAGER_SEQUENCE_PROPERTY->items[batch_index].text.value, "angle") != NULL) {
			rotator_needed = true;
		}
		if (strstr(AGENT_IMAGER_SEQUENCE_PROPERTY->items[batch_index].text.value, "guide") != NULL || strstr(AGENT_IMAGER_SEQUENCE_PROPERTY->items[batch_index].text.value, "calibrate") != NULL) {
			guider_needed = true;
		}
		if (strstr(AGENT_IMAGER_SEQUENCE_PROPERTY->items[batch_index].text.value, "precise") != NULL) {
			solver_needed = true;
		}
	}
	if (focuser_needed && FILTER_FOCUSER_LIST_PROPERTY->items->sw.value) {
		AGENT_IMAGER_START_SEQUENCE_ITEM->sw.value = false;
		indigo_update_property(device, AGENT_IMAGER_STATS_PROPERTY, NULL);
		AGENT_START_PROCESS_PROPERTY->state = INDIGO_ALERT_STATE;
		FILTER_DEVICE_CONTEXT->running_process = false;
		indigo_update_property(device, AGENT_START_PROCESS_PROPERTY, "No focuser is selected");
		indigo_safe_free(sequence_text);
		return;
	}
	if (wheel_needed && FILTER_WHEEL_LIST_PROPERTY->items->sw.value) {
		AGENT_IMAGER_START_SEQUENCE_ITEM->sw.value = false;
		indigo_update_property(device, AGENT_IMAGER_STATS_PROPERTY, NULL);
		AGENT_START_PROCESS_PROPERTY->state = INDIGO_ALERT_STATE;
		FILTER_DEVICE_CONTEXT->running_process = false;
		indigo_update_property(device, AGENT_START_PROCESS_PROPERTY, "No filter wheen is selected");
		indigo_safe_free(sequence_text);
		return;
	}
	if (mount_needed && indigo_filter_first_related_agent(device, "Mount Agent") == NULL) {
		AGENT_IMAGER_START_SEQUENCE_ITEM->sw.value = false;
		indigo_update_property(device, AGENT_IMAGER_STATS_PROPERTY, NULL);
		AGENT_START_PROCESS_PROPERTY->state = INDIGO_ALERT_STATE;
		FILTER_DEVICE_CONTEXT->running_process = false;
		indigo_update_property(device, AGENT_START_PROCESS_PROPERTY, "No mount agent is selected");
		indigo_safe_free(sequence_text);
		return;
	}
	if (guider_needed && indigo_filter_first_related_agent(device, "Guider Agent") == NULL) {
		AGENT_IMAGER_START_SEQUENCE_ITEM->sw.value = false;
		indigo_update_property(device, AGENT_IMAGER_STATS_PROPERTY, NULL);
		AGENT_START_PROCESS_PROPERTY->state = INDIGO_ALERT_STATE;
		FILTER_DEVICE_CONTEXT->running_process = false;
		indigo_update_property(device, AGENT_START_PROCESS_PROPERTY, "No guider agent is selected");
		indigo_safe_free(sequence_text);
		return;
	}
	if (solver_needed && indigo_filter_first_related_agent_2(device, "Astrometry Agent", "ASTAP Agent") == NULL) {
		AGENT_IMAGER_START_SEQUENCE_ITEM->sw.value = false;
		indigo_update_property(device, AGENT_IMAGER_STATS_PROPERTY, NULL);
		AGENT_START_PROCESS_PROPERTY->state = INDIGO_ALERT_STATE;
		FILTER_DEVICE_CONTEXT->running_process = false;
		indigo_update_property(device, AGENT_START_PROCESS_PROPERTY, "No solver agent is selected");
		indigo_safe_free(sequence_text);
		return;
	}
	indigo_send_message(device, "Sequence started");
	indigo_update_property(device, AGENT_IMAGER_STATS_PROPERTY, NULL);
	strcpy(sequence_text, indigo_get_text_item_value(AGENT_IMAGER_SEQUENCE_ITEM));
	for (char *token = strtok_r(sequence_text, ";", &sequence_text_pnt); AGENT_ABORT_PROCESS_PROPERTY->state != INDIGO_BUSY_STATE && token; token = strtok_r(NULL, ";", &sequence_text_pnt)) {
		allow_abort_by_mount_agent(device, false);
		disable_solver(device);
		value = strchr(token, '=');
		if (value) {
			*value++ = 0;
			set_property(device, token, value);
			continue;
		}
		if (!strcmp(token, "park")) {
			park_mount(device);
			continue;
		}
		if (!strcmp(token, "unpark")) {
			unpark_mount(device);
			continue;
		}
		int batch_index = atoi(token);
		if (batch_index < 1 || batch_index > sequence_size) {
			continue;
		}
		indigo_send_message(device, "Batch %d started", batch_index);
		AGENT_IMAGER_STATS_FRAME_ITEM->number.value = 0;
		AGENT_IMAGER_STATS_BATCH_ITEM->number.value++;
		AGENT_IMAGER_STATS_BATCH_INDEX_ITEM->number.value = batch_index;
		AGENT_IMAGER_STATS_FRAMES_ITEM->number.value = 0;
		indigo_update_property(device, AGENT_IMAGER_STATS_PROPERTY, NULL);
		char batch_text[INDIGO_VALUE_SIZE], *batch_text_pnt;
		indigo_copy_value(batch_text, AGENT_IMAGER_SEQUENCE_PROPERTY->items[batch_index].text.value);
		bool valid_batch = true;
		for (char *token = strtok_r(batch_text, ";", &batch_text_pnt); token; token = strtok_r(NULL, ";", &batch_text_pnt)) {
			value = strchr(token, '=');
			if (value == NULL) {
				continue;
			}
			*value++ = 0;
			if (!set_property(device, token, value)) {
				valid_batch = false;
			}
		}
		if (valid_batch) {
			allow_abort_by_mount_agent(device, true);
			if (DEVICE_PRIVATE_DATA->focus_exposure > 0) {
				AGENT_IMAGER_STATS_PHASE_ITEM->number.value = INDIGO_IMAGER_PHASE_FOCUSING;
				indigo_update_property(device, AGENT_IMAGER_STATS_PROPERTY, NULL);
				double exposure = AGENT_IMAGER_BATCH_EXPOSURE_ITEM->number.target;
				AGENT_IMAGER_BATCH_EXPOSURE_ITEM->number.target = AGENT_IMAGER_BATCH_EXPOSURE_ITEM->number.value = DEVICE_PRIVATE_DATA->focus_exposure;
				indigo_update_property(device, AGENT_IMAGER_BATCH_PROPERTY, NULL);
				indigo_send_message(device, "Autofocus started");
				bool success = autofocus_repeat(device);
				if (success) {
					indigo_send_message(device, "Autofocus finished");
				} else {
					if (AGENT_ABORT_PROCESS_PROPERTY->state == INDIGO_BUSY_STATE) {
						indigo_send_message(device, "Autofocus aborted");
					} else {
						indigo_send_message(device, "Autofocus failed");
					}
				}
				AGENT_IMAGER_BATCH_EXPOSURE_ITEM->number.target = AGENT_IMAGER_BATCH_EXPOSURE_ITEM->number.value = exposure;
				indigo_update_property(device, AGENT_IMAGER_BATCH_PROPERTY, NULL);
				DEVICE_PRIVATE_DATA->focus_exposure = 0;
				if (!success) {
					break;
				}
			}
			if (AGENT_ABORT_PROCESS_PROPERTY->state == INDIGO_BUSY_STATE) {
				break;
			}
			if (exposure_batch(device)) {
				indigo_send_message(device, "Batch %d finished", batch_index);
			} else {
				indigo_send_message(device, "Batch %d failed", batch_index);
				continue;
			}
		} else {
			indigo_send_message(device, "Batch %d failed", batch_index);
			continue;
		}
	}
	allow_abort_by_mount_agent(device, false);
	indigo_safe_free(sequence_text);
	if (AGENT_START_PROCESS_PROPERTY->state == INDIGO_BUSY_STATE) {
		AGENT_START_PROCESS_PROPERTY->state = AGENT_IMAGER_STATS_PROPERTY->state = INDIGO_OK_STATE;
		// Sometimes blob arrives after the end of the sequence - gives sime time to the blob update
		indigo_usleep((unsigned)(0.2 * ONE_SECOND_DELAY));
		indigo_send_message(device, "Sequence finished");
	} else {
		indigo_send_message(device, "Sequence failed");
	}
	AGENT_IMAGER_STATS_PHASE_ITEM->number.value = INDIGO_IMAGER_PHASE_IDLE;
	indigo_update_property(device, AGENT_IMAGER_STATS_PROPERTY, NULL);
	AGENT_IMAGER_START_SEQUENCE_ITEM->sw.value = false;
	indigo_update_property(device, AGENT_START_PROCESS_PROPERTY, NULL);
	if (AGENT_ABORT_PROCESS_PROPERTY->state == INDIGO_BUSY_STATE) {
		AGENT_ABORT_PROCESS_PROPERTY->state = INDIGO_OK_STATE;
		indigo_update_property(device, AGENT_ABORT_PROCESS_PROPERTY, NULL);
	}
	FILTER_DEVICE_CONTEXT->running_process = false;
}

#ifdef __GNUC__
#pragma GCC diagnostic pop
#endif

// ------- Sequencer is deprecated ------------------------------------------------ end

// -------------------------------------------------------------------------------- INDIGO agent device implementation

static indigo_result agent_enumerate_properties(indigo_device *device, indigo_client *client, indigo_property *property);

static indigo_result agent_device_attach(indigo_device *device) {
	assert(device != NULL);
	assert(DEVICE_PRIVATE_DATA != NULL);
	if (indigo_filter_device_attach(device, DRIVER_NAME, DRIVER_VERSION, INDIGO_INTERFACE_CCD | INDIGO_INTERFACE_WHEEL | INDIGO_INTERFACE_FOCUSER) == INDIGO_OK) {
		// -------------------------------------------------------------------------------- Device properties
		FILTER_CCD_LIST_PROPERTY->hidden = false;
		FILTER_WHEEL_LIST_PROPERTY->hidden = false;
		FILTER_FOCUSER_LIST_PROPERTY->hidden = false;
		FILTER_RELATED_AGENT_LIST_PROPERTY->hidden = false;
		FILTER_AUX_1_LIST_PROPERTY->hidden = false;
		strcpy(FILTER_AUX_1_LIST_PROPERTY->label, "External shutter list");
		strcpy(FILTER_AUX_1_LIST_PROPERTY->items->label, "No external shutter");
		FILTER_DEVICE_CONTEXT->validate_device = validate_device;
		// -------------------------------------------------------------------------------- Batch properties
		AGENT_IMAGER_BATCH_PROPERTY = indigo_init_number_property(NULL, device->name, AGENT_IMAGER_BATCH_PROPERTY_NAME, "Agent", "Batch settings", INDIGO_OK_STATE, INDIGO_RW_PERM, 5);
		if (AGENT_IMAGER_BATCH_PROPERTY == NULL)
			return INDIGO_FAILED;
		indigo_init_number_item(AGENT_IMAGER_BATCH_COUNT_ITEM, AGENT_IMAGER_BATCH_COUNT_ITEM_NAME, "Frame count", -1, 0xFFFF, 1, 1);
		indigo_init_number_item(AGENT_IMAGER_BATCH_EXPOSURE_ITEM, AGENT_IMAGER_BATCH_EXPOSURE_ITEM_NAME, "Exposure time (s)", 0, 0xFFFF, 1, 1);
		indigo_init_number_item(AGENT_IMAGER_BATCH_DELAY_ITEM, AGENT_IMAGER_BATCH_DELAY_ITEM_NAME, "Delay after each exposure (s)", 0, 0xFFFF, 1, 0);
		indigo_init_number_item(AGENT_IMAGER_BATCH_FRAMES_TO_SKIP_BEFORE_DITHER_ITEM, AGENT_IMAGER_BATCH_FRAMES_TO_SKIP_BEFORE_DITHER_ITEM_NAME, "Frames to skip before dither", -1, 1000, 1, 0);
		indigo_init_number_item(AGENT_IMAGER_BATCH_PAUSE_AFTER_TRANSIT_ITEM, AGENT_IMAGER_BATCH_PAUSE_AFTER_TRANSIT_ITEM_NAME, "Pause after transit (h)", -2, 2, 1, 0);
		strcpy(AGENT_IMAGER_BATCH_PAUSE_AFTER_TRANSIT_ITEM->number.format, "%12.3m");
		// -------------------------------------------------------------------------------- Focus properties
		AGENT_IMAGER_FOCUS_PROPERTY = indigo_init_number_property(NULL, device->name, AGENT_IMAGER_FOCUS_PROPERTY_NAME, "Agent", "Autofocus settings", INDIGO_OK_STATE, INDIGO_RW_PERM, 13);
		if (AGENT_IMAGER_FOCUS_PROPERTY == NULL)
			return INDIGO_FAILED;
		indigo_init_number_item(AGENT_IMAGER_FOCUS_INITIAL_ITEM, AGENT_IMAGER_FOCUS_INITIAL_ITEM_NAME, "Initial / U-Curve step (obsolete)", 1, 0xFFFF, 1, 20); // obsolete
		indigo_init_number_item(AGENT_IMAGER_FOCUS_FINAL_ITEM, AGENT_IMAGER_FOCUS_FINAL_ITEM_NAME, "Final step (obsolete)", 1, 0xFFFF, 1, 5);
		indigo_init_number_item(AGENT_IMAGER_FOCUS_ITERATIVE_INITIAL_ITEM, AGENT_IMAGER_FOCUS_ITERATIVE_INITIAL_ITEM_NAME, "Iterative initial step", 1, 0xFFFF, 1, 20);
		indigo_init_number_item(AGENT_IMAGER_FOCUS_ITERATIVE_FINAL_ITEM, AGENT_IMAGER_FOCUS_ITERATIVE_FINAL_ITEM_NAME, "Iterative final step", 1, 0xFFFF, 1, 5);
		indigo_init_number_item(AGENT_IMAGER_FOCUS_UCURVE_SAMPLES_ITEM, AGENT_IMAGER_FOCUS_UCURVE_SAMPLES_ITEM_NAME, "U-Curve fitting samples", 6, MAX_UCURVE_SAMPLES, 1, 10);
		indigo_init_number_item(AGENT_IMAGER_FOCUS_UCURVE_STEP_ITEM, AGENT_IMAGER_FOCUS_UCURVE_STEP_ITEM_NAME, "U-Curve sample step", 1, 0xFFFF, 1, 20);
		indigo_init_number_item(AGENT_IMAGER_FOCUS_BAHTINOV_SIGMA_ITEM, AGENT_IMAGER_FOCUS_BAHTINOV_SIGMA_ITEM_NAME, "Bahtinov sigma", 0, 3, 0, 0.15);
		indigo_init_number_item(AGENT_IMAGER_FOCUS_BRACKETING_STEP_ITEM, AGENT_IMAGER_FOCUS_BRACKETING_STEP_ITEM_NAME, "Bracketing step", -0xFFFF, 0xFFFF, 1, 1);
		indigo_init_number_item(AGENT_IMAGER_FOCUS_BACKLASH_ITEM, AGENT_IMAGER_FOCUS_BACKLASH_ITEM_NAME, "Backlash", 0, 0xFFFF, 1, 0);
		indigo_init_number_item(AGENT_IMAGER_FOCUS_BACKLASH_OVERSHOOT_ITEM, AGENT_IMAGER_FOCUS_BACKLASH_OVERSHOOT_ITEM_NAME, "Backlash overshoot factor (1 disabled)", 1, 3, 0.5, 1);
		indigo_init_number_item(AGENT_IMAGER_FOCUS_STACK_ITEM, AGENT_IMAGER_FOCUS_STACK_ITEM_NAME, "Stacking", 1, 5, 1, 3);
		indigo_init_number_item(AGENT_IMAGER_FOCUS_REPEAT_ITEM, AGENT_IMAGER_FOCUS_REPEAT_ITEM_NAME, "Repeat count", 0, 10, 1, 0);
		indigo_init_number_item(AGENT_IMAGER_FOCUS_DELAY_ITEM, AGENT_IMAGER_FOCUS_DELAY_ITEM_NAME, "Initial repeat delay (s)", 0, 3600, 1, 0);
		// -------------------------------------------------------------------------------- Focus failure handling
		AGENT_IMAGER_FOCUS_FAILURE_PROPERTY = indigo_init_switch_property(NULL, device->name, AGENT_IMAGER_FOCUS_FAILURE_PROPERTY_NAME, "Agent", "On Peak / HFD autofocus failure", INDIGO_OK_STATE, INDIGO_RW_PERM, INDIGO_ONE_OF_MANY_RULE, 2);
		if (AGENT_IMAGER_FOCUS_FAILURE_PROPERTY == NULL)
			return INDIGO_FAILED;
		indigo_init_switch_item(AGENT_IMAGER_FOCUS_FAILURE_STOP_ITEM, AGENT_IMAGER_FOCUS_FAILURE_STOP_ITEM_NAME, "Stop on failure", false);
		indigo_init_switch_item(AGENT_IMAGER_FOCUS_FAILURE_RESTORE_ITEM, AGENT_IMAGER_FOCUS_FAILURE_RESTORE_ITEM_NAME, "Goto starting position", true);
		// -------------------------------------------------------------------------------- Focus Quality Estimator
		AGENT_IMAGER_FOCUS_ESTIMATOR_PROPERTY = indigo_init_switch_property(NULL, device->name, AGENT_IMAGER_FOCUS_ESTIMATOR_PROPERTY_NAME, "Agent", "Focus estimator", INDIGO_OK_STATE, INDIGO_RW_PERM, INDIGO_ONE_OF_MANY_RULE, 4);
		if (AGENT_IMAGER_FOCUS_ESTIMATOR_PROPERTY == NULL)
			return INDIGO_FAILED;
		indigo_init_switch_item(AGENT_IMAGER_FOCUS_ESTIMATOR_UCURVE_ITEM, AGENT_IMAGER_FOCUS_ESTIMATOR_UCURVE_ITEM_NAME, "U-Curve (HFD)", true);
		indigo_init_switch_item(AGENT_IMAGER_FOCUS_ESTIMATOR_HFD_PEAK_ITEM, AGENT_IMAGER_FOCUS_ESTIMATOR_HFD_PEAK_ITEM_NAME, "Iterative (HFD/Peak)", false);
		indigo_init_switch_item(AGENT_IMAGER_FOCUS_ESTIMATOR_RMS_CONTRAST_ITEM, AGENT_IMAGER_FOCUS_ESTIMATOR_RMS_CONTRAST_ITEM_NAME, "Iterative (RMS/contrast)", false);
		indigo_init_switch_item(AGENT_IMAGER_FOCUS_ESTIMATOR_BAHTINOV_ITEM, AGENT_IMAGER_FOCUS_ESTIMATOR_BAHTINOV_ITEM_NAME, "Iterative (Bahtinov)", false);
		// -------------------------------------------------------------------------------- Plain capture
		AGENT_IMAGER_CAPTURE_PROPERTY = indigo_init_number_property(NULL, device->name, AGENT_IMAGER_CAPTURE_PROPERTY_NAME, "Agent", "Capture frame", INDIGO_OK_STATE, INDIGO_RW_PERM, 1);
		if (AGENT_IMAGER_CAPTURE_PROPERTY == NULL)
			return INDIGO_FAILED;
		indigo_init_number_item(AGENT_IMAGER_CAPTURE_ITEM, AGENT_IMAGER_CAPTURE_ITEM_NAME, "Capture single frame", 0, 100000, 1, 0);
		// -------------------------------------------------------------------------------- Process properties
		AGENT_START_PROCESS_PROPERTY = indigo_init_switch_property(NULL, device->name, AGENT_START_PROCESS_PROPERTY_NAME, "Agent", "Start process", INDIGO_OK_STATE, INDIGO_RW_PERM, INDIGO_AT_MOST_ONE_RULE, 7);
		if (AGENT_START_PROCESS_PROPERTY == NULL)
			return INDIGO_FAILED;
		indigo_init_switch_item(AGENT_IMAGER_START_PREVIEW_1_ITEM, AGENT_IMAGER_START_PREVIEW_1_ITEM_NAME, "Preview single frame", false);
		indigo_init_switch_item(AGENT_IMAGER_START_PREVIEW_ITEM, AGENT_IMAGER_START_PREVIEW_ITEM_NAME, "Start preview", false);
		indigo_init_switch_item(AGENT_IMAGER_START_EXPOSURE_ITEM, AGENT_IMAGER_START_EXPOSURE_ITEM_NAME, "Start exposure batch", false);
		indigo_init_switch_item(AGENT_IMAGER_START_STREAMING_ITEM, AGENT_IMAGER_START_STREAMING_ITEM_NAME, "Start streaming batch", false);
		indigo_init_switch_item(AGENT_IMAGER_START_FOCUSING_ITEM, AGENT_IMAGER_START_FOCUSING_ITEM_NAME, "Start focusing", false);
		indigo_init_switch_item(AGENT_IMAGER_START_SEQUENCE_ITEM, AGENT_IMAGER_START_SEQUENCE_ITEM_NAME, "Start sequence", false);
		indigo_init_switch_item(AGENT_IMAGER_CLEAR_SELECTION_ITEM, AGENT_IMAGER_CLEAR_SELECTION_ITEM_NAME, "Clear star selection", false);
		AGENT_PAUSE_PROCESS_PROPERTY = indigo_init_switch_property(NULL, device->name, AGENT_PAUSE_PROCESS_PROPERTY_NAME, "Agent", "Pause/Resume process", INDIGO_OK_STATE, INDIGO_RW_PERM, INDIGO_AT_MOST_ONE_RULE, 3);
		if (AGENT_PAUSE_PROCESS_PROPERTY == NULL)
			return INDIGO_FAILED;
		indigo_init_switch_item(AGENT_PAUSE_PROCESS_ITEM, AGENT_PAUSE_PROCESS_ITEM_NAME, "Pause/resume process (with abort)", false);
		indigo_init_switch_item(AGENT_PAUSE_PROCESS_WAIT_ITEM, AGENT_PAUSE_PROCESS_WAIT_ITEM_NAME, "Pause/resume process (with wait)", false);
		indigo_init_switch_item(AGENT_PAUSE_PROCESS_AFTER_TRANSIT_ITEM, AGENT_PAUSE_PROCESS_AFTER_TRANSIT_ITEM_NAME, "Pause/resume process (at transit)", false);
		AGENT_ABORT_PROCESS_PROPERTY = indigo_init_switch_property(NULL, device->name, AGENT_ABORT_PROCESS_PROPERTY_NAME, "Agent", "Abort process", INDIGO_OK_STATE, INDIGO_RW_PERM, INDIGO_AT_MOST_ONE_RULE, 1);
		if (AGENT_ABORT_PROCESS_PROPERTY == NULL)
			return INDIGO_FAILED;
		indigo_init_switch_item(AGENT_ABORT_PROCESS_ITEM, AGENT_ABORT_PROCESS_ITEM_NAME, "Abort process", false);
		AGENT_PROCESS_FEATURES_PROPERTY = indigo_init_switch_property(NULL, device->name, AGENT_PROCESS_FEATURES_PROPERTY_NAME, "Agent", "Process features", INDIGO_OK_STATE, INDIGO_RW_PERM, INDIGO_ANY_OF_MANY_RULE, 4);
		if (AGENT_PROCESS_FEATURES_PROPERTY == NULL)
			return INDIGO_FAILED;
		indigo_init_switch_item(AGENT_IMAGER_ENABLE_DITHERING_FEATURE_ITEM, AGENT_IMAGER_ENABLE_DITHERING_FEATURE_ITEM_NAME, "Enable dithering", true);
		indigo_init_switch_item(AGENT_IMAGER_DITHER_AFTER_BATCH_FEATURE_ITEM, AGENT_IMAGER_DITHER_AFTER_BATCH_FEATURE_ITEM_NAME, "Dither after last frame", false);
		indigo_init_switch_item(AGENT_IMAGER_PAUSE_AFTER_TRANSIT_FEATURE_ITEM, AGENT_IMAGER_PAUSE_AFTER_TRANSIT_FEATURE_ITEM_NAME, "Pause after transit", false);
		indigo_init_switch_item(AGENT_IMAGER_MACRO_MODE_FEATURE_ITEM, AGENT_IMAGER_MACRO_MODE_FEATURE_ITEM_NAME, "Use macro mode", false);
		// -------------------------------------------------------------------------------- Download properties
		AGENT_IMAGER_DOWNLOAD_FILE_PROPERTY = indigo_init_text_property(NULL, device->name, AGENT_IMAGER_DOWNLOAD_FILE_PROPERTY_NAME, "Agent", "Download image", INDIGO_OK_STATE, INDIGO_RW_PERM, 1);
		if (AGENT_IMAGER_DOWNLOAD_FILE_PROPERTY == NULL)
			return INDIGO_FAILED;
		indigo_init_text_item(AGENT_IMAGER_DOWNLOAD_FILE_ITEM, AGENT_IMAGER_DOWNLOAD_FILE_ITEM_NAME, "File name", "");
		AGENT_IMAGER_DOWNLOAD_FILES_PROPERTY = indigo_init_switch_property(NULL, device->name, AGENT_IMAGER_DOWNLOAD_FILES_PROPERTY_NAME, "Agent", "Download image list", INDIGO_OK_STATE, INDIGO_RW_PERM, INDIGO_ANY_OF_MANY_RULE, INDIGO_PREALLOCATED_COUNT);
		AGENT_IMAGER_DOWNLOAD_FILES_PROPERTY->count = 1;
		if (AGENT_IMAGER_DOWNLOAD_FILES_PROPERTY == NULL)
			return INDIGO_FAILED;
		indigo_init_switch_item(AGENT_IMAGER_DOWNLOAD_FILES_REFRESH_ITEM, AGENT_IMAGER_DOWNLOAD_FILES_REFRESH_ITEM_NAME, "Refresh", false);
		AGENT_IMAGER_DOWNLOAD_IMAGE_PROPERTY = indigo_init_blob_property(NULL, device->name, AGENT_IMAGER_DOWNLOAD_IMAGE_PROPERTY_NAME, "Agent", "Download image data", INDIGO_OK_STATE, 1);
		if (AGENT_IMAGER_DOWNLOAD_IMAGE_PROPERTY == NULL)
			return INDIGO_FAILED;
		indigo_init_blob_item(AGENT_IMAGER_DOWNLOAD_IMAGE_ITEM, AGENT_IMAGER_DOWNLOAD_IMAGE_ITEM_NAME, "Image");
		AGENT_IMAGER_DELETE_FILE_PROPERTY = indigo_init_text_property(NULL, device->name, AGENT_IMAGER_DELETE_FILE_PROPERTY_NAME, "Agent", "Delete image", INDIGO_OK_STATE, INDIGO_RW_PERM, 1);
		if (AGENT_IMAGER_DELETE_FILE_PROPERTY == NULL)
			return INDIGO_FAILED;
		indigo_init_text_item(AGENT_IMAGER_DELETE_FILE_ITEM, AGENT_IMAGER_DELETE_FILE_ITEM_NAME, "File name", "");
		// -------------------------------------------------------------------------------- Wheel helpers
		AGENT_WHEEL_FILTER_PROPERTY = indigo_init_switch_property(NULL, device->name, AGENT_WHEEL_FILTER_PROPERTY_NAME, "Agent", "Selected filter", INDIGO_OK_STATE, INDIGO_RW_PERM, INDIGO_ONE_OF_MANY_RULE, FILTER_SLOT_COUNT);
		if (AGENT_WHEEL_FILTER_PROPERTY == NULL)
			return INDIGO_FAILED;
		for (int i = 0; i < FILTER_SLOT_COUNT; i++) {
			char name[8], label[32];
			sprintf(name, "%d", i + 1);
			sprintf(label, "Filter #%d", i + 1);
			indigo_init_switch_item(AGENT_WHEEL_FILTER_PROPERTY->items + i, name, label, false);
		}
		AGENT_WHEEL_FILTER_PROPERTY->count = 0;
		// -------------------------------------------------------------------------------- Focuser helpers
		AGENT_FOCUSER_CONTROL_PROPERTY = indigo_init_switch_property(NULL, device->name, AGENT_FOCUSER_CONTROL_PROPERTY_NAME, "Agent", "Focus", INDIGO_OK_STATE, INDIGO_RW_PERM, INDIGO_ANY_OF_MANY_RULE, 2);
		if (AGENT_FOCUSER_CONTROL_PROPERTY == NULL)
			return INDIGO_FAILED;
		indigo_init_switch_item(AGENT_FOCUSER_FOCUS_IN_ITEM, AGENT_FOCUSER_FOCUS_IN_ITEM_NAME, "Focus in", false);
		indigo_init_switch_item(AGENT_FOCUSER_FOCUS_OUT_ITEM, AGENT_FOCUSER_FOCUS_OUT_ITEM_NAME, "Focus out", false);
		// -------------------------------------------------------------------------------- Detected stars
		AGENT_IMAGER_STARS_PROPERTY = indigo_init_switch_property(NULL, device->name, AGENT_IMAGER_STARS_PROPERTY_NAME, "Agent", "Stars", INDIGO_OK_STATE, INDIGO_RW_PERM, INDIGO_ONE_OF_MANY_RULE, MAX_STAR_COUNT + 1);
		if (AGENT_IMAGER_STARS_PROPERTY == NULL)
			return INDIGO_FAILED;
		AGENT_IMAGER_STARS_PROPERTY->count = 1;
		indigo_init_switch_item(AGENT_IMAGER_STARS_REFRESH_ITEM, AGENT_IMAGER_STARS_REFRESH_ITEM_NAME, "Refresh", false);
		// -------------------------------------------------------------------------------- Selected star
		AGENT_IMAGER_SELECTION_PROPERTY = indigo_init_number_property(NULL, device->name, AGENT_IMAGER_SELECTION_PROPERTY_NAME, "Agent", "Selection", INDIGO_OK_STATE, INDIGO_RW_PERM, 11 + 2 * INDIGO_MAX_MULTISTAR_COUNT);
		if (AGENT_IMAGER_SELECTION_PROPERTY == NULL)
			return INDIGO_FAILED;
		indigo_init_number_item(AGENT_IMAGER_SELECTION_RADIUS_ITEM, AGENT_IMAGER_SELECTION_RADIUS_ITEM_NAME, "Radius (px)", 1, 75, 1, 12);
		indigo_init_number_item(AGENT_IMAGER_SELECTION_SUBFRAME_ITEM, AGENT_IMAGER_SELECTION_SUBFRAME_ITEM_NAME, "Subframe", 0, 10, 1, 0);
		indigo_init_number_item(AGENT_IMAGER_SELECTION_INCLUDE_LEFT_ITEM, AGENT_IMAGER_SELECTION_INCLUDE_LEFT_ITEM_NAME, "Include left (px)", 0, 0xFFFF, 1, 0);
		indigo_init_number_item(AGENT_IMAGER_SELECTION_INCLUDE_TOP_ITEM, AGENT_IMAGER_SELECTION_INCLUDE_TOP_ITEM_NAME, "Include top (px)", 0, 0xFFFF, 1, 0);
		indigo_init_number_item(AGENT_IMAGER_SELECTION_INCLUDE_WIDTH_ITEM, AGENT_IMAGER_SELECTION_INCLUDE_WIDTH_ITEM_NAME, "Include width (px)", 0, 0xFFFF, 1, 0);
		indigo_init_number_item(AGENT_IMAGER_SELECTION_INCLUDE_HEIGHT_ITEM, AGENT_IMAGER_SELECTION_INCLUDE_HEIGHT_ITEM_NAME, "Include height (px)", 0, 0xFFFF, 1, 0);
		indigo_init_number_item(AGENT_IMAGER_SELECTION_EXCLUDE_LEFT_ITEM, AGENT_IMAGER_SELECTION_EXCLUDE_LEFT_ITEM_NAME, "Exclude left (px)", 0, 0xFFFF, 1, 0);
		indigo_init_number_item(AGENT_IMAGER_SELECTION_EXCLUDE_TOP_ITEM, AGENT_IMAGER_SELECTION_EXCLUDE_TOP_ITEM_NAME, "Exclude top (px)", 0, 0xFFFF, 1, 0);
		indigo_init_number_item(AGENT_IMAGER_SELECTION_EXCLUDE_WIDTH_ITEM, AGENT_IMAGER_SELECTION_EXCLUDE_WIDTH_ITEM_NAME, "Exclude width (px)", 0, 0xFFFF, 1, 0);
		indigo_init_number_item(AGENT_IMAGER_SELECTION_EXCLUDE_HEIGHT_ITEM, AGENT_IMAGER_SELECTION_EXCLUDE_HEIGHT_ITEM_NAME, "Exclude height (px)", 0, 0xFFFF, 1, 0);
		indigo_init_number_item(AGENT_IMAGER_SELECTION_STAR_COUNT_ITEM, AGENT_IMAGER_SELECTION_STAR_COUNT_ITEM_NAME, "Maximum number of stars", 1, INDIGO_MAX_MULTISTAR_COUNT, 1, 1);
		for (int i = 0; i < INDIGO_MAX_MULTISTAR_COUNT; i++) {
			indigo_item *item_x = AGENT_IMAGER_SELECTION_X_ITEM + 2 * i;
			indigo_item *item_y = AGENT_IMAGER_SELECTION_Y_ITEM + 2 * i;
			char name[INDIGO_NAME_SIZE], label[INDIGO_VALUE_SIZE];
			sprintf(name, i ? "%s_%d" : "%s", AGENT_IMAGER_SELECTION_X_ITEM_NAME, i + 1);
			sprintf(label, i ? "Selection #%d X (px)" : "Selection X (px)", i + 1);
			indigo_init_number_item(item_x, name, label, 0, 0xFFFF, 1, 0);
			sprintf(name, i ? "%s_%d" : "%s", AGENT_IMAGER_SELECTION_Y_ITEM_NAME, i + 1);
			sprintf(label, i ? "Selection #%d Y (px)" : "Selection Y (px)", i + 1);
			indigo_init_number_item(item_y, name, label, 0, 0xFFFF, 1, 0);
		}
		AGENT_IMAGER_SELECTION_PROPERTY->count = 13;

		// -------------------------------------------------------------------------------- Spikes generated by bahtinov mask
		AGENT_IMAGER_SPIKES_PROPERTY = indigo_init_number_property(NULL, device->name, AGENT_IMAGER_SPIKES_PROPERTY_NAME, "Agent", "Bahtinov spikes", INDIGO_OK_STATE, INDIGO_RO_PERM, 6);
		if (AGENT_IMAGER_SELECTION_PROPERTY == NULL)
			return INDIGO_FAILED;
		indigo_init_number_item(AGENT_IMAGER_SPIKE_1_RHO_ITEM, AGENT_IMAGER_SPIKE_1_RHO_ITEM_NAME, "Spike #1 ρ (px)", 0, 0xFFFFFFFF, 0, 0);
		indigo_init_number_item(AGENT_IMAGER_SPIKE_1_THETA_ITEM, AGENT_IMAGER_SPIKE_1_THETA_ITEM_NAME, "Spike #1 θ (px)", 0, 0xFFFFFFFF, 0, 0);
		indigo_init_number_item(AGENT_IMAGER_SPIKE_2_RHO_ITEM, AGENT_IMAGER_SPIKE_2_RHO_ITEM_NAME, "Spike #2 ρ (px)", 0, 0xFFFFFFFF, 0, 0);
		indigo_init_number_item(AGENT_IMAGER_SPIKE_2_THETA_ITEM, AGENT_IMAGER_SPIKE_2_THETA_ITEM_NAME, "Spike #2 θ (px)", 0, 0xFFFFFFFF, 0, 0);
		indigo_init_number_item(AGENT_IMAGER_SPIKE_3_RHO_ITEM, AGENT_IMAGER_SPIKE_3_RHO_ITEM_NAME, "Spike #3 ρ (px)", 0, 0xFFFFFFFF, 0, 0);
		indigo_init_number_item(AGENT_IMAGER_SPIKE_3_THETA_ITEM, AGENT_IMAGER_SPIKE_3_THETA_ITEM_NAME, "Spike #3 θ (px)", 0, 0xFFFFFFFF, 0, 0);

		// -------------------------------------------------------------------------------- Focusing stats
		AGENT_IMAGER_STATS_PROPERTY = indigo_init_number_property(NULL, device->name, AGENT_IMAGER_STATS_PROPERTY_NAME, "Agent", "Statistics", INDIGO_OK_STATE, INDIGO_RO_PERM, 21 + INDIGO_MAX_MULTISTAR_COUNT - 1);
		if (AGENT_IMAGER_STATS_PROPERTY == NULL)
			return INDIGO_FAILED;
		indigo_init_number_item(AGENT_IMAGER_STATS_EXPOSURE_ITEM, AGENT_IMAGER_STATS_EXPOSURE_ITEM_NAME, "Exposure remaining (s)", 0, 3600, 0, 0);
		indigo_init_number_item(AGENT_IMAGER_STATS_DELAY_ITEM, AGENT_IMAGER_STATS_DELAY_ITEM_NAME, "Delay remaining (s)", 0, 3600, 0, 0);
		indigo_init_number_item(AGENT_IMAGER_STATS_FRAME_ITEM, AGENT_IMAGER_STATS_FRAME_ITEM_NAME, "Current frame", 0, 0xFFFFFFFF, 0, 0);
		indigo_init_number_item(AGENT_IMAGER_STATS_FRAMES_ITEM, AGENT_IMAGER_STATS_FRAMES_ITEM_NAME, "Frame count", 0, 0xFFFFFFFF, 0, 0);
		indigo_init_number_item(AGENT_IMAGER_STATS_BATCH_INDEX_ITEM, AGENT_IMAGER_STATS_BATCH_INDEX_ITEM_NAME, "Current batch index", 0, 0xFFFFFFFF, 0, 0);
		indigo_init_number_item(AGENT_IMAGER_STATS_BATCH_ITEM, AGENT_IMAGER_STATS_BATCH_ITEM_NAME, "Current batch", 0, 0xFFFFFFFF, 0, 0);
		indigo_init_number_item(AGENT_IMAGER_STATS_BATCHES_ITEM, AGENT_IMAGER_STATS_BATCHES_ITEM_NAME, "Batch count", 0, 0xFFFFFFFF, 0, 0);
		indigo_init_number_item(AGENT_IMAGER_STATS_PHASE_ITEM, AGENT_IMAGER_STATS_PHASE_ITEM_NAME, "Batch phase", 0, 0xFFFFFFFF, 0, 0);
		indigo_init_number_item(AGENT_IMAGER_STATS_DRIFT_X_ITEM, AGENT_IMAGER_STATS_DRIFT_X_ITEM_NAME, "Drift X", -1000, 1000, 0, 0);
		indigo_init_number_item(AGENT_IMAGER_STATS_DRIFT_Y_ITEM, AGENT_IMAGER_STATS_DRIFT_Y_ITEM_NAME, "Drift Y", -1000, 1000, 0, 0);
		indigo_init_number_item(AGENT_IMAGER_STATS_DITHERING_ITEM, AGENT_IMAGER_STATS_DITHERING_ITEM_NAME, "Dithering RMSE", 0, 0xFFFF, 0, 0);
		indigo_init_number_item(AGENT_IMAGER_STATS_FOCUS_OFFSET_ITEM, AGENT_IMAGER_STATS_FOCUS_OFFSET_ITEM_NAME, "Autofocus offset", -0xFFFF, 0xFFFF, 0, 0);
		indigo_init_number_item(AGENT_IMAGER_STATS_FOCUS_POSITION_ITEM, AGENT_IMAGER_STATS_FOCUS_POSITION_ITEM_NAME, "Focuser position", -0xFFFFFF, 0xFFFFFF, 0, 0);
		indigo_init_number_item(AGENT_IMAGER_STATS_RMS_CONTRAST_ITEM, AGENT_IMAGER_STATS_RMS_CONTRAST_ITEM_NAME, "RMS contrast", 0, 1, 0, 0);
		indigo_init_number_item(AGENT_IMAGER_STATS_FOCUS_DEVIATION_ITEM, AGENT_IMAGER_STATS_FOCUS_DEVIATION_ITEM_NAME, "Best focus deviation (%)", -100, 100, 0, 100);
		indigo_init_number_item(AGENT_IMAGER_STATS_FRAMES_TO_DITHERING_ITEM, AGENT_IMAGER_STATS_FRAMES_TO_DITHERING_ITEM_NAME, "Frames to dithering", 0, 0xFFFFFFFF, 0, 0);
		indigo_init_number_item(AGENT_IMAGER_STATS_BAHTINOV_ITEM, AGENT_IMAGER_STATS_BAHTINOV_ITEM_NAME, "Bahtinov mask focus error", 0, 0xFFFFFFFF, 0, 0);
		indigo_init_number_item(AGENT_IMAGER_STATS_MAX_STARS_TO_USE_ITEM, AGENT_IMAGER_STATS_MAX_STARS_TO_USE_ITEM_NAME, "Maximum number of stars to use", 0, 0xFFFFFFFF, 0, AGENT_IMAGER_SELECTION_STAR_COUNT_ITEM->number.value);
		indigo_init_number_item(AGENT_IMAGER_STATS_PEAK_ITEM, AGENT_IMAGER_STATS_PEAK_ITEM_NAME, "Peak", 0, 0xFFFF, 0, 0);
		indigo_init_number_item(AGENT_IMAGER_STATS_FWHM_ITEM, AGENT_IMAGER_STATS_FWHM_ITEM_NAME, "FWHM", 0, 0xFFFF, 0, 0);
		// initialize HFD stats
		for (int i = 0; i < INDIGO_MAX_MULTISTAR_COUNT; i++) {
			indigo_item *item = AGENT_IMAGER_STATS_HFD_ITEM + i;
			char name[INDIGO_NAME_SIZE], label[INDIGO_VALUE_SIZE];
			sprintf(name, i ? "%s_%d" : "%s", AGENT_IMAGER_STATS_HFD_ITEM_NAME, i + 1);
			sprintf(label, i ? "HFD #%d" : "HFD", i + 1);
			indigo_init_number_item(item, name, label, 0, 0xFFFF, 1, 0);
		}
		AGENT_IMAGER_STATS_PROPERTY->count = 21;

		// -------------------------------------------------------------------------------- Sequence size
		AGENT_IMAGER_SEQUENCE_SIZE_PROPERTY = indigo_init_number_property(NULL, device->name, AGENT_IMAGER_SEQUENCE_SIZE_PROPERTY_NAME, "Agent", "Sequence size", INDIGO_OK_STATE, INDIGO_RW_PERM, 1);
		if (AGENT_IMAGER_SEQUENCE_SIZE_PROPERTY == NULL)
			return INDIGO_FAILED;
		indigo_init_number_item(AGENT_IMAGER_SEQUENCE_SIZE_ITEM, AGENT_IMAGER_SEQUENCE_SIZE_ITEM_NAME, "Number of batches", 1, MAX_SEQUENCE_SIZE, 1, SEQUENCE_SIZE);
		// -------------------------------------------------------------------------------- Sequencer
		AGENT_IMAGER_SEQUENCE_PROPERTY = indigo_init_text_property(NULL, device->name, AGENT_IMAGER_SEQUENCE_PROPERTY_NAME, "Agent", "Sequence", INDIGO_OK_STATE, INDIGO_RW_PERM, 1 + MAX_SEQUENCE_SIZE);
		if (AGENT_IMAGER_SEQUENCE_PROPERTY == NULL)
			return INDIGO_FAILED;
		indigo_init_text_item(AGENT_IMAGER_SEQUENCE_ITEM, AGENT_IMAGER_SEQUENCE_ITEM_NAME, "Sequence", "");
		for (int i = 1; i <= MAX_SEQUENCE_SIZE; i++) {
			char name[32], label[32];
			sprintf(name, "%02d", i);
			sprintf(label, "Batch #%d", i);
			indigo_init_text_item(AGENT_IMAGER_SEQUENCE_PROPERTY->items + i, name, label, "");
		}
		AGENT_IMAGER_SEQUENCE_PROPERTY->count = SEQUENCE_SIZE + 1;
		// -------------------------------------------------------------------------------- Breakpoint support
		AGENT_IMAGER_BREAKPOINT_PROPERTY = indigo_init_switch_property(NULL, device->name, AGENT_IMAGER_BREAKPOINT_PROPERTY_NAME, MAIN_GROUP, "Breakpoints", INDIGO_OK_STATE, INDIGO_RW_PERM, INDIGO_ANY_OF_MANY_RULE, 6);
		if (AGENT_IMAGER_BREAKPOINT_PROPERTY == NULL)
			return INDIGO_FAILED;
		indigo_init_switch_item(AGENT_IMAGER_BREAKPOINT_PRE_BATCH_ITEM, AGENT_IMAGER_BREAKPOINT_PRE_BATCH_ITEM_NAME, "Pre-batch", false);
		indigo_init_switch_item(AGENT_IMAGER_BREAKPOINT_PRE_CAPTURE_ITEM, AGENT_IMAGER_BREAKPOINT_PRE_CAPTURE_ITEM_NAME, "Pre-capture", false);
		indigo_init_switch_item(AGENT_IMAGER_BREAKPOINT_POST_CAPTURE_ITEM, AGENT_IMAGER_BREAKPOINT_POST_CAPTURE_ITEM_NAME, "Post-capture", false);
		indigo_init_switch_item(AGENT_IMAGER_BREAKPOINT_PRE_DELAY_ITEM, AGENT_IMAGER_BREAKPOINT_PRE_DELAY_ITEM_NAME, "Pre-delay", false);
		indigo_init_switch_item(AGENT_IMAGER_BREAKPOINT_POST_DELAY_ITEM, AGENT_IMAGER_BREAKPOINT_POST_DELAY_ITEM_NAME, "Post-delay", false);
		indigo_init_switch_item(AGENT_IMAGER_BREAKPOINT_POST_BATCH_ITEM, AGENT_IMAGER_BREAKPOINT_POST_BATCH_ITEM_NAME, "Post-batch", false);
		AGENT_IMAGER_RESUME_CONDITION_PROPERTY = indigo_init_switch_property(NULL, device->name, AGENT_IMAGER_RESUME_CONDITION_PROPERTY_NAME, MAIN_GROUP, "Breakpoint resume condition", INDIGO_OK_STATE, INDIGO_RW_PERM, INDIGO_ONE_OF_MANY_RULE, 2);
		if (AGENT_IMAGER_RESUME_CONDITION_PROPERTY == NULL)
			return INDIGO_FAILED;
		indigo_init_switch_item(AGENT_IMAGER_RESUME_CONDITION_TRIGGER_ITEM, AGENT_IMAGER_RESUME_CONDITION_TRIGGER_ITEM_NAME, "Trigger/manual", true);
		indigo_init_switch_item(AGENT_IMAGER_RESUME_CONDITION_BARRIER_ITEM, AGENT_IMAGER_RESUME_CONDITION_BARRIER_ITEM_NAME, "Barrier", false);
		AGENT_IMAGER_BARRIER_STATE_PROPERTY = indigo_init_light_property(NULL, device->name, AGENT_IMAGER_BARRIER_STATE_PROPERTY_NAME, MAIN_GROUP, "Breakpoint barrier state", INDIGO_OK_STATE, 0);
		if (AGENT_IMAGER_BARRIER_STATE_PROPERTY == NULL)
			return INDIGO_FAILED;
		// --------------------------------------------------------------------------------
		DEVICE_PRIVATE_DATA->bin_x = DEVICE_PRIVATE_DATA->bin_y = 1;
		CONNECTION_PROPERTY->hidden = true;
		ADDITIONAL_INSTANCES_PROPERTY->hidden = DEVICE_CONTEXT->base_device != NULL;
		pthread_mutex_init(&DEVICE_PRIVATE_DATA->mutex, NULL);
		pthread_mutex_init(&DEVICE_PRIVATE_DATA->last_image_mutex, NULL);
		indigo_load_properties(device, false);
		INDIGO_DEVICE_ATTACH_LOG(DRIVER_NAME, device->name);
		return agent_enumerate_properties(device, NULL, NULL);
	}
	return INDIGO_FAILED;
}

static indigo_result agent_enumerate_properties(indigo_device *device, indigo_client *client, indigo_property *property) {
	if (client != NULL && client == FILTER_DEVICE_CONTEXT->client)
		return INDIGO_OK;
	indigo_define_matching_property(AGENT_IMAGER_BATCH_PROPERTY);
	indigo_define_matching_property(AGENT_IMAGER_FOCUS_PROPERTY);
	indigo_define_matching_property(AGENT_IMAGER_FOCUS_FAILURE_PROPERTY);
	indigo_define_matching_property(AGENT_IMAGER_FOCUS_ESTIMATOR_PROPERTY);
	indigo_define_matching_property(AGENT_IMAGER_DOWNLOAD_IMAGE_PROPERTY);
	indigo_define_matching_property(AGENT_IMAGER_DOWNLOAD_FILE_PROPERTY);
	indigo_define_matching_property(AGENT_IMAGER_DOWNLOAD_FILES_PROPERTY);
	indigo_define_matching_property(AGENT_IMAGER_DELETE_FILE_PROPERTY);
	indigo_define_matching_property(AGENT_IMAGER_CAPTURE_PROPERTY);
	indigo_define_matching_property(AGENT_START_PROCESS_PROPERTY);
	indigo_define_matching_property(AGENT_PAUSE_PROCESS_PROPERTY);
	indigo_define_matching_property(AGENT_ABORT_PROCESS_PROPERTY);
	indigo_define_matching_property(AGENT_PROCESS_FEATURES_PROPERTY);
	indigo_define_matching_property(AGENT_IMAGER_STARS_PROPERTY);
	indigo_define_matching_property(AGENT_IMAGER_SELECTION_PROPERTY);
	indigo_define_matching_property(AGENT_IMAGER_STATS_PROPERTY);
	indigo_define_matching_property(AGENT_IMAGER_SPIKES_PROPERTY);
	indigo_define_matching_property(AGENT_IMAGER_SEQUENCE_PROPERTY);
	indigo_define_matching_property(AGENT_IMAGER_SEQUENCE_SIZE_PROPERTY);
	indigo_define_matching_property(AGENT_IMAGER_BREAKPOINT_PROPERTY);
	indigo_define_matching_property(AGENT_IMAGER_RESUME_CONDITION_PROPERTY);
	indigo_define_matching_property(AGENT_IMAGER_BARRIER_STATE_PROPERTY);
	indigo_define_matching_property(AGENT_FOCUSER_CONTROL_PROPERTY);
	return indigo_filter_enumerate_properties(device, client, property);
}

static indigo_result agent_change_property(indigo_device *device, indigo_client *client, indigo_property *property) {
	assert(device != NULL);
	assert(DEVICE_CONTEXT != NULL);
	assert(property != NULL);
	if (indigo_property_match(FILTER_CCD_LIST_PROPERTY, property)) {
// -------------------------------------------------------------------------------- FILTER_CCD_LIST_PROPERTY
		if (!FILTER_DEVICE_CONTEXT->running_process) {
			bool reset_selection = true;
			for (int i = 0; i < property->count; i++) {
				if (property->items[i].sw.value) {
					for (int j = 0; j < FILTER_CCD_LIST_PROPERTY->count; j++) {
						if (FILTER_CCD_LIST_PROPERTY->items[j].sw.value) {
							if (!strcmp(property->items[i].name, FILTER_CCD_LIST_PROPERTY->items[j].name))
								reset_selection = false;
							break;
						}
					}
				}
			}
			if (reset_selection) {
				for (int i = 0; i < AGENT_IMAGER_SELECTION_STAR_COUNT_ITEM->number.value; i++) {
					indigo_item *item_x = AGENT_IMAGER_SELECTION_X_ITEM + 2 * i;
					indigo_item *item_y = AGENT_IMAGER_SELECTION_Y_ITEM + 2 * i;
					item_x->number.target = item_x->number.value = 0;
					item_y->number.target = item_y->number.value = 0;
				}
				indigo_update_property(device, AGENT_IMAGER_SELECTION_PROPERTY, NULL);
			}
		}
	} else if (indigo_property_match(AGENT_IMAGER_BATCH_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- AGENT_IMAGER_BATCH
		indigo_property_copy_values(AGENT_IMAGER_BATCH_PROPERTY, property, false);
		AGENT_IMAGER_BATCH_PROPERTY->state = INDIGO_OK_STATE;
		save_config(device);
		indigo_update_property(device, AGENT_IMAGER_BATCH_PROPERTY, NULL);
		return INDIGO_OK;
	} else if (indigo_property_match(AGENT_IMAGER_FOCUS_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- AGENT_IMAGER_FOCUS
		double initial_step = AGENT_IMAGER_FOCUS_INITIAL_ITEM->number.value; // temporary solution
		double final_step = AGENT_IMAGER_FOCUS_FINAL_ITEM->number.value;
		double iterative_initial_step = AGENT_IMAGER_FOCUS_ITERATIVE_INITIAL_ITEM->number.value;
		double iterative_final_step = AGENT_IMAGER_FOCUS_ITERATIVE_FINAL_ITEM->number.value;
		double ucurve_step = AGENT_IMAGER_FOCUS_UCURVE_STEP_ITEM->number.value;
		indigo_property_copy_values(AGENT_IMAGER_FOCUS_PROPERTY, property, false);
		if (AGENT_IMAGER_FOCUS_ITERATIVE_INITIAL_ITEM->number.value != iterative_initial_step || AGENT_IMAGER_FOCUS_UCURVE_STEP_ITEM->number.value != ucurve_step) {
			// ignore
		} else if (AGENT_IMAGER_FOCUS_INITIAL_ITEM->number.value != initial_step) {
			AGENT_IMAGER_FOCUS_ITERATIVE_INITIAL_ITEM->number.target = AGENT_IMAGER_FOCUS_ITERATIVE_INITIAL_ITEM->number.value = AGENT_IMAGER_FOCUS_INITIAL_ITEM->number.target;
			AGENT_IMAGER_FOCUS_UCURVE_STEP_ITEM->number.target = AGENT_IMAGER_FOCUS_UCURVE_STEP_ITEM->number.value = AGENT_IMAGER_FOCUS_INITIAL_ITEM->number.target;
		}
		if (AGENT_IMAGER_FOCUS_ITERATIVE_FINAL_ITEM->number.value != iterative_final_step) {
			// ignore
		} else if (AGENT_IMAGER_FOCUS_FINAL_ITEM->number.value != final_step) {
			AGENT_IMAGER_FOCUS_ITERATIVE_FINAL_ITEM->number.target = AGENT_IMAGER_FOCUS_ITERATIVE_FINAL_ITEM->number.value = AGENT_IMAGER_FOCUS_FINAL_ITEM->number.target;
		}
		indigo_change_number_property_1(FILTER_DEVICE_CONTEXT->client, device->name, FOCUSER_BACKLASH_PROPERTY_NAME, FOCUSER_BACKLASH_ITEM_NAME, AGENT_IMAGER_FOCUS_BACKLASH_ITEM->number.value);
		AGENT_IMAGER_FOCUS_PROPERTY->state = INDIGO_OK_STATE;
		save_config(device);
		indigo_update_property(device, AGENT_IMAGER_FOCUS_PROPERTY, NULL);
		return INDIGO_OK;
	} else if (indigo_property_match(AGENT_IMAGER_FOCUS_FAILURE_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- AGENT_IMAGER_FOCUS_FAILURE
		indigo_property_copy_values(AGENT_IMAGER_FOCUS_FAILURE_PROPERTY, property, false);
		AGENT_IMAGER_FOCUS_FAILURE_PROPERTY->state = INDIGO_OK_STATE;
		save_config(device);
		indigo_update_property(device, AGENT_IMAGER_FOCUS_FAILURE_PROPERTY, NULL);
		return INDIGO_OK;
	} else if (indigo_property_match(AGENT_IMAGER_FOCUS_ESTIMATOR_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- AGENT_IMAGER_FOCUS_ESTIMATOR
		if (FILTER_DEVICE_CONTEXT->running_process && !AGENT_IMAGER_START_PREVIEW_ITEM->sw.value) {
			indigo_update_property(device, AGENT_IMAGER_FOCUS_ESTIMATOR_PROPERTY, "Warning: Focus estimator can not be changed while process is running!");
			return INDIGO_OK;
		}
		indigo_property_copy_values(AGENT_IMAGER_FOCUS_ESTIMATOR_PROPERTY, property, false);
		DEVICE_PRIVATE_DATA->use_hfd_estimator = false;
		DEVICE_PRIVATE_DATA->use_rms_estimator = false;
		DEVICE_PRIVATE_DATA->use_bahtinov_estimator = false;
		DEVICE_PRIVATE_DATA->use_ucurve_focusing = false;
		DEVICE_PRIVATE_DATA->use_iterative_focusing = false;
		if (AGENT_IMAGER_FOCUS_ESTIMATOR_UCURVE_ITEM->sw.value) {
			DEVICE_PRIVATE_DATA->use_hfd_estimator = true;
			DEVICE_PRIVATE_DATA->use_ucurve_focusing = true;
		} else if (AGENT_IMAGER_FOCUS_ESTIMATOR_HFD_PEAK_ITEM->sw.value) {
			DEVICE_PRIVATE_DATA->use_hfd_estimator = true;
			DEVICE_PRIVATE_DATA->use_iterative_focusing = true;
		} else if (AGENT_IMAGER_FOCUS_ESTIMATOR_RMS_CONTRAST_ITEM->sw.value) {
			DEVICE_PRIVATE_DATA->use_rms_estimator = true;
			DEVICE_PRIVATE_DATA->use_iterative_focusing = true;
		} else if (AGENT_IMAGER_FOCUS_ESTIMATOR_BAHTINOV_ITEM->sw.value) {
			DEVICE_PRIVATE_DATA->use_bahtinov_estimator = true;
			DEVICE_PRIVATE_DATA->use_iterative_focusing = true;
		}
		AGENT_IMAGER_FOCUS_ESTIMATOR_PROPERTY->state = INDIGO_OK_STATE;
		adjust_stats_max_stars_to_use(device);
		clear_stats(device);
		save_config(device);
		indigo_update_property(device, AGENT_IMAGER_STATS_PROPERTY, NULL);
		indigo_update_property(device, AGENT_IMAGER_FOCUS_ESTIMATOR_PROPERTY, NULL);
		return INDIGO_OK;
	} else if (indigo_property_match(AGENT_IMAGER_STARS_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- AGENT_IMAGER_STARS
		if (FILTER_DEVICE_CONTEXT->running_process || AGENT_IMAGER_STARS_PROPERTY->state == INDIGO_BUSY_STATE) {
			indigo_update_property(device, AGENT_IMAGER_FOCUS_ESTIMATOR_PROPERTY, "Warning: Star list can not be changed while process is running!");
			return INDIGO_OK;
		}
		indigo_property_copy_values(AGENT_IMAGER_STARS_PROPERTY, property, false);
		if (AGENT_IMAGER_STARS_REFRESH_ITEM->sw.value) {
			AGENT_IMAGER_STARS_REFRESH_ITEM->sw.value = false;
			AGENT_IMAGER_STARS_PROPERTY->state = INDIGO_BUSY_STATE;
			indigo_update_property(device, AGENT_IMAGER_STARS_PROPERTY, NULL);
			indigo_set_timer(device, 0, find_stars_process, NULL);
		} else {
			for (int i = 1; i < AGENT_IMAGER_STARS_PROPERTY->count; i++) {
				if (AGENT_IMAGER_STARS_PROPERTY->items[i].sw.value) {
					int j = atoi(AGENT_IMAGER_STARS_PROPERTY->items[i].name);
					AGENT_IMAGER_SELECTION_X_ITEM->number.target = AGENT_IMAGER_SELECTION_X_ITEM->number.value = DEVICE_PRIVATE_DATA->stars[j].x;
					AGENT_IMAGER_SELECTION_Y_ITEM->number.target = AGENT_IMAGER_SELECTION_Y_ITEM->number.value = DEVICE_PRIVATE_DATA->stars[j].y;
					indigo_update_property(device, AGENT_IMAGER_SELECTION_PROPERTY, NULL);
					AGENT_IMAGER_STARS_PROPERTY->items[i].sw.value = false;
				}
			}
			AGENT_IMAGER_STARS_PROPERTY->state = INDIGO_OK_STATE;
		}
		indigo_update_property(device, AGENT_IMAGER_STARS_PROPERTY, NULL);
	} else if (indigo_property_match(AGENT_IMAGER_SELECTION_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- AGENT_IMAGER_SELECTION
		if (FILTER_DEVICE_CONTEXT->running_process && !AGENT_IMAGER_START_PREVIEW_ITEM->sw.value) {
			indigo_update_property(device, AGENT_IMAGER_SELECTION_PROPERTY, "Warning: Selection can not be changed while process is running!");
			return INDIGO_OK;
		}
		int count = (int)AGENT_IMAGER_SELECTION_STAR_COUNT_ITEM->number.value;
		double include_left = AGENT_IMAGER_SELECTION_INCLUDE_LEFT_ITEM->number.value;
		double include_top = AGENT_IMAGER_SELECTION_INCLUDE_TOP_ITEM->number.value;
		double include_width = AGENT_IMAGER_SELECTION_INCLUDE_WIDTH_ITEM->number.value;
		double include_height = AGENT_IMAGER_SELECTION_INCLUDE_HEIGHT_ITEM->number.value;
		double exclude_left = AGENT_IMAGER_SELECTION_EXCLUDE_LEFT_ITEM->number.value;
		double exclude_top = AGENT_IMAGER_SELECTION_EXCLUDE_TOP_ITEM->number.value;
		double exclude_width = AGENT_IMAGER_SELECTION_EXCLUDE_WIDTH_ITEM->number.value;
		double exclude_height = AGENT_IMAGER_SELECTION_EXCLUDE_HEIGHT_ITEM->number.value;
		indigo_property_copy_values(AGENT_IMAGER_SELECTION_PROPERTY, property, false);
		bool force = include_left != AGENT_IMAGER_SELECTION_INCLUDE_LEFT_ITEM->number.value || include_top != AGENT_IMAGER_SELECTION_INCLUDE_TOP_ITEM->number.value || include_width != AGENT_IMAGER_SELECTION_INCLUDE_WIDTH_ITEM->number.value || include_height != AGENT_IMAGER_SELECTION_INCLUDE_HEIGHT_ITEM->number.value || exclude_left != AGENT_IMAGER_SELECTION_EXCLUDE_LEFT_ITEM->number.value|| exclude_top != AGENT_IMAGER_SELECTION_EXCLUDE_TOP_ITEM->number.value || exclude_width != AGENT_IMAGER_SELECTION_EXCLUDE_WIDTH_ITEM->number.value || exclude_height != AGENT_IMAGER_SELECTION_EXCLUDE_HEIGHT_ITEM->number.value;
		validate_include_region(device, force);
		AGENT_IMAGER_SELECTION_STAR_COUNT_ITEM->number.value = AGENT_IMAGER_SELECTION_STAR_COUNT_ITEM->number.target = (int)AGENT_IMAGER_SELECTION_STAR_COUNT_ITEM->number.target;
		if (count != AGENT_IMAGER_SELECTION_STAR_COUNT_ITEM->number.value) {
			indigo_delete_property(device, AGENT_IMAGER_SELECTION_PROPERTY, NULL);
			indigo_delete_property(device, AGENT_IMAGER_STATS_PROPERTY, NULL);
			AGENT_IMAGER_SELECTION_PROPERTY->count = (int)((AGENT_IMAGER_SELECTION_X_ITEM - AGENT_IMAGER_SELECTION_PROPERTY->items) + 2 * AGENT_IMAGER_SELECTION_STAR_COUNT_ITEM->number.value);
			for (int i = 0; i < AGENT_IMAGER_SELECTION_STAR_COUNT_ITEM->number.value; i++) {
				indigo_item *item_x = AGENT_IMAGER_SELECTION_X_ITEM + 2 * i;
				indigo_item *item_y = AGENT_IMAGER_SELECTION_Y_ITEM + 2 * i;
				item_x->number.value = item_x->number.target = 0;
				item_y->number.value = item_y->number.target = 0;
			}
			adjust_stats_max_stars_to_use(device);
			for (int i = 0; i < INDIGO_MAX_MULTISTAR_COUNT; i++) {
				AGENT_IMAGER_STATS_HFD_ITEM[i].number.value = 0;
			}
			AGENT_IMAGER_STATS_PROPERTY->count = (int)((AGENT_IMAGER_STATS_HFD_ITEM - AGENT_IMAGER_STATS_PROPERTY->items) + AGENT_IMAGER_SELECTION_STAR_COUNT_ITEM->number.value);
			indigo_define_property(device, AGENT_IMAGER_SELECTION_PROPERTY, NULL);
			indigo_define_property(device, AGENT_IMAGER_STATS_PROPERTY, NULL);
		}
		clear_stats(device);
		save_config(device);
		AGENT_IMAGER_SELECTION_PROPERTY->state = INDIGO_OK_STATE;
		indigo_update_property(device, AGENT_IMAGER_SELECTION_PROPERTY, NULL);
	} else if (indigo_property_match(AGENT_IMAGER_CAPTURE_PROPERTY, property)) {
		if (AGENT_START_PROCESS_PROPERTY->state != INDIGO_BUSY_STATE || AGENT_PAUSE_PROCESS_PROPERTY->state == INDIGO_BUSY_STATE) {
			indigo_property_copy_values(AGENT_IMAGER_CAPTURE_PROPERTY, property, false);
			AGENT_IMAGER_CAPTURE_PROPERTY->state = INDIGO_BUSY_STATE;
			indigo_update_property(device, AGENT_IMAGER_CAPTURE_PROPERTY, NULL);
			indigo_set_timer(device, 0, capture, NULL);
		}
		return INDIGO_OK;
	} else if (indigo_property_match(AGENT_START_PROCESS_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- AGENT_START_PROCESS
		if (AGENT_START_PROCESS_PROPERTY->state != INDIGO_BUSY_STATE && AGENT_IMAGER_STARS_PROPERTY->state != INDIGO_BUSY_STATE && AGENT_IMAGER_CAPTURE_PROPERTY->state != INDIGO_BUSY_STATE) {
			indigo_property_copy_values(AGENT_START_PROCESS_PROPERTY, property, false);
			if (AGENT_IMAGER_CLEAR_SELECTION_ITEM->sw.value) {
				AGENT_START_PROCESS_PROPERTY->state = INDIGO_BUSY_STATE;
				indigo_set_timer(device, 0, clear_selection_process, NULL);
				indigo_update_property(device, AGENT_IMAGER_STATS_PROPERTY, NULL);
			} else if (INDIGO_FILTER_CCD_SELECTED) {
				if (AGENT_IMAGER_START_PREVIEW_1_ITEM->sw.value) {
					AGENT_START_PROCESS_PROPERTY->state = INDIGO_BUSY_STATE;
					if (DEVICE_PRIVATE_DATA->use_bahtinov_estimator && (DEVICE_PRIVATE_DATA->frame[2] > MAX_BAHTINOV_FRAME_SIZE || DEVICE_PRIVATE_DATA->frame[3] > MAX_BAHTINOV_FRAME_SIZE)) {
						indigo_send_message(device, "Warning: Bahtinov focus estimator can't process frames larger than %d x %d pixels", MAX_BAHTINOV_FRAME_SIZE, MAX_BAHTINOV_FRAME_SIZE);
					}
					indigo_set_timer(device, 0, preview_1_process, NULL);
					indigo_update_property(device, AGENT_IMAGER_STATS_PROPERTY, NULL);
				} else if (AGENT_IMAGER_START_PREVIEW_ITEM->sw.value) {
					AGENT_START_PROCESS_PROPERTY->state = INDIGO_BUSY_STATE;
					if (DEVICE_PRIVATE_DATA->use_bahtinov_estimator && (DEVICE_PRIVATE_DATA->frame[2] > MAX_BAHTINOV_FRAME_SIZE || DEVICE_PRIVATE_DATA->frame[3] > MAX_BAHTINOV_FRAME_SIZE)) {
						indigo_send_message(device, "Warning: Bahtinov focus estimator can't process frames larger than %d x %d pixels", MAX_BAHTINOV_FRAME_SIZE, MAX_BAHTINOV_FRAME_SIZE);
					}
					indigo_set_timer(device, 0, preview_process, NULL);
					indigo_update_property(device, AGENT_IMAGER_STATS_PROPERTY, NULL);
				} else if (AGENT_IMAGER_START_EXPOSURE_ITEM->sw.value && !AGENT_IMAGER_MACRO_MODE_FEATURE_ITEM->sw.value) {
					AGENT_START_PROCESS_PROPERTY->state = INDIGO_BUSY_STATE;
					indigo_set_timer(device, 0, exposure_batch_process, NULL);
					indigo_update_property(device, AGENT_IMAGER_STATS_PROPERTY, NULL);
				} else if (AGENT_IMAGER_START_STREAMING_ITEM->sw.value) {
					AGENT_START_PROCESS_PROPERTY->state = INDIGO_BUSY_STATE;
					indigo_set_timer(device, 0, streaming_batch_process, NULL);
					indigo_update_property(device, AGENT_IMAGER_STATS_PROPERTY, NULL);
				} else if (AGENT_IMAGER_START_SEQUENCE_ITEM->sw.value) {
					AGENT_START_PROCESS_PROPERTY->state = INDIGO_BUSY_STATE;
					indigo_set_timer(device, 0, sequence_process, NULL);
					indigo_update_property(device, AGENT_IMAGER_STATS_PROPERTY, NULL);
				} else if (INDIGO_FILTER_FOCUSER_SELECTED) {
					if (AGENT_IMAGER_START_FOCUSING_ITEM->sw.value) {
						AGENT_START_PROCESS_PROPERTY->state = INDIGO_BUSY_STATE;
						if (DEVICE_PRIVATE_DATA->use_bahtinov_estimator && (DEVICE_PRIVATE_DATA->frame[2] > MAX_BAHTINOV_FRAME_SIZE || DEVICE_PRIVATE_DATA->frame[3] > MAX_BAHTINOV_FRAME_SIZE)) {
							indigo_send_message(device, "Warning: Bahtinov focus estimator can't process frames larger than %d x %d pixels", MAX_BAHTINOV_FRAME_SIZE, MAX_BAHTINOV_FRAME_SIZE);
						}
						indigo_set_timer(device, 0, autofocus_process, NULL);
					} else if (AGENT_IMAGER_START_EXPOSURE_ITEM->sw.value && AGENT_IMAGER_MACRO_MODE_FEATURE_ITEM->sw.value) {
						AGENT_START_PROCESS_PROPERTY->state = INDIGO_BUSY_STATE;
						indigo_set_timer(device, 0, bracketing_batch_process, NULL);
						indigo_update_property(device, AGENT_IMAGER_STATS_PROPERTY, NULL);
					}
					indigo_update_property(device, AGENT_IMAGER_STATS_PROPERTY, NULL);
				} else {
					AGENT_IMAGER_START_PREVIEW_1_ITEM->sw.value = AGENT_IMAGER_START_PREVIEW_ITEM->sw.value = AGENT_IMAGER_START_EXPOSURE_ITEM->sw.value = AGENT_IMAGER_START_STREAMING_ITEM->sw.value = AGENT_IMAGER_START_FOCUSING_ITEM->sw.value = AGENT_IMAGER_START_SEQUENCE_ITEM->sw.value = AGENT_IMAGER_CLEAR_SELECTION_ITEM->sw.value = false;
					AGENT_START_PROCESS_PROPERTY->state = INDIGO_ALERT_STATE;
					indigo_update_property(device, AGENT_START_PROCESS_PROPERTY, "No focuser is selected");
				}
			} else {
				AGENT_IMAGER_START_PREVIEW_1_ITEM->sw.value = AGENT_IMAGER_START_PREVIEW_ITEM->sw.value = AGENT_IMAGER_START_EXPOSURE_ITEM->sw.value = AGENT_IMAGER_START_STREAMING_ITEM->sw.value = AGENT_IMAGER_START_FOCUSING_ITEM->sw.value = AGENT_IMAGER_START_SEQUENCE_ITEM->sw.value = AGENT_IMAGER_CLEAR_SELECTION_ITEM->sw.value = false;
				AGENT_START_PROCESS_PROPERTY->state = INDIGO_ALERT_STATE;
				indigo_update_property(device, AGENT_START_PROCESS_PROPERTY, "No CCD is selected");
			}
			AGENT_PAUSE_PROCESS_ITEM->sw.value = AGENT_PAUSE_PROCESS_WAIT_ITEM->sw.value = AGENT_PAUSE_PROCESS_AFTER_TRANSIT_ITEM->sw.value = false;
			AGENT_PAUSE_PROCESS_PROPERTY->state = INDIGO_OK_STATE;
			indigo_update_property(device, AGENT_PAUSE_PROCESS_PROPERTY, NULL);
			AGENT_ABORT_PROCESS_ITEM->sw.value = false;
			AGENT_ABORT_PROCESS_PROPERTY->state = INDIGO_OK_STATE;
			indigo_update_property(device, AGENT_ABORT_PROCESS_PROPERTY, NULL);
			indigo_update_property(device, AGENT_START_PROCESS_PROPERTY, NULL);
		}
		return INDIGO_OK;
	} else if (indigo_property_match(AGENT_PAUSE_PROCESS_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- AGENT_PAUSE_PROCESS
		if (AGENT_START_PROCESS_PROPERTY->state == INDIGO_BUSY_STATE) {
			indigo_property_copy_values(AGENT_PAUSE_PROCESS_PROPERTY, property, false);
			if (AGENT_PAUSE_PROCESS_PROPERTY->state == INDIGO_BUSY_STATE) {
				AGENT_PAUSE_PROCESS_ITEM->sw.value = AGENT_PAUSE_PROCESS_WAIT_ITEM->sw.value = AGENT_PAUSE_PROCESS_AFTER_TRANSIT_ITEM->sw.value = false;
				AGENT_PAUSE_PROCESS_PROPERTY->state = INDIGO_OK_STATE;
			} else {
				if (AGENT_PAUSE_PROCESS_AFTER_TRANSIT_ITEM->sw.value) {
					AGENT_PAUSE_PROCESS_AFTER_TRANSIT_ITEM->sw.value = false; // can be only cleared when set by agent
				} else {
					AGENT_PAUSE_PROCESS_PROPERTY->state = INDIGO_BUSY_STATE;
					if (AGENT_PAUSE_PROCESS_ITEM->sw.value) {
						abort_process(device);
					}
				}
			}
		} else {
			AGENT_PAUSE_PROCESS_PROPERTY->state = INDIGO_ALERT_STATE;
		}
		indigo_update_property(device, AGENT_PAUSE_PROCESS_PROPERTY, NULL);
		return INDIGO_OK;
	} else if (indigo_property_match(AGENT_ABORT_PROCESS_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- AGENT_ABORT_PROCESS
		if (AGENT_START_PROCESS_PROPERTY->state == INDIGO_BUSY_STATE || AGENT_IMAGER_STARS_PROPERTY->state == INDIGO_BUSY_STATE) {
			indigo_property_copy_values(AGENT_ABORT_PROCESS_PROPERTY, property, false);
			if (AGENT_PAUSE_PROCESS_PROPERTY->state == INDIGO_BUSY_STATE) {
				AGENT_PAUSE_PROCESS_ITEM->sw.value = AGENT_PAUSE_PROCESS_WAIT_ITEM->sw.value =  false;
				AGENT_PAUSE_PROCESS_PROPERTY->state = INDIGO_ALERT_STATE;
				indigo_update_property(device, AGENT_PAUSE_PROCESS_PROPERTY, NULL);
			}
			AGENT_ABORT_PROCESS_PROPERTY->state = INDIGO_BUSY_STATE;
			abort_process(device);
		}
		AGENT_ABORT_PROCESS_ITEM->sw.value = false;
		indigo_update_property(device, AGENT_ABORT_PROCESS_PROPERTY, NULL);
		return INDIGO_OK;
	} else if (indigo_property_match(AGENT_PROCESS_FEATURES_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- AGENT_PROCESS_FEATURES
		indigo_property_copy_values(AGENT_PROCESS_FEATURES_PROPERTY, property, false);
		AGENT_PROCESS_FEATURES_PROPERTY->state = INDIGO_OK_STATE;
		save_config(device);
		indigo_update_property(device, AGENT_PROCESS_FEATURES_PROPERTY, NULL);
		return INDIGO_OK;
		// -------------------------------------------------------------------------------- AGENT_IMAGER_DOWNLOAD_FILE
	} else if (indigo_property_match(AGENT_IMAGER_DOWNLOAD_FILE_PROPERTY, property)) {
		pthread_mutex_lock(&DEVICE_PRIVATE_DATA->mutex);
		indigo_property_copy_values(AGENT_IMAGER_DOWNLOAD_FILE_PROPERTY, property, false);
		AGENT_IMAGER_DOWNLOAD_FILE_PROPERTY->state = INDIGO_BUSY_STATE;
		indigo_update_property(device, AGENT_IMAGER_DOWNLOAD_FILE_PROPERTY, NULL);
		AGENT_IMAGER_DOWNLOAD_FILE_PROPERTY->state = INDIGO_ALERT_STATE;
		for (int i = 1; i < AGENT_IMAGER_DOWNLOAD_FILES_PROPERTY->count; i++) {
			indigo_item *item = AGENT_IMAGER_DOWNLOAD_FILES_PROPERTY->items + i;
			if (!strcmp(item->name, AGENT_IMAGER_DOWNLOAD_FILE_ITEM->text.value)) {
				char file_name[PATH_MAX];
				struct stat file_stat;
				snprintf(file_name, sizeof(file_name), "%s%s", DEVICE_PRIVATE_DATA->current_folder, AGENT_IMAGER_DOWNLOAD_FILE_ITEM->text.value);
				if (stat(file_name, &file_stat) < 0 || file_stat.st_size == 0) {
					AGENT_IMAGER_DOWNLOAD_IMAGE_PROPERTY->state = INDIGO_ALERT_STATE;
					indigo_update_property(device, AGENT_IMAGER_DOWNLOAD_IMAGE_PROPERTY, NULL);
					AGENT_IMAGER_DOWNLOAD_FILE_PROPERTY->state = INDIGO_ALERT_STATE;
					break;
				}
				/* allocate 5% more mem to accomodate size fluctuation of the compressed images
				   and reallocate smaller buffer only if the next image is more than 50% smaller
				*/
				size_t malloc_size = (size_t)(1.05 * file_stat.st_size);
				if (DEVICE_PRIVATE_DATA->image_buffer && (DEVICE_PRIVATE_DATA->image_buffer_size < file_stat.st_size || DEVICE_PRIVATE_DATA->image_buffer_size > 2 * file_stat.st_size)) {
					DEVICE_PRIVATE_DATA->image_buffer = indigo_safe_realloc(DEVICE_PRIVATE_DATA->image_buffer, malloc_size);
					DEVICE_PRIVATE_DATA->image_buffer_size = malloc_size;
				} else if (DEVICE_PRIVATE_DATA->image_buffer == NULL){
					DEVICE_PRIVATE_DATA->image_buffer = indigo_safe_malloc(malloc_size);
					DEVICE_PRIVATE_DATA->image_buffer_size = malloc_size;
				}
				indigo_uni_handle *handle = indigo_uni_open_file(file_name);
				if (handle == NULL) {
					break;
				}
				long result = indigo_uni_read(handle, AGENT_IMAGER_DOWNLOAD_IMAGE_ITEM->blob.value = DEVICE_PRIVATE_DATA->image_buffer, AGENT_IMAGER_DOWNLOAD_IMAGE_ITEM->blob.size = file_stat.st_size);
				indigo_uni_close(&handle);
				if (result == -1) {
					AGENT_IMAGER_DOWNLOAD_IMAGE_PROPERTY->state = INDIGO_ALERT_STATE;
					indigo_update_property(device, AGENT_IMAGER_DOWNLOAD_IMAGE_PROPERTY, NULL);
					break;
				}
				*AGENT_IMAGER_DOWNLOAD_IMAGE_ITEM->blob.url = 0;
				*AGENT_IMAGER_DOWNLOAD_IMAGE_ITEM->blob.format = 0;
				char *file_type = strrchr(file_name, '.');
				if (file_type)
					strcpy(AGENT_IMAGER_DOWNLOAD_IMAGE_ITEM->blob.format, file_type);
				AGENT_IMAGER_DOWNLOAD_IMAGE_PROPERTY->state = INDIGO_OK_STATE;
				indigo_update_property(device, AGENT_IMAGER_DOWNLOAD_IMAGE_PROPERTY, NULL);
				AGENT_IMAGER_DOWNLOAD_FILE_PROPERTY->state = INDIGO_OK_STATE;
				break;
			}
		}
		indigo_update_property(device, AGENT_IMAGER_DOWNLOAD_FILE_PROPERTY, NULL);
		pthread_mutex_unlock(&DEVICE_PRIVATE_DATA->mutex);
		return INDIGO_OK;
		// -------------------------------------------------------------------------------- AGENT_IMAGER_DELETE_FILE
	} else if (indigo_property_match(AGENT_IMAGER_DELETE_FILE_PROPERTY, property)) {
		pthread_mutex_lock(&DEVICE_PRIVATE_DATA->mutex);
		indigo_property_copy_values(AGENT_IMAGER_DELETE_FILE_PROPERTY, property, false);
		AGENT_IMAGER_DELETE_FILE_PROPERTY->state = INDIGO_BUSY_STATE;
		indigo_update_property(device, AGENT_IMAGER_DELETE_FILE_PROPERTY, NULL);
		AGENT_IMAGER_DELETE_FILE_PROPERTY->state = INDIGO_ALERT_STATE;
		for (int i = 1; i < AGENT_IMAGER_DOWNLOAD_FILES_PROPERTY->count; i++) {
			indigo_item *item = AGENT_IMAGER_DOWNLOAD_FILES_PROPERTY->items + i;
			if (!strcmp(item->name, AGENT_IMAGER_DELETE_FILE_ITEM->text.value)) {
				char file_name[PATH_MAX];
				struct stat file_stat;
				snprintf(file_name, sizeof(file_name), "%s%s", DEVICE_PRIVATE_DATA->current_folder, AGENT_IMAGER_DELETE_FILE_ITEM->text.value);
				if (stat(file_name, &file_stat) < 0) {
					break;
				}
				indigo_update_property(device, AGENT_IMAGER_DELETE_FILE_PROPERTY, NULL);
				if (!indigo_uni_remove(file_name)) {
					break;
				}
				AGENT_IMAGER_DELETE_FILE_PROPERTY->state = INDIGO_OK_STATE;
				break;
			}
		}
		setup_download(device);
		indigo_update_property(device, AGENT_IMAGER_DELETE_FILE_PROPERTY, NULL);
		pthread_mutex_unlock(&DEVICE_PRIVATE_DATA->mutex);
		return INDIGO_OK;
	} else if (indigo_property_match(AGENT_IMAGER_DOWNLOAD_FILES_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- AGENT_IMAGER_DOWNLOAD_FILES
		indigo_property_copy_values(AGENT_IMAGER_DOWNLOAD_FILES_PROPERTY, property, false);
		if (AGENT_IMAGER_DOWNLOAD_FILES_REFRESH_ITEM->sw.value) {
			AGENT_IMAGER_DOWNLOAD_FILES_REFRESH_ITEM->sw.value = false;
			pthread_mutex_lock(&DEVICE_PRIVATE_DATA->mutex);
			setup_download(device);
			pthread_mutex_unlock(&DEVICE_PRIVATE_DATA->mutex);
		} else {
			pthread_mutex_lock(&DEVICE_PRIVATE_DATA->mutex);
			indigo_update_property(device, AGENT_IMAGER_DOWNLOAD_FILES_PROPERTY, NULL);
			for (int i = 1; i < AGENT_IMAGER_DOWNLOAD_FILES_PROPERTY->count; i++) {
				indigo_item *item = AGENT_IMAGER_DOWNLOAD_FILES_PROPERTY->items + i;
				if (item->sw.value) {
					item->sw.value = false;
					AGENT_IMAGER_DOWNLOAD_FILES_PROPERTY->state = INDIGO_OK_STATE;
					indigo_update_property(device, AGENT_IMAGER_DOWNLOAD_FILES_PROPERTY, NULL);
					pthread_mutex_unlock(&DEVICE_PRIVATE_DATA->mutex);
					indigo_change_text_property_1(client, device->name, AGENT_IMAGER_DOWNLOAD_FILE_PROPERTY_NAME, AGENT_IMAGER_DOWNLOAD_FILE_ITEM_NAME, item->name);
					return INDIGO_OK;
				}
			}
			pthread_mutex_unlock(&DEVICE_PRIVATE_DATA->mutex);
		}
		return INDIGO_OK;
	// -------------------------------------------------------------------------------- AGENT_WHEEL_FILTER
	} else if (indigo_property_match(AGENT_WHEEL_FILTER_PROPERTY, property)) {
		indigo_property_copy_values(AGENT_WHEEL_FILTER_PROPERTY, property, false);
		for (int i = 0; i < AGENT_WHEEL_FILTER_PROPERTY->count; i++) {
			if (AGENT_WHEEL_FILTER_PROPERTY->items[i].sw.value) {
				indigo_change_number_property_1(FILTER_DEVICE_CONTEXT->client, device->name, WHEEL_SLOT_PROPERTY_NAME, WHEEL_SLOT_ITEM_NAME, i + 1);
				break;
			}
		}
		AGENT_WHEEL_FILTER_PROPERTY->state = INDIGO_BUSY_STATE;
		indigo_update_property(device, AGENT_WHEEL_FILTER_PROPERTY,NULL);
		return INDIGO_OK;
		// -------------------------------------------------------------------------------- AGENT_FOCUSER_CONTROL
	} else if (indigo_property_match(AGENT_FOCUSER_CONTROL_PROPERTY, property)) {
		indigo_property_copy_values(AGENT_FOCUSER_CONTROL_PROPERTY, property, false);
		if (AGENT_FOCUSER_FOCUS_IN_ITEM->sw.value) {
			indigo_change_switch_property_1(FILTER_DEVICE_CONTEXT->client, device->name, FOCUSER_DIRECTION_PROPERTY_NAME, FOCUSER_DIRECTION_MOVE_INWARD_ITEM_NAME, true);
			indigo_change_number_property_1(FILTER_DEVICE_CONTEXT->client, device->name, FOCUSER_STEPS_PROPERTY_NAME, FOCUSER_STEPS_ITEM_NAME, 65535);
			AGENT_FOCUSER_CONTROL_PROPERTY->state = INDIGO_BUSY_STATE;
		} else if (AGENT_FOCUSER_FOCUS_OUT_ITEM->sw.value) {
			indigo_change_switch_property_1(FILTER_DEVICE_CONTEXT->client, device->name, FOCUSER_DIRECTION_PROPERTY_NAME, FOCUSER_DIRECTION_MOVE_OUTWARD_ITEM_NAME, true);
			indigo_change_number_property_1(FILTER_DEVICE_CONTEXT->client, device->name, FOCUSER_STEPS_PROPERTY_NAME, FOCUSER_STEPS_ITEM_NAME, 65535);
			AGENT_FOCUSER_CONTROL_PROPERTY->state = INDIGO_BUSY_STATE;
		} else {
			indigo_change_switch_property_1(FILTER_DEVICE_CONTEXT->client, device->name, FOCUSER_ABORT_MOTION_PROPERTY_NAME, FOCUSER_ABORT_MOTION_ITEM_NAME, true);
			AGENT_FOCUSER_CONTROL_PROPERTY->state = INDIGO_OK_STATE;
		}
		indigo_update_property(device, AGENT_FOCUSER_CONTROL_PROPERTY,NULL);
		return INDIGO_OK;
	// -------------------------------------------------------------------------------- AGENT_IMAGER_SEQUENCE_SIZE
	} else if (indigo_property_match(AGENT_IMAGER_SEQUENCE_SIZE_PROPERTY, property)) {
		indigo_property_copy_values(AGENT_IMAGER_SEQUENCE_SIZE_PROPERTY, property, false);
		AGENT_IMAGER_SEQUENCE_SIZE_PROPERTY->state = INDIGO_OK_STATE;
		int old_size = AGENT_IMAGER_SEQUENCE_PROPERTY->count;
		int new_size = (int)AGENT_IMAGER_SEQUENCE_SIZE_ITEM->number.value + 1;
		if (old_size != new_size) {
			indigo_delete_property(device, AGENT_IMAGER_SEQUENCE_PROPERTY, NULL);
			if (old_size < new_size) {
				for (int i = old_size; i < new_size; i++) {
					indigo_set_text_item_value(AGENT_IMAGER_SEQUENCE_PROPERTY->items + i, "");
				}
			}
			AGENT_IMAGER_SEQUENCE_PROPERTY->count = new_size;
			indigo_define_property(device, AGENT_IMAGER_SEQUENCE_PROPERTY, NULL);
			save_config(device);
		}
		indigo_update_property(device, AGENT_IMAGER_SEQUENCE_SIZE_PROPERTY, NULL);
		return INDIGO_OK;
	// -------------------------------------------------------------------------------- AGENT_IMAGER_SEQUENCE
	} else if (indigo_property_match(AGENT_IMAGER_SEQUENCE_PROPERTY, property)) {
		indigo_property_copy_values(AGENT_IMAGER_SEQUENCE_PROPERTY, property, false);
		AGENT_IMAGER_SEQUENCE_PROPERTY->state = INDIGO_OK_STATE;
		save_config(device);
		indigo_update_property(device, AGENT_IMAGER_SEQUENCE_PROPERTY, NULL);
		return INDIGO_OK;
		// -------------------------------------------------------------------------------- AGENT_IMAGER_BREAKPOINT
	} else if (indigo_property_match(AGENT_IMAGER_BREAKPOINT_PROPERTY, property)) {
		indigo_property_copy_values(AGENT_IMAGER_BREAKPOINT_PROPERTY, property, false);
		if (AGENT_IMAGER_RESUME_CONDITION_BARRIER_ITEM->sw.value) {
			// On related imager agents duplicate AGENT_IMAGER_BREAKPOINT_PROPERTY
			indigo_property *related_agents_property = FILTER_DEVICE_CONTEXT->filter_related_agent_list_property;
			indigo_property *clone = indigo_init_switch_property(NULL, AGENT_IMAGER_BREAKPOINT_PROPERTY->device, AGENT_IMAGER_BREAKPOINT_PROPERTY->name, NULL, NULL, 0, 0, 0, AGENT_IMAGER_BREAKPOINT_PROPERTY->count);
			memcpy(clone, AGENT_IMAGER_BREAKPOINT_PROPERTY, sizeof(indigo_property) + AGENT_IMAGER_BREAKPOINT_PROPERTY->count * sizeof(indigo_item));
			for (int i = 0; i < related_agents_property->count; i++) {
				indigo_item *item = related_agents_property->items + i;
				if (item->sw.value && !strncmp(item->name, "Imager Agent", 12)) {
					strcpy(clone->device, item->name);
					indigo_change_property(client, clone);
				}
			}
			indigo_release_property(clone);
		}
		AGENT_IMAGER_BREAKPOINT_PROPERTY->state = INDIGO_OK_STATE;
		indigo_update_property(device, AGENT_IMAGER_BREAKPOINT_PROPERTY, NULL);
		return INDIGO_OK;
		// -------------------------------------------------------------------------------- AGENT_IMAGER_RESUME_CONDITION
	} else if (indigo_property_match(AGENT_IMAGER_RESUME_CONDITION_PROPERTY, property)) {
		indigo_property_copy_values(AGENT_IMAGER_RESUME_CONDITION_PROPERTY, property, false);
		if (AGENT_IMAGER_RESUME_CONDITION_BARRIER_ITEM->sw.value) {
			// On related imager agents reset AGENT_IMAGER_RESUME_CONDITION_PROPERTY to AGENT_IMAGER_RESUME_CONDITION_TRIGGER_ITEM
			indigo_property *related_agents_property = FILTER_DEVICE_CONTEXT->filter_related_agent_list_property;
			for (int i = 0; i < related_agents_property->count; i++) {
				indigo_item *item = related_agents_property->items + i;
				if (item->sw.value && !strncmp(item->name, "Imager Agent", 12)) {
					indigo_change_switch_property_1(client, item->name, AGENT_IMAGER_RESUME_CONDITION_PROPERTY_NAME, AGENT_IMAGER_RESUME_CONDITION_TRIGGER_ITEM_NAME, true);
				}
			}
		}
		AGENT_IMAGER_RESUME_CONDITION_PROPERTY->state = INDIGO_OK_STATE;
		indigo_update_property(device, AGENT_IMAGER_RESUME_CONDITION_PROPERTY, NULL);
		return INDIGO_OK;
		// -------------------------------------------------------------------------------- ADDITIONAL_INSTANCES
	} else if (indigo_property_match(ADDITIONAL_INSTANCES_PROPERTY, property)) {
		if (indigo_filter_change_property(device, client, property) == INDIGO_OK) {
			save_config(device);
		}
		return INDIGO_OK;
	} else if (!strcmp(property->device, device->name)) {
		if (!strcmp(property->name, FOCUSER_BACKLASH_PROPERTY_NAME)) {
			AGENT_IMAGER_FOCUS_BACKLASH_ITEM->number.value = AGENT_IMAGER_FOCUS_BACKLASH_ITEM->number.target = property->items[0].number.value;
			indigo_update_property(device, AGENT_IMAGER_FOCUS_PROPERTY, NULL);
		} else if (!strcmp(property->name, CCD_SET_FITS_HEADER_PROPERTY_NAME)) {
			char *name = NULL;
			char *value = NULL;
			for (int i = 0; i < property->count; i++) {
				indigo_item *item = property->items + i;
				if (!strcmp(item->name, CCD_SET_FITS_HEADER_KEYWORD_ITEM_NAME)) {
					name = item->text.value;
				} else if (!strcmp(item->name, CCD_SET_FITS_HEADER_VALUE_ITEM_NAME)) {
					value = item->text.value;
				}
			}
			if (name != NULL && value != NULL) {
				int d, m, s;
				if (sscanf(value, "'%d %d %d'", &d, &m, &s) == 3) {
					double value = d + m / 60.0 + s / 3600.0;
					if (!strcmp(name, "OBJCTRA")) {
						DEVICE_PRIVATE_DATA->ra = value;
					} else if (!strcmp(name, "OBJCTDEC")) {
						DEVICE_PRIVATE_DATA->dec = value;
					} else if (!strcmp(name, "SITELAT")) {
						DEVICE_PRIVATE_DATA->latitude = value;
					} else if (!strcmp(name, "SITELONG")) {
						DEVICE_PRIVATE_DATA->longitude = value;
					}
					time_t utc = time(NULL);
					double lst = indigo_lst(&utc, DEVICE_PRIVATE_DATA->longitude);
					double ra = DEVICE_PRIVATE_DATA->ra;
					double dec = DEVICE_PRIVATE_DATA->dec;
					double transit;
					indigo_j2k_to_jnow(&ra, &dec);
					indigo_raise_set(UT2JD(utc), DEVICE_PRIVATE_DATA->latitude, DEVICE_PRIVATE_DATA->longitude, ra, dec, NULL, &transit, NULL);
					DEVICE_PRIVATE_DATA->time_to_transit = indigo_time_to_transit(ra, lst);
				}
			}
		}
	}
	return indigo_filter_change_property(device, client, property);
}

static indigo_result agent_device_detach(indigo_device *device) {
	assert(device != NULL);
	save_config(device);
	indigo_release_property(AGENT_IMAGER_BATCH_PROPERTY);
	indigo_release_property(AGENT_IMAGER_FOCUS_PROPERTY);
	indigo_release_property(AGENT_IMAGER_FOCUS_FAILURE_PROPERTY);
	indigo_release_property(AGENT_IMAGER_FOCUS_ESTIMATOR_PROPERTY);
	indigo_release_property(AGENT_IMAGER_DOWNLOAD_IMAGE_PROPERTY);
	indigo_release_property(AGENT_IMAGER_DOWNLOAD_FILE_PROPERTY);
	indigo_release_property(AGENT_IMAGER_DOWNLOAD_FILES_PROPERTY);
	indigo_release_property(AGENT_IMAGER_DELETE_FILE_PROPERTY);
	indigo_release_property(AGENT_IMAGER_STARS_PROPERTY);
	indigo_release_property(AGENT_IMAGER_SELECTION_PROPERTY);
	indigo_release_property(AGENT_IMAGER_STATS_PROPERTY);
	indigo_release_property(AGENT_IMAGER_SPIKES_PROPERTY);
	indigo_release_property(AGENT_IMAGER_CAPTURE_PROPERTY);
	indigo_release_property(AGENT_START_PROCESS_PROPERTY);
	indigo_release_property(AGENT_PAUSE_PROCESS_PROPERTY);
	indigo_release_property(AGENT_ABORT_PROCESS_PROPERTY);
	indigo_release_property(AGENT_PROCESS_FEATURES_PROPERTY);
	indigo_release_property(AGENT_IMAGER_SEQUENCE_PROPERTY);
	indigo_release_property(AGENT_IMAGER_SEQUENCE_SIZE_PROPERTY);
	indigo_release_property(AGENT_IMAGER_BREAKPOINT_PROPERTY);
	indigo_release_property(AGENT_IMAGER_RESUME_CONDITION_PROPERTY);
	indigo_release_property(AGENT_IMAGER_BARRIER_STATE_PROPERTY);
	indigo_release_property(AGENT_WHEEL_FILTER_PROPERTY);
	indigo_release_property(AGENT_FOCUSER_CONTROL_PROPERTY);
	pthread_mutex_destroy(&DEVICE_PRIVATE_DATA->mutex);
	pthread_mutex_destroy(&DEVICE_PRIVATE_DATA->last_image_mutex);
	indigo_safe_free(DEVICE_PRIVATE_DATA->image_buffer);
	DEVICE_PRIVATE_DATA->image_buffer_size = 0;
	indigo_safe_free(DEVICE_PRIVATE_DATA->last_image);
	DEVICE_PRIVATE_DATA->last_image_size = 0;
	return indigo_filter_device_detach(device);
}

// -------------------------------------------------------------------------------- INDIGO agent client implementation

static void snoop_changes(indigo_client *client, indigo_device *device, indigo_property *property) {
	if (!strcmp(property->name, FILTER_CCD_LIST_PROPERTY_NAME)) { // Snoop CCD ...
		if (!INDIGO_FILTER_CCD_SELECTED) {
			DEVICE_PRIVATE_DATA->steps_state = INDIGO_IDLE_STATE;
		}
	} else if (!strcmp(property->name, CCD_EXPOSURE_PROPERTY_NAME)) {
		if (!DEVICE_PRIVATE_DATA->has_camera) {
			DEVICE_PRIVATE_DATA->has_camera = true;
			clear_selection(device);
		}
		if (!CLIENT_PRIVATE_DATA->use_aux_1) {
			for (int i = 0; i < property->count; i++) {
				indigo_item *item = property->items + i;
				if (!strcmp(item->name, CCD_EXPOSURE_ITEM_NAME)) {
					DEVICE_PRIVATE_DATA->remaining_exposure_time = item->number.value;
					break;
				}
			}
		}
		DEVICE_PRIVATE_DATA->exposure_state = property->state;
	} else if (!strcmp(property->name, CCD_STREAMING_PROPERTY_NAME)) {
		for (int i = 0; i < property->count; i++) {
			indigo_item *item = property->items + i;
			if (!strcmp(item->name, CCD_STREAMING_COUNT_ITEM_NAME)) {
				DEVICE_PRIVATE_DATA->remaining_streaming_count = item->number.value;
				break;
			}
		}
		DEVICE_PRIVATE_DATA->streaming_state = property->state;
	} else if (!strcmp(property->name, CCD_FRAME_TYPE_PROPERTY_NAME)) {
		DEVICE_PRIVATE_DATA->light_frame = true;
		for (int i = 0; i < property->count; i++) {
			indigo_item *item = property->items + i;
			if (item->sw.value) {
				DEVICE_PRIVATE_DATA->light_frame = strcmp(item->name, CCD_FRAME_TYPE_LIGHT_ITEM_NAME) == 0;
				break;
			}
		}
	} else if (!strcmp(property->name, CCD_FRAME_PROPERTY_NAME)) {
		if (!DEVICE_PRIVATE_DATA->autosubframing) {
			bool reset_selection = false;
			for (int i = 0; i < property->count; i++) {
				indigo_item *item = property->items + i;
				if (strcmp(item->name, CCD_FRAME_LEFT_ITEM_NAME) == 0) {
					if (DEVICE_PRIVATE_DATA->frame[0] != item->number.value) {
						DEVICE_PRIVATE_DATA->frame[0] = item->number.value;
						reset_selection = true;
					}
				} else if (strcmp(item->name, CCD_FRAME_TOP_ITEM_NAME) == 0) {
					if (DEVICE_PRIVATE_DATA->frame[1] != item->number.value) {
						DEVICE_PRIVATE_DATA->frame[1] = item->number.value;
						reset_selection = true;
					}
				} else if (strcmp(item->name, CCD_FRAME_WIDTH_ITEM_NAME) == 0) {
					if (DEVICE_PRIVATE_DATA->frame[2] != item->number.value) {
						DEVICE_PRIVATE_DATA->frame[2] = item->number.value;
						reset_selection = true;
					}
				} else if (strcmp(item->name, CCD_FRAME_HEIGHT_ITEM_NAME) == 0) {
					if (DEVICE_PRIVATE_DATA->frame[3] != item->number.value) {
						DEVICE_PRIVATE_DATA->frame[3] = item->number.value;
						reset_selection = true;
					}
				}
			}
			if (reset_selection) {
				DEVICE_PRIVATE_DATA->last_width = (int)(DEVICE_PRIVATE_DATA->frame[2] / DEVICE_PRIVATE_DATA->bin_x);
				DEVICE_PRIVATE_DATA->last_height = (int)(DEVICE_PRIVATE_DATA->frame[3] / DEVICE_PRIVATE_DATA->bin_y);
				AGENT_IMAGER_SELECTION_INCLUDE_LEFT_ITEM->number.value = AGENT_IMAGER_SELECTION_INCLUDE_TOP_ITEM->number.value = AGENT_IMAGER_SELECTION_INCLUDE_WIDTH_ITEM->number.value = AGENT_IMAGER_SELECTION_INCLUDE_HEIGHT_ITEM->number.value = AGENT_IMAGER_SELECTION_EXCLUDE_LEFT_ITEM->number.value = AGENT_IMAGER_SELECTION_EXCLUDE_TOP_ITEM->number.value = AGENT_IMAGER_SELECTION_EXCLUDE_WIDTH_ITEM->number.value = AGENT_IMAGER_SELECTION_EXCLUDE_HEIGHT_ITEM->number.value = 0;
				if (validate_include_region(device, false)) {
					indigo_update_property(device, AGENT_IMAGER_SELECTION_PROPERTY, NULL);
				}
			}
		}
	} else if (!strcmp(property->name, CCD_LOCAL_MODE_PROPERTY_NAME)) {
		*DEVICE_PRIVATE_DATA->current_folder = 0;
		for (int i = 0; i < property->count; i++) {
			indigo_item *item = property->items + i;
			if (strcmp(item->name, CCD_LOCAL_MODE_DIR_ITEM_NAME) == 0) {
				indigo_copy_value(CLIENT_PRIVATE_DATA->current_folder, item->text.value);
				break;
			}
		}
		setup_download(FILTER_CLIENT_CONTEXT->device);
	} else if (!strcmp(property->name, CCD_IMAGE_FILE_PROPERTY_NAME)) {
		 setup_download(FILTER_CLIENT_CONTEXT->device);
	} else if (!strcmp(property->name, FILTER_AUX_1_LIST_PROPERTY_NAME)) { // Snoop AUX_1 ...
		if (!INDIGO_FILTER_AUX_1_SELECTED) {
			DEVICE_PRIVATE_DATA->use_aux_1 = false;
		}
	} else if (!strcmp(property->name, "AUX_1_" CCD_EXPOSURE_PROPERTY_NAME)) {
		DEVICE_PRIVATE_DATA->use_aux_1 = true;
		for (int i = 0; i < property->count; i++) {
			indigo_item *item = property->items + i;
			if (!strcmp(item->name, CCD_EXPOSURE_ITEM_NAME)) {
				DEVICE_PRIVATE_DATA->remaining_exposure_time = item->number.value;
				break;
			}
		}
	} else if (!strcmp(property->name, FILTER_FOCUSER_LIST_PROPERTY_NAME)) { // Snoop focuser ...
		if (!INDIGO_FILTER_FOCUSER_SELECTED) {
			DEVICE_PRIVATE_DATA->steps_state = INDIGO_IDLE_STATE;
			DEVICE_PRIVATE_DATA->focuser_position = NAN;
			DEVICE_PRIVATE_DATA->focuser_temperature = NAN;
			DEVICE_PRIVATE_DATA->focuser_has_backlash = false;
		}
	} else if (!strcmp(property->name, FOCUSER_STEPS_PROPERTY_NAME)) {
		DEVICE_PRIVATE_DATA->steps_state = property->state;
	} else if (!strcmp(property->name, FOCUSER_POSITION_PROPERTY_NAME)) {
		DEVICE_PRIVATE_DATA->focuser_position = property->items[0].number.value;
	} else if (!strcmp(property->name, FOCUSER_TEMPERATURE_PROPERTY_NAME)) {
		DEVICE_PRIVATE_DATA->focuser_temperature = property->items[0].number.value;
	} else if (!strcmp(property->name, FOCUSER_BACKLASH_PROPERTY_NAME)) {
		indigo_device *device = FILTER_CLIENT_CONTEXT->device;
		DEVICE_PRIVATE_DATA->focuser_has_backlash = true;
		AGENT_IMAGER_FOCUS_BACKLASH_ITEM->number.value = AGENT_IMAGER_FOCUS_BACKLASH_ITEM->number.target = property->items[0].number.value;
		indigo_update_property(device, AGENT_IMAGER_FOCUS_PROPERTY, NULL);
	} else if (!strcmp(property->name, FILTER_WHEEL_LIST_PROPERTY_NAME)) { // Snoop wheel ...
		if (!INDIGO_FILTER_WHEEL_SELECTED) {
			indigo_delete_property(FILTER_CLIENT_CONTEXT->device, AGENT_WHEEL_FILTER_PROPERTY, NULL);
			AGENT_WHEEL_FILTER_PROPERTY->count = 0;
			indigo_define_property(FILTER_CLIENT_CONTEXT->device, AGENT_WHEEL_FILTER_PROPERTY, NULL);
		}
	} else if (!strcmp(property->name, WHEEL_SLOT_NAME_PROPERTY_NAME)) {
		indigo_delete_property(FILTER_CLIENT_CONTEXT->device, AGENT_WHEEL_FILTER_PROPERTY, NULL);
		AGENT_WHEEL_FILTER_PROPERTY->count = property->count;
		for (int i = 0; i < property->count; i++)
			strcpy(AGENT_WHEEL_FILTER_PROPERTY->items[i].label, property->items[i].text.value);
		indigo_define_property(FILTER_CLIENT_CONTEXT->device, AGENT_WHEEL_FILTER_PROPERTY, NULL);
	} else if (!strcmp(property->name, WHEEL_SLOT_PROPERTY_NAME)) {
		indigo_device *device = FILTER_CLIENT_CONTEXT->device;
		int value = (int)property->items->number.value;
		if (value)
			indigo_set_switch(AGENT_WHEEL_FILTER_PROPERTY, AGENT_WHEEL_FILTER_PROPERTY->items + value - 1, true);
		else
			indigo_set_switch(AGENT_WHEEL_FILTER_PROPERTY, AGENT_WHEEL_FILTER_PROPERTY->items, false);
		AGENT_WHEEL_FILTER_PROPERTY->state = property->state;
		indigo_update_property(FILTER_CLIENT_CONTEXT->device, AGENT_WHEEL_FILTER_PROPERTY, NULL);
	} else if (!strcmp(property->name, FILTER_RELATED_AGENT_LIST_PROPERTY_NAME)) { // Snoop related agent list
		if (property->state == INDIGO_OK_STATE) {
			 AGENT_IMAGER_BARRIER_STATE_PROPERTY->count = 0;
			 indigo_property *clone = indigo_init_switch_property(NULL, AGENT_IMAGER_BREAKPOINT_PROPERTY->device, AGENT_IMAGER_BREAKPOINT_PROPERTY->name, NULL, NULL, 0, 0, 0, AGENT_IMAGER_BREAKPOINT_PROPERTY->count);
			 memcpy(clone, AGENT_IMAGER_BREAKPOINT_PROPERTY, sizeof(indigo_property) + AGENT_IMAGER_BREAKPOINT_PROPERTY->count * sizeof(indigo_item));
			 for (int i = 0; i < property->count; i++) {
				 indigo_item *item = property->items + i;
				 if (item->sw.value && !strncmp(item->name, "Imager Agent", 12)) {
					 AGENT_IMAGER_BARRIER_STATE_PROPERTY = indigo_resize_property(AGENT_IMAGER_BARRIER_STATE_PROPERTY, AGENT_IMAGER_BARRIER_STATE_PROPERTY->count + 1);
					 indigo_init_light_item(AGENT_IMAGER_BARRIER_STATE_PROPERTY->items + AGENT_IMAGER_BARRIER_STATE_PROPERTY->count - 1, item->name, item->label, INDIGO_IDLE_STATE);
					 if (AGENT_IMAGER_RESUME_CONDITION_BARRIER_ITEM->sw.value) {
						 // Duplicate AGENT_IMAGER_BREAKPOINT_PROPERTY and reset AGENT_IMAGER_RESUME_CONDITION_PROPERTY to AGENT_IMAGER_RESUME_CONDITION_TRIGGER_ITEM
						 strcpy(clone->device, item->name);
						 indigo_change_property(client, clone);
						 indigo_change_switch_property_1(client, item->name, AGENT_IMAGER_RESUME_CONDITION_PROPERTY_NAME, AGENT_IMAGER_RESUME_CONDITION_TRIGGER_ITEM_NAME, true);
					 }
				 }
			 }
			 indigo_release_property(clone);
			 indigo_delete_property(device, AGENT_IMAGER_BARRIER_STATE_PROPERTY, NULL);
			 indigo_define_property(device, AGENT_IMAGER_BARRIER_STATE_PROPERTY, NULL);
			 indigo_property property = { 0 };
			 strcpy(property.name, AGENT_PAUSE_PROCESS_PROPERTY_NAME);
			 for (int i = 0; i < AGENT_IMAGER_BARRIER_STATE_PROPERTY->count; i++) {
				 indigo_item *item = AGENT_IMAGER_BARRIER_STATE_PROPERTY->items + i;
				 strcpy(property.device, item->name);
				 indigo_enumerate_properties(client, &property);
			 }
		 }
	 }
}

static void snoop_barrier_state(indigo_client *client, indigo_property *property) {
	if (!strcmp(property->name, AGENT_PAUSE_PROCESS_PROPERTY_NAME)) {
		indigo_device *device = FILTER_CLIENT_CONTEXT->device;
		CLIENT_PRIVATE_DATA->barrier_resume = true;
		for (int i = 0; i < AGENT_IMAGER_BARRIER_STATE_PROPERTY->count; i++) {
			indigo_item *item = AGENT_IMAGER_BARRIER_STATE_PROPERTY->items + i;
			if (!strcmp(item->name, property->device)) {
				item->light.value = property->state;
				indigo_update_property(device, AGENT_IMAGER_BARRIER_STATE_PROPERTY, NULL);
			}
			CLIENT_PRIVATE_DATA->barrier_resume &= (item->light.value == INDIGO_BUSY_STATE);
		}
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "Breakpoint barrier state %s", CLIENT_PRIVATE_DATA->barrier_resume ? "complete" : "incomplete");
	}
}

static void snoop_guider_changes(indigo_client *client, indigo_property *property) {
	indigo_device *device = FILTER_CLIENT_CONTEXT->device;
	char *related_agent_name = indigo_filter_first_related_agent(device, "Guider Agent");
	if (related_agent_name && !strcmp(related_agent_name, property->device)) {
		if (!strcmp(property->name, AGENT_GUIDER_STATS_PROPERTY_NAME)) {
			int phase = 0;
			int frame = 0;
			for (int i = 0; i < property->count; i++) {
				indigo_item *item = property->items + i;
				if (!strcmp(item->name, AGENT_GUIDER_STATS_DITHERING_ITEM_NAME)) {
					AGENT_IMAGER_STATS_DITHERING_ITEM->number.value = item->number.value;
					indigo_update_property(device, AGENT_IMAGER_STATS_PROPERTY, NULL);
				} else if (!strcmp(item->name, AGENT_GUIDER_STATS_PHASE_ITEM_NAME)) {
					phase = (int)item->number.value;
				} else if (!strcmp(item->name, AGENT_GUIDER_STATS_FRAME_ITEM_NAME)) {
					frame = (int)item->number.value;
				}
			}
			DEVICE_PRIVATE_DATA->guiding = (phase == INDIGO_GUIDER_PHASE_GUIDING) && (frame > 5);
		} else if (!strcmp(property->name, AGENT_GUIDER_DITHER_PROPERTY_NAME)) {
			for (int i = 0; i < property->count; i++) {
				indigo_item *item = property->items + i;
				if (!strcmp(item->name, AGENT_GUIDER_DITHER_TRIGGER_ITEM_NAME)) {
					if (!CLIENT_PRIVATE_DATA->dithering_finished) {
						if (item->sw.value && property->state == INDIGO_BUSY_STATE && !CLIENT_PRIVATE_DATA->dithering_started) {
							CLIENT_PRIVATE_DATA->dithering_started = true;
						} else if (property->state == INDIGO_OK_STATE && CLIENT_PRIVATE_DATA->dithering_started) {
							CLIENT_PRIVATE_DATA->dithering_finished = true;
						} else if (property->state == INDIGO_ALERT_STATE) {
							CLIENT_PRIVATE_DATA->dithering_started = true;
							CLIENT_PRIVATE_DATA->dithering_finished = true;
						}
					}
					break;
				}
			}
		} else if (!strcmp(property->name, AGENT_START_PROCESS_PROPERTY_NAME)) {
			CLIENT_PRIVATE_DATA->related_guider_process_state = property->state;
		}
	}
}

static void snoop_astrometry_changes(indigo_client *client, indigo_property *property) {
	char *related_agent_name = indigo_filter_first_related_agent(FILTER_CLIENT_CONTEXT->device, "Astrometry Agent");
	if (related_agent_name && !strcmp(property->device, related_agent_name)) {
		if (!strcmp(property->name, AGENT_START_PROCESS_PROPERTY_NAME)) {
			CLIENT_PRIVATE_DATA->related_solver_process_state = property->state;
		}
	} else {
		related_agent_name = indigo_filter_first_related_agent(FILTER_CLIENT_CONTEXT->device, "ASTAP Agent");
		if (related_agent_name && !strcmp(property->device, related_agent_name)) {
			if (!strcmp(property->name, AGENT_START_PROCESS_PROPERTY_NAME)) {
				CLIENT_PRIVATE_DATA->related_solver_process_state = property->state;
			}
		}
	}
}

static indigo_result agent_define_property(indigo_client *client, indigo_device *device, indigo_property *property, const char *message) {
	if (device == FILTER_CLIENT_CONTEXT->device) {
		if (!strcmp(property->name, CCD_BIN_PROPERTY_NAME)) {
			if (property->state == INDIGO_OK_STATE) {
				bool reset_selection = false;
				for (int i = 0; i < property->count; i++) {
					indigo_item *item = property->items + i;
					if (strcmp(item->name, CCD_BIN_HORIZONTAL_ITEM_NAME) == 0) {
						if (CLIENT_PRIVATE_DATA->bin_x != item->number.value) {
							CLIENT_PRIVATE_DATA->bin_x = (int)item->number.value;
							reset_selection = true;
						}
					} else if (strcmp(item->name, CCD_BIN_VERTICAL_ITEM_NAME) == 0) {
						if (CLIENT_PRIVATE_DATA->bin_y != item->number.value) {
							CLIENT_PRIVATE_DATA->bin_y = (int)item->number.value;
							reset_selection = true;
						}
					}
				}
				if (reset_selection) {
					CLIENT_PRIVATE_DATA->last_width = (int)(CLIENT_PRIVATE_DATA->frame[2] / CLIENT_PRIVATE_DATA->bin_x);
					CLIENT_PRIVATE_DATA->last_height = (int)(CLIENT_PRIVATE_DATA->frame[3] / CLIENT_PRIVATE_DATA->bin_y);
					AGENT_IMAGER_SELECTION_INCLUDE_LEFT_ITEM->number.value = AGENT_IMAGER_SELECTION_INCLUDE_TOP_ITEM->number.value = AGENT_IMAGER_SELECTION_INCLUDE_WIDTH_ITEM->number.value = AGENT_IMAGER_SELECTION_INCLUDE_HEIGHT_ITEM->number.value = AGENT_IMAGER_SELECTION_EXCLUDE_LEFT_ITEM->number.value = AGENT_IMAGER_SELECTION_EXCLUDE_TOP_ITEM->number.value = AGENT_IMAGER_SELECTION_EXCLUDE_WIDTH_ITEM->number.value = AGENT_IMAGER_SELECTION_EXCLUDE_HEIGHT_ITEM->number.value = 0;
					validate_include_region(device, false);
					clear_selection(device);
				}
			}
		} else {
			snoop_changes(client, device, property);
		}
	} else {
		snoop_barrier_state(client, property);
		snoop_guider_changes(client, property);
		snoop_astrometry_changes(client, property);
	}
	return indigo_filter_define_property(client, device, property, message);
}

static indigo_result agent_update_property(indigo_client *client, indigo_device *device, indigo_property *property, const char *message) {
	if (device == FILTER_CLIENT_CONTEXT->device) {
		if (!strcmp(property->name, CCD_IMAGE_PROPERTY_NAME)) {
			if (property->state == INDIGO_OK_STATE) {
				indigo_item *item = property->items;
				indigo_copy_value(CLIENT_PRIVATE_DATA->last_image_url, item->blob.url);
				if (pthread_mutex_trylock(&DEVICE_PRIVATE_DATA->last_image_mutex) == 0) {
					if (item->blob.value) {
						CLIENT_PRIVATE_DATA->last_image = indigo_safe_realloc(CLIENT_PRIVATE_DATA->last_image, item->blob.size);
						memcpy(CLIENT_PRIVATE_DATA->last_image, item->blob.value, item->blob.size);
						CLIENT_PRIVATE_DATA->last_image_size = item->blob.size;
						if (validate_include_region(device, false)) {
							indigo_update_property(device, AGENT_IMAGER_SELECTION_PROPERTY, NULL);
						}
					} else if (CLIENT_PRIVATE_DATA->last_image) {
						free(CLIENT_PRIVATE_DATA->last_image);
						CLIENT_PRIVATE_DATA->last_image_size = 0;
						CLIENT_PRIVATE_DATA->last_image = NULL;
					}
					pthread_mutex_unlock(&DEVICE_PRIVATE_DATA->last_image_mutex);
				}
			}
		} else if (!strcmp(property->name, CCD_BIN_PROPERTY_NAME)) {
			if (property->state == INDIGO_OK_STATE) {
				bool reset_selection = false;
				for (int i = 0; i < property->count; i++) {
					indigo_item *item = property->items + i;
					if (strcmp(item->name, CCD_BIN_HORIZONTAL_ITEM_NAME) == 0) {
						if (CLIENT_PRIVATE_DATA->bin_x != item->number.value) {
							CLIENT_PRIVATE_DATA->bin_x = (int)item->number.value;
							reset_selection = true;
						}
					} else if (strcmp(item->name, CCD_BIN_VERTICAL_ITEM_NAME) == 0) {
						if (CLIENT_PRIVATE_DATA->bin_y != item->number.value) {
							CLIENT_PRIVATE_DATA->bin_y = (int)item->number.value;
							reset_selection = true;
						}
					}
				}
				if (reset_selection) {
					CLIENT_PRIVATE_DATA->last_width = (int)(CLIENT_PRIVATE_DATA->frame[2] / CLIENT_PRIVATE_DATA->bin_x);
					CLIENT_PRIVATE_DATA->last_height = (int)(CLIENT_PRIVATE_DATA->frame[3] / CLIENT_PRIVATE_DATA->bin_y);
					AGENT_IMAGER_SELECTION_INCLUDE_LEFT_ITEM->number.value = AGENT_IMAGER_SELECTION_INCLUDE_TOP_ITEM->number.value = AGENT_IMAGER_SELECTION_INCLUDE_WIDTH_ITEM->number.value = AGENT_IMAGER_SELECTION_INCLUDE_HEIGHT_ITEM->number.value = AGENT_IMAGER_SELECTION_EXCLUDE_LEFT_ITEM->number.value = AGENT_IMAGER_SELECTION_EXCLUDE_TOP_ITEM->number.value = AGENT_IMAGER_SELECTION_EXCLUDE_WIDTH_ITEM->number.value = AGENT_IMAGER_SELECTION_EXCLUDE_HEIGHT_ITEM->number.value = 0;
					validate_include_region(device, false);
					clear_selection(device);
				}
			}
		} else {
			snoop_changes(client, device, property);
		}
	} else {
		snoop_barrier_state(client, property);
		snoop_guider_changes(client, property);
		snoop_astrometry_changes(client, property);
	}
	return indigo_filter_update_property(client, device, property, message);
}

static indigo_result agent_delete_property(indigo_client *client, indigo_device *device, indigo_property *property, const char *message) {
	if (device == FILTER_CLIENT_CONTEXT->device) {
		if (!strcmp(property->name, CCD_EXPOSURE_PROPERTY_NAME) || *property->name == 0) {
			DEVICE_PRIVATE_DATA->has_camera = false;
		}
	}
	return indigo_filter_delete_property(client, device, property, message);
}


// -------------------------------------------------------------------------------- Initialization

static imager_agent_private_data *private_data = NULL;

static indigo_device *agent_device = NULL;
static indigo_client *agent_client = NULL;

indigo_result indigo_agent_imager(indigo_driver_action action, indigo_driver_info *info) {
	static indigo_device agent_device_template = INDIGO_DEVICE_INITIALIZER(
		IMAGER_AGENT_NAME,
		agent_device_attach,
		agent_enumerate_properties,
		agent_change_property,
		NULL,
		agent_device_detach
	);

	static indigo_client agent_client_template = {
		IMAGER_AGENT_NAME, false, NULL, INDIGO_OK, INDIGO_VERSION_CURRENT, NULL,
		indigo_filter_client_attach,
		agent_define_property,
		agent_update_property,
		agent_delete_property,
		NULL,
		indigo_filter_client_detach
	};

	static indigo_driver_action last_action = INDIGO_DRIVER_SHUTDOWN;

	SET_DRIVER_INFO(info, IMAGER_AGENT_NAME, __FUNCTION__, DRIVER_VERSION, false, last_action);

	if (action == last_action)
		return INDIGO_OK;

	switch(action) {
		case INDIGO_DRIVER_INIT:
			last_action = action;
			private_data = indigo_safe_malloc(sizeof(imager_agent_private_data));
			agent_device = indigo_safe_malloc_copy(sizeof(indigo_device), &agent_device_template);
			agent_device->private_data = private_data;
			indigo_attach_device(agent_device);

			agent_client = indigo_safe_malloc_copy(sizeof(indigo_client), &agent_client_template);
			agent_client->client_context = agent_device->device_context;
			indigo_attach_client(agent_client);
			break;

		case INDIGO_DRIVER_SHUTDOWN:
			last_action = action;
			if (agent_client != NULL) {
				indigo_detach_client(agent_client);
				free(agent_client);
				agent_client = NULL;
			}
			if (agent_device != NULL) {
				indigo_detach_device(agent_device);
				free(agent_device);
				agent_device = NULL;
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
