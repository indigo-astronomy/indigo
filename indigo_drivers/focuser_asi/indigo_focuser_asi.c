// Copyright (C) 2019 Rumen G. Bogdanovski
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

/** INDIGO ASI focuser driver
 \file indigo_focuser_asi.c
 */

#define DRIVER_VERSION 0x001A
#define DRIVER_NAME "indigo_focuser_asi"

#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <math.h>
#include <assert.h>
#include <pthread.h>
#include <stdbool.h>
#include <sys/time.h>

#include <indigo/indigo_driver_xml.h>
#include "indigo_focuser_asi.h"

#if defined(INDIGO_FREEBSD)
#include <libusb.h>
#else
#include <libusb-1.0/libusb.h>
#endif

#include <EAF_focuser.h>

#define ASI_VENDOR_ID                   0x03c3
#define EAF_PRODUCT_ID                  0x1f10

#define PRIVATE_DATA                    ((asi_private_data *)device->private_data)

#define EAF_BEEP_PROPERTY               (PRIVATE_DATA->beep_property)
#define EAF_BEEP_ON_ITEM                (EAF_BEEP_PROPERTY->items+0)
#define EAF_BEEP_OFF_ITEM               (EAF_BEEP_PROPERTY->items+1)
#define EAF_BEEP_PROPERTY_NAME          "EAF_BEEP_ON_MOVE"
#define EAF_BEEP_ON_ITEM_NAME           "ON"
#define EAF_BEEP_OFF_ITEM_NAME          "OFF"

#define EAF_CUSTOM_SUFFIX_PROPERTY     (PRIVATE_DATA->custom_suffix_property)
#define EAF_CUSTOM_SUFFIX_ITEM         (EAF_CUSTOM_SUFFIX_PROPERTY->items+0)
#define EAF_CUSTOM_SUFFIX_PROPERTY_NAME   "EAF_CUSTOM_SUFFIX"
#define EAF_CUSTOM_SUFFIX_NAME         "SUFFIX"

// gp_bits is used as boolean
#define is_connected                    gp_bits

typedef struct {
	int dev_id;
	EAF_INFO info;
	char model[64];
	char custom_suffix[9];
	int current_position, target_position, max_position, backlash;
	double prev_temp;
	indigo_timer *focuser_timer, *temperature_timer;
	pthread_mutex_t usb_mutex;
	indigo_property *beep_property;
	indigo_property *custom_suffix_property;
} asi_private_data;

static int find_index_by_device_id(int id);
static void compensate_focus(indigo_device *device, double new_temp);

// -------------------------------------------------------------------------------- INDIGO focuser device implementation
static void focuser_timer_callback(indigo_device *device) {
	bool moving, moving_HC;

	pthread_mutex_lock(&PRIVATE_DATA->usb_mutex);
	int res = EAFIsMoving(PRIVATE_DATA->dev_id, &moving, &moving_HC);
	if (res != EAF_SUCCESS) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "EAFIsMoving(%d) = %d", PRIVATE_DATA->dev_id, res);
		FOCUSER_POSITION_PROPERTY->state = INDIGO_ALERT_STATE;
		FOCUSER_STEPS_PROPERTY->state = INDIGO_ALERT_STATE;
	}

	res = EAFGetPosition(PRIVATE_DATA->dev_id, &(PRIVATE_DATA->current_position));
	if (res != EAF_SUCCESS) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "EAFGetPosition(%d) = %d", PRIVATE_DATA->dev_id, res);
		FOCUSER_POSITION_PROPERTY->state = INDIGO_ALERT_STATE;
		FOCUSER_STEPS_PROPERTY->state = INDIGO_ALERT_STATE;
	}
	INDIGO_DRIVER_DEBUG(DRIVER_NAME, "EAFGetPosition(%d, -> %d) = %d", PRIVATE_DATA->dev_id, PRIVATE_DATA->current_position, res);

	pthread_mutex_unlock(&PRIVATE_DATA->usb_mutex);
	FOCUSER_POSITION_ITEM->number.value = PRIVATE_DATA->current_position;
	if ((!moving) || (PRIVATE_DATA->current_position == PRIVATE_DATA->target_position)) {
		FOCUSER_POSITION_PROPERTY->state = INDIGO_OK_STATE;
		FOCUSER_STEPS_PROPERTY->state = INDIGO_OK_STATE;
	} else {
		indigo_reschedule_timer(device, 0.5, &(PRIVATE_DATA->focuser_timer));
	}
	indigo_update_property(device, FOCUSER_STEPS_PROPERTY, NULL);
	indigo_update_property(device, FOCUSER_POSITION_PROPERTY, NULL);
}


static void temperature_timer_callback(indigo_device *device) {
	float temp;
	static bool has_sensor = true;
	//bool moving = false, moving_HC = false;
	int res;

	FOCUSER_TEMPERATURE_PROPERTY->state = INDIGO_OK_STATE;
	pthread_mutex_lock(&PRIVATE_DATA->usb_mutex);
	res = EAFGetTemp(PRIVATE_DATA->dev_id, &temp);
	FOCUSER_TEMPERATURE_ITEM->number.value = (double)temp;
	pthread_mutex_unlock(&PRIVATE_DATA->usb_mutex);
	if ((res != EAF_SUCCESS) && (FOCUSER_TEMPERATURE_ITEM->number.value > -270.0)) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "EAFGetTemp(%d, -> %f) = %d", PRIVATE_DATA->dev_id, FOCUSER_TEMPERATURE_ITEM->number.value, res);
		FOCUSER_TEMPERATURE_PROPERTY->state = INDIGO_ALERT_STATE;
	} else {
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "EAFGetTemp(%d, -> %f) = %d", PRIVATE_DATA->dev_id, FOCUSER_TEMPERATURE_ITEM->number.value, res);
	}
	// static double ctemp = 0;
	// FOCUSER_TEMPERATURE_ITEM->number.value = ctemp;
	// temp = ctemp;
	// ctemp += 0.12;
	if (FOCUSER_TEMPERATURE_ITEM->number.value < -270.0) { /* -273 is returned when the sensor is not connected */
		FOCUSER_TEMPERATURE_PROPERTY->state = INDIGO_IDLE_STATE;
		if (has_sensor) {
			INDIGO_DRIVER_LOG(DRIVER_NAME, "The temperature sensor is not connected.");
			indigo_update_property(device, FOCUSER_TEMPERATURE_PROPERTY, "The temperature sensor is not connected.");
			has_sensor = false;
		}
	} else {
		has_sensor = true;
		indigo_update_property(device, FOCUSER_TEMPERATURE_PROPERTY, NULL);
	}
	if (FOCUSER_MODE_AUTOMATIC_ITEM->sw.value) {
		compensate_focus(device, temp);
	} else {
		/* reset temp so that the compensation starts when auto mode is selected */
		PRIVATE_DATA->prev_temp = -273;
	}
	indigo_reschedule_timer(device, 2, &(PRIVATE_DATA->temperature_timer));
}


static void compensate_focus(indigo_device *device, double new_temp) {
	int compensation;
	double temp_difference = new_temp - PRIVATE_DATA->prev_temp;

	/* we do not have previous temperature reading */
	if (PRIVATE_DATA->prev_temp < -270) {
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "Not compensating: PRIVATE_DATA->prev_temp = %f", PRIVATE_DATA->prev_temp);
		PRIVATE_DATA->prev_temp = new_temp;
		return;
	}

	/* we do not have current temperature reading or focuser is moving */
	if ((new_temp < -270) || (FOCUSER_POSITION_PROPERTY->state != INDIGO_OK_STATE)) {
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "Not compensating: new_temp = %f, FOCUSER_POSITION_PROPERTY->state = %d", new_temp, FOCUSER_POSITION_PROPERTY->state);
		return;
	}

	/* temperature difference if more than 1 degree so compensation needed */
	if ((fabs(temp_difference) >= FOCUSER_COMPENSATION_THRESHOLD_ITEM->number.value) && (fabs(temp_difference) < 100)) {
		compensation = (int)(temp_difference * FOCUSER_COMPENSATION_ITEM->number.value);
		INDIGO_DRIVER_DEBUG(
			DRIVER_NAME,
			"Compensation: temp_difference = %.2f, compensation = %d, steps/degC = %.0f, threshold = %.2f",
			temp_difference,
			compensation,
			FOCUSER_COMPENSATION_ITEM->number.value,
			FOCUSER_COMPENSATION_THRESHOLD_ITEM->number.value
		);
	} else {
		INDIGO_DRIVER_DEBUG(
			DRIVER_NAME,
			"Not compensating (not needed): temp_difference = %.2f, threshold = %.2f",
			temp_difference,
			FOCUSER_COMPENSATION_THRESHOLD_ITEM->number.value
		);
		return;
	}

	PRIVATE_DATA->target_position = PRIVATE_DATA->current_position + compensation;
	INDIGO_DRIVER_DEBUG(DRIVER_NAME, "Compensation: PRIVATE_DATA->current_position = %d, PRIVATE_DATA->target_position = %d", PRIVATE_DATA->current_position, PRIVATE_DATA->target_position);

	pthread_mutex_lock(&PRIVATE_DATA->usb_mutex);
	int res = EAFGetPosition(PRIVATE_DATA->dev_id, &PRIVATE_DATA->current_position);
	if (res != EAF_SUCCESS) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "EAFGetPosition(%d) = %d", PRIVATE_DATA->dev_id, res);
	}
	pthread_mutex_unlock(&PRIVATE_DATA->usb_mutex);

	/* Make sure we do not attempt to go beyond the limits */
	if (FOCUSER_POSITION_ITEM->number.max < PRIVATE_DATA->target_position) {
		PRIVATE_DATA->target_position = FOCUSER_POSITION_ITEM->number.max;
	} else if (FOCUSER_POSITION_ITEM->number.min > PRIVATE_DATA->target_position) {
		PRIVATE_DATA->target_position = FOCUSER_POSITION_ITEM->number.min;
	}
	INDIGO_DRIVER_DEBUG(DRIVER_NAME, "Compensating: Corrected PRIVATE_DATA->target_position = %d", PRIVATE_DATA->target_position);

	pthread_mutex_lock(&PRIVATE_DATA->usb_mutex);
	res = EAFMove(PRIVATE_DATA->dev_id, PRIVATE_DATA->target_position);
	if (res != EAF_SUCCESS) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "EAFMove(%d, %d) = %d", PRIVATE_DATA->dev_id, PRIVATE_DATA->target_position, res);
		FOCUSER_STEPS_PROPERTY->state = INDIGO_ALERT_STATE;
	}
	pthread_mutex_unlock(&PRIVATE_DATA->usb_mutex);

	PRIVATE_DATA->prev_temp = new_temp;
	FOCUSER_POSITION_ITEM->number.value = PRIVATE_DATA->current_position;
	FOCUSER_POSITION_PROPERTY->state = INDIGO_BUSY_STATE;
	indigo_update_property(device, FOCUSER_POSITION_PROPERTY, NULL);
	indigo_set_timer(device, 0.5, focuser_timer_callback, &PRIVATE_DATA->focuser_timer);
}


static indigo_result eaf_enumerate_properties(indigo_device *device, indigo_client *client, indigo_property *property) {
	if (IS_CONNECTED) {
		indigo_define_matching_property(EAF_BEEP_PROPERTY);
		indigo_define_matching_property(EAF_CUSTOM_SUFFIX_PROPERTY);
	}
	return indigo_focuser_enumerate_properties(device, NULL, NULL);
}


static indigo_result focuser_attach(indigo_device *device) {
	assert(device != NULL);
	assert(PRIVATE_DATA != NULL);
	if (indigo_focuser_attach(device, DRIVER_NAME, DRIVER_VERSION) == INDIGO_OK) {
		pthread_mutex_init(&PRIVATE_DATA->usb_mutex, NULL);

		INFO_PROPERTY->count = 6;
		indigo_copy_value(INFO_DEVICE_MODEL_ITEM->text.value, PRIVATE_DATA->model);
		char *sdk_version = EAFGetSDKVersion();
		indigo_copy_value(INFO_DEVICE_FW_REVISION_ITEM->text.value, sdk_version);
		indigo_copy_value(INFO_DEVICE_FW_REVISION_ITEM->label, "SDK version");


		FOCUSER_LIMITS_PROPERTY->hidden = false;
		FOCUSER_LIMITS_MAX_POSITION_ITEM->number.min = 0;
		FOCUSER_LIMITS_MAX_POSITION_ITEM->number.max = PRIVATE_DATA->info.MaxStep;
		FOCUSER_LIMITS_MIN_POSITION_ITEM->number.min = 0;
		FOCUSER_LIMITS_MIN_POSITION_ITEM->number.value = 0;
		FOCUSER_LIMITS_MIN_POSITION_ITEM->number.max = 0;
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "\'%s\' MaxStep = %d",device->name ,PRIVATE_DATA->info.MaxStep);

		FOCUSER_SPEED_PROPERTY->hidden = true;

		FOCUSER_BACKLASH_PROPERTY->hidden = false;
		FOCUSER_BACKLASH_ITEM->number.min = 0;
		FOCUSER_BACKLASH_ITEM->number.max = 10000;
		FOCUSER_BACKLASH_ITEM->number.step = 1;

		FOCUSER_POSITION_ITEM->number.min = 0;
		FOCUSER_POSITION_ITEM->number.step = 1;
		FOCUSER_POSITION_ITEM->number.max = PRIVATE_DATA->info.MaxStep;

		FOCUSER_STEPS_ITEM->number.min = 0;
		FOCUSER_STEPS_ITEM->number.step = 1;
		FOCUSER_STEPS_ITEM->number.max = PRIVATE_DATA->info.MaxStep;

		FOCUSER_ON_POSITION_SET_PROPERTY->hidden = false;
		FOCUSER_TEMPERATURE_PROPERTY->hidden = false;
		FOCUSER_REVERSE_MOTION_PROPERTY->hidden = false;

		// -------------------------------------------------------------------------- FOCUSER_COMPENSATION
		FOCUSER_COMPENSATION_PROPERTY->hidden = false;
		FOCUSER_COMPENSATION_ITEM->number.min = -10000;
		FOCUSER_COMPENSATION_ITEM->number.max = 10000;
		FOCUSER_COMPENSATION_PROPERTY->count = 2;
		// -------------------------------------------------------------------------- FOCUSER_MODE
		FOCUSER_MODE_PROPERTY->hidden = false;
		// -------------------------------------------------------------------------- BEEP_PROPERTY
		EAF_BEEP_PROPERTY = indigo_init_switch_property(NULL, device->name, EAF_BEEP_PROPERTY_NAME, FOCUSER_ADVANCED_GROUP, "Beep on move", INDIGO_OK_STATE, INDIGO_RW_PERM, INDIGO_ONE_OF_MANY_RULE, 2);
		if (EAF_BEEP_PROPERTY == NULL)
			return INDIGO_FAILED;

		indigo_init_switch_item(EAF_BEEP_ON_ITEM, EAF_BEEP_ON_ITEM_NAME, "On", false);
		indigo_init_switch_item(EAF_BEEP_OFF_ITEM, EAF_BEEP_OFF_ITEM_NAME, "Off", true);
		// --------------------------------------------------------------------------------- EAF_CUSTOM_SUFFIX
		EAF_CUSTOM_SUFFIX_PROPERTY = indigo_init_text_property(NULL, device->name, "EAF_CUSTOM_SUFFIX", FOCUSER_ADVANCED_GROUP, "Device name custom suffix", INDIGO_OK_STATE, INDIGO_RW_PERM, 1);
		if (EAF_CUSTOM_SUFFIX_PROPERTY == NULL)
			return INDIGO_FAILED;
		indigo_init_text_item(EAF_CUSTOM_SUFFIX_ITEM, EAF_CUSTOM_SUFFIX_NAME, "Suffix", PRIVATE_DATA->custom_suffix);
		// --------------------------------------------------------------------------
		return eaf_enumerate_properties(device, NULL, NULL);
	}
	return INDIGO_FAILED;
}

static void focuser_connect_callback(indigo_device *device) {
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
					EAFGetID(index, &(PRIVATE_DATA->dev_id));
					int res = EAFOpen(PRIVATE_DATA->dev_id);
					pthread_mutex_unlock(&PRIVATE_DATA->usb_mutex);
					if (!res) {
						INDIGO_DRIVER_DEBUG(DRIVER_NAME, "EAFOpen(%d) = %d", PRIVATE_DATA->dev_id, res);
						pthread_mutex_lock(&PRIVATE_DATA->usb_mutex);
						res = EAFGetMaxStep(PRIVATE_DATA->dev_id, &(PRIVATE_DATA->max_position));
						if (res != EAF_SUCCESS) {
							INDIGO_DRIVER_ERROR(DRIVER_NAME, "EAFGetMaxStep(%d) = %d", PRIVATE_DATA->dev_id, res);
						}
						FOCUSER_LIMITS_MAX_POSITION_ITEM->number.value = (double)PRIVATE_DATA->max_position;

						res = EAFGetBacklash(PRIVATE_DATA->dev_id, &(PRIVATE_DATA->backlash));
						if (res != EAF_SUCCESS) {
							INDIGO_DRIVER_ERROR(DRIVER_NAME, "EAFGetBacklash(%d) = %d", PRIVATE_DATA->dev_id, res);
						}
						FOCUSER_BACKLASH_ITEM->number.value = (double)PRIVATE_DATA->backlash;

						res = EAFGetPosition(PRIVATE_DATA->dev_id, &(PRIVATE_DATA->target_position));
						if (res != EAF_SUCCESS) {
							INDIGO_DRIVER_ERROR(DRIVER_NAME, "EAFGetPosition(%d, -> %d) = %d", PRIVATE_DATA->dev_id, PRIVATE_DATA->target_position, res);
						}
						FOCUSER_POSITION_ITEM->number.value = PRIVATE_DATA->target_position;

						res = EAFGetReverse(PRIVATE_DATA->dev_id, &(FOCUSER_REVERSE_MOTION_ENABLED_ITEM->sw.value));
						if (res != EAF_SUCCESS) {
							INDIGO_DRIVER_ERROR(DRIVER_NAME, "EAFGetReverse(%d, -> %d) = %d", PRIVATE_DATA->dev_id, FOCUSER_REVERSE_MOTION_ENABLED_ITEM->sw.value, res);
						}
						FOCUSER_REVERSE_MOTION_DISABLED_ITEM->sw.value = !FOCUSER_REVERSE_MOTION_ENABLED_ITEM->sw.value;

						res = EAFGetBeep(PRIVATE_DATA->dev_id, &(EAF_BEEP_ON_ITEM->sw.value));
						if (res != EAF_SUCCESS) {
							INDIGO_DRIVER_ERROR(DRIVER_NAME, "EAFGetBeep(%d, -> %d) = %d", PRIVATE_DATA->dev_id, EAF_BEEP_ON_ITEM->sw.value, res);
						}
						EAF_BEEP_OFF_ITEM->sw.value = !EAF_BEEP_ON_ITEM->sw.value;
						pthread_mutex_unlock(&PRIVATE_DATA->usb_mutex);

						CONNECTION_PROPERTY->state = INDIGO_OK_STATE;

						indigo_define_property(device, EAF_BEEP_PROPERTY, NULL);
						indigo_define_property(device, EAF_CUSTOM_SUFFIX_PROPERTY, NULL);

						PRIVATE_DATA->prev_temp = -273;  /* we do not have previous temperature reading */
						device->is_connected = true;
						indigo_set_timer(device, 0.5, focuser_timer_callback, &PRIVATE_DATA->focuser_timer);
						indigo_set_timer(device, 0.1, temperature_timer_callback, &PRIVATE_DATA->temperature_timer);
					} else {
						INDIGO_DRIVER_ERROR(DRIVER_NAME, "EAFOpen(%d) = %d", index, res);
						CONNECTION_PROPERTY->state = INDIGO_ALERT_STATE;
						indigo_set_switch(CONNECTION_PROPERTY, CONNECTION_DISCONNECTED_ITEM, true);
						indigo_update_property(device, CONNECTION_PROPERTY, NULL);
					}
				}
			}
		}
	} else {
		if (device->is_connected) {
			indigo_cancel_timer_sync(device, &PRIVATE_DATA->focuser_timer);
			indigo_cancel_timer_sync(device, &PRIVATE_DATA->temperature_timer);
			indigo_delete_property(device, EAF_BEEP_PROPERTY, NULL);
			indigo_delete_property(device, EAF_CUSTOM_SUFFIX_PROPERTY, NULL);
			pthread_mutex_lock(&PRIVATE_DATA->usb_mutex);
			int res = EAFStop(PRIVATE_DATA->dev_id);
			res = EAFClose(PRIVATE_DATA->dev_id);
			if (res != EAF_SUCCESS) {
				INDIGO_DRIVER_ERROR(DRIVER_NAME, "EAFClose(%d) = %d", PRIVATE_DATA->dev_id, res);
			} else {
				INDIGO_DRIVER_DEBUG(DRIVER_NAME, "EAFClose(%d) = %d", PRIVATE_DATA->dev_id, res);
			}
			res = EAFGetID(index, &(PRIVATE_DATA->dev_id));
			if (res != EAF_SUCCESS) {
				INDIGO_DRIVER_ERROR(DRIVER_NAME, "EAFGetID(%d, -> %d) = %d", index, PRIVATE_DATA->dev_id, res);
			} else {
				INDIGO_DRIVER_DEBUG(DRIVER_NAME, "EAFGetID(%d, -> %d) = %d", index, PRIVATE_DATA->dev_id, res);
			}
			indigo_global_unlock(device);
			pthread_mutex_unlock(&PRIVATE_DATA->usb_mutex);
			device->is_connected = false;
			CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
		}
	}
	indigo_focuser_change_property(device, NULL, CONNECTION_PROPERTY);
}

static indigo_result focuser_change_property(indigo_device *device, indigo_client *client, indigo_property *property) {
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
		indigo_set_timer(device, 0, focuser_connect_callback, NULL);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(FOCUSER_REVERSE_MOTION_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- FOCUSER_REVERSE_MOTION
		indigo_property_copy_values(FOCUSER_REVERSE_MOTION_PROPERTY, property, false);
		FOCUSER_REVERSE_MOTION_PROPERTY->state = INDIGO_OK_STATE;
		pthread_mutex_lock(&PRIVATE_DATA->usb_mutex);
		int res = EAFSetReverse(PRIVATE_DATA->dev_id, FOCUSER_REVERSE_MOTION_ENABLED_ITEM->sw.value);
		if (res != EAF_SUCCESS) {
			INDIGO_DRIVER_ERROR(DRIVER_NAME, "EAFSetReverse(%d, %d) = %d", PRIVATE_DATA->dev_id, FOCUSER_REVERSE_MOTION_ENABLED_ITEM->sw.value, res);
			FOCUSER_REVERSE_MOTION_PROPERTY->state = INDIGO_ALERT_STATE;
		}
		pthread_mutex_unlock(&PRIVATE_DATA->usb_mutex);
		indigo_update_property(device, FOCUSER_REVERSE_MOTION_PROPERTY, NULL);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(FOCUSER_POSITION_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- FOCUSER_POSITION
		indigo_property_copy_values(FOCUSER_POSITION_PROPERTY, property, false);
		if (FOCUSER_POSITION_PROPERTY->state == INDIGO_BUSY_STATE) {
			return INDIGO_OK;
		}
		if (FOCUSER_POSITION_ITEM->number.target < 0 || FOCUSER_POSITION_ITEM->number.target > FOCUSER_POSITION_ITEM->number.max) {
			FOCUSER_POSITION_PROPERTY->state = INDIGO_ALERT_STATE;
			FOCUSER_STEPS_PROPERTY->state = INDIGO_ALERT_STATE;
			indigo_update_property(device, FOCUSER_STEPS_PROPERTY, NULL);
			indigo_update_property(device, FOCUSER_POSITION_PROPERTY, NULL);
		} else if (FOCUSER_POSITION_ITEM->number.target == PRIVATE_DATA->current_position) {
			FOCUSER_POSITION_PROPERTY->state = INDIGO_OK_STATE;
			FOCUSER_STEPS_PROPERTY->state = INDIGO_OK_STATE;
			indigo_update_property(device, FOCUSER_STEPS_PROPERTY, NULL);
			indigo_update_property(device, FOCUSER_POSITION_PROPERTY, NULL);
		} else {
			FOCUSER_POSITION_PROPERTY->state = INDIGO_BUSY_STATE;
			FOCUSER_STEPS_PROPERTY->state = INDIGO_BUSY_STATE;
			PRIVATE_DATA->target_position = FOCUSER_POSITION_ITEM->number.target;
			FOCUSER_POSITION_ITEM->number.value = PRIVATE_DATA->current_position;
			indigo_update_property(device, FOCUSER_STEPS_PROPERTY, NULL);
			indigo_update_property(device, FOCUSER_POSITION_PROPERTY, NULL);
			if (FOCUSER_ON_POSITION_SET_GOTO_ITEM->sw.value) { /* GOTO POSITION */
				pthread_mutex_lock(&PRIVATE_DATA->usb_mutex);
				int res = EAFMove(PRIVATE_DATA->dev_id, PRIVATE_DATA->target_position);
				if (res != EAF_SUCCESS) {
					INDIGO_DRIVER_ERROR(DRIVER_NAME, "EAFMove(%d, %d) = %d", PRIVATE_DATA->dev_id, PRIVATE_DATA->target_position, res);
				}
				pthread_mutex_unlock(&PRIVATE_DATA->usb_mutex);
				indigo_set_timer(device, 0.5, focuser_timer_callback, &PRIVATE_DATA->focuser_timer);
			} else { /* RESET CURRENT POSITION - SYNC */
				FOCUSER_POSITION_PROPERTY->state = INDIGO_OK_STATE;
				FOCUSER_STEPS_PROPERTY->state = INDIGO_OK_STATE;
				pthread_mutex_lock(&PRIVATE_DATA->usb_mutex);
				int res = EAFResetPostion(PRIVATE_DATA->dev_id, PRIVATE_DATA->target_position);
				if (res != EAF_SUCCESS) {
					INDIGO_DRIVER_ERROR(DRIVER_NAME, "EAFResetPostion(%d, %d) = %d", PRIVATE_DATA->dev_id, PRIVATE_DATA->target_position, res);
					FOCUSER_POSITION_PROPERTY->state = INDIGO_ALERT_STATE;
					FOCUSER_STEPS_PROPERTY->state = INDIGO_ALERT_STATE;
				}
				res = EAFGetPosition(PRIVATE_DATA->dev_id, &PRIVATE_DATA->current_position);
				FOCUSER_POSITION_ITEM->number.value = PRIVATE_DATA->current_position;
				if (res != EAF_SUCCESS) {
					INDIGO_DRIVER_ERROR(DRIVER_NAME, "EAFGetPosition(%d) = %d", PRIVATE_DATA->dev_id, res);
					FOCUSER_POSITION_PROPERTY->state = INDIGO_ALERT_STATE;
					FOCUSER_STEPS_PROPERTY->state = INDIGO_ALERT_STATE;
				}
				pthread_mutex_unlock(&PRIVATE_DATA->usb_mutex);
				indigo_update_property(device, FOCUSER_STEPS_PROPERTY, NULL);
				indigo_update_property(device, FOCUSER_POSITION_PROPERTY, NULL);
			}
		}
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(FOCUSER_LIMITS_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- FOCUSER_LIMITS
		indigo_property_copy_values(FOCUSER_LIMITS_PROPERTY, property, false);
		FOCUSER_LIMITS_PROPERTY->state = INDIGO_OK_STATE;
		PRIVATE_DATA->max_position = (int)FOCUSER_LIMITS_MAX_POSITION_ITEM->number.target;
		pthread_mutex_lock(&PRIVATE_DATA->usb_mutex);
		int res = EAFSetMaxStep(PRIVATE_DATA->dev_id, PRIVATE_DATA->max_position);
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "EAFSetMaxStep(%d, -> %d) = %d", PRIVATE_DATA->dev_id, PRIVATE_DATA->max_position, res);
		if (res != EAF_SUCCESS) {
			INDIGO_DRIVER_ERROR(DRIVER_NAME, "EAFSetMaxStep(%d) = %d", PRIVATE_DATA->dev_id, res);
			FOCUSER_LIMITS_PROPERTY->state = INDIGO_ALERT_STATE;
		}
		res = EAFGetMaxStep(PRIVATE_DATA->dev_id, &(PRIVATE_DATA->max_position));
		if (res != EAF_SUCCESS) {
			INDIGO_DRIVER_ERROR(DRIVER_NAME, "EAFGetMaxStep(%d) = %d", PRIVATE_DATA->dev_id, res);
		}
		FOCUSER_LIMITS_MAX_POSITION_ITEM->number.value = (double)PRIVATE_DATA->max_position;
		pthread_mutex_unlock(&PRIVATE_DATA->usb_mutex);
		indigo_update_property(device, FOCUSER_LIMITS_PROPERTY, NULL);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(FOCUSER_BACKLASH_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- FOCUSER_BACKLASH
		indigo_property_copy_values(FOCUSER_BACKLASH_PROPERTY, property, false);
		FOCUSER_BACKLASH_PROPERTY->state = INDIGO_OK_STATE;
		pthread_mutex_lock(&PRIVATE_DATA->usb_mutex);
		PRIVATE_DATA->backlash = (int)FOCUSER_BACKLASH_ITEM->number.target;
		int res = EAFSetBacklash(PRIVATE_DATA->dev_id, PRIVATE_DATA->backlash);
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "EAFSetBacklash(%d, -> %d) = %d", PRIVATE_DATA->dev_id, PRIVATE_DATA->backlash, res);
		if (res != EAF_SUCCESS) {
			INDIGO_DRIVER_ERROR(DRIVER_NAME, "EAFSetBacklash(%d) = %d", PRIVATE_DATA->dev_id, res);
			FOCUSER_BACKLASH_PROPERTY->state = INDIGO_ALERT_STATE;
		}
		res = EAFGetBacklash(PRIVATE_DATA->dev_id, &(PRIVATE_DATA->backlash));
		if (res != EAF_SUCCESS) {
			INDIGO_DRIVER_ERROR(DRIVER_NAME, "EAFGetBacklash(%d) = %d", PRIVATE_DATA->dev_id, res);
		}
		FOCUSER_BACKLASH_ITEM->number.value = (double)PRIVATE_DATA->backlash;
		pthread_mutex_unlock(&PRIVATE_DATA->usb_mutex);
		indigo_update_property(device, FOCUSER_BACKLASH_PROPERTY, NULL);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(FOCUSER_STEPS_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- FOCUSER_STEPS
		indigo_property_copy_values(FOCUSER_STEPS_PROPERTY, property, false);
		if (FOCUSER_STEPS_PROPERTY->state == INDIGO_BUSY_STATE) {
			return INDIGO_OK;
		}
		if (FOCUSER_STEPS_ITEM->number.value < 0 || FOCUSER_STEPS_ITEM->number.value > FOCUSER_STEPS_ITEM->number.max) {
			FOCUSER_POSITION_PROPERTY->state = INDIGO_ALERT_STATE;
			FOCUSER_STEPS_PROPERTY->state = INDIGO_ALERT_STATE;
			indigo_update_property(device, FOCUSER_STEPS_PROPERTY, NULL);
			indigo_update_property(device, FOCUSER_POSITION_PROPERTY, NULL);
		} else {
			FOCUSER_STEPS_PROPERTY->state = INDIGO_BUSY_STATE;
			FOCUSER_POSITION_PROPERTY->state = INDIGO_BUSY_STATE;
			indigo_update_property(device, FOCUSER_STEPS_PROPERTY, NULL);
			indigo_update_property(device, FOCUSER_POSITION_PROPERTY, NULL);
			pthread_mutex_lock(&PRIVATE_DATA->usb_mutex);
			int res = EAFGetPosition(PRIVATE_DATA->dev_id, &PRIVATE_DATA->current_position);
			if (res != EAF_SUCCESS) {
				INDIGO_DRIVER_ERROR(DRIVER_NAME, "EAFGetPosition(%d) = %d", PRIVATE_DATA->dev_id, res);
			}
			if (FOCUSER_DIRECTION_MOVE_INWARD_ITEM->sw.value) {
				PRIVATE_DATA->target_position = PRIVATE_DATA->current_position - FOCUSER_STEPS_ITEM->number.value;
			} else {
				PRIVATE_DATA->target_position = PRIVATE_DATA->current_position + FOCUSER_STEPS_ITEM->number.value;
			}

			/* Make sure we do not attempt to go beyond the limits */
			if (FOCUSER_POSITION_ITEM->number.max < PRIVATE_DATA->target_position) {
				PRIVATE_DATA->target_position = FOCUSER_POSITION_ITEM->number.max;
			} else if (FOCUSER_POSITION_ITEM->number.min > PRIVATE_DATA->target_position) {
				PRIVATE_DATA->target_position = FOCUSER_POSITION_ITEM->number.min;
			}

			FOCUSER_POSITION_ITEM->number.value = PRIVATE_DATA->current_position;
			res = EAFMove(PRIVATE_DATA->dev_id, PRIVATE_DATA->target_position);
			if (res != EAF_SUCCESS) {
				INDIGO_DRIVER_ERROR(DRIVER_NAME, "EAFMove(%d, %d) = %d", PRIVATE_DATA->dev_id, PRIVATE_DATA->target_position, res);
			}
			pthread_mutex_unlock(&PRIVATE_DATA->usb_mutex);
			indigo_set_timer(device, 0.5, focuser_timer_callback, &PRIVATE_DATA->focuser_timer);
		}
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(FOCUSER_ABORT_MOTION_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- FOCUSER_ABORT_MOTION
		indigo_property_copy_values(FOCUSER_ABORT_MOTION_PROPERTY, property, false);
		FOCUSER_STEPS_PROPERTY->state = INDIGO_OK_STATE;
		FOCUSER_POSITION_PROPERTY->state = INDIGO_OK_STATE;
		FOCUSER_ABORT_MOTION_PROPERTY->state = INDIGO_OK_STATE;
		indigo_cancel_timer(device, &PRIVATE_DATA->focuser_timer);
		pthread_mutex_lock(&PRIVATE_DATA->usb_mutex);
		int res = EAFStop(PRIVATE_DATA->dev_id);
		if (res != EAF_SUCCESS) {
			INDIGO_DRIVER_ERROR(DRIVER_NAME, "EAFStop(%d) = %d", PRIVATE_DATA->dev_id, res);
			FOCUSER_ABORT_MOTION_PROPERTY->state = INDIGO_ALERT_STATE;
		}
		res = EAFGetPosition(PRIVATE_DATA->dev_id, &PRIVATE_DATA->current_position);
		if (res != EAF_SUCCESS) {
			INDIGO_DRIVER_ERROR(DRIVER_NAME, "EAFGetPosition(%d) = %d", PRIVATE_DATA->dev_id, res);
			FOCUSER_ABORT_MOTION_PROPERTY->state = INDIGO_ALERT_STATE;
		}
		pthread_mutex_unlock(&PRIVATE_DATA->usb_mutex);
		FOCUSER_POSITION_ITEM->number.value = PRIVATE_DATA->current_position;
		FOCUSER_ABORT_MOTION_ITEM->sw.value = false;
		indigo_update_property(device, FOCUSER_POSITION_PROPERTY, NULL);
		indigo_update_property(device, FOCUSER_STEPS_PROPERTY, NULL);
		indigo_update_property(device, FOCUSER_ABORT_MOTION_PROPERTY, NULL);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(FOCUSER_COMPENSATION_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- FOCUSER_COMPENSATION_PROPERTY
		indigo_property_copy_values(FOCUSER_COMPENSATION_PROPERTY, property, false);
		FOCUSER_COMPENSATION_PROPERTY->state = INDIGO_OK_STATE;
		indigo_update_property(device, FOCUSER_COMPENSATION_PROPERTY, NULL);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(EAF_BEEP_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- EAF_BEEP_PROPERTY
		indigo_property_copy_values(EAF_BEEP_PROPERTY, property, false);
		EAF_BEEP_PROPERTY->state = INDIGO_OK_STATE;
		pthread_mutex_lock(&PRIVATE_DATA->usb_mutex);
		int res = EAFSetBeep(PRIVATE_DATA->dev_id, EAF_BEEP_ON_ITEM->sw.value);
		if (res != EAF_SUCCESS) {
			INDIGO_DRIVER_ERROR(DRIVER_NAME, "EAFSetBeep(%d, %d) = %d", PRIVATE_DATA->dev_id, EAF_BEEP_ON_ITEM->sw.value, res);
			EAF_BEEP_PROPERTY->state = INDIGO_ALERT_STATE;
		}
		pthread_mutex_unlock(&PRIVATE_DATA->usb_mutex);
		indigo_update_property(device, EAF_BEEP_PROPERTY, NULL);
		return INDIGO_OK;
		// ------------------------------------------------------------------------------- EAF_CUSTOM_SUFFIX
	} else if (indigo_property_match_changeable(EAF_CUSTOM_SUFFIX_PROPERTY, property)) {
		EAF_CUSTOM_SUFFIX_PROPERTY->state = INDIGO_OK_STATE;
		indigo_property_copy_values(EAF_CUSTOM_SUFFIX_PROPERTY, property, false);
		if (strlen(EAF_CUSTOM_SUFFIX_ITEM->text.value) > 8) {
			EAF_CUSTOM_SUFFIX_PROPERTY->state = INDIGO_ALERT_STATE;
			indigo_update_property(device, EAF_CUSTOM_SUFFIX_PROPERTY, "Custom suffix too long");
			return INDIGO_OK;
		}
		pthread_mutex_lock(&PRIVATE_DATA->usb_mutex);
		EAF_ID eaf_id = {0};
		memcpy(eaf_id.id, EAF_CUSTOM_SUFFIX_ITEM->text.value, 8);
		memcpy(PRIVATE_DATA->custom_suffix, EAF_CUSTOM_SUFFIX_ITEM->text.value, sizeof(PRIVATE_DATA->custom_suffix));
		int res = EAFSetID(PRIVATE_DATA->dev_id, eaf_id);
		pthread_mutex_unlock(&PRIVATE_DATA->usb_mutex);
		if (res) {
			INDIGO_DRIVER_ERROR(DRIVER_NAME, "EAFSetID(%d, \"%s\") = %d", PRIVATE_DATA->dev_id, EAF_CUSTOM_SUFFIX_ITEM->text.value, res);
			EAF_CUSTOM_SUFFIX_PROPERTY->state = INDIGO_ALERT_STATE;
			indigo_update_property(device, EAF_CUSTOM_SUFFIX_PROPERTY, NULL);
		} else {
			INDIGO_DRIVER_ERROR(DRIVER_NAME, "EAFSetID(%d, \"%s\") = %d", PRIVATE_DATA->dev_id, EAF_CUSTOM_SUFFIX_ITEM->text.value, res);
			EAF_CUSTOM_SUFFIX_PROPERTY->state = INDIGO_OK_STATE;
			if (strlen(EAF_CUSTOM_SUFFIX_ITEM->text.value) > 0) {
				indigo_update_property(device, EAF_CUSTOM_SUFFIX_PROPERTY, "Focuser name suffix '#%s' will be used on replug", EAF_CUSTOM_SUFFIX_ITEM->text.value);
			} else {
				indigo_update_property(device, EAF_CUSTOM_SUFFIX_PROPERTY, "Focuser name suffix cleared, will be used on replug");
			}
		}
		return INDIGO_OK;
		// -------------------------------------------------------------------------------- FOCUSER_MODE
	} else if (indigo_property_match_changeable(FOCUSER_MODE_PROPERTY, property)) {
		indigo_property_copy_values(FOCUSER_MODE_PROPERTY, property, false);
		if (FOCUSER_MODE_MANUAL_ITEM->sw.value) {
			indigo_define_property(device, FOCUSER_ON_POSITION_SET_PROPERTY, NULL);
			indigo_define_property(device, FOCUSER_SPEED_PROPERTY, NULL);
			indigo_define_property(device, FOCUSER_REVERSE_MOTION_PROPERTY, NULL);
			indigo_define_property(device, FOCUSER_DIRECTION_PROPERTY, NULL);
			indigo_define_property(device, FOCUSER_STEPS_PROPERTY, NULL);
			indigo_define_property(device, FOCUSER_ABORT_MOTION_PROPERTY, NULL);
			indigo_define_property(device, FOCUSER_BACKLASH_PROPERTY, NULL);
			indigo_delete_property(device, FOCUSER_POSITION_PROPERTY, NULL);
			FOCUSER_POSITION_PROPERTY->perm = INDIGO_RW_PERM;
			indigo_define_property(device, FOCUSER_POSITION_PROPERTY, NULL);
		} else {
			indigo_delete_property(device, FOCUSER_ON_POSITION_SET_PROPERTY, NULL);
			indigo_delete_property(device, FOCUSER_SPEED_PROPERTY, NULL);
			indigo_delete_property(device, FOCUSER_REVERSE_MOTION_PROPERTY, NULL);
			indigo_delete_property(device, FOCUSER_DIRECTION_PROPERTY, NULL);
			indigo_delete_property(device, FOCUSER_STEPS_PROPERTY, NULL);
			indigo_delete_property(device, FOCUSER_ABORT_MOTION_PROPERTY, NULL);
			indigo_delete_property(device, FOCUSER_BACKLASH_PROPERTY, NULL);
			indigo_delete_property(device, FOCUSER_POSITION_PROPERTY, NULL);
			FOCUSER_POSITION_PROPERTY->perm = INDIGO_RO_PERM;
			indigo_define_property(device, FOCUSER_POSITION_PROPERTY, NULL);
		}
		FOCUSER_MODE_PROPERTY->state = INDIGO_OK_STATE;
		indigo_update_property(device, FOCUSER_MODE_PROPERTY, NULL);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(CONFIG_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- CONFIG
		if (indigo_switch_match(CONFIG_SAVE_ITEM, property)) {
			indigo_save_property(device, NULL, EAF_BEEP_PROPERTY);
		}
		// --------------------------------------------------------------------------------
	}
	return indigo_focuser_change_property(device, client, property);
}


static indigo_result focuser_detach(indigo_device *device) {
	assert(device != NULL);
	if (IS_CONNECTED) {
		indigo_set_switch(CONNECTION_PROPERTY, CONNECTION_DISCONNECTED_ITEM, true);
		focuser_connect_callback(device);
	}
	indigo_release_property(EAF_BEEP_PROPERTY);
	indigo_release_property(EAF_CUSTOM_SUFFIX_PROPERTY);
	INDIGO_DEVICE_DETACH_LOG(DRIVER_NAME, device->name);
	return indigo_focuser_detach(device);
}

// -------------------------------------------------------------------------------- hot-plug support

#define MAX_DEVICES                   10
#define NO_DEVICE                 (-1000)


static int eaf_products[100];
static int eaf_id_count = 0;


static indigo_device *devices[MAX_DEVICES] = {NULL};
static bool connected_ids[EAF_ID_MAX];

static int find_index_by_device_id(int id) {
	int count = EAFGetNum();
	INDIGO_DRIVER_DEBUG(DRIVER_NAME, "EAFGetNum() = %d", count);
	int cur_id;
	for (int index = 0; index < count; index++) {
		int res = EAFGetID(index, &cur_id);
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "EAFGetID(%d, -> %d) = %d", index, cur_id, res);
		if (res == EAF_SUCCESS && cur_id == id) {
			return index;
		}
	}
	return -1;
}


static int find_plugged_device_id() {
	int id = NO_DEVICE, new_id = NO_DEVICE;
	int count = EAFGetNum();
	INDIGO_DRIVER_DEBUG(DRIVER_NAME, "EAFGetNum() = %d", count);
	for (int index = 0; index < count; index++) {
		int res = EAFGetID(index, &id);
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "EAFGetID(%d, -> %d) = %d", index, id, res);
		if (res == EAF_SUCCESS && !connected_ids[id]) {
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
		if (device == NULL) {
			continue;
		}
		if (PRIVATE_DATA->dev_id == id) return slot;
	}
	return -1;
}


static int find_unplugged_device_id() {
	bool dev_tmp[EAF_ID_MAX] = { false };
	int id = -1;
	int count = EAFGetNum();
	INDIGO_DRIVER_DEBUG(DRIVER_NAME, "EAFGetNum() = %d", count);
	for (int index = 0; index < count; index++) {
		int res = EAFGetID(index, &id);
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "EAFGetID(%d, -> %d) = %d", index, id, res);
		if (res == EAF_SUCCESS)
			dev_tmp[id] = true;
	}
	id = -1;
	for (int index = 0; index < EAF_ID_MAX; index++) {
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
	EAF_INFO info;
	static indigo_device focuser_template = INDIGO_DEVICE_INITIALIZER(
		"",
		focuser_attach,
		eaf_enumerate_properties,
		focuser_change_property,
		NULL,
		focuser_detach
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
	int res = EAFOpen(id);
	if (res) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "EAFOpen(%d}) = %d", id, res);
		pthread_mutex_unlock(&indigo_device_enumeration_mutex);
		return;
	} else {
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "EAFOpen(%d}) = %d", id, res);
	}
	while (true) {
		res = EAFGetProperty(id, &info);
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "EAFGetProperty(%d, -> { %d, '%s', %d }) = %d", id, info.ID, info.Name, info.MaxStep, res);
		if (res == EAF_SUCCESS) {
			EAFClose(id);
			break;
		}
		if (res != EAF_ERROR_MOVING) {
			EAFClose(id);
			pthread_mutex_unlock(&indigo_device_enumeration_mutex);
			return;
		}
		  indigo_usleep(ONE_SECOND_DELAY);
	}
	indigo_device *device = indigo_safe_malloc_copy(sizeof(indigo_device), &focuser_template);
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
		if (slot < 0) {
			continue;
		}
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
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "No ASI EAF device unplugged (maybe ASI Camera)!");
	}
	pthread_mutex_unlock(&indigo_device_enumeration_mutex);
}

static int hotplug_callback(libusb_context *ctx, libusb_device *dev, libusb_hotplug_event event, void *user_data) {
	struct libusb_device_descriptor descriptor;
	switch (event) {
		case LIBUSB_HOTPLUG_EVENT_DEVICE_ARRIVED: {
			libusb_get_device_descriptor(dev, &descriptor);
			INDIGO_DRIVER_DEBUG(DRIVER_NAME, "Device plugged has PID:VID = %x:%x", descriptor.idVendor, descriptor.idProduct);
			for (int i = 0; i < eaf_id_count; i++) {
				if (descriptor.idVendor != ASI_VENDOR_ID || eaf_products[i] != descriptor.idProduct) {
					INDIGO_DRIVER_DEBUG(DRIVER_NAME, "No ASI EAF device plugged (maybe ASI Camera)!");
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
		if (*device == NULL) {
			continue;
		}
		indigo_detach_device(*device);
		free((*device)->private_data);
		free(*device);
		*device = NULL;
	}
	for (int index = 0; index < EAF_ID_MAX; index++)
		connected_ids[index] = false;
}


static libusb_hotplug_callback_handle callback_handle;

indigo_result indigo_focuser_asi(indigo_driver_action action, indigo_driver_info *info) {
	static indigo_driver_action last_action = INDIGO_DRIVER_SHUTDOWN;

	SET_DRIVER_INFO(info, "ZWO ASI Focuser", __FUNCTION__, DRIVER_VERSION, true, last_action);

	if (action == last_action)
		return INDIGO_OK;

	switch (action) {
	case INDIGO_DRIVER_INIT:
		last_action = action;

		const char *sdk_version = EAFGetSDKVersion();
		INDIGO_DRIVER_LOG(DRIVER_NAME, "EAF SDK v. %s ", sdk_version);

		for(int index = 0; index < EAF_ID_MAX; index++)
			connected_ids[index] = false;
//		eaf_id_count = EAFGetProductIDs(eaf_products);
//		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "EAFGetProductIDs(-> [ %d, %d, ... ]) = %d", eaf_products[0], eaf_products[1], eaf_id_count);
		eaf_products[0] = EAF_PRODUCT_ID;
		eaf_id_count = 1;
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

