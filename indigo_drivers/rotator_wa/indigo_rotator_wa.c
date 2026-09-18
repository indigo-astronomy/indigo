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

// This file generated from indigo_rotator_wa.driver

#pragma mark - Includes

#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <assert.h>

//+ include

#include <errno.h>

//- include

#include <indigo/indigo_driver_xml.h>
#include <indigo/indigo_rotator_driver.h>
#include <indigo/indigo_uni_io.h>

#include "indigo_rotator_wa.h"

#pragma mark - Common definitions

#define DRIVER_VERSION       0x03000005
#define DRIVER_NAME          "indigo_rotator_wa"
#define DRIVER_LABEL         "WandererAstro rotator"
#define ROTATOR_DEVICE_NAME  "WandererAstro rotator"
#define PRIVATE_DATA         ((wa_private_data *)device->private_data)

//+ define

#define WA_RESPONSE_SIZE     256

//- define

#pragma mark - Property definitions

#define X_SET_ZERO_POSITION_PROPERTY      (PRIVATE_DATA->x_set_zero_position_property)
#define X_SET_ZERO_POSITION_ITEM          (X_SET_ZERO_POSITION_PROPERTY->items + 0)

#define X_SET_ZERO_POSITION_PROPERTY_NAME "X_SET_ZERO_POSITION"
#define X_SET_ZERO_POSITION_ITEM_NAME     "SET_ZERO_POSITION"

#pragma mark - Private data definition

typedef struct {
	indigo_uni_handle *handle;
	indigo_property *x_set_zero_position_property;
	//+ data
	char response[WA_RESPONSE_SIZE];
	char model[50];
	char firmware[20];
	int steps_degree;
	double current_position;
	double pivot_position;
	double backlash;
	bool reverse;
	bool motion;
	double motion_deadline;
	//- data
} wa_private_data;

#pragma mark - Low level code

//+ code

static bool wa_write(indigo_device *device, const char *format, ...) {
	va_list args;
	va_start(args, format);
	long result = indigo_uni_vprintf(PRIVATE_DATA->handle, format, args);
	va_end(args);
	return result > 0;
}

static bool wa_read(indigo_device *device) {
	long count = indigo_uni_read_section2(PRIVATE_DATA->handle, PRIVATE_DATA->response, sizeof(PRIVATE_DATA->response) - 1, "\n", "\r", INDIGO_DELAY(1), INDIGO_DELAY(0.1));
	if (count <= 1 || count >= (long)sizeof(PRIVATE_DATA->response) - 1 || PRIVATE_DATA->response[count - 1] != '\n') {
		return false;
	}
	PRIVATE_DATA->response[count - 1] = 0;
	return true;
}

static bool wa_number(const char *text, double *value) {
	char *end;
	errno = 0;
	double parsed = strtod(text, &end);
	if (text == end || *end || errno || !isfinite(parsed)) {
		return false;
	}
	*value = parsed;
	return true;
}

static bool wa_parse(indigo_device *device, bool handshake) {
	char *fields[6];
	int count = 0;
	char *cursor = PRIVATE_DATA->response;
	while (*cursor && count < 6) {
		fields[count++] = cursor;
		char *delimiter = strchr(cursor, 'A');
		if (delimiter == NULL) {
			cursor += strlen(cursor);
			break;
		}
		*delimiter = 0;
		cursor = delimiter + 1;
	}
	if (*cursor || count != (handshake ? 5 : 2)) {
		return false;
	}
	double position;
	if (!wa_number(fields[handshake ? 2 : 1], &position) || fabs(position) > 1e9) {
		return false;
	}
	if (handshake) {
		int steps = 0;
		if (!strcmp(fields[0], "WandererRotatorMini") || !strcmp(fields[0], "WandererRotatorMiniV2")) {
			steps = 1142;
		} else if (!strcmp(fields[0], "WandererRotatorLite") || !strcmp(fields[0], "WandererRotatorLiteV2")) {
			steps = 1199;
		}
		double firmware, backlash, reverse;
		if (!steps || strlen(fields[1]) >= sizeof(PRIVATE_DATA->firmware) || !wa_number(fields[1], &firmware) || firmware < 1 || firmware != floor(firmware) || !wa_number(fields[3], &backlash) || backlash < 0 || backlash > 5 || !wa_number(fields[4], &reverse) || (reverse != 0 && reverse != 1)) {
			return false;
		}
		snprintf(PRIVATE_DATA->model, sizeof(PRIVATE_DATA->model), "%s", fields[0]);
		snprintf(PRIVATE_DATA->firmware, sizeof(PRIVATE_DATA->firmware), "%s", fields[1]);
		PRIVATE_DATA->steps_degree = steps;
		PRIVATE_DATA->backlash = backlash;
		PRIVATE_DATA->reverse = reverse != 0;
	} else {
		double moved;
		if (!wa_number(fields[0], &moved)) {
			return false;
		}
	}
	PRIVATE_DATA->current_position = position / 1000;
	return true;
}

static bool wa_status(indigo_device *device) {
	return indigo_uni_discard(PRIVATE_DATA->handle) >= 0 && wa_write(device, "1500001\n") && wa_read(device) && wa_parse(device, true);
}

static bool wa_open(indigo_device *device) {
	PRIVATE_DATA->handle = indigo_uni_open_serial_with_speed(DEVICE_PORT_ITEM->text.value, 19200, INDIGO_LOG_DEBUG);
	if (PRIVATE_DATA->handle != NULL) {
		indigo_sleep(2);
		if (wa_status(device)) {
			return true;
		}
	}
	indigo_uni_close(&PRIVATE_DATA->handle);
	return false;
}

static void wa_close(indigo_device *device) {
	if (PRIVATE_DATA->motion) {
		wa_write(device, "stop\n");
	}
	PRIVATE_DATA->motion = false;
	indigo_uni_close(&PRIVATE_DATA->handle);
}

//- code

//+ rotator.code

static void wa_pivot(indigo_device *device) {
	PRIVATE_DATA->pivot_position = round((PRIVATE_DATA->current_position + ROTATOR_POSITION_OFFSET_ITEM->number.value) / 360) * 360;
}

static void wa_publish_position(indigo_device *device) {
	ROTATOR_RAW_POSITION_ITEM->number.value = PRIVATE_DATA->current_position;
	ROTATOR_POSITION_ITEM->number.value = indigo_range360(PRIVATE_DATA->current_position + ROTATOR_POSITION_OFFSET_ITEM->number.value);
	indigo_update_property(device, ROTATOR_RAW_POSITION_PROPERTY, NULL);
	indigo_update_property(device, ROTATOR_POSITION_PROPERTY, NULL);
}

static void wa_finish(indigo_device *device, bool success) {
	PRIVATE_DATA->motion = false;
	ROTATOR_POSITION_PROPERTY->state = ROTATOR_RELATIVE_MOVE_PROPERTY->state = success ? INDIGO_OK_STATE : INDIGO_ALERT_STATE;
	ROTATOR_RELATIVE_MOVE_ITEM->number.value = ROTATOR_RELATIVE_MOVE_ITEM->number.target = 0;
	wa_publish_position(device);
	indigo_update_property(device, ROTATOR_RELATIVE_MOVE_PROPERTY, success ? NULL : "Rotator motion failed or was interrupted");
}

static void motion_finalizer(indigo_device *device) {
	if (!PRIVATE_DATA->motion) {
		return;
	}
	long ready = indigo_uni_wait_for_data(PRIVATE_DATA->handle, INDIGO_DELAY(0.001));
	if (ready > 0) {
		bool success = wa_read(device) && wa_parse(device, false);
		if (!success) {
			wa_write(device, "stop\n");
		}
		wa_finish(device, success);
	} else if (ready < 0 || indigo_monotonic_time() >= PRIVATE_DATA->motion_deadline) {
		wa_write(device, "stop\n");
		wa_finish(device, false);
	} else {
		indigo_execute_handler_in(device, 0.05, motion_finalizer);
	}
}

static void wa_start_motion(indigo_device *device, double degrees) {
	int steps = (int)round(degrees * PRIVATE_DATA->steps_degree);
	if (steps == 0) {
		wa_finish(device, true);
	} else if (indigo_uni_discard(PRIVATE_DATA->handle) >= 0 && wa_write(device, "%d\n", steps)) {
		PRIVATE_DATA->motion = true;
		PRIVATE_DATA->motion_deadline = indigo_monotonic_time() + 30 + fabs(degrees) * 2;
		ROTATOR_POSITION_PROPERTY->state = ROTATOR_RELATIVE_MOVE_PROPERTY->state = INDIGO_BUSY_STATE;
		indigo_update_property(device, ROTATOR_POSITION_PROPERTY, NULL);
		indigo_update_property(device, ROTATOR_RELATIVE_MOVE_PROPERTY, NULL);
		indigo_execute_handler_in(device, 0.05, motion_finalizer);
	} else {
		wa_finish(device, false);
	}
}

//- rotator.code

#pragma mark - High level code (rotator)

static void rotator_connection_handler(indigo_device *device) {
	if (CONNECTION_CONNECTED_ITEM->sw.value) {
		bool connection_result = true;
		connection_result = wa_open(device);
		if (connection_result) {
			//+ rotator.on_connect
			PRIVATE_DATA->motion = false;
			ROTATOR_BACKLASH_PROPERTY->state = ROTATOR_DIRECTION_PROPERTY->state = X_SET_ZERO_POSITION_PROPERTY->state = INDIGO_OK_STATE;
			ROTATOR_RELATIVE_MOVE_ITEM->number.value = ROTATOR_RELATIVE_MOVE_ITEM->number.target = 0;
			ROTATOR_POSITION_PROPERTY->state = ROTATOR_RELATIVE_MOVE_PROPERTY->state = INDIGO_OK_STATE;
			ROTATOR_POSITION_ITEM->number.value = ROTATOR_POSITION_ITEM->number.target = indigo_range360(PRIVATE_DATA->current_position + ROTATOR_POSITION_OFFSET_ITEM->number.value);
			ROTATOR_RAW_POSITION_ITEM->number.value = PRIVATE_DATA->current_position;
			ROTATOR_BACKLASH_ITEM->number.value = ROTATOR_BACKLASH_ITEM->number.target = PRIVATE_DATA->backlash;
			indigo_set_switch(ROTATOR_DIRECTION_PROPERTY, PRIVATE_DATA->reverse ? ROTATOR_DIRECTION_REVERSED_ITEM : ROTATOR_DIRECTION_NORMAL_ITEM, true);
			INDIGO_COPY_VALUE(INFO_DEVICE_MODEL_ITEM->text.value, PRIVATE_DATA->model);
			INDIGO_COPY_VALUE(INFO_DEVICE_FW_REVISION_ITEM->text.value, PRIVATE_DATA->firmware);
			wa_pivot(device);
			indigo_update_property(device, INFO_PROPERTY, NULL);
			//- rotator.on_connect
		}
		if (connection_result) {
			indigo_define_property(device, X_SET_ZERO_POSITION_PROPERTY, NULL);
			CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
			indigo_send_message(device, OK_PROPERTY, "Connected to %s on %s", ROTATOR_DEVICE_NAME, DEVICE_PORT_ITEM->text.value);
		} else {
			indigo_send_message(device, ALERT_PROPERTY, "Failed to connect to %s on %s", ROTATOR_DEVICE_NAME, DEVICE_PORT_ITEM->text.value);
			CONNECTION_PROPERTY->state = INDIGO_ALERT_STATE;
			indigo_set_switch(CONNECTION_PROPERTY, CONNECTION_DISCONNECTED_ITEM, true);
		}
	} else {
		indigo_cancel_pending_handlers(device);
		//+ rotator.on_disconnect
		ROTATOR_POSITION_PROPERTY->state = ROTATOR_RELATIVE_MOVE_PROPERTY->state = INDIGO_ALERT_STATE;
		INDIGO_COPY_VALUE(INFO_DEVICE_MODEL_ITEM->text.value, "Unknown");
		INDIGO_COPY_VALUE(INFO_DEVICE_FW_REVISION_ITEM->text.value, "Unknown");
		indigo_update_property(device, INFO_PROPERTY, NULL);
		//- rotator.on_disconnect
		indigo_delete_property(device, X_SET_ZERO_POSITION_PROPERTY, NULL);
		wa_close(device);
		indigo_send_message(device, OK_PROPERTY, "Disconnected from %s", device->name);
		CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
	}
	indigo_rotator_change_property(device, NULL, CONNECTION_PROPERTY);
}

static void rotator_position_offset_handler(indigo_device *device) {
	ROTATOR_POSITION_OFFSET_PROPERTY->state = INDIGO_OK_STATE;
	//+ rotator.ROTATOR_POSITION_OFFSET.on_change
	wa_pivot(device);
	wa_publish_position(device);
	indigo_rotator_save_calibration(device);
	//- rotator.ROTATOR_POSITION_OFFSET.on_change
	indigo_update_property(device, ROTATOR_POSITION_OFFSET_PROPERTY, NULL);
}

static void rotator_position_handler(indigo_device *device) {
	//+ rotator.ROTATOR_POSITION.on_change
	if (ROTATOR_ON_POSITION_SET_SYNC_ITEM->sw.value && !wa_status(device)) {
		wa_finish(device, false);
	} else if (ROTATOR_ON_POSITION_SET_SYNC_ITEM->sw.value) {
		ROTATOR_POSITION_OFFSET_ITEM->number.value = ROTATOR_POSITION_OFFSET_ITEM->number.target = indigo_range360(ROTATOR_POSITION_ITEM->number.target - PRIVATE_DATA->current_position);
		wa_pivot(device);
		indigo_rotator_save_calibration(device);
		ROTATOR_POSITION_PROPERTY->state = INDIGO_OK_STATE;
		wa_publish_position(device);
		indigo_update_property(device, ROTATOR_POSITION_OFFSET_PROPERTY, NULL);
	} else if (wa_status(device)) {
		double base = PRIVATE_DATA->current_position + ROTATOR_POSITION_OFFSET_ITEM->number.value;
		double move = ROTATOR_POSITION_ITEM->number.target - indigo_range360(base);
		if (move < 0 && base + move < PRIVATE_DATA->pivot_position - 180) {
			move += 360;
		} else if (move > 0 && base + move > PRIVATE_DATA->pivot_position + 180) {
			move -= 360;
		}
		wa_start_motion(device, move); // motion_finalizer completes the operation
	} else {
		wa_finish(device, false);
	}
	//- rotator.ROTATOR_POSITION.on_change
}

static void rotator_relative_move_handler(indigo_device *device) {
	//+ rotator.ROTATOR_RELATIVE_MOVE.on_change
	wa_start_motion(device, ROTATOR_RELATIVE_MOVE_ITEM->number.target); /* motion_finalizer */
	//- rotator.ROTATOR_RELATIVE_MOVE.on_change
}

static void rotator_abort_motion_handler(indigo_device *device) {
	//+ rotator.ROTATOR_ABORT_MOTION.on_change
	ROTATOR_ABORT_MOTION_PROPERTY->state = INDIGO_OK_STATE;
	bool requested = ROTATOR_ABORT_MOTION_ITEM->sw.value;
	ROTATOR_ABORT_MOTION_ITEM->sw.value = false;
	if (requested) {
		indigo_cancel_pending_handler(device, rotator_position_handler);
		indigo_cancel_pending_handler(device, rotator_relative_move_handler);
		indigo_cancel_pending_handler(device, motion_finalizer);
		bool success = wa_write(device, "stop\n") && wa_status(device);
		wa_finish(device, false);
		if (!success) {
			ROTATOR_ABORT_MOTION_PROPERTY->state = INDIGO_ALERT_STATE;
		}
	}
	indigo_update_property(device, ROTATOR_ABORT_MOTION_PROPERTY, NULL);
	//- rotator.ROTATOR_ABORT_MOTION.on_change
}

static void rotator_direction_handler(indigo_device *device) {
	ROTATOR_DIRECTION_PROPERTY->state = INDIGO_OK_STATE;
	//+ rotator.ROTATOR_DIRECTION.on_change
	bool reverse = ROTATOR_DIRECTION_REVERSED_ITEM->sw.value;
	bool previous = PRIVATE_DATA->reverse;
	if (!wa_write(device, "170000%d\n", reverse ? 1 : 0) || !wa_status(device) || PRIVATE_DATA->reverse != reverse) {
		indigo_set_switch(ROTATOR_DIRECTION_PROPERTY, previous ? ROTATOR_DIRECTION_REVERSED_ITEM : ROTATOR_DIRECTION_NORMAL_ITEM, true);
		ROTATOR_DIRECTION_PROPERTY->state = INDIGO_ALERT_STATE;
	}
	//- rotator.ROTATOR_DIRECTION.on_change
	indigo_update_property(device, ROTATOR_DIRECTION_PROPERTY, NULL);
}

static void rotator_backlash_handler(indigo_device *device) {
	ROTATOR_BACKLASH_PROPERTY->state = INDIGO_OK_STATE;
	//+ rotator.ROTATOR_BACKLASH.on_change
	int tenths = (int)(ROTATOR_BACKLASH_ITEM->number.target * 10);
	double requested = tenths / 10.0;
	if (wa_write(device, "1600%03d\n", tenths) && wa_status(device) && fabs(PRIVATE_DATA->backlash - requested) < 0.001) {
		ROTATOR_BACKLASH_ITEM->number.value = PRIVATE_DATA->backlash;
	} else {
		ROTATOR_BACKLASH_PROPERTY->state = INDIGO_ALERT_STATE;
	}
	//- rotator.ROTATOR_BACKLASH.on_change
	indigo_update_property(device, ROTATOR_BACKLASH_PROPERTY, NULL);
}

static void rotator_x_set_zero_position_handler(indigo_device *device) {
	X_SET_ZERO_POSITION_PROPERTY->state = INDIGO_OK_STATE;
	//+ rotator.X_SET_ZERO_POSITION.on_change
	bool requested = X_SET_ZERO_POSITION_ITEM->sw.value;
	X_SET_ZERO_POSITION_ITEM->sw.value = false;
	if (requested) {
		if (wa_write(device, "1500002\n") && wa_status(device) && fabs(PRIVATE_DATA->current_position) < 0.001) {
			ROTATOR_POSITION_OFFSET_ITEM->number.value = ROTATOR_POSITION_OFFSET_ITEM->number.target = 0;
			ROTATOR_POSITION_ITEM->number.target = 0;
			wa_pivot(device);
			wa_finish(device, true);
			indigo_update_property(device, ROTATOR_POSITION_OFFSET_PROPERTY, NULL);
			indigo_rotator_save_calibration(device);
		} else {
			X_SET_ZERO_POSITION_PROPERTY->state = INDIGO_ALERT_STATE;
		}
	}
	//- rotator.X_SET_ZERO_POSITION.on_change
	indigo_update_property(device, X_SET_ZERO_POSITION_PROPERTY, NULL);
}

#pragma mark - Device API (rotator)

static indigo_result rotator_enumerate_properties(indigo_device *device, indigo_client *client, indigo_property *property);

static indigo_result rotator_attach(indigo_device *device) {
	if (indigo_rotator_attach(device, DRIVER_NAME, DRIVER_VERSION) == INDIGO_OK) {
		ADDITIONAL_INSTANCES_PROPERTY->hidden = device->base_device != NULL;
		DEVICE_PORT_PROPERTY->hidden = false;
		DEVICE_PORTS_PROPERTY->hidden = false;
		indigo_enumerate_serial_ports(device, DEVICE_PORTS_PROPERTY);
		//+ rotator.on_attach
		INFO_PROPERTY->count = 6;
		ROTATOR_POSITION_ITEM->number.min = 0;
		ROTATOR_POSITION_ITEM->number.max = 360;
		ROTATOR_BACKLASH_ITEM->number.min = 0;
		ROTATOR_BACKLASH_ITEM->number.max = 5;
		INDIGO_COPY_VALUE(ROTATOR_BACKLASH_ITEM->label, "Backlash [°]");
		INDIGO_COPY_VALUE(ROTATOR_BACKLASH_ITEM->number.format, "%g");
		//- rotator.on_attach
		ROTATOR_ON_POSITION_SET_PROPERTY->hidden = false;
		ROTATOR_RAW_POSITION_PROPERTY->hidden = false;
		ROTATOR_POSITION_OFFSET_PROPERTY->hidden = false;
		ROTATOR_POSITION_PROPERTY->hidden = false;
		ROTATOR_RELATIVE_MOVE_PROPERTY->hidden = false;
		ROTATOR_ABORT_MOTION_PROPERTY->hidden = false;
		ROTATOR_DIRECTION_PROPERTY->hidden = false;
		ROTATOR_BACKLASH_PROPERTY->hidden = false;
		X_SET_ZERO_POSITION_PROPERTY = indigo_init_switch_property(NULL, device->name, X_SET_ZERO_POSITION_PROPERTY_NAME, ROTATOR_ADVANCED_GROUP, "Set current position as mechanical zero", INDIGO_OK_STATE, INDIGO_RW_PERM, INDIGO_ONE_OF_MANY_RULE, 1);
		if (X_SET_ZERO_POSITION_PROPERTY == NULL) {
			return INDIGO_FAILED;
		}
		indigo_init_switch_item(X_SET_ZERO_POSITION_ITEM, X_SET_ZERO_POSITION_ITEM_NAME, "Set mechanical zero", false);
		INDIGO_DEVICE_ATTACH_LOG(DRIVER_NAME, device->name);
		return rotator_enumerate_properties(device, NULL, NULL);
	}
	return INDIGO_FAILED;
}

static indigo_result rotator_enumerate_properties(indigo_device *device, indigo_client *client, indigo_property *property) {
	if (IS_CONNECTED) {
		INDIGO_DEFINE_MATCHING_PROPERTY(X_SET_ZERO_POSITION_PROPERTY);
	}
	return indigo_rotator_enumerate_properties(device, client, property);
}

static indigo_result rotator_change_property(indigo_device *device, indigo_client *client, indigo_property *property) {
	if (indigo_property_match_changeable(CONNECTION_PROPERTY, property)) {
		if (!indigo_ignore_connection_change(device, property)) {
			indigo_property_copy_values(CONNECTION_PROPERTY, property, false);
			INDIGO_UPDATE_PROPERTY_STATE(CONNECTION_PROPERTY, INDIGO_BUSY_STATE, NULL);
			indigo_execute_handler(device, rotator_connection_handler);
		}
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(ROTATOR_ON_POSITION_SET_PROPERTY, property)) {
		indigo_property_copy_values(ROTATOR_ON_POSITION_SET_PROPERTY, property, false);
		ROTATOR_ON_POSITION_SET_PROPERTY->state = INDIGO_OK_STATE;
		indigo_update_property(device, ROTATOR_ON_POSITION_SET_PROPERTY, NULL);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(ROTATOR_POSITION_OFFSET_PROPERTY, property)) {
		if (ROTATOR_POSITION_PROPERTY->state == INDIGO_BUSY_STATE || ROTATOR_RELATIVE_MOVE_PROPERTY->state == INDIGO_BUSY_STATE) {
			for (int i = 0; i < ROTATOR_POSITION_OFFSET_PROPERTY->count; i++) {
				ROTATOR_POSITION_OFFSET_PROPERTY->items[i].do_update = true;
			}
			ROTATOR_POSITION_OFFSET_PROPERTY->state = INDIGO_ALERT_STATE;
			indigo_update_property(device, ROTATOR_POSITION_OFFSET_PROPERTY, "Rotator is moving: request can not be completed");
			return INDIGO_OK;
		}
		INDIGO_COPY_VALUES_PROCESS_CHANGE(ROTATOR_POSITION_OFFSET_PROPERTY, rotator_position_offset_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(ROTATOR_POSITION_PROPERTY, property)) {
		if (ROTATOR_RELATIVE_MOVE_PROPERTY->state == INDIGO_BUSY_STATE) {
			for (int i = 0; i < ROTATOR_POSITION_PROPERTY->count; i++) {
				ROTATOR_POSITION_PROPERTY->items[i].do_update = true;
			}
			ROTATOR_POSITION_PROPERTY->state = INDIGO_ALERT_STATE;
			indigo_update_property(device, ROTATOR_POSITION_PROPERTY, "Rotator is moving: request can not be completed");
			return INDIGO_OK;
		}
		INDIGO_COPY_TARGETS_PROCESS_CHANGE(ROTATOR_POSITION_PROPERTY, rotator_position_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(ROTATOR_RELATIVE_MOVE_PROPERTY, property)) {
		if (ROTATOR_POSITION_PROPERTY->state == INDIGO_BUSY_STATE) {
			for (int i = 0; i < ROTATOR_RELATIVE_MOVE_PROPERTY->count; i++) {
				ROTATOR_RELATIVE_MOVE_PROPERTY->items[i].do_update = true;
			}
			ROTATOR_RELATIVE_MOVE_PROPERTY->state = INDIGO_ALERT_STATE;
			indigo_update_property(device, ROTATOR_RELATIVE_MOVE_PROPERTY, "Rotator is moving: request can not be completed");
			return INDIGO_OK;
		}
		INDIGO_COPY_TARGETS_PROCESS_CHANGE(ROTATOR_RELATIVE_MOVE_PROPERTY, rotator_relative_move_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(ROTATOR_ABORT_MOTION_PROPERTY, property)) {
		INDIGO_COPY_VALUES_PROCESS_URGENT_CHANGE(ROTATOR_ABORT_MOTION_PROPERTY, rotator_abort_motion_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(ROTATOR_DIRECTION_PROPERTY, property)) {
		if (ROTATOR_POSITION_PROPERTY->state == INDIGO_BUSY_STATE || ROTATOR_RELATIVE_MOVE_PROPERTY->state == INDIGO_BUSY_STATE) {
			for (int i = 0; i < ROTATOR_DIRECTION_PROPERTY->count; i++) {
				ROTATOR_DIRECTION_PROPERTY->items[i].do_update = true;
			}
			ROTATOR_DIRECTION_PROPERTY->state = INDIGO_ALERT_STATE;
			indigo_update_property(device, ROTATOR_DIRECTION_PROPERTY, "Rotator is moving: request can not be completed");
			return INDIGO_OK;
		}
		INDIGO_COPY_VALUES_PROCESS_CHANGE(ROTATOR_DIRECTION_PROPERTY, rotator_direction_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(ROTATOR_BACKLASH_PROPERTY, property)) {
		if (ROTATOR_POSITION_PROPERTY->state == INDIGO_BUSY_STATE || ROTATOR_RELATIVE_MOVE_PROPERTY->state == INDIGO_BUSY_STATE) {
			for (int i = 0; i < ROTATOR_BACKLASH_PROPERTY->count; i++) {
				ROTATOR_BACKLASH_PROPERTY->items[i].do_update = true;
			}
			ROTATOR_BACKLASH_PROPERTY->state = INDIGO_ALERT_STATE;
			indigo_update_property(device, ROTATOR_BACKLASH_PROPERTY, "Rotator is moving: request can not be completed");
			return INDIGO_OK;
		}
		INDIGO_COPY_TARGETS_PROCESS_CHANGE(ROTATOR_BACKLASH_PROPERTY, rotator_backlash_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(X_SET_ZERO_POSITION_PROPERTY, property)) {
		if (ROTATOR_POSITION_PROPERTY->state == INDIGO_BUSY_STATE || ROTATOR_RELATIVE_MOVE_PROPERTY->state == INDIGO_BUSY_STATE) {
			for (int i = 0; i < X_SET_ZERO_POSITION_PROPERTY->count; i++) {
				X_SET_ZERO_POSITION_PROPERTY->items[i].do_update = true;
			}
			X_SET_ZERO_POSITION_PROPERTY->state = INDIGO_ALERT_STATE;
			indigo_update_property(device, X_SET_ZERO_POSITION_PROPERTY, "Rotator is moving: request can not be completed");
			return INDIGO_OK;
		}
		INDIGO_COPY_VALUES_PROCESS_CHANGE(X_SET_ZERO_POSITION_PROPERTY, rotator_x_set_zero_position_handler);
		return INDIGO_OK;
	}
	return indigo_rotator_change_property(device, client, property);
}

static indigo_result rotator_detach(indigo_device *device) {
	if (IS_CONNECTED) {
		indigo_set_switch(CONNECTION_PROPERTY, CONNECTION_DISCONNECTED_ITEM, true);
		rotator_connection_handler(device);
	}
	indigo_release_property(X_SET_ZERO_POSITION_PROPERTY);
	INDIGO_DEVICE_DETACH_LOG(DRIVER_NAME, device->name);
	return indigo_rotator_detach(device);
}

#pragma mark - Device templates

static indigo_device rotator_template = INDIGO_DEVICE_INITIALIZER(ROTATOR_DEVICE_NAME, rotator_attach, rotator_enumerate_properties, rotator_change_property, NULL, rotator_detach);

#pragma mark - Main code

indigo_result indigo_rotator_wa(indigo_driver_action action, indigo_driver_info *info) {
	static indigo_driver_action last_action = INDIGO_DRIVER_SHUTDOWN;
	static wa_private_data *private_data = NULL;
	static indigo_device *rotator = NULL;

	SET_DRIVER_INFO(info, DRIVER_LABEL, __FUNCTION__, DRIVER_VERSION, true, last_action);

	if (action == last_action) {
		return INDIGO_OK;
	}

	switch (action) {
		case INDIGO_DRIVER_INIT: {
			last_action = action;
			private_data = (wa_private_data *)indigo_safe_malloc(sizeof(wa_private_data));
			rotator = (indigo_device *)indigo_safe_malloc_copy(sizeof(indigo_device), &rotator_template);
			rotator->private_data = private_data;
			indigo_attach_device(rotator);
			break;

		}
		case INDIGO_DRIVER_SHUTDOWN: {
			VERIFY_NOT_CONNECTED(rotator);
			last_action = action;
			if (rotator != NULL) {
				indigo_detach_device(rotator);
				indigo_safe_free(rotator);
				rotator = NULL;
			}
			if (private_data != NULL) {
				indigo_safe_free(private_data);
				private_data = NULL;
			}
			break;

		}
		case INDIGO_DRIVER_INFO:
			break;
	}

	return INDIGO_OK;
}
