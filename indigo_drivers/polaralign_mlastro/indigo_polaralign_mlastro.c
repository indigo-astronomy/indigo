// Copyright (c) 2026 by Rumen G.Bogdanovski
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

// This file generated from indigo_polaralign_mlastro.driver

#pragma mark - Includes

#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <assert.h>
#include <indigo/indigo_driver_xml.h>
#include <indigo/indigo_polaralign_driver.h>
#include <indigo/indigo_uni_io.h>

#include "indigo_polaralign_mlastro.h"

#pragma mark - Common definitions

#define DRIVER_VERSION       0x03000004
#define DRIVER_NAME          "indigo_polaralign_mlastro"
#define DRIVER_LABEL         "MLAstro RPA"
#define POLARALIGN_DEVICE_NAME DRIVER_LABEL
#define PRIVATE_DATA         ((mlastro_private_data *)device->private_data)

//+ define

#define MLASTRO_BAUDRATE     115200
#define MLASTRO_CMD_LEN      128
#define MLASTRO_RESPONSE_LEN 2048
#define MLASTRO_HANDSHAKE_ATTEMPTS 5
#define MLASTRO_MAX_LINES    16
#define MLASTRO_MIN_MOVE     (0.5 / 3600.0)
/* Firmware 1.8.1 with its communication watchdog on (the default) drops serial
   control when no command arrives for about 1.15 s, so the poll has to stay
   well inside that. */
#define MLASTRO_POLL_INTERVAL 0.5

//- define

#pragma mark - Property definitions

#define X_MLASTRO_SPEED_PROPERTY       (PRIVATE_DATA->x_mlastro_speed_property)
#define X_MLASTRO_SPEED_ITEM           (X_MLASTRO_SPEED_PROPERTY->items + 0)

#define X_MLASTRO_SPEED_PROPERTY_NAME  "X_MLASTRO_SPEED"
#define X_MLASTRO_SPEED_ITEM_NAME      "SPEED"

#define X_MLASTRO_BACKLASH_ENABLE_PROPERTY           (PRIVATE_DATA->x_mlastro_backlash_enable_property)
#define X_MLASTRO_BACKLASH_ENABLE_ENABLED_ITEM       (X_MLASTRO_BACKLASH_ENABLE_PROPERTY->items + 0)
#define X_MLASTRO_BACKLASH_ENABLE_DISABLED_ITEM      (X_MLASTRO_BACKLASH_ENABLE_PROPERTY->items + 1)

#define X_MLASTRO_BACKLASH_ENABLE_PROPERTY_NAME      "X_MLASTRO_BACKLASH_ENABLE"
#define X_MLASTRO_BACKLASH_ENABLE_ENABLED_ITEM_NAME  "ENABLED"
#define X_MLASTRO_BACKLASH_ENABLE_DISABLED_ITEM_NAME "DISABLED"

#define X_MLASTRO_BACKLASH_STEPS_PROPERTY      (PRIVATE_DATA->x_mlastro_backlash_steps_property)
#define X_MLASTRO_BACKLASH_STEPS_ALT_ITEM      (X_MLASTRO_BACKLASH_STEPS_PROPERTY->items + 0)
#define X_MLASTRO_BACKLASH_STEPS_AZ_ITEM       (X_MLASTRO_BACKLASH_STEPS_PROPERTY->items + 1)

#define X_MLASTRO_BACKLASH_STEPS_PROPERTY_NAME "X_MLASTRO_BACKLASH_STEPS"
#define X_MLASTRO_BACKLASH_STEPS_ALT_ITEM_NAME "ALT"
#define X_MLASTRO_BACKLASH_STEPS_AZ_ITEM_NAME  "AZ"

#define X_MLASTRO_STATUS_PROPERTY      (PRIVATE_DATA->x_mlastro_status_property)
#define X_MLASTRO_STATUS_ITEM          (X_MLASTRO_STATUS_PROPERTY->items + 0)

#define X_MLASTRO_STATUS_PROPERTY_NAME "X_MLASTRO_STATUS"
#define X_MLASTRO_STATUS_ITEM_NAME     "STATUS"

#define X_MLASTRO_HOMED_PROPERTY       (PRIVATE_DATA->x_mlastro_homed_property)
#define X_MLASTRO_HOMED_ITEM           (X_MLASTRO_HOMED_PROPERTY->items + 0)

#define X_MLASTRO_HOMED_PROPERTY_NAME  "X_MLASTRO_HOMED"
#define X_MLASTRO_HOMED_ITEM_NAME      "HOMED"

#define X_MLASTRO_GOTO_HOME_PROPERTY      (PRIVATE_DATA->x_mlastro_goto_home_property)
#define X_MLASTRO_GOTO_HOME_ITEM          (X_MLASTRO_GOTO_HOME_PROPERTY->items + 0)

#define X_MLASTRO_GOTO_HOME_PROPERTY_NAME "X_MLASTRO_GOTO_HOME"
#define X_MLASTRO_GOTO_HOME_ITEM_NAME     "GOTO_HOME"

#define X_MLASTRO_CLEAR_HOME_PROPERTY      (PRIVATE_DATA->x_mlastro_clear_home_property)
#define X_MLASTRO_CLEAR_HOME_ITEM          (X_MLASTRO_CLEAR_HOME_PROPERTY->items + 0)

#define X_MLASTRO_CLEAR_HOME_PROPERTY_NAME "X_MLASTRO_CLEAR_HOME"
#define X_MLASTRO_CLEAR_HOME_ITEM_NAME     "CLEAR_HOME"

#define X_MLASTRO_RESET_ERROR_PROPERTY      (PRIVATE_DATA->x_mlastro_reset_error_property)
#define X_MLASTRO_RESET_ERROR_ITEM          (X_MLASTRO_RESET_ERROR_PROPERTY->items + 0)

#define X_MLASTRO_RESET_ERROR_PROPERTY_NAME "X_MLASTRO_RESET_ERROR"
#define X_MLASTRO_RESET_ERROR_ITEM_NAME     "RESET_ERROR"

#pragma mark - Private data definition

typedef struct {
	indigo_uni_handle *handle;
	indigo_property *x_mlastro_speed_property;
	indigo_property *x_mlastro_backlash_enable_property;
	indigo_property *x_mlastro_backlash_steps_property;
	indigo_property *x_mlastro_status_property;
	indigo_property *x_mlastro_homed_property;
	indigo_property *x_mlastro_goto_home_property;
	indigo_property *x_mlastro_clear_home_property;
	indigo_property *x_mlastro_reset_error_property;
	//+ data
	char response[MLASTRO_RESPONSE_LEN];
	double current_alt;    /* degrees, relative to the last SetH:1 reference */
	double current_az;     /* degrees, relative to the last SetH:1 reference */
	bool moving;
	bool error;
	bool homed;
	bool control_lost;
	//- data
} mlastro_private_data;

#pragma mark - Low level code

//+ code

static void polaralign_connection_handler(indigo_device *device);

/* Lines telling that the controller no longer takes commands from this port:
   "DISCONNECTED" when the Web UI takes control back, "REBOOTING..." after
   Save&Reboot, the "error: Serial heartbeat timeout -> ESTOP" push when the
   communication watchdog expires, and the "error: Not connected. ..." reply the
   firmware gives every command afterwards. */
static bool mlastro_control_lost_line(const char *line) {
	return !strncmp(line, "DISCONNECTED", 12) || !strncmp(line, "REBOOTING", 9) || !strncmp(line, "error: Serial heartbeat timeout", 31) || !strncmp(line, "error: Not connected", 20);
}

/* Lines the controller pushes on its own: "AzAN:COMPLETED", "AlAN:COMPLETED",
   "AAll:COMPLETED", "HOME_COMPLETED", "SetH:COMPLETED", "SetH:STOPPED", the
   upper-case "ERROR:Sys:..." status line, the lines of mlastro_control_lost_line(),
   and the firmware's boot banner and WiFi log. None of them is a reply to a command. */
static void mlastro_unsolicited(indigo_device *device, const char *line) {
	if (mlastro_control_lost_line(line)) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "Controller released serial control: '%s'", line);
		PRIVATE_DATA->control_lost = true;
	} else if (!strncmp(line, "ERROR:", 6)) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "Controller error report: '%s'", line);
	} else {
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "Unsolicited line: '%s'", line);
	}
}

/* One line from the controller without its CR/LF terminator; returns its length. */
static long mlastro_read_line(indigo_device *device, double first_byte_timeout) {
	long result = indigo_uni_read_section2(PRIVATE_DATA->handle, PRIVATE_DATA->response, sizeof(PRIVATE_DATA->response) - 1, "\n", "\r", INDIGO_DELAY(first_byte_timeout), INDIGO_DELAY(0.5));
	if (result < 0) {
		return result;
	}
	PRIVATE_DATA->response[result] = '\0';
	while (result > 0 && (PRIVATE_DATA->response[result - 1] == '\n' || PRIVATE_DATA->response[result - 1] == '\r')) {
		PRIVATE_DATA->response[--result] = '\0';
	}
	return result;
}

/* Sends one command line and reads lines until its reply arrives: a '<' telemetry
   frame for "?", an "ok..." or "error: ..." line for everything else. Lines already
   waiting and lines arriving before the reply are unsolicited pushes, handed to
   mlastro_unsolicited() instead of being discarded or mistaken for the reply. */
static bool mlastro_exchange(indigo_device *device, const char *command, bool telemetry, double timeout) {
	for (int i = 0; i < MLASTRO_MAX_LINES * 4 && indigo_uni_wait_for_data(PRIVATE_DATA->handle, 0) > 0; i++) {
		long result = mlastro_read_line(device, 0.1);
		if (result < 0) {
			return false;
		}
		if (result > 0) {
			mlastro_unsolicited(device, PRIVATE_DATA->response);
		}
	}
	PRIVATE_DATA->response[0] = '\0';
	if (PRIVATE_DATA->control_lost) {
		return false;
	}
	if (indigo_uni_printf(PRIVATE_DATA->handle, "%s\n", command) <= 0) {
		return false;
	}
	for (int i = 0; i < MLASTRO_MAX_LINES; i++) {
		if (indigo_uni_wait_for_data(PRIVATE_DATA->handle, INDIGO_DELAY(timeout)) <= 0) {
			PRIVATE_DATA->response[0] = '\0';
			return false;
		}
		long result = mlastro_read_line(device, 0.5);
		if (result < 0) {
			return false;
		}
		if (result == 0) {
			continue;
		}
		if (mlastro_control_lost_line(PRIVATE_DATA->response)) {
			mlastro_unsolicited(device, PRIVATE_DATA->response);
			return false;
		}
		if (telemetry ? PRIVATE_DATA->response[0] == '<' : (!strncmp(PRIVATE_DATA->response, "ok", 2) || !strncmp(PRIVATE_DATA->response, "error", 5))) {
			return true;
		}
		mlastro_unsolicited(device, PRIVATE_DATA->response);
	}
	PRIVATE_DATA->response[0] = '\0';
	return false;
}

/* mlastro_exchange() plus the "ok" acknowledgement check every writable
   command in the protocol replies with (errors reply "error: ...\n"). */
static bool mlastro_command_ok(indigo_device *device, char *command, ...) {
	char formatted[MLASTRO_CMD_LEN];
	va_list args;
	va_start(args, command);
	vsnprintf(formatted, sizeof(formatted), command, args);
	va_end(args);
	if (!mlastro_exchange(device, formatted, false, 2)) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "No response to '%s'", formatted);
		return false;
	}
	if (strncmp(PRIVATE_DATA->response, "ok", 2)) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "'%s' -> '%s'", formatted, PRIVATE_DATA->response);
		return false;
	}
	return true;
}

/* Signed decimal degrees -> whole degrees/arcmin/arcsec + direction, as the
   protocol's ReDe/ReAM/ReAS and AzED/AzEM/AzES/AlED/AlEM/AlES fields require. */
static void mlastro_degrees_to_dms(double degrees, int *d, int *m, int *s, bool *positive) {
	*positive = degrees >= 0.0;
	double abs_deg = fabs(degrees);
	*d = (int)abs_deg;
	double rem = (abs_deg - *d) * 60.0;
	*m = (int)rem;
	*s = (int)((rem - *m) * 60.0 + 0.5);
	if (*s >= 60) {
		*s -= 60;
		*m += 1;
	}
	if (*m >= 60) {
		*m -= 60;
		*d += 1;
	}
}

/* Parse '<STATUS|Mpos:az,alt|>key:val,key:val,...' telemetry and return whether
   the position changed. The position is AzPH/AlPH, the angle from the SetH:1
   reference; Mpos is only the angle moved since the last motion started and is
   used just as a fallback for firmware that does not report AzPH/AlPH. notify
   must be false while called from mlastro_open(): the X_MLASTRO_* properties are
   not defined to the client yet at that point, and updating an undefined
   property is a protocol violation. A property with a change in flight
   (BUSY) is left alone: the client's new values are already copied into it
   and the reply to this poll still carries the old setting. */
static bool mlastro_parse_telemetry(indigo_device *device, const char *line, bool notify) {
	const char *lt = strchr(line, '<');
	const char *pipe1 = strchr(line, '|');
	if (lt && pipe1 && pipe1 > lt) {
		char status[32] = { 0 };
		int len = (int)(pipe1 - lt - 1);
		if (len > 0 && len < (int)sizeof(status)) {
			strncpy(status, lt + 1, len);
			status[len] = '\0';
			PRIVATE_DATA->moving = !strcmp(status, "MOVING") || !strcmp(status, "ALIGNING") || !strcmp(status, "HOMING") || !strcmp(status, "CALIBRATING");
			PRIVATE_DATA->error = !strcmp(status, "ERROR");
			indigo_property_state status_state = PRIVATE_DATA->error ? INDIGO_ALERT_STATE : INDIGO_OK_STATE;
			bool status_changed = strcmp(X_MLASTRO_STATUS_ITEM->text.value, status) || X_MLASTRO_STATUS_PROPERTY->state != status_state;
			INDIGO_COPY_VALUE(X_MLASTRO_STATUS_ITEM->text.value, status);
			X_MLASTRO_STATUS_PROPERTY->state = status_state;
			if (notify && status_changed) {
				indigo_update_property(device, X_MLASTRO_STATUS_PROPERTY, NULL);
			}
		}
	}
	double mpos_az = 0, mpos_alt = 0, ph_az = 0, ph_alt = 0;
	const char *mpos = strstr(line, "Mpos:");
	bool have_mpos = mpos && sscanf(mpos + 5, "%lf,%lf", &mpos_az, &mpos_alt) == 2;
	bool have_ph_az = false, have_ph_alt = false;
	// indigo_set_switch() only flags items dirty; it does not itself notify
	// the client. Track which multi-item properties changed while parsing
	// and push each one once, after the loop, instead of per token.
	bool speed_changed = false, steps_changed = false, direction_az_changed = false, direction_alt_changed = false;
	bool limits_changed = false, backlash_enable_changed = false, backlash_steps_changed = false;
	const char *data_start = strstr(line, "|>");
	if (data_start) {
		char data[MLASTRO_RESPONSE_LEN];
		strncpy(data, data_start + 2, sizeof(data) - 1);
		data[sizeof(data) - 1] = '\0';
		char *save = NULL;
		char *token = strtok_r(data, ",", &save);
		while (token) {
			char key[16] = { 0 }, val[32] = { 0 };
			if (sscanf(token, "%15[^:]:%31s", key, val) == 2) {
				double v = indigo_atod(val);
				if (!strcmp(key, "Home")) {
					bool homed = v != 0.0;
					if (homed != PRIVATE_DATA->homed) {
						PRIVATE_DATA->homed = homed;
						X_MLASTRO_HOMED_ITEM->light.value = homed ? INDIGO_OK_STATE : INDIGO_IDLE_STATE;
						if (notify) {
							indigo_update_property(device, X_MLASTRO_HOMED_PROPERTY, NULL);
						}
					}
				} else if (!strcmp(key, "AzPH")) {
					ph_az = v;
					have_ph_az = true;
				} else if (!strcmp(key, "AlPH")) {
					ph_alt = v;
					have_ph_alt = true;
				} else if (!strcmp(key, "SLvl") && X_MLASTRO_SPEED_PROPERTY->state != INDIGO_BUSY_STATE) {
					speed_changed = speed_changed || X_MLASTRO_SPEED_ITEM->number.value != v;
					X_MLASTRO_SPEED_ITEM->number.value = v;
				} else if (!strcmp(key, "AzSD") && POLARALIGN_STEPS_PER_DEGREE_PROPERTY->state != INDIGO_BUSY_STATE) {
					steps_changed = steps_changed || POLARALIGN_STEPS_PER_DEGREE_AZ_ITEM->number.value != v;
					POLARALIGN_STEPS_PER_DEGREE_AZ_ITEM->number.value = v;
				} else if (!strcmp(key, "AlSD") && POLARALIGN_STEPS_PER_DEGREE_PROPERTY->state != INDIGO_BUSY_STATE) {
					steps_changed = steps_changed || POLARALIGN_STEPS_PER_DEGREE_ALT_ITEM->number.value != v;
					POLARALIGN_STEPS_PER_DEGREE_ALT_ITEM->number.value = v;
				} else if (!strcmp(key, "AzRD") && POLARALIGN_DIRECTION_AZ_PROPERTY->state != INDIGO_BUSY_STATE) {
					direction_az_changed = POLARALIGN_DIRECTION_AZ_REVERSED_ITEM->sw.value != (v != 0.0);
					indigo_set_switch(POLARALIGN_DIRECTION_AZ_PROPERTY, v != 0.0 ? POLARALIGN_DIRECTION_AZ_REVERSED_ITEM : POLARALIGN_DIRECTION_AZ_NORMAL_ITEM, true);
				} else if (!strcmp(key, "AlRD") && POLARALIGN_DIRECTION_ALT_PROPERTY->state != INDIGO_BUSY_STATE) {
					direction_alt_changed = POLARALIGN_DIRECTION_ALT_REVERSED_ITEM->sw.value != (v != 0.0);
					indigo_set_switch(POLARALIGN_DIRECTION_ALT_PROPERTY, v != 0.0 ? POLARALIGN_DIRECTION_ALT_REVERSED_ITEM : POLARALIGN_DIRECTION_ALT_NORMAL_ITEM, true);
				} else if (!strcmp(key, "AzL1") && POLARALIGN_LIMITS_PROPERTY->state != INDIGO_BUSY_STATE) {
					limits_changed = limits_changed || POLARALIGN_LIMITS_MIN_POSITION_AZ_ITEM->number.value != v * 60.0;
					POLARALIGN_LIMITS_MIN_POSITION_AZ_ITEM->number.value = v * 60.0;
				} else if (!strcmp(key, "AzL2") && POLARALIGN_LIMITS_PROPERTY->state != INDIGO_BUSY_STATE) {
					limits_changed = limits_changed || POLARALIGN_LIMITS_MAX_POSITION_AZ_ITEM->number.value != v * 60.0;
					POLARALIGN_LIMITS_MAX_POSITION_AZ_ITEM->number.value = v * 60.0;
				} else if (!strcmp(key, "AlL1") && POLARALIGN_LIMITS_PROPERTY->state != INDIGO_BUSY_STATE) {
					limits_changed = limits_changed || POLARALIGN_LIMITS_MIN_POSITION_ALT_ITEM->number.value != v * 60.0;
					POLARALIGN_LIMITS_MIN_POSITION_ALT_ITEM->number.value = v * 60.0;
				} else if (!strcmp(key, "AlL2") && POLARALIGN_LIMITS_PROPERTY->state != INDIGO_BUSY_STATE) {
					limits_changed = limits_changed || POLARALIGN_LIMITS_MAX_POSITION_ALT_ITEM->number.value != v * 60.0;
					POLARALIGN_LIMITS_MAX_POSITION_ALT_ITEM->number.value = v * 60.0;
				} else if (!strcmp(key, "Back") && X_MLASTRO_BACKLASH_ENABLE_PROPERTY->state != INDIGO_BUSY_STATE) {
					backlash_enable_changed = X_MLASTRO_BACKLASH_ENABLE_ENABLED_ITEM->sw.value != (v != 0.0);
					indigo_set_switch(X_MLASTRO_BACKLASH_ENABLE_PROPERTY, v != 0.0 ? X_MLASTRO_BACKLASH_ENABLE_ENABLED_ITEM : X_MLASTRO_BACKLASH_ENABLE_DISABLED_ITEM, true);
				} else if (!strcmp(key, "AzBl") && X_MLASTRO_BACKLASH_STEPS_PROPERTY->state != INDIGO_BUSY_STATE) {
					backlash_steps_changed = backlash_steps_changed || X_MLASTRO_BACKLASH_STEPS_AZ_ITEM->number.value != v;
					X_MLASTRO_BACKLASH_STEPS_AZ_ITEM->number.value = v;
				} else if (!strcmp(key, "AlBl") && X_MLASTRO_BACKLASH_STEPS_PROPERTY->state != INDIGO_BUSY_STATE) {
					backlash_steps_changed = backlash_steps_changed || X_MLASTRO_BACKLASH_STEPS_ALT_ITEM->number.value != v;
					X_MLASTRO_BACKLASH_STEPS_ALT_ITEM->number.value = v;
				}
			}
			token = strtok_r(NULL, ",", &save);
		}
	}
	bool position_changed = false;
	if ((have_ph_az && have_ph_alt) || have_mpos) {
		double az = have_ph_az && have_ph_alt ? ph_az : mpos_az;
		double alt = have_ph_az && have_ph_alt ? ph_alt : mpos_alt;
		position_changed = az != PRIVATE_DATA->current_az || alt != PRIVATE_DATA->current_alt;
		PRIVATE_DATA->current_az = az;
		PRIVATE_DATA->current_alt = alt;
		POLARALIGN_OFFSET_ALT_ITEM->number.value = alt * 60.0;
		POLARALIGN_OFFSET_AZ_ITEM->number.value = az * 60.0;
	}
	if (notify) {
		if (speed_changed) {
			indigo_update_property(device, X_MLASTRO_SPEED_PROPERTY, NULL);
		}
		if (steps_changed) {
			indigo_update_property(device, POLARALIGN_STEPS_PER_DEGREE_PROPERTY, NULL);
		}
		if (direction_az_changed) {
			indigo_update_property(device, POLARALIGN_DIRECTION_AZ_PROPERTY, NULL);
		}
		if (direction_alt_changed) {
			indigo_update_property(device, POLARALIGN_DIRECTION_ALT_PROPERTY, NULL);
		}
		if (limits_changed) {
			indigo_update_property(device, POLARALIGN_LIMITS_PROPERTY, NULL);
		}
		if (backlash_enable_changed) {
			indigo_update_property(device, X_MLASTRO_BACKLASH_ENABLE_PROPERTY, NULL);
		}
		if (backlash_steps_changed) {
			indigo_update_property(device, X_MLASTRO_BACKLASH_STEPS_PROPERTY, NULL);
		}
	}
	return position_changed;
}

/* The controller has one shared zero reference for both axes: SetH:1 marks the
   current physical position as (0,0). There is no way to zero one axis alone. */
static bool mlastro_set_home(indigo_device *device) {
	if (!mlastro_command_ok(device, "SetH:1")) {
		return false;
	}
	PRIVATE_DATA->current_alt = PRIVATE_DATA->current_az = 0;
	POLARALIGN_OFFSET_ALT_ITEM->number.value = POLARALIGN_OFFSET_ALT_ITEM->number.target = 0;
	POLARALIGN_OFFSET_AZ_ITEM->number.value = POLARALIGN_OFFSET_AZ_ITEM->number.target = 0;
	PRIVATE_DATA->homed = true;
	X_MLASTRO_HOMED_ITEM->light.value = INDIGO_OK_STATE;
	indigo_update_property(device, POLARALIGN_OFFSET_PROPERTY, NULL);
	indigo_update_property(device, X_MLASTRO_HOMED_PROPERTY, NULL);
	return true;
}

static bool mlastro_open(indigo_device *device) {
	PRIVATE_DATA->handle = indigo_uni_open_serial_with_speed(DEVICE_PORT_ITEM->text.value, MLASTRO_BAUDRATE, INDIGO_LOG_DEBUG);
	if (PRIVATE_DATA->handle == NULL) {
		return false;
	}
	PRIVATE_DATA->moving = PRIVATE_DATA->error = false;
	/* Opening the port resets most ESP32 boards, which print a boot log and
	   ignore the handshake until the firmware is up, so keep retrying. */
	bool handshake = false;
	for (int attempt = 0; attempt < MLASTRO_HANDSHAKE_ATTEMPTS && !handshake; attempt++) {
		indigo_usleep(INDIGO_DELAY(attempt == 0 ? 0.2 : 0.5));
		/* A "Not connected" line left over from before the handshake is not a
		   loss of the control this handshake is about to take. */
		PRIVATE_DATA->control_lost = false;
		handshake = mlastro_exchange(device, "[MLAstroRPA-TC]", false, 1) && !strncmp(PRIVATE_DATA->response, "ok", 2);
	}
	if (!handshake) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "Unexpected handshake response '%s'", PRIVATE_DATA->response);
		indigo_uni_close(&PRIVATE_DATA->handle);
		return false;
	}
	/* Newer firmware replies "ok,firmware X.Y.Z,SN:AA:BB:CC:DD:EE:F0"; older
	   firmware just replies "ok" with no version/serial. */
	char *fw_start = strchr(PRIVATE_DATA->response, ',');
	char *sn_field = strstr(PRIVATE_DATA->response, "SN:");
	if (fw_start && sn_field && sn_field > fw_start) {
		fw_start++;
		size_t fw_len = (size_t)(sn_field - fw_start);
		while (fw_len > 0 && (fw_start[fw_len - 1] == ',' || fw_start[fw_len - 1] == ' ')) {
			fw_len--;
		}
		if (fw_len >= sizeof(INFO_DEVICE_FW_REVISION_ITEM->text.value)) {
			fw_len = sizeof(INFO_DEVICE_FW_REVISION_ITEM->text.value) - 1;
		}
		strncpy(INFO_DEVICE_FW_REVISION_ITEM->text.value, fw_start, fw_len);
		INFO_DEVICE_FW_REVISION_ITEM->text.value[fw_len] = '\0';
		INDIGO_COPY_VALUE(INFO_DEVICE_SERIAL_NUM_ITEM->text.value, sn_field + 3);
	}
	INDIGO_COPY_VALUE(INFO_DEVICE_MODEL_ITEM->text.value, "MLAstro RPA");
	indigo_update_property(device, INFO_PROPERTY, NULL);
	/* Lock the controller into relative (angle) mode for single-axis moves. */
	if (!mlastro_command_ok(device, "JoRe:1")) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "Could not set relative mode, proceeding anyway");
	}
	if (mlastro_exchange(device, "?", true, 2)) {
		mlastro_parse_telemetry(device, PRIVATE_DATA->response, false);
	}
	return true;
}

/* The NINA plugin sends "Disconnect" before closing the port so the firmware
   hands control back to the Web UI; without it a controller whose communication
   watchdog is off keeps the Web UI locked out. No reply is read. */
static void mlastro_close(indigo_device *device) {
	if (PRIVATE_DATA->handle != NULL && !PRIVATE_DATA->control_lost) {
		indigo_uni_printf(PRIVATE_DATA->handle, "Disconnect\n");
	}
	indigo_uni_close(&PRIVATE_DATA->handle);
}

//- code

#pragma mark - High level code (polaralign)

static void polaralign_timer_callback(indigo_device *device) {
	if (!IS_CONNECTED) {
		return;
	}
	//+ polaralign.on_timer
	bool position_changed = false;
	if (mlastro_exchange(device, "?", true, 2)) {
		position_changed = mlastro_parse_telemetry(device, PRIVATE_DATA->response, true);
	}
	if (PRIVATE_DATA->control_lost) {
		indigo_set_switch(CONNECTION_PROPERTY, CONNECTION_DISCONNECTED_ITEM, true);
		polaralign_connection_handler(device);
		CONNECTION_PROPERTY->state = INDIGO_ALERT_STATE;
		indigo_update_property(device, CONNECTION_PROPERTY, NULL);
		indigo_send_message(device, ALERT_PROPERTY, "%s released serial control (Web UI took over, communication watchdog expired or controller rebooted)", device->name);
		return;
	}
	if (POLARALIGN_OFFSET_PROPERTY->state == INDIGO_BUSY_STATE) {
		if (PRIVATE_DATA->error) {
			POLARALIGN_OFFSET_ALT_ITEM->number.target = POLARALIGN_OFFSET_ALT_ITEM->number.value;
			POLARALIGN_OFFSET_AZ_ITEM->number.target = POLARALIGN_OFFSET_AZ_ITEM->number.value;
			POLARALIGN_OFFSET_PROPERTY->state = INDIGO_ALERT_STATE;
			indigo_update_property(device, POLARALIGN_OFFSET_PROPERTY, "Controller stopped with an error (hard limit or driver fault), reset it with 'Reset controller error'");
		} else {
			if (!PRIVATE_DATA->moving) {
				POLARALIGN_OFFSET_PROPERTY->state = INDIGO_OK_STATE;
			}
			indigo_update_property(device, POLARALIGN_OFFSET_PROPERTY, NULL);
		}
	} else if (position_changed) {
		/* Idle position changes still reach the client: a soft stop decelerates
		   after abort, and a finished move leaves the target where it arrived. */
		POLARALIGN_OFFSET_ALT_ITEM->number.target = POLARALIGN_OFFSET_ALT_ITEM->number.value;
		POLARALIGN_OFFSET_AZ_ITEM->number.target = POLARALIGN_OFFSET_AZ_ITEM->number.value;
		indigo_update_property(device, POLARALIGN_OFFSET_PROPERTY, NULL);
	}
	indigo_execute_handler_in(device, MLASTRO_POLL_INTERVAL, polaralign_timer_callback);
	//- polaralign.on_timer
}

static void polaralign_connection_handler(indigo_device *device) {
	if (CONNECTION_CONNECTED_ITEM->sw.value) {
		bool connection_result = true;
		connection_result = mlastro_open(device);
		if (connection_result) {
			indigo_define_property(device, X_MLASTRO_SPEED_PROPERTY, NULL);
			indigo_define_property(device, X_MLASTRO_BACKLASH_ENABLE_PROPERTY, NULL);
			indigo_define_property(device, X_MLASTRO_BACKLASH_STEPS_PROPERTY, NULL);
			indigo_define_property(device, X_MLASTRO_STATUS_PROPERTY, NULL);
			indigo_define_property(device, X_MLASTRO_HOMED_PROPERTY, NULL);
			indigo_define_property(device, X_MLASTRO_GOTO_HOME_PROPERTY, NULL);
			indigo_define_property(device, X_MLASTRO_CLEAR_HOME_PROPERTY, NULL);
			indigo_define_property(device, X_MLASTRO_RESET_ERROR_PROPERTY, NULL);
			CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
			indigo_send_message(device, OK_PROPERTY, "Connected to %s on %s", POLARALIGN_DEVICE_NAME, DEVICE_PORT_ITEM->text.value);
		} else {
			indigo_send_message(device, ALERT_PROPERTY, "Failed to connect to %s on %s", POLARALIGN_DEVICE_NAME, DEVICE_PORT_ITEM->text.value);
			CONNECTION_PROPERTY->state = INDIGO_ALERT_STATE;
			indigo_set_switch(CONNECTION_PROPERTY, CONNECTION_DISCONNECTED_ITEM, true);
		}
	} else {
		indigo_cancel_pending_handlers(device);
		//+ polaralign.on_disconnect
		INDIGO_COPY_VALUE(INFO_DEVICE_MODEL_ITEM->text.value, "Unknown");
		INDIGO_COPY_VALUE(INFO_DEVICE_FW_REVISION_ITEM->text.value, "Unknown");
		INDIGO_COPY_VALUE(INFO_DEVICE_SERIAL_NUM_ITEM->text.value, "Unknown");
		indigo_update_property(device, INFO_PROPERTY, NULL);
		//- polaralign.on_disconnect
		indigo_delete_property(device, X_MLASTRO_SPEED_PROPERTY, NULL);
		indigo_delete_property(device, X_MLASTRO_BACKLASH_ENABLE_PROPERTY, NULL);
		indigo_delete_property(device, X_MLASTRO_BACKLASH_STEPS_PROPERTY, NULL);
		indigo_delete_property(device, X_MLASTRO_STATUS_PROPERTY, NULL);
		indigo_delete_property(device, X_MLASTRO_HOMED_PROPERTY, NULL);
		indigo_delete_property(device, X_MLASTRO_GOTO_HOME_PROPERTY, NULL);
		indigo_delete_property(device, X_MLASTRO_CLEAR_HOME_PROPERTY, NULL);
		indigo_delete_property(device, X_MLASTRO_RESET_ERROR_PROPERTY, NULL);
		mlastro_close(device);
		indigo_send_message(device, OK_PROPERTY, "Disconnected from %s", device->name);
		CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
	}
	indigo_polaralign_change_property(device, NULL, CONNECTION_PROPERTY);
	if (IS_CONNECTED) {
		indigo_execute_handler(device, polaralign_timer_callback);
	}
}

static void polaralign_offset_handler(indigo_device *device) {
	POLARALIGN_OFFSET_PROPERTY->state = INDIGO_OK_STATE;
	//+ polaralign.POLARALIGN_OFFSET.on_change
	/* Only the targets are copied, .value keeps the last reported position.
	   Differences below half an arcsecond round to a zero-length move and
	   are left alone, so they neither issue an empty move nor turn a
	   single-axis move into a two-axis one. */
	double delta_alt = POLARALIGN_OFFSET_ALT_ITEM->number.target / 60.0 - PRIVATE_DATA->current_alt;
	double delta_az = POLARALIGN_OFFSET_AZ_ITEM->number.target / 60.0 - PRIVATE_DATA->current_az;
	bool move_alt = fabs(delta_alt) >= MLASTRO_MIN_MOVE;
	bool move_az = fabs(delta_az) >= MLASTRO_MIN_MOVE;
	bool ok = true;
	if (move_alt && move_az) {
		int alt_d, alt_m, alt_s, az_d, az_m, az_s;
		bool alt_positive, az_positive;
		mlastro_degrees_to_dms(delta_alt, &alt_d, &alt_m, &alt_s, &alt_positive);
		mlastro_degrees_to_dms(delta_az, &az_d, &az_m, &az_s, &az_positive);
		ok = mlastro_command_ok(device, "AzED:%d,AzEM:%d,AzES:%d,AzDi:%d,AlED:%d,AlEM:%d,AlES:%d,AlDi:%d,AAll:1", az_d, az_m, az_s, az_positive ? 1 : 0, alt_d, alt_m, alt_s, alt_positive ? 1 : 0);
	} else if (move_az) {
		int d, m, s;
		bool positive;
		mlastro_degrees_to_dms(delta_az, &d, &m, &s, &positive);
		ok = mlastro_command_ok(device, "ReDe:%d,ReAM:%d,ReAS:%d,%s", d, m, s, positive ? "MAzR:1" : "MAzL:1");
	} else if (move_alt) {
		int d, m, s;
		bool positive;
		mlastro_degrees_to_dms(delta_alt, &d, &m, &s, &positive);
		ok = mlastro_command_ok(device, "ReDe:%d,ReAM:%d,ReAS:%d,%s", d, m, s, positive ? "MAlU:1" : "MAlD:1");
	}
	if (!ok) {
		POLARALIGN_OFFSET_ALT_ITEM->number.target = POLARALIGN_OFFSET_ALT_ITEM->number.value;
		POLARALIGN_OFFSET_AZ_ITEM->number.target = POLARALIGN_OFFSET_AZ_ITEM->number.value;
		POLARALIGN_OFFSET_PROPERTY->state = INDIGO_ALERT_STATE;
	} else if (move_alt || move_az) {
		PRIVATE_DATA->moving = true;
		POLARALIGN_OFFSET_PROPERTY->state = INDIGO_BUSY_STATE;
	}
	//- polaralign.POLARALIGN_OFFSET.on_change
	indigo_update_property(device, POLARALIGN_OFFSET_PROPERTY, NULL);
}

static void polaralign_abort_motion_handler(indigo_device *device) {
	POLARALIGN_ABORT_MOTION_PROPERTY->state = INDIGO_OK_STATE;
	//+ polaralign.POLARALIGN_ABORT_MOTION.on_change
	if (POLARALIGN_ABORT_MOTION_ITEM->sw.value) {
		/* STOP:1 is a soft stop: the axes decelerate after the reply, and the
		   timer keeps publishing the position until they come to rest. */
		if (mlastro_command_ok(device, "STOP:1")) {
			if (POLARALIGN_OFFSET_PROPERTY->state == INDIGO_BUSY_STATE) {
				POLARALIGN_OFFSET_ALT_ITEM->number.target = POLARALIGN_OFFSET_ALT_ITEM->number.value;
				POLARALIGN_OFFSET_AZ_ITEM->number.target = POLARALIGN_OFFSET_AZ_ITEM->number.value;
				POLARALIGN_OFFSET_PROPERTY->state = INDIGO_ALERT_STATE;
				indigo_update_property(device, POLARALIGN_OFFSET_PROPERTY, NULL);
			}
		} else {
			POLARALIGN_ABORT_MOTION_PROPERTY->state = INDIGO_ALERT_STATE;
		}
		POLARALIGN_ABORT_MOTION_ITEM->sw.value = false;
	}
	//- polaralign.POLARALIGN_ABORT_MOTION.on_change
	indigo_update_property(device, POLARALIGN_ABORT_MOTION_PROPERTY, NULL);
}

static void polaralign_steps_per_degree_handler(indigo_device *device) {
	POLARALIGN_STEPS_PER_DEGREE_PROPERTY->state = INDIGO_OK_STATE;
	//+ polaralign.POLARALIGN_STEPS_PER_DEGREE.on_change
	if (!mlastro_command_ok(device, "AzSD:%.0f,AlSD:%.0f", POLARALIGN_STEPS_PER_DEGREE_AZ_ITEM->number.target, POLARALIGN_STEPS_PER_DEGREE_ALT_ITEM->number.target)) {
		POLARALIGN_STEPS_PER_DEGREE_PROPERTY->state = INDIGO_ALERT_STATE;
	}
	//- polaralign.POLARALIGN_STEPS_PER_DEGREE.on_change
	indigo_update_property(device, POLARALIGN_STEPS_PER_DEGREE_PROPERTY, NULL);
}

static void polaralign_direction_alt_handler(indigo_device *device) {
	POLARALIGN_DIRECTION_ALT_PROPERTY->state = INDIGO_OK_STATE;
	//+ polaralign.POLARALIGN_DIRECTION_ALT.on_change
	if (!mlastro_command_ok(device, "AlRD:%d", POLARALIGN_DIRECTION_ALT_REVERSED_ITEM->sw.value ? 1 : 0)) {
		POLARALIGN_DIRECTION_ALT_PROPERTY->state = INDIGO_ALERT_STATE;
	}
	//- polaralign.POLARALIGN_DIRECTION_ALT.on_change
	indigo_update_property(device, POLARALIGN_DIRECTION_ALT_PROPERTY, NULL);
}

static void polaralign_direction_az_handler(indigo_device *device) {
	POLARALIGN_DIRECTION_AZ_PROPERTY->state = INDIGO_OK_STATE;
	//+ polaralign.POLARALIGN_DIRECTION_AZ.on_change
	if (!mlastro_command_ok(device, "AzRD:%d", POLARALIGN_DIRECTION_AZ_REVERSED_ITEM->sw.value ? 1 : 0)) {
		POLARALIGN_DIRECTION_AZ_PROPERTY->state = INDIGO_ALERT_STATE;
	}
	//- polaralign.POLARALIGN_DIRECTION_AZ.on_change
	indigo_update_property(device, POLARALIGN_DIRECTION_AZ_PROPERTY, NULL);
}

static void polaralign_reset_position_alt_handler(indigo_device *device) {
	POLARALIGN_RESET_POSITION_ALT_PROPERTY->state = INDIGO_OK_STATE;
	//+ polaralign.POLARALIGN_RESET_POSITION_ALT.on_change
	/* SetH:1 zeroes both axes at once; see mlastro_set_home(). */
	if (POLARALIGN_RESET_POSITION_ALT_ITEM->sw.value) {
		if (!mlastro_set_home(device)) {
			POLARALIGN_RESET_POSITION_ALT_PROPERTY->state = INDIGO_ALERT_STATE;
		}
		POLARALIGN_RESET_POSITION_ALT_ITEM->sw.value = false;
	}
	//- polaralign.POLARALIGN_RESET_POSITION_ALT.on_change
	indigo_update_property(device, POLARALIGN_RESET_POSITION_ALT_PROPERTY, NULL);
}

static void polaralign_reset_position_az_handler(indigo_device *device) {
	POLARALIGN_RESET_POSITION_AZ_PROPERTY->state = INDIGO_OK_STATE;
	//+ polaralign.POLARALIGN_RESET_POSITION_AZ.on_change
	/* SetH:1 zeroes both axes at once; see mlastro_set_home(). */
	if (POLARALIGN_RESET_POSITION_AZ_ITEM->sw.value) {
		if (!mlastro_set_home(device)) {
			POLARALIGN_RESET_POSITION_AZ_PROPERTY->state = INDIGO_ALERT_STATE;
		}
		POLARALIGN_RESET_POSITION_AZ_ITEM->sw.value = false;
	}
	//- polaralign.POLARALIGN_RESET_POSITION_AZ.on_change
	indigo_update_property(device, POLARALIGN_RESET_POSITION_AZ_PROPERTY, NULL);
}

static void polaralign_limits_handler(indigo_device *device) {
	POLARALIGN_LIMITS_PROPERTY->state = INDIGO_OK_STATE;
	//+ polaralign.POLARALIGN_LIMITS.on_change
	bool ok = mlastro_command_ok(device, "AzL1:%.2f,AzL2:%.2f", POLARALIGN_LIMITS_MIN_POSITION_AZ_ITEM->number.target / 60.0, POLARALIGN_LIMITS_MAX_POSITION_AZ_ITEM->number.target / 60.0);
	ok &= mlastro_command_ok(device, "AlL1:%.2f,AlL2:%.2f", POLARALIGN_LIMITS_MIN_POSITION_ALT_ITEM->number.target / 60.0, POLARALIGN_LIMITS_MAX_POSITION_ALT_ITEM->number.target / 60.0);
	if (!ok) {
		POLARALIGN_LIMITS_PROPERTY->state = INDIGO_ALERT_STATE;
	}
	//- polaralign.POLARALIGN_LIMITS.on_change
	indigo_update_property(device, POLARALIGN_LIMITS_PROPERTY, NULL);
}

static void polaralign_x_mlastro_speed_handler(indigo_device *device) {
	X_MLASTRO_SPEED_PROPERTY->state = INDIGO_OK_STATE;
	//+ polaralign.X_MLASTRO_SPEED.on_change
	if (!mlastro_command_ok(device, "SLvl:%d", (int)X_MLASTRO_SPEED_ITEM->number.target)) {
		X_MLASTRO_SPEED_PROPERTY->state = INDIGO_ALERT_STATE;
	}
	//- polaralign.X_MLASTRO_SPEED.on_change
	indigo_update_property(device, X_MLASTRO_SPEED_PROPERTY, NULL);
}

static void polaralign_x_mlastro_backlash_enable_handler(indigo_device *device) {
	X_MLASTRO_BACKLASH_ENABLE_PROPERTY->state = INDIGO_OK_STATE;
	//+ polaralign.X_MLASTRO_BACKLASH_ENABLE.on_change
	if (!mlastro_command_ok(device, "Back:%d", X_MLASTRO_BACKLASH_ENABLE_ENABLED_ITEM->sw.value ? 1 : 0)) {
		X_MLASTRO_BACKLASH_ENABLE_PROPERTY->state = INDIGO_ALERT_STATE;
	}
	//- polaralign.X_MLASTRO_BACKLASH_ENABLE.on_change
	indigo_update_property(device, X_MLASTRO_BACKLASH_ENABLE_PROPERTY, NULL);
}

static void polaralign_x_mlastro_backlash_steps_handler(indigo_device *device) {
	X_MLASTRO_BACKLASH_STEPS_PROPERTY->state = INDIGO_OK_STATE;
	//+ polaralign.X_MLASTRO_BACKLASH_STEPS.on_change
	bool ok = mlastro_command_ok(device, "AzBl:%d", (int)X_MLASTRO_BACKLASH_STEPS_AZ_ITEM->number.target);
	ok &= mlastro_command_ok(device, "AlBl:%d", (int)X_MLASTRO_BACKLASH_STEPS_ALT_ITEM->number.target);
	if (!ok) {
		X_MLASTRO_BACKLASH_STEPS_PROPERTY->state = INDIGO_ALERT_STATE;
	}
	//- polaralign.X_MLASTRO_BACKLASH_STEPS.on_change
	indigo_update_property(device, X_MLASTRO_BACKLASH_STEPS_PROPERTY, NULL);
}

static void polaralign_x_mlastro_goto_home_handler(indigo_device *device) {
	X_MLASTRO_GOTO_HOME_PROPERTY->state = INDIGO_OK_STATE;
	//+ polaralign.X_MLASTRO_GOTO_HOME.on_change
	if (X_MLASTRO_GOTO_HOME_ITEM->sw.value) {
		if (mlastro_command_ok(device, "RetH:1")) {
			PRIVATE_DATA->moving = true;
			POLARALIGN_OFFSET_PROPERTY->state = INDIGO_BUSY_STATE;
			indigo_update_property(device, POLARALIGN_OFFSET_PROPERTY, NULL);
		} else {
			X_MLASTRO_GOTO_HOME_PROPERTY->state = INDIGO_ALERT_STATE;
		}
		X_MLASTRO_GOTO_HOME_ITEM->sw.value = false;
	}
	//- polaralign.X_MLASTRO_GOTO_HOME.on_change
	indigo_update_property(device, X_MLASTRO_GOTO_HOME_PROPERTY, NULL);
}

static void polaralign_x_mlastro_clear_home_handler(indigo_device *device) {
	X_MLASTRO_CLEAR_HOME_PROPERTY->state = INDIGO_OK_STATE;
	//+ polaralign.X_MLASTRO_CLEAR_HOME.on_change
	if (X_MLASTRO_CLEAR_HOME_ITEM->sw.value) {
		if (mlastro_command_ok(device, "RstH:1")) {
			PRIVATE_DATA->homed = false;
			X_MLASTRO_HOMED_ITEM->light.value = INDIGO_IDLE_STATE;
			indigo_update_property(device, X_MLASTRO_HOMED_PROPERTY, NULL);
		} else {
			X_MLASTRO_CLEAR_HOME_PROPERTY->state = INDIGO_ALERT_STATE;
		}
		X_MLASTRO_CLEAR_HOME_ITEM->sw.value = false;
	}
	//- polaralign.X_MLASTRO_CLEAR_HOME.on_change
	indigo_update_property(device, X_MLASTRO_CLEAR_HOME_PROPERTY, NULL);
}

static void polaralign_x_mlastro_reset_error_handler(indigo_device *device) {
	X_MLASTRO_RESET_ERROR_PROPERTY->state = INDIGO_OK_STATE;
	//+ polaralign.X_MLASTRO_RESET_ERROR.on_change
	/* ReER:1 clears a driver error or hard-limit lock, returns the controller
	   to READY and stops both motors, which ends any motion in progress. */
	if (X_MLASTRO_RESET_ERROR_ITEM->sw.value) {
		if (mlastro_command_ok(device, "ReER:1")) {
			if (POLARALIGN_OFFSET_PROPERTY->state == INDIGO_BUSY_STATE) {
				POLARALIGN_OFFSET_ALT_ITEM->number.target = POLARALIGN_OFFSET_ALT_ITEM->number.value;
				POLARALIGN_OFFSET_AZ_ITEM->number.target = POLARALIGN_OFFSET_AZ_ITEM->number.value;
				POLARALIGN_OFFSET_PROPERTY->state = INDIGO_ALERT_STATE;
				indigo_update_property(device, POLARALIGN_OFFSET_PROPERTY, NULL);
			}
		} else {
			X_MLASTRO_RESET_ERROR_PROPERTY->state = INDIGO_ALERT_STATE;
		}
		X_MLASTRO_RESET_ERROR_ITEM->sw.value = false;
	}
	//- polaralign.X_MLASTRO_RESET_ERROR.on_change
	indigo_update_property(device, X_MLASTRO_RESET_ERROR_PROPERTY, NULL);
}

#pragma mark - Device API (polaralign)

static indigo_result polaralign_enumerate_properties(indigo_device *device, indigo_client *client, indigo_property *property);

static indigo_result polaralign_attach(indigo_device *device) {
	if (indigo_polaralign_attach(device, DRIVER_NAME, DRIVER_VERSION) == INDIGO_OK) {
		DEVICE_PORT_PROPERTY->hidden = false;
		DEVICE_PORTS_PROPERTY->hidden = false;
		indigo_enumerate_serial_ports(device, DEVICE_PORTS_PROPERTY);
		//+ polaralign.on_attach
		INFO_PROPERTY->count = 8;
		INDIGO_COPY_VALUE(INFO_DEVICE_MODEL_ITEM->text.value, "Unknown");
		INDIGO_COPY_VALUE(INFO_DEVICE_FW_REVISION_ITEM->text.value, "Unknown");
		INDIGO_COPY_VALUE(INFO_DEVICE_SERIAL_NUM_ITEM->text.value, "Unknown");
		//- polaralign.on_attach
		POLARALIGN_OFFSET_PROPERTY->hidden = false;
		//+ polaralign.POLARALIGN_OFFSET.on_attach
		POLARALIGN_OFFSET_ALT_ITEM->number.min = POLARALIGN_OFFSET_AZ_ITEM->number.min = -5400;
		POLARALIGN_OFFSET_ALT_ITEM->number.max = POLARALIGN_OFFSET_AZ_ITEM->number.max = 5400;
		//- polaralign.POLARALIGN_OFFSET.on_attach
		POLARALIGN_ABORT_MOTION_PROPERTY->hidden = false;
		POLARALIGN_STEPS_PER_DEGREE_PROPERTY->hidden = false;
		//+ polaralign.POLARALIGN_STEPS_PER_DEGREE.on_attach
		POLARALIGN_STEPS_PER_DEGREE_ALT_ITEM->number.max = POLARALIGN_STEPS_PER_DEGREE_AZ_ITEM->number.max = 100000;
		//- polaralign.POLARALIGN_STEPS_PER_DEGREE.on_attach
		POLARALIGN_DIRECTION_ALT_PROPERTY->hidden = false;
		POLARALIGN_DIRECTION_AZ_PROPERTY->hidden = false;
		POLARALIGN_RESET_POSITION_ALT_PROPERTY->hidden = false;
		POLARALIGN_RESET_POSITION_AZ_PROPERTY->hidden = false;
		POLARALIGN_LIMITS_PROPERTY->hidden = false;
		//+ polaralign.POLARALIGN_LIMITS.on_attach
		POLARALIGN_LIMITS_MIN_POSITION_ALT_ITEM->number.min = POLARALIGN_LIMITS_MIN_POSITION_AZ_ITEM->number.min = -5400;
		POLARALIGN_LIMITS_MAX_POSITION_ALT_ITEM->number.max = POLARALIGN_LIMITS_MAX_POSITION_AZ_ITEM->number.max = 5400;
		//- polaralign.POLARALIGN_LIMITS.on_attach
		X_MLASTRO_SPEED_PROPERTY = indigo_init_number_property(NULL, device->name, X_MLASTRO_SPEED_PROPERTY_NAME, POLARALIGN_MAIN_GROUP, "Speed level", INDIGO_OK_STATE, INDIGO_RW_PERM, 1);
		if (X_MLASTRO_SPEED_PROPERTY == NULL) {
			return INDIGO_FAILED;
		}
		indigo_init_number_item(X_MLASTRO_SPEED_ITEM, X_MLASTRO_SPEED_ITEM_NAME, "Speed level (1-5)", 1, 5, 1, 3);
		X_MLASTRO_BACKLASH_ENABLE_PROPERTY = indigo_init_switch_property(NULL, device->name, X_MLASTRO_BACKLASH_ENABLE_PROPERTY_NAME, POLARALIGN_ADVANCED_GROUP, "Backlash compensation", INDIGO_OK_STATE, INDIGO_RW_PERM, INDIGO_ONE_OF_MANY_RULE, 2);
		if (X_MLASTRO_BACKLASH_ENABLE_PROPERTY == NULL) {
			return INDIGO_FAILED;
		}
		indigo_init_switch_item(X_MLASTRO_BACKLASH_ENABLE_ENABLED_ITEM, X_MLASTRO_BACKLASH_ENABLE_ENABLED_ITEM_NAME, "Enabled", false);
		indigo_init_switch_item(X_MLASTRO_BACKLASH_ENABLE_DISABLED_ITEM, X_MLASTRO_BACKLASH_ENABLE_DISABLED_ITEM_NAME, "Disabled", true);
		X_MLASTRO_BACKLASH_STEPS_PROPERTY = indigo_init_number_property(NULL, device->name, X_MLASTRO_BACKLASH_STEPS_PROPERTY_NAME, POLARALIGN_ADVANCED_GROUP, "Backlash steps", INDIGO_OK_STATE, INDIGO_RW_PERM, 2);
		if (X_MLASTRO_BACKLASH_STEPS_PROPERTY == NULL) {
			return INDIGO_FAILED;
		}
		indigo_init_number_item(X_MLASTRO_BACKLASH_STEPS_ALT_ITEM, X_MLASTRO_BACKLASH_STEPS_ALT_ITEM_NAME, "Altitude backlash (steps)", 0, 9999, 1, 0);
		indigo_init_number_item(X_MLASTRO_BACKLASH_STEPS_AZ_ITEM, X_MLASTRO_BACKLASH_STEPS_AZ_ITEM_NAME, "Azimuth backlash (steps)", 0, 9999, 1, 0);
		X_MLASTRO_STATUS_PROPERTY = indigo_init_text_property(NULL, device->name, X_MLASTRO_STATUS_PROPERTY_NAME, POLARALIGN_MAIN_GROUP, "Controller status", INDIGO_OK_STATE, INDIGO_RO_PERM, 1);
		if (X_MLASTRO_STATUS_PROPERTY == NULL) {
			return INDIGO_FAILED;
		}
		indigo_init_text_item(X_MLASTRO_STATUS_ITEM, X_MLASTRO_STATUS_ITEM_NAME, "Status", "Unknown");
		X_MLASTRO_HOMED_PROPERTY = indigo_init_light_property(NULL, device->name, X_MLASTRO_HOMED_PROPERTY_NAME, POLARALIGN_MAIN_GROUP, "Homed", INDIGO_OK_STATE, 1);
		if (X_MLASTRO_HOMED_PROPERTY == NULL) {
			return INDIGO_FAILED;
		}
		indigo_init_light_item(X_MLASTRO_HOMED_ITEM, X_MLASTRO_HOMED_ITEM_NAME, "Home reference set", INDIGO_IDLE_STATE);
		X_MLASTRO_GOTO_HOME_PROPERTY = indigo_init_switch_property(NULL, device->name, X_MLASTRO_GOTO_HOME_PROPERTY_NAME, POLARALIGN_MAIN_GROUP, "Return to home", INDIGO_OK_STATE, INDIGO_RW_PERM, INDIGO_AT_MOST_ONE_RULE, 1);
		if (X_MLASTRO_GOTO_HOME_PROPERTY == NULL) {
			return INDIGO_FAILED;
		}
		indigo_init_switch_item(X_MLASTRO_GOTO_HOME_ITEM, X_MLASTRO_GOTO_HOME_ITEM_NAME, "Return both axes to home (0,0)", false);
		X_MLASTRO_CLEAR_HOME_PROPERTY = indigo_init_switch_property(NULL, device->name, X_MLASTRO_CLEAR_HOME_PROPERTY_NAME, POLARALIGN_ADVANCED_GROUP, "Clear home reference", INDIGO_OK_STATE, INDIGO_RW_PERM, INDIGO_AT_MOST_ONE_RULE, 1);
		if (X_MLASTRO_CLEAR_HOME_PROPERTY == NULL) {
			return INDIGO_FAILED;
		}
		indigo_init_switch_item(X_MLASTRO_CLEAR_HOME_ITEM, X_MLASTRO_CLEAR_HOME_ITEM_NAME, "Forget home reference", false);
		X_MLASTRO_RESET_ERROR_PROPERTY = indigo_init_switch_property(NULL, device->name, X_MLASTRO_RESET_ERROR_PROPERTY_NAME, POLARALIGN_MAIN_GROUP, "Reset controller error", INDIGO_OK_STATE, INDIGO_RW_PERM, INDIGO_AT_MOST_ONE_RULE, 1);
		if (X_MLASTRO_RESET_ERROR_PROPERTY == NULL) {
			return INDIGO_FAILED;
		}
		indigo_init_switch_item(X_MLASTRO_RESET_ERROR_ITEM, X_MLASTRO_RESET_ERROR_ITEM_NAME, "Clear error and stop motors", false);
		INDIGO_DEVICE_ATTACH_LOG(DRIVER_NAME, device->name);
		return polaralign_enumerate_properties(device, NULL, NULL);
	}
	return INDIGO_FAILED;
}

static indigo_result polaralign_enumerate_properties(indigo_device *device, indigo_client *client, indigo_property *property) {
	if (IS_CONNECTED) {
		INDIGO_DEFINE_MATCHING_PROPERTY(X_MLASTRO_SPEED_PROPERTY);
		INDIGO_DEFINE_MATCHING_PROPERTY(X_MLASTRO_BACKLASH_ENABLE_PROPERTY);
		INDIGO_DEFINE_MATCHING_PROPERTY(X_MLASTRO_BACKLASH_STEPS_PROPERTY);
		INDIGO_DEFINE_MATCHING_PROPERTY(X_MLASTRO_STATUS_PROPERTY);
		INDIGO_DEFINE_MATCHING_PROPERTY(X_MLASTRO_HOMED_PROPERTY);
		INDIGO_DEFINE_MATCHING_PROPERTY(X_MLASTRO_GOTO_HOME_PROPERTY);
		INDIGO_DEFINE_MATCHING_PROPERTY(X_MLASTRO_CLEAR_HOME_PROPERTY);
		INDIGO_DEFINE_MATCHING_PROPERTY(X_MLASTRO_RESET_ERROR_PROPERTY);
	}
	return indigo_polaralign_enumerate_properties(device, client, property);
}

static indigo_result polaralign_change_property(indigo_device *device, indigo_client *client, indigo_property *property) {
	if (indigo_property_match_changeable(CONNECTION_PROPERTY, property)) {
		INDIGO_PROCESS_CONNECT(polaralign_connection_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(POLARALIGN_OFFSET_PROPERTY, property)) {
		INDIGO_REJECT_CHANGE_IF(POLARALIGN_OFFSET_PROPERTY->state == INDIGO_BUSY_STATE, POLARALIGN_OFFSET_PROPERTY, "Another motion operation is pending");
		INDIGO_COPY_TARGETS_PROCESS_CHANGE(POLARALIGN_OFFSET_PROPERTY, polaralign_offset_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(POLARALIGN_ABORT_MOTION_PROPERTY, property)) {
		INDIGO_COPY_VALUES_PROCESS_URGENT_CHANGE(POLARALIGN_ABORT_MOTION_PROPERTY, polaralign_abort_motion_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(POLARALIGN_STEPS_PER_DEGREE_PROPERTY, property)) {
		INDIGO_COPY_VALUES_PROCESS_CHANGE(POLARALIGN_STEPS_PER_DEGREE_PROPERTY, polaralign_steps_per_degree_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(POLARALIGN_DIRECTION_ALT_PROPERTY, property)) {
		INDIGO_COPY_VALUES_PROCESS_CHANGE(POLARALIGN_DIRECTION_ALT_PROPERTY, polaralign_direction_alt_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(POLARALIGN_DIRECTION_AZ_PROPERTY, property)) {
		INDIGO_COPY_VALUES_PROCESS_CHANGE(POLARALIGN_DIRECTION_AZ_PROPERTY, polaralign_direction_az_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(POLARALIGN_RESET_POSITION_ALT_PROPERTY, property)) {
		INDIGO_COPY_VALUES_PROCESS_CHANGE(POLARALIGN_RESET_POSITION_ALT_PROPERTY, polaralign_reset_position_alt_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(POLARALIGN_RESET_POSITION_AZ_PROPERTY, property)) {
		INDIGO_COPY_VALUES_PROCESS_CHANGE(POLARALIGN_RESET_POSITION_AZ_PROPERTY, polaralign_reset_position_az_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(POLARALIGN_LIMITS_PROPERTY, property)) {
		INDIGO_COPY_VALUES_PROCESS_CHANGE(POLARALIGN_LIMITS_PROPERTY, polaralign_limits_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(X_MLASTRO_SPEED_PROPERTY, property)) {
		INDIGO_COPY_VALUES_PROCESS_CHANGE(X_MLASTRO_SPEED_PROPERTY, polaralign_x_mlastro_speed_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(X_MLASTRO_BACKLASH_ENABLE_PROPERTY, property)) {
		INDIGO_COPY_VALUES_PROCESS_CHANGE(X_MLASTRO_BACKLASH_ENABLE_PROPERTY, polaralign_x_mlastro_backlash_enable_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(X_MLASTRO_BACKLASH_STEPS_PROPERTY, property)) {
		INDIGO_COPY_VALUES_PROCESS_CHANGE(X_MLASTRO_BACKLASH_STEPS_PROPERTY, polaralign_x_mlastro_backlash_steps_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(X_MLASTRO_GOTO_HOME_PROPERTY, property)) {
		INDIGO_REJECT_CHANGE_IF(POLARALIGN_OFFSET_PROPERTY->state == INDIGO_BUSY_STATE, X_MLASTRO_GOTO_HOME_PROPERTY, "Another motion operation is pending");
		INDIGO_COPY_VALUES_PROCESS_CHANGE(X_MLASTRO_GOTO_HOME_PROPERTY, polaralign_x_mlastro_goto_home_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(X_MLASTRO_CLEAR_HOME_PROPERTY, property)) {
		INDIGO_COPY_VALUES_PROCESS_CHANGE(X_MLASTRO_CLEAR_HOME_PROPERTY, polaralign_x_mlastro_clear_home_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(X_MLASTRO_RESET_ERROR_PROPERTY, property)) {
		INDIGO_COPY_VALUES_PROCESS_CHANGE(X_MLASTRO_RESET_ERROR_PROPERTY, polaralign_x_mlastro_reset_error_handler);
		return INDIGO_OK;
	} else if (indigo_property_match(CONFIG_PROPERTY, property)) {
		if (indigo_switch_match(CONFIG_SAVE_ITEM, property)) {
			indigo_save_property(device, NULL, POLARALIGN_LIMITS_PROPERTY);
		}
	}
	return indigo_polaralign_change_property(device, client, property);
}

static indigo_result polaralign_detach(indigo_device *device) {
	if (IS_CONNECTED) {
		indigo_set_switch(CONNECTION_PROPERTY, CONNECTION_DISCONNECTED_ITEM, true);
		polaralign_connection_handler(device);
	}
	indigo_release_property(X_MLASTRO_SPEED_PROPERTY);
	indigo_release_property(X_MLASTRO_BACKLASH_ENABLE_PROPERTY);
	indigo_release_property(X_MLASTRO_BACKLASH_STEPS_PROPERTY);
	indigo_release_property(X_MLASTRO_STATUS_PROPERTY);
	indigo_release_property(X_MLASTRO_HOMED_PROPERTY);
	indigo_release_property(X_MLASTRO_GOTO_HOME_PROPERTY);
	indigo_release_property(X_MLASTRO_CLEAR_HOME_PROPERTY);
	indigo_release_property(X_MLASTRO_RESET_ERROR_PROPERTY);
	INDIGO_DEVICE_DETACH_LOG(DRIVER_NAME, device->name);
	return indigo_polaralign_detach(device);
}

#pragma mark - Device templates

static indigo_device polaralign_template = INDIGO_DEVICE_INITIALIZER(POLARALIGN_DEVICE_NAME, polaralign_attach, polaralign_enumerate_properties, polaralign_change_property, NULL, polaralign_detach);

#pragma mark - Main code

indigo_result indigo_polaralign_mlastro(indigo_driver_action action, indigo_driver_info *info) {
	static indigo_driver_action last_action = INDIGO_DRIVER_SHUTDOWN;
	static mlastro_private_data *private_data = NULL;
	static indigo_device *polaralign = NULL;

	SET_DRIVER_INFO(info, DRIVER_LABEL, __FUNCTION__, DRIVER_VERSION, false, last_action);

	if (action == last_action) {
		return INDIGO_OK;
	}

	switch (action) {
		case INDIGO_DRIVER_INIT: {
			last_action = action;
			private_data = (mlastro_private_data *)indigo_safe_malloc(sizeof(mlastro_private_data));
			polaralign = (indigo_device *)indigo_safe_malloc_copy(sizeof(indigo_device), &polaralign_template);
			polaralign->private_data = private_data;
			indigo_attach_device(polaralign);
			break;

		}
		case INDIGO_DRIVER_SHUTDOWN: {
			VERIFY_NOT_CONNECTED(polaralign);
			last_action = action;
			if (polaralign != NULL) {
				indigo_detach_device(polaralign);
				indigo_safe_free(polaralign);
				polaralign = NULL;
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
