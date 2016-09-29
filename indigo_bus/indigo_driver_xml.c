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
#include "indigo_driver_xml.h"

static pthread_mutex_t xmutex = PTHREAD_MUTEX_INITIALIZER;

static char encoding_table[] =
{ 'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H',
  'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P',
  'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X',
  'Y', 'Z', 'a', 'b', 'c', 'd', 'e', 'f',
  'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n',
  'o', 'p', 'q', 'r', 's', 't', 'u', 'v',
  'w', 'x', 'y', 'z', '0', '1', '2', '3',
  '4', '5', '6', '7', '8', '9', '+', '/'
};

static int mod_table[] = {0, 2, 1};

static const char *message_attribute(const char *message) {
  if (message) {
    static char buffer[INDIGO_VALUE_SIZE];
    snprintf(buffer, INDIGO_VALUE_SIZE, " message='%s'", message);
    return buffer;
  }
  return "";
}

static indigo_result xml_driver_adapter_define_property(indigo_client *client, struct indigo_driver *driver, indigo_property *property, const char *message) {
  assert(driver != NULL);
  assert(client != NULL);
  assert(property != NULL);
  INDIGO_LOCK(&xmutex);
  indigo_xml_driver_adapter_context *client_context = (indigo_xml_driver_adapter_context *)client->client_context;
  assert(client_context != NULL);
  int handle = client_context->output;
  switch (property->type) {
    case INDIGO_TEXT_VECTOR:
      indigo_xml_prinf(handle, "<defTextVector device='%s' name='%s' group='%s' label='%s' perm='%s' state='%s'%s>\n", property->device, property->name, property->group, property->label, indigo_property_perm_text[property->perm], indigo_property_state_text[property->state], message_attribute(message));
      for (int i = 0; i < property->count; i++) {
        indigo_item *item = &property->items[i];
        indigo_xml_prinf(handle, "<defText name='%s' label='%s'>%s</defText>\n", item->name, item->label, item->text_value);
      }
      indigo_xml_prinf(handle, "</defTextVector>\n");
      break;
    case INDIGO_NUMBER_VECTOR:
      indigo_xml_prinf(handle, "<defNumberVector device='%s' name='%s' group='%s' label='%s' perm='%s' state='%s'%s>\n", property->device, property->name, property->group, property->label, indigo_property_perm_text[property->perm], indigo_property_state_text[property->state], message_attribute(message));
      for (int i = 0; i < property->count; i++) {
        indigo_item *item = &property->items[i];
        indigo_xml_prinf(handle, "<defNumber name='%s' label='%s' min='%g' max='%g' step='%g'>%g</defNumber>\n", item->name, item->label, item->number_min, item->number_max, item->number_step, item->number_value);
      }
      indigo_xml_prinf(handle, "</defNumberVector>\n");
      break;
    case INDIGO_SWITCH_VECTOR:
      indigo_xml_prinf(handle, "<defSwitchVector device='%s' name='%s' group='%s' label='%s' perm='%s' state='%s' rule='%s'%s>\n", property->device, property->name, property->group, property->label, indigo_property_perm_text[property->perm], indigo_property_state_text[property->state], indigo_switch_rule_text[property->rule], message_attribute(message));
      for (int i = 0; i < property->count; i++) {
        indigo_item *item = &property->items[i];
        indigo_xml_prinf(handle, "<defSwitch name='%s' label='%s'>%s</defSwitch>\n", item->name, item->label, item->switch_value ? "On" : "Off");
      }
      indigo_xml_prinf(handle, "</defSwitchVector>\n");
      break;
    case INDIGO_LIGHT_VECTOR:
      indigo_xml_prinf(handle, "<defLightVector device='%s' name='%s' group='%s' label='%s' perm='%s' state='%s'%s>\n", property->device, property->name, property->group, property->label, indigo_property_perm_text[property->perm], indigo_property_state_text[property->state], message_attribute(message));
      for (int i = 0; i < property->count; i++) {
        indigo_item *item = &property->items[i];
        indigo_xml_prinf(handle, " <defLight name='%s' label='%s'>%s</defLight>\n", item->name, item->label, indigo_property_state_text[item->light_value]);
      }
      indigo_xml_prinf(handle, "</defLightVector>\n");
      break;
    case INDIGO_BLOB_VECTOR:
      indigo_xml_prinf(handle, "<defBLOBVector device='%s' name='%s' group='%s' label='%s' perm='%s' state='%s'%s>\n", property->device, property->name, property->group, property->label, indigo_property_perm_text[property->perm], indigo_property_state_text[property->state], message_attribute(message));
      for (int i = 0; i < property->count; i++) {
        indigo_item *item = &property->items[i];
        indigo_xml_prinf(handle, "<defBLOB name='%s' label='%s'/>\n", item->name, item->label);
      }
      indigo_xml_prinf(handle, "</defBLOBVector>\n");
      break;
  }
  INDIGO_UNLOCK(&xmutex);
  return INDIGO_OK;
}

static indigo_result xml_driver_adapter_update_property(indigo_client *client, indigo_driver *driver, indigo_property *property, const char *message) {
  assert(driver != NULL);
  assert(client != NULL);
  assert(property != NULL);
  INDIGO_LOCK(&xmutex);
  indigo_xml_driver_adapter_context *client_context = (indigo_xml_driver_adapter_context *)client->client_context;
  assert(client_context != NULL);
  int handle = client_context->output;
  switch (property->type) {
    case INDIGO_TEXT_VECTOR:
      indigo_xml_prinf(handle, "<setTextVector device='%s' name='%s' state='%s'%s>\n", property->device, property->name, indigo_property_state_text[property->state], message_attribute(message));
      for (int i = 0; i < property->count; i++) {
        indigo_item *item = &property->items[i];
        indigo_xml_prinf(handle, "<oneText name='%s'>%s</oneText>\n", item->name, item->text_value);
      }
      indigo_xml_prinf(handle, "</setTextVector>\n");
      break;
    case INDIGO_NUMBER_VECTOR:
      indigo_xml_prinf(handle, "<setNumberVector device='%s' name='%s' state='%s'%s>\n", property->device, property->name, indigo_property_state_text[property->state], message_attribute(message));
      for (int i = 0; i < property->count; i++) {
        indigo_item *item = &property->items[i];
        indigo_xml_prinf(handle, "<oneNumber name='%s'>%g</oneNumber>\n", item->name, item->number_value);
      }
      indigo_xml_prinf(handle, "</setNumberVector>\n");
      break;
    case INDIGO_SWITCH_VECTOR:
      indigo_xml_prinf(handle, "<setSwitchVector device='%s' name='%s' state='%s'%s>\n", property->device, property->name, indigo_property_state_text[property->state], message_attribute(message));
      for (int i = 0; i < property->count; i++) {
        indigo_item *item = &property->items[i];
        indigo_xml_prinf(handle, "<oneSwitch name='%s'>%s</oneSwitch>\n", item->name, item->switch_value ? "On" : "Off");
      }
      indigo_xml_prinf(handle, "</setSwitchVector>\n");
      break;
    case INDIGO_LIGHT_VECTOR:
      indigo_xml_prinf(handle, "<setLightVector device='%s' name='%s' state='%s'%s>\n", property->device, property->name, indigo_property_state_text[property->state], message_attribute(message));
      for (int i = 0; i < property->count; i++) {
        indigo_item *item = &property->items[i];
        indigo_xml_prinf(handle, "<oneLight name='%s'>%s</oneLight>\n", item->name, indigo_property_state_text[item->light_value]);
      }
      indigo_xml_prinf(handle, "</setLightVector>\n");
      break;
    case INDIGO_BLOB_VECTOR:
      indigo_xml_prinf(handle, "<setBLOBVector device='%s' name='%s' state='%s'%s>\n", property->device, property->name, indigo_property_state_text[property->state], message_attribute(message));
      if (property->state == INDIGO_OK_STATE) {
        for (int i = 0; i < property->count; i++) {
          indigo_item *item = &property->items[i];
          long input_length = item->blob_size;
          unsigned char *data = item->blob_value;
          char encoded_data[74];
          indigo_xml_prinf(handle, "<oneBLOB name='%s' format='%s' size='%ld'>\n", item->name, item->blob_format, item->blob_size);
          int j = 0;
          int i = 0;
          while (i < input_length) {
            unsigned int octet_a = i < input_length ? (unsigned char)data[i++] : 0;
            unsigned int octet_b = i < input_length ? (unsigned char)data[i++] : 0;
            unsigned int octet_c = i < input_length ? (unsigned char)data[i++] : 0;
            unsigned int triple = (octet_a << 0x10) + (octet_b << 0x08) + octet_c;
            encoded_data[j++] = encoding_table[(triple >> 3 * 6) & 0x3F];
            encoded_data[j++] = encoding_table[(triple >> 2 * 6) & 0x3F];
            encoded_data[j++] = encoding_table[(triple >> 1 * 6) & 0x3F];
            encoded_data[j++] = encoding_table[(triple >> 0 * 6) & 0x3F];
            if (j == 72) {
              encoded_data[j++] = '\n';
              encoded_data[j++] = 0;
              indigo_xml_prinf(handle, encoded_data);
              j = 0;
            }
          }
          for (int i = 0; i < mod_table[input_length % 3]; i++) {
            encoded_data[j++] = '=';
          }
          encoded_data[j++] = '\n';
          encoded_data[j++] = 0;
          indigo_xml_prinf(handle, encoded_data);
          indigo_xml_prinf(handle, "</oneBLOB>\n");
        }
      }
      indigo_xml_prinf(handle, "</setBLOBVector>\n");
      break;
  }
  INDIGO_UNLOCK(&xmutex);
  return INDIGO_OK;
}

static indigo_result xml_driver_adapter_delete_property(indigo_client *client, indigo_driver *driver, indigo_property *property, const char *message) {
  assert(driver != NULL);
  assert(client != NULL);
  assert(property != NULL);
  INDIGO_LOCK(&xmutex);
  indigo_xml_driver_adapter_context *client_context = (indigo_xml_driver_adapter_context *)client->client_context;
  assert(client_context != NULL);
  int handle = client_context->output;
  indigo_xml_prinf(handle, "<delProperty device='%s' name='%s'%s/>\n", property->device, property->name, message_attribute(message));
  INDIGO_UNLOCK(&xmutex);
  return INDIGO_OK;
}

static indigo_result xml_driver_adapter_send_message(indigo_client *client, indigo_driver *driver, const char *message) {
  assert(driver != NULL);
  assert(client != NULL);
  INDIGO_LOCK(&xmutex);
  indigo_xml_driver_adapter_context *client_context = (indigo_xml_driver_adapter_context *)client->client_context;
  assert(client_context != NULL);
  int handle = client_context->output;
  if (message)
    indigo_xml_prinf(handle, "<message%s/>\n", message_attribute(message));
  INDIGO_UNLOCK(&xmutex);
  return INDIGO_OK;
}

indigo_client *xml_driver_adapter(int input, int ouput) {
  static indigo_client client_template = {
    NULL, INDIGO_OK, INDIGO_VERSION_CURRENT,
    NULL,
    xml_driver_adapter_define_property,
    xml_driver_adapter_update_property,
    xml_driver_adapter_delete_property,
    xml_driver_adapter_send_message,
    NULL
  };
  indigo_client *client = malloc(sizeof(indigo_client));
  if (client != NULL) {
    memcpy(client, &client_template, sizeof(indigo_client));
    indigo_xml_driver_adapter_context *client_context = malloc(sizeof(indigo_xml_driver_adapter_context));
    client_context->input = input;
    client_context->output = ouput;
    client->client_context = client_context;
  }
  return client;
}
