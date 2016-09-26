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

#ifndef indigo_bus_h
#define indigo_bus_h

#include <stdbool.h>

#include "indigo_config.h"

#ifdef INDIGO_USE_LOCKS
#define INDIGO_LOCK(mutex) pthread_mutex_lock(mutex)
#define INDIGO_UNLOCK(mutex) assert(pthread_mutex_unlock(mutex) == 0)
#else
#define INDIGO_LOCK(mutex) 0
#define INDIGO_UNLOCK(mutex)
#endif

#define INDIGO_VERSION_NONE    0x0000
#define INDIGO_VERSION_LEGACY  0x0107
#define INDIGO_VERSION_2_0     0x0200

typedef enum {
  INDIGO_OK = 0,
  INDIGO_INIT_FAILED,
  INDIGO_START_FAILED,
  INDIGO_REQUEST_FAILED,
  INDIGO_TOO_MANY_DRIVERS,
  INDIGO_LOCK_ERROR,
  INDIGO_NOT_FOUND
} indigo_result;

typedef enum {
  INDIGO_TEXT_VECTOR = 1,
  INDIGO_NUMBER_VECTOR,
  INDIGO_SWITCH_VECTOR,
  INDIGO_LIGHT_VECTOR,
  INDIGO_BLOB_VECTOR
} indigo_property_type;

extern char *indigo_property_type_text[];

typedef enum {
  INDIGO_IDLE_STATE = 0,
  INDIGO_OK_STATE,
  INDIGO_BUSY_STATE,
  INDIGO_ALERT_STATE
} indigo_property_state;

extern char *indigo_property_state_text[];

typedef enum {
  INDIGO_RO_PERM = 1,
  INDIGO_RW_PERM,
  INDIGO_WO_PERM
} indigo_property_perm;

extern char *indigo_property_perm_text[];

typedef enum {
  INDIGO_ONE_OF_MANY_RULE = 1,
  INDIGO_AT_MOST_ONE_RULE,
  INDIGO_ANY_OF_MANY_RULE
} indigo_rule;

extern char *indigo_switch_rule_text[];

typedef struct {
  char name[INDIGO_NAME_SIZE];
  char label[INDIGO_VALUE_SIZE];
  char text_value[INDIGO_VALUE_SIZE];
  double number_min, number_max, number_step, number_value;
  bool switch_value;
  indigo_property_state light_value;
  char blob_format[INDIGO_NAME_SIZE];
  long blob_size;
  void *blob_value;
} indigo_item;

typedef struct {
  char device[INDIGO_NAME_SIZE];
  char name[INDIGO_NAME_SIZE];
  char group[INDIGO_NAME_SIZE];
  char label[INDIGO_VALUE_SIZE];
  indigo_property_state state;
  indigo_property_type type;
  indigo_property_perm perm;
  indigo_rule rule;
  short version;
  int count;
  indigo_item items[];
} indigo_property;

struct indigo_client;

typedef struct indigo_driver {
  void *driver_context;
  indigo_result last_result;
  indigo_result (*connect)(struct indigo_driver *driver);
  indigo_result (*enumerate_properties)(struct indigo_driver *driver, struct indigo_client *client, indigo_property *property);
  indigo_result (*change_property)(struct indigo_driver *driver, struct indigo_client *client, indigo_property *property);
  indigo_result (*disconnect)(struct indigo_driver *driver);
} indigo_driver;

typedef struct indigo_client {
  void *client_context;
  indigo_result last_result;
  indigo_result (*connect)(struct indigo_client *client);
  indigo_result (*define_property)(struct indigo_client *client, struct indigo_driver *driver, indigo_property *property);
  indigo_result (*update_property)(struct indigo_client *client, struct indigo_driver *driver, indigo_property *property);
  indigo_result (*delete_property)(struct indigo_client *client, struct indigo_driver *driver, indigo_property *property);
  indigo_result (*disconnect)(struct indigo_client *client);
} indigo_client;

typedef indigo_driver *(*indigo_driver_entry_point)();
typedef indigo_client *(*indigo_client_entry_point)();

extern void indigo_trace(const char *format, ...);
extern void indigo_debug(const char *format, ...);
extern void indigo_error(const char *format, ...);
extern void indigo_log(const char *format, ...);
extern void indigo_debug_property(const char *message, indigo_property *property, bool defs, bool items);

extern indigo_result indigo_start();
extern indigo_result indigo_connect_driver(indigo_driver *driver);
extern indigo_result indigo_disconnect_driver(indigo_driver *driver);
extern indigo_result indigo_connect_client(indigo_client *client);
extern indigo_result indigo_disconnect_client(indigo_client *client);
extern indigo_result indigo_stop();

extern indigo_result indigo_define_property(indigo_driver *driver, indigo_property *property);
extern indigo_result indigo_update_property(indigo_driver *driver, indigo_property *property);
extern indigo_result indigo_delete_property(indigo_driver *driver, indigo_property *property);

extern indigo_result indigo_enumerate_properties(indigo_client *client, indigo_property *property);
extern indigo_result indigo_change_property(indigo_client *client, indigo_property *property);

extern indigo_result indigo_stop();

extern indigo_property *indigo_init_text_property(indigo_property *property, const char *device, const char *name, const char *group, const char *label, indigo_property_state state, indigo_property_perm perm, int count);
extern indigo_property *indigo_init_number_property(indigo_property *property, const char *device, const char *name, const char *group, const char *label, indigo_property_state state, indigo_property_perm perm, int count);
extern indigo_property *indigo_init_switch_property(indigo_property *property, const char *device, const char *name, const char *group, const char *label, indigo_property_state state, indigo_property_perm perm, indigo_rule rule, int count);
extern indigo_property *indigo_init_light_property(indigo_property *property, const char *device, const char *name, const char *group, const char *label, indigo_property_state state, int count);
extern indigo_property *indigo_init_blob_property(indigo_property *property, const char *device, const char *name, const char *group, const char *label, indigo_property_state state, int count);

extern void indigo_init_text_item(indigo_item *item, const char *name, const char *label, const char *value);
extern void indigo_init_number_item(indigo_item *item, const char *name, const char *label, double min, double max, double step, double value);
extern void indigo_init_switch_item(indigo_item *item, const char *name, const char *label, bool value);
extern void indigo_init_light_item(indigo_item *item, const char *name, const char *label, indigo_property_state value);
extern void indigo_init_blob_item(indigo_item *item, const char *name, const char *label);

extern bool indigo_property_match(indigo_property *property, indigo_property *other);
extern void indigo_property_copy_values(indigo_property *property, indigo_property *other);

extern indigo_property INDIGO_ALL_PROPERTIES;

#endif /* indigo_bus_h */
