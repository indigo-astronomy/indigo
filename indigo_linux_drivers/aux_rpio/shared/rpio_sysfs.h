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
 \file rpio_sysfs.h

 Both drivers drive the same 40-pin header through /sys/class/gpio and
 /sys/class/pwm and differ only in which pins they use and how many. The sysfs
 GPIO numbering is global rather than per controller, so the number of a BCM pin
 is the base of the controller that owns the header plus the BCM number. That
 base is 0 on a Raspberry Pi 4 and earlier and 571 on a Raspberry Pi 5, so it is
 resolved at connect time instead of being assumed.
 */

#ifndef rpio_sysfs_h
#define rpio_sysfs_h

#include <stdbool.h>

#include <indigo/indigo_driver.h>

/** Resolved sysfs topology and the pin assignment of one driver. */
typedef struct {
	const int *output_pins;
	int output_count;
	const int *input_pins;
	int input_count;
	/** Output line driven by each PWM channel, one entry per channel. The
	    channels are not always the leading outputs: the ASIAIR drives its
	    first and its fourth output from PWM channels 0 and 1. */
	const int *pwm_lines;
	int pwm_count;
	/** Base of the sysfs GPIO controller that owns the 40-pin header. */
	int gpio_base;
	/** Index of the resolved PWM chip, negative when none was found. */
	int pwm_chip;
	/** True when the PWM chip was found and the PWM outputs are in use. */
	bool pwm_present;
} rpio_sysfs;

/** Resolve the GPIO controller base and the PWM chip. Always succeeds; the
    absence of a PWM chip is reported through pwm_present, not as a failure. */
extern void rpio_sysfs_resolve(rpio_sysfs *sysfs);

/** Export every pin and set its direction. On failure every pin exported by
    this call is unexported again, so a failed attempt leaves nothing behind. */
extern bool rpio_sysfs_export_all(rpio_sysfs *sysfs);

/** Unexport everything this driver exported. Best effort: a failing pin is
    logged and the remaining pins are still released. */
extern void rpio_sysfs_unexport_all(rpio_sysfs *sysfs);

/** Write one output line, through PWM enable for a PWM backed line. */
extern bool rpio_sysfs_write_output(rpio_sysfs *sysfs, int line, bool value);

/** Read every output line into values, which must hold output_count entries. */
extern bool rpio_sysfs_read_outputs(rpio_sysfs *sysfs, int *values);

/** Read every input line into values, which must hold input_count entries. */
extern bool rpio_sysfs_read_inputs(rpio_sysfs *sysfs, int *values);

/** Program one PWM channel from a frequency in Hz and a duty cycle in percent. */
extern bool rpio_sysfs_set_pwm(rpio_sysfs *sysfs, int channel, double frequency, double duty);

/** Read one PWM channel back as a frequency in Hz and a duty cycle in percent.
    Fails rather than reporting infinity when the device reports a zero period. */
extern bool rpio_sysfs_get_pwm(rpio_sysfs *sysfs, int channel, double *frequency, double *duty);

/** Re-arm a PWM channel that is already enabled, so a new period takes effect. */
extern bool rpio_sysfs_restart_pwm(rpio_sysfs *sysfs, int channel);

#endif /* rpio_sysfs_h */
