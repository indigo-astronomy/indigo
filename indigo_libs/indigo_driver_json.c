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

/** INDIGO JSON wire protocol client side adapter
 \file indigo_driver_json.c
 */

#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <stdio.h>
#include <ctype.h>
#include <pthread.h>
#include <assert.h>
#include <stdint.h>

#include <indigo/indigo_json.h>
#include <indigo/indigo_io.h>

indigo_client *indigo_json_device_adapter(indigo_uni_handle **input, indigo_uni_handle **output, bool web_socket) {
	static indigo_client client_template = {
		"JSON Driver Adapter", false, NULL, INDIGO_OK, INDIGO_VERSION_CURRENT, NULL,
		NULL,
		indigo_json_device_adapter_define_property,
		indigo_json_device_adapter_update_property,
		indigo_json_device_adapter_delete_property,
		indigo_json_device_adapter_message_property,
		NULL,

	};
	indigo_client *client = indigo_safe_malloc_copy(sizeof(indigo_client), &client_template);
	indigo_adapter_context *client_context = indigo_safe_malloc(sizeof(indigo_adapter_context));
	snprintf(client->name, sizeof(client->name), "JSON Driver Adapter #%d", (*input)->fd);
	client_context->input = input;
	client_context->output = output;
	client_context->web_socket = web_socket;
	client->client_context = client_context;
	client->is_remote = (*input)->type == INDIGO_TCP_HANDLE;
	return client;
}

void indigo_release_json_device_adapter(indigo_client *client) {
	assert(client != NULL);
	assert(client->client_context != NULL);
	free(client->client_context);
	free(client);
}
