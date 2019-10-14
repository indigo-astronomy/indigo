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

/** INDIGO USB_Dewpoint aux driver
 \file indigo_aux_usbdp.c
 */

#define DRIVER_VERSION 0x0001
#define DRIVER_NAME "indigo_aux_usbdp"

#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <math.h>
#include <assert.h>
#include <errno.h>
#include <pthread.h>
#include <stdarg.h>

#include <sys/time.h>
#include <sys/termios.h>

#include <indigo/indigo_driver_xml.h>
#include <indigo/indigo_io.h>

#include "indigo_aux_usbdp.h"

#define PRIVATE_DATA									((usbdp_private_data *)device->private_data)

#define AUX_OUTLET_NAMES_PROPERTY						(PRIVATE_DATA->outlet_names_property)
#define AUX_HEATER_OUTLET_NAME_1_ITEM				(AUX_OUTLET_NAMES_PROPERTY->items + 0)
#define AUX_HEATER_OUTLET_NAME_2_ITEM				(AUX_OUTLET_NAMES_PROPERTY->items + 1)
#define AUX_HEATER_OUTLET_NAME_3_ITEM				(AUX_OUTLET_NAMES_PROPERTY->items + 2)

#define AUX_HEATER_OUTLET_PROPERTY					(PRIVATE_DATA->heater_outlet_property)
#define AUX_HEATER_OUTLET_1_ITEM						(AUX_HEATER_OUTLET_PROPERTY->items + 0)
#define AUX_HEATER_OUTLET_2_ITEM						(AUX_HEATER_OUTLET_PROPERTY->items + 1)
#define AUX_HEATER_OUTLET_3_ITEM						(AUX_HEATER_OUTLET_PROPERTY->items + 2)

#define AUX_HEATER_OUTLET_STATE_PROPERTY		(PRIVATE_DATA->heater_outlet_state_property)
#define AUX_HEATER_OUTLET_STATE_1_ITEM			(AUX_HEATER_OUTLET_STATE_PROPERTY->items + 0)
#define AUX_HEATER_OUTLET_STATE_2_ITEM			(AUX_HEATER_OUTLET_STATE_PROPERTY->items + 1)
#define AUX_HEATER_OUTLET_STATE_3_ITEM			(AUX_HEATER_OUTLET_STATE_PROPERTY->items + 2)

#define AUX_HEATER_OUTLET_CURRENT_PROPERTY	(PRIVATE_DATA->heater_outlet_current_property)
#define AUX_HEATER_OUTLET_CURRENT_1_ITEM		(AUX_HEATER_OUTLET_CURRENT_PROPERTY->items + 0)
#define AUX_HEATER_OUTLET_CURRENT_2_ITEM		(AUX_HEATER_OUTLET_CURRENT_PROPERTY->items + 1)
#define AUX_HEATER_OUTLET_CURRENT_3_ITEM		(AUX_HEATER_OUTLET_CURRENT_PROPERTY->items + 2)


#define AUX_WEATHER_PROPERTY								(PRIVATE_DATA->weather_property)
#define AUX_WEATHER_TEMPERATURE_ITEM				(AUX_WEATHER_PROPERTY->items + 0)
#define AUX_WEATHER_HUMIDITY_ITEM						(AUX_WEATHER_PROPERTY->items + 1)
#define AUX_WEATHER_DEWPOINT_ITEM						(AUX_WEATHER_PROPERTY->items + 2)

#define AUX_DEW_CONTROL_PROPERTY						(PRIVATE_DATA->heating_mode_property)
#define AUX_DEW_CONTROL_MANUAL_ITEM					(AUX_DEW_CONTROL_PROPERTY->items + 0)
#define AUX_DEW_CONTROL_AUTOMATIC_ITEM			(AUX_DEW_CONTROL_PROPERTY->items + 1)

#define AUX_INFO_PROPERTY										(PRIVATE_DATA->info_property)
#define X_AUX_AVERAGE_ITEM									(AUX_INFO_PROPERTY->items + 0)
#define X_AUX_AMP_HOUR_ITEM									(AUX_INFO_PROPERTY->items + 1)
#define X_AUX_WATT_HOUR_ITEM								(AUX_INFO_PROPERTY->items + 2)
#define X_AUX_VOLTAGE_ITEM									(AUX_INFO_PROPERTY->items + 3)
#define X_AUX_CURRENT_ITEM									(AUX_INFO_PROPERTY->items + 4)
#define X_AUX_POWER_ITEM										(AUX_INFO_PROPERTY->items + 5)



#define AUX_GROUP															"USB Dewpoint"




typedef struct {
	int handle;
	indigo_timer *aux_timer;
	indigo_property *outlet_names_property;
	indigo_property *heater_outlet_property;
	indigo_property *heater_outlet_state_property;
	indigo_property *heater_outlet_current_property;
	indigo_property *heating_mode_property;
	indigo_property *weather_property;
	indigo_property *info_property;
	int version;
	pthread_mutex_t mutex;
} usbdp_private_data;

// -------------------------------------------------------------------------------- Low level communication routines

#define UDP_CMD_LEN 6
#define UDP_STATUS_CMD "SGETAL"
#define UDP2_OUTPUT_CMD "S%1uO%03u"         // channel 1-3, power 0-100
#define UDP2_THRESHOLD_CMD "STHR%1u%1u"     // channel 1-2, value 0-9
#define UDP2_CALIBRATION_CMD "SCA%1u%1u%1u" // channel 1-2-A, value 0-9
#define UDP2_LINK_CMD "SLINK%1u"            // 0 or 1 to link channels 2 and 3
#define UDP2_AUTO_CMD "SAUTO%1u"            // 0 or 1 to enable auto mode
#define UDP2_AGGRESSIVITY_CMD "SAGGR%1u"    // 1-4 (1, 2, 5, 10)
#define UDP2_IDENTIFY_CMD "SWHOIS"
#define UDP_RESET_CMD "SEERAZ"

#define UDP1_STATUS_RESPONSE "Tloc=%f-Tamb=%f-RH=%f-DP=%f-TH=%d-C=%d"
#define UDP2_STATUS_RESPONSE "##%f/%f/%f/%f/%f/%u/%u/%u/%u/%u/%u/%u/%u/%u/%u/%u**"
#define UDP2_STATUS_START "##"
#define UDP2_STATUS_SEPARATOR "/"
#define UDP2_STATUS_END "**"

#define UDP_IDENTIFY_RESPONSE "UDP2(%u)" // Firmware version? Mine is "UDP2(1446)"

typedef struct {
	float tloc;
	float tamb;
	float rh;
	float dewpoint;
	int th;
	int c;
} usbdp_status_t;


static bool usbdp_command(indigo_device *device, char *command, char *response, int max) {
	tcflush(PRIVATE_DATA->handle, TCIOFLUSH);
	indigo_write(PRIVATE_DATA->handle, command, strlen(command));
	//indigo_write(PRIVATE_DATA->handle, "\n\r", 2);
	if (response != NULL) {
		if (indigo_read_line(PRIVATE_DATA->handle, response, max) == -1) {
			INDIGO_DRIVER_LOG(DRIVER_NAME, "Command %s -> no response", command);
			return false;
		}
	}
	INDIGO_DRIVER_LOG(DRIVER_NAME, "Command %s -> <%s>", command, response != NULL ? response : "NULL");
	return true;
}

static bool usbdp_status(indigo_device *device, usbdp_status_t *status) {
	char response[80];
	if (!usbdp_command(device, UDP_STATUS_CMD, response, sizeof(response))) {
		return false;
	}

	if (PRIVATE_DATA->version == 1) {
		int parsed = sscanf(response, UDP1_STATUS_RESPONSE, &status->tloc, &status->tamb, &status->rh, &status->dewpoint, &status->th, &status->c);
		if (parsed == 6) {
			INDIGO_DRIVER_LOG(DRIVER_NAME, "Tloc=%f Tamb=%f RH=%f DP=%f TH=%d C=%d", status->tloc, status->tamb, status->rh, status->dewpoint, status->th, status->c);
			return true;
		} else {
			INDIGO_DRIVER_ERROR(DRIVER_NAME," status parse failed parsed %d values", parsed);
			return false;
		}
	} else if (PRIVATE_DATA->version == 2) {

	} else {
		return false;
	}
}

// -------------------------------------------------------------------------------- INDIGO aux device implementation

static indigo_result aux_enumerate_properties(indigo_device *device, indigo_client *client, indigo_property *property);

static indigo_result aux_attach(indigo_device *device) {
	assert(device != NULL);
	assert(PRIVATE_DATA != NULL);
	if (indigo_aux_attach(device, DRIVER_VERSION, INDIGO_INTERFACE_AUX_POWERBOX) == INDIGO_OK) {
		INFO_PROPERTY->count = 5;
		strcpy(INFO_DEVICE_MODEL_ITEM->text.value, "Unknown");
		strcpy(INFO_DEVICE_FW_REVISION_ITEM->text.value, "Unknown");
		// -------------------------------------------------------------------------------- OUTLET_NAMES
		AUX_OUTLET_NAMES_PROPERTY = indigo_init_text_property(NULL, device->name, "X_AUX_OUTLET_NAMES", AUX_GROUP, "Outlet names", INDIGO_OK_STATE, INDIGO_RW_PERM, 3);
		if (AUX_OUTLET_NAMES_PROPERTY == NULL)
			return INDIGO_FAILED;
		indigo_init_text_item(AUX_HEATER_OUTLET_NAME_1_ITEM, AUX_HEATER_OUTLET_NAME_1_ITEM_NAME, "Heater #1", "Heater #1");
		indigo_init_text_item(AUX_HEATER_OUTLET_NAME_2_ITEM, AUX_HEATER_OUTLET_NAME_2_ITEM_NAME, "Heater #2", "Heater #2");
		indigo_init_text_item(AUX_HEATER_OUTLET_NAME_3_ITEM, AUX_HEATER_OUTLET_NAME_3_ITEM_NAME, "Heater #3", "Heater #3");
		// -------------------------------------------------------------------------------- HEATER OUTLETS
		AUX_HEATER_OUTLET_PROPERTY = indigo_init_number_property(NULL, device->name, AUX_HEATER_OUTLET_PROPERTY_NAME, AUX_GROUP, "Heater outlets", INDIGO_OK_STATE, INDIGO_RW_PERM, 3);
		if (AUX_HEATER_OUTLET_PROPERTY == NULL)
			return INDIGO_FAILED;
		indigo_init_number_item(AUX_HEATER_OUTLET_1_ITEM, AUX_HEATER_OUTLET_1_ITEM_NAME, "Heater #1 [%]", 0, 100, 5, 0);
		indigo_init_number_item(AUX_HEATER_OUTLET_2_ITEM, AUX_HEATER_OUTLET_2_ITEM_NAME, "Heater #2 [%]", 0, 100, 5, 0);
		indigo_init_number_item(AUX_HEATER_OUTLET_3_ITEM, AUX_HEATER_OUTLET_3_ITEM_NAME, "Heater #3 [%]", 0, 100, 5, 0);
		AUX_HEATER_OUTLET_STATE_PROPERTY = indigo_init_light_property(NULL, device->name, AUX_HEATER_OUTLET_STATE_PROPERTY_NAME, AUX_GROUP, "Heater outlets state", INDIGO_OK_STATE, 3);
		if (AUX_HEATER_OUTLET_STATE_PROPERTY == NULL)
			return INDIGO_FAILED;
		indigo_init_light_item(AUX_HEATER_OUTLET_STATE_1_ITEM, AUX_HEATER_OUTLET_STATE_1_ITEM_NAME, "Heater #1 state", INDIGO_OK_STATE);
		indigo_init_light_item(AUX_HEATER_OUTLET_STATE_2_ITEM, AUX_HEATER_OUTLET_STATE_2_ITEM_NAME, "Heater #2 state", INDIGO_OK_STATE);
		indigo_init_light_item(AUX_HEATER_OUTLET_STATE_3_ITEM, AUX_HEATER_OUTLET_STATE_3_ITEM_NAME, "Heater #3 state", INDIGO_OK_STATE);
		AUX_HEATER_OUTLET_CURRENT_PROPERTY = indigo_init_number_property(NULL, device->name, AUX_HEATER_OUTLET_CURRENT_PROPERTY_NAME, AUX_GROUP, "Heater outlets current", INDIGO_OK_STATE, INDIGO_RO_PERM, 3);
		if (AUX_HEATER_OUTLET_CURRENT_PROPERTY == NULL)
			return INDIGO_FAILED;
		indigo_init_number_item(AUX_HEATER_OUTLET_CURRENT_1_ITEM, AUX_HEATER_OUTLET_CURRENT_1_ITEM_NAME, "Heater #1 current [A]", 0, 3, 0, 0);
		indigo_init_number_item(AUX_HEATER_OUTLET_CURRENT_2_ITEM, AUX_HEATER_OUTLET_CURRENT_2_ITEM_NAME, "Heater #2 current [A]", 0, 3, 0, 0);
		indigo_init_number_item(AUX_HEATER_OUTLET_CURRENT_3_ITEM, AUX_HEATER_OUTLET_CURRENT_3_ITEM_NAME, "Heater #3 current [A]", 0, 3, 0, 0);
		AUX_DEW_CONTROL_PROPERTY = indigo_init_switch_property(NULL, device->name, AUX_DEW_CONTROL_PROPERTY_NAME, AUX_GROUP, "Dew control", INDIGO_OK_STATE, INDIGO_RW_PERM, INDIGO_ONE_OF_MANY_RULE, 2);
		if (AUX_DEW_CONTROL_PROPERTY == NULL)
			return INDIGO_FAILED;
		indigo_init_switch_item(AUX_DEW_CONTROL_MANUAL_ITEM, AUX_DEW_CONTROL_MANUAL_ITEM_NAME, "Manual", true);
		indigo_init_switch_item(AUX_DEW_CONTROL_AUTOMATIC_ITEM, AUX_DEW_CONTROL_AUTOMATIC_ITEM_NAME, "Automatic", true);
		// -------------------------------------------------------------------------------- WEATHER
		AUX_WEATHER_PROPERTY = indigo_init_number_property(NULL, device->name, AUX_WEATHER_PROPERTY_NAME, AUX_GROUP, "Weather info", INDIGO_OK_STATE, INDIGO_RO_PERM, 3);
		if (AUX_WEATHER_PROPERTY == NULL)
			return INDIGO_FAILED;
		indigo_init_number_item(AUX_WEATHER_TEMPERATURE_ITEM, AUX_WEATHER_TEMPERATURE_ITEM_NAME, "Temperature [C]", -50, 100, 0, 0);
		indigo_init_number_item(AUX_WEATHER_HUMIDITY_ITEM, AUX_WEATHER_HUMIDITY_ITEM_NAME, "Humidity [%]", 0, 100, 0, 0);
		indigo_init_number_item(AUX_WEATHER_DEWPOINT_ITEM, AUX_WEATHER_DEWPOINT_ITEM_NAME, "Dewpoint [C]", -50, 100, 0, 0);
		// -------------------------------------------------------------------------------- INFO
		AUX_INFO_PROPERTY = indigo_init_number_property(NULL, device->name, AUX_INFO_PROPERTY_NAME, AUX_GROUP, "Sensors", INDIGO_OK_STATE, INDIGO_RO_PERM, 6);
		if (AUX_INFO_PROPERTY == NULL)
			return INDIGO_FAILED;
		indigo_init_number_item(X_AUX_AVERAGE_ITEM, "X_AUX_AVERAGE", "Avereage current [A]", 0, 100, 0, 0);
		indigo_init_number_item(X_AUX_AMP_HOUR_ITEM, "X_AUX_AMP_HOUR", "Amp-hour [Ah]", 0, 100, 0, 0);
		indigo_init_number_item(X_AUX_WATT_HOUR_ITEM, "X_AUX_WATT_HOUR", "Watt-hour [Wh]", 0, 100, 0, 0);
		indigo_init_number_item(X_AUX_VOLTAGE_ITEM, "X_AUX_VOLTAGE", "Voltage [V]", 0, 15, 0, 0);
		indigo_init_number_item(X_AUX_CURRENT_ITEM, "X_AUX_CURRENT", "Current [A]", 0, 20, 0, 0);
		indigo_init_number_item(X_AUX_POWER_ITEM, "X_AUX_POWER_OUTLET", "Power [W]", 0, 200, 0, 0);
		// -------------------------------------------------------------------------------- DEVICE_PORT, DEVICE_PORTS
		DEVICE_PORT_PROPERTY->hidden = false;
		DEVICE_PORTS_PROPERTY->hidden = false;
#ifdef INDIGO_LINUX
		strcpy(DEVICE_PORT_ITEM->text.value, "/dev/ttyACM0");
#endif
		// --------------------------------------------------------------------------------
		pthread_mutex_init(&PRIVATE_DATA->mutex, NULL);
		INDIGO_DEVICE_ATTACH_LOG(DRIVER_NAME, device->name);
		return aux_enumerate_properties(device, NULL, NULL);
	}
	return INDIGO_FAILED;
}

static indigo_result aux_enumerate_properties(indigo_device *device, indigo_client *client, indigo_property *property) {
	if (IS_CONNECTED) {
		if (indigo_property_match(AUX_HEATER_OUTLET_PROPERTY, property))
			indigo_define_property(device, AUX_HEATER_OUTLET_PROPERTY, NULL);
		if (indigo_property_match(AUX_HEATER_OUTLET_STATE_PROPERTY, property))
			indigo_define_property(device, AUX_HEATER_OUTLET_STATE_PROPERTY, NULL);
		if (indigo_property_match(AUX_HEATER_OUTLET_CURRENT_PROPERTY, property))
			indigo_define_property(device, AUX_HEATER_OUTLET_CURRENT_PROPERTY, NULL);
		if (indigo_property_match(AUX_DEW_CONTROL_PROPERTY, property))
			indigo_define_property(device, AUX_DEW_CONTROL_PROPERTY, NULL);
		if (indigo_property_match(AUX_WEATHER_PROPERTY, property))
			indigo_define_property(device, AUX_WEATHER_PROPERTY, NULL);
		if (indigo_property_match(AUX_INFO_PROPERTY, property))
			indigo_define_property(device, AUX_INFO_PROPERTY, NULL);
	}
	if (indigo_property_match(AUX_OUTLET_NAMES_PROPERTY, property))
		indigo_define_property(device, AUX_OUTLET_NAMES_PROPERTY, NULL);
	return indigo_aux_enumerate_properties(device, NULL, NULL);
}

static void aux_timer_callback(indigo_device *device) {
	if (!IS_CONNECTED)
		return;
	char response[128];

	bool updateHeaterOutlet = false;
	bool updateHeaterOutletState = false;
	bool updateHeaterOutletCurrent = false;
	bool updateWeather = false;
	bool updateInfo = false;
	bool updateAutoHeater = false;
	bool updateHub = false;

	usbdp_status_t status;

	pthread_mutex_lock(&PRIVATE_DATA->mutex);
	if (usbdp_status(device, &status)) {
	}

	if (updateHeaterOutlet) {
		AUX_HEATER_OUTLET_PROPERTY->state = INDIGO_OK_STATE;
		indigo_update_property(device, AUX_HEATER_OUTLET_PROPERTY, NULL);
	}
	if (updateHeaterOutletState) {
		AUX_HEATER_OUTLET_STATE_PROPERTY->state = INDIGO_OK_STATE;
		indigo_update_property(device, AUX_HEATER_OUTLET_STATE_PROPERTY, NULL);
	}
	if (updateHeaterOutletCurrent) {
		AUX_HEATER_OUTLET_CURRENT_PROPERTY->state = INDIGO_OK_STATE;
		indigo_update_property(device, AUX_HEATER_OUTLET_CURRENT_PROPERTY, NULL);
	}
	if (updateAutoHeater) {
		AUX_DEW_CONTROL_PROPERTY->state = INDIGO_OK_STATE;
		indigo_update_property(device, AUX_DEW_CONTROL_PROPERTY, NULL);
	}
	if (updateWeather) {
		AUX_WEATHER_PROPERTY->state = INDIGO_OK_STATE;
		indigo_update_property(device, AUX_WEATHER_PROPERTY, NULL);
	}
	if (updateInfo) {
		AUX_INFO_PROPERTY->state = INDIGO_OK_STATE;
		indigo_update_property(device, AUX_INFO_PROPERTY, NULL);
	}

	indigo_reschedule_timer(device, 2, &PRIVATE_DATA->aux_timer);
	pthread_mutex_unlock(&PRIVATE_DATA->mutex);
}

static void aux_connection_handler(indigo_device *device) {
	char response[128];
	pthread_mutex_lock(&PRIVATE_DATA->mutex);
	if (CONNECTION_CONNECTED_ITEM->sw.value) {
		CONNECTION_PROPERTY->state = INDIGO_BUSY_STATE;
		indigo_update_property(device, CONNECTION_PROPERTY, NULL);
		PRIVATE_DATA->handle = indigo_open_serial_with_speed(DEVICE_PORT_ITEM->text.value, 19200);
		if (PRIVATE_DATA->handle > 0) {
			if (usbdp_command(device, "SWHOIS", response, sizeof(response))) {
				if (!strcmp(response, "UDP")) {
					INDIGO_DRIVER_LOG(DRIVER_NAME, "Connected to USB_Dewpoint at %s", DEVICE_PORT_ITEM->text.value);
					PRIVATE_DATA->version = 1;
					AUX_HEATER_OUTLET_PROPERTY->hidden = true;
					AUX_HEATER_OUTLET_STATE_PROPERTY->hidden = true;
					AUX_HEATER_OUTLET_CURRENT_PROPERTY->hidden = true;
				} else if (!strncmp(response, "UDP2", 4)) {
					INDIGO_DRIVER_LOG(DRIVER_NAME, "Connected to USB_Dewpoint v2 at %s", DEVICE_PORT_ITEM->text.value);
					PRIVATE_DATA->version = 2;
					AUX_HEATER_OUTLET_PROPERTY->count = 3;
					AUX_HEATER_OUTLET_STATE_PROPERTY->count = 3;
					AUX_HEATER_OUTLET_CURRENT_PROPERTY->count = 3;
				} else {
					INDIGO_DRIVER_ERROR(DRIVER_NAME, "USB_Dewpoint not detected");
					close(PRIVATE_DATA->handle);
					PRIVATE_DATA->handle = 0;
				}
			} else {
				INDIGO_DRIVER_ERROR(DRIVER_NAME, "USB_Dewpoint not detected");
				close(PRIVATE_DATA->handle);
				PRIVATE_DATA->handle = 0;
			}
		}

		if (PRIVATE_DATA->handle > 0) {
			usbdp_command(device, "FTPAMB", response, sizeof(response));
			//char cmd[80];
			//sprintf(cmd, UDP2_AUTO_CMD, 1);
			//usbdp_command(device, cmd, response, sizeof(response));
			//usbdp_command(device, "123456", response, sizeof(response));
			usbdp_status_t status;

			if (usbdp_status(device, &status)) {
				if (PRIVATE_DATA->version == 1) {

				} else if (PRIVATE_DATA->version == 2){

				} else {
					INDIGO_DRIVER_ERROR(DRIVER_NAME, "Failed to parse 'SGETAL' response");
					close(PRIVATE_DATA->handle);
					PRIVATE_DATA->handle = 0;
				}
			} else {
				INDIGO_DRIVER_ERROR(DRIVER_NAME, "Failed to read 'SGETAL' response");
				close(PRIVATE_DATA->handle);
				PRIVATE_DATA->handle = 0;
			}
			PRIVATE_DATA->aux_timer = indigo_set_timer(device, 0, aux_timer_callback);
			CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
		} else {
			INDIGO_DRIVER_ERROR(DRIVER_NAME, "Failed to connect to %s", DEVICE_PORT_ITEM->text.value);
			CONNECTION_PROPERTY->state = INDIGO_ALERT_STATE;
			indigo_set_switch(CONNECTION_PROPERTY, CONNECTION_DISCONNECTED_ITEM, true);
		}
	} else {
		indigo_cancel_timer(device, &PRIVATE_DATA->aux_timer);
		// TODO: stop heaters
		indigo_delete_property(device, AUX_HEATER_OUTLET_PROPERTY, NULL);
		indigo_delete_property(device, AUX_HEATER_OUTLET_STATE_PROPERTY, NULL);
		indigo_delete_property(device, AUX_HEATER_OUTLET_CURRENT_PROPERTY, NULL);
		indigo_delete_property(device, AUX_DEW_CONTROL_PROPERTY, NULL);
		indigo_delete_property(device, AUX_WEATHER_PROPERTY, NULL);
		indigo_delete_property(device, AUX_INFO_PROPERTY, NULL);

		strcpy(INFO_DEVICE_MODEL_ITEM->text.value, "Unknown");
		strcpy(INFO_DEVICE_FW_REVISION_ITEM->text.value, "Unknown");
		indigo_update_property(device, INFO_PROPERTY, NULL);
		if (PRIVATE_DATA->handle > 0) {
			usbdp_command(device, "PL:0", response, sizeof(response));
			INDIGO_DRIVER_LOG(DRIVER_NAME, "Disconnected");
			close(PRIVATE_DATA->handle);
			PRIVATE_DATA->handle = 0;
		}

		CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
	}
	indigo_aux_change_property(device, NULL, CONNECTION_PROPERTY);
	pthread_mutex_unlock(&PRIVATE_DATA->mutex);
}

static void aux_outlet_names_handler(indigo_device *device) {
	pthread_mutex_lock(&PRIVATE_DATA->mutex);
	if (IS_CONNECTED) {
		indigo_delete_property(device, AUX_HEATER_OUTLET_PROPERTY, NULL);
		indigo_delete_property(device, AUX_HEATER_OUTLET_STATE_PROPERTY, NULL);
		indigo_delete_property(device, AUX_HEATER_OUTLET_CURRENT_PROPERTY, NULL);
	}
	snprintf(AUX_HEATER_OUTLET_1_ITEM->label, INDIGO_NAME_SIZE, "%s [%%]", AUX_HEATER_OUTLET_NAME_1_ITEM->text.value);
	snprintf(AUX_HEATER_OUTLET_2_ITEM->label, INDIGO_NAME_SIZE, "%s [%%]", AUX_HEATER_OUTLET_NAME_2_ITEM->text.value);
	snprintf(AUX_HEATER_OUTLET_3_ITEM->label, INDIGO_NAME_SIZE, "%s [%%]", AUX_HEATER_OUTLET_NAME_3_ITEM->text.value);
	snprintf(AUX_HEATER_OUTLET_STATE_1_ITEM->label, INDIGO_NAME_SIZE, "%s state", AUX_HEATER_OUTLET_NAME_1_ITEM->text.value);
	snprintf(AUX_HEATER_OUTLET_STATE_2_ITEM->label, INDIGO_NAME_SIZE, "%s state", AUX_HEATER_OUTLET_NAME_2_ITEM->text.value);
	snprintf(AUX_HEATER_OUTLET_STATE_3_ITEM->label, INDIGO_NAME_SIZE, "%s state", AUX_HEATER_OUTLET_NAME_3_ITEM->text.value);
	snprintf(AUX_HEATER_OUTLET_CURRENT_1_ITEM->label, INDIGO_NAME_SIZE, "%s current [A]", AUX_HEATER_OUTLET_NAME_1_ITEM->text.value);
	snprintf(AUX_HEATER_OUTLET_CURRENT_2_ITEM->label, INDIGO_NAME_SIZE, "%s current [A]", AUX_HEATER_OUTLET_NAME_2_ITEM->text.value);
	snprintf(AUX_HEATER_OUTLET_CURRENT_3_ITEM->label, INDIGO_NAME_SIZE, "%s current [A]", AUX_HEATER_OUTLET_NAME_3_ITEM->text.value);
	AUX_OUTLET_NAMES_PROPERTY->state = INDIGO_OK_STATE;
	if (IS_CONNECTED) {
		indigo_define_property(device, AUX_HEATER_OUTLET_PROPERTY, NULL);
		indigo_define_property(device, AUX_HEATER_OUTLET_STATE_PROPERTY, NULL);
		indigo_define_property(device, AUX_HEATER_OUTLET_CURRENT_PROPERTY, NULL);
		indigo_update_property(device, AUX_OUTLET_NAMES_PROPERTY, NULL);
	}
	pthread_mutex_unlock(&PRIVATE_DATA->mutex);
}

static void aux_heater_outlet_handler(indigo_device *device) {
	char command[16], response[128];
	pthread_mutex_lock(&PRIVATE_DATA->mutex);
	if (IS_CONNECTED) {
		sprintf(command, "P5:%d", (int)(AUX_HEATER_OUTLET_1_ITEM->number.value * 255.0 / 100.0));
		usbdp_command(device, command, response, sizeof(response));
		sprintf(command, "P6:%d", (int)(AUX_HEATER_OUTLET_2_ITEM->number.value * 255.0 / 100.0));
		usbdp_command(device, command, response, sizeof(response));
		if (PRIVATE_DATA->version == 2) {
			sprintf(command, "P7:%d", (int)(AUX_HEATER_OUTLET_3_ITEM->number.value * 255.0 / 100.0));
			usbdp_command(device, command, response, sizeof(response));
		}
		AUX_HEATER_OUTLET_PROPERTY->state = INDIGO_OK_STATE;
		indigo_update_property(device, AUX_HEATER_OUTLET_PROPERTY, NULL);
	}
	pthread_mutex_unlock(&PRIVATE_DATA->mutex);
}

static void aux_dew_control_handler(indigo_device *device) {
	char response[128];
	pthread_mutex_lock(&PRIVATE_DATA->mutex);
	if (IS_CONNECTED) {
		usbdp_command(device, AUX_DEW_CONTROL_AUTOMATIC_ITEM->sw.value ? "PD:1" : "PD:0", response, sizeof(response));
		AUX_DEW_CONTROL_PROPERTY->state = INDIGO_OK_STATE;
		indigo_update_property(device, AUX_DEW_CONTROL_PROPERTY, NULL);
	}
	pthread_mutex_unlock(&PRIVATE_DATA->mutex);
}

static indigo_result aux_change_property(indigo_device *device, indigo_client *client, indigo_property *property) {
	assert(device != NULL);
	assert(DEVICE_CONTEXT != NULL);
	assert(property != NULL);
	if (indigo_property_match(CONNECTION_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- CONNECTION
		indigo_property_copy_values(CONNECTION_PROPERTY, property, false);
		indigo_set_timer(device, 0, aux_connection_handler);
		return INDIGO_OK;
	} else if (indigo_property_match(AUX_OUTLET_NAMES_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- X_AUX_OUTLET_NAMES
		indigo_property_copy_values(AUX_OUTLET_NAMES_PROPERTY, property, false);
		indigo_set_timer(device, 0, aux_outlet_names_handler);
		return INDIGO_OK;
	} else if (indigo_property_match(AUX_HEATER_OUTLET_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- AUX_HEATER_OUTLET
		indigo_property_copy_values(AUX_HEATER_OUTLET_PROPERTY, property, false);
		indigo_set_timer(device, 0, aux_heater_outlet_handler);
		return INDIGO_OK;
	} else if (indigo_property_match(AUX_DEW_CONTROL_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- AUX_DEW_CONTROL
		indigo_property_copy_values(AUX_DEW_CONTROL_PROPERTY, property, false);
		indigo_set_timer(device, 0, aux_dew_control_handler);
		return INDIGO_OK;
		// -------------------------------------------------------------------------------- CONFIG
	} else if (indigo_property_match(CONFIG_PROPERTY, property)) {
		if (indigo_switch_match(CONFIG_SAVE_ITEM, property)) {
			indigo_save_property(device, NULL, AUX_OUTLET_NAMES_PROPERTY);
		}
	}
	return indigo_aux_change_property(device, client, property);
}

static indigo_result aux_detach(indigo_device *device) {
	assert(device != NULL);
	if (CONNECTION_CONNECTED_ITEM->sw.value) {
		indigo_set_switch(CONNECTION_PROPERTY, CONNECTION_DISCONNECTED_ITEM, true);
		aux_connection_handler(device);
	}
	indigo_release_property(AUX_HEATER_OUTLET_PROPERTY);
	indigo_release_property(AUX_HEATER_OUTLET_STATE_PROPERTY);
	indigo_release_property(AUX_HEATER_OUTLET_CURRENT_PROPERTY);
	indigo_release_property(AUX_DEW_CONTROL_PROPERTY);
	indigo_release_property(AUX_WEATHER_PROPERTY);
	indigo_release_property(AUX_INFO_PROPERTY);
	indigo_release_property(AUX_OUTLET_NAMES_PROPERTY);
	pthread_mutex_destroy(&PRIVATE_DATA->mutex);
	INDIGO_DEVICE_DETACH_LOG(DRIVER_NAME, device->name);
	return indigo_aux_detach(device);
}

// -------------------------------------------------------------------------------- INDIGO driver implementation

indigo_result indigo_aux_usbdp(indigo_driver_action action, indigo_driver_info *info) {
	static indigo_driver_action last_action = INDIGO_DRIVER_SHUTDOWN;
	static usbdp_private_data *private_data = NULL;
	static indigo_device *aux = NULL;

	static indigo_device aux_template = INDIGO_DEVICE_INITIALIZER(
		"USB Dewpoint",
		aux_attach,
		aux_enumerate_properties,
		aux_change_property,
		NULL,
		aux_detach
	);

	SET_DRIVER_INFO(info, "USB Dewpoint", __FUNCTION__, DRIVER_VERSION, false, last_action);

	if (action == last_action)
		return INDIGO_OK;

	switch (action) {
		case INDIGO_DRIVER_INIT:
			last_action = action;
			private_data = malloc(sizeof(usbdp_private_data));
			assert(private_data != NULL);
			memset(private_data, 0, sizeof(usbdp_private_data));
			aux = malloc(sizeof(indigo_device));
			assert(aux != NULL);
			memcpy(aux, &aux_template, sizeof(indigo_device));
			aux->private_data = private_data;
			indigo_attach_device(aux);
			break;

		case INDIGO_DRIVER_SHUTDOWN:
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
