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

#include <stdint.h>
#include <pthread.h>

#include <indigo/indigo_bus.h>
#include <indigo/indigo_names.h>
#include <indigo/indigo_timer.h>

#ifdef INDIGO_LINUX
#include <malloc.h>
#define MALLOCED_SIZE malloc_usable_size
#endif

#ifdef INDIGO_MACOS
#include <malloc/malloc.h>
#define MALLOCED_SIZE malloc_size
#endif

#ifdef INDIGO_WINDOWS
#include <malloc.h>
#define MALLOCED_SIZE _msize
#endif

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

/** INFO.DEVICE_DRIVER  property item pointer.
 */
#define INFO_DEVICE_DRIVER_ITEM      (INFO_PROPERTY->items+1)

/** INFO.DEVICE_VERSION property item pointer.
 */
#define INFO_DEVICE_VERSION_ITEM      (INFO_PROPERTY->items+2)

/** INFO.DEVICE_INTERFACE property item pointer.
 */
#define INFO_DEVICE_INTERFACE_ITEM    (INFO_PROPERTY->items+3)

/** INFO.DEVICE_MODEL property item pointer.
 */
#define INFO_DEVICE_MODEL_ITEM         (INFO_PROPERTY->items+4)

/** INFO.DEVICE_FIRMWARE_REVISION property item pointer.
 */
#define INFO_DEVICE_FW_REVISION_ITEM   (INFO_PROPERTY->items+5)

/** INFO.DEVICE_HARDWARE_REVISION property item pointer.
 */
#define INFO_DEVICE_HW_REVISION_ITEM   (INFO_PROPERTY->items+6)

/** INFO.DEVICE_SERIAL_NUMBER property item pointer.
 */
#define INFO_DEVICE_SERIAL_NUM_ITEM    (INFO_PROPERTY->items+7)

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

/** DEVICE_BAUDRATE property pointer, property is optional, property change request is handled by indigo_device_change_property.
 */
#define DEVICE_BAUDRATE_PROPERTY                                    (DEVICE_CONTEXT->device_baudrate_property)

/** DEVICE_BAUDRATE.BAUDRATE property item pointer.
 */
#define DEVICE_BAUDRATE_ITEM                                                        (DEVICE_BAUDRATE_PROPERTY->items+0)


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

/** Client name for saved configuration reader.
 */

/** ADDITIONAL_INSTANCES  property pointer, property is optional, property change request is handled by indigo_device_change_property.
 */
#define ADDITIONAL_INSTANCES_PROPERTY					(DEVICE_CONTEXT->device_inst_property)

/** ADDITIONAL_INSTANCES.COUNT property item pointer.
 */
#define ADDITIONAL_INSTANCES_COUNT_ITEM			(ADDITIONAL_INSTANCES_PROPERTY->items+0)


#define CONFIG_READER								"CONFIG_READER"
#define MAX_ADDITIONAL_INSTANCES		4

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
	pthread_mutex_t config_mutex;							///< mutex for configuration load/save synchronisation
	indigo_timer *timers;											///< active timer list
	indigo_property *connection_property;     ///< CONNECTION property pointer
	indigo_property *info_property;           ///< INFO property pointer
	indigo_property *simulation_property;     ///< SIMULATION property pointer
	indigo_property *configuration_property;  ///< CONFIGURATION property pointer
	indigo_property *profile_property; 				///< PROFILE property pointer
	indigo_property *device_port_property;		///< DEVICE_PORT property pointer
	indigo_property *device_baudrate_property;///< DEVICE_BAUDRATE property pointer
	indigo_property *device_ports_property;		///< DEVICE_PORTS property pointer
	indigo_property *device_auth_property;		///< SECURITY property pointer
	indigo_property *device_inst_property;		///< ADDITIONAL_INSTANCES  property pointer
	indigo_device *base_device;								///< base instance for additional devices
	indigo_device *additional_device_instances[MAX_ADDITIONAL_INSTANCES]; ///< additional device instances
} indigo_device_context;

/** log macros
*/

#define INDIGO_DRIVER_LOG(driver_name, fmt, ...) INDIGO_LOG(indigo_log("%s: " fmt, driver_name, ##__VA_ARGS__))
#define INDIGO_DRIVER_ERROR(driver_name, fmt, ...) INDIGO_ERROR(indigo_error("%s[%s:%d, %p]: " fmt, driver_name, __FUNCTION__, __LINE__, pthread_self(), ##__VA_ARGS__))
#define INDIGO_DRIVER_DEBUG(driver_name, fmt, ...) INDIGO_DEBUG_DRIVER(indigo_debug("%s[%s:%d]: " fmt, driver_name, __FUNCTION__, __LINE__, ##__VA_ARGS__))
#define INDIGO_DRIVER_TRACE(driver_name, fmt, ...) INDIGO_TRACE_DRIVER(indigo_trace("%s[%s:%d]: " fmt, driver_name,__FUNCTION__, __LINE__, ##__VA_ARGS__))

#define INDIGO_DEVICE_ATTACH_LOG(driver_name, device_name) INDIGO_DRIVER_LOG(driver_name, "'%s' attached", device_name)
#define INDIGO_DEVICE_DETACH_LOG(driver_name, device_name) INDIGO_DRIVER_LOG(driver_name, "'%s' detached", device_name)

 /** set driver info.
  */

#define SET_DRIVER_INFO(dinfo, ddescr, dname, dversion, dmulti, dstatus)\
{\
	if(dinfo) {\
		indigo_copy_name(dinfo->description, ddescr);\
		indigo_copy_name(dinfo->name, dname);\
		dinfo->version = dversion;\
		dinfo->multi_device_support = dmulti;\
		dinfo->status = dstatus;\
	}\
}

#define VERIFY_NOT_CONNECTED(dev)\
{\
	indigo_device *device = dev;\
	if (device) {\
		if (!IS_DISCONNECTED)\
			return INDIGO_BUSY;\
		for (int i = 0; i < MAX_ADDITIONAL_INSTANCES; i++) {\
			indigo_device *tmp = DEVICE_CONTEXT->additional_device_instances[i];\
			indigo_device *device = tmp;\
			if (device != NULL && !IS_DISCONNECTED)\
				return INDIGO_BUSY;\
		}\
		indigo_usleep(100000);\
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

#define IS_CONNECTED	(DEVICE_CONTEXT != NULL && CONNECTION_CONNECTED_ITEM->sw.value && CONNECTION_PROPERTY->state == INDIGO_OK_STATE)

/** Device is disconnected.
 */

#define IS_DISCONNECTED	((DEVICE_CONTEXT == NULL) || (DEVICE_CONTEXT != NULL && CONNECTION_DISCONNECTED_ITEM->sw.value && CONNECTION_PROPERTY->state != INDIGO_BUSY_STATE))

/** Attach callback function.
 */
extern indigo_result indigo_device_attach(indigo_device *device, const char* driver_name, indigo_version version, int interface);

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

/** Save items of a property.
 */
extern indigo_result indigo_save_property_items(indigo_device*device, int *file_handle, indigo_property *property, const int count, const char **items);

/** Remove properties.
 */
extern indigo_result indigo_remove_properties(indigo_device *device);

/** Start USB event handler thread.
 */
extern void indigo_start_usb_event_handler(void);

/** get current utc. TO BE REMOVED!
 */
/*
time_t indigo_utc(time_t *ltime);
*/

/** Convert time_t to UTC ISO 8601 string.
 */
void indigo_timetoisogm(time_t tstamp, char *isotime, int isotime_len);

/** Convert UTC ISO 8601 time string to time_t.
 */
time_t indigo_isogmtotime(char *isotime);

/** Convert time_t to local time ISO 8601 string.
 */
void indigo_timetoisolocal(time_t tstamp, char *isotime, int isotime_len);

/** Convert local time ISO 8601 string to time_t.
 */
time_t indigo_isolocaltotime(char *isotime);

/** Enumerate serial ports.
 */
void indigo_enumerate_serial_ports(indigo_device *device, indigo_property *property);

/** Check for double connect/disconnect request.
 */
extern bool indigo_ignore_connection_change(indigo_device *device, indigo_property *request);

/** Calculate position corrected with a backlash
*/
extern int indigo_compensate_backlash(int requested_position, int current_position, int backlash, bool *is_last_move_poitive);

#ifdef __cplusplus
}
#endif

#endif /* indigo_device_h */
