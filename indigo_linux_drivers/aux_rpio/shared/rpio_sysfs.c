// Copyright (C) 2020-2026 Rumen G. Bogdanovski
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
// 3.0 by Peter Polakovic <peter.polakovic@cloudmakers.eu>

/** Legacy sysfs GPIO and PWM access shared by the Raspberry Pi GPIO and the
    ZWO ASIAIR power port drivers.
 \file rpio_sysfs.c
 Included by the driver that uses it, so DRIVER_NAME is the including driver.
 */

#define RPIO_SYSFS_GPIO_ROOT "/sys/class/gpio"
#define RPIO_SYSFS_PWM_ROOT "/sys/class/pwm"
#define RPIO_SYSFS_PATH_SIZE 256
#define RPIO_SYSFS_VALUE_SIZE 32
// A pin node appears asynchronously, so its direction file is waited for
// instead of sleeping for a fixed time that is both too long and not a
// guarantee.
#define RPIO_SYSFS_SETTLE_STEPS 100
#define RPIO_SYSFS_SETTLE_DELAY 10000

static bool rpio_sysfs_read_text(const char *path, char *buffer, int size) {
	int fd = open(path, O_RDONLY);
	if (fd < 0) {
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "Failed to open %s for reading", path);
		return false;
	}
	ssize_t bytes = read(fd, buffer, size - 1);
	close(fd);
	if (bytes <= 0) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "Failed to read %s", path);
		return false;
	}
	buffer[bytes] = 0;
	return true;
}

static bool rpio_sysfs_read_number(const char *path, int *value) {
	char buffer[RPIO_SYSFS_VALUE_SIZE];
	if (!rpio_sysfs_read_text(path, buffer, sizeof(buffer))) {
		return false;
	}
	*value = atoi(buffer);
	return true;
}

static bool rpio_sysfs_write_text(const char *path, const char *text) {
	int fd = open(path, O_WRONLY);
	if (fd < 0) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "Failed to open %s for writing", path);
		return false;
	}
	ssize_t length = (ssize_t)strlen(text);
	ssize_t bytes = write(fd, text, length);
	close(fd);
	if (bytes != length) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "Failed to write '%s' to %s", text, path);
		return false;
	}
	return true;
}

static bool rpio_sysfs_write_number(const char *path, int value) {
	char text[RPIO_SYSFS_VALUE_SIZE];
	snprintf(text, sizeof(text), "%d", value);
	return rpio_sysfs_write_text(path, text);
}

static int rpio_sysfs_highest_pin(rpio_sysfs *sysfs) {
	int highest = 0;
	for (int i = 0; i < sysfs->output_count; i++) {
		if (sysfs->output_pins[i] > highest) {
			highest = sysfs->output_pins[i];
		}
	}
	for (int i = 0; i < sysfs->input_count; i++) {
		if (sysfs->input_pins[i] > highest) {
			highest = sysfs->input_pins[i];
		}
	}
	return highest;
}

// The sysfs GPIO number of a BCM pin is the base of the controller that owns
// the 40-pin header plus the BCM number. The header controller is a pinctrl
// driver, pinctrl-bcm2835 or pinctrl-bcm2711 up to a Raspberry Pi 4 and
// pinctrl-rp1 on a Raspberry Pi 5, while the other controllers of the same
// machine are gpio-brcmstb banks that must not be selected.
static int rpio_sysfs_find_gpio_base(rpio_sysfs *sysfs) {
	char path[RPIO_SYSFS_PATH_SIZE];
	char label[RPIO_SYSFS_VALUE_SIZE];
	int highest = rpio_sysfs_highest_pin(sysfs);
	int base = 0;
	int found = -1;
	DIR *directory = opendir(RPIO_SYSFS_GPIO_ROOT);
	if (directory == NULL) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "Failed to open %s, is sysfs GPIO enabled?", RPIO_SYSFS_GPIO_ROOT);
		return 0;
	}
	struct dirent *entry = readdir(directory);
	while (entry != NULL) {
		if (!strncmp(entry->d_name, "gpiochip", 8)) {
			int lines = 0;
			snprintf(path, sizeof(path), "%s/%s/label", RPIO_SYSFS_GPIO_ROOT, entry->d_name);
			if (rpio_sysfs_read_text(path, label, sizeof(label)) && !strncmp(label, "pinctrl-", 8)) {
				snprintf(path, sizeof(path), "%s/%s/ngpio", RPIO_SYSFS_GPIO_ROOT, entry->d_name);
				if (rpio_sysfs_read_number(path, &lines) && lines > highest) {
					snprintf(path, sizeof(path), "%s/%s/base", RPIO_SYSFS_GPIO_ROOT, entry->d_name);
					if (rpio_sysfs_read_number(path, &base) && (found < 0 || base < found)) {
						found = base;
					}
				}
			}
		}
		entry = readdir(directory);
	}
	closedir(directory);
	if (found < 0) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "No sysfs GPIO controller owns the header, assuming base 0");
		return 0;
	}
	INDIGO_DRIVER_DEBUG(DRIVER_NAME, "Header GPIO controller base = %d", found);
	return found;
}

// The PWM chip index is not fixed either, so the first chip that has enough
// channels is used instead of assuming pwmchip0.
static int rpio_sysfs_find_pwm_chip(rpio_sysfs *sysfs) {
	char path[RPIO_SYSFS_PATH_SIZE];
	int found = -1;
	DIR *directory = opendir(RPIO_SYSFS_PWM_ROOT);
	if (directory == NULL) {
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "No PWM present");
		return -1;
	}
	struct dirent *entry = readdir(directory);
	while (entry != NULL) {
		int index = 0;
		int channels = 0;
		if (sscanf(entry->d_name, "pwmchip%d", &index) == 1) {
			snprintf(path, sizeof(path), "%s/%s/npwm", RPIO_SYSFS_PWM_ROOT, entry->d_name);
			if (rpio_sysfs_read_number(path, &channels) && channels >= sysfs->pwm_count && (found < 0 || index < found)) {
				found = index;
			}
		}
		entry = readdir(directory);
	}
	closedir(directory);
	if (found < 0) {
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "No PWM chip with %d channels present", sysfs->pwm_count);
	} else {
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "PWM chip = pwmchip%d", found);
	}
	return found;
}

void rpio_sysfs_resolve(rpio_sysfs *sysfs) {
	sysfs->gpio_base = rpio_sysfs_find_gpio_base(sysfs);
	sysfs->pwm_chip = sysfs->pwm_count > 0 ? rpio_sysfs_find_pwm_chip(sysfs) : -1;
	sysfs->pwm_present = sysfs->pwm_chip >= 0;
}

// The PWM channel that drives an output line, or a negative value when the
// line is a plain GPIO.
static int rpio_sysfs_channel_of(rpio_sysfs *sysfs, int line) {
	if (!sysfs->pwm_present) {
		return -1;
	}
	for (int i = 0; i < sysfs->pwm_count; i++) {
		if (sysfs->pwm_lines[i] == line) {
			return i;
		}
	}
	return -1;
}

static void rpio_sysfs_pin_path(rpio_sysfs *sysfs, char *path, int size, int pin, const char *file) {
	if (file == NULL) {
		snprintf(path, size, "%s/gpio%d", RPIO_SYSFS_GPIO_ROOT, sysfs->gpio_base + pin);
	} else {
		snprintf(path, size, "%s/gpio%d/%s", RPIO_SYSFS_GPIO_ROOT, sysfs->gpio_base + pin, file);
	}
}

static void rpio_sysfs_pwm_path(rpio_sysfs *sysfs, char *path, int size, int channel, const char *file) {
	snprintf(path, size, "%s/pwmchip%d/pwm%d/%s", RPIO_SYSFS_PWM_ROOT, sysfs->pwm_chip, channel, file);
}

static bool rpio_sysfs_pin_exported(rpio_sysfs *sysfs, int pin) {
	char path[RPIO_SYSFS_PATH_SIZE];
	struct stat status = { 0 };
	rpio_sysfs_pin_path(sysfs, path, sizeof(path), pin, NULL);
	return stat(path, &status) == 0 && S_ISDIR(status.st_mode);
}

static bool rpio_sysfs_export_pin(rpio_sysfs *sysfs, int pin) {
	char path[RPIO_SYSFS_PATH_SIZE];
	if (rpio_sysfs_pin_exported(sysfs, pin)) {
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "Pin #%d is already exported", pin);
		return true;
	}
	snprintf(path, sizeof(path), "%s/export", RPIO_SYSFS_GPIO_ROOT);
	if (!rpio_sysfs_write_number(path, sysfs->gpio_base + pin)) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "Failed to export pin #%d as sysfs GPIO %d", pin, sysfs->gpio_base + pin);
		return false;
	}
	return true;
}

static bool rpio_sysfs_unexport_pin(rpio_sysfs *sysfs, int pin) {
	char path[RPIO_SYSFS_PATH_SIZE];
	if (!rpio_sysfs_pin_exported(sysfs, pin)) {
		return true;
	}
	snprintf(path, sizeof(path), "%s/unexport", RPIO_SYSFS_GPIO_ROOT);
	return rpio_sysfs_write_number(path, sysfs->gpio_base + pin);
}

// A freshly exported node appears owned by root and is only handed to the gpio
// group once udev has processed the event, so waiting for the direction file to
// exist is not enough: it has to be writable. This is what the fixed one second
// sleep in the original driver was really waiting for.
static bool rpio_sysfs_wait_for_pin(rpio_sysfs *sysfs, int pin) {
	char path[RPIO_SYSFS_PATH_SIZE];
	rpio_sysfs_pin_path(sysfs, path, sizeof(path), pin, "direction");
	for (int i = 0; i < RPIO_SYSFS_SETTLE_STEPS; i++) {
		int fd = open(path, O_WRONLY);
		if (fd >= 0) {
			close(fd);
			return true;
		}
		indigo_usleep(RPIO_SYSFS_SETTLE_DELAY);
	}
	INDIGO_DRIVER_ERROR(DRIVER_NAME, "Pin #%d did not become writable", pin);
	return false;
}

static bool rpio_sysfs_set_direction(rpio_sysfs *sysfs, int pin, bool input) {
	char path[RPIO_SYSFS_PATH_SIZE];
	char current[RPIO_SYSFS_VALUE_SIZE];
	rpio_sysfs_pin_path(sysfs, path, sizeof(path), pin, "direction");
	if (rpio_sysfs_read_text(path, current, sizeof(current))) {
		if ((current[0] == 'i') == input) {
			INDIGO_DRIVER_DEBUG(DRIVER_NAME, "Pin #%d direction is already %s", pin, input ? "in" : "out");
			return true;
		}
	}
	return rpio_sysfs_write_text(path, input ? "in" : "out");
}

static bool rpio_sysfs_read_pin(rpio_sysfs *sysfs, int pin, int *value) {
	char path[RPIO_SYSFS_PATH_SIZE];
	rpio_sysfs_pin_path(sysfs, path, sizeof(path), pin, "value");
	return rpio_sysfs_read_number(path, value);
}

static bool rpio_sysfs_write_pin(rpio_sysfs *sysfs, int pin, bool value) {
	char path[RPIO_SYSFS_PATH_SIZE];
	rpio_sysfs_pin_path(sysfs, path, sizeof(path), pin, "value");
	return rpio_sysfs_write_text(path, value ? "1" : "0");
}

static bool rpio_sysfs_export_pwm(rpio_sysfs *sysfs, int channel) {
	char path[RPIO_SYSFS_PATH_SIZE];
	rpio_sysfs_pwm_path(sysfs, path, sizeof(path), channel, "enable");
	int fd = open(path, O_RDONLY);
	if (fd >= 0) {
		close(fd);
		return true;
	}
	snprintf(path, sizeof(path), "%s/pwmchip%d/export", RPIO_SYSFS_PWM_ROOT, sysfs->pwm_chip);
	return rpio_sysfs_write_number(path, channel);
}

static bool rpio_sysfs_unexport_pwm(rpio_sysfs *sysfs, int channel) {
	char path[RPIO_SYSFS_PATH_SIZE];
	snprintf(path, sizeof(path), "%s/pwmchip%d/unexport", RPIO_SYSFS_PWM_ROOT, sysfs->pwm_chip);
	return rpio_sysfs_write_number(path, channel);
}

static bool rpio_sysfs_enable_pwm(rpio_sysfs *sysfs, int channel, bool value) {
	char path[RPIO_SYSFS_PATH_SIZE];
	rpio_sysfs_pwm_path(sysfs, path, sizeof(path), channel, "enable");
	return rpio_sysfs_write_text(path, value ? "1" : "0");
}

static bool rpio_sysfs_pwm_enabled(rpio_sysfs *sysfs, int channel, int *value) {
	char path[RPIO_SYSFS_PATH_SIZE];
	rpio_sysfs_pwm_path(sysfs, path, sizeof(path), channel, "enable");
	return rpio_sysfs_read_number(path, value);
}

bool rpio_sysfs_export_all(rpio_sysfs *sysfs) {
	int exported_outputs = 0;
	int exported_inputs = 0;
	int exported_channels = 0;
	bool failed = false;
	for (int i = 0; i < sysfs->pwm_count && sysfs->pwm_present && !failed; i++) {
		failed = !rpio_sysfs_export_pwm(sysfs, i);
		exported_channels += failed ? 0 : 1;
	}
	for (int i = 0; i < sysfs->output_count && !failed; i++) {
		if (rpio_sysfs_channel_of(sysfs, i) >= 0) {
			continue;
		}
		failed = !rpio_sysfs_export_pin(sysfs, sysfs->output_pins[i]);
		exported_outputs += failed ? 0 : 1;
	}
	for (int i = 0; i < sysfs->input_count && !failed; i++) {
		failed = !rpio_sysfs_export_pin(sysfs, sysfs->input_pins[i]);
		exported_inputs += failed ? 0 : 1;
	}
	for (int i = 0; i < sysfs->output_count && !failed; i++) {
		if (rpio_sysfs_channel_of(sysfs, i) >= 0) {
			continue;
		}
		failed = !rpio_sysfs_wait_for_pin(sysfs, sysfs->output_pins[i]) || !rpio_sysfs_set_direction(sysfs, sysfs->output_pins[i], false);
	}
	for (int i = 0; i < sysfs->input_count && !failed; i++) {
		failed = !rpio_sysfs_wait_for_pin(sysfs, sysfs->input_pins[i]) || !rpio_sysfs_set_direction(sysfs, sysfs->input_pins[i], true);
	}
	if (failed) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "Rolling back %d output, %d input and %d PWM exports", exported_outputs, exported_inputs, exported_channels);
		rpio_sysfs_unexport_all(sysfs);
		return false;
	}
	return true;
}

void rpio_sysfs_unexport_all(rpio_sysfs *sysfs) {
	for (int i = 0; i < sysfs->input_count; i++) {
		rpio_sysfs_unexport_pin(sysfs, sysfs->input_pins[i]);
	}
	for (int i = 0; i < sysfs->output_count; i++) {
		if (rpio_sysfs_channel_of(sysfs, i) >= 0) {
			continue;
		}
		rpio_sysfs_unexport_pin(sysfs, sysfs->output_pins[i]);
	}
	for (int i = 0; i < sysfs->pwm_count && sysfs->pwm_present; i++) {
		rpio_sysfs_unexport_pwm(sysfs, i);
	}
}

bool rpio_sysfs_write_output(rpio_sysfs *sysfs, int line, bool value) {
	if (line < 0 || line >= sysfs->output_count) {
		return false;
	}
	int channel = rpio_sysfs_channel_of(sysfs, line);
	if (channel >= 0) {
		return rpio_sysfs_enable_pwm(sysfs, channel, value);
	}
	return rpio_sysfs_write_pin(sysfs, sysfs->output_pins[line], value);
}

bool rpio_sysfs_read_outputs(rpio_sysfs *sysfs, int *values) {
	for (int i = 0; i < sysfs->output_count; i++) {
		int channel = rpio_sysfs_channel_of(sysfs, i);
		if (channel >= 0) {
			if (!rpio_sysfs_pwm_enabled(sysfs, channel, values + i)) {
				return false;
			}
		} else if (!rpio_sysfs_read_pin(sysfs, sysfs->output_pins[i], values + i)) {
			return false;
		}
	}
	return true;
}

bool rpio_sysfs_read_inputs(rpio_sysfs *sysfs, int *values) {
	for (int i = 0; i < sysfs->input_count; i++) {
		if (!rpio_sysfs_read_pin(sysfs, sysfs->input_pins[i], values + i)) {
			return false;
		}
	}
	return true;
}

bool rpio_sysfs_set_pwm(rpio_sysfs *sysfs, int channel, double frequency, double duty) {
	char path[RPIO_SYSFS_PATH_SIZE];
	if (!sysfs->pwm_present || frequency <= 0) {
		return false;
	}
	int period = (int)(1e9 / frequency);
	int duty_cycle = (int)(period * duty / 100.0);
	// The duty cycle can never exceed the period, so the order of the two
	// writes depends on whether the period grows or shrinks.
	rpio_sysfs_pwm_path(sysfs, path, sizeof(path), channel, "duty_cycle");
	int current = 0;
	bool shrinking = rpio_sysfs_read_number(path, &current) && current > duty_cycle;
	if (shrinking && !rpio_sysfs_write_number(path, duty_cycle)) {
		return false;
	}
	rpio_sysfs_pwm_path(sysfs, path, sizeof(path), channel, "period");
	if (!rpio_sysfs_write_number(path, period)) {
		return false;
	}
	rpio_sysfs_pwm_path(sysfs, path, sizeof(path), channel, "duty_cycle");
	return rpio_sysfs_write_number(path, duty_cycle);
}

bool rpio_sysfs_get_pwm(rpio_sysfs *sysfs, int channel, double *frequency, double *duty) {
	char path[RPIO_SYSFS_PATH_SIZE];
	int period = 0;
	int duty_cycle = 0;
	if (!sysfs->pwm_present) {
		return false;
	}
	rpio_sysfs_pwm_path(sysfs, path, sizeof(path), channel, "period");
	if (!rpio_sysfs_read_number(path, &period)) {
		return false;
	}
	rpio_sysfs_pwm_path(sysfs, path, sizeof(path), channel, "duty_cycle");
	if (!rpio_sysfs_read_number(path, &duty_cycle)) {
		return false;
	}
	if (period <= 0) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "PWM channel %d reported a period of %d", channel, period);
		return false;
	}
	*frequency = 1e9 / period;
	*duty = (double)duty_cycle / period * 100;
	return true;
}

bool rpio_sysfs_restart_pwm(rpio_sysfs *sysfs, int channel) {
	int enabled = 0;
	if (!sysfs->pwm_present || !rpio_sysfs_pwm_enabled(sysfs, channel, &enabled) || enabled != 1) {
		return false;
	}
	return rpio_sysfs_enable_pwm(sysfs, channel, false) && rpio_sysfs_enable_pwm(sysfs, channel, true);
}
