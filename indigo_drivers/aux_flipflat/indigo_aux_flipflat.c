// Copyright (c) 2019 CloudMakers, s. r. o.
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

/** INDIGO FlipFlat aux driver
 \file indigo_aux_flipflat.c
 */

#define DRIVER_VERSION 0x0006
#define DRIVER_NAME "indigo_aux_flipflat"

#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <math.h>
#include <assert.h>
#include <pthread.h>
#include <termios.h>
#include <errno.h>
#include <sys/time.h>
#include <sys/ioctl.h>

#include <indigo/indigo_driver_xml.h>
#include <indigo/indigo_io.h>
#include "indigo_aux_flipflat.h"

#define PRIVATE_DATA												((flipflat_private_data *)device->private_data)

#define AUX_LIGHT_SWITCH_PROPERTY          	(PRIVATE_DATA->light_switch_property)
#define AUX_LIGHT_SWITCH_ON_ITEM            (AUX_LIGHT_SWITCH_PROPERTY->items+0)
#define AUX_LIGHT_SWITCH_OFF_ITEM           (AUX_LIGHT_SWITCH_PROPERTY->items+1)

#define AUX_LIGHT_INTENSITY_PROPERTY				(PRIVATE_DATA->light_intensity_property)
#define AUX_LIGHT_INTENSITY_ITEM						(AUX_LIGHT_INTENSITY_PROPERTY->items+0)

#define AUX_COVER_PROPERTY          				(PRIVATE_DATA->cover_property)
#define AUX_COVER_CLOSE_ITEM            		(AUX_COVER_PROPERTY->items+0)
#define AUX_COVER_OPEN_ITEM           			(AUX_COVER_PROPERTY->items+1)

typedef struct {
	int handle;
	indigo_property *light_switch_property;
	indigo_property *light_intensity_property;
	indigo_property *cover_property;
	pthread_mutex_t mutex;
	int type;
} flipflat_private_data;

static bool flipflat_command(int handle, char *command, char *response) {
	int result = indigo_write(handle, command, strlen(command));
	result |= indigo_write(handle, "\r", 1);
	INDIGO_DRIVER_DEBUG(DRIVER_NAME, "%d <- %s (%s)", handle, command, result ? "OK" : strerror(errno));
	if (result) {
		*response = 0;
		result = indigo_read_line(handle, response, 10) > 0;
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "%d -> %s (%s)", handle, response, result ? "OK" : strerror(errno));
	}
	return result;
}

// -------------------------------------------------------------------------------- INDIGO aux device implementation

static indigo_result aux_attach(indigo_device *device) {
	assert(device != NULL);
	assert(PRIVATE_DATA != NULL);
	if (indigo_aux_attach(device, DRIVER_NAME, DRIVER_VERSION, INDIGO_INTERFACE_AUX_LIGHTBOX) == INDIGO_OK) {
		// -------------------------------------------------------------------------------- AUX_LIGHT_SWITCH
		AUX_LIGHT_SWITCH_PROPERTY = indigo_init_switch_property(NULL, device->name, AUX_LIGHT_SWITCH_PROPERTY_NAME, AUX_MAIN_GROUP, "Light (on/off)", INDIGO_OK_STATE, INDIGO_RW_PERM, INDIGO_ONE_OF_MANY_RULE, 2);
		if (AUX_LIGHT_SWITCH_PROPERTY == NULL)
			return INDIGO_FAILED;
		indigo_init_switch_item(AUX_LIGHT_SWITCH_ON_ITEM, AUX_LIGHT_SWITCH_ON_ITEM_NAME, "On", false);
		indigo_init_switch_item(AUX_LIGHT_SWITCH_OFF_ITEM, AUX_LIGHT_SWITCH_OFF_ITEM_NAME, "Off", true);
		// -------------------------------------------------------------------------------- AUX_LIGHT_INTENSITY
		AUX_LIGHT_INTENSITY_PROPERTY = indigo_init_number_property(NULL, device->name, AUX_LIGHT_INTENSITY_PROPERTY_NAME, AUX_MAIN_GROUP, "Light intensity", INDIGO_OK_STATE, INDIGO_RW_PERM, 1);
		if (AUX_LIGHT_INTENSITY_PROPERTY == NULL)
			return INDIGO_FAILED;
		indigo_init_number_item(AUX_LIGHT_INTENSITY_ITEM, AUX_LIGHT_INTENSITY_ITEM_NAME, "Intensity", 0, 255, 1, 0);
		strcpy(AUX_LIGHT_INTENSITY_ITEM->number.format, "%g");
		// -------------------------------------------------------------------------------- AUX_COVER
		AUX_COVER_PROPERTY = indigo_init_switch_property(NULL, device->name, AUX_COVER_PROPERTY_NAME, AUX_MAIN_GROUP, "Cover (open/close)", INDIGO_OK_STATE, INDIGO_RW_PERM, INDIGO_ONE_OF_MANY_RULE, 2);
		if (AUX_COVER_PROPERTY == NULL)
			return INDIGO_FAILED;
		indigo_init_switch_item(AUX_COVER_OPEN_ITEM, AUX_COVER_OPEN_ITEM_NAME, "Open", false);
		indigo_init_switch_item(AUX_COVER_CLOSE_ITEM, AUX_COVER_CLOSE_ITEM_NAME, "Close", true);
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
		strcpy(DEVICE_PORT_ITEM->text.value, "/dev/ttyUSB0");
#endif
		// --------------------------------------------------------------------------------
		ADDITIONAL_INSTANCES_PROPERTY->hidden = DEVICE_CONTEXT->base_device != NULL;
		pthread_mutex_init(&PRIVATE_DATA->mutex, NULL);
		INDIGO_DEVICE_ATTACH_LOG(DRIVER_NAME, device->name);
		return indigo_aux_enumerate_properties(device, NULL, NULL);
	}
	return INDIGO_FAILED;
}

static indigo_result aux_enumerate_properties(indigo_device *device, indigo_client *client, indigo_property *property) {
	if (IS_CONNECTED) {
		if (indigo_property_match(AUX_LIGHT_SWITCH_PROPERTY, property))
			indigo_define_property(device, AUX_LIGHT_SWITCH_PROPERTY, NULL);
		if (indigo_property_match(AUX_LIGHT_INTENSITY_PROPERTY, property))
			indigo_define_property(device, AUX_LIGHT_INTENSITY_PROPERTY, NULL);
	if (indigo_property_match(AUX_COVER_PROPERTY, property))
		indigo_define_property(device, AUX_COVER_PROPERTY, NULL);
	}
	return indigo_aux_enumerate_properties(device, NULL, NULL);
}

static void aux_connection_handler(indigo_device *device) {
	char response[16];
	pthread_mutex_lock(&PRIVATE_DATA->mutex);
	if (CONNECTION_CONNECTED_ITEM->sw.value) {
		for (int i = 0; i < 2; i++) {
			PRIVATE_DATA->handle = indigo_open_serial(DEVICE_PORT_ITEM->text.value);
			if (PRIVATE_DATA->handle > 0) {
				INDIGO_DRIVER_LOG(DRIVER_NAME, "Connected on %s", DEVICE_PORT_ITEM->text.value);
				int bits = TIOCM_DTR;
				int result = ioctl(PRIVATE_DATA->handle, TIOCMBIS, &bits);
				INDIGO_DRIVER_DEBUG(DRIVER_NAME, "%d ← DTR %s", PRIVATE_DATA->handle, result < 0 ? strerror(errno) : "set");
				indigo_usleep(100000);
				bits = TIOCM_RTS;
				result = ioctl(PRIVATE_DATA->handle, TIOCMBIC, &bits);
				INDIGO_DRIVER_DEBUG(DRIVER_NAME, "%d ← RTS %s", PRIVATE_DATA->handle, result < 0 ? strerror(errno) : "cleared");
				indigo_usleep(2 * ONE_SECOND_DELAY);
				if (flipflat_command(PRIVATE_DATA->handle, ">P000", response) && *response == '*') {
					if (sscanf(response, "*P%02d000", &PRIVATE_DATA->type) != 1)
						PRIVATE_DATA->type = 0;
					switch (PRIVATE_DATA->type) {
						case 10:
							AUX_LIGHT_SWITCH_PROPERTY->hidden = AUX_LIGHT_INTENSITY_PROPERTY->hidden = false;
							AUX_COVER_PROPERTY->hidden = true;
							INDIGO_DRIVER_LOG(DRIVER_NAME, "Flat-Man_XL detected");
							break;
						case 15:
							AUX_LIGHT_SWITCH_PROPERTY->hidden = AUX_LIGHT_INTENSITY_PROPERTY->hidden = false;
							AUX_COVER_PROPERTY->hidden = true;
							INDIGO_DRIVER_LOG(DRIVER_NAME, "Flat-Man_L detected");
							break;
						case 19:
							AUX_LIGHT_SWITCH_PROPERTY->hidden = AUX_LIGHT_INTENSITY_PROPERTY->hidden = false;
							AUX_COVER_PROPERTY->hidden = true;
							INDIGO_DRIVER_LOG(DRIVER_NAME, "Flat-Man detected");
							break;
						case 98:
							AUX_LIGHT_SWITCH_PROPERTY->hidden = AUX_LIGHT_INTENSITY_PROPERTY->hidden = true;
							AUX_COVER_PROPERTY->hidden = false;
							INDIGO_DRIVER_LOG(DRIVER_NAME, "Flip-Mask/Remote Dust Cover detected");
							break;
						case 99:
							AUX_LIGHT_SWITCH_PROPERTY->hidden = AUX_LIGHT_INTENSITY_PROPERTY->hidden = false;
							AUX_COVER_PROPERTY->hidden = false;
							INDIGO_DRIVER_LOG(DRIVER_NAME, "Flip-Flap detected");
							break;
						default:
							AUX_LIGHT_SWITCH_PROPERTY->hidden = AUX_LIGHT_INTENSITY_PROPERTY->hidden = false;
							AUX_COVER_PROPERTY->hidden = false;
							INDIGO_DRIVER_LOG(DRIVER_NAME, "Unknown device detected");
							break;
					}
					break;
				} else {
					INDIGO_DRIVER_ERROR(DRIVER_NAME, "Handshake failed");
					close(PRIVATE_DATA->handle);
					PRIVATE_DATA->handle = 0;
				}
			}
		}
		if (PRIVATE_DATA->handle > 0) {
			if (!AUX_LIGHT_SWITCH_PROPERTY->hidden) {
				AUX_LIGHT_SWITCH_PROPERTY->state = INDIGO_ALERT_STATE;
				AUX_COVER_PROPERTY->state = INDIGO_ALERT_STATE;
				if (flipflat_command(PRIVATE_DATA->handle, ">S000", response) && *response == '*') {
					int type, q, r, s;
					if (sscanf(response, "*S%02d%1d%1d%1d", &type, &q, &r, &s) == 4) {
						if (s == 1 || s == 2) {
							AUX_COVER_PROPERTY->state = INDIGO_OK_STATE;
							indigo_set_switch(AUX_COVER_PROPERTY, s == 1 ? AUX_COVER_CLOSE_ITEM : AUX_COVER_OPEN_ITEM, true);
						}
						AUX_LIGHT_SWITCH_PROPERTY->state = INDIGO_OK_STATE;
						indigo_set_switch(AUX_LIGHT_SWITCH_PROPERTY, r == 1 ? AUX_LIGHT_SWITCH_ON_ITEM : AUX_LIGHT_SWITCH_OFF_ITEM, true);
					}
				}
			}
			if (!AUX_LIGHT_INTENSITY_PROPERTY->hidden) {
				AUX_LIGHT_INTENSITY_PROPERTY->state = INDIGO_ALERT_STATE;
				if (flipflat_command(PRIVATE_DATA->handle, ">J000", response) && *response == '*') {
					int type, value;
					if (sscanf(response, "*J%02d%3d", &type, &value) == 2) {
						AUX_LIGHT_INTENSITY_ITEM->number.value = AUX_LIGHT_INTENSITY_ITEM->number.target = value;
						AUX_LIGHT_INTENSITY_PROPERTY->state = INDIGO_OK_STATE;
					}
				}
			}
			indigo_define_property(device, AUX_LIGHT_SWITCH_PROPERTY, NULL);
			indigo_define_property(device, AUX_LIGHT_INTENSITY_PROPERTY, NULL);
			indigo_define_property(device, AUX_COVER_PROPERTY, NULL);
			CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
		} else {
			INDIGO_DRIVER_ERROR(DRIVER_NAME, "Failed to connect to %s", DEVICE_PORT_ITEM->text.value);
			CONNECTION_PROPERTY->state = INDIGO_ALERT_STATE;
			indigo_set_switch(CONNECTION_PROPERTY, CONNECTION_DISCONNECTED_ITEM, true);
		}
	} else {
		indigo_delete_property(device, AUX_LIGHT_SWITCH_PROPERTY, NULL);
		indigo_delete_property(device, AUX_LIGHT_INTENSITY_PROPERTY, NULL);
		indigo_delete_property(device, AUX_COVER_PROPERTY, NULL);
		close(PRIVATE_DATA->handle);
		PRIVATE_DATA->handle = 0;
		INDIGO_DRIVER_LOG(DRIVER_NAME, "Disconnected");
		CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
	}
	indigo_aux_change_property(device, NULL, CONNECTION_PROPERTY);
	pthread_mutex_unlock(&PRIVATE_DATA->mutex);
}

static void aux_switch_handler(indigo_device *device) {
	pthread_mutex_lock(&PRIVATE_DATA->mutex);
	char command[16],	response[16];
	strcpy(command, AUX_LIGHT_SWITCH_ON_ITEM->sw.value ? ">L000" : ">D000");
	if (flipflat_command(PRIVATE_DATA->handle, command, response) && *response == '*')
		AUX_LIGHT_SWITCH_PROPERTY->state = INDIGO_OK_STATE;
	else
		AUX_LIGHT_SWITCH_PROPERTY->state = INDIGO_ALERT_STATE;
	indigo_update_property(device, AUX_LIGHT_SWITCH_PROPERTY, NULL);
	pthread_mutex_unlock(&PRIVATE_DATA->mutex);
}

static void aux_intensity_handler(indigo_device *device) {
	pthread_mutex_lock(&PRIVATE_DATA->mutex);
	char command[16],	response[16];
	sprintf(command, ">B%03d", (int)(AUX_LIGHT_INTENSITY_ITEM->number.value));
	if (flipflat_command(PRIVATE_DATA->handle, command, response))
		AUX_LIGHT_INTENSITY_PROPERTY->state = INDIGO_OK_STATE;
	else
		AUX_LIGHT_INTENSITY_PROPERTY->state = INDIGO_ALERT_STATE;
	indigo_update_property(device, AUX_LIGHT_INTENSITY_PROPERTY, NULL);
	pthread_mutex_unlock(&PRIVATE_DATA->mutex);
}

static void aux_cover_handler(indigo_device *device) {
	pthread_mutex_lock(&PRIVATE_DATA->mutex);
	char command[16],	response[16];
	strcpy(command, AUX_COVER_OPEN_ITEM->sw.value ? ">O000" : ">C000");
	if (flipflat_command(PRIVATE_DATA->handle, command, response) && *response == '*') {
		AUX_COVER_PROPERTY->state = INDIGO_BUSY_STATE;
		indigo_update_property(device, AUX_COVER_PROPERTY, NULL);
		AUX_COVER_PROPERTY->state = INDIGO_ALERT_STATE;
		for (int i = 0; i < 10; i++) {
			indigo_usleep(ONE_SECOND_DELAY);
			if (flipflat_command(PRIVATE_DATA->handle, ">S000", response) && *response == '*') {
				int type, q, r, s;
				if (sscanf(response, "*S%02d%1d%1d%1d", &type, &q, &r, &s) == 4) {
					if ((AUX_COVER_OPEN_ITEM->sw.value && s == 2) || (AUX_COVER_CLOSE_ITEM->sw.value && s == 1)) {
						AUX_COVER_PROPERTY->state = INDIGO_OK_STATE;
						break;
					}
				}
			}
		}
	} else {
		AUX_COVER_PROPERTY->state = INDIGO_ALERT_STATE;
	}
	indigo_update_property(device, AUX_COVER_PROPERTY, NULL);
	pthread_mutex_unlock(&PRIVATE_DATA->mutex);
}


static indigo_result aux_change_property(indigo_device *device, indigo_client *client, indigo_property *property) {
	assert(device != NULL);
	assert(DEVICE_CONTEXT != NULL);
	assert(property != NULL);
	if (indigo_property_match(CONNECTION_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- CONNECTION
		if (indigo_ignore_connection_change(device, property))
			return INDIGO_OK;
		indigo_property_copy_values(CONNECTION_PROPERTY, property, false);
		CONNECTION_PROPERTY->state = INDIGO_BUSY_STATE;
		indigo_update_property(device, CONNECTION_PROPERTY, NULL);
		indigo_set_timer(device, 0, aux_connection_handler, NULL);
		return INDIGO_OK;
		// -------------------------------------------------------------------------------- AUX_LIGHT_SWITCH
	} else if (indigo_property_match(AUX_LIGHT_SWITCH_PROPERTY, property)) {
		indigo_property_copy_values(AUX_LIGHT_SWITCH_PROPERTY, property, false);
		if (IS_CONNECTED) {
			AUX_LIGHT_SWITCH_PROPERTY->state = INDIGO_BUSY_STATE;
			indigo_update_property(device, AUX_LIGHT_SWITCH_PROPERTY, NULL);
			indigo_set_timer(device, 0, aux_switch_handler, NULL);
		}
		return INDIGO_OK;
		// -------------------------------------------------------------------------------- AUX_LIGHT_INTENSITY
	} else if (indigo_property_match(AUX_LIGHT_INTENSITY_PROPERTY, property)) {
		indigo_property_copy_values(AUX_LIGHT_INTENSITY_PROPERTY, property, false);
		if (IS_CONNECTED) {
			AUX_LIGHT_INTENSITY_PROPERTY->state = INDIGO_BUSY_STATE;
			indigo_update_property(device, AUX_LIGHT_INTENSITY_PROPERTY, NULL);
			indigo_set_timer(device, 0, aux_intensity_handler, NULL);
		}
		return INDIGO_OK;
		// -------------------------------------------------------------------------------- AUX_COVER
	} else if (indigo_property_match(AUX_COVER_PROPERTY, property)) {
		indigo_property_copy_values(AUX_COVER_PROPERTY, property, false);
		if (IS_CONNECTED) {
			AUX_COVER_PROPERTY->state = INDIGO_BUSY_STATE;
			indigo_update_property(device, AUX_COVER_PROPERTY, NULL);
			indigo_set_timer(device, 0, aux_cover_handler, NULL);
		}
		return INDIGO_OK;
	}
	return indigo_aux_change_property(device, client, property);
}

static indigo_result aux_detach(indigo_device *device) {
	assert(device != NULL);
	if (IS_CONNECTED) {
		indigo_set_switch(CONNECTION_PROPERTY, CONNECTION_DISCONNECTED_ITEM, true);
		aux_connection_handler(device);
	}
	indigo_release_property(AUX_LIGHT_SWITCH_PROPERTY);
	indigo_release_property(AUX_LIGHT_INTENSITY_PROPERTY);
	indigo_release_property(AUX_COVER_PROPERTY);
	pthread_mutex_destroy(&PRIVATE_DATA->mutex);
	INDIGO_DEVICE_DETACH_LOG(DRIVER_NAME, device->name);
	return indigo_aux_detach(device);
}

// -------------------------------------------------------------------------------- INDIGO driver implementation

indigo_result indigo_aux_flipflat(indigo_driver_action action, indigo_driver_info *info) {
	static indigo_driver_action last_action = INDIGO_DRIVER_SHUTDOWN;
	static flipflat_private_data *private_data = NULL;
	static indigo_device *aux = NULL;

	static indigo_device aux_template = INDIGO_DEVICE_INITIALIZER(
		"Flip-Flat",
		aux_attach,
		aux_enumerate_properties,
		aux_change_property,
		NULL,
		aux_detach
		);

	SET_DRIVER_INFO(info, "Optec Flip-Flat", __FUNCTION__, DRIVER_VERSION, false, last_action);

	if (action == last_action)
		return INDIGO_OK;

	switch (action) {
		case INDIGO_DRIVER_INIT:
			last_action = action;
			private_data = indigo_safe_malloc(sizeof(flipflat_private_data));
			aux = indigo_safe_malloc_copy(sizeof(indigo_device), &aux_template);
			aux->private_data = private_data;
			indigo_attach_device(aux);
			break;

		case INDIGO_DRIVER_SHUTDOWN:
			VERIFY_NOT_CONNECTED(aux);
			last_action = action;
			if (aux != NULL) {
				indigo_detach_device(aux);
				free(aux);
				aux = NULL;
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
