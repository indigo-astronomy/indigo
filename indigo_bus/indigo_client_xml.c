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

#include "indigo_xml.h"
#include "indigo_client_xml.h"

typedef struct {
  int input, output;
} indigo_xml_client_context;

static pthread_mutex_t xmutex = PTHREAD_MUTEX_INITIALIZER;

static void xprintf(indigo_driver *driver, const char *format, ...) {
  indigo_xml_client_context *driver_context = (indigo_xml_client_context *)driver->driver_context;
  char buffer[1024];
  va_list args;
  va_start(args, format);
  int length = vsnprintf(buffer, 1024, format, args);
  va_end(args);
  write(driver_context->output, buffer, length);
  INDIGO_TRACE(indigo_trace("client: %s", buffer));
}

static indigo_result xml_client_parser_init(indigo_driver *driver) {
  return INDIGO_OK;
}

static indigo_result xml_client_parser_start(indigo_driver *driver) {
  return INDIGO_OK;
}

static indigo_result xml_client_parser_enumerate_properties(indigo_driver *driver, indigo_client *client, indigo_property *property) {
  pthread_mutex_lock(&xmutex);
  if (property != NULL) {
    if (*property->device && *property->name) {
      xprintf(driver, "<getProperties version='%d.%d' device='%s' name='%s'/>\n", (property->version >> 8) & 0xFF, property->version & 0xFF, property->device, property->name);
    } else if (*property->device) {
      xprintf(driver, "<getProperties version='%d.%d' device='%s'/>\n", (property->version >> 8) & 0xFF, property->version & 0xFF, property->device);
    } else if (*property->name) {
      xprintf(driver, "<getProperties version='%d.%d' name='%s'/>\n", (property->version >> 8) & 0xFF, property->version & 0xFF, property->name);
    } else {
      xprintf(driver, "<getProperties version='%d.%d'/>\n", (property->version >> 8) & 0xFF, property->version & 0xFF);
    }
  } else {
    xprintf(driver, "<getProperties version='1.7'/>\n");
  }
  pthread_mutex_unlock(&xmutex);
  return INDIGO_OK;
}

static indigo_result xml_client_parser_change_property(indigo_driver *driver, indigo_client *client, indigo_property *property) {
  pthread_mutex_lock(&xmutex);
  switch (property->type) {
    case INDIGO_TEXT_VECTOR:
      xprintf(driver, "<newTextVector device='%s' name='%s' state='%s'>\n", property->device, property->name, indigo_property_state_text[property->state]);
      for (int i = 0; i < property->count; i++) {
        indigo_item *item = &property->items[i];
        xprintf(driver, "<oneText name='%s'>%s</oneText>\n", item->name, item->text_value);
      }
      xprintf(driver, "</newTextVector>\n");
      break;
    case INDIGO_NUMBER_VECTOR:
      xprintf(driver, "<newNumberVector device='%s' name='%s' state='%s'>\n", property->device, property->name, indigo_property_state_text[property->state]);
      for (int i = 0; i < property->count; i++) {
        indigo_item *item = &property->items[i];
        xprintf(driver, "<oneNumber name='%s'>%g</oneNumber>\n", item->name, item->number_value);
      }
      xprintf(driver, "</newNumberVector>\n");
      break;
    case INDIGO_SWITCH_VECTOR:
      xprintf(driver, "<newSwitchVector device='%s' name='%s' state='%s'>\n", property->device, property->name, indigo_property_state_text[property->state]);
      for (int i = 0; i < property->count; i++) {
        indigo_item *item = &property->items[i];
        xprintf(driver, "<oneSwitch name='%s'>%s</oneSwitch>\n", item->name, item->switch_value ? "On" : "Off");
      }
      xprintf(driver, "</newSwitchVector>\n");
      break;
    default:
      break;
  }
  pthread_mutex_unlock(&xmutex);
  return INDIGO_OK;
}

static indigo_result xml_client_parser_stop(indigo_driver *driver) {
  indigo_xml_client_context *driver_context = (indigo_xml_client_context *)driver->driver_context;
  close(driver_context->input);
  close(driver_context->output);
  return INDIGO_OK;
}

indigo_driver *xml_client_adapter(int input, int ouput) {
  static indigo_driver driver_template = {
    NULL,
    xml_client_parser_init,
    xml_client_parser_start,
    xml_client_parser_enumerate_properties,
    xml_client_parser_change_property,
    xml_client_parser_stop
  };
  indigo_driver *driver = malloc(sizeof(indigo_driver));
  memcpy(driver, &driver_template, sizeof(indigo_driver));
  indigo_xml_client_context *driver_context = malloc(sizeof(indigo_xml_client_context));
  driver_context->input = input;
  driver_context->output = ouput;
  driver->driver_context = driver_context;
  return driver;
}
