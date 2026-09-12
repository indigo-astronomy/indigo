// Copyright (c) 2024-2026 CloudMakers, s. r. o.
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

/** INDIGO Optec FocusLynx focuser driver
 \file indigo_focuser_optecfl.c
 */

#define DRIVER_VERSION 0x02000002
#define DRIVER_NAME "indigo_focuser_optecfl"

#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <assert.h>
#include <stdarg.h>

#include <indigo/indigo_driver_xml.h>
#include <indigo/indigo_uni_io.h>

#include "indigo_focuser_optecfl.h"

#define PRIVATE_DATA								((optecfl_private_data *)device->private_data)

#define FOCUSER_ID									(device->gp_bits & 0x3)
#define FOCUSER_SLOT								(FOCUSER_ID - 1)

#define X_FOCUSER_TYPE_PROPERTY			(PRIVATE_DATA->type_property[FOCUSER_SLOT])

// The controller acknowledges every request with "!" before the payload; requests
// themselves are unterminated "<...>" sequences and every reply line ends with LF.
#define OPTECFL_FIRST_BYTE_DELAY		INDIGO_DELAY(1)
#define OPTECFL_NEXT_BYTE_DELAY			INDIGO_DELAY(0.1)
#define OPTECFL_MAX_BLOCK_LINES			32

typedef struct {
	indigo_uni_handle *handle;
	int count;
	char response[128];
	indigo_property *type_property[2];
	bool can_sync[2];
	bool update_position[2];
	bool update_temperature[2];
} optecfl_private_data;

typedef void (*optecfl_field_handler)(indigo_device *device, const char *key, const char *value);

// -------------------------------------------------------------------------------- Low level communication routines

static char *optecfl_trim(char *text) {
	while (*text == ' ' || *text == '\t') {
		text++;
	}
	char *end = text + strlen(text);
	while (end > text && (end[-1] == ' ' || end[-1] == '\t' || end[-1] == '\r')) {
		*--end = 0;
	}
	return text;
}

static bool optecfl_read_line(indigo_device *device) {
	// Keep the terminator so a truncated or timed out reply is distinguishable.
	long count = indigo_uni_read_section2(PRIVATE_DATA->handle, PRIVATE_DATA->response, sizeof(PRIVATE_DATA->response) - 1, "\n", "\r", OPTECFL_FIRST_BYTE_DELAY, OPTECFL_NEXT_BYTE_DELAY);
	if (count <= 0 || PRIVATE_DATA->response[count - 1] != '\n' || (long)strlen(PRIVATE_DATA->response) != count) {
		return false;
	}
	PRIVATE_DATA->response[count - 1] = 0;
	return true;
}

static bool optecfl_vcommand(indigo_device *device, const char *format, va_list args) {
	long result = indigo_uni_discard(PRIVATE_DATA->handle);
	if (result >= 0) {
		result = indigo_uni_vprintf(PRIVATE_DATA->handle, format, args);
	}
	if (result <= 0) {
		return false;
	}
	return optecfl_read_line(device) && !strcmp(PRIVATE_DATA->response, "!");
}

static bool optecfl_command(indigo_device *device, const char *format, ...) {
	va_list args;
	va_start(args, format);
	bool result = optecfl_vcommand(device, format, args);
	va_end(args);
	return result;
}

static bool optecfl_echo(indigo_device *device, const char *expected, const char *format, ...) {
	va_list args;
	va_start(args, format);
	bool result = optecfl_vcommand(device, format, args);
	va_end(args);
	return result && optecfl_read_line(device) && !strcmp(PRIVATE_DATA->response, expected);
}

// Reads a "header ... key = value ... END" block. The line budget bounds a reply
// whose END terminator never arrives.
static bool optecfl_block(indigo_device *device, optecfl_field_handler handler) {
	for (int line = 0; line < OPTECFL_MAX_BLOCK_LINES; line++) {
		if (!optecfl_read_line(device)) {
			return false;
		}
		if (!strcmp(PRIVATE_DATA->response, "END")) {
			return true;
		}
		char *separator = strchr(PRIVATE_DATA->response, '=');
		if (separator == NULL) {
			continue;
		}
		*separator = 0;
		char *key = optecfl_trim(PRIVATE_DATA->response);
		char *value = optecfl_trim(separator + 1);
		handler(device, key, value);
	}
	return false;
}

static void optecfl_hub_field(indigo_device *device, const char *key, const char *value) {
	if (!strcmp(key, "Hub FVer")) {
		INDIGO_COPY_VALUE(INFO_DEVICE_FW_REVISION_ITEM->text.value, value);
	}
}

static void optecfl_config_field(indigo_device *device, const char *key, const char *value) {
	if (!strcmp(key, "Max Pos")) {
		FOCUSER_LIMITS_MAX_POSITION_ITEM->number.value = FOCUSER_LIMITS_MAX_POSITION_ITEM->number.target = FOCUSER_POSITION_ITEM->number.max = atoi(value);
	} else if (!strcmp(key, "Dev Typ")) {
		// Optec focusers must home and therefore reject the SCCP sync command.
		PRIVATE_DATA->can_sync[FOCUSER_SLOT] = value[0] != 'O';
		for (int i = 0; i < X_FOCUSER_TYPE_PROPERTY->count; i++) {
			indigo_item *item = X_FOCUSER_TYPE_PROPERTY->items + i;
			if (!strncmp(item->name, value, 2)) {
				indigo_set_switch(X_FOCUSER_TYPE_PROPERTY, item, true);
			}
		}
	}
}

static void optecfl_status_field(indigo_device *device, const char *key, const char *value) {
	if (!strcmp(key, "Temp(C)")) {
		double temperature = atof(value);
		if (FOCUSER_TEMPERATURE_ITEM->number.value != temperature) {
			FOCUSER_TEMPERATURE_ITEM->number.value = temperature;
			PRIVATE_DATA->update_temperature[FOCUSER_SLOT] = true;
		}
	} else if (!strcmp(key, "TmpProbe")) {
		int attached = atoi(value);
		if (FOCUSER_TEMPERATURE_PROPERTY->state != INDIGO_IDLE_STATE && attached == 0) {
			FOCUSER_TEMPERATURE_PROPERTY->state = INDIGO_IDLE_STATE;
			PRIVATE_DATA->update_temperature[FOCUSER_SLOT] = true;
		} else if (FOCUSER_TEMPERATURE_PROPERTY->state == INDIGO_IDLE_STATE && attached == 1) {
			FOCUSER_TEMPERATURE_PROPERTY->state = INDIGO_OK_STATE;
			PRIVATE_DATA->update_temperature[FOCUSER_SLOT] = true;
		}
	} else if (!strcmp(key, "Curr Pos")) {
		int position = atoi(value);
		if (FOCUSER_POSITION_ITEM->number.value != position) {
			FOCUSER_POSITION_ITEM->number.value = position;
			PRIVATE_DATA->update_position[FOCUSER_SLOT] = true;
		}
	} else if (!strcmp(key, "Targ Pos")) {
		int target = atoi(value);
		if (FOCUSER_POSITION_ITEM->number.target != target) {
			FOCUSER_POSITION_ITEM->number.target = target;
			PRIVATE_DATA->update_position[FOCUSER_SLOT] = true;
		}
	} else if (!strcmp(key, "IsMoving")) {
		int moving = atoi(value);
		if (FOCUSER_POSITION_PROPERTY->state != INDIGO_BUSY_STATE && moving == 1) {
			FOCUSER_POSITION_PROPERTY->state = FOCUSER_STEPS_PROPERTY->state = INDIGO_BUSY_STATE;
			PRIVATE_DATA->update_position[FOCUSER_SLOT] = true;
		} else if (FOCUSER_POSITION_PROPERTY->state == INDIGO_BUSY_STATE && moving == 0) {
			FOCUSER_POSITION_PROPERTY->state = FOCUSER_STEPS_PROPERTY->state = INDIGO_OK_STATE;
			PRIVATE_DATA->update_position[FOCUSER_SLOT] = true;
		}
	}
}

static bool optecfl_open(indigo_device *device) {
	char *name = DEVICE_PORT_ITEM->text.value;
	bool opened = PRIVATE_DATA->count == 0;
	if (opened) {
		PRIVATE_DATA->handle = indigo_uni_open_serial_with_speed(name, 115200, INDIGO_LOG_DEBUG);
		if (PRIVATE_DATA->handle == NULL) {
			INDIGO_DRIVER_ERROR(DRIVER_NAME, "Failed to connect to %s", name);
			return false;
		}
		INDIGO_DRIVER_LOG(DRIVER_NAME, "Connected to %s", name);
		if (!optecfl_command(device, "<FHGETHUBINFO>") || !optecfl_block(device, optecfl_hub_field)) {
			INDIGO_DRIVER_ERROR(DRIVER_NAME, "Failed to read hub info");
			indigo_uni_close(&PRIVATE_DATA->handle);
			return false;
		}
	}
	if (!optecfl_command(device, "<F%dGETCONFIG>", FOCUSER_ID) || !optecfl_block(device, optecfl_config_field)) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "Failed to open focuser #%d", FOCUSER_ID);
		if (opened) {
			indigo_uni_close(&PRIVATE_DATA->handle);
		}
		return false;
	}
	PRIVATE_DATA->count++;
	return true;
}

static void optecfl_close(indigo_device *device) {
	if (--PRIVATE_DATA->count == 0) {
		indigo_uni_close(&PRIVATE_DATA->handle);
		INDIGO_DRIVER_LOG(DRIVER_NAME, "Disconnected from %s", DEVICE_PORT_ITEM->text.value);
	}
}

// -------------------------------------------------------------------------------- Handlers

static void focuser_timer_callback(indigo_device *device) {
	if (!IS_CONNECTED) {
		return;
	}
	PRIVATE_DATA->update_position[FOCUSER_SLOT] = PRIVATE_DATA->update_temperature[FOCUSER_SLOT] = false;
	if (optecfl_command(device, "<F%dGETSTATUS>", FOCUSER_ID) && optecfl_block(device, optecfl_status_field)) {
		if (PRIVATE_DATA->update_temperature[FOCUSER_SLOT]) {
			indigo_update_property(device, FOCUSER_TEMPERATURE_PROPERTY, NULL);
		}
		if (PRIVATE_DATA->update_position[FOCUSER_SLOT]) {
			indigo_update_property(device, FOCUSER_POSITION_PROPERTY, NULL);
			indigo_update_property(device, FOCUSER_STEPS_PROPERTY, NULL);
		}
	}
	indigo_execute_handler_in(device, 1, focuser_timer_callback);
}

static void focuser_connection_handler(indigo_device *device) {
	if (CONNECTION_CONNECTED_ITEM->sw.value) {
		if (optecfl_open(device)) {
			indigo_define_property(device, X_FOCUSER_TYPE_PROPERTY, NULL);
			indigo_execute_handler_in(device, 1, focuser_timer_callback);
			CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
		} else {
			CONNECTION_PROPERTY->state = INDIGO_ALERT_STATE;
			indigo_set_switch(CONNECTION_PROPERTY, CONNECTION_DISCONNECTED_ITEM, true);
		}
	} else {
		indigo_cancel_pending_handlers(device);
		indigo_delete_property(device, X_FOCUSER_TYPE_PROPERTY, NULL);
		optecfl_close(device);
		CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
	}
	indigo_focuser_change_property(device, NULL, CONNECTION_PROPERTY);
}

static void focuser_type_handler(indigo_device *device) {
	char *value = NULL;
	for (int i = 0; i < X_FOCUSER_TYPE_PROPERTY->count; i++) {
		indigo_item *item = X_FOCUSER_TYPE_PROPERTY->items + i;
		if (item->sw.value) {
			value = item->name;
			break;
		}
	}
	if (value == NULL) {
		X_FOCUSER_TYPE_PROPERTY->state = INDIGO_ALERT_STATE;
		indigo_update_property(device, X_FOCUSER_TYPE_PROPERTY, "No focuser type selected");
		return;
	}
	if (optecfl_echo(device, "SET", "<F%dSCDT%.2s>", FOCUSER_ID, value)) {
		PRIVATE_DATA->can_sync[FOCUSER_SLOT] = value[0] != 'O';
		X_FOCUSER_TYPE_PROPERTY->state = INDIGO_OK_STATE;
		// Max Pos follows the device type, so re-read the configuration block.
		if (optecfl_command(device, "<F%dGETCONFIG>", FOCUSER_ID) && optecfl_block(device, optecfl_config_field)) {
			indigo_update_property(device, FOCUSER_LIMITS_PROPERTY, NULL);
		}
	} else {
		X_FOCUSER_TYPE_PROPERTY->state = INDIGO_ALERT_STATE;
	}
	indigo_update_property(device, X_FOCUSER_TYPE_PROPERTY, NULL);
}

static void focuser_position_handler(indigo_device *device) {
	int position = (int)FOCUSER_POSITION_ITEM->number.target;
	if (FOCUSER_ON_POSITION_SET_GOTO_ITEM->sw.value) {
		if (optecfl_echo(device, "M", "<F%dMA%06d>", FOCUSER_ID, position)) {
			FOCUSER_POSITION_PROPERTY->state = FOCUSER_STEPS_PROPERTY->state = INDIGO_BUSY_STATE;
		} else {
			FOCUSER_POSITION_PROPERTY->state = FOCUSER_STEPS_PROPERTY->state = INDIGO_ALERT_STATE;
		}
	} else if (!PRIVATE_DATA->can_sync[FOCUSER_SLOT]) {
		FOCUSER_POSITION_PROPERTY->state = FOCUSER_STEPS_PROPERTY->state = INDIGO_ALERT_STATE;
		indigo_update_property(device, FOCUSER_STEPS_PROPERTY, NULL);
		indigo_update_property(device, FOCUSER_POSITION_PROPERTY, "This focuser type must home and does not support sync");
		return;
	} else if (optecfl_echo(device, "SET", "<F%dSCCP%06d>", FOCUSER_ID, position)) {
		FOCUSER_POSITION_ITEM->number.value = position;
		FOCUSER_POSITION_PROPERTY->state = FOCUSER_STEPS_PROPERTY->state = INDIGO_OK_STATE;
	} else {
		FOCUSER_POSITION_PROPERTY->state = FOCUSER_STEPS_PROPERTY->state = INDIGO_ALERT_STATE;
	}
	indigo_update_property(device, FOCUSER_STEPS_PROPERTY, NULL);
	indigo_update_property(device, FOCUSER_POSITION_PROPERTY, NULL);
}

static void focuser_steps_handler(indigo_device *device) {
	// The protocol has no reverse setting, so the requested direction is resolved here.
	bool inward = FOCUSER_DIRECTION_MOVE_INWARD_ITEM->sw.value ^ FOCUSER_REVERSE_MOTION_ENABLED_ITEM->sw.value;
	int steps = (int)FOCUSER_STEPS_ITEM->number.value;
	int position = (int)FOCUSER_POSITION_ITEM->number.value + (inward ? -steps : steps);
	if (position < 0) {
		position = 0;
	} else if (position > FOCUSER_POSITION_ITEM->number.max) {
		position = (int)FOCUSER_POSITION_ITEM->number.max;
	}
	if (optecfl_echo(device, "M", "<F%dMA%06d>", FOCUSER_ID, position)) {
		FOCUSER_POSITION_ITEM->number.target = position;
		FOCUSER_POSITION_PROPERTY->state = FOCUSER_STEPS_PROPERTY->state = INDIGO_BUSY_STATE;
	} else {
		FOCUSER_POSITION_PROPERTY->state = FOCUSER_STEPS_PROPERTY->state = INDIGO_ALERT_STATE;
	}
	indigo_update_property(device, FOCUSER_POSITION_PROPERTY, NULL);
	indigo_update_property(device, FOCUSER_STEPS_PROPERTY, NULL);
}

static void focuser_abort_handler(indigo_device *device) {
	if (optecfl_echo(device, "HALTED", "<F%dHALT>", FOCUSER_ID)) {
		FOCUSER_ABORT_MOTION_PROPERTY->state = INDIGO_OK_STATE;
		if (FOCUSER_POSITION_PROPERTY->state == INDIGO_BUSY_STATE) {
			FOCUSER_POSITION_PROPERTY->state = FOCUSER_STEPS_PROPERTY->state = INDIGO_OK_STATE;
			indigo_update_property(device, FOCUSER_POSITION_PROPERTY, NULL);
			indigo_update_property(device, FOCUSER_STEPS_PROPERTY, NULL);
		}
	} else {
		FOCUSER_ABORT_MOTION_PROPERTY->state = INDIGO_ALERT_STATE;
	}
	FOCUSER_ABORT_MOTION_ITEM->sw.value = false;
	indigo_update_property(device, FOCUSER_ABORT_MOTION_PROPERTY, NULL);
}

// -------------------------------------------------------------------------------- INDIGO focuser device implementation

static indigo_result focuser_enumerate_properties(indigo_device *device, indigo_client *client, indigo_property *property);

static indigo_result focuser_attach(indigo_device *device) {
	assert(device != NULL);
	assert(PRIVATE_DATA != NULL);
	if (indigo_focuser_attach(device, DRIVER_NAME, DRIVER_VERSION) == INDIGO_OK) {
		// -------------------------------------------------------------------------------- DEVICE_PORT, DEVICE_PORTS
		DEVICE_PORT_PROPERTY->hidden = false;
		DEVICE_PORTS_PROPERTY->hidden = false;
		indigo_enumerate_serial_ports(device, DEVICE_PORTS_PROPERTY);
#ifdef INDIGO_MACOS
		for (int i = 0; i < DEVICE_PORTS_PROPERTY->count; i++) {
			if (!strncmp(DEVICE_PORTS_PROPERTY->items[i].name, "/dev/cu.usbmodem", 16)) {
				INDIGO_COPY_VALUE(DEVICE_PORT_ITEM->text.value, DEVICE_PORTS_PROPERTY->items[i].name);
				break;
			}
		}
#endif
#ifdef INDIGO_LINUX
		INDIGO_COPY_VALUE(DEVICE_PORT_ITEM->text.value, "/dev/usb_focuser");
#endif
		// -------------------------------------------------------------------------------- FOCUSER_TEMPERATURE
		FOCUSER_TEMPERATURE_PROPERTY->hidden = false;
		// -------------------------------------------------------------------------------- FOCUSER_SPEED
		FOCUSER_SPEED_PROPERTY->hidden = true;
		// -------------------------------------------------------------------------------- FOCUSER_LIMITS
		FOCUSER_LIMITS_PROPERTY->hidden = false;
		FOCUSER_LIMITS_MIN_POSITION_ITEM->number.min = FOCUSER_LIMITS_MIN_POSITION_ITEM->number.max = FOCUSER_LIMITS_MIN_POSITION_ITEM->number.value = 0;
		// -------------------------------------------------------------------------------- FOCUSER_POSITION
		FOCUSER_POSITION_ITEM->number.min = 0;
		FOCUSER_POSITION_ITEM->number.step = 1;
		// -------------------------------------------------------------------------------- FOCUSER_STEPS
		FOCUSER_STEPS_ITEM->number.min = 0;
		FOCUSER_STEPS_ITEM->number.max = 99999;
		FOCUSER_STEPS_ITEM->number.step = 1;
		// -------------------------------------------------------------------------------- FOCUSER_REVERSE_MOTION
		FOCUSER_REVERSE_MOTION_PROPERTY->hidden = false;
		// -------------------------------------------------------------------------------- FOCUSER_ON_POSITION_SET
		FOCUSER_ON_POSITION_SET_PROPERTY->hidden = false;
		// -------------------------------------------------------------------------------- X_FOCUSER_TYPE
		X_FOCUSER_TYPE_PROPERTY = indigo_init_switch_property(NULL, device->name, "X_FOCUSER_TYPE", FOCUSER_MAIN_GROUP, "Focuser type", INDIGO_OK_STATE, INDIGO_RW_PERM, INDIGO_ONE_OF_MANY_RULE, 29);
		if (X_FOCUSER_TYPE_PROPERTY == NULL) {
			return INDIGO_FAILED;
		}
		// Device types and labels follow Appendix A of FocusLynx_Command_Processing_rev3.pdf.
		indigo_init_switch_item(X_FOCUSER_TYPE_PROPERTY->items + 0, "OA", "Optec TCF-Lynx 2\"", true);
		indigo_init_switch_item(X_FOCUSER_TYPE_PROPERTY->items + 1, "OB", "Optec TCF-Lynx 3\"", false);
		indigo_init_switch_item(X_FOCUSER_TYPE_PROPERTY->items + 2, "OC", "Optec TCF-Lynx 2\" with Extended Travel", false);
		indigo_init_switch_item(X_FOCUSER_TYPE_PROPERTY->items + 3, "OD", "Optec Fast Focus Secondary Focuser", false);
		indigo_init_switch_item(X_FOCUSER_TYPE_PROPERTY->items + 4, "OE", "Optec TCF-S Classic converted", false);
		indigo_init_switch_item(X_FOCUSER_TYPE_PROPERTY->items + 5, "OF", "Optec TCF-S3 Classic converted", false);
		indigo_init_switch_item(X_FOCUSER_TYPE_PROPERTY->items + 6, "OG", "Optec Gemini (reserved for future use)", false);
		indigo_init_switch_item(X_FOCUSER_TYPE_PROPERTY->items + 7, "FA", "FocusLynx QuickSync FT Hi-Torque", false);
		indigo_init_switch_item(X_FOCUSER_TYPE_PROPERTY->items + 8, "FB", "FocusLynx QuickSync FT Hi-Speed", false);
		indigo_init_switch_item(X_FOCUSER_TYPE_PROPERTY->items + 9, "FC", "FocusLynx QuickSync SV (reserved for future use)", false);
		indigo_init_switch_item(X_FOCUSER_TYPE_PROPERTY->items + 10, "SA", "Starlight Focuser FTF2008BCR", false);
		indigo_init_switch_item(X_FOCUSER_TYPE_PROPERTY->items + 11, "SB", "Starlight Focuser FTF2015BCR", false);
		indigo_init_switch_item(X_FOCUSER_TYPE_PROPERTY->items + 12, "SC", "Starlight Focuser FTF2020BCR", false);
		indigo_init_switch_item(X_FOCUSER_TYPE_PROPERTY->items + 13, "SD", "Starlight Focuser FTF2025", false);
		indigo_init_switch_item(X_FOCUSER_TYPE_PROPERTY->items + 14, "SE", "Starlight Focuser FTF2515B-A", false);
		indigo_init_switch_item(X_FOCUSER_TYPE_PROPERTY->items + 15, "SF", "Starlight Focuser FTF2525B-A", false);
		indigo_init_switch_item(X_FOCUSER_TYPE_PROPERTY->items + 16, "SG", "Starlight Focuser FTF2535B-A", false);
		indigo_init_switch_item(X_FOCUSER_TYPE_PROPERTY->items + 17, "SH", "Starlight Focuser FTF3015B-A", false);
		indigo_init_switch_item(X_FOCUSER_TYPE_PROPERTY->items + 18, "SI", "Starlight Focuser FTF3025B-A", false);
		indigo_init_switch_item(X_FOCUSER_TYPE_PROPERTY->items + 19, "SJ", "Starlight Focuser FTF3035B-A", false);
		indigo_init_switch_item(X_FOCUSER_TYPE_PROPERTY->items + 20, "SK", "Starlight Focuser FTF3515B-A", false);
		indigo_init_switch_item(X_FOCUSER_TYPE_PROPERTY->items + 21, "SL", "Starlight Focuser FTF3545B-A", false);
		indigo_init_switch_item(X_FOCUSER_TYPE_PROPERTY->items + 22, "SM", "Starlight Focuser AP27FOC3E", false);
		indigo_init_switch_item(X_FOCUSER_TYPE_PROPERTY->items + 23, "SN", "Starlight Focuser AP4FOC3E", false);
		indigo_init_switch_item(X_FOCUSER_TYPE_PROPERTY->items + 24, "SO", "FeatherTouch Motor Hi-Speed", false);
		indigo_init_switch_item(X_FOCUSER_TYPE_PROPERTY->items + 25, "SP", "FeatherTouch Motor Hi-Torque", false);
		indigo_init_switch_item(X_FOCUSER_TYPE_PROPERTY->items + 26, "SQ", "Starlight Instruments FTM with MicroTouch", false);
		indigo_init_switch_item(X_FOCUSER_TYPE_PROPERTY->items + 27, "TA", "Televue Focuser with Micro-Touch motor", false);
		indigo_init_switch_item(X_FOCUSER_TYPE_PROPERTY->items + 28, "ZZ", "Default setting, no function", false);
		// --------------------------------------------------------------------------------
		INFO_PROPERTY->count = 6;
		INDIGO_COPY_VALUE(INFO_DEVICE_MODEL_ITEM->text.value, "FocusLynx");
		INDIGO_DEVICE_ATTACH_LOG(DRIVER_NAME, device->name);
		return focuser_enumerate_properties(device, NULL, NULL);
	}
	return INDIGO_FAILED;
}

static indigo_result focuser_enumerate_properties(indigo_device *device, indigo_client *client, indigo_property *property) {
	if (IS_CONNECTED) {
		INDIGO_DEFINE_MATCHING_PROPERTY(X_FOCUSER_TYPE_PROPERTY);
	}
	return indigo_focuser_enumerate_properties(device, client, property);
}

static indigo_result focuser_change_property(indigo_device *device, indigo_client *client, indigo_property *property) {
	assert(device != NULL);
	assert(DEVICE_CONTEXT != NULL);
	assert(property != NULL);
	if (indigo_property_match_changeable(CONNECTION_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- CONNECTION
		if (indigo_ignore_connection_change(device, property)) {
			return INDIGO_OK;
		}
		indigo_property_copy_values(CONNECTION_PROPERTY, property, false);
		INDIGO_UPDATE_PROPERTY_STATE(CONNECTION_PROPERTY, INDIGO_BUSY_STATE, NULL);
		indigo_execute_handler(device, focuser_connection_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(FOCUSER_POSITION_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- FOCUSER_POSITION
		INDIGO_COPY_TARGETS_PROCESS_CHANGE(FOCUSER_POSITION_PROPERTY, focuser_position_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(FOCUSER_STEPS_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- FOCUSER_STEPS
		INDIGO_COPY_VALUES_PROCESS_CHANGE(FOCUSER_STEPS_PROPERTY, focuser_steps_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(FOCUSER_ABORT_MOTION_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- FOCUSER_ABORT_MOTION
		INDIGO_COPY_VALUES_PROCESS_CHANGE_ANYTIME(FOCUSER_ABORT_MOTION_PROPERTY, focuser_abort_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(X_FOCUSER_TYPE_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- X_FOCUSER_TYPE
		INDIGO_COPY_VALUES_PROCESS_CHANGE(X_FOCUSER_TYPE_PROPERTY, focuser_type_handler);
		return INDIGO_OK;
	}
	return indigo_focuser_change_property(device, client, property);
}

static indigo_result focuser_detach(indigo_device *device) {
	assert(device != NULL);
	if (IS_CONNECTED) {
		indigo_set_switch(CONNECTION_PROPERTY, CONNECTION_DISCONNECTED_ITEM, true);
		focuser_connection_handler(device);
	}
	indigo_release_property(X_FOCUSER_TYPE_PROPERTY);
	INDIGO_DEVICE_DETACH_LOG(DRIVER_NAME, device->name);
	return indigo_focuser_detach(device);
}

// -------------------------------------------------------------------------------- INDIGO driver implementation

indigo_result indigo_focuser_optecfl(indigo_driver_action action, indigo_driver_info *info) {
	static indigo_driver_action last_action = INDIGO_DRIVER_SHUTDOWN;
	static optecfl_private_data *private_data = NULL;
	static indigo_device *focuser1 = NULL;
	static indigo_device *focuser2 = NULL;

	static indigo_device focuser_template = INDIGO_DEVICE_INITIALIZER(
		"Optec FocusLynx",
		focuser_attach,
		focuser_enumerate_properties,
		focuser_change_property,
		NULL,
		focuser_detach
	);

	SET_DRIVER_INFO(info, "Optec FocusLynx Focuser", __FUNCTION__, DRIVER_VERSION, false, last_action);

	if (action == last_action) {
		return INDIGO_OK;
	}

	switch (action) {
		case INDIGO_DRIVER_INIT:
			last_action = action;
			private_data = indigo_safe_malloc(sizeof(optecfl_private_data));
			focuser1 = indigo_safe_malloc_copy(sizeof(indigo_device), &focuser_template);
			strcat(focuser1->name, " #1");
			focuser1->private_data = private_data;
			focuser1->gp_bits = 1;
			indigo_attach_device(focuser1);
			focuser2 = indigo_safe_malloc_copy(sizeof(indigo_device), &focuser_template);
			strcat(focuser2->name, " #2");
			focuser2->private_data = private_data;
			focuser2->gp_bits = 2;
			// Both logical focusers share one serial handle, so one handler queue owns it.
			focuser2->master_device = focuser1;
			indigo_attach_device(focuser2);
			break;

		case INDIGO_DRIVER_SHUTDOWN:
			VERIFY_NOT_CONNECTED(focuser1);
			VERIFY_NOT_CONNECTED(focuser2);
			last_action = action;
			if (focuser2 != NULL) {
				indigo_detach_device(focuser2);
				free(focuser2);
				focuser2 = NULL;
			}
			if (focuser1 != NULL) {
				indigo_detach_device(focuser1);
				free(focuser1);
				focuser1 = NULL;
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
