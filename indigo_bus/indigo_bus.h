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

#define INDIGO_TRACE(c) c
#define INDIGO_DEBUG(c) c
#define INDIGO_ERROR(c) c
#define INDIGO_LOG(c) c

#define NAME_SIZE 32
#define VALUE_SIZE 256
#define MAX_ITEMS 32

typedef enum {
  INDIGO_OK = 0,
  INDIGO_PARTIALLY_FAILED,
  INDIGO_TOO_MANY_DRIVERS
} indigo_result;

typedef enum {
  INDIGO_TEXT_VECTOR,
  INDIGO_NUMBER_VECTOR,
  INDIGO_SWITCH_VECTOR,
  INDIGO_LIGHT_VECTOR,
  INDIGO_BLOB_VECTOR
} indigo_property_type;

extern char *indigo_property_type_text[];

typedef enum {
  INDIGO_IDLE_STATE,
  INDIGO_OK_STATE,
  INDIGO_BUSY_STATE,
  INDIGO_ALERT_STATE
} indigo_property_state;

extern char *indigo_property_state_text[];

typedef enum {
  INDIGO_RO_PERM,
  INDIGO_RW_PERM,
  INDIGO_WO_PERM
} indigo_property_perm;

extern char *indigo_property_perm_text[];

typedef enum {
  INDIGO_ONE_OF_MANY_RULE,
  INDIGO_AT_MOST_ONE_RULE,
  INDIGO_ANY_OF_MANY_RULE
} indigo_rule;

extern char *indigo_switch_type_text[];

typedef struct {
  char name[NAME_SIZE];
  char label[VALUE_SIZE];
  char text_value[VALUE_SIZE];
  double number_min, number_max, number_step, number_value;
  bool switch_value;
  indigo_property_state light_value;
  char blob_format[NAME_SIZE];
  long blob_size;
  void *blob_value;
} indigo_item;

typedef struct {
  char device[NAME_SIZE];
  char name[NAME_SIZE];
  char label[VALUE_SIZE];
  indigo_property_state state;
  indigo_property_type type;
  indigo_property_perm perm;
  indigo_rule rule;
  float version;
  int count;
  indigo_item items[];
} indigo_property;

struct indigo_client;

typedef struct indigo_driver {
  indigo_result (*init)(struct indigo_driver *driver);
  indigo_result (*start)(struct indigo_driver *driver);
  indigo_result (*enumerate_properties)(struct indigo_driver *driver, struct indigo_client *client);
  indigo_result (*change_property)(struct indigo_driver *driver, struct indigo_client *client, indigo_property *property);
  indigo_result (*stop)(struct indigo_driver *driver);
} indigo_driver;

typedef struct indigo_client {
  indigo_result (*init)(struct indigo_client *client);
  indigo_result (*start)(struct indigo_client *client);
  indigo_result (*define_property)(struct indigo_client *client, struct indigo_driver *driver, indigo_property *property);
  indigo_result (*update_property)(struct indigo_client *client, struct indigo_driver *driver, indigo_property *property);
  indigo_result (*delete_property)(struct indigo_client *client, struct indigo_driver *driver, indigo_property *property);
  indigo_result (*stop)(struct indigo_client *client);
} indigo_client;

typedef indigo_driver *(*indigo_driver_entry_point)();
typedef indigo_client *(*indigo_client_entry_point)();

extern void indigo_trace(const char *format, ...);
extern void indigo_debug(const char *format, ...);
extern void indigo_error(const char *format, ...);
extern void indigo_log(const char *format, ...);
extern void indigo_debug_property(const char *message, indigo_property *property, bool defs, bool items);

extern indigo_result indigo_init();
extern indigo_result indigo_register_driver(indigo_driver_entry_point entry_point);
extern indigo_result indigo_register_client(indigo_client_entry_point entry_point);
extern indigo_result indigo_start();

extern indigo_result indigo_define_property(indigo_driver *driver, indigo_property *property);
extern indigo_result indigo_update_property(indigo_driver *driver, indigo_property *property);
extern indigo_result indigo_delete_property(indigo_driver *driver, indigo_property *property);

extern indigo_result indigo_enumerate_properties(indigo_client *client);
extern indigo_result indigo_change_property(indigo_client *client, indigo_property *property);

extern indigo_result indigo_stop();

extern indigo_property *indigo_allocate_text_property(const char *device, const char *name, const char *label, indigo_property_state state, indigo_property_perm perm, int count);
extern indigo_property *indigo_allocate_number_property(const char *device, const char *name, const char *label, indigo_property_state state, indigo_property_perm perm, int count);
extern indigo_property *indigo_allocate_switch_property(const char *device, const char *name, const char *label, indigo_property_state state, indigo_property_perm perm, indigo_rule rule, int count);
extern indigo_property *indigo_allocate_light_property(const char *device, const char *name, const char *label, indigo_property_state state, int count);
extern indigo_property *indigo_allocate_blob_property(const char *device, const char *name, const char *label, indigo_property_state state, int count);

extern void indigo_init_text_item(indigo_item *item, const char *name, const char *label, const char *value);
extern void indigo_init_number_item(indigo_item *item, const char *name, const char *label, double min, double max, double step, double value);
extern void indigo_init_switch_item(indigo_item *item, const char *name, const char *label, bool value);
extern void indigo_init_light_item(indigo_item *item, const char *name, const char *label, indigo_property_state value);
extern void indigo_init_blob_item(indigo_item *item, const char *name, const char *label);

extern bool indigo_property_match(indigo_property *property, indigo_property *other);
extern void indigo_property_copy_values(indigo_property *property, indigo_property *other);

#endif /* indigo_bus_h */
