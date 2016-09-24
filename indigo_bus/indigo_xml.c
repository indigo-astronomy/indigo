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

//#define INDIGO_TRACE(c) c

#define BUFFER_SIZE 4096
#define PROPERTY_SIZE sizeof(indigo_property)+MAX_ITEMS*(sizeof(indigo_item))

typedef enum {
  ERROR,
  IDLE,
  BEGIN_TAG1,
  BEGIN_TAG,
  ATTRIBUTE_NAME1,
  ATTRIBUTE_NAME,
  ATTRIBUTE_VALUE1,
  ATTRIBUTE_VALUE,
  TEXT1,
  TEXT,
  END_TAG1,
  END_TAG2,
  END_TAG
} parser_state;

typedef void *(* parser_handler)(parser_state state, char *name, char *value, indigo_property *property, indigo_driver *driver, indigo_client *client);

void *top_level_handler(parser_state state, char *name, char *value, indigo_property *property, indigo_driver *driver, indigo_client *client);
void *new_text_vector_handler(parser_state state, char *name, char *value, indigo_property *property, indigo_driver *driver, indigo_client *client);
void *new_number_vector_handler(parser_state state, char *name, char *value, indigo_property *property, indigo_driver *driver, indigo_client *client);
void *new_switch_vector_handler(parser_state state, char *name, char *value, indigo_property *property, indigo_driver *driver, indigo_client *client);

void *get_properties_handler(parser_state state, char *name, char *value, indigo_property *property, indigo_driver *driver, indigo_client *client) {
  if (state == ATTRIBUTE_VALUE) {
    if (!strcmp(name, "version")) {
      if (!strcmp(value, "1.7"))
        property->version = INDIGO_VERSION_LEGACY;
      else if (!strcmp(value, "2.0"))
        property->version = INDIGO_VERSION_2_0;
      else
        property->version = INDIGO_VERSION_NONE;
    } else if (!strncmp(name, "device", NAME_SIZE)) {
      strcpy(property->device, value);
    } else if (!strncmp(name, "name", NAME_SIZE)) {
      strcpy(property->name, value);
    }
    return get_properties_handler;
  } else if (state == TEXT) {
    return get_properties_handler;
  } else if (state == END_TAG) {
    if (property->version != INDIGO_VERSION_NONE) {
      indigo_enumerate_properties(client, property);
    } else {
      indigo_error("XML Parser error: unknown protocol version");
    }
    int version = property->version;
    memset(property, 0, PROPERTY_SIZE);
    property->version = version;
  }
  return top_level_handler;
}

void *new_one_text_vector_handler(parser_state state, char *name, char *value, indigo_property *property, indigo_driver *driver, indigo_client *client) {
  if (state == ATTRIBUTE_VALUE) {
    if (!strcmp(name, "name")) {
      strncpy(property->items[property->count-1].name, value, NAME_SIZE);
      return new_one_text_vector_handler;
    }
  } else if (state == TEXT) {
    strncat(property->items[property->count-1].text_value, value, VALUE_SIZE);
    return new_one_text_vector_handler;
  }
  return new_text_vector_handler;
}

void *new_text_vector_handler(parser_state state, char *name, char *value, indigo_property *property, indigo_driver *driver, indigo_client *client) {
  if (state == BEGIN_TAG) {
    if (!strcmp(name, "oneText")) {
      property->count++;
      return new_one_text_vector_handler;
    }
  } else if (state == ATTRIBUTE_VALUE) {
    if (!strcmp(name, "device")) {
      strncpy(property->device, value, NAME_SIZE);
    } else if (!strcmp(name, "name")) {
      strncpy(property->name, value, NAME_SIZE);
    } else if (!strcmp(name, "state")) {
      if (!strcmp(value, "Idle")) {
        property->state = INDIGO_OK_STATE;
      } else if (!strcmp(value, "Ok")) {
        property->state = INDIGO_OK_STATE;
      } else if (!strcmp(value, "Busy")) {
        property->state = INDIGO_BUSY_STATE;
      } else if (!strcmp(value, "Alert")) {
        property->state = INDIGO_ALERT_STATE;
      }
    }
    return new_text_vector_handler;
  } else if (state == TEXT) {
    return new_text_vector_handler;
  } else if (state == END_TAG) {
    if (property->version != INDIGO_VERSION_NONE) {
      property->type = INDIGO_TEXT_VECTOR;
      indigo_change_property(client, property);
    } else {
      indigo_error("XML Parser error: unknown protocol version");
    }
    int version = property->version;
    memset(property, 0, PROPERTY_SIZE);
    property->version = version;
  }
  return top_level_handler;
}

void *new_one_number_vector_handler(parser_state state, char *name, char *value, indigo_property *property, indigo_driver *driver, indigo_client *client) {
  if (state == ATTRIBUTE_VALUE) {
    if (!strcmp(name, "name")) {
      strncpy(property->items[property->count-1].name, value, NAME_SIZE);
      return new_one_number_vector_handler;
    }
  } else if (state == TEXT) {
    property->items[property->count-1].number_value = atof(value);
    return new_one_number_vector_handler;
  }
  return new_number_vector_handler;
}

void *new_number_vector_handler(parser_state state, char *name, char *value, indigo_property *property, indigo_driver *driver, indigo_client *client) {
  if (state == BEGIN_TAG) {
    if (!strcmp(name, "oneNumber")) {
      property->count++;
      return new_one_number_vector_handler;
    }
  } else if (state == ATTRIBUTE_VALUE) {
    if (!strcmp(name, "device")) {
      strncpy(property->device, value, NAME_SIZE);
    } else if (!strcmp(name, "name")) {
      strncpy(property->name, value, NAME_SIZE);
    } else if (!strcmp(name, "state")) {
      if (!strcmp(value, "Idle")) {
        property->state = INDIGO_OK_STATE;
      } else if (!strcmp(value, "Ok")) {
        property->state = INDIGO_OK_STATE;
      } else if (!strcmp(value, "Busy")) {
        property->state = INDIGO_BUSY_STATE;
      } else if (!strcmp(value, "Alert")) {
        property->state = INDIGO_ALERT_STATE;
      }
    }
    return new_number_vector_handler;
  } else if (state == TEXT) {
    return new_number_vector_handler;
  } else if (state == END_TAG) {
    if (property->version != INDIGO_VERSION_NONE) {
      property->type = INDIGO_NUMBER_VECTOR;
      indigo_change_property(client, property);
    } else {
      indigo_error("XML Parser error: unknown protocol version");
    }
    int version = property->version;
    memset(property, 0, PROPERTY_SIZE);
    property->version = version;
  }
  return top_level_handler;
}

void *new_one_switch_vector_handler(parser_state state, char *name, char *value, indigo_property *property, indigo_driver *driver, indigo_client *client) {
  if (state == ATTRIBUTE_VALUE) {
    if (!strcmp(name, "name")) {
      strncpy(property->items[property->count-1].name, value, NAME_SIZE);
      return new_one_switch_vector_handler;
    }
  } else if (state == TEXT) {
    property->items[property->count-1].switch_value = !strcmp(value, "On");
    return new_one_switch_vector_handler;
  }
  return new_switch_vector_handler;
}

void *new_switch_vector_handler(parser_state state, char *name, char *value, indigo_property *property, indigo_driver *driver, indigo_client *client) {
  if (state == BEGIN_TAG) {
    if (!strcmp(name, "oneSwitch")) {
      property->count++;
      return new_one_switch_vector_handler;
    }
  } else if (state == ATTRIBUTE_VALUE) {
    if (!strcmp(name, "device")) {
      strncpy(property->device, value, NAME_SIZE);
    } else if (!strcmp(name, "name")) {
      strncpy(property->name, value, NAME_SIZE);
    } else if (!strcmp(name, "state")) {
      if (!strcmp(value, "Idle")) {
        property->state = INDIGO_OK_STATE;
      } else if (!strcmp(value, "Ok")) {
        property->state = INDIGO_OK_STATE;
      } else if (!strcmp(value, "Busy")) {
        property->state = INDIGO_BUSY_STATE;
      } else if (!strcmp(value, "Alert")) {
        property->state = INDIGO_ALERT_STATE;
      }
    }
    return new_switch_vector_handler;
  } else if (state == TEXT) {
    return new_switch_vector_handler;
  } else if (state == END_TAG) {
    if (property->version != INDIGO_VERSION_NONE) {
      property->type = INDIGO_SWITCH_VECTOR;
      indigo_change_property(client, property);
    } else {
      indigo_error("XML Parser error: unknown protocol version");
    }
    int version = property->version;
    memset(property, 0, PROPERTY_SIZE);
    property->version = version;
  }
  return top_level_handler;
}

void *top_level_handler(parser_state state, char *name, char *value, indigo_property *property, indigo_driver *driver, indigo_client *client) {
  if (state == BEGIN_TAG) {
    if (!strcmp(name, "getProperties"))
      return get_properties_handler;
    if (!strcmp(name, "newTextVector"))
      return new_text_vector_handler;
    if (!strcmp(name, "newNumberVector"))
      return new_number_vector_handler;
    if (!strcmp(name, "newSwitchVector"))
      return new_switch_vector_handler;
  }
  return top_level_handler;
}

void indigo_xml_parse(int handle, indigo_driver *driver, indigo_client *client) {
  char buffer[BUFFER_SIZE] = { 0 };
  char name_buffer[NAME_SIZE];
  char value_buffer[VALUE_SIZE];
  char property_buffer[PROPERTY_SIZE];

  char *pointer = buffer;
  char *name_pointer = name_buffer;
  char *value_pointer = value_buffer;
  parser_state state = IDLE;
  char q = '"';
  int depth = 0;
  char c;
  parser_handler handler = top_level_handler;
  indigo_property *property = (indigo_property *)property_buffer;
  memset(property_buffer, 0, PROPERTY_SIZE);
  
  while (true) {
    if (state == ERROR) {
      indigo_error("XML Parser error");
      state = IDLE;
    }
    while ((c = *pointer++) == 0) {
      ssize_t count = (int)read(handle, (void *)buffer, (ssize_t)BUFFER_SIZE);
      pointer = buffer;
      buffer[count] = 0;
    }
    
    switch (state) {
      case IDLE:
        if (c == '<') {
          state = BEGIN_TAG1;
          INDIGO_TRACE(indigo_trace("XML Parser: '%c' IDLE -> BEGIN_TAG1", c));
        }
        break;
      case BEGIN_TAG1:
        name_pointer = name_buffer;
        if (isalpha(c)) {
          *name_pointer++ = c;
          state = BEGIN_TAG;
          INDIGO_TRACE(indigo_trace("XML Parser: '%c' BEGIN_TAG1 -> BEGIN_TAG", c));
        } else if (c == '/') {
          state = END_TAG;
          INDIGO_TRACE(indigo_trace("XML Parser: '%c' BEGIN_TAG1 -> END_TAG", c));
        }
        break;
      case BEGIN_TAG:
        if (name_pointer - name_buffer < NAME_SIZE && isalpha(c)) {
          *name_pointer++ = c;
          INDIGO_TRACE(indigo_trace("XML Parser: '%c' BEGIN_TAG", c));
        } else {
          *name_pointer = 0;
          depth++;
          handler = handler(BEGIN_TAG, name_buffer, NULL, property, driver, client);
          if (isblank(c)) {
            state = ATTRIBUTE_NAME1;
            INDIGO_TRACE(indigo_trace("XML Parser: '%c' BEGIN_TAG -> ATTRIBUTE_NAME1", c));
          } else if (c == '/') {
            state = END_TAG1;
            INDIGO_TRACE(indigo_trace("XML Parser: '%c' BEGIN_TAG -> END_TAG1", c));
          } else if (c == '>') {
            state = TEXT;
            value_pointer = value_buffer;
            INDIGO_TRACE(indigo_trace("XML Parser: '%c' BEGIN_TAG -> TEXT", c));
          } else {
            state = ERROR;
            INDIGO_TRACE(indigo_trace("XML Parser: '%c' error BEGIN_TAG", c));
          }
        }
        break;
      case END_TAG1:
        if (c == '>') {
          INDIGO_TRACE(indigo_trace("XML Parser: '%c' END_TAG1 -> END_TAG", c));
          handler = handler(END_TAG, NULL, NULL, property, driver, client);
          depth--;
          state = IDLE;
        } else {
          state = ERROR;
          INDIGO_TRACE(indigo_trace("XML Parser: '%c' error END_TAG1", c));
        }
        break;
      case END_TAG2:
        if (c == '/') {
          state = END_TAG;
          INDIGO_TRACE(indigo_trace("XML Parser: '%c' END_TAG2 -> END_TAG", c));
        } else {
          state = ERROR;
          INDIGO_TRACE(indigo_trace("XML Parser: '%c' error END_TAG2", c));
        }
        break;
      case END_TAG:
        if (isalpha(c)) {
          INDIGO_TRACE(indigo_trace("XML Parser: '%c' END_TAG", c));
        } else if (c == '>') {
          handler = handler(END_TAG, NULL, NULL, property, driver, client);
          depth--;
          state = IDLE;
          INDIGO_TRACE(indigo_trace("XML Parser: '%c' END_TAG -> IDLE", c));
        } else {
          state = ERROR;
          INDIGO_TRACE(indigo_trace("XML Parser: '%c' error END_TAG", c));
        }
        break;
      case TEXT:
        if (c == '<') {
          if (depth == 2) {
            *value_pointer = 0;
            handler = handler(TEXT, NULL, value_buffer, property, driver, client);
          }
          state = TEXT1;
          INDIGO_TRACE(indigo_trace("XML Parser: '%c' %d TEXT -> TEXT1", c, depth));
          break;
        } else {
          if (depth == 2) {
            if (value_pointer - value_buffer < VALUE_SIZE) {
              *value_pointer++ = c;
            }
          }
          INDIGO_TRACE(indigo_trace("XML Parser: '%c' %d TEXT", c, depth));
        }
        break;
      case TEXT1:
        if (c=='/') {
          state = END_TAG;
          INDIGO_TRACE(indigo_trace("XML Parser: '%c' TEXT -> END_TAG", c));
        } else if (isalpha(c)) {
          name_pointer = name_buffer;
          *name_pointer++ = c;
          state = BEGIN_TAG;
          INDIGO_TRACE(indigo_trace("XML Parser: '%c' TEXT -> BEGIN_TAG", c));
        }
        break;
      case ATTRIBUTE_NAME1:
        if (isspace(c)) {
          INDIGO_TRACE(indigo_trace("XML Parser: '%c' ATTRIBUTE_NAME1", c));
        } else if (isalpha(c)) {
          name_pointer = name_buffer;
          *name_pointer++ = c;
          state = ATTRIBUTE_NAME;
          INDIGO_TRACE(indigo_trace("XML Parser: '%c' ATTRIBUTE_NAME1 -> ATTRIBUTE_NAME", c));
        } else if (c == '/') {
          state = END_TAG1;
        } else if (c == '>') {
          value_pointer = value_buffer;
          state = TEXT;
          INDIGO_TRACE(indigo_trace("XML Parser: '%c' ATTRIBUTE_NAME1 -> TEXT", c));
        }
       break;
      case ATTRIBUTE_NAME:
        if (name_pointer - name_buffer < NAME_SIZE && isalpha(c)) {
          *name_pointer++ = c;
          INDIGO_TRACE(indigo_trace("XML Parser: '%c' ATTRIBUTE_NAME", c));
        } else {
          *name_pointer = 0;
          if (c == '=') {
            state = ATTRIBUTE_VALUE1;
            INDIGO_TRACE(indigo_trace("XML Parser: '%c' ATTRIBUTE_NAME -> ATTRIBUTE_VALUE1", c));
          } else {
            INDIGO_TRACE(indigo_trace("XML Parser: '%c' ATTRIBUTE_NAME", c));
          }
        }
        break;
      case ATTRIBUTE_VALUE1:
        if (c == '"' || c == '\'') {
          q = c;
          value_pointer = value_buffer;
          state = ATTRIBUTE_VALUE;
          INDIGO_TRACE(indigo_trace("XML Parser: '%c' ATTRIBUTE_VALUE1 -> ATTRIBUTE_VALUE2", c));
        } else {
          INDIGO_TRACE(indigo_trace("XML Parser: '%c' ATTRIBUTE_VALUE1", c));
        }
        break;
      case ATTRIBUTE_VALUE:
        if (c == q) {
          *value_pointer = 0;
          state = ATTRIBUTE_NAME1;
          handler = handler(ATTRIBUTE_VALUE, name_buffer, value_buffer, property, driver, client);
          INDIGO_TRACE(indigo_trace("XML Parser: '%c' ATTRIBUTE_VALUE -> ATTRIBUTE_NAME1", c));
        } else {
          *value_pointer++ = c;
          INDIGO_TRACE(indigo_trace("XML Parser: '%c' ATTRIBUTE_VALUE", c));
        }
        break;
      default:
        break;
    }
  }
}

static pthread_mutex_t xmutex = PTHREAD_MUTEX_INITIALIZER;

static void xprintf(const char *format, ...) {
  char buffer[1024];
  va_list args;
  va_start(args, format);
  int length = vsnprintf(buffer, 1024, format, args);
  va_end(args);
  write(1, buffer, length);
}

static indigo_result xml_driver_parser_init(indigo_client *client) {
  return INDIGO_OK;
}

static indigo_result xml_driver_parser_start(indigo_client *client) {
  return INDIGO_OK;
}

static indigo_result xml_driver_parser_define_property(struct indigo_client *client, struct indigo_driver *driver, indigo_property *property) {
  pthread_mutex_lock(&xmutex);
  switch (property->type) {
    case INDIGO_TEXT_VECTOR:
      xprintf("<defTextVector device='%s' name='%s' group='%s' label='%s' perm='%s' state='%s'>\n", property->device, property->name, property->group, property->label, indigo_property_perm_text[property->perm], indigo_property_state_text[property->state]);
      for (int i = 0; i < property->count; i++) {
        indigo_item *item = &property->items[i];
        xprintf("<defText name='%s' label='%s'>%s</defText>\n", item->name, item->label, item->text_value);
      }
      xprintf("</defTextVector>\n");
      break;
    case INDIGO_NUMBER_VECTOR:
      xprintf("<defNumberVector device='%s' name='%s' group='%s' label='%s' perm='%s' state='%s'>\n", property->device, property->name, property->group, property->label, indigo_property_perm_text[property->perm], indigo_property_state_text[property->state]);
      for (int i = 0; i < property->count; i++) {
        indigo_item *item = &property->items[i];
        xprintf("<defNumber name='%s' label='%s' min='%g' max='%g' step='%g'>%g</defNumber>\n", item->name, item->label, item->number_min, item->number_max, item->number_step, item->number_value);
      }
      xprintf("</defNumberVector>\n");
      break;
    case INDIGO_SWITCH_VECTOR:
      xprintf("<defSwitchVector device='%s' name='%s' group='%s' label='%s' perm='%s' state='%s' rule='%s'>\n", property->device, property->name, property->group, property->label, indigo_property_perm_text[property->perm], indigo_property_state_text[property->state], indigo_switch_rule_text[property->rule]);
      for (int i = 0; i < property->count; i++) {
        indigo_item *item = &property->items[i];
        xprintf("<defSwitch name='%s' label='%s'>%s</defSwitch>\n", item->name, item->label, item->switch_value ? "On" : "Off");
      }
      xprintf("</defSwitchVector>\n");
      break;
    case INDIGO_LIGHT_VECTOR:
      xprintf("<defLightVector device='%s' name='%s' group='%s' label='%s' perm='%s' state='%s'>\n", property->device, property->name, property->group, property->label, indigo_property_perm_text[property->perm], indigo_property_state_text[property->state]);
      for (int i = 0; i < property->count; i++) {
        indigo_item *item = &property->items[i];
        xprintf(" <defLight name='%s' label='%s'>%s</defLight>\n", item->name, item->label, indigo_property_state_text[item->light_value]);
      }
      xprintf("</defLightVector>\n");
      break;
    case INDIGO_BLOB_VECTOR:
      xprintf("<defBLOBVector device='%s' name='%s' group='%s' label='%s' perm='%s' state='%s'>\n", property->device, property->name, property->group, property->label, indigo_property_perm_text[property->perm], indigo_property_state_text[property->state]);
      for (int i = 0; i < property->count; i++) {
        indigo_item *item = &property->items[i];
        xprintf("<defBLOB name='%s' label='%s'/>\n", item->name, item->label);
      }
      xprintf("</defBLOBtVector>\n");
      break;
  }
  pthread_mutex_unlock(&xmutex);
  return INDIGO_OK;
}

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

static indigo_result xml_driver_parser_update_property(struct indigo_client *client, struct indigo_driver *driver, indigo_property *property) {
  pthread_mutex_lock(&xmutex);
  switch (property->type) {
    case INDIGO_TEXT_VECTOR:
      xprintf("<setTextVector device='%s' name='%s' state='%s'>\n", property->device, property->name, indigo_property_state_text[property->state]);
      for (int i = 0; i < property->count; i++) {
        indigo_item *item = &property->items[i];
        xprintf("<oneText name='%s'>%s</oneText>\n", item->name, item->text_value);
      }
      xprintf("</setTextVector>\n");
      break;
    case INDIGO_NUMBER_VECTOR:
      xprintf("<setNumberVector device='%s' name='%s' state='%s'>\n", property->device, property->name, indigo_property_state_text[property->state]);
      for (int i = 0; i < property->count; i++) {
        indigo_item *item = &property->items[i];
        xprintf("<oneNumber name='%s'>%g</oneNumber>\n", item->name, item->number_value);
      }
      xprintf("</setNumberVector>\n");
      break;
    case INDIGO_SWITCH_VECTOR:
      xprintf("<setSwitchVector device='%s' name='%s' state='%s'>\n", property->device, property->name, indigo_property_state_text[property->state]);
      for (int i = 0; i < property->count; i++) {
        indigo_item *item = &property->items[i];
        xprintf("<oneSwitch name='%s'>%s</oneSwitch>\n", item->name, item->switch_value ? "On" : "Off");
      }
      xprintf("</setSwitchVector>\n");
      break;
    case INDIGO_LIGHT_VECTOR:
      xprintf("<setLightVector device='%s' name='%s' state='%s'>\n", property->device, property->name, indigo_property_state_text[property->state]);
      for (int i = 0; i < property->count; i++) {
        indigo_item *item = &property->items[i];
        xprintf("<oneLight name='%s'>%s</oneLight>\n", item->name, indigo_property_state_text[item->light_value]);
      }
      xprintf("</setLightVector>\n");
      break;
    case INDIGO_BLOB_VECTOR:
      xprintf("<setBLOBVector device='%s' name='%s' group='%s' label='%s' perm='%s' state='%s'>\n", property->device, property->name, property->group, property->label, indigo_property_perm_text[property->perm], indigo_property_state_text[property->state]);
      if (property->state == INDIGO_OK_STATE) {
        for (int i = 0; i < property->count; i++) {
          indigo_item *item = &property->items[i];
          long input_length = item->blob_size;
          long output_length = 4 * ((input_length + 2) / 3);
          unsigned char *data = item->blob_value;
          char encoded_data[74];
          xprintf("<oneBLOB name='%s' format='%s' size='%ld'>\n", item->name, item->blob_format, output_length);
          int j = 0;
          int i = 0;
          while (i < input_length) {
            uint32_t octet_a = i < input_length ? (unsigned char)data[i++] : 0;
            uint32_t octet_b = i < input_length ? (unsigned char)data[i++] : 0;
            uint32_t octet_c = i < input_length ? (unsigned char)data[i++] : 0;
            uint32_t triple = (octet_a << 0x10) + (octet_b << 0x08) + octet_c;
            encoded_data[j++] = encoding_table[(triple >> 3 * 6) & 0x3F];
            encoded_data[j++] = encoding_table[(triple >> 2 * 6) & 0x3F];
            encoded_data[j++] = encoding_table[(triple >> 1 * 6) & 0x3F];
            encoded_data[j++] = encoding_table[(triple >> 0 * 6) & 0x3F];
            if (j == 72) {
              encoded_data[j++] = '\n';
              encoded_data[j++] = 0;
              xprintf(encoded_data);
              j = 0;
            }
          }
          for (int i = 0; i < mod_table[input_length % 3]; i++) {
            encoded_data[j++] = '=';
          }
          encoded_data[j++] = '\n';
          encoded_data[j++] = 0;
          xprintf(encoded_data);
          xprintf("</oneBLOB>\n");
        }
      }
      xprintf("</setBLOBtVector>\n");
      break;
  }
  pthread_mutex_unlock(&xmutex);
  return INDIGO_OK;
}

static indigo_result xml_driver_parser_delete_property(struct indigo_client *client, struct indigo_driver *driver, indigo_property *property) {
  pthread_mutex_lock(&xmutex);
  xprintf("<delProperty device='%s' name='%s'>\n", property->device, property->name);
  pthread_mutex_unlock(&xmutex);
  return INDIGO_OK;
}

static indigo_result xml_driver_parser_stop(indigo_client *client) {
  return INDIGO_OK;
}

indigo_client *xml_driver_parser() {
  static indigo_client test = {
    xml_driver_parser_init,
    xml_driver_parser_start,
    xml_driver_parser_define_property,
    xml_driver_parser_update_property,
    xml_driver_parser_delete_property,
    xml_driver_parser_stop
  };
  return &test;
}
