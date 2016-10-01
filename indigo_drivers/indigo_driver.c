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
#include <unistd.h>
#include <stdarg.h>
#include <string.h>
#include <pthread.h>
#include <errno.h>
#include <assert.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>

#include "indigo_driver.h"
#include "indigo_xml.h"

indigo_result indigo_device_attach(indigo_device *device, char *name, int version, int interface) {
  assert(device != NULL);
  assert(device != NULL);
  if (DEVICE_CONTEXT == NULL)
    device->device_context = malloc(sizeof(indigo_device_context));
  if (DEVICE_CONTEXT != NULL) {
    // -------------------------------------------------------------------------------- CONNECTION
    CONNECTION_PROPERTY = indigo_init_switch_property(NULL, name, "CONNECTION", MAIN_GROUP, "Connection", INDIGO_IDLE_STATE, INDIGO_RW_PERM, INDIGO_ONE_OF_MANY_RULE, 2);
    if (CONNECTION_PROPERTY == NULL)
      return INDIGO_FAILED;
    indigo_init_switch_item(CONNECTION_CONNECTED_ITEM, "CONNECTED", "Connected", false);
    indigo_init_switch_item(CONNECTION_DISCONNECTED_ITEM, "DISCONNECTED", "Disconnected", true);
    // -------------------------------------------------------------------------------- DEVICE_INFO
    INFO_PROPERTY = indigo_init_text_property(NULL, name, "DEVICE_INFO", OPTIONS_GROUP, "Device Info", INDIGO_IDLE_STATE, INDIGO_RO_PERM, 3);
    if (INFO_PROPERTY == NULL)
      return INDIGO_FAILED;
    indigo_init_text_item(INFO_DEVICE_NAME_ITEM, "NAME", "Name", name);
    indigo_init_text_item(INFO_DEVICE_VERSION_ITEM, "VERSION", "Version", "%d.%d", (version >> 8) & 0xFF, version & 0xFF);
    indigo_init_text_item(INFO_DEVICE_INTERFACE_ITEM, "INTERFACE", "Interface", "%d", interface);
    // -------------------------------------------------------------------------------- DEBUG
    DEBUG_PROPERTY = indigo_init_switch_property(NULL, name, "DEBUG", OPTIONS_GROUP, "Debug", INDIGO_IDLE_STATE, INDIGO_RW_PERM, INDIGO_ONE_OF_MANY_RULE, 2);
    if (DEBUG_PROPERTY == NULL)
      return INDIGO_FAILED;
    indigo_init_switch_item(DEBUG_ENABLED_ITEM, "ENABLED", "Enabled", false);
    indigo_init_switch_item(DEBUG_DISABLED_ITEM, "DISABLED", "Disabled", true);
    // -------------------------------------------------------------------------------- SIMULATION
    SIMULATION_PROPERTY = indigo_init_switch_property(NULL, name, "SIMULATION", OPTIONS_GROUP, "Simulation", INDIGO_IDLE_STATE, INDIGO_RW_PERM, INDIGO_ONE_OF_MANY_RULE, 2);
    if (SIMULATION_PROPERTY == NULL)
      return INDIGO_FAILED;
    indigo_init_switch_item(SIMULATION_ENABLED_ITEM, "ENABLED", "Enabled", false);
    indigo_init_switch_item(SIMULATION_DISABLED_ITEM, "DISABLED", "Disabled", true);
    // -------------------------------------------------------------------------------- CONFIG
    CONFIG_PROPERTY = indigo_init_switch_property(NULL, name, "CONFIG", OPTIONS_GROUP, "Configuration", INDIGO_IDLE_STATE, INDIGO_RW_PERM, INDIGO_ONE_OF_MANY_RULE, 3);
    if (CONFIG_PROPERTY == NULL)
      return INDIGO_FAILED;
    indigo_init_switch_item(CONFIG_LOAD_ITEM, "LOAD", "Load", false);
    indigo_init_switch_item(CONFIG_SAVE_ITEM, "SAVE", "Save", false);
    indigo_init_switch_item(CONFIG_DEFAULT_ITEM, "DEFAULT", "Default", false);
    return INDIGO_OK;
  }
  return INDIGO_FAILED;
}

indigo_result indigo_device_enumerate_properties(indigo_device *device, indigo_client *client, indigo_property *property) {
  assert(device != NULL);
  assert(device->device_context != NULL);
  assert(property != NULL);
  if (indigo_property_match(CONNECTION_PROPERTY, property))
    indigo_define_property(device, CONNECTION_PROPERTY, NULL);
  if (indigo_property_match(INFO_PROPERTY, property))
    indigo_define_property(device, INFO_PROPERTY, NULL);
  if (indigo_property_match(DEBUG_PROPERTY, property))
    indigo_define_property(device, DEBUG_PROPERTY, NULL);
  if (indigo_property_match(SIMULATION_PROPERTY, property))
    indigo_define_property(device, SIMULATION_PROPERTY, NULL);
  if (indigo_property_match(CONFIG_PROPERTY, property))
    indigo_define_property(device, CONFIG_PROPERTY, NULL);
  return INDIGO_OK;
}

indigo_result indigo_device_change_property(indigo_device *device, indigo_client *client, indigo_property *property) {
  assert(device != NULL);
  assert(device->device_context != NULL);
  assert(property != NULL);
  if (indigo_property_match(CONNECTION_PROPERTY, property)) {
    // -------------------------------------------------------------------------------- CONNECTION
    indigo_update_property(device, CONNECTION_PROPERTY, NULL);
  } else if (indigo_property_match(DEBUG_PROPERTY, property)) {
    // -------------------------------------------------------------------------------- DEBUG
    indigo_property_copy_values(DEBUG_PROPERTY, property, false);
    DEBUG_PROPERTY->state = INDIGO_OK_STATE;
    indigo_update_property(device, DEBUG_PROPERTY, NULL);
  } else if (indigo_property_match(SIMULATION_PROPERTY, property)) {
    // -------------------------------------------------------------------------------- SIMULATION
    indigo_property_copy_values(SIMULATION_PROPERTY, property, false);
    SIMULATION_PROPERTY->state = INDIGO_OK_STATE;
    indigo_update_property(device, SIMULATION_PROPERTY, NULL);
  } else if (indigo_property_match(CONFIG_PROPERTY, property)) {
    // -------------------------------------------------------------------------------- CONFIG
    if (CONFIG_LOAD_ITEM->switch_value) {
      if (indigo_load_properties(device, false) == INDIGO_OK)
        CONFIG_PROPERTY->state = INDIGO_OK_STATE;
      else
        CONFIG_PROPERTY->state = INDIGO_ALERT_STATE;
      CONFIG_LOAD_ITEM->switch_value = false;
    } else if (CONFIG_SAVE_ITEM->switch_value) {
      if (DEBUG_PROPERTY->perm == INDIGO_RW_PERM)
        indigo_save_property(DEBUG_PROPERTY);
      if (SIMULATION_PROPERTY->perm == INDIGO_RW_PERM)
        indigo_save_property(SIMULATION_PROPERTY);
      if (indigo_save_properties(device) == INDIGO_OK)
        CONFIG_PROPERTY->state = INDIGO_OK_STATE;
      else
        CONFIG_PROPERTY->state = INDIGO_ALERT_STATE;
      CONFIG_SAVE_ITEM->switch_value = false;
    } else if (CONFIG_DEFAULT_ITEM->switch_value) {
      if (indigo_load_properties(device, true) == INDIGO_OK)
        CONFIG_PROPERTY->state = INDIGO_OK_STATE;
      else
        CONFIG_PROPERTY->state = INDIGO_ALERT_STATE;
      CONFIG_DEFAULT_ITEM->switch_value = false;
    }
    indigo_update_property(device, CONFIG_PROPERTY, NULL);
  }
  return INDIGO_OK;
}

indigo_result indigo_device_detach(indigo_device *device) {
  return INDIGO_OK;
}

static pthread_mutex_t save_mutex = PTHREAD_RECURSIVE_MUTEX_INITIALIZER;
static indigo_property *save_properties[INDIGO_MAX_PROPERTIES];
static int save_property_count = 0;

static void xprintf(int handle, const char *format, ...) {
  char buffer[1024];
  va_list args;
  va_start(args, format);
  int length = vsnprintf(buffer, 1024, format, args);
  va_end(args);
  write(handle, buffer, length);
  INDIGO_DEBUG(indigo_debug("saved: %s", buffer));
}

static int open_config_file(indigo_device *device, int mode, const char *suffix) {
  assert(device != NULL);
  pthread_mutex_lock(&save_mutex);
  static char path[128];
  int path_end = snprintf(path, sizeof(path), "%s/.indigo", getenv("HOME"));
  int handle = mkdir(path, 0777);
  if (handle == 0 || errno == EEXIST) {
    snprintf(path + path_end, sizeof(path) - path_end, "/%s%s", INFO_DEVICE_NAME_ITEM->text_value, suffix);
    char *space = strchr(path, ' ');
    while (space != NULL) {
      *space = '_';
      space = strchr(space+1, ' ');
    }
    handle = open(path, mode, 0644);
    if (handle < 0)
      indigo_error("Can't create %s (%s)", path, strerror(errno));
  } else {
    indigo_error("Can't create %s (%s)", path, strerror(errno));
  }
  return handle;
}

indigo_result indigo_load_properties(indigo_device *device, bool default_properties) {
  assert(device != NULL);
  pthread_mutex_lock(&save_mutex);
  int handle = open_config_file(device, O_RDONLY, default_properties ? ".default" : ".config");
  if (handle > 0) {
    indigo_xml_parse(handle, NULL, NULL);    
    close(handle);
  }
  pthread_mutex_unlock(&save_mutex);
  return handle > 0 ? INDIGO_OK : INDIGO_FAILED;
}

void indigo_save_property(indigo_property *property) {
  if (save_property_count < INDIGO_MAX_PROPERTIES) {
    pthread_mutex_lock(&save_mutex);
    save_properties[save_property_count++] = property;
  }
}

indigo_result indigo_save_properties(indigo_device *device) {
  assert(device != NULL);
  pthread_mutex_lock(&save_mutex);
  int handle = open_config_file(device, O_WRONLY | O_CREAT | O_TRUNC, ".config");
  for (int i = 0; i < save_property_count; i++) {
    if (handle > 0) {
      indigo_property *property = save_properties[i];
      switch (property->type) {
        case INDIGO_TEXT_VECTOR:
          xprintf(handle, "<newTextVector device='%s' name='%s'>\n", property->device, property->name, indigo_property_state_text[property->state]);
          for (int i = 0; i < property->count; i++) {
            indigo_item *item = &property->items[i];
            xprintf(handle, "<oneText name='%s'>%s</oneText>\n", item->name, item->text_value);
          }
          xprintf(handle, "</newTextVector>\n");
          break;
        case INDIGO_NUMBER_VECTOR:
          xprintf(handle, "<newNumberVector device='%s' name='%s'>\n", property->device, property->name, indigo_property_state_text[property->state]);
          for (int i = 0; i < property->count; i++) {
            indigo_item *item = &property->items[i];
            xprintf(handle, "<oneNumber name='%s'>%g</oneNumber>\n", item->name, item->number_value);
          }
          xprintf(handle, "</newNumberVector>\n");
          break;
        case INDIGO_SWITCH_VECTOR:
          xprintf(handle, "<newSwitchVector device='%s' name='%s'>\n", property->device, property->name, indigo_property_state_text[property->state]);
          for (int i = 0; i < property->count; i++) {
            indigo_item *item = &property->items[i];
            xprintf(handle, "<oneSwitch name='%s'>%s</oneSwitch>\n", item->name, item->switch_value ? "On" : "Off");
          }
          xprintf(handle, "</newSwitchVector>\n");
          break;
        default:
          break;
      }
    }
    pthread_mutex_unlock(&save_mutex);
  }
  if (handle > 0)
    close(handle);
  pthread_mutex_unlock(&save_mutex);
  return handle > 0 ? INDIGO_OK : INDIGO_FAILED;
}

