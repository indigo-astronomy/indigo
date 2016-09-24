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
#include <sys/time.h>

#include "indigo_bus.h"

#define MAX_DRIVERS 128
#define MAX_CLIENTS 8

static indigo_driver *drivers[MAX_DRIVERS];
static indigo_client *clients[MAX_CLIENTS];

char *indigo_property_type_text[] = {
  "UNDEFINED_TYPE",
  "TEXT",
  "NUMBER",
  "SWITCH",
  "LIGHT",
  "BLOB"
};

char *indigo_property_state_text[] = {
  "IDLE",
  "OK",
  "BUSY",
  "ALERT"
};

char *indigo_property_perm_text[] = {
  "UNDEFINED_PERM",
  "RO",
  "RW",
  "WO"
};

char *indigo_switch_rule_text[] = {
  "UNDEFINED_RULE",
  "ONE_OF_MANY",
  "AT_MOST_ONE",
  "ANY_OF_MANY"
};

indigo_property INDIGO_ALL_PROPERTIES;

static void log_message(const char *format, va_list args) {
  char buffer[256];
  struct timeval tmnow;
  gettimeofday(&tmnow, NULL);
  strftime (buffer, 9, "%H:%M:%S", localtime(&tmnow.tv_sec));
  sprintf(buffer + 8, ".%06d ", tmnow.tv_usec);
  vsnprintf(buffer + 16, 240, format, args);
  fprintf(stderr, "%s\n", buffer);
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

indigo_result indigo_init() {
  memset(drivers, 0, MAX_DRIVERS * sizeof(indigo_driver *));
  memset(clients, 0, MAX_CLIENTS * sizeof(indigo_client *));
  memset(&INDIGO_ALL_PROPERTIES, 0, sizeof(INDIGO_ALL_PROPERTIES));
  INDIGO_ALL_PROPERTIES.version = INDIGO_VERSION_CURRENT;
  return INDIGO_OK;
}

indigo_result indigo_register_driver(indigo_driver_entry_point entry_point) {
  indigo_driver *driver = entry_point();
  for (int i = 0; i < MAX_DRIVERS; i++) {
    if (drivers[i] == NULL) {
      drivers[i] = driver;
      return driver->init(driver);
    }
  }
  return INDIGO_TOO_MANY_DRIVERS;
}

indigo_result indigo_register_client(indigo_client_entry_point entry_point) {
  indigo_client *client = entry_point();
  for (int i = 0; i < MAX_CLIENTS; i++) {
    if (clients[i] == NULL) {
      clients[i] = client;
      return client->init(client);
    }
  }
  return INDIGO_TOO_MANY_DRIVERS;
}

indigo_result indigo_start() {
  bool success = true;
  for (int i = 0; i < MAX_DRIVERS; i++) {
    indigo_driver *driver = drivers[i];
    if (driver == NULL)
      break;
    if (driver->start(driver))
      success = false;
  }
  for (int i = 0; i < MAX_CLIENTS; i++) {
    indigo_client *client = clients[i];
    if (client == NULL)
      break;
    if (client->start(client))
      success = false;
  }
  return success ? INDIGO_OK : INDIGO_PARTIALLY_FAILED;
}

indigo_result indigo_enumerate_properties(indigo_client *client, indigo_property *property) {
  INDIGO_DEBUG(indigo_debug_property("INDIGO Bus: property enumeration request", property, false, true));
  bool success = true;
  for (int i = 0; i < MAX_DRIVERS; i++) {
    indigo_driver *driver = drivers[i];
    if (driver == NULL)
      break;
    if (driver->enumerate_properties(driver, client, property))
      success = false;
  }
  return success ? INDIGO_OK : INDIGO_PARTIALLY_FAILED;
}

indigo_result indigo_change_property(indigo_client *client, indigo_property *property) {
  INDIGO_DEBUG(indigo_debug_property("INDIGO Bus: property change request", property, false, true));
  bool success = true;
  for (int i = 0; i < MAX_DRIVERS; i++) {
    indigo_driver *driver = drivers[i];
    if (driver == NULL)
      break;
    if (driver->change_property(driver, client, property))
      success = false;
  }
  return success ? INDIGO_OK : INDIGO_PARTIALLY_FAILED;
}

indigo_result indigo_define_property(indigo_driver *driver, indigo_property *property) {
  INDIGO_DEBUG(indigo_debug_property("INDIGO Bus: property definition", property, true, true));
  bool success = true;
  for (int i = 0; i < MAX_CLIENTS; i++) {
    indigo_client *client = clients[i];
    if (client == NULL)
      break;
    if (client->define_property(client, driver, property))
      success = false;
  }
  return success ? INDIGO_OK : INDIGO_PARTIALLY_FAILED;
}

indigo_result indigo_update_property(indigo_driver *driver, indigo_property *property) {
  INDIGO_DEBUG(indigo_debug_property("INDIGO Bus: property update", property, false, true));
  bool success = true;
  for (int i = 0; i < MAX_CLIENTS; i++) {
    indigo_client *client = clients[i];
    if (client == NULL)
      break;
    if (client->update_property(client, driver, property))
      success = false;
  }
  return success ? INDIGO_OK : INDIGO_PARTIALLY_FAILED;
}

indigo_result indigo_delete_property(indigo_driver *driver, indigo_property *property) {
  INDIGO_DEBUG(indigo_debug_property("INDIGO Bus: property removal", property, false, false));
  bool success = true;
  for (int i = 0; i < MAX_CLIENTS; i++) {
    indigo_client *client = clients[i];
    if (client == NULL)
      break;
    if (client->delete_property(client, driver, property))
      success = false;
  }
  return success ? INDIGO_OK : INDIGO_PARTIALLY_FAILED;
}

indigo_result indigo_stop() {
  int success = true;
  for (int i = 0; i < MAX_DRIVERS; i++) {
    indigo_driver *driver = drivers[i];
    if (driver == NULL)
      break;
    if (driver->stop(driver))
      success = false;
  }
  for (int i = 0; i < MAX_CLIENTS; i++) {
    indigo_client *client = clients[i];
    if (client == NULL)
      break;
    if (client->stop(client))
      success = false;
  }
  return success ? INDIGO_OK : INDIGO_PARTIALLY_FAILED;
}

indigo_property *indigo_allocate_text_property(const char *device, const char *name, const char *group, const char *label, indigo_property_state state, indigo_property_perm perm, int count) {
  int size = sizeof(indigo_property)+count*(sizeof(indigo_item));
  indigo_property *property = malloc(size);
  memset(property, 0, size);
  strncpy(property->device, device, NAME_SIZE);
  strncpy(property->name, name, NAME_SIZE);
  strncpy(property->group, group, NAME_SIZE);
  strncpy(property->label, label, VALUE_SIZE);
  property->type = INDIGO_TEXT_VECTOR;
  property->state = state;
  property->perm = perm;
  property->version = INDIGO_VERSION_CURRENT;
  property->count = count;
  return property;
}

indigo_property *indigo_allocate_number_property(const char *device, const char *name, const char *group, const char *label, indigo_property_state state, indigo_property_perm perm, int count) {
  int size = sizeof(indigo_property)+count*(sizeof(indigo_item));
  indigo_property *property = malloc(size);
  memset(property, 0, size);
  strncpy(property->device, device, NAME_SIZE);
  strncpy(property->name, name, NAME_SIZE);
  strncpy(property->group, group, NAME_SIZE);
  strncpy(property->label, label, VALUE_SIZE);
  property->type = INDIGO_NUMBER_VECTOR;
  property->state = state;
  property->perm = perm;
  property->version = INDIGO_VERSION_CURRENT;
  property->count = count;
  return property;
}

indigo_property *indigo_allocate_switch_property(const char *device, const char *name, const char *group, const char *label, indigo_property_state state, indigo_property_perm perm, indigo_rule rule, int count) {
  int size = sizeof(indigo_property)+count*(sizeof(indigo_item));
  indigo_property *property = malloc(size);
  memset(property, 0, size);
  strncpy(property->device, device, NAME_SIZE);
  strncpy(property->name, name, NAME_SIZE);
  strncpy(property->group, group, NAME_SIZE);
  strncpy(property->label, label, VALUE_SIZE);
  property->type = INDIGO_SWITCH_VECTOR;
  property->state = state;
  property->perm = perm;
  property->rule = rule;
  property->version = INDIGO_VERSION_CURRENT;
  property->count = count;
  return property;
}

indigo_property *indigo_allocate_light_property(const char *device, const char *name, const char *group, const char *label, indigo_property_state state, int count) {
  int size = sizeof(indigo_property)+count*(sizeof(indigo_item));
  indigo_property *property = malloc(size);
  memset(property, 0, size);
  strncpy(property->device, device, NAME_SIZE);
  strncpy(property->name, name, NAME_SIZE);
  strncpy(property->group, group, NAME_SIZE);
  strncpy(property->label, label, VALUE_SIZE);
  property->type = INDIGO_LIGHT_VECTOR;
  property->perm = INDIGO_RO_PERM;
  property->state = state;
  property->version = INDIGO_VERSION_CURRENT;
  property->count = count;
  return property;
}

indigo_property *indigo_allocate_blob_property(const char *device, const char *name, const char *group, const char *label, indigo_property_state state, int count) {
  int size = sizeof(indigo_property)+count*(sizeof(indigo_item));
  indigo_property *property = malloc(size);
  memset(property, 0, size);
  strncpy(property->device, device, NAME_SIZE);
  strncpy(property->name, name, NAME_SIZE);
  strncpy(property->group, group, NAME_SIZE);
  strncpy(property->label, label, VALUE_SIZE);
  property->type = INDIGO_BLOB_VECTOR;
  property->perm = INDIGO_RO_PERM;
  property->state = state;
  property->version = INDIGO_VERSION_CURRENT;
  property->count = count;
  return property;
}

void indigo_init_text_item(indigo_item *item, const char *name, const char *label, const char *value) {
  memset(item, 0, sizeof(indigo_item));
  strncpy(item->name, name, NAME_SIZE);
  strncpy(item->label, label, NAME_SIZE);
  strncpy(item->text_value, value, VALUE_SIZE);
}

void indigo_init_number_item(indigo_item *item, const char *name, const char *label, double min, double max, double step, double value) {
  memset(item, sizeof(indigo_item), 0);
  strncpy(item->name, name, NAME_SIZE);
  strncpy(item->label, label, VALUE_SIZE);
  item->number_min = min;
  item->number_max = max;
  item->number_step = step;
  item->number_value = value;
}

void indigo_init_switch_item(indigo_item *item, const char *name, const char *label, bool value) {
  memset(item, sizeof(indigo_item), 0);
  strncpy(item->name, name, NAME_SIZE);
  strncpy(item->label, label, VALUE_SIZE);
  item->switch_value = value;
}

void indigo_init_light_item(indigo_item *item, const char *name, const char *label, indigo_property_state value) {
  memset(item, sizeof(indigo_item), 0);
  strncpy(item->name, name, NAME_SIZE);
  strncpy(item->label, label, VALUE_SIZE);
  item->light_value = value;
}

void indigo_init_blob_item(indigo_item *item, const char *name, const char *label) {
  memset(item, sizeof(indigo_item), 0);
  strncpy(item->name, name, NAME_SIZE);
  strncpy(item->label, label, VALUE_SIZE);
}

bool indigo_property_match(indigo_property *property, indigo_property *other) {
  return property->type == other->type && !strcmp(property->device, other->device) & !strcmp(property->name, other->name);
}

void indigo_property_copy_values(indigo_property *property, indigo_property *other) {
  if (property->type == other->type) {
    property->state = other->state;
    for (int i = 0; i < other->count; i++) {
      indigo_item *other_item = &other->items[i];
      for (int j = 0; j < property->count; j++) {
        indigo_item *property_item = &property->items[j];
        if (!strcmp(property_item->name, other_item->name)) {
          switch (property->type) {
            case INDIGO_TEXT_VECTOR:
              strncpy(property_item->text_value, other_item->text_value, VALUE_SIZE);
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
              strncpy(property_item->blob_format, other_item->blob_format, NAME_SIZE);
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
