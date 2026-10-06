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

#define JSMN_STRICT          
#define JSMN_PARENT_LINKS    
#include "jsmn.h"
#include <ctype.h>

//- include

#include <indigo/indigo_driver_xml.h>
#include <indigo/indigo_focuser_driver.h>
#include <indigo/indigo_rotator_driver.h>
#include <indigo/indigo_uni_io.h>

#include "indigo_focuser_primaluce.h"

#pragma mark - Common definitions

#define DRIVER_VERSION       0x03000016
#define DRIVER_NAME          "indigo_focuser_primaluce"
#define DRIVER_LABEL         "PrimaluceLab Focuser/Rotator"
#define FOCUSER_DEVICE_NAME  "PrimaluceLab Focuser"
#define ROTATOR_DEVICE_NAME  "PrimaluceLab Rotator"
#define PRIVATE_DATA         ((primaluce_private_data *)device->private_data)

//+ define

#define MAX_RESPONSE_SIZE    8192
#define MAX_TOKEN_COUT       1024
// The controller stores named focuser positions as PRESET_1 to PRESET_9 (M1POS).
#define PRESET_COUNT         9
// Idle position poll period; the controller state (temperature, supplies) is read every ENVIRONMENT_TICKS polls.
#define POLL_PERIOD          2
#define ENVIRONMENT_TICKS    5
// A motor that reports running without advancing for this many 0.2 s polls has stalled.
#define STALL_POLLS          25
// Consecutive failed position reads tolerated during a move.
#define MAX_POLL_FAILURES    3
// EXT_T reported by a controller without an external probe.
#define NO_PROBE_TEMPERATURE -127

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
	char response[MAX_RESPONSE_SIZE];
	jsmntok_t tokens[MAX_TOKEN_COUT];
	jsmn_parser parser;
	bool has_abs_pos;
	bool rotator_has_abs_pos;
	bool rotator_has_position_deg;
	int preset_positions[PRESET_COUNT];
	char preset_names[PRESET_COUNT][INDIGO_VALUE_SIZE];
	int rotator_calibration_polls;
	bool is_sestosenso_3;
	bool is_http;
	char http_host[INDIGO_NAME_SIZE];
	int http_port;
	char last_error[INDIGO_VALUE_SIZE];
	double position, last_position;
	bool link_failed;
	int stalled_polls, poll_failures, environment_ticks;
	bool external_motion, poll_alert, abort_requested, calibrating, pending;
	int backlash, speed;
	indigo_item *leds_item, *hold_item, *wifi_item;
	//- data
} primaluce_private_data;

#pragma mark - Low level code

//+ code

static void focuser_position_handler(indigo_device *device);
static void focuser_steps_handler(indigo_device *device);
static void rotator_position_handler(indigo_device *device);

static char *GET_MODNAME[] = { "res", "get", "MODNAME", NULL };
static char *GET_ERROR[] = { "res", "get", "ERROR", NULL };
static char *SET_ERROR[] = { "res", "set", "ERROR", NULL };
static char *CMD_ERROR[] = { "res", "cmd", "ERROR", NULL };
static char *GET_SN[] = { "res", "get", "SN", NULL };
static char *GET_SWAPP[] = { "res", "get", "SWVERS", "SWAPP", NULL };
static char *GET_SWWEB[] = { "res", "get", "SWVERS", "SWWEB", NULL };
static char *GET_CALRESTART_MOT1[] = { "res", "get", "CALRESTART", "MOT1", NULL };
static char *GET_CALRESTART_MOT2[] = { "res", "get", "CALRESTART", "MOT2", NULL };
static char *GET_MOT1_ABS_POS_STEP[] = { "res", "get", "MOT1", "ABS_POS_STEP", NULL };
static char *GET_MOT1_ABS_POS[] = { "res", "get", "MOT1", "ABS_POS", NULL };
static char *GET_MOT2_ABS_POS_DEG[] = { "res", "get", "MOT2", "ABS_POS_DEG", NULL };
static char *GET_MOT2_ABS_POS[] = { "res", "get", "MOT2", "ABS_POS", NULL };
static char *GET_MOT2_POSITION_DEG[] = { "res", "get", "MOT2", "POSITION_DEG", NULL };
static char *GET_MOT2_CAL_STATUS[] = { "res", "get", "MOT2", "CAL_STATUS", NULL };
static char *SET_MOT2_CAL_STATUS[] = { "res", "set", "MOT2", "CAL_STATUS", NULL };
static char *GET_MOT1_BKLASH[] = { "res", "get", "MOT1", "BKLASH", NULL };
static char *SET_MOT1_BKLASH[] = { "res", "set", "MOT1", "BKLASH", NULL };
static char *GET_MOT1_SPEED[] = { "res", "get", "MOT1", "SPEED", NULL };
static char *SET_MOT1_SPEED[] = { "res", "set", "MOT1", "SPEED", NULL };
static char *GET_MOT1_MST[] = { "res", "get", "MOT1", "STATUS", "MST", NULL };
static char *GET_MOT2_MST[] = { "res", "get", "MOT2", "STATUS", "MST", NULL };
static char *CMD_MOT1_STEP[] = { "res", "cmd", "MOT1", "STEP", NULL };
static char *CMD_MOT1_GOTO[] = { "res", "cmd", "MOT1", "GOTO", NULL };
//static char *CMD_MOT1_MOVE_REL[] = { "res", "cmd", "MOT1", "MOVE_REL", NULL };
static char *CMD_MOT1_MOT_STOP[] = { "res", "cmd", "MOT1", "MOT_STOP", NULL };
static char *CMD_MOT1_MOT_ABORT[] = { "res", "cmd", "MOT1", "MOT_ABORT", NULL };
static char *GET_PRESET_1[] = { "res", "get", "PRESET_1", NULL };
static char *CMD_MOT2_STEP[] = { "res", "cmd", "MOT2", "STEP", NULL };
static char *CMD_MOT2_MOT_STOP[] = { "res", "cmd", "MOT2", "MOT_STOP", NULL };
static char *CMD_MOT2_SYNC_POS[] = { "res", "cmd", "MOT2", "SYNC_POS", NULL };
static char *GET_MOT1_CAL_MINPOS[] = { "res", "get", "MOT1", "CAL_MINPOS", NULL };
static char *GET_MOT1_CAL_MAXPOS[] = { "res", "get", "MOT1", "CAL_MAXPOS", NULL };
static char *GET_EXT_T[] = { "res", "get", "EXT_T", NULL };
static char *GET_DIMLEDS[] = { "res", "get", "DIMLEDS", NULL };
static char *GET_VIN_12V[] = { "res", "get", "VIN_12V", NULL };
static char *GET_VIN_USB[] = { "res", "get", "VIN_USB", NULL };
static char *GET_MOT1_NTC_T[] = { "res", "get", "MOT1", "NTC_T", NULL };
static char *GET_MOT1_ERROR[] = { "res", "get", "MOT1", "ERROR", NULL };
static char *GET_MOT2_ERROR[] = { "res", "get", "MOT2", "ERROR", NULL };
static char *GET_WIFIAP_STATUS[] = { "res", "get", "WIFIAP", "STATUS", NULL };
static char *GET_LANCFG[] = { "res", "get", "LANCFG", NULL };
static char *GET_WIFIAP_SSID[] = { "res", "get", "WIFIAP", "SSID", NULL };
static char *GET_WIFIAP_PWD[] = { "res", "get", "WIFIAP", "PWD", NULL };
static char *GET_WIFISTA_SSID[] = { "res", "get", "WIFISTA", "SSID", NULL };
static char *GET_WIFISTA_PWD[] = { "res", "get", "WIFISTA", "PWD", NULL };
static char *GET_MOT1_FnRUN_ACC[] = { "res", "get", "MOT1", "FnRUN_ACC", NULL };
static char *GET_MOT1_FnRUN_SPD[] = { "res", "get", "MOT1", "FnRUN_SPD", NULL };
static char *GET_MOT1_FnRUN_DEC[] = { "res", "get", "MOT1", "FnRUN_DEC", NULL };
static char *GET_MOT1_FnRUN_CURR_ACC[] = { "res", "get", "MOT1", "FnRUN_CURR_ACC", NULL };
static char *GET_MOT1_FnRUN_CURR_SPD[] = { "res", "get", "MOT1", "FnRUN_CURR_SPD", NULL };
static char *GET_MOT1_FnRUN_CURR_DEC[] = { "res", "get", "MOT1", "FnRUN_CURR_DEC", NULL };
static char *GET_MOT1_FnRUN_CURR_HOLD[] = { "res", "get", "MOT1", "FnRUN_CURR_HOLD", NULL };
static char *GET_MOT1_HOLDCURR_STATUS[] = { "res", "get", "MOT1", "HOLDCURR_STATUS", NULL };
static char *SET_MOT1_HOLDCURR_STATUS[] = { "res", "set", "MOT1", "HOLDCURR_STATUS", NULL };
static char *GET_RUNPRESET_L_M1ACC[] = { "res", "get", "RUNPRESET_L", "M1ACC", NULL };
static char *GET_RUNPRESET_L_M1SPD[] = { "res", "get", "RUNPRESET_L", "M1SPD", NULL };
static char *GET_RUNPRESET_L_M1DEC[] = { "res", "get", "RUNPRESET_L", "M1DEC", NULL };
static char *GET_RUNPRESET_L_M1CACC[] = { "res", "get", "RUNPRESET_L", "M1CACC", NULL };
static char *GET_RUNPRESET_L_M1CSPD[] = { "res", "get", "RUNPRESET_L", "M1CSPD", NULL };
static char *GET_RUNPRESET_L_M1CDEC[] = { "res", "get", "RUNPRESET_L", "M1CDEC", NULL };
static char *GET_RUNPRESET_L_M1HOLD[] = { "res", "get", "RUNPRESET_L", "M1HOLD", NULL };
static char *GET_RUNPRESET_M_M1ACC[] = { "res", "get", "RUNPRESET_M", "M1ACC", NULL };
static char *GET_RUNPRESET_M_M1SPD[] = { "res", "get", "RUNPRESET_M", "M1SPD", NULL };
static char *GET_RUNPRESET_M_M1DEC[] = { "res", "get", "RUNPRESET_M", "M1DEC", NULL };
static char *GET_RUNPRESET_M_M1CACC[] = { "res", "get", "RUNPRESET_M", "M1CACC", NULL };
static char *GET_RUNPRESET_M_M1CSPD[] = { "res", "get", "RUNPRESET_M", "M1CSPD", NULL };
static char *GET_RUNPRESET_M_M1CDEC[] = { "res", "get", "RUNPRESET_M", "M1CDEC", NULL };
static char *GET_RUNPRESET_M_M1HOLD[] = { "res", "get", "RUNPRESET_M", "M1HOLD", NULL };
static char *GET_RUNPRESET_S_M1ACC[] = { "res", "get", "RUNPRESET_S", "M1ACC", NULL };
static char *GET_RUNPRESET_S_M1SPD[] = { "res", "get", "RUNPRESET_S", "M1SPD", NULL };
static char *GET_RUNPRESET_S_M1DEC[] = { "res", "get", "RUNPRESET_S", "M1DEC", NULL };
static char *GET_RUNPRESET_S_M1CACC[] = { "res", "get", "RUNPRESET_S", "M1CACC", NULL };
static char *GET_RUNPRESET_S_M1CSPD[] = { "res", "get", "RUNPRESET_S", "M1CSPD", NULL };
static char *GET_RUNPRESET_S_M1CDEC[] = { "res", "get", "RUNPRESET_S", "M1CDEC", NULL };
static char *GET_RUNPRESET_S_M1HOLD[] = { "res", "get", "RUNPRESET_S", "M1HOLD", NULL };
static char *GET_RUNPRESET_1_M1ACC[] = { "res", "get", "RUNPRESET_1", "M1ACC", NULL };
static char *GET_RUNPRESET_1_M1SPD[] = { "res", "get", "RUNPRESET_1", "M1SPD", NULL };
static char *GET_RUNPRESET_1_M1DEC[] = { "res", "get", "RUNPRESET_1", "M1DEC", NULL };
static char *GET_RUNPRESET_1_M1CACC[] = { "res", "get", "RUNPRESET_1", "M1CACC", NULL };
static char *GET_RUNPRESET_1_M1CSPD[] = { "res", "get", "RUNPRESET_1", "M1CSPD", NULL };
static char *GET_RUNPRESET_1_M1CDEC[] = { "res", "get", "RUNPRESET_1", "M1CDEC", NULL };
static char *GET_RUNPRESET_1_M1HOLD[] = { "res", "get", "RUNPRESET_1", "M1HOLD", NULL };
static char *GET_RUNPRESET_2_M1ACC[] = { "res", "get", "RUNPRESET_2", "M1ACC", NULL };
static char *GET_RUNPRESET_2_M1SPD[] = { "res", "get", "RUNPRESET_2", "M1SPD", NULL };
static char *GET_RUNPRESET_2_M1DEC[] = { "res", "get", "RUNPRESET_2", "M1DEC", NULL };
static char *GET_RUNPRESET_2_M1CACC[] = { "res", "get", "RUNPRESET_2", "M1CACC", NULL };
static char *GET_RUNPRESET_2_M1CSPD[] = { "res", "get", "RUNPRESET_2", "M1CSPD", NULL };
static char *GET_RUNPRESET_2_M1CDEC[] = { "res", "get", "RUNPRESET_2", "M1CDEC", NULL };
static char *GET_RUNPRESET_2_M1HOLD[] = { "res", "get", "RUNPRESET_2", "M1HOLD", NULL };
static char *GET_RUNPRESET_3_M1ACC[] = { "res", "get", "RUNPRESET_3", "M1ACC", NULL };
static char *GET_RUNPRESET_3_M1SPD[] = { "res", "get", "RUNPRESET_3", "M1SPD", NULL };
static char *GET_RUNPRESET_3_M1DEC[] = { "res", "get", "RUNPRESET_3", "M1DEC", NULL };
static char *GET_RUNPRESET_3_M1CACC[] = { "res", "get", "RUNPRESET_3", "M1CACC", NULL };
static char *GET_RUNPRESET_3_M1CSPD[] = { "res", "get", "RUNPRESET_3", "M1CSPD", NULL };
static char *GET_RUNPRESET_3_M1CDEC[] = { "res", "get", "RUNPRESET_3", "M1CDEC", NULL };
static char *GET_RUNPRESET_3_M1HOLD[] = { "res", "get", "RUNPRESET_3", "M1HOLD", NULL };

// Over the network the controller serves the same JSON requests as GET /ajax.php?jreq=<request>, one request per connection.
static bool primaluce_http_exchange(indigo_device *device, const char *request) {
	char query[3 * MAX_RESPONSE_SIZE + 1];
	char *out = query;
	for (const unsigned char *in = (const unsigned char *)request; *in && out < query + sizeof(query) - 4; in++) {
		if (isalnum(*in) || strchr("-_.~", *in)) {
			*out++ = *in;
		} else {
			out += sprintf(out, "%%%02X", *in);
		}
	}
	*out = 0;
	indigo_uni_handle *handle = indigo_uni_open_client_socket_with_timeout(PRIVATE_DATA->http_host, PRIVATE_DATA->http_port, SOCK_STREAM, INDIGO_DELAY(5), INDIGO_LOG_DEBUG);
	if (handle == NULL) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "Can't connect to %s:%d", PRIVATE_DATA->http_host, PRIVATE_DATA->http_port);
		return false;
	}
	bool result = false;
	char reply[MAX_RESPONSE_SIZE + 1024];
	long length = 0;
	if (indigo_uni_printf(handle, "GET /ajax.php?jreq=%s HTTP/1.1\r\nHost: %s\r\nConnection: close\r\n\r\n", query, PRIVATE_DATA->http_host) > 0) {
		char *body = NULL;
		long content_length = -1;
		// The controller keeps the connection open after the reply, so Content-Length ends the read.
		while (length < (long)sizeof(reply) - 1 && indigo_uni_wait_for_data(handle, INDIGO_DELAY(5)) > 0) {
			long count = indigo_uni_read_available(handle, reply + length, sizeof(reply) - 1 - length);
			if (count <= 0) {
				break;
			}
			length += count;
			reply[length] = 0;
			if (body == NULL && (body = strstr(reply, "\r\n\r\n")) != NULL) {
				char *field = strstr(reply, "\r\nContent-Length:");
				if (field != NULL && field < body) {
					content_length = atol(field + 17);
				}
			}
			if (body != NULL && content_length >= 0 && length >= body + 4 - reply + content_length) {
				break;
			}
		}
		reply[length] = 0;
		if (strncmp(reply, "HTTP/1.", 7) || strncmp(reply + 8, " 200 ", 5) || body == NULL) {
			INDIGO_DRIVER_ERROR(DRIVER_NAME, "Unexpected HTTP response: %.40s", reply);
		} else if (strlen(body + 4) >= MAX_RESPONSE_SIZE) {
			INDIGO_DRIVER_ERROR(DRIVER_NAME, "HTTP response too long");
		} else {
			strcpy(PRIVATE_DATA->response, body + 4);
			INDIGO_DRIVER_DEBUG(DRIVER_NAME, "%s", PRIVATE_DATA->response);
			result = *PRIVATE_DATA->response != 0;
		}
	}
	indigo_uni_close(&handle);
	return result;
}

static char *get_string(indigo_device *device, char *path[]);

static bool primaluce_command(indigo_device *device, char *command, ...) {
	long result;
	PRIVATE_DATA->last_error[0] = 0;
	va_list args;
	va_start(args, command);
	if (PRIVATE_DATA->is_http) {
		char request[MAX_RESPONSE_SIZE];
		vsnprintf(request, sizeof(request), command, args);
		va_end(args);
		if (!primaluce_http_exchange(device, request)) {
			PRIVATE_DATA->link_failed = true;
			return false;
		}
	} else {
		result = indigo_uni_vprintf(PRIVATE_DATA->handle, command, args);
		va_end(args);
		if (result <= 0) {
			PRIVATE_DATA->link_failed = true;
			return false;
		}
		while (true) {
			result = indigo_uni_read_section(PRIVATE_DATA->handle, PRIVATE_DATA->response, MAX_RESPONSE_SIZE - 1, "\n", "\r\n", -1);
			if (result < 1) {
				PRIVATE_DATA->link_failed = true;
				return false;
			}
			if (*PRIVATE_DATA->response == '[') {
				INDIGO_DRIVER_DEBUG(DRIVER_NAME, "Ignored");
				continue;
			}
			break;
		}
	}
	// The controller answered, whatever the answer is.
	PRIVATE_DATA->link_failed = false;
	memset(PRIVATE_DATA->tokens, 0, sizeof(PRIVATE_DATA->tokens));
	jsmn_init(&PRIVATE_DATA->parser);
	if (*PRIVATE_DATA->response == '"' || jsmn_parse(&PRIVATE_DATA->parser, PRIVATE_DATA->response, MAX_RESPONSE_SIZE, PRIVATE_DATA->tokens, MAX_TOKEN_COUT) <= 0) {
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "Failed to parse");
		return false;
	}
	for (int i = 0; i < MAX_TOKEN_COUT; i++) {
		if (PRIVATE_DATA->tokens[i].type == JSMN_UNDEFINED) {
			break;
		}
		if (PRIVATE_DATA->tokens[i].type == JSMN_STRING) {
			PRIVATE_DATA->response[PRIVATE_DATA->tokens[i].end] = 0;
		}
	}
	// A rejected request is answered as an ERROR under the request verb
	char *error = get_string(device, CMD_ERROR);
	if (error == NULL) {
		error = get_string(device, SET_ERROR);
	}
	if (error == NULL) {
		error = get_string(device, GET_ERROR);
	}
	if (error != NULL) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "Request rejected: %s", error);
		INDIGO_COPY_VALUE(PRIVATE_DATA->last_error, error);
		return false;
	}
	return true;
}

static int getToken(indigo_device *device, int start, char *path[]) {
	if (PRIVATE_DATA->tokens[start].type != JSMN_OBJECT) {
		return -1;
	}
	int count = PRIVATE_DATA->tokens[start].size;
	int index = start + 1;
	char *name = *path;
	for (int i = 0; i < count; i++) {
		if (PRIVATE_DATA->tokens[index].type != JSMN_STRING) {
			return -1;
		}
		char *n = PRIVATE_DATA->response + PRIVATE_DATA->tokens[index].start;
		int l = PRIVATE_DATA->tokens[index].end - PRIVATE_DATA->tokens[index].start;
		if (!strncmp(n, name, l)) {
			index++;
			if (*++path == NULL) {
				return index;
			}
			return getToken(device, index, path);
		} else {
			while (true) {
				index++;
				if (PRIVATE_DATA->tokens[index].type == JSMN_UNDEFINED) {
					return -1;
				}
				if (PRIVATE_DATA->tokens[index].parent == start) {
					break;
				}
			}
		}
	}
	return -1;
}

static char *get_string(indigo_device *device, char *path[]) {
	int index = getToken(device, 0, path);
	if (index == -1 || PRIVATE_DATA->tokens[index].type != JSMN_STRING) {
		return NULL;
	}
	return PRIVATE_DATA->response + PRIVATE_DATA->tokens[index].start;
}

static double get_number2(indigo_device *device, char *path[], char *alt_path[]) {
	int index = getToken(device, 0, path);
	if (index == -1) {
		if (alt_path != NULL) {
			index = getToken(device, 0, alt_path);
			if (index == -1) {
				return 0;
			}
		} else {
			return 0;
		}
	}
	if  (PRIVATE_DATA->tokens[index].type == JSMN_PRIMITIVE || PRIVATE_DATA->tokens[index].type == JSMN_STRING) {
		return atof(PRIVATE_DATA->response + PRIVATE_DATA->tokens[index].start);
	}
	return 0;
}

static double get_number(indigo_device *device, char *path[]) {
	return get_number2(device, path, NULL);
}

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
	if (!primaluce_command(device, "{\"req\":{\"get\":{\"LANCFG\":\"\"}}}") || (text = get_string(device, GET_LANCFG)) == NULL) {
		return false;
	}
	if (!strcmp(text, mode)) {
		return true;
	}
	if (!primaluce_command(device, "{\"req\":{\"set\":{\"LANCFG\":\"%s\"}}}", mode) || !primaluce_command(device, "{\"req\":{\"cmd\":{\"REBOOT\":\"\"}}}")) {
		return false;
	}
	if (PRIVATE_DATA->is_http) {
		indigo_send_message(device, X_WIFI_PROPERTY, "The controller restarts to apply the WiFi mode and leaves its current network");
		return true;
	}
	// The restart prints a boot log on the serial line before the controller answers again.
	for (int i = 0; i < 20; i++) {
		indigo_usleep(INDIGO_DELAY(0.5));
		indigo_uni_discard(PRIVATE_DATA->handle);
		if (primaluce_command(device, "{\"req\":{\"get\":{\"MODNAME\":\"\"}}}") && get_string(device, GET_MODNAME) != NULL) {
			return true;
		}
	}
	INDIGO_DRIVER_ERROR(DRIVER_NAME, "No answer after restart");
	return false;
}

static bool primaluce_open(indigo_device *device) {
	char *name = DEVICE_PORT_ITEM->text.value;
	PRIVATE_DATA->is_http = indigo_uni_is_url(name, "http");
	if (PRIVATE_DATA->is_http) {
		// http://host[:port] or tcp://host[:port] reaches the controller on a WiFi network
		char *host = strchr(name, ':') + 3;
		size_t host_length = strcspn(host, ":/");
		if (host_length == 0 || host_length >= sizeof(PRIVATE_DATA->http_host)) {
			INDIGO_DRIVER_ERROR(DRIVER_NAME, "Malformed URL '%s'", name);
			PRIVATE_DATA->is_http = false;
			return false;
		}
		memcpy(PRIVATE_DATA->http_host, host, host_length);
		PRIVATE_DATA->http_host[host_length] = 0;
		PRIVATE_DATA->http_port = host[host_length] == ':' ? atoi(host + host_length + 1) : 80;
	} else {
		PRIVATE_DATA->handle = indigo_uni_open_serial_with_speed(name, 115200, INDIGO_LOG_DEBUG);
	}
	if (PRIVATE_DATA->is_http || PRIVATE_DATA->handle != NULL) {
		char *text;
		if (primaluce_command(device, "{\"req\":{\"get\":{\"MODNAME\":\"\"}}}") && (text = get_string(device, GET_MODNAME))) {
			if (!strncmp(text, "SESTOSENSO", 10) || !strncmp(text, "ESATTO", 6)) {
				PRIVATE_DATA->is_sestosenso_3 = strncmp(text, "SESTOSENSO3", 11)==0;
				if (primaluce_command(device, "{\"req\":{\"get\":{\"SWVERS\":{\"SWAPP\":\"\"}}}}") && (text = get_string(device, GET_SWAPP))) {
					double version = atof(text);
					if (!PRIVATE_DATA->is_sestosenso_3 && version < 3.05) {
						indigo_send_message(device, BUSY_PROPERTY, "%s has firmware version %.2f and at least 3.05 is needed", INFO_DEVICE_MODEL_ITEM->text.value, version);
					}
					// Diagnostic output would interleave with the replies on the serial line.
					primaluce_command(device, "{\"req\":{\"cmd\":{\"LOGLEVEL\":\"no output\"}}}");
					return true;
				} else {
					INDIGO_DRIVER_ERROR(DRIVER_NAME, "Unsupported version");
				}
			} else {
				INDIGO_DRIVER_ERROR(DRIVER_NAME, "Unsupported device");
			}
		}
		if (PRIVATE_DATA->handle != NULL) {
			indigo_uni_close(&PRIVATE_DATA->handle);
		}
		PRIVATE_DATA->is_http = false;
	}
	return false;
}

static void primaluce_close(indigo_device *device) {
	if (PRIVATE_DATA->handle != NULL || PRIVATE_DATA->is_http) {
		if (PRIVATE_DATA->handle != NULL) {
			indigo_uni_close(&PRIVATE_DATA->handle);
		}
		PRIVATE_DATA->is_http = false;
		INDIGO_COPY_VALUE(INFO_DEVICE_MODEL_ITEM->text.value, "N/A");
		INDIGO_COPY_VALUE(INFO_DEVICE_SERIAL_NUM_ITEM->text.value, "N/A");
		INDIGO_COPY_VALUE(INFO_DEVICE_FW_REVISION_ITEM->text.value, "N/A");
		indigo_update_property(device, INFO_PROPERTY, NULL);
		INDIGO_DRIVER_LOG(DRIVER_NAME, "Disconnected from %s", DEVICE_PORT_ITEM->text.value);
	}
}

//- code

//+ focuser.code

// The controller refuses to drive the motor without its 12 V supply and says so in the command reply.
static const char *power_message(const char *state) {
	return state != NULL && !strcmp(state, "12V_PowerSupply_Error") ? "The motor has no 12 V power supply" : NULL;
}

static void focuser_motion_failed(indigo_device *device, const char *state) {
	// Both motion properties end the move ALERT at the position the focuser has, and the
	// reason the controller gave reaches the client.
	const char *message = power_message(state);
	char reason[INDIGO_VALUE_SIZE];
	if (message == NULL && *PRIVATE_DATA->last_error) {
		snprintf(reason, sizeof(reason), "Move refused: %s", PRIVATE_DATA->last_error);
		message = reason;
	}
	PRIVATE_DATA->abort_requested = false;
	FOCUSER_POSITION_ITEM->number.target = FOCUSER_POSITION_ITEM->number.value;
	FOCUSER_STEPS_PROPERTY->state = INDIGO_ALERT_STATE;
	indigo_update_property(device, FOCUSER_STEPS_PROPERTY, NULL);
	FOCUSER_POSITION_PROPERTY->state = INDIGO_ALERT_STATE;
	if (message != NULL) {
		indigo_update_property(device, FOCUSER_POSITION_PROPERTY, "%s", message);
	} else {
		indigo_update_property(device, FOCUSER_POSITION_PROPERTY, NULL);
	}
}

// Reads the focuser position and whether the motor runs. A reply without the position
// is a failed read; a reply without MST cannot prove the motor is still running, so it
// counts as stopped.
static bool focuser_read_motion(indigo_device *device, double *position, bool *moving) {
	char *get_position = PRIVATE_DATA->has_abs_pos ? "{\"req\":{\"get\":{\"MOT1\":{\"ABS_POS\":\"STEP\",\"STATUS\":\"\"}}}}" : "{\"req\":{\"get\":{\"MOT1\":{\"ABS_POS_STEP\":\"\",\"STATUS\":\"\"}}}}";
	char **path = PRIVATE_DATA->has_abs_pos ? GET_MOT1_ABS_POS : GET_MOT1_ABS_POS_STEP;
	if (!primaluce_command(device, get_position) || getToken(device, 0, path) == -1) {
		return false;
	}
	*position = PRIVATE_DATA->position = get_number(device, path);
	char *state = NULL;
	if (!PRIVATE_DATA->is_sestosenso_3 || primaluce_command(device, "{\"req\":{\"get\":{\"MOT1\":{\"STATUS\":{\"MST\":\"\"}}}}}")) {
		state = get_string(device, GET_MOT1_MST);
	}
	*moving = state != NULL && strcmp(state, "stop");
	return true;
}

// MOT_ABORT stops the motor at once, MOT_STOP decelerates first and is the fallback.
static bool focuser_stop(indigo_device *device, char **reason) {
	char *state = NULL;
	*reason = NULL;
	if (primaluce_command(device, "{\"req\":{\"cmd\":{\"MOT1\":{\"MOT_ABORT\":\"\"}}}}") && (state = get_string(device, CMD_MOT1_MOT_ABORT)) != NULL && !strcmp(state, "done")) {
		return true;
	}
	if (!primaluce_command(device, "{\"req\":{\"cmd\":{\"MOT1\":{\"MOT_STOP\":\"\"}}}}")) {
		return false;
	}
	state = get_string(device, CMD_MOT1_MOT_STOP);
	if (state == NULL || strcmp(state, "done")) {
		*reason = state;
		return false;
	}
	return true;
}

static void focuser_movement_ended(indigo_device *device, indigo_property_state state) {
	// An interrupted or failed move ends at the position the focuser reached, never at the
	// requested one.
	if (state != INDIGO_OK_STATE) {
		FOCUSER_POSITION_ITEM->number.target = FOCUSER_POSITION_ITEM->number.value;
	}
	PRIVATE_DATA->abort_requested = false;
	FOCUSER_POSITION_PROPERTY->state = FOCUSER_STEPS_PROPERTY->state = state;
}

static void focuser_movement_finalizer(indigo_device *device) {
	double position = 0;
	bool moving = false;
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
				focuser_stop(device, &reason);
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
		focuser_stop(device, &reason);
		focuser_movement_ended(device, INDIGO_ALERT_STATE);
	}
	indigo_update_property(device, FOCUSER_POSITION_PROPERTY, NULL);
	indigo_update_property(device, FOCUSER_STEPS_PROPERTY, NULL);
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
		FOCUSER_POSITION_ITEM->number.value = FOCUSER_POSITION_ITEM->number.target = position;
		if (PRIVATE_DATA->calibrating) {
			// The calibration run is the driver's own motion: it is followed, but not taken
			// for an uncommanded move, which would leave the position BUSY and refuse END.
		} else if (moving) {
			PRIVATE_DATA->external_motion = true;
			PRIVATE_DATA->poll_alert = false;
			FOCUSER_POSITION_PROPERTY->state = FOCUSER_STEPS_PROPERTY->state = INDIGO_BUSY_STATE;
		} else if (PRIVATE_DATA->external_motion) {
			PRIVATE_DATA->external_motion = false;
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
	bool result;
	if (PRIVATE_DATA->rotator_has_position_deg) {
		result = primaluce_command(device, "{\"req\":{\"get\":{\"MOT2\":{\"POSITION_DEG\":\"\",\"STATUS\":\"\"}}}}");
	} else {
		result = primaluce_command(device, PRIVATE_DATA->rotator_has_abs_pos ? "{\"req\":{\"get\":{\"MOT2\":{\"ABS_POS\":\"DEG\",\"STATUS\":\"\"}}}}" : "{\"req\":{\"get\":{\"MOT2\":{\"ABS_POS_DEG\":\"\",\"STATUS\":\"\"}}}}");
	}
	if (result) {
		ROTATOR_POSITION_ITEM->number.value = get_number(device, PRIVATE_DATA->rotator_has_position_deg ? GET_MOT2_POSITION_DEG : (PRIVATE_DATA->rotator_has_abs_pos ? GET_MOT2_ABS_POS : GET_MOT2_ABS_POS_DEG));
	}
	return result;
}

// The controller calibrates the ARCO on its own and reports CAL_STATUS "stop" when it is done.
static void rotator_calibration_poll(indigo_device *device) {
	char *state;
	// An abort ends the calibration while a poll may already be queued.
	if (X_CALIBRATE_R_PROPERTY->state != INDIGO_BUSY_STATE) {
		return;
	}
	if (!primaluce_command(device, "{\"req\":{\"get\":{\"MOT2\":{\"CAL_STATUS\":\"\"}}}}") || (state = get_string(device, GET_MOT2_CAL_STATUS)) == NULL) {
		INDIGO_UPDATE_PROPERTY_STATE(X_CALIBRATE_R_PROPERTY, INDIGO_ALERT_STATE, NULL);
	} else if (strcmp(state, "stop")) {
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
		INDIGO_UPDATE_PROPERTY_STATE(X_CALIBRATE_R_PROPERTY, INDIGO_OK_STATE, NULL);
	}
}

static void rotator_movement_finalizer(indigo_device *device) {
	if (rotator_read_angle(device)) {
		char *state = get_string(device, GET_MOT2_MST);
		if (state != NULL && strcmp(state, "stop")) {
			indigo_execute_handler_in(device, 0.2, rotator_movement_finalizer);
		} else {
			for (int i = 0; i < 10; i++) {
				indigo_usleep(100000);
				rotator_read_angle(device);
				if (ROTATOR_POSITION_ITEM->number.target == ROTATOR_POSITION_ITEM->number.value) {
					ROTATOR_POSITION_PROPERTY->state = INDIGO_OK_STATE;
					break;
				}
			}
			if (ROTATOR_POSITION_PROPERTY->state == INDIGO_BUSY_STATE) {
				ROTATOR_POSITION_PROPERTY->state = INDIGO_ALERT_STATE;
			}
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
	if (PRIVATE_DATA->environment_ticks++ % ENVIRONMENT_TICKS == 0) {
		if (primaluce_command(device, "{\"req\":{\"get\":{\"EXT_T\":\"\", \"VIN_12V\": \"\", \"MOT1\":{\"NTC_T\":\"\"}}}}") && getToken(device, 0, GET_EXT_T) != -1) {
			double temp = get_number(device, GET_EXT_T);
			// A controller without an external probe reports -127, which is no reading.
			if (temp <= NO_PROBE_TEMPERATURE) {
				FOCUSER_TEMPERATURE_PROPERTY->state = INDIGO_IDLE_STATE;
			} else {
				FOCUSER_TEMPERATURE_ITEM->number.value = temp;
				FOCUSER_TEMPERATURE_PROPERTY->state = INDIGO_OK_STATE;
			}
			indigo_update_property(device, FOCUSER_TEMPERATURE_PROPERTY, NULL);
			double motor_temp = get_number(device, GET_MOT1_NTC_T);
			double vin12v = get_number(device, GET_VIN_12V);
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
		if (PRIVATE_DATA->count == 0) {
			connection_result = primaluce_open(device);
		}
		if (connection_result) {
			PRIVATE_DATA->count++;
		}
		if (connection_result) {
			//+ focuser.on_connect
			char *text;
			if (primaluce_command(device, "{\"req\":{\"get\": \"\"}}")) {
				if ((text = get_string(device, GET_MODNAME))) {
					INDIGO_DRIVER_DEBUG(DRIVER_NAME, "Model: %s", text);
					INDIGO_COPY_VALUE(INFO_DEVICE_MODEL_ITEM->text.value, text);
					if (!strncmp(text, "SESTOSENSO", 10)) {
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
					} else if (!strncmp(text, "ESATTO", 6)) {
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
				if ((text = get_string(device, GET_SWAPP))) {
					INDIGO_DRIVER_DEBUG(DRIVER_NAME, "SWAPP: %s", text);
					INDIGO_COPY_VALUE(INFO_DEVICE_FW_REVISION_ITEM->text.value, text);
					if ((text = get_string(device, GET_SWWEB))) {
						INDIGO_DRIVER_DEBUG(DRIVER_NAME, "SWWEB: %s", text);
						strcat(INFO_DEVICE_FW_REVISION_ITEM->text.value, " / ");
						strcat(INFO_DEVICE_FW_REVISION_ITEM->text.value, text);
					}
				}
				if ((text = get_string(device, GET_SN))) {
					INDIGO_DRIVER_DEBUG(DRIVER_NAME, "SN: %s", text);
					INDIGO_COPY_VALUE(INFO_DEVICE_SERIAL_NUM_ITEM->text.value, text);
				}
				indigo_update_property(device, INFO_PROPERTY, NULL);
				if ((text = get_string(device, GET_MOT1_ERROR)) && *text) {
					indigo_send_message(device, ALERT_PROPERTY, "%s", text);
				}
				if (get_number(device, GET_CALRESTART_MOT1)) {
					indigo_send_message(device, BUSY_PROPERTY, "%s needs calibration", INFO_DEVICE_MODEL_ITEM->text.value);
				}
				PRIVATE_DATA->has_abs_pos = getToken(device, 0, GET_MOT1_ABS_POS) != -1;
				// The calibrated travel limits the positions a client can request; an uncalibrated controller reports no usable range.
				double min_position = get_number(device, GET_MOT1_CAL_MINPOS);
				double max_position = get_number(device, GET_MOT1_CAL_MAXPOS);
				if (max_position <= min_position) {
					min_position = 0;
					max_position = 1000000;
				}
				FOCUSER_POSITION_ITEM->number.min = min_position;
				FOCUSER_POSITION_ITEM->number.max = max_position;
				FOCUSER_STEPS_ITEM->number.max = max_position - min_position;
				FOCUSER_POSITION_ITEM->number.value = FOCUSER_POSITION_ITEM->number.target = PRIVATE_DATA->position = get_number(device, PRIVATE_DATA->has_abs_pos ? GET_MOT1_ABS_POS : GET_MOT1_ABS_POS_STEP);
				if (getToken(device, 0, GET_MOT1_SPEED) == -1) {
					FOCUSER_SPEED_PROPERTY->hidden = true;
				} else {
					FOCUSER_SPEED_ITEM->number.value = FOCUSER_SPEED_ITEM->number.target = get_number(device, GET_MOT1_SPEED);
					FOCUSER_SPEED_PROPERTY->hidden = false;
				}
				FOCUSER_BACKLASH_ITEM->number.value = FOCUSER_BACKLASH_ITEM->number.target = PRIVATE_DATA->backlash = (int)get_number(device, GET_MOT1_BKLASH);
				PRIVATE_DATA->speed = (int)FOCUSER_SPEED_ITEM->number.value;
				PRIVATE_DATA->external_motion = PRIVATE_DATA->poll_alert = PRIVATE_DATA->abort_requested = PRIVATE_DATA->calibrating = PRIVATE_DATA->pending = false;
				PRIVATE_DATA->environment_ticks = 0;
				// Models without stored positions report no PRESET_n.
				bool has_presets = getToken(device, 0, GET_PRESET_1) != -1;
				X_PRESETS_PROPERTY->hidden = X_PRESET_NAMES_PROPERTY->hidden = X_PRESET_GOTO_PROPERTY->hidden = !has_presets;
				for (int i = 0; has_presets && i < PRESET_COUNT; i++) {
					char key[16];
					snprintf(key, sizeof(key), "PRESET_%d", i + 1);
					char *position_path[] = { "res", "get", key, "M1POS", NULL };
					char *name_path[] = { "res", "get", key, "NAME", NULL };
					PRIVATE_DATA->preset_positions[i] = (int)get_number(device, position_path);
					X_PRESETS_PROPERTY->items[i].number.value = X_PRESETS_PROPERTY->items[i].number.target = PRIVATE_DATA->preset_positions[i];
					X_PRESETS_PROPERTY->items[i].number.max = FOCUSER_POSITION_ITEM->number.max;
					INDIGO_COPY_VALUE(PRIVATE_DATA->preset_names[i], (text = get_string(device, name_path)) ? text : "");
					INDIGO_COPY_VALUE(X_PRESET_NAMES_PROPERTY->items[i].text.value, PRIVATE_DATA->preset_names[i]);
				}
				X_STATE_MOTOR_TEMP_ITEM->number.value = get_number(device, GET_MOT1_NTC_T);
				X_STATE_VIN_12V_ITEM->number.value = get_number(device, GET_VIN_12V);
				X_STATE_VIN_USB_ITEM->number.value = get_number(device, GET_VIN_USB);
				// LANCFG selects between the access point and a connection to an existing network
				if ((text = get_string(device, GET_LANCFG)) && !strcmp(text, "sta")) {
					indigo_set_switch(X_WIFI_PROPERTY, X_WIFI_STA_ITEM, true);
				} else if ((text = get_string(device, GET_WIFIAP_STATUS))) {
					if (!strcmp(text, "on")) {
						indigo_set_switch(X_WIFI_PROPERTY, X_WIFI_AP_ITEM, true);
					} else {
						indigo_set_switch(X_WIFI_PROPERTY, X_WIFI_OFF_ITEM, true);
					}
				}
				if ((text = get_string(device, GET_WIFIAP_SSID))) {
					INDIGO_COPY_VALUE(X_WIFI_AP_SSID_ITEM->text.value, text);
				}
				if ((text = get_string(device, GET_WIFIAP_PWD))) {
					INDIGO_COPY_VALUE(X_WIFI_AP_PASSWORD_ITEM->text.value, text);
				}
				if ((text = get_string(device, GET_WIFISTA_SSID))) {
					INDIGO_COPY_VALUE(X_WIFI_STA_SSID_ITEM->text.value, text);
				}
				if ((text = get_string(device, GET_WIFISTA_PWD))) {
					INDIGO_COPY_VALUE(X_WIFI_STA_PASSWORD_ITEM->text.value, text);
				}
				if ((text = get_string(device, GET_DIMLEDS))) {
					if (!strcmp(text, "on")) {
						indigo_set_switch(X_LEDS_PROPERTY, X_LEDS_ON_ITEM, true);
					} else if (!strcmp(text, "low")) {
						indigo_set_switch(X_LEDS_PROPERTY, X_LEDS_DIM_ITEM, true);
					} else if (!strcmp(text, "middle")) {
						indigo_set_switch(X_LEDS_PROPERTY, X_LEDS_MIDDLE_ITEM, true);
					} else {
						indigo_set_switch(X_LEDS_PROPERTY, X_LEDS_OFF_ITEM, true);
					}
				}
				X_CONFIG_M1ACC_ITEM->number.value = X_CONFIG_M1ACC_ITEM->number.target = get_number(device, GET_MOT1_FnRUN_ACC);
				X_CONFIG_M1SPD_ITEM->number.value = X_CONFIG_M1SPD_ITEM->number.target = get_number(device, GET_MOT1_FnRUN_SPD);
				X_CONFIG_M1DEC_ITEM->number.value = X_CONFIG_M1DEC_ITEM->number.target = get_number(device, GET_MOT1_FnRUN_DEC);
				X_CONFIG_M1CACC_ITEM->number.value = X_CONFIG_M1CACC_ITEM->number.target = get_number(device, GET_MOT1_FnRUN_CURR_ACC);
				X_CONFIG_M1CSPD_ITEM->number.value = X_CONFIG_M1CSPD_ITEM->number.target = get_number(device, GET_MOT1_FnRUN_CURR_SPD);
				X_CONFIG_M1CDEC_ITEM->number.value = X_CONFIG_M1CDEC_ITEM->number.target = get_number(device, GET_MOT1_FnRUN_CURR_DEC);
				X_CONFIG_M1HOLD_ITEM->number.value = X_CONFIG_M1HOLD_ITEM->number.target = get_number(device, GET_MOT1_FnRUN_CURR_HOLD);
				X_RUNPRESET_L_M1ACC_ITEM->number.value = X_RUNPRESET_L_M1ACC_ITEM->number.target = get_number(device, GET_RUNPRESET_L_M1ACC);
				X_RUNPRESET_L_M1SPD_ITEM->number.value = X_RUNPRESET_L_M1SPD_ITEM->number.target = get_number(device, GET_RUNPRESET_L_M1SPD);
				X_RUNPRESET_L_M1DEC_ITEM->number.value = X_RUNPRESET_L_M1DEC_ITEM->number.target = get_number(device, GET_RUNPRESET_L_M1DEC);
				X_RUNPRESET_L_M1CACC_ITEM->number.value = X_RUNPRESET_L_M1CACC_ITEM->number.target = get_number(device, GET_RUNPRESET_L_M1CACC);
				X_RUNPRESET_L_M1CSPD_ITEM->number.value = X_RUNPRESET_L_M1CSPD_ITEM->number.target = get_number(device, GET_RUNPRESET_L_M1CSPD);
				X_RUNPRESET_L_M1CDEC_ITEM->number.value = X_RUNPRESET_L_M1CDEC_ITEM->number.target = get_number(device, GET_RUNPRESET_L_M1CDEC);
				X_RUNPRESET_L_M1HOLD_ITEM->number.value = X_RUNPRESET_L_M1HOLD_ITEM->number.target = get_number(device, GET_RUNPRESET_L_M1HOLD);
				X_RUNPRESET_M_M1ACC_ITEM->number.value = X_RUNPRESET_M_M1ACC_ITEM->number.target = get_number(device, GET_RUNPRESET_M_M1ACC);
				X_RUNPRESET_M_M1SPD_ITEM->number.value = X_RUNPRESET_M_M1SPD_ITEM->number.target = get_number(device, GET_RUNPRESET_M_M1SPD);
				X_RUNPRESET_M_M1DEC_ITEM->number.value = X_RUNPRESET_M_M1DEC_ITEM->number.target = get_number(device, GET_RUNPRESET_M_M1DEC);
				X_RUNPRESET_M_M1CACC_ITEM->number.value = X_RUNPRESET_M_M1CACC_ITEM->number.target = get_number(device, GET_RUNPRESET_M_M1CACC);
				X_RUNPRESET_M_M1CSPD_ITEM->number.value = X_RUNPRESET_M_M1CSPD_ITEM->number.target = get_number(device, GET_RUNPRESET_M_M1CSPD);
				X_RUNPRESET_M_M1CDEC_ITEM->number.value = X_RUNPRESET_M_M1CDEC_ITEM->number.target = get_number(device, GET_RUNPRESET_M_M1CDEC);
				X_RUNPRESET_M_M1HOLD_ITEM->number.value = X_RUNPRESET_M_M1HOLD_ITEM->number.target = get_number(device, GET_RUNPRESET_M_M1HOLD);
				X_RUNPRESET_S_M1ACC_ITEM->number.value = X_RUNPRESET_S_M1ACC_ITEM->number.target = get_number(device, GET_RUNPRESET_S_M1ACC);
				X_RUNPRESET_S_M1SPD_ITEM->number.value = X_RUNPRESET_S_M1SPD_ITEM->number.target = get_number(device, GET_RUNPRESET_S_M1SPD);
				X_RUNPRESET_S_M1DEC_ITEM->number.value = X_RUNPRESET_S_M1DEC_ITEM->number.target = get_number(device, GET_RUNPRESET_S_M1DEC);
				X_RUNPRESET_S_M1CACC_ITEM->number.value = X_RUNPRESET_S_M1CACC_ITEM->number.target = get_number(device, GET_RUNPRESET_S_M1CACC);
				X_RUNPRESET_S_M1CSPD_ITEM->number.value = X_RUNPRESET_S_M1CSPD_ITEM->number.target = get_number(device, GET_RUNPRESET_S_M1CSPD);
				X_RUNPRESET_S_M1CDEC_ITEM->number.value = X_RUNPRESET_S_M1CDEC_ITEM->number.target = get_number(device, GET_RUNPRESET_S_M1CDEC);
				X_RUNPRESET_S_M1HOLD_ITEM->number.value = X_RUNPRESET_S_M1HOLD_ITEM->number.target = get_number(device, GET_RUNPRESET_S_M1HOLD);
				X_RUNPRESET_1_M1ACC_ITEM->number.value = X_RUNPRESET_1_M1ACC_ITEM->number.target = get_number(device, GET_RUNPRESET_1_M1ACC);
				X_RUNPRESET_1_M1SPD_ITEM->number.value = X_RUNPRESET_1_M1SPD_ITEM->number.target = get_number(device, GET_RUNPRESET_1_M1SPD);
				X_RUNPRESET_1_M1DEC_ITEM->number.value = X_RUNPRESET_1_M1DEC_ITEM->number.target = get_number(device, GET_RUNPRESET_1_M1DEC);
				X_RUNPRESET_1_M1CACC_ITEM->number.value = X_RUNPRESET_1_M1CACC_ITEM->number.target = get_number(device, GET_RUNPRESET_1_M1CACC);
				X_RUNPRESET_1_M1CSPD_ITEM->number.value = X_RUNPRESET_1_M1CSPD_ITEM->number.target = get_number(device, GET_RUNPRESET_1_M1CSPD);
				X_RUNPRESET_1_M1CDEC_ITEM->number.value = X_RUNPRESET_1_M1CDEC_ITEM->number.target = get_number(device, GET_RUNPRESET_1_M1CDEC);
				X_RUNPRESET_1_M1HOLD_ITEM->number.value = X_RUNPRESET_1_M1HOLD_ITEM->number.target = get_number(device, GET_RUNPRESET_1_M1HOLD);
				X_RUNPRESET_2_M1ACC_ITEM->number.value = X_RUNPRESET_2_M1ACC_ITEM->number.target = get_number(device, GET_RUNPRESET_2_M1ACC);
				X_RUNPRESET_2_M1SPD_ITEM->number.value = X_RUNPRESET_2_M1SPD_ITEM->number.target = get_number(device, GET_RUNPRESET_2_M1SPD);
				X_RUNPRESET_2_M1DEC_ITEM->number.value = X_RUNPRESET_2_M1DEC_ITEM->number.target = get_number(device, GET_RUNPRESET_2_M1DEC);
				X_RUNPRESET_2_M1CACC_ITEM->number.value = X_RUNPRESET_2_M1CACC_ITEM->number.target = get_number(device, GET_RUNPRESET_2_M1CACC);
				X_RUNPRESET_2_M1CSPD_ITEM->number.value = X_RUNPRESET_2_M1CSPD_ITEM->number.target = get_number(device, GET_RUNPRESET_2_M1CSPD);
				X_RUNPRESET_2_M1CDEC_ITEM->number.value = X_RUNPRESET_2_M1CDEC_ITEM->number.target = get_number(device, GET_RUNPRESET_2_M1CDEC);
				X_RUNPRESET_2_M1HOLD_ITEM->number.value = X_RUNPRESET_2_M1HOLD_ITEM->number.target = get_number(device, GET_RUNPRESET_2_M1HOLD);
				X_RUNPRESET_3_M1ACC_ITEM->number.value = X_RUNPRESET_3_M1ACC_ITEM->number.target = get_number(device, GET_RUNPRESET_3_M1ACC);
				X_RUNPRESET_3_M1SPD_ITEM->number.value = X_RUNPRESET_3_M1SPD_ITEM->number.target = get_number(device, GET_RUNPRESET_3_M1SPD);
				X_RUNPRESET_3_M1DEC_ITEM->number.value = X_RUNPRESET_3_M1DEC_ITEM->number.target = get_number(device, GET_RUNPRESET_3_M1DEC);
				X_RUNPRESET_3_M1CACC_ITEM->number.value = X_RUNPRESET_3_M1CACC_ITEM->number.target = get_number(device, GET_RUNPRESET_3_M1CACC);
				X_RUNPRESET_3_M1CSPD_ITEM->number.value = X_RUNPRESET_3_M1CSPD_ITEM->number.target = get_number(device, GET_RUNPRESET_3_M1CSPD);
				X_RUNPRESET_3_M1CDEC_ITEM->number.value = X_RUNPRESET_3_M1CDEC_ITEM->number.target = get_number(device, GET_RUNPRESET_3_M1CDEC);
				X_RUNPRESET_3_M1HOLD_ITEM->number.value = X_RUNPRESET_3_M1HOLD_ITEM->number.target = get_number(device, GET_RUNPRESET_3_M1HOLD);
				if (get_number(device, GET_MOT1_HOLDCURR_STATUS)) {
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
			indigo_send_message(device, ALERT_PROPERTY, "Failed to connect to %s on %s", FOCUSER_DEVICE_NAME, DEVICE_PORT_ITEM->text.value);
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
			if (!focuser_stop(device, &reason)) {
				INDIGO_DRIVER_ERROR(DRIVER_NAME, "Failed to stop the focuser");
			}
		}
		PRIVATE_DATA->external_motion = PRIVATE_DATA->poll_alert = PRIVATE_DATA->abort_requested = PRIVATE_DATA->calibrating = PRIVATE_DATA->pending = false;
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
		result = primaluce_command(device, "{\"req\":{\"cmd\":{\"AP_SET_STATUS\":\"off\"}}}");
	} else if (X_WIFI_AP_ITEM->sw.value) {
		result = primaluce_set_lan_cfg(device, "ap") && primaluce_command(device, "{\"req\":{\"cmd\":{\"AP_SET_STATUS\":\"on\"}}}");
	} else if (X_WIFI_STA_ITEM->sw.value) {
		result = primaluce_set_lan_cfg(device, "sta");
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
	if (!primaluce_command(device, "{\"req\":{\"set\":{\"WIFIAP\":{\"SSID\":\"%s\", \"PWD\":\"%s\"}}}}", X_WIFI_AP_SSID_ITEM->text.value, X_WIFI_AP_PASSWORD_ITEM->text.value)) {
		X_WIFI_AP_PROPERTY->state = INDIGO_ALERT_STATE;
	}
	//- focuser.X_WIFI_AP.on_change
	indigo_update_property(device, X_WIFI_AP_PROPERTY, NULL);
}

static void focuser_x_wifi_sta_handler(indigo_device *device) {
	X_WIFI_STA_PROPERTY->state = INDIGO_OK_STATE;
	//+ focuser.X_WIFI_STA.on_change
	if (!primaluce_command(device, "{\"req\":{\"set\":{\"WIFISTA\":{\"SSID\":\"%s\", \"PWD\":\"%s\"}}}}", X_WIFI_STA_SSID_ITEM->text.value, X_WIFI_STA_PASSWORD_ITEM->text.value)) {
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
		result = primaluce_command(device, "{\"req\":{\"cmd\":{\"DIMLEDS\":\"off\"}}}");
	} else if (X_LEDS_DIM_ITEM->sw.value) {
		result = primaluce_command(device, "{\"req\":{\"cmd\":{\"DIMLEDS\":\"low\"}}}");
	} else if (X_LEDS_MIDDLE_ITEM->sw.value) {
		result = primaluce_command(device, "{\"req\":{\"cmd\":{\"DIMLEDS\":\"middle\"}}}");
	} else if (X_LEDS_ON_ITEM->sw.value) {
		result = primaluce_command(device, "{\"req\":{\"cmd\":{\"DIMLEDS\":\"on\"}}}");
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
	if (!primaluce_command(device, "{\"req\":{\"set\":{\"RUNPRESET_1\":{\"M1ACC\":%d,\"M1DEC\":%d,\"M1SPD\":%d,\"M1CACC\":%d,\"M1CDEC\":%d,\"M1CSPD\":%d,\"M1HOLD\":%d}}}}", (int)X_RUNPRESET_1_M1ACC_ITEM->number.target, (int)X_RUNPRESET_1_M1DEC_ITEM->number.target, (int)X_RUNPRESET_1_M1SPD_ITEM->number.target, (int)X_RUNPRESET_1_M1CACC_ITEM->number.target, (int)X_RUNPRESET_1_M1CDEC_ITEM->number.target, (int)X_RUNPRESET_1_M1CSPD_ITEM->number.target, (int)X_RUNPRESET_1_M1HOLD_ITEM->number.target)) {
		INDIGO_UPDATE_PROPERTY_STATE(X_RUNPRESET_1_PROPERTY, INDIGO_ALERT_STATE, NULL);
		return;
	}
	//- focuser.X_RUNPRESET_1.on_change
	indigo_update_property(device, X_RUNPRESET_1_PROPERTY, NULL);
}

static void focuser_x_runpreset_2_handler(indigo_device *device) {
	X_RUNPRESET_2_PROPERTY->state = INDIGO_OK_STATE;
	//+ focuser.X_RUNPRESET_2.on_change
	if (!primaluce_command(device, "{\"req\":{\"set\":{\"RUNPRESET_2\":{\"M1ACC\":%d,\"M1DEC\":%d,\"M1SPD\":%d,\"M1CACC\":%d,\"M1CDEC\":%d,\"M1CSPD\":%d,\"M1HOLD\":%d}}}}", (int)X_RUNPRESET_2_M1ACC_ITEM->number.target, (int)X_RUNPRESET_2_M1DEC_ITEM->number.target, (int)X_RUNPRESET_2_M1SPD_ITEM->number.target, (int)X_RUNPRESET_2_M1CACC_ITEM->number.target, (int)X_RUNPRESET_2_M1CDEC_ITEM->number.target, (int)X_RUNPRESET_2_M1CSPD_ITEM->number.target, (int)X_RUNPRESET_2_M1HOLD_ITEM->number.target)) {
		INDIGO_UPDATE_PROPERTY_STATE(X_RUNPRESET_2_PROPERTY, INDIGO_ALERT_STATE, NULL);
		return;
	}
	//- focuser.X_RUNPRESET_2.on_change
	indigo_update_property(device, X_RUNPRESET_2_PROPERTY, NULL);
}

static void focuser_x_runpreset_3_handler(indigo_device *device) {
	X_RUNPRESET_3_PROPERTY->state = INDIGO_OK_STATE;
	//+ focuser.X_RUNPRESET_3.on_change
	if (!primaluce_command(device, "{\"req\":{\"set\":{\"RUNPRESET_3\":{\"M1ACC\":%d,\"M1DEC\":%d,\"M1SPD\":%d,\"M1CACC\":%d,\"M1CDEC\":%d,\"M1CSPD\":%d,\"M1HOLD\":%d}}}}", (int)X_RUNPRESET_3_M1ACC_ITEM->number.target, (int)X_RUNPRESET_3_M1DEC_ITEM->number.target, (int)X_RUNPRESET_3_M1SPD_ITEM->number.target, (int)X_RUNPRESET_3_M1CACC_ITEM->number.target, (int)X_RUNPRESET_3_M1CDEC_ITEM->number.target, (int)X_RUNPRESET_3_M1CSPD_ITEM->number.target, (int)X_RUNPRESET_3_M1HOLD_ITEM->number.target)) {
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
		result = primaluce_command(device, "{\"req\":{\"cmd\":{\"RUNPRESET\":\"light\"}}}");
		X_RUNPRESET_L_ITEM->sw.value = false;
	} else if (X_RUNPRESET_M_ITEM->sw.value) {
		result = primaluce_command(device, "{\"req\":{\"cmd\":{\"RUNPRESET\":\"medium\"}}}");
		X_RUNPRESET_M_ITEM->sw.value = false;
	} else if (X_RUNPRESET_S_ITEM->sw.value) {
		result = primaluce_command(device, "{\"req\":{\"cmd\":{\"RUNPRESET\":\"slow\"}}}");
		X_RUNPRESET_S_ITEM->sw.value = false;
	} else if (X_RUNPRESET_1_ITEM->sw.value) {
		result = primaluce_command(device, "{\"req\":{\"cmd\":{\"RUNPRESET\":1}}}");
		X_RUNPRESET_1_ITEM->sw.value = false;
	} else if (X_RUNPRESET_2_ITEM->sw.value) {
		result = primaluce_command(device, "{\"req\":{\"cmd\":{\"RUNPRESET\":2}}}");
		X_RUNPRESET_2_ITEM->sw.value = false;
	} else if (X_RUNPRESET_3_ITEM->sw.value) {
		result = primaluce_command(device, "{\"req\":{\"cmd\":{\"RUNPRESET\":3}}}");
		X_RUNPRESET_3_ITEM->sw.value = false;
	}
	if (!result) {
		X_RUNPRESET_PROPERTY->state = INDIGO_ALERT_STATE;
	} else  if (!primaluce_command(device, "{\"req\":{\"get\":{\"MOT1\":{\"HOLDCURR_STATUS\":\"\",\"FnRUN_SPD\":\"\",\"FnRUN_DEC\":\"\",\"FnRUN_ACC\":\"\",\"FnRUN_CURR_SPD\":\"\",\"FnRUN_CURR_DEC\":\"\",\"FnRUN_CURR_ACC\":\"\",\"FnRUN_CURR_HOLD\":\"\"}}}}")) {
		X_CONFIG_PROPERTY->state = INDIGO_ALERT_STATE;
	}
	X_CONFIG_M1ACC_ITEM->number.value = X_CONFIG_M1ACC_ITEM->number.target = get_number(device, GET_MOT1_FnRUN_ACC);
	X_CONFIG_M1SPD_ITEM->number.value = X_CONFIG_M1SPD_ITEM->number.target = get_number(device, GET_MOT1_FnRUN_SPD);
	X_CONFIG_M1DEC_ITEM->number.value = X_CONFIG_M1DEC_ITEM->number.target = get_number(device, GET_MOT1_FnRUN_DEC);
	X_CONFIG_M1CACC_ITEM->number.value = X_CONFIG_M1CACC_ITEM->number.target = get_number(device, GET_MOT1_FnRUN_CURR_ACC);
	X_CONFIG_M1CSPD_ITEM->number.value = X_CONFIG_M1CSPD_ITEM->number.target = get_number(device, GET_MOT1_FnRUN_CURR_SPD);
	X_CONFIG_M1CDEC_ITEM->number.value = X_CONFIG_M1CDEC_ITEM->number.target = get_number(device, GET_MOT1_FnRUN_CURR_DEC);
	X_CONFIG_M1HOLD_ITEM->number.value = X_CONFIG_M1HOLD_ITEM->number.target = get_number(device, GET_MOT1_FnRUN_CURR_HOLD);
	indigo_update_property(device, X_CONFIG_PROPERTY, NULL);
	//- focuser.X_RUNPRESET.on_change
	indigo_update_property(device, X_RUNPRESET_PROPERTY, NULL);
}

static void focuser_x_presets_handler(indigo_device *device) {
	X_PRESETS_PROPERTY->state = INDIGO_OK_STATE;
	//+ focuser.X_PRESETS.on_change
	// The controller refuses a zero position, so only positions that changed to a stored value are written.
	for (int i = 0; i < PRESET_COUNT; i++) {
		int position = (int)X_PRESETS_PROPERTY->items[i].number.target;
		if (position == PRIVATE_DATA->preset_positions[i]) {
			continue;
		}
		char key[16];
		snprintf(key, sizeof(key), "PRESET_%d", i + 1);
		char *path[] = { "res", "set", key, "M1POS", NULL };
		char *state;
		if (position > 0 && primaluce_command(device, "{\"req\":{\"set\":{\"%s\":{\"M1POS\":%d}}}}", key, position) && (state = get_string(device, path)) != NULL && !strcmp(state, "done")) {
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
	for (int i = 0; i < PRESET_COUNT; i++) {
		char *name = X_PRESET_NAMES_PROPERTY->items[i].text.value;
		if (!strcmp(name, PRIVATE_DATA->preset_names[i])) {
			continue;
		}
		char key[16];
		snprintf(key, sizeof(key), "PRESET_%d", i + 1);
		char *path[] = { "res", "set", key, "NAME", NULL };
		char *state;
		if (strchr(name, '"') == NULL && strchr(name, '\\') == NULL && primaluce_command(device, "{\"req\":{\"set\":{\"%s\":{\"NAME\":\"%s\"}}}}", key, name) && (state = get_string(device, path)) != NULL && !strcmp(state, "done")) {
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
	for (int i = 0; i < PRESET_COUNT; i++) {
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
		result = primaluce_command(device, "{\"req\":{\"set\":{\"MOT1\":{\"HOLDCURR_STATUS\":0}}}}");
	} else if (X_HOLD_CURR_ON_ITEM->sw.value) {
		result = primaluce_command(device, "{\"req\":{\"set\":{\"MOT1\":{\"HOLDCURR_STATUS\":1}}}}");
	}
	if (result) {
		char *state = get_string(device, SET_MOT1_HOLDCURR_STATUS);
		result = state != NULL && !strcmp(state, "done");
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
	X_CALIBRATE_F_PROPERTY->state = INDIGO_OK_STATE;
	//+ focuser.X_CALIBRATE_F.on_change
	bool result = true;
	bool started = X_CALIBRATE_F_START_ITEM->sw.value || X_CALIBRATE_F_START_INVERTED_ITEM->sw.value;
	if (X_CALIBRATE_F_START_ITEM->sw.value) {
		result = primaluce_command(device, "{\"req\":{\"cmd\": {\"MOT1\": {\"CAL_FOCUSER\":\"Init\"}}}}");
		if (result) {
			indigo_usleep(1000000);
			result = primaluce_command(device, "{\"req\":{\"set\": {\"MOT1\": {\"CAL_DIR\":\"normal\"}}}}");
		}
		if (result) {
			indigo_usleep(1000000);
			result = primaluce_command(device, "{\"req\":{\"cmd\": {\"MOT1\": {\"CAL_FOCUSER\":\"StoreAsMinPos\"}}}}");
		}
		if (result) {
			indigo_usleep(1000000);
			result = primaluce_command(device, "{\"req\":{\"cmd\": {\"MOT1\": {\"CAL_FOCUSER\":\"GoOutToFindMaxPos\"}}}}");
		}
	} else if (X_CALIBRATE_F_START_INVERTED_ITEM->sw.value) {
		result = primaluce_command(device, "{\"req\":{\"cmd\": {\"MOT1\": {\"CAL_FOCUSER\":\"Init\"}}}}");
		if (result) {
			indigo_usleep(1000000);
			result = primaluce_command(device, "{\"req\":{\"set\": {\"MOT1\": {\"CAL_DIR\":\"invert\"}}}}");
		}
		if (result) {
			indigo_usleep(1000000);
			result = primaluce_command(device, "{\"req\":{\"cmd\": {\"MOT1\": {\"CAL_FOCUSER\":\"StoreAsMinPos\"}}}}");
		}
		if (result) {
			indigo_usleep(1000000);
			result = primaluce_command(device, "{\"req\":{\"cmd\": {\"MOT1\": {\"CAL_FOCUSER\":\"GoOutToFindMaxPos\"}}}}");
		}
	} else if (X_CALIBRATE_F_END_ITEM->sw.value) {
		X_CALIBRATE_F_START_ITEM->sw.value = X_CALIBRATE_F_START_INVERTED_ITEM->sw.value = X_CALIBRATE_F_END_ITEM->sw.value = false;
		result = primaluce_command(device, "{\"req\":{\"cmd\": {\"MOT1\": {\"CAL_FOCUSER\":\"StoreAsMaxPos\"}}}}");
		if (result) {
			char *get_pos_command = PRIVATE_DATA->has_abs_pos ? "{\"req\":{\"get\":{\"MOT1\":{\"ABS_POS\":\"STEP\",\"STATUS\":\"\"}}}}" : "{\"req\":{\"get\":{\"MOT1\":{\"ABS_POS_STEP\":\"\",\"STATUS\":\"\"}}}}";
			if (primaluce_command(device, get_pos_command)) {
				FOCUSER_POSITION_ITEM->number.value = FOCUSER_POSITION_ITEM->number.target = PRIVATE_DATA->position = get_number(device, PRIVATE_DATA->has_abs_pos ? GET_MOT1_ABS_POS : GET_MOT1_ABS_POS_STEP);
				indigo_update_property(device, FOCUSER_POSITION_PROPERTY, NULL);
			}
		}
	}
	// The motor runs out to the far end between START and END.
	PRIVATE_DATA->calibrating = result && started;
	if (!result) {
		X_CALIBRATE_F_START_ITEM->sw.value = X_CALIBRATE_F_START_INVERTED_ITEM->sw.value = X_CALIBRATE_F_END_ITEM->sw.value = false;
		X_CALIBRATE_F_PROPERTY->state = INDIGO_ALERT_STATE;
	}
	//- focuser.X_CALIBRATE_F.on_change
	indigo_update_property(device, X_CALIBRATE_F_PROPERTY, NULL);
}

static void focuser_backlash_handler(indigo_device *device) {
	FOCUSER_BACKLASH_PROPERTY->state = INDIGO_OK_STATE;
	//+ focuser.FOCUSER_BACKLASH.on_change
	char *state = NULL;
	if (!primaluce_command(device, "{\"req\":{\"set\":{\"MOT1\":{\"BKLASH\":%d}}}}", (int)FOCUSER_BACKLASH_ITEM->number.target) || (state = get_string(device, SET_MOT1_BKLASH)) == NULL || strcmp(state, "done")) {
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
		PRIVATE_DATA->last_error[0] = 0;
		focuser_motion_failed(device, NULL);
	} else if (!primaluce_command(device, PRIVATE_DATA->is_sestosenso_3 ? "{\"req\":{\"cmd\":{\"MOT1\":{\"GOTO\":%d}}}}" : "{\"req\":{\"cmd\":{\"MOT1\":{\"MOVE_ABS\":{\"STEP\":%d}}}}}", (int)FOCUSER_POSITION_ITEM->number.target)) {
		focuser_motion_failed(device, NULL);
	} else {
		char *state = get_string(device, PRIVATE_DATA->is_sestosenso_3 ? CMD_MOT1_GOTO : CMD_MOT1_STEP);
		if (state == NULL || strcmp(state, "done")) {
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
	char *state = NULL;
	if (!primaluce_command(device, "{\"req\":{\"set\":{\"MOT1\":{\"SPEED\":%d}}}}", (int)FOCUSER_SPEED_ITEM->number.target) || (state = get_string(device, SET_MOT1_SPEED)) == NULL || strcmp(state, "done")) {
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
	FOCUSER_ABORT_MOTION_PROPERTY->state = INDIGO_OK_STATE;
	//+ focuser.FOCUSER_ABORT_MOTION.on_change
	// A running motion is owned by the focuser movement finalizer, so the motion
	// properties are left busy here. That finalizer observes the stop, reads the position
	// the draw tube actually reached and publishes it as ALERT because the requested
	// target was not reached. A move still queued behind this abort is never sent.
	bool requested = FOCUSER_ABORT_MOTION_ITEM->sw.value;
	FOCUSER_ABORT_MOTION_ITEM->sw.value = false;
	bool moving = PRIVATE_DATA->pending || FOCUSER_POSITION_PROPERTY->state == INDIGO_BUSY_STATE || FOCUSER_STEPS_PROPERTY->state == INDIGO_BUSY_STATE;
	if (requested && !moving && !PRIVATE_DATA->calibrating && PRIVATE_DATA->link_failed) {
		// Nothing to stop, but the controller did not answer the last request.
		INDIGO_UPDATE_PROPERTY_STATE(FOCUSER_ABORT_MOTION_PROPERTY, INDIGO_ALERT_STATE, NULL);
		return;
	}
	if (requested && (moving || PRIVATE_DATA->calibrating)) {
		char *reason;
		if (!focuser_stop(device, &reason)) {
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
	//- focuser.FOCUSER_ABORT_MOTION.on_change
	indigo_update_property(device, FOCUSER_ABORT_MOTION_PROPERTY, NULL);
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
		INDIGO_COPY_VALUES_PROCESS_CHANGE(X_CALIBRATE_F_PROPERTY, focuser_x_calibrate_f_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(FOCUSER_BACKLASH_PROPERTY, property)) {
		INDIGO_REJECT_CHANGE_IF(FOCUSER_POSITION_PROPERTY->state == INDIGO_BUSY_STATE || FOCUSER_STEPS_PROPERTY->state == INDIGO_BUSY_STATE, FOCUSER_BACKLASH_PROPERTY, "The focuser is moving");
		INDIGO_COPY_VALUES_PROCESS_CHANGE(FOCUSER_BACKLASH_PROPERTY, focuser_backlash_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(FOCUSER_POSITION_PROPERTY, property)) {
		INDIGO_REJECT_CHANGE_IF(PRIVATE_DATA->pending || FOCUSER_POSITION_PROPERTY->state == INDIGO_BUSY_STATE || FOCUSER_STEPS_PROPERTY->state == INDIGO_BUSY_STATE, FOCUSER_POSITION_PROPERTY, "Another motion operation is pending");
		//+ focuser.FOCUSER_POSITION.on_change_request
		// The accepted move is pending until its handler runs.
		PRIVATE_DATA->pending = true;
		//- focuser.FOCUSER_POSITION.on_change_request
		INDIGO_COPY_VALUES_PROCESS_CHANGE(FOCUSER_POSITION_PROPERTY, focuser_position_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(FOCUSER_STEPS_PROPERTY, property)) {
		INDIGO_REJECT_CHANGE_IF(PRIVATE_DATA->pending || FOCUSER_POSITION_PROPERTY->state == INDIGO_BUSY_STATE || FOCUSER_STEPS_PROPERTY->state == INDIGO_BUSY_STATE, FOCUSER_STEPS_PROPERTY, "Another motion operation is pending");
		//+ focuser.FOCUSER_STEPS.on_change_request
		// The accepted move is pending until its handler runs.
		PRIVATE_DATA->pending = true;
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
		if (PRIVATE_DATA->count == 0) {
			connection_result = primaluce_open(device->master_device);
		}
		if (connection_result) {
			PRIVATE_DATA->count++;
		}
		if (connection_result) {
			//+ rotator.on_connect
			char *text;
			if (primaluce_command(device, "{\"req\":{\"set\": {\"ARCO\":1}}}")) {
				if (primaluce_command(device, "{\"req\":{\"get\": \"\"}}")) {
					if ((text = get_string(device, GET_MOT2_ERROR)) && *text) {
						indigo_send_message(device, ALERT_PROPERTY, "%s", text);
					}
					if (get_number(device, GET_CALRESTART_MOT2)) {
						indigo_send_message(device, BUSY_PROPERTY, "ARCO needs calibration");
					}
				}
				PRIVATE_DATA->rotator_has_abs_pos = getToken(device, 0, GET_MOT2_ABS_POS) != -1;
				PRIVATE_DATA->rotator_has_position_deg = getToken(device, 0, GET_MOT2_POSITION_DEG) != -1;
				ROTATOR_POSITION_ITEM->number.value = ROTATOR_POSITION_ITEM->number.target = get_number(device, PRIVATE_DATA->rotator_has_position_deg ? GET_MOT2_POSITION_DEG : (PRIVATE_DATA->rotator_has_abs_pos ? GET_MOT2_ABS_POS : GET_MOT2_ABS_POS_DEG));
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
			indigo_send_message(device, ALERT_PROPERTY, "Failed to connect to %s on %s", ROTATOR_DEVICE_NAME, DEVICE_PORT_ITEM->text.value);
			if (PRIVATE_DATA->count > 0 && --PRIVATE_DATA->count == 0) {
				primaluce_close(device);
			}
			CONNECTION_PROPERTY->state = INDIGO_ALERT_STATE;
			indigo_set_switch(CONNECTION_PROPERTY, CONNECTION_DISCONNECTED_ITEM, true);
		}
	} else {
		indigo_cancel_pending_handlers(device);
		//+ rotator.on_disconnect
		primaluce_command(device, "{\"req\":{\"set\": {\"ARCO\":0}}}");
		//- rotator.on_disconnect
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
		char *state;
		if (!primaluce_command(device, "{\"req\":{\"set\":{\"MOT2\":{\"CAL_STATUS\":\"exec\"}}}}") || (state = get_string(device, SET_MOT2_CAL_STATUS)) == NULL || strcmp(state, "done")) {
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
		char *state = NULL;
		if (primaluce_command(device, "{\"req\":{\"cmd\":{\"MOT2\":{\"SYNC_POS\":{\"DEG\":%g}}}}}", ROTATOR_POSITION_ITEM->number.target) && (state = get_string(device, CMD_MOT2_SYNC_POS)) != NULL && !strcmp(state, "done")) {
			ROTATOR_POSITION_ITEM->number.value = ROTATOR_POSITION_ITEM->number.target;
			ROTATOR_POSITION_PROPERTY->state = INDIGO_OK_STATE;
		} else {
			// The request already replaced the value, so the angle the rotator keeps is read back.
			if (rotator_read_angle(device)) {
				ROTATOR_POSITION_ITEM->number.target = ROTATOR_POSITION_ITEM->number.value;
			}
			ROTATOR_POSITION_PROPERTY->state = INDIGO_ALERT_STATE;
		}
		indigo_update_property(device, ROTATOR_POSITION_PROPERTY, NULL);
	} else if (!primaluce_command(device, "{\"req\":{\"cmd\":{\"MOT2\":{\"MOVE_ABS\":{\"DEG\":%g}}}}}", ROTATOR_POSITION_ITEM->number.target)) {
		ROTATOR_POSITION_PROPERTY->state = INDIGO_ALERT_STATE;
		indigo_update_property(device, ROTATOR_POSITION_PROPERTY, NULL);
	} else {
		char *state = get_string(device, CMD_MOT2_STEP);
		if (state == NULL || strcmp(state, "done")) {
			ROTATOR_POSITION_PROPERTY->state = INDIGO_ALERT_STATE;
			indigo_update_property(device, ROTATOR_POSITION_PROPERTY, NULL);
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
		primaluce_command(device, "{\"req\":{\"set\":{\"MOT2\":{\"CAL_STATUS\":\"stop\"}}}}");
		INDIGO_UPDATE_PROPERTY_STATE(X_CALIBRATE_R_PROPERTY, INDIGO_ALERT_STATE, "Calibration aborted");
	}
	if (ROTATOR_POSITION_PROPERTY->state == INDIGO_BUSY_STATE) {
		INDIGO_UPDATE_PROPERTY_STATE(ROTATOR_POSITION_PROPERTY, INDIGO_ALERT_STATE, NULL);
	}
	ROTATOR_ABORT_MOTION_ITEM->sw.value = false;
	if (!primaluce_command(device, "{\"req\":{\"cmd\":{\"MOT2\":{\"MOT_STOP\":\"\"}}}}")) {
		ROTATOR_ABORT_MOTION_PROPERTY->state = INDIGO_ALERT_STATE;
	} else {
		char *state = get_string(device, CMD_MOT2_MOT_STOP);
		if (state == NULL || strcmp(state, "done")) {
			ROTATOR_ABORT_MOTION_PROPERTY->state = INDIGO_ALERT_STATE;
		}
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
		INDIGO_COPY_VALUES_PROCESS_CHANGE(ROTATOR_POSITION_PROPERTY, rotator_position_handler);
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
