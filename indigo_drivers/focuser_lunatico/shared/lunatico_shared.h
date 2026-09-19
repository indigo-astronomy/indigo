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
// 2.0 by Rumen G. Bogdanovski <rumenastro@gmail.com>
// 3.0 refactoring to indigo_generator by Peter Polakovic <peter.polakovic@cloudmakers.eu>

/** INDIGO Lunatico Astronomia shared definitions
 \file lunatico_shared.h
 */

// Constants and types shared by indigo_focuser_lunatico and
// indigo_rotator_lunatico. Included from the define { } block of both .driver
// definitions, so they are available to the private data declaration and to
// shared/lunatico_shared.c.

#ifndef lunatico_shared_h
#define lunatico_shared_h

// DB9 ports of a Platypus. Seletek and Armadillo have two, a Limpet one.
#define LUNATICO_PORTS              3

// The SLP request and its reply always fit this buffer; it is the historical
// buffer size of the driver.
#define LUNATICO_CMD_LEN            100

// Default UDP port of the controller when DEVICE_PORT carries a lunatico:// URL.
#define LUNATICO_UDP_PORT           10000

// Serial line speed of the controller.
#define LUNATICO_BAUDRATE           "115200"

// Reply timeouts, matching the 3.1 s first-byte and 0.1 s inter-byte select()
// timeouts of the hand-written reader this replaces.
#define LUNATICO_REPLY_TIMEOUT      INDIGO_DELAY(3.1)
#define LUNATICO_BYTE_TIMEOUT       INDIGO_DELAY(0.1)

// Interval of the position poll of a moving axis, and of the temperature and
// sensor polls.
#define LUNATICO_MOTION_POLL        0.5
#define LUNATICO_SENSOR_POLL        3

// A temperature at or below this is what the controller reports when no sensor
// is connected.
#define NO_TEMP_READING             (-25)

// Full scale of the coil current registers; the driver scales a percentage by
// 1023 / 100 into it.
#define LUNATICO_POWER_SCALE        10.23

// Speed is configured in microseconds per step, derived from a kHz step rate.
#define LUNATICO_MIN_SPEED_US       50
#define LUNATICO_MAX_SPEED_US       500000

#define AUX_POWERBOX_GROUP          "Powerbox"
#define AUX_SENSORS_GROUP           "Sensors"

// Model digit of !seletek version:<MOPFF>#.
typedef enum {
	MODEL_SELETEK = 1,
	MODEL_ARMADILLO = 2,
	MODEL_PLATYPUS = 3,
	MODEL_DRAGONFLY = 4,
	MODEL_LIMPET = 5
} lunatico_model;

typedef enum {
	STEP_MODE_FULL = 0,
	STEP_MODE_HALF = 1
} lunatico_step_mode;

// The controller combines the motor wiring and the direction into one value.
typedef enum {
	MW_LUNATICO_NORMAL = 0,
	MW_LUNATICO_REVERSED = 1,
	MW_MOONLITE_NORMAL = 2,
	MW_MOONLITE_REVERSED = 3
} lunatico_wiring;

typedef enum {
	MT_UNIPOLAR = 0,
	MT_BIPOLAR = 1,
	MT_DC = 2,
	MT_STEP_DIR = 3
} lunatico_motor_type;

// State of one physical port. Each port is represented by a focuser, a rotator
// and a powerbox logical device, but the hardware has one stepper and one DB9
// connector per port, so only one of them can hold the port at a time and they
// share this slot. owner is the logical device that currently holds it.
typedef struct {
	indigo_device *owner;
	int32_t focuser_position, focuser_target;
	double rotator_position, rotator_target;
	double previous_temperature;
	int temperature_sensor;
	bool has_temperature_sensor;
} lunatico_port_state;

#endif /* lunatico_shared_h */
