// Copyright (C) 2026 Rumen G. Bogdanovski
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
// 2.0 by Rumen G. Bogdanovski <rumenastro@gmail.com>

/** INDIGO SVBONY PowerBox aux driver
 \file indigo_aux_svbpowerbox.c
 */

#define DRIVER_VERSION 0x0001
#define DRIVER_NAME "indigo_aux_svbpowerbox"

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
#include <sys/ioctl.h>

#include <indigo/indigo_driver_xml.h>
#include <indigo/indigo_io.h>

#include "indigo_aux_svbpowerbox.h"

#define PRIVATE_DATA	((svbpb_private_data *)device->private_data)

#define AUX_OUTLET_NAMES_PROPERTY		(PRIVATE_DATA->outlet_names_property)
#define AUX_POWER_OUTLET_NAME_1_ITEM	(AUX_OUTLET_NAMES_PROPERTY->items + 0)
#define AUX_POWER_OUTLET_NAME_2_ITEM	(AUX_OUTLET_NAMES_PROPERTY->items + 1)
#define AUX_POWER_OUTLET_NAME_3_ITEM	(AUX_OUTLET_NAMES_PROPERTY->items + 2)
#define AUX_POWER_OUTLET_NAME_4_ITEM	(AUX_OUTLET_NAMES_PROPERTY->items + 3)
#define AUX_POWER_OUTLET_NAME_5_ITEM	(AUX_OUTLET_NAMES_PROPERTY->items + 4)
#define AUX_POWER_OUTLET_NAME_6_ITEM	(AUX_OUTLET_NAMES_PROPERTY->items + 5)
#define AUX_HEATER_OUTLET_NAME_1_ITEM	(AUX_OUTLET_NAMES_PROPERTY->items + 6)
#define AUX_HEATER_OUTLET_NAME_2_ITEM	(AUX_OUTLET_NAMES_PROPERTY->items + 7)

#define AUX_POWER_OUTLET_PROPERTY		(PRIVATE_DATA->power_outlet_property)
#define AUX_POWER_OUTLET_1_ITEM			(AUX_POWER_OUTLET_PROPERTY->items + 0)
#define AUX_POWER_OUTLET_2_ITEM			(AUX_POWER_OUTLET_PROPERTY->items + 1)
#define AUX_POWER_OUTLET_3_ITEM			(AUX_POWER_OUTLET_PROPERTY->items + 2)
#define AUX_POWER_OUTLET_4_ITEM			(AUX_POWER_OUTLET_PROPERTY->items + 3)
#define AUX_POWER_OUTLET_5_ITEM			(AUX_POWER_OUTLET_PROPERTY->items + 4)
#define AUX_POWER_OUTLET_6_ITEM			(AUX_POWER_OUTLET_PROPERTY->items + 5)

#define AUX_POWER_OUTLET_VOLTAGE_PROPERTY	(PRIVATE_DATA->power_outlet_voltage_property)
#define AUX_POWER_OUTLET_VOLTAGE_1_ITEM		(AUX_POWER_OUTLET_VOLTAGE_PROPERTY->items + 0)

#define AUX_POWER_OUTLET_CURRENT_PROPERTY	(PRIVATE_DATA->power_outlet_current_property)
#define AUX_POWER_OUTLET_CURRENT_1_ITEM		(AUX_POWER_OUTLET_CURRENT_PROPERTY->items + 0)
#define AUX_POWER_OUTLET_CURRENT_2_ITEM		(AUX_POWER_OUTLET_CURRENT_PROPERTY->items + 1)

#define AUX_HEATER_OUTLET_PROPERTY		(PRIVATE_DATA->heater_outlet_property)
#define AUX_HEATER_OUTLET_1_ITEM		(AUX_HEATER_OUTLET_PROPERTY->items + 0)
#define AUX_HEATER_OUTLET_2_ITEM		(AUX_HEATER_OUTLET_PROPERTY->items + 1)

#define AUX_USB_PORT_PROPERTY			(PRIVATE_DATA->usb_ports_property)
#define AUX_USB_PORT_1_ITEM				(AUX_USB_PORT_PROPERTY->items + 0)
#define AUX_USB_PORT_2_ITEM				(AUX_USB_PORT_PROPERTY->items + 1)

#define AUX_WEATHER_PROPERTY			(PRIVATE_DATA->weather_property)
#define AUX_WEATHER_TEMPERATURE_ITEM	(AUX_WEATHER_PROPERTY->items + 0)
#define AUX_WEATHER_HUMIDITY_ITEM		(AUX_WEATHER_PROPERTY->items + 1)
#define AUX_WEATHER_DEWPOINT_ITEM		(AUX_WEATHER_PROPERTY->items + 2)

#define AUX_DEW_CONTROL_PROPERTY		(PRIVATE_DATA->heating_mode_property)
#define AUX_DEW_CONTROL_MANUAL_ITEM		(AUX_DEW_CONTROL_PROPERTY->items + 0)
#define AUX_DEW_CONTROL_AUTOMATIC_ITEM	(AUX_DEW_CONTROL_PROPERTY->items + 1)

#define AUX_TEMPERATURE_SENSORS_PROPERTY		(PRIVATE_DATA->temperature_sensors_property)
#define AUX_TEMPERATURE_SENSORS_SENSOR_1_ITEM	(AUX_TEMPERATURE_SENSORS_PROPERTY->items + 0)
#define AUX_TEMPERATURE_SENSORS_SENSOR_2_ITEM	(AUX_TEMPERATURE_SENSORS_PROPERTY->items + 1)

#define AUX_DEW_WARNING_PROPERTY		(PRIVATE_DATA->dew_warning_property)
#define AUX_DEW_WARNING_SENSOR_1_ITEM	(AUX_DEW_WARNING_PROPERTY->items + 0)

#define AUX_INFO_PROPERTY			(PRIVATE_DATA->info_property)
#define AUX_INFO_VOLTAGE_ITEM		(AUX_INFO_PROPERTY->items + 0)
#define AUX_INFO_CURRENT_ITEM		(AUX_INFO_PROPERTY->items + 1)
#define AUX_INFO_POWER_ITEM			(AUX_INFO_PROPERTY->items + 2)

#define AUX_GROUP		"Powerbox"
#define WEATHER_GROUP	"Weather"

// Serial protocol constants
#define SVB_FRAME_HEADER		0x24   // '$'
#define SVB_CMD_SET_PORT		0x01
#define SVB_CMD_READ_POWER		0x02
#define SVB_CMD_READ_VOLTAGE	0x03
#define SVB_CMD_READ_DS18B20	0x04
#define SVB_CMD_READ_SHT40_TEMP	0x05
#define SVB_CMD_READ_SHT40_HUMI	0x06
#define SVB_CMD_READ_CURRENT	0x07
#define SVB_CMD_READ_STATE		0x08

// Port indices for set_port command
#define SVB_PORT_DC1		0
#define SVB_PORT_DC2		1
#define SVB_PORT_DC3		2
#define SVB_PORT_DC4		3
#define SVB_PORT_DC5		4
#define SVB_PORT_USB_A		5   // USB C, 1, 2
#define SVB_PORT_USB_B		6   // USB 3, 4, 5
#define SVB_PORT_REGULATED	7
#define SVB_PORT_PWM_A		8
#define SVB_PORT_PWM_B		9

#define SVB_MAX_VOLTAGE		15.3

typedef struct {
	int handle;
	indigo_timer *aux_timer;
	indigo_property *outlet_names_property;
	indigo_property *power_outlet_property;
	indigo_property *heater_outlet_property;
	indigo_property *heating_mode_property;
	indigo_property *weather_property;
	indigo_property *info_property;
	indigo_property *usb_ports_property;
	indigo_property *power_outlet_voltage_property;
	indigo_property *power_outlet_current_property;
	indigo_property *temperature_sensors_property;
	indigo_property *dew_warning_property;
	pthread_mutex_t mutex;
} svbpb_private_data;

// ------------------------------------------------------------ Low level I/O

static int svbpb_read_bytes(int fd, unsigned char *buf, int n, int timeout_ms) {
	int total = 0;
	while (total < n) {
		fd_set rfds;
		struct timeval tv;
		FD_ZERO(&rfds);
		FD_SET(fd, &rfds);
		tv.tv_sec  = timeout_ms / 1000;
		tv.tv_usec = (timeout_ms % 1000) * 1000;
		int ret = select(fd + 1, &rfds, NULL, NULL, &tv);
		if (ret <= 0) {
			return -1; // timeout or error
		}
		int nr = (int)read(fd, buf + total, n - total);
		if (nr <= 0) {
			return -1;
		}
		total += nr;
	}
	return total;
}

static bool svbpb_command(indigo_device *device, const unsigned char *cmd, int cmd_len, unsigned char *res, int res_len) {
	unsigned char frame[8] = {0};
	int full_cmd_len = 2 + cmd_len + 1;   // header + data_len + cmd + checksum
	int full_res_len = 3 + res_len + 1;   // header + data_len + cmd_echo + res + checksum
	unsigned int checksum = 0;

	if (full_cmd_len > (int)sizeof(frame)) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "Command too long");
		return false;
	}

	// Build frame
	frame[0] = SVB_FRAME_HEADER;
	frame[1] = (unsigned char)full_cmd_len;
	for (int i = 0; i < cmd_len; i++) {
		frame[2 + i] = cmd[i];
	}
	checksum = 0;
	for (int i = 0; i < 2 + cmd_len; i++) {
		checksum += frame[i];
	}
	frame[full_cmd_len - 1] = (unsigned char)(checksum % 0xFF);

	INDIGO_DRIVER_DEBUG(DRIVER_NAME, "TX frame (%d bytes)", full_cmd_len);

	// Send
	tcflush(PRIVATE_DATA->handle, TCIOFLUSH);
	if ((int)write(PRIVATE_DATA->handle, frame, full_cmd_len) != full_cmd_len) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "Write failed: %s", strerror(errno));
		return false;
	}
	tcdrain(PRIVATE_DATA->handle);
	indigo_usleep(100000); // 100 ms

	// Receive
	unsigned char response[20] = {0};
	if (svbpb_read_bytes(PRIVATE_DATA->handle, response, full_res_len, 500) != full_res_len) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "Read timeout or error (expected %d bytes)", full_res_len);
		return false;
	}
	tcflush(PRIVATE_DATA->handle, TCIOFLUSH);

	// Validate checksum
	checksum = 0;
	for (int i = 0; i < full_res_len - 1; i++) {
		checksum += response[i];
	}
	if ((unsigned char)(checksum % 0xFF) != response[full_res_len - 1]) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "Checksum mismatch in response");
		return false;
	}

	// Copy result data
	if (res != NULL) {
		for (int i = 0; i < res_len; i++) {
			res[i] = response[3 + i];
		}
	}

	// cmd echo byte 0xaa means device reported an error
	return (response[2] != 0xaa);
}

/*
 * Convert a big-endian 4-byte buffer to a double by dividing by scale.
 */
static double svbpb_4bytes_to_double(const unsigned char *data, double scale) {
	if (data == NULL || scale == 0.0) return 0.0;
	uint32_t raw = ((uint32_t)data[0] << 24) |
				   ((uint32_t)data[1] << 16) |
				   ((uint32_t)data[2] <<  8) |
					(uint32_t)data[3];
	return (double)raw / scale;
}

// ------------------------------------------------------------ Port set helpers

static bool svbpb_set_port(indigo_device *device, uint8_t port, uint8_t value) {
	unsigned char cmd[3] = { SVB_CMD_SET_PORT, port, value };
	return svbpb_command(device, cmd, 3, NULL, 2);
}

// ------------------------------------------------------------ INDIGO aux device implementation

static indigo_result aux_enumerate_properties(indigo_device *device, indigo_client *client, indigo_property *property);

static indigo_result aux_attach(indigo_device *device) {
	assert(device != NULL);
	assert(PRIVATE_DATA != NULL);
	if (indigo_aux_attach(device, DRIVER_NAME, DRIVER_VERSION, INDIGO_INTERFACE_AUX_POWERBOX | INDIGO_INTERFACE_AUX_WEATHER) == INDIGO_OK) {
		INFO_PROPERTY->count = 6;
		strcpy(INFO_DEVICE_MODEL_ITEM->text.value, "Unknown");
		strcpy(INFO_DEVICE_FW_REVISION_ITEM->text.value, "Unknown");
		// -------------------------------------------------------------------------------- OUTLET_NAMES
		AUX_OUTLET_NAMES_PROPERTY = indigo_init_text_property(NULL, device->name, AUX_OUTLET_NAMES_PROPERTY_NAME, AUX_GROUP, "Outlet names", INDIGO_OK_STATE, INDIGO_RW_PERM, 8);
		if (AUX_OUTLET_NAMES_PROPERTY == NULL)
			return INDIGO_FAILED;
		indigo_init_text_item(AUX_POWER_OUTLET_NAME_1_ITEM, AUX_POWER_OUTLET_NAME_1_ITEM_NAME, "Power outlet (DC1)", "DC1");
		indigo_init_text_item(AUX_POWER_OUTLET_NAME_2_ITEM, AUX_POWER_OUTLET_NAME_2_ITEM_NAME, "Power outlet (DC2)", "DC2");
		indigo_init_text_item(AUX_POWER_OUTLET_NAME_3_ITEM, AUX_POWER_OUTLET_NAME_3_ITEM_NAME, "Power outlet (DC3)", "DC3");
		indigo_init_text_item(AUX_POWER_OUTLET_NAME_4_ITEM, AUX_POWER_OUTLET_NAME_4_ITEM_NAME, "Power outlet (DC4)", "DC4");
		indigo_init_text_item(AUX_POWER_OUTLET_NAME_5_ITEM, AUX_POWER_OUTLET_NAME_5_ITEM_NAME, "Power outlet (DC5)", "DC5");
		indigo_init_text_item(AUX_POWER_OUTLET_NAME_6_ITEM, AUX_POWER_OUTLET_NAME_6_ITEM_NAME, "Regulated output", "Regulated");
		indigo_init_text_item(AUX_HEATER_OUTLET_NAME_1_ITEM, AUX_HEATER_OUTLET_NAME_1_ITEM_NAME, "Heater outlet (PWM1)", "PWM1");
		indigo_init_text_item(AUX_HEATER_OUTLET_NAME_2_ITEM, AUX_HEATER_OUTLET_NAME_2_ITEM_NAME, "Heater outlet (PWM2)", "PWM2");
		// -------------------------------------------------------------------------------- POWER_OUTLET
		AUX_POWER_OUTLET_PROPERTY = indigo_init_switch_property(NULL, device->name, AUX_POWER_OUTLET_PROPERTY_NAME, AUX_GROUP, "Power outlets", INDIGO_OK_STATE, INDIGO_RW_PERM, INDIGO_ANY_OF_MANY_RULE, 6);
		if (AUX_POWER_OUTLET_PROPERTY == NULL)
			return INDIGO_FAILED;
		indigo_init_switch_item(AUX_POWER_OUTLET_1_ITEM, AUX_POWER_OUTLET_1_ITEM_NAME, "DC1", true);
		indigo_init_switch_item(AUX_POWER_OUTLET_2_ITEM, AUX_POWER_OUTLET_2_ITEM_NAME, "DC2", true);
		indigo_init_switch_item(AUX_POWER_OUTLET_3_ITEM, AUX_POWER_OUTLET_3_ITEM_NAME, "DC3", true);
		indigo_init_switch_item(AUX_POWER_OUTLET_4_ITEM, AUX_POWER_OUTLET_4_ITEM_NAME, "DC4", true);
		indigo_init_switch_item(AUX_POWER_OUTLET_5_ITEM, AUX_POWER_OUTLET_5_ITEM_NAME, "DC5", true);
		indigo_init_switch_item(AUX_POWER_OUTLET_6_ITEM, AUX_POWER_OUTLET_6_ITEM_NAME, "Regulated", true);
		// -------------------------------------------------------------------------------- POWER_OUTLET_VOLTAGE
		AUX_POWER_OUTLET_VOLTAGE_PROPERTY = indigo_init_number_property(NULL, device->name, AUX_POWER_OUTLET_VOLTAGE_PROPERTY_NAME, AUX_GROUP, "Regulated output voltage", INDIGO_OK_STATE, INDIGO_RW_PERM, 1);
		if (AUX_POWER_OUTLET_VOLTAGE_PROPERTY == NULL)
			return INDIGO_FAILED;
		indigo_init_number_item(AUX_POWER_OUTLET_VOLTAGE_1_ITEM, AUX_POWER_OUTLET_VOLTAGE_1_ITEM_NAME, "Regulated output [V]", 0, SVB_MAX_VOLTAGE, 0.1, SVB_MAX_VOLTAGE);
		// -------------------------------------------------------------------------------- POWER_OUTLET_CURRENT
		AUX_POWER_OUTLET_CURRENT_PROPERTY = indigo_init_number_property(NULL, device->name, AUX_POWER_OUTLET_CURRENT_PROPERTY_NAME, AUX_GROUP, "Output sensors", INDIGO_OK_STATE, INDIGO_RO_PERM, 2);
		if (AUX_POWER_OUTLET_CURRENT_PROPERTY == NULL)
			return INDIGO_FAILED;
		indigo_init_number_item(AUX_POWER_OUTLET_CURRENT_1_ITEM, AUX_POWER_OUTLET_CURRENT_1_ITEM_NAME, "Total current [A]", 0, 20, 0, 0);
		indigo_init_number_item(AUX_POWER_OUTLET_CURRENT_2_ITEM, AUX_POWER_OUTLET_CURRENT_2_ITEM_NAME, "Total power [W]", 0, 300, 0, 0);
		// -------------------------------------------------------------------------------- USB_PORT
		AUX_USB_PORT_PROPERTY = indigo_init_switch_property(NULL, device->name, AUX_USB_PORT_PROPERTY_NAME, AUX_GROUP, "USB Ports", INDIGO_OK_STATE, INDIGO_RW_PERM, INDIGO_ANY_OF_MANY_RULE, 2);
		if (AUX_USB_PORT_PROPERTY == NULL)
			return INDIGO_FAILED;
		indigo_init_switch_item(AUX_USB_PORT_1_ITEM, AUX_USB_PORT_1_ITEM_NAME, "USB ports (USB-C, 1, 2)", true);
		indigo_init_switch_item(AUX_USB_PORT_2_ITEM, AUX_USB_PORT_2_ITEM_NAME, "USB ports (3 - 5)", true);
		// -------------------------------------------------------------------------------- HEATER_OUTLET
		AUX_HEATER_OUTLET_PROPERTY = indigo_init_number_property(NULL, device->name, AUX_HEATER_OUTLET_PROPERTY_NAME, AUX_GROUP, "Heater outlets", INDIGO_OK_STATE, INDIGO_RW_PERM, 2);
		if (AUX_HEATER_OUTLET_PROPERTY == NULL)
			return INDIGO_FAILED;
		indigo_init_number_item(AUX_HEATER_OUTLET_1_ITEM, AUX_HEATER_OUTLET_1_ITEM_NAME, "PWM1 [%]", 0, 100, 5, 0);
		indigo_init_number_item(AUX_HEATER_OUTLET_2_ITEM, AUX_HEATER_OUTLET_2_ITEM_NAME, "PWM2 [%]", 0, 100, 5, 0);
		// -------------------------------------------------------------------------------- AUX_DEW_CONTROL
		AUX_DEW_CONTROL_PROPERTY = indigo_init_switch_property(NULL, device->name, AUX_DEW_CONTROL_PROPERTY_NAME, AUX_GROUP, "Dew control", INDIGO_OK_STATE, INDIGO_RW_PERM, INDIGO_ONE_OF_MANY_RULE, 2);
		if (AUX_DEW_CONTROL_PROPERTY == NULL)
			return INDIGO_FAILED;
		indigo_init_switch_item(AUX_DEW_CONTROL_MANUAL_ITEM, AUX_DEW_CONTROL_MANUAL_ITEM_NAME, "Manual", true);
		indigo_init_switch_item(AUX_DEW_CONTROL_AUTOMATIC_ITEM, AUX_DEW_CONTROL_AUTOMATIC_ITEM_NAME, "Automatic", false);
		// -------------------------------------------------------------------------------- WEATHER
		AUX_WEATHER_PROPERTY = indigo_init_number_property(NULL, device->name, AUX_WEATHER_PROPERTY_NAME, WEATHER_GROUP, "Weather info (SHT40)", INDIGO_IDLE_STATE, INDIGO_RO_PERM, 3);
		if (AUX_WEATHER_PROPERTY == NULL)
			return INDIGO_FAILED;
		indigo_init_number_item(AUX_WEATHER_TEMPERATURE_ITEM, AUX_WEATHER_TEMPERATURE_ITEM_NAME, "Temperature [°C]", -50, 100, 0, 0);
		indigo_init_number_item(AUX_WEATHER_HUMIDITY_ITEM, AUX_WEATHER_HUMIDITY_ITEM_NAME, "Humidity [%]", 0, 100, 0, 0);
		indigo_init_number_item(AUX_WEATHER_DEWPOINT_ITEM, AUX_WEATHER_DEWPOINT_ITEM_NAME, "Dewpoint [°C]", -50, 100, 0, 0);
		// -------------------------------------------------------------------------------- AUX_TEMPERATURE_SENSORS
		AUX_TEMPERATURE_SENSORS_PROPERTY = indigo_init_number_property(NULL, device->name, AUX_TEMPERATURE_SENSORS_PROPERTY_NAME, WEATHER_GROUP, "Temperature sensors", INDIGO_IDLE_STATE, INDIGO_RO_PERM, 2);
		if (AUX_TEMPERATURE_SENSORS_PROPERTY == NULL)
			return INDIGO_FAILED;
		indigo_init_number_item(AUX_TEMPERATURE_SENSORS_SENSOR_1_ITEM, AUX_TEMPERATURE_SENSORS_SENSOR_1_ITEM_NAME, "Temperature (SHT40) [°C]", -50, 120, 0, 0);
		indigo_init_number_item(AUX_TEMPERATURE_SENSORS_SENSOR_2_ITEM, AUX_TEMPERATURE_SENSORS_SENSOR_2_ITEM_NAME, "Temperature (DS18B20) [°C]", -50, 120, 0, 0);
		// -------------------------------------------------------------------------------- DEW_WARNING
		AUX_DEW_WARNING_PROPERTY = indigo_init_light_property(NULL, device->name, AUX_DEW_WARNING_PROPERTY_NAME, WEATHER_GROUP, "Dew warning", INDIGO_IDLE_STATE, 1);
		if (AUX_DEW_WARNING_PROPERTY == NULL)
			return INDIGO_FAILED;
		indigo_init_light_item(AUX_DEW_WARNING_SENSOR_1_ITEM, AUX_DEW_WARNING_SENSOR_1_ITEM_NAME, "Dew warning (SHT40)", INDIGO_IDLE_STATE);
		// -------------------------------------------------------------------------------- INFO
		AUX_INFO_PROPERTY = indigo_init_number_property(NULL, device->name, AUX_INFO_PROPERTY_NAME, AUX_GROUP, "Sensors", INDIGO_OK_STATE, INDIGO_RO_PERM, 3);
		if (AUX_INFO_PROPERTY == NULL)
			return INDIGO_FAILED;
		indigo_init_number_item(AUX_INFO_VOLTAGE_ITEM, AUX_INFO_VOLTAGE_ITEM_NAME, "Voltage [V]", 0, 20, 0, 0);
		indigo_init_number_item(AUX_INFO_CURRENT_ITEM, AUX_INFO_CURRENT_ITEM_NAME, "Current [A]", 0, 30, 0, 0);
		indigo_init_number_item(AUX_INFO_POWER_ITEM, "POWER", "Power [W]", 0, 400, 0, 0);
		// -------------------------------------------------------------------------------- DEVICE_PORT, DEVICE_PORTS
		DEVICE_PORT_PROPERTY->hidden = false;
		DEVICE_PORTS_PROPERTY->hidden = false;
		ADDITIONAL_INSTANCES_PROPERTY->hidden = DEVICE_CONTEXT->base_device != NULL;
#ifdef INDIGO_MACOS
		for (int i = 0; i < DEVICE_PORTS_PROPERTY->count; i++) {
			if (strstr(DEVICE_PORTS_PROPERTY->items[i].name, "usbserial")) {
				indigo_copy_value(DEVICE_PORT_ITEM->text.value, DEVICE_PORTS_PROPERTY->items[i].name);
				break;
			}
		}
#endif
#ifdef INDIGO_LINUX
		strcpy(DEVICE_PORT_ITEM->text.value, "/dev/ttyUSB0");
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
		indigo_define_matching_property(AUX_POWER_OUTLET_PROPERTY);
		indigo_define_matching_property(AUX_POWER_OUTLET_VOLTAGE_PROPERTY);
		indigo_define_matching_property(AUX_POWER_OUTLET_CURRENT_PROPERTY);
		indigo_define_matching_property(AUX_USB_PORT_PROPERTY);
		indigo_define_matching_property(AUX_HEATER_OUTLET_PROPERTY);
		indigo_define_matching_property(AUX_DEW_CONTROL_PROPERTY);
		indigo_define_matching_property(AUX_WEATHER_PROPERTY);
		indigo_define_matching_property(AUX_TEMPERATURE_SENSORS_PROPERTY);
		indigo_define_matching_property(AUX_DEW_WARNING_PROPERTY);
		indigo_define_matching_property(AUX_INFO_PROPERTY);
	}
	indigo_define_matching_property(AUX_OUTLET_NAMES_PROPERTY);
	return indigo_aux_enumerate_properties(device, client, property);
}

static void aux_update_states(indigo_device *device) {
	unsigned char res[10] = {0};
	unsigned char cmd;
	double value;

	// --- INA219 load voltage ---
	cmd = SVB_CMD_READ_VOLTAGE;
	if (svbpb_command(device, &cmd, 1, res, 4)) {
		value = svbpb_4bytes_to_double(res, 100.0);
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "INA219 Voltage: %.2f V", value);
		AUX_INFO_VOLTAGE_ITEM->number.value = value;
	}

	// --- INA219 current ---
	cmd = SVB_CMD_READ_CURRENT;
	if (svbpb_command(device, &cmd, 1, res, 4)) {
		value = svbpb_4bytes_to_double(res, 100.0) / 1000.0; // mA -> A
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "INA219 Current: %.3f A", value);
		AUX_INFO_CURRENT_ITEM->number.value = value;
		AUX_POWER_OUTLET_CURRENT_1_ITEM->number.value = value;
	}

	// --- INA219 power ---
	cmd = SVB_CMD_READ_POWER;
	if (svbpb_command(device, &cmd, 1, res, 4)) {
		value = svbpb_4bytes_to_double(res, 100.0) / 1000.0; // mW -> W
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "INA219 Power: %.3f W", value);
		AUX_INFO_POWER_ITEM->number.value = value;
		AUX_POWER_OUTLET_CURRENT_2_ITEM->number.value = value;
	}
	indigo_update_property(device, AUX_INFO_PROPERTY, NULL);
	indigo_update_property(device, AUX_POWER_OUTLET_CURRENT_PROPERTY, NULL);

	// --- DS18B20 temperature ---
	double ds18b20_temp = -273.15;
	cmd = SVB_CMD_READ_DS18B20;
	if (svbpb_command(device, &cmd, 1, res, 4)) {
		ds18b20_temp = round((svbpb_4bytes_to_double(res, 100.0) - 255.5) * 100.0) / 100.0;
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "DS18B20 Temperature: %.2f C", ds18b20_temp);
	}

	// --- SHT40 temperature ---
	double sht40_temp = -273.15;
	bool has_sht40_temp = false;
	cmd = SVB_CMD_READ_SHT40_TEMP;
	if (svbpb_command(device, &cmd, 1, res, 4)) {
		sht40_temp = round((svbpb_4bytes_to_double(res, 100.0) - 254.0) * 10.0) / 10.0;
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "SHT40 Temperature: %.1f C", sht40_temp);
		has_sht40_temp = true;
	}

	// --- SHT40 humidity ---
	double sht40_humi = -1.0;
	bool has_sht40_humi = false;
	cmd = SVB_CMD_READ_SHT40_HUMI;
	if (svbpb_command(device, &cmd, 1, res, 4)) {
		sht40_humi = round((svbpb_4bytes_to_double(res, 100.0) - 254.0) * 10.0) / 10.0;
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "SHT40 Humidity: %.1f %%", sht40_humi);
		if (sht40_humi < 0.0 || sht40_humi > 110.0) { // leave some margin for sensor errors
			// looks like we do not have a SHT40 sensor
			// when we do not have it we have random reading so we hope they are out of range.
			has_sht40_humi = false;
			has_sht40_temp = false;
			sht40_humi = -1.0;
			sht40_temp = -273.15;
		} else {
			has_sht40_humi = true;
		}
	}

	// Update temperature sensors property
	AUX_TEMPERATURE_SENSORS_SENSOR_1_ITEM->number.value = sht40_temp > -100.0 ? sht40_temp : -273.15;
	AUX_TEMPERATURE_SENSORS_SENSOR_2_ITEM->number.value = ds18b20_temp > -100.0 ? ds18b20_temp : -273.15;
	if (!has_sht40_temp && ds18b20_temp <= -100.0) {
		AUX_TEMPERATURE_SENSORS_PROPERTY->state = INDIGO_IDLE_STATE;
	} else {
		AUX_TEMPERATURE_SENSORS_PROPERTY->state = INDIGO_OK_STATE;
	}
	indigo_update_property(device, AUX_TEMPERATURE_SENSORS_PROPERTY, NULL);

	AUX_WEATHER_TEMPERATURE_ITEM->number.value = sht40_temp > -100.0 ? sht40_temp : -273.15;
	AUX_WEATHER_HUMIDITY_ITEM->number.value = sht40_humi;

	// Update weather property (SHT40)
	if (has_sht40_temp && has_sht40_humi) {
		AUX_WEATHER_DEWPOINT_ITEM->number.value = indigo_aux_dewpoint(sht40_temp, sht40_humi);
		AUX_WEATHER_PROPERTY->state = INDIGO_OK_STATE;
	} else {
		AUX_WEATHER_PROPERTY->state = INDIGO_IDLE_STATE;
		AUX_WEATHER_DEWPOINT_ITEM->number.value = -273.15;
	}
	indigo_update_property(device, AUX_WEATHER_PROPERTY, NULL);

	// --- Device state (ports + PWM) ---
	cmd = SVB_CMD_READ_STATE;
	if (svbpb_command(device, &cmd, 1, res, 10)) {
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "State: DC1=%02X DC2=%02X DC3=%02X DC4=%02X DC5=%02X USBA=%02X USBB=%02X REG=%02X PWM1=%02X PWM2=%02X",
			res[0], res[1], res[2], res[3], res[4], res[5], res[6], res[7], res[8], res[9]);

		// Power outlets (DC1-DC5) + regulated
		if (AUX_POWER_OUTLET_PROPERTY->state != INDIGO_BUSY_STATE) {
			AUX_POWER_OUTLET_1_ITEM->sw.value = (res[0] != 0);
			AUX_POWER_OUTLET_2_ITEM->sw.value = (res[1] != 0);
			AUX_POWER_OUTLET_3_ITEM->sw.value = (res[2] != 0);
			AUX_POWER_OUTLET_4_ITEM->sw.value = (res[3] != 0);
			AUX_POWER_OUTLET_5_ITEM->sw.value = (res[4] != 0);
			AUX_POWER_OUTLET_6_ITEM->sw.value = (res[7] != 0);
			indigo_update_property(device, AUX_POWER_OUTLET_PROPERTY, NULL);
		}

		// Regulated output voltage
		if (AUX_POWER_OUTLET_VOLTAGE_PROPERTY->state != INDIGO_BUSY_STATE) {
			double reg_voltage = round((res[7] * SVB_MAX_VOLTAGE / 255.0) * 10.0) / 10.0;
			AUX_POWER_OUTLET_VOLTAGE_1_ITEM->number.value = reg_voltage;
			indigo_update_property(device, AUX_POWER_OUTLET_VOLTAGE_PROPERTY, NULL);
		}

		// USB ports
		if (AUX_USB_PORT_PROPERTY->state != INDIGO_BUSY_STATE) {
			AUX_USB_PORT_1_ITEM->sw.value = (res[5] != 0);
			AUX_USB_PORT_2_ITEM->sw.value = (res[6] != 0);
			indigo_update_property(device, AUX_USB_PORT_PROPERTY, NULL);
		}

		// Heater outlets (PWM)
		if (AUX_HEATER_OUTLET_PROPERTY->state != INDIGO_BUSY_STATE) {
			AUX_HEATER_OUTLET_1_ITEM->number.value = round(100.0 * (res[8] / 255.0));
			AUX_HEATER_OUTLET_2_ITEM->number.value = round(100.0 * (res[9] / 255.0));
			indigo_update_property(device, AUX_HEATER_OUTLET_PROPERTY, NULL);
		}

		// Auto dew control
		if (AUX_DEW_CONTROL_AUTOMATIC_ITEM->sw.value && AUX_WEATHER_PROPERTY->state == INDIGO_OK_STATE) {
			double dewpoint = AUX_WEATHER_DEWPOINT_ITEM->number.value;
			if (
				((dewpoint + 1) > sht40_temp) &&
				(res[8] != 255 || res[9] != 255)
			) {
				svbpb_set_port(device, SVB_PORT_PWM_A, 255);
				svbpb_set_port(device, SVB_PORT_PWM_B, 255);
				indigo_send_message(device, "Heating started: Approaching dewpoint");
			}
			if (
				((dewpoint + 2) < sht40_temp) &&
				(res[8] != 0 || res[9] != 0)
			) {
				svbpb_set_port(device, SVB_PORT_PWM_A, 0);
				svbpb_set_port(device, SVB_PORT_PWM_B, 0);
				indigo_send_message(device, "Heating stopped: Conditions are dry");
			}
		}
	}

	// Dew warning
	if (!has_sht40_temp) {
		AUX_DEW_WARNING_PROPERTY->state = INDIGO_IDLE_STATE;
		AUX_DEW_WARNING_SENSOR_1_ITEM->light.value = INDIGO_IDLE_STATE;
	} else if (sht40_temp - 1 <= AUX_WEATHER_DEWPOINT_ITEM->number.value) {
		AUX_DEW_WARNING_PROPERTY->state = INDIGO_OK_STATE;
		AUX_DEW_WARNING_SENSOR_1_ITEM->light.value = INDIGO_ALERT_STATE;
	} else {
		AUX_DEW_WARNING_PROPERTY->state = INDIGO_OK_STATE;
		AUX_DEW_WARNING_SENSOR_1_ITEM->light.value = INDIGO_OK_STATE;
	}
	indigo_update_property(device, AUX_DEW_WARNING_PROPERTY, NULL);
}

static void aux_timer_callback(indigo_device *device) {
	if (!IS_CONNECTED)
		return;
	pthread_mutex_lock(&PRIVATE_DATA->mutex);
	aux_update_states(device);
	indigo_reschedule_timer(device, 1, &PRIVATE_DATA->aux_timer);
	pthread_mutex_unlock(&PRIVATE_DATA->mutex);
}

static void aux_connection_handler(indigo_device *device) {
	pthread_mutex_lock(&PRIVATE_DATA->mutex);
	if (CONNECTION_CONNECTED_ITEM->sw.value) {
		PRIVATE_DATA->handle = indigo_open_serial_with_speed(DEVICE_PORT_ITEM->text.value, 115200);
		if (PRIVATE_DATA->handle > 0) {
			// Reset the ESP32 by clearing DTR/RTS
			int flags = 0;
			ioctl(PRIVATE_DATA->handle, TIOCMGET, &flags);
			flags &= ~(TIOCM_RTS | TIOCM_DTR);
			ioctl(PRIVATE_DATA->handle, TIOCMSET, &flags);

			// Wait for boot messages to clear (up to 3 seconds)
			INDIGO_DRIVER_DEBUG(DRIVER_NAME, "Waiting for device boot...");
			char boot_buf[512];
			bool booting = true;
			int retry = 0;
			while (booting && retry < 10) {
				fd_set rfds;
				struct timeval tv = { 0, 300000 }; // 300 ms
				FD_ZERO(&rfds);
				FD_SET(PRIVATE_DATA->handle, &rfds);
				if (select(PRIVATE_DATA->handle + 1, &rfds, NULL, NULL, &tv) > 0) {
					int n = (int)read(PRIVATE_DATA->handle, boot_buf, sizeof(boot_buf) - 1);
					if (n > 0) {
						boot_buf[n] = '\0';
						INDIGO_DRIVER_DEBUG(DRIVER_NAME, "Boot data: %s", boot_buf);
						// Keep reading while ESP32 sends boot messages
						retry = 0;
						continue;
					}
				}
				retry++;
			}
			tcflush(PRIVATE_DATA->handle, TCIOFLUSH);

			// Handshake: request device state and check it responds
			unsigned char cmd = SVB_CMD_READ_STATE;
			unsigned char state[10] = {0};
			if (!svbpb_command(device, &cmd, 1, state, 10)) {
				INDIGO_DRIVER_ERROR(DRIVER_NAME, "Device handshake failed for %s", DEVICE_PORT_ITEM->text.value);
				close(PRIVATE_DATA->handle);
				PRIVATE_DATA->handle = 0;
			} else {
				INDIGO_DRIVER_DEBUG(DRIVER_NAME, "Device handshake successful");
				strcpy(INFO_DEVICE_MODEL_ITEM->text.value, "SVBONY PowerBox");
				strcpy(INFO_DEVICE_FW_REVISION_ITEM->text.value, "Unknown");
				indigo_update_property(device, INFO_PROPERTY, NULL);
			}
		}

		if (PRIVATE_DATA->handle > 0) {
			indigo_define_property(device, AUX_POWER_OUTLET_PROPERTY, NULL);
			indigo_define_property(device, AUX_POWER_OUTLET_VOLTAGE_PROPERTY, NULL);
			indigo_define_property(device, AUX_POWER_OUTLET_CURRENT_PROPERTY, NULL);
			indigo_define_property(device, AUX_USB_PORT_PROPERTY, NULL);
			indigo_define_property(device, AUX_HEATER_OUTLET_PROPERTY, NULL);
			indigo_define_property(device, AUX_DEW_CONTROL_PROPERTY, NULL);
			indigo_define_property(device, AUX_WEATHER_PROPERTY, NULL);
			indigo_define_property(device, AUX_TEMPERATURE_SENSORS_PROPERTY, NULL);
			indigo_define_property(device, AUX_DEW_WARNING_PROPERTY, NULL);
			indigo_define_property(device, AUX_INFO_PROPERTY, NULL);
			indigo_set_timer(device, 0, aux_timer_callback, &PRIVATE_DATA->aux_timer);
			CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
		} else {
			INDIGO_DRIVER_ERROR(DRIVER_NAME, "Failed to connect to %s", DEVICE_PORT_ITEM->text.value);
			CONNECTION_PROPERTY->state = INDIGO_ALERT_STATE;
			indigo_set_switch(CONNECTION_PROPERTY, CONNECTION_DISCONNECTED_ITEM, true);
		}
	} else {
		indigo_cancel_timer_sync(device, &PRIVATE_DATA->aux_timer);
		indigo_delete_property(device, AUX_POWER_OUTLET_PROPERTY, NULL);
		indigo_delete_property(device, AUX_POWER_OUTLET_VOLTAGE_PROPERTY, NULL);
		indigo_delete_property(device, AUX_POWER_OUTLET_CURRENT_PROPERTY, NULL);
		indigo_delete_property(device, AUX_USB_PORT_PROPERTY, NULL);
		indigo_delete_property(device, AUX_HEATER_OUTLET_PROPERTY, NULL);
		indigo_delete_property(device, AUX_DEW_CONTROL_PROPERTY, NULL);
		indigo_delete_property(device, AUX_WEATHER_PROPERTY, NULL);
		indigo_delete_property(device, AUX_TEMPERATURE_SENSORS_PROPERTY, NULL);
		indigo_delete_property(device, AUX_DEW_WARNING_PROPERTY, NULL);
		indigo_delete_property(device, AUX_INFO_PROPERTY, NULL);
		strcpy(INFO_DEVICE_MODEL_ITEM->text.value, "Unknown");
		strcpy(INFO_DEVICE_FW_REVISION_ITEM->text.value, "Unknown");
		indigo_update_property(device, INFO_PROPERTY, NULL);

		if (PRIVATE_DATA->handle > 0) {
			INDIGO_DRIVER_DEBUG(DRIVER_NAME, "Disconnected");
			close(PRIVATE_DATA->handle);
			PRIVATE_DATA->handle = 0;
		}
		CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
	}
	indigo_aux_change_property(device, NULL, CONNECTION_PROPERTY);
	pthread_mutex_unlock(&PRIVATE_DATA->mutex);
}

static void aux_power_outlet_handler(indigo_device *device) {
	pthread_mutex_lock(&PRIVATE_DATA->mutex);
	svbpb_set_port(device, SVB_PORT_DC1, AUX_POWER_OUTLET_1_ITEM->sw.value ? 0xFF : 0x00);
	svbpb_set_port(device, SVB_PORT_DC2, AUX_POWER_OUTLET_2_ITEM->sw.value ? 0xFF : 0x00);
	svbpb_set_port(device, SVB_PORT_DC3, AUX_POWER_OUTLET_3_ITEM->sw.value ? 0xFF : 0x00);
	svbpb_set_port(device, SVB_PORT_DC4, AUX_POWER_OUTLET_4_ITEM->sw.value ? 0xFF : 0x00);
	svbpb_set_port(device, SVB_PORT_DC5, AUX_POWER_OUTLET_5_ITEM->sw.value ? 0xFF : 0x00);
	// Regulated output: if outlet 6 is OFF, set voltage to 0; otherwise keep target voltage
	if (!AUX_POWER_OUTLET_6_ITEM->sw.value) {
		svbpb_set_port(device, SVB_PORT_REGULATED, 0x00);
	} else {
		uint8_t pwm = (uint8_t)round((AUX_POWER_OUTLET_VOLTAGE_1_ITEM->number.target / SVB_MAX_VOLTAGE) * 255.0);
		svbpb_set_port(device, SVB_PORT_REGULATED, pwm);
	}
	indigo_usleep(ONE_SECOND_DELAY / 2);
	AUX_POWER_OUTLET_PROPERTY->state = INDIGO_OK_STATE;
	indigo_update_property(device, AUX_POWER_OUTLET_PROPERTY, NULL);
	pthread_mutex_unlock(&PRIVATE_DATA->mutex);
}

static void aux_usb_port_handler(indigo_device *device) {
	pthread_mutex_lock(&PRIVATE_DATA->mutex);
	svbpb_set_port(device, SVB_PORT_USB_A, AUX_USB_PORT_1_ITEM->sw.value ? 0xFF : 0x00);
	svbpb_set_port(device, SVB_PORT_USB_B, AUX_USB_PORT_2_ITEM->sw.value ? 0xFF : 0x00);
	indigo_usleep(ONE_SECOND_DELAY / 2);
	AUX_USB_PORT_PROPERTY->state = INDIGO_OK_STATE;
	indigo_update_property(device, AUX_USB_PORT_PROPERTY, NULL);
	pthread_mutex_unlock(&PRIVATE_DATA->mutex);
}

static void aux_power_outlet_voltage_handler(indigo_device *device) {
	pthread_mutex_lock(&PRIVATE_DATA->mutex);
	uint8_t pwm = (uint8_t)round((AUX_POWER_OUTLET_VOLTAGE_1_ITEM->number.target / SVB_MAX_VOLTAGE) * 255.0);
	if (pwm > 255) pwm = 255;
	svbpb_set_port(device, SVB_PORT_REGULATED, pwm);
	indigo_usleep(ONE_SECOND_DELAY / 2);
	AUX_POWER_OUTLET_VOLTAGE_PROPERTY->state = INDIGO_OK_STATE;
	indigo_update_property(device, AUX_POWER_OUTLET_VOLTAGE_PROPERTY, NULL);
	pthread_mutex_unlock(&PRIVATE_DATA->mutex);
}

static void aux_heater_outlet_handler(indigo_device *device) {
	pthread_mutex_lock(&PRIVATE_DATA->mutex);
	uint8_t pwm1 = (uint8_t)round(AUX_HEATER_OUTLET_1_ITEM->number.target * 255.0 / 100.0);
	uint8_t pwm2 = (uint8_t)round(AUX_HEATER_OUTLET_2_ITEM->number.target * 255.0 / 100.0);
	svbpb_set_port(device, SVB_PORT_PWM_A, pwm1);
	svbpb_set_port(device, SVB_PORT_PWM_B, pwm2);
	indigo_usleep(ONE_SECOND_DELAY / 2);
	AUX_HEATER_OUTLET_PROPERTY->state = INDIGO_OK_STATE;
	indigo_update_property(device, AUX_HEATER_OUTLET_PROPERTY, NULL);
	pthread_mutex_unlock(&PRIVATE_DATA->mutex);
}

static indigo_result aux_change_property(indigo_device *device, indigo_client *client, indigo_property *property) {
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
		indigo_set_timer(device, 0, aux_connection_handler, NULL);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(AUX_OUTLET_NAMES_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- AUX_OUTLET_NAMES
		indigo_property_copy_values(AUX_OUTLET_NAMES_PROPERTY, property, false);
		snprintf(AUX_POWER_OUTLET_1_ITEM->label,           INDIGO_NAME_SIZE, "%s", AUX_POWER_OUTLET_NAME_1_ITEM->text.value);
		snprintf(AUX_POWER_OUTLET_2_ITEM->label,           INDIGO_NAME_SIZE, "%s", AUX_POWER_OUTLET_NAME_2_ITEM->text.value);
		snprintf(AUX_POWER_OUTLET_3_ITEM->label,           INDIGO_NAME_SIZE, "%s", AUX_POWER_OUTLET_NAME_3_ITEM->text.value);
		snprintf(AUX_POWER_OUTLET_4_ITEM->label,           INDIGO_NAME_SIZE, "%s", AUX_POWER_OUTLET_NAME_4_ITEM->text.value);
		snprintf(AUX_POWER_OUTLET_5_ITEM->label,           INDIGO_NAME_SIZE, "%s", AUX_POWER_OUTLET_NAME_5_ITEM->text.value);
		snprintf(AUX_POWER_OUTLET_6_ITEM->label,           INDIGO_NAME_SIZE, "%s", AUX_POWER_OUTLET_NAME_6_ITEM->text.value);
		snprintf(AUX_POWER_OUTLET_VOLTAGE_1_ITEM->label,   INDIGO_NAME_SIZE, "%s [V]", AUX_POWER_OUTLET_NAME_6_ITEM->text.value);
		snprintf(AUX_HEATER_OUTLET_1_ITEM->label,          INDIGO_NAME_SIZE, "%s [%%]", AUX_HEATER_OUTLET_NAME_1_ITEM->text.value);
		snprintf(AUX_HEATER_OUTLET_2_ITEM->label,          INDIGO_NAME_SIZE, "%s [%%]", AUX_HEATER_OUTLET_NAME_2_ITEM->text.value);
		AUX_OUTLET_NAMES_PROPERTY->state = INDIGO_OK_STATE;
		if (IS_CONNECTED) {
			indigo_delete_property(device, AUX_POWER_OUTLET_PROPERTY, NULL);
			indigo_define_property(device, AUX_POWER_OUTLET_PROPERTY, NULL);
			indigo_delete_property(device, AUX_POWER_OUTLET_VOLTAGE_PROPERTY, NULL);
			indigo_define_property(device, AUX_POWER_OUTLET_VOLTAGE_PROPERTY, NULL);
			indigo_delete_property(device, AUX_HEATER_OUTLET_PROPERTY, NULL);
			indigo_define_property(device, AUX_HEATER_OUTLET_PROPERTY, NULL);
		}
		indigo_update_property(device, AUX_OUTLET_NAMES_PROPERTY, NULL);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(AUX_POWER_OUTLET_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- AUX_POWER_OUTLET
		AUX_POWER_OUTLET_PROPERTY->state = INDIGO_BUSY_STATE;
		indigo_property_copy_values(AUX_POWER_OUTLET_PROPERTY, property, false);
		indigo_update_property(device, AUX_POWER_OUTLET_PROPERTY, NULL);
		indigo_set_timer(device, 0, aux_power_outlet_handler, NULL);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(AUX_POWER_OUTLET_VOLTAGE_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- AUX_POWER_OUTLET_VOLTAGE
		AUX_POWER_OUTLET_VOLTAGE_PROPERTY->state = INDIGO_BUSY_STATE;
		indigo_property_copy_values(AUX_POWER_OUTLET_VOLTAGE_PROPERTY, property, false);
		indigo_update_property(device, AUX_POWER_OUTLET_VOLTAGE_PROPERTY, NULL);
		indigo_set_timer(device, 0, aux_power_outlet_voltage_handler, NULL);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(AUX_USB_PORT_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- AUX_USB_PORT
		AUX_USB_PORT_PROPERTY->state = INDIGO_BUSY_STATE;
		indigo_property_copy_values(AUX_USB_PORT_PROPERTY, property, false);
		indigo_update_property(device, AUX_USB_PORT_PROPERTY, NULL);
		indigo_set_timer(device, 0, aux_usb_port_handler, NULL);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(AUX_HEATER_OUTLET_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- AUX_HEATER_OUTLET
		AUX_HEATER_OUTLET_PROPERTY->state = INDIGO_BUSY_STATE;
		indigo_property_copy_values(AUX_HEATER_OUTLET_PROPERTY, property, false);
		indigo_update_property(device, AUX_HEATER_OUTLET_PROPERTY, NULL);
		indigo_set_timer(device, 0, aux_heater_outlet_handler, NULL);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(AUX_DEW_CONTROL_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- AUX_DEW_CONTROL
		indigo_property_copy_values(AUX_DEW_CONTROL_PROPERTY, property, false);
		AUX_DEW_CONTROL_PROPERTY->state = INDIGO_OK_STATE;
		indigo_update_property(device, AUX_DEW_CONTROL_PROPERTY, NULL);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(CONFIG_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- CONFIG
		if (indigo_switch_match(CONFIG_SAVE_ITEM, property)) {
			indigo_save_property(device, NULL, AUX_OUTLET_NAMES_PROPERTY);
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
	indigo_release_property(AUX_POWER_OUTLET_PROPERTY);
	indigo_release_property(AUX_POWER_OUTLET_VOLTAGE_PROPERTY);
	indigo_release_property(AUX_POWER_OUTLET_CURRENT_PROPERTY);
	indigo_release_property(AUX_USB_PORT_PROPERTY);
	indigo_release_property(AUX_HEATER_OUTLET_PROPERTY);
	indigo_release_property(AUX_DEW_CONTROL_PROPERTY);
	indigo_release_property(AUX_WEATHER_PROPERTY);
	indigo_release_property(AUX_TEMPERATURE_SENSORS_PROPERTY);
	indigo_release_property(AUX_DEW_WARNING_PROPERTY);
	indigo_release_property(AUX_INFO_PROPERTY);
	indigo_release_property(AUX_OUTLET_NAMES_PROPERTY);
	pthread_mutex_destroy(&PRIVATE_DATA->mutex);
	INDIGO_DEVICE_DETACH_LOG(DRIVER_NAME, device->name);
	return indigo_aux_detach(device);
}

// -------------------------------------------------------------------------------- INDIGO driver implementation

indigo_result indigo_aux_svbpowerbox(indigo_driver_action action, indigo_driver_info *info) {
	static indigo_driver_action last_action = INDIGO_DRIVER_SHUTDOWN;
	static svbpb_private_data *private_data = NULL;
	static indigo_device *aux = NULL;

	static indigo_device aux_template = INDIGO_DEVICE_INITIALIZER(
		"SVBONY PowerBox",
		aux_attach,
		aux_enumerate_properties,
		aux_change_property,
		NULL,
		aux_detach
	);

	SET_DRIVER_INFO(info, "SVBONY PowerBox", __FUNCTION__, DRIVER_VERSION, false, last_action);

	if (action == last_action)
		return INDIGO_OK;

	switch (action) {
		case INDIGO_DRIVER_INIT:
			last_action = action;
			private_data = indigo_safe_malloc(sizeof(svbpb_private_data));
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
