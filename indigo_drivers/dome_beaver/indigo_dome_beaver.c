// Copyright (c) 2020-2026 Rumen G. Bogdanovski
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

// This file generated from indigo_dome_beaver.driver

#pragma mark - Includes

#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <assert.h>

//+ include

#include <math.h>
#include <stdarg.h>

//- include

#include <indigo/indigo_driver_xml.h>
#include <indigo/indigo_dome_driver.h>
#include <indigo/indigo_uni_io.h>

#include "indigo_dome_beaver.h"

#pragma mark - Common definitions

#define DRIVER_VERSION       0x03000004
#define DRIVER_NAME          "indigo_dome_beaver"
#define DRIVER_LABEL         "Nexdome Beaver Dome"
#define DOME_DEVICE_NAME     "Nexdome Beaver Dome"
#define PRIVATE_DATA         ((beaver_private_data *)device->private_data)

//+ define

#define BEAVER_BAUDRATE      "115200"
#define BEAVER_NETWORK_PORT  8080
#define BEAVER_FIRST_BYTE_TIMEOUT 3
#define BEAVER_NEXT_BYTE_TIMEOUT 0.5
#define BEAVER_FIRST_POLL_DELAY 0.5
#define BEAVER_POLL_DELAY    1
#define BEAVER_SETTLE_DELAY  0.5
#define CHECK_BIT(bitmap,    bit) (((bitmap) >> (bit)) & 1UL)

//- define

#pragma mark - Property definitions

#define X_SHUTTER_CALIBRATE_PROPERTY      (PRIVATE_DATA->x_shutter_calibrate_property)
#define X_SHUTTER_CALIBRATE_ITEM          (X_SHUTTER_CALIBRATE_PROPERTY->items + 0)

#define X_SHUTTER_CALIBRATE_PROPERTY_NAME "X_SHUTTER_CALIBRATE"
#define X_SHUTTER_CALIBRATE_ITEM_NAME     "CALIBRATE"

#define X_ROTATOR_CALIBRATE_PROPERTY      (PRIVATE_DATA->x_rotator_calibrate_property)
#define X_ROTATOR_CALIBRATE_ITEM          (X_ROTATOR_CALIBRATE_PROPERTY->items + 0)

#define X_ROTATOR_CALIBRATE_PROPERTY_NAME "X_ROTATOR_CALIBRATE"
#define X_ROTATOR_CALIBRATE_ITEM_NAME     "CALIBRATE"

#define X_FAILURE_MESSAGE_PROPERTY          (PRIVATE_DATA->x_failure_message_property)
#define X_FAILURE_MESSAGE_ROTATOR_ITEM      (X_FAILURE_MESSAGE_PROPERTY->items + 0)
#define X_FAILURE_MESSAGE_SHUTTER_ITEM      (X_FAILURE_MESSAGE_PROPERTY->items + 1)

#define X_FAILURE_MESSAGE_PROPERTY_NAME     "X_FAILURE_MESSAGES"
#define X_FAILURE_MESSAGE_ROTATOR_ITEM_NAME "ROTATOR"
#define X_FAILURE_MESSAGE_SHUTTER_ITEM_NAME "SHUTTER"

#define X_CLEAR_FAILURE_PROPERTY       (PRIVATE_DATA->x_clear_failure_property)
#define X_CLEAR_FAILURE_ITEM           (X_CLEAR_FAILURE_PROPERTY->items + 0)

#define X_CLEAR_FAILURE_PROPERTY_NAME  "X_CLEAR_FAILURES"
#define X_CLEAR_FAILURE_ITEM_NAME      "CLEAR"

#define X_CONDITIONS_SAFETY_PROPERTY      (PRIVATE_DATA->x_conditions_safety_property)
#define X_SAFE_CW_ITEM                    (X_CONDITIONS_SAFETY_PROPERTY->items + 0)
#define X_SAFE_HYDREON_ITEM               (X_CONDITIONS_SAFETY_PROPERTY->items + 1)

#define X_CONDITIONS_SAFETY_PROPERTY_NAME "X_CONDITIONS_SAFETY"
#define X_SAFE_CW_ITEM_NAME               "CLOUD_WATCHER"
#define X_SAFE_HYDREON_ITEM_NAME          "HYDREON"

#pragma mark - Private data definition

typedef struct {
	indigo_uni_handle *handle;
	indigo_property *x_shutter_calibrate_property;
	indigo_property *x_rotator_calibrate_property;
	indigo_property *x_failure_message_property;
	indigo_property *x_clear_failure_property;
	indigo_property *x_conditions_safety_property;
	//+ data
	char request[128];
	char response[INDIGO_VALUE_SIZE];
	bool disconnection_queued;
	double current_position, target_position, park_position;
	int dome_status, prev_dome_status;
	int shutter_status, prev_shutter_status, shutter_target;
	int rotator_failure_code, shutter_failure_code;
	bool shutter_is_up, safety_known;
	bool park_requested, aborted, clear_requested;
	bool rotation_active, rotation_observed, rotation_error_at_start, home_active, shutter_active;
	bool rotator_calibration_active, shutter_calibration_active;
	double poll_hold_until;
	//- data
} beaver_private_data;

#pragma mark - Low level code

//+ code

typedef enum {
	BDB_ROTATOR_MOVING = 0,
	BDB_SHUTTER_MOVING = 1,
	BDB_ROTATOR_ERROR = 2,
	BDB_SHUTTER_ERROR = 3,
	BDB_COMM_ERROR = 4,
	BDB_UNSAFE_CW = 5,
	BDB_UNSAFE_HYDREON = 6
} beaver_status_bits_t;

typedef enum {
	BDS_NO_CALIBRATION = 0,
	BDS_CALIBRATING = 1,
	BDS_CALIBRATED = 2,
	BDS_CALIBRATION_ERROR = 3
} beaver_calibration_status_t;

typedef enum {
	BD_SHUTTER_OPEN = 0,
	BD_SHUTTER_CLOSED = 1,
	BD_SHUTTER_OPENING = 2,
	BD_SHUTTER_CLOSING = 3,
	BD_SHUTTER_ERROR = 4
} beaver_shutter_status_t;

static void dome_connection_handler(indigo_device *device);

static void beaver_network_disconnection(indigo_device *device) {
	if (CONNECTION_CONNECTED_ITEM->sw.value) {
		indigo_set_switch(CONNECTION_PROPERTY, CONNECTION_DISCONNECTED_ITEM, true);
		dome_connection_handler(device);
		// the alert state signals the unexpected disconnection
		CONNECTION_PROPERTY->state = INDIGO_ALERT_STATE;
		indigo_update_property(device, CONNECTION_PROPERTY, NULL);
		indigo_send_message(device, ALERT_PROPERTY, "Device disconnected unexpectedly");
	}
}

// sends a request and validates the reply "!<request without #>:<result>#"; returns the result or NULL
static char *beaver_vcommand(indigo_device *device, const char *format, va_list args) {
	char *request = PRIVATE_DATA->request, *response = PRIVATE_DATA->response;
	vsnprintf(request, sizeof(PRIVATE_DATA->request), format, args);
	long length = -1;
	if (indigo_uni_discard(PRIVATE_DATA->handle) >= 0 && indigo_uni_write(PRIVATE_DATA->handle, request, (long)strlen(request)) > 0) {
		length = indigo_uni_read_section2(PRIVATE_DATA->handle, response, sizeof(PRIVATE_DATA->response) - 1, "#", "", INDIGO_DELAY(BEAVER_FIRST_BYTE_TIMEOUT), INDIGO_DELAY(BEAVER_NEXT_BYTE_TIMEOUT));
	}
	if (length <= 0) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "No response to %s", request);
		if (PRIVATE_DATA->handle != NULL && PRIVATE_DATA->handle->type == INDIGO_TCP_HANDLE && !PRIVATE_DATA->disconnection_queued) {
			PRIVATE_DATA->disconnection_queued = true;
			indigo_execute_handler(device, beaver_network_disconnection);
			INDIGO_DRIVER_ERROR(DRIVER_NAME, "Unexpected disconnection from %s", DEVICE_PORT_ITEM->text.value);
		}
		return NULL;
	}
	indigo_usleep(5000);
	size_t prefix = strlen(request) - 1;
	if (response[length - 1] != '#' || strncmp(response, request, prefix) || response[prefix] != ':') {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "Invalid reply to %s: '%s'", request, response);
		return NULL;
	}
	response[length - 1] = 0;
	INDIGO_DRIVER_DEBUG(DRIVER_NAME, "%s -> %s", request, response);
	return response + prefix + 1;
}

static bool beaver_int(indigo_device *device, int *value, const char *format, ...) {
	va_list args;
	va_start(args, format);
	char *result = beaver_vcommand(device, format, args);
	va_end(args);
	if (result == NULL) {
		return false;
	}
	char *end;
	long parsed = strtol(result, &end, 10);
	if (end == result || *end) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "%s: invalid integer '%s'", PRIVATE_DATA->request, result);
		return false;
	}
	*value = (int)parsed;
	return true;
}

static bool beaver_float(indigo_device *device, double *value, const char *format, ...) {
	va_list args;
	va_start(args, format);
	char *result = beaver_vcommand(device, format, args);
	va_end(args);
	if (result == NULL) {
		return false;
	}
	char *end;
	double parsed = strtod(result, &end);
	if (end == result || *end || !isfinite(parsed)) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "%s: invalid number '%s'", PRIVATE_DATA->request, result);
		return false;
	}
	*value = parsed;
	return true;
}

static bool beaver_text(indigo_device *device, char *value, const char *format, ...) {
	va_list args;
	va_start(args, format);
	char *result = beaver_vcommand(device, format, args);
	va_end(args);
	if (result == NULL) {
		return false;
	}
	snprintf(value, INDIGO_VALUE_SIZE, "%s", result);
	return true;
}

// azimuth replies outside 0..360 are error codes
static bool beaver_get_azimuth(indigo_device *device, double *azimuth, const char *command) {
	double value;
	if (!beaver_float(device, &value, command) || value < 0 || value > 360) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "%s failed", command);
		return false;
	}
	*azimuth = value;
	return true;
}

static bool beaver_goto_azimuth(indigo_device *device, double azimuth) {
	int result = -1;
	// the azimuth is formatted as float like the original driver
	return beaver_int(device, &result, "!dome gotoaz %f#", (float)azimuth) && result == 0;
}

static bool beaver_open(indigo_device *device) {
	if (indigo_try_global_lock(device) != INDIGO_OK) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "indigo_try_global_lock(): failed to get lock");
		return false;
	}
	char *name = DEVICE_PORT_ITEM->text.value;
	if (!indigo_uni_is_url(name, "nexdome")) {
		PRIVATE_DATA->handle = indigo_uni_open_serial_with_speed(name, atoi(DEVICE_BAUDRATE_ITEM->text.value), INDIGO_LOG_DEBUG);
	} else {
		PRIVATE_DATA->handle = indigo_uni_open_url(name, BEAVER_NETWORK_PORT, INDIGO_TCP_HANDLE, INDIGO_LOG_DEBUG);
	}
	if (PRIVATE_DATA->handle == NULL) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "Opening device %s: failed", name);
		indigo_global_unlock(device);
		return false;
	}
	PRIVATE_DATA->disconnection_queued = false;
	static const char *models[] = { "Error", "Seletek", "Armadillo", "Platypus", "Dragonfly", "Limpet", "Lynx", "Beaver (rotator)", "Beaver (shutter)", "Error" };
	const char *message = "No response from the device";
	int version;
	if (beaver_int(device, &version, "!seletek version#") && version >= 0) {
		// OMFNN: operation mode, model, firmware major and minor
		int model = (version / 1000) % 10;
		if (model == 7) {
			INDIGO_COPY_VALUE(INFO_DEVICE_MODEL_ITEM->text.value, models[model]);
			snprintf(INFO_DEVICE_FW_REVISION_ITEM->text.value, INDIGO_VALUE_SIZE, "%d.%d", (version / 100) % 10, version % 100);
			indigo_update_property(device, INFO_PROPERTY, NULL);
			INDIGO_DRIVER_DEBUG(DRIVER_NAME, "!seletek version# -> %d = %s %s", version, models[model], INFO_DEVICE_FW_REVISION_ITEM->text.value);
			return true;
		}
		message = model == 8 ? "Beaver shutter controler found, this driver works with Beaver rotator" : "Connected device is not a Beaver dome controler";
	}
	INDIGO_DRIVER_ERROR(DRIVER_NAME, "%s", message);
	indigo_send_message(device, CONNECTION_PROPERTY, "%s", message);
	indigo_uni_close(&PRIVATE_DATA->handle);
	indigo_global_unlock(device);
	return false;
}

static void beaver_close(indigo_device *device) {
	indigo_uni_close(&PRIVATE_DATA->handle);
	indigo_global_unlock(device);
	INDIGO_DRIVER_LOG(DRIVER_NAME, "Disconnected from %s", DEVICE_PORT_ITEM->text.value);
	INDIGO_COPY_VALUE(INFO_DEVICE_MODEL_ITEM->text.value, DOME_DEVICE_NAME);
	INDIGO_COPY_VALUE(INFO_DEVICE_FW_REVISION_ITEM->text.value, "N/A");
	indigo_update_property(device, INFO_PROPERTY, NULL);
}

//- code

//+ dome.code

static void dome_horizontal_coordinates_handler(indigo_device *device);
static void dome_steps_handler(indigo_device *device);
static void dome_park_handler(indigo_device *device);
static void dome_home_handler(indigo_device *device);
static void dome_shutter_handler(indigo_device *device);
static void dome_x_rotator_calibrate_handler(indigo_device *device);
static void dome_x_shutter_calibrate_handler(indigo_device *device);

// the original handlers slept 0.5 s after a device command, so no status poll ran earlier
static void beaver_hold_poll(indigo_device *device) {
	PRIVATE_DATA->poll_hold_until = indigo_monotonic_time() + BEAVER_SETTLE_DELAY;
}

static void beaver_start_rotation(indigo_device *device) {
	PRIVATE_DATA->rotation_active = true;
	PRIVATE_DATA->rotation_error_at_start = CHECK_BIT(PRIVATE_DATA->dome_status, BDB_ROTATOR_ERROR);
	beaver_hold_poll(device);
}

// a closed shutter selects CLOSED, any other known state OPENED
static void beaver_update_shutter_switches(indigo_device *device) {
	if (PRIVATE_DATA->shutter_status == BD_SHUTTER_CLOSED) {
		indigo_set_switch(DOME_SHUTTER_PROPERTY, DOME_SHUTTER_CLOSED_ITEM, true);
	} else if (PRIVATE_DATA->shutter_status >= 0) {
		indigo_set_switch(DOME_SHUTTER_PROPERTY, DOME_SHUTTER_OPENED_ITEM, true);
	}
}

static void beaver_update_park_position(indigo_device *device) {
	double park;
	if (beaver_get_azimuth(device, &park, "!domerot getpark#")) {
		DOME_PARK_POSITION_AZ_ITEM->number.target = DOME_PARK_POSITION_AZ_ITEM->number.value = PRIVATE_DATA->park_position = park;
		DOME_PARK_POSITION_PROPERTY->state = INDIGO_OK_STATE;
		indigo_update_property(device, DOME_PARK_POSITION_PROPERTY, NULL);
	}
}

static bool beaver_rotation_queued(indigo_device *device) {
	return (DOME_HORIZONTAL_COORDINATES_PROPERTY->state == INDIGO_BUSY_STATE || DOME_STEPS_PROPERTY->state == INDIGO_BUSY_STATE) && !PRIVATE_DATA->rotation_active && !PRIVATE_DATA->rotation_observed;
}

static void dome_status_poll(indigo_device *device) {
	if (!IS_CONNECTED) {
		return;
	}
	double hold = PRIVATE_DATA->poll_hold_until - indigo_monotonic_time();
	if (hold > 0) {
		indigo_execute_handler_in(device, hold, dome_status_poll);
		return;
	}
	int value;
	bool status_read = beaver_int(device, &value, "!dome status#") && value >= 0;
	if (status_read) {
		PRIVATE_DATA->dome_status = value;
	} else {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "!dome status# failed");
	}
	beaver_get_azimuth(device, &PRIVATE_DATA->current_position, "!dome getaz#");
	int status = PRIVATE_DATA->dome_status, previous = PRIVATE_DATA->prev_dome_status;
	bool moving = CHECK_BIT(status, BDB_ROTATOR_MOVING);
	/* Handle observing conditions */
	if ((status_read && !PRIVATE_DATA->safety_known) || CHECK_BIT(status, BDB_UNSAFE_CW) != CHECK_BIT(previous, BDB_UNSAFE_CW) || CHECK_BIT(status, BDB_UNSAFE_HYDREON) != CHECK_BIT(previous, BDB_UNSAFE_HYDREON)) {
		PRIVATE_DATA->safety_known = true;
		X_CONDITIONS_SAFETY_PROPERTY->state = INDIGO_OK_STATE;
		X_SAFE_CW_ITEM->light.value = CHECK_BIT(status, BDB_UNSAFE_CW) ? INDIGO_ALERT_STATE : INDIGO_OK_STATE;
		X_SAFE_HYDREON_ITEM->light.value = CHECK_BIT(status, BDB_UNSAFE_HYDREON) ? INDIGO_ALERT_STATE : INDIGO_OK_STATE;
		if (CHECK_BIT(status, BDB_UNSAFE_CW) || CHECK_BIT(status, BDB_UNSAFE_HYDREON)) {
			indigo_update_property(device, X_CONDITIONS_SAFETY_PROPERTY, "Unsafe weather conditions reported, check X_CONDITIONS_SAFETY prperty");
		} else {
			indigo_update_property(device, X_CONDITIONS_SAFETY_PROPERTY, NULL);
		}
	}
	/* Handle dome rotation */
	if (PRIVATE_DATA->rotation_active || PRIVATE_DATA->rotation_observed || PRIVATE_DATA->park_requested || PRIVATE_DATA->home_active || status != previous) {
		// a property is BUSY without an active operation while its request waits in the queue; requests can arrive during the poll
		if (beaver_rotation_queued(device)) {
			INDIGO_DRIVER_DEBUG(DRIVER_NAME, "Rotation request queued");
		} else if (moving) {
			DOME_HORIZONTAL_COORDINATES_PROPERTY->state = INDIGO_BUSY_STATE;
			DOME_HORIZONTAL_COORDINATES_AZ_ITEM->number.value = PRIVATE_DATA->current_position;
			indigo_update_property(device, DOME_HORIZONTAL_COORDINATES_PROPERTY, NULL);
			indigo_update_property(device, DOME_STEPS_PROPERTY, NULL);
			PRIVATE_DATA->rotation_observed = true;
		} else if (PRIVATE_DATA->rotation_active && CHECK_BIT(status, BDB_ROTATOR_ERROR) && !PRIVATE_DATA->rotation_error_at_start && !PRIVATE_DATA->aborted) {
			INDIGO_DRIVER_ERROR(DRIVER_NAME, "Rotation stopped by rotator failure");
			DOME_HORIZONTAL_COORDINATES_AZ_ITEM->number.value = PRIVATE_DATA->current_position;
			DOME_HORIZONTAL_COORDINATES_PROPERTY->state = INDIGO_ALERT_STATE;
			indigo_update_property(device, DOME_HORIZONTAL_COORDINATES_PROPERTY, "Rotation stopped by rotator failure");
			DOME_STEPS_PROPERTY->state = INDIGO_ALERT_STATE;
			indigo_update_property(device, DOME_STEPS_PROPERTY, NULL);
			if (PRIVATE_DATA->park_requested) {
				PRIVATE_DATA->park_requested = false;
				indigo_set_switch(DOME_PARK_PROPERTY, DOME_PARK_UNPARKED_ITEM, true);
				DOME_PARK_PROPERTY->state = INDIGO_ALERT_STATE;
				indigo_update_property(device, DOME_PARK_PROPERTY, "Rotation stopped by rotator failure");
			}
			PRIVATE_DATA->rotation_active = PRIVATE_DATA->rotation_observed = false;
		} else {
			DOME_HORIZONTAL_COORDINATES_PROPERTY->state = INDIGO_OK_STATE;
			DOME_HORIZONTAL_COORDINATES_AZ_ITEM->number.value = PRIVATE_DATA->current_position;
			indigo_update_property(device, DOME_HORIZONTAL_COORDINATES_PROPERTY, NULL);
			DOME_STEPS_PROPERTY->state = INDIGO_OK_STATE;
			indigo_update_property(device, DOME_STEPS_PROPERTY, NULL);
			PRIVATE_DATA->rotation_active = PRIVATE_DATA->rotation_observed = false;
		}
		int atpark = 0;
		if (!beaver_int(device, &atpark, "!dome atpark#") || atpark < 0) {
			INDIGO_DRIVER_ERROR(DRIVER_NAME, "!dome atpark# failed");
			atpark = 0;
		}
		if (atpark && PRIVATE_DATA->park_requested) {
			DOME_PARK_PROPERTY->state = INDIGO_OK_STATE;
			PRIVATE_DATA->park_requested = false;
			indigo_set_switch(DOME_PARK_PROPERTY, DOME_PARK_PARKED_ITEM, true);
			indigo_update_property(device, DOME_PARK_PROPERTY, NULL);
			beaver_update_park_position(device);
		}
		int athome = 0;
		if (!beaver_int(device, &athome, "!dome athome#") || athome < 0) {
			INDIGO_DRIVER_ERROR(DRIVER_NAME, "!dome athome# failed");
			athome = 0;
		}
		if (PRIVATE_DATA->home_active) {
			if (athome) {
				PRIVATE_DATA->home_active = false;
				DOME_HOME_PROPERTY->state = INDIGO_OK_STATE;
				indigo_set_switch(DOME_HOME_PROPERTY, DOME_HOME_ITEM, true);
				indigo_update_property(device, DOME_HOME_PROPERTY, "Dome is at home position");
			} else if (!moving) {
				PRIVATE_DATA->home_active = false;
				DOME_HOME_PROPERTY->state = INDIGO_ALERT_STATE;
				indigo_set_switch(DOME_HOME_PROPERTY, DOME_HOME_ITEM, false);
				indigo_update_property(device, DOME_HOME_PROPERTY, "Failed to find home.");
			}
		} else if (DOME_HOME_PROPERTY->state != INDIGO_BUSY_STATE && (athome != 0) != DOME_HOME_ITEM->sw.value) {
			DOME_HOME_ITEM->sw.value = athome != 0;
			indigo_update_property(device, DOME_HOME_PROPERTY, NULL);
		}
		PRIVATE_DATA->prev_dome_status = status;
	}
	/* Handle dome rotator calibration */
	if (!moving && PRIVATE_DATA->rotator_calibration_active) {
		PRIVATE_DATA->rotator_calibration_active = false;
		X_ROTATOR_CALIBRATE_ITEM->sw.value = false;
		if (CHECK_BIT(status, BDB_ROTATOR_ERROR)) {
			X_ROTATOR_CALIBRATE_PROPERTY->state = INDIGO_ALERT_STATE;
			indigo_update_property(device, X_ROTATOR_CALIBRATE_PROPERTY, "Rotator calibration failed");
		} else {
			X_ROTATOR_CALIBRATE_PROPERTY->state = INDIGO_OK_STATE;
			indigo_update_property(device, X_ROTATOR_CALIBRATE_PROPERTY, "Rotator calibration complete");
		}
	}
	/* Handle dome shutter */
	int shutter = -1;
	if (!beaver_int(device, &shutter, "!dome shutterstatus#") || shutter < BD_SHUTTER_OPEN || shutter > BD_SHUTTER_ERROR) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "!dome shutterstatus# failed");
	} else {
		PRIVATE_DATA->shutter_status = shutter;
		bool shutter_queued = DOME_SHUTTER_PROPERTY->state == INDIGO_BUSY_STATE && !PRIVATE_DATA->shutter_active;
		// a request for the state the shutter is already in completes without a status change
		bool reached = PRIVATE_DATA->shutter_active && shutter == PRIVATE_DATA->shutter_target;
		if (!shutter_queued && (shutter != PRIVATE_DATA->prev_shutter_status || reached)) {
			if (shutter == BD_SHUTTER_OPEN) {
				PRIVATE_DATA->shutter_active = false;
				indigo_set_switch(DOME_SHUTTER_PROPERTY, DOME_SHUTTER_OPENED_ITEM, true);
				DOME_SHUTTER_PROPERTY->state = INDIGO_OK_STATE;
				indigo_update_property(device, DOME_SHUTTER_PROPERTY, "Shutter open");
			} else if (shutter == BD_SHUTTER_CLOSED) {
				PRIVATE_DATA->shutter_active = false;
				indigo_set_switch(DOME_SHUTTER_PROPERTY, DOME_SHUTTER_CLOSED_ITEM, true);
				DOME_SHUTTER_PROPERTY->state = INDIGO_OK_STATE;
				indigo_update_property(device, DOME_SHUTTER_PROPERTY, "Shutter closed");
			} else if (shutter == BD_SHUTTER_ERROR) {
				PRIVATE_DATA->shutter_active = false;
				indigo_set_switch(DOME_SHUTTER_PROPERTY, DOME_SHUTTER_OPENED_ITEM, true);
				DOME_SHUTTER_PROPERTY->state = INDIGO_ALERT_STATE;
				indigo_update_property(device, DOME_SHUTTER_PROPERTY, PRIVATE_DATA->aborted ? "Shutter aborted" : "Shutter error");
			} else if (shutter == BD_SHUTTER_OPENING) {
				indigo_set_switch(DOME_SHUTTER_PROPERTY, DOME_SHUTTER_OPENED_ITEM, true);
				DOME_SHUTTER_PROPERTY->state = INDIGO_BUSY_STATE;
				indigo_update_property(device, DOME_SHUTTER_PROPERTY, "Opening shutter...");
			} else {
				indigo_set_switch(DOME_SHUTTER_PROPERTY, DOME_SHUTTER_OPENED_ITEM, true);
				DOME_SHUTTER_PROPERTY->state = INDIGO_BUSY_STATE;
				indigo_update_property(device, DOME_SHUTTER_PROPERTY, "Closing shutter...");
			}
			PRIVATE_DATA->prev_shutter_status = shutter;
		}
	}
	/* Handle dome shutter calibration */
	if (PRIVATE_DATA->shutter_calibration_active) {
		X_SHUTTER_CALIBRATE_ITEM->sw.value = false;
		int calibration = -1;
		if (!beaver_int(device, &calibration, "!dome sendtoshutter \"shutter getcalibrationstatus\"#") || calibration < 0) {
			INDIGO_DRIVER_ERROR(DRIVER_NAME, "Shutter calibration status failed");
		} else if (calibration == BDS_CALIBRATION_ERROR) {
			PRIVATE_DATA->shutter_calibration_active = false;
			X_SHUTTER_CALIBRATE_PROPERTY->state = INDIGO_ALERT_STATE;
			indigo_update_property(device, X_SHUTTER_CALIBRATE_PROPERTY, "Shutter calibration failed");
		} else if (calibration == BDS_CALIBRATED || calibration == BDS_NO_CALIBRATION) {
			PRIVATE_DATA->shutter_calibration_active = false;
			X_SHUTTER_CALIBRATE_PROPERTY->state = INDIGO_OK_STATE;
			indigo_update_property(device, X_SHUTTER_CALIBRATE_PROPERTY, "Shutter calibration complete");
		}
	}
	/* Abort */
	if (PRIVATE_DATA->aborted) {
		PRIVATE_DATA->aborted = false;
		PRIVATE_DATA->prev_dome_status = status;
		if (!beaver_rotation_queued(device)) {
			DOME_HORIZONTAL_COORDINATES_PROPERTY->state = INDIGO_OK_STATE;
			DOME_HORIZONTAL_COORDINATES_AZ_ITEM->number.value = PRIVATE_DATA->target_position = PRIVATE_DATA->current_position;
			indigo_update_property(device, DOME_HORIZONTAL_COORDINATES_PROPERTY, NULL);
			DOME_STEPS_PROPERTY->state = INDIGO_OK_STATE;
			indigo_update_property(device, DOME_STEPS_PROPERTY, NULL);
			PRIVATE_DATA->rotation_active = PRIVATE_DATA->rotation_observed = false;
		}
	}
	/* Handle failures */
	if (PRIVATE_DATA->clear_requested) {
		PRIVATE_DATA->clear_requested = false;
		X_CLEAR_FAILURE_ITEM->sw.value = false;
		int rotator_result = -1, shutter_result = 0;
		bool responded = beaver_int(device, &rotator_result, "!seletek clearfailure#") && (!PRIVATE_DATA->shutter_is_up || beaver_int(device, &shutter_result, "!dome sendtoshutter \"seletek clearfailure\"#"));
		if (!responded || rotator_result < 0 || shutter_result < 0) {
			INDIGO_DRIVER_ERROR(DRIVER_NAME, "Clear failure failed");
			X_CLEAR_FAILURE_PROPERTY->state = INDIGO_ALERT_STATE;
		} else {
			X_CLEAR_FAILURE_PROPERTY->state = INDIGO_OK_STATE;
		}
		indigo_update_property(device, X_CLEAR_FAILURE_PROPERTY, NULL);
	}
	int rotator_code = 0, shutter_code = 0;
	if (!beaver_int(device, &rotator_code, "!seletek getfailurecode#") || (PRIVATE_DATA->shutter_is_up && !beaver_int(device, &shutter_code, "!dome sendtoshutter \"seletek getfailurecode\"#"))) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "Failure codes failed");
	} else if (rotator_code != PRIVATE_DATA->rotator_failure_code || shutter_code != PRIVATE_DATA->shutter_failure_code) {
		char message[INDIGO_VALUE_SIZE];
		if (beaver_text(device, message, "!seletek getfailuremsg#")) {
			INDIGO_COPY_VALUE(X_FAILURE_MESSAGE_ROTATOR_ITEM->text.value, message);
			if (PRIVATE_DATA->shutter_is_up && beaver_text(device, message, "!dome sendtoshutter \"seletek getfailuremsg\"#")) {
				INDIGO_COPY_VALUE(X_FAILURE_MESSAGE_SHUTTER_ITEM->text.value, message);
			}
		}
		if (rotator_code != 0 || shutter_code != 0) {
			X_FAILURE_MESSAGE_PROPERTY->state = INDIGO_ALERT_STATE;
			indigo_update_property(device, X_FAILURE_MESSAGE_PROPERTY, "Rotator or Shutter failure detected, check X_FAILURE_MESSAGES property");
		} else {
			X_FAILURE_MESSAGE_PROPERTY->state = INDIGO_OK_STATE;
			indigo_update_property(device, X_FAILURE_MESSAGE_PROPERTY, NULL);
		}
		PRIVATE_DATA->rotator_failure_code = rotator_code;
		PRIVATE_DATA->shutter_failure_code = shutter_code;
	}
	indigo_execute_handler_in(device, BEAVER_POLL_DELAY, dome_status_poll);
}

//- dome.code

#pragma mark - High level code (dome)

static void dome_connection_handler(indigo_device *device) {
	if (CONNECTION_CONNECTED_ITEM->sw.value) {
		bool connection_result = true;
		connection_result = beaver_open(device);
		if (connection_result) {
			//+ dome.on_connect
			int shutter_is_up = 0;
			if (!beaver_int(device, &shutter_is_up, "!dome shutterisup#")) {
				INDIGO_DRIVER_ERROR(DRIVER_NAME, "!dome shutterisup# failed");
				shutter_is_up = 0;
			}
			PRIVATE_DATA->shutter_is_up = shutter_is_up != 0;
			if (!PRIVATE_DATA->shutter_is_up) {
				indigo_send_message(device, ALERT_PROPERTY, "Shutter not detected");
			}
			DOME_SHUTTER_PROPERTY->hidden = X_SHUTTER_CALIBRATE_PROPERTY->hidden = !PRIVATE_DATA->shutter_is_up;
			PRIVATE_DATA->prev_shutter_status = PRIVATE_DATA->prev_dome_status = PRIVATE_DATA->shutter_status = -1;
			PRIVATE_DATA->safety_known = PRIVATE_DATA->aborted = PRIVATE_DATA->clear_requested = false;
			PRIVATE_DATA->rotation_active = PRIVATE_DATA->rotation_observed = PRIVATE_DATA->home_active = PRIVATE_DATA->shutter_active = false;
			PRIVATE_DATA->rotator_calibration_active = PRIVATE_DATA->shutter_calibration_active = false;
			PRIVATE_DATA->poll_hold_until = 0;
			beaver_get_azimuth(device, &PRIVATE_DATA->current_position, "!dome getaz#");
			DOME_HORIZONTAL_COORDINATES_AZ_ITEM->number.value = DOME_HORIZONTAL_COORDINATES_AZ_ITEM->number.target = PRIVATE_DATA->target_position = PRIVATE_DATA->current_position;
			int atpark = 0;
			if (!beaver_int(device, &atpark, "!dome atpark#") || atpark < 0) {
				INDIGO_DRIVER_ERROR(DRIVER_NAME, "!dome atpark# failed");
				atpark = 0;
			}
			indigo_set_switch(DOME_PARK_PROPERTY, atpark ? DOME_PARK_PARKED_ITEM : DOME_PARK_UNPARKED_ITEM, true);
			DOME_PARK_PROPERTY->state = INDIGO_OK_STATE;
			PRIVATE_DATA->park_requested = false;
			indigo_update_property(device, DOME_PARK_PROPERTY, NULL);
			beaver_update_park_position(device);
			indigo_execute_handler_in(device, BEAVER_FIRST_POLL_DELAY, dome_status_poll);
			//- dome.on_connect
		}
		if (connection_result) {
			indigo_define_property(device, X_SHUTTER_CALIBRATE_PROPERTY, NULL);
			indigo_define_property(device, X_ROTATOR_CALIBRATE_PROPERTY, NULL);
			indigo_define_property(device, X_FAILURE_MESSAGE_PROPERTY, NULL);
			indigo_define_property(device, X_CLEAR_FAILURE_PROPERTY, NULL);
			indigo_define_property(device, X_CONDITIONS_SAFETY_PROPERTY, NULL);
			CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
			indigo_send_message(device, OK_PROPERTY, "Connected to %s on %s", DOME_DEVICE_NAME, DEVICE_PORT_ITEM->text.value);
		} else {
			indigo_send_message(device, ALERT_PROPERTY, "Failed to connect to %s on %s", DOME_DEVICE_NAME, DEVICE_PORT_ITEM->text.value);
			CONNECTION_PROPERTY->state = INDIGO_ALERT_STATE;
			indigo_set_switch(CONNECTION_PROPERTY, CONNECTION_DISCONNECTED_ITEM, true);
		}
	} else {
		indigo_cancel_pending_handlers(device);
		indigo_delete_property(device, X_SHUTTER_CALIBRATE_PROPERTY, NULL);
		indigo_delete_property(device, X_ROTATOR_CALIBRATE_PROPERTY, NULL);
		indigo_delete_property(device, X_FAILURE_MESSAGE_PROPERTY, NULL);
		indigo_delete_property(device, X_CLEAR_FAILURE_PROPERTY, NULL);
		indigo_delete_property(device, X_CONDITIONS_SAFETY_PROPERTY, NULL);
		beaver_close(device);
		indigo_send_message(device, OK_PROPERTY, "Disconnected from %s", device->name);
		CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
	}
	indigo_dome_change_property(device, NULL, CONNECTION_PROPERTY);
}

static void dome_horizontal_coordinates_handler(indigo_device *device) {
	//+ dome.DOME_HORIZONTAL_COORDINATES.on_change
	DOME_HORIZONTAL_COORDINATES_PROPERTY->state = INDIGO_BUSY_STATE;
	// keep the framework BUSY guard closed while the device command runs
	if (DOME_PARK_PARKED_ITEM->sw.value) {
		beaver_get_azimuth(device, &PRIVATE_DATA->current_position, "!dome getaz#");
		DOME_HORIZONTAL_COORDINATES_AZ_ITEM->number.value = PRIVATE_DATA->current_position;
		DOME_HORIZONTAL_COORDINATES_PROPERTY->state = INDIGO_ALERT_STATE;
		DOME_STEPS_PROPERTY->state = INDIGO_ALERT_STATE;
		indigo_update_property(device, DOME_STEPS_PROPERTY, NULL);
		indigo_update_property(device, DOME_HORIZONTAL_COORDINATES_PROPERTY, "Dome is parked, please unpark");
		return;
	}
	DOME_HORIZONTAL_COORDINATES_AZ_ITEM->number.value = PRIVATE_DATA->current_position;
	DOME_STEPS_PROPERTY->state = INDIGO_BUSY_STATE;
	indigo_update_property(device, DOME_STEPS_PROPERTY, NULL);
	double target = DOME_HORIZONTAL_COORDINATES_AZ_ITEM->number.target;
	if (DOME_ON_COORDINATES_SET_SYNC_ITEM->sw.value) {
		int result = -1;
		if (!beaver_int(device, &result, "!dome setaz %f#", (float)target) || result != 0) {
			DOME_HORIZONTAL_COORDINATES_PROPERTY->state = INDIGO_ALERT_STATE;
			DOME_STEPS_PROPERTY->state = INDIGO_ALERT_STATE;
			indigo_update_property(device, DOME_STEPS_PROPERTY, NULL);
			indigo_update_property(device, DOME_HORIZONTAL_COORDINATES_PROPERTY, "Set azimuth failed");
			return;
		}
		// the result of saving the settings is not checked
		beaver_int(device, &result, "!seletek savefs#");
	} else if (!beaver_goto_azimuth(device, target)) {
		DOME_HORIZONTAL_COORDINATES_PROPERTY->state = INDIGO_ALERT_STATE;
		DOME_STEPS_PROPERTY->state = INDIGO_ALERT_STATE;
		indigo_update_property(device, DOME_STEPS_PROPERTY, NULL);
		indigo_update_property(device, DOME_HORIZONTAL_COORDINATES_PROPERTY, "Goto azimuth failed");
		return;
	}
	PRIVATE_DATA->target_position = target;
	beaver_start_rotation(device);
	//- dome.DOME_HORIZONTAL_COORDINATES.on_change
	indigo_update_property(device, DOME_HORIZONTAL_COORDINATES_PROPERTY, NULL);
}

static void dome_steps_handler(indigo_device *device) {
	//+ dome.DOME_STEPS.on_change
	DOME_STEPS_PROPERTY->state = INDIGO_BUSY_STATE;
	// keep the framework BUSY guard closed while the device commands run
	if (DOME_PARK_PARKED_ITEM->sw.value) {
		DOME_STEPS_PROPERTY->state = INDIGO_ALERT_STATE;
		indigo_update_property(device, DOME_STEPS_PROPERTY, "Dome is parked, please unpark");
		return;
	}
	beaver_get_azimuth(device, &PRIVATE_DATA->current_position, "!dome getaz#");
	DOME_HORIZONTAL_COORDINATES_PROPERTY->state = INDIGO_BUSY_STATE;
	DOME_HORIZONTAL_COORDINATES_AZ_ITEM->number.value = PRIVATE_DATA->current_position;
	indigo_update_property(device, DOME_HORIZONTAL_COORDINATES_PROPERTY, NULL);
	// integer tenths of degree avoid truncating a floating-point sum
	long current = lround(PRIVATE_DATA->current_position * 10), steps = lround(DOME_STEPS_ITEM->number.value * 10);
	double target = PRIVATE_DATA->target_position;
	if (DOME_DIRECTION_MOVE_COUNTERCLOCKWISE_ITEM->sw.value) {
		target = ((current - steps) % 3600 + 3600) % 3600 / 10.0;
	} else if (DOME_DIRECTION_MOVE_CLOCKWISE_ITEM->sw.value) {
		target = ((current + steps) % 3600 + 3600) % 3600 / 10.0;
	}
	INDIGO_DRIVER_DEBUG(DRIVER_NAME, "target_position = %.1f", target);
	if (!beaver_goto_azimuth(device, target)) {
		DOME_HORIZONTAL_COORDINATES_PROPERTY->state = INDIGO_ALERT_STATE;
		DOME_HORIZONTAL_COORDINATES_AZ_ITEM->number.value = PRIVATE_DATA->current_position;
		indigo_update_property(device, DOME_HORIZONTAL_COORDINATES_PROPERTY, NULL);
		DOME_STEPS_PROPERTY->state = INDIGO_ALERT_STATE;
		indigo_update_property(device, DOME_STEPS_PROPERTY, "Goto azimuth failed");
		return;
	}
	PRIVATE_DATA->target_position = target;
	beaver_start_rotation(device);
	//- dome.DOME_STEPS.on_change
	indigo_update_property(device, DOME_STEPS_PROPERTY, NULL);
}

static void dome_park_handler(indigo_device *device) {
	//+ dome.DOME_PARK.on_change
	DOME_PARK_PROPERTY->state = INDIGO_BUSY_STATE;
	// keep the framework BUSY guard closed while the device command runs
	if (DOME_PARK_UNPARKED_ITEM->sw.value) {
		DOME_PARK_PROPERTY->state = INDIGO_OK_STATE;
		PRIVATE_DATA->park_requested = false;
	} else {
		indigo_set_switch(DOME_PARK_PROPERTY, DOME_PARK_UNPARKED_ITEM, true);
		DOME_STEPS_PROPERTY->state = INDIGO_BUSY_STATE;
		DOME_HORIZONTAL_COORDINATES_PROPERTY->state = INDIGO_BUSY_STATE;
		indigo_update_property(device, DOME_STEPS_PROPERTY, NULL);
		indigo_update_property(device, DOME_HORIZONTAL_COORDINATES_PROPERTY, NULL);
		int result = -1;
		if (!beaver_int(device, &result, "!dome gopark#") || result < 0) {
			INDIGO_DRIVER_ERROR(DRIVER_NAME, "!dome gopark# failed");
			DOME_STEPS_PROPERTY->state = INDIGO_OK_STATE;
			DOME_HORIZONTAL_COORDINATES_PROPERTY->state = INDIGO_OK_STATE;
			indigo_update_property(device, DOME_STEPS_PROPERTY, NULL);
			indigo_update_property(device, DOME_HORIZONTAL_COORDINATES_PROPERTY, NULL);
			DOME_PARK_PROPERTY->state = INDIGO_ALERT_STATE;
			indigo_update_property(device, DOME_PARK_PROPERTY, "Goto park failed");
			return;
		}
		PRIVATE_DATA->park_requested = true;
		beaver_start_rotation(device);
	}
	//- dome.DOME_PARK.on_change
	indigo_update_property(device, DOME_PARK_PROPERTY, NULL);
}

static void dome_park_position_handler(indigo_device *device) {
	//+ dome.DOME_PARK_POSITION.on_change
	DOME_PARK_POSITION_PROPERTY->state = INDIGO_BUSY_STATE;
	// the controller makes the current azimuth the park position; the requested value is not used
	DOME_PARK_POSITION_AZ_ITEM->number.target = DOME_PARK_POSITION_AZ_ITEM->number.value = PRIVATE_DATA->park_position;
	int result = -1;
	if (!beaver_int(device, &result, "!dome setpark#") || result < 0) {
		DOME_PARK_POSITION_PROPERTY->state = INDIGO_ALERT_STATE;
		indigo_update_property(device, DOME_PARK_POSITION_PROPERTY, "Failed to set current position to park position");
		return;
	}
	// the result of saving the settings is not checked
	beaver_int(device, &result, "!seletek savefs#");
	double park;
	if (!beaver_get_azimuth(device, &park, "!domerot getpark#")) {
		DOME_PARK_POSITION_PROPERTY->state = INDIGO_ALERT_STATE;
		indigo_update_property(device, DOME_PARK_POSITION_PROPERTY, "Failed to set current position to park position");
		return;
	}
	DOME_PARK_POSITION_AZ_ITEM->number.target = DOME_PARK_POSITION_AZ_ITEM->number.value = PRIVATE_DATA->park_position = park;
	DOME_PARK_POSITION_PROPERTY->state = INDIGO_OK_STATE;
	//- dome.DOME_PARK_POSITION.on_change
	indigo_update_property(device, DOME_PARK_POSITION_PROPERTY, NULL);
}

static void dome_home_handler(indigo_device *device) {
	//+ dome.DOME_HOME.on_change
	DOME_HOME_PROPERTY->state = INDIGO_BUSY_STATE;
	// keep the framework BUSY guard closed while the device command runs
	if (DOME_PARK_PARKED_ITEM->sw.value) {
		DOME_HOME_ITEM->sw.value = false;
		DOME_HOME_PROPERTY->state = INDIGO_ALERT_STATE;
		indigo_update_property(device, DOME_HOME_PROPERTY, "Dome is parked, please unpark");
		return;
	}
	if (!DOME_HOME_ITEM->sw.value) {
		DOME_HOME_PROPERTY->state = INDIGO_OK_STATE;
	} else {
		DOME_HOME_ITEM->sw.value = false;
		indigo_update_property(device, DOME_HOME_PROPERTY, "Dome going to home position...");
		DOME_STEPS_PROPERTY->state = INDIGO_BUSY_STATE;
		indigo_update_property(device, DOME_STEPS_PROPERTY, NULL);
		DOME_HORIZONTAL_COORDINATES_PROPERTY->state = INDIGO_BUSY_STATE;
		indigo_update_property(device, DOME_HORIZONTAL_COORDINATES_PROPERTY, NULL);
		int result = -1;
		if (!beaver_int(device, &result, "!dome gohome#") || result < 0) {
			INDIGO_DRIVER_ERROR(DRIVER_NAME, "!dome gohome# failed");
			DOME_STEPS_PROPERTY->state = INDIGO_OK_STATE;
			DOME_HORIZONTAL_COORDINATES_PROPERTY->state = INDIGO_OK_STATE;
			indigo_update_property(device, DOME_STEPS_PROPERTY, NULL);
			indigo_update_property(device, DOME_HORIZONTAL_COORDINATES_PROPERTY, NULL);
			DOME_HOME_PROPERTY->state = INDIGO_ALERT_STATE;
			indigo_update_property(device, DOME_HOME_PROPERTY, "Failed to find home.");
			return;
		}
		PRIVATE_DATA->home_active = true;
		beaver_start_rotation(device);
	}
	//- dome.DOME_HOME.on_change
	indigo_update_property(device, DOME_HOME_PROPERTY, NULL);
}

static void dome_abort_motion_handler(indigo_device *device) {
	DOME_ABORT_MOTION_PROPERTY->state = INDIGO_OK_STATE;
	//+ dome.DOME_ABORT_MOTION.on_change
	// urgent abort can overtake queued requests: settle the properties they left BUSY
	indigo_cancel_pending_handler(device, dome_horizontal_coordinates_handler);
	indigo_cancel_pending_handler(device, dome_steps_handler);
	indigo_cancel_pending_handler(device, dome_park_handler);
	indigo_cancel_pending_handler(device, dome_home_handler);
	indigo_cancel_pending_handler(device, dome_shutter_handler);
	indigo_cancel_pending_handler(device, dome_x_rotator_calibrate_handler);
	indigo_cancel_pending_handler(device, dome_x_shutter_calibrate_handler);
	if (!PRIVATE_DATA->rotation_active && !PRIVATE_DATA->rotation_observed && (DOME_HORIZONTAL_COORDINATES_PROPERTY->state == INDIGO_BUSY_STATE || DOME_STEPS_PROPERTY->state == INDIGO_BUSY_STATE)) {
		DOME_HORIZONTAL_COORDINATES_AZ_ITEM->number.value = DOME_HORIZONTAL_COORDINATES_AZ_ITEM->number.target = PRIVATE_DATA->current_position;
		INDIGO_UPDATE_PROPERTY_STATE(DOME_HORIZONTAL_COORDINATES_PROPERTY, INDIGO_OK_STATE, NULL);
		INDIGO_UPDATE_PROPERTY_STATE(DOME_STEPS_PROPERTY, INDIGO_OK_STATE, NULL);
	}
	if (!PRIVATE_DATA->home_active && DOME_HOME_PROPERTY->state == INDIGO_BUSY_STATE) {
		DOME_HOME_ITEM->sw.value = false;
		INDIGO_UPDATE_PROPERTY_STATE(DOME_HOME_PROPERTY, INDIGO_OK_STATE, NULL);
	}
	if (!PRIVATE_DATA->shutter_active && DOME_SHUTTER_PROPERTY->state == INDIGO_BUSY_STATE) {
		beaver_update_shutter_switches(device);
		INDIGO_UPDATE_PROPERTY_STATE(DOME_SHUTTER_PROPERTY, INDIGO_OK_STATE, NULL);
	}
	if (!PRIVATE_DATA->rotator_calibration_active && X_ROTATOR_CALIBRATE_PROPERTY->state == INDIGO_BUSY_STATE) {
		X_ROTATOR_CALIBRATE_ITEM->sw.value = false;
		INDIGO_UPDATE_PROPERTY_STATE(X_ROTATOR_CALIBRATE_PROPERTY, INDIGO_OK_STATE, NULL);
	}
	if (!PRIVATE_DATA->shutter_calibration_active && X_SHUTTER_CALIBRATE_PROPERTY->state == INDIGO_BUSY_STATE) {
		X_SHUTTER_CALIBRATE_ITEM->sw.value = false;
		INDIGO_UPDATE_PROPERTY_STATE(X_SHUTTER_CALIBRATE_PROPERTY, INDIGO_OK_STATE, NULL);
	}
	int result = -1;
	if (!beaver_int(device, &result, "!dome abort 1#") || result != 0) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "!dome abort 1# failed");
		if (!PRIVATE_DATA->park_requested && DOME_PARK_PROPERTY->state == INDIGO_BUSY_STATE) {
			indigo_set_switch(DOME_PARK_PROPERTY, DOME_PARK_UNPARKED_ITEM, true);
			INDIGO_UPDATE_PROPERTY_STATE(DOME_PARK_PROPERTY, INDIGO_ALERT_STATE, NULL);
		}
		DOME_ABORT_MOTION_PROPERTY->state = INDIGO_ALERT_STATE;
		DOME_ABORT_MOTION_ITEM->sw.value = false;
		indigo_update_property(device, DOME_ABORT_MOTION_PROPERTY, "Abort failed");
		return;
	}
	PRIVATE_DATA->aborted = true;
	PRIVATE_DATA->park_requested = false;
	if (DOME_ABORT_MOTION_ITEM->sw.value && DOME_PARK_PROPERTY->state == INDIGO_BUSY_STATE) {
		indigo_set_switch(DOME_PARK_PROPERTY, DOME_PARK_UNPARKED_ITEM, true);
		DOME_PARK_PROPERTY->state = INDIGO_ALERT_STATE;
		indigo_update_property(device, DOME_PARK_PROPERTY, NULL);
	}
	if (PRIVATE_DATA->rotator_calibration_active) {
		PRIVATE_DATA->rotator_calibration_active = false;
		X_ROTATOR_CALIBRATE_ITEM->sw.value = false;
		X_ROTATOR_CALIBRATE_PROPERTY->state = INDIGO_ALERT_STATE;
		indigo_update_property(device, X_ROTATOR_CALIBRATE_PROPERTY, "Rotator calibration aborted");
	}
	if (PRIVATE_DATA->shutter_calibration_active) {
		PRIVATE_DATA->shutter_calibration_active = false;
		X_SHUTTER_CALIBRATE_ITEM->sw.value = false;
		X_SHUTTER_CALIBRATE_PROPERTY->state = INDIGO_ALERT_STATE;
		indigo_update_property(device, X_SHUTTER_CALIBRATE_PROPERTY, "Shutter calibration aborted");
	}
	PRIVATE_DATA->target_position = PRIVATE_DATA->current_position;
	PRIVATE_DATA->shutter_active = false;
	DOME_SHUTTER_PROPERTY->state = INDIGO_OK_STATE;
	indigo_update_property(device, DOME_SHUTTER_PROPERTY, NULL);
	DOME_ABORT_MOTION_ITEM->sw.value = false;
	//- dome.DOME_ABORT_MOTION.on_change
	indigo_update_property(device, DOME_ABORT_MOTION_PROPERTY, NULL);
}

static void dome_shutter_handler(indigo_device *device) {
	//+ dome.DOME_SHUTTER.on_change
	DOME_SHUTTER_PROPERTY->state = INDIGO_BUSY_STATE;
	// keep the framework BUSY guard closed while the device command runs
	bool open = DOME_SHUTTER_OPENED_ITEM->sw.value;
	int result = -1;
	if (!beaver_int(device, &result, "!dome %s#", open ? "openshutter" : "closeshutter") || result != 0) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "Shutter open/close failed");
		beaver_update_shutter_switches(device);
		DOME_SHUTTER_PROPERTY->state = INDIGO_ALERT_STATE;
		indigo_update_property(device, DOME_SHUTTER_PROPERTY, "Shutter open/close failed");
		return;
	}
	PRIVATE_DATA->shutter_active = true;
	PRIVATE_DATA->shutter_target = open ? BD_SHUTTER_OPEN : BD_SHUTTER_CLOSED;
	beaver_hold_poll(device);
	//- dome.DOME_SHUTTER.on_change
	indigo_update_property(device, DOME_SHUTTER_PROPERTY, NULL);
}

static void dome_x_shutter_calibrate_handler(indigo_device *device) {
	//+ dome.X_SHUTTER_CALIBRATE.on_change
	X_SHUTTER_CALIBRATE_PROPERTY->state = INDIGO_BUSY_STATE;
	// keep the framework BUSY guard closed while the device command runs
	if (!X_SHUTTER_CALIBRATE_ITEM->sw.value) {
		X_SHUTTER_CALIBRATE_PROPERTY->state = INDIGO_OK_STATE;
	} else {
		int result = -1;
		if (!beaver_int(device, &result, "!dome autocalshutter#") || result < 0) {
			X_SHUTTER_CALIBRATE_PROPERTY->state = INDIGO_ALERT_STATE;
			indigo_update_property(device, X_SHUTTER_CALIBRATE_PROPERTY, "Shutter calibration falied");
			return;
		}
		PRIVATE_DATA->shutter_calibration_active = true;
		beaver_hold_poll(device);
		indigo_send_message(device, X_SHUTTER_CALIBRATE_PROPERTY, "Calibrating shutter...");
	}
	//- dome.X_SHUTTER_CALIBRATE.on_change
	indigo_update_property(device, X_SHUTTER_CALIBRATE_PROPERTY, NULL);
}

static void dome_x_rotator_calibrate_handler(indigo_device *device) {
	//+ dome.X_ROTATOR_CALIBRATE.on_change
	X_ROTATOR_CALIBRATE_PROPERTY->state = INDIGO_BUSY_STATE;
	// keep the framework BUSY guard closed while the device command runs
	if (DOME_PARK_PARKED_ITEM->sw.value) {
		X_ROTATOR_CALIBRATE_ITEM->sw.value = false;
		X_ROTATOR_CALIBRATE_PROPERTY->state = INDIGO_ALERT_STATE;
		indigo_update_property(device, X_ROTATOR_CALIBRATE_PROPERTY, "Dome is parked, please unpark");
		return;
	}
	if (!X_ROTATOR_CALIBRATE_ITEM->sw.value) {
		X_ROTATOR_CALIBRATE_PROPERTY->state = INDIGO_OK_STATE;
	} else {
		int result = -1;
		if (!beaver_int(device, &result, "!dome autocalrot 2#") || result < 0) {
			X_ROTATOR_CALIBRATE_PROPERTY->state = INDIGO_ALERT_STATE;
			indigo_update_property(device, X_ROTATOR_CALIBRATE_PROPERTY, "Rotator calibration falied");
			return;
		}
		PRIVATE_DATA->rotator_calibration_active = true;
		beaver_hold_poll(device);
		indigo_send_message(device, X_ROTATOR_CALIBRATE_PROPERTY, "Calibrating rotator...");
	}
	//- dome.X_ROTATOR_CALIBRATE.on_change
	indigo_update_property(device, X_ROTATOR_CALIBRATE_PROPERTY, NULL);
}

static void dome_x_clear_failure_handler(indigo_device *device) {
	//+ dome.X_CLEAR_FAILURE.on_change
	X_CLEAR_FAILURE_PROPERTY->state = INDIGO_BUSY_STATE;
	// the next status poll clears the failures before it reads the failure codes
	if (X_CLEAR_FAILURE_ITEM->sw.value) {
		PRIVATE_DATA->clear_requested = true;
	} else {
		X_CLEAR_FAILURE_PROPERTY->state = INDIGO_OK_STATE;
	}
	//- dome.X_CLEAR_FAILURE.on_change
	indigo_update_property(device, X_CLEAR_FAILURE_PROPERTY, NULL);
}

#pragma mark - Device API (dome)

static indigo_result dome_enumerate_properties(indigo_device *device, indigo_client *client, indigo_property *property);

static indigo_result dome_attach(indigo_device *device) {
	if (indigo_dome_attach(device, DRIVER_NAME, DRIVER_VERSION) == INDIGO_OK) {
		ADDITIONAL_INSTANCES_PROPERTY->hidden = device->base_device != NULL;
		DEVICE_PORT_PROPERTY->hidden = false;
		DEVICE_PORTS_PROPERTY->hidden = false;
		indigo_enumerate_serial_ports(device, DEVICE_PORTS_PROPERTY);
		//+ dome.on_attach
		INDIGO_COPY_VALUE(DEVICE_BAUDRATE_ITEM->text.value, BEAVER_BAUDRATE);
		INFO_PROPERTY->count = 6;
		//- dome.on_attach
		DOME_SPEED_PROPERTY->hidden = true;
		DOME_ON_COORDINATES_SET_PROPERTY->hidden = false;
		//+ dome.DOME_ON_COORDINATES_SET.on_attach
		DOME_ON_COORDINATES_SET_PROPERTY->count = 2;
		//- dome.DOME_ON_COORDINATES_SET.on_attach
		DOME_SLAVING_PARAMETERS_PROPERTY->hidden = false;
		DOME_HORIZONTAL_COORDINATES_PROPERTY->hidden = false;
		//+ dome.DOME_HORIZONTAL_COORDINATES.on_attach
		DOME_HORIZONTAL_COORDINATES_PROPERTY->perm = INDIGO_RW_PERM;
		//- dome.DOME_HORIZONTAL_COORDINATES.on_attach
		DOME_STEPS_PROPERTY->hidden = false;
		DOME_PARK_PROPERTY->hidden = false;
		DOME_PARK_POSITION_PROPERTY->hidden = false;
		DOME_HOME_PROPERTY->hidden = false;
		DOME_ABORT_MOTION_PROPERTY->hidden = false;
		DOME_SHUTTER_PROPERTY->hidden = false;
		X_SHUTTER_CALIBRATE_PROPERTY = indigo_init_switch_property(NULL, device->name, X_SHUTTER_CALIBRATE_PROPERTY_NAME, "Misc", "Calibrate shutter", INDIGO_OK_STATE, INDIGO_RW_PERM, INDIGO_AT_MOST_ONE_RULE, 1);
		if (X_SHUTTER_CALIBRATE_PROPERTY == NULL) {
			return INDIGO_FAILED;
		}
		indigo_init_switch_item(X_SHUTTER_CALIBRATE_ITEM, X_SHUTTER_CALIBRATE_ITEM_NAME, "Calibrate", false);
		X_ROTATOR_CALIBRATE_PROPERTY = indigo_init_switch_property(NULL, device->name, X_ROTATOR_CALIBRATE_PROPERTY_NAME, "Misc", "Calibrate rotator", INDIGO_OK_STATE, INDIGO_RW_PERM, INDIGO_AT_MOST_ONE_RULE, 1);
		if (X_ROTATOR_CALIBRATE_PROPERTY == NULL) {
			return INDIGO_FAILED;
		}
		indigo_init_switch_item(X_ROTATOR_CALIBRATE_ITEM, X_ROTATOR_CALIBRATE_ITEM_NAME, "Calibrate", false);
		X_FAILURE_MESSAGE_PROPERTY = indigo_init_text_property(NULL, device->name, X_FAILURE_MESSAGE_PROPERTY_NAME, "Misc", "Last failures", INDIGO_OK_STATE, INDIGO_RO_PERM, 2);
		if (X_FAILURE_MESSAGE_PROPERTY == NULL) {
			return INDIGO_FAILED;
		}
		indigo_init_text_item(X_FAILURE_MESSAGE_ROTATOR_ITEM, X_FAILURE_MESSAGE_ROTATOR_ITEM_NAME, "Rotator message", "");
		indigo_init_text_item(X_FAILURE_MESSAGE_SHUTTER_ITEM, X_FAILURE_MESSAGE_SHUTTER_ITEM_NAME, "Shutter message", "");
		X_CLEAR_FAILURE_PROPERTY = indigo_init_switch_property(NULL, device->name, X_CLEAR_FAILURE_PROPERTY_NAME, "Misc", "Clear last failures", INDIGO_OK_STATE, INDIGO_RW_PERM, INDIGO_AT_MOST_ONE_RULE, 1);
		if (X_CLEAR_FAILURE_PROPERTY == NULL) {
			return INDIGO_FAILED;
		}
		indigo_init_switch_item(X_CLEAR_FAILURE_ITEM, X_CLEAR_FAILURE_ITEM_NAME, "Clear", false);
		X_CONDITIONS_SAFETY_PROPERTY = indigo_init_light_property(NULL, device->name, X_CONDITIONS_SAFETY_PROPERTY_NAME, "Misc", "Observing conditions safety", INDIGO_OK_STATE, 2);
		if (X_CONDITIONS_SAFETY_PROPERTY == NULL) {
			return INDIGO_FAILED;
		}
		indigo_init_light_item(X_SAFE_CW_ITEM, X_SAFE_CW_ITEM_NAME, "Safe by Cloud Wacher", INDIGO_IDLE_STATE);
		indigo_init_light_item(X_SAFE_HYDREON_ITEM, X_SAFE_HYDREON_ITEM_NAME, "Safe by Hydreon RG-x", INDIGO_IDLE_STATE);
		//+ dome.X_CONDITIONS_SAFETY.on_attach
		X_CONDITIONS_SAFETY_PROPERTY->state = INDIGO_IDLE_STATE;
		//- dome.X_CONDITIONS_SAFETY.on_attach
		INDIGO_DEVICE_ATTACH_LOG(DRIVER_NAME, device->name);
		return dome_enumerate_properties(device, NULL, NULL);
	}
	return INDIGO_FAILED;
}

static indigo_result dome_enumerate_properties(indigo_device *device, indigo_client *client, indigo_property *property) {
	if (IS_CONNECTED) {
		INDIGO_DEFINE_MATCHING_PROPERTY(X_SHUTTER_CALIBRATE_PROPERTY);
		INDIGO_DEFINE_MATCHING_PROPERTY(X_ROTATOR_CALIBRATE_PROPERTY);
		INDIGO_DEFINE_MATCHING_PROPERTY(X_FAILURE_MESSAGE_PROPERTY);
		INDIGO_DEFINE_MATCHING_PROPERTY(X_CLEAR_FAILURE_PROPERTY);
		INDIGO_DEFINE_MATCHING_PROPERTY(X_CONDITIONS_SAFETY_PROPERTY);
	}
	return indigo_dome_enumerate_properties(device, client, property);
}

static indigo_result dome_change_property(indigo_device *device, indigo_client *client, indigo_property *property) {
	if (indigo_property_match_changeable(CONNECTION_PROPERTY, property)) {
		if (!indigo_ignore_connection_change(device, property)) {
			indigo_property_copy_values(CONNECTION_PROPERTY, property, false);
			INDIGO_UPDATE_PROPERTY_STATE(CONNECTION_PROPERTY, INDIGO_BUSY_STATE, NULL);
			indigo_execute_handler(device, dome_connection_handler);
		}
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(DOME_HORIZONTAL_COORDINATES_PROPERTY, property)) {
		INDIGO_COPY_VALUES_PROCESS_CHANGE(DOME_HORIZONTAL_COORDINATES_PROPERTY, dome_horizontal_coordinates_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(DOME_STEPS_PROPERTY, property)) {
		//+ dome.DOME_STEPS.on_change_request
		if (DOME_HORIZONTAL_COORDINATES_PROPERTY->state == INDIGO_BUSY_STATE) {
			indigo_update_property(device, DOME_STEPS_PROPERTY, "Dome is moving: request can not be completed");
			return INDIGO_OK;
		}
		//- dome.DOME_STEPS.on_change_request
		INDIGO_COPY_VALUES_PROCESS_CHANGE(DOME_STEPS_PROPERTY, dome_steps_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(DOME_PARK_PROPERTY, property)) {
		INDIGO_COPY_VALUES_PROCESS_CHANGE(DOME_PARK_PROPERTY, dome_park_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(DOME_PARK_POSITION_PROPERTY, property)) {
		INDIGO_COPY_VALUES_PROCESS_CHANGE(DOME_PARK_POSITION_PROPERTY, dome_park_position_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(DOME_HOME_PROPERTY, property)) {
		INDIGO_COPY_VALUES_PROCESS_CHANGE(DOME_HOME_PROPERTY, dome_home_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(DOME_ABORT_MOTION_PROPERTY, property)) {
		INDIGO_COPY_VALUES_PROCESS_URGENT_CHANGE(DOME_ABORT_MOTION_PROPERTY, dome_abort_motion_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(DOME_SHUTTER_PROPERTY, property)) {
		INDIGO_COPY_VALUES_PROCESS_CHANGE(DOME_SHUTTER_PROPERTY, dome_shutter_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(X_SHUTTER_CALIBRATE_PROPERTY, property)) {
		INDIGO_COPY_VALUES_PROCESS_CHANGE(X_SHUTTER_CALIBRATE_PROPERTY, dome_x_shutter_calibrate_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(X_ROTATOR_CALIBRATE_PROPERTY, property)) {
		INDIGO_COPY_VALUES_PROCESS_CHANGE(X_ROTATOR_CALIBRATE_PROPERTY, dome_x_rotator_calibrate_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(X_CLEAR_FAILURE_PROPERTY, property)) {
		INDIGO_COPY_VALUES_PROCESS_CHANGE(X_CLEAR_FAILURE_PROPERTY, dome_x_clear_failure_handler);
		return INDIGO_OK;
	}
	return indigo_dome_change_property(device, client, property);
}

static indigo_result dome_detach(indigo_device *device) {
	if (IS_CONNECTED) {
		indigo_set_switch(CONNECTION_PROPERTY, CONNECTION_DISCONNECTED_ITEM, true);
		dome_connection_handler(device);
	}
	indigo_release_property(X_SHUTTER_CALIBRATE_PROPERTY);
	indigo_release_property(X_ROTATOR_CALIBRATE_PROPERTY);
	indigo_release_property(X_FAILURE_MESSAGE_PROPERTY);
	indigo_release_property(X_CLEAR_FAILURE_PROPERTY);
	indigo_release_property(X_CONDITIONS_SAFETY_PROPERTY);
	INDIGO_DEVICE_DETACH_LOG(DRIVER_NAME, device->name);
	return indigo_dome_detach(device);
}

#pragma mark - Device templates

static indigo_device dome_template = INDIGO_DEVICE_INITIALIZER(DOME_DEVICE_NAME, dome_attach, dome_enumerate_properties, dome_change_property, NULL, dome_detach);

#pragma mark - Main code

indigo_result indigo_dome_beaver(indigo_driver_action action, indigo_driver_info *info) {
	static indigo_driver_action last_action = INDIGO_DRIVER_SHUTDOWN;
	static beaver_private_data *private_data = NULL;
	static indigo_device *dome = NULL;

	SET_DRIVER_INFO(info, DRIVER_LABEL, __FUNCTION__, DRIVER_VERSION, false, last_action);

	if (action == last_action) {
		return INDIGO_OK;
	}

	switch (action) {
		case INDIGO_DRIVER_INIT: {
			last_action = action;
			private_data = (beaver_private_data *)indigo_safe_malloc(sizeof(beaver_private_data));
			dome = (indigo_device *)indigo_safe_malloc_copy(sizeof(indigo_device), &dome_template);
			dome->private_data = private_data;
			indigo_attach_device(dome);
			break;

		}
		case INDIGO_DRIVER_SHUTDOWN: {
			VERIFY_NOT_CONNECTED(dome);
			last_action = action;
			if (dome != NULL) {
				indigo_detach_device(dome);
				indigo_safe_free(dome);
				dome = NULL;
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
