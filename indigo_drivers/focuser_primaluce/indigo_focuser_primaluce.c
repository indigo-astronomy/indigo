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
#include <pthread.h>

//+ include

#define JSMN_STRICT          
#define JSMN_PARENT_LINKS    
#include "jsmn.h"

//- include

#include <indigo/indigo_driver_xml.h>
#include <indigo/indigo_focuser_driver.h>
#include <indigo/indigo_rotator_driver.h>
#include <indigo/indigo_uni_io.h>

#include "indigo_focuser_primaluce.h"

#pragma mark - Common definitions

#define DRIVER_VERSION       0x03000009
#define DRIVER_NAME          "indigo_focuser_primaluce"
#define DRIVER_LABEL         "PrimaluceLab Focuser/Rotator"
#define FOCUSER_DEVICE_NAME  "PrimaluceLab Focuser"
#define ROTATOR_DEVICE_NAME  "PrimaluceLab Rotator"
#define PRIVATE_DATA         ((primaluce_private_data *)device->private_data)

//+ define

#define MAX_RESPONSE_SIZE    4096
#define MAX_TOKEN_COUT       1024

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
#define X_LEDS_ON_ITEM                 (X_LEDS_PROPERTY->items + 2)

#define X_LEDS_PROPERTY_NAME           "X_LEDS"
#define X_LEDS_OFF_ITEM_NAME           "OFF"
#define X_LEDS_DIM_ITEM_NAME           "DIM"
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
	indigo_property *x_hold_curr_property;
	indigo_property *x_calibrate_f_property;
	indigo_property *x_calibrate_r_property;
	//+ data
	char response[MAX_RESPONSE_SIZE];
	jsmntok_t tokens[MAX_TOKEN_COUT];
	jsmn_parser parser;
	bool has_abs_pos;
	bool is_sestosenso_3;
	//- data
} primaluce_private_data;

#pragma mark - Low level code

//+ code

static char *GET_MODNAME[] = { "res", "get", "MODNAME", NULL };
static char *GET_SN[] = { "res", "get", "SN", NULL };
static char *GET_SWAPP[] = { "res", "get", "SWVERS", "SWAPP", NULL };
static char *GET_SWWEB[] = { "res", "get", "SWVERS", "SWWEB", NULL };
static char *GET_CALRESTART_MOT1[] = { "res", "get", "CALRESTART", "MOT1", NULL };
static char *GET_CALRESTART_MOT2[] = { "res", "get", "CALRESTART", "MOT2", NULL };
static char *GET_MOT1_ABS_POS_STEP[] = { "res", "get", "MOT1", "ABS_POS_STEP", NULL };
static char *GET_MOT1_ABS_POS[] = { "res", "get", "MOT1", "ABS_POS", NULL };
static char *GET_MOT2_ABS_POS_DEG[] = { "res", "get", "MOT2", "ABS_POS_DEG", NULL };
static char *GET_MOT2_ABS_POS[] = { "res", "get", "MOT2", "ABS_POS", NULL };
static char *GET_MOT1_BKLASH[] = { "res", "get", "MOT1", "BKLASH", NULL };
static char *SET_MOT1_BKLASH[] = { "res", "set", "MOT1", "BKLASH", NULL };
static char *GET_MOT1_SPEED[] = { "res", "get", "MOT1", "SPEED", NULL };
static char *SET_MOT1_SPEED[] = { "res", "set", "MOT1", "SPEED", NULL };
static char *GET_MOT1_MST[] = { "res", "get", "MOT1", "STATUS", "MST", NULL };
static char *CMD_MOT1_STEP[] = { "res", "cmd", "MOT1", "STEP", NULL };
static char *CMD_MOT1_GOTO[] = { "res", "cmd", "MOT1", "GOTO", NULL };
//static char *CMD_MOT1_MOVE_REL[] = { "res", "cmd", "MOT1", "MOVE_REL", NULL };
static char *CMD_MOT1_MOT_STOP[] = { "res", "cmd", "MOT1", "MOT_STOP", NULL };
static char *CMD_MOT2_STEP[] = { "res", "cmd", "MOT2", "STEP", NULL };
static char *CMD_MOT2_MOT_STOP[] = { "res", "cmd", "MOT2", "MOT_STOP", NULL };
static char *CMD_MOT2_CAL_STATUS[] = { "res", "cmd", "MOT2", "CAL_STATUS", NULL };
static char *GET_EXT_T[] = { "res", "get", "EXT_T", NULL };
static char *GET_DIMLEDS[] = { "res", "get", "DIMLEDS", NULL };
static char *GET_VIN_12V[] = { "res", "get", "VIN_12V", NULL };
static char *GET_VIN_USB[] = { "res", "get", "VIN_USB", NULL };
static char *GET_MOT1_NTC_T[] = { "res", "get", "MOT1", "NTC_T", NULL };
static char *GET_MOT1_ERROR[] = { "res", "get", "MOT1", "ERROR", NULL };
static char *GET_MOT2_ERROR[] = { "res", "get", "MOT1", "ERROR", NULL };
static char *GET_WIFIAP_STATUS[] = { "res", "get", "WIFIAP", "STATUS", NULL };
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

static bool primaluce_command(indigo_device *device, char *command, ...) {
	long result;
	va_list args;
	va_start(args, command);
	result = indigo_uni_vprintf(PRIVATE_DATA->handle, command, args);
	va_end(args);
	if (result <= 0) {
		return false;
	}
	while (true) {
		result = indigo_uni_read_section(PRIVATE_DATA->handle, PRIVATE_DATA->response, MAX_RESPONSE_SIZE, "\n", "\r\n", -1);
		if (result < 1) {
			return false;
		}
		if (*PRIVATE_DATA->response == '[') {
			INDIGO_DRIVER_DEBUG(DRIVER_NAME, "Ignored");
			continue;
		}
		break;
	}
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

static bool primaluce_open(indigo_device *device) {
	char *name = DEVICE_PORT_ITEM->text.value;
	PRIVATE_DATA->handle = indigo_uni_open_serial_with_speed(name, 115200, INDIGO_LOG_DEBUG);
	if (PRIVATE_DATA->handle != NULL) {
		char *text;
		if (primaluce_command(device, "{\"req\":{\"get\":{\"MODNAME\":\"\"}}}") && (text = get_string(device, GET_MODNAME))) {
			if (!strncmp(text, "SESTOSENSO", 10) || !strncmp(text, "ESATTO", 6)) {
				PRIVATE_DATA->is_sestosenso_3 = strncmp(text, "SESTOSENSO3", 11)==0;
				if (primaluce_command(device, "{\"req\":{\"get\":{\"SWVERS\":{\"SWAPP\":\"\"}}}}") && (text = get_string(device, GET_SWAPP))) {
					double version = atof(text);
					if (!PRIVATE_DATA->is_sestosenso_3 && version < 3.05) {
						indigo_send_message(device, BUSY_PROPERTY, "Warning: %s has firmware version %.2f and at least 3.05 is needed", INFO_DEVICE_MODEL_ITEM->text.value, version);
					}
					//primaluce_command(device, "{\"req\":{\"cmd\":{\"LOGLEVEL\":\"no output\"}}}");
					return true;
				} else {
					INDIGO_DRIVER_ERROR(DRIVER_NAME, "Unsupported version");
				}
			} else {
				INDIGO_DRIVER_ERROR(DRIVER_NAME, "Unsupported device");
			}
		}
		indigo_uni_close(&PRIVATE_DATA->handle);
	}
	return false;
}

static void primaluce_close(indigo_device *device) {
	if (PRIVATE_DATA->handle != NULL) {
		indigo_uni_close(&PRIVATE_DATA->handle);
		INDIGO_COPY_VALUE(INFO_DEVICE_MODEL_ITEM->text.value, "N/A");
		INDIGO_COPY_VALUE(INFO_DEVICE_SERIAL_NUM_ITEM->text.value, "N/A");
		INDIGO_COPY_VALUE(INFO_DEVICE_FW_REVISION_ITEM->text.value, "N/A");
		indigo_update_property(device, INFO_PROPERTY, NULL);
		INDIGO_DRIVER_LOG(DRIVER_NAME, "Disconnected from %s", DEVICE_PORT_ITEM->text.value);
	}
}

//- code

//+ focuser.code

static void focuser_movement_finalizer(indigo_device *device) {
	if (primaluce_command(device, PRIVATE_DATA->has_abs_pos ? "{\"req\":{\"get\":{\"MOT1\":{\"ABS_POS\":\"STEP\",\"STATUS\":\"\"}}}}" : "{\"req\":{\"get\":{\"MOT1\":{\"ABS_POS_STEP\":\"\",\"STATUS\":\"\"}}}}")) {
		FOCUSER_POSITION_ITEM->number.value = get_number(device, PRIVATE_DATA->has_abs_pos ? GET_MOT1_ABS_POS : GET_MOT1_ABS_POS_STEP);
		if (!PRIVATE_DATA->is_sestosenso_3 || primaluce_command(device, "{\"req\":{\"get\":{\"MOT1\":{\"STATUS\":{\"MST\":\"\"}}}}}")) {
			if (strcmp(get_string(device, GET_MOT1_MST), "stop")) {
				indigo_execute_handler(device, focuser_movement_finalizer);
			} else {
				for (int i = 0; i < 10; i++) {
					indigo_usleep(100000);
					if (primaluce_command(device, PRIVATE_DATA->has_abs_pos ? "{\"req\":{\"get\":{\"MOT1\":{\"ABS_POS\":\"STEP\",\"STATUS\":\"\"}}}}" : "{\"req\":{\"get\":{\"MOT1\":{\"ABS_POS_STEP\":\"\",\"STATUS\":\"\"}}}}")) {
						FOCUSER_POSITION_ITEM->number.value = get_number(device, PRIVATE_DATA->has_abs_pos ? GET_MOT1_ABS_POS : GET_MOT1_ABS_POS_STEP);
					}
					if (FOCUSER_POSITION_ITEM->number.target == FOCUSER_POSITION_ITEM->number.value) {
						FOCUSER_POSITION_PROPERTY->state = FOCUSER_STEPS_PROPERTY->state = INDIGO_OK_STATE;
						break;
					}
				}
				if (FOCUSER_POSITION_PROPERTY->state == INDIGO_BUSY_STATE) {
					FOCUSER_POSITION_PROPERTY->state = FOCUSER_STEPS_PROPERTY->state = INDIGO_ALERT_STATE;
				}
			}
		}
		indigo_update_property(device, FOCUSER_POSITION_PROPERTY, NULL);
		indigo_update_property(device, FOCUSER_STEPS_PROPERTY, NULL);
	}
}

//- focuser.code

//+ rotator.code

static void rotator_movement_finalizer(indigo_device *device) {
	if (primaluce_command(device, PRIVATE_DATA->has_abs_pos ? "{\"req\":{\"get\":{\"MOT2\":{\"ABS_POS\":\"DEG\",\"STATUS\":\"\"}}}}" : "{\"req\":{\"get\":{\"MOT2\":{\"ABS_POS_DEG\":\"\",\"STATUS\":\"\"}}}}")) {
		ROTATOR_POSITION_ITEM->number.value = get_number(device, PRIVATE_DATA->has_abs_pos ? GET_MOT2_ABS_POS : GET_MOT2_ABS_POS_DEG);
		if (strcmp(get_string(device, CMD_MOT2_STEP), "stop")) {
			indigo_execute_handler(device, rotator_movement_finalizer);
		} else {
			for (int i = 0; i < 10; i++) {
				indigo_usleep(100000);
				if (primaluce_command(device, PRIVATE_DATA->has_abs_pos ? "{\"req\":{\"get\":{\"MOT2\":{\"ABS_POS\":\"DEG\",\"STATUS\":\"\"}}}}" : "{\"req\":{\"get\":{\"MOT2\":{\"ABS_POS_DEG\":\"\",\"STATUS\":\"\"}}}}")) {
					ROTATOR_POSITION_ITEM->number.value = get_number(device, PRIVATE_DATA->has_abs_pos ? GET_MOT2_ABS_POS : GET_MOT2_ABS_POS_DEG);
				}
				if (ROTATOR_POSITION_ITEM->number.target == ROTATOR_POSITION_ITEM->number.value) {
					ROTATOR_POSITION_PROPERTY->state = INDIGO_OK_STATE;
					break;
				}
			}
			if (ROTATOR_POSITION_PROPERTY->state == INDIGO_BUSY_STATE) {
				ROTATOR_POSITION_PROPERTY->state = INDIGO_ALERT_STATE;
			}
		}
	}
	indigo_update_property(device, ROTATOR_POSITION_PROPERTY, NULL);
}

//- rotator.code

#pragma mark - High level code (focuser)

static void focuser_timer_callback(indigo_device *device) {
	if (!IS_CONNECTED) {
		return;
	}
	//+ focuser.on_timer
	if (primaluce_command(device, "{\"req\":{\"get\":{\"EXT_T\":\"\", \"VIN_12V\": \"\", \"MOT1\":{\"NTC_T\":\"\"}}}}")) {
		double temp = get_number(device, GET_EXT_T);
		if (temp != FOCUSER_TEMPERATURE_ITEM->number.value) {
			FOCUSER_TEMPERATURE_ITEM->number.value = temp;
			indigo_update_property(device, FOCUSER_TEMPERATURE_PROPERTY, NULL);
		}
		double motor_temp = get_number(device, GET_MOT1_NTC_T);
		double vin12v = get_number(device, GET_VIN_12V);
		if (motor_temp != X_STATE_MOTOR_TEMP_ITEM->number.value || vin12v != X_STATE_VIN_12V_ITEM->number.value) {
			X_STATE_MOTOR_TEMP_ITEM->number.value = motor_temp;
			X_STATE_VIN_12V_ITEM->number.value = vin12v;
			indigo_update_property(device, X_STATE_PROPERTY, NULL);
		}
	}
	indigo_execute_handler_in(device, 10, focuser_timer_callback);
	//- focuser.on_timer
}

static void focuser_connection_handler(indigo_device *device) {
	if (CONNECTION_CONNECTED_ITEM->sw.value) {
		bool connection_result = true;
		if (PRIVATE_DATA->count++ == 0) {
			connection_result = primaluce_open(device);
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
					indigo_send_message(device, ALERT_PROPERTY, "Error: %s", text);
				}
				if (get_number(device, GET_CALRESTART_MOT1)) {
					indigo_send_message(device, BUSY_PROPERTY, "Warning: %s needs calibration", INFO_DEVICE_MODEL_ITEM->text.value);
				}
				PRIVATE_DATA->has_abs_pos = getToken(device, 0, GET_MOT1_ABS_POS) != -1;
				FOCUSER_POSITION_ITEM->number.value = FOCUSER_POSITION_ITEM->number.target = get_number(device, PRIVATE_DATA->has_abs_pos ? GET_MOT1_ABS_POS : GET_MOT1_ABS_POS_STEP);
				if (getToken(device, 0, GET_MOT1_SPEED) == -1) {
					FOCUSER_SPEED_PROPERTY->hidden = true;
				} else {
					FOCUSER_SPEED_ITEM->number.value = FOCUSER_SPEED_ITEM->number.target = get_number(device, GET_MOT1_SPEED);
					FOCUSER_SPEED_PROPERTY->hidden = false;
				}
				FOCUSER_BACKLASH_ITEM->number.value = get_number(device, GET_MOT1_BKLASH);
				X_STATE_MOTOR_TEMP_ITEM->number.value = get_number(device, GET_MOT1_NTC_T);
				X_STATE_VIN_12V_ITEM->number.value = get_number(device, GET_VIN_12V);
				X_STATE_VIN_USB_ITEM->number.value = get_number(device, GET_VIN_USB);
				// TBD: STA doesn't work
				if ((text = get_string(device, GET_WIFIAP_STATUS))) {
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
			indigo_define_property(device, X_HOLD_CURR_PROPERTY, NULL);
			indigo_define_property(device, X_CALIBRATE_F_PROPERTY, NULL);
			indigo_execute_handler(device, focuser_timer_callback);
			CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
			indigo_send_message(device, OK_PROPERTY, "Connected to %s on %s", FOCUSER_DEVICE_NAME, DEVICE_PORT_ITEM->text.value);
		} else {
			indigo_send_message(device, ALERT_PROPERTY, "Failed to connect to %s on %s", FOCUSER_DEVICE_NAME, DEVICE_PORT_ITEM->text.value);
			PRIVATE_DATA->count--;
			CONNECTION_PROPERTY->state = INDIGO_ALERT_STATE;
			indigo_set_switch(CONNECTION_PROPERTY, CONNECTION_DISCONNECTED_ITEM, true);
		}
	} else {
		indigo_cancel_pending_handlers(device);
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
		indigo_delete_property(device, X_HOLD_CURR_PROPERTY, NULL);
		indigo_delete_property(device, X_CALIBRATE_F_PROPERTY, NULL);
		if (--PRIVATE_DATA->count == 0) {
			primaluce_close(device);
		}
		indigo_send_message(device, OK_PROPERTY, "Disconnected from %s", device->name);
		CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
	}
	indigo_focuser_change_property(device, NULL, CONNECTION_PROPERTY);
}

static void focuser_x_wifi_handler(indigo_device *device) {
	X_WIFI_PROPERTY->state = INDIGO_OK_STATE;
	//+ focuser.X_WIFI.on_change
	bool result = false;
	if (X_WIFI_OFF_ITEM->sw.value) {
		result = primaluce_command(device, "{\"req\":{\"cmd\":{\"AP_SET_STATUS\":\"off\"}}}");
	} else if (X_WIFI_AP_ITEM->sw.value) {
		result = primaluce_command(device, "{\"req\":{\"cmd\":{\"AP_SET_STATUS\":\"on\"}}}");
	} else if (X_WIFI_STA_ITEM->sw.value) {
		result = primaluce_command(device, "{\"req\":{\"cmd\":{\"STA_SET_STATUS\":\"on\"}}}");
	}
	if (!result) {
		X_WIFI_PROPERTY->state = INDIGO_ALERT_STATE;
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
	} else if (X_LEDS_ON_ITEM->sw.value) {
		result = primaluce_command(device, "{\"req\":{\"cmd\":{\"DIMLEDS\":\"on\"}}}");
	}
	if (!result) {
		X_LEDS_PROPERTY->state = INDIGO_ALERT_STATE;
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

static void focuser_x_hold_curr_handler(indigo_device *device) {
	X_HOLD_CURR_PROPERTY->state = INDIGO_OK_STATE;
	//+ focuser.X_HOLD_CURR.on_change
	bool result = false;
	if (X_HOLD_CURR_OFF_ITEM->sw.value) {
		result = primaluce_command(device, "{\"req\":{\"set\":{\"MOT1\":{\"HOLDCURR_STATUS\":0}}}}");
	} else if (X_HOLD_CURR_ON_ITEM->sw.value) {
		result = primaluce_command(device, "{\"req\":{\"set\":{\"MOT1\":{\"HOLDCURR_STATUS\":1}}}}");
	}
	if (!result) {
		X_HOLD_CURR_PROPERTY->state = INDIGO_ALERT_STATE;
	} else {
		char *state = get_string(device, SET_MOT1_HOLDCURR_STATUS);
		if (state == NULL || strcmp(state, "done")) {
			X_HOLD_CURR_PROPERTY->state = INDIGO_ALERT_STATE;
		}
	}
	//- focuser.X_HOLD_CURR.on_change
	indigo_update_property(device, X_HOLD_CURR_PROPERTY, NULL);
}

static void focuser_x_calibrate_f_handler(indigo_device *device) {
	X_CALIBRATE_F_PROPERTY->state = INDIGO_OK_STATE;
	//+ focuser.X_CALIBRATE_F.on_change
	bool result = true;
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
				FOCUSER_POSITION_ITEM->number.value = FOCUSER_POSITION_ITEM->number.target = get_number(device, PRIVATE_DATA->has_abs_pos ? GET_MOT1_ABS_POS : GET_MOT1_ABS_POS_STEP);
				indigo_update_property(device, FOCUSER_POSITION_PROPERTY, NULL);
			}
		}
	}
	if (!result) {
		X_CALIBRATE_F_PROPERTY->state = INDIGO_ALERT_STATE;
	}
	//- focuser.X_CALIBRATE_F.on_change
	indigo_update_property(device, X_CALIBRATE_F_PROPERTY, NULL);
}

static void focuser_backlash_handler(indigo_device *device) {
	FOCUSER_BACKLASH_PROPERTY->state = INDIGO_OK_STATE;
	//+ focuser.FOCUSER_BACKLASH.on_change
	if (!primaluce_command(device, "{\"req\":{\"set\":{\"MOT1\":{\"BKLASH\":%d}}}}", (int)FOCUSER_BACKLASH_ITEM->number.target)) {
		INDIGO_UPDATE_PROPERTY_STATE(FOCUSER_BACKLASH_PROPERTY, INDIGO_ALERT_STATE, NULL);
		return;
	}
	char *state = get_string(device, SET_MOT1_BKLASH);
	if (state == NULL || strcmp(state, "done")) {
		INDIGO_UPDATE_PROPERTY_STATE(FOCUSER_BACKLASH_PROPERTY, INDIGO_ALERT_STATE, NULL);
		return;
	}
	//- focuser.FOCUSER_BACKLASH.on_change
	indigo_update_property(device, FOCUSER_BACKLASH_PROPERTY, NULL);
}

static void focuser_position_handler(indigo_device *device) {
	//+ focuser.FOCUSER_POSITION.on_change
	if (!primaluce_command(device, PRIVATE_DATA->is_sestosenso_3 ? "{\"req\":{\"cmd\":{\"MOT1\":{\"GOTO\":%d}}}}" : "{\"req\":{\"cmd\":{\"MOT1\":{\"MOVE_ABS\":{\"STEP\":%d}}}}}", (int)FOCUSER_POSITION_ITEM->number.target)) {
		INDIGO_UPDATE_PROPERTY_STATE(FOCUSER_POSITION_PROPERTY, INDIGO_ALERT_STATE, NULL);
	} else {
		char *state = get_string(device, PRIVATE_DATA->is_sestosenso_3 ? CMD_MOT1_GOTO : CMD_MOT1_STEP);
		if (state == NULL || strcmp(state, "done")) {
			INDIGO_UPDATE_PROPERTY_STATE(FOCUSER_POSITION_PROPERTY, INDIGO_ALERT_STATE, NULL);
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
	if (FOCUSER_POSITION_ITEM->number.target < 0) {
		FOCUSER_POSITION_ITEM->number.target = 0;
	}
	focuser_position_handler(device);
	FOCUSER_POSITION_PROPERTY->state = FOCUSER_POSITION_PROPERTY->state;
	//- focuser.FOCUSER_STEPS.on_change
	indigo_update_property(device, FOCUSER_STEPS_PROPERTY, NULL);
}

static void focuser_speed_handler(indigo_device *device) {
	FOCUSER_SPEED_PROPERTY->state = INDIGO_OK_STATE;
	//+ focuser.FOCUSER_SPEED.on_change
	if (!primaluce_command(device, "{\"req\":{\"set\":{\"MOT1\":{\"SPEED\":%d}}}}", (int)FOCUSER_SPEED_ITEM->number.target)) {
		INDIGO_UPDATE_PROPERTY_STATE(FOCUSER_SPEED_PROPERTY, INDIGO_ALERT_STATE, NULL);
		return;
	}
	char *state = get_string(device, SET_MOT1_SPEED);
	if (state == NULL || strcmp(state, "done")) {
		INDIGO_UPDATE_PROPERTY_STATE(FOCUSER_SPEED_PROPERTY, INDIGO_ALERT_STATE, NULL);
		return;
	}
	//- focuser.FOCUSER_SPEED.on_change
	indigo_update_property(device, FOCUSER_SPEED_PROPERTY, NULL);
}

static void focuser_abort_motion_handler(indigo_device *device) {
	FOCUSER_ABORT_MOTION_PROPERTY->state = INDIGO_OK_STATE;
	//+ focuser.FOCUSER_ABORT_MOTION.on_change
	if (!primaluce_command(device, "{\"req\":{\"cmd\":{\"MOT1\":{\"MOT_STOP\":\"\"}}}}")) {
		INDIGO_UPDATE_PROPERTY_STATE(FOCUSER_ABORT_MOTION_PROPERTY, INDIGO_ALERT_STATE, NULL);
		return;
	}
	char *state = get_string(device, CMD_MOT1_MOT_STOP);
	if (state == NULL || strcmp(state, "done")) {
		INDIGO_UPDATE_PROPERTY_STATE(FOCUSER_ABORT_MOTION_PROPERTY, INDIGO_ALERT_STATE, NULL);
		return;
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
		X_WIFI_STA_PROPERTY->hidden = true;
		X_LEDS_PROPERTY = indigo_init_switch_property(NULL, device->name, X_LEDS_PROPERTY_NAME, FOCUSER_ADVANCED_GROUP, "LEDs", INDIGO_OK_STATE, INDIGO_RW_PERM, INDIGO_ONE_OF_MANY_RULE, 3);
		if (X_LEDS_PROPERTY == NULL) {
			return INDIGO_FAILED;
		}
		indigo_init_switch_item(X_LEDS_OFF_ITEM, X_LEDS_OFF_ITEM_NAME, "Off", true);
		indigo_init_switch_item(X_LEDS_DIM_ITEM, X_LEDS_DIM_ITEM_NAME, "Dim", false);
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
		INDIGO_DEFINE_MATCHING_PROPERTY(X_HOLD_CURR_PROPERTY);
		INDIGO_DEFINE_MATCHING_PROPERTY(X_CALIBRATE_F_PROPERTY);
	}
	return indigo_focuser_enumerate_properties(device, client, property);
}

static indigo_result focuser_change_property(indigo_device *device, indigo_client *client, indigo_property *property) {
	if (indigo_property_match_changeable(CONNECTION_PROPERTY, property)) {
		if (!indigo_ignore_connection_change(device, property)) {
			indigo_property_copy_values(CONNECTION_PROPERTY, property, false);
			INDIGO_UPDATE_PROPERTY_STATE(CONNECTION_PROPERTY, INDIGO_BUSY_STATE, NULL);
			indigo_execute_handler(device, focuser_connection_handler);
		}
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
	} else if (indigo_property_match_changeable(X_HOLD_CURR_PROPERTY, property)) {
		INDIGO_COPY_VALUES_PROCESS_CHANGE(X_HOLD_CURR_PROPERTY, focuser_x_hold_curr_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(X_CALIBRATE_F_PROPERTY, property)) {
		INDIGO_COPY_VALUES_PROCESS_CHANGE(X_CALIBRATE_F_PROPERTY, focuser_x_calibrate_f_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(FOCUSER_BACKLASH_PROPERTY, property)) {
		INDIGO_COPY_VALUES_PROCESS_CHANGE(FOCUSER_BACKLASH_PROPERTY, focuser_backlash_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(FOCUSER_POSITION_PROPERTY, property)) {
		INDIGO_COPY_VALUES_PROCESS_CHANGE(FOCUSER_POSITION_PROPERTY, focuser_position_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(FOCUSER_STEPS_PROPERTY, property)) {
		INDIGO_COPY_VALUES_PROCESS_CHANGE(FOCUSER_STEPS_PROPERTY, focuser_steps_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(FOCUSER_SPEED_PROPERTY, property)) {
		INDIGO_COPY_VALUES_PROCESS_CHANGE(FOCUSER_SPEED_PROPERTY, focuser_speed_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(FOCUSER_ABORT_MOTION_PROPERTY, property)) {
		INDIGO_COPY_VALUES_PROCESS_CHANGE(FOCUSER_ABORT_MOTION_PROPERTY, focuser_abort_motion_handler);
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
	indigo_release_property(X_HOLD_CURR_PROPERTY);
	indigo_release_property(X_CALIBRATE_F_PROPERTY);
	INDIGO_DEVICE_DETACH_LOG(DRIVER_NAME, device->name);
	return indigo_focuser_detach(device);
}

#pragma mark - High level code (rotator)

static void rotator_connection_handler(indigo_device *device) {
	if (CONNECTION_CONNECTED_ITEM->sw.value) {
		bool connection_result = true;
		if (PRIVATE_DATA->count++ == 0) {
			connection_result = primaluce_open(device->master_device);
		}
		if (connection_result) {
			//+ rotator.on_connect
			char *text;
			if (primaluce_command(device, "{\"req\":{\"set\": {\"ARCO\":1}}}")) {
				if (primaluce_command(device, "{\"req\":{\"get\": \"\"}}")) {
					if ((text = get_string(device, GET_MOT2_ERROR)) && *text) {
						indigo_send_message(device, ALERT_PROPERTY, "Error: %s", text);
					}
					if (get_number(device, GET_CALRESTART_MOT2)) {
						indigo_send_message(device, BUSY_PROPERTY, "Warning: ARCO needs calibration");
					}
				}
				PRIVATE_DATA->has_abs_pos = getToken(device, 0, GET_MOT2_ABS_POS) != -1;
				ROTATOR_POSITION_ITEM->number.value = ROTATOR_POSITION_ITEM->number.target = get_number(device, PRIVATE_DATA->has_abs_pos ? GET_MOT2_ABS_POS : GET_MOT2_ABS_POS_DEG);
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
			PRIVATE_DATA->count--;
			CONNECTION_PROPERTY->state = INDIGO_ALERT_STATE;
			indigo_set_switch(CONNECTION_PROPERTY, CONNECTION_DISCONNECTED_ITEM, true);
		}
	} else {
		indigo_cancel_pending_handlers(device);
		//+ rotator.on_disconnect
		primaluce_command(device, "{\"req\":{\"set\": {\"ARCO\":0}}}");
		//- rotator.on_disconnect
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
	//+ rotator.X_CALIBRATE_R.on_change
	if (X_CALIBRATE_R_START_ITEM->sw.value) {
		X_CALIBRATE_R_START_ITEM->sw.value = false;
		if (!primaluce_command(device, "{\"req\":{\"cmd\": {\"MOT2\": {\"CAL_STATUS\":\"exec\"}}}}")) {
			INDIGO_UPDATE_PROPERTY_STATE(X_CALIBRATE_R_PROPERTY, INDIGO_ALERT_STATE, NULL);
		} else {
			char *state = get_string(device, CMD_MOT2_CAL_STATUS);
			if (state == NULL || strcmp(state, "done")) {
				INDIGO_UPDATE_PROPERTY_STATE(X_CALIBRATE_R_PROPERTY, INDIGO_ALERT_STATE, NULL);
			} else {
				indigo_execute_handler(device, rotator_movement_finalizer);
			}
		}
	}
	//- rotator.X_CALIBRATE_R.on_change
}

static void rotator_position_handler(indigo_device *device) {
	//+ rotator.ROTATOR_POSITION.on_change
	if (!primaluce_command(device, "{\"req\":{\"cmd\":{\"MOT2\":{\"MOVE_ABS\":{\"DEG\":%g}}}}}", FOCUSER_POSITION_ITEM->number.target)) {
		ROTATOR_POSITION_PROPERTY->state = INDIGO_ALERT_STATE;
		indigo_update_property(device, X_CALIBRATE_R_PROPERTY, NULL);
	} else {
		char *state = get_string(device, CMD_MOT2_STEP);
		if (state == NULL || strcmp(state, "done")) {
			ROTATOR_POSITION_PROPERTY->state = INDIGO_ALERT_STATE;
			indigo_update_property(device, FOCUSER_POSITION_PROPERTY, NULL);
		} else {
			indigo_execute_handler(device, rotator_movement_finalizer);
		}
	}
	//- rotator.ROTATOR_POSITION.on_change
}

static void rotator_abort_motion_handler(indigo_device *device) {
	ROTATOR_ABORT_MOTION_PROPERTY->state = INDIGO_OK_STATE;
	//+ rotator.ROTATOR_ABORT_MOTION.on_change
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
		ROTATOR_ON_POSITION_SET_PROPERTY->hidden = true;
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
		if (!indigo_ignore_connection_change(device, property)) {
			indigo_property_copy_values(CONNECTION_PROPERTY, property, false);
			INDIGO_UPDATE_PROPERTY_STATE(CONNECTION_PROPERTY, INDIGO_BUSY_STATE, NULL);
			indigo_execute_handler(device, rotator_connection_handler);
		}
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(X_CALIBRATE_R_PROPERTY, property)) {
		INDIGO_COPY_VALUES_PROCESS_CHANGE(X_CALIBRATE_R_PROPERTY, rotator_x_calibrate_r_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(ROTATOR_POSITION_PROPERTY, property)) {
		INDIGO_COPY_VALUES_PROCESS_CHANGE(ROTATOR_POSITION_PROPERTY, rotator_position_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(ROTATOR_ABORT_MOTION_PROPERTY, property)) {
		INDIGO_COPY_VALUES_PROCESS_CHANGE(ROTATOR_ABORT_MOTION_PROPERTY, rotator_abort_motion_handler);
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
		case INDIGO_DRIVER_INIT:
			last_action = action;
			static indigo_device_match_pattern patterns[1] = { 0 };
			strcpy(patterns[0].product_string, "CP2102N");
			INDIGO_REGISER_MATCH_PATTERNS(focuser_template, patterns, 1);
			private_data = indigo_safe_malloc(sizeof(primaluce_private_data));
			focuser = indigo_safe_malloc_copy(sizeof(indigo_device), &focuser_template);
			focuser->private_data = private_data;
			indigo_attach_device(focuser);
			rotator = indigo_safe_malloc_copy(sizeof(indigo_device), &rotator_template);
			rotator->private_data = private_data;
			rotator->master_device = focuser;
			indigo_attach_device(rotator);
			break;

		case INDIGO_DRIVER_SHUTDOWN:
			VERIFY_NOT_CONNECTED(focuser);
			VERIFY_NOT_CONNECTED(rotator);
			last_action = action;
			if (focuser != NULL) {
				indigo_detach_device(focuser);
				free(focuser);
				focuser = NULL;
			}
			if (rotator != NULL) {
				indigo_detach_device(rotator);
				free(rotator);
				rotator = NULL;
			}
			if (private_data != NULL) {
				free(private_data);
				private_data = NULL;
			}
			break;

		case INDIGO_DRIVER_INFO:
			break;
	}

	return INDIGO_OK;
}
