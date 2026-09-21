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

// This file generated from indigo_rotator_lunatico.driver

#pragma mark - Includes

#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <assert.h>

//+ include

#include <indigo/indigo_client.h>

//- include

#include <indigo/indigo_driver_xml.h>
#include <indigo/indigo_rotator_driver.h>
#include <indigo/indigo_focuser_driver.h>
#include <indigo/indigo_rotator_driver.h>
#include <indigo/indigo_aux_driver.h>
#include <indigo/indigo_focuser_driver.h>
#include <indigo/indigo_rotator_driver.h>
#include <indigo/indigo_aux_driver.h>
#include <indigo/indigo_uni_io.h>

#include "indigo_rotator_lunatico.h"

#pragma mark - Common definitions

#define DRIVER_VERSION       0x0300000C
#define DRIVER_NAME          "indigo_rotator_lunatico"
#define DRIVER_LABEL         "Lunatico Astronomia Rotator"
#define ROTATOR_MAIN_DEVICE_NAME "Rotator Lunatico (Main)"
#define FOCUSER_EXP_DEVICE_NAME "Focuser Lunatico (Exp)"
#define ROTATOR_EXP_DEVICE_NAME "Rotator Lunatico (Exp)"
#define AUX_EXP_DEVICE_NAME  "Powerbox Lunatico (Exp)"
#define FOCUSER_THIRD_DEVICE_NAME "Focuser Lunatico (Third)"
#define ROTATOR_THIRD_DEVICE_NAME "Rotator Lunatico (Third)"
#define AUX_THIRD_DEVICE_NAME "Powerbox Lunatico (Third)"
#define PRIVATE_DATA         ((lunatico_private_data *)device->private_data)

//+ define

#include "../focuser_lunatico/shared/lunatico_shared.h"

#define CONFLICTING_DRIVER   "indigo_focuser_lunatico"

//- define

#pragma mark - Property definitions

#define X_ROTATOR_STEP_MODE_MAIN_PROPERTY       (PRIVATE_DATA->x_rotator_step_mode_main_property)
#define X_ROTATOR_STEP_MODE_MAIN_FULL_ITEM      (X_ROTATOR_STEP_MODE_MAIN_PROPERTY->items + 0)
#define X_ROTATOR_STEP_MODE_MAIN_HALF_ITEM      (X_ROTATOR_STEP_MODE_MAIN_PROPERTY->items + 1)

#define X_ROTATOR_STEP_MODE_MAIN_PROPERTY_NAME  "X_ROTATOR_STEP_MODE"
#define X_ROTATOR_STEP_MODE_MAIN_FULL_ITEM_NAME "FULL"
#define X_ROTATOR_STEP_MODE_MAIN_HALF_ITEM_NAME "HALF"

#define X_ROTATOR_POWER_CONTROL_MAIN_PROPERTY       (PRIVATE_DATA->x_rotator_power_control_main_property)
#define X_ROTATOR_POWER_CONTROL_MAIN_MOVE_ITEM      (X_ROTATOR_POWER_CONTROL_MAIN_PROPERTY->items + 0)
#define X_ROTATOR_POWER_CONTROL_MAIN_STOP_ITEM      (X_ROTATOR_POWER_CONTROL_MAIN_PROPERTY->items + 1)

#define X_ROTATOR_POWER_CONTROL_MAIN_PROPERTY_NAME  "X_ROTATOR_POWER_CONTROL"
#define X_ROTATOR_POWER_CONTROL_MAIN_MOVE_ITEM_NAME "MOVE_POWER"
#define X_ROTATOR_POWER_CONTROL_MAIN_STOP_ITEM_NAME "STOP_POWER"

#define X_ROTATOR_MOTOR_WIRING_MAIN_PROPERTY           (PRIVATE_DATA->x_rotator_motor_wiring_main_property)
#define X_ROTATOR_MOTOR_WIRING_MAIN_LUNATICO_ITEM      (X_ROTATOR_MOTOR_WIRING_MAIN_PROPERTY->items + 0)
#define X_ROTATOR_MOTOR_WIRING_MAIN_MOONLITE_ITEM      (X_ROTATOR_MOTOR_WIRING_MAIN_PROPERTY->items + 1)

#define X_ROTATOR_MOTOR_WIRING_MAIN_PROPERTY_NAME      "X_ROTATOR_MOTOR_WIRING"
#define X_ROTATOR_MOTOR_WIRING_MAIN_LUNATICO_ITEM_NAME "LUNATICO"
#define X_ROTATOR_MOTOR_WIRING_MAIN_MOONLITE_ITEM_NAME "MOONLITE"

#define X_ROTATOR_MOTOR_TYPE_MAIN_PROPERTY           (PRIVATE_DATA->x_rotator_motor_type_main_property)
#define X_ROTATOR_MOTOR_TYPE_MAIN_UNIPOLAR_ITEM      (X_ROTATOR_MOTOR_TYPE_MAIN_PROPERTY->items + 0)
#define X_ROTATOR_MOTOR_TYPE_MAIN_BIPOLAR_ITEM       (X_ROTATOR_MOTOR_TYPE_MAIN_PROPERTY->items + 1)
#define X_ROTATOR_MOTOR_TYPE_MAIN_DC_ITEM            (X_ROTATOR_MOTOR_TYPE_MAIN_PROPERTY->items + 2)
#define X_ROTATOR_MOTOR_TYPE_MAIN_STEP_DIR_ITEM      (X_ROTATOR_MOTOR_TYPE_MAIN_PROPERTY->items + 3)

#define X_ROTATOR_MOTOR_TYPE_MAIN_PROPERTY_NAME      "X_ROTATOR_MOTOR_TYPE"
#define X_ROTATOR_MOTOR_TYPE_MAIN_UNIPOLAR_ITEM_NAME "UNIPOLAR"
#define X_ROTATOR_MOTOR_TYPE_MAIN_BIPOLAR_ITEM_NAME  "BIPOLAR"
#define X_ROTATOR_MOTOR_TYPE_MAIN_DC_ITEM_NAME       "DC"
#define X_ROTATOR_MOTOR_TYPE_MAIN_STEP_DIR_ITEM_NAME "STEP_DIR"

#define X_FOCUSER_STEP_MODE_EXP_PROPERTY       (PRIVATE_DATA->x_focuser_step_mode_exp_property)
#define X_FOCUSER_STEP_MODE_EXP_FULL_ITEM      (X_FOCUSER_STEP_MODE_EXP_PROPERTY->items + 0)
#define X_FOCUSER_STEP_MODE_EXP_HALF_ITEM      (X_FOCUSER_STEP_MODE_EXP_PROPERTY->items + 1)

#define X_FOCUSER_STEP_MODE_EXP_PROPERTY_NAME  "X_FOCUSER_STEP_MODE"
#define X_FOCUSER_STEP_MODE_EXP_FULL_ITEM_NAME "FULL"
#define X_FOCUSER_STEP_MODE_EXP_HALF_ITEM_NAME "HALF"

#define X_FOCUSER_POWER_CONTROL_EXP_PROPERTY       (PRIVATE_DATA->x_focuser_power_control_exp_property)
#define X_FOCUSER_POWER_CONTROL_EXP_MOVE_ITEM      (X_FOCUSER_POWER_CONTROL_EXP_PROPERTY->items + 0)
#define X_FOCUSER_POWER_CONTROL_EXP_STOP_ITEM      (X_FOCUSER_POWER_CONTROL_EXP_PROPERTY->items + 1)

#define X_FOCUSER_POWER_CONTROL_EXP_PROPERTY_NAME  "X_FOCUSER_POWER_CONTROL"
#define X_FOCUSER_POWER_CONTROL_EXP_MOVE_ITEM_NAME "MOVE_POWER"
#define X_FOCUSER_POWER_CONTROL_EXP_STOP_ITEM_NAME "STOP_POWER"

#define X_FOCUSER_MOTOR_WIRING_EXP_PROPERTY           (PRIVATE_DATA->x_focuser_motor_wiring_exp_property)
#define X_FOCUSER_MOTOR_WIRING_EXP_LUNATICO_ITEM      (X_FOCUSER_MOTOR_WIRING_EXP_PROPERTY->items + 0)
#define X_FOCUSER_MOTOR_WIRING_EXP_MOONLITE_ITEM      (X_FOCUSER_MOTOR_WIRING_EXP_PROPERTY->items + 1)

#define X_FOCUSER_MOTOR_WIRING_EXP_PROPERTY_NAME      "X_FOCUSER_MOTOR_WIRING"
#define X_FOCUSER_MOTOR_WIRING_EXP_LUNATICO_ITEM_NAME "LUNATICO"
#define X_FOCUSER_MOTOR_WIRING_EXP_MOONLITE_ITEM_NAME "MOONLITE"

#define X_FOCUSER_MOTOR_TYPE_EXP_PROPERTY           (PRIVATE_DATA->x_focuser_motor_type_exp_property)
#define X_FOCUSER_MOTOR_TYPE_EXP_UNIPOLAR_ITEM      (X_FOCUSER_MOTOR_TYPE_EXP_PROPERTY->items + 0)
#define X_FOCUSER_MOTOR_TYPE_EXP_BIPOLAR_ITEM       (X_FOCUSER_MOTOR_TYPE_EXP_PROPERTY->items + 1)
#define X_FOCUSER_MOTOR_TYPE_EXP_DC_ITEM            (X_FOCUSER_MOTOR_TYPE_EXP_PROPERTY->items + 2)
#define X_FOCUSER_MOTOR_TYPE_EXP_STEP_DIR_ITEM      (X_FOCUSER_MOTOR_TYPE_EXP_PROPERTY->items + 3)

#define X_FOCUSER_MOTOR_TYPE_EXP_PROPERTY_NAME      "X_FOCUSER_MOTOR_TYPE"
#define X_FOCUSER_MOTOR_TYPE_EXP_UNIPOLAR_ITEM_NAME "UNIPOLAR"
#define X_FOCUSER_MOTOR_TYPE_EXP_BIPOLAR_ITEM_NAME  "BIPOLAR"
#define X_FOCUSER_MOTOR_TYPE_EXP_DC_ITEM_NAME       "DC"
#define X_FOCUSER_MOTOR_TYPE_EXP_STEP_DIR_ITEM_NAME "STEP_DIR"

#define X_FOCUSER_TEMPERATURE_SENSOR_EXP_PROPERTY           (PRIVATE_DATA->x_focuser_temperature_sensor_exp_property)
#define X_FOCUSER_TEMPERATURE_SENSOR_EXP_INTERNAL_ITEM      (X_FOCUSER_TEMPERATURE_SENSOR_EXP_PROPERTY->items + 0)
#define X_FOCUSER_TEMPERATURE_SENSOR_EXP_EXTERNAL_ITEM      (X_FOCUSER_TEMPERATURE_SENSOR_EXP_PROPERTY->items + 1)

#define X_FOCUSER_TEMPERATURE_SENSOR_EXP_PROPERTY_NAME      "X_FOCUSER_TEMPERATURE_SENSOR"
#define X_FOCUSER_TEMPERATURE_SENSOR_EXP_INTERNAL_ITEM_NAME "INTERNAL"
#define X_FOCUSER_TEMPERATURE_SENSOR_EXP_EXTERNAL_ITEM_NAME "EXTERNAL"

#define X_ROTATOR_STEP_MODE_EXP_PROPERTY       (PRIVATE_DATA->x_rotator_step_mode_exp_property)
#define X_ROTATOR_STEP_MODE_EXP_FULL_ITEM      (X_ROTATOR_STEP_MODE_EXP_PROPERTY->items + 0)
#define X_ROTATOR_STEP_MODE_EXP_HALF_ITEM      (X_ROTATOR_STEP_MODE_EXP_PROPERTY->items + 1)

#define X_ROTATOR_STEP_MODE_EXP_PROPERTY_NAME  "X_ROTATOR_STEP_MODE"
#define X_ROTATOR_STEP_MODE_EXP_FULL_ITEM_NAME "FULL"
#define X_ROTATOR_STEP_MODE_EXP_HALF_ITEM_NAME "HALF"

#define X_ROTATOR_POWER_CONTROL_EXP_PROPERTY       (PRIVATE_DATA->x_rotator_power_control_exp_property)
#define X_ROTATOR_POWER_CONTROL_EXP_MOVE_ITEM      (X_ROTATOR_POWER_CONTROL_EXP_PROPERTY->items + 0)
#define X_ROTATOR_POWER_CONTROL_EXP_STOP_ITEM      (X_ROTATOR_POWER_CONTROL_EXP_PROPERTY->items + 1)

#define X_ROTATOR_POWER_CONTROL_EXP_PROPERTY_NAME  "X_ROTATOR_POWER_CONTROL"
#define X_ROTATOR_POWER_CONTROL_EXP_MOVE_ITEM_NAME "MOVE_POWER"
#define X_ROTATOR_POWER_CONTROL_EXP_STOP_ITEM_NAME "STOP_POWER"

#define X_ROTATOR_MOTOR_WIRING_EXP_PROPERTY           (PRIVATE_DATA->x_rotator_motor_wiring_exp_property)
#define X_ROTATOR_MOTOR_WIRING_EXP_LUNATICO_ITEM      (X_ROTATOR_MOTOR_WIRING_EXP_PROPERTY->items + 0)
#define X_ROTATOR_MOTOR_WIRING_EXP_MOONLITE_ITEM      (X_ROTATOR_MOTOR_WIRING_EXP_PROPERTY->items + 1)

#define X_ROTATOR_MOTOR_WIRING_EXP_PROPERTY_NAME      "X_ROTATOR_MOTOR_WIRING"
#define X_ROTATOR_MOTOR_WIRING_EXP_LUNATICO_ITEM_NAME "LUNATICO"
#define X_ROTATOR_MOTOR_WIRING_EXP_MOONLITE_ITEM_NAME "MOONLITE"

#define X_ROTATOR_MOTOR_TYPE_EXP_PROPERTY           (PRIVATE_DATA->x_rotator_motor_type_exp_property)
#define X_ROTATOR_MOTOR_TYPE_EXP_UNIPOLAR_ITEM      (X_ROTATOR_MOTOR_TYPE_EXP_PROPERTY->items + 0)
#define X_ROTATOR_MOTOR_TYPE_EXP_BIPOLAR_ITEM       (X_ROTATOR_MOTOR_TYPE_EXP_PROPERTY->items + 1)
#define X_ROTATOR_MOTOR_TYPE_EXP_DC_ITEM            (X_ROTATOR_MOTOR_TYPE_EXP_PROPERTY->items + 2)
#define X_ROTATOR_MOTOR_TYPE_EXP_STEP_DIR_ITEM      (X_ROTATOR_MOTOR_TYPE_EXP_PROPERTY->items + 3)

#define X_ROTATOR_MOTOR_TYPE_EXP_PROPERTY_NAME      "X_ROTATOR_MOTOR_TYPE"
#define X_ROTATOR_MOTOR_TYPE_EXP_UNIPOLAR_ITEM_NAME "UNIPOLAR"
#define X_ROTATOR_MOTOR_TYPE_EXP_BIPOLAR_ITEM_NAME  "BIPOLAR"
#define X_ROTATOR_MOTOR_TYPE_EXP_DC_ITEM_NAME       "DC"
#define X_ROTATOR_MOTOR_TYPE_EXP_STEP_DIR_ITEM_NAME "STEP_DIR"

#define AUX_OUTLET_NAMES_EXP_PROPERTY  (PRIVATE_DATA->aux_outlet_names_exp_property)
#define AUX_OUTLET_NAME_EXP_1_ITEM     (AUX_OUTLET_NAMES_EXP_PROPERTY->items + 0)
#define AUX_OUTLET_NAME_EXP_2_ITEM     (AUX_OUTLET_NAMES_EXP_PROPERTY->items + 1)
#define AUX_OUTLET_NAME_EXP_3_ITEM     (AUX_OUTLET_NAMES_EXP_PROPERTY->items + 2)
#define AUX_OUTLET_NAME_EXP_4_ITEM     (AUX_OUTLET_NAMES_EXP_PROPERTY->items + 3)

#define AUX_POWER_OUTLET_EXP_PROPERTY  (PRIVATE_DATA->aux_power_outlet_exp_property)
#define AUX_POWER_OUTLET_EXP_1_ITEM    (AUX_POWER_OUTLET_EXP_PROPERTY->items + 0)
#define AUX_POWER_OUTLET_EXP_2_ITEM    (AUX_POWER_OUTLET_EXP_PROPERTY->items + 1)
#define AUX_POWER_OUTLET_EXP_3_ITEM    (AUX_POWER_OUTLET_EXP_PROPERTY->items + 2)
#define AUX_POWER_OUTLET_EXP_4_ITEM    (AUX_POWER_OUTLET_EXP_PROPERTY->items + 3)

#define AUX_SENSOR_NAMES_EXP_PROPERTY  (PRIVATE_DATA->aux_sensor_names_exp_property)
#define AUX_SENSOR_NAME_EXP_1_ITEM     (AUX_SENSOR_NAMES_EXP_PROPERTY->items + 0)
#define AUX_SENSOR_NAME_EXP_2_ITEM     (AUX_SENSOR_NAMES_EXP_PROPERTY->items + 1)
#define AUX_SENSOR_NAME_EXP_3_ITEM     (AUX_SENSOR_NAMES_EXP_PROPERTY->items + 2)
#define AUX_SENSOR_NAME_EXP_4_ITEM     (AUX_SENSOR_NAMES_EXP_PROPERTY->items + 3)

#define AUX_GPIO_SENSORS_EXP_PROPERTY  (PRIVATE_DATA->aux_gpio_sensors_exp_property)
#define AUX_GPIO_SENSOR_EXP_1_ITEM     (AUX_GPIO_SENSORS_EXP_PROPERTY->items + 0)
#define AUX_GPIO_SENSOR_EXP_2_ITEM     (AUX_GPIO_SENSORS_EXP_PROPERTY->items + 1)
#define AUX_GPIO_SENSOR_EXP_3_ITEM     (AUX_GPIO_SENSORS_EXP_PROPERTY->items + 2)
#define AUX_GPIO_SENSOR_EXP_4_ITEM     (AUX_GPIO_SENSORS_EXP_PROPERTY->items + 3)

#define X_FOCUSER_STEP_MODE_THIRD_PROPERTY       (PRIVATE_DATA->x_focuser_step_mode_third_property)
#define X_FOCUSER_STEP_MODE_THIRD_FULL_ITEM      (X_FOCUSER_STEP_MODE_THIRD_PROPERTY->items + 0)
#define X_FOCUSER_STEP_MODE_THIRD_HALF_ITEM      (X_FOCUSER_STEP_MODE_THIRD_PROPERTY->items + 1)

#define X_FOCUSER_STEP_MODE_THIRD_PROPERTY_NAME  "X_FOCUSER_STEP_MODE"
#define X_FOCUSER_STEP_MODE_THIRD_FULL_ITEM_NAME "FULL"
#define X_FOCUSER_STEP_MODE_THIRD_HALF_ITEM_NAME "HALF"

#define X_FOCUSER_POWER_CONTROL_THIRD_PROPERTY       (PRIVATE_DATA->x_focuser_power_control_third_property)
#define X_FOCUSER_POWER_CONTROL_THIRD_MOVE_ITEM      (X_FOCUSER_POWER_CONTROL_THIRD_PROPERTY->items + 0)
#define X_FOCUSER_POWER_CONTROL_THIRD_STOP_ITEM      (X_FOCUSER_POWER_CONTROL_THIRD_PROPERTY->items + 1)

#define X_FOCUSER_POWER_CONTROL_THIRD_PROPERTY_NAME  "X_FOCUSER_POWER_CONTROL"
#define X_FOCUSER_POWER_CONTROL_THIRD_MOVE_ITEM_NAME "MOVE_POWER"
#define X_FOCUSER_POWER_CONTROL_THIRD_STOP_ITEM_NAME "STOP_POWER"

#define X_FOCUSER_MOTOR_WIRING_THIRD_PROPERTY           (PRIVATE_DATA->x_focuser_motor_wiring_third_property)
#define X_FOCUSER_MOTOR_WIRING_THIRD_LUNATICO_ITEM      (X_FOCUSER_MOTOR_WIRING_THIRD_PROPERTY->items + 0)
#define X_FOCUSER_MOTOR_WIRING_THIRD_MOONLITE_ITEM      (X_FOCUSER_MOTOR_WIRING_THIRD_PROPERTY->items + 1)

#define X_FOCUSER_MOTOR_WIRING_THIRD_PROPERTY_NAME      "X_FOCUSER_MOTOR_WIRING"
#define X_FOCUSER_MOTOR_WIRING_THIRD_LUNATICO_ITEM_NAME "LUNATICO"
#define X_FOCUSER_MOTOR_WIRING_THIRD_MOONLITE_ITEM_NAME "MOONLITE"

#define X_FOCUSER_MOTOR_TYPE_THIRD_PROPERTY           (PRIVATE_DATA->x_focuser_motor_type_third_property)
#define X_FOCUSER_MOTOR_TYPE_THIRD_UNIPOLAR_ITEM      (X_FOCUSER_MOTOR_TYPE_THIRD_PROPERTY->items + 0)
#define X_FOCUSER_MOTOR_TYPE_THIRD_BIPOLAR_ITEM       (X_FOCUSER_MOTOR_TYPE_THIRD_PROPERTY->items + 1)
#define X_FOCUSER_MOTOR_TYPE_THIRD_DC_ITEM            (X_FOCUSER_MOTOR_TYPE_THIRD_PROPERTY->items + 2)
#define X_FOCUSER_MOTOR_TYPE_THIRD_STEP_DIR_ITEM      (X_FOCUSER_MOTOR_TYPE_THIRD_PROPERTY->items + 3)

#define X_FOCUSER_MOTOR_TYPE_THIRD_PROPERTY_NAME      "X_FOCUSER_MOTOR_TYPE"
#define X_FOCUSER_MOTOR_TYPE_THIRD_UNIPOLAR_ITEM_NAME "UNIPOLAR"
#define X_FOCUSER_MOTOR_TYPE_THIRD_BIPOLAR_ITEM_NAME  "BIPOLAR"
#define X_FOCUSER_MOTOR_TYPE_THIRD_DC_ITEM_NAME       "DC"
#define X_FOCUSER_MOTOR_TYPE_THIRD_STEP_DIR_ITEM_NAME "STEP_DIR"

#define X_FOCUSER_TEMPERATURE_SENSOR_THIRD_PROPERTY           (PRIVATE_DATA->x_focuser_temperature_sensor_third_property)
#define X_FOCUSER_TEMPERATURE_SENSOR_THIRD_INTERNAL_ITEM      (X_FOCUSER_TEMPERATURE_SENSOR_THIRD_PROPERTY->items + 0)
#define X_FOCUSER_TEMPERATURE_SENSOR_THIRD_EXTERNAL_ITEM      (X_FOCUSER_TEMPERATURE_SENSOR_THIRD_PROPERTY->items + 1)

#define X_FOCUSER_TEMPERATURE_SENSOR_THIRD_PROPERTY_NAME      "X_FOCUSER_TEMPERATURE_SENSOR"
#define X_FOCUSER_TEMPERATURE_SENSOR_THIRD_INTERNAL_ITEM_NAME "INTERNAL"
#define X_FOCUSER_TEMPERATURE_SENSOR_THIRD_EXTERNAL_ITEM_NAME "EXTERNAL"

#define X_ROTATOR_STEP_MODE_THIRD_PROPERTY       (PRIVATE_DATA->x_rotator_step_mode_third_property)
#define X_ROTATOR_STEP_MODE_THIRD_FULL_ITEM      (X_ROTATOR_STEP_MODE_THIRD_PROPERTY->items + 0)
#define X_ROTATOR_STEP_MODE_THIRD_HALF_ITEM      (X_ROTATOR_STEP_MODE_THIRD_PROPERTY->items + 1)

#define X_ROTATOR_STEP_MODE_THIRD_PROPERTY_NAME  "X_ROTATOR_STEP_MODE"
#define X_ROTATOR_STEP_MODE_THIRD_FULL_ITEM_NAME "FULL"
#define X_ROTATOR_STEP_MODE_THIRD_HALF_ITEM_NAME "HALF"

#define X_ROTATOR_POWER_CONTROL_THIRD_PROPERTY       (PRIVATE_DATA->x_rotator_power_control_third_property)
#define X_ROTATOR_POWER_CONTROL_THIRD_MOVE_ITEM      (X_ROTATOR_POWER_CONTROL_THIRD_PROPERTY->items + 0)
#define X_ROTATOR_POWER_CONTROL_THIRD_STOP_ITEM      (X_ROTATOR_POWER_CONTROL_THIRD_PROPERTY->items + 1)

#define X_ROTATOR_POWER_CONTROL_THIRD_PROPERTY_NAME  "X_ROTATOR_POWER_CONTROL"
#define X_ROTATOR_POWER_CONTROL_THIRD_MOVE_ITEM_NAME "MOVE_POWER"
#define X_ROTATOR_POWER_CONTROL_THIRD_STOP_ITEM_NAME "STOP_POWER"

#define X_ROTATOR_MOTOR_WIRING_THIRD_PROPERTY           (PRIVATE_DATA->x_rotator_motor_wiring_third_property)
#define X_ROTATOR_MOTOR_WIRING_THIRD_LUNATICO_ITEM      (X_ROTATOR_MOTOR_WIRING_THIRD_PROPERTY->items + 0)
#define X_ROTATOR_MOTOR_WIRING_THIRD_MOONLITE_ITEM      (X_ROTATOR_MOTOR_WIRING_THIRD_PROPERTY->items + 1)

#define X_ROTATOR_MOTOR_WIRING_THIRD_PROPERTY_NAME      "X_ROTATOR_MOTOR_WIRING"
#define X_ROTATOR_MOTOR_WIRING_THIRD_LUNATICO_ITEM_NAME "LUNATICO"
#define X_ROTATOR_MOTOR_WIRING_THIRD_MOONLITE_ITEM_NAME "MOONLITE"

#define X_ROTATOR_MOTOR_TYPE_THIRD_PROPERTY           (PRIVATE_DATA->x_rotator_motor_type_third_property)
#define X_ROTATOR_MOTOR_TYPE_THIRD_UNIPOLAR_ITEM      (X_ROTATOR_MOTOR_TYPE_THIRD_PROPERTY->items + 0)
#define X_ROTATOR_MOTOR_TYPE_THIRD_BIPOLAR_ITEM       (X_ROTATOR_MOTOR_TYPE_THIRD_PROPERTY->items + 1)
#define X_ROTATOR_MOTOR_TYPE_THIRD_DC_ITEM            (X_ROTATOR_MOTOR_TYPE_THIRD_PROPERTY->items + 2)
#define X_ROTATOR_MOTOR_TYPE_THIRD_STEP_DIR_ITEM      (X_ROTATOR_MOTOR_TYPE_THIRD_PROPERTY->items + 3)

#define X_ROTATOR_MOTOR_TYPE_THIRD_PROPERTY_NAME      "X_ROTATOR_MOTOR_TYPE"
#define X_ROTATOR_MOTOR_TYPE_THIRD_UNIPOLAR_ITEM_NAME "UNIPOLAR"
#define X_ROTATOR_MOTOR_TYPE_THIRD_BIPOLAR_ITEM_NAME  "BIPOLAR"
#define X_ROTATOR_MOTOR_TYPE_THIRD_DC_ITEM_NAME       "DC"
#define X_ROTATOR_MOTOR_TYPE_THIRD_STEP_DIR_ITEM_NAME "STEP_DIR"

#define AUX_OUTLET_NAMES_THIRD_PROPERTY (PRIVATE_DATA->aux_outlet_names_third_property)
#define AUX_OUTLET_NAME_THIRD_1_ITEM    (AUX_OUTLET_NAMES_THIRD_PROPERTY->items + 0)
#define AUX_OUTLET_NAME_THIRD_2_ITEM    (AUX_OUTLET_NAMES_THIRD_PROPERTY->items + 1)
#define AUX_OUTLET_NAME_THIRD_3_ITEM    (AUX_OUTLET_NAMES_THIRD_PROPERTY->items + 2)
#define AUX_OUTLET_NAME_THIRD_4_ITEM    (AUX_OUTLET_NAMES_THIRD_PROPERTY->items + 3)

#define AUX_POWER_OUTLET_THIRD_PROPERTY (PRIVATE_DATA->aux_power_outlet_third_property)
#define AUX_POWER_OUTLET_THIRD_1_ITEM   (AUX_POWER_OUTLET_THIRD_PROPERTY->items + 0)
#define AUX_POWER_OUTLET_THIRD_2_ITEM   (AUX_POWER_OUTLET_THIRD_PROPERTY->items + 1)
#define AUX_POWER_OUTLET_THIRD_3_ITEM   (AUX_POWER_OUTLET_THIRD_PROPERTY->items + 2)
#define AUX_POWER_OUTLET_THIRD_4_ITEM   (AUX_POWER_OUTLET_THIRD_PROPERTY->items + 3)

#define AUX_SENSOR_NAMES_THIRD_PROPERTY (PRIVATE_DATA->aux_sensor_names_third_property)
#define AUX_SENSOR_NAME_THIRD_1_ITEM    (AUX_SENSOR_NAMES_THIRD_PROPERTY->items + 0)
#define AUX_SENSOR_NAME_THIRD_2_ITEM    (AUX_SENSOR_NAMES_THIRD_PROPERTY->items + 1)
#define AUX_SENSOR_NAME_THIRD_3_ITEM    (AUX_SENSOR_NAMES_THIRD_PROPERTY->items + 2)
#define AUX_SENSOR_NAME_THIRD_4_ITEM    (AUX_SENSOR_NAMES_THIRD_PROPERTY->items + 3)

#define AUX_GPIO_SENSORS_THIRD_PROPERTY (PRIVATE_DATA->aux_gpio_sensors_third_property)
#define AUX_GPIO_SENSOR_THIRD_1_ITEM    (AUX_GPIO_SENSORS_THIRD_PROPERTY->items + 0)
#define AUX_GPIO_SENSOR_THIRD_2_ITEM    (AUX_GPIO_SENSORS_THIRD_PROPERTY->items + 1)
#define AUX_GPIO_SENSOR_THIRD_3_ITEM    (AUX_GPIO_SENSORS_THIRD_PROPERTY->items + 2)
#define AUX_GPIO_SENSOR_THIRD_4_ITEM    (AUX_GPIO_SENSORS_THIRD_PROPERTY->items + 3)

#pragma mark - Private data definition

typedef struct {
	int count;
	indigo_uni_handle *handle;
	indigo_property *x_rotator_step_mode_main_property;
	indigo_property *x_rotator_power_control_main_property;
	indigo_property *x_rotator_motor_wiring_main_property;
	indigo_property *x_rotator_motor_type_main_property;
	indigo_property *x_focuser_step_mode_exp_property;
	indigo_property *x_focuser_power_control_exp_property;
	indigo_property *x_focuser_motor_wiring_exp_property;
	indigo_property *x_focuser_motor_type_exp_property;
	indigo_property *x_focuser_temperature_sensor_exp_property;
	indigo_property *x_rotator_step_mode_exp_property;
	indigo_property *x_rotator_power_control_exp_property;
	indigo_property *x_rotator_motor_wiring_exp_property;
	indigo_property *x_rotator_motor_type_exp_property;
	indigo_property *aux_outlet_names_exp_property;
	indigo_property *aux_power_outlet_exp_property;
	indigo_property *aux_sensor_names_exp_property;
	indigo_property *aux_gpio_sensors_exp_property;
	indigo_property *x_focuser_step_mode_third_property;
	indigo_property *x_focuser_power_control_third_property;
	indigo_property *x_focuser_motor_wiring_third_property;
	indigo_property *x_focuser_motor_type_third_property;
	indigo_property *x_focuser_temperature_sensor_third_property;
	indigo_property *x_rotator_step_mode_third_property;
	indigo_property *x_rotator_power_control_third_property;
	indigo_property *x_rotator_motor_wiring_third_property;
	indigo_property *x_rotator_motor_type_third_property;
	indigo_property *aux_outlet_names_third_property;
	indigo_property *aux_power_outlet_third_property;
	indigo_property *aux_sensor_names_third_property;
	indigo_property *aux_gpio_sensors_third_property;
	//+ data
	char command[LUNATICO_CMD_LEN];
	char response[LUNATICO_CMD_LEN];
	char board[INDIGO_VALUE_SIZE];
	char firmware[INDIGO_VALUE_SIZE];
	int model;
	lunatico_port_state port[LUNATICO_PORTS];
	//- data
} lunatico_private_data;

#pragma mark - Low level code

//+ code

#include "../focuser_lunatico/shared/lunatico_shared.c"

//- code

#pragma mark - High level code (rotator_main)
// device_id: rotator_main type: rotator

static void rotator_main_connection_handler(indigo_device *device) {
	if (CONNECTION_CONNECTED_ITEM->sw.value) {
		bool connection_result = true;
		if (PRIVATE_DATA->count == 0) {
			connection_result = lunatico_open(device);
		}
		if (connection_result) {
			PRIVATE_DATA->count++;
		}
		if (connection_result) {
			//+ rotator_main.on_connect
			connection_result = lunatico_rotator_connect(device, X_ROTATOR_POWER_CONTROL_MAIN_PROPERTY, X_ROTATOR_MOTOR_TYPE_MAIN_PROPERTY, X_ROTATOR_STEP_MODE_MAIN_PROPERTY, X_ROTATOR_MOTOR_WIRING_MAIN_PROPERTY);
			//- rotator_main.on_connect
		}
		if (connection_result) {
			indigo_define_property(device, X_ROTATOR_STEP_MODE_MAIN_PROPERTY, NULL);
			indigo_define_property(device, X_ROTATOR_POWER_CONTROL_MAIN_PROPERTY, NULL);
			indigo_define_property(device, X_ROTATOR_MOTOR_WIRING_MAIN_PROPERTY, NULL);
			indigo_define_property(device, X_ROTATOR_MOTOR_TYPE_MAIN_PROPERTY, NULL);
			CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
			indigo_send_message(device, OK_PROPERTY, "Connected to %s on %s", ROTATOR_MAIN_DEVICE_NAME, DEVICE_PORT_ITEM->text.value);
		} else {
			indigo_send_message(device, ALERT_PROPERTY, "Failed to connect to %s on %s", ROTATOR_MAIN_DEVICE_NAME, DEVICE_PORT_ITEM->text.value);
			if (PRIVATE_DATA->count > 0 && --PRIVATE_DATA->count == 0) {
				lunatico_close(device);
			}
			CONNECTION_PROPERTY->state = INDIGO_ALERT_STATE;
			indigo_set_switch(CONNECTION_PROPERTY, CONNECTION_DISCONNECTED_ITEM, true);
		}
	} else {
		indigo_cancel_pending_handlers(device);
		//+ rotator_main.on_disconnect
		lunatico_release_port(device);
		//- rotator_main.on_disconnect
		indigo_delete_property(device, X_ROTATOR_STEP_MODE_MAIN_PROPERTY, NULL);
		indigo_delete_property(device, X_ROTATOR_POWER_CONTROL_MAIN_PROPERTY, NULL);
		indigo_delete_property(device, X_ROTATOR_MOTOR_WIRING_MAIN_PROPERTY, NULL);
		indigo_delete_property(device, X_ROTATOR_MOTOR_TYPE_MAIN_PROPERTY, NULL);
		if (--PRIVATE_DATA->count == 0) {
			lunatico_close(device);
		}
		indigo_send_message(device, OK_PROPERTY, "Disconnected from %s", device->name);
		CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
	}
	indigo_rotator_change_property(device, NULL, CONNECTION_PROPERTY);
}

static void rotator_main_rotator_steps_per_revolution_handler(indigo_device *device) {
	ROTATOR_STEPS_PER_REVOLUTION_PROPERTY->state = INDIGO_OK_STATE;
	//+ rotator_main.ROTATOR_STEPS_PER_REVOLUTION.on_change
	lunatico_rotator_resync(device);
	//- rotator_main.ROTATOR_STEPS_PER_REVOLUTION.on_change
	indigo_update_property(device, ROTATOR_STEPS_PER_REVOLUTION_PROPERTY, NULL);
}

static void rotator_main_rotator_direction_handler(indigo_device *device) {
	ROTATOR_DIRECTION_PROPERTY->state = INDIGO_OK_STATE;
	//+ rotator_main.ROTATOR_DIRECTION.on_change
	if (!lunatico_apply_wiring(device, X_ROTATOR_MOTOR_WIRING_MAIN_PROPERTY, !ROTATOR_DIRECTION_NORMAL_ITEM->sw.value)) {
		ROTATOR_DIRECTION_PROPERTY->state = INDIGO_ALERT_STATE;
	}
	//- rotator_main.ROTATOR_DIRECTION.on_change
	indigo_update_property(device, ROTATOR_DIRECTION_PROPERTY, NULL);
}

static void rotator_main_rotator_limits_handler(indigo_device *device) {
	ROTATOR_LIMITS_PROPERTY->state = INDIGO_OK_STATE;
	//+ rotator_main.ROTATOR_LIMITS.on_change
	lunatico_rotator_limits(device);
	//- rotator_main.ROTATOR_LIMITS.on_change
	indigo_update_property(device, ROTATOR_LIMITS_PROPERTY, NULL);
}

static void rotator_main_rotator_position_handler(indigo_device *device) {
	//+ rotator_main.ROTATOR_POSITION.on_change
	if (lunatico_rotator_position(device)) {
		indigo_execute_handler_in(device, LUNATICO_MOTION_POLL, rotator_motion_finalizer);
	}
	//- rotator_main.ROTATOR_POSITION.on_change
}

static void rotator_main_rotator_abort_motion_handler(indigo_device *device) {
	//+ rotator_main.ROTATOR_ABORT_MOTION.on_change
	// An urgent abort can overtake a queued move, so the start
	// handler of this device is cancelled together with the
	// completion poll it would have scheduled.
	indigo_cancel_pending_handler(device, rotator_main_rotator_position_handler);
	indigo_cancel_pending_handler(device, rotator_motion_finalizer);
	lunatico_rotator_abort(device);
	//- rotator_main.ROTATOR_ABORT_MOTION.on_change
}

static void rotator_main_x_rotator_step_mode_main_handler(indigo_device *device) {
	X_ROTATOR_STEP_MODE_MAIN_PROPERTY->state = INDIGO_OK_STATE;
	//+ rotator_main.X_ROTATOR_STEP_MODE_MAIN.on_change
	if (!lunatico_apply_step_mode(device, X_ROTATOR_STEP_MODE_MAIN_PROPERTY)) {
		X_ROTATOR_STEP_MODE_MAIN_PROPERTY->state = INDIGO_ALERT_STATE;
	}
	//- rotator_main.X_ROTATOR_STEP_MODE_MAIN.on_change
	indigo_update_property(device, X_ROTATOR_STEP_MODE_MAIN_PROPERTY, NULL);
}

static void rotator_main_x_rotator_power_control_main_handler(indigo_device *device) {
	X_ROTATOR_POWER_CONTROL_MAIN_PROPERTY->state = INDIGO_OK_STATE;
	//+ rotator_main.X_ROTATOR_POWER_CONTROL_MAIN.on_change
	if (!lunatico_apply_power_control(device, X_ROTATOR_POWER_CONTROL_MAIN_PROPERTY)) {
		X_ROTATOR_POWER_CONTROL_MAIN_PROPERTY->state = INDIGO_ALERT_STATE;
	}
	//- rotator_main.X_ROTATOR_POWER_CONTROL_MAIN.on_change
	indigo_update_property(device, X_ROTATOR_POWER_CONTROL_MAIN_PROPERTY, NULL);
}

static void rotator_main_x_rotator_motor_wiring_main_handler(indigo_device *device) {
	X_ROTATOR_MOTOR_WIRING_MAIN_PROPERTY->state = INDIGO_OK_STATE;
	//+ rotator_main.X_ROTATOR_MOTOR_WIRING_MAIN.on_change
	if (!lunatico_apply_wiring(device, X_ROTATOR_MOTOR_WIRING_MAIN_PROPERTY, !ROTATOR_DIRECTION_NORMAL_ITEM->sw.value)) {
		X_ROTATOR_MOTOR_WIRING_MAIN_PROPERTY->state = INDIGO_ALERT_STATE;
	}
	//- rotator_main.X_ROTATOR_MOTOR_WIRING_MAIN.on_change
	indigo_update_property(device, X_ROTATOR_MOTOR_WIRING_MAIN_PROPERTY, NULL);
}

static void rotator_main_x_rotator_motor_type_main_handler(indigo_device *device) {
	X_ROTATOR_MOTOR_TYPE_MAIN_PROPERTY->state = INDIGO_OK_STATE;
	//+ rotator_main.X_ROTATOR_MOTOR_TYPE_MAIN.on_change
	if (!lunatico_apply_motor_type(device, X_ROTATOR_MOTOR_TYPE_MAIN_PROPERTY)) {
		X_ROTATOR_MOTOR_TYPE_MAIN_PROPERTY->state = INDIGO_ALERT_STATE;
	}
	//- rotator_main.X_ROTATOR_MOTOR_TYPE_MAIN.on_change
	indigo_update_property(device, X_ROTATOR_MOTOR_TYPE_MAIN_PROPERTY, NULL);
}

#pragma mark - Device API (rotator_main)

static indigo_result rotator_main_enumerate_properties(indigo_device *device, indigo_client *client, indigo_property *property);

static indigo_result rotator_main_attach(indigo_device *device) {
	if (indigo_rotator_attach(device, DRIVER_NAME, DRIVER_VERSION) == INDIGO_OK) {
		DEVICE_PORT_PROPERTY->hidden = false;
		DEVICE_PORTS_PROPERTY->hidden = false;
		indigo_enumerate_serial_ports(device, DEVICE_PORTS_PROPERTY);
		DEVICE_BAUDRATE_PROPERTY->hidden = false;
		//+ rotator_main.on_attach
		device->gp_bits = 0;
		INFO_PROPERTY->count = 6;
		INDIGO_COPY_VALUE(DEVICE_BAUDRATE_ITEM->text.value, LUNATICO_BAUDRATE);
		//- rotator_main.on_attach
		ROTATOR_STEPS_PER_REVOLUTION_PROPERTY->hidden = false;
		//+ rotator_main.ROTATOR_STEPS_PER_REVOLUTION.on_attach
		ROTATOR_STEPS_PER_REVOLUTION_ITEM->number.min = 100;
		ROTATOR_STEPS_PER_REVOLUTION_ITEM->number.max = 100000;
		ROTATOR_STEPS_PER_REVOLUTION_ITEM->number.value = ROTATOR_STEPS_PER_REVOLUTION_ITEM->number.target = 3600;
		//- rotator_main.ROTATOR_STEPS_PER_REVOLUTION.on_attach
		ROTATOR_DIRECTION_PROPERTY->hidden = false;
		ROTATOR_BACKLASH_PROPERTY->hidden = false;
		ROTATOR_LIMITS_PROPERTY->hidden = false;
		//+ rotator_main.ROTATOR_LIMITS.on_attach
		ROTATOR_LIMITS_MIN_POSITION_ITEM->number.min = -180;
		ROTATOR_LIMITS_MIN_POSITION_ITEM->number.max = 360;
		ROTATOR_LIMITS_MIN_POSITION_ITEM->number.value = ROTATOR_LIMITS_MIN_POSITION_ITEM->number.target = -180;
		ROTATOR_LIMITS_MAX_POSITION_ITEM->number.min = -180;
		ROTATOR_LIMITS_MAX_POSITION_ITEM->number.max = 360;
		ROTATOR_LIMITS_MAX_POSITION_ITEM->number.value = ROTATOR_LIMITS_MAX_POSITION_ITEM->number.target = 180;
		//- rotator_main.ROTATOR_LIMITS.on_attach
		ROTATOR_POSITION_PROPERTY->hidden = false;
		ROTATOR_ABORT_MOTION_PROPERTY->hidden = false;
		X_ROTATOR_STEP_MODE_MAIN_PROPERTY = indigo_init_switch_property(NULL, device->name, X_ROTATOR_STEP_MODE_MAIN_PROPERTY_NAME, ROTATOR_ADVANCED_GROUP, "Step mode", INDIGO_OK_STATE, INDIGO_RW_PERM, INDIGO_ONE_OF_MANY_RULE, 2);
		if (X_ROTATOR_STEP_MODE_MAIN_PROPERTY == NULL) {
			return INDIGO_FAILED;
		}
		indigo_init_switch_item(X_ROTATOR_STEP_MODE_MAIN_FULL_ITEM, X_ROTATOR_STEP_MODE_MAIN_FULL_ITEM_NAME, "Full step", true);
		indigo_init_switch_item(X_ROTATOR_STEP_MODE_MAIN_HALF_ITEM, X_ROTATOR_STEP_MODE_MAIN_HALF_ITEM_NAME, "1/2 step", false);
		X_ROTATOR_POWER_CONTROL_MAIN_PROPERTY = indigo_init_number_property(NULL, device->name, X_ROTATOR_POWER_CONTROL_MAIN_PROPERTY_NAME, ROTATOR_ADVANCED_GROUP, "Coils current control", INDIGO_OK_STATE, INDIGO_RW_PERM, 2);
		if (X_ROTATOR_POWER_CONTROL_MAIN_PROPERTY == NULL) {
			return INDIGO_FAILED;
		}
		indigo_init_number_item(X_ROTATOR_POWER_CONTROL_MAIN_MOVE_ITEM, X_ROTATOR_POWER_CONTROL_MAIN_MOVE_ITEM_NAME, "Move power (%)", 0, 100, 1, 100);
		indigo_init_number_item(X_ROTATOR_POWER_CONTROL_MAIN_STOP_ITEM, X_ROTATOR_POWER_CONTROL_MAIN_STOP_ITEM_NAME, "Stop power (%)", 0, 100, 1, 0);
		X_ROTATOR_MOTOR_WIRING_MAIN_PROPERTY = indigo_init_switch_property(NULL, device->name, X_ROTATOR_MOTOR_WIRING_MAIN_PROPERTY_NAME, ROTATOR_ADVANCED_GROUP, "Motor wiring", INDIGO_OK_STATE, INDIGO_RW_PERM, INDIGO_ONE_OF_MANY_RULE, 2);
		if (X_ROTATOR_MOTOR_WIRING_MAIN_PROPERTY == NULL) {
			return INDIGO_FAILED;
		}
		indigo_init_switch_item(X_ROTATOR_MOTOR_WIRING_MAIN_LUNATICO_ITEM, X_ROTATOR_MOTOR_WIRING_MAIN_LUNATICO_ITEM_NAME, "Lunatico", true);
		indigo_init_switch_item(X_ROTATOR_MOTOR_WIRING_MAIN_MOONLITE_ITEM, X_ROTATOR_MOTOR_WIRING_MAIN_MOONLITE_ITEM_NAME, "RF/Moonlite", false);
		X_ROTATOR_MOTOR_TYPE_MAIN_PROPERTY = indigo_init_switch_property(NULL, device->name, X_ROTATOR_MOTOR_TYPE_MAIN_PROPERTY_NAME, ROTATOR_ADVANCED_GROUP, "Motor type", INDIGO_OK_STATE, INDIGO_RW_PERM, INDIGO_ONE_OF_MANY_RULE, 4);
		if (X_ROTATOR_MOTOR_TYPE_MAIN_PROPERTY == NULL) {
			return INDIGO_FAILED;
		}
		indigo_init_switch_item(X_ROTATOR_MOTOR_TYPE_MAIN_UNIPOLAR_ITEM, X_ROTATOR_MOTOR_TYPE_MAIN_UNIPOLAR_ITEM_NAME, "Unipolar", true);
		indigo_init_switch_item(X_ROTATOR_MOTOR_TYPE_MAIN_BIPOLAR_ITEM, X_ROTATOR_MOTOR_TYPE_MAIN_BIPOLAR_ITEM_NAME, "Bipolar", false);
		indigo_init_switch_item(X_ROTATOR_MOTOR_TYPE_MAIN_DC_ITEM, X_ROTATOR_MOTOR_TYPE_MAIN_DC_ITEM_NAME, "DC", false);
		indigo_init_switch_item(X_ROTATOR_MOTOR_TYPE_MAIN_STEP_DIR_ITEM, X_ROTATOR_MOTOR_TYPE_MAIN_STEP_DIR_ITEM_NAME, "Step-dir", false);
		INDIGO_DEVICE_ATTACH_LOG(DRIVER_NAME, device->name);
		return rotator_main_enumerate_properties(device, NULL, NULL);
	}
	return INDIGO_FAILED;
}

static indigo_result rotator_main_enumerate_properties(indigo_device *device, indigo_client *client, indigo_property *property) {
	if (IS_CONNECTED) {
		INDIGO_DEFINE_MATCHING_PROPERTY(X_ROTATOR_STEP_MODE_MAIN_PROPERTY);
		INDIGO_DEFINE_MATCHING_PROPERTY(X_ROTATOR_POWER_CONTROL_MAIN_PROPERTY);
		INDIGO_DEFINE_MATCHING_PROPERTY(X_ROTATOR_MOTOR_WIRING_MAIN_PROPERTY);
		INDIGO_DEFINE_MATCHING_PROPERTY(X_ROTATOR_MOTOR_TYPE_MAIN_PROPERTY);
	}
	return indigo_rotator_enumerate_properties(device, client, property);
}

static indigo_result rotator_main_change_property(indigo_device *device, indigo_client *client, indigo_property *property) {
	if (indigo_property_match_changeable(CONNECTION_PROPERTY, property)) {
		INDIGO_PROCESS_CONNECT(rotator_main_connection_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(ROTATOR_STEPS_PER_REVOLUTION_PROPERTY, property)) {
		INDIGO_COPY_VALUES_PROCESS_CHANGE(ROTATOR_STEPS_PER_REVOLUTION_PROPERTY, rotator_main_rotator_steps_per_revolution_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(ROTATOR_DIRECTION_PROPERTY, property)) {
		INDIGO_COPY_VALUES_PROCESS_CHANGE(ROTATOR_DIRECTION_PROPERTY, rotator_main_rotator_direction_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(ROTATOR_BACKLASH_PROPERTY, property)) {
		indigo_property_copy_values(ROTATOR_BACKLASH_PROPERTY, property, false);
		ROTATOR_BACKLASH_PROPERTY->state = INDIGO_OK_STATE;
		indigo_update_property(device, ROTATOR_BACKLASH_PROPERTY, NULL);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(ROTATOR_LIMITS_PROPERTY, property)) {
		INDIGO_COPY_VALUES_PROCESS_CHANGE(ROTATOR_LIMITS_PROPERTY, rotator_main_rotator_limits_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(ROTATOR_POSITION_PROPERTY, property)) {
		INDIGO_COPY_VALUES_PROCESS_CHANGE(ROTATOR_POSITION_PROPERTY, rotator_main_rotator_position_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(ROTATOR_ABORT_MOTION_PROPERTY, property)) {
		INDIGO_COPY_VALUES_PROCESS_URGENT_CHANGE(ROTATOR_ABORT_MOTION_PROPERTY, rotator_main_rotator_abort_motion_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(X_ROTATOR_STEP_MODE_MAIN_PROPERTY, property)) {
		INDIGO_COPY_VALUES_PROCESS_CHANGE(X_ROTATOR_STEP_MODE_MAIN_PROPERTY, rotator_main_x_rotator_step_mode_main_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(X_ROTATOR_POWER_CONTROL_MAIN_PROPERTY, property)) {
		INDIGO_COPY_VALUES_PROCESS_CHANGE(X_ROTATOR_POWER_CONTROL_MAIN_PROPERTY, rotator_main_x_rotator_power_control_main_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(X_ROTATOR_MOTOR_WIRING_MAIN_PROPERTY, property)) {
		INDIGO_COPY_VALUES_PROCESS_CHANGE(X_ROTATOR_MOTOR_WIRING_MAIN_PROPERTY, rotator_main_x_rotator_motor_wiring_main_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(X_ROTATOR_MOTOR_TYPE_MAIN_PROPERTY, property)) {
		INDIGO_COPY_VALUES_PROCESS_CHANGE(X_ROTATOR_MOTOR_TYPE_MAIN_PROPERTY, rotator_main_x_rotator_motor_type_main_handler);
		return INDIGO_OK;
	} else if (indigo_property_match(CONFIG_PROPERTY, property)) {
		if (indigo_switch_match(CONFIG_SAVE_ITEM, property)) {
			indigo_save_property(device, NULL, ROTATOR_STEPS_PER_REVOLUTION_PROPERTY);
			indigo_save_property(device, NULL, ROTATOR_DIRECTION_PROPERTY);
			indigo_save_property(device, NULL, ROTATOR_BACKLASH_PROPERTY);
			indigo_save_property(device, NULL, ROTATOR_LIMITS_PROPERTY);
			indigo_save_property(device, NULL, X_ROTATOR_STEP_MODE_MAIN_PROPERTY);
			indigo_save_property(device, NULL, X_ROTATOR_POWER_CONTROL_MAIN_PROPERTY);
			indigo_save_property(device, NULL, X_ROTATOR_MOTOR_WIRING_MAIN_PROPERTY);
			indigo_save_property(device, NULL, X_ROTATOR_MOTOR_TYPE_MAIN_PROPERTY);
		}
	}
	return indigo_rotator_change_property(device, client, property);
}

static indigo_result rotator_main_detach(indigo_device *device) {
	if (IS_CONNECTED) {
		indigo_set_switch(CONNECTION_PROPERTY, CONNECTION_DISCONNECTED_ITEM, true);
		rotator_main_connection_handler(device);
	}
	indigo_release_property(X_ROTATOR_STEP_MODE_MAIN_PROPERTY);
	indigo_release_property(X_ROTATOR_POWER_CONTROL_MAIN_PROPERTY);
	indigo_release_property(X_ROTATOR_MOTOR_WIRING_MAIN_PROPERTY);
	indigo_release_property(X_ROTATOR_MOTOR_TYPE_MAIN_PROPERTY);
	INDIGO_DEVICE_DETACH_LOG(DRIVER_NAME, device->name);
	return indigo_rotator_detach(device);
}

#pragma mark - High level code (focuser_exp)
// device_id: focuser_exp type: focuser

static void focuser_exp_timer_callback(indigo_device *device) {
	if (!IS_CONNECTED) {
		return;
	}
	//+ focuser_exp.on_timer
	lunatico_focuser_poll_temperature(device);
	indigo_execute_handler_in(device, LUNATICO_SENSOR_POLL, focuser_exp_timer_callback);
	//- focuser_exp.on_timer
}

static void focuser_exp_connection_handler(indigo_device *device) {
	if (CONNECTION_CONNECTED_ITEM->sw.value) {
		bool connection_result = true;
		if (PRIVATE_DATA->count == 0) {
			connection_result = lunatico_open(device->master_device);
		}
		if (connection_result) {
			PRIVATE_DATA->count++;
		}
		if (connection_result) {
			//+ focuser_exp.on_connect
			connection_result = lunatico_focuser_connect(device, X_FOCUSER_POWER_CONTROL_EXP_PROPERTY, X_FOCUSER_MOTOR_TYPE_EXP_PROPERTY, X_FOCUSER_STEP_MODE_EXP_PROPERTY, X_FOCUSER_MOTOR_WIRING_EXP_PROPERTY, X_FOCUSER_TEMPERATURE_SENSOR_EXP_PROPERTY);
			//- focuser_exp.on_connect
		}
		if (connection_result) {
			indigo_define_property(device, X_FOCUSER_STEP_MODE_EXP_PROPERTY, NULL);
			indigo_define_property(device, X_FOCUSER_POWER_CONTROL_EXP_PROPERTY, NULL);
			indigo_define_property(device, X_FOCUSER_MOTOR_WIRING_EXP_PROPERTY, NULL);
			indigo_define_property(device, X_FOCUSER_MOTOR_TYPE_EXP_PROPERTY, NULL);
			indigo_define_property(device, X_FOCUSER_TEMPERATURE_SENSOR_EXP_PROPERTY, NULL);
			CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
			indigo_send_message(device, OK_PROPERTY, "Connected to %s on %s", FOCUSER_EXP_DEVICE_NAME, DEVICE_PORT_ITEM->text.value);
		} else {
			indigo_send_message(device, ALERT_PROPERTY, "Failed to connect to %s on %s", FOCUSER_EXP_DEVICE_NAME, DEVICE_PORT_ITEM->text.value);
			if (PRIVATE_DATA->count > 0 && --PRIVATE_DATA->count == 0) {
				lunatico_close(device);
			}
			CONNECTION_PROPERTY->state = INDIGO_ALERT_STATE;
			indigo_set_switch(CONNECTION_PROPERTY, CONNECTION_DISCONNECTED_ITEM, true);
		}
	} else {
		indigo_cancel_pending_handlers(device);
		//+ focuser_exp.on_disconnect
		lunatico_release_port(device);
		//- focuser_exp.on_disconnect
		indigo_delete_property(device, X_FOCUSER_STEP_MODE_EXP_PROPERTY, NULL);
		indigo_delete_property(device, X_FOCUSER_POWER_CONTROL_EXP_PROPERTY, NULL);
		indigo_delete_property(device, X_FOCUSER_MOTOR_WIRING_EXP_PROPERTY, NULL);
		indigo_delete_property(device, X_FOCUSER_MOTOR_TYPE_EXP_PROPERTY, NULL);
		indigo_delete_property(device, X_FOCUSER_TEMPERATURE_SENSOR_EXP_PROPERTY, NULL);
		if (--PRIVATE_DATA->count == 0) {
			lunatico_close(device);
		}
		indigo_send_message(device, OK_PROPERTY, "Disconnected from %s", device->name);
		CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
	}
	indigo_focuser_change_property(device, NULL, CONNECTION_PROPERTY);
	if (IS_CONNECTED) {
		indigo_execute_handler(device, focuser_exp_timer_callback);
	}
}

static void focuser_exp_focuser_speed_handler(indigo_device *device) {
	FOCUSER_SPEED_PROPERTY->state = INDIGO_OK_STATE;
	//+ focuser_exp.FOCUSER_SPEED.on_change
	if (!lunatico_set_speed(device, FOCUSER_SPEED_ITEM->number.target)) {
		FOCUSER_SPEED_PROPERTY->state = INDIGO_ALERT_STATE;
	}
	//- focuser_exp.FOCUSER_SPEED.on_change
	indigo_update_property(device, FOCUSER_SPEED_PROPERTY, NULL);
}

static void focuser_exp_focuser_limits_handler(indigo_device *device) {
	FOCUSER_LIMITS_PROPERTY->state = INDIGO_OK_STATE;
	//+ focuser_exp.FOCUSER_LIMITS.on_change
	lunatico_focuser_limits(device);
	//- focuser_exp.FOCUSER_LIMITS.on_change
	indigo_update_property(device, FOCUSER_LIMITS_PROPERTY, NULL);
}

static void focuser_exp_focuser_mode_handler(indigo_device *device) {
	FOCUSER_MODE_PROPERTY->state = INDIGO_OK_STATE;
	//+ focuser_exp.FOCUSER_MODE.on_change
	lunatico_focuser_mode(device);
	//- focuser_exp.FOCUSER_MODE.on_change
	indigo_update_property(device, FOCUSER_MODE_PROPERTY, NULL);
}

static void focuser_exp_focuser_reverse_motion_handler(indigo_device *device) {
	FOCUSER_REVERSE_MOTION_PROPERTY->state = INDIGO_OK_STATE;
	//+ focuser_exp.FOCUSER_REVERSE_MOTION.on_change
	if (!lunatico_apply_wiring(device, X_FOCUSER_MOTOR_WIRING_EXP_PROPERTY, !FOCUSER_REVERSE_MOTION_DISABLED_ITEM->sw.value)) {
		FOCUSER_REVERSE_MOTION_PROPERTY->state = INDIGO_ALERT_STATE;
	}
	//- focuser_exp.FOCUSER_REVERSE_MOTION.on_change
	indigo_update_property(device, FOCUSER_REVERSE_MOTION_PROPERTY, NULL);
}

static void focuser_exp_focuser_position_handler(indigo_device *device) {
	//+ focuser_exp.FOCUSER_POSITION.on_change
	if (lunatico_focuser_position(device)) {
		indigo_execute_handler_in(device, LUNATICO_MOTION_POLL, focuser_motion_finalizer);
	}
	//- focuser_exp.FOCUSER_POSITION.on_change
}

static void focuser_exp_focuser_steps_handler(indigo_device *device) {
	//+ focuser_exp.FOCUSER_STEPS.on_change
	if (lunatico_focuser_steps(device)) {
		indigo_execute_handler_in(device, LUNATICO_MOTION_POLL, focuser_motion_finalizer);
	}
	//- focuser_exp.FOCUSER_STEPS.on_change
}

static void focuser_exp_focuser_abort_motion_handler(indigo_device *device) {
	//+ focuser_exp.FOCUSER_ABORT_MOTION.on_change
	// An urgent abort can overtake a queued move, so the start
	// handlers of this device are cancelled together with the
	// completion poll they would have scheduled.
	indigo_cancel_pending_handler(device, focuser_exp_focuser_position_handler);
	indigo_cancel_pending_handler(device, focuser_exp_focuser_steps_handler);
	indigo_cancel_pending_handler(device, focuser_motion_finalizer);
	lunatico_focuser_abort(device);
	//- focuser_exp.FOCUSER_ABORT_MOTION.on_change
}

static void focuser_exp_x_focuser_step_mode_exp_handler(indigo_device *device) {
	X_FOCUSER_STEP_MODE_EXP_PROPERTY->state = INDIGO_OK_STATE;
	//+ focuser_exp.X_FOCUSER_STEP_MODE_EXP.on_change
	if (!lunatico_apply_step_mode(device, X_FOCUSER_STEP_MODE_EXP_PROPERTY)) {
		X_FOCUSER_STEP_MODE_EXP_PROPERTY->state = INDIGO_ALERT_STATE;
	}
	//- focuser_exp.X_FOCUSER_STEP_MODE_EXP.on_change
	indigo_update_property(device, X_FOCUSER_STEP_MODE_EXP_PROPERTY, NULL);
}

static void focuser_exp_x_focuser_power_control_exp_handler(indigo_device *device) {
	X_FOCUSER_POWER_CONTROL_EXP_PROPERTY->state = INDIGO_OK_STATE;
	//+ focuser_exp.X_FOCUSER_POWER_CONTROL_EXP.on_change
	if (!lunatico_apply_power_control(device, X_FOCUSER_POWER_CONTROL_EXP_PROPERTY)) {
		X_FOCUSER_POWER_CONTROL_EXP_PROPERTY->state = INDIGO_ALERT_STATE;
	}
	//- focuser_exp.X_FOCUSER_POWER_CONTROL_EXP.on_change
	indigo_update_property(device, X_FOCUSER_POWER_CONTROL_EXP_PROPERTY, NULL);
}

static void focuser_exp_x_focuser_motor_wiring_exp_handler(indigo_device *device) {
	X_FOCUSER_MOTOR_WIRING_EXP_PROPERTY->state = INDIGO_OK_STATE;
	//+ focuser_exp.X_FOCUSER_MOTOR_WIRING_EXP.on_change
	if (!lunatico_apply_wiring(device, X_FOCUSER_MOTOR_WIRING_EXP_PROPERTY, !FOCUSER_REVERSE_MOTION_DISABLED_ITEM->sw.value)) {
		X_FOCUSER_MOTOR_WIRING_EXP_PROPERTY->state = INDIGO_ALERT_STATE;
	}
	//- focuser_exp.X_FOCUSER_MOTOR_WIRING_EXP.on_change
	indigo_update_property(device, X_FOCUSER_MOTOR_WIRING_EXP_PROPERTY, NULL);
}

static void focuser_exp_x_focuser_motor_type_exp_handler(indigo_device *device) {
	X_FOCUSER_MOTOR_TYPE_EXP_PROPERTY->state = INDIGO_OK_STATE;
	//+ focuser_exp.X_FOCUSER_MOTOR_TYPE_EXP.on_change
	if (!lunatico_apply_motor_type(device, X_FOCUSER_MOTOR_TYPE_EXP_PROPERTY)) {
		X_FOCUSER_MOTOR_TYPE_EXP_PROPERTY->state = INDIGO_ALERT_STATE;
	}
	//- focuser_exp.X_FOCUSER_MOTOR_TYPE_EXP.on_change
	indigo_update_property(device, X_FOCUSER_MOTOR_TYPE_EXP_PROPERTY, NULL);
}

static void focuser_exp_x_focuser_temperature_sensor_exp_handler(indigo_device *device) {
	X_FOCUSER_TEMPERATURE_SENSOR_EXP_PROPERTY->state = INDIGO_OK_STATE;
	//+ focuser_exp.X_FOCUSER_TEMPERATURE_SENSOR_EXP.on_change
	lunatico_apply_temperature_sensor(device, X_FOCUSER_TEMPERATURE_SENSOR_EXP_PROPERTY);
	//- focuser_exp.X_FOCUSER_TEMPERATURE_SENSOR_EXP.on_change
	indigo_update_property(device, X_FOCUSER_TEMPERATURE_SENSOR_EXP_PROPERTY, NULL);
}

#pragma mark - Device API (focuser_exp)

static indigo_result focuser_exp_enumerate_properties(indigo_device *device, indigo_client *client, indigo_property *property);

static indigo_result focuser_exp_attach(indigo_device *device) {
	if (indigo_focuser_attach(device, DRIVER_NAME, DRIVER_VERSION) == INDIGO_OK) {
		//+ focuser_exp.on_attach
		device->gp_bits = 1;
		INFO_PROPERTY->count = 6;
		//- focuser_exp.on_attach
		FOCUSER_TEMPERATURE_PROPERTY->hidden = false;
		FOCUSER_ON_POSITION_SET_PROPERTY->hidden = false;
		FOCUSER_SPEED_PROPERTY->hidden = false;
		//+ focuser_exp.FOCUSER_SPEED.on_attach
		FOCUSER_SPEED_ITEM->number.min = .002;
		FOCUSER_SPEED_ITEM->number.max = 20;
		FOCUSER_SPEED_ITEM->number.step = .1;
		FOCUSER_SPEED_ITEM->number.value = FOCUSER_SPEED_ITEM->number.target = .1;
		strcpy(FOCUSER_SPEED_ITEM->label, "Speed (kHz)");
		//- focuser_exp.FOCUSER_SPEED.on_attach
		FOCUSER_LIMITS_PROPERTY->hidden = false;
		//+ focuser_exp.FOCUSER_LIMITS.on_attach
		FOCUSER_LIMITS_MAX_POSITION_ITEM->number.min = 1;
		FOCUSER_LIMITS_MAX_POSITION_ITEM->number.max = 100000;
		FOCUSER_LIMITS_MAX_POSITION_ITEM->number.value = FOCUSER_LIMITS_MAX_POSITION_ITEM->number.target = 100000;
		FOCUSER_LIMITS_MAX_POSITION_ITEM->number.step = 1;
		FOCUSER_LIMITS_MIN_POSITION_ITEM->number.min = 0;
		FOCUSER_LIMITS_MIN_POSITION_ITEM->number.max = 100000;
		FOCUSER_LIMITS_MIN_POSITION_ITEM->number.value = FOCUSER_LIMITS_MIN_POSITION_ITEM->number.target = 0;
		FOCUSER_LIMITS_MIN_POSITION_ITEM->number.step = 1;
		//- focuser_exp.FOCUSER_LIMITS.on_attach
		FOCUSER_BACKLASH_PROPERTY->hidden = false;
		//+ focuser_exp.FOCUSER_BACKLASH.on_attach
		FOCUSER_BACKLASH_ITEM->number.min = 0;
		FOCUSER_BACKLASH_ITEM->number.max = 200;
		FOCUSER_BACKLASH_ITEM->number.step = 5;
		FOCUSER_BACKLASH_ITEM->number.value = FOCUSER_BACKLASH_ITEM->number.target = 0;
		//- focuser_exp.FOCUSER_BACKLASH.on_attach
		FOCUSER_COMPENSATION_PROPERTY->hidden = false;
		//+ focuser_exp.FOCUSER_COMPENSATION.on_attach
		FOCUSER_COMPENSATION_ITEM->number.min = -10000;
		FOCUSER_COMPENSATION_ITEM->number.max = 10000;
		//- focuser_exp.FOCUSER_COMPENSATION.on_attach
		FOCUSER_MODE_PROPERTY->hidden = false;
		FOCUSER_REVERSE_MOTION_PROPERTY->hidden = false;
		FOCUSER_POSITION_PROPERTY->hidden = false;
		//+ focuser_exp.FOCUSER_POSITION.on_attach
		FOCUSER_POSITION_ITEM->number.min = 0;
		FOCUSER_POSITION_ITEM->number.max = 100000;
		FOCUSER_POSITION_ITEM->number.step = 100;
		//- focuser_exp.FOCUSER_POSITION.on_attach
		FOCUSER_STEPS_PROPERTY->hidden = false;
		//+ focuser_exp.FOCUSER_STEPS.on_attach
		FOCUSER_STEPS_ITEM->number.min = 0;
		FOCUSER_STEPS_ITEM->number.step = 1;
		//- focuser_exp.FOCUSER_STEPS.on_attach
		FOCUSER_ABORT_MOTION_PROPERTY->hidden = false;
		X_FOCUSER_STEP_MODE_EXP_PROPERTY = indigo_init_switch_property(NULL, device->name, X_FOCUSER_STEP_MODE_EXP_PROPERTY_NAME, FOCUSER_ADVANCED_GROUP, "Step mode", INDIGO_OK_STATE, INDIGO_RW_PERM, INDIGO_ONE_OF_MANY_RULE, 2);
		if (X_FOCUSER_STEP_MODE_EXP_PROPERTY == NULL) {
			return INDIGO_FAILED;
		}
		indigo_init_switch_item(X_FOCUSER_STEP_MODE_EXP_FULL_ITEM, X_FOCUSER_STEP_MODE_EXP_FULL_ITEM_NAME, "Full step", true);
		indigo_init_switch_item(X_FOCUSER_STEP_MODE_EXP_HALF_ITEM, X_FOCUSER_STEP_MODE_EXP_HALF_ITEM_NAME, "1/2 step", false);
		X_FOCUSER_POWER_CONTROL_EXP_PROPERTY = indigo_init_number_property(NULL, device->name, X_FOCUSER_POWER_CONTROL_EXP_PROPERTY_NAME, FOCUSER_ADVANCED_GROUP, "Coils current control", INDIGO_OK_STATE, INDIGO_RW_PERM, 2);
		if (X_FOCUSER_POWER_CONTROL_EXP_PROPERTY == NULL) {
			return INDIGO_FAILED;
		}
		indigo_init_number_item(X_FOCUSER_POWER_CONTROL_EXP_MOVE_ITEM, X_FOCUSER_POWER_CONTROL_EXP_MOVE_ITEM_NAME, "Move power (%)", 0, 100, 1, 100);
		indigo_init_number_item(X_FOCUSER_POWER_CONTROL_EXP_STOP_ITEM, X_FOCUSER_POWER_CONTROL_EXP_STOP_ITEM_NAME, "Stop power (%)", 0, 100, 1, 0);
		X_FOCUSER_MOTOR_WIRING_EXP_PROPERTY = indigo_init_switch_property(NULL, device->name, X_FOCUSER_MOTOR_WIRING_EXP_PROPERTY_NAME, FOCUSER_ADVANCED_GROUP, "Motor wiring", INDIGO_OK_STATE, INDIGO_RW_PERM, INDIGO_ONE_OF_MANY_RULE, 2);
		if (X_FOCUSER_MOTOR_WIRING_EXP_PROPERTY == NULL) {
			return INDIGO_FAILED;
		}
		indigo_init_switch_item(X_FOCUSER_MOTOR_WIRING_EXP_LUNATICO_ITEM, X_FOCUSER_MOTOR_WIRING_EXP_LUNATICO_ITEM_NAME, "Lunatico", true);
		indigo_init_switch_item(X_FOCUSER_MOTOR_WIRING_EXP_MOONLITE_ITEM, X_FOCUSER_MOTOR_WIRING_EXP_MOONLITE_ITEM_NAME, "RF/Moonlite", false);
		X_FOCUSER_MOTOR_TYPE_EXP_PROPERTY = indigo_init_switch_property(NULL, device->name, X_FOCUSER_MOTOR_TYPE_EXP_PROPERTY_NAME, FOCUSER_ADVANCED_GROUP, "Motor type", INDIGO_OK_STATE, INDIGO_RW_PERM, INDIGO_ONE_OF_MANY_RULE, 4);
		if (X_FOCUSER_MOTOR_TYPE_EXP_PROPERTY == NULL) {
			return INDIGO_FAILED;
		}
		indigo_init_switch_item(X_FOCUSER_MOTOR_TYPE_EXP_UNIPOLAR_ITEM, X_FOCUSER_MOTOR_TYPE_EXP_UNIPOLAR_ITEM_NAME, "Unipolar", true);
		indigo_init_switch_item(X_FOCUSER_MOTOR_TYPE_EXP_BIPOLAR_ITEM, X_FOCUSER_MOTOR_TYPE_EXP_BIPOLAR_ITEM_NAME, "Bipolar", false);
		indigo_init_switch_item(X_FOCUSER_MOTOR_TYPE_EXP_DC_ITEM, X_FOCUSER_MOTOR_TYPE_EXP_DC_ITEM_NAME, "DC", false);
		indigo_init_switch_item(X_FOCUSER_MOTOR_TYPE_EXP_STEP_DIR_ITEM, X_FOCUSER_MOTOR_TYPE_EXP_STEP_DIR_ITEM_NAME, "Step-dir", false);
		X_FOCUSER_TEMPERATURE_SENSOR_EXP_PROPERTY = indigo_init_switch_property(NULL, device->name, X_FOCUSER_TEMPERATURE_SENSOR_EXP_PROPERTY_NAME, FOCUSER_ADVANCED_GROUP, "Temperature sensor in use", INDIGO_OK_STATE, INDIGO_RW_PERM, INDIGO_ONE_OF_MANY_RULE, 2);
		if (X_FOCUSER_TEMPERATURE_SENSOR_EXP_PROPERTY == NULL) {
			return INDIGO_FAILED;
		}
		indigo_init_switch_item(X_FOCUSER_TEMPERATURE_SENSOR_EXP_INTERNAL_ITEM, X_FOCUSER_TEMPERATURE_SENSOR_EXP_INTERNAL_ITEM_NAME, "Internal sensor", true);
		indigo_init_switch_item(X_FOCUSER_TEMPERATURE_SENSOR_EXP_EXTERNAL_ITEM, X_FOCUSER_TEMPERATURE_SENSOR_EXP_EXTERNAL_ITEM_NAME, "External sensor", false);
		INDIGO_DEVICE_ATTACH_LOG(DRIVER_NAME, device->name);
		return focuser_exp_enumerate_properties(device, NULL, NULL);
	}
	return INDIGO_FAILED;
}

static indigo_result focuser_exp_enumerate_properties(indigo_device *device, indigo_client *client, indigo_property *property) {
	if (IS_CONNECTED) {
		INDIGO_DEFINE_MATCHING_PROPERTY(X_FOCUSER_STEP_MODE_EXP_PROPERTY);
		INDIGO_DEFINE_MATCHING_PROPERTY(X_FOCUSER_POWER_CONTROL_EXP_PROPERTY);
		INDIGO_DEFINE_MATCHING_PROPERTY(X_FOCUSER_MOTOR_WIRING_EXP_PROPERTY);
		INDIGO_DEFINE_MATCHING_PROPERTY(X_FOCUSER_MOTOR_TYPE_EXP_PROPERTY);
		INDIGO_DEFINE_MATCHING_PROPERTY(X_FOCUSER_TEMPERATURE_SENSOR_EXP_PROPERTY);
	}
	return indigo_focuser_enumerate_properties(device, client, property);
}

static indigo_result focuser_exp_change_property(indigo_device *device, indigo_client *client, indigo_property *property) {
	if (indigo_property_match_changeable(CONNECTION_PROPERTY, property)) {
		INDIGO_PROCESS_CONNECT(focuser_exp_connection_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(FOCUSER_SPEED_PROPERTY, property)) {
		INDIGO_COPY_VALUES_PROCESS_CHANGE(FOCUSER_SPEED_PROPERTY, focuser_exp_focuser_speed_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(FOCUSER_LIMITS_PROPERTY, property)) {
		INDIGO_COPY_VALUES_PROCESS_CHANGE(FOCUSER_LIMITS_PROPERTY, focuser_exp_focuser_limits_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(FOCUSER_BACKLASH_PROPERTY, property)) {
		indigo_property_copy_values(FOCUSER_BACKLASH_PROPERTY, property, false);
		FOCUSER_BACKLASH_PROPERTY->state = INDIGO_OK_STATE;
		indigo_update_property(device, FOCUSER_BACKLASH_PROPERTY, NULL);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(FOCUSER_COMPENSATION_PROPERTY, property)) {
		indigo_property_copy_values(FOCUSER_COMPENSATION_PROPERTY, property, false);
		FOCUSER_COMPENSATION_PROPERTY->state = INDIGO_OK_STATE;
		indigo_update_property(device, FOCUSER_COMPENSATION_PROPERTY, NULL);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(FOCUSER_MODE_PROPERTY, property)) {
		INDIGO_COPY_VALUES_PROCESS_CHANGE(FOCUSER_MODE_PROPERTY, focuser_exp_focuser_mode_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(FOCUSER_REVERSE_MOTION_PROPERTY, property)) {
		INDIGO_COPY_VALUES_PROCESS_CHANGE(FOCUSER_REVERSE_MOTION_PROPERTY, focuser_exp_focuser_reverse_motion_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(FOCUSER_POSITION_PROPERTY, property)) {
		INDIGO_COPY_VALUES_PROCESS_CHANGE(FOCUSER_POSITION_PROPERTY, focuser_exp_focuser_position_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(FOCUSER_STEPS_PROPERTY, property)) {
		INDIGO_COPY_VALUES_PROCESS_CHANGE(FOCUSER_STEPS_PROPERTY, focuser_exp_focuser_steps_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(FOCUSER_ABORT_MOTION_PROPERTY, property)) {
		INDIGO_COPY_VALUES_PROCESS_URGENT_CHANGE(FOCUSER_ABORT_MOTION_PROPERTY, focuser_exp_focuser_abort_motion_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(X_FOCUSER_STEP_MODE_EXP_PROPERTY, property)) {
		INDIGO_COPY_VALUES_PROCESS_CHANGE(X_FOCUSER_STEP_MODE_EXP_PROPERTY, focuser_exp_x_focuser_step_mode_exp_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(X_FOCUSER_POWER_CONTROL_EXP_PROPERTY, property)) {
		INDIGO_COPY_VALUES_PROCESS_CHANGE(X_FOCUSER_POWER_CONTROL_EXP_PROPERTY, focuser_exp_x_focuser_power_control_exp_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(X_FOCUSER_MOTOR_WIRING_EXP_PROPERTY, property)) {
		INDIGO_COPY_VALUES_PROCESS_CHANGE(X_FOCUSER_MOTOR_WIRING_EXP_PROPERTY, focuser_exp_x_focuser_motor_wiring_exp_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(X_FOCUSER_MOTOR_TYPE_EXP_PROPERTY, property)) {
		INDIGO_COPY_VALUES_PROCESS_CHANGE(X_FOCUSER_MOTOR_TYPE_EXP_PROPERTY, focuser_exp_x_focuser_motor_type_exp_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(X_FOCUSER_TEMPERATURE_SENSOR_EXP_PROPERTY, property)) {
		INDIGO_COPY_VALUES_PROCESS_CHANGE(X_FOCUSER_TEMPERATURE_SENSOR_EXP_PROPERTY, focuser_exp_x_focuser_temperature_sensor_exp_handler);
		return INDIGO_OK;
	} else if (indigo_property_match(CONFIG_PROPERTY, property)) {
		if (indigo_switch_match(CONFIG_SAVE_ITEM, property)) {
			indigo_save_property(device, NULL, FOCUSER_SPEED_PROPERTY);
			indigo_save_property(device, NULL, FOCUSER_LIMITS_PROPERTY);
			indigo_save_property(device, NULL, FOCUSER_BACKLASH_PROPERTY);
			indigo_save_property(device, NULL, FOCUSER_COMPENSATION_PROPERTY);
			indigo_save_property(device, NULL, FOCUSER_REVERSE_MOTION_PROPERTY);
			indigo_save_property(device, NULL, X_FOCUSER_STEP_MODE_EXP_PROPERTY);
			indigo_save_property(device, NULL, X_FOCUSER_POWER_CONTROL_EXP_PROPERTY);
			indigo_save_property(device, NULL, X_FOCUSER_MOTOR_WIRING_EXP_PROPERTY);
			indigo_save_property(device, NULL, X_FOCUSER_MOTOR_TYPE_EXP_PROPERTY);
			indigo_save_property(device, NULL, X_FOCUSER_TEMPERATURE_SENSOR_EXP_PROPERTY);
		}
	}
	return indigo_focuser_change_property(device, client, property);
}

static indigo_result focuser_exp_detach(indigo_device *device) {
	if (IS_CONNECTED) {
		indigo_set_switch(CONNECTION_PROPERTY, CONNECTION_DISCONNECTED_ITEM, true);
		focuser_exp_connection_handler(device);
	}
	indigo_release_property(X_FOCUSER_STEP_MODE_EXP_PROPERTY);
	indigo_release_property(X_FOCUSER_POWER_CONTROL_EXP_PROPERTY);
	indigo_release_property(X_FOCUSER_MOTOR_WIRING_EXP_PROPERTY);
	indigo_release_property(X_FOCUSER_MOTOR_TYPE_EXP_PROPERTY);
	indigo_release_property(X_FOCUSER_TEMPERATURE_SENSOR_EXP_PROPERTY);
	INDIGO_DEVICE_DETACH_LOG(DRIVER_NAME, device->name);
	return indigo_focuser_detach(device);
}

#pragma mark - High level code (rotator_exp)
// device_id: rotator_exp type: rotator

static void rotator_exp_connection_handler(indigo_device *device) {
	if (CONNECTION_CONNECTED_ITEM->sw.value) {
		bool connection_result = true;
		if (PRIVATE_DATA->count == 0) {
			connection_result = lunatico_open(device->master_device);
		}
		if (connection_result) {
			PRIVATE_DATA->count++;
		}
		if (connection_result) {
			//+ rotator_exp.on_connect
			connection_result = lunatico_rotator_connect(device, X_ROTATOR_POWER_CONTROL_EXP_PROPERTY, X_ROTATOR_MOTOR_TYPE_EXP_PROPERTY, X_ROTATOR_STEP_MODE_EXP_PROPERTY, X_ROTATOR_MOTOR_WIRING_EXP_PROPERTY);
			//- rotator_exp.on_connect
		}
		if (connection_result) {
			indigo_define_property(device, X_ROTATOR_STEP_MODE_EXP_PROPERTY, NULL);
			indigo_define_property(device, X_ROTATOR_POWER_CONTROL_EXP_PROPERTY, NULL);
			indigo_define_property(device, X_ROTATOR_MOTOR_WIRING_EXP_PROPERTY, NULL);
			indigo_define_property(device, X_ROTATOR_MOTOR_TYPE_EXP_PROPERTY, NULL);
			CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
			indigo_send_message(device, OK_PROPERTY, "Connected to %s on %s", ROTATOR_EXP_DEVICE_NAME, DEVICE_PORT_ITEM->text.value);
		} else {
			indigo_send_message(device, ALERT_PROPERTY, "Failed to connect to %s on %s", ROTATOR_EXP_DEVICE_NAME, DEVICE_PORT_ITEM->text.value);
			if (PRIVATE_DATA->count > 0 && --PRIVATE_DATA->count == 0) {
				lunatico_close(device);
			}
			CONNECTION_PROPERTY->state = INDIGO_ALERT_STATE;
			indigo_set_switch(CONNECTION_PROPERTY, CONNECTION_DISCONNECTED_ITEM, true);
		}
	} else {
		indigo_cancel_pending_handlers(device);
		//+ rotator_exp.on_disconnect
		lunatico_release_port(device);
		//- rotator_exp.on_disconnect
		indigo_delete_property(device, X_ROTATOR_STEP_MODE_EXP_PROPERTY, NULL);
		indigo_delete_property(device, X_ROTATOR_POWER_CONTROL_EXP_PROPERTY, NULL);
		indigo_delete_property(device, X_ROTATOR_MOTOR_WIRING_EXP_PROPERTY, NULL);
		indigo_delete_property(device, X_ROTATOR_MOTOR_TYPE_EXP_PROPERTY, NULL);
		if (--PRIVATE_DATA->count == 0) {
			lunatico_close(device);
		}
		indigo_send_message(device, OK_PROPERTY, "Disconnected from %s", device->name);
		CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
	}
	indigo_rotator_change_property(device, NULL, CONNECTION_PROPERTY);
}

static void rotator_exp_rotator_steps_per_revolution_handler(indigo_device *device) {
	ROTATOR_STEPS_PER_REVOLUTION_PROPERTY->state = INDIGO_OK_STATE;
	//+ rotator_exp.ROTATOR_STEPS_PER_REVOLUTION.on_change
	lunatico_rotator_resync(device);
	//- rotator_exp.ROTATOR_STEPS_PER_REVOLUTION.on_change
	indigo_update_property(device, ROTATOR_STEPS_PER_REVOLUTION_PROPERTY, NULL);
}

static void rotator_exp_rotator_direction_handler(indigo_device *device) {
	ROTATOR_DIRECTION_PROPERTY->state = INDIGO_OK_STATE;
	//+ rotator_exp.ROTATOR_DIRECTION.on_change
	if (!lunatico_apply_wiring(device, X_ROTATOR_MOTOR_WIRING_EXP_PROPERTY, !ROTATOR_DIRECTION_NORMAL_ITEM->sw.value)) {
		ROTATOR_DIRECTION_PROPERTY->state = INDIGO_ALERT_STATE;
	}
	//- rotator_exp.ROTATOR_DIRECTION.on_change
	indigo_update_property(device, ROTATOR_DIRECTION_PROPERTY, NULL);
}

static void rotator_exp_rotator_limits_handler(indigo_device *device) {
	ROTATOR_LIMITS_PROPERTY->state = INDIGO_OK_STATE;
	//+ rotator_exp.ROTATOR_LIMITS.on_change
	lunatico_rotator_limits(device);
	//- rotator_exp.ROTATOR_LIMITS.on_change
	indigo_update_property(device, ROTATOR_LIMITS_PROPERTY, NULL);
}

static void rotator_exp_rotator_position_handler(indigo_device *device) {
	//+ rotator_exp.ROTATOR_POSITION.on_change
	if (lunatico_rotator_position(device)) {
		indigo_execute_handler_in(device, LUNATICO_MOTION_POLL, rotator_motion_finalizer);
	}
	//- rotator_exp.ROTATOR_POSITION.on_change
}

static void rotator_exp_rotator_abort_motion_handler(indigo_device *device) {
	//+ rotator_exp.ROTATOR_ABORT_MOTION.on_change
	// An urgent abort can overtake a queued move, so the start
	// handler of this device is cancelled together with the
	// completion poll it would have scheduled.
	indigo_cancel_pending_handler(device, rotator_exp_rotator_position_handler);
	indigo_cancel_pending_handler(device, rotator_motion_finalizer);
	lunatico_rotator_abort(device);
	//- rotator_exp.ROTATOR_ABORT_MOTION.on_change
}

static void rotator_exp_x_rotator_step_mode_exp_handler(indigo_device *device) {
	X_ROTATOR_STEP_MODE_EXP_PROPERTY->state = INDIGO_OK_STATE;
	//+ rotator_exp.X_ROTATOR_STEP_MODE_EXP.on_change
	if (!lunatico_apply_step_mode(device, X_ROTATOR_STEP_MODE_EXP_PROPERTY)) {
		X_ROTATOR_STEP_MODE_EXP_PROPERTY->state = INDIGO_ALERT_STATE;
	}
	//- rotator_exp.X_ROTATOR_STEP_MODE_EXP.on_change
	indigo_update_property(device, X_ROTATOR_STEP_MODE_EXP_PROPERTY, NULL);
}

static void rotator_exp_x_rotator_power_control_exp_handler(indigo_device *device) {
	X_ROTATOR_POWER_CONTROL_EXP_PROPERTY->state = INDIGO_OK_STATE;
	//+ rotator_exp.X_ROTATOR_POWER_CONTROL_EXP.on_change
	if (!lunatico_apply_power_control(device, X_ROTATOR_POWER_CONTROL_EXP_PROPERTY)) {
		X_ROTATOR_POWER_CONTROL_EXP_PROPERTY->state = INDIGO_ALERT_STATE;
	}
	//- rotator_exp.X_ROTATOR_POWER_CONTROL_EXP.on_change
	indigo_update_property(device, X_ROTATOR_POWER_CONTROL_EXP_PROPERTY, NULL);
}

static void rotator_exp_x_rotator_motor_wiring_exp_handler(indigo_device *device) {
	X_ROTATOR_MOTOR_WIRING_EXP_PROPERTY->state = INDIGO_OK_STATE;
	//+ rotator_exp.X_ROTATOR_MOTOR_WIRING_EXP.on_change
	if (!lunatico_apply_wiring(device, X_ROTATOR_MOTOR_WIRING_EXP_PROPERTY, !ROTATOR_DIRECTION_NORMAL_ITEM->sw.value)) {
		X_ROTATOR_MOTOR_WIRING_EXP_PROPERTY->state = INDIGO_ALERT_STATE;
	}
	//- rotator_exp.X_ROTATOR_MOTOR_WIRING_EXP.on_change
	indigo_update_property(device, X_ROTATOR_MOTOR_WIRING_EXP_PROPERTY, NULL);
}

static void rotator_exp_x_rotator_motor_type_exp_handler(indigo_device *device) {
	X_ROTATOR_MOTOR_TYPE_EXP_PROPERTY->state = INDIGO_OK_STATE;
	//+ rotator_exp.X_ROTATOR_MOTOR_TYPE_EXP.on_change
	if (!lunatico_apply_motor_type(device, X_ROTATOR_MOTOR_TYPE_EXP_PROPERTY)) {
		X_ROTATOR_MOTOR_TYPE_EXP_PROPERTY->state = INDIGO_ALERT_STATE;
	}
	//- rotator_exp.X_ROTATOR_MOTOR_TYPE_EXP.on_change
	indigo_update_property(device, X_ROTATOR_MOTOR_TYPE_EXP_PROPERTY, NULL);
}

#pragma mark - Device API (rotator_exp)

static indigo_result rotator_exp_enumerate_properties(indigo_device *device, indigo_client *client, indigo_property *property);

static indigo_result rotator_exp_attach(indigo_device *device) {
	if (indigo_rotator_attach(device, DRIVER_NAME, DRIVER_VERSION) == INDIGO_OK) {
		//+ rotator_exp.on_attach
		device->gp_bits = 1;
		INFO_PROPERTY->count = 6;
		//- rotator_exp.on_attach
		ROTATOR_STEPS_PER_REVOLUTION_PROPERTY->hidden = false;
		//+ rotator_exp.ROTATOR_STEPS_PER_REVOLUTION.on_attach
		ROTATOR_STEPS_PER_REVOLUTION_ITEM->number.min = 100;
		ROTATOR_STEPS_PER_REVOLUTION_ITEM->number.max = 100000;
		ROTATOR_STEPS_PER_REVOLUTION_ITEM->number.value = ROTATOR_STEPS_PER_REVOLUTION_ITEM->number.target = 3600;
		//- rotator_exp.ROTATOR_STEPS_PER_REVOLUTION.on_attach
		ROTATOR_DIRECTION_PROPERTY->hidden = false;
		ROTATOR_BACKLASH_PROPERTY->hidden = false;
		ROTATOR_LIMITS_PROPERTY->hidden = false;
		//+ rotator_exp.ROTATOR_LIMITS.on_attach
		ROTATOR_LIMITS_MIN_POSITION_ITEM->number.min = -180;
		ROTATOR_LIMITS_MIN_POSITION_ITEM->number.max = 360;
		ROTATOR_LIMITS_MIN_POSITION_ITEM->number.value = ROTATOR_LIMITS_MIN_POSITION_ITEM->number.target = -180;
		ROTATOR_LIMITS_MAX_POSITION_ITEM->number.min = -180;
		ROTATOR_LIMITS_MAX_POSITION_ITEM->number.max = 360;
		ROTATOR_LIMITS_MAX_POSITION_ITEM->number.value = ROTATOR_LIMITS_MAX_POSITION_ITEM->number.target = 180;
		//- rotator_exp.ROTATOR_LIMITS.on_attach
		ROTATOR_POSITION_PROPERTY->hidden = false;
		ROTATOR_ABORT_MOTION_PROPERTY->hidden = false;
		X_ROTATOR_STEP_MODE_EXP_PROPERTY = indigo_init_switch_property(NULL, device->name, X_ROTATOR_STEP_MODE_EXP_PROPERTY_NAME, ROTATOR_ADVANCED_GROUP, "Step mode", INDIGO_OK_STATE, INDIGO_RW_PERM, INDIGO_ONE_OF_MANY_RULE, 2);
		if (X_ROTATOR_STEP_MODE_EXP_PROPERTY == NULL) {
			return INDIGO_FAILED;
		}
		indigo_init_switch_item(X_ROTATOR_STEP_MODE_EXP_FULL_ITEM, X_ROTATOR_STEP_MODE_EXP_FULL_ITEM_NAME, "Full step", true);
		indigo_init_switch_item(X_ROTATOR_STEP_MODE_EXP_HALF_ITEM, X_ROTATOR_STEP_MODE_EXP_HALF_ITEM_NAME, "1/2 step", false);
		X_ROTATOR_POWER_CONTROL_EXP_PROPERTY = indigo_init_number_property(NULL, device->name, X_ROTATOR_POWER_CONTROL_EXP_PROPERTY_NAME, ROTATOR_ADVANCED_GROUP, "Coils current control", INDIGO_OK_STATE, INDIGO_RW_PERM, 2);
		if (X_ROTATOR_POWER_CONTROL_EXP_PROPERTY == NULL) {
			return INDIGO_FAILED;
		}
		indigo_init_number_item(X_ROTATOR_POWER_CONTROL_EXP_MOVE_ITEM, X_ROTATOR_POWER_CONTROL_EXP_MOVE_ITEM_NAME, "Move power (%)", 0, 100, 1, 100);
		indigo_init_number_item(X_ROTATOR_POWER_CONTROL_EXP_STOP_ITEM, X_ROTATOR_POWER_CONTROL_EXP_STOP_ITEM_NAME, "Stop power (%)", 0, 100, 1, 0);
		X_ROTATOR_MOTOR_WIRING_EXP_PROPERTY = indigo_init_switch_property(NULL, device->name, X_ROTATOR_MOTOR_WIRING_EXP_PROPERTY_NAME, ROTATOR_ADVANCED_GROUP, "Motor wiring", INDIGO_OK_STATE, INDIGO_RW_PERM, INDIGO_ONE_OF_MANY_RULE, 2);
		if (X_ROTATOR_MOTOR_WIRING_EXP_PROPERTY == NULL) {
			return INDIGO_FAILED;
		}
		indigo_init_switch_item(X_ROTATOR_MOTOR_WIRING_EXP_LUNATICO_ITEM, X_ROTATOR_MOTOR_WIRING_EXP_LUNATICO_ITEM_NAME, "Lunatico", true);
		indigo_init_switch_item(X_ROTATOR_MOTOR_WIRING_EXP_MOONLITE_ITEM, X_ROTATOR_MOTOR_WIRING_EXP_MOONLITE_ITEM_NAME, "RF/Moonlite", false);
		X_ROTATOR_MOTOR_TYPE_EXP_PROPERTY = indigo_init_switch_property(NULL, device->name, X_ROTATOR_MOTOR_TYPE_EXP_PROPERTY_NAME, ROTATOR_ADVANCED_GROUP, "Motor type", INDIGO_OK_STATE, INDIGO_RW_PERM, INDIGO_ONE_OF_MANY_RULE, 4);
		if (X_ROTATOR_MOTOR_TYPE_EXP_PROPERTY == NULL) {
			return INDIGO_FAILED;
		}
		indigo_init_switch_item(X_ROTATOR_MOTOR_TYPE_EXP_UNIPOLAR_ITEM, X_ROTATOR_MOTOR_TYPE_EXP_UNIPOLAR_ITEM_NAME, "Unipolar", true);
		indigo_init_switch_item(X_ROTATOR_MOTOR_TYPE_EXP_BIPOLAR_ITEM, X_ROTATOR_MOTOR_TYPE_EXP_BIPOLAR_ITEM_NAME, "Bipolar", false);
		indigo_init_switch_item(X_ROTATOR_MOTOR_TYPE_EXP_DC_ITEM, X_ROTATOR_MOTOR_TYPE_EXP_DC_ITEM_NAME, "DC", false);
		indigo_init_switch_item(X_ROTATOR_MOTOR_TYPE_EXP_STEP_DIR_ITEM, X_ROTATOR_MOTOR_TYPE_EXP_STEP_DIR_ITEM_NAME, "Step-dir", false);
		INDIGO_DEVICE_ATTACH_LOG(DRIVER_NAME, device->name);
		return rotator_exp_enumerate_properties(device, NULL, NULL);
	}
	return INDIGO_FAILED;
}

static indigo_result rotator_exp_enumerate_properties(indigo_device *device, indigo_client *client, indigo_property *property) {
	if (IS_CONNECTED) {
		INDIGO_DEFINE_MATCHING_PROPERTY(X_ROTATOR_STEP_MODE_EXP_PROPERTY);
		INDIGO_DEFINE_MATCHING_PROPERTY(X_ROTATOR_POWER_CONTROL_EXP_PROPERTY);
		INDIGO_DEFINE_MATCHING_PROPERTY(X_ROTATOR_MOTOR_WIRING_EXP_PROPERTY);
		INDIGO_DEFINE_MATCHING_PROPERTY(X_ROTATOR_MOTOR_TYPE_EXP_PROPERTY);
	}
	return indigo_rotator_enumerate_properties(device, client, property);
}

static indigo_result rotator_exp_change_property(indigo_device *device, indigo_client *client, indigo_property *property) {
	if (indigo_property_match_changeable(CONNECTION_PROPERTY, property)) {
		INDIGO_PROCESS_CONNECT(rotator_exp_connection_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(ROTATOR_STEPS_PER_REVOLUTION_PROPERTY, property)) {
		INDIGO_COPY_VALUES_PROCESS_CHANGE(ROTATOR_STEPS_PER_REVOLUTION_PROPERTY, rotator_exp_rotator_steps_per_revolution_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(ROTATOR_DIRECTION_PROPERTY, property)) {
		INDIGO_COPY_VALUES_PROCESS_CHANGE(ROTATOR_DIRECTION_PROPERTY, rotator_exp_rotator_direction_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(ROTATOR_BACKLASH_PROPERTY, property)) {
		indigo_property_copy_values(ROTATOR_BACKLASH_PROPERTY, property, false);
		ROTATOR_BACKLASH_PROPERTY->state = INDIGO_OK_STATE;
		indigo_update_property(device, ROTATOR_BACKLASH_PROPERTY, NULL);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(ROTATOR_LIMITS_PROPERTY, property)) {
		INDIGO_COPY_VALUES_PROCESS_CHANGE(ROTATOR_LIMITS_PROPERTY, rotator_exp_rotator_limits_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(ROTATOR_POSITION_PROPERTY, property)) {
		INDIGO_COPY_VALUES_PROCESS_CHANGE(ROTATOR_POSITION_PROPERTY, rotator_exp_rotator_position_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(ROTATOR_ABORT_MOTION_PROPERTY, property)) {
		INDIGO_COPY_VALUES_PROCESS_URGENT_CHANGE(ROTATOR_ABORT_MOTION_PROPERTY, rotator_exp_rotator_abort_motion_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(X_ROTATOR_STEP_MODE_EXP_PROPERTY, property)) {
		INDIGO_COPY_VALUES_PROCESS_CHANGE(X_ROTATOR_STEP_MODE_EXP_PROPERTY, rotator_exp_x_rotator_step_mode_exp_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(X_ROTATOR_POWER_CONTROL_EXP_PROPERTY, property)) {
		INDIGO_COPY_VALUES_PROCESS_CHANGE(X_ROTATOR_POWER_CONTROL_EXP_PROPERTY, rotator_exp_x_rotator_power_control_exp_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(X_ROTATOR_MOTOR_WIRING_EXP_PROPERTY, property)) {
		INDIGO_COPY_VALUES_PROCESS_CHANGE(X_ROTATOR_MOTOR_WIRING_EXP_PROPERTY, rotator_exp_x_rotator_motor_wiring_exp_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(X_ROTATOR_MOTOR_TYPE_EXP_PROPERTY, property)) {
		INDIGO_COPY_VALUES_PROCESS_CHANGE(X_ROTATOR_MOTOR_TYPE_EXP_PROPERTY, rotator_exp_x_rotator_motor_type_exp_handler);
		return INDIGO_OK;
	} else if (indigo_property_match(CONFIG_PROPERTY, property)) {
		if (indigo_switch_match(CONFIG_SAVE_ITEM, property)) {
			indigo_save_property(device, NULL, ROTATOR_STEPS_PER_REVOLUTION_PROPERTY);
			indigo_save_property(device, NULL, ROTATOR_DIRECTION_PROPERTY);
			indigo_save_property(device, NULL, ROTATOR_BACKLASH_PROPERTY);
			indigo_save_property(device, NULL, ROTATOR_LIMITS_PROPERTY);
			indigo_save_property(device, NULL, X_ROTATOR_STEP_MODE_EXP_PROPERTY);
			indigo_save_property(device, NULL, X_ROTATOR_POWER_CONTROL_EXP_PROPERTY);
			indigo_save_property(device, NULL, X_ROTATOR_MOTOR_WIRING_EXP_PROPERTY);
			indigo_save_property(device, NULL, X_ROTATOR_MOTOR_TYPE_EXP_PROPERTY);
		}
	}
	return indigo_rotator_change_property(device, client, property);
}

static indigo_result rotator_exp_detach(indigo_device *device) {
	if (IS_CONNECTED) {
		indigo_set_switch(CONNECTION_PROPERTY, CONNECTION_DISCONNECTED_ITEM, true);
		rotator_exp_connection_handler(device);
	}
	indigo_release_property(X_ROTATOR_STEP_MODE_EXP_PROPERTY);
	indigo_release_property(X_ROTATOR_POWER_CONTROL_EXP_PROPERTY);
	indigo_release_property(X_ROTATOR_MOTOR_WIRING_EXP_PROPERTY);
	indigo_release_property(X_ROTATOR_MOTOR_TYPE_EXP_PROPERTY);
	INDIGO_DEVICE_DETACH_LOG(DRIVER_NAME, device->name);
	return indigo_rotator_detach(device);
}

#pragma mark - High level code (aux_exp)
// device_id: aux_exp type: aux

static void aux_exp_timer_callback(indigo_device *device) {
	if (!IS_CONNECTED) {
		return;
	}
	//+ aux_exp.on_timer
	lunatico_poll_sensors(device, AUX_GPIO_SENSORS_EXP_PROPERTY);
	indigo_execute_handler_in(device, LUNATICO_SENSOR_POLL, aux_exp_timer_callback);
	//- aux_exp.on_timer
}

static void aux_exp_connection_handler(indigo_device *device) {
	if (CONNECTION_CONNECTED_ITEM->sw.value) {
		bool connection_result = true;
		if (PRIVATE_DATA->count == 0) {
			connection_result = lunatico_open(device->master_device);
		}
		if (connection_result) {
			PRIVATE_DATA->count++;
		}
		if (connection_result) {
			//+ aux_exp.on_connect
			connection_result = lunatico_aux_connect(device, AUX_POWER_OUTLET_EXP_PROPERTY);
			//- aux_exp.on_connect
		}
		if (connection_result) {
			indigo_define_property(device, AUX_POWER_OUTLET_EXP_PROPERTY, NULL);
			indigo_define_property(device, AUX_GPIO_SENSORS_EXP_PROPERTY, NULL);
			CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
			indigo_send_message(device, OK_PROPERTY, "Connected to %s on %s", AUX_EXP_DEVICE_NAME, DEVICE_PORT_ITEM->text.value);
		} else {
			indigo_send_message(device, ALERT_PROPERTY, "Failed to connect to %s on %s", AUX_EXP_DEVICE_NAME, DEVICE_PORT_ITEM->text.value);
			if (PRIVATE_DATA->count > 0 && --PRIVATE_DATA->count == 0) {
				lunatico_close(device);
			}
			CONNECTION_PROPERTY->state = INDIGO_ALERT_STATE;
			indigo_set_switch(CONNECTION_PROPERTY, CONNECTION_DISCONNECTED_ITEM, true);
		}
	} else {
		indigo_cancel_pending_handlers(device);
		//+ aux_exp.on_disconnect
		lunatico_release_port(device);
		//- aux_exp.on_disconnect
		indigo_delete_property(device, AUX_POWER_OUTLET_EXP_PROPERTY, NULL);
		indigo_delete_property(device, AUX_GPIO_SENSORS_EXP_PROPERTY, NULL);
		if (--PRIVATE_DATA->count == 0) {
			lunatico_close(device);
		}
		indigo_send_message(device, OK_PROPERTY, "Disconnected from %s", device->name);
		CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
	}
	indigo_aux_change_property(device, NULL, CONNECTION_PROPERTY);
	if (IS_CONNECTED) {
		indigo_execute_handler(device, aux_exp_timer_callback);
	}
}

static void aux_exp_aux_outlet_names_exp_handler(indigo_device *device) {
	AUX_OUTLET_NAMES_EXP_PROPERTY->state = INDIGO_OK_STATE;
	//+ aux_exp.AUX_OUTLET_NAMES_EXP.on_change
	lunatico_apply_names(device, AUX_OUTLET_NAMES_EXP_PROPERTY, AUX_POWER_OUTLET_EXP_PROPERTY);
	//- aux_exp.AUX_OUTLET_NAMES_EXP.on_change
	indigo_update_property(device, AUX_OUTLET_NAMES_EXP_PROPERTY, NULL);
}

static void aux_exp_aux_power_outlet_exp_handler(indigo_device *device) {
	AUX_POWER_OUTLET_EXP_PROPERTY->state = INDIGO_OK_STATE;
	//+ aux_exp.AUX_POWER_OUTLET_EXP.on_change
	if (!lunatico_apply_outlets(device, AUX_POWER_OUTLET_EXP_PROPERTY)) {
		AUX_POWER_OUTLET_EXP_PROPERTY->state = INDIGO_ALERT_STATE;
	}
	//- aux_exp.AUX_POWER_OUTLET_EXP.on_change
	indigo_update_property(device, AUX_POWER_OUTLET_EXP_PROPERTY, NULL);
}

static void aux_exp_aux_sensor_names_exp_handler(indigo_device *device) {
	AUX_SENSOR_NAMES_EXP_PROPERTY->state = INDIGO_OK_STATE;
	//+ aux_exp.AUX_SENSOR_NAMES_EXP.on_change
	lunatico_apply_names(device, AUX_SENSOR_NAMES_EXP_PROPERTY, AUX_GPIO_SENSORS_EXP_PROPERTY);
	//- aux_exp.AUX_SENSOR_NAMES_EXP.on_change
	indigo_update_property(device, AUX_SENSOR_NAMES_EXP_PROPERTY, NULL);
}

#pragma mark - Device API (aux_exp)

static indigo_result aux_exp_enumerate_properties(indigo_device *device, indigo_client *client, indigo_property *property);

static indigo_result aux_exp_attach(indigo_device *device) {
	if (indigo_aux_attach(device, DRIVER_NAME, DRIVER_VERSION, INDIGO_INTERFACE_AUX_GPIO | INDIGO_INTERFACE_AUX_POWERBOX) == INDIGO_OK) {
		//+ aux_exp.on_attach
		device->gp_bits = 1;
		INFO_PROPERTY->count = 6;
		//- aux_exp.on_attach
		AUX_OUTLET_NAMES_EXP_PROPERTY = indigo_init_text_property(NULL, device->name, AUX_OUTLET_NAMES_PROPERTY_NAME, AUX_POWERBOX_GROUP, "Power outlet names", INDIGO_OK_STATE, INDIGO_RW_PERM, 4);
		if (AUX_OUTLET_NAMES_EXP_PROPERTY == NULL) {
			return INDIGO_FAILED;
		}
		indigo_init_text_item(AUX_OUTLET_NAME_EXP_1_ITEM, AUX_POWER_OUTLET_NAME_1_ITEM_NAME, "DB9 Pin 1", "Power #1");
		indigo_init_text_item(AUX_OUTLET_NAME_EXP_2_ITEM, AUX_POWER_OUTLET_NAME_2_ITEM_NAME, "DB9 Pin 2", "Power #2");
		indigo_init_text_item(AUX_OUTLET_NAME_EXP_3_ITEM, AUX_POWER_OUTLET_NAME_3_ITEM_NAME, "DB9 Pin 3", "Power #3");
		indigo_init_text_item(AUX_OUTLET_NAME_EXP_4_ITEM, AUX_POWER_OUTLET_NAME_4_ITEM_NAME, "DB9 Pin 4", "Power #4");
		AUX_POWER_OUTLET_EXP_PROPERTY = indigo_init_switch_property(NULL, device->name, AUX_POWER_OUTLET_PROPERTY_NAME, AUX_POWERBOX_GROUP, "Power outlets", INDIGO_OK_STATE, INDIGO_RW_PERM, INDIGO_ANY_OF_MANY_RULE, 4);
		if (AUX_POWER_OUTLET_EXP_PROPERTY == NULL) {
			return INDIGO_FAILED;
		}
		indigo_init_switch_item(AUX_POWER_OUTLET_EXP_1_ITEM, AUX_POWER_OUTLET_1_ITEM_NAME, "Power #1", false);
		indigo_init_switch_item(AUX_POWER_OUTLET_EXP_2_ITEM, AUX_POWER_OUTLET_2_ITEM_NAME, "Power #2", false);
		indigo_init_switch_item(AUX_POWER_OUTLET_EXP_3_ITEM, AUX_POWER_OUTLET_3_ITEM_NAME, "Power #3", false);
		indigo_init_switch_item(AUX_POWER_OUTLET_EXP_4_ITEM, AUX_POWER_OUTLET_4_ITEM_NAME, "Power #4", false);
		AUX_SENSOR_NAMES_EXP_PROPERTY = indigo_init_text_property(NULL, device->name, AUX_SENSOR_NAMES_PROPERTY_NAME, AUX_SENSORS_GROUP, "Sensor names", INDIGO_OK_STATE, INDIGO_RW_PERM, 4);
		if (AUX_SENSOR_NAMES_EXP_PROPERTY == NULL) {
			return INDIGO_FAILED;
		}
		indigo_init_text_item(AUX_SENSOR_NAME_EXP_1_ITEM, AUX_GPIO_SENSOR_NAME_1_ITEM_NAME, "DB9 Pin 6", "Sensor #1");
		indigo_init_text_item(AUX_SENSOR_NAME_EXP_2_ITEM, AUX_GPIO_SENSOR_NAME_2_ITEM_NAME, "DB9 Pin 7", "Sensor #2");
		indigo_init_text_item(AUX_SENSOR_NAME_EXP_3_ITEM, AUX_GPIO_SENSOR_NAME_3_ITEM_NAME, "DB9 Pin 8", "Sensor #3");
		indigo_init_text_item(AUX_SENSOR_NAME_EXP_4_ITEM, AUX_GPIO_SENSOR_NAME_4_ITEM_NAME, "DB9 Pin 9", "Sensor #4");
		AUX_GPIO_SENSORS_EXP_PROPERTY = indigo_init_number_property(NULL, device->name, AUX_GPIO_SENSORS_PROPERTY_NAME, AUX_SENSORS_GROUP, "GPIO sensors", INDIGO_OK_STATE, INDIGO_RO_PERM, 4);
		if (AUX_GPIO_SENSORS_EXP_PROPERTY == NULL) {
			return INDIGO_FAILED;
		}
		indigo_init_number_item(AUX_GPIO_SENSOR_EXP_1_ITEM, AUX_GPIO_SENSOR_NAME_1_ITEM_NAME, "Sensor #1", 0, 1024, 1, 0);
		indigo_init_number_item(AUX_GPIO_SENSOR_EXP_2_ITEM, AUX_GPIO_SENSOR_NAME_2_ITEM_NAME, "Sensor #2", 0, 1024, 1, 0);
		indigo_init_number_item(AUX_GPIO_SENSOR_EXP_3_ITEM, AUX_GPIO_SENSOR_NAME_3_ITEM_NAME, "Sensor #3", 0, 1024, 1, 0);
		indigo_init_number_item(AUX_GPIO_SENSOR_EXP_4_ITEM, AUX_GPIO_SENSOR_NAME_4_ITEM_NAME, "Sensor #4", 0, 1024, 1, 0);
		INDIGO_DEVICE_ATTACH_LOG(DRIVER_NAME, device->name);
		return aux_exp_enumerate_properties(device, NULL, NULL);
	}
	return INDIGO_FAILED;
}

static indigo_result aux_exp_enumerate_properties(indigo_device *device, indigo_client *client, indigo_property *property) {
	if (IS_CONNECTED) {
		INDIGO_DEFINE_MATCHING_PROPERTY(AUX_POWER_OUTLET_EXP_PROPERTY);
		INDIGO_DEFINE_MATCHING_PROPERTY(AUX_GPIO_SENSORS_EXP_PROPERTY);
	}
	INDIGO_DEFINE_MATCHING_PROPERTY(AUX_OUTLET_NAMES_EXP_PROPERTY);
	INDIGO_DEFINE_MATCHING_PROPERTY(AUX_SENSOR_NAMES_EXP_PROPERTY);
	return indigo_aux_enumerate_properties(device, client, property);
}

static indigo_result aux_exp_change_property(indigo_device *device, indigo_client *client, indigo_property *property) {
	if (indigo_property_match_changeable(CONNECTION_PROPERTY, property)) {
		INDIGO_PROCESS_CONNECT(aux_exp_connection_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(AUX_OUTLET_NAMES_EXP_PROPERTY, property)) {
		INDIGO_COPY_VALUES_PROCESS_CHANGE(AUX_OUTLET_NAMES_EXP_PROPERTY, aux_exp_aux_outlet_names_exp_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(AUX_POWER_OUTLET_EXP_PROPERTY, property)) {
		INDIGO_COPY_VALUES_PROCESS_CHANGE(AUX_POWER_OUTLET_EXP_PROPERTY, aux_exp_aux_power_outlet_exp_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(AUX_SENSOR_NAMES_EXP_PROPERTY, property)) {
		INDIGO_COPY_VALUES_PROCESS_CHANGE(AUX_SENSOR_NAMES_EXP_PROPERTY, aux_exp_aux_sensor_names_exp_handler);
		return INDIGO_OK;
	} else if (indigo_property_match(CONFIG_PROPERTY, property)) {
		if (indigo_switch_match(CONFIG_SAVE_ITEM, property)) {
			indigo_save_property(device, NULL, AUX_OUTLET_NAMES_EXP_PROPERTY);
			indigo_save_property(device, NULL, AUX_SENSOR_NAMES_EXP_PROPERTY);
		}
	}
	return indigo_aux_change_property(device, client, property);
}

static indigo_result aux_exp_detach(indigo_device *device) {
	if (IS_CONNECTED) {
		indigo_set_switch(CONNECTION_PROPERTY, CONNECTION_DISCONNECTED_ITEM, true);
		aux_exp_connection_handler(device);
	}
	indigo_release_property(AUX_OUTLET_NAMES_EXP_PROPERTY);
	indigo_release_property(AUX_POWER_OUTLET_EXP_PROPERTY);
	indigo_release_property(AUX_SENSOR_NAMES_EXP_PROPERTY);
	indigo_release_property(AUX_GPIO_SENSORS_EXP_PROPERTY);
	INDIGO_DEVICE_DETACH_LOG(DRIVER_NAME, device->name);
	return indigo_aux_detach(device);
}

#pragma mark - High level code (focuser_third)
// device_id: focuser_third type: focuser

static void focuser_third_timer_callback(indigo_device *device) {
	if (!IS_CONNECTED) {
		return;
	}
	//+ focuser_third.on_timer
	lunatico_focuser_poll_temperature(device);
	indigo_execute_handler_in(device, LUNATICO_SENSOR_POLL, focuser_third_timer_callback);
	//- focuser_third.on_timer
}

static void focuser_third_connection_handler(indigo_device *device) {
	if (CONNECTION_CONNECTED_ITEM->sw.value) {
		bool connection_result = true;
		if (PRIVATE_DATA->count == 0) {
			connection_result = lunatico_open(device->master_device);
		}
		if (connection_result) {
			PRIVATE_DATA->count++;
		}
		if (connection_result) {
			//+ focuser_third.on_connect
			connection_result = lunatico_focuser_connect(device, X_FOCUSER_POWER_CONTROL_THIRD_PROPERTY, X_FOCUSER_MOTOR_TYPE_THIRD_PROPERTY, X_FOCUSER_STEP_MODE_THIRD_PROPERTY, X_FOCUSER_MOTOR_WIRING_THIRD_PROPERTY, X_FOCUSER_TEMPERATURE_SENSOR_THIRD_PROPERTY);
			//- focuser_third.on_connect
		}
		if (connection_result) {
			indigo_define_property(device, X_FOCUSER_STEP_MODE_THIRD_PROPERTY, NULL);
			indigo_define_property(device, X_FOCUSER_POWER_CONTROL_THIRD_PROPERTY, NULL);
			indigo_define_property(device, X_FOCUSER_MOTOR_WIRING_THIRD_PROPERTY, NULL);
			indigo_define_property(device, X_FOCUSER_MOTOR_TYPE_THIRD_PROPERTY, NULL);
			indigo_define_property(device, X_FOCUSER_TEMPERATURE_SENSOR_THIRD_PROPERTY, NULL);
			CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
			indigo_send_message(device, OK_PROPERTY, "Connected to %s on %s", FOCUSER_THIRD_DEVICE_NAME, DEVICE_PORT_ITEM->text.value);
		} else {
			indigo_send_message(device, ALERT_PROPERTY, "Failed to connect to %s on %s", FOCUSER_THIRD_DEVICE_NAME, DEVICE_PORT_ITEM->text.value);
			if (PRIVATE_DATA->count > 0 && --PRIVATE_DATA->count == 0) {
				lunatico_close(device);
			}
			CONNECTION_PROPERTY->state = INDIGO_ALERT_STATE;
			indigo_set_switch(CONNECTION_PROPERTY, CONNECTION_DISCONNECTED_ITEM, true);
		}
	} else {
		indigo_cancel_pending_handlers(device);
		//+ focuser_third.on_disconnect
		lunatico_release_port(device);
		//- focuser_third.on_disconnect
		indigo_delete_property(device, X_FOCUSER_STEP_MODE_THIRD_PROPERTY, NULL);
		indigo_delete_property(device, X_FOCUSER_POWER_CONTROL_THIRD_PROPERTY, NULL);
		indigo_delete_property(device, X_FOCUSER_MOTOR_WIRING_THIRD_PROPERTY, NULL);
		indigo_delete_property(device, X_FOCUSER_MOTOR_TYPE_THIRD_PROPERTY, NULL);
		indigo_delete_property(device, X_FOCUSER_TEMPERATURE_SENSOR_THIRD_PROPERTY, NULL);
		if (--PRIVATE_DATA->count == 0) {
			lunatico_close(device);
		}
		indigo_send_message(device, OK_PROPERTY, "Disconnected from %s", device->name);
		CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
	}
	indigo_focuser_change_property(device, NULL, CONNECTION_PROPERTY);
	if (IS_CONNECTED) {
		indigo_execute_handler(device, focuser_third_timer_callback);
	}
}

static void focuser_third_focuser_speed_handler(indigo_device *device) {
	FOCUSER_SPEED_PROPERTY->state = INDIGO_OK_STATE;
	//+ focuser_third.FOCUSER_SPEED.on_change
	if (!lunatico_set_speed(device, FOCUSER_SPEED_ITEM->number.target)) {
		FOCUSER_SPEED_PROPERTY->state = INDIGO_ALERT_STATE;
	}
	//- focuser_third.FOCUSER_SPEED.on_change
	indigo_update_property(device, FOCUSER_SPEED_PROPERTY, NULL);
}

static void focuser_third_focuser_limits_handler(indigo_device *device) {
	FOCUSER_LIMITS_PROPERTY->state = INDIGO_OK_STATE;
	//+ focuser_third.FOCUSER_LIMITS.on_change
	lunatico_focuser_limits(device);
	//- focuser_third.FOCUSER_LIMITS.on_change
	indigo_update_property(device, FOCUSER_LIMITS_PROPERTY, NULL);
}

static void focuser_third_focuser_mode_handler(indigo_device *device) {
	FOCUSER_MODE_PROPERTY->state = INDIGO_OK_STATE;
	//+ focuser_third.FOCUSER_MODE.on_change
	lunatico_focuser_mode(device);
	//- focuser_third.FOCUSER_MODE.on_change
	indigo_update_property(device, FOCUSER_MODE_PROPERTY, NULL);
}

static void focuser_third_focuser_reverse_motion_handler(indigo_device *device) {
	FOCUSER_REVERSE_MOTION_PROPERTY->state = INDIGO_OK_STATE;
	//+ focuser_third.FOCUSER_REVERSE_MOTION.on_change
	if (!lunatico_apply_wiring(device, X_FOCUSER_MOTOR_WIRING_THIRD_PROPERTY, !FOCUSER_REVERSE_MOTION_DISABLED_ITEM->sw.value)) {
		FOCUSER_REVERSE_MOTION_PROPERTY->state = INDIGO_ALERT_STATE;
	}
	//- focuser_third.FOCUSER_REVERSE_MOTION.on_change
	indigo_update_property(device, FOCUSER_REVERSE_MOTION_PROPERTY, NULL);
}

static void focuser_third_focuser_position_handler(indigo_device *device) {
	//+ focuser_third.FOCUSER_POSITION.on_change
	if (lunatico_focuser_position(device)) {
		indigo_execute_handler_in(device, LUNATICO_MOTION_POLL, focuser_motion_finalizer);
	}
	//- focuser_third.FOCUSER_POSITION.on_change
}

static void focuser_third_focuser_steps_handler(indigo_device *device) {
	//+ focuser_third.FOCUSER_STEPS.on_change
	if (lunatico_focuser_steps(device)) {
		indigo_execute_handler_in(device, LUNATICO_MOTION_POLL, focuser_motion_finalizer);
	}
	//- focuser_third.FOCUSER_STEPS.on_change
}

static void focuser_third_focuser_abort_motion_handler(indigo_device *device) {
	//+ focuser_third.FOCUSER_ABORT_MOTION.on_change
	// An urgent abort can overtake a queued move, so the start
	// handlers of this device are cancelled together with the
	// completion poll they would have scheduled.
	indigo_cancel_pending_handler(device, focuser_third_focuser_position_handler);
	indigo_cancel_pending_handler(device, focuser_third_focuser_steps_handler);
	indigo_cancel_pending_handler(device, focuser_motion_finalizer);
	lunatico_focuser_abort(device);
	//- focuser_third.FOCUSER_ABORT_MOTION.on_change
}

static void focuser_third_x_focuser_step_mode_third_handler(indigo_device *device) {
	X_FOCUSER_STEP_MODE_THIRD_PROPERTY->state = INDIGO_OK_STATE;
	//+ focuser_third.X_FOCUSER_STEP_MODE_THIRD.on_change
	if (!lunatico_apply_step_mode(device, X_FOCUSER_STEP_MODE_THIRD_PROPERTY)) {
		X_FOCUSER_STEP_MODE_THIRD_PROPERTY->state = INDIGO_ALERT_STATE;
	}
	//- focuser_third.X_FOCUSER_STEP_MODE_THIRD.on_change
	indigo_update_property(device, X_FOCUSER_STEP_MODE_THIRD_PROPERTY, NULL);
}

static void focuser_third_x_focuser_power_control_third_handler(indigo_device *device) {
	X_FOCUSER_POWER_CONTROL_THIRD_PROPERTY->state = INDIGO_OK_STATE;
	//+ focuser_third.X_FOCUSER_POWER_CONTROL_THIRD.on_change
	if (!lunatico_apply_power_control(device, X_FOCUSER_POWER_CONTROL_THIRD_PROPERTY)) {
		X_FOCUSER_POWER_CONTROL_THIRD_PROPERTY->state = INDIGO_ALERT_STATE;
	}
	//- focuser_third.X_FOCUSER_POWER_CONTROL_THIRD.on_change
	indigo_update_property(device, X_FOCUSER_POWER_CONTROL_THIRD_PROPERTY, NULL);
}

static void focuser_third_x_focuser_motor_wiring_third_handler(indigo_device *device) {
	X_FOCUSER_MOTOR_WIRING_THIRD_PROPERTY->state = INDIGO_OK_STATE;
	//+ focuser_third.X_FOCUSER_MOTOR_WIRING_THIRD.on_change
	if (!lunatico_apply_wiring(device, X_FOCUSER_MOTOR_WIRING_THIRD_PROPERTY, !FOCUSER_REVERSE_MOTION_DISABLED_ITEM->sw.value)) {
		X_FOCUSER_MOTOR_WIRING_THIRD_PROPERTY->state = INDIGO_ALERT_STATE;
	}
	//- focuser_third.X_FOCUSER_MOTOR_WIRING_THIRD.on_change
	indigo_update_property(device, X_FOCUSER_MOTOR_WIRING_THIRD_PROPERTY, NULL);
}

static void focuser_third_x_focuser_motor_type_third_handler(indigo_device *device) {
	X_FOCUSER_MOTOR_TYPE_THIRD_PROPERTY->state = INDIGO_OK_STATE;
	//+ focuser_third.X_FOCUSER_MOTOR_TYPE_THIRD.on_change
	if (!lunatico_apply_motor_type(device, X_FOCUSER_MOTOR_TYPE_THIRD_PROPERTY)) {
		X_FOCUSER_MOTOR_TYPE_THIRD_PROPERTY->state = INDIGO_ALERT_STATE;
	}
	//- focuser_third.X_FOCUSER_MOTOR_TYPE_THIRD.on_change
	indigo_update_property(device, X_FOCUSER_MOTOR_TYPE_THIRD_PROPERTY, NULL);
}

static void focuser_third_x_focuser_temperature_sensor_third_handler(indigo_device *device) {
	X_FOCUSER_TEMPERATURE_SENSOR_THIRD_PROPERTY->state = INDIGO_OK_STATE;
	//+ focuser_third.X_FOCUSER_TEMPERATURE_SENSOR_THIRD.on_change
	lunatico_apply_temperature_sensor(device, X_FOCUSER_TEMPERATURE_SENSOR_THIRD_PROPERTY);
	//- focuser_third.X_FOCUSER_TEMPERATURE_SENSOR_THIRD.on_change
	indigo_update_property(device, X_FOCUSER_TEMPERATURE_SENSOR_THIRD_PROPERTY, NULL);
}

#pragma mark - Device API (focuser_third)

static indigo_result focuser_third_enumerate_properties(indigo_device *device, indigo_client *client, indigo_property *property);

static indigo_result focuser_third_attach(indigo_device *device) {
	if (indigo_focuser_attach(device, DRIVER_NAME, DRIVER_VERSION) == INDIGO_OK) {
		//+ focuser_third.on_attach
		device->gp_bits = 2;
		INFO_PROPERTY->count = 6;
		//- focuser_third.on_attach
		FOCUSER_TEMPERATURE_PROPERTY->hidden = false;
		FOCUSER_ON_POSITION_SET_PROPERTY->hidden = false;
		FOCUSER_SPEED_PROPERTY->hidden = false;
		//+ focuser_third.FOCUSER_SPEED.on_attach
		FOCUSER_SPEED_ITEM->number.min = .002;
		FOCUSER_SPEED_ITEM->number.max = 20;
		FOCUSER_SPEED_ITEM->number.step = .1;
		FOCUSER_SPEED_ITEM->number.value = FOCUSER_SPEED_ITEM->number.target = .1;
		strcpy(FOCUSER_SPEED_ITEM->label, "Speed (kHz)");
		//- focuser_third.FOCUSER_SPEED.on_attach
		FOCUSER_LIMITS_PROPERTY->hidden = false;
		//+ focuser_third.FOCUSER_LIMITS.on_attach
		FOCUSER_LIMITS_MAX_POSITION_ITEM->number.min = 1;
		FOCUSER_LIMITS_MAX_POSITION_ITEM->number.max = 100000;
		FOCUSER_LIMITS_MAX_POSITION_ITEM->number.value = FOCUSER_LIMITS_MAX_POSITION_ITEM->number.target = 100000;
		FOCUSER_LIMITS_MAX_POSITION_ITEM->number.step = 1;
		FOCUSER_LIMITS_MIN_POSITION_ITEM->number.min = 0;
		FOCUSER_LIMITS_MIN_POSITION_ITEM->number.max = 100000;
		FOCUSER_LIMITS_MIN_POSITION_ITEM->number.value = FOCUSER_LIMITS_MIN_POSITION_ITEM->number.target = 0;
		FOCUSER_LIMITS_MIN_POSITION_ITEM->number.step = 1;
		//- focuser_third.FOCUSER_LIMITS.on_attach
		FOCUSER_BACKLASH_PROPERTY->hidden = false;
		//+ focuser_third.FOCUSER_BACKLASH.on_attach
		FOCUSER_BACKLASH_ITEM->number.min = 0;
		FOCUSER_BACKLASH_ITEM->number.max = 200;
		FOCUSER_BACKLASH_ITEM->number.step = 5;
		FOCUSER_BACKLASH_ITEM->number.value = FOCUSER_BACKLASH_ITEM->number.target = 0;
		//- focuser_third.FOCUSER_BACKLASH.on_attach
		FOCUSER_COMPENSATION_PROPERTY->hidden = false;
		//+ focuser_third.FOCUSER_COMPENSATION.on_attach
		FOCUSER_COMPENSATION_ITEM->number.min = -10000;
		FOCUSER_COMPENSATION_ITEM->number.max = 10000;
		//- focuser_third.FOCUSER_COMPENSATION.on_attach
		FOCUSER_MODE_PROPERTY->hidden = false;
		FOCUSER_REVERSE_MOTION_PROPERTY->hidden = false;
		FOCUSER_POSITION_PROPERTY->hidden = false;
		//+ focuser_third.FOCUSER_POSITION.on_attach
		FOCUSER_POSITION_ITEM->number.min = 0;
		FOCUSER_POSITION_ITEM->number.max = 100000;
		FOCUSER_POSITION_ITEM->number.step = 100;
		//- focuser_third.FOCUSER_POSITION.on_attach
		FOCUSER_STEPS_PROPERTY->hidden = false;
		//+ focuser_third.FOCUSER_STEPS.on_attach
		FOCUSER_STEPS_ITEM->number.min = 0;
		FOCUSER_STEPS_ITEM->number.step = 1;
		//- focuser_third.FOCUSER_STEPS.on_attach
		FOCUSER_ABORT_MOTION_PROPERTY->hidden = false;
		X_FOCUSER_STEP_MODE_THIRD_PROPERTY = indigo_init_switch_property(NULL, device->name, X_FOCUSER_STEP_MODE_THIRD_PROPERTY_NAME, FOCUSER_ADVANCED_GROUP, "Step mode", INDIGO_OK_STATE, INDIGO_RW_PERM, INDIGO_ONE_OF_MANY_RULE, 2);
		if (X_FOCUSER_STEP_MODE_THIRD_PROPERTY == NULL) {
			return INDIGO_FAILED;
		}
		indigo_init_switch_item(X_FOCUSER_STEP_MODE_THIRD_FULL_ITEM, X_FOCUSER_STEP_MODE_THIRD_FULL_ITEM_NAME, "Full step", true);
		indigo_init_switch_item(X_FOCUSER_STEP_MODE_THIRD_HALF_ITEM, X_FOCUSER_STEP_MODE_THIRD_HALF_ITEM_NAME, "1/2 step", false);
		X_FOCUSER_POWER_CONTROL_THIRD_PROPERTY = indigo_init_number_property(NULL, device->name, X_FOCUSER_POWER_CONTROL_THIRD_PROPERTY_NAME, FOCUSER_ADVANCED_GROUP, "Coils current control", INDIGO_OK_STATE, INDIGO_RW_PERM, 2);
		if (X_FOCUSER_POWER_CONTROL_THIRD_PROPERTY == NULL) {
			return INDIGO_FAILED;
		}
		indigo_init_number_item(X_FOCUSER_POWER_CONTROL_THIRD_MOVE_ITEM, X_FOCUSER_POWER_CONTROL_THIRD_MOVE_ITEM_NAME, "Move power (%)", 0, 100, 1, 100);
		indigo_init_number_item(X_FOCUSER_POWER_CONTROL_THIRD_STOP_ITEM, X_FOCUSER_POWER_CONTROL_THIRD_STOP_ITEM_NAME, "Stop power (%)", 0, 100, 1, 0);
		X_FOCUSER_MOTOR_WIRING_THIRD_PROPERTY = indigo_init_switch_property(NULL, device->name, X_FOCUSER_MOTOR_WIRING_THIRD_PROPERTY_NAME, FOCUSER_ADVANCED_GROUP, "Motor wiring", INDIGO_OK_STATE, INDIGO_RW_PERM, INDIGO_ONE_OF_MANY_RULE, 2);
		if (X_FOCUSER_MOTOR_WIRING_THIRD_PROPERTY == NULL) {
			return INDIGO_FAILED;
		}
		indigo_init_switch_item(X_FOCUSER_MOTOR_WIRING_THIRD_LUNATICO_ITEM, X_FOCUSER_MOTOR_WIRING_THIRD_LUNATICO_ITEM_NAME, "Lunatico", true);
		indigo_init_switch_item(X_FOCUSER_MOTOR_WIRING_THIRD_MOONLITE_ITEM, X_FOCUSER_MOTOR_WIRING_THIRD_MOONLITE_ITEM_NAME, "RF/Moonlite", false);
		X_FOCUSER_MOTOR_TYPE_THIRD_PROPERTY = indigo_init_switch_property(NULL, device->name, X_FOCUSER_MOTOR_TYPE_THIRD_PROPERTY_NAME, FOCUSER_ADVANCED_GROUP, "Motor type", INDIGO_OK_STATE, INDIGO_RW_PERM, INDIGO_ONE_OF_MANY_RULE, 4);
		if (X_FOCUSER_MOTOR_TYPE_THIRD_PROPERTY == NULL) {
			return INDIGO_FAILED;
		}
		indigo_init_switch_item(X_FOCUSER_MOTOR_TYPE_THIRD_UNIPOLAR_ITEM, X_FOCUSER_MOTOR_TYPE_THIRD_UNIPOLAR_ITEM_NAME, "Unipolar", true);
		indigo_init_switch_item(X_FOCUSER_MOTOR_TYPE_THIRD_BIPOLAR_ITEM, X_FOCUSER_MOTOR_TYPE_THIRD_BIPOLAR_ITEM_NAME, "Bipolar", false);
		indigo_init_switch_item(X_FOCUSER_MOTOR_TYPE_THIRD_DC_ITEM, X_FOCUSER_MOTOR_TYPE_THIRD_DC_ITEM_NAME, "DC", false);
		indigo_init_switch_item(X_FOCUSER_MOTOR_TYPE_THIRD_STEP_DIR_ITEM, X_FOCUSER_MOTOR_TYPE_THIRD_STEP_DIR_ITEM_NAME, "Step-dir", false);
		X_FOCUSER_TEMPERATURE_SENSOR_THIRD_PROPERTY = indigo_init_switch_property(NULL, device->name, X_FOCUSER_TEMPERATURE_SENSOR_THIRD_PROPERTY_NAME, FOCUSER_ADVANCED_GROUP, "Temperature sensor in use", INDIGO_OK_STATE, INDIGO_RW_PERM, INDIGO_ONE_OF_MANY_RULE, 2);
		if (X_FOCUSER_TEMPERATURE_SENSOR_THIRD_PROPERTY == NULL) {
			return INDIGO_FAILED;
		}
		indigo_init_switch_item(X_FOCUSER_TEMPERATURE_SENSOR_THIRD_INTERNAL_ITEM, X_FOCUSER_TEMPERATURE_SENSOR_THIRD_INTERNAL_ITEM_NAME, "Internal sensor", true);
		indigo_init_switch_item(X_FOCUSER_TEMPERATURE_SENSOR_THIRD_EXTERNAL_ITEM, X_FOCUSER_TEMPERATURE_SENSOR_THIRD_EXTERNAL_ITEM_NAME, "External sensor", false);
		INDIGO_DEVICE_ATTACH_LOG(DRIVER_NAME, device->name);
		return focuser_third_enumerate_properties(device, NULL, NULL);
	}
	return INDIGO_FAILED;
}

static indigo_result focuser_third_enumerate_properties(indigo_device *device, indigo_client *client, indigo_property *property) {
	if (IS_CONNECTED) {
		INDIGO_DEFINE_MATCHING_PROPERTY(X_FOCUSER_STEP_MODE_THIRD_PROPERTY);
		INDIGO_DEFINE_MATCHING_PROPERTY(X_FOCUSER_POWER_CONTROL_THIRD_PROPERTY);
		INDIGO_DEFINE_MATCHING_PROPERTY(X_FOCUSER_MOTOR_WIRING_THIRD_PROPERTY);
		INDIGO_DEFINE_MATCHING_PROPERTY(X_FOCUSER_MOTOR_TYPE_THIRD_PROPERTY);
		INDIGO_DEFINE_MATCHING_PROPERTY(X_FOCUSER_TEMPERATURE_SENSOR_THIRD_PROPERTY);
	}
	return indigo_focuser_enumerate_properties(device, client, property);
}

static indigo_result focuser_third_change_property(indigo_device *device, indigo_client *client, indigo_property *property) {
	if (indigo_property_match_changeable(CONNECTION_PROPERTY, property)) {
		INDIGO_PROCESS_CONNECT(focuser_third_connection_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(FOCUSER_SPEED_PROPERTY, property)) {
		INDIGO_COPY_VALUES_PROCESS_CHANGE(FOCUSER_SPEED_PROPERTY, focuser_third_focuser_speed_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(FOCUSER_LIMITS_PROPERTY, property)) {
		INDIGO_COPY_VALUES_PROCESS_CHANGE(FOCUSER_LIMITS_PROPERTY, focuser_third_focuser_limits_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(FOCUSER_BACKLASH_PROPERTY, property)) {
		indigo_property_copy_values(FOCUSER_BACKLASH_PROPERTY, property, false);
		FOCUSER_BACKLASH_PROPERTY->state = INDIGO_OK_STATE;
		indigo_update_property(device, FOCUSER_BACKLASH_PROPERTY, NULL);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(FOCUSER_COMPENSATION_PROPERTY, property)) {
		indigo_property_copy_values(FOCUSER_COMPENSATION_PROPERTY, property, false);
		FOCUSER_COMPENSATION_PROPERTY->state = INDIGO_OK_STATE;
		indigo_update_property(device, FOCUSER_COMPENSATION_PROPERTY, NULL);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(FOCUSER_MODE_PROPERTY, property)) {
		INDIGO_COPY_VALUES_PROCESS_CHANGE(FOCUSER_MODE_PROPERTY, focuser_third_focuser_mode_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(FOCUSER_REVERSE_MOTION_PROPERTY, property)) {
		INDIGO_COPY_VALUES_PROCESS_CHANGE(FOCUSER_REVERSE_MOTION_PROPERTY, focuser_third_focuser_reverse_motion_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(FOCUSER_POSITION_PROPERTY, property)) {
		INDIGO_COPY_VALUES_PROCESS_CHANGE(FOCUSER_POSITION_PROPERTY, focuser_third_focuser_position_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(FOCUSER_STEPS_PROPERTY, property)) {
		INDIGO_COPY_VALUES_PROCESS_CHANGE(FOCUSER_STEPS_PROPERTY, focuser_third_focuser_steps_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(FOCUSER_ABORT_MOTION_PROPERTY, property)) {
		INDIGO_COPY_VALUES_PROCESS_URGENT_CHANGE(FOCUSER_ABORT_MOTION_PROPERTY, focuser_third_focuser_abort_motion_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(X_FOCUSER_STEP_MODE_THIRD_PROPERTY, property)) {
		INDIGO_COPY_VALUES_PROCESS_CHANGE(X_FOCUSER_STEP_MODE_THIRD_PROPERTY, focuser_third_x_focuser_step_mode_third_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(X_FOCUSER_POWER_CONTROL_THIRD_PROPERTY, property)) {
		INDIGO_COPY_VALUES_PROCESS_CHANGE(X_FOCUSER_POWER_CONTROL_THIRD_PROPERTY, focuser_third_x_focuser_power_control_third_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(X_FOCUSER_MOTOR_WIRING_THIRD_PROPERTY, property)) {
		INDIGO_COPY_VALUES_PROCESS_CHANGE(X_FOCUSER_MOTOR_WIRING_THIRD_PROPERTY, focuser_third_x_focuser_motor_wiring_third_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(X_FOCUSER_MOTOR_TYPE_THIRD_PROPERTY, property)) {
		INDIGO_COPY_VALUES_PROCESS_CHANGE(X_FOCUSER_MOTOR_TYPE_THIRD_PROPERTY, focuser_third_x_focuser_motor_type_third_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(X_FOCUSER_TEMPERATURE_SENSOR_THIRD_PROPERTY, property)) {
		INDIGO_COPY_VALUES_PROCESS_CHANGE(X_FOCUSER_TEMPERATURE_SENSOR_THIRD_PROPERTY, focuser_third_x_focuser_temperature_sensor_third_handler);
		return INDIGO_OK;
	} else if (indigo_property_match(CONFIG_PROPERTY, property)) {
		if (indigo_switch_match(CONFIG_SAVE_ITEM, property)) {
			indigo_save_property(device, NULL, FOCUSER_SPEED_PROPERTY);
			indigo_save_property(device, NULL, FOCUSER_LIMITS_PROPERTY);
			indigo_save_property(device, NULL, FOCUSER_BACKLASH_PROPERTY);
			indigo_save_property(device, NULL, FOCUSER_COMPENSATION_PROPERTY);
			indigo_save_property(device, NULL, FOCUSER_REVERSE_MOTION_PROPERTY);
			indigo_save_property(device, NULL, X_FOCUSER_STEP_MODE_THIRD_PROPERTY);
			indigo_save_property(device, NULL, X_FOCUSER_POWER_CONTROL_THIRD_PROPERTY);
			indigo_save_property(device, NULL, X_FOCUSER_MOTOR_WIRING_THIRD_PROPERTY);
			indigo_save_property(device, NULL, X_FOCUSER_MOTOR_TYPE_THIRD_PROPERTY);
			indigo_save_property(device, NULL, X_FOCUSER_TEMPERATURE_SENSOR_THIRD_PROPERTY);
		}
	}
	return indigo_focuser_change_property(device, client, property);
}

static indigo_result focuser_third_detach(indigo_device *device) {
	if (IS_CONNECTED) {
		indigo_set_switch(CONNECTION_PROPERTY, CONNECTION_DISCONNECTED_ITEM, true);
		focuser_third_connection_handler(device);
	}
	indigo_release_property(X_FOCUSER_STEP_MODE_THIRD_PROPERTY);
	indigo_release_property(X_FOCUSER_POWER_CONTROL_THIRD_PROPERTY);
	indigo_release_property(X_FOCUSER_MOTOR_WIRING_THIRD_PROPERTY);
	indigo_release_property(X_FOCUSER_MOTOR_TYPE_THIRD_PROPERTY);
	indigo_release_property(X_FOCUSER_TEMPERATURE_SENSOR_THIRD_PROPERTY);
	INDIGO_DEVICE_DETACH_LOG(DRIVER_NAME, device->name);
	return indigo_focuser_detach(device);
}

#pragma mark - High level code (rotator_third)
// device_id: rotator_third type: rotator

static void rotator_third_connection_handler(indigo_device *device) {
	if (CONNECTION_CONNECTED_ITEM->sw.value) {
		bool connection_result = true;
		if (PRIVATE_DATA->count == 0) {
			connection_result = lunatico_open(device->master_device);
		}
		if (connection_result) {
			PRIVATE_DATA->count++;
		}
		if (connection_result) {
			//+ rotator_third.on_connect
			connection_result = lunatico_rotator_connect(device, X_ROTATOR_POWER_CONTROL_THIRD_PROPERTY, X_ROTATOR_MOTOR_TYPE_THIRD_PROPERTY, X_ROTATOR_STEP_MODE_THIRD_PROPERTY, X_ROTATOR_MOTOR_WIRING_THIRD_PROPERTY);
			//- rotator_third.on_connect
		}
		if (connection_result) {
			indigo_define_property(device, X_ROTATOR_STEP_MODE_THIRD_PROPERTY, NULL);
			indigo_define_property(device, X_ROTATOR_POWER_CONTROL_THIRD_PROPERTY, NULL);
			indigo_define_property(device, X_ROTATOR_MOTOR_WIRING_THIRD_PROPERTY, NULL);
			indigo_define_property(device, X_ROTATOR_MOTOR_TYPE_THIRD_PROPERTY, NULL);
			CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
			indigo_send_message(device, OK_PROPERTY, "Connected to %s on %s", ROTATOR_THIRD_DEVICE_NAME, DEVICE_PORT_ITEM->text.value);
		} else {
			indigo_send_message(device, ALERT_PROPERTY, "Failed to connect to %s on %s", ROTATOR_THIRD_DEVICE_NAME, DEVICE_PORT_ITEM->text.value);
			if (PRIVATE_DATA->count > 0 && --PRIVATE_DATA->count == 0) {
				lunatico_close(device);
			}
			CONNECTION_PROPERTY->state = INDIGO_ALERT_STATE;
			indigo_set_switch(CONNECTION_PROPERTY, CONNECTION_DISCONNECTED_ITEM, true);
		}
	} else {
		indigo_cancel_pending_handlers(device);
		//+ rotator_third.on_disconnect
		lunatico_release_port(device);
		//- rotator_third.on_disconnect
		indigo_delete_property(device, X_ROTATOR_STEP_MODE_THIRD_PROPERTY, NULL);
		indigo_delete_property(device, X_ROTATOR_POWER_CONTROL_THIRD_PROPERTY, NULL);
		indigo_delete_property(device, X_ROTATOR_MOTOR_WIRING_THIRD_PROPERTY, NULL);
		indigo_delete_property(device, X_ROTATOR_MOTOR_TYPE_THIRD_PROPERTY, NULL);
		if (--PRIVATE_DATA->count == 0) {
			lunatico_close(device);
		}
		indigo_send_message(device, OK_PROPERTY, "Disconnected from %s", device->name);
		CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
	}
	indigo_rotator_change_property(device, NULL, CONNECTION_PROPERTY);
}

static void rotator_third_rotator_steps_per_revolution_handler(indigo_device *device) {
	ROTATOR_STEPS_PER_REVOLUTION_PROPERTY->state = INDIGO_OK_STATE;
	//+ rotator_third.ROTATOR_STEPS_PER_REVOLUTION.on_change
	lunatico_rotator_resync(device);
	//- rotator_third.ROTATOR_STEPS_PER_REVOLUTION.on_change
	indigo_update_property(device, ROTATOR_STEPS_PER_REVOLUTION_PROPERTY, NULL);
}

static void rotator_third_rotator_direction_handler(indigo_device *device) {
	ROTATOR_DIRECTION_PROPERTY->state = INDIGO_OK_STATE;
	//+ rotator_third.ROTATOR_DIRECTION.on_change
	if (!lunatico_apply_wiring(device, X_ROTATOR_MOTOR_WIRING_THIRD_PROPERTY, !ROTATOR_DIRECTION_NORMAL_ITEM->sw.value)) {
		ROTATOR_DIRECTION_PROPERTY->state = INDIGO_ALERT_STATE;
	}
	//- rotator_third.ROTATOR_DIRECTION.on_change
	indigo_update_property(device, ROTATOR_DIRECTION_PROPERTY, NULL);
}

static void rotator_third_rotator_limits_handler(indigo_device *device) {
	ROTATOR_LIMITS_PROPERTY->state = INDIGO_OK_STATE;
	//+ rotator_third.ROTATOR_LIMITS.on_change
	lunatico_rotator_limits(device);
	//- rotator_third.ROTATOR_LIMITS.on_change
	indigo_update_property(device, ROTATOR_LIMITS_PROPERTY, NULL);
}

static void rotator_third_rotator_position_handler(indigo_device *device) {
	//+ rotator_third.ROTATOR_POSITION.on_change
	if (lunatico_rotator_position(device)) {
		indigo_execute_handler_in(device, LUNATICO_MOTION_POLL, rotator_motion_finalizer);
	}
	//- rotator_third.ROTATOR_POSITION.on_change
}

static void rotator_third_rotator_abort_motion_handler(indigo_device *device) {
	//+ rotator_third.ROTATOR_ABORT_MOTION.on_change
	// An urgent abort can overtake a queued move, so the start
	// handler of this device is cancelled together with the
	// completion poll it would have scheduled.
	indigo_cancel_pending_handler(device, rotator_third_rotator_position_handler);
	indigo_cancel_pending_handler(device, rotator_motion_finalizer);
	lunatico_rotator_abort(device);
	//- rotator_third.ROTATOR_ABORT_MOTION.on_change
}

static void rotator_third_x_rotator_step_mode_third_handler(indigo_device *device) {
	X_ROTATOR_STEP_MODE_THIRD_PROPERTY->state = INDIGO_OK_STATE;
	//+ rotator_third.X_ROTATOR_STEP_MODE_THIRD.on_change
	if (!lunatico_apply_step_mode(device, X_ROTATOR_STEP_MODE_THIRD_PROPERTY)) {
		X_ROTATOR_STEP_MODE_THIRD_PROPERTY->state = INDIGO_ALERT_STATE;
	}
	//- rotator_third.X_ROTATOR_STEP_MODE_THIRD.on_change
	indigo_update_property(device, X_ROTATOR_STEP_MODE_THIRD_PROPERTY, NULL);
}

static void rotator_third_x_rotator_power_control_third_handler(indigo_device *device) {
	X_ROTATOR_POWER_CONTROL_THIRD_PROPERTY->state = INDIGO_OK_STATE;
	//+ rotator_third.X_ROTATOR_POWER_CONTROL_THIRD.on_change
	if (!lunatico_apply_power_control(device, X_ROTATOR_POWER_CONTROL_THIRD_PROPERTY)) {
		X_ROTATOR_POWER_CONTROL_THIRD_PROPERTY->state = INDIGO_ALERT_STATE;
	}
	//- rotator_third.X_ROTATOR_POWER_CONTROL_THIRD.on_change
	indigo_update_property(device, X_ROTATOR_POWER_CONTROL_THIRD_PROPERTY, NULL);
}

static void rotator_third_x_rotator_motor_wiring_third_handler(indigo_device *device) {
	X_ROTATOR_MOTOR_WIRING_THIRD_PROPERTY->state = INDIGO_OK_STATE;
	//+ rotator_third.X_ROTATOR_MOTOR_WIRING_THIRD.on_change
	if (!lunatico_apply_wiring(device, X_ROTATOR_MOTOR_WIRING_THIRD_PROPERTY, !ROTATOR_DIRECTION_NORMAL_ITEM->sw.value)) {
		X_ROTATOR_MOTOR_WIRING_THIRD_PROPERTY->state = INDIGO_ALERT_STATE;
	}
	//- rotator_third.X_ROTATOR_MOTOR_WIRING_THIRD.on_change
	indigo_update_property(device, X_ROTATOR_MOTOR_WIRING_THIRD_PROPERTY, NULL);
}

static void rotator_third_x_rotator_motor_type_third_handler(indigo_device *device) {
	X_ROTATOR_MOTOR_TYPE_THIRD_PROPERTY->state = INDIGO_OK_STATE;
	//+ rotator_third.X_ROTATOR_MOTOR_TYPE_THIRD.on_change
	if (!lunatico_apply_motor_type(device, X_ROTATOR_MOTOR_TYPE_THIRD_PROPERTY)) {
		X_ROTATOR_MOTOR_TYPE_THIRD_PROPERTY->state = INDIGO_ALERT_STATE;
	}
	//- rotator_third.X_ROTATOR_MOTOR_TYPE_THIRD.on_change
	indigo_update_property(device, X_ROTATOR_MOTOR_TYPE_THIRD_PROPERTY, NULL);
}

#pragma mark - Device API (rotator_third)

static indigo_result rotator_third_enumerate_properties(indigo_device *device, indigo_client *client, indigo_property *property);

static indigo_result rotator_third_attach(indigo_device *device) {
	if (indigo_rotator_attach(device, DRIVER_NAME, DRIVER_VERSION) == INDIGO_OK) {
		//+ rotator_third.on_attach
		device->gp_bits = 2;
		INFO_PROPERTY->count = 6;
		//- rotator_third.on_attach
		ROTATOR_STEPS_PER_REVOLUTION_PROPERTY->hidden = false;
		//+ rotator_third.ROTATOR_STEPS_PER_REVOLUTION.on_attach
		ROTATOR_STEPS_PER_REVOLUTION_ITEM->number.min = 100;
		ROTATOR_STEPS_PER_REVOLUTION_ITEM->number.max = 100000;
		ROTATOR_STEPS_PER_REVOLUTION_ITEM->number.value = ROTATOR_STEPS_PER_REVOLUTION_ITEM->number.target = 3600;
		//- rotator_third.ROTATOR_STEPS_PER_REVOLUTION.on_attach
		ROTATOR_DIRECTION_PROPERTY->hidden = false;
		ROTATOR_BACKLASH_PROPERTY->hidden = false;
		ROTATOR_LIMITS_PROPERTY->hidden = false;
		//+ rotator_third.ROTATOR_LIMITS.on_attach
		ROTATOR_LIMITS_MIN_POSITION_ITEM->number.min = -180;
		ROTATOR_LIMITS_MIN_POSITION_ITEM->number.max = 360;
		ROTATOR_LIMITS_MIN_POSITION_ITEM->number.value = ROTATOR_LIMITS_MIN_POSITION_ITEM->number.target = -180;
		ROTATOR_LIMITS_MAX_POSITION_ITEM->number.min = -180;
		ROTATOR_LIMITS_MAX_POSITION_ITEM->number.max = 360;
		ROTATOR_LIMITS_MAX_POSITION_ITEM->number.value = ROTATOR_LIMITS_MAX_POSITION_ITEM->number.target = 180;
		//- rotator_third.ROTATOR_LIMITS.on_attach
		ROTATOR_POSITION_PROPERTY->hidden = false;
		ROTATOR_ABORT_MOTION_PROPERTY->hidden = false;
		X_ROTATOR_STEP_MODE_THIRD_PROPERTY = indigo_init_switch_property(NULL, device->name, X_ROTATOR_STEP_MODE_THIRD_PROPERTY_NAME, ROTATOR_ADVANCED_GROUP, "Step mode", INDIGO_OK_STATE, INDIGO_RW_PERM, INDIGO_ONE_OF_MANY_RULE, 2);
		if (X_ROTATOR_STEP_MODE_THIRD_PROPERTY == NULL) {
			return INDIGO_FAILED;
		}
		indigo_init_switch_item(X_ROTATOR_STEP_MODE_THIRD_FULL_ITEM, X_ROTATOR_STEP_MODE_THIRD_FULL_ITEM_NAME, "Full step", true);
		indigo_init_switch_item(X_ROTATOR_STEP_MODE_THIRD_HALF_ITEM, X_ROTATOR_STEP_MODE_THIRD_HALF_ITEM_NAME, "1/2 step", false);
		X_ROTATOR_POWER_CONTROL_THIRD_PROPERTY = indigo_init_number_property(NULL, device->name, X_ROTATOR_POWER_CONTROL_THIRD_PROPERTY_NAME, ROTATOR_ADVANCED_GROUP, "Coils current control", INDIGO_OK_STATE, INDIGO_RW_PERM, 2);
		if (X_ROTATOR_POWER_CONTROL_THIRD_PROPERTY == NULL) {
			return INDIGO_FAILED;
		}
		indigo_init_number_item(X_ROTATOR_POWER_CONTROL_THIRD_MOVE_ITEM, X_ROTATOR_POWER_CONTROL_THIRD_MOVE_ITEM_NAME, "Move power (%)", 0, 100, 1, 100);
		indigo_init_number_item(X_ROTATOR_POWER_CONTROL_THIRD_STOP_ITEM, X_ROTATOR_POWER_CONTROL_THIRD_STOP_ITEM_NAME, "Stop power (%)", 0, 100, 1, 0);
		X_ROTATOR_MOTOR_WIRING_THIRD_PROPERTY = indigo_init_switch_property(NULL, device->name, X_ROTATOR_MOTOR_WIRING_THIRD_PROPERTY_NAME, ROTATOR_ADVANCED_GROUP, "Motor wiring", INDIGO_OK_STATE, INDIGO_RW_PERM, INDIGO_ONE_OF_MANY_RULE, 2);
		if (X_ROTATOR_MOTOR_WIRING_THIRD_PROPERTY == NULL) {
			return INDIGO_FAILED;
		}
		indigo_init_switch_item(X_ROTATOR_MOTOR_WIRING_THIRD_LUNATICO_ITEM, X_ROTATOR_MOTOR_WIRING_THIRD_LUNATICO_ITEM_NAME, "Lunatico", true);
		indigo_init_switch_item(X_ROTATOR_MOTOR_WIRING_THIRD_MOONLITE_ITEM, X_ROTATOR_MOTOR_WIRING_THIRD_MOONLITE_ITEM_NAME, "RF/Moonlite", false);
		X_ROTATOR_MOTOR_TYPE_THIRD_PROPERTY = indigo_init_switch_property(NULL, device->name, X_ROTATOR_MOTOR_TYPE_THIRD_PROPERTY_NAME, ROTATOR_ADVANCED_GROUP, "Motor type", INDIGO_OK_STATE, INDIGO_RW_PERM, INDIGO_ONE_OF_MANY_RULE, 4);
		if (X_ROTATOR_MOTOR_TYPE_THIRD_PROPERTY == NULL) {
			return INDIGO_FAILED;
		}
		indigo_init_switch_item(X_ROTATOR_MOTOR_TYPE_THIRD_UNIPOLAR_ITEM, X_ROTATOR_MOTOR_TYPE_THIRD_UNIPOLAR_ITEM_NAME, "Unipolar", true);
		indigo_init_switch_item(X_ROTATOR_MOTOR_TYPE_THIRD_BIPOLAR_ITEM, X_ROTATOR_MOTOR_TYPE_THIRD_BIPOLAR_ITEM_NAME, "Bipolar", false);
		indigo_init_switch_item(X_ROTATOR_MOTOR_TYPE_THIRD_DC_ITEM, X_ROTATOR_MOTOR_TYPE_THIRD_DC_ITEM_NAME, "DC", false);
		indigo_init_switch_item(X_ROTATOR_MOTOR_TYPE_THIRD_STEP_DIR_ITEM, X_ROTATOR_MOTOR_TYPE_THIRD_STEP_DIR_ITEM_NAME, "Step-dir", false);
		INDIGO_DEVICE_ATTACH_LOG(DRIVER_NAME, device->name);
		return rotator_third_enumerate_properties(device, NULL, NULL);
	}
	return INDIGO_FAILED;
}

static indigo_result rotator_third_enumerate_properties(indigo_device *device, indigo_client *client, indigo_property *property) {
	if (IS_CONNECTED) {
		INDIGO_DEFINE_MATCHING_PROPERTY(X_ROTATOR_STEP_MODE_THIRD_PROPERTY);
		INDIGO_DEFINE_MATCHING_PROPERTY(X_ROTATOR_POWER_CONTROL_THIRD_PROPERTY);
		INDIGO_DEFINE_MATCHING_PROPERTY(X_ROTATOR_MOTOR_WIRING_THIRD_PROPERTY);
		INDIGO_DEFINE_MATCHING_PROPERTY(X_ROTATOR_MOTOR_TYPE_THIRD_PROPERTY);
	}
	return indigo_rotator_enumerate_properties(device, client, property);
}

static indigo_result rotator_third_change_property(indigo_device *device, indigo_client *client, indigo_property *property) {
	if (indigo_property_match_changeable(CONNECTION_PROPERTY, property)) {
		INDIGO_PROCESS_CONNECT(rotator_third_connection_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(ROTATOR_STEPS_PER_REVOLUTION_PROPERTY, property)) {
		INDIGO_COPY_VALUES_PROCESS_CHANGE(ROTATOR_STEPS_PER_REVOLUTION_PROPERTY, rotator_third_rotator_steps_per_revolution_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(ROTATOR_DIRECTION_PROPERTY, property)) {
		INDIGO_COPY_VALUES_PROCESS_CHANGE(ROTATOR_DIRECTION_PROPERTY, rotator_third_rotator_direction_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(ROTATOR_BACKLASH_PROPERTY, property)) {
		indigo_property_copy_values(ROTATOR_BACKLASH_PROPERTY, property, false);
		ROTATOR_BACKLASH_PROPERTY->state = INDIGO_OK_STATE;
		indigo_update_property(device, ROTATOR_BACKLASH_PROPERTY, NULL);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(ROTATOR_LIMITS_PROPERTY, property)) {
		INDIGO_COPY_VALUES_PROCESS_CHANGE(ROTATOR_LIMITS_PROPERTY, rotator_third_rotator_limits_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(ROTATOR_POSITION_PROPERTY, property)) {
		INDIGO_COPY_VALUES_PROCESS_CHANGE(ROTATOR_POSITION_PROPERTY, rotator_third_rotator_position_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(ROTATOR_ABORT_MOTION_PROPERTY, property)) {
		INDIGO_COPY_VALUES_PROCESS_URGENT_CHANGE(ROTATOR_ABORT_MOTION_PROPERTY, rotator_third_rotator_abort_motion_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(X_ROTATOR_STEP_MODE_THIRD_PROPERTY, property)) {
		INDIGO_COPY_VALUES_PROCESS_CHANGE(X_ROTATOR_STEP_MODE_THIRD_PROPERTY, rotator_third_x_rotator_step_mode_third_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(X_ROTATOR_POWER_CONTROL_THIRD_PROPERTY, property)) {
		INDIGO_COPY_VALUES_PROCESS_CHANGE(X_ROTATOR_POWER_CONTROL_THIRD_PROPERTY, rotator_third_x_rotator_power_control_third_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(X_ROTATOR_MOTOR_WIRING_THIRD_PROPERTY, property)) {
		INDIGO_COPY_VALUES_PROCESS_CHANGE(X_ROTATOR_MOTOR_WIRING_THIRD_PROPERTY, rotator_third_x_rotator_motor_wiring_third_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(X_ROTATOR_MOTOR_TYPE_THIRD_PROPERTY, property)) {
		INDIGO_COPY_VALUES_PROCESS_CHANGE(X_ROTATOR_MOTOR_TYPE_THIRD_PROPERTY, rotator_third_x_rotator_motor_type_third_handler);
		return INDIGO_OK;
	} else if (indigo_property_match(CONFIG_PROPERTY, property)) {
		if (indigo_switch_match(CONFIG_SAVE_ITEM, property)) {
			indigo_save_property(device, NULL, ROTATOR_STEPS_PER_REVOLUTION_PROPERTY);
			indigo_save_property(device, NULL, ROTATOR_DIRECTION_PROPERTY);
			indigo_save_property(device, NULL, ROTATOR_BACKLASH_PROPERTY);
			indigo_save_property(device, NULL, ROTATOR_LIMITS_PROPERTY);
			indigo_save_property(device, NULL, X_ROTATOR_STEP_MODE_THIRD_PROPERTY);
			indigo_save_property(device, NULL, X_ROTATOR_POWER_CONTROL_THIRD_PROPERTY);
			indigo_save_property(device, NULL, X_ROTATOR_MOTOR_WIRING_THIRD_PROPERTY);
			indigo_save_property(device, NULL, X_ROTATOR_MOTOR_TYPE_THIRD_PROPERTY);
		}
	}
	return indigo_rotator_change_property(device, client, property);
}

static indigo_result rotator_third_detach(indigo_device *device) {
	if (IS_CONNECTED) {
		indigo_set_switch(CONNECTION_PROPERTY, CONNECTION_DISCONNECTED_ITEM, true);
		rotator_third_connection_handler(device);
	}
	indigo_release_property(X_ROTATOR_STEP_MODE_THIRD_PROPERTY);
	indigo_release_property(X_ROTATOR_POWER_CONTROL_THIRD_PROPERTY);
	indigo_release_property(X_ROTATOR_MOTOR_WIRING_THIRD_PROPERTY);
	indigo_release_property(X_ROTATOR_MOTOR_TYPE_THIRD_PROPERTY);
	INDIGO_DEVICE_DETACH_LOG(DRIVER_NAME, device->name);
	return indigo_rotator_detach(device);
}

#pragma mark - High level code (aux_third)
// device_id: aux_third type: aux

static void aux_third_timer_callback(indigo_device *device) {
	if (!IS_CONNECTED) {
		return;
	}
	//+ aux_third.on_timer
	lunatico_poll_sensors(device, AUX_GPIO_SENSORS_THIRD_PROPERTY);
	indigo_execute_handler_in(device, LUNATICO_SENSOR_POLL, aux_third_timer_callback);
	//- aux_third.on_timer
}

static void aux_third_connection_handler(indigo_device *device) {
	if (CONNECTION_CONNECTED_ITEM->sw.value) {
		bool connection_result = true;
		if (PRIVATE_DATA->count == 0) {
			connection_result = lunatico_open(device->master_device);
		}
		if (connection_result) {
			PRIVATE_DATA->count++;
		}
		if (connection_result) {
			//+ aux_third.on_connect
			connection_result = lunatico_aux_connect(device, AUX_POWER_OUTLET_THIRD_PROPERTY);
			//- aux_third.on_connect
		}
		if (connection_result) {
			indigo_define_property(device, AUX_POWER_OUTLET_THIRD_PROPERTY, NULL);
			indigo_define_property(device, AUX_GPIO_SENSORS_THIRD_PROPERTY, NULL);
			CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
			indigo_send_message(device, OK_PROPERTY, "Connected to %s on %s", AUX_THIRD_DEVICE_NAME, DEVICE_PORT_ITEM->text.value);
		} else {
			indigo_send_message(device, ALERT_PROPERTY, "Failed to connect to %s on %s", AUX_THIRD_DEVICE_NAME, DEVICE_PORT_ITEM->text.value);
			if (PRIVATE_DATA->count > 0 && --PRIVATE_DATA->count == 0) {
				lunatico_close(device);
			}
			CONNECTION_PROPERTY->state = INDIGO_ALERT_STATE;
			indigo_set_switch(CONNECTION_PROPERTY, CONNECTION_DISCONNECTED_ITEM, true);
		}
	} else {
		indigo_cancel_pending_handlers(device);
		//+ aux_third.on_disconnect
		lunatico_release_port(device);
		//- aux_third.on_disconnect
		indigo_delete_property(device, AUX_POWER_OUTLET_THIRD_PROPERTY, NULL);
		indigo_delete_property(device, AUX_GPIO_SENSORS_THIRD_PROPERTY, NULL);
		if (--PRIVATE_DATA->count == 0) {
			lunatico_close(device);
		}
		indigo_send_message(device, OK_PROPERTY, "Disconnected from %s", device->name);
		CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
	}
	indigo_aux_change_property(device, NULL, CONNECTION_PROPERTY);
	if (IS_CONNECTED) {
		indigo_execute_handler(device, aux_third_timer_callback);
	}
}

static void aux_third_aux_outlet_names_third_handler(indigo_device *device) {
	AUX_OUTLET_NAMES_THIRD_PROPERTY->state = INDIGO_OK_STATE;
	//+ aux_third.AUX_OUTLET_NAMES_THIRD.on_change
	lunatico_apply_names(device, AUX_OUTLET_NAMES_THIRD_PROPERTY, AUX_POWER_OUTLET_THIRD_PROPERTY);
	//- aux_third.AUX_OUTLET_NAMES_THIRD.on_change
	indigo_update_property(device, AUX_OUTLET_NAMES_THIRD_PROPERTY, NULL);
}

static void aux_third_aux_power_outlet_third_handler(indigo_device *device) {
	AUX_POWER_OUTLET_THIRD_PROPERTY->state = INDIGO_OK_STATE;
	//+ aux_third.AUX_POWER_OUTLET_THIRD.on_change
	if (!lunatico_apply_outlets(device, AUX_POWER_OUTLET_THIRD_PROPERTY)) {
		AUX_POWER_OUTLET_THIRD_PROPERTY->state = INDIGO_ALERT_STATE;
	}
	//- aux_third.AUX_POWER_OUTLET_THIRD.on_change
	indigo_update_property(device, AUX_POWER_OUTLET_THIRD_PROPERTY, NULL);
}

static void aux_third_aux_sensor_names_third_handler(indigo_device *device) {
	AUX_SENSOR_NAMES_THIRD_PROPERTY->state = INDIGO_OK_STATE;
	//+ aux_third.AUX_SENSOR_NAMES_THIRD.on_change
	lunatico_apply_names(device, AUX_SENSOR_NAMES_THIRD_PROPERTY, AUX_GPIO_SENSORS_THIRD_PROPERTY);
	//- aux_third.AUX_SENSOR_NAMES_THIRD.on_change
	indigo_update_property(device, AUX_SENSOR_NAMES_THIRD_PROPERTY, NULL);
}

#pragma mark - Device API (aux_third)

static indigo_result aux_third_enumerate_properties(indigo_device *device, indigo_client *client, indigo_property *property);

static indigo_result aux_third_attach(indigo_device *device) {
	if (indigo_aux_attach(device, DRIVER_NAME, DRIVER_VERSION, INDIGO_INTERFACE_AUX_GPIO | INDIGO_INTERFACE_AUX_POWERBOX) == INDIGO_OK) {
		//+ aux_third.on_attach
		device->gp_bits = 2;
		INFO_PROPERTY->count = 6;
		//- aux_third.on_attach
		AUX_OUTLET_NAMES_THIRD_PROPERTY = indigo_init_text_property(NULL, device->name, AUX_OUTLET_NAMES_PROPERTY_NAME, AUX_POWERBOX_GROUP, "Power outlet names", INDIGO_OK_STATE, INDIGO_RW_PERM, 4);
		if (AUX_OUTLET_NAMES_THIRD_PROPERTY == NULL) {
			return INDIGO_FAILED;
		}
		indigo_init_text_item(AUX_OUTLET_NAME_THIRD_1_ITEM, AUX_POWER_OUTLET_NAME_1_ITEM_NAME, "DB9 Pin 1", "Power #1");
		indigo_init_text_item(AUX_OUTLET_NAME_THIRD_2_ITEM, AUX_POWER_OUTLET_NAME_2_ITEM_NAME, "DB9 Pin 2", "Power #2");
		indigo_init_text_item(AUX_OUTLET_NAME_THIRD_3_ITEM, AUX_POWER_OUTLET_NAME_3_ITEM_NAME, "DB9 Pin 3", "Power #3");
		indigo_init_text_item(AUX_OUTLET_NAME_THIRD_4_ITEM, AUX_POWER_OUTLET_NAME_4_ITEM_NAME, "DB9 Pin 4", "Power #4");
		AUX_POWER_OUTLET_THIRD_PROPERTY = indigo_init_switch_property(NULL, device->name, AUX_POWER_OUTLET_PROPERTY_NAME, AUX_POWERBOX_GROUP, "Power outlets", INDIGO_OK_STATE, INDIGO_RW_PERM, INDIGO_ANY_OF_MANY_RULE, 4);
		if (AUX_POWER_OUTLET_THIRD_PROPERTY == NULL) {
			return INDIGO_FAILED;
		}
		indigo_init_switch_item(AUX_POWER_OUTLET_THIRD_1_ITEM, AUX_POWER_OUTLET_1_ITEM_NAME, "Power #1", false);
		indigo_init_switch_item(AUX_POWER_OUTLET_THIRD_2_ITEM, AUX_POWER_OUTLET_2_ITEM_NAME, "Power #2", false);
		indigo_init_switch_item(AUX_POWER_OUTLET_THIRD_3_ITEM, AUX_POWER_OUTLET_3_ITEM_NAME, "Power #3", false);
		indigo_init_switch_item(AUX_POWER_OUTLET_THIRD_4_ITEM, AUX_POWER_OUTLET_4_ITEM_NAME, "Power #4", false);
		AUX_SENSOR_NAMES_THIRD_PROPERTY = indigo_init_text_property(NULL, device->name, AUX_SENSOR_NAMES_PROPERTY_NAME, AUX_SENSORS_GROUP, "Sensor names", INDIGO_OK_STATE, INDIGO_RW_PERM, 4);
		if (AUX_SENSOR_NAMES_THIRD_PROPERTY == NULL) {
			return INDIGO_FAILED;
		}
		indigo_init_text_item(AUX_SENSOR_NAME_THIRD_1_ITEM, AUX_GPIO_SENSOR_NAME_1_ITEM_NAME, "DB9 Pin 6", "Sensor #1");
		indigo_init_text_item(AUX_SENSOR_NAME_THIRD_2_ITEM, AUX_GPIO_SENSOR_NAME_2_ITEM_NAME, "DB9 Pin 7", "Sensor #2");
		indigo_init_text_item(AUX_SENSOR_NAME_THIRD_3_ITEM, AUX_GPIO_SENSOR_NAME_3_ITEM_NAME, "DB9 Pin 8", "Sensor #3");
		indigo_init_text_item(AUX_SENSOR_NAME_THIRD_4_ITEM, AUX_GPIO_SENSOR_NAME_4_ITEM_NAME, "DB9 Pin 9", "Sensor #4");
		AUX_GPIO_SENSORS_THIRD_PROPERTY = indigo_init_number_property(NULL, device->name, AUX_GPIO_SENSORS_PROPERTY_NAME, AUX_SENSORS_GROUP, "GPIO sensors", INDIGO_OK_STATE, INDIGO_RO_PERM, 4);
		if (AUX_GPIO_SENSORS_THIRD_PROPERTY == NULL) {
			return INDIGO_FAILED;
		}
		indigo_init_number_item(AUX_GPIO_SENSOR_THIRD_1_ITEM, AUX_GPIO_SENSOR_NAME_1_ITEM_NAME, "Sensor #1", 0, 1024, 1, 0);
		indigo_init_number_item(AUX_GPIO_SENSOR_THIRD_2_ITEM, AUX_GPIO_SENSOR_NAME_2_ITEM_NAME, "Sensor #2", 0, 1024, 1, 0);
		indigo_init_number_item(AUX_GPIO_SENSOR_THIRD_3_ITEM, AUX_GPIO_SENSOR_NAME_3_ITEM_NAME, "Sensor #3", 0, 1024, 1, 0);
		indigo_init_number_item(AUX_GPIO_SENSOR_THIRD_4_ITEM, AUX_GPIO_SENSOR_NAME_4_ITEM_NAME, "Sensor #4", 0, 1024, 1, 0);
		INDIGO_DEVICE_ATTACH_LOG(DRIVER_NAME, device->name);
		return aux_third_enumerate_properties(device, NULL, NULL);
	}
	return INDIGO_FAILED;
}

static indigo_result aux_third_enumerate_properties(indigo_device *device, indigo_client *client, indigo_property *property) {
	if (IS_CONNECTED) {
		INDIGO_DEFINE_MATCHING_PROPERTY(AUX_POWER_OUTLET_THIRD_PROPERTY);
		INDIGO_DEFINE_MATCHING_PROPERTY(AUX_GPIO_SENSORS_THIRD_PROPERTY);
	}
	INDIGO_DEFINE_MATCHING_PROPERTY(AUX_OUTLET_NAMES_THIRD_PROPERTY);
	INDIGO_DEFINE_MATCHING_PROPERTY(AUX_SENSOR_NAMES_THIRD_PROPERTY);
	return indigo_aux_enumerate_properties(device, client, property);
}

static indigo_result aux_third_change_property(indigo_device *device, indigo_client *client, indigo_property *property) {
	if (indigo_property_match_changeable(CONNECTION_PROPERTY, property)) {
		INDIGO_PROCESS_CONNECT(aux_third_connection_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(AUX_OUTLET_NAMES_THIRD_PROPERTY, property)) {
		INDIGO_COPY_VALUES_PROCESS_CHANGE(AUX_OUTLET_NAMES_THIRD_PROPERTY, aux_third_aux_outlet_names_third_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(AUX_POWER_OUTLET_THIRD_PROPERTY, property)) {
		INDIGO_COPY_VALUES_PROCESS_CHANGE(AUX_POWER_OUTLET_THIRD_PROPERTY, aux_third_aux_power_outlet_third_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(AUX_SENSOR_NAMES_THIRD_PROPERTY, property)) {
		INDIGO_COPY_VALUES_PROCESS_CHANGE(AUX_SENSOR_NAMES_THIRD_PROPERTY, aux_third_aux_sensor_names_third_handler);
		return INDIGO_OK;
	} else if (indigo_property_match(CONFIG_PROPERTY, property)) {
		if (indigo_switch_match(CONFIG_SAVE_ITEM, property)) {
			indigo_save_property(device, NULL, AUX_OUTLET_NAMES_THIRD_PROPERTY);
			indigo_save_property(device, NULL, AUX_SENSOR_NAMES_THIRD_PROPERTY);
		}
	}
	return indigo_aux_change_property(device, client, property);
}

static indigo_result aux_third_detach(indigo_device *device) {
	if (IS_CONNECTED) {
		indigo_set_switch(CONNECTION_PROPERTY, CONNECTION_DISCONNECTED_ITEM, true);
		aux_third_connection_handler(device);
	}
	indigo_release_property(AUX_OUTLET_NAMES_THIRD_PROPERTY);
	indigo_release_property(AUX_POWER_OUTLET_THIRD_PROPERTY);
	indigo_release_property(AUX_SENSOR_NAMES_THIRD_PROPERTY);
	indigo_release_property(AUX_GPIO_SENSORS_THIRD_PROPERTY);
	INDIGO_DEVICE_DETACH_LOG(DRIVER_NAME, device->name);
	return indigo_aux_detach(device);
}

#pragma mark - Device templates

static indigo_device rotator_main_template = INDIGO_DEVICE_INITIALIZER(ROTATOR_MAIN_DEVICE_NAME, rotator_main_attach, rotator_main_enumerate_properties, rotator_main_change_property, NULL, rotator_main_detach);

static indigo_device focuser_exp_template = INDIGO_DEVICE_INITIALIZER(FOCUSER_EXP_DEVICE_NAME, focuser_exp_attach, focuser_exp_enumerate_properties, focuser_exp_change_property, NULL, focuser_exp_detach);

static indigo_device rotator_exp_template = INDIGO_DEVICE_INITIALIZER(ROTATOR_EXP_DEVICE_NAME, rotator_exp_attach, rotator_exp_enumerate_properties, rotator_exp_change_property, NULL, rotator_exp_detach);

static indigo_device aux_exp_template = INDIGO_DEVICE_INITIALIZER(AUX_EXP_DEVICE_NAME, aux_exp_attach, aux_exp_enumerate_properties, aux_exp_change_property, NULL, aux_exp_detach);

static indigo_device focuser_third_template = INDIGO_DEVICE_INITIALIZER(FOCUSER_THIRD_DEVICE_NAME, focuser_third_attach, focuser_third_enumerate_properties, focuser_third_change_property, NULL, focuser_third_detach);

static indigo_device rotator_third_template = INDIGO_DEVICE_INITIALIZER(ROTATOR_THIRD_DEVICE_NAME, rotator_third_attach, rotator_third_enumerate_properties, rotator_third_change_property, NULL, rotator_third_detach);

static indigo_device aux_third_template = INDIGO_DEVICE_INITIALIZER(AUX_THIRD_DEVICE_NAME, aux_third_attach, aux_third_enumerate_properties, aux_third_change_property, NULL, aux_third_detach);

#pragma mark - Main code

indigo_result indigo_rotator_lunatico(indigo_driver_action action, indigo_driver_info *info) {
	static indigo_driver_action last_action = INDIGO_DRIVER_SHUTDOWN;
	static lunatico_private_data *private_data = NULL;
	static indigo_device *rotator_main = NULL;
	static indigo_device *focuser_exp = NULL;
	static indigo_device *rotator_exp = NULL;
	static indigo_device *aux_exp = NULL;
	static indigo_device *focuser_third = NULL;
	static indigo_device *rotator_third = NULL;
	static indigo_device *aux_third = NULL;

	SET_DRIVER_INFO(info, DRIVER_LABEL, __FUNCTION__, DRIVER_VERSION, true, last_action);

	if (action == last_action) {
		return INDIGO_OK;
	}

	switch (action) {
		case INDIGO_DRIVER_INIT: {
			last_action = action;
			//+ on_init
			// Both drivers drive the same controller over the same connection, so
			// they must not be loaded at the same time.
			if (indigo_driver_initialized(CONFLICTING_DRIVER)) {
				INDIGO_DRIVER_ERROR(DRIVER_NAME, "Conflicting driver %s is already loaded", CONFLICTING_DRIVER);
				last_action = INDIGO_DRIVER_SHUTDOWN;
				return INDIGO_FAILED;
			}
			//- on_init
			private_data = (lunatico_private_data *)indigo_safe_malloc(sizeof(lunatico_private_data));
			rotator_main = (indigo_device *)indigo_safe_malloc_copy(sizeof(indigo_device), &rotator_main_template);
			rotator_main->private_data = private_data;
			indigo_attach_device(rotator_main);
			focuser_exp = (indigo_device *)indigo_safe_malloc_copy(sizeof(indigo_device), &focuser_exp_template);
			focuser_exp->private_data = private_data;
			focuser_exp->master_device = rotator_main;
			indigo_attach_device(focuser_exp);
			rotator_exp = (indigo_device *)indigo_safe_malloc_copy(sizeof(indigo_device), &rotator_exp_template);
			rotator_exp->private_data = private_data;
			rotator_exp->master_device = rotator_main;
			indigo_attach_device(rotator_exp);
			aux_exp = (indigo_device *)indigo_safe_malloc_copy(sizeof(indigo_device), &aux_exp_template);
			aux_exp->private_data = private_data;
			aux_exp->master_device = rotator_main;
			indigo_attach_device(aux_exp);
			focuser_third = (indigo_device *)indigo_safe_malloc_copy(sizeof(indigo_device), &focuser_third_template);
			focuser_third->private_data = private_data;
			focuser_third->master_device = rotator_main;
			indigo_attach_device(focuser_third);
			rotator_third = (indigo_device *)indigo_safe_malloc_copy(sizeof(indigo_device), &rotator_third_template);
			rotator_third->private_data = private_data;
			rotator_third->master_device = rotator_main;
			indigo_attach_device(rotator_third);
			aux_third = (indigo_device *)indigo_safe_malloc_copy(sizeof(indigo_device), &aux_third_template);
			aux_third->private_data = private_data;
			aux_third->master_device = rotator_main;
			indigo_attach_device(aux_third);
			break;

		}
		case INDIGO_DRIVER_SHUTDOWN: {
			VERIFY_NOT_CONNECTED(rotator_main);
			VERIFY_NOT_CONNECTED(focuser_exp);
			VERIFY_NOT_CONNECTED(rotator_exp);
			VERIFY_NOT_CONNECTED(aux_exp);
			VERIFY_NOT_CONNECTED(focuser_third);
			VERIFY_NOT_CONNECTED(rotator_third);
			VERIFY_NOT_CONNECTED(aux_third);
			last_action = action;
			if (aux_third != NULL) {
				indigo_detach_device(aux_third);
				indigo_safe_free(aux_third);
				aux_third = NULL;
			}
			if (rotator_third != NULL) {
				indigo_detach_device(rotator_third);
				indigo_safe_free(rotator_third);
				rotator_third = NULL;
			}
			if (focuser_third != NULL) {
				indigo_detach_device(focuser_third);
				indigo_safe_free(focuser_third);
				focuser_third = NULL;
			}
			if (aux_exp != NULL) {
				indigo_detach_device(aux_exp);
				indigo_safe_free(aux_exp);
				aux_exp = NULL;
			}
			if (rotator_exp != NULL) {
				indigo_detach_device(rotator_exp);
				indigo_safe_free(rotator_exp);
				rotator_exp = NULL;
			}
			if (focuser_exp != NULL) {
				indigo_detach_device(focuser_exp);
				indigo_safe_free(focuser_exp);
				focuser_exp = NULL;
			}
			if (rotator_main != NULL) {
				indigo_detach_device(rotator_main);
				indigo_safe_free(rotator_main);
				rotator_main = NULL;
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
