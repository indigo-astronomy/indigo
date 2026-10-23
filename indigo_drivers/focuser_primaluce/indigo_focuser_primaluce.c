// Copyright (c) 2023-2026 CloudMakers, s. r. o.
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

// This file generated from indigo_focuser_primaluce.driver

#pragma mark - Includes

#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <assert.h>

//+ include

#include "../shared/primalucelib/primalucelib.h"

//- include

#include <indigo/indigo_driver_xml.h>
#include <indigo/indigo_focuser_driver.h>
#include <indigo/indigo_rotator_driver.h>
#include <indigo/indigo_uni_io.h>

#include "indigo_focuser_primaluce.h"

#pragma mark - Common definitions

#define DRIVER_VERSION       0x03000023
#define DRIVER_NAME          "indigo_focuser_primaluce"
#define DRIVER_LABEL         "PrimaluceLab Focuser/Rotator"
#define FOCUSER_DEVICE_NAME  "PrimaluceLab Focuser"
#define ROTATOR_DEVICE_NAME  "PrimaluceLab Rotator"
#define PRIVATE_DATA         ((primaluce_private_data *)device->private_data)

//+ define

// The controller connection, see indigo_drivers/shared/primalucelib (linked as a static library).
#define LINK                 (&PRIVATE_DATA->link)
// Idle position poll period; the controller state (temperature, supplies) is read every ENVIRONMENT_TICKS polls.
#define POLL_PERIOD          2
#define ENVIRONMENT_TICKS    5
// A motor that reports running without advancing for this many 0.2 s polls has stalled.
#define STALL_POLLS          25
// Consecutive failed position reads tolerated during a move.
#define MAX_POLL_FAILURES    3
// A rotator angle this close to the target (degrees) has reached it; a SESTO SENSO 3 stops a move to 15 at 14.99994.
#define ROTATOR_POSITION_TOLERANCE 0.1
// The controller needs this long between calibration steps (seconds).
#define CALIBRATION_STEP_INTERVAL 1.0
// After END the new range is polled this often, this far apart (seconds).
#define CALIBRATION_RANGE_POLLS 20
#define CALIBRATION_RANGE_POLL_INTERVAL 0.25

//- define

#pragma mark - Property definitions

#define X_CONFIG_PROPERTY              (PRIVATE_DATA->x_config_property)
#define X_CONFIG_M1ACC_ITEM            (X_CONFIG_PROPERTY->items + 0)
#define X_CONFIG_M1SPD_ITEM            (X_CONFIG_PROPERTY->items + 1)
#define X_CONFIG_M1DEC_ITEM            (X_CONFIG_PROPERTY->items + 2)
#define X_CONFIG_M1CACC_ITEM           (X_CONFIG_PROPERTY->items + 3)
#define X_CONFIG_M1CSPD_ITEM           (X_CONFIG_PROPERTY->items + 4)
#define X_CONFIG_M1CDEC_ITEM           (X_CONFIG_PROPERTY->items + 5)
#define X_CONFIG_M1HOLD_ITEM           (X_CONFIG_PROPERTY->items + 6)

#define X_CONFIG_PROPERTY_NAME         "X_CONFIG"
#define X_CONFIG_M1ACC_ITEM_NAME       "M1ACC"
#define X_CONFIG_M1SPD_ITEM_NAME       "M1SPD"
#define X_CONFIG_M1DEC_ITEM_NAME       "M1DEC"
#define X_CONFIG_M1CACC_ITEM_NAME      "M1CACC"
#define X_CONFIG_M1CSPD_ITEM_NAME      "M1CSPD"
#define X_CONFIG_M1CDEC_ITEM_NAME      "M1CDEC"
#define X_CONFIG_M1HOLD_ITEM_NAME      "M1HOLD"

#define X_STATE_PROPERTY               (PRIVATE_DATA->x_state_property)
#define X_STATE_MOTOR_TEMP_ITEM        (X_STATE_PROPERTY->items + 0)
#define X_STATE_VIN_12V_ITEM           (X_STATE_PROPERTY->items + 1)
#define X_STATE_VIN_USB_ITEM           (X_STATE_PROPERTY->items + 2)

#define X_STATE_PROPERTY_NAME          "X_STATE"
#define X_STATE_MOTOR_TEMP_ITEM_NAME   "MOTOR_TEMP"
#define X_STATE_VIN_12V_ITEM_NAME      "VIN_12V"
#define X_STATE_VIN_USB_ITEM_NAME      "VIN_USB"

#define X_WIFI_PROPERTY                (PRIVATE_DATA->x_wifi_property)
#define X_WIFI_OFF_ITEM                (X_WIFI_PROPERTY->items + 0)
#define X_WIFI_AP_ITEM                 (X_WIFI_PROPERTY->items + 1)
#define X_WIFI_STA_ITEM                (X_WIFI_PROPERTY->items + 2)

#define X_WIFI_PROPERTY_NAME           "X_WIFI"
#define X_WIFI_OFF_ITEM_NAME           "OFF"
#define X_WIFI_AP_ITEM_NAME            "AP"
#define X_WIFI_STA_ITEM_NAME           "STA"

#define X_WIFI_AP_PROPERTY             (PRIVATE_DATA->x_wifi_ap_property)
#define X_WIFI_AP_SSID_ITEM            (X_WIFI_AP_PROPERTY->items + 0)
#define X_WIFI_AP_PASSWORD_ITEM        (X_WIFI_AP_PROPERTY->items + 1)

#define X_WIFI_AP_PROPERTY_NAME        "X_WIFI_AP"
#define X_WIFI_AP_SSID_ITEM_NAME       "AP_SSID"
#define X_WIFI_AP_PASSWORD_ITEM_NAME   "AP_PASSWORD"

#define X_WIFI_STA_PROPERTY            (PRIVATE_DATA->x_wifi_sta_property)
#define X_WIFI_STA_SSID_ITEM           (X_WIFI_STA_PROPERTY->items + 0)
#define X_WIFI_STA_PASSWORD_ITEM       (X_WIFI_STA_PROPERTY->items + 1)

#define X_WIFI_STA_PROPERTY_NAME       "X_WIFI_STA"
#define X_WIFI_STA_SSID_ITEM_NAME      "STA_SSID"
#define X_WIFI_STA_PASSWORD_ITEM_NAME  "STA_PASSWORD"

#define X_LEDS_PROPERTY                (PRIVATE_DATA->x_leds_property)
#define X_LEDS_OFF_ITEM                (X_LEDS_PROPERTY->items + 0)
#define X_LEDS_DIM_ITEM                (X_LEDS_PROPERTY->items + 1)
#define X_LEDS_MIDDLE_ITEM             (X_LEDS_PROPERTY->items + 2)
#define X_LEDS_ON_ITEM                 (X_LEDS_PROPERTY->items + 3)

#define X_LEDS_PROPERTY_NAME           "X_LEDS"
#define X_LEDS_OFF_ITEM_NAME           "OFF"
#define X_LEDS_DIM_ITEM_NAME           "DIM"
#define X_LEDS_MIDDLE_ITEM_NAME        "MIDDLE"
#define X_LEDS_ON_ITEM_NAME            "ON"

#define X_RUNPRESET_L_PROPERTY         (PRIVATE_DATA->x_runpreset_l_property)
#define X_RUNPRESET_L_M1ACC_ITEM       (X_RUNPRESET_L_PROPERTY->items + 0)
#define X_RUNPRESET_L_M1SPD_ITEM       (X_RUNPRESET_L_PROPERTY->items + 1)
#define X_RUNPRESET_L_M1DEC_ITEM       (X_RUNPRESET_L_PROPERTY->items + 2)
#define X_RUNPRESET_L_M1CACC_ITEM      (X_RUNPRESET_L_PROPERTY->items + 3)
#define X_RUNPRESET_L_M1CSPD_ITEM      (X_RUNPRESET_L_PROPERTY->items + 4)
#define X_RUNPRESET_L_M1CDEC_ITEM      (X_RUNPRESET_L_PROPERTY->items + 5)
#define X_RUNPRESET_L_M1HOLD_ITEM      (X_RUNPRESET_L_PROPERTY->items + 6)

#define X_RUNPRESET_L_PROPERTY_NAME    "X_RUNPRESET_L"
#define X_RUNPRESET_L_M1ACC_ITEM_NAME  "M1ACC"
#define X_RUNPRESET_L_M1SPD_ITEM_NAME  "M1SPD"
#define X_RUNPRESET_L_M1DEC_ITEM_NAME  "M1DEC"
#define X_RUNPRESET_L_M1CACC_ITEM_NAME "M1CACC"
#define X_RUNPRESET_L_M1CSPD_ITEM_NAME "M1CSPD"
#define X_RUNPRESET_L_M1CDEC_ITEM_NAME "M1CDEC"
#define X_RUNPRESET_L_M1HOLD_ITEM_NAME "M1HOLD"

#define X_RUNPRESET_M_PROPERTY         (PRIVATE_DATA->x_runpreset_m_property)
#define X_RUNPRESET_M_M1ACC_ITEM       (X_RUNPRESET_M_PROPERTY->items + 0)
#define X_RUNPRESET_M_M1SPD_ITEM       (X_RUNPRESET_M_PROPERTY->items + 1)
#define X_RUNPRESET_M_M1DEC_ITEM       (X_RUNPRESET_M_PROPERTY->items + 2)
#define X_RUNPRESET_M_M1CACC_ITEM      (X_RUNPRESET_M_PROPERTY->items + 3)
#define X_RUNPRESET_M_M1CSPD_ITEM      (X_RUNPRESET_M_PROPERTY->items + 4)
#define X_RUNPRESET_M_M1CDEC_ITEM      (X_RUNPRESET_M_PROPERTY->items + 5)
#define X_RUNPRESET_M_M1HOLD_ITEM      (X_RUNPRESET_M_PROPERTY->items + 6)

#define X_RUNPRESET_M_PROPERTY_NAME    "X_RUNPRESET_M"
#define X_RUNPRESET_M_M1ACC_ITEM_NAME  "M1ACC"
#define X_RUNPRESET_M_M1SPD_ITEM_NAME  "M1SPD"
#define X_RUNPRESET_M_M1DEC_ITEM_NAME  "M1DEC"
#define X_RUNPRESET_M_M1CACC_ITEM_NAME "M1CACC"
#define X_RUNPRESET_M_M1CSPD_ITEM_NAME "M1CSPD"
#define X_RUNPRESET_M_M1CDEC_ITEM_NAME "M1CDEC"
#define X_RUNPRESET_M_M1HOLD_ITEM_NAME "M1HOLD"

#define X_RUNPRESET_S_PROPERTY         (PRIVATE_DATA->x_runpreset_s_property)
#define X_RUNPRESET_S_M1ACC_ITEM       (X_RUNPRESET_S_PROPERTY->items + 0)
#define X_RUNPRESET_S_M1SPD_ITEM       (X_RUNPRESET_S_PROPERTY->items + 1)
#define X_RUNPRESET_S_M1DEC_ITEM       (X_RUNPRESET_S_PROPERTY->items + 2)
#define X_RUNPRESET_S_M1CACC_ITEM      (X_RUNPRESET_S_PROPERTY->items + 3)
#define X_RUNPRESET_S_M1CSPD_ITEM      (X_RUNPRESET_S_PROPERTY->items + 4)
#define X_RUNPRESET_S_M1CDEC_ITEM      (X_RUNPRESET_S_PROPERTY->items + 5)
#define X_RUNPRESET_S_M1HOLD_ITEM      (X_RUNPRESET_S_PROPERTY->items + 6)

#define X_RUNPRESET_S_PROPERTY_NAME    "X_RUNPRESET_S"
#define X_RUNPRESET_S_M1ACC_ITEM_NAME  "M1ACC"
#define X_RUNPRESET_S_M1SPD_ITEM_NAME  "M1SPD"
#define X_RUNPRESET_S_M1DEC_ITEM_NAME  "M1DEC"
#define X_RUNPRESET_S_M1CACC_ITEM_NAME "M1CACC"
#define X_RUNPRESET_S_M1CSPD_ITEM_NAME "M1CSPD"
#define X_RUNPRESET_S_M1CDEC_ITEM_NAME "M1CDEC"
#define X_RUNPRESET_S_M1HOLD_ITEM_NAME "M1HOLD"

#define X_RUNPRESET_1_PROPERTY         (PRIVATE_DATA->x_runpreset_1_property)
#define X_RUNPRESET_1_M1ACC_ITEM       (X_RUNPRESET_1_PROPERTY->items + 0)
#define X_RUNPRESET_1_M1SPD_ITEM       (X_RUNPRESET_1_PROPERTY->items + 1)
#define X_RUNPRESET_1_M1DEC_ITEM       (X_RUNPRESET_1_PROPERTY->items + 2)
#define X_RUNPRESET_1_M1CACC_ITEM      (X_RUNPRESET_1_PROPERTY->items + 3)
#define X_RUNPRESET_1_M1CSPD_ITEM      (X_RUNPRESET_1_PROPERTY->items + 4)
#define X_RUNPRESET_1_M1CDEC_ITEM      (X_RUNPRESET_1_PROPERTY->items + 5)
#define X_RUNPRESET_1_M1HOLD_ITEM      (X_RUNPRESET_1_PROPERTY->items + 6)

#define X_RUNPRESET_1_PROPERTY_NAME    "X_RUNPRESET_1"
#define X_RUNPRESET_1_M1ACC_ITEM_NAME  "M1ACC"
#define X_RUNPRESET_1_M1SPD_ITEM_NAME  "M1SPD"
#define X_RUNPRESET_1_M1DEC_ITEM_NAME  "M1DEC"
#define X_RUNPRESET_1_M1CACC_ITEM_NAME "M1CACC"
#define X_RUNPRESET_1_M1CSPD_ITEM_NAME "M1CSPD"
#define X_RUNPRESET_1_M1CDEC_ITEM_NAME "M1CDEC"
#define X_RUNPRESET_1_M1HOLD_ITEM_NAME "M1HOLD"

#define X_RUNPRESET_2_PROPERTY         (PRIVATE_DATA->x_runpreset_2_property)
#define X_RUNPRESET_2_M1ACC_ITEM       (X_RUNPRESET_2_PROPERTY->items + 0)
#define X_RUNPRESET_2_M1SPD_ITEM       (X_RUNPRESET_2_PROPERTY->items + 1)
#define X_RUNPRESET_2_M1DEC_ITEM       (X_RUNPRESET_2_PROPERTY->items + 2)
#define X_RUNPRESET_2_M1CACC_ITEM      (X_RUNPRESET_2_PROPERTY->items + 3)
#define X_RUNPRESET_2_M1CSPD_ITEM      (X_RUNPRESET_2_PROPERTY->items + 4)
#define X_RUNPRESET_2_M1CDEC_ITEM      (X_RUNPRESET_2_PROPERTY->items + 5)
#define X_RUNPRESET_2_M1HOLD_ITEM      (X_RUNPRESET_2_PROPERTY->items + 6)

#define X_RUNPRESET_2_PROPERTY_NAME    "X_RUNPRESET_2"
#define X_RUNPRESET_2_M1ACC_ITEM_NAME  "M1ACC"
#define X_RUNPRESET_2_M1SPD_ITEM_NAME  "M1SPD"
#define X_RUNPRESET_2_M1DEC_ITEM_NAME  "M1DEC"
#define X_RUNPRESET_2_M1CACC_ITEM_NAME "M1CACC"
#define X_RUNPRESET_2_M1CSPD_ITEM_NAME "M1CSPD"
#define X_RUNPRESET_2_M1CDEC_ITEM_NAME "M1CDEC"
#define X_RUNPRESET_2_M1HOLD_ITEM_NAME "M1HOLD"

#define X_RUNPRESET_3_PROPERTY         (PRIVATE_DATA->x_runpreset_3_property)
#define X_RUNPRESET_3_M1ACC_ITEM       (X_RUNPRESET_3_PROPERTY->items + 0)
#define X_RUNPRESET_3_M1SPD_ITEM       (X_RUNPRESET_3_PROPERTY->items + 1)
#define X_RUNPRESET_3_M1DEC_ITEM       (X_RUNPRESET_3_PROPERTY->items + 2)
#define X_RUNPRESET_3_M1CACC_ITEM      (X_RUNPRESET_3_PROPERTY->items + 3)
#define X_RUNPRESET_3_M1CSPD_ITEM      (X_RUNPRESET_3_PROPERTY->items + 4)
#define X_RUNPRESET_3_M1CDEC_ITEM      (X_RUNPRESET_3_PROPERTY->items + 5)
#define X_RUNPRESET_3_M1HOLD_ITEM      (X_RUNPRESET_3_PROPERTY->items + 6)

#define X_RUNPRESET_3_PROPERTY_NAME    "X_RUNPRESET_3"
#define X_RUNPRESET_3_M1ACC_ITEM_NAME  "M1ACC"
#define X_RUNPRESET_3_M1SPD_ITEM_NAME  "M1SPD"
#define X_RUNPRESET_3_M1DEC_ITEM_NAME  "M1DEC"
#define X_RUNPRESET_3_M1CACC_ITEM_NAME "M1CACC"
#define X_RUNPRESET_3_M1CSPD_ITEM_NAME "M1CSPD"
#define X_RUNPRESET_3_M1CDEC_ITEM_NAME "M1CDEC"
#define X_RUNPRESET_3_M1HOLD_ITEM_NAME "M1HOLD"

#define X_RUNPRESET_PROPERTY           (PRIVATE_DATA->x_runpreset_property)
#define X_RUNPRESET_L_ITEM             (X_RUNPRESET_PROPERTY->items + 0)
#define X_RUNPRESET_M_ITEM             (X_RUNPRESET_PROPERTY->items + 1)
#define X_RUNPRESET_S_ITEM             (X_RUNPRESET_PROPERTY->items + 2)
#define X_RUNPRESET_1_ITEM             (X_RUNPRESET_PROPERTY->items + 3)
#define X_RUNPRESET_2_ITEM             (X_RUNPRESET_PROPERTY->items + 4)
#define X_RUNPRESET_3_ITEM             (X_RUNPRESET_PROPERTY->items + 5)

#define X_RUNPRESET_PROPERTY_NAME      "X_RUNPRESET"
#define X_RUNPRESET_L_ITEM_NAME        "L"
#define X_RUNPRESET_M_ITEM_NAME        "M"
#define X_RUNPRESET_S_ITEM_NAME        "S"
#define X_RUNPRESET_1_ITEM_NAME        "1"
#define X_RUNPRESET_2_ITEM_NAME        "2"
#define X_RUNPRESET_3_ITEM_NAME        "3"

#define X_PRESETS_PROPERTY             (PRIVATE_DATA->x_presets_property)
#define X_PRESETS_1_ITEM               (X_PRESETS_PROPERTY->items + 0)
#define X_PRESETS_2_ITEM               (X_PRESETS_PROPERTY->items + 1)
#define X_PRESETS_3_ITEM               (X_PRESETS_PROPERTY->items + 2)
#define X_PRESETS_4_ITEM               (X_PRESETS_PROPERTY->items + 3)
#define X_PRESETS_5_ITEM               (X_PRESETS_PROPERTY->items + 4)
#define X_PRESETS_6_ITEM               (X_PRESETS_PROPERTY->items + 5)
#define X_PRESETS_7_ITEM               (X_PRESETS_PROPERTY->items + 6)
#define X_PRESETS_8_ITEM               (X_PRESETS_PROPERTY->items + 7)
#define X_PRESETS_9_ITEM               (X_PRESETS_PROPERTY->items + 8)

#define X_PRESETS_PROPERTY_NAME        "X_PRESETS"
#define X_PRESETS_1_ITEM_NAME          "PRESET_1"
#define X_PRESETS_2_ITEM_NAME          "PRESET_2"
#define X_PRESETS_3_ITEM_NAME          "PRESET_3"
#define X_PRESETS_4_ITEM_NAME          "PRESET_4"
#define X_PRESETS_5_ITEM_NAME          "PRESET_5"
#define X_PRESETS_6_ITEM_NAME          "PRESET_6"
#define X_PRESETS_7_ITEM_NAME          "PRESET_7"
#define X_PRESETS_8_ITEM_NAME          "PRESET_8"
#define X_PRESETS_9_ITEM_NAME          "PRESET_9"

#define X_PRESET_NAMES_PROPERTY        (PRIVATE_DATA->x_preset_names_property)
#define X_PRESET_NAMES_1_ITEM          (X_PRESET_NAMES_PROPERTY->items + 0)
#define X_PRESET_NAMES_2_ITEM          (X_PRESET_NAMES_PROPERTY->items + 1)
#define X_PRESET_NAMES_3_ITEM          (X_PRESET_NAMES_PROPERTY->items + 2)
#define X_PRESET_NAMES_4_ITEM          (X_PRESET_NAMES_PROPERTY->items + 3)
#define X_PRESET_NAMES_5_ITEM          (X_PRESET_NAMES_PROPERTY->items + 4)
#define X_PRESET_NAMES_6_ITEM          (X_PRESET_NAMES_PROPERTY->items + 5)
#define X_PRESET_NAMES_7_ITEM          (X_PRESET_NAMES_PROPERTY->items + 6)
#define X_PRESET_NAMES_8_ITEM          (X_PRESET_NAMES_PROPERTY->items + 7)
#define X_PRESET_NAMES_9_ITEM          (X_PRESET_NAMES_PROPERTY->items + 8)

#define X_PRESET_NAMES_PROPERTY_NAME   "X_PRESET_NAMES"
#define X_PRESET_NAMES_1_ITEM_NAME     "PRESET_1"
#define X_PRESET_NAMES_2_ITEM_NAME     "PRESET_2"
#define X_PRESET_NAMES_3_ITEM_NAME     "PRESET_3"
#define X_PRESET_NAMES_4_ITEM_NAME     "PRESET_4"
#define X_PRESET_NAMES_5_ITEM_NAME     "PRESET_5"
#define X_PRESET_NAMES_6_ITEM_NAME     "PRESET_6"
#define X_PRESET_NAMES_7_ITEM_NAME     "PRESET_7"
#define X_PRESET_NAMES_8_ITEM_NAME     "PRESET_8"
#define X_PRESET_NAMES_9_ITEM_NAME     "PRESET_9"

#define X_PRESET_GOTO_PROPERTY         (PRIVATE_DATA->x_preset_goto_property)
#define X_PRESET_GOTO_1_ITEM           (X_PRESET_GOTO_PROPERTY->items + 0)
#define X_PRESET_GOTO_2_ITEM           (X_PRESET_GOTO_PROPERTY->items + 1)
#define X_PRESET_GOTO_3_ITEM           (X_PRESET_GOTO_PROPERTY->items + 2)
#define X_PRESET_GOTO_4_ITEM           (X_PRESET_GOTO_PROPERTY->items + 3)
#define X_PRESET_GOTO_5_ITEM           (X_PRESET_GOTO_PROPERTY->items + 4)
#define X_PRESET_GOTO_6_ITEM           (X_PRESET_GOTO_PROPERTY->items + 5)
#define X_PRESET_GOTO_7_ITEM           (X_PRESET_GOTO_PROPERTY->items + 6)
#define X_PRESET_GOTO_8_ITEM           (X_PRESET_GOTO_PROPERTY->items + 7)
#define X_PRESET_GOTO_9_ITEM           (X_PRESET_GOTO_PROPERTY->items + 8)

#define X_PRESET_GOTO_PROPERTY_NAME    "X_PRESET_GOTO"
#define X_PRESET_GOTO_1_ITEM_NAME      "PRESET_1"
#define X_PRESET_GOTO_2_ITEM_NAME      "PRESET_2"
#define X_PRESET_GOTO_3_ITEM_NAME      "PRESET_3"
#define X_PRESET_GOTO_4_ITEM_NAME      "PRESET_4"
#define X_PRESET_GOTO_5_ITEM_NAME      "PRESET_5"
#define X_PRESET_GOTO_6_ITEM_NAME      "PRESET_6"
#define X_PRESET_GOTO_7_ITEM_NAME      "PRESET_7"
#define X_PRESET_GOTO_8_ITEM_NAME      "PRESET_8"
#define X_PRESET_GOTO_9_ITEM_NAME      "PRESET_9"

#define X_HOLD_CURR_PROPERTY           (PRIVATE_DATA->x_hold_curr_property)
#define X_HOLD_CURR_OFF_ITEM           (X_HOLD_CURR_PROPERTY->items + 0)
#define X_HOLD_CURR_ON_ITEM            (X_HOLD_CURR_PROPERTY->items + 1)

#define X_HOLD_CURR_PROPERTY_NAME      "X_HOLD_CURR"
#define X_HOLD_CURR_OFF_ITEM_NAME      "OFF"
#define X_HOLD_CURR_ON_ITEM_NAME       "ON"

#define X_CALIBRATE_F_PROPERTY                 (PRIVATE_DATA->x_calibrate_f_property)
#define X_CALIBRATE_F_START_ITEM               (X_CALIBRATE_F_PROPERTY->items + 0)
#define X_CALIBRATE_F_START_INVERTED_ITEM      (X_CALIBRATE_F_PROPERTY->items + 1)
#define X_CALIBRATE_F_END_ITEM                 (X_CALIBRATE_F_PROPERTY->items + 2)

#define X_CALIBRATE_F_PROPERTY_NAME            "X_CALIBRATE"
#define X_CALIBRATE_F_START_ITEM_NAME          "START"
#define X_CALIBRATE_F_START_INVERTED_ITEM_NAME "START_INVERTED"
#define X_CALIBRATE_F_END_ITEM_NAME            "END"

#define X_CALIBRATE_R_PROPERTY         (PRIVATE_DATA->x_calibrate_r_property)
#define X_CALIBRATE_R_START_ITEM       (X_CALIBRATE_R_PROPERTY->items + 0)

#define X_CALIBRATE_R_PROPERTY_NAME    "X_CALIBRATE_A"
#define X_CALIBRATE_R_START_ITEM_NAME  "START"

#pragma mark - Private data definition

typedef struct {
	int count;
	indigo_uni_handle *handle;
	indigo_property *x_config_property;
	indigo_property *x_state_property;
	indigo_property *x_wifi_property;
	indigo_property *x_wifi_ap_property;
	indigo_property *x_wifi_sta_property;
	indigo_property *x_leds_property;
	indigo_property *x_runpreset_l_property;
	indigo_property *x_runpreset_m_property;
	indigo_property *x_runpreset_s_property;
	indigo_property *x_runpreset_1_property;
	indigo_property *x_runpreset_2_property;
	indigo_property *x_runpreset_3_property;
	indigo_property *x_runpreset_property;
	indigo_property *x_presets_property;
	indigo_property *x_preset_names_property;
	indigo_property *x_preset_goto_property;
	indigo_property *x_hold_curr_property;
	indigo_property *x_calibrate_f_property;
	indigo_property *x_calibrate_r_property;
	//+ data
	primalucelib_link link;
	bool has_abs_pos;
	bool rotator_has_abs_pos;
	bool rotator_has_position_deg;
	int preset_positions[PRIMALUCE_STORED_POSITION_COUNT];
	char preset_names[PRIMALUCE_STORED_POSITION_COUNT][INDIGO_VALUE_SIZE];
	int rotator_calibration_polls;
	bool is_sestosenso_3;
	// The controller flags a motor for calibration (CALRESTART); its moves may be acknowledged and not run.
	bool focuser_needs_calibration, rotator_needs_calibration;
	// The controller reports a travel range (CAL_MAXPOS above CAL_MINPOS).
	bool calibrated;
	double position, last_position;
	int stalled_polls, poll_failures, environment_ticks;
	bool external_motion, poll_alert, abort_requested, calibrating, calibration_pending, pending;
	// The focuser calibration in progress: the direction it uses, the next start step, the polls of the
	// current wait, the maximum before END, and whether the steps before the run are still pending.
	const char *calibration_direction;
	int calibration_step, calibration_polls;
	double calibration_previous_max;
	bool calibration_starting;
	int backlash, speed;
	indigo_item *leds_item, *hold_item, *wifi_item;
	//- data
} primaluce_private_data;

#pragma mark - Low level code

//+ code

static void focuser_position_handler(indigo_device *device);
static void focuser_steps_handler(indigo_device *device);
static void rotator_position_handler(indigo_device *device);

static indigo_item *selected_item(indigo_property *property) {
	for (int i = 0; i < property->count; i++) {
		if (property->items[i].sw.value) {
			return property->items + i;
		}
	}
	return NULL;
}

// A refused switch request keeps the item the device has.
static void restore_switch(indigo_property *property, indigo_item *item) {
	if (item != NULL) {
		indigo_set_switch(property, item, true);
	}
	property->state = INDIGO_ALERT_STATE;
}

// The firmware applies a new LANCFG only after a restart, so a changed mode restarts the controller.
static bool primaluce_set_lan_cfg(indigo_device *device, const char *mode) {
	char *text;
	if ((text = primalucelib_get_lan_config(LINK)) == NULL) {
		return false;
	}
	if (!strcmp(text, mode)) {
		return true;
	}
	if (!primalucelib_set_lan_config(LINK, mode) || !primalucelib_reboot(LINK)) {
		return false;
	}
	if (primalucelib_is_http(LINK)) {
		indigo_send_message(device, X_WIFI_PROPERTY, "The controller restarts to apply the WiFi mode and leaves its current network");
		return true;
	}
	return primalucelib_wait_for_restart(LINK);
}

static bool primaluce_open(indigo_device *device, const char **error_message) {
	if (primalucelib_open(LINK, DRIVER_NAME, DEVICE_PORT_ITEM->text.value, error_message)) {
		char *text;
		if ((text = primalucelib_get_model_name(LINK))) {
			primalucelib_model model = primalucelib_model_from_name(text);
			if (primalucelib_model_is_focuser(model)) {
				PRIVATE_DATA->is_sestosenso_3 = model == PRIMALUCELIB_MODEL_SESTOSENSO3;
				if ((text = primalucelib_get_firmware_version(LINK))) {
					double version = atof(text);
					if (!PRIVATE_DATA->is_sestosenso_3 && version < PRIMALUCE_MINIMUM_FIRMWARE_VERSION) {
						indigo_send_message(device, BUSY_PROPERTY, "%s has firmware version %.2f and at least 3.05 is needed", INFO_DEVICE_MODEL_ITEM->text.value, version);
					}
					// Diagnostic output would interleave with the replies on the serial line.
					primalucelib_disable_log_output(LINK);
					return true;
				} else {
					INDIGO_DRIVER_ERROR(DRIVER_NAME, "Unsupported version");
				}
			} else {
				INDIGO_DRIVER_ERROR(DRIVER_NAME, "Unsupported device");
			}
		}
		primalucelib_close(LINK);
	}
	return false;
}

static void primaluce_close(indigo_device *device) {
	if (primalucelib_is_open(LINK)) {
		primalucelib_close(LINK);
		INDIGO_COPY_VALUE(INFO_DEVICE_MODEL_ITEM->text.value, "N/A");
		INDIGO_COPY_VALUE(INFO_DEVICE_SERIAL_NUM_ITEM->text.value, "N/A");
		INDIGO_COPY_VALUE(INFO_DEVICE_FW_REVISION_ITEM->text.value, "N/A");
		indigo_update_property(device, INFO_PROPERTY, NULL);
		INDIGO_DRIVER_LOG(DRIVER_NAME, "Disconnected from %s", DEVICE_PORT_ITEM->text.value);
	}
}

//- code

//+ focuser.code

// X_CONFIG and the X_RUNPRESET_* properties list M1ACC, M1SPD, M1DEC, M1CACC, M1CSPD, M1CDEC and M1HOLD in this order.
static void set_run_config_items(indigo_property *property, primalucelib_run_config *config) {
	double values[] = { config->acceleration_speed, config->run_speed, config->deceleration_speed, config->acceleration_current, config->run_current, config->deceleration_current, config->hold_current };
	for (int i = 0; i < 7; i++) {
		property->items[i].number.value = property->items[i].number.target = values[i];
	}
}

static void get_run_config_targets(indigo_property *property, primalucelib_run_config *config) {
	config->acceleration_speed = property->items[0].number.target;
	config->run_speed = property->items[1].number.target;
	config->deceleration_speed = property->items[2].number.target;
	config->acceleration_current = property->items[3].number.target;
	config->run_current = property->items[4].number.target;
	config->deceleration_current = property->items[5].number.target;
	config->hold_current = property->items[6].number.target;
}
// The controller refuses to drive the motor without its 12 V supply and says so in the command reply.
static const char *power_message(const char *state) {
	return primalucelib_is_power_supply_error(state) ? "The motor has no 12 V power supply" : NULL;
}

// Both motion properties end the move ALERT with the position the focuser has as value and the
// request as target; message, when there is one, reaches the client.
static void focuser_motion_refused(indigo_device *device, const char *message) {
	PRIVATE_DATA->abort_requested = false;
	FOCUSER_STEPS_PROPERTY->state = INDIGO_ALERT_STATE;
	indigo_update_property(device, FOCUSER_STEPS_PROPERTY, NULL);
	FOCUSER_POSITION_PROPERTY->state = INDIGO_ALERT_STATE;
	if (message != NULL) {
		indigo_update_property(device, FOCUSER_POSITION_PROPERTY, "%s", message);
	} else {
		indigo_update_property(device, FOCUSER_POSITION_PROPERTY, NULL);
	}
}

// The reason the controller gave for refusing a move.
static void focuser_motion_failed(indigo_device *device, const char *state) {
	const char *message = power_message(state);
	char reason[INDIGO_VALUE_SIZE];
	if (message == NULL && *primalucelib_last_error(LINK)) {
		snprintf(reason, sizeof(reason), "Move refused: %s", primalucelib_last_error(LINK));
		message = reason;
	} else if (message == NULL && state != NULL && *state) {
		// The controller reports a refusal in the acknowledgement itself, e.g. "Error: invalid parameter".
		snprintf(reason, sizeof(reason), "Move refused: %s", state);
		message = reason;
	}
	focuser_motion_refused(device, message);
}

// The calibrated travel limits the positions a client can request; an uncalibrated controller
// reports no usable range and keeps the wide default. Ranges travel only with a definition, so a
// running session republishes the motion properties.
static void focuser_set_range(indigo_device *device, double min_position, double max_position, bool redefine) {
	PRIVATE_DATA->calibrated = max_position > min_position;
	if (!PRIVATE_DATA->calibrated) {
		min_position = 0;
		max_position = 1000000;
	}
	FOCUSER_POSITION_ITEM->number.min = min_position;
	FOCUSER_POSITION_ITEM->number.max = max_position;
	FOCUSER_STEPS_ITEM->number.max = max_position - min_position;
	for (int i = 0; i < PRIMALUCE_STORED_POSITION_COUNT; i++) {
		X_PRESETS_PROPERTY->items[i].number.max = max_position;
	}
	if (redefine) {
		indigo_delete_property(device, FOCUSER_POSITION_PROPERTY, NULL);
		indigo_define_property(device, FOCUSER_POSITION_PROPERTY, NULL);
		indigo_delete_property(device, FOCUSER_STEPS_PROPERTY, NULL);
		indigo_define_property(device, FOCUSER_STEPS_PROPERTY, NULL);
		if (!X_PRESETS_PROPERTY->hidden) {
			indigo_delete_property(device, X_PRESETS_PROPERTY, NULL);
			indigo_define_property(device, X_PRESETS_PROPERTY, NULL);
		}
	}
}

// Ends the calibration ALERT with message, or with the reason the controller gave for the last refusal.
static void focuser_calibration_failed(indigo_device *device, const char *message) {
	PRIVATE_DATA->calibration_pending = PRIVATE_DATA->calibration_starting = PRIVATE_DATA->calibrating = false;
	X_CALIBRATE_F_START_ITEM->sw.value = X_CALIBRATE_F_START_INVERTED_ITEM->sw.value = X_CALIBRATE_F_END_ITEM->sw.value = false;
	X_CALIBRATE_F_PROPERTY->state = INDIGO_ALERT_STATE;
	if (message == NULL && *primalucelib_last_error(LINK)) {
		message = primalucelib_last_error(LINK);
	}
	if (message != NULL) {
		indigo_update_property(device, X_CALIBRATE_F_PROPERTY, "%s", message);
	} else {
		indigo_update_property(device, X_CALIBRATE_F_PROPERTY, NULL);
	}
}

// Runs the next step of the calibration start. The controller needs a second between the steps, so each
// step is a finalizer of its own and abort and disconnect are served in between. A SESTO SENSO 2 sets the
// direction inside the calibration, a SESTO SENSO 3 before it (it restarts after the write).
static void focuser_calibration_start_finalizer(indigo_device *device) {
	static const char *sestosenso_2_steps[] = { PRIMALUCE_FOCUSER_CALIBRATION_INIT, PRIMALUCE_KEY_CALIBRATION_DIRECTION, PRIMALUCE_FOCUSER_CALIBRATION_STORE_MIN, PRIMALUCE_FOCUSER_CALIBRATION_FIND_MAX, NULL };
	static const char *sestosenso_3_steps[] = { PRIMALUCE_FOCUSER_CALIBRATION_INIT, PRIMALUCE_FOCUSER_CALIBRATION_STORE_MIN, PRIMALUCE_FOCUSER_CALIBRATION_FIND_MAX, NULL };
	const char **steps = PRIVATE_DATA->is_sestosenso_3 ? sestosenso_3_steps : sestosenso_2_steps;
	const char *step = steps[PRIVATE_DATA->calibration_step++];
	bool result = !strcmp(step, PRIMALUCE_KEY_CALIBRATION_DIRECTION) ? primalucelib_focuser_set_calibration_direction(LINK, PRIVATE_DATA->calibration_direction) : primalucelib_focuser_calibration_step(LINK, step);
	if (!result) {
		focuser_calibration_failed(device, NULL);
	} else if (steps[PRIVATE_DATA->calibration_step] != NULL) {
		indigo_execute_handler_in(device, CALIBRATION_STEP_INTERVAL, focuser_calibration_start_finalizer);
	} else {
		// The motor runs out to the far end until END; the property is OK so that END is accepted.
		PRIVATE_DATA->calibration_pending = PRIVATE_DATA->calibration_starting = false;
		PRIVATE_DATA->calibrating = true;
		X_CALIBRATE_F_PROPERTY->state = INDIGO_OK_STATE;
		indigo_update_property(device, X_CALIBRATE_F_PROPERTY, NULL);
	}
}

// Probes a SESTO SENSO 3 restarting after the CAL_DIR write with one short request at a time; requests of
// other handlers fail at once meanwhile (primalucelib_begin_restart()). Once it answers the calibration
// starts, unless it was aborted during the restart.
static void focuser_calibration_restart_finalizer(indigo_device *device) {
	if (primalucelib_probe_restart(LINK)) {
		if (PRIVATE_DATA->calibration_starting) {
			focuser_calibration_start_finalizer(device);
		}
	} else if (++PRIVATE_DATA->calibration_polls < PRIMALUCE_RESTART_POLLS) {
		indigo_execute_handler_in(device, PRIMALUCE_RESTART_POLL_INTERVAL, focuser_calibration_restart_finalizer);
	} else {
		primalucelib_end_restart(LINK);
		if (PRIVATE_DATA->calibration_starting) {
			focuser_calibration_failed(device, "The focuser did not answer after the restart");
		}
	}
}

// The controller stores the new range a moment after it acknowledges StoreAsMaxPos: a SESTO SENSO 3 still
// reported the previous CAL_MAXPOS (0 uncalibrated, 15237 from the calibration before) right after the
// acknowledgement, so the range is polled until the maximum changes or the polls run out.
static void focuser_calibration_end_finalizer(indigo_device *device) {
	double min_position = 0, max_position = 0;
	bool read = primalucelib_focuser_get_calibration(LINK, &min_position, &max_position);
	if ((!read || max_position <= min_position || max_position == PRIVATE_DATA->calibration_previous_max) && ++PRIVATE_DATA->calibration_polls < CALIBRATION_RANGE_POLLS) {
		indigo_execute_handler_in(device, CALIBRATION_RANGE_POLL_INTERVAL, focuser_calibration_end_finalizer);
		return;
	}
	focuser_set_range(device, min_position, max_position, true);
	double position = 0;
	bool position_read = primalucelib_focuser_get_position(LINK, PRIVATE_DATA->has_abs_pos, &position);
	if (position_read) {
		FOCUSER_POSITION_ITEM->number.value = FOCUSER_POSITION_ITEM->number.target = PRIVATE_DATA->position = position;
		indigo_update_property(device, FOCUSER_POSITION_PROPERTY, NULL);
	}
	if (!PRIVATE_DATA->calibrated) {
		focuser_calibration_failed(device, "The calibration left no travel range");
		return;
	}
	// A SESTO SENSO 3 that lost its coordinate reports 1000000 with the motor off (firmware 1.0.9), far outside
	// the range it has just stored, and the calibration has to be repeated.
	if (position_read && (position < min_position || position > max_position)) {
		focuser_calibration_failed(device, "The focuser reports a position outside the calibrated range, run the calibration again");
		return;
	}
	PRIVATE_DATA->calibration_pending = PRIVATE_DATA->focuser_needs_calibration = false;
	X_CALIBRATE_F_PROPERTY->state = INDIGO_OK_STATE;
	indigo_update_property(device, X_CALIBRATE_F_PROPERTY, NULL);
}

// Reads the focuser position and whether the motor runs. A reply without the position
// is a failed read; a reply without MST cannot prove the motor is still running, so it
// counts as stopped.
static bool focuser_read_motion(indigo_device *device, double *position, bool *moving) {
	if (!primalucelib_focuser_get_position(LINK, PRIVATE_DATA->has_abs_pos, position)) {
		return false;
	}
	PRIVATE_DATA->position = *position;
	char *state = NULL;
	if (!PRIVATE_DATA->is_sestosenso_3 || primalucelib_focuser_request_motor_state(LINK)) {
		state = primalucelib_reply_focuser_motor_state(LINK);
	}
	*moving = state != NULL && !primalucelib_is_motor_stopped(state);
	return true;
}

static void focuser_movement_ended(indigo_device *device, indigo_property_state state) {
	// An interrupted or failed move publishes the position the focuser reached as value; the
	// target keeps the request.
	PRIVATE_DATA->abort_requested = false;
	FOCUSER_POSITION_PROPERTY->state = FOCUSER_STEPS_PROPERTY->state = state;
}

static void focuser_movement_finalizer(indigo_device *device) {
	double position = 0;
	bool moving = false;
	const char *message = NULL;
	if (focuser_read_motion(device, &position, &moving)) {
		PRIVATE_DATA->poll_failures = 0;
		FOCUSER_POSITION_ITEM->number.value = position;
		if (moving) {
			if (position != PRIVATE_DATA->last_position) {
				PRIVATE_DATA->last_position = position;
				PRIVATE_DATA->stalled_polls = 0;
			} else if (++PRIVATE_DATA->stalled_polls >= STALL_POLLS) {
				// The motor reports running but the position no longer changes.
				char *reason;
				primalucelib_focuser_stop(LINK, &reason);
				if (focuser_read_motion(device, &position, &moving)) {
					FOCUSER_POSITION_ITEM->number.value = position;
				}
				focuser_movement_ended(device, INDIGO_ALERT_STATE);
				indigo_update_property(device, FOCUSER_STEPS_PROPERTY, NULL);
				indigo_update_property(device, FOCUSER_POSITION_PROPERTY, "The motor stalled");
				return;
			}
			// Progress is published on every poll, spaced like the rotator poll. Polling
			// as fast as the link allows publishes the same property from this queue
			// hundreds of times per second, which floods the clients.
			indigo_execute_handler_in(device, 0.2, focuser_movement_finalizer);
		} else {
			indigo_property_state state = INDIGO_ALERT_STATE;
			for (int i = 0; i < 10; i++) {
				if (FOCUSER_POSITION_ITEM->number.target == FOCUSER_POSITION_ITEM->number.value) {
					state = INDIGO_OK_STATE;
					break;
				}
				indigo_usleep(100000);
				if (focuser_read_motion(device, &position, &moving)) {
					FOCUSER_POSITION_ITEM->number.value = position;
				}
			}
			// A SESTO SENSO 3 with its motor bridges off (STATUS.HIZ after a motor fault) or outside its
			// calibrated range acknowledges GOTO and stays where it is.
			if (state == INDIGO_ALERT_STATE && !PRIVATE_DATA->abort_requested) {
				message = PRIVATE_DATA->focuser_needs_calibration ? "The focuser needs calibration" : "The focuser stopped before reaching the target";
			}
			focuser_movement_ended(device, state);
		}
	} else if (++PRIVATE_DATA->poll_failures < MAX_POLL_FAILURES) {
		// A single lost readback is retried.
		indigo_execute_handler_in(device, 0.2, focuser_movement_finalizer);
		return;
	} else {
		// A persistent failure must neither leave the motion properties busy nor let the
		// motor run unobserved.
		char *reason;
		primalucelib_focuser_stop(LINK, &reason);
		focuser_movement_ended(device, INDIGO_ALERT_STATE);
	}
	indigo_update_property(device, FOCUSER_STEPS_PROPERTY, NULL);
	if (message != NULL) {
		indigo_update_property(device, FOCUSER_POSITION_PROPERTY, "%s", message);
	} else {
		indigo_update_property(device, FOCUSER_POSITION_PROPERTY, NULL);
	}
}

// Follows the position while no move of the driver runs: motion nobody commanded here
// (the virtual keypad of the web interface, another client on the other link, a move
// running at connect) is published BUSY with the target following the measured position
// and OK once it stops.
static void focuser_poll_position(indigo_device *device) {
	if (PRIVATE_DATA->pending || ((FOCUSER_POSITION_PROPERTY->state == INDIGO_BUSY_STATE || FOCUSER_STEPS_PROPERTY->state == INDIGO_BUSY_STATE) && !PRIVATE_DATA->external_motion)) {
		return;
	}
	double position = 0;
	bool moving = false;
	bool read = focuser_read_motion(device, &position, &moving);
	// A request accepted while the poll was in flight owns the motion properties now.
	if (PRIVATE_DATA->pending || ((FOCUSER_POSITION_PROPERTY->state == INDIGO_BUSY_STATE || FOCUSER_STEPS_PROPERTY->state == INDIGO_BUSY_STATE) && !PRIVATE_DATA->external_motion)) {
		return;
	}
	if (read) {
		// The target follows only calibration and uncommanded motion; after a move of the
		// driver it keeps the request.
		FOCUSER_POSITION_ITEM->number.value = position;
		if (PRIVATE_DATA->calibrating) {
			// The calibration run is the driver's own motion: it is followed, but not taken
			// for an uncommanded move, which would leave the position BUSY and refuse END.
			FOCUSER_POSITION_ITEM->number.target = position;
		} else if (moving) {
			PRIVATE_DATA->external_motion = true;
			PRIVATE_DATA->poll_alert = false;
			FOCUSER_POSITION_ITEM->number.target = position;
			FOCUSER_POSITION_PROPERTY->state = FOCUSER_STEPS_PROPERTY->state = INDIGO_BUSY_STATE;
		} else if (PRIVATE_DATA->external_motion) {
			PRIVATE_DATA->external_motion = false;
			FOCUSER_POSITION_ITEM->number.target = position;
			FOCUSER_POSITION_PROPERTY->state = FOCUSER_STEPS_PROPERTY->state = INDIGO_OK_STATE;
		} else if (PRIVATE_DATA->poll_alert) {
			PRIVATE_DATA->poll_alert = false;
			FOCUSER_POSITION_PROPERTY->state = INDIGO_OK_STATE;
		}
	} else {
		if (PRIVATE_DATA->external_motion) {
			PRIVATE_DATA->external_motion = false;
			FOCUSER_STEPS_PROPERTY->state = INDIGO_ALERT_STATE;
		}
		PRIVATE_DATA->poll_alert = true;
		FOCUSER_POSITION_PROPERTY->state = INDIGO_ALERT_STATE;
	}
	indigo_update_property(device, FOCUSER_POSITION_PROPERTY, NULL);
	indigo_update_property(device, FOCUSER_STEPS_PROPERTY, NULL);
}

//- focuser.code

//+ rotator.code

// POSITION_DEG is the angle after a sync and ABS_POS_DEG the mechanical one, so the
// synced angle is reported where the firmware has it.
static bool rotator_read_angle(indigo_device *device) {
	return primalucelib_rotator_get_angle(LINK, primalucelib_rotator_angle_source(PRIVATE_DATA->rotator_has_position_deg, PRIVATE_DATA->rotator_has_abs_pos), &ROTATOR_POSITION_ITEM->number.value);
}

// The controller calibrates the ARCO on its own and reports CAL_STATUS "stop" when it is done.
static void rotator_calibration_poll(indigo_device *device) {
	char *state;
	// An abort ends the calibration while a poll may already be queued.
	if (X_CALIBRATE_R_PROPERTY->state != INDIGO_BUSY_STATE) {
		return;
	}
	if ((state = primalucelib_rotator_get_calibration_status(LINK)) == NULL) {
		INDIGO_UPDATE_PROPERTY_STATE(X_CALIBRATE_R_PROPERTY, INDIGO_ALERT_STATE, NULL);
	} else if (!primalucelib_is_rotator_calibration_finished(state)) {
		if (++PRIVATE_DATA->rotator_calibration_polls > 600) {
			INDIGO_UPDATE_PROPERTY_STATE(X_CALIBRATE_R_PROPERTY, INDIGO_ALERT_STATE, "Calibration did not finish");
		} else {
			indigo_execute_handler_in(device, 1, rotator_calibration_poll);
		}
	} else {
		if (rotator_read_angle(device)) {
			ROTATOR_POSITION_ITEM->number.target = ROTATOR_POSITION_ITEM->number.value;
			indigo_update_property(device, ROTATOR_POSITION_PROPERTY, NULL);
		}
		PRIVATE_DATA->rotator_needs_calibration = false;
		INDIGO_UPDATE_PROPERTY_STATE(X_CALIBRATE_R_PROPERTY, INDIGO_OK_STATE, NULL);
	}
}

static void rotator_movement_finalizer(indigo_device *device) {
	if (rotator_read_angle(device)) {
		char *state = primalucelib_reply_rotator_motor_state(LINK);
		if (state != NULL && !primalucelib_is_motor_stopped(state)) {
			// The angle is published on every poll, so a client follows the rotation.
			indigo_update_property(device, ROTATOR_POSITION_PROPERTY, NULL);
			indigo_execute_handler_in(device, 0.2, rotator_movement_finalizer);
		} else if (ROTATOR_POSITION_PROPERTY->state == INDIGO_BUSY_STATE) {
			for (int i = 0; i < 10; i++) {
				indigo_usleep(100000);
				rotator_read_angle(device);
				if (fabs(ROTATOR_POSITION_ITEM->number.target - ROTATOR_POSITION_ITEM->number.value) <= ROTATOR_POSITION_TOLERANCE) {
					ROTATOR_POSITION_PROPERTY->state = INDIGO_OK_STATE;
					break;
				}
			}
			if (ROTATOR_POSITION_PROPERTY->state == INDIGO_BUSY_STATE) {
				// An ARCO flagged for calibration acknowledges the move and does not turn (SESTO SENSO 3 firmware 1.0.9).
				ROTATOR_POSITION_PROPERTY->state = INDIGO_ALERT_STATE;
				indigo_update_property(device, ROTATOR_POSITION_PROPERTY, PRIVATE_DATA->rotator_needs_calibration ? "The rotator needs calibration" : "The rotator stopped before reaching the target");
			} else {
				indigo_update_property(device, ROTATOR_POSITION_PROPERTY, NULL);
			}
		} else {
			// an aborted move publishes the angle it stopped at and stays ALERT, also when that is the target
			indigo_update_property(device, ROTATOR_POSITION_PROPERTY, NULL);
		}
	} else {
		ROTATOR_POSITION_PROPERTY->state = INDIGO_ALERT_STATE;
		indigo_update_property(device, ROTATOR_POSITION_PROPERTY, NULL);
	}
}

//- rotator.code

#pragma mark - High level code (focuser)

static void focuser_timer_callback(indigo_device *device) {
	if (!IS_CONNECTED) {
		return;
	}
	//+ focuser.on_timer
	// A restarting controller answers nothing; it is not polled until focuser_calibration_restart_finalizer
	// finds it back.
	if (primalucelib_is_restarting(LINK)) {
		indigo_execute_handler_in(device, POLL_PERIOD, focuser_timer_callback);
		return;
	}
	if (PRIVATE_DATA->environment_ticks++ % ENVIRONMENT_TICKS == 0) {
		if (primalucelib_get_environment(LINK)) {
			double temp = primalucelib_reply_external_temperature(LINK);
			// A controller without an external probe reports -127, which is no reading.
			if (primalucelib_is_no_probe_temperature(temp)) {
				FOCUSER_TEMPERATURE_PROPERTY->state = INDIGO_IDLE_STATE;
			} else {
				FOCUSER_TEMPERATURE_ITEM->number.value = temp;
				FOCUSER_TEMPERATURE_PROPERTY->state = INDIGO_OK_STATE;
			}
			indigo_update_property(device, FOCUSER_TEMPERATURE_PROPERTY, NULL);
			double motor_temp = primalucelib_reply_motor_temperature(LINK);
			double vin12v = primalucelib_reply_supply_voltage(LINK);
			if (motor_temp != X_STATE_MOTOR_TEMP_ITEM->number.value || vin12v != X_STATE_VIN_12V_ITEM->number.value || X_STATE_PROPERTY->state != INDIGO_OK_STATE) {
				X_STATE_MOTOR_TEMP_ITEM->number.value = motor_temp;
				X_STATE_VIN_12V_ITEM->number.value = vin12v;
				X_STATE_PROPERTY->state = INDIGO_OK_STATE;
				indigo_update_property(device, X_STATE_PROPERTY, NULL);
			}
		} else {
			// A failed reading keeps the last valid values and says so.
			if (FOCUSER_TEMPERATURE_PROPERTY->state != INDIGO_IDLE_STATE) {
				FOCUSER_TEMPERATURE_PROPERTY->state = INDIGO_ALERT_STATE;
				indigo_update_property(device, FOCUSER_TEMPERATURE_PROPERTY, NULL);
			}
			X_STATE_PROPERTY->state = INDIGO_ALERT_STATE;
			indigo_update_property(device, X_STATE_PROPERTY, NULL);
		}
	}
	focuser_poll_position(device);
	indigo_execute_handler_in(device, PRIVATE_DATA->external_motion ? 0.5 : POLL_PERIOD, focuser_timer_callback);
	//- focuser.on_timer
}

static void focuser_connection_handler(indigo_device *device) {
	if (CONNECTION_CONNECTED_ITEM->sw.value) {
		bool connection_result = true;
		const char *error_message = NULL;
		if (PRIVATE_DATA->count == 0) {
			connection_result = primaluce_open(device, &error_message);
		}
		if (connection_result) {
			PRIVATE_DATA->count++;
		}
		if (connection_result) {
			//+ focuser.on_connect
			char *text;
			if (primalucelib_get_state(LINK)) {
				if ((text = primalucelib_reply_string(LINK, PRIMALUCE_REPLY_MODEL_NAME))) {
					INDIGO_DRIVER_DEBUG(DRIVER_NAME, "Model: %s", text);
					INDIGO_COPY_VALUE(INFO_DEVICE_MODEL_ITEM->text.value, text);
					primalucelib_model model = primalucelib_model_from_name(text);
					if (primalucelib_model_is_sestosenso(model)) {
						X_STATE_PROPERTY->count = 2;
						X_CONFIG_PROPERTY->hidden = false;
						X_RUNPRESET_PROPERTY->hidden = false;
						X_RUNPRESET_L_PROPERTY->hidden = false;
						X_RUNPRESET_M_PROPERTY->hidden = false;
						X_RUNPRESET_S_PROPERTY->hidden = false;
						X_RUNPRESET_1_PROPERTY->hidden = false;
						X_RUNPRESET_2_PROPERTY->hidden = false;
						X_RUNPRESET_3_PROPERTY->hidden = false;
						X_HOLD_CURR_PROPERTY->hidden = false;
					} else if (model == PRIMALUCELIB_MODEL_ESATTO) {
						X_STATE_PROPERTY->count = 3;
						X_CONFIG_PROPERTY->hidden = true;
						X_RUNPRESET_PROPERTY->hidden = true;
						X_RUNPRESET_L_PROPERTY->hidden = true;
						X_RUNPRESET_M_PROPERTY->hidden = true;
						X_RUNPRESET_S_PROPERTY->hidden = true;
						X_RUNPRESET_1_PROPERTY->hidden = true;
						X_RUNPRESET_2_PROPERTY->hidden = true;
						X_RUNPRESET_3_PROPERTY->hidden = true;
						X_HOLD_CURR_PROPERTY->hidden = true;
					} else {
						X_STATE_PROPERTY->count = 2;
						X_CALIBRATE_F_PROPERTY->hidden = true;
						X_CONFIG_PROPERTY->hidden = true;
						X_RUNPRESET_PROPERTY->hidden = true;
						X_RUNPRESET_L_PROPERTY->hidden = true;
						X_RUNPRESET_M_PROPERTY->hidden = true;
						X_RUNPRESET_S_PROPERTY->hidden = true;
						X_RUNPRESET_1_PROPERTY->hidden = true;
						X_RUNPRESET_2_PROPERTY->hidden = true;
						X_RUNPRESET_3_PROPERTY->hidden = true;
						X_HOLD_CURR_PROPERTY->hidden = true;
					}
				}
				if ((text = primalucelib_reply_string(LINK, PRIMALUCE_REPLY_APPLICATION_FIRMWARE))) {
					INDIGO_DRIVER_DEBUG(DRIVER_NAME, "SWAPP: %s", text);
					INDIGO_COPY_VALUE(INFO_DEVICE_FW_REVISION_ITEM->text.value, text);
					if ((text = primalucelib_reply_string(LINK, PRIMALUCE_REPLY_WEB_FIRMWARE))) {
						INDIGO_DRIVER_DEBUG(DRIVER_NAME, "SWWEB: %s", text);
						strcat(INFO_DEVICE_FW_REVISION_ITEM->text.value, " / ");
						strcat(INFO_DEVICE_FW_REVISION_ITEM->text.value, text);
					}
				}
				if ((text = primalucelib_reply_string(LINK, PRIMALUCE_REPLY_SERIAL_NUMBER))) {
					INDIGO_DRIVER_DEBUG(DRIVER_NAME, "SN: %s", text);
					INDIGO_COPY_VALUE(INFO_DEVICE_SERIAL_NUM_ITEM->text.value, text);
				}
				indigo_update_property(device, INFO_PROPERTY, NULL);
				if ((text = primalucelib_reply_string(LINK, PRIMALUCE_REPLY_FOCUSER_ERROR)) && *text) {
					indigo_send_message(device, ALERT_PROPERTY, "%s", text);
				}
				PRIVATE_DATA->focuser_needs_calibration = primalucelib_reply_number(LINK, PRIMALUCE_REPLY_FOCUSER_CALIBRATION_RESTART) != 0;
				if (PRIVATE_DATA->focuser_needs_calibration) {
					indigo_send_message(device, BUSY_PROPERTY, "%s needs calibration", INFO_DEVICE_MODEL_ITEM->text.value);
				}
				PRIVATE_DATA->has_abs_pos = primalucelib_reply_has_focuser_position(LINK, true);
				focuser_set_range(device, primalucelib_reply_number(LINK, PRIMALUCE_REPLY_FOCUSER_CALIBRATION_MIN), primalucelib_reply_number(LINK, PRIMALUCE_REPLY_FOCUSER_CALIBRATION_MAX), false);
				FOCUSER_POSITION_ITEM->number.value = FOCUSER_POSITION_ITEM->number.target = PRIVATE_DATA->position = primalucelib_reply_focuser_position(LINK, PRIVATE_DATA->has_abs_pos);
				if (!primalucelib_reply_has(LINK, PRIMALUCE_REPLY_FOCUSER_SPEED)) {
					FOCUSER_SPEED_PROPERTY->hidden = true;
				} else {
					FOCUSER_SPEED_ITEM->number.value = FOCUSER_SPEED_ITEM->number.target = primalucelib_reply_number(LINK, PRIMALUCE_REPLY_FOCUSER_SPEED);
					FOCUSER_SPEED_PROPERTY->hidden = false;
				}
				FOCUSER_BACKLASH_ITEM->number.value = FOCUSER_BACKLASH_ITEM->number.target = PRIVATE_DATA->backlash = (int)primalucelib_reply_number(LINK, PRIMALUCE_REPLY_FOCUSER_BACKLASH);
				PRIVATE_DATA->speed = (int)FOCUSER_SPEED_ITEM->number.value;
				PRIVATE_DATA->external_motion = PRIVATE_DATA->poll_alert = PRIVATE_DATA->abort_requested = PRIVATE_DATA->calibrating = PRIVATE_DATA->calibration_pending = PRIVATE_DATA->calibration_starting = PRIVATE_DATA->pending = false;
				PRIVATE_DATA->environment_ticks = 0;
				// Models without stored positions report no PRESET_n.
				bool has_presets = primalucelib_reply_has(LINK, PRIMALUCE_REPLY_FIRST_STORED_POSITION);
				X_PRESETS_PROPERTY->hidden = X_PRESET_NAMES_PROPERTY->hidden = X_PRESET_GOTO_PROPERTY->hidden = !has_presets;
				for (int i = 0; has_presets && i < PRIMALUCE_STORED_POSITION_COUNT; i++) {
					PRIVATE_DATA->preset_positions[i] = (int)primalucelib_reply_stored_position(LINK, i + 1);
					X_PRESETS_PROPERTY->items[i].number.value = X_PRESETS_PROPERTY->items[i].number.target = PRIVATE_DATA->preset_positions[i];
					INDIGO_COPY_VALUE(PRIVATE_DATA->preset_names[i], (text = primalucelib_reply_stored_position_name(LINK, i + 1)) ? text : "");
					INDIGO_COPY_VALUE(X_PRESET_NAMES_PROPERTY->items[i].text.value, PRIVATE_DATA->preset_names[i]);
				}
				X_STATE_MOTOR_TEMP_ITEM->number.value = primalucelib_reply_motor_temperature(LINK);
				X_STATE_VIN_12V_ITEM->number.value = primalucelib_reply_supply_voltage(LINK);
				X_STATE_VIN_USB_ITEM->number.value = primalucelib_reply_usb_voltage(LINK);
				// LANCFG selects between the access point and a connection to an existing network
				if ((text = primalucelib_reply_string(LINK, PRIMALUCE_REPLY_LAN_CONFIG)) && !strcmp(text, PRIMALUCE_LAN_STATION)) {
					indigo_set_switch(X_WIFI_PROPERTY, X_WIFI_STA_ITEM, true);
				} else if ((text = primalucelib_reply_string(LINK, PRIMALUCE_REPLY_ACCESS_POINT_STATUS))) {
					if (!strcmp(text, PRIMALUCE_ACCESS_POINT_ON)) {
						indigo_set_switch(X_WIFI_PROPERTY, X_WIFI_AP_ITEM, true);
					} else {
						indigo_set_switch(X_WIFI_PROPERTY, X_WIFI_OFF_ITEM, true);
					}
				}
				if ((text = primalucelib_reply_string(LINK, PRIMALUCE_REPLY_ACCESS_POINT_SSID))) {
					INDIGO_COPY_VALUE(X_WIFI_AP_SSID_ITEM->text.value, text);
				}
				if ((text = primalucelib_reply_string(LINK, PRIMALUCE_REPLY_ACCESS_POINT_PASSWORD))) {
					INDIGO_COPY_VALUE(X_WIFI_AP_PASSWORD_ITEM->text.value, text);
				}
				if ((text = primalucelib_reply_string(LINK, PRIMALUCE_REPLY_STATION_SSID))) {
					INDIGO_COPY_VALUE(X_WIFI_STA_SSID_ITEM->text.value, text);
				}
				if ((text = primalucelib_reply_string(LINK, PRIMALUCE_REPLY_STATION_PASSWORD))) {
					INDIGO_COPY_VALUE(X_WIFI_STA_PASSWORD_ITEM->text.value, text);
				}
				if ((text = primalucelib_reply_string(LINK, PRIMALUCE_REPLY_LEDS))) {
					if (!strcmp(text, PRIMALUCE_LEDS_ON)) {
						indigo_set_switch(X_LEDS_PROPERTY, X_LEDS_ON_ITEM, true);
					} else if (!strcmp(text, PRIMALUCE_LEDS_LOW)) {
						indigo_set_switch(X_LEDS_PROPERTY, X_LEDS_DIM_ITEM, true);
					} else if (!strcmp(text, PRIMALUCE_LEDS_MIDDLE)) {
						indigo_set_switch(X_LEDS_PROPERTY, X_LEDS_MIDDLE_ITEM, true);
					} else {
						indigo_set_switch(X_LEDS_PROPERTY, X_LEDS_OFF_ITEM, true);
					}
				}
				primalucelib_run_config config;
				primalucelib_reply_motor_config(LINK, &config);
				set_run_config_items(X_CONFIG_PROPERTY, &config);
				primalucelib_reply_run_preset(LINK, PRIMALUCE_KEY_RUN_PRESET_LIGHT, &config);
				set_run_config_items(X_RUNPRESET_L_PROPERTY, &config);
				primalucelib_reply_run_preset(LINK, PRIMALUCE_KEY_RUN_PRESET_MEDIUM, &config);
				set_run_config_items(X_RUNPRESET_M_PROPERTY, &config);
				primalucelib_reply_run_preset(LINK, PRIMALUCE_KEY_RUN_PRESET_SLOW, &config);
				set_run_config_items(X_RUNPRESET_S_PROPERTY, &config);
				primalucelib_reply_run_preset(LINK, PRIMALUCE_KEY_RUN_PRESET_USER_PREFIX "1", &config);
				set_run_config_items(X_RUNPRESET_1_PROPERTY, &config);
				primalucelib_reply_run_preset(LINK, PRIMALUCE_KEY_RUN_PRESET_USER_PREFIX "2", &config);
				set_run_config_items(X_RUNPRESET_2_PROPERTY, &config);
				primalucelib_reply_run_preset(LINK, PRIMALUCE_KEY_RUN_PRESET_USER_PREFIX "3", &config);
				set_run_config_items(X_RUNPRESET_3_PROPERTY, &config);
				if (primalucelib_reply_number(LINK, PRIMALUCE_REPLY_FOCUSER_HOLD_CURRENT_STATUS)) {
					indigo_set_switch(X_HOLD_CURR_PROPERTY, X_HOLD_CURR_ON_ITEM, true);
				} else {
					indigo_set_switch(X_HOLD_CURR_PROPERTY, X_HOLD_CURR_OFF_ITEM, true);
				}
				PRIVATE_DATA->hold_item = selected_item(X_HOLD_CURR_PROPERTY);
				PRIVATE_DATA->leds_item = selected_item(X_LEDS_PROPERTY);
				PRIVATE_DATA->wifi_item = selected_item(X_WIFI_PROPERTY);
			}
			//- focuser.on_connect
		}
		if (connection_result) {
			indigo_define_property(device, X_CONFIG_PROPERTY, NULL);
			indigo_define_property(device, X_STATE_PROPERTY, NULL);
			indigo_define_property(device, X_WIFI_PROPERTY, NULL);
			indigo_define_property(device, X_WIFI_AP_PROPERTY, NULL);
			indigo_define_property(device, X_WIFI_STA_PROPERTY, NULL);
			indigo_define_property(device, X_LEDS_PROPERTY, NULL);
			indigo_define_property(device, X_RUNPRESET_L_PROPERTY, NULL);
			indigo_define_property(device, X_RUNPRESET_M_PROPERTY, NULL);
			indigo_define_property(device, X_RUNPRESET_S_PROPERTY, NULL);
			indigo_define_property(device, X_RUNPRESET_1_PROPERTY, NULL);
			indigo_define_property(device, X_RUNPRESET_2_PROPERTY, NULL);
			indigo_define_property(device, X_RUNPRESET_3_PROPERTY, NULL);
			indigo_define_property(device, X_RUNPRESET_PROPERTY, NULL);
			indigo_define_property(device, X_PRESETS_PROPERTY, NULL);
			indigo_define_property(device, X_PRESET_NAMES_PROPERTY, NULL);
			indigo_define_property(device, X_PRESET_GOTO_PROPERTY, NULL);
			indigo_define_property(device, X_HOLD_CURR_PROPERTY, NULL);
			indigo_define_property(device, X_CALIBRATE_F_PROPERTY, NULL);
			CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
			indigo_send_message(device, OK_PROPERTY, "Connected to %s on %s", FOCUSER_DEVICE_NAME, DEVICE_PORT_ITEM->text.value);
		} else {
			if (error_message != NULL) {
				indigo_send_message(device, ALERT_PROPERTY, "Failed to connect to %s on %s (%s)", FOCUSER_DEVICE_NAME, DEVICE_PORT_ITEM->text.value, error_message);
			} else {
				indigo_send_message(device, ALERT_PROPERTY, "Failed to connect to %s on %s", FOCUSER_DEVICE_NAME, DEVICE_PORT_ITEM->text.value);
			}
			if (PRIVATE_DATA->count > 0 && --PRIVATE_DATA->count == 0) {
				primaluce_close(device);
			}
			CONNECTION_PROPERTY->state = INDIGO_ALERT_STATE;
			indigo_set_switch(CONNECTION_PROPERTY, CONNECTION_DISCONNECTED_ITEM, true);
		}
	} else {
		indigo_cancel_pending_handlers(device);
		//+ focuser.on_disconnect
		// A move, an uncommanded motion or a calibration still running is stopped before the
		// connection closes.
		if (FOCUSER_POSITION_PROPERTY->state == INDIGO_BUSY_STATE || FOCUSER_STEPS_PROPERTY->state == INDIGO_BUSY_STATE || PRIVATE_DATA->calibrating) {
			char *reason;
			if (!primalucelib_focuser_stop(LINK, &reason)) {
				INDIGO_DRIVER_ERROR(DRIVER_NAME, "Failed to stop the focuser");
			}
		}
		PRIVATE_DATA->external_motion = PRIVATE_DATA->poll_alert = PRIVATE_DATA->abort_requested = PRIVATE_DATA->calibrating = PRIVATE_DATA->calibration_pending = PRIVATE_DATA->calibration_starting = PRIVATE_DATA->pending = false;
		//- focuser.on_disconnect
		// Cancelled change handlers must not leave properties BUSY: a new session starts in a clean state.
		indigo_property *cancelled_properties[] = {
			X_CONFIG_PROPERTY,
			X_STATE_PROPERTY,
			X_WIFI_PROPERTY,
			X_WIFI_AP_PROPERTY,
			X_WIFI_STA_PROPERTY,
			X_LEDS_PROPERTY,
			X_RUNPRESET_L_PROPERTY,
			X_RUNPRESET_M_PROPERTY,
			X_RUNPRESET_S_PROPERTY,
			X_RUNPRESET_1_PROPERTY,
			X_RUNPRESET_2_PROPERTY,
			X_RUNPRESET_3_PROPERTY,
			X_RUNPRESET_PROPERTY,
			X_PRESETS_PROPERTY,
			X_PRESET_NAMES_PROPERTY,
			X_PRESET_GOTO_PROPERTY,
			X_HOLD_CURR_PROPERTY,
			X_CALIBRATE_F_PROPERTY,
			FOCUSER_TEMPERATURE_PROPERTY,
			FOCUSER_BACKLASH_PROPERTY,
			FOCUSER_POSITION_PROPERTY,
			FOCUSER_STEPS_PROPERTY,
			FOCUSER_SPEED_PROPERTY,
			FOCUSER_ABORT_MOTION_PROPERTY,
		};
		for (unsigned i = 0; i < sizeof(cancelled_properties) / sizeof(cancelled_properties[0]); i++) {
			if (cancelled_properties[i] != NULL && cancelled_properties[i]->state == INDIGO_BUSY_STATE) {
				cancelled_properties[i]->state = INDIGO_OK_STATE;
			}
		}
		indigo_delete_property(device, X_CONFIG_PROPERTY, NULL);
		indigo_delete_property(device, X_STATE_PROPERTY, NULL);
		indigo_delete_property(device, X_WIFI_PROPERTY, NULL);
		indigo_delete_property(device, X_WIFI_AP_PROPERTY, NULL);
		indigo_delete_property(device, X_WIFI_STA_PROPERTY, NULL);
		indigo_delete_property(device, X_LEDS_PROPERTY, NULL);
		indigo_delete_property(device, X_RUNPRESET_L_PROPERTY, NULL);
		indigo_delete_property(device, X_RUNPRESET_M_PROPERTY, NULL);
		indigo_delete_property(device, X_RUNPRESET_S_PROPERTY, NULL);
		indigo_delete_property(device, X_RUNPRESET_1_PROPERTY, NULL);
		indigo_delete_property(device, X_RUNPRESET_2_PROPERTY, NULL);
		indigo_delete_property(device, X_RUNPRESET_3_PROPERTY, NULL);
		indigo_delete_property(device, X_RUNPRESET_PROPERTY, NULL);
		indigo_delete_property(device, X_PRESETS_PROPERTY, NULL);
		indigo_delete_property(device, X_PRESET_NAMES_PROPERTY, NULL);
		indigo_delete_property(device, X_PRESET_GOTO_PROPERTY, NULL);
		indigo_delete_property(device, X_HOLD_CURR_PROPERTY, NULL);
		indigo_delete_property(device, X_CALIBRATE_F_PROPERTY, NULL);
		if (--PRIVATE_DATA->count == 0) {
			primaluce_close(device);
		}
		indigo_send_message(device, OK_PROPERTY, "Disconnected from %s", device->name);
		CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
	}
	indigo_focuser_change_property(device, NULL, CONNECTION_PROPERTY);
	if (IS_CONNECTED) {
		indigo_execute_handler(device, focuser_timer_callback);
	}
}

static void focuser_x_wifi_handler(indigo_device *device) {
	X_WIFI_PROPERTY->state = INDIGO_OK_STATE;
	//+ focuser.X_WIFI.on_change
	bool result = false;
	// Switching between the access point and station mode restarts the controller, which a
	// connected rotator shares.
	bool restart = X_WIFI_STA_ITEM->sw.value ? PRIVATE_DATA->wifi_item != X_WIFI_STA_ITEM : (X_WIFI_AP_ITEM->sw.value && PRIVATE_DATA->wifi_item == X_WIFI_STA_ITEM);
	if (restart && PRIVATE_DATA->count > 1) {
		restore_switch(X_WIFI_PROPERTY, PRIVATE_DATA->wifi_item);
		indigo_update_property(device, X_WIFI_PROPERTY, "Changing the WiFi mode restarts the controller, disconnect the rotator first");
		return;
	}
	if (X_WIFI_OFF_ITEM->sw.value) {
		result = primalucelib_set_access_point(LINK, false);
	} else if (X_WIFI_AP_ITEM->sw.value) {
		result = primaluce_set_lan_cfg(device, PRIMALUCE_LAN_ACCESS_POINT) && primalucelib_set_access_point(LINK, true);
	} else if (X_WIFI_STA_ITEM->sw.value) {
		result = primaluce_set_lan_cfg(device, PRIMALUCE_LAN_STATION);
	}
	if (result) {
		PRIVATE_DATA->wifi_item = selected_item(X_WIFI_PROPERTY);
	} else {
		restore_switch(X_WIFI_PROPERTY, PRIVATE_DATA->wifi_item);
	}
	//- focuser.X_WIFI.on_change
	indigo_update_property(device, X_WIFI_PROPERTY, NULL);
}

static void focuser_x_wifi_ap_handler(indigo_device *device) {
	X_WIFI_AP_PROPERTY->state = INDIGO_OK_STATE;
	//+ focuser.X_WIFI_AP.on_change
	if (!primalucelib_set_access_point_credentials(LINK, X_WIFI_AP_SSID_ITEM->text.value, X_WIFI_AP_PASSWORD_ITEM->text.value)) {
		X_WIFI_AP_PROPERTY->state = INDIGO_ALERT_STATE;
	}
	//- focuser.X_WIFI_AP.on_change
	indigo_update_property(device, X_WIFI_AP_PROPERTY, NULL);
}

static void focuser_x_wifi_sta_handler(indigo_device *device) {
	X_WIFI_STA_PROPERTY->state = INDIGO_OK_STATE;
	//+ focuser.X_WIFI_STA.on_change
	if (!primalucelib_set_station_credentials(LINK, X_WIFI_STA_SSID_ITEM->text.value, X_WIFI_STA_PASSWORD_ITEM->text.value)) {
		X_WIFI_STA_PROPERTY->state = INDIGO_ALERT_STATE;
	}
	//- focuser.X_WIFI_STA.on_change
	indigo_update_property(device, X_WIFI_STA_PROPERTY, NULL);
}

static void focuser_x_leds_handler(indigo_device *device) {
	X_LEDS_PROPERTY->state = INDIGO_OK_STATE;
	//+ focuser.X_LEDS.on_change
	bool result = false;
	if (X_LEDS_OFF_ITEM->sw.value) {
		result = primalucelib_set_leds(LINK, PRIMALUCE_LEDS_OFF);
	} else if (X_LEDS_DIM_ITEM->sw.value) {
		result = primalucelib_set_leds(LINK, PRIMALUCE_LEDS_LOW);
	} else if (X_LEDS_MIDDLE_ITEM->sw.value) {
		result = primalucelib_set_leds(LINK, PRIMALUCE_LEDS_MIDDLE);
	} else if (X_LEDS_ON_ITEM->sw.value) {
		result = primalucelib_set_leds(LINK, PRIMALUCE_LEDS_ON);
	}
	if (result) {
		PRIVATE_DATA->leds_item = selected_item(X_LEDS_PROPERTY);
	} else {
		restore_switch(X_LEDS_PROPERTY, PRIVATE_DATA->leds_item);
	}
	//- focuser.X_LEDS.on_change
	indigo_update_property(device, X_LEDS_PROPERTY, NULL);
}

static void focuser_x_runpreset_1_handler(indigo_device *device) {
	X_RUNPRESET_1_PROPERTY->state = INDIGO_OK_STATE;
	//+ focuser.X_RUNPRESET_1.on_change
	primalucelib_run_config config;
	get_run_config_targets(X_RUNPRESET_1_PROPERTY, &config);
	if (!primalucelib_focuser_set_user_run_preset(LINK, 1, &config)) {
		INDIGO_UPDATE_PROPERTY_STATE(X_RUNPRESET_1_PROPERTY, INDIGO_ALERT_STATE, NULL);
		return;
	}
	//- focuser.X_RUNPRESET_1.on_change
	indigo_update_property(device, X_RUNPRESET_1_PROPERTY, NULL);
}

static void focuser_x_runpreset_2_handler(indigo_device *device) {
	X_RUNPRESET_2_PROPERTY->state = INDIGO_OK_STATE;
	//+ focuser.X_RUNPRESET_2.on_change
	primalucelib_run_config config;
	get_run_config_targets(X_RUNPRESET_2_PROPERTY, &config);
	if (!primalucelib_focuser_set_user_run_preset(LINK, 2, &config)) {
		INDIGO_UPDATE_PROPERTY_STATE(X_RUNPRESET_2_PROPERTY, INDIGO_ALERT_STATE, NULL);
		return;
	}
	//- focuser.X_RUNPRESET_2.on_change
	indigo_update_property(device, X_RUNPRESET_2_PROPERTY, NULL);
}

static void focuser_x_runpreset_3_handler(indigo_device *device) {
	X_RUNPRESET_3_PROPERTY->state = INDIGO_OK_STATE;
	//+ focuser.X_RUNPRESET_3.on_change
	primalucelib_run_config config;
	get_run_config_targets(X_RUNPRESET_3_PROPERTY, &config);
	if (!primalucelib_focuser_set_user_run_preset(LINK, 3, &config)) {
		INDIGO_UPDATE_PROPERTY_STATE(X_RUNPRESET_3_PROPERTY, INDIGO_ALERT_STATE, NULL);
		return;
	}
	//- focuser.X_RUNPRESET_3.on_change
	indigo_update_property(device, X_RUNPRESET_3_PROPERTY, NULL);
}

static void focuser_x_runpreset_handler(indigo_device *device) {
	X_RUNPRESET_PROPERTY->state = INDIGO_OK_STATE;
	//+ focuser.X_RUNPRESET.on_change
	bool result = false;
	X_CONFIG_PROPERTY->state = INDIGO_OK_STATE;
	if (X_RUNPRESET_L_ITEM->sw.value) {
		result = primalucelib_focuser_apply_run_preset(LINK, PRIMALUCE_RUN_PRESET_LIGHT);
		X_RUNPRESET_L_ITEM->sw.value = false;
	} else if (X_RUNPRESET_M_ITEM->sw.value) {
		result = primalucelib_focuser_apply_run_preset(LINK, PRIMALUCE_RUN_PRESET_MEDIUM);
		X_RUNPRESET_M_ITEM->sw.value = false;
	} else if (X_RUNPRESET_S_ITEM->sw.value) {
		result = primalucelib_focuser_apply_run_preset(LINK, PRIMALUCE_RUN_PRESET_SLOW);
		X_RUNPRESET_S_ITEM->sw.value = false;
	} else if (X_RUNPRESET_1_ITEM->sw.value) {
		result = primalucelib_focuser_apply_user_run_preset(LINK, 1);
		X_RUNPRESET_1_ITEM->sw.value = false;
	} else if (X_RUNPRESET_2_ITEM->sw.value) {
		result = primalucelib_focuser_apply_user_run_preset(LINK, 2);
		X_RUNPRESET_2_ITEM->sw.value = false;
	} else if (X_RUNPRESET_3_ITEM->sw.value) {
		result = primalucelib_focuser_apply_user_run_preset(LINK, 3);
		X_RUNPRESET_3_ITEM->sw.value = false;
	}
	if (!result) {
		X_RUNPRESET_PROPERTY->state = INDIGO_ALERT_STATE;
	} else  if (!primalucelib_focuser_get_motor_config(LINK)) {
		X_CONFIG_PROPERTY->state = INDIGO_ALERT_STATE;
	}
	primalucelib_run_config config;
	primalucelib_reply_motor_config(LINK, &config);
	set_run_config_items(X_CONFIG_PROPERTY, &config);
	indigo_update_property(device, X_CONFIG_PROPERTY, NULL);
	//- focuser.X_RUNPRESET.on_change
	indigo_update_property(device, X_RUNPRESET_PROPERTY, NULL);
}

static void focuser_x_presets_handler(indigo_device *device) {
	X_PRESETS_PROPERTY->state = INDIGO_OK_STATE;
	//+ focuser.X_PRESETS.on_change
	// The controller refuses a zero position, so only positions that changed to a stored value are written.
	for (int i = 0; i < PRIMALUCE_STORED_POSITION_COUNT; i++) {
		int position = (int)X_PRESETS_PROPERTY->items[i].number.target;
		if (position == PRIVATE_DATA->preset_positions[i]) {
			continue;
		}
		if (primalucelib_focuser_set_stored_position(LINK, i + 1, position)) {
			PRIVATE_DATA->preset_positions[i] = position;
		} else {
			X_PRESETS_PROPERTY->state = INDIGO_ALERT_STATE;
		}
		X_PRESETS_PROPERTY->items[i].number.value = X_PRESETS_PROPERTY->items[i].number.target = PRIVATE_DATA->preset_positions[i];
	}
	//- focuser.X_PRESETS.on_change
	indigo_update_property(device, X_PRESETS_PROPERTY, NULL);
}

static void focuser_x_preset_names_handler(indigo_device *device) {
	X_PRESET_NAMES_PROPERTY->state = INDIGO_OK_STATE;
	//+ focuser.X_PRESET_NAMES.on_change
	for (int i = 0; i < PRIMALUCE_STORED_POSITION_COUNT; i++) {
		char *name = X_PRESET_NAMES_PROPERTY->items[i].text.value;
		if (!strcmp(name, PRIVATE_DATA->preset_names[i])) {
			continue;
		}
		if (primalucelib_focuser_set_stored_position_name(LINK, i + 1, name)) {
			INDIGO_COPY_VALUE(PRIVATE_DATA->preset_names[i], name);
		} else {
			X_PRESET_NAMES_PROPERTY->state = INDIGO_ALERT_STATE;
			INDIGO_COPY_VALUE(name, PRIVATE_DATA->preset_names[i]);
		}
	}
	//- focuser.X_PRESET_NAMES.on_change
	indigo_update_property(device, X_PRESET_NAMES_PROPERTY, NULL);
}

static void focuser_x_preset_goto_handler(indigo_device *device) {
	X_PRESET_GOTO_PROPERTY->state = INDIGO_OK_STATE;
	//+ focuser.X_PRESET_GOTO.on_change
	int index = -1;
	for (int i = 0; i < PRIMALUCE_STORED_POSITION_COUNT; i++) {
		if (X_PRESET_GOTO_PROPERTY->items[i].sw.value) {
			index = i;
		}
		X_PRESET_GOTO_PROPERTY->items[i].sw.value = false;
	}
	if (index < 0) {
		// Nothing selected, nothing to do.
	} else if (PRIVATE_DATA->preset_positions[index] <= 0) {
		X_PRESET_GOTO_PROPERTY->state = INDIGO_ALERT_STATE;
		indigo_update_property(device, X_PRESET_GOTO_PROPERTY, "Preset #%d has no stored position", index + 1);
		return;
	} else if (FOCUSER_POSITION_PROPERTY->state == INDIGO_BUSY_STATE || FOCUSER_STEPS_PROPERTY->state == INDIGO_BUSY_STATE) {
		X_PRESET_GOTO_PROPERTY->state = INDIGO_ALERT_STATE;
		indigo_update_property(device, X_PRESET_GOTO_PROPERTY, "Another motion operation is pending");
		return;
	} else {
		// The stored position is reached through the absolute move, which reports the motion.
		FOCUSER_POSITION_ITEM->number.target = PRIVATE_DATA->preset_positions[index];
		FOCUSER_POSITION_PROPERTY->state = INDIGO_BUSY_STATE;
		indigo_update_property(device, FOCUSER_POSITION_PROPERTY, NULL);
		focuser_position_handler(device);
	}
	//- focuser.X_PRESET_GOTO.on_change
	indigo_update_property(device, X_PRESET_GOTO_PROPERTY, NULL);
}

static void focuser_x_hold_curr_handler(indigo_device *device) {
	X_HOLD_CURR_PROPERTY->state = INDIGO_OK_STATE;
	//+ focuser.X_HOLD_CURR.on_change
	bool result = false;
	if (X_HOLD_CURR_OFF_ITEM->sw.value) {
		result = primalucelib_focuser_set_hold_current(LINK, false);
	} else if (X_HOLD_CURR_ON_ITEM->sw.value) {
		result = primalucelib_focuser_set_hold_current(LINK, true);
	}
	if (result) {
		PRIVATE_DATA->hold_item = selected_item(X_HOLD_CURR_PROPERTY);
	} else {
		restore_switch(X_HOLD_CURR_PROPERTY, PRIVATE_DATA->hold_item);
	}
	//- focuser.X_HOLD_CURR.on_change
	indigo_update_property(device, X_HOLD_CURR_PROPERTY, NULL);
}

static void focuser_x_calibrate_f_handler(indigo_device *device) {
	//+ focuser.X_CALIBRATE_F.on_change
	// The whole calibration publishes its own states: the steps run as finalizers on the device queue.
	if (X_CALIBRATE_F_START_ITEM->sw.value || X_CALIBRATE_F_START_INVERTED_ITEM->sw.value) {
		const char *direction = X_CALIBRATE_F_START_ITEM->sw.value ? PRIMALUCE_CALIBRATION_DIRECTION_NORMAL : PRIMALUCE_CALIBRATION_DIRECTION_INVERTED;
		PRIVATE_DATA->calibration_direction = direction;
		PRIVATE_DATA->calibration_step = PRIVATE_DATA->calibration_polls = 0;
		PRIVATE_DATA->calibration_starting = true;
		if (PRIVATE_DATA->is_sestosenso_3) {
			// A SESTO SENSO 3 restarts after every CAL_DIR write (firmware 1.0.9), inside a calibration as well,
			// so the direction is set first and only when it changes, and the calibration follows the restart.
			char *current = primalucelib_focuser_get_calibration_direction(LINK);
			bool change = current != NULL && strcmp(current, direction);
			if (current == NULL || (change && !primalucelib_focuser_set_calibration_direction(LINK, direction))) {
				focuser_calibration_failed(device, NULL);
			} else if (change) {
				primalucelib_begin_restart(LINK);
				indigo_update_property(device, X_CALIBRATE_F_PROPERTY, "The focuser restarts to change the calibration direction");
				indigo_execute_handler_in(device, PRIMALUCE_RESTART_POLL_INTERVAL, focuser_calibration_restart_finalizer);
			} else {
				focuser_calibration_start_finalizer(device);
			}
		} else {
			focuser_calibration_start_finalizer(device);
		}
	} else if (X_CALIBRATE_F_END_ITEM->sw.value) {
		X_CALIBRATE_F_START_ITEM->sw.value = X_CALIBRATE_F_START_INVERTED_ITEM->sw.value = X_CALIBRATE_F_END_ITEM->sw.value = false;
		PRIVATE_DATA->calibrating = false;
		// A SESTO SENSO 3 stores the maximum only after the calibration run is stopped.
		bool result = !PRIVATE_DATA->is_sestosenso_3 || primalucelib_focuser_calibration_step(LINK, PRIMALUCE_FOCUSER_CALIBRATION_STOP_MOTOR);
		// The previous maximum tells a stale range from the new one, see focuser_calibration_end_finalizer().
		double previous_min = 0, previous_max = 0;
		if (result) {
			primalucelib_focuser_get_calibration(LINK, &previous_min, &previous_max);
			result = primalucelib_focuser_calibration_step(LINK, PRIMALUCE_FOCUSER_CALIBRATION_STORE_MAX);
		}
		if (result) {
			PRIVATE_DATA->calibration_previous_max = previous_max;
			PRIVATE_DATA->calibration_polls = 0;
			indigo_update_property(device, X_CALIBRATE_F_PROPERTY, NULL);
			indigo_execute_handler_in(device, CALIBRATION_RANGE_POLL_INTERVAL, focuser_calibration_end_finalizer);
		} else {
			focuser_calibration_failed(device, NULL);
		}
	} else {
		PRIVATE_DATA->calibration_pending = false;
		X_CALIBRATE_F_PROPERTY->state = INDIGO_OK_STATE;
		indigo_update_property(device, X_CALIBRATE_F_PROPERTY, NULL);
	}
	//- focuser.X_CALIBRATE_F.on_change
}

static void focuser_backlash_handler(indigo_device *device) {
	FOCUSER_BACKLASH_PROPERTY->state = INDIGO_OK_STATE;
	//+ focuser.FOCUSER_BACKLASH.on_change
	if (!primalucelib_focuser_set_backlash(LINK, (int)FOCUSER_BACKLASH_ITEM->number.target)) {
		// The controller keeps the backlash it had.
		FOCUSER_BACKLASH_ITEM->number.value = FOCUSER_BACKLASH_ITEM->number.target = PRIVATE_DATA->backlash;
		INDIGO_UPDATE_PROPERTY_STATE(FOCUSER_BACKLASH_PROPERTY, INDIGO_ALERT_STATE, NULL);
		return;
	}
	PRIVATE_DATA->backlash = (int)FOCUSER_BACKLASH_ITEM->number.target;
	//- focuser.FOCUSER_BACKLASH.on_change
	indigo_update_property(device, FOCUSER_BACKLASH_PROPERTY, NULL);
}

static void focuser_position_handler(indigo_device *device) {
	//+ focuser.FOCUSER_POSITION.on_change
	PRIVATE_DATA->pending = false;
	// Both motion properties are busy for every move, an absolute one included.
	if (FOCUSER_STEPS_PROPERTY->state != INDIGO_BUSY_STATE) {
		FOCUSER_STEPS_PROPERTY->state = INDIGO_BUSY_STATE;
		indigo_update_property(device, FOCUSER_STEPS_PROPERTY, NULL);
	}
	PRIVATE_DATA->external_motion = PRIVATE_DATA->poll_alert = false;
	PRIVATE_DATA->stalled_polls = PRIVATE_DATA->poll_failures = 0;
	// The request replaced the published value; the focuser is still where it was read last.
	FOCUSER_POSITION_ITEM->number.value = PRIVATE_DATA->last_position = PRIVATE_DATA->position;
	if (PRIVATE_DATA->abort_requested) {
		// An abort overtook this move while it was queued, so it is never sent.
		primalucelib_clear_last_error(LINK);
		focuser_motion_failed(device, NULL);
	} else if (PRIVATE_DATA->is_sestosenso_3 && !PRIVATE_DATA->calibrated) {
		// An uncalibrated SESTO SENSO 3 acknowledges GOTO with "done" and does not move.
		focuser_motion_refused(device, "The focuser is not calibrated");
	} else {
		char *state = NULL;
		if (!primalucelib_focuser_move_to(LINK, PRIVATE_DATA->is_sestosenso_3, (int)FOCUSER_POSITION_ITEM->number.target, &state)) {
			focuser_motion_failed(device, NULL);
		} else if (!primalucelib_is_done(state)) {
			focuser_motion_failed(device, state);
		} else {
			indigo_execute_handler(device, focuser_movement_finalizer);
		}
	}
	//- focuser.FOCUSER_POSITION.on_change
}

static void focuser_steps_handler(indigo_device *device) {
	FOCUSER_STEPS_PROPERTY->state = INDIGO_OK_STATE;
	//+ focuser.FOCUSER_STEPS.on_change
	int steps = FOCUSER_DIRECTION_MOVE_OUTWARD_ITEM->sw.value ? (int)FOCUSER_STEPS_ITEM->number.target : -(int)FOCUSER_STEPS_ITEM->number.target;
	FOCUSER_POSITION_ITEM->number.target = FOCUSER_POSITION_ITEM->number.value + steps;
	// A move longer than the remaining travel stops at the calibrated end.
	if (FOCUSER_POSITION_ITEM->number.target < FOCUSER_POSITION_ITEM->number.min) {
		FOCUSER_POSITION_ITEM->number.target = FOCUSER_POSITION_ITEM->number.min;
	} else if (FOCUSER_POSITION_ITEM->number.target > FOCUSER_POSITION_ITEM->number.max) {
		FOCUSER_POSITION_ITEM->number.target = FOCUSER_POSITION_ITEM->number.max;
	}
	// The relative move is carried out by the absolute move handler, so both motion
	// properties have to be published as busy here and both have to stay busy until
	// the shared finalizer completes them.
	FOCUSER_POSITION_PROPERTY->state = FOCUSER_STEPS_PROPERTY->state = INDIGO_BUSY_STATE;
	indigo_update_property(device, FOCUSER_POSITION_PROPERTY, NULL);
	focuser_position_handler(device);
	//- focuser.FOCUSER_STEPS.on_change
	indigo_update_property(device, FOCUSER_STEPS_PROPERTY, NULL);
}

static void focuser_speed_handler(indigo_device *device) {
	FOCUSER_SPEED_PROPERTY->state = INDIGO_OK_STATE;
	//+ focuser.FOCUSER_SPEED.on_change
	if (!primalucelib_focuser_set_speed(LINK, (int)FOCUSER_SPEED_ITEM->number.target)) {
		// The controller keeps the speed it had.
		FOCUSER_SPEED_ITEM->number.value = FOCUSER_SPEED_ITEM->number.target = PRIVATE_DATA->speed;
		INDIGO_UPDATE_PROPERTY_STATE(FOCUSER_SPEED_PROPERTY, INDIGO_ALERT_STATE, NULL);
		return;
	}
	PRIVATE_DATA->speed = (int)FOCUSER_SPEED_ITEM->number.target;
	//- focuser.FOCUSER_SPEED.on_change
	indigo_update_property(device, FOCUSER_SPEED_PROPERTY, NULL);
}

static void focuser_abort_motion_handler(indigo_device *device) {
	//+ focuser.FOCUSER_ABORT_MOTION.on_change
	// A running motion is owned by the focuser movement finalizer, so the motion
	// properties are left busy here. That finalizer observes the stop, reads the position
	// the draw tube actually reached and publishes it as value, ALERT because the requested
	// target, which stays published, was not reached. A move still queued behind this abort
	// is never sent.
	bool requested = FOCUSER_ABORT_MOTION_ITEM->sw.value;
	FOCUSER_ABORT_MOTION_ITEM->sw.value = false;
	bool moving = PRIVATE_DATA->pending || FOCUSER_POSITION_PROPERTY->state == INDIGO_BUSY_STATE || FOCUSER_STEPS_PROPERTY->state == INDIGO_BUSY_STATE;
	if (requested && PRIVATE_DATA->calibration_starting) {
		// The calibration has not started to move yet: the pending step is cancelled, and a SESTO SENSO 3 that
		// already entered the procedure leaves it uncalibrated. A restart in progress is still followed by
		// focuser_calibration_restart_finalizer, which reopens the link once the controller answers.
		indigo_cancel_pending_handler(device, focuser_calibration_start_finalizer);
		if (PRIVATE_DATA->is_sestosenso_3 && PRIVATE_DATA->calibration_step > 0) {
			if (primalucelib_focuser_calibration_step(LINK, PRIMALUCE_FOCUSER_CALIBRATION_AUTO_STOP)) {
				focuser_set_range(device, 0, 0, true);
			}
		}
		focuser_calibration_failed(device, "Calibration aborted");
	} else if (requested && !moving && !PRIVATE_DATA->calibrating && primalucelib_link_failed(LINK)) {
		// Nothing to stop, but the controller did not answer the last request.
		INDIGO_UPDATE_PROPERTY_STATE(FOCUSER_ABORT_MOTION_PROPERTY, INDIGO_ALERT_STATE, NULL);
		return;
	} else if (requested && PRIVATE_DATA->calibrating && PRIVATE_DATA->is_sestosenso_3) {
		// A SESTO SENSO 3 calibration run stops only on its own StopMotor; stop_calib then leaves the
		// procedure and the focuser uncalibrated.
		PRIVATE_DATA->calibrating = false;
		X_CALIBRATE_F_START_ITEM->sw.value = X_CALIBRATE_F_START_INVERTED_ITEM->sw.value = X_CALIBRATE_F_END_ITEM->sw.value = false;
		if (!primalucelib_focuser_calibration_step(LINK, PRIMALUCE_FOCUSER_CALIBRATION_STOP_MOTOR) || !primalucelib_focuser_calibration_step(LINK, PRIMALUCE_FOCUSER_CALIBRATION_AUTO_STOP)) {
			INDIGO_UPDATE_PROPERTY_STATE(X_CALIBRATE_F_PROPERTY, INDIGO_ALERT_STATE, NULL);
			INDIGO_UPDATE_PROPERTY_STATE(FOCUSER_ABORT_MOTION_PROPERTY, INDIGO_ALERT_STATE, NULL);
			return;
		}
		focuser_set_range(device, 0, 0, true);
		INDIGO_UPDATE_PROPERTY_STATE(X_CALIBRATE_F_PROPERTY, INDIGO_ALERT_STATE, "Calibration aborted, the focuser is not calibrated");
	} else if (requested && (moving || PRIVATE_DATA->calibrating)) {
		char *reason;
		if (!primalucelib_focuser_stop(LINK, &reason)) {
			INDIGO_UPDATE_PROPERTY_STATE(FOCUSER_ABORT_MOTION_PROPERTY, INDIGO_ALERT_STATE, power_message(reason));
			return;
		}
		if (moving && !PRIVATE_DATA->external_motion) {
			PRIVATE_DATA->abort_requested = true;
		}
		if (PRIVATE_DATA->calibrating) {
			PRIVATE_DATA->calibrating = false;
			X_CALIBRATE_F_START_ITEM->sw.value = X_CALIBRATE_F_START_INVERTED_ITEM->sw.value = X_CALIBRATE_F_END_ITEM->sw.value = false;
			INDIGO_UPDATE_PROPERTY_STATE(X_CALIBRATE_F_PROPERTY, INDIGO_ALERT_STATE, "Calibration aborted");
		}
	}
	// Cancelling the calibration finalizers makes this handler publish its own completion.
	INDIGO_UPDATE_PROPERTY_STATE(FOCUSER_ABORT_MOTION_PROPERTY, INDIGO_OK_STATE, NULL);
	//- focuser.FOCUSER_ABORT_MOTION.on_change
}

#pragma mark - Device API (focuser)

static indigo_result focuser_enumerate_properties(indigo_device *device, indigo_client *client, indigo_property *property);

static indigo_result focuser_attach(indigo_device *device) {
	if (indigo_focuser_attach(device, DRIVER_NAME, DRIVER_VERSION) == INDIGO_OK) {
		ADDITIONAL_INSTANCES_PROPERTY->hidden = device->base_device != NULL;
		DEVICE_PORT_PROPERTY->hidden = false;
		DEVICE_PORTS_PROPERTY->hidden = false;
		indigo_enumerate_serial_ports(device, DEVICE_PORTS_PROPERTY);
		//+ focuser.on_attach
		INFO_PROPERTY->count = 8;
		//- focuser.on_attach
		X_CONFIG_PROPERTY = indigo_init_number_property(NULL, device->name, X_CONFIG_PROPERTY_NAME, FOCUSER_ADVANCED_GROUP, "Configuration", INDIGO_OK_STATE, INDIGO_RO_PERM, 7);
		if (X_CONFIG_PROPERTY == NULL) {
			return INDIGO_FAILED;
		}
		indigo_init_number_item(X_CONFIG_M1ACC_ITEM, X_CONFIG_M1ACC_ITEM_NAME, "Acceleration speed", 0, 10, 0, 0);
		indigo_init_number_item(X_CONFIG_M1SPD_ITEM, X_CONFIG_M1SPD_ITEM_NAME, "Run speed", 0, 10, 0, 0);
		indigo_init_number_item(X_CONFIG_M1DEC_ITEM, X_CONFIG_M1DEC_ITEM_NAME, "Deceleration speed", 0, 10, 0, 0);
		indigo_init_number_item(X_CONFIG_M1CACC_ITEM, X_CONFIG_M1CACC_ITEM_NAME, "Acceleration current", 0, 10, 0, 0);
		indigo_init_number_item(X_CONFIG_M1CSPD_ITEM, X_CONFIG_M1CSPD_ITEM_NAME, "Run current", 0, 10, 0, 0);
		indigo_init_number_item(X_CONFIG_M1CDEC_ITEM, X_CONFIG_M1CDEC_ITEM_NAME, "Deceleration current", 0, 10, 0, 0);
		indigo_init_number_item(X_CONFIG_M1HOLD_ITEM, X_CONFIG_M1HOLD_ITEM_NAME, "Hold current", 0, 10, 0, 0);
		X_STATE_PROPERTY = indigo_init_number_property(NULL, device->name, X_STATE_PROPERTY_NAME, FOCUSER_ADVANCED_GROUP, "State", INDIGO_OK_STATE, INDIGO_RO_PERM, 3);
		if (X_STATE_PROPERTY == NULL) {
			return INDIGO_FAILED;
		}
		indigo_init_number_item(X_STATE_MOTOR_TEMP_ITEM, X_STATE_MOTOR_TEMP_ITEM_NAME, "Motor emperature (°C)", -50, 100, 0, 0);
		indigo_init_number_item(X_STATE_VIN_12V_ITEM, X_STATE_VIN_12V_ITEM_NAME, "12V power (V)", 0, 50, 0, 0);
		indigo_init_number_item(X_STATE_VIN_USB_ITEM, X_STATE_VIN_USB_ITEM_NAME, "USB power (V)", 0, 10, 0, 0);
		X_WIFI_PROPERTY = indigo_init_switch_property(NULL, device->name, X_WIFI_PROPERTY_NAME, FOCUSER_ADVANCED_GROUP, "WiFi mode", INDIGO_OK_STATE, INDIGO_RW_PERM, INDIGO_ONE_OF_MANY_RULE, 3);
		if (X_WIFI_PROPERTY == NULL) {
			return INDIGO_FAILED;
		}
		indigo_init_switch_item(X_WIFI_OFF_ITEM, X_WIFI_OFF_ITEM_NAME, "Off", true);
		indigo_init_switch_item(X_WIFI_AP_ITEM, X_WIFI_AP_ITEM_NAME, "Access Point mode", false);
		indigo_init_switch_item(X_WIFI_STA_ITEM, X_WIFI_STA_ITEM_NAME, "Station mode", false);
		X_WIFI_AP_PROPERTY = indigo_init_text_property(NULL, device->name, X_WIFI_AP_PROPERTY_NAME, FOCUSER_ADVANCED_GROUP, "AP WiFi settings", INDIGO_OK_STATE, INDIGO_RW_PERM, 2);
		if (X_WIFI_AP_PROPERTY == NULL) {
			return INDIGO_FAILED;
		}
		indigo_init_text_item(X_WIFI_AP_SSID_ITEM, X_WIFI_AP_SSID_ITEM_NAME, "SSID", "");
		indigo_init_text_item(X_WIFI_AP_PASSWORD_ITEM, X_WIFI_AP_PASSWORD_ITEM_NAME, "Password", "");
		X_WIFI_STA_PROPERTY = indigo_init_text_property(NULL, device->name, X_WIFI_STA_PROPERTY_NAME, FOCUSER_ADVANCED_GROUP, "STA WiFi settings", INDIGO_OK_STATE, INDIGO_RW_PERM, 2);
		if (X_WIFI_STA_PROPERTY == NULL) {
			return INDIGO_FAILED;
		}
		indigo_init_text_item(X_WIFI_STA_SSID_ITEM, X_WIFI_STA_SSID_ITEM_NAME, "SSID", "");
		indigo_init_text_item(X_WIFI_STA_PASSWORD_ITEM, X_WIFI_STA_PASSWORD_ITEM_NAME, "Password", "");
		X_LEDS_PROPERTY = indigo_init_switch_property(NULL, device->name, X_LEDS_PROPERTY_NAME, FOCUSER_ADVANCED_GROUP, "LEDs", INDIGO_OK_STATE, INDIGO_RW_PERM, INDIGO_ONE_OF_MANY_RULE, 4);
		if (X_LEDS_PROPERTY == NULL) {
			return INDIGO_FAILED;
		}
		indigo_init_switch_item(X_LEDS_OFF_ITEM, X_LEDS_OFF_ITEM_NAME, "Off", true);
		indigo_init_switch_item(X_LEDS_DIM_ITEM, X_LEDS_DIM_ITEM_NAME, "Dim", false);
		indigo_init_switch_item(X_LEDS_MIDDLE_ITEM, X_LEDS_MIDDLE_ITEM_NAME, "Middle", false);
		indigo_init_switch_item(X_LEDS_ON_ITEM, X_LEDS_ON_ITEM_NAME, "On", false);
		X_RUNPRESET_L_PROPERTY = indigo_init_number_property(NULL, device->name, X_RUNPRESET_L_PROPERTY_NAME, FOCUSER_ADVANCED_GROUP, "Preset light", INDIGO_OK_STATE, INDIGO_RO_PERM, 7);
		if (X_RUNPRESET_L_PROPERTY == NULL) {
			return INDIGO_FAILED;
		}
		indigo_init_number_item(X_RUNPRESET_L_M1ACC_ITEM, X_RUNPRESET_L_M1ACC_ITEM_NAME, "Acceleration speed", 0, 10, 0, 0);
		indigo_init_number_item(X_RUNPRESET_L_M1SPD_ITEM, X_RUNPRESET_L_M1SPD_ITEM_NAME, "Run speed", 0, 10, 0, 0);
		indigo_init_number_item(X_RUNPRESET_L_M1DEC_ITEM, X_RUNPRESET_L_M1DEC_ITEM_NAME, "Deceleration speed", 0, 10, 0, 0);
		indigo_init_number_item(X_RUNPRESET_L_M1CACC_ITEM, X_RUNPRESET_L_M1CACC_ITEM_NAME, "Acceleration current", 0, 10, 0, 0);
		indigo_init_number_item(X_RUNPRESET_L_M1CSPD_ITEM, X_RUNPRESET_L_M1CSPD_ITEM_NAME, "Run current", 0, 10, 0, 0);
		indigo_init_number_item(X_RUNPRESET_L_M1CDEC_ITEM, X_RUNPRESET_L_M1CDEC_ITEM_NAME, "Deceleration current", 0, 10, 0, 0);
		indigo_init_number_item(X_RUNPRESET_L_M1HOLD_ITEM, X_RUNPRESET_L_M1HOLD_ITEM_NAME, "Hold current", 0, 10, 0, 0);
		X_RUNPRESET_M_PROPERTY = indigo_init_number_property(NULL, device->name, X_RUNPRESET_M_PROPERTY_NAME, FOCUSER_ADVANCED_GROUP, "Preset medium", INDIGO_OK_STATE, INDIGO_RO_PERM, 7);
		if (X_RUNPRESET_M_PROPERTY == NULL) {
			return INDIGO_FAILED;
		}
		indigo_init_number_item(X_RUNPRESET_M_M1ACC_ITEM, X_RUNPRESET_M_M1ACC_ITEM_NAME, "Acceleration speed", 0, 10, 0, 0);
		indigo_init_number_item(X_RUNPRESET_M_M1SPD_ITEM, X_RUNPRESET_M_M1SPD_ITEM_NAME, "Run speed", 0, 10, 0, 0);
		indigo_init_number_item(X_RUNPRESET_M_M1DEC_ITEM, X_RUNPRESET_M_M1DEC_ITEM_NAME, "Deceleration speed", 0, 10, 0, 0);
		indigo_init_number_item(X_RUNPRESET_M_M1CACC_ITEM, X_RUNPRESET_M_M1CACC_ITEM_NAME, "Acceleration current", 0, 10, 0, 0);
		indigo_init_number_item(X_RUNPRESET_M_M1CSPD_ITEM, X_RUNPRESET_M_M1CSPD_ITEM_NAME, "Run current", 0, 10, 0, 0);
		indigo_init_number_item(X_RUNPRESET_M_M1CDEC_ITEM, X_RUNPRESET_M_M1CDEC_ITEM_NAME, "Deceleration current", 0, 10, 0, 0);
		indigo_init_number_item(X_RUNPRESET_M_M1HOLD_ITEM, X_RUNPRESET_M_M1HOLD_ITEM_NAME, "Hold current", 0, 10, 0, 0);
		X_RUNPRESET_S_PROPERTY = indigo_init_number_property(NULL, device->name, X_RUNPRESET_S_PROPERTY_NAME, FOCUSER_ADVANCED_GROUP, "Preset slow", INDIGO_OK_STATE, INDIGO_RO_PERM, 7);
		if (X_RUNPRESET_S_PROPERTY == NULL) {
			return INDIGO_FAILED;
		}
		indigo_init_number_item(X_RUNPRESET_S_M1ACC_ITEM, X_RUNPRESET_S_M1ACC_ITEM_NAME, "Acceleration speed", 0, 10, 0, 0);
		indigo_init_number_item(X_RUNPRESET_S_M1SPD_ITEM, X_RUNPRESET_S_M1SPD_ITEM_NAME, "Run speed", 0, 10, 0, 0);
		indigo_init_number_item(X_RUNPRESET_S_M1DEC_ITEM, X_RUNPRESET_S_M1DEC_ITEM_NAME, "Deceleration speed", 0, 10, 0, 0);
		indigo_init_number_item(X_RUNPRESET_S_M1CACC_ITEM, X_RUNPRESET_S_M1CACC_ITEM_NAME, "Acceleration current", 0, 10, 0, 0);
		indigo_init_number_item(X_RUNPRESET_S_M1CSPD_ITEM, X_RUNPRESET_S_M1CSPD_ITEM_NAME, "Run current", 0, 10, 0, 0);
		indigo_init_number_item(X_RUNPRESET_S_M1CDEC_ITEM, X_RUNPRESET_S_M1CDEC_ITEM_NAME, "Deceleration current", 0, 10, 0, 0);
		indigo_init_number_item(X_RUNPRESET_S_M1HOLD_ITEM, X_RUNPRESET_S_M1HOLD_ITEM_NAME, "Hold current", 0, 10, 0, 0);
		X_RUNPRESET_1_PROPERTY = indigo_init_number_property(NULL, device->name, X_RUNPRESET_1_PROPERTY_NAME, FOCUSER_ADVANCED_GROUP, "Preset #1", INDIGO_OK_STATE, INDIGO_RW_PERM, 7);
		if (X_RUNPRESET_1_PROPERTY == NULL) {
			return INDIGO_FAILED;
		}
		indigo_init_number_item(X_RUNPRESET_1_M1ACC_ITEM, X_RUNPRESET_1_M1ACC_ITEM_NAME, "Acceleration speed", 0, 10, 0, 0);
		indigo_init_number_item(X_RUNPRESET_1_M1SPD_ITEM, X_RUNPRESET_1_M1SPD_ITEM_NAME, "Run speed", 0, 10, 0, 0);
		indigo_init_number_item(X_RUNPRESET_1_M1DEC_ITEM, X_RUNPRESET_1_M1DEC_ITEM_NAME, "Deceleration speed", 0, 10, 0, 0);
		indigo_init_number_item(X_RUNPRESET_1_M1CACC_ITEM, X_RUNPRESET_1_M1CACC_ITEM_NAME, "Acceleration current", 0, 10, 0, 0);
		indigo_init_number_item(X_RUNPRESET_1_M1CSPD_ITEM, X_RUNPRESET_1_M1CSPD_ITEM_NAME, "Run current", 0, 10, 0, 0);
		indigo_init_number_item(X_RUNPRESET_1_M1CDEC_ITEM, X_RUNPRESET_1_M1CDEC_ITEM_NAME, "Deceleration current", 0, 10, 0, 0);
		indigo_init_number_item(X_RUNPRESET_1_M1HOLD_ITEM, X_RUNPRESET_1_M1HOLD_ITEM_NAME, "Hold current", 0, 10, 0, 0);
		X_RUNPRESET_2_PROPERTY = indigo_init_number_property(NULL, device->name, X_RUNPRESET_2_PROPERTY_NAME, FOCUSER_ADVANCED_GROUP, "Preset #2", INDIGO_OK_STATE, INDIGO_RW_PERM, 7);
		if (X_RUNPRESET_2_PROPERTY == NULL) {
			return INDIGO_FAILED;
		}
		indigo_init_number_item(X_RUNPRESET_2_M1ACC_ITEM, X_RUNPRESET_2_M1ACC_ITEM_NAME, "Acceleration speed", 0, 10, 0, 0);
		indigo_init_number_item(X_RUNPRESET_2_M1SPD_ITEM, X_RUNPRESET_2_M1SPD_ITEM_NAME, "Run speed", 0, 10, 0, 0);
		indigo_init_number_item(X_RUNPRESET_2_M1DEC_ITEM, X_RUNPRESET_2_M1DEC_ITEM_NAME, "Deceleration speed", 0, 10, 0, 0);
		indigo_init_number_item(X_RUNPRESET_2_M1CACC_ITEM, X_RUNPRESET_2_M1CACC_ITEM_NAME, "Acceleration current", 0, 10, 0, 0);
		indigo_init_number_item(X_RUNPRESET_2_M1CSPD_ITEM, X_RUNPRESET_2_M1CSPD_ITEM_NAME, "Run current", 0, 10, 0, 0);
		indigo_init_number_item(X_RUNPRESET_2_M1CDEC_ITEM, X_RUNPRESET_2_M1CDEC_ITEM_NAME, "Deceleration current", 0, 10, 0, 0);
		indigo_init_number_item(X_RUNPRESET_2_M1HOLD_ITEM, X_RUNPRESET_2_M1HOLD_ITEM_NAME, "Hold current", 0, 10, 0, 0);
		X_RUNPRESET_3_PROPERTY = indigo_init_number_property(NULL, device->name, X_RUNPRESET_3_PROPERTY_NAME, FOCUSER_ADVANCED_GROUP, "Preset #3", INDIGO_OK_STATE, INDIGO_RW_PERM, 7);
		if (X_RUNPRESET_3_PROPERTY == NULL) {
			return INDIGO_FAILED;
		}
		indigo_init_number_item(X_RUNPRESET_3_M1ACC_ITEM, X_RUNPRESET_3_M1ACC_ITEM_NAME, "Acceleration speed", 0, 10, 0, 0);
		indigo_init_number_item(X_RUNPRESET_3_M1SPD_ITEM, X_RUNPRESET_3_M1SPD_ITEM_NAME, "Run speed", 0, 10, 0, 0);
		indigo_init_number_item(X_RUNPRESET_3_M1DEC_ITEM, X_RUNPRESET_3_M1DEC_ITEM_NAME, "Deceleration speed", 0, 10, 0, 0);
		indigo_init_number_item(X_RUNPRESET_3_M1CACC_ITEM, X_RUNPRESET_3_M1CACC_ITEM_NAME, "Acceleration current", 0, 10, 0, 0);
		indigo_init_number_item(X_RUNPRESET_3_M1CSPD_ITEM, X_RUNPRESET_3_M1CSPD_ITEM_NAME, "Run current", 0, 10, 0, 0);
		indigo_init_number_item(X_RUNPRESET_3_M1CDEC_ITEM, X_RUNPRESET_3_M1CDEC_ITEM_NAME, "Deceleration current", 0, 10, 0, 0);
		indigo_init_number_item(X_RUNPRESET_3_M1HOLD_ITEM, X_RUNPRESET_3_M1HOLD_ITEM_NAME, "Hold current", 0, 10, 0, 0);
		X_RUNPRESET_PROPERTY = indigo_init_switch_property(NULL, device->name, X_RUNPRESET_PROPERTY_NAME, FOCUSER_ADVANCED_GROUP, "Presets", INDIGO_OK_STATE, INDIGO_RW_PERM, INDIGO_ONE_OF_MANY_RULE, 6);
		if (X_RUNPRESET_PROPERTY == NULL) {
			return INDIGO_FAILED;
		}
		indigo_init_switch_item(X_RUNPRESET_L_ITEM, X_RUNPRESET_L_ITEM_NAME, "Preset light", false);
		indigo_init_switch_item(X_RUNPRESET_M_ITEM, X_RUNPRESET_M_ITEM_NAME, "Preset medium", false);
		indigo_init_switch_item(X_RUNPRESET_S_ITEM, X_RUNPRESET_S_ITEM_NAME, "Preset slow", false);
		indigo_init_switch_item(X_RUNPRESET_1_ITEM, X_RUNPRESET_1_ITEM_NAME, "Preset #1", false);
		indigo_init_switch_item(X_RUNPRESET_2_ITEM, X_RUNPRESET_2_ITEM_NAME, "Preset #2", false);
		indigo_init_switch_item(X_RUNPRESET_3_ITEM, X_RUNPRESET_3_ITEM_NAME, "Preset #3", false);
		X_PRESETS_PROPERTY = indigo_init_number_property(NULL, device->name, X_PRESETS_PROPERTY_NAME, FOCUSER_ADVANCED_GROUP, "Stored positions", INDIGO_OK_STATE, INDIGO_RW_PERM, 9);
		if (X_PRESETS_PROPERTY == NULL) {
			return INDIGO_FAILED;
		}
		indigo_init_number_item(X_PRESETS_1_ITEM, X_PRESETS_1_ITEM_NAME, "Preset #1", 0, 1000000, 1, 0);
		indigo_init_number_item(X_PRESETS_2_ITEM, X_PRESETS_2_ITEM_NAME, "Preset #2", 0, 1000000, 1, 0);
		indigo_init_number_item(X_PRESETS_3_ITEM, X_PRESETS_3_ITEM_NAME, "Preset #3", 0, 1000000, 1, 0);
		indigo_init_number_item(X_PRESETS_4_ITEM, X_PRESETS_4_ITEM_NAME, "Preset #4", 0, 1000000, 1, 0);
		indigo_init_number_item(X_PRESETS_5_ITEM, X_PRESETS_5_ITEM_NAME, "Preset #5", 0, 1000000, 1, 0);
		indigo_init_number_item(X_PRESETS_6_ITEM, X_PRESETS_6_ITEM_NAME, "Preset #6", 0, 1000000, 1, 0);
		indigo_init_number_item(X_PRESETS_7_ITEM, X_PRESETS_7_ITEM_NAME, "Preset #7", 0, 1000000, 1, 0);
		indigo_init_number_item(X_PRESETS_8_ITEM, X_PRESETS_8_ITEM_NAME, "Preset #8", 0, 1000000, 1, 0);
		indigo_init_number_item(X_PRESETS_9_ITEM, X_PRESETS_9_ITEM_NAME, "Preset #9", 0, 1000000, 1, 0);
		X_PRESET_NAMES_PROPERTY = indigo_init_text_property(NULL, device->name, X_PRESET_NAMES_PROPERTY_NAME, FOCUSER_ADVANCED_GROUP, "Stored position names", INDIGO_OK_STATE, INDIGO_RW_PERM, 9);
		if (X_PRESET_NAMES_PROPERTY == NULL) {
			return INDIGO_FAILED;
		}
		indigo_init_text_item(X_PRESET_NAMES_1_ITEM, X_PRESET_NAMES_1_ITEM_NAME, "Preset #1", "");
		indigo_init_text_item(X_PRESET_NAMES_2_ITEM, X_PRESET_NAMES_2_ITEM_NAME, "Preset #2", "");
		indigo_init_text_item(X_PRESET_NAMES_3_ITEM, X_PRESET_NAMES_3_ITEM_NAME, "Preset #3", "");
		indigo_init_text_item(X_PRESET_NAMES_4_ITEM, X_PRESET_NAMES_4_ITEM_NAME, "Preset #4", "");
		indigo_init_text_item(X_PRESET_NAMES_5_ITEM, X_PRESET_NAMES_5_ITEM_NAME, "Preset #5", "");
		indigo_init_text_item(X_PRESET_NAMES_6_ITEM, X_PRESET_NAMES_6_ITEM_NAME, "Preset #6", "");
		indigo_init_text_item(X_PRESET_NAMES_7_ITEM, X_PRESET_NAMES_7_ITEM_NAME, "Preset #7", "");
		indigo_init_text_item(X_PRESET_NAMES_8_ITEM, X_PRESET_NAMES_8_ITEM_NAME, "Preset #8", "");
		indigo_init_text_item(X_PRESET_NAMES_9_ITEM, X_PRESET_NAMES_9_ITEM_NAME, "Preset #9", "");
		X_PRESET_GOTO_PROPERTY = indigo_init_switch_property(NULL, device->name, X_PRESET_GOTO_PROPERTY_NAME, FOCUSER_ADVANCED_GROUP, "Go to stored position", INDIGO_OK_STATE, INDIGO_RW_PERM, INDIGO_ONE_OF_MANY_RULE, 9);
		if (X_PRESET_GOTO_PROPERTY == NULL) {
			return INDIGO_FAILED;
		}
		indigo_init_switch_item(X_PRESET_GOTO_1_ITEM, X_PRESET_GOTO_1_ITEM_NAME, "Preset #1", false);
		indigo_init_switch_item(X_PRESET_GOTO_2_ITEM, X_PRESET_GOTO_2_ITEM_NAME, "Preset #2", false);
		indigo_init_switch_item(X_PRESET_GOTO_3_ITEM, X_PRESET_GOTO_3_ITEM_NAME, "Preset #3", false);
		indigo_init_switch_item(X_PRESET_GOTO_4_ITEM, X_PRESET_GOTO_4_ITEM_NAME, "Preset #4", false);
		indigo_init_switch_item(X_PRESET_GOTO_5_ITEM, X_PRESET_GOTO_5_ITEM_NAME, "Preset #5", false);
		indigo_init_switch_item(X_PRESET_GOTO_6_ITEM, X_PRESET_GOTO_6_ITEM_NAME, "Preset #6", false);
		indigo_init_switch_item(X_PRESET_GOTO_7_ITEM, X_PRESET_GOTO_7_ITEM_NAME, "Preset #7", false);
		indigo_init_switch_item(X_PRESET_GOTO_8_ITEM, X_PRESET_GOTO_8_ITEM_NAME, "Preset #8", false);
		indigo_init_switch_item(X_PRESET_GOTO_9_ITEM, X_PRESET_GOTO_9_ITEM_NAME, "Preset #9", false);
		X_HOLD_CURR_PROPERTY = indigo_init_switch_property(NULL, device->name, X_HOLD_CURR_PROPERTY_NAME, FOCUSER_ADVANCED_GROUP, "Hold current", INDIGO_OK_STATE, INDIGO_RW_PERM, INDIGO_ONE_OF_MANY_RULE, 2);
		if (X_HOLD_CURR_PROPERTY == NULL) {
			return INDIGO_FAILED;
		}
		indigo_init_switch_item(X_HOLD_CURR_OFF_ITEM, X_HOLD_CURR_OFF_ITEM_NAME, "Off", true);
		indigo_init_switch_item(X_HOLD_CURR_ON_ITEM, X_HOLD_CURR_ON_ITEM_NAME, "On", false);
		X_CALIBRATE_F_PROPERTY = indigo_init_switch_property(NULL, device->name, X_CALIBRATE_F_PROPERTY_NAME, FOCUSER_ADVANCED_GROUP, "Calibrate focuser", INDIGO_OK_STATE, INDIGO_RW_PERM, INDIGO_ONE_OF_MANY_RULE, 3);
		if (X_CALIBRATE_F_PROPERTY == NULL) {
			return INDIGO_FAILED;
		}
		indigo_init_switch_item(X_CALIBRATE_F_START_ITEM, X_CALIBRATE_F_START_ITEM_NAME, "Start", false);
		indigo_init_switch_item(X_CALIBRATE_F_START_INVERTED_ITEM, X_CALIBRATE_F_START_INVERTED_ITEM_NAME, "Start inverted", false);
		indigo_init_switch_item(X_CALIBRATE_F_END_ITEM, X_CALIBRATE_F_END_ITEM_NAME, "End", false);
		FOCUSER_TEMPERATURE_PROPERTY->hidden = false;
		FOCUSER_BACKLASH_PROPERTY->hidden = false;
		FOCUSER_POSITION_PROPERTY->hidden = false;
		//+ focuser.FOCUSER_POSITION.on_attach
		FOCUSER_POSITION_ITEM->number.min = 0;
		FOCUSER_POSITION_ITEM->number.max = 1000000;
		strcpy(FOCUSER_POSITION_ITEM->number.format, "%.0f");
		//- focuser.FOCUSER_POSITION.on_attach
		FOCUSER_STEPS_PROPERTY->hidden = false;
		//+ focuser.FOCUSER_STEPS.on_attach
		FOCUSER_STEPS_ITEM->number.min = 0;
		FOCUSER_STEPS_ITEM->number.max = 1000000;
		strcpy(FOCUSER_STEPS_ITEM->number.format, "%.0f");
		//- focuser.FOCUSER_STEPS.on_attach
		FOCUSER_SPEED_PROPERTY->hidden = false;
		//+ focuser.FOCUSER_SPEED.on_attach
		FOCUSER_SPEED_ITEM->number.min = 0;
		FOCUSER_SPEED_ITEM->number.max = 0;
		//- focuser.FOCUSER_SPEED.on_attach
		FOCUSER_ABORT_MOTION_PROPERTY->hidden = false;
		INDIGO_DEVICE_ATTACH_LOG(DRIVER_NAME, device->name);
		return focuser_enumerate_properties(device, NULL, NULL);
	}
	return INDIGO_FAILED;
}

static indigo_result focuser_enumerate_properties(indigo_device *device, indigo_client *client, indigo_property *property) {
	if (IS_CONNECTED) {
		INDIGO_DEFINE_MATCHING_PROPERTY(X_CONFIG_PROPERTY);
		INDIGO_DEFINE_MATCHING_PROPERTY(X_STATE_PROPERTY);
		INDIGO_DEFINE_MATCHING_PROPERTY(X_WIFI_PROPERTY);
		INDIGO_DEFINE_MATCHING_PROPERTY(X_WIFI_AP_PROPERTY);
		INDIGO_DEFINE_MATCHING_PROPERTY(X_WIFI_STA_PROPERTY);
		INDIGO_DEFINE_MATCHING_PROPERTY(X_LEDS_PROPERTY);
		INDIGO_DEFINE_MATCHING_PROPERTY(X_RUNPRESET_L_PROPERTY);
		INDIGO_DEFINE_MATCHING_PROPERTY(X_RUNPRESET_M_PROPERTY);
		INDIGO_DEFINE_MATCHING_PROPERTY(X_RUNPRESET_S_PROPERTY);
		INDIGO_DEFINE_MATCHING_PROPERTY(X_RUNPRESET_1_PROPERTY);
		INDIGO_DEFINE_MATCHING_PROPERTY(X_RUNPRESET_2_PROPERTY);
		INDIGO_DEFINE_MATCHING_PROPERTY(X_RUNPRESET_3_PROPERTY);
		INDIGO_DEFINE_MATCHING_PROPERTY(X_RUNPRESET_PROPERTY);
		INDIGO_DEFINE_MATCHING_PROPERTY(X_PRESETS_PROPERTY);
		INDIGO_DEFINE_MATCHING_PROPERTY(X_PRESET_NAMES_PROPERTY);
		INDIGO_DEFINE_MATCHING_PROPERTY(X_PRESET_GOTO_PROPERTY);
		INDIGO_DEFINE_MATCHING_PROPERTY(X_HOLD_CURR_PROPERTY);
		INDIGO_DEFINE_MATCHING_PROPERTY(X_CALIBRATE_F_PROPERTY);
	}
	return indigo_focuser_enumerate_properties(device, client, property);
}

static indigo_result focuser_change_property(indigo_device *device, indigo_client *client, indigo_property *property) {
	if (indigo_property_match_changeable(CONNECTION_PROPERTY, property)) {
		INDIGO_PROCESS_CONNECT(focuser_connection_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(X_WIFI_PROPERTY, property)) {
		INDIGO_COPY_VALUES_PROCESS_CHANGE(X_WIFI_PROPERTY, focuser_x_wifi_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(X_WIFI_AP_PROPERTY, property)) {
		INDIGO_COPY_VALUES_PROCESS_CHANGE(X_WIFI_AP_PROPERTY, focuser_x_wifi_ap_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(X_WIFI_STA_PROPERTY, property)) {
		INDIGO_COPY_VALUES_PROCESS_CHANGE(X_WIFI_STA_PROPERTY, focuser_x_wifi_sta_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(X_LEDS_PROPERTY, property)) {
		INDIGO_COPY_VALUES_PROCESS_CHANGE(X_LEDS_PROPERTY, focuser_x_leds_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(X_RUNPRESET_1_PROPERTY, property)) {
		INDIGO_COPY_VALUES_PROCESS_CHANGE(X_RUNPRESET_1_PROPERTY, focuser_x_runpreset_1_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(X_RUNPRESET_2_PROPERTY, property)) {
		INDIGO_COPY_VALUES_PROCESS_CHANGE(X_RUNPRESET_2_PROPERTY, focuser_x_runpreset_2_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(X_RUNPRESET_3_PROPERTY, property)) {
		INDIGO_COPY_VALUES_PROCESS_CHANGE(X_RUNPRESET_3_PROPERTY, focuser_x_runpreset_3_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(X_RUNPRESET_PROPERTY, property)) {
		INDIGO_COPY_VALUES_PROCESS_CHANGE(X_RUNPRESET_PROPERTY, focuser_x_runpreset_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(X_PRESETS_PROPERTY, property)) {
		INDIGO_COPY_VALUES_PROCESS_CHANGE(X_PRESETS_PROPERTY, focuser_x_presets_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(X_PRESET_NAMES_PROPERTY, property)) {
		INDIGO_COPY_VALUES_PROCESS_CHANGE(X_PRESET_NAMES_PROPERTY, focuser_x_preset_names_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(X_PRESET_GOTO_PROPERTY, property)) {
		INDIGO_COPY_VALUES_PROCESS_CHANGE(X_PRESET_GOTO_PROPERTY, focuser_x_preset_goto_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(X_HOLD_CURR_PROPERTY, property)) {
		INDIGO_COPY_VALUES_PROCESS_CHANGE(X_HOLD_CURR_PROPERTY, focuser_x_hold_curr_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(X_CALIBRATE_F_PROPERTY, property)) {
		INDIGO_REJECT_CHANGE_IF(!PRIVATE_DATA->calibrating && (FOCUSER_POSITION_PROPERTY->state == INDIGO_BUSY_STATE || FOCUSER_STEPS_PROPERTY->state == INDIGO_BUSY_STATE), X_CALIBRATE_F_PROPERTY, "The focuser is moving");
		//+ focuser.X_CALIBRATE_F.on_change_request
		// The handler sets the property OK when it starts, so the framework BUSY guard does not refuse a request
		// while a step runs (a SESTO SENSO 3 START takes about 12 s with the restart); such a request would run
		// a second calibration after the first one. The run started by START or START_INVERTED accepts only END.
		bool end_requested = indigo_get_switch(property, X_CALIBRATE_F_END_ITEM_NAME);
		if (PRIVATE_DATA->calibration_pending) {
			indigo_reject_change(device, X_CALIBRATE_F_PROPERTY, "A calibration step is in progress");
			return INDIGO_OK;
		}
		if (PRIVATE_DATA->calibrating && !end_requested) {
			indigo_reject_change(device, X_CALIBRATE_F_PROPERTY, "The calibration is running, select End at the far end or abort the motion");
			return INDIGO_OK;
		}
		if (!PRIVATE_DATA->calibrating && end_requested) {
			indigo_reject_change(device, X_CALIBRATE_F_PROPERTY, "No calibration is running");
			return INDIGO_OK;
		}
		PRIVATE_DATA->calibration_pending = true;
		//- focuser.X_CALIBRATE_F.on_change_request
		INDIGO_COPY_VALUES_PROCESS_CHANGE(X_CALIBRATE_F_PROPERTY, focuser_x_calibrate_f_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(FOCUSER_BACKLASH_PROPERTY, property)) {
		INDIGO_REJECT_CHANGE_IF(FOCUSER_POSITION_PROPERTY->state == INDIGO_BUSY_STATE || FOCUSER_STEPS_PROPERTY->state == INDIGO_BUSY_STATE, FOCUSER_BACKLASH_PROPERTY, "The focuser is moving");
		INDIGO_COPY_VALUES_PROCESS_CHANGE(FOCUSER_BACKLASH_PROPERTY, focuser_backlash_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(FOCUSER_POSITION_PROPERTY, property)) {
		INDIGO_REJECT_CHANGE_IF(PRIVATE_DATA->pending || FOCUSER_POSITION_PROPERTY->state == INDIGO_BUSY_STATE || FOCUSER_STEPS_PROPERTY->state == INDIGO_BUSY_STATE, FOCUSER_POSITION_PROPERTY, "Another motion operation is pending");
		//+ focuser.FOCUSER_POSITION.on_change_request
		// The accepted move is pending until its handler runs. Both motion properties are busy from now on, so a
		// request refused meanwhile is answered with a BUSY message and does not end FOCUSER_STEPS ALERT.
		PRIVATE_DATA->pending = true;
		FOCUSER_STEPS_PROPERTY->state = INDIGO_BUSY_STATE;
		indigo_update_property(device, FOCUSER_STEPS_PROPERTY, NULL);
		//- focuser.FOCUSER_POSITION.on_change_request
		INDIGO_COPY_TARGETS_PROCESS_CHANGE(FOCUSER_POSITION_PROPERTY, focuser_position_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(FOCUSER_STEPS_PROPERTY, property)) {
		INDIGO_REJECT_CHANGE_IF(PRIVATE_DATA->pending || FOCUSER_POSITION_PROPERTY->state == INDIGO_BUSY_STATE || FOCUSER_STEPS_PROPERTY->state == INDIGO_BUSY_STATE, FOCUSER_STEPS_PROPERTY, "Another motion operation is pending");
		//+ focuser.FOCUSER_STEPS.on_change_request
		// The accepted move is pending until its handler runs; FOCUSER_POSITION is busy with it from now on.
		PRIVATE_DATA->pending = true;
		FOCUSER_POSITION_PROPERTY->state = INDIGO_BUSY_STATE;
		indigo_update_property(device, FOCUSER_POSITION_PROPERTY, NULL);
		//- focuser.FOCUSER_STEPS.on_change_request
		INDIGO_COPY_VALUES_PROCESS_CHANGE(FOCUSER_STEPS_PROPERTY, focuser_steps_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(FOCUSER_SPEED_PROPERTY, property)) {
		INDIGO_COPY_VALUES_PROCESS_CHANGE(FOCUSER_SPEED_PROPERTY, focuser_speed_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(FOCUSER_ABORT_MOTION_PROPERTY, property)) {
		INDIGO_COPY_VALUES_PROCESS_URGENT_CHANGE(FOCUSER_ABORT_MOTION_PROPERTY, focuser_abort_motion_handler);
		return INDIGO_OK;
	}
	return indigo_focuser_change_property(device, client, property);
}

static indigo_result focuser_detach(indigo_device *device) {
	if (IS_CONNECTED) {
		indigo_set_switch(CONNECTION_PROPERTY, CONNECTION_DISCONNECTED_ITEM, true);
		focuser_connection_handler(device);
	}
	indigo_release_property(X_CONFIG_PROPERTY);
	indigo_release_property(X_STATE_PROPERTY);
	indigo_release_property(X_WIFI_PROPERTY);
	indigo_release_property(X_WIFI_AP_PROPERTY);
	indigo_release_property(X_WIFI_STA_PROPERTY);
	indigo_release_property(X_LEDS_PROPERTY);
	indigo_release_property(X_RUNPRESET_L_PROPERTY);
	indigo_release_property(X_RUNPRESET_M_PROPERTY);
	indigo_release_property(X_RUNPRESET_S_PROPERTY);
	indigo_release_property(X_RUNPRESET_1_PROPERTY);
	indigo_release_property(X_RUNPRESET_2_PROPERTY);
	indigo_release_property(X_RUNPRESET_3_PROPERTY);
	indigo_release_property(X_RUNPRESET_PROPERTY);
	indigo_release_property(X_PRESETS_PROPERTY);
	indigo_release_property(X_PRESET_NAMES_PROPERTY);
	indigo_release_property(X_PRESET_GOTO_PROPERTY);
	indigo_release_property(X_HOLD_CURR_PROPERTY);
	indigo_release_property(X_CALIBRATE_F_PROPERTY);
	INDIGO_DEVICE_DETACH_LOG(DRIVER_NAME, device->name);
	return indigo_focuser_detach(device);
}

#pragma mark - High level code (rotator)

static void rotator_connection_handler(indigo_device *device) {
	if (CONNECTION_CONNECTED_ITEM->sw.value) {
		bool connection_result = true;
		const char *error_message = NULL;
		if (PRIVATE_DATA->count == 0) {
			connection_result = primaluce_open(device->master_device, &error_message);
		}
		if (connection_result) {
			PRIVATE_DATA->count++;
		}
		if (connection_result) {
			//+ rotator.on_connect
			char *text;
			// Switching the ARCO off and on again loses its calibration (SESTO SENSO 3 firmware 1.0.9), so the port
			// is switched on only when it is off, and it stays on after a disconnect, as INDI keeps it.
			bool enabled = primalucelib_get_state(LINK) && primalucelib_reply_has(LINK, PRIMALUCE_REPLY_ROTATOR_ENABLED) && primalucelib_reply_number(LINK, PRIMALUCE_REPLY_ROTATOR_ENABLED) != 0;
			if (enabled || primalucelib_rotator_enable(LINK, true)) {
				if (enabled || primalucelib_get_state(LINK)) {
					if ((text = primalucelib_reply_string(LINK, PRIMALUCE_REPLY_ROTATOR_ERROR)) && *text) {
						indigo_send_message(device, ALERT_PROPERTY, "%s", text);
					}
					PRIVATE_DATA->rotator_needs_calibration = primalucelib_reply_number(LINK, PRIMALUCE_REPLY_ROTATOR_CALIBRATION_RESTART) != 0;
					if (PRIVATE_DATA->rotator_needs_calibration) {
						indigo_send_message(device, BUSY_PROPERTY, "ARCO needs calibration");
					}
				}
				PRIVATE_DATA->rotator_has_abs_pos = primalucelib_reply_has(LINK, PRIMALUCE_REPLY_ROTATOR_ABS_POS_REGISTER);
				PRIVATE_DATA->rotator_has_position_deg = primalucelib_reply_has(LINK, PRIMALUCE_REPLY_ROTATOR_POSITION_DEG);
				ROTATOR_POSITION_ITEM->number.value = ROTATOR_POSITION_ITEM->number.target = primalucelib_reply_rotator_angle(LINK, primalucelib_rotator_angle_source(PRIVATE_DATA->rotator_has_position_deg, PRIVATE_DATA->rotator_has_abs_pos));
			} else {
				connection_result = false;
			}
			//- rotator.on_connect
		}
		if (connection_result) {
			indigo_define_property(device, X_CALIBRATE_R_PROPERTY, NULL);
			CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
			indigo_send_message(device, OK_PROPERTY, "Connected to %s on %s", ROTATOR_DEVICE_NAME, DEVICE_PORT_ITEM->text.value);
		} else {
			if (error_message != NULL) {
				indigo_send_message(device, ALERT_PROPERTY, "Failed to connect to %s on %s (%s)", ROTATOR_DEVICE_NAME, DEVICE_PORT_ITEM->text.value, error_message);
			} else {
				indigo_send_message(device, ALERT_PROPERTY, "Failed to connect to %s on %s", ROTATOR_DEVICE_NAME, DEVICE_PORT_ITEM->text.value);
			}
			if (PRIVATE_DATA->count > 0 && --PRIVATE_DATA->count == 0) {
				primaluce_close(device);
			}
			CONNECTION_PROPERTY->state = INDIGO_ALERT_STATE;
			indigo_set_switch(CONNECTION_PROPERTY, CONNECTION_DISCONNECTED_ITEM, true);
		}
	} else {
		indigo_cancel_pending_handlers(device);
		// Cancelled change handlers must not leave properties BUSY: a new session starts in a clean state.
		indigo_property *cancelled_properties[] = {
			X_CALIBRATE_R_PROPERTY,
			ROTATOR_ON_POSITION_SET_PROPERTY,
			ROTATOR_POSITION_PROPERTY,
			ROTATOR_ABORT_MOTION_PROPERTY,
		};
		for (unsigned i = 0; i < sizeof(cancelled_properties) / sizeof(cancelled_properties[0]); i++) {
			if (cancelled_properties[i] != NULL && cancelled_properties[i]->state == INDIGO_BUSY_STATE) {
				cancelled_properties[i]->state = INDIGO_OK_STATE;
			}
		}
		indigo_delete_property(device, X_CALIBRATE_R_PROPERTY, NULL);
		if (--PRIVATE_DATA->count == 0) {
			primaluce_close(device);
		}
		indigo_send_message(device, OK_PROPERTY, "Disconnected from %s", device->name);
		CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
	}
	indigo_rotator_change_property(device, NULL, CONNECTION_PROPERTY);
}

static void rotator_x_calibrate_r_handler(indigo_device *device) {
	X_CALIBRATE_R_PROPERTY->state = INDIGO_OK_STATE;
	//+ rotator.X_CALIBRATE_R.on_change
	if (X_CALIBRATE_R_START_ITEM->sw.value) {
		X_CALIBRATE_R_START_ITEM->sw.value = false;
		if (!primalucelib_rotator_start_calibration(LINK)) {
			INDIGO_UPDATE_PROPERTY_STATE(X_CALIBRATE_R_PROPERTY, INDIGO_ALERT_STATE, NULL);
		} else {
			// The property stays busy until the controller reports the calibration finished.
			X_CALIBRATE_R_PROPERTY->state = INDIGO_BUSY_STATE;
			PRIVATE_DATA->rotator_calibration_polls = 0;
			indigo_execute_handler_in(device, 1, rotator_calibration_poll);
		}
	}
	//- rotator.X_CALIBRATE_R.on_change
	indigo_update_property(device, X_CALIBRATE_R_PROPERTY, NULL);
}

static void rotator_position_handler(indigo_device *device) {
	//+ rotator.ROTATOR_POSITION.on_change
	if (ROTATOR_ON_POSITION_SET_SYNC_ITEM->sw.value) {
		// A sync redefines the current angle without moving the rotator.
		char reason[INDIGO_VALUE_SIZE] = "";
		if (primalucelib_rotator_sync(LINK, ROTATOR_POSITION_ITEM->number.target)) {
			ROTATOR_POSITION_ITEM->number.value = ROTATOR_POSITION_ITEM->number.target;
			ROTATOR_POSITION_PROPERTY->state = INDIGO_OK_STATE;
		} else {
			// The reason of the refusal is copied before the angle the rotator keeps is read back.
			if (*primalucelib_last_error(LINK)) {
				snprintf(reason, sizeof(reason), "Sync refused: %s", primalucelib_last_error(LINK));
			}
			if (rotator_read_angle(device)) {
				ROTATOR_POSITION_ITEM->number.target = ROTATOR_POSITION_ITEM->number.value;
			}
			ROTATOR_POSITION_PROPERTY->state = INDIGO_ALERT_STATE;
		}
		if (*reason) {
			indigo_update_property(device, ROTATOR_POSITION_PROPERTY, "%s", reason);
		} else {
			indigo_update_property(device, ROTATOR_POSITION_PROPERTY, NULL);
		}
	} else {
		char *state = NULL;
		if (!primalucelib_rotator_move_to(LINK, ROTATOR_POSITION_ITEM->number.target, &state) || !primalucelib_is_done(state)) {
			// The reason is the refusal the controller sent, in an ERROR or in the acknowledgement.
			const char *reason = *primalucelib_last_error(LINK) ? primalucelib_last_error(LINK) : state;
			ROTATOR_POSITION_PROPERTY->state = INDIGO_ALERT_STATE;
			if (reason != NULL && *reason) {
				indigo_update_property(device, ROTATOR_POSITION_PROPERTY, "Move refused: %s", reason);
			} else {
				indigo_update_property(device, ROTATOR_POSITION_PROPERTY, NULL);
			}
		} else {
			indigo_execute_handler(device, rotator_movement_finalizer);
		}
	}
	//- rotator.ROTATOR_POSITION.on_change
}

static void rotator_abort_motion_handler(indigo_device *device) {
	ROTATOR_ABORT_MOTION_PROPERTY->state = INDIGO_OK_STATE;
	//+ rotator.ROTATOR_ABORT_MOTION.on_change
	indigo_cancel_pending_handler(device, rotator_position_handler);
	if (X_CALIBRATE_R_PROPERTY->state == INDIGO_BUSY_STATE) {
		indigo_cancel_pending_handler(device, rotator_calibration_poll);
		primalucelib_rotator_stop_calibration(LINK);
		INDIGO_UPDATE_PROPERTY_STATE(X_CALIBRATE_R_PROPERTY, INDIGO_ALERT_STATE, "Calibration aborted");
	}
	if (ROTATOR_POSITION_PROPERTY->state == INDIGO_BUSY_STATE) {
		INDIGO_UPDATE_PROPERTY_STATE(ROTATOR_POSITION_PROPERTY, INDIGO_ALERT_STATE, NULL);
	}
	ROTATOR_ABORT_MOTION_ITEM->sw.value = false;
	if (!primalucelib_rotator_stop(LINK)) {
		ROTATOR_ABORT_MOTION_PROPERTY->state = INDIGO_ALERT_STATE;
	}
	//- rotator.ROTATOR_ABORT_MOTION.on_change
	indigo_update_property(device, ROTATOR_ABORT_MOTION_PROPERTY, NULL);
}

#pragma mark - Device API (rotator)

static indigo_result rotator_enumerate_properties(indigo_device *device, indigo_client *client, indigo_property *property);

static indigo_result rotator_attach(indigo_device *device) {
	if (indigo_rotator_attach(device, DRIVER_NAME, DRIVER_VERSION) == INDIGO_OK) {
		X_CALIBRATE_R_PROPERTY = indigo_init_switch_property(NULL, device->name, X_CALIBRATE_R_PROPERTY_NAME, FOCUSER_ADVANCED_GROUP, "Calibrate rotator", INDIGO_OK_STATE, INDIGO_RW_PERM, INDIGO_ONE_OF_MANY_RULE, 1);
		if (X_CALIBRATE_R_PROPERTY == NULL) {
			return INDIGO_FAILED;
		}
		indigo_init_switch_item(X_CALIBRATE_R_START_ITEM, X_CALIBRATE_R_START_ITEM_NAME, "Start", false);
		ROTATOR_ON_POSITION_SET_PROPERTY->hidden = false;
		ROTATOR_POSITION_PROPERTY->hidden = false;
		ROTATOR_ABORT_MOTION_PROPERTY->hidden = false;
		INDIGO_DEVICE_ATTACH_LOG(DRIVER_NAME, device->name);
		return rotator_enumerate_properties(device, NULL, NULL);
	}
	return INDIGO_FAILED;
}

static indigo_result rotator_enumerate_properties(indigo_device *device, indigo_client *client, indigo_property *property) {
	if (IS_CONNECTED) {
		INDIGO_DEFINE_MATCHING_PROPERTY(X_CALIBRATE_R_PROPERTY);
	}
	return indigo_rotator_enumerate_properties(device, client, property);
}

static indigo_result rotator_change_property(indigo_device *device, indigo_client *client, indigo_property *property) {
	if (indigo_property_match_changeable(CONNECTION_PROPERTY, property)) {
		INDIGO_PROCESS_CONNECT(rotator_connection_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(X_CALIBRATE_R_PROPERTY, property)) {
		INDIGO_COPY_VALUES_PROCESS_CHANGE(X_CALIBRATE_R_PROPERTY, rotator_x_calibrate_r_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(ROTATOR_POSITION_PROPERTY, property)) {
		INDIGO_COPY_TARGETS_PROCESS_CHANGE(ROTATOR_POSITION_PROPERTY, rotator_position_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(ROTATOR_ABORT_MOTION_PROPERTY, property)) {
		INDIGO_COPY_VALUES_PROCESS_URGENT_CHANGE(ROTATOR_ABORT_MOTION_PROPERTY, rotator_abort_motion_handler);
		return INDIGO_OK;
	}
	return indigo_rotator_change_property(device, client, property);
}

static indigo_result rotator_detach(indigo_device *device) {
	if (IS_CONNECTED) {
		indigo_set_switch(CONNECTION_PROPERTY, CONNECTION_DISCONNECTED_ITEM, true);
		rotator_connection_handler(device);
	}
	indigo_release_property(X_CALIBRATE_R_PROPERTY);
	INDIGO_DEVICE_DETACH_LOG(DRIVER_NAME, device->name);
	return indigo_rotator_detach(device);
}

#pragma mark - Device templates

static indigo_device focuser_template = INDIGO_DEVICE_INITIALIZER(FOCUSER_DEVICE_NAME, focuser_attach, focuser_enumerate_properties, focuser_change_property, NULL, focuser_detach);

static indigo_device rotator_template = INDIGO_DEVICE_INITIALIZER(ROTATOR_DEVICE_NAME, rotator_attach, rotator_enumerate_properties, rotator_change_property, NULL, rotator_detach);

#pragma mark - Main code

indigo_result indigo_focuser_primaluce(indigo_driver_action action, indigo_driver_info *info) {
	static indigo_driver_action last_action = INDIGO_DRIVER_SHUTDOWN;
	static primaluce_private_data *private_data = NULL;
	static indigo_device *focuser = NULL;
	static indigo_device *rotator = NULL;

	SET_DRIVER_INFO(info, DRIVER_LABEL, __FUNCTION__, DRIVER_VERSION, false, last_action);

	if (action == last_action) {
		return INDIGO_OK;
	}

	switch (action) {
		case INDIGO_DRIVER_INIT: {
			last_action = action;
			static indigo_device_match_pattern patterns[1] = { 0 };
			strcpy(patterns[0].product_string, "CP2102N");
			INDIGO_REGISER_MATCH_PATTERNS(focuser_template, patterns, 1);
			private_data = (primaluce_private_data *)indigo_safe_malloc(sizeof(primaluce_private_data));
			focuser = (indigo_device *)indigo_safe_malloc_copy(sizeof(indigo_device), &focuser_template);
			focuser->private_data = private_data;
			focuser->master_device = focuser;
			indigo_attach_device(focuser);
			rotator = (indigo_device *)indigo_safe_malloc_copy(sizeof(indigo_device), &rotator_template);
			rotator->private_data = private_data;
			rotator->master_device = focuser;
			indigo_attach_device(rotator);
			break;

		}
		case INDIGO_DRIVER_SHUTDOWN: {
			VERIFY_NOT_CONNECTED(focuser);
			VERIFY_NOT_CONNECTED(rotator);
			last_action = action;
			if (rotator != NULL) {
				indigo_detach_device(rotator);
				indigo_safe_free(rotator);
				rotator = NULL;
			}
			if (focuser != NULL) {
				indigo_detach_device(focuser);
				indigo_safe_free(focuser);
				focuser = NULL;
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
