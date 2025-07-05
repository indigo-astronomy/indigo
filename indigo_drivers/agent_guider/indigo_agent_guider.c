// Copyright (c) 2019 CloudMakers, s. r. o.
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

/** INDIGO Guider agent
 \file indigo_agent_guider.c
 */

#define DRIVER_VERSION 0x0029
#define DRIVER_NAME	"indigo_agent_guider"

#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <math.h>
#include <assert.h>
#include <pthread.h>
#include <errno.h>
#include <limits.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>

#include <indigo/indigo_driver_xml.h>
#include <indigo/indigo_filter.h>
#include <indigo/indigo_ccd_driver.h>
#include <indigo/indigo_raw_utils.h>
#include <indigo/indigo_io.h>
#include <indigo/indigo_server_tcp.h>

#include "indigo_agent_guider.h"

#define PI (3.14159265358979)
#define PI2 (PI/2)

#define DONUTS_MIN_SNR (19.0)      /* Minimum SNR for donuts detection */

#define MIN_COS_DEC (0.017)       /* DEC = ~89 degrees */

#define SAFE_RADIUS_FACTOR (0.9)   /* factor to multiply SELECTION_RADIUS in which the star will not be lost */

#define DEVICE_PRIVATE_DATA										((guider_agent_private_data *)device->private_data)
#define CLIENT_PRIVATE_DATA										((guider_agent_private_data *)FILTER_CLIENT_CONTEXT->device->private_data)

#define AGENT_GUIDER_DETECTION_MODE_PROPERTY	(DEVICE_PRIVATE_DATA->agent_guider_detection_mode_property)
#define AGENT_GUIDER_DETECTION_SELECTION_ITEM 	(AGENT_GUIDER_DETECTION_MODE_PROPERTY->items+0)
#define AGENT_GUIDER_DETECTION_WEIGHTED_SELECTION_ITEM 	(AGENT_GUIDER_DETECTION_MODE_PROPERTY->items+1)
#define AGENT_GUIDER_DETECTION_DONUTS_ITEM  		(AGENT_GUIDER_DETECTION_MODE_PROPERTY->items+2)
#define AGENT_GUIDER_DETECTION_CENTROID_ITEM  	(AGENT_GUIDER_DETECTION_MODE_PROPERTY->items+3)

#define AGENT_GUIDER_DEC_MODE_PROPERTY				(DEVICE_PRIVATE_DATA->agent_guider_dec_mode_property)
#define AGENT_GUIDER_DEC_MODE_BOTH_ITEM    		(AGENT_GUIDER_DEC_MODE_PROPERTY->items+0)
#define AGENT_GUIDER_DEC_MODE_NORTH_ITEM    	(AGENT_GUIDER_DEC_MODE_PROPERTY->items+1)
#define AGENT_GUIDER_DEC_MODE_SOUTH_ITEM    	(AGENT_GUIDER_DEC_MODE_PROPERTY->items+2)
#define AGENT_GUIDER_DEC_MODE_NONE_ITEM    		(AGENT_GUIDER_DEC_MODE_PROPERTY->items+3)

#define AGENT_GUIDER_APPLY_DEC_BACKLASH_PROPERTY	(DEVICE_PRIVATE_DATA->agent_guider_apply_dec_backlash_property)
#define AGENT_GUIDER_APPLY_DEC_BACKLASH_DISABLED_ITEM    		(AGENT_GUIDER_APPLY_DEC_BACKLASH_PROPERTY->items+0)
#define AGENT_GUIDER_APPLY_DEC_BACKLASH_ENABLED_ITEM    		(AGENT_GUIDER_APPLY_DEC_BACKLASH_PROPERTY->items+1)

#define AGENT_START_PROCESS_PROPERTY					(DEVICE_PRIVATE_DATA->agent_start_process_property)
#define AGENT_GUIDER_START_PREVIEW_1_ITEM  		(AGENT_START_PROCESS_PROPERTY->items+0)
#define AGENT_GUIDER_START_PREVIEW_ITEM  			(AGENT_START_PROCESS_PROPERTY->items+1)
#define AGENT_GUIDER_START_CALIBRATION_ITEM 	(AGENT_START_PROCESS_PROPERTY->items+2)
#define AGENT_GUIDER_START_CALIBRATION_AND_GUIDING_ITEM 	(AGENT_START_PROCESS_PROPERTY->items+3)
#define AGENT_GUIDER_START_GUIDING_ITEM 			(AGENT_START_PROCESS_PROPERTY->items+4)
#define AGENT_GUIDER_CLEAR_SELECTION_ITEM 		(AGENT_START_PROCESS_PROPERTY->items+5)

#define AGENT_ABORT_PROCESS_PROPERTY					(DEVICE_PRIVATE_DATA->agent_abort_process_property)
#define AGENT_ABORT_PROCESS_ITEM      				(AGENT_ABORT_PROCESS_PROPERTY->items+0)

#define AGENT_GUIDER_MOUNT_COORDINATES_PROPERTY       (DEVICE_PRIVATE_DATA->agent_mount_coordinates_property)
#define AGENT_GUIDER_MOUNT_COORDINATES_RA_ITEM        (AGENT_GUIDER_MOUNT_COORDINATES_PROPERTY->items+0)
#define AGENT_GUIDER_MOUNT_COORDINATES_DEC_ITEM       (AGENT_GUIDER_MOUNT_COORDINATES_PROPERTY->items+1)
#define AGENT_GUIDER_MOUNT_COORDINATES_SOP_ITEM       (AGENT_GUIDER_MOUNT_COORDINATES_PROPERTY->items+2)

#define AGENT_GUIDER_SETTINGS_PROPERTY				(DEVICE_PRIVATE_DATA->agent_settings_property)
#define AGENT_GUIDER_SETTINGS_EXPOSURE_ITEM   (AGENT_GUIDER_SETTINGS_PROPERTY->items+0)
#define AGENT_GUIDER_SETTINGS_DELAY_ITEM   		(AGENT_GUIDER_SETTINGS_PROPERTY->items+1)
#define AGENT_GUIDER_SETTINGS_STEP_ITEM       (AGENT_GUIDER_SETTINGS_PROPERTY->items+2)
#define AGENT_GUIDER_SETTINGS_BL_STEPS_ITEM  	(AGENT_GUIDER_SETTINGS_PROPERTY->items+3)
#define AGENT_GUIDER_SETTINGS_BL_DRIFT_ITEM  	(AGENT_GUIDER_SETTINGS_PROPERTY->items+4)
#define AGENT_GUIDER_SETTINGS_CAL_STEPS_ITEM  (AGENT_GUIDER_SETTINGS_PROPERTY->items+5)
#define AGENT_GUIDER_SETTINGS_CAL_DRIFT_ITEM  (AGENT_GUIDER_SETTINGS_PROPERTY->items+6)
#define AGENT_GUIDER_SETTINGS_ANGLE_ITEM      (AGENT_GUIDER_SETTINGS_PROPERTY->items+7)
#define AGENT_GUIDER_SETTINGS_SOP_ITEM        (AGENT_GUIDER_SETTINGS_PROPERTY->items+8)
#define AGENT_GUIDER_SETTINGS_BACKLASH_ITEM   (AGENT_GUIDER_SETTINGS_PROPERTY->items+9)
#define AGENT_GUIDER_SETTINGS_SPEED_RA_ITEM   (AGENT_GUIDER_SETTINGS_PROPERTY->items+10)
#define AGENT_GUIDER_SETTINGS_SPEED_DEC_ITEM  (AGENT_GUIDER_SETTINGS_PROPERTY->items+11)
#define AGENT_GUIDER_SETTINGS_MIN_ERR_ITEM  	(AGENT_GUIDER_SETTINGS_PROPERTY->items+12)
#define AGENT_GUIDER_SETTINGS_MIN_PULSE_ITEM  (AGENT_GUIDER_SETTINGS_PROPERTY->items+13)
#define AGENT_GUIDER_SETTINGS_MAX_PULSE_ITEM  (AGENT_GUIDER_SETTINGS_PROPERTY->items+14)
#define AGENT_GUIDER_SETTINGS_AGG_RA_ITEM  		(AGENT_GUIDER_SETTINGS_PROPERTY->items+15)
#define AGENT_GUIDER_SETTINGS_AGG_DEC_ITEM  	(AGENT_GUIDER_SETTINGS_PROPERTY->items+16)
#define AGENT_GUIDER_SETTINGS_I_GAIN_RA_ITEM		(AGENT_GUIDER_SETTINGS_PROPERTY->items+17)
#define AGENT_GUIDER_SETTINGS_I_GAIN_DEC_ITEM  	(AGENT_GUIDER_SETTINGS_PROPERTY->items+18)
#define AGENT_GUIDER_SETTINGS_STACK_ITEM  		(AGENT_GUIDER_SETTINGS_PROPERTY->items+19)
#define AGENT_GUIDER_SETTINGS_DITHERING_AMOUNT_ITEM	(AGENT_GUIDER_SETTINGS_PROPERTY->items+20)
#define AGENT_GUIDER_SETTINGS_DITHERING_TIME_LIMIT_ITEM 		(AGENT_GUIDER_SETTINGS_PROPERTY->items+21)
#define AGENT_GUIDER_SETTINGS_DITH_LIMIT_ITEM	(AGENT_GUIDER_SETTINGS_PROPERTY->items+22)

#define AGENT_GUIDER_FLIP_REVERSES_DEC_PROPERTY	(DEVICE_PRIVATE_DATA->agent_flip_reverses_dec_property)
#define AGENT_GUIDER_FLIP_REVERSES_DEC_ENABLED_ITEM		(AGENT_GUIDER_FLIP_REVERSES_DEC_PROPERTY->items+0)
#define AGENT_GUIDER_FLIP_REVERSES_DEC_DISABLED_ITEM		(AGENT_GUIDER_FLIP_REVERSES_DEC_PROPERTY->items+1)

#define MAX_STAR_COUNT												75
#define AGENT_GUIDER_STARS_PROPERTY						(DEVICE_PRIVATE_DATA->agent_stars_property)
#define AGENT_GUIDER_STARS_REFRESH_ITEM  			(AGENT_GUIDER_STARS_PROPERTY->items+0)

#define AGENT_GUIDER_SELECTION_PROPERTY							(DEVICE_PRIVATE_DATA->agent_selection_property)
#define AGENT_GUIDER_SELECTION_RADIUS_ITEM  				(AGENT_GUIDER_SELECTION_PROPERTY->items+0)
#define AGENT_GUIDER_SELECTION_SUBFRAME_ITEM				(AGENT_GUIDER_SELECTION_PROPERTY->items+1)
#define AGENT_GUIDER_SELECTION_EDGE_CLIPPING_ITEM		(AGENT_GUIDER_SELECTION_PROPERTY->items+2)
#define AGENT_GUIDER_SELECTION_INCLUDE_LEFT_ITEM		(AGENT_GUIDER_SELECTION_PROPERTY->items+3)
#define AGENT_GUIDER_SELECTION_INCLUDE_TOP_ITEM			(AGENT_GUIDER_SELECTION_PROPERTY->items+4)
#define AGENT_GUIDER_SELECTION_INCLUDE_WIDTH_ITEM		(AGENT_GUIDER_SELECTION_PROPERTY->items+5)
#define AGENT_GUIDER_SELECTION_INCLUDE_HEIGHT_ITEM	(AGENT_GUIDER_SELECTION_PROPERTY->items+6)
#define AGENT_GUIDER_SELECTION_EXCLUDE_LEFT_ITEM		(AGENT_GUIDER_SELECTION_PROPERTY->items+7)
#define AGENT_GUIDER_SELECTION_EXCLUDE_TOP_ITEM			(AGENT_GUIDER_SELECTION_PROPERTY->items+8)
#define AGENT_GUIDER_SELECTION_EXCLUDE_WIDTH_ITEM		(AGENT_GUIDER_SELECTION_PROPERTY->items+9)
#define AGENT_GUIDER_SELECTION_EXCLUDE_HEIGHT_ITEM	(AGENT_GUIDER_SELECTION_PROPERTY->items+10)
#define AGENT_GUIDER_SELECTION_STAR_COUNT_ITEM 			(AGENT_GUIDER_SELECTION_PROPERTY->items+11)
#define AGENT_GUIDER_SELECTION_X_ITEM  							(AGENT_GUIDER_SELECTION_PROPERTY->items+12)
#define AGENT_GUIDER_SELECTION_Y_ITEM  							(AGENT_GUIDER_SELECTION_PROPERTY->items+13)

#define MAX_STACK															20

#define AGENT_GUIDER_STATS_PROPERTY						(DEVICE_PRIVATE_DATA->agent_stats_property)
#define AGENT_GUIDER_STATS_PHASE_ITEM      		(AGENT_GUIDER_STATS_PROPERTY->items+0)
#define AGENT_GUIDER_STATS_FRAME_ITEM      		(AGENT_GUIDER_STATS_PROPERTY->items+1)
#define AGENT_GUIDER_STATS_REFERENCE_X_ITEM		(AGENT_GUIDER_STATS_PROPERTY->items+2)
#define AGENT_GUIDER_STATS_REFERENCE_Y_ITEM		(AGENT_GUIDER_STATS_PROPERTY->items+3)
#define AGENT_GUIDER_STATS_DRIFT_X_ITEM      	(AGENT_GUIDER_STATS_PROPERTY->items+4)
#define AGENT_GUIDER_STATS_DRIFT_Y_ITEM      	(AGENT_GUIDER_STATS_PROPERTY->items+5)
#define AGENT_GUIDER_STATS_DRIFT_RA_ITEM      (AGENT_GUIDER_STATS_PROPERTY->items+6)
#define AGENT_GUIDER_STATS_DRIFT_DEC_ITEM     (AGENT_GUIDER_STATS_PROPERTY->items+7)
#define AGENT_GUIDER_STATS_DRIFT_RA_S_ITEM      (AGENT_GUIDER_STATS_PROPERTY->items+8)
#define AGENT_GUIDER_STATS_DRIFT_DEC_S_ITEM     (AGENT_GUIDER_STATS_PROPERTY->items+9)
#define AGENT_GUIDER_STATS_CORR_RA_ITEM      	(AGENT_GUIDER_STATS_PROPERTY->items+10)
#define AGENT_GUIDER_STATS_CORR_DEC_ITEM      (AGENT_GUIDER_STATS_PROPERTY->items+11)
#define AGENT_GUIDER_STATS_RMSE_RA_ITEM      	(AGENT_GUIDER_STATS_PROPERTY->items+12)
#define AGENT_GUIDER_STATS_RMSE_DEC_ITEM      (AGENT_GUIDER_STATS_PROPERTY->items+13)
#define AGENT_GUIDER_STATS_RMSE_RA_S_ITEM      	(AGENT_GUIDER_STATS_PROPERTY->items+14)
#define AGENT_GUIDER_STATS_RMSE_DEC_S_ITEM      (AGENT_GUIDER_STATS_PROPERTY->items+15)
#define AGENT_GUIDER_STATS_SNR_ITEM      			(AGENT_GUIDER_STATS_PROPERTY->items+16)
#define AGENT_GUIDER_STATS_DELAY_ITEM      		(AGENT_GUIDER_STATS_PROPERTY->items+17)
#define AGENT_GUIDER_STATS_DITHERING_ITEM			(AGENT_GUIDER_STATS_PROPERTY->items+18)

#define AGENT_GUIDER_DITHERING_STRATEGY_PROPERTY			(DEVICE_PRIVATE_DATA->agent_dithering_strategy_property)
#define AGENT_GUIDER_DITHERING_STRATEGY_RANDOM_SPIRAL_ITEM	(AGENT_GUIDER_DITHERING_STRATEGY_PROPERTY->items+0)
#define AGENT_GUIDER_DITHERING_STRATEGY_RANDOM_ITEM			(AGENT_GUIDER_DITHERING_STRATEGY_PROPERTY->items+1)
#define AGENT_GUIDER_DITHERING_STRATEGY_SPIRAL_ITEM			(AGENT_GUIDER_DITHERING_STRATEGY_PROPERTY->items+2)

#define AGENT_GUIDER_DITHERING_OFFSETS_PROPERTY		(DEVICE_PRIVATE_DATA->agent_dithering_offsets_property)
#define AGENT_GUIDER_DITHERING_OFFSETS_X_ITEM  		(AGENT_GUIDER_DITHERING_OFFSETS_PROPERTY->items+0)
#define AGENT_GUIDER_DITHERING_OFFSETS_Y_ITEM  		(AGENT_GUIDER_DITHERING_OFFSETS_PROPERTY->items+1)

#define AGENT_GUIDER_DITHER_PROPERTY				(DEVICE_PRIVATE_DATA->agent_dither_property)
#define AGENT_GUIDER_DITHER_TRIGGER_ITEM					(AGENT_GUIDER_DITHER_PROPERTY->items+0)
#define AGENT_GUIDER_DITHER_RESET_ITEM					(AGENT_GUIDER_DITHER_PROPERTY->items+1)

#define AGENT_GUIDER_LOG_PROPERTY           (DEVICE_PRIVATE_DATA->agent_log_property)
#define AGENT_GUIDER_LOG_DIR_ITEM           (AGENT_GUIDER_LOG_PROPERTY->items+0)
#define AGENT_GUIDER_LOG_TEMPLATE_ITEM        (AGENT_GUIDER_LOG_PROPERTY->items+1)

#define AGENT_PROCESS_FEATURES_PROPERTY				(DEVICE_PRIVATE_DATA->agent_process_features_property)
#define AGENT_GUIDER_ENABLE_LOGGING_FEATURE_ITEM			(AGENT_PROCESS_FEATURES_PROPERTY->items+0)
#define AGENT_GUIDER_FAIL_ON_CALIBRATION_ERROR_ITEM		(AGENT_PROCESS_FEATURES_PROPERTY->items+1)
#define AGENT_GUIDER_RESET_ON_CALIBRATION_ERROR_ITEM	(AGENT_PROCESS_FEATURES_PROPERTY->items+2)
#define AGENT_GUIDER_FAIL_ON_GUIDING_ERROR_ITEM				(AGENT_PROCESS_FEATURES_PROPERTY->items+3)
#define AGENT_GUIDER_CONTINUE_ON_GUIDING_ERROR_ITEM		(AGENT_PROCESS_FEATURES_PROPERTY->items+4)
#define AGENT_GUIDER_RESET_ON_GUIDING_ERROR_ITEM			(AGENT_PROCESS_FEATURES_PROPERTY->items+5)
#define AGENT_GUIDER_RESET_ON_GUIDING_ERROR_WAIT_ALL_STARS_ITEM			(AGENT_PROCESS_FEATURES_PROPERTY->items+6)
#define AGENT_GUIDER_USE_INCLUDE_FOR_DONUTS_ITEM			(AGENT_PROCESS_FEATURES_PROPERTY->items+7)

#define IS_DITHERING (AGENT_GUIDER_STATS_DITHERING_ITEM->number.value != 0)
#define NOT_DITHERING (AGENT_GUIDER_STATS_DITHERING_ITEM->number.value == 0)

#define BUSY_TIMEOUT 5

#define DIGEST_CONVERGE_ITERATIONS 3
#define GRID	32


typedef struct {
	indigo_property *agent_guider_detection_mode_property;
	indigo_property *agent_guider_dec_mode_property;
	indigo_property *agent_guider_apply_dec_backlash_property;
	indigo_property *agent_start_process_property;
	indigo_property *agent_abort_process_property;
	indigo_property *agent_mount_coordinates_property;
	indigo_property *agent_settings_property;
	indigo_property *agent_flip_reverses_dec_property;
	indigo_property *agent_stars_property;
	indigo_property *agent_selection_property;
	indigo_property *agent_stats_property;
	indigo_property *agent_dithering_strategy_property;
	indigo_property *agent_dithering_offsets_property;
	indigo_property *agent_dither_property;
	indigo_property *agent_log_property;
	indigo_property *agent_process_features_property;
	indigo_property_state guide_ra_state, guide_dec_state;
	double remaining_exposure_time;
	indigo_property_state exposure_state;
	int bin_x, bin_y;
	double frame[4];
	double saved_frame[4];
	double saved_include_region[4], saved_exclude_region[4];
	double saved_selection_x, saved_selection_y;
	bool autosubframing;
	indigo_star_detection stars[MAX_STAR_COUNT];
	indigo_frame_digest reference[INDIGO_MAX_MULTISTAR_COUNT + 1];
	double drift_x, drift_y, drift;
	double avg_drift_x, avg_drift_y;
	double rmse_ra_sum, rmse_dec_sum;
	double rmse_ra_s_sum, rmse_dec_s_sum;
	double rmse_ra_threshold, rmse_dec_threshold;
	double cos_dec;
	unsigned long rmse_count;
	void *last_image;
	long last_image_size;
	char last_image_url[INDIGO_VALUE_SIZE];
	pthread_mutex_t last_image_mutex;
	int last_width;
	int last_height;
	int phase;
	double stack_x[MAX_STACK], stack_y[MAX_STACK];
	int stack_size;
	unsigned int dither_num;
	pthread_mutex_t mutex;
	int log_file;
	char log_file_name[PATH_MAX];
	int stars_used_at_start;
	bool no_guiding_star;
	bool first_frame;
	bool has_camera;
	bool silence_warnings;
} guider_agent_private_data;

static char default_log_path[PATH_MAX] = { 0 };

// -------------------------------------------------------------------------------- INDIGO agent common code

static void save_config(indigo_device *device) {
	if (pthread_mutex_trylock(&DEVICE_CONTEXT->config_mutex) == 0) {
		pthread_mutex_unlock(&DEVICE_CONTEXT->config_mutex);
		pthread_mutex_lock(&DEVICE_PRIVATE_DATA->mutex);
		indigo_save_property(device, NULL, AGENT_GUIDER_SETTINGS_PROPERTY);
		indigo_save_property(device, NULL, AGENT_GUIDER_FLIP_REVERSES_DEC_PROPERTY);
		indigo_save_property(device, NULL, AGENT_GUIDER_DETECTION_MODE_PROPERTY);
		indigo_save_property(device, NULL, AGENT_GUIDER_DEC_MODE_PROPERTY);
		indigo_save_property(device, NULL, AGENT_GUIDER_APPLY_DEC_BACKLASH_PROPERTY);
		indigo_save_property(device, NULL, ADDITIONAL_INSTANCES_PROPERTY);
		indigo_save_property(device, NULL, AGENT_GUIDER_DITHERING_STRATEGY_PROPERTY);
		indigo_save_property(device, NULL, AGENT_GUIDER_LOG_PROPERTY);
		indigo_save_property(device, NULL, AGENT_PROCESS_FEATURES_PROPERTY);
		char *selection_property_items[] = { AGENT_GUIDER_SELECTION_RADIUS_ITEM_NAME, AGENT_GUIDER_SELECTION_SUBFRAME_ITEM_NAME, AGENT_GUIDER_SELECTION_EDGE_CLIPPING_ITEM_NAME, AGENT_GUIDER_SELECTION_STAR_COUNT_ITEM_NAME };
		indigo_save_property_items(device, NULL, AGENT_GUIDER_SELECTION_PROPERTY, 4, (const char **)selection_property_items);
		if (DEVICE_CONTEXT->property_save_file_handle) {
			CONFIG_PROPERTY->state = INDIGO_OK_STATE;
			close(DEVICE_CONTEXT->property_save_file_handle);
			DEVICE_CONTEXT->property_save_file_handle = 0;
		} else {
			CONFIG_PROPERTY->state = INDIGO_ALERT_STATE;
		}
		CONFIG_SAVE_ITEM->sw.value = false;
		indigo_update_property(device, CONFIG_PROPERTY, NULL);
		pthread_mutex_unlock(&DEVICE_PRIVATE_DATA->mutex);
	}
}

static void open_log(indigo_device *device) {
	time_t now = time(NULL);
	struct tm *local = localtime(&now);
	strncpy(DEVICE_PRIVATE_DATA->log_file_name, AGENT_GUIDER_LOG_DIR_ITEM->text.value, PATH_MAX);
	int len = (int)strlen(DEVICE_PRIVATE_DATA->log_file_name);
	strftime(DEVICE_PRIVATE_DATA->log_file_name + len, PATH_MAX - len, AGENT_GUIDER_LOG_TEMPLATE_ITEM->text.value, local);
	if (DEVICE_PRIVATE_DATA->log_file > 0) {
		close(DEVICE_PRIVATE_DATA->log_file);
	}
	DEVICE_PRIVATE_DATA->log_file = open(DEVICE_PRIVATE_DATA->log_file_name, O_WRONLY | O_CREAT | O_TRUNC, 0644);
	if (DEVICE_PRIVATE_DATA->log_file == -1) {
		indigo_send_message(device, "Failed to create guiding log file (%s)", strerror(errno));
	}
	indigo_server_remove_resource("/guiding");
	indigo_server_add_file_resource("/guiding", DEVICE_PRIVATE_DATA->log_file_name, "text/csv; charset=UTF-8");
}

static void write_log_header(indigo_device *device, const char *log_type) {
	if (DEVICE_PRIVATE_DATA->log_file > 0) {
		indigo_printf(DEVICE_PRIVATE_DATA->log_file, "\"Type:\",\"%s\"\r\n", log_type);
		indigo_printf(DEVICE_PRIVATE_DATA->log_file, "\r\n", log_type);
		indigo_printf(DEVICE_PRIVATE_DATA->log_file, "\"Camera:\",\"%s\"\r\n", FILTER_DEVICE_CONTEXT->device_name[INDIGO_FILTER_CCD_INDEX]);
		indigo_printf(DEVICE_PRIVATE_DATA->log_file, "\"Guider:\",\"%s\"\r\n", FILTER_DEVICE_CONTEXT->device_name[INDIGO_FILTER_GUIDER_INDEX]);
		indigo_printf(DEVICE_PRIVATE_DATA->log_file, "\r\n", log_type);
		for (int i = 0; i <AGENT_GUIDER_SETTINGS_PROPERTY->count; i++) {
			indigo_item *item = AGENT_GUIDER_SETTINGS_PROPERTY->items + i;
			indigo_printf(DEVICE_PRIVATE_DATA->log_file, "\"%s:\",%g\r\n", item->label, item->number.value);
		}
		for (int i = 0; i <AGENT_GUIDER_DETECTION_MODE_PROPERTY->count; i++) {
			indigo_item *item = AGENT_GUIDER_DETECTION_MODE_PROPERTY->items + i;
			if (item->sw.value) {
				indigo_printf(DEVICE_PRIVATE_DATA->log_file, "\"%s:\",\"%s\"\r\n", AGENT_GUIDER_DETECTION_MODE_PROPERTY->label, item->label);
			}
		}
		for (int i = 0; i <AGENT_GUIDER_DEC_MODE_PROPERTY->count; i++) {
			indigo_item *item = AGENT_GUIDER_DEC_MODE_PROPERTY->items + i;
			if (item->sw.value) {
				indigo_printf(DEVICE_PRIVATE_DATA->log_file, "\"%s:\",\"%s\"\r\n", AGENT_GUIDER_DEC_MODE_PROPERTY->label, item->label);
			}
		}
		for (int i = 0; i <AGENT_GUIDER_DITHERING_STRATEGY_PROPERTY->count; i++) {
			indigo_item *item = AGENT_GUIDER_DITHERING_STRATEGY_PROPERTY->items + i;
			if (item->sw.value) {
				indigo_printf(DEVICE_PRIVATE_DATA->log_file, "\"%s:\",\"%s\"\r\n", AGENT_GUIDER_DITHERING_STRATEGY_PROPERTY->label, item->label);
			}
		}
		for (int i = 0; i <AGENT_GUIDER_SELECTION_PROPERTY->count; i++) {
			indigo_item *item = AGENT_GUIDER_SELECTION_PROPERTY->items + i;
			indigo_printf(DEVICE_PRIVATE_DATA->log_file, "\"%s:\",%g\r\n", item->label, item->number.value);
		}
		indigo_printf(DEVICE_PRIVATE_DATA->log_file, "\r\n", log_type);
		indigo_printf(DEVICE_PRIVATE_DATA->log_file, "\"phase\",\"frame\",\"ref x\",\"ref y\",\"drift x\",\"drift y\",\"drift ra\",\"drift dec\",\"corr ra\",\"corr dec\",\"rmse ra\",\"rmse dec\",\"rmse dith\",\"snr\"\r\n");
	}
}

static void write_log_record(indigo_device *device) {
	if (DEVICE_PRIVATE_DATA->log_file > 0) {
		indigo_printf(DEVICE_PRIVATE_DATA->log_file, "%d,%d,%g,%g,%g,%g,%g,%g,%g,%g,%g,%g,%g,%g\r\n", (int)AGENT_GUIDER_STATS_PHASE_ITEM->number.value, (int)AGENT_GUIDER_STATS_FRAME_ITEM->number.value, AGENT_GUIDER_STATS_REFERENCE_X_ITEM->number.value, AGENT_GUIDER_STATS_REFERENCE_Y_ITEM->number.value, AGENT_GUIDER_STATS_DRIFT_X_ITEM->number.value, AGENT_GUIDER_STATS_DRIFT_Y_ITEM->number.value, AGENT_GUIDER_STATS_DRIFT_RA_S_ITEM->number.value, AGENT_GUIDER_STATS_DRIFT_DEC_S_ITEM->number.value, AGENT_GUIDER_STATS_CORR_RA_ITEM->number.value, AGENT_GUIDER_STATS_CORR_DEC_ITEM->number.value, AGENT_GUIDER_STATS_RMSE_RA_S_ITEM->number.value, AGENT_GUIDER_STATS_RMSE_DEC_S_ITEM->number.value, AGENT_GUIDER_STATS_DITHERING_ITEM->number.value, AGENT_GUIDER_STATS_SNR_ITEM->number.value);
	}
}

static void close_log(indigo_device *device) {
	if (DEVICE_PRIVATE_DATA->log_file > 0) {
		close(DEVICE_PRIVATE_DATA->log_file);
	}
	DEVICE_PRIVATE_DATA->log_file = -1;
}

static void allow_abort_by_mount_agent(indigo_device *device, bool state) {
	char *related_agent_name = indigo_filter_first_related_agent(device, "Mount Agent");
	if (related_agent_name) {
		indigo_change_switch_property_1(FILTER_DEVICE_CONTEXT->client, related_agent_name, AGENT_ABORT_RELATED_PROCESS_PROPERTY_NAME, AGENT_ABORT_GUIDER_ITEM_NAME, state);
	}
}

static inline bool after_meridian_flip(indigo_device *device) {
	return (AGENT_GUIDER_SETTINGS_SOP_ITEM->number.value != 0 && AGENT_GUIDER_MOUNT_COORDINATES_SOP_ITEM->number.value != 0 && AGENT_GUIDER_SETTINGS_SOP_ITEM->number.value != AGENT_GUIDER_MOUNT_COORDINATES_SOP_ITEM->number.value);
}

static inline double get_rotation_angle(indigo_device *device) {
	double angle = AGENT_GUIDER_SETTINGS_ANGLE_ITEM->number.value;
	if (after_meridian_flip(device)) {
		angle += 180;
		if (angle > 180) {
			angle -= 360;
		}
	}
	INDIGO_DRIVER_DEBUG(DRIVER_NAME, "Calibration Rotation Angle = %.3f (SOP = %d), Current SOP = %d => Effecive angle = %.3f", AGENT_GUIDER_SETTINGS_ANGLE_ITEM->number.value, (int)AGENT_GUIDER_SETTINGS_SOP_ITEM->number.value, (int)AGENT_GUIDER_MOUNT_COORDINATES_SOP_ITEM->number.value, angle);
	return angle;
}

static inline double get_dec_speed(indigo_device *device) {
	if (after_meridian_flip(device) && AGENT_GUIDER_FLIP_REVERSES_DEC_ENABLED_ITEM->sw.value) {
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "Declination speed reversed");
		return -AGENT_GUIDER_SETTINGS_SPEED_DEC_ITEM->number.value;
	}
	return AGENT_GUIDER_SETTINGS_SPEED_DEC_ITEM->number.value;
}

static void random_dither_values(double amount, double *dither_x, double *dither_y) {
	*dither_x = fabs(amount) * (drand48() - 0.5);
	*dither_y = fabs(amount) * (drand48() - 0.5);
}

static void spiral_dither_values(unsigned int dither_number, double amount, bool randomize, double *dither_x, double *dither_y) {
	int dx = -1;
	int dy = -1;
	int dither_num = dither_number / 4;
	int corner_num = dither_number % 4;
	if (corner_num == 0) {
		dx = -1;
		dy = 1;
	} else if (corner_num == 1) {
		dx = 1;
		dy = 1;
	} else if (corner_num == 2) {
		dx = 1;
		dy = -1;
	} else {
		dx = -1;
		dy = -1;
	}
	int amount2 = (int)round(amount/2);
	if (amount2 == 0) {
		*dither_x = 0;
		*dither_y = 0;
	} else {
		*dither_x = (double)(dx * dither_num % amount2 + dx);
		*dither_y = (double)(dy * dither_num % amount2 + dy);
	}
	if (randomize) {
		*dither_x = *dither_x - dx * (drand48()/1.1);
		*dither_y = *dither_y - dy * (drand48()/1.1);
	}
}

static void do_dither(indigo_device *device) {
	// if not guiding clear state and return
	if (AGENT_GUIDER_STATS_PHASE_ITEM->number.value != INDIGO_GUIDER_PHASE_GUIDING) {
		AGENT_GUIDER_DITHER_TRIGGER_ITEM->sw.value = false;
		AGENT_GUIDER_DITHER_RESET_ITEM->sw.value = false;
		AGENT_GUIDER_DITHER_PROPERTY->state = INDIGO_ALERT_STATE;
		indigo_update_property(device, AGENT_GUIDER_DITHER_PROPERTY, NULL);
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "Dither request igored, not guiding");
		return;
	}
	AGENT_GUIDER_DITHER_RESET_ITEM->sw.value = false;
	AGENT_GUIDER_DITHER_PROPERTY->state = INDIGO_BUSY_STATE;
	indigo_update_property(device, AGENT_GUIDER_DITHER_PROPERTY, NULL);
	double x_value = 0;
	double y_value = 0;
	if (AGENT_GUIDER_DITHERING_STRATEGY_RANDOM_SPIRAL_ITEM->sw.value) {
		spiral_dither_values(DEVICE_PRIVATE_DATA->dither_num++, AGENT_GUIDER_SETTINGS_DITHERING_AMOUNT_ITEM->number.target * 2, true, &x_value, &y_value);
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "Dithering RANDOMIZED_SPIRAL x_value = %.4f y_value = %.4f dither_num = %d", x_value, y_value, DEVICE_PRIVATE_DATA->dither_num - 1);
	} else if (AGENT_GUIDER_DITHERING_STRATEGY_SPIRAL_ITEM->sw.value) {
		spiral_dither_values(DEVICE_PRIVATE_DATA->dither_num++, AGENT_GUIDER_SETTINGS_DITHERING_AMOUNT_ITEM->number.target * 2, false, &x_value, &y_value);
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "Dithering SPIRAL x_value = %.4f y_value = %.4f dither_num = %d", x_value, y_value, DEVICE_PRIVATE_DATA->dither_num - 1);
	} else {
		random_dither_values(AGENT_GUIDER_SETTINGS_DITHERING_AMOUNT_ITEM->number.target * 2, &x_value, &y_value);
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "Dithering RANDOM x_value = %.4f y_value = %.4f", x_value, y_value);
	}
	static const char *names[] = { AGENT_GUIDER_DITHERING_OFFSETS_X_ITEM_NAME, AGENT_GUIDER_DITHERING_OFFSETS_Y_ITEM_NAME };
	double item_values[] = { x_value, y_value };
	indigo_change_number_property(NULL, device->name, AGENT_GUIDER_DITHERING_OFFSETS_PROPERTY_NAME, 2, names, item_values);
	for (int i = 0; i < 15; i++) { // wait up to 3s to start dithering
		if (IS_DITHERING) {
			break;
		}
		if (AGENT_ABORT_PROCESS_PROPERTY->state == INDIGO_BUSY_STATE) {
			AGENT_GUIDER_DITHER_PROPERTY->state = INDIGO_ALERT_STATE;
			AGENT_GUIDER_DITHER_TRIGGER_ITEM->sw.value = false;
			AGENT_GUIDER_DITHER_RESET_ITEM->sw.value = false;
			indigo_update_property(device, AGENT_GUIDER_DITHER_PROPERTY, NULL);
			return;
		}
		indigo_usleep(200000);
	}
	if (IS_DITHERING) {
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "Dithering started");
		double time_limit = AGENT_GUIDER_SETTINGS_DITHERING_TIME_LIMIT_ITEM->number.value * 5;
		for (int i = 0; i < time_limit; i++) { // wait up to time limit to finish dithering
			if (NOT_DITHERING) {
				INDIGO_DRIVER_DEBUG(DRIVER_NAME, "Dithering finished");
				break;
			}
			if (AGENT_ABORT_PROCESS_PROPERTY->state == INDIGO_BUSY_STATE) {
				AGENT_GUIDER_DITHER_PROPERTY->state = INDIGO_ALERT_STATE;
				AGENT_GUIDER_DITHER_TRIGGER_ITEM->sw.value = false;
				AGENT_GUIDER_DITHER_RESET_ITEM->sw.value = false;
				indigo_update_property(device, AGENT_GUIDER_DITHER_PROPERTY, NULL);
				return;
			}
			indigo_usleep(200000);
		}
		if (IS_DITHERING) {
			AGENT_GUIDER_DITHER_PROPERTY->state = INDIGO_ALERT_STATE;
			AGENT_GUIDER_DITHER_TRIGGER_ITEM->sw.value = false;
			AGENT_GUIDER_DITHER_RESET_ITEM->sw.value = false;
			AGENT_GUIDER_STATS_DITHERING_ITEM->number.value = 0;
			indigo_update_property(device, AGENT_GUIDER_STATS_PROPERTY, NULL);
			indigo_update_property(device, AGENT_GUIDER_DITHER_PROPERTY, NULL);
			INDIGO_DRIVER_ERROR(DRIVER_NAME, "Dithering failed");
			indigo_send_message(device, "Dithering failed to settle down, maybe the timeout is too short");
			indigo_usleep(200000);
			return;
		}
	}
	AGENT_GUIDER_DITHER_PROPERTY->state = INDIGO_OK_STATE;
	AGENT_GUIDER_DITHER_TRIGGER_ITEM->sw.value = false;
	AGENT_GUIDER_DITHER_RESET_ITEM->sw.value = false;
	indigo_update_property(device, AGENT_GUIDER_DITHER_PROPERTY, NULL);
	return;
}

static bool capture_frame(indigo_device *device) {
	indigo_property_state state = INDIGO_ALERT_STATE;
	if (DEVICE_PRIVATE_DATA->last_image) {
		free (DEVICE_PRIVATE_DATA->last_image);
		DEVICE_PRIVATE_DATA->last_image = NULL;
		DEVICE_PRIVATE_DATA->last_image_size = 0;
	}
	for (int exposure_attempt = 0; exposure_attempt < 3; exposure_attempt++) {
		if (AGENT_ABORT_PROCESS_PROPERTY->state == INDIGO_BUSY_STATE)
			return false;
		indigo_change_number_property_1(FILTER_DEVICE_CONTEXT->client, device->name, CCD_EXPOSURE_PROPERTY_NAME, CCD_EXPOSURE_ITEM_NAME, AGENT_GUIDER_SETTINGS_EXPOSURE_ITEM->number.target);
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
		while ((state = DEVICE_PRIVATE_DATA->exposure_state) == INDIGO_BUSY_STATE) {
			if (AGENT_ABORT_PROCESS_PROPERTY->state == INDIGO_BUSY_STATE)
				return false;
			if (remaining_exposure_time > 1) {
				indigo_usleep(200000);
			} else {
				indigo_usleep(10000);
			}
		}
		if (AGENT_ABORT_PROCESS_PROPERTY->state == INDIGO_BUSY_STATE)
			return false;
		if (state != INDIGO_OK_STATE) {
			INDIGO_DRIVER_ERROR(DRIVER_NAME, "CCD_EXPOSURE_PROPERTY didn't become OK");
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
			indigo_send_message(device, "RAW image not received");
			return false;
		}
		/* This is potentially bayered image, if so we need to equalize the channels */
		if (indigo_is_bayered_image(header, DEVICE_PRIVATE_DATA->last_image_size)) {
			INDIGO_DRIVER_DEBUG(DRIVER_NAME, "Bayered image detected, equalizing channels");
			indigo_equalize_bayer_channels(header->signature, (void*)header + sizeof(indigo_raw_header), header->width, header->height);
		}
		return true;
	}
	INDIGO_DRIVER_ERROR(DRIVER_NAME, "Exposure failed");
	return false;
}

static int usable_star_count(indigo_device *device) {
	int star_count = 0;
	for (star_count = 0; star_count < AGENT_GUIDER_SELECTION_STAR_COUNT_ITEM->number.value; star_count++) {
		if ((AGENT_GUIDER_SELECTION_X_ITEM + star_count)->number.value == 0 && (AGENT_GUIDER_SELECTION_Y_ITEM + star_count)->number.value == 0) {
			return star_count;
		}
	}
	return star_count;
}

static void clear_selection(indigo_device *device) {
	if (AGENT_GUIDER_STARS_PROPERTY->count > 1) {
		indigo_delete_property(device, AGENT_GUIDER_STARS_PROPERTY, NULL);
		AGENT_GUIDER_STARS_PROPERTY->count = 1;
		indigo_define_property(device, AGENT_GUIDER_STARS_PROPERTY, NULL);
	}
	indigo_update_property(device, AGENT_GUIDER_STARS_PROPERTY, NULL);
	for (int i = (int)(AGENT_GUIDER_SELECTION_X_ITEM - AGENT_GUIDER_SELECTION_PROPERTY->items); i < AGENT_GUIDER_SELECTION_PROPERTY->count; i++) {
		indigo_item *item = AGENT_GUIDER_SELECTION_PROPERTY->items + i;
		item->number.value = item->number.target = 0;
	}
	indigo_update_property(device, AGENT_GUIDER_SELECTION_PROPERTY, NULL);
}

static bool find_stars(indigo_device *device) {
	int star_count;
	indigo_raw_header *header = (indigo_raw_header *)(DEVICE_PRIVATE_DATA->last_image);
	indigo_delete_property(device, AGENT_GUIDER_STARS_PROPERTY, NULL);
	indigo_find_stars_precise_clipped(header->signature, (void*)header + sizeof(indigo_raw_header), AGENT_GUIDER_SELECTION_RADIUS_ITEM->number.value, header->width, header->height, MAX_STAR_COUNT, AGENT_GUIDER_SELECTION_INCLUDE_LEFT_ITEM->number.value, AGENT_GUIDER_SELECTION_INCLUDE_TOP_ITEM->number.value, AGENT_GUIDER_SELECTION_INCLUDE_WIDTH_ITEM->number.value, AGENT_GUIDER_SELECTION_INCLUDE_HEIGHT_ITEM->number.value, AGENT_GUIDER_SELECTION_EXCLUDE_LEFT_ITEM->number.value, AGENT_GUIDER_SELECTION_EXCLUDE_TOP_ITEM->number.value, AGENT_GUIDER_SELECTION_EXCLUDE_WIDTH_ITEM->number.value, AGENT_GUIDER_SELECTION_EXCLUDE_HEIGHT_ITEM->number.value, (indigo_star_detection *)&DEVICE_PRIVATE_DATA->stars, &star_count);
	AGENT_GUIDER_STARS_PROPERTY->count = star_count + 1;
	for (int i = 0; i < star_count; i++) {
		char name[8];
		char label[INDIGO_NAME_SIZE];
		snprintf(name, sizeof(name), "%d", i);
		snprintf(label, sizeof(label), "[%d, %d]", (int)DEVICE_PRIVATE_DATA->stars[i].x, (int)DEVICE_PRIVATE_DATA->stars[i].y);
		indigo_init_switch_item(AGENT_GUIDER_STARS_PROPERTY->items + i + 1, name, label, false);
	}
	AGENT_GUIDER_STARS_PROPERTY->state = INDIGO_OK_STATE;
	indigo_define_property(device, AGENT_GUIDER_STARS_PROPERTY, NULL);
	if (star_count == 0) {
		if (!DEVICE_PRIVATE_DATA->silence_warnings) {
			indigo_send_message(device, "No stars detected");
		}
		return false;
	}
	return true;
}

static bool validate_include_region(indigo_device *device, bool force) {
	if (!DEVICE_PRIVATE_DATA->autosubframing && DEVICE_PRIVATE_DATA->last_width > 0 && DEVICE_PRIVATE_DATA->last_height > 0) {
		int safety_margin = DEVICE_PRIVATE_DATA->last_width < DEVICE_PRIVATE_DATA->last_height ? DEVICE_PRIVATE_DATA->last_width * 0.05 : DEVICE_PRIVATE_DATA->last_height * 0.05;
		int safety_limit_left = safety_margin;
		int safety_limit_top = safety_margin;
		int safety_limit_right = DEVICE_PRIVATE_DATA->last_width - safety_margin;
		int safety_limit_bottom = DEVICE_PRIVATE_DATA->last_height - safety_margin;
		int include_left = AGENT_GUIDER_SELECTION_INCLUDE_LEFT_ITEM->number.value;
		int include_top = AGENT_GUIDER_SELECTION_INCLUDE_TOP_ITEM->number.value;
		int include_width = AGENT_GUIDER_SELECTION_INCLUDE_WIDTH_ITEM->number.value;
		int include_height = AGENT_GUIDER_SELECTION_INCLUDE_HEIGHT_ITEM->number.value;
		bool update = false;
		if (include_width > 0 && include_height > 0) {
			if (include_left < safety_limit_left) {
				AGENT_GUIDER_SELECTION_INCLUDE_LEFT_ITEM->number.value = include_left = safety_limit_left;
				update = true;
			}
			int include_right = include_left + include_width;
			if (include_right > safety_limit_right) {
				AGENT_GUIDER_SELECTION_INCLUDE_WIDTH_ITEM->number.value = include_width = safety_limit_right - include_left;
				update = true;
			}
			if (include_top < safety_limit_top) {
				AGENT_GUIDER_SELECTION_INCLUDE_TOP_ITEM->number.value = include_top = safety_limit_top;
				update = true;
			}
			int include_bottom = include_top + include_height;
			if (include_bottom > safety_limit_bottom) {
				AGENT_GUIDER_SELECTION_INCLUDE_HEIGHT_ITEM->number.value = include_height = safety_limit_bottom - include_top;
				update = true;
			}
		} else {
			AGENT_GUIDER_SELECTION_INCLUDE_LEFT_ITEM->number.value = safety_limit_left;
			AGENT_GUIDER_SELECTION_INCLUDE_TOP_ITEM->number.value = safety_limit_top;
			AGENT_GUIDER_SELECTION_INCLUDE_WIDTH_ITEM->number.value = safety_limit_right - safety_limit_left;
			AGENT_GUIDER_SELECTION_INCLUDE_HEIGHT_ITEM->number.value = safety_limit_bottom - safety_limit_top;
			update = true;
		}
		if (update || force) {
			if (AGENT_GUIDER_STARS_PROPERTY->count > 1) {
				indigo_delete_property(device, AGENT_GUIDER_STARS_PROPERTY, NULL);
				AGENT_GUIDER_STARS_PROPERTY->count = 1;
				indigo_define_property(device, AGENT_GUIDER_STARS_PROPERTY, NULL);
			}
			indigo_update_property(device, AGENT_GUIDER_STARS_PROPERTY, NULL);
			for (int i = (int)(AGENT_GUIDER_SELECTION_X_ITEM - AGENT_GUIDER_SELECTION_PROPERTY->items); i < AGENT_GUIDER_SELECTION_PROPERTY->count; i++) {
				indigo_item *item = AGENT_GUIDER_SELECTION_PROPERTY->items + i;
				item->number.value = item->number.target = 0;
			}
		}
		return update;
	}
	return false;
}

static int select_stars(indigo_device *device) {
	int star_count = 0;
	for (int i = 0; i < AGENT_GUIDER_SELECTION_STAR_COUNT_ITEM->number.value; i++) {
		indigo_item *item_x = AGENT_GUIDER_SELECTION_X_ITEM + 2 * i;
		indigo_item *item_y = AGENT_GUIDER_SELECTION_Y_ITEM + 2 * i;
		if (i == AGENT_GUIDER_STARS_PROPERTY->count - 1) {
			if (!DEVICE_PRIVATE_DATA->silence_warnings) {
				indigo_send_message(device, "Warning: Only %d suitable %s found (%d requested).", star_count, star_count == 1 ? "star" : "stars", (int)AGENT_GUIDER_SELECTION_STAR_COUNT_ITEM->number.value);
			}
			break;
		}
		item_x->number.target = item_x->number.value = DEVICE_PRIVATE_DATA->stars[i].x;
		item_y->number.target = item_y->number.value = DEVICE_PRIVATE_DATA->stars[i].y;
		star_count++;
	}
	/* In case the number of the stars found is less than AGENT_GUIDER_SELECTION_STAR_COUNT_ITEM
	 set ramaining selections to 0. Otherwise we will have leftover "ghost" stars from the
	 previous search.
	 */
	for (int i = star_count; i < AGENT_GUIDER_SELECTION_STAR_COUNT_ITEM->number.value; i++) {
		indigo_item *item_x = AGENT_GUIDER_SELECTION_X_ITEM + 2 * i;
		indigo_item *item_y = AGENT_GUIDER_SELECTION_Y_ITEM + 2 * i;
		item_x->number.target = item_x->number.value = 0;
		item_y->number.target = item_y->number.value = 0;
	}
	indigo_update_property(device, AGENT_GUIDER_SELECTION_PROPERTY, NULL);
	INDIGO_DRIVER_DEBUG(DRIVER_NAME, "Selected %d of %d stars (needed %d)", star_count, (int)AGENT_GUIDER_SELECTION_STAR_COUNT_ITEM->number.value, DEVICE_PRIVATE_DATA->stars_used_at_start);
	return star_count;
}

static bool check_selection(indigo_device *device) {
	if (AGENT_GUIDER_DETECTION_SELECTION_ITEM->sw.value || AGENT_GUIDER_DETECTION_WEIGHTED_SELECTION_ITEM->sw.value) {
		for (int i = 0; i < AGENT_GUIDER_SELECTION_STAR_COUNT_ITEM->number.value; i++) {
			if ((AGENT_GUIDER_SELECTION_X_ITEM + i)->number.value != 0 && (AGENT_GUIDER_SELECTION_Y_ITEM + i)->number.value != 0) {
				return true;
			}
		}
	}
	bool result;
	if ((result = capture_frame(device)) && AGENT_ABORT_PROCESS_PROPERTY->state != INDIGO_BUSY_STATE) {
		AGENT_GUIDER_STARS_PROPERTY->count = 1;
		result = find_stars(device);
	}
	if (result && AGENT_ABORT_PROCESS_PROPERTY->state != INDIGO_BUSY_STATE) {
		result = select_stars(device) > 0;
	}
	return result;
}

static bool capture_and_process_frame(indigo_device *device) {
	DEVICE_PRIVATE_DATA->no_guiding_star = false;
	if (!capture_frame(device)) {
		return false;
	}
	if (AGENT_ABORT_PROCESS_PROPERTY->state == INDIGO_BUSY_STATE) {
		return false;
	}
	indigo_raw_header *header = (indigo_raw_header *)(DEVICE_PRIVATE_DATA->last_image);
	if (AGENT_GUIDER_STATS_FRAME_ITEM->number.value <= 0) {
		AGENT_GUIDER_STATS_SNR_ITEM->number.value = 0;
		AGENT_GUIDER_STATS_REFERENCE_X_ITEM->number.value = 0;
		AGENT_GUIDER_STATS_REFERENCE_Y_ITEM->number.value = 0;
		for (int i = 0; i <= INDIGO_MAX_MULTISTAR_COUNT; i++) {
			indigo_delete_frame_digest(DEVICE_PRIVATE_DATA->reference + i);
		}
		DEVICE_PRIVATE_DATA->stack_size = 0;
		DEVICE_PRIVATE_DATA->drift_x = DEVICE_PRIVATE_DATA->drift_y = 0;
		if (AGENT_GUIDER_DETECTION_DONUTS_ITEM->sw.value) {
			indigo_result result;
			if (AGENT_GUIDER_USE_INCLUDE_FOR_DONUTS_ITEM->sw.value) {
				result = indigo_donuts_frame_digest_clipped(header->signature, (void*)header + sizeof(indigo_raw_header), header->width, header->height, AGENT_GUIDER_SELECTION_INCLUDE_LEFT_ITEM->number.value, AGENT_GUIDER_SELECTION_INCLUDE_TOP_ITEM->number.value, AGENT_GUIDER_SELECTION_INCLUDE_WIDTH_ITEM->number.value, AGENT_GUIDER_SELECTION_INCLUDE_HEIGHT_ITEM->number.value, DEVICE_PRIVATE_DATA->reference);
			} else {
				result = indigo_donuts_frame_digest(header->signature, (void*)header + sizeof(indigo_raw_header), header->width, header->height, (int)AGENT_GUIDER_SELECTION_EDGE_CLIPPING_ITEM->number.value, DEVICE_PRIVATE_DATA->reference);
			}
			if (result != INDIGO_OK) {
				if (!DEVICE_PRIVATE_DATA->silence_warnings) {
					indigo_send_message(device, "Warning: Failed to compute DONUTS digest");
				}
				DEVICE_PRIVATE_DATA->no_guiding_star = true;
				return false;
			}
			AGENT_GUIDER_STATS_SNR_ITEM->number.value = DEVICE_PRIVATE_DATA->reference->snr;
			if (AGENT_GUIDER_STATS_PHASE_ITEM->number.value >= INDIGO_GUIDER_PHASE_GUIDING && DEVICE_PRIVATE_DATA->reference->snr < DONUTS_MIN_SNR) {
				if (!DEVICE_PRIVATE_DATA->silence_warnings) {
					indigo_send_message(device, "Warning: Signal to noise ratio is poor, increase exposure time or use different star detection mode");
				}
				DEVICE_PRIVATE_DATA->no_guiding_star = true;
				return false;
			}
		} else if (AGENT_GUIDER_DETECTION_CENTROID_ITEM->sw.value) {
			indigo_result result = indigo_centroid_frame_digest(header->signature, (void*)header + sizeof(indigo_raw_header), header->width, header->height, DEVICE_PRIVATE_DATA->reference);
			if (result != INDIGO_OK) {
				if (!DEVICE_PRIVATE_DATA->silence_warnings) {
					indigo_send_message(device, "Warning: Failed to compute centroid digest");
				}
				DEVICE_PRIVATE_DATA->no_guiding_star = true;
				return false;
			}
			AGENT_GUIDER_STATS_SNR_ITEM->number.value = DEVICE_PRIVATE_DATA->reference->snr;
		} else if (AGENT_GUIDER_DETECTION_SELECTION_ITEM->sw.value || AGENT_GUIDER_DETECTION_WEIGHTED_SELECTION_ITEM->sw.value) {
			int count = AGENT_GUIDER_SELECTION_STAR_COUNT_ITEM->number.value;
			int used = 0;
			indigo_result result = INDIGO_OK;
			DEVICE_PRIVATE_DATA->reference->algorithm = centroid;
			DEVICE_PRIVATE_DATA->reference->width = header->width;
			DEVICE_PRIVATE_DATA->reference->height = header->height;
			DEVICE_PRIVATE_DATA->reference->centroid_x = 0;
			DEVICE_PRIVATE_DATA->reference->centroid_y = 0;
			DEVICE_PRIVATE_DATA->reference->snr = 0;
			for (int i = 0; i < count && result == INDIGO_OK; i++) {
				indigo_item *item_x = AGENT_GUIDER_SELECTION_X_ITEM + 2 * i;
				indigo_item *item_y = AGENT_GUIDER_SELECTION_Y_ITEM + 2 * i;
				if (item_x->number.value != 0 && item_y->number.value != 0) {
					used++;
					result = indigo_selection_frame_digest_iterative(header->signature, (void*)header + sizeof(indigo_raw_header), &item_x->number.value, &item_y->number.value, AGENT_GUIDER_SELECTION_RADIUS_ITEM->number.value, header->width, header->height, DEVICE_PRIVATE_DATA->reference + used, DIGEST_CONVERGE_ITERATIONS);
					DEVICE_PRIVATE_DATA->reference->centroid_x += DEVICE_PRIVATE_DATA->reference[used].centroid_x;
					DEVICE_PRIVATE_DATA->reference->centroid_y += DEVICE_PRIVATE_DATA->reference[used].centroid_y;
				}
			}
			if (used > 0) {
				DEVICE_PRIVATE_DATA->reference->centroid_x /= used;
				DEVICE_PRIVATE_DATA->reference->centroid_y /= used;
				DEVICE_PRIVATE_DATA->reference->snr = DEVICE_PRIVATE_DATA->reference[1].snr;
			} else {
				result = INDIGO_FAILED;
			}
			AGENT_GUIDER_STATS_SNR_ITEM->number.value = DEVICE_PRIVATE_DATA->reference->snr;
			if (result == INDIGO_OK) {
				indigo_update_property(device, AGENT_GUIDER_SELECTION_PROPERTY, NULL);
			} else {
				if (!DEVICE_PRIVATE_DATA->silence_warnings) {
					indigo_send_message(device, "Warning: No stars detected");
				}
				DEVICE_PRIVATE_DATA->no_guiding_star = true;
				return false;
			}
		} else {
			indigo_send_message(device, "No detection mode");
			return false;
		}
		if (DEVICE_PRIVATE_DATA->reference->algorithm == centroid) {
			AGENT_GUIDER_STATS_REFERENCE_X_ITEM->number.value = DEVICE_PRIVATE_DATA->reference->centroid_x + AGENT_GUIDER_DITHERING_OFFSETS_X_ITEM->number.value;
			AGENT_GUIDER_STATS_REFERENCE_Y_ITEM->number.value = DEVICE_PRIVATE_DATA->reference->centroid_y + AGENT_GUIDER_DITHERING_OFFSETS_Y_ITEM->number.value;
		}
		AGENT_GUIDER_STATS_FRAME_ITEM->number.value++;
		//indigo_update_property(device, AGENT_GUIDER_STATS_PROPERTY, NULL);
		return true;
	} else if (AGENT_GUIDER_STATS_FRAME_ITEM->number.value > 0) {
		indigo_frame_digest digest = { 0 };
		if (AGENT_GUIDER_DETECTION_DONUTS_ITEM->sw.value) {
			indigo_result result;
			if (AGENT_GUIDER_USE_INCLUDE_FOR_DONUTS_ITEM->sw.value) {
				result = indigo_donuts_frame_digest_clipped(header->signature, (void*)header + sizeof(indigo_raw_header), header->width, header->height, AGENT_GUIDER_SELECTION_INCLUDE_LEFT_ITEM->number.value, AGENT_GUIDER_SELECTION_INCLUDE_TOP_ITEM->number.value, AGENT_GUIDER_SELECTION_INCLUDE_WIDTH_ITEM->number.value, AGENT_GUIDER_SELECTION_INCLUDE_HEIGHT_ITEM->number.value, &digest);
			} else {
				result = indigo_donuts_frame_digest(header->signature, (void*)header + sizeof(indigo_raw_header), header->width, header->height, (int)AGENT_GUIDER_SELECTION_EDGE_CLIPPING_ITEM->number.value, &digest);
			}
			if (result != INDIGO_OK) {
				if (!DEVICE_PRIVATE_DATA->silence_warnings) {
					indigo_send_message(device, "Warning: Failed to compute DONUTS digest");
				}
				DEVICE_PRIVATE_DATA->no_guiding_star = true;
				return false;
			}
			AGENT_GUIDER_STATS_SNR_ITEM->number.value = digest.snr;
			if (AGENT_GUIDER_STATS_PHASE_ITEM->number.value >= INDIGO_GUIDER_PHASE_GUIDING && digest.snr < DONUTS_MIN_SNR) {
				if (!DEVICE_PRIVATE_DATA->silence_warnings) {
					indigo_send_message(device, "Warning: Signal to noise ratio is poor, increase exposure time or use different star detection mode");
				}
				DEVICE_PRIVATE_DATA->no_guiding_star = true;
				return false;
			}
		} else if (AGENT_GUIDER_DETECTION_CENTROID_ITEM->sw.value) {
			indigo_result result = indigo_centroid_frame_digest(header->signature, (void*)header + sizeof(indigo_raw_header), header->width, header->height, &digest) == INDIGO_OK;
			if (result != INDIGO_OK) {
				if (!DEVICE_PRIVATE_DATA->silence_warnings) {
					indigo_send_message(device, "Warning: Failed to compute centroid digest");
				}
				DEVICE_PRIVATE_DATA->no_guiding_star = true;
				return false;
			}
			AGENT_GUIDER_STATS_SNR_ITEM->number.value = DEVICE_PRIVATE_DATA->reference->snr;
		} else if (AGENT_GUIDER_DETECTION_SELECTION_ITEM->sw.value || AGENT_GUIDER_DETECTION_WEIGHTED_SELECTION_ITEM->sw.value) {
			int count = AGENT_GUIDER_SELECTION_STAR_COUNT_ITEM->number.value;
			int used = 0;
			indigo_frame_digest digests[INDIGO_MAX_MULTISTAR_COUNT] = { 0 };
			indigo_result result = INDIGO_OK;
			digest.algorithm = centroid;
			digest.centroid_x = 0;
			digest.centroid_y = 0;
			for (int i = 0; i < count && result == INDIGO_OK; i++) {
				indigo_item *item_x = AGENT_GUIDER_SELECTION_X_ITEM + 2 * i;
				indigo_item *item_y = AGENT_GUIDER_SELECTION_Y_ITEM + 2 * i;
				if (item_x->number.value != 0 && item_y->number.value != 0) {
					digests[used].algorithm = centroid;
					result = indigo_selection_frame_digest_iterative(header->signature, (void*)header + sizeof(indigo_raw_header), &item_x->number.value, &item_y->number.value, AGENT_GUIDER_SELECTION_RADIUS_ITEM->number.value, header->width, header->height, &digests[used], DIGEST_CONVERGE_ITERATIONS);
					used++;
				}
			}
			if (result == INDIGO_OK) {
				if (AGENT_GUIDER_DETECTION_SELECTION_ITEM->sw.value) {
					result = indigo_reduce_multistar_digest(DEVICE_PRIVATE_DATA->reference, DEVICE_PRIVATE_DATA->reference + 1, digests, used, &digest);
				} else {
					result = indigo_reduce_weighted_multistar_digest(DEVICE_PRIVATE_DATA->reference, DEVICE_PRIVATE_DATA->reference + 1, digests, used, &digest);
				}
			}
			AGENT_GUIDER_STATS_SNR_ITEM->number.value = digest.snr;
			if (result == INDIGO_OK) {
				indigo_update_property(device, AGENT_GUIDER_SELECTION_PROPERTY, NULL);
			} else {
				if (!DEVICE_PRIVATE_DATA->silence_warnings) {
					indigo_send_message(device, "Warning: No stars detected");
				}
				DEVICE_PRIVATE_DATA->no_guiding_star = true;
				return false;
			}
		} else {
			indigo_send_message(device, "No detection mode");
			return false;
		}
		double drift_x, drift_y;
		indigo_result result = indigo_calculate_drift(DEVICE_PRIVATE_DATA->reference, &digest, &drift_x, &drift_y);
		if (result != INDIGO_OK) {
			indigo_send_message(device, "Warning: Can't calculate drift");
			return false;
		}
		DEVICE_PRIVATE_DATA->drift_x = drift_x - AGENT_GUIDER_DITHERING_OFFSETS_X_ITEM->number.value;
		DEVICE_PRIVATE_DATA->drift_y = drift_y - AGENT_GUIDER_DITHERING_OFFSETS_Y_ITEM->number.value;
		int count = (int)fmin(AGENT_GUIDER_SETTINGS_STACK_ITEM->number.value, DEVICE_PRIVATE_DATA->stack_size);
		double stddev_x = indigo_stddev(DEVICE_PRIVATE_DATA->stack_x, count);
		double stddev_y = indigo_stddev(DEVICE_PRIVATE_DATA->stack_y, count);
		/* Use Modified PID controller - Large random errors are not used in I, to prevent overshoots */
		if (fabs(DEVICE_PRIVATE_DATA->drift_x) < 5 * stddev_x || AGENT_GUIDER_SETTINGS_STACK_ITEM->number.value > DEVICE_PRIVATE_DATA->stack_size || AGENT_GUIDER_SETTINGS_STACK_ITEM->number.value == 1) {
			memmove(DEVICE_PRIVATE_DATA->stack_x + 1, DEVICE_PRIVATE_DATA->stack_x, sizeof(double) * (MAX_STACK - 1));
			DEVICE_PRIVATE_DATA->stack_x[0] = DEVICE_PRIVATE_DATA->drift_x;
		} else {
			INDIGO_DRIVER_DEBUG(DRIVER_NAME, "Drift X = %.3f (avg = %.3f, stddev = %.3f) jump detected - not used in the I-term", DEVICE_PRIVATE_DATA->drift_x, DEVICE_PRIVATE_DATA->avg_drift_x, stddev_x);
		}
		if (fabs(DEVICE_PRIVATE_DATA->drift_y) < 5 * stddev_y || AGENT_GUIDER_SETTINGS_STACK_ITEM->number.value > DEVICE_PRIVATE_DATA->stack_size || AGENT_GUIDER_SETTINGS_STACK_ITEM->number.value == 1) {
			memmove(DEVICE_PRIVATE_DATA->stack_y + 1, DEVICE_PRIVATE_DATA->stack_y, sizeof(double) * (MAX_STACK - 1));
			DEVICE_PRIVATE_DATA->stack_y[0] = DEVICE_PRIVATE_DATA->drift_y;
		} else {
			INDIGO_DRIVER_DEBUG(DRIVER_NAME, "Drift Y = %.3f (avg = %.3f, stddev = %.3f) jump detected - not used in the I-term", DEVICE_PRIVATE_DATA->drift_y, DEVICE_PRIVATE_DATA->avg_drift_y, stddev_y);
		}
		if (DEVICE_PRIVATE_DATA->stack_size < MAX_STACK) {
			DEVICE_PRIVATE_DATA->stack_size++;
		}
		if (AGENT_GUIDER_SETTINGS_STACK_ITEM->number.value == 1 || AGENT_GUIDER_STATS_PHASE_ITEM->number.value != INDIGO_GUIDER_PHASE_GUIDING) {
			DEVICE_PRIVATE_DATA->avg_drift_x = 0;
			DEVICE_PRIVATE_DATA->avg_drift_y = 0;
		} else {
			double avg_x, avg_y;
			avg_x = DEVICE_PRIVATE_DATA->stack_x[0];
			avg_y = DEVICE_PRIVATE_DATA->stack_y[0];
			for (int i = 1; i < count; i++) {
				avg_x += DEVICE_PRIVATE_DATA->stack_x[i];
				avg_y += DEVICE_PRIVATE_DATA->stack_y[i];
			}
			/* Regardless if the stack is full we devide on stack size.
				This allows smooth error integration and proper I term operation at startup.
				*/
			DEVICE_PRIVATE_DATA->avg_drift_x = avg_x / AGENT_GUIDER_SETTINGS_STACK_ITEM->number.value;
			DEVICE_PRIVATE_DATA->avg_drift_y = avg_y / AGENT_GUIDER_SETTINGS_STACK_ITEM->number.value;
		}
		AGENT_GUIDER_STATS_DRIFT_X_ITEM->number.value = round(1000 * DEVICE_PRIVATE_DATA->drift_x) / 1000;
		AGENT_GUIDER_STATS_DRIFT_Y_ITEM->number.value = round(1000 * DEVICE_PRIVATE_DATA->drift_y) / 1000;
		DEVICE_PRIVATE_DATA->drift = sqrt(DEVICE_PRIVATE_DATA->drift_x * DEVICE_PRIVATE_DATA->drift_x + DEVICE_PRIVATE_DATA->drift_y * DEVICE_PRIVATE_DATA->drift_y);
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "Drift %.4gpx (%.4g, %.4g)", DEVICE_PRIVATE_DATA->drift, DEVICE_PRIVATE_DATA->drift_x, DEVICE_PRIVATE_DATA->drift_y);
		indigo_delete_frame_digest(&digest);
	}
	AGENT_GUIDER_STATS_FRAME_ITEM->number.value++;
	//indigo_update_property(device, AGENT_GUIDER_STATS_PROPERTY, NULL);
	return true;
}

static bool select_subframe(indigo_device *device) {
	int selection_x = AGENT_GUIDER_SELECTION_X_ITEM->number.value;
	int selection_y = AGENT_GUIDER_SELECTION_Y_ITEM->number.value;
	if (selection_x == 0 || selection_y == 0) {
		indigo_send_message(device, "Warning: Failed to select subframe.");
		return false;
	}
	if (AGENT_GUIDER_SELECTION_SUBFRAME_ITEM->number.value && DEVICE_PRIVATE_DATA->saved_frame[2] == 0 && DEVICE_PRIVATE_DATA->saved_frame[3] == 0) {
		DEVICE_PRIVATE_DATA->autosubframing = true;
		DEVICE_PRIVATE_DATA->saved_selection_x = selection_x;
		DEVICE_PRIVATE_DATA->saved_selection_y = selection_y;
		memcpy(DEVICE_PRIVATE_DATA->saved_frame, DEVICE_PRIVATE_DATA->frame, 4 * sizeof(double));
		int bin_x = DEVICE_PRIVATE_DATA->bin_x;
		int bin_y = DEVICE_PRIVATE_DATA->bin_y;
		selection_x += DEVICE_PRIVATE_DATA->frame[0] / bin_x; // left
		selection_y += DEVICE_PRIVATE_DATA->frame[1] / bin_y; // top
		int window_size = AGENT_GUIDER_SELECTION_SUBFRAME_ITEM->number.value * AGENT_GUIDER_SELECTION_RADIUS_ITEM->number.value;
		if (window_size < GRID)
			window_size = GRID;
		int frame_left = rint((selection_x - window_size) / (double)GRID) * GRID;
		int frame_top = rint((selection_y - window_size) / (double)GRID) * GRID;
		if (selection_x - frame_left < AGENT_GUIDER_SELECTION_RADIUS_ITEM->number.value)
			frame_left -= GRID;
		if (selection_y - frame_top < AGENT_GUIDER_SELECTION_RADIUS_ITEM->number.value)
			frame_top -= GRID;
		int frame_width = (2 * window_size / GRID + 1) * GRID;
		int frame_height = (2 * window_size / GRID + 1) * GRID;
		AGENT_GUIDER_SELECTION_X_ITEM->number.value = selection_x -= frame_left;
		AGENT_GUIDER_SELECTION_Y_ITEM->number.value = selection_y -= frame_top;
		DEVICE_PRIVATE_DATA->saved_include_region[0] = AGENT_GUIDER_SELECTION_INCLUDE_LEFT_ITEM->number.value;
		DEVICE_PRIVATE_DATA->saved_include_region[1] = AGENT_GUIDER_SELECTION_INCLUDE_TOP_ITEM->number.value;
		DEVICE_PRIVATE_DATA->saved_include_region[2] = AGENT_GUIDER_SELECTION_INCLUDE_WIDTH_ITEM->number.value;
		DEVICE_PRIVATE_DATA->saved_include_region[3] = AGENT_GUIDER_SELECTION_INCLUDE_HEIGHT_ITEM->number.value;
		DEVICE_PRIVATE_DATA->saved_exclude_region[0] = AGENT_GUIDER_SELECTION_EXCLUDE_LEFT_ITEM->number.value;
		DEVICE_PRIVATE_DATA->saved_exclude_region[1] = AGENT_GUIDER_SELECTION_EXCLUDE_TOP_ITEM->number.value;
		DEVICE_PRIVATE_DATA->saved_exclude_region[2] = AGENT_GUIDER_SELECTION_EXCLUDE_WIDTH_ITEM->number.value;
		DEVICE_PRIVATE_DATA->saved_exclude_region[3] = AGENT_GUIDER_SELECTION_EXCLUDE_HEIGHT_ITEM->number.value;
		AGENT_GUIDER_SELECTION_INCLUDE_LEFT_ITEM->number.value = AGENT_GUIDER_SELECTION_INCLUDE_TOP_ITEM->number.value = AGENT_GUIDER_SELECTION_INCLUDE_WIDTH_ITEM->number.value = AGENT_GUIDER_SELECTION_INCLUDE_HEIGHT_ITEM->number.value = AGENT_GUIDER_SELECTION_EXCLUDE_LEFT_ITEM->number.value = AGENT_GUIDER_SELECTION_EXCLUDE_TOP_ITEM->number.value = AGENT_GUIDER_SELECTION_EXCLUDE_WIDTH_ITEM->number.value = AGENT_GUIDER_SELECTION_EXCLUDE_HEIGHT_ITEM->number.value = 0;
		indigo_update_property(device, AGENT_GUIDER_SELECTION_PROPERTY, NULL);
		if (frame_width - selection_x < AGENT_GUIDER_SELECTION_RADIUS_ITEM->number.value)
			frame_width += GRID;
		if (frame_height - selection_y < AGENT_GUIDER_SELECTION_RADIUS_ITEM->number.value)
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
		AGENT_GUIDER_SELECTION_X_ITEM->number.target = AGENT_GUIDER_SELECTION_X_ITEM->number.value = DEVICE_PRIVATE_DATA->saved_selection_x;
		AGENT_GUIDER_SELECTION_Y_ITEM->number.target = AGENT_GUIDER_SELECTION_Y_ITEM->number.value = DEVICE_PRIVATE_DATA->saved_selection_y;
		AGENT_GUIDER_SELECTION_INCLUDE_LEFT_ITEM->number.value = DEVICE_PRIVATE_DATA->saved_include_region[0];
		AGENT_GUIDER_SELECTION_INCLUDE_TOP_ITEM->number.value = DEVICE_PRIVATE_DATA->saved_include_region[1];
		AGENT_GUIDER_SELECTION_INCLUDE_WIDTH_ITEM->number.value = DEVICE_PRIVATE_DATA->saved_include_region[2];
		AGENT_GUIDER_SELECTION_INCLUDE_HEIGHT_ITEM->number.value = DEVICE_PRIVATE_DATA->saved_include_region[3];
		AGENT_GUIDER_SELECTION_EXCLUDE_LEFT_ITEM->number.value = DEVICE_PRIVATE_DATA->saved_exclude_region[0];
		AGENT_GUIDER_SELECTION_EXCLUDE_TOP_ITEM->number.value = DEVICE_PRIVATE_DATA->saved_exclude_region[1];
		AGENT_GUIDER_SELECTION_EXCLUDE_WIDTH_ITEM->number.value = DEVICE_PRIVATE_DATA->saved_exclude_region[2];
		AGENT_GUIDER_SELECTION_EXCLUDE_HEIGHT_ITEM->number.value = DEVICE_PRIVATE_DATA->saved_exclude_region[3];
		/* capture_frame() should be here in order to have the correct frame and correct selection */
		indigo_property_state state = AGENT_ABORT_PROCESS_PROPERTY->state;
		AGENT_ABORT_PROCESS_PROPERTY->state = INDIGO_OK_STATE;
		indigo_usleep(0.5 * ONE_SECOND_DELAY);
		capture_frame(device);
		AGENT_ABORT_PROCESS_PROPERTY->state = state;
		indigo_update_property(device, AGENT_GUIDER_SELECTION_PROPERTY, NULL);
		DEVICE_PRIVATE_DATA->saved_selection_x = 0;
		DEVICE_PRIVATE_DATA->saved_selection_y = 0;
		DEVICE_PRIVATE_DATA->autosubframing = false;
	}
}

static indigo_property_state pulse_guide(indigo_device *device, double ra, double dec) {
	double ra_duration = 0, dec_duration = 0;
	if (ra) {
		static const char *names[] = { GUIDER_GUIDE_WEST_ITEM_NAME, GUIDER_GUIDE_EAST_ITEM_NAME };
		double values[] = { ra > 0 ? ra * 1000 : 0, ra < 0 ? -ra * 1000 : 0 };
		indigo_change_number_property(FILTER_DEVICE_CONTEXT->client, device->name, GUIDER_GUIDE_RA_PROPERTY_NAME, 2, names, values);
		ra_duration = fabs(ra) * 1000000;
	}
	if (dec) {
		static const char *names[] = { GUIDER_GUIDE_NORTH_ITEM_NAME, GUIDER_GUIDE_SOUTH_ITEM_NAME };
		double values[] = { dec > 0 ? dec * 1000 : 0, dec < 0 ? -dec * 1000 : 0 };
		indigo_change_number_property(FILTER_DEVICE_CONTEXT->client, device->name, GUIDER_GUIDE_DEC_PROPERTY_NAME, 2, names, values);
		dec_duration =fabs(dec) * 1000000;
	}
	if (ra_duration || dec_duration) {
		indigo_usleep(ra_duration > dec_duration ? ra_duration : dec_duration);
		for (int i = 0; i < 200 && (DEVICE_PRIVATE_DATA->guide_ra_state == INDIGO_BUSY_STATE || DEVICE_PRIVATE_DATA->guide_ra_state == INDIGO_BUSY_STATE); i++) {
			indigo_usleep(50000);
		}
	}
	return INDIGO_OK_STATE;
}

static void preview_1_process(indigo_device *device) {
	FILTER_DEVICE_CONTEXT->running_process = true;
	AGENT_GUIDER_STATS_PHASE_ITEM->number.value = INDIGO_GUIDER_PHASE_PREVIEWING;
	AGENT_GUIDER_STATS_FRAME_ITEM->number.value = AGENT_GUIDER_STATS_FRAME_ITEM->number.value = AGENT_GUIDER_STATS_REFERENCE_X_ITEM->number.value = AGENT_GUIDER_STATS_REFERENCE_Y_ITEM->number.value = AGENT_GUIDER_STATS_DRIFT_X_ITEM->number.value = AGENT_GUIDER_STATS_DRIFT_Y_ITEM->number.value = AGENT_GUIDER_STATS_DRIFT_RA_ITEM->number.value = AGENT_GUIDER_STATS_DRIFT_DEC_ITEM->number.value = AGENT_GUIDER_STATS_DRIFT_RA_S_ITEM->number.value = AGENT_GUIDER_STATS_DRIFT_DEC_S_ITEM->number.value = AGENT_GUIDER_STATS_CORR_RA_ITEM->number.value = AGENT_GUIDER_STATS_CORR_DEC_ITEM->number.value = AGENT_GUIDER_STATS_RMSE_RA_ITEM->number.value = AGENT_GUIDER_STATS_RMSE_DEC_ITEM->number.value = AGENT_GUIDER_STATS_RMSE_RA_S_ITEM->number.value = AGENT_GUIDER_STATS_RMSE_DEC_S_ITEM->number.value = AGENT_GUIDER_STATS_SNR_ITEM->number.value = AGENT_GUIDER_STATS_DITHERING_ITEM->number.value = 0;
	AGENT_GUIDER_DITHERING_OFFSETS_X_ITEM->number.value = AGENT_GUIDER_DITHERING_OFFSETS_X_ITEM->number.target = AGENT_GUIDER_DITHERING_OFFSETS_Y_ITEM->number.value = AGENT_GUIDER_DITHERING_OFFSETS_Y_ITEM->number.target = 0;
	indigo_update_property(device, AGENT_GUIDER_STATS_PROPERTY, NULL);
	indigo_update_property(device, AGENT_GUIDER_DITHERING_OFFSETS_PROPERTY, NULL);
	allow_abort_by_mount_agent(device, false);
	int upload_mode = indigo_save_switch_state(device, CCD_UPLOAD_MODE_PROPERTY_NAME, CCD_UPLOAD_MODE_CLIENT_ITEM_NAME);
	int image_format = indigo_save_switch_state(device, CCD_IMAGE_FORMAT_PROPERTY_NAME, CCD_IMAGE_FORMAT_RAW_ITEM_NAME);
	capture_frame(device);
	indigo_restore_switch_state(device, CCD_UPLOAD_MODE_PROPERTY_NAME, upload_mode);
	indigo_restore_switch_state(device, CCD_IMAGE_FORMAT_PROPERTY_NAME, image_format);
	if (AGENT_ABORT_PROCESS_PROPERTY->state == INDIGO_BUSY_STATE) {
		AGENT_ABORT_PROCESS_PROPERTY->state = INDIGO_OK_STATE;
		indigo_update_property(device, AGENT_ABORT_PROCESS_PROPERTY, NULL);
	}
	AGENT_GUIDER_STATS_PHASE_ITEM->number.value = AGENT_ABORT_PROCESS_PROPERTY->state == INDIGO_BUSY_STATE ? INDIGO_GUIDER_PHASE_DONE : INDIGO_GUIDER_PHASE_FAILED;
	AGENT_GUIDER_STATS_REFERENCE_X_ITEM->number.value = AGENT_GUIDER_STATS_REFERENCE_Y_ITEM->number.value = AGENT_GUIDER_STATS_DITHERING_ITEM->number.value = 0;
	indigo_update_property(device, AGENT_GUIDER_STATS_PROPERTY, NULL);
	AGENT_GUIDER_START_PREVIEW_1_ITEM->sw.value = false;
	AGENT_START_PROCESS_PROPERTY->state = AGENT_ABORT_PROCESS_PROPERTY->state == INDIGO_ALERT_STATE;
	indigo_update_property(device, AGENT_START_PROCESS_PROPERTY, NULL);
	FILTER_DEVICE_CONTEXT->running_process = false;
}

static void preview_process(indigo_device *device) {
	FILTER_DEVICE_CONTEXT->running_process = true;
	AGENT_GUIDER_STATS_PHASE_ITEM->number.value = INDIGO_GUIDER_PHASE_PREVIEWING;
	AGENT_GUIDER_STATS_FRAME_ITEM->number.value = AGENT_GUIDER_STATS_FRAME_ITEM->number.value = AGENT_GUIDER_STATS_REFERENCE_X_ITEM->number.value = AGENT_GUIDER_STATS_REFERENCE_Y_ITEM->number.value = AGENT_GUIDER_STATS_DRIFT_X_ITEM->number.value = AGENT_GUIDER_STATS_DRIFT_Y_ITEM->number.value = AGENT_GUIDER_STATS_DRIFT_RA_ITEM->number.value = AGENT_GUIDER_STATS_DRIFT_DEC_ITEM->number.value = AGENT_GUIDER_STATS_DRIFT_RA_S_ITEM->number.value = AGENT_GUIDER_STATS_DRIFT_DEC_S_ITEM->number.value = AGENT_GUIDER_STATS_CORR_RA_ITEM->number.value = AGENT_GUIDER_STATS_CORR_DEC_ITEM->number.value = AGENT_GUIDER_STATS_RMSE_RA_ITEM->number.value = AGENT_GUIDER_STATS_RMSE_DEC_ITEM->number.value = AGENT_GUIDER_STATS_RMSE_RA_S_ITEM->number.value = AGENT_GUIDER_STATS_RMSE_DEC_S_ITEM->number.value = AGENT_GUIDER_STATS_SNR_ITEM->number.value = AGENT_GUIDER_STATS_DITHERING_ITEM->number.value = 0;
	AGENT_GUIDER_DITHERING_OFFSETS_X_ITEM->number.value = AGENT_GUIDER_DITHERING_OFFSETS_X_ITEM->number.target = AGENT_GUIDER_DITHERING_OFFSETS_Y_ITEM->number.value = AGENT_GUIDER_DITHERING_OFFSETS_Y_ITEM->number.target = 0;
	indigo_update_property(device, AGENT_GUIDER_STATS_PROPERTY, NULL);
	indigo_update_property(device, AGENT_GUIDER_DITHERING_OFFSETS_PROPERTY, NULL);
	allow_abort_by_mount_agent(device, false);
	int upload_mode = indigo_save_switch_state(device, CCD_UPLOAD_MODE_PROPERTY_NAME, CCD_UPLOAD_MODE_CLIENT_ITEM_NAME);
	int image_format = indigo_save_switch_state(device, CCD_IMAGE_FORMAT_PROPERTY_NAME, CCD_IMAGE_FORMAT_RAW_ITEM_NAME);
	while (capture_frame(device));
	indigo_restore_switch_state(device, CCD_UPLOAD_MODE_PROPERTY_NAME, upload_mode);
	indigo_restore_switch_state(device, CCD_IMAGE_FORMAT_PROPERTY_NAME, image_format);
	if (AGENT_ABORT_PROCESS_PROPERTY->state == INDIGO_BUSY_STATE) {
		AGENT_ABORT_PROCESS_PROPERTY->state = INDIGO_OK_STATE;
		indigo_update_property(device, AGENT_ABORT_PROCESS_PROPERTY, NULL);
	}
	AGENT_GUIDER_STATS_PHASE_ITEM->number.value = AGENT_ABORT_PROCESS_PROPERTY->state == INDIGO_BUSY_STATE ? INDIGO_GUIDER_PHASE_DONE : INDIGO_GUIDER_PHASE_FAILED;
	AGENT_GUIDER_STATS_REFERENCE_X_ITEM->number.value = AGENT_GUIDER_STATS_REFERENCE_Y_ITEM->number.value = AGENT_GUIDER_STATS_DITHERING_ITEM->number.value = 0;
	indigo_update_property(device, AGENT_GUIDER_STATS_PROPERTY, NULL);
	AGENT_GUIDER_START_PREVIEW_ITEM->sw.value = false;
	AGENT_START_PROCESS_PROPERTY->state = AGENT_ABORT_PROCESS_PROPERTY->state == INDIGO_ALERT_STATE;
	indigo_update_property(device, AGENT_START_PROCESS_PROPERTY, NULL);
	FILTER_DEVICE_CONTEXT->running_process = false;
}

static void change_step(indigo_device *device, double q) {
	if (q > 1) {
		indigo_send_message(device, "Drift is too slow");
		if (AGENT_GUIDER_SETTINGS_STEP_ITEM->number.value < AGENT_GUIDER_SETTINGS_STEP_ITEM->number.max) {
			AGENT_GUIDER_SETTINGS_STEP_ITEM->number.target = (AGENT_GUIDER_SETTINGS_STEP_ITEM->number.value *= q);
			indigo_update_property(device, AGENT_GUIDER_SETTINGS_PROPERTY, "Increasing calibration step to %.3g", AGENT_GUIDER_SETTINGS_STEP_ITEM->number.target);
			DEVICE_PRIVATE_DATA->phase = INDIGO_GUIDER_PHASE_INITIALIZING;
		} else {
			DEVICE_PRIVATE_DATA->phase = INDIGO_GUIDER_PHASE_FAILED;
		}
	} else {
		indigo_send_message(device, "Drift is too fast");
		if (AGENT_GUIDER_SETTINGS_STEP_ITEM->number.value < AGENT_GUIDER_SETTINGS_STEP_ITEM->number.max) {
			AGENT_GUIDER_SETTINGS_STEP_ITEM->number.target = (AGENT_GUIDER_SETTINGS_STEP_ITEM->number.value *= q);
			indigo_update_property(device, AGENT_GUIDER_SETTINGS_PROPERTY, "Decreasing calibration step to %.3g", AGENT_GUIDER_SETTINGS_STEP_ITEM->number.target);
			DEVICE_PRIVATE_DATA->phase = INDIGO_GUIDER_PHASE_INITIALIZING;
		} else {
			DEVICE_PRIVATE_DATA->phase = INDIGO_GUIDER_PHASE_FAILED;
		}
	}
	indigo_update_property(device, AGENT_GUIDER_SETTINGS_PROPERTY, NULL);
}

static bool guide_and_capture_frame(indigo_device *device, double ra, double dec, char *message) {
	write_log_record(device);
	if ((ra != 0 || dec != 0) && pulse_guide(device, ra, dec) != INDIGO_OK_STATE) {
		return false;
	}
	if (!capture_and_process_frame(device)) {
		if (DEVICE_PRIVATE_DATA->no_guiding_star) {
			if (DEVICE_PRIVATE_DATA->first_frame) {
				if (!AGENT_GUIDER_DETECTION_DONUTS_ITEM->sw.value) {
					clear_selection(device);
					if (check_selection(device)) {
						indigo_send_message(device, "Warning: Selection changed");
					}
				}
				AGENT_GUIDER_STATS_FRAME_ITEM->number.value = 0;
				DEVICE_PRIVATE_DATA->phase = INDIGO_GUIDER_PHASE_INITIALIZING;
			} else if (AGENT_GUIDER_FAIL_ON_CALIBRATION_ERROR_ITEM->sw.value) {
				DEVICE_PRIVATE_DATA->phase = INDIGO_GUIDER_PHASE_FAILED;
			} else if (AGENT_GUIDER_RESET_ON_CALIBRATION_ERROR_ITEM->sw.value) {
				DEVICE_PRIVATE_DATA->phase = INDIGO_GUIDER_PHASE_INITIALIZING;
				indigo_send_message(device, "Warning: Resetting and waiting for stars to reappear");
				DEVICE_PRIVATE_DATA->silence_warnings = true;
				clear_selection(device);
				if (AGENT_GUIDER_DETECTION_DONUTS_ITEM->sw.value) {
					while (AGENT_ABORT_PROCESS_PROPERTY->state != INDIGO_BUSY_STATE && (!capture_and_process_frame(device) || DEVICE_PRIVATE_DATA->no_guiding_star)) {
						indigo_usleep(1000000);
					}
				} else {
					while (AGENT_ABORT_PROCESS_PROPERTY->state != INDIGO_BUSY_STATE && (!capture_frame(device) || !find_stars(device) || !select_stars(device))) {
						indigo_usleep(1000000);
					}
				}
			}
		} else {
			DEVICE_PRIVATE_DATA->phase = INDIGO_GUIDER_PHASE_FAILED;
		}
		DEVICE_PRIVATE_DATA->first_frame = false;
		indigo_update_property(device, AGENT_GUIDER_STATS_PROPERTY, message);
		return false;
	}
	DEVICE_PRIVATE_DATA->first_frame = false;
	indigo_update_property(device, AGENT_GUIDER_STATS_PROPERTY, message);
	return true;
}

static bool calibrate(indigo_device *device) {
	double last_drift = 0, dec_angle = 0;
	int last_count = 0;
	DEVICE_PRIVATE_DATA->silence_warnings = false;
	AGENT_GUIDER_STATS_PHASE_ITEM->number.value = DEVICE_PRIVATE_DATA->phase = INDIGO_GUIDER_PHASE_INITIALIZING;
	AGENT_GUIDER_STATS_FRAME_ITEM->number.value = AGENT_GUIDER_STATS_FRAME_ITEM->number.value = AGENT_GUIDER_STATS_REFERENCE_X_ITEM->number.value = AGENT_GUIDER_STATS_REFERENCE_Y_ITEM->number.value = AGENT_GUIDER_STATS_DRIFT_X_ITEM->number.value = AGENT_GUIDER_STATS_DRIFT_Y_ITEM->number.value = AGENT_GUIDER_STATS_DRIFT_RA_ITEM->number.value = AGENT_GUIDER_STATS_DRIFT_DEC_ITEM->number.value = AGENT_GUIDER_STATS_DRIFT_RA_S_ITEM->number.value = AGENT_GUIDER_STATS_DRIFT_DEC_S_ITEM->number.value = AGENT_GUIDER_STATS_CORR_RA_ITEM->number.value = AGENT_GUIDER_STATS_CORR_DEC_ITEM->number.value = AGENT_GUIDER_STATS_RMSE_RA_ITEM->number.value = AGENT_GUIDER_STATS_RMSE_DEC_ITEM->number.value = AGENT_GUIDER_STATS_RMSE_RA_S_ITEM->number.value = AGENT_GUIDER_STATS_RMSE_DEC_S_ITEM->number.value = AGENT_GUIDER_STATS_SNR_ITEM->number.value = AGENT_GUIDER_STATS_DITHERING_ITEM->number.value = 0;
	AGENT_GUIDER_DITHERING_OFFSETS_X_ITEM->number.value = AGENT_GUIDER_DITHERING_OFFSETS_X_ITEM->number.target = AGENT_GUIDER_DITHERING_OFFSETS_Y_ITEM->number.value = AGENT_GUIDER_DITHERING_OFFSETS_Y_ITEM->number.target = 0;
	indigo_update_property(device, AGENT_GUIDER_STATS_PROPERTY, NULL);
	indigo_update_property(device, AGENT_GUIDER_DITHERING_OFFSETS_PROPERTY, NULL);
	indigo_send_message(device, "Calibration started");
	allow_abort_by_mount_agent(device, true);
	if (AGENT_GUIDER_ENABLE_LOGGING_FEATURE_ITEM->sw.value) {
		open_log(device);
		write_log_header(device, "calibration");
		write_log_record(device);
	}
	int upload_mode = indigo_save_switch_state(device, CCD_UPLOAD_MODE_PROPERTY_NAME, CCD_UPLOAD_MODE_CLIENT_ITEM_NAME);
	int image_format = indigo_save_switch_state(device, CCD_IMAGE_FORMAT_PROPERTY_NAME, CCD_IMAGE_FORMAT_RAW_ITEM_NAME);
	if (!AGENT_GUIDER_DETECTION_DONUTS_ITEM->sw.value) {
		check_selection(device);
	}
	DEVICE_PRIVATE_DATA->first_frame = true;
	while (AGENT_START_PROCESS_PROPERTY->state == INDIGO_BUSY_STATE) {
		if (AGENT_ABORT_PROCESS_PROPERTY->state == INDIGO_BUSY_STATE) {
			DEVICE_PRIVATE_DATA->phase = INDIGO_GUIDER_PHASE_FAILED;
		}
		AGENT_GUIDER_STATS_PHASE_ITEM->number.value = DEVICE_PRIVATE_DATA->phase;
		switch (DEVICE_PRIVATE_DATA->phase) {
			case INDIGO_GUIDER_PHASE_INITIALIZING: {
				AGENT_GUIDER_SETTINGS_ANGLE_ITEM->number.value = AGENT_GUIDER_SETTINGS_ANGLE_ITEM->number.target = AGENT_GUIDER_SETTINGS_SOP_ITEM->number.value = AGENT_GUIDER_SETTINGS_SOP_ITEM->number.target = AGENT_GUIDER_SETTINGS_BACKLASH_ITEM->number.value = AGENT_GUIDER_SETTINGS_BACKLASH_ITEM->number.target = AGENT_GUIDER_SETTINGS_SPEED_RA_ITEM->number.value = AGENT_GUIDER_SETTINGS_SPEED_RA_ITEM->number.target = AGENT_GUIDER_SETTINGS_SPEED_DEC_ITEM->number.value = AGENT_GUIDER_SETTINGS_SPEED_DEC_ITEM->number.target = AGENT_GUIDER_DITHERING_OFFSETS_X_ITEM->number.value = AGENT_GUIDER_DITHERING_OFFSETS_X_ITEM->number.target = AGENT_GUIDER_DITHERING_OFFSETS_Y_ITEM->number.value = AGENT_GUIDER_DITHERING_OFFSETS_Y_ITEM->number.target = 0;
				indigo_update_property(device, AGENT_GUIDER_SETTINGS_PROPERTY, NULL);
				indigo_update_property(device, AGENT_GUIDER_DITHERING_OFFSETS_PROPERTY, NULL);
				indigo_update_property(device, AGENT_GUIDER_STATS_PROPERTY, "Initializing calibration");
				DEVICE_PRIVATE_DATA->phase = INDIGO_GUIDER_PHASE_CLEARING_DEC;
				break;
			}
			case INDIGO_GUIDER_PHASE_CLEARING_DEC: {
				if (AGENT_GUIDER_DEC_MODE_NONE_ITEM->sw.value) {
					DEVICE_PRIVATE_DATA->phase = INDIGO_GUIDER_PHASE_CLEARING_RA;
					break;
				}
				if (AGENT_GUIDER_SETTINGS_CAL_STEPS_ITEM->number.value == 0) {
					DEVICE_PRIVATE_DATA->phase = INDIGO_GUIDER_PHASE_MOVING_NORTH;
					break;
				}
				AGENT_GUIDER_STATS_FRAME_ITEM->number.value = AGENT_GUIDER_STATS_DRIFT_X_ITEM->number.value = AGENT_GUIDER_STATS_DRIFT_Y_ITEM->number.value = 0;
				if (!guide_and_capture_frame(device, 0, 0, "Clearing DEC backlash")) {
					break;
				}
				for (int i = 0; i < AGENT_GUIDER_SETTINGS_BL_STEPS_ITEM->number.value; i++) {
					if (AGENT_ABORT_PROCESS_PROPERTY->state == INDIGO_BUSY_STATE) {
						DEVICE_PRIVATE_DATA->phase = INDIGO_GUIDER_PHASE_FAILED;
						break;
					}
					if (!guide_and_capture_frame(device, 0, AGENT_GUIDER_SETTINGS_STEP_ITEM->number.value, NULL)) {
						break;
					}
					if (DEVICE_PRIVATE_DATA->drift > AGENT_GUIDER_SETTINGS_BL_DRIFT_ITEM->number.value) {
						DEVICE_PRIVATE_DATA->phase = INDIGO_GUIDER_PHASE_CLEARING_RA;
						break;
					}
				}
				if (DEVICE_PRIVATE_DATA->phase == INDIGO_GUIDER_PHASE_CLEARING_DEC) {
					change_step(device, 2);
				}
				break;
			}
			case INDIGO_GUIDER_PHASE_CLEARING_RA: {
				if (AGENT_GUIDER_SETTINGS_CAL_STEPS_ITEM->number.value == 0) {
					DEVICE_PRIVATE_DATA->phase = INDIGO_GUIDER_PHASE_MOVING_NORTH;
					break;
				}
				AGENT_GUIDER_STATS_FRAME_ITEM->number.value = AGENT_GUIDER_STATS_DRIFT_X_ITEM->number.value = AGENT_GUIDER_STATS_DRIFT_Y_ITEM->number.value = 0;
				if (!guide_and_capture_frame(device, 0, 0, "Clearing RA backlash")) {
					break;
				}
				/* cos(87deg) = 0.05 => so 20 is ok for 87 declination */
				for (int i = 0; i < AGENT_GUIDER_SETTINGS_BL_STEPS_ITEM->number.value * 20; i++) {
					if (AGENT_ABORT_PROCESS_PROPERTY->state == INDIGO_BUSY_STATE) {
						DEVICE_PRIVATE_DATA->phase = INDIGO_GUIDER_PHASE_FAILED;
						break;
					}
					if (!guide_and_capture_frame(device, AGENT_GUIDER_SETTINGS_STEP_ITEM->number.value, 0, NULL)) {
						break;
					}
					indigo_update_property(device, AGENT_GUIDER_STATS_PROPERTY, NULL);
					if (DEVICE_PRIVATE_DATA->drift > AGENT_GUIDER_SETTINGS_BL_DRIFT_ITEM->number.value) {
						indigo_send_message(device, "Backlash cleared");
						DEVICE_PRIVATE_DATA->phase = INDIGO_GUIDER_PHASE_MOVING_NORTH;
						break;
					}
				}
				if (DEVICE_PRIVATE_DATA->phase == INDIGO_GUIDER_PHASE_CLEARING_RA) {
					change_step(device, 2);
				}
				break;
			}
			case INDIGO_GUIDER_PHASE_MOVING_NORTH: {
				if (AGENT_GUIDER_DEC_MODE_NONE_ITEM->sw.value) {
					DEVICE_PRIVATE_DATA->phase = INDIGO_GUIDER_PHASE_MOVING_WEST;
					break;
				}
				AGENT_GUIDER_STATS_FRAME_ITEM->number.value = AGENT_GUIDER_STATS_DRIFT_X_ITEM->number.value = AGENT_GUIDER_STATS_DRIFT_Y_ITEM->number.value = 0;
				if (!guide_and_capture_frame(device, 0, 0, "Moving north")) {
					break;
				}
				for (int i = 0; i < AGENT_GUIDER_SETTINGS_CAL_STEPS_ITEM->number.value; i++) {
					if (AGENT_ABORT_PROCESS_PROPERTY->state == INDIGO_BUSY_STATE) {
						DEVICE_PRIVATE_DATA->phase = INDIGO_GUIDER_PHASE_FAILED;
						break;
					}
					if (!guide_and_capture_frame(device, 0, AGENT_GUIDER_SETTINGS_STEP_ITEM->number.value, NULL)) {
						break;
					}
					if (DEVICE_PRIVATE_DATA->drift > AGENT_GUIDER_SETTINGS_CAL_DRIFT_ITEM->number.value) {
						if (i < AGENT_GUIDER_SETTINGS_CAL_STEPS_ITEM->number.value / 5) {
							change_step(device, 0.5);
							break;
						} else {
							last_drift = DEVICE_PRIVATE_DATA->drift;
							dec_angle = atan2(-AGENT_GUIDER_STATS_DRIFT_Y_ITEM->number.value, AGENT_GUIDER_STATS_DRIFT_X_ITEM->number.value);
							AGENT_GUIDER_SETTINGS_ANGLE_ITEM->number.value = AGENT_GUIDER_SETTINGS_ANGLE_ITEM->number.target = round(180 * dec_angle / PI);
							AGENT_GUIDER_SETTINGS_SPEED_DEC_ITEM->number.value = AGENT_GUIDER_SETTINGS_SPEED_DEC_ITEM->number.target = round(1000 * last_drift / (i * AGENT_GUIDER_SETTINGS_STEP_ITEM->number.value)) / 1000;
							indigo_update_property(device, AGENT_GUIDER_SETTINGS_PROPERTY, NULL);
							last_count = i;
							if (AGENT_GUIDER_SETTINGS_SPEED_DEC_ITEM->number.value == 0) {
								indigo_send_message(device, "DEC speed is 0 px/\"");
								DEVICE_PRIVATE_DATA->phase = INDIGO_GUIDER_PHASE_FAILED;
							} else {
								DEVICE_PRIVATE_DATA->phase = INDIGO_GUIDER_PHASE_MOVING_SOUTH;
							}
							break;
						}
					}
				}
				if (DEVICE_PRIVATE_DATA->phase == INDIGO_GUIDER_PHASE_MOVING_NORTH) {
					change_step(device, 2);
				}
				break;
			}
			case INDIGO_GUIDER_PHASE_MOVING_SOUTH: {
				AGENT_GUIDER_STATS_FRAME_ITEM->number.value = AGENT_GUIDER_STATS_DRIFT_X_ITEM->number.value = AGENT_GUIDER_STATS_DRIFT_Y_ITEM->number.value = 0;
				if (!guide_and_capture_frame(device, 0, 0, "Moving south")) {
					break;
				}
				for (int i = 0; i <= last_count; i++) {
					if (AGENT_ABORT_PROCESS_PROPERTY->state == INDIGO_BUSY_STATE) {
						DEVICE_PRIVATE_DATA->phase = INDIGO_GUIDER_PHASE_FAILED;
						break;
					}
					if (!guide_and_capture_frame(device, 0, -AGENT_GUIDER_SETTINGS_STEP_ITEM->number.value, NULL)) {
						break;
					}
				}
				if (DEVICE_PRIVATE_DATA->phase == INDIGO_GUIDER_PHASE_MOVING_SOUTH) {
					/* allow for 1 step measurement error  */
					if (DEVICE_PRIVATE_DATA->drift < last_drift + last_drift / last_count) {
						double backlash = round(1000 * (last_drift - DEVICE_PRIVATE_DATA->drift)) / 1000;
						if (backlash < 0) {
							indigo_error("Warning: Negative backlash %.3fpx, set to 0", backlash);
							backlash = 0;
						}
						AGENT_GUIDER_SETTINGS_BACKLASH_ITEM->number.value = AGENT_GUIDER_SETTINGS_BACKLASH_ITEM->number.target = backlash;
					} else {
						AGENT_GUIDER_SETTINGS_BACKLASH_ITEM->number.value = AGENT_GUIDER_SETTINGS_BACKLASH_ITEM->number.target = 0;
						indigo_send_message(device, "Warning: Inconsitent backlash");
					}
					indigo_update_property(device, AGENT_GUIDER_SETTINGS_PROPERTY, NULL);
					DEVICE_PRIVATE_DATA->phase = INDIGO_GUIDER_PHASE_MOVING_WEST;
				}
				break;
			}
			case INDIGO_GUIDER_PHASE_MOVING_WEST: {
				AGENT_GUIDER_STATS_FRAME_ITEM->number.value = AGENT_GUIDER_STATS_DRIFT_X_ITEM->number.value = AGENT_GUIDER_STATS_DRIFT_Y_ITEM->number.value = 0;
				if (!guide_and_capture_frame(device, 0, 0, "Moving west")) {
					break;
				}
				/* we need more iterations closer to the pole */
				for (int i = 0; i < AGENT_GUIDER_SETTINGS_CAL_STEPS_ITEM->number.value * 5; i++) {
					if (AGENT_ABORT_PROCESS_PROPERTY->state == INDIGO_BUSY_STATE) {
						DEVICE_PRIVATE_DATA->phase = INDIGO_GUIDER_PHASE_FAILED;
						break;
					}
					if (!guide_and_capture_frame(device, AGENT_GUIDER_SETTINGS_STEP_ITEM->number.value, 0, NULL)) {
						break;
					}
					/* even if we do not reach AGENT_GUIDER_SETTINGS_CAL_DRIFT it is ok. We are close enough to te pole and tracking is less sensitive to RA errors */
					if (DEVICE_PRIVATE_DATA->drift > AGENT_GUIDER_SETTINGS_CAL_DRIFT_ITEM->number.value || i+1 >= AGENT_GUIDER_SETTINGS_CAL_STEPS_ITEM->number.value * 5) {
						last_drift = DEVICE_PRIVATE_DATA->drift;
						double ra_angle = atan2(-AGENT_GUIDER_STATS_DRIFT_Y_ITEM->number.value, AGENT_GUIDER_STATS_DRIFT_X_ITEM->number.value);
						if (!AGENT_GUIDER_DEC_MODE_NONE_ITEM->sw.value) {
							double dif_p = PI - fabs(fabs(ra_angle - dec_angle + PI2) - PI);
							double dif_m = PI - fabs(fabs(ra_angle - dec_angle - PI2) - PI);
							//indigo_send_message(device, "ra angle = %g, dec angle = %g, dif_p = %g, dif_m = %g", 180.0 * ra_angle / M_PI, 180.0 * dec_angle / M_PI, dif_p, dif_m);
							if (dif_p < dif_m) {
								dec_angle -= PI2;
							} else {
								dec_angle += PI2;
								AGENT_GUIDER_SETTINGS_SPEED_DEC_ITEM->number.value = AGENT_GUIDER_SETTINGS_SPEED_DEC_ITEM->number.target = -AGENT_GUIDER_SETTINGS_SPEED_DEC_ITEM->number.target;
							}
							AGENT_GUIDER_SETTINGS_ANGLE_ITEM->number.value = AGENT_GUIDER_SETTINGS_ANGLE_ITEM->number.target = round(180 * atan2((sin(dec_angle) + sin(ra_angle)) / 2, (cos(dec_angle) + cos(ra_angle)) / 2) / PI);
						} else {
							AGENT_GUIDER_SETTINGS_ANGLE_ITEM->number.value = AGENT_GUIDER_SETTINGS_ANGLE_ITEM->number.target = round(180 * atan2(sin(ra_angle), cos(ra_angle)) / PI);
						}
						AGENT_GUIDER_SETTINGS_SPEED_RA_ITEM->number.value = AGENT_GUIDER_SETTINGS_SPEED_RA_ITEM->number.target = round(1000 * last_drift / (i * AGENT_GUIDER_SETTINGS_STEP_ITEM->number.value)) / 1000;
						indigo_update_property(device, AGENT_GUIDER_SETTINGS_PROPERTY, NULL);
						last_count = i;
						if (fabs(AGENT_GUIDER_SETTINGS_SPEED_RA_ITEM->number.value) < 0.1) {
							indigo_send_message(device, "RA drift speed is too slow");
							DEVICE_PRIVATE_DATA->phase = INDIGO_GUIDER_PHASE_FAILED;
						} else {
							DEVICE_PRIVATE_DATA->phase = INDIGO_GUIDER_PHASE_MOVING_EAST;
						}
						break;
					}
				}
				break;
			}
			case INDIGO_GUIDER_PHASE_MOVING_EAST: {
				AGENT_GUIDER_STATS_FRAME_ITEM->number.value = AGENT_GUIDER_STATS_DRIFT_X_ITEM->number.value = AGENT_GUIDER_STATS_DRIFT_Y_ITEM->number.value = 0;
				if (!guide_and_capture_frame(device, 0, 0, "Moving east")) {
					break;
				}
				for (int i = 0; i <= last_count; i++) {
					if (AGENT_ABORT_PROCESS_PROPERTY->state == INDIGO_BUSY_STATE) {
						DEVICE_PRIVATE_DATA->phase = INDIGO_GUIDER_PHASE_FAILED;
						break;
					}
					if (!guide_and_capture_frame(device, -AGENT_GUIDER_SETTINGS_STEP_ITEM->number.value, 0, NULL)) {
						break;
					}
					indigo_update_property(device, AGENT_GUIDER_STATS_PROPERTY, NULL);
				}
				if (DEVICE_PRIVATE_DATA->phase == INDIGO_GUIDER_PHASE_MOVING_EAST) {
					/* Large periodoc error like in harmonic drive mounts can result in RA speed over or under estimation.
					 The average of West and East measured speeds should be the precise speed.
					 */
					last_drift = DEVICE_PRIVATE_DATA->drift;
					double east_speed = round(1000 * last_drift / (last_count * AGENT_GUIDER_SETTINGS_STEP_ITEM->number.value)) / 1000;
					/* West speed is already stored in AGENT_GUIDER_SETTINGS_SPEED_RA_ITEM */
					INDIGO_DRIVER_DEBUG(DRIVER_NAME, "RA speeds: West = %.3lf px/sec, East = %.3lf px/sec", AGENT_GUIDER_SETTINGS_SPEED_RA_ITEM->number.value, east_speed);
					AGENT_GUIDER_SETTINGS_SPEED_RA_ITEM->number.value = AGENT_GUIDER_SETTINGS_SPEED_RA_ITEM->number.target = (east_speed + AGENT_GUIDER_SETTINGS_SPEED_RA_ITEM->number.value) / 2;
					
					INDIGO_DRIVER_DEBUG(DRIVER_NAME, "RA speed (without PE) = %.3lf px/sec", AGENT_GUIDER_SETTINGS_SPEED_RA_ITEM->number.value);
					indigo_update_property(device, AGENT_GUIDER_SETTINGS_PROPERTY, NULL);
					DEVICE_PRIVATE_DATA->phase = INDIGO_GUIDER_PHASE_DONE;
				}
				break;
			}
			case INDIGO_GUIDER_PHASE_FAILED: {
				indigo_send_message(device, "Calibration failed");
				indigo_update_property(device, AGENT_GUIDER_STATS_PROPERTY, NULL);
				AGENT_START_PROCESS_PROPERTY->state = INDIGO_ALERT_STATE;
				break;
			}
			case INDIGO_GUIDER_PHASE_DONE: {
				double pole_distance = 90 - fabs(AGENT_GUIDER_MOUNT_COORDINATES_DEC_ITEM->number.value);
				/* Declination is > 85deg or < -85deg */
				if (pole_distance < 5) {
					indigo_send_message(device, "Pole distance %.1f°. RA calibration may be off", pole_distance);
				}
				if (DEVICE_PRIVATE_DATA->cos_dec == 0) {
					DEVICE_PRIVATE_DATA->cos_dec = MIN_COS_DEC;
				}
				INDIGO_DRIVER_DEBUG(DRIVER_NAME, "Calculated RA speed = %.3f, RA speed at equator = %.3f (cos_dec = %.3f)", AGENT_GUIDER_SETTINGS_SPEED_RA_ITEM->number.value, AGENT_GUIDER_SETTINGS_SPEED_RA_ITEM->number.value / DEVICE_PRIVATE_DATA->cos_dec, DEVICE_PRIVATE_DATA->cos_dec);
				AGENT_GUIDER_SETTINGS_SPEED_RA_ITEM->number.value =
				AGENT_GUIDER_SETTINGS_SPEED_RA_ITEM->number.target = AGENT_GUIDER_SETTINGS_SPEED_RA_ITEM->number.value / DEVICE_PRIVATE_DATA->cos_dec;
				AGENT_GUIDER_SETTINGS_SOP_ITEM->number.value =
				AGENT_GUIDER_SETTINGS_SOP_ITEM->number.target = AGENT_GUIDER_MOUNT_COORDINATES_SOP_ITEM->number.value;
				indigo_update_property(device, AGENT_GUIDER_SETTINGS_PROPERTY, NULL);
				indigo_send_message(device, "Calibration complete");
				indigo_update_property(device, AGENT_GUIDER_STATS_PROPERTY, NULL);
				save_config(device);
				AGENT_START_PROCESS_PROPERTY->state = INDIGO_OK_STATE;
				break;
			}
			default:
				AGENT_START_PROCESS_PROPERTY->state = INDIGO_ALERT_STATE;
				break;
		}
	}
	indigo_restore_switch_state(device, CCD_UPLOAD_MODE_PROPERTY_NAME, upload_mode);
	indigo_restore_switch_state(device, CCD_IMAGE_FORMAT_PROPERTY_NAME, image_format);
	bool result = true;
	if (AGENT_ABORT_PROCESS_PROPERTY->state == INDIGO_BUSY_STATE) {
		AGENT_ABORT_PROCESS_PROPERTY->state = INDIGO_OK_STATE;
		indigo_update_property(device, AGENT_ABORT_PROCESS_PROPERTY, NULL);
		result = false;
	}
	indigo_update_property(device, AGENT_GUIDER_STATS_PROPERTY, NULL);
	AGENT_GUIDER_START_CALIBRATION_ITEM->sw.value = AGENT_GUIDER_START_CALIBRATION_AND_GUIDING_ITEM->sw.value = false;
	indigo_update_property(device, AGENT_START_PROCESS_PROPERTY, NULL);
	close_log(device);
	allow_abort_by_mount_agent(device, false);
	return result;
}

static bool guide(indigo_device *device) {
	if (AGENT_GUIDER_SETTINGS_SPEED_RA_ITEM->number.value == 0 || fabs(AGENT_GUIDER_MOUNT_COORDINATES_DEC_ITEM->number.value) > 89) {
		AGENT_START_PROCESS_PROPERTY->state = INDIGO_ALERT_STATE;
		AGENT_GUIDER_START_CALIBRATION_AND_GUIDING_ITEM->sw.value = AGENT_GUIDER_START_GUIDING_ITEM->sw.value = false;
		indigo_update_property(device, AGENT_START_PROCESS_PROPERTY, NULL);
		if (AGENT_GUIDER_SETTINGS_SPEED_RA_ITEM->number.value == 0) {
			indigo_send_message(device, "Guiding failed (not calibrated)");
		} else {
			indigo_send_message(device, "Guiding failed (too close to the pole)");
		}
		return false;
	}
	indigo_send_message(device, "Guiding started");
	FILTER_DEVICE_CONTEXT->running_process = true;
	DEVICE_PRIVATE_DATA->silence_warnings = false;
	AGENT_GUIDER_STATS_PHASE_ITEM->number.value = INDIGO_GUIDER_PHASE_GUIDING;
	AGENT_GUIDER_STATS_FRAME_ITEM->number.value = AGENT_GUIDER_STATS_REFERENCE_X_ITEM->number.value = AGENT_GUIDER_STATS_REFERENCE_Y_ITEM->number.value = AGENT_GUIDER_STATS_DRIFT_X_ITEM->number.value = AGENT_GUIDER_STATS_DRIFT_Y_ITEM->number.value = AGENT_GUIDER_STATS_DRIFT_RA_ITEM->number.value = AGENT_GUIDER_STATS_DRIFT_DEC_ITEM->number.value = AGENT_GUIDER_STATS_DRIFT_RA_S_ITEM->number.value = AGENT_GUIDER_STATS_DRIFT_DEC_S_ITEM->number.value = AGENT_GUIDER_STATS_CORR_RA_ITEM->number.value = AGENT_GUIDER_STATS_CORR_DEC_ITEM->number.value = AGENT_GUIDER_STATS_RMSE_RA_ITEM->number.value = AGENT_GUIDER_STATS_RMSE_DEC_ITEM->number.value = AGENT_GUIDER_STATS_RMSE_RA_S_ITEM->number.value = AGENT_GUIDER_STATS_RMSE_DEC_S_ITEM->number.value = AGENT_GUIDER_STATS_SNR_ITEM->number.value = AGENT_GUIDER_STATS_DITHERING_ITEM->number.value = 0;
	indigo_update_property(device, AGENT_GUIDER_STATS_PROPERTY, NULL);
	DEVICE_PRIVATE_DATA->rmse_ra_threshold = DEVICE_PRIVATE_DATA->rmse_dec_threshold = 0;
	allow_abort_by_mount_agent(device, true);
	int upload_mode = indigo_save_switch_state(device, CCD_UPLOAD_MODE_PROPERTY_NAME, CCD_UPLOAD_MODE_CLIENT_ITEM_NAME);
	int image_format = indigo_save_switch_state(device, CCD_IMAGE_FORMAT_PROPERTY_NAME, CCD_IMAGE_FORMAT_RAW_ITEM_NAME);
	if (!AGENT_GUIDER_DETECTION_DONUTS_ITEM->sw.value) {
		check_selection(device);
		DEVICE_PRIVATE_DATA->stars_used_at_start = usable_star_count(device);
	}
	double prev_correction_dec = 0;
	indigo_update_property(device, AGENT_GUIDER_STATS_PROPERTY, NULL);
	AGENT_GUIDER_DITHERING_OFFSETS_X_ITEM->number.value = AGENT_GUIDER_DITHERING_OFFSETS_X_ITEM->number.target = AGENT_GUIDER_DITHERING_OFFSETS_Y_ITEM->number.value = AGENT_GUIDER_DITHERING_OFFSETS_Y_ITEM->number.target = 0;
	indigo_update_property(device, AGENT_GUIDER_DITHERING_OFFSETS_PROPERTY, NULL);
	DEVICE_PRIVATE_DATA->rmse_ra_sum = DEVICE_PRIVATE_DATA->rmse_dec_sum = DEVICE_PRIVATE_DATA->rmse_ra_s_sum = DEVICE_PRIVATE_DATA->rmse_dec_s_sum = DEVICE_PRIVATE_DATA->rmse_count = 0;
	if (AGENT_GUIDER_ENABLE_LOGGING_FEATURE_ITEM->sw.value) {
		open_log(device);
		write_log_header(device, "guiding");
		write_log_record(device);
	}
	DEVICE_PRIVATE_DATA->first_frame = true;
	while (AGENT_START_PROCESS_PROPERTY->state == INDIGO_BUSY_STATE) {
		if (AGENT_ABORT_PROCESS_PROPERTY->state == INDIGO_BUSY_STATE) {
			break;
		}
		if (!capture_and_process_frame(device)) {
			if (DEVICE_PRIVATE_DATA->no_guiding_star) {
				if (DEVICE_PRIVATE_DATA->first_frame) {
					AGENT_GUIDER_STATS_FRAME_ITEM->number.value = 0;
					DEVICE_PRIVATE_DATA->first_frame = false;
					DEVICE_PRIVATE_DATA->silence_warnings = true;
					if (!AGENT_GUIDER_DETECTION_DONUTS_ITEM->sw.value) {
						clear_selection(device);
						if (!find_stars(device) || !select_stars(device)) {
							indigo_send_message(device, "Error: No guide stars found");
							break;
						}
						DEVICE_PRIVATE_DATA->stars_used_at_start = usable_star_count(device);
					} else {
						break;
					}
				} else if (AGENT_GUIDER_FAIL_ON_GUIDING_ERROR_ITEM->sw.value) {
					indigo_update_property(device, AGENT_GUIDER_STATS_PROPERTY, NULL);
					break;
				} else if (AGENT_GUIDER_CONTINUE_ON_GUIDING_ERROR_ITEM->sw.value) {
					if (!DEVICE_PRIVATE_DATA->silence_warnings) {
						indigo_send_message(device, "Warning: Pausing and waiting for stars to reappear");
					}
					indigo_usleep(1000000);
					DEVICE_PRIVATE_DATA->silence_warnings = true;
				} else if (AGENT_GUIDER_RESET_ON_GUIDING_ERROR_ITEM->sw.value) {
					DEVICE_PRIVATE_DATA->phase = INDIGO_GUIDER_PHASE_INITIALIZING;
					if (AGENT_GUIDER_DETECTION_DONUTS_ITEM->sw.value) {
						if (!DEVICE_PRIVATE_DATA->silence_warnings) {
							indigo_send_message(device, "Warning: Resetting and waiting for stars to reappear");
						}
						DEVICE_PRIVATE_DATA->silence_warnings = true;
						while (AGENT_ABORT_PROCESS_PROPERTY->state != INDIGO_BUSY_STATE && (!capture_and_process_frame(device) || DEVICE_PRIVATE_DATA->no_guiding_star)) {
							indigo_usleep(1000000);
						}
					} else {
						int min_usable_stars = 1;
						if (AGENT_GUIDER_RESET_ON_GUIDING_ERROR_WAIT_ALL_STARS_ITEM->sw.value) {
							min_usable_stars = DEVICE_PRIVATE_DATA->stars_used_at_start;
						}
						if (!DEVICE_PRIVATE_DATA->silence_warnings) {
							indigo_send_message(device, "Warning: Resetting and waiting for %d %s to reappear", min_usable_stars, min_usable_stars == 1 ? "star" : "stars");
						}
						restore_subframe(device);
						clear_selection(device);
						DEVICE_PRIVATE_DATA->silence_warnings = true;
						while (AGENT_ABORT_PROCESS_PROPERTY->state != INDIGO_BUSY_STATE && (!capture_frame(device) || !find_stars(device) || min_usable_stars > select_stars(device))) {
							indigo_usleep(1000000);
						}
					}
					AGENT_GUIDER_STATS_FRAME_ITEM->number.value = 0;
				}
				indigo_update_property(device, AGENT_GUIDER_STATS_PROPERTY, NULL);
				continue;
			} else {
				indigo_update_property(device, AGENT_GUIDER_STATS_PROPERTY, NULL);
				break;
			}
		} else {
			if (AGENT_GUIDER_STATS_FRAME_ITEM->number.value == 1 && !DEVICE_PRIVATE_DATA->autosubframing && AGENT_GUIDER_SELECTION_STAR_COUNT_ITEM->number.value == 1) {
				if (!AGENT_GUIDER_DETECTION_DONUTS_ITEM->sw.value) {
					if (select_subframe(device)) {
						AGENT_GUIDER_STATS_FRAME_ITEM->number.value = 0;
					}
				}
			}
		}
		DEVICE_PRIVATE_DATA->first_frame = false;
		if (DEVICE_PRIVATE_DATA->silence_warnings) {
			indigo_send_message(device, "Warning: Guiding recovered");
			DEVICE_PRIVATE_DATA->silence_warnings = false;
		}
		if (DEVICE_PRIVATE_DATA->drift_x || DEVICE_PRIVATE_DATA->drift_y) {
			double angle = -PI * get_rotation_angle(device) / 180;
			double sin_angle = sin(angle);
			double cos_angle = cos(angle);
			double pix_scale_x = CCD_LENS_FOV_PIXEL_SCALE_WIDTH_ITEM->number.value * 3600;
			double pix_scale_y = CCD_LENS_FOV_PIXEL_SCALE_HEIGHT_ITEM->number.value * 3600;
			double min_error = AGENT_GUIDER_SETTINGS_MIN_ERR_ITEM->number.value;
			double min_pulse = AGENT_GUIDER_SETTINGS_MIN_PULSE_ITEM->number.value;
			double max_pulse = AGENT_GUIDER_SETTINGS_MAX_PULSE_ITEM->number.value;
			double drift_ra = DEVICE_PRIVATE_DATA->drift_x * cos_angle + DEVICE_PRIVATE_DATA->drift_y * sin_angle;
			double drift_dec = DEVICE_PRIVATE_DATA->drift_x * sin_angle - DEVICE_PRIVATE_DATA->drift_y * cos_angle;
			double drift_ra_s = DEVICE_PRIVATE_DATA->drift_x * cos_angle * pix_scale_x + DEVICE_PRIVATE_DATA->drift_y * sin_angle * pix_scale_y;
			double drift_dec_s = DEVICE_PRIVATE_DATA->drift_x * sin_angle * pix_scale_x - DEVICE_PRIVATE_DATA->drift_y * cos_angle * pix_scale_y;
			double avg_drift_ra = DEVICE_PRIVATE_DATA->avg_drift_x * cos_angle + DEVICE_PRIVATE_DATA->avg_drift_y * sin_angle;
			double avg_drift_dec = DEVICE_PRIVATE_DATA->avg_drift_x * sin_angle - DEVICE_PRIVATE_DATA->avg_drift_y * cos_angle;
			AGENT_GUIDER_STATS_DRIFT_RA_ITEM->number.value = round(1000 * drift_ra) / 1000;
			AGENT_GUIDER_STATS_DRIFT_DEC_ITEM->number.value = round(1000 * drift_dec) / 1000;
			AGENT_GUIDER_STATS_DRIFT_RA_S_ITEM->number.value = round(1000 * drift_ra_s) / 1000;
			AGENT_GUIDER_STATS_DRIFT_DEC_S_ITEM->number.value = round(1000 * drift_dec_s) / 1000;
			double correction_ra = 0, correction_dec = 0;
			double max_safe_correction = AGENT_GUIDER_SELECTION_RADIUS_ITEM->number.value * SAFE_RADIUS_FACTOR;
			if (fabs(drift_ra) > min_error) {
				correction_ra = indigo_guider_reponse(AGENT_GUIDER_SETTINGS_AGG_RA_ITEM->number.value / 100, AGENT_GUIDER_SETTINGS_I_GAIN_RA_ITEM->number.value, AGENT_GUIDER_SETTINGS_EXPOSURE_ITEM->number.value + AGENT_GUIDER_SETTINGS_DELAY_ITEM->number.value, drift_ra, avg_drift_ra);
				/* Limit correction_ra, so that we will not lose the stars in the slection if we apply it and let the next cycle complete complete it */
				if ((AGENT_GUIDER_DETECTION_SELECTION_ITEM->sw.value || AGENT_GUIDER_DETECTION_WEIGHTED_SELECTION_ITEM->sw.value) && (fabs(correction_ra) > max_safe_correction)) {
					INDIGO_DRIVER_DEBUG(DRIVER_NAME, "RA correction = %.4fpx will lose stars with radius = %.2fpx, reduced RA correction = %.4fpx", correction_ra, AGENT_GUIDER_SELECTION_RADIUS_ITEM->number.value, copysign(1.0, correction_ra) * max_safe_correction);
					correction_ra = copysign(1.0, correction_ra) * max_safe_correction;
				}
				double cos_dec = (DEVICE_PRIVATE_DATA->cos_dec > MIN_COS_DEC) ? DEVICE_PRIVATE_DATA->cos_dec : MIN_COS_DEC;
				correction_ra = correction_ra / (AGENT_GUIDER_SETTINGS_SPEED_RA_ITEM->number.value * cos_dec);
				if (correction_ra > max_pulse) {
					correction_ra = max_pulse;
				} else if (correction_ra < -max_pulse) {
					correction_ra = -max_pulse;
				} else if (fabs(correction_ra) < min_pulse) {
					correction_ra = 0;
				}
			}
			if (fabs(drift_dec) > min_error) {
				correction_dec = indigo_guider_reponse(AGENT_GUIDER_SETTINGS_AGG_DEC_ITEM->number.value / 100, AGENT_GUIDER_SETTINGS_I_GAIN_DEC_ITEM->number.value, AGENT_GUIDER_SETTINGS_EXPOSURE_ITEM->number.value + AGENT_GUIDER_SETTINGS_DELAY_ITEM->number.value, drift_dec, avg_drift_dec);
				/* Limit correction_dec, so that we will not lose the stars in the slection if we apply it and let the next cycle complete complete it */
				if ((AGENT_GUIDER_DETECTION_SELECTION_ITEM->sw.value || AGENT_GUIDER_DETECTION_WEIGHTED_SELECTION_ITEM->sw.value) && (fabs(correction_dec) > max_safe_correction)) {
					INDIGO_DRIVER_DEBUG(DRIVER_NAME, "Dec correction = %.4fpx will lose stars with radius = %.2fpx, reduced Dec correction = %.4fpx", correction_dec, AGENT_GUIDER_SELECTION_RADIUS_ITEM->number.value, copysign(1.0, correction_dec) * max_safe_correction);
					correction_dec = copysign(1.0, correction_dec) * max_safe_correction;
				}
				correction_dec = correction_dec / get_dec_speed(device);
				if (correction_dec > max_pulse) {
					correction_dec = max_pulse;
				} else if (correction_dec < -max_pulse) {
					correction_dec = -max_pulse;
				} else if (fabs(correction_dec) < min_pulse) {
					correction_dec = 0;
				}
			}
			if (AGENT_GUIDER_DEC_MODE_NONE_ITEM->sw.value) {
				correction_dec = 0;
			} else if (AGENT_GUIDER_DEC_MODE_NORTH_ITEM->sw.value && correction_dec < 0) {
				correction_dec = 0;
			} else if (AGENT_GUIDER_DEC_MODE_SOUTH_ITEM->sw.value && correction_dec > 0) {
				correction_dec = 0;
			}
			AGENT_GUIDER_STATS_CORR_RA_ITEM->number.value = round(1000 * correction_ra) / 1000;
			AGENT_GUIDER_STATS_CORR_DEC_ITEM->number.value = round(1000 * correction_dec) / 1000;
			/* Apply DEC backlash. It is after AGENT_GUIDER_STATS_CORR_DEC_ITEM asignment, so that it will not show on the correction graph. */
			if (AGENT_GUIDER_APPLY_DEC_BACKLASH_ENABLED_ITEM->sw.value) {
				if ((prev_correction_dec <= 0 && correction_dec <= 0) || (prev_correction_dec >= 0 && correction_dec >= 0)) {
					INDIGO_DRIVER_DEBUG(DRIVER_NAME, "(-) No Dec backlash appled: prev_correction_dec = %.3fs, correction_dec = %.3fs", prev_correction_dec, correction_dec);
				} else {
					double backlash = fabs(AGENT_GUIDER_SETTINGS_BACKLASH_ITEM->number.value / AGENT_GUIDER_SETTINGS_SPEED_DEC_ITEM->number.value);
					INDIGO_DRIVER_DEBUG(DRIVER_NAME, "(+) Dec backlash appled: prev_correction_dec = %.3fs, correction_dec = %.3fs, backlash = %.3fs", prev_correction_dec, correction_dec, backlash);
					/* apply backlash only if correction_dec != 0 (+0 or -0 are excluded too) */
					if (correction_dec > 0) {
						correction_dec += backlash;
					} else if (correction_dec < 0) {
						correction_dec -= backlash;
					}
					INDIGO_DRIVER_DEBUG(DRIVER_NAME, "(+) correction_dec + backlash = %.3fs", correction_dec);
				}
			}
			/* save current dec corrction as previous dec correction only if it will be aplied */
			/* It is saved regardless if BL is apllied or not because we need to be able to turn BL on and off any time */
			if (fabs(correction_dec) > 0) {
				prev_correction_dec = correction_dec;
			}
			if (pulse_guide(device, correction_ra, correction_dec) != INDIGO_OK_STATE) {
				AGENT_START_PROCESS_PROPERTY->state = AGENT_START_PROCESS_PROPERTY->state == INDIGO_OK_STATE ? INDIGO_OK_STATE : INDIGO_ALERT_STATE;
				break;
			}
			if (AGENT_GUIDER_STATS_DITHERING_ITEM->number.value == 0) {
				DEVICE_PRIVATE_DATA->rmse_ra_sum += drift_ra * drift_ra;
				DEVICE_PRIVATE_DATA->rmse_dec_sum += drift_dec * drift_dec;
				DEVICE_PRIVATE_DATA->rmse_ra_s_sum += drift_ra_s * drift_ra_s;
				DEVICE_PRIVATE_DATA->rmse_dec_s_sum += drift_dec_s * drift_dec_s;
				DEVICE_PRIVATE_DATA->rmse_count++;
			} else {
				DEVICE_PRIVATE_DATA->rmse_ra_sum = DEVICE_PRIVATE_DATA->rmse_dec_sum = DEVICE_PRIVATE_DATA->rmse_ra_s_sum = DEVICE_PRIVATE_DATA->rmse_dec_s_sum = 0;
				if (DEVICE_PRIVATE_DATA->rmse_count < AGENT_GUIDER_SETTINGS_DITH_LIMIT_ITEM->number.value) {
					DEVICE_PRIVATE_DATA->rmse_count++;
				}
				int count = (int)DEVICE_PRIVATE_DATA->rmse_count;
				if (count > MAX_STACK) {
					count = MAX_STACK;
				}
				for (int i = 0; i < count; i++) {
					double drift_ra_i =  DEVICE_PRIVATE_DATA->stack_x[i] * cos_angle + DEVICE_PRIVATE_DATA->stack_y[i] * sin_angle;
					double drift_dec_i = DEVICE_PRIVATE_DATA->stack_x[i] * sin_angle - DEVICE_PRIVATE_DATA->stack_y[i] * cos_angle;
					DEVICE_PRIVATE_DATA->rmse_ra_sum += drift_ra_i * drift_ra_i;
					DEVICE_PRIVATE_DATA->rmse_dec_sum += drift_dec_i * drift_dec_i;
					double drift_ra_s_i = DEVICE_PRIVATE_DATA->stack_x[i] * cos_angle * pix_scale_x + DEVICE_PRIVATE_DATA->stack_y[i] * sin_angle * pix_scale_y;
					double drift_dec_s_i = DEVICE_PRIVATE_DATA->stack_x[i] * sin_angle * pix_scale_x - DEVICE_PRIVATE_DATA->stack_y[i] * cos_angle * pix_scale_y;
					DEVICE_PRIVATE_DATA->rmse_ra_s_sum += drift_ra_s_i * drift_ra_s_i;
					DEVICE_PRIVATE_DATA->rmse_dec_s_sum += drift_dec_s_i * drift_dec_s_i;
				}
			}
			AGENT_GUIDER_STATS_RMSE_RA_ITEM->number.value = round(1000 * sqrt(DEVICE_PRIVATE_DATA->rmse_ra_sum / DEVICE_PRIVATE_DATA->rmse_count)) / 1000;
			AGENT_GUIDER_STATS_RMSE_DEC_ITEM->number.value = round(1000 * sqrt(DEVICE_PRIVATE_DATA->rmse_dec_sum / DEVICE_PRIVATE_DATA->rmse_count)) / 1000;
			AGENT_GUIDER_STATS_RMSE_RA_S_ITEM->number.value = round(1000 * sqrt(DEVICE_PRIVATE_DATA->rmse_ra_s_sum / DEVICE_PRIVATE_DATA->rmse_count)) / 1000;
			AGENT_GUIDER_STATS_RMSE_DEC_S_ITEM->number.value = round(1000 * sqrt(DEVICE_PRIVATE_DATA->rmse_dec_s_sum / DEVICE_PRIVATE_DATA->rmse_count)) / 1000;
			if (AGENT_GUIDER_STATS_DITHERING_ITEM->number.value != 0) {
				bool dithering_finished = false;
				if (AGENT_GUIDER_DEC_MODE_BOTH_ITEM->sw.value) {
					if (DEVICE_PRIVATE_DATA->rmse_ra_threshold > 0 && DEVICE_PRIVATE_DATA->rmse_dec_threshold > 0) {
						dithering_finished = DEVICE_PRIVATE_DATA->rmse_count >= AGENT_GUIDER_SETTINGS_DITH_LIMIT_ITEM->number.value && AGENT_GUIDER_STATS_RMSE_RA_ITEM->number.value < DEVICE_PRIVATE_DATA->rmse_ra_threshold && AGENT_GUIDER_STATS_RMSE_DEC_ITEM->number.value < DEVICE_PRIVATE_DATA->rmse_dec_threshold;
					} else {
						dithering_finished = DEVICE_PRIVATE_DATA->rmse_count >= AGENT_GUIDER_SETTINGS_DITH_LIMIT_ITEM->number.value;
					}
				} else {
					if (DEVICE_PRIVATE_DATA->rmse_ra_threshold > 0) {
						dithering_finished = DEVICE_PRIVATE_DATA->rmse_count >= AGENT_GUIDER_SETTINGS_DITH_LIMIT_ITEM->number.value && AGENT_GUIDER_STATS_RMSE_RA_ITEM->number.value < DEVICE_PRIVATE_DATA->rmse_ra_threshold;
					} else {
						dithering_finished = DEVICE_PRIVATE_DATA->rmse_count >= AGENT_GUIDER_SETTINGS_DITH_LIMIT_ITEM->number.value;
					}
				}
				if (dithering_finished) {
					AGENT_GUIDER_STATS_DITHERING_ITEM->number.value = 0;
				} else {
					AGENT_GUIDER_STATS_DITHERING_ITEM->number.value = fmax(AGENT_GUIDER_STATS_RMSE_RA_ITEM->number.value, AGENT_GUIDER_STATS_RMSE_DEC_ITEM->number.value);
				}
			}
		}
		double reported_delay_time = AGENT_GUIDER_SETTINGS_DELAY_ITEM->number.target;
		if (reported_delay_time > 0) {
			AGENT_GUIDER_STATS_DELAY_ITEM->number.value = reported_delay_time;
			indigo_update_property(device, AGENT_GUIDER_STATS_PROPERTY, NULL);
			while (reported_delay_time > 0) {
				if (reported_delay_time < floor(AGENT_GUIDER_STATS_DELAY_ITEM->number.value)) {
					double c = ceil(reported_delay_time);
					if (AGENT_GUIDER_STATS_DELAY_ITEM->number.value > c) {
						AGENT_GUIDER_STATS_DELAY_ITEM->number.value = c;
						indigo_update_property(device, AGENT_GUIDER_STATS_PROPERTY, NULL);
					}
				}
				if (reported_delay_time > 1) {
					reported_delay_time -= 0.2;
					indigo_usleep(200000);
				} else {
					reported_delay_time -= 0.01;
					indigo_usleep(10000);
				}
			}
			AGENT_GUIDER_STATS_DELAY_ITEM->number.value = 0;
		}
		indigo_update_property(device, AGENT_GUIDER_STATS_PROPERTY, NULL);
		write_log_record(device);
	}
	DEVICE_PRIVATE_DATA->silence_warnings = false;
	close_log(device);
	allow_abort_by_mount_agent(device, false);
	if (!AGENT_GUIDER_DETECTION_DONUTS_ITEM->sw.value) {
		restore_subframe(device);
	}
	indigo_restore_switch_state(device, CCD_UPLOAD_MODE_PROPERTY_NAME, upload_mode);
	indigo_restore_switch_state(device, CCD_IMAGE_FORMAT_PROPERTY_NAME, image_format);
	bool result;
	if (AGENT_ABORT_PROCESS_PROPERTY->state == INDIGO_BUSY_STATE) {
		AGENT_ABORT_PROCESS_PROPERTY->state = INDIGO_OK_STATE;
		indigo_update_property(device, AGENT_ABORT_PROCESS_PROPERTY, NULL);
		AGENT_GUIDER_STATS_PHASE_ITEM->number.value = INDIGO_GUIDER_PHASE_DONE;
		AGENT_START_PROCESS_PROPERTY->state = INDIGO_OK_STATE;
		indigo_send_message(device, "Guiding aborted");
		result = true;
	} else {
		AGENT_GUIDER_STATS_PHASE_ITEM->number.value = INDIGO_GUIDER_PHASE_FAILED;
		AGENT_START_PROCESS_PROPERTY->state = INDIGO_ALERT_STATE;
		indigo_send_message(device, "Guiding failed");
		result = false;
	}
	AGENT_GUIDER_STATS_DITHERING_ITEM->number.value = 0;
	indigo_update_property(device, AGENT_GUIDER_STATS_PROPERTY, NULL);
	AGENT_GUIDER_START_CALIBRATION_AND_GUIDING_ITEM->sw.value = AGENT_GUIDER_START_GUIDING_ITEM->sw.value = false;
	indigo_update_property(device, AGENT_START_PROCESS_PROPERTY, NULL);
	return result;
}

static void calibrate_process(indigo_device *device) {
	FILTER_DEVICE_CONTEXT->running_process = true;
	calibrate(device);
	FILTER_DEVICE_CONTEXT->running_process = false;
}

static void calibrate_and_guide_process(indigo_device *device) {
	FILTER_DEVICE_CONTEXT->running_process = true;
	if (calibrate(device)) {
		AGENT_START_PROCESS_PROPERTY->state = INDIGO_BUSY_STATE;
		AGENT_GUIDER_START_GUIDING_ITEM->sw.value = true;
		indigo_update_property(device, AGENT_START_PROCESS_PROPERTY, NULL);
		guide(device);
	}
	FILTER_DEVICE_CONTEXT->running_process = false;
}

static void guide_process(indigo_device *device) {
	FILTER_DEVICE_CONTEXT->running_process = true;
	guide(device);
	FILTER_DEVICE_CONTEXT->running_process = false;
}

static void find_stars_process(indigo_device *device) {
	FILTER_DEVICE_CONTEXT->running_process = true;
	int upload_mode = indigo_save_switch_state(device, CCD_UPLOAD_MODE_PROPERTY_NAME, CCD_UPLOAD_MODE_CLIENT_ITEM_NAME);
	int image_format = indigo_save_switch_state(device, CCD_IMAGE_FORMAT_PROPERTY_NAME, CCD_IMAGE_FORMAT_RAW_ITEM_NAME);
	if (capture_frame(device) && find_stars(device)) {
		AGENT_GUIDER_STARS_PROPERTY->state = INDIGO_OK_STATE;
	} else {
		AGENT_GUIDER_STARS_PROPERTY->state = INDIGO_ALERT_STATE;
	}
	indigo_restore_switch_state(device, CCD_UPLOAD_MODE_PROPERTY_NAME, upload_mode);
	indigo_restore_switch_state(device, CCD_IMAGE_FORMAT_PROPERTY_NAME, image_format);
	if (AGENT_ABORT_PROCESS_PROPERTY->state == INDIGO_BUSY_STATE) {
		AGENT_ABORT_PROCESS_PROPERTY->state = INDIGO_OK_STATE;
		indigo_update_property(device, AGENT_ABORT_PROCESS_PROPERTY, NULL);
	}
	indigo_update_property(device, AGENT_GUIDER_STARS_PROPERTY, NULL);
	FILTER_DEVICE_CONTEXT->running_process = false;
}

static void clear_selection_process(indigo_device *device) {
	FILTER_DEVICE_CONTEXT->running_process = true;
	clear_selection(device);
	AGENT_GUIDER_CLEAR_SELECTION_ITEM->sw.value = false;
	AGENT_START_PROCESS_PROPERTY->state = INDIGO_OK_STATE;
	indigo_update_property(device, AGENT_START_PROCESS_PROPERTY, NULL);
	FILTER_DEVICE_CONTEXT->running_process = false;
}

static void abort_process(indigo_device *device) {
	indigo_change_switch_property_1(FILTER_DEVICE_CONTEXT->client, device->name, CCD_ABORT_EXPOSURE_PROPERTY_NAME, CCD_ABORT_EXPOSURE_ITEM_NAME, true);
}

static void snoop_changes(indigo_client *client, indigo_device *device, indigo_property *property) {
	if (!strcmp(property->name, FILTER_CCD_LIST_PROPERTY_NAME)) { // Snoop CCD
		if (!INDIGO_FILTER_CCD_SELECTED) {
			DEVICE_PRIVATE_DATA->exposure_state = INDIGO_IDLE_STATE;
		}
	} else if (!strcmp(property->name, CCD_EXPOSURE_PROPERTY_NAME)) {
		if (!DEVICE_PRIVATE_DATA->has_camera) {
			DEVICE_PRIVATE_DATA->has_camera = true;
			clear_selection(device);
		}
		for (int i = 0; i < property->count; i++) {
			indigo_item *item = property->items + i;
			if (!strcmp(item->name, CCD_EXPOSURE_ITEM_NAME)) {
				DEVICE_PRIVATE_DATA->remaining_exposure_time = item->number.value;
				break;
			}
		}
		DEVICE_PRIVATE_DATA->exposure_state = property->state;
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
				DEVICE_PRIVATE_DATA->last_width = DEVICE_PRIVATE_DATA->frame[2] / DEVICE_PRIVATE_DATA->bin_x;
				DEVICE_PRIVATE_DATA->last_height = DEVICE_PRIVATE_DATA->frame[3] / DEVICE_PRIVATE_DATA->bin_y;
				AGENT_GUIDER_SELECTION_INCLUDE_LEFT_ITEM->number.value = AGENT_GUIDER_SELECTION_INCLUDE_TOP_ITEM->number.value = AGENT_GUIDER_SELECTION_INCLUDE_WIDTH_ITEM->number.value = AGENT_GUIDER_SELECTION_INCLUDE_HEIGHT_ITEM->number.value = AGENT_GUIDER_SELECTION_EXCLUDE_LEFT_ITEM->number.value = AGENT_GUIDER_SELECTION_EXCLUDE_TOP_ITEM->number.value = AGENT_GUIDER_SELECTION_EXCLUDE_WIDTH_ITEM->number.value = AGENT_GUIDER_SELECTION_EXCLUDE_HEIGHT_ITEM->number.value = 0;
				validate_include_region(device, false);
				indigo_update_property(device, AGENT_GUIDER_SELECTION_PROPERTY, NULL);
			}
		}
	} else if (!strcmp(property->name, FILTER_GUIDER_LIST_PROPERTY_NAME)) { // Snoop guider
		if (!INDIGO_FILTER_ROTATOR_SELECTED) {
			DEVICE_PRIVATE_DATA->guide_ra_state = INDIGO_IDLE_STATE;
			DEVICE_PRIVATE_DATA->guide_dec_state = INDIGO_IDLE_STATE;
		}
	} else if (!strcmp(property->name, GUIDER_GUIDE_RA_PROPERTY_NAME)) {
		DEVICE_PRIVATE_DATA->guide_ra_state = property->state;
	} else if (!strcmp(property->name, GUIDER_GUIDE_DEC_PROPERTY_NAME)) {
		DEVICE_PRIVATE_DATA->guide_dec_state = property->state;
	}
}

// -------------------------------------------------------------------------------- INDIGO agent device implementation

static indigo_result agent_enumerate_properties(indigo_device *device, indigo_client *client, indigo_property *property);

static indigo_result agent_device_attach(indigo_device *device) {
	assert(device != NULL);
	assert(DEVICE_PRIVATE_DATA != NULL);
	if (indigo_is_sandboxed) {
		snprintf(default_log_path, PATH_MAX, "%s/", getenv("HOME"));
	} else {
		snprintf(default_log_path, PATH_MAX, "%s/indigo_image_cache/", getenv("HOME"));
	}
	if (indigo_filter_device_attach(device, DRIVER_NAME, DRIVER_VERSION, INDIGO_INTERFACE_CCD | INDIGO_INTERFACE_GUIDER) == INDIGO_OK) {
		// -------------------------------------------------------------------------------- Device properties
		FILTER_CCD_LIST_PROPERTY->hidden = false;
		FILTER_GUIDER_LIST_PROPERTY->hidden = false;
		FILTER_RELATED_AGENT_LIST_PROPERTY->hidden = false;
		// -------------------------------------------------------------------------------- Process properties
		AGENT_GUIDER_DETECTION_MODE_PROPERTY = indigo_init_switch_property(NULL, device->name, AGENT_GUIDER_DETECTION_MODE_PROPERTY_NAME, "Agent", "Drift detection mode", INDIGO_OK_STATE, INDIGO_RW_PERM, INDIGO_ONE_OF_MANY_RULE, 4);
		if (AGENT_GUIDER_DETECTION_MODE_PROPERTY == NULL)
			return INDIGO_FAILED;
		indigo_init_switch_item(AGENT_GUIDER_DETECTION_SELECTION_ITEM, AGENT_GUIDER_DETECTION_SELECTION_ITEM_NAME, "Selection", true);
		indigo_init_switch_item(AGENT_GUIDER_DETECTION_WEIGHTED_SELECTION_ITEM, AGENT_GUIDER_DETECTION_WEIGHTED_SELECTION_ITEM_NAME, "Weighted selection", false);
		indigo_init_switch_item(AGENT_GUIDER_DETECTION_DONUTS_ITEM, AGENT_GUIDER_DETECTION_DONUTS_ITEM_NAME, "Donuts", false);
		indigo_init_switch_item(AGENT_GUIDER_DETECTION_CENTROID_ITEM, AGENT_GUIDER_DETECTION_CENTROID_ITEM_NAME, "Centroid", false);

		AGENT_GUIDER_DEC_MODE_PROPERTY = indigo_init_switch_property(NULL, device->name, AGENT_GUIDER_DEC_MODE_PROPERTY_NAME, "Agent", "Dec guiding mode", INDIGO_OK_STATE, INDIGO_RW_PERM, INDIGO_ONE_OF_MANY_RULE, 4);
		if (AGENT_GUIDER_DEC_MODE_PROPERTY == NULL)
			return INDIGO_FAILED;
		indigo_init_switch_item(AGENT_GUIDER_DEC_MODE_BOTH_ITEM, AGENT_GUIDER_DEC_MODE_BOTH_ITEM_NAME, "North and south", true);
		indigo_init_switch_item(AGENT_GUIDER_DEC_MODE_NORTH_ITEM, AGENT_GUIDER_DEC_MODE_NORTH_ITEM_NAME, "North only", false);
		indigo_init_switch_item(AGENT_GUIDER_DEC_MODE_SOUTH_ITEM, AGENT_GUIDER_DEC_MODE_SOUTH_ITEM_NAME, "South only", false);
		indigo_init_switch_item(AGENT_GUIDER_DEC_MODE_NONE_ITEM, AGENT_GUIDER_DEC_MODE_NONE_ITEM_NAME, "None", false);

		AGENT_GUIDER_APPLY_DEC_BACKLASH_PROPERTY = indigo_init_switch_property(NULL, device->name,AGENT_GUIDER_APPLY_DEC_BACKLASH_PROPERTY_NAME, "Agent", "Apply Dec backlash", INDIGO_OK_STATE, INDIGO_RW_PERM, INDIGO_ONE_OF_MANY_RULE, 2);
		if (AGENT_GUIDER_APPLY_DEC_BACKLASH_PROPERTY == NULL)
			return INDIGO_FAILED;
		indigo_init_switch_item(AGENT_GUIDER_APPLY_DEC_BACKLASH_DISABLED_ITEM, AGENT_GUIDER_APPLY_DEC_BACKLASH_DISABLED_ITEM_NAME, "Disabled", true);
		indigo_init_switch_item(AGENT_GUIDER_APPLY_DEC_BACKLASH_ENABLED_ITEM, AGENT_GUIDER_APPLY_DEC_BACKLASH_ENABLED_ITEM_NAME, "Enabled", false);

		AGENT_START_PROCESS_PROPERTY = indigo_init_switch_property(NULL, device->name, AGENT_START_PROCESS_PROPERTY_NAME, "Agent", "Start process", INDIGO_OK_STATE, INDIGO_RW_PERM, INDIGO_AT_MOST_ONE_RULE, 6);
		if (AGENT_START_PROCESS_PROPERTY == NULL)
			return INDIGO_FAILED;
		indigo_init_switch_item(AGENT_GUIDER_START_PREVIEW_1_ITEM, AGENT_GUIDER_START_PREVIEW_1_ITEM_NAME, "Preview single frame", false);
		indigo_init_switch_item(AGENT_GUIDER_START_PREVIEW_ITEM, AGENT_GUIDER_START_PREVIEW_ITEM_NAME, "Start preview", false);
		indigo_init_switch_item(AGENT_GUIDER_START_CALIBRATION_ITEM, AGENT_GUIDER_START_CALIBRATION_ITEM_NAME, "Start calibration", false);
		indigo_init_switch_item(AGENT_GUIDER_START_CALIBRATION_AND_GUIDING_ITEM, AGENT_GUIDER_START_CALIBRATION_AND_GUIDING_ITEM_NAME, "Start calibration and guiding", false);
		indigo_init_switch_item(AGENT_GUIDER_START_GUIDING_ITEM, AGENT_GUIDER_START_GUIDING_ITEM_NAME, "Start guiding", false);
		indigo_init_switch_item(AGENT_GUIDER_CLEAR_SELECTION_ITEM, AGENT_GUIDER_CLEAR_SELECTION_ITEM_NAME, "Clear star selection", false);
		AGENT_ABORT_PROCESS_PROPERTY = indigo_init_switch_property(NULL, device->name, AGENT_ABORT_PROCESS_PROPERTY_NAME, "Agent", "Abort process", INDIGO_OK_STATE, INDIGO_RW_PERM, INDIGO_AT_MOST_ONE_RULE, 1);
		if (AGENT_ABORT_PROCESS_PROPERTY == NULL)
			return INDIGO_FAILED;
		indigo_init_switch_item(AGENT_ABORT_PROCESS_ITEM, AGENT_ABORT_PROCESS_ITEM_NAME, "Abort", false);
		
		AGENT_PROCESS_FEATURES_PROPERTY = indigo_init_switch_property(NULL, device->name, AGENT_PROCESS_FEATURES_PROPERTY_NAME, "Agent", "Process features", INDIGO_OK_STATE, INDIGO_RW_PERM, INDIGO_ANY_OF_MANY_RULE, 8);
		if (AGENT_PROCESS_FEATURES_PROPERTY == NULL)
			return INDIGO_FAILED;
		indigo_init_switch_item(AGENT_GUIDER_ENABLE_LOGGING_FEATURE_ITEM, AGENT_GUIDER_ENABLE_LOGGING_FEATURE_ITEM_NAME, "Enable logging", false);
		indigo_init_switch_item(AGENT_GUIDER_FAIL_ON_CALIBRATION_ERROR_ITEM, AGENT_GUIDER_FAIL_ON_CALIBRATION_ERROR_ITEM_NAME, "Fail on calibration error", false);
		indigo_init_switch_item(AGENT_GUIDER_RESET_ON_CALIBRATION_ERROR_ITEM, AGENT_GUIDER_RESET_ON_CALIBRATION_ERROR_ITEM_NAME, "Reset selection on calibration error", true);
		indigo_init_switch_item(AGENT_GUIDER_FAIL_ON_GUIDING_ERROR_ITEM, AGENT_GUIDER_FAIL_ON_GUIDING_ERROR_ITEM_NAME, "Fail on guiding error", false);
		indigo_init_switch_item(AGENT_GUIDER_CONTINUE_ON_GUIDING_ERROR_ITEM, AGENT_GUIDER_CONTINUE_ON_GUIDING_ERROR_ITEM_NAME, "Continue on guiding error", true);
		indigo_init_switch_item(AGENT_GUIDER_RESET_ON_GUIDING_ERROR_ITEM, AGENT_GUIDER_RESET_ON_GUIDING_ERROR_ITEM_NAME, "Reset selection on guiding error", false);
		indigo_init_switch_item(AGENT_GUIDER_RESET_ON_GUIDING_ERROR_WAIT_ALL_STARS_ITEM, AGENT_GUIDER_RESET_ON_GUIDING_ERROR_WAIT_ALL_STARS_ITEM_NAME, "Reset selection should wait for all stars before resuming", true);
		indigo_init_switch_item(AGENT_GUIDER_USE_INCLUDE_FOR_DONUTS_ITEM, AGENT_GUIDER_USE_INCLUDE_FOR_DONUTS_ITEM_NAME, "Use include region for DONUTS", false);
		//------------------------------------------------------------------------------- Mount orientation
		AGENT_GUIDER_MOUNT_COORDINATES_PROPERTY = indigo_init_number_property(NULL, device->name, AGENT_GUIDER_MOUNT_COORDINATES_PROPERTY_NAME, "Agent", "Telescope coordinates", INDIGO_OK_STATE, INDIGO_RW_PERM, 3);
		if (AGENT_GUIDER_MOUNT_COORDINATES_PROPERTY == NULL)
			return INDIGO_FAILED;
		indigo_init_sexagesimal_number_item(AGENT_GUIDER_MOUNT_COORDINATES_RA_ITEM, AGENT_GUIDER_MOUNT_COORDINATES_RA_ITEM_NAME, "Right ascension (0 to 24 hrs)", 0, 24, 1, 0);
		indigo_init_sexagesimal_number_item(AGENT_GUIDER_MOUNT_COORDINATES_DEC_ITEM, AGENT_GUIDER_MOUNT_COORDINATES_DEC_ITEM_NAME, "Declination (-90° to +90°)", -90, 90, 1, 0);
		indigo_init_number_item(AGENT_GUIDER_MOUNT_COORDINATES_SOP_ITEM, AGENT_GUIDER_MOUNT_COORDINATES_SOP_ITEM_NAME, "Side of Pier (-1=E, 1=W, 0=undef)", -1, 1, 1, 0);
		DEVICE_PRIVATE_DATA->cos_dec = 1; /* default dec is 0 until set */
		// -------------------------------------------------------------------------------- Guiding settings
		AGENT_GUIDER_SETTINGS_PROPERTY = indigo_init_number_property(NULL, device->name, AGENT_GUIDER_SETTINGS_PROPERTY_NAME, "Agent", "Settings", INDIGO_OK_STATE, INDIGO_RW_PERM, 23);
		if (AGENT_GUIDER_SETTINGS_PROPERTY == NULL)
			return INDIGO_FAILED;
		indigo_init_number_item(AGENT_GUIDER_SETTINGS_EXPOSURE_ITEM, AGENT_GUIDER_SETTINGS_EXPOSURE_ITEM_NAME, "Exposure time (s)", 0, 120, 0.1, 1);
		indigo_init_number_item(AGENT_GUIDER_SETTINGS_DELAY_ITEM, AGENT_GUIDER_SETTINGS_DELAY_ITEM_NAME, "Delay time (s)", 0, 120, 1, 0);
		indigo_init_number_item(AGENT_GUIDER_SETTINGS_STEP_ITEM, AGENT_GUIDER_SETTINGS_STEP_ITEM_NAME, "Calibration step (s)", 0.05, 2, 0.05, 0.200);
		indigo_init_number_item(AGENT_GUIDER_SETTINGS_BL_STEPS_ITEM, AGENT_GUIDER_SETTINGS_BL_STEPS_ITEM_NAME, "Max clear backlash steps", 0, 50, 1, 10);
		indigo_init_number_item(AGENT_GUIDER_SETTINGS_BL_DRIFT_ITEM, AGENT_GUIDER_SETTINGS_BL_DRIFT_ITEM_NAME, "Min clear backlash drift (px)", 0, 25, 1, 3);
		indigo_init_number_item(AGENT_GUIDER_SETTINGS_CAL_STEPS_ITEM, AGENT_GUIDER_SETTINGS_CAL_STEPS_ITEM_NAME, "Max calibration steps", 0, 50, 1, 20);
		indigo_init_number_item(AGENT_GUIDER_SETTINGS_CAL_DRIFT_ITEM, AGENT_GUIDER_SETTINGS_CAL_DRIFT_ITEM_NAME, "Min calibration drift (px)", 0, 100, 5, 20);
		indigo_init_number_item(AGENT_GUIDER_SETTINGS_ANGLE_ITEM, AGENT_GUIDER_SETTINGS_ANGLE_ITEM_NAME, "Angle (°)", -180, 180, 1, 0);
		indigo_init_number_item(AGENT_GUIDER_SETTINGS_SOP_ITEM, AGENT_GUIDER_SETTINGS_SOP_ITEM_NAME, "Side of Pier (-1=E, 1=W, 0=undef)", -1, 1, 1, 0);
		indigo_init_number_item(AGENT_GUIDER_SETTINGS_BACKLASH_ITEM, AGENT_GUIDER_SETTINGS_BACKLASH_ITEM_NAME, "Dec backlash (px)", 0, 100, 0, 0);
		indigo_init_number_item(AGENT_GUIDER_SETTINGS_SPEED_RA_ITEM, AGENT_GUIDER_SETTINGS_SPEED_RA_ITEM_NAME, "RA speed at equator (px/s)", -500, 500, 0.1, 0);
		indigo_init_number_item(AGENT_GUIDER_SETTINGS_SPEED_DEC_ITEM, AGENT_GUIDER_SETTINGS_SPEED_DEC_ITEM_NAME, "Dec speed (px/s)", -500, 500, 0.1, 0);
		indigo_init_number_item(AGENT_GUIDER_SETTINGS_MIN_ERR_ITEM, AGENT_GUIDER_SETTINGS_MIN_ERR_ITEM_NAME, "Min error (px)", 0, 5, 0.1, 0);
		indigo_init_number_item(AGENT_GUIDER_SETTINGS_MIN_PULSE_ITEM, AGENT_GUIDER_SETTINGS_MIN_PULSE_ITEM_NAME, "Min pulse (s)", 0, 1, 0.001, 0.01);
		indigo_init_number_item(AGENT_GUIDER_SETTINGS_MAX_PULSE_ITEM, AGENT_GUIDER_SETTINGS_MAX_PULSE_ITEM_NAME, "Max pulse (s)", 0, 5, 0.1, 1);
		indigo_init_number_item(AGENT_GUIDER_SETTINGS_AGG_RA_ITEM, AGENT_GUIDER_SETTINGS_AGG_RA_ITEM_NAME, "RA Proportional aggressivity (%)", 0, 500, 5, 100);
		indigo_init_number_item(AGENT_GUIDER_SETTINGS_AGG_DEC_ITEM, AGENT_GUIDER_SETTINGS_AGG_DEC_ITEM_NAME, "Dec Proportional aggressivity (%)", 0, 500, 5, 100);
		indigo_init_number_item(AGENT_GUIDER_SETTINGS_I_GAIN_RA_ITEM, AGENT_GUIDER_SETTINGS_I_GAIN_RA_ITEM_NAME, "RA Integral gain", 0, 10, 0.05, 0.5);
		indigo_init_number_item(AGENT_GUIDER_SETTINGS_I_GAIN_DEC_ITEM, AGENT_GUIDER_SETTINGS_I_GAIN_DEC_ITEM_NAME, "Dec Integral gain", 0, 10, 0.05, 0.5);
		indigo_init_number_item(AGENT_GUIDER_SETTINGS_STACK_ITEM, AGENT_GUIDER_SETTINGS_STACK_ITEM_NAME, "Integral stack size (frames)", 1, MAX_STACK, 1, 1);
		indigo_init_number_item(AGENT_GUIDER_SETTINGS_DITHERING_AMOUNT_ITEM, AGENT_GUIDER_SETTINGS_DITHERING_AMOUNT_ITEM_NAME, "Dithering max amount (px)", 0, 15, 1, 1);
		indigo_init_number_item(AGENT_GUIDER_SETTINGS_DITHERING_TIME_LIMIT_ITEM, AGENT_GUIDER_SETTINGS_DITHERING_TIME_LIMIT_ITEM_NAME, "Dithering Settle time limit (s)", 0, 300, 1, 60);
		indigo_init_number_item(AGENT_GUIDER_SETTINGS_DITH_LIMIT_ITEM, AGENT_GUIDER_SETTINGS_DITH_LIMIT_ITEM_NAME, "Dithering settling limit (frames)", 1, 50, 1, 5);
		// -------------------------------------------------------------------------------- FLIP_REVERSE_DEC
		AGENT_GUIDER_FLIP_REVERSES_DEC_PROPERTY = indigo_init_switch_property(NULL, device->name, AGENT_GUIDER_FLIP_REVERSES_DEC_PROPERTY_NAME, "Agent", "Reverse Dec speed after meridian flip", INDIGO_OK_STATE, INDIGO_RW_PERM, INDIGO_ONE_OF_MANY_RULE, 2);
		if (AGENT_GUIDER_FLIP_REVERSES_DEC_PROPERTY == NULL)
			return INDIGO_FAILED;
		indigo_init_switch_item(AGENT_GUIDER_FLIP_REVERSES_DEC_ENABLED_ITEM, AGENT_GUIDER_FLIP_REVERSES_DEC_ENABLED_ITEM_NAME, "Enabled", true);
		indigo_init_switch_item(AGENT_GUIDER_FLIP_REVERSES_DEC_DISABLED_ITEM, AGENT_GUIDER_FLIP_REVERSES_DEC_DISABLED_ITEM_NAME, "Disabled", false);
		// -------------------------------------------------------------------------------- Detected stars
		AGENT_GUIDER_STARS_PROPERTY = indigo_init_switch_property(NULL, device->name, AGENT_GUIDER_STARS_PROPERTY_NAME, "Agent", "Stars", INDIGO_OK_STATE, INDIGO_RW_PERM, INDIGO_ONE_OF_MANY_RULE, MAX_STAR_COUNT + 1);
		if (AGENT_GUIDER_STARS_PROPERTY == NULL)
			return INDIGO_FAILED;
		AGENT_GUIDER_STARS_PROPERTY->count = 1;
		indigo_init_switch_item(AGENT_GUIDER_STARS_REFRESH_ITEM, AGENT_GUIDER_STARS_REFRESH_ITEM_NAME, "Refresh", false);
		// -------------------------------------------------------------------------------- Selected star
		AGENT_GUIDER_SELECTION_PROPERTY = indigo_init_number_property(NULL, device->name, AGENT_GUIDER_SELECTION_PROPERTY_NAME, "Agent", "Selection", INDIGO_OK_STATE, INDIGO_RW_PERM, 12 + 2 * INDIGO_MAX_MULTISTAR_COUNT);
		if (AGENT_GUIDER_SELECTION_PROPERTY == NULL)
			return INDIGO_FAILED;
		indigo_init_number_item(AGENT_GUIDER_SELECTION_RADIUS_ITEM, AGENT_GUIDER_SELECTION_RADIUS_ITEM_NAME, "Radius (px)", 1, 50, 1, 8);
		indigo_init_number_item(AGENT_GUIDER_SELECTION_SUBFRAME_ITEM, AGENT_GUIDER_SELECTION_SUBFRAME_ITEM_NAME, "Subframe", 0, 20, 1, 0);
		indigo_init_number_item(AGENT_GUIDER_SELECTION_EDGE_CLIPPING_ITEM, AGENT_GUIDER_SELECTION_EDGE_CLIPPING_ITEM_NAME, "Edge Clipping (px)", 0, 500, 1, 8);
		indigo_init_number_item(AGENT_GUIDER_SELECTION_INCLUDE_LEFT_ITEM, AGENT_GUIDER_SELECTION_INCLUDE_LEFT_ITEM_NAME, "Include left (px)", 0, 0xFFFF, 1, 0);
		indigo_init_number_item(AGENT_GUIDER_SELECTION_INCLUDE_TOP_ITEM, AGENT_GUIDER_SELECTION_INCLUDE_TOP_ITEM_NAME, "Include top (px)", 0, 0xFFFF, 1, 0);
		indigo_init_number_item(AGENT_GUIDER_SELECTION_INCLUDE_WIDTH_ITEM, AGENT_GUIDER_SELECTION_INCLUDE_WIDTH_ITEM_NAME, "Include width (px)", 0, 0xFFFF, 1, 0);
		indigo_init_number_item(AGENT_GUIDER_SELECTION_INCLUDE_HEIGHT_ITEM, AGENT_GUIDER_SELECTION_INCLUDE_HEIGHT_ITEM_NAME, "Include height (px)", 0, 0xFFFF, 1, 0);
		indigo_init_number_item(AGENT_GUIDER_SELECTION_EXCLUDE_LEFT_ITEM, AGENT_GUIDER_SELECTION_EXCLUDE_LEFT_ITEM_NAME, "Exclude left (px)", 0, 0xFFFF, 1, 0);
		indigo_init_number_item(AGENT_GUIDER_SELECTION_EXCLUDE_TOP_ITEM, AGENT_GUIDER_SELECTION_EXCLUDE_TOP_ITEM_NAME, "Exclude top (px)", 0, 0xFFFF, 1, 0);
		indigo_init_number_item(AGENT_GUIDER_SELECTION_EXCLUDE_WIDTH_ITEM, AGENT_GUIDER_SELECTION_EXCLUDE_WIDTH_ITEM_NAME, "Exclude width (px)", 0, 0xFFFF, 1, 0);
		indigo_init_number_item(AGENT_GUIDER_SELECTION_EXCLUDE_HEIGHT_ITEM, AGENT_GUIDER_SELECTION_EXCLUDE_HEIGHT_ITEM_NAME, "Exclude height (px)", 0, 0xFFFF, 1, 0);
		indigo_init_number_item(AGENT_GUIDER_SELECTION_STAR_COUNT_ITEM, AGENT_GUIDER_SELECTION_STAR_COUNT_ITEM_NAME, "Maximum number of stars", 1, INDIGO_MAX_MULTISTAR_COUNT, 1, 1);
		for (int i = 0; i < INDIGO_MAX_MULTISTAR_COUNT; i++) {
			indigo_item *item_x = AGENT_GUIDER_SELECTION_X_ITEM + 2 * i;
			indigo_item *item_y = AGENT_GUIDER_SELECTION_Y_ITEM + 2 * i;
			char name[INDIGO_NAME_SIZE], label[INDIGO_VALUE_SIZE];
			sprintf(name, i ? "%s_%d" : "%s", AGENT_GUIDER_SELECTION_X_ITEM_NAME, i + 1);
			sprintf(label, i ? "Selection #%d X (px)" : "Selection X (px)", i + 1);
			indigo_init_number_item(item_x, name, label, 0, 0xFFFF, 1, 0);
			sprintf(name, i ? "%s_%d" : "%s", AGENT_GUIDER_SELECTION_Y_ITEM_NAME, i + 1);
			sprintf(label, i ? "Selection #%d Y (px)" : "Selection Y (px)", i + 1);
			indigo_init_number_item(item_y, name, label, 0, 0xFFFF, 1, 0);
		}
		AGENT_GUIDER_SELECTION_PROPERTY->count = 14;
		// -------------------------------------------------------------------------------- Guiding stats
		AGENT_GUIDER_STATS_PROPERTY = indigo_init_number_property(NULL, device->name, AGENT_GUIDER_STATS_PROPERTY_NAME, "Agent", "Statistics", INDIGO_OK_STATE, INDIGO_RO_PERM, 19);
		if (AGENT_GUIDER_STATS_PROPERTY == NULL)
			return INDIGO_FAILED;
		indigo_init_number_item(AGENT_GUIDER_STATS_PHASE_ITEM, AGENT_GUIDER_STATS_PHASE_ITEM_NAME, "Phase #", -1, 100, 0, INDIGO_GUIDER_PHASE_DONE);
		indigo_init_number_item(AGENT_GUIDER_STATS_FRAME_ITEM, AGENT_GUIDER_STATS_FRAME_ITEM_NAME, "Frame #", 0, 0xFFFFFFFF, 0, 0);
		indigo_init_number_item(AGENT_GUIDER_STATS_REFERENCE_X_ITEM, AGENT_GUIDER_STATS_REFERENCE_X_ITEM_NAME, "Reference X (px)", 0, 100000, 0, 0);
		indigo_init_number_item(AGENT_GUIDER_STATS_REFERENCE_Y_ITEM, AGENT_GUIDER_STATS_REFERENCE_Y_ITEM_NAME, "Reference Y (px)", 0, 100000, 0, 0);
		indigo_init_number_item(AGENT_GUIDER_STATS_DRIFT_X_ITEM, AGENT_GUIDER_STATS_DRIFT_X_ITEM_NAME, "Drift X (px)", -1000, 1000, 0, 0);
		indigo_init_number_item(AGENT_GUIDER_STATS_DRIFT_Y_ITEM, AGENT_GUIDER_STATS_DRIFT_Y_ITEM_NAME, "Drift Y (px)", -1000, 1000, 0, 0);
		indigo_init_number_item(AGENT_GUIDER_STATS_DRIFT_RA_ITEM, AGENT_GUIDER_STATS_DRIFT_RA_ITEM_NAME, "Drift RA (px)", -1000, 1000, 0, 0);
		indigo_init_number_item(AGENT_GUIDER_STATS_DRIFT_DEC_ITEM, AGENT_GUIDER_STATS_DRIFT_DEC_ITEM_NAME, "Drift Dec (px)", -1000, 1000, 0, 0);
		indigo_init_number_item(AGENT_GUIDER_STATS_DRIFT_RA_S_ITEM, AGENT_GUIDER_STATS_DRIFT_RA_S_ITEM_NAME, "Drift RA (\")", -1000, 1000, 0, 0);
		indigo_init_number_item(AGENT_GUIDER_STATS_DRIFT_DEC_S_ITEM, AGENT_GUIDER_STATS_DRIFT_DEC_S_ITEM_NAME, "Drift Dec (\")", -1000, 1000, 0, 0);
		indigo_init_number_item(AGENT_GUIDER_STATS_CORR_RA_ITEM, AGENT_GUIDER_STATS_CORR_RA_ITEM_NAME, "Correction RA (s)", -1000, 1000, 0, 0);
		indigo_init_number_item(AGENT_GUIDER_STATS_CORR_DEC_ITEM, AGENT_GUIDER_STATS_CORR_DEC_ITEM_NAME, "Correction Dec (s)", -1000, 1000, 0, 0);
		indigo_init_number_item(AGENT_GUIDER_STATS_RMSE_RA_ITEM, AGENT_GUIDER_STATS_RMSE_RA_ITEM_NAME, "RMSE RA (px)", -1000, 1000, 0, 0);
		indigo_init_number_item(AGENT_GUIDER_STATS_RMSE_DEC_ITEM, AGENT_GUIDER_STATS_RMSE_DEC_ITEM_NAME, "RMSE Dec (px)", -1000, 1000, 0, 0);
		indigo_init_number_item(AGENT_GUIDER_STATS_RMSE_RA_S_ITEM, AGENT_GUIDER_STATS_RMSE_RA_S_ITEM_NAME, "RMSE RA (\")", -1000, 1000, 0, 0);
		indigo_init_number_item(AGENT_GUIDER_STATS_RMSE_DEC_S_ITEM, AGENT_GUIDER_STATS_RMSE_DEC_S_ITEM_NAME, "RMSE Dec (\")", -1000, 1000, 0, 0);
		indigo_init_number_item(AGENT_GUIDER_STATS_SNR_ITEM, AGENT_GUIDER_STATS_SNR_ITEM_NAME, "Frame digest SNR", 0, 1000, 0, 0);
		indigo_init_number_item(AGENT_GUIDER_STATS_DELAY_ITEM, AGENT_GUIDER_STATS_DELAY_ITEM_NAME, "Remaining delay (s)", 0, 100, 0, 0);
		indigo_init_number_item(AGENT_GUIDER_STATS_DITHERING_ITEM, AGENT_GUIDER_STATS_DITHERING_ITEM_NAME, "Dithering RMSE (px)", 0, 100, 0, 0);
		// -------------------------------------------------------------------------------- Logging
		AGENT_GUIDER_LOG_PROPERTY = indigo_init_text_property(NULL, device->name, AGENT_GUIDER_LOG_PROPERTY_NAME, "Agent", "Logging", INDIGO_OK_STATE, INDIGO_RW_PERM, 2);
		if (AGENT_GUIDER_LOG_PROPERTY == NULL)
			return INDIGO_FAILED;
		indigo_init_text_item(AGENT_GUIDER_LOG_DIR_ITEM, AGENT_GUIDER_LOG_DIR_ITEM_NAME, "Directory", default_log_path);
		indigo_init_text_item(AGENT_GUIDER_LOG_TEMPLATE_ITEM, AGENT_GUIDER_LOG_TEMPLATE_ITEM_NAME, "Name template", "GUIDING_%%y%%m%%d_%%H%%M%%S.csv"); // strftime format specifiers accepted
		// -------------------------------------------------------------------------------- Dithering offsets
		AGENT_GUIDER_DITHERING_OFFSETS_PROPERTY = indigo_init_number_property(NULL, device->name, AGENT_GUIDER_DITHERING_OFFSETS_PROPERTY_NAME, "Agent", "Dithering offsets", INDIGO_OK_STATE, INDIGO_RW_PERM, 2);
		if (AGENT_GUIDER_DITHERING_OFFSETS_PROPERTY == NULL)
			return INDIGO_FAILED;
		indigo_init_number_item(AGENT_GUIDER_DITHERING_OFFSETS_X_ITEM, AGENT_GUIDER_DITHERING_OFFSETS_X_ITEM_NAME, "Offset X (px)", -15, 15, 1, 0);
		indigo_init_number_item(AGENT_GUIDER_DITHERING_OFFSETS_Y_ITEM, AGENT_GUIDER_DITHERING_OFFSETS_Y_ITEM_NAME, "Offset Y (px)", -15, 15, 1, 0);
		// -------------------------------------------------------------------------------- Dithering strategy
		AGENT_GUIDER_DITHERING_STRATEGY_PROPERTY = indigo_init_switch_property(NULL, device->name, AGENT_GUIDER_DITHERING_STRATEGY_PROPERTY_NAME, "Agent", "Dithering strategy", INDIGO_OK_STATE, INDIGO_RW_PERM, INDIGO_ONE_OF_MANY_RULE, 3);
		if (AGENT_GUIDER_DITHERING_STRATEGY_PROPERTY == NULL)
			return INDIGO_FAILED;
		indigo_init_switch_item(AGENT_GUIDER_DITHERING_STRATEGY_RANDOM_SPIRAL_ITEM, AGENT_GUIDER_DITHERING_STRATEGY_RANDOM_SPIRAL_ITEM_NAME, "Randomized spiral", true);
		indigo_init_switch_item(AGENT_GUIDER_DITHERING_STRATEGY_RANDOM_ITEM, AGENT_GUIDER_DITHERING_STRATEGY_RANDOM_ITEM_NAME, "Random", false);
		indigo_init_switch_item(AGENT_GUIDER_DITHERING_STRATEGY_SPIRAL_ITEM, AGENT_GUIDER_DITHERING_STRATEGY_SPIRAL_ITEM_NAME, "Spiral", false);
		// -------------------------------------------------------------------------------- Dither now
		AGENT_GUIDER_DITHER_PROPERTY = indigo_init_switch_property(NULL, device->name, AGENT_GUIDER_DITHER_PROPERTY_NAME, "Agent", "Dithering", INDIGO_OK_STATE, INDIGO_RW_PERM, INDIGO_AT_MOST_ONE_RULE, 2);
		if (AGENT_GUIDER_DITHER_PROPERTY == NULL)
			return INDIGO_FAILED;
		indigo_init_switch_item(AGENT_GUIDER_DITHER_TRIGGER_ITEM, AGENT_GUIDER_DITHER_TRIGGER_ITEM_NAME, "Trigger", false);
		indigo_init_switch_item(AGENT_GUIDER_DITHER_RESET_ITEM, AGENT_GUIDER_DITHER_RESET_ITEM_NAME, "Reset", false);

		// --------------------------------------------------------------------------------
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
	indigo_define_matching_property(AGENT_GUIDER_DETECTION_MODE_PROPERTY);
	indigo_define_matching_property(AGENT_GUIDER_MOUNT_COORDINATES_PROPERTY);
	indigo_define_matching_property(AGENT_GUIDER_SETTINGS_PROPERTY);
	indigo_define_matching_property(AGENT_GUIDER_FLIP_REVERSES_DEC_PROPERTY);
	indigo_define_matching_property(AGENT_GUIDER_STARS_PROPERTY);
	indigo_define_matching_property(AGENT_GUIDER_SELECTION_PROPERTY);
	indigo_define_matching_property(AGENT_GUIDER_STATS_PROPERTY);
	indigo_define_matching_property(AGENT_GUIDER_DEC_MODE_PROPERTY);
	indigo_define_matching_property(AGENT_GUIDER_APPLY_DEC_BACKLASH_PROPERTY);
	indigo_define_matching_property(AGENT_START_PROCESS_PROPERTY);
	indigo_define_matching_property(AGENT_ABORT_PROCESS_PROPERTY);
	indigo_define_matching_property(AGENT_GUIDER_DITHERING_OFFSETS_PROPERTY);
	indigo_define_matching_property(AGENT_GUIDER_DITHERING_STRATEGY_PROPERTY);
	indigo_define_matching_property(AGENT_GUIDER_DITHER_PROPERTY);
	indigo_define_matching_property(AGENT_GUIDER_LOG_PROPERTY);
	indigo_define_matching_property(AGENT_PROCESS_FEATURES_PROPERTY);
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
				for (int i = 0; i < AGENT_GUIDER_SELECTION_STAR_COUNT_ITEM->number.value; i++) {
					indigo_item *item_x = AGENT_GUIDER_SELECTION_X_ITEM + 2 * i;
					indigo_item *item_y = AGENT_GUIDER_SELECTION_Y_ITEM + 2 * i;
					item_x->number.target = item_x->number.value = 0;
					item_y->number.target = item_y->number.value = 0;
				}
				indigo_update_property(device, AGENT_GUIDER_SELECTION_PROPERTY, NULL);
			}
		}
	} else if (indigo_property_match(AGENT_GUIDER_DETECTION_MODE_PROPERTY, property)) {
// -------------------------------------------------------------------------------- AGENT_GUIDER_DETECTION_MODE
		if (FILTER_DEVICE_CONTEXT->running_process) {
			indigo_update_property(device, AGENT_GUIDER_DETECTION_MODE_PROPERTY, "Warning: Detection mode can not be changed while process is running!");
			return INDIGO_OK;
		}
		indigo_property_copy_values(AGENT_GUIDER_DETECTION_MODE_PROPERTY, property, false);
		AGENT_GUIDER_DETECTION_MODE_PROPERTY->state = INDIGO_OK_STATE;
		save_config(device);
		indigo_update_property(device, AGENT_GUIDER_DETECTION_MODE_PROPERTY, NULL);
		return INDIGO_OK;
	} else if (indigo_property_match(AGENT_GUIDER_DEC_MODE_PROPERTY, property)) {
// -------------------------------------------------------------------------------- AGENT_GUIDER_DEC_MODE
		bool is_current_dec_guiding_both = AGENT_GUIDER_DEC_MODE_BOTH_ITEM->sw.value;
		bool is_requested_dec_guiding_both = indigo_get_switch(property, AGENT_GUIDER_DEC_MODE_BOTH_ITEM_NAME);
		if (
			(!FILTER_DEVICE_CONTEXT->running_process) ||
			(FILTER_DEVICE_CONTEXT->running_process && !AGENT_GUIDER_START_GUIDING_ITEM->sw.value) ||
			(FILTER_DEVICE_CONTEXT->running_process && AGENT_GUIDER_START_GUIDING_ITEM->sw.value && !(is_current_dec_guiding_both || is_requested_dec_guiding_both))
		) {
			indigo_property_copy_values(AGENT_GUIDER_DEC_MODE_PROPERTY, property, false);
			AGENT_GUIDER_DEC_MODE_PROPERTY->state = INDIGO_OK_STATE;
			save_config(device);
			indigo_update_property(device, AGENT_GUIDER_DEC_MODE_PROPERTY, NULL);
		} else {
			indigo_update_property(device, AGENT_GUIDER_DEC_MODE_PROPERTY, "Warning: Can not change declination guiding method to/from 'North and South' while guiding!");
		}
		return INDIGO_OK;
	} else if (indigo_property_match(AGENT_GUIDER_APPLY_DEC_BACKLASH_PROPERTY, property)) {
// -------------------------------------------------------------------------------- AGENT_GUIDER_APPLY_DEC_BACKLASH
		indigo_property_copy_values(AGENT_GUIDER_APPLY_DEC_BACKLASH_PROPERTY, property, false);
		AGENT_GUIDER_APPLY_DEC_BACKLASH_PROPERTY->state = INDIGO_OK_STATE;
		save_config(device);
		indigo_update_property(device, AGENT_GUIDER_APPLY_DEC_BACKLASH_PROPERTY, NULL);
		return INDIGO_OK;
	} else if (indigo_property_match(AGENT_GUIDER_MOUNT_COORDINATES_PROPERTY, property)) {
// -------------------------------------------------------------------------------- AGENT_GUIDER_MOUNT_COORDINATES
		double dec = AGENT_GUIDER_MOUNT_COORDINATES_DEC_ITEM->number.value;
		indigo_property_copy_values(AGENT_GUIDER_MOUNT_COORDINATES_PROPERTY, property, false);
		AGENT_GUIDER_MOUNT_COORDINATES_PROPERTY->state = INDIGO_OK_STATE;
		if (dec != AGENT_GUIDER_MOUNT_COORDINATES_DEC_ITEM->number.target) {
			DEVICE_PRIVATE_DATA->cos_dec = cos(-PI * AGENT_GUIDER_MOUNT_COORDINATES_DEC_ITEM->number.value / 180);
			if (DEVICE_PRIVATE_DATA->cos_dec == 0) {
				DEVICE_PRIVATE_DATA->cos_dec = MIN_COS_DEC;
			}
		}
		/* force -1, 0, 1 */
		if (AGENT_GUIDER_MOUNT_COORDINATES_SOP_ITEM->number.target != 0) {
			AGENT_GUIDER_MOUNT_COORDINATES_SOP_ITEM->number.value =
			AGENT_GUIDER_MOUNT_COORDINATES_SOP_ITEM->number.target = copysign(1.0, AGENT_GUIDER_MOUNT_COORDINATES_SOP_ITEM->number.target);
		}
		indigo_update_property(device, AGENT_GUIDER_MOUNT_COORDINATES_PROPERTY, NULL);
		return INDIGO_OK;
	} else if (indigo_property_match(AGENT_GUIDER_DITHERING_OFFSETS_PROPERTY, property)) {
// -------------------------------------------------------------------------------- AGENT_GUIDER_DITHERING_OFFSETS
		double dith_x = AGENT_GUIDER_DITHERING_OFFSETS_X_ITEM->number.target;
		double dith_y = AGENT_GUIDER_DITHERING_OFFSETS_Y_ITEM->number.target;
		indigo_property_copy_values(AGENT_GUIDER_DITHERING_OFFSETS_PROPERTY, property, false);
		if (!AGENT_GUIDER_DEC_MODE_BOTH_ITEM->sw.value) {
			/* If Dec guiding is not "North and South" do not dither in Dec, however if cos(angle) == 0 we end up in devision by 0.
			   In this case we set the limits -> DITH_X = 0 and DITH_Y = dith_total.
			   Note: we preserve the requested amount of dithering but we do it in RA only.
			*/
			double angle = -PI * get_rotation_angle(device) / 180;
			double sign = copysign(1.0, AGENT_GUIDER_DITHERING_OFFSETS_X_ITEM->number.target) * copysign(1.0, AGENT_GUIDER_DITHERING_OFFSETS_Y_ITEM->number.target);
			double dith_total = sign * sqrt(AGENT_GUIDER_DITHERING_OFFSETS_X_ITEM->number.target * AGENT_GUIDER_DITHERING_OFFSETS_X_ITEM->number.target + AGENT_GUIDER_DITHERING_OFFSETS_Y_ITEM->number.target * AGENT_GUIDER_DITHERING_OFFSETS_Y_ITEM->number.target);
			double cos_angle = cos(angle);
			if (cos_angle != 0) {
				double tan_angle = tan(angle);
				AGENT_GUIDER_DITHERING_OFFSETS_X_ITEM->number.value = dith_total / (cos_angle + tan_angle);
				AGENT_GUIDER_DITHERING_OFFSETS_Y_ITEM->number.value = AGENT_GUIDER_DITHERING_OFFSETS_X_ITEM->number.value * tan_angle;
			} else {
				AGENT_GUIDER_DITHERING_OFFSETS_X_ITEM->number.value = 0;
				AGENT_GUIDER_DITHERING_OFFSETS_Y_ITEM->number.value = dith_total;
			}
			if (dith_x != AGENT_GUIDER_DITHERING_OFFSETS_X_ITEM->number.target || dith_y != AGENT_GUIDER_DITHERING_OFFSETS_Y_ITEM->number.target) {
				INDIGO_DRIVER_DEBUG(DRIVER_NAME,
					"Dithering request altered as Dec dithering is disabled, requested X/Y: %.3f/%.3f calculated X/Y: %.3f/%.3f",
					AGENT_GUIDER_DITHERING_OFFSETS_X_ITEM->number.target,
					AGENT_GUIDER_DITHERING_OFFSETS_Y_ITEM->number.target,
					AGENT_GUIDER_DITHERING_OFFSETS_X_ITEM->number.value,
					AGENT_GUIDER_DITHERING_OFFSETS_Y_ITEM->number.value
				);
			}
		} else {
			if (dith_x != AGENT_GUIDER_DITHERING_OFFSETS_X_ITEM->number.target || dith_y != AGENT_GUIDER_DITHERING_OFFSETS_Y_ITEM->number.target) {
				INDIGO_DRIVER_DEBUG(DRIVER_NAME,
					"Dithering requested X/Y: %.3f/%.3f",
					AGENT_GUIDER_DITHERING_OFFSETS_X_ITEM->number.value,
					AGENT_GUIDER_DITHERING_OFFSETS_Y_ITEM->number.value
				);
			}
		}
		bool update_stats = false;
		if (DEVICE_PRIVATE_DATA->reference->algorithm == centroid) {
			AGENT_GUIDER_STATS_REFERENCE_X_ITEM->number.value = DEVICE_PRIVATE_DATA->reference->centroid_x + AGENT_GUIDER_DITHERING_OFFSETS_X_ITEM->number.value;
			AGENT_GUIDER_STATS_REFERENCE_Y_ITEM->number.value = DEVICE_PRIVATE_DATA->reference->centroid_y + AGENT_GUIDER_DITHERING_OFFSETS_Y_ITEM->number.value;
			update_stats = true;
		}
		/* Important: We compare dithering X and Y targets because values will be changed in case we dither in RA only and in this case
		   every guider settings item update (not only dithering related) will trigger dithering. Changing guider settings may happen in
		   the middle of the exposure.
		*/
		if (AGENT_GUIDER_STATS_PHASE_ITEM->number.value == INDIGO_GUIDER_PHASE_GUIDING && (dith_x != AGENT_GUIDER_DITHERING_OFFSETS_X_ITEM->number.target || dith_y != AGENT_GUIDER_DITHERING_OFFSETS_Y_ITEM->number.target)) {
			double diff_x = fabs(AGENT_GUIDER_DITHERING_OFFSETS_X_ITEM->number.value - dith_x);
			double diff_y = fabs(AGENT_GUIDER_DITHERING_OFFSETS_Y_ITEM->number.value - dith_y);
			DEVICE_PRIVATE_DATA->rmse_ra_sum =
			DEVICE_PRIVATE_DATA->rmse_dec_sum =
			DEVICE_PRIVATE_DATA->rmse_ra_s_sum =
			DEVICE_PRIVATE_DATA->rmse_dec_s_sum =
			DEVICE_PRIVATE_DATA->rmse_count = 0;
			DEVICE_PRIVATE_DATA->rmse_ra_threshold = 1.5 * AGENT_GUIDER_STATS_RMSE_RA_ITEM->number.value + AGENT_GUIDER_SETTINGS_MIN_ERR_ITEM->number.value/2.0;
			DEVICE_PRIVATE_DATA->rmse_dec_threshold = 1.5 * AGENT_GUIDER_STATS_RMSE_DEC_ITEM->number.value + AGENT_GUIDER_SETTINGS_MIN_ERR_ITEM->number.value/2.0;
			INDIGO_DRIVER_DEBUG(DRIVER_NAME, "Dithering RMSE RA threshold = %g, RMSE DEC threshold = %g ", DEVICE_PRIVATE_DATA->rmse_ra_threshold, DEVICE_PRIVATE_DATA->rmse_dec_threshold);
			AGENT_GUIDER_STATS_DITHERING_ITEM->number.value = round(1000 * fmax(fabs(diff_x), fabs(diff_y))) / 1000;
			update_stats = true;
		}
		if (update_stats) {
			indigo_update_property(device, AGENT_GUIDER_STATS_PROPERTY, NULL);
		}
		AGENT_GUIDER_DITHERING_OFFSETS_PROPERTY->state = INDIGO_OK_STATE;
		indigo_update_property(device, AGENT_GUIDER_DITHERING_OFFSETS_PROPERTY, NULL);
		return INDIGO_OK;
	} else if (indigo_property_match(AGENT_GUIDER_SETTINGS_PROPERTY, property)) {
// -------------------------------------------------------------------------------- AGENT_GUIDER_SETTINGS
		indigo_property_copy_values(AGENT_GUIDER_SETTINGS_PROPERTY, property, false);
		AGENT_GUIDER_SETTINGS_STACK_ITEM->number.value = AGENT_GUIDER_SETTINGS_STACK_ITEM->number.target = (int)AGENT_GUIDER_SETTINGS_STACK_ITEM->number.target;
		AGENT_GUIDER_SETTINGS_BL_STEPS_ITEM->number.value = AGENT_GUIDER_SETTINGS_BL_STEPS_ITEM->number.target = (int)AGENT_GUIDER_SETTINGS_BL_STEPS_ITEM->number.target;
		AGENT_GUIDER_SETTINGS_CAL_STEPS_ITEM->number.value = AGENT_GUIDER_SETTINGS_CAL_STEPS_ITEM->number.target = (int)AGENT_GUIDER_SETTINGS_CAL_STEPS_ITEM->number.target;
		/* force -1, 0, 1 */
		if (AGENT_GUIDER_SETTINGS_SOP_ITEM->number.target != 0) {
			AGENT_GUIDER_SETTINGS_SOP_ITEM->number.value =
			AGENT_GUIDER_SETTINGS_SOP_ITEM->number.target = copysign(1.0, AGENT_GUIDER_SETTINGS_SOP_ITEM->number.target);
		}
		save_config(device);
		AGENT_GUIDER_SETTINGS_PROPERTY->state = INDIGO_OK_STATE;
		indigo_update_property(device, AGENT_GUIDER_SETTINGS_PROPERTY, NULL);
	} else if (indigo_property_match(AGENT_GUIDER_FLIP_REVERSES_DEC_PROPERTY, property)) {
	// -------------------------------------------------------------------------------- AGENT_GUIDER_FLIP_REVERSES_DEC
		indigo_property_copy_values(AGENT_GUIDER_FLIP_REVERSES_DEC_PROPERTY, property, false);
		AGENT_GUIDER_FLIP_REVERSES_DEC_PROPERTY->state = INDIGO_OK_STATE;
		save_config(device);
		indigo_update_property(device, AGENT_GUIDER_FLIP_REVERSES_DEC_PROPERTY, NULL);
		return INDIGO_OK;
	} else if (indigo_property_match(AGENT_GUIDER_STARS_PROPERTY, property)) {
	// -------------------------------------------------------------------------------- AGENT_GUIDER_STARS
		if (AGENT_START_PROCESS_PROPERTY->state != INDIGO_BUSY_STATE && AGENT_GUIDER_STARS_PROPERTY->state != INDIGO_BUSY_STATE) {
			indigo_property_copy_values(AGENT_GUIDER_STARS_PROPERTY, property, false);
			if (AGENT_GUIDER_STARS_REFRESH_ITEM->sw.value) {
				AGENT_GUIDER_STARS_REFRESH_ITEM->sw.value = false;
				AGENT_GUIDER_STARS_PROPERTY->state = INDIGO_BUSY_STATE;
				indigo_update_property(device, AGENT_GUIDER_STARS_PROPERTY, NULL);
				indigo_set_timer(device, 0, find_stars_process, NULL);
			} else {
				for (int i = 1; i < AGENT_GUIDER_STARS_PROPERTY->count; i++) {
					if (AGENT_GUIDER_STARS_PROPERTY->items[i].sw.value) {
						int j = atoi(AGENT_GUIDER_STARS_PROPERTY->items[i].name);
						AGENT_GUIDER_SELECTION_X_ITEM->number.target = AGENT_GUIDER_SELECTION_X_ITEM->number.value = DEVICE_PRIVATE_DATA->stars[j].x;
						AGENT_GUIDER_SELECTION_Y_ITEM->number.target = AGENT_GUIDER_SELECTION_Y_ITEM->number.value = DEVICE_PRIVATE_DATA->stars[j].y;
						indigo_update_property(device, AGENT_GUIDER_SELECTION_PROPERTY, NULL);
						AGENT_GUIDER_STARS_PROPERTY->items[i].sw.value = false;
					}
				}
				AGENT_GUIDER_STARS_PROPERTY->state = INDIGO_OK_STATE;
			}
		}
		indigo_update_property(device, AGENT_GUIDER_STARS_PROPERTY, NULL);
		return INDIGO_OK;
	} else if (indigo_property_match(AGENT_GUIDER_SELECTION_PROPERTY, property)) {
// -------------------------------------------------------------------------------- AGENT_GUIDER_SELECTION
		if (FILTER_DEVICE_CONTEXT->running_process) {
			indigo_item *item = indigo_get_item(property, AGENT_GUIDER_SELECTION_EDGE_CLIPPING_ITEM_NAME);
			if (item && AGENT_GUIDER_SELECTION_EDGE_CLIPPING_ITEM->number.value != item->number.value) {
				indigo_update_property(device, AGENT_GUIDER_SELECTION_PROPERTY, "Warning: Edge clipping can not be changed while process is running!");
				return INDIGO_OK;
			}
			item = indigo_get_item(property, AGENT_GUIDER_SELECTION_STAR_COUNT_ITEM_NAME);
			if (item && AGENT_GUIDER_SELECTION_STAR_COUNT_ITEM->number.value != item->number.value) {
				indigo_update_property(device, AGENT_GUIDER_SELECTION_PROPERTY, "Warning: Star count can not be changed while process is running!");
				return INDIGO_OK;
			}
		}
		int count = AGENT_GUIDER_SELECTION_STAR_COUNT_ITEM->number.value;
		double include_left = AGENT_GUIDER_SELECTION_INCLUDE_LEFT_ITEM->number.value;
		double include_top = AGENT_GUIDER_SELECTION_INCLUDE_TOP_ITEM->number.value;
		double include_width = AGENT_GUIDER_SELECTION_INCLUDE_WIDTH_ITEM->number.value;
		double include_height = AGENT_GUIDER_SELECTION_INCLUDE_HEIGHT_ITEM->number.value;
		double exclude_left = AGENT_GUIDER_SELECTION_EXCLUDE_LEFT_ITEM->number.value;
		double exclude_top = AGENT_GUIDER_SELECTION_EXCLUDE_TOP_ITEM->number.value;
		double exclude_width = AGENT_GUIDER_SELECTION_EXCLUDE_WIDTH_ITEM->number.value;
		double exclude_height = AGENT_GUIDER_SELECTION_EXCLUDE_HEIGHT_ITEM->number.value;
		indigo_property_copy_values(AGENT_GUIDER_SELECTION_PROPERTY, property, false);
		bool force = include_left != AGENT_GUIDER_SELECTION_INCLUDE_LEFT_ITEM->number.value || include_top != AGENT_GUIDER_SELECTION_INCLUDE_TOP_ITEM->number.value || include_width != AGENT_GUIDER_SELECTION_INCLUDE_WIDTH_ITEM->number.value || include_height != AGENT_GUIDER_SELECTION_INCLUDE_HEIGHT_ITEM->number.value || exclude_left != AGENT_GUIDER_SELECTION_EXCLUDE_LEFT_ITEM->number.value|| exclude_top != AGENT_GUIDER_SELECTION_EXCLUDE_TOP_ITEM->number.value || exclude_width != AGENT_GUIDER_SELECTION_EXCLUDE_WIDTH_ITEM->number.value || exclude_height != AGENT_GUIDER_SELECTION_EXCLUDE_HEIGHT_ITEM->number.value;
		validate_include_region(device, force);
		AGENT_GUIDER_SELECTION_STAR_COUNT_ITEM->number.value = AGENT_GUIDER_SELECTION_STAR_COUNT_ITEM->number.target = (int)AGENT_GUIDER_SELECTION_STAR_COUNT_ITEM->number.target;
		if (count != AGENT_GUIDER_SELECTION_STAR_COUNT_ITEM->number.value) {
			indigo_delete_property(device, AGENT_GUIDER_SELECTION_PROPERTY, NULL);
			AGENT_GUIDER_SELECTION_PROPERTY->count = (AGENT_GUIDER_SELECTION_X_ITEM - AGENT_GUIDER_SELECTION_PROPERTY->items) + 2 * AGENT_GUIDER_SELECTION_STAR_COUNT_ITEM->number.value;
			for (int i = 0; i < AGENT_GUIDER_SELECTION_STAR_COUNT_ITEM->number.value; i++) {
				indigo_item *item_x = AGENT_GUIDER_SELECTION_X_ITEM + 2 * i;
				indigo_item *item_y = AGENT_GUIDER_SELECTION_Y_ITEM + 2 * i;
				item_x->number.value = item_x->number.target = 0;
				item_y->number.value = item_y->number.target = 0;
			}
			indigo_define_property(device, AGENT_GUIDER_SELECTION_PROPERTY, NULL);
		}
		save_config(device);
		AGENT_GUIDER_SELECTION_PROPERTY->state = INDIGO_OK_STATE;
		indigo_update_property(device, AGENT_GUIDER_SELECTION_PROPERTY, NULL);
		return INDIGO_OK;
	} else if (indigo_property_match(AGENT_START_PROCESS_PROPERTY, property)) {
// -------------------------------------------------------------------------------- AGENT_START_PROCESS
		if (AGENT_START_PROCESS_PROPERTY->state != INDIGO_BUSY_STATE && AGENT_GUIDER_STARS_PROPERTY->state != INDIGO_BUSY_STATE) {
			indigo_property_copy_values(AGENT_START_PROCESS_PROPERTY, property, false);
			if (AGENT_GUIDER_CLEAR_SELECTION_ITEM->sw.value) {
				AGENT_START_PROCESS_PROPERTY->state = INDIGO_BUSY_STATE;
				indigo_set_timer(device, 0, clear_selection_process, NULL);
			} else if (INDIGO_FILTER_CCD_SELECTED) {
				if (AGENT_GUIDER_START_PREVIEW_1_ITEM->sw.value) {
					AGENT_START_PROCESS_PROPERTY->state = INDIGO_BUSY_STATE;
					indigo_set_timer(device, 0, preview_1_process, NULL);
					indigo_update_property(device, AGENT_START_PROCESS_PROPERTY, NULL);
				} else if (AGENT_GUIDER_START_PREVIEW_ITEM->sw.value) {
					AGENT_START_PROCESS_PROPERTY->state = INDIGO_BUSY_STATE;
					indigo_set_timer(device, 0, preview_process, NULL);
					indigo_update_property(device, AGENT_START_PROCESS_PROPERTY, NULL);
				} else if (INDIGO_FILTER_GUIDER_SELECTED) {
					if (AGENT_GUIDER_START_CALIBRATION_ITEM->sw.value) {
						AGENT_START_PROCESS_PROPERTY->state = INDIGO_BUSY_STATE;
						indigo_set_timer(device, 0, calibrate_process, NULL);
					} else if (AGENT_GUIDER_START_CALIBRATION_AND_GUIDING_ITEM->sw.value) {
						AGENT_START_PROCESS_PROPERTY->state = INDIGO_BUSY_STATE;
						indigo_set_timer(device, 0, calibrate_and_guide_process, NULL);
					} else if (AGENT_GUIDER_START_GUIDING_ITEM->sw.value) {
						AGENT_START_PROCESS_PROPERTY->state = INDIGO_BUSY_STATE;
						indigo_set_timer(device, 0, guide_process, NULL);
					}
					indigo_update_property(device, AGENT_START_PROCESS_PROPERTY, NULL);
				} else {
					AGENT_GUIDER_START_PREVIEW_1_ITEM->sw.value =
					AGENT_GUIDER_START_PREVIEW_ITEM->sw.value =
					AGENT_GUIDER_START_CALIBRATION_ITEM->sw.value =
					AGENT_GUIDER_START_CALIBRATION_AND_GUIDING_ITEM->sw.value =
					AGENT_GUIDER_START_GUIDING_ITEM->sw.value = false;
					AGENT_START_PROCESS_PROPERTY->state = INDIGO_ALERT_STATE;
					indigo_update_property(device, AGENT_START_PROCESS_PROPERTY, "No guider is selected");
				}
			} else {
				AGENT_GUIDER_START_PREVIEW_1_ITEM->sw.value =
				AGENT_GUIDER_START_PREVIEW_ITEM->sw.value =
				AGENT_GUIDER_START_CALIBRATION_ITEM->sw.value =
				AGENT_GUIDER_START_CALIBRATION_AND_GUIDING_ITEM->sw.value =
				AGENT_GUIDER_START_GUIDING_ITEM->sw.value = false;
				AGENT_START_PROCESS_PROPERTY->state = INDIGO_ALERT_STATE;
				indigo_update_property(device, AGENT_START_PROCESS_PROPERTY, "No guider camera is selected");
			}
		}
		return INDIGO_OK;
	} else if (indigo_property_match(AGENT_ABORT_PROCESS_PROPERTY, property)) {
// -------------------------------------------------------------------------------- AGENT_ABORT_PROCESS
		if (AGENT_START_PROCESS_PROPERTY->state == INDIGO_BUSY_STATE || AGENT_GUIDER_STARS_PROPERTY->state == INDIGO_BUSY_STATE) {
			indigo_property_copy_values(AGENT_ABORT_PROCESS_PROPERTY, property, false);
			AGENT_ABORT_PROCESS_PROPERTY->state = INDIGO_BUSY_STATE;
			abort_process(device);
		}
		AGENT_ABORT_PROCESS_ITEM->sw.value = false;
		indigo_update_property(device, AGENT_ABORT_PROCESS_PROPERTY, NULL);
		return INDIGO_OK;
	} else if (indigo_property_match(AGENT_GUIDER_DITHERING_STRATEGY_PROPERTY, property)) {
// -------------------------------------------------------------------------------- AGENT_DITHERING_STRATEGY
		indigo_property_copy_values(AGENT_GUIDER_DITHERING_STRATEGY_PROPERTY, property, false);
		AGENT_GUIDER_DITHERING_STRATEGY_PROPERTY->state = INDIGO_OK_STATE;
		srand((unsigned int)time(NULL));
		DEVICE_PRIVATE_DATA->dither_num = 0;
		save_config(device);
		indigo_update_property(device, AGENT_GUIDER_DITHERING_STRATEGY_PROPERTY, NULL);
		return INDIGO_OK;
	} else if (indigo_property_match(AGENT_GUIDER_DITHER_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- AGENT_DITHER
		if (AGENT_GUIDER_DITHER_PROPERTY->state != INDIGO_BUSY_STATE) {
			indigo_property_copy_values(AGENT_GUIDER_DITHER_PROPERTY, property, false);
			if (AGENT_GUIDER_DITHER_TRIGGER_ITEM->sw.value) {
				AGENT_GUIDER_DITHER_PROPERTY->state = INDIGO_BUSY_STATE;
				indigo_update_property(device, AGENT_GUIDER_DITHER_PROPERTY, NULL);
				indigo_set_timer(device, 0, do_dither, NULL);
			} else if (AGENT_GUIDER_DITHER_RESET_ITEM->sw.value) {
				srand((unsigned int)time(NULL));
				DEVICE_PRIVATE_DATA->dither_num = 0;
				AGENT_GUIDER_DITHER_TRIGGER_ITEM->sw.value = false;
				AGENT_GUIDER_DITHER_RESET_ITEM->sw.value = false;
				AGENT_GUIDER_DITHER_PROPERTY->state = INDIGO_OK_STATE;
				indigo_update_property(device, AGENT_GUIDER_DITHER_PROPERTY, "Dithering reset");
			} else {
				AGENT_GUIDER_DITHER_TRIGGER_ITEM->sw.value = false;
				AGENT_GUIDER_DITHER_RESET_ITEM->sw.value = false;
				indigo_update_property(device, AGENT_GUIDER_DITHER_PROPERTY, NULL);
			}
		}
		return INDIGO_OK;
	} else if (indigo_property_match(AGENT_GUIDER_LOG_PROPERTY, property)) {
// -------------------------------------------------------------------------------- AGENT_GUIDER_LOG
		if (AGENT_START_PROCESS_PROPERTY->state == INDIGO_BUSY_STATE) {
			AGENT_GUIDER_LOG_PROPERTY->state = INDIGO_ALERT_STATE;
			indigo_update_property(device, AGENT_GUIDER_LOG_PROPERTY, "Guider log settings can't be changed while any process is in progress");
		} else {
			indigo_property_copy_values(AGENT_GUIDER_LOG_PROPERTY, property, false);
			char *path = AGENT_GUIDER_LOG_DIR_ITEM->text.value;
			int len = (int)strlen(path);
			if (path[len - 1] != '/') {
				path[len++] = '/';
				path[len] = 0;
			}
			AGENT_GUIDER_LOG_PROPERTY->state = INDIGO_OK_STATE;
			indigo_update_property(device, AGENT_GUIDER_LOG_PROPERTY, NULL);
			save_config(device);
		}
		return INDIGO_OK;
	} else if (indigo_property_match(AGENT_PROCESS_FEATURES_PROPERTY, property)) {
// -------------------------------------------------------------------------------- AGENT_PROCESS_FEATURES
		bool fail_on_calibration_error = AGENT_GUIDER_FAIL_ON_CALIBRATION_ERROR_ITEM->sw.value;
		bool reset_on_calibration_error = AGENT_GUIDER_RESET_ON_CALIBRATION_ERROR_ITEM->sw.value;
		bool fail_on_guiding_error = AGENT_GUIDER_FAIL_ON_GUIDING_ERROR_ITEM->sw.value;
		bool continue_on_guiding_error = AGENT_GUIDER_CONTINUE_ON_GUIDING_ERROR_ITEM->sw.value;
		bool reset_on_guiding_error = AGENT_GUIDER_RESET_ON_GUIDING_ERROR_ITEM->sw.value;
		indigo_property_copy_values(AGENT_PROCESS_FEATURES_PROPERTY, property, false);
		if (!reset_on_calibration_error && AGENT_GUIDER_RESET_ON_CALIBRATION_ERROR_ITEM->sw.value) {
			AGENT_GUIDER_FAIL_ON_CALIBRATION_ERROR_ITEM->sw.value = false;
		} else if (!fail_on_calibration_error && AGENT_GUIDER_FAIL_ON_CALIBRATION_ERROR_ITEM->sw.value) {
			AGENT_GUIDER_RESET_ON_CALIBRATION_ERROR_ITEM->sw.value = false;
		}
		if (!reset_on_guiding_error && AGENT_GUIDER_RESET_ON_GUIDING_ERROR_ITEM->sw.value) {
			AGENT_GUIDER_FAIL_ON_GUIDING_ERROR_ITEM->sw.value = false;
			AGENT_GUIDER_CONTINUE_ON_GUIDING_ERROR_ITEM->sw.value = false;
		} else if (!continue_on_guiding_error && AGENT_GUIDER_CONTINUE_ON_GUIDING_ERROR_ITEM->sw.value) {
			AGENT_GUIDER_FAIL_ON_GUIDING_ERROR_ITEM->sw.value = false;
			AGENT_GUIDER_RESET_ON_GUIDING_ERROR_ITEM->sw.value = false;
		} else if (!fail_on_guiding_error && AGENT_GUIDER_FAIL_ON_GUIDING_ERROR_ITEM->sw.value) {
			AGENT_GUIDER_CONTINUE_ON_GUIDING_ERROR_ITEM->sw.value = false;
			AGENT_GUIDER_RESET_ON_GUIDING_ERROR_ITEM->sw.value = false;
		}
		AGENT_PROCESS_FEATURES_PROPERTY->state = INDIGO_OK_STATE;
		indigo_update_property(device, AGENT_PROCESS_FEATURES_PROPERTY, NULL);
		save_config(device);
		return INDIGO_OK;
	} else if (indigo_property_match(ADDITIONAL_INSTANCES_PROPERTY, property)) {
// -------------------------------------------------------------------------------- ADDITIONAL_INSTANCES
		if (indigo_filter_change_property(device, client, property) == INDIGO_OK) {
			save_config(device);
		}
		return INDIGO_OK;
	}
	return indigo_filter_change_property(device, client, property);
}

static indigo_result agent_device_detach(indigo_device *device) {
	assert(device != NULL);
	save_config(device);
	indigo_release_property(AGENT_GUIDER_DETECTION_MODE_PROPERTY);
	indigo_release_property(AGENT_START_PROCESS_PROPERTY);
	indigo_release_property(AGENT_ABORT_PROCESS_PROPERTY);
	indigo_release_property(AGENT_GUIDER_MOUNT_COORDINATES_PROPERTY);
	indigo_release_property(AGENT_GUIDER_SETTINGS_PROPERTY);
	indigo_release_property(AGENT_GUIDER_FLIP_REVERSES_DEC_PROPERTY);
	indigo_release_property(AGENT_GUIDER_STARS_PROPERTY);
	indigo_release_property(AGENT_GUIDER_SELECTION_PROPERTY);
	indigo_release_property(AGENT_GUIDER_STATS_PROPERTY);
	indigo_release_property(AGENT_GUIDER_DEC_MODE_PROPERTY);
	indigo_release_property(AGENT_GUIDER_APPLY_DEC_BACKLASH_PROPERTY);
	indigo_release_property(AGENT_GUIDER_DITHERING_OFFSETS_PROPERTY);
	indigo_release_property(AGENT_GUIDER_DITHERING_STRATEGY_PROPERTY);
	indigo_release_property(AGENT_GUIDER_DITHER_PROPERTY);
	indigo_release_property(AGENT_GUIDER_LOG_PROPERTY);
	indigo_release_property(AGENT_PROCESS_FEATURES_PROPERTY);
	for (int i = 0; i <= INDIGO_MAX_MULTISTAR_COUNT; i++)
		indigo_delete_frame_digest(DEVICE_PRIVATE_DATA->reference + i);
	pthread_mutex_destroy(&DEVICE_PRIVATE_DATA->mutex);
	pthread_mutex_destroy(&DEVICE_PRIVATE_DATA->last_image_mutex);
	indigo_safe_free(DEVICE_PRIVATE_DATA->last_image);
	DEVICE_PRIVATE_DATA->last_image_size = 0;
	return indigo_filter_device_detach(device);
}

// -------------------------------------------------------------------------------- INDIGO agent client implementation

static indigo_result agent_define_property(indigo_client *client, indigo_device *device, indigo_property *property, const char *message) {
	if (device == FILTER_CLIENT_CONTEXT->device) {
		if (!strcmp(property->name, CCD_BIN_PROPERTY_NAME)) {
			if (property->state == INDIGO_OK_STATE) {
				bool reset_selection = false;
				for (int i = 0; i < property->count; i++) {
					indigo_item *item = property->items + i;
					if (strcmp(item->name, CCD_BIN_HORIZONTAL_ITEM_NAME) == 0) {
						if (CLIENT_PRIVATE_DATA->bin_x != item->number.value) {
							CLIENT_PRIVATE_DATA->bin_x = item->number.value;
							reset_selection = true;
						}
					} else if (strcmp(item->name, CCD_BIN_VERTICAL_ITEM_NAME) == 0) {
						if (CLIENT_PRIVATE_DATA->bin_y != item->number.value) {
							CLIENT_PRIVATE_DATA->bin_y = item->number.value;
							reset_selection = true;
						}
					}
				}
				if (reset_selection) {
					CLIENT_PRIVATE_DATA->last_width = CLIENT_PRIVATE_DATA->frame[2] / CLIENT_PRIVATE_DATA->bin_x;
					CLIENT_PRIVATE_DATA->last_height = CLIENT_PRIVATE_DATA->frame[3] / CLIENT_PRIVATE_DATA->bin_y;
					AGENT_GUIDER_SELECTION_INCLUDE_LEFT_ITEM->number.value = AGENT_GUIDER_SELECTION_INCLUDE_TOP_ITEM->number.value = AGENT_GUIDER_SELECTION_INCLUDE_WIDTH_ITEM->number.value = AGENT_GUIDER_SELECTION_INCLUDE_HEIGHT_ITEM->number.value = AGENT_GUIDER_SELECTION_EXCLUDE_LEFT_ITEM->number.value = AGENT_GUIDER_SELECTION_EXCLUDE_TOP_ITEM->number.value = AGENT_GUIDER_SELECTION_EXCLUDE_WIDTH_ITEM->number.value = AGENT_GUIDER_SELECTION_EXCLUDE_HEIGHT_ITEM->number.value = 0;
					validate_include_region(device, false);
					clear_selection(device);
				}
			}
		} else {
			snoop_changes(client, device, property);
		}
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
							indigo_update_property(device, AGENT_GUIDER_SELECTION_PROPERTY, NULL);
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
							CLIENT_PRIVATE_DATA->bin_x = item->number.value;
							reset_selection = true;
						}
					} else if (strcmp(item->name, CCD_BIN_VERTICAL_ITEM_NAME) == 0) {
						if (CLIENT_PRIVATE_DATA->bin_y != item->number.value) {
							CLIENT_PRIVATE_DATA->bin_y = item->number.value;
							reset_selection = true;
						}
					}
				}
				if (reset_selection) {
					CLIENT_PRIVATE_DATA->last_width = CLIENT_PRIVATE_DATA->frame[2] / CLIENT_PRIVATE_DATA->bin_x;
					CLIENT_PRIVATE_DATA->last_height = CLIENT_PRIVATE_DATA->frame[3] / CLIENT_PRIVATE_DATA->bin_y;
					AGENT_GUIDER_SELECTION_INCLUDE_LEFT_ITEM->number.value = AGENT_GUIDER_SELECTION_INCLUDE_TOP_ITEM->number.value = AGENT_GUIDER_SELECTION_INCLUDE_WIDTH_ITEM->number.value = AGENT_GUIDER_SELECTION_INCLUDE_HEIGHT_ITEM->number.value = AGENT_GUIDER_SELECTION_EXCLUDE_LEFT_ITEM->number.value = AGENT_GUIDER_SELECTION_EXCLUDE_TOP_ITEM->number.value = AGENT_GUIDER_SELECTION_EXCLUDE_WIDTH_ITEM->number.value = AGENT_GUIDER_SELECTION_EXCLUDE_HEIGHT_ITEM->number.value = 0;
					validate_include_region(device, false);
					clear_selection(device);
				}
			}
		} else {
			snoop_changes(client, device, property);
		}
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

static guider_agent_private_data *private_data = NULL;

static indigo_device *agent_device = NULL;
static indigo_client *agent_client = NULL;

indigo_result indigo_agent_guider(indigo_driver_action action, indigo_driver_info *info) {
	static indigo_device agent_device_template = INDIGO_DEVICE_INITIALIZER(
		GUIDER_AGENT_NAME,
		agent_device_attach,
		agent_enumerate_properties,
		agent_change_property,
		NULL,
		agent_device_detach
	);

	static indigo_client agent_client_template = {
		GUIDER_AGENT_NAME, false, NULL, INDIGO_OK, INDIGO_VERSION_CURRENT, NULL,
		indigo_filter_client_attach,
		agent_define_property,
		agent_update_property,
		agent_delete_property,
		NULL,
		indigo_filter_client_detach
	};

	static indigo_driver_action last_action = INDIGO_DRIVER_SHUTDOWN;

	SET_DRIVER_INFO(info, GUIDER_AGENT_NAME, __FUNCTION__, DRIVER_VERSION, false, last_action);

	if (action == last_action)
		return INDIGO_OK;

	switch(action) {
		case INDIGO_DRIVER_INIT:
			last_action = action;
			private_data = indigo_safe_malloc(sizeof(guider_agent_private_data));
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
