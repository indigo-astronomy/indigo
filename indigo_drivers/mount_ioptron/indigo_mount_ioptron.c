// Copyright (c) 2018-2026 CloudMakers, s. r. o.
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

// This file generated from indigo_mount_ioptron.driver

#pragma mark - Includes

#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <assert.h>
#include <pthread.h>
#include <indigo/indigo_driver_xml.h>
#include <indigo/indigo_mount_driver.h>
#include <indigo/indigo_align.h>
#include <indigo/indigo_guider_driver.h>
#include <indigo/indigo_uni_io.h>

#include "indigo_mount_ioptron.h"

#pragma mark - Common definitions

#define DRIVER_VERSION       0x0300002D
#define DRIVER_NAME          "indigo_mount_ioptron"
#define DRIVER_LABEL         "iOptron Mount"
#define MOUNT_DEVICE_NAME    "iOptron Mount"
#define GUIDER_DEVICE_NAME   "iOptron Mount (guider)"
#define PRIVATE_DATA         ((ioptron_private_data *)device->private_data)

//+ define

#define RA_MIN_DIF           0.1
#define DEC_MIN_DIF          0.1

typedef enum {
	UNKNOWN = 0,
	HC_8406,
	HC_8407,
	V1_0,
	V2_0,
	V2_5,
	V3_0
} protocol_type;

typedef struct {
	unsigned product;
	char *min_firmware;
	char *model;
	protocol_type protocol;
	bool has_encoders;
	bool can_park;
	bool can_search_home;
} mount_type;

static mount_type PRODUCTS[] = {
	// firmware 210101+ -> 3.1
	{   26, "210101", "CEM26", V3_0, true, false, false  },
	{   27, "210101", "CEM26EC", V3_0, true, false, false  },

	// firmware 171001+ -> 3.0
	{   45, "171001", "iEQ45Pro", V3_0, false, true, false },
	{   46, "171001", "iEQ45ProAA", V3_0, false, true, false },
	{   60, "171001", "CEM60", V3_0, false, true, true },
	{   61, "171001", "CEM60EC", V3_0, true, true, true },
	{   40, "210101", "CEM40", V2_5, false, true, false },
	{   41, "210101", "CEM40EC", V2_5, true, true, false },
	{   70, "210101", "CEM70", V3_0, false, false, false  },
	{   71, "210101", "CEM70EC", V3_0, true, false, false  },


	// firmware 161101+ -> 2.5
	{   45, "161101", "iEQ45Pro", V2_5, false, true, false },
	{   46, "161101", "iEQ45ProAA", V2_5, false, true, false },
	{   60, "161101", "CEM60", V1_0, false, true, true },
	{   61, "161101", "CEM60EC", V1_0, true, true, true },
	
	// fallback -> 2.5
	{   40, NULL, "CEM40", V2_5, false, true, true },
	{   41, NULL, "CEM40EC", V2_5, true, true, true },
	{   30, NULL, "iEQ30Pro", V2_5, false, true, false },
	{   25, NULL, "CEM25", V2_5, false, false, false  },
	{   26, NULL, "CEM25EC", V2_5, false, false, false  },
	{   11, NULL, "SmartEQPro+", V2_5, false, false, false  },
	{ 5035, NULL, "AZMountPro", V2_5, false, false, false  },
	{   10, NULL, "CubeII", V2_5, false, false, false  },
	{ 5010, NULL, "CubeIIAA", V2_5, false, false, false  },

	// firmware 140807 -> 2.0
	{   45, "140807", "iEQ45Pro", V2_0, false, true, false },
	{   46, "140807", "iEQ45ProAA", V2_0, false, true, false },
	{   60, "140807", "CEM60", V2_0, false, true, true },
	{   61, "140807", "CEM60EC", V2_0, true, true, true },
	
	// fallback -> 1.0
	{   45, NULL, "iEQ45Pro", V1_0, false, true, false },
	{   46, NULL, "iEQ45ProAA", V1_0, false, true, false },
	{   60, NULL, "CEM60", V1_0, false, true, false },
	{   61, NULL, "CEM60EC", V1_0, true, true, false },
	
	// firmware independent
	{ 8407, NULL, "iEQ45/iEQ30", HC_8407, false, true, false },
	{ 8497, NULL, "iEQ45 AA", HC_8407, false, true, false },
	{ 8408, NULL, "ZEQ25", HC_8407, false, true, false },
	{ 8498, NULL, "SmartEQ", HC_8407, false, true, false },
	{   70, NULL, "CEM70", V3_0, false, false, true  },
	{   71, NULL, "CEM70EC", V3_0, true, false, true  },
	{  120, NULL, "CEM120", V3_0, false, true, true },
	{  121, NULL, "CEM120EC", V3_0, true, true, true },
	{  122, NULL, "CEM120EC2", V3_0, false, true, true },
	{ 9035, NULL, "AZMountProSM", V3_0, false, false, false },
	{   28, NULL, "GEM28", V3_0, false, true, false },
	{   29, NULL, "GEM28EC", V3_0, true, true, false },
	{ 5045, NULL, "iEQ45ProAA", V3_0, false, true, false },
	{   43, NULL, "GEM45", V3_0, false, true, true },
	{   44, NULL, "GEM45EC", V3_0, true, true, true },
	{   12, NULL, "HEM26", V3_0, false, true, false },
	{   15, NULL, "HEM15", V3_0, false, true, false },
	{   35, NULL, "HAZ31", V3_0, false, true, false },
	{ 8035, NULL, "HAZ31SM", V3_0, false, true, false },
	{   52, NULL, "HAZ46", V3_0, false, true, false },
	{ 8052, NULL, "HAZ46SM", V3_0, false, true, false },
	{   33, NULL, "HAE29", V3_0, false, true, false },
	{   34, NULL, "HAE29AA", V3_0, false, true, false },
	{ 8033, NULL, "HAE29SM", V3_0, false, true, false },
	{ 8034, NULL, "HAE29AASM", V3_0, false, true, false },
	{   50, NULL, "HAE43", V3_0, false, true, false },
	{   51, NULL, "HAE43AA", V3_0, false, true, false },
	{ 8050, NULL, "HAE43SM", V3_0, false, true, false },
	{ 8051, NULL, "HAE43AASM", V3_0, false, true, false },
	{    0, NULL, "", V3_0, false, false, false },
};

//- define

#pragma mark - Property definitions

#define MOUNT_MERIDIAN_HANDLING_PROPERTY      (PRIVATE_DATA->mount_meridian_handling_property)
#define MOUNT_MERIDIAN_STOP_ITEM              (MOUNT_MERIDIAN_HANDLING_PROPERTY->items + 0)
#define MOUNT_MERIDIAN_FLIP_ITEM              (MOUNT_MERIDIAN_HANDLING_PROPERTY->items + 1)

#define MOUNT_MERIDIAN_HANDLING_PROPERTY_NAME "MOUNT_MERIDIAN_HANDLING"
#define MOUNT_MERIDIAN_STOP_ITEM_NAME         "STOP"
#define MOUNT_MERIDIAN_FLIP_ITEM_NAME         "FLIP"

#define MOUNT_MERIDIAN_LIMIT_PROPERTY      (PRIVATE_DATA->mount_meridian_limit_property)
#define MOUNT_MERIDIAN_LIMIT_ITEM          (MOUNT_MERIDIAN_LIMIT_PROPERTY->items + 0)

#define MOUNT_MERIDIAN_LIMIT_PROPERTY_NAME "MOUNT_MERIDIAN_LIMIT"
#define MOUNT_MERIDIAN_LIMIT_ITEM_NAME     "LIMIT"

#define MOUNT_PROTOCOL_PROPERTY        (PRIVATE_DATA->mount_protocol_property)
#define PROTOCOL_AUTO_ITEM             (MOUNT_PROTOCOL_PROPERTY->items + 0)
#define PROTOCOL_8406_ITEM             (MOUNT_PROTOCOL_PROPERTY->items + 1)
#define PROTOCOL_8407_ITEM             (MOUNT_PROTOCOL_PROPERTY->items + 2)
#define PROTOCOL_0100_ITEM             (MOUNT_PROTOCOL_PROPERTY->items + 3)
#define PROTOCOL_0200_ITEM             (MOUNT_PROTOCOL_PROPERTY->items + 4)
#define PROTOCOL_0205_ITEM             (MOUNT_PROTOCOL_PROPERTY->items + 5)
#define PROTOCOL_0300_ITEM             (MOUNT_PROTOCOL_PROPERTY->items + 6)

#define MOUNT_PROTOCOL_PROPERTY_NAME   "PROTOCOL_VERSION"
#define PROTOCOL_AUTO_ITEM_NAME        "AUTO"
#define PROTOCOL_8406_ITEM_NAME        "8406"
#define PROTOCOL_8407_ITEM_NAME        "8407"
#define PROTOCOL_0100_ITEM_NAME        "0100"
#define PROTOCOL_0200_ITEM_NAME        "0200"
#define PROTOCOL_0205_ITEM_NAME        "0205"
#define PROTOCOL_0300_ITEM_NAME        "0300"

#pragma mark - Private data definition

typedef struct {
	int count;
	indigo_uni_handle *handle;
	indigo_property *mount_meridian_handling_property;
	indigo_property *mount_meridian_limit_property;
	indigo_property *mount_protocol_property;
	//+ data
	int last_tracking_rate, last_slew_rate;
	char last_motion_ra, last_motion_dec;
	double last_custom_tracking_rate;
	char lastUTC[INDIGO_VALUE_SIZE];
	unsigned product;
	protocol_type protocol;
	mount_type *type;
	bool has_encoders;
	bool can_park;
	bool can_search_home;
	char response[128];
	long time_difference;
	int utc_offset;
	bool slewing, tracking, parked, homed;
	//- data
} ioptron_private_data;

#pragma mark - Low level code

//+ code

static bool ioptron_validate_handle(indigo_device *device);

static bool ioptron_no_reply_command(indigo_device *device, char *command, ...) {
	if (!ioptron_validate_handle(device)) {
		return false;
	}
	long result = indigo_uni_discard(PRIVATE_DATA->handle);
	if (result >= 0) {
		va_list args;
		va_start(args, command);
		result = indigo_uni_vprintf(PRIVATE_DATA->handle, command, args);
		va_end(args);
	}
	if (result >= 0) {
		indigo_usleep(50000);
	}
	return result >= 0;
}

static bool ioptron_simple_reply_command(indigo_device *device, char *command, ...) {
	if (!ioptron_validate_handle(device)) {
		return false;
	}
	long result = indigo_uni_discard(PRIVATE_DATA->handle);
	if (result >= 0) {
		va_list args;
		va_start(args, command);
		result = indigo_uni_vprintf(PRIVATE_DATA->handle, command, args);
		va_end(args);
	}
	if (result >= 0) {
		if (!strcmp(command, ":MountInfo#")) {
			result = indigo_uni_read_section(PRIVATE_DATA->handle, PRIVATE_DATA->response, 4, "", "", INDIGO_DELAY(1));
		} else {
			result = indigo_uni_read_section(PRIVATE_DATA->handle, PRIVATE_DATA->response, 1, "", "", INDIGO_DELAY(1));
		}
	}
	if (result >= 0) {
		indigo_usleep(50000);
	}
	return result >= 0;
}

static bool ioptron_command(indigo_device *device, char *command, ...) {
	if (!ioptron_validate_handle(device)) {
		return false;
	}
	long result = indigo_uni_discard(PRIVATE_DATA->handle);
	if (result >= 0) {
		va_list args;
		va_start(args, command);
		result = indigo_uni_vprintf(PRIVATE_DATA->handle, command, args);
		va_end(args);
	}
	if (result >= 0) {
		if (!strcmp(command, ":V#")) {
			result = indigo_uni_read_section2(PRIVATE_DATA->handle, PRIVATE_DATA->response, sizeof(PRIVATE_DATA->response), "#", "#", INDIGO_DELAY(0.1), INDIGO_DELAY(0.1));
		} else {
			result = indigo_uni_read_section2(PRIVATE_DATA->handle, PRIVATE_DATA->response, sizeof(PRIVATE_DATA->response), "#", "#", INDIGO_DELAY(1), INDIGO_DELAY(0.1));
		}
	}
	if (result > 0) {
		indigo_usleep(50000);
	}
	return result > 0;
}

static bool ioptron_open(indigo_device *device) {
	char *name = DEVICE_PORT_ITEM->text.value;
	if (!indigo_uni_is_url(name, "ieq")) {
		if (DEVICE_BAUDRATE_ITEM->text.value[0] != '\0') {
			PRIVATE_DATA->handle = indigo_uni_open_serial_with_config(name, DEVICE_BAUDRATE_ITEM->text.value, INDIGO_LOG_DEBUG);
			if (!ioptron_simple_reply_command(device, ":MountInfo#") || strlen(PRIVATE_DATA->response) < 4) {
				indigo_uni_close(&PRIVATE_DATA->handle);
			}
		} else {
			PRIVATE_DATA->handle = indigo_uni_open_serial_with_speed(name, 9600, INDIGO_LOG_DEBUG);
			if (PRIVATE_DATA->handle != NULL) {
				bool reopenAt115200 = true;
				if (ioptron_command(device, ":V#") && *PRIVATE_DATA->response == 'V') {
					reopenAt115200 = false;
				} else if (ioptron_simple_reply_command(device, ":MountInfo#") && strlen(PRIVATE_DATA->response) == 4) {
					reopenAt115200 = false;
				}
				if (reopenAt115200) {
					indigo_uni_close(&PRIVATE_DATA->handle);
					PRIVATE_DATA->handle = indigo_uni_open_serial_with_speed(name, 115200, INDIGO_LOG_DEBUG);
					if (!ioptron_simple_reply_command(device, ":MountInfo#") || strlen(PRIVATE_DATA->response) != 4) {
						indigo_uni_close(&PRIVATE_DATA->handle);
					}
				}
			}
		}
	} else {
		PRIVATE_DATA->handle = indigo_uni_open_url(name, 4030, INDIGO_TCP_HANDLE, INDIGO_LOG_DEBUG);
	}
	return PRIVATE_DATA->handle != NULL;
}

static void ioptron_close(indigo_device *device) {
	indigo_uni_close(&PRIVATE_DATA->handle);
}

static bool ioptron_validate_handle(indigo_device *device) {
	if (PRIVATE_DATA->handle == NULL) {
		return false;
	}
	if (!indigo_uni_is_valid(PRIVATE_DATA->handle)) {
		ioptron_close(device);
		indigo_execute_handler(device->master_device, indigo_disconnect_slave_devices);
		return false;
	}
	return true;
}

// ---------------------------------------------------------------------  low level mount commands

static bool ioptron_detect_mount(indigo_device *device) {
	PRIVATE_DATA->protocol = UNKNOWN;
	PRIVATE_DATA->product = 0;
	PRIVATE_DATA->has_encoders = false;
	PRIVATE_DATA->can_park = true;
	indigo_set_text_item_value(INFO_DEVICE_MODEL_ITEM, "N/A");
	indigo_set_text_item_value(INFO_DEVICE_FW_REVISION_ITEM, "N/A");
	if (PROTOCOL_8406_ITEM->sw.value) {
		PRIVATE_DATA->protocol = HC_8406;
		indigo_set_text_item_value(INFO_DEVICE_MODEL_ITEM, "HC8406 mount");
	} else if (PROTOCOL_8407_ITEM->sw.value) {
		PRIVATE_DATA->protocol = HC_8407;
		indigo_set_text_item_value(INFO_DEVICE_MODEL_ITEM, "HC8407 mount");
	} else if (PROTOCOL_0100_ITEM->sw.value) {
		PRIVATE_DATA->protocol = V1_0;
		indigo_set_text_item_value(INFO_DEVICE_MODEL_ITEM, "Protocol 1.0 mount");
	} else if (PROTOCOL_0200_ITEM->sw.value) {
		PRIVATE_DATA->protocol = V2_0;
		indigo_set_text_item_value(INFO_DEVICE_MODEL_ITEM, "Protocol 2.0 mount");
	} else if (PROTOCOL_0205_ITEM->sw.value) {
		PRIVATE_DATA->protocol = V2_5;
		indigo_set_text_item_value(INFO_DEVICE_MODEL_ITEM, "Protocol 2.5 mount");
	} else if (PROTOCOL_0300_ITEM->sw.value) {
		PRIVATE_DATA->protocol = V3_0;
		indigo_set_text_item_value(INFO_DEVICE_MODEL_ITEM, "Protocol 3.0 mount");
	}
	if (PRIVATE_DATA->protocol == UNKNOWN) {
		if (ioptron_simple_reply_command(device, ":MountInfo#")) {
			PRIVATE_DATA->protocol = UNKNOWN;
			PRIVATE_DATA->product = atoi(PRIVATE_DATA->response);
			if (ioptron_command(device, ":FW1#") && *PRIVATE_DATA->response) {
				PRIVATE_DATA->response[6] = 0;
				indigo_set_text_item_value(INFO_DEVICE_FW_REVISION_ITEM, PRIVATE_DATA->response);
				for (int i = 0; PRODUCTS[i].product; i++) {
					mount_type *type = PRODUCTS + i;
					if (type->product == PRIVATE_DATA->product && (type->min_firmware == NULL || strncmp(type->min_firmware, INFO_DEVICE_FW_REVISION_ITEM->text.value, 6) <= 0)) {
						indigo_set_text_item_value(INFO_DEVICE_MODEL_ITEM, type->model);
						PRIVATE_DATA->type = type;
						PRIVATE_DATA->protocol = type->protocol;
						PRIVATE_DATA->has_encoders = type->has_encoders;
						PRIVATE_DATA->can_park = type->can_park;
						PRIVATE_DATA->can_search_home = type->can_search_home;
						break;
					}
				}
			}
		} else {
			indigo_set_text_item_value(INFO_DEVICE_MODEL_ITEM, "HC8406");
			PRIVATE_DATA->protocol = HC_8406;
			PRIVATE_DATA->can_park = true;
		}
	} else if (PRIVATE_DATA->protocol >= HC_8407) {
		if (ioptron_command(device, ":FW1#") && *PRIVATE_DATA->response) {
			PRIVATE_DATA->response[6] = 0;
			indigo_set_text_item_value(INFO_DEVICE_FW_REVISION_ITEM, PRIVATE_DATA->response);
		}
	}
	if (PRIVATE_DATA->protocol == UNKNOWN) {
		if (strncmp("140807", INFO_DEVICE_FW_REVISION_ITEM->text.value, 6) >= 0) {
			INDIGO_DRIVER_DEBUG(DRIVER_NAME, "Unknown mount model (%04u %s), protocol 1.0 assumed", PRIVATE_DATA->product, INFO_DEVICE_FW_REVISION_ITEM->text.value);
			PRIVATE_DATA->protocol = V1_0;
		} else if (strncmp("161101", INFO_DEVICE_FW_REVISION_ITEM->text.value, 6) >= 0) {
			INDIGO_DRIVER_DEBUG(DRIVER_NAME, "Unknown mount model (%04u %s), protocol 2.0 assumed", PRIVATE_DATA->product, INFO_DEVICE_FW_REVISION_ITEM->text.value);
			PRIVATE_DATA->protocol = V2_0;
		} else if (ioptron_command(device, ":GLS#")) {
			if (strlen(PRIVATE_DATA->response) == 19) {
				INDIGO_DRIVER_DEBUG(DRIVER_NAME, "Unknown mount model (%04u %s), protocol 2.5 assumed", PRIVATE_DATA->product, INFO_DEVICE_FW_REVISION_ITEM->text.value);
				PRIVATE_DATA->protocol = V2_5;
			} else if (strlen(PRIVATE_DATA->response) == 23) {
				INDIGO_DRIVER_DEBUG(DRIVER_NAME, "Unknown mount model (%04u %s), protocol 3.0 assumed", PRIVATE_DATA->product, INFO_DEVICE_FW_REVISION_ITEM->text.value);
				PRIVATE_DATA->protocol = V3_0;
			}
		}
	} else {
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "Mount %s (%04d, %s, %s)", INFO_DEVICE_MODEL_ITEM->text.value, PRIVATE_DATA->product, PRIVATE_DATA->can_park ? "can park" : "can't park", PRIVATE_DATA->has_encoders ? "has encoders" : "no encoders");
	}
	if (PRIVATE_DATA->protocol != UNKNOWN) {
		indigo_update_property(device, INFO_PROPERTY, NULL);
		indigo_set_text_item_value(MOUNT_INFO_VENDOR_ITEM, "iOptron");
		indigo_set_text_item_value(MOUNT_INFO_MODEL_ITEM, indigo_get_text_item_value(INFO_DEVICE_MODEL_ITEM));
		indigo_set_text_item_value(MOUNT_INFO_FIRMWARE_ITEM, indigo_get_text_item_value(INFO_DEVICE_FW_REVISION_ITEM));
		return true;
	}
	return false;
}

static bool ioptron_set_utc(indigo_device *device, time_t secs, int utc_offset) {
	PRIVATE_DATA->time_difference = time(NULL) - secs;
	time_t seconds = secs + utc_offset * 3600;
	struct tm tm;
	indigo_gmtime(&seconds, &tm);
	if (PRIVATE_DATA->protocol == HC_8406) {
		if (!ioptron_command(device, ":SL %02d:%02d:%02d#", tm.tm_hour, tm.tm_min, tm.tm_sec) || *PRIVATE_DATA->response != '1') {
			MOUNT_SET_HOST_TIME_PROPERTY->state = INDIGO_ALERT_STATE;
		} else {
			if (!ioptron_simple_reply_command(device, ":SC %02d/%02d/%02d#", tm.tm_mon + 1, tm.tm_mday, tm.tm_year % 100)) {
				MOUNT_SET_HOST_TIME_PROPERTY->state = INDIGO_ALERT_STATE;
			} else {
				if (!ioptron_simple_reply_command(device, ":SG %+03d#", utc_offset) || *PRIVATE_DATA->response != '1') {
					MOUNT_SET_HOST_TIME_PROPERTY->state = INDIGO_ALERT_STATE;
				} else {
					return true;
				}
			}
		}
	} else if (PRIVATE_DATA->protocol == HC_8407 || PRIVATE_DATA->protocol == V1_0) {
		if (!ioptron_simple_reply_command(device, ":SL %02d:%02d:%02d#", tm.tm_hour, tm.tm_min, tm.tm_sec) || *PRIVATE_DATA->response != '1') {
			MOUNT_SET_HOST_TIME_PROPERTY->state = INDIGO_ALERT_STATE;
		} else {
			if (!ioptron_simple_reply_command(device, ":SC %02d/%02d/%02d#", tm.tm_mon + 1, tm.tm_mday, tm.tm_year % 100) || *PRIVATE_DATA->response != '1') {
				MOUNT_SET_HOST_TIME_PROPERTY->state = INDIGO_ALERT_STATE;
			} else {
				if (!ioptron_simple_reply_command(device, ":SG %+03d:00#", utc_offset) || *PRIVATE_DATA->response != '1') {
					MOUNT_SET_HOST_TIME_PROPERTY->state = INDIGO_ALERT_STATE;
				} else {
					if (!ioptron_simple_reply_command(device, ":SDS%d#", indigo_get_dst_state()) || *PRIVATE_DATA->response != '1') {
						MOUNT_SET_HOST_TIME_PROPERTY->state = INDIGO_ALERT_STATE;
					} else {
						return true;
					}
				}
			}
		}
	} else if (PRIVATE_DATA->protocol == V2_0 || PRIVATE_DATA->protocol == V2_5) {
		if (!ioptron_simple_reply_command(device, ":SL%02d%02d%02d#", tm.tm_hour, tm.tm_min, tm.tm_sec) || *PRIVATE_DATA->response != '1') {
			MOUNT_SET_HOST_TIME_PROPERTY->state = INDIGO_ALERT_STATE;
		} else {
			if (!ioptron_simple_reply_command(device, ":SC%02d%02d%02d#", tm.tm_year % 100, tm.tm_mon + 1, tm.tm_mday) || *PRIVATE_DATA->response != '1') {
				MOUNT_SET_HOST_TIME_PROPERTY->state = INDIGO_ALERT_STATE;
			} else {
				if (!ioptron_simple_reply_command(device, ":SG%+04d#", utc_offset * 60) || *PRIVATE_DATA->response != '1') {
					MOUNT_SET_HOST_TIME_PROPERTY->state = INDIGO_ALERT_STATE;
				} else {
					if (!ioptron_simple_reply_command(device, ":SDS%d#", indigo_get_dst_state()) || *PRIVATE_DATA->response != '1') {
						MOUNT_SET_HOST_TIME_PROPERTY->state = INDIGO_ALERT_STATE;
					} else {
						return true;
					}
				}
			}
		}
	} else if (PRIVATE_DATA->protocol == V3_0) {
		if (!ioptron_simple_reply_command(device, ":SUT%013llu#", (uint64_t)((JDNOW - JD2000) * 8.64e+7)) || *PRIVATE_DATA->response != '1') {
			MOUNT_SET_HOST_TIME_PROPERTY->state = INDIGO_ALERT_STATE;
		} else {
			if (!ioptron_simple_reply_command(device, ":SG%+04d#", utc_offset * 60) || *PRIVATE_DATA->response != '1') {
				MOUNT_SET_HOST_TIME_PROPERTY->state = INDIGO_ALERT_STATE;
			} else {
				if (!ioptron_simple_reply_command(device, ":SDS%d#", indigo_get_dst_state()) || *PRIVATE_DATA->response != '1') {
					MOUNT_SET_HOST_TIME_PROPERTY->state = INDIGO_ALERT_STATE;
				} else {
					return true;
				}
			}
		}
	}
	return false;
}

static bool ioptron_get_utc(indigo_device *device, time_t *secs, int *utc_offset) {
	struct tm tm;
	char sep;
	memset(&tm, 0, sizeof(tm));
	MOUNT_UTC_TIME_PROPERTY->state = INDIGO_ALERT_STATE;
	if (PRIVATE_DATA->protocol == HC_8406) {
		if (ioptron_command(device, ":GC#") && sscanf(PRIVATE_DATA->response, "%2d%c%2d%c%2d", &tm.tm_mon, &sep, &tm.tm_mday, &sep, &tm.tm_year) == 5) {
			if (ioptron_command(device, ":GL#") && sscanf(PRIVATE_DATA->response, "%2d:%2d:%2d", &tm.tm_hour, &tm.tm_min, &tm.tm_sec) == 3) {
				tm.tm_year += 100; // TODO: To be fixed in year 2100 :)
				tm.tm_mon -= 1;
				if (ioptron_command(device, ":GG#")) {
					if (PRIVATE_DATA->response[0] ==  'E') {
						*utc_offset = atoi(PRIVATE_DATA->response + 1);
					} else {
						*utc_offset = -atoi(PRIVATE_DATA->response + 1);
					}
					*secs = indigo_timegm(&tm) - *utc_offset * 3600;
					PRIVATE_DATA->time_difference = time(NULL) - *secs;
					return true;
				}
			}
		}
	} else if (PRIVATE_DATA->protocol == HC_8407 || PRIVATE_DATA->protocol == UNKNOWN) {
		if (ioptron_command(device, ":GC#") && sscanf(PRIVATE_DATA->response, "%2d%c%2d%c%2d", &tm.tm_mon, &sep, &tm.tm_mday, &sep, &tm.tm_year) == 5) {
			if (ioptron_command(device, ":GL#") && sscanf(PRIVATE_DATA->response, "%2d:%2d:%2d", &tm.tm_hour, &tm.tm_min, &tm.tm_sec) == 3) {
				tm.tm_year += 100; // TODO: To be fixed in year 2100 :)
				tm.tm_mon -= 1;
				if (ioptron_command(device, ":GG#")) {
					*utc_offset = atoi(PRIVATE_DATA->response);
					*secs = indigo_timegm(&tm) - *utc_offset * 3600;
					PRIVATE_DATA->time_difference = time(NULL) - *secs;
					return true;
				}
			}
		}
	} else if (PRIVATE_DATA->protocol == V1_0 || PRIVATE_DATA->protocol == V2_0 || PRIVATE_DATA->protocol == V2_5) {
		int offset;
		if (ioptron_command(device, ":GLT#") && sscanf(PRIVATE_DATA->response, "%4d%c%2d%2d%2d%2d%2d%2d", &offset, &sep, &tm.tm_year, &tm.tm_mon, &tm.tm_mday, &tm.tm_hour, &tm.tm_min, &tm.tm_sec) == 8) {
			tm.tm_year += 100; // TODO: To be fixed in year 2100 :)
			tm.tm_mon -= 1;
			*utc_offset = offset / 60;
			*secs = indigo_timegm(&tm) - *utc_offset * 3600;
			PRIVATE_DATA->time_difference = time(NULL) - *secs;
			return true;
		}
	} else if (PRIVATE_DATA->protocol == V3_0) {
		if (ioptron_command(device, ":GUT#")) {
			PRIVATE_DATA->response[4] = 0;
			int offset = atoi(PRIVATE_DATA->response);
			*utc_offset = offset / 60;
			double jd = atoll(PRIVATE_DATA->response + 5) / 8.64e+7 + JD2000;
			*secs = (time_t)((jd - DELTA_UT1_UTC - 2440587.5) * 86400.0);
			PRIVATE_DATA->time_difference = time(NULL) - *secs;
			return true;
		}
	}
	return false;
}

static bool ioptron_get_site(indigo_device *device, double *latitude, double *longitude) {
	if (PRIVATE_DATA->protocol == HC_8406 || PRIVATE_DATA->protocol == HC_8407 || PRIVATE_DATA->protocol == V1_0) {
		if (ioptron_command(device, ":Gt#")) {
			*latitude = indigo_stod(PRIVATE_DATA->response);
			if (ioptron_command(device, ":Gg#")) {
				*longitude = indigo_stod(PRIVATE_DATA->response);
				return true;
			}
		}		
	} else if (PRIVATE_DATA->protocol == V2_0) {
		if (ioptron_command(device, ":Gt#")) {
			*latitude = atol(PRIVATE_DATA->response) / 60.0 / 60.0;
			if (ioptron_command(device, ":Gg#")) {
				*longitude = atol(PRIVATE_DATA->response) / 60.0 / 60.0;
				if (*longitude < 0) {
					*longitude += 360;
				}
				return true;
			}
		}
	} else if (PRIVATE_DATA->protocol == V2_5) {
		if (ioptron_command(device, ":GLS#") && strlen(PRIVATE_DATA->response) == 19) {
			char val[16];
			memcpy(val, PRIVATE_DATA->response, 7);
			val[7] = 0;
			*longitude = atol(val) / 60.0 / 60.0;
			if (*longitude < 0) {
				*longitude += 360;
			}
			memcpy(val, PRIVATE_DATA->response + 7, 6);
			val[6] = 0;
			*latitude = atol(val) / 60.0 / 60.0 - 90;
			return true;
		}
	} else if (PRIVATE_DATA->protocol == V3_0) {
		if (ioptron_command(device, ":GLS#") && strlen(PRIVATE_DATA->response) == 23) {
			char val[16];
			memcpy(val, PRIVATE_DATA->response, 9);
			val[9] = 0;
			*longitude = atol(val) / 60.0 / 60.0 / 100.0;
			if (*longitude < 0) {
				*longitude += 360;
			}
			strncpy(val, PRIVATE_DATA->response + 9, 8);
			val[8] = 0;
			*latitude = atol(val) / 60.0 / 60.0 / 100.0 - 90;
			return true;
		}
	}
	return false;
}

static bool ioptron_set_site(indigo_device *device, double latitude, double longitude, double elevation) {
	if (longitude < 0) {
		longitude += 360;
	}
	if (PRIVATE_DATA->protocol == HC_8406 || PRIVATE_DATA->protocol == HC_8407 || PRIVATE_DATA->protocol == V1_0) {
		if (ioptron_simple_reply_command(device, ":St %s#", indigo_dtos(latitude, "%+03d*%02d:%02.0f")) && *PRIVATE_DATA->response == '1') {
			if (ioptron_simple_reply_command(device, ":Sg %s#", indigo_dtos(longitude, "%+04d*%02d:%02.0f")) && *PRIVATE_DATA->response == '1') {
				return true;
			}
		}
	} else if (PRIVATE_DATA->protocol == V2_0 || PRIVATE_DATA->protocol == V2_5) {
		if (ioptron_simple_reply_command(device, ":St%+07.0f#", latitude * 60 * 60) && *PRIVATE_DATA->response == '1') {
			if (ioptron_simple_reply_command(device, ":Sg%+07.0f#", longitude * 60 * 60) && *PRIVATE_DATA->response == '1') {
				return true;
			}
		}
	} else if (PRIVATE_DATA->protocol == V3_0) {
		if (ioptron_simple_reply_command(device, ":SLA%+09.0f#", latitude * 60 * 60 * 100) && *PRIVATE_DATA->response == '1') {
			if (ioptron_simple_reply_command(device, ":SLO%+09.0f#", longitude * 60 * 60 * 100) && *PRIVATE_DATA->response == '1') {
				return true;
			}
		}
	}
	return false;
}

static bool ioptron_park(indigo_device *device) {
	if (PRIVATE_DATA->protocol == HC_8406) {
		if (ioptron_simple_reply_command(device, ":PK#") && *PRIVATE_DATA->response == '1') {
			return true;
		}
	} else if (PRIVATE_DATA->protocol == HC_8407) {
		if (ioptron_simple_reply_command(device, ":MP1#") && *PRIVATE_DATA->response == '1') {
			return true;
		}
	} else if (PRIVATE_DATA->protocol == V1_0) {
		double ra = MOUNT_LST_TIME_ITEM->number.value;
		if (ioptron_simple_reply_command(device, ":Sr %s#", indigo_dtos(ra, "%02d:%02d:%02.0f")) && *PRIVATE_DATA->response == '1') {
			double dec = MOUNT_GEOGRAPHIC_COORDINATES_LATITUDE_ITEM->number.value >= 0 ? 90.0 : -90.0;
			if (ioptron_simple_reply_command(device, ":Sd %s#", indigo_dtos(dec, "%+03d*%02d:%02.0f")) && *PRIVATE_DATA->response == '1') {
				if (ioptron_simple_reply_command(device, ":MP1#") && *PRIVATE_DATA->response == '1') {
					return true;
				}
			}
		}
	} else if (PRIVATE_DATA->protocol == V2_0) {
		double ra = MOUNT_LST_TIME_ITEM->number.value;
		if (ioptron_simple_reply_command(device, ":Sr%08.0f#", ra * 60 * 60 * 1000) && *PRIVATE_DATA->response == '1') {
			double dec = MOUNT_GEOGRAPHIC_COORDINATES_LATITUDE_ITEM->number.value >= 0 ? 90.0 : -90.0;
			if (ioptron_simple_reply_command(device, ":Sd%+09.0f#", dec * 60 * 60 * 100) && *PRIVATE_DATA->response == '1') {
				if (ioptron_simple_reply_command(device, ":MP1#") && *PRIVATE_DATA->response == '1') {
					return true;
				}
			}
		}
	} else if (PRIVATE_DATA->protocol == V2_5 || PRIVATE_DATA->protocol == V3_0) {
		if (ioptron_simple_reply_command(device, ":MP1#") && *PRIVATE_DATA->response == '1') {
			return true;
		}
	}
	return false;
}

static bool ioptron_unpark(indigo_device *device) {
	if (PRIVATE_DATA->protocol >= HC_8407) {
		if (ioptron_simple_reply_command(device, ":MP0#") && *PRIVATE_DATA->response == '1') {
			return true;
		}
	}
	return false;
}

static bool ioptron_park_set_default(indigo_device *device) {
	if (PRIVATE_DATA->protocol == V2_5 || PRIVATE_DATA->protocol == V3_0) {
		if (ioptron_simple_reply_command(device, ":SPA%9d", 0 * 60 * 60 * 100) && *PRIVATE_DATA->response == '1') {
			double latitude = MOUNT_GEOGRAPHIC_COORDINATES_LATITUDE_ITEM->number.value;
			if (ioptron_simple_reply_command(device, ":SPH%8d", (int)(fabs(latitude) * 60 * 60 * 100)) && *PRIVATE_DATA->response == '1') {
				return true;
			}
		}
	}
	return false;
}

static bool ioptron_park_set_current(indigo_device *device) {
	if (PRIVATE_DATA->protocol == V2_5 || PRIVATE_DATA->protocol == V3_0) {
		if (ioptron_command(device, ":GAC#") && *PRIVATE_DATA->response == '+') {
			char alt[9], az[10];
			strncpy(alt, PRIVATE_DATA->response + 1, 8);
			alt[8] = 0;
			strncpy(az, PRIVATE_DATA->response + 9, 9);
			az[9] = 0;
			if (ioptron_simple_reply_command(device, ":SPH%s#", alt) && *PRIVATE_DATA->response == '1') {
				if (ioptron_simple_reply_command(device, ":SPA%s#", az) && *PRIVATE_DATA->response == '1') {
					return true;
				}
			}
		}
	}
	return false;
}

static bool ioptron_home(indigo_device *device) {
	if (PRIVATE_DATA->protocol >= HC_8406) {
		if (ioptron_simple_reply_command(device, ":MH#") && *PRIVATE_DATA->response == '1') {
			return true;
		}
	}
	return false;
}

static bool ioptron_search_home(indigo_device *device) {
	if (PRIVATE_DATA->protocol >= V2_0) {
		if (ioptron_simple_reply_command(device, ":MSH#") && *PRIVATE_DATA->response == '1') {
			return true;
		}
	}
	return false;
}

static bool ioptron_get_coordinates(indigo_device *device, double *ra, double *dec) {
	if (PRIVATE_DATA->protocol == HC_8406 || PRIVATE_DATA->protocol == HC_8407) {
		if (ioptron_command(device, ":GR#")) {
			*ra = indigo_stod(PRIVATE_DATA->response);
			if (ioptron_command(device, ":GD#")) {
				*dec = indigo_stod(PRIVATE_DATA->response);
				if (ioptron_command(device, ":pS#")) {
					if (PRIVATE_DATA->response[0] == 'E' || PRIVATE_DATA->response[0] == '0') {
						indigo_set_switch(MOUNT_SIDE_OF_PIER_PROPERTY, MOUNT_SIDE_OF_PIER_EAST_ITEM, true);
					} else if (PRIVATE_DATA->response[0] == 'W' || PRIVATE_DATA->response[0] == '1') {
						indigo_set_switch(MOUNT_SIDE_OF_PIER_PROPERTY, MOUNT_SIDE_OF_PIER_WEST_ITEM, true);
					}
					return true;
				}
			}
		}
	} else if (PRIVATE_DATA->protocol == V1_0) {
		long ra_raw, dec_raw;
		if (ioptron_command(device, ":GEC#") && sscanf(PRIVATE_DATA->response, "%10ld%9ld", &dec_raw, &ra_raw) == 2) {
			*ra = ra_raw / 100.0 / 60.0 / 60.0;
			*dec = dec_raw / 100.0 / 60.0 / 60.0;
			return true;
		}
	} else if (PRIVATE_DATA->protocol == V2_0 || PRIVATE_DATA->protocol == V2_5) {
		long ra_raw, dec_raw;
		if (ioptron_command(device, ":GEC#") && sscanf(PRIVATE_DATA->response, "%9ld%8ld", &dec_raw, &ra_raw) == 2) {
			*ra = ra_raw / 1000.0 / 60.0 / 60.0;
			*dec = dec_raw / 100.0 / 60.0 / 60.0;
			return true;
		}
	} else if (PRIVATE_DATA->protocol == V3_0) {
		long ra_raw, dec_raw;
		char side_of_pier, pointing_state;
		if (ioptron_command(device, ":GEP#") && sscanf(PRIVATE_DATA->response, "%9ld%9ld%c%c", &dec_raw, &ra_raw, &side_of_pier, &pointing_state) == 4) {
			*ra = ra_raw / 100.0 / 60.0 / 60.0 / 15;
			*dec = dec_raw / 100.0 / 60.0 / 60.0;
			if (side_of_pier == '0') {
				indigo_set_switch(MOUNT_SIDE_OF_PIER_PROPERTY, MOUNT_SIDE_OF_PIER_EAST_ITEM, true);
			} else if (side_of_pier == '1') {
				indigo_set_switch(MOUNT_SIDE_OF_PIER_PROPERTY, MOUNT_SIDE_OF_PIER_WEST_ITEM, true);
			}
			return true;
		}
	}
	return false;
}

static bool ioptron_set_tracking_rate(indigo_device *device, char tracking_rate, double custom_tracking_rate) {
	bool result;
	if (PRIVATE_DATA->protocol == HC_8406) {
		result = ioptron_simple_reply_command(device, ":STR%d#", tracking_rate) && *PRIVATE_DATA->response == '1';
		if (result) {
			PRIVATE_DATA->last_tracking_rate = tracking_rate;
		}
	} else {
		result = ioptron_simple_reply_command(device, ":RT%d#", tracking_rate) && *PRIVATE_DATA->response == '1';
		if (result) {
			PRIVATE_DATA->last_tracking_rate = tracking_rate;
			if (tracking_rate == 4 && PRIVATE_DATA->last_custom_tracking_rate != custom_tracking_rate) {
				if (PRIVATE_DATA->protocol == HC_8407 || PRIVATE_DATA->protocol == V1_0) {
					result = ioptron_simple_reply_command(device, ":RR %+8.4f#", custom_tracking_rate) && *PRIVATE_DATA->response == '1';
				} else if (PRIVATE_DATA->protocol == V2_0) {
					result = ioptron_simple_reply_command(device, ":RR%+8.4f#", custom_tracking_rate) && *PRIVATE_DATA->response == '1';
				} else if (PRIVATE_DATA->protocol >= V2_5) {
					result = ioptron_simple_reply_command(device, ":RR%05d#", (int)(custom_tracking_rate * 1e4)) && *PRIVATE_DATA->response == '1';
				}
				if (result) {
					PRIVATE_DATA->last_custom_tracking_rate = custom_tracking_rate;
				}
			}
		}
		result = result && ioptron_simple_reply_command(device, ":ST1#");
	}
	return result;
}

static bool ioptron_slew(indigo_device *device, double ra, double dec) {
	bool result = false;
	if (PRIVATE_DATA->protocol == HC_8406) {
		result = ioptron_simple_reply_command(device, ":Sr %s#", indigo_dtos(ra, "%02d:%02d:%04.1f")) && *PRIVATE_DATA->response == '1';
	} else if (PRIVATE_DATA->protocol == HC_8407 || PRIVATE_DATA->protocol == UNKNOWN || PRIVATE_DATA->protocol == V1_0) {
		result = ioptron_simple_reply_command(device, ":Sr %s#", indigo_dtos(ra, "%02d:%02d:%02.0f")) && *PRIVATE_DATA->response == '1';
	} else if (PRIVATE_DATA->protocol == V2_0 || PRIVATE_DATA->protocol == V2_5) {
		result = ioptron_simple_reply_command(device, ":Sr%08.0f#", ra * 60 * 60 * 1000) && *PRIVATE_DATA->response == '1';
	} else if (PRIVATE_DATA->protocol == V3_0) {
		result = ioptron_simple_reply_command(device, ":SRA%09.0f#", ra * 15 * 60 * 60 * 100) && *PRIVATE_DATA->response == '1';
	}
	if (result) {
		if (PRIVATE_DATA->protocol == HC_8406 || PRIVATE_DATA->protocol == HC_8407 || PRIVATE_DATA->protocol == UNKNOWN || PRIVATE_DATA->protocol == V1_0) {
			result= ioptron_simple_reply_command(device, ":Sd %s#", indigo_dtos(dec, "%+03d*%02d:%02.0f")) && *PRIVATE_DATA->response == '1';
		} else if (PRIVATE_DATA->protocol >= V2_0) {
			result = ioptron_simple_reply_command(device, ":Sd%+09.0f#", dec * 60 * 60 * 100) && *PRIVATE_DATA->response == '1';
		}
	}
	if (result) {
		if (PRIVATE_DATA->protocol == HC_8406) {
			result = ioptron_simple_reply_command(device, ":MS#") && *PRIVATE_DATA->response == '0';
		} else if (PRIVATE_DATA->protocol == HC_8407 || PRIVATE_DATA->protocol == V1_0 || PRIVATE_DATA->protocol == V2_0 || PRIVATE_DATA->protocol == V2_5) {
			result = ioptron_simple_reply_command(device, ":MS#") && *PRIVATE_DATA->response == '1';
		} else if (PRIVATE_DATA->protocol == V3_0) {
			result = ioptron_simple_reply_command(device, ":MS1#") && *PRIVATE_DATA->response == '1';
		}
	}
	return result;
}

static bool ioptron_sync(indigo_device *device, double ra, double dec) {
	bool result = false;
	if (PRIVATE_DATA->protocol == HC_8406) {
		result = ioptron_simple_reply_command(device, ":Sr %s#", indigo_dtos(ra, "%02d:%02d:%04.1f")) && *PRIVATE_DATA->response == '1';
	} else if (PRIVATE_DATA->protocol == HC_8407 || PRIVATE_DATA->protocol == V1_0) {
		result = ioptron_simple_reply_command(device, ":Sr %s#", indigo_dtos(ra, "%02d:%02d:%02.0f")) && *PRIVATE_DATA->response == '1';
	} else if (PRIVATE_DATA->protocol == V2_0 || PRIVATE_DATA->protocol == V2_5) {
		result = ioptron_simple_reply_command(device, ":Sr%08.0f#", ra * 60 * 60 * 1000) && *PRIVATE_DATA->response == '1';
	} else if (PRIVATE_DATA->protocol == V3_0) {
		result = ioptron_simple_reply_command(device, ":SRA%09.0f#", ra * 15 * 60 * 60 * 100) && *PRIVATE_DATA->response == '1';
	}
	if (result) {
		if (PRIVATE_DATA->protocol == HC_8406 || PRIVATE_DATA->protocol == HC_8407 || PRIVATE_DATA->protocol == UNKNOWN || PRIVATE_DATA->protocol == V1_0) {
			result = ioptron_simple_reply_command(device, ":Sd %s#", indigo_dtos(dec, "%+03d*%02d:%02.0f")) && *PRIVATE_DATA->response == '1';
		} else if (PRIVATE_DATA->protocol >= V2_0) {
			result = ioptron_simple_reply_command(device, ":Sd%+08.0f#", dec * 60 * 60 * 100) && *PRIVATE_DATA->response == '1';
		}
	}
	if (result) {
		if (PRIVATE_DATA->protocol == HC_8406) {
			result = ioptron_command(device, ":CMR#");
		} else {
			result = ioptron_simple_reply_command(device, ":CM#") && *PRIVATE_DATA->response == '1';
		}
	}

	return result;
}

static bool ioptron_move(indigo_device *device, int slew_rate, char direction) {
	bool result = true;
	if (PRIVATE_DATA->last_slew_rate != slew_rate) {
		result = ioptron_simple_reply_command(device, ":SR%d#", slew_rate) && *PRIVATE_DATA->response == '1';
		if (result) {
			PRIVATE_DATA->last_slew_rate = slew_rate;
		}
	}
	if (result) {
		result = ioptron_no_reply_command(device, ":m%c#", direction);
	}
	return result;
}

static bool ioptron_stop(indigo_device *device) {
	if (PRIVATE_DATA->protocol == HC_8406) {
		return ioptron_no_reply_command(device, ":Q#");
	} else {
		if (ioptron_simple_reply_command(device, ":Q#")) {
			if (PRIVATE_DATA->protocol == HC_8407 || PRIVATE_DATA->protocol == V1_0 || PRIVATE_DATA->protocol == V2_0 || PRIVATE_DATA->protocol == V2_5) {
				return ioptron_no_reply_command(device, ":q#");
			} else {
				return true;
			}
		}
	}
	return false;
}

static bool ioptron_set_tracking(indigo_device *device, bool on) {
	return ioptron_simple_reply_command(device, ":ST%c#", on ? '1' : '0') && *PRIVATE_DATA->response == '1';
}

static bool ioptron_set_guide_rate(indigo_device *device, int ra, int dec) {
	bool result = false;
	if (PRIVATE_DATA->protocol == HC_8407 || PRIVATE_DATA->protocol == V1_0 || PRIVATE_DATA->protocol == V2_0) {
		result = ioptron_simple_reply_command(device, ":RG%03d#", ra) && *PRIVATE_DATA->response == '1';
	} else if (PRIVATE_DATA->protocol == V2_5 || PRIVATE_DATA->protocol == V3_0) {
		result = ioptron_simple_reply_command(device, ":RG%02d%02d#", ra, dec) && *PRIVATE_DATA->response == '1';
	}
	return result;
}

static bool ioptron_set_pec(indigo_device *device, bool on) {
	bool result = false;
	if (PRIVATE_DATA->protocol == V3_0 && !PRIVATE_DATA->has_encoders) {
		result = ioptron_simple_reply_command(device, ":SPP%c#",on ? '1' : '0') && *PRIVATE_DATA->response == '1';
	}
	return result;
}

static bool ioptron_set_pec_training(indigo_device *device, bool on) {
	bool result = false;
	if (PRIVATE_DATA->protocol == V3_0 && !PRIVATE_DATA->has_encoders) {
		result = ioptron_simple_reply_command(device, ":SPR%c#",on ? '1' : '0') && *PRIVATE_DATA->response == '1';
	}
	return result;
}

static bool ioptron_set_meridian_handling(indigo_device *device, bool flip, int degrees) {
	bool result = false;
	if (PRIVATE_DATA->protocol == V2_5 || PRIVATE_DATA->protocol == V3_0) {
		result = ioptron_simple_reply_command(device, ":SMT%c%02d#", flip ? '1' : '0', degrees) && *PRIVATE_DATA->response == '1';
	}
	return result;
}

static bool ioptron_guide_dec(indigo_device *device, int north, int south) {
	if (north > 0) {
		return ioptron_no_reply_command(device, ":Mn%05d#", north);
	} else if (south > 0) {
		return ioptron_no_reply_command(device, ":Ms%05d#", south);
	}
	return false;
}

static bool ioptron_guide_ra(indigo_device *device, int west, int east) {
	if (west > 0) {
		return ioptron_no_reply_command(device, ":Mw%03d#", west);
	} else if (east > 0) {
		return ioptron_no_reply_command(device, ":Me%03d#", east);
	}
	return false;
}

static bool ioptron_get_state(indigo_device *device) {
	PRIVATE_DATA->slewing = PRIVATE_DATA->tracking = PRIVATE_DATA->parked = PRIVATE_DATA->homed = false;
	if (PRIVATE_DATA->protocol == HC_8406) {
		if (ioptron_simple_reply_command(device, ":SE?#") && *PRIVATE_DATA->response == '1') {
			PRIVATE_DATA->slewing = true;
			return true;
		}
	} else if (PRIVATE_DATA->protocol == HC_8407) {
		if (ioptron_simple_reply_command(device, ":AP#") && *PRIVATE_DATA->response == '1') {
			PRIVATE_DATA->parked = true;
		}
		if (ioptron_simple_reply_command(device, ":AH#") && *PRIVATE_DATA->response == '1') {
			PRIVATE_DATA->homed = true;
		}
		if (ioptron_simple_reply_command(device, ":AT#") && *PRIVATE_DATA->response == '1') {
			PRIVATE_DATA->tracking = true;
		}
		if (ioptron_simple_reply_command(device, ":SE?#") && *PRIVATE_DATA->response == '1') {
			PRIVATE_DATA->slewing = true;
		}
		return true;
	} else if (PRIVATE_DATA->protocol == V1_0 || PRIVATE_DATA->protocol == V2_0) {
		if (ioptron_command(device, ":GAS#")) {
			switch (PRIVATE_DATA->response[1]) {
				case '0': // stopped (not at zero position)
					break;
				case '1': // tracking with PEC disabled
				case '3': // guiding
				case '4': // meridian flipping
				case '5': // tracking with PEC enabled (only for non-encoder edition)
					PRIVATE_DATA->tracking = true;
					break;
				case '2': // slewing
					PRIVATE_DATA->slewing = true;
					break;
				case '6': // parked
					PRIVATE_DATA->parked = true;
					break;
				case '7': // stopped at zero position
					PRIVATE_DATA->homed = true;
					break;
			}
			return true;
		}
	} else if (PRIVATE_DATA->protocol == V2_5) {
		if (ioptron_command(device, ":GLS#")) {
			switch (PRIVATE_DATA->response[14]) {
				case '0': // stopped (not at zero position)
					break;
				case '1': // tracking with PEC disabled
				case '3': // guiding
				case '4': // meridian flipping
				case '5': // tracking with PEC enabled (only for non-encoder edition)
					PRIVATE_DATA->tracking = true;
					break;
				case '2': // slewing
					PRIVATE_DATA->slewing = true;
					break;
				case '6': // parked
					PRIVATE_DATA->parked = true;
					break;
				case '7': // stopped at zero position
					PRIVATE_DATA->homed = true;
					break;
			}
			return true;
		}
	} else if (PRIVATE_DATA->protocol == V3_0) {
		if (ioptron_command(device, ":GLS#")) {
			switch (PRIVATE_DATA->response[18]) {
				case '0': // stopped (not at zero position)
					break;
				case '1': // tracking with PEC disabled
				case '3': // guiding
				case '4': // meridian flipping
				case '5': // tracking with PEC enabled (only for non-encoder edition)
					PRIVATE_DATA->tracking = true;
					break;
				case '2': // slewing
					PRIVATE_DATA->slewing = true;
					break;
				case '6': // parked
					PRIVATE_DATA->parked = true;
					break;
				case '7': // stopped at zero position
					PRIVATE_DATA->homed = true;
					break;
			}
			return true;
		}
	}
	return false;
}

// ---------------------------------------------------------------------  generic init & state update

static bool ioptron_init_mount(indigo_device *device) {
	MOUNT_SET_HOST_TIME_PROPERTY->hidden = false;
	MOUNT_SET_HOST_TIME_PROPERTY->state = INDIGO_OK_STATE;
	MOUNT_UTC_TIME_PROPERTY->hidden = false;
	MOUNT_UTC_TIME_PROPERTY->state = INDIGO_OK_STATE;
	MOUNT_TRACKING_PROPERTY->hidden = true;
	MOUNT_TRACKING_PROPERTY->state = INDIGO_OK_STATE;
	indigo_set_switch(MOUNT_TRACKING_PROPERTY, MOUNT_TRACKING_OFF_ITEM, true);
	MOUNT_TRACK_RATE_PROPERTY->hidden = true;
	MOUNT_TRACK_RATE_PROPERTY->count = 3;
	MOUNT_TRACK_RATE_PROPERTY->state = INDIGO_OK_STATE;
	indigo_set_switch(MOUNT_TRACK_RATE_PROPERTY, MOUNT_TRACK_RATE_SIDEREAL_ITEM, true);
	MOUNT_PARK_PROPERTY->hidden = true;
	MOUNT_PARK_PROPERTY->count = 2;
	MOUNT_PARK_PROPERTY->state = INDIGO_OK_STATE;
	MOUNT_PARK_SET_PROPERTY->hidden = true;
	MOUNT_PARK_SET_PROPERTY->count = 2;
	MOUNT_PARK_SET_PROPERTY->state = INDIGO_OK_STATE;
	MOUNT_HOME_PROPERTY->hidden = true;
	MOUNT_HOME_PROPERTY->count = 2;
	MOUNT_HOME_PROPERTY->rule = INDIGO_AT_MOST_ONE_RULE;
	MOUNT_HOME_PROPERTY->state = INDIGO_OK_STATE;
	MOUNT_HOME_ITEM->sw.value = false;
	MOUNT_GUIDE_RATE_PROPERTY->hidden = true;
	MOUNT_GUIDE_RATE_PROPERTY->count = 1;
	MOUNT_GUIDE_RATE_PROPERTY->state = INDIGO_OK_STATE;
	MOUNT_SLEW_RATE_PROPERTY->hidden = true;
	MOUNT_SLEW_RATE_PROPERTY->state = INDIGO_OK_STATE;
	MOUNT_CUSTOM_TRACKING_RATE_PROPERTY->hidden = true;
	MOUNT_CUSTOM_TRACKING_RATE_PROPERTY->state = INDIGO_OK_STATE;
	MOUNT_SIDE_OF_PIER_PROPERTY->hidden = true;
	MOUNT_SIDE_OF_PIER_EAST_ITEM->sw.value = MOUNT_SIDE_OF_PIER_WEST_ITEM->sw.value = false;
	MOUNT_SIDE_OF_PIER_PROPERTY->state = INDIGO_OK_STATE;
	MOUNT_PEC_PROPERTY->hidden = true;
	MOUNT_PEC_PROPERTY->state = INDIGO_OK_STATE;
	indigo_set_switch(MOUNT_PEC_PROPERTY, MOUNT_PEC_DISABLED_ITEM, true);
	MOUNT_PEC_TRAINING_PROPERTY->hidden = true;
	MOUNT_PEC_TRAINING_PROPERTY->state = INDIGO_OK_STATE;
	indigo_set_switch(MOUNT_PEC_TRAINING_PROPERTY, MOUNT_PEC_TRAINIG_STOPPED_ITEM, true);
	MOUNT_INFO_PROPERTY->count = 3;
	indigo_set_text_item_value(MOUNT_INFO_VENDOR_ITEM, "Unknown");
	indigo_set_text_item_value(MOUNT_INFO_MODEL_ITEM, "Unknown");
	indigo_set_text_item_value(MOUNT_INFO_FIRMWARE_ITEM, "Unknown");
	MOUNT_MERIDIAN_HANDLING_PROPERTY->hidden = true;
	MOUNT_MERIDIAN_HANDLING_PROPERTY->state = INDIGO_OK_STATE;
	MOUNT_MERIDIAN_LIMIT_PROPERTY->hidden = true;
	MOUNT_MERIDIAN_LIMIT_PROPERTY->state = INDIGO_OK_STATE;
	if (ioptron_detect_mount(device)) {
		if (PRIVATE_DATA->protocol == HC_8406) {
			MOUNT_PARK_PROPERTY->hidden = false;
			MOUNT_PARK_PROPERTY->count = 1;
			MOUNT_SIDE_OF_PIER_PROPERTY->hidden = false;
		} else if (PRIVATE_DATA->protocol == HC_8407) {
			MOUNT_PARK_PROPERTY->hidden = false;
			MOUNT_SIDE_OF_PIER_PROPERTY->hidden = false;
			MOUNT_TRACKING_PROPERTY->hidden = false;
			MOUNT_TRACK_RATE_PROPERTY->hidden = false;
			MOUNT_TRACK_RATE_PROPERTY->count = 5;
			if (ioptron_simple_reply_command(device, ":QT#")) {
				switch (*PRIVATE_DATA->response) {
					case '0':
						indigo_set_switch(MOUNT_TRACK_RATE_PROPERTY, MOUNT_TRACK_RATE_SIDEREAL_ITEM, true);
						break;
					case '1':
						indigo_set_switch(MOUNT_TRACK_RATE_PROPERTY, MOUNT_TRACK_RATE_LUNAR_ITEM, true);
						break;
					case '2':
						indigo_set_switch(MOUNT_TRACK_RATE_PROPERTY, MOUNT_TRACK_RATE_SOLAR_ITEM, true);
						break;
					case '3':
						indigo_set_switch(MOUNT_TRACK_RATE_PROPERTY, MOUNT_TRACK_RATE_KING_ITEM, true);
						break;
					case '4':
						indigo_set_switch(MOUNT_TRACK_RATE_PROPERTY, MOUNT_TRACK_RATE_CUSTOM_ITEM, true);
						break;
				}
			} else {
				MOUNT_TRACK_RATE_PROPERTY->state = INDIGO_ALERT_STATE;
			}
			if (ioptron_command(device, ":AG#")) {
				MOUNT_GUIDE_RATE_RA_ITEM->number.value = atof(PRIVATE_DATA->response) * 100;
			} else {
				MOUNT_GUIDE_RATE_PROPERTY->state = INDIGO_ALERT_STATE;
			}
			MOUNT_SLEW_RATE_PROPERTY->hidden = false;
		} else if (PRIVATE_DATA->protocol == V1_0) {
			MOUNT_PARK_PROPERTY->hidden = !PRIVATE_DATA->can_park;
			MOUNT_TRACKING_PROPERTY->hidden = false;
			MOUNT_TRACK_RATE_PROPERTY->hidden = false;
			MOUNT_TRACK_RATE_PROPERTY->count = 5;
			if (ioptron_command(device, ":GAS#")) {
				switch (PRIVATE_DATA->response[2]) {
					case '0':
						indigo_set_switch(MOUNT_TRACK_RATE_PROPERTY, MOUNT_TRACK_RATE_SIDEREAL_ITEM, true);
						break;
					case '1':
						indigo_set_switch(MOUNT_TRACK_RATE_PROPERTY, MOUNT_TRACK_RATE_LUNAR_ITEM, true);
						break;
					case '2':
						indigo_set_switch(MOUNT_TRACK_RATE_PROPERTY, MOUNT_TRACK_RATE_SOLAR_ITEM, true);
						break;
					case '3':
						indigo_set_switch(MOUNT_TRACK_RATE_PROPERTY, MOUNT_TRACK_RATE_KING_ITEM, true);
						break;
					case '4':
						indigo_set_switch(MOUNT_TRACK_RATE_PROPERTY, MOUNT_TRACK_RATE_CUSTOM_ITEM, true);
						break;
				}
			} else {
				MOUNT_TRACK_RATE_PROPERTY->state = INDIGO_ALERT_STATE;
			}
			MOUNT_GUIDE_RATE_PROPERTY->hidden = false;
			if (ioptron_command(device, ":AG#")) {
				MOUNT_GUIDE_RATE_RA_ITEM->number.value = atof(PRIVATE_DATA->response) * 100;
			} else {
				MOUNT_GUIDE_RATE_PROPERTY->state = INDIGO_ALERT_STATE;
			}
			MOUNT_SLEW_RATE_PROPERTY->hidden = false;
		} else if (PRIVATE_DATA->protocol == V2_0) {
			MOUNT_PARK_PROPERTY->hidden = !PRIVATE_DATA->can_park;
			MOUNT_HOME_PROPERTY->hidden = false;
			MOUNT_HOME_PROPERTY->count = PRIVATE_DATA->can_search_home ? 3 : 2;
			MOUNT_PARK_SET_PROPERTY->hidden = false;
			MOUNT_PARK_SET_PROPERTY->count = 1;
			MOUNT_TRACKING_PROPERTY->hidden = false;
			MOUNT_TRACK_RATE_PROPERTY->hidden = false;
			if (ioptron_command(device, ":GAS#")) {
				switch (PRIVATE_DATA->response[2]) {
					case '0':
						indigo_set_switch(MOUNT_TRACK_RATE_PROPERTY, MOUNT_TRACK_RATE_SIDEREAL_ITEM, true);
						break;
					case '1':
						indigo_set_switch(MOUNT_TRACK_RATE_PROPERTY, MOUNT_TRACK_RATE_LUNAR_ITEM, true);
						break;
					case '2':
						indigo_set_switch(MOUNT_TRACK_RATE_PROPERTY, MOUNT_TRACK_RATE_SOLAR_ITEM, true);
						break;
					case '3':
						indigo_set_switch(MOUNT_TRACK_RATE_PROPERTY, MOUNT_TRACK_RATE_KING_ITEM, true);
						break;
					case '4':
						indigo_set_switch(MOUNT_TRACK_RATE_PROPERTY, MOUNT_TRACK_RATE_CUSTOM_ITEM, true);
						break;
				}
			} else {
				MOUNT_TRACK_RATE_PROPERTY->state = INDIGO_ALERT_STATE;
			}
			MOUNT_GUIDE_RATE_PROPERTY->hidden = false;
			if (ioptron_command(device, ":AG#")) {
				MOUNT_GUIDE_RATE_RA_ITEM->number.value = atoi(PRIVATE_DATA->response);
			} else {
				MOUNT_GUIDE_RATE_PROPERTY->state = INDIGO_ALERT_STATE;
			}
			MOUNT_SLEW_RATE_PROPERTY->hidden = false;
		} else if (PRIVATE_DATA->protocol == V2_5) {
			MOUNT_PARK_PROPERTY->hidden = !PRIVATE_DATA->can_park;
			MOUNT_HOME_PROPERTY->hidden = false;
			MOUNT_HOME_PROPERTY->count = PRIVATE_DATA->can_search_home ? 3 : 2;
			MOUNT_PARK_SET_PROPERTY->hidden = false;
			MOUNT_PARK_SET_PROPERTY->count = 1;
			MOUNT_TRACKING_PROPERTY->hidden = false;
			MOUNT_TRACK_RATE_PROPERTY->hidden = false;
			if (ioptron_command(device, ":GLS#")) {
				switch (PRIVATE_DATA->response[15]) {
					case '0':
						indigo_set_switch(MOUNT_TRACK_RATE_PROPERTY, MOUNT_TRACK_RATE_SIDEREAL_ITEM, true);
						break;
					case '1':
						indigo_set_switch(MOUNT_TRACK_RATE_PROPERTY, MOUNT_TRACK_RATE_LUNAR_ITEM, true);
						break;
					case '2':
						indigo_set_switch(MOUNT_TRACK_RATE_PROPERTY, MOUNT_TRACK_RATE_SOLAR_ITEM, true);
						break;
					case '3':
						indigo_set_switch(MOUNT_TRACK_RATE_PROPERTY, MOUNT_TRACK_RATE_KING_ITEM, true);
						break;
					case '4':
						indigo_set_switch(MOUNT_TRACK_RATE_PROPERTY, MOUNT_TRACK_RATE_CUSTOM_ITEM, true);
						break;
				}
			} else {
				MOUNT_TRACK_RATE_PROPERTY->state = INDIGO_ALERT_STATE;
			}
			MOUNT_GUIDE_RATE_PROPERTY->hidden = false;
			MOUNT_GUIDE_RATE_PROPERTY->count = 2;
			if (ioptron_command(device, ":AG#")) {
				MOUNT_GUIDE_RATE_DEC_ITEM->number.value = atoi(PRIVATE_DATA->response + 2);
				PRIVATE_DATA->response[2] = 0;
				MOUNT_GUIDE_RATE_RA_ITEM->number.value = atoi(PRIVATE_DATA->response);
			} else {
				MOUNT_GUIDE_RATE_PROPERTY->state = INDIGO_ALERT_STATE;
			}
			MOUNT_SLEW_RATE_PROPERTY->hidden = false;
		} else if (PRIVATE_DATA->protocol == V3_0) {
			MOUNT_PARK_PROPERTY->hidden = !PRIVATE_DATA->can_park;
			MOUNT_HOME_PROPERTY->hidden = false;
			MOUNT_HOME_PROPERTY->count = PRIVATE_DATA->can_search_home ? 3 : 2;
			MOUNT_TRACK_RATE_PROPERTY->hidden = false;
			MOUNT_SIDE_OF_PIER_PROPERTY->hidden = false;
			MOUNT_PEC_PROPERTY->hidden = PRIVATE_DATA->has_encoders;
			MOUNT_PEC_TRAINING_PROPERTY->hidden = PRIVATE_DATA->has_encoders;
			if (ioptron_command(device, ":GLS#")) {
				switch (PRIVATE_DATA->response[19]) {
					case '0':
						indigo_set_switch(MOUNT_TRACK_RATE_PROPERTY, MOUNT_TRACK_RATE_SIDEREAL_ITEM, true);
						break;
					case '1':
						indigo_set_switch(MOUNT_TRACK_RATE_PROPERTY, MOUNT_TRACK_RATE_LUNAR_ITEM, true);
						break;
					case '2':
						indigo_set_switch(MOUNT_TRACK_RATE_PROPERTY, MOUNT_TRACK_RATE_SOLAR_ITEM, true);
						break;
					case '3':
						indigo_set_switch(MOUNT_TRACK_RATE_PROPERTY, MOUNT_TRACK_RATE_KING_ITEM, true);
						break;
					case '4':
						indigo_set_switch(MOUNT_TRACK_RATE_PROPERTY, MOUNT_TRACK_RATE_CUSTOM_ITEM, true);
						break;
				}
			} else {
				MOUNT_TRACK_RATE_PROPERTY->state = INDIGO_ALERT_STATE;
			}
			MOUNT_GUIDE_RATE_PROPERTY->hidden = false;
			MOUNT_GUIDE_RATE_PROPERTY->count = 2;
			if (ioptron_command(device, ":AG#")) {
				MOUNT_GUIDE_RATE_DEC_ITEM->number.value = atoi(PRIVATE_DATA->response + 2);
				PRIVATE_DATA->response[2] = 0;
				MOUNT_GUIDE_RATE_RA_ITEM->number.value = atoi(PRIVATE_DATA->response);
			} else {
				MOUNT_GUIDE_RATE_PROPERTY->state = INDIGO_ALERT_STATE;
			}
			MOUNT_TRACKING_PROPERTY->hidden = false;
			MOUNT_CUSTOM_TRACKING_RATE_PROPERTY->hidden = false;
			MOUNT_CUSTOM_TRACKING_RATE_ITEM->number.min = 0.1;
			MOUNT_CUSTOM_TRACKING_RATE_ITEM->number.max = 1.9;
			if (ioptron_command(device, ":GTR#")) {
				MOUNT_CUSTOM_TRACKING_RATE_ITEM->number.value = atoi(PRIVATE_DATA->response) / 10000.0;
			} else {
				MOUNT_CUSTOM_TRACKING_RATE_PROPERTY->state = INDIGO_ALERT_STATE;
			}
			MOUNT_SLEW_RATE_PROPERTY->hidden = false;
			MOUNT_MERIDIAN_HANDLING_PROPERTY->hidden = false;
			MOUNT_MERIDIAN_LIMIT_PROPERTY->hidden = false;
		}
	} else {
		return false;
	}
	if (ioptron_get_state(device)) {
		if (PRIVATE_DATA->parked) {
			indigo_set_switch(MOUNT_PARK_PROPERTY, MOUNT_PARK_PARKED_ITEM, true);
			MOUNT_PARK_PROPERTY->state = MOUNT_STATE_PARK_ITEM->light.value = INDIGO_OK_STATE;
		} else {
			indigo_set_switch(MOUNT_PARK_PROPERTY, MOUNT_PARK_UNPARKED_ITEM, true);
			MOUNT_PARK_PROPERTY->state = INDIGO_OK_STATE;
			MOUNT_STATE_PARK_ITEM->light.value = INDIGO_IDLE_STATE;
		}
		if (PRIVATE_DATA->homed) {
			indigo_set_switch(MOUNT_HOME_PROPERTY, MOUNT_HOME_ITEM, true);
			MOUNT_HOME_PROPERTY->state = MOUNT_STATE_HOME_ITEM->light.value = INDIGO_OK_STATE;
		} else {
			indigo_set_switch(MOUNT_HOME_PROPERTY, MOUNT_AWAY_ITEM, true);
			MOUNT_HOME_PROPERTY->state = INDIGO_OK_STATE;
			MOUNT_STATE_HOME_ITEM->light.value = INDIGO_IDLE_STATE;
		}
		if (PRIVATE_DATA->tracking) {
			indigo_set_switch(MOUNT_TRACKING_PROPERTY, MOUNT_TRACKING_ON_ITEM, true);
			MOUNT_STATE_TRACKING_ITEM->light.value = INDIGO_OK_STATE;
		} else {
			indigo_set_switch(MOUNT_TRACKING_PROPERTY, MOUNT_TRACKING_OFF_ITEM, true);
			MOUNT_STATE_TRACKING_ITEM->light.value = INDIGO_IDLE_STATE;
		}
	} else {
		return false;
	}
	return true;
}

static bool ioptron_init_guider(indigo_device *device) {
	GUIDER_RATE_PROPERTY->state = INDIGO_OK_STATE;
	if (ioptron_detect_mount(device->master_device)) {
		if (PRIVATE_DATA->protocol == HC_8406) {
			GUIDER_RATE_PROPERTY->hidden = true;
		} else if (PRIVATE_DATA->protocol == HC_8407 || PRIVATE_DATA->protocol == V1_0) {
			GUIDER_RATE_PROPERTY->hidden = false;
			GUIDER_RATE_PROPERTY->count = 1;
			if (ioptron_command(device, ":AG#")) {
				GUIDER_RATE_ITEM->number.value = atof(PRIVATE_DATA->response) * 100;
			} else {
				GUIDER_RATE_PROPERTY->state = INDIGO_ALERT_STATE;
			}
		} else if (PRIVATE_DATA->protocol == V2_0) {
			GUIDER_RATE_PROPERTY->hidden = false;
			GUIDER_RATE_PROPERTY->count = 1;
			if (ioptron_command(device, ":AG#")) {
				GUIDER_RATE_ITEM->number.value = atoi(PRIVATE_DATA->response);
			} else {
				GUIDER_RATE_PROPERTY->state = INDIGO_ALERT_STATE;
			}
		} else if (PRIVATE_DATA->protocol == V2_5 || PRIVATE_DATA->protocol == V3_0) {
			GUIDER_RATE_PROPERTY->hidden = false;
			GUIDER_RATE_PROPERTY->count = 2;
			if (ioptron_command(device, ":AG#")) {
				GUIDER_DEC_RATE_ITEM->number.value = atoi(PRIVATE_DATA->response + 2);
				PRIVATE_DATA->response[2] = 0;
				GUIDER_RATE_ITEM->number.value = atoi(PRIVATE_DATA->response);
			} else {
				GUIDER_RATE_PROPERTY->state = INDIGO_ALERT_STATE;
			}
		} else {
			return false;
		}
	} else {
		return false;
	}
	return true;
}

static void ioptron_update_mount_state(indigo_device *device) {
	double ra = 0, dec = 0;
	if (ioptron_get_coordinates(device, &ra, &dec)) {
		indigo_eq_to_j2k(MOUNT_EPOCH_ITEM->number.value, &ra, &dec);
		MOUNT_EQUATORIAL_COORDINATES_RA_ITEM->number.value = ra;
		MOUNT_EQUATORIAL_COORDINATES_DEC_ITEM->number.value = dec;
	}
	if (ioptron_get_state(device)) {
		if (PRIVATE_DATA->slewing) {
			MOUNT_EQUATORIAL_COORDINATES_PROPERTY->state = MOUNT_STATE_SLEW_ITEM->light.value = INDIGO_BUSY_STATE;
		} else {
			if (MOUNT_EQUATORIAL_COORDINATES_PROPERTY->state != INDIGO_ALERT_STATE) {
				MOUNT_STATE_SLEW_ITEM->light.value = INDIGO_IDLE_STATE;
				MOUNT_EQUATORIAL_COORDINATES_PROPERTY->state = INDIGO_OK_STATE;
			} else {
				 MOUNT_STATE_SLEW_ITEM->light.value = INDIGO_ALERT_STATE;
			}
		}
		if (MOUNT_TRACKING_PROPERTY->state != INDIGO_BUSY_STATE) { // to avoid race never change tracking state if BUSY
			if (PRIVATE_DATA->tracking && !MOUNT_TRACKING_ON_ITEM->sw.value) {
				indigo_set_switch(MOUNT_TRACKING_PROPERTY, MOUNT_TRACKING_ON_ITEM, true);
				MOUNT_TRACKING_PROPERTY->state = MOUNT_STATE_TRACKING_ITEM->light.value = INDIGO_OK_STATE;
			} else if (!PRIVATE_DATA->tracking && !MOUNT_TRACKING_OFF_ITEM->sw.value) {
				indigo_set_switch(MOUNT_TRACKING_PROPERTY, MOUNT_TRACKING_OFF_ITEM, true);
				MOUNT_STATE_TRACKING_ITEM->light.value = INDIGO_IDLE_STATE;
				MOUNT_TRACKING_PROPERTY->state = INDIGO_OK_STATE;
			}
		}
		if (MOUNT_PARK_PROPERTY->state == INDIGO_BUSY_STATE) { // to avoid race never change parking state if BUSY with these exceptions
			if (MOUNT_PARK_PARKED_ITEM->sw.value && PRIVATE_DATA->parked) {
				MOUNT_PARK_PROPERTY->state = MOUNT_STATE_PARK_ITEM->light.value = INDIGO_OK_STATE;
			} else if (MOUNT_PARK_PROPERTY->count == 2 && MOUNT_PARK_UNPARKED_ITEM->sw.value && !PRIVATE_DATA->parked) {
				MOUNT_STATE_PARK_ITEM->light.value = INDIGO_IDLE_STATE;
				MOUNT_PARK_PROPERTY->state = INDIGO_OK_STATE;
			}
		} else { // otherwise mirror state reported by mount
			if (PRIVATE_DATA->parked) {
				indigo_set_switch(MOUNT_PARK_PROPERTY, MOUNT_PARK_PARKED_ITEM, true);
				MOUNT_STATE_PARK_ITEM->light.value = INDIGO_OK_STATE;
			} else {
				indigo_set_switch(MOUNT_PARK_PROPERTY, MOUNT_PARK_UNPARKED_ITEM, true);
				MOUNT_STATE_PARK_ITEM->light.value = INDIGO_IDLE_STATE;
			}
		}
		if (MOUNT_HOME_PROPERTY->state == INDIGO_BUSY_STATE) { // to avoid race never change parking state if BUSY with this exception
			if (PRIVATE_DATA->homed) {
				MOUNT_HOME_PROPERTY->state = MOUNT_STATE_HOME_ITEM->light.value = INDIGO_OK_STATE;
			}
		} else { // otherwise mirror state reported by mount
			if (!PRIVATE_DATA->homed) {
				indigo_set_switch(MOUNT_HOME_PROPERTY, MOUNT_AWAY_ITEM, true);
				MOUNT_STATE_HOME_ITEM->light.value = INDIGO_IDLE_STATE;
			} else if (PRIVATE_DATA->homed) {
				indigo_set_switch(MOUNT_HOME_PROPERTY, MOUNT_HOME_ITEM, true);
				MOUNT_STATE_HOME_ITEM->light.value = INDIGO_OK_STATE;
			}
		}
	}
	sprintf(MOUNT_UTC_OFFSET_ITEM->text.value, "%d", PRIVATE_DATA->utc_offset);
	indigo_timetoisogm(time(NULL) - PRIVATE_DATA->time_difference, MOUNT_UTC_ITEM->text.value, INDIGO_VALUE_SIZE);
	MOUNT_UTC_TIME_PROPERTY->state = INDIGO_OK_STATE;
	indigo_update_property(device, MOUNT_STATE_PROPERTY, NULL);
	indigo_update_property(device, MOUNT_TRACKING_PROPERTY, NULL);
	indigo_update_property(device, MOUNT_PARK_PROPERTY, NULL);
	indigo_update_property(device, MOUNT_HOME_PROPERTY, NULL);
	indigo_update_property(device, MOUNT_SIDE_OF_PIER_PROPERTY, NULL);
	indigo_update_property(device, MOUNT_UTC_TIME_PROPERTY, NULL);
	indigo_update_coordinates(device, NULL);
}

//- code

//+ guider.code

static void guider_guide_dec_finalizer(indigo_device *device) {
	GUIDER_GUIDE_NORTH_ITEM->number.value = GUIDER_GUIDE_SOUTH_ITEM->number.value = 0;
	INDIGO_UPDATE_PROPERTY_STATE(GUIDER_GUIDE_DEC_PROPERTY, INDIGO_OK_STATE, NULL);
}

static void guider_guide_ra_finalizer(indigo_device *device) {
	GUIDER_GUIDE_WEST_ITEM->number.value = GUIDER_GUIDE_EAST_ITEM->number.value = 0;
	INDIGO_UPDATE_PROPERTY_STATE(GUIDER_GUIDE_RA_PROPERTY, INDIGO_OK_STATE, NULL);
}

//- guider.code

#pragma mark - High level code (mount)

static void mount_timer_callback(indigo_device *device) {
	if (!IS_CONNECTED) {
		return;
	}
	//+ mount.on_timer
	if (PRIVATE_DATA->handle != NULL) {
		ioptron_update_mount_state(device);
		indigo_execute_handler_in(device, MOUNT_EQUATORIAL_COORDINATES_PROPERTY->state == INDIGO_BUSY_STATE ? 0.5 : 1, mount_timer_callback);
	}
	//- mount.on_timer
}

static void mount_connection_handler(indigo_device *device) {
	if (CONNECTION_CONNECTED_ITEM->sw.value) {
		bool connection_result = true;
		if (PRIVATE_DATA->count++ == 0) {
			connection_result = ioptron_open(device);
		}
		if (connection_result) {
			//+ mount.on_connect
			connection_result = ioptron_init_mount(device);
			//- mount.on_connect
		}
		if (connection_result) {
			indigo_define_property(device, MOUNT_MERIDIAN_HANDLING_PROPERTY, NULL);
			indigo_define_property(device, MOUNT_MERIDIAN_LIMIT_PROPERTY, NULL);
			indigo_define_property(device, MOUNT_PROTOCOL_PROPERTY, NULL);
			indigo_execute_handler(device, mount_timer_callback);
			CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
			indigo_send_message(device, OK_PROPERTY, "Connected to %s on %s", MOUNT_DEVICE_NAME, DEVICE_PORT_ITEM->text.value);
		} else {
			indigo_send_message(device, ALERT_PROPERTY, "Failed to connect to %s on %s", MOUNT_DEVICE_NAME, DEVICE_PORT_ITEM->text.value);
			PRIVATE_DATA->count--;
			CONNECTION_PROPERTY->state = INDIGO_ALERT_STATE;
			indigo_set_switch(CONNECTION_PROPERTY, CONNECTION_DISCONNECTED_ITEM, true);
		}
	} else {
		indigo_cancel_pending_handlers(device);
		indigo_delete_property(device, MOUNT_MERIDIAN_HANDLING_PROPERTY, NULL);
		indigo_delete_property(device, MOUNT_MERIDIAN_LIMIT_PROPERTY, NULL);
		indigo_delete_property(device, MOUNT_PROTOCOL_PROPERTY, NULL);
		if (--PRIVATE_DATA->count == 0) {
			ioptron_close(device);
		}
		indigo_send_message(device, OK_PROPERTY, "Disconnected from %s", device->name);
		CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
	}
	indigo_mount_change_property(device, NULL, CONNECTION_PROPERTY);
}

static void mount_park_set_handler(indigo_device *device) {
	MOUNT_PARK_SET_PROPERTY->state = INDIGO_OK_STATE;
	//+ mount.MOUNT_PARK_SET.on_change
	if (MOUNT_PARK_SET_DEFAULT_ITEM->sw.value) {
		MOUNT_PARK_SET_DEFAULT_ITEM->sw.value = false;
		if (!ioptron_park_set_default(device)) {
			MOUNT_PARK_SET_PROPERTY->state = INDIGO_ALERT_STATE;
		}
	} else if (MOUNT_PARK_SET_CURRENT_ITEM->sw.value) {
		if (!ioptron_park_set_current(device)) {
			MOUNT_PARK_SET_CURRENT_ITEM->sw.value = false;
			MOUNT_PARK_SET_PROPERTY->state = INDIGO_ALERT_STATE;
		}
	}
	//- mount.MOUNT_PARK_SET.on_change
	indigo_update_property(device, MOUNT_PARK_SET_PROPERTY, NULL);
}

static void mount_park_handler(indigo_device *device) {
	MOUNT_PARK_PROPERTY->state = INDIGO_OK_STATE;
	//+ mount.MOUNT_PARK.on_change
	if (MOUNT_PARK_PARKED_ITEM->sw.value) {
		if (MOUNT_PARK_PROPERTY->count == 1) {
			MOUNT_PARK_PARKED_ITEM->sw.value = false;
		}
		if (!ioptron_park(device)) {
			MOUNT_PARK_PROPERTY->state = INDIGO_ALERT_STATE;
		} else if (MOUNT_PARK_PROPERTY->count == 1) {
			MOUNT_PARK_PROPERTY->state = INDIGO_OK_STATE;
		} else {
			MOUNT_PARK_PROPERTY->state = INDIGO_BUSY_STATE;
		}
		MOUNT_STATE_PARK_ITEM->light.value = MOUNT_PARK_PROPERTY->state;
	} else {
		if (ioptron_unpark(device)) {
			MOUNT_STATE_PARK_ITEM->light.value = INDIGO_IDLE_STATE;
		} else {
			MOUNT_PARK_PROPERTY->state = MOUNT_STATE_PARK_ITEM->light.value = INDIGO_ALERT_STATE;
		}
	}
	indigo_update_property(device, MOUNT_STATE_PROPERTY, NULL);
	//- mount.MOUNT_PARK.on_change
	indigo_update_property(device, MOUNT_PARK_PROPERTY, NULL);
}

static void mount_home_handler(indigo_device *device) {
	//+ mount.MOUNT_HOME.on_change
	MOUNT_HOME_PROPERTY->state = INDIGO_BUSY_STATE;
	if (MOUNT_HOME_ITEM->sw.value) {
		MOUNT_HOME_ITEM->sw.value = false;
		if (!ioptron_home(device)) {
			MOUNT_HOME_PROPERTY->state = INDIGO_ALERT_STATE;
		}
	} else if (MOUNT_AWAY_ITEM->sw.value) {
		MOUNT_AWAY_ITEM->sw.value = false;
		MOUNT_HOME_PROPERTY->state = INDIGO_OK_STATE;
	} else if (MOUNT_HOME_SEARCH_ITEM->sw.value) {
		MOUNT_HOME_SEARCH_ITEM->sw.value = false;
		if (!ioptron_search_home(device)) {
			MOUNT_HOME_PROPERTY->state = INDIGO_ALERT_STATE;
		}
	}
	MOUNT_STATE_HOME_ITEM->light.value = MOUNT_HOME_PROPERTY->state;
	indigo_update_property(device, MOUNT_STATE_PROPERTY, NULL);
	//- mount.MOUNT_HOME.on_change
	indigo_update_property(device, MOUNT_HOME_PROPERTY, NULL);
}

static void mount_geographic_coordinates_handler(indigo_device *device) {
	MOUNT_GEOGRAPHIC_COORDINATES_PROPERTY->state = INDIGO_OK_STATE;
	//+ mount.MOUNT_GEOGRAPHIC_COORDINATES.on_change
	if (!ioptron_set_site(device, MOUNT_GEOGRAPHIC_COORDINATES_LATITUDE_ITEM->number.value, MOUNT_GEOGRAPHIC_COORDINATES_LONGITUDE_ITEM->number.value, 0)) {
		MOUNT_GEOGRAPHIC_COORDINATES_PROPERTY->state = INDIGO_ALERT_STATE;
	}
	//- mount.MOUNT_GEOGRAPHIC_COORDINATES.on_change
	indigo_update_property(device, MOUNT_GEOGRAPHIC_COORDINATES_PROPERTY, NULL);
}

static void mount_equatorial_coordinates_handler(indigo_device *device) {
	if (!MOUNT_PARK_PROPERTY->hidden && MOUNT_PARK_PARKED_ITEM->sw.value) {
		indigo_send_message(device, MOUNT_EQUATORIAL_COORDINATES_PROPERTY, "Mount is parked!");
		INDIGO_UPDATE_PROPERTY_STATE(MOUNT_EQUATORIAL_COORDINATES_PROPERTY, INDIGO_ALERT_STATE, NULL);
		return;
	}
	//+ mount.MOUNT_EQUATORIAL_COORDINATES.on_change
	MOUNT_EQUATORIAL_COORDINATES_PROPERTY->state = INDIGO_BUSY_STATE;
	double ra = MOUNT_EQUATORIAL_COORDINATES_RA_ITEM->number.target;
	double dec = MOUNT_EQUATORIAL_COORDINATES_DEC_ITEM->number.target;
	indigo_j2k_to_eq(MOUNT_EPOCH_ITEM->number.value, &ra, &dec);
	if (MOUNT_ON_COORDINATES_SET_TRACK_ITEM->sw.value) {
		if (MOUNT_TRACK_RATE_SIDEREAL_ITEM->sw.value) {
			if (!ioptron_set_tracking_rate(device, 0, 0)) {
				MOUNT_EQUATORIAL_COORDINATES_PROPERTY->state = INDIGO_ALERT_STATE;
			}
		} else if (MOUNT_TRACK_RATE_LUNAR_ITEM->sw.value) {
			if (!ioptron_set_tracking_rate(device, 1, 0)) {
				MOUNT_EQUATORIAL_COORDINATES_PROPERTY->state = INDIGO_ALERT_STATE;
			}
		} else if (MOUNT_TRACK_RATE_SOLAR_ITEM->sw.value) {
			if (!ioptron_set_tracking_rate(device, 2, 0)) {
				MOUNT_EQUATORIAL_COORDINATES_PROPERTY->state = INDIGO_ALERT_STATE;
			}
		} else if (MOUNT_TRACK_RATE_KING_ITEM->sw.value) {
			if (!ioptron_set_tracking_rate(device, 3, 0)) {
				MOUNT_EQUATORIAL_COORDINATES_PROPERTY->state = INDIGO_ALERT_STATE;
			}
		} else if (MOUNT_TRACK_RATE_CUSTOM_ITEM->sw.value) {
			if (!ioptron_set_tracking_rate(device, 4, MOUNT_CUSTOM_TRACKING_RATE_ITEM->number.value)) {
				MOUNT_EQUATORIAL_COORDINATES_PROPERTY->state = INDIGO_ALERT_STATE;
			}
		}
		if (MOUNT_EQUATORIAL_COORDINATES_PROPERTY->state == INDIGO_BUSY_STATE) {
			if (!ioptron_slew(device, ra, dec)) {
				MOUNT_EQUATORIAL_COORDINATES_PROPERTY->state = INDIGO_ALERT_STATE;
			}
		}
	} else if (MOUNT_ON_COORDINATES_SET_SYNC_ITEM->sw.value) {
		if (ioptron_sync(device, ra, dec)) {
			MOUNT_EQUATORIAL_COORDINATES_PROPERTY->state = INDIGO_OK_STATE;
		} else {
			MOUNT_EQUATORIAL_COORDINATES_PROPERTY->state = INDIGO_ALERT_STATE;
		}
	}
	//- mount.MOUNT_EQUATORIAL_COORDINATES.on_change
	indigo_update_coordinates(device, NULL);
}

static void mount_abort_motion_handler(indigo_device *device) {
	MOUNT_ABORT_MOTION_PROPERTY->state = INDIGO_OK_STATE;
	//+ mount.MOUNT_ABORT_MOTION.on_change
	if (MOUNT_ABORT_MOTION_ITEM->sw.value) {
		MOUNT_ABORT_MOTION_ITEM->sw.value = false;
		if (ioptron_stop(device)) {
			PRIVATE_DATA->last_motion_dec = 0;
			MOUNT_MOTION_NORTH_ITEM->sw.value = false;
			MOUNT_MOTION_SOUTH_ITEM->sw.value = false;
			INDIGO_UPDATE_PROPERTY_STATE(MOUNT_MOTION_DEC_PROPERTY, INDIGO_OK_STATE, NULL);
			PRIVATE_DATA->last_motion_ra = 0;
			MOUNT_MOTION_WEST_ITEM->sw.value = false;
			MOUNT_MOTION_EAST_ITEM->sw.value = false;
			INDIGO_UPDATE_PROPERTY_STATE(MOUNT_MOTION_RA_PROPERTY, INDIGO_OK_STATE, NULL);
			MOUNT_EQUATORIAL_COORDINATES_PROPERTY->state = INDIGO_OK_STATE;
			indigo_update_coordinates(device, NULL);
		} else {
			MOUNT_ABORT_MOTION_PROPERTY->state = INDIGO_ALERT_STATE;
		}
	}
	//- mount.MOUNT_ABORT_MOTION.on_change
	indigo_update_property(device, MOUNT_ABORT_MOTION_PROPERTY, NULL);
}

static void mount_motion_dec_handler(indigo_device *device) {
	if (!MOUNT_PARK_PROPERTY->hidden && MOUNT_PARK_PARKED_ITEM->sw.value) {
		indigo_send_message(device, MOUNT_MOTION_DEC_PROPERTY, "Mount is parked!");
		INDIGO_UPDATE_PROPERTY_STATE(MOUNT_MOTION_DEC_PROPERTY, INDIGO_ALERT_STATE, NULL);
		return;
	}
	//+ mount.MOUNT_MOTION_DEC.on_change
	MOUNT_MOTION_DEC_PROPERTY->state = INDIGO_BUSY_STATE;
	int slew_rate = 0;
	if (MOUNT_SLEW_RATE_GUIDE_ITEM->sw.value) {
		slew_rate = 1;
	} else if (MOUNT_SLEW_RATE_CENTERING_ITEM->sw.value) {
		slew_rate = 3;
	} else if (MOUNT_SLEW_RATE_FIND_ITEM->sw.value) {
		slew_rate = 5;
	} else if (MOUNT_SLEW_RATE_MAX_ITEM->sw.value) {
		slew_rate = 9;
	}
	if (MOUNT_MOTION_NORTH_ITEM->sw.value) {
		if (!ioptron_move(device, slew_rate, 'n')) {
			MOUNT_MOTION_DEC_PROPERTY->state = INDIGO_ALERT_STATE;
		}
	} else if (MOUNT_MOTION_SOUTH_ITEM->sw.value) {
		if (!ioptron_move(device, slew_rate, 's')) {
			MOUNT_MOTION_DEC_PROPERTY->state = INDIGO_ALERT_STATE;
		}
	} else {
		if (ioptron_stop(device)) {
			MOUNT_MOTION_DEC_PROPERTY->state = INDIGO_OK_STATE;
		} else {
			MOUNT_MOTION_DEC_PROPERTY->state = INDIGO_ALERT_STATE;
		}
	}
	//- mount.MOUNT_MOTION_DEC.on_change
	indigo_update_property(device, MOUNT_MOTION_DEC_PROPERTY, NULL);
}

static void mount_motion_ra_handler(indigo_device *device) {
	if (!MOUNT_PARK_PROPERTY->hidden && MOUNT_PARK_PARKED_ITEM->sw.value) {
		indigo_send_message(device, MOUNT_MOTION_RA_PROPERTY, "Mount is parked!");
		INDIGO_UPDATE_PROPERTY_STATE(MOUNT_MOTION_RA_PROPERTY, INDIGO_ALERT_STATE, NULL);
		return;
	}
	//+ mount.MOUNT_MOTION_RA.on_change
	MOUNT_MOTION_RA_PROPERTY->state = INDIGO_BUSY_STATE;
	int slew_rate = 0;
	if (MOUNT_SLEW_RATE_GUIDE_ITEM->sw.value) {
		slew_rate = 1;
	} else if (MOUNT_SLEW_RATE_CENTERING_ITEM->sw.value) {
		slew_rate = 3;
	} else if (MOUNT_SLEW_RATE_FIND_ITEM->sw.value) {
		slew_rate = 5;
	} else if (MOUNT_SLEW_RATE_MAX_ITEM->sw.value) {
		slew_rate = 9;
	}
	if (MOUNT_MOTION_WEST_ITEM->sw.value) {
		if (!ioptron_move(device, slew_rate, 'w')) {
			MOUNT_MOTION_RA_PROPERTY->state = INDIGO_ALERT_STATE;
		}
	} else if (MOUNT_MOTION_EAST_ITEM->sw.value) {
		if (!ioptron_move(device, slew_rate, 'e')) {
			MOUNT_MOTION_RA_PROPERTY->state = INDIGO_ALERT_STATE;
		}
	} else {
		if (ioptron_stop(device)) {
			MOUNT_MOTION_RA_PROPERTY->state = INDIGO_OK_STATE;
		} else {
			MOUNT_MOTION_RA_PROPERTY->state = INDIGO_ALERT_STATE;
		}
	}
	//- mount.MOUNT_MOTION_RA.on_change
	indigo_update_property(device, MOUNT_MOTION_RA_PROPERTY, NULL);
}

static void mount_set_host_time_handler(indigo_device *device) {
	MOUNT_SET_HOST_TIME_PROPERTY->state = INDIGO_OK_STATE;
	//+ mount.MOUNT_SET_HOST_TIME.on_change
	if (MOUNT_SET_HOST_TIME_ITEM->sw.value) {
		MOUNT_SET_HOST_TIME_ITEM->sw.value = false;
		time_t secs = time(NULL);
		if (ioptron_set_utc(device, secs, indigo_get_utc_offset())) {
			indigo_timetoisogm(secs, MOUNT_UTC_ITEM->text.value, INDIGO_VALUE_SIZE);
			INDIGO_UPDATE_PROPERTY_STATE(MOUNT_UTC_TIME_PROPERTY, INDIGO_OK_STATE, NULL);
		} else {
			MOUNT_SET_HOST_TIME_PROPERTY->state = INDIGO_ALERT_STATE;
		}
	}
	//- mount.MOUNT_SET_HOST_TIME.on_change
	indigo_update_property(device, MOUNT_SET_HOST_TIME_PROPERTY, NULL);
}

static void mount_utc_time_handler(indigo_device *device) {
	MOUNT_UTC_TIME_PROPERTY->state = INDIGO_OK_STATE;
	//+ mount.MOUNT_UTC_TIME.on_change
	time_t secs = indigo_isogmtotime(MOUNT_UTC_ITEM->text.value);
	if (secs == -1) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "Wrong date/time format!");
		MOUNT_UTC_TIME_PROPERTY->state = INDIGO_ALERT_STATE;
	} else {
		int offset = atoi(MOUNT_UTC_OFFSET_ITEM->text.value);
		if (!ioptron_set_utc(device, secs, offset)) {
			MOUNT_UTC_TIME_PROPERTY->state = INDIGO_ALERT_STATE;
		}
	}
	//- mount.MOUNT_UTC_TIME.on_change
	indigo_update_property(device, MOUNT_UTC_TIME_PROPERTY, NULL);
}

static void mount_tracking_handler(indigo_device *device) {
	if (!MOUNT_PARK_PROPERTY->hidden && MOUNT_PARK_PARKED_ITEM->sw.value) {
		indigo_send_message(device, MOUNT_TRACKING_PROPERTY, "Mount is parked!");
		INDIGO_UPDATE_PROPERTY_STATE(MOUNT_TRACKING_PROPERTY, INDIGO_ALERT_STATE, NULL);
		return;
	}
	MOUNT_TRACKING_PROPERTY->state = INDIGO_OK_STATE;
	//+ mount.MOUNT_TRACKING.on_change
	if (ioptron_set_tracking(device, MOUNT_TRACKING_ON_ITEM->sw.value)) {
		MOUNT_STATE_TRACKING_ITEM->light.value = MOUNT_TRACKING_ON_ITEM->sw.value ? INDIGO_OK_STATE : INDIGO_IDLE_STATE;
	} else {
		MOUNT_TRACKING_PROPERTY->state = MOUNT_STATE_TRACKING_ITEM->light.value = INDIGO_ALERT_STATE;
	}
	indigo_update_property(device, MOUNT_STATE_PROPERTY, NULL);
	//- mount.MOUNT_TRACKING.on_change
	indigo_update_property(device, MOUNT_TRACKING_PROPERTY, NULL);
}

static void mount_guide_rate_handler(indigo_device *device) {
	MOUNT_GUIDE_RATE_PROPERTY->state = INDIGO_OK_STATE;
	//+ mount.MOUNT_GUIDE_RATE.on_change
	if (!ioptron_set_guide_rate(device, (int)MOUNT_GUIDE_RATE_RA_ITEM->number.value, (int)MOUNT_GUIDE_RATE_DEC_ITEM->number.value)) {
		MOUNT_GUIDE_RATE_PROPERTY->state = INDIGO_ALERT_STATE;
	}
	//- mount.MOUNT_GUIDE_RATE.on_change
	indigo_update_property(device, MOUNT_GUIDE_RATE_PROPERTY, NULL);
}

static void mount_pec_handler(indigo_device *device) {
	MOUNT_PEC_PROPERTY->state = INDIGO_OK_STATE;
	//+ mount.MOUNT_PEC.on_change
	if (MOUNT_PEC_ENABLED_ITEM->sw.value) {
		if (MOUNT_PEC_TRAINIG_STARTED_ITEM->sw.value) {
			if (ioptron_set_pec_training(device, false)) {
				MOUNT_PEC_TRAINING_PROPERTY->state = INDIGO_OK_STATE;
				indigo_set_switch(MOUNT_PEC_TRAINING_PROPERTY, MOUNT_PEC_TRAINIG_STOPPED_ITEM, true);
			} else {
				MOUNT_PEC_TRAINING_PROPERTY->state = INDIGO_ALERT_STATE;
			}
			indigo_update_property(device, MOUNT_PEC_TRAINING_PROPERTY, NULL);
		}
		if (!ioptron_set_pec(device, true)) {
			MOUNT_PEC_PROPERTY->state = INDIGO_ALERT_STATE;
		}
	} else {
		if (!ioptron_set_pec(device, false)) {
			MOUNT_PEC_PROPERTY->state = INDIGO_ALERT_STATE;
		}
	}
	//- mount.MOUNT_PEC.on_change
	indigo_update_property(device, MOUNT_PEC_PROPERTY, NULL);
}

static void mount_pec_training_handler(indigo_device *device) {
	MOUNT_PEC_TRAINING_PROPERTY->state = INDIGO_OK_STATE;
	//+ mount.MOUNT_PEC_TRAINING.on_change
	if (MOUNT_PEC_TRAINIG_STARTED_ITEM->sw.value) {
		if (MOUNT_PEC_ENABLED_ITEM->sw.value) {
			if (ioptron_set_pec(device, false)) {
				MOUNT_PEC_PROPERTY->state = INDIGO_OK_STATE;
				indigo_set_switch(MOUNT_PEC_PROPERTY, MOUNT_PEC_DISABLED_ITEM, true);
			} else {
				MOUNT_PEC_PROPERTY->state = INDIGO_ALERT_STATE;
			}
			indigo_update_property(device, MOUNT_PEC_PROPERTY, NULL);
		}
		if (!ioptron_set_pec_training(device, true)) {
			MOUNT_PEC_TRAINING_PROPERTY->state = INDIGO_ALERT_STATE;
		}
	} else {
		if (!ioptron_set_pec_training(device, false)) {
			MOUNT_PEC_TRAINING_PROPERTY->state = INDIGO_ALERT_STATE;
		}
	}
	//- mount.MOUNT_PEC_TRAINING.on_change
	indigo_update_property(device, MOUNT_PEC_TRAINING_PROPERTY, NULL);
}

static void mount_meridian_handling_handler(indigo_device *device) {
	MOUNT_MERIDIAN_HANDLING_PROPERTY->state = INDIGO_OK_STATE;
	//+ mount.MOUNT_MERIDIAN_HANDLING.on_change
	if (!ioptron_set_meridian_handling(device,  MOUNT_MERIDIAN_FLIP_ITEM->sw.value, (int)MOUNT_MERIDIAN_LIMIT_ITEM->number.target)) {
		MOUNT_MERIDIAN_HANDLING_PROPERTY->state = INDIGO_ALERT_STATE;
	}
	//- mount.MOUNT_MERIDIAN_HANDLING.on_change
	indigo_update_property(device, MOUNT_MERIDIAN_HANDLING_PROPERTY, NULL);
}

static void mount_meridian_limit_handler(indigo_device *device) {
	MOUNT_MERIDIAN_LIMIT_PROPERTY->state = INDIGO_OK_STATE;
	//+ mount.MOUNT_MERIDIAN_LIMIT.on_change
	if (!ioptron_set_meridian_handling(device,  MOUNT_MERIDIAN_FLIP_ITEM->sw.value, (int)MOUNT_MERIDIAN_LIMIT_ITEM->number.target)) {
		MOUNT_MERIDIAN_LIMIT_PROPERTY->state = INDIGO_ALERT_STATE;
	}
	//- mount.MOUNT_MERIDIAN_LIMIT.on_change
	indigo_update_property(device, MOUNT_MERIDIAN_LIMIT_PROPERTY, NULL);
}

static void mount_protocol_handler(indigo_device *device) {
	MOUNT_PROTOCOL_PROPERTY->state = INDIGO_OK_STATE;
	indigo_update_property(device, MOUNT_PROTOCOL_PROPERTY, NULL);
}

#pragma mark - Device API (mount)

static indigo_result mount_enumerate_properties(indigo_device *device, indigo_client *client, indigo_property *property);

static indigo_result mount_attach(indigo_device *device) {
	if (indigo_mount_attach(device, DRIVER_NAME, DRIVER_VERSION) == INDIGO_OK) {
		ADDITIONAL_INSTANCES_PROPERTY->hidden = device->base_device != NULL;
		DEVICE_PORT_PROPERTY->hidden = false;
		DEVICE_PORTS_PROPERTY->hidden = false;
		indigo_enumerate_serial_ports(device, DEVICE_PORTS_PROPERTY);
		DEVICE_BAUDRATE_PROPERTY->hidden = false;
		//+ mount.on_attach
		INFO_PROPERTY->count = 6;
		MOUNT_ON_COORDINATES_SET_PROPERTY->count = 2;
		MOUNT_TRACK_RATE_PROPERTY->count = 5;
		MOUNT_HOME_PROPERTY->count = 2;
		*DEVICE_BAUDRATE_ITEM->text.value = 0;
		//- mount.on_attach
		MOUNT_PARK_SET_PROPERTY->hidden = false;
		MOUNT_PARK_PROPERTY->hidden = false;
		MOUNT_HOME_PROPERTY->hidden = false;
		MOUNT_GEOGRAPHIC_COORDINATES_PROPERTY->hidden = false;
		MOUNT_EQUATORIAL_COORDINATES_PROPERTY->hidden = false;
		MOUNT_ABORT_MOTION_PROPERTY->hidden = false;
		MOUNT_MOTION_DEC_PROPERTY->hidden = false;
		MOUNT_MOTION_RA_PROPERTY->hidden = false;
		MOUNT_SET_HOST_TIME_PROPERTY->hidden = false;
		MOUNT_UTC_TIME_PROPERTY->hidden = false;
		MOUNT_TRACKING_PROPERTY->hidden = false;
		MOUNT_GUIDE_RATE_PROPERTY->hidden = false;
		MOUNT_PEC_PROPERTY->hidden = false;
		MOUNT_PEC_TRAINING_PROPERTY->hidden = false;
		MOUNT_MERIDIAN_HANDLING_PROPERTY = indigo_init_switch_property(NULL, device->name, MOUNT_MERIDIAN_HANDLING_PROPERTY_NAME, MOUNT_MAIN_GROUP, "Meridian handling", INDIGO_OK_STATE, INDIGO_RW_PERM, INDIGO_ONE_OF_MANY_RULE, 2);
		if (MOUNT_MERIDIAN_HANDLING_PROPERTY == NULL) {
			return INDIGO_FAILED;
		}
		indigo_init_switch_item(MOUNT_MERIDIAN_STOP_ITEM, MOUNT_MERIDIAN_STOP_ITEM_NAME, "Stop at meridian", true);
		indigo_init_switch_item(MOUNT_MERIDIAN_FLIP_ITEM, MOUNT_MERIDIAN_FLIP_ITEM_NAME, "Flip at meridian", false);
		MOUNT_MERIDIAN_HANDLING_PROPERTY->hidden = true;
		MOUNT_MERIDIAN_LIMIT_PROPERTY = indigo_init_number_property(NULL, device->name, MOUNT_MERIDIAN_LIMIT_PROPERTY_NAME, MOUNT_MAIN_GROUP, "Meridian limit", INDIGO_OK_STATE, INDIGO_RW_PERM, 1);
		if (MOUNT_MERIDIAN_LIMIT_PROPERTY == NULL) {
			return INDIGO_FAILED;
		}
		indigo_init_number_item(MOUNT_MERIDIAN_LIMIT_ITEM, MOUNT_MERIDIAN_LIMIT_ITEM_NAME, "Meridian limit [°]", 0, 30, 1, 0);
		MOUNT_MERIDIAN_LIMIT_PROPERTY->hidden = true;
		MOUNT_STATE_PROPERTY->hidden = false;
		MOUNT_TRACK_RATE_PROPERTY->hidden = false;
		MOUNT_PROTOCOL_PROPERTY = indigo_init_switch_property(NULL, device->name, MOUNT_PROTOCOL_PROPERTY_NAME, MAIN_GROUP, "Mount protocol version", INDIGO_OK_STATE, INDIGO_RW_PERM, INDIGO_ONE_OF_MANY_RULE, 7);
		if (MOUNT_PROTOCOL_PROPERTY == NULL) {
			return INDIGO_FAILED;
		}
		indigo_init_switch_item(PROTOCOL_AUTO_ITEM, PROTOCOL_AUTO_ITEM_NAME, "Autodetection", true);
		indigo_init_switch_item(PROTOCOL_8406_ITEM, PROTOCOL_8406_ITEM_NAME, "HC 8406", false);
		indigo_init_switch_item(PROTOCOL_8407_ITEM, PROTOCOL_8407_ITEM_NAME, "HC 8407", false);
		indigo_init_switch_item(PROTOCOL_0100_ITEM, PROTOCOL_0100_ITEM_NAME, "1.0", false);
		indigo_init_switch_item(PROTOCOL_0200_ITEM, PROTOCOL_0200_ITEM_NAME, "2.0", false);
		indigo_init_switch_item(PROTOCOL_0205_ITEM, PROTOCOL_0205_ITEM_NAME, "2.5", false);
		indigo_init_switch_item(PROTOCOL_0300_ITEM, PROTOCOL_0300_ITEM_NAME, "3.0", false);
		INDIGO_DEVICE_ATTACH_LOG(DRIVER_NAME, device->name);
		return mount_enumerate_properties(device, NULL, NULL);
	}
	return INDIGO_FAILED;
}

static indigo_result mount_enumerate_properties(indigo_device *device, indigo_client *client, indigo_property *property) {
	if (IS_CONNECTED) {
		INDIGO_DEFINE_MATCHING_PROPERTY(MOUNT_MERIDIAN_HANDLING_PROPERTY);
		INDIGO_DEFINE_MATCHING_PROPERTY(MOUNT_MERIDIAN_LIMIT_PROPERTY);
		INDIGO_DEFINE_MATCHING_PROPERTY(MOUNT_PROTOCOL_PROPERTY);
	}
	return indigo_mount_enumerate_properties(device, client, property);
}

static indigo_result mount_change_property(indigo_device *device, indigo_client *client, indigo_property *property) {
	if (indigo_property_match_changeable(CONNECTION_PROPERTY, property)) {
		if (!indigo_ignore_connection_change(device, property)) {
			indigo_property_copy_values(CONNECTION_PROPERTY, property, false);
			INDIGO_UPDATE_PROPERTY_STATE(CONNECTION_PROPERTY, INDIGO_BUSY_STATE, NULL);
			indigo_execute_handler(device, mount_connection_handler);
		}
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(MOUNT_PARK_SET_PROPERTY, property)) {
		INDIGO_COPY_VALUES_PROCESS_CHANGE(MOUNT_PARK_SET_PROPERTY, mount_park_set_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(MOUNT_PARK_PROPERTY, property)) {
		INDIGO_COPY_VALUES_PROCESS_CHANGE(MOUNT_PARK_PROPERTY, mount_park_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(MOUNT_HOME_PROPERTY, property)) {
		INDIGO_COPY_VALUES_PROCESS_CHANGE(MOUNT_HOME_PROPERTY, mount_home_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(MOUNT_GEOGRAPHIC_COORDINATES_PROPERTY, property)) {
		INDIGO_COPY_VALUES_PROCESS_CHANGE(MOUNT_GEOGRAPHIC_COORDINATES_PROPERTY, mount_geographic_coordinates_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(MOUNT_EQUATORIAL_COORDINATES_PROPERTY, property)) {
		INDIGO_COPY_TARGETS_PROCESS_CHANGE(MOUNT_EQUATORIAL_COORDINATES_PROPERTY, mount_equatorial_coordinates_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(MOUNT_ABORT_MOTION_PROPERTY, property)) {
		INDIGO_COPY_VALUES_PROCESS_CHANGE(MOUNT_ABORT_MOTION_PROPERTY, mount_abort_motion_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(MOUNT_MOTION_DEC_PROPERTY, property)) {
		INDIGO_COPY_VALUES_PROCESS_CHANGE_ANYTIME(MOUNT_MOTION_DEC_PROPERTY, mount_motion_dec_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(MOUNT_MOTION_RA_PROPERTY, property)) {
		INDIGO_COPY_VALUES_PROCESS_CHANGE_ANYTIME(MOUNT_MOTION_RA_PROPERTY, mount_motion_ra_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(MOUNT_SET_HOST_TIME_PROPERTY, property)) {
		INDIGO_COPY_VALUES_PROCESS_CHANGE(MOUNT_SET_HOST_TIME_PROPERTY, mount_set_host_time_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(MOUNT_UTC_TIME_PROPERTY, property)) {
		INDIGO_COPY_VALUES_PROCESS_CHANGE(MOUNT_UTC_TIME_PROPERTY, mount_utc_time_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(MOUNT_TRACKING_PROPERTY, property)) {
		INDIGO_COPY_VALUES_PROCESS_CHANGE(MOUNT_TRACKING_PROPERTY, mount_tracking_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(MOUNT_GUIDE_RATE_PROPERTY, property)) {
		INDIGO_COPY_VALUES_PROCESS_CHANGE(MOUNT_GUIDE_RATE_PROPERTY, mount_guide_rate_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(MOUNT_PEC_PROPERTY, property)) {
		INDIGO_COPY_VALUES_PROCESS_CHANGE(MOUNT_PEC_PROPERTY, mount_pec_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(MOUNT_PEC_TRAINING_PROPERTY, property)) {
		INDIGO_COPY_VALUES_PROCESS_CHANGE(MOUNT_PEC_TRAINING_PROPERTY, mount_pec_training_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(MOUNT_MERIDIAN_HANDLING_PROPERTY, property)) {
		INDIGO_COPY_VALUES_PROCESS_CHANGE(MOUNT_MERIDIAN_HANDLING_PROPERTY, mount_meridian_handling_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(MOUNT_MERIDIAN_LIMIT_PROPERTY, property)) {
		INDIGO_COPY_VALUES_PROCESS_CHANGE(MOUNT_MERIDIAN_LIMIT_PROPERTY, mount_meridian_limit_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(MOUNT_PROTOCOL_PROPERTY, property)) {
		INDIGO_COPY_VALUES_PROCESS_CHANGE(MOUNT_PROTOCOL_PROPERTY, mount_protocol_handler);
		return INDIGO_OK;
	} else if (indigo_property_match(CONFIG_PROPERTY, property)) {
		if (indigo_switch_match(CONFIG_SAVE_ITEM, property)) {
			indigo_save_property(device, NULL, MOUNT_PROTOCOL_PROPERTY);
		}
	}
	return indigo_mount_change_property(device, client, property);
}

static indigo_result mount_detach(indigo_device *device) {
	if (IS_CONNECTED) {
		indigo_set_switch(CONNECTION_PROPERTY, CONNECTION_DISCONNECTED_ITEM, true);
		mount_connection_handler(device);
	}
	indigo_release_property(MOUNT_MERIDIAN_HANDLING_PROPERTY);
	indigo_release_property(MOUNT_MERIDIAN_LIMIT_PROPERTY);
	indigo_release_property(MOUNT_PROTOCOL_PROPERTY);
	INDIGO_DEVICE_DETACH_LOG(DRIVER_NAME, device->name);
	return indigo_mount_detach(device);
}

#pragma mark - High level code (guider)

static void guider_connection_handler(indigo_device *device) {
	if (CONNECTION_CONNECTED_ITEM->sw.value) {
		bool connection_result = true;
		if (PRIVATE_DATA->count++ == 0) {
			connection_result = ioptron_open(device->master_device);
		}
		if (connection_result) {
			//+ guider.on_connect
			connection_result = ioptron_init_guider(device);
			//- guider.on_connect
		}
		if (connection_result) {
			CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
			indigo_send_message(device, OK_PROPERTY, "Connected to %s on %s", GUIDER_DEVICE_NAME, DEVICE_PORT_ITEM->text.value);
		} else {
			indigo_send_message(device, ALERT_PROPERTY, "Failed to connect to %s on %s", GUIDER_DEVICE_NAME, DEVICE_PORT_ITEM->text.value);
			PRIVATE_DATA->count--;
			CONNECTION_PROPERTY->state = INDIGO_ALERT_STATE;
			indigo_set_switch(CONNECTION_PROPERTY, CONNECTION_DISCONNECTED_ITEM, true);
		}
	} else {
		indigo_cancel_pending_handlers(device);
		if (--PRIVATE_DATA->count == 0) {
			ioptron_close(device);
		}
		indigo_send_message(device, OK_PROPERTY, "Disconnected from %s", device->name);
		CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
	}
	indigo_guider_change_property(device, NULL, CONNECTION_PROPERTY);
}

static void guider_guide_dec_handler(indigo_device *device) {
	//+ guider.GUIDER_GUIDE_DEC.on_change
	int north = (int)GUIDER_GUIDE_NORTH_ITEM->number.value;
	int south = (int)GUIDER_GUIDE_SOUTH_ITEM->number.value;
	ioptron_guide_dec(device, north, south);
	if (north > 0) {
		indigo_execute_priority_handler_in(device, INDIGO_TASK_PRIORITY_URGENT, ((double)north) / 1000.0, guider_guide_dec_finalizer);
	} else if (south > 0) {
		indigo_execute_priority_handler_in(device, INDIGO_TASK_PRIORITY_URGENT, ((double)south) / 1000.0, guider_guide_dec_finalizer);
	} else {
		guider_guide_dec_finalizer(device);
	}
	//- guider.GUIDER_GUIDE_DEC.on_change
}

static void guider_guide_ra_handler(indigo_device *device) {
	//+ guider.GUIDER_GUIDE_RA.on_change
	int west = (int)GUIDER_GUIDE_WEST_ITEM->number.value;
	int east = (int)GUIDER_GUIDE_EAST_ITEM->number.value;
	ioptron_guide_ra(device, west, east);
	if (west > 0) {
		indigo_execute_priority_handler_in(device, INDIGO_TASK_PRIORITY_URGENT, ((double)west) / 1000.0, guider_guide_ra_finalizer);
	} else if (east > 0) {
		indigo_execute_priority_handler_in(device, INDIGO_TASK_PRIORITY_URGENT, ((double)east) / 1000.0, guider_guide_ra_finalizer);
	} else {
		guider_guide_ra_finalizer(device);
	}
	//- guider.GUIDER_GUIDE_RA.on_change
}

static void guider_rate_handler(indigo_device *device) {
	GUIDER_RATE_PROPERTY->state = INDIGO_OK_STATE;
	//+ guider.GUIDER_RATE.on_change
	if (!ioptron_set_guide_rate(device, (int)GUIDER_RATE_ITEM->number.value, (int)GUIDER_DEC_RATE_ITEM->number.value)) {
		GUIDER_RATE_PROPERTY->state = INDIGO_ALERT_STATE;
	}
	//- guider.GUIDER_RATE.on_change
	indigo_update_property(device, GUIDER_RATE_PROPERTY, NULL);
}

#pragma mark - Device API (guider)

static indigo_result guider_enumerate_properties(indigo_device *device, indigo_client *client, indigo_property *property);

static indigo_result guider_attach(indigo_device *device) {
	if (indigo_guider_attach(device, DRIVER_NAME, DRIVER_VERSION) == INDIGO_OK) {
		GUIDER_GUIDE_DEC_PROPERTY->hidden = false;
		GUIDER_GUIDE_RA_PROPERTY->hidden = false;
		GUIDER_RATE_PROPERTY->hidden = false;
		INDIGO_DEVICE_ATTACH_LOG(DRIVER_NAME, device->name);
		return guider_enumerate_properties(device, NULL, NULL);
	}
	return INDIGO_FAILED;
}

static indigo_result guider_enumerate_properties(indigo_device *device, indigo_client *client, indigo_property *property) {
	return indigo_guider_enumerate_properties(device, client, property);
}

static indigo_result guider_change_property(indigo_device *device, indigo_client *client, indigo_property *property) {
	if (indigo_property_match_changeable(CONNECTION_PROPERTY, property)) {
		if (!indigo_ignore_connection_change(device, property)) {
			indigo_property_copy_values(CONNECTION_PROPERTY, property, false);
			INDIGO_UPDATE_PROPERTY_STATE(CONNECTION_PROPERTY, INDIGO_BUSY_STATE, NULL);
			indigo_execute_handler(device, guider_connection_handler);
		}
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(GUIDER_GUIDE_DEC_PROPERTY, property)) {
		INDIGO_COPY_VALUES_PROCESS_CHANGE(GUIDER_GUIDE_DEC_PROPERTY, guider_guide_dec_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(GUIDER_GUIDE_RA_PROPERTY, property)) {
		INDIGO_COPY_VALUES_PROCESS_CHANGE(GUIDER_GUIDE_RA_PROPERTY, guider_guide_ra_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(GUIDER_RATE_PROPERTY, property)) {
		INDIGO_COPY_VALUES_PROCESS_CHANGE(GUIDER_RATE_PROPERTY, guider_rate_handler);
		return INDIGO_OK;
	}
	return indigo_guider_change_property(device, client, property);
}

static indigo_result guider_detach(indigo_device *device) {
	if (IS_CONNECTED) {
		indigo_set_switch(CONNECTION_PROPERTY, CONNECTION_DISCONNECTED_ITEM, true);
		guider_connection_handler(device);
	}
	INDIGO_DEVICE_DETACH_LOG(DRIVER_NAME, device->name);
	return indigo_guider_detach(device);
}

#pragma mark - Device templates

static indigo_device mount_template = INDIGO_DEVICE_INITIALIZER(MOUNT_DEVICE_NAME, mount_attach, mount_enumerate_properties, mount_change_property, NULL, mount_detach);

static indigo_device guider_template = INDIGO_DEVICE_INITIALIZER(GUIDER_DEVICE_NAME, guider_attach, guider_enumerate_properties, guider_change_property, NULL, guider_detach);

#pragma mark - Main code

indigo_result indigo_mount_ioptron(indigo_driver_action action, indigo_driver_info *info) {
	static indigo_driver_action last_action = INDIGO_DRIVER_SHUTDOWN;
	static ioptron_private_data *private_data = NULL;
	static indigo_device *mount = NULL;
	static indigo_device *guider = NULL;

	SET_DRIVER_INFO(info, DRIVER_LABEL, __FUNCTION__, DRIVER_VERSION, false, last_action);

	if (action == last_action) {
		return INDIGO_OK;
	}

	switch (action) {
		case INDIGO_DRIVER_INIT:
			last_action = action;
			private_data = indigo_safe_malloc(sizeof(ioptron_private_data));
			mount = indigo_safe_malloc_copy(sizeof(indigo_device), &mount_template);
			mount->private_data = private_data;
			indigo_attach_device(mount);
			guider = indigo_safe_malloc_copy(sizeof(indigo_device), &guider_template);
			guider->private_data = private_data;
			guider->master_device = mount;
			indigo_attach_device(guider);
			break;

		case INDIGO_DRIVER_SHUTDOWN:
			VERIFY_NOT_CONNECTED(mount);
			VERIFY_NOT_CONNECTED(guider);
			last_action = action;
			if (mount != NULL) {
				indigo_detach_device(mount);
				free(mount);
				mount = NULL;
			}
			if (guider != NULL) {
				indigo_detach_device(guider);
				free(guider);
				guider = NULL;
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
