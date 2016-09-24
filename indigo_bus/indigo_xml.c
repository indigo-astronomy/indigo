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

static indigo_property_state parse_state(char *value) {
  if (!strcmp(value, "Ok"))
    return INDIGO_OK_STATE;
  if (!strcmp(value, "Busy"))
    return INDIGO_BUSY_STATE;
  if (!strcmp(value, "Alert"))
    return INDIGO_ALERT_STATE;
  return INDIGO_IDLE_STATE;
}

static indigo_property_perm parse_perm(char *value) {
  if (!strcmp(value, "rw"))
    return INDIGO_RW_PERM;
  if (!strcmp(value, "wo"))
    return INDIGO_WO_PERM;
  return INDIGO_RO_PERM;
}

static indigo_rule parse_rule(char *value) {
  if (!strcmp(value, "OneOfMany"))
    return INDIGO_ONE_OF_MANY_RULE;
  if (!strcmp(value, "AtMostOne"))
    return INDIGO_AT_MOST_ONE_RULE;
  return INDIGO_ANY_OF_MANY_RULE;
}

typedef void *(* parser_handler)(parser_state state, char *name, char *value, indigo_property *property, indigo_driver *driver, indigo_client *client);

void *top_level_handler(parser_state state, char *name, char *value, indigo_property *property, indigo_driver *driver, indigo_client *client);
void *new_text_vector_handler(parser_state state, char *name, char *value, indigo_property *property, indigo_driver *driver, indigo_client *client);
void *new_number_vector_handler(parser_state state, char *name, char *value, indigo_property *property, indigo_driver *driver, indigo_client *client);
void *new_switch_vector_handler(parser_state state, char *name, char *value, indigo_property *property, indigo_driver *driver, indigo_client *client);
void *def_text_vector_handler(parser_state state, char *name, char *value, indigo_property *property, indigo_driver *driver, indigo_client *client);
void *def_number_vector_handler(parser_state state, char *name, char *value, indigo_property *property, indigo_driver *driver, indigo_client *client);
void *def_switch_vector_handler(parser_state state, char *name, char *value, indigo_property *property, indigo_driver *driver, indigo_client *client);
void *def_light_vector_handler(parser_state state, char *name, char *value, indigo_property *property, indigo_driver *driver, indigo_client *client);
void *def_blob_vector_handler(parser_state state, char *name, char *value, indigo_property *property, indigo_driver *driver, indigo_client *client);
void *set_text_vector_handler(parser_state state, char *name, char *value, indigo_property *property, indigo_driver *driver, indigo_client *client);
void *set_number_vector_handler(parser_state state, char *name, char *value, indigo_property *property, indigo_driver *driver, indigo_client *client);
void *set_switch_vector_handler(parser_state state, char *name, char *value, indigo_property *property, indigo_driver *driver, indigo_client *client);
void *set_light_vector_handler(parser_state state, char *name, char *value, indigo_property *property, indigo_driver *driver, indigo_client *client);
void *set_blob_vector_handler(parser_state state, char *name, char *value, indigo_property *property, indigo_driver *driver, indigo_client *client);

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
      property->state = parse_state(value);
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
      property->state = parse_state(value);
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
      property->state = parse_state(value);
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

void *set_one_text_vector_handler(parser_state state, char *name, char *value, indigo_property *property, indigo_driver *driver, indigo_client *client) {
  if (state == ATTRIBUTE_VALUE) {
    if (!strcmp(name, "name")) {
      strncpy(property->items[property->count-1].name, value, NAME_SIZE);
      return set_one_text_vector_handler;
    }
  } else if (state == TEXT) {
    strncat(property->items[property->count-1].text_value, value, VALUE_SIZE);
    return set_one_text_vector_handler;
  }
  return set_text_vector_handler;
}

void *set_text_vector_handler(parser_state state, char *name, char *value, indigo_property *property, indigo_driver *driver, indigo_client *client) {
  if (state == BEGIN_TAG) {
    if (!strcmp(name, "oneText")) {
      property->count++;
      return set_one_text_vector_handler;
    }
  } else if (state == ATTRIBUTE_VALUE) {
    if (!strcmp(name, "device")) {
      strncpy(property->device, value, NAME_SIZE);
    } else if (!strcmp(name, "name")) {
      strncpy(property->name, value, NAME_SIZE);
    } else if (!strcmp(name, "state")) {
      property->state = parse_state(value);
    }
    return set_text_vector_handler;
  } else if (state == TEXT) {
    return set_text_vector_handler;
  } else if (state == END_TAG) {
    property->type = INDIGO_TEXT_VECTOR;
    indigo_update_property(driver, property);
    memset(property, 0, PROPERTY_SIZE);
  }
  return top_level_handler;
}

void *set_one_number_vector_handler(parser_state state, char *name, char *value, indigo_property *property, indigo_driver *driver, indigo_client *client) {
  if (state == ATTRIBUTE_VALUE) {
    if (!strcmp(name, "name")) {
      strncpy(property->items[property->count-1].name, value, NAME_SIZE);
      return set_one_number_vector_handler;
    }
  } else if (state == TEXT) {
    property->items[property->count-1].number_value = atof(value);
    return set_one_number_vector_handler;
  }
  return set_number_vector_handler;
}

void *set_number_vector_handler(parser_state state, char *name, char *value, indigo_property *property, indigo_driver *driver, indigo_client *client) {
  if (state == BEGIN_TAG) {
    if (!strcmp(name, "oneNumber")) {
      property->count++;
      return set_one_number_vector_handler;
    }
  } else if (state == ATTRIBUTE_VALUE) {
    if (!strcmp(name, "device")) {
      strncpy(property->device, value, NAME_SIZE);
    } else if (!strcmp(name, "name")) {
      strncpy(property->name, value, NAME_SIZE);
    } else if (!strcmp(name, "state")) {
      property->state = parse_state(value);
    }
    return set_number_vector_handler;
  } else if (state == TEXT) {
    return set_number_vector_handler;
  } else if (state == END_TAG) {
    property->type = INDIGO_NUMBER_VECTOR;
    indigo_update_property(driver, property);
    memset(property, 0, PROPERTY_SIZE);
  }
  return top_level_handler;
}

void *set_one_switch_vector_handler(parser_state state, char *name, char *value, indigo_property *property, indigo_driver *driver, indigo_client *client) {
  if (state == ATTRIBUTE_VALUE) {
    if (!strcmp(name, "name")) {
      strncpy(property->items[property->count-1].name, value, NAME_SIZE);
      return set_one_switch_vector_handler;
    }
  } else if (state == TEXT) {
    property->items[property->count-1].switch_value = !strcmp(value, "On");
    return set_one_switch_vector_handler;
  }
  return set_switch_vector_handler;
}

void *set_switch_vector_handler(parser_state state, char *name, char *value, indigo_property *property, indigo_driver *driver, indigo_client *client) {
  if (state == BEGIN_TAG) {
    if (!strcmp(name, "oneSwitch")) {
      property->count++;
      return set_one_switch_vector_handler;
    }
  } else if (state == ATTRIBUTE_VALUE) {
    if (!strcmp(name, "device")) {
      strncpy(property->device, value, NAME_SIZE);
    } else if (!strcmp(name, "name")) {
      strncpy(property->name, value, NAME_SIZE);
    } else if (!strcmp(name, "state")) {
      property->state = parse_state(value);
    }
    return set_switch_vector_handler;
  } else if (state == TEXT) {
    return set_switch_vector_handler;
  } else if (state == END_TAG) {
    property->type = INDIGO_SWITCH_VECTOR;
    indigo_update_property(driver, property);
    memset(property, 0, PROPERTY_SIZE);
  }
  return top_level_handler;
}

void *set_one_light_vector_handler(parser_state state, char *name, char *value, indigo_property *property, indigo_driver *driver, indigo_client *client) {
  if (state == ATTRIBUTE_VALUE) {
    if (!strcmp(name, "name")) {
      strncpy(property->items[property->count-1].name, value, NAME_SIZE);
      return set_one_light_vector_handler;
    }
  } else if (state == TEXT) {
    property->items[property->count-1].light_value = parse_state(value);
    return set_one_light_vector_handler;
  }
  return set_light_vector_handler;
}

void *set_light_vector_handler(parser_state state, char *name, char *value, indigo_property *property, indigo_driver *driver, indigo_client *client) {
  if (state == BEGIN_TAG) {
    if (!strcmp(name, "oneLight")) {
      property->count++;
      return set_one_light_vector_handler;
    }
  } else if (state == ATTRIBUTE_VALUE) {
    if (!strcmp(name, "device")) {
      strncpy(property->device, value, NAME_SIZE);
    } else if (!strcmp(name, "name")) {
      strncpy(property->name, value, NAME_SIZE);
    } else if (!strcmp(name, "state")) {
      property->state = parse_state(value);
    }
    return set_light_vector_handler;
  } else if (state == TEXT) {
    return set_light_vector_handler;
  } else if (state == END_TAG) {
    property->type = INDIGO_LIGHT_VECTOR;
    indigo_update_property(driver, property);
    memset(property, 0, PROPERTY_SIZE);
  }
  return top_level_handler;
}

void *set_one_blob_vector_handler(parser_state state, char *name, char *value, indigo_property *property, indigo_driver *driver, indigo_client *client) {
  if (state == ATTRIBUTE_VALUE) {
    if (!strcmp(name, "name")) {
      strncpy(property->items[property->count-1].name, value, NAME_SIZE);
      return set_one_blob_vector_handler;
    }
    if (!strcmp(name, "format")) {
      strncpy(property->items[property->count-1].blob_format, value, NAME_SIZE);
      return set_one_blob_vector_handler;
    }
    if (!strcmp(name, "size")) {
      property->items[property->count-1].blob_size = atol(value);
      return set_one_blob_vector_handler;
    }
  } else if (state == TEXT) {
    
    // TBD
    
    return set_one_blob_vector_handler;
  }
  return set_blob_vector_handler;
}

void *set_blob_vector_handler(parser_state state, char *name, char *value, indigo_property *property, indigo_driver *driver, indigo_client *client) {
  if (state == BEGIN_TAG) {
    if (!strcmp(name, "oneBLOB")) {
      property->count++;
      return set_one_blob_vector_handler;
    }
  } else if (state == ATTRIBUTE_VALUE) {
    if (!strcmp(name, "device")) {
      strncpy(property->device, value, NAME_SIZE);
    } else if (!strcmp(name, "name")) {
      strncpy(property->name, value, NAME_SIZE);
    } else if (!strcmp(name, "state")) {
      property->state = parse_state(value);
    }
    return set_blob_vector_handler;
  } else if (state == TEXT) {
    return set_blob_vector_handler;
  } else if (state == END_TAG) {
    property->type = INDIGO_BLOB_VECTOR;
    indigo_update_property(driver, property);
    memset(property, 0, PROPERTY_SIZE);
  }
  return top_level_handler;
}

void *def_text_handler(parser_state state, char *name, char *value, indigo_property *property, indigo_driver *driver, indigo_client *client) {
  if (state == ATTRIBUTE_VALUE) {
    if (!strcmp(name, "name")) {
      strncpy(property->items[property->count-1].name, value, NAME_SIZE);
      return def_text_handler;
    }
    if (!strcmp(name, "label")) {
      strncpy(property->items[property->count-1].label, value, NAME_SIZE);
      return def_text_handler;
    }
  } else if (state == TEXT) {
    strncat(property->items[property->count-1].text_value, value, VALUE_SIZE);
    return def_text_handler;
  }
  return def_text_vector_handler;
}

void *def_text_vector_handler(parser_state state, char *name, char *value, indigo_property *property, indigo_driver *driver, indigo_client *client) {
  if (state == BEGIN_TAG) {
    if (!strcmp(name, "defText")) {
      property->count++;
      return def_text_handler;
    }
  } else if (state == ATTRIBUTE_VALUE) {
    if (!strcmp(name, "device")) {
      strncpy(property->device, value, NAME_SIZE);
    } else if (!strcmp(name, "name")) {
      strncpy(property->name, value, NAME_SIZE);
    } else if (!strcmp(name, "group")) {
      strncpy(property->group, value, NAME_SIZE);
    } else if (!strcmp(name, "label")) {
      strncpy(property->label, value, NAME_SIZE);
    } else if (!strcmp(name, "state")) {
      property->state = parse_state(value);
    } else if (!strcmp(name, "perm")) {
      property->perm = parse_perm(value);
    }
    return def_text_vector_handler;
  } else if (state == TEXT) {
    return def_text_vector_handler;
  } else if (state == END_TAG) {
    property->type = INDIGO_TEXT_VECTOR;
    indigo_define_property(driver, property);
    memset(property, 0, PROPERTY_SIZE);
  }
  return top_level_handler;
}

void *def_number_handler(parser_state state, char *name, char *value, indigo_property *property, indigo_driver *driver, indigo_client *client) {
  if (state == ATTRIBUTE_VALUE) {
    if (!strcmp(name, "name")) {
      strncpy(property->items[property->count-1].name, value, NAME_SIZE);
      return def_number_handler;
    }
    if (!strcmp(name, "label")) {
      strncpy(property->items[property->count-1].label, value, NAME_SIZE);
      return def_number_handler;
    }
    if (!strcmp(name, "min")) {
      property->items[property->count-1].number_min = atof(value);
      return def_number_handler;
    }
    if (!strcmp(name, "max")) {
      property->items[property->count-1].number_max = atof(value);
      return def_number_handler;
    }
    if (!strcmp(name, "step")) {
      property->items[property->count-1].number_step = atof(value);
      return def_number_handler;
    }
  } else if (state == TEXT) {
    property->items[property->count-1].number_value = atof(value);
    return def_number_handler;
  }
  return def_number_vector_handler;
}

void *def_number_vector_handler(parser_state state, char *name, char *value, indigo_property *property, indigo_driver *driver, indigo_client *client) {
  if (state == BEGIN_TAG) {
    if (!strcmp(name, "defNumber")) {
      property->count++;
      return def_number_handler;
    }
  } else if (state == ATTRIBUTE_VALUE) {
    if (!strcmp(name, "device")) {
      strncpy(property->device, value, NAME_SIZE);
    } else if (!strcmp(name, "name")) {
      strncpy(property->name, value, NAME_SIZE);
    } else if (!strcmp(name, "group")) {
      strncpy(property->group, value, NAME_SIZE);
    } else if (!strcmp(name, "label")) {
      strncpy(property->label, value, NAME_SIZE);
    } else if (!strcmp(name, "state")) {
      property->state = parse_state(value);
    } else if (!strcmp(name, "perm")) {
      property->perm = parse_perm(value);
    }
    return def_number_vector_handler;
  } else if (state == TEXT) {
    return def_number_vector_handler;
  } else if (state == END_TAG) {
    property->type = INDIGO_NUMBER_VECTOR;
    indigo_define_property(driver, property);
    memset(property, 0, PROPERTY_SIZE);
  }
  return top_level_handler;
}

void *def_switch_handler(parser_state state, char *name, char *value, indigo_property *property, indigo_driver *driver, indigo_client *client) {
  if (state == ATTRIBUTE_VALUE) {
    if (!strcmp(name, "name")) {
      strncpy(property->items[property->count-1].name, value, NAME_SIZE);
      return def_switch_handler;
    }
    if (!strcmp(name, "label")) {
      strncpy(property->items[property->count-1].label, value, NAME_SIZE);
      return def_switch_handler;
    }
  } else if (state == TEXT) {
    property->items[property->count-1].switch_value = !strcmp(value, "On");
    return def_switch_handler;
  }
  return def_switch_vector_handler;
}

void *def_switch_vector_handler(parser_state state, char *name, char *value, indigo_property *property, indigo_driver *driver, indigo_client *client) {
  if (state == BEGIN_TAG) {
    if (!strcmp(name, "defSwitch")) {
      property->count++;
      return def_switch_handler;
    }
  } else if (state == ATTRIBUTE_VALUE) {
    if (!strcmp(name, "device")) {
      strncpy(property->device, value, NAME_SIZE);
    } else if (!strcmp(name, "name")) {
      strncpy(property->name, value, NAME_SIZE);
    } else if (!strcmp(name, "group")) {
      strncpy(property->group, value, NAME_SIZE);
    } else if (!strcmp(name, "label")) {
      strncpy(property->label, value, NAME_SIZE);
    } else if (!strcmp(name, "state")) {
      property->state = parse_state(value);
    } else if (!strcmp(name, "perm")) {
      property->perm = parse_perm(value);
    } else if (!strcmp(name, "rule")) {
      property->rule = parse_rule(value);
    }
    return def_switch_vector_handler;
  } else if (state == TEXT) {
    return def_switch_vector_handler;
  } else if (state == END_TAG) {
    property->type = INDIGO_SWITCH_VECTOR;
    indigo_define_property(driver, property);
    memset(property, 0, PROPERTY_SIZE);
  }
  return top_level_handler;
}

void *def_light_handler(parser_state state, char *name, char *value, indigo_property *property, indigo_driver *driver, indigo_client *client) {
  if (state == ATTRIBUTE_VALUE) {
    if (!strcmp(name, "name")) {
      strncpy(property->items[property->count-1].name, value, NAME_SIZE);
      return def_light_handler;
    }
    if (!strcmp(name, "label")) {
      strncpy(property->items[property->count-1].label, value, NAME_SIZE);
      return def_light_handler;
    }
  } else if (state == TEXT) {
    property->items[property->count-1].light_value = parse_state(value);
    return def_light_handler;
  }
  return def_light_vector_handler;
}

void *def_light_vector_handler(parser_state state, char *name, char *value, indigo_property *property, indigo_driver *driver, indigo_client *client) {
  if (state == BEGIN_TAG) {
    if (!strcmp(name, "defLight")) {
      property->count++;
      return def_light_handler;
    }
  } else if (state == ATTRIBUTE_VALUE) {
    if (!strcmp(name, "device")) {
      strncpy(property->device, value, NAME_SIZE);
    } else if (!strcmp(name, "name")) {
      strncpy(property->name, value, NAME_SIZE);
    } else if (!strcmp(name, "group")) {
      strncpy(property->group, value, NAME_SIZE);
    } else if (!strcmp(name, "label")) {
      strncpy(property->label, value, NAME_SIZE);
    } else if (!strcmp(name, "state")) {
      property->state = parse_state(value);
    }
    return def_light_vector_handler;
  } else if (state == TEXT) {
    return def_light_vector_handler;
  } else if (state == END_TAG) {
    property->type = INDIGO_LIGHT_VECTOR;
    indigo_define_property(driver, property);
    memset(property, 0, PROPERTY_SIZE);
  }
  return top_level_handler;
}

void *def_blob_handler(parser_state state, char *name, char *value, indigo_property *property, indigo_driver *driver, indigo_client *client) {
  if (state == ATTRIBUTE_VALUE) {
    if (!strcmp(name, "name")) {
      strncpy(property->items[property->count-1].name, value, NAME_SIZE);
      return def_blob_handler;
    }
    if (!strcmp(name, "label")) {
      strncpy(property->items[property->count-1].label, value, NAME_SIZE);
      return def_blob_handler;
    }
  } else if (state == TEXT) {
    return def_blob_handler;
  }
  return def_blob_vector_handler;
}

void *def_blob_vector_handler(parser_state state, char *name, char *value, indigo_property *property, indigo_driver *driver, indigo_client *client) {
  if (state == BEGIN_TAG) {
    if (!strcmp(name, "defBLOB")) {
      property->count++;
      return def_blob_handler;
    }
  } else if (state == ATTRIBUTE_VALUE) {
    if (!strcmp(name, "device")) {
      strncpy(property->device, value, NAME_SIZE);
    } else if (!strcmp(name, "name")) {
      strncpy(property->name, value, NAME_SIZE);
    } else if (!strcmp(name, "group")) {
      strncpy(property->group, value, NAME_SIZE);
    } else if (!strcmp(name, "label")) {
      strncpy(property->label, value, NAME_SIZE);
    } else if (!strcmp(name, "state")) {
      property->state = parse_state(value);
    }
    return def_blob_vector_handler;
  } else if (state == TEXT) {
    return def_blob_vector_handler;
  } else if (state == END_TAG) {
    property->type = INDIGO_BLOB_VECTOR;
    indigo_define_property(driver, property);
    memset(property, 0, PROPERTY_SIZE);
  }
  return top_level_handler;
}

void *del_property_handler(parser_state state, char *name, char *value, indigo_property *property, indigo_driver *driver, indigo_client *client) {
  if (state == ATTRIBUTE_VALUE) {
    if (!strncmp(name, "device", NAME_SIZE)) {
      strcpy(property->device, value);
    } else if (!strncmp(name, "name", NAME_SIZE)) {
      strcpy(property->name, value);
    }
    return del_property_handler;
  } else if (state == TEXT) {
    return del_property_handler;
  } else if (state == END_TAG) {
    indigo_delete_property(driver, property);
    memset(property, 0, PROPERTY_SIZE);
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
    if (!strcmp(name, "setTextVector"))
      return set_text_vector_handler;
    if (!strcmp(name, "setNumberVector"))
      return set_number_vector_handler;
    if (!strcmp(name, "setSwitchVector"))
      return set_switch_vector_handler;
    if (!strcmp(name, "setLightVector"))
      return set_light_vector_handler;
    if (!strcmp(name, "setBLOBVector"))
      return set_blob_vector_handler;
    if (!strcmp(name, "defTextVector"))
      return def_text_vector_handler;
    if (!strcmp(name, "defNumberVector"))
      return def_number_vector_handler;
    if (!strcmp(name, "defSwitchVector"))
      return def_switch_vector_handler;
    if (!strcmp(name, "defLightVector"))
      return def_light_vector_handler;
    if (!strcmp(name, "defBLOBVector"))
      return def_blob_vector_handler;
    if (!strcmp(name, "delProperty"))
      return del_property_handler;
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

//int main() {
//  indigo_xml_parse(0, NULL, NULL);
//}
