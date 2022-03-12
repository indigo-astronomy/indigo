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

#define DRIVER_VERSION 0x0016
#define DRIVER_NAME	"indigo_agent_guider"

#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <math.h>
#include <assert.h>
#include <pthread.h>

#include <indigo/indigo_driver_xml.h>
#include <indigo/indigo_filter.h>
#include <indigo/indigo_ccd_driver.h>
#include <indigo/indigo_raw_utils.h>

#include "indigo_agent_guider.h"

#define PI (3.14159265358979)
#define PI2 (PI/2)

#define DEVICE_PRIVATE_DATA										((agent_private_data *)device->private_data)
#define CLIENT_PRIVATE_DATA										((agent_private_data *)FILTER_CLIENT_CONTEXT->device->private_data)

#define AGENT_GUIDER_DETECTION_MODE_PROPERTY		(DEVICE_PRIVATE_DATA->agent_guider_detection_mode_property)
#define AGENT_GUIDER_DETECTION_DONUTS_ITEM  		(AGENT_GUIDER_DETECTION_MODE_PROPERTY->items+0)
#define AGENT_GUIDER_DETECTION_SELECTION_ITEM 	(AGENT_GUIDER_DETECTION_MODE_PROPERTY->items+1)
#define AGENT_GUIDER_DETECTION_CENTROID_ITEM  	(AGENT_GUIDER_DETECTION_MODE_PROPERTY->items+2)

#define AGENT_GUIDER_DEC_MODE_PROPERTY				(DEVICE_PRIVATE_DATA->agent_guider_dec_mode_property)
#define AGENT_GUIDER_DEC_MODE_BOTH_ITEM    		(AGENT_GUIDER_DEC_MODE_PROPERTY->items+0)
#define AGENT_GUIDER_DEC_MODE_NORTH_ITEM    	(AGENT_GUIDER_DEC_MODE_PROPERTY->items+1)
#define AGENT_GUIDER_DEC_MODE_SOUTH_ITEM    	(AGENT_GUIDER_DEC_MODE_PROPERTY->items+2)
#define AGENT_GUIDER_DEC_MODE_NONE_ITEM    		(AGENT_GUIDER_DEC_MODE_PROPERTY->items+3)

#define AGENT_START_PROCESS_PROPERTY					(DEVICE_PRIVATE_DATA->agent_start_process_property)
#define AGENT_GUIDER_START_PREVIEW_ITEM  			(AGENT_START_PROCESS_PROPERTY->items+0)
#define AGENT_GUIDER_START_CALIBRATION_ITEM 	(AGENT_START_PROCESS_PROPERTY->items+1)
#define AGENT_GUIDER_START_CALIBRATION_AND_GUIDING_ITEM 	(AGENT_START_PROCESS_PROPERTY->items+2)
#define AGENT_GUIDER_START_GUIDING_ITEM 			(AGENT_START_PROCESS_PROPERTY->items+3)

#define AGENT_ABORT_PROCESS_PROPERTY					(DEVICE_PRIVATE_DATA->agent_abort_process_property)
#define AGENT_ABORT_PROCESS_ITEM      				(AGENT_ABORT_PROCESS_PROPERTY->items+0)

#define AGENT_GUIDER_SETTINGS_PROPERTY				(DEVICE_PRIVATE_DATA->agent_settings_property)
#define AGENT_GUIDER_SETTINGS_EXPOSURE_ITEM   (AGENT_GUIDER_SETTINGS_PROPERTY->items+0)
#define AGENT_GUIDER_SETTINGS_DELAY_ITEM   		(AGENT_GUIDER_SETTINGS_PROPERTY->items+1)
#define AGENT_GUIDER_SETTINGS_STEP_ITEM       (AGENT_GUIDER_SETTINGS_PROPERTY->items+2)
#define AGENT_GUIDER_SETTINGS_BL_STEPS_ITEM  	(AGENT_GUIDER_SETTINGS_PROPERTY->items+3)
#define AGENT_GUIDER_SETTINGS_BL_DRIFT_ITEM  	(AGENT_GUIDER_SETTINGS_PROPERTY->items+4)
#define AGENT_GUIDER_SETTINGS_CAL_STEPS_ITEM  (AGENT_GUIDER_SETTINGS_PROPERTY->items+5)
#define AGENT_GUIDER_SETTINGS_CAL_DRIFT_ITEM  (AGENT_GUIDER_SETTINGS_PROPERTY->items+6)
#define AGENT_GUIDER_SETTINGS_ANGLE_ITEM      (AGENT_GUIDER_SETTINGS_PROPERTY->items+7)
#define AGENT_GUIDER_SETTINGS_BACKLASH_ITEM   (AGENT_GUIDER_SETTINGS_PROPERTY->items+8)
#define AGENT_GUIDER_SETTINGS_SPEED_RA_ITEM   (AGENT_GUIDER_SETTINGS_PROPERTY->items+9)
#define AGENT_GUIDER_SETTINGS_SPEED_DEC_ITEM  (AGENT_GUIDER_SETTINGS_PROPERTY->items+10)
#define AGENT_GUIDER_SETTINGS_MIN_ERR_ITEM  	(AGENT_GUIDER_SETTINGS_PROPERTY->items+11)
#define AGENT_GUIDER_SETTINGS_MIN_PULSE_ITEM  (AGENT_GUIDER_SETTINGS_PROPERTY->items+12)
#define AGENT_GUIDER_SETTINGS_MAX_PULSE_ITEM  (AGENT_GUIDER_SETTINGS_PROPERTY->items+13)
#define AGENT_GUIDER_SETTINGS_AGG_RA_ITEM  		(AGENT_GUIDER_SETTINGS_PROPERTY->items+14)
#define AGENT_GUIDER_SETTINGS_AGG_DEC_ITEM  	(AGENT_GUIDER_SETTINGS_PROPERTY->items+15)
#define AGENT_GUIDER_SETTINGS_I_GAIN_RA_ITEM		(AGENT_GUIDER_SETTINGS_PROPERTY->items+16)
#define AGENT_GUIDER_SETTINGS_I_GAIN_DEC_ITEM  	(AGENT_GUIDER_SETTINGS_PROPERTY->items+17)
#define AGENT_GUIDER_SETTINGS_STACK_ITEM  		(AGENT_GUIDER_SETTINGS_PROPERTY->items+18)
#define AGENT_GUIDER_SETTINGS_DITH_X_ITEM  		(AGENT_GUIDER_SETTINGS_PROPERTY->items+19)
#define AGENT_GUIDER_SETTINGS_DITH_Y_ITEM  		(AGENT_GUIDER_SETTINGS_PROPERTY->items+20)
#define AGENT_GUIDER_SETTINGS_DITH_LIMIT_ITEM	(AGENT_GUIDER_SETTINGS_PROPERTY->items+21)

#define MAX_STAR_COUNT												50
#define AGENT_GUIDER_STARS_PROPERTY						(DEVICE_PRIVATE_DATA->agent_stars_property)
#define AGENT_GUIDER_STARS_REFRESH_ITEM  			(AGENT_GUIDER_STARS_PROPERTY->items+0)

#define AGENT_GUIDER_SELECTION_PROPERTY				(DEVICE_PRIVATE_DATA->agent_selection_property)
#define AGENT_GUIDER_SELECTION_RADIUS_ITEM  	(AGENT_GUIDER_SELECTION_PROPERTY->items+0)
#define AGENT_GUIDER_SELECTION_SUBFRAME_ITEM	(AGENT_GUIDER_SELECTION_PROPERTY->items+1)
#define AGENT_GUIDER_SELECTION_EDGE_CLIPPING_ITEM	(AGENT_GUIDER_SELECTION_PROPERTY->items+2)
#define AGENT_GUIDER_SELECTION_STAR_COUNT_ITEM (AGENT_GUIDER_SELECTION_PROPERTY->items+3)
#define AGENT_GUIDER_SELECTION_X_ITEM  				(AGENT_GUIDER_SELECTION_PROPERTY->items+4)
#define AGENT_GUIDER_SELECTION_Y_ITEM  				(AGENT_GUIDER_SELECTION_PROPERTY->items+5)

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
#define AGENT_GUIDER_STATS_CORR_RA_ITEM      	(AGENT_GUIDER_STATS_PROPERTY->items+8)
#define AGENT_GUIDER_STATS_CORR_DEC_ITEM      (AGENT_GUIDER_STATS_PROPERTY->items+9)
#define AGENT_GUIDER_STATS_RMSE_RA_ITEM      	(AGENT_GUIDER_STATS_PROPERTY->items+10)
#define AGENT_GUIDER_STATS_RMSE_DEC_ITEM      (AGENT_GUIDER_STATS_PROPERTY->items+11)
#define AGENT_GUIDER_STATS_SNR_ITEM      			(AGENT_GUIDER_STATS_PROPERTY->items+12)
#define AGENT_GUIDER_STATS_DELAY_ITEM      		(AGENT_GUIDER_STATS_PROPERTY->items+13)
#define AGENT_GUIDER_STATS_DITHERING_ITEM			(AGENT_GUIDER_STATS_PROPERTY->items+14)

#define BUSY_TIMEOUT 5

#define DIGEST_CONVERGE_ITERATIONS 3

typedef struct {
	indigo_property *agent_guider_detection_mode_property;
	indigo_property *agent_guider_dec_mode_property;
	indigo_property *agent_start_process_property;
	indigo_property *agent_abort_process_property;
	indigo_property *agent_settings_property;
	indigo_property *agent_stars_property;
	indigo_property *agent_selection_property;
	indigo_property *agent_stats_property;
	indigo_property *saved_frame;
	double saved_frame_left, saved_frame_top;
	bool properties_defined;
	indigo_star_detection stars[MAX_STAR_COUNT];
	indigo_frame_digest reference[MAX_MULTISTAR_COUNT + 1];
	double drift_x, drift_y, drift;
	double avg_drift_x, avg_drift_y;
	double rmse_ra_sum, rmse_dec_sum;
	double rmse_ra_threshold, rmse_dec_threshold;
	unsigned long rmse_count;
	void *last_image;
	enum { IGNORE = -2, PREVIEW, GUIDING, INIT, CLEAR_DEC, CLEAR_RA, MOVE_NORTH, MOVE_SOUTH, MOVE_WEST, MOVE_EAST, FAILED, DONE } phase;
	double stack_x[MAX_STACK], stack_y[MAX_STACK];
	int stack_size;
	pthread_mutex_t mutex;
} agent_private_data;

// -------------------------------------------------------------------------------- INDIGO agent common code

static void save_config(indigo_device *device) {
	if (pthread_mutex_trylock(&DEVICE_CONTEXT->config_mutex) == 0) {
		pthread_mutex_unlock(&DEVICE_CONTEXT->config_mutex);
		pthread_mutex_lock(&DEVICE_PRIVATE_DATA->mutex);
		indigo_save_property(device, NULL, AGENT_GUIDER_SETTINGS_PROPERTY);
		indigo_save_property(device, NULL, AGENT_GUIDER_DETECTION_MODE_PROPERTY);
		indigo_save_property(device, NULL, AGENT_GUIDER_DEC_MODE_PROPERTY);
		indigo_save_property(device, NULL, ADDITIONAL_INSTANCES_PROPERTY);
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

static indigo_property_state capture_raw_frame(indigo_device *device) {
	char *ccd_name = FILTER_DEVICE_CONTEXT->device_name[INDIGO_FILTER_CCD_INDEX];
	indigo_property_state state = INDIGO_ALERT_STATE;
	indigo_property *device_exposure_property, *agent_exposure_property, *device_format_property;
	if (!indigo_filter_cached_property(device, INDIGO_FILTER_CCD_INDEX, CCD_EXPOSURE_PROPERTY_NAME, &device_exposure_property, &agent_exposure_property)) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "CCD_EXPOSURE not found");
		return INDIGO_ALERT_STATE;
	}
	if (!indigo_filter_cached_property(device, INDIGO_FILTER_CCD_INDEX, CCD_IMAGE_FORMAT_PROPERTY_NAME, &device_format_property, NULL)) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "CCD_IMAGE_FORMAT not found");
		return INDIGO_ALERT_STATE;
	}
	indigo_change_switch_property_1(FILTER_DEVICE_CONTEXT->client, ccd_name, CCD_IMAGE_FORMAT_PROPERTY_NAME, CCD_IMAGE_FORMAT_RAW_ITEM_NAME, true);
	FILTER_DEVICE_CONTEXT->property_removed = false;
	for (int exposure_attempt = 0; exposure_attempt < 3; exposure_attempt++) {
		if (FILTER_DEVICE_CONTEXT->property_removed)
			return INDIGO_ALERT_STATE;
		if (AGENT_ABORT_PROCESS_PROPERTY->state == INDIGO_BUSY_STATE)
			return INDIGO_ALERT_STATE;
		indigo_change_number_property_1(FILTER_DEVICE_CONTEXT->client, ccd_name, CCD_EXPOSURE_PROPERTY_NAME, CCD_EXPOSURE_ITEM_NAME, AGENT_GUIDER_SETTINGS_EXPOSURE_ITEM->number.value);
		for (int i = 0; i < BUSY_TIMEOUT * 1000 && !FILTER_DEVICE_CONTEXT->property_removed && (state = agent_exposure_property->state) != INDIGO_BUSY_STATE && AGENT_ABORT_PROCESS_PROPERTY->state != INDIGO_BUSY_STATE; i++)
			indigo_usleep(1000);
		if (AGENT_ABORT_PROCESS_PROPERTY->state == INDIGO_BUSY_STATE)
			return INDIGO_ALERT_STATE;
		if (FILTER_DEVICE_CONTEXT->property_removed || state != INDIGO_BUSY_STATE) {
			INDIGO_DRIVER_ERROR(DRIVER_NAME, "CCD_EXPOSURE didn't become busy in %d second(s)", BUSY_TIMEOUT);
			indigo_usleep(ONE_SECOND_DELAY);
			continue;
		}
		double reported_exposure_time = agent_exposure_property->items[0].number.value;
		while (!FILTER_DEVICE_CONTEXT->property_removed && (state = agent_exposure_property->state) == INDIGO_BUSY_STATE) {
			if (AGENT_ABORT_PROCESS_PROPERTY->state == INDIGO_BUSY_STATE)
				return INDIGO_ALERT_STATE;
			if (reported_exposure_time > 1) {
				indigo_usleep(200000);
			} else {
				indigo_usleep(10000);
			}
		}
		if (AGENT_ABORT_PROCESS_PROPERTY->state == INDIGO_BUSY_STATE)
			return INDIGO_ALERT_STATE;
		if (state != INDIGO_OK_STATE) {
			INDIGO_DRIVER_ERROR(DRIVER_NAME, "CCD_EXPOSURE_PROPERTY didn't become OK");
			indigo_usleep(ONE_SECOND_DELAY);
			continue;
		}
		if (FILTER_DEVICE_CONTEXT->property_removed || state != INDIGO_OK_STATE) {
			INDIGO_DRIVER_ERROR(DRIVER_NAME, "Exposure failed");
			return INDIGO_ALERT_STATE;
		}
		if (AGENT_GUIDER_STATS_PHASE_ITEM->number.value == IGNORE)
			return agent_exposure_property->state;
		indigo_raw_header *header = (indigo_raw_header *)(DEVICE_PRIVATE_DATA->last_image);
		if (header == NULL || (header->signature != INDIGO_RAW_MONO8 && header->signature != INDIGO_RAW_MONO16 && header->signature != INDIGO_RAW_RGB24 && header->signature != INDIGO_RAW_RGB48)) {
			indigo_send_message(device, "No RAW image received");
			return INDIGO_ALERT_STATE;
		}
		bool missing_selection = false;
		if (AGENT_GUIDER_DETECTION_SELECTION_ITEM->sw.value) {
			for (int i = 0; i < AGENT_GUIDER_SELECTION_STAR_COUNT_ITEM->number.value; i++) {
				if ((AGENT_GUIDER_SELECTION_X_ITEM + i)->number.value == 0) {
					missing_selection = true;
					break;
				}
			}
		}
		if (AGENT_GUIDER_STARS_PROPERTY->state == INDIGO_BUSY_STATE || missing_selection) {
			int star_count;
			indigo_star_detection stars[MAX_STAR_COUNT];
			indigo_delete_property(device, AGENT_GUIDER_STARS_PROPERTY, NULL);
			indigo_find_stars_precise(
				header->signature,
				(void*)header + sizeof(indigo_raw_header),
				(int)AGENT_GUIDER_SELECTION_RADIUS_ITEM->number.value,
				header->width,
				header->height,
				MAX_STAR_COUNT,
				(indigo_star_detection *)&stars,
				&star_count
			);
			int j = 0;
			for (int i = 0; i < star_count; i++) {
				if (stars[i].oversaturated || stars[i].nc_distance > 0.9)
					continue;
				DEVICE_PRIVATE_DATA->stars[j] = stars[i];
				char name[8];
				char label[32];
				snprintf(name, sizeof(name), "%d", j);
				snprintf(label, sizeof(label), "[%d, %d]", (int)DEVICE_PRIVATE_DATA->stars[j].x, (int)DEVICE_PRIVATE_DATA->stars[j].y);
				indigo_init_switch_item(AGENT_GUIDER_STARS_PROPERTY->items + j++ + 1, name, label, false);
			}
			AGENT_GUIDER_STARS_PROPERTY->count = j + 1;
			AGENT_GUIDER_STARS_PROPERTY->state = INDIGO_OK_STATE;
			indigo_define_property(device, AGENT_GUIDER_STARS_PROPERTY, NULL);
			if (j == 0 && (!AGENT_GUIDER_START_PREVIEW_ITEM->sw.value || AGENT_GUIDER_STATS_FRAME_ITEM->number.value == 0)) {
				indigo_send_message(device, "No stars detected");
				AGENT_GUIDER_STATS_FRAME_ITEM->number.value++;
				indigo_update_property(device, AGENT_GUIDER_STATS_PROPERTY, NULL);
				return AGENT_GUIDER_START_PREVIEW_ITEM->sw.value ? INDIGO_OK_STATE : INDIGO_ALERT_STATE;
			}
		}
		if (missing_selection && AGENT_GUIDER_STATS_FRAME_ITEM->number.value == 0) {
			int star_count = 0;
			int j = 0;
			for (int i = 0; i < AGENT_GUIDER_SELECTION_STAR_COUNT_ITEM->number.value; i++) {
				indigo_item *item_x = AGENT_GUIDER_SELECTION_X_ITEM + 2 * i;
				indigo_item *item_y = AGENT_GUIDER_SELECTION_Y_ITEM + 2 * i;
				if (item_x->number.value == 0 || item_y->number.value == 0) {
					if (j == AGENT_GUIDER_STARS_PROPERTY->count - 1) {
						indigo_send_message(device, "Warning: Only %d suitable stars found (%d requested).", star_count, (int)AGENT_GUIDER_SELECTION_STAR_COUNT_ITEM->number.value);
						break;
					}
					item_x->number.target = item_x->number.value = DEVICE_PRIVATE_DATA->stars[j].x;
					item_y->number.target = item_y->number.value = DEVICE_PRIVATE_DATA->stars[j].y;
					j++;
				}
				star_count++;
			}
			for (int i = star_count; i < AGENT_GUIDER_SELECTION_STAR_COUNT_ITEM->number.value; i++) {
				indigo_item *item_x = AGENT_GUIDER_SELECTION_X_ITEM + 2 * i;
				indigo_item *item_y = AGENT_GUIDER_SELECTION_Y_ITEM + 2 * i;
				item_x->number.target = item_x->number.value = 0;
				item_y->number.target = item_y->number.value = 0;
			}
			indigo_update_property(device, AGENT_GUIDER_SELECTION_PROPERTY, NULL);
		}
		if (AGENT_GUIDER_STATS_FRAME_ITEM->number.value <= 0) {
			indigo_result result;
			AGENT_GUIDER_STATS_SNR_ITEM->number.value = 0;
			AGENT_GUIDER_STATS_REFERENCE_X_ITEM->number.value = 0;
			AGENT_GUIDER_STATS_REFERENCE_Y_ITEM->number.value = 0;
			for (int i = 0; i <= MAX_MULTISTAR_COUNT; i++)
				indigo_delete_frame_digest(DEVICE_PRIVATE_DATA->reference + i);
			DEVICE_PRIVATE_DATA->stack_size = 0;
			DEVICE_PRIVATE_DATA->drift_x = DEVICE_PRIVATE_DATA->drift_y = 0;
			if (AGENT_GUIDER_DETECTION_DONUTS_ITEM->sw.value) {
				result = indigo_donuts_frame_digest(
					header->signature,
					(void*)header + sizeof(indigo_raw_header),
					header->width, header->height,
					(int)AGENT_GUIDER_SELECTION_EDGE_CLIPPING_ITEM->number.value,
					DEVICE_PRIVATE_DATA->reference
				);
				AGENT_GUIDER_STATS_SNR_ITEM->number.value = DEVICE_PRIVATE_DATA->reference->snr;
				if (AGENT_GUIDER_STATS_PHASE_ITEM->number.value >= GUIDING && DEVICE_PRIVATE_DATA->reference->snr < 9) {
					if (exposure_attempt == 2)
						indigo_send_message(device, "Signal to noise ratio is poor, increase exposure time or use different star detection mode");
					state = INDIGO_ALERT_STATE;
					continue;
				}
			} else if (AGENT_GUIDER_DETECTION_CENTROID_ITEM->sw.value) {
				result = indigo_centroid_frame_digest(
					header->signature,
					(void*)header + sizeof(indigo_raw_header),
					header->width,
					header->height,
					DEVICE_PRIVATE_DATA->reference
				);
			} else if (AGENT_GUIDER_DETECTION_SELECTION_ITEM->sw.value) {
				int count = AGENT_GUIDER_SELECTION_STAR_COUNT_ITEM->number.value;
				int used = 0;
				result = INDIGO_OK;
				DEVICE_PRIVATE_DATA->reference->algorithm = centroid;
				DEVICE_PRIVATE_DATA->reference->width = header->width;
				DEVICE_PRIVATE_DATA->reference->height = header->height;
				DEVICE_PRIVATE_DATA->reference->centroid_x = 0;
				DEVICE_PRIVATE_DATA->reference->centroid_y = 0;
				for (int i = 0; i < count && result == INDIGO_OK; i++) {
					indigo_item *item_x = AGENT_GUIDER_SELECTION_X_ITEM + 2 * i;
					indigo_item *item_y = AGENT_GUIDER_SELECTION_Y_ITEM + 2 * i;
					if (item_x->number.value != 0 && item_y->number.value != 0) {
						used++;
						result = indigo_selection_frame_digest_iterative(
							header->signature,
							(void*)header + sizeof(indigo_raw_header),
							&item_x->number.value,
							&item_y->number.value,
							AGENT_GUIDER_SELECTION_RADIUS_ITEM->number.value,
							header->width,
							header->height,
							DEVICE_PRIVATE_DATA->reference + used,
							DIGEST_CONVERGE_ITERATIONS
						);
						DEVICE_PRIVATE_DATA->reference->centroid_x += DEVICE_PRIVATE_DATA->reference[used].centroid_x;
						DEVICE_PRIVATE_DATA->reference->centroid_y += DEVICE_PRIVATE_DATA->reference[used].centroid_y;
					}
				}
				DEVICE_PRIVATE_DATA->reference->centroid_x /= used;
				DEVICE_PRIVATE_DATA->reference->centroid_y /= used;
				if (result == INDIGO_OK) {
					indigo_update_property(device, AGENT_GUIDER_SELECTION_PROPERTY, NULL);
				} else if (AGENT_GUIDER_STATS_PHASE_ITEM->number.value >= GUIDING && result == INDIGO_GUIDE_ERROR) {
					indigo_send_message(device, "Warning: Can not detect star, reseting selection");
					DEVICE_PRIVATE_DATA->reference->centroid_x = 0;
					DEVICE_PRIVATE_DATA->reference->centroid_y = 0;
					for (int i = 0; i < AGENT_GUIDER_SELECTION_STAR_COUNT_ITEM->number.value; i++) {
						indigo_item *item_x = AGENT_GUIDER_SELECTION_X_ITEM + 2 * i;
						indigo_item *item_y = AGENT_GUIDER_SELECTION_Y_ITEM + 2 * i;
						item_x->number.target = item_x->number.value = 0;
						item_y->number.target = item_y->number.value = 0;
					}
					if (exposure_attempt == 2)
						indigo_send_message(device, "Can not detect star in the selection");
					state = INDIGO_ALERT_STATE;
					continue;
				}
			} else {
				result = INDIGO_FAILED;
			}
			if (result == INDIGO_OK) {
				if (DEVICE_PRIVATE_DATA->reference->algorithm == centroid) {
					AGENT_GUIDER_STATS_REFERENCE_X_ITEM->number.value = DEVICE_PRIVATE_DATA->reference->centroid_x + AGENT_GUIDER_SETTINGS_DITH_X_ITEM->number.value;
					AGENT_GUIDER_STATS_REFERENCE_Y_ITEM->number.value = DEVICE_PRIVATE_DATA->reference->centroid_y + AGENT_GUIDER_SETTINGS_DITH_Y_ITEM->number.value;
				}
				AGENT_GUIDER_STATS_FRAME_ITEM->number.value++;
				indigo_update_property(device, AGENT_GUIDER_STATS_PROPERTY, NULL);
			} else if (AGENT_GUIDER_STATS_PHASE_ITEM->number.value == PREVIEW) {
				AGENT_GUIDER_STATS_FRAME_ITEM->number.value++;
				indigo_update_property(device, AGENT_GUIDER_STATS_PROPERTY, NULL);
				return INDIGO_OK_STATE;
			} else {
				return INDIGO_ALERT_STATE;
			}
		} else if (AGENT_GUIDER_STATS_FRAME_ITEM->number.value > 0) {
			indigo_frame_digest digest = { 0 };
			indigo_result result;
			if (AGENT_GUIDER_DETECTION_DONUTS_ITEM->sw.value) {
				result = indigo_donuts_frame_digest(
					header->signature,
					(void*)header + sizeof(indigo_raw_header),
					header->width,
					header->height,
					(int)AGENT_GUIDER_SELECTION_EDGE_CLIPPING_ITEM->number.value,
					&digest
				);
				AGENT_GUIDER_STATS_SNR_ITEM->number.value = digest.snr;
				if (AGENT_GUIDER_STATS_PHASE_ITEM->number.value >= GUIDING && digest.snr < 9) {
					if (exposure_attempt == 2)
						indigo_send_message(device, "Signal to noise ratio is poor, increase exposure time or use different star detection mode");
					state = INDIGO_ALERT_STATE;
					continue;
				}
			} else if (AGENT_GUIDER_DETECTION_CENTROID_ITEM->sw.value) {
				result = indigo_centroid_frame_digest(
					header->signature,
					(void*)header + sizeof(indigo_raw_header),
					header->width,
					header->height,
					&digest
				);
			} else if (AGENT_GUIDER_DETECTION_SELECTION_ITEM->sw.value) {
				int count = AGENT_GUIDER_SELECTION_STAR_COUNT_ITEM->number.value;
				int used = 0;
				indigo_frame_digest digests[MAX_MULTISTAR_COUNT] = { 0 };
				result = INDIGO_OK;
				digest.algorithm = centroid;
				digest.centroid_x = 0;
				digest.centroid_y = 0;
				for (int i = 0; i < count && result == INDIGO_OK; i++) {
					indigo_item *item_x = AGENT_GUIDER_SELECTION_X_ITEM + 2 * i;
					indigo_item *item_y = AGENT_GUIDER_SELECTION_Y_ITEM + 2 * i;
					if (item_x->number.value != 0 && item_y->number.value != 0) {
						digests[used].algorithm = centroid;
						result = indigo_selection_frame_digest_iterative(
							header->signature,
							(void*)header + sizeof(indigo_raw_header),
							&item_x->number.value,
							&item_y->number.value,
							AGENT_GUIDER_SELECTION_RADIUS_ITEM->number.value,
							header->width,
							header->height,
							&digests[used],
							DIGEST_CONVERGE_ITERATIONS
						);
						used++;
					}
				}

				if (result == INDIGO_OK) {
					result = indigo_reduce_multistar_digest(DEVICE_PRIVATE_DATA->reference, DEVICE_PRIVATE_DATA->reference + 1, digests, used, &digest);
				}

				if (result == INDIGO_OK) {
					indigo_update_property(device, AGENT_GUIDER_SELECTION_PROPERTY, NULL);
				} else if (AGENT_GUIDER_STATS_PHASE_ITEM->number.value >= GUIDING && result == INDIGO_GUIDE_ERROR) {
					if (exposure_attempt < 2)
						continue;
					if (DEVICE_PRIVATE_DATA->drift_x || DEVICE_PRIVATE_DATA->drift_y) {
						indigo_send_message(device, "Can not detect star in the selection");
						DEVICE_PRIVATE_DATA->drift_x = DEVICE_PRIVATE_DATA->drift_y = 0;
					}
					AGENT_GUIDER_STATS_FRAME_ITEM->number.value++;
					indigo_update_property(device, AGENT_GUIDER_STATS_PROPERTY, NULL);
					return INDIGO_OK_STATE;
				}
			} else {
				result = INDIGO_FAILED;
			}
			if (result == INDIGO_OK) {
				double drift_x, drift_y;
				result = indigo_calculate_drift(DEVICE_PRIVATE_DATA->reference, &digest, &drift_x, &drift_y);
				DEVICE_PRIVATE_DATA->drift_x = drift_x - AGENT_GUIDER_SETTINGS_DITH_X_ITEM->number.value;
				DEVICE_PRIVATE_DATA->drift_y = drift_y - AGENT_GUIDER_SETTINGS_DITH_Y_ITEM->number.value;

				int count = (int)fmin(AGENT_GUIDER_SETTINGS_STACK_ITEM->number.value, DEVICE_PRIVATE_DATA->stack_size);
				double stddev_x = indigo_stddev(DEVICE_PRIVATE_DATA->stack_x, count);
				double stddev_y = indigo_stddev(DEVICE_PRIVATE_DATA->stack_y, count);

				double tmp[MAX_STACK - 1];

				/* Use Modified PID controller - Large random errors are not used in I, to prevent overshoots */
				if (
					fabs(DEVICE_PRIVATE_DATA->drift_x) < 5 * stddev_x ||
					AGENT_GUIDER_SETTINGS_STACK_ITEM->number.value > DEVICE_PRIVATE_DATA->stack_size ||
					AGENT_GUIDER_SETTINGS_STACK_ITEM->number.value == 1
				) {
					memcpy(tmp, DEVICE_PRIVATE_DATA->stack_x, sizeof(double) * (MAX_STACK - 1));
					memcpy(DEVICE_PRIVATE_DATA->stack_x + 1, tmp, sizeof(double) * (MAX_STACK - 1));
					DEVICE_PRIVATE_DATA->stack_x[0] = DEVICE_PRIVATE_DATA->drift_x;
				} else {
					indigo_debug(
						"Drift X = %.3f (avg = %.3f, stddev = %.3f) jump detected - not used in the I-term",
						DEVICE_PRIVATE_DATA->drift_x,
						DEVICE_PRIVATE_DATA->avg_drift_x,
						stddev_x
					);
				}

				if (
					fabs(DEVICE_PRIVATE_DATA->drift_y) < 5 * stddev_y ||
					AGENT_GUIDER_SETTINGS_STACK_ITEM->number.value > DEVICE_PRIVATE_DATA->stack_size ||
					AGENT_GUIDER_SETTINGS_STACK_ITEM->number.value == 1
				) {
					memcpy(tmp, DEVICE_PRIVATE_DATA->stack_y, sizeof(double) * (MAX_STACK - 1));
					memcpy(DEVICE_PRIVATE_DATA->stack_y + 1, tmp, sizeof(double) * (MAX_STACK - 1));
					DEVICE_PRIVATE_DATA->stack_y[0] = DEVICE_PRIVATE_DATA->drift_y;
				} else {
					indigo_debug(
						"Drift Y = %.3f (avg = %.3f, stddev = %.3f) jump detected - not used in the I-term",
						DEVICE_PRIVATE_DATA->drift_y,
						DEVICE_PRIVATE_DATA->avg_drift_y,
						stddev_y
					);
				}

				if (DEVICE_PRIVATE_DATA->stack_size < MAX_STACK) {
					DEVICE_PRIVATE_DATA->stack_size++;
				}

				if (
					AGENT_GUIDER_SETTINGS_STACK_ITEM->number.value == 1 ||
					AGENT_GUIDER_STATS_PHASE_ITEM->number.value != GUIDING
				) {
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
				if (result == INDIGO_OK) {
					AGENT_GUIDER_STATS_FRAME_ITEM->number.value++;
					AGENT_GUIDER_STATS_DRIFT_X_ITEM->number.value = round(1000 * DEVICE_PRIVATE_DATA->drift_x) / 1000;
					AGENT_GUIDER_STATS_DRIFT_Y_ITEM->number.value = round(1000 * DEVICE_PRIVATE_DATA->drift_y) / 1000;
					DEVICE_PRIVATE_DATA->drift = sqrt(DEVICE_PRIVATE_DATA->drift_x * DEVICE_PRIVATE_DATA->drift_x + DEVICE_PRIVATE_DATA->drift_y * DEVICE_PRIVATE_DATA->drift_y);
					INDIGO_DRIVER_DEBUG(DRIVER_NAME, "Drift %.4gpx (%.4g, %.4g)", DEVICE_PRIVATE_DATA->drift, DEVICE_PRIVATE_DATA->drift_x, DEVICE_PRIVATE_DATA->drift_y);
				}
			} else if (AGENT_GUIDER_STATS_PHASE_ITEM->number.value == PREVIEW) {
				AGENT_GUIDER_STATS_FRAME_ITEM->number.value++;
				indigo_update_property(device, AGENT_GUIDER_STATS_PROPERTY, NULL);
				return INDIGO_OK_STATE;
			} else {
				return INDIGO_ALERT_STATE;
			}
			indigo_delete_frame_digest(&digest);
		}
		break;
	}
	return state;
}

#define GRID	32

static void select_subframe(indigo_device *device) {
	int bin_x = 1;
	int bin_y = 1;
	if (AGENT_GUIDER_SELECTION_SUBFRAME_ITEM->number.value && DEVICE_PRIVATE_DATA->saved_frame == NULL) {
		indigo_property *device_ccd_frame_property, *agent_ccd_frame_property, *agent_ccd_bin_property;
		if (indigo_filter_cached_property(device, INDIGO_FILTER_CCD_INDEX, CCD_FRAME_PROPERTY_NAME, &device_ccd_frame_property, &agent_ccd_frame_property) && agent_ccd_frame_property->perm == INDIGO_RW_PERM) {
			if (capture_raw_frame(device) != INDIGO_OK_STATE) {
				AGENT_START_PROCESS_PROPERTY->state = AGENT_START_PROCESS_PROPERTY->state == INDIGO_OK_STATE ? INDIGO_OK_STATE : INDIGO_ALERT_STATE;
				return;
			}
			int selection_x = AGENT_GUIDER_SELECTION_X_ITEM->number.value;
			int selection_y = AGENT_GUIDER_SELECTION_Y_ITEM->number.value;
			if (selection_x == 0 || selection_y == 0) {
				AGENT_START_PROCESS_PROPERTY->state = AGENT_START_PROCESS_PROPERTY->state == INDIGO_OK_STATE ? INDIGO_OK_STATE : INDIGO_ALERT_STATE;
				return;
			}
			if (indigo_filter_cached_property(device, INDIGO_FILTER_CCD_INDEX, CCD_BIN_PROPERTY_NAME, NULL, &agent_ccd_bin_property)) {
				for (int i = 0; i < agent_ccd_bin_property->count; i++) {
					indigo_item *item = agent_ccd_bin_property->items + i;
					if (!strcmp(item->name, CCD_BIN_HORIZONTAL_ITEM_NAME))
						bin_x = item->number.value;
					else if (!strcmp(item->name, CCD_BIN_VERTICAL_ITEM_NAME))
						bin_y = item->number.value;
				}
			}
			for (int i = 0; i < agent_ccd_frame_property->count; i++) {
				indigo_item *item = agent_ccd_frame_property->items + i;
				if (!strcmp(item->name, CCD_FRAME_LEFT_ITEM_NAME))
					selection_x += item->number.value / bin_x;
				else if (!strcmp(item->name, CCD_FRAME_TOP_ITEM_NAME))
					selection_y += item->number.value / bin_y;
			}
			int window_size = AGENT_GUIDER_SELECTION_SUBFRAME_ITEM->number.value * AGENT_GUIDER_SELECTION_RADIUS_ITEM->number.value;
			int frame_left = rint((selection_x - window_size) / (double)GRID) * GRID;
			int frame_top = rint((selection_y - window_size) / (double)GRID) * GRID;
			if (selection_x - frame_left < AGENT_GUIDER_SELECTION_RADIUS_ITEM->number.value)
				frame_left -= GRID;
			if (selection_y - frame_top < AGENT_GUIDER_SELECTION_RADIUS_ITEM->number.value)
				frame_top -= GRID;
			int frame_width = (2 * window_size / GRID + 1) * GRID;
			int frame_height = (2 * window_size / GRID + 1) * GRID;
			DEVICE_PRIVATE_DATA->saved_frame_left = frame_left;
			DEVICE_PRIVATE_DATA->saved_frame_top = frame_top;
			AGENT_GUIDER_SELECTION_X_ITEM->number.value = selection_x -= frame_left;
			AGENT_GUIDER_SELECTION_Y_ITEM->number.value = selection_y -= frame_top;
			indigo_update_property(device, AGENT_GUIDER_SELECTION_PROPERTY, NULL);
			if (frame_width - selection_x < AGENT_GUIDER_SELECTION_RADIUS_ITEM->number.value)
				frame_width += GRID;
			if (frame_height - selection_y < AGENT_GUIDER_SELECTION_RADIUS_ITEM->number.value)
				frame_height += GRID;
			int size = sizeof(indigo_property) + device_ccd_frame_property->count * sizeof(indigo_item);
			DEVICE_PRIVATE_DATA->saved_frame = indigo_safe_malloc_copy(size, agent_ccd_frame_property);
			strcpy(DEVICE_PRIVATE_DATA->saved_frame->device, device_ccd_frame_property->device);
			char *names[] = { CCD_FRAME_LEFT_ITEM_NAME, CCD_FRAME_TOP_ITEM_NAME, CCD_FRAME_WIDTH_ITEM_NAME, CCD_FRAME_HEIGHT_ITEM_NAME };
			double values[] = { frame_left * bin_x, frame_top * bin_y,  frame_width * bin_x, frame_height * bin_y };
			indigo_change_number_property(FILTER_DEVICE_CONTEXT->client, device_ccd_frame_property->device, CCD_FRAME_PROPERTY_NAME, 4, (const char **)names, values);
		}
	}
}

static void restore_subframe(indigo_device *device) {
	if (DEVICE_PRIVATE_DATA->saved_frame) {
		indigo_change_property(FILTER_DEVICE_CONTEXT->client, DEVICE_PRIVATE_DATA->saved_frame);
		indigo_release_property(DEVICE_PRIVATE_DATA->saved_frame);
		DEVICE_PRIVATE_DATA->saved_frame = NULL;
		AGENT_GUIDER_SELECTION_X_ITEM->number.value += DEVICE_PRIVATE_DATA->saved_frame_left;
		AGENT_GUIDER_SELECTION_X_ITEM->number.target = AGENT_GUIDER_SELECTION_X_ITEM->number.value;
		AGENT_GUIDER_SELECTION_Y_ITEM->number.value += DEVICE_PRIVATE_DATA->saved_frame_top;
		AGENT_GUIDER_SELECTION_Y_ITEM->number.target = AGENT_GUIDER_SELECTION_Y_ITEM->number.value;
		/* TRICKY: No idea why but this prevents ensures frame to be restored correctly */
		indigo_usleep(0.5 * ONE_SECOND_DELAY);
		/* TRICKY: capture_raw_frame() should be here in order to have the correct frame and correct selection
			 so that HFD, FWHM etc are evaluated correctly, but selection property should not be updated. */
		capture_raw_frame(device);
		indigo_update_property(device, AGENT_GUIDER_SELECTION_PROPERTY, NULL);
		DEVICE_PRIVATE_DATA->saved_frame_left = 0;
		DEVICE_PRIVATE_DATA->saved_frame_top = 0;
	}
}

static indigo_property_state pulse_guide(indigo_device *device, double ra, double dec) {
	char *guider_name = FILTER_DEVICE_CONTEXT->device_name[INDIGO_FILTER_GUIDER_INDEX];
	indigo_property *agent_guide_property;
	if (ra) {
		static const char *names[] = { GUIDER_GUIDE_WEST_ITEM_NAME, GUIDER_GUIDE_EAST_ITEM_NAME };
		double values[] = { ra > 0 ? ra * 1000 : 0, ra < 0 ? -ra * 1000 : 0 };
		indigo_change_number_property(FILTER_DEVICE_CONTEXT->client, guider_name, GUIDER_GUIDE_RA_PROPERTY_NAME, 2, names, values);
		indigo_usleep(fabs(ra) * 1000000);
		if (!indigo_filter_cached_property(device, INDIGO_FILTER_GUIDER_INDEX, GUIDER_GUIDE_RA_PROPERTY_NAME, NULL, &agent_guide_property)) {
			INDIGO_DRIVER_ERROR(DRIVER_NAME, "GUIDER_GUIDE_RA_PROPERTY not found");
			return INDIGO_ALERT_STATE;
		}
		FILTER_DEVICE_CONTEXT->property_removed = false;
		for (int i = 0; i < 200 && !FILTER_DEVICE_CONTEXT->property_removed && agent_guide_property->state == INDIGO_BUSY_STATE; i++) {
			indigo_usleep(50000);
		}
	}
	if (dec) {
		static const char *names[] = { GUIDER_GUIDE_NORTH_ITEM_NAME, GUIDER_GUIDE_SOUTH_ITEM_NAME };
		double values[] = { dec > 0 ? dec * 1000 : 0, dec < 0 ? -dec * 1000 : 0 };
		indigo_change_number_property(FILTER_DEVICE_CONTEXT->client, guider_name, GUIDER_GUIDE_DEC_PROPERTY_NAME, 2, names, values);
		indigo_usleep(fabs(dec) * 1000000);
		if (!indigo_filter_cached_property(device, INDIGO_FILTER_GUIDER_INDEX, GUIDER_GUIDE_DEC_PROPERTY_NAME, NULL, &agent_guide_property)) {
			INDIGO_DRIVER_ERROR(DRIVER_NAME, "GUIDER_GUIDE_DEC_PROPERTY not found");
			return INDIGO_ALERT_STATE;
		}
		FILTER_DEVICE_CONTEXT->property_removed = false;
		for (int i = 0; i < 200 && !FILTER_DEVICE_CONTEXT->property_removed && agent_guide_property->state == INDIGO_BUSY_STATE; i++) {
			indigo_usleep(50000);
		}
	}
	return INDIGO_OK_STATE;
}

static void preview_process(indigo_device *device) {
	FILTER_DEVICE_CONTEXT->running_process = true;
	AGENT_GUIDER_STATS_PHASE_ITEM->number.value = PREVIEW;
	AGENT_GUIDER_STATS_FRAME_ITEM->number.value =
	AGENT_GUIDER_STATS_FRAME_ITEM->number.value =
	AGENT_GUIDER_STATS_REFERENCE_X_ITEM->number.value =
	AGENT_GUIDER_STATS_REFERENCE_Y_ITEM->number.value =
	AGENT_GUIDER_STATS_DRIFT_X_ITEM->number.value =
	AGENT_GUIDER_STATS_DRIFT_Y_ITEM->number.value =
	AGENT_GUIDER_STATS_DRIFT_RA_ITEM->number.value =
	AGENT_GUIDER_STATS_DRIFT_DEC_ITEM->number.value =
	AGENT_GUIDER_STATS_CORR_RA_ITEM->number.value =
	AGENT_GUIDER_STATS_CORR_DEC_ITEM->number.value =
	AGENT_GUIDER_STATS_RMSE_RA_ITEM->number.value =
	AGENT_GUIDER_STATS_RMSE_DEC_ITEM->number.value =
	AGENT_GUIDER_STATS_SNR_ITEM->number.value =
	AGENT_GUIDER_STATS_DITHERING_ITEM->number.value = 0;

	AGENT_GUIDER_SETTINGS_DITH_X_ITEM->number.value =
	AGENT_GUIDER_SETTINGS_DITH_X_ITEM->number.target =
	AGENT_GUIDER_SETTINGS_DITH_Y_ITEM->number.value =
	AGENT_GUIDER_SETTINGS_DITH_Y_ITEM->number.target = 0;

	indigo_update_property(device, AGENT_GUIDER_STATS_PROPERTY, NULL);
	indigo_update_property(device, AGENT_GUIDER_SETTINGS_PROPERTY, NULL);
	while (capture_raw_frame(device) == INDIGO_OK_STATE)
		indigo_update_property(device, AGENT_GUIDER_STATS_PROPERTY, NULL);
	AGENT_GUIDER_STATS_PHASE_ITEM->number.value = AGENT_ABORT_PROCESS_PROPERTY->state == INDIGO_BUSY_STATE ? DONE : FAILED;
	AGENT_GUIDER_STATS_REFERENCE_X_ITEM->number.value =
	AGENT_GUIDER_STATS_REFERENCE_Y_ITEM->number.value =
	AGENT_GUIDER_STATS_DITHERING_ITEM->number.value = 0;
	indigo_update_property(device, AGENT_GUIDER_STATS_PROPERTY, NULL);
	AGENT_GUIDER_START_PREVIEW_ITEM->sw.value =
	AGENT_GUIDER_START_CALIBRATION_ITEM->sw.value =
	AGENT_GUIDER_START_CALIBRATION_AND_GUIDING_ITEM->sw.value =
	AGENT_GUIDER_START_GUIDING_ITEM->sw.value = false;
	AGENT_START_PROCESS_PROPERTY->state = AGENT_ABORT_PROCESS_PROPERTY->state == INDIGO_BUSY_STATE ? INDIGO_OK_STATE : INDIGO_ALERT_STATE;
	indigo_update_property(device, AGENT_START_PROCESS_PROPERTY, NULL);
	if (AGENT_ABORT_PROCESS_PROPERTY->state == INDIGO_BUSY_STATE) {
		AGENT_ABORT_PROCESS_PROPERTY->state = INDIGO_OK_STATE;
		indigo_update_property(device, AGENT_ABORT_PROCESS_PROPERTY, NULL);
	}
	FILTER_DEVICE_CONTEXT->running_process = false;
}

static void change_step(indigo_device *device, double q) {
	if (q > 1) {
		indigo_send_message(device, "Drift is too slow");
		if (AGENT_GUIDER_SETTINGS_STEP_ITEM->number.value < AGENT_GUIDER_SETTINGS_STEP_ITEM->number.max) {
			AGENT_GUIDER_SETTINGS_STEP_ITEM->number.target = (AGENT_GUIDER_SETTINGS_STEP_ITEM->number.value *= q);
			indigo_update_property(device, AGENT_GUIDER_SETTINGS_PROPERTY, "Increasing calibration step to %.3g", AGENT_GUIDER_SETTINGS_STEP_ITEM->number.target);
			DEVICE_PRIVATE_DATA->phase = INIT;
		} else {
			DEVICE_PRIVATE_DATA->phase = FAILED;
		}
	} else {
		indigo_send_message(device, "Drift is too fast");
		if (AGENT_GUIDER_SETTINGS_STEP_ITEM->number.value < AGENT_GUIDER_SETTINGS_STEP_ITEM->number.max) {
			AGENT_GUIDER_SETTINGS_STEP_ITEM->number.target = (AGENT_GUIDER_SETTINGS_STEP_ITEM->number.value *= q);
			indigo_update_property(device, AGENT_GUIDER_SETTINGS_PROPERTY, "Decreasing calibration step to %.3g", AGENT_GUIDER_SETTINGS_STEP_ITEM->number.target);
			DEVICE_PRIVATE_DATA->phase = INIT;
		} else {
			DEVICE_PRIVATE_DATA->phase = FAILED;
		}
	}
	indigo_update_property(device, AGENT_GUIDER_SETTINGS_PROPERTY, NULL);
}

static bool guide_and_capture_frame(indigo_device *device, double ra, double dec) {
	if (pulse_guide(device, ra, dec) != INDIGO_OK_STATE) {
		return false;
	}
	if (capture_raw_frame(device) != INDIGO_OK_STATE) {
		return false;
	}
	return true;
}

static void guide_process(indigo_device *device);

static void _calibrate_process(indigo_device *device, bool will_guide) {
	double last_drift = 0, dec_angle = 0;
	int last_count = 0;
	AGENT_GUIDER_STATS_PHASE_ITEM->number.value = DEVICE_PRIVATE_DATA->phase = INIT;
	AGENT_GUIDER_STATS_FRAME_ITEM->number.value =
	AGENT_GUIDER_STATS_FRAME_ITEM->number.value =
	AGENT_GUIDER_STATS_REFERENCE_X_ITEM->number.value =
	AGENT_GUIDER_STATS_REFERENCE_Y_ITEM->number.value =
	AGENT_GUIDER_STATS_DRIFT_X_ITEM->number.value =
	AGENT_GUIDER_STATS_DRIFT_Y_ITEM->number.value =
	AGENT_GUIDER_STATS_DRIFT_RA_ITEM->number.value =
	AGENT_GUIDER_STATS_DRIFT_DEC_ITEM->number.value =
	AGENT_GUIDER_STATS_CORR_RA_ITEM->number.value =
	AGENT_GUIDER_STATS_CORR_DEC_ITEM->number.value =
	AGENT_GUIDER_STATS_RMSE_RA_ITEM->number.value =
	AGENT_GUIDER_STATS_RMSE_DEC_ITEM->number.value =
	AGENT_GUIDER_STATS_SNR_ITEM->number.value =
	AGENT_GUIDER_STATS_DITHERING_ITEM->number.value = 0;

	AGENT_GUIDER_SETTINGS_DITH_X_ITEM->number.value =
	AGENT_GUIDER_SETTINGS_DITH_X_ITEM->number.target =
	AGENT_GUIDER_SETTINGS_DITH_Y_ITEM->number.value =
	AGENT_GUIDER_SETTINGS_DITH_Y_ITEM->number.target = 0;

	indigo_update_property(device, AGENT_GUIDER_STATS_PROPERTY, NULL);
	indigo_update_property(device, AGENT_GUIDER_SETTINGS_PROPERTY, NULL);
	while (AGENT_START_PROCESS_PROPERTY->state == INDIGO_BUSY_STATE) {
		AGENT_GUIDER_STATS_PHASE_ITEM->number.value = DEVICE_PRIVATE_DATA->phase;
		switch (DEVICE_PRIVATE_DATA->phase) {
			case INIT: {
				AGENT_GUIDER_SETTINGS_ANGLE_ITEM->number.value = AGENT_GUIDER_SETTINGS_ANGLE_ITEM->number.target = 0;
				AGENT_GUIDER_SETTINGS_BACKLASH_ITEM->number.value = AGENT_GUIDER_SETTINGS_BACKLASH_ITEM->number.target = 0;
				AGENT_GUIDER_SETTINGS_SPEED_RA_ITEM->number.value = AGENT_GUIDER_SETTINGS_SPEED_RA_ITEM->number.target = 0;
				AGENT_GUIDER_SETTINGS_SPEED_DEC_ITEM->number.value = AGENT_GUIDER_SETTINGS_SPEED_DEC_ITEM->number.target = 0;
				AGENT_GUIDER_SETTINGS_DITH_X_ITEM->number.value = AGENT_GUIDER_SETTINGS_DITH_X_ITEM->number.target = 0;
				AGENT_GUIDER_SETTINGS_DITH_Y_ITEM->number.value = AGENT_GUIDER_SETTINGS_DITH_Y_ITEM->number.target = 0;
				DEVICE_PRIVATE_DATA->phase = CLEAR_DEC;
				indigo_send_message(device, "Calibration started");
				indigo_update_property(device, AGENT_GUIDER_SETTINGS_PROPERTY, NULL);
				break;
			}
			case CLEAR_DEC: {
				if (AGENT_GUIDER_DEC_MODE_NONE_ITEM->sw.value) {
					DEVICE_PRIVATE_DATA->phase = CLEAR_RA;
					break;
				}
				if (AGENT_GUIDER_SETTINGS_CAL_STEPS_ITEM->number.value == 0) {
					DEVICE_PRIVATE_DATA->phase = MOVE_NORTH;
					break;
				}
				AGENT_GUIDER_STATS_FRAME_ITEM->number.value =
				AGENT_GUIDER_STATS_DRIFT_X_ITEM->number.value =
				AGENT_GUIDER_STATS_DRIFT_Y_ITEM->number.value = 0;
				if (capture_raw_frame(device) != INDIGO_OK_STATE) {
					DEVICE_PRIVATE_DATA->phase = FAILED;
					break;
				}
				indigo_update_property(device, AGENT_GUIDER_STATS_PROPERTY, "Clearing DEC backlash");
				for (int i = 0; i < AGENT_GUIDER_SETTINGS_BL_STEPS_ITEM->number.value; i++) {
					if (AGENT_START_PROCESS_PROPERTY->state != INDIGO_BUSY_STATE) {
						DEVICE_PRIVATE_DATA->phase = FAILED;
						break;
					}
					if (!guide_and_capture_frame(device, 0, AGENT_GUIDER_SETTINGS_STEP_ITEM->number.value)) {
						DEVICE_PRIVATE_DATA->phase = FAILED;
						break;
					}
					indigo_update_property(device, AGENT_GUIDER_STATS_PROPERTY, NULL);
					if (DEVICE_PRIVATE_DATA->drift > AGENT_GUIDER_SETTINGS_BL_DRIFT_ITEM->number.value) {
						DEVICE_PRIVATE_DATA->phase = CLEAR_RA;
						break;
					}
				}
				if (DEVICE_PRIVATE_DATA->phase == CLEAR_DEC) {
					change_step(device, 2);
				}
				break;
			}
			case CLEAR_RA: {
				if (AGENT_GUIDER_SETTINGS_CAL_STEPS_ITEM->number.value == 0) {
					DEVICE_PRIVATE_DATA->phase = MOVE_NORTH;
					break;
				}
				AGENT_GUIDER_STATS_FRAME_ITEM->number.value =
				AGENT_GUIDER_STATS_DRIFT_X_ITEM->number.value =
				AGENT_GUIDER_STATS_DRIFT_Y_ITEM->number.value = 0;
				if (capture_raw_frame(device) != INDIGO_OK_STATE) {
					DEVICE_PRIVATE_DATA->phase = FAILED;
					break;
				}
				indigo_update_property(device, AGENT_GUIDER_STATS_PROPERTY, "Clearing RA backlash");
				/* cos(87deg) = 0.05 => so 20 is ok for 87 declination */
				for (int i = 0; i < AGENT_GUIDER_SETTINGS_BL_STEPS_ITEM->number.value * 20; i++) {
					if (!guide_and_capture_frame(device, AGENT_GUIDER_SETTINGS_STEP_ITEM->number.value, 0)) {
						DEVICE_PRIVATE_DATA->phase = FAILED;
						break;
					}
					indigo_update_property(device, AGENT_GUIDER_STATS_PROPERTY, NULL);
					if (DEVICE_PRIVATE_DATA->drift > AGENT_GUIDER_SETTINGS_BL_DRIFT_ITEM->number.value) {
						indigo_send_message(device, "Backlash cleared");
						DEVICE_PRIVATE_DATA->phase = MOVE_NORTH;
						break;
					}
				}
				if (DEVICE_PRIVATE_DATA->phase == CLEAR_RA) {
					change_step(device, 2);
				}
				break;
			}
			case MOVE_NORTH: {
				if (AGENT_GUIDER_DEC_MODE_NONE_ITEM->sw.value) {
					DEVICE_PRIVATE_DATA->phase = MOVE_WEST;
					break;
				}
				AGENT_GUIDER_STATS_FRAME_ITEM->number.value = AGENT_GUIDER_STATS_DRIFT_X_ITEM->number.value = AGENT_GUIDER_STATS_DRIFT_Y_ITEM->number.value = 0;
				if (capture_raw_frame(device) != INDIGO_OK_STATE) {
					DEVICE_PRIVATE_DATA->phase = FAILED;
					break;
				}
				indigo_update_property(device, AGENT_GUIDER_STATS_PROPERTY, "Moving north");
				for (int i = 0; i < AGENT_GUIDER_SETTINGS_CAL_STEPS_ITEM->number.value; i++) {
					if (!guide_and_capture_frame(device, 0, AGENT_GUIDER_SETTINGS_STEP_ITEM->number.value)) {
						DEVICE_PRIVATE_DATA->phase = FAILED;
						break;
					}
					indigo_update_property(device, AGENT_GUIDER_STATS_PROPERTY, NULL);
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
								DEVICE_PRIVATE_DATA->phase = FAILED;
							} else {
								DEVICE_PRIVATE_DATA->phase = MOVE_SOUTH;
							}
							break;
						}
					}
				}
				if (DEVICE_PRIVATE_DATA->phase == MOVE_NORTH) {
					change_step(device, 2);
				}
				break;
			}
			case MOVE_SOUTH: {
				AGENT_GUIDER_STATS_FRAME_ITEM->number.value = AGENT_GUIDER_STATS_DRIFT_X_ITEM->number.value = AGENT_GUIDER_STATS_DRIFT_Y_ITEM->number.value = 0;
				if (capture_raw_frame(device) != INDIGO_OK_STATE) {
					DEVICE_PRIVATE_DATA->phase = FAILED;
					break;
				}
				indigo_update_property(device, AGENT_GUIDER_STATS_PROPERTY, "Moving south");
				for (int i = 0; i < last_count; i++) {
					if (!guide_and_capture_frame(device, 0, -AGENT_GUIDER_SETTINGS_STEP_ITEM->number.value)) {
						DEVICE_PRIVATE_DATA->phase = FAILED;
						break;
					}
					indigo_update_property(device, AGENT_GUIDER_STATS_PROPERTY, NULL);
				}
				if (DEVICE_PRIVATE_DATA->phase == MOVE_SOUTH) {
					if (DEVICE_PRIVATE_DATA->drift < last_drift) {
						AGENT_GUIDER_SETTINGS_BACKLASH_ITEM->number.value = AGENT_GUIDER_SETTINGS_BACKLASH_ITEM->number.target = round(1000 * (last_drift - DEVICE_PRIVATE_DATA->drift)) / 1000;
					} else {
						AGENT_GUIDER_SETTINGS_BACKLASH_ITEM->number.value = AGENT_GUIDER_SETTINGS_BACKLASH_ITEM->number.target = 0;
						indigo_send_message(device, "Warning: Inconsitent backlash");
					}
					indigo_update_property(device, AGENT_GUIDER_SETTINGS_PROPERTY, NULL);
					DEVICE_PRIVATE_DATA->phase = MOVE_WEST;
				}
				break;
			}
			case MOVE_WEST: {
				AGENT_GUIDER_STATS_FRAME_ITEM->number.value = AGENT_GUIDER_STATS_DRIFT_X_ITEM->number.value = AGENT_GUIDER_STATS_DRIFT_Y_ITEM->number.value = 0;
				if (capture_raw_frame(device) != INDIGO_OK_STATE) {
					DEVICE_PRIVATE_DATA->phase = FAILED;
					break;
				}
				indigo_update_property(device, AGENT_GUIDER_STATS_PROPERTY, "Moving west");
				/* we need more iterations closer to the pole */
				for (int i = 0; i < AGENT_GUIDER_SETTINGS_CAL_STEPS_ITEM->number.value * 5; i++) {
					if (!guide_and_capture_frame(device, AGENT_GUIDER_SETTINGS_STEP_ITEM->number.value, 0)) {
						DEVICE_PRIVATE_DATA->phase = FAILED;
						break;
					}
					indigo_update_property(device, AGENT_GUIDER_STATS_PROPERTY, NULL);
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
							DEVICE_PRIVATE_DATA->phase = FAILED;
						} else {
							DEVICE_PRIVATE_DATA->phase = MOVE_EAST;
						}
						break;
					}
				}
				break;
			}
			case MOVE_EAST: {
				AGENT_GUIDER_STATS_FRAME_ITEM->number.value = AGENT_GUIDER_STATS_DRIFT_X_ITEM->number.value = AGENT_GUIDER_STATS_DRIFT_Y_ITEM->number.value = 0;
				if (capture_raw_frame(device) != INDIGO_OK_STATE) {
					DEVICE_PRIVATE_DATA->phase = FAILED;
					break;
				}
				indigo_update_property(device, AGENT_GUIDER_STATS_PROPERTY, "Moving east");
				for (int i = 0; i < last_count; i++) {
					if (!guide_and_capture_frame(device, -AGENT_GUIDER_SETTINGS_STEP_ITEM->number.value, 0)) {
						DEVICE_PRIVATE_DATA->phase = FAILED;
						break;
					}
					indigo_update_property(device, AGENT_GUIDER_STATS_PROPERTY, NULL);
				}
				if (DEVICE_PRIVATE_DATA->phase == MOVE_EAST) {
					DEVICE_PRIVATE_DATA->phase = DONE;
				}
				break;
			}
			case FAILED: {
				indigo_send_message(device, "Calibration failed");
				AGENT_START_PROCESS_PROPERTY->state = INDIGO_ALERT_STATE;
				break;
			}
			case DONE: {
				double acos_dec = fabs(AGENT_GUIDER_SETTINGS_SPEED_RA_ITEM->number.value / AGENT_GUIDER_SETTINGS_SPEED_DEC_ITEM->number.value);
				if (acos_dec > 1) acos_dec = 1;
				double pole_distnce = 90 - 180 / PI * acos(acos_dec);
				/* Declination is > 85deg or < -85deg */
				if (pole_distnce < 5) {
					indigo_send_message(device, "Calculated pole distance %.1f°. RA calibration may be off", pole_distnce);
				} else {
					indigo_send_message(device, "Calculated pole distance %.1f°", pole_distnce);
				}
				indigo_send_message(device, "Calibration complete");
				save_config(device);
				AGENT_START_PROCESS_PROPERTY->state = INDIGO_OK_STATE;
				break;
			}
			default:
				AGENT_START_PROCESS_PROPERTY->state = INDIGO_ALERT_STATE;
				break;
		}
	}
	indigo_update_property(device, AGENT_GUIDER_STATS_PROPERTY, NULL);
	AGENT_GUIDER_START_PREVIEW_ITEM->sw.value =
	AGENT_GUIDER_START_CALIBRATION_ITEM->sw.value =
	AGENT_GUIDER_START_CALIBRATION_AND_GUIDING_ITEM->sw.value =
	AGENT_GUIDER_START_GUIDING_ITEM->sw.value = false;
	indigo_update_property(device, AGENT_START_PROCESS_PROPERTY, NULL);
	if (AGENT_ABORT_PROCESS_PROPERTY->state == INDIGO_BUSY_STATE) {
		AGENT_ABORT_PROCESS_PROPERTY->state = INDIGO_OK_STATE;
		indigo_update_property(device, AGENT_ABORT_PROCESS_PROPERTY, NULL);
	} else if (AGENT_START_PROCESS_PROPERTY->state == INDIGO_OK_STATE && will_guide) {
		AGENT_START_PROCESS_PROPERTY->state = INDIGO_BUSY_STATE;
		AGENT_GUIDER_START_GUIDING_ITEM->sw.value = true;
		indigo_update_property(device, AGENT_START_PROCESS_PROPERTY, NULL);
		indigo_set_timer(device, 0, guide_process, NULL);
	}
}

static void calibrate_process(indigo_device *device) {
	FILTER_DEVICE_CONTEXT->running_process = true;
	_calibrate_process(device, false);
	FILTER_DEVICE_CONTEXT->running_process = false;
}

static void calibrate_and_guide_process(indigo_device *device) {
	FILTER_DEVICE_CONTEXT->running_process = true;
	_calibrate_process(device, true);
	FILTER_DEVICE_CONTEXT->running_process = false;
}

static void guide_process(indigo_device *device) {
	if (AGENT_GUIDER_SETTINGS_SPEED_RA_ITEM->number.value == 0) {
		AGENT_START_PROCESS_PROPERTY->state = INDIGO_ALERT_STATE;
		indigo_update_property(device, AGENT_START_PROCESS_PROPERTY, NULL);
		indigo_send_message(device, "Guiding failed (not calibrated)");
		FILTER_DEVICE_CONTEXT->running_process = false;
		return;
	}
	FILTER_DEVICE_CONTEXT->running_process = true;
	AGENT_GUIDER_STATS_PHASE_ITEM->number.value = IGNORE;
	AGENT_GUIDER_STATS_FRAME_ITEM->number.value =
	AGENT_GUIDER_STATS_REFERENCE_X_ITEM->number.value =
	AGENT_GUIDER_STATS_REFERENCE_Y_ITEM->number.value =
	AGENT_GUIDER_STATS_DRIFT_X_ITEM->number.value =
	AGENT_GUIDER_STATS_DRIFT_Y_ITEM->number.value =
	AGENT_GUIDER_STATS_DRIFT_RA_ITEM->number.value =
	AGENT_GUIDER_STATS_DRIFT_DEC_ITEM->number.value =
	AGENT_GUIDER_STATS_CORR_RA_ITEM->number.value =
	AGENT_GUIDER_STATS_CORR_DEC_ITEM->number.value =
	AGENT_GUIDER_STATS_RMSE_RA_ITEM->number.value =
	AGENT_GUIDER_STATS_RMSE_DEC_ITEM->number.value =
	AGENT_GUIDER_STATS_SNR_ITEM->number.value =
	AGENT_GUIDER_STATS_DITHERING_ITEM->number.value = 0;
	DEVICE_PRIVATE_DATA->rmse_ra_threshold =
	DEVICE_PRIVATE_DATA->rmse_dec_threshold = 0;
	indigo_send_message(device, "Guiding started");
	double saved_exposure_time = AGENT_GUIDER_SETTINGS_EXPOSURE_ITEM->number.value;
	AGENT_GUIDER_SETTINGS_EXPOSURE_ITEM->number.value = 0;
	if (capture_raw_frame(device) != INDIGO_OK_STATE) {
		AGENT_START_PROCESS_PROPERTY->state = AGENT_START_PROCESS_PROPERTY->state == INDIGO_OK_STATE ? INDIGO_OK_STATE : INDIGO_ALERT_STATE;
	}
	AGENT_GUIDER_SETTINGS_EXPOSURE_ITEM->number.value = saved_exposure_time;
	AGENT_GUIDER_STATS_PHASE_ITEM->number.value = GUIDING;
	if (AGENT_GUIDER_DETECTION_SELECTION_ITEM->sw.value && AGENT_GUIDER_SELECTION_STAR_COUNT_ITEM->number.value == 1) {
		AGENT_GUIDER_STATS_FRAME_ITEM->number.value = -1;
		indigo_update_property(device, AGENT_GUIDER_STATS_PROPERTY, NULL);
		select_subframe(device);
		AGENT_GUIDER_STATS_FRAME_ITEM->number.value = 0;
	}
	indigo_update_property(device, AGENT_GUIDER_STATS_PROPERTY, NULL);
	AGENT_GUIDER_SETTINGS_DITH_X_ITEM->number.value =
	AGENT_GUIDER_SETTINGS_DITH_X_ITEM->number.target =
	AGENT_GUIDER_SETTINGS_DITH_Y_ITEM->number.value =
	AGENT_GUIDER_SETTINGS_DITH_Y_ITEM->number.target = 0;
	indigo_update_property(device, AGENT_GUIDER_SETTINGS_PROPERTY, NULL);
	DEVICE_PRIVATE_DATA->rmse_ra_sum = DEVICE_PRIVATE_DATA->rmse_dec_sum = DEVICE_PRIVATE_DATA->rmse_count = 0;
	while (AGENT_START_PROCESS_PROPERTY->state == INDIGO_BUSY_STATE) {
		if (capture_raw_frame(device) != INDIGO_OK_STATE) {
			AGENT_START_PROCESS_PROPERTY->state = AGENT_START_PROCESS_PROPERTY->state == INDIGO_OK_STATE ? INDIGO_OK_STATE : INDIGO_ALERT_STATE;
			break;
		}
		if (DEVICE_PRIVATE_DATA->drift_x || DEVICE_PRIVATE_DATA->drift_y) {
			double angle = -PI * AGENT_GUIDER_SETTINGS_ANGLE_ITEM->number.value / 180;
			double sin_angle = sin(angle);
			double cos_angle = cos(angle);
			double min_error = AGENT_GUIDER_SETTINGS_MIN_ERR_ITEM->number.value;
			double min_pulse = AGENT_GUIDER_SETTINGS_MIN_PULSE_ITEM->number.value;
			double max_pulse = AGENT_GUIDER_SETTINGS_MAX_PULSE_ITEM->number.value;
			double drift_ra = DEVICE_PRIVATE_DATA->drift_x * cos_angle + DEVICE_PRIVATE_DATA->drift_y * sin_angle;
			double drift_dec = DEVICE_PRIVATE_DATA->drift_x * sin_angle - DEVICE_PRIVATE_DATA->drift_y * cos_angle;
			double avg_drift_ra = DEVICE_PRIVATE_DATA->avg_drift_x * cos_angle + DEVICE_PRIVATE_DATA->avg_drift_y * sin_angle;
			double avg_drift_dec = DEVICE_PRIVATE_DATA->avg_drift_x * sin_angle - DEVICE_PRIVATE_DATA->avg_drift_y * cos_angle;
			AGENT_GUIDER_STATS_DRIFT_RA_ITEM->number.value = round(1000 * drift_ra) / 1000;
			AGENT_GUIDER_STATS_DRIFT_DEC_ITEM->number.value = round(1000 * drift_dec) / 1000;
			double correction_ra = 0, correction_dec = 0;
			if (fabs(drift_ra) > min_error) {
				correction_ra = indigo_guider_reponse(
					AGENT_GUIDER_SETTINGS_AGG_RA_ITEM->number.value / 100,
					AGENT_GUIDER_SETTINGS_I_GAIN_RA_ITEM->number.value,
					AGENT_GUIDER_SETTINGS_EXPOSURE_ITEM->number.value + AGENT_GUIDER_SETTINGS_DELAY_ITEM->number.value,
					drift_ra,
					avg_drift_ra
				);
				correction_ra = correction_ra / AGENT_GUIDER_SETTINGS_SPEED_RA_ITEM->number.value;
				if (correction_ra > max_pulse)
					correction_ra = max_pulse;
				else if (correction_ra < -max_pulse)
					correction_ra = -max_pulse;
				else if (fabs(correction_ra) < min_pulse)
					correction_ra = 0;
			}
			if (fabs(drift_dec) > min_error) {
				correction_dec = indigo_guider_reponse(
					AGENT_GUIDER_SETTINGS_AGG_DEC_ITEM->number.value / 100,
					AGENT_GUIDER_SETTINGS_I_GAIN_DEC_ITEM->number.value,
					AGENT_GUIDER_SETTINGS_EXPOSURE_ITEM->number.value + AGENT_GUIDER_SETTINGS_DELAY_ITEM->number.value,
					drift_dec,
					avg_drift_dec
				);
				correction_dec = correction_dec / AGENT_GUIDER_SETTINGS_SPEED_DEC_ITEM->number.value;
				if (correction_dec > max_pulse)
					correction_dec = max_pulse;
				else if (correction_dec < -max_pulse)
					correction_dec = -max_pulse;
				else if (fabs(correction_dec) < min_pulse)
					correction_dec = 0;
			}
			if (AGENT_GUIDER_DEC_MODE_NONE_ITEM->sw.value)
				correction_dec = 0;
			else if (AGENT_GUIDER_DEC_MODE_NORTH_ITEM->sw.value && correction_dec < 0)
				correction_dec = 0;
			else if (AGENT_GUIDER_DEC_MODE_SOUTH_ITEM->sw.value && correction_dec > 0)
				correction_dec = 0;
			AGENT_GUIDER_STATS_CORR_RA_ITEM->number.value = round(1000 * correction_ra) / 1000;
			AGENT_GUIDER_STATS_CORR_DEC_ITEM->number.value = round(1000 * correction_dec) / 1000;
			if (pulse_guide(device, correction_ra, correction_dec) != INDIGO_OK_STATE) {
				AGENT_START_PROCESS_PROPERTY->state = AGENT_START_PROCESS_PROPERTY->state == INDIGO_OK_STATE ? INDIGO_OK_STATE : INDIGO_ALERT_STATE;
				break;
			}
			if (AGENT_GUIDER_STATS_DITHERING_ITEM->number.value == 0) {
				DEVICE_PRIVATE_DATA->rmse_ra_sum += drift_ra * drift_ra;
				DEVICE_PRIVATE_DATA->rmse_dec_sum += drift_dec * drift_dec;
				DEVICE_PRIVATE_DATA->rmse_count++;
			} else {
				DEVICE_PRIVATE_DATA->rmse_ra_sum = 0;
				DEVICE_PRIVATE_DATA->rmse_dec_sum = 0;
				if (DEVICE_PRIVATE_DATA->rmse_count < AGENT_GUIDER_SETTINGS_DITH_LIMIT_ITEM->number.value)
					DEVICE_PRIVATE_DATA->rmse_count++;
				for (int i = 0; i < DEVICE_PRIVATE_DATA->rmse_count; i++) {
					double drift_ra_i =  DEVICE_PRIVATE_DATA->stack_x[i] * cos_angle + DEVICE_PRIVATE_DATA->stack_y[i] * sin_angle;
					double drift_dec_i = DEVICE_PRIVATE_DATA->stack_x[i] * sin_angle - DEVICE_PRIVATE_DATA->stack_y[i] * cos_angle;
					DEVICE_PRIVATE_DATA->rmse_ra_sum += drift_ra_i * drift_ra_i;
					DEVICE_PRIVATE_DATA->rmse_dec_sum += drift_dec_i * drift_dec_i;
				}
			}
			AGENT_GUIDER_STATS_RMSE_RA_ITEM->number.value = round(1000 * sqrt(DEVICE_PRIVATE_DATA->rmse_ra_sum / DEVICE_PRIVATE_DATA->rmse_count)) / 1000;
			AGENT_GUIDER_STATS_RMSE_DEC_ITEM->number.value = round(1000 * sqrt(DEVICE_PRIVATE_DATA->rmse_dec_sum / DEVICE_PRIVATE_DATA->rmse_count)) / 1000;
			if (AGENT_GUIDER_STATS_DITHERING_ITEM->number.value != 0) {
				bool dithering_finished = false;
				if (AGENT_GUIDER_DEC_MODE_NONE_ITEM->sw.value) {
					if (DEVICE_PRIVATE_DATA->rmse_ra_threshold > 0)
						dithering_finished = DEVICE_PRIVATE_DATA->rmse_count >= AGENT_GUIDER_SETTINGS_DITH_LIMIT_ITEM->number.value && AGENT_GUIDER_STATS_RMSE_RA_ITEM->number.value < DEVICE_PRIVATE_DATA->rmse_ra_threshold;
					else
						dithering_finished = DEVICE_PRIVATE_DATA->rmse_count >= AGENT_GUIDER_SETTINGS_DITH_LIMIT_ITEM->number.value;
				} else {
					if (DEVICE_PRIVATE_DATA->rmse_ra_threshold > 0 && DEVICE_PRIVATE_DATA->rmse_dec_threshold > 0)
						dithering_finished = DEVICE_PRIVATE_DATA->rmse_count >= AGENT_GUIDER_SETTINGS_DITH_LIMIT_ITEM->number.value && AGENT_GUIDER_STATS_RMSE_RA_ITEM->number.value < DEVICE_PRIVATE_DATA->rmse_ra_threshold && AGENT_GUIDER_STATS_RMSE_DEC_ITEM->number.value < DEVICE_PRIVATE_DATA->rmse_dec_threshold;
					else
						dithering_finished = DEVICE_PRIVATE_DATA->rmse_count >= AGENT_GUIDER_SETTINGS_DITH_LIMIT_ITEM->number.value;
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
	}
	AGENT_GUIDER_STATS_PHASE_ITEM->number.value = AGENT_ABORT_PROCESS_PROPERTY->state == INDIGO_BUSY_STATE ? DONE : FAILED;
	AGENT_GUIDER_STATS_DITHERING_ITEM->number.value = 0;
	indigo_update_property(device, AGENT_GUIDER_STATS_PROPERTY, NULL);
	AGENT_GUIDER_START_PREVIEW_ITEM->sw.value =
	AGENT_GUIDER_START_CALIBRATION_ITEM->sw.value =
	AGENT_GUIDER_START_CALIBRATION_AND_GUIDING_ITEM->sw.value =
	AGENT_GUIDER_START_GUIDING_ITEM->sw.value = false;
	AGENT_START_PROCESS_PROPERTY->state = AGENT_ABORT_PROCESS_PROPERTY->state == INDIGO_BUSY_STATE ? INDIGO_OK_STATE : INDIGO_ALERT_STATE;
	indigo_update_property(device, AGENT_START_PROCESS_PROPERTY, NULL);
	if (AGENT_ABORT_PROCESS_PROPERTY->state == INDIGO_BUSY_STATE) {
		AGENT_ABORT_PROCESS_PROPERTY->state = INDIGO_OK_STATE;
		indigo_update_property(device, AGENT_ABORT_PROCESS_PROPERTY, NULL);
		indigo_send_message(device, "Guiding finished");
	} else {
		indigo_send_message(device, "Guiding failed");
	}
	restore_subframe(device);
	FILTER_DEVICE_CONTEXT->running_process = false;
}

static void find_stars_process(indigo_device *device) {
	FILTER_DEVICE_CONTEXT->running_process = true;
	AGENT_GUIDER_STATS_PHASE_ITEM->number.value = PREVIEW;
	AGENT_GUIDER_STATS_FRAME_ITEM->number.value =
	AGENT_GUIDER_STATS_FRAME_ITEM->number.value =
	AGENT_GUIDER_STATS_REFERENCE_X_ITEM->number.value =
	AGENT_GUIDER_STATS_REFERENCE_Y_ITEM->number.value =
	AGENT_GUIDER_STATS_DRIFT_X_ITEM->number.value =
	AGENT_GUIDER_STATS_DRIFT_Y_ITEM->number.value =
	AGENT_GUIDER_STATS_DRIFT_RA_ITEM->number.value =
	AGENT_GUIDER_STATS_DRIFT_DEC_ITEM->number.value =
	AGENT_GUIDER_STATS_CORR_RA_ITEM->number.value =
	AGENT_GUIDER_STATS_CORR_DEC_ITEM->number.value =
	AGENT_GUIDER_STATS_RMSE_RA_ITEM->number.value =
	AGENT_GUIDER_STATS_RMSE_DEC_ITEM->number.value =
	AGENT_GUIDER_STATS_SNR_ITEM->number.value =
	AGENT_GUIDER_STATS_DITHERING_ITEM->number.value = 0;
	if (capture_raw_frame(device) != INDIGO_OK_STATE) {
		AGENT_GUIDER_STARS_PROPERTY->state = INDIGO_ALERT_STATE;
		indigo_update_property(device, AGENT_GUIDER_STARS_PROPERTY, NULL);
	}
	indigo_update_property(device, AGENT_GUIDER_STATS_PROPERTY, NULL);
	if (AGENT_ABORT_PROCESS_PROPERTY->state == INDIGO_BUSY_STATE) {
		AGENT_ABORT_PROCESS_PROPERTY->state = INDIGO_OK_STATE;
		indigo_update_property(device, AGENT_ABORT_PROCESS_PROPERTY, NULL);
	}
	FILTER_DEVICE_CONTEXT->running_process = false;
}

static void abort_process(indigo_device *device) {
	indigo_change_switch_property_1(FILTER_DEVICE_CONTEXT->client, FILTER_DEVICE_CONTEXT->device_name[INDIGO_FILTER_CCD_INDEX], CCD_ABORT_EXPOSURE_PROPERTY_NAME, CCD_ABORT_EXPOSURE_ITEM_NAME, true);
}

// -------------------------------------------------------------------------------- INDIGO agent device implementation

static indigo_result agent_enumerate_properties(indigo_device *device, indigo_client *client, indigo_property *property);

static indigo_result agent_device_attach(indigo_device *device) {
	assert(device != NULL);
	assert(DEVICE_PRIVATE_DATA != NULL);
	if (indigo_filter_device_attach(device, DRIVER_NAME, DRIVER_VERSION, INDIGO_INTERFACE_CCD | INDIGO_INTERFACE_GUIDER) == INDIGO_OK) {
		// -------------------------------------------------------------------------------- Device properties
		FILTER_CCD_LIST_PROPERTY->hidden = false;
		FILTER_GUIDER_LIST_PROPERTY->hidden = false;
		// -------------------------------------------------------------------------------- Process properties
		AGENT_GUIDER_DETECTION_MODE_PROPERTY = indigo_init_switch_property(NULL, device->name, AGENT_GUIDER_DETECTION_MODE_PROPERTY_NAME, "Agent", "Drift detection mode", INDIGO_OK_STATE, INDIGO_RW_PERM, INDIGO_ONE_OF_MANY_RULE, 3);
		if (AGENT_GUIDER_DETECTION_MODE_PROPERTY == NULL)
			return INDIGO_FAILED;
		indigo_init_switch_item(AGENT_GUIDER_DETECTION_DONUTS_ITEM, AGENT_GUIDER_DETECTION_DONUTS_ITEM_NAME, "Donuts", true);
		indigo_init_switch_item(AGENT_GUIDER_DETECTION_SELECTION_ITEM, AGENT_GUIDER_DETECTION_SELECTION_ITEM_NAME, "Selection", false);
		indigo_init_switch_item(AGENT_GUIDER_DETECTION_CENTROID_ITEM, AGENT_GUIDER_DETECTION_CENTROID_ITEM_NAME, "Centroid", false);
		AGENT_GUIDER_DEC_MODE_PROPERTY = indigo_init_switch_property(NULL, device->name, AGENT_GUIDER_DEC_MODE_PROPERTY_NAME, "Agent", "Dec guiding mode", INDIGO_OK_STATE, INDIGO_RW_PERM, INDIGO_ONE_OF_MANY_RULE, 4);
		if (AGENT_GUIDER_DEC_MODE_PROPERTY == NULL)
			return INDIGO_FAILED;
		indigo_init_switch_item(AGENT_GUIDER_DEC_MODE_BOTH_ITEM, AGENT_GUIDER_DEC_MODE_BOTH_ITEM_NAME, "North and south", true);
		indigo_init_switch_item(AGENT_GUIDER_DEC_MODE_NORTH_ITEM, AGENT_GUIDER_DEC_MODE_NORTH_ITEM_NAME, "North only", false);
		indigo_init_switch_item(AGENT_GUIDER_DEC_MODE_SOUTH_ITEM, AGENT_GUIDER_DEC_MODE_SOUTH_ITEM_NAME, "South only", false);
		indigo_init_switch_item(AGENT_GUIDER_DEC_MODE_NONE_ITEM, AGENT_GUIDER_DEC_MODE_NONE_ITEM_NAME, "None", false);
		AGENT_START_PROCESS_PROPERTY = indigo_init_switch_property(NULL, device->name, AGENT_START_PROCESS_PROPERTY_NAME, "Agent", "Start process", INDIGO_OK_STATE, INDIGO_RW_PERM, INDIGO_ANY_OF_MANY_RULE, 4);
		if (AGENT_START_PROCESS_PROPERTY == NULL)
			return INDIGO_FAILED;
		indigo_init_switch_item(AGENT_GUIDER_START_PREVIEW_ITEM, AGENT_GUIDER_START_PREVIEW_ITEM_NAME, "Start preview", false);
		indigo_init_switch_item(AGENT_GUIDER_START_CALIBRATION_ITEM, AGENT_GUIDER_START_CALIBRATION_ITEM_NAME, "Start calibration", false);
		indigo_init_switch_item(AGENT_GUIDER_START_CALIBRATION_AND_GUIDING_ITEM, AGENT_GUIDER_START_CALIBRATION_AND_GUIDING_ITEM_NAME, "Start calibration and guiding", false);
		indigo_init_switch_item(AGENT_GUIDER_START_GUIDING_ITEM, AGENT_GUIDER_START_GUIDING_ITEM_NAME, "Start guiding", false);
		AGENT_ABORT_PROCESS_PROPERTY = indigo_init_switch_property(NULL, device->name, AGENT_ABORT_PROCESS_PROPERTY_NAME, "Agent", "Abort", INDIGO_OK_STATE, INDIGO_RW_PERM, INDIGO_ANY_OF_MANY_RULE, 1);
		if (AGENT_ABORT_PROCESS_PROPERTY == NULL)
			return INDIGO_FAILED;
		indigo_init_switch_item(AGENT_ABORT_PROCESS_ITEM, AGENT_ABORT_PROCESS_ITEM_NAME, "Abort", false);
		// -------------------------------------------------------------------------------- Guiding settings
		AGENT_GUIDER_SETTINGS_PROPERTY = indigo_init_number_property(NULL, device->name, AGENT_GUIDER_SETTINGS_PROPERTY_NAME, "Agent", "Settings", INDIGO_OK_STATE, INDIGO_RW_PERM, 22);
		if (AGENT_GUIDER_SETTINGS_PROPERTY == NULL)
			return INDIGO_FAILED;
		indigo_init_number_item(AGENT_GUIDER_SETTINGS_EXPOSURE_ITEM, AGENT_GUIDER_SETTINGS_EXPOSURE_ITEM_NAME, "Exposure time (s)", 0, 120, 1, 1);
		indigo_init_number_item(AGENT_GUIDER_SETTINGS_DELAY_ITEM, AGENT_GUIDER_SETTINGS_DELAY_ITEM_NAME, "Delay time (s)", 0, 120, 1, 0);
		indigo_init_number_item(AGENT_GUIDER_SETTINGS_STEP_ITEM, AGENT_GUIDER_SETTINGS_STEP_ITEM_NAME, "Calibration step (s)", 0.05, 2, 0.05, 0.200);
		indigo_init_number_item(AGENT_GUIDER_SETTINGS_BL_STEPS_ITEM, AGENT_GUIDER_SETTINGS_BL_STEPS_ITEM_NAME, "Max clear backlash steps", 0, 50, 1, 10);
		indigo_init_number_item(AGENT_GUIDER_SETTINGS_BL_DRIFT_ITEM, AGENT_GUIDER_SETTINGS_BL_DRIFT_ITEM_NAME, "Min clear backlash drift (px)", 0, 25, 1, 3);
		indigo_init_number_item(AGENT_GUIDER_SETTINGS_CAL_STEPS_ITEM, AGENT_GUIDER_SETTINGS_CAL_STEPS_ITEM_NAME, "Max calibration steps", 0, 50, 1, 20);
		indigo_init_number_item(AGENT_GUIDER_SETTINGS_CAL_DRIFT_ITEM, AGENT_GUIDER_SETTINGS_CAL_DRIFT_ITEM_NAME, "Min calibration drift (px)", 0, 100, 5, 20);
		indigo_init_number_item(AGENT_GUIDER_SETTINGS_ANGLE_ITEM, AGENT_GUIDER_SETTINGS_ANGLE_ITEM_NAME, "Angle (°)", -180, 180, 1, 0);
		indigo_init_number_item(AGENT_GUIDER_SETTINGS_BACKLASH_ITEM, AGENT_GUIDER_SETTINGS_BACKLASH_ITEM_NAME, "Dec backlash (px)", 0, 100, 0, 0);
		indigo_init_number_item(AGENT_GUIDER_SETTINGS_SPEED_RA_ITEM, AGENT_GUIDER_SETTINGS_SPEED_RA_ITEM_NAME, "RA speed (px/s)", -500, 500, 0.1, 0);
		indigo_init_number_item(AGENT_GUIDER_SETTINGS_SPEED_DEC_ITEM, AGENT_GUIDER_SETTINGS_SPEED_DEC_ITEM_NAME, "Dec speed (px/s)", -500, 500, 0.1, 0);
		indigo_init_number_item(AGENT_GUIDER_SETTINGS_MIN_ERR_ITEM, AGENT_GUIDER_SETTINGS_MIN_ERR_ITEM_NAME, "Min error (px)", 0, 5, 0.1, 0);
		indigo_init_number_item(AGENT_GUIDER_SETTINGS_MIN_PULSE_ITEM, AGENT_GUIDER_SETTINGS_MIN_PULSE_ITEM_NAME, "Min pulse (s)", 0, 1, 0.01, 0.02);
		indigo_init_number_item(AGENT_GUIDER_SETTINGS_MAX_PULSE_ITEM, AGENT_GUIDER_SETTINGS_MAX_PULSE_ITEM_NAME, "Max pulse (s)", 0, 5, 0.1, 1);
		indigo_init_number_item(AGENT_GUIDER_SETTINGS_AGG_RA_ITEM, AGENT_GUIDER_SETTINGS_AGG_RA_ITEM_NAME, "RA Proportional aggressivity (%)", 0, 500, 5, 100);
		indigo_init_number_item(AGENT_GUIDER_SETTINGS_AGG_DEC_ITEM, AGENT_GUIDER_SETTINGS_AGG_DEC_ITEM_NAME, "Dec Proportional aggressivity (%)", 0, 500, 5, 100);
		indigo_init_number_item(AGENT_GUIDER_SETTINGS_I_GAIN_RA_ITEM, AGENT_GUIDER_SETTINGS_I_GAIN_RA_ITEM_NAME, "RA Integral gain", 0, 10, 0.05, 0.5);
		indigo_init_number_item(AGENT_GUIDER_SETTINGS_I_GAIN_DEC_ITEM, AGENT_GUIDER_SETTINGS_I_GAIN_DEC_ITEM_NAME, "Dec Integral gain", 0, 10, 0.05, 0.5);
		indigo_init_number_item(AGENT_GUIDER_SETTINGS_STACK_ITEM, AGENT_GUIDER_SETTINGS_STACK_ITEM_NAME, "Integral stack size (frames)", 1, MAX_STACK, 1, 1);
		indigo_init_number_item(AGENT_GUIDER_SETTINGS_DITH_X_ITEM, AGENT_GUIDER_SETTINGS_DITH_X_ITEM_NAME, "Dithering offset X (px)", -15, 15, 1, 0);
		indigo_init_number_item(AGENT_GUIDER_SETTINGS_DITH_Y_ITEM, AGENT_GUIDER_SETTINGS_DITH_Y_ITEM_NAME, "Dithering offset Y (px)", -15, 15, 1, 0);
		indigo_init_number_item(AGENT_GUIDER_SETTINGS_DITH_LIMIT_ITEM, AGENT_GUIDER_SETTINGS_DITH_LIMIT_ITEM_NAME, "Dithering settling limit (frames)", 1, 50, 1, 5);
		// -------------------------------------------------------------------------------- Detected stars
		AGENT_GUIDER_STARS_PROPERTY = indigo_init_switch_property(NULL, device->name, AGENT_GUIDER_STARS_PROPERTY_NAME, "Agent", "Stars", INDIGO_OK_STATE, INDIGO_RW_PERM, INDIGO_ONE_OF_MANY_RULE, MAX_STAR_COUNT + 1);
		if (AGENT_GUIDER_STARS_PROPERTY == NULL)
			return INDIGO_FAILED;
		AGENT_GUIDER_STARS_PROPERTY->count = 1;
		indigo_init_switch_item(AGENT_GUIDER_STARS_REFRESH_ITEM, AGENT_GUIDER_STARS_REFRESH_ITEM_NAME, "Refresh", false);
		// -------------------------------------------------------------------------------- Selected star
		AGENT_GUIDER_SELECTION_PROPERTY = indigo_init_number_property(NULL, device->name, AGENT_GUIDER_SELECTION_PROPERTY_NAME, "Agent", "Selection", INDIGO_OK_STATE, INDIGO_RW_PERM, 4 + 2 * MAX_MULTISTAR_COUNT);
		if (AGENT_GUIDER_SELECTION_PROPERTY == NULL)
			return INDIGO_FAILED;
		indigo_init_number_item(AGENT_GUIDER_SELECTION_RADIUS_ITEM, AGENT_GUIDER_SELECTION_RADIUS_ITEM_NAME, "Radius (px)", 1, 50, 1, 8);
		indigo_init_number_item(AGENT_GUIDER_SELECTION_SUBFRAME_ITEM, AGENT_GUIDER_SELECTION_SUBFRAME_ITEM_NAME, "Subframe", 0, 20, 1, 0);
		indigo_init_number_item(AGENT_GUIDER_SELECTION_EDGE_CLIPPING_ITEM, AGENT_GUIDER_SELECTION_EDGE_CLIPPING_ITEM_NAME, "Edge Clipping (px)", 0, 500, 1, 8);
		indigo_init_number_item(AGENT_GUIDER_SELECTION_STAR_COUNT_ITEM, AGENT_GUIDER_SELECTION_STAR_COUNT_ITEM_NAME, "Maximaum number of stars", 1, MAX_MULTISTAR_COUNT, 1, 1);
		for (int i = 0; i < MAX_MULTISTAR_COUNT; i++) {
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
		AGENT_GUIDER_SELECTION_PROPERTY->count = 6;
		// -------------------------------------------------------------------------------- Guiding stats
		AGENT_GUIDER_STATS_PROPERTY = indigo_init_number_property(NULL, device->name, AGENT_GUIDER_STATS_PROPERTY_NAME, "Agent", "Statistics", INDIGO_OK_STATE, INDIGO_RO_PERM, 15);
		if (AGENT_GUIDER_STATS_PROPERTY == NULL)
			return INDIGO_FAILED;
		indigo_init_number_item(AGENT_GUIDER_STATS_PHASE_ITEM, AGENT_GUIDER_STATS_PHASE_ITEM_NAME, "Phase #", -1, 100, 0, DONE);
		indigo_init_number_item(AGENT_GUIDER_STATS_FRAME_ITEM, AGENT_GUIDER_STATS_FRAME_ITEM_NAME, "Frame #", 0, 0xFFFFFFFF, 0, 0);
		indigo_init_number_item(AGENT_GUIDER_STATS_REFERENCE_X_ITEM, AGENT_GUIDER_STATS_REFERENCE_X_ITEM_NAME, "Reference X (px)", 0, 100000, 0, 0);
		indigo_init_number_item(AGENT_GUIDER_STATS_REFERENCE_Y_ITEM, AGENT_GUIDER_STATS_REFERENCE_Y_ITEM_NAME, "Reference Y (px)", 0, 100000, 0, 0);
		indigo_init_number_item(AGENT_GUIDER_STATS_DRIFT_X_ITEM, AGENT_GUIDER_STATS_DRIFT_X_ITEM_NAME, "Drift X (px)", -1000, 1000, 0, 0);
		indigo_init_number_item(AGENT_GUIDER_STATS_DRIFT_Y_ITEM, AGENT_GUIDER_STATS_DRIFT_Y_ITEM_NAME, "Drift Y (px)", -1000, 1000, 0, 0);
		indigo_init_number_item(AGENT_GUIDER_STATS_DRIFT_RA_ITEM, AGENT_GUIDER_STATS_DRIFT_RA_ITEM_NAME, "Drift RA (px)", -1000, 1000, 0, 0);
		indigo_init_number_item(AGENT_GUIDER_STATS_DRIFT_DEC_ITEM, AGENT_GUIDER_STATS_DRIFT_DEC_ITEM_NAME, "Drift Dec (px)", -1000, 1000, 0, 0);
		indigo_init_number_item(AGENT_GUIDER_STATS_CORR_RA_ITEM, AGENT_GUIDER_STATS_CORR_RA_ITEM_NAME, "Correction RA (s)", -1000, 1000, 0, 0);
		indigo_init_number_item(AGENT_GUIDER_STATS_CORR_DEC_ITEM, AGENT_GUIDER_STATS_CORR_DEC_ITEM_NAME, "Correction Dec (s)", -1000, 1000, 0, 0);
		indigo_init_number_item(AGENT_GUIDER_STATS_RMSE_RA_ITEM, AGENT_GUIDER_STATS_RMSE_RA_ITEM_NAME, "RMSE RA (px)", -1000, 1000, 0, 0);
		indigo_init_number_item(AGENT_GUIDER_STATS_RMSE_DEC_ITEM, AGENT_GUIDER_STATS_RMSE_DEC_ITEM_NAME, "RMSE Dec (px)", -1000, 1000, 0, 0);
		indigo_init_number_item(AGENT_GUIDER_STATS_SNR_ITEM, AGENT_GUIDER_STATS_SNR_ITEM_NAME, "SNR", 0, 1000, 0, 0);
		indigo_init_number_item(AGENT_GUIDER_STATS_DELAY_ITEM, AGENT_GUIDER_STATS_DELAY_ITEM_NAME, "Remaining delay (s)", 0, 100, 0, 0);
		indigo_init_number_item(AGENT_GUIDER_STATS_DITHERING_ITEM, AGENT_GUIDER_STATS_DITHERING_ITEM_NAME, "Dithering RMSE (px)", 0, 100, 0, 0);
		// --------------------------------------------------------------------------------
		CONNECTION_PROPERTY->hidden = true;
		ADDITIONAL_INSTANCES_PROPERTY->hidden = DEVICE_CONTEXT->base_device != NULL;
		pthread_mutex_init(&DEVICE_PRIVATE_DATA->mutex, NULL);
		indigo_load_properties(device, false);
		INDIGO_DEVICE_ATTACH_LOG(DRIVER_NAME, device->name);
		return agent_enumerate_properties(device, NULL, NULL);
	}
	return INDIGO_FAILED;
}

static indigo_result agent_enumerate_properties(indigo_device *device, indigo_client *client, indigo_property *property) {
	if (client != NULL && client == FILTER_DEVICE_CONTEXT->client)
		return INDIGO_OK;
	if (indigo_property_match(AGENT_GUIDER_DETECTION_MODE_PROPERTY, property))
		indigo_define_property(device, AGENT_GUIDER_DETECTION_MODE_PROPERTY, NULL);
	if (indigo_property_match(AGENT_GUIDER_SETTINGS_PROPERTY, property))
		indigo_define_property(device, AGENT_GUIDER_SETTINGS_PROPERTY, NULL);
	if (indigo_property_match(AGENT_GUIDER_STARS_PROPERTY, property))
		indigo_define_property(device, AGENT_GUIDER_STARS_PROPERTY, NULL);
	if (indigo_property_match(AGENT_GUIDER_SELECTION_PROPERTY, property))
		indigo_define_property(device, AGENT_GUIDER_SELECTION_PROPERTY, NULL);
	if (indigo_property_match(AGENT_GUIDER_STATS_PROPERTY, property))
		indigo_define_property(device, AGENT_GUIDER_STATS_PROPERTY, NULL);
	if (indigo_property_match(AGENT_GUIDER_DEC_MODE_PROPERTY, property))
		indigo_define_property(device, AGENT_GUIDER_DEC_MODE_PROPERTY, NULL);
	if (indigo_property_match(AGENT_START_PROCESS_PROPERTY, property))
		indigo_define_property(device, AGENT_START_PROCESS_PROPERTY, NULL);
	if (indigo_property_match(AGENT_ABORT_PROCESS_PROPERTY, property))
		indigo_define_property(device, AGENT_ABORT_PROCESS_PROPERTY, NULL);
	return indigo_filter_enumerate_properties(device, client, property);
}

static indigo_result agent_change_property(indigo_device *device, indigo_client *client, indigo_property *property) {
	assert(device != NULL);
	assert(DEVICE_CONTEXT != NULL);
	assert(property != NULL);
	if (client == FILTER_DEVICE_CONTEXT->client)
		return INDIGO_OK;
	if (indigo_property_match_w(FILTER_CCD_LIST_PROPERTY, property)) {
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
	} else if (indigo_property_match_w(AGENT_GUIDER_DETECTION_MODE_PROPERTY, property)) {
// -------------------------------------------------------------------------------- AGENT_GUIDER_DETECTION_MODE
		if (FILTER_DEVICE_CONTEXT->running_process) {
			indigo_update_property(device, AGENT_GUIDER_DETECTION_MODE_PROPERTY, "Detection mode can not be changed while process is running!");
			return INDIGO_OK;
		}
		indigo_property_copy_values(AGENT_GUIDER_DETECTION_MODE_PROPERTY, property, false);
		AGENT_GUIDER_DETECTION_MODE_PROPERTY->state = INDIGO_OK_STATE;
		save_config(device);
		indigo_update_property(device, AGENT_GUIDER_DETECTION_MODE_PROPERTY, NULL);
	} else if (indigo_property_match(AGENT_GUIDER_DEC_MODE_PROPERTY, property)) {
// -------------------------------------------------------------------------------- AGENT_GUIDER_DEC_MODE
		indigo_property_copy_values(AGENT_GUIDER_DEC_MODE_PROPERTY, property, false);
		AGENT_GUIDER_DEC_MODE_PROPERTY->state = INDIGO_OK_STATE;
		save_config(device);
		indigo_update_property(device, AGENT_GUIDER_DEC_MODE_PROPERTY, NULL);
	} else if (indigo_property_match(AGENT_GUIDER_SETTINGS_PROPERTY, property)) {
// -------------------------------------------------------------------------------- AGENT_GUIDER_SETTINGS
		double dith_x = AGENT_GUIDER_SETTINGS_DITH_X_ITEM->number.value;
		double dith_y = AGENT_GUIDER_SETTINGS_DITH_Y_ITEM->number.value;
		indigo_property_copy_values(AGENT_GUIDER_SETTINGS_PROPERTY, property, false);
		AGENT_GUIDER_SETTINGS_PROPERTY->state = INDIGO_OK_STATE;
		bool update_stats = false;
		if (DEVICE_PRIVATE_DATA->reference->algorithm == centroid) {
			AGENT_GUIDER_STATS_REFERENCE_X_ITEM->number.value = DEVICE_PRIVATE_DATA->reference->centroid_x + AGENT_GUIDER_SETTINGS_DITH_X_ITEM->number.value;
			AGENT_GUIDER_STATS_REFERENCE_Y_ITEM->number.value = DEVICE_PRIVATE_DATA->reference->centroid_y + AGENT_GUIDER_SETTINGS_DITH_Y_ITEM->number.value;
			update_stats = true;
		}
		if (AGENT_GUIDER_STATS_PHASE_ITEM->number.value == GUIDING && (dith_x != AGENT_GUIDER_SETTINGS_DITH_X_ITEM->number.value || dith_y != AGENT_GUIDER_SETTINGS_DITH_Y_ITEM->number.value)) {
			double diff_x = fabs(AGENT_GUIDER_SETTINGS_DITH_X_ITEM->number.value - dith_x);
			double diff_y = fabs(AGENT_GUIDER_SETTINGS_DITH_Y_ITEM->number.value - dith_y);
			DEVICE_PRIVATE_DATA->rmse_ra_sum = DEVICE_PRIVATE_DATA->rmse_dec_sum = DEVICE_PRIVATE_DATA->rmse_count = 0;
			DEVICE_PRIVATE_DATA->rmse_ra_threshold = 1.5 * AGENT_GUIDER_STATS_RMSE_RA_ITEM->number.value + AGENT_GUIDER_SETTINGS_MIN_ERR_ITEM->number.value/2.0;
			DEVICE_PRIVATE_DATA->rmse_dec_threshold = 1.5 * AGENT_GUIDER_STATS_RMSE_DEC_ITEM->number.value + AGENT_GUIDER_SETTINGS_MIN_ERR_ITEM->number.value/2.0;
			INDIGO_DRIVER_DEBUG(DRIVER_NAME, "Dithering RMSE RA threshold = %g, RMSE DEC threshold = %g ", DEVICE_PRIVATE_DATA->rmse_ra_threshold, DEVICE_PRIVATE_DATA->rmse_dec_threshold);
			AGENT_GUIDER_STATS_DITHERING_ITEM->number.value = round(1000 * fmax(fabs(diff_x), fabs(diff_y))) / 1000;
			update_stats = true;
		}
		if (update_stats) {
			indigo_update_property(device, AGENT_GUIDER_STATS_PROPERTY, NULL);
		}
		save_config(device);
		indigo_update_property(device, AGENT_GUIDER_SETTINGS_PROPERTY, NULL);
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
	} else if (indigo_property_match(AGENT_GUIDER_SELECTION_PROPERTY, property)) {
// -------------------------------------------------------------------------------- AGENT_GUIDER_SELECTION
		if (FILTER_DEVICE_CONTEXT->running_process) {
			indigo_item *item = indigo_get_item(property, AGENT_GUIDER_SELECTION_EDGE_CLIPPING_ITEM_NAME);
			if (item && AGENT_GUIDER_SELECTION_EDGE_CLIPPING_ITEM->number.value != item->number.value) {
				indigo_update_property(device, AGENT_GUIDER_SELECTION_PROPERTY, "Edge clipping can not be changed while process is running!");
				return INDIGO_OK;
			}
		}
		int count = AGENT_GUIDER_SELECTION_STAR_COUNT_ITEM->number.value;
		indigo_property_copy_values(AGENT_GUIDER_SELECTION_PROPERTY, property, false);
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
		AGENT_GUIDER_SELECTION_PROPERTY->state = INDIGO_OK_STATE;
		save_config(device);
		indigo_update_property(device, AGENT_GUIDER_SELECTION_PROPERTY, NULL);
	} else if (indigo_property_match(AGENT_START_PROCESS_PROPERTY, property)) {
// -------------------------------------------------------------------------------- AGENT_START_PROCESS
		if (AGENT_START_PROCESS_PROPERTY->state != INDIGO_BUSY_STATE && AGENT_GUIDER_STARS_PROPERTY->state != INDIGO_BUSY_STATE) {
			indigo_property_copy_values(AGENT_START_PROCESS_PROPERTY, property, false);
			if (!FILTER_CCD_LIST_PROPERTY->items->sw.value) {
				if (AGENT_GUIDER_START_PREVIEW_ITEM->sw.value) {
					AGENT_START_PROCESS_PROPERTY->state = INDIGO_BUSY_STATE;
					indigo_set_timer(device, 0, preview_process, NULL);
					indigo_update_property(device, AGENT_START_PROCESS_PROPERTY, NULL);
				} else if (!FILTER_GUIDER_LIST_PROPERTY->items->sw.value) {
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
					AGENT_GUIDER_START_PREVIEW_ITEM->sw.value =
					AGENT_GUIDER_START_CALIBRATION_ITEM->sw.value =
					AGENT_GUIDER_START_CALIBRATION_AND_GUIDING_ITEM->sw.value =
					AGENT_GUIDER_START_GUIDING_ITEM->sw.value = false;
					AGENT_START_PROCESS_PROPERTY->state = INDIGO_ALERT_STATE;
					indigo_update_property(device, AGENT_START_PROCESS_PROPERTY, "No guider is selected");
				}
			} else {
				AGENT_GUIDER_START_PREVIEW_ITEM->sw.value =
				AGENT_GUIDER_START_CALIBRATION_ITEM->sw.value =
				AGENT_GUIDER_START_CALIBRATION_AND_GUIDING_ITEM->sw.value =
				AGENT_GUIDER_START_GUIDING_ITEM->sw.value = false;
				AGENT_START_PROCESS_PROPERTY->state = INDIGO_ALERT_STATE;
				indigo_update_property(device, AGENT_START_PROCESS_PROPERTY, "No CCD is selected");
			}
		}
	} else 	if (indigo_property_match(AGENT_ABORT_PROCESS_PROPERTY, property)) {
// -------------------------------------------------------------------------------- AGENT_ABORT_PROCESS
		if (AGENT_START_PROCESS_PROPERTY->state == INDIGO_BUSY_STATE || AGENT_GUIDER_STARS_PROPERTY->state == INDIGO_BUSY_STATE) {
			indigo_property_copy_values(AGENT_ABORT_PROCESS_PROPERTY, property, false);
			AGENT_ABORT_PROCESS_PROPERTY->state = INDIGO_BUSY_STATE;
			abort_process(device);
		}
		AGENT_ABORT_PROCESS_ITEM->sw.value = false;
		indigo_update_property(device, AGENT_ABORT_PROCESS_PROPERTY, NULL);
		return INDIGO_OK;
		// -------------------------------------------------------------------------------- ADDITIONAL_INSTANCES
	} else if (indigo_property_match(ADDITIONAL_INSTANCES_PROPERTY, property)) {
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
	indigo_release_property(AGENT_GUIDER_SETTINGS_PROPERTY);
	indigo_release_property(AGENT_GUIDER_STARS_PROPERTY);
	indigo_release_property(AGENT_GUIDER_SELECTION_PROPERTY);
	indigo_release_property(AGENT_GUIDER_STATS_PROPERTY);
	indigo_release_property(AGENT_GUIDER_DEC_MODE_PROPERTY);
	for (int i = 0; i <= MAX_MULTISTAR_COUNT; i++)
		indigo_delete_frame_digest(DEVICE_PRIVATE_DATA->reference + i);
	pthread_mutex_destroy(&DEVICE_PRIVATE_DATA->mutex);
	indigo_safe_free(DEVICE_PRIVATE_DATA->last_image);
	return indigo_filter_device_detach(device);
}

// -------------------------------------------------------------------------------- INDIGO agent client implementation

static indigo_result agent_update_property(indigo_client *client, indigo_device *device, indigo_property *property, const char *message) {
	if (*FILTER_CLIENT_CONTEXT->device_name[INDIGO_FILTER_CCD_INDEX] && !strcmp(property->device, FILTER_CLIENT_CONTEXT->device_name[INDIGO_FILTER_CCD_INDEX])) {
		if (property->state == INDIGO_OK_STATE && !strcmp(property->name, CCD_IMAGE_PROPERTY_NAME)) {
			if (strchr(property->device, '@'))
				indigo_populate_http_blob_item(property->items);
			if (property->items->blob.value) {
				CLIENT_PRIVATE_DATA->last_image = indigo_safe_realloc(CLIENT_PRIVATE_DATA->last_image, property->items->blob.size);
				memcpy(CLIENT_PRIVATE_DATA->last_image, property->items->blob.value, property->items->blob.size);
			} else if (CLIENT_PRIVATE_DATA->last_image) {
				free(CLIENT_PRIVATE_DATA->last_image);
				CLIENT_PRIVATE_DATA->last_image = NULL;
			}
		}
	}
	return indigo_filter_update_property(client, device, property, message);
}

// -------------------------------------------------------------------------------- Initialization

static agent_private_data *private_data = NULL;

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
		indigo_filter_define_property,
		agent_update_property,
		indigo_filter_delete_property,
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
			private_data = indigo_safe_malloc(sizeof(agent_private_data));
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
