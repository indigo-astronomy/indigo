// Copyright (c) 2021-2026 Rumen G. Bogdanovski
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
// 3.0 refactoring by Peter Polakovic <peter.polakovic@cloudmakers.eu>

/** INDIGO ASCOM ALPACA bridge agent
 \file alpaca_switch.c
 */

#include <indigo/indigo_aux_driver.h>

#include "indigo_alpaca_common.h"

// The Alpaca switches of an INDIGO device are its outlets and sensors, section after section in the order of this table, the writable
// ones first; a section holds at most ALPACA_MAX_SWITCHES switches. X_ALPACA_SWITCH_VALUES is the property of system_alpaca for a
// writable switch that is not boolean (a range with a step), so an Alpaca device proxied by system_alpaca keeps those switches.
typedef struct {
	const char *property;				// property of the switches
	const char *item;						// item name format, numbered from 1
	const char *names;					// property that names them
	const char *name_item;			// item name format in that property
	int names_index;						// index of that property in sw.nameset
	bool number;								// number items with a range (otherwise boolean switch items)
	bool writable;							// can be written at all (sensors can not)
} switch_section;

static const switch_section sections[ALPACA_SWITCH_SECTIONS] = {
	{ AUX_POWER_OUTLET_PROPERTY_NAME, "OUTLET_%d", AUX_OUTLET_NAMES_PROPERTY_NAME, "POWER_OUTLET_NAME_%d", 0, false, true },
	{ AUX_HEATER_OUTLET_PROPERTY_NAME, "OUTLET_%d", AUX_OUTLET_NAMES_PROPERTY_NAME, "HEATER_OUTLET_NAME_%d", 0, true, true },
	{ AUX_USB_PORT_PROPERTY_NAME, "PORT_%d", AUX_OUTLET_NAMES_PROPERTY_NAME, "USB_PORT_NAME_%d", 0, false, true },
	{ AUX_GPIO_OUTLETS_PROPERTY_NAME, "OUTLET_%d", AUX_OUTLET_NAMES_PROPERTY_NAME, "GPIO_OUTLET_NAME_%d", 0, false, true },
	{ "X_ALPACA_SWITCH_VALUES", "VALUE_%d", AUX_OUTLET_NAMES_PROPERTY_NAME, "VALUE_NAME_%d", 0, true, true },
	{ AUX_GPIO_SENSORS_PROPERTY_NAME, "SENSOR_%d", AUX_SENSOR_NAMES_PROPERTY_NAME, "GPIO_SENSOR_NAME_%d", 1, true, false }
};

static int get_switch_number(indigo_alpaca_device *device) {
	int count = 0;
	for (int section = 0; section < ALPACA_SWITCH_SECTIONS; section++) {
		count += device->sw.count[section];
	}
	return count;
}

// Section of the Alpaca switch id and its index in the section, or false if there is no such switch. Called with the mutex held.
static bool find_switch(indigo_alpaca_device *device, int id, int *section, int *index) {
	if (id < 0) {
		return false;
	}
	for (int i = 0; i < ALPACA_SWITCH_SECTIONS; i++) {
		if (id < device->sw.count[i]) {
			*section = i;
			*index = id;
			return true;
		}
		id -= device->sw.count[i];
	}
	return false;
}

// Locks the mutex and finds the switch; the slot is section * ALPACA_MAX_SWITCHES + index. On an error the mutex is unlocked.
static indigo_alpaca_error lock_switch(indigo_alpaca_device *device, int id, int *section, int *index, int *slot) {
	pthread_mutex_lock(&device->mutex);
	if (!device->connected) {
		pthread_mutex_unlock(&device->mutex);
		return indigo_alpaca_error_NotConnected;
	}
	if (!find_switch(device, id, section, index)) {
		pthread_mutex_unlock(&device->mutex);
		return indigo_alpaca_error_InvalidValue;
	}
	*slot = *section * ALPACA_MAX_SWITCHES + *index;
	return indigo_alpaca_error_OK;
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
	int section, index, slot;
	indigo_alpaca_error result = lock_switch(device, id, &section, &index, &slot);
	if (result == indigo_alpaca_error_OK) {
		*value = device->sw.canwrite[slot];
		pthread_mutex_unlock(&device->mutex);
	}
	return result;
}

static indigo_alpaca_error alpaca_get_minswitchvalue(indigo_alpaca_device *device, int version, int id, double *value) {
	int section, index, slot;
	indigo_alpaca_error result = lock_switch(device, id, &section, &index, &slot);
	if (result == indigo_alpaca_error_OK) {
		*value = device->sw.minswitchvalue[slot];
		pthread_mutex_unlock(&device->mutex);
	}
	return result;
}

static indigo_alpaca_error alpaca_get_maxswitchvalue(indigo_alpaca_device *device, int version, int id, double *value) {
	int section, index, slot;
	indigo_alpaca_error result = lock_switch(device, id, &section, &index, &slot);
	if (result == indigo_alpaca_error_OK) {
		*value = device->sw.maxswitchvalue[slot];
		pthread_mutex_unlock(&device->mutex);
	}
	return result;
}

static indigo_alpaca_error alpaca_get_switchstep(indigo_alpaca_device *device, int version, int id, double *value) {
	int section, index, slot;
	indigo_alpaca_error result = lock_switch(device, id, &section, &index, &slot);
	if (result == indigo_alpaca_error_OK) {
		*value = device->sw.switchstep[slot];
		pthread_mutex_unlock(&device->mutex);
	}
	return result;
}

static indigo_alpaca_error alpaca_get_switchvalue(indigo_alpaca_device *device, int version, int id, double *value) {
	int section, index, slot;
	indigo_alpaca_error result = lock_switch(device, id, &section, &index, &slot);
	if (result == indigo_alpaca_error_OK) {
		*value = device->sw.switchvalue[slot];
		pthread_mutex_unlock(&device->mutex);
	}
	return result;
}

static indigo_alpaca_error alpaca_get_switch(indigo_alpaca_device *device, int version, int id, bool *value) {
	int section, index, slot;
	indigo_alpaca_error result = lock_switch(device, id, &section, &index, &slot);
	if (result == indigo_alpaca_error_OK) {
		*value = device->sw.switchvalue[slot] == device->sw.maxswitchvalue[slot];
		pthread_mutex_unlock(&device->mutex);
	}
	return result;
}

static indigo_alpaca_error alpaca_get_switchname(indigo_alpaca_device *device, int version, int id, char **value) {
	int section, index, slot;
	indigo_alpaca_error result = lock_switch(device, id, &section, &index, &slot);
	if (result == indigo_alpaca_error_OK) {
		// a switch that has no entry in AUX_OUTLET_NAMES or AUX_SENSOR_NAMES is named by the label of its item
		*value = device->sw.switchname[slot];
		if (**value == 0) {
			*value = device->sw.switchlabel[slot];
		}
		pthread_mutex_unlock(&device->mutex);
	}
	return result;
}

// Set a writable switch to a value within its range, and wait until its property is no longer busy. Called with the mutex held,
// returns with it released.
static indigo_alpaca_error set_value(indigo_alpaca_device *device, int section, int index, int slot, double value) {
	const switch_section *s = sections + section;
	char name[INDIGO_NAME_SIZE];
	snprintf(name, sizeof(name), s->item, index + 1);
	device->sw.valueset[section] = false;
	if (s->number) {
		indigo_change_number_property_1(indigo_agent_alpaca_client, device->indigo_device, s->property, name, value);
	} else {
		indigo_change_switch_property_1(indigo_agent_alpaca_client, device->indigo_device, s->property, name, value == device->sw.maxswitchvalue[slot]);
	}
	pthread_mutex_unlock(&device->mutex);
	return indigo_alpaca_wait_for_bool(&device->sw.valueset[section], true, 30);
}

static indigo_alpaca_error alpaca_set_setswitch(indigo_alpaca_device *device, int version, int id, bool value) {
	int section, index, slot;
	indigo_alpaca_error result = lock_switch(device, id, &section, &index, &slot);
	if (result != indigo_alpaca_error_OK) {
		return result;
	}
	if (!device->sw.canwrite[slot]) {
		pthread_mutex_unlock(&device->mutex);
		return indigo_alpaca_error_NotImplemented;
	}
	return set_value(device, section, index, slot, value ? device->sw.maxswitchvalue[slot] : device->sw.minswitchvalue[slot]);
}

static indigo_alpaca_error alpaca_set_setswitchvalue(indigo_alpaca_device *device, int version, int id, double value) {
	int section, index, slot;
	indigo_alpaca_error result = lock_switch(device, id, &section, &index, &slot);
	if (result != indigo_alpaca_error_OK) {
		return result;
	}
	if (!device->sw.canwrite[slot]) {
		pthread_mutex_unlock(&device->mutex);
		return indigo_alpaca_error_NotImplemented;
	}
	if (value < device->sw.minswitchvalue[slot] || value > device->sw.maxswitchvalue[slot]) {
		pthread_mutex_unlock(&device->mutex);
		return indigo_alpaca_error_InvalidValue;
	}
	return set_value(device, section, index, slot, value);
}

static indigo_alpaca_error alpaca_set_setswitchname(indigo_alpaca_device *device, int version, int id, char *value) {
	int section, index, slot;
	indigo_alpaca_error result = lock_switch(device, id, &section, &index, &slot);
	if (result != indigo_alpaca_error_OK) {
		return result;
	}
	const switch_section *s = sections + section;
	char name[INDIGO_NAME_SIZE];
	snprintf(name, sizeof(name), s->name_item, index + 1);
	device->sw.nameset[s->names_index] = false;
	indigo_change_text_property_1(indigo_agent_alpaca_client, device->indigo_device, s->names, name, value);
	pthread_mutex_unlock(&device->mutex);
	return indigo_alpaca_wait_for_bool(&device->sw.nameset[s->names_index], true, 30);
}

void indigo_alpaca_switch_update_property(indigo_alpaca_device *alpaca_device, indigo_property *property) {
	for (int section = 0; section < ALPACA_SWITCH_SECTIONS; section++) {
		const switch_section *s = sections + section;
		if (!strcmp(property->name, s->property)) {
			int count = property->count < ALPACA_MAX_SWITCHES ? property->count : ALPACA_MAX_SWITCHES;
			alpaca_device->sw.count[section] = count;
			alpaca_device->sw.valueset[section] = property->state == INDIGO_OK_STATE;
			if (property->state == INDIGO_OK_STATE) {
				int offset = section * ALPACA_MAX_SWITCHES;
				for (int i = 0; i < count; i++) {
					indigo_item *item = property->items + i;
					INDIGO_COPY_VALUE(alpaca_device->sw.switchlabel[offset + i], item->label);
					alpaca_device->sw.canwrite[offset + i] = s->writable && property->perm == INDIGO_RW_PERM;
					if (s->number) {
						alpaca_device->sw.minswitchvalue[offset + i] = item->number.min;
						alpaca_device->sw.maxswitchvalue[offset + i] = item->number.max;
						alpaca_device->sw.switchstep[offset + i] = item->number.step;
						alpaca_device->sw.switchvalue[offset + i] = item->number.value;
					} else {
						alpaca_device->sw.minswitchvalue[offset + i] = 0;
						alpaca_device->sw.maxswitchvalue[offset + i] = 1;
						alpaca_device->sw.switchstep[offset + i] = 1;
						alpaca_device->sw.switchvalue[offset + i] = item->sw.value ? 1 : 0;
					}
				}
			}
			return;
		}
	}
	bool outlet_names = !strcmp(property->name, AUX_OUTLET_NAMES_PROPERTY_NAME);
	if (outlet_names || !strcmp(property->name, AUX_SENSOR_NAMES_PROPERTY_NAME)) {
		// one property names the outlets of all writable sections: the item name tells the section and the index
		alpaca_device->sw.nameset[outlet_names ? 0 : 1] = property->state == INDIGO_OK_STATE;
		for (int i = 0; i < property->count; i++) {
			indigo_item *item = property->items + i;
			for (int section = 0; section < ALPACA_SWITCH_SECTIONS; section++) {
				const switch_section *s = sections + section;
				if (strcmp(s->names, property->name)) {
					continue;
				}
				size_t length = strlen(s->name_item) - 2;		// the format without "%d"
				if (!strncmp(item->name, s->name_item, length)) {
					int index = atoi(item->name + length) - 1;
					if (index >= 0 && index < ALPACA_MAX_SWITCHES) {
						INDIGO_COPY_VALUE(alpaca_device->sw.switchname[section * ALPACA_MAX_SWITCHES + index], item->text.value);
					}
					break;
				}
			}
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
		if (sscanf(param_1, "Id=%d", &id) == 1) {
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
		if (sscanf(param_1, "Id=%d", &id) == 1) {
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
		if (sscanf(param_1, "Id=%d", &id) == 1) {
			char value[INDIGO_VALUE_SIZE];
			sscanf(param_2, "Name=%127s", value);
			result = alpaca_set_setswitchname(alpaca_device, version, id, value);
		} else {
			result = indigo_alpaca_error_InvalidValue;
		}
		return indigo_alpaca_append_error(buffer, buffer_length, result);
	}
	return snprintf(buffer, buffer_length, "\"ErrorNumber\": %d, \"ErrorMessage\": \"%s\"", indigo_alpaca_error_NotImplemented, indigo_alpaca_error_string(indigo_alpaca_error_NotImplemented));
}
