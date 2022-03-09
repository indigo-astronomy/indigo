// Copyright (c) 2019 Rumen G. Bogdanovski
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
// 2.0 by Rumen G. Bogdanovski <rumen@skyarchive.org>

/** INDIGO Lacerta FBC aux driver
 \file indigo_aux_fbc.c
 */

#define DRIVER_VERSION 0x0004
#define DRIVER_NAME "indigo_aux_fbc"

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
#include "indigo_aux_fbc.h"

// gp_bits is used as boolean
#define is_connected               gp_bits

#define PRIVATE_DATA                                          ((fbc_private_data *)device->private_data)

#define AUX_LIGHT_SWITCH_PROPERTY                             (PRIVATE_DATA->light_switch_property)
#define AUX_LIGHT_SWITCH_ON_ITEM                              (AUX_LIGHT_SWITCH_PROPERTY->items+0)
#define AUX_LIGHT_SWITCH_OFF_ITEM                             (AUX_LIGHT_SWITCH_PROPERTY->items+1)

#define AUX_LIGHT_INTENSITY_PROPERTY                          (PRIVATE_DATA->light_intensity_property)
#define AUX_LIGHT_INTENSITY_ITEM                              (AUX_LIGHT_INTENSITY_PROPERTY->items+0)

#define AUX_LIGHT_IMPULSE_PROPERTY                            (PRIVATE_DATA->light_impulse_property)
#define AUX_LIGHT_IMPULSE_DURATION_ITEM                       (AUX_LIGHT_IMPULSE_PROPERTY->items+0)

#define CCD_EXPOSURE_PROPERTY                                 (PRIVATE_DATA->exposure_property)
#define CCD_EXPOSURE_ITEM                                     (CCD_EXPOSURE_PROPERTY->items+0)

typedef struct {
	int handle;
	indigo_timer *exposure_timer, *illumination_timer;
	indigo_property *light_switch_property;
	indigo_property *light_intensity_property;
	indigo_property *light_impulse_property;
	indigo_property *exposure_property;
	pthread_mutex_t mutex;
} fbc_private_data;

static bool fbc_command(int handle, char *command, char *response, int resp_len) {
	if(response) {
		indigo_usleep(20000);
		tcflush(handle, TCIOFLUSH);
	}

	int result = indigo_write(handle, command, strlen(command));
	INDIGO_DRIVER_DEBUG(DRIVER_NAME, "%d <- %s (%s)", handle, command, result ? "OK" : strerror(errno));

	if ((result) && (response)) {
		READ_AGAIN:
		*response = 0;
		result = indigo_read_line(handle, response, resp_len) > 0;
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "%d -> %s (%s)", handle, response, result ? "OK" : strerror(errno));
		if ((result) && (!strncmp("D -", response, 3))) goto READ_AGAIN;
	}
	return result;
}

static void aux_intensity_handler(indigo_device *device);
static void aux_switch_handler(indigo_device *device);


static void ccd_exposure_callback(indigo_device *device) {
	if (!IS_CONNECTED) return;
	CCD_EXPOSURE_ITEM->number.value -= 1;
	if (CCD_EXPOSURE_ITEM->number.value > 1) {
		indigo_reschedule_timer(device, 1, &PRIVATE_DATA->exposure_timer);
	} else if (CCD_EXPOSURE_ITEM->number.value >= 0) {
		indigo_reschedule_timer(device, CCD_EXPOSURE_ITEM->number.value, &PRIVATE_DATA->exposure_timer);
	} else {
		CCD_EXPOSURE_ITEM->number.value = 0;
		CCD_EXPOSURE_PROPERTY->state = INDIGO_OK_STATE;
	}
	indigo_update_property(device, CCD_EXPOSURE_PROPERTY, NULL);
}


static void light_impulse_callback(indigo_device *device) {
	if (!IS_CONNECTED) return;
	AUX_LIGHT_IMPULSE_DURATION_ITEM->number.value -= 1;
	if (AUX_LIGHT_IMPULSE_DURATION_ITEM->number.value > 1) {
		indigo_reschedule_timer(device, 1, &PRIVATE_DATA->illumination_timer);
	} else if (AUX_LIGHT_IMPULSE_DURATION_ITEM->number.value >= 0) {
		indigo_reschedule_timer(device, AUX_LIGHT_IMPULSE_DURATION_ITEM->number.value, &PRIVATE_DATA->illumination_timer);
	} else {
		AUX_LIGHT_IMPULSE_DURATION_ITEM->number.value = 0;
		AUX_LIGHT_IMPULSE_PROPERTY->state = INDIGO_OK_STATE;
	}
	indigo_update_property(device, AUX_LIGHT_IMPULSE_PROPERTY, NULL);
}

// -------------------------------------------------------------------------------- INDIGO aux device implementation

static indigo_result aux_attach(indigo_device *device) {
	assert(device != NULL);
	assert(PRIVATE_DATA != NULL);
	if (indigo_aux_attach(device, DRIVER_NAME, DRIVER_VERSION, INDIGO_INTERFACE_AUX_LIGHTBOX | INDIGO_INTERFACE_AUX_SHUTTER) == INDIGO_OK) {
		INFO_PROPERTY->count = 6;
		// -------------------------------------------------------------------------------- AUX_LIGHT_SWITCH
		AUX_LIGHT_SWITCH_PROPERTY = indigo_init_switch_property(NULL, device->name, AUX_LIGHT_SWITCH_PROPERTY_NAME, AUX_MAIN_GROUP, "Light (on/off)", INDIGO_OK_STATE, INDIGO_RW_PERM, INDIGO_ONE_OF_MANY_RULE, 2);
		if (AUX_LIGHT_SWITCH_PROPERTY == NULL)
			return INDIGO_FAILED;
		AUX_LIGHT_SWITCH_PROPERTY->hidden = true; /* it can not stay on forever */
		indigo_init_switch_item(AUX_LIGHT_SWITCH_ON_ITEM, AUX_LIGHT_SWITCH_ON_ITEM_NAME, "On", false);
		indigo_init_switch_item(AUX_LIGHT_SWITCH_OFF_ITEM, AUX_LIGHT_SWITCH_OFF_ITEM_NAME, "Off", true);
		// -------------------------------------------------------------------------------- AUX_LIGHT_INTENSITY
		AUX_LIGHT_INTENSITY_PROPERTY = indigo_init_number_property(NULL, device->name, AUX_LIGHT_INTENSITY_PROPERTY_NAME, AUX_MAIN_GROUP, "Light intensity", INDIGO_OK_STATE, INDIGO_RW_PERM, 1);
		if (AUX_LIGHT_INTENSITY_PROPERTY == NULL)
			return INDIGO_FAILED;
		indigo_init_number_item(AUX_LIGHT_INTENSITY_ITEM, AUX_LIGHT_INTENSITY_ITEM_NAME, "Intensity (%)", 0, 100, 1, 50);
		strcpy(AUX_LIGHT_INTENSITY_ITEM->number.format, "%g");
		// -------------------------------------------------------------------------------- AUX_LIGHT_IMPULSE
		AUX_LIGHT_IMPULSE_PROPERTY = indigo_init_number_property(NULL, device->name, AUX_LIGHT_IMPULSE_PROPERTY_NAME, AUX_MAIN_GROUP, "Light impulse", INDIGO_OK_STATE, INDIGO_RW_PERM, 1);
		if (AUX_LIGHT_IMPULSE_PROPERTY == NULL)
			return INDIGO_FAILED;
		indigo_init_number_item(AUX_LIGHT_IMPULSE_DURATION_ITEM, AUX_LIGHT_IMPULSE_DURATION_ITEM_NAME, "Duration (s)", 0, 30, 1, 0);
		// -------------------------------------------------------------------------------- CCD_EXPOSURE_PROPERTY
		CCD_EXPOSURE_PROPERTY = indigo_init_number_property(NULL, device->name, CCD_EXPOSURE_PROPERTY_NAME, AUX_MAIN_GROUP, "Shutter Control", INDIGO_OK_STATE, INDIGO_RW_PERM, 1);
		if (CCD_EXPOSURE_PROPERTY == NULL)
			return INDIGO_FAILED;
		indigo_init_number_item(CCD_EXPOSURE_ITEM, CCD_EXPOSURE_ITEM_NAME, "Exposure (s)", 0, 30, 1, 0);
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
		strcpy(DEVICE_PORT_ITEM->text.value, "/dev/lacertaFBC");
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
		if (indigo_property_match(AUX_LIGHT_INTENSITY_PROPERTY, property))
			indigo_define_property(device, AUX_LIGHT_INTENSITY_PROPERTY, NULL);
		if (indigo_property_match(AUX_LIGHT_SWITCH_PROPERTY, property))
			indigo_define_property(device, AUX_LIGHT_SWITCH_PROPERTY, NULL);
		if (indigo_property_match(AUX_LIGHT_IMPULSE_PROPERTY, property))
			indigo_define_property(device, AUX_LIGHT_IMPULSE_PROPERTY, NULL);
		if (indigo_property_match(CCD_EXPOSURE_PROPERTY, property))
			indigo_define_property(device, CCD_EXPOSURE_PROPERTY, NULL);
	}
	return indigo_aux_enumerate_properties(device, NULL, NULL);
}


static void aux_connection_handler(indigo_device *device) {
	char command[160], response[160];
	pthread_mutex_lock(&PRIVATE_DATA->mutex);
	if (CONNECTION_CONNECTED_ITEM->sw.value) {
		for (int i = 0; i < 2; i++) {
			PRIVATE_DATA->handle = indigo_open_serial(DEVICE_PORT_ITEM->text.value);
			if (PRIVATE_DATA->handle > 0) {
				int fc_flag;
				fc_flag = TIOCM_RTS;   /* Modem Constant for RTS pin */
				ioctl(PRIVATE_DATA->handle,TIOCMBIC,&fc_flag);
				fc_flag = TIOCM_CTS;   /* Modem Constant for CTS pin */
				ioctl(PRIVATE_DATA->handle,TIOCMBIC,&fc_flag);

				INDIGO_DRIVER_LOG(DRIVER_NAME, "Connected on %s", DEVICE_PORT_ITEM->text.value);
				if (fbc_command(PRIVATE_DATA->handle, ": I #", response, sizeof(response)) && !strcmp("I FBC", response)) {
					if (fbc_command(PRIVATE_DATA->handle, ": P #", response, sizeof(response))) {
						if (strcmp("P SerialMode", response)) {
							INDIGO_DRIVER_ERROR(DRIVER_NAME, "FBC is not in SerialMode. Turn all knobs to 0 and powercycle the device.");
							indigo_send_message(device, "FBC is not in SerialMode. Turn all knobs to 0 and powercycle the device.");
							close(PRIVATE_DATA->handle);
							PRIVATE_DATA->handle = 0;
							break;
						}
					}
				} else {
					INDIGO_DRIVER_ERROR(DRIVER_NAME, "Handshake failed");
					close(PRIVATE_DATA->handle);
					PRIVATE_DATA->handle = 0;
				}
			}
		}
		if (PRIVATE_DATA->handle > 0) {
			if (fbc_command(PRIVATE_DATA->handle, ": V #", response, sizeof(response))) {
				sscanf(response, "V %s", INFO_DEVICE_FW_REVISION_ITEM->text.value);
				indigo_update_property(device, INFO_PROPERTY, NULL);
			}

			/*
			sprintf(command, ": E 15000 #");
			fbc_command(PRIVATE_DATA->handle, command, NULL, 0);

			sprintf(command, ": F 15000 #");
			fbc_command(PRIVATE_DATA->handle, command, NULL, 0);
			*/

			/* Stop ilumination and exposure */
			fbc_command(PRIVATE_DATA->handle, ": E 0 #", NULL, 0);
			fbc_command(PRIVATE_DATA->handle, ": F 0 #", NULL, 0);
			sprintf(command, ": B %d #", (int)AUX_LIGHT_INTENSITY_ITEM->number.value);
			fbc_command(PRIVATE_DATA->handle, command, NULL, 0);

			indigo_define_property(device, AUX_LIGHT_IMPULSE_PROPERTY, NULL);
			indigo_define_property(device, CCD_EXPOSURE_PROPERTY, NULL);
			indigo_define_property(device, AUX_LIGHT_INTENSITY_PROPERTY, NULL);
			indigo_define_property(device, AUX_LIGHT_SWITCH_PROPERTY, NULL);
			CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
		} else {
			INDIGO_DRIVER_ERROR(DRIVER_NAME, "Failed to connect to %s", DEVICE_PORT_ITEM->text.value);
			CONNECTION_PROPERTY->state = INDIGO_ALERT_STATE;
			indigo_set_switch(CONNECTION_PROPERTY, CONNECTION_DISCONNECTED_ITEM, true);
		}
	} else {
		indigo_delete_property(device, AUX_LIGHT_IMPULSE_PROPERTY, NULL);
		indigo_delete_property(device, CCD_EXPOSURE_PROPERTY, NULL);
		indigo_delete_property(device, AUX_LIGHT_INTENSITY_PROPERTY, NULL);
		indigo_delete_property(device, AUX_LIGHT_SWITCH_PROPERTY, NULL);
		// turn off fbc at disconnecect - stop ilumination and exposure */
		fbc_command(PRIVATE_DATA->handle, ": E 0 #", NULL, 0);
		fbc_command(PRIVATE_DATA->handle, ": F 0 #", NULL, 0);

		close(PRIVATE_DATA->handle);
		PRIVATE_DATA->handle = 0;
		INDIGO_DRIVER_LOG(DRIVER_NAME, "Disconnected");
		CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
	}
	indigo_aux_change_property(device, NULL, CONNECTION_PROPERTY);
	pthread_mutex_unlock(&PRIVATE_DATA->mutex);
}


static void aux_intensity_handler(indigo_device *device) {
	pthread_mutex_lock(&PRIVATE_DATA->mutex);
	char command[16];
	sprintf(command, ": B %d #", (int)AUX_LIGHT_INTENSITY_ITEM->number.value);
	if (fbc_command(PRIVATE_DATA->handle, command, NULL, 0))
		AUX_LIGHT_INTENSITY_PROPERTY->state = INDIGO_OK_STATE;
	else
		AUX_LIGHT_INTENSITY_PROPERTY->state = INDIGO_ALERT_STATE;
	indigo_update_property(device, AUX_LIGHT_INTENSITY_PROPERTY, NULL);
	pthread_mutex_unlock(&PRIVATE_DATA->mutex);
}


static void aux_impulse_handler(indigo_device *device) {
	pthread_mutex_lock(&PRIVATE_DATA->mutex);
	char command[16];
	sprintf(command, ": F %d #", (int)(AUX_LIGHT_IMPULSE_DURATION_ITEM->number.value * 1000));
	if (fbc_command(PRIVATE_DATA->handle, command, NULL, 0)) {
		AUX_LIGHT_IMPULSE_PROPERTY->state = INDIGO_BUSY_STATE;
		double delay = AUX_LIGHT_IMPULSE_DURATION_ITEM->number.value;
		if (AUX_LIGHT_IMPULSE_DURATION_ITEM->number.value > 1)
			delay = 1;
		indigo_set_timer(device, delay, light_impulse_callback, &PRIVATE_DATA->illumination_timer);
	} else {
		AUX_LIGHT_IMPULSE_PROPERTY->state = INDIGO_ALERT_STATE;
	}
	indigo_update_property(device, AUX_LIGHT_IMPULSE_PROPERTY, NULL);
	pthread_mutex_unlock(&PRIVATE_DATA->mutex);
}


static void aux_shutter_handler(indigo_device *device) {
	pthread_mutex_lock(&PRIVATE_DATA->mutex);
	char command[16];
	sprintf(command, ": E %d #", (int)(CCD_EXPOSURE_ITEM->number.value * 1000));
	if (fbc_command(PRIVATE_DATA->handle, command, NULL, 0)) {
		CCD_EXPOSURE_PROPERTY->state = INDIGO_BUSY_STATE;
		double delay = CCD_EXPOSURE_ITEM->number.value;
		if (CCD_EXPOSURE_ITEM->number.value > 1)
			delay = 1;
		indigo_set_timer(device, delay, ccd_exposure_callback, &PRIVATE_DATA->exposure_timer);
	} else {
		CCD_EXPOSURE_PROPERTY->state = INDIGO_ALERT_STATE;
	}
	indigo_update_property(device, CCD_EXPOSURE_PROPERTY, NULL);
	pthread_mutex_unlock(&PRIVATE_DATA->mutex);
}


static void aux_switch_handler(indigo_device *device) {
	pthread_mutex_lock(&PRIVATE_DATA->mutex);
	char command[16],	response[16];
	sprintf(command, "E:%d", AUX_LIGHT_SWITCH_ON_ITEM->sw.value);
	if (fbc_command(PRIVATE_DATA->handle, command, response, sizeof(response)))
		AUX_LIGHT_SWITCH_PROPERTY->state = INDIGO_OK_STATE;
	else
		AUX_LIGHT_SWITCH_PROPERTY->state = INDIGO_ALERT_STATE;
	indigo_update_property(device, AUX_LIGHT_SWITCH_PROPERTY, NULL);
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
		// -------------------------------------------------------------------------------- AUX_LIGHT_IMPULSE
	} else if (indigo_property_match(AUX_LIGHT_IMPULSE_PROPERTY, property)) {
		indigo_property_copy_values(AUX_LIGHT_IMPULSE_PROPERTY, property, false);
		if (IS_CONNECTED) {
			AUX_LIGHT_IMPULSE_PROPERTY->state = INDIGO_BUSY_STATE;
			indigo_update_property(device, AUX_LIGHT_IMPULSE_PROPERTY, NULL);
			indigo_set_timer(device, 0, aux_impulse_handler, NULL);
		}
		return INDIGO_OK;
		// -------------------------------------------------------------------------------- CCD_EXPOSURE
	} else if (indigo_property_match(CCD_EXPOSURE_PROPERTY, property)) {
		indigo_property_copy_values(CCD_EXPOSURE_PROPERTY, property, false);
		if (IS_CONNECTED) {
			CCD_EXPOSURE_PROPERTY->state = INDIGO_BUSY_STATE;
			indigo_update_property(device, CCD_EXPOSURE_PROPERTY, NULL);
			indigo_set_timer(device, 0, aux_shutter_handler, NULL);
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
		// -------------------------------------------------------------------------------- CONFIG
	} else if (indigo_property_match(CONFIG_PROPERTY, property)) {
		if (indigo_switch_match(CONFIG_SAVE_ITEM, property)) {
			indigo_save_property(device, NULL, AUX_LIGHT_INTENSITY_PROPERTY);
			//indigo_save_property(device, NULL, AUX_LIGHT_SWITCH_PROPERTY);
		}
	}
	return indigo_aux_change_property(device, client, property);
}

static indigo_result aux_detach(indigo_device *device) {
	assert(device != NULL);
	if (IS_CONNECTED) {
		indigo_set_switch(CONNECTION_PROPERTY, CONNECTION_DISCONNECTED_ITEM, true);
		aux_connection_handler(device);
	}
	indigo_release_property(AUX_LIGHT_IMPULSE_PROPERTY);
	indigo_release_property(CCD_EXPOSURE_PROPERTY);
	indigo_release_property(AUX_LIGHT_INTENSITY_PROPERTY);
	indigo_release_property(AUX_LIGHT_SWITCH_PROPERTY);
	pthread_mutex_destroy(&PRIVATE_DATA->mutex);
	INDIGO_DEVICE_DETACH_LOG(DRIVER_NAME, device->name);
	return indigo_aux_detach(device);
}

// -------------------------------------------------------------------------------- INDIGO driver implementation

indigo_result indigo_aux_fbc(indigo_driver_action action, indigo_driver_info *info) {
	static indigo_driver_action last_action = INDIGO_DRIVER_SHUTDOWN;
	static fbc_private_data *private_data = NULL;
	static indigo_device *aux = NULL;

	static indigo_device aux_template = INDIGO_DEVICE_INITIALIZER(
		"Lacerta FBC",
		aux_attach,
		aux_enumerate_properties,
		aux_change_property,
		NULL,
		aux_detach
		);

	SET_DRIVER_INFO(info, "Lacerta FBC", __FUNCTION__, DRIVER_VERSION, false, last_action);

	if (action == last_action)
		return INDIGO_OK;

	switch (action) {
		case INDIGO_DRIVER_INIT:
			last_action = action;
			private_data = indigo_safe_malloc(sizeof(fbc_private_data));
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
