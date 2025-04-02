// Copyright (c) 2016 CloudMakers, s. r. o.
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions
// are met:
//
// 1. Redistributions of source code must retain the above copyright
// notice, this list of conditions and the following disclaimer.
//
// 2. Redistributions in binary form must reproduce the above
// copyright notice, this list of conditions and the following
// disclaimer in the documentation and/or other materials provided
// with the distribution.
//
// 3. The name of the author may not be used to endorse or promote
// products derived from this software without specific prior
// written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE AUTHOR 'AS IS' AND ANY EXPRESS
// OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
// WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY
// DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
// DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
// GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
// WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
// NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
// SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

// version history
// 2.0 by Peter Polakovic <peter.polakovic@cloudmakers.eu>

/** INDIGO XML wire protocol client side adapter
 \file indigo_driver_xml.c
 */

#include <string.h>
#include <stdio.h>
#include <assert.h>

#include <indigo/indigo_version.h>
#include <indigo/indigo_driver_xml.h>

indigo_client *indigo_xml_device_adapter(indigo_uni_handle **input, indigo_uni_handle **output) {
	static indigo_client client_template = {
		"XML Driver Adapter", false, NULL, INDIGO_OK, INDIGO_VERSION_NONE, NULL,
		NULL,
		indigo_xml_device_adapter_define_property,
		indigo_xml_device_adapter_update_property,
		indigo_xml_device_adapter_delete_property,
		indigo_xml_device_adapter_send_message,
		NULL
	};
	indigo_client *client = indigo_safe_malloc_copy(sizeof(indigo_client), &client_template);
	indigo_adapter_context *client_context = indigo_safe_malloc(sizeof(indigo_adapter_context));
	snprintf(client->name, sizeof(client->name), "XML Driver Adapter #%d", (*input)->fd);
	client_context->input = input;
	client_context->output = output;
	client->client_context = client_context;
	client->is_remote = (*input)->type == INDIGO_TCP_HANDLE;
	return client;
}

void indigo_release_xml_device_adapter(indigo_client *client) {
	assert(client != NULL);
	assert(client->client_context != NULL);
	indigo_enable_blob_mode_record *blob_record = client->enable_blob_mode_records;
	while (blob_record) {
		client->enable_blob_mode_records = blob_record->next;
		indigo_safe_free(blob_record);
		blob_record = client->enable_blob_mode_records;
	}
	indigo_safe_free(client->client_context);
	indigo_safe_free(client);
}
