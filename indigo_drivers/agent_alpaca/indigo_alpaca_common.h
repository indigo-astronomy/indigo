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
#define ALPACA_MAX_SWITCHES				8
#define ALPACA_MAX_ITEMS					128

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
	indigo_device_interface indigo_interface;
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
			bool canabortexposure;
			bool cangetcoolerpower;
			bool cangetccdtemperature;
			bool cansetccdtemperature;
			int binx;
			int biny;
			int camerastate;
			int cameraxsize;
			int cameraysize;
			int startx;
			int starty;
			int numx;
			int numy;
			double ccdtemperature;
			double setccdtemperature;
			bool cooleron;
			double coolerpower;
			double electronsperadu;
			double fullwellcapacity;
			indigo_item *imageready;
			int maxadu;
			double pixelsizex;
			double pixelsizey;
			double exposuremin;
			double exposuremax;
			double lastexposureduration;
			char lastexposuretarttime[20];
			int gainmin;
			int gainmax;
			int gain;
			int offsetmin;
			int offsetmax;
			int offset;
			char *readoutmodes_names[ALPACA_MAX_ITEMS];
			char *readoutmodes_labels[ALPACA_MAX_ITEMS];
			int readoutmode;
			indigo_item *bayer_matrix;
		} ccd;
		struct {
			int count;
			int position;
			int focusoffsets[ALPACA_MAX_FILTERS];
			char *names[ALPACA_MAX_FILTERS];
		} wheel;
		struct {
			bool absolute;
			bool ismoving;
			bool tempcompavailable;
			bool tempcomp;
			bool temperatureavailable;
			bool halted;
			int offset;
			int maxstep;
			int maxincrement;
			int position;
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
			bool haslimits;
			double min;
			double max;
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
			bool cansetaltitude;
			double altitude;
			bool cansetazimuth;
			bool cansyncazimuth;
			double azimuth;
			bool canfindhome;
			bool athome;
			bool atpark;
			bool canpark;
			bool cansetpark;
			bool cansetshutter;
			bool canslave;
			int shutterstatus;
			bool slaved;
			bool isrotating;
			bool isshuttermoving;
			bool isflapmoving;
		} dome;
		struct {
			int maxswitch_power_outlet;
			int maxswitch_heater_outlet;
			int maxswitch_usb_port;
			int maxswitch_gpio_outlet;
			int maxswitch_gpio_sensor;
			bool canwrite[5 * ALPACA_MAX_SWITCHES];
			char switchname[5 * ALPACA_MAX_SWITCHES][INDIGO_VALUE_SIZE];
			double switchvalue[5 * ALPACA_MAX_SWITCHES];
			double minswitchvalue[5 * ALPACA_MAX_SWITCHES];
			double maxswitchvalue[5 * ALPACA_MAX_SWITCHES];
			double switchstep[5 * ALPACA_MAX_SWITCHES];
			bool valueset[5];
			bool nameset[5];
		} sw;
		struct {
			bool canpulseguide;
			bool cansetguiderates;
			bool ispulseguiding;
			double guideratedeclination;
			double guideraterightascension;
		} guider;
		struct {
			int calibratorstate;
			int brightness;
			int maxbrightness;
			int coverstate;
		} covercalibrator;
	};
	struct indigo_alpaca_device_struct *next;
} indigo_alpaca_device;

typedef enum {
	indigo_alpaca_type_unknown = 0, // 0 to 3 are values already used in the Alpaca standard
	indigo_alpaca_type_int16 = 1,
	indigo_alpaca_type_int32 = 2,
	indigo_alpaca_type_double = 3,
	indigo_alpaca_type_single = 4, // 4 to 9 are an extension to include other numeric types
	indigo_alpaca_type_uint64 = 5,
	indigo_alpaca_type_byte = 6,
	indigo_alpaca_type_int64 = 7,
	indigo_alpaca_type_uint16 = 8,
	indigo_alpaca_type_uint32 = 9
} indigo_alpaca_element_type;

typedef struct {
	int metadata_version; // Bytes 0..3 - Metadata version = 1
	int error_number; // Bytes 4..7 - Alpaca error number or zero for success
	int client_transaction_id; // Bytes 8..11 - Client's transaction ID
	int server_transaction_id; // Bytes 12..15 - Device's transaction ID
	int data_start; // Bytes 16..19 - Offset of the start of the data bytes
	int image_element_type; // Bytes 20..23 - Element type of the source image array
	int transmission_element_type; // Bytes 24..27 - Element type as sent over the network
	int rank; // Bytes 28..31 - Image array rank (2 or 3)
	int dimension1; // Bytes 32..35 - Length of image array first dimension
	int dimension2; // Bytes 36..39 - Length of image array second dimension
	int dimension3; // Bytes 40..43 - Length of image array third dimension (0 for 2D array)
} indigo_alpaca_metadata;

extern bool get_bayer_RGGB_offsets(const char *pattern, int *x_offset, int *y_offset);

extern char *indigo_alpaca_error_string(int code);
extern long indigo_alpaca_append_error(char *buffer, long buffer_length, indigo_alpaca_error result);
extern long indigo_alpaca_append_value_bool(char *buffer, long buffer_length, bool value, indigo_alpaca_error result);
extern long indigo_alpaca_append_value_int(char *buffer, long buffer_length, int value, indigo_alpaca_error result);
extern long indigo_alpaca_append_value_double(char *buffer, long buffer_length, double value, indigo_alpaca_error result);
extern long indigo_alpaca_append_value_string(char *buffer, long buffer_length, char *value, indigo_alpaca_error result);
extern bool indigo_alpaca_wait_for_bool(bool *reference, bool value, int timeout);
extern bool indigo_alpaca_wait_for_int32(int *reference, int value, int timeout);
extern bool indigo_alpaca_wait_for_double(double *reference, double value, int timeout);

extern void indigo_alpaca_update_property(indigo_alpaca_device *alpaca_device, indigo_property *property);
extern long indigo_alpaca_get_command(indigo_alpaca_device *alpaca_device, int version, char *command, int id, char *buffer, long buffer_length);
extern long indigo_alpaca_set_command(indigo_alpaca_device *alpaca_device, int version, char *command, char *buffer, long buffer_length, char *param_1, char *param_2);

extern void indigo_alpaca_ccd_update_property(indigo_alpaca_device *alpaca_device, indigo_property *property);
extern long indigo_alpaca_ccd_get_command(indigo_alpaca_device *alpaca_device, int version, char *command, char *buffer, long buffer_length);
extern long indigo_alpaca_ccd_set_command(indigo_alpaca_device *alpaca_device, int version, char *command, char *buffer, long buffer_length, char *param_1, char *param_2);
extern void indigo_alpaca_ccd_get_imagearray(indigo_alpaca_device *alpaca_device, int version, indigo_uni_handle *handle, int client_transaction_id, int server_transaction_id, bool use_gzip, bool use_imagebytes);

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

extern void indigo_alpaca_dome_update_property(indigo_alpaca_device *alpaca_device, indigo_property *property);
extern long indigo_alpaca_dome_get_command(indigo_alpaca_device *alpaca_device, int version, char *command, char *buffer, long buffer_length);
extern long indigo_alpaca_dome_set_command(indigo_alpaca_device *alpaca_device, int version, char *command, char *buffer, long buffer_length, char *param_1, char *param_2);

extern void indigo_alpaca_switch_update_property(indigo_alpaca_device *alpaca_device, indigo_property *property);
extern long indigo_alpaca_switch_get_command(indigo_alpaca_device *alpaca_device, int version, char *command, int id, char *buffer, long buffer_length);
extern long indigo_alpaca_switch_set_command(indigo_alpaca_device *alpaca_device, int version, char *command, char *buffer, long buffer_length, char *param_1, char *param_2);

extern indigo_device *indigo_agent_alpaca_device;
extern indigo_client *indigo_agent_alpaca_client;

#define IS_DEVICE_TYPE(device, type) ((device->indigo_interface & type) == type)

#endif /* alpaca_common_h */
