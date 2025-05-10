// Copyright (c) 2016 CloudMakers, s. r. o.
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

/** INDIGO Bus
 \file indigo_bus.h
 */

#ifndef indigo_bus_h
#define indigo_bus_h

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdarg.h>
#include <string.h>
#include <pthread.h>
#include <assert.h>
#include <indigo/indigo_config.h>
#include <indigo/indigo_token.h>
#include <indigo/indigo_uni_io.h>

#define INDIGO_VERSION_3

#if defined(INDIGO_WINDOWS)
#if defined(INDIGO_WINDOWS_DLL)
#define INDIGO_EXTERN __declspec(dllexport)
#else
#define INDIGO_EXTERN __declspec(dllimport)
#endif
#pragma warning(disable:4200)
#else
#define INDIGO_EXTERN extern
#endif

#ifdef __cplusplus
extern "C" {
#endif

/** Device, property or item name size.
 */
#define INDIGO_NAME_SIZE      128

/** Item value size.
 */
#define INDIGO_VALUE_SIZE     512

/** Pre-allocated item count for property buffers.
 */
#define INDIGO_PREALLOCATED_COUNT     512

// forward definitions

typedef struct indigo_client indigo_client;
typedef struct indigo_device indigo_device;

/** Device interface (value should be used for INFO_DEVICE_INTERFACE_ITEM->text.value)
 */
typedef enum {
	INDIGO_INTERFACE_MOUNT     = (1 << 0), 			   					///< Mount interface
	INDIGO_INTERFACE_CCD       = (1 << 1),    							///< CCD interface
	INDIGO_INTERFACE_GUIDER    = (1 << 2),    							///< Guider interface
	INDIGO_INTERFACE_FOCUSER   = (1 << 3),   								///< Focuser interface
	INDIGO_INTERFACE_WHEEL     = (1 << 4),    							///< Filter wheel interface
	INDIGO_INTERFACE_DOME      = (1 << 5),    							///< Dome interface
	INDIGO_INTERFACE_GPS       = (1 << 6),    							///< GPS interface
	INDIGO_INTERFACE_AO        = (1 << 8),    							///< Adaptive Optics Interface
	INDIGO_INTERFACE_ROTATOR   = (1 << 12),   							///< Rotator interface
	INDIGO_INTERFACE_AGENT     = (1 << 14),  								///< Agent interface
	INDIGO_INTERFACE_AUX       = (1 << 15),    							///< Auxiliary interface
	INDIGO_INTERFACE_AUX_JOYSTICK  = (1 << 15) | (1 << 16),	///< Joystick AUX interface
	INDIGO_INTERFACE_AUX_SHUTTER   = (1 << 15) | (1 << 17),	///< Shutter AUX interface
	INDIGO_INTERFACE_AUX_POWERBOX  = (1 << 15) | (1 << 18),	///< Powerbox AUX interface
	INDIGO_INTERFACE_AUX_SQM       = (1 << 15) | (1 << 19),	///< SQM AUX interface
	INDIGO_INTERFACE_AUX_DUSTCAP   = (1 << 15) | (1 << 20), ///< Dust Cap AUX Interface
	INDIGO_INTERFACE_AUX_LIGHTBOX  = (1 << 15) | (1 << 21), ///< Light Box AUX Interface
	INDIGO_INTERFACE_AUX_WEATHER   = (1 << 15) | (1 << 22), ///< Weather AUX interface
	INDIGO_INTERFACE_AUX_GPIO      = (1 << 15) | (1 << 23) ///< General purpose IO AUX interface
} indigo_device_interface;

/** Property, device or client version.
 Version value is used to map property or item names correctly or to trigger for version specific property handling. Do not use other than defined values!
 */
typedef enum {
	INDIGO_VERSION_NONE			= 0x0000, ///< undefined version
	INDIGO_VERSION_LEGACY		= 0x0107, ///< INDI compatible version
	INDIGO_VERSION_2_0			= 0x0200, ///< INDIGO version
	INDIGO_VERSION_3_0			= 0x0300, ///< INDIGO version
	INDIGO_VERSION_CURRENT	= 0x0300  ///< INDIGO version
} indigo_version;

/** Bus operation return status.
 */
typedef enum {
	INDIGO_OK = 0,              ///< success
	INDIGO_FAILED,              ///< unspecified error
	INDIGO_TOO_MANY_ELEMENTS,   ///< too many clients/devices/properties/items etc.
	INDIGO_LOCK_ERROR,          ///< mutex lock error
	INDIGO_NOT_FOUND,           ///< unknown client/device/property/item etc.
	INDIGO_CANT_START_SERVER,   ///< network server start failure
	INDIGO_DUPLICATED,					///< duplicated items etc.
	INDIGO_BUSY,                ///< operation failed because the resourse is busy.
	INDIGO_GUIDE_ERROR,         ///< Guide process error (srar lost, SNR too low etc..).
	INDIGO_UNSUPPORTED_ARCH,    ///< Unsupported architecture.
	INDIGO_UNRESOLVED_DEPS			///< Unresolved dependencies (missing library, executable, ...).
} indigo_result;

/** Property data type.
 */
typedef enum {
	INDIGO_TEXT_VECTOR = 1,     ///< strings of limited width
	INDIGO_NUMBER_VECTOR,       ///< float numbers with defined min, max values and increment
	INDIGO_SWITCH_VECTOR,       ///< logical values representing “on” and “off” state
	INDIGO_LIGHT_VECTOR,        ///< status values with four possible values INDIGO_IDLE_STATE, INDIGO_OK_STATE, INDIGO_BUSY_STATE and INDIGO_ALERT_STATE
	INDIGO_BLOB_VECTOR          ///< binary data of any type and any length
} indigo_property_type;

/** Textual representations of indigo_property_type values.
 */
INDIGO_EXTERN char *indigo_property_type_text[];

/** Property state.
 */
typedef enum {
	INDIGO_IDLE_STATE = 0,      ///< property is passive (unused by INDIGO)
	INDIGO_OK_STATE,            ///< property is in correct state or if operation on property was successful
	INDIGO_BUSY_STATE,          ///< property is transient state or if operation on property is pending
	INDIGO_ALERT_STATE          ///< property is in incorrect state or if operation on property failed
} indigo_property_state;

/** Textual representations of indigo_property_state values.
 */
INDIGO_EXTERN char *indigo_property_state_text[];

/** Property access permission.
 */
typedef enum {
	INDIGO_RO_PERM = 1,         ///< read-only
	INDIGO_RW_PERM,             ///< read-write
	INDIGO_WO_PERM              ///< write-only
} indigo_property_perm;

/** Textual representation of indigo_property_perm values.
 */
INDIGO_EXTERN char *indigo_property_perm_text[];

/** Switch behaviour rule.
 */
typedef enum {
	INDIGO_ONE_OF_MANY_RULE = 1,///< radio button group like behaviour with one switch in "on" state
	INDIGO_AT_MOST_ONE_RULE,    ///< radio button group like behaviour with none or one switch in "on" state
	INDIGO_ANY_OF_MANY_RULE     ///< checkbox button group like behaviour
} indigo_rule;

/** Textual representation of indigo_rule values.
 */
INDIGO_EXTERN char *indigo_switch_rule_text[];

typedef enum {
	INDIGO_ENABLE_BLOB_ALSO,
	INDIGO_ENABLE_BLOB_NEVER,
	INDIGO_ENABLE_BLOB_URL
} indigo_enable_blob_mode;

#define INDIGO_ENABLE_BLOB INDIGO_ENABLE_BLOB_URL

/** Enable BLOB mode record
 */

typedef struct indigo_enable_blob_mode_record {
	char device[INDIGO_NAME_SIZE];				///< device name
	char name[INDIGO_NAME_SIZE];					///< property name
	indigo_enable_blob_mode mode;					///< mode
	struct indigo_enable_blob_mode_record *next; ///< next record
} indigo_enable_blob_mode_record;

/** RAW image header.
 */

typedef struct {
	uint32_t signature; // 8bit mono = RAW1 = 0x31574152, 16bit mono = RAW2 = 0x32574152, 24bit RGB = RAW3 = 0x33574152
	uint32_t width;
	uint32_t height;
} indigo_raw_header;

/** RAW image type
 */

typedef enum {
	INDIGO_RAW_MONO8 = 0x31574152,
	INDIGO_RAW_MONO16 = 0x32574152,
	INDIGO_RAW_RGB24 = 0x33574152,
	INDIGO_RAW_RGBA32 = 0x36424752,
	INDIGO_RAW_ABGR32 = 0x36524742,
	INDIGO_RAW_RGB48 = 0x36574152
} indigo_raw_type;


typedef enum {
	INDIGO_LOG_PLAIN = 0,
	INDIGO_LOG_ERROR,
	INDIGO_LOG_INFO,
	INDIGO_LOG_DEBUG,
	INDIGO_LOG_TRACE_BUS,
	INDIGO_LOG_TRACE,
	INDIGO_LOG_NONE = 9999
} indigo_log_levels;

/** Property item definition.
 */
typedef struct {/* there is no .name =  because of g++ C99 bug affecting string initialier */
	char name[INDIGO_NAME_SIZE];        ///< property wide unique item name
	char label[INDIGO_VALUE_SIZE];      ///< item description in human readable form
	char hints[INDIGO_VALUE_SIZE];			///< item GUI hints
	union {
		/** Text property item specific fields.
		 */
		struct {
			char value[INDIGO_VALUE_SIZE];  ///< item value (for text properties)
			char *long_value;								///< item value, set if text is longer than NDIGO_VALUE_SIZE
			long length;										///< text length (including terminating 0)
		} text;
		/** Number property item specific fields.
		 */
		struct {/* there is no .name =  because of g++ C99 bug affecting string initialier */
			char format[INDIGO_VALUE_SIZE]; ///< item format (for number properties)
			double min;                     ///< item min value (for number properties)
			double max;                     ///< item max value (for number properties)
			double step;                    ///< item increment value (for number properties)
			double value;                   ///< item value (for number properties)
			double target;                  ///< item target value (for number properties)
		} number;
		/** Switch property item specific fields.
		 */
		struct {
			bool value;                     ///< item value (for switch properties)
		} sw;
		/** Light property item specific fields.
		 */
		struct {
			indigo_property_state value;    ///< item value (for light properties)
		} light;
		/** BLOB property item specific fields.
		 */
		struct {
			char format[INDIGO_NAME_SIZE];  ///< item format (for blob properties), known file type suffix like ".fits" or ".jpeg"
			char url[INDIGO_VALUE_SIZE];		///< item URL on source server
			long size;                      ///< item size (for blob properties) in bytes
			void *value;                    ///< item value (for blob properties)
		} blob;
	};
} indigo_item;

/** Property definition.
 */
typedef struct {
	char device[INDIGO_NAME_SIZE];      ///< system wide unique device name
	char name[INDIGO_NAME_SIZE];        ///< device wide unique property name
	char group[INDIGO_NAME_SIZE];       ///< property group in human readable form (presented as a tab or a subtree in GUI
	char label[INDIGO_VALUE_SIZE];      ///< property description in human readable form
	char hints[INDIGO_VALUE_SIZE];			///< property GUI hints
	indigo_property_state state;        ///< property state
	indigo_property_type type;          ///< property type
	indigo_property_perm perm;          ///< property access permission
	indigo_rule rule;                   ///< switch behaviour rule (for switch properties)
	indigo_token access_token;					///< allow change request on locked device
	short version;                      ///< property version INDIGO_VERSION_NONE, INDIGO_VERSION_LEGACY or INDIGO_VERSION_2_0
	bool hidden;                        ///< property is hidden/unused by  driver (for optional properties)
	bool defined;												///< property is defined
	int allocated_count;                ///< number of allocated property items
	int count;                          ///< number of used property items
	indigo_item items[];                ///< property items
} indigo_property;

/** Device structure definition
 */
typedef struct indigo_device {
	char name[INDIGO_NAME_SIZE];        ///< device name
	indigo_uni_handle *lock;            ///< device global lock
	bool is_remote;                     ///< is remote device
	uint16_t gp_bits;                   ///< general purpose bits for driver specific usage
	void *device_context;               ///< any device specific data
	void *private_data;                 ///< private data
	indigo_device *master_device;       ///< if the device provides many logical devices, this must point to one of the locical devices, otherwise is safe to be NULL
	indigo_result last_result;          ///< result of last bus operation
	indigo_version version;             ///< device version
	indigo_token access_token;			///< allow change request with correct access token only
	void *match_patterns;               ///< device matching patterns
	int match_patterns_count;           ///< device matching patterns count

	/** callback called when device is attached to bus
	 */
	indigo_result (*attach)(indigo_device *device);
	/** callback called when client broadcast enumerate properties request on bus, device and name elements of property can be set NULL to address all
	 */
	indigo_result (*enumerate_properties)(indigo_device *device, indigo_client *client, indigo_property *property);
	/** callback called when client broadcast property change request
	 */
	indigo_result (*change_property)(indigo_device *device, indigo_client *client, indigo_property *property);
	/** callback called when client broadcast enableBLOB mode change request
	 */
	indigo_result (*enable_blob)(indigo_device *device, indigo_client *client, indigo_property *property, indigo_enable_blob_mode mode);
	/** callback called when device is detached from the bus
	 */
	indigo_result (*detach)(indigo_device *device);
} indigo_device;

#define INDIGO_DEVICE_INITIALIZER(name_str, attach_cb, enumerate_properties_cb, change_property_cb, enable_blob_cb, detach_cb) { \
	name_str, \
	NULL, \
	false, \
	0, \
	NULL, \
	NULL, \
	NULL, \
	INDIGO_OK, \
	INDIGO_VERSION_LEGACY, \
	0L, \
	NULL, \
	0, \
	attach_cb, \
	enumerate_properties_cb, \
	change_property_cb, \
	enable_blob_cb, \
	detach_cb \
}

/** Client structure definition
 */
typedef struct indigo_client {
	char name[INDIGO_NAME_SIZE];															///< client name
	bool is_remote;																						///< is remote client
	void *client_context;																			///< any client specific data
	indigo_result last_result;																///< result of last bus operation
	indigo_version version;																		///< client version
	indigo_enable_blob_mode_record *enable_blob_mode_records;	///< enable blob mode

	/** callback called when client is attached to the bus
	 */
	indigo_result (*attach)(indigo_client *client);
	/** callback called when device broadcast property definition
	 */
	indigo_result (*define_property)(indigo_client *client, indigo_device *device, indigo_property *property, const char *message);
	/** callback called when device broadcast property value change
	 */
	indigo_result (*update_property)(indigo_client *client, indigo_device *device, indigo_property *property, const char *message);
	/** callback called when device broadcast property removal
	 */
	indigo_result (*delete_property)(indigo_client *client, indigo_device *device, indigo_property *property, const char *message);
	/** callback called when device broadcast a message
	 */
	indigo_result (*send_message)(indigo_client *client, indigo_device *device, const char *message);
	/** callback called when client is detached from the bus
	 */
	indigo_result (*detach)(indigo_client *client);
} indigo_client;

/** Wire protocol adapter private data structure.
 */
typedef struct {
	indigo_uni_handle **input;					///< input handle
	indigo_uni_handle **output;					///< output handle
	bool web_socket;										///< connection over WebSocket (RFC6455)
	char url_prefix[INDIGO_NAME_SIZE];	///< server url prefix (for BLOB download)
} indigo_adapter_context;

/** BLOB entry type.
 */
typedef struct {
	indigo_property *property;					///<BLOB property
	indigo_item *item;     							///< BLOB item
	void *content;            					///< BLOB content
	long size;              						///< BLOB size
	char format[INDIGO_NAME_SIZE];  		///< BLOB format, known file type suffix like ".fits" or ".jpeg"
	pthread_mutex_t mutext;							///< BLOB mutex
} indigo_blob_entry;

/** Last diagnostic messages.
 */
INDIGO_EXTERN char *indigo_last_message;

/** Name to be used in log (if not changed ot will be filled with executable name).
 */
INDIGO_EXTERN char indigo_log_name[];

/** If set, handler is used to print message instead of stderr/syslog output.
 */
INDIGO_EXTERN void (*indigo_log_message_handler)(indigo_log_levels level, const char *message);

/** Get INDIGO version major.minor-build.
 */
INDIGO_EXTERN void indigo_get_version(int *major, int *minor, int *build);

/** Print diagnostic messages - low level.
 */
INDIGO_EXTERN void indigo_log_base(indigo_log_levels level, const char *format, va_list args);

/** Print diagnostic messages - plain.
 */
INDIGO_EXTERN void indigo_log_message(const char *format, va_list args);

/** Print diagnostic messages on trace level, wrap calls to INDIGO_TRACE() macro.
 */
INDIGO_EXTERN void indigo_trace(const char *format, ...);

/** Print diagnostic messages on debug level, wrap calls to INDIGO_DEBUG() macro.
 */

INDIGO_EXTERN void indigo_debug(const char *format, ...);
/** Print diagnostic messages on debug_bus level, wrap calls to INDIGO_DEBUG() macro.
 */

INDIGO_EXTERN void indigo_trace_bus(const char *format, ...);
/** Print diagnostic messages on error level, wrap calls to INDIGO_ERROR() macro.
 */

INDIGO_EXTERN void indigo_error(const char *format, ...);
/** Print diagnostic messages on info level, wrap calls to INDIGO_LOG() macro.
 */
INDIGO_EXTERN void indigo_log(const char *format, ...);

/** Print diagnostic messages on given log level.
 */
INDIGO_EXTERN void indigo_log_on_level(indigo_log_levels log_level, const char *format, ...);

/** Internal use only
 */
INDIGO_EXTERN void indigo_driver_log(indigo_log_levels log_level, const char *driver, const char *function, int line, const char *format, ...);

/** Print diagnostic message on trace level with property value, full property definition and items dump can be requested.
 */
INDIGO_EXTERN void indigo_trace_property(const char *message, indigo_client *client, indigo_property *property, bool defs, bool items);

/** Start bus operation.
 Call has no effect, if bus is already started.
 */
INDIGO_EXTERN indigo_result indigo_start(void);

/** Set log level; see enum indigo_log_levels
*/
INDIGO_EXTERN void indigo_set_log_level(indigo_log_levels level);

/** Get log level; see enum indigo_log_levels
 */
INDIGO_EXTERN indigo_log_levels indigo_get_log_level(void);

/** Attach device to bus.
 Return value of attach() callback function is assigned to last_result in device structure.
 */
INDIGO_EXTERN indigo_result indigo_attach_device(indigo_device *device);

/** Detach device from bus.
 Return value of detach() callback function is assigned to last_result in device structure.
 */
INDIGO_EXTERN indigo_result indigo_detach_device(indigo_device *device);

/** Attach client to bus.
 Return value of attach() callback function is assigned to last_result in client structure.
 */
INDIGO_EXTERN indigo_result indigo_attach_client(indigo_client *client);

/** Detach client from bus.
 Return value of detach() callback function is assigned to last_result in client structure.
 */
INDIGO_EXTERN indigo_result indigo_detach_client(indigo_client *client);

/** Broadcast property definition.
 */
INDIGO_EXTERN indigo_result indigo_define_property(indigo_device *device, indigo_property *property, const char *format, ...);

/** Broadcast property value change.
 */
INDIGO_EXTERN indigo_result indigo_update_property(indigo_device *device, indigo_property *property, const char *format, ...);

/** Broadcast property removal.
 */
INDIGO_EXTERN indigo_result indigo_delete_property(indigo_device *device, indigo_property *property, const char *format, ...);

/** Broadcast message.
 */
INDIGO_EXTERN indigo_result indigo_send_message(indigo_device *device, const char *format, ...);

/** Broadcast property enumeration request.
 */
INDIGO_EXTERN indigo_result indigo_enumerate_properties(indigo_client *client, indigo_property *property);

/** Broadcast property change request.
 */
INDIGO_EXTERN indigo_result indigo_change_property(indigo_client *client, indigo_property *property);

/** Broadcast enableBLOB request.
 */
INDIGO_EXTERN indigo_result indigo_enable_blob(indigo_client *client, indigo_property *property, indigo_enable_blob_mode mode);

/** Stop bus operation.
 Call has no effect if bus is already stopped.
 */
INDIGO_EXTERN indigo_result indigo_stop(void);

/** Initialize text property.
 */
INDIGO_EXTERN indigo_property *indigo_init_text_property(indigo_property *property, const char *device, const char *name, const char *group, const char *label, indigo_property_state state, indigo_property_perm perm, int count);
/** Initialize number property.
 */
INDIGO_EXTERN indigo_property *indigo_init_number_property(indigo_property *property, const char *device, const char *name, const char *group, const char *label, indigo_property_state state, indigo_property_perm perm, int count);
/** Initialize switch property.
 */
INDIGO_EXTERN indigo_property *indigo_init_switch_property(indigo_property *property, const char *device, const char *name, const char *group, const char *label, indigo_property_state state, indigo_property_perm perm, indigo_rule rule, int count);
/** Initialize light property.
 */
INDIGO_EXTERN indigo_property *indigo_init_light_property(indigo_property *property, const char *device, const char *name, const char *group, const char *label, indigo_property_state state, int count);
/** Initialize BLOB property.
 */
INDIGO_EXTERN indigo_property *indigo_init_blob_property_p(indigo_property *property, const char *device, const char *name, const char *group, const char *label, indigo_property_state state, indigo_property_perm perm, int count);
/** Initialize read only BLOB property.
 */
INDIGO_EXTERN indigo_property *indigo_init_blob_property(indigo_property *property, const char *device, const char *name, const char *group, const char *label, indigo_property_state state, int count);
/** Resize property.
 */
INDIGO_EXTERN indigo_property *indigo_resize_property(indigo_property *property, int count);
/** Copy "property" to "copy". Allocate, if copy is NULL.
 */
INDIGO_EXTERN indigo_property *indigo_copy_property(indigo_property *copy, indigo_property *property);
/** Clear property.
 */
INDIGO_EXTERN indigo_property *indigo_clear_property(indigo_property *property);
/** Allocate blob buffer (rounded up to 2880 bytes).
 */
INDIGO_EXTERN void *indigo_alloc_blob_buffer(long size);
/** Resize property.
 */
INDIGO_EXTERN void indigo_release_property(indigo_property *property);

/** Validate address of item of registered BLOB property.
 */
INDIGO_EXTERN indigo_blob_entry *indigo_validate_blob(indigo_item *item);

/** Find BLOB entry.
 */
INDIGO_EXTERN indigo_blob_entry *indigo_find_blob(indigo_property *other_property, indigo_item *other_item);

/** Initialize text item.
 */
INDIGO_EXTERN void indigo_init_text_item(indigo_item *item, const char *name, const char *label, const char *format, ...);
/** Initialize raw text item.
 */
INDIGO_EXTERN void indigo_init_text_item_raw(indigo_item *item, const char *name, const char *label, const char *value);
/** Initialize number item.
 */
INDIGO_EXTERN void indigo_init_number_item(indigo_item *item, const char *name, const char *label, double min, double max, double step, double value);

#define indigo_init_sexagesimal_number_item(item, name, label, min, max, step, value) { indigo_init_number_item(item, name, label, min, max, step, value); strcpy(item->number.format, "%12.9m"); }

/** Initialize switch item.
 */
INDIGO_EXTERN void indigo_init_switch_item(indigo_item *item, const char *name, const char *label, bool value);
/** Initialize light item.
 */
INDIGO_EXTERN void indigo_init_light_item(indigo_item *item, const char *name, const char *label, indigo_property_state value);
/** Initialize BLOB item.
 */
INDIGO_EXTERN void indigo_init_blob_item(indigo_item *item, const char *name, const char *label);

/** download BLOB for given url.
 */
INDIGO_EXTERN bool indigo_download_blob(char *url, void **value, long *size, char *format);

/** populate BLOB item if url is given.
 */
INDIGO_EXTERN bool indigo_populate_http_blob_item(indigo_item *blob_item);

/** upload BLOB item if url is given.
 */
INDIGO_EXTERN bool indigo_upload_http_blob_item(indigo_item *blob_item);

/** get property hint value by key, returns false if key is not found, if the key has no value empty string is returned
 */
INDIGO_EXTERN bool indigo_get_property_hint(indigo_property *property, const char *key, char *value);

/** get item hint value by key, returns false if key is not found, if the key has no value empty string is returned
 */
INDIGO_EXTERN bool indigo_get_item_hint(indigo_item *item, const char *key, char *value);

/** Test, if property matches other property.
 */
INDIGO_EXTERN bool indigo_property_match(indigo_property *property, indigo_property *other);

/** Test, if property matches other property and is defined.
 */
INDIGO_EXTERN bool indigo_property_match_defined(indigo_property *property, indigo_property *other);

/** Test, if property matches other property and is defined and RW.
 */
INDIGO_EXTERN bool indigo_property_match_changeable(indigo_property *property, indigo_property *other);

/** Test, if switch item matches other switch item.
 */
INDIGO_EXTERN bool indigo_switch_match(indigo_item *item, indigo_property *other);

/** Set switch item on (and reset other if needed).
 */
INDIGO_EXTERN void indigo_set_switch(indigo_property *property, indigo_item *item, bool value);

/** Get item.
 */
INDIGO_EXTERN indigo_item *indigo_get_item(indigo_property *property, const char *item_name);

/** Get switch item value.
 */
INDIGO_EXTERN bool indigo_get_switch(indigo_property *property, const char *item_name);

/** Copy item values from other property into property (optionally including property state).
 */
INDIGO_EXTERN void indigo_property_copy_values(indigo_property *property, indigo_property *other, bool with_state);

/** Copy item values into target from other number property into property (optionally including property state).
 */
INDIGO_EXTERN void indigo_property_copy_targets(indigo_property *property, indigo_property *other, bool with_state);

/** Sort item values on description
 */

INDIGO_EXTERN void indigo_property_sort_items(indigo_property *property, int first);

/** Request text property change.
 */
INDIGO_EXTERN indigo_result indigo_change_text_property(indigo_client *client, const char *device, const char *name, int count, const char **items, const char **values);

/** Request text property change with access token.
 */
INDIGO_EXTERN indigo_result indigo_change_text_property_with_token(indigo_client *client, const char *device, indigo_token token, const char *name, int count, const char **items, const char **values);

/** Request text property change.
 */
INDIGO_EXTERN indigo_result indigo_change_text_property_1(indigo_client *client, const char *device, const char *name, const char *item, const char *format, ...);

/** Request raw text property change.
 */
INDIGO_EXTERN indigo_result indigo_change_text_property_1_raw(indigo_client *client, const char *device, const char *name, const char *item, const char *value);

/** Request text property change with access token.
 */
INDIGO_EXTERN indigo_result indigo_change_text_property_1_with_token(indigo_client *client, const char *device, indigo_token token, const char *name, const char *item, const char *format, ...);

/** Request raw text property change with access token.
 */
INDIGO_EXTERN indigo_result indigo_change_text_property_1_with_token_raw(indigo_client *client, const char *device, indigo_token token, const char *name, const char *item, const char *value);

/** Request number property change.
 */
INDIGO_EXTERN indigo_result indigo_change_number_property(indigo_client *client, const char *device, const char *name, int count, const char **items, const double *values);

/** Request number property change with access token.
 */
INDIGO_EXTERN indigo_result indigo_change_number_property_with_token(indigo_client *client, const char *device, indigo_token token, const char *name, int count, const char **items, const double *values);

/** Request number property change.
 */
INDIGO_EXTERN indigo_result indigo_change_number_property_1(indigo_client *client, const char *device, const char *name, const char *item, const double value);

/** Request number property change with access token.
 */
INDIGO_EXTERN indigo_result indigo_change_number_property_1_with_token(indigo_client *client, const char *device, indigo_token token, const char *name, const char *item, const double value);

/** Request switch property change.
 */
INDIGO_EXTERN indigo_result indigo_change_switch_property(indigo_client *client, const char *device, const char *name, int count, const char **items, const bool *values);

/** Request switch property change with access token.
 */
INDIGO_EXTERN indigo_result indigo_change_switch_property_with_token(indigo_client *client, const char *device, indigo_token token, const char *name, int count, const char **items, const bool *values);

/** Request switch property change.
 */
INDIGO_EXTERN indigo_result indigo_change_switch_property_1(indigo_client *client, const char *device, const char *name, const char *item, const bool value);

/** Request switch property change with access_token.
 */
INDIGO_EXTERN indigo_result indigo_change_switch_property_1_with_token(indigo_client *client, const char *device, indigo_token token, const char *name, const char *item, const bool value);

/** Request BLOB property change.
 */
INDIGO_EXTERN indigo_result indigo_change_blob_property(indigo_client *client, const char *device, const char *name, int count, const char **items, void **values, const long *sizes, const char **formats, const char **url);

/** Request BLOB property change with access_token.
 */
INDIGO_EXTERN indigo_result indigo_change_blob_property_with_token(indigo_client *client, const char *device, indigo_token token, const char *name, int count, const char **items, void **values, const long *sizes, const char **formats, const char **urls);

/** Request BLOB property change.
 */
INDIGO_EXTERN indigo_result indigo_change_blob_property_1(indigo_client *client, const char *device, const char *name, const char *item, void *value, const long size, const char *format, const char *url);

/** Request BLOB property change with access_token.
 */
INDIGO_EXTERN indigo_result indigo_change_blob_property_1_with_token(indigo_client *client, const char *device, indigo_token token, const char *name, const char *item, void *value, const long size, const char *format, const char *url);

/** Send connect message.
 */
INDIGO_EXTERN indigo_result indigo_device_connect(indigo_client *client, char *device);

/** Send disconnect message.
 */
INDIGO_EXTERN indigo_result indigo_device_disconnect(indigo_client *client, char *device);

/** Query slave devices of given master device.
 */
INDIGO_EXTERN int indigo_query_slave_devices(indigo_device *master, indigo_device **slaves, int max);

/** Trim " @ local_service_name" from the string.
 */
INDIGO_EXTERN void indigo_trim_local_service(char *device_name);

/** Asynchronous handle property change in sepatate thread
*/
INDIGO_EXTERN bool indigo_handle_property_async(
	void (*handler)(indigo_device *device, indigo_client *client, indigo_property *property),
	indigo_device *device,
	indigo_client *client,
	indigo_property *property
);

/** Asynchronous execution in thread.
 */
INDIGO_EXTERN bool indigo_async(void *fun(void *data), void *data);

#define INDIGO_ASYNC(call, data) (indigo_async((void*(*)(void *))call, (void*)data))

/** Convert sexagesimal string to double.
 */
INDIGO_EXTERN double indigo_stod(char *string);

/** Convert double to sexagesimal string.
 */
INDIGO_EXTERN char* indigo_dtos(double value, const char *format);

/** Sleeps for specified number of microseconds.
 */
INDIGO_EXTERN void indigo_usleep(long delay);

#define ONE_SECOND_DELAY 1000000L

#define INDIGO_DELAY(fraction) ((long)(ONE_SECOND_DELAY * (fraction)))
#define indigo_sleep(fraction) indigo_usleep(INDIGO_DELAY(fraction))

/** Locale independent atod()
 */
INDIGO_EXTERN double indigo_atod(const char *str);

/** Locale independent dtoa()
 */
INDIGO_EXTERN char *indigo_dtoa(double value, char *str);

/** Get size independent text item value
 */
INDIGO_EXTERN char *indigo_get_text_item_value(indigo_item *item);

/** Set size independent text item value
 */
INDIGO_EXTERN void indigo_set_text_item_value(indigo_item *item, const char *value);

#define indigo_fix_locale(s) { char *fc = strchr(s, ','); if (fc) *fc = '.'; }

#define indigo_copy_name(target, source) { memset(target, 0, INDIGO_NAME_SIZE); strncpy(target, source, INDIGO_NAME_SIZE - 1); }
#define indigo_copy_value(target, source) { memset(target, 0, INDIGO_VALUE_SIZE); strncpy(target, source, INDIGO_VALUE_SIZE - 1); }

#define indigo_define_matching_property(template) if (indigo_property_match(template, property)) { indigo_define_property(device, template, NULL); }
#define indigo_copy_values_process_change(property, handler, timer) if (PRIVATE_DATA->timer == NULL) { indigo_property_copy_values(property, property, false); property->state = INDIGO_BUSY_STATE; indigo_update_property(device, property, NULL); indigo_set_timer(device, 0, handler, &PRIVATE_DATA->timer); }
#define indigo_copy_targets_process_change(property, handler, timer) if (PRIVATE_DATA->timer == NULL) { indigo_property_copy_targets(property, property, false); property->state = INDIGO_BUSY_STATE; indigo_update_property(device, property, NULL); indigo_set_timer(device, 0, handler, &PRIVATE_DATA->timer); }


/** Property representing all properties of all devices (used for enumeration broadcast).
 */
INDIGO_EXTERN indigo_property INDIGO_ALL_PROPERTIES;

/** Variable used to store main() argc argument for loging purposes.
 */
INDIGO_EXTERN int indigo_main_argc;

/** Variable used to store main() argv argument for loging purposes.
 */
INDIGO_EXTERN const char **indigo_main_argv;

#if defined(INDIGO_LINUX) || defined(INDIGO_MACOS)
/** Send logging messages to syslog instead of stderr.
 */
INDIGO_EXTERN bool indigo_use_syslog;
#endif

/** Ignore messages from remote devices containing local service name to avoid loops.
 */
INDIGO_EXTERN char indigo_local_service_name[INDIGO_NAME_SIZE];

/** Reshare remote devices. Set to true with care, can create loops.
 */
INDIGO_EXTERN bool indigo_reshare_remote_devices;

/** Do not add @ host:port suffix to remote devices - for case with single remote server and no local devices only.
 */
INDIGO_EXTERN bool indigo_use_host_suffix;

/** Is sandboxed environment (macOS only).
 */
INDIGO_EXTERN bool indigo_is_sandboxed;

/** Cache BLOB content
 */
INDIGO_EXTERN bool indigo_use_blob_caching;

/** Proxy BLOB content
 */
INDIGO_EXTERN bool indigo_proxy_blob;

/** Use recursive locks for dispaching all bus messages
 */
INDIGO_EXTERN bool indigo_use_strict_locking;

/** Allocate, assert and zero
 */

static inline void *indigo_safe_malloc(size_t size) {
	void *pointer = malloc(size);
	assert(pointer != NULL);
	if (pointer != NULL) {
		memset(pointer, 0, size);
		return pointer;
	}
	fprintf(stderr, "Failed to allocate memory");
	exit(0);
}

static inline void* indigo_safe_malloc_copy(size_t size, void* from) {
	void* pointer = malloc(size);
	assert(pointer != NULL);
	if (pointer != NULL) {
		memcpy(pointer, from, size);
		return pointer;
	}
	fprintf(stderr, "Failed to allocate memory");
	exit(0);
}

#ifdef _MSC_VER
#pragma warning(disable: 6308)
#endif

static inline void *indigo_safe_realloc(void *pointer, size_t size) {
	pointer = realloc(pointer, size);
	assert(pointer != NULL);
	if (pointer != NULL) {
		return pointer;
	}
	fprintf(stderr, "Failed to allocate memory");
	exit(0);
}

static inline void *indigo_safe_realloc_copy(void *pointer, size_t size, void *from) {
	pointer = realloc(pointer, size);
	assert(pointer != NULL);
	if (pointer != NULL) {
		memcpy(pointer, from, size);
		return pointer;
	}
	fprintf(stderr, "Failed to allocate memory");
	exit(0);
}

static inline void indigo_safe_free(void *pointer) {
	if (pointer) {
		free(pointer);
	}
}

static inline char *indigo_safe_strncpy(char *dst, const char *src, size_t size) {
    if (size > 0) {
		dst[size - 1] = '\0';
        strncpy(dst, src, size - 1);
    }
	return dst;
}

#define INDIGO_BUFFER_SIZE (128 * 1024)

INDIGO_EXTERN void *indigo_alloc_large_buffer(void);
INDIGO_EXTERN void indigo_free_large_buffer(void *large_buffer);

/** Calculate pixel scale in arcsec/pixel
 */
INDIGO_EXTERN double indigo_pixel_scale(double focal_length_cm, double pixel_size_um);

/** Check for duplicate device name
 */
INDIGO_EXTERN bool indigo_device_name_exists(const char *name);

/** Fix device name to be unique with #number suffix
 */
INDIGO_EXTERN bool indigo_make_name_unique(char *name, const char *format, ...);

#if defined(INDIGO_WINDOWS)

INDIGO_EXTERN int gettimeofday(struct timeval * tp, struct timezone * tzp);

#endif

#ifdef __cplusplus
}
#endif

#endif /* indigo_bus_h */
