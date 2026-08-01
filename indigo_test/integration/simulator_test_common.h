// Copyright (c) 2026 CloudMakers, s. r. o.
// All rights reserved.
//
// You can use this software under the terms of 'INDIGO Astronomy
// open-source license' (see LICENSE.md).

#ifndef simulator_test_common_h
#define simulator_test_common_h

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include <indigo/indigo_bus.h>
#include <indigo/indigo_names.h>

#include "../test_runner.h"

#define MAX_DEFINED_PROPERTIES 128
#define ARRAY_SIZE(array) ((int)(sizeof(array) / sizeof((array)[0])))

typedef indigo_result (*simulator_driver_entry)(indigo_driver_action action, indigo_driver_info *info);

typedef struct {
	const char *label;
	const char *driver_name;
	const char *device_name;
	simulator_driver_entry entry;
	bool multi_device_support;
	const char * const *base_properties;
	int base_property_count;
	const char * const *hidden_base_properties;
	int hidden_base_property_count;
	const char * const *connected_properties;
	int connected_property_count;
	const char * const *hidden_connected_properties;
	int hidden_connected_property_count;
} simulator_driver_case;

typedef struct {
	const simulator_driver_case *driver_case;
	int define_count;
	int update_count;
	int defined_property_count;
	char defined_properties[MAX_DEFINED_PROPERTIES][INDIGO_NAME_SIZE];
	indigo_property *cached_properties[MAX_DEFINED_PROPERTIES];
	bool connected;
	bool disconnected;
	indigo_property_state last_connection_state;
} simulator_test_context;

static simulator_test_context context;

static const char *base_properties_with_instances[] = {
	INFO_PROPERTY_NAME,
	SIMULATION_PROPERTY_NAME,
	CONFIG_PROPERTY_NAME,
	PROFILE_NAME_PROPERTY_NAME,
	PROFILE_PROPERTY_NAME,
	ADDITIONAL_INSTANCES_PROPERTY_NAME,
	CONNECTION_PROPERTY_NAME
};

static const char *base_properties_without_instances[] = {
	INFO_PROPERTY_NAME,
	SIMULATION_PROPERTY_NAME,
	CONFIG_PROPERTY_NAME,
	PROFILE_NAME_PROPERTY_NAME,
	PROFILE_PROPERTY_NAME,
	CONNECTION_PROPERTY_NAME
};

static const char *hidden_base_properties[] = {
	DEVICE_PORT_PROPERTY_NAME,
	DEVICE_BAUDRATE_PROPERTY_NAME,
	DEVICE_PORTS_PROPERTY_NAME,
	AUTHENTICATION_PROPERTY_NAME
};

static const char *hidden_base_properties_without_instances[] = {
	DEVICE_PORT_PROPERTY_NAME,
	DEVICE_BAUDRATE_PROPERTY_NAME,
	DEVICE_PORTS_PROPERTY_NAME,
	AUTHENTICATION_PROPERTY_NAME,
	ADDITIONAL_INSTANCES_PROPERTY_NAME
};

static bool has_defined_property(const char *name) {
	for (int i = 0; i < context.defined_property_count; i++) {
		if (!strcmp(context.defined_properties[i], name)) {
			return true;
		}
	}
	return false;
}

static indigo_property *find_cached_property(const char *name) {
	for (int i = 0; i < MAX_DEFINED_PROPERTIES; i++) {
		if (context.cached_properties[i] != NULL && !strcmp(context.cached_properties[i]->name, name)) {
			return context.cached_properties[i];
		}
	}
	return NULL;
}

static indigo_item *find_cached_item(const char *property_name, const char *item_name) {
	indigo_property *property = find_cached_property(property_name);
	if (property == NULL) {
		return NULL;
	}
	for (int i = 0; i < property->count; i++) {
		if (!strcmp(property->items[i].name, item_name)) {
			return property->items + i;
		}
	}
	return NULL;
}

static void cache_property(indigo_property *property) {
	indigo_property *cached_property = find_cached_property(property->name);
	if (cached_property != NULL) {
		for (int i = 0; i < MAX_DEFINED_PROPERTIES; i++) {
			if (context.cached_properties[i] == cached_property) {
				indigo_release_property(context.cached_properties[i]);
				context.cached_properties[i] = indigo_copy_property(NULL, property);
				ASSERT_TRUE(context.cached_properties[i] != NULL);
				break;
			}
		}
		return;
	}
	for (int i = 0; i < MAX_DEFINED_PROPERTIES; i++) {
		if (context.cached_properties[i] == NULL) {
			context.cached_properties[i] = indigo_copy_property(NULL, property);
			ASSERT_TRUE(context.cached_properties[i] != NULL);
			return;
		}
	}
	ASSERT_TRUE(false);
}

static void cache_property_update(indigo_property *property) {
	indigo_property *cached_property = find_cached_property(property->name);
	if (cached_property == NULL) {
		return;
	}
	cached_property->state = property->state;
	cached_property->access_token = property->access_token;
	for (int i = 0; i < property->count; i++) {
		indigo_item *item = property->items + i;
		indigo_item *cached_item = find_cached_item(property->name, item->name);
		if (cached_item == NULL) {
			continue;
		}
		switch (cached_property->type) {
			case INDIGO_TEXT_VECTOR:
				indigo_set_text_item_value(cached_item, indigo_get_text_item_value(item));
				break;
			case INDIGO_NUMBER_VECTOR:
				cached_item->number = item->number;
				break;
			case INDIGO_SWITCH_VECTOR:
				cached_item->sw = item->sw;
				break;
			case INDIGO_LIGHT_VECTOR:
				cached_item->light = item->light;
				break;
			case INDIGO_BLOB_VECTOR:
				cached_item->blob = item->blob;
				break;
		}
	}
}

static void uncache_property(indigo_property *property) {
	for (int i = 0; i < MAX_DEFINED_PROPERTIES; i++) {
		if (context.cached_properties[i] != NULL && !strcmp(context.cached_properties[i]->name, property->name)) {
			indigo_release_property(context.cached_properties[i]);
			context.cached_properties[i] = NULL;
			return;
		}
	}
}

static void release_cached_properties(void) {
	for (int i = 0; i < MAX_DEFINED_PROPERTIES; i++) {
		if (context.cached_properties[i] != NULL) {
			indigo_release_property(context.cached_properties[i]);
			context.cached_properties[i] = NULL;
		}
	}
}

static void record_defined_property(const char *name) {
	if (has_defined_property(name)) {
		return;
	}
	ASSERT_TRUE(context.defined_property_count < MAX_DEFINED_PROPERTIES);
	strncpy(context.defined_properties[context.defined_property_count], name, INDIGO_NAME_SIZE - 1);
	context.defined_properties[context.defined_property_count][INDIGO_NAME_SIZE - 1] = 0;
	context.defined_property_count++;
}

static void assert_defined_property(const char *name) {
	if (!has_defined_property(name)) {
		fprintf(stderr, "Missing %s property on %s: %s\n", context.driver_case->label, context.driver_case->device_name, name);
	}
	ASSERT_TRUE(has_defined_property(name));
}

static void assert_not_defined_property(const char *name) {
	if (has_defined_property(name)) {
		fprintf(stderr, "Unexpected %s property on %s: %s\n", context.driver_case->label, context.driver_case->device_name, name);
	}
	ASSERT_FALSE(has_defined_property(name));
}

static void assert_defined_properties(const char * const *names, int count) {
	for (int i = 0; i < count; i++) {
		assert_defined_property(names[i]);
	}
}

static void assert_not_defined_properties(const char * const *names, int count) {
	for (int i = 0; i < count; i++) {
		assert_not_defined_property(names[i]);
	}
}

static void assert_defined_property_count(int expected_count) {
	if (context.defined_property_count != expected_count) {
		fprintf(stderr, "Expected %d %s properties on %s, got %d:\n", expected_count, context.driver_case->label, context.driver_case->device_name, context.defined_property_count);
		for (int i = 0; i < context.defined_property_count; i++) {
			fprintf(stderr, "  %s\n", context.defined_properties[i]);
		}
	}
	ASSERT_EQ_INT(expected_count, context.defined_property_count);
}

static void assert_property_has_item(const char *property_name, const char *item_name) {
	if (find_cached_item(property_name, item_name) == NULL) {
		fprintf(stderr, "Missing item %s.%s on %s\n", property_name, item_name, context.driver_case->device_name);
	}
	ASSERT_TRUE(find_cached_item(property_name, item_name) != NULL);
}

static void assert_property_has_items(const char *property_name, const char * const *item_names, int count) {
	assert_defined_property(property_name);
	for (int i = 0; i < count; i++) {
		assert_property_has_item(property_name, item_names[i]);
	}
}

static void assert_device_interface(unsigned int expected_interface) {
	indigo_item *item = find_cached_item(INFO_PROPERTY_NAME, INFO_DEVICE_INTERFACE_ITEM_NAME);
	ASSERT_TRUE(item != NULL);
	ASSERT_TRUE(((unsigned int)strtoul(item->text.value, NULL, 10) & expected_interface) != 0);
}

static void assert_number_item_in_range(const char *property_name, const char *item_name) {
	indigo_item *item = find_cached_item(property_name, item_name);
	if (item == NULL) {
		fprintf(stderr, "Missing number item %s.%s on %s\n", property_name, item_name, context.driver_case->device_name);
	}
	ASSERT_TRUE(item != NULL);
	if (item->number.min > item->number.value || item->number.value > item->number.max) {
		fprintf(stderr, "Number item %s.%s on %s is out of range: min=%g value=%g max=%g\n", property_name, item_name, context.driver_case->device_name, item->number.min, item->number.value, item->number.max);
	}
	ASSERT_TRUE(item->number.min <= item->number.value);
	ASSERT_TRUE(item->number.value <= item->number.max);
}

static double cached_number_value(const char *property_name, const char *item_name) {
	indigo_item *item = find_cached_item(property_name, item_name);
	if (item == NULL) {
		return NAN;
	}
	return item->number.value;
}

static void assert_switch_item_value(const char *property_name, const char *item_name, bool expected_value) {
	indigo_item *item = find_cached_item(property_name, item_name);
	if (item == NULL) {
		fprintf(stderr, "Missing switch item %s.%s on %s\n", property_name, item_name, context.driver_case->device_name);
	}
	ASSERT_TRUE(item != NULL);
	ASSERT_EQ_INT(expected_value, item->sw.value);
}

static void assert_any_light_item_active(const char *property_name, const char * const *item_names, int count) {
	bool active = false;
	for (int i = 0; i < count; i++) {
		indigo_item *item = find_cached_item(property_name, item_names[i]);
		ASSERT_TRUE(item != NULL);
		active |= item->light.value != INDIGO_IDLE_STATE;
	}
	ASSERT_TRUE(active);
}

static bool wait_for_property_state(const char *property_name, indigo_property_state state) {
	for (int i = 0; i < 100; i++) {
		indigo_property *property = find_cached_property(property_name);
		if (property != NULL && property->state == state) {
			return true;
		}
		indigo_usleep(100000);
	}
	return false;
}

static bool wait_for_property_not_busy(const char *property_name) {
	for (int i = 0; i < 100; i++) {
		indigo_property *property = find_cached_property(property_name);
		if (property != NULL && property->state != INDIGO_BUSY_STATE) {
			return true;
		}
		indigo_usleep(100000);
	}
	return false;
}

static bool wait_for_number_item_value(const char *property_name, const char *item_name, double value, double tolerance) {
	for (int i = 0; i < 100; i++) {
		indigo_item *item = find_cached_item(property_name, item_name);
		if (item != NULL && fabs(item->number.value - value) <= tolerance) {
			return true;
		}
		indigo_usleep(100000);
	}
	return false;
}

static indigo_result simulator_client_define_property(indigo_client *client, indigo_device *device, indigo_property *property, const char *message) {
	if (context.driver_case != NULL && !strcmp(property->device, context.driver_case->device_name)) {
		context.define_count++;
		record_defined_property(property->name);
		cache_property(property);
	}
	return INDIGO_OK;
}

static indigo_result simulator_client_update_property(indigo_client *client, indigo_device *device, indigo_property *property, const char *message) {
	if (context.driver_case != NULL && !strcmp(property->device, context.driver_case->device_name)) {
		cache_property_update(property);
	}
	if (context.driver_case != NULL && !strcmp(property->device, context.driver_case->device_name) && !strcmp(property->name, CONNECTION_PROPERTY_NAME)) {
		context.update_count++;
		context.last_connection_state = property->state;
		for (int i = 0; i < property->count; i++) {
			if (!strcmp(property->items[i].name, CONNECTION_CONNECTED_ITEM_NAME)) {
				context.connected = property->items[i].sw.value;
			} else if (!strcmp(property->items[i].name, CONNECTION_DISCONNECTED_ITEM_NAME)) {
				context.disconnected = property->items[i].sw.value;
			}
		}
	}
	return INDIGO_OK;
}

static indigo_result simulator_client_delete_property(indigo_client *client, indigo_device *device, indigo_property *property, const char *message) {
	if (context.driver_case != NULL && !strcmp(property->device, context.driver_case->device_name)) {
		uncache_property(property);
	}
	return INDIGO_OK;
}

static indigo_client simulator_test_client = {
	"Simulator Driver Test Client",
	false,
	NULL,
	INDIGO_OK,
	INDIGO_VERSION_CURRENT,
	NULL,
	NULL,
	simulator_client_define_property,
	simulator_client_update_property,
	simulator_client_delete_property,
	NULL,
	NULL,
	false,
	false
};

static void reset_simulator_context(const simulator_driver_case *driver_case) {
	release_cached_properties();
	memset(&context, 0, sizeof(context));
	context.driver_case = driver_case;
	simulator_test_client.version = INDIGO_VERSION_CURRENT;
	simulator_test_client.last_result = INDIGO_OK;
}

static bool wait_for_simulator_connection_state(bool connected) {
	for (int i = 0; i < 50; i++) {
		if (connected) {
			if (context.connected && !context.disconnected && context.last_connection_state == INDIGO_OK_STATE) {
				return true;
			}
		} else {
			if (!context.connected && context.disconnected && context.last_connection_state == INDIGO_OK_STATE) {
				return true;
			}
		}
		indigo_usleep(100000);
	}
	return false;
}

static void enumerate_simulator_device(void) {
	indigo_property *selector = indigo_init_text_property(NULL, context.driver_case->device_name, "", "", "", INDIGO_OK_STATE, INDIGO_RO_PERM, 0);
	ASSERT_TRUE(selector != NULL);
	ASSERT_EQ_INT(INDIGO_OK, indigo_enumerate_properties(&simulator_test_client, selector));
	indigo_release_property(selector);
}

static void assert_simulator_driver_info(const simulator_driver_case *driver_case) {
	indigo_driver_info info;
	memset(&info, 0, sizeof(info));

	ASSERT_EQ_INT(INDIGO_OK, driver_case->entry(INDIGO_DRIVER_INFO, &info));
	ASSERT_STREQ(driver_case->label, info.description);
	ASSERT_STREQ(driver_case->driver_name, info.name);
	ASSERT_EQ_INT(driver_case->multi_device_support, info.multi_device_support);
	ASSERT_EQ_INT(INDIGO_DRIVER_SHUTDOWN, info.status);
}

static void assert_simulator_properties(const simulator_driver_case *driver_case) {
	reset_simulator_context(driver_case);

	ASSERT_EQ_INT(INDIGO_OK, indigo_start());
	ASSERT_EQ_INT(INDIGO_OK, indigo_attach_client(&simulator_test_client));
	ASSERT_EQ_INT(INDIGO_OK, driver_case->entry(INDIGO_DRIVER_INIT, NULL));

	enumerate_simulator_device();
	ASSERT_TRUE(context.define_count > 0);
	assert_defined_properties(driver_case->base_properties, driver_case->base_property_count);
	assert_not_defined_properties(driver_case->hidden_base_properties, driver_case->hidden_base_property_count);
	assert_not_defined_properties(driver_case->hidden_connected_properties, driver_case->hidden_connected_property_count);
	assert_defined_property_count(driver_case->base_property_count);

	ASSERT_EQ_INT(INDIGO_OK, indigo_change_switch_property_1(&simulator_test_client, driver_case->device_name, CONNECTION_PROPERTY_NAME, CONNECTION_CONNECTED_ITEM_NAME, true));
	ASSERT_TRUE(wait_for_simulator_connection_state(true));
	assert_defined_properties(driver_case->base_properties, driver_case->base_property_count);
	assert_defined_properties(driver_case->connected_properties, driver_case->connected_property_count);
	assert_not_defined_properties(driver_case->hidden_base_properties, driver_case->hidden_base_property_count);
	assert_not_defined_properties(driver_case->hidden_connected_properties, driver_case->hidden_connected_property_count);
	assert_defined_property_count(driver_case->base_property_count + driver_case->connected_property_count);

	ASSERT_EQ_INT(INDIGO_OK, indigo_change_switch_property_1(&simulator_test_client, driver_case->device_name, CONNECTION_PROPERTY_NAME, CONNECTION_DISCONNECTED_ITEM_NAME, true));
	ASSERT_TRUE(wait_for_simulator_connection_state(false));
	ASSERT_EQ_INT(INDIGO_OK, indigo_change_switch_property_1(&simulator_test_client, driver_case->device_name, CONNECTION_PROPERTY_NAME, CONNECTION_DISCONNECTED_ITEM_NAME, true));
	ASSERT_TRUE(wait_for_simulator_connection_state(false));

	ASSERT_EQ_INT(INDIGO_OK, driver_case->entry(INDIGO_DRIVER_SHUTDOWN, NULL));
	ASSERT_EQ_INT(INDIGO_OK, indigo_detach_client(&simulator_test_client));
	ASSERT_EQ_INT(INDIGO_OK, indigo_stop());
}

static void start_connected_simulator(const simulator_driver_case *driver_case) {
	reset_simulator_context(driver_case);

	ASSERT_EQ_INT(INDIGO_OK, indigo_start());
	ASSERT_EQ_INT(INDIGO_OK, indigo_attach_client(&simulator_test_client));
	ASSERT_EQ_INT(INDIGO_OK, driver_case->entry(INDIGO_DRIVER_INIT, NULL));
	enumerate_simulator_device();
	ASSERT_EQ_INT(INDIGO_OK, indigo_change_switch_property_1(&simulator_test_client, driver_case->device_name, CONNECTION_PROPERTY_NAME, CONNECTION_CONNECTED_ITEM_NAME, true));
	ASSERT_TRUE(wait_for_simulator_connection_state(true));
}

static void stop_connected_simulator(const simulator_driver_case *driver_case) {
	ASSERT_EQ_INT(INDIGO_OK, indigo_change_switch_property_1(&simulator_test_client, driver_case->device_name, CONNECTION_PROPERTY_NAME, CONNECTION_DISCONNECTED_ITEM_NAME, true));
	ASSERT_TRUE(wait_for_simulator_connection_state(false));
	ASSERT_EQ_INT(INDIGO_OK, driver_case->entry(INDIGO_DRIVER_SHUTDOWN, NULL));
	ASSERT_EQ_INT(INDIGO_OK, indigo_detach_client(&simulator_test_client));
	ASSERT_EQ_INT(INDIGO_OK, indigo_stop());
	release_cached_properties();
}

#endif /* simulator_test_common_h */
