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

/** INDIGO XML wire protocol driver side adapter
 \file indigo_client_xml.c
 */

#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <stdio.h>
#include <ctype.h>
#include <pthread.h>
#include <assert.h>

#if defined(INDIGO_LINUX) || defined(INDIGO_MACOS)
#include <unistd.h>
#include <libgen.h>
#endif

#if defined(INDIGO_WINDOWS)
#include <io.h>
#include <winsock2.h>
#include <basetsd.h>
#define ssize_t SSIZE_T
#define close closesocket
#pragma warning(disable:4996)
#endif

#include <indigo/indigo_xml.h>
#include <indigo/indigo_io.h>
#include <indigo/indigo_version.h>
#include <indigo/indigo_client_xml.h>

#define INDIGO_PRINTF(...) if (!indigo_printf(__VA_ARGS__)) goto failure

extern char *indigo_client_name;

static pthread_mutex_t xml_mutex = PTHREAD_MUTEX_INITIALIZER;

static indigo_result xml_client_parser_enumerate_properties(indigo_device *device, indigo_client *client, indigo_property *property) {
	assert(device != NULL);
	if (!indigo_reshare_remote_devices && client && client->is_remote)
		return INDIGO_OK;
	indigo_adapter_context *device_context = (indigo_adapter_context *)device->device_context;
	if (device_context->output <= 0)
		return INDIGO_OK;
	pthread_mutex_lock(&xml_mutex);
	assert(device_context != NULL);
	int handle = device_context->output;
	char device_name[INDIGO_NAME_SIZE];
	const char *property_name = NULL;
	if (property != NULL) {
		property_name = indigo_property_name(device->version, property);
		if (*property->device) {
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
	}
	if (property != NULL) {
		if (*property->device && property_name) {
			INDIGO_PRINTF(handle, "<getProperties version='1.7' switch='%d.%d' device='%s' name='%s'/>\n", (INDIGO_VERSION_CURRENT >> 8) & 0xFF, INDIGO_VERSION_CURRENT & 0xFF, indigo_xml_escape(device_name), property_name);
		} else if (*property->device) {
			INDIGO_PRINTF(handle, "<getProperties version='1.7' switch='%d.%d' device='%s'/>\n", (INDIGO_VERSION_CURRENT >> 8) & 0xFF, INDIGO_VERSION_CURRENT & 0xFF, indigo_xml_escape(device_name));
		} else if (property_name) {
			INDIGO_PRINTF(handle, "<getProperties version='1.7' switch='%d.%d' name='%s'/>\n", (INDIGO_VERSION_CURRENT >> 8) & 0xFF, INDIGO_VERSION_CURRENT & 0xFF, property_name);
		} else {
			INDIGO_PRINTF(handle, "<getProperties version='1.7' switch='%d.%d'/>\n", (INDIGO_VERSION_CURRENT >> 8) & 0xFF, INDIGO_VERSION_CURRENT & 0xFF);
		}
	} else if (indigo_client_name) {
		INDIGO_PRINTF(handle, "<getProperties version='1.7' client='%s' switch='%d.%d'/>\n", indigo_client_name, (INDIGO_VERSION_CURRENT >> 8) & 0xFF, INDIGO_VERSION_CURRENT & 0xFF);
#if defined(INDIGO_LINUX) || defined(INDIGO_MACOS)
	} else if (indigo_main_argv) {
		INDIGO_PRINTF(handle, "<getProperties version='1.7' client='%s' switch='%d.%d'/>\n", basename((char *)indigo_main_argv[0]), (INDIGO_VERSION_CURRENT >> 8) & 0xFF, INDIGO_VERSION_CURRENT & 0xFF);
#endif
	} else {
		INDIGO_PRINTF(handle, "<getProperties version='1.7' switch='%d.%d'/>\n", (INDIGO_VERSION_CURRENT >> 8) & 0xFF, INDIGO_VERSION_CURRENT & 0xFF);
	}
	pthread_mutex_unlock(&xml_mutex);
	return INDIGO_OK;
failure:
	if (device_context->output == device_context->input) {
		close(device_context->input);
	} else {
		close(device_context->input);
		close(device_context->output);
	}
	device_context->output = device_context->input = -1;
	pthread_mutex_unlock(&xml_mutex);
	return INDIGO_OK;
}

static indigo_result xml_client_parser_change_property(indigo_device *device, indigo_client *client, indigo_property *property) {
	assert(device != NULL);
	assert(property != NULL);
	if (!indigo_reshare_remote_devices && client && client->is_remote)
		return INDIGO_OK;
	indigo_adapter_context *device_context = (indigo_adapter_context *)device->device_context;
	if (device_context->output <= 0)
		return INDIGO_OK;
	pthread_mutex_lock(&xml_mutex);
	assert(device_context != NULL);
	int handle = device_context->output;
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
		INDIGO_PRINTF(handle, "<newTextVector device='%s' name='%s'%s>\n", indigo_xml_escape(device_name), indigo_property_name(device->version, property), token);
		for (int i = 0; i < property->count; i++) {
			indigo_item *item = &property->items[i];
			INDIGO_PRINTF(handle, "<oneText name='%s'>%s</oneText>\n", indigo_item_name(device->version, property, item), item->text.long_value ? indigo_xml_escape(item->text.long_value) : indigo_xml_escape(item->text.value));
		}
		INDIGO_PRINTF(handle, "</newTextVector>\n");
		break;
	case INDIGO_NUMBER_VECTOR:
		INDIGO_PRINTF(handle, "<newNumberVector device='%s' name='%s'%s>\n", indigo_xml_escape(device_name), indigo_property_name(device->version, property), token);
		for (int i = 0; i < property->count; i++) {
			indigo_item *item = &property->items[i];
			INDIGO_PRINTF(handle, "<oneNumber name='%s'>%s</oneNumber>\n", indigo_item_name(device->version, property, item), indigo_dtoa(item->number.value, b1));
		}
		INDIGO_PRINTF(handle, "</newNumberVector>\n");
		break;
	case INDIGO_SWITCH_VECTOR:
		INDIGO_PRINTF(handle, "<newSwitchVector device='%s' name='%s'%s>\n", indigo_xml_escape(device_name), indigo_property_name(device->version, property), token);
		for (int i = 0; i < property->count; i++) {
			indigo_item *item = &property->items[i];
			INDIGO_PRINTF(handle, "<oneSwitch name='%s'>%s</oneSwitch>\n", indigo_item_name(device->version, property, item), item->sw.value ? "On" : "Off");
		}
		INDIGO_PRINTF(handle, "</newSwitchVector>\n");
		break;
	case INDIGO_BLOB_VECTOR:
		for (int i = 0; i < property->count; i++) {
			indigo_item *item = &property->items[i];
			indigo_upload_http_blob_item(item);
		}
		INDIGO_PRINTF(handle, "<newBLOBVector device='%s' name='%s'%s>\n", indigo_xml_escape(device_name), indigo_property_name(device->version, property), token);
		for (int i = 0; i < property->count; i++) {
			indigo_item *item = &property->items[i];
			INDIGO_PRINTF(handle, "<oneBLOB name='%s' format='%s'/>\n", indigo_item_name(device->version, property, item), item->blob.format);
		}
		INDIGO_PRINTF(handle, "</newBLOBVector>\n");
		break;
	default:
		break;
	}
	pthread_mutex_unlock(&xml_mutex);
	return INDIGO_OK;
failure:
	if (device_context->output == device_context->input) {
		close(device_context->input);
	} else {
		close(device_context->input);
		close(device_context->output);
	}
	device_context->output = device_context->input = -1;
	pthread_mutex_unlock(&xml_mutex);
	return INDIGO_OK;
}

static indigo_result xml_client_parser_enable_blob(indigo_device *device, indigo_client *client, indigo_property *property, indigo_enable_blob_mode mode) {
	assert(device != NULL);
	assert(property != NULL);
	if (!indigo_reshare_remote_devices && client && client->is_remote)
		return INDIGO_OK;
	indigo_adapter_context *device_context = (indigo_adapter_context *)device->device_context;
	if (device_context->output <= 0)
		return INDIGO_OK;
	pthread_mutex_lock(&xml_mutex);
	assert(device_context != NULL);
	int handle = device_context->output;
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
	if (mode == INDIGO_ENABLE_BLOB_NEVER)
		mode_text = "Never";
	else if (mode == INDIGO_ENABLE_BLOB_URL && device->version >= INDIGO_VERSION_2_0)
		mode_text = "URL";
	if (*property->name) {
		INDIGO_PRINTF(handle, "<enableBLOB device='%s' name='%s'>%s</enableBLOB>\n", indigo_xml_escape(device_name), indigo_property_name(device->version, property), mode_text);
	} else {
		INDIGO_PRINTF(handle, "<enableBLOB device='%s'>%s</enableBLOB>\n", indigo_xml_escape(device_name), mode_text);
	}
	pthread_mutex_unlock(&xml_mutex);
	return INDIGO_OK;
failure:
	if (device_context->output == device_context->input) {
		close(device_context->input);
	} else {
		close(device_context->input);
		close(device_context->output);
	}
	device_context->output = device_context->input = -1;
	pthread_mutex_unlock(&xml_mutex);
	return INDIGO_OK;
}

static indigo_result xml_client_parser_detach(indigo_device *device) {
	assert(device != NULL);
	indigo_adapter_context *device_context = (indigo_adapter_context *)device->device_context;
	if (device_context->output <= 0)
		return INDIGO_OK;
	close(device_context->input);
	close(device_context->output);
	return INDIGO_OK;
}

indigo_device *indigo_xml_client_adapter(char *name, char *url_prefix, int input, int output) {
	static indigo_device device_template = INDIGO_DEVICE_INITIALIZER(
		"XML Client Adapter", NULL,
		xml_client_parser_enumerate_properties,
		xml_client_parser_change_property,
		xml_client_parser_enable_blob,
		xml_client_parser_detach
	);
	indigo_device *device = indigo_safe_malloc_copy(sizeof(indigo_device), &device_template);
	sprintf(device->name, "@ %s", name);
	device->is_remote = input == output; // is socket, otherwise is pipe
	indigo_adapter_context *device_context = indigo_safe_malloc(sizeof(indigo_adapter_context));
	device_context->input = input;
	device_context->output = output;
	indigo_copy_name(device_context->url_prefix, url_prefix);
	device->device_context = device_context;
	return device;
}
