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

#include <string.h>
#include <stdio.h>
#include <assert.h>

#include <indigo/indigo_version.h>
#include <indigo/indigo_client_xml.h>


static indigo_result xml_client_detach(indigo_device *device) {
	assert(device != NULL);
	indigo_adapter_context *device_context = (indigo_adapter_context *)device->device_context;
	if (device_context->output == NULL)
		return INDIGO_OK;
	indigo_uni_close(device_context->input);
	indigo_uni_close(device_context->output);
	return INDIGO_OK;
}

indigo_device *indigo_xml_client_adapter(char *name, char *url_prefix, indigo_uni_handle **input, indigo_uni_handle **output) {
	static indigo_device device_template = INDIGO_DEVICE_INITIALIZER(
		"XML Client Adapter", NULL,
		indigo_xml_client_parser_enumerate_properties,
		indigo_xml_client_parser_change_property,
		indigo_xml_client_parser_enable_blob,
		xml_client_detach
	);
	indigo_device *device = indigo_safe_malloc_copy(sizeof(indigo_device), &device_template);
	sprintf(device->name, "@ %s", name);
	device->is_remote = (*input)->type == INDIGO_TCP_HANDLE;
	indigo_adapter_context *device_context = indigo_safe_malloc(sizeof(indigo_adapter_context));
	device_context->input = input;
	device_context->output = output;
	indigo_copy_name(device_context->url_prefix, url_prefix);
	device->device_context = device_context;
	return device;
}

void indigo_release_xml_client_adapter(indigo_device *device) {
	assert(device != NULL);
	assert(device->device_context != NULL);
	indigo_safe_free(device->device_context);
	indigo_safe_free(device);
}
