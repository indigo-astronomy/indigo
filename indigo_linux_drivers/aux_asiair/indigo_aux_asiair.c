// Copyright (C) 2023 Joey Troy
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
// 1.0 by Joey Troy - Code modified from Rumen G. Bogdanovski

/** INDIGO ZWO Power Ports ASIAIR AUX driver
 \file indigo_aux_asiair.c
 */

#include "indigo_aux_asiair.h"

#define DRIVER_VERSION         0x0002
#define AUX_ASIAIR_NAME     "ZWO Power Ports ASIAIR"

#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <math.h>
#include <assert.h>
#include <errno.h>
#include <pthread.h>
#include <stdbool.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>

#include <indigo/indigo_driver_xml.h>
#include <indigo/indigo_io.h>
#include <indigo/indigo_client.h>
#include <indigo/indigo_dome_driver.h>
#include <indigo/indigo_aux_driver.h>

#define PRIVATE_DATA                      ((asiair_private_data *)device->private_data)

#define AUX_RELAYS_GROUP	"Power Pin Control"

#define AUX_OUTLET_NAMES_PROPERTY      (PRIVATE_DATA->outlet_names_property)
#define AUX_OUTLET_NAME_1_ITEM         (AUX_OUTLET_NAMES_PROPERTY->items + 0)
#define AUX_OUTLET_NAME_2_ITEM         (AUX_OUTLET_NAMES_PROPERTY->items + 1)
#define AUX_OUTLET_NAME_3_ITEM         (AUX_OUTLET_NAMES_PROPERTY->items + 2)
#define AUX_OUTLET_NAME_4_ITEM         (AUX_OUTLET_NAMES_PROPERTY->items + 3)

#define AUX_GPIO_OUTLET_PROPERTY      (PRIVATE_DATA->gpio_outlet_property)
#define AUX_GPIO_OUTLET_1_ITEM        (AUX_GPIO_OUTLET_PROPERTY->items + 0)
#define AUX_GPIO_OUTLET_2_ITEM        (AUX_GPIO_OUTLET_PROPERTY->items + 1)
#define AUX_GPIO_OUTLET_3_ITEM        (AUX_GPIO_OUTLET_PROPERTY->items + 2)
#define AUX_GPIO_OUTLET_4_ITEM        (AUX_GPIO_OUTLET_PROPERTY->items + 3)

#define AUX_OUTLET_PULSE_LENGTHS_PROPERTY      (PRIVATE_DATA->gpio_outlet_pulse_property)
#define AUX_OUTLET_PULSE_LENGTHS_1_ITEM        (AUX_OUTLET_PULSE_LENGTHS_PROPERTY->items + 0)
#define AUX_OUTLET_PULSE_LENGTHS_2_ITEM        (AUX_OUTLET_PULSE_LENGTHS_PROPERTY->items + 1)
#define AUX_OUTLET_PULSE_LENGTHS_3_ITEM        (AUX_OUTLET_PULSE_LENGTHS_PROPERTY->items + 2)
#define AUX_OUTLET_PULSE_LENGTHS_4_ITEM        (AUX_OUTLET_PULSE_LENGTHS_PROPERTY->items + 3)

#define AUX_GPIO_OUTLET_FREQUENCIES_PROPERTY         (PRIVATE_DATA->gpio_outlet_frequencies_property)
#define AUX_GPIO_OUTLET_FREQUENCIES_OUTLET_1_ITEM    (AUX_GPIO_OUTLET_FREQUENCIES_PROPERTY->items + 0)
#define AUX_GPIO_OUTLET_FREQUENCIES_OUTLET_2_ITEM    (AUX_GPIO_OUTLET_FREQUENCIES_PROPERTY->items + 1)

#define AUX_GPIO_OUTLET_DUTY_PROPERTY          (PRIVATE_DATA->gpio_outlet_duty_property)
#define AUX_GPIO_OUTLET_DUTY_OUTLET_1_ITEM     (AUX_GPIO_OUTLET_DUTY_PROPERTY->items + 0)
#define AUX_GPIO_OUTLET_DUTY_OUTLET_2_ITEM     (AUX_GPIO_OUTLET_DUTY_PROPERTY->items + 1)

typedef struct {

} logical_device_data;

typedef struct {
	int handle;
	int count_open;
	bool udp;
	pthread_mutex_t port_mutex;
	bool pwm_present;

	bool relay_active[4];
	indigo_timer *relay_timers[4];
	pthread_mutex_t relay_mutex;
	indigo_timer *pwm_settings_timer;

	indigo_property *outlet_names_property,
	                *gpio_outlet_property,
	                *gpio_outlet_pulse_property,
	                *gpio_outlet_frequencies_property,
	                *gpio_outlet_duty_property;
} asiair_private_data;

typedef struct {
	indigo_device *device;
	asiair_private_data *private_data;
} asiair_device_data;

static asiair_device_data device_data = {0};

static void create_device();
static void delete_device();


static int output_pins[] = {12, 13, 26, 18}; /* 12 -> PWM0, 18 -> PWM1 */

static bool asiair_pwm_present() {
	struct stat sb;
	if (stat("/sys/class/pwm/pwmchip0", &sb) == 0 && S_ISDIR(sb.st_mode)) {
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "PWM is present");
		return true;
	} else {
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "No PWM present");
		return false;
	}
}

static bool asiair_pwm_export(int channel) {
	char buffer[10];
	ssize_t bytes_written;
	int fd;

	fd = open("/sys/class/pwm/pwmchip0/export", O_WRONLY);
	if (fd < 0) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "Failed to open export for writing!");
		return false;
	}
	INDIGO_DRIVER_DEBUG(DRIVER_NAME, "EXPORT pwm Channel = %d", channel);
	bytes_written = snprintf(buffer, 10, "%d", channel);
	write(fd, buffer, bytes_written);
	close(fd);
	return true;
}


static bool asiair_pwm_unexport(int channel) {
	char buffer[10];
	ssize_t bytes;
	int fd;

	fd = open("/sys/class/pwm/pwmchip0/unexport", O_WRONLY);
	if (fd == -1) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "Failed to open unexport for writing!");
		return false;
	}

	INDIGO_DRIVER_DEBUG(DRIVER_NAME, "UNEXPORT PWM Channel = %d", channel);
	bytes = snprintf(buffer, 10, "%d", channel);
	write(fd, buffer, bytes);
	close(fd);
	return true;
}


static int asiair_pwm_get_enable(int channel, int *value) {
	char path[255];
	char value_str[3];
	int fd;

	if (value == NULL) return false;

	sprintf(path, "/sys/class/pwm/pwmchip0/pwm%d/enable", channel);
	fd = open(path, O_RDONLY);
	if (fd < 0) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "Failed to open PWM channel %d value for reading", channel);
		return false;
	}

	if (read(fd, value_str, 3) < 0) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "Failed to read value!");
		close(fd);
		return false;
	}
	close(fd);
	*value = atoi(value_str);
	return true;
}


static bool asiair_pwm_set_enable(int channel, int value) {
	char path[255];
	int fd;

	sprintf(path, "/sys/class/pwm/pwmchip0/pwm%d/enable", channel);
	fd = open(path, O_WRONLY);
	if (fd < 0) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "Failed to open PWM channel %d value for writing", channel);
		return false;
	}

	char val = value ? '1' : '0';
	INDIGO_DRIVER_DEBUG(DRIVER_NAME, "Value = %d (%c) channel = %d", value, val, channel);
	if (write(fd, &val, 1) != 1) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "Failed to write value!");
		close(fd);
		return false;
	}
	close(fd);
	return true;
}


static bool asiair_pwm_set(int channel, int period, int duty_cycle) {
	char path[255];
	char buf[100];
	int fd;

	sprintf(path, "/sys/class/pwm/pwmchip0/pwm%d/duty_cycle", channel);
	fd = open(path, O_WRONLY);
	if (fd < 0) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "Failed to open PWM channel %d duty_cycle for writing", channel);
		return false;
	}

	sprintf(buf, "%d", 0);
	INDIGO_DRIVER_DEBUG(DRIVER_NAME, "Clear duty_cycle = %d channel = %d", duty_cycle, channel);
	if (write(fd, buf, strlen(buf)) <= 0) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "Failed to clear PWM duty_cycle for channel %d!", channel);
		close(fd);
	}
	close(fd);

	sprintf(path, "/sys/class/pwm/pwmchip0/pwm%d/period", channel);
	fd = open(path, O_WRONLY);
	if (fd < 0) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "Failed to open PWM channel %d period for writing", channel);
		return false;
	}

	sprintf(buf, "%d", period);
	INDIGO_DRIVER_DEBUG(DRIVER_NAME, "Set period = %d channel = %d", period, channel);
	if (write(fd, buf, strlen(buf)) <= 0) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "Failed to set PWM period for channel %d!", channel);
		close(fd);
		return false;
	}
	close(fd);

	sprintf(path, "/sys/class/pwm/pwmchip0/pwm%d/duty_cycle", channel);
	fd = open(path, O_WRONLY);
	if (fd < 0) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "Failed to open PWM channel %d duty_cycle for writing", channel);
		return false;
	}

	sprintf(buf, "%d", duty_cycle);
	INDIGO_DRIVER_DEBUG(DRIVER_NAME, "Set duty_cycle = %d channel = %d", duty_cycle, channel);
	if (write(fd, buf, strlen(buf)) <= 0) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "Failed to set PWM duty_cycle for channel %d!", channel);
		close(fd);
		return false;
	}
	close(fd);
	return true;
}

static bool asiair_pwm_get(int channel, int *period, int *duty_cycle) {
	char path[255];
	char buf[100];
	int fd;

	sprintf(path, "/sys/class/pwm/pwmchip0/pwm%d/period", channel);
	fd = open(path, O_RDONLY);
	if (fd < 0) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "Failed to open PWM channel %d period for reading", channel);
		return false;
	}
	if (read(fd, buf, sizeof(buf)) <= 0) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "Failed to get PWM period for channel %d!", channel);
		close(fd);
		return false;
	}
	*period = atoi(buf);
	INDIGO_DRIVER_DEBUG(DRIVER_NAME, "Got period = %d channel = %d", *period, channel);
	close(fd);

	sprintf(path, "/sys/class/pwm/pwmchip0/pwm%d/duty_cycle", channel);
	fd = open(path, O_RDONLY);
	if (fd < 0) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "Failed to open PWM channel %d duty_cycle for reading", channel);
		return false;
	}

	if (read(fd, buf, sizeof(buf)) <= 0) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "Failed to get PWM duty_cycle for channel %d!", channel);
		close(fd);
		return false;
	}
	*duty_cycle = atoi(buf);
	INDIGO_DRIVER_DEBUG(DRIVER_NAME, "Got duty_cycle = %d channel = %d", *duty_cycle, channel);
	close(fd);
	return true;
}


static bool asiair_pin_export(int pin) {
	char buffer[10];
	ssize_t bytes_written;
	int fd;
	char path[256];
	struct stat sb = {0};

	sprintf(path, "/sys/class/gpio/gpio%d", pin);
	if (stat(path, &sb) == 0 && (S_ISDIR(sb.st_mode))) {
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "Pin #%d already exported!", pin);
		return true;
	}

	fd = open("/sys/class/gpio/export", O_WRONLY);
	if (fd < 0) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "Failed to open export for writing!");
		return false;
	}
	INDIGO_DRIVER_DEBUG(DRIVER_NAME, "EXPORT pin = %d", pin);
	bytes_written = snprintf(buffer, 10, "%d", pin);
	write(fd, buffer, bytes_written);
	close(fd);
	return true;
}


static bool asiair_pin_unexport(int pin) {
	char buffer[10];
	ssize_t bytes;
	int fd;

	fd = open("/sys/class/gpio/unexport", O_WRONLY);
	if (fd == -1) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "Failed to open unexport for writing!");
		return false;
	}

	INDIGO_DRIVER_DEBUG(DRIVER_NAME, "UNEXPORT Pin = %d", pin);
	bytes = snprintf(buffer, 10, "%d", pin);
	write(fd, buffer, bytes);
	close(fd);
	return true;
}


static bool asiair_get_pin_direction(int pin, bool *input) {
	char path[255];
	char direction_str[32] = {0};
	int fd;

	if (input == NULL) return false;

	sprintf(path, "/sys/class/gpio/gpio%d/direction", pin);
	fd = open(path, O_RDONLY);
	if (fd < 0) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "Failed to open gpio%d direction for reading", pin);
		return false;
	}

	if (read(fd, direction_str, 3) < 0) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "Failed to read direction!\n");
		close(fd);
		return false;
	}
	close(fd);
	if (direction_str[0] == 'i') {
		*input = true;
	} else if (direction_str[0] == 'o') {
		*input = false;
	} else {
		return false;
	}
	return true;
}


static bool asiair_set_output(int pin) {
	char path[256];
	int fd;
	bool is_input = false;

	if (asiair_get_pin_direction(pin, &is_input) && !is_input) {
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "Pin gpio%d direction is already output", pin);
		return true;
	}

	sprintf(path, "/sys/class/gpio/gpio%d/direction", pin);
	fd = open(path, O_WRONLY);
	if (fd < 0) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "Failed to open gpio%d direction for writing", pin);
		return false;
	}

	if (write(fd, "out", 3) < 0) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "Failed to set direction!");
		close(fd);
		return false;
	}

	close(fd);
	return true;
}


static int asiair_pin_read(int pin, int *value) {
	char path[255];
	char value_str[3];
	int fd;

	if (value == NULL) return false;

	sprintf(path, "/sys/class/gpio/gpio%d/value", pin);
	fd = open(path, O_RDONLY);
	if (fd < 0) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "Failed to open gpio%d value for reading", pin);
		return false;
	}

	if (read(fd, value_str, 3) < 0) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "Failed to read value!\n");
		close(fd);
		return false;
	}
	close(fd);
	*value = atoi(value_str);
	return true;
}


static bool asiair_pin_write(int pin, int value) {
	char path[255];
	int fd;

	sprintf(path, "/sys/class/gpio/gpio%d/value", pin);
	fd = open(path, O_WRONLY);
	if (fd < 0) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "Failed to open gpio%d value for writing", pin);
		return false;
	}

	char val = value ? '1' : '0';
	INDIGO_DRIVER_DEBUG(DRIVER_NAME, "Value = %d (%c) pin = %d", value, val, pin);
	if (write(fd, &val, 1) != 1) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "Failed to write value!");
		close(fd);
		return false;
	}

	close(fd);
	return true;
}

static bool asiair_set_output_line(uint16_t line, int value, bool use_pwm) {
	if (line >= 4) return false;
	if (use_pwm && line == 0) {
		return asiair_pwm_set_enable(0, value);
	} else if(use_pwm && line == 3) {
		return asiair_pwm_set_enable(1, value);
	} else {
		return asiair_pin_write(output_pins[line], value);
	}
}

static bool asiair_read_output_lines(int *values, bool use_pwm) {
	if (use_pwm) {
		if (!asiair_pwm_get_enable(0, &values[0])) return false;
		if (!asiair_pwm_get_enable(1, &values[3])) return false;
		if (!asiair_pin_read(output_pins[1], &values[1])) return false;
		if (!asiair_pin_read(output_pins[2], &values[2])) return false;
	} else {
		for (int i = 0; i < 4; i++) {
			if (!asiair_pin_read(output_pins[i], &values[i])) return false;
		}
	}
	return true;
}

bool asiair_export_all(bool use_pwm) {
	if (use_pwm) {
		if (!asiair_pwm_export(0)) return false;
		if (!asiair_pwm_export(1)) return false;
		if (!asiair_pin_export(output_pins[1])) return false;
		if (!asiair_pin_export(output_pins[2])) return false;
	} else {
		for (int i = 0; i < 4; i++) {
			if (!asiair_pin_export(output_pins[i])) return false;
		}
	}
	indigo_usleep(1000000);
	if (use_pwm) {
		if (!asiair_set_output(output_pins[1])) return false;
		if (!asiair_set_output(output_pins[2])) return false;
	} else {
		for (int i = 0; i < 4; i++) {
			if (!asiair_set_output(output_pins[i])) return false;
		}
	}
	return true;
}

bool asiair_unexport_all(bool use_pwm) {
	int first = 0;
	if (use_pwm) {
		if (!asiair_pwm_unexport(0)) return false;
		if (!asiair_pwm_unexport(1)) return false;
		if (!asiair_pin_unexport(output_pins[1])) return false;
		if (!asiair_pin_unexport(output_pins[2])) return false;
	} else {
		for (int i = 0; i < 4; i++) {
			if (!asiair_pin_unexport(output_pins[i])) return false;
		}
	}
	return true;
}

// --------------------------------------------------------------------------------- INDIGO AUX RELAYS device implementation

static int asiair_init_properties(indigo_device *device) {
	// -------------------------------------------------------------------------------- SIMULATION
	SIMULATION_PROPERTY->hidden = true;
	// -------------------------------------------------------------------------------- DEVICE_PORT
	DEVICE_PORT_PROPERTY->hidden = true;
	// --------------------------------------------------------------------------------
	INFO_PROPERTY->count = 5;
	// -------------------------------------------------------------------------------- OUTLET_NAMES
	AUX_OUTLET_NAMES_PROPERTY = indigo_init_text_property(NULL, device->name, AUX_OUTLET_NAMES_PROPERTY_NAME, AUX_RELAYS_GROUP, "Customize Output names", INDIGO_OK_STATE, INDIGO_RW_PERM, 4);
	if (AUX_OUTLET_NAMES_PROPERTY == NULL)
		return INDIGO_FAILED;
	indigo_init_text_item(AUX_OUTLET_NAME_1_ITEM, AUX_GPIO_OUTLET_NAME_1_ITEM_NAME, "Output 1", "Power #1");
	indigo_init_text_item(AUX_OUTLET_NAME_2_ITEM, AUX_GPIO_OUTLET_NAME_2_ITEM_NAME, "Output 2", "Power #2");
	indigo_init_text_item(AUX_OUTLET_NAME_3_ITEM, AUX_GPIO_OUTLET_NAME_3_ITEM_NAME, "Output 3", "Power #3");
	indigo_init_text_item(AUX_OUTLET_NAME_4_ITEM, AUX_GPIO_OUTLET_NAME_4_ITEM_NAME, "Output 4", "Power #4");
	// -------------------------------------------------------------------------------- GPIO OUTLETS
	AUX_GPIO_OUTLET_PROPERTY = indigo_init_switch_property(NULL, device->name, AUX_GPIO_OUTLETS_PROPERTY_NAME, AUX_RELAYS_GROUP, "Outputs", INDIGO_OK_STATE, INDIGO_RW_PERM, INDIGO_ANY_OF_MANY_RULE, 4);
	if (AUX_GPIO_OUTLET_PROPERTY == NULL)
		return INDIGO_FAILED;
	indigo_init_switch_item(AUX_GPIO_OUTLET_1_ITEM, AUX_GPIO_OUTLETS_OUTLET_1_ITEM_NAME, "Power #1", false);
	indigo_init_switch_item(AUX_GPIO_OUTLET_2_ITEM, AUX_GPIO_OUTLETS_OUTLET_2_ITEM_NAME, "Power #2", false);
	indigo_init_switch_item(AUX_GPIO_OUTLET_3_ITEM, AUX_GPIO_OUTLETS_OUTLET_3_ITEM_NAME, "Power #3", false);
	indigo_init_switch_item(AUX_GPIO_OUTLET_4_ITEM, AUX_GPIO_OUTLETS_OUTLET_4_ITEM_NAME, "Power #4", false);
	// -------------------------------------------------------------------------------- GPIO PULSE OUTLETS
	AUX_OUTLET_PULSE_LENGTHS_PROPERTY = indigo_init_number_property(NULL, device->name, AUX_OUTLET_PULSE_LENGTHS_PROPERTY_NAME, AUX_RELAYS_GROUP, "Output pulse lengths (ms)", INDIGO_OK_STATE, INDIGO_RW_PERM, 4);
	if (AUX_OUTLET_PULSE_LENGTHS_PROPERTY == NULL)
		return INDIGO_FAILED;
	indigo_init_number_item(AUX_OUTLET_PULSE_LENGTHS_1_ITEM, AUX_GPIO_OUTLETS_OUTLET_1_ITEM_NAME, "Output #1", 0, 100000, 100, 0);
	indigo_init_number_item(AUX_OUTLET_PULSE_LENGTHS_2_ITEM, AUX_GPIO_OUTLETS_OUTLET_2_ITEM_NAME, "Output #2", 0, 100000, 100, 0);
	indigo_init_number_item(AUX_OUTLET_PULSE_LENGTHS_3_ITEM, AUX_GPIO_OUTLETS_OUTLET_3_ITEM_NAME, "Output #3", 0, 100000, 100, 0);
	indigo_init_number_item(AUX_OUTLET_PULSE_LENGTHS_4_ITEM, AUX_GPIO_OUTLETS_OUTLET_4_ITEM_NAME, "Output #4", 0, 100000, 100, 0);
	// -------------------------------------------------------------------------------- AUX_GPIO_OUTLET_FREQUENCIES
	AUX_GPIO_OUTLET_FREQUENCIES_PROPERTY = indigo_init_number_property(NULL, device->name, AUX_GPIO_OUTLET_FREQUENCIES_PROPERTY_NAME, AUX_RELAYS_GROUP, "PWM Frequencies (Hz)", INDIGO_OK_STATE, INDIGO_RW_PERM, 2);
	if (AUX_GPIO_OUTLET_FREQUENCIES_PROPERTY == NULL)
		return INDIGO_FAILED;
	indigo_init_number_item(AUX_GPIO_OUTLET_FREQUENCIES_OUTLET_1_ITEM, AUX_GPIO_OUTLETS_OUTLET_1_ITEM_NAME, "Output #1", 0.5, 1000000, 100, 100);
	indigo_init_number_item(AUX_GPIO_OUTLET_FREQUENCIES_OUTLET_2_ITEM, AUX_GPIO_OUTLETS_OUTLET_2_ITEM_NAME, "Output #2", 0.5, 1000000, 100, 100);
	// -------------------------------------------------------------------------------- AUX_GPIO_OUTLET_DUTY_CYCLES
	AUX_GPIO_OUTLET_DUTY_PROPERTY = indigo_init_number_property(NULL, device->name, AUX_GPIO_OUTLET_DUTY_PROPERTY_NAME, AUX_RELAYS_GROUP, "PWM Duty cycles (%)", INDIGO_OK_STATE, INDIGO_RW_PERM, 2);
	if (AUX_GPIO_OUTLET_DUTY_PROPERTY == NULL)
		return INDIGO_FAILED;
	indigo_init_number_item(AUX_GPIO_OUTLET_DUTY_OUTLET_1_ITEM, AUX_GPIO_OUTLETS_OUTLET_1_ITEM_NAME, "Output #1", 0, 100, 1, 100);
	indigo_init_number_item(AUX_GPIO_OUTLET_DUTY_OUTLET_2_ITEM, AUX_GPIO_OUTLETS_OUTLET_2_ITEM_NAME, "Output #2", 0, 100, 1, 100);
	//---------------------------------------------------------------------------
	return INDIGO_OK;
}


static void pwm_settings_timer_callback(indigo_device *device) {
	if (PRIVATE_DATA->pwm_present) {
		int period, duty_cycle;
		if (!asiair_pwm_get(0, &period, &duty_cycle)) {
			AUX_GPIO_OUTLET_FREQUENCIES_PROPERTY->state = INDIGO_ALERT_STATE;
			AUX_GPIO_OUTLET_DUTY_PROPERTY->state = INDIGO_ALERT_STATE;
		} else {
			AUX_GPIO_OUTLET_DUTY_OUTLET_1_ITEM->number.value = AUX_GPIO_OUTLET_DUTY_OUTLET_1_ITEM->number.target = (double)(duty_cycle) / period * 100;
			AUX_GPIO_OUTLET_FREQUENCIES_OUTLET_1_ITEM->number.value = AUX_GPIO_OUTLET_FREQUENCIES_OUTLET_1_ITEM->number.target = 1 / (period / 1e9);
		}
		if (!asiair_pwm_get(1, &period, &duty_cycle)) {
			AUX_GPIO_OUTLET_FREQUENCIES_PROPERTY->state = INDIGO_ALERT_STATE;
			AUX_GPIO_OUTLET_DUTY_PROPERTY->state = INDIGO_ALERT_STATE;
		} else {
			AUX_GPIO_OUTLET_DUTY_OUTLET_2_ITEM->number.value = AUX_GPIO_OUTLET_DUTY_OUTLET_2_ITEM->number.target = (double)(duty_cycle) / period * 100;
			AUX_GPIO_OUTLET_FREQUENCIES_OUTLET_2_ITEM->number.value = AUX_GPIO_OUTLET_FREQUENCIES_OUTLET_2_ITEM->number.target = 1 / (period / 1e9);
		}
	}

	indigo_update_property(device, AUX_GPIO_OUTLET_FREQUENCIES_PROPERTY, NULL);
	indigo_update_property(device, AUX_GPIO_OUTLET_DUTY_PROPERTY, NULL);
	indigo_reschedule_timer(device, 1, &PRIVATE_DATA->pwm_settings_timer);
}


static void relay_1_timer_callback(indigo_device *device) {
	pthread_mutex_lock(&PRIVATE_DATA->relay_mutex);
	PRIVATE_DATA->relay_active[0] = false;
	asiair_set_output_line(0, 0, PRIVATE_DATA->pwm_present);
	AUX_GPIO_OUTLET_1_ITEM->sw.value = false;
	indigo_update_property(device, AUX_GPIO_OUTLET_PROPERTY, NULL);
	pthread_mutex_unlock(&PRIVATE_DATA->relay_mutex);
}

static void relay_2_timer_callback(indigo_device *device) {
	pthread_mutex_lock(&PRIVATE_DATA->relay_mutex);
	PRIVATE_DATA->relay_active[1] = false;
	asiair_set_output_line(1, 0, PRIVATE_DATA->pwm_present);
	AUX_GPIO_OUTLET_2_ITEM->sw.value = false;
	indigo_update_property(device, AUX_GPIO_OUTLET_PROPERTY, NULL);
	pthread_mutex_unlock(&PRIVATE_DATA->relay_mutex);
}

static void relay_3_timer_callback(indigo_device *device) {
	pthread_mutex_lock(&PRIVATE_DATA->relay_mutex);
	PRIVATE_DATA->relay_active[2] = false;
	asiair_set_output_line(2, 0, PRIVATE_DATA->pwm_present);
	AUX_GPIO_OUTLET_3_ITEM->sw.value = false;
	indigo_update_property(device, AUX_GPIO_OUTLET_PROPERTY, NULL);
	pthread_mutex_unlock(&PRIVATE_DATA->relay_mutex);
}

static void relay_4_timer_callback(indigo_device *device) {
	pthread_mutex_lock(&PRIVATE_DATA->relay_mutex);
	PRIVATE_DATA->relay_active[3] = false;
	asiair_set_output_line(3, 0, PRIVATE_DATA->pwm_present);
	AUX_GPIO_OUTLET_4_ITEM->sw.value = false;
	indigo_update_property(device, AUX_GPIO_OUTLET_PROPERTY, NULL);
	pthread_mutex_unlock(&PRIVATE_DATA->relay_mutex);
}



static void (*relay_timer_callbacks[])(indigo_device*) = {
	relay_1_timer_callback,
	relay_2_timer_callback,
	relay_3_timer_callback,
	relay_4_timer_callback,
};


static bool set_gpio_outlets(indigo_device *device) {
	bool success = true;
	int relay_value[4];

	if (!asiair_read_output_lines(relay_value, PRIVATE_DATA->pwm_present)) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "asiair_pin_read(%d) failed", PRIVATE_DATA->handle);
		return false;
	}

	for (int i = 0; i < 4; i++) {
		if ((AUX_GPIO_OUTLET_PROPERTY->items + i)->sw.value != relay_value[i]) {
			if (((AUX_OUTLET_PULSE_LENGTHS_PROPERTY->items + i)->number.value > 0) && (AUX_GPIO_OUTLET_PROPERTY->items + i)->sw.value && !PRIVATE_DATA->relay_active[i]) {
				if (!asiair_set_output_line(i, (int)(AUX_OUTLET_PULSE_LENGTHS_PROPERTY->items + i)->number.value, PRIVATE_DATA->pwm_present)) {
					INDIGO_DRIVER_ERROR(DRIVER_NAME, "asiair_pin_write(%d) failed, did you authorize?", PRIVATE_DATA->handle);
					success = false;
				} else {
					PRIVATE_DATA->relay_active[i] = true;
					indigo_set_timer(device, ((AUX_OUTLET_PULSE_LENGTHS_PROPERTY->items + i)->number.value)/1000.0, relay_timer_callbacks[i], &PRIVATE_DATA->relay_timers[i]);
				}
			} else if ((AUX_OUTLET_PULSE_LENGTHS_PROPERTY->items + i)->number.value == 0 || (!(AUX_GPIO_OUTLET_PROPERTY->items + i)->sw.value && !PRIVATE_DATA->relay_active[i])) {
				if (!asiair_set_output_line(i, (int)(AUX_GPIO_OUTLET_PROPERTY->items + i)->sw.value, PRIVATE_DATA->pwm_present)) {
					INDIGO_DRIVER_ERROR(DRIVER_NAME, "asiair_pin_write(%d) failed, did you authorize?", PRIVATE_DATA->handle);
					success = false;
				}
			}
		}
	}
	return success;
}


static indigo_result aux_enumerate_properties(indigo_device *device, indigo_client *client, indigo_property *property) {
	if (IS_CONNECTED) {
		indigo_define_matching_property(AUX_GPIO_OUTLET_PROPERTY);
		indigo_define_matching_property(AUX_OUTLET_PULSE_LENGTHS_PROPERTY);
		indigo_define_matching_property(AUX_GPIO_OUTLET_FREQUENCIES_PROPERTY);
		indigo_define_matching_property(AUX_GPIO_OUTLET_DUTY_PROPERTY);
	}
	indigo_define_matching_property(AUX_OUTLET_NAMES_PROPERTY);

	return indigo_aux_enumerate_properties(device, NULL, NULL);
}


static indigo_result aux_attach(indigo_device *device) {
	assert(device != NULL);
	assert(PRIVATE_DATA != NULL);
	if (indigo_aux_attach(device, DRIVER_NAME, DRIVER_VERSION, INDIGO_INTERFACE_AUX_GPIO) == INDIGO_OK) {
		pthread_mutex_init(&PRIVATE_DATA->relay_mutex, NULL);
		// --------------------------------------------------------------------------------
		if (asiair_init_properties(device) != INDIGO_OK) return INDIGO_FAILED;
		INDIGO_DEVICE_ATTACH_LOG(DRIVER_NAME, device->name);
		return aux_enumerate_properties(device, NULL, NULL);
	}
	return INDIGO_FAILED;
}


static void handle_aux_connect_property(indigo_device *device) {
	if (CONNECTION_CONNECTED_ITEM->sw.value) {
		PRIVATE_DATA->pwm_present = asiair_pwm_present();
		if(PRIVATE_DATA->pwm_present) {
			AUX_GPIO_OUTLET_DUTY_PROPERTY->hidden = false;
			AUX_GPIO_OUTLET_FREQUENCIES_PROPERTY->hidden = false;
			indigo_send_message(device, "PWM on Outputs #1 and #4 is present");
		} else {
			AUX_GPIO_OUTLET_DUTY_PROPERTY->hidden = true;
			AUX_GPIO_OUTLET_FREQUENCIES_PROPERTY->hidden = true;
			indigo_send_message(device, "No PWM channels found");
		}
		if (asiair_export_all(PRIVATE_DATA->pwm_present)) {
			char board[INDIGO_VALUE_SIZE] = "N/A";
			char firmware[INDIGO_VALUE_SIZE] = "N/A";
			indigo_copy_value(INFO_DEVICE_MODEL_ITEM->text.value, board);
			indigo_copy_value(INFO_DEVICE_FW_REVISION_ITEM->text.value, firmware);
			indigo_update_property(device, INFO_PROPERTY, NULL);

			int relay_value[4];
			if (!asiair_read_output_lines(relay_value, PRIVATE_DATA->pwm_present)) {
				INDIGO_DRIVER_ERROR(DRIVER_NAME, "asiair_pin_read(%d) failed", PRIVATE_DATA->handle);
				AUX_GPIO_OUTLET_PROPERTY->state = INDIGO_ALERT_STATE;
			} else {
				if (PRIVATE_DATA->pwm_present) {
					int period, duty_cycle;
					period = (int)(1 / AUX_GPIO_OUTLET_FREQUENCIES_OUTLET_1_ITEM->number.target * 1e9);
					duty_cycle = (int)(period * AUX_GPIO_OUTLET_DUTY_OUTLET_1_ITEM->number.target / 100.0);
					asiair_pwm_set(0, period, duty_cycle);
					period = (int)(1 / AUX_GPIO_OUTLET_FREQUENCIES_OUTLET_2_ITEM->number.target * 1e9);
					duty_cycle = (int)(period * AUX_GPIO_OUTLET_DUTY_OUTLET_2_ITEM->number.target / 100.0);
					asiair_pwm_set(1, period, duty_cycle);

					/* Reset PWM lines if enabled */
					if (relay_value[0] == 1) {
						asiair_pwm_set_enable(0, false);
						asiair_pwm_set_enable(0, true);
					}
					if (relay_value[3] == 1) {
						asiair_pwm_set_enable(1, false);
						asiair_pwm_set_enable(1, true);
					}
					AUX_GPIO_OUTLET_FREQUENCIES_PROPERTY->hidden = false;
					AUX_GPIO_OUTLET_DUTY_PROPERTY->hidden = false;
				} else {
					AUX_GPIO_OUTLET_FREQUENCIES_PROPERTY->hidden = true;
					AUX_GPIO_OUTLET_DUTY_PROPERTY->hidden = true;
				}

				for (int i = 0; i < 4; i++) {
					(AUX_GPIO_OUTLET_PROPERTY->items + i)->sw.value = relay_value[i];
					PRIVATE_DATA->relay_active[i] = false;
				}
			}

			indigo_define_property(device, AUX_GPIO_OUTLET_PROPERTY, NULL);
			indigo_define_property(device, AUX_OUTLET_PULSE_LENGTHS_PROPERTY, NULL);
			indigo_define_property(device, AUX_GPIO_OUTLET_FREQUENCIES_PROPERTY, NULL);
			indigo_define_property(device, AUX_GPIO_OUTLET_DUTY_PROPERTY, NULL);
			indigo_set_timer(device, 0, pwm_settings_timer_callback, &PRIVATE_DATA->pwm_settings_timer);
			CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
		} else {
			CONNECTION_PROPERTY->state = INDIGO_ALERT_STATE;
			indigo_set_switch(CONNECTION_PROPERTY, CONNECTION_DISCONNECTED_ITEM, false);
		}
	} else {
		indigo_update_property(device, CONNECTION_PROPERTY, NULL);
		for (int i = 0; i < 4; i++) {
			indigo_cancel_timer_sync(device, &PRIVATE_DATA->relay_timers[i]);
		}
		indigo_cancel_timer_sync(device, &PRIVATE_DATA->pwm_settings_timer);
		asiair_unexport_all(PRIVATE_DATA->pwm_present);
		indigo_delete_property(device, AUX_GPIO_OUTLET_PROPERTY, NULL);
		indigo_delete_property(device, AUX_OUTLET_PULSE_LENGTHS_PROPERTY, NULL);
		indigo_delete_property(device, AUX_GPIO_OUTLET_FREQUENCIES_PROPERTY, NULL);
		indigo_delete_property(device, AUX_GPIO_OUTLET_DUTY_PROPERTY, NULL);
		CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
	}
	indigo_aux_change_property(device, NULL, CONNECTION_PROPERTY);
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
		indigo_set_timer(device, 0, handle_aux_connect_property, NULL);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(AUX_OUTLET_NAMES_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- X_AUX_OUTLET_NAMES
		indigo_property_copy_values(AUX_OUTLET_NAMES_PROPERTY, property, false);
		if (IS_CONNECTED) {
			indigo_delete_property(device, AUX_GPIO_OUTLET_PROPERTY, NULL);
			indigo_delete_property(device, AUX_OUTLET_PULSE_LENGTHS_PROPERTY, NULL);
			indigo_delete_property(device, AUX_GPIO_OUTLET_FREQUENCIES_PROPERTY, NULL);
			indigo_delete_property(device, AUX_GPIO_OUTLET_DUTY_PROPERTY, NULL);
		}
		snprintf(AUX_GPIO_OUTLET_1_ITEM->label, INDIGO_NAME_SIZE, "%s", AUX_OUTLET_NAME_1_ITEM->text.value);
		snprintf(AUX_GPIO_OUTLET_2_ITEM->label, INDIGO_NAME_SIZE, "%s", AUX_OUTLET_NAME_2_ITEM->text.value);
		snprintf(AUX_GPIO_OUTLET_3_ITEM->label, INDIGO_NAME_SIZE, "%s", AUX_OUTLET_NAME_3_ITEM->text.value);
		snprintf(AUX_GPIO_OUTLET_4_ITEM->label, INDIGO_NAME_SIZE, "%s", AUX_OUTLET_NAME_4_ITEM->text.value);

		snprintf(AUX_OUTLET_PULSE_LENGTHS_1_ITEM->label, INDIGO_NAME_SIZE, "%s", AUX_OUTLET_NAME_1_ITEM->text.value);
		snprintf(AUX_OUTLET_PULSE_LENGTHS_2_ITEM->label, INDIGO_NAME_SIZE, "%s", AUX_OUTLET_NAME_2_ITEM->text.value);
		snprintf(AUX_OUTLET_PULSE_LENGTHS_3_ITEM->label, INDIGO_NAME_SIZE, "%s", AUX_OUTLET_NAME_3_ITEM->text.value);
		snprintf(AUX_OUTLET_PULSE_LENGTHS_4_ITEM->label, INDIGO_NAME_SIZE, "%s", AUX_OUTLET_NAME_4_ITEM->text.value);

		snprintf(AUX_GPIO_OUTLET_FREQUENCIES_OUTLET_1_ITEM->label, INDIGO_NAME_SIZE, "%s", AUX_OUTLET_NAME_1_ITEM->text.value);
		snprintf(AUX_GPIO_OUTLET_FREQUENCIES_OUTLET_2_ITEM->label, INDIGO_NAME_SIZE, "%s", AUX_OUTLET_NAME_2_ITEM->text.value);

		snprintf(AUX_GPIO_OUTLET_DUTY_OUTLET_1_ITEM->label, INDIGO_NAME_SIZE, "%s", AUX_OUTLET_NAME_1_ITEM->text.value);
		snprintf(AUX_GPIO_OUTLET_DUTY_OUTLET_2_ITEM->label, INDIGO_NAME_SIZE, "%s", AUX_OUTLET_NAME_2_ITEM->text.value);

		AUX_OUTLET_NAMES_PROPERTY->state = INDIGO_OK_STATE;
		if (IS_CONNECTED) {
			indigo_define_property(device, AUX_GPIO_OUTLET_PROPERTY, NULL);
			indigo_define_property(device, AUX_OUTLET_PULSE_LENGTHS_PROPERTY, NULL);
			indigo_define_property(device, AUX_GPIO_OUTLET_FREQUENCIES_PROPERTY, NULL);
			indigo_define_property(device, AUX_GPIO_OUTLET_DUTY_PROPERTY, NULL);
		}
		indigo_update_property(device, AUX_OUTLET_NAMES_PROPERTY, NULL);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(AUX_GPIO_OUTLET_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- AUX_GPIO_OUTLET
		indigo_property_copy_values(AUX_GPIO_OUTLET_PROPERTY, property, false);
		if (set_gpio_outlets(device) == true) {
			AUX_GPIO_OUTLET_PROPERTY->state = INDIGO_OK_STATE;
			indigo_update_property(device, AUX_GPIO_OUTLET_PROPERTY, NULL);
		} else {
			AUX_GPIO_OUTLET_PROPERTY->state = INDIGO_ALERT_STATE;
			indigo_update_property(device, AUX_GPIO_OUTLET_PROPERTY, "Output operation failed, did you authorize?");
		}
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(AUX_OUTLET_PULSE_LENGTHS_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- AUX_OUTLET_PULSE_LENGTHS
		indigo_property_copy_values(AUX_OUTLET_PULSE_LENGTHS_PROPERTY, property, false);
		indigo_update_property(device, AUX_OUTLET_PULSE_LENGTHS_PROPERTY, NULL);
		return INDIGO_OK;

	} else if (indigo_property_match_changeable(AUX_GPIO_OUTLET_FREQUENCIES_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- AUX_GPIO_OUTLET_FREQUENCIES
		indigo_property_copy_values(AUX_GPIO_OUTLET_FREQUENCIES_PROPERTY, property, false);
		int period = 0, duty_cycle = 0;
		asiair_pwm_get(0, &period, &duty_cycle);
		double freq = 1 / (period / 1e9);
		if (AUX_GPIO_OUTLET_FREQUENCIES_OUTLET_1_ITEM->number.target != freq) {
			period = (int)(1 / AUX_GPIO_OUTLET_FREQUENCIES_OUTLET_1_ITEM->number.target * 1e9);
			duty_cycle = (int)(period * AUX_GPIO_OUTLET_DUTY_OUTLET_1_ITEM->number.target / 100.0);
			asiair_pwm_set(0, period, duty_cycle);
		}

		asiair_pwm_get(1, &period, &duty_cycle);
		freq = 1 / (period / 1e9);
		if (AUX_GPIO_OUTLET_FREQUENCIES_OUTLET_2_ITEM->number.target != freq) {
			period = (int)(1 / AUX_GPIO_OUTLET_FREQUENCIES_OUTLET_2_ITEM->number.target * 1e9);
			duty_cycle = (int)(period * AUX_GPIO_OUTLET_DUTY_OUTLET_2_ITEM->number.target / 100.0);
			asiair_pwm_set(1, period, duty_cycle);
		}

		indigo_update_property(device, AUX_GPIO_OUTLET_FREQUENCIES_PROPERTY, NULL);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(AUX_GPIO_OUTLET_DUTY_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- AUX_GPIO_OUTLET_DUTY_PROPERTY
		indigo_property_copy_values(AUX_GPIO_OUTLET_DUTY_PROPERTY, property, false);
		int period = 0, duty_cycle = 0;
		asiair_pwm_get(0, &period, &duty_cycle);
		double duty = (double)(duty_cycle) / period * 100;
		if (AUX_GPIO_OUTLET_DUTY_OUTLET_1_ITEM->number.target != duty) {
			duty_cycle = (int)(period * AUX_GPIO_OUTLET_DUTY_OUTLET_1_ITEM->number.target / 100.0);
			asiair_pwm_set(0, period, duty_cycle);
		}

		asiair_pwm_get(1, &period, &duty_cycle);
		duty = (double)(duty_cycle) / period * 100;
		if (AUX_GPIO_OUTLET_DUTY_OUTLET_2_ITEM->number.target != duty) {
			duty_cycle = (int)(period * AUX_GPIO_OUTLET_DUTY_OUTLET_2_ITEM->number.target / 100.0);
			asiair_pwm_set(1, period, duty_cycle);
		}

		indigo_update_property(device, AUX_GPIO_OUTLET_DUTY_PROPERTY, NULL);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(CONFIG_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- CONFIG
		if (indigo_switch_match(CONFIG_SAVE_ITEM, property)) {
			indigo_save_property(device, NULL, AUX_OUTLET_NAMES_PROPERTY);
			indigo_save_property(device, NULL, AUX_OUTLET_PULSE_LENGTHS_PROPERTY);
			indigo_save_property(device, NULL, AUX_GPIO_OUTLET_FREQUENCIES_PROPERTY);
			indigo_save_property(device, NULL, AUX_GPIO_OUTLET_DUTY_PROPERTY);
		}
	}
	// --------------------------------------------------------------------------------
	return indigo_aux_change_property(device, client, property);
}


static indigo_result aux_detach(indigo_device *device) {
	assert(device != NULL);
	if (IS_CONNECTED) {
		indigo_set_switch(CONNECTION_PROPERTY, CONNECTION_DISCONNECTED_ITEM, true);
		handle_aux_connect_property(device);
	}
	indigo_release_property(AUX_GPIO_OUTLET_PROPERTY);
	indigo_release_property(AUX_OUTLET_PULSE_LENGTHS_PROPERTY);
	indigo_release_property(AUX_GPIO_OUTLET_FREQUENCIES_PROPERTY);
	indigo_release_property(AUX_GPIO_OUTLET_DUTY_PROPERTY);
	INDIGO_DEVICE_DETACH_LOG(DRIVER_NAME, device->name);

	indigo_delete_property(device, AUX_OUTLET_NAMES_PROPERTY, NULL);
	indigo_release_property(AUX_OUTLET_NAMES_PROPERTY);

	return indigo_aux_detach(device);
}

// --------------------------------------------------------------------------------

//static int device_number = 0;

static void create_device() {
	static indigo_device aux_template = INDIGO_DEVICE_INITIALIZER(
		AUX_ASIAIR_NAME,
		aux_attach,
		aux_enumerate_properties,
		aux_change_property,
		NULL,
		aux_detach
	);

	if (device_data.device != NULL) return;

	if (device_data.private_data == NULL) {
		device_data.private_data = indigo_safe_malloc(sizeof(asiair_private_data));
		pthread_mutex_init(&device_data.private_data->port_mutex, NULL);
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "ADD: PRIVATE_DATA");
	}

	device_data.device = indigo_safe_malloc_copy(sizeof(indigo_device), &aux_template);
	sprintf(device_data.device->name, "%s", AUX_ASIAIR_NAME);

	device_data.device->private_data = device_data.private_data;
	indigo_attach_device(device_data.device);
	INDIGO_DRIVER_DEBUG(DRIVER_NAME, "ADD: Device.");
}

static void delete_device() {
	if (device_data.device != NULL) {
		indigo_detach_device(device_data.device);
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "REMOVE: Device.");
		free(device_data.device);
		device_data.device = NULL;
	}

	if (device_data.private_data != NULL) {
		free(device_data.private_data);
		device_data.private_data = NULL;
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "REMOVE: PRIVATE_DATA");
	}
}


indigo_result indigo_aux_asiair(indigo_driver_action action, indigo_driver_info *info) {

	static indigo_driver_action last_action = INDIGO_DRIVER_SHUTDOWN;

	SET_DRIVER_INFO(info, DRIVER_INFO, __FUNCTION__, DRIVER_VERSION, false, last_action);

	if (action == last_action)
		return INDIGO_OK;

	switch (action) {
	case INDIGO_DRIVER_INIT:
		last_action = action;
		create_device();
		break;

	case INDIGO_DRIVER_SHUTDOWN:
		VERIFY_NOT_CONNECTED(device_data.device);
		last_action = action;
		delete_device();
		break;

	case INDIGO_DRIVER_INFO:
		break;
	}

	return INDIGO_OK;
}
