// Copyright (c) 2021 CloudMakers, s. r. o.
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

/** INDIGO ASCOM ALPACA bridge agent
 \file alpaca_common.h
 */

#ifndef alpaca_common_h
#define alpaca_common_h

#include <stdbool.h>

#include <indigo/indigo_bus.h>

#define ALPACA_INTERFACE_VERSION	1

typedef enum {
	indigo_alpaca_error_OK = 0
} indigo_alpaca_error;

typedef struct indigo_alpaca_device_struct {
	uint64_t indigo_interface;
	char indigo_device[INDIGO_NAME_SIZE];
	char device_name[INDIGO_NAME_SIZE];
	char driver_info[INDIGO_VALUE_SIZE];
	char driver_version[INDIGO_VALUE_SIZE];
	char *device_type;
	uint32_t device_number;
	char device_uid[40];
	pthread_mutex_t mutex;
	bool connected;
	union {
		struct {
			uint32_t position;
			uint32_t focusoffsets[32];
			char *names[32];
		} filterwheel;
	};
	struct indigo_alpaca_device_struct *next;
} indigo_alpaca_device;

extern void indigo_alpaca_update_property(indigo_alpaca_device *alpaca_device, indigo_property *property);
extern indigo_alpaca_error indigo_alpaca_get_devicename(indigo_alpaca_device *device, int version, char *value);
extern indigo_alpaca_error indigo_alpaca_get_description(indigo_alpaca_device *device, int version, char *value);
extern indigo_alpaca_error indigo_alpaca_get_driverinfo(indigo_alpaca_device *device, int version, char *value);
extern indigo_alpaca_error indigo_alpaca_get_driverversion(indigo_alpaca_device *device, int version, char *value);
extern indigo_alpaca_error indigo_alpaca_get_interfaceversion(indigo_alpaca_device *device, int version, uint32_t *value);
extern indigo_alpaca_error indigo_alpaca_get_connected(indigo_alpaca_device *device, int version, bool *value);
extern indigo_alpaca_error indigo_alpaca_set_connected(indigo_alpaca_device *device, int version, bool value);

extern void indigo_alpaca_wheel_update_property(indigo_alpaca_device *alpaca_device, indigo_property *property);
extern indigo_alpaca_error indigo_alpaca_wheel_get_position(indigo_alpaca_device *device, int version, uint32_t *value);
extern indigo_alpaca_error indigo_alpaca_wheel_set_position(indigo_alpaca_device *device, int version, uint32_t value);
extern indigo_alpaca_error indigo_alpaca_wheel_get_focusoffsets(indigo_alpaca_device *device, int version, uint32_t **value);
extern indigo_alpaca_error indigo_alpaca_wheel_get_names(indigo_alpaca_device *device, int version, char **value);

extern indigo_device *indigo_agent_alpaca_device;
extern indigo_client *indigo_agent_alpaca_client;

#endif /* alpaca_common_h */
