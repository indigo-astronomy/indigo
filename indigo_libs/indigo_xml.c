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

#include <string.h>
#include <stdint.h>
#include <stdarg.h>
#include <stdio.h>
#include <errno.h>
#include <ctype.h>
#include <assert.h>
#include <pthread.h>
#include <math.h>

#if defined(INDIGO_LINUX) || defined(INDIGO_MACOS)
#include <libgen.h>
#endif

#include <indigo/indigo_base64.h>
#include <indigo/indigo_xml.h>
#include <indigo/indigo_client_xml.h>
#include <indigo/indigo_uni_io.h>
#include <indigo/indigo_version.h>
#include <indigo/indigo_names.h>

#define BUFFER_SIZE 524288  /* BUFFER_SIZE % 4 == 0, inportant for base64 */

typedef enum PARSE_STATES {
	ERROR_STATE,
	IDLE_STATE,
	BEGIN_TAG1_STATE,
	BEGIN_TAG_STATE,
	ATTRIBUTE_NAME1_STATE,
	ATTRIBUTE_NAME_STATE,
	ATTRIBUTE_VALUE1_STATE,
	ATTRIBUTE_VALUE_STATE,
	TEXT1_STATE,
	TEXT_STATE,
	BLOB_STATE,
	BLOB_END_STATE,
	END_TAG1_STATE,
	END_TAG2_STATE,
	END_TAG_STATE,
	HEADER_STATE,
	HEADER1_STATE
} parser_state;

#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunused-variable"
#else
#pragma warning(push)
#pragma warning(disable: 4101)
#endif

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

#ifdef __clang__
#pragma clang diagnostic pop
#else
#pragma warning(pop)
#endif

static indigo_property_state parse_state(indigo_version version, char *value) {
	if (!strcmp(value, "Ok"))
		return INDIGO_OK_STATE;
	if (!strcmp(value, "Busy"))
		return INDIGO_BUSY_STATE;
	if (!strcmp(value, "Alert"))
		return INDIGO_ALERT_STATE;
	if (version == INDIGO_VERSION_LEGACY && !strcmp(value, "Idle"))
		return INDIGO_OK_STATE;
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
	indigo_property *property;
	char call_back_name[INDIGO_NAME_SIZE];
	char call_back_url[INDIGO_NAME_SIZE];
	indigo_device *device;
	indigo_client *client;
	int count;
	indigo_property **properties;
	pthread_mutex_t mutex;
} parser_context;

bool indigo_use_blob_urls = true;

typedef void *(* parser_handler)(parser_state state, parser_context *context, char *name, char *value, char *message);

static void *top_level_handler(parser_state state, parser_context *context, char *name, char *value, char *message);
static void *new_text_vector_handler(parser_state state, parser_context *context, char *name, char *value, char *message);
static void *new_number_vector_handler(parser_state state, parser_context *context, char *name, char *value, char *message);
static void *new_switch_vector_handler(parser_state state, parser_context *context, char *name, char *value, char *message);
static void *new_blob_vector_handler(parser_state state, parser_context *context, char *name, char *value, char *message);
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
	indigo_property *property = context->property;
	indigo_client *client = context->client;
	assert(client != NULL);
	INDIGO_TRACE_PARSER(indigo_trace("XML Parser: enable_blob_handler %s '%s' '%s'", parser_state_name[state], name != NULL ? name : "", value != NULL ? value : ""));
	if (state == ATTRIBUTE_VALUE_STATE) {
		if (!strcmp(name, "device")) {
			strncpy(property->device, value,INDIGO_NAME_SIZE);
		} else if (!strcmp(name, "name")) {
			indigo_copy_property_name(client ? client->version : INDIGO_VERSION_CURRENT, property, value);
		}
	} else if (state == TEXT_STATE) {
		indigo_enable_blob_mode_record *record = client->enable_blob_mode_records;
		indigo_enable_blob_mode_record *prev = NULL;
		while (record) {
			if (!strcmp(property->device, record->device) && (*record->name == 0 || !strcmp(property->name, record->name))) {
				if (prev) {
					prev->next = record->next;
					indigo_safe_free(record);
					record = prev->next;
				} else {
					client->enable_blob_mode_records = record->next;
					indigo_safe_free(record);
					record = client->enable_blob_mode_records;
				}
			} else {
				prev = record;
				record = record->next;
			}
		}
		if (strcmp(value, "Never")) {
			record = indigo_safe_malloc(sizeof(indigo_enable_blob_mode_record));
			indigo_copy_name(record->device, property->device);
			indigo_copy_name(record->name, property->name);
			if (!strcmp(value, "URL") && indigo_use_blob_urls) {
				record->mode = INDIGO_ENABLE_BLOB_URL;
			} else {
				record->mode = INDIGO_ENABLE_BLOB_ALSO;
			}
			record->next = client->enable_blob_mode_records;
			client->enable_blob_mode_records = record;
			indigo_enable_blob(client, property, record->mode);
		} else {
			indigo_enable_blob(client, property, INDIGO_ENABLE_BLOB_NEVER);
		}
	} else if (state == END_TAG_STATE) {
		indigo_clear_property(property);
		return top_level_handler;
	}
	return enable_blob_handler;
}

static void *get_properties_handler(parser_state state, parser_context *context, char *name, char *value, char *message) {
	indigo_property *property = (indigo_property *)context->property;
	indigo_client *client = context->client;
	assert(client != NULL);
	INDIGO_TRACE_PARSER(indigo_trace("XML Parser: get_properties_handler %s '%s' '%s'", parser_state_name[state], name != NULL ? name : "", value != NULL ? value : ""));
	if (state == ATTRIBUTE_VALUE_STATE) {
		if (!strcmp(name, "version")) {
			indigo_version version = INDIGO_VERSION_CURRENT;
			if (!strncmp(value, "1.", 2))
				version = INDIGO_VERSION_LEGACY;
			else if (!strcmp(value, "2.0"))
				version = INDIGO_VERSION_2_0;
			else if (!strcmp(value, "3.0"))
				version = INDIGO_VERSION_3_0;
			client->version = version;
		} else if (!strcmp(name, "switch")) {
			indigo_version version = INDIGO_VERSION_CURRENT;
			if (!strncmp(value, "1.", 2))
				version = INDIGO_VERSION_LEGACY;
			else if (!strcmp(value, "2.0"))
				version = INDIGO_VERSION_2_0;
			else if (!strcmp(value, "3.0"))
				version = INDIGO_VERSION_3_0;
			if (version > client->version) {
				assert(client->client_context != NULL);
				indigo_uni_handle **handle = ((indigo_adapter_context *)(client->client_context))->output;
				indigo_uni_printf(*handle, "<switchProtocol version='%d.%d'/>\n", (version >> 8) & 0xFF, version & 0xFF);
				client->version = version;
			}
		} else if (!strcmp(name, "device")) {
			indigo_copy_name(property->device, value);
		} else if (!strcmp(name, "name")) {
			indigo_copy_property_name(client->version, property, value);;
		} else if (!strcmp(name, "client")) {
			indigo_copy_name(client->name, value);
		}
	} else if (state == END_TAG_STATE) {
		if (client->version == INDIGO_VERSION_NONE) {
			client->version = INDIGO_VERSION_LEGACY;
		}
		indigo_enumerate_properties(client, property);
		indigo_clear_property(property);
		return top_level_handler;
	}
	return get_properties_handler;
}

struct call_back_data {
	indigo_uni_handle **handle;
	char call_back_name[INDIGO_NAME_SIZE];
	char call_back_url[INDIGO_NAME_SIZE];
};

static void *call_back_client_handler(struct call_back_data *data) {
	indigo_device *protocol_adapter = indigo_xml_client_adapter(data->call_back_name, data->call_back_url, data->handle, data->handle);
	indigo_attach_device(protocol_adapter);
	indigo_xml_parse(protocol_adapter, NULL);
	((indigo_adapter_context *)(protocol_adapter->device_context))->output = ((indigo_adapter_context *)(protocol_adapter->device_context))->input = NULL;
	indigo_detach_device(protocol_adapter);
	indigo_release_xml_client_adapter(protocol_adapter);
	indigo_safe_free(data);
	return NULL;
}

static void *call_back_handler(parser_state state, parser_context *context, char *name, char *value, char *message) {
	indigo_client *client = context->client;
	assert(client != NULL);
	if (state == ATTRIBUTE_VALUE_STATE) {
		if (!strcmp(name, "name")) {
			indigo_copy_name(context->call_back_name, value);
		} else if (!strcmp(name, "url")) {
			indigo_copy_name(context->call_back_url, value);
		}
	} else if (state == END_TAG_STATE) {
		pthread_t call_back_thread;
		struct call_back_data *data = indigo_safe_malloc(sizeof(struct call_back_data));
		data->handle = ((indigo_adapter_context *)(context->client->client_context))->output;
		indigo_copy_name(data->call_back_name, context->call_back_name);
		indigo_copy_name(data->call_back_url, context->call_back_url);
		pthread_create(&call_back_thread, NULL, (void * (*)(void*))call_back_client_handler, data);
		return top_level_handler;
	}
	return call_back_handler;
}

static void *new_one_text_vector_handler(parser_state state, parser_context *context, char *name, char *value, char *message) {
	indigo_property *property = (indigo_property *)context->property;
	indigo_client *client = context->client;
	INDIGO_TRACE_PARSER(indigo_trace("XML Parser: new_one_text_vector_handler %s '%s' '%s'", parser_state_name[state], name != NULL ? name : "", value != NULL ? value : ""));
	if (state == ATTRIBUTE_VALUE_STATE) {
		if (!strcmp(name, "name")) {
			indigo_copy_item_name(client ? client->version : INDIGO_VERSION_CURRENT, property, property->items + property->count - 1, value);
		}
	} else if (state == TEXT_STATE) {
		indigo_set_text_item_value(property->items + property->count - 1, value);
	} else if (state == END_TAG_STATE) {
		return new_text_vector_handler;
	}
	return new_one_text_vector_handler;
}

static void *new_text_vector_handler(parser_state state, parser_context *context, char *name, char *value, char *message) {
	indigo_property *property = (indigo_property *)context->property;
	indigo_client *client = context->client;
	INDIGO_TRACE_PARSER(indigo_trace("XML Parser: new_text_vector_handler %s '%s' '%s'", parser_state_name[state], name != NULL ? name : "", value != NULL ? value : ""));
	if (state == BEGIN_TAG_STATE) {
		if (!strcmp(name, "oneText")) {
			context->property = indigo_resize_property(property, property->count + 1);
			return new_one_text_vector_handler;
		}
	} else if (state == ATTRIBUTE_VALUE_STATE) {
		if (!strcmp(name, "device")) {
			strncpy(property->device, value, INDIGO_NAME_SIZE);
		} else if (!strcmp(name, "name")) {
			indigo_copy_property_name(client ? client->version : INDIGO_VERSION_CURRENT, property, value);
		} else if (!strcmp(name, "token")) {
			property->access_token = strtol(value, NULL, 16);
		}
	} else if (state == END_TAG_STATE) {
		indigo_change_property(client, property);
		indigo_clear_property(property);
		return top_level_handler;
	}
	return new_text_vector_handler;
}

static void *new_one_number_vector_handler(parser_state state, parser_context *context, char *name, char *value, char *message) {
	indigo_property *property = (indigo_property *)context->property;
	indigo_client *client = context->client;
	INDIGO_TRACE_PARSER(indigo_trace("XML Parser: new_one_number_vector_handler %s '%s' '%s'", parser_state_name[state], name != NULL ? name : "", value != NULL ? value : ""));
	if (state == ATTRIBUTE_VALUE_STATE) {
		if (!strcmp(name, "name")) {
			indigo_copy_item_name(client ? client->version : INDIGO_VERSION_CURRENT, property, property->items + property->count - 1, value);
		}
	} else if (state == TEXT_STATE) {
		property->items[property->count - 1].number.value = indigo_atod(value);
	} else if (state == END_TAG_STATE) {
		return new_number_vector_handler;
	}
	return new_one_number_vector_handler;
}

static void *new_number_vector_handler(parser_state state, parser_context *context, char *name, char *value, char *message) {
	indigo_property *property = (indigo_property *)context->property;
	indigo_client *client = context->client;
	INDIGO_TRACE_PARSER(indigo_trace("XML Parser: new_number_vector_handler %s '%s' '%s'", parser_state_name[state], name != NULL ? name : "", value != NULL ? value : ""));
	if (state == BEGIN_TAG_STATE) {
		if (!strcmp(name, "oneNumber")) {
			context->property = indigo_resize_property(property, property->count + 1);
			return new_one_number_vector_handler;
		}
	} else if (state == ATTRIBUTE_VALUE_STATE) {
		if (!strcmp(name, "device")) {
			strncpy(property->device, value,INDIGO_NAME_SIZE);
		} else if (!strcmp(name, "name")) {
			indigo_copy_property_name(client ? client->version : INDIGO_VERSION_CURRENT, property, value);
		} else if (!strcmp(name, "token")) {
			property->access_token = strtol(value, NULL, 16);
		}
	} else if (state == END_TAG_STATE) {
		indigo_change_property(client, property);
		indigo_clear_property(property);
		return top_level_handler;
	}
	return new_number_vector_handler;
}

static void *new_one_switch_vector_handler(parser_state state, parser_context *context, char *name, char *value, char *message) {
	indigo_property *property = (indigo_property *)context->property;
	indigo_client *client = context->client;
	INDIGO_TRACE_PARSER(indigo_trace("XML Parser: new_one_switch_vector_handler %s '%s' '%s'", parser_state_name[state], name != NULL ? name : "", value != NULL ? value : ""));
	if (state == ATTRIBUTE_VALUE_STATE) {
		if (!strcmp(name, "name")) {
			indigo_copy_item_name(client ? client->version : INDIGO_VERSION_CURRENT, property, property->items + property->count - 1, value);
		}
	} else if (state == TEXT_STATE) {
		property->items[property->count - 1].sw.value = !strcmp(value, "On");
	} else if (state == END_TAG_STATE) {
		return new_switch_vector_handler;
	}
	return new_one_switch_vector_handler;
}

static void *new_switch_vector_handler(parser_state state, parser_context *context, char *name, char *value, char *message) {
	indigo_property *property = (indigo_property *)context->property;
	indigo_client *client = context->client;
	INDIGO_TRACE_PARSER(indigo_trace("XML Parser: new_switch_vector_handler %s '%s' '%s'", parser_state_name[state], name != NULL ? name : "", value != NULL ? value : ""));
	if (state == BEGIN_TAG_STATE) {
		if (!strcmp(name, "oneSwitch")) {
			context->property = indigo_resize_property(property, property->count + 1);
			return new_one_switch_vector_handler;
		}
	} else if (state == ATTRIBUTE_VALUE_STATE) {
		if (!strcmp(name, "device")) {
			strncpy(property->device, value,INDIGO_NAME_SIZE);
		} else if (!strcmp(name, "name")) {
			indigo_copy_property_name(client ? client->version : INDIGO_VERSION_CURRENT, property, value);
		} else if (!strcmp(name, "token")) {
			property->access_token = strtol(value, NULL, 16);
		}
		return new_switch_vector_handler;
	} else if (state == END_TAG_STATE) {
		indigo_change_property(client, property);
		indigo_clear_property(property);
		return top_level_handler;
	}
	return new_switch_vector_handler;
}

static void *new_one_blob_vector_handler(parser_state state, parser_context *context, char *name, char *value, char *message) {
	indigo_property *property = (indigo_property *)context->property;
	indigo_client *client = context->client;
	INDIGO_TRACE_PARSER(indigo_trace("XML Parser: new_one_blob_vector_handler %s '%s' '%s'", parser_state_name[state], name != NULL ? name : "", value != NULL ? value : ""));
	if (state == ATTRIBUTE_VALUE_STATE) {
		if (!strcmp(name, "name")) {
			indigo_copy_item_name(client ? client->version : INDIGO_VERSION_CURRENT, property, property->items + property->count - 1, value);
		} else if (!strcmp(name, "format")) {
			indigo_copy_name((property->items + property->count - 1)->blob.format, value);
		}
	} else if (state == END_TAG_STATE) {
		return new_blob_vector_handler;
	}
	return new_one_blob_vector_handler;
}

static void *new_blob_vector_handler(parser_state state, parser_context *context, char *name, char *value, char *message) {
	indigo_property *property = (indigo_property *)context->property;
	indigo_client *client = context->client;
	INDIGO_TRACE_PARSER(indigo_trace("XML Parser: new_blob_vector_handler %s '%s' '%s'", parser_state_name[state], name != NULL ? name : "", value != NULL ? value : ""));
	if (state == BEGIN_TAG_STATE) {
		if (!strcmp(name, "oneBLOB")) {
			context->property = indigo_resize_property(property, property->count + 1);
			return new_one_blob_vector_handler;
		}
	} else if (state == ATTRIBUTE_VALUE_STATE) {
		if (!strcmp(name, "device")) {
			strncpy(property->device, value,INDIGO_NAME_SIZE);
		} else if (!strcmp(name, "name")) {
			indigo_copy_property_name(client ? client->version : INDIGO_VERSION_CURRENT, property, value);
		} else if (!strcmp(name, "token")) {
			property->access_token = strtol(value, NULL, 16);
		}
	} else if (state == END_TAG_STATE) {
		for (int i = 0; i < property->count; i++) {
			indigo_item *item = property->items + i;
			indigo_blob_entry *entry = indigo_find_blob(property, item);
			if (entry) {
				item->blob.value = indigo_safe_malloc_copy(item->blob.size = entry->size, entry->content);
			}
		}
		property->perm = INDIGO_WO_PERM;
		indigo_change_property(client, property);
		indigo_clear_property(property);
		return top_level_handler;
	}
	return new_blob_vector_handler;
}

static void *switch_protocol_handler(parser_state state, parser_context *context, char *name, char *value, char *message) {
	indigo_device *device = context->device;
	assert(device != NULL);
	INDIGO_TRACE_PARSER(indigo_trace("XML Parser: switch_protocol_handler %s '%s' '%s'", parser_state_name[state], name != NULL ? name : "", value != NULL ? value : ""));
	if (state == ATTRIBUTE_VALUE_STATE) {
		if (!strcmp(name, "version")) {
			int major, minor;
			sscanf(value, "%d.%d", &major, &minor);
			device->version = major << 8 | minor;
		}
	} else if (state == END_TAG_STATE) {
		indigo_uni_handle **output = ((indigo_adapter_context *)(context->device->device_context))->output;
		indigo_uni_handle **input = ((indigo_adapter_context *)(context->device->device_context))->input;
		if (input == output) {
//			TODO: Finish callBack support
//			indigo_uni_printf(*output, "<callBack name='%s' url='http://%s:7624'/>\n", indigo_local_service_name, indigo_local_service_name);
		}
		return top_level_handler;
	}
	return switch_protocol_handler;
}

static void set_property(parser_context *context, indigo_property *other, char *message) {
	pthread_mutex_lock(&context->mutex);
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
								indigo_set_text_item_value(property_item, indigo_get_text_item_value(other_item));
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
									//property_item->number.value = property_item->number.min;
									if (strcmp(property->name, CCD_EXPOSURE_PROPERTY_NAME))
										indigo_debug("%s.%s value out of range", property->name, property_item->name);
								}
								if (property_item->number.value > property_item->number.max) {
									//property_item->number.value = property_item->number.max;
									if (strcmp(property->name, CCD_EXPOSURE_PROPERTY_NAME))
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
								indigo_copy_name(property_item->blob.format, other_item->blob.format);
								indigo_copy_value(property_item->blob.url, other_item->blob.url);
								if (property->perm == INDIGO_RO_PERM) {
									property_item->blob.size = other_item->blob.size;
									if (other_item->blob.value) {
										if (property_item->blob.value != NULL) {
											property_item->blob.value = indigo_safe_realloc(property_item->blob.value, property_item->blob.size);
										} else {
											property_item->blob.value = indigo_safe_malloc(property_item->blob.size);
										}
										memcpy(property_item->blob.value, other_item->blob.value, property_item->blob.size);
									} else {
										if (property_item->blob.value != NULL) {
											indigo_safe_free(property_item->blob.value);
											property_item->blob.value = NULL;
										}
										char *ext = strrchr(property_item->blob.url, '.');
										if (ext) {
											strcpy(property_item->blob.format, ext);
										}
									}
								}
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
	pthread_mutex_unlock(&context->mutex);
}

static void *set_one_text_vector_handler(parser_state state, parser_context *context, char *name, char *value, char *message) {
	indigo_property *property = (indigo_property *)context->property;
	indigo_device *device = context->device;
	INDIGO_TRACE_PARSER(indigo_trace("XML Parser: set_one_text_vector_handler %s '%s' '%s'", parser_state_name[state], name != NULL ? name : "", value != NULL ? value : ""));
	if (state == ATTRIBUTE_VALUE_STATE) {
		if (!strcmp(name, "name")) {
			indigo_copy_item_name(device->version, property, property->items + property->count - 1, value);
		}
	} else if (state == TEXT_STATE) {
		indigo_set_text_item_value(property->items + property->count - 1, value);
	} else if (state == END_TAG_STATE) {
		return set_text_vector_handler;
	}
	return set_one_text_vector_handler;
}

static void *set_text_vector_handler(parser_state state, parser_context *context, char *name, char *value, char *message) {
	indigo_property *property = (indigo_property *)context->property;
	indigo_device *device = context->device;
	INDIGO_TRACE_PARSER(indigo_trace("XML Parser: set_text_vector_handler %s '%s' '%s'", parser_state_name[state], name != NULL ? name : "", value != NULL ? value : ""));
	if (state == BEGIN_TAG_STATE) {
		if (!strcmp(name, "oneText")) {
			context->property = indigo_resize_property(property, property->count + 1);
			return set_one_text_vector_handler;
		}
	} else if (state == ATTRIBUTE_VALUE_STATE) {
		if (!strcmp(name, "device")) {
			if (indigo_use_host_suffix) {
				snprintf(property->device, INDIGO_NAME_SIZE, "%s %s", value, context->device->name);
			} else {
				indigo_copy_name(property->device, value);
			}
		} else if (!strcmp(name, "name")) {
			indigo_copy_property_name(device->version, property, value);
		} else if (!strcmp(name, "state")) {
			property->state = parse_state(device->version, value);
		} else if (!strcmp(name, "message")) {
			indigo_copy_value(message, value);
		}
	} else if (state == END_TAG_STATE) {
		set_property(context, property, message);
		indigo_clear_property(property);
		return top_level_handler;
	}
	return set_text_vector_handler;
}

static void *set_one_number_vector_handler(parser_state state, parser_context *context, char *name, char *value, char *message) {
	indigo_property *property = (indigo_property *)context->property;
	indigo_device *device = context->device;
	INDIGO_TRACE_PARSER(indigo_trace("XML Parser: set_one_number_vector_handler %s '%s' '%s'", parser_state_name[state], name != NULL ? name : "", value != NULL ? value : ""));
	if (state == ATTRIBUTE_VALUE_STATE) {
		if (!strcmp(name, "name")) {
			indigo_copy_item_name(device->version, property, property->items + property->count - 1, value);
		} else if (!strcmp(name, "target")) {
			property->items[property->count - 1].number.target = indigo_atod(value);
		} else if (!strcmp(name, "min")) {
			property->items[property->count - 1].number.min = indigo_atod(value);
		} else if (!strcmp(name, "max")) {
			property->items[property->count - 1].number.max = indigo_atod(value);
		} else if (!strcmp(name, "step")) {
			property->items[property->count - 1].number.step = indigo_atod(value);
		} else if (!strcmp(name, "format")) {
			indigo_copy_name(property->items[property->count - 1].number.format, value);
		}
	} else if (state == TEXT_STATE) {
		property->items[property->count - 1].number.value = indigo_atod(value);
	} else if (state == END_TAG_STATE) {
		return set_number_vector_handler;
	}
	return set_one_number_vector_handler;
}

static void *set_number_vector_handler(parser_state state, parser_context *context, char *name, char *value, char *message) {
	indigo_property *property = (indigo_property *)context->property;
	indigo_device *device = context->device;
	INDIGO_TRACE_PARSER(indigo_trace("XML Parser: set_number_vector_handler %s '%s' '%s'", parser_state_name[state], name != NULL ? name : "", value != NULL ? value : ""));
	if (state == BEGIN_TAG_STATE) {
		if (!strcmp(name, "oneNumber")) {
			property = context->property = indigo_resize_property(property, property->count + 1);
			property->items[property->count - 1].number.min = NAN;
			property->items[property->count - 1].number.max = NAN;
			property->items[property->count - 1].number.step = NAN;
			return set_one_number_vector_handler;
		}
	} else if (state == ATTRIBUTE_VALUE_STATE) {
		if (!strcmp(name, "device")) {
			if (indigo_use_host_suffix) {
				snprintf(property->device, INDIGO_NAME_SIZE, "%s %s", value, context->device->name);
			} else {
				indigo_copy_name(property->device, value);
			}
		} else if (!strcmp(name, "name")) {
			indigo_copy_property_name(device->version, property, value);
		} else if (!strcmp(name, "state")) {
			property->state = parse_state(device->version, value);
		} else if (!strcmp(name, "message")) {
			indigo_copy_value(message, value);
		}
	} else if (state == END_TAG_STATE) {
		set_property(context, property, message);
		indigo_clear_property(property);
		return top_level_handler;
	}
	return set_number_vector_handler;
}

static void *set_one_switch_vector_handler(parser_state state, parser_context *context, char *name, char *value, char *message) {
	indigo_property *property = (indigo_property *)context->property;
	indigo_device *device = context->device;
	INDIGO_TRACE_PARSER(indigo_trace("XML Parser: set_one_switch_vector_handler %s '%s' '%s'", parser_state_name[state], name != NULL ? name : "", value != NULL ? value : ""));
	if (state == ATTRIBUTE_VALUE_STATE) {
		if (!strcmp(name, "name")) {
			indigo_copy_item_name(device->version, property, property->items + property->count - 1, value);
			return set_one_switch_vector_handler;
		}
	} else if (state == TEXT_STATE) {
		property->items[property->count - 1].sw.value = !strcmp(value, "On");
		return set_one_switch_vector_handler;
	}
	return set_switch_vector_handler;
}

static void *set_switch_vector_handler(parser_state state, parser_context *context, char *name, char *value, char *message) {
	indigo_property *property = (indigo_property *)context->property;
	indigo_device *device = context->device;
	INDIGO_TRACE_PARSER(indigo_trace("XML Parser: set_switch_vector_handler %s '%s' '%s'", parser_state_name[state], name != NULL ? name : "", value != NULL ? value : ""));
	if (state == BEGIN_TAG_STATE) {
		if (!strcmp(name, "oneSwitch")) {
			context->property = indigo_resize_property(property, property->count + 1);
			return set_one_switch_vector_handler;
		}
	} else if (state == ATTRIBUTE_VALUE_STATE) {
		if (!strcmp(name, "device")) {
			if (indigo_use_host_suffix) {
				snprintf(property->device, INDIGO_NAME_SIZE, "%s %s", value, context->device->name);
			} else {
				indigo_copy_name(property->device, value);
			}
		} else if (!strcmp(name, "name")) {
			indigo_copy_property_name(device->version, property, value);
		} else if (!strcmp(name, "state")) {
			property->state = parse_state(device->version, value);
		} else if (!strcmp(name, "message")) {
			indigo_copy_value(message, value);
		}
	} else if (state == END_TAG_STATE) {
		set_property(context, property, message);
		indigo_clear_property(property);
		return top_level_handler;
	}
	return set_switch_vector_handler;
}

static void *set_one_light_vector_handler(parser_state state, parser_context *context, char *name, char *value, char *message) {
	indigo_property *property = (indigo_property *)context->property;
	indigo_device *device = context->device;
	INDIGO_TRACE_PARSER(indigo_trace("XML Parser: set_one_light_vector_handler %s '%s' '%s'", parser_state_name[state], name != NULL ? name : "", value != NULL ? value : ""));
	if (state == ATTRIBUTE_VALUE_STATE) {
		if (!strcmp(name, "name")) {
			indigo_copy_item_name(device->version, property, property->items + property->count - 1, value);
		}
	} else if (state == TEXT_STATE) {
		property->items[property->count - 1].light.value = parse_state(INDIGO_VERSION_CURRENT, value);
	} else if (state == END_TAG_STATE) {
		return set_light_vector_handler;
	}
	return set_one_light_vector_handler;
}

static void *set_light_vector_handler(parser_state state, parser_context *context, char *name, char *value, char *message) {
	indigo_property *property = (indigo_property *)context->property;
	indigo_device *device = context->device;
	INDIGO_TRACE_PARSER(indigo_trace("XML Parser: set_light_vector_handler %s '%s' '%s'", parser_state_name[state], name != NULL ? name : "", value != NULL ? value : ""));
	if (state == BEGIN_TAG_STATE) {
		if (!strcmp(name, "oneLight")) {
			context->property = indigo_resize_property(property, property->count + 1);
			return set_one_light_vector_handler;
		}
	} else if (state == ATTRIBUTE_VALUE_STATE) {
		if (!strcmp(name, "device")) {
			if (indigo_use_host_suffix) {
				snprintf(property->device, INDIGO_NAME_SIZE, "%s %s", value, context->device->name);
			} else {
				indigo_copy_name(property->device, value);
			}
		} else if (!strcmp(name, "name")) {
			indigo_copy_property_name(device->version, property, value);
		} else if (!strcmp(name, "state")) {
			property->state = parse_state(device->version, value);
		} else if (!strcmp(name, "message")) {
			indigo_copy_value(message, value);
		}
	} else if (state == END_TAG_STATE) {
		set_property(context, property, message);
		indigo_clear_property(property);
		return top_level_handler;
	}
	return set_light_vector_handler;
}

static void *set_one_blob_vector_handler(parser_state state, parser_context *context, char *name, char *value, char *message) {
	indigo_property *property = (indigo_property *)context->property;
	indigo_device *device = context->device;
	INDIGO_DEBUG_PROTOCOL(if (state == BLOB_STATE))
	INDIGO_TRACE_PARSER(indigo_trace("XML Parser: set_one_blob_vector_handler %s '%s' DATA", parser_state_name[state], name != NULL ? name : ""));
	INDIGO_DEBUG_PROTOCOL(else)
	INDIGO_TRACE_PARSER(indigo_trace("XML Parser: set_one_blob_vector_handler %s '%s' '%s'", parser_state_name[state], name != NULL ? name : "", value != NULL ? value : ""));
	if (state == ATTRIBUTE_VALUE_STATE) {
		if (!strcmp(name, "name")) {
			indigo_copy_item_name(device->version, property, property->items + property->count - 1, value);
		} else if (!strcmp(name, "format")) {
			indigo_copy_name(property->items[property->count - 1].blob.format, value);
		} else if (!strcmp(name, "size")) {
			property->items[property->count - 1].blob.size = atol(value);
		} else if (!strcmp(name, "path")) {
			snprintf(property->items[property->count - 1].blob.url, INDIGO_VALUE_SIZE, "%s%s", ((indigo_adapter_context *)context->device->device_context)->url_prefix, value);
		} else if (!strcmp(name, "url")) {
			indigo_copy_value(property->items[property->count - 1].blob.url, value);
		}
	} else if (state == BLOB_STATE) {
		property->items[property->count - 1].blob.value = value;
	} else if (state == END_TAG_STATE) {
		return set_blob_vector_handler;
	}
	return set_one_blob_vector_handler;
}

static void *set_blob_vector_handler(parser_state state, parser_context *context, char *name, char *value, char *message) {
	indigo_property *property = (indigo_property *)context->property;
	indigo_device *device = context->device;
	INDIGO_TRACE_PARSER(indigo_trace("XML Parser: set_blob_vector_handler %s '%s' '%s'", parser_state_name[state], name != NULL ? name : "", value != NULL ? value : ""));
	if (state == BEGIN_TAG_STATE) {
		if (!strcmp(name, "oneBLOB")) {
			context->property = indigo_resize_property(property, property->count + 1);
			return set_one_blob_vector_handler;
		}
	} else if (state == ATTRIBUTE_VALUE_STATE) {
		if (!strcmp(name, "device")) {
			if (indigo_use_host_suffix) {
				snprintf(property->device, INDIGO_NAME_SIZE, "%s %s", value, context->device->name);
			} else {
				indigo_copy_name(property->device, value);
			}
		} else if (!strcmp(name, "name")) {
			indigo_copy_property_name(device->version, property, value);
		} else if (!strcmp(name, "state")) {
			property->state = parse_state(device->version, value);
		} else if (!strcmp(name, "message")) {
			indigo_copy_value(message, value);
		}
	} else if (state == END_TAG_STATE) {
		set_property(context, property, message);
		indigo_clear_property(property);
		return top_level_handler;
	}
	return set_blob_vector_handler;
}

static void def_property(parser_context *context, indigo_property *other, char *message) {
	indigo_property *property = NULL;
	int index;
	pthread_mutex_lock(&context->mutex);
	for (index = 0; index < context->count; index++) {
		property = context->properties[index];
		if (property == NULL) {
			break;
		}
		if (!strncmp(property->device, other->device, INDIGO_NAME_SIZE) && !strncmp(property->name, other->name, INDIGO_NAME_SIZE))
			break;
	}
	if (index == context->count) {
		context->properties = indigo_safe_realloc(context->properties, context->count * 2 * sizeof(indigo_property *));
		memset(context->properties + context->count, 0, context->count * sizeof(indigo_property *));
		context->count *= 2;
		property = NULL;
	}
	if (property == NULL) {
		switch (other->type) {
			case INDIGO_TEXT_VECTOR:
				property = indigo_init_text_property(property, other->device, other->name, other->group, other->label, other->state, other->perm, other->count);
				memcpy(property->items, other->items, other->count * sizeof(indigo_item));
				indigo_copy_value(property->hints, other->hints);
				for (int i = 0; i < property->count; i++) {
					indigo_item *property_item = property->items + i;
					indigo_item *other_item = other->items + i;
					if (other_item->text.long_value) {
						property_item->text.long_value = indigo_safe_malloc_copy(property_item->text.length = other_item->text.length, other_item->text.long_value);
					}
				}
				break;
			case INDIGO_NUMBER_VECTOR:
				property = indigo_init_number_property(property, other->device, other->name, other->group, other->label, other->state, other->perm, other->count);
				memcpy(property->items, other->items, other->count * sizeof(indigo_item));
				indigo_copy_value(property->hints, other->hints);
				break;
			case INDIGO_SWITCH_VECTOR:
				property = indigo_init_switch_property(property, other->device, other->name, other->group, other->label, other->state, other->perm, other->rule, other->count);
				memcpy(property->items, other->items, other->count * sizeof(indigo_item));
				indigo_copy_value(property->hints, other->hints);
				break;
			case INDIGO_LIGHT_VECTOR:
				property = indigo_init_light_property(property, other->device, other->name, other->group, other->label, other->state, other->count);
				memcpy(property->items, other->items, other->count * sizeof(indigo_item));
				indigo_copy_value(property->hints, other->hints);
				break;
			case INDIGO_BLOB_VECTOR:
				property = indigo_init_blob_property_p(property, other->device, other->name, other->group, other->label, other->state, other->perm, other->count);
				memcpy(property->items, other->items, other->count * sizeof(indigo_item));
				indigo_copy_value(property->hints, other->hints);
				for (int i = 0; i < property->count; i++) {
					indigo_item *item = property->items + i;
					item->blob.value = NULL;
					item->blob.size = 0;
				}
				break;
		}
		context->properties[index] = property;
	}
	INDIGO_TRACE_PARSER(indigo_trace("XML Parser: def_property '%s' '%s' %d", property->device, property->name, index));
	indigo_define_property(context->device, property, *message ? message : NULL);
	pthread_mutex_unlock(&context->mutex);
}

static void *def_text_handler(parser_state state, parser_context *context, char *name, char *value, char *message) {
	indigo_property *property = (indigo_property *)context->property;
	indigo_device *device = context->device;
	INDIGO_TRACE_PARSER(indigo_trace("XML Parser: def_text_handler %s '%s' '%s'", parser_state_name[state], name != NULL ? name : "", value != NULL ? value : ""));
	if (state == ATTRIBUTE_VALUE_STATE) {
		if (!strcmp(name, "name")) {
			indigo_copy_item_name(device->version, property, property->items + property->count - 1, value);
		} else if (!strcmp(name, "label")) {
			indigo_copy_value(property->items[property->count - 1].label, value);
		} else if (!strcmp(name, "hints")) {
			indigo_copy_value(property->items[property->count - 1].hints, value);
		}
	} else if (state == TEXT_STATE) {
		indigo_set_text_item_value(property->items + property->count - 1, value);
	} else if (state == END_TAG_STATE) {
		return def_text_vector_handler;
	}
	return def_text_handler;
}

static void *def_text_vector_handler(parser_state state, parser_context *context, char *name, char *value, char *message) {
	indigo_property *property = (indigo_property *)context->property;
	indigo_device *device = context->device;
	INDIGO_TRACE_PARSER(indigo_trace("XML Parser: def_text_vector_handler %s '%s' '%s'", parser_state_name[state], name != NULL ? name : "", value != NULL ? value : ""));
	if (state == BEGIN_TAG_STATE) {
		if (!strcmp(name, "defText")) {
			context->property = indigo_resize_property(property, property->count + 1);
			return def_text_handler;
		}
	} else if (state == ATTRIBUTE_VALUE_STATE) {
		if (!strcmp(name, "device")) {
			if (indigo_use_host_suffix) {
				snprintf(property->device, INDIGO_NAME_SIZE, "%s %s", value, context->device->name);
			} else {
				indigo_copy_name(property->device, value);
			}
		} else if (!strcmp(name, "name")) {
			indigo_copy_property_name(device->version, property, value);
		} else if (!strcmp(name, "group")) {
			strncpy(property->group, value,INDIGO_NAME_SIZE);
		} else if (!strcmp(name, "label")) {
			indigo_copy_value(property->label, value);
		} else if (!strcmp(name, "hints")) {
			indigo_copy_value(property->hints, value);
		} else if (!strcmp(name, "state")) {
			property->state = parse_state(device->version, value);
		} else if (!strcmp(name, "perm")) {
			property->perm = parse_perm(value);
		} else if (!strcmp(name, "message")) {
			indigo_copy_value(message, value);
		}
	} else if (state == END_TAG_STATE) {
		def_property(context, property, message);
		indigo_clear_property(property);
		return top_level_handler;
	}
	return def_text_vector_handler;
}

static void *def_number_handler(parser_state state, parser_context *context, char *name, char *value, char *message) {
	indigo_property *property = (indigo_property *)context->property;
	indigo_device *device = context->device;
	INDIGO_TRACE_PARSER(indigo_trace("XML Parser: def_number_handler %s '%s' '%s'", parser_state_name[state], name != NULL ? name : "", value != NULL ? value : ""));
	if (state == ATTRIBUTE_VALUE_STATE) {
		if (!strcmp(name, "name")) {
			indigo_copy_item_name(device->version, property, property->items + property->count - 1, value);
		} else if (!strcmp(name, "target")) {
			property->items[property->count - 1].number.target = indigo_atod(value);
		} else if (!strcmp(name, "label")) {
			indigo_copy_value(property->items[property->count - 1].label, value);
		} else if (!strcmp(name, "hints")) {
			indigo_copy_value(property->items[property->count - 1].hints, value);
		} else if (!strcmp(name, "min")) {
			property->items[property->count - 1].number.min = indigo_atod(value);
		} else if (!strcmp(name, "max")) {
			property->items[property->count - 1].number.max = indigo_atod(value);
		} else if (!strcmp(name, "step")) {
			property->items[property->count - 1].number.step = indigo_atod(value);
		} else if (!strcmp(name, "format")) {
			indigo_copy_name(property->items[property->count - 1].number.format, value);
		}
	} else if (state == TEXT_STATE) {
		property->items[property->count - 1].number.value = indigo_atod(value);
	} else if (state == END_TAG_STATE) {
		return def_number_vector_handler;
	}
	return def_number_handler;
}

static void *def_number_vector_handler(parser_state state, parser_context *context, char *name, char *value, char *message) {
	indigo_property *property = (indigo_property *)context->property;
	indigo_device *device = context->device;
	INDIGO_TRACE_PARSER(indigo_trace("XML Parser: def_number_vector_handler %s '%s' '%s'", parser_state_name[state], name != NULL ? name : "", value != NULL ? value : ""));
	if (state == BEGIN_TAG_STATE) {
		if (!strcmp(name, "defNumber")) {
			context->property = indigo_resize_property(property, property->count + 1);
			return def_number_handler;
		}
	} else if (state == ATTRIBUTE_VALUE_STATE) {
		if (!strcmp(name, "device")) {
			if (indigo_use_host_suffix) {
				snprintf(property->device, INDIGO_NAME_SIZE, "%s %s", value, context->device->name);
			} else {
				indigo_copy_name(property->device, value);
			}
		} else if (!strcmp(name, "name")) {
			indigo_copy_property_name(device->version, property, value);
		} else if (!strcmp(name, "group")) {
			strncpy(property->group, value,INDIGO_NAME_SIZE);
		} else if (!strcmp(name, "label")) {
			indigo_copy_value(property->label, value);
		} else if (!strcmp(name, "hints")) {
			indigo_copy_value(property->hints, value);
		} else if (!strcmp(name, "state")) {
			property->state = parse_state(device->version, value);
		} else if (!strcmp(name, "perm")) {
			property->perm = parse_perm(value);
		} else if (!strcmp(name, "message")) {
			indigo_copy_value(message, value);
		}
	} else if (state == END_TAG_STATE) {
		def_property(context, property, message);
		indigo_clear_property(property);
		return top_level_handler;
	}
	return def_number_vector_handler;
}

static void *def_switch_handler(parser_state state, parser_context *context, char *name, char *value, char *message) {
	indigo_property *property = (indigo_property *)context->property;
	indigo_device *device = context->device;
	INDIGO_TRACE_PARSER(indigo_trace("XML Parser: def_switch_handler %s '%s' '%s'", parser_state_name[state], name != NULL ? name : "", value != NULL ? value : ""));
	if (state == ATTRIBUTE_VALUE_STATE) {
		if (!strcmp(name, "name")) {
			indigo_copy_item_name(device->version, property, property->items + property->count - 1, value);
		} else if (!strcmp(name, "label")) {
			indigo_copy_value(property->items[property->count - 1].label, value);
		} else if (!strcmp(name, "hints")) {
			indigo_copy_value(property->items[property->count - 1].hints, value);
		}
	} else if (state == TEXT_STATE) {
		property->items[property->count - 1].sw.value = !strcmp(value, "On");
	} else if (state == END_TAG_STATE) {
		return def_switch_vector_handler;
	}
	return def_switch_handler;
}

static void *def_switch_vector_handler(parser_state state, parser_context *context, char *name, char *value, char *message) {
	indigo_property *property = (indigo_property *)context->property;
	indigo_device *device = context->device;
	INDIGO_TRACE_PARSER(indigo_trace("XML Parser: def_switch_vector_handler %s '%s' '%s'", parser_state_name[state], name != NULL ? name : "", value != NULL ? value : ""));
	if (state == BEGIN_TAG_STATE) {
		if (!strcmp(name, "defSwitch")) {
			context->property = indigo_resize_property(property, property->count + 1);
			return def_switch_handler;
		}
	} else if (state == ATTRIBUTE_VALUE_STATE) {
		if (!strcmp(name, "device")) {
			if (indigo_use_host_suffix) {
				snprintf(property->device, INDIGO_NAME_SIZE, "%s %s", value, context->device->name);
			} else {
				indigo_copy_name(property->device, value);
			}
		} else if (!strcmp(name, "name")) {
			indigo_copy_property_name(device->version, property, value);
		} else if (!strcmp(name, "group")) {
			strncpy(property->group, value,INDIGO_NAME_SIZE);
		} else if (!strcmp(name, "label")) {
			indigo_copy_value(property->label, value);
		} else if (!strcmp(name, "hints")) {
			indigo_copy_value(property->hints, value);
		} else if (!strcmp(name, "state")) {
			property->state = parse_state(device->version, value);
		} else if (!strcmp(name, "perm")) {
			property->perm = parse_perm(value);
		} else if (!strcmp(name, "rule")) {
			property->rule = parse_rule(value);
		} else if (!strcmp(name, "message")) {
			indigo_copy_value(message, value);
		}
	} else if (state == END_TAG_STATE) {
		def_property(context, property, message);
		indigo_clear_property(property);
		return top_level_handler;
	}
	return def_switch_vector_handler;
}

static void *def_light_handler(parser_state state, parser_context *context, char *name, char *value, char *message) {
	indigo_property *property = (indigo_property *)context->property;
	indigo_device *device = context->device;
	INDIGO_TRACE_PARSER(indigo_trace("XML Parser: def_light_handler %s '%s' '%s'", parser_state_name[state], name != NULL ? name : "", value != NULL ? value : ""));
	if (state == ATTRIBUTE_VALUE_STATE) {
		if (!strcmp(name, "name")) {
			indigo_copy_item_name(device->version, property, property->items + property->count - 1, value);
		} else if (!strcmp(name, "label")) {
			indigo_copy_value(property->items[property->count - 1].label, value);
		} else if (!strcmp(name, "hints")) {
			indigo_copy_value(property->items[property->count - 1].hints, value);
		}
	} else if (state == TEXT_STATE) {
		property->items[property->count - 1].light.value = parse_state(INDIGO_VERSION_CURRENT, value);
	} else if (state == END_TAG_STATE) {
		return def_light_vector_handler;
	}
	return def_light_handler;
}

static void *def_light_vector_handler(parser_state state, parser_context *context, char *name, char *value, char *message) {
	indigo_property *property = (indigo_property *)context->property;
	indigo_device *device = context->device;
	INDIGO_TRACE_PARSER(indigo_trace("XML Parser: def_light_vector_handler %s '%s' '%s'", parser_state_name[state], name != NULL ? name : "", value != NULL ? value : ""));
	if (state == BEGIN_TAG_STATE) {
		if (!strcmp(name, "defLight")) {
			context->property = indigo_resize_property(property, property->count + 1);
			return def_light_handler;
		}
	} else if (state == ATTRIBUTE_VALUE_STATE) {
		if (!strcmp(name, "device")) {
			if (indigo_use_host_suffix) {
				snprintf(property->device, INDIGO_NAME_SIZE, "%s %s", value, context->device->name);
			} else {
				indigo_copy_name(property->device, value);
			}
		} else if (!strcmp(name, "name")) {
			indigo_copy_property_name(device->version, property, value);
		} else if (!strcmp(name, "group")) {
			strncpy(property->group, value,INDIGO_NAME_SIZE);
		} else if (!strcmp(name, "label")) {
			indigo_copy_value(property->label, value);
		} else if (!strcmp(name, "hints")) {
			indigo_copy_value(property->hints, value);
		} else if (!strcmp(name, "state")) {
			property->state = parse_state(device->version, value);
		} else if (!strcmp(name, "message")) {
			indigo_copy_value(message, value);
		}
	} else if (state == END_TAG_STATE) {
		def_property(context, property, message);
		indigo_clear_property(property);
		return top_level_handler;
	}
	return def_light_vector_handler;
}

static void *def_blob_handler(parser_state state, parser_context *context, char *name, char *value, char *message) {
	indigo_property *property = (indigo_property *)context->property;
	indigo_device *device = context->device;
	INDIGO_TRACE_PARSER(indigo_trace("XML Parser: def_blob_handler %s '%s' '%s'", parser_state_name[state], name != NULL ? name : "", value != NULL ? value : ""));
	if (state == ATTRIBUTE_VALUE_STATE) {
		if (!strcmp(name, "name")) {
			indigo_copy_item_name(device->version, property, property->items + property->count - 1, value);
		} else if (!strcmp(name, "label")) {
			indigo_copy_value(property->items[property->count - 1].label, value);
		} else if (!strcmp(name, "hints")) {
			indigo_copy_value(property->items[property->count - 1].hints, value);
		} else if (!strcmp(name, "path")) {
			snprintf(property->items[property->count - 1].blob.url, INDIGO_VALUE_SIZE, "%s%s", ((indigo_adapter_context *)context->device->device_context)->url_prefix, value);
		} else if (!strcmp(name, "url")) {
			indigo_copy_value(property->items[property->count - 1].blob.url, value);
		}
	} else if (state == END_TAG_STATE) {
		return def_blob_vector_handler;
	}
	return def_blob_handler;
}

static void *def_blob_vector_handler(parser_state state, parser_context *context, char *name, char *value, char *message) {
	indigo_property *property = (indigo_property *)context->property;
	indigo_device *device = context->device;
	INDIGO_TRACE_PARSER(indigo_trace("XML Parser: def_blob_vector_handler %s '%s' '%s'", parser_state_name[state], name != NULL ? name : "", value != NULL ? value : ""));
	if (state == BEGIN_TAG_STATE) {
		if (!strcmp(name, "defBLOB")) {
			context->property = indigo_resize_property(property, property->count + 1);
			return def_blob_handler;
		}
	} else if (state == ATTRIBUTE_VALUE_STATE) {
		if (!strcmp(name, "device")) {
			if (indigo_use_host_suffix) {
				snprintf(property->device, INDIGO_NAME_SIZE, "%s %s", value, context->device->name);
			} else {
				indigo_copy_name(property->device, value);
			}
		} else if (!strcmp(name, "name")) {
			indigo_copy_property_name(device->version, property, value);
		} else if (!strcmp(name, "group")) {
			strncpy(property->group, value,INDIGO_NAME_SIZE);
		} else if (!strcmp(name, "label")) {
			indigo_copy_value(property->label, value);
		} else if (!strcmp(name, "hints")) {
			indigo_copy_value(property->hints, value);
		} else if (!strcmp(name, "state")) {
			property->state = parse_state(device->version, value);
		} else if (!strcmp(name, "perm")) {
			property->perm = parse_perm(value);
		} else if (!strcmp(name, "message")) {
			indigo_copy_value(message, value);
		}
	} else if (state == END_TAG_STATE) {
		def_property(context, property, message);
		indigo_clear_property(property);
		return top_level_handler;
	}
	return def_blob_vector_handler;
}

static void *del_property_handler(parser_state state, parser_context *context, char *name, char *value, char *message) {
	pthread_mutex_lock(&context->mutex);
	indigo_property *property = (indigo_property *)context->property;
	indigo_device *device = context->device;
	INDIGO_TRACE_PARSER(indigo_trace("XML Parser: del_property_handler %s '%s' '%s'", parser_state_name[state], name != NULL ? name : "", value != NULL ? value : ""));
	if (state == ATTRIBUTE_VALUE_STATE) {
		if (!strncmp(name, "device", INDIGO_NAME_SIZE)) {
			if (indigo_use_host_suffix) {
				snprintf(property->device, INDIGO_NAME_SIZE, "%s %s", value, context->device->name);
			} else {
				indigo_copy_name(property->device, value);
			}
		} else if (!strncmp(name, "name",INDIGO_NAME_SIZE)) {
			indigo_copy_property_name(device->version, property, value);;
		} else if (!strcmp(name, "message")) {
			indigo_copy_value(message, value);
		}
	} else if (state == END_TAG_STATE) {
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
		indigo_clear_property(property);
		pthread_mutex_unlock(&context->mutex);
		return top_level_handler;
	}
	pthread_mutex_unlock(&context->mutex);
	return del_property_handler;
}

static void *message_handler(parser_state state, parser_context *context, char *name, char *value, char *message) {
	indigo_property *property = (indigo_property *)context->property;
	indigo_device *device = context->device;
	INDIGO_TRACE_PARSER(indigo_trace("XML Parser: message_handler %s '%s' '%s'", parser_state_name[state], name != NULL ? name : "", value != NULL ? value : ""));
	if (state == ATTRIBUTE_VALUE_STATE) {
		if (!strncmp(name, "device", INDIGO_NAME_SIZE)) {
			if (indigo_use_host_suffix) {
				snprintf(message, INDIGO_NAME_SIZE, "%s %s: ", value, context->device->name);
			} else {
				snprintf(message, INDIGO_NAME_SIZE, "%s: ", value);
			}
		} else if (!strcmp(name, "message")) {
			strcat(message, value);
		}
	} else if (state == END_TAG_STATE) {
		indigo_send_message(device, *message ? message : NULL);
		indigo_clear_property(property);
		return top_level_handler;
	}
	return message_handler;
}

static void *top_level_handler(parser_state state, parser_context *context, char *name, char *value, char *message) {
	indigo_property *property = (indigo_property *)context->property;
	indigo_client *client = context->client;
  property->version = client ? client->version : INDIGO_VERSION_CURRENT;
	INDIGO_TRACE_PARSER(indigo_trace("XML Parser: top_level_handler %s '%s' '%s'", parser_state_name[state], name != NULL ? name : "", value != NULL ? value : ""));
	if (state == BEGIN_TAG_STATE) {
		*message = 0;
		if (!strcmp(name, "enableBLOB"))
			return enable_blob_handler;
		if (!strcmp(name, "getProperties") && client != NULL)
			return get_properties_handler;
// TODO: Finish callBack support
//		if (!strcmp(name, "callBack"))
//			return call_back_handler;
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
		if (!strcmp(name, "newBLOBVector")) {
			property->type = INDIGO_BLOB_VECTOR;
			return new_blob_vector_handler;
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
	char *buffer = indigo_safe_malloc(BUFFER_SIZE + 3); /* BUFFER_SIZE % 4 == 0 and keep always +3 for base64 alignmet */
	char *value_buffer = indigo_safe_malloc(BUFFER_SIZE + 1); /* +1 to accomodate \0" */
	char *name_buffer = indigo_safe_malloc(INDIGO_NAME_SIZE);
	char *message = indigo_safe_malloc(INDIGO_VALUE_SIZE);
	unsigned char *blob_buffer = NULL;
	char *pointer = buffer;
	char *buffer_end = NULL;
	char *name_pointer = name_buffer;
	char *value_pointer = value_buffer;
	unsigned char *blob_pointer = NULL;
	long blob_size = 0;
	char q = '"';
	int depth = 0;
	char c = 0;
	char entity_buffer[8];
	char *entity_pointer = NULL;
	bool is_escaped = false;
	/* (void)parser_state_name; */
	
	parser_handler handler = top_level_handler;
	
	parser_state state = IDLE_STATE;
	
	parser_context *context = indigo_safe_malloc(sizeof(parser_context));
	context->client = client;
	context->device = device;
	pthread_mutex_init(&context->mutex, NULL);
	if (device != NULL) {
		context->count = 32;
		context->properties = indigo_safe_malloc(context->count * sizeof(indigo_property *));
	} else {
		context->count = 0;
		context->properties = NULL;
	}
	
	context->property = indigo_safe_malloc(sizeof(indigo_property) + INDIGO_PREALLOCATED_COUNT * sizeof(indigo_item));
	context->property->allocated_count = INDIGO_PREALLOCATED_COUNT;
	
	indigo_uni_handle **handle = NULL;
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
		if (state == ERROR_STATE) {
			indigo_error("XML Parser: syntax error");
			goto exit_loop;
		}
		while ((c = *pointer++) == 0) {
			long count = indigo_uni_read_available(*handle, buffer, BUFFER_SIZE);
			if (count <= 0) {
				goto exit_loop;
			}
			pointer = buffer;
			buffer_end = buffer + count;
			buffer[count] = 0;
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
			case IDLE_STATE:
				if (c == '<') {
					state = BEGIN_TAG1_STATE;
					INDIGO_TRACE_PARSER(indigo_trace("XML Parser: '%c' IDLE -> BEGIN_TAG1", c));
				}
				break;
			case BEGIN_TAG1_STATE:
				if (c == '?') {
					state = HEADER_STATE;
					INDIGO_TRACE_PARSER(indigo_trace("XML Parser: '%c' BEGIN_TAG1 -> HEADER", c));
				} else {
					name_pointer = name_buffer;
					if (isalpha(c)) {
						*name_pointer++ = c;
						state = BEGIN_TAG_STATE;
						INDIGO_TRACE_PARSER(indigo_trace("XML Parser: '%c' BEGIN_TAG1 -> BEGIN_TAG", c));
					} else if (c == '/') {
						state = END_TAG_STATE;
						INDIGO_TRACE_PARSER(indigo_trace("XML Parser: '%c' BEGIN_TAG1 -> END_TAG", c));
					}
				}
				break;
			case HEADER_STATE:
				if (c == '?') {
					state = HEADER1_STATE;
					INDIGO_TRACE_PARSER(indigo_trace("XML Parser: '%c' HEADER -> HEADER1", c));
				}
				break;
			case HEADER1_STATE:
				if (c == '>') {
					state = IDLE_STATE;
					INDIGO_TRACE_PARSER(indigo_trace("XML Parser: '%c' HEADER1 -> IDLE", c));
				} else {
					state = ERROR_STATE;
					INDIGO_TRACE_PARSER(indigo_trace("XML Parser: '%c' HEADER1 -> ERROR", c));
				}
				break;
			case BEGIN_TAG_STATE:
				if (name_pointer - name_buffer <INDIGO_NAME_SIZE && isalpha(c)) {
					*name_pointer++ = c;
					INDIGO_TRACE_PARSER(indigo_trace("XML Parser: '%c' BEGIN_TAG", c));
				} else {
					*name_pointer = 0;
					depth++;
					handler = handler(BEGIN_TAG_STATE, context, name_buffer, NULL, message);
					if (isspace(c)) {
						state = ATTRIBUTE_NAME1_STATE;
						INDIGO_TRACE_PARSER(indigo_trace("XML Parser: '%c' BEGIN_TAG -> ATTRIBUTE_NAME1", c));
					} else if (c == '/') {
						state = END_TAG1_STATE;
						INDIGO_TRACE_PARSER(indigo_trace("XML Parser: '%c' BEGIN_TAG -> END_TAG1", c));
					} else if (c == '>') {
						state = TEXT_STATE;
						value_pointer = value_buffer;
						INDIGO_TRACE_PARSER(indigo_trace("XML Parser: '%c' BEGIN_TAG -> TEXT", c));
					} else {
						state = ERROR_STATE;
						INDIGO_TRACE_PARSER(indigo_trace("XML Parser: '%c' error BEGIN_TAG", c));
					}
				}
				break;
			case END_TAG1_STATE:
				if (c == '>') {
					INDIGO_TRACE_PARSER(indigo_trace("XML Parser: '%c' END_TAG1 -> IDLE", c));
					handler = handler(END_TAG_STATE, context, NULL, NULL, message);
					depth--;
					state = IDLE_STATE;
				} else {
					state = ERROR_STATE;
					INDIGO_TRACE_PARSER(indigo_trace("XML Parser: '%c' error END_TAG1", c));
				}
				break;
			case END_TAG2_STATE:
				if (c == '/') {
					state = END_TAG_STATE;
					INDIGO_TRACE_PARSER(indigo_trace("XML Parser: '%c' END_TAG2 -> END_TAG", c));
				} else {
					state = ERROR_STATE;
					INDIGO_TRACE_PARSER(indigo_trace("XML Parser: '%c' error END_TAG2", c));
				}
				break;
			case END_TAG_STATE:
				if (isalpha(c)) {
					INDIGO_TRACE_PARSER(indigo_trace("XML Parser: '%c' END_TAG", c));
				} else if (c == '>') {
					handler = handler(END_TAG_STATE, context, NULL, NULL, message);
					depth--;
					state = IDLE_STATE;
					INDIGO_TRACE_PARSER(indigo_trace("XML Parser: '%c' END_TAG -> IDLE", c));
				} else {
					state = ERROR_STATE;
					INDIGO_TRACE_PARSER(indigo_trace("XML Parser: '%c' error END_TAG", c));
				}
				break;
			case TEXT_STATE:
				if (c == '<' && !is_escaped) {
					if (depth == 2 || handler == enable_blob_handler) {
						*value_pointer-- = 0;
						while (value_pointer >= value_buffer && isspace(*value_pointer))
							*value_pointer-- = 0;
						value_pointer = value_buffer;
						while (*value_pointer && isspace(*value_pointer))
							value_pointer++;
						handler = handler(TEXT_STATE, context, NULL, value_pointer, message);
					}
					state = TEXT1_STATE;
					INDIGO_TRACE_PARSER(indigo_trace("XML Parser: '%c' %d TEXT -> TEXT1", c, depth));
					break;
				} else {
					if (depth == 2 || handler == enable_blob_handler) {
						if (value_pointer - value_buffer < BUFFER_SIZE) {
							*value_pointer++ = c;
						}
					}
					INDIGO_TRACE_PARSER(indigo_trace("XML Parser: '%c' %d TEXT", c, depth));
				}
				break;
			case TEXT1_STATE:
				if (c=='/') {
					state = END_TAG_STATE;
					INDIGO_TRACE_PARSER(indigo_trace("XML Parser: '%c' TEXT -> END_TAG", c));
				} else if (isalpha(c)) {
					name_pointer = name_buffer;
					*name_pointer++ = c;
					state = BEGIN_TAG_STATE;
					INDIGO_TRACE_PARSER(indigo_trace("XML Parser: '%c' TEXT -> BEGIN_TAG", c));
				}
				break;
			case BLOB_END_STATE:
				if (c == '<') {
					state = TEXT1_STATE;
					INDIGO_TRACE_PARSER(indigo_trace("XML Parser: '%c' BLOB_END -> TEXT1", c));
				}
				*name_pointer++ = c;
				break;
			case BLOB_STATE:
				if (device->version >= INDIGO_VERSION_2_0) {
					long count;
					pointer--;
					while (isspace(*pointer)) {
						pointer++;
					}
					unsigned long blob_len = (blob_size + 2) / 3 * 4;
					unsigned long len = (long)(buffer_end - pointer);
					len = (len < blob_len) ? len : blob_len;
					long bytes_needed = len % 4;
					if (bytes_needed) {
						bytes_needed = 4 - bytes_needed;
					}
					while (bytes_needed) {
						count = indigo_uni_read(*handle, (void *)buffer_end, bytes_needed);
						if (count <= 0) {
							goto exit_loop;
						}
						len += count;
						bytes_needed -= count;
						buffer_end += count;
					}
					blob_pointer += base64_decode_fast((unsigned char*)blob_pointer, (unsigned char*)pointer, len);
					pointer += len;
					blob_len -= len;
					while (blob_len) {
						len = ((BUFFER_SIZE) < blob_len) ? (BUFFER_SIZE) : blob_len;
						long to_read = len;
						char *ptr = buffer;
						while(to_read) {
							count = indigo_uni_read(*handle, (void *)ptr, to_read);
							if (count <= 0) {
								goto exit_loop;
							}
							ptr += count;
							to_read -= count;
						}
						blob_pointer += base64_decode_fast((unsigned char*)blob_pointer, (unsigned char*)buffer, len);
						blob_len -= len;
					}
					
					handler = handler(BLOB_STATE, context, NULL, (char *)blob_buffer, message);
					pointer = buffer;
					*pointer = 0;
					state = BLOB_END_STATE;
					INDIGO_TRACE_PARSER(indigo_trace("XML Parser: '%c' %d BLOB -> BLOB_END", c, depth));
					break;
				} else {
					if (c == '<') {
						if (depth == 2) {
							*value_pointer = 0;
							blob_pointer += base64_decode_fast((unsigned char*)blob_pointer, (unsigned char*)value_buffer, (int)(value_pointer-value_buffer));
							handler = handler(BLOB_STATE, context, NULL, (char *)blob_buffer, message);
						}
						state = TEXT1_STATE;
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
			case ATTRIBUTE_NAME1_STATE:
				if (isspace(c)) {
					INDIGO_TRACE_PARSER(indigo_trace("XML Parser: '%c' ATTRIBUTE_NAME1", c));
				} else if (isalpha(c)) {
					name_pointer = name_buffer;
					*name_pointer++ = c;
					state = ATTRIBUTE_NAME_STATE;
					INDIGO_TRACE_PARSER(indigo_trace("XML Parser: '%c' ATTRIBUTE_NAME1 -> ATTRIBUTE_NAME", c));
				} else if (c == '/') {
					state = END_TAG1_STATE;
				} else if (c == '>') {
					value_pointer = value_buffer;
					if (handler == set_one_blob_vector_handler) {
						indigo_property *property = context->property;
						blob_size = property->items[property->count - 1].blob.size;
						if (blob_size > 0) {
							state = BLOB_STATE;
							if (blob_buffer != NULL) {
								unsigned char *ptmp = indigo_safe_realloc(blob_buffer, blob_size + 3); /* +3 to handle indi - reason unknown */
								assert(ptmp != NULL);
								blob_buffer = ptmp;
							} else {
								blob_buffer = indigo_safe_malloc(blob_size + 3); /* +3 to handle indi - reason unknown */
							}
							blob_pointer = blob_buffer;
						} else {
							state = TEXT_STATE;
						}
					} else
						state = TEXT_STATE;
					INDIGO_TRACE_PARSER(indigo_trace("XML Parser: '%c' ATTRIBUTE_NAME1 -> TEXT", c));
				} else {
					state = ERROR_STATE;
					INDIGO_TRACE_PARSER(indigo_trace("XML Parser: '%c' error ATTRIBUTE_NAME1", c));
				}
				break;
			case ATTRIBUTE_NAME_STATE:
				if (name_pointer - name_buffer <INDIGO_NAME_SIZE && isalpha(c)) {
					*name_pointer++ = c;
					INDIGO_TRACE_PARSER(indigo_trace("XML Parser: '%c' ATTRIBUTE_NAME", c));
				} else {
					*name_pointer = 0;
					if (c == '=') {
						state = ATTRIBUTE_VALUE1_STATE;
						INDIGO_TRACE_PARSER(indigo_trace("XML Parser: '%c' ATTRIBUTE_NAME -> ATTRIBUTE_VALUE1", c));
					} else {
						INDIGO_TRACE_PARSER(indigo_trace("XML Parser: '%c' ATTRIBUTE_NAME", c));
					}
				}
				break;
			case ATTRIBUTE_VALUE1_STATE:
				if (c == '"' || c == '\'') {
					q = c;
					value_pointer = value_buffer;
					state = ATTRIBUTE_VALUE_STATE;
					INDIGO_TRACE_PARSER(indigo_trace("XML Parser: '%c' ATTRIBUTE_VALUE1 -> ATTRIBUTE_VALUE2", c));
				} else {
					INDIGO_TRACE_PARSER(indigo_trace("XML Parser: '%c' ATTRIBUTE_VALUE1", c));
				}
				break;
			case ATTRIBUTE_VALUE_STATE:
				if (c == q && !is_escaped) {
					*value_pointer = 0;
					state = ATTRIBUTE_NAME1_STATE;
					handler = handler(ATTRIBUTE_VALUE_STATE, context, name_buffer, value_buffer, message);
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
	pthread_mutex_lock(&context->mutex);
	while (true) {
		indigo_property *property = NULL;
		int index;
		for (index = 0; index < context->count; index++) {
			property = context->properties[index];
			if (property != NULL) {
				break;
			}
		}
		if (property == NULL) {
			break;
		}
		indigo_device remote_device;
		indigo_copy_name(remote_device.name, property->device);
		remote_device.version = property->version;
		indigo_property *all_properties = indigo_init_text_property(NULL, remote_device.name, "", "", "", INDIGO_OK_STATE, INDIGO_RO_PERM, 0);
		indigo_delete_property(&remote_device, all_properties, NULL);
		indigo_release_property(all_properties);
		for (; index < context->count; index++) {
			indigo_property *property = context->properties[index];
			if (property != NULL && !strncmp(remote_device.name, property->device, INDIGO_NAME_SIZE)) {
				indigo_release_property(property);
				context->properties[index] = NULL;
			}
		}
	}
	indigo_safe_free(blob_buffer);
	indigo_safe_free(name_buffer);
	indigo_safe_free(message);
	indigo_release_property(context->property);
	indigo_safe_free(context->properties);
	pthread_mutex_unlock(&context->mutex);
	pthread_mutex_destroy(&context->mutex);
	indigo_uni_close(handle);
	indigo_safe_free(context);
	indigo_safe_free(buffer);
	indigo_safe_free(value_buffer);
	INDIGO_TRACE_PARSER(indigo_trace("XML Parser: parser finished"));
}


#define BUFFER_COUNT	10
static char *escape_buffer[BUFFER_COUNT] = { NULL };
static long escape_buffer_size[BUFFER_COUNT] =  { 0 };
static bool free_escape_buffers_registered = false;

static void free_escape_buffers(void) {
	for (int i = 0; i < BUFFER_COUNT; i++)
		if (escape_buffer[i]) {
			indigo_safe_free(escape_buffer[i]);
		}
}

const char *indigo_xml_escape(const char *string) {
	if (strpbrk(string, "&<>\"'")) {
		if (!free_escape_buffers_registered) {
			atexit(free_escape_buffers);
			free_escape_buffers_registered = true;
		}
		long length = 5 * (long)strlen(string);
		static int	buffer_index = 0;
		int index = buffer_index = (buffer_index + 1) % BUFFER_COUNT;
		char *buffer;
		if (escape_buffer[index] == NULL) {
			escape_buffer[index] = buffer = indigo_safe_malloc(escape_buffer_size[index] = length);
		} else if (escape_buffer_size[index] < length) {
			escape_buffer[index] = buffer = indigo_safe_realloc(escape_buffer[index], escape_buffer_size[index] = length);
		} else {
			buffer = escape_buffer[index];
		}
		const char *in = string;
		char *out = buffer;
		char c;
		while ((c = *in++)) {
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

#define INDIGO_PRINTF(...) if (!indigo_uni_printf(__VA_ARGS__)) goto failure
#define RAW_BUF_SIZE 98304
#define BASE64_BUF_SIZE 131072  /* BASE64_BUF_SIZE >= (RAW_BUF_SIZE + 2) / 3 * 4 */

extern char *indigo_client_name;

static pthread_mutex_t xml_mutex = PTHREAD_MUTEX_INITIALIZER;

indigo_result indigo_xml_client_parser_enumerate_properties(indigo_device *device, indigo_client *client, indigo_property *property) {
	assert(device != NULL);
	if (!indigo_reshare_remote_devices && client && client->is_remote) {
		return INDIGO_OK;
	}
	indigo_adapter_context *device_context = (indigo_adapter_context *)device->device_context;
	if (device_context->output == NULL) {
		return INDIGO_OK;
	}
	pthread_mutex_lock(&xml_mutex);
	assert(device_context != NULL);
	indigo_uni_handle **handle = device_context->output;
	char device_name[INDIGO_NAME_SIZE];
	if (property != NULL && *property->device) {
		indigo_copy_name(device_name, property->device);
		if (indigo_use_host_suffix) {
			char *at = strrchr(device_name, '@');
			if (at != NULL) {
				while (at > device_name && at[-1] == ' ')
					at--;
				*at = 0;
			}
		}
	}
	if (property != NULL) {
		if (*property->device && *indigo_property_name(device->version, property)) {
			INDIGO_PRINTF(*handle, "<getProperties version='1.7' switch='%d.%d' device='%s' name='%s'/>\n", (INDIGO_VERSION_CURRENT >> 8) & 0xFF, INDIGO_VERSION_CURRENT & 0xFF, indigo_xml_escape(device_name), indigo_property_name(device->version, property));
		} else if (*property->device) {
			INDIGO_PRINTF(*handle, "<getProperties version='1.7' switch='%d.%d' device='%s'/>\n", (INDIGO_VERSION_CURRENT >> 8) & 0xFF, INDIGO_VERSION_CURRENT & 0xFF, indigo_xml_escape(device_name));
		} else if (*indigo_property_name(device->version, property)) {
			INDIGO_PRINTF(*handle, "<getProperties version='1.7' switch='%d.%d' name='%s'/>\n", (INDIGO_VERSION_CURRENT >> 8) & 0xFF, INDIGO_VERSION_CURRENT & 0xFF, indigo_property_name(device->version, property));
		} else {
			INDIGO_PRINTF(*handle, "<getProperties version='1.7' switch='%d.%d'/>\n", (INDIGO_VERSION_CURRENT >> 8) & 0xFF, INDIGO_VERSION_CURRENT & 0xFF);
		}
	} else if (indigo_client_name) {
		INDIGO_PRINTF(*handle, "<getProperties version='1.7' client='%s' switch='%d.%d'/>\n", indigo_client_name, (INDIGO_VERSION_CURRENT >> 8) & 0xFF, INDIGO_VERSION_CURRENT & 0xFF);
#if defined(INDIGO_LINUX) || defined(INDIGO_MACOS)
	} else if (indigo_main_argv) {
		INDIGO_PRINTF(*handle, "<getProperties version='1.7' client='%s' switch='%d.%d'/>\n", basename((char *)indigo_main_argv[0]), (INDIGO_VERSION_CURRENT >> 8) & 0xFF, INDIGO_VERSION_CURRENT & 0xFF);
#endif
	} else {
		INDIGO_PRINTF(*handle, "<getProperties version='1.7' switch='%d.%d'/>\n", (INDIGO_VERSION_CURRENT >> 8) & 0xFF, INDIGO_VERSION_CURRENT & 0xFF);
	}
	pthread_mutex_unlock(&xml_mutex);
	return INDIGO_OK;
failure:
	indigo_uni_close(device_context->input);
	indigo_uni_close(device_context->output);
	pthread_mutex_unlock(&xml_mutex);
	return INDIGO_OK;
}

indigo_result indigo_xml_client_parser_change_property(indigo_device *device, indigo_client *client, indigo_property *property) {
	assert(device != NULL);
	assert(property != NULL);
	if (!indigo_reshare_remote_devices && client && client->is_remote) {
		return INDIGO_OK;
	}
	indigo_adapter_context *device_context = (indigo_adapter_context *)device->device_context;
	if (device_context->output == NULL) {
		return INDIGO_OK;
	}
	pthread_mutex_lock(&xml_mutex);
	assert(device_context != NULL);
	indigo_uni_handle **handle = device_context->output;
	char device_name[INDIGO_NAME_SIZE];
	char token[64] = "";
	char b1[32];
	indigo_copy_name(device_name, property->device);
	if (indigo_use_host_suffix) {
		char *at = strrchr(device_name, '@');
		if (at != NULL) {
			while (at > device_name && at[-1] == ' ')
				at--;
			*at = 0;
		}
	}
	if (property->access_token) {
		sprintf(token, " token='%llx'", property->access_token);
	}
	switch (property->type) {
		case INDIGO_TEXT_VECTOR:
			INDIGO_PRINTF(*handle, "<newTextVector device='%s' name='%s'%s>\n", indigo_xml_escape(device_name), indigo_property_name(device->version, property), token);
			for (int i = 0; i < property->count; i++) {
				indigo_item *item = &property->items[i];
				INDIGO_PRINTF(*handle, "<oneText name='%s'>%s</oneText>\n", indigo_item_name(device->version, property, item), item->text.long_value ? indigo_xml_escape(item->text.long_value) : indigo_xml_escape(item->text.value));
			}
			INDIGO_PRINTF(*handle, "</newTextVector>\n");
			break;
		case INDIGO_NUMBER_VECTOR:
			INDIGO_PRINTF(*handle, "<newNumberVector device='%s' name='%s'%s>\n", indigo_xml_escape(device_name), indigo_property_name(device->version, property), token);
			for (int i = 0; i < property->count; i++) {
				indigo_item *item = &property->items[i];
				INDIGO_PRINTF(*handle, "<oneNumber name='%s'>%s</oneNumber>\n", indigo_item_name(device->version, property, item), indigo_dtoa(item->number.value, b1));
			}
			INDIGO_PRINTF(*handle, "</newNumberVector>\n");
			break;
		case INDIGO_SWITCH_VECTOR:
			INDIGO_PRINTF(*handle, "<newSwitchVector device='%s' name='%s'%s>\n", indigo_xml_escape(device_name), indigo_property_name(device->version, property), token);
			for (int i = 0; i < property->count; i++) {
				indigo_item *item = &property->items[i];
				INDIGO_PRINTF(*handle, "<oneSwitch name='%s'>%s</oneSwitch>\n", indigo_item_name(device->version, property, item), item->sw.value ? "On" : "Off");
			}
			INDIGO_PRINTF(*handle, "</newSwitchVector>\n");
			break;
		case INDIGO_BLOB_VECTOR:
			for (int i = 0; i < property->count; i++) {
				indigo_item *item = &property->items[i];
				indigo_upload_http_blob_item(item);
			}
			INDIGO_PRINTF(*handle, "<newBLOBVector device='%s' name='%s'%s>\n", indigo_xml_escape(device_name), indigo_property_name(device->version, property), token);
			for (int i = 0; i < property->count; i++) {
				indigo_item *item = &property->items[i];
				INDIGO_PRINTF(*handle, "<oneBLOB name='%s' format='%s'/>\n", indigo_item_name(device->version, property, item), item->blob.format);
			}
			INDIGO_PRINTF(*handle, "</newBLOBVector>\n");
			break;
		default:
			break;
	}
	pthread_mutex_unlock(&xml_mutex);
	return INDIGO_OK;
failure:
	indigo_uni_close(device_context->input);
	indigo_uni_close(device_context->output);
	pthread_mutex_unlock(&xml_mutex);
	return INDIGO_OK;
}

indigo_result indigo_xml_client_parser_enable_blob(indigo_device *device, indigo_client *client, indigo_property *property, indigo_enable_blob_mode mode) {
	assert(device != NULL);
	assert(property != NULL);
	if (!indigo_reshare_remote_devices && client && client->is_remote) {
		return INDIGO_OK;
	}
	indigo_adapter_context *device_context = (indigo_adapter_context *)device->device_context;
	if (device_context->output == NULL) {
		return INDIGO_OK;
	}
	pthread_mutex_lock(&xml_mutex);
	assert(device_context != NULL);
	indigo_uni_handle **handle = device_context->output;
	char device_name[INDIGO_NAME_SIZE];
	indigo_copy_name(device_name, property->device);
	if (indigo_use_host_suffix) {
		char *at = strrchr(device_name, '@');
		if (at != NULL) {
			while (at > device_name && at[-1] == ' ')
				at--;
			*at = 0;
		}
	}
	char *mode_text = "Also";
	if (mode == INDIGO_ENABLE_BLOB_NEVER) {
		mode_text = "Never";
	} else if (mode == INDIGO_ENABLE_BLOB_URL && device->version >= INDIGO_VERSION_2_0)
		mode_text = "URL";
	if (*property->name) {
		INDIGO_PRINTF(*handle, "<enableBLOB device='%s' name='%s'>%s</enableBLOB>\n", indigo_xml_escape(device_name), indigo_property_name(device->version, property), mode_text);
	} else {
		INDIGO_PRINTF(*handle, "<enableBLOB device='%s'>%s</enableBLOB>\n", indigo_xml_escape(device_name), mode_text);
	}
	pthread_mutex_unlock(&xml_mutex);
	return INDIGO_OK;
failure:
	indigo_uni_close(device_context->input);
	indigo_uni_close(device_context->output);
	pthread_mutex_unlock(&xml_mutex);
	return INDIGO_OK;
}

static pthread_mutex_t write_mutex = PTHREAD_MUTEX_INITIALIZER;

static const char *message_attribute(const char *message) {
	if (message) {
		static char buffer[INDIGO_VALUE_SIZE];
		snprintf(buffer, INDIGO_VALUE_SIZE, " message='%s'", indigo_xml_escape((char *)message));
		return buffer;
	}
	return "";
}

static const char *hints_attribute(const char *hints) {
	if (*hints) {
		static char buffer[INDIGO_VALUE_SIZE];
		snprintf(buffer, INDIGO_VALUE_SIZE, " hints='%s'", indigo_xml_escape((char *)hints));
		return buffer;
	}
	return "";
}

indigo_result indigo_xml_device_adapter_define_property(indigo_client *client, indigo_device *device, indigo_property *property, const char *message) {
	assert(device != NULL);
	assert(client != NULL);
	assert(property != NULL);
	if (!indigo_reshare_remote_devices && device->is_remote) {
		return INDIGO_OK;
	}
	if (client->version == INDIGO_VERSION_NONE) {
		return INDIGO_OK;
	}
	indigo_adapter_context *client_context = (indigo_adapter_context *)client->client_context;
	if (client_context->output == NULL) {
		return INDIGO_OK;
	}
	pthread_mutex_lock(&write_mutex);
	assert(client_context != NULL);
	indigo_uni_handle **handle = client_context->output;
	char b1[32], b2[32], b3[32], b4[32], b5[32];
	switch (property->type) {
		case INDIGO_TEXT_VECTOR:
			INDIGO_PRINTF(*handle, "<defTextVector device='%s' name='%s' group='%s' label='%s' perm='%s' state='%s'%s%s>\n", indigo_xml_escape(property->device), indigo_property_name(client->version, property), indigo_xml_escape(property->group), indigo_xml_escape(property->label), indigo_property_perm_text[property->perm], indigo_property_state_text[property->state], hints_attribute(property->hints), message_attribute(message));
			for (int i = 0; i < property->count; i++) {
				indigo_item *item = &property->items[i];
				INDIGO_PRINTF(*handle, "<defText name='%s' label='%s'%s>%s</defText>\n", indigo_item_name(client->version, property, item), indigo_xml_escape(item->label), hints_attribute(item->hints), indigo_xml_escape(indigo_get_text_item_value(item)));
			}
			INDIGO_PRINTF(*handle, "</defTextVector>\n");
			break;
		case INDIGO_NUMBER_VECTOR:
			INDIGO_PRINTF(*handle, "<defNumberVector device='%s' name='%s' group='%s' label='%s' perm='%s' state='%s'%s%s>\n", indigo_xml_escape(property->device), indigo_property_name(client->version, property), indigo_xml_escape(property->group), indigo_xml_escape(property->label), indigo_property_perm_text[property->perm], indigo_property_state_text[property->state], hints_attribute(property->hints), message_attribute(message));
			for (int i = 0; i < property->count; i++) {
				indigo_item *item = &property->items[i];
				if (client->version >= INDIGO_VERSION_2_0 && property->perm != INDIGO_RO_PERM) {
					INDIGO_PRINTF(*handle, "<defNumber name='%s' label='%s' format='%s' min='%s' max='%s' step='%s' target='%s'>%s</defNumber>\n", indigo_item_name(client->version, property, item), indigo_xml_escape(item->label), item->number.format, indigo_dtoa(item->number.min, b1), indigo_dtoa(item->number.max, b2), indigo_dtoa(item->number.step, b3), indigo_dtoa(item->number.target, b4), indigo_dtoa(item->number.value, b5));
				} else {
					INDIGO_PRINTF(*handle, "<defNumber name='%s' label='%s'%s format='%s' min='%s' max='%s' step='%s'>%s</defNumber>\n", indigo_item_name(client->version, property, item), indigo_xml_escape(item->label), hints_attribute(item->hints), item->number.format, indigo_dtoa(item->number.min, b1), indigo_dtoa(item->number.max, b2), indigo_dtoa(item->number.step, b3), indigo_dtoa(item->number.value, b4));
				}
			}
			INDIGO_PRINTF(*handle, "</defNumberVector>\n");
			break;
		case INDIGO_SWITCH_VECTOR:
			INDIGO_PRINTF(*handle, "<defSwitchVector device='%s' name='%s' group='%s' label='%s' perm='%s' state='%s' rule='%s'%s%s>\n", indigo_xml_escape(property->device), indigo_property_name(client->version, property), indigo_xml_escape(property->group), indigo_xml_escape(property->label), indigo_property_perm_text[property->perm], indigo_property_state_text[property->state], indigo_switch_rule_text[property->rule], hints_attribute(property->hints), message_attribute(message));
			for (int i = 0; i < property->count; i++) {
				indigo_item *item = &property->items[i];
				INDIGO_PRINTF(*handle, "<defSwitch name='%s' label='%s'%s>%s</defSwitch>\n", indigo_item_name(client->version, property, item), indigo_xml_escape(item->label), hints_attribute(item->hints), item->sw.value ? "On" : "Off");
			}
			INDIGO_PRINTF(*handle, "</defSwitchVector>\n");
			break;
		case INDIGO_LIGHT_VECTOR:
			INDIGO_PRINTF(*handle, "<defLightVector device='%s' name='%s' group='%s' label='%s' perm='%s' state='%s'%s%s>\n", indigo_xml_escape(property->device), indigo_property_name(client->version, property), indigo_xml_escape(property->group), indigo_xml_escape(property->label), indigo_property_perm_text[property->perm], indigo_property_state_text[property->state], hints_attribute(property->hints), message_attribute(message));
			for (int i = 0; i < property->count; i++) {
				indigo_item *item = &property->items[i];
				INDIGO_PRINTF(*handle, " <defLight name='%s' label='%s'%s>%s</defLight>\n", indigo_item_name(client->version, property, item), indigo_xml_escape(item->label), hints_attribute(item->hints), indigo_property_state_text[item->light.value]);
			}
			INDIGO_PRINTF(*handle, "</defLightVector>\n");
			break;
		case INDIGO_BLOB_VECTOR:
			INDIGO_PRINTF(*handle, "<defBLOBVector device='%s' name='%s' group='%s' label='%s' perm='%s' state='%s'%s%s>\n", indigo_xml_escape(property->device), indigo_property_name(client->version, property), indigo_xml_escape(property->group), indigo_xml_escape(property->label), indigo_property_perm_text[property->perm], indigo_property_state_text[property->state], hints_attribute(property->hints), message_attribute(message));
			for (int i = 0; i < property->count; i++) {
				indigo_item *item = &property->items[i];
				if (property->perm == INDIGO_WO_PERM && client->version >= INDIGO_VERSION_2_0) {
					if (item->blob.url[0] == 0 || indigo_proxy_blob) {
						INDIGO_PRINTF(*handle, "<defBLOB name='%s' path='/blob/%p' label='%s'%s/>\n", indigo_item_name(client->version, property, item), item, indigo_xml_escape(item->label), hints_attribute(item->hints));
					} else {
						INDIGO_PRINTF(*handle, "<defBLOB name='%s' url='%s' label='%s'%s/>\n", indigo_item_name(client->version, property, item), item->blob.url, indigo_xml_escape(item->label), hints_attribute(item->hints));
					}
				} else {
					INDIGO_PRINTF(*handle, "<defBLOB name='%s' label='%s'%s/>\n", indigo_item_name(client->version, property, item), indigo_xml_escape(item->label), hints_attribute(item->hints));
				}
			}
			INDIGO_PRINTF(*handle, "</defBLOBVector>\n");
			break;
	}
	pthread_mutex_unlock(&write_mutex);
	return INDIGO_OK;
failure:
	indigo_uni_close(client_context->input);
	indigo_uni_close(client_context->output);
	pthread_mutex_unlock(&write_mutex);
	return INDIGO_OK;
}

indigo_result indigo_xml_device_adapter_update_property(indigo_client *client, indigo_device *device, indigo_property *property, const char *message) {
	assert(device != NULL);
	assert(client != NULL);
	assert(property != NULL);
	if (!indigo_reshare_remote_devices && device->is_remote) {
		return INDIGO_OK;
	}
	if (client->version == INDIGO_VERSION_NONE) {
		return INDIGO_OK;
	}
	indigo_adapter_context *client_context = (indigo_adapter_context *)client->client_context;
	if (client_context->output == NULL) {
		return INDIGO_OK;
	}
	pthread_mutex_lock(&write_mutex);
	assert(client_context != NULL);
	indigo_uni_handle **handle = client_context->output;
	char b1[32], b2[32];
	switch (property->type) {
		case INDIGO_TEXT_VECTOR:
			INDIGO_PRINTF(*handle, "<setTextVector device='%s' name='%s' state='%s'%s>\n", indigo_xml_escape(property->device), indigo_property_name(client->version, property), indigo_property_state_text[property->state], message_attribute(message));
			for (int i = 0; i < property->count; i++) {
				indigo_item *item = &property->items[i];
				INDIGO_PRINTF(*handle, "<oneText name='%s'>%s</oneText>\n", indigo_item_name(client->version, property, item), indigo_xml_escape(indigo_get_text_item_value(item)));
			}
			INDIGO_PRINTF(*handle, "</setTextVector>\n");
			break;
		case INDIGO_NUMBER_VECTOR:
			INDIGO_PRINTF(*handle, "<setNumberVector device='%s' name='%s' state='%s'%s>\n", indigo_xml_escape(property->device), indigo_property_name(client->version, property), indigo_property_state_text[property->state], message_attribute(message));
			for (int i = 0; i < property->count; i++) {
				indigo_item *item = &property->items[i];
				if (client->version >= INDIGO_VERSION_2_0 && property->perm != INDIGO_RO_PERM) {
					INDIGO_PRINTF(*handle, "<oneNumber name='%s' target='%s'>%s</oneNumber>\n", indigo_item_name(client->version, property, item), indigo_dtoa(item->number.target, b1), indigo_dtoa(item->number.value, b2));
				} else {
					INDIGO_PRINTF(*handle, "<oneNumber name='%s'>%s</oneNumber>\n", indigo_item_name(client->version, property, item), indigo_dtoa(item->number.value, b1));
				}
			}
			INDIGO_PRINTF(*handle, "</setNumberVector>\n");
			break;
		case INDIGO_SWITCH_VECTOR:
			INDIGO_PRINTF(*handle, "<setSwitchVector device='%s' name='%s' state='%s'%s>\n", indigo_xml_escape(property->device), indigo_property_name(client->version, property), indigo_property_state_text[property->state], message_attribute(message));
			for (int i = 0; i < property->count; i++) {
				indigo_item *item = &property->items[i];
				INDIGO_PRINTF(*handle, "<oneSwitch name='%s'>%s</oneSwitch>\n", indigo_item_name(client->version, property, item), item->sw.value ? "On" : "Off");
			}
			INDIGO_PRINTF(*handle, "</setSwitchVector>\n");
			break;
		case INDIGO_LIGHT_VECTOR:
			INDIGO_PRINTF(*handle, "<setLightVector device='%s' name='%s' state='%s'%s>\n", indigo_xml_escape(property->device), indigo_property_name(client->version, property), indigo_property_state_text[property->state], message_attribute(message));
			for (int i = 0; i < property->count; i++) {
				indigo_item *item = &property->items[i];
				INDIGO_PRINTF(*handle, "<oneLight name='%s'>%s</oneLight>\n", indigo_item_name(client->version, property, item), indigo_property_state_text[item->light.value]);
			}
			INDIGO_PRINTF(*handle, "</setLightVector>\n");
			break;
		case INDIGO_BLOB_VECTOR: {
			indigo_enable_blob_mode mode = INDIGO_ENABLE_BLOB_NEVER;
			indigo_enable_blob_mode_record *record = client->enable_blob_mode_records;
			while (record) {
				if ((*record->device == 0 || !strcmp(property->device, record->device)) && (*record->name == 0 || !strcmp(property->name, record->name))) {
					mode = record->mode;
					break;
				}
				record = record->next;
			}
			if (mode != INDIGO_ENABLE_BLOB_NEVER) {
				INDIGO_PRINTF(*handle, "<setBLOBVector device='%s' name='%s' state='%s'%s>\n", indigo_xml_escape(property->device), indigo_property_name(client->version, property), indigo_property_state_text[property->state], message_attribute(message));
				if (property->state == INDIGO_OK_STATE) {
					for (int i = 0; i < property->count; i++) {
						indigo_item *item = &property->items[i];
						if (mode == INDIGO_ENABLE_BLOB_URL && client->version >= INDIGO_VERSION_2_0) {
							if (item->blob.value || indigo_proxy_blob) {
								INDIGO_PRINTF(*handle, "<oneBLOB name='%s' path='/blob/%p%s'/>\n", indigo_item_name(client->version, property, item), item, item->blob.format);
							} else {
								INDIGO_PRINTF(*handle, "<oneBLOB name='%s' url='%s'/>\n", indigo_item_name(client->version, property, item), item->blob.url);
							}
						} else {
							long input_length = item->blob.size;
							unsigned char *data = item->blob.value;
							INDIGO_PRINTF(*handle, "<oneBLOB name='%s' format='%s' size='%ld'>\n", indigo_item_name(client->version, property, item), item->blob.format, item->blob.size);
							if (client->version >= INDIGO_VERSION_2_0) {
								while (input_length) {
									char encoded_data[BASE64_BUF_SIZE + 1];
									long len = (RAW_BUF_SIZE < input_length) ?  RAW_BUF_SIZE : input_length;
									long enclen = base64_encode((unsigned char*)encoded_data, (unsigned char*)data, len);
									indigo_uni_write(*handle, encoded_data, enclen);
									input_length -= len;
									data += len;
								}
							} else {
								static char encoded_data[74];
								while (input_length) {
									long len = (54 < input_length) ?  54 : input_length;
									long enclen = base64_encode((unsigned char*)encoded_data, (unsigned char*)data, len);
									encoded_data[enclen] = '\n';
									indigo_uni_write(*handle, encoded_data, enclen);
									input_length -= len;
									data += len;
								}
							}
							INDIGO_PRINTF(*handle, "</oneBLOB>\n");
						}
					}
				}
				INDIGO_PRINTF(*handle, "</setBLOBVector>\n");
			}
			break;
		}
	}
	pthread_mutex_unlock(&write_mutex);
	return INDIGO_OK;
failure:
	indigo_uni_close(client_context->input);
	indigo_uni_close(client_context->output);
	pthread_mutex_unlock(&write_mutex);
	return INDIGO_OK;
}

indigo_result indigo_xml_device_adapter_delete_property(indigo_client *client, indigo_device *device, indigo_property *property, const char *message) {
	assert(device != NULL);
	assert(client != NULL);
	assert(property != NULL);
	if (!indigo_reshare_remote_devices && device->is_remote) {
		return INDIGO_OK;
	}
	if (client->version == INDIGO_VERSION_NONE) {
		return INDIGO_OK;
	}
	indigo_adapter_context *client_context = (indigo_adapter_context *)client->client_context;
	if (client_context->output == NULL) {
		return INDIGO_OK;
	}
	pthread_mutex_lock(&write_mutex);
	assert(client_context != NULL);
	indigo_uni_handle **handle = client_context->output;
	if (*property->name) {
		INDIGO_PRINTF(*handle, "<delProperty device='%s' name='%s'%s/>\n", indigo_xml_escape(property->device), indigo_property_name(client->version, property), message_attribute(message));
	} else {
		INDIGO_PRINTF(*handle, "<delProperty device='%s'%s/>\n", device->name, message_attribute(message));
	}
	pthread_mutex_unlock(&write_mutex);
	return INDIGO_OK;
failure:
	indigo_uni_close(client_context->input);
	indigo_uni_close(client_context->output);
	pthread_mutex_unlock(&write_mutex);
	return INDIGO_OK;
}

indigo_result indigo_xml_device_adapter_send_message(indigo_client *client, indigo_device *device, const char *message) {
	assert(device != NULL);
	assert(client != NULL);
	if (!indigo_reshare_remote_devices && device->is_remote) {
		return INDIGO_OK;
	}
	if (client->version == INDIGO_VERSION_NONE) {
		return INDIGO_OK;
	}
	indigo_adapter_context *client_context = (indigo_adapter_context *)client->client_context;
	if (client_context->output == NULL) {
		return INDIGO_OK;
	}
	pthread_mutex_lock(&write_mutex);
	assert(client_context != NULL);
	indigo_uni_handle **handle = client_context->output;
	if (message) {
		if (device) {
			INDIGO_PRINTF(*handle, "<message device='%s'%s/>\n", device->name, message_attribute(message));
		} else {
			INDIGO_PRINTF(*handle, "<message%s/>\n", message_attribute(message));
		}
	}
	pthread_mutex_unlock(&write_mutex);
	return INDIGO_OK;
failure:
	indigo_uni_close(client_context->input);
	indigo_uni_close(client_context->output);
	pthread_mutex_unlock(&write_mutex);
	return INDIGO_OK;
}
