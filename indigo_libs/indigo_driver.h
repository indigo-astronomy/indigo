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

/** INDIGO Driver base
 \file indigo_driver.h
 */

#ifndef indigo_device_h
#define indigo_device_h

//#define INDIGO_LINUX
//#undef INDIGO_MACOS

#if defined(INDIGO_LINUX) || defined(INDIGO_FREEBSD)
#include <pthread.h>
#elif defined(INDIGO_MACOS)
#include <dispatch/dispatch.h>
#endif

#include <stdint.h>

#include "indigo_bus.h"
#include "indigo_names.h"
#include "indigo_timer.h"

#ifdef __cplusplus
extern "C" {
#endif

/** Main group name string.
 */
#define MAIN_GROUP                    "Main"

/** Device context pointer.
 */
#define DEVICE_CONTEXT                ((indigo_device_context *)device->device_context)

/** CONNECTION property pointer, property is mandatory, property change request handler should set property items and state and call indigo_device_change_property() on exit.
 */
#define CONNECTION_PROPERTY           (DEVICE_CONTEXT->connection_property)

/** CONNECTION.CONNECTED property item pointer.
 */
#define CONNECTION_CONNECTED_ITEM     (CONNECTION_PROPERTY->items+0)

/** CONNECTION.DISCONNECTED property item pointer.
 */
#define CONNECTION_DISCONNECTED_ITEM  (CONNECTION_PROPERTY->items+1)

/** INFO property pointer, property is mandatory.
 */
#define INFO_PROPERTY                 (DEVICE_CONTEXT->info_property)

/** INFO.DEVICE_NAME property item pointer.
 */
#define INFO_DEVICE_NAME_ITEM         (INFO_PROPERTY->items+0)

/** INFO.DEVICE_VERSION property item pointer.
 */
#define INFO_DEVICE_VERSION_ITEM      (INFO_PROPERTY->items+1)

/** INFO.DEVICE_INTERFACE property item pointer.
 */
#define INFO_DEVICE_INTERFACE_ITEM    (INFO_PROPERTY->items+2)

/** INFO.DEVICE_MODEL property item pointer.
 */
#define INFO_DEVICE_MODEL_ITEM         (INFO_PROPERTY->items+3)

/** INFO.DEVICE_FIRMWARE_REVISION property item pointer.
 */
#define INFO_DEVICE_FW_REVISION_ITEM   (INFO_PROPERTY->items+4)

/** INFO.DEVICE_HARDWARE_REVISION property item pointer.
 */
#define INFO_DEVICE_HW_REVISION_ITEM   (INFO_PROPERTY->items+5)

/** INFO.DEVICE_SERIAL_NUMBER property item pointer.
 */
#define INFO_DEVICE_SERIAL_NUM_ITEM    (INFO_PROPERTY->items+6)

/** INFO.FRAMEWORK_NAME property item pointer.
 */
#define INFO_FRAMEWORK_NAME_ITEM      (INFO_PROPERTY->items+3)

/** INFO.FRAMEWORK_VERSION property item pointer.
 */
#define INFO_FRAMEWORK_VERSION_ITEM   (INFO_PROPERTY->items+4)

/** SIMULATION property pointer, property is optional.
 */
#define SIMULATION_PROPERTY           (DEVICE_CONTEXT->simulation_property)

/** SIMULATION.DISABLED property item pointer.
 */
#define SIMULATION_ENABLED_ITEM       (SIMULATION_PROPERTY->items+0)

/** SIMULATION.DISABLED property item pointer.
 */
#define SIMULATION_DISABLED_ITEM      (SIMULATION_PROPERTY->items+1)

/** CONFIG property pointer, property is mandatory.
 */
#define CONFIG_PROPERTY               (DEVICE_CONTEXT->configuration_property)

/** CONFIG.LOAD property item pointer.
 */
#define CONFIG_LOAD_ITEM              (CONFIG_PROPERTY->items+0)

/** CONFIG.SAVE property item pointer.
 */
#define CONFIG_SAVE_ITEM              (CONFIG_PROPERTY->items+1)

/** CONFIG.DEFAULT property item pointer.
 */

#define CONFIG_REMOVE_ITEM           (CONFIG_PROPERTY->items+2)

/** Number of profiles
 */

#define PROFILE_COUNT									5

/** PROFILE property pointer, property is mandatory.
 */
#define PROFILE_PROPERTY               (DEVICE_CONTEXT->profile_property)

/** PROFILE.PROFILE_0 property item pointer.
 */
#define PROFILE_ITEM              		(PROFILE_PROPERTY->items+0)

/** DEVICE_PORT property pointer, property is optional, property change request is handled by indigo_device_change_property.
 */
#define DEVICE_PORT_PROPERTY					(DEVICE_CONTEXT->device_port_property)

/** DEVICE_PORT.DEVICE_PORT property item pointer.
 */
#define DEVICE_PORT_ITEM							(DEVICE_PORT_PROPERTY->items+0)

/** DEVICE_PORTS property pointer, property is optional, property change request is handled by indigo_device_change_property.
 */
#define DEVICE_PORTS_PROPERTY					(DEVICE_CONTEXT->device_ports_property)

/** AUTHENTICATION property pointer, property is optional, property change request is handled by indigo_device_change_property.
 */
#define AUTHENTICATION_PROPERTY					(DEVICE_CONTEXT->device_auth_property)

/** AUTHENTICATION.PASSWORD property item pointer.
 */
#define AUTHENTICATION_PASSWORD_ITEM			(AUTHENTICATION_PROPERTY->items+0)

/** AUTHENTICATION.USER property item pointer.
 */
#define AUTHENTICATION_USER_ITEM					(AUTHENTICATION_PROPERTY->items+1)


/** Device interface (value shout be used for INFO_DEVICE_INTERFACE_ITEM->number.value
 */
typedef enum {
	INDIGO_INTERFACE_MOUNT     = (1 << 0),    ///< Mount interface
	INDIGO_INTERFACE_CCD       = (1 << 1),    ///< CCD interface
	INDIGO_INTERFACE_GUIDER    = (1 << 2),    ///< Guider interface
	INDIGO_INTERFACE_FOCUSER   = (1 << 3),    ///< Focuser interface
	INDIGO_INTERFACE_WHEEL     = (1 << 4),    ///< Filter wheel interface
	INDIGO_INTERFACE_DOME      = (1 << 5),    ///< Dome interface
	INDIGO_INTERFACE_GPS       = (1 << 6),    ///< GPS interface
	INDIGO_INTERFACE_WEATHER   = (1 << 7),    ///< Weather interface
	INDIGO_INTERFACE_AO        = (1 << 8),    ///< Adaptive Optics Interface
	INDIGO_INTERFACE_DUSTCAP   = (1 << 9),    ///< Dust Cap Interface
	INDIGO_INTERFACE_LIGHTBOX  = (1 << 10),   ///< Light Box Interface
	INDIGO_INTERFACE_AUX       = (1 << 15)    ///< Auxiliary interface
} indigo_device_interface;

/** Device driver entrypoint actions
 */
typedef enum {
	INDIGO_DRIVER_INIT,
	INDIGO_DRIVER_INFO,
	INDIGO_DRIVER_SHUTDOWN
} indigo_driver_action;

/** Version major and minor
 */
#define INDIGO_VERSION_MAJOR(ver) ((ver >> 8) & 0xff)
#define INDIGO_VERSION_MINOR(ver) (ver & 0xff)

/** Device driver info structure
 */
typedef struct {
	char description[INDIGO_NAME_SIZE];
	char name[INDIGO_NAME_SIZE];
	uint16_t version;  /* version - MSB, revision - LSB */
	bool multi_device_support;
	indigo_driver_action status;
} indigo_driver_info;

/** Device driver entry point prototype
 */
typedef indigo_result (*driver_entry_point)(indigo_driver_action, indigo_driver_info*);

/** Device context structure.
 */
typedef struct {
	int property_save_file_handle;            ///< handle for property save
	indigo_timer *timers;											///< active timer list
	indigo_property *connection_property;     ///< CONNECTION property pointer
	indigo_property *info_property;           ///< INFO property pointer
	indigo_property *simulation_property;     ///< SIMULATION property pointer
	indigo_property *configuration_property;  ///< CONFIGURATION property pointer
	indigo_property *profile_property; 				///< PROFILE property pointer
	indigo_property *device_port_property;		///< DEVICE_PORT property pointer
	indigo_property *device_ports_property;		///< DEVICE_PORTS property pointer
	indigo_property *device_auth_property;		///< SECURITY property pointer
} indigo_device_context;

/** log macros
*/

#define INDIGO_DRIVER_LOG(driver_name, fmt, ...) INDIGO_LOG(indigo_log("%s: " fmt, driver_name, ##__VA_ARGS__))
#define INDIGO_DRIVER_ERROR(driver_name, fmt, ...) INDIGO_ERROR(indigo_error("%s[%d]: " fmt, driver_name, __LINE__, ##__VA_ARGS__))
#define INDIGO_DRIVER_DEBUG(driver_name, fmt, ...) INDIGO_DEBUG_DRIVER(indigo_debug("%s[%d, %s]: " fmt, driver_name, __LINE__, __FUNCTION__, ##__VA_ARGS__))
#define INDIGO_DRIVER_TRACE(driver_name, fmt, ...) INDIGO_TRACE_DRIVER(indigo_trace("%s[%d, %s]: " fmt, driver_name, __LINE__, __FUNCTION__, ##__VA_ARGS__))

#define INDIGO_DEVICE_ATTACH_LOG(driver_name, device_name) INDIGO_DRIVER_LOG(driver_name, "'%s' attached", device_name)
#define INDIGO_DEVICE_DETACH_LOG(driver_name, device_name) INDIGO_DRIVER_LOG(driver_name, "'%s' detached", device_name)

 /** set driver info.
  */

#define SET_DRIVER_INFO(dinfo, ddescr, dname, dversion, dmulti, dstatus)\
{\
	if(dinfo) {\
		strncpy(dinfo->description, ddescr, INDIGO_NAME_SIZE);\
		strncpy(dinfo->name, dname, INDIGO_NAME_SIZE);\
		dinfo->version = dversion;\
		dinfo->multi_device_support = dmulti;\
		dinfo->status = dstatus;\
	}\
}


/** Try to aquire global lock
*/
extern indigo_result indigo_try_global_lock(indigo_device *device);

/** Globally unlock
*/
extern indigo_result indigo_global_unlock(indigo_device *device);

/** Device is connected.
 */

#define IS_CONNECTED	(CONNECTION_CONNECTED_ITEM->sw.value && CONNECTION_PROPERTY->state == INDIGO_OK_STATE)

/** Attach callback function.
 */
extern indigo_result indigo_device_attach(indigo_device *device, indigo_version version, int interface);

/** Enumerate properties callback function.
 */
extern indigo_result indigo_device_enumerate_properties(indigo_device *device, indigo_client *client, indigo_property *property);

/** Change property callback function.
 */
extern indigo_result indigo_device_change_property(indigo_device *device, indigo_client *client, indigo_property *property);

/** Detach callback function.
 */
extern indigo_result indigo_device_detach(indigo_device *device);

/** Open config file.
 */

extern int indigo_open_config_file(char *device_name, int profile, int mode, const char *suffix);

/** Load properties.
 */
extern indigo_result indigo_load_properties(indigo_device *device, bool default_properties);

/** Save single property.
 */
extern indigo_result indigo_save_property(indigo_device*device, int *file_handle, indigo_property *property);

/** Remove properties.
 */
extern indigo_result indigo_remove_properties(indigo_device *device);

/** Start USB event handler thread.
 */
extern void indigo_start_usb_event_handler(void);

/** Asynchronous execution in thread.
 */
extern void indigo_async(void *fun(void *data), void *data);

/** Convert sexagesimal string to double.
 */
extern double indigo_stod(char *string);

/** Convert double to sexagesimal string.
 */
extern char* indigo_dtos(double value, char *format);

/** get current utc.
 */
time_t indigo_utc(time_t *ltime);

/** Convert time_t to ISO 8601 string.
 */
void indigo_timetoiso(time_t tstamp, char *isotime, int isotime_len);

/** Convert ISO 8601 string to time_t.
 */
time_t indigo_isototime(char *isotime);

/** Enumerate serial ports.
 */
	void indigo_enumerate_serial_ports(indigo_device *device, indigo_property *property);

#ifdef __cplusplus
}
#endif

#endif /* indigo_device_h */
