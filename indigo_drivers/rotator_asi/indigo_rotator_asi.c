// Copyright (C) 2024-2026 Rumen G. Bogdanovski
// All rights reserved.

// You may use this software under the terms of 'INDIGO Astronomy
// open-source license' (see LICENSE.md).

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

// This file generated from indigo_rotator_asi.driver

#pragma mark - Includes

#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <assert.h>
#include <pthread.h>

//+ include

#include <stdbool.h>
#include <CAA_API.h>

//- include

#include <indigo/indigo_driver_xml.h>
#include <indigo/indigo_rotator_driver.h>
#include <indigo/indigo_uni_io.h>
#include <indigo/indigo_usb_utils.h>

#include "indigo_rotator_asi.h"

#pragma mark - Common definitions

#define DRIVER_VERSION       0x03000004
#define DRIVER_NAME          "indigo_rotator_asi"
#define DRIVER_LABEL         "ZWO CAA Rotator"
#define ROTATOR_DEVICE_NAME  "%s"
#define MAX_DEVICES          5
#define PRIVATE_DATA         ((asi_private_data *)device->private_data)

//+ define

#define ASI_VENDOR_ID        0x03c3
#define NO_DEVICE            (-1000)

//- define

#pragma mark - Property definitions

#define CAA_BEEP_PROPERTY              (PRIVATE_DATA->caa_beep_property)
#define CAA_BEEP_ON_ITEM               (CAA_BEEP_PROPERTY->items + 0)
#define CAA_BEEP_OFF_ITEM              (CAA_BEEP_PROPERTY->items + 1)

#define CAA_BEEP_PROPERTY_NAME         "CAA_BEEP_ON_MOVE"
#define CAA_BEEP_ON_ITEM_NAME          "ON"
#define CAA_BEEP_OFF_ITEM_NAME         "OFF"

#define CAA_CUSTOM_SUFFIX_PROPERTY      (PRIVATE_DATA->caa_custom_suffix_property)
#define CAA_CUSTOM_SUFFIX_ITEM          (CAA_CUSTOM_SUFFIX_PROPERTY->items + 0)

#define CAA_CUSTOM_SUFFIX_PROPERTY_NAME "CAA_CUSTOM_SUFFIX"
#define CAA_CUSTOM_SUFFIX_ITEM_NAME     "SUFFIX"

#pragma mark - Private data definition

typedef struct {
	libusb_device *usbdev;
	indigo_property *caa_beep_property;
	indigo_property *caa_custom_suffix_property;
	//+ data
	int dev_id;
	bool sdk_open;
	bool moving;
	bool abort_pending;
	CAA_INFO info;
	char model[64];
	char custom_suffix[9];
	float current_position, target_position, min_position, max_position;
	//- data
} asi_private_data;

#pragma mark - Low level code

static indigo_queue *driver_queue = NULL;
static pthread_mutex_t driver_queue_mutex = PTHREAD_MUTEX_INITIALIZER;

//+ code

static int caa_products[100];
static int caa_id_count = 0;

static int find_index_by_device_id(int id) {
	int count = CAAGetNum();
	INDIGO_DRIVER_DEBUG(DRIVER_NAME, "CAAGetNum() = %d", count);
	int cur_id = NO_DEVICE;
	for (int index = 0; index < count; index++) {
		int res = CAAGetID(index, &cur_id);
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "CAAGetID(%d, -> %d) = %d", index, cur_id, res);
		if (res == CAA_SUCCESS && cur_id == id) {
			return index;
		}
	}
	return -1;
}

static void split_device_name(const char *fill_device_name, char *device_name, char *suffix) {
	if (fill_device_name == NULL || device_name == NULL || suffix == NULL) {
		return;
	}
	snprintf(device_name, 64, "%.*s", 63, fill_device_name);
	suffix[0] = 0;
	char *suffix_start = strchr(device_name, '(');
	char *suffix_end = strrchr(device_name, ')');
	if (suffix_start == NULL || suffix_end == NULL || suffix_end <= suffix_start) {
		return;
	}
	*suffix_start++ = 0;
	*suffix_end = 0;
	while (*suffix_start == ' ') {
		suffix_start++;
	}
	snprintf(suffix, 9, "%.8s", suffix_start);
	size_t length = strlen(device_name);
	while (length > 0 && device_name[length - 1] == ' ') {
		device_name[--length] = 0;
	}
}

static bool asi_open(indigo_device *device) {
	if (PRIVATE_DATA->sdk_open) {
		return true;
	}
	if (PRIVATE_DATA->dev_id < 0 || PRIVATE_DATA->dev_id >= CAA_ID_MAX || find_index_by_device_id(PRIVATE_DATA->dev_id) < 0) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "CAA device %d is no longer available", PRIVATE_DATA->dev_id);
		return false;
	}
	if (indigo_try_global_lock(device) != INDIGO_OK) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "indigo_try_global_lock(): failed to get lock.");
		return false;
	}
	int res = CAAOpen(PRIVATE_DATA->dev_id);
	if (res != CAA_SUCCESS) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "CAAOpen(%d) = %d", PRIVATE_DATA->dev_id, res);
		indigo_global_unlock(device);
	} else {
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "CAAOpen(%d) = %d", PRIVATE_DATA->dev_id, res);
		PRIVATE_DATA->sdk_open = true;
	}
	return res == CAA_SUCCESS;
}

static void asi_close(indigo_device *device) {
	if (!PRIVATE_DATA->sdk_open) {
		return;
	}
	int res = CAAStop(PRIVATE_DATA->dev_id);
	if (res != CAA_SUCCESS) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "CAAStop(%d) = %d", PRIVATE_DATA->dev_id, res);
	}
	res = CAAClose(PRIVATE_DATA->dev_id);
	if (res != CAA_SUCCESS) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "CAAClose(%d) = %d", PRIVATE_DATA->dev_id, res);
	} else {
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "CAAClose(%d) = %d", PRIVATE_DATA->dev_id, res);
	}
	indigo_global_unlock(device);
	PRIVATE_DATA->sdk_open = false;
	PRIVATE_DATA->moving = false;
	PRIVATE_DATA->abort_pending = false;
}

//- code

//+ rotator.code

static void rotator_update_motion(indigo_device *device, indigo_property_state state, const char *message) {
	ROTATOR_POSITION_PROPERTY->state = ROTATOR_RELATIVE_MOVE_PROPERTY->state = state;
	ROTATOR_POSITION_ITEM->number.value = PRIVATE_DATA->current_position;
	indigo_update_property(device, ROTATOR_POSITION_PROPERTY, message);
	indigo_update_property(device, ROTATOR_RELATIVE_MOVE_PROPERTY, NULL);
}

static bool rotator_read_position(indigo_device *device) {
	float position = 0;
	int res = CAAGetDegree(PRIVATE_DATA->dev_id, &position);
	if (res != CAA_SUCCESS || !isfinite(position)) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "CAAGetDegree(%d) = %d, position = %f", PRIVATE_DATA->dev_id, res, position);
		return false;
	}
	PRIVATE_DATA->current_position = position;
	ROTATOR_POSITION_ITEM->number.value = position;
	return true;
}

static void rotator_move_finalizer(indigo_device *device);

static bool rotator_motion_ready(indigo_device *device) {
	bool moving = false, hand_control = false;
	int res = CAAIsMoving(PRIVATE_DATA->dev_id, &moving, &hand_control);
	if (res != CAA_SUCCESS) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "CAAIsMoving(%d) = %d", PRIVATE_DATA->dev_id, res);
		rotator_update_motion(device, INDIGO_ALERT_STATE, "Failed to read motion status");
		return false;
	}
	PRIVATE_DATA->moving = moving || hand_control;
	if (PRIVATE_DATA->moving || PRIVATE_DATA->abort_pending) {
		ROTATOR_POSITION_ITEM->number.target = PRIVATE_DATA->target_position;
		rotator_update_motion(device, INDIGO_BUSY_STATE, "Rotator is moving");
		indigo_cancel_pending_handler(device, rotator_move_finalizer);
		indigo_execute_handler_in(device, 0.5, rotator_move_finalizer);
		return false;
	}
	return true;
}

static void rotator_move_finalizer(indigo_device *device) {
	if (!IS_CONNECTED) {
		return;
	}
	bool moving = false, hand_control = false;
	int res = CAAIsMoving(PRIVATE_DATA->dev_id, &moving, &hand_control);
	bool position_ok = rotator_read_position(device);
	if (res != CAA_SUCCESS || !position_ok) {
		PRIVATE_DATA->moving = res != CAA_SUCCESS || moving || hand_control;
		rotator_update_motion(device, INDIGO_ALERT_STATE, "Failed to read motion status or position");
		if (PRIVATE_DATA->abort_pending) {
			PRIVATE_DATA->abort_pending = false;
			ROTATOR_ABORT_MOTION_PROPERTY->state = INDIGO_ALERT_STATE;
			indigo_update_property(device, ROTATOR_ABORT_MOTION_PROPERTY, NULL);
		}
		return;
	}
	PRIVATE_DATA->moving = moving || hand_control;
	rotator_update_motion(device, PRIVATE_DATA->moving ? INDIGO_BUSY_STATE : INDIGO_OK_STATE, NULL);
	if (PRIVATE_DATA->abort_pending) {
		if (hand_control) {
			PRIVATE_DATA->abort_pending = false;
			ROTATOR_ABORT_MOTION_PROPERTY->state = INDIGO_ALERT_STATE;
		} else if (!moving) {
			PRIVATE_DATA->abort_pending = false;
			PRIVATE_DATA->target_position = PRIVATE_DATA->current_position;
			ROTATOR_POSITION_ITEM->number.target = PRIVATE_DATA->current_position;
			ROTATOR_ABORT_MOTION_PROPERTY->state = INDIGO_OK_STATE;
			indigo_update_property(device, ROTATOR_POSITION_PROPERTY, NULL);
		}
		indigo_update_property(device, ROTATOR_ABORT_MOTION_PROPERTY, hand_control ? "Release the hand controller to stop motion" : NULL);
	}
	if (PRIVATE_DATA->moving) {
		indigo_execute_handler_in(device, 0.5, rotator_move_finalizer);
	}
}

static bool rotator_valid_target(indigo_device *device, double target) {
	return target >= PRIVATE_DATA->min_position && target <= PRIVATE_DATA->max_position;
}

static void rotator_start_move(indigo_device *device, double target) {
	if (!rotator_valid_target(device, target)) {
		rotator_update_motion(device, INDIGO_ALERT_STATE, "Target is outside rotator limits");
		return;
	}
	indigo_cancel_pending_handler(device, rotator_move_finalizer);
	int res = CAAMoveTo(PRIVATE_DATA->dev_id, (float)target);
	if (res != CAA_SUCCESS) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "CAAMoveTo(%d, %f) = %d", PRIVATE_DATA->dev_id, target, res);
		rotator_update_motion(device, INDIGO_ALERT_STATE, "Failed to start motion");
		return;
	}
	PRIVATE_DATA->target_position = (float)target;
	ROTATOR_POSITION_ITEM->number.target = target;
	PRIVATE_DATA->moving = true;
	rotator_update_motion(device, INDIGO_BUSY_STATE, NULL);
	indigo_execute_handler_in(device, 0.5, rotator_move_finalizer);
}

//- rotator.code

#pragma mark - High level code (rotator)

static void rotator_connection_handler(indigo_device *device) {
	if (CONNECTION_CONNECTED_ITEM->sw.value) {
		bool connection_result = true;
		connection_result = asi_open(device);
		if (connection_result) {
			//+ rotator.on_connect
			float maximum = 0, position = 0;
			bool reversed = false, beep = false;
			int res = CAAGetMaxDegree(PRIVATE_DATA->dev_id, &maximum);
			if (res != CAA_SUCCESS || !isfinite(maximum) || maximum < 0 || maximum > 480) {
				INDIGO_DRIVER_ERROR(DRIVER_NAME, "CAAGetMaxDegree(%d) = %d", PRIVATE_DATA->dev_id, res);
				connection_result = false;
			}
			res = CAAGetDegree(PRIVATE_DATA->dev_id, &position);
			if (res != CAA_SUCCESS || !isfinite(position)) {
				INDIGO_DRIVER_ERROR(DRIVER_NAME, "CAAGetDegree(%d) = %d", PRIVATE_DATA->dev_id, res);
				connection_result = false;
			}
			res = CAAGetReverse(PRIVATE_DATA->dev_id, &reversed);
			if (res != CAA_SUCCESS) {
				INDIGO_DRIVER_ERROR(DRIVER_NAME, "CAAGetReverse(%d) = %d", PRIVATE_DATA->dev_id, res);
				connection_result = false;
			}
			res = CAAGetBeep(PRIVATE_DATA->dev_id, &beep);
			if (res != CAA_SUCCESS) {
				INDIGO_DRIVER_ERROR(DRIVER_NAME, "CAAGetBeep(%d) = %d", PRIVATE_DATA->dev_id, res);
				connection_result = false;
			}
			if (connection_result) {
				PRIVATE_DATA->moving = false;
				PRIVATE_DATA->abort_pending = false;
				ROTATOR_ABORT_MOTION_ITEM->sw.value = false;
				ROTATOR_ABORT_MOTION_PROPERTY->state = INDIGO_OK_STATE;
				PRIVATE_DATA->min_position = 0;
				PRIVATE_DATA->max_position = maximum;
				PRIVATE_DATA->current_position = PRIVATE_DATA->target_position = position;
				ROTATOR_LIMITS_MIN_POSITION_ITEM->number.value = ROTATOR_LIMITS_MIN_POSITION_ITEM->number.target = ROTATOR_LIMITS_MIN_POSITION_ITEM->number.min = ROTATOR_LIMITS_MIN_POSITION_ITEM->number.max = 0;
				ROTATOR_LIMITS_MAX_POSITION_ITEM->number.value = ROTATOR_LIMITS_MAX_POSITION_ITEM->number.target = maximum;
				ROTATOR_POSITION_ITEM->number.value = ROTATOR_POSITION_ITEM->number.target = position;
				ROTATOR_DIRECTION_REVERSED_ITEM->sw.value = reversed;
				ROTATOR_DIRECTION_NORMAL_ITEM->sw.value = !reversed;
				CAA_BEEP_ON_ITEM->sw.value = beep;
				CAA_BEEP_OFF_ITEM->sw.value = !beep;
				ROTATOR_POSITION_PROPERTY->state = ROTATOR_RELATIVE_MOVE_PROPERTY->state = INDIGO_OK_STATE;
				ROTATOR_DIRECTION_PROPERTY->state = CAA_BEEP_PROPERTY->state = INDIGO_OK_STATE;
				indigo_execute_handler_in(device, 0.5, rotator_move_finalizer);
			} else {
				asi_close(device);
			}
			//- rotator.on_connect
		}
		if (connection_result) {
			indigo_define_property(device, CAA_BEEP_PROPERTY, NULL);
			indigo_define_property(device, CAA_CUSTOM_SUFFIX_PROPERTY, NULL);
			CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
			indigo_send_message(device, OK_PROPERTY, "Connected to %s", device->name);
		} else {
			indigo_send_message(device, ALERT_PROPERTY, "Failed to connect to %s", device->name);
			CONNECTION_PROPERTY->state = INDIGO_ALERT_STATE;
			indigo_set_switch(CONNECTION_PROPERTY, CONNECTION_DISCONNECTED_ITEM, true);
		}
	} else {
		indigo_cancel_pending_handlers(device);
		indigo_delete_property(device, CAA_BEEP_PROPERTY, NULL);
		indigo_delete_property(device, CAA_CUSTOM_SUFFIX_PROPERTY, NULL);
		asi_close(device);
		indigo_send_message(device, OK_PROPERTY, "Disconnected from %s", device->name);
		CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
	}
	indigo_rotator_change_property(device, NULL, CONNECTION_PROPERTY);
}

static void rotator_direction_handler(indigo_device *device) {
	ROTATOR_DIRECTION_PROPERTY->state = INDIGO_OK_STATE;
	//+ rotator.ROTATOR_DIRECTION.on_change
	int res = CAASetReverse(PRIVATE_DATA->dev_id, ROTATOR_DIRECTION_REVERSED_ITEM->sw.value);
	if (res != CAA_SUCCESS) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "CAASetReverse(%d, %d) = %d", PRIVATE_DATA->dev_id, ROTATOR_DIRECTION_REVERSED_ITEM->sw.value, res);
		ROTATOR_DIRECTION_PROPERTY->state = INDIGO_ALERT_STATE;
	}
	//- rotator.ROTATOR_DIRECTION.on_change
	indigo_update_property(device, ROTATOR_DIRECTION_PROPERTY, NULL);
}

static void rotator_position_handler(indigo_device *device) {
	//+ rotator.ROTATOR_POSITION.on_change
	double target = ROTATOR_POSITION_ITEM->number.target;
	if (!rotator_motion_ready(device)) {
		return;
	}
	if (!rotator_valid_target(device, target)) {
		rotator_update_motion(device, INDIGO_ALERT_STATE, "Target is outside rotator limits");
		return;
	}
	if (!rotator_read_position(device)) {
		rotator_update_motion(device, INDIGO_ALERT_STATE, "Failed to read position");
		return;
	}
	if (ROTATOR_ON_POSITION_SET_GOTO_ITEM->sw.value) {
		rotator_start_move(device, target);
	} else {
		indigo_cancel_pending_handler(device, rotator_move_finalizer);
		int res = CAACurDegree(PRIVATE_DATA->dev_id, (float)target);
		bool position_ok = rotator_read_position(device);
		if (res != CAA_SUCCESS || !position_ok) {
			INDIGO_DRIVER_ERROR(DRIVER_NAME, "CAACurDegree(%d, %f) = %d", PRIVATE_DATA->dev_id, target, res);
			rotator_update_motion(device, INDIGO_ALERT_STATE, "Failed to sync position");
		} else {
			PRIVATE_DATA->target_position = PRIVATE_DATA->current_position;
			ROTATOR_POSITION_ITEM->number.target = PRIVATE_DATA->current_position;
			rotator_update_motion(device, INDIGO_OK_STATE, NULL);
		}
	}
	//- rotator.ROTATOR_POSITION.on_change
}

static void rotator_limits_handler(indigo_device *device) {
	ROTATOR_LIMITS_PROPERTY->state = INDIGO_OK_STATE;
	//+ rotator.ROTATOR_LIMITS.on_change
	double requested = ROTATOR_LIMITS_MAX_POSITION_ITEM->number.target;
	int res = CAASetMaxDegree(PRIVATE_DATA->dev_id, (float)requested);
	if (res != CAA_SUCCESS) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "CAASetMaxDegree(%d, %f) = %d", PRIVATE_DATA->dev_id, requested, res);
		ROTATOR_LIMITS_PROPERTY->state = INDIGO_ALERT_STATE;
	}
	float confirmed = 0;
	res = CAAGetMaxDegree(PRIVATE_DATA->dev_id, &confirmed);
	if (res != CAA_SUCCESS || !isfinite(confirmed) || confirmed < 0 || confirmed > ROTATOR_LIMITS_MAX_POSITION_ITEM->number.max) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "CAAGetMaxDegree(%d) = %d", PRIVATE_DATA->dev_id, res);
		ROTATOR_LIMITS_PROPERTY->state = INDIGO_ALERT_STATE;
	} else {
		PRIVATE_DATA->max_position = confirmed;
	}
	ROTATOR_LIMITS_MIN_POSITION_ITEM->number.value = ROTATOR_LIMITS_MIN_POSITION_ITEM->number.target = 0;
	ROTATOR_LIMITS_MAX_POSITION_ITEM->number.value = ROTATOR_LIMITS_MAX_POSITION_ITEM->number.target = PRIVATE_DATA->max_position;
	//- rotator.ROTATOR_LIMITS.on_change
	indigo_update_property(device, ROTATOR_LIMITS_PROPERTY, NULL);
}

static void rotator_relative_move_handler(indigo_device *device) {
	//+ rotator.ROTATOR_RELATIVE_MOVE.on_change
	double delta = ROTATOR_RELATIVE_MOVE_ITEM->number.target;
	if (!rotator_motion_ready(device)) {
		return;
	}
	if (!rotator_read_position(device)) {
		rotator_update_motion(device, INDIGO_ALERT_STATE, "Failed to read position");
		return;
	}
	indigo_cancel_pending_handler(device, rotator_move_finalizer);
	rotator_start_move(device, (double)PRIVATE_DATA->current_position + delta);
	//- rotator.ROTATOR_RELATIVE_MOVE.on_change
}

static void rotator_abort_motion_handler(indigo_device *device) {
	//+ rotator.ROTATOR_ABORT_MOTION.on_change
	ROTATOR_ABORT_MOTION_ITEM->sw.value = false;
	int res = CAAStop(PRIVATE_DATA->dev_id);
	indigo_cancel_pending_handler(device, rotator_move_finalizer);
	PRIVATE_DATA->abort_pending = res == CAA_SUCCESS;
	ROTATOR_ABORT_MOTION_PROPERTY->state = res == CAA_SUCCESS ? INDIGO_BUSY_STATE : INDIGO_ALERT_STATE;
	rotator_move_finalizer(device);
	if (res != CAA_SUCCESS) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "CAAStop(%d) = %d", PRIVATE_DATA->dev_id, res);
		ROTATOR_ABORT_MOTION_PROPERTY->state = INDIGO_ALERT_STATE;
	}
	indigo_update_property(device, ROTATOR_ABORT_MOTION_PROPERTY, res == CAA_SUCCESS ? NULL : "Failed to stop rotator");
	//- rotator.ROTATOR_ABORT_MOTION.on_change
}

static void rotator_caa_beep_handler(indigo_device *device) {
	CAA_BEEP_PROPERTY->state = INDIGO_OK_STATE;
	//+ rotator.CAA_BEEP.on_change
	int res = CAASetBeep(PRIVATE_DATA->dev_id, CAA_BEEP_ON_ITEM->sw.value);
	if (res != CAA_SUCCESS) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "CAASetBeep(%d, %d) = %d", PRIVATE_DATA->dev_id, CAA_BEEP_ON_ITEM->sw.value, res);
		CAA_BEEP_PROPERTY->state = INDIGO_ALERT_STATE;
	}
	//- rotator.CAA_BEEP.on_change
	indigo_update_property(device, CAA_BEEP_PROPERTY, NULL);
}

static void rotator_caa_custom_suffix_handler(indigo_device *device) {
	CAA_CUSTOM_SUFFIX_PROPERTY->state = INDIGO_OK_STATE;
	//+ rotator.CAA_CUSTOM_SUFFIX.on_change
	if (strlen(CAA_CUSTOM_SUFFIX_ITEM->text.value) > 8) {
		CAA_CUSTOM_SUFFIX_PROPERTY->state = INDIGO_ALERT_STATE;
		INDIGO_COPY_VALUE(CAA_CUSTOM_SUFFIX_ITEM->text.value, PRIVATE_DATA->custom_suffix);
		indigo_update_property(device, CAA_CUSTOM_SUFFIX_PROPERTY, "Custom suffix too long");
		return;
	}
	CAA_ID caa_id = { 0 };
	memcpy(caa_id.id, CAA_CUSTOM_SUFFIX_ITEM->text.value, strlen(CAA_CUSTOM_SUFFIX_ITEM->text.value));
	int res = CAASetID(PRIVATE_DATA->dev_id, caa_id);
	if (res != CAA_SUCCESS) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "CAASetID(%d) = %d", PRIVATE_DATA->dev_id, res);
		CAA_CUSTOM_SUFFIX_PROPERTY->state = INDIGO_ALERT_STATE;
		INDIGO_COPY_VALUE(CAA_CUSTOM_SUFFIX_ITEM->text.value, PRIVATE_DATA->custom_suffix);
		indigo_update_property(device, CAA_CUSTOM_SUFFIX_PROPERTY, "Failed to set custom suffix");
		return;
	}
	INDIGO_DRIVER_DEBUG(DRIVER_NAME, "CAASetID(%d, \"%s\") = %d", PRIVATE_DATA->dev_id, CAA_CUSTOM_SUFFIX_ITEM->text.value, res);
	snprintf(PRIVATE_DATA->custom_suffix, sizeof(PRIVATE_DATA->custom_suffix), "%.8s", CAA_CUSTOM_SUFFIX_ITEM->text.value);
	indigo_send_message(device, CAA_CUSTOM_SUFFIX_PROPERTY, PRIVATE_DATA->custom_suffix[0] ? "Rotator name suffix will be used on replug" : "Rotator name suffix cleared, will be used on replug");
	//- rotator.CAA_CUSTOM_SUFFIX.on_change
	indigo_update_property(device, CAA_CUSTOM_SUFFIX_PROPERTY, NULL);
}

#pragma mark - Device API (rotator)

static indigo_result rotator_enumerate_properties(indigo_device *device, indigo_client *client, indigo_property *property);

static indigo_result rotator_attach(indigo_device *device) {
	if (indigo_rotator_attach(device, DRIVER_NAME, DRIVER_VERSION) == INDIGO_OK) {
		//+ rotator.on_attach
		INFO_PROPERTY->count = 6;
		INDIGO_COPY_VALUE(INFO_DEVICE_MODEL_ITEM->text.value, PRIVATE_DATA->model);
		const char *sdk_version = CAAGetSDKVersion();
		INDIGO_COPY_VALUE(INFO_DEVICE_FW_REVISION_ITEM->text.value, sdk_version);
		INDIGO_COPY_VALUE(INFO_DEVICE_FW_REVISION_ITEM->label, "SDK version");
		ROTATOR_LIMITS_MAX_POSITION_ITEM->number.min = 0;
		ROTATOR_LIMITS_MAX_POSITION_ITEM->number.value =
		ROTATOR_LIMITS_MAX_POSITION_ITEM->number.target = 360;
		ROTATOR_LIMITS_MAX_POSITION_ITEM->number.max = 480;
		ROTATOR_LIMITS_MIN_POSITION_ITEM->number.min = 0;
		ROTATOR_LIMITS_MIN_POSITION_ITEM->number.value =
		ROTATOR_LIMITS_MIN_POSITION_ITEM->number.target = 0;
		ROTATOR_LIMITS_MIN_POSITION_ITEM->number.max = 480;
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "\'%s\' MaxStep = %d", device->name, PRIVATE_DATA->info.MaxStep);
		ROTATOR_POSITION_ITEM->number.min = 0;
		ROTATOR_POSITION_ITEM->number.step = 1;
		ROTATOR_POSITION_ITEM->number.max = 480;
		ROTATOR_RELATIVE_MOVE_ITEM->number.min = -120;
		ROTATOR_RELATIVE_MOVE_ITEM->number.step = 1;
		ROTATOR_RELATIVE_MOVE_ITEM->number.max = 120;
		//- rotator.on_attach
		ROTATOR_DIRECTION_PROPERTY->hidden = false;
		ROTATOR_POSITION_PROPERTY->hidden = false;
		ROTATOR_LIMITS_PROPERTY->hidden = false;
		ROTATOR_RELATIVE_MOVE_PROPERTY->hidden = false;
		ROTATOR_ABORT_MOTION_PROPERTY->hidden = false;
		ROTATOR_ON_POSITION_SET_PROPERTY->hidden = false;
		ROTATOR_BACKLASH_PROPERTY->hidden = true;
		CAA_BEEP_PROPERTY = indigo_init_switch_property(NULL, device->name, CAA_BEEP_PROPERTY_NAME, ROTATOR_ADVANCED_GROUP, "Beep on move", INDIGO_OK_STATE, INDIGO_RW_PERM, INDIGO_ONE_OF_MANY_RULE, 2);
		if (CAA_BEEP_PROPERTY == NULL) {
			return INDIGO_FAILED;
		}
		indigo_init_switch_item(CAA_BEEP_ON_ITEM, CAA_BEEP_ON_ITEM_NAME, "On", false);
		indigo_init_switch_item(CAA_BEEP_OFF_ITEM, CAA_BEEP_OFF_ITEM_NAME, "Off", true);
		CAA_CUSTOM_SUFFIX_PROPERTY = indigo_init_text_property(NULL, device->name, CAA_CUSTOM_SUFFIX_PROPERTY_NAME, ROTATOR_ADVANCED_GROUP, "Device name custom suffix", INDIGO_OK_STATE, INDIGO_RW_PERM, 1);
		if (CAA_CUSTOM_SUFFIX_PROPERTY == NULL) {
			return INDIGO_FAILED;
		}
		indigo_init_text_item(CAA_CUSTOM_SUFFIX_ITEM, CAA_CUSTOM_SUFFIX_ITEM_NAME, "Suffix", PRIVATE_DATA->custom_suffix);
		INDIGO_DEVICE_ATTACH_LOG(DRIVER_NAME, device->name);
		return rotator_enumerate_properties(device, NULL, NULL);
	}
	return INDIGO_FAILED;
}

static indigo_result rotator_enumerate_properties(indigo_device *device, indigo_client *client, indigo_property *property) {
	if (IS_CONNECTED) {
		INDIGO_DEFINE_MATCHING_PROPERTY(CAA_BEEP_PROPERTY);
		INDIGO_DEFINE_MATCHING_PROPERTY(CAA_CUSTOM_SUFFIX_PROPERTY);
	}
	return indigo_rotator_enumerate_properties(device, client, property);
}

static indigo_result rotator_change_property(indigo_device *device, indigo_client *client, indigo_property *property) {
	if (indigo_property_match_changeable(CONNECTION_PROPERTY, property)) {
		if (!indigo_ignore_connection_change(device, property)) {
			indigo_property_copy_values(CONNECTION_PROPERTY, property, false);
			INDIGO_UPDATE_PROPERTY_STATE(CONNECTION_PROPERTY, INDIGO_BUSY_STATE, NULL);
			indigo_queue_add(driver_queue, device, INDIGO_TASK_PRIORITY_NORMAL, 0, rotator_connection_handler, &driver_queue_mutex);
		}
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(ROTATOR_DIRECTION_PROPERTY, property)) {
		INDIGO_COPY_VALUES_PROCESS_CHANGE(ROTATOR_DIRECTION_PROPERTY, rotator_direction_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(ROTATOR_POSITION_PROPERTY, property)) {
		INDIGO_COPY_TARGETS_PROCESS_CHANGE(ROTATOR_POSITION_PROPERTY, rotator_position_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(ROTATOR_LIMITS_PROPERTY, property)) {
		INDIGO_COPY_TARGETS_PROCESS_CHANGE(ROTATOR_LIMITS_PROPERTY, rotator_limits_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(ROTATOR_RELATIVE_MOVE_PROPERTY, property)) {
		INDIGO_COPY_TARGETS_PROCESS_CHANGE(ROTATOR_RELATIVE_MOVE_PROPERTY, rotator_relative_move_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(ROTATOR_ABORT_MOTION_PROPERTY, property)) {
		INDIGO_COPY_VALUES_PROCESS_CHANGE(ROTATOR_ABORT_MOTION_PROPERTY, rotator_abort_motion_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(CAA_BEEP_PROPERTY, property)) {
		INDIGO_COPY_VALUES_PROCESS_CHANGE(CAA_BEEP_PROPERTY, rotator_caa_beep_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(CAA_CUSTOM_SUFFIX_PROPERTY, property)) {
		INDIGO_COPY_VALUES_PROCESS_CHANGE(CAA_CUSTOM_SUFFIX_PROPERTY, rotator_caa_custom_suffix_handler);
		return INDIGO_OK;
	} else if (indigo_property_match(CONFIG_PROPERTY, property)) {
		if (indigo_switch_match(CONFIG_SAVE_ITEM, property)) {
			indigo_save_property(device, NULL, CAA_BEEP_PROPERTY);
		}
	}
	return indigo_rotator_change_property(device, client, property);
}

static indigo_result rotator_detach(indigo_device *device) {
	if (IS_CONNECTED) {
		indigo_set_switch(CONNECTION_PROPERTY, CONNECTION_DISCONNECTED_ITEM, true);
		rotator_connection_handler(device);
	}
	indigo_release_property(CAA_BEEP_PROPERTY);
	indigo_release_property(CAA_CUSTOM_SUFFIX_PROPERTY);
	INDIGO_DEVICE_DETACH_LOG(DRIVER_NAME, device->name);
	return indigo_rotator_detach(device);
}

#pragma mark - Device templates

static indigo_device rotator_template = INDIGO_DEVICE_INITIALIZER(ROTATOR_DEVICE_NAME, rotator_attach, rotator_enumerate_properties, rotator_change_property, NULL, rotator_detach);

#pragma mark - Hot-plug code

static indigo_device *devices[MAX_DEVICES];

static void process_plug_event_handler(indigo_device *device, void *data) {
	indigo_set_handler_max_run_time(1);
	libusb_device *dev = (libusb_device *)data;
	bool dev_ref_transferred = false;
	asi_private_data *private_data = NULL;
	bool plug_result = true;
	char name[INDIGO_NAME_SIZE] = DRIVER_LABEL;
	private_data = indigo_safe_malloc(sizeof(asi_private_data));
	private_data->usbdev = dev;
	struct libusb_device_descriptor descriptor;
	if (!(libusb_get_device_descriptor(dev, &descriptor) == LIBUSB_SUCCESS && descriptor.idVendor == ASI_VENDOR_ID)) {
		plug_result = false;
	}
	if (plug_result) {
		//+ sdk.plug
		plug_result = false;
		for (int i = 0; i < caa_id_count; i++) {
			if (caa_products[i] == descriptor.idProduct) {
				plug_result = true;
				break;
			}
		}
		if (plug_result) {
			plug_result = false;
			int count = CAAGetNum();
			for (int index = 0; index < count; index++) {
				int id = -1;
				int res = CAAGetID(index, &id);
				if (res != CAA_SUCCESS || id < 0 || id >= CAA_ID_MAX) {
					continue;
				}
				bool attached = false;
				for (int slot = 0; slot < MAX_DEVICES; slot++) {
					if (devices[slot] != NULL && ((asi_private_data *)devices[slot]->private_data)->dev_id == id) {
						attached = true;
						break;
					}
				}
				if (attached) {
					continue;
				}
				res = CAAOpen(id);
				if (res != CAA_SUCCESS) {
					continue;
				}
				CAA_INFO info = { 0 };
				res = CAAGetProperty(id, &info);
				CAAClose(id);
				if (res != CAA_SUCCESS) {
					continue;
				}
				private_data->dev_id = id;
				private_data->info = info;
				split_device_name(info.Name, private_data->model, private_data->custom_suffix);
				if (private_data->custom_suffix[0]) {
					snprintf(name, INDIGO_NAME_SIZE, "%s #%s", private_data->model, private_data->custom_suffix);
				} else {
					snprintf(name, INDIGO_NAME_SIZE, "%s", private_data->model);
				}
				indigo_make_name_unique(name, "%d", id);
				plug_result = true;
				break;
			}
		}
		//- sdk.plug
	}
	if (plug_result) {
		indigo_device *rotator = indigo_safe_malloc_copy(sizeof(indigo_device), &rotator_template);
		rotator->private_data = private_data;
		snprintf(rotator->name, INDIGO_NAME_SIZE, "%s", name);
		bool rotator_attached = false;
		for (int j = 0; j < MAX_DEVICES; j++) {
			if (devices[j] == NULL) {
				devices[j] = rotator;
				if (indigo_attach_device(rotator) == INDIGO_OK) {
					dev_ref_transferred = true;
					rotator_attached = true;
				} else {
					devices[j] = NULL;
				}
				break;
			}
		}
		if (!rotator_attached) {
			free(rotator);
		}
	}
	if (!dev_ref_transferred) {
		free(private_data);
		libusb_unref_device(dev);
	}
}

static void process_unplug_event_handler(indigo_device *device, void *data) {
	libusb_device *dev = (libusb_device *)data;
	asi_private_data *private_data = NULL;
	for (int j = MAX_DEVICES - 1; j >= 0; j--) {
		if (devices[j] != NULL) {
			indigo_device *device = devices[j];
			if (PRIVATE_DATA->usbdev == dev) {
				private_data = PRIVATE_DATA;
				indigo_detach_device(device);
				free(device);
				devices[j] = NULL;
			}
		}
	}
	if (private_data != NULL) {
		libusb_unref_device(dev);
		free(private_data);
	}
	libusb_unref_device(dev);
}

static int hotplug_callback(libusb_context *ctx, libusb_device *dev, libusb_hotplug_event event, void *user_data) {
	switch (event) {
		case LIBUSB_HOTPLUG_EVENT_DEVICE_ARRIVED: {
			dev = libusb_ref_device(dev);
			indigo_queue_add_with_data(driver_queue, NULL, INDIGO_TASK_PRIORITY_NORMAL, 0, process_plug_event_handler, dev, &driver_queue_mutex);
			break;
		}
		case LIBUSB_HOTPLUG_EVENT_DEVICE_LEFT: {
			dev = libusb_ref_device(dev);
			indigo_queue_add_with_data(driver_queue, NULL, INDIGO_TASK_PRIORITY_NORMAL, 0, process_unplug_event_handler, dev, &driver_queue_mutex);
			break;
		}
		default:
			break;
	}
	return 0;
}

static libusb_hotplug_callback_handle callback_handle;

#pragma mark - Main code

indigo_result indigo_rotator_asi(indigo_driver_action action, indigo_driver_info *info) {
	static indigo_driver_action last_action = INDIGO_DRIVER_SHUTDOWN;

	SET_DRIVER_INFO(info, DRIVER_LABEL, __FUNCTION__, DRIVER_VERSION, false, last_action);

	if (action == last_action) {
		return INDIGO_OK;
	}

	switch (action) {
		case INDIGO_DRIVER_INIT:
			last_action = action;
			//+ on_init
			const char *sdk_version = CAAGetSDKVersion();
			INDIGO_DRIVER_LOG(DRIVER_NAME, "CAA SDK v. %s ", sdk_version);
			caa_id_count = CAAGetProductIDs(NULL);
			if (caa_id_count <= 0 || caa_id_count > (int)(sizeof(caa_products) / sizeof(caa_products[0]))) {
				INDIGO_DRIVER_ERROR(DRIVER_NAME, "Invalid CAA product count: %d", caa_id_count);
				return INDIGO_FAILED;
			}
			caa_id_count = CAAGetProductIDs(caa_products);
			if (caa_id_count <= 0 || caa_id_count > (int)(sizeof(caa_products) / sizeof(caa_products[0]))) {
				return INDIGO_FAILED;
			}
			INDIGO_DRIVER_DEBUG(DRIVER_NAME, "CAAGetProductIDs(-> [ %d, %d, ... ]) = %d", caa_products[0], caa_products[1], caa_id_count);
			//- on_init
			for (int i = 0; i < MAX_DEVICES; i++) {
				devices[i] = NULL;
			}
			driver_queue = indigo_queue_create(NULL);
			if (driver_queue == NULL) {
				INDIGO_DRIVER_ERROR(DRIVER_NAME, "Failed to create driver queue");
				return INDIGO_FAILED;
			}
			indigo_queue_set_name(driver_queue, "Queue " DRIVER_LABEL);
			indigo_start_usb_event_handler();
			int rc = libusb_hotplug_register_callback(NULL, LIBUSB_HOTPLUG_EVENT_DEVICE_ARRIVED | LIBUSB_HOTPLUG_EVENT_DEVICE_LEFT, LIBUSB_HOTPLUG_ENUMERATE, ASI_VENDOR_ID, LIBUSB_HOTPLUG_MATCH_ANY, LIBUSB_HOTPLUG_MATCH_ANY, hotplug_callback, NULL, &callback_handle);
			INDIGO_DRIVER_DEBUG(DRIVER_NAME, "libusb_hotplug_register_callback ->  %s", rc < 0 ? libusb_error_name(rc) : "OK");
			break;

		case INDIGO_DRIVER_SHUTDOWN:
			for (int i = 0; i < MAX_DEVICES; i++) {
				VERIFY_NOT_CONNECTED(devices[i]);
			}
			last_action = action;
			libusb_hotplug_deregister_callback(NULL, callback_handle);
			INDIGO_DRIVER_DEBUG(DRIVER_NAME, "libusb_hotplug_deregister_callback");
			for (int i = 0; i < MAX_DEVICES; i++) {
				if (devices[i] != NULL) {
					indigo_device *device = devices[i];
					process_unplug_event_handler(NULL, libusb_ref_device(PRIVATE_DATA->usbdev));
				}
			}
			indigo_queue_delete(&driver_queue);
			break;

		case INDIGO_DRIVER_INFO:
			break;
	}

	return INDIGO_OK;
}
