// Copyright (c) 2024 CloudMakers, s. r. o.
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

#define DRIVER_VERSION 0x0001
#define DRIVER_NAME "indigo_focuser_optecfl"

#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <math.h>
#include <assert.h>
#include <errno.h>
#include <pthread.h>
#include <stdarg.h>

#include <sys/time.h>

#include <indigo/indigo_driver_xml.h>
#include <indigo/indigo_io.h>

#include "indigo_focuser_optecfl.h"

#define PRIVATE_DATA													((optecfl_private_data *)device->private_data)

typedef struct {
	int handle;
	indigo_timer *timer_1;
	indigo_timer *timer_2;
	pthread_mutex_t mutex;
	int count;
} optecfl_private_data;

struct {
	char *name;
	char *type;
} FOCUSER_TYPE[] = {
	{ "Optec TCF-Lynx 2\"", "OA" },
	{ "Optec TCF-Lynx 3\"", "OB" },
	{ "Optec TCF-S 2\" with Extended Travel", "OC" },
	{ "Optec Fast Focus for Celestron Telescopes", "OD" },
	{ "Optec TCF-S Classic converted", "OE" },
	{ "Optec TCF-S3 Classic converted", "OF" },
	{ "Optec Gemini (reserved for future use)", "OG" },
	{ "Optec QuickSync FT motor Hi-Torque", "FA" },
	{ "Optec QuickSync FT motor Hi-Speed", "FB" },
	{ "Optec QuickSync SV motor for StellarVue focusers", "FC" },
	{ "Optec DirectSync TEC motor for TEC focusers", "FD" },
	{ "Optec driver for Robo-Focus uni-polar motors", "RA" },
	{ "Starlight Focuser FTF2008BCR", "SA" },
	{ "Starlight Focuser FTF2015BCR", "SB" },
	{ "Starlight Focuser FTF2020BCR", "SC" },
	{ "Starlight Focuser FTF2025", "SD" },
	{ "Starlight Focuser FTF2515B-A", "SE" },
	{ "Starlight Focuser FTF2525B-A", "SF" },
	{ "Starlight Focuser FTF2535B-A", "SG" },
	{ "Starlight Focuser FTF3015B-A", "SH" },
	{ "Starlight Focuser FTF3025B-A", "SI" },
	{ "Starlight Focuser FTF3035B-A", "SJ" },
	{ "Starlight Focuser FTF3515B-A", "SK" },
	{ "Starlight Focuser FTF3545B-A", "SL" },
	{ "Starlight Focuser AP27FOC3E", "SM" },
	{ "Starlight Focuser AP4FOC3E", "SN" },
	{ "Starlight FeatherTouch Motor Hi-Speed", "SO" },
	{ "Starlight FeatherTouch Motor Hi-Torque", "SQ" },
	{ "Televue focusers with FeatherTouch pinion", "TA" },
	{ NULL, NULL }
};

// -------------------------------------------------------------------------------- Low level communication routines

static void focuser_timer_callback(indigo_device *device) {
	if (!IS_CONNECTED)
		return;
	pthread_mutex_lock(&PRIVATE_DATA->mutex);
	int target = device->gp_bits & 0x3;
	if (FOCUSER_MODE_PROPERTY->state == INDIGO_OK_STATE && FOCUSER_MODE_MANUAL_ITEM->sw.value) {
		char line[80];
		if (indigo_printf(PRIVATE_DATA->handle, "<F%dGETSTATUS>", target) && indigo_read_line(PRIVATE_DATA->handle, line, sizeof(line)) == 1 && *line == '!') {
			INDIGO_DRIVER_DEBUG(DRIVER_NAME, "<F%dGETSTATUS>> -> !", target);
			bool update_position = false;
			bool update_temperature = false;
			while (true) {
				if (indigo_read_line(PRIVATE_DATA->handle, line, sizeof(line)) >= 0) {
					INDIGO_DRIVER_DEBUG(DRIVER_NAME, "  -> %s", line);
					if (!strncmp(line, "END", 3)) {
						PRIVATE_DATA->count++;
						break;
					}
					char key[16], value[80];
					if (sscanf(line, "%15[^=]=%15[^\n]s", key, value) == 2) {
						if (!strncmp(key, "Temp(C)", 7)) {
							double d = atof(value);
							if (FOCUSER_TEMPERATURE_ITEM->number.value != d || FOCUSER_TEMPERATURE_PROPERTY->state != INDIGO_OK_STATE) {
								FOCUSER_TEMPERATURE_ITEM->number.value = d;
								update_temperature = true;
							}
						} else if (!strncmp(key, "TmpProbe", 8)) {
							int i = atoi(value);
							if (FOCUSER_TEMPERATURE_PROPERTY->state != INDIGO_IDLE_STATE && i == 0)  {
								FOCUSER_TEMPERATURE_PROPERTY->state = INDIGO_IDLE_STATE;
								update_temperature = true;
							} else if (FOCUSER_TEMPERATURE_PROPERTY->state == INDIGO_IDLE_STATE && i == 1)  {
								FOCUSER_TEMPERATURE_PROPERTY->state = INDIGO_OK_STATE;
								update_temperature = true;
							}
						} else if (!strncmp(key, "Curr Pos", 8)) {
							int i = atoi(value);
							if (FOCUSER_POSITION_ITEM->number.value != i) {
								FOCUSER_TEMPERATURE_ITEM->number.value = i;
								update_position = true;
							}
						} else if (!strncmp(key, "Targ Pos", 8)) {
							int i = atoi(value);
							if (FOCUSER_POSITION_ITEM->number.target != i) {
								FOCUSER_TEMPERATURE_ITEM->number.target = i;
								update_position = true;
							}
						} else if (!strncmp(key, "IsMoving", 8)) {
							int i = atoi(value);
							if (FOCUSER_POSITION_PROPERTY->state != INDIGO_BUSY_STATE && i == 1) {
								FOCUSER_POSITION_PROPERTY->state = FOCUSER_STEPS_PROPERTY->state = INDIGO_BUSY_STATE;
								update_position = true;
							} else if (FOCUSER_POSITION_PROPERTY->state == INDIGO_BUSY_STATE && i == 0) {
								FOCUSER_POSITION_PROPERTY->state = FOCUSER_STEPS_PROPERTY->state = INDIGO_OK_STATE;
								update_position = true;
							}
						}
					}
				} else {
					break;
				}
			}
			if (update_temperature) {
				indigo_update_property(device, FOCUSER_TEMPERATURE_PROPERTY, NULL);
			}
			if (update_position) {
				indigo_update_property(device, FOCUSER_POSITION_PROPERTY, NULL);
				indigo_update_property(device, FOCUSER_STEPS_PROPERTY, NULL);
			}
		} else {
			INDIGO_DRIVER_DEBUG(DRIVER_NAME, "<F%dGETSTATUS>> -> %s", target, line);
		}
	}
	if (target == 1)
		indigo_reschedule_timer(device, 1, &PRIVATE_DATA->timer_1);
	else
		indigo_reschedule_timer(device, 1, &PRIVATE_DATA->timer_2);
	pthread_mutex_unlock(&PRIVATE_DATA->mutex);
}

static bool optecfl_open(indigo_device *device) {
	char line[80];
	char *name = DEVICE_PORT_ITEM->text.value;
	if (PRIVATE_DATA->count == 0) {
		PRIVATE_DATA->handle = indigo_open_serial_with_speed(name, 115200);
		if (PRIVATE_DATA->handle > 0) {
			INDIGO_DRIVER_LOG(DRIVER_NAME, "Connected to %s", name);
			if (indigo_printf(PRIVATE_DATA->handle, "<FHGETHUBINFO>") && indigo_read_line(PRIVATE_DATA->handle, line, sizeof(line)) == 1 && *line == '!') {
				INDIGO_DRIVER_DEBUG(DRIVER_NAME, "<FHGETHUBINFO> -> !");
				while (true) {
					if (indigo_read_line(PRIVATE_DATA->handle, line, sizeof(line)) >= 0) {
						INDIGO_DRIVER_DEBUG(DRIVER_NAME, "  -> %s", line);
						if (!strncmp(line, "END", 3))
							break;
					} else {
						close(PRIVATE_DATA->handle);
						PRIVATE_DATA->handle = 0;
						PRIVATE_DATA->count = 0;
						break;
					}
				}
			} else {
				INDIGO_DRIVER_DEBUG(DRIVER_NAME, "<FHGETHUBINFO> -> %s", line);
			}
		}
	}
	if (PRIVATE_DATA->handle > 0) {
		int target = device->gp_bits & 0x3;
		if (indigo_printf(PRIVATE_DATA->handle, "<F%dGETCONFIG>", target) && indigo_read_line(PRIVATE_DATA->handle, line, sizeof(line)) == 1 && *line == '!') {
			INDIGO_DRIVER_DEBUG(DRIVER_NAME, "<F%dGETCONFIG> -> !", target);
			while (true) {
				if (indigo_read_line(PRIVATE_DATA->handle, line, sizeof(line)) >= 0) {
					INDIGO_DRIVER_DEBUG(DRIVER_NAME, "  -> %s", line);
					if (!strncmp(line, "END", 3)) {
						PRIVATE_DATA->count++;
						break;
					}
					char key[16], value[80];
					if (sscanf(line, "%15[^=]=%15[^\n]s", key, value) == 2) {
						if (!strncmp(key, "Max Pos", 7)) {
							FOCUSER_LIMITS_MAX_POSITION_ITEM->number.value = FOCUSER_LIMITS_MAX_POSITION_ITEM->number.target = FOCUSER_POSITION_ITEM->number.max = atoi(value);
						} else if (!strncmp(key, "Dev Type", 8)) {
							strncpy(INFO_DEVICE_MODEL_ITEM->text.value, value, INDIGO_VALUE_SIZE);
						}
					}
				} else {
					break;
				}
			}
			return true;
		} else {
			INDIGO_DRIVER_DEBUG(DRIVER_NAME, "<F%dGETCONFIG> -> %s", target, line);
			INDIGO_DRIVER_ERROR(DRIVER_NAME, "Failed to open focuser #%d", target);
		}
	} else {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "Failed to connect to %s", name);
	}
	return false;
}

static void optecfl_close(indigo_device *device) {
	if (PRIVATE_DATA->count > 1) {
		PRIVATE_DATA->count--;
	} else {
		close(PRIVATE_DATA->handle);
		PRIVATE_DATA->handle = 0;
		INDIGO_DRIVER_LOG(DRIVER_NAME, "Disconnected from %s", DEVICE_PORT_ITEM->text.value);
	}
}

static void focuser_connection_handler(indigo_device *device) {
	pthread_mutex_lock(&PRIVATE_DATA->mutex);
	int target = device->gp_bits & 0x3;
	if (CONNECTION_CONNECTED_ITEM->sw.value) {
		if (optecfl_open(device)) {
			if (target == 1)
				indigo_set_timer(device, 0, focuser_timer_callback, &PRIVATE_DATA->timer_1);
			else
				indigo_set_timer(device, 0, focuser_timer_callback, &PRIVATE_DATA->timer_2);
			CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
		} else {
			CONNECTION_PROPERTY->state = INDIGO_ALERT_STATE;
			indigo_set_switch(CONNECTION_PROPERTY, CONNECTION_DISCONNECTED_ITEM, true);
		}
	} else {
		if (target == 1)
			indigo_cancel_timer_sync(device, &PRIVATE_DATA->timer_1);
		else
			indigo_cancel_timer_sync(device, &PRIVATE_DATA->timer_2);
		INDIGO_DRIVER_LOG(DRIVER_NAME, "Disconnected");
		optecfl_close(device);
		CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
	}
	indigo_focuser_change_property(device, NULL, CONNECTION_PROPERTY);
	pthread_mutex_unlock(&PRIVATE_DATA->mutex);
}

static void focuser_position_handler(indigo_device *device) {
	pthread_mutex_lock(&PRIVATE_DATA->mutex);
	char line[80];
	int target = device->gp_bits & 0x3;
	int position = FOCUSER_POSITION_ITEM->number.target;
	if (indigo_printf(PRIVATE_DATA->handle, "<F%dMA%06d>", target, position) && indigo_read_line(PRIVATE_DATA->handle, line, sizeof(line)) == 1 && *line == '!') {
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "<F%dMA%06d> -> !", target, position);
		if (indigo_read_line(PRIVATE_DATA->handle, line, sizeof(line)) == 1 && *line == 'M') {
			INDIGO_DRIVER_DEBUG(DRIVER_NAME, "  -> M");
			FOCUSER_STEPS_PROPERTY->state = INDIGO_BUSY_STATE;
			indigo_update_property(device, FOCUSER_STEPS_PROPERTY, NULL);
		} else {
			INDIGO_DRIVER_DEBUG(DRIVER_NAME, "  -> %s");
			FOCUSER_POSITION_PROPERTY->state = FOCUSER_STEPS_PROPERTY->state = INDIGO_ALERT_STATE;
			indigo_update_property(device, FOCUSER_STEPS_PROPERTY, NULL);
			indigo_update_property(device, FOCUSER_POSITION_PROPERTY, NULL);
		}
	} else {
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "<F%dMA%06d> -> %s", target, position, line);
		FOCUSER_POSITION_PROPERTY->state = FOCUSER_STEPS_PROPERTY->state = INDIGO_ALERT_STATE;
		indigo_update_property(device, FOCUSER_STEPS_PROPERTY, NULL);
		indigo_update_property(device, FOCUSER_POSITION_PROPERTY, NULL);
	}
	pthread_mutex_unlock(&PRIVATE_DATA->mutex);
}

static void focuser_steps_handler(indigo_device *device) {
	pthread_mutex_lock(&PRIVATE_DATA->mutex);
	char line[80];
	int target = device->gp_bits & 0x3;
	int position = FOCUSER_POSITION_ITEM->number.value + (FOCUSER_DIRECTION_MOVE_INWARD_ITEM->sw.value ^ FOCUSER_REVERSE_MOTION_ENABLED_ITEM->sw.value ? (int)-FOCUSER_STEPS_ITEM->number.value : (int)FOCUSER_STEPS_ITEM->number.value);
	if (position < 0) {
		position = 0;
	} else if (position > FOCUSER_POSITION_ITEM->number.max) {
		position = FOCUSER_POSITION_ITEM->number.max;
	}
	if (indigo_printf(PRIVATE_DATA->handle, "<F%dMA%06d>", target, position) && indigo_read_line(PRIVATE_DATA->handle, line, sizeof(line)) == 1 && *line == '!') {
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "<F%dMA%06d> -> !", target, position);
		if (indigo_read_line(PRIVATE_DATA->handle, line, sizeof(line)) == 1 && *line == 'M') {
			INDIGO_DRIVER_DEBUG(DRIVER_NAME, "  -> M");
			FOCUSER_POSITION_PROPERTY->state = INDIGO_BUSY_STATE;
			indigo_update_property(device, FOCUSER_POSITION_PROPERTY, NULL);
		} else {
			INDIGO_DRIVER_DEBUG(DRIVER_NAME, "  -> %s");
			FOCUSER_POSITION_PROPERTY->state = FOCUSER_STEPS_PROPERTY->state = INDIGO_ALERT_STATE;
			indigo_update_property(device, FOCUSER_POSITION_PROPERTY, NULL);
			indigo_update_property(device, FOCUSER_STEPS_PROPERTY, NULL);
		}
	} else {
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "<F%dMA%06d> -> %s", target, position, line);
		FOCUSER_POSITION_PROPERTY->state = FOCUSER_STEPS_PROPERTY->state = INDIGO_ALERT_STATE;
		indigo_update_property(device, FOCUSER_POSITION_PROPERTY, NULL);
		indigo_update_property(device, FOCUSER_STEPS_PROPERTY, NULL);
	}
	pthread_mutex_unlock(&PRIVATE_DATA->mutex);
}

static void focuser_abort_handler(indigo_device *device) {
	pthread_mutex_lock(&PRIVATE_DATA->mutex);
	char line[80];
	int target = device->gp_bits & 0x3;
	if (indigo_printf(PRIVATE_DATA->handle, "<F%dHALT>", target) && indigo_read_line(PRIVATE_DATA->handle, line, sizeof(line)) == 1 && *line == '!') {
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "<F%dHALT> -> !", target);
		if (indigo_read_line(PRIVATE_DATA->handle, line, sizeof(line)) == 6 && strncpy(line, "HALTED", 6)) {
			INDIGO_DRIVER_DEBUG(DRIVER_NAME, "  -> HALTED");
			FOCUSER_ABORT_MOTION_PROPERTY->state = INDIGO_OK_STATE;
			indigo_update_property(device, FOCUSER_ABORT_MOTION_PROPERTY, NULL);
		} else {
			INDIGO_DRIVER_DEBUG(DRIVER_NAME, "  -> %s");
			FOCUSER_ABORT_MOTION_PROPERTY->state = INDIGO_ALERT_STATE;
			indigo_update_property(device, FOCUSER_ABORT_MOTION_PROPERTY, NULL);
		}
	} else {
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "<F%dHALT> -> %s", target, line);
		FOCUSER_ABORT_MOTION_PROPERTY->state = INDIGO_ALERT_STATE;
		indigo_update_property(device, FOCUSER_ABORT_MOTION_PROPERTY, NULL);
	}
	pthread_mutex_unlock(&PRIVATE_DATA->mutex);
}

// -------------------------------------------------------------------------------- INDIGO focuser device implementation

static indigo_result focuser_attach(indigo_device *device) {
	assert(device != NULL);
	assert(PRIVATE_DATA != NULL);
	if (indigo_focuser_attach(device, DRIVER_NAME, DRIVER_VERSION) == INDIGO_OK) {
		// -------------------------------------------------------------------------------- DEVICE_PORT, DEVICE_PORTS
		DEVICE_PORT_PROPERTY->hidden = false;
		DEVICE_PORTS_PROPERTY->hidden = false;
#ifdef INDIGO_MACOS
		for (int i = 0; i < DEVICE_PORTS_PROPERTY->count; i++) {
			if (!strncmp(DEVICE_PORTS_PROPERTY->items[i].name, "/dev/cu.usbmodem", 16)) {
				indigo_copy_value(DEVICE_PORT_ITEM->text.value, DEVICE_PORTS_PROPERTY->items[i].name);
				break;
			}
		}
#endif
#ifdef INDIGO_LINUX
		strcpy(DEVICE_PORT_ITEM->text.value, "/dev/usb_focuser");
#endif
		// -------------------------------------------------------------------------------- FOCUSER_TEMPERATURE
		FOCUSER_TEMPERATURE_PROPERTY->hidden = false;
		// -------------------------------------------------------------------------------- FOCUSER_SPEED
		FOCUSER_SPEED_PROPERTY->hidden = true;
		// -------------------------------------------------------------------------------- FOCUSER_STEPS
		FOCUSER_STEPS_ITEM->number.min = 0;
		FOCUSER_STEPS_ITEM->number.max = 99999;
		FOCUSER_STEPS_ITEM->number.step = 1;
//		// -------------------------------------------------------------------------------- FOCUSER_COMPENSATION
//		FOCUSER_COMPENSATION_PROPERTY->hidden = false;
//		FOCUSER_COMPENSATION_ITEM->number.min = -999;
//		FOCUSER_COMPENSATION_ITEM->number.max = 999;
//		// -------------------------------------------------------------------------------- FOCUSER_MODE
//		FOCUSER_MODE_PROPERTY->hidden = false;
		// -------------------------------------------------------------------------------- FOCUSER_REVERSE_MOTION
		FOCUSER_REVERSE_MOTION_PROPERTY->hidden = false;
		// --------------------------------------------------------------------------------
		pthread_mutex_init(&PRIVATE_DATA->mutex, NULL);
		INDIGO_DEVICE_ATTACH_LOG(DRIVER_NAME, device->name);
		return indigo_focuser_enumerate_properties(device, NULL, NULL);
	}
	return INDIGO_FAILED;
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
		indigo_set_timer(device, 0, focuser_connection_handler, NULL);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(FOCUSER_POSITION_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- FOCUSER_POSITION
		indigo_property_copy_values(FOCUSER_POSITION_PROPERTY, property, false);
		FOCUSER_POSITION_PROPERTY->state = INDIGO_BUSY_STATE;
		indigo_update_property(device, FOCUSER_POSITION_PROPERTY, NULL);
		indigo_set_timer(device, 0, focuser_position_handler, NULL);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(FOCUSER_STEPS_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- FOCUSER_STEPS
		indigo_property_copy_values(FOCUSER_STEPS_PROPERTY, property, false);
		FOCUSER_STEPS_PROPERTY->state = INDIGO_BUSY_STATE;
		indigo_update_property(device, FOCUSER_STEPS_PROPERTY, NULL);
		indigo_set_timer(device, 0, focuser_steps_handler, NULL);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(FOCUSER_ABORT_MOTION_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- FOCUSER_ABORT_MOTION
		indigo_property_copy_values(FOCUSER_ABORT_MOTION_PROPERTY, property, false);
		FOCUSER_ABORT_MOTION_PROPERTY->state = INDIGO_BUSY_STATE;
		indigo_update_property(device, FOCUSER_ABORT_MOTION_PROPERTY, NULL);
		indigo_set_timer(device, 0, focuser_abort_handler, NULL);
		return INDIGO_OK;
//	} else if (indigo_property_match_changeable(FOCUSER_COMPENSATION_PROPERTY, property)) {
//		// -------------------------------------------------------------------------------- FOCUSER_COMPENSATION
//		indigo_property_copy_values(FOCUSER_COMPENSATION_PROPERTY, property, false);
//		FOCUSER_MODE_PROPERTY->state = INDIGO_BUSY_STATE;
//		indigo_update_property(device, FOCUSER_MODE_PROPERTY, NULL);
//		indigo_set_timer(device, 0, focuser_compensation_handler, NULL);
//		return INDIGO_OK;
//	} else if (indigo_property_match_changeable(FOCUSER_MODE_PROPERTY, property)) {
//		// -------------------------------------------------------------------------------- FOCUSER_MODE
//		indigo_property_copy_values(FOCUSER_MODE_PROPERTY, property, false);
//		FOCUSER_MODE_PROPERTY->state = INDIGO_BUSY_STATE;
//		indigo_update_property(device, FOCUSER_MODE_PROPERTY, NULL);
//		indigo_set_timer(device, 0, focuser_mode_handler, NULL);
//		return INDIGO_OK;
	}
	return indigo_focuser_change_property(device, client, property);
}

static indigo_result focuser_detach(indigo_device *device) {
	assert(device != NULL);
	if (IS_CONNECTED) {
		indigo_set_switch(CONNECTION_PROPERTY, CONNECTION_DISCONNECTED_ITEM, true);
		focuser_connection_handler(device);
	}
	pthread_mutex_destroy(&PRIVATE_DATA->mutex);
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
		indigo_focuser_enumerate_properties,
		focuser_change_property,
		NULL,
		focuser_detach
	);

	SET_DRIVER_INFO(info, "Optec FocusLynx Focuser", __FUNCTION__, DRIVER_VERSION, false, last_action);

	if (action == last_action)
		return INDIGO_OK;

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
			indigo_attach_device(focuser2);
			break;

		case INDIGO_DRIVER_SHUTDOWN:
			VERIFY_NOT_CONNECTED(focuser1);
			VERIFY_NOT_CONNECTED(focuser2);
			last_action = action;
			if (focuser1 != NULL) {
				indigo_detach_device(focuser1);
				free(focuser1);
				focuser1 = NULL;
			}
			if (focuser2 != NULL) {
				indigo_detach_device(focuser2);
				free(focuser2);
				focuser2 = NULL;
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
