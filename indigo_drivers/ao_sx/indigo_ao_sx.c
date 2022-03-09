// Copyright (c) 2016 CloudMakers, s. r. o.
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

/** INDIGO StarlighXpress AO driver
 \file indigo_ao_sx.c
 */

#define DRIVER_VERSION 0x0007
#define DRIVER_NAME	"indigo_ao_sx"

#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <math.h>
#include <assert.h>
#include <errno.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/time.h>

#include <indigo/indigo_driver_xml.h>
#include <indigo/indigo_io.h>

#include "indigo_ao_sx.h"

#define PRIVATE_DATA        ((sx_private_data *)device->private_data)

typedef struct {
	int handle;
	int device_count;
	pthread_mutex_t mutex;
} sx_private_data;

// -------------------------------------------------------------------------------- Low level communication routines

static bool sx_flush(indigo_device *device) {
	char c;
	struct timeval tv;
	while (true) {
		fd_set readout;
		FD_ZERO(&readout);
		FD_SET(PRIVATE_DATA->handle, &readout);
		tv.tv_sec = 0;
		tv.tv_usec = 1000;
		long result = select(PRIVATE_DATA->handle+1, &readout, NULL, NULL, &tv);
		if (result == 0)
			return true;
		if (result < 0) {
			return false;
		}
		result = read(PRIVATE_DATA->handle, &c, 1);
		if (result < 1) {
			return false;
		}
	}
}

static bool sx_command(indigo_device *device, char *command, char *response, int max) {
	char c;
	struct timeval tv;
	indigo_write(PRIVATE_DATA->handle, command, strlen(command));
	if (response != NULL) {
		int index = 0;
		int timeout = *command == 'K' || *command == 'R' ? 15 : 1;
		*response = 0;
		while (index < max) {
			fd_set readout;
			FD_ZERO(&readout);
			FD_SET(PRIVATE_DATA->handle, &readout);
			tv.tv_sec = timeout;
			tv.tv_usec = 100000;
			long result = select(PRIVATE_DATA->handle+1, &readout, NULL, NULL, &tv);
			if (result <= 0)
				break;
			result = read(PRIVATE_DATA->handle, &c, 1);
			if (result < 1) {
				INDIGO_DRIVER_ERROR(DRIVER_NAME, "Failed to read from %s -> %s (%d)", DEVICE_PORT_ITEM->text.value, strerror(errno), errno);
				return false;
			}
			response[index++] = c;
		}
		response[index] = 0;
	}
	INDIGO_DRIVER_DEBUG(DRIVER_NAME, "Command %s -> %s", command, response != NULL ? response : "NULL");
	return true;
}

static bool sx_open(indigo_device *device) {
	if (PRIVATE_DATA->device_count++ > 0)
		return true;
	char *name = DEVICE_PORT_ITEM->text.value;
	PRIVATE_DATA->handle = indigo_open_serial(name);
	if (PRIVATE_DATA->handle >= 0) {
		char response[5];
		if (sx_flush(device)) {
			if (sx_command(device, "X", response, 1) && *response == 'Y') {
				if (sx_command(device, "V", response, 4) && *response == 'V') {
					INDIGO_DRIVER_LOG(DRIVER_NAME, "Connected to %s", name);
					return true;
				}
			}
		}
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "Handshake failed on %s", name);
	} else {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "Failed to connect to %s", name);
	}
	PRIVATE_DATA->device_count = 0;
	return false;
}

static void sx_close(indigo_device *device) {
	if (--PRIVATE_DATA->device_count > 0)
		return;
	if (PRIVATE_DATA->handle > 0) {
		close(PRIVATE_DATA->handle);
		PRIVATE_DATA->handle = 0;
		INDIGO_DRIVER_LOG(DRIVER_NAME, "Disconnected from %s", DEVICE_PORT_ITEM->text.value);
	}
	PRIVATE_DATA->device_count = 0;
}

// -------------------------------------------------------------------------------- INDIGO AO device implementation

static indigo_result ao_attach(indigo_device *device) {
	assert(device != NULL);
	assert(PRIVATE_DATA != NULL);
	if (indigo_ao_attach(device, DRIVER_NAME, DRIVER_VERSION) == INDIGO_OK) {
		DEVICE_PORT_PROPERTY->hidden = false;
		DEVICE_PORTS_PROPERTY->hidden = false;
		AO_GUIDE_NORTH_ITEM->number.max = AO_GUIDE_SOUTH_ITEM->number.max = AO_GUIDE_EAST_ITEM->number.max = AO_GUIDE_WEST_ITEM->number.max = 50;
		ADDITIONAL_INSTANCES_PROPERTY->hidden = DEVICE_CONTEXT->base_device != NULL;
		pthread_mutex_init(&PRIVATE_DATA->mutex, NULL);
		INDIGO_DEVICE_ATTACH_LOG(DRIVER_NAME, device->name);
		return indigo_ao_enumerate_properties(device, NULL, NULL);
	}
	return INDIGO_FAILED;
}

static void ao_connection_handler(indigo_device *device) {
	pthread_mutex_lock(&PRIVATE_DATA->mutex);
	if (CONNECTION_CONNECTED_ITEM->sw.value) {
		if (sx_open(device)) {
			char response[2];
			if (sx_command(device, "L", response, 1)) {
				AO_GUIDE_DEC_PROPERTY->state = INDIGO_OK_STATE;
				AO_GUIDE_RA_PROPERTY->state = INDIGO_OK_STATE;
				if (response[0] & 0x05)
					AO_GUIDE_DEC_PROPERTY->state = INDIGO_ALERT_STATE;
				if (response[0] & 0x0A)
					AO_GUIDE_RA_PROPERTY->state = INDIGO_ALERT_STATE;
			}
			CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
		} else {
			CONNECTION_PROPERTY->state = INDIGO_ALERT_STATE;
			indigo_set_switch(CONNECTION_PROPERTY, CONNECTION_DISCONNECTED_ITEM, true);
		}
	} else {
		sx_close(device);
		CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
	}
	indigo_ao_change_property(device, NULL, CONNECTION_PROPERTY);
	pthread_mutex_unlock(&PRIVATE_DATA->mutex);
}

static void ao_guide_dec_handler(indigo_device *device) {
	char response[2], command[16];
	pthread_mutex_lock(&PRIVATE_DATA->mutex);
	if (AO_GUIDE_NORTH_ITEM->number.value > 0) {
		sprintf(command, "GN%05d", (int)AO_GUIDE_NORTH_ITEM->number.value);
		sx_command(device, command, response, 1);
	} else if (AO_GUIDE_SOUTH_ITEM->number.value > 0) {
		sprintf(command, "GS%05d", (int)AO_GUIDE_SOUTH_ITEM->number.value);
		sx_command(device, command, response, 1);
	}
	AO_GUIDE_NORTH_ITEM->number.value = AO_GUIDE_SOUTH_ITEM->number.value = 0;
	AO_GUIDE_DEC_PROPERTY->state = *response == 'G' ? INDIGO_OK_STATE : INDIGO_ALERT_STATE;
	indigo_update_property(device, AO_GUIDE_DEC_PROPERTY, NULL);
	pthread_mutex_unlock(&PRIVATE_DATA->mutex);
}

static void ao_guide_ra_handler(indigo_device *device) {
	char response[2], command[16];
	pthread_mutex_lock(&PRIVATE_DATA->mutex);
	if (AO_GUIDE_WEST_ITEM->number.value > 0) {
		sprintf(command, "GW%05d", (int)AO_GUIDE_WEST_ITEM->number.value);
		sx_command(device, command, response, 1);
	} else if (AO_GUIDE_EAST_ITEM->number.value > 0) {
		sprintf(command, "GT%05d", (int)AO_GUIDE_EAST_ITEM->number.value);
		sx_command(device, command, response, 1);
	}
	AO_GUIDE_WEST_ITEM->number.value = AO_GUIDE_EAST_ITEM->number.value = 0;
	AO_GUIDE_RA_PROPERTY->state = *response == 'G' ? INDIGO_OK_STATE : INDIGO_ALERT_STATE;
	indigo_update_property(device, AO_GUIDE_RA_PROPERTY, NULL);
	pthread_mutex_unlock(&PRIVATE_DATA->mutex);
}

static void ao_reset_handler(indigo_device *device) {
	char response[2] = { 0 };
	pthread_mutex_lock(&PRIVATE_DATA->mutex);
	if (AO_CENTER_ITEM->sw.value) {
		sx_command(device, "K", response, 1);
		AO_GUIDE_DEC_PROPERTY->state = INDIGO_OK_STATE;
		indigo_update_property(device, AO_GUIDE_DEC_PROPERTY, NULL);
		AO_GUIDE_RA_PROPERTY->state = INDIGO_OK_STATE;
		indigo_update_property(device, AO_GUIDE_RA_PROPERTY, NULL);
	} else if (AO_UNJAM_ITEM->sw.value) {
		sx_command(device, "R", response, 1);
		AO_GUIDE_DEC_PROPERTY->state = INDIGO_OK_STATE;
		indigo_update_property(device, AO_GUIDE_DEC_PROPERTY, NULL);
		AO_GUIDE_RA_PROPERTY->state = INDIGO_OK_STATE;
		indigo_update_property(device, AO_GUIDE_RA_PROPERTY, NULL);
	}
	AO_CENTER_ITEM->sw.value = AO_UNJAM_ITEM->sw.value = false;
	AO_RESET_PROPERTY->state = *response == 'K' ? INDIGO_OK_STATE : INDIGO_ALERT_STATE;
	indigo_update_property(device, AO_RESET_PROPERTY, NULL);
	pthread_mutex_unlock(&PRIVATE_DATA->mutex);
}

static indigo_result ao_change_property(indigo_device *device, indigo_client *client, indigo_property *property) {
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
		indigo_set_timer(device, 0, ao_connection_handler, NULL);
		return INDIGO_OK;
	} else if (indigo_property_match(AO_GUIDE_DEC_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- AO_GUIDE_DEC
		indigo_property_copy_values(AO_GUIDE_DEC_PROPERTY, property, false);
		AO_GUIDE_DEC_PROPERTY->state = INDIGO_BUSY_STATE;
		indigo_update_property(device, AO_GUIDE_DEC_PROPERTY, NULL);
		indigo_set_timer(device, 0, ao_guide_dec_handler, NULL);
		return INDIGO_OK;
	} else if (indigo_property_match(AO_GUIDE_RA_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- AO_GUIDE_RA
		indigo_property_copy_values(AO_GUIDE_RA_PROPERTY, property, false);
		AO_GUIDE_RA_PROPERTY->state = INDIGO_BUSY_STATE;
		indigo_update_property(device, AO_GUIDE_RA_PROPERTY, NULL);
		indigo_set_timer(device, 0, ao_guide_ra_handler, NULL);
		return INDIGO_OK;
	} else if (indigo_property_match(AO_RESET_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- AO_RESET
		indigo_property_copy_values(AO_RESET_PROPERTY, property, false);
		AO_RESET_PROPERTY->state = INDIGO_BUSY_STATE;
		indigo_update_property(device, AO_RESET_PROPERTY, NULL);
		indigo_set_timer(device, 0, ao_reset_handler, NULL);
		return INDIGO_OK;
		// --------------------------------------------------------------------------------
	}
	return indigo_ao_change_property(device, client, property);
}

static indigo_result ao_detach(indigo_device *device) {
	assert(device != NULL);
	if (IS_CONNECTED) {
		indigo_set_switch(CONNECTION_PROPERTY, CONNECTION_DISCONNECTED_ITEM, true);
		ao_connection_handler(device);
	}
	pthread_mutex_destroy(&PRIVATE_DATA->mutex);
	INDIGO_DEVICE_DETACH_LOG(DRIVER_NAME, device->name);
	return indigo_ao_detach(device);
}

// -------------------------------------------------------------------------------- INDIGO guider device implementation

static indigo_result guider_attach(indigo_device *device) {
	assert(device != NULL);
	assert(PRIVATE_DATA != NULL);
	if (indigo_guider_attach(device, DRIVER_NAME, DRIVER_VERSION) == INDIGO_OK) {
		DEVICE_PORT_PROPERTY->hidden = false;
		DEVICE_PORTS_PROPERTY->hidden = false;
		GUIDER_RATE_PROPERTY->hidden = true;
		INDIGO_DEVICE_ATTACH_LOG(DRIVER_NAME, device->name);
		return indigo_guider_enumerate_properties(device, NULL, NULL);
	}
	return INDIGO_FAILED;
}

static void guider_connection_handler(indigo_device *device) {
	pthread_mutex_lock(&PRIVATE_DATA->mutex);
	if (CONNECTION_CONNECTED_ITEM->sw.value) {
		CONNECTION_PROPERTY->state = INDIGO_BUSY_STATE;
		indigo_update_property(device, CONNECTION_PROPERTY, NULL);
		if (sx_open(device)) {
			CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
		} else {
			CONNECTION_PROPERTY->state = INDIGO_ALERT_STATE;
			indigo_set_switch(CONNECTION_PROPERTY, CONNECTION_DISCONNECTED_ITEM, true);
		}
	} else {
		sx_close(device);
		CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
	}
	indigo_guider_change_property(device, NULL, CONNECTION_PROPERTY);
	pthread_mutex_unlock(&PRIVATE_DATA->mutex);
}

static void guider_guide_dec_handler(indigo_device *device) {
	char response[2], command[16];
	pthread_mutex_lock(&PRIVATE_DATA->mutex);
	if (GUIDER_GUIDE_NORTH_ITEM->number.value > 0) {
		sprintf(command, "MN%05d", (int)GUIDER_GUIDE_NORTH_ITEM->number.value / 10);
		sx_command(device, command, response, 1);
	} else if (GUIDER_GUIDE_SOUTH_ITEM->number.value > 0) {
		sprintf(command, "MS%05d", (int)GUIDER_GUIDE_SOUTH_ITEM->number.value / 10);
		sx_command(device, command, response, 1);
	}
	GUIDER_GUIDE_NORTH_ITEM->number.value = GUIDER_GUIDE_SOUTH_ITEM->number.value = 0;
	GUIDER_GUIDE_DEC_PROPERTY->state = *response == 'M' ? INDIGO_OK_STATE : INDIGO_ALERT_STATE;
	indigo_update_property(device, GUIDER_GUIDE_DEC_PROPERTY, NULL);
	pthread_mutex_unlock(&PRIVATE_DATA->mutex);
}

static void guider_guide_ra_handler(indigo_device *device) {
	char response[2], command[16];
	pthread_mutex_lock(&PRIVATE_DATA->mutex);
	if (GUIDER_GUIDE_WEST_ITEM->number.value > 0) {
		sprintf(command, "MW%05d", (int)GUIDER_GUIDE_WEST_ITEM->number.value / 10);
		sx_command(device, command, response, 1);
	} else if (GUIDER_GUIDE_EAST_ITEM->number.value > 0) {
		sprintf(command, "MT%05d", (int)GUIDER_GUIDE_EAST_ITEM->number.value / 10);
		sx_command(device, command, response, 1);
	}
	GUIDER_GUIDE_WEST_ITEM->number.value = GUIDER_GUIDE_EAST_ITEM->number.value = 0;
	GUIDER_GUIDE_RA_PROPERTY->state = *response == 'M' ? INDIGO_OK_STATE : INDIGO_ALERT_STATE;
	indigo_update_property(device, GUIDER_GUIDE_RA_PROPERTY, NULL);
	pthread_mutex_unlock(&PRIVATE_DATA->mutex);
}

static indigo_result guider_change_property(indigo_device *device, indigo_client *client, indigo_property *property) {
	assert(device != NULL);
	assert(DEVICE_CONTEXT != NULL);
	assert(property != NULL);
	if (indigo_property_match(CONNECTION_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- CONNECTION
		indigo_property_copy_values(CONNECTION_PROPERTY, property, false);
		indigo_set_timer(device, 0, guider_connection_handler, NULL);
		return INDIGO_OK;
	} else if (indigo_property_match(GUIDER_GUIDE_DEC_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- GUIDER_GUIDE_DEC
		indigo_property_copy_values(GUIDER_GUIDE_DEC_PROPERTY, property, false);
		indigo_set_timer(device, 0, guider_guide_dec_handler, NULL);
		return INDIGO_OK;
	} else if (indigo_property_match(GUIDER_GUIDE_RA_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- GUIDER_GUIDE_RA
		indigo_property_copy_values(GUIDER_GUIDE_RA_PROPERTY, property, false);
		indigo_set_timer(device, 0, guider_guide_ra_handler, NULL);
		return INDIGO_OK;
		// --------------------------------------------------------------------------------
	}
	return indigo_ao_change_property(device, client, property);
}

static indigo_result guider_detach(indigo_device *device) {
	assert(device != NULL);
	if (IS_CONNECTED) {
		indigo_set_switch(CONNECTION_PROPERTY, CONNECTION_DISCONNECTED_ITEM, true);
		guider_connection_handler(device);
	}
	INDIGO_DEVICE_DETACH_LOG(DRIVER_NAME, device->name);
	return indigo_guider_detach(device);
}

// -------------------------------------------------------------------------------- INDIGO driver implementation

static sx_private_data *private_data = NULL;
static indigo_device *ao = NULL;
static indigo_device *guider = NULL;

indigo_result indigo_ao_sx(indigo_driver_action action, indigo_driver_info *info) {
	static indigo_device ao_template = INDIGO_DEVICE_INITIALIZER(
		"SX AO",
		ao_attach,
		indigo_ao_enumerate_properties,
		ao_change_property,
		NULL,
		ao_detach
	);

	static indigo_device guider_template = INDIGO_DEVICE_INITIALIZER(
		"SX AO (guider)",
		guider_attach,
		indigo_guider_enumerate_properties,
		guider_change_property,
		NULL,
		guider_detach
	);

	static indigo_driver_action last_action = INDIGO_DRIVER_SHUTDOWN;

	SET_DRIVER_INFO(info, "StarlightXpress AO", __FUNCTION__, DRIVER_VERSION, false, last_action);

	if (action == last_action)
		return INDIGO_OK;

	switch (action) {
		case INDIGO_DRIVER_INIT:
			last_action = action;
			private_data = indigo_safe_malloc(sizeof(sx_private_data));
			ao = indigo_safe_malloc_copy(sizeof(indigo_device), &ao_template);
			ao->private_data = private_data;
			indigo_attach_device(ao);
			guider = indigo_safe_malloc_copy(sizeof(indigo_device), &guider_template);
			guider->private_data = private_data;
			indigo_attach_device(guider);
			break;

		case INDIGO_DRIVER_SHUTDOWN:
			VERIFY_NOT_CONNECTED(guider);
			VERIFY_NOT_CONNECTED(ao);
			last_action = action;
			if (guider != NULL) {
				indigo_detach_device(guider);
				free(guider);
				guider = NULL;
			}
			if (ao != NULL) {
				indigo_detach_device(ao);
				free(ao);
				ao = NULL;
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
