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
#define ALPACA_MAX_FILTERS				32

typedef enum {
	indigo_alpaca_error_OK = 0x000,
	indigo_alpaca_error_NotImplemented = 0x400,
	indigo_alpaca_error_InvalidValue = 0x401,
	indigo_alpaca_error_ValueNotSet = 0x402,
	indigo_alpaca_error_NotConnected = 0x407,
	indigo_alpaca_error_InvalidWhileParked = 0x408,
	indigo_alpaca_error_InvalidWhileSlaved = 0x409,
	indigo_alpaca_error_InvalidOperation = 0x40B,
	indigo_alpaca_error_ActionNotImplemented = 0x40C
} indigo_alpaca_error;

typedef struct indigo_alpaca_device_struct {
	uint64_t indigo_interface;
	char indigo_device[INDIGO_NAME_SIZE];
	char device_name[INDIGO_NAME_SIZE];
	char driver_info[INDIGO_VALUE_SIZE];
	char driver_version[INDIGO_VALUE_SIZE];
	char *device_type;
	int device_number;
	char device_uid[40];
	pthread_mutex_t mutex;
	bool connected;
	char utcdate[64];
	double latitude;
	double longitude;
	double elevation;
	union {
		struct {
			uint32_t count;
			int32_t position;
			uint32_t focusoffsets[ALPACA_MAX_FILTERS];
			char *names[ALPACA_MAX_FILTERS];
		} filterwheel;
		struct {
			bool absolute;
			bool ismoving;
			bool tempcompavailable;
			bool tempcomp;
			bool temperatureavailable;
			bool halted;
			int32_t offset;
			uint32_t maxstep;
			uint32_t maxincrement;
			uint32_t position;
			double temperature;
		} focuser;
		struct {
			bool canreverse;
			bool ismoving;
			double mechanicalposition;
			double position;
			double targetposition;
			bool reversed;
			double stepsize;
		} rotator;
		struct {
			bool cansetguiderates;
			double guideratedeclination;
			double guideraterightascension;
			bool canpark;
			bool canunpark;
			bool canslew;
			bool cansync;
			bool cansettracking;
			bool athome;
			bool atpark;
			double siderealtime;
			int equatorialsystem;
			double declination;
			double rightascension;
			double altitude;
			double azimuth;
			bool targetdeclinationset;
			bool targetrightascensionset;
			double targetdeclination;
			double targetrightascension;
			bool slewing;
			bool tracking;
			int trackingrate;
			bool trackingrates[4];
			int sideofpier;
		} mount;
		struct {
			bool canpulseguide;
			bool cansetguiderates;
			bool ispulseguiding;
			double guideratedeclination;
			double guideraterightascension;
		} guider;
		struct {
			int calibratorstate;
			uint32_t brightness;
			uint32_t maxbrightness;
			int coverstate;
		} covercalibrator;
	};
	struct indigo_alpaca_device_struct *next;
} indigo_alpaca_device;

extern char *indigo_alpaca_error_string(int code);
extern long indigo_alpaca_append_error(char *buffer, long buffer_length, indigo_alpaca_error result);
extern long indigo_alpaca_append_value_bool(char *buffer, long buffer_length, bool value, indigo_alpaca_error result);
extern long indigo_alpaca_append_value_int(char *buffer, long buffer_length, int value, indigo_alpaca_error result);
extern long indigo_alpaca_append_value_double(char *buffer, long buffer_length, double value, indigo_alpaca_error result);

extern bool indigo_alpaca_wait_for_bool(bool *reference, bool value, int timeout);
extern bool indigo_alpaca_wait_for_int32(int32_t *reference, int32_t value, int timeout);
extern bool indigo_alpaca_wait_for_double(double *reference, double value, int timeout);

extern void indigo_alpaca_update_property(indigo_alpaca_device *alpaca_device, indigo_property *property);
extern long indigo_alpaca_get_command(indigo_alpaca_device *alpaca_device, int version, char *command, char *buffer, long buffer_length);
extern long indigo_alpaca_set_command(indigo_alpaca_device *alpaca_device, int version, char *command, char *buffer, long buffer_length, char *param_1, char *param_2);

extern void indigo_alpaca_wheel_update_property(indigo_alpaca_device *alpaca_device, indigo_property *property);
extern long indigo_alpaca_wheel_get_command(indigo_alpaca_device *alpaca_device, int version, char *command, char *buffer, long buffer_length);
extern long indigo_alpaca_wheel_set_command(indigo_alpaca_device *alpaca_device, int version, char *command, char *buffer, long buffer_length, char *param_1, char *param_2);

extern void indigo_alpaca_focuser_update_property(indigo_alpaca_device *alpaca_device, indigo_property *property);
extern long indigo_alpaca_focuser_get_command(indigo_alpaca_device *alpaca_device, int version, char *command, char *buffer, long buffer_length);
extern long indigo_alpaca_focuser_set_command(indigo_alpaca_device *alpaca_device, int version, char *command, char *buffer, long buffer_length, char *param_1, char *param_2);

extern void indigo_alpaca_mount_update_property(indigo_alpaca_device *alpaca_device, indigo_property *property);
extern long indigo_alpaca_mount_get_command(indigo_alpaca_device *alpaca_device, int version, char *command, char *buffer, long buffer_length);
extern long indigo_alpaca_mount_set_command(indigo_alpaca_device *alpaca_device, int version, char *command, char *buffer, long buffer_length, char *param_1, char *param_2);

extern void indigo_alpaca_guider_update_property(indigo_alpaca_device *alpaca_device, indigo_property *property);
extern long indigo_alpaca_guider_get_command(indigo_alpaca_device *alpaca_device, int version, char *command, char *buffer, long buffer_length);
extern long indigo_alpaca_guider_set_command(indigo_alpaca_device *alpaca_device, int version, char *command, char *buffer, long buffer_length, char *param_1, char *param_2);

extern void indigo_alpaca_lightbox_update_property(indigo_alpaca_device *alpaca_device, indigo_property *property);
extern long indigo_alpaca_lightbox_get_command(indigo_alpaca_device *alpaca_device, int version, char *command, char *buffer, long buffer_length);
extern long indigo_alpaca_lightbox_set_command(indigo_alpaca_device *alpaca_device, int version, char *command, char *buffer, long buffer_length, char *param_1, char *param_2);

extern void indigo_alpaca_rotator_update_property(indigo_alpaca_device *alpaca_device, indigo_property *property);
extern long indigo_alpaca_rotator_get_command(indigo_alpaca_device *alpaca_device, int version, char *command, char *buffer, long buffer_length);
extern long indigo_alpaca_rotator_set_command(indigo_alpaca_device *alpaca_device, int version, char *command, char *buffer, long buffer_length, char *param_1, char *param_2);

extern indigo_device *indigo_agent_alpaca_device;
extern indigo_client *indigo_agent_alpaca_client;

#endif /* alpaca_common_h */
