  //  Copyright (c) 2016 CloudMakers, s. r. o.
  //  All rights reserved.
  //
  //  Redistribution and use in source and binary forms, with or without
  //  modification, are permitted provided that the following conditions
  //  are met:
  //
  //  1. Redistributions of source code must retain the above copyright
  //  notice, this list of conditions and the following disclaimer.
  //
  //  2. Redistributions in binary form must reproduce the above
  //  copyright notice, this list of conditions and the following
  //  disclaimer in the documentation and/or other materials provided
  //  with the distribution.
  //
  //  3. The name of the author may not be used to endorse or promote
  //  products derived from this software without specific prior
  //  written permission.
  //
  //  THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS
  //  OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
  //  WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
  //  ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY
  //  DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
  //  DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
  //  GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
  //  INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
  //  WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
  //  NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
  //  SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

  //  version history
  //  0.0 PoC by Peter Polakovic <peter.polakovic@cloudmakers.eu>

/** INDIGO Bus
 \file indigo_bus.h
 */

#ifndef indigo_bus_h
#define indigo_bus_h

#include <stdbool.h>

#include "indigo_config.h"

/** Device, property or item name size.
 */
#define INDIGO_NAME_SIZE      128

/** Item value size.
 */
#define INDIGO_VALUE_SIZE     256

/** Max number of item per property.
 */
#define INDIGO_MAX_ITEMS      64

// forward definitions

typedef struct indigo_client indigo_client;
typedef struct indigo_device indigo_device;

/** Property, device or client version.
 Version value is used to map property or item names correctly or to trigger for version specific property handling. Do not use other than defined values!
 */
typedef enum {
  INDIGO_VERSION_NONE   = 0x0000, ///< undefined version
  INDIGO_VERSION_LEGACY = 0x0107, ///< INDI compatible version
  INDIGO_VERSION_2_0    = 0x0200  ///< INDIGO version
} indigo_version;

/** Bus operation return status.
 */
typedef enum {
  INDIGO_OK = 0,              ///< success
  INDIGO_FAILED,              ///< unspecified error
  INDIGO_TOO_MANY_ELEMENTS,   ///< too many clients/devices/properties/items etc.
  INDIGO_LOCK_ERROR,          ///< mutex lock error
  INDIGO_NOT_FOUND,           ///< unknown client/device/property/item etc.
  INDIGO_CANT_START_SERVER    ///< network server start failure
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
extern char *indigo_property_type_text[];

/** Property state.
 */
typedef enum {
  INDIGO_IDLE_STATE = 0,      ///< property is passive or unused
  INDIGO_OK_STATE,            ///< property is in correct state or if operation on property was successful
  INDIGO_BUSY_STATE,          ///< property is transient state or if operation on property is pending
  INDIGO_ALERT_STATE          ///< property is in incorrect state or if operation on property failed
} indigo_property_state;

/** Textual representations of indigo_property_state values.
 */
extern char *indigo_property_state_text[];

/** Property access permission.
 */
typedef enum {
  INDIGO_RO_PERM = 1,         ///< read-only
  INDIGO_RW_PERM,             ///< read-write
  INDIGO_WO_PERM              ///< write-only
} indigo_property_perm;

/** Textual representation of indigo_property_perm values.
 */
extern char *indigo_property_perm_text[];

/** Switch behaviour rule.
 */
typedef enum {
  INDIGO_ONE_OF_MANY_RULE = 1,///< radio button group like behaviour with one switch in "on" state
  INDIGO_AT_MOST_ONE_RULE,    ///< radio button group like behaviour with none or one switch in "on" state
  INDIGO_ANY_OF_MANY_RULE     ///< checkbox button group like behaviour
} indigo_rule;

/** Textual representation of indigo_rule values.
 */
extern char *indigo_switch_rule_text[];

/** Property item definition.
 */
typedef struct {
  char name[INDIGO_NAME_SIZE];        ///< property wide unique item name
  char label[INDIGO_VALUE_SIZE];      ///< item description in human readable form
  union {
    /** Text property item specific fields.
     */
    struct {
      char value[INDIGO_VALUE_SIZE];  ///< item value (for text properties)
    } text;
    /** Number property item specific fields.
     */
    struct {
      double min;                     ///< item min value (for number properties)
      double max;                     ///< item max value (for number properties)
      double step;                    ///< item increment value (for number properties)
      double value;                   ///< item values (for number properties)
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
  indigo_property_state state;        ///< property state
  indigo_property_type type;          ///< property type
  indigo_property_perm perm;          ///< property access permission
  indigo_rule rule;                   ///< switch behaviour rule (for switch properties)
  short version;                      ///< property version INDIGO_VERSION_NONE, INDIGO_VERSION_LEGACY or INDIGO_VERSION_2_0
  bool hidden;                        ///< property is hidden/unused by  driver (for optional properties)
  int count;                          ///< number of property items
  indigo_item items[];                ///< property items
} indigo_property;

/** Device structure definition
 */
typedef struct indigo_device {
  void *device_context;               ///< any device specific data
  indigo_result last_result;          ///< result of last bus operation
  indigo_version version;             ///< device version
  
  /** callback called when device is attached to bus
   */
  indigo_result (*attach)(indigo_device *device);
  /** callback called when client broadcast enumerate properties request on bus, device and name elements of property can be set NULL to address all
   */
  indigo_result (*enumerate_properties)(indigo_device *device, indigo_client *client, indigo_property *property);
  /** callback called when client broadcast property change request
   */
  indigo_result (*change_property)(indigo_device *device, indigo_client *client, indigo_property *property);
  /** callback called when device is detached from the bus
   */
  indigo_result (*detach)(indigo_device *device);
} indigo_device;

/** Client structure definition
 */
typedef struct indigo_client {
  void *client_context;               ///< any client specific data
  indigo_result last_result;          ///< result of last bus operation
  indigo_version version;             ///< client version
  
  /** callback called when client is attached to the bus
   */
  indigo_result (*attach)(indigo_client *client);
  /** callback called when device broadcast property definition
   */
  indigo_result (*define_property)(indigo_client *client, struct indigo_device *device, indigo_property *property, const char *message);
  /** callback called when device broadcast property value change
   */
  indigo_result (*update_property)(indigo_client *client, struct indigo_device *device, indigo_property *property, const char *message);
  /** callback called when device broadcast property definition
   */
  indigo_result (*delete_property)(indigo_client *client, struct indigo_device *device, indigo_property *property, const char *message);
  /** callback called when device broadcast property removal
   */
  indigo_result (*send_message)(indigo_client *client, struct indigo_device *device, const char *message);
  /** callback called when client is detached from the bus
   */
  indigo_result (*detach)(indigo_client *client);
} indigo_client;


/** Variable used to store main() argc argument for loging purposes.
 */
extern int indigo_main_argc;
/** Variable used to store main() argv argument for loging purposes.
 */
extern const char **indigo_main_argv;

/** Print diagnostic messages on trace level, wrap calls to INDIGO_TRACE() macro.
 */
extern void indigo_trace(const char *format, ...);
/** Print diagnostic messages on debug level, wrap calls to INDIGO_DEBUG() macro.
 */
extern void indigo_debug(const char *format, ...);
/** Print diagnostic messages on error level, wrap calls to INDIGO_ERROR() macro.
 */
extern void indigo_error(const char *format, ...);
/** Print diagnostic messages on log level, wrap calls to INDIGO_LOG() macro.
 */
extern void indigo_log(const char *format, ...);

/** Print diagnostic message on debug level with property value, full property definition and items dump can be requested.
 */
extern void indigo_debug_property(const char *message, indigo_property *property, bool defs, bool items);

/** Start bus operation.
 Call has no effect, if bus is already started.
 */
extern indigo_result indigo_start();

/** Attach device to bus.
 Return value of attach() callback function is assigned to last_result in device structure.
 */
extern indigo_result indigo_attach_device(indigo_device *device);

/** Detach device from bus.
 Return value of detach() callback function is assigned to last_result in device structure.
 */
extern indigo_result indigo_detach_device(indigo_device *device);

/** Attach client to bus.
 Return value of attach() callback function is assigned to last_result in client structure.
 */
extern indigo_result indigo_attach_client(indigo_client *client);

/** Detach client from bus.
 Return value of detach() callback function is assigned to last_result in client structure.
 */
extern indigo_result indigo_detach_client(indigo_client *client);

/** Broadcast property definition.
 */
extern indigo_result indigo_define_property(indigo_device *device, indigo_property *property, const char *format, ...);

/** Broadcast property value change.
 */
extern indigo_result indigo_update_property(indigo_device *device, indigo_property *property, const char *format, ...);

/** Broadcast property removal.
 */
extern indigo_result indigo_delete_property(indigo_device *device, indigo_property *property, const char *format, ...);

/** Broadcast message.
 */
extern indigo_result indigo_send_message(indigo_device *device, const char *format, ...);

/** Broadcast property enumeration request.
 */
extern indigo_result indigo_enumerate_properties(indigo_client *client, indigo_property *property);

/** Broadcast property change request.
 */
extern indigo_result indigo_change_property(indigo_client *client, indigo_property *property);

/** Stop bus operation.
 Call has no effect if bus is already stopped.
 */
extern indigo_result indigo_stop();

/** Initialize text property.
 */
extern indigo_property *indigo_init_text_property(indigo_property *property, const char *device, const char *name, const char *group, const char *label, indigo_property_state state, indigo_property_perm perm, int count);
/** Initialize number property.
 */
extern indigo_property *indigo_init_number_property(indigo_property *property, const char *device, const char *name, const char *group, const char *label, indigo_property_state state, indigo_property_perm perm, int count);
/** Initialize switch property.
 */
extern indigo_property *indigo_init_switch_property(indigo_property *property, const char *device, const char *name, const char *group, const char *label, indigo_property_state state, indigo_property_perm perm, indigo_rule rule, int count);
/** Initialize light property.
 */
extern indigo_property *indigo_init_light_property(indigo_property *property, const char *device, const char *name, const char *group, const char *label, indigo_property_state state, int count);
/** Initialize BLOB property.
 */
extern indigo_property *indigo_init_blob_property(indigo_property *property, const char *device, const char *name, const char *group, const char *label, indigo_property_state state, int count);

/** Initialize text item.
 */
extern void indigo_init_text_item(indigo_item *item, const char *name, const char *label, const char *format, ...);
/** Initialize number item.
 */
extern void indigo_init_number_item(indigo_item *item, const char *name, const char *label, double min, double max, double step, double value);
/** Initialize switch item.
 */
extern void indigo_init_switch_item(indigo_item *item, const char *name, const char *label, bool value);
/** Initialize light item.
 */
extern void indigo_init_light_item(indigo_item *item, const char *name, const char *label, indigo_property_state value);
/** Initialize BLOB item.
 */
extern void indigo_init_blob_item(indigo_item *item, const char *name, const char *label);

/** Test, if property matches other property.
 */
extern bool indigo_property_match(indigo_property *property, indigo_property *other);

/** Test, if switch item matches other switch item.
 */
extern bool indigo_switch_match(indigo_item *item, indigo_property *other);

/** Set switch item on (and reset other if needed).
 */
extern void indigo_set_switch(indigo_property *property, indigo_item *item, bool value);

/** Copy item values from other property into property (optionally including property state).
 */
extern void indigo_property_copy_values(indigo_property *property, indigo_property *other, bool with_state);

/** Property representing all properties of all devices (used for enumeration broadcast).
 */
extern indigo_property INDIGO_ALL_PROPERTIES;

#endif /* indigo_bus_h */
