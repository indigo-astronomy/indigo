// Copyright (c) 2017-2025 CloudMakers, s. r. o.
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

/** INDIGO Test agent
 \file indigo_agent_test.c
 */

#define DRIVER_VERSION 0x03000000
#define DRIVER_NAME	"indigo_agent_test"

#define TEST_AGENT_NAME "Test Agent"

#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <pthread.h>

#include <indigo/indigo_uni_io.h>

#include "indigo_agent_test.h"

#define PRIVATE_DATA	((agent_private_data *)device->private_data)

#define TEST_PROPERTY	(PRIVATE_DATA->test_property)
#define TEST_1_ITEM		(TEST_PROPERTY->items+0)
#define TEST_2_ITEM		(TEST_PROPERTY->items+1)
#define TEST_3_ITEM		(TEST_PROPERTY->items+2)
#define TEST_4_ITEM		(TEST_PROPERTY->items+3)

typedef struct {
	indigo_property *test_property;
} agent_private_data;

static indigo_result agent_enumerate_properties(indigo_device *device, indigo_client *client, indigo_property *property);

static void test_1_handler(indigo_device *device) {
	indigo_log("Test #1 started");
	indigo_usleep(1000000);
	TEST_1_ITEM->sw.value = false;
	TEST_PROPERTY->state = INDIGO_OK_STATE;
	indigo_update_property(device, TEST_PROPERTY, NULL);
	indigo_log("Test #1 finished");
}

static void test_2_handler(indigo_device *device) {
	indigo_log("Test #2 started");
	indigo_usleep(1000000);
	TEST_2_ITEM->sw.value = false;
	TEST_PROPERTY->state = INDIGO_OK_STATE;
	indigo_update_property(device, TEST_PROPERTY, NULL);
	indigo_log("Test #2 finished");
}

static void test_3_handler(indigo_device *device) {
	indigo_log("Test #3 started");
	indigo_usleep(1000000);
	TEST_3_ITEM->sw.value = false;
	TEST_PROPERTY->state = INDIGO_OK_STATE;
	indigo_update_property(device, TEST_PROPERTY, NULL);
	indigo_log("Test #3 finished");
}

static void test_4_handler(indigo_device *device) {
	indigo_log("Test #4 started");
	indigo_execute_handler(device, test_1_handler);
	indigo_execute_handler(device, test_1_handler);
	indigo_execute_handler_in(device, 10, test_3_handler);
	indigo_execute_priority_handler(device, 100, test_2_handler);
	TEST_4_ITEM->sw.value = false;
	TEST_PROPERTY->state = INDIGO_OK_STATE;
	indigo_update_property(device, TEST_PROPERTY, NULL);
	indigo_log("Test #4 finished");
}

// -------------------------------------------------------------------------------- INDIGO agent device implementation

static indigo_result agent_device_attach(indigo_device *device) {
	assert(device != NULL);
	assert(PRIVATE_DATA != NULL);
	if (indigo_agent_attach(device, DRIVER_NAME, DRIVER_VERSION) == INDIGO_OK) {
		TEST_PROPERTY = indigo_init_switch_property(NULL, device->name, "TEST", MAIN_GROUP, "Tests", INDIGO_OK_STATE, INDIGO_RW_PERM, INDIGO_AT_MOST_ONE_RULE, 4);
		if (TEST_PROPERTY == NULL) {
			return INDIGO_FAILED;
		}
		indigo_init_switch_item(TEST_1_ITEM, "1", "Test #1", false);
		indigo_init_switch_item(TEST_2_ITEM, "2", "Test #2", false);
		indigo_init_switch_item(TEST_3_ITEM, "3", "Test #3", false);
		indigo_init_switch_item(TEST_4_ITEM, "4", "Test #4", false);
		INDIGO_DEVICE_ATTACH_LOG(DRIVER_NAME, device->name);
		return agent_enumerate_properties(device, NULL, NULL);
	}
	return INDIGO_FAILED;
}

static indigo_result agent_enumerate_properties(indigo_device *device, indigo_client *client, indigo_property *property) {
	indigo_result result = INDIGO_OK;
	if ((result = indigo_agent_enumerate_properties(device, client, property)) == INDIGO_OK) {
		INDIGO_DEFINE_MATCHING_PROPERTY(TEST_PROPERTY);
	}
	return result;
}

static indigo_result agent_change_property(indigo_device *device, indigo_client *client, indigo_property *property) {
	assert(device != NULL);
	assert(DEVICE_CONTEXT != NULL);
	assert(property != NULL);
	if (indigo_property_match(TEST_PROPERTY, property)) {
		if (TEST_PROPERTY->state != INDIGO_BUSY_STATE) {
			indigo_property_copy_values(TEST_PROPERTY, property, false);
			if (TEST_1_ITEM->sw.value) {
				TEST_PROPERTY->state = INDIGO_BUSY_STATE;
				indigo_update_property(device, TEST_PROPERTY, NULL);
				indigo_execute_handler(device, test_1_handler);
			} else if (TEST_2_ITEM->sw.value) {
				TEST_PROPERTY->state = INDIGO_BUSY_STATE;
				indigo_update_property(device, TEST_PROPERTY, NULL);
				indigo_execute_handler(device, test_2_handler);
			} else if (TEST_3_ITEM->sw.value) {
				TEST_PROPERTY->state = INDIGO_BUSY_STATE;
				indigo_update_property(device, TEST_PROPERTY, NULL);
				indigo_execute_handler(device, test_3_handler);
			} else if (TEST_4_ITEM->sw.value) {
				TEST_PROPERTY->state = INDIGO_BUSY_STATE;
				indigo_update_property(device, TEST_PROPERTY, NULL);
				indigo_execute_handler(device, test_4_handler);
			}
		}
	}
	return indigo_agent_change_property(device, client, property);
}

static indigo_result agent_device_detach(indigo_device *device) {
	assert(device != NULL);
	indigo_release_property(TEST_PROPERTY);
	INDIGO_DEVICE_DETACH_LOG(DRIVER_NAME, device->name);
	return indigo_agent_detach(device);
}

// --------------------------------------------------------------------------------

static agent_private_data *private_data = NULL;
static indigo_device *agent_device = NULL;

indigo_result indigo_agent_test(indigo_driver_action action, indigo_driver_info *info) {
	static indigo_device agent_device_template = INDIGO_DEVICE_INITIALIZER(
		TEST_AGENT_NAME,
		agent_device_attach,
		agent_enumerate_properties,
		agent_change_property,
		NULL,
		agent_device_detach
	);

	static indigo_driver_action last_action = INDIGO_DRIVER_SHUTDOWN;

	SET_DRIVER_INFO(info, "Test agent", __FUNCTION__, DRIVER_VERSION, false, last_action);

	if (action == last_action) {
		return INDIGO_OK;
	}

	switch(action) {
		case INDIGO_DRIVER_INIT:
			last_action = action;
			private_data = indigo_safe_malloc(sizeof(agent_private_data));
			agent_device = indigo_safe_malloc_copy(sizeof(indigo_device), &agent_device_template);
			agent_device->private_data = private_data;
			indigo_attach_device(agent_device);
			break;

		case INDIGO_DRIVER_SHUTDOWN:
			last_action = action;
			if (agent_device != NULL) {
				indigo_detach_device(agent_device);
				free(agent_device);
				agent_device = NULL;
			}
			if (private_data != NULL) {
				free(private_data);
				private_data = NULL;
			}
			break;

		case INDIGO_DRIVER_INFO:
			break;
	}
	return INDIGO_OK;
}
