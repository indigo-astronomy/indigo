// Copyright (c) 2026 CloudMakers, s. r. o.
// All rights reserved.
//
// You can use this software under the terms of 'INDIGO Astronomy
// open-source license' (see LICENSE.md).

#include <pthread.h>
#include <stdatomic.h>
#include <stdlib.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <indigo/indigo_filter.h>
#include <indigo/indigo_server_tcp.h>
#include <indigo_drivers/agent_config/indigo_agent_config.h>

#include "../test_runner.h"

#define AGENT CONFIG_AGENT_NAME
#define ARRAY_SIZE(a) (sizeof(a) / sizeof((a)[0]))
#define REQUIRE(c) do { if (!(c)) { fprintf(stderr, "%s:%d: %s\n", __FILE__, __LINE__, #c); indigo_test_failures++; return false; } } while (0)

typedef struct {
	indigo_property *property;
	unsigned revision;
} observation;
static observation cache[256];
static pthread_mutex_t cache_mutex = PTHREAD_MUTEX_INITIALIZER;
static indigo_client client;
static char test_root[128], config_folder[256];
static bool bus_started, client_attached, agent_started;
static atomic_bool hold_load, load_waiting;
static bool fail_save, fail_write, fail_replace;

bool config_test_replace(const char *source, const char *destination) {
	return !fail_replace && indigo_uni_replace_file(source, destination);
}
static atomic_bool client_detached;
static atomic_int late_commands;
static void (*sleep_hook)(void);

indigo_result config_test_detach(indigo_client *client) {
	indigo_result result = indigo_detach_client(client);
	client_detached = true;
	return result;
}

indigo_result config_test_save(indigo_device *device, indigo_uni_handle **handle, indigo_property *property) {
	if (fail_save && handle) {
		return INDIGO_FAILED;
	}
	if (fail_write && handle && *handle) {
		close((*handle)->fd);
		(*handle)->fd = -1;
	}
	return indigo_save_property(device, handle, property);
}

const char *config_test_folder(void) {
	return config_folder;
}

char *config_test_getenv(const char *name) {
	return !strcmp(name, "HOME") ? test_root : getenv(name);
}

// Only agent waits are shortened; bus scheduling and timeout predicates are real.
void config_test_sleep(long delay) {
	if (sleep_hook) {
		sleep_hook();
	}
	load_waiting = true;
	while (hold_load) {
		indigo_usleep(1000);
	}
	indigo_usleep(getenv("INDIGO_CONFIG_REALTIME") ? delay : 1000);
}

static observation *find(const char *device, const char *name) {
	for (int i = 0; i < ARRAY_SIZE(cache); i++) {
		if (cache[i].property && !strcmp(cache[i].property->device, device) && !strcmp(cache[i].property->name, name)) {
			return cache + i;
		}
	}
	return NULL;
}

static indigo_result observe(indigo_property *property) {
	pthread_mutex_lock(&cache_mutex);
	observation *entry = find(property->device, property->name);
	if (!entry) {
		for (int i = 0; i < ARRAY_SIZE(cache); i++) {
			if (!cache[i].property) {
				entry = cache + i;
				break;
			}
		}
	}
	if (entry) {
		indigo_release_property(entry->property);
		entry->property = indigo_copy_property(NULL, property);
		entry->revision++;
	}
	pthread_mutex_unlock(&cache_mutex);
	return INDIGO_OK;
}

static indigo_result defined(indigo_client *client, indigo_device *device, indigo_property *property, const char *message) {
	return observe(property);
}

static indigo_result updated(indigo_client *client, indigo_device *device, indigo_property *property, const char *message) {
	return observe(property);
}

static indigo_result deleted(indigo_client *client, indigo_device *device, indigo_property *property, const char *message) {
	pthread_mutex_lock(&cache_mutex);
	for (int i = 0; i < ARRAY_SIZE(cache); i++) {
		if (cache[i].property && !strcmp(cache[i].property->device, property->device) && (!*property->name || !strcmp(cache[i].property->name, property->name))) {
			indigo_release_property(cache[i].property);
			cache[i].property = NULL;
		}
	}
	pthread_mutex_unlock(&cache_mutex);
	return INDIGO_OK;
}

static indigo_client client = { .name = "Config integration client", .define_property = defined, .update_property = updated, .delete_property = deleted };

static indigo_property *snapshot(const char *name) {
	pthread_mutex_lock(&cache_mutex);
	observation *entry = find(AGENT, name);
	indigo_property *result = entry ? indigo_copy_property(NULL, entry->property) : NULL;
	pthread_mutex_unlock(&cache_mutex);
	return result;
}

static int state(const char *name) {
	indigo_property *p = snapshot(name);
	int result = p ? p->state : -1;
	indigo_release_property(p);
	return result;
}

static bool wait_state(const char *name, int expected) {
	double deadline = indigo_monotonic_time() + 15;
	while (indigo_monotonic_time() < deadline) {
		if (expected < 0 ? state(name) != INDIGO_BUSY_STATE : state(name) == expected) {
			return true;
		}
		indigo_usleep(1000);
	}
	fprintf(stderr, "%s: expected state %d, got %d\n", name, expected, state(name));
	return false;
}

static bool has_item(const char *name, const char *item, const char *text, int value) {
	indigo_property *p = snapshot(name);
	bool result = false;
	if (p) {
		for (int i = 0; i < p->count; i++) {
			if (!strcmp(p->items[i].name, item)) {
				result = p->type == INDIGO_TEXT_VECTOR ? (!text || !strcmp(p->items[i].text.value, text)) : (value < 0 || p->items[i].sw.value == value);
			}
		}
	}
	indigo_release_property(p);
	return result;
}

#define SAVE AGENT_CONFIG_SAVE_PROPERTY_NAME
#define LOAD AGENT_CONFIG_LOAD_PROPERTY_NAME
#define REMOVE AGENT_CONFIG_DELETE_PROPERTY_NAME
#define LAST AGENT_CONFIG_LAST_CONFIG_PROPERTY_NAME
#define SETUP AGENT_CONFIG_SETUP_PROPERTY_NAME
#define DRIVERS AGENT_CONFIG_DRIVERS_PROPERTY_NAME
#define PROFILES AGENT_CONFIG_PROFILES_PROPERTY_NAME
#define FILTER "FILTER_CCD_LIST"
#define RELATED FILTER_RELATED_AGENT_LIST_PROPERTY_NAME
#define MIRROR "AGENT_CONFIG Test Agent"

static bool text_change(const char *property, const char *value, int expected) {
	REQUIRE(indigo_change_text_property_1(&client, AGENT, property, "NAME", value) == INDIGO_OK);
	REQUIRE(state(property) == expected);
	return true;
}

static void option(const char *item, bool value) {
	ASSERT_EQ_INT(INDIGO_OK, indigo_change_switch_property_1(&client, AGENT, SETUP, item, value));
	ASSERT_EQ_INT(INDIGO_OK_STATE, state(SETUP));
}

static bool load(const char *name, int expected) {
	REQUIRE(indigo_change_switch_property_1(&client, AGENT, LOAD, name, true) == INDIGO_OK);
	REQUIRE(wait_state(LOAD, -1));
	if (state(LOAD) != expected) {
		fprintf(stderr, "Load %s: expected %d, observed %d\n", name, expected, state(LOAD));
	}
	REQUIRE(state(LOAD) == expected);
	REQUIRE(has_item(LOAD, name, NULL, 0));
	return true;
}

// Synchronous local peers implement only properties consumed by Configuration Agent.
static indigo_device peer;
static indigo_property *props[96];
static int prop_count;
static pthread_mutex_t peer_mutex = PTHREAD_MUTEX_INITIALIZER;
static int save_requests, profile_requests, driver_requests, selection_requests, related_requests;
static bool reject_profile, reject_driver, reject_select, reject_related, busy_select, refuse_deselect, reenter_save;
static indigo_property *drivers, *profile, *selection, *related;

static indigo_result peer_enumerate(indigo_device *device, indigo_client *c, indigo_property *match) {
	for (int i = 0; i < prop_count; i++) {
		if (indigo_property_match(props[i], match)) {
			indigo_define_property(device, props[i], NULL);
		}
	}
	return INDIGO_OK;
}

static indigo_result peer_change(indigo_device *device, indigo_client *c, indigo_property *request) {
	if (!strcmp(request->device, "Test Camera") && !strcmp(request->name, "CONFIG")) {
		save_requests++;
		if (reenter_save) {
			profile->items[0].sw.value = true;
			profile->items[1].sw.value = false;
			indigo_update_property(device, profile, NULL);
		}
		return INDIGO_OK;
	}
	pthread_mutex_lock(&peer_mutex);
	for (int i = 0; i < prop_count; i++) {
		indigo_property *p = props[i];
		if (indigo_property_match(p, request)) {
			if (client_detached) { late_commands++; }
			bool reject = (p == profile && reject_profile) || (p == drivers && reject_driver) || (p == selection && reject_select) || (p == related && reject_related);
			if (p == profile) { profile_requests++; }
			if (p == drivers) { driver_requests++; }
			if (p == selection) { selection_requests++; }
			if (p == related) { related_requests++; }
			bool deselect = p == selection && request->count && !strcmp(request->items[0].name, "NONE");
			if (!reject && !(deselect && refuse_deselect)) {
				indigo_property_copy_values(p, request, false);
			}
			p->state = reject ? INDIGO_ALERT_STATE : (p == selection && busy_select && !deselect ? INDIGO_BUSY_STATE : INDIGO_OK_STATE);
			indigo_update_property(device, p, NULL);
			break;
		}
	}
	pthread_mutex_unlock(&peer_mutex);
	return INDIGO_OK;
}

static indigo_device peer = INDIGO_DEVICE_INITIALIZER("@Config test peers", NULL, peer_enumerate, peer_change, NULL, NULL);

static indigo_property *publish(const char *device, const char *name, const char **items, int count, int selected, bool many) {
	indigo_property *p = indigo_init_switch_property(NULL, device, name, "Test", name, INDIGO_OK_STATE, INDIGO_RW_PERM, many ? INDIGO_ANY_OF_MANY_RULE : INDIGO_ONE_OF_MANY_RULE, count);
	for (int i = 0; i < count; i++) {
		indigo_init_switch_item(p->items + i, items[i], items[i], selected == i);
	}
	props[prop_count++] = p;
	indigo_define_property(&peer, p, NULL);
	return p;
}

static void populate(void) {
	const char *driver_items[] = { "indigo_agent_config", "indigo_test_camera", "indigo_extra" };
	const char *profile_items[] = { "Default", "Night" };
	const char *filter_items[] = { "NONE", "Test Camera" };
	const char *related_items[] = { "Focus Agent", "Guide Agent" };
	drivers = publish("Test Server", SERVER_DRIVERS_PROPERTY_NAME, driver_items, 3, 0, true);
	profile = publish("Test Camera", "PROFILE", profile_items, 2, 1, false);
	selection = publish("Test Agent", FILTER, filter_items, 2, 1, false);
	related = publish("Test Agent", RELATED, related_items, 2, 0, true);
	related->items[1].sw.value = true;
	indigo_update_property(&peer, related, NULL);
}

static void choose(indigo_property *p, int selected) {
	pthread_mutex_lock(&peer_mutex);
	for (int i = 0; i < p->count; i++) {
		p->items[i].sw.value = i == selected;
	}
	indigo_update_property(&peer, p, NULL);
	pthread_mutex_unlock(&peer_mutex);
}

static bool fixture(const char *name, const char *content) {
	char path[512];
	snprintf(path, sizeof(path), "%s/%s", config_folder, name);
	FILE *f = fopen(path, "w");
	REQUIRE(f != NULL);
	bool ok = fputs(content, f) >= 0;
	REQUIRE(fclose(f) == 0 && ok);
	return true;
}

static bool exists(const char *name) {
	char path[512];
	snprintf(path, sizeof(path), "%s/%s", config_folder, name);
	return access(path, F_OK) == 0;
}

static bool file_contains(const char *name, const char *needle) {
	char path[512], buffer[32768];
	snprintf(path, sizeof(path), "%s/%s", config_folder, name);
	FILE *f = fopen(path, "r");
	if (!f) { return false; }
	size_t n = fread(buffer, 1, sizeof(buffer) - 1, f);
	buffer[n] = 0;
	fclose(f);
	return strstr(buffer, needle) != NULL;
}

static void refresh(void) {
	ASSERT_TRUE(text_change(SAVE, "", INDIGO_ALERT_STATE));
}

static void lifecycle(void) {
	indigo_driver_info info;
	ASSERT_EQ_INT(INDIGO_OK, indigo_agent_config(INDIGO_DRIVER_INFO, &info));
	ASSERT_STREQ("indigo_agent_config", info.name);
	ASSERT_FALSE(info.multi_device_support);
	ASSERT_EQ_INT(INDIGO_OK, indigo_agent_config(INDIGO_DRIVER_INIT, NULL));
	client_detached = false;
	const char *names[] = { SETUP, SAVE, REMOVE, LOAD, LAST, DRIVERS, PROFILES, "INFO" };
	for (int i = 0; i < ARRAY_SIZE(names); i++) {
		indigo_property *p = snapshot(names[i]);
		ASSERT_TRUE(p != NULL);
		indigo_release_property(p);
	}
	ASSERT_TRUE(has_item(SETUP, "AUTOSAVE_DEVICE_CONFIGS", NULL, 0));
	ASSERT_TRUE(has_item(SETUP, "UNLOAD_UNUSED_DRIVERS", NULL, 0));
	ASSERT_TRUE(has_item(LAST, "NAME", "", -1));
	for (int i = 0; i < 3; i++) {
		ASSERT_EQ_INT(INDIGO_OK, indigo_agent_config(INDIGO_DRIVER_SHUTDOWN, NULL));
		ASSERT_EQ_INT(-1, state(SETUP));
		ASSERT_EQ_INT(INDIGO_OK, indigo_agent_config(INDIGO_DRIVER_SHUTDOWN, NULL));
		ASSERT_EQ_INT(INDIGO_OK, indigo_agent_config(INDIGO_DRIVER_INIT, NULL));
		client_detached = false;
	}
}

static void schema(void) {
	const char *names[] = { SETUP, SAVE, REMOVE, LOAD, LAST, DRIVERS, PROFILES };
	const int types[] = { INDIGO_SWITCH_VECTOR, INDIGO_TEXT_VECTOR, INDIGO_TEXT_VECTOR, INDIGO_SWITCH_VECTOR, INDIGO_TEXT_VECTOR, INDIGO_SWITCH_VECTOR, INDIGO_TEXT_VECTOR };
	for (int i = 0; i < ARRAY_SIZE(names); i++) {
		indigo_property *p = snapshot(names[i]);
		ASSERT_TRUE(p != NULL);
		bool ok = p->type == types[i] && p->perm == (i >= 4 ? INDIGO_RO_PERM : INDIGO_RW_PERM);
		indigo_release_property(p);
		ASSERT_TRUE(ok);
	}
}

static void discovery(void) {
	populate();
	ASSERT_TRUE(has_item(DRIVERS, "indigo_agent_config", NULL, 1));
	ASSERT_TRUE(has_item(PROFILES, "Test Camera", "Night", -1));
	ASSERT_TRUE(has_item(MIRROR, FILTER, "Test Camera", -1));
	ASSERT_TRUE(has_item(MIRROR, RELATED, "Focus Agent;Guide Agent", -1));
	choose(profile, 0);
	choose(selection, 0);
	ASSERT_TRUE(has_item(PROFILES, "Test Camera", "Default", -1));
	ASSERT_TRUE(has_item(MIRROR, FILTER, "", -1));
	selection->state = INDIGO_ALERT_STATE;
	indigo_update_property(&peer, selection, NULL);
	ASSERT_EQ_INT(INDIGO_ALERT_STATE, state(MIRROR));
	ASSERT_EQ_INT(INDIGO_IDLE_STATE, state(LAST));
}

static void remote_ignored(void) {
	const char *items[] = { "remote" };
	const char *names[] = { SERVER_DRIVERS_PROPERTY_NAME, "PROFILE", FILTER, RELATED };
	for (int i = 0; i < ARRAY_SIZE(names); i++) {
		indigo_property *p = publish("Remote @ service", names[i], items, 1, 0, false);
		p->state = INDIGO_ALERT_STATE;
		indigo_update_property(&peer, p, NULL);
		indigo_delete_property(&peer, p, NULL);
	}
	ASSERT_FALSE(has_item(DRIVERS, "remote", NULL, -1));
	ASSERT_FALSE(has_item(PROFILES, "Remote @ service", NULL, -1));
	ASSERT_EQ_INT(-1, state("AGENT_CONFIG Remote @ service"));
}

static void deletion(void) {
	populate();
	indigo_delete_property(&peer, selection, NULL);
	ASSERT_FALSE(has_item(MIRROR, FILTER, NULL, -1));
	ASSERT_TRUE(has_item(MIRROR, RELATED, NULL, -1));
	indigo_delete_property(&peer, profile, NULL);
	ASSERT_FALSE(has_item(PROFILES, "Test Camera", NULL, -1));
	indigo_delete_property(&peer, drivers, NULL);
	ASSERT_FALSE(has_item(DRIVERS, "indigo_agent_config", NULL, -1));
	indigo_property *all = indigo_init_switch_property(NULL, "Test Agent", "", "", "", INDIGO_OK_STATE, INDIGO_RO_PERM, INDIGO_ANY_OF_MANY_RULE, 0);
	indigo_delete_property(&peer, all, NULL);
	indigo_release_property(all);
	ASSERT_EQ_INT(-1, state(MIRROR));
	indigo_define_property(&peer, selection, NULL);
	ASSERT_TRUE(has_item(MIRROR, FILTER, "Test Camera", -1));
}

static void save_roundtrip(void) {
	populate();
	ASSERT_TRUE(text_change(SAVE, "Night sky\tA", INDIGO_OK_STATE));
	ASSERT_TRUE(exists("Night_sky_A.saved"));
	ASSERT_TRUE(has_item(LOAD, "Night_sky_A", NULL, 0));
	ASSERT_TRUE(has_item(LAST, "NAME", "Night_sky_A", -1));
	ASSERT_TRUE(file_contains("Night_sky_A.saved", DRIVERS));
	ASSERT_TRUE(file_contains("Night_sky_A.saved", PROFILES));
	ASSERT_TRUE(file_contains("Night_sky_A.saved", "Test Camera"));
	ASSERT_EQ_INT(0, save_requests);
	choose(profile, 0);
	choose(selection, 0);
	choose(related, -1);
	ASSERT_TRUE(load("Night_sky_A", INDIGO_OK_STATE));
	ASSERT_TRUE(has_item(PROFILES, "Test Camera", "Night", -1));
	ASSERT_TRUE(has_item(MIRROR, FILTER, "Test Camera", -1));
	ASSERT_TRUE(has_item(MIRROR, RELATED, "Focus Agent;Guide Agent", -1));
	ASSERT_EQ_INT(INDIGO_OK_STATE, state(LAST));
	ASSERT_TRUE(profile_requests > 0 && driver_requests > 0 && selection_requests > 0 && related_requests > 0);
}

static void overwrite(void) {
	populate();
	ASSERT_TRUE(text_change(SAVE, "same", INDIGO_OK_STATE));
	choose(profile, 0);
	ASSERT_TRUE(text_change(SAVE, "same", INDIGO_OK_STATE));
	ASSERT_TRUE(file_contains("same.saved", "Default"));
	ASSERT_FALSE(file_contains("same.saved", "Night"));
	indigo_property *p = snapshot(LOAD);
	ASSERT_TRUE(p != NULL);
	int count = p->count;
	indigo_release_property(p);
	ASSERT_EQ_INT(1, count);
}

static void save_empty_and_failure(void) {
	ASSERT_TRUE(text_change(SAVE, "good", INDIGO_OK_STATE));
	ASSERT_TRUE(text_change(SAVE, "", INDIGO_ALERT_STATE));
	ASSERT_TRUE(has_item(LAST, "NAME", "good", -1));
	char original[256];
	strcpy(original, config_folder);
	strcpy(config_folder, "/dev/null/config-test");
	bool failed = text_change(SAVE, "bad", INDIGO_ALERT_STATE);
	strcpy(config_folder, original);
	ASSERT_TRUE(failed);
	ASSERT_TRUE(has_item(LAST, "NAME", "good", -1));
	ASSERT_TRUE(text_change(SAVE, "recovered", INDIGO_OK_STATE));
}

static void remove_roundtrip(void) {
	ASSERT_TRUE(text_change(SAVE, "one", INDIGO_OK_STATE));
	ASSERT_TRUE(text_change(SAVE, "two words", INDIGO_OK_STATE));
	ASSERT_TRUE(text_change(REMOVE, "one", INDIGO_OK_STATE));
	ASSERT_TRUE(has_item(LAST, "NAME", "two_words", -1));
	ASSERT_TRUE(text_change(REMOVE, "two words", INDIGO_OK_STATE));
	ASSERT_FALSE(exists("two_words.saved"));
	ASSERT_FALSE(has_item(LOAD, "two_words", NULL, -1));
	ASSERT_TRUE(has_item(LAST, "NAME", "", -1));
	ASSERT_TRUE(text_change(REMOVE, "missing", INDIGO_ALERT_STATE));
	ASSERT_TRUE(text_change(REMOVE, "../outside", INDIGO_ALERT_STATE));
	ASSERT_TRUE(text_change(REMOVE, "", INDIGO_ALERT_STATE));
}

static void setup_persistence(void) {
	option("AUTOSAVE_DEVICE_CONFIGS", true);
	option("UNLOAD_UNUSED_DRIVERS", true);
	ASSERT_TRUE(exists("Configuration_Agent.config"));
	ASSERT_TRUE(file_contains("Configuration_Agent.config", "AUTOSAVE_DEVICE_CONFIGS"));
	option("AUTOSAVE_DEVICE_CONFIGS", false);
	ASSERT_EQ_INT(INDIGO_OK, indigo_change_switch_property_1(&client, AGENT, "CONFIG", "LOAD", true));
	ASSERT_TRUE(has_item(SETUP, "AUTOSAVE_DEVICE_CONFIGS", NULL, 0));
	ASSERT_TRUE(has_item(SETUP, "UNLOAD_UNUSED_DRIVERS", NULL, 1));
}

static void autosave(void) {
	populate();
	option("AUTOSAVE_DEVICE_CONFIGS", true);
	ASSERT_TRUE(text_change(SAVE, "auto", INDIGO_OK_STATE));
	ASSERT_EQ_INT(1, save_requests);
	ASSERT_TRUE(text_change(SAVE, "", INDIGO_ALERT_STATE));
	ASSERT_EQ_INT(1, save_requests);
	choose(selection, 0);
	ASSERT_TRUE(text_change(SAVE, "auto_empty", INDIGO_OK_STATE));
	ASSERT_EQ_INT(1, save_requests);
}

static void driver_unload_policy(void) {
	populate();
	ASSERT_TRUE(text_change(SAVE, "policy", INDIGO_OK_STATE));
	choose(drivers, 2);
	ASSERT_TRUE(load("policy", INDIGO_OK_STATE));
	ASSERT_TRUE(has_item(DRIVERS, "indigo_extra", NULL, 1));
	option("UNLOAD_UNUSED_DRIVERS", true);
	ASSERT_TRUE(load("policy", INDIGO_OK_STATE));
	ASSERT_TRUE(has_item(DRIVERS, "indigo_extra", NULL, 0));
	ASSERT_TRUE(has_item(DRIVERS, "indigo_agent_config", NULL, 1));
}

static void repeated_load(void) {
	populate();
	ASSERT_TRUE(text_change(SAVE, "repeat", INDIGO_OK_STATE));
	for (int i = 0; i < 8; i++) {
		choose(profile, 0);
		ASSERT_TRUE(load("repeat", INDIGO_OK_STATE));
		ASSERT_TRUE(has_item(PROFILES, "Test Camera", "Night", -1));
	}
}

static void missing_profile(void) {
	ASSERT_TRUE(fixture("missing.saved", "<newTextVector device='Configuration Agent' name='AGENT_CONFIG_PROFILES'><oneText name='Absent Camera'>Night</oneText></newTextVector>"));
	refresh();
	ASSERT_TRUE(load("missing", INDIGO_ALERT_STATE));
	ASSERT_EQ_INT(INDIGO_ALERT_STATE, state(LAST));
	ASSERT_TRUE(text_change(SAVE, "recovery", INDIGO_OK_STATE));
	ASSERT_TRUE(load("recovery", INDIGO_OK_STATE));
}

static void selection_failure(void) {
	populate();
	indigo_delete_property(&peer, related, NULL);
	ASSERT_TRUE(text_change(SAVE, "fail", INDIGO_OK_STATE));
	choose(selection, 0);
	reject_select = true;
	ASSERT_TRUE(load("fail", INDIGO_ALERT_STATE));
	reject_select = false;
	ASSERT_TRUE(load("fail", INDIGO_OK_STATE));
}

static void busy_guard(void) {
	ASSERT_TRUE(text_change(SAVE, "first", INDIGO_OK_STATE));
	ASSERT_TRUE(text_change(SAVE, "second", INDIGO_OK_STATE));
	hold_load = true;
	ASSERT_EQ_INT(INDIGO_OK, indigo_change_switch_property_1(&client, AGENT, LOAD, "first", true));
	ASSERT_EQ_INT(INDIGO_BUSY_STATE, state(LOAD));
	ASSERT_EQ_INT(INDIGO_OK, indigo_change_switch_property_1(&client, AGENT, LOAD, "second", true));
	ASSERT_TRUE(has_item(LOAD, "first", NULL, 1));
	ASSERT_TRUE(has_item(LOAD, "second", NULL, 0));
	hold_load = false;
	ASSERT_TRUE(wait_state(LOAD, INDIGO_OK_STATE));
	ASSERT_TRUE(has_item(LAST, "NAME", "first", -1));
}

static void missing_file(void) {
	ASSERT_TRUE(text_change(SAVE, "gone", INDIGO_OK_STATE));
	char path[512];
	snprintf(path, sizeof(path), "%s/gone.saved", config_folder);
	ASSERT_EQ_INT(0, unlink(path));
	ASSERT_TRUE(load("gone", INDIGO_ALERT_STATE));
}

static void malformed_file(void) {
	ASSERT_TRUE(fixture("broken.saved", "this is not an INDIGO configuration"));
	refresh();
	ASSERT_TRUE(load("broken", INDIGO_ALERT_STATE));
}

static void scan_suffix(void) {
	ASSERT_TRUE(fixture("real.saved", ""));
	ASSERT_TRUE(fixture("backup.saved.bak", ""));
	ASSERT_TRUE(fixture("embedded.saved.name.saved", ""));
	ASSERT_TRUE(fixture("unrelated.config", ""));
	refresh();
	ASSERT_TRUE(has_item(LOAD, "real", NULL, 0));
	ASSERT_TRUE(has_item(LOAD, "embedded.saved.name", NULL, 0));
	ASSERT_FALSE(has_item(LOAD, "backup", NULL, -1));
	ASSERT_FALSE(has_item(LOAD, "unrelated.config", NULL, -1));
}

static void alternate_folder_remove(void) {
	snprintf(config_folder, sizeof(config_folder), "%s/alternate", test_root);
	ASSERT_TRUE(text_change(SAVE, "alternate", INDIGO_OK_STATE));
	ASSERT_TRUE(text_change(REMOVE, "alternate", INDIGO_OK_STATE));
	ASSERT_FALSE(exists("alternate.saved"));
}

static void port_namespace(void) {
	indigo_server_tcp_port = 7625;
	ASSERT_TRUE(text_change(SAVE, "port", INDIGO_OK_STATE));
	ASSERT_TRUE(has_item(LOAD, "port", NULL, 0));
	ASSERT_TRUE(load("port", INDIGO_OK_STATE));
	ASSERT_TRUE(text_change(REMOVE, "port", INDIGO_OK_STATE));
}

static void deselection_timeout(void) {
	populate();
	ASSERT_TRUE(text_change(SAVE, "stuck", INDIGO_OK_STATE));
	refuse_deselect = true;
	ASSERT_TRUE(load("stuck", INDIGO_ALERT_STATE));
	ASSERT_EQ_INT(INDIGO_ALERT_STATE, state(LAST));
}

static void restore_busy_timeout(void) {
	populate();
	indigo_delete_property(&peer, related, NULL);
	ASSERT_TRUE(text_change(SAVE, "busy", INDIGO_OK_STATE));
	busy_select = true;
	ASSERT_TRUE(load("busy", INDIGO_ALERT_STATE));
}

static void profile_rejection(void) {
	populate();
	ASSERT_TRUE(text_change(SAVE, "reject", INDIGO_OK_STATE));
	choose(profile, 0);
	reject_profile = true;
	ASSERT_TRUE(load("reject", INDIGO_ALERT_STATE));
	choose(profile, 1);
	ASSERT_TRUE(load("reject", INDIGO_ALERT_STATE));
	reject_profile = false;
	ASSERT_TRUE(load("reject", INDIGO_OK_STATE));
}

static void driver_rejection(void) {
	populate();
	ASSERT_TRUE(text_change(SAVE, "reject", INDIGO_OK_STATE));
	reject_driver = true;
	ASSERT_TRUE(load("reject", INDIGO_ALERT_STATE));
	reject_driver = false;
	ASSERT_TRUE(load("reject", INDIGO_OK_STATE));
}

static void absent_agent(void) {
	ASSERT_TRUE(fixture("absent.saved", "<newTextVector device='Configuration Agent' name='AGENT_CONFIG Absent Agent'><oneText name='FILTER_CCD_LIST'>Test Camera</oneText></newTextVector>"));
	refresh();
	ASSERT_TRUE(load("absent", INDIGO_ALERT_STATE));
}

static void server_disappearance(void) {
	populate();
	indigo_property *all = indigo_init_switch_property(NULL, "Test Server", "", "", "", INDIGO_OK_STATE, INDIGO_RO_PERM, INDIGO_ANY_OF_MANY_RULE, 0);
	indigo_delete_property(&peer, all, NULL);
	indigo_release_property(all);
	ASSERT_FALSE(has_item(DRIVERS, "indigo_agent_config", NULL, -1));
}

static void profile_no_selection(void) {
	populate();
	choose(profile, -1);
	ASSERT_TRUE(has_item(PROFILES, "Test Camera", "", -1));
}

static void capacity_restore(void) {
	const char *items[] = { "NONE", "Test Camera" };
	for (int i = 0; i < 16; i++) {
		char name[64];
		snprintf(name, sizeof(name), "Agent %02d", i);
		publish(name, FILTER, items, 2, 1, false);
	}
	ASSERT_TRUE(text_change(SAVE, "capacity", INDIGO_OK_STATE));
	ASSERT_TRUE(load("capacity", INDIGO_OK_STATE));
	for (int i = 0; i < 16; i++) {
		char name[64];
		snprintf(name, sizeof(name), "AGENT_CONFIG Agent %02d", i);
		ASSERT_TRUE(has_item(name, FILTER, "Test Camera", -1));
	}
}

static void restore_queue_overflow(void) {
	char content[8192] = "";
	for (int i = 0; i < 19; i++) {
		strcat(content, "<newTextVector device='Configuration Agent' name='AGENT_CONFIG_PROFILES'></newTextVector>");
	}
	ASSERT_TRUE(fixture("overflow.saved", content));
	refresh();
	ASSERT_TRUE(load("overflow", INDIGO_ALERT_STATE));
	ASSERT_TRUE(text_change(SAVE, "recovered", INDIGO_OK_STATE));
	ASSERT_TRUE(load("recovered", INDIGO_OK_STATE));
}

static void autosave_reentrant(void) {
	populate();
	option("AUTOSAVE_DEVICE_CONFIGS", true);
	reenter_save = true;
	alarm(5);
	ASSERT_TRUE(text_change(SAVE, "deadlock", INDIGO_OK_STATE));
}

static void concurrent_save(void) {
	ASSERT_TRUE(text_change(SAVE, "first", INDIGO_OK_STATE));
	hold_load = true;
	ASSERT_EQ_INT(INDIGO_OK, indigo_change_switch_property_1(&client, AGENT, LOAD, "first", true));
	double deadline = indigo_monotonic_time() + 2;
	while (!load_waiting && indigo_monotonic_time() < deadline) {
		indigo_usleep(1000);
	}
	ASSERT_TRUE(load_waiting);
	ASSERT_TRUE(text_change(SAVE, "second", INDIGO_ALERT_STATE));
	ASSERT_FALSE(exists("second.saved"));
	hold_load = false;
	ASSERT_TRUE(wait_state(LOAD, INDIGO_OK_STATE));
	ASSERT_TRUE(has_item(LAST, "NAME", "first", -1));
}

static void startup_scan(void) {
	ASSERT_TRUE(text_change(SAVE, "persisted", INDIGO_OK_STATE));
	ASSERT_EQ_INT(INDIGO_OK, indigo_agent_config(INDIGO_DRIVER_SHUTDOWN, NULL));
	ASSERT_EQ_INT(INDIGO_OK, indigo_agent_config(INDIGO_DRIVER_INIT, NULL));
	client_detached = false;
	ASSERT_TRUE(has_item(LOAD, "persisted", NULL, 0));
	ASSERT_TRUE(load("persisted", INDIGO_OK_STATE));
}

static void enumeration_filter(void) {
	indigo_property *p = indigo_init_switch_property(NULL, AGENT, SETUP, "", "", INDIGO_OK_STATE, INDIGO_RW_PERM, INDIGO_ANY_OF_MANY_RULE, 0);
	ASSERT_EQ_INT(INDIGO_OK, indigo_enumerate_properties(&client, p));
	indigo_release_property(p);
	ASSERT_TRUE(has_item(SETUP, "AUTOSAVE_DEVICE_CONFIGS", NULL, 0));
}

static void save_write_failure(void) {
	populate();
	ASSERT_TRUE(text_change(SAVE, "write_error", INDIGO_OK_STATE));
	choose(profile, 0);
	fail_save = true;
	ASSERT_TRUE(text_change(SAVE, "write_error", INDIGO_ALERT_STATE));
	ASSERT_TRUE(file_contains("write_error.saved", "Night"));
	ASSERT_FALSE(exists("write_error.saved.tmp"));
	fail_save = false;
	fail_write = true;
	ASSERT_TRUE(text_change(SAVE, "write_error", INDIGO_ALERT_STATE));
	ASSERT_TRUE(file_contains("write_error.saved", "Night"));
	ASSERT_FALSE(exists("write_error.saved.tmp"));
	fail_write = false;
	fail_replace = true;
	ASSERT_TRUE(text_change(SAVE, "write_error", INDIGO_ALERT_STATE));
	ASSERT_TRUE(file_contains("write_error.saved", "Night"));
	ASSERT_FALSE(exists("write_error.saved.tmp"));
	fail_replace = false;
	ASSERT_TRUE(text_change(SAVE, "write_error", INDIGO_OK_STATE));
	ASSERT_TRUE(file_contains("write_error.saved", "Default"));
	ASSERT_TRUE(load("write_error", INDIGO_OK_STATE));
}

static void selection_alert_masked(void) {
	populate();
	ASSERT_TRUE(text_change(SAVE, "masked", INDIGO_OK_STATE));
	choose(selection, 0);
	reject_select = true;
	ASSERT_TRUE(load("masked", INDIGO_ALERT_STATE));
}

static void empty_load(void) {
	populate();
	ASSERT_TRUE(text_change(SAVE, "keep", INDIGO_OK_STATE));
	int before = selection_requests;
	ASSERT_EQ_INT(INDIGO_OK, indigo_change_switch_property_1(&client, AGENT, LOAD, "keep", false));
	ASSERT_TRUE(wait_state(LOAD, -1));
	ASSERT_EQ_INT(before, selection_requests);
	ASSERT_TRUE(has_item(MIRROR, FILTER, "Test Camera", -1));
	ASSERT_TRUE(has_item(LAST, "NAME", "keep", -1));
}

static void concurrent_remove(void) {
	ASSERT_TRUE(text_change(SAVE, "first", INDIGO_OK_STATE));
	hold_load = true;
	ASSERT_EQ_INT(INDIGO_OK, indigo_change_switch_property_1(&client, AGENT, LOAD, "first", true));
	double deadline = indigo_monotonic_time() + 2;
	while (!load_waiting && indigo_monotonic_time() < deadline) {
		indigo_usleep(1000);
	}
	ASSERT_TRUE(load_waiting);
	ASSERT_TRUE(text_change(REMOVE, "first", INDIGO_ALERT_STATE));
	hold_load = false;
	ASSERT_TRUE(wait_state(LOAD, -1));
	ASSERT_EQ_INT(INDIGO_OK_STATE, state(LOAD));
	ASSERT_TRUE(exists("first.saved"));
	ASSERT_TRUE(has_item(LAST, "NAME", "first", -1));
}

static void save_path_separator(void) {
	const char *names[] = { "../outside", "..\\outside", "/absolute", "C:outside", "a/b", "a\\b", ".", ".." };
	for (int i = 0; i < ARRAY_SIZE(names); i++) {
		ASSERT_TRUE(text_change(SAVE, names[i], INDIGO_ALERT_STATE));
		ASSERT_TRUE(text_change(REMOVE, names[i], INDIGO_ALERT_STATE));
	}
	ASSERT_FALSE(exists("../outside.saved"));
	ASSERT_TRUE(text_change(SAVE, "night..sky", INDIGO_OK_STATE));
	ASSERT_TRUE(text_change(REMOVE, "night..sky", INDIGO_OK_STATE));
}

static void discovery_compaction(void) {
	const char *items[] = { "Default", "Night" };
	indigo_property *a = publish("Camera A", "PROFILE", items, 2, 0, false);
	indigo_property *b = publish("Camera B", "PROFILE", items, 2, 1, false);
	publish("Camera C", "PROFILE", items, 2, 0, false);
	indigo_delete_property(&peer, a, NULL);
	ASSERT_TRUE(has_item(PROFILES, "Camera B", "Night", -1));
	ASSERT_TRUE(has_item(PROFILES, "Camera C", "Default", -1));
	indigo_delete_property(&peer, b, NULL);
	ASSERT_TRUE(has_item(PROFILES, "Camera C", "Default", -1));
	indigo_define_property(&peer, a, NULL);
	ASSERT_TRUE(has_item(PROFILES, "Camera A", "Default", -1));
}

static void agent_capacity_reuse(void) {
	const char *items[] = { "NONE", "Test Camera" };
	for (int i = 0; i < 17; i++) {
		char name[64];
		snprintf(name, sizeof(name), "Agent %02d", i);
		publish(name, FILTER, items, 2, 1, false);
	}
	ASSERT_EQ_INT(-1, state("AGENT_CONFIG Agent 16"));
	indigo_property *all = indigo_init_switch_property(NULL, "Agent 00", "", "", "", INDIGO_OK_STATE, INDIGO_RO_PERM, INDIGO_ANY_OF_MANY_RULE, 0);
	indigo_delete_property(&peer, all, NULL);
	indigo_release_property(all);
	indigo_define_property(&peer, props[16], NULL);
	ASSERT_TRUE(has_item("AGENT_CONFIG Agent 16", FILTER, "Test Camera", -1));
}

static void long_related_list(void) {
	char item_names[40][INDIGO_NAME_SIZE];
	const char *items[40];
	for (int i = 0; i < 40; i++) {
		memset(item_names[i], 'A' + i % 26, INDIGO_NAME_SIZE - 8);
		item_names[i][INDIGO_NAME_SIZE - 8] = 0;
		items[i] = item_names[i];
	}
	indigo_property *p = publish("Test Agent", RELATED, items, 40, -1, true);
	for (int i = 0; i < 40; i++) { p->items[i].sw.value = true; }
	indigo_update_property(&peer, p, NULL);
	indigo_property *copy = snapshot(MIRROR);
	ASSERT_TRUE(copy != NULL && copy->count == 1);
	size_t size = strlen(copy->items[0].text.value);
	ASSERT_EQ_INT(INDIGO_VALUE_SIZE - 1, size);
	ASSERT_EQ_INT('A', copy->items[0].text.value[0]);
	indigo_release_property(copy);
	choose(p, -1);
	ASSERT_TRUE(has_item(MIRROR, RELATED, "", -1));
}

static void filter_prefix_consistency(void) {
	const char *items[] = { "NONE", "Camera" };
	indigo_property *p = publish("Test Agent", "FILTERX_CCD_LIST", items, 2, 0, false);
	ASSERT_EQ_INT(-1, state(MIRROR));
	choose(p, 1);
	ASSERT_EQ_INT(-1, state(MIRROR));
}

static void all_filter_classes(void) {
	const char *names[] = { "FILTER_CCD_LIST", "FILTER_WHEEL_LIST", "FILTER_FOCUSER_LIST", "FILTER_MOUNT_LIST", "FILTER_GUIDER_LIST", "FILTER_DOME_LIST", "FILTER_GPS_LIST", "FILTER_ROTATOR_LIST", "FILTER_AUX_1_LIST" };
	const char *items[] = { "NONE", "Selected Device" };
	for (int i = 0; i < ARRAY_SIZE(names); i++) {
		publish("Test Agent", names[i], items, 2, 1, false);
		ASSERT_TRUE(has_item(MIRROR, names[i], "Selected Device", -1));
	}
	ASSERT_TRUE(text_change(SAVE, "classes", INDIGO_OK_STATE));
	ASSERT_TRUE(load("classes", INDIGO_OK_STATE));
	for (int i = 0; i < ARRAY_SIZE(names); i++) {
		ASSERT_TRUE(has_item(MIRROR, names[i], "Selected Device", -1));
	}
}

static void xml_escaping(void) {
	const char *items[] = { "A&B <night> \"quoted\"" };
	publish("Camera & One", "PROFILE", items, 1, 0, false);
	ASSERT_TRUE(text_change(SAVE, "escaping", INDIGO_OK_STATE));
	ASSERT_TRUE(file_contains("escaping.saved", "&amp;"));
	ASSERT_TRUE(load("escaping", INDIGO_OK_STATE));
	ASSERT_TRUE(has_item(PROFILES, "Camera & One", items[0], -1));
}

static void setup_restart(void) {
	option("AUTOSAVE_DEVICE_CONFIGS", true);
	option("UNLOAD_UNUSED_DRIVERS", true);
	ASSERT_EQ_INT(INDIGO_OK, indigo_agent_config(INDIGO_DRIVER_SHUTDOWN, NULL));
	ASSERT_EQ_INT(INDIGO_OK, indigo_agent_config(INDIGO_DRIVER_INIT, NULL));
	client_detached = false;
	ASSERT_TRUE(has_item(SETUP, "AUTOSAVE_DEVICE_CONFIGS", NULL, 1));
	ASSERT_TRUE(has_item(SETUP, "UNLOAD_UNUSED_DRIVERS", NULL, 1));
}

static void setup_save_failure(void) {
	char original[256];
	strcpy(original, config_folder);
	strcpy(config_folder, "/dev/null/config-test");
	ASSERT_EQ_INT(INDIGO_OK, indigo_change_switch_property_1(&client, AGENT, SETUP, "AUTOSAVE_DEVICE_CONFIGS", true));
	strcpy(config_folder, original);
	ASSERT_EQ_INT(INDIGO_ALERT_STATE, state(SETUP));
	option("AUTOSAVE_DEVICE_CONFIGS", false);
	ASSERT_TRUE(exists("Configuration_Agent.config"));
}

static void nondefault_port_load(void) {
	indigo_server_tcp_port = 7625;
	ASSERT_TRUE(text_change(SAVE, "port", INDIGO_OK_STATE));
	ASSERT_TRUE(exists("port_7625.saved"));
	ASSERT_TRUE(has_item(LOAD, "port", NULL, 0));
	ASSERT_TRUE(load("port", INDIGO_OK_STATE));
	ASSERT_TRUE(has_item(LAST, "NAME", "port", -1));
	ASSERT_TRUE(fixture("other_7626.saved", ""));
	refresh();
	ASSERT_FALSE(has_item(LOAD, "other_7626", NULL, -1));
	indigo_is_ephemeral_port = true;
	ASSERT_TRUE(text_change(SAVE, "ephemeral", INDIGO_OK_STATE));
	ASSERT_TRUE(exists("ephemeral.saved"));
	ASSERT_TRUE(load("ephemeral", INDIGO_OK_STATE));
	ASSERT_TRUE(text_change(REMOVE, "ephemeral", INDIGO_OK_STATE));
}

static void shutdown_idle_reinitialize(void) {
	populate();
	ASSERT_TRUE(text_change(SAVE, "idle", INDIGO_OK_STATE));
	ASSERT_TRUE(load("idle", INDIGO_OK_STATE));
	ASSERT_EQ_INT(INDIGO_OK, indigo_agent_config(INDIGO_DRIVER_SHUTDOWN, NULL));
	ASSERT_EQ_INT(INDIGO_OK, indigo_agent_config(INDIGO_DRIVER_INIT, NULL));
	client_detached = false;
	ASSERT_EQ_INT(INDIGO_OK, indigo_enumerate_properties(&client, &INDIGO_ALL_PROPERTIES));
	ASSERT_TRUE(load("idle", INDIGO_OK_STATE));
}

static void direct_restore_capacity(void) {
	populate();
	for (int i = 0; i < 64; i++) {
		indigo_property *p = indigo_init_text_property(NULL, AGENT, PROFILES, "", "", INDIGO_OK_STATE, INDIGO_RW_PERM, 1);
		indigo_init_text_item(p->items, "Test Camera", "Test Camera", i % 2 ? "Night" : "Default");
		ASSERT_EQ_INT(INDIGO_OK, indigo_change_property(&client, p));
		indigo_release_property(p);
		double deadline = indigo_monotonic_time() + 1;
		while (indigo_monotonic_time() < deadline) {
			pthread_mutex_lock(&peer_mutex);
			int requests = profile_requests;
			pthread_mutex_unlock(&peer_mutex);
			if (requests == i + 1) { break; }
			indigo_usleep(1000);
		}
		pthread_mutex_lock(&peer_mutex);
		int requests = profile_requests;
		pthread_mutex_unlock(&peer_mutex);
		ASSERT_EQ_INT(i + 1, requests);
	}
}

static void empty_selection_roundtrip(void) {
	populate();
	choose(selection, 0);
	choose(related, -1);
	ASSERT_TRUE(text_change(SAVE, "empty_selection", INDIGO_OK_STATE));
	choose(selection, 1);
	choose(related, 0);
	ASSERT_TRUE(load("empty_selection", INDIGO_OK_STATE));
	ASSERT_TRUE(has_item(MIRROR, FILTER, "", -1));
	ASSERT_TRUE(has_item(MIRROR, RELATED, "", -1));
}

static int hook_count;

static void publish_delayed_profile(void) {
	if (++hook_count == 3) {
		const char *items[] = { "Default", "Night" };
		profile = publish("Delayed Camera", "PROFILE", items, 2, 0, false);
	}
}

static void delayed_profile(void) {
	ASSERT_TRUE(fixture("delayed.saved", "<newTextVector device='Configuration Agent' name='AGENT_CONFIG_PROFILES'><oneText name='Delayed Camera'>Night</oneText></newTextVector>"));
	refresh();
	sleep_hook = publish_delayed_profile;
	ASSERT_TRUE(load("delayed", INDIGO_OK_STATE));
	ASSERT_TRUE(has_item(PROFILES, "Delayed Camera", "Night", -1));
	ASSERT_EQ_INT(1, profile_requests);
}

static void finish_selection(void) {
	if (selection && selection->state == INDIGO_BUSY_STATE) {
		selection->state = INDIGO_OK_STATE;
		indigo_update_property(&peer, selection, NULL);
	}
}

static void busy_then_ok(void) {
	populate();
	indigo_delete_property(&peer, related, NULL);
	ASSERT_TRUE(text_change(SAVE, "eventual", INDIGO_OK_STATE));
	busy_select = true;
	sleep_hook = finish_selection;
	ASSERT_TRUE(load("eventual", INDIGO_OK_STATE));
	ASSERT_TRUE(has_item(MIRROR, FILTER, "Test Camera", -1));
}

static void *shutdown_worker(void *unused) {
	indigo_agent_config(INDIGO_DRIVER_SHUTDOWN, NULL);
	return NULL;
}

static void shutdown_active(void) {
	populate();
	ASSERT_TRUE(text_change(SAVE, "active", INDIGO_OK_STATE));
	hold_load = true;
	ASSERT_EQ_INT(INDIGO_OK, indigo_change_switch_property_1(&client, AGENT, LOAD, "active", true));
	double deadline = indigo_monotonic_time() + 2;
	while (!load_waiting && indigo_monotonic_time() < deadline) { indigo_usleep(1000); }
	ASSERT_TRUE(load_waiting);
	pthread_t thread;
	ASSERT_EQ_INT(0, pthread_create(&thread, NULL, shutdown_worker, NULL));
	deadline = indigo_monotonic_time() + 0.1;
	while (!client_detached && indigo_monotonic_time() < deadline) { indigo_usleep(1000); }
	hold_load = false;
	ASSERT_EQ_INT(0, pthread_join(thread, NULL));
	agent_started = false;
	ASSERT_EQ_INT(0, late_commands);
}

static void related_failure(void) {
	populate();
	ASSERT_TRUE(text_change(SAVE, "related_failure", INDIGO_OK_STATE));
	choose(related, -1);
	reject_related = true;
	ASSERT_TRUE(load("related_failure", INDIGO_ALERT_STATE));
}

static void driver_unload_new(void) {
	populate();
	ASSERT_TRUE(text_change(SAVE, "before_new_driver", INDIGO_OK_STATE));
	pthread_mutex_lock(&peer_mutex);
	drivers = indigo_resize_property(drivers, 4);
	props[0] = drivers;
	indigo_init_switch_item(drivers->items + 3, "indigo_new_driver", "New driver", true);
	indigo_update_property(&peer, drivers, NULL);
	pthread_mutex_unlock(&peer_mutex);
	option("UNLOAD_UNUSED_DRIVERS", true);
	ASSERT_TRUE(load("before_new_driver", INDIGO_OK_STATE));
	ASSERT_TRUE(has_item(DRIVERS, "indigo_new_driver", NULL, 0));
}

static bool setup(void) {
	indigo_use_host_suffix = false;
	indigo_set_log_level(INDIGO_LOG_ERROR);
	REQUIRE(indigo_start() == INDIGO_OK);
	bus_started = true;
	REQUIRE(indigo_attach_client(&client) == INDIGO_OK);
	client_attached = true;
	REQUIRE(indigo_attach_device(&peer) == INDIGO_OK);
	REQUIRE(indigo_agent_config(INDIGO_DRIVER_INIT, NULL) == INDIGO_OK);
	agent_started = true;
	REQUIRE(indigo_enumerate_properties(&client, &INDIGO_ALL_PROPERTIES) == INDIGO_OK);
	return true;
}

static void cleanup(void) {
	hold_load = false;
	if (agent_started) {
		if (indigo_agent_config(INDIGO_DRIVER_SHUTDOWN, NULL) != INDIGO_OK) { indigo_test_failures++; }
	}
	indigo_detach_device(&peer);
	if (client_attached) { indigo_detach_client(&client); }
	if (bus_started) { indigo_stop(); }
	for (int i = 0; i < prop_count; i++) { indigo_release_property(props[i]); }
	for (int i = 0; i < ARRAY_SIZE(cache); i++) { indigo_release_property(cache[i].property); }
}

static void remove_files(const char *folder) {
	DIR *dir = opendir(folder);
	if (!dir) { return; }
	struct dirent *entry;
	while ((entry = readdir(dir))) {
		if (!strcmp(entry->d_name, ".") || !strcmp(entry->d_name, "..")) { continue; }
		char path[512];
		snprintf(path, sizeof(path), "%s/%s", folder, entry->d_name);
		struct stat st;
		if (lstat(path, &st) == 0 && S_ISDIR(st.st_mode)) { remove_files(path); } else { unlink(path); }
	}
	closedir(dir);
	rmdir(folder);
}

static const indigo_test_case tests[] = {
	{ "restore_queue_overflow", restore_queue_overflow },
	{ "related_failure", related_failure },
	{ "driver_unload_new", driver_unload_new },
	{ "empty_selection_roundtrip", empty_selection_roundtrip },
	{ "delayed_profile", delayed_profile },
	{ "busy_then_ok", busy_then_ok },
	{ "shutdown_active", shutdown_active },

	{ "save_write_failure", save_write_failure },
	{ "selection_alert_masked", selection_alert_masked },
	{ "empty_load", empty_load },
	{ "concurrent_remove", concurrent_remove },
	{ "save_path_separator", save_path_separator },
	{ "discovery_compaction", discovery_compaction },
	{ "agent_capacity_reuse", agent_capacity_reuse },
	{ "long_related_list", long_related_list },
	{ "filter_prefix_consistency", filter_prefix_consistency },
	{ "all_filter_classes", all_filter_classes },
	{ "xml_escaping", xml_escaping },
	{ "setup_restart", setup_restart },
	{ "setup_save_failure", setup_save_failure },
	{ "nondefault_port_load", nondefault_port_load },
	{ "shutdown_idle_reinitialize", shutdown_idle_reinitialize },
	{ "direct_restore_capacity", direct_restore_capacity },

	{ "lifecycle", lifecycle },
	{ "schema", schema },
	{ "discovery", discovery },
	{ "remote_ignored", remote_ignored },
	{ "deletion", deletion },
	{ "save_roundtrip", save_roundtrip },
	{ "overwrite", overwrite },
	{ "save_empty_and_failure", save_empty_and_failure },
	{ "remove_roundtrip", remove_roundtrip },
	{ "setup_persistence", setup_persistence },
	{ "autosave", autosave },
	{ "driver_unload_policy", driver_unload_policy },
	{ "repeated_load", repeated_load },
	{ "missing_profile", missing_profile },
	{ "selection_failure", selection_failure },
	{ "busy_guard", busy_guard },
	{ "missing_file", missing_file },
	{ "malformed_file", malformed_file },
	{ "scan_suffix", scan_suffix },
	{ "alternate_folder_remove", alternate_folder_remove },
	{ "port_namespace", port_namespace },
	{ "deselection_timeout", deselection_timeout },
	{ "restore_busy_timeout", restore_busy_timeout },
	{ "profile_rejection", profile_rejection },
	{ "driver_rejection", driver_rejection },
	{ "absent_agent", absent_agent },
	{ "server_disappearance", server_disappearance },
	{ "profile_no_selection", profile_no_selection },
	{ "capacity_restore", capacity_restore },
	{ "autosave_reentrant", autosave_reentrant },
	{ "concurrent_save", concurrent_save },
	{ "startup_scan", startup_scan },
	{ "enumeration_filter", enumeration_filter },
};

int main(int argc, char **argv) {
	setvbuf(stdout, NULL, _IONBF, 0);
	int failed = 0, executed = 0;
	for (int i = 0; i < ARRAY_SIZE(tests); i++) {
		if (argc > 1 && !strstr(tests[i].name, argv[1])) { continue; }
		executed++;
		strcpy(test_root, "/tmp/indigo_config_test_XXXXXX");
		if (!mkdtemp(test_root)) { return 1; }
		snprintf(config_folder, sizeof(config_folder), "%s/.indigo", test_root);
		mkdir(config_folder, 0700);
		pid_t child = fork();
		if (child == 0) {
			alarm(45);
			int status = setup() ? indigo_run_tests("Configuration Agent integration", tests + i, 1) : 1;
			cleanup();
			exit(status || indigo_test_failures ? 1 : 0);
		}
		int status = -1;
		if (child < 0 || waitpid(child, &status, 0) != child || !WIFEXITED(status) || WEXITSTATUS(status)) {
			fprintf(stderr, "FAILED %s (status %d)\n", tests[i].name, status);
			failed++;
		}
		remove_files(test_root);
	}
	printf("Configuration Agent: %d/%d passed (including cleanup)\n", executed - failed, executed);
	return executed ? (failed ? 1 : 0) : 2;
}
