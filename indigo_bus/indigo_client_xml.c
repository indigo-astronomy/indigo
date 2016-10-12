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


#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <stdio.h>
#include <unistd.h>
#include <ctype.h>
#include <pthread.h>
#include <assert.h>

#include "indigo_xml.h"
#include "indigo_version.h"
#include "indigo_client_xml.h"

static pthread_mutex_t xmutex = PTHREAD_MUTEX_INITIALIZER;

static indigo_result xml_client_parser_enumerate_properties(indigo_device *device, indigo_client *client, indigo_property *property) {
  assert(device != NULL);
  pthread_mutex_lock(&xmutex);
  indigo_xml_client_adapter_context *device_context = (indigo_xml_client_adapter_context *)device->device_context;
  assert(device_context != NULL);
  int handle = device_context->output;
  if (property != NULL) {
    if (*property->device && *indigo_property_name(device->version, property)) {
      indigo_xml_printf(handle, "<getProperties version='1.7' switch='%d.%d' device='%s' name='%s'/>\n", (device->version >> 8) & 0xFF, device->version & 0xFF, property->device, indigo_property_name(device->version, property));
    } else if (*property->device) {
      indigo_xml_printf(handle, "<getProperties version='1.7' switch='%d.%d' device='%s'/>\n", (device->version >> 8) & 0xFF, device->version & 0xFF, property->device);
    } else if (*indigo_property_name(device->version, property)) {
      indigo_xml_printf(handle, "<getProperties version='1.7' switch='%d.%d' name='%s'/>\n", (device->version >> 8) & 0xFF, device->version & 0xFF, indigo_property_name(device->version, property));
    } else {
      indigo_xml_printf(handle, "<getProperties version='1.7' switch='%d.%d'/>\n", (device->version >> 8) & 0xFF, device->version & 0xFF);
    }
  } else {
    indigo_xml_printf(handle, "<getProperties version='1.7' switch='%d.%d'/>\n", (device->version >> 8) & 0xFF, device->version & 0xFF);
  }
  pthread_mutex_unlock(&xmutex);
  return INDIGO_OK;
}

static indigo_result xml_client_parser_change_property(indigo_device *device, indigo_client *client, indigo_property *property) {
  assert(device != NULL);
  assert(property != NULL);
  pthread_mutex_lock(&xmutex);
  indigo_xml_client_adapter_context *device_context = (indigo_xml_client_adapter_context *)device->device_context;
  assert(device_context != NULL);
  int handle = device_context->output;
  switch (property->type) {
    case INDIGO_TEXT_VECTOR:
      indigo_xml_printf(handle, "<newTextVector device='%s' name='%s'>\n", property->device, indigo_property_name(device->version, property), indigo_property_state_text[property->state]);
      for (int i = 0; i < property->count; i++) {
        indigo_item *item = &property->items[i];
        indigo_xml_printf(handle, "<oneText name='%s'>%s</oneText>\n", indigo_item_name(device->version, property, item), item->text.value);
      }
      indigo_xml_printf(handle, "</newTextVector>\n");
      break;
    case INDIGO_NUMBER_VECTOR:
      indigo_xml_printf(handle, "<newNumberVector device='%s' name='%s'>\n", property->device, indigo_property_name(device->version, property), indigo_property_state_text[property->state]);
      for (int i = 0; i < property->count; i++) {
        indigo_item *item = &property->items[i];
        indigo_xml_printf(handle, "<oneNumber name='%s'>%g</oneNumber>\n", indigo_item_name(device->version, property, item), item->number.value);
      }
      indigo_xml_printf(handle, "</newNumberVector>\n");
      break;
    case INDIGO_SWITCH_VECTOR:
      indigo_xml_printf(handle, "<newSwitchVector device='%s' name='%s'>\n", property->device, indigo_property_name(device->version, property), indigo_property_state_text[property->state]);
      for (int i = 0; i < property->count; i++) {
        indigo_item *item = &property->items[i];
        indigo_xml_printf(handle, "<oneSwitch name='%s'>%s</oneSwitch>\n", indigo_item_name(device->version, property, item), item->sw.value ? "On" : "Off");
      }
      indigo_xml_printf(handle, "</newSwitchVector>\n");
      break;
    default:
      break;
  }
  pthread_mutex_unlock(&xmutex);
  return INDIGO_OK;
}

static indigo_result xml_client_parser_detach(indigo_device *device) {
  assert(device != NULL);
  indigo_xml_client_adapter_context *device_context = (indigo_xml_client_adapter_context *)device->device_context;
  close(device_context->input);
  close(device_context->output);
  return INDIGO_OK;
}

indigo_device *xml_client_adapter(int input, int ouput) {
  static indigo_device device_template = {
    NULL, INDIGO_OK, INDIGO_VERSION_CURRENT,
    NULL,
    xml_client_parser_enumerate_properties,
    xml_client_parser_change_property,
    xml_client_parser_detach
  };
  indigo_device *device = malloc(sizeof(indigo_device));
  if (device != NULL) {
    memcpy(device, &device_template, sizeof(indigo_device));
    indigo_xml_client_adapter_context *device_context = malloc(sizeof(indigo_xml_client_adapter_context));
    device_context->input = input;
    device_context->output = ouput;
    device->device_context = device_context;
  }
  return device;
}
