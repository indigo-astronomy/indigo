// Copyright (c) 2024-2026 CloudMakers, s. r. o.
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

// This file generated from indigo_focuser_optecfl.driver

#pragma mark - Includes

#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <assert.h>

//+ include

#include <stdarg.h>

//- include

#include <indigo/indigo_driver_xml.h>
#include <indigo/indigo_focuser_driver.h>
#include <indigo/indigo_focuser_driver.h>
#include <indigo/indigo_uni_io.h>

#include "indigo_focuser_optecfl.h"

#pragma mark - Common definitions

#define DRIVER_VERSION       0x03000003
#define DRIVER_NAME          "indigo_focuser_optecfl"
#define DRIVER_LABEL         "Optec FocusLynx Focuser"
#define FOCUSER_1_DEVICE_NAME "Optec FocusLynx #1"
#define FOCUSER_2_DEVICE_NAME "Optec FocusLynx #2"
#define PRIVATE_DATA         ((optecfl_private_data *)device->private_data)

//+ define

// The controller acknowledges every request with "!" before the payload;
// requests themselves are unterminated "<...>" sequences and every reply
// line ends with LF.
#define OPTECFL_FIRST_BYTE_DELAY INDIGO_DELAY(1)
#define OPTECFL_NEXT_BYTE_DELAY INDIGO_DELAY(0.1)
#define OPTECFL_MAX_BLOCK_LINES 32
#define FOCUSER_ID           (device->gp_bits & 0x3)
#define FOCUSER_SLOT         (FOCUSER_ID - 1)

//- define

#pragma mark - Property definitions

#define X_FOCUSER_1_TYPE_PROPERTY      (PRIVATE_DATA->x_focuser_1_type_property)
#define X_FOCUSER_1_TYPE_OA_ITEM       (X_FOCUSER_1_TYPE_PROPERTY->items + 0)
#define X_FOCUSER_1_TYPE_OB_ITEM       (X_FOCUSER_1_TYPE_PROPERTY->items + 1)
#define X_FOCUSER_1_TYPE_OC_ITEM       (X_FOCUSER_1_TYPE_PROPERTY->items + 2)
#define X_FOCUSER_1_TYPE_OD_ITEM       (X_FOCUSER_1_TYPE_PROPERTY->items + 3)
#define X_FOCUSER_1_TYPE_OE_ITEM       (X_FOCUSER_1_TYPE_PROPERTY->items + 4)
#define X_FOCUSER_1_TYPE_OF_ITEM       (X_FOCUSER_1_TYPE_PROPERTY->items + 5)
#define X_FOCUSER_1_TYPE_OG_ITEM       (X_FOCUSER_1_TYPE_PROPERTY->items + 6)
#define X_FOCUSER_1_TYPE_FA_ITEM       (X_FOCUSER_1_TYPE_PROPERTY->items + 7)
#define X_FOCUSER_1_TYPE_FB_ITEM       (X_FOCUSER_1_TYPE_PROPERTY->items + 8)
#define X_FOCUSER_1_TYPE_FC_ITEM       (X_FOCUSER_1_TYPE_PROPERTY->items + 9)
#define X_FOCUSER_1_TYPE_SA_ITEM       (X_FOCUSER_1_TYPE_PROPERTY->items + 10)
#define X_FOCUSER_1_TYPE_SB_ITEM       (X_FOCUSER_1_TYPE_PROPERTY->items + 11)
#define X_FOCUSER_1_TYPE_SC_ITEM       (X_FOCUSER_1_TYPE_PROPERTY->items + 12)
#define X_FOCUSER_1_TYPE_SD_ITEM       (X_FOCUSER_1_TYPE_PROPERTY->items + 13)
#define X_FOCUSER_1_TYPE_SE_ITEM       (X_FOCUSER_1_TYPE_PROPERTY->items + 14)
#define X_FOCUSER_1_TYPE_SF_ITEM       (X_FOCUSER_1_TYPE_PROPERTY->items + 15)
#define X_FOCUSER_1_TYPE_SG_ITEM       (X_FOCUSER_1_TYPE_PROPERTY->items + 16)
#define X_FOCUSER_1_TYPE_SH_ITEM       (X_FOCUSER_1_TYPE_PROPERTY->items + 17)
#define X_FOCUSER_1_TYPE_SI_ITEM       (X_FOCUSER_1_TYPE_PROPERTY->items + 18)
#define X_FOCUSER_1_TYPE_SJ_ITEM       (X_FOCUSER_1_TYPE_PROPERTY->items + 19)
#define X_FOCUSER_1_TYPE_SK_ITEM       (X_FOCUSER_1_TYPE_PROPERTY->items + 20)
#define X_FOCUSER_1_TYPE_SL_ITEM       (X_FOCUSER_1_TYPE_PROPERTY->items + 21)
#define X_FOCUSER_1_TYPE_SM_ITEM       (X_FOCUSER_1_TYPE_PROPERTY->items + 22)
#define X_FOCUSER_1_TYPE_SN_ITEM       (X_FOCUSER_1_TYPE_PROPERTY->items + 23)
#define X_FOCUSER_1_TYPE_SO_ITEM       (X_FOCUSER_1_TYPE_PROPERTY->items + 24)
#define X_FOCUSER_1_TYPE_SP_ITEM       (X_FOCUSER_1_TYPE_PROPERTY->items + 25)
#define X_FOCUSER_1_TYPE_SQ_ITEM       (X_FOCUSER_1_TYPE_PROPERTY->items + 26)
#define X_FOCUSER_1_TYPE_TA_ITEM       (X_FOCUSER_1_TYPE_PROPERTY->items + 27)
#define X_FOCUSER_1_TYPE_ZZ_ITEM       (X_FOCUSER_1_TYPE_PROPERTY->items + 28)

#define X_FOCUSER_1_TYPE_PROPERTY_NAME "X_FOCUSER_TYPE"
#define X_FOCUSER_1_TYPE_OA_ITEM_NAME  "OA"
#define X_FOCUSER_1_TYPE_OB_ITEM_NAME  "OB"
#define X_FOCUSER_1_TYPE_OC_ITEM_NAME  "OC"
#define X_FOCUSER_1_TYPE_OD_ITEM_NAME  "OD"
#define X_FOCUSER_1_TYPE_OE_ITEM_NAME  "OE"
#define X_FOCUSER_1_TYPE_OF_ITEM_NAME  "OF"
#define X_FOCUSER_1_TYPE_OG_ITEM_NAME  "OG"
#define X_FOCUSER_1_TYPE_FA_ITEM_NAME  "FA"
#define X_FOCUSER_1_TYPE_FB_ITEM_NAME  "FB"
#define X_FOCUSER_1_TYPE_FC_ITEM_NAME  "FC"
#define X_FOCUSER_1_TYPE_SA_ITEM_NAME  "SA"
#define X_FOCUSER_1_TYPE_SB_ITEM_NAME  "SB"
#define X_FOCUSER_1_TYPE_SC_ITEM_NAME  "SC"
#define X_FOCUSER_1_TYPE_SD_ITEM_NAME  "SD"
#define X_FOCUSER_1_TYPE_SE_ITEM_NAME  "SE"
#define X_FOCUSER_1_TYPE_SF_ITEM_NAME  "SF"
#define X_FOCUSER_1_TYPE_SG_ITEM_NAME  "SG"
#define X_FOCUSER_1_TYPE_SH_ITEM_NAME  "SH"
#define X_FOCUSER_1_TYPE_SI_ITEM_NAME  "SI"
#define X_FOCUSER_1_TYPE_SJ_ITEM_NAME  "SJ"
#define X_FOCUSER_1_TYPE_SK_ITEM_NAME  "SK"
#define X_FOCUSER_1_TYPE_SL_ITEM_NAME  "SL"
#define X_FOCUSER_1_TYPE_SM_ITEM_NAME  "SM"
#define X_FOCUSER_1_TYPE_SN_ITEM_NAME  "SN"
#define X_FOCUSER_1_TYPE_SO_ITEM_NAME  "SO"
#define X_FOCUSER_1_TYPE_SP_ITEM_NAME  "SP"
#define X_FOCUSER_1_TYPE_SQ_ITEM_NAME  "SQ"
#define X_FOCUSER_1_TYPE_TA_ITEM_NAME  "TA"
#define X_FOCUSER_1_TYPE_ZZ_ITEM_NAME  "ZZ"

#define X_FOCUSER_2_TYPE_PROPERTY      (PRIVATE_DATA->x_focuser_2_type_property)
#define X_FOCUSER_2_TYPE_OA_ITEM       (X_FOCUSER_2_TYPE_PROPERTY->items + 0)
#define X_FOCUSER_2_TYPE_OB_ITEM       (X_FOCUSER_2_TYPE_PROPERTY->items + 1)
#define X_FOCUSER_2_TYPE_OC_ITEM       (X_FOCUSER_2_TYPE_PROPERTY->items + 2)
#define X_FOCUSER_2_TYPE_OD_ITEM       (X_FOCUSER_2_TYPE_PROPERTY->items + 3)
#define X_FOCUSER_2_TYPE_OE_ITEM       (X_FOCUSER_2_TYPE_PROPERTY->items + 4)
#define X_FOCUSER_2_TYPE_OF_ITEM       (X_FOCUSER_2_TYPE_PROPERTY->items + 5)
#define X_FOCUSER_2_TYPE_OG_ITEM       (X_FOCUSER_2_TYPE_PROPERTY->items + 6)
#define X_FOCUSER_2_TYPE_FA_ITEM       (X_FOCUSER_2_TYPE_PROPERTY->items + 7)
#define X_FOCUSER_2_TYPE_FB_ITEM       (X_FOCUSER_2_TYPE_PROPERTY->items + 8)
#define X_FOCUSER_2_TYPE_FC_ITEM       (X_FOCUSER_2_TYPE_PROPERTY->items + 9)
#define X_FOCUSER_2_TYPE_SA_ITEM       (X_FOCUSER_2_TYPE_PROPERTY->items + 10)
#define X_FOCUSER_2_TYPE_SB_ITEM       (X_FOCUSER_2_TYPE_PROPERTY->items + 11)
#define X_FOCUSER_2_TYPE_SC_ITEM       (X_FOCUSER_2_TYPE_PROPERTY->items + 12)
#define X_FOCUSER_2_TYPE_SD_ITEM       (X_FOCUSER_2_TYPE_PROPERTY->items + 13)
#define X_FOCUSER_2_TYPE_SE_ITEM       (X_FOCUSER_2_TYPE_PROPERTY->items + 14)
#define X_FOCUSER_2_TYPE_SF_ITEM       (X_FOCUSER_2_TYPE_PROPERTY->items + 15)
#define X_FOCUSER_2_TYPE_SG_ITEM       (X_FOCUSER_2_TYPE_PROPERTY->items + 16)
#define X_FOCUSER_2_TYPE_SH_ITEM       (X_FOCUSER_2_TYPE_PROPERTY->items + 17)
#define X_FOCUSER_2_TYPE_SI_ITEM       (X_FOCUSER_2_TYPE_PROPERTY->items + 18)
#define X_FOCUSER_2_TYPE_SJ_ITEM       (X_FOCUSER_2_TYPE_PROPERTY->items + 19)
#define X_FOCUSER_2_TYPE_SK_ITEM       (X_FOCUSER_2_TYPE_PROPERTY->items + 20)
#define X_FOCUSER_2_TYPE_SL_ITEM       (X_FOCUSER_2_TYPE_PROPERTY->items + 21)
#define X_FOCUSER_2_TYPE_SM_ITEM       (X_FOCUSER_2_TYPE_PROPERTY->items + 22)
#define X_FOCUSER_2_TYPE_SN_ITEM       (X_FOCUSER_2_TYPE_PROPERTY->items + 23)
#define X_FOCUSER_2_TYPE_SO_ITEM       (X_FOCUSER_2_TYPE_PROPERTY->items + 24)
#define X_FOCUSER_2_TYPE_SP_ITEM       (X_FOCUSER_2_TYPE_PROPERTY->items + 25)
#define X_FOCUSER_2_TYPE_SQ_ITEM       (X_FOCUSER_2_TYPE_PROPERTY->items + 26)
#define X_FOCUSER_2_TYPE_TA_ITEM       (X_FOCUSER_2_TYPE_PROPERTY->items + 27)
#define X_FOCUSER_2_TYPE_ZZ_ITEM       (X_FOCUSER_2_TYPE_PROPERTY->items + 28)

#define X_FOCUSER_2_TYPE_PROPERTY_NAME "X_FOCUSER_TYPE"
#define X_FOCUSER_2_TYPE_OA_ITEM_NAME  "OA"
#define X_FOCUSER_2_TYPE_OB_ITEM_NAME  "OB"
#define X_FOCUSER_2_TYPE_OC_ITEM_NAME  "OC"
#define X_FOCUSER_2_TYPE_OD_ITEM_NAME  "OD"
#define X_FOCUSER_2_TYPE_OE_ITEM_NAME  "OE"
#define X_FOCUSER_2_TYPE_OF_ITEM_NAME  "OF"
#define X_FOCUSER_2_TYPE_OG_ITEM_NAME  "OG"
#define X_FOCUSER_2_TYPE_FA_ITEM_NAME  "FA"
#define X_FOCUSER_2_TYPE_FB_ITEM_NAME  "FB"
#define X_FOCUSER_2_TYPE_FC_ITEM_NAME  "FC"
#define X_FOCUSER_2_TYPE_SA_ITEM_NAME  "SA"
#define X_FOCUSER_2_TYPE_SB_ITEM_NAME  "SB"
#define X_FOCUSER_2_TYPE_SC_ITEM_NAME  "SC"
#define X_FOCUSER_2_TYPE_SD_ITEM_NAME  "SD"
#define X_FOCUSER_2_TYPE_SE_ITEM_NAME  "SE"
#define X_FOCUSER_2_TYPE_SF_ITEM_NAME  "SF"
#define X_FOCUSER_2_TYPE_SG_ITEM_NAME  "SG"
#define X_FOCUSER_2_TYPE_SH_ITEM_NAME  "SH"
#define X_FOCUSER_2_TYPE_SI_ITEM_NAME  "SI"
#define X_FOCUSER_2_TYPE_SJ_ITEM_NAME  "SJ"
#define X_FOCUSER_2_TYPE_SK_ITEM_NAME  "SK"
#define X_FOCUSER_2_TYPE_SL_ITEM_NAME  "SL"
#define X_FOCUSER_2_TYPE_SM_ITEM_NAME  "SM"
#define X_FOCUSER_2_TYPE_SN_ITEM_NAME  "SN"
#define X_FOCUSER_2_TYPE_SO_ITEM_NAME  "SO"
#define X_FOCUSER_2_TYPE_SP_ITEM_NAME  "SP"
#define X_FOCUSER_2_TYPE_SQ_ITEM_NAME  "SQ"
#define X_FOCUSER_2_TYPE_TA_ITEM_NAME  "TA"
#define X_FOCUSER_2_TYPE_ZZ_ITEM_NAME  "ZZ"

#pragma mark - Private data definition

typedef struct {
	int count;
	indigo_uni_handle *handle;
	indigo_property *x_focuser_1_type_property;
	indigo_property *x_focuser_2_type_property;
	//+ data
	char response[128];
	char firmware[INDIGO_VALUE_SIZE];
	bool can_sync[2];
	// Set when a move request has been accepted and cleared when its handler
	// has run, so a status poll landing in between cannot clear the BUSY
	// state that guards against a second move.
	bool move_pending[2];
	bool update_position[2];
	bool update_temperature[2];
	//- data
} optecfl_private_data;

#pragma mark - Low level code

//+ code

typedef void (*optecfl_field_handler)(indigo_device *device, const char *key, const char *value);

static void focuser_1_focuser_position_handler(indigo_device *device);
static void focuser_1_focuser_steps_handler(indigo_device *device);
static void focuser_2_focuser_position_handler(indigo_device *device);
static void focuser_2_focuser_steps_handler(indigo_device *device);

// Both logical focusers publish X_FOCUSER_TYPE, so each one owns its own
// declaration and the shared helpers resolve it from the focuser number.
static indigo_property *optecfl_type_property(indigo_device *device) {
	return FOCUSER_ID == 1 ? X_FOCUSER_1_TYPE_PROPERTY : X_FOCUSER_2_TYPE_PROPERTY;
}

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

// Reads a "header ... key = value ... END" block. The line budget bounds a
// reply whose END terminator never arrives.
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
		INDIGO_COPY_VALUE(PRIVATE_DATA->firmware, value);
	}
}

static void optecfl_config_field(indigo_device *device, const char *key, const char *value) {
	if (!strcmp(key, "Max Pos")) {
		FOCUSER_LIMITS_MAX_POSITION_ITEM->number.value = FOCUSER_LIMITS_MAX_POSITION_ITEM->number.target = FOCUSER_POSITION_ITEM->number.max = atoi(value);
	} else if (!strcmp(key, "Dev Typ")) {
		// Optec focusers must home and therefore reject the SCCP sync command.
		PRIVATE_DATA->can_sync[FOCUSER_SLOT] = value[0] != 'O';
		indigo_property *type_property = optecfl_type_property(device);
		for (int i = 0; i < type_property->count; i++) {
			indigo_item *item = type_property->items + i;
			if (!strncmp(item->name, value, 2)) {
				indigo_set_switch(type_property, item, true);
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
		// A requested move makes the property BUSY before its handler
		// runs, so the controller's target is only adopted while the
		// driver is not the one driving the move. Otherwise a poll
		// landing in that window would overwrite the requested target.
		int target = atoi(value);
		if (FOCUSER_POSITION_PROPERTY->state != INDIGO_BUSY_STATE && FOCUSER_POSITION_ITEM->number.target != target) {
			FOCUSER_POSITION_ITEM->number.target = target;
			PRIVATE_DATA->update_position[FOCUSER_SLOT] = true;
		}
	} else if (!strcmp(key, "IsMoving")) {
		int moving = atoi(value);
		if (FOCUSER_POSITION_PROPERTY->state != INDIGO_BUSY_STATE && moving == 1) {
			FOCUSER_POSITION_PROPERTY->state = FOCUSER_STEPS_PROPERTY->state = INDIGO_BUSY_STATE;
			PRIVATE_DATA->update_position[FOCUSER_SLOT] = true;
		} else if (FOCUSER_POSITION_PROPERTY->state == INDIGO_BUSY_STATE && moving == 0 && !PRIVATE_DATA->move_pending[FOCUSER_SLOT]) {
			FOCUSER_POSITION_PROPERTY->state = FOCUSER_STEPS_PROPERTY->state = INDIGO_OK_STATE;
			PRIVATE_DATA->update_position[FOCUSER_SLOT] = true;
		}
	}
}

// Opens the shared port and reads the hub block. The generator owns the
// shared reference count, so this runs only for the first connection.
static bool optecfl_open(indigo_device *device) {
	char *name = DEVICE_PORT_ITEM->text.value;
	PRIVATE_DATA->handle = indigo_uni_open_serial_with_speed(name, 115200, INDIGO_LOG_DEBUG);
	if (PRIVATE_DATA->handle == NULL) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "Failed to connect to %s", name);
		return false;
	}
	INDIGO_DRIVER_LOG(DRIVER_NAME, "Connected to %s", name);
	INDIGO_COPY_VALUE(PRIVATE_DATA->firmware, "N/A");
	if (!optecfl_command(device, "<FHGETHUBINFO>") || !optecfl_block(device, optecfl_hub_field)) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "Failed to read hub info");
		indigo_uni_close(&PRIVATE_DATA->handle);
		return false;
	}
	return true;
}

// Called for the last disconnect only; the port belongs to the master
// device, so the message names the driver rather than this device's
// hidden DEVICE_PORT.
static void optecfl_close(indigo_device *device) {
	indigo_uni_close(&PRIVATE_DATA->handle);
	INDIGO_DRIVER_LOG(DRIVER_NAME, "Hub disconnected");
}

// Reads the configuration of one logical focuser. A failure here rolls the
// whole connection attempt back through the generated handler.
static bool optecfl_connect(indigo_device *device) {
	PRIVATE_DATA->move_pending[FOCUSER_SLOT] = false;
	if (!optecfl_command(device, "<F%dGETCONFIG>", FOCUSER_ID) || !optecfl_block(device, optecfl_config_field)) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "Failed to open focuser #%d", FOCUSER_ID);
		return false;
	}
	INDIGO_COPY_VALUE(INFO_DEVICE_FW_REVISION_ITEM->text.value, PRIVATE_DATA->firmware);
	indigo_update_property(device, INFO_PROPERTY, NULL);
	return true;
}

static void optecfl_poll(indigo_device *device) {
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
}

static void optecfl_apply_type(indigo_device *device) {
	indigo_property *type_property = optecfl_type_property(device);
	char *value = NULL;
	for (int i = 0; i < type_property->count; i++) {
		indigo_item *item = type_property->items + i;
		if (item->sw.value) {
			value = item->name;
			break;
		}
	}
	if (value == NULL || !optecfl_echo(device, "SET", "<F%dSCDT%.2s>", FOCUSER_ID, value)) {
		type_property->state = INDIGO_ALERT_STATE;
		return;
	}
	PRIVATE_DATA->can_sync[FOCUSER_SLOT] = value[0] != 'O';
	// Max Pos follows the device type, so re-read the configuration block.
	if (optecfl_command(device, "<F%dGETCONFIG>", FOCUSER_ID) && optecfl_block(device, optecfl_config_field)) {
		indigo_update_property(device, FOCUSER_LIMITS_PROPERTY, NULL);
	}
}

static void optecfl_set_position(indigo_device *device) {
	PRIVATE_DATA->move_pending[FOCUSER_SLOT] = false;
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

static void optecfl_move_steps(indigo_device *device) {
	PRIVATE_DATA->move_pending[FOCUSER_SLOT] = false;
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

static void optecfl_abort(indigo_device *device) {
	PRIVATE_DATA->move_pending[FOCUSER_SLOT] = false;
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
}

//- code

#pragma mark - High level code (focuser_1)
// device_id: focuser_1 type: focuser

static void focuser_1_timer_callback(indigo_device *device) {
	if (!IS_CONNECTED) {
		return;
	}
	//+ focuser_1.on_timer
	optecfl_poll(device);
	indigo_execute_handler_in(device, 1, focuser_1_timer_callback);
	//- focuser_1.on_timer
}

static void focuser_1_connection_handler(indigo_device *device) {
	if (CONNECTION_CONNECTED_ITEM->sw.value) {
		bool connection_result = true;
		if (PRIVATE_DATA->count == 0) {
			connection_result = optecfl_open(device);
		}
		if (connection_result) {
			PRIVATE_DATA->count++;
		}
		if (connection_result) {
			//+ focuser_1.on_connect
			connection_result = optecfl_connect(device);
			//- focuser_1.on_connect
		}
		if (connection_result) {
			indigo_define_property(device, X_FOCUSER_1_TYPE_PROPERTY, NULL);
			CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
			indigo_send_message(device, OK_PROPERTY, "Connected to %s on %s", FOCUSER_1_DEVICE_NAME, DEVICE_PORT_ITEM->text.value);
		} else {
			indigo_send_message(device, ALERT_PROPERTY, "Failed to connect to %s on %s", FOCUSER_1_DEVICE_NAME, DEVICE_PORT_ITEM->text.value);
			if (PRIVATE_DATA->count > 0 && --PRIVATE_DATA->count == 0) {
				optecfl_close(device);
			}
			CONNECTION_PROPERTY->state = INDIGO_ALERT_STATE;
			indigo_set_switch(CONNECTION_PROPERTY, CONNECTION_DISCONNECTED_ITEM, true);
		}
	} else {
		indigo_cancel_pending_handlers(device);
		indigo_delete_property(device, X_FOCUSER_1_TYPE_PROPERTY, NULL);
		if (--PRIVATE_DATA->count == 0) {
			optecfl_close(device);
		}
		indigo_send_message(device, OK_PROPERTY, "Disconnected from %s", device->name);
		CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
	}
	indigo_focuser_change_property(device, NULL, CONNECTION_PROPERTY);
	if (IS_CONNECTED) {
		indigo_execute_handler(device, focuser_1_timer_callback);
	}
}

static void focuser_1_focuser_position_handler(indigo_device *device) {
	FOCUSER_POSITION_PROPERTY->state = INDIGO_OK_STATE;
	//+ focuser_1.FOCUSER_POSITION.on_change
	optecfl_set_position(device);
	//- focuser_1.FOCUSER_POSITION.on_change
	indigo_update_property(device, FOCUSER_POSITION_PROPERTY, NULL);
}

static void focuser_1_focuser_steps_handler(indigo_device *device) {
	FOCUSER_STEPS_PROPERTY->state = INDIGO_OK_STATE;
	//+ focuser_1.FOCUSER_STEPS.on_change
	optecfl_move_steps(device);
	//- focuser_1.FOCUSER_STEPS.on_change
	indigo_update_property(device, FOCUSER_STEPS_PROPERTY, NULL);
}

static void focuser_1_focuser_abort_motion_handler(indigo_device *device) {
	FOCUSER_ABORT_MOTION_PROPERTY->state = INDIGO_OK_STATE;
	//+ focuser_1.FOCUSER_ABORT_MOTION.on_change
	// The urgent abort can overtake a move that is still queued
	// behind the polling callback.
	indigo_cancel_pending_handler(device, focuser_1_focuser_position_handler);
	indigo_cancel_pending_handler(device, focuser_1_focuser_steps_handler);
	optecfl_abort(device);
	//- focuser_1.FOCUSER_ABORT_MOTION.on_change
	indigo_update_property(device, FOCUSER_ABORT_MOTION_PROPERTY, NULL);
}

static void focuser_1_x_focuser_1_type_handler(indigo_device *device) {
	X_FOCUSER_1_TYPE_PROPERTY->state = INDIGO_OK_STATE;
	//+ focuser_1.X_FOCUSER_1_TYPE.on_change
	optecfl_apply_type(device);
	//- focuser_1.X_FOCUSER_1_TYPE.on_change
	indigo_update_property(device, X_FOCUSER_1_TYPE_PROPERTY, NULL);
}

#pragma mark - Device API (focuser_1)

static indigo_result focuser_1_enumerate_properties(indigo_device *device, indigo_client *client, indigo_property *property);

static indigo_result focuser_1_attach(indigo_device *device) {
	if (indigo_focuser_attach(device, DRIVER_NAME, DRIVER_VERSION) == INDIGO_OK) {
		DEVICE_PORT_PROPERTY->hidden = false;
		DEVICE_PORTS_PROPERTY->hidden = false;
		indigo_enumerate_serial_ports(device, DEVICE_PORTS_PROPERTY);
		//+ focuser_1.on_attach
		// The logical focuser number addresses the hub and indexes the
		// shared per focuser state.
		device->gp_bits = 1;
		INFO_PROPERTY->count = 6;
		INDIGO_COPY_VALUE(INFO_DEVICE_MODEL_ITEM->text.value, "FocusLynx");
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
		//- focuser_1.on_attach
		FOCUSER_SPEED_PROPERTY->hidden = true;
		FOCUSER_TEMPERATURE_PROPERTY->hidden = false;
		FOCUSER_REVERSE_MOTION_PROPERTY->hidden = false;
		FOCUSER_ON_POSITION_SET_PROPERTY->hidden = false;
		FOCUSER_LIMITS_PROPERTY->hidden = false;
		//+ focuser_1.FOCUSER_LIMITS.on_attach
		FOCUSER_LIMITS_MIN_POSITION_ITEM->number.min = FOCUSER_LIMITS_MIN_POSITION_ITEM->number.max = FOCUSER_LIMITS_MIN_POSITION_ITEM->number.value = 0;
		//- focuser_1.FOCUSER_LIMITS.on_attach
		FOCUSER_POSITION_PROPERTY->hidden = false;
		//+ focuser_1.FOCUSER_POSITION.on_attach
		FOCUSER_POSITION_ITEM->number.min = 0;
		FOCUSER_POSITION_ITEM->number.step = 1;
		//- focuser_1.FOCUSER_POSITION.on_attach
		FOCUSER_STEPS_PROPERTY->hidden = false;
		//+ focuser_1.FOCUSER_STEPS.on_attach
		FOCUSER_STEPS_ITEM->number.min = 0;
		FOCUSER_STEPS_ITEM->number.max = 99999;
		FOCUSER_STEPS_ITEM->number.step = 1;
		//- focuser_1.FOCUSER_STEPS.on_attach
		FOCUSER_ABORT_MOTION_PROPERTY->hidden = false;
		X_FOCUSER_1_TYPE_PROPERTY = indigo_init_switch_property(NULL, device->name, X_FOCUSER_1_TYPE_PROPERTY_NAME, FOCUSER_MAIN_GROUP, "Focuser type", INDIGO_OK_STATE, INDIGO_RW_PERM, INDIGO_ONE_OF_MANY_RULE, 29);
		if (X_FOCUSER_1_TYPE_PROPERTY == NULL) {
			return INDIGO_FAILED;
		}
		indigo_init_switch_item(X_FOCUSER_1_TYPE_OA_ITEM, X_FOCUSER_1_TYPE_OA_ITEM_NAME, "Optec TCF-Lynx 2\"", true);
		indigo_init_switch_item(X_FOCUSER_1_TYPE_OB_ITEM, X_FOCUSER_1_TYPE_OB_ITEM_NAME, "Optec TCF-Lynx 3\"", false);
		indigo_init_switch_item(X_FOCUSER_1_TYPE_OC_ITEM, X_FOCUSER_1_TYPE_OC_ITEM_NAME, "Optec TCF-Lynx 2\" with Extended Travel", false);
		indigo_init_switch_item(X_FOCUSER_1_TYPE_OD_ITEM, X_FOCUSER_1_TYPE_OD_ITEM_NAME, "Optec Fast Focus Secondary Focuser", false);
		indigo_init_switch_item(X_FOCUSER_1_TYPE_OE_ITEM, X_FOCUSER_1_TYPE_OE_ITEM_NAME, "Optec TCF-S Classic converted", false);
		indigo_init_switch_item(X_FOCUSER_1_TYPE_OF_ITEM, X_FOCUSER_1_TYPE_OF_ITEM_NAME, "Optec TCF-S3 Classic converted", false);
		indigo_init_switch_item(X_FOCUSER_1_TYPE_OG_ITEM, X_FOCUSER_1_TYPE_OG_ITEM_NAME, "Optec Gemini (reserved for future use)", false);
		indigo_init_switch_item(X_FOCUSER_1_TYPE_FA_ITEM, X_FOCUSER_1_TYPE_FA_ITEM_NAME, "FocusLynx QuickSync FT Hi-Torque", false);
		indigo_init_switch_item(X_FOCUSER_1_TYPE_FB_ITEM, X_FOCUSER_1_TYPE_FB_ITEM_NAME, "FocusLynx QuickSync FT Hi-Speed", false);
		indigo_init_switch_item(X_FOCUSER_1_TYPE_FC_ITEM, X_FOCUSER_1_TYPE_FC_ITEM_NAME, "FocusLynx QuickSync SV (reserved for future use)", false);
		indigo_init_switch_item(X_FOCUSER_1_TYPE_SA_ITEM, X_FOCUSER_1_TYPE_SA_ITEM_NAME, "Starlight Focuser FTF2008BCR", false);
		indigo_init_switch_item(X_FOCUSER_1_TYPE_SB_ITEM, X_FOCUSER_1_TYPE_SB_ITEM_NAME, "Starlight Focuser FTF2015BCR", false);
		indigo_init_switch_item(X_FOCUSER_1_TYPE_SC_ITEM, X_FOCUSER_1_TYPE_SC_ITEM_NAME, "Starlight Focuser FTF2020BCR", false);
		indigo_init_switch_item(X_FOCUSER_1_TYPE_SD_ITEM, X_FOCUSER_1_TYPE_SD_ITEM_NAME, "Starlight Focuser FTF2025", false);
		indigo_init_switch_item(X_FOCUSER_1_TYPE_SE_ITEM, X_FOCUSER_1_TYPE_SE_ITEM_NAME, "Starlight Focuser FTF2515B-A", false);
		indigo_init_switch_item(X_FOCUSER_1_TYPE_SF_ITEM, X_FOCUSER_1_TYPE_SF_ITEM_NAME, "Starlight Focuser FTF2525B-A", false);
		indigo_init_switch_item(X_FOCUSER_1_TYPE_SG_ITEM, X_FOCUSER_1_TYPE_SG_ITEM_NAME, "Starlight Focuser FTF2535B-A", false);
		indigo_init_switch_item(X_FOCUSER_1_TYPE_SH_ITEM, X_FOCUSER_1_TYPE_SH_ITEM_NAME, "Starlight Focuser FTF3015B-A", false);
		indigo_init_switch_item(X_FOCUSER_1_TYPE_SI_ITEM, X_FOCUSER_1_TYPE_SI_ITEM_NAME, "Starlight Focuser FTF3025B-A", false);
		indigo_init_switch_item(X_FOCUSER_1_TYPE_SJ_ITEM, X_FOCUSER_1_TYPE_SJ_ITEM_NAME, "Starlight Focuser FTF3035B-A", false);
		indigo_init_switch_item(X_FOCUSER_1_TYPE_SK_ITEM, X_FOCUSER_1_TYPE_SK_ITEM_NAME, "Starlight Focuser FTF3515B-A", false);
		indigo_init_switch_item(X_FOCUSER_1_TYPE_SL_ITEM, X_FOCUSER_1_TYPE_SL_ITEM_NAME, "Starlight Focuser FTF3545B-A", false);
		indigo_init_switch_item(X_FOCUSER_1_TYPE_SM_ITEM, X_FOCUSER_1_TYPE_SM_ITEM_NAME, "Starlight Focuser AP27FOC3E", false);
		indigo_init_switch_item(X_FOCUSER_1_TYPE_SN_ITEM, X_FOCUSER_1_TYPE_SN_ITEM_NAME, "Starlight Focuser AP4FOC3E", false);
		indigo_init_switch_item(X_FOCUSER_1_TYPE_SO_ITEM, X_FOCUSER_1_TYPE_SO_ITEM_NAME, "FeatherTouch Motor Hi-Speed", false);
		indigo_init_switch_item(X_FOCUSER_1_TYPE_SP_ITEM, X_FOCUSER_1_TYPE_SP_ITEM_NAME, "FeatherTouch Motor Hi-Torque", false);
		indigo_init_switch_item(X_FOCUSER_1_TYPE_SQ_ITEM, X_FOCUSER_1_TYPE_SQ_ITEM_NAME, "Starlight Instruments FTM with MicroTouch", false);
		indigo_init_switch_item(X_FOCUSER_1_TYPE_TA_ITEM, X_FOCUSER_1_TYPE_TA_ITEM_NAME, "Televue Focuser with Micro-Touch motor", false);
		indigo_init_switch_item(X_FOCUSER_1_TYPE_ZZ_ITEM, X_FOCUSER_1_TYPE_ZZ_ITEM_NAME, "Default setting, no function", false);
		INDIGO_DEVICE_ATTACH_LOG(DRIVER_NAME, device->name);
		return focuser_1_enumerate_properties(device, NULL, NULL);
	}
	return INDIGO_FAILED;
}

static indigo_result focuser_1_enumerate_properties(indigo_device *device, indigo_client *client, indigo_property *property) {
	if (IS_CONNECTED) {
		INDIGO_DEFINE_MATCHING_PROPERTY(X_FOCUSER_1_TYPE_PROPERTY);
	}
	return indigo_focuser_enumerate_properties(device, client, property);
}

static indigo_result focuser_1_change_property(indigo_device *device, indigo_client *client, indigo_property *property) {
	if (indigo_property_match_changeable(CONNECTION_PROPERTY, property)) {
		if (!indigo_ignore_connection_change(device, property)) {
			indigo_property_copy_values(CONNECTION_PROPERTY, property, false);
			INDIGO_UPDATE_PROPERTY_STATE(CONNECTION_PROPERTY, INDIGO_BUSY_STATE, NULL);
			indigo_execute_handler(device, focuser_1_connection_handler);
		}
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(FOCUSER_POSITION_PROPERTY, property)) {
		if (PRIVATE_DATA->move_pending[FOCUSER_SLOT] || FOCUSER_POSITION_PROPERTY->state == INDIGO_BUSY_STATE) {
			for (int i = 0; i < FOCUSER_POSITION_PROPERTY->count; i++) {
				FOCUSER_POSITION_PROPERTY->items[i].do_update = true;
			}
			FOCUSER_POSITION_PROPERTY->state = INDIGO_ALERT_STATE;
			indigo_update_property(device, FOCUSER_POSITION_PROPERTY, "Motion already in progress");
			return INDIGO_OK;
		}
		//+ focuser_1.FOCUSER_POSITION.on_change_request
		PRIVATE_DATA->move_pending[FOCUSER_SLOT] = true;
		//- focuser_1.FOCUSER_POSITION.on_change_request
		INDIGO_COPY_TARGETS_PROCESS_CHANGE(FOCUSER_POSITION_PROPERTY, focuser_1_focuser_position_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(FOCUSER_STEPS_PROPERTY, property)) {
		if (PRIVATE_DATA->move_pending[FOCUSER_SLOT] || FOCUSER_STEPS_PROPERTY->state == INDIGO_BUSY_STATE || FOCUSER_POSITION_PROPERTY->state == INDIGO_BUSY_STATE) {
			for (int i = 0; i < FOCUSER_STEPS_PROPERTY->count; i++) {
				FOCUSER_STEPS_PROPERTY->items[i].do_update = true;
			}
			FOCUSER_STEPS_PROPERTY->state = INDIGO_ALERT_STATE;
			indigo_update_property(device, FOCUSER_STEPS_PROPERTY, "Motion already in progress");
			return INDIGO_OK;
		}
		//+ focuser_1.FOCUSER_STEPS.on_change_request
		PRIVATE_DATA->move_pending[FOCUSER_SLOT] = true;
		//- focuser_1.FOCUSER_STEPS.on_change_request
		INDIGO_COPY_VALUES_PROCESS_CHANGE(FOCUSER_STEPS_PROPERTY, focuser_1_focuser_steps_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(FOCUSER_ABORT_MOTION_PROPERTY, property)) {
		INDIGO_COPY_VALUES_PROCESS_URGENT_CHANGE(FOCUSER_ABORT_MOTION_PROPERTY, focuser_1_focuser_abort_motion_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(X_FOCUSER_1_TYPE_PROPERTY, property)) {
		INDIGO_COPY_VALUES_PROCESS_CHANGE(X_FOCUSER_1_TYPE_PROPERTY, focuser_1_x_focuser_1_type_handler);
		return INDIGO_OK;
	}
	return indigo_focuser_change_property(device, client, property);
}

static indigo_result focuser_1_detach(indigo_device *device) {
	if (IS_CONNECTED) {
		indigo_set_switch(CONNECTION_PROPERTY, CONNECTION_DISCONNECTED_ITEM, true);
		focuser_1_connection_handler(device);
	}
	indigo_release_property(X_FOCUSER_1_TYPE_PROPERTY);
	INDIGO_DEVICE_DETACH_LOG(DRIVER_NAME, device->name);
	return indigo_focuser_detach(device);
}

#pragma mark - High level code (focuser_2)
// device_id: focuser_2 type: focuser

static void focuser_2_timer_callback(indigo_device *device) {
	if (!IS_CONNECTED) {
		return;
	}
	//+ focuser_2.on_timer
	optecfl_poll(device);
	indigo_execute_handler_in(device, 1, focuser_2_timer_callback);
	//- focuser_2.on_timer
}

static void focuser_2_connection_handler(indigo_device *device) {
	if (CONNECTION_CONNECTED_ITEM->sw.value) {
		bool connection_result = true;
		if (PRIVATE_DATA->count == 0) {
			connection_result = optecfl_open(device->master_device);
		}
		if (connection_result) {
			PRIVATE_DATA->count++;
		}
		if (connection_result) {
			//+ focuser_2.on_connect
			connection_result = optecfl_connect(device);
			//- focuser_2.on_connect
		}
		if (connection_result) {
			indigo_define_property(device, X_FOCUSER_2_TYPE_PROPERTY, NULL);
			CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
			indigo_send_message(device, OK_PROPERTY, "Connected to %s on %s", FOCUSER_2_DEVICE_NAME, DEVICE_PORT_ITEM->text.value);
		} else {
			indigo_send_message(device, ALERT_PROPERTY, "Failed to connect to %s on %s", FOCUSER_2_DEVICE_NAME, DEVICE_PORT_ITEM->text.value);
			if (PRIVATE_DATA->count > 0 && --PRIVATE_DATA->count == 0) {
				optecfl_close(device);
			}
			CONNECTION_PROPERTY->state = INDIGO_ALERT_STATE;
			indigo_set_switch(CONNECTION_PROPERTY, CONNECTION_DISCONNECTED_ITEM, true);
		}
	} else {
		indigo_cancel_pending_handlers(device);
		indigo_delete_property(device, X_FOCUSER_2_TYPE_PROPERTY, NULL);
		if (--PRIVATE_DATA->count == 0) {
			optecfl_close(device);
		}
		indigo_send_message(device, OK_PROPERTY, "Disconnected from %s", device->name);
		CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
	}
	indigo_focuser_change_property(device, NULL, CONNECTION_PROPERTY);
	if (IS_CONNECTED) {
		indigo_execute_handler(device, focuser_2_timer_callback);
	}
}

static void focuser_2_focuser_position_handler(indigo_device *device) {
	FOCUSER_POSITION_PROPERTY->state = INDIGO_OK_STATE;
	//+ focuser_2.FOCUSER_POSITION.on_change
	optecfl_set_position(device);
	//- focuser_2.FOCUSER_POSITION.on_change
	indigo_update_property(device, FOCUSER_POSITION_PROPERTY, NULL);
}

static void focuser_2_focuser_steps_handler(indigo_device *device) {
	FOCUSER_STEPS_PROPERTY->state = INDIGO_OK_STATE;
	//+ focuser_2.FOCUSER_STEPS.on_change
	optecfl_move_steps(device);
	//- focuser_2.FOCUSER_STEPS.on_change
	indigo_update_property(device, FOCUSER_STEPS_PROPERTY, NULL);
}

static void focuser_2_focuser_abort_motion_handler(indigo_device *device) {
	FOCUSER_ABORT_MOTION_PROPERTY->state = INDIGO_OK_STATE;
	//+ focuser_2.FOCUSER_ABORT_MOTION.on_change
	// The urgent abort can overtake a move that is still queued
	// behind the polling callback.
	indigo_cancel_pending_handler(device, focuser_2_focuser_position_handler);
	indigo_cancel_pending_handler(device, focuser_2_focuser_steps_handler);
	optecfl_abort(device);
	//- focuser_2.FOCUSER_ABORT_MOTION.on_change
	indigo_update_property(device, FOCUSER_ABORT_MOTION_PROPERTY, NULL);
}

static void focuser_2_x_focuser_2_type_handler(indigo_device *device) {
	X_FOCUSER_2_TYPE_PROPERTY->state = INDIGO_OK_STATE;
	//+ focuser_2.X_FOCUSER_2_TYPE.on_change
	optecfl_apply_type(device);
	//- focuser_2.X_FOCUSER_2_TYPE.on_change
	indigo_update_property(device, X_FOCUSER_2_TYPE_PROPERTY, NULL);
}

#pragma mark - Device API (focuser_2)

static indigo_result focuser_2_enumerate_properties(indigo_device *device, indigo_client *client, indigo_property *property);

static indigo_result focuser_2_attach(indigo_device *device) {
	if (indigo_focuser_attach(device, DRIVER_NAME, DRIVER_VERSION) == INDIGO_OK) {
		//+ focuser_2.on_attach
		// The logical focuser number addresses the hub and indexes the
		// shared per focuser state.
		device->gp_bits = 2;
		INFO_PROPERTY->count = 6;
		INDIGO_COPY_VALUE(INFO_DEVICE_MODEL_ITEM->text.value, "FocusLynx");
		//- focuser_2.on_attach
		FOCUSER_SPEED_PROPERTY->hidden = true;
		FOCUSER_TEMPERATURE_PROPERTY->hidden = false;
		FOCUSER_REVERSE_MOTION_PROPERTY->hidden = false;
		FOCUSER_ON_POSITION_SET_PROPERTY->hidden = false;
		FOCUSER_LIMITS_PROPERTY->hidden = false;
		//+ focuser_2.FOCUSER_LIMITS.on_attach
		FOCUSER_LIMITS_MIN_POSITION_ITEM->number.min = FOCUSER_LIMITS_MIN_POSITION_ITEM->number.max = FOCUSER_LIMITS_MIN_POSITION_ITEM->number.value = 0;
		//- focuser_2.FOCUSER_LIMITS.on_attach
		FOCUSER_POSITION_PROPERTY->hidden = false;
		//+ focuser_2.FOCUSER_POSITION.on_attach
		FOCUSER_POSITION_ITEM->number.min = 0;
		FOCUSER_POSITION_ITEM->number.step = 1;
		//- focuser_2.FOCUSER_POSITION.on_attach
		FOCUSER_STEPS_PROPERTY->hidden = false;
		//+ focuser_2.FOCUSER_STEPS.on_attach
		FOCUSER_STEPS_ITEM->number.min = 0;
		FOCUSER_STEPS_ITEM->number.max = 99999;
		FOCUSER_STEPS_ITEM->number.step = 1;
		//- focuser_2.FOCUSER_STEPS.on_attach
		FOCUSER_ABORT_MOTION_PROPERTY->hidden = false;
		X_FOCUSER_2_TYPE_PROPERTY = indigo_init_switch_property(NULL, device->name, X_FOCUSER_2_TYPE_PROPERTY_NAME, FOCUSER_MAIN_GROUP, "Focuser type", INDIGO_OK_STATE, INDIGO_RW_PERM, INDIGO_ONE_OF_MANY_RULE, 29);
		if (X_FOCUSER_2_TYPE_PROPERTY == NULL) {
			return INDIGO_FAILED;
		}
		indigo_init_switch_item(X_FOCUSER_2_TYPE_OA_ITEM, X_FOCUSER_2_TYPE_OA_ITEM_NAME, "Optec TCF-Lynx 2\"", true);
		indigo_init_switch_item(X_FOCUSER_2_TYPE_OB_ITEM, X_FOCUSER_2_TYPE_OB_ITEM_NAME, "Optec TCF-Lynx 3\"", false);
		indigo_init_switch_item(X_FOCUSER_2_TYPE_OC_ITEM, X_FOCUSER_2_TYPE_OC_ITEM_NAME, "Optec TCF-Lynx 2\" with Extended Travel", false);
		indigo_init_switch_item(X_FOCUSER_2_TYPE_OD_ITEM, X_FOCUSER_2_TYPE_OD_ITEM_NAME, "Optec Fast Focus Secondary Focuser", false);
		indigo_init_switch_item(X_FOCUSER_2_TYPE_OE_ITEM, X_FOCUSER_2_TYPE_OE_ITEM_NAME, "Optec TCF-S Classic converted", false);
		indigo_init_switch_item(X_FOCUSER_2_TYPE_OF_ITEM, X_FOCUSER_2_TYPE_OF_ITEM_NAME, "Optec TCF-S3 Classic converted", false);
		indigo_init_switch_item(X_FOCUSER_2_TYPE_OG_ITEM, X_FOCUSER_2_TYPE_OG_ITEM_NAME, "Optec Gemini (reserved for future use)", false);
		indigo_init_switch_item(X_FOCUSER_2_TYPE_FA_ITEM, X_FOCUSER_2_TYPE_FA_ITEM_NAME, "FocusLynx QuickSync FT Hi-Torque", false);
		indigo_init_switch_item(X_FOCUSER_2_TYPE_FB_ITEM, X_FOCUSER_2_TYPE_FB_ITEM_NAME, "FocusLynx QuickSync FT Hi-Speed", false);
		indigo_init_switch_item(X_FOCUSER_2_TYPE_FC_ITEM, X_FOCUSER_2_TYPE_FC_ITEM_NAME, "FocusLynx QuickSync SV (reserved for future use)", false);
		indigo_init_switch_item(X_FOCUSER_2_TYPE_SA_ITEM, X_FOCUSER_2_TYPE_SA_ITEM_NAME, "Starlight Focuser FTF2008BCR", false);
		indigo_init_switch_item(X_FOCUSER_2_TYPE_SB_ITEM, X_FOCUSER_2_TYPE_SB_ITEM_NAME, "Starlight Focuser FTF2015BCR", false);
		indigo_init_switch_item(X_FOCUSER_2_TYPE_SC_ITEM, X_FOCUSER_2_TYPE_SC_ITEM_NAME, "Starlight Focuser FTF2020BCR", false);
		indigo_init_switch_item(X_FOCUSER_2_TYPE_SD_ITEM, X_FOCUSER_2_TYPE_SD_ITEM_NAME, "Starlight Focuser FTF2025", false);
		indigo_init_switch_item(X_FOCUSER_2_TYPE_SE_ITEM, X_FOCUSER_2_TYPE_SE_ITEM_NAME, "Starlight Focuser FTF2515B-A", false);
		indigo_init_switch_item(X_FOCUSER_2_TYPE_SF_ITEM, X_FOCUSER_2_TYPE_SF_ITEM_NAME, "Starlight Focuser FTF2525B-A", false);
		indigo_init_switch_item(X_FOCUSER_2_TYPE_SG_ITEM, X_FOCUSER_2_TYPE_SG_ITEM_NAME, "Starlight Focuser FTF2535B-A", false);
		indigo_init_switch_item(X_FOCUSER_2_TYPE_SH_ITEM, X_FOCUSER_2_TYPE_SH_ITEM_NAME, "Starlight Focuser FTF3015B-A", false);
		indigo_init_switch_item(X_FOCUSER_2_TYPE_SI_ITEM, X_FOCUSER_2_TYPE_SI_ITEM_NAME, "Starlight Focuser FTF3025B-A", false);
		indigo_init_switch_item(X_FOCUSER_2_TYPE_SJ_ITEM, X_FOCUSER_2_TYPE_SJ_ITEM_NAME, "Starlight Focuser FTF3035B-A", false);
		indigo_init_switch_item(X_FOCUSER_2_TYPE_SK_ITEM, X_FOCUSER_2_TYPE_SK_ITEM_NAME, "Starlight Focuser FTF3515B-A", false);
		indigo_init_switch_item(X_FOCUSER_2_TYPE_SL_ITEM, X_FOCUSER_2_TYPE_SL_ITEM_NAME, "Starlight Focuser FTF3545B-A", false);
		indigo_init_switch_item(X_FOCUSER_2_TYPE_SM_ITEM, X_FOCUSER_2_TYPE_SM_ITEM_NAME, "Starlight Focuser AP27FOC3E", false);
		indigo_init_switch_item(X_FOCUSER_2_TYPE_SN_ITEM, X_FOCUSER_2_TYPE_SN_ITEM_NAME, "Starlight Focuser AP4FOC3E", false);
		indigo_init_switch_item(X_FOCUSER_2_TYPE_SO_ITEM, X_FOCUSER_2_TYPE_SO_ITEM_NAME, "FeatherTouch Motor Hi-Speed", false);
		indigo_init_switch_item(X_FOCUSER_2_TYPE_SP_ITEM, X_FOCUSER_2_TYPE_SP_ITEM_NAME, "FeatherTouch Motor Hi-Torque", false);
		indigo_init_switch_item(X_FOCUSER_2_TYPE_SQ_ITEM, X_FOCUSER_2_TYPE_SQ_ITEM_NAME, "Starlight Instruments FTM with MicroTouch", false);
		indigo_init_switch_item(X_FOCUSER_2_TYPE_TA_ITEM, X_FOCUSER_2_TYPE_TA_ITEM_NAME, "Televue Focuser with Micro-Touch motor", false);
		indigo_init_switch_item(X_FOCUSER_2_TYPE_ZZ_ITEM, X_FOCUSER_2_TYPE_ZZ_ITEM_NAME, "Default setting, no function", false);
		INDIGO_DEVICE_ATTACH_LOG(DRIVER_NAME, device->name);
		return focuser_2_enumerate_properties(device, NULL, NULL);
	}
	return INDIGO_FAILED;
}

static indigo_result focuser_2_enumerate_properties(indigo_device *device, indigo_client *client, indigo_property *property) {
	if (IS_CONNECTED) {
		INDIGO_DEFINE_MATCHING_PROPERTY(X_FOCUSER_2_TYPE_PROPERTY);
	}
	return indigo_focuser_enumerate_properties(device, client, property);
}

static indigo_result focuser_2_change_property(indigo_device *device, indigo_client *client, indigo_property *property) {
	if (indigo_property_match_changeable(CONNECTION_PROPERTY, property)) {
		if (!indigo_ignore_connection_change(device, property)) {
			indigo_property_copy_values(CONNECTION_PROPERTY, property, false);
			INDIGO_UPDATE_PROPERTY_STATE(CONNECTION_PROPERTY, INDIGO_BUSY_STATE, NULL);
			indigo_execute_handler(device, focuser_2_connection_handler);
		}
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(FOCUSER_POSITION_PROPERTY, property)) {
		if (PRIVATE_DATA->move_pending[FOCUSER_SLOT] || FOCUSER_POSITION_PROPERTY->state == INDIGO_BUSY_STATE) {
			for (int i = 0; i < FOCUSER_POSITION_PROPERTY->count; i++) {
				FOCUSER_POSITION_PROPERTY->items[i].do_update = true;
			}
			FOCUSER_POSITION_PROPERTY->state = INDIGO_ALERT_STATE;
			indigo_update_property(device, FOCUSER_POSITION_PROPERTY, "Motion already in progress");
			return INDIGO_OK;
		}
		//+ focuser_2.FOCUSER_POSITION.on_change_request
		PRIVATE_DATA->move_pending[FOCUSER_SLOT] = true;
		//- focuser_2.FOCUSER_POSITION.on_change_request
		INDIGO_COPY_TARGETS_PROCESS_CHANGE(FOCUSER_POSITION_PROPERTY, focuser_2_focuser_position_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(FOCUSER_STEPS_PROPERTY, property)) {
		if (PRIVATE_DATA->move_pending[FOCUSER_SLOT] || FOCUSER_STEPS_PROPERTY->state == INDIGO_BUSY_STATE || FOCUSER_POSITION_PROPERTY->state == INDIGO_BUSY_STATE) {
			for (int i = 0; i < FOCUSER_STEPS_PROPERTY->count; i++) {
				FOCUSER_STEPS_PROPERTY->items[i].do_update = true;
			}
			FOCUSER_STEPS_PROPERTY->state = INDIGO_ALERT_STATE;
			indigo_update_property(device, FOCUSER_STEPS_PROPERTY, "Motion already in progress");
			return INDIGO_OK;
		}
		//+ focuser_2.FOCUSER_STEPS.on_change_request
		PRIVATE_DATA->move_pending[FOCUSER_SLOT] = true;
		//- focuser_2.FOCUSER_STEPS.on_change_request
		INDIGO_COPY_VALUES_PROCESS_CHANGE(FOCUSER_STEPS_PROPERTY, focuser_2_focuser_steps_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(FOCUSER_ABORT_MOTION_PROPERTY, property)) {
		INDIGO_COPY_VALUES_PROCESS_URGENT_CHANGE(FOCUSER_ABORT_MOTION_PROPERTY, focuser_2_focuser_abort_motion_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(X_FOCUSER_2_TYPE_PROPERTY, property)) {
		INDIGO_COPY_VALUES_PROCESS_CHANGE(X_FOCUSER_2_TYPE_PROPERTY, focuser_2_x_focuser_2_type_handler);
		return INDIGO_OK;
	}
	return indigo_focuser_change_property(device, client, property);
}

static indigo_result focuser_2_detach(indigo_device *device) {
	if (IS_CONNECTED) {
		indigo_set_switch(CONNECTION_PROPERTY, CONNECTION_DISCONNECTED_ITEM, true);
		focuser_2_connection_handler(device);
	}
	indigo_release_property(X_FOCUSER_2_TYPE_PROPERTY);
	INDIGO_DEVICE_DETACH_LOG(DRIVER_NAME, device->name);
	return indigo_focuser_detach(device);
}

#pragma mark - Device templates

static indigo_device focuser_1_template = INDIGO_DEVICE_INITIALIZER(FOCUSER_1_DEVICE_NAME, focuser_1_attach, focuser_1_enumerate_properties, focuser_1_change_property, NULL, focuser_1_detach);

static indigo_device focuser_2_template = INDIGO_DEVICE_INITIALIZER(FOCUSER_2_DEVICE_NAME, focuser_2_attach, focuser_2_enumerate_properties, focuser_2_change_property, NULL, focuser_2_detach);

#pragma mark - Main code

indigo_result indigo_focuser_optecfl(indigo_driver_action action, indigo_driver_info *info) {
	static indigo_driver_action last_action = INDIGO_DRIVER_SHUTDOWN;
	static optecfl_private_data *private_data = NULL;
	static indigo_device *focuser_1 = NULL;
	static indigo_device *focuser_2 = NULL;

	SET_DRIVER_INFO(info, DRIVER_LABEL, __FUNCTION__, DRIVER_VERSION, true, last_action);

	if (action == last_action) {
		return INDIGO_OK;
	}

	switch (action) {
		case INDIGO_DRIVER_INIT: {
			last_action = action;
			private_data = (optecfl_private_data *)indigo_safe_malloc(sizeof(optecfl_private_data));
			focuser_1 = (indigo_device *)indigo_safe_malloc_copy(sizeof(indigo_device), &focuser_1_template);
			focuser_1->private_data = private_data;
			indigo_attach_device(focuser_1);
			focuser_2 = (indigo_device *)indigo_safe_malloc_copy(sizeof(indigo_device), &focuser_2_template);
			focuser_2->private_data = private_data;
			focuser_2->master_device = focuser_1;
			indigo_attach_device(focuser_2);
			break;

		}
		case INDIGO_DRIVER_SHUTDOWN: {
			VERIFY_NOT_CONNECTED(focuser_1);
			VERIFY_NOT_CONNECTED(focuser_2);
			last_action = action;
			if (focuser_2 != NULL) {
				indigo_detach_device(focuser_2);
				indigo_safe_free(focuser_2);
				focuser_2 = NULL;
			}
			if (focuser_1 != NULL) {
				indigo_detach_device(focuser_1);
				indigo_safe_free(focuser_1);
				focuser_1 = NULL;
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
