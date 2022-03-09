// Copyright (c) 2018 CloudMakers, s. r. o.
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

/** INDIGO RTS on COM aux driver
 \file indigo_aux_rts.c
 */

#define DRIVER_VERSION 0x0006
#define DRIVER_NAME "indigo_aux_rts"

#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <math.h>
#include <assert.h>
#include <pthread.h>
#include <termios.h>
#include <sys/time.h>
#include <sys/ioctl.h>

#include <indigo/indigo_driver_xml.h>
#include <indigo/indigo_io.h>
#include "indigo_aux_rts.h"

#define PRIVATE_DATA												((rts_private_data *)device->private_data)

#define X_CCD_EXPOSURE_PROPERTY							(PRIVATE_DATA->exposure_property)
#define X_CCD_EXPOSURE_ITEM									(X_CCD_EXPOSURE_PROPERTY->items+0)

#define X_CCD_ABORT_EXPOSURE_PROPERTY				(PRIVATE_DATA->abort_exposure_property)
#define X_CCD_ABORT_EXPOSURE_ITEM						(X_CCD_ABORT_EXPOSURE_PROPERTY->items+0)

typedef struct {
	int handle;
	indigo_property *exposure_property;
	indigo_property *abort_exposure_property;
	indigo_timer *timer_callback;
	pthread_mutex_t mutex;
} rts_private_data;

// -------------------------------------------------------------------------------- Low level communication routines

static int rts_flag = TIOCM_RTS;

static void rts_on(indigo_device *device) {
	INDIGO_DRIVER_DEBUG(DRIVER_NAME, "RTS on");
	ioctl(PRIVATE_DATA->handle, TIOCMBIS, &rts_flag);
}

static void rts_off(indigo_device *device) {
	INDIGO_DRIVER_DEBUG(DRIVER_NAME, "RTS off");
	ioctl(PRIVATE_DATA->handle, TIOCMBIC, &rts_flag);
}

// -------------------------------------------------------------------------------- INDIGO aux device implementation

static indigo_result aux_attach(indigo_device *device) {
	assert(device != NULL);
	assert(PRIVATE_DATA != NULL);
	if (indigo_aux_attach(device, DRIVER_NAME, DRIVER_VERSION, INDIGO_INTERFACE_AUX_SHUTTER) == INDIGO_OK) {
		// -------------------------------------------------------------------------------- X_CCD_EXPOSURE
		X_CCD_EXPOSURE_PROPERTY = indigo_init_number_property(NULL, device->name, CCD_EXPOSURE_PROPERTY_NAME, AUX_MAIN_GROUP, "Start exposure", INDIGO_OK_STATE, INDIGO_RW_PERM, 1);
		if (X_CCD_EXPOSURE_PROPERTY == NULL)
			return INDIGO_FAILED;
		indigo_init_number_item(X_CCD_EXPOSURE_ITEM, CCD_EXPOSURE_ITEM_NAME, "Start exposure", 0, 10000, 1, 0);
		strcpy(X_CCD_EXPOSURE_ITEM->number.format, "%g");
		// -------------------------------------------------------------------------------- X_CCD_ABORT_EXPOSURE
		X_CCD_ABORT_EXPOSURE_PROPERTY = indigo_init_switch_property(NULL, device->name, CCD_ABORT_EXPOSURE_PROPERTY_NAME, AUX_MAIN_GROUP, "Abort exposure", INDIGO_OK_STATE, INDIGO_RW_PERM, INDIGO_AT_MOST_ONE_RULE, 1);
		if (X_CCD_ABORT_EXPOSURE_PROPERTY == NULL)
			return INDIGO_FAILED;
		indigo_init_switch_item(X_CCD_ABORT_EXPOSURE_ITEM, CCD_ABORT_EXPOSURE_ITEM_NAME, "Abort exposure", false);
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
		strcpy(DEVICE_PORT_ITEM->text.value, "/dev/usb_aux");
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
		if (indigo_property_match(X_CCD_EXPOSURE_PROPERTY, property))
			indigo_define_property(device, X_CCD_EXPOSURE_PROPERTY, NULL);
		if (indigo_property_match(X_CCD_ABORT_EXPOSURE_PROPERTY, property))
			indigo_define_property(device, X_CCD_ABORT_EXPOSURE_PROPERTY, NULL);
	}
	return indigo_aux_enumerate_properties(device, NULL, NULL);
}

static void aux_timer_callback(indigo_device *device) {
	if (!IS_CONNECTED)
		return;
	pthread_mutex_lock(&PRIVATE_DATA->mutex);
	if (X_CCD_EXPOSURE_PROPERTY->state == INDIGO_BUSY_STATE) {
		X_CCD_EXPOSURE_ITEM->number.value--;
		if (X_CCD_EXPOSURE_ITEM->number.value <= 0) {
			X_CCD_EXPOSURE_ITEM->number.value = 0;
			X_CCD_EXPOSURE_PROPERTY->state = INDIGO_OK_STATE;
			rts_off(device);
		}
		indigo_update_property(device, X_CCD_EXPOSURE_PROPERTY, NULL);
		if (X_CCD_EXPOSURE_ITEM->number.value > 0)
			indigo_reschedule_timer(device, X_CCD_EXPOSURE_ITEM->number.value < 1 ? X_CCD_EXPOSURE_ITEM->number.value : 1, &PRIVATE_DATA->timer_callback);
	}
	pthread_mutex_unlock(&PRIVATE_DATA->mutex);
}

static void aux_connection_handler(indigo_device *device) {
	pthread_mutex_lock(&PRIVATE_DATA->mutex);
	if (CONNECTION_CONNECTED_ITEM->sw.value) {
		PRIVATE_DATA->handle = indigo_open_serial(DEVICE_PORT_ITEM->text.value);
		if (PRIVATE_DATA->handle > 0) {
			INDIGO_DRIVER_LOG(DRIVER_NAME, "Connected on %s", DEVICE_PORT_ITEM->text.value);
			indigo_define_property(device, X_CCD_EXPOSURE_PROPERTY, NULL);
			indigo_define_property(device, X_CCD_ABORT_EXPOSURE_PROPERTY, NULL);
			CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
		} else {
			INDIGO_DRIVER_ERROR(DRIVER_NAME, "Failed to connect to %s", DEVICE_PORT_ITEM->text.value);
			CONNECTION_PROPERTY->state = INDIGO_ALERT_STATE;
			indigo_set_switch(CONNECTION_PROPERTY, CONNECTION_DISCONNECTED_ITEM, true);
		}
	} else {
		indigo_cancel_timer_sync(device, &PRIVATE_DATA->timer_callback);
		rts_off(device);
		indigo_delete_property(device, X_CCD_EXPOSURE_PROPERTY, NULL);
		indigo_delete_property(device, X_CCD_ABORT_EXPOSURE_PROPERTY, NULL);
		close(PRIVATE_DATA->handle);
		PRIVATE_DATA->handle = 0;
		INDIGO_DRIVER_LOG(DRIVER_NAME, "Disconnected");
		CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
	}
	indigo_aux_change_property(device, NULL, CONNECTION_PROPERTY);
	pthread_mutex_unlock(&PRIVATE_DATA->mutex);
}

static void aux_exposure_handler(indigo_device *device) {
	pthread_mutex_lock(&PRIVATE_DATA->mutex);
	rts_on(device);
	indigo_set_timer(device, X_CCD_EXPOSURE_ITEM->number.value < 1 ? X_CCD_EXPOSURE_ITEM->number.value : 1, aux_timer_callback, &PRIVATE_DATA->timer_callback);
	indigo_update_property(device, X_CCD_EXPOSURE_PROPERTY, NULL);
	pthread_mutex_unlock(&PRIVATE_DATA->mutex);
}

static void aux_abort_handler(indigo_device *device) {
	pthread_mutex_lock(&PRIVATE_DATA->mutex);
	if (X_CCD_ABORT_EXPOSURE_ITEM->sw.value && X_CCD_EXPOSURE_PROPERTY->state == INDIGO_BUSY_STATE) {
		indigo_cancel_timer(device, &PRIVATE_DATA->timer_callback);
		rts_off(device);
		X_CCD_EXPOSURE_PROPERTY->state = INDIGO_ALERT_STATE;
		indigo_update_property(device, X_CCD_EXPOSURE_PROPERTY, NULL);
	}
	X_CCD_ABORT_EXPOSURE_ITEM->sw.value = false;
	X_CCD_ABORT_EXPOSURE_PROPERTY->state = INDIGO_OK_STATE;
	indigo_update_property(device, X_CCD_ABORT_EXPOSURE_PROPERTY, NULL);
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
		// -------------------------------------------------------------------------------- X_CCD_EXPOSURE
	} else if (indigo_property_match(X_CCD_EXPOSURE_PROPERTY, property)) {
		indigo_property_copy_values(X_CCD_EXPOSURE_PROPERTY, property, false);
		X_CCD_EXPOSURE_PROPERTY->state = INDIGO_BUSY_STATE;
		indigo_update_property(device, X_CCD_EXPOSURE_PROPERTY, NULL);
		indigo_set_timer(device, 0, aux_exposure_handler, NULL);
		return INDIGO_OK;
		// -------------------------------------------------------------------------------- X_CCD_ABORT_EXPOSURE
	} else if (indigo_property_match(X_CCD_ABORT_EXPOSURE_PROPERTY, property)) {
		indigo_property_copy_values(X_CCD_ABORT_EXPOSURE_PROPERTY, property, false);
		X_CCD_ABORT_EXPOSURE_PROPERTY->state = INDIGO_BUSY_STATE;
		indigo_update_property(device, X_CCD_ABORT_EXPOSURE_PROPERTY, NULL);
		indigo_set_timer(device, 0, aux_abort_handler, NULL);
		return INDIGO_OK;
		// --------------------------------------------------------------------------------
	}
	return indigo_aux_change_property(device, client, property);
}

static indigo_result aux_detach(indigo_device *device) {
	assert(device != NULL);
	if (IS_CONNECTED) {
		indigo_set_switch(CONNECTION_PROPERTY, CONNECTION_DISCONNECTED_ITEM, true);
		aux_connection_handler(device);
	}
	indigo_release_property(X_CCD_EXPOSURE_PROPERTY);
	indigo_release_property(X_CCD_ABORT_EXPOSURE_PROPERTY);
	pthread_mutex_destroy(&PRIVATE_DATA->mutex);
	INDIGO_DEVICE_DETACH_LOG(DRIVER_NAME, device->name);
	return indigo_aux_detach(device);
}

// -------------------------------------------------------------------------------- INDIGO driver implementation

indigo_result indigo_aux_rts(indigo_driver_action action, indigo_driver_info *info) {
	static indigo_driver_action last_action = INDIGO_DRIVER_SHUTDOWN;
	static rts_private_data *private_data = NULL;
	static indigo_device *aux = NULL;
	
	static indigo_device aux_template = INDIGO_DEVICE_INITIALIZER(
		"RTS-on-COM",
		aux_attach,
		aux_enumerate_properties,
		aux_change_property,
		NULL,
		aux_detach
		);
	
	SET_DRIVER_INFO(info, "RTS-on-COM shutter", __FUNCTION__, DRIVER_VERSION, false, last_action);
	
	if (action == last_action)
		return INDIGO_OK;
	
	switch (action) {
		case INDIGO_DRIVER_INIT:
			last_action = action;
			private_data = indigo_safe_malloc(sizeof(rts_private_data));
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
