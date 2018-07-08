// Copyright (c) 2016 CloudMakers, s. r. o.
// All rights reserved.
//
// You can use this software under the terms of 'INDIGO Astronomy
// open-source license' (see LICENSE.md).
//
// THIS SOFTWARE IS PROVIDED BY THE AUTHORS 'AS IS' AND ANY EXPRESS
// OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
// WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY
// DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
// DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
// GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
// WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
// NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
// SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

// version history
// 2.0 by Peter Polakovic <peter.polakovic@cloudmakers.eu>

/** INDIGO XML wire protocol parser
 \file indigo_xml.c
 */

#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <ctype.h>
#include <assert.h>
#include <pthread.h>
#include <math.h>
#include <fcntl.h>

#include "indigo_base64.h"
#include "indigo_xml.h"
#include "indigo_io.h"
#include "indigo_version.h"
#include "indigo_driver_xml.h"

#define BUFFER_SIZE 524288  /* BUFFER_SIZE % 4 == 0, inportant for base64 */

#define PROPERTY_SIZE sizeof(indigo_property)+INDIGO_MAX_ITEMS*(sizeof(indigo_item))

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
	BLOB,
	BLOB_END,
	END_TAG1,
	END_TAG2,
	END_TAG,
	HEADER,
	HEADER1
} parser_state;


static char *parser_state_name[] = {
	"ERROR",
	"IDLE",
	"BEGIN_TAG1",
	"BEGIN_TAG",
	"ATTRIBUTE_NAME1",
	"ATTRIBUTE_NAME",
	"ATTRIBUTE_VALUE1",
	"ATTRIBUTE_VALUE",
	"TEXT1",
	"TEXT",
	"BLOB",
	"BLOB_END",
	"END_TAG1",
	"END_TAG2",
	"END_TAG",
	"HEADER",
	"HEADER1"
};

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

typedef struct {
	char property_buffer[PROPERTY_SIZE];
	indigo_device *device;
	indigo_client *client;
	int count;
	indigo_property **properties;
} parser_context;

bool indigo_use_blob_urls = true;

typedef void *(* parser_handler)(parser_state state, parser_context *context, char *name, char *value, char *message);

static void *top_level_handler(parser_state state, parser_context *context, char *name, char *value, char *message);
static void *new_text_vector_handler(parser_state state, parser_context *context, char *name, char *value, char *message);
static void *new_number_vector_handler(parser_state state, parser_context *context, char *name, char *value, char *message);
static void *new_switch_vector_handler(parser_state state, parser_context *context, char *name, char *value, char *message);
static void *def_text_vector_handler(parser_state state, parser_context *context, char *name, char *value, char *message);
static void *def_number_vector_handler(parser_state state, parser_context *context, char *name, char *value, char *message);
static void *def_switch_vector_handler(parser_state state, parser_context *context, char *name, char *value, char *message);
static void *def_light_vector_handler(parser_state state, parser_context *context, char *name, char *value, char *message);
static void *def_blob_vector_handler(parser_state state, parser_context *context, char *name, char *value, char *message);
static void *set_text_vector_handler(parser_state state, parser_context *context, char *name, char *value, char *message);
static void *set_number_vector_handler(parser_state state, parser_context *context, char *name, char *value, char *message);
static void *set_switch_vector_handler(parser_state state, parser_context *context, char *name, char *value, char *message);
static void *set_light_vector_handler(parser_state state, parser_context *context, char *name, char *value, char *message);
static void *set_blob_vector_handler(parser_state state, parser_context *context, char *name, char *value, char *message);

static void *enable_blob_handler(parser_state state, parser_context *context, char *name, char *value, char *message) {
	indigo_property *property = (indigo_property *)context->property_buffer;
	indigo_client *client = context->client;
	assert(client != NULL);
	INDIGO_TRACE_PARSER(indigo_trace("XML Parser: enable_blob_handler %s '%s' '%s'", parser_state_name[state], name != NULL ? name : "", value != NULL ? value : ""));
	if (state == ATTRIBUTE_VALUE) {
		if (!strcmp(name, "device")) {
			strncpy(property->device, value,INDIGO_NAME_SIZE);
		} else if (!strcmp(name, "name")) {
			indigo_copy_property_name(client ? client->version : INDIGO_VERSION_CURRENT, property, value);
		}
	} else if (state == TEXT) {
		indigo_enable_blob_mode_record *record = client->enable_blob_mode_records;
		indigo_enable_blob_mode_record *prev = NULL;
		while (record) {
			if (!strcmp(property->device, record->device) && (*record->name == 0 || !strcmp(property->name, record->name))) {
				if (prev) {
					prev->next = record->next;
					free(record);
					record = prev->next;
				} else {
					client->enable_blob_mode_records = record->next;
					free(record);
					record = client->enable_blob_mode_records;
				}
			} else {
				prev = record;
				record = record->next;
			}
		}
		if (strcmp(value, "Never")) {
			record = malloc(sizeof(indigo_enable_blob_mode_record));
			strncpy(record->device, property->device, INDIGO_NAME_SIZE);
			strncpy(record->name, property->name, INDIGO_NAME_SIZE);
			if (!strcmp(value, "URL"))
				record->mode = INDIGO_ENABLE_BLOB_URL;
			else
				record->mode = INDIGO_ENABLE_BLOB_ALSO;
			record->next = client->enable_blob_mode_records;
			client->enable_blob_mode_records = record;
			indigo_enable_blob(client, property, record->mode);
		} else {
			indigo_enable_blob(client, property, INDIGO_ENABLE_BLOB_NEVER);
		}		
		INDIGO_DEBUG(indigo_debug("enableBLOB device='%s' name='%s' mode='%s'", property->device, property->name, value));
		INDIGO_DEBUG(record = client->enable_blob_mode_records; while (record) { indigo_debug("   %s %s %d", record->device, record->name, record->mode); record = record->next; });
	} else if (state == END_TAG) {
		memset(property, 0, PROPERTY_SIZE);
		return top_level_handler;
	}
	return enable_blob_handler;
}

static void *get_properties_handler(parser_state state, parser_context *context, char *name, char *value, char *message) {
	indigo_property *property = (indigo_property *)context->property_buffer;
	indigo_client *client = context->client;
	assert(client != NULL);
	INDIGO_TRACE_PARSER(indigo_trace("XML Parser: get_properties_handler %s '%s' '%s'", parser_state_name[state], name != NULL ? name : "", value != NULL ? value : ""));
	if (state == ATTRIBUTE_VALUE) {
		if (!strcmp(name, "version")) {
			indigo_version version = INDIGO_VERSION_LEGACY;
			if (!strncmp(value, "1.", 2))
				version = INDIGO_VERSION_LEGACY;
			else if (!strcmp(value, "2.0"))
				version = INDIGO_VERSION_2_0;
			client->version = version;
		} else if (!strcmp(name, "switch")) {
			indigo_version version = INDIGO_VERSION_LEGACY;
			if (!strncmp(value, "1.", 2))
				version = INDIGO_VERSION_LEGACY;
			else if (!strcmp(value, "2.0"))
				version = INDIGO_VERSION_2_0;
			if (version > client->version) {
				assert(client->client_context != NULL);
				int handle = ((indigo_adapter_context *)(client->client_context))->output;
				indigo_printf(handle, "<switchProtocol version='%d.%d'/>\n", (version >> 8) & 0xFF, version & 0xFF);
				client->version = version;
			}
		} else if (!strncmp(name, "device",INDIGO_NAME_SIZE)) {
			strncpy(property->device, value, INDIGO_NAME_SIZE);
		} else if (!strncmp(name, "name",INDIGO_NAME_SIZE)) {
			indigo_copy_property_name(client->version, property, value);;
		}
	} else if (state == END_TAG) {
		indigo_enumerate_properties(client, property);
		memset(property, 0, PROPERTY_SIZE);
		return top_level_handler;
	}
	return get_properties_handler;
}

static void *new_one_text_vector_handler(parser_state state, parser_context *context, char *name, char *value, char *message) {
	indigo_property *property = (indigo_property *)context->property_buffer;
	indigo_client *client = context->client;
	INDIGO_TRACE_PARSER(indigo_trace("XML Parser: new_one_text_vector_handler %s '%s' '%s'", parser_state_name[state], name != NULL ? name : "", value != NULL ? value : ""));
	if (state == ATTRIBUTE_VALUE) {
		if (!strcmp(name, "name")) {
			indigo_copy_item_name(client ? client->version : INDIGO_VERSION_CURRENT, property, property->items+property->count-1, value);
		}
	} else if (state == TEXT) {
		strncat(property->items[property->count-1].text.value, value, INDIGO_VALUE_SIZE-1);
	} else if (state == END_TAG) {
		return new_text_vector_handler;
	}
	return new_one_text_vector_handler;
}

static void *new_text_vector_handler(parser_state state, parser_context *context, char *name, char *value, char *message) {
	indigo_property *property = (indigo_property *)context->property_buffer;
	indigo_client *client = context->client;
	INDIGO_TRACE_PARSER(indigo_trace("XML Parser: new_text_vector_handler %s '%s' '%s'", parser_state_name[state], name != NULL ? name : "", value != NULL ? value : ""));
	if (state == BEGIN_TAG) {
		if (!strcmp(name, "oneText")) {
			if (property->count < INDIGO_MAX_ITEMS)
				property->count++;
			return new_one_text_vector_handler;
		}
	} else if (state == ATTRIBUTE_VALUE) {
		if (!strcmp(name, "device")) {
			strncpy(property->device, value,INDIGO_NAME_SIZE);
		} else if (!strcmp(name, "name")) {
			indigo_copy_property_name(client ? client->version : INDIGO_VERSION_CURRENT, property, value);
		} else if (!strcmp(name, "state")) {
			property->state = parse_state(value);
		}
	} else if (state == END_TAG) {
		indigo_change_property(client, property);
		memset(property, 0, PROPERTY_SIZE);
		return top_level_handler;
	}
	return new_text_vector_handler;
}

static void *new_one_number_vector_handler(parser_state state, parser_context *context, char *name, char *value, char *message) {
	indigo_property *property = (indigo_property *)context->property_buffer;
	indigo_client *client = context->client;
	INDIGO_TRACE_PARSER(indigo_trace("XML Parser: new_one_number_vector_handler %s '%s' '%s'", parser_state_name[state], name != NULL ? name : "", value != NULL ? value : ""));
	if (state == ATTRIBUTE_VALUE) {
		if (!strcmp(name, "name")) {
			indigo_copy_item_name(client ? client->version : INDIGO_VERSION_CURRENT, property, property->items+property->count-1, value);
		}
	} else if (state == TEXT) {
		property->items[property->count-1].number.value = atof(value);
	} else if (state == END_TAG) {
		return new_number_vector_handler;
	}
	return new_one_number_vector_handler;
}

static void *new_number_vector_handler(parser_state state, parser_context *context, char *name, char *value, char *message) {
	indigo_property *property = (indigo_property *)context->property_buffer;
	indigo_client *client = context->client;
	INDIGO_TRACE_PARSER(indigo_trace("XML Parser: new_number_vector_handler %s '%s' '%s'", parser_state_name[state], name != NULL ? name : "", value != NULL ? value : ""));
	if (state == BEGIN_TAG) {
		if (!strcmp(name, "oneNumber")) {
			if (property->count < INDIGO_MAX_ITEMS)
				property->count++;
			return new_one_number_vector_handler;
		}
	} else if (state == ATTRIBUTE_VALUE) {
		if (!strcmp(name, "device")) {
			strncpy(property->device, value,INDIGO_NAME_SIZE);
		} else if (!strcmp(name, "name")) {
			indigo_copy_property_name(client ? client->version : INDIGO_VERSION_CURRENT, property, value);
		} else if (!strcmp(name, "state")) {
			property->state = parse_state(value);
		}
	} else if (state == END_TAG) {
		indigo_change_property(client, property);
		memset(property, 0, PROPERTY_SIZE);
		return top_level_handler;
	}
	return new_number_vector_handler;
}

static void *new_one_switch_vector_handler(parser_state state, parser_context *context, char *name, char *value, char *message) {
	indigo_property *property = (indigo_property *)context->property_buffer;
	indigo_client *client = context->client;
	INDIGO_TRACE_PARSER(indigo_trace("XML Parser: new_one_switch_vector_handler %s '%s' '%s'", parser_state_name[state], name != NULL ? name : "", value != NULL ? value : ""));
	if (state == ATTRIBUTE_VALUE) {
		if (!strcmp(name, "name")) {
			indigo_copy_item_name(client ? client->version : INDIGO_VERSION_CURRENT, property, property->items+property->count-1, value);
		}
	} else if (state == TEXT) {
		property->items[property->count-1].sw.value = !strcmp(value, "On");
	} else if (state == END_TAG) {
		return new_switch_vector_handler;
	}
	return new_one_switch_vector_handler;
}

static void *new_switch_vector_handler(parser_state state, parser_context *context, char *name, char *value, char *message) {
	indigo_property *property = (indigo_property *)context->property_buffer;
	indigo_client *client = context->client;
	INDIGO_TRACE_PARSER(indigo_trace("XML Parser: new_switch_vector_handler %s '%s' '%s'", parser_state_name[state], name != NULL ? name : "", value != NULL ? value : ""));
	if (state == BEGIN_TAG) {
		if (!strcmp(name, "oneSwitch")) {
			if (property->count < INDIGO_MAX_ITEMS)
				property->count++;
			return new_one_switch_vector_handler;
		}
	} else if (state == ATTRIBUTE_VALUE) {
		if (!strcmp(name, "device")) {
			strncpy(property->device, value,INDIGO_NAME_SIZE);
		} else if (!strcmp(name, "name")) {
			indigo_copy_property_name(client ? client->version : INDIGO_VERSION_CURRENT, property, value);
		} else if (!strcmp(name, "state")) {
			property->state = parse_state(value);
		}
		return new_switch_vector_handler;
	} else if (state == END_TAG) {
		indigo_change_property(client, property);
		memset(property, 0, PROPERTY_SIZE);
		return top_level_handler;
	}
	return new_switch_vector_handler;
}

static void *switch_protocol_handler(parser_state state, parser_context *context, char *name, char *value, char *message) {
	indigo_device *device = context->device;
	assert(device != NULL);
	INDIGO_TRACE_PARSER(indigo_trace("XML Parser: switch_protocol_handler %s '%s' '%s'", parser_state_name[state], name != NULL ? name : "", value != NULL ? value : ""));
	if (state == ATTRIBUTE_VALUE) {
		if (!strcmp(name, "version")) {
			int major, minor;
			sscanf(value, "%d.%d", &major, &minor);
			device->version = major << 8 | minor;
		}
	} else if (state == END_TAG) {
		return top_level_handler;
	}
	return switch_protocol_handler;
}

static void set_property(parser_context *context, indigo_property *other, char *message) {
	for (int index = 0; index < context->count; index++) {
		indigo_property *property = context->properties[index];
		if (property != NULL && !strncmp(property->device, other->device, INDIGO_NAME_SIZE) && !strncmp(property->name, other->name, INDIGO_NAME_SIZE)) {
			property->state = other->state;
			if (property->type == INDIGO_SWITCH_VECTOR && property->rule != INDIGO_ANY_OF_MANY_RULE) {
				for (int j = 0; j < property->count; j++) {
					property->items[j].sw.value = false;
				}
			}
			for (int i = 0; i < other->count; i++) {
				indigo_item *other_item = &other->items[i];
				for (int j = 0; j < property->count; j++) {
					indigo_item *property_item = &property->items[j];
					if (!strcmp(property_item->name, other_item->name)) {
						switch (property->type) {
							case INDIGO_TEXT_VECTOR:
								strncpy(property_item->text.value, other_item->text.value, INDIGO_VALUE_SIZE);
								break;
							case INDIGO_NUMBER_VECTOR:
								property_item->number.value = other_item->number.value;
								if (!isnan(other_item->number.min))
									property_item->number.min = other_item->number.min;
								if (!isnan(other_item->number.max))
									property_item->number.max = other_item->number.max;
								if (!isnan(other_item->number.step))
									property_item->number.step = other_item->number.step;
								if (property_item->number.value < property_item->number.min) {
									property_item->number.value = property_item->number.min;
									indigo_debug("%s.%s value out of range", property->name, property_item->name);
								}
								if (property_item->number.value > property_item->number.max) {
									property_item->number.value = property_item->number.max;
									indigo_debug("%s.%s value out of range", property->name, property_item->name);
								}
								property_item->number.target = other_item->number.target;
								break;
							case INDIGO_SWITCH_VECTOR:
								property_item->sw.value = other_item->sw.value;
								break;
							case INDIGO_LIGHT_VECTOR:
								property_item->light.value = other_item->light.value;
								break;
							case INDIGO_BLOB_VECTOR:
								strncpy(property_item->blob.format, other_item->blob.format, INDIGO_NAME_SIZE);
								strncpy(property_item->blob.url, other_item->blob.url, INDIGO_VALUE_SIZE);
								property_item->blob.size = other_item->blob.size;
								if (property_item->blob.value != NULL)
									property_item->blob.value = realloc(property_item->blob.value, property_item->blob.size);
								else
									property_item->blob.value = malloc(property_item->blob.size);
								memcpy(property_item->blob.value, other_item->blob.value, property_item->blob.size);
								break;
						}
						break;
					}
				}
			}
			INDIGO_TRACE_PARSER(indigo_trace("XML Parser: set_property '%s' '%s' %d", property->device, property->name, index));
			indigo_update_property(context->device, property, *message ? message : NULL);
			break;
		}
	}
}

static void *set_one_text_vector_handler(parser_state state, parser_context *context, char *name, char *value, char *message) {
	indigo_property *property = (indigo_property *)context->property_buffer;
	indigo_device *device = context->device;
	INDIGO_TRACE_PARSER(indigo_trace("XML Parser: set_one_text_vector_handler %s '%s' '%s'", parser_state_name[state], name != NULL ? name : "", value != NULL ? value : ""));
	if (state == ATTRIBUTE_VALUE) {
		if (!strcmp(name, "name")) {
			indigo_copy_item_name(device->version, property, property->items+property->count-1, value);
		}
	} else if (state == TEXT) {
		strncat(property->items[property->count-1].text.value, value, INDIGO_VALUE_SIZE-1);
	} else if (state == END_TAG) {
		return set_text_vector_handler;
	}
	return set_one_text_vector_handler;
}

static void *set_text_vector_handler(parser_state state, parser_context *context, char *name, char *value, char *message) {
	indigo_property *property = (indigo_property *)context->property_buffer;
	indigo_device *device = context->device;
	INDIGO_TRACE_PARSER(indigo_trace("XML Parser: set_text_vector_handler %s '%s' '%s'", parser_state_name[state], name != NULL ? name : "", value != NULL ? value : ""));
	if (state == BEGIN_TAG) {
		if (!strcmp(name, "oneText")) {
			if (property->count < INDIGO_MAX_ITEMS)
				property->count++;
			return set_one_text_vector_handler;
		}
	} else if (state == ATTRIBUTE_VALUE) {
		if (!strcmp(name, "device")) {
			if (indigo_use_host_suffix)
				snprintf(property->device, INDIGO_NAME_SIZE, "%s %s", value, context->device->name);
			else
				strncpy(property->device, value, INDIGO_NAME_SIZE);
		} else if (!strcmp(name, "name")) {
			indigo_copy_property_name(device->version, property, value);
		} else if (!strcmp(name, "state")) {
			property->state = parse_state(value);
		} else if (!strcmp(name, "message")) {
			strncpy(message, value, INDIGO_VALUE_SIZE);
		}
	} else if (state == END_TAG) {
		set_property(context, property, message);
		memset(property, 0, PROPERTY_SIZE);
		return top_level_handler;
	}
	return set_text_vector_handler;
}

static void *set_one_number_vector_handler(parser_state state, parser_context *context, char *name, char *value, char *message) {
	indigo_property *property = (indigo_property *)context->property_buffer;
	indigo_device *device = context->device;
	INDIGO_TRACE_PARSER(indigo_trace("XML Parser: set_one_number_vector_handler %s '%s' '%s'", parser_state_name[state], name != NULL ? name : "", value != NULL ? value : ""));
	if (state == ATTRIBUTE_VALUE) {
		if (!strcmp(name, "name")) {
			indigo_copy_item_name(device->version, property, property->items+property->count-1, value);
		} else if (!strcmp(name, "target")) {
			property->items[property->count-1].number.target = atof(value);
		} else if (!strcmp(name, "min")) {
			property->items[property->count-1].number.min = atof(value);
		} else if (!strcmp(name, "max")) {
			property->items[property->count-1].number.max = atof(value);
		} else if (!strcmp(name, "step")) {
			property->items[property->count-1].number.step = atof(value);
		} else if (!strcmp(name, "format")) {
			strncpy(property->items[property->count-1].number.format, value, INDIGO_NAME_SIZE);
		}
	} else if (state == TEXT) {
		property->items[property->count-1].number.value = atof(value);
	} else if (state == END_TAG) {
		return set_number_vector_handler;
	}
	return set_one_number_vector_handler;
}

static void *set_number_vector_handler(parser_state state, parser_context *context, char *name, char *value, char *message) {
	indigo_property *property = (indigo_property *)context->property_buffer;
	indigo_device *device = context->device;
	INDIGO_TRACE_PARSER(indigo_trace("XML Parser: set_number_vector_handler %s '%s' '%s'", parser_state_name[state], name != NULL ? name : "", value != NULL ? value : ""));
	if (state == BEGIN_TAG) {
		if (!strcmp(name, "oneNumber")) {
			if (property->count < INDIGO_MAX_ITEMS) {
				property->items[property->count].number.min = NAN;
				property->items[property->count].number.max = NAN;
				property->items[property->count].number.step = NAN;
				property->count++;
			}
			return set_one_number_vector_handler;
		}
	} else if (state == ATTRIBUTE_VALUE) {
		if (!strcmp(name, "device")) {
			if (indigo_use_host_suffix)
				snprintf(property->device, INDIGO_NAME_SIZE, "%s %s", value, context->device->name);
			else
				strncpy(property->device, value, INDIGO_NAME_SIZE);
		} else if (!strcmp(name, "name")) {
			indigo_copy_property_name(device->version, property, value);
		} else if (!strcmp(name, "state")) {
			property->state = parse_state(value);
		} else if (!strcmp(name, "message")) {
			strncpy(message, value, INDIGO_VALUE_SIZE);
		}
	} else if (state == END_TAG) {
		set_property(context, property, message);
		memset(property, 0, PROPERTY_SIZE);
		return top_level_handler;
	}
	return set_number_vector_handler;
}

static void *set_one_switch_vector_handler(parser_state state, parser_context *context, char *name, char *value, char *message) {
	indigo_property *property = (indigo_property *)context->property_buffer;
	indigo_device *device = context->device;
	INDIGO_TRACE_PARSER(indigo_trace("XML Parser: set_one_switch_vector_handler %s '%s' '%s'", parser_state_name[state], name != NULL ? name : "", value != NULL ? value : ""));
	if (state == ATTRIBUTE_VALUE) {
		if (!strcmp(name, "name")) {
			indigo_copy_item_name(device->version, property, property->items+property->count-1, value);
			return set_one_switch_vector_handler;
		}
	} else if (state == TEXT) {
		property->items[property->count-1].sw.value = !strcmp(value, "On");
		return set_one_switch_vector_handler;
	}
	return set_switch_vector_handler;
}

static void *set_switch_vector_handler(parser_state state, parser_context *context, char *name, char *value, char *message) {
	indigo_property *property = (indigo_property *)context->property_buffer;
	indigo_device *device = context->device;
	INDIGO_TRACE_PARSER(indigo_trace("XML Parser: set_switch_vector_handler %s '%s' '%s'", parser_state_name[state], name != NULL ? name : "", value != NULL ? value : ""));
	if (state == BEGIN_TAG) {
		if (!strcmp(name, "oneSwitch")) {
			if (property->count < INDIGO_MAX_ITEMS)
				property->count++;
			return set_one_switch_vector_handler;
		}
	} else if (state == ATTRIBUTE_VALUE) {
		if (!strcmp(name, "device")) {
			if (indigo_use_host_suffix)
				snprintf(property->device, INDIGO_NAME_SIZE, "%s %s", value, context->device->name);
			else
				strncpy(property->device, value, INDIGO_NAME_SIZE);
		} else if (!strcmp(name, "name")) {
			indigo_copy_property_name(device->version, property, value);
		} else if (!strcmp(name, "state")) {
			property->state = parse_state(value);
		} else if (!strcmp(name, "message")) {
			strncpy(message, value, INDIGO_VALUE_SIZE);
		}
	} else if (state == END_TAG) {
		set_property(context, property, message);
		memset(property, 0, PROPERTY_SIZE);
		return top_level_handler;
	}
	return set_switch_vector_handler;
}

static void *set_one_light_vector_handler(parser_state state, parser_context *context, char *name, char *value, char *message) {
	indigo_property *property = (indigo_property *)context->property_buffer;
	indigo_device *device = context->device;
	INDIGO_TRACE_PARSER(indigo_trace("XML Parser: set_one_light_vector_handler %s '%s' '%s'", parser_state_name[state], name != NULL ? name : "", value != NULL ? value : ""));
	if (state == ATTRIBUTE_VALUE) {
		if (!strcmp(name, "name")) {
			indigo_copy_item_name(device->version, property, property->items+property->count-1, value);
		}
	} else if (state == TEXT) {
		property->items[property->count-1].light.value = parse_state(value);
	} else if (state == END_TAG) {
		return set_light_vector_handler;
	}
	return set_one_light_vector_handler;
}

static void *set_light_vector_handler(parser_state state, parser_context *context, char *name, char *value, char *message) {
	indigo_property *property = (indigo_property *)context->property_buffer;
	indigo_device *device = context->device;
	INDIGO_TRACE_PARSER(indigo_trace("XML Parser: set_light_vector_handler %s '%s' '%s'", parser_state_name[state], name != NULL ? name : "", value != NULL ? value : ""));
	if (state == BEGIN_TAG) {
		if (!strcmp(name, "oneLight")) {
			if (property->count < INDIGO_MAX_ITEMS)
				property->count++;
			return set_one_light_vector_handler;
		}
	} else if (state == ATTRIBUTE_VALUE) {
		if (!strcmp(name, "device")) {
			if (indigo_use_host_suffix)
				snprintf(property->device, INDIGO_NAME_SIZE, "%s %s", value, context->device->name);
			else
				strncpy(property->device, value, INDIGO_NAME_SIZE);
		} else if (!strcmp(name, "name")) {
			indigo_copy_property_name(device->version, property, value);
		} else if (!strcmp(name, "state")) {
			property->state = parse_state(value);
		} else if (!strcmp(name, "message")) {
			strncpy(message, value, INDIGO_VALUE_SIZE);
		}
	} else if (state == END_TAG) {
		set_property(context, property, message);
		memset(property, 0, PROPERTY_SIZE);
		return top_level_handler;
	}
	return set_light_vector_handler;
}

static void *set_one_blob_vector_handler(parser_state state, parser_context *context, char *name, char *value, char *message) {
	indigo_property *property = (indigo_property *)context->property_buffer;
	indigo_device *device = context->device;
	INDIGO_DEBUG_PROTOCOL(if (state == BLOB))
	INDIGO_TRACE_PARSER(indigo_trace("XML Parser: set_one_blob_vector_handler %s '%s' DATA", parser_state_name[state], name != NULL ? name : ""));
	INDIGO_DEBUG_PROTOCOL(else)
	INDIGO_TRACE_PARSER(indigo_trace("XML Parser: set_one_blob_vector_handler %s '%s' '%s'", parser_state_name[state], name != NULL ? name : "", value != NULL ? value : ""));
	if (state == ATTRIBUTE_VALUE) {
		if (!strcmp(name, "name")) {
			indigo_copy_item_name(device->version, property, property->items+property->count-1, value);
		} else if (!strcmp(name, "format")) {
			strncpy(property->items[property->count-1].blob.format, value, INDIGO_NAME_SIZE);
		} else if (!strcmp(name, "size")) {
			property->items[property->count-1].blob.size = atol(value);
		} else if (!strcmp(name, "path")) {
			snprintf(property->items[property->count-1].blob.url, INDIGO_VALUE_SIZE, "%s%s", ((indigo_adapter_context *)context->device->device_context)->url_prefix, value);
		} else if (!strcmp(name, "url")) {
			strncpy(property->items[property->count-1].blob.url, value, INDIGO_VALUE_SIZE);
		}
	} else if (state == BLOB) {
		property->items[property->count-1].blob.value = value;
	} else if (state == END_TAG) {
		return set_blob_vector_handler;
	}
	return set_one_blob_vector_handler;
}

static void *set_blob_vector_handler(parser_state state, parser_context *context, char *name, char *value, char *message) {
	indigo_property *property = (indigo_property *)context->property_buffer;
	indigo_device *device = context->device;
	INDIGO_TRACE_PARSER(indigo_trace("XML Parser: set_blob_vector_handler %s '%s' '%s'", parser_state_name[state], name != NULL ? name : "", value != NULL ? value : ""));
	if (state == BEGIN_TAG) {
		if (!strcmp(name, "oneBLOB")) {
			if (property->count < INDIGO_MAX_ITEMS)
				property->count++;
			return set_one_blob_vector_handler;
		}
	} else if (state == ATTRIBUTE_VALUE) {
		if (!strcmp(name, "device")) {
			if (indigo_use_host_suffix)
				snprintf(property->device, INDIGO_NAME_SIZE, "%s %s", value, context->device->name);
			else
				strncpy(property->device, value, INDIGO_NAME_SIZE);
		} else if (!strcmp(name, "name")) {
			indigo_copy_property_name(device->version, property, value);
		} else if (!strcmp(name, "state")) {
			property->state = parse_state(value);
		} else if (!strcmp(name, "message")) {
			strncpy(message, value, INDIGO_VALUE_SIZE);
		}
	} else if (state == END_TAG) {
		set_property(context, property, message);
		memset(property, 0, PROPERTY_SIZE);
		return top_level_handler;
	}
	return set_blob_vector_handler;
}

static void def_property(parser_context *context, indigo_property *other, char *message) {
	indigo_property *property = NULL;
	int index;
	for (index = 0; index < context->count; index++) {
		property = context->properties[index];
		if (property == NULL)
			break;
		if (!strncmp(property->device, other->device, INDIGO_NAME_SIZE) && !strncmp(property->name, other->name, INDIGO_NAME_SIZE))
			break;
	}
	if (index == context->count) {
		context->properties = realloc(context->properties, context->count * 2 * sizeof(indigo_property *));
		memset(context->properties + context->count, 0, context->count * sizeof(indigo_property *));
		context->count *= 2;
		property = NULL;
	}
	if (property == NULL) {
		switch (other->type) {
			case INDIGO_TEXT_VECTOR:
				property = indigo_init_text_property(property, other->device, other->name, other->group, other->label, other->state, other->perm, other->count);
				memcpy(property->items, other->items, other->count * sizeof(indigo_item));
				break;
			case INDIGO_NUMBER_VECTOR:
				property = indigo_init_number_property(property, other->device, other->name, other->group, other->label, other->state, other->perm, other->count);
				memcpy(property->items, other->items, other->count * sizeof(indigo_item));
				break;
			case INDIGO_SWITCH_VECTOR:
				property = indigo_init_switch_property(property, other->device, other->name, other->group, other->label, other->state, other->perm, other->rule, other->count);
				memcpy(property->items, other->items, other->count * sizeof(indigo_item));
				break;
			case INDIGO_LIGHT_VECTOR:
				property = indigo_init_light_property(property, other->device, other->name, other->group, other->label, other->state, other->count);
				memcpy(property->items, other->items, other->count * sizeof(indigo_item));
				break;
			case INDIGO_BLOB_VECTOR:
				property = indigo_init_blob_property(property, other->device, other->name, other->group, other->label, other->state, other->count);
				memcpy(property->items, other->items, other->count * sizeof(indigo_item));
				for (int i = 0; i < property->count; i++) {
					indigo_item *item = property->items + i;
					if (item->blob.size > 0 && item->blob.value != NULL) {
						void *tmp = malloc(item->blob.size);
						memcpy(tmp, item->blob.value, item->blob.size);
						item->blob.value = tmp;
					}
				}
				break;
		}
		context->properties[index] = property;
	}
	INDIGO_TRACE_PARSER(indigo_trace("XML Parser: def_property '%s' '%s' %d", property->device, property->name, index));
	indigo_define_property(context->device, property, *message ? message : NULL);
}

static void *def_text_handler(parser_state state, parser_context *context, char *name, char *value, char *message) {
	indigo_property *property = (indigo_property *)context->property_buffer;
	indigo_device *device = context->device;
	INDIGO_TRACE_PARSER(indigo_trace("XML Parser: def_text_handler %s '%s' '%s'", parser_state_name[state], name != NULL ? name : "", value != NULL ? value : ""));
	if (state == ATTRIBUTE_VALUE) {
		if (!strcmp(name, "name")) {
			indigo_copy_item_name(device->version, property, property->items+property->count-1, value);
		} else if (!strcmp(name, "label")) {
			strncpy(property->items[property->count-1].label, value, INDIGO_VALUE_SIZE);
		}
	} else if (state == TEXT) {
		strncat(property->items[property->count-1].text.value, value, INDIGO_VALUE_SIZE-1);
	} else if (state == END_TAG) {
		return def_text_vector_handler;
	}
	return def_text_handler;
}

static void *def_text_vector_handler(parser_state state, parser_context *context, char *name, char *value, char *message) {
	indigo_property *property = (indigo_property *)context->property_buffer;
	indigo_device *device = context->device;
	INDIGO_TRACE_PARSER(indigo_trace("XML Parser: def_text_vector_handler %s '%s' '%s'", parser_state_name[state], name != NULL ? name : "", value != NULL ? value : ""));
	if (state == BEGIN_TAG) {
		if (!strcmp(name, "defText")) {
			if (property->count < INDIGO_MAX_ITEMS)
				property->count++;
			return def_text_handler;
		}
	} else if (state == ATTRIBUTE_VALUE) {
		if (!strcmp(name, "device")) {
			if (indigo_use_host_suffix)
				snprintf(property->device, INDIGO_NAME_SIZE, "%s %s", value, context->device->name);
			else
				strncpy(property->device, value, INDIGO_NAME_SIZE);
		} else if (!strcmp(name, "name")) {
			indigo_copy_property_name(device->version, property, value);
		} else if (!strcmp(name, "group")) {
			strncpy(property->group, value,INDIGO_NAME_SIZE);
		} else if (!strcmp(name, "label")) {
			strncpy(property->label, value, INDIGO_VALUE_SIZE);
		} else if (!strcmp(name, "state")) {
			property->state = parse_state(value);
		} else if (!strcmp(name, "perm")) {
			property->perm = parse_perm(value);
		} else if (!strcmp(name, "message")) {
			strncpy(message, value, INDIGO_VALUE_SIZE);
		}
	} else if (state == END_TAG) {
		def_property(context, property, message);
		memset(property, 0, PROPERTY_SIZE);
		return top_level_handler;
	}
	return def_text_vector_handler;
}

static void *def_number_handler(parser_state state, parser_context *context, char *name, char *value, char *message) {
	indigo_property *property = (indigo_property *)context->property_buffer;
	indigo_device *device = context->device;
	INDIGO_TRACE_PARSER(indigo_trace("XML Parser: def_number_handler %s '%s' '%s'", parser_state_name[state], name != NULL ? name : "", value != NULL ? value : ""));
	if (state == ATTRIBUTE_VALUE) {
		if (!strcmp(name, "name")) {
			indigo_copy_item_name(device->version, property, property->items+property->count-1, value);
		} else if (!strcmp(name, "label")) {
			strncpy(property->items[property->count-1].label, value, INDIGO_VALUE_SIZE);
		} else if (!strcmp(name, "min")) {
			property->items[property->count-1].number.min = atof(value);
		} else if (!strcmp(name, "max")) {
			property->items[property->count-1].number.max = atof(value);
		} else if (!strcmp(name, "step")) {
			property->items[property->count-1].number.step = atof(value);
		} else if (!strcmp(name, "format")) {
			strncpy(property->items[property->count-1].number.format, value, INDIGO_NAME_SIZE);
		}
	} else if (state == TEXT) {
		property->items[property->count-1].number.value = atof(value);
	} else if (state == END_TAG) {
		return def_number_vector_handler;
	}
	return def_number_handler;
}

static void *def_number_vector_handler(parser_state state, parser_context *context, char *name, char *value, char *message) {
	indigo_property *property = (indigo_property *)context->property_buffer;
	indigo_device *device = context->device;
	INDIGO_TRACE_PARSER(indigo_trace("XML Parser: def_number_vector_handler %s '%s' '%s'", parser_state_name[state], name != NULL ? name : "", value != NULL ? value : ""));
	if (state == BEGIN_TAG) {
		if (!strcmp(name, "defNumber")) {
			if (property->count < INDIGO_MAX_ITEMS)
				property->count++;
			return def_number_handler;
		}
	} else if (state == ATTRIBUTE_VALUE) {
		if (!strcmp(name, "device")) {
			if (indigo_use_host_suffix)
				snprintf(property->device, INDIGO_NAME_SIZE, "%s %s", value, context->device->name);
			else
				strncpy(property->device, value, INDIGO_NAME_SIZE);
		} else if (!strcmp(name, "name")) {
			indigo_copy_property_name(device->version, property, value);
		} else if (!strcmp(name, "group")) {
			strncpy(property->group, value,INDIGO_NAME_SIZE);
		} else if (!strcmp(name, "label")) {
			strncpy(property->label, value, INDIGO_VALUE_SIZE);
		} else if (!strcmp(name, "state")) {
			property->state = parse_state(value);
		} else if (!strcmp(name, "perm")) {
			property->perm = parse_perm(value);
		} else if (!strcmp(name, "message")) {
			strncpy(message, value, INDIGO_VALUE_SIZE);
		}
	} else if (state == END_TAG) {
		def_property(context, property, message);
		memset(property, 0, PROPERTY_SIZE);
		return top_level_handler;
	}
	return def_number_vector_handler;
}

static void *def_switch_handler(parser_state state, parser_context *context, char *name, char *value, char *message) {
	indigo_property *property = (indigo_property *)context->property_buffer;
	indigo_device *device = context->device;
	INDIGO_TRACE_PARSER(indigo_trace("XML Parser: def_switch_handler %s '%s' '%s'", parser_state_name[state], name != NULL ? name : "", value != NULL ? value : ""));
	if (state == ATTRIBUTE_VALUE) {
		if (!strcmp(name, "name")) {
			indigo_copy_item_name(device->version, property, property->items+property->count-1, value);
		} else if (!strcmp(name, "label")) {
			strncpy(property->items[property->count-1].label, value, INDIGO_VALUE_SIZE);
		}
	} else if (state == TEXT) {
		property->items[property->count-1].sw.value = !strcmp(value, "On");
	} else if (state == END_TAG) {
		return def_switch_vector_handler;
	}
	return def_switch_handler;
}

static void *def_switch_vector_handler(parser_state state, parser_context *context, char *name, char *value, char *message) {
	indigo_property *property = (indigo_property *)context->property_buffer;
	indigo_device *device = context->device;
	INDIGO_TRACE_PARSER(indigo_trace("XML Parser: def_switch_vector_handler %s '%s' '%s'", parser_state_name[state], name != NULL ? name : "", value != NULL ? value : ""));
	if (state == BEGIN_TAG) {
		if (!strcmp(name, "defSwitch")) {
			if (property->count < INDIGO_MAX_ITEMS)
				property->count++;
			return def_switch_handler;
		}
	} else if (state == ATTRIBUTE_VALUE) {
		if (!strcmp(name, "device")) {
			if (indigo_use_host_suffix)
				snprintf(property->device, INDIGO_NAME_SIZE, "%s %s", value, context->device->name);
			else
				strncpy(property->device, value, INDIGO_NAME_SIZE);
		} else if (!strcmp(name, "name")) {
			indigo_copy_property_name(device->version, property, value);
		} else if (!strcmp(name, "group")) {
			strncpy(property->group, value,INDIGO_NAME_SIZE);
		} else if (!strcmp(name, "label")) {
			strncpy(property->label, value, INDIGO_VALUE_SIZE);
		} else if (!strcmp(name, "state")) {
			property->state = parse_state(value);
		} else if (!strcmp(name, "perm")) {
			property->perm = parse_perm(value);
		} else if (!strcmp(name, "rule")) {
			property->rule = parse_rule(value);
		} else if (!strcmp(name, "message")) {
			strncpy(message, value, INDIGO_VALUE_SIZE);
		}
	} else if (state == END_TAG) {
		def_property(context, property, message);
		memset(property, 0, PROPERTY_SIZE);
		return top_level_handler;
	}
	return def_switch_vector_handler;
}

static void *def_light_handler(parser_state state, parser_context *context, char *name, char *value, char *message) {
	indigo_property *property = (indigo_property *)context->property_buffer;
	indigo_device *device = context->device;
	INDIGO_TRACE_PARSER(indigo_trace("XML Parser: def_light_handler %s '%s' '%s'", parser_state_name[state], name != NULL ? name : "", value != NULL ? value : ""));
	if (state == ATTRIBUTE_VALUE) {
		if (!strcmp(name, "name")) {
			indigo_copy_item_name(device->version, property, property->items+property->count-1, value);
		} else if (!strcmp(name, "label")) {
			strncpy(property->items[property->count-1].label, value, INDIGO_VALUE_SIZE);
		}
	} else if (state == TEXT) {
		property->items[property->count-1].light.value = parse_state(value);
	} else if (state == END_TAG) {
		return def_light_vector_handler;
	}
	return def_light_handler;
}

static void *def_light_vector_handler(parser_state state, parser_context *context, char *name, char *value, char *message) {
	indigo_property *property = (indigo_property *)context->property_buffer;
	indigo_device *device = context->device;
	INDIGO_TRACE_PARSER(indigo_trace("XML Parser: def_light_vector_handler %s '%s' '%s'", parser_state_name[state], name != NULL ? name : "", value != NULL ? value : ""));
	if (state == BEGIN_TAG) {
		if (!strcmp(name, "defLight")) {
			if (property->count < INDIGO_MAX_ITEMS)
				property->count++;
			return def_light_handler;
		}
	} else if (state == ATTRIBUTE_VALUE) {
		if (!strcmp(name, "device")) {
			if (indigo_use_host_suffix)
				snprintf(property->device, INDIGO_NAME_SIZE, "%s %s", value, context->device->name);
			else
				strncpy(property->device, value, INDIGO_NAME_SIZE);
		} else if (!strcmp(name, "name")) {
			indigo_copy_property_name(device->version, property, value);
		} else if (!strcmp(name, "group")) {
			strncpy(property->group, value,INDIGO_NAME_SIZE);
		} else if (!strcmp(name, "label")) {
			strncpy(property->label, value, INDIGO_VALUE_SIZE);
		} else if (!strcmp(name, "state")) {
			property->state = parse_state(value);
		} else if (!strcmp(name, "message")) {
			strncpy(message, value, INDIGO_VALUE_SIZE);
		}
	} else if (state == END_TAG) {
		def_property(context, property, message);
		memset(property, 0, PROPERTY_SIZE);
		return top_level_handler;
	}
	return def_light_vector_handler;
}

static void *def_blob_handler(parser_state state, parser_context *context, char *name, char *value, char *message) {
	indigo_property *property = (indigo_property *)context->property_buffer;
	indigo_device *device = context->device;
	INDIGO_TRACE_PARSER(indigo_trace("XML Parser: def_blob_handler %s '%s' '%s'", parser_state_name[state], name != NULL ? name : "", value != NULL ? value : ""));
	if (state == ATTRIBUTE_VALUE) {
		if (!strcmp(name, "name")) {
			indigo_copy_item_name(device->version, property, property->items+property->count-1, value);
		} else if (!strcmp(name, "label")) {
			strncpy(property->items[property->count-1].label, value, INDIGO_VALUE_SIZE);
		} else if (!strcmp(name, "path")) {
			snprintf(property->items[property->count-1].blob.url, INDIGO_VALUE_SIZE, "%s%s", ((indigo_adapter_context *)context->device->device_context)->url_prefix, value);
		} else if (!strcmp(name, "url")) {
			strncpy(property->items[property->count-1].blob.url, value, INDIGO_VALUE_SIZE);
		}
	} else if (state == END_TAG) {
		return def_blob_vector_handler;
	}
	return def_blob_handler;
}

static void *def_blob_vector_handler(parser_state state, parser_context *context, char *name, char *value, char *message) {
	indigo_property *property = (indigo_property *)context->property_buffer;
	indigo_device *device = context->device;
	INDIGO_TRACE_PARSER(indigo_trace("XML Parser: def_blob_vector_handler %s '%s' '%s'", parser_state_name[state], name != NULL ? name : "", value != NULL ? value : ""));
	if (state == BEGIN_TAG) {
		if (!strcmp(name, "defBLOB")) {
			if (property->count < INDIGO_MAX_ITEMS)
				property->count++;
			return def_blob_handler;
		}
	} else if (state == ATTRIBUTE_VALUE) {
		if (!strcmp(name, "device")) {
			if (indigo_use_host_suffix)
				snprintf(property->device, INDIGO_NAME_SIZE, "%s %s", value, context->device->name);
			else
				strncpy(property->device, value, INDIGO_NAME_SIZE);
		} else if (!strcmp(name, "name")) {
			indigo_copy_property_name(device->version, property, value);
		} else if (!strcmp(name, "group")) {
			strncpy(property->group, value,INDIGO_NAME_SIZE);
		} else if (!strcmp(name, "label")) {
			strncpy(property->label, value, INDIGO_VALUE_SIZE);
		} else if (!strcmp(name, "state")) {
			property->state = parse_state(value);
		} else if (!strcmp(name, "perm")) {
			property->perm = parse_perm(value);
		} else if (!strcmp(name, "message")) {
			strncpy(message, value, INDIGO_VALUE_SIZE);
		}
	} else if (state == END_TAG) {
		def_property(context, property, message);
		memset(property, 0, PROPERTY_SIZE);
		return top_level_handler;
	}
	return def_blob_vector_handler;
}

static void *del_property_handler(parser_state state, parser_context *context, char *name, char *value, char *message) {
	indigo_property *property = (indigo_property *)context->property_buffer;
	indigo_device *device = context->device;
	INDIGO_TRACE_PARSER(indigo_trace("XML Parser: del_property_handler %s '%s' '%s'", parser_state_name[state], name != NULL ? name : "", value != NULL ? value : ""));
	if (state == ATTRIBUTE_VALUE) {
		if (!strncmp(name, "device",INDIGO_NAME_SIZE)) {
			if (indigo_use_host_suffix)
				snprintf(property->device, INDIGO_NAME_SIZE, "%s %s", value, context->device->name);
			else
				strncpy(property->device, value, INDIGO_NAME_SIZE);
		} else if (!strncmp(name, "name",INDIGO_NAME_SIZE)) {
			indigo_copy_property_name(device->version, property, value);;
		} else if (!strcmp(name, "message")) {
			strncpy(message, value, INDIGO_VALUE_SIZE);
		}
	} else if (state == END_TAG) {
		if (*property->name) {
			for (int i = 0; i < context->count; i++) {
				indigo_property *tmp = context->properties[i];
				if (tmp != NULL && !strncmp(tmp->device, property->device, INDIGO_NAME_SIZE) && !strncmp(tmp->name, property->name, INDIGO_NAME_SIZE)) {
					indigo_delete_property(device, tmp, *message ? message : NULL);
					indigo_release_property(tmp);
					context->properties[i] = NULL;
					break;
				}
			}
		} else {
			for (int i = 0; i < context->count; i++) {
				indigo_property *tmp = context->properties[i];
				if (tmp != NULL && !strncmp(tmp->device, property->device, INDIGO_NAME_SIZE)) {
					indigo_delete_property(device, tmp, *message ? message : NULL);
					indigo_release_property(tmp);
					context->properties[i] = NULL;
				}
			}
		}
		memset(property, 0, PROPERTY_SIZE);
		return top_level_handler;
	}
	return del_property_handler;
}

static void *message_handler(parser_state state, parser_context *context, char *name, char *value, char *message) {
	indigo_property *property = (indigo_property *)context->property_buffer;
	indigo_device *device = context->device;
	INDIGO_TRACE_PARSER(indigo_trace("XML Parser: message_handler %s '%s' '%s'", parser_state_name[state], name != NULL ? name : "", value != NULL ? value : ""));
	if (state == ATTRIBUTE_VALUE) {
		if (!strcmp(name, "message")) {
			strncpy(message, value, INDIGO_VALUE_SIZE);
		}
	} else if (state == END_TAG) {
		indigo_send_message(device, *message ? message : NULL);
		memset(property, 0, PROPERTY_SIZE);
		return top_level_handler;
	}
	return message_handler;
}

static void *top_level_handler(parser_state state, parser_context *context, char *name, char *value, char *message) {
	indigo_property *property = (indigo_property *)context->property_buffer;
	indigo_client *client = context->client;
	INDIGO_TRACE_PARSER(indigo_trace("XML Parser: top_level_handler %s '%s' '%s'", parser_state_name[state], name != NULL ? name : "", value != NULL ? value : ""));
	if (state == BEGIN_TAG) {
		*message = 0;
		if (!strcmp(name, "enableBLOB"))
			return enable_blob_handler;
		if (!strcmp(name, "getProperties") && client != NULL)
			return get_properties_handler;
		if (!strcmp(name, "newTextVector")) {
			property->type = INDIGO_TEXT_VECTOR;
			return new_text_vector_handler;
		}
		if (!strcmp(name, "newNumberVector")) {
			property->type = INDIGO_NUMBER_VECTOR;
			return new_number_vector_handler;
		}
		if (!strcmp(name, "newSwitchVector")) {
			property->type = INDIGO_SWITCH_VECTOR;
			return new_switch_vector_handler;
		}
		if (!strcmp(name, "switchProtocol"))
			return switch_protocol_handler;
		if (!strcmp(name, "setTextVector")) {
			property->type = INDIGO_TEXT_VECTOR;
			return set_text_vector_handler;
		}
		if (!strcmp(name, "setNumberVector")) {
			property->type = INDIGO_NUMBER_VECTOR;
			return set_number_vector_handler;
		}
		if (!strcmp(name, "setSwitchVector")) {
			property->type = INDIGO_SWITCH_VECTOR;
			return set_switch_vector_handler;
		}
		if (!strcmp(name, "setLightVector")) {
			property->type = INDIGO_LIGHT_VECTOR;
			return set_light_vector_handler;
		}
		if (!strcmp(name, "setBLOBVector")) {
			property->type = INDIGO_BLOB_VECTOR;
			return set_blob_vector_handler;
		}
		if (!strcmp(name, "defTextVector")) {
			property->type = INDIGO_TEXT_VECTOR;
			return def_text_vector_handler;
		}
		if (!strcmp(name, "defNumberVector")) {
			property->type = INDIGO_NUMBER_VECTOR;
			return def_number_vector_handler;
		}
		if (!strcmp(name, "defSwitchVector")) {
			property->type = INDIGO_SWITCH_VECTOR;
			return def_switch_vector_handler;
		}
		if (!strcmp(name, "defLightVector")) {
			property->type = INDIGO_LIGHT_VECTOR;
			return def_light_vector_handler;
		}
		if (!strcmp(name, "defBLOBVector")) {
			property->type = INDIGO_BLOB_VECTOR;
			return def_blob_vector_handler;
		}
		if (!strcmp(name, "delProperty"))
			return del_property_handler;
		if (!strcmp(name, "message"))
			return message_handler;
	}
	return top_level_handler;
}

void indigo_xml_parse(indigo_device *device, indigo_client *client) {
	char *buffer = malloc(BUFFER_SIZE+3); /* BUFFER_SIZE % 4 == 0 and keep always +3 for base64 alignmet */
	assert(buffer != NULL);
	char *value_buffer = malloc(BUFFER_SIZE+1); /* +1 to accomodate \0" */
	assert(value_buffer != NULL);
	char name_buffer[INDIGO_NAME_SIZE];
	unsigned char *blob_buffer = NULL;
	char *pointer = buffer;
	char *buffer_end = NULL;
	char *name_pointer = name_buffer;
	char *value_pointer = value_buffer;
	unsigned char *blob_pointer = NULL;
	long blob_size = 0;
	char message[INDIGO_VALUE_SIZE];
	char q = '"';
	int depth = 0;
	char c = 0;
	char entity_buffer[8];
	char *entity_pointer = NULL;
	bool is_escaped = false;
	/* (void)parser_state_name; */

	parser_handler handler = top_level_handler;

	parser_state state = IDLE;

	parser_context context;
	context.client = client;
	context.device = device;
	if (device != NULL) {
		context.count = 32;
		context.properties = malloc(context.count * sizeof(indigo_property *));
		memset(context.properties, 0, context.count * sizeof(indigo_property *));
	} else {
		context.count = 0;
		context.properties = NULL;
	}

	indigo_property *property = (indigo_property *)&context.property_buffer;
	memset(context.property_buffer, 0, PROPERTY_SIZE);

	int handle = 0;
	if (device != NULL) {
		handle = ((indigo_adapter_context *)device->device_context)->input;
		device->enumerate_properties(device, client, NULL);
	} else {
		handle = ((indigo_adapter_context *)client->client_context)->input;
	}
	*pointer = 0;
	while (true) {
		assert(pointer - buffer <= BUFFER_SIZE);
		assert(value_pointer - value_buffer <= BUFFER_SIZE);
		assert(name_pointer - name_buffer <= INDIGO_NAME_SIZE);
		if (state == ERROR) {
			indigo_error("XML Parser: syntax error");
			goto exit_loop;
		}
		while ((c = *pointer++) == 0) {
			ssize_t count = (int)read(handle, (void *)buffer, (ssize_t)BUFFER_SIZE);
			if (count <= 0) {
				goto exit_loop;
			}
			pointer = buffer;
			buffer_end = buffer + count;
			buffer[count] = 0;
			INDIGO_TRACE_PROTOCOL(indigo_trace("%d → %s", handle, buffer));
		}
		if (c == '&') {
			entity_pointer = entity_buffer;
			continue;
		}
		if (entity_pointer != NULL) {
			if (c == ';') {
				*entity_pointer++ = 0;
				if (!strcmp(entity_buffer, "amp"))
					c = '&';
				else if (!strcmp(entity_buffer, "lt"))
					c = '<';
				else if (!strcmp(entity_buffer, "gt"))
					c = '>';
				else if (!strcmp(entity_buffer, "quot"))
					c = '"';
				else if (!strcmp(entity_buffer, "apos"))
					c = '\'';
				entity_pointer = NULL;
				is_escaped = true;
			} else if (isalpha(c) && entity_pointer - entity_buffer < sizeof(entity_buffer)) {
				*entity_pointer++ = c;
				continue;
			} else {
				INDIGO_TRACE_PARSER(indigo_trace("XML Parser: invalid entity '&%s%c...'", entity_buffer, c));
				continue;
			}
		} else {
			is_escaped = false;
		}
		switch (state) {
			case IDLE:
				if (c == '<') {
					state = BEGIN_TAG1;
					INDIGO_TRACE_PARSER(indigo_trace("XML Parser: '%c' IDLE -> BEGIN_TAG1", c));
				}
				break;
			case BEGIN_TAG1:
				if (c == '?') {
					state = HEADER;
					INDIGO_TRACE_PARSER(indigo_trace("XML Parser: '%c' BEGIN_TAG1 -> HEADER", c));
				} else {
					name_pointer = name_buffer;
					if (isalpha(c)) {
						*name_pointer++ = c;
						state = BEGIN_TAG;
						INDIGO_TRACE_PARSER(indigo_trace("XML Parser: '%c' BEGIN_TAG1 -> BEGIN_TAG", c));
					} else if (c == '/') {
						state = END_TAG;
						INDIGO_TRACE_PARSER(indigo_trace("XML Parser: '%c' BEGIN_TAG1 -> END_TAG", c));
					}
				}
				break;
			case HEADER:
				if (c == '?') {
					state = HEADER1;
					INDIGO_TRACE_PARSER(indigo_trace("XML Parser: '%c' HEADER -> HEADER1", c));
				}
				break;
			case HEADER1:
				if (c == '>') {
					state = IDLE;
					INDIGO_TRACE_PARSER(indigo_trace("XML Parser: '%c' HEADER1 -> IDLE", c));
				} else {
					state = ERROR;
					INDIGO_TRACE_PARSER(indigo_trace("XML Parser: '%c' HEADER1 -> ERROR", c));
				}
				break;
			case BEGIN_TAG:
				if (name_pointer - name_buffer <INDIGO_NAME_SIZE && isalpha(c)) {
					*name_pointer++ = c;
					INDIGO_TRACE_PARSER(indigo_trace("XML Parser: '%c' BEGIN_TAG", c));
				} else {
					*name_pointer = 0;
					depth++;
					handler = handler(BEGIN_TAG, &context, name_buffer, NULL, message);
					if (isspace(c)) {
						state = ATTRIBUTE_NAME1;
						INDIGO_TRACE_PARSER(indigo_trace("XML Parser: '%c' BEGIN_TAG -> ATTRIBUTE_NAME1", c));
					} else if (c == '/') {
						state = END_TAG1;
						INDIGO_TRACE_PARSER(indigo_trace("XML Parser: '%c' BEGIN_TAG -> END_TAG1", c));
					} else if (c == '>') {
						state = TEXT;
						value_pointer = value_buffer;
						INDIGO_TRACE_PARSER(indigo_trace("XML Parser: '%c' BEGIN_TAG -> TEXT", c));
					} else {
						state = ERROR;
						INDIGO_TRACE_PARSER(indigo_trace("XML Parser: '%c' error BEGIN_TAG", c));
					}
				}
				break;
			case END_TAG1:
				if (c == '>') {
					INDIGO_TRACE_PARSER(indigo_trace("XML Parser: '%c' END_TAG1 -> IDLE", c));
					handler = handler(END_TAG, &context, NULL, NULL, message);
					depth--;
					state = IDLE;
				} else {
					state = ERROR;
					INDIGO_TRACE_PARSER(indigo_trace("XML Parser: '%c' error END_TAG1", c));
				}
				break;
			case END_TAG2:
				if (c == '/') {
					state = END_TAG;
					INDIGO_TRACE_PARSER(indigo_trace("XML Parser: '%c' END_TAG2 -> END_TAG", c));
				} else {
					state = ERROR;
					INDIGO_TRACE_PARSER(indigo_trace("XML Parser: '%c' error END_TAG2", c));
				}
				break;
			case END_TAG:
				if (isalpha(c)) {
					INDIGO_TRACE_PARSER(indigo_trace("XML Parser: '%c' END_TAG", c));
				} else if (c == '>') {
					handler = handler(END_TAG, &context, NULL, NULL, message);
					depth--;
					state = IDLE;
					INDIGO_TRACE_PARSER(indigo_trace("XML Parser: '%c' END_TAG -> IDLE", c));
				} else {
					state = ERROR;
					INDIGO_TRACE_PARSER(indigo_trace("XML Parser: '%c' error END_TAG", c));
				}
				break;
			case TEXT:
				if (c == '<' && !is_escaped) {
					if (depth == 2 || handler == enable_blob_handler) {
						*value_pointer-- = 0;
						while (value_pointer >= value_buffer && isspace(*value_pointer))
							*value_pointer-- = 0;
						value_pointer = value_buffer;
						while (*value_pointer && isspace(*value_pointer))
							value_pointer++;
						handler = handler(TEXT, &context, NULL, value_pointer, message);
					}
					state = TEXT1;
					INDIGO_TRACE_PARSER(indigo_trace("XML Parser: '%c' %d TEXT -> TEXT1", c, depth));
					break;
				} else {
					if (depth == 2 || handler == enable_blob_handler) {
						if (value_pointer - value_buffer < INDIGO_VALUE_SIZE) {
							*value_pointer++ = c;
						}
					}
					INDIGO_TRACE_PARSER(indigo_trace("XML Parser: '%c' %d TEXT", c, depth));
				}
				break;
			case TEXT1:
				if (c=='/') {
					state = END_TAG;
					INDIGO_TRACE_PARSER(indigo_trace("XML Parser: '%c' TEXT -> END_TAG", c));
				} else if (isalpha(c)) {
					name_pointer = name_buffer;
					*name_pointer++ = c;
					state = BEGIN_TAG;
					INDIGO_TRACE_PARSER(indigo_trace("XML Parser: '%c' TEXT -> BEGIN_TAG", c));
				}
				break;
			case BLOB_END:
				if (c == '<') {
					state = TEXT1;
					INDIGO_TRACE_PARSER(indigo_trace("XML Parser: '%c' BLOB_END -> TEXT1", c));
				}
				*name_pointer++ = c;
				break;
			case BLOB:
				if (device->version >= INDIGO_VERSION_2_0) {
					ssize_t count;
					pointer--;
					while (isspace(*pointer)) pointer++;
					unsigned long blob_len = (blob_size + 2) / 3 * 4;
					unsigned long len = (long)(buffer_end - pointer);
					len = (len < blob_len) ? len : blob_len;
					ssize_t bytes_needed = len % 4;
					if(bytes_needed) bytes_needed = 4 - bytes_needed;
					while (bytes_needed) {
						count = (int)read(handle, (void *)buffer_end, bytes_needed);
						if (count <= 0)
							goto exit_loop;
						len += count;
						bytes_needed -= count;
						buffer_end += count;
					}
					blob_pointer += base64_decode_fast((unsigned char*)blob_pointer, (unsigned char*)pointer, len);
					pointer += len;
					blob_len -= len;
					while(blob_len) {
						len = ((BUFFER_SIZE) < blob_len) ? (BUFFER_SIZE) : blob_len;
						ssize_t to_read = len;
						char *ptr = buffer;
						while(to_read) {
							count = (int)read(handle, (void *)ptr, to_read);
							if (count <= 0)
								goto exit_loop;
							ptr += count;
							to_read -= count;
						}
						blob_pointer += base64_decode_fast((unsigned char*)blob_pointer, (unsigned char*)buffer, len);
						blob_len -= len;
					}

					handler = handler(BLOB, &context, NULL, (char *)blob_buffer, message);
					pointer = buffer;
					*pointer = 0;
					state = BLOB_END;
					INDIGO_TRACE_PARSER(indigo_trace("XML Parser: '%c' %d BLOB -> BLOB_END", c, depth));
					break;
				} else {
					if (c == '<') {
						if (depth == 2) {
							*value_pointer = 0;
							blob_pointer += base64_decode_fast((unsigned char*)blob_pointer, (unsigned char*)value_buffer, (int)(value_pointer-value_buffer));
							handler = handler(BLOB, &context, NULL, (char *)blob_buffer, message);
						}
						state = TEXT1;
						INDIGO_TRACE_PARSER(indigo_trace("XML Parser: '%c' %d BLOB -> TEXT1", c, depth));
						break;
					} else if (c != '\n') {
						if (depth == 2) {
							if (value_pointer - value_buffer < BUFFER_SIZE) {
								*value_pointer++ = c;
							} else {
								*value_pointer = 0;
								blob_pointer += base64_decode_fast((unsigned char*)blob_pointer, (unsigned char*)value_buffer, (int)(value_pointer-value_buffer));
								value_pointer = value_buffer;
								*value_pointer++ = c;
							}
						}
						INDIGO_TRACE_PARSER(indigo_trace("XML Parser: '%c' %d BLOB", c, depth));
					}
				}
				break;
			case ATTRIBUTE_NAME1:
				if (isspace(c)) {
					INDIGO_TRACE_PARSER(indigo_trace("XML Parser: '%c' ATTRIBUTE_NAME1", c));
				} else if (isalpha(c)) {
					name_pointer = name_buffer;
					*name_pointer++ = c;
					state = ATTRIBUTE_NAME;
					INDIGO_TRACE_PARSER(indigo_trace("XML Parser: '%c' ATTRIBUTE_NAME1 -> ATTRIBUTE_NAME", c));
				} else if (c == '/') {
					state = END_TAG1;
				} else if (c == '>') {
					value_pointer = value_buffer;
					if (handler == set_one_blob_vector_handler) {
						blob_size = property->items[property->count-1].blob.size;
						if (blob_size > 0) {
							state = BLOB;
							if (blob_buffer != NULL) {
								unsigned char *ptmp = realloc(blob_buffer, blob_size);
								assert(ptmp != NULL);
								blob_buffer = ptmp;
							} else {
								blob_buffer = malloc(blob_size);
								assert(blob_buffer != NULL);
							}
							blob_pointer = blob_buffer;
						} else {
							state = TEXT;
						}
					} else
						state = TEXT;
					INDIGO_TRACE_PARSER(indigo_trace("XML Parser: '%c' ATTRIBUTE_NAME1 -> TEXT", c));
				} else {
					state = ERROR;
					INDIGO_TRACE_PARSER(indigo_trace("XML Parser: '%c' error ATTRIBUTE_NAME1", c));
				}
				break;
			case ATTRIBUTE_NAME:
				if (name_pointer - name_buffer <INDIGO_NAME_SIZE && isalpha(c)) {
					*name_pointer++ = c;
					INDIGO_TRACE_PARSER(indigo_trace("XML Parser: '%c' ATTRIBUTE_NAME", c));
				} else {
					*name_pointer = 0;
					if (c == '=') {
						state = ATTRIBUTE_VALUE1;
						INDIGO_TRACE_PARSER(indigo_trace("XML Parser: '%c' ATTRIBUTE_NAME -> ATTRIBUTE_VALUE1", c));
					} else {
						INDIGO_TRACE_PARSER(indigo_trace("XML Parser: '%c' ATTRIBUTE_NAME", c));
					}
				}
				break;
			case ATTRIBUTE_VALUE1:
				if (c == '"' || c == '\'') {
					q = c;
					value_pointer = value_buffer;
					state = ATTRIBUTE_VALUE;
					INDIGO_TRACE_PARSER(indigo_trace("XML Parser: '%c' ATTRIBUTE_VALUE1 -> ATTRIBUTE_VALUE2", c));
				} else {
					INDIGO_TRACE_PARSER(indigo_trace("XML Parser: '%c' ATTRIBUTE_VALUE1", c));
				}
				break;
			case ATTRIBUTE_VALUE:
				if (c == q && !is_escaped) {
					*value_pointer = 0;
					state = ATTRIBUTE_NAME1;
					handler = handler(ATTRIBUTE_VALUE, &context, name_buffer, value_buffer, message);
					INDIGO_TRACE_PARSER(indigo_trace("XML Parser: '%c' ATTRIBUTE_VALUE -> ATTRIBUTE_NAME1", c));
				} else {
					*value_pointer++ = c;
					INDIGO_TRACE_PARSER(indigo_trace("XML Parser: '%c' ATTRIBUTE_VALUE", c));
				}
				break;
			default:
				break;
		}
	}
exit_loop:
	while (true) {
		indigo_property *property = NULL;
		int index;
		for (index = 0; index < context.count; index++) {
			property = context.properties[index];
			if (property != NULL)
				break;
		}
		if (property == NULL)
			break;
		indigo_device remote_device;
		strncpy(remote_device.name, property->device, INDIGO_NAME_SIZE);
		remote_device.version = property->version;
		indigo_property *all_properties = indigo_init_text_property(NULL, remote_device.name, "", "", "", INDIGO_OK_STATE, INDIGO_RO_PERM, 0);
		indigo_delete_property(&remote_device, all_properties, NULL);
		indigo_release_property(all_properties);
		for (; index < context.count; index++) {
			indigo_property *property = context.properties[index];
			if (property != NULL && !strncmp(remote_device.name, property->device, INDIGO_NAME_SIZE)) {
				if (property->type == INDIGO_BLOB_VECTOR) {
					for (int i = 0; i < property->count; i++) {
						void *blob = property->items[i].blob.value;
						if (blob)
							free(blob);
					}
				}
				indigo_release_property(property);
				context.properties[index] = NULL;
			}
		}
	}
	if (blob_buffer != NULL)
		free(blob_buffer);
	free(buffer);
	free(value_buffer);
	close(handle);
	indigo_log("XML Parser: parser finished");
}

char *indigo_xml_escape(char *string) {
	if (strpbrk(string, "%<>\"'")) {
		static char buffers[5][INDIGO_VALUE_SIZE];
		static int	buffer_index = 0;
		char *buffer = buffers[buffer_index = (buffer_index + 1) % 5];
		char *in = string;
		char *out = buffer;
		char c;

		while ((c = *in++) && out - buffer < INDIGO_VALUE_SIZE) {
			switch (c) {
				case '&':
					*out++ = '&';
					*out++ = 'a';
					*out++ = 'm';
					*out++ = 'p';
					*out++ = ';';
					break;
				case '<':
					*out++ = '&';
					*out++ = 'l';
					*out++ = 't';
					*out++ = ';';
					break;
				case '>':
					*out++ = '&';
					*out++ = 'g';
					*out++ = 't';
					*out++ = ';';
					break;
				case '"':
					*out++ = '&';
					*out++ = 'q';
					*out++ = 'u';
					*out++ = 'o';
					*out++ = 't';
					*out++ = ';';
					break;
				case '\'':
					*out++ = '&';
					*out++ = 'a';
					*out++ = 'p';
					*out++ = 'o';
					*out++ = 's';
					*out++ = ';';
					break;
				default:
					*out++ = c;
			}
		}
		*out = 0;
		return buffer;
	}
	return string;
}
