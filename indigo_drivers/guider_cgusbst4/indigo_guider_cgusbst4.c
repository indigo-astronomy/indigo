// Copyright (c) 2016-2025 CloudMakers, s. r. o.
// All rights reserved.

// You can use this software under the terms of 'INDIGO Astronomy
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

// This file generated from indigo_guider_cgusbst4.driver

// version history
// 3.0 by Peter Polakovic

#pragma mark - Includes

#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <assert.h>
#include <pthread.h>
#include <indigo/indigo_driver_xml.h>
#include <indigo/indigo_guider_driver.h>
#include <indigo/indigo_uni_io.h>

#include "indigo_guider_cgusbst4.h"

#pragma mark - Common definitions

#define DRIVER_VERSION       0x03000005
#define DRIVER_NAME          "indigo_guider_cgusbst4"
#define DRIVER_LABEL         "CG-USB-ST4 Adapter"
#define GUIDER_DEVICE_NAME   "CG-USB-ST4 Adapter"
#define PRIVATE_DATA         ((cgusbst4_private_data *)device->private_data)

#pragma mark - Private data definition

typedef struct {
	pthread_mutex_t mutex;
	indigo_uni_handle *handle;
	indigo_timer *guider_connection_handler_timer;
	indigo_timer *guider_guide_dec_handler_timer;
	indigo_timer *guider_guide_ra_handler_timer;
} cgusbst4_private_data;

#pragma mark - Low level code

//+ code

static bool cgusbst4_command(indigo_device *device, char *command, char *response, int max) {
	if (indigo_uni_discard(PRIVATE_DATA->handle) >= 0) {
		if (indigo_uni_write(PRIVATE_DATA->handle, command, (long)strlen(command)) > 0) {
			if (response != NULL) {
				indigo_usleep(INDIGO_DELAY(1));
				if (indigo_uni_read_section(PRIVATE_DATA->handle, response, max, "#", "", INDIGO_DELAY(1) > 0)) {
					return true;
				}
			}
		}
	}
	return false;
}

static bool cgusbst4_open(indigo_device *device) {
	char *name = DEVICE_PORT_ITEM->text.value;
	PRIVATE_DATA->handle = indigo_uni_open_serial(name, INDIGO_LOG_DEBUG);
	if (PRIVATE_DATA->handle != NULL) {
		char response[2];
		if (cgusbst4_command(device, "\006", response, 1) && *response == 'A') {
			return true;
		}
		indigo_send_message(device, "Handshake failed");
		indigo_uni_close(&PRIVATE_DATA->handle);
	}
	return false;
}

static void cgusbst4_close(indigo_device *device) {
	if (PRIVATE_DATA->handle != NULL) {
		indigo_uni_close(&PRIVATE_DATA->handle);
		PRIVATE_DATA->handle = 0;
		INDIGO_DRIVER_LOG(DRIVER_NAME, "Disconnected from %s", DEVICE_PORT_ITEM->text.value);
	}
}

//- code

#pragma mark - High level code (guider)

// CONNECTION change handler

static void guider_connection_handler(indigo_device *device) {
	indigo_lock_master_device(device);
	if (CONNECTION_CONNECTED_ITEM->sw.value) {
		pthread_mutex_lock(&PRIVATE_DATA->mutex);
		bool connection_result = true;
		connection_result = cgusbst4_open(device);
		if (connection_result) {
			CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
			indigo_send_message(device, "Connected to %s on %s", GUIDER_DEVICE_NAME, DEVICE_PORT_ITEM->text.value);
		} else {
			indigo_send_message(device, "Failed to connect to %s on %s", GUIDER_DEVICE_NAME, DEVICE_PORT_ITEM->text.value);
			CONNECTION_PROPERTY->state = INDIGO_ALERT_STATE;
			indigo_set_switch(CONNECTION_PROPERTY, CONNECTION_DISCONNECTED_ITEM, true);
		}
		pthread_mutex_unlock(&PRIVATE_DATA->mutex);
	} else {
		indigo_cancel_timer_sync(device, &PRIVATE_DATA->guider_guide_dec_handler_timer);
		indigo_cancel_timer_sync(device, &PRIVATE_DATA->guider_guide_ra_handler_timer);
		cgusbst4_close(device);
		indigo_send_message(device, "Disconnected from %s", device->name);
		CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
	}
	indigo_guider_change_property(device, NULL, CONNECTION_PROPERTY);
	indigo_unlock_master_device(device);
}

// GUIDER_GUIDE_DEC change handler

static void guider_guide_dec_handler(indigo_device *device) {
	pthread_mutex_lock(&PRIVATE_DATA->mutex);
	GUIDER_GUIDE_DEC_PROPERTY->state = INDIGO_OK_STATE;
	//+ guider.GUIDER_GUIDE_DEC.on_change
	char command[16];
	int duration = GUIDER_GUIDE_NORTH_ITEM->number.value;
	if (duration > 0) {
		sprintf(command, ":Mgn%4d#", (int)GUIDER_GUIDE_NORTH_ITEM->number.value);
		cgusbst4_command(device, command, NULL, 0);
	} else {
		duration = GUIDER_GUIDE_SOUTH_ITEM->number.value;
		if (duration > 0) {
			sprintf(command, ":Mgs%4d#", (int)GUIDER_GUIDE_SOUTH_ITEM->number.value);
			cgusbst4_command(device, command, NULL, 0);
		}
	}
	if (duration > 0) {
		GUIDER_GUIDE_DEC_PROPERTY->state = INDIGO_BUSY_STATE;
		indigo_update_property(device, GUIDER_GUIDE_DEC_PROPERTY, NULL);
		pthread_mutex_unlock(&PRIVATE_DATA->mutex);
		indigo_usleep(duration * 1000.0);
		pthread_mutex_lock(&PRIVATE_DATA->mutex);
		GUIDER_GUIDE_NORTH_ITEM->number.value = GUIDER_GUIDE_SOUTH_ITEM->number.value = 0;
		GUIDER_GUIDE_DEC_PROPERTY->state = INDIGO_OK_STATE;
	}
	//- guider.GUIDER_GUIDE_DEC.on_change
	indigo_update_property(device, GUIDER_GUIDE_DEC_PROPERTY, NULL);
	pthread_mutex_unlock(&PRIVATE_DATA->mutex);
}

// GUIDER_GUIDE_RA change handler

static void guider_guide_ra_handler(indigo_device *device) {
	pthread_mutex_lock(&PRIVATE_DATA->mutex);
	GUIDER_GUIDE_RA_PROPERTY->state = INDIGO_OK_STATE;
	//+ guider.GUIDER_GUIDE_RA.on_change
	char command[128];
	int duration = GUIDER_GUIDE_EAST_ITEM->number.value;
	if (duration > 0) {
		sprintf(command, ":Mge%4d#", (int)GUIDER_GUIDE_EAST_ITEM->number.value);
		cgusbst4_command(device, command, NULL, 0);
	} else {
		duration = GUIDER_GUIDE_WEST_ITEM->number.value;
		if (duration > 0) {
			sprintf(command, ":Mgw%4d#", (int)GUIDER_GUIDE_WEST_ITEM->number.value);
			cgusbst4_command(device, command, NULL, 0);
		}
	}
	if (duration > 0) {
		GUIDER_GUIDE_RA_PROPERTY->state = INDIGO_BUSY_STATE;
		indigo_update_property(device, GUIDER_GUIDE_RA_PROPERTY, NULL);
		pthread_mutex_unlock(&PRIVATE_DATA->mutex);
		indigo_usleep(duration * 1000.0);
		pthread_mutex_lock(&PRIVATE_DATA->mutex);
		GUIDER_GUIDE_WEST_ITEM->number.value = GUIDER_GUIDE_WEST_ITEM->number.value = 0;
		GUIDER_GUIDE_RA_PROPERTY->state = INDIGO_OK_STATE;
	}
	//- guider.GUIDER_GUIDE_RA.on_change
	indigo_update_property(device, GUIDER_GUIDE_RA_PROPERTY, NULL);
	pthread_mutex_unlock(&PRIVATE_DATA->mutex);
}

#pragma mark - Device API (guider)

static indigo_result guider_enumerate_properties(indigo_device *device, indigo_client *client, indigo_property *property);

// guider attach API callback

static indigo_result guider_attach(indigo_device *device) {
	if (indigo_guider_attach(device, DRIVER_NAME, DRIVER_VERSION) == INDIGO_OK) {
		DEVICE_PORT_PROPERTY->hidden = false;
		DEVICE_PORTS_PROPERTY->hidden = false;
		indigo_enumerate_serial_ports(device, DEVICE_PORTS_PROPERTY);
		GUIDER_GUIDE_DEC_PROPERTY->hidden = false;
		GUIDER_GUIDE_RA_PROPERTY->hidden = false;
		INDIGO_DEVICE_ATTACH_LOG(DRIVER_NAME, device->name);
		pthread_mutex_init(&PRIVATE_DATA->mutex, NULL);
		return guider_enumerate_properties(device, NULL, NULL);
	}
	return INDIGO_FAILED;
}

// guider enumerate API callback

static indigo_result guider_enumerate_properties(indigo_device *device, indigo_client *client, indigo_property *property) {
	return indigo_guider_enumerate_properties(device, NULL, NULL);
}

// guider change property API callback

static indigo_result guider_change_property(indigo_device *device, indigo_client *client, indigo_property *property) {
	if (indigo_property_match_changeable(CONNECTION_PROPERTY, property)) {
		if (!indigo_ignore_connection_change(device, property)) {
			indigo_property_copy_values(CONNECTION_PROPERTY, property, false);
			CONNECTION_PROPERTY->state = INDIGO_BUSY_STATE;
			indigo_update_property(device, CONNECTION_PROPERTY, NULL);
			indigo_set_timer(device, 0, guider_connection_handler, &PRIVATE_DATA->guider_connection_handler_timer);
		}
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(GUIDER_GUIDE_DEC_PROPERTY, property)) {
		if (PRIVATE_DATA->guider_guide_dec_handler_timer == NULL) {
			indigo_property_copy_values(GUIDER_GUIDE_DEC_PROPERTY, property, false);
			GUIDER_GUIDE_DEC_PROPERTY->state = INDIGO_BUSY_STATE;
			indigo_update_property(device, GUIDER_GUIDE_DEC_PROPERTY, NULL);
			indigo_set_timer(device, 0, guider_guide_dec_handler, &PRIVATE_DATA->guider_guide_dec_handler_timer);
		}
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(GUIDER_GUIDE_RA_PROPERTY, property)) {
		if (PRIVATE_DATA->guider_guide_ra_handler_timer == NULL) {
			indigo_property_copy_values(GUIDER_GUIDE_RA_PROPERTY, property, false);
			GUIDER_GUIDE_RA_PROPERTY->state = INDIGO_BUSY_STATE;
			indigo_update_property(device, GUIDER_GUIDE_RA_PROPERTY, NULL);
			indigo_set_timer(device, 0, guider_guide_ra_handler, &PRIVATE_DATA->guider_guide_ra_handler_timer);
		}
		return INDIGO_OK;
	}
	return indigo_guider_change_property(device, client, property);
}

// guider detach API callback

static indigo_result guider_detach(indigo_device *device) {
	if (IS_CONNECTED) {
		indigo_set_switch(CONNECTION_PROPERTY, CONNECTION_DISCONNECTED_ITEM, true);
		guider_connection_handler(device);
	}
	INDIGO_DEVICE_DETACH_LOG(DRIVER_NAME, device->name);
	pthread_mutex_destroy(&PRIVATE_DATA->mutex);
	return indigo_guider_detach(device);
}

#pragma mark - Device templates

static indigo_device guider_template = INDIGO_DEVICE_INITIALIZER(GUIDER_DEVICE_NAME, guider_attach, guider_enumerate_properties, guider_change_property, NULL, guider_detach);

#pragma mark - Main code

// CG-USB-ST4 Adapter driver entry point

indigo_result indigo_guider_cgusbst4(indigo_driver_action action, indigo_driver_info *info) {
	static indigo_driver_action last_action = INDIGO_DRIVER_SHUTDOWN;
	static cgusbst4_private_data *private_data = NULL;
	static indigo_device *guider = NULL;

	SET_DRIVER_INFO(info, DRIVER_LABEL, __FUNCTION__, DRIVER_VERSION, false, last_action);

	if (action == last_action) {
		return INDIGO_OK;
	}

	switch (action) {
		case INDIGO_DRIVER_INIT:
			last_action = action;
			static indigo_device_match_pattern patterns[1] = { 0 };
			strcpy(patterns[0].product_string, "USB to ST4 Astrogene_1000");
			INDIGO_REGISER_MATCH_PATTERNS(guider_template, patterns, 1);
			private_data = indigo_safe_malloc(sizeof(cgusbst4_private_data));
			guider = indigo_safe_malloc_copy(sizeof(indigo_device), &guider_template);
			guider->private_data = private_data;
			indigo_attach_device(guider);
			break;

		case INDIGO_DRIVER_SHUTDOWN:
			VERIFY_NOT_CONNECTED(guider);
			last_action = action;
			if (guider != NULL) {
				indigo_detach_device(guider);
				free(guider);
				guider = NULL;
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
