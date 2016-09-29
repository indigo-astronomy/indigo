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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <time.h>
#include <assert.h>
#include <pthread.h>
#include <sys/time.h>

#ifdef INDIGO_USE_SYSLOG
#include <syslog.h>
#endif

#include "indigo_bus.h"

#define MAX_DRIVERS 128
#define MAX_CLIENTS 8

static indigo_driver *drivers[MAX_DRIVERS];
static indigo_client *clients[MAX_CLIENTS];
static pthread_mutex_t bus_mutex = PTHREAD_RECURSIVE_MUTEX_INITIALIZER;
static bool is_started = false;

char *indigo_property_type_text[] = {
  "UNDEFINED",
  "TEXT",
  "NUMBER",
  "SWITCH",
  "LIGHT",
  "BLOB"
};

char *indigo_property_state_text[] = {
  "Idle",
  "Ok",
  "Busy",
  "Alert"
};

char *indigo_property_perm_text[] = {
  "UNDEFINED",
  "ro",
  "rw",
  "wo"
};

char *indigo_switch_rule_text[] = {
  "UNDEFINED",
  "OneOfMany",
  "AtMostOne",
  "AnyOfMany"
};

indigo_property INDIGO_ALL_PROPERTIES;
const char **indigo_main_argv;
int indigo_main_argc;

static void log_message(const char *format, va_list args) {
  static char buffer[256];
  static pthread_mutex_t log_mutex = PTHREAD_MUTEX_INITIALIZER;  
  if (!INDIGO_LOCK(&log_mutex)) {
    vsnprintf(buffer, sizeof(buffer), format, args);
    char *line = buffer;
#ifdef INDIGO_USE_SYSLOG
    while (line) {
      char *eol = strchr(line, '\n');
      if (eol)
        *eol = 0;
      if (eol > line)
        syslog (LOG_NOTICE, "%s", buffer);
      fprintf(stderr, "%s: %s %s\n", log_executable_name, timestamp, line);
      if (eol)
        line = eol + 1;
      else
        line = NULL;
    }
#else
    char timestamp[16];
    struct timeval tmnow;
    gettimeofday(&tmnow, NULL);
    strftime (timestamp, 9, "%H:%M:%S", localtime(&tmnow.tv_sec));
    snprintf(timestamp + 8, sizeof(timestamp) - 8, ".%06d ", tmnow.tv_usec);
    static const char *log_executable_name = NULL;
    if (log_executable_name == NULL) {
      log_executable_name = strrchr(indigo_main_argv[0], '/');
      if (log_executable_name != NULL)
        log_executable_name++;
      else
        log_executable_name = indigo_main_argv[0];
    }
    while (line) {
      char *eol = strchr(line, '\n');
      if (eol)
        *eol = 0;
      if (*line)
        fprintf(stderr, "%s: %s %s\n", log_executable_name, timestamp, line);
      if (eol)
        line = eol + 1;
      else
        line = NULL;
    }
#endif
  }
  INDIGO_UNLOCK(&log_mutex);
}

void indigo_trace(const char *format, ...) {
  va_list argList;
  va_start(argList, format);
  log_message(format, argList);
  va_end(argList);
}

void indigo_debug(const char *format, ...) {
  va_list argList;
  va_start(argList, format);
  log_message(format, argList);
  va_end(argList);
}

void indigo_debug_property(const char *message, indigo_property *property, bool defs, bool items) {
  if (message != NULL)
    indigo_debug(message);
  if (defs)
    indigo_debug("'%s'.'%s' %s %s %s %d.%d %s { // %s", property->device, property->name, indigo_property_type_text[property->type], indigo_property_perm_text[property->perm], indigo_property_state_text[property->state], (property->version >> 8) & 0xFF, property->version & 0xFF, (property->type == INDIGO_SWITCH_VECTOR ? indigo_switch_rule_text[property->rule]: ""), property->label);
  else
    indigo_debug("'%s'.'%s' %s %s %s %d.%d %s {", property->device, property->name, indigo_property_type_text[property->type], indigo_property_perm_text[property->perm], indigo_property_state_text[property->state], (property->version >> 8) & 0xFF, property->version & 0xFF, (property->type == INDIGO_SWITCH_VECTOR ? indigo_switch_rule_text[property->rule]: ""));
  if (items) {
    for (int i = 0; i < property->count; i++) {
      indigo_item *item = &property->items[i];
      switch (property->type) {
        case INDIGO_TEXT_VECTOR:
          if (defs)
            indigo_debug("  '%s' = '%s' // %s", item->name, item->text_value, item->label);
          else
            indigo_debug("  '%s' = '%s' ",item->name, item->text_value);
          break;
        case INDIGO_NUMBER_VECTOR:
          if (defs)
            indigo_debug("  '%s' = %g (%g, %g, %g) // %s", item->name, item->number_value, item->number_min, item->number_max, item->number_step, item->label);
          else
            indigo_debug("  '%s' = %g ",item->name, item->number_value);
          break;
        case INDIGO_SWITCH_VECTOR:
          if (defs)
            indigo_debug("  '%s' = %s // %s", item->name, (item->switch_value ? "On" : "Off"), item->label);
          else
            indigo_debug("  '%s' = %s ",item->name, (item->switch_value ? "On" : "Off"));
          break;
        case INDIGO_LIGHT_VECTOR:
          if (defs)
            indigo_debug("  '%s' = %s // %s", item->name, indigo_property_state_text[item->light_value], item->label);
          else
            indigo_debug("  '%s' = %s ",item->name, indigo_property_state_text[item->light_value]);
          break;
        case INDIGO_BLOB_VECTOR:
          if (defs)
            indigo_debug("  '%s' // %s", item->name, item->label);
          else
            indigo_debug("  '%s' (%ld bytes, '%s')",item->name, item->blob_size, item->blob_format);
          break;
      }
    }
  }
  indigo_debug("}");
}

void indigo_error(const char *format, ...) {
  va_list argList;
  va_start(argList, format);
  log_message(format, argList);
  va_end(argList);
}

void indigo_log(const char *format, ...) {
  va_list argList;
  va_start(argList, format);
  log_message(format, argList);
  va_end(argList);
}

indigo_result indigo_start() {
  if (!INDIGO_LOCK(&bus_mutex)) {
    if (!is_started) {
      memset(drivers, 0, MAX_DRIVERS * sizeof(indigo_driver *));
      memset(clients, 0, MAX_CLIENTS * sizeof(indigo_client *));
      memset(&INDIGO_ALL_PROPERTIES, 0, sizeof(INDIGO_ALL_PROPERTIES));
      INDIGO_ALL_PROPERTIES.version = INDIGO_VERSION_CURRENT;
      is_started = true;
      INDIGO_UNLOCK(&bus_mutex);
    }
    return INDIGO_OK;
  }
  return INDIGO_LOCK_ERROR;
}

indigo_result indigo_connect_driver(indigo_driver *driver) {
  assert(driver != NULL);
  if (!INDIGO_LOCK(&bus_mutex)) {
    indigo_result result = INDIGO_TOO_MANY_DRIVERS;
    for (int i = 0; i < MAX_DRIVERS; i++) {
      if (drivers[i] == NULL) {
        drivers[i] = driver;
        if (driver->attach == NULL)
          result = INDIGO_OK;
        else
          result = driver->last_result = driver->attach(driver);
        break;
      }
    }
    INDIGO_UNLOCK(&bus_mutex);
    return result;
  }
  return INDIGO_LOCK_ERROR;
}

indigo_result indigo_disconnect_driver(indigo_driver *driver) {
  assert(driver != NULL);
  if (!INDIGO_LOCK(&bus_mutex)) {
    indigo_result result = INDIGO_NOT_FOUND;
    for (int i = 0; i < MAX_DRIVERS; i++) {
      if (drivers[i] == NULL)
        break;
      if (drivers[i] == driver) {
        if (driver->detach == NULL)
          result = INDIGO_OK;
        else
          result = driver->last_result = driver->detach(driver);
        for (int j = i + 1; j < MAX_DRIVERS; j++) {
          drivers[j - 1] = drivers[j];
          if (drivers[j] == NULL)
            break;
        }
      }
    }
    INDIGO_UNLOCK(&bus_mutex);
    return result;
  }
  return INDIGO_LOCK_ERROR;
}

indigo_result indigo_connect_client(indigo_client *client) {
  assert(client != NULL);
  if (!INDIGO_LOCK(&bus_mutex)) {
    indigo_result result = INDIGO_TOO_MANY_DRIVERS;
    for (int i = 0; i < MAX_CLIENTS; i++) {
      if (clients[i] == NULL) {
        clients[i] = client;
        if (client->attach == NULL)
          result = INDIGO_OK;
        else
          result = client->last_result = client->attach(client);
        break;
      }
    }
    INDIGO_UNLOCK(&bus_mutex);
    return result;
  }
  return INDIGO_LOCK_ERROR;
}

indigo_result indigo_disconnect_client(indigo_client *client) {
  assert(client != NULL);
  if (!INDIGO_LOCK(&bus_mutex)) {
    indigo_result result = INDIGO_NOT_FOUND;
    for (int i = 0; i < MAX_CLIENTS; i++) {
      if (clients[i] == NULL)
        break;
      if (clients[i] == client) {
        if (client->detach == NULL)
          result = INDIGO_OK;
        else
          result = client->last_result = client->detach(client);
        for (int j = i + 1; j < MAX_CLIENTS; j++) {
          clients[j - 1] = clients[j];
          if (clients[j] == NULL)
            break;
        }
      }
    }
    INDIGO_UNLOCK(&bus_mutex);
    return result;
  }
  return INDIGO_LOCK_ERROR;
}

indigo_result indigo_enumerate_properties(indigo_client *client, indigo_property *property) {
  assert(client != NULL);
  if (!INDIGO_LOCK(&bus_mutex)) {
    property->version = client->version;
    INDIGO_DEBUG(indigo_debug_property("INDIGO Bus: property enumeration request", property, false, true));
    for (int i = 0; i < MAX_DRIVERS; i++) {
      indigo_driver *driver = drivers[i];
      if (driver == NULL)
        break;
      if (driver->enumerate_properties != NULL)
        driver->last_result = driver->enumerate_properties(driver, client, property);
    }
    INDIGO_UNLOCK(&bus_mutex);
    return INDIGO_OK;
  }
  return INDIGO_LOCK_ERROR;
}

indigo_result indigo_change_property(indigo_client *client, indigo_property *property) {
  assert(client != NULL);
  assert(property != NULL);
  if (!INDIGO_LOCK(&bus_mutex)) {
    property->version = client->version;
    INDIGO_DEBUG(indigo_debug_property("INDIGO Bus: property change request", property, false, true));
    for (int i = 0; i < MAX_DRIVERS; i++) {
      indigo_driver *driver = drivers[i];
      if (driver == NULL)
        break;
      if (driver->change_property != NULL)
        driver->last_result = driver->change_property(driver, client, property);
    }
    INDIGO_UNLOCK(&bus_mutex);
    return INDIGO_OK;
  }
  return INDIGO_LOCK_ERROR;
}

indigo_result indigo_define_property(indigo_driver *driver, indigo_property *property) {
  assert(driver != NULL);
  assert(property != NULL);
  if (!INDIGO_LOCK(&bus_mutex)) {
    property->version = driver->version;
    INDIGO_DEBUG(indigo_debug_property("INDIGO Bus: property definition", property, true, true));
    for (int i = 0; i < MAX_CLIENTS; i++) {
      indigo_client *client = clients[i];
      if (client == NULL)
        break;
      if (client->define_property != NULL)
        client->last_result = client->define_property(client, driver, property);
    }
    INDIGO_UNLOCK(&bus_mutex);
    return INDIGO_OK;
  }
  return INDIGO_LOCK_ERROR;
}

indigo_result indigo_update_property(indigo_driver *driver, indigo_property *property) {
  assert(driver != NULL);
  assert(property != NULL);
  if (!INDIGO_LOCK(&bus_mutex)) {
    property->version = driver->version;
    INDIGO_DEBUG(indigo_debug_property("INDIGO Bus: property update", property, false, true));
    for (int i = 0; i < MAX_CLIENTS; i++) {
      indigo_client *client = clients[i];
      if (client == NULL)
        break;
      if (client->update_property != NULL)
        client->last_result = client->update_property(client, driver, property);
    }
    INDIGO_UNLOCK(&bus_mutex);
    return INDIGO_OK;
  }
  return INDIGO_LOCK_ERROR;
}

indigo_result indigo_delete_property(indigo_driver *driver, indigo_property *property) {
  assert(driver != NULL);
  assert(property != NULL);
  if (!INDIGO_LOCK(&bus_mutex)) {
    property->version = driver->version;
    INDIGO_DEBUG(indigo_debug_property("INDIGO Bus: property removal", property, false, false));
    for (int i = 0; i < MAX_CLIENTS; i++) {
      indigo_client *client = clients[i];
      if (client == NULL)
        break;
      if (client->delete_property != NULL)
        client->last_result = client->delete_property(client, driver, property);
    }
    INDIGO_UNLOCK(&bus_mutex);
    return INDIGO_OK;
  }
  return INDIGO_LOCK_ERROR;
}

indigo_result indigo_stop() {
  if (!INDIGO_LOCK(&bus_mutex)) {
    if (is_started) {
      for (int i = 0; i < MAX_DRIVERS; i++) {
        indigo_driver *driver = drivers[i];
        if (driver == NULL)
          break;
        if (driver->detach != NULL)
          driver->last_result = driver->detach(driver);
      }
      for (int i = 0; i < MAX_CLIENTS; i++) {
        indigo_client *client = clients[i];
        if (client == NULL)
          break;
        if (client->detach != NULL)
          client->last_result = client->detach(client);
      }
      INDIGO_UNLOCK(&bus_mutex);
      is_started = false;
    }
    return INDIGO_OK;
  }
  return INDIGO_LOCK_ERROR;
}

indigo_property *indigo_init_text_property(indigo_property *property, const char *device, const char *name, const char *group, const char *label, indigo_property_state state, indigo_property_perm perm, int count) {
  int size = sizeof(indigo_property)+count*(sizeof(indigo_item));
  if (property == NULL) {
    property = malloc(size);
  }
  memset(property, 0, size);
  strncpy(property->device, device,INDIGO_NAME_SIZE);
  strncpy(property->name, name,INDIGO_NAME_SIZE);
  strncpy(property->group, group,INDIGO_NAME_SIZE);
  strncpy(property->label, label, INDIGO_VALUE_SIZE);
  property->type = INDIGO_TEXT_VECTOR;
  property->state = state;
  property->perm = perm;
  property->version = INDIGO_VERSION_CURRENT;
  property->count = count;
  return property;
}

indigo_property *indigo_init_number_property(indigo_property *property, const char *device, const char *name, const char *group, const char *label, indigo_property_state state, indigo_property_perm perm, int count) {
  int size = sizeof(indigo_property)+count*(sizeof(indigo_item));
  if (property == NULL) {
    property = malloc(size);
  }
  memset(property, 0, size);
  strncpy(property->device, device,INDIGO_NAME_SIZE);
  strncpy(property->name, name,INDIGO_NAME_SIZE);
  strncpy(property->group, group,INDIGO_NAME_SIZE);
  strncpy(property->label, label, INDIGO_VALUE_SIZE);
  property->type = INDIGO_NUMBER_VECTOR;
  property->state = state;
  property->perm = perm;
  property->version = INDIGO_VERSION_CURRENT;
  property->count = count;
  return property;
}

indigo_property *indigo_init_switch_property(indigo_property *property, const char *device, const char *name, const char *group, const char *label, indigo_property_state state, indigo_property_perm perm, indigo_rule rule, int count) {
  int size = sizeof(indigo_property)+count*(sizeof(indigo_item));
  if (property == NULL) {
    property = malloc(size);
  }
  memset(property, 0, size);
  strncpy(property->device, device,INDIGO_NAME_SIZE);
  strncpy(property->name, name,INDIGO_NAME_SIZE);
  strncpy(property->group, group,INDIGO_NAME_SIZE);
  strncpy(property->label, label, INDIGO_VALUE_SIZE);
  property->type = INDIGO_SWITCH_VECTOR;
  property->state = state;
  property->perm = perm;
  property->rule = rule;
  property->version = INDIGO_VERSION_CURRENT;
  property->count = count;
  return property;
}

indigo_property *indigo_init_light_property(indigo_property *property, const char *device, const char *name, const char *group, const char *label, indigo_property_state state, int count) {
  int size = sizeof(indigo_property)+count*(sizeof(indigo_item));
  if (property == NULL) {
    property = malloc(size);
  }
  memset(property, 0, size);
  strncpy(property->device, device,INDIGO_NAME_SIZE);
  strncpy(property->name, name,INDIGO_NAME_SIZE);
  strncpy(property->group, group,INDIGO_NAME_SIZE);
  strncpy(property->label, label, INDIGO_VALUE_SIZE);
  property->type = INDIGO_LIGHT_VECTOR;
  property->perm = INDIGO_RO_PERM;
  property->state = state;
  property->version = INDIGO_VERSION_CURRENT;
  property->count = count;
  return property;
}

indigo_property *indigo_init_blob_property(indigo_property *property, const char *device, const char *name, const char *group, const char *label, indigo_property_state state, int count) {
  int size = sizeof(indigo_property)+count*(sizeof(indigo_item));
  if (property == NULL) {
    property = malloc(size);
  }
  memset(property, 0, size);
  strncpy(property->device, device,INDIGO_NAME_SIZE);
  strncpy(property->name, name,INDIGO_NAME_SIZE);
  strncpy(property->group, group,INDIGO_NAME_SIZE);
  strncpy(property->label, label, INDIGO_VALUE_SIZE);
  property->type = INDIGO_BLOB_VECTOR;
  property->perm = INDIGO_RO_PERM;
  property->state = state;
  property->version = INDIGO_VERSION_CURRENT;
  property->count = count;
  return property;
}

void indigo_init_text_item(indigo_item *item, const char *name, const char *label, const char *format, ...) {
  memset(item, 0, sizeof(indigo_item));
  strncpy(item->name, name,INDIGO_NAME_SIZE);
  strncpy(item->label, label,INDIGO_NAME_SIZE);
  va_list args;
  va_start(args, format);
  vsnprintf(item->text_value, INDIGO_VALUE_SIZE, format, args);
  va_end(args);
}

void indigo_init_number_item(indigo_item *item, const char *name, const char *label, double min, double max, double step, double value) {
  memset(item, sizeof(indigo_item), 0);
  strncpy(item->name, name,INDIGO_NAME_SIZE);
  strncpy(item->label, label, INDIGO_VALUE_SIZE);
  item->number_min = min;
  item->number_max = max;
  item->number_step = step;
  item->number_value = value;
}

void indigo_init_switch_item(indigo_item *item, const char *name, const char *label, bool value) {
  memset(item, sizeof(indigo_item), 0);
  strncpy(item->name, name,INDIGO_NAME_SIZE);
  strncpy(item->label, label, INDIGO_VALUE_SIZE);
  item->switch_value = value;
}

void indigo_init_light_item(indigo_item *item, const char *name, const char *label, indigo_property_state value) {
  memset(item, sizeof(indigo_item), 0);
  strncpy(item->name, name,INDIGO_NAME_SIZE);
  strncpy(item->label, label, INDIGO_VALUE_SIZE);
  item->light_value = value;
}

void indigo_init_blob_item(indigo_item *item, const char *name, const char *label) {
  memset(item, sizeof(indigo_item), 0);
  strncpy(item->name, name,INDIGO_NAME_SIZE);
  strncpy(item->label, label, INDIGO_VALUE_SIZE);
}

bool indigo_property_match(indigo_property *property, indigo_property *other) {
  assert(property != NULL);
  return other == NULL || ((other->type == 0 || property->type == other->type) && (*other->device == 0 || !strcmp(property->device, other->device)) && (*other->name == 0 || !strcmp(property->name, other->name)));
}

void indigo_property_copy_values(indigo_property *property, indigo_property *other, bool with_state) {
  if (property->type == other->type) {
    if (with_state)
      property->state = other->state;
    if (property->type == INDIGO_SWITCH_VECTOR && property->rule != INDIGO_ANY_OF_MANY_RULE) {
      for (int j = 0; j < property->count; j++) {
        property->items[j].switch_value = false;
      }
    }
    for (int i = 0; i < other->count; i++) {
      indigo_item *other_item = &other->items[i];
      for (int j = 0; j < property->count; j++) {
        indigo_item *property_item = &property->items[j];
        if (!strcmp(property_item->name, other_item->name)) {
          switch (property->type) {
            case INDIGO_TEXT_VECTOR:
              strncpy(property_item->text_value, other_item->text_value, INDIGO_VALUE_SIZE);
              break;
            case INDIGO_NUMBER_VECTOR:
              property_item->number_value = other_item->number_value;
              break;
            case INDIGO_SWITCH_VECTOR:
              property_item->switch_value = other_item->switch_value;
              break;
            case INDIGO_LIGHT_VECTOR:
              property_item->light_value = other_item->light_value;
              break;
            case INDIGO_BLOB_VECTOR:
              strncpy(property_item->blob_format, other_item->blob_format,INDIGO_NAME_SIZE);
              property_item->blob_size = other_item->blob_size;
              property_item->blob_value = other_item->blob_value;
              break;
          }
          break;
        }
      }
    }
  }
}
