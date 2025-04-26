// Copyright (c) 2021 Rumen G. Bogdanovski
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
// 2.0 by Rumen G.Bogdanovski <rumenastro@gmail.com>

/** INDIGO ASCOM ALPACA bridge agent
 \file alpaca_switch.c
 */

#include <indigo/indigo_aux_driver.h>

#include "alpaca_common.h"

static int get_switch_number(indigo_alpaca_device *device) {
	return device->sw.maxswitch_power_outlet + device->sw.maxswitch_heater_outlet + device->sw.maxswitch_usb_port + device->sw.maxswitch_gpio_outlet + device->sw.maxswitch_gpio_sensor;
}

static indigo_alpaca_error alpaca_get_interfaceversion(indigo_alpaca_device *device, int version, int *value) {
	*value = 2;
	return indigo_alpaca_error_OK;
}

static indigo_alpaca_error alpaca_get_maxswitch(indigo_alpaca_device *device, int version, int *value) {
	pthread_mutex_lock(&device->mutex);
	if (!device->connected) {
		pthread_mutex_unlock(&device->mutex);
		return indigo_alpaca_error_NotConnected;
	}
	*value = get_switch_number(device);
	pthread_mutex_unlock(&device->mutex);
	return indigo_alpaca_error_OK;
}

static indigo_alpaca_error alpaca_get_canwrite(indigo_alpaca_device *device, int version, int id, bool *value) {
	pthread_mutex_lock(&device->mutex);
	if (!device->connected) {
		pthread_mutex_unlock(&device->mutex);
		return indigo_alpaca_error_NotConnected;
	}
	if (id < 0 || id >= get_switch_number(device)) {
		pthread_mutex_unlock(&device->mutex);
		return indigo_alpaca_error_InvalidValue;
	}
	if (id < device->sw.maxswitch_power_outlet) {
		*value = device->sw.canwrite[0 * ALPACA_MAX_SWITCHES + id];
		pthread_mutex_unlock(&device->mutex);
		return indigo_alpaca_error_OK;
	}
	id -= device->sw.maxswitch_power_outlet;
	if (id < device->sw.maxswitch_heater_outlet) {
		*value = device->sw.canwrite[1 * ALPACA_MAX_SWITCHES + id];
		pthread_mutex_unlock(&device->mutex);
		return indigo_alpaca_error_OK;
	}
	id -= device->sw.maxswitch_heater_outlet;
	if (id < device->sw.maxswitch_usb_port) {
		*value = device->sw.canwrite[2 * ALPACA_MAX_SWITCHES + id];
		pthread_mutex_unlock(&device->mutex);
		return indigo_alpaca_error_OK;
	}
	id -= device->sw.maxswitch_usb_port;
	if (id < device->sw.maxswitch_gpio_outlet) {
		*value = device->sw.canwrite[3 * ALPACA_MAX_SWITCHES + id];
		pthread_mutex_unlock(&device->mutex);
		return indigo_alpaca_error_OK;
	}
	id -= device->sw.maxswitch_gpio_outlet;
	*value = device->sw.canwrite[4 * ALPACA_MAX_SWITCHES + id];
	pthread_mutex_unlock(&device->mutex);
	return indigo_alpaca_error_OK;
}

static indigo_alpaca_error alpaca_get_minswitchvalue(indigo_alpaca_device *device, int version, int id, double *value) {
	pthread_mutex_lock(&device->mutex);
	if (!device->connected) {
		pthread_mutex_unlock(&device->mutex);
		return indigo_alpaca_error_NotConnected;
	}
	if (id < 0 || id >= get_switch_number(device)) {
		pthread_mutex_unlock(&device->mutex);
		return indigo_alpaca_error_InvalidValue;
	}
	if (id < device->sw.maxswitch_power_outlet) {
		*value = device->sw.minswitchvalue[0 * ALPACA_MAX_SWITCHES + id];
		pthread_mutex_unlock(&device->mutex);
		return indigo_alpaca_error_OK;
	}
	id -= device->sw.maxswitch_power_outlet;
	if (id < device->sw.maxswitch_heater_outlet) {
		*value = device->sw.minswitchvalue[1 * ALPACA_MAX_SWITCHES + id];
		pthread_mutex_unlock(&device->mutex);
		return indigo_alpaca_error_OK;
	}
	id -= device->sw.maxswitch_heater_outlet;
	if (id < device->sw.maxswitch_usb_port) {
		*value = device->sw.minswitchvalue[2 * ALPACA_MAX_SWITCHES + id];
		pthread_mutex_unlock(&device->mutex);
		return indigo_alpaca_error_OK;
	}
	id -= device->sw.maxswitch_usb_port;
	if (id < device->sw.maxswitch_gpio_outlet) {
		*value = device->sw.minswitchvalue[3 * ALPACA_MAX_SWITCHES + id];
		pthread_mutex_unlock(&device->mutex);
		return indigo_alpaca_error_OK;
	}
	id -= device->sw.maxswitch_gpio_outlet;
	*value = device->sw.minswitchvalue[4 * ALPACA_MAX_SWITCHES + id];
	pthread_mutex_unlock(&device->mutex);
	return indigo_alpaca_error_OK;
}

static indigo_alpaca_error alpaca_get_maxswitchvalue(indigo_alpaca_device *device, int version, int id, double *value) {
	pthread_mutex_lock(&device->mutex);
	if (!device->connected) {
		pthread_mutex_unlock(&device->mutex);
		return indigo_alpaca_error_NotConnected;
	}
	if (id < 0 || id >= get_switch_number(device)) {
		pthread_mutex_unlock(&device->mutex);
		return indigo_alpaca_error_InvalidValue;
	}
	if (id < device->sw.maxswitch_power_outlet) {
		*value = device->sw.maxswitchvalue[0 * ALPACA_MAX_SWITCHES + id];
		pthread_mutex_unlock(&device->mutex);
		return indigo_alpaca_error_OK;
	}
	id -= device->sw.maxswitch_power_outlet;
	if (id < device->sw.maxswitch_heater_outlet) {
		*value = device->sw.maxswitchvalue[1 * ALPACA_MAX_SWITCHES + id];
		pthread_mutex_unlock(&device->mutex);
		return indigo_alpaca_error_OK;
	}
	id -= device->sw.maxswitch_heater_outlet;
	if (id < device->sw.maxswitch_usb_port) {
		*value = device->sw.maxswitchvalue[2 * ALPACA_MAX_SWITCHES + id];
		pthread_mutex_unlock(&device->mutex);
		return indigo_alpaca_error_OK;
	}
	id -= device->sw.maxswitch_usb_port;
	if (id < device->sw.maxswitch_gpio_outlet) {
		*value = device->sw.maxswitchvalue[3 * ALPACA_MAX_SWITCHES + id];
		pthread_mutex_unlock(&device->mutex);
		return indigo_alpaca_error_OK;
	}
	id -= device->sw.maxswitch_gpio_outlet;
	*value = device->sw.maxswitchvalue[4 * ALPACA_MAX_SWITCHES + id];
	pthread_mutex_unlock(&device->mutex);
	return indigo_alpaca_error_OK;
}

static indigo_alpaca_error alpaca_get_switchstep(indigo_alpaca_device *device, int version, int id, double *value) {
	pthread_mutex_lock(&device->mutex);
	if (!device->connected) {
		pthread_mutex_unlock(&device->mutex);
		return indigo_alpaca_error_NotConnected;
	}
	if (id < 0 || id >= get_switch_number(device)) {
		pthread_mutex_unlock(&device->mutex);
		return indigo_alpaca_error_InvalidValue;
	}
	if (id < device->sw.maxswitch_power_outlet) {
		*value = device->sw.switchstep[0 * ALPACA_MAX_SWITCHES + id];
		pthread_mutex_unlock(&device->mutex);
		return indigo_alpaca_error_OK;
	}
	id -= device->sw.maxswitch_power_outlet;
	if (id < device->sw.maxswitch_heater_outlet) {
		*value = device->sw.switchstep[1 * ALPACA_MAX_SWITCHES + id];
		pthread_mutex_unlock(&device->mutex);
		return indigo_alpaca_error_OK;
	}
	id -= device->sw.maxswitch_heater_outlet;
	if (id < device->sw.maxswitch_usb_port) {
		*value = device->sw.switchstep[2 * ALPACA_MAX_SWITCHES + id];
		pthread_mutex_unlock(&device->mutex);
		return indigo_alpaca_error_OK;
	}
	id -= device->sw.maxswitch_usb_port;
	if (id < device->sw.maxswitch_gpio_outlet) {
		*value = device->sw.switchstep[3 * ALPACA_MAX_SWITCHES + id];
		pthread_mutex_unlock(&device->mutex);
		return indigo_alpaca_error_OK;
	}
	id -= device->sw.maxswitch_gpio_outlet;
	*value = device->sw.switchstep[4 * ALPACA_MAX_SWITCHES + id];
	pthread_mutex_unlock(&device->mutex);
	return indigo_alpaca_error_OK;
}

static indigo_alpaca_error alpaca_get_switchvalue(indigo_alpaca_device *device, int version, int id, double *value) {
	pthread_mutex_lock(&device->mutex);
	if (!device->connected) {
		pthread_mutex_unlock(&device->mutex);
		return indigo_alpaca_error_NotConnected;
	}
	if (id < 0 || id >= get_switch_number(device)) {
		pthread_mutex_unlock(&device->mutex);
		return indigo_alpaca_error_InvalidValue;
	}
	if (id < device->sw.maxswitch_power_outlet) {
		*value = device->sw.switchvalue[0 * ALPACA_MAX_SWITCHES + id];
		pthread_mutex_unlock(&device->mutex);
		return indigo_alpaca_error_OK;
	}
	id -= device->sw.maxswitch_power_outlet;
	if (id < device->sw.maxswitch_heater_outlet) {
		*value = device->sw.switchvalue[1 * ALPACA_MAX_SWITCHES + id];
		pthread_mutex_unlock(&device->mutex);
		return indigo_alpaca_error_OK;
	}
	id -= device->sw.maxswitch_heater_outlet;
	if (id < device->sw.maxswitch_usb_port) {
		*value = device->sw.switchvalue[2 * ALPACA_MAX_SWITCHES + id];
		pthread_mutex_unlock(&device->mutex);
		return indigo_alpaca_error_OK;
	}
	id -= device->sw.maxswitch_usb_port;
	if (id < device->sw.maxswitch_gpio_outlet) {
		*value = device->sw.switchvalue[3 * ALPACA_MAX_SWITCHES + id];
		pthread_mutex_unlock(&device->mutex);
		return indigo_alpaca_error_OK;
	}
	id -= device->sw.maxswitch_gpio_outlet;
	*value = device->sw.switchvalue[4 * ALPACA_MAX_SWITCHES + id];
	pthread_mutex_unlock(&device->mutex);
	return indigo_alpaca_error_OK;
}

static indigo_alpaca_error alpaca_get_switch(indigo_alpaca_device *device, int version, int id, bool *value) {
	pthread_mutex_lock(&device->mutex);
	if (!device->connected) {
		pthread_mutex_unlock(&device->mutex);
		return indigo_alpaca_error_NotConnected;
	}
	if (id < 0 || id >= get_switch_number(device)) {
		pthread_mutex_unlock(&device->mutex);
		return indigo_alpaca_error_InvalidValue;
	}
	if (id < device->sw.maxswitch_power_outlet) {
		*value = device->sw.switchvalue[0 * ALPACA_MAX_SWITCHES + id] == device->sw.maxswitchvalue[0 * ALPACA_MAX_SWITCHES + id];
		pthread_mutex_unlock(&device->mutex);
		return indigo_alpaca_error_OK;
	}
	id -= device->sw.maxswitch_power_outlet;
	if (id < device->sw.maxswitch_heater_outlet) {
		*value = device->sw.switchvalue[1 * ALPACA_MAX_SWITCHES + id] == device->sw.maxswitchvalue[1 * ALPACA_MAX_SWITCHES + id];
		pthread_mutex_unlock(&device->mutex);
		return indigo_alpaca_error_OK;
	}
	id -= device->sw.maxswitch_heater_outlet;
	if (id < device->sw.maxswitch_usb_port) {
		*value = device->sw.switchvalue[2 * ALPACA_MAX_SWITCHES + id] == device->sw.maxswitchvalue[2 * ALPACA_MAX_SWITCHES + id];
		pthread_mutex_unlock(&device->mutex);
		return indigo_alpaca_error_OK;
	}
	id -= device->sw.maxswitch_usb_port;
	if (id < device->sw.maxswitch_gpio_outlet) {
		*value = device->sw.switchvalue[3 * ALPACA_MAX_SWITCHES + id] == device->sw.maxswitchvalue[3 * ALPACA_MAX_SWITCHES + id];
		pthread_mutex_unlock(&device->mutex);
		return indigo_alpaca_error_OK;
	}
	id -= device->sw.maxswitch_gpio_outlet;
	*value = device->sw.switchvalue[4 * ALPACA_MAX_SWITCHES + id] == device->sw.maxswitchvalue[4 * ALPACA_MAX_SWITCHES + id];
	pthread_mutex_unlock(&device->mutex);
	return indigo_alpaca_error_OK;
}

static indigo_alpaca_error alpaca_get_switchname(indigo_alpaca_device *device, int version, int id, char **value) {
	pthread_mutex_lock(&device->mutex);
	if (!device->connected) {
		pthread_mutex_unlock(&device->mutex);
		return indigo_alpaca_error_NotConnected;
	}
	if (id < 0 || id >= get_switch_number(device)) {
		pthread_mutex_unlock(&device->mutex);
		return indigo_alpaca_error_InvalidValue;
	}
	if (id < device->sw.maxswitch_power_outlet + device->sw.maxswitch_heater_outlet + device->sw.maxswitch_usb_port + device->sw.maxswitch_gpio_outlet) {
		*value = device->sw.switchname[0 * ALPACA_MAX_SWITCHES + id];
		pthread_mutex_unlock(&device->mutex);
		return indigo_alpaca_error_OK;
	}
	id -= device->sw.maxswitch_power_outlet + device->sw.maxswitch_heater_outlet + device->sw.maxswitch_usb_port + device->sw.maxswitch_gpio_outlet;
	*value = device->sw.switchname[4 * ALPACA_MAX_SWITCHES + id];
	pthread_mutex_unlock(&device->mutex);
	return indigo_alpaca_error_OK;
}

static indigo_alpaca_error alpaca_set_setswitch(indigo_alpaca_device *device, int version, int id, bool value) {
	bool canwrite = false;
	alpaca_get_canwrite(device, version, id, &canwrite);
	pthread_mutex_lock(&device->mutex);
	if (!device->connected) {
		pthread_mutex_unlock(&device->mutex);
		return indigo_alpaca_error_NotConnected;
	}
	int switch_number = get_switch_number(device);
	if (!canwrite && id >= 0 && id < switch_number) {
		pthread_mutex_unlock(&device->mutex);
		return indigo_alpaca_error_NotImplemented;
	}
	if (id < 0 || id >= switch_number) {
		pthread_mutex_unlock(&device->mutex);
		return indigo_alpaca_error_InvalidValue;
	}
	char name[INDIGO_NAME_SIZE];
	if (id < device->sw.maxswitch_power_outlet) {
		sprintf(name, "OUTLET_%d", id + 1);
		device->sw.valueset[0] = false;
		indigo_change_switch_property_1(indigo_agent_alpaca_client, device->indigo_device, AUX_POWER_OUTLET_PROPERTY_NAME, name, value);
		pthread_mutex_unlock(&device->mutex);
		return indigo_alpaca_wait_for_bool(&device->sw.valueset[0], true, 30);
	}
	id -= device->sw.maxswitch_power_outlet;
	if (id < device->sw.maxswitch_heater_outlet) {
		sprintf(name, "OUTLET_%d", id + 1);
		device->sw.valueset[1] = false;
		indigo_change_number_property_1(indigo_agent_alpaca_client, device->indigo_device, AUX_HEATER_OUTLET_PROPERTY_NAME, name, value ? 100 : 0);
		pthread_mutex_unlock(&device->mutex);
		return indigo_alpaca_wait_for_bool(&device->sw.valueset[1], true, 30);
	}
	id -= device->sw.maxswitch_heater_outlet;
	if (id < device->sw.maxswitch_usb_port) {
		sprintf(name, "PORT_%d", id + 1);
		device->sw.valueset[2] = false;
		indigo_change_switch_property_1(indigo_agent_alpaca_client, device->indigo_device, AUX_USB_PORT_PROPERTY_NAME, name, value);
		pthread_mutex_unlock(&device->mutex);
		return indigo_alpaca_wait_for_bool(&device->sw.valueset[2], true, 30);
	}
	id -= device->sw.maxswitch_usb_port;
	sprintf(name, "OUTLET_%d", id + 1);
	device->sw.valueset[3] = false;
	indigo_change_switch_property_1(indigo_agent_alpaca_client, device->indigo_device, AUX_GPIO_OUTLETS_PROPERTY_NAME, name, value);
	pthread_mutex_unlock(&device->mutex);
	return indigo_alpaca_wait_for_bool(&device->sw.valueset[3], true, 30);
}

static indigo_alpaca_error alpaca_set_setswitchvalue(indigo_alpaca_device *device, int version, int id, double value) {
	bool canwrite = false;
	alpaca_get_canwrite(device, version, id, &canwrite);
	pthread_mutex_lock(&device->mutex);
	if (!device->connected) {
		pthread_mutex_unlock(&device->mutex);
		return indigo_alpaca_error_NotConnected;
	}
	int switch_number = get_switch_number(device);
	if (!canwrite && id >= 0 && id < switch_number) {
		pthread_mutex_unlock(&device->mutex);
		return indigo_alpaca_error_NotImplemented;
	}
	if (id < 0 || id >= switch_number) {
		pthread_mutex_unlock(&device->mutex);
		return indigo_alpaca_error_InvalidValue;
	}
	char name[INDIGO_NAME_SIZE];
	if (id < device->sw.maxswitch_power_outlet) {
		if (value < device->sw.minswitchvalue[0 * ALPACA_MAX_SWITCHES + id] || value > device->sw.maxswitchvalue[0 * ALPACA_MAX_SWITCHES + id]) {
			pthread_mutex_unlock(&device->mutex);
			return indigo_alpaca_error_InvalidValue;
		}
		sprintf(name, "OUTLET_%d", id + 1);
		device->sw.valueset[0] = false;
		indigo_change_switch_property_1(indigo_agent_alpaca_client, device->indigo_device, AUX_POWER_OUTLET_PROPERTY_NAME, name, value == 1);
		pthread_mutex_unlock(&device->mutex);
		return indigo_alpaca_wait_for_bool(&device->sw.valueset[0], true, 30);
	}
	id -= device->sw.maxswitch_power_outlet;
	if (id < device->sw.maxswitch_heater_outlet) {
		if (value < device->sw.minswitchvalue[1 * ALPACA_MAX_SWITCHES + id] || value > device->sw.maxswitchvalue[1 * ALPACA_MAX_SWITCHES + id]) {
			pthread_mutex_unlock(&device->mutex);
			return indigo_alpaca_error_InvalidValue;
		}
		sprintf(name, "OUTLET_%d", id + 1);
		device->sw.valueset[1] = false;
		indigo_change_number_property_1(indigo_agent_alpaca_client, device->indigo_device, AUX_HEATER_OUTLET_PROPERTY_NAME, name, value);
		pthread_mutex_unlock(&device->mutex);
		return indigo_alpaca_wait_for_bool(&device->sw.valueset[1], true, 30);
	}
	id -= device->sw.maxswitch_heater_outlet;
	if (id < device->sw.maxswitch_usb_port) {
		if (value < device->sw.minswitchvalue[2 * ALPACA_MAX_SWITCHES + id] || value > device->sw.maxswitchvalue[2 * ALPACA_MAX_SWITCHES + id]) {
			pthread_mutex_unlock(&device->mutex);
			return indigo_alpaca_error_InvalidValue;
		}
		sprintf(name, "PORT_%d", id + 1);
		device->sw.valueset[2] = false;
		indigo_change_switch_property_1(indigo_agent_alpaca_client, device->indigo_device, AUX_USB_PORT_PROPERTY_NAME, name, value == 1);
		pthread_mutex_unlock(&device->mutex);
		return indigo_alpaca_wait_for_bool(&device->sw.valueset[2], true, 30);
	}
	id -= device->sw.maxswitch_usb_port;
	if (value < device->sw.minswitchvalue[3 * ALPACA_MAX_SWITCHES + id] || value > device->sw.maxswitchvalue[3 * ALPACA_MAX_SWITCHES + id]) {
		pthread_mutex_unlock(&device->mutex);
		return indigo_alpaca_error_InvalidValue;
	}
	sprintf(name, "OUTLET_%d", id + 1);
	device->sw.valueset[3] = false;
	indigo_change_switch_property_1(indigo_agent_alpaca_client, device->indigo_device, AUX_GPIO_OUTLETS_PROPERTY_NAME, name, value == 1);
	pthread_mutex_unlock(&device->mutex);
	return indigo_alpaca_wait_for_bool(&device->sw.valueset[3], true, 30);
}

static indigo_alpaca_error alpaca_set_setswitchname(indigo_alpaca_device *device, int version, int id, char *value) {
	pthread_mutex_lock(&device->mutex);
	if (!device->connected) {
		pthread_mutex_unlock(&device->mutex);
		return indigo_alpaca_error_NotConnected;
	}
	if (id < 0 || id >= get_switch_number(device)) {
		pthread_mutex_unlock(&device->mutex);
		return indigo_alpaca_error_InvalidValue;
	}
	char name[INDIGO_NAME_SIZE];
	if (id < device->sw.maxswitch_power_outlet) {
		sprintf(name, "POWER_OUTLET_NAME_%d", id + 1);
		device->sw.nameset[0] = false;
		indigo_change_text_property_1(indigo_agent_alpaca_client, device->indigo_device, AUX_OUTLET_NAMES_PROPERTY_NAME, name, value);
		pthread_mutex_unlock(&device->mutex);
		return indigo_alpaca_wait_for_bool(&device->sw.nameset[0], true, 30);
	}
	id -= device->sw.maxswitch_power_outlet;
	if (id < device->sw.maxswitch_heater_outlet) {
		sprintf(name, "HEATER_OUTLET_NAME_%d", id + 1);
		device->sw.nameset[0] = false;
		indigo_change_text_property_1(indigo_agent_alpaca_client, device->indigo_device, AUX_OUTLET_NAMES_PROPERTY_NAME, name, value);
		pthread_mutex_unlock(&device->mutex);
		return indigo_alpaca_wait_for_bool(&device->sw.nameset[0], true, 30);
	}
	id -= device->sw.maxswitch_heater_outlet;
	if (id < device->sw.maxswitch_usb_port) {
		sprintf(name, "USB_PORT_NAME_%d", id + 1);
		device->sw.nameset[0] = false;
		indigo_change_text_property_1(indigo_agent_alpaca_client, device->indigo_device, AUX_OUTLET_NAMES_PROPERTY_NAME, name, value);
		pthread_mutex_unlock(&device->mutex);
		return indigo_alpaca_wait_for_bool(&device->sw.nameset[0], true, 30);
	}
	id -= device->sw.maxswitch_usb_port;
	if (id < device->sw.maxswitch_gpio_outlet) {
		sprintf(name, "GPIO_OUTLET_%d", id + 1);
		device->sw.nameset[0] = false;
		indigo_change_text_property_1(indigo_agent_alpaca_client, device->indigo_device, AUX_OUTLET_NAMES_PROPERTY_NAME, name, value);
		pthread_mutex_unlock(&device->mutex);
		return indigo_alpaca_wait_for_bool(&device->sw.nameset[0], true, 30);
	}
	id -= device->sw.maxswitch_gpio_outlet;
	sprintf(name, "GPIO_SENSOR_NAME_%d", id + 1);
	device->sw.nameset[4] = false;
	indigo_change_text_property_1(indigo_agent_alpaca_client, device->indigo_device, AUX_SENSOR_NAMES_PROPERTY_NAME, name, value);
	pthread_mutex_unlock(&device->mutex);
	return indigo_alpaca_wait_for_bool(&device->sw.nameset[4], true, 30);
}

void indigo_alpaca_switch_update_property(indigo_alpaca_device *alpaca_device, indigo_property *property) {
	if (!strcmp(property->name, AUX_POWER_OUTLET_PROPERTY_NAME)) {
		int count = property->count;
		if (count > 8) {
			count = 8;
		}
		alpaca_device->sw.maxswitch_power_outlet = count;
		int offset = 0 * ALPACA_MAX_SWITCHES;
		alpaca_device->sw.valueset[0] = property->state == INDIGO_OK_STATE;
		if (property->state == INDIGO_OK_STATE) {
			for (int i = 0; i < count; i++) {
				indigo_item *item = property->items + i;
				alpaca_device->sw.canwrite[offset + i] = property->perm == INDIGO_RW_PERM;
				alpaca_device->sw.minswitchvalue[offset + i] = 0;
				alpaca_device->sw.maxswitchvalue[offset + i] = 1;
				alpaca_device->sw.switchstep[offset + i] = 1;
				alpaca_device->sw.switchvalue[offset + i] = item->sw.value ? 1 : 0;
			}
		}
	} else if (!strcmp(property->name, AUX_HEATER_OUTLET_PROPERTY_NAME)) {
		int count = property->count;
		if (count > 8) {
			count = 8;
		}
		alpaca_device->sw.maxswitch_heater_outlet = count;
		int offset = 1 * ALPACA_MAX_SWITCHES;
		alpaca_device->sw.valueset[1] = property->state == INDIGO_OK_STATE;
		if (property->state == INDIGO_OK_STATE) {
			for (int i = 0; i < count; i++) {
				indigo_item *item = property->items + i;
				alpaca_device->sw.canwrite[offset + i] = property->perm == INDIGO_RW_PERM;
				alpaca_device->sw.minswitchvalue[offset + i] = item->number.min;
				alpaca_device->sw.maxswitchvalue[offset + i] = item->number.max;
				alpaca_device->sw.switchstep[offset + i] = item->number.step;
				alpaca_device->sw.switchvalue[offset + i] = item->number.value;
			}
		}
	} else if (!strcmp(property->name, AUX_USB_PORT_PROPERTY_NAME)) {
		int count = property->count;
		if (count > 8) {
			count = 8;
		}
		alpaca_device->sw.maxswitch_usb_port = count;
		int offset = 2 * ALPACA_MAX_SWITCHES;
		alpaca_device->sw.valueset[2] = property->state == INDIGO_OK_STATE;
		if (property->state == INDIGO_OK_STATE) {
			for (int i = 0; i < count; i++) {
				indigo_item *item = property->items + i;
				alpaca_device->sw.canwrite[offset + i] = property->perm == INDIGO_RW_PERM;
				alpaca_device->sw.minswitchvalue[offset + i] = 0;
				alpaca_device->sw.maxswitchvalue[offset + i] = 1;
				alpaca_device->sw.switchstep[offset + i] = 1;
				alpaca_device->sw.switchvalue[offset + i] = item->sw.value ? 1 : 0;
			}
		}
	} else if (!strcmp(property->name, AUX_GPIO_OUTLETS_PROPERTY_NAME)) {
		int count = property->count;
		if (count > 8) {
			count = 8;
		}
		alpaca_device->sw.maxswitch_gpio_outlet = count;
		int offset = 3 * ALPACA_MAX_SWITCHES;
		alpaca_device->sw.valueset[3] = property->state == INDIGO_OK_STATE;
		if (property->state == INDIGO_OK_STATE) {
			for (int i = 0; i < count; i++) {
				indigo_item *item = property->items + i;
				alpaca_device->sw.canwrite[offset + i] = property->perm == INDIGO_RW_PERM;
				alpaca_device->sw.minswitchvalue[offset + i] = 0;
				alpaca_device->sw.maxswitchvalue[offset + i] = 1;
				alpaca_device->sw.switchstep[offset + i] = 1;
				alpaca_device->sw.switchvalue[offset + i] = item->sw.value ? 1 : 0;
			}
		}
	} else if (!strcmp(property->name, AUX_GPIO_SENSORS_PROPERTY_NAME)) {
		int count = property->count;
		if (count > 8) {
			count = 8;
		}
		alpaca_device->sw.maxswitch_gpio_sensor = count;
		int offset = 4 * ALPACA_MAX_SWITCHES;
		alpaca_device->sw.valueset[4] = property->state == INDIGO_OK_STATE;
		if (property->state == INDIGO_OK_STATE) {
			for (int i = 0; i < count; i++) {
				indigo_item *item = property->items + i;
				alpaca_device->sw.canwrite[offset + i] = false;
				alpaca_device->sw.minswitchvalue[offset + i] = item->number.min;
				alpaca_device->sw.maxswitchvalue[offset + i] = item->number.max;
				alpaca_device->sw.switchstep[offset + i] = item->number.step;
				alpaca_device->sw.switchvalue[offset + i] = item->number.value;
			}
		}
	} else if (!strcmp(property->name, AUX_OUTLET_NAMES_PROPERTY_NAME)) {
		int offset = 0 * ALPACA_MAX_SWITCHES;
		alpaca_device->sw.nameset[0] = property->state == INDIGO_OK_STATE;
		for (int i = 0; i < property->count; i++) {
			indigo_item *item = property->items + i;
			strcpy(alpaca_device->sw.switchname[offset + i], item->text.value);
		}
	} else if (!strcmp(property->name, AUX_SENSOR_NAMES_PROPERTY_NAME)) {
		int offset = 4 * ALPACA_MAX_SWITCHES;
		alpaca_device->sw.nameset[4] = property->state == INDIGO_OK_STATE;
		for (int i = 0; i < property->count; i++) {
			indigo_item *item = property->items + i;
			strcpy(alpaca_device->sw.switchname[offset + i], item->text.value);
		}
	}
}

long indigo_alpaca_switch_get_command(indigo_alpaca_device *alpaca_device, int version, char *command, int id, char *buffer, long buffer_length) {
	if (!strcmp(command, "supportedactions")) {
		return snprintf(buffer, buffer_length, "\"Value\": [ ], \"ErrorNumber\": 0, \"ErrorMessage\": \"\"");
	}
	if (!strcmp(command, "interfaceversion")) {
		int value;
		indigo_alpaca_error result = alpaca_get_interfaceversion(alpaca_device, version, &value);
		return indigo_alpaca_append_value_int(buffer, buffer_length, value, result);
	}
	if (!strcmp(command, "maxswitch")) {
		int value;
		indigo_alpaca_error result = alpaca_get_maxswitch(alpaca_device, version, &value);
		return indigo_alpaca_append_value_int(buffer, buffer_length, value, result);
	}
	if (!strcmp(command, "canwrite")) {
		bool value;
		indigo_alpaca_error result = alpaca_get_canwrite(alpaca_device, version, id, &value);
		return indigo_alpaca_append_value_bool(buffer, buffer_length, value, result);
	}
	if (!strcmp(command, "getswitchname") || !strcmp(command, "getswitchdescription")) {
		char *value = NULL;
		indigo_alpaca_error result = alpaca_get_switchname(alpaca_device, version, id, &value);
		return indigo_alpaca_append_value_string(buffer, buffer_length, value, result);
	}
	if (!strcmp(command, "getswitch")) {
		bool value;
		indigo_alpaca_error result = alpaca_get_switch(alpaca_device, version, id, &value);
		return indigo_alpaca_append_value_bool(buffer, buffer_length, value, result);
	}
	if (!strcmp(command, "getswitchvalue")) {
		double value;
		indigo_alpaca_error result = alpaca_get_switchvalue(alpaca_device, version, id, &value);
		return indigo_alpaca_append_value_double(buffer, buffer_length, value, result);
	}
	if (!strcmp(command, "minswitchvalue")) {
		double value;
		indigo_alpaca_error result = alpaca_get_minswitchvalue(alpaca_device, version, id, &value);
		return indigo_alpaca_append_value_double(buffer, buffer_length, value, result);
	}
	if (!strcmp(command, "maxswitchvalue")) {
		double value;
		indigo_alpaca_error result = alpaca_get_maxswitchvalue(alpaca_device, version, id, &value);
		return indigo_alpaca_append_value_double(buffer, buffer_length, value, result);
	}
	if (!strcmp(command, "switchstep")) {
		double value;
		indigo_alpaca_error result = alpaca_get_switchstep(alpaca_device, version, id, &value);
		return indigo_alpaca_append_value_double(buffer, buffer_length, value, result);
	}
	return snprintf(buffer, buffer_length, "\"ErrorNumber\": %d, \"ErrorMessage\": \"%s\"", indigo_alpaca_error_NotImplemented, indigo_alpaca_error_string(indigo_alpaca_error_NotImplemented));
}

long indigo_alpaca_switch_set_command(indigo_alpaca_device *alpaca_device, int version, char *command, char *buffer, long buffer_length, char *param_1, char *param_2) {
	if (!strcmp(command, "setswitch")) {
		int id = 0;
		indigo_alpaca_error result;
		if (sscanf(param_1, "ID=%d", &id) == 1) {
			bool value = !strcasecmp(param_2, "State=true");
			result = alpaca_set_setswitch(alpaca_device, version, id, value);
		} else {
			result = indigo_alpaca_error_InvalidValue;
		}
		return indigo_alpaca_append_error(buffer, buffer_length, result);
	}
	if (!strcmp(command, "setswitchvalue")) {
		int id = 0;
		indigo_alpaca_error result;
		if (sscanf(param_1, "ID=%d", &id) == 1) {
			double value = 0;
			sscanf(param_2, "Value=%lf", &value);
			result = alpaca_set_setswitchvalue(alpaca_device, version, id, value);
		} else {
			result = indigo_alpaca_error_InvalidValue;
		}
		return indigo_alpaca_append_error(buffer, buffer_length, result);
	}
	if (!strcmp(command, "setswitchname")) {
		int id = 0;
		indigo_alpaca_error result;
		if (sscanf(param_1, "ID=%d", &id) == 1) {
			char value[INDIGO_VALUE_SIZE];
			sscanf(param_2, "Name=%s", value);
			result = alpaca_set_setswitchname(alpaca_device, version, id, value);
		} else {
			result = indigo_alpaca_error_InvalidValue;
		}
		return indigo_alpaca_append_error(buffer, buffer_length, result);
	}
	return snprintf(buffer, buffer_length, "\"ErrorNumber\": %d, \"ErrorMessage\": \"%s\"", indigo_alpaca_error_NotImplemented, indigo_alpaca_error_string(indigo_alpaca_error_NotImplemented));
}
