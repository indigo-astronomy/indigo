// Copyright (C) 2024 Rumen G. Bogdanovski
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
// 2.0 by Rumen G. Bogdanovski

/** INDIGO ASI rotator driver
 \file indigo_rotator_asi.c
 */

#define DRIVER_VERSION 0x0003
#define DRIVER_NAME "indigo_rotator_asi"

#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <math.h>
#include <assert.h>
#include <pthread.h>
#include <stdbool.h>
#include <sys/time.h>

#include <indigo/indigo_driver_xml.h>
#include "indigo_rotator_asi.h"

#if defined(INDIGO_FREEBSD)
#include <libusb.h>
#else
#include <libusb-1.0/libusb.h>
#endif

#include <CAA_API.h>

#define ASI_VENDOR_ID                   0x03c3
//#define CAA_PRODUCT_ID                  0x1f10

#define PRIVATE_DATA                    ((asi_private_data *)device->private_data)

#define CAA_BEEP_PROPERTY               (PRIVATE_DATA->beep_property)
#define CAA_BEEP_ON_ITEM                (CAA_BEEP_PROPERTY->items+0)
#define CAA_BEEP_OFF_ITEM               (CAA_BEEP_PROPERTY->items+1)
#define CAA_BEEP_PROPERTY_NAME          "CAA_BEEP_ON_MOVE"
#define CAA_BEEP_ON_ITEM_NAME           "ON"
#define CAA_BEEP_OFF_ITEM_NAME          "OFF"

#define CAA_CUSTOM_SUFFIX_PROPERTY     (PRIVATE_DATA->custom_suffix_property)
#define CAA_CUSTOM_SUFFIX_ITEM         (CAA_CUSTOM_SUFFIX_PROPERTY->items+0)
#define CAA_CUSTOM_SUFFIX_PROPERTY_NAME   "CAA_CUSTOM_SUFFIX"
#define CAA_CUSTOM_SUFFIX_NAME         "SUFFIX"

// gp_bits is used as boolean
#define is_connected                    gp_bits

typedef struct {
	int dev_id;
	CAA_INFO info;
	char model[64];
	char custom_suffix[9];
	float current_position, target_position, min_position, max_position;
	indigo_timer *rotator_timer, *temperature_timer;
	pthread_mutex_t usb_mutex;
	indigo_property *beep_property;
	indigo_property *custom_suffix_property;
} asi_private_data;

static int find_index_by_device_id(int id);

// -------------------------------------------------------------------------------- INDIGO rotator device implementation
static void rotator_timer_callback(indigo_device *device) {
	bool moving, moving_HC;

	pthread_mutex_lock(&PRIVATE_DATA->usb_mutex);
	int res = CAAIsMoving(PRIVATE_DATA->dev_id, &moving, &moving_HC);
	if (res != CAA_SUCCESS) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "CAAIsMoving(%d) = %d", PRIVATE_DATA->dev_id, res);
		ROTATOR_POSITION_PROPERTY->state = INDIGO_ALERT_STATE;
		ROTATOR_RELATIVE_MOVE_PROPERTY->state = INDIGO_ALERT_STATE;
	}

	res = CAAGetDegree(PRIVATE_DATA->dev_id, &(PRIVATE_DATA->current_position));
	if (res != CAA_SUCCESS) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "CAAGetDegree(%d) = %d", PRIVATE_DATA->dev_id, res);
		ROTATOR_POSITION_PROPERTY->state = INDIGO_ALERT_STATE;
		ROTATOR_RELATIVE_MOVE_PROPERTY->state = INDIGO_ALERT_STATE;
	}
	INDIGO_DRIVER_DEBUG(DRIVER_NAME, "CAAGetDegree(%d, -> %f) = %d", PRIVATE_DATA->dev_id, PRIVATE_DATA->current_position, res);

	pthread_mutex_unlock(&PRIVATE_DATA->usb_mutex);
	ROTATOR_POSITION_ITEM->number.value = PRIVATE_DATA->current_position;
	if ((!moving) || (PRIVATE_DATA->current_position == PRIVATE_DATA->target_position)) {
		ROTATOR_POSITION_PROPERTY->state = INDIGO_OK_STATE;
		ROTATOR_RELATIVE_MOVE_PROPERTY->state = INDIGO_OK_STATE;
	} else {
		indigo_reschedule_timer(device, 0.5, &(PRIVATE_DATA->rotator_timer));
	}
	indigo_update_property(device, ROTATOR_RELATIVE_MOVE_PROPERTY, NULL);
	indigo_update_property(device, ROTATOR_POSITION_PROPERTY, NULL);
}


static void temperature_timer_callback(indigo_device *device) {
	/*
	pthread_mutex_lock(&PRIVATE_DATA->usb_mutex);
	res = CAAGetTemp(PRIVATE_DATA->dev_id, &temp);
	pthread_mutex_unlock(&PRIVATE_DATA->usb_mutex);
	*/
	indigo_reschedule_timer(device, 2, &(PRIVATE_DATA->temperature_timer));
}

static indigo_result caa_enumerate_properties(indigo_device *device, indigo_client *client, indigo_property *property) {
	if (IS_CONNECTED) {
		indigo_define_matching_property(CAA_BEEP_PROPERTY);
		indigo_define_matching_property(CAA_CUSTOM_SUFFIX_PROPERTY);
	}
	return indigo_rotator_enumerate_properties(device, NULL, NULL);
}


static indigo_result rotator_attach(indigo_device *device) {
	assert(device != NULL);
	assert(PRIVATE_DATA != NULL);
	if (indigo_rotator_attach(device, DRIVER_NAME, DRIVER_VERSION) == INDIGO_OK) {
		pthread_mutex_init(&PRIVATE_DATA->usb_mutex, NULL);

		INFO_PROPERTY->count = 6;
		indigo_copy_value(INFO_DEVICE_MODEL_ITEM->text.value, PRIVATE_DATA->model);
		char *sdk_version = CAAGetSDKVersion();
		indigo_copy_value(INFO_DEVICE_FW_REVISION_ITEM->text.value, sdk_version);
		indigo_copy_value(INFO_DEVICE_FW_REVISION_ITEM->label, "SDK version");


		ROTATOR_LIMITS_PROPERTY->hidden = false;
		ROTATOR_LIMITS_MAX_POSITION_ITEM->number.min = 0;
		ROTATOR_LIMITS_MAX_POSITION_ITEM->number.value =
		ROTATOR_LIMITS_MAX_POSITION_ITEM->number.target = 360;
		ROTATOR_LIMITS_MAX_POSITION_ITEM->number.max = 480;
		ROTATOR_LIMITS_MIN_POSITION_ITEM->number.min = 0;
		ROTATOR_LIMITS_MIN_POSITION_ITEM->number.value =
		ROTATOR_LIMITS_MIN_POSITION_ITEM->number.target = 0;
		ROTATOR_LIMITS_MIN_POSITION_ITEM->number.max = 480;
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "\'%s\' MaxStep = %d",device->name ,PRIVATE_DATA->info.MaxStep);

		ROTATOR_BACKLASH_PROPERTY->hidden = true;

		ROTATOR_POSITION_ITEM->number.min = 0;
		ROTATOR_POSITION_ITEM->number.step = 1;
		ROTATOR_POSITION_ITEM->number.max = 480;

		ROTATOR_RELATIVE_MOVE_ITEM->number.min = -120;
		ROTATOR_RELATIVE_MOVE_ITEM->number.step = 1;
		ROTATOR_RELATIVE_MOVE_ITEM->number.max = 120;
		ROTATOR_RELATIVE_MOVE_PROPERTY->hidden = false;

		ROTATOR_ON_POSITION_SET_PROPERTY->hidden = false;
		ROTATOR_DIRECTION_PROPERTY->hidden = false;
		// -------------------------------------------------------------------------- BEEP_PROPERTY
		CAA_BEEP_PROPERTY = indigo_init_switch_property(NULL, device->name, CAA_BEEP_PROPERTY_NAME, ROTATOR_ADVANCED_GROUP, "Beep on move", INDIGO_OK_STATE, INDIGO_RW_PERM, INDIGO_ONE_OF_MANY_RULE, 2);
		if (CAA_BEEP_PROPERTY == NULL)
			return INDIGO_FAILED;

		indigo_init_switch_item(CAA_BEEP_ON_ITEM, CAA_BEEP_ON_ITEM_NAME, "On", false);
		indigo_init_switch_item(CAA_BEEP_OFF_ITEM, CAA_BEEP_OFF_ITEM_NAME, "Off", true);
		// --------------------------------------------------------------------------------- CAA_CUSTOM_SUFFIX
		CAA_CUSTOM_SUFFIX_PROPERTY = indigo_init_text_property(NULL, device->name, "CAA_CUSTOM_SUFFIX", ROTATOR_ADVANCED_GROUP, "Device name custom suffix", INDIGO_OK_STATE, INDIGO_RW_PERM, 1);
		if (CAA_CUSTOM_SUFFIX_PROPERTY == NULL)
			return INDIGO_FAILED;
		indigo_init_text_item(CAA_CUSTOM_SUFFIX_ITEM, CAA_CUSTOM_SUFFIX_NAME, "Suffix", PRIVATE_DATA->custom_suffix);
		// --------------------------------------------------------------------------
		return caa_enumerate_properties(device, NULL, NULL);
	}
	return INDIGO_FAILED;
}

static void rotator_connect_callback(indigo_device *device) {
	int index = 0;
	CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
	if (CONNECTION_CONNECTED_ITEM->sw.value) {
		index = find_index_by_device_id(PRIVATE_DATA->dev_id);
		if (index >= 0) {
			if (!device->is_connected) {
				pthread_mutex_lock(&PRIVATE_DATA->usb_mutex);
				if (indigo_try_global_lock(device) != INDIGO_OK) {
					pthread_mutex_unlock(&PRIVATE_DATA->usb_mutex);
					INDIGO_DRIVER_ERROR(DRIVER_NAME, "indigo_try_global_lock(): failed to get lock.");
					CONNECTION_PROPERTY->state = INDIGO_ALERT_STATE;
					indigo_set_switch(CONNECTION_PROPERTY, CONNECTION_DISCONNECTED_ITEM, true);
					indigo_update_property(device, CONNECTION_PROPERTY, NULL);
				} else {
					CAAGetID(index, &(PRIVATE_DATA->dev_id));
					int res = CAAOpen(PRIVATE_DATA->dev_id);
					pthread_mutex_unlock(&PRIVATE_DATA->usb_mutex);
					if (!res) {
						INDIGO_DRIVER_DEBUG(DRIVER_NAME, "CAAOpen(%d) = %d", PRIVATE_DATA->dev_id, res);
						pthread_mutex_lock(&PRIVATE_DATA->usb_mutex);

						// there is no such call in the SDK!!!
						// res = CAAMinDegree(PRIVATE_DATA->dev_id, &(PRIVATE_DATA->min_position));
						PRIVATE_DATA->min_position = 0;
						if (res != CAA_SUCCESS) {
							INDIGO_DRIVER_ERROR(DRIVER_NAME, "CAAMinDegree(%d) = %d", PRIVATE_DATA->dev_id, res);
						}
						ROTATOR_LIMITS_MIN_POSITION_ITEM->number.value = ROTATOR_LIMITS_MIN_POSITION_ITEM->number.target =
						ROTATOR_LIMITS_MIN_POSITION_ITEM->number.min = ROTATOR_LIMITS_MIN_POSITION_ITEM->number.max = (double)PRIVATE_DATA->min_position;

						res = CAAGetMaxDegree(PRIVATE_DATA->dev_id, &(PRIVATE_DATA->max_position));
						if (res != CAA_SUCCESS) {
							INDIGO_DRIVER_ERROR(DRIVER_NAME, "CAAGetMaxDegree(%d) = %d", PRIVATE_DATA->dev_id, res);
						}
						ROTATOR_LIMITS_MAX_POSITION_ITEM->number.value = ROTATOR_LIMITS_MAX_POSITION_ITEM->number.target = (double)PRIVATE_DATA->max_position;

						res = CAAGetDegree(PRIVATE_DATA->dev_id, &(PRIVATE_DATA->target_position));
						if (res != CAA_SUCCESS) {
							INDIGO_DRIVER_ERROR(DRIVER_NAME, "CAAGetDegree(%d, -> %f) = %d", PRIVATE_DATA->dev_id, PRIVATE_DATA->target_position, res);
						}
						ROTATOR_POSITION_ITEM->number.value = PRIVATE_DATA->target_position;

						res = CAAGetReverse(PRIVATE_DATA->dev_id, &(ROTATOR_DIRECTION_REVERSED_ITEM->sw.value));
						if (res != CAA_SUCCESS) {
							INDIGO_DRIVER_ERROR(DRIVER_NAME, "CAAGetReverse(%d, -> %d) = %d", PRIVATE_DATA->dev_id, ROTATOR_DIRECTION_REVERSED_ITEM->sw.value, res);
						}
						ROTATOR_DIRECTION_NORMAL_ITEM->sw.value = !ROTATOR_DIRECTION_REVERSED_ITEM->sw.value;

						res = CAAGetBeep(PRIVATE_DATA->dev_id, &(CAA_BEEP_ON_ITEM->sw.value));
						if (res != CAA_SUCCESS) {
							INDIGO_DRIVER_ERROR(DRIVER_NAME, "CAAGetBeep(%d, -> %d) = %d", PRIVATE_DATA->dev_id, CAA_BEEP_ON_ITEM->sw.value, res);
						}
						CAA_BEEP_OFF_ITEM->sw.value = !CAA_BEEP_ON_ITEM->sw.value;
						pthread_mutex_unlock(&PRIVATE_DATA->usb_mutex);

						CONNECTION_PROPERTY->state = INDIGO_OK_STATE;

						indigo_define_property(device, CAA_BEEP_PROPERTY, NULL);
						indigo_define_property(device, CAA_CUSTOM_SUFFIX_PROPERTY, NULL);

						device->is_connected = true;
						indigo_set_timer(device, 0.5, rotator_timer_callback, &PRIVATE_DATA->rotator_timer);
						indigo_set_timer(device, 0.1, temperature_timer_callback, &PRIVATE_DATA->temperature_timer);
					} else {
						INDIGO_DRIVER_ERROR(DRIVER_NAME, "CAAOpen(%d) = %d", index, res);
						CONNECTION_PROPERTY->state = INDIGO_ALERT_STATE;
						indigo_set_switch(CONNECTION_PROPERTY, CONNECTION_DISCONNECTED_ITEM, true);
						indigo_update_property(device, CONNECTION_PROPERTY, NULL);
					}
				}
			}
		}
	} else {
		if (device->is_connected) {
			indigo_cancel_timer_sync(device, &PRIVATE_DATA->rotator_timer);
			indigo_cancel_timer_sync(device, &PRIVATE_DATA->temperature_timer);
			indigo_delete_property(device, CAA_BEEP_PROPERTY, NULL);
			indigo_delete_property(device, CAA_CUSTOM_SUFFIX_PROPERTY, NULL);
			pthread_mutex_lock(&PRIVATE_DATA->usb_mutex);
			int res = CAAStop(PRIVATE_DATA->dev_id);
			res = CAAClose(PRIVATE_DATA->dev_id);
			if (res != CAA_SUCCESS) {
				INDIGO_DRIVER_ERROR(DRIVER_NAME, "CAAClose(%d) = %d", PRIVATE_DATA->dev_id, res);
			} else {
				INDIGO_DRIVER_DEBUG(DRIVER_NAME, "CAAClose(%d) = %d", PRIVATE_DATA->dev_id, res);
			}
			indigo_global_unlock(device);
			pthread_mutex_unlock(&PRIVATE_DATA->usb_mutex);
			device->is_connected = false;
			CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
		}
	}
	indigo_rotator_change_property(device, NULL, CONNECTION_PROPERTY);
}

static indigo_result rotator_change_property(indigo_device *device, indigo_client *client, indigo_property *property) {
	assert(device != NULL);
	assert(DEVICE_CONTEXT != NULL);
	assert(property != NULL);
	if (indigo_property_match_changeable(CONNECTION_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- CONNECTION
		if (indigo_ignore_connection_change(device, property))
			return INDIGO_OK;
		indigo_property_copy_values(CONNECTION_PROPERTY, property, false);
		CONNECTION_PROPERTY->state = INDIGO_BUSY_STATE;
		indigo_update_property(device, CONNECTION_PROPERTY, NULL);
		indigo_set_timer(device, 0, rotator_connect_callback, NULL);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(ROTATOR_DIRECTION_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- ROTATOR_REVERSE_MOTION
		indigo_property_copy_values(ROTATOR_DIRECTION_PROPERTY, property, false);
		ROTATOR_DIRECTION_PROPERTY->state = INDIGO_OK_STATE;
		pthread_mutex_lock(&PRIVATE_DATA->usb_mutex);
		int res = CAASetReverse(PRIVATE_DATA->dev_id, ROTATOR_DIRECTION_REVERSED_ITEM->sw.value);
		if (res != CAA_SUCCESS) {
			INDIGO_DRIVER_ERROR(DRIVER_NAME, "CAASetReverse(%d, %d) = %d", PRIVATE_DATA->dev_id, ROTATOR_DIRECTION_REVERSED_ITEM->sw.value, res);
			ROTATOR_DIRECTION_PROPERTY->state = INDIGO_ALERT_STATE;
		}
		pthread_mutex_unlock(&PRIVATE_DATA->usb_mutex);
		indigo_update_property(device, ROTATOR_DIRECTION_PROPERTY, NULL);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(ROTATOR_POSITION_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- ROTATOR_POSITION
		indigo_property_copy_values(ROTATOR_POSITION_PROPERTY, property, false);
		if (ROTATOR_POSITION_PROPERTY->state == INDIGO_BUSY_STATE) {
			return INDIGO_OK;
		}
		if (ROTATOR_POSITION_ITEM->number.target == PRIVATE_DATA->current_position) {
			ROTATOR_POSITION_PROPERTY->state = INDIGO_OK_STATE;
			ROTATOR_RELATIVE_MOVE_PROPERTY->state = INDIGO_OK_STATE;
			indigo_update_property(device, ROTATOR_RELATIVE_MOVE_PROPERTY, NULL);
			indigo_update_property(device, ROTATOR_POSITION_PROPERTY, NULL);
		} else {
			ROTATOR_POSITION_PROPERTY->state = INDIGO_BUSY_STATE;
			ROTATOR_RELATIVE_MOVE_PROPERTY->state = INDIGO_BUSY_STATE;
			PRIVATE_DATA->target_position = ROTATOR_POSITION_ITEM->number.target;
			ROTATOR_POSITION_ITEM->number.value = PRIVATE_DATA->current_position;
			indigo_update_property(device, ROTATOR_RELATIVE_MOVE_PROPERTY, NULL);
			indigo_update_property(device, ROTATOR_POSITION_PROPERTY, NULL);
			if (ROTATOR_ON_POSITION_SET_GOTO_ITEM->sw.value) { /* GOTO POSITION */
				pthread_mutex_lock(&PRIVATE_DATA->usb_mutex);
				int res = CAAMoveTo(PRIVATE_DATA->dev_id, PRIVATE_DATA->target_position);
				if (res != CAA_SUCCESS) {
					INDIGO_DRIVER_ERROR(DRIVER_NAME, "CAAMoveTo(%d, %d) = %d", PRIVATE_DATA->dev_id, PRIVATE_DATA->target_position, res);
				}
				pthread_mutex_unlock(&PRIVATE_DATA->usb_mutex);
				indigo_set_timer(device, 0.5, rotator_timer_callback, &PRIVATE_DATA->rotator_timer);
			} else { /* RESET CURRENT POSITION - SYNC */
				ROTATOR_POSITION_PROPERTY->state = INDIGO_OK_STATE;
				ROTATOR_RELATIVE_MOVE_PROPERTY->state = INDIGO_OK_STATE;
				pthread_mutex_lock(&PRIVATE_DATA->usb_mutex);
				int res = CAACurDegree(PRIVATE_DATA->dev_id, PRIVATE_DATA->target_position);
				if (res != CAA_SUCCESS) {
					INDIGO_DRIVER_ERROR(DRIVER_NAME, "CAACurDegree(%d, %d) = %d", PRIVATE_DATA->dev_id, PRIVATE_DATA->target_position, res);
					ROTATOR_POSITION_PROPERTY->state = INDIGO_ALERT_STATE;
					ROTATOR_RELATIVE_MOVE_PROPERTY->state = INDIGO_ALERT_STATE;
				}
				res = CAAGetDegree(PRIVATE_DATA->dev_id, &PRIVATE_DATA->current_position);
				ROTATOR_POSITION_ITEM->number.value = PRIVATE_DATA->current_position;
				if (res != CAA_SUCCESS) {
					INDIGO_DRIVER_ERROR(DRIVER_NAME, "CAAGetDegree(%d) = %d", PRIVATE_DATA->dev_id, res);
					ROTATOR_POSITION_PROPERTY->state = INDIGO_ALERT_STATE;
					ROTATOR_RELATIVE_MOVE_PROPERTY->state = INDIGO_ALERT_STATE;
				}
				pthread_mutex_unlock(&PRIVATE_DATA->usb_mutex);
				indigo_update_property(device, ROTATOR_RELATIVE_MOVE_PROPERTY, NULL);
				indigo_update_property(device, ROTATOR_POSITION_PROPERTY, NULL);
			}
		}
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(ROTATOR_LIMITS_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- ROTATOR_LIMITS
		indigo_property_copy_values(ROTATOR_LIMITS_PROPERTY, property, false);
		ROTATOR_LIMITS_PROPERTY->state = INDIGO_OK_STATE;
		int res = CAASetMaxDegree(PRIVATE_DATA->dev_id, PRIVATE_DATA->max_position);
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "CAASetMaxDegree(%d, -> %d) = %d", PRIVATE_DATA->dev_id, PRIVATE_DATA->max_position, res);
		if (res != CAA_SUCCESS) {
			INDIGO_DRIVER_ERROR(DRIVER_NAME, "CAASetMaxDegree(%d) = %d", PRIVATE_DATA->dev_id, res);
			ROTATOR_LIMITS_PROPERTY->state = INDIGO_ALERT_STATE;
		}
		res = CAAGetMaxDegree(PRIVATE_DATA->dev_id, &(PRIVATE_DATA->max_position));
		if (res != CAA_SUCCESS) {
			INDIGO_DRIVER_ERROR(DRIVER_NAME, "CAAGetMaxDegree(%d) = %d", PRIVATE_DATA->dev_id, res);
		}
		ROTATOR_LIMITS_MAX_POSITION_ITEM->number.value = (double)PRIVATE_DATA->max_position;
		pthread_mutex_unlock(&PRIVATE_DATA->usb_mutex);
		indigo_update_property(device, ROTATOR_LIMITS_PROPERTY, NULL);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(ROTATOR_RELATIVE_MOVE_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- ROTATOR_STEPS
		indigo_property_copy_values(ROTATOR_RELATIVE_MOVE_PROPERTY, property, false);
		if (ROTATOR_RELATIVE_MOVE_PROPERTY->state == INDIGO_BUSY_STATE) {
			return INDIGO_OK;
		}
		if (fabs(ROTATOR_RELATIVE_MOVE_ITEM->number.target) > ROTATOR_RELATIVE_MOVE_ITEM->number.max) {
			ROTATOR_POSITION_PROPERTY->state = INDIGO_ALERT_STATE;
			ROTATOR_RELATIVE_MOVE_PROPERTY->state = INDIGO_ALERT_STATE;
			indigo_update_property(device, ROTATOR_RELATIVE_MOVE_PROPERTY, NULL);
			indigo_update_property(device, ROTATOR_POSITION_PROPERTY, NULL);
		} else {
			ROTATOR_RELATIVE_MOVE_PROPERTY->state = INDIGO_BUSY_STATE;
			ROTATOR_POSITION_PROPERTY->state = INDIGO_BUSY_STATE;
			indigo_update_property(device, ROTATOR_RELATIVE_MOVE_PROPERTY, NULL);
			indigo_update_property(device, ROTATOR_POSITION_PROPERTY, NULL);
			pthread_mutex_lock(&PRIVATE_DATA->usb_mutex);
			int res = CAAGetDegree(PRIVATE_DATA->dev_id, &PRIVATE_DATA->current_position);
			if (res != CAA_SUCCESS) {
				INDIGO_DRIVER_ERROR(DRIVER_NAME, "CAAGetDegree(%d) = %d", PRIVATE_DATA->dev_id, res);
			}

			PRIVATE_DATA->target_position = PRIVATE_DATA->current_position + ROTATOR_RELATIVE_MOVE_ITEM->number.target;

			ROTATOR_POSITION_ITEM->number.value = PRIVATE_DATA->current_position;
			res = CAAMoveTo(PRIVATE_DATA->dev_id, PRIVATE_DATA->target_position);
			if (res != CAA_SUCCESS) {
				INDIGO_DRIVER_ERROR(DRIVER_NAME, "CAAMoveTo(%d, %d) = %d", PRIVATE_DATA->dev_id, PRIVATE_DATA->target_position, res);
			}
			pthread_mutex_unlock(&PRIVATE_DATA->usb_mutex);
			indigo_set_timer(device, 0.5, rotator_timer_callback, &PRIVATE_DATA->rotator_timer);
		}
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(ROTATOR_ABORT_MOTION_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- ROTATOR_ABORT_MOTION
		indigo_property_copy_values(ROTATOR_ABORT_MOTION_PROPERTY, property, false);
		ROTATOR_RELATIVE_MOVE_PROPERTY->state = INDIGO_OK_STATE;
		ROTATOR_POSITION_PROPERTY->state = INDIGO_OK_STATE;
		ROTATOR_ABORT_MOTION_PROPERTY->state = INDIGO_OK_STATE;
		indigo_cancel_timer(device, &PRIVATE_DATA->rotator_timer);
		pthread_mutex_lock(&PRIVATE_DATA->usb_mutex);
		int res = CAAStop(PRIVATE_DATA->dev_id);
		if (res != CAA_SUCCESS) {
			INDIGO_DRIVER_ERROR(DRIVER_NAME, "CAAStop(%d) = %d", PRIVATE_DATA->dev_id, res);
			ROTATOR_ABORT_MOTION_PROPERTY->state = INDIGO_ALERT_STATE;
		}
		res = CAAGetDegree(PRIVATE_DATA->dev_id, &PRIVATE_DATA->current_position);
		if (res != CAA_SUCCESS) {
			INDIGO_DRIVER_ERROR(DRIVER_NAME, "CAAGetDegree(%d) = %d", PRIVATE_DATA->dev_id, res);
			ROTATOR_ABORT_MOTION_PROPERTY->state = INDIGO_ALERT_STATE;
		}
		pthread_mutex_unlock(&PRIVATE_DATA->usb_mutex);
		ROTATOR_POSITION_ITEM->number.value = PRIVATE_DATA->current_position;
		ROTATOR_ABORT_MOTION_ITEM->sw.value = false;
		indigo_update_property(device, ROTATOR_POSITION_PROPERTY, NULL);
		indigo_update_property(device, ROTATOR_RELATIVE_MOVE_PROPERTY, NULL);
		indigo_update_property(device, ROTATOR_ABORT_MOTION_PROPERTY, NULL);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(CAA_BEEP_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- CAA_BEEP_PROPERTY
		indigo_property_copy_values(CAA_BEEP_PROPERTY, property, false);
		CAA_BEEP_PROPERTY->state = INDIGO_OK_STATE;
		pthread_mutex_lock(&PRIVATE_DATA->usb_mutex);
		int res = CAASetBeep(PRIVATE_DATA->dev_id, CAA_BEEP_ON_ITEM->sw.value);
		if (res != CAA_SUCCESS) {
			INDIGO_DRIVER_ERROR(DRIVER_NAME, "CAASetBeep(%d, %d) = %d", PRIVATE_DATA->dev_id, CAA_BEEP_ON_ITEM->sw.value, res);
			CAA_BEEP_PROPERTY->state = INDIGO_ALERT_STATE;
		}
		pthread_mutex_unlock(&PRIVATE_DATA->usb_mutex);
		indigo_update_property(device, CAA_BEEP_PROPERTY, NULL);
		return INDIGO_OK;
		// ------------------------------------------------------------------------------- CAA_CUSTOM_SUFFIX
	} else if (indigo_property_match_changeable(CAA_CUSTOM_SUFFIX_PROPERTY, property)) {
		CAA_CUSTOM_SUFFIX_PROPERTY->state = INDIGO_OK_STATE;
		indigo_property_copy_values(CAA_CUSTOM_SUFFIX_PROPERTY, property, false);
		if (strlen(CAA_CUSTOM_SUFFIX_ITEM->text.value) > 8) {
			CAA_CUSTOM_SUFFIX_PROPERTY->state = INDIGO_ALERT_STATE;
			indigo_update_property(device, CAA_CUSTOM_SUFFIX_PROPERTY, "Custom suffix too long");
			return INDIGO_OK;
		}
		pthread_mutex_lock(&PRIVATE_DATA->usb_mutex);
		CAA_ID caa_id = {0};
		memcpy(caa_id.id, CAA_CUSTOM_SUFFIX_ITEM->text.value, 8);
		memcpy(PRIVATE_DATA->custom_suffix, CAA_CUSTOM_SUFFIX_ITEM->text.value, sizeof(PRIVATE_DATA->custom_suffix));
		int res = CAASetID(PRIVATE_DATA->dev_id, caa_id);
		pthread_mutex_unlock(&PRIVATE_DATA->usb_mutex);
		if (res) {
			INDIGO_DRIVER_ERROR(DRIVER_NAME, "CAASetID(%d, \"%s\") = %d", PRIVATE_DATA->dev_id, CAA_CUSTOM_SUFFIX_ITEM->text.value, res);
			CAA_CUSTOM_SUFFIX_PROPERTY->state = INDIGO_ALERT_STATE;
			indigo_update_property(device, CAA_CUSTOM_SUFFIX_PROPERTY, NULL);
		} else {
			INDIGO_DRIVER_ERROR(DRIVER_NAME, "CAASetID(%d, \"%s\") = %d", PRIVATE_DATA->dev_id, CAA_CUSTOM_SUFFIX_ITEM->text.value, res);
			CAA_CUSTOM_SUFFIX_PROPERTY->state = INDIGO_OK_STATE;
			if (strlen(CAA_CUSTOM_SUFFIX_ITEM->text.value) > 0) {
				indigo_update_property(device, CAA_CUSTOM_SUFFIX_PROPERTY, "Rotator name suffix '#%s' will be used on replug", CAA_CUSTOM_SUFFIX_ITEM->text.value);
			} else {
				indigo_update_property(device, CAA_CUSTOM_SUFFIX_PROPERTY, "Rotator name suffix cleared, will be used on replug");
			}
		}
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(CONFIG_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- CONFIG
		if (indigo_switch_match(CONFIG_SAVE_ITEM, property)) {
			indigo_save_property(device, NULL, CAA_BEEP_PROPERTY);
		}
		// --------------------------------------------------------------------------------
	}
	return indigo_rotator_change_property(device, client, property);
}


static indigo_result rotator_detach(indigo_device *device) {
	assert(device != NULL);
	if (IS_CONNECTED) {
		indigo_set_switch(CONNECTION_PROPERTY, CONNECTION_DISCONNECTED_ITEM, true);
		rotator_connect_callback(device);
	}
	indigo_release_property(CAA_BEEP_PROPERTY);
	indigo_release_property(CAA_CUSTOM_SUFFIX_PROPERTY);
	INDIGO_DEVICE_DETACH_LOG(DRIVER_NAME, device->name);
	return indigo_rotator_detach(device);
}

// -------------------------------------------------------------------------------- hot-plug support

#define MAX_DEVICES                   10
#define NO_DEVICE                 (-1000)


static int caa_products[100];
static int caa_id_count = 0;


static indigo_device *devices[MAX_DEVICES] = {NULL};
static bool connected_ids[CAA_ID_MAX];

static int find_index_by_device_id(int id) {
	int count = CAAGetNum();
	INDIGO_DRIVER_DEBUG(DRIVER_NAME, "CAAGetNum() = %d", count);
	int cur_id;
	for (int index = 0; index < count; index++) {
		int res = CAAGetID(index, &cur_id);
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "CAAGetID(%d, -> %d) = %d", index, cur_id, res);
		if (res == CAA_SUCCESS && cur_id == id) {
			return index;
		}
	}
	return -1;
}


static int find_plugged_device_id() {
	int id = NO_DEVICE, new_id = NO_DEVICE;
	int count = CAAGetNum();
	INDIGO_DRIVER_DEBUG(DRIVER_NAME, "CAAGetNum() = %d", count);
	for (int index = 0; index < count; index++) {
		int res = CAAGetID(index, &id);
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "CAAGetID(%d, -> %d) = %d", index, id, res);
		if (res == CAA_SUCCESS && !connected_ids[id]) {
			new_id = id;
			connected_ids[id] = true;
			break;
		}
	}
	return new_id;
}


static int find_available_device_slot() {
	for(int slot = 0; slot < MAX_DEVICES; slot++) {
		if (devices[slot] == NULL) return slot;
	}
	return -1;
}


static int find_device_slot(int id) {
	for(int slot = 0; slot < MAX_DEVICES; slot++) {
		indigo_device *device = devices[slot];
		if (device == NULL) continue;
		if (PRIVATE_DATA->dev_id == id) return slot;
	}
	return -1;
}


static int find_unplugged_device_id() {
	bool dev_tmp[CAA_ID_MAX] = { false };
	int id = -1;
	int count = CAAGetNum();
	INDIGO_DRIVER_DEBUG(DRIVER_NAME, "CAAGetNum() = %d", count);
	for (int index = 0; index < count; index++) {
		int res = CAAGetID(index, &id);
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "CAAGetID(%d, -> %d) = %d", index, id, res);
		if (res == CAA_SUCCESS)
			dev_tmp[id] = true;
	}
	id = -1;
	for (int index = 0; index < CAA_ID_MAX; index++) {
		if (connected_ids[index] && !dev_tmp[index]) {
			id = index;
			connected_ids[id] = false;
			break;
		}
	}
	return id;
}


static void split_device_name(const char *fill_device_name, char *device_name, char *suffix) {
	if (fill_device_name == NULL || device_name == NULL || suffix == NULL) {
		return;
	}

	char name_buf[64];
	strncpy(name_buf, fill_device_name, sizeof(name_buf));
	char *suffix_start = strchr(name_buf, '(');
	char *suffix_end = strrchr(name_buf, ')');

	if (suffix_start == NULL || suffix_end == NULL) {
		strncpy(device_name, name_buf, 64);
		suffix[0] = '\0';
		return;
	}
	suffix_start[0] = '\0';
	suffix_end[0] = '\0';
	suffix_start++;

	strncpy(device_name, name_buf, 64);
	strncpy(suffix, suffix_start, 9);
}


static void process_plug_event(indigo_device *unused) {
	CAA_INFO info;
	static indigo_device rotator_template = INDIGO_DEVICE_INITIALIZER(
		"",
		rotator_attach,
		caa_enumerate_properties,
		rotator_change_property,
		NULL,
		rotator_detach
		);
	pthread_mutex_lock(&indigo_device_enumeration_mutex);
	int slot = find_available_device_slot();
	if (slot < 0) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "No device slots available.");
		pthread_mutex_unlock(&indigo_device_enumeration_mutex);
		return;
	}
	int id = find_plugged_device_id();
	if (id == NO_DEVICE) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "No plugged device found.");
		pthread_mutex_unlock(&indigo_device_enumeration_mutex);
		return;
	}
	int res = CAAOpen(id);
	if (res) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "CAAOpen(%d}) = %d", id, res);
		pthread_mutex_unlock(&indigo_device_enumeration_mutex);
		return;
	} else {
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "CAAOpen(%d}) = %d", id, res);
	}
	while (true) {
		res = CAAGetProperty(id, &info);
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "CAAGetProperty(%d, -> { %d, '%s', %d }) = %d", id, info.ID, info.Name, info.MaxStep, res);
		if (res == CAA_SUCCESS) {
			CAAClose(id);
			break;
		}
		if (res != CAA_ERROR_MOVING) {
			CAAClose(id);
			pthread_mutex_unlock(&indigo_device_enumeration_mutex);
			return;
		}
		  indigo_usleep(ONE_SECOND_DELAY);
	}
	indigo_device *device = indigo_safe_malloc_copy(sizeof(indigo_device), &rotator_template);
	char name[64] = {0};
	char suffix[9] = {0};
	char device_name[64] = {0};
	split_device_name(info.Name, name, suffix);
	if (suffix[0] != '\0') {
		sprintf(device_name, "%s #%s", name, suffix);
	} else {
		sprintf(device_name, "%s", name);
	}
	sprintf(device->name, "%s", device_name);
	indigo_make_name_unique(device->name, "%d", id);

	INDIGO_DEVICE_ATTACH_LOG(DRIVER_NAME, device->name);
	asi_private_data *private_data = indigo_safe_malloc(sizeof(asi_private_data));
	private_data->dev_id = id;
	private_data->info = info;
	strncpy(private_data->custom_suffix, suffix, 9);
	strncpy(private_data->model, name, 64);
	device->private_data = private_data;
	indigo_attach_device(device);
	devices[slot]=device;
	pthread_mutex_unlock(&indigo_device_enumeration_mutex);
}

static void process_unplug_event(indigo_device *unused) {
	int slot, id;
	bool removed = false;
	pthread_mutex_lock(&indigo_device_enumeration_mutex);
	while ((id = find_unplugged_device_id()) != -1) {
		slot = find_device_slot(id);
		if (slot < 0) continue;
		indigo_device **device = &devices[slot];
		if (*device == NULL) {
			pthread_mutex_unlock(&indigo_device_enumeration_mutex);
			return;
		}
		indigo_detach_device(*device);
		free((*device)->private_data);
		free(*device);
		*device = NULL;
		removed = true;
	}
	if (!removed) {
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "No ZWO CAA device unplugged (maybe ASI Camera)!");
	}
	pthread_mutex_unlock(&indigo_device_enumeration_mutex);
}

static int hotplug_callback(libusb_context *ctx, libusb_device *dev, libusb_hotplug_event event, void *user_data) {
	struct libusb_device_descriptor descriptor;
	switch (event) {
		case LIBUSB_HOTPLUG_EVENT_DEVICE_ARRIVED: {
			libusb_get_device_descriptor(dev, &descriptor);
			INDIGO_DRIVER_DEBUG(DRIVER_NAME, "Device plugged has PID:VID = %x:%x", descriptor.idVendor, descriptor.idProduct);
			for (int i = 0; i < caa_id_count; i++) {
				if (descriptor.idVendor != ASI_VENDOR_ID || caa_products[i] != descriptor.idProduct) {
					INDIGO_DRIVER_DEBUG(DRIVER_NAME, "No ZWO CAA device plugged (maybe ASI Camera)!");
					continue;
				}
				indigo_set_timer(NULL, 0.5, process_plug_event, NULL);
			}
			break;
		}
		case LIBUSB_HOTPLUG_EVENT_DEVICE_LEFT: {
			indigo_set_timer(NULL, 0.5, process_unplug_event, NULL);
			break;
		}
	}
	return 0;
};


static void remove_all_devices() {
	for (int index = 0; index < MAX_DEVICES; index++) {
		indigo_device **device = &devices[index];
		if (*device == NULL)
			continue;
		indigo_detach_device(*device);
		free((*device)->private_data);
		free(*device);
		*device = NULL;
	}
	for (int index = 0; index < CAA_ID_MAX; index++)
		connected_ids[index] = false;
}


static libusb_hotplug_callback_handle callback_handle;

indigo_result indigo_rotator_asi(indigo_driver_action action, indigo_driver_info *info) {
	static indigo_driver_action last_action = INDIGO_DRIVER_SHUTDOWN;

	SET_DRIVER_INFO(info, "ZWO CAA Rotator", __FUNCTION__, DRIVER_VERSION, true, last_action);

	if (action == last_action)
		return INDIGO_OK;

	switch (action) {
	case INDIGO_DRIVER_INIT:
		last_action = action;

		const char *sdk_version = CAAGetSDKVersion();
		INDIGO_DRIVER_LOG(DRIVER_NAME, "CAA SDK v. %s ", sdk_version);

		for(int index = 0; index < CAA_ID_MAX; index++)
			connected_ids[index] = false;
		caa_id_count = CAAGetProductIDs(caa_products);
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "CAAGetProductIDs(-> [ %d, %d, ... ]) = %d", caa_products[0], caa_products[1], caa_id_count);
		//caa_products[0] = CAA_PRODUCT_ID;
		//caa_id_count = 1;
		indigo_start_usb_event_handler();
		int rc = libusb_hotplug_register_callback(NULL, LIBUSB_HOTPLUG_EVENT_DEVICE_ARRIVED | LIBUSB_HOTPLUG_EVENT_DEVICE_LEFT, LIBUSB_HOTPLUG_ENUMERATE, ASI_VENDOR_ID, LIBUSB_HOTPLUG_MATCH_ANY, LIBUSB_HOTPLUG_MATCH_ANY, hotplug_callback, NULL, &callback_handle);
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "libusb_hotplug_register_callback ->  %s", rc < 0 ? libusb_error_name(rc) : "OK");
		return rc >= 0 ? INDIGO_OK : INDIGO_FAILED;

	case INDIGO_DRIVER_SHUTDOWN:
		for (int i = 0; i < MAX_DEVICES; i++)
			VERIFY_NOT_CONNECTED(devices[i]);
		last_action = action;
		libusb_hotplug_deregister_callback(NULL, callback_handle);
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "libusb_hotplug_deregister_callback");
		remove_all_devices();
		break;

	case INDIGO_DRIVER_INFO:
		break;
	}

	return INDIGO_OK;
}
