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
#include <indigo/indigo_filter.h>
#include <indigo/indigo_align.h>
#include <indigo_drivers/agent_mount/indigo_agent_mount.h>

#include "../test_runner.h"

#define AGENT "Mount Agent"
#define ARRAY_SIZE(a) (sizeof(a) / sizeof((a)[0]))
#define REQUIRE(c) do { if (!(c)) { fprintf(stderr, "%s:%d: %s\n", __FILE__, __LINE__, #c); indigo_test_failures++; return false; } } while (0)

typedef struct {
	indigo_property *property;
	unsigned revision;
} observation;
static observation cache[2048];
static pthread_mutex_t cache_mutex = PTHREAD_MUTEX_INITIALIZER;
static indigo_client client;
static char config_folder[] = "/tmp/indigo_mount_test_XXXXXX";
// Each case forks before starting INDIGO threads and isolates configuration writes.
static bool bus_started, client_attached, agent_started;

const char *mount_test_config_folder(void) {
	return config_folder;
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

static indigo_result message(indigo_client *client, indigo_device *device, indigo_property *property, const char *text) {
	fprintf(stderr, "  %s: %s\n", device->name, text ? text : "");
	return INDIGO_OK;
}

static indigo_client client = { .name = "Mount integration client", .define_property = defined, .update_property = updated, .delete_property = deleted, .send_message = message };

static indigo_property *snapshot(const char *device, const char *name) {
	pthread_mutex_lock(&cache_mutex);
	observation *entry = find(device, name);
	indigo_property *result = entry ? indigo_copy_property(NULL, entry->property) : NULL;
	pthread_mutex_unlock(&cache_mutex);
	return result;
}

static unsigned revision(const char *device, const char *name) {
	pthread_mutex_lock(&cache_mutex);
	observation *entry = find(device, name);
	unsigned result = entry ? entry->revision : 0;
	pthread_mutex_unlock(&cache_mutex);
	return result;
}

static double value(const char *device, const char *name, const char *item) {
	indigo_property *p = snapshot(device, name);
	double result = NAN;
	if (p) {
		for (int i = 0; i < p->count; i++) {
			if (!strcmp(p->items[i].name, item)) {
				result = p->type == INDIGO_SWITCH_VECTOR ? p->items[i].sw.value : p->type == INDIGO_LIGHT_VECTOR ? p->items[i].light.value : p->items[i].number.value;
			}
		}
		indigo_release_property(p);
	}
	return result;
}

static bool wait_state(const char *device, const char *name, unsigned after, int state) {
	double deadline = indigo_monotonic_time() + 5;
	while (indigo_monotonic_time() < deadline) {
		pthread_mutex_lock(&cache_mutex);
		observation *entry = find(device, name);
		bool ready = entry && entry->revision > after && (state < 0 ? entry->property->state != INDIGO_BUSY_STATE : entry->property->state == state);
		pthread_mutex_unlock(&cache_mutex);
		if (ready) {
			return true;
		}
		indigo_usleep(1000);
	}
	fprintf(stderr, "Timeout: %s.%s state %d after %u (now %u)\n", device, name, state, after, revision(device, name));
	return false;
}

static bool sw(const char *device, const char *name, const char *item, bool enabled, int state) {
	REQUIRE(!isnan(value(device, name, item)));
	if (getenv("INDIGO_TEST_TRACE")) {
		fprintf(stderr, "change %s.%s.%s = %d\n", device, name, item, enabled);
	}
	unsigned before = revision(device, name);
	REQUIRE(indigo_change_switch_property_1(&client, device, name, item, enabled) == INDIGO_OK);
	return wait_state(device, name, before, state);
}

static bool num(const char *device, const char *name, const char *item, double number) {
	if (getenv("INDIGO_TEST_TRACE")) {
		fprintf(stderr, "NUMBER %s %s %s = %g\n", device, name, item, number);
	}
	REQUIRE(!isnan(value(device, name, item)));
	unsigned before = revision(device, name);
	REQUIRE(indigo_change_number_property_1(&client, device, name, item, number) == INDIGO_OK);
	return wait_state(device, name, before, INDIGO_OK_STATE);
}

#define START AGENT_START_PROCESS_PROPERTY_NAME
#define TARGET AGENT_MOUNT_TARGET_COORDINATES_PROPERTY_NAME
#define DISPLAY AGENT_MOUNT_DISPLAY_COORDINATES_PROPERTY_NAME
#define FEATURES AGENT_PROCESS_FEATURES_PROPERTY_NAME
#define CHECK(c) do { if (!(c)) { fprintf(stderr, "%s:%d: %s\n", __FILE__, __LINE__, #c); indigo_test_failures++; return; } } while (0)

typedef struct {
	indigo_device device;
	indigo_property *properties[32];
	int count, mask;
	bool attached, modern, limited;
} peer;
static peer peers[7];
static const char *peer_names[] = { "Test Mount", "Test Dome", "Test Rotator", "Test GPS", "Test Joystick", "Imager Agent Test", "Guider Agent Test" };
static const char *lists[] = { "FILTER_MOUNT_LIST", "FILTER_DOME_LIST", "FILTER_ROTATOR_LIST", "FILTER_GPS_LIST", "FILTER_JOYSTICK_LIST" };
static pthread_mutex_t peer_mutex;
typedef struct {
	int peer;
	indigo_property *property;
} command;
static command commands[4096];
static int command_count;
static bool immediate, reject_motion;

static indigo_property *prop(int index, const char *name) {
	for (int i = 0; i < peers[index].count; i++) {
		if (!strcmp(peers[index].properties[i]->name, name)) {
			return peers[index].properties[i];
		}
	}
	return NULL;
}

static indigo_property *add(int index, const char *name, int type, const char *items) {
	char buffer[512];
	strcpy(buffer, items);
	int count = 1;
	for (const char *p = items; *p; p++) {
		count += *p == ',';
	}
	indigo_property *p = type == INDIGO_SWITCH_VECTOR ? indigo_init_switch_property(NULL, peer_names[index], name, "Test", name, INDIGO_OK_STATE, INDIGO_RW_PERM, INDIGO_ONE_OF_MANY_RULE, count) : type == INDIGO_LIGHT_VECTOR ? indigo_init_light_property(NULL, peer_names[index], name, "Test", name, INDIGO_OK_STATE, count) : type == INDIGO_TEXT_VECTOR ? indigo_init_text_property(NULL, peer_names[index], name, "Test", name, INDIGO_OK_STATE, INDIGO_RW_PERM, count) : indigo_init_number_property(NULL, peer_names[index], name, "Test", name, INDIGO_OK_STATE, INDIGO_RW_PERM, count);
	char *save = NULL;
	int i = 0;
	for (char *item = strtok_r(buffer, ",", &save); item; item = strtok_r(NULL, ",", &save), i++) {
		if (type == INDIGO_SWITCH_VECTOR) {
			indigo_init_switch_item(p->items + i, item, item, i == count - 1);
		} else if (type == INDIGO_LIGHT_VECTOR) {
			indigo_init_light_item(p->items + i, item, item, INDIGO_IDLE_STATE);
		} else if (type == INDIGO_TEXT_VECTOR) {
			indigo_init_text_item(p->items + i, item, item, "");
		} else {
			indigo_init_number_item(p->items + i, item, item, -100000, 100000, 0, 0);
		}
	}
	peers[index].properties[peers[index].count++] = p;
	return p;
}

static bool motion(const char *name) {
	return !strcmp(name, "MOUNT_EQUATORIAL_COORDINATES") || !strcmp(name, "MOUNT_PARK") || !strcmp(name, "MOUNT_HOME") || !strcmp(name, "MOUNT_TRACKING") || !strcmp(name, "DOME_PARK") || !strcmp(name, "DOME_SHUTTER") || !strcmp(name, "DOME_HORIZONTAL_COORDINATES") || !strcmp(name, "ROTATOR_POSITION");
}

static void publish(int index, indigo_property *p) {
	p->do_update = true;
	indigo_update_property(&peers[index].device, p, NULL);
	if (peers[index].modern && index < 2) {
		const char *item = strstr(p->name, "PARK") ? "PARK" : strstr(p->name, "HOME") ? "HOME" : strstr(p->name, "TRACKING") ? "TRACK" : strstr(p->name, "SHUTTER") ? "OPEN" : strstr(p->name, "COORDINATES") && !strstr(p->name, "ON_") && !strstr(p->name, "GEOGRAPHIC") ? "SLEW" : NULL;
		indigo_property *state = prop(index, index == 0 ? "MOUNT_STATE" : "DOME_STATE");
		if (item && state) {
			indigo_item *light = indigo_get_item(state, item);
			if (light) {
				light->light.value = p->state == INDIGO_OK_STATE ? (!strcmp(item, "SLEW") ? INDIGO_IDLE_STATE : p->items[0].sw.value ? INDIGO_OK_STATE : INDIGO_IDLE_STATE) : p->state;
				indigo_update_property(&peers[index].device, state, NULL);
			}
		}
	}
}

static indigo_result peer_enumerate(indigo_device *device, indigo_client *client, indigo_property *request) {
	peer *p = device->private_data;
	if (IS_CONNECTED || p->mask == INDIGO_INTERFACE_AGENT) {
		for (int i = 0; i < p->count; i++) {
			if (indigo_property_match(p->properties[i], request)) {
				indigo_define_property(device, p->properties[i], NULL);
			}
		}
	}
	return indigo_device_enumerate_properties(device, client, request);
}

static indigo_result peer_attach(indigo_device *device) {
	peer *p = device->private_data;
	indigo_result result = indigo_device_attach(device, "mount_agent_test_peer", 1, p->mask);
	peer_enumerate(device, NULL, NULL);
	return result;
}

static indigo_result peer_change(indigo_device *device, indigo_client *client, indigo_property *request) {
	peer *p = device->private_data;
	int index = (int)(p - peers);
	if (indigo_property_match(CONNECTION_PROPERTY, request)) {
		indigo_property_copy_values(CONNECTION_PROPERTY, request, false);
		CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
		indigo_update_property(device, CONNECTION_PROPERTY, NULL);
		if (IS_CONNECTED) {
			peer_enumerate(device, NULL, NULL);
		} else {
			for (int i = 0; i < p->count; i++) {
				if (p->properties[i]->state == INDIGO_BUSY_STATE) {
					p->properties[i]->state = INDIGO_ALERT_STATE;
				}
				indigo_delete_property(device, p->properties[i], NULL);
			}
		}
		return INDIGO_OK;
	}
	pthread_mutex_lock(&peer_mutex);
	indigo_property *target = prop(index, request->name);
	if (target) {
		if (command_count < ARRAY_SIZE(commands)) {
			commands[command_count++] = (command){ index, indigo_copy_property(NULL, request) };
		}
		indigo_property_copy_values(target, request, false);
		target->state = motion(target->name) && index < 3 ? (reject_motion ? INDIGO_ALERT_STATE : immediate ? INDIGO_OK_STATE : INDIGO_BUSY_STATE) : INDIGO_OK_STATE;
		publish(index, target);
		if (strstr(target->name, "ABORT") && index < 3) {
			for (int i = 0; i < p->count; i++) {
				if (p->properties[i]->state == INDIGO_BUSY_STATE) {
					p->properties[i]->state = INDIGO_ALERT_STATE;
					publish(index, p->properties[i]);
				}
			}
		}
		pthread_mutex_unlock(&peer_mutex);
		return INDIGO_OK;
	}
	pthread_mutex_unlock(&peer_mutex);
	return indigo_device_change_property(device, client, request);
}

static indigo_result peer_detach(indigo_device *device) {
	indigo_delete_property(device, NULL, NULL);
	return indigo_device_detach(device);
}

static bool attach_peer(int index, bool modern, bool limited) {
	peer *p = peers + index;
	static const int masks[] = { INDIGO_INTERFACE_MOUNT, INDIGO_INTERFACE_DOME, INDIGO_INTERFACE_ROTATOR, INDIGO_INTERFACE_GPS, INDIGO_INTERFACE_AUX_JOYSTICK, INDIGO_INTERFACE_AGENT, INDIGO_INTERFACE_AGENT };
	p->mask = masks[index];
	p->modern = modern;
	p->limited = limited;
	p->device = (indigo_device)INDIGO_DEVICE_INITIALIZER("", peer_attach, peer_enumerate, peer_change, NULL, peer_detach);
	strcpy(p->device.name, peer_names[index]);
	p->device.private_data = p;
	if (index < 4) {
		add(index, "GEOGRAPHIC_COORDINATES", INDIGO_NUMBER_VECTOR, "LATITUDE,LONGITUDE,ELEVATION");
	}
	if (!limited && (index == 0 || index == 4)) {
		add(index, "MOUNT_ON_COORDINATES_SET", INDIGO_SWITCH_VECTOR, "TRACK,SYNC,SLEW");
		add(index, "MOUNT_EQUATORIAL_COORDINATES", INDIGO_NUMBER_VECTOR, "RA,DEC");
		add(index, "MOUNT_PARK", INDIGO_SWITCH_VECTOR, "PARKED,UNPARKED");
		add(index, "MOUNT_HOME", INDIGO_SWITCH_VECTOR, "HOME");
		add(index, "MOUNT_TRACKING", INDIGO_SWITCH_VECTOR, "ON,OFF");
		add(index, "MOUNT_ABORT_MOTION", INDIGO_SWITCH_VECTOR, "ABORT_MOTION");
		add(index, "MOUNT_SLEW_RATE", INDIGO_SWITCH_VECTOR, "GUIDE,CENTERING,FIND,MAX");
		add(index, "MOUNT_MOTION_RA", INDIGO_SWITCH_VECTOR, "WEST,EAST")->rule = INDIGO_AT_MOST_ONE_RULE;
		add(index, "MOUNT_MOTION_DEC", INDIGO_SWITCH_VECTOR, "NORTH,SOUTH")->rule = INDIGO_AT_MOST_ONE_RULE;
		add(index, "MOUNT_SET_HOST_TIME", INDIGO_SWITCH_VECTOR, "SET_HOST_TIME");
		add(index, "MOUNT_SIDE_OF_PIER", INDIGO_SWITCH_VECTOR, "EAST,WEST");
		add(index, "MOUNT_LST_TIME", INDIGO_NUMBER_VECTOR, "TIME");
		if (modern) {
			add(index, "MOUNT_STATE", INDIGO_LIGHT_VECTOR, "SLEW,PARK,HOME,TRACK");
		}
	} else if (!limited && index == 1) {
		add(index, "DOME_ON_COORDINATES_SET", INDIGO_SWITCH_VECTOR, "GOTO,SYNC");
		add(index, "DOME_HORIZONTAL_COORDINATES", INDIGO_NUMBER_VECTOR, "AZ");
		add(index, "DOME_PARK", INDIGO_SWITCH_VECTOR, "PARKED,UNPARKED");
		add(index, "DOME_SHUTTER", INDIGO_SWITCH_VECTOR, "OPENED,CLOSED");
		add(index, "DOME_ABORT_MOTION", INDIGO_SWITCH_VECTOR, "ABORT_MOTION");
		add(index, "DOME_SET_HOST_TIME", INDIGO_SWITCH_VECTOR, "SET_HOST_TIME");
		indigo_property *dim = add(index, "DOME_DIMENSION", INDIGO_NUMBER_VECTOR, "RADIUS,SHUTTER_WIDTH,MOUNT_PIVOT_OFFSET_NS,MOUNT_PIVOT_OFFSET_EW,MOUNT_PIVOT_VERTICAL_OFFSET,MOUNT_PIVOT_OTA_OFFSET");
		dim->items[0].number.value = dim->items[0].number.target = 3;
		dim->items[1].number.value = dim->items[1].number.target = 1;
		add(index, "DOME_SLAVING_PARAMETERS", INDIGO_NUMBER_VECTOR, "MOVE_THRESHOLD")->items[0].number.value = 0.1;
		if (modern) {
			add(index, "DOME_STATE", INDIGO_LIGHT_VECTOR, "SLEW,PARK,OPEN");
		}
	} else if (index == 2) {
		add(index, "ROTATOR_POSITION", INDIGO_NUMBER_VECTOR, "POSITION");
		add(index, "ROTATOR_ON_POSITION_SET", INDIGO_SWITCH_VECTOR, "GOTO,SYNC");
		add(index, "ROTATOR_ABORT_MOTION", INDIGO_SWITCH_VECTOR, "ABORT_MOTION");
	} else if (index >= 5) {
		add(index, "AGENT_ABORT_PROCESS", INDIGO_SWITCH_VECTOR, "ABORT");
		add(index, "AGENT_START_PROCESS", INDIGO_SWITCH_VECTOR, "CLEAR_SELECTION");
		add(index, "CCD_SET_FITS_HEADER", INDIGO_TEXT_VECTOR, "KEYWORD,VALUE");
		add(index, "CCD_REMOVE_FITS_HEADER", INDIGO_TEXT_VECTOR, "KEYWORD");
		if (index == 6) {
			add(index, "AGENT_GUIDER_MOUNT_COORDINATES", INDIGO_NUMBER_VECTOR, "RA,DEC,SIDE_OF_PIER");
		}
	}
	REQUIRE(indigo_attach_device(&p->device) == INDIGO_OK);
	p->attached = true;
	return true;
}

static bool select_peer(int index, bool modern, bool limited) {
	if (!peers[index].attached) {
		REQUIRE(attach_peer(index, modern, limited));
	}
	REQUIRE(sw(AGENT, index < 5 ? lists[index] : "FILTER_RELATED_AGENT_LIST", peer_names[index], true, INDIGO_OK_STATE));
	return true;
}

static int requests(int index, const char *name) {
	pthread_mutex_lock(&peer_mutex);
	int n = 0;
	for (int i = 0; i < command_count; i++) {
		n += commands[i].peer == index && !strcmp(commands[i].property->name, name);
	}
	pthread_mutex_unlock(&peer_mutex);
	return n;
}

static bool wait_request(int index, const char *name, int before) {
	double end = indigo_monotonic_time() + 5;
	while (requests(index, name) <= before && indigo_monotonic_time() < end) {
		indigo_usleep(1000);
	}
	REQUIRE(requests(index, name) > before);
	return true;
}

static void complete(int index, const char *name, int state) {
	pthread_mutex_lock(&peer_mutex);
	indigo_property *p = prop(index, name);
	p->state = state;
	publish(index, p);
	pthread_mutex_unlock(&peer_mutex);
}

static void emit_number(int index, const char *name, const char *item, double number) {
	pthread_mutex_lock(&peer_mutex);
	indigo_property *p = prop(index, name);
	indigo_get_item(p, item)->number.value = number;
	indigo_update_property(&peers[index].device, p, NULL);
	pthread_mutex_unlock(&peer_mutex);
}

static int state(const char *device, const char *name) {
	indigo_property *p = snapshot(device, name);
	int result = p ? p->state : -1;
	indigo_release_property(p);
	return result;
}

// Only the LX200 transport boundary is substituted. The production parser,
// dispatcher, filter, timers and handler queues remain unchanged.
static void (*lx_worker)(indigo_uni_worker_data *);
static void *lx_device;
static atomic_bool server_open, server_fail;
static indigo_uni_handle listener, connection;
static const char *lx_input;
static size_t lx_offset;
static char lx_output[8192];
static long lx_read_error;
static atomic_int socket_closes;

void mount_test_server(int *port, indigo_uni_handle **handle, void (*worker)(indigo_uni_worker_data *), void *data, void (*callback)(int), int level) {
	lx_worker = worker;
	lx_device = data;
	if (server_fail) {
		return;
	}
	*handle = &listener;
	server_open = true;
	while (server_open) {
		indigo_usleep(1000);
	}
	*handle = NULL;
}

void mount_test_close(indigo_uni_handle **handle) {
	if (*handle == &listener) {
		server_open = false;
		socket_closes++;
		*handle = NULL;
	} else if (*handle == &connection) {
		socket_closes++;
		*handle = NULL;
	} else {
		indigo_uni_close(handle);
	}
}

void mount_test_read_timeout(indigo_uni_handle *handle, long timeout) {
}

long mount_test_read(indigo_uni_handle *handle, void *buffer, long size) {
	if (!lx_input[lx_offset]) {
		return lx_read_error;
	}
	*(char *)buffer = lx_input[lx_offset++];
	return 1;
}

long mount_test_write(indigo_uni_handle *handle, const char *buffer, long size) {
	if (strlen(lx_output) + size < sizeof(lx_output)) {
		strncat(lx_output, buffer, size);
	}
	return size;
}

static void exchange(const char *input) {
	lx_input = input;
	lx_offset = 0;
	lx_output[0] = 0;
	indigo_uni_worker_data *data = indigo_safe_malloc(sizeof(*data));
	data->handle = &connection;
	data->data = lx_device;
	lx_worker(data);
}

static bool start_server(void) {
	REQUIRE(sw(AGENT, AGENT_LX200_SERVER_PROPERTY_NAME, "STARTED", true, INDIGO_OK_STATE));
	double deadline = indigo_monotonic_time() + 5;
	while (!server_open && indigo_monotonic_time() < deadline) {
		indigo_usleep(1000);
	}
	REQUIRE(server_open);
	return true;
}

static bool setup(void) {
	pthread_mutexattr_t attr;
	pthread_mutexattr_init(&attr);
	pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);
	pthread_mutex_init(&peer_mutex, &attr);
	pthread_mutexattr_destroy(&attr);
	REQUIRE(indigo_start() == INDIGO_OK);
	bus_started = true;
	REQUIRE(indigo_attach_client(&client) == INDIGO_OK);
	client_attached = true;
	REQUIRE(indigo_agent_mount(INDIGO_DRIVER_INIT, NULL) == INDIGO_OK);
	agent_started = true;
	REQUIRE(wait_state(AGENT, START, 0, INDIGO_OK_STATE));
	return true;
}
static bool operation(const char *item, int index, const char *name, int result) {
	int before = requests(index, name);
	unsigned rev = revision(AGENT, START);
	REQUIRE(sw(AGENT, START, item, true, INDIGO_BUSY_STATE));
	REQUIRE(wait_request(index, name, before));
	complete(index, name, result);
	REQUIRE(wait_state(AGENT, START, rev, result));
	REQUIRE(value(AGENT, START, item) == 0);
	return true;
}

static const struct { const char *item; int peer; const char *property; } operations[] = {
	{ "SLEW", 0, "MOUNT_EQUATORIAL_COORDINATES" },
	{ "SYNC", 0, "MOUNT_EQUATORIAL_COORDINATES" },
	{ "PARK", 0, "MOUNT_PARK" },
	{ "UNPARK", 0, "MOUNT_PARK" },
	{ "HOME", 0, "MOUNT_HOME" },
	{ "TRACK_ON", 0, "MOUNT_TRACKING" },
	{ "TRACK_OFF", 0, "MOUNT_TRACKING" },
	{ "DOME_PARK", 1, "DOME_PARK" },
	{ "DOME_UNPARK", 1, "DOME_PARK" },
	{ "DOME_OPEN", 1, "DOME_SHUTTER" },
	{ "DOME_CLOSE", 1, "DOME_SHUTTER" }
};

static void metadata(void) {
	indigo_driver_info info;
	CHECK(indigo_agent_mount(INDIGO_DRIVER_INFO, &info) == INDIGO_OK);
	CHECK(!strcmp(info.name, "indigo_agent_mount"));
	CHECK(indigo_agent_mount(INDIGO_DRIVER_INIT, NULL) == INDIGO_OK);
	const struct { const char *name; int count; } inventory[] = {
		{ "GEOGRAPHIC_COORDINATES", 3 }, { "AGENT_SITE_DATA_SOURCE", 4 }, { "AGENT_SET_HOST_TIME", 2 }, { "ABORT_RELATED_PROCESS", 2 }, { "AGENT_LX200_SERVER", 2 }, { "AGENT_LX200_CONFIGURATION", 2 }, { "AGENT_LIMITS", 3 }, { "AGENT_MOUNT_FOV", 3 }, { TARGET, 2 }, { DISPLAY, 13 }, { START, 12 }, { "AGENT_ABORT_PROCESS", 1 }, { FEATURES, 7 }, { "AGENT_MOUNT_STATE", 6 }, { "AGENT_DOME_STATE", 3 }, { AGENT_MOUNT_FEATURES_PROPERTY_NAME, 5 }, { AGENT_DOME_FEATURES_PROPERTY_NAME, 4 }
	};
	for (int i = 0; i < ARRAY_SIZE(inventory); i++) {
		indigo_property *p = snapshot(AGENT, inventory[i].name);
		CHECK(p);
		int count = p->count;
		for (int j = 0; j < p->count; j++) {
			CHECK(*p->items[j].name);
		}
		indigo_release_property(p);
		CHECK(count == inventory[i].count);
	}
	CHECK(!snapshot(AGENT, "CONNECTION"));
	CHECK(value(AGENT, FEATURES, "ENABLE_DOME_SLAVING") == 0);
	CHECK(value(AGENT, FEATURES, "ENABLE_FIELD_DEROTATION") == 0);
	CHECK(value(AGENT, FEATURES, "ENABLE_JOYSTICK_CONTROL") == 0);
	CHECK(sw(AGENT, "AGENT_ABORT_PROCESS", "ABORT", true, INDIGO_OK_STATE));
	CHECK(value(AGENT, "AGENT_ABORT_PROCESS", "ABORT") == 0);
}

static void missing_devices(void) {
	for (int i = 0; i < ARRAY_SIZE(operations); i++) {
		CHECK(sw(AGENT, START, operations[i].item, true, INDIGO_ALERT_STATE));
		CHECK(value(AGENT, START, operations[i].item) == 0);
	}
}

static void missing_capabilities(void) {
	CHECK(select_peer(0, false, true));
	CHECK(select_peer(1, false, true));
	missing_devices();
	CHECK(requests(0, "MOUNT_PARK") == 0);
	CHECK(requests(1, "DOME_PARK") == 0);
}

static void run_matrix(bool modern, int terminal, bool aborting) {
	CHECK(select_peer(0, modern, false));
	CHECK(select_peer(1, modern, false));
	CHECK(num(AGENT, TARGET, "RA", 5.25));
	CHECK(num(AGENT, TARGET, "DEC", -20.5));
	for (int i = 0; i < ARRAY_SIZE(operations); i++) {
		fprintf(stderr, "  operation %s, %s, terminal %d\n", operations[i].item, modern ? "state lights" : "legacy", terminal);
		if (aborting) {
			int n = requests(operations[i].peer, operations[i].property);
			CHECK(sw(AGENT, START, operations[i].item, true, INDIGO_BUSY_STATE));
			CHECK(wait_request(operations[i].peer, operations[i].property, n));
			CHECK(sw(AGENT, "AGENT_ABORT_PROCESS", "ABORT", true, INDIGO_OK_STATE));
			CHECK(wait_state(AGENT, START, 0, INDIGO_ALERT_STATE));
		} else {
			CHECK(operation(operations[i].item, operations[i].peer, operations[i].property, terminal));
		}
	}
}

static void legacy_success(void) { run_matrix(false, INDIGO_OK_STATE, false); }

static void modern_success(void) { run_matrix(true, INDIGO_OK_STATE, false); }

static void legacy_failure(void) { run_matrix(false, INDIGO_ALERT_STATE, false); }

static void modern_failure(void) { run_matrix(true, INDIGO_ALERT_STATE, false); }

static void legacy_abort(void) { run_matrix(false, INDIGO_ALERT_STATE, true); }

static void modern_abort(void) { run_matrix(true, INDIGO_ALERT_STATE, true); }

static void coordinates_and_busy(void) {
	CHECK(select_peer(0, false, false));
	emit_number(0, "MOUNT_EQUATORIAL_COORDINATES", "RA", 3.25);
	emit_number(0, "MOUNT_EQUATORIAL_COORDINATES", "DEC", -12.75);
	CHECK(num(AGENT, TARGET, "RA", 8.5));
	CHECK(value(AGENT, TARGET, "RA") == 3.25);
	indigo_property *p = snapshot(AGENT, TARGET);
	CHECK(p && p->items[0].number.target == 8.5);
	indigo_release_property(p);
	int before = requests(0, "MOUNT_EQUATORIAL_COORDINATES");
	CHECK(sw(AGENT, START, "SLEW", true, INDIGO_BUSY_STATE));
	CHECK(wait_request(0, "MOUNT_EQUATORIAL_COORDINATES", before));
	CHECK(value(peer_names[0], "MOUNT_EQUATORIAL_COORDINATES", "RA") == 8.5);
	CHECK(value(peer_names[0], "MOUNT_ON_COORDINATES_SET", "TRACK") == 1);
	int park = requests(0, "MOUNT_PARK");
	indigo_change_switch_property_1(&client, AGENT, START, "PARK", true);
	CHECK(requests(0, "MOUNT_PARK") == park);
	CHECK(value(AGENT, START, "SLEW") == 1);
	complete(0, "MOUNT_EQUATORIAL_COORDINATES", INDIGO_OK_STATE);
	CHECK(wait_state(AGENT, START, 0, INDIGO_OK_STATE));
	CHECK(operation("SYNC", 0, "MOUNT_EQUATORIAL_COORDINATES", INDIGO_OK_STATE));
	CHECK(value(peer_names[0], "MOUNT_ON_COORDINATES_SET", "SYNC") == 1);
	CHECK(state(AGENT, DISPLAY) == INDIGO_IDLE_STATE);
	CHECK(operation("TRACK_ON", 0, "MOUNT_TRACKING", INDIGO_OK_STATE));
	CHECK(state(AGENT, DISPLAY) == INDIGO_OK_STATE);
	CHECK(isfinite(value(AGENT, DISPLAY, "RA_JNOW")));
	CHECK(fabs(value(AGENT, DISPLAY, "RA_JNOW") - 8.5) < 0.1);
	CHECK(sw(AGENT, lists[0], "NONE", true, INDIGO_OK_STATE));
	CHECK(value(AGENT, AGENT_MOUNT_FEATURES_PROPERTY_NAME, "SLEW") == 0);
	CHECK(value(AGENT, DISPLAY, "RA_JNOW") == 0);
	CHECK(select_peer(0, false, false));
	CHECK(operation("PARK", 0, "MOUNT_PARK", INDIGO_OK_STATE));
}

static void sites(void) {
	for (int i = 0; i < 4; i++) {
		CHECK(select_peer(i, false, false));
	}
	CHECK(num(AGENT, "GEOGRAPHIC_COORDINATES", "LATITUDE", 48.125));
	CHECK(num(AGENT, "GEOGRAPHIC_COORDINATES", "LONGITUDE", 17.25));
	CHECK(num(AGENT, "GEOGRAPHIC_COORDINATES", "ELEVATION", 230));
	for (int i = 0; i < 2; i++) {
		CHECK(value(peer_names[i], "GEOGRAPHIC_COORDINATES", "LATITUDE") == 48.125);
		CHECK(value(peer_names[i], "GEOGRAPHIC_COORDINATES", "LONGITUDE") == 17.25);
		CHECK(value(peer_names[i], "GEOGRAPHIC_COORDINATES", "ELEVATION") == 230);
	}
	const char *sources[] = { "MOUNT", "DOME", "GPS" };
	const int indices[] = { 0, 1, 3 };
	for (int j = 0; j < 3; j++) {
		CHECK(sw(AGENT, "AGENT_SITE_DATA_SOURCE", sources[j], true, INDIGO_OK_STATE));
		emit_number(indices[j], "GEOGRAPHIC_COORDINATES", "LATITUDE", -30 - j);
		emit_number(indices[j], "GEOGRAPHIC_COORDINATES", "LONGITUDE", 60 + j);
		emit_number(indices[j], "GEOGRAPHIC_COORDINATES", "ELEVATION", 500 + j);
		CHECK(value(AGENT, "GEOGRAPHIC_COORDINATES", "LATITUDE") == -30 - j);
		CHECK(value(AGENT, "GEOGRAPHIC_COORDINATES", "LONGITUDE") == 60 + j);
		CHECK(value(AGENT, "GEOGRAPHIC_COORDINATES", "ELEVATION") == 500 + j);
	}
	CHECK(num(AGENT, "AGENT_LIMITS", "COORDINATES_PROPAGATE_THRESHOLD", 1));
	int n = requests(0, "GEOGRAPHIC_COORDINATES");
	emit_number(3, "GEOGRAPHIC_COORDINATES", "LATITUDE", -32.5);
	CHECK(requests(0, "GEOGRAPHIC_COORDINATES") == n);
	emit_number(3, "GEOGRAPHIC_COORDINATES", "LATITUDE", -34);
	CHECK(requests(0, "GEOGRAPHIC_COORDINATES") > n);
	CHECK(sw(AGENT, "AGENT_SET_HOST_TIME", "MOUNT", false, INDIGO_OK_STATE));
	CHECK(sw(AGENT, "AGENT_SET_HOST_TIME", "DOME", false, INDIGO_OK_STATE));
	n = requests(0, "MOUNT_SET_HOST_TIME");
	CHECK(sw(AGENT, "AGENT_SITE_DATA_SOURCE", "HOST", true, INDIGO_OK_STATE));
	CHECK(requests(0, "MOUNT_SET_HOST_TIME") == n);
	CHECK(value(AGENT, "GEOGRAPHIC_COORDINATES", "LATITUDE") == 48.125);
	CHECK(sw(AGENT, "AGENT_SET_HOST_TIME", "MOUNT", true, INDIGO_OK_STATE));
	CHECK(requests(0, "MOUNT_SET_HOST_TIME") == n + 1);
}

static void configuration(void) {
	CHECK(num(AGENT, "AGENT_MOUNT_FOV", "ANGLE", 12.5));
	CHECK(num(AGENT, "AGENT_MOUNT_FOV", "WIDTH", 2.5));
	CHECK(num(AGENT, "AGENT_MOUNT_FOV", "HEIGHT", 1.5));
	CHECK(num(AGENT, "AGENT_LX200_CONFIGURATION", "EPOCH", 2000));
	CHECK(num(AGENT, "AGENT_LX200_CONFIGURATION", "EPOCH", 1950));
	CHECK(value(AGENT, "AGENT_LX200_CONFIGURATION", "EPOCH") == 0);
	CHECK(sw(AGENT, FEATURES, "ENABLE_JOYSTICK_CONTROL", true, INDIGO_OK_STATE));
	CHECK(indigo_agent_mount(INDIGO_DRIVER_SHUTDOWN, NULL) == INDIGO_OK);
	agent_started = false;
	CHECK(indigo_agent_mount(INDIGO_DRIVER_INIT, NULL) == INDIGO_OK);
	agent_started = true;
	CHECK(value(AGENT, "AGENT_MOUNT_FOV", "WIDTH") == 2.5);
	CHECK(value(AGENT, FEATURES, "ENABLE_JOYSTICK_CONTROL") == 1);
	CHECK(sw(AGENT, START, "RESET", true, INDIGO_OK_STATE));
	CHECK(value(AGENT, FEATURES, "ENABLE_JOYSTICK_CONTROL") == 0);
	CHECK(value(AGENT, "AGENT_LIMITS", "HA_TRACKING") == 24);
}

static void coupled_motion(void) {
	CHECK(select_peer(0, true, false));
	CHECK(select_peer(1, true, false));
	CHECK(select_peer(2, false, false));
	CHECK(sw(AGENT, FEATURES, "ENABLE_DOME_SLAVING", true, INDIGO_OK_STATE));
	CHECK(sw(AGENT, FEATURES, "ENABLE_FIELD_DEROTATION", true, INDIGO_OK_STATE));
	CHECK(num(AGENT, TARGET, "RA", 7));
	CHECK(num(AGENT, TARGET, "DEC", 20));
	int m = requests(0, "MOUNT_EQUATORIAL_COORDINATES"), d = requests(1, "DOME_HORIZONTAL_COORDINATES"), r = requests(2, "ROTATOR_POSITION");
	CHECK(sw(AGENT, START, "SLEW", true, INDIGO_BUSY_STATE));
	CHECK(wait_request(0, "MOUNT_EQUATORIAL_COORDINATES", m));
	CHECK(wait_request(1, "DOME_HORIZONTAL_COORDINATES", d));
	CHECK(wait_request(2, "ROTATOR_POSITION", r));
	complete(0, "MOUNT_EQUATORIAL_COORDINATES", INDIGO_OK_STATE);
	complete(1, "DOME_HORIZONTAL_COORDINATES", INDIGO_OK_STATE);
	CHECK(state(AGENT, START) == INDIGO_BUSY_STATE);
	complete(2, "ROTATOR_POSITION", INDIGO_ALERT_STATE);
	CHECK(wait_state(AGENT, START, 0, INDIGO_ALERT_STATE));
	CHECK(value(AGENT, "AGENT_MOUNT_STATE", "FIELD_DEROTATION") == INDIGO_ALERT_STATE);
	CHECK(value(AGENT, "AGENT_MOUNT_STATE", "DOME_SLAVING") != INDIGO_ALERT_STATE);
	complete(2, "ROTATOR_POSITION", INDIGO_OK_STATE);
	CHECK(sw(AGENT, FEATURES, "ENABLE_FIELD_DEROTATION", false, INDIGO_OK_STATE));
	m = requests(0, "MOUNT_PARK");
	d = requests(1, "DOME_PARK");
	CHECK(sw(AGENT, START, "PARK", true, INDIGO_BUSY_STATE));
	CHECK(wait_request(0, "MOUNT_PARK", m));
	CHECK(wait_request(1, "DOME_PARK", d));
	complete(0, "MOUNT_PARK", INDIGO_OK_STATE);
	CHECK(state(AGENT, START) == INDIGO_BUSY_STATE);
	complete(1, "DOME_PARK", INDIGO_ALERT_STATE);
	CHECK(wait_state(AGENT, START, 0, INDIGO_ALERT_STATE));
	CHECK(value(AGENT, "AGENT_MOUNT_STATE", "DOME_SLAVING") == INDIGO_ALERT_STATE);
}

static void slaving(void) {
	CHECK(select_peer(0, true, false));
	CHECK(select_peer(1, true, false));
	CHECK(select_peer(2, false, false));
	CHECK(operation("TRACK_ON", 0, "MOUNT_TRACKING", INDIGO_OK_STATE));
	CHECK(sw(AGENT, FEATURES, "ENABLE_DOME_SLAVING", true, INDIGO_OK_STATE));
	CHECK(sw(AGENT, FEATURES, "ENABLE_FIELD_DEROTATION", true, INDIGO_OK_STATE));
	emit_number(0, "MOUNT_EQUATORIAL_COORDINATES", "RA", 6);
	CHECK(requests(1, "DOME_HORIZONTAL_COORDINATES") > 0);
	CHECK(requests(2, "ROTATOR_POSITION") > 0);
	complete(1, "DOME_HORIZONTAL_COORDINATES", INDIGO_ALERT_STATE);
	complete(2, "ROTATOR_POSITION", INDIGO_ALERT_STATE);
	int d = requests(1, "DOME_HORIZONTAL_COORDINATES"), r = requests(2, "ROTATOR_POSITION");
	emit_number(0, "MOUNT_LST_TIME", "TIME", 1);
	CHECK(value(AGENT, "AGENT_MOUNT_STATE", "DOME_SLAVING") == INDIGO_ALERT_STATE);
	CHECK(value(AGENT, "AGENT_MOUNT_STATE", "FIELD_DEROTATION") == INDIGO_ALERT_STATE);
	CHECK(requests(1, "DOME_HORIZONTAL_COORDINATES") == d);
	CHECK(requests(2, "ROTATOR_POSITION") == r);
	CHECK(sw(AGENT, lists[2], "NONE", true, INDIGO_OK_STATE));
	CHECK(value(AGENT, "AGENT_MOUNT_STATE", "FIELD_DEROTATION") == INDIGO_IDLE_STATE);
	CHECK(sw(AGENT, lists[0], "NONE", true, INDIGO_OK_STATE));
	CHECK(value(AGENT, "AGENT_MOUNT_STATE", "DOME_SLAVING") == INDIGO_IDLE_STATE);
}

static void joystick(void) {
	CHECK(select_peer(0, true, false));
	CHECK(select_peer(4, false, false));
	const char *names[] = { "MOUNT_PARK", "MOUNT_TRACKING", "MOUNT_HOME", "MOUNT_SLEW_RATE", "MOUNT_MOTION_RA", "MOUNT_MOTION_DEC", "MOUNT_ABORT_MOTION" };
	for (int i = 0; i < ARRAY_SIZE(names); i++) {
		int n = requests(0, names[i]);
		complete(4, names[i], INDIGO_OK_STATE);
		CHECK(requests(0, names[i]) == n);
	}
	CHECK(sw(AGENT, FEATURES, "ENABLE_JOYSTICK_CONTROL", true, INDIGO_OK_STATE));
	immediate = true;
	for (int i = 0; i < ARRAY_SIZE(names); i++) {
		indigo_property *p = prop(4, names[i]);
		for (int j = 0; j < p->count; j++) {
			fprintf(stderr, "  joystick %s.%s\n", names[i], p->items[j].name);
			indigo_set_switch(p, p->items + j, true);
			int n = requests(0, names[i]);
			complete(4, names[i], INDIGO_OK_STATE);
			CHECK(wait_request(0, names[i], n));
			CHECK(wait_state(AGENT, START, 0, -1));
		}
	}
}

static void lx200_protocol(void) {
	CHECK(num(AGENT, "AGENT_LX200_CONFIGURATION", "EPOCH", 2000));
	CHECK(start_server());
	exchange("#:GVP#:Sr05:15:30#:Sd-12*30:00#:GR#:GD#");
	fprintf(stderr, "LX reply: %s\n", lx_output);
	CHECK(!strcmp(lx_output, "indigo#1100:00:00#+00*00'00#"));
	exchange(":SrBAD#:SdBAD#:Sw1#:Sw2#:Sw3#:Sw4#:SC09/10/26#:SG-2#");
	CHECK(!strcmp(lx_output, "0011111Updating        planetary data. #                                #1"));
	exchange(":unknown#");
	CHECK(!*lx_output);
	CHECK(select_peer(0, false, false));
	CHECK(num(AGENT, "AGENT_LX200_CONFIGURATION", "EPOCH", 2000));
	exchange(":Sr05:15:30#:Sd-12*30:00#:MS#");
	CHECK(!strcmp(lx_output, "110"));
	CHECK(wait_request(0, "MOUNT_EQUATORIAL_COORDINATES", 0));
	CHECK(fabs(value(peer_names[0], "MOUNT_EQUATORIAL_COORDINATES", "RA") - 5.258333333333) < 1e-8);
	CHECK(value(peer_names[0], "MOUNT_EQUATORIAL_COORDINATES", "DEC") == -12.5);
	complete(0, "MOUNT_EQUATORIAL_COORDINATES", INDIGO_OK_STATE);
	CHECK(wait_state(AGENT, START, 0, INDIGO_OK_STATE));
	exchange(":Sr06:30#:Sd+20*15#:CM#");
	CHECK(!strcmp(lx_output, "11OK#"));
	CHECK(wait_request(0, "MOUNT_EQUATORIAL_COORDINATES", 1));
	CHECK(value(peer_names[0], "MOUNT_ON_COORDINATES_SET", "SYNC") == 1);
	complete(0, "MOUNT_EQUATORIAL_COORDINATES", INDIGO_OK_STATE);
	CHECK(wait_state(AGENT, START, 0, INDIGO_OK_STATE));
	immediate = true;
	const char *rates[] = { ":RG#", ":RC#", ":RM#", ":RS#" };
	const char *items[] = { "GUIDE", "CENTERING", "FIND", "MAX" };
	for (int i = 0; i < 4; i++) {
		exchange(rates[i]);
		CHECK(value(peer_names[0], "MOUNT_SLEW_RATE", items[i]) == 1);
	}
	const char *moves[] = { ":Mn#", ":Ms#", ":Mw#", ":Me#" };
	const char *stops[] = { ":Qn#", ":Qs#", ":Qw#", ":Qe#" };
	const char *directions[] = { "NORTH", "SOUTH", "WEST", "EAST" };
	for (int i = 0; i < 4; i++) {
		const char *p = i < 2 ? "MOUNT_MOTION_DEC" : "MOUNT_MOTION_RA";
		exchange(moves[i]);
		CHECK(value(peer_names[0], p, directions[i]) == 1);
		exchange(stops[i]);
		CHECK(value(peer_names[0], p, directions[i]) == 0);
	}
	int n = requests(0, "MOUNT_ABORT_MOTION");
	exchange(":Q#");
	CHECK(requests(0, "MOUNT_ABORT_MOTION") == n + 1);
	lx_read_error = -1;
	exchange(":unfinished");
	CHECK(!*lx_output);
	CHECK(sw(AGENT, "AGENT_LX200_SERVER", "STOPPED", true, INDIGO_OK_STATE));
	CHECK(!server_open);
}
static void dome_reselection(void) {
	CHECK(select_peer(1, false, false));
	CHECK(operation("DOME_OPEN", 1, "DOME_SHUTTER", INDIGO_OK_STATE));
	CHECK(sw(AGENT, lists[1], "NONE", true, INDIGO_OK_STATE));
	CHECK(value(AGENT, "AGENT_DOME_STATE", "OPEN") == INDIGO_IDLE_STATE);
	CHECK(select_peer(1, false, false));
	CHECK(operation("DOME_CLOSE", 1, "DOME_SHUTTER", INDIGO_OK_STATE));
	CHECK(operation("DOME_PARK", 1, "DOME_PARK", INDIGO_OK_STATE));
}

static void dome_reselection_legacy(void) {
	CHECK(select_peer(1, false, false));
	CHECK(sw(AGENT, lists[1], "NONE", true, INDIGO_OK_STATE));
	CHECK(select_peer(1, false, false));
	CHECK(operation("DOME_PARK", 1, "DOME_PARK", INDIGO_OK_STATE));
}

static void feature_persistence(void) {
	CHECK(sw(AGENT, FEATURES, "ENABLE_DOME_SLAVING", true, INDIGO_OK_STATE));
	CHECK(sw(AGENT, FEATURES, "ENABLE_FIELD_DEROTATION", true, INDIGO_OK_STATE));
	CHECK(num(AGENT, "AGENT_MOUNT_FOV", "WIDTH", 4));
	unsigned rev = revision(AGENT, FEATURES);
	indigo_enumerate_properties(&client, &INDIGO_ALL_PROPERTIES);
	CHECK(wait_state(AGENT, FEATURES, rev, INDIGO_OK_STATE));
	CHECK(value(AGENT, FEATURES, "ENABLE_DOME_SLAVING") == 1);
	CHECK(value(AGENT, FEATURES, "ENABLE_FIELD_DEROTATION") == 1);
	CHECK(indigo_agent_mount(INDIGO_DRIVER_SHUTDOWN, NULL) == INDIGO_OK);
	agent_started = false;
	CHECK(indigo_agent_mount(INDIGO_DRIVER_INIT, NULL) == INDIGO_OK);
	agent_started = true;
	CHECK(value(AGENT, FEATURES, "ENABLE_DOME_SLAVING") == 0);
	CHECK(value(AGENT, FEATURES, "ENABLE_FIELD_DEROTATION") == 0);
}

static void persistent_features(void) {
	CHECK(sw(AGENT, FEATURES, "MAKE_DOME_SLAVING_PERSISTENT", true, INDIGO_OK_STATE));
	CHECK(sw(AGENT, FEATURES, "MAKE_FIELD_DEROTATION_PERSISTENT", true, INDIGO_OK_STATE));
	CHECK(sw(AGENT, FEATURES, "ENABLE_DOME_SLAVING", true, INDIGO_OK_STATE));
	CHECK(sw(AGENT, FEATURES, "ENABLE_FIELD_DEROTATION", true, INDIGO_OK_STATE));
	CHECK(indigo_agent_mount(INDIGO_DRIVER_SHUTDOWN, NULL) == INDIGO_OK);
	agent_started = false;
	CHECK(indigo_agent_mount(INDIGO_DRIVER_INIT, NULL) == INDIGO_OK);
	agent_started = true;
	CHECK(value(AGENT, FEATURES, "ENABLE_DOME_SLAVING") == 1);
	CHECK(value(AGENT, FEATURES, "ENABLE_FIELD_DEROTATION") == 1);
}

static void lx200_ack(void) {
	CHECK(start_server());
	exchange("\006");
	CHECK(!strcmp(lx_output, "P"));
}

static void lx200_positive_zero(void) {
	CHECK(start_server());
	CHECK(num(AGENT, "AGENT_LX200_CONFIGURATION", "EPOCH", 2000));
	exchange(":Sr05:00#:Sd+00*30:00#:MS#");
	CHECK(!strcmp(lx_output, "110"));
	indigo_property *p = snapshot(AGENT, TARGET);
	CHECK(p);
	double dec = indigo_get_item(p, "DEC")->number.target;
	indigo_release_property(p);
	CHECK(dec == 0.5);
}

static void lx200_truncated_command(void) {
	CHECK(start_server());
	exchange(":Sr05:30");
	CHECK(!*lx_output);
}

static void lx200_bind_failure(void) {
	server_fail = true;
	unsigned rev = revision(AGENT, "AGENT_LX200_SERVER");
	indigo_change_switch_property_1(&client, AGENT, "AGENT_LX200_SERVER", "STARTED", true);
	CHECK(wait_state(AGENT, "AGENT_LX200_SERVER", rev, INDIGO_ALERT_STATE));
}

static void lx200_idle_stop(void) {
	CHECK(sw(AGENT, "AGENT_LX200_SERVER", "STOPPED", true, INDIGO_OK_STATE));
}

static char *header_value(int index, const char *keyword) {
	static char result[INDIGO_VALUE_SIZE];
	result[0] = 0;
	pthread_mutex_lock(&peer_mutex);
	for (int i = 0; i < command_count; i++) {
		indigo_property *p = commands[i].property;
		if (commands[i].peer == index && !strcmp(p->name, "CCD_SET_FITS_HEADER")) {
			indigo_item *key = indigo_get_item(p, "KEYWORD");
			indigo_item *val = indigo_get_item(p, "VALUE");
			if (key && val && !strcmp(indigo_get_text_item_value(key), keyword)) {
				snprintf(result, sizeof(result), "%s", indigo_get_text_item_value(val));
			}
		}
	}
	pthread_mutex_unlock(&peer_mutex);
	return result;
}

static void related_agents(void) {
	CHECK(select_peer(0, false, false));
	CHECK(select_peer(5, false, false));
	CHECK(select_peer(6, false, false));
	emit_number(0, "MOUNT_EQUATORIAL_COORDINATES", "RA", 8.25);
	emit_number(0, "MOUNT_EQUATORIAL_COORDINATES", "DEC", 20.5);
	CHECK(value(peer_names[6], "AGENT_GUIDER_MOUNT_COORDINATES", "RA") == 8.25);
	CHECK(value(peer_names[6], "AGENT_GUIDER_MOUNT_COORDINATES", "DEC") == 20.5);
	CHECK(!strcmp(header_value(5, "OBJCTRA"), "'8 15 00'"));
	CHECK(!strcmp(header_value(5, "OBJCTDEC"), "'20 30 00'"));
	CHECK(!strcmp(header_value(6, "OBJCTDEC"), "'20 30 00'"));
	for (int k = 0; k < 2; k++) {
		CHECK(sw(AGENT, "ABORT_RELATED_PROCESS", "IMAGER", k, INDIGO_OK_STATE));
		CHECK(sw(AGENT, "ABORT_RELATED_PROCESS", "GUIDER", k, INDIGO_OK_STATE));
		int im = requests(5, "AGENT_ABORT_PROCESS"), gu = requests(6, "AGENT_ABORT_PROCESS");
		CHECK(operation("SLEW", 0, "MOUNT_EQUATORIAL_COORDINATES", INDIGO_OK_STATE));
		CHECK(requests(5, "AGENT_ABORT_PROCESS") == im + k);
		CHECK(requests(6, "AGENT_ABORT_PROCESS") == gu + k);
		CHECK(requests(5, "AGENT_START_PROCESS") > 0);
		CHECK(requests(6, "AGENT_START_PROCESS") > 0);
	}
	CHECK(num(AGENT, "GEOGRAPHIC_COORDINATES", "LATITUDE", 48.5));
	CHECK(!strcmp(header_value(5, "SITELAT"), "'48 30 00'"));
	CHECK(sw(peer_names[0], "MOUNT_SIDE_OF_PIER", "EAST", true, INDIGO_OK_STATE));
	CHECK(value(peer_names[6], "AGENT_GUIDER_MOUNT_COORDINATES", "SIDE_OF_PIER") == -1);
	CHECK(!strcmp(header_value(6, "PIERSIDE"), "0"));
}

static void negative_fits(void) {
	CHECK(select_peer(0, false, false));
	CHECK(select_peer(5, false, false));
	emit_number(0, "MOUNT_EQUATORIAL_COORDINATES", "DEC", -12.5);
	CHECK(!strcmp(header_value(5, "OBJCTDEC"), "'-12 30 00'"));
}

static void negative_zero_fits(void) {
	CHECK(select_peer(0, false, false));
	CHECK(select_peer(6, false, false));
	emit_number(0, "MOUNT_EQUATORIAL_COORDINATES", "DEC", -0.5);
	CHECK(!strcmp(header_value(6, "OBJCTDEC"), "'-0 30 00'"));
}

static void limits(void) {
	CHECK(select_peer(0, true, false));
	CHECK(num(AGENT, "GEOGRAPHIC_COORDINATES", "LONGITUDE", 10));
	CHECK(sw(AGENT, FEATURES, "ENABLE_HA_LIMIT", true, INDIGO_OK_STATE));
	const double hours[] = { 2, 23, 1, 22 };
	const double bounds[] = { 1, 1, 23, 23 };
	const bool parked[] = { true, false, true, false };
	for (int i = 0; i < 4; i++) {
		CHECK(num(AGENT, "AGENT_LIMITS", "HA_TRACKING", bounds[i]));
		time_t utc = time(NULL);
		double ra = fmod(indigo_lst(&utc, 10) - hours[i] + 24, 24), dec = 20;
		indigo_jnow_to_j2k(&ra, &dec);
		int n = requests(0, "MOUNT_PARK");
		emit_number(0, "MOUNT_EQUATORIAL_COORDINATES", "RA", ra);
		CHECK((requests(0, "MOUNT_PARK") > n) == parked[i]);
		if (parked[i]) {
			complete(0, "MOUNT_PARK", INDIGO_OK_STATE);
			CHECK(operation("UNPARK", 0, "MOUNT_PARK", INDIGO_OK_STATE));
		}
	}
	CHECK(sw(AGENT, FEATURES, "ENABLE_HA_LIMIT", false, INDIGO_OK_STATE));
	CHECK(sw(AGENT, FEATURES, "ENABLE_TIME_LIMIT", true, INDIGO_OK_STATE));
	time_t utc = time(NULL);
	struct tm now = *localtime(&utc);
	double hour = now.tm_hour + now.tm_min / 60.0 + now.tm_sec / 3600.0;
	CHECK(num(AGENT, "AGENT_LIMITS", "LOCAL_TIME", hour + 0.01));
	int n = requests(0, "MOUNT_PARK");
	emit_number(0, "MOUNT_LST_TIME", "TIME", 1);
	CHECK(requests(0, "MOUNT_PARK") == n);
	CHECK(num(AGENT, "AGENT_LIMITS", "LOCAL_TIME", hour - 0.01));
	emit_number(0, "MOUNT_LST_TIME", "TIME", 2);
	CHECK(requests(0, "MOUNT_PARK") > n);
}

static void instances(void) {
	CHECK(num(AGENT, "ADDITIONAL_INSTANCES", "COUNT", 1));
	CHECK(wait_state("Mount Agent #2", START, 0, INDIGO_OK_STATE));
	CHECK(num("Mount Agent #2", TARGET, "RA", 4));
	indigo_property *p = snapshot(AGENT, TARGET);
	CHECK(p && p->items[0].number.target == 0);
	indigo_release_property(p);
	CHECK(num(AGENT, "ADDITIONAL_INSTANCES", "COUNT", 0));
	CHECK(!snapshot("Mount Agent #2", START));
}

static void stale_capability(void) {
	CHECK(select_peer(0, true, false));
	CHECK(value(AGENT, AGENT_MOUNT_FEATURES_PROPERTY_NAME, "HOME") == 1);
	indigo_delete_property(&peers[0].device, prop(0, "MOUNT_HOME"), NULL);
	CHECK(value(AGENT, AGENT_MOUNT_FEATURES_PROPERTY_NAME, "HOME") == 0);
}

static void process_deselection(void) {
	CHECK(select_peer(0, true, false));
	CHECK(sw(AGENT, START, "SLEW", true, INDIGO_BUSY_STATE));
	CHECK(wait_request(0, "MOUNT_EQUATORIAL_COORDINATES", 0));
	CHECK(sw(peer_names[0], "CONNECTION", "DISCONNECTED", true, INDIGO_OK_STATE));
	CHECK(wait_state(AGENT, START, 0, INDIGO_ALERT_STATE));
	CHECK(select_peer(0, true, false));
	CHECK(operation("SLEW", 0, "MOUNT_EQUATORIAL_COORDINATES", INDIGO_OK_STATE));
}

static atomic_bool fast_wait;
static atomic_uint accelerated_waits;

void mount_test_sleep(long delay) {
	if (fast_wait) {
		accelerated_waits++;
	} else {
		indigo_usleep(delay);
	}
}

static void operation_timeouts(void) {
	CHECK(select_peer(0, true, false));
	CHECK(select_peer(1, true, false));
	for (int i = 0; i < ARRAY_SIZE(operations); i++) {
		fast_wait = false;
		int n = requests(operations[i].peer, operations[i].property);
		CHECK(sw(AGENT, START, operations[i].item, true, INDIGO_BUSY_STATE));
		CHECK(wait_request(operations[i].peer, operations[i].property, n));
		unsigned before = accelerated_waits;
		fast_wait = true;
		CHECK(wait_state(AGENT, START, 0, INDIGO_ALERT_STATE));
		CHECK(accelerated_waits - before > 100000);
		fast_wait = false;
		complete(operations[i].peer, operations[i].property, INDIGO_ALERT_STATE);
	}
	CHECK(operation("SLEW", 0, "MOUNT_EQUATORIAL_COORDINATES", INDIGO_OK_STATE));
}

static void coupled_timeouts(void) {
	CHECK(select_peer(0, true, false));
	CHECK(select_peer(1, true, false));
	CHECK(select_peer(2, false, false));
	CHECK(sw(AGENT, FEATURES, "ENABLE_DOME_SLAVING", true, INDIGO_OK_STATE));
	CHECK(sw(AGENT, FEATURES, "ENABLE_FIELD_DEROTATION", true, INDIGO_OK_STATE));
	CHECK(num(AGENT, TARGET, "RA", 7));
	CHECK(num(AGENT, TARGET, "DEC", 30));
	CHECK(sw(AGENT, START, "SLEW", true, INDIGO_BUSY_STATE));
	CHECK(wait_request(2, "ROTATOR_POSITION", 0));
	fast_wait = true;
	CHECK(wait_state(AGENT, START, 0, INDIGO_ALERT_STATE));
	fast_wait = false;
	CHECK(value(AGENT, "AGENT_MOUNT_STATE", "DOME_SLAVING") == INDIGO_ALERT_STATE);
	CHECK(value(AGENT, "AGENT_MOUNT_STATE", "FIELD_DEROTATION") == INDIGO_ALERT_STATE);
}

static void immediate_completion(void) {
	CHECK(select_peer(0, true, false));
	immediate = true;
	fast_wait = true;
	CHECK(sw(AGENT, START, "SYNC", true, INDIGO_OK_STATE));
	CHECK(accelerated_waits >= 3000);
	CHECK(requests(0, "MOUNT_EQUATORIAL_COORDINATES") == 1);
	fast_wait = false;
}

static void coupled_abort(void) {
	CHECK(select_peer(0, true, false));
	CHECK(select_peer(1, true, false));
	CHECK(select_peer(2, false, false));
	CHECK(sw(AGENT, FEATURES, "ENABLE_DOME_SLAVING", true, INDIGO_OK_STATE));
	CHECK(sw(AGENT, FEATURES, "ENABLE_FIELD_DEROTATION", true, INDIGO_OK_STATE));
	const char *items[] = { "PARK", "UNPARK", "SLEW" };
	for (int i = 0; i < 3; i++) {
		const char *target = i == 2 ? "MOUNT_EQUATORIAL_COORDINATES" : "MOUNT_PARK";
		int n = requests(0, target);
		int a[] = { requests(0, "MOUNT_ABORT_MOTION"), requests(1, "DOME_ABORT_MOTION"), requests(2, "ROTATOR_ABORT_MOTION") };
		CHECK(sw(AGENT, START, items[i], true, INDIGO_BUSY_STATE));
		CHECK(wait_request(0, target, n));
		CHECK(sw(AGENT, "AGENT_ABORT_PROCESS", "ABORT", true, INDIGO_OK_STATE));
		CHECK(wait_state(AGENT, START, 0, INDIGO_ALERT_STATE));
		CHECK(requests(0, "MOUNT_ABORT_MOTION") > a[0]);
		CHECK(requests(1, "DOME_ABORT_MOTION") > a[1]);
		CHECK(requests(2, "ROTATOR_ABORT_MOTION") > a[2]);
		CHECK(value(AGENT, "AGENT_MOUNT_STATE", "DOME_SLAVING") == INDIGO_IDLE_STATE);
	}
}

static void shutdown_active(void) {
	CHECK(select_peer(0, true, false));
	CHECK(sw(AGENT, START, "SLEW", true, INDIGO_BUSY_STATE));
	CHECK(wait_request(0, "MOUNT_EQUATORIAL_COORDINATES", 0));
	// A stuck detach must not stall the suite or leave threads in the parent.
	alarm(5);
	CHECK(indigo_agent_mount(INDIGO_DRIVER_SHUTDOWN, NULL) == INDIGO_OK);
	agent_started = false;
	alarm(45);
}

static void shutdown_server(void) {
	CHECK(start_server());
	alarm(5);
	CHECK(indigo_agent_mount(INDIGO_DRIVER_SHUTDOWN, NULL) == INDIGO_OK);
	agent_started = false;
	alarm(45);
	CHECK(!server_open);
}

static void all_false_start(void) {
	CHECK(sw(AGENT, START, "SLEW", false, INDIGO_OK_STATE));
}

static void home_momentary(void) {
	CHECK(select_peer(0, false, false));
	CHECK(sw(AGENT, START, "HOME", true, INDIGO_BUSY_STATE));
	CHECK(wait_request(0, "MOUNT_HOME", 0));
	prop(0, "MOUNT_HOME")->items[0].sw.value = false;
	complete(0, "MOUNT_HOME", INDIGO_OK_STATE);
	CHECK(wait_state(AGENT, START, 0, INDIGO_OK_STATE));
	CHECK(value(AGENT, "AGENT_MOUNT_STATE", "HOME") == INDIGO_OK_STATE);
}

static void state_lights(void) {
	CHECK(select_peer(0, true, false));
	CHECK(select_peer(1, true, false));
	for (int index = 0; index < 2; index++) {
		indigo_property *p = prop(index, index == 0 ? "MOUNT_STATE" : "DOME_STATE");
		const char *name = index == 0 ? "AGENT_MOUNT_STATE" : "AGENT_DOME_STATE";
		for (int i = 0; i < p->count; i++) {
			for (int st = INDIGO_IDLE_STATE; st <= INDIGO_ALERT_STATE; st++) {
				p->items[i].light.value = st;
				complete(index, p->name, INDIGO_OK_STATE);
				CHECK(value(AGENT, name, p->items[i].name) == st);
			}
		}
	}
}

static void selection_orders(void) {
	CHECK(sw(AGENT, FEATURES, "ENABLE_DOME_SLAVING", true, INDIGO_OK_STATE));
	CHECK(sw(AGENT, FEATURES, "ENABLE_FIELD_DEROTATION", true, INDIGO_OK_STATE));
	CHECK(select_peer(1, true, false));
	CHECK(select_peer(2, false, false));
	CHECK(select_peer(0, true, false));
	CHECK(sw(AGENT, lists[1], "NONE", true, INDIGO_OK_STATE));
	CHECK(select_peer(1, true, false));
	CHECK(sw(AGENT, lists[2], "NONE", true, INDIGO_OK_STATE));
	CHECK(select_peer(2, false, false));
	CHECK(select_peer(3, false, false));
	CHECK(sw(AGENT, lists[3], "NONE", true, INDIGO_OK_STATE));
	CHECK(sw(AGENT, lists[0], "NONE", true, INDIGO_OK_STATE));
	CHECK(value(AGENT, "AGENT_MOUNT_STATE", "DOME_SLAVING") == INDIGO_IDLE_STATE);
	CHECK(value(AGENT, "AGENT_MOUNT_STATE", "FIELD_DEROTATION") == INDIGO_IDLE_STATE);
}

static void immediate_rejection(void) {
	CHECK(select_peer(0, false, false));
	CHECK(select_peer(1, false, false));
	reject_motion = true;
	fast_wait = true;
	for (int i = 0; i < ARRAY_SIZE(operations); i++) {
		unsigned n = accelerated_waits;
		CHECK(sw(AGENT, START, operations[i].item, true, INDIGO_ALERT_STATE));
		CHECK(accelerated_waits - n < 10000);
	}
	fast_wait = false;
}

static void lx200_input_matrix(void) {
	CHECK(start_server());
	CHECK(num(AGENT, "AGENT_LX200_CONFIGURATION", "EPOCH", 2000));
	const char *inputs[] = { ":Sr12:30:15#:Sd+45*20:30#:MS#", ":Sr23:59#:Sd-00*30#:MS#", ":Sr00:00#:Sd-90*00:00#:CM#" };
	const double ra[] = { 12.5041666667, 23.9833333333, 0 };
	const double dec[] = { 45.3416666667, -0.5, -90 };
	for (int i = 0; i < 3; i++) {
		exchange(inputs[i]);
		indigo_property *p = snapshot(AGENT, TARGET);
		CHECK(p);
		CHECK(fabs(p->items[0].number.target - ra[i]) < 1e-8);
		CHECK(fabs(p->items[1].number.target - dec[i]) < 1e-8);
		indigo_release_property(p);
	}
	char oversized[512];
	memset(oversized, 'x', sizeof(oversized));
	oversized[0] = ':';
	oversized[sizeof(oversized) - 2] = '#';
	oversized[sizeof(oversized) - 1] = 0;
	exchange(oversized);
	CHECK(!*lx_output);
	exchange(":GVP#");
	CHECK(!strcmp(lx_output, "indigo#"));
}

static void unpark_failure(void) {
	CHECK(select_peer(0, true, false));
	CHECK(select_peer(1, true, false));
	CHECK(operation("PARK", 0, "MOUNT_PARK", INDIGO_OK_STATE));
	CHECK(operation("DOME_PARK", 1, "DOME_PARK", INDIGO_OK_STATE));
	CHECK(sw(AGENT, FEATURES, "ENABLE_DOME_SLAVING", true, INDIGO_OK_STATE));
	reject_motion = true;
	fast_wait = true;
	CHECK(sw(AGENT, START, "SLEW", true, INDIGO_ALERT_STATE));
	CHECK(requests(0, "MOUNT_EQUATORIAL_COORDINATES") == 0);
	CHECK(requests(1, "DOME_HORIZONTAL_COORDINATES") == 0);
}

static void legacy_dome_slew(void) {
	CHECK(select_peer(1, false, false));
	complete(1, "DOME_HORIZONTAL_COORDINATES", INDIGO_BUSY_STATE);
	CHECK(value(AGENT, "AGENT_DOME_STATE", "SLEW") == INDIGO_BUSY_STATE);
	complete(1, "DOME_HORIZONTAL_COORDINATES", INDIGO_ALERT_STATE);
	CHECK(value(AGENT, "AGENT_DOME_STATE", "SLEW") == INDIGO_ALERT_STATE);
	complete(1, "DOME_HORIZONTAL_COORDINATES", INDIGO_OK_STATE);
	CHECK(value(AGENT, "AGENT_DOME_STATE", "SLEW") == INDIGO_IDLE_STATE);
}

static void geometry_and_threshold(void) {
	CHECK(select_peer(0, true, false));
	CHECK(select_peer(1, true, false));
	CHECK(select_peer(2, false, false));
	CHECK(num(AGENT, "GEOGRAPHIC_COORDINATES", "LATITUDE", 45));
	CHECK(num(AGENT, "GEOGRAPHIC_COORDINATES", "LONGITUDE", 0));
	CHECK(operation("TRACK_ON", 0, "MOUNT_TRACKING", INDIGO_OK_STATE));
	time_t utc = time(NULL);
	double ra = indigo_lst(&utc, 0), dec = 0;
	indigo_jnow_to_j2k(&ra, &dec);
	emit_number(0, "MOUNT_EQUATORIAL_COORDINATES", "DEC", dec);
	emit_number(0, "MOUNT_EQUATORIAL_COORDINATES", "RA", ra);
	CHECK(sw(AGENT, FEATURES, "ENABLE_DOME_SLAVING", true, INDIGO_OK_STATE));
	emit_number(0, "MOUNT_LST_TIME", "TIME", 1);
	CHECK(fabs(value(peer_names[1], "DOME_HORIZONTAL_COORDINATES", "AZ") - 180) < 0.05);
	complete(1, "DOME_HORIZONTAL_COORDINATES", INDIGO_OK_STATE);
	int n = requests(1, "DOME_HORIZONTAL_COORDINATES");
	emit_number(0, "MOUNT_LST_TIME", "TIME", 2);
	CHECK(requests(1, "DOME_HORIZONTAL_COORDINATES") == n);
	CHECK(num(AGENT, "DOME_SLAVING_PARAMETERS", "MOVE_THRESHOLD", 360));
	emit_number(0, "MOUNT_EQUATORIAL_COORDINATES", "RA", ra - 1);
	CHECK(requests(1, "DOME_HORIZONTAL_COORDINATES") == n);
	CHECK(sw(AGENT, FEATURES, "ENABLE_FIELD_DEROTATION", true, INDIGO_OK_STATE));
	CHECK(value(AGENT, "AGENT_MOUNT_STATE", "FIELD_DEROTATION") == INDIGO_IDLE_STATE);
	emit_number(0, "MOUNT_EQUATORIAL_COORDINATES", "RA", ra + 1);
	CHECK(requests(2, "ROTATOR_POSITION") > 0);
	double angle = value(peer_names[2], "ROTATOR_POSITION", "POSITION");
	CHECK(angle >= 0 && angle < 360);
	complete(2, "ROTATOR_POSITION", INDIGO_OK_STATE);
	CHECK(sw(AGENT, FEATURES, "ENABLE_FIELD_DEROTATION", false, INDIGO_OK_STATE));
	CHECK(sw(AGENT, FEATURES, "ENABLE_DOME_SLAVING", false, INDIGO_OK_STATE));
	CHECK(value(AGENT, "AGENT_MOUNT_STATE", "FIELD_DEROTATION") == INDIGO_IDLE_STATE);
	CHECK(value(AGENT, "AGENT_MOUNT_STATE", "DOME_SLAVING") == INDIGO_IDLE_STATE);
}

static void related_solver_filters(void) {
	peer_names[5] = "Astrometry Agent Test";
	peer_names[6] = "ASTAP Agent Test";
	CHECK(select_peer(5, false, false));
	CHECK(select_peer(6, false, false));
}

static void related_invalid_filter(void) {
	peer_names[5] = "Solver Agent Test";
	peer_names[6] = "Unrelated Agent Test";
	CHECK(select_peer(5, false, false));
	CHECK(attach_peer(6, false, false));
	CHECK(isnan(value(AGENT, "FILTER_RELATED_AGENT_LIST", peer_names[6])));
}

static void negative_site_fits(void) {
	CHECK(select_peer(5, false, false));
	CHECK(num(AGENT, "GEOGRAPHIC_COORDINATES", "LATITUDE", -0.5));
	CHECK(!strcmp(header_value(5, "SITELAT"), "'-0 30 00'"));
}

static void lx200_invalid_coordinates(void) {
	CHECK(start_server());
	exchange(":Sr25:00:00#:Sd+91*00:00#:Sr12:75:00#:Sd+20*75:00#");
	CHECK(!strcmp(lx_output, "0000"));
}

static time_t fixed_utc;

time_t mount_test_time(time_t *result) {
	time_t now = fixed_utc ? fixed_utc : time(NULL);
	if (result) {
		*result = now;
	}
	return now;
}

static void time_limit_matrix(void) {
	setenv("TZ", "UTC", 1);
	tzset();
	CHECK(select_peer(0, true, false));
	CHECK(sw(AGENT, FEATURES, "ENABLE_TIME_LIMIT", true, INDIGO_OK_STATE));
	const int hours[] = { 8, 8, 16, 16 };
	const int bounds[] = { 9, 7, 17, 15 };
	for (int i = 0; i < 4; i++) {
		struct tm tm = { .tm_year = 126, .tm_mon = 8, .tm_mday = 10, .tm_hour = hours[i] };
		fixed_utc = mktime(&tm);
		CHECK(num(AGENT, "AGENT_LIMITS", "LOCAL_TIME", bounds[i]));
		int n = requests(0, "MOUNT_PARK");
		emit_number(0, "MOUNT_LST_TIME", "TIME", i + 1);
		CHECK((requests(0, "MOUNT_PARK") > n) == (i % 2));
		if (i % 2) {
			complete(0, "MOUNT_PARK", INDIGO_OK_STATE);
			CHECK(operation("UNPARK", 0, "MOUNT_PARK", INDIGO_OK_STATE));
		}
	}
}

static void rotator_sync_wrap(void) {
	CHECK(select_peer(0, true, false));
	CHECK(select_peer(1, true, false));
	CHECK(select_peer(2, false, false));
	CHECK(num(AGENT, "GEOGRAPHIC_COORDINATES", "LATITUDE", 45));
	CHECK(sw(AGENT, FEATURES, "ENABLE_FIELD_DEROTATION", true, INDIGO_OK_STATE));
	const double positions[] = { 0, 359 };
	for (int i = 0; i < 2; i++) {
		emit_number(2, "ROTATOR_POSITION", "POSITION", positions[i]);
		complete(2, "ROTATOR_POSITION", INDIGO_OK_STATE);
		time_t utc = time(NULL);
		double ra = fmod(indigo_lst(&utc, 0) + (i ? -3 : 3) + 24, 24);
		CHECK(num(AGENT, TARGET, "RA", ra));
		CHECK(num(AGENT, TARGET, "DEC", 0));
		int r = requests(2, "ROTATOR_POSITION"), d = requests(1, "DOME_HORIZONTAL_COORDINATES");
		CHECK(sw(AGENT, START, "SYNC", true, INDIGO_BUSY_STATE));
		CHECK(wait_request(2, "ROTATOR_POSITION", r));
		CHECK(value(peer_names[2], "ROTATOR_ON_POSITION_SET", "SYNC") == 1);
		double angle = value(peer_names[2], "ROTATOR_POSITION", "POSITION");
		CHECK(angle >= 0 && angle < 360);
		CHECK(requests(1, "DOME_HORIZONTAL_COORDINATES") == d);
		complete(0, "MOUNT_EQUATORIAL_COORDINATES", INDIGO_OK_STATE);
		complete(2, "ROTATOR_POSITION", INDIGO_OK_STATE);
		CHECK(wait_state(AGENT, START, 0, INDIGO_OK_STATE));
	}
}

static void configuration_failure(void) {
	CHECK(num(AGENT, "AGENT_MOUNT_FOV", "WIDTH", 1));
	char path[sizeof(config_folder)], filename[256], before[16384], after[16384];
	strcpy(path, config_folder);
	snprintf(filename, sizeof(filename), "%s/Mount_Agent.config", path);
	FILE *file = fopen(filename, "r");
	CHECK(file);
	size_t size = fread(before, 1, sizeof(before), file);
	fclose(file);
	// ENOTDIR is deterministic even when the test account has broad privileges.
	strcpy(config_folder, "/dev/null");
	bool accepted = num(AGENT, "AGENT_MOUNT_FOV", "WIDTH", 2);
	strcpy(config_folder, path);
	CHECK(accepted);
	CHECK(value(AGENT, "AGENT_MOUNT_FOV", "WIDTH") == 2);
	file = fopen(filename, "r");
	CHECK(file);
	size_t new_size = fread(after, 1, sizeof(after), file);
	fclose(file);
	CHECK(size == new_size && !memcmp(before, after, size));
	CHECK(num(AGENT, "AGENT_MOUNT_FOV", "WIDTH", 3));
	CHECK(indigo_agent_mount(INDIGO_DRIVER_SHUTDOWN, NULL) == INDIGO_OK);
	agent_started = false;
	CHECK(indigo_agent_mount(INDIGO_DRIVER_INIT, NULL) == INDIGO_OK);
	agent_started = true;
	CHECK(value(AGENT, "AGENT_MOUNT_FOV", "WIDTH") == 3);
}

static void cleanup(void) {
	if (server_open) {
		sw(AGENT, "AGENT_LX200_SERVER", "STOPPED", true, INDIGO_OK_STATE);
	}
	if (agent_started) {
		indigo_change_switch_property_1(&client, AGENT, "AGENT_ABORT_PROCESS", "ABORT", true);
		wait_state(AGENT, START, 0, -1);
		if (indigo_agent_mount(INDIGO_DRIVER_SHUTDOWN, NULL) != INDIGO_OK) {
			indigo_test_failures++;
		}
	}
	for (int i = 0; i < ARRAY_SIZE(peers); i++) {
		if (peers[i].attached) {
			if (i < 5 && value(peer_names[i], "CONNECTION", "CONNECTED") == 1) {
				sw(peer_names[i], "CONNECTION", "DISCONNECTED", true, INDIGO_OK_STATE);
			}
			indigo_detach_device(&peers[i].device);
		}
		for (int j = 0; j < peers[i].count; j++) {
			indigo_release_property(peers[i].properties[j]);
		}
	}
	if (client_attached) {
		indigo_detach_client(&client);
	}
	if (bus_started) {
		indigo_stop();
	}
	for (int i = 0; i < ARRAY_SIZE(cache); i++) {
		indigo_release_property(cache[i].property);
	}
	for (int i = 0; i < command_count; i++) {
		indigo_release_property(commands[i].property);
	}
	pthread_mutex_destroy(&peer_mutex);
}

static void remove_test_files(void) {
	DIR *dir = opendir(config_folder);
	if (dir) {
		struct dirent *entry;
		while ((entry = readdir(dir))) {
			if (strcmp(entry->d_name, ".") && strcmp(entry->d_name, "..")) {
				char path[1024];
				snprintf(path, sizeof(path), "%s/%s", config_folder, entry->d_name);
				unlink(path);
			}
		}
		closedir(dir);
		rmdir(config_folder);
	}
}

static const indigo_test_case tests[] = {
	{ "time limit matrix", time_limit_matrix },
	{ "rotator sync wrap", rotator_sync_wrap },
	{ "configuration failure", configuration_failure },

	{ "unpark failure", unpark_failure },
	{ "legacy dome slew", legacy_dome_slew },
	{ "geometry and threshold", geometry_and_threshold },
	{ "related solver filters", related_solver_filters },
	{ "related invalid filter", related_invalid_filter },
	{ "negative site fits", negative_site_fits },
	{ "lx200 invalid coordinates", lx200_invalid_coordinates },

	{ "operation timeouts", operation_timeouts },
	{ "coupled timeouts", coupled_timeouts },
	{ "immediate completion", immediate_completion },
	{ "coupled abort", coupled_abort },
	{ "shutdown active", shutdown_active },
	{ "shutdown server", shutdown_server },
	{ "all false start", all_false_start },
	{ "home momentary", home_momentary },
	{ "state lights", state_lights },
	{ "selection orders", selection_orders },
	{ "immediate rejection", immediate_rejection },
	{ "lx200 input matrix", lx200_input_matrix },

	{ "dome reselection", dome_reselection },
	{ "dome reselection legacy", dome_reselection_legacy },
	{ "feature persistence", feature_persistence },
	{ "persistent features", persistent_features },
	{ "lx200 ack", lx200_ack },
	{ "lx200 positive zero", lx200_positive_zero },
	{ "lx200 truncated command", lx200_truncated_command },
	{ "lx200 bind failure", lx200_bind_failure },
	{ "lx200 idle stop", lx200_idle_stop },
	{ "related agents", related_agents },
	{ "negative fits", negative_fits },
	{ "negative zero fits", negative_zero_fits },
	{ "limits", limits },
	{ "instances", instances },
	{ "stale capability", stale_capability },
	{ "process deselection", process_deselection },

	{ "metadata", metadata },
	{ "missing devices", missing_devices },
	{ "missing capabilities", missing_capabilities },
	{ "legacy success", legacy_success },
	{ "modern success", modern_success },
	{ "legacy failure", legacy_failure },
	{ "modern failure", modern_failure },
	{ "legacy abort", legacy_abort },
	{ "modern abort", modern_abort },
	{ "coordinates and busy", coordinates_and_busy },
	{ "sites", sites },
	{ "configuration", configuration },
	{ "coupled motion", coupled_motion },
	{ "slaving", slaving },
	{ "joystick", joystick },
	{ "LX200 protocol", lx200_protocol }
};

int main(int argc, char **argv) {
	setvbuf(stdout, NULL, _IONBF, 0);
	int result = 0, executed = 0, passed = 0;
	for (int i = 0; i < ARRAY_SIZE(tests); i++) {
		if (argc > 1 && !strstr(tests[i].name, argv[1])) {
			continue;
		}
		executed++;
		strcpy(config_folder, "/tmp/indigo_mount_test_XXXXXX");
		if (!mkdtemp(config_folder)) {
			return 1;
		}
		pid_t child = fork();
		if (child == 0) {
			alarm(45);
			bool ready = setup();
			int status = ready ? indigo_run_tests("Mount Agent integration", tests + i, 1) : 1;
			cleanup();
			exit(status || indigo_test_failures ? 1 : 0);
		}
		int status = -1;
		if (child < 0 || waitpid(child, &status, 0) != child || !WIFEXITED(status) || WEXITSTATUS(status)) {
			fprintf(stderr, "Failed case: %s (status %d)\n", tests[i].name, status);
			result = 1;
		} else {
			passed++;
		}
		remove_test_files();
	}
	printf("Mount Agent: %d/%d cases passed (including cleanup)\n", passed, executed);
	return executed ? result : 2;
}
