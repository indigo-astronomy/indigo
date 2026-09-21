// Copyright (c) 2020-2026 Rumen G. Bogdanovski
// All rights reserved.

// You may use this software under the terms of 'INDIGO Astronomy
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

// This file generated from indigo_aux_rpio.driver

#pragma mark - Includes

#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <assert.h>

//+ include

#include <dirent.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>

//- include

#include <indigo/indigo_driver_xml.h>
#include <indigo/indigo_aux_driver.h>
#include <indigo/indigo_uni_io.h>

#include "indigo_aux_rpio.h"

#pragma mark - Common definitions

#define DRIVER_VERSION       0x03000009
#define DRIVER_NAME          "indigo_aux_rpio"
#define DRIVER_LABEL         "Raspberry Pi GPIO"
#define AUX_DEVICE_NAME      "Raspberry Pi GPIO"
#define PRIVATE_DATA         ((rpio_private_data *)device->private_data)

//+ define

#include "shared/rpio_sysfs.h"

#define AUX_RELAYS_GROUP     "Pin Control"
#define AUX_SENSORS_GROUP    "Inputs"
#define OUTPUT_COUNT         8
#define INPUT_COUNT          8
// Outputs #1 and #2 are driven as PWM channels when a PWM chip is found.
#define PWM_COUNT            2

//- define

#pragma mark - Property definitions

#define AUX_OUTLET_NAMES_PROPERTY      (PRIVATE_DATA->aux_outlet_names_property)
#define AUX_OUTLET_NAME_1_ITEM         (AUX_OUTLET_NAMES_PROPERTY->items + 0)
#define AUX_OUTLET_NAME_2_ITEM         (AUX_OUTLET_NAMES_PROPERTY->items + 1)
#define AUX_OUTLET_NAME_3_ITEM         (AUX_OUTLET_NAMES_PROPERTY->items + 2)
#define AUX_OUTLET_NAME_4_ITEM         (AUX_OUTLET_NAMES_PROPERTY->items + 3)
#define AUX_OUTLET_NAME_5_ITEM         (AUX_OUTLET_NAMES_PROPERTY->items + 4)
#define AUX_OUTLET_NAME_6_ITEM         (AUX_OUTLET_NAMES_PROPERTY->items + 5)
#define AUX_OUTLET_NAME_7_ITEM         (AUX_OUTLET_NAMES_PROPERTY->items + 6)
#define AUX_OUTLET_NAME_8_ITEM         (AUX_OUTLET_NAMES_PROPERTY->items + 7)

#define AUX_GPIO_OUTLETS_PROPERTY      (PRIVATE_DATA->aux_gpio_outlets_property)
#define AUX_GPIO_OUTLET_1_ITEM         (AUX_GPIO_OUTLETS_PROPERTY->items + 0)
#define AUX_GPIO_OUTLET_2_ITEM         (AUX_GPIO_OUTLETS_PROPERTY->items + 1)
#define AUX_GPIO_OUTLET_3_ITEM         (AUX_GPIO_OUTLETS_PROPERTY->items + 2)
#define AUX_GPIO_OUTLET_4_ITEM         (AUX_GPIO_OUTLETS_PROPERTY->items + 3)
#define AUX_GPIO_OUTLET_5_ITEM         (AUX_GPIO_OUTLETS_PROPERTY->items + 4)
#define AUX_GPIO_OUTLET_6_ITEM         (AUX_GPIO_OUTLETS_PROPERTY->items + 5)
#define AUX_GPIO_OUTLET_7_ITEM         (AUX_GPIO_OUTLETS_PROPERTY->items + 6)
#define AUX_GPIO_OUTLET_8_ITEM         (AUX_GPIO_OUTLETS_PROPERTY->items + 7)

#define AUX_OUTLET_PULSE_LENGTHS_PROPERTY (PRIVATE_DATA->aux_outlet_pulse_lengths_property)
#define AUX_OUTLET_PULSE_LENGTHS_1_ITEM   (AUX_OUTLET_PULSE_LENGTHS_PROPERTY->items + 0)
#define AUX_OUTLET_PULSE_LENGTHS_2_ITEM   (AUX_OUTLET_PULSE_LENGTHS_PROPERTY->items + 1)
#define AUX_OUTLET_PULSE_LENGTHS_3_ITEM   (AUX_OUTLET_PULSE_LENGTHS_PROPERTY->items + 2)
#define AUX_OUTLET_PULSE_LENGTHS_4_ITEM   (AUX_OUTLET_PULSE_LENGTHS_PROPERTY->items + 3)
#define AUX_OUTLET_PULSE_LENGTHS_5_ITEM   (AUX_OUTLET_PULSE_LENGTHS_PROPERTY->items + 4)
#define AUX_OUTLET_PULSE_LENGTHS_6_ITEM   (AUX_OUTLET_PULSE_LENGTHS_PROPERTY->items + 5)
#define AUX_OUTLET_PULSE_LENGTHS_7_ITEM   (AUX_OUTLET_PULSE_LENGTHS_PROPERTY->items + 6)
#define AUX_OUTLET_PULSE_LENGTHS_8_ITEM   (AUX_OUTLET_PULSE_LENGTHS_PROPERTY->items + 7)

#define X_AUX_PWM_PROPERTY             (PRIVATE_DATA->x_aux_pwm_property)
#define X_AUX_PWM_ENABLED_ITEM         (X_AUX_PWM_PROPERTY->items + 0)
#define X_AUX_PWM_DISABLED_ITEM        (X_AUX_PWM_PROPERTY->items + 1)

#define X_AUX_PWM_PROPERTY_NAME        "X_AUX_PWM"
#define X_AUX_PWM_ENABLED_ITEM_NAME    "ENABLED"
#define X_AUX_PWM_DISABLED_ITEM_NAME   "DISABLED"

#define AUX_GPIO_OUTLET_FREQUENCIES_PROPERTY      (PRIVATE_DATA->aux_gpio_outlet_frequencies_property)
#define AUX_GPIO_OUTLET_FREQUENCIES_OUTLET_1_ITEM (AUX_GPIO_OUTLET_FREQUENCIES_PROPERTY->items + 0)
#define AUX_GPIO_OUTLET_FREQUENCIES_OUTLET_2_ITEM (AUX_GPIO_OUTLET_FREQUENCIES_PROPERTY->items + 1)

#define AUX_GPIO_OUTLET_DUTY_PROPERTY      (PRIVATE_DATA->aux_gpio_outlet_duty_property)
#define AUX_GPIO_OUTLET_DUTY_OUTLET_1_ITEM (AUX_GPIO_OUTLET_DUTY_PROPERTY->items + 0)
#define AUX_GPIO_OUTLET_DUTY_OUTLET_2_ITEM (AUX_GPIO_OUTLET_DUTY_PROPERTY->items + 1)

#define AUX_SENSOR_NAMES_PROPERTY      (PRIVATE_DATA->aux_sensor_names_property)
#define AUX_SENSOR_NAME_1_ITEM         (AUX_SENSOR_NAMES_PROPERTY->items + 0)
#define AUX_SENSOR_NAME_2_ITEM         (AUX_SENSOR_NAMES_PROPERTY->items + 1)
#define AUX_SENSOR_NAME_3_ITEM         (AUX_SENSOR_NAMES_PROPERTY->items + 2)
#define AUX_SENSOR_NAME_4_ITEM         (AUX_SENSOR_NAMES_PROPERTY->items + 3)
#define AUX_SENSOR_NAME_5_ITEM         (AUX_SENSOR_NAMES_PROPERTY->items + 4)
#define AUX_SENSOR_NAME_6_ITEM         (AUX_SENSOR_NAMES_PROPERTY->items + 5)
#define AUX_SENSOR_NAME_7_ITEM         (AUX_SENSOR_NAMES_PROPERTY->items + 6)
#define AUX_SENSOR_NAME_8_ITEM         (AUX_SENSOR_NAMES_PROPERTY->items + 7)

#define AUX_GPIO_SENSORS_PROPERTY      (PRIVATE_DATA->aux_gpio_sensors_property)
#define AUX_GPIO_SENSOR_1_ITEM         (AUX_GPIO_SENSORS_PROPERTY->items + 0)
#define AUX_GPIO_SENSOR_2_ITEM         (AUX_GPIO_SENSORS_PROPERTY->items + 1)
#define AUX_GPIO_SENSOR_3_ITEM         (AUX_GPIO_SENSORS_PROPERTY->items + 2)
#define AUX_GPIO_SENSOR_4_ITEM         (AUX_GPIO_SENSORS_PROPERTY->items + 3)
#define AUX_GPIO_SENSOR_5_ITEM         (AUX_GPIO_SENSORS_PROPERTY->items + 4)
#define AUX_GPIO_SENSOR_6_ITEM         (AUX_GPIO_SENSORS_PROPERTY->items + 5)
#define AUX_GPIO_SENSOR_7_ITEM         (AUX_GPIO_SENSORS_PROPERTY->items + 6)
#define AUX_GPIO_SENSOR_8_ITEM         (AUX_GPIO_SENSORS_PROPERTY->items + 7)

#pragma mark - Private data definition

typedef struct {
	indigo_property *aux_outlet_names_property;
	indigo_property *aux_gpio_outlets_property;
	indigo_property *aux_outlet_pulse_lengths_property;
	indigo_property *x_aux_pwm_property;
	indigo_property *aux_gpio_outlet_frequencies_property;
	indigo_property *aux_gpio_outlet_duty_property;
	indigo_property *aux_sensor_names_property;
	indigo_property *aux_gpio_sensors_property;
	//+ data
	rpio_sysfs sysfs;
	double pulse_until[OUTPUT_COUNT];
	//- data
} rpio_private_data;

#pragma mark - Low level code

//+ code

#include "shared/rpio_sysfs.c"

static const int rpio_output_pins[OUTPUT_COUNT] = { 18, 12, 13, 26, 16, 5, 6, 21 };
static const int rpio_input_pins[INPUT_COUNT] = { 19, 17, 27, 22, 23, 24, 25, 20 };
// PWM channel 0 drives Output #1 and channel 1 drives Output #2.
static const int rpio_pwm_lines[PWM_COUNT] = { 0, 1 };

static void rpio_apply_outlet_names(indigo_device *device) {
	if (IS_CONNECTED) {
		indigo_delete_property(device, AUX_GPIO_OUTLETS_PROPERTY, NULL);
		indigo_delete_property(device, AUX_OUTLET_PULSE_LENGTHS_PROPERTY, NULL);
		indigo_delete_property(device, AUX_GPIO_OUTLET_FREQUENCIES_PROPERTY, NULL);
		indigo_delete_property(device, AUX_GPIO_OUTLET_DUTY_PROPERTY, NULL);
	}
	for (int i = 0; i < OUTPUT_COUNT; i++) {
		const char *name = (AUX_OUTLET_NAMES_PROPERTY->items + i)->text.value;
		INDIGO_COPY_VALUE((AUX_GPIO_OUTLETS_PROPERTY->items + i)->label, name);
		INDIGO_COPY_VALUE((AUX_OUTLET_PULSE_LENGTHS_PROPERTY->items + i)->label, name);
		if (i < PWM_COUNT) {
			INDIGO_COPY_VALUE((AUX_GPIO_OUTLET_FREQUENCIES_PROPERTY->items + i)->label, name);
			INDIGO_COPY_VALUE((AUX_GPIO_OUTLET_DUTY_PROPERTY->items + i)->label, name);
		}
	}
	if (IS_CONNECTED) {
		indigo_define_property(device, AUX_GPIO_OUTLETS_PROPERTY, NULL);
		indigo_define_property(device, AUX_OUTLET_PULSE_LENGTHS_PROPERTY, NULL);
		indigo_define_property(device, AUX_GPIO_OUTLET_FREQUENCIES_PROPERTY, NULL);
		indigo_define_property(device, AUX_GPIO_OUTLET_DUTY_PROPERTY, NULL);
	}
}

static void rpio_apply_sensor_names(indigo_device *device) {
	if (IS_CONNECTED) {
		indigo_delete_property(device, AUX_GPIO_SENSORS_PROPERTY, NULL);
	}
	for (int i = 0; i < INPUT_COUNT; i++) {
		INDIGO_COPY_VALUE((AUX_GPIO_SENSORS_PROPERTY->items + i)->label, (AUX_SENSOR_NAMES_PROPERTY->items + i)->text.value);
	}
	if (IS_CONNECTED) {
		indigo_define_property(device, AUX_GPIO_SENSORS_PROPERTY, NULL);
	}
}

// The shortest remaining pulse, or a negative value when none is running.
static double rpio_pulse_delay(indigo_device *device) {
	double now = indigo_monotonic_time();
	double delay = -1;
	for (int i = 0; i < OUTPUT_COUNT; i++) {
		if (PRIVATE_DATA->pulse_until[i] > 0) {
			double remaining = PRIVATE_DATA->pulse_until[i] - now;
			if (remaining < 0) {
				remaining = 0;
			}
			if (delay < 0 || remaining < delay) {
				delay = remaining;
			}
		}
	}
	return delay;
}

// One finalizer serves every output of the device: it switches off each
// output whose pulse has elapsed and rearms itself for the next deadline.
static void relay_pulse_finalizer(indigo_device *device) {
	double now = indigo_monotonic_time();
	bool updated = false;
	for (int i = 0; i < OUTPUT_COUNT; i++) {
		if (PRIVATE_DATA->pulse_until[i] > 0 && now >= PRIVATE_DATA->pulse_until[i]) {
			PRIVATE_DATA->pulse_until[i] = 0;
			rpio_sysfs_write_output(&PRIVATE_DATA->sysfs, i, false);
			(AUX_GPIO_OUTLETS_PROPERTY->items + i)->sw.value = false;
			updated = true;
		}
	}
	if (updated) {
		indigo_update_property(device, AUX_GPIO_OUTLETS_PROPERTY, NULL);
	}
	double delay = rpio_pulse_delay(device);
	if (delay >= 0) {
		indigo_execute_handler_in(device, delay, relay_pulse_finalizer);
	}
}

// Write only the outputs whose requested state differs from the state the
// pins report. An output with a non-zero pulse length is pulsed when it is
// switched on and no pulse is already running for it.
static bool rpio_set_outlets(indigo_device *device) {
	int values[OUTPUT_COUNT];
	if (!rpio_sysfs_read_outputs(&PRIVATE_DATA->sysfs, values)) {
		return false;
	}
	bool result = true;
	for (int i = 0; i < OUTPUT_COUNT; i++) {
		indigo_item *outlet = AUX_GPIO_OUTLETS_PROPERTY->items + i;
		double length = (AUX_OUTLET_PULSE_LENGTHS_PROPERTY->items + i)->number.value;
		bool pulse_running = PRIVATE_DATA->pulse_until[i] > 0;
		if (outlet->sw.value == (values[i] != 0)) {
			continue;
		}
		if (length > 0 && outlet->sw.value && !pulse_running) {
			if (rpio_sysfs_write_output(&PRIVATE_DATA->sysfs, i, true)) {
				PRIVATE_DATA->pulse_until[i] = indigo_monotonic_time() + length / 1000;
			} else {
				INDIGO_DRIVER_ERROR(DRIVER_NAME, "Output #%d pulse failed, did you authorize?", i + 1);
				result = false;
			}
		} else if (length == 0 || (!outlet->sw.value && !pulse_running)) {
			if (!rpio_sysfs_write_output(&PRIVATE_DATA->sysfs, i, outlet->sw.value)) {
				INDIGO_DRIVER_ERROR(DRIVER_NAME, "Output #%d write failed, did you authorize?", i + 1);
				result = false;
			}
		}
	}
	return result;
}

static void rpio_update_sensors(indigo_device *device) {
	int values[INPUT_COUNT];
	if (rpio_sysfs_read_inputs(&PRIVATE_DATA->sysfs, values)) {
		for (int i = 0; i < INPUT_COUNT; i++) {
			(AUX_GPIO_SENSORS_PROPERTY->items + i)->number.value = values[i];
		}
		AUX_GPIO_SENSORS_PROPERTY->state = INDIGO_OK_STATE;
	} else {
		AUX_GPIO_SENSORS_PROPERTY->state = INDIGO_ALERT_STATE;
	}
	indigo_update_property(device, AUX_GPIO_SENSORS_PROPERTY, NULL);
	if (!PRIVATE_DATA->sysfs.pwm_present) {
		return;
	}
	bool ok = true;
	for (int i = 0; i < PWM_COUNT; i++) {
		double frequency = 0;
		double duty = 0;
		if (rpio_sysfs_get_pwm(&PRIVATE_DATA->sysfs, i, &frequency, &duty)) {
			indigo_item *frequency_item = AUX_GPIO_OUTLET_FREQUENCIES_PROPERTY->items + i;
			indigo_item *duty_item = AUX_GPIO_OUTLET_DUTY_PROPERTY->items + i;
			frequency_item->number.value = frequency_item->number.target = frequency;
			duty_item->number.value = duty_item->number.target = duty;
		} else {
			ok = false;
		}
	}
	AUX_GPIO_OUTLET_FREQUENCIES_PROPERTY->state = AUX_GPIO_OUTLET_DUTY_PROPERTY->state = ok ? INDIGO_OK_STATE : INDIGO_ALERT_STATE;
	indigo_update_property(device, AUX_GPIO_OUTLET_FREQUENCIES_PROPERTY, NULL);
	indigo_update_property(device, AUX_GPIO_OUTLET_DUTY_PROPERTY, NULL);
}

// Program both channels from the stored targets. Used on connect and
// whenever a frequency or a duty cycle changes.
static bool rpio_apply_pwm(indigo_device *device) {
	bool result = true;
	for (int i = 0; i < PWM_COUNT; i++) {
		double frequency = (AUX_GPIO_OUTLET_FREQUENCIES_PROPERTY->items + i)->number.target;
		double duty = (AUX_GPIO_OUTLET_DUTY_PROPERTY->items + i)->number.target;
		if (!rpio_sysfs_set_pwm(&PRIVATE_DATA->sysfs, i, frequency, duty)) {
			result = false;
		} else {
			// A running channel has to be restarted for a new period.
			rpio_sysfs_restart_pwm(&PRIVATE_DATA->sysfs, i);
		}
	}
	return result;
}

//- code

#pragma mark - High level code (aux)

static void aux_timer_callback(indigo_device *device) {
	if (!IS_CONNECTED) {
		return;
	}
	//+ aux.on_timer
	rpio_update_sensors(device);
	indigo_execute_handler_in(device, 1, aux_timer_callback);
	//- aux.on_timer
}

static void aux_connection_handler(indigo_device *device) {
	if (CONNECTION_CONNECTED_ITEM->sw.value) {
		bool connection_result = true;
		//+ aux.on_connect
		rpio_sysfs sysfs = { rpio_output_pins, OUTPUT_COUNT, rpio_input_pins, INPUT_COUNT, rpio_pwm_lines, X_AUX_PWM_ENABLED_ITEM->sw.value ? PWM_COUNT : 0, 0, -1, false };
		PRIVATE_DATA->sysfs = sysfs;
		rpio_sysfs_resolve(&PRIVATE_DATA->sysfs);
		// The existence of a PWM chip says nothing about whether any header pin
		// is routed to it, so PWM is used only when it is asked for.
		if (X_AUX_PWM_ENABLED_ITEM->sw.value && !PRIVATE_DATA->sysfs.pwm_present) {
			indigo_send_message(device, ALERT_PROPERTY, "PWM is enabled but no PWM chip was found, Outputs #1 and #2 stay plain GPIO");
		} else if (PRIVATE_DATA->sysfs.pwm_present) {
			indigo_send_message(device, IDLE_PROPERTY, "PWM drives Outputs #1 and #2");
		}
		AUX_GPIO_OUTLET_FREQUENCIES_PROPERTY->hidden = AUX_GPIO_OUTLET_DUTY_PROPERTY->hidden = !PRIVATE_DATA->sysfs.pwm_present;
		connection_result = rpio_sysfs_export_all(&PRIVATE_DATA->sysfs);
		if (connection_result) {
			int values[OUTPUT_COUNT];
			connection_result = rpio_sysfs_read_outputs(&PRIVATE_DATA->sysfs, values);
			if (connection_result) {
				for (int i = 0; i < OUTPUT_COUNT; i++) {
					(AUX_GPIO_OUTLETS_PROPERTY->items + i)->sw.value = values[i] != 0;
					PRIVATE_DATA->pulse_until[i] = 0;
				}
				if (PRIVATE_DATA->sysfs.pwm_present) {
					rpio_apply_pwm(device);
				}
			} else {
				rpio_sysfs_unexport_all(&PRIVATE_DATA->sysfs);
			}
		}
		//- aux.on_connect
		if (connection_result) {
			indigo_define_property(device, AUX_GPIO_OUTLETS_PROPERTY, NULL);
			indigo_define_property(device, AUX_OUTLET_PULSE_LENGTHS_PROPERTY, NULL);
			indigo_define_property(device, AUX_GPIO_OUTLET_FREQUENCIES_PROPERTY, NULL);
			indigo_define_property(device, AUX_GPIO_OUTLET_DUTY_PROPERTY, NULL);
			indigo_define_property(device, AUX_GPIO_SENSORS_PROPERTY, NULL);
			CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
			indigo_send_message(device, OK_PROPERTY, "Connected to %s", device->name);
		} else {
			CONNECTION_PROPERTY->state = INDIGO_ALERT_STATE;
			indigo_set_switch(CONNECTION_PROPERTY, CONNECTION_DISCONNECTED_ITEM, true);
			indigo_send_message(device, ALERT_PROPERTY, "Failed to connect to %s", device->name);
		}
	} else {
		indigo_cancel_pending_handlers(device);
		//+ aux.on_disconnect
		for (int i = 0; i < OUTPUT_COUNT; i++) {
			PRIVATE_DATA->pulse_until[i] = 0;
		}
		rpio_sysfs_unexport_all(&PRIVATE_DATA->sysfs);
		//- aux.on_disconnect
		indigo_delete_property(device, AUX_GPIO_OUTLETS_PROPERTY, NULL);
		indigo_delete_property(device, AUX_OUTLET_PULSE_LENGTHS_PROPERTY, NULL);
		indigo_delete_property(device, AUX_GPIO_OUTLET_FREQUENCIES_PROPERTY, NULL);
		indigo_delete_property(device, AUX_GPIO_OUTLET_DUTY_PROPERTY, NULL);
		indigo_delete_property(device, AUX_GPIO_SENSORS_PROPERTY, NULL);
		indigo_send_message(device, OK_PROPERTY, "Disconnected from %s", device->name);
		CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
	}
	indigo_aux_change_property(device, NULL, CONNECTION_PROPERTY);
	if (IS_CONNECTED) {
		indigo_execute_handler(device, aux_timer_callback);
	}
}

static void aux_outlet_names_handler(indigo_device *device) {
	AUX_OUTLET_NAMES_PROPERTY->state = INDIGO_OK_STATE;
	//+ aux.AUX_OUTLET_NAMES.on_change
	rpio_apply_outlet_names(device);
	//- aux.AUX_OUTLET_NAMES.on_change
	indigo_update_property(device, AUX_OUTLET_NAMES_PROPERTY, NULL);
}

static void aux_gpio_outlets_handler(indigo_device *device) {
	//+ aux.AUX_GPIO_OUTLETS.on_change
	if (rpio_set_outlets(device)) {
		INDIGO_UPDATE_PROPERTY_STATE(AUX_GPIO_OUTLETS_PROPERTY, INDIGO_OK_STATE, NULL);
	} else {
		INDIGO_UPDATE_PROPERTY_STATE(AUX_GPIO_OUTLETS_PROPERTY, INDIGO_ALERT_STATE, "Output operation failed, did you authorize?");
	}
	double delay = rpio_pulse_delay(device);
	if (delay >= 0) {
		// One finalizer serves every output, so a pulse started while
		// another is running replaces its wake-up.
		indigo_cancel_pending_handler(device, relay_pulse_finalizer);
		indigo_execute_handler_in(device, delay, relay_pulse_finalizer);
	}
	//- aux.AUX_GPIO_OUTLETS.on_change
}

static void aux_x_aux_pwm_handler(indigo_device *device) {
	X_AUX_PWM_PROPERTY->state = INDIGO_OK_STATE;
	//+ aux.X_AUX_PWM.on_change
	if (IS_CONNECTED) {
		indigo_send_message(device, IDLE_PROPERTY, "The PWM setting takes effect on the next connection");
	}
	//- aux.X_AUX_PWM.on_change
	indigo_update_property(device, X_AUX_PWM_PROPERTY, NULL);
}

static void aux_gpio_outlet_frequencies_handler(indigo_device *device) {
	AUX_GPIO_OUTLET_FREQUENCIES_PROPERTY->state = INDIGO_OK_STATE;
	//+ aux.AUX_GPIO_OUTLET_FREQUENCIES.on_change
	rpio_apply_pwm(device);
	//- aux.AUX_GPIO_OUTLET_FREQUENCIES.on_change
	indigo_update_property(device, AUX_GPIO_OUTLET_FREQUENCIES_PROPERTY, NULL);
}

static void aux_gpio_outlet_duty_handler(indigo_device *device) {
	AUX_GPIO_OUTLET_DUTY_PROPERTY->state = INDIGO_OK_STATE;
	//+ aux.AUX_GPIO_OUTLET_DUTY.on_change
	rpio_apply_pwm(device);
	//- aux.AUX_GPIO_OUTLET_DUTY.on_change
	indigo_update_property(device, AUX_GPIO_OUTLET_DUTY_PROPERTY, NULL);
}

static void aux_sensor_names_handler(indigo_device *device) {
	AUX_SENSOR_NAMES_PROPERTY->state = INDIGO_OK_STATE;
	//+ aux.AUX_SENSOR_NAMES.on_change
	rpio_apply_sensor_names(device);
	//- aux.AUX_SENSOR_NAMES.on_change
	indigo_update_property(device, AUX_SENSOR_NAMES_PROPERTY, NULL);
}

#pragma mark - Device API (aux)

static indigo_result aux_enumerate_properties(indigo_device *device, indigo_client *client, indigo_property *property);

static indigo_result aux_attach(indigo_device *device) {
	if (indigo_aux_attach(device, DRIVER_NAME, DRIVER_VERSION, INDIGO_INTERFACE_AUX_GPIO) == INDIGO_OK) {
		//+ aux.on_attach
		INFO_PROPERTY->count = 5;
		//- aux.on_attach
		AUX_OUTLET_NAMES_PROPERTY = indigo_init_text_property(NULL, device->name, AUX_OUTLET_NAMES_PROPERTY_NAME, AUX_RELAYS_GROUP, "Output names", INDIGO_OK_STATE, INDIGO_RW_PERM, 8);
		if (AUX_OUTLET_NAMES_PROPERTY == NULL) {
			return INDIGO_FAILED;
		}
		indigo_init_text_item(AUX_OUTLET_NAME_1_ITEM, AUX_GPIO_OUTLET_NAME_1_ITEM_NAME, "Output 1", "Output #1");
		indigo_init_text_item(AUX_OUTLET_NAME_2_ITEM, AUX_GPIO_OUTLET_NAME_2_ITEM_NAME, "Output 2", "Output #2");
		indigo_init_text_item(AUX_OUTLET_NAME_3_ITEM, AUX_GPIO_OUTLET_NAME_3_ITEM_NAME, "Output 3", "Output #3");
		indigo_init_text_item(AUX_OUTLET_NAME_4_ITEM, AUX_GPIO_OUTLET_NAME_4_ITEM_NAME, "Output 4", "Output #4");
		indigo_init_text_item(AUX_OUTLET_NAME_5_ITEM, AUX_GPIO_OUTLET_NAME_5_ITEM_NAME, "Output 5", "Output #5");
		indigo_init_text_item(AUX_OUTLET_NAME_6_ITEM, AUX_GPIO_OUTLET_NAME_6_ITEM_NAME, "Output 6", "Output #6");
		indigo_init_text_item(AUX_OUTLET_NAME_7_ITEM, AUX_GPIO_OUTLET_NAME_7_ITEM_NAME, "Output 7", "Output #7");
		indigo_init_text_item(AUX_OUTLET_NAME_8_ITEM, AUX_GPIO_OUTLET_NAME_8_ITEM_NAME, "Output 8", "Output #8");
		AUX_GPIO_OUTLETS_PROPERTY = indigo_init_switch_property(NULL, device->name, AUX_GPIO_OUTLETS_PROPERTY_NAME, AUX_RELAYS_GROUP, "Outputs", INDIGO_OK_STATE, INDIGO_RW_PERM, INDIGO_ANY_OF_MANY_RULE, 8);
		if (AUX_GPIO_OUTLETS_PROPERTY == NULL) {
			return INDIGO_FAILED;
		}
		indigo_init_switch_item(AUX_GPIO_OUTLET_1_ITEM, AUX_GPIO_OUTLETS_OUTLET_1_ITEM_NAME, "Output #1", false);
		indigo_init_switch_item(AUX_GPIO_OUTLET_2_ITEM, AUX_GPIO_OUTLETS_OUTLET_2_ITEM_NAME, "Output #2", false);
		indigo_init_switch_item(AUX_GPIO_OUTLET_3_ITEM, AUX_GPIO_OUTLETS_OUTLET_3_ITEM_NAME, "Output #3", false);
		indigo_init_switch_item(AUX_GPIO_OUTLET_4_ITEM, AUX_GPIO_OUTLETS_OUTLET_4_ITEM_NAME, "Output #4", false);
		indigo_init_switch_item(AUX_GPIO_OUTLET_5_ITEM, AUX_GPIO_OUTLETS_OUTLET_5_ITEM_NAME, "Output #5", false);
		indigo_init_switch_item(AUX_GPIO_OUTLET_6_ITEM, AUX_GPIO_OUTLETS_OUTLET_6_ITEM_NAME, "Output #6", false);
		indigo_init_switch_item(AUX_GPIO_OUTLET_7_ITEM, AUX_GPIO_OUTLETS_OUTLET_7_ITEM_NAME, "Output #7", false);
		indigo_init_switch_item(AUX_GPIO_OUTLET_8_ITEM, AUX_GPIO_OUTLETS_OUTLET_8_ITEM_NAME, "Output #8", false);
		AUX_OUTLET_PULSE_LENGTHS_PROPERTY = indigo_init_number_property(NULL, device->name, AUX_OUTLET_PULSE_LENGTHS_PROPERTY_NAME, AUX_RELAYS_GROUP, "Output pulse lengths (ms)", INDIGO_OK_STATE, INDIGO_RW_PERM, 8);
		if (AUX_OUTLET_PULSE_LENGTHS_PROPERTY == NULL) {
			return INDIGO_FAILED;
		}
		indigo_init_number_item(AUX_OUTLET_PULSE_LENGTHS_1_ITEM, AUX_GPIO_OUTLETS_OUTLET_1_ITEM_NAME, "Output #1", 0, 100000, 100, 0);
		indigo_init_number_item(AUX_OUTLET_PULSE_LENGTHS_2_ITEM, AUX_GPIO_OUTLETS_OUTLET_2_ITEM_NAME, "Output #2", 0, 100000, 100, 0);
		indigo_init_number_item(AUX_OUTLET_PULSE_LENGTHS_3_ITEM, AUX_GPIO_OUTLETS_OUTLET_3_ITEM_NAME, "Output #3", 0, 100000, 100, 0);
		indigo_init_number_item(AUX_OUTLET_PULSE_LENGTHS_4_ITEM, AUX_GPIO_OUTLETS_OUTLET_4_ITEM_NAME, "Output #4", 0, 100000, 100, 0);
		indigo_init_number_item(AUX_OUTLET_PULSE_LENGTHS_5_ITEM, AUX_GPIO_OUTLETS_OUTLET_5_ITEM_NAME, "Output #5", 0, 100000, 100, 0);
		indigo_init_number_item(AUX_OUTLET_PULSE_LENGTHS_6_ITEM, AUX_GPIO_OUTLETS_OUTLET_6_ITEM_NAME, "Output #6", 0, 100000, 100, 0);
		indigo_init_number_item(AUX_OUTLET_PULSE_LENGTHS_7_ITEM, AUX_GPIO_OUTLETS_OUTLET_7_ITEM_NAME, "Output #7", 0, 100000, 100, 0);
		indigo_init_number_item(AUX_OUTLET_PULSE_LENGTHS_8_ITEM, AUX_GPIO_OUTLETS_OUTLET_8_ITEM_NAME, "Output #8", 0, 100000, 100, 0);
		X_AUX_PWM_PROPERTY = indigo_init_switch_property(NULL, device->name, X_AUX_PWM_PROPERTY_NAME, AUX_RELAYS_GROUP, "PWM on Outputs #1 and #2", INDIGO_OK_STATE, INDIGO_RW_PERM, INDIGO_ONE_OF_MANY_RULE, 2);
		if (X_AUX_PWM_PROPERTY == NULL) {
			return INDIGO_FAILED;
		}
		indigo_init_switch_item(X_AUX_PWM_ENABLED_ITEM, X_AUX_PWM_ENABLED_ITEM_NAME, "Enabled", false);
		indigo_init_switch_item(X_AUX_PWM_DISABLED_ITEM, X_AUX_PWM_DISABLED_ITEM_NAME, "Disabled", true);
		AUX_GPIO_OUTLET_FREQUENCIES_PROPERTY = indigo_init_number_property(NULL, device->name, AUX_GPIO_OUTLET_FREQUENCIES_PROPERTY_NAME, AUX_RELAYS_GROUP, "PWM Frequencies (Hz)", INDIGO_OK_STATE, INDIGO_RW_PERM, 2);
		if (AUX_GPIO_OUTLET_FREQUENCIES_PROPERTY == NULL) {
			return INDIGO_FAILED;
		}
		indigo_init_number_item(AUX_GPIO_OUTLET_FREQUENCIES_OUTLET_1_ITEM, AUX_GPIO_OUTLETS_OUTLET_1_ITEM_NAME, "Output #1", 0.5, 1000000, 100, 100);
		indigo_init_number_item(AUX_GPIO_OUTLET_FREQUENCIES_OUTLET_2_ITEM, AUX_GPIO_OUTLETS_OUTLET_2_ITEM_NAME, "Output #2", 0.5, 1000000, 100, 100);
		AUX_GPIO_OUTLET_DUTY_PROPERTY = indigo_init_number_property(NULL, device->name, AUX_GPIO_OUTLET_DUTY_PROPERTY_NAME, AUX_RELAYS_GROUP, "PWM Duty cycles (%)", INDIGO_OK_STATE, INDIGO_RW_PERM, 2);
		if (AUX_GPIO_OUTLET_DUTY_PROPERTY == NULL) {
			return INDIGO_FAILED;
		}
		indigo_init_number_item(AUX_GPIO_OUTLET_DUTY_OUTLET_1_ITEM, AUX_GPIO_OUTLETS_OUTLET_1_ITEM_NAME, "Output #1", 0, 100, 1, 100);
		indigo_init_number_item(AUX_GPIO_OUTLET_DUTY_OUTLET_2_ITEM, AUX_GPIO_OUTLETS_OUTLET_2_ITEM_NAME, "Output #2", 0, 100, 1, 100);
		AUX_SENSOR_NAMES_PROPERTY = indigo_init_text_property(NULL, device->name, AUX_SENSOR_NAMES_PROPERTY_NAME, AUX_SENSORS_GROUP, "Input names", INDIGO_OK_STATE, INDIGO_RW_PERM, 8);
		if (AUX_SENSOR_NAMES_PROPERTY == NULL) {
			return INDIGO_FAILED;
		}
		indigo_init_text_item(AUX_SENSOR_NAME_1_ITEM, AUX_GPIO_SENSOR_NAME_1_ITEM_NAME, "Input 1", "Input #1");
		indigo_init_text_item(AUX_SENSOR_NAME_2_ITEM, AUX_GPIO_SENSOR_NAME_2_ITEM_NAME, "Input 2", "Input #2");
		indigo_init_text_item(AUX_SENSOR_NAME_3_ITEM, AUX_GPIO_SENSOR_NAME_3_ITEM_NAME, "Input 3", "Input #3");
		indigo_init_text_item(AUX_SENSOR_NAME_4_ITEM, AUX_GPIO_SENSOR_NAME_4_ITEM_NAME, "Input 4", "Input #4");
		indigo_init_text_item(AUX_SENSOR_NAME_5_ITEM, AUX_GPIO_SENSOR_NAME_5_ITEM_NAME, "Input 5", "Input #5");
		indigo_init_text_item(AUX_SENSOR_NAME_6_ITEM, AUX_GPIO_SENSOR_NAME_6_ITEM_NAME, "Input 6", "Input #6");
		indigo_init_text_item(AUX_SENSOR_NAME_7_ITEM, AUX_GPIO_SENSOR_NAME_7_ITEM_NAME, "Input 7", "Input #7");
		indigo_init_text_item(AUX_SENSOR_NAME_8_ITEM, AUX_GPIO_SENSOR_NAME_8_ITEM_NAME, "Input 8", "Input #8");
		AUX_GPIO_SENSORS_PROPERTY = indigo_init_number_property(NULL, device->name, AUX_GPIO_SENSORS_PROPERTY_NAME, AUX_SENSORS_GROUP, "Inputs", INDIGO_OK_STATE, INDIGO_RO_PERM, 8);
		if (AUX_GPIO_SENSORS_PROPERTY == NULL) {
			return INDIGO_FAILED;
		}
		indigo_init_number_item(AUX_GPIO_SENSOR_1_ITEM, AUX_GPIO_SENSOR_NAME_1_ITEM_NAME, "Input #1", 0, 1024, 1, 0);
		indigo_init_number_item(AUX_GPIO_SENSOR_2_ITEM, AUX_GPIO_SENSOR_NAME_2_ITEM_NAME, "Input #2", 0, 1024, 1, 0);
		indigo_init_number_item(AUX_GPIO_SENSOR_3_ITEM, AUX_GPIO_SENSOR_NAME_3_ITEM_NAME, "Input #3", 0, 1024, 1, 0);
		indigo_init_number_item(AUX_GPIO_SENSOR_4_ITEM, AUX_GPIO_SENSOR_NAME_4_ITEM_NAME, "Input #4", 0, 1024, 1, 0);
		indigo_init_number_item(AUX_GPIO_SENSOR_5_ITEM, AUX_GPIO_SENSOR_NAME_5_ITEM_NAME, "Input #5", 0, 1024, 1, 0);
		indigo_init_number_item(AUX_GPIO_SENSOR_6_ITEM, AUX_GPIO_SENSOR_NAME_6_ITEM_NAME, "Input #6", 0, 1024, 1, 0);
		indigo_init_number_item(AUX_GPIO_SENSOR_7_ITEM, AUX_GPIO_SENSOR_NAME_7_ITEM_NAME, "Input #7", 0, 1024, 1, 0);
		indigo_init_number_item(AUX_GPIO_SENSOR_8_ITEM, AUX_GPIO_SENSOR_NAME_8_ITEM_NAME, "Input #8", 0, 1024, 1, 0);
		INDIGO_DEVICE_ATTACH_LOG(DRIVER_NAME, device->name);
		return aux_enumerate_properties(device, NULL, NULL);
	}
	return INDIGO_FAILED;
}

static indigo_result aux_enumerate_properties(indigo_device *device, indigo_client *client, indigo_property *property) {
	if (IS_CONNECTED) {
		INDIGO_DEFINE_MATCHING_PROPERTY(AUX_GPIO_OUTLETS_PROPERTY);
		INDIGO_DEFINE_MATCHING_PROPERTY(AUX_OUTLET_PULSE_LENGTHS_PROPERTY);
		INDIGO_DEFINE_MATCHING_PROPERTY(AUX_GPIO_OUTLET_FREQUENCIES_PROPERTY);
		INDIGO_DEFINE_MATCHING_PROPERTY(AUX_GPIO_OUTLET_DUTY_PROPERTY);
		INDIGO_DEFINE_MATCHING_PROPERTY(AUX_GPIO_SENSORS_PROPERTY);
	}
	INDIGO_DEFINE_MATCHING_PROPERTY(AUX_OUTLET_NAMES_PROPERTY);
	INDIGO_DEFINE_MATCHING_PROPERTY(X_AUX_PWM_PROPERTY);
	INDIGO_DEFINE_MATCHING_PROPERTY(AUX_SENSOR_NAMES_PROPERTY);
	return indigo_aux_enumerate_properties(device, client, property);
}

static indigo_result aux_change_property(indigo_device *device, indigo_client *client, indigo_property *property) {
	if (indigo_property_match_changeable(CONNECTION_PROPERTY, property)) {
		if (!indigo_ignore_connection_change(device, property)) {
			indigo_property_copy_values(CONNECTION_PROPERTY, property, false);
			INDIGO_UPDATE_PROPERTY_STATE(CONNECTION_PROPERTY, INDIGO_BUSY_STATE, NULL);
			indigo_execute_handler(device, aux_connection_handler);
		}
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(AUX_OUTLET_NAMES_PROPERTY, property)) {
		INDIGO_COPY_VALUES_PROCESS_CHANGE(AUX_OUTLET_NAMES_PROPERTY, aux_outlet_names_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(AUX_GPIO_OUTLETS_PROPERTY, property)) {
		INDIGO_COPY_VALUES_PROCESS_CHANGE(AUX_GPIO_OUTLETS_PROPERTY, aux_gpio_outlets_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(AUX_OUTLET_PULSE_LENGTHS_PROPERTY, property)) {
		indigo_property_copy_values(AUX_OUTLET_PULSE_LENGTHS_PROPERTY, property, false);
		AUX_OUTLET_PULSE_LENGTHS_PROPERTY->state = INDIGO_OK_STATE;
		indigo_update_property(device, AUX_OUTLET_PULSE_LENGTHS_PROPERTY, NULL);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(X_AUX_PWM_PROPERTY, property)) {
		INDIGO_COPY_VALUES_PROCESS_CHANGE(X_AUX_PWM_PROPERTY, aux_x_aux_pwm_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(AUX_GPIO_OUTLET_FREQUENCIES_PROPERTY, property)) {
		INDIGO_COPY_VALUES_PROCESS_CHANGE(AUX_GPIO_OUTLET_FREQUENCIES_PROPERTY, aux_gpio_outlet_frequencies_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(AUX_GPIO_OUTLET_DUTY_PROPERTY, property)) {
		INDIGO_COPY_VALUES_PROCESS_CHANGE(AUX_GPIO_OUTLET_DUTY_PROPERTY, aux_gpio_outlet_duty_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(AUX_SENSOR_NAMES_PROPERTY, property)) {
		INDIGO_COPY_VALUES_PROCESS_CHANGE(AUX_SENSOR_NAMES_PROPERTY, aux_sensor_names_handler);
		return INDIGO_OK;
	} else if (indigo_property_match(CONFIG_PROPERTY, property)) {
		if (indigo_switch_match(CONFIG_SAVE_ITEM, property)) {
			indigo_save_property(device, NULL, AUX_OUTLET_NAMES_PROPERTY);
			indigo_save_property(device, NULL, AUX_OUTLET_PULSE_LENGTHS_PROPERTY);
			indigo_save_property(device, NULL, X_AUX_PWM_PROPERTY);
			indigo_save_property(device, NULL, AUX_GPIO_OUTLET_FREQUENCIES_PROPERTY);
			indigo_save_property(device, NULL, AUX_GPIO_OUTLET_DUTY_PROPERTY);
			indigo_save_property(device, NULL, AUX_SENSOR_NAMES_PROPERTY);
		}
	}
	return indigo_aux_change_property(device, client, property);
}

static indigo_result aux_detach(indigo_device *device) {
	if (IS_CONNECTED) {
		indigo_set_switch(CONNECTION_PROPERTY, CONNECTION_DISCONNECTED_ITEM, true);
		aux_connection_handler(device);
	}
	indigo_release_property(AUX_OUTLET_NAMES_PROPERTY);
	indigo_release_property(AUX_GPIO_OUTLETS_PROPERTY);
	indigo_release_property(AUX_OUTLET_PULSE_LENGTHS_PROPERTY);
	indigo_release_property(X_AUX_PWM_PROPERTY);
	indigo_release_property(AUX_GPIO_OUTLET_FREQUENCIES_PROPERTY);
	indigo_release_property(AUX_GPIO_OUTLET_DUTY_PROPERTY);
	indigo_release_property(AUX_SENSOR_NAMES_PROPERTY);
	indigo_release_property(AUX_GPIO_SENSORS_PROPERTY);
	INDIGO_DEVICE_DETACH_LOG(DRIVER_NAME, device->name);
	return indigo_aux_detach(device);
}

#pragma mark - Device templates

static indigo_device aux_template = INDIGO_DEVICE_INITIALIZER(AUX_DEVICE_NAME, aux_attach, aux_enumerate_properties, aux_change_property, NULL, aux_detach);

#pragma mark - Main code

indigo_result indigo_aux_rpio(indigo_driver_action action, indigo_driver_info *info) {
	static indigo_driver_action last_action = INDIGO_DRIVER_SHUTDOWN;
	static rpio_private_data *private_data = NULL;
	static indigo_device *aux = NULL;

	SET_DRIVER_INFO(info, DRIVER_LABEL, __FUNCTION__, DRIVER_VERSION, false, last_action);

	if (action == last_action) {
		return INDIGO_OK;
	}

	switch (action) {
		case INDIGO_DRIVER_INIT: {
			last_action = action;
			private_data = (rpio_private_data *)indigo_safe_malloc(sizeof(rpio_private_data));
			aux = (indigo_device *)indigo_safe_malloc_copy(sizeof(indigo_device), &aux_template);
			aux->private_data = private_data;
			indigo_attach_device(aux);
			break;

		}
		case INDIGO_DRIVER_SHUTDOWN: {
			VERIFY_NOT_CONNECTED(aux);
			last_action = action;
			if (aux != NULL) {
				indigo_detach_device(aux);
				indigo_safe_free(aux);
				aux = NULL;
			}
			if (private_data != NULL) {
				indigo_safe_free(private_data);
				private_data = NULL;
			}
			break;

		}
		case INDIGO_DRIVER_INFO:
			break;
	}

	return INDIGO_OK;
}
