// Copyright (c) 2025 CloudMakers, s. r. o.
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <signal.h>

#define OS_NAME "Windows"

#if defined(__x86_64__) || defined(_M_X64)
#define ARCH_NAME "x86_64"
#elif defined(__i386) || defined(_M_IX86)
#define ARCH_NAME "x86"
#elif defined(__aarch64__)
#define ARCH_NAME "arm64"
#elif defined(__arm__) || defined(_M_ARM)
#define ARCH_NAME "armhf"
#else
#define ARCH_NAME "unknown arch"
#endif

#if defined(INDIGO_WINDOWS)
#define INDIGO_EXTERN	extern __declspec(dllimport)
#else
#define INDIGO_EXTERN	extern	
#endif

#include <indigo/indigo_bus.h>
#include <indigo/indigo_uni_io.h>
#include <indigo/indigo_server_tcp.h>
#include <indigo/indigo_driver.h>
#include <indigo/indigo_client.h>
#include <indigo/indigo_xml.h>
#include <indigo/indigo_token.h>

#include "ccd_simulator/indigo_ccd_simulator.h"
#include "mount_simulator/indigo_mount_simulator.h"
#include "gps_simulator/indigo_gps_simulator.h"
#include "dome_simulator/indigo_dome_simulator.h"
#include "rotator_simulator/indigo_rotator_simulator.h"

#include "agent_imager/indigo_agent_imager.h"
#include "agent_mount/indigo_agent_mount.h"
#include "agent_guider/indigo_agent_guider.h"
#include "agent_auxiliary/indigo_agent_auxiliary.h"
#include "wheel_manual/indigo_wheel_manual.h"
#include "agent_alpaca/indigo_agent_alpaca.h"
#include "agent_config/indigo_agent_config.h"
#include "agent_scripting/indigo_agent_scripting.h"

#define SERVER_NAME         "INDIGO Server"

driver_entry_point static_drivers[] = {
	indigo_agent_auxiliary,
	indigo_agent_guider,
	indigo_agent_imager,
	indigo_agent_config,
	indigo_agent_scripting,
	indigo_agent_mount,
	indigo_ccd_simulator,
	indigo_dome_simulator,
	indigo_gps_simulator,
	indigo_mount_simulator,
	indigo_rotator_simulator,
	NULL
};

static struct {
	char* name;
	char* description;
} dynamic_drivers[INDIGO_MAX_DRIVERS] = {
	NULL
};

static int static_drivers_count = 0;
static int dynamic_drivers_count = 0;
static bool command_line_drivers = false;

static indigo_property* info_property;
static indigo_property* drivers_property;
static indigo_property* servers_property;
static indigo_property* load_property;
static indigo_property* unload_property;
static indigo_property* restart_property;
static indigo_property* log_level_property;
static indigo_property* blob_buffering_property;
static indigo_property* blob_proxy_property;
static indigo_property* server_features_property;

static void* star_data = NULL;
static void* dso_data = NULL;
static void* constellation_data = NULL;

#define SERVER_INFO_PROPERTY											info_property
#define SERVER_INFO_VERSION_ITEM									(SERVER_INFO_PROPERTY->items + 0)
#define SERVER_INFO_SERVICE_ITEM									(SERVER_INFO_PROPERTY->items + 1)

#define SERVER_DRIVERS_PROPERTY										drivers_property

#define SERVER_SERVERS_PROPERTY										servers_property

#define SERVER_LOAD_PROPERTY											load_property
#define SERVER_LOAD_ITEM													(SERVER_LOAD_PROPERTY->items + 0)

#define SERVER_UNLOAD_PROPERTY										unload_property
#define SERVER_UNLOAD_ITEM												(SERVER_UNLOAD_PROPERTY->items + 0)

#define SERVER_LOG_LEVEL_PROPERTY									log_level_property
#define SERVER_LOG_LEVEL_ERROR_ITEM								(SERVER_LOG_LEVEL_PROPERTY->items + 0)
#define SERVER_LOG_LEVEL_INFO_ITEM								(SERVER_LOG_LEVEL_PROPERTY->items + 1)
#define SERVER_LOG_LEVEL_DEBUG_ITEM								(SERVER_LOG_LEVEL_PROPERTY->items + 2)
#define SERVER_LOG_LEVEL_TRACE_BUS_ITEM						(SERVER_LOG_LEVEL_PROPERTY->items + 3)
#define SERVER_LOG_LEVEL_TRACE_ITEM								(SERVER_LOG_LEVEL_PROPERTY->items + 4)

#define SERVER_BLOB_BUFFERING_PROPERTY						blob_buffering_property
#define SERVER_BLOB_BUFFERING_DISABLED_ITEM				(SERVER_BLOB_BUFFERING_PROPERTY->items + 0)
#define SERVER_BLOB_BUFFERING_ENABLED_ITEM				(SERVER_BLOB_BUFFERING_PROPERTY->items + 1)
#define SERVER_BLOB_COMPRESSION_ENABLED_ITEM			(SERVER_BLOB_BUFFERING_PROPERTY->items + 2)

#define SERVER_BLOB_PROXY_PROPERTY								blob_proxy_property
#define SERVER_BLOB_PROXY_DISABLED_ITEM						(SERVER_BLOB_PROXY_PROPERTY->items + 0)
#define SERVER_BLOB_PROXY_ENABLED_ITEM						(SERVER_BLOB_PROXY_PROPERTY->items + 1)

static char const* server_argv[128];
static int server_argc = 1;

static indigo_result attach(indigo_device* device);
static indigo_result enumerate_properties(indigo_device* device, indigo_client* client, indigo_property* property);
static indigo_result change_property(indigo_device* device, indigo_client* client, indigo_property* property);
static indigo_result detach(indigo_device* device);

static indigo_device server_device = INDIGO_DEVICE_INITIALIZER(
	"Server",
	attach,
	enumerate_properties,
	change_property,
	NULL,
	detach
);

static indigo_result attach(indigo_device* device) {
	assert(device != NULL);
	char hostname[INDIGO_NAME_SIZE];
	gethostname(hostname, sizeof(hostname));
	if (*indigo_local_service_name == 0) {
		indigo_service_name(hostname, indigo_server_tcp_port, indigo_local_service_name);
	}
	SERVER_INFO_PROPERTY = indigo_init_text_property(NULL, server_device.name, SERVER_INFO_PROPERTY_NAME, MAIN_GROUP, "Server info", INDIGO_OK_STATE, INDIGO_RO_PERM, 2);
	indigo_init_text_item(SERVER_INFO_VERSION_ITEM, SERVER_INFO_VERSION_ITEM_NAME, "INDIGO version", "%d.%d-%s", INDIGO_VERSION_MAJOR(INDIGO_VERSION_CURRENT), INDIGO_VERSION_MINOR(INDIGO_VERSION_CURRENT), INDIGO_BUILD);
	indigo_init_text_item(SERVER_INFO_SERVICE_ITEM, SERVER_INFO_SERVICE_ITEM_NAME, "INDIGO service", indigo_local_service_name);
	SERVER_DRIVERS_PROPERTY = indigo_init_switch_property(NULL, server_device.name, SERVER_DRIVERS_PROPERTY_NAME, MAIN_GROUP, "Available drivers", INDIGO_OK_STATE, INDIGO_RW_PERM, INDIGO_ANY_OF_MANY_RULE, INDIGO_MAX_DRIVERS);
	SERVER_DRIVERS_PROPERTY->count = 0;
	for (int i = 0; i < INDIGO_MAX_DRIVERS; i++)
		if (indigo_available_drivers[i].driver != NULL)
			indigo_init_switch_item(&SERVER_DRIVERS_PROPERTY->items[SERVER_DRIVERS_PROPERTY->count++], indigo_available_drivers[i].name, indigo_available_drivers[i].description, indigo_available_drivers[i].initialized);
	for (int i = 0; i < dynamic_drivers_count && SERVER_DRIVERS_PROPERTY->count < INDIGO_MAX_DRIVERS; i++)
		indigo_init_switch_item(&SERVER_DRIVERS_PROPERTY->items[SERVER_DRIVERS_PROPERTY->count++], dynamic_drivers[i].name, dynamic_drivers[i].description, false);
	indigo_property_sort_items(SERVER_DRIVERS_PROPERTY, 0);
	SERVER_SERVERS_PROPERTY = indigo_init_light_property(NULL, server_device.name, SERVER_SERVERS_PROPERTY_NAME, MAIN_GROUP, "Configured servers", INDIGO_OK_STATE, 2 * INDIGO_MAX_SERVERS);
	SERVER_SERVERS_PROPERTY->count = 0;
	for (int i = 0; i < INDIGO_MAX_SERVERS; i++) {
		indigo_server_entry* entry = indigo_available_servers + i;
		if (*entry->host) {
			char buf[INDIGO_NAME_SIZE];
			if (entry->port == 7624)
				strncpy(buf, entry->host, sizeof(buf));
			else
				snprintf(buf, sizeof(buf), "%s:%d", entry->host, entry->port);
			indigo_init_light_item(&SERVER_SERVERS_PROPERTY->items[SERVER_SERVERS_PROPERTY->count++], buf, buf, INDIGO_OK_STATE);
		}
	}
	indigo_property_sort_items(SERVER_SERVERS_PROPERTY, 0);
	SERVER_LOAD_PROPERTY = indigo_init_text_property(NULL, server_device.name, SERVER_LOAD_PROPERTY_NAME, MAIN_GROUP, "Load driver", INDIGO_OK_STATE, INDIGO_RW_PERM, 1);
	indigo_init_text_item(SERVER_LOAD_ITEM, SERVER_LOAD_ITEM_NAME, "Load driver", "");
	SERVER_UNLOAD_PROPERTY = indigo_init_text_property(NULL, server_device.name, SERVER_UNLOAD_PROPERTY_NAME, MAIN_GROUP, "Unload driver", INDIGO_OK_STATE, INDIGO_RW_PERM, 1);
	indigo_init_text_item(SERVER_UNLOAD_ITEM, SERVER_UNLOAD_ITEM_NAME, "Unload driver", "");
	SERVER_LOG_LEVEL_PROPERTY = indigo_init_switch_property(NULL, device->name, SERVER_LOG_LEVEL_PROPERTY_NAME, MAIN_GROUP, "Log level", INDIGO_OK_STATE, INDIGO_RW_PERM, INDIGO_ONE_OF_MANY_RULE, 5);
	indigo_init_switch_item(SERVER_LOG_LEVEL_ERROR_ITEM, SERVER_LOG_LEVEL_ERROR_ITEM_NAME, "Error", false);
	indigo_init_switch_item(SERVER_LOG_LEVEL_INFO_ITEM, SERVER_LOG_LEVEL_INFO_ITEM_NAME, "Info", false);
	indigo_init_switch_item(SERVER_LOG_LEVEL_DEBUG_ITEM, SERVER_LOG_LEVEL_DEBUG_ITEM_NAME, "Debug", false);
	indigo_init_switch_item(SERVER_LOG_LEVEL_TRACE_BUS_ITEM, SERVER_LOG_LEVEL_TRACE_BUS_ITEM_NAME, "Trace bus", false);
	indigo_init_switch_item(SERVER_LOG_LEVEL_TRACE_ITEM, SERVER_LOG_LEVEL_TRACE_ITEM_NAME, "Trace", false);
	SERVER_BLOB_BUFFERING_PROPERTY = indigo_init_switch_property(NULL, device->name, SERVER_BLOB_BUFFERING_PROPERTY_NAME, MAIN_GROUP, "BLOB buffering", INDIGO_OK_STATE, INDIGO_RW_PERM, INDIGO_ONE_OF_MANY_RULE, 3);
	indigo_init_switch_item(SERVER_BLOB_BUFFERING_DISABLED_ITEM, SERVER_BLOB_BUFFERING_DISABLED_ITEM_NAME, "Disabled", !indigo_use_blob_buffering);
	indigo_init_switch_item(SERVER_BLOB_BUFFERING_ENABLED_ITEM, SERVER_BLOB_BUFFERING_ENABLED_ITEM_NAME, "Enabled", indigo_use_blob_buffering && !indigo_use_blob_compression);
	indigo_init_switch_item(SERVER_BLOB_COMPRESSION_ENABLED_ITEM, SERVER_BLOB_COMPRESSION_ENABLED_ITEM_NAME, "Enabled with compression", indigo_use_blob_buffering && indigo_use_blob_compression);
	SERVER_BLOB_PROXY_PROPERTY = indigo_init_switch_property(NULL, device->name, SERVER_BLOB_PROXY_PROPERTY_NAME, MAIN_GROUP, "BLOB proxy", INDIGO_OK_STATE, INDIGO_RW_PERM, INDIGO_ONE_OF_MANY_RULE, 2);
	indigo_init_switch_item(SERVER_BLOB_PROXY_DISABLED_ITEM, SERVER_BLOB_PROXY_DISABLED_ITEM_NAME, "Disabled", !indigo_proxy_blob);
	indigo_init_switch_item(SERVER_BLOB_PROXY_ENABLED_ITEM, SERVER_BLOB_PROXY_ENABLED_ITEM_NAME, "Enabled", indigo_proxy_blob);
	indigo_log_levels log_level = indigo_get_log_level();
	switch (log_level) {
	case INDIGO_LOG_ERROR:
		SERVER_LOG_LEVEL_ERROR_ITEM->sw.value = true;
		break;
	case INDIGO_LOG_INFO:
		SERVER_LOG_LEVEL_INFO_ITEM->sw.value = true;
		break;
	case INDIGO_LOG_DEBUG:
		SERVER_LOG_LEVEL_DEBUG_ITEM->sw.value = true;
		break;
	case INDIGO_LOG_TRACE_BUS:
		SERVER_LOG_LEVEL_TRACE_BUS_ITEM->sw.value = true;
		break;
	case INDIGO_LOG_TRACE:
		SERVER_LOG_LEVEL_TRACE_ITEM->sw.value = true;
		break;
	default:
		break;
	}
	if (!command_line_drivers)
		indigo_load_properties(device, false);
	INDIGO_LOG(indigo_log("%s attached", device->name));
	return INDIGO_OK;
}

static indigo_result enumerate_properties(indigo_device* device, indigo_client* client, indigo_property* property) {
	assert(device != NULL);
	indigo_define_property(device, SERVER_INFO_PROPERTY, NULL);
	indigo_define_property(device, SERVER_DRIVERS_PROPERTY, NULL);
	if (SERVER_SERVERS_PROPERTY->count > 0) {
		indigo_define_property(device, SERVER_SERVERS_PROPERTY, NULL);
	}
	indigo_define_property(device, SERVER_LOAD_PROPERTY, NULL);
	indigo_define_property(device, SERVER_UNLOAD_PROPERTY, NULL);
	indigo_define_property(device, SERVER_LOG_LEVEL_PROPERTY, NULL);
	indigo_define_property(device, SERVER_BLOB_BUFFERING_PROPERTY, NULL);
	indigo_define_property(device, SERVER_BLOB_PROXY_PROPERTY, NULL);
	return INDIGO_OK;
}

static void send_driver_load_error_message(indigo_result result, char* driver_name) {
	if (result == INDIGO_UNSUPPORTED_ARCH) {
		indigo_send_message(&server_device, "Driver '%s' failed to load: not supported on this architecture", driver_name);
	} else if (result == INDIGO_UNRESOLVED_DEPS) {
		indigo_send_message(&server_device, "Driver '%s' failed to load: unresolved dependencies", driver_name);
	} else if (result != INDIGO_OK) {
		indigo_send_message(&server_device, "Driver '%s' failed to load", driver_name);
	}
}

static indigo_result change_property(indigo_device* device, indigo_client* client, indigo_property* property) {
	assert(device != NULL);
	assert(property != NULL);
	if (indigo_property_match(SERVER_DRIVERS_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- DRIVERS
		if (command_line_drivers && !strcmp(client->name, CONFIG_READER)) {
			return INDIGO_OK;
		}
		indigo_property_copy_values(SERVER_DRIVERS_PROPERTY, property, false);
		for (int i = 0; i < SERVER_DRIVERS_PROPERTY->count; i++) {
			char* name = SERVER_DRIVERS_PROPERTY->items[i].name;
			indigo_driver_entry* driver = NULL;
			for (int j = 0; j < INDIGO_MAX_DRIVERS; j++) {
				if (!strcmp(indigo_available_drivers[j].name, name)) {
					driver = &indigo_available_drivers[j];
					break;
				}
			}
			if (SERVER_DRIVERS_PROPERTY->items[i].sw.value) {
				if (driver) {
					if (driver->dl_handle == NULL && !driver->initialized) {
						indigo_result result = driver->driver(INDIGO_DRIVER_INIT, NULL);
						SERVER_DRIVERS_PROPERTY->items[i].sw.value = driver->initialized = result == INDIGO_OK;
						send_driver_load_error_message(result, driver->name);
					} else if (driver->dl_handle != NULL && !driver->initialized) {
						indigo_result result = driver->driver(INDIGO_DRIVER_INIT, NULL);
						SERVER_DRIVERS_PROPERTY->items[i].sw.value = driver->initialized = result == INDIGO_OK;
						send_driver_load_error_message(result, driver->name);
						if (driver && !driver->initialized)
							indigo_remove_driver(driver);
					}
				} else {
					indigo_result result = indigo_load_driver(name, true, &driver);
					SERVER_DRIVERS_PROPERTY->items[i].sw.value = result == INDIGO_OK;
					if (driver && !driver->initialized) {
						indigo_remove_driver(driver);
					}
					send_driver_load_error_message(result, name);
				}
			} else if (driver) {
				indigo_result result = INDIGO_OK;
				if (driver->dl_handle) {
					result = indigo_remove_driver(driver);
					if (result != INDIGO_OK) {
						SERVER_DRIVERS_PROPERTY->items[i].sw.value = true;
					}
				} else if (driver->initialized) {
					result = driver->driver(INDIGO_DRIVER_SHUTDOWN, NULL);
					if (result != INDIGO_OK) {
						SERVER_DRIVERS_PROPERTY->items[i].sw.value = true;
					} else {
						driver->initialized = false;
					}
				}
				if (result != INDIGO_OK) {
					if (result == INDIGO_BUSY) {
						indigo_send_message(device, "Driver %s is in use, can't be unloaded", name);
					} else {
						indigo_send_message(device, "Driver %s failed to unload", name);
					}
				}
			}
		}
		SERVER_DRIVERS_PROPERTY->state = INDIGO_OK_STATE;
		indigo_update_property(device, SERVER_DRIVERS_PROPERTY, NULL);
		indigo_uni_handle* handle = { 0 };
		if (!command_line_drivers) {
			indigo_save_property(device, &handle, SERVER_DRIVERS_PROPERTY);
			indigo_uni_close(&handle);
		}
		return INDIGO_OK;
	} else if (indigo_property_match(SERVER_LOAD_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- LOAD
		indigo_property_copy_values(SERVER_LOAD_PROPERTY, property, false);
		if (*SERVER_LOAD_ITEM->text.value) {
			char* name = indigo_uni_basename(SERVER_LOAD_ITEM->text.value);
			for (int i = 0; i < INDIGO_MAX_DRIVERS; i++)
				if (!strcmp(name, indigo_available_drivers[i].name)) {
					SERVER_LOAD_PROPERTY->state = INDIGO_ALERT_STATE;
					indigo_update_property(device, SERVER_LOAD_PROPERTY, "Driver %s (%s) is already loaded", name, indigo_available_drivers[i].description);
					return INDIGO_OK;
				}
			indigo_driver_entry* driver = NULL;
			indigo_result result = INDIGO_OK;
			if ((result = indigo_load_driver(SERVER_LOAD_ITEM->text.value, true, &driver)) == INDIGO_OK) {
				bool found = false;
				for (int i = 0; i < SERVER_DRIVERS_PROPERTY->count; i++) {
					if (!strcmp(SERVER_DRIVERS_PROPERTY->items[i].name, name)) {
						SERVER_DRIVERS_PROPERTY->items[i].sw.value = true;
						SERVER_DRIVERS_PROPERTY->state = INDIGO_OK_STATE;
						indigo_update_property(device, SERVER_DRIVERS_PROPERTY, NULL);
						found = true;
						break;
					}
				}
				if (!found) {
					indigo_delete_property(device, SERVER_DRIVERS_PROPERTY, NULL);
					indigo_init_switch_item(&SERVER_DRIVERS_PROPERTY->items[SERVER_DRIVERS_PROPERTY->count++], driver->name, driver->description, driver->initialized);
					SERVER_DRIVERS_PROPERTY->state = INDIGO_OK_STATE;
					indigo_define_property(device, SERVER_DRIVERS_PROPERTY, NULL);
				}
				SERVER_LOAD_PROPERTY->state = INDIGO_OK_STATE;
				indigo_update_property(device, SERVER_LOAD_PROPERTY, "Driver %s (%s) loaded", name, driver->description);
			} else {
				SERVER_LOAD_PROPERTY->state = INDIGO_ALERT_STATE;
				if (driver && !driver->initialized) {
					indigo_remove_driver(driver);
				}
				send_driver_load_error_message(result, name);
				indigo_update_property(device, SERVER_LOAD_PROPERTY, NULL);
			}
		}
		return INDIGO_OK;
	} else if (indigo_property_match(SERVER_UNLOAD_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- UNLOAD
		indigo_property_copy_values(SERVER_UNLOAD_PROPERTY, property, false);
		if (*SERVER_UNLOAD_ITEM->text.value) {
			char* name = indigo_uni_basename(SERVER_UNLOAD_ITEM->text.value);
			for (int i = 0; i < INDIGO_MAX_DRIVERS; i++)
				if (!strcmp(name, indigo_available_drivers[i].name)) {
					indigo_result result;
					if (indigo_available_drivers[i].dl_handle) {
						result = indigo_remove_driver(&indigo_available_drivers[i]);
					} else {
						result = indigo_available_drivers[i].driver(INDIGO_DRIVER_SHUTDOWN, NULL);
					}
					if (result == INDIGO_OK) {
						for (int j = 0; j < SERVER_DRIVERS_PROPERTY->count; j++) {
							if (!strcmp(SERVER_DRIVERS_PROPERTY->items[j].name, name)) {
								SERVER_DRIVERS_PROPERTY->items[j].sw.value = false;
								SERVER_DRIVERS_PROPERTY->state = INDIGO_OK_STATE;
								indigo_update_property(device, SERVER_DRIVERS_PROPERTY, NULL);
								break;
							}
						}
						SERVER_UNLOAD_PROPERTY->state = INDIGO_OK_STATE;
						indigo_update_property(device, SERVER_UNLOAD_PROPERTY, "Driver %s unloaded", name);
					}
					else if (result == INDIGO_BUSY) {
						SERVER_UNLOAD_PROPERTY->state = INDIGO_ALERT_STATE;
						indigo_update_property(device, SERVER_UNLOAD_PROPERTY, "Driver %s is in use, can't be unloaded", name);
					} else {
						SERVER_UNLOAD_PROPERTY->state = INDIGO_ALERT_STATE;
						indigo_update_property(device, SERVER_UNLOAD_PROPERTY, "Driver %s failed to unload", name);
					}
					return INDIGO_OK;
				}
			SERVER_UNLOAD_PROPERTY->state = INDIGO_ALERT_STATE;
			indigo_update_property(device, SERVER_UNLOAD_PROPERTY, "Driver %s is not loaded", name);
		}
		return INDIGO_OK;
	}	else if (indigo_property_match(SERVER_LOG_LEVEL_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- LOG_LEVEL
		indigo_property_copy_values(SERVER_LOG_LEVEL_PROPERTY, property, false);
		if (SERVER_LOG_LEVEL_ERROR_ITEM->sw.value) {
			indigo_set_log_level(INDIGO_LOG_ERROR);
		} else if (SERVER_LOG_LEVEL_INFO_ITEM->sw.value) {
			indigo_set_log_level(INDIGO_LOG_INFO);
		} else if (SERVER_LOG_LEVEL_DEBUG_ITEM->sw.value) {
			indigo_set_log_level(INDIGO_LOG_DEBUG);
		} else if (SERVER_LOG_LEVEL_TRACE_BUS_ITEM->sw.value) {
			indigo_set_log_level(INDIGO_LOG_TRACE_BUS);
		} else if (SERVER_LOG_LEVEL_TRACE_ITEM->sw.value) {
			indigo_set_log_level(INDIGO_LOG_TRACE);
		}
		SERVER_LOG_LEVEL_PROPERTY->state = INDIGO_OK_STATE;
		indigo_update_property(device, SERVER_LOG_LEVEL_PROPERTY, NULL);
		return INDIGO_OK;
	} else if (indigo_property_match(SERVER_BLOB_BUFFERING_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- SERVER_BLOB_BUFFERING
		indigo_property_copy_values(SERVER_BLOB_BUFFERING_PROPERTY, property, false);
		indigo_use_blob_buffering = SERVER_BLOB_BUFFERING_ENABLED_ITEM->sw.value || SERVER_BLOB_COMPRESSION_ENABLED_ITEM->sw.value;
		indigo_use_blob_compression = SERVER_BLOB_COMPRESSION_ENABLED_ITEM->sw.value;
		SERVER_BLOB_BUFFERING_PROPERTY->state = INDIGO_OK_STATE;
		indigo_update_property(device, SERVER_BLOB_BUFFERING_PROPERTY, NULL);
		return INDIGO_OK;
	} else if (indigo_property_match(SERVER_BLOB_PROXY_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- SERVER_BLOB_PROXY
		indigo_property_copy_values(SERVER_BLOB_PROXY_PROPERTY, property, false);
		indigo_proxy_blob = SERVER_BLOB_PROXY_ENABLED_ITEM->sw.value;
		SERVER_BLOB_PROXY_PROPERTY->state = INDIGO_OK_STATE;
		indigo_update_property(device, SERVER_BLOB_PROXY_PROPERTY, NULL);
		return INDIGO_OK;
		// --------------------------------------------------------------------------------
	}
	return INDIGO_OK;
}


static indigo_result detach(indigo_device* device) {
	assert(device != NULL);
	indigo_delete_property(device, SERVER_INFO_PROPERTY, NULL);
	indigo_delete_property(device, SERVER_DRIVERS_PROPERTY, NULL);
	if (SERVER_SERVERS_PROPERTY->count > 0) {
		indigo_delete_property(device, SERVER_SERVERS_PROPERTY, NULL);
	}
	indigo_delete_property(device, SERVER_LOAD_PROPERTY, NULL);
	indigo_delete_property(device, SERVER_UNLOAD_PROPERTY, NULL);
	indigo_delete_property(device, SERVER_LOG_LEVEL_PROPERTY, NULL);
	indigo_delete_property(device, SERVER_BLOB_BUFFERING_PROPERTY, NULL);
	indigo_delete_property(device, SERVER_BLOB_PROXY_PROPERTY, NULL);
	indigo_release_property(SERVER_INFO_PROPERTY);
	indigo_release_property(SERVER_DRIVERS_PROPERTY);
	indigo_release_property(SERVER_SERVERS_PROPERTY);
	indigo_release_property(SERVER_LOAD_PROPERTY);
	indigo_release_property(SERVER_UNLOAD_PROPERTY);
	indigo_release_property(SERVER_LOG_LEVEL_PROPERTY);
	indigo_release_property(SERVER_BLOB_BUFFERING_PROPERTY);
	indigo_release_property(SERVER_BLOB_PROXY_PROPERTY);
	INDIGO_LOG(indigo_log("%s detached", device->name));
	return INDIGO_OK;
}

static void add_drivers(const char* folder) {
	//char folder_path[PATH_MAX];
	//if (NULL == realpath(folder, folder_path)) {
	//	INDIGO_DEBUG(indigo_debug("realpath(%s, folder_path): failed", folder));
	//	return;
	//}
	//DIR* dir = opendir(folder_path);
	//if (dir) {
	//	struct dirent* ent;
	//	char* line = NULL;
	//	size_t len = 0;
	//	while ((ent = readdir(dir)) != NULL) {
	//		if (!strncmp(ent->d_name, "indigo_", 7)) {
	//			char path[PATH_MAX];
	//			sprintf(path, "%s/%s", folder_path, ent->d_name);
	//			indigo_log("Loading driver list from %s", path);
	//			FILE* list = fopen(path, "r");
	//			if (list) {
	//				while (getline(&line, &len, list) > 0 && dynamic_drivers_count < INDIGO_MAX_DRIVERS) {
	//					char* pnt, * token = strtok_r(line, ",", &pnt);
	//					if (token && (token = strchr(token, '"'))) {
	//						char* end = strchr(++token, '"');
	//						if (end) {
	//							*end = 0;
	//							for (int i = 0; i < INDIGO_MAX_DRIVERS; i++) {
	//								if (!strcmp(indigo_available_drivers[i].name, token)) {
	//									token = NULL;
	//									break;
	//								}
	//							}
	//							if (token) {
	//								for (int i = 0; i < dynamic_drivers_count; i++) {
	//									if (!strcmp(dynamic_drivers[i].name, token)) {
	//										token = NULL;
	//										break;
	//									}
	//								}
	//							}
	//							if (token) {
	//								dynamic_drivers[dynamic_drivers_count].name = strdup(token);
	//							}
	//							else {
	//								continue;
	//							}
	//						}
	//					}
	//					token = strtok_r(NULL, ",", &pnt);
	//					if (token && (token = strchr(token, '"'))) {
	//						char* end = strchr(token + 1, '"');
	//						if (end) {
	//							*end = 0;
	//							dynamic_drivers[dynamic_drivers_count].description = strdup(token + 1);
	//						}
	//					}
	//					dynamic_drivers_count++;
	//				}
	//				fclose(list);
	//			}
	//		}
	//	}
	//	closedir(dir);
	//	if (line)
	//		free(line);
	//}
}

static void server_main() {
	indigo_start_usb_event_handler();
	indigo_start();
	indigo_log("INDIGO server %d.%d-%s %s/%s built on %s %s", (INDIGO_VERSION_CURRENT >> 8) & 0xFF, INDIGO_VERSION_CURRENT & 0xFF, INDIGO_BUILD, OS_NAME, ARCH_NAME, INDIGO_BUILD_TIME, INDIGO_BUILD_COMMIT);
	indigo_use_blob_caching = true;
	for (int i = 1; i < server_argc; i++) {
		if ((!strcmp(server_argv[i], "-T") || !strcmp(server_argv[i], "--master-token")) && i < server_argc - 1) {
			indigo_set_master_token(indigo_string_to_token(server_argv[i + 1]));
			i++;
		} else if ((!strcmp(server_argv[i], "-a") || !strcmp(server_argv[i], "--acl-file")) && i < server_argc - 1) {
			indigo_load_device_tokens_from_file(server_argv[i + 1]);
			i++;
		}
	}
	for (int i = 1; i < server_argc; i++) {
		if ((!strcmp(server_argv[i], "-p") || !strcmp(server_argv[i], "--port")) && i < server_argc - 1) {
			indigo_server_tcp_port = atoi(server_argv[i + 1]);
			i++;
		} else if ((!strcmp(server_argv[i], "-r") || !strcmp(server_argv[i], "--remote-server")) && i < server_argc - 1) {
			char host[INDIGO_NAME_SIZE];
			indigo_copy_name(host, server_argv[i + 1]);
			char* colon = strchr(host, ':');
			int port = 7624;
			if (colon != NULL) {
				*colon++ = 0;
				port = atoi(colon);
			}
			indigo_reshare_remote_devices = true;
			indigo_connect_server(NULL, host, port, NULL);
			i++;
		} else if ((!strcmp(server_argv[i], "-T") || !strcmp(server_argv[i], "--master-token")) && i < server_argc - 1) {
			i++;
		} else if ((!strcmp(server_argv[i], "-a") || !strcmp(server_argv[i], "--acl-file")) && i < server_argc - 1) {
			i++;
		} else if (!strcmp(server_argv[i], "-b-") || !strcmp(server_argv[i], "--disable-bonjour")) {
			indigo_use_bonjour = false;
		} else if (!strcmp(server_argv[i], "-b") || !strcmp(server_argv[i], "--bonjour")) {
			indigo_copy_name(indigo_local_service_name, server_argv[i + 1]);
			i++;
		} else if (!strcmp(server_argv[i], "-u-") || !strcmp(server_argv[i], "--disable-blob-urls")) {
			indigo_use_blob_urls = false;
		} else if (!strcmp(server_argv[i], "-d-") || !strcmp(server_argv[i], "--disable-blob-buffering")) {
			indigo_use_blob_buffering = false;
			indigo_use_blob_compression = false;
		} else if (!strcmp(server_argv[i], "-C") || !strcmp(server_argv[i], "--enable-blob-compression")) {
			indigo_use_blob_compression = true;
		} else if (!strcmp(server_argv[i], "-x") || !strcmp(server_argv[i], "--enable-blob-proxy")) {
			indigo_proxy_blob = true;
		} else if (server_argv[i][0] != '-') {
			indigo_load_driver(server_argv[i], true, NULL);
			command_line_drivers = true;
		}
	}
	if (!command_line_drivers) {
		for (static_drivers_count = 0; static_drivers[static_drivers_count]; static_drivers_count++) {
			indigo_add_driver(static_drivers[static_drivers_count], false, NULL);
		}
		char* last = strrchr(server_argv[0], '/');
		if (last) {
			char path[PATH_MAX];
			long len = (long)(last - server_argv[0]);
			strncpy(path, server_argv[0], len);
			path[len] = 0;
			strcat(path, "/../share/indigo");
			add_drivers(path);
		}
		add_drivers("/usr/share/indigo");
		add_drivers("/usr/local/share/indigo");
	}
	indigo_attach_device(&server_device);
	indigo_server_start(NULL);
	for (int i = 0; i < INDIGO_MAX_DRIVERS; i++) {
		if (indigo_available_drivers[i].driver) {
			indigo_remove_driver(&indigo_available_drivers[i]);
		}
	}
	indigo_detach_device(&server_device);
	indigo_stop();
	indigo_server_remove_resources();
	for (int i = 0; i < INDIGO_MAX_SERVERS; i++) {
		if (indigo_available_servers[i].thread_started) {
			indigo_disconnect_server(&indigo_available_servers[i]);
		}
	}
	exit(EXIT_SUCCESS);
}

int main(int argc, const char* argv[]) {
	bool do_fork = true;
	server_argv[0] = argv[0];
	indigo_main_argc = argc;
	indigo_main_argv = argv;
	for (int i = 1; i < argc; i++) {
		if (!strcmp(argv[i], "-h") || !strcmp(argv[i], "--help")) {
			printf("INDIGO server v.%d.%d-%s %s/%s built on %s %s.\n", (INDIGO_VERSION_CURRENT >> 8) & 0xFF, INDIGO_VERSION_CURRENT & 0xFF, INDIGO_BUILD, OS_NAME, ARCH_NAME, __DATE__, __TIME__);
			printf("usage: %s [-h | --help]\n", argv[0]);
			printf("       %s [options] indigo_driver_name indigo_driver_name ...\n", argv[0]);
			printf("options:\n"
				"       -p  | --port port                     (default: 7624)\n"
				"       -b  | --bonjour name                  (default: hostname)\n"
				"       -T  | --master-token token            (master token for devce access default: 0 = none)\n"
				"       -a  | --acl-file file\n"
				"       -b- | --disable-bonjour\n"
				"       -u- | --disable-blob-urls\n"
				"       -d- | --disable-blob-buffering\n"
				"       -C  | --enable-blob-compression\n"
				"       -w- | --disable-web-apps\n"
				"       -c- | --disable-control-panel\n"
				"       -v  | --enable-info\n"
				"       -vv | --enable-debug\n"
				"       -vvb| --enable-trace-bus\n"
				"       -vvv| --enable-trace\n"
				"       -r  | --remote-server host[:port]     (default port: 7624)\n"
				"       -x  | --enable-blob-proxy\n"
			);
			return 0;
		} else {
			server_argv[server_argc++] = argv[i];
		}
	}
	server_main();
}
