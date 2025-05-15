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
 \file indigo_platesolver_driver.h
 */

#ifndef indigo_platesolver_h
#define indigo_platesolver_h

#include <indigo/indigo_bus.h>
#include <indigo/indigo_driver.h>
#include <indigo/indigo_align.h>
#include <indigo/indigo_names.h>

#ifdef __cplusplus
extern "C" {
#endif

#define PLATESOLVER_MAIN_GROUP		"Plate solver"

#define INDIGO_PLATESOLVER_DEVICE_PRIVATE_DATA										((platesolver_private_data *)device->private_data)
#define INDIGO_PLATESOLVER_CLIENT_PRIVATE_DATA										((platesolver_private_data *)FILTER_CLIENT_CONTEXT->device->private_data)

#define AGENT_PLATESOLVER_USE_INDEX_PROPERTY	(INDIGO_PLATESOLVER_DEVICE_PRIVATE_DATA->use_index_property)

#define AGENT_PLATESOLVER_HINTS_PROPERTY			(INDIGO_PLATESOLVER_DEVICE_PRIVATE_DATA->hints_property)
#define AGENT_PLATESOLVER_HINTS_RADIUS_ITEM 	(AGENT_PLATESOLVER_HINTS_PROPERTY->items+0)
#define AGENT_PLATESOLVER_HINTS_RA_ITEM    		(AGENT_PLATESOLVER_HINTS_PROPERTY->items+1)
#define AGENT_PLATESOLVER_HINTS_DEC_ITEM    	(AGENT_PLATESOLVER_HINTS_PROPERTY->items+2)
#define AGENT_PLATESOLVER_HINTS_EPOCH_ITEM    (AGENT_PLATESOLVER_HINTS_PROPERTY->items+3)
#define AGENT_PLATESOLVER_HINTS_SCALE_ITEM		(AGENT_PLATESOLVER_HINTS_PROPERTY->items+4)
#define AGENT_PLATESOLVER_HINTS_PARITY_ITEM   (AGENT_PLATESOLVER_HINTS_PROPERTY->items+5)
#define AGENT_PLATESOLVER_HINTS_DOWNSAMPLE_ITEM (AGENT_PLATESOLVER_HINTS_PROPERTY->items+6)
#define AGENT_PLATESOLVER_HINTS_DEPTH_ITEM    (AGENT_PLATESOLVER_HINTS_PROPERTY->items+7)
#define AGENT_PLATESOLVER_HINTS_CPU_LIMIT_ITEM (AGENT_PLATESOLVER_HINTS_PROPERTY->items+8)

#define AGENT_PLATESOLVER_WCS_PROPERTY			(INDIGO_PLATESOLVER_DEVICE_PRIVATE_DATA->wcs_property)
#define AGENT_PLATESOLVER_WCS_STATE_ITEM    	(AGENT_PLATESOLVER_WCS_PROPERTY->items+0)
#define AGENT_PLATESOLVER_WCS_RA_ITEM    		(AGENT_PLATESOLVER_WCS_PROPERTY->items+1)
#define AGENT_PLATESOLVER_WCS_DEC_ITEM    		(AGENT_PLATESOLVER_WCS_PROPERTY->items+2)
#define AGENT_PLATESOLVER_WCS_EPOCH_ITEM    	(AGENT_PLATESOLVER_WCS_PROPERTY->items+3)
#define AGENT_PLATESOLVER_WCS_ANGLE_ITEM    	(AGENT_PLATESOLVER_WCS_PROPERTY->items+4)
#define AGENT_PLATESOLVER_WCS_WIDTH_ITEM    	(AGENT_PLATESOLVER_WCS_PROPERTY->items+5)
#define AGENT_PLATESOLVER_WCS_HEIGHT_ITEM    	(AGENT_PLATESOLVER_WCS_PROPERTY->items+6)
#define AGENT_PLATESOLVER_WCS_SCALE_ITEM    	(AGENT_PLATESOLVER_WCS_PROPERTY->items+7)
#define AGENT_PLATESOLVER_WCS_PARITY_ITEM    	(AGENT_PLATESOLVER_WCS_PROPERTY->items+8)
#define AGENT_PLATESOLVER_WCS_INDEX_ITEM    	(AGENT_PLATESOLVER_WCS_PROPERTY->items+9)

#define AGENT_PLATESOLVER_SYNC_PROPERTY				(INDIGO_PLATESOLVER_DEVICE_PRIVATE_DATA->sync_mode_property)
#define AGENT_PLATESOLVER_SYNC_DISABLED_ITEM	(AGENT_PLATESOLVER_SYNC_PROPERTY->items+0)
#define AGENT_PLATESOLVER_SYNC_SYNC_ITEM			(AGENT_PLATESOLVER_SYNC_PROPERTY->items+1)
#define AGENT_PLATESOLVER_SYNC_CENTER_ITEM		(AGENT_PLATESOLVER_SYNC_PROPERTY->items+2)
#define AGENT_PLATESOLVER_SYNC_CALCULATE_PA_ERROR_ITEM		(AGENT_PLATESOLVER_SYNC_PROPERTY->items+3)
#define AGENT_PLATESOLVER_SYNC_RECALCULATE_PA_ERROR_ITEM	(AGENT_PLATESOLVER_SYNC_PROPERTY->items+4)

#define AGENT_START_PROCESS_PROPERTY					(INDIGO_PLATESOLVER_DEVICE_PRIVATE_DATA->start_process_property)
#define AGENT_PLATESOLVER_START_SOLVE_ITEM		(AGENT_START_PROCESS_PROPERTY->items+0)
#define AGENT_PLATESOLVER_START_SYNC_ITEM			(AGENT_START_PROCESS_PROPERTY->items+1)
#define AGENT_PLATESOLVER_START_CENTER_ITEM		(AGENT_START_PROCESS_PROPERTY->items+2)
#define AGENT_PLATESOLVER_START_PRECISE_GOTO_ITEM		(AGENT_START_PROCESS_PROPERTY->items+3)
#define AGENT_PLATESOLVER_START_CALCULATE_PA_ERROR_ITEM		(AGENT_START_PROCESS_PROPERTY->items+4)
#define AGENT_PLATESOLVER_START_RECALCULATE_PA_ERROR_ITEM	(AGENT_START_PROCESS_PROPERTY->items+5)

#define AGENT_ABORT_PROCESS_PROPERTY					(INDIGO_PLATESOLVER_DEVICE_PRIVATE_DATA->agent_abort_process_property)
#define AGENT_ABORT_PROCESS_ITEM      				(AGENT_ABORT_PROCESS_PROPERTY->items+0)

#define AGENT_PLATESOLVER_PA_STATE_PROPERTY				(INDIGO_PLATESOLVER_DEVICE_PRIVATE_DATA->polar_alignment_state_property)
#define AGENT_PLATESOLVER_PA_STATE_ITEM					(AGENT_PLATESOLVER_PA_STATE_PROPERTY->items+0)
#define AGENT_PLATESOLVER_PA_STATE_DEC_DRIFT_2_ITEM		(AGENT_PLATESOLVER_PA_STATE_PROPERTY->items+1)
#define AGENT_PLATESOLVER_PA_STATE_DEC_DRIFT_3_ITEM		(AGENT_PLATESOLVER_PA_STATE_PROPERTY->items+2)
#define AGENT_PLATESOLVER_PA_STATE_TARGET_RA_ITEM		(AGENT_PLATESOLVER_PA_STATE_PROPERTY->items+3)
#define AGENT_PLATESOLVER_PA_STATE_TARGET_DEC_ITEM		(AGENT_PLATESOLVER_PA_STATE_PROPERTY->items+4)
#define AGENT_PLATESOLVER_PA_STATE_CURRENT_RA_ITEM		(AGENT_PLATESOLVER_PA_STATE_PROPERTY->items+5)
#define AGENT_PLATESOLVER_PA_STATE_CURRENT_DEC_ITEM		(AGENT_PLATESOLVER_PA_STATE_PROPERTY->items+6)
#define AGENT_PLATESOLVER_PA_STATE_ALT_ERROR_ITEM		(AGENT_PLATESOLVER_PA_STATE_PROPERTY->items+7)
#define AGENT_PLATESOLVER_PA_STATE_AZ_ERROR_ITEM		(AGENT_PLATESOLVER_PA_STATE_PROPERTY->items+8)
#define AGENT_PLATESOLVER_PA_STATE_POLAR_ERROR_ITEM		(AGENT_PLATESOLVER_PA_STATE_PROPERTY->items+9)
#define AGENT_PLATESOLVER_PA_STATE_ALT_CORRECTION_UP_ITEM	(AGENT_PLATESOLVER_PA_STATE_PROPERTY->items+10)
#define AGENT_PLATESOLVER_PA_STATE_AZ_CORRECTION_CW_ITEM	(AGENT_PLATESOLVER_PA_STATE_PROPERTY->items+11)

#define AGENT_PLATESOLVER_EXPOSURE_SETTINGS_PROPERTY			(INDIGO_PLATESOLVER_DEVICE_PRIVATE_DATA->exposure_settings_property)
#define AGENT_PLATESOLVER_EXPOSURE_SETTINGS_EXPOSURE_ITEM		(AGENT_PLATESOLVER_EXPOSURE_SETTINGS_PROPERTY->items+0)

#define AGENT_PLATESOLVER_PA_SETTINGS_PROPERTY			(INDIGO_PLATESOLVER_DEVICE_PRIVATE_DATA->polar_alignment_settings_property)
#define AGENT_PLATESOLVER_PA_SETTINGS_EXPOSURE_ITEM				(AGENT_PLATESOLVER_PA_SETTINGS_PROPERTY->items+0)
#define AGENT_PLATESOLVER_PA_SETTINGS_HA_MOVE_ITEM		(AGENT_PLATESOLVER_PA_SETTINGS_PROPERTY->items+1)
#define AGENT_PLATESOLVER_PA_SETTINGS_COMPENSATE_REFRACTION_ITEM		(AGENT_PLATESOLVER_PA_SETTINGS_PROPERTY->items+2)

#define AGENT_PLATESOLVER_GOTO_SETTINGS_PROPERTY	(INDIGO_PLATESOLVER_DEVICE_PRIVATE_DATA->precise_goto_settings_property)
#define AGENT_PLATESOLVER_GOTO_SETTINGS_RA_ITEM		(AGENT_PLATESOLVER_GOTO_SETTINGS_PROPERTY->items+0)
#define AGENT_PLATESOLVER_GOTO_SETTINGS_DEC_ITEM	(AGENT_PLATESOLVER_GOTO_SETTINGS_PROPERTY->items+1)

#define AGENT_PLATESOLVER_MOUNT_SETTLE_TIME_PROPERTY	(INDIGO_PLATESOLVER_DEVICE_PRIVATE_DATA->mount_settle_time_property)
#define AGENT_PLATESOLVER_MOUNT_SETTLE_TIME_ITEM		(AGENT_PLATESOLVER_MOUNT_SETTLE_TIME_PROPERTY->items+0)

#define AGENT_PLATESOLVER_ABORT_PROPERTY			(INDIGO_PLATESOLVER_DEVICE_PRIVATE_DATA->abort_property)
#define AGENT_PLATESOLVER_ABORT_ITEM					(AGENT_PLATESOLVER_ABORT_PROPERTY->items+0)

#define AGENT_PLATESOLVER_SOLVE_IMAGES_PROPERTY			(INDIGO_PLATESOLVER_DEVICE_PRIVATE_DATA->solve_images_property)
#define AGENT_PLATESOLVER_SOLVE_IMAGES_ENABLED_ITEM		(AGENT_PLATESOLVER_SOLVE_IMAGES_PROPERTY->items+0)
#define AGENT_PLATESOLVER_SOLVE_IMAGES_DISABLED_ITEM	(AGENT_PLATESOLVER_SOLVE_IMAGES_PROPERTY->items+1)

#define AGENT_PLATESOLVER_IMAGE_PROPERTY			(INDIGO_PLATESOLVER_DEVICE_PRIVATE_DATA->image_property)
#define AGENT_PLATESOLVER_IMAGE_ITEM					(AGENT_PLATESOLVER_IMAGE_PROPERTY->items+0)

#define AGENT_PLATESOLVER_IMAGE_OUTPUT_PROPERTY	(INDIGO_PLATESOLVER_DEVICE_PRIVATE_DATA->image_output_property)
#define AGENT_PLATESOLVER_IMAGE_OUTPUT_ITEM			(AGENT_PLATESOLVER_IMAGE_OUTPUT_PROPERTY->items+0)

/** Plate solver  structure.
 */
typedef struct {
	indigo_device *device;
	void *image;
	long size;
	char format[INDIGO_NAME_SIZE];
	char image_url[INDIGO_VALUE_SIZE];
} indigo_platesolver_task;

/** Platesolver private data structure.
 */
typedef struct {
	indigo_property *use_index_property;
	indigo_property *hints_property;
	indigo_property *wcs_property;
	indigo_property *sync_mode_property;
	indigo_property *start_process_property;
	indigo_property *agent_abort_process_property;
	indigo_property *abort_property;
	indigo_property *image_property;
	indigo_property *image_output_property;
	indigo_property *exposure_settings_property;
	indigo_property *polar_alignment_state_property;
	indigo_property *polar_alignment_settings_property;
	indigo_property *precise_goto_settings_property;
	indigo_property *mount_settle_time_property;
	indigo_property *solve_images_property;
	indigo_property_state mount_process_state;
	indigo_spherical_point_t eq_coordinates;
	indigo_spherical_point_t eq_start_coordinates;
	indigo_spherical_point_t geo_coordinates;
	indigo_spherical_point_t pa_reference1;
	indigo_spherical_point_t pa_reference2;
	indigo_spherical_point_t pa_reference3;
	double pa_current_ra;
	double pa_current_dec;
	double pa_target_ra;
	double pa_target_dec;
	double pa_alt_error;
	double pa_az_error;
	double pa_initial_error;
	indigo_property_state imager_capture_state;
	indigo_property_state guider_process_state;
	void (*save_config)(indigo_device *);
	bool (*solve)(indigo_device *, void *image, unsigned long size);
	void (*abort)(indigo_device *);
	pthread_mutex_t mutex;
	double pixel_scale;
	bool failed;
	bool abort_process_requested;
	int saved_sync_mode;
} platesolver_private_data;

extern bool indigo_platesolver_validate_executable(const char *executable);
extern void indigo_platesolver_save_config(indigo_device *device);
extern void indigo_platesolver_sync(indigo_device *device);

/** Device attach callback function.
 */
extern indigo_result indigo_platesolver_device_attach(indigo_device *device, const char* driver_name, unsigned version, indigo_device_interface device_interface);
/** Enumerate properties callback function.
 */
extern indigo_result indigo_platesolver_enumerate_properties(indigo_device *device, indigo_client *client, indigo_property *property);
/** Change property callback function.
 */
extern indigo_result indigo_platesolver_change_property(indigo_device *device, indigo_client *client, indigo_property *property);
/** Detach callback function.
 */
extern indigo_result indigo_platesolver_device_detach(indigo_device *device);

#define indigo_platesolver_client_attach indigo_filter_client_attach
#define indigo_platesolver_delete_property indigo_filter_delete_property
#define indigo_platesolver_client_detach indigo_filter_client_detach

/** Client define property callback function.
 */
extern indigo_result indigo_platesolver_define_property(indigo_client *client, indigo_device *device, indigo_property *property, const char *message);

/** Client update property callback function.
 */
extern indigo_result indigo_platesolver_update_property(indigo_client *client, indigo_device *device, indigo_property *property, const char *message);

#ifdef __cplusplus
}
#endif

#endif /* indigo_platesolver_h */
