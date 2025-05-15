// Copyright (c) 2023-2025 CloudMakers, s. r. o.
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
// 2.0 by Peter Polakovic <peter.polakovic@cloudmakers.eu>
// 3.0 refactoring by Peter Polakovic <peter.polakovic@cloudmakers.eu>

/** INDIGO PrimaluceLab focuser/rotator driver
 \file indigo_focuser_primaluce.c
 */

#define DRIVER_VERSION 0x03000007
#define DRIVER_NAME "indigo_focuser_primaluce"

#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <assert.h>
#include <errno.h>
#include <pthread.h>
#include <stdarg.h>

#include <indigo/indigo_driver_xml.h>
#include <indigo/indigo_uni_io.h>

#include "indigo_focuser_primaluce.h"

#define JSMN_STRICT
#define JSMN_PARENT_LINKS
#include "jsmn.h"

#define PRIVATE_DATA													((primaluce_private_data *)device->private_data)

#define X_CONFIG_PROPERTY											(PRIVATE_DATA->config_property)
#define X_CONFIG_M1ACC_ITEM										(X_CONFIG_PROPERTY->items+0)
#define X_CONFIG_M1SPD_ITEM										(X_CONFIG_PROPERTY->items+1)
#define X_CONFIG_M1DEC_ITEM										(X_CONFIG_PROPERTY->items+2)
#define X_CONFIG_M1CACC_ITEM									(X_CONFIG_PROPERTY->items+3)
#define X_CONFIG_M1CSPD_ITEM									(X_CONFIG_PROPERTY->items+4)
#define X_CONFIG_M1CDEC_ITEM									(X_CONFIG_PROPERTY->items+5)
#define X_CONFIG_M1HOLD_ITEM									(X_CONFIG_PROPERTY->items+6)

#define X_STATE_PROPERTY											(PRIVATE_DATA->state_property)
#define X_STATE_MOTOR_TEMP_ITEM								(X_STATE_PROPERTY->items+0)
#define X_STATE_VIN_12V_ITEM									(X_STATE_PROPERTY->items+1)
#define X_STATE_VIN_USB_ITEM									(X_STATE_PROPERTY->items+2)

#define X_LEDS_PROPERTY												(PRIVATE_DATA->dim_leds_property)
#define X_LEDS_OFF_ITEM												(X_LEDS_PROPERTY->items+0)
#define X_LEDS_DIM_ITEM												(X_LEDS_PROPERTY->items+1)
#define X_LEDS_ON_ITEM												(X_LEDS_PROPERTY->items+2)

#define X_WIFI_PROPERTY												(PRIVATE_DATA->wifi_property)
#define X_WIFI_OFF_ITEM												(X_WIFI_PROPERTY->items+0)
#define X_WIFI_AP_ITEM												(X_WIFI_PROPERTY->items+1)
#define X_WIFI_STA_ITEM												(X_WIFI_PROPERTY->items+2)

#define X_WIFI_AP_PROPERTY										(PRIVATE_DATA->wifi_ap_property)
#define X_WIFI_AP_SSID_ITEM										(X_WIFI_AP_PROPERTY->items+0)
#define X_WIFI_AP_PASSWORD_ITEM								(X_WIFI_AP_PROPERTY->items+1)

#define X_WIFI_STA_PROPERTY										(PRIVATE_DATA->wifi_sta_property)
#define X_WIFI_STA_SSID_ITEM									(X_WIFI_STA_PROPERTY->items+0)
#define X_WIFI_STA_PASSWORD_ITEM							(X_WIFI_STA_PROPERTY->items+1)

#define X_RUNPRESET_L_PROPERTY								(PRIVATE_DATA->runpreset_l_property)
#define X_RUNPRESET_L_M1ACC_ITEM							(X_RUNPRESET_L_PROPERTY->items+0)
#define X_RUNPRESET_L_M1SPD_ITEM							(X_RUNPRESET_L_PROPERTY->items+1)
#define X_RUNPRESET_L_M1DEC_ITEM							(X_RUNPRESET_L_PROPERTY->items+2)
#define X_RUNPRESET_L_M1CACC_ITEM							(X_RUNPRESET_L_PROPERTY->items+3)
#define X_RUNPRESET_L_M1CSPD_ITEM							(X_RUNPRESET_L_PROPERTY->items+4)
#define X_RUNPRESET_L_M1CDEC_ITEM							(X_RUNPRESET_L_PROPERTY->items+5)
#define X_RUNPRESET_L_M1HOLD_ITEM							(X_RUNPRESET_L_PROPERTY->items+6)

#define X_RUNPRESET_M_PROPERTY								(PRIVATE_DATA->runpreset_m_property)
#define X_RUNPRESET_M_M1ACC_ITEM							(X_RUNPRESET_M_PROPERTY->items+0)
#define X_RUNPRESET_M_M1SPD_ITEM							(X_RUNPRESET_M_PROPERTY->items+1)
#define X_RUNPRESET_M_M1DEC_ITEM							(X_RUNPRESET_M_PROPERTY->items+2)
#define X_RUNPRESET_M_M1CACC_ITEM							(X_RUNPRESET_M_PROPERTY->items+3)
#define X_RUNPRESET_M_M1CSPD_ITEM							(X_RUNPRESET_M_PROPERTY->items+4)
#define X_RUNPRESET_M_M1CDEC_ITEM							(X_RUNPRESET_M_PROPERTY->items+5)
#define X_RUNPRESET_M_M1HOLD_ITEM							(X_RUNPRESET_M_PROPERTY->items+6)

#define X_RUNPRESET_S_PROPERTY								(PRIVATE_DATA->runpreset_s_property)
#define X_RUNPRESET_S_M1ACC_ITEM							(X_RUNPRESET_S_PROPERTY->items+0)
#define X_RUNPRESET_S_M1SPD_ITEM							(X_RUNPRESET_S_PROPERTY->items+1)
#define X_RUNPRESET_S_M1DEC_ITEM							(X_RUNPRESET_S_PROPERTY->items+2)
#define X_RUNPRESET_S_M1CACC_ITEM							(X_RUNPRESET_S_PROPERTY->items+3)
#define X_RUNPRESET_S_M1CSPD_ITEM							(X_RUNPRESET_S_PROPERTY->items+4)
#define X_RUNPRESET_S_M1CDEC_ITEM							(X_RUNPRESET_S_PROPERTY->items+5)
#define X_RUNPRESET_S_M1HOLD_ITEM							(X_RUNPRESET_S_PROPERTY->items+6)

#define X_RUNPRESET_1_PROPERTY								(PRIVATE_DATA->runpreset_1_property)
#define X_RUNPRESET_1_M1ACC_ITEM							(X_RUNPRESET_1_PROPERTY->items+0)
#define X_RUNPRESET_1_M1SPD_ITEM							(X_RUNPRESET_1_PROPERTY->items+1)
#define X_RUNPRESET_1_M1DEC_ITEM							(X_RUNPRESET_1_PROPERTY->items+2)
#define X_RUNPRESET_1_M1CACC_ITEM							(X_RUNPRESET_1_PROPERTY->items+3)
#define X_RUNPRESET_1_M1CSPD_ITEM							(X_RUNPRESET_1_PROPERTY->items+4)
#define X_RUNPRESET_1_M1CDEC_ITEM							(X_RUNPRESET_1_PROPERTY->items+5)
#define X_RUNPRESET_1_M1HOLD_ITEM							(X_RUNPRESET_1_PROPERTY->items+6)

#define X_RUNPRESET_2_PROPERTY								(PRIVATE_DATA->runpreset_2_property)
#define X_RUNPRESET_2_M1ACC_ITEM							(X_RUNPRESET_2_PROPERTY->items+0)
#define X_RUNPRESET_2_M1SPD_ITEM							(X_RUNPRESET_2_PROPERTY->items+1)
#define X_RUNPRESET_2_M1DEC_ITEM							(X_RUNPRESET_2_PROPERTY->items+2)
#define X_RUNPRESET_2_M1CACC_ITEM							(X_RUNPRESET_2_PROPERTY->items+3)
#define X_RUNPRESET_2_M1CSPD_ITEM							(X_RUNPRESET_2_PROPERTY->items+4)
#define X_RUNPRESET_2_M1CDEC_ITEM							(X_RUNPRESET_2_PROPERTY->items+5)
#define X_RUNPRESET_2_M1HOLD_ITEM							(X_RUNPRESET_2_PROPERTY->items+6)

#define X_RUNPRESET_3_PROPERTY								(PRIVATE_DATA->runpreset_3_property)
#define X_RUNPRESET_3_M1ACC_ITEM							(X_RUNPRESET_3_PROPERTY->items+0)
#define X_RUNPRESET_3_M1SPD_ITEM							(X_RUNPRESET_3_PROPERTY->items+1)
#define X_RUNPRESET_3_M1DEC_ITEM							(X_RUNPRESET_3_PROPERTY->items+2)
#define X_RUNPRESET_3_M1CACC_ITEM							(X_RUNPRESET_3_PROPERTY->items+3)
#define X_RUNPRESET_3_M1CSPD_ITEM							(X_RUNPRESET_3_PROPERTY->items+4)
#define X_RUNPRESET_3_M1CDEC_ITEM							(X_RUNPRESET_3_PROPERTY->items+5)
#define X_RUNPRESET_3_M1HOLD_ITEM							(X_RUNPRESET_3_PROPERTY->items+6)

#define X_RUNPRESET_PROPERTY									(PRIVATE_DATA->runpreset_property)
#define X_RUNPRESET_L_ITEM										(X_RUNPRESET_PROPERTY->items+0)
#define X_RUNPRESET_M_ITEM										(X_RUNPRESET_PROPERTY->items+1)
#define X_RUNPRESET_S_ITEM										(X_RUNPRESET_PROPERTY->items+2)
#define X_RUNPRESET_1_ITEM										(X_RUNPRESET_PROPERTY->items+3)
#define X_RUNPRESET_2_ITEM										(X_RUNPRESET_PROPERTY->items+4)
#define X_RUNPRESET_3_ITEM										(X_RUNPRESET_PROPERTY->items+5)

#define X_HOLD_CURR_PROPERTY									(PRIVATE_DATA->hold_curr_property)
#define X_HOLD_CURR_OFF_ITEM									(X_HOLD_CURR_PROPERTY->items+0)
#define X_HOLD_CURR_ON_ITEM										(X_HOLD_CURR_PROPERTY->items+1)

#define X_CALIBRATE_F_PROPERTY								(PRIVATE_DATA->calibrate_f_property)
#define X_CALIBRATE_F_START_ITEM							(X_CALIBRATE_F_PROPERTY->items+0)
#define X_CALIBRATE_F_START_INVERTED_ITEM			(X_CALIBRATE_F_PROPERTY->items+1)
#define X_CALIBRATE_F_END_ITEM								(X_CALIBRATE_F_PROPERTY->items+2)

#define X_CALIBRATE_R_PROPERTY								(PRIVATE_DATA->calibrate_r_property)
#define X_CALIBRATE_R_START_ITEM							(X_CALIBRATE_R_PROPERTY->items+0)

typedef struct {
	indigo_uni_handle *handle;
	int device_count;
	indigo_timer *timer;
	pthread_mutex_t mutex;
	jsmn_parser parser;
	bool has_abs_pos;
	indigo_property *state_property;
	indigo_property *dim_leds_property;
	indigo_property *wifi_property;
	indigo_property *wifi_ap_property;
	indigo_property *wifi_sta_property;
	indigo_property *config_property;
	indigo_property *runpreset_l_property;
	indigo_property *runpreset_m_property;
	indigo_property *runpreset_s_property;
	indigo_property *runpreset_1_property;
	indigo_property *runpreset_2_property;
	indigo_property *runpreset_3_property;
	indigo_property *runpreset_property;
	indigo_property *hold_curr_property;
	indigo_property *calibrate_f_property;
	indigo_property *calibrate_r_property;
} primaluce_private_data;

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

// -------------------------------------------------------------------------------- Low level communication routines

static bool primaluce_command(indigo_device *device, char *command, char *response, int size, jsmntok_t *tokens, int count) {
	long result;
	pthread_mutex_lock(&PRIVATE_DATA->mutex);
	if (!indigo_uni_write(PRIVATE_DATA->handle, command, strlen(command))) {
		pthread_mutex_unlock(&PRIVATE_DATA->mutex);
		return false;
	}
	while (true) {
		result = indigo_uni_read_line(PRIVATE_DATA->handle, response, size);
		if (result < 1) {
			INDIGO_DRIVER_ERROR(DRIVER_NAME, "Failed to read from %s -> %s (%d)", DEVICE_PORT_ITEM->text.value, strerror(errno), errno);
			pthread_mutex_unlock(&PRIVATE_DATA->mutex);
			return false;
		}
		if (*response == '[') {
			INDIGO_DRIVER_DEBUG(DRIVER_NAME, "Ignored '%s' -> '%s'", command, response);
			continue;
		}
		break;
	}
	memset(tokens, 0, count * sizeof(jsmntok_t));
	jsmn_init(&PRIVATE_DATA->parser);
	if (*response == '"' || jsmn_parse(&PRIVATE_DATA->parser, response, size, tokens, count) <= 0) {
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "Failed to parse '%s' -> '%s'", command, response);
		pthread_mutex_unlock(&PRIVATE_DATA->mutex);
		return false;
	}
	INDIGO_DRIVER_DEBUG(DRIVER_NAME, "Parsed '%s' -> '%s'", command, response);
	for (int i = 0; i < count; i++) {
		if (tokens[i].type == JSMN_UNDEFINED) {
			break;
		}
		if (tokens[i].type == JSMN_STRING) {
			response[tokens[i].end] = 0;
		}
	}
	pthread_mutex_unlock(&PRIVATE_DATA->mutex);
	return true;
}

static int getToken(char *response, jsmntok_t *tokens, int start, char *path[]) {
	if (tokens[start].type != JSMN_OBJECT) {
		return -1;
	}
	int count = tokens[start].size;
	int index = start + 1;
	char *name = *path;
	for (int i = 0; i < count; i++) {
		if (tokens[index].type != JSMN_STRING) {
			return -1;
		}
		char *n = response + tokens[index].start;
		int l = tokens[index].end - tokens[index].start;
		if (!strncmp(n, name, l)) {
			index++;
			if (*++path == NULL) {
				return index;
			}
			return getToken(response, tokens, index, path);
		} else {
			while (true) {
				index++;
				if (tokens[index].type == JSMN_UNDEFINED) {
					return -1;
				}
				if (tokens[index].parent == start) {
					break;
				}
			}
		}
	}
	return -1;
}

static char *get_string(char *response, jsmntok_t *tokens, char *path[]) {
	int index = getToken(response, tokens, 0, path);
	if (index == -1 || tokens[index].type != JSMN_STRING) {
		return NULL;
	}
	return response + tokens[index].start;
}

static double get_number2(char *response, jsmntok_t *tokens, char *path[], char *alt_path[]) {
	int index = getToken(response, tokens, 0, path);
	if (index == -1) {
		if (alt_path != NULL) {
			index = getToken(response, tokens, 0, alt_path);
			if (index == -1) {
				return 0;
			}
		} else {
			return 0;
		}
	}
	if  (tokens[index].type == JSMN_PRIMITIVE || tokens[index].type == JSMN_STRING) {
		return atof(response + tokens[index].start);
	}
	return 0;
}

static double get_number(char *response, jsmntok_t *tokens, char *path[]) {
	return get_number2(response, tokens, path, NULL);
}

static bool primaluce_open(indigo_device *device) {
	char *name = DEVICE_PORT_ITEM->text.value;
	PRIVATE_DATA->handle = indigo_uni_open_serial_with_speed(name, 115200, INDIGO_LOG_DEBUG);
	if (PRIVATE_DATA->handle != NULL) {
		INDIGO_DRIVER_LOG(DRIVER_NAME, "Connected to %s", name);
		char response[1024];
		jsmntok_t tokens[128];
		char *text;
		if (primaluce_command(device, "{\"req\":{\"get\":{\"MODNAME\":\"\"}}}", response, sizeof(response), tokens, 128) && (text = get_string(response, tokens, GET_MODNAME))) {
			if (!strncmp(text, "SESTOSENSO", 10) || !strncmp(text, "ESATTO", 6)) {
				if (primaluce_command(device, "{\"req\":{\"get\":{\"SWVERS\":{\"SWAPP\":\"\"}}}}", response, sizeof(response), tokens, 128) && (text = get_string(response, tokens, GET_SWAPP))) {
					double version = atof(text);
					if (version < 3.05) {
						indigo_send_message(device, "WARNING: %s has firmware version %.2f and at least 3.05 is needed", INFO_DEVICE_MODEL_ITEM->text.value, version);
					}
					//primaluce_command(device, "{\"req\":{\"cmd\":{\"LOGLEVEL\":\"no output\"}}}", response, sizeof(response), tokens, 128);
					return true;
				} else {
					INDIGO_DRIVER_ERROR(DRIVER_NAME, "Unsupported version");
				}
			} else {
				INDIGO_DRIVER_ERROR(DRIVER_NAME, "Unsupported device");
			}
		} else {
			indigo_send_message(device, "Handshake failed");
		}
		indigo_uni_close(&PRIVATE_DATA->handle);
	} else {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "Failed to connect to %s", name);
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

// -------------------------------------------------------------------------------- INDIGO focuser device implementation

static indigo_result focuser_enumerate_properties(indigo_device *device, indigo_client *client, indigo_property *property);

static indigo_result focuser_attach(indigo_device *device) {
	assert(device != NULL);
	assert(PRIVATE_DATA != NULL);
	if (indigo_focuser_attach(device, DRIVER_NAME, DRIVER_VERSION) == INDIGO_OK) {
		// -------------------------------------------------------------------------------- X_CONFIG
		X_CONFIG_PROPERTY = indigo_init_number_property(NULL, device->name, "X_CONFIG", FOCUSER_ADVANCED_GROUP, "Configuration", INDIGO_OK_STATE, INDIGO_RO_PERM, 7);
		if (X_CONFIG_PROPERTY == NULL) {
			return INDIGO_FAILED;
		}
		indigo_init_number_item(X_CONFIG_M1ACC_ITEM, "M1ACC", "Acceleration speed", 0, 10, 0, 0);
		indigo_init_number_item(X_CONFIG_M1SPD_ITEM, "M1SPD", "Run speed", 0, 10, 0, 0);
		indigo_init_number_item(X_CONFIG_M1DEC_ITEM, "M1DEC", "Deceleration speed", 0, 10, 0, 0);
		indigo_init_number_item(X_CONFIG_M1CACC_ITEM, "M1CACC", "Acceleration current", 0, 10, 0, 0);
		indigo_init_number_item(X_CONFIG_M1CSPD_ITEM, "M1CSPD", "Run current", 0, 10, 0, 0);
		indigo_init_number_item(X_CONFIG_M1CDEC_ITEM, "M1CDEC", "Deceleration current", 0, 10, 0, 0);
		indigo_init_number_item(X_CONFIG_M1HOLD_ITEM, "M1HOLD", "Hold current", 0, 10, 0, 0);
		// -------------------------------------------------------------------------------- X_STATE
		X_STATE_PROPERTY = indigo_init_number_property(NULL, device->name, "X_STATE", FOCUSER_ADVANCED_GROUP, "State", INDIGO_OK_STATE, INDIGO_RO_PERM, 3);
		if (X_STATE_PROPERTY == NULL) {
			return INDIGO_FAILED;
		}
		indigo_init_number_item(X_STATE_MOTOR_TEMP_ITEM, "MOTOR_TEMP", "Motor emperature (°C)", -50, 100, 0, 0);
		indigo_init_number_item(X_STATE_VIN_12V_ITEM, "VIN_12V", "12V power (V)", 0, 50, 0, 0);
		indigo_init_number_item(X_STATE_VIN_USB_ITEM, "VIN_USB", "USB power (V)", 0, 10, 0, 0);
		// -------------------------------------------------------------------------------- X_WIFI
		X_WIFI_PROPERTY = indigo_init_switch_property(NULL, device->name, "X_WIFI", FOCUSER_ADVANCED_GROUP, "WiFi mode", INDIGO_OK_STATE, INDIGO_RW_PERM, INDIGO_ONE_OF_MANY_RULE, 3);
		if (X_WIFI_PROPERTY == NULL) {
			return INDIGO_FAILED;
		}
		indigo_init_switch_item(X_WIFI_OFF_ITEM, "OFF", "Off", true);
		indigo_init_switch_item(X_WIFI_AP_ITEM, "AP", "Access Point mode", false);
		indigo_init_switch_item(X_WIFI_STA_ITEM, "STA", "Station mode", false);
		// TBD: STA doesn't work
		X_WIFI_PROPERTY->count = 2;
		// ---------------------------------------------------------------------------- X_WIFI_AP
		X_WIFI_AP_PROPERTY = indigo_init_text_property(NULL, device->name, "X_WIFI_AP", FOCUSER_ADVANCED_GROUP, "AP WiFi settings", INDIGO_OK_STATE, INDIGO_RW_PERM, 2);
		if (X_WIFI_AP_PROPERTY == NULL) {
			return INDIGO_FAILED;
		}
		indigo_init_text_item(X_WIFI_AP_SSID_ITEM, "AP_SSID", "SSID", "");
		indigo_init_text_item(X_WIFI_AP_PASSWORD_ITEM, "AP_PASSWORD", "Password", "");
		// ---------------------------------------------------------------------------- X_WIFI_STA
		X_WIFI_STA_PROPERTY = indigo_init_text_property(NULL, device->name, "X_WIFI_STA", FOCUSER_ADVANCED_GROUP, "STA WiFi settings", INDIGO_OK_STATE, INDIGO_RW_PERM, 2);
		if (X_WIFI_STA_PROPERTY == NULL) {
			return INDIGO_FAILED;
		}
		indigo_init_text_item(X_WIFI_STA_SSID_ITEM, "STA_SSID", "SSID", "");
		indigo_init_text_item(X_WIFI_STA_PASSWORD_ITEM, "STA_PASSWORD", "Password", "");
		// TBD: STA doesn't work
		X_WIFI_STA_PROPERTY->hidden = true;
		// -------------------------------------------------------------------------------- X_LEDS
		X_LEDS_PROPERTY = indigo_init_switch_property(NULL, device->name, "X_LEDS", FOCUSER_ADVANCED_GROUP, "LEDs", INDIGO_OK_STATE, INDIGO_RW_PERM, INDIGO_ONE_OF_MANY_RULE, 3);
		if (X_LEDS_PROPERTY == NULL) {
			return INDIGO_FAILED;
		}
		indigo_init_switch_item(X_LEDS_OFF_ITEM, "OFF", "Off", true);
		indigo_init_switch_item(X_LEDS_DIM_ITEM, "DIM", "Dim", false);
		indigo_init_switch_item(X_LEDS_ON_ITEM, "ON", "On", false);
		// -------------------------------------------------------------------------------- X_RUNPRESET_L
		X_RUNPRESET_L_PROPERTY = indigo_init_number_property(NULL, device->name, "X_RUNPRESET_L", FOCUSER_ADVANCED_GROUP, "Preset light", INDIGO_OK_STATE, INDIGO_RO_PERM, 7);
		if (X_RUNPRESET_L_PROPERTY == NULL) {
			return INDIGO_FAILED;
		}
		indigo_init_number_item(X_RUNPRESET_L_M1ACC_ITEM, "M1ACC", "Acceleration speed", 0, 10, 0, 0);
		indigo_init_number_item(X_RUNPRESET_L_M1SPD_ITEM, "M1SPD", "Run speed", 0, 10, 0, 0);
		indigo_init_number_item(X_RUNPRESET_L_M1DEC_ITEM, "M1DEC", "Deceleration speed", 0, 10, 0, 0);
		indigo_init_number_item(X_RUNPRESET_L_M1CACC_ITEM, "M1CACC", "Acceleration current", 0, 10, 0, 0);
		indigo_init_number_item(X_RUNPRESET_L_M1CSPD_ITEM, "M1CSPD", "Run current", 0, 10, 0, 0);
		indigo_init_number_item(X_RUNPRESET_L_M1CDEC_ITEM, "M1CDEC", "Deceleration current", 0, 10, 0, 0);
		indigo_init_number_item(X_RUNPRESET_L_M1HOLD_ITEM, "M1HOLD", "Hold current", 0, 10, 0, 0);
		// -------------------------------------------------------------------------------- X_RUNPRESET_M
		X_RUNPRESET_M_PROPERTY = indigo_init_number_property(NULL, device->name, "X_RUNPRESET_M", FOCUSER_ADVANCED_GROUP, "Preset medium", INDIGO_OK_STATE, INDIGO_RO_PERM, 7);
		if (X_RUNPRESET_M_PROPERTY == NULL) {
			return INDIGO_FAILED;
		}
		indigo_init_number_item(X_RUNPRESET_M_M1ACC_ITEM, "M1ACC", "Acceleration speed", 0, 10, 0, 0);
		indigo_init_number_item(X_RUNPRESET_M_M1SPD_ITEM, "M1SPD", "Run speed", 0, 10, 0, 0);
		indigo_init_number_item(X_RUNPRESET_M_M1DEC_ITEM, "M1DEC", "Deceleration speed", 0, 10, 0, 0);
		indigo_init_number_item(X_RUNPRESET_M_M1CACC_ITEM, "M1CACC", "Acceleration current", 0, 10, 0, 0);
		indigo_init_number_item(X_RUNPRESET_M_M1CSPD_ITEM, "M1CSPD", "Run current", 0, 10, 0, 0);
		indigo_init_number_item(X_RUNPRESET_M_M1CDEC_ITEM, "M1CDEC", "Deceleration current", 0, 10, 0, 0);
		indigo_init_number_item(X_RUNPRESET_M_M1HOLD_ITEM, "M1HOLD", "Hold current", 0, 10, 0, 0);
		// -------------------------------------------------------------------------------- X_RUNPRESET_S
		X_RUNPRESET_S_PROPERTY = indigo_init_number_property(NULL, device->name, "X_RUNPRESET_S", FOCUSER_ADVANCED_GROUP, "Preset slow", INDIGO_OK_STATE, INDIGO_RO_PERM, 7);
		if (X_RUNPRESET_S_PROPERTY == NULL) {
			return INDIGO_FAILED;
		}
		indigo_init_number_item(X_RUNPRESET_S_M1ACC_ITEM, "M1ACC", "Acceleration speed", 0, 10, 0, 0);
		indigo_init_number_item(X_RUNPRESET_S_M1SPD_ITEM, "M1SPD", "Run speed", 0, 10, 0, 0);
		indigo_init_number_item(X_RUNPRESET_S_M1DEC_ITEM, "M1DEC", "Deceleration speed", 0, 10, 0, 0);
		indigo_init_number_item(X_RUNPRESET_S_M1CACC_ITEM, "M1CACC", "Acceleration current", 0, 10, 0, 0);
		indigo_init_number_item(X_RUNPRESET_S_M1CSPD_ITEM, "M1CSPD", "Run current", 0, 10, 0, 0);
		indigo_init_number_item(X_RUNPRESET_S_M1CDEC_ITEM, "M1CDEC", "Deceleration current", 0, 10, 0, 0);
		indigo_init_number_item(X_RUNPRESET_S_M1HOLD_ITEM, "M1HOLD", "Hold current", 0, 10, 0, 0);
		// -------------------------------------------------------------------------------- X_RUNPRESET_1
		X_RUNPRESET_1_PROPERTY = indigo_init_number_property(NULL, device->name, "X_RUNPRESET_1", FOCUSER_ADVANCED_GROUP, "Preset #1", INDIGO_OK_STATE, INDIGO_RW_PERM, 7);
		if (X_RUNPRESET_1_PROPERTY == NULL) {
			return INDIGO_FAILED;
		}
		indigo_init_number_item(X_RUNPRESET_1_M1ACC_ITEM, "M1ACC", "Acceleration speed", 0, 10, 0, 0);
		indigo_init_number_item(X_RUNPRESET_1_M1SPD_ITEM, "M1SPD", "Run speed", 0, 10, 0, 0);
		indigo_init_number_item(X_RUNPRESET_1_M1DEC_ITEM, "M1DEC", "Deceleration speed", 0, 10, 0, 0);
		indigo_init_number_item(X_RUNPRESET_1_M1CACC_ITEM, "M1CACC", "Acceleration current", 0, 10, 0, 0);
		indigo_init_number_item(X_RUNPRESET_1_M1CSPD_ITEM, "M1CSPD", "Run current", 0, 10, 0, 0);
		indigo_init_number_item(X_RUNPRESET_1_M1CDEC_ITEM, "M1CDEC", "Deceleration current", 0, 10, 0, 0);
		indigo_init_number_item(X_RUNPRESET_1_M1HOLD_ITEM, "M1HOLD", "Hold current", 0, 10, 0, 0);
		// -------------------------------------------------------------------------------- X_RUNPRESET_2
		X_RUNPRESET_2_PROPERTY = indigo_init_number_property(NULL, device->name, "X_RUNPRESET_2", FOCUSER_ADVANCED_GROUP, "Preset #2", INDIGO_OK_STATE, INDIGO_RW_PERM, 7);
		if (X_RUNPRESET_2_PROPERTY == NULL) {
			return INDIGO_FAILED;
		}
		indigo_init_number_item(X_RUNPRESET_2_M1ACC_ITEM, "M1ACC", "Acceleration speed", 0, 10, 0, 0);
		indigo_init_number_item(X_RUNPRESET_2_M1SPD_ITEM, "M1SPD", "Run speed", 0, 10, 0, 0);
		indigo_init_number_item(X_RUNPRESET_2_M1DEC_ITEM, "M1DEC", "Deceleration speed", 0, 10, 0, 0);
		indigo_init_number_item(X_RUNPRESET_2_M1CACC_ITEM, "M1CACC", "Acceleration current", 0, 10, 0, 0);
		indigo_init_number_item(X_RUNPRESET_2_M1CSPD_ITEM, "M1CSPD", "Run current", 0, 10, 0, 0);
		indigo_init_number_item(X_RUNPRESET_2_M1CDEC_ITEM, "M1CDEC", "Deceleration current", 0, 10, 0, 0);
		indigo_init_number_item(X_RUNPRESET_2_M1HOLD_ITEM, "M1HOLD", "Hold current", 0, 10, 0, 0);
		// -------------------------------------------------------------------------------- X_RUNPRESET_3
		X_RUNPRESET_3_PROPERTY = indigo_init_number_property(NULL, device->name, "X_RUNPRESET_3", FOCUSER_ADVANCED_GROUP, "Preset #3", INDIGO_OK_STATE, INDIGO_RW_PERM, 7);
		if (X_RUNPRESET_3_PROPERTY == NULL) {
			return INDIGO_FAILED;
		}
		indigo_init_number_item(X_RUNPRESET_3_M1ACC_ITEM, "M1ACC", "Acceleration speed", 0, 10, 0, 0);
		indigo_init_number_item(X_RUNPRESET_3_M1SPD_ITEM, "M1SPD", "Run speed", 0, 10, 0, 0);
		indigo_init_number_item(X_RUNPRESET_3_M1DEC_ITEM, "M1DEC", "Deceleration speed", 0, 10, 0, 0);
		indigo_init_number_item(X_RUNPRESET_3_M1CACC_ITEM, "M1CACC", "Acceleration current", 0, 10, 0, 0);
		indigo_init_number_item(X_RUNPRESET_3_M1CSPD_ITEM, "M1CSPD", "Run current", 0, 10, 0, 0);
		indigo_init_number_item(X_RUNPRESET_3_M1CDEC_ITEM, "M1CDEC", "Deceleration current", 0, 10, 0, 0);
		indigo_init_number_item(X_RUNPRESET_3_M1HOLD_ITEM, "M1HOLD", "Hold current", 0, 10, 0, 0);
		// -------------------------------------------------------------------------------- X_RUNPRESET
		X_RUNPRESET_PROPERTY = indigo_init_switch_property(NULL, device->name, "X_RUNPRESET", FOCUSER_ADVANCED_GROUP, "Presets", INDIGO_OK_STATE, INDIGO_RW_PERM, INDIGO_ONE_OF_MANY_RULE, 6);
		if (X_RUNPRESET_PROPERTY == NULL) {
			return INDIGO_FAILED;
		}
		indigo_init_switch_item(X_RUNPRESET_L_ITEM, "L", "Preset light", false);
		indigo_init_switch_item(X_RUNPRESET_M_ITEM, "M", "Preset medium", false);
		indigo_init_switch_item(X_RUNPRESET_S_ITEM, "S", "Preset slow", false);
		indigo_init_switch_item(X_RUNPRESET_1_ITEM, "1", "Preset #1", false);
		indigo_init_switch_item(X_RUNPRESET_2_ITEM, "2", "Preset #2", false);
		indigo_init_switch_item(X_RUNPRESET_3_ITEM, "3", "Preset #3", false);
		// -------------------------------------------------------------------------------- X_HOLD_CURR
		X_HOLD_CURR_PROPERTY = indigo_init_switch_property(NULL, device->name, "X_HOLD_CURR", FOCUSER_ADVANCED_GROUP, "Hold current", INDIGO_OK_STATE, INDIGO_RW_PERM, INDIGO_ONE_OF_MANY_RULE, 2);
		if (X_HOLD_CURR_PROPERTY == NULL) {
			return INDIGO_FAILED;
		}
		indigo_init_switch_item(X_HOLD_CURR_OFF_ITEM, "OFF", "Off", true);
		indigo_init_switch_item(X_HOLD_CURR_ON_ITEM, "ON", "On", false);
		// -------------------------------------------------------------------------------- X_CALIBRATE
		X_CALIBRATE_F_PROPERTY = indigo_init_switch_property(NULL, device->name, "X_CALIBRATE", FOCUSER_ADVANCED_GROUP, "Calibrate focuser", INDIGO_OK_STATE, INDIGO_RW_PERM, INDIGO_ONE_OF_MANY_RULE, 3);
		if (X_CALIBRATE_F_PROPERTY == NULL) {
			return INDIGO_FAILED;
		}
		indigo_init_switch_item(X_CALIBRATE_F_START_ITEM, "START", "Start", false);
		indigo_init_switch_item(X_CALIBRATE_F_START_INVERTED_ITEM, "START_INVERTED", "Start inverted", false);
		indigo_init_switch_item(X_CALIBRATE_F_END_ITEM, "END", "End", false);
		// -------------------------------------------------------------------------------- DEVICE_PORT, DEVICE_PORTS
		DEVICE_PORT_PROPERTY->hidden = false;
		DEVICE_PORTS_PROPERTY->hidden = false;
		indigo_enumerate_serial_ports(device, DEVICE_PORTS_PROPERTY);
		FOCUSER_POSITION_ITEM->number.min = 0;
		FOCUSER_POSITION_ITEM->number.max = 1000000;
		strcpy(FOCUSER_POSITION_ITEM->number.format, "%.0f");
		FOCUSER_STEPS_ITEM->number.min = 0;
		FOCUSER_STEPS_ITEM->number.max = 1000000;
		strcpy(FOCUSER_STEPS_ITEM->number.format, "%.0f");
		// --------------------------------------------------------------------------------
		INFO_PROPERTY->count = 8;
		FOCUSER_SPEED_ITEM->number.min = 0;
		FOCUSER_SPEED_ITEM->number.max = 0;
		FOCUSER_TEMPERATURE_PROPERTY->hidden = false;
		FOCUSER_BACKLASH_PROPERTY->hidden = false;
		// --------------------------------------------------------------------------------
		ADDITIONAL_INSTANCES_PROPERTY->hidden = DEVICE_CONTEXT->base_device != NULL;
		pthread_mutex_init(&PRIVATE_DATA->mutex, NULL);
		INDIGO_DEVICE_ATTACH_LOG(DRIVER_NAME, device->name);
		return focuser_enumerate_properties(device, NULL, NULL);
	}
	return INDIGO_FAILED;
}

static indigo_result focuser_enumerate_properties(indigo_device *device, indigo_client *client, indigo_property *property) {
	if (IS_CONNECTED) {
		INDIGO_DEFINE_MATCHING_PROPERTY(X_STATE_PROPERTY);
		INDIGO_DEFINE_MATCHING_PROPERTY(X_CONFIG_PROPERTY);
		INDIGO_DEFINE_MATCHING_PROPERTY(X_LEDS_PROPERTY);
		INDIGO_DEFINE_MATCHING_PROPERTY(X_WIFI_PROPERTY);
		INDIGO_DEFINE_MATCHING_PROPERTY(X_WIFI_AP_PROPERTY);
		INDIGO_DEFINE_MATCHING_PROPERTY(X_WIFI_STA_PROPERTY);
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
	return indigo_focuser_enumerate_properties(device, NULL, NULL);
}

static void focuser_timer_callback(indigo_device *device) {
	char response[1024];
	jsmntok_t tokens[128];
	if (!IS_CONNECTED) {
		return;
	}
	if (primaluce_command(device, "{\"req\":{\"get\":{\"EXT_T\":\"\", \"VIN_12V\": \"\", \"MOT1\":{\"NTC_T\":\"\"}}}}", response, sizeof(response), tokens, 128)) {
		double temp = get_number(response, tokens, GET_EXT_T);
		if (temp != FOCUSER_TEMPERATURE_ITEM->number.value) {
			FOCUSER_TEMPERATURE_ITEM->number.value = temp;
			indigo_update_property(device, FOCUSER_TEMPERATURE_PROPERTY, NULL);
		}
		double motor_temp = get_number(response, tokens, GET_MOT1_NTC_T);
		double vin12v = get_number(response, tokens, GET_VIN_12V);
		if (motor_temp != X_STATE_MOTOR_TEMP_ITEM->number.value || vin12v != X_STATE_VIN_12V_ITEM->number.value) {
			X_STATE_MOTOR_TEMP_ITEM->number.value = motor_temp;
			X_STATE_VIN_12V_ITEM->number.value = vin12v;
			indigo_update_property(device, X_STATE_PROPERTY, NULL);
		}
	}
	indigo_reschedule_timer(device, 1, &PRIVATE_DATA->timer);
}

static void focuser_connection_handler(indigo_device *device) {
	char response[8 * 1024];
	jsmntok_t tokens[1024];
	char *text;
	if (CONNECTION_CONNECTED_ITEM->sw.value) {
		bool result = true;
		if (PRIVATE_DATA->device_count++ == 0) {
			result = primaluce_open(device->master_device);
		}
		if (result) {
			if (primaluce_command(device, "{\"req\":{\"get\": \"\"}}}", response, sizeof(response), tokens, 1024)) {
				if ((text = get_string(response, tokens, GET_MODNAME))) {
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
				if ((text = get_string(response, tokens, GET_SWAPP))) {
					INDIGO_DRIVER_DEBUG(DRIVER_NAME, "SWAPP: %s", text);
					INDIGO_COPY_VALUE(INFO_DEVICE_FW_REVISION_ITEM->text.value, text);
					if ((text = get_string(response, tokens, GET_SWWEB))) {
						INDIGO_DRIVER_DEBUG(DRIVER_NAME, "SWWEB: %s", text);
						strcat(INFO_DEVICE_FW_REVISION_ITEM->text.value, " / ");
						strcat(INFO_DEVICE_FW_REVISION_ITEM->text.value, text);
					}
				}
				if ((text = get_string(response, tokens, GET_SN))) {
					INDIGO_DRIVER_DEBUG(DRIVER_NAME, "SN: %s", text);
					INDIGO_COPY_VALUE(INFO_DEVICE_SERIAL_NUM_ITEM->text.value, text);
				}
				indigo_update_property(device, INFO_PROPERTY, NULL);
				if ((text = get_string(response, tokens, GET_MOT1_ERROR)) && *text) {
					indigo_send_message(device, "ERROR: %s", text);
				}
				if (get_number(response, tokens, GET_CALRESTART_MOT1)) {
					indigo_send_message(device, "ERROR: %s needs calibration", INFO_DEVICE_MODEL_ITEM->text.value);
				}
				PRIVATE_DATA->has_abs_pos = getToken(response, tokens, 0, GET_MOT1_ABS_POS) != -1;
				FOCUSER_POSITION_ITEM->number.value = FOCUSER_POSITION_ITEM->number.target = get_number(response, tokens, PRIVATE_DATA->has_abs_pos ? GET_MOT1_ABS_POS : GET_MOT1_ABS_POS_STEP);
				if (getToken(response, tokens, 0, GET_MOT1_SPEED) == -1) {
					FOCUSER_SPEED_PROPERTY->hidden = true;
				} else {
					FOCUSER_SPEED_ITEM->number.value = FOCUSER_SPEED_ITEM->number.target = get_number(response, tokens, GET_MOT1_SPEED);
					FOCUSER_SPEED_PROPERTY->hidden = false;
				}
				FOCUSER_BACKLASH_ITEM->number.value = get_number(response, tokens, GET_MOT1_BKLASH);
				X_STATE_MOTOR_TEMP_ITEM->number.value = get_number(response, tokens, GET_MOT1_NTC_T);
				X_STATE_VIN_12V_ITEM->number.value = get_number(response, tokens, GET_VIN_12V);
				X_STATE_VIN_USB_ITEM->number.value = get_number(response, tokens, GET_VIN_USB);
				// TBD: STA doesn't work
				if ((text = get_string(response, tokens, GET_WIFIAP_STATUS))) {
					if (!strcmp(text, "on")) {
						indigo_set_switch(X_WIFI_PROPERTY, X_WIFI_AP_ITEM, true);
					} else {
						indigo_set_switch(X_WIFI_PROPERTY, X_WIFI_OFF_ITEM, true);
					}
				}
				if ((text = get_string(response, tokens, GET_WIFIAP_SSID))) {
					INDIGO_COPY_VALUE(X_WIFI_AP_SSID_ITEM->text.value, text);
				}
				if ((text = get_string(response, tokens, GET_WIFIAP_PWD))) {
					INDIGO_COPY_VALUE(X_WIFI_AP_PASSWORD_ITEM->text.value, text);
				}
				if ((text = get_string(response, tokens, GET_WIFISTA_SSID))) {
					INDIGO_COPY_VALUE(X_WIFI_STA_SSID_ITEM->text.value, text);
				}
				if ((text = get_string(response, tokens, GET_WIFISTA_PWD))) {
					INDIGO_COPY_VALUE(X_WIFI_STA_PASSWORD_ITEM->text.value, text);
				}
				if ((text = get_string(response, tokens, GET_DIMLEDS))) {
					if (!strcmp(text, "on")) {
						indigo_set_switch(X_LEDS_PROPERTY, X_LEDS_ON_ITEM, true);
					} else if (!strcmp(text, "low")) {
						indigo_set_switch(X_LEDS_PROPERTY, X_LEDS_DIM_ITEM, true);
					} else {
						indigo_set_switch(X_LEDS_PROPERTY, X_LEDS_OFF_ITEM, true);
					}
				}
				X_CONFIG_M1ACC_ITEM->number.value = X_CONFIG_M1ACC_ITEM->number.target = get_number(response, tokens, GET_MOT1_FnRUN_ACC);
				X_CONFIG_M1SPD_ITEM->number.value = X_CONFIG_M1SPD_ITEM->number.target = get_number(response, tokens, GET_MOT1_FnRUN_SPD);
				X_CONFIG_M1DEC_ITEM->number.value = X_CONFIG_M1DEC_ITEM->number.target = get_number(response, tokens, GET_MOT1_FnRUN_DEC);
				X_CONFIG_M1CACC_ITEM->number.value = X_CONFIG_M1CACC_ITEM->number.target = get_number(response, tokens, GET_MOT1_FnRUN_CURR_ACC);
				X_CONFIG_M1CSPD_ITEM->number.value = X_CONFIG_M1CSPD_ITEM->number.target = get_number(response, tokens, GET_MOT1_FnRUN_CURR_SPD);
				X_CONFIG_M1CDEC_ITEM->number.value = X_CONFIG_M1CDEC_ITEM->number.target = get_number(response, tokens, GET_MOT1_FnRUN_CURR_DEC);
				X_CONFIG_M1HOLD_ITEM->number.value = X_CONFIG_M1HOLD_ITEM->number.target = get_number(response, tokens, GET_MOT1_FnRUN_CURR_HOLD);
				X_RUNPRESET_L_M1ACC_ITEM->number.value = X_RUNPRESET_L_M1ACC_ITEM->number.target = get_number(response, tokens, GET_RUNPRESET_L_M1ACC);
				X_RUNPRESET_L_M1SPD_ITEM->number.value = X_RUNPRESET_L_M1SPD_ITEM->number.target = get_number(response, tokens, GET_RUNPRESET_L_M1SPD);
				X_RUNPRESET_L_M1DEC_ITEM->number.value = X_RUNPRESET_L_M1DEC_ITEM->number.target = get_number(response, tokens, GET_RUNPRESET_L_M1DEC);
				X_RUNPRESET_L_M1CACC_ITEM->number.value = X_RUNPRESET_L_M1CACC_ITEM->number.target = get_number(response, tokens, GET_RUNPRESET_L_M1CACC);
				X_RUNPRESET_L_M1CSPD_ITEM->number.value = X_RUNPRESET_L_M1CSPD_ITEM->number.target = get_number(response, tokens, GET_RUNPRESET_L_M1CSPD);
				X_RUNPRESET_L_M1CDEC_ITEM->number.value = X_RUNPRESET_L_M1CDEC_ITEM->number.target = get_number(response, tokens, GET_RUNPRESET_L_M1CDEC);
				X_RUNPRESET_L_M1HOLD_ITEM->number.value = X_RUNPRESET_L_M1HOLD_ITEM->number.target = get_number(response, tokens, GET_RUNPRESET_L_M1HOLD);
				X_RUNPRESET_M_M1ACC_ITEM->number.value = X_RUNPRESET_M_M1ACC_ITEM->number.target = get_number(response, tokens, GET_RUNPRESET_M_M1ACC);
				X_RUNPRESET_M_M1SPD_ITEM->number.value = X_RUNPRESET_M_M1SPD_ITEM->number.target = get_number(response, tokens, GET_RUNPRESET_M_M1SPD);
				X_RUNPRESET_M_M1DEC_ITEM->number.value = X_RUNPRESET_M_M1DEC_ITEM->number.target = get_number(response, tokens, GET_RUNPRESET_M_M1DEC);
				X_RUNPRESET_M_M1CACC_ITEM->number.value = X_RUNPRESET_M_M1CACC_ITEM->number.target = get_number(response, tokens, GET_RUNPRESET_M_M1CACC);
				X_RUNPRESET_M_M1CSPD_ITEM->number.value = X_RUNPRESET_M_M1CSPD_ITEM->number.target = get_number(response, tokens, GET_RUNPRESET_M_M1CSPD);
				X_RUNPRESET_M_M1CDEC_ITEM->number.value = X_RUNPRESET_M_M1CDEC_ITEM->number.target = get_number(response, tokens, GET_RUNPRESET_M_M1CDEC);
				X_RUNPRESET_M_M1HOLD_ITEM->number.value = X_RUNPRESET_M_M1HOLD_ITEM->number.target = get_number(response, tokens, GET_RUNPRESET_M_M1HOLD);
				X_RUNPRESET_S_M1ACC_ITEM->number.value = X_RUNPRESET_S_M1ACC_ITEM->number.target = get_number(response, tokens, GET_RUNPRESET_S_M1ACC);
				X_RUNPRESET_S_M1SPD_ITEM->number.value = X_RUNPRESET_S_M1SPD_ITEM->number.target = get_number(response, tokens, GET_RUNPRESET_S_M1SPD);
				X_RUNPRESET_S_M1DEC_ITEM->number.value = X_RUNPRESET_S_M1DEC_ITEM->number.target = get_number(response, tokens, GET_RUNPRESET_S_M1DEC);
				X_RUNPRESET_S_M1CACC_ITEM->number.value = X_RUNPRESET_S_M1CACC_ITEM->number.target = get_number(response, tokens, GET_RUNPRESET_S_M1CACC);
				X_RUNPRESET_S_M1CSPD_ITEM->number.value = X_RUNPRESET_S_M1CSPD_ITEM->number.target = get_number(response, tokens, GET_RUNPRESET_S_M1CSPD);
				X_RUNPRESET_S_M1CDEC_ITEM->number.value = X_RUNPRESET_S_M1CDEC_ITEM->number.target = get_number(response, tokens, GET_RUNPRESET_S_M1CDEC);
				X_RUNPRESET_S_M1HOLD_ITEM->number.value = X_RUNPRESET_S_M1HOLD_ITEM->number.target = get_number(response, tokens, GET_RUNPRESET_S_M1HOLD);
				X_RUNPRESET_1_M1ACC_ITEM->number.value = X_RUNPRESET_1_M1ACC_ITEM->number.target = get_number(response, tokens, GET_RUNPRESET_1_M1ACC);
				X_RUNPRESET_1_M1SPD_ITEM->number.value = X_RUNPRESET_1_M1SPD_ITEM->number.target = get_number(response, tokens, GET_RUNPRESET_1_M1SPD);
				X_RUNPRESET_1_M1DEC_ITEM->number.value = X_RUNPRESET_1_M1DEC_ITEM->number.target = get_number(response, tokens, GET_RUNPRESET_1_M1DEC);
				X_RUNPRESET_1_M1CACC_ITEM->number.value = X_RUNPRESET_1_M1CACC_ITEM->number.target = get_number(response, tokens, GET_RUNPRESET_1_M1CACC);
				X_RUNPRESET_1_M1CSPD_ITEM->number.value = X_RUNPRESET_1_M1CSPD_ITEM->number.target = get_number(response, tokens, GET_RUNPRESET_1_M1CSPD);
				X_RUNPRESET_1_M1CDEC_ITEM->number.value = X_RUNPRESET_1_M1CDEC_ITEM->number.target = get_number(response, tokens, GET_RUNPRESET_1_M1CDEC);
				X_RUNPRESET_1_M1HOLD_ITEM->number.value = X_RUNPRESET_1_M1HOLD_ITEM->number.target = get_number(response, tokens, GET_RUNPRESET_1_M1HOLD);
				X_RUNPRESET_2_M1ACC_ITEM->number.value = X_RUNPRESET_2_M1ACC_ITEM->number.target = get_number(response, tokens, GET_RUNPRESET_2_M1ACC);
				X_RUNPRESET_2_M1SPD_ITEM->number.value = X_RUNPRESET_2_M1SPD_ITEM->number.target = get_number(response, tokens, GET_RUNPRESET_2_M1SPD);
				X_RUNPRESET_2_M1DEC_ITEM->number.value = X_RUNPRESET_2_M1DEC_ITEM->number.target = get_number(response, tokens, GET_RUNPRESET_2_M1DEC);
				X_RUNPRESET_2_M1CACC_ITEM->number.value = X_RUNPRESET_2_M1CACC_ITEM->number.target = get_number(response, tokens, GET_RUNPRESET_2_M1CACC);
				X_RUNPRESET_2_M1CSPD_ITEM->number.value = X_RUNPRESET_2_M1CSPD_ITEM->number.target = get_number(response, tokens, GET_RUNPRESET_2_M1CSPD);
				X_RUNPRESET_2_M1CDEC_ITEM->number.value = X_RUNPRESET_2_M1CDEC_ITEM->number.target = get_number(response, tokens, GET_RUNPRESET_2_M1CDEC);
				X_RUNPRESET_2_M1HOLD_ITEM->number.value = X_RUNPRESET_2_M1HOLD_ITEM->number.target = get_number(response, tokens, GET_RUNPRESET_2_M1HOLD);
				X_RUNPRESET_3_M1ACC_ITEM->number.value = X_RUNPRESET_3_M1ACC_ITEM->number.target = get_number(response, tokens, GET_RUNPRESET_3_M1ACC);
				X_RUNPRESET_3_M1SPD_ITEM->number.value = X_RUNPRESET_3_M1SPD_ITEM->number.target = get_number(response, tokens, GET_RUNPRESET_3_M1SPD);
				X_RUNPRESET_3_M1DEC_ITEM->number.value = X_RUNPRESET_3_M1DEC_ITEM->number.target = get_number(response, tokens, GET_RUNPRESET_3_M1DEC);
				X_RUNPRESET_3_M1CACC_ITEM->number.value = X_RUNPRESET_3_M1CACC_ITEM->number.target = get_number(response, tokens, GET_RUNPRESET_3_M1CACC);
				X_RUNPRESET_3_M1CSPD_ITEM->number.value = X_RUNPRESET_3_M1CSPD_ITEM->number.target = get_number(response, tokens, GET_RUNPRESET_3_M1CSPD);
				X_RUNPRESET_3_M1CDEC_ITEM->number.value = X_RUNPRESET_3_M1CDEC_ITEM->number.target = get_number(response, tokens, GET_RUNPRESET_3_M1CDEC);
				X_RUNPRESET_3_M1HOLD_ITEM->number.value = X_RUNPRESET_3_M1HOLD_ITEM->number.target = get_number(response, tokens, GET_RUNPRESET_3_M1HOLD);
				if (get_number(response, tokens, GET_MOT1_HOLDCURR_STATUS)) {
					indigo_set_switch(X_HOLD_CURR_PROPERTY, X_HOLD_CURR_ON_ITEM, true);
				} else {
					indigo_set_switch(X_HOLD_CURR_PROPERTY, X_HOLD_CURR_OFF_ITEM, true);
				}
			}
			indigo_define_property(device, X_STATE_PROPERTY, NULL);
			indigo_define_property(device, X_CONFIG_PROPERTY, NULL);
			indigo_define_property(device, X_LEDS_PROPERTY, NULL);
			indigo_define_property(device, X_WIFI_PROPERTY, NULL);
			indigo_define_property(device, X_WIFI_AP_PROPERTY, NULL);
			indigo_define_property(device, X_WIFI_STA_PROPERTY, NULL);
			indigo_define_property(device, X_RUNPRESET_L_PROPERTY, NULL);
			indigo_define_property(device, X_RUNPRESET_M_PROPERTY, NULL);
			indigo_define_property(device, X_RUNPRESET_S_PROPERTY, NULL);
			indigo_define_property(device, X_RUNPRESET_1_PROPERTY, NULL);
			indigo_define_property(device, X_RUNPRESET_2_PROPERTY, NULL);
			indigo_define_property(device, X_RUNPRESET_3_PROPERTY, NULL);
			indigo_define_property(device, X_RUNPRESET_PROPERTY, NULL);
			indigo_define_property(device, X_HOLD_CURR_PROPERTY, NULL);
			indigo_define_property(device, X_CALIBRATE_F_PROPERTY, NULL);
			indigo_set_timer(device, 0, focuser_timer_callback, &PRIVATE_DATA->timer);
			CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
		} else {
			PRIVATE_DATA->device_count--;
			CONNECTION_PROPERTY->state = INDIGO_ALERT_STATE;
			indigo_set_switch(CONNECTION_PROPERTY, CONNECTION_DISCONNECTED_ITEM, true);
		}
	} else {
		if (PRIVATE_DATA->handle != NULL) {
			indigo_cancel_timer_sync(device, &PRIVATE_DATA->timer);
			indigo_delete_property(device, X_STATE_PROPERTY, NULL);
			indigo_delete_property(device, X_CONFIG_PROPERTY, NULL);
			indigo_delete_property(device, X_LEDS_PROPERTY, NULL);
			indigo_delete_property(device, X_WIFI_PROPERTY, NULL);
			indigo_delete_property(device, X_WIFI_AP_PROPERTY, NULL);
			indigo_delete_property(device, X_WIFI_STA_PROPERTY, NULL);
			indigo_delete_property(device, X_RUNPRESET_L_PROPERTY, NULL);
			indigo_delete_property(device, X_RUNPRESET_M_PROPERTY, NULL);
			indigo_delete_property(device, X_RUNPRESET_S_PROPERTY, NULL);
			indigo_delete_property(device, X_RUNPRESET_1_PROPERTY, NULL);
			indigo_delete_property(device, X_RUNPRESET_2_PROPERTY, NULL);
			indigo_delete_property(device, X_RUNPRESET_3_PROPERTY, NULL);
			indigo_delete_property(device, X_RUNPRESET_PROPERTY, NULL);
			indigo_delete_property(device, X_HOLD_CURR_PROPERTY, NULL);
			indigo_delete_property(device, X_CALIBRATE_F_PROPERTY, NULL);
			INDIGO_DRIVER_LOG(DRIVER_NAME, "Disconnected");
			if (--PRIVATE_DATA->device_count == 0) {
				primaluce_close(device->master_device);
			}
		}
		CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
	}
	indigo_focuser_change_property(device, NULL, CONNECTION_PROPERTY);
}

static void focuser_position_handler(indigo_device *device) {
	char command[1024];
	char response[1024];
	jsmntok_t tokens[128];
	snprintf(command, sizeof(command), "{\"req\":{\"cmd\":{\"MOT1\":{\"MOVE_ABS\":{\"STEP\":%d}}}}}", (int)FOCUSER_POSITION_ITEM->number.target);
	if (!primaluce_command(device, command, response, sizeof(response), tokens, 128)) {
		FOCUSER_POSITION_PROPERTY->state = FOCUSER_STEPS_PROPERTY->state = INDIGO_ALERT_STATE;
		indigo_update_property(device, FOCUSER_STEPS_PROPERTY, NULL);
		indigo_update_property(device, FOCUSER_POSITION_PROPERTY, NULL);
		return;
	}
	char *state = get_string(response, tokens, CMD_MOT1_STEP);
	if (state == NULL || strcmp(state, "done")) {
		FOCUSER_POSITION_PROPERTY->state = FOCUSER_STEPS_PROPERTY->state = INDIGO_ALERT_STATE;
		indigo_update_property(device, FOCUSER_STEPS_PROPERTY, NULL);
		indigo_update_property(device, FOCUSER_POSITION_PROPERTY, NULL);
		return;
	}
	char *get_pos_command = PRIVATE_DATA->has_abs_pos ? "{\"req\":{\"get\":{\"MOT1\":{\"ABS_POS\":\"STEP\",\"STATUS\":\"\"}}}}" : "{\"req\":{\"get\":{\"MOT1\":{\"ABS_POS_STEP\":\"\",\"STATUS\":\"\"}}}}";
	while (true) {
		if (primaluce_command(device, get_pos_command, response, sizeof(response), tokens, 128)) {
			FOCUSER_POSITION_ITEM->number.value = get_number(response, tokens, PRIVATE_DATA->has_abs_pos ? GET_MOT1_ABS_POS : GET_MOT1_ABS_POS_STEP);
			if (!strcmp(get_string(response, tokens, GET_MOT1_MST), "stop")) {
				break;
			}
			indigo_update_property(device, FOCUSER_POSITION_PROPERTY, NULL);
		}
	}
	for (int i = 0; i < 10; i++) {
		indigo_usleep(100000);
		if (primaluce_command(device, get_pos_command, response, sizeof(response), tokens, 128)) {
			FOCUSER_POSITION_ITEM->number.value = get_number(response, tokens, PRIVATE_DATA->has_abs_pos ? GET_MOT1_ABS_POS : GET_MOT1_ABS_POS_STEP);
			indigo_update_property(device, FOCUSER_POSITION_PROPERTY, NULL);
		}
		if (FOCUSER_POSITION_ITEM->number.target == FOCUSER_POSITION_ITEM->number.value) {
			break;
		}
	}
	FOCUSER_POSITION_PROPERTY->state = FOCUSER_STEPS_PROPERTY->state = INDIGO_OK_STATE;
	indigo_update_property(device, FOCUSER_STEPS_PROPERTY, NULL);
	indigo_update_property(device, FOCUSER_POSITION_PROPERTY, NULL);
}

static void focuser_steps_handler(indigo_device *device) {
	int steps = FOCUSER_DIRECTION_MOVE_OUTWARD_ITEM->sw.value ? (int)FOCUSER_STEPS_ITEM->number.target : -(int)FOCUSER_STEPS_ITEM->number.target;
	FOCUSER_POSITION_ITEM->number.target = FOCUSER_POSITION_ITEM->number.value + steps;
	if (FOCUSER_POSITION_ITEM->number.target < 0) {
		FOCUSER_POSITION_ITEM->number.target = 0;
	}
	focuser_position_handler(device);
}

static void focuser_bklash_handler(indigo_device *device) {
	char command[1024];
	char response[1024];
	jsmntok_t tokens[128];
	snprintf(command, sizeof(command), "{\"req\":{\"set\":{\"MOT1\":{\"BKLASH\":%d}}}}", (int)FOCUSER_BACKLASH_ITEM->number.target);
	if (!primaluce_command(device, command, response, sizeof(response), tokens, 128)) {
		FOCUSER_BACKLASH_PROPERTY->state = INDIGO_ALERT_STATE;
		indigo_update_property(device, FOCUSER_BACKLASH_PROPERTY, NULL);
		return;
	}
	char *state = get_string(response, tokens, SET_MOT1_BKLASH);
	if (state == NULL || strcmp(state, "done")) {
		FOCUSER_BACKLASH_PROPERTY->state = INDIGO_ALERT_STATE;
		indigo_update_property(device, FOCUSER_BACKLASH_PROPERTY, NULL);
		return;
	}
	FOCUSER_BACKLASH_PROPERTY->state = INDIGO_OK_STATE;
	indigo_update_property(device, FOCUSER_BACKLASH_PROPERTY, NULL);
}

static void focuser_preset_1_handler(indigo_device *device) {
	char command[1024];
	char response[1024];
	jsmntok_t tokens[128];
	snprintf(command, sizeof(command), "{\"req\":{\"set\":{\"RUNPRESET_1\":{\"M1ACC\":%d,\"M1DEC\":%d,\"M1SPD\":%d,\"M1CACC\":%d,\"M1CDEC\":%d,\"M1CSPD\":%d,\"M1HOLD\":%d}}}}", (int)X_RUNPRESET_1_M1ACC_ITEM->number.target, (int)X_RUNPRESET_1_M1DEC_ITEM->number.target, (int)X_RUNPRESET_1_M1SPD_ITEM->number.target, (int)X_RUNPRESET_1_M1CACC_ITEM->number.target, (int)X_RUNPRESET_1_M1CDEC_ITEM->number.target, (int)X_RUNPRESET_1_M1CSPD_ITEM->number.target, (int)X_RUNPRESET_1_M1HOLD_ITEM->number.target);
	if (!primaluce_command(device, command, response, sizeof(response), tokens, 128)) {
		X_RUNPRESET_1_PROPERTY->state = INDIGO_ALERT_STATE;
		indigo_update_property(device, X_RUNPRESET_1_PROPERTY, NULL);
		return;
	}
	X_RUNPRESET_1_PROPERTY->state = INDIGO_OK_STATE;
	indigo_update_property(device, X_RUNPRESET_1_PROPERTY, NULL);
}

static void focuser_preset_2_handler(indigo_device *device) {
	char command[1024];
	char response[1024];
	jsmntok_t tokens[128];
	snprintf(command, sizeof(command), "{\"req\":{\"set\":{\"RUNPRESET_2\":{\"M1ACC\":%d,\"M1DEC\":%d,\"M1SPD\":%d,\"M1CACC\":%d,\"M1CDEC\":%d,\"M1CSPD\":%d,\"M1HOLD\":%d}}}}", (int)X_RUNPRESET_2_M1ACC_ITEM->number.target, (int)X_RUNPRESET_2_M1DEC_ITEM->number.target, (int)X_RUNPRESET_2_M1SPD_ITEM->number.target, (int)X_RUNPRESET_2_M1CACC_ITEM->number.target, (int)X_RUNPRESET_2_M1CDEC_ITEM->number.target, (int)X_RUNPRESET_2_M1CSPD_ITEM->number.target, (int)X_RUNPRESET_2_M1HOLD_ITEM->number.target);
	if (!primaluce_command(device, command, response, sizeof(response), tokens, 128)) {
		X_RUNPRESET_2_PROPERTY->state = INDIGO_ALERT_STATE;
		indigo_update_property(device, X_RUNPRESET_2_PROPERTY, NULL);
		return;
	}
	X_RUNPRESET_2_PROPERTY->state = INDIGO_OK_STATE;
	indigo_update_property(device, X_RUNPRESET_2_PROPERTY, NULL);
}

static void focuser_preset_3_handler(indigo_device *device) {
	char command[1024];
	char response[1024];
	jsmntok_t tokens[128];
	snprintf(command, sizeof(command), "{\"req\":{\"set\":{\"RUNPRESET_3\":{\"M1ACC\":%d,\"M1DEC\":%d,\"M1SPD\":%d,\"M1CACC\":%d,\"M1CDEC\":%d,\"M1CSPD\":%d,\"M1HOLD\":%d}}}}", (int)X_RUNPRESET_3_M1ACC_ITEM->number.target, (int)X_RUNPRESET_3_M1DEC_ITEM->number.target, (int)X_RUNPRESET_3_M1SPD_ITEM->number.target, (int)X_RUNPRESET_3_M1CACC_ITEM->number.target, (int)X_RUNPRESET_3_M1CDEC_ITEM->number.target, (int)X_RUNPRESET_3_M1CSPD_ITEM->number.target, (int)X_RUNPRESET_3_M1HOLD_ITEM->number.target);
	if (!primaluce_command(device, command, response, sizeof(response), tokens, 128)) {
		X_RUNPRESET_3_PROPERTY->state = INDIGO_ALERT_STATE;
		indigo_update_property(device, X_RUNPRESET_3_PROPERTY, NULL);
		return;
	}
	X_RUNPRESET_3_PROPERTY->state = INDIGO_OK_STATE;
	indigo_update_property(device, X_RUNPRESET_3_PROPERTY, NULL);
}

static void focuser_preset_handler(indigo_device *device) {
	char command[1024];
	char response[1024];
	jsmntok_t tokens[128];
	if (X_RUNPRESET_L_ITEM->sw.value) {
		snprintf(command, sizeof(command), "{\"req\":{\"cmd\":{\"RUNPRESET\":\"light\"}}}");
		X_RUNPRESET_L_ITEM->sw.value = false;
	} else if (X_RUNPRESET_M_ITEM->sw.value) {
		snprintf(command, sizeof(command), "{\"req\":{\"cmd\":{\"RUNPRESET\":\"medium\"}}}");
		X_RUNPRESET_M_ITEM->sw.value = false;
	} else if (X_RUNPRESET_S_ITEM->sw.value) {
		snprintf(command, sizeof(command), "{\"req\":{\"cmd\":{\"RUNPRESET\":\"slow\"}}}");
		X_RUNPRESET_S_ITEM->sw.value = false;
	} else if (X_RUNPRESET_1_ITEM->sw.value) {
		snprintf(command, sizeof(command), "{\"req\":{\"cmd\":{\"RUNPRESET\":1}}}");
		X_RUNPRESET_1_ITEM->sw.value = false;
	} else if (X_RUNPRESET_2_ITEM->sw.value) {
		snprintf(command, sizeof(command), "{\"req\":{\"cmd\":{\"RUNPRESET\":2}}}");
		X_RUNPRESET_2_ITEM->sw.value = false;
	} else if (X_RUNPRESET_3_ITEM->sw.value) {
		snprintf(command, sizeof(command), "{\"req\":{\"cmd\":{\"RUNPRESET\":3}}}");
		X_RUNPRESET_3_ITEM->sw.value = false;
	}
	if (!primaluce_command(device, command, response, sizeof(response), tokens, 128)) {
		X_RUNPRESET_PROPERTY->state = INDIGO_ALERT_STATE;
		indigo_update_property(device, X_RUNPRESET_PROPERTY, NULL);
		return;
	}
	if (!primaluce_command(device, "{\"req\":{\"get\":{\"MOT1\":{\"HOLDCURR_STATUS\":\"\",\"FnRUN_SPD\":\"\",\"FnRUN_DEC\":\"\",\"FnRUN_ACC\":\"\",\"FnRUN_CURR_SPD\":\"\",\"FnRUN_CURR_DEC\":\"\",\"FnRUN_CURR_ACC\":\"\",\"FnRUN_CURR_HOLD\":\"\"}}}}", response, sizeof(response), tokens, 128)) {
		X_CONFIG_PROPERTY->state = INDIGO_ALERT_STATE;
		indigo_update_property(device, X_CONFIG_PROPERTY, NULL);
		return;
	}
	X_CONFIG_M1ACC_ITEM->number.value = X_CONFIG_M1ACC_ITEM->number.target = get_number(response, tokens, GET_MOT1_FnRUN_ACC);
	X_CONFIG_M1SPD_ITEM->number.value = X_CONFIG_M1SPD_ITEM->number.target = get_number(response, tokens, GET_MOT1_FnRUN_SPD);
	X_CONFIG_M1DEC_ITEM->number.value = X_CONFIG_M1DEC_ITEM->number.target = get_number(response, tokens, GET_MOT1_FnRUN_DEC);
	X_CONFIG_M1CACC_ITEM->number.value = X_CONFIG_M1CACC_ITEM->number.target = get_number(response, tokens, GET_MOT1_FnRUN_CURR_ACC);
	X_CONFIG_M1CSPD_ITEM->number.value = X_CONFIG_M1CSPD_ITEM->number.target = get_number(response, tokens, GET_MOT1_FnRUN_CURR_SPD);
	X_CONFIG_M1CDEC_ITEM->number.value = X_CONFIG_M1CDEC_ITEM->number.target = get_number(response, tokens, GET_MOT1_FnRUN_CURR_DEC);
	X_CONFIG_M1HOLD_ITEM->number.value = X_CONFIG_M1HOLD_ITEM->number.target = get_number(response, tokens, GET_MOT1_FnRUN_CURR_HOLD);
	X_CONFIG_PROPERTY->state = INDIGO_OK_STATE;
	indigo_update_property(device, X_CONFIG_PROPERTY, NULL);
	X_RUNPRESET_PROPERTY->state = INDIGO_OK_STATE;
	indigo_update_property(device, X_RUNPRESET_PROPERTY, NULL);
}

static void focuser_hold_curr_handler(indigo_device *device) {
	char command[1024];
	char response[1024];
	jsmntok_t tokens[128];
	if (X_HOLD_CURR_OFF_ITEM->sw.value) {
		snprintf(command, sizeof(command), "{\"req\":{\"set\":{\"MOT1\":{\"HOLDCURR_STATUS\":0}}}}");
	} else if (X_LEDS_ON_ITEM->sw.value) {
		snprintf(command, sizeof(command), "{\"req\":{\"set\":{\"MOT1\":{\"HOLDCURR_STATUS\":1}}}}");
	}
	if (!primaluce_command(device, command, response, sizeof(response), tokens, 128)) {
		X_HOLD_CURR_PROPERTY->state = INDIGO_ALERT_STATE;
		indigo_update_property(device, X_HOLD_CURR_PROPERTY, NULL);
		return;
	}
	char *state = get_string(response, tokens, SET_MOT1_HOLDCURR_STATUS);
	if (state == NULL || strcmp(state, "done")) {
		X_HOLD_CURR_PROPERTY->state = INDIGO_ALERT_STATE;
		indigo_update_property(device, X_HOLD_CURR_PROPERTY, NULL);
		return;
	}
	X_HOLD_CURR_PROPERTY->state = INDIGO_OK_STATE;
	indigo_update_property(device, X_HOLD_CURR_PROPERTY, NULL);
}

static void focuser_speed_handler(indigo_device *device) {
	char command[1024];
	char response[1024];
	jsmntok_t tokens[128];
	snprintf(command, sizeof(command), "{\"req\":{\"set\":{\"MOT1\":{\"SPEED\":%d}}}}", (int)FOCUSER_SPEED_ITEM->number.target);
	if (!primaluce_command(device, command, response, sizeof(response), tokens, 128)) {
		FOCUSER_SPEED_PROPERTY->state = INDIGO_ALERT_STATE;
		indigo_update_property(device, FOCUSER_SPEED_PROPERTY, NULL);
		return;
	}
	char *state = get_string(response, tokens, SET_MOT1_SPEED);
	if (state == NULL || strcmp(state, "done")) {
		FOCUSER_SPEED_PROPERTY->state = INDIGO_ALERT_STATE;
		indigo_update_property(device, FOCUSER_SPEED_PROPERTY, NULL);
		return;
	}
	FOCUSER_SPEED_PROPERTY->state = INDIGO_OK_STATE;
	indigo_update_property(device, FOCUSER_SPEED_PROPERTY, NULL);
}

static void focuser_abort_handler(indigo_device *device) {
	char response[1024];
	jsmntok_t tokens[128];
	FOCUSER_ABORT_MOTION_ITEM->sw.value = false;
	if (!primaluce_command(device, "{\"req\":{\"cmd\":{\"MOT1\":{\"MOT_STOP\":\"\"}}}}", response, sizeof(response), tokens, 128)) {
		FOCUSER_ABORT_MOTION_PROPERTY->state = INDIGO_ALERT_STATE;
		indigo_update_property(device, FOCUSER_ABORT_MOTION_PROPERTY, NULL);
		return;
	}
	char *state = get_string(response, tokens, CMD_MOT1_MOT_STOP);
	if (state == NULL || strcmp(state, "done")) {
		FOCUSER_ABORT_MOTION_PROPERTY->state = INDIGO_ALERT_STATE;
		indigo_update_property(device, FOCUSER_ABORT_MOTION_PROPERTY, NULL);
		return;
	}
	FOCUSER_ABORT_MOTION_PROPERTY->state = INDIGO_OK_STATE;
	indigo_update_property(device, FOCUSER_ABORT_MOTION_PROPERTY, NULL);
}

static void focuser_leds_handler(indigo_device *device) {
	char command[1024];
	char response[1024];
	jsmntok_t tokens[128];
	if (X_LEDS_OFF_ITEM->sw.value) {
		snprintf(command, sizeof(command), "{\"req\":{\"cmd\":{\"DIMLEDS\":\"off\"}}}");
	} else if (X_LEDS_DIM_ITEM->sw.value) {
		snprintf(command, sizeof(command), "{\"req\":{\"cmd\":{\"DIMLEDS\":\"low\"}}}");
	} else if (X_LEDS_ON_ITEM->sw.value) {
		snprintf(command, sizeof(command), "{\"req\":{\"cmd\":{\"DIMLEDS\":\"on\"}}}");
	}
	if (!primaluce_command(device, command, response, sizeof(response), tokens, 128)) {
		X_LEDS_PROPERTY->state = INDIGO_ALERT_STATE;
		indigo_update_property(device, X_LEDS_PROPERTY, NULL);
		return;
	}
	X_LEDS_PROPERTY->state = INDIGO_OK_STATE;
	indigo_update_property(device, X_LEDS_PROPERTY, NULL);
}

static void focuser_wifi_handler(indigo_device *device) {
	char command[1024];
	char response[1024];
	jsmntok_t tokens[128];
	if (X_WIFI_OFF_ITEM->sw.value) {
		snprintf(command, sizeof(command), "{\"req\":{\"cmd\":{\"AP_SET_STATUS\":\"off\"}}}");
	} else if (X_WIFI_AP_ITEM->sw.value) {
		snprintf(command, sizeof(command), "{\"req\":{\"cmd\":{\"AP_SET_STATUS\":\"on\"}}}");
	} else if (X_WIFI_STA_ITEM->sw.value) {
		snprintf(command, sizeof(command), "{\"req\":{\"cmd\":{\"STA_SET_STATUS\":\"on\"}}}");
	}
	if (!primaluce_command(device, command, response, sizeof(response), tokens, 128)) {
		X_WIFI_PROPERTY->state = INDIGO_ALERT_STATE;
		indigo_update_property(device, X_WIFI_PROPERTY, NULL);
		return;
	}
	X_WIFI_PROPERTY->state = INDIGO_OK_STATE;
	indigo_update_property(device, X_WIFI_PROPERTY, NULL);
}

static void focuser_wifi_ap_handler(indigo_device *device) {
	char command[1024];
	char response[1024];
	jsmntok_t tokens[128];
	snprintf(command, sizeof(command), "{\"req\":{\"set\":{\"WIFIAP\":{\"SSID\":\"%s\", \"PWD\":\"%s\"}}}}", X_WIFI_AP_SSID_ITEM->text.value, X_WIFI_AP_PASSWORD_ITEM->text.value);
	if (primaluce_command(device, command, response, sizeof(response), tokens, 128)) {
		X_WIFI_AP_PROPERTY->state = INDIGO_OK_STATE;
	} else {
		X_WIFI_AP_PROPERTY->state = INDIGO_ALERT_STATE;
	}
	indigo_update_property(device, X_WIFI_AP_PROPERTY, NULL);
}

static void focuser_wifi_sta_handler(indigo_device *device) {
	char command[1024];
	char response[1024];
	jsmntok_t tokens[128];
	snprintf(command, sizeof(command), "{\"req\":{\"set\":{\"WIFISTA\":{\"SSID\":\"%s\", \"PWD\":\"%s\"}}}}", X_WIFI_STA_SSID_ITEM->text.value, X_WIFI_STA_PASSWORD_ITEM->text.value);
	if (primaluce_command(device, command, response, sizeof(response), tokens, 128)) {
		X_WIFI_STA_PROPERTY->state = INDIGO_OK_STATE;
	} else {
		X_WIFI_STA_PROPERTY->state = INDIGO_ALERT_STATE;
	}
	indigo_update_property(device, X_WIFI_STA_PROPERTY, NULL);
}

static void focuser_calibrate_handler(indigo_device *device) {
	char response[1024];
	jsmntok_t tokens[128];
	bool result = true;
	if (X_CALIBRATE_F_START_ITEM->sw.value) {
		result = primaluce_command(device, "{\"req\":{\"cmd\": {\"MOT1\": {\"CAL_FOCUSER\":\"Init\"}}}}", response, sizeof(response), tokens, 128);
		if (result) {
			indigo_usleep(1000000);
			result = primaluce_command(device, "{\"req\":{\"set\": {\"MOT1\": {\"CAL_DIR\":\"normal\"}}}}", response, sizeof(response), tokens, 128);
		}
		if (result) {
			indigo_usleep(1000000);
			result = primaluce_command(device, "{\"req\":{\"cmd\": {\"MOT1\": {\"CAL_FOCUSER\":\"StoreAsMinPos\"}}}}", response, sizeof(response), tokens, 128);
		}
		if (result) {
			indigo_usleep(1000000);
			result = primaluce_command(device, "{\"req\":{\"cmd\": {\"MOT1\": {\"CAL_FOCUSER\":\"GoOutToFindMaxPos\"}}}}", response, sizeof(response), tokens, 128);
		}
	} else if (X_CALIBRATE_F_START_INVERTED_ITEM->sw.value) {
		result = primaluce_command(device, "{\"req\":{\"cmd\": {\"MOT1\": {\"CAL_FOCUSER\":\"Init\"}}}}", response, sizeof(response), tokens, 128);
		if (result) {
			indigo_usleep(1000000);
			result = primaluce_command(device, "{\"req\":{\"set\": {\"MOT1\": {\"CAL_DIR\":\"invert\"}}}}", response, sizeof(response), tokens, 128);
		}
		if (result) {
			indigo_usleep(1000000);
			result = primaluce_command(device, "{\"req\":{\"cmd\": {\"MOT1\": {\"CAL_FOCUSER\":\"StoreAsMinPos\"}}}}", response, sizeof(response), tokens, 128);
		}
		if (result) {
			indigo_usleep(1000000);
			result = primaluce_command(device, "{\"req\":{\"cmd\": {\"MOT1\": {\"CAL_FOCUSER\":\"GoOutToFindMaxPos\"}}}}", response, sizeof(response), tokens, 128);
		}
	} else if (X_CALIBRATE_F_END_ITEM->sw.value) {
		X_CALIBRATE_F_START_ITEM->sw.value = X_CALIBRATE_F_START_INVERTED_ITEM->sw.value = X_CALIBRATE_F_END_ITEM->sw.value = false;
		result = primaluce_command(device, "{\"req\":{\"cmd\": {\"MOT1\": {\"CAL_FOCUSER\":\"StoreAsMaxPos\"}}}}", response, sizeof(response), tokens, 128);
		if (result) {
			char *get_pos_command = PRIVATE_DATA->has_abs_pos ? "{\"req\":{\"get\":{\"MOT1\":{\"ABS_POS\":\"STEP\",\"STATUS\":\"\"}}}}" : "{\"req\":{\"get\":{\"MOT1\":{\"ABS_POS_STEP\":\"\",\"STATUS\":\"\"}}}}";
			if (primaluce_command(device, get_pos_command, response, sizeof(response), tokens, 128)) {
				FOCUSER_POSITION_ITEM->number.value = FOCUSER_POSITION_ITEM->number.target = get_number(response, tokens, PRIVATE_DATA->has_abs_pos ? GET_MOT1_ABS_POS : GET_MOT1_ABS_POS_STEP);
				indigo_update_property(device, FOCUSER_POSITION_PROPERTY, NULL);
			}
		}
	}
	X_CALIBRATE_F_PROPERTY->state = result ? INDIGO_OK_STATE : INDIGO_ALERT_STATE;
	indigo_update_property(device, X_CALIBRATE_F_PROPERTY, NULL);
}

static indigo_result focuser_change_property(indigo_device *device, indigo_client *client, indigo_property *property) {
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
		indigo_set_timer(device, 0, focuser_connection_handler, NULL);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(FOCUSER_STEPS_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- FOCUSER_STEPS
		indigo_property_copy_values(FOCUSER_STEPS_PROPERTY, property, false);
		FOCUSER_STEPS_PROPERTY->state = INDIGO_BUSY_STATE;
		indigo_update_property(device, FOCUSER_STEPS_PROPERTY, NULL);
		indigo_set_timer(device, 0, focuser_steps_handler, NULL);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(FOCUSER_POSITION_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- FOCUSER_POSITION
		indigo_property_copy_values(FOCUSER_POSITION_PROPERTY, property, false);
		FOCUSER_POSITION_PROPERTY->state = INDIGO_BUSY_STATE;
		indigo_update_property(device, FOCUSER_POSITION_PROPERTY, NULL);
		indigo_set_timer(device, 0, focuser_position_handler, NULL);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(FOCUSER_ABORT_MOTION_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- FOCUSER_ABORT_MOTION
		indigo_property_copy_values(FOCUSER_ABORT_MOTION_PROPERTY, property, false);
		FOCUSER_ABORT_MOTION_PROPERTY->state = INDIGO_BUSY_STATE;
		indigo_update_property(device, FOCUSER_ABORT_MOTION_PROPERTY, NULL);
		indigo_set_timer(device, 0, focuser_abort_handler, NULL);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(FOCUSER_SPEED_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- FOCUSER_SPEED
		indigo_property_copy_values(FOCUSER_SPEED_PROPERTY, property, false);
		FOCUSER_SPEED_PROPERTY->state = INDIGO_BUSY_STATE;
		indigo_update_property(device, FOCUSER_SPEED_PROPERTY, NULL);
		indigo_set_timer(device, 0, focuser_speed_handler, NULL);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(FOCUSER_BACKLASH_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- FOCUSER_BACKLASH
		indigo_property_copy_values(FOCUSER_BACKLASH_PROPERTY, property, false);
		FOCUSER_BACKLASH_PROPERTY->state = INDIGO_BUSY_STATE;
		indigo_update_property(device, FOCUSER_BACKLASH_PROPERTY, NULL);
		indigo_set_timer(device, 0, focuser_bklash_handler, NULL);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(X_RUNPRESET_1_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- X_RUNPRESET_1
		indigo_property_copy_values(X_RUNPRESET_1_PROPERTY, property, false);
		X_RUNPRESET_1_PROPERTY->state = INDIGO_BUSY_STATE;
		indigo_update_property(device, X_RUNPRESET_1_PROPERTY, NULL);
		indigo_set_timer(device, 0, focuser_preset_1_handler, NULL);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(X_RUNPRESET_2_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- X_RUNPRESET_2
		indigo_property_copy_values(X_RUNPRESET_2_PROPERTY, property, false);
		X_RUNPRESET_2_PROPERTY->state = INDIGO_BUSY_STATE;
		indigo_update_property(device, X_RUNPRESET_2_PROPERTY, NULL);
		indigo_set_timer(device, 0, focuser_preset_2_handler, NULL);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(X_RUNPRESET_3_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- X_RUNPRESET_3
		indigo_property_copy_values(X_RUNPRESET_3_PROPERTY, property, false);
		X_RUNPRESET_3_PROPERTY->state = INDIGO_BUSY_STATE;
		indigo_update_property(device, X_RUNPRESET_3_PROPERTY, NULL);
		indigo_set_timer(device, 0, focuser_preset_3_handler, NULL);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(X_RUNPRESET_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- X_RUNPRESET
		indigo_property_copy_values(X_RUNPRESET_PROPERTY, property, false);
		X_RUNPRESET_PROPERTY->state = INDIGO_BUSY_STATE;
		indigo_update_property(device, X_RUNPRESET_PROPERTY, NULL);
		indigo_set_timer(device, 0, focuser_preset_handler, NULL);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(X_HOLD_CURR_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- X_HOLD_CURR
		indigo_property_copy_values(X_HOLD_CURR_PROPERTY, property, false);
		X_HOLD_CURR_PROPERTY->state = INDIGO_BUSY_STATE;
		indigo_update_property(device, X_HOLD_CURR_PROPERTY, NULL);
		indigo_set_timer(device, 0, focuser_hold_curr_handler, NULL);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(X_LEDS_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- X_LEDS
		indigo_property_copy_values(X_LEDS_PROPERTY, property, false);
		X_LEDS_PROPERTY->state = INDIGO_BUSY_STATE;
		indigo_update_property(device, X_LEDS_PROPERTY, NULL);
		indigo_set_timer(device, 0, focuser_leds_handler, NULL);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(X_WIFI_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- X_WIFI
		indigo_property_copy_values(X_WIFI_PROPERTY, property, false);
		X_WIFI_PROPERTY->state = INDIGO_BUSY_STATE;
		indigo_update_property(device, X_WIFI_PROPERTY, NULL);
		indigo_set_timer(device, 0, focuser_wifi_handler, NULL);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(X_WIFI_AP_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- X_WIFI_AP
		indigo_property_copy_values(X_WIFI_AP_PROPERTY, property, false);
		X_WIFI_AP_PROPERTY->state = INDIGO_BUSY_STATE;
		indigo_update_property(device, X_WIFI_AP_PROPERTY, NULL);
		indigo_set_timer(device, 0, focuser_wifi_ap_handler, NULL);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(X_WIFI_STA_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- X_WIFI_STA
		indigo_property_copy_values(X_WIFI_STA_PROPERTY, property, false);
		X_WIFI_STA_PROPERTY->state = INDIGO_BUSY_STATE;
		indigo_update_property(device, X_WIFI_STA_PROPERTY, NULL);
		indigo_set_timer(device, 0, focuser_wifi_sta_handler, NULL);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(X_CALIBRATE_F_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- X_CALIBRATE
		indigo_property_copy_values(X_CALIBRATE_F_PROPERTY, property, false);
		X_CALIBRATE_F_PROPERTY->state = INDIGO_BUSY_STATE;
		indigo_update_property(device, X_CALIBRATE_F_PROPERTY, NULL);
		indigo_set_timer(device, 0, focuser_calibrate_handler, NULL);
		return INDIGO_OK;
	}
	return indigo_focuser_change_property(device, client, property);
}

static indigo_result focuser_detach(indigo_device *device) {
	assert(device != NULL);
	if (IS_CONNECTED) {
		indigo_set_switch(CONNECTION_PROPERTY, CONNECTION_DISCONNECTED_ITEM, true);
		focuser_connection_handler(device);
	}
	indigo_release_property(X_STATE_PROPERTY);
	indigo_release_property(X_CONFIG_PROPERTY);
	indigo_release_property(X_LEDS_PROPERTY);
	indigo_release_property(X_WIFI_PROPERTY);
	indigo_release_property(X_WIFI_AP_PROPERTY);
	indigo_release_property(X_WIFI_STA_PROPERTY);
	indigo_release_property(X_RUNPRESET_L_PROPERTY);
	indigo_release_property(X_RUNPRESET_M_PROPERTY);
	indigo_release_property(X_RUNPRESET_S_PROPERTY);
	indigo_release_property(X_RUNPRESET_1_PROPERTY);
	indigo_release_property(X_RUNPRESET_2_PROPERTY);
	indigo_release_property(X_RUNPRESET_3_PROPERTY);
	indigo_release_property(X_RUNPRESET_PROPERTY);
	indigo_release_property(X_HOLD_CURR_PROPERTY);
	indigo_release_property(X_CALIBRATE_F_PROPERTY);
	pthread_mutex_destroy(&PRIVATE_DATA->mutex);
	INDIGO_DEVICE_DETACH_LOG(DRIVER_NAME, device->name);
	return indigo_focuser_detach(device);
}

// -------------------------------------------------------------------------------- INDIGO rotator device implementation

static indigo_result rotator_attach(indigo_device *device) {
	assert(device != NULL);
	assert(PRIVATE_DATA != NULL);
	if (indigo_rotator_attach(device, DRIVER_NAME, DRIVER_VERSION) == INDIGO_OK) {
		// -------------------------------------------------------------------------------- X_CALIBRATE
		X_CALIBRATE_R_PROPERTY = indigo_init_switch_property(NULL, device->name, "X_CALIBRATE_A", FOCUSER_ADVANCED_GROUP, "Calibrate rotator", INDIGO_OK_STATE, INDIGO_RW_PERM, INDIGO_ONE_OF_MANY_RULE, 1);
		if (X_CALIBRATE_R_PROPERTY == NULL) {
			return INDIGO_FAILED;
		}
		indigo_init_switch_item(X_CALIBRATE_R_START_ITEM, "START", "Start", false);
		// --------------------------------------------------------------------------------
		ROTATOR_ON_POSITION_SET_PROPERTY->hidden = true;
		// --------------------------------------------------------------------------------
		INDIGO_DEVICE_ATTACH_LOG(DRIVER_NAME, device->name);
		return indigo_rotator_enumerate_properties(device, NULL, NULL);
	}
	return INDIGO_FAILED;
}

static void rotator_connection_handler(indigo_device *device) {
	char response[8 * 1024];
	jsmntok_t tokens[1024];
	char *text;
	if (CONNECTION_CONNECTED_ITEM->sw.value) {
		bool result = true;
		if (PRIVATE_DATA->device_count++ == 0) {
			result = primaluce_open(device->master_device);
		}
		if (result) {
			if (primaluce_command(device, "{\"req\":{\"set\": {\"ARCO\":1}}}}", response, sizeof(response), tokens, 1024)) {
				if (primaluce_command(device, "{\"req\":{\"get\": \"\"}}}", response, sizeof(response), tokens, 1024)) {
					if ((text = get_string(response, tokens, GET_MOT2_ERROR)) && *text) {
						indigo_send_message(device, "ERROR: %s", text);
					}
					if (get_number(response, tokens, GET_CALRESTART_MOT2)) {
						indigo_send_message(device, "ERROR: ARCO needs calibration");
					}
				}
				PRIVATE_DATA->has_abs_pos = getToken(response, tokens, 0, GET_MOT2_ABS_POS) != -1;
				ROTATOR_POSITION_ITEM->number.value = ROTATOR_POSITION_ITEM->number.target = get_number(response, tokens, PRIVATE_DATA->has_abs_pos ? GET_MOT2_ABS_POS : GET_MOT2_ABS_POS_DEG);
				indigo_define_property(device, X_CALIBRATE_R_PROPERTY, NULL);
				CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
			} else {
				CONNECTION_PROPERTY->state = INDIGO_ALERT_STATE;
				PRIVATE_DATA->device_count--;
				indigo_set_switch(CONNECTION_PROPERTY, CONNECTION_DISCONNECTED_ITEM, true);
			}
		} else {
			CONNECTION_PROPERTY->state = INDIGO_ALERT_STATE;
			PRIVATE_DATA->device_count--;
			indigo_set_switch(CONNECTION_PROPERTY, CONNECTION_DISCONNECTED_ITEM, true);
		}
	} else {
		if (PRIVATE_DATA->handle != NULL) {
			if (primaluce_command(device, "{\"req\":{\"set\": {\"ARCO\":0}}}}", response, sizeof(response), tokens, 1024)) {
			}
			indigo_cancel_timer_sync(device, &PRIVATE_DATA->timer);
			indigo_delete_property(device, X_CALIBRATE_R_PROPERTY, NULL);
			INDIGO_DRIVER_LOG(DRIVER_NAME, "Disconnected");
			if (--PRIVATE_DATA->device_count == 0) {
				primaluce_close(device);
			}
		}
		CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
	}
	indigo_rotator_change_property(device, NULL, CONNECTION_PROPERTY);
}

static void rotator_position_handler(indigo_device *device) {
	char command[1024];
	char response[1024];
	jsmntok_t tokens[128];
	snprintf(command, sizeof(command), "{\"req\":{\"cmd\":{\"MOT2\":{\"MOVE_ABS\":{\"DEG\":%g}}}}}", FOCUSER_POSITION_ITEM->number.target);
	if (!primaluce_command(device, command, response, sizeof(response), tokens, 128)) {
		ROTATOR_POSITION_PROPERTY->state = INDIGO_ALERT_STATE;
		indigo_update_property(device, ROTATOR_POSITION_PROPERTY, NULL);
		return;
	}
	char *state = get_string(response, tokens, CMD_MOT2_STEP);
	if (state == NULL || strcmp(state, "done")) {
		ROTATOR_POSITION_PROPERTY->state = INDIGO_ALERT_STATE;
		indigo_update_property(device, ROTATOR_POSITION_PROPERTY, NULL);
		return;
	}
	char *get_pos_command = PRIVATE_DATA->has_abs_pos ? "{\"req\":{\"get\":{\"MOT2\":{\"ABS_POS\":\"DEG\",\"STATUS\":\"\"}}}}" : "{\"req\":{\"get\":{\"MOT2\":{\"ABS_POS_DEG\":\"\",\"STATUS\":\"\"}}}}";
	while (true) {
		if (primaluce_command(device, get_pos_command, response, sizeof(response), tokens, 128)) {
			ROTATOR_POSITION_ITEM->number.value = get_number(response, tokens, PRIVATE_DATA->has_abs_pos ? GET_MOT2_ABS_POS : GET_MOT2_ABS_POS_DEG);
			if (!strcmp(get_string(response, tokens, GET_MOT1_MST), "stop")) {
				break;
			}
			indigo_update_property(device, ROTATOR_POSITION_PROPERTY, NULL);
		}
	}
	for (int i = 0; i < 10; i++) {
		indigo_usleep(100000);
		if (primaluce_command(device, get_pos_command, response, sizeof(response), tokens, 128)) {
			ROTATOR_POSITION_ITEM->number.value = get_number(response, tokens, PRIVATE_DATA->has_abs_pos ? GET_MOT2_ABS_POS : GET_MOT2_ABS_POS_DEG);
			indigo_update_property(device, ROTATOR_POSITION_PROPERTY, NULL);
		}
		if (ROTATOR_POSITION_ITEM->number.target == ROTATOR_POSITION_ITEM->number.value) {
			break;
		}
	}
	ROTATOR_POSITION_PROPERTY->state = INDIGO_OK_STATE;
	indigo_update_property(device, ROTATOR_POSITION_PROPERTY, NULL);
}

static void rotator_abort_handler(indigo_device *device) {
	char response[1024];
	jsmntok_t tokens[128];
	ROTATOR_ABORT_MOTION_ITEM->sw.value = false;
	if (!primaluce_command(device, "{\"req\":{\"cmd\":{\"MOT2\":{\"MOT_STOP\":\"\"}}}}", response, sizeof(response), tokens, 128)) {
		ROTATOR_ABORT_MOTION_PROPERTY->state = INDIGO_ALERT_STATE;
		indigo_update_property(device, ROTATOR_ABORT_MOTION_PROPERTY, NULL);
		return;
	}
	char *state = get_string(response, tokens, CMD_MOT2_MOT_STOP);
	if (state == NULL || strcmp(state, "done")) {
		ROTATOR_ABORT_MOTION_PROPERTY->state = INDIGO_ALERT_STATE;
		indigo_update_property(device, ROTATOR_ABORT_MOTION_PROPERTY, NULL);
		return;
	}
	ROTATOR_ABORT_MOTION_PROPERTY->state = INDIGO_OK_STATE;
	indigo_update_property(device, ROTATOR_ABORT_MOTION_PROPERTY, NULL);
}

static void focuser_calibrate_a_handler(indigo_device *device) {
	char response[1024];
	jsmntok_t tokens[128];
	if (X_CALIBRATE_F_START_ITEM->sw.value) {
		X_CALIBRATE_F_START_ITEM->sw.value = false;
		if (!primaluce_command(device, "{\"req\":{\"cmd\": {\"MOT2\": {\"CAL_STATUS\":\"exec\"}}}}", response, sizeof(response), tokens, 128)) {
			X_CALIBRATE_R_PROPERTY->state = INDIGO_ALERT_STATE;
			indigo_update_property(device, X_CALIBRATE_R_PROPERTY, NULL);
		}
		char *state = get_string(response, tokens, CMD_MOT2_CAL_STATUS);
		if (state == NULL || strcmp(state, "done")) {
			X_CALIBRATE_R_PROPERTY->state = INDIGO_ALERT_STATE;
			indigo_update_property(device, X_CALIBRATE_R_PROPERTY, NULL);
			return;
		}
		char *get_pos_command = PRIVATE_DATA->has_abs_pos ? "{\"req\":{\"get\":{\"MOT2\":{\"ABS_POS\":\"DEG\",\"CAL_STATUS\":\"\"}}}}" : "{\"req\":{\"get\":{\"MOT2\":{\"ABS_POS_DEG\":\"\",\"CAL_STATUS\":\"\"}}}}";
		while (true) {
			if (primaluce_command(device, get_pos_command, response, sizeof(response), tokens, 128)) {
				ROTATOR_POSITION_ITEM->number.value = get_number(response, tokens, PRIVATE_DATA->has_abs_pos ? GET_MOT2_ABS_POS : GET_MOT2_ABS_POS_DEG);
				if (!strcmp(get_string(response, tokens, GET_MOT1_MST), "stop")) {
					break;
				}
				indigo_update_property(device, ROTATOR_POSITION_PROPERTY, NULL);
			}
		}
		
	}
	X_CALIBRATE_F_PROPERTY->state = INDIGO_OK_STATE;
	indigo_update_property(device, X_CALIBRATE_F_PROPERTY, NULL);
}

static indigo_result rotator_change_property(indigo_device *device, indigo_client *client, indigo_property *property) {
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
		indigo_set_timer(device, 0, rotator_connection_handler, NULL);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(ROTATOR_POSITION_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- ROTATOR_POSITION
		indigo_property_copy_values(ROTATOR_POSITION_PROPERTY, property, false);
		ROTATOR_POSITION_PROPERTY->state = INDIGO_BUSY_STATE;
		indigo_update_property(device, ROTATOR_POSITION_PROPERTY, NULL);
		indigo_set_timer(device, 0, rotator_position_handler, NULL);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(ROTATOR_ABORT_MOTION_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- ROTATOR_ABORT_MOTION
		indigo_property_copy_values(ROTATOR_ABORT_MOTION_PROPERTY, property, false);
		ROTATOR_ABORT_MOTION_PROPERTY->state = INDIGO_BUSY_STATE;
		indigo_update_property(device, ROTATOR_ABORT_MOTION_PROPERTY, NULL);
		indigo_set_timer(device, 0, rotator_abort_handler, NULL);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(X_CALIBRATE_R_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- X_CALIBRATE_A
		indigo_property_copy_values(X_CALIBRATE_R_PROPERTY, property, false);
		X_CALIBRATE_R_PROPERTY->state = INDIGO_BUSY_STATE;
		indigo_update_property(device, X_CALIBRATE_R_PROPERTY, NULL);
		indigo_set_timer(device, 0, focuser_calibrate_a_handler, NULL);
		return INDIGO_OK;
	}
	return indigo_rotator_change_property(device, client, property);
}

static indigo_result rotator_detach(indigo_device *device) {
	assert(device != NULL);
	if (IS_CONNECTED) {
		indigo_set_switch(CONNECTION_PROPERTY, CONNECTION_DISCONNECTED_ITEM, true);
		rotator_connection_handler(device);
	}
	indigo_release_property(X_CALIBRATE_R_PROPERTY);
	INDIGO_DEVICE_DETACH_LOG(DRIVER_NAME, device->name);
	return indigo_rotator_detach(device);
}

// -------------------------------------------------------------------------------- INDIGO driver implementation

indigo_result indigo_focuser_primaluce(indigo_driver_action action, indigo_driver_info *info) {
	static indigo_driver_action last_action = INDIGO_DRIVER_SHUTDOWN;
	static primaluce_private_data *private_data = NULL;
	static indigo_device *focuser = NULL;
	static indigo_device *rotator = NULL;
	
	static indigo_device focuser_template = INDIGO_DEVICE_INITIALIZER(
		"PrimaluceLab Focuser",
		focuser_attach,
		focuser_enumerate_properties,
		focuser_change_property,
		NULL,
		focuser_detach
		);
	
	static indigo_device rotator_template = INDIGO_DEVICE_INITIALIZER(
		"PrimaluceLab Rotator",
		rotator_attach,
		indigo_rotator_enumerate_properties,
		rotator_change_property,
		NULL,
		rotator_detach
		);
	
	static indigo_device_match_pattern patterns[1] = { 0 };
	strcpy(patterns[0].product_string, "CP2102N");
	INDIGO_REGISER_MATCH_PATTERNS(focuser_template, patterns, 1);


	SET_DRIVER_INFO(info, "PrimaluceLab Focuser/Rotator", __FUNCTION__, DRIVER_VERSION, false, last_action);
	
	if (action == last_action) {
		return INDIGO_OK;
	}
	
	switch (action) {
		case INDIGO_DRIVER_INIT:
			last_action = action;
			private_data = indigo_safe_malloc(sizeof(primaluce_private_data));
			focuser = indigo_safe_malloc_copy(sizeof(indigo_device), &focuser_template);
			focuser->private_data = private_data;
			focuser->master_device = focuser;
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
