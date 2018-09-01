// Copyright (c) 2017 CloudMakers, s. r. o.
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

/** INDIGO Agent Driver base
 \file indigo_agent_driver.c
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <assert.h>
#include <string.h>
#include <errno.h>
#include <time.h>
#include <math.h>
#include <fcntl.h>
#include <sys/stat.h>

#include "indigo_agent.h"

indigo_result indigo_agent_attach(indigo_device *device, unsigned version) {
	assert(device != NULL);
	assert(device != NULL);
	if (AGENT_CONTEXT == NULL) {
		device->device_context = malloc(sizeof(indigo_agent_context));
		assert(device->device_context);
		memset(device->device_context, 0, sizeof(indigo_agent_context));
	}
	if (AGENT_CONTEXT != NULL) {
		if (indigo_device_attach(device, version, 0) == INDIGO_OK) {
			CONNECTION_PROPERTY->hidden = true;
			CONFIG_PROPERTY->hidden = true;
			PROFILE_PROPERTY->hidden = true;
			return INDIGO_OK;
		}
	}
	return INDIGO_FAILED;
}

indigo_result indigo_agent_enumerate_properties(indigo_device *device, indigo_client *client, indigo_property *property) {
	assert(device != NULL);
	assert(DEVICE_CONTEXT != NULL);
	return indigo_device_enumerate_properties(device, client, property);
}

indigo_result indigo_agent_change_property(indigo_device *device, indigo_client *client, indigo_property *property) {
	assert(device != NULL);
	assert(DEVICE_CONTEXT != NULL);
	assert(property != NULL);
	return indigo_device_change_property(device, client, property);
}

indigo_result indigo_agent_detach(indigo_device *device) {
	assert(device != NULL);
	return indigo_device_detach(device);
}

indigo_result indigo_add_snoop_rule(indigo_property *target, const char *source_device, const char *source_property) {
	if (*source_device == 0)
		return INDIGO_OK;
	indigo_property *property = indigo_init_text_property(NULL, SNOOP_AGENT_NAME, SNOOP_ADD_RULE_PROPERTY_NAME, NULL, NULL, INDIGO_OK_STATE, INDIGO_RW_PERM, 4);
	if (property == NULL)
		return INDIGO_FAILED;
	indigo_init_text_item(property->items + 0, SNOOP_ADD_RULE_SOURCE_DEVICE_ITEM_NAME, NULL, source_device);
	indigo_init_text_item(property->items + 1, SNOOP_ADD_RULE_SOURCE_PROPERTY_ITEM_NAME, NULL, source_property);
	indigo_init_text_item(property->items + 2, SNOOP_ADD_RULE_TARGET_DEVICE_ITEM_NAME, NULL, target->device);
	indigo_init_text_item(property->items + 3, SNOOP_ADD_RULE_TARGET_PROPERTY_ITEM_NAME, NULL, target->name);
	indigo_result result = indigo_change_property(NULL, property);
	indigo_release_property(property);
	return result;
}

indigo_result indigo_remove_snoop_rule(indigo_property *target, const char *source_device, const char *source_property) {
	if (*source_device == 0)
		return INDIGO_OK;
	indigo_property *property = indigo_init_text_property(NULL, SNOOP_AGENT_NAME, SNOOP_REMOVE_RULE_PROPERTY_NAME, NULL, NULL, INDIGO_OK_STATE, INDIGO_RW_PERM, 4);
	if (property == NULL)
		return INDIGO_FAILED;
	indigo_init_text_item(property->items + 0, SNOOP_REMOVE_RULE_SOURCE_DEVICE_ITEM_NAME, NULL, source_device);
	indigo_init_text_item(property->items + 1, SNOOP_REMOVE_RULE_SOURCE_PROPERTY_ITEM_NAME, NULL, source_property);
	indigo_init_text_item(property->items + 2, SNOOP_REMOVE_RULE_TARGET_DEVICE_ITEM_NAME, NULL, target->device);
	indigo_init_text_item(property->items + 3, SNOOP_REMOVE_RULE_TARGET_PROPERTY_ITEM_NAME, NULL, target->name);
	indigo_result result = indigo_change_property(NULL, property);
	indigo_release_property(property);
	return result;
}
