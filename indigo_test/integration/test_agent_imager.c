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
#include <indigo/indigo_ccd_driver.h>
#include <indigo_drivers/agent_imager/indigo_agent_imager.h>
#include <indigo_drivers/ccd_simulator/indigo_ccd_simulator.h>
#include "../test_runner.h"

#define AGENT "Imager Agent"
#define CAMERA CCD_SIMULATOR_IMAGER_CAMERA_NAME
#define FOCUSER CCD_SIMULATOR_FOCUSER_NAME
#define WHEEL CCD_SIMULATOR_WHEEL_NAME
#define ARRAY_SIZE(a) (sizeof(a) / sizeof((a)[0]))
#define REQUIRE(c) do { if (!(c)) { fprintf(stderr, "%s:%d: %s\n", __FILE__, __LINE__, #c); indigo_test_failures++; return false; } } while (0)

typedef struct {
	indigo_property *property;
	unsigned revision, busy, terminal, blobs;
	long blob_size;
	unsigned char blob_prefix[32];
} observation;
static observation cache[2048];
static pthread_mutex_t cache_mutex = PTHREAD_MUTEX_INITIALIZER;
static indigo_client client;
static char config_folder[] = "/tmp/indigo_imager_test_XXXXXX";
static bool bus_started, client_attached, simulator_started, agent_started;
static atomic_int camera_requests;
static atomic_int camera_abort_requests;
static _Atomic(indigo_device *) camera_device;
static indigo_result (*camera_change)(indigo_device *, indigo_client *, indigo_property *);
static atomic_int exposure_failures;

const char *imager_test_config_folder(void) {
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

static indigo_result observe(indigo_device *device, indigo_property *property, bool update) {
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
		if (update) {
			if (property->state == INDIGO_BUSY_STATE) {
				entry->busy++;
			} else {
				entry->terminal++;
			}
			if (property->type == INDIGO_BLOB_VECTOR && property->state == INDIGO_OK_STATE && property->count && property->items[0].blob.value) {
				entry->blobs++;
				entry->blob_size = property->items[0].blob.size;
				memcpy(entry->blob_prefix, property->items[0].blob.value, entry->blob_size < sizeof(entry->blob_prefix) ? entry->blob_size : sizeof(entry->blob_prefix));
			}
		}
	}
	if (!strcmp(property->device, CAMERA)) {
		camera_device = device;
	}
	pthread_mutex_unlock(&cache_mutex);
	return INDIGO_OK;
}

static indigo_result defined(indigo_client *client, indigo_device *device, indigo_property *property, const char *message) {
	return observe(device, property, false);
}

static indigo_result updated(indigo_client *client, indigo_device *device, indigo_property *property, const char *message) {
	return observe(device, property, true);
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

static indigo_client client = { .name = "Imager integration client", .define_property = defined, .update_property = updated, .delete_property = deleted, .send_message = message };

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
				result = p->type == INDIGO_SWITCH_VECTOR ? p->items[i].sw.value : p->items[i].number.value;
			}
		}
		indigo_release_property(p);
	}
	return result;
}

static unsigned blobs(const char *device) {
	pthread_mutex_lock(&cache_mutex);
	observation *entry = find(device, "CCD_IMAGE");
	unsigned result = entry ? entry->blobs : 0;
	pthread_mutex_unlock(&cache_mutex);
	return result;
}

static bool wait_state(const char *device, const char *name, unsigned after, int state) {
	double deadline = indigo_monotonic_time() + 20;
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

static bool txt(const char *device, const char *name, const char *item, const char *text, int state) {
	unsigned before = revision(device, name);
	REQUIRE(indigo_change_text_property_1(&client, device, name, item, text) == INDIGO_OK);
	return wait_state(device, name, before, state);
}

static void fail_exposure(indigo_device *device) {
	CCD_EXPOSURE_PROPERTY->state = INDIGO_ALERT_STATE;
	indigo_update_property(device, CCD_EXPOSURE_PROPERTY, "Injected exposure failure");
}

static indigo_result camera_spy(indigo_device *device, indigo_client *sender, indigo_property *property) {
	if (!strcmp(property->name, "CCD_ABORT_EXPOSURE")) {
		atomic_fetch_add(&camera_abort_requests, 1);
	}
	if (!strcmp(property->name, "CCD_EXPOSURE")) {
		atomic_fetch_add(&camera_requests, 1);
		if (atomic_load(&exposure_failures) > 0) {
			atomic_fetch_sub(&exposure_failures, 1);
			indigo_property_copy_values(CCD_EXPOSURE_PROPERTY, property, false);
			CCD_EXPOSURE_PROPERTY->state = INDIGO_BUSY_STATE;
			indigo_update_property(device, CCD_EXPOSURE_PROPERTY, NULL);
			indigo_set_timer(device, 0.05, fail_exposure, NULL);
			return INDIGO_OK;
		}
	}
	return camera_change(device, sender, property);
}

static bool setup(void) {
	REQUIRE(indigo_start() == INDIGO_OK);
	bus_started = true;
	REQUIRE(indigo_attach_client(&client) == INDIGO_OK);
	client_attached = true;
	REQUIRE(indigo_ccd_simulator(INDIGO_DRIVER_INIT, NULL) == INDIGO_OK);
	simulator_started = true;
	REQUIRE(indigo_agent_imager(INDIGO_DRIVER_INIT, NULL) == INDIGO_OK);
	agent_started = true;
	REQUIRE(wait_state(AGENT, "AGENT_START_PROCESS", 0, INDIGO_OK_STATE));
	camera_change = camera_device->change_property;
	camera_device->change_property = camera_spy;
	return true;
}

static bool connect_camera(void) {
	REQUIRE(sw(AGENT, "FILTER_CCD_LIST", CAMERA, true, INDIGO_OK_STATE));
	REQUIRE(sw(AGENT, "CCD_UPLOAD_MODE", "CLIENT", true, INDIGO_OK_STATE));
	REQUIRE(sw(AGENT, "CCD_IMAGE_FORMAT", "RAW", true, INDIGO_OK_STATE));
	REQUIRE(num(AGENT, "AGENT_IMAGER_BATCH", "EXPOSURE", 0.1));
	REQUIRE(sw(AGENT, "AGENT_PROCESS_FEATURES", "ENABLE_DITHERING", false, INDIGO_OK_STATE));
	return true;
}

static bool batch(int count) {
	REQUIRE(num(AGENT, "AGENT_IMAGER_BATCH", "COUNT", count));
	return true;
}

static bool run(const char *process, int state) {
	return sw(AGENT, "AGENT_START_PROCESS", process, true, state);
}

static bool abort_running(void) {
	unsigned start = revision(AGENT, "AGENT_START_PROCESS");
	REQUIRE(sw(AGENT, "AGENT_ABORT_PROCESS", "ABORT", true, INDIGO_OK_STATE));
	REQUIRE(wait_state(AGENT, "AGENT_START_PROCESS", start, -1));
	REQUIRE(value(AGENT, "AGENT_ABORT_PROCESS", "ABORT") == 0);
	return true;
}

typedef struct {
	indigo_device device;
	indigo_property *properties[4];
	int count;
	atomic_int requests;
	atomic_bool fail, hold;
	indigo_timer *timer;
	bool attached;
} peer;
static peer peers[4];

static indigo_result peer_enumerate(indigo_device *device, indigo_client *client, indigo_property *property) {
	peer *p = device->private_data;
	for (int i = 0; i < p->count; i++) {
		if (indigo_property_match(p->properties[i], property)) {
			indigo_define_property(device, p->properties[i], NULL);
		}
	}
	return indigo_device_enumerate_properties(device, client, property);
}

static void peer_finish(indigo_device *device) {
	peer *p = device->private_data;
	if (p == peers + 3) {
		p->properties[0]->state = INDIGO_OK_STATE;
		p->properties[0]->items[0].number.value = 0;
		indigo_update_property(device, p->properties[0], NULL);
		return;
	}
	p->properties[0]->state = atomic_load(&p->fail) ? INDIGO_ALERT_STATE : INDIGO_OK_STATE;
	p->properties[0]->items[0].sw.value = false;
	indigo_update_property(device, p->properties[0], NULL);
}

static indigo_result peer_change(indigo_device *device, indigo_client *client, indigo_property *property) {
	peer *p = device->private_data;
	for (int i = 0; i < p->count; i++) {
		if (indigo_property_match(p->properties[i], property)) {
			indigo_property_copy_values(p->properties[i], property, false);
			atomic_fetch_add(&p->requests, 1);
			p->properties[i]->state = INDIGO_OK_STATE;
			if (!strcmp(property->name, "CCD_ABORT_EXPOSURE")) {
				indigo_cancel_timer_sync(device, &p->timer);
				p->properties[0]->state = INDIGO_ALERT_STATE;
				indigo_update_property(device, p->properties[0], NULL);
				p->properties[i]->items[0].sw.value = false;
			}
			if (!strcmp(property->name, "AGENT_GUIDER_DITHER") || !strcmp(property->name, "CCD_EXPOSURE")) {
				p->properties[i]->state = INDIGO_BUSY_STATE;
				indigo_update_property(device, p->properties[i], NULL);
				if (!atomic_load(&p->hold)) {
					indigo_set_timer(device, 0.05, peer_finish, &p->timer);
				}
			} else {
				indigo_update_property(device, p->properties[i], NULL);
			}
			return INDIGO_OK;
		}
	}
	if (!strcmp(property->name, "CONNECTION")) {
		indigo_property_copy_values(CONNECTION_PROPERTY, property, false);
		CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
		indigo_update_property(device, CONNECTION_PROPERTY, NULL);
		return INDIGO_OK;
	}
	return indigo_device_change_property(device, client, property);
}

static indigo_result peer_attach(indigo_device *device) {
	indigo_device_attach(device, "agent_imager_test_peer", 1, device->private_data == peers + 3 ? INDIGO_INTERFACE_AUX_SHUTTER : INDIGO_INTERFACE_AGENT);
	return peer_enumerate(device, NULL, NULL);
}

static indigo_result peer_detach(indigo_device *device) {
	peer *p = device->private_data;
	indigo_cancel_timer_sync(device, &p->timer);
	for (int i = 0; i < p->count; i++) {
		indigo_delete_property(device, p->properties[i], NULL);
		indigo_release_property(p->properties[i]);
	}
	return indigo_device_detach(device);
}

static bool attach_peer(int index, const char *name, const char *property, const char *item) {
	peer *p = peers + index;
	p->device = (indigo_device)INDIGO_DEVICE_INITIALIZER("", peer_attach, peer_enumerate, peer_change, NULL, peer_detach);
	strcpy(p->device.name, name);
	p->device.private_data = p;
	p->properties[0] = indigo_init_switch_property(NULL, name, property, "Test", "Test", INDIGO_OK_STATE, INDIGO_RW_PERM, INDIGO_ANY_OF_MANY_RULE, 1);
	indigo_init_switch_item(p->properties[0]->items, item, item, false);
	p->count = 1;
	if (index == 1) {
		p->properties[1] = indigo_init_number_property(NULL, name, AGENT_MOUNT_DISPLAY_COORDINATES_PROPERTY_NAME, "Test", "Coordinates", INDIGO_OK_STATE, INDIGO_RW_PERM, 1);
		indigo_init_number_item(p->properties[1]->items, "TIME_TO_TRANSIT", "Transit", -24, 24, 0, -0.01);
		p->count++;
	}
	REQUIRE(indigo_attach_device(&p->device) == INDIGO_OK);
	p->attached = true;
	REQUIRE(sw(AGENT, "FILTER_RELATED_AGENT_LIST", name, true, INDIGO_OK_STATE));
	return true;
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

static void cleanup(void) {
	if (agent_started) {
		indigo_change_switch_property_1(&client, AGENT, "AGENT_ABORT_PROCESS", "ABORT", true);
		wait_state(AGENT, "AGENT_START_PROCESS", 0, -1);
		if (indigo_agent_imager(INDIGO_DRIVER_SHUTDOWN, NULL) != INDIGO_OK) {
			indigo_test_failures++;
		}
	}
	for (int i = 0; i < ARRAY_SIZE(peers); i++) {
		if (peers[i].attached) {
			indigo_detach_device(&peers[i].device);
		}
	}
	if (simulator_started) {
		const char *names[] = { CAMERA, FOCUSER, WHEEL, CCD_SIMULATOR_BAHTINOV_CAMERA_NAME };
		for (int i = 0; i < ARRAY_SIZE(names); i++) {
			if (value(names[i], "CONNECTION", "CONNECTED") == 1) {
				sw(names[i], "CONNECTION", "DISCONNECTED", true, INDIGO_OK_STATE);
			}
		}
		if (camera_device && camera_change) {
			camera_device->change_property = camera_change;
		}
		if (indigo_ccd_simulator(INDIGO_DRIVER_SHUTDOWN, NULL) != INDIGO_OK) {
			indigo_test_failures++;
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
}

static void metadata_and_missing_devices(void) {
	const char *properties[] = { "AGENT_IMAGER_BATCH", "AGENT_IMAGER_FOCUS", "AGENT_IMAGER_FOCUS_FAILURE", "AGENT_IMAGER_FOCUS_ESTIMATOR", "AGENT_IMAGER_CAPTURE", "AGENT_START_PROCESS", "AGENT_PAUSE_PROCESS", "AGENT_ABORT_PROCESS", "AGENT_PROCESS_FEATURES", "AGENT_IMAGER_DOWNLOAD_FILE", "AGENT_IMAGER_DOWNLOAD_FILES", "AGENT_IMAGER_DOWNLOAD_IMAGE", "AGENT_IMAGER_DELETE_FILE", "AGENT_IMAGER_DISK_USAGE", "AGENT_WHEEL_FILTER", "AGENT_FOCUSER_CONTROL", "AGENT_IMAGER_STARS", "AGENT_IMAGER_SELECTION", "AGENT_IMAGER_SPIKES", "AGENT_IMAGER_STATS", "AGENT_IMAGER_BREAKPOINT", "AGENT_IMAGER_RESUME_CONDITION", "AGENT_IMAGER_BARRIER_STATE" };
	for (int i = 0; i < ARRAY_SIZE(properties); i++) {
		ASSERT_TRUE(revision(AGENT, properties[i]) > 0);
	}
	const char *processes[] = { "PREVIEW_1", "PREVIEW", "EXPOSURE", "STREAMING", "FOCUSING" };
	for (int i = 0; i < ARRAY_SIZE(processes); i++) {
		ASSERT_TRUE(run(processes[i], INDIGO_ALERT_STATE));
		ASSERT_EQ_INT(0, value(AGENT, "AGENT_START_PROCESS", processes[i]));
	}
	ASSERT_TRUE(sw(AGENT, "AGENT_ABORT_PROCESS", "ABORT", true, INDIGO_OK_STATE));
	ASSERT_TRUE(sw(AGENT, "AGENT_PAUSE_PROCESS", "PAUSE", true, INDIGO_ALERT_STATE));
	ASSERT_TRUE(connect_camera());
	ASSERT_TRUE(run("FOCUSING", INDIGO_ALERT_STATE));
}

static void finite_acquisitions(void) {
	ASSERT_TRUE(connect_camera());
	unsigned before = blobs(CAMERA);
	ASSERT_TRUE(num(AGENT, "AGENT_IMAGER_CAPTURE", "CAPTURE", 0.1));
	ASSERT_EQ_INT(before + 1, blobs(CAMERA));
	ASSERT_TRUE(run("PREVIEW_1", INDIGO_OK_STATE));
	ASSERT_EQ_INT(before + 2, blobs(CAMERA));
	ASSERT_TRUE(batch(3));
	ASSERT_TRUE(run("EXPOSURE", INDIGO_OK_STATE));
	ASSERT_EQ_INT(before + 5, blobs(CAMERA));
	ASSERT_EQ_INT(3, value(AGENT, "AGENT_IMAGER_STATS", "FRAME"));
	ASSERT_TRUE(run("STREAMING", INDIGO_OK_STATE));
	ASSERT_EQ_INT(before + 8, blobs(CAMERA));
}

static void abort_all_capture_modes(void) {
	ASSERT_TRUE(connect_camera());
	const char *modes[] = { "PREVIEW_1", "PREVIEW", "EXPOSURE", "STREAMING" };
	for (int repeat = 0; repeat < 2; repeat++) {
		for (int i = 0; i < ARRAY_SIZE(modes); i++) {
			ASSERT_TRUE(batch(repeat ? -1 : 20));
			ASSERT_TRUE(num(AGENT, "AGENT_IMAGER_BATCH", "EXPOSURE", 1));
			ASSERT_TRUE(run(modes[i], INDIGO_BUSY_STATE));
			ASSERT_TRUE(wait_state(CAMERA, i == 3 ? "CCD_STREAMING" : "CCD_EXPOSURE", 0, INDIGO_BUSY_STATE));
			ASSERT_TRUE(abort_running());
			unsigned count = blobs(CAMERA);
			indigo_usleep(150000);
			ASSERT_EQ_INT(count, blobs(CAMERA));
			ASSERT_TRUE(num(AGENT, "AGENT_IMAGER_BATCH", "EXPOSURE", 0.1));
			ASSERT_TRUE(run("PREVIEW_1", INDIGO_OK_STATE));
		}
	}
}

static void exposure_failure_retry_and_recovery(void) {
	ASSERT_TRUE(connect_camera());
	atomic_store(&exposure_failures, 2);
	int before = atomic_load(&camera_requests);
	ASSERT_TRUE(run("PREVIEW_1", INDIGO_OK_STATE));
	ASSERT_EQ_INT(before + 3, atomic_load(&camera_requests));
	ASSERT_EQ_INT(1, blobs(CAMERA));
	ASSERT_TRUE(batch(2));
	atomic_store(&exposure_failures, 2);
	before = atomic_load(&camera_requests);
	ASSERT_TRUE(run("EXPOSURE", INDIGO_OK_STATE));
	ASSERT_EQ_INT(before + 4, atomic_load(&camera_requests));
	ASSERT_EQ_INT(3, blobs(CAMERA));
	const int counts[] = { 1, 2, -1 };
	for (int i = 0; i < ARRAY_SIZE(counts); i++) {
		ASSERT_TRUE(batch(counts[i]));
		atomic_store(&exposure_failures, 3);
		before = atomic_load(&camera_requests);
		unsigned images = blobs(CAMERA);
		ASSERT_TRUE(run("EXPOSURE", INDIGO_ALERT_STATE));
		ASSERT_EQ_INT(before + 3, atomic_load(&camera_requests));
		ASSERT_EQ_INT(images, blobs(CAMERA));
		ASSERT_TRUE(batch(1));
		ASSERT_TRUE(run("EXPOSURE", INDIGO_OK_STATE));
		ASSERT_EQ_INT(before + 4, atomic_load(&camera_requests));
		ASSERT_EQ_INT(images + 1, blobs(CAMERA));
	}
}

static void settings_selection_and_reset(void) {
	ASSERT_TRUE(connect_camera());
	ASSERT_TRUE(num(AGENT, "AGENT_IMAGER_FOCUS", "INITIAL", 24));
	ASSERT_EQ_INT(24, value(AGENT, "AGENT_IMAGER_FOCUS", "ITERATIVE_INITIAL"));
	ASSERT_EQ_INT(24, value(AGENT, "AGENT_IMAGER_FOCUS", "U_CURVE_STEP"));
	ASSERT_TRUE(num(AGENT, "AGENT_IMAGER_FOCUS", "FINAL", 3));
	ASSERT_EQ_INT(3, value(AGENT, "AGENT_IMAGER_FOCUS", "ITERATIVE_FINAL"));
	ASSERT_TRUE(num(AGENT, "AGENT_IMAGER_SELECTION", AGENT_IMAGER_SELECTION_STAR_COUNT_ITEM_NAME, 3));
	indigo_property *p = snapshot(AGENT, "AGENT_IMAGER_SELECTION");
	ASSERT_TRUE(p != NULL);
	ASSERT_EQ_INT(17, p->count);
	indigo_release_property(p);
	const char *estimators[] = { "U_CURVE", "HFD_PEAK", "RMS_CONTRAST", "BAHTINOV" };
	const int stars[] = { 3, 1, 0, 0 };
	for (int i = 0; i < ARRAY_SIZE(estimators); i++) {
		ASSERT_TRUE(sw(AGENT, "AGENT_IMAGER_FOCUS_ESTIMATOR", estimators[i], true, INDIGO_OK_STATE));
		ASSERT_EQ_INT(stars[i], value(AGENT, "AGENT_IMAGER_STATS", "MAX_STARS_TO_USE"));
	}
	ASSERT_TRUE(run("RESET", INDIGO_OK_STATE));
	ASSERT_EQ_INT(1, value(AGENT, "AGENT_IMAGER_BATCH", "COUNT"));
	ASSERT_EQ_INT(1, value(AGENT, "AGENT_IMAGER_FOCUS_ESTIMATOR", "U_CURVE"));
}

static void stars_statistics_and_format_restoration(void) {
	ASSERT_TRUE(connect_camera());
	ASSERT_TRUE(sw(AGENT, "AGENT_IMAGER_FOCUS_ESTIMATOR", "U_CURVE", true, INDIGO_OK_STATE));
	ASSERT_TRUE(sw(AGENT, "CCD_IMAGE_FORMAT", "FITS", true, INDIGO_OK_STATE));
	ASSERT_TRUE(sw(AGENT, "AGENT_IMAGER_STARS", "REFRESH", true, INDIGO_OK_STATE));
	indigo_property *p = snapshot(AGENT, "AGENT_IMAGER_STARS");
	ASSERT_TRUE(p && p->count > 1);
	char star[INDIGO_NAME_SIZE];
	strcpy(star, p->items[1].name);
	indigo_release_property(p);
	ASSERT_TRUE(sw(AGENT, "AGENT_IMAGER_STARS", star, true, INDIGO_OK_STATE));
	ASSERT_TRUE(value(AGENT, "AGENT_IMAGER_SELECTION", "X") > 0);
	ASSERT_TRUE(value(AGENT, "AGENT_IMAGER_SELECTION", "Y") > 0);
	ASSERT_TRUE(run("PREVIEW_1", INDIGO_OK_STATE));
	ASSERT_TRUE(value(AGENT, "AGENT_IMAGER_STATS", "HFD") > 0);
	ASSERT_EQ_INT(1, value(AGENT, "CCD_IMAGE_FORMAT", "FITS"));
	ASSERT_TRUE(run("CLEAR_SELECTION", INDIGO_OK_STATE));
	ASSERT_EQ_INT(0, value(AGENT, "AGENT_IMAGER_SELECTION", "X"));
}

static void breakpoint_case(const char *point) {
	ASSERT_TRUE(connect_camera());
	ASSERT_TRUE(batch(2));
	ASSERT_TRUE(num(AGENT, "AGENT_IMAGER_BATCH", "DELAY", 0.01));
	const char *points[] = { point };
	for (int action = 0; action < 2; action++) {
		for (int i = 0; i < ARRAY_SIZE(points); i++) {
			ASSERT_TRUE(sw(AGENT, "AGENT_IMAGER_BREAKPOINT", points[i], true, INDIGO_OK_STATE));
			unsigned paused = revision(AGENT, "AGENT_PAUSE_PROCESS");
			unsigned started = revision(AGENT, "AGENT_START_PROCESS");
			ASSERT_TRUE(run("EXPOSURE", INDIGO_BUSY_STATE));
			ASSERT_TRUE(wait_state(AGENT, "AGENT_PAUSE_PROCESS", paused, INDIGO_BUSY_STATE));
			int requested = atomic_load(&camera_requests);
			indigo_usleep(50000);
			ASSERT_EQ_INT(requested, atomic_load(&camera_requests));
			ASSERT_TRUE(sw(AGENT, "AGENT_IMAGER_BREAKPOINT", points[i], false, INDIGO_OK_STATE));
			if (action == 0) {
				ASSERT_TRUE(sw(AGENT, "AGENT_PAUSE_PROCESS", "PAUSE", false, INDIGO_OK_STATE));
				ASSERT_TRUE(wait_state(AGENT, "AGENT_START_PROCESS", started, INDIGO_OK_STATE));
			} else {
				ASSERT_TRUE(abort_running());
				ASSERT_EQ_INT(requested, atomic_load(&camera_requests));
			}
			ASSERT_TRUE(sw(AGENT, "AGENT_IMAGER_BREAKPOINT", points[i], false, INDIGO_OK_STATE));
		}
	}
	ASSERT_TRUE(run("PREVIEW_1", INDIGO_OK_STATE));
}

static void breakpoint_pre_batch(void) {
	breakpoint_case("PRE_BATCH");
}

static void breakpoint_pre_capture(void) {
	breakpoint_case("PRE_CAPTURE");
}

static void breakpoint_post_capture(void) {
	breakpoint_case("POST_CAPTURE");
}

static void breakpoint_pre_delay(void) {
	breakpoint_case("PRE_DELAY");
}

static void breakpoint_post_delay(void) {
	breakpoint_case("POST_DELAY");
}

static void breakpoint_post_batch(void) {
	breakpoint_case("POST_BATCH");
}

static void pause_resume_and_busy_guards(void) {
	ASSERT_TRUE(connect_camera());
	ASSERT_TRUE(num(AGENT, "AGENT_IMAGER_BATCH", "EXPOSURE", 0.5));
	ASSERT_TRUE(batch(3));
	const char *pauses[] = { "PAUSE", "PAUSE_WAIT" };
	for (int i = 0; i < ARRAY_SIZE(pauses); i++) {
		unsigned started = revision(AGENT, "AGENT_START_PROCESS");
		ASSERT_TRUE(run("EXPOSURE", INDIGO_BUSY_STATE));
		ASSERT_TRUE(wait_state(CAMERA, "CCD_EXPOSURE", 0, INDIGO_BUSY_STATE));
		indigo_change_switch_property_1(&client, AGENT, "AGENT_START_PROCESS", "STREAMING", true);
		ASSERT_EQ_INT(1, value(AGENT, "AGENT_START_PROCESS", "EXPOSURE"));
		ASSERT_TRUE(sw(AGENT, "AGENT_PAUSE_PROCESS", pauses[i], true, INDIGO_BUSY_STATE));
		ASSERT_TRUE(wait_state(CAMERA, "CCD_EXPOSURE", 0, -1));
		int count = atomic_load(&camera_requests);
		indigo_usleep(100000);
		ASSERT_EQ_INT(count, atomic_load(&camera_requests));
		ASSERT_TRUE(sw(AGENT, "AGENT_PAUSE_PROCESS", "PAUSE", false, INDIGO_OK_STATE));
		ASSERT_TRUE(wait_state(AGENT, "AGENT_START_PROCESS", started, INDIGO_OK_STATE));
	}
	ASSERT_TRUE(run("EXPOSURE", INDIGO_BUSY_STATE));
	ASSERT_TRUE(sw(AGENT, "AGENT_PAUSE_PROCESS", "PAUSE_WAIT", true, INDIGO_BUSY_STATE));
	ASSERT_TRUE(abort_running());
	ASSERT_TRUE(wait_state(AGENT, "AGENT_PAUSE_PROCESS", 0, INDIGO_ALERT_STATE));
}

static bool connect_focuser(void) {
	REQUIRE(sw(AGENT, "FILTER_FOCUSER_LIST", FOCUSER, true, INDIGO_OK_STATE));
	REQUIRE(num(FOCUSER, "FOCUSER_SPEED", "SPEED", 100));
	REQUIRE(num(AGENT, "AGENT_IMAGER_FOCUS", "STACK", 1));
	return true;
}

static void wheel_offsets_and_manual_focus(void) {
	ASSERT_TRUE(connect_camera());
	ASSERT_TRUE(connect_focuser());
	ASSERT_TRUE(sw(AGENT, "FILTER_WHEEL_LIST", WHEEL, true, INDIGO_OK_STATE));
	ASSERT_TRUE(txt(WHEEL, "WHEEL_SLOT_NAME", "SLOT_NAME_2", "Green", INDIGO_OK_STATE));
	ASSERT_TRUE(num(WHEEL, "WHEEL_SLOT_OFFSET", "SLOT_OFFSET_2", 20));
	ASSERT_TRUE(sw(AGENT, "AGENT_PROCESS_FEATURES", "APPLY_FILTER_OFFSETS", true, INDIGO_OK_STATE));
	double position = value(FOCUSER, "FOCUSER_POSITION", "POSITION");
	ASSERT_TRUE(sw(AGENT, "AGENT_WHEEL_FILTER", "2", true, INDIGO_OK_STATE));
	ASSERT_EQ_INT(2, value(WHEEL, "WHEEL_SLOT", "SLOT"));
	ASSERT_NEAR(position + 20, value(FOCUSER, "FOCUSER_POSITION", "POSITION"), 0.1);
	ASSERT_TRUE(sw(AGENT, "AGENT_WHEEL_FILTER", "1", true, INDIGO_OK_STATE));
	ASSERT_NEAR(position, value(FOCUSER, "FOCUSER_POSITION", "POSITION"), 0.1);
	ASSERT_TRUE(sw(AGENT, "AGENT_FOCUSER_CONTROL", "FOCUS_OUT", true, INDIGO_BUSY_STATE));
	ASSERT_TRUE(wait_state(FOCUSER, "FOCUSER_STEPS", 0, INDIGO_BUSY_STATE));
	ASSERT_TRUE(sw(AGENT, "AGENT_FOCUSER_CONTROL", "FOCUS_OUT", false, INDIGO_OK_STATE));
	ASSERT_TRUE(wait_state(FOCUSER, "FOCUSER_STEPS", 0, -1));
}

static void bracketing_returns_to_start(void) {
	ASSERT_TRUE(connect_camera());
	ASSERT_TRUE(connect_focuser());
	ASSERT_TRUE(sw(AGENT, "AGENT_PROCESS_FEATURES", "MACRO_MODE", true, INDIGO_OK_STATE));
	ASSERT_TRUE(num(AGENT, "AGENT_IMAGER_FOCUS", "BRACKETING_STEP", 10));
	ASSERT_TRUE(batch(3));
	double position = value(FOCUSER, "FOCUSER_POSITION", "POSITION");
	unsigned before = blobs(CAMERA);
	ASSERT_TRUE(run("EXPOSURE", INDIGO_OK_STATE));
	ASSERT_EQ_INT(before + 3, blobs(CAMERA));
	ASSERT_NEAR(position, value(FOCUSER, "FOCUSER_POSITION", "POSITION"), 0.1);
	ASSERT_TRUE(num(AGENT, "AGENT_IMAGER_BATCH", "EXPOSURE", 1));
	ASSERT_TRUE(run("EXPOSURE", INDIGO_BUSY_STATE));
	ASSERT_TRUE(wait_state(CAMERA, "CCD_EXPOSURE", 0, INDIGO_BUSY_STATE));
	ASSERT_TRUE(abort_running());
	ASSERT_NEAR(position, value(FOCUSER, "FOCUSER_POSITION", "POSITION"), 0.1);
}

static void autofocus_estimators(void) {
	ASSERT_TRUE(connect_camera());
	ASSERT_TRUE(connect_focuser());
	const char *estimators[] = { "U_CURVE", "HFD_PEAK", "RMS_CONTRAST" };
	for (int i = 0; i < ARRAY_SIZE(estimators); i++) {
		ASSERT_TRUE(num(FOCUSER, "FOCUSER_POSITION", "POSITION", 60));
		ASSERT_TRUE(sw(AGENT, "AGENT_IMAGER_FOCUS_ESTIMATOR", estimators[i], true, INDIGO_OK_STATE));
		ASSERT_TRUE(run("FOCUSING", INDIGO_OK_STATE));
		ASSERT_TRUE(fabs(value(FOCUSER, "FOCUSER_POSITION", "POSITION")) <= 20);
		ASSERT_EQ_INT(0, value(AGENT, "AGENT_START_PROCESS", "FOCUSING"));
		ASSERT_TRUE(isfinite(value(AGENT, "AGENT_IMAGER_STATS", "BEST_FOCUS_DEVIATION")));
	}
}

static void autofocus_abort_and_failure(void) {
	ASSERT_TRUE(connect_camera());
	ASSERT_TRUE(connect_focuser());
	ASSERT_TRUE(sw(AGENT, "AGENT_IMAGER_FOCUS_ESTIMATOR", "U_CURVE", true, INDIGO_OK_STATE));
	ASSERT_TRUE(num(AGENT, "AGENT_IMAGER_BATCH", "EXPOSURE", 1));
	ASSERT_TRUE(run("FOCUSING", INDIGO_BUSY_STATE));
	ASSERT_TRUE(wait_state(CAMERA, "CCD_EXPOSURE", 0, INDIGO_BUSY_STATE));
	ASSERT_TRUE(abort_running());
	ASSERT_TRUE(wait_state(AGENT, "AGENT_START_PROCESS", 0, INDIGO_ALERT_STATE));
	ASSERT_TRUE(num(AGENT, "AGENT_IMAGER_BATCH", "EXPOSURE", 0.1));
	atomic_store(&exposure_failures, 3);
	ASSERT_TRUE(run("FOCUSING", INDIGO_ALERT_STATE));
	ASSERT_TRUE(run("PREVIEW_1", INDIGO_OK_STATE));
}

static void download_listing_payload_delete(void) {
	ASSERT_TRUE(connect_camera());
	char folder[1024], path[1024];
	snprintf(folder, sizeof(folder), "%s/", config_folder);
	snprintf(path, sizeof(path), "%s/frame.raw", config_folder);
	FILE *file = fopen(path, "wb");
	ASSERT_TRUE(file != NULL);
	const char payload[] = "INDIGO integration image payload";
	ASSERT_EQ_INT(sizeof(payload), fwrite(payload, 1, sizeof(payload), file));
	fclose(file);
	ASSERT_TRUE(txt(AGENT, "CCD_LOCAL_MODE", "DIR", folder, INDIGO_OK_STATE));
	ASSERT_TRUE(sw(AGENT, "AGENT_IMAGER_DOWNLOAD_FILES", "REFRESH", true, INDIGO_OK_STATE));
	ASSERT_TRUE(txt(AGENT, "AGENT_IMAGER_DOWNLOAD_FILE", "FILE", "frame.raw", INDIGO_OK_STATE));
	pthread_mutex_lock(&cache_mutex);
	observation *entry = find(AGENT, "AGENT_IMAGER_DOWNLOAD_IMAGE");
	bool matches = entry && entry->blob_size == sizeof(payload) && !memcmp(entry->blob_prefix, payload, sizeof(payload));
	pthread_mutex_unlock(&cache_mutex);
	ASSERT_TRUE(matches);
	ASSERT_TRUE(txt(AGENT, "AGENT_IMAGER_DOWNLOAD_FILE", "FILE", "absent.raw", INDIGO_ALERT_STATE));
	ASSERT_TRUE(txt(AGENT, "AGENT_IMAGER_DELETE_FILE", "FILE", "frame.raw", INDIGO_OK_STATE));
	ASSERT_TRUE(access(path, F_OK) != 0);
	ASSERT_TRUE(txt(AGENT, "AGENT_IMAGER_DELETE_FILE", "FILE", "frame.raw", INDIGO_ALERT_STATE));
	ASSERT_TRUE(value(AGENT, "AGENT_IMAGER_DISK_USAGE", "TOTAL") > 0);
}

static void dither_cadence_failure_and_abort(void) {
	ASSERT_TRUE(connect_camera());
	ASSERT_TRUE(attach_peer(0, "Guider Agent Test", "AGENT_GUIDER_DITHER", "TRIGGER"));
	ASSERT_TRUE(sw(AGENT, "AGENT_PROCESS_FEATURES", "ENABLE_DITHERING", true, INDIGO_OK_STATE));
	ASSERT_TRUE(batch(3));
	ASSERT_TRUE(num(AGENT, "AGENT_IMAGER_BATCH", "FRAMES_TO_SKIP_BEFORE_DITHER", 0));
	ASSERT_TRUE(run("EXPOSURE", INDIGO_OK_STATE));
	ASSERT_EQ_INT(2, atomic_load(&peers[0].requests));
	ASSERT_TRUE(sw(AGENT, "AGENT_PROCESS_FEATURES", AGENT_IMAGER_DITHER_AFTER_BATCH_FEATURE_ITEM_NAME, true, INDIGO_OK_STATE));
	ASSERT_TRUE(num(AGENT, "AGENT_IMAGER_BATCH", "FRAMES_TO_SKIP_BEFORE_DITHER", 1));
	ASSERT_TRUE(batch(4));
	ASSERT_TRUE(run("EXPOSURE", INDIGO_OK_STATE));
	ASSERT_EQ_INT(4, atomic_load(&peers[0].requests));
	atomic_store(&peers[0].fail, true);
	ASSERT_TRUE(batch(1));
	ASSERT_TRUE(num(AGENT, "AGENT_IMAGER_BATCH", "FRAMES_TO_SKIP_BEFORE_DITHER", 0));
	ASSERT_TRUE(run("EXPOSURE", INDIGO_OK_STATE));
	ASSERT_EQ_INT(5, atomic_load(&peers[0].requests));
	atomic_store(&peers[0].hold, true);
	ASSERT_TRUE(run("EXPOSURE", INDIGO_BUSY_STATE));
	ASSERT_TRUE(wait_state("Guider Agent Test", "AGENT_GUIDER_DITHER", 0, INDIGO_BUSY_STATE));
	ASSERT_TRUE(abort_running());
}

static void mount_transit_and_solver_coordination(void) {
	ASSERT_TRUE(connect_camera());
	ASSERT_TRUE(attach_peer(1, "Mount Agent Test", "ABORT_RELATED_PROCESS", "IMAGER"));
	ASSERT_TRUE(attach_peer(2, "Astrometry Agent Test", "AGENT_PLATESOLVER_SOLVE_IMAGES", "DISABLED"));
	ASSERT_TRUE(sw(AGENT, "AGENT_PROCESS_FEATURES", "PAUSE_AFTER_TRANSIT", true, INDIGO_OK_STATE));
	ASSERT_TRUE(num("Mount Agent Test", AGENT_MOUNT_DISPLAY_COORDINATES_PROPERTY_NAME, "TIME_TO_TRANSIT", -0.01));
	ASSERT_TRUE(run("EXPOSURE", INDIGO_BUSY_STATE));
	ASSERT_TRUE(wait_state(AGENT, "AGENT_PAUSE_PROCESS", 0, INDIGO_BUSY_STATE));
	ASSERT_EQ_INT(1, value(AGENT, "AGENT_PAUSE_PROCESS", "PAUSE_AFTER_TRANSIT"));
	ASSERT_EQ_INT(0, value("Mount Agent Test", "ABORT_RELATED_PROCESS", "IMAGER"));
	ASSERT_EQ_INT(1, value("Astrometry Agent Test", "AGENT_PLATESOLVER_SOLVE_IMAGES", "DISABLED"));
	ASSERT_TRUE(sw(AGENT, "AGENT_PAUSE_PROCESS", "PAUSE", false, INDIGO_OK_STATE));
	ASSERT_TRUE(wait_state(AGENT, "AGENT_START_PROCESS", 0, INDIGO_OK_STATE));
	ASSERT_EQ_INT(0, value("Mount Agent Test", "ABORT_RELATED_PROCESS", "IMAGER"));
	ASSERT_TRUE(atomic_load(&peers[1].requests) >= 4);
}

static void additional_instances_and_barrier(void) {
	ASSERT_TRUE(connect_camera());
	ASSERT_TRUE(num(AGENT, "ADDITIONAL_INSTANCES", "COUNT", 1));
	const char *other = "Imager Agent #2";
	ASSERT_TRUE(wait_state(other, "AGENT_START_PROCESS", 0, INDIGO_OK_STATE));
	ASSERT_TRUE(isnan(value(AGENT, "FILTER_RELATED_AGENT_LIST", AGENT)));
	ASSERT_TRUE(isnan(value(other, "FILTER_RELATED_AGENT_LIST", other)));
	ASSERT_TRUE(sw(other, "FILTER_CCD_LIST", CCD_SIMULATOR_BAHTINOV_CAMERA_NAME, true, INDIGO_OK_STATE));
	ASSERT_TRUE(sw(other, "CCD_UPLOAD_MODE", "CLIENT", true, INDIGO_OK_STATE));
	ASSERT_TRUE(sw(other, "CCD_IMAGE_FORMAT", "RAW", true, INDIGO_OK_STATE));
	ASSERT_TRUE(num(other, "AGENT_IMAGER_BATCH", "EXPOSURE", 0.1));
	ASSERT_TRUE(num(other, "AGENT_IMAGER_BATCH", "COUNT", 1));
	ASSERT_TRUE(sw(AGENT, "AGENT_IMAGER_RESUME_CONDITION", "BARRIER", true, INDIGO_OK_STATE));
	ASSERT_TRUE(sw(AGENT, "FILTER_RELATED_AGENT_LIST", other, true, INDIGO_OK_STATE));
	ASSERT_EQ_INT(0, value(other, "FILTER_RELATED_AGENT_LIST", AGENT));
	ASSERT_TRUE(sw(AGENT, "AGENT_IMAGER_BREAKPOINT", "PRE_CAPTURE", true, INDIGO_OK_STATE));
	ASSERT_EQ_INT(1, value(other, "AGENT_IMAGER_BREAKPOINT", "PRE_CAPTURE"));
	ASSERT_EQ_INT(1, value(other, "AGENT_IMAGER_RESUME_CONDITION", "TRIGGER"));
	ASSERT_TRUE(batch(1));
	ASSERT_TRUE(run("EXPOSURE", INDIGO_OK_STATE));
	ASSERT_TRUE(wait_state(other, "AGENT_START_PROCESS", 0, INDIGO_OK_STATE));
	ASSERT_TRUE(isnan(value(AGENT, "FILTER_RELATED_AGENT_LIST", AGENT)));
	ASSERT_TRUE(isnan(value(other, "FILTER_RELATED_AGENT_LIST", other)));
	ASSERT_EQ_INT(1, blobs(CAMERA));
	ASSERT_EQ_INT(1, blobs(CCD_SIMULATOR_BAHTINOV_CAMERA_NAME));
	ASSERT_TRUE(sw(AGENT, "AGENT_IMAGER_BREAKPOINT", "PRE_CAPTURE", false, INDIGO_OK_STATE));
	ASSERT_TRUE(num(AGENT, "AGENT_IMAGER_BATCH", "EXPOSURE", 2));
	ASSERT_TRUE(num(other, "AGENT_IMAGER_BATCH", "EXPOSURE", 2));
	ASSERT_TRUE(run("EXPOSURE", INDIGO_BUSY_STATE));
	ASSERT_TRUE(wait_state(other, "AGENT_START_PROCESS", 0, INDIGO_BUSY_STATE));
	ASSERT_TRUE(abort_running());
	ASSERT_TRUE(wait_state(other, "AGENT_START_PROCESS", 0, INDIGO_ALERT_STATE));
	ASSERT_TRUE(sw(other, "FILTER_CCD_LIST", "NONE", true, INDIGO_OK_STATE));
	ASSERT_TRUE(sw(AGENT, "FILTER_RELATED_AGENT_LIST", other, false, INDIGO_OK_STATE));
}

static void additional_instance_lifecycle(void) {
	for (int i = 0; i < 3; i++) {
		ASSERT_TRUE(num(AGENT, "ADDITIONAL_INSTANCES", "COUNT", 2));
		ASSERT_TRUE(num("Imager Agent #3", "AGENT_IMAGER_BATCH", "COUNT", 3));
		ASSERT_TRUE(num(AGENT, "ADDITIONAL_INSTANCES", "COUNT", 1));
		ASSERT_TRUE(num("Imager Agent #2", "AGENT_IMAGER_BATCH", "COUNT", 2));
		ASSERT_TRUE(num(AGENT, "ADDITIONAL_INSTANCES", "COUNT", 0));
	}
	ASSERT_TRUE(num(AGENT, "ADDITIONAL_INSTANCES", "COUNT", 2));
}

static void independent_instances_and_reselection(void) {
	ASSERT_TRUE(connect_camera());
	ASSERT_TRUE(num(AGENT, "ADDITIONAL_INSTANCES", "COUNT", 1));
	const char *other = "Imager Agent #2";
	ASSERT_TRUE(sw(other, "FILTER_CCD_LIST", CCD_SIMULATOR_BAHTINOV_CAMERA_NAME, true, INDIGO_OK_STATE));
	ASSERT_TRUE(num(other, "AGENT_IMAGER_BATCH", "EXPOSURE", 0.1));
	ASSERT_TRUE(num(AGENT, "AGENT_IMAGER_BATCH", "EXPOSURE", 1));
	ASSERT_TRUE(run("PREVIEW", INDIGO_BUSY_STATE));
	ASSERT_TRUE(sw(other, "AGENT_START_PROCESS", "PREVIEW_1", true, INDIGO_OK_STATE));
	ASSERT_TRUE(abort_running());
	ASSERT_EQ_INT(1, blobs(CCD_SIMULATOR_BAHTINOV_CAMERA_NAME));
	ASSERT_TRUE(sw(other, "FILTER_CCD_LIST", "NONE", true, INDIGO_OK_STATE));
	ASSERT_TRUE(num(AGENT, "ADDITIONAL_INSTANCES", "COUNT", 0));
	ASSERT_TRUE(sw(AGENT, "FILTER_CCD_LIST", "NONE", true, INDIGO_OK_STATE));
	ASSERT_TRUE(sw(AGENT, "FILTER_CCD_LIST", CCD_SIMULATOR_BAHTINOV_CAMERA_NAME, true, INDIGO_OK_STATE));
	ASSERT_TRUE(num(AGENT, "AGENT_IMAGER_BATCH", "EXPOSURE", 0.1));
	ASSERT_TRUE(run("PREVIEW_1", INDIGO_OK_STATE));
}

static void bahtinov_preview_and_focus(void) {
	ASSERT_TRUE(sw(AGENT, "FILTER_CCD_LIST", CCD_SIMULATOR_BAHTINOV_CAMERA_NAME, true, INDIGO_OK_STATE));
	ASSERT_TRUE(connect_focuser());
	ASSERT_TRUE(num(AGENT, "AGENT_IMAGER_BATCH", "EXPOSURE", 0.1));
	ASSERT_TRUE(sw(AGENT, "AGENT_IMAGER_FOCUS_ESTIMATOR", "BAHTINOV", true, INDIGO_OK_STATE));
	ASSERT_TRUE(run("PREVIEW_1", INDIGO_OK_STATE));
	ASSERT_TRUE(value(AGENT, "AGENT_IMAGER_STATS", AGENT_IMAGER_STATS_BAHTINOV_ITEM_NAME) >= 0);
	ASSERT_TRUE(num(AGENT, "AGENT_IMAGER_FOCUS", "ITERATIVE_INITIAL", 2));
	ASSERT_TRUE(num(AGENT, "AGENT_IMAGER_FOCUS", "ITERATIVE_FINAL", 1));
	ASSERT_TRUE(run("FOCUSING", INDIGO_OK_STATE));
}

static void configuration_reload(void) {
	ASSERT_TRUE(num(AGENT, "AGENT_IMAGER_BATCH", "COUNT", 7));
	ASSERT_TRUE(num(AGENT, "AGENT_IMAGER_FOCUS", "ITERATIVE_INITIAL", 27));
	ASSERT_TRUE(sw(AGENT, "AGENT_IMAGER_FOCUS_FAILURE", "STOP", true, INDIGO_OK_STATE));
	ASSERT_TRUE(indigo_agent_imager(INDIGO_DRIVER_SHUTDOWN, NULL) == INDIGO_OK);
	agent_started = false;
	ASSERT_TRUE(indigo_agent_imager(INDIGO_DRIVER_INIT, NULL) == INDIGO_OK);
	agent_started = true;
	ASSERT_TRUE(wait_state(AGENT, "AGENT_IMAGER_BATCH", 0, INDIGO_OK_STATE));
	ASSERT_EQ_INT(7, value(AGENT, "AGENT_IMAGER_BATCH", "COUNT"));
	ASSERT_EQ_INT(27, value(AGENT, "AGENT_IMAGER_FOCUS", "ITERATIVE_INITIAL"));
	ASSERT_EQ_INT(1, value(AGENT, "AGENT_IMAGER_FOCUS_FAILURE", "STOP"));
}

static void shutdown_while_paused(void) {
	ASSERT_TRUE(connect_camera());
	ASSERT_TRUE(sw(AGENT, "AGENT_IMAGER_BREAKPOINT", "PRE_CAPTURE", true, INDIGO_OK_STATE));
	ASSERT_TRUE(run("EXPOSURE", INDIGO_BUSY_STATE));
	ASSERT_TRUE(wait_state(AGENT, "AGENT_PAUSE_PROCESS", 0, INDIGO_BUSY_STATE));
	ASSERT_TRUE(indigo_agent_imager(INDIGO_DRIVER_SHUTDOWN, NULL) == INDIGO_OK);
	agent_started = false;
	ASSERT_TRUE(indigo_agent_imager(INDIGO_DRIVER_INIT, NULL) == INDIGO_OK);
	agent_started = true;
	ASSERT_TRUE(wait_state(AGENT, "AGENT_START_PROCESS", 0, INDIGO_OK_STATE));
}

static void camera_disconnect_and_recover(void) {
	ASSERT_TRUE(connect_camera());
	const char *modes[] = { "PREVIEW", "EXPOSURE", "STREAMING" };
	for (int i = 0; i < ARRAY_SIZE(modes); i++) {
		ASSERT_TRUE(num(AGENT, "AGENT_IMAGER_BATCH", "EXPOSURE", 2));
		const char *property = i == 2 ? "CCD_STREAMING" : "CCD_EXPOSURE";
		unsigned exposure = revision(CAMERA, property);
		ASSERT_TRUE(run(modes[i], INDIGO_BUSY_STATE));
		ASSERT_TRUE(wait_state(CAMERA, property, exposure, INDIGO_BUSY_STATE));
		unsigned process = revision(AGENT, "AGENT_START_PROCESS");
		ASSERT_TRUE(sw(CAMERA, "CONNECTION", "DISCONNECTED", true, INDIGO_OK_STATE));
		ASSERT_TRUE(wait_state(AGENT, "AGENT_START_PROCESS", process, i == 0 ? INDIGO_OK_STATE : INDIGO_ALERT_STATE));
		ASSERT_EQ_INT(0, value(AGENT, "AGENT_START_PROCESS", modes[i]));
		ASSERT_TRUE(connect_camera());
		unsigned images = blobs(CAMERA);
		ASSERT_TRUE(run(i == 2 ? "STREAMING" : "PREVIEW_1", INDIGO_OK_STATE));
		ASSERT_EQ_INT(images + 1, blobs(CAMERA));
	}
}

static void selection_regions_binning_and_subframe(void) {
	ASSERT_TRUE(connect_camera());
	ASSERT_TRUE(num(AGENT, "AGENT_IMAGER_SELECTION", "INCLUDE_LEFT", 100));
	ASSERT_TRUE(num(AGENT, "AGENT_IMAGER_SELECTION", "INCLUDE_TOP", 100));
	ASSERT_TRUE(num(AGENT, "AGENT_IMAGER_SELECTION", "INCLUDE_WIDTH", 800));
	ASSERT_TRUE(num(AGENT, "AGENT_IMAGER_SELECTION", "INCLUDE_HEIGHT", 800));
	ASSERT_TRUE(sw(AGENT, "AGENT_IMAGER_STARS", "REFRESH", true, INDIGO_OK_STATE));
	ASSERT_TRUE(num(AGENT, "AGENT_IMAGER_SELECTION", "SUBFRAME", 2));
	ASSERT_TRUE(run("PREVIEW_1", INDIGO_OK_STATE));
	unsigned bin_revision = revision(AGENT, "CCD_BIN");
	indigo_change_number_property(&client, AGENT, "CCD_BIN", 2, (const char *[]){ "HORIZONTAL", "VERTICAL" }, (double []){ 2, 2 });
	ASSERT_TRUE(wait_state(AGENT, "CCD_BIN", bin_revision, INDIGO_OK_STATE));
	ASSERT_TRUE(run("PREVIEW_1", INDIGO_OK_STATE));
	ASSERT_TRUE(value(AGENT, "AGENT_IMAGER_SELECTION", "X") < 800);
	ASSERT_TRUE(value(AGENT, "AGENT_IMAGER_SELECTION", "Y") < 600);
}

static void local_batch_and_fits_headers(void) {
	ASSERT_TRUE(connect_camera());
	ASSERT_TRUE(connect_focuser());
	ASSERT_TRUE(sw(AGENT, "FILTER_WHEEL_LIST", WHEEL, true, INDIGO_OK_STATE));
	char folder[1024];
	snprintf(folder, sizeof(folder), "%s/", config_folder);
	ASSERT_TRUE(txt(AGENT, "CCD_LOCAL_MODE", "DIR", folder, INDIGO_OK_STATE));
	ASSERT_TRUE(txt(AGENT, "CCD_LOCAL_MODE", "PREFIX", "test_XXX", INDIGO_OK_STATE));
	ASSERT_TRUE(sw(AGENT, "CCD_UPLOAD_MODE", "BOTH", true, INDIGO_OK_STATE));
	ASSERT_TRUE(sw(AGENT, "CCD_IMAGE_FORMAT", "FITS", true, INDIGO_OK_STATE));
	ASSERT_TRUE(batch(2));
	ASSERT_TRUE(run("EXPOSURE", INDIGO_OK_STATE));
	ASSERT_TRUE(sw(AGENT, "AGENT_IMAGER_DOWNLOAD_FILES", "REFRESH", true, INDIGO_OK_STATE));
	indigo_property *p = snapshot(AGENT, "AGENT_IMAGER_DOWNLOAD_FILES");
	ASSERT_TRUE(p && p->count == 3);
	char file[INDIGO_NAME_SIZE];
	strcpy(file, p->items[1].name);
	indigo_release_property(p);
	ASSERT_TRUE(txt(AGENT, "AGENT_IMAGER_DOWNLOAD_FILE", "FILE", file, INDIGO_OK_STATE));
	pthread_mutex_lock(&cache_mutex);
	observation *entry = find(AGENT, "AGENT_IMAGER_DOWNLOAD_IMAGE");
	bool fits = entry && !memcmp(entry->blob_prefix, "SIMPLE", 6);
	pthread_mutex_unlock(&cache_mutex);
	ASSERT_TRUE(fits);
	char path[1024], header[2881] = { 0 };
	snprintf(path, sizeof(path), "%s/%s", config_folder, file);
	FILE *saved = fopen(path, "rb");
	ASSERT_TRUE(saved != NULL);
	size_t read_size = fread(header, 1, 2880, saved);
	fclose(saved);
	ASSERT_EQ_INT(2880, read_size);
	ASSERT_TRUE(strstr(header, "FILTER") && strstr(header, "FOCUSPOS") && strstr(header, "FOCTEMP"));
}

static void external_shutter_routing(void) {
	ASSERT_TRUE(connect_camera());
	peer *p = peers + 3;
	p->device = (indigo_device)INDIGO_DEVICE_INITIALIZER("External shutter test", peer_attach, peer_enumerate, peer_change, NULL, peer_detach);
	p->device.private_data = p;
	p->properties[0] = indigo_init_number_property(NULL, p->device.name, "CCD_EXPOSURE", "Test", "Exposure", INDIGO_OK_STATE, INDIGO_RW_PERM, 1);
	indigo_init_number_item(p->properties[0]->items, "EXPOSURE", "Exposure", 0, 100, 0, 0);
	p->properties[1] = indigo_init_switch_property(NULL, p->device.name, "CCD_ABORT_EXPOSURE", "Test", "Abort", INDIGO_OK_STATE, INDIGO_RW_PERM, INDIGO_AT_MOST_ONE_RULE, 1);
	indigo_init_switch_item(p->properties[1]->items, "ABORT_EXPOSURE", "Abort", false);
	p->count = 2;
	ASSERT_TRUE(indigo_attach_device(&p->device) == INDIGO_OK);
	p->attached = true;
	ASSERT_TRUE(sw(AGENT, "FILTER_AUX_1_LIST", p->device.name, true, INDIGO_OK_STATE));
	ASSERT_TRUE(run("PREVIEW_1", INDIGO_OK_STATE));
	ASSERT_TRUE(atomic_load(&p->requests) > 0);
	ASSERT_NEAR(0, value(CAMERA, "CCD_EXPOSURE", "EXPOSURE"), 0.001);
	const char *modes[] = { "PREVIEW", "EXPOSURE" };
	for (int i = 0; i < ARRAY_SIZE(modes); i++) {
		atomic_store(&p->hold, true);
		unsigned exposure = revision(p->device.name, "CCD_EXPOSURE");
		unsigned aborted = revision(p->device.name, "CCD_ABORT_EXPOSURE");
		int camera_aborts = atomic_load(&camera_abort_requests);
		ASSERT_TRUE(run(modes[i], INDIGO_BUSY_STATE));
		ASSERT_TRUE(wait_state(p->device.name, "CCD_EXPOSURE", exposure, INDIGO_BUSY_STATE));
		int shutter_requests = atomic_load(&p->requests);
		ASSERT_TRUE(abort_running());
		ASSERT_TRUE(wait_state(p->device.name, "CCD_ABORT_EXPOSURE", aborted, INDIGO_OK_STATE));
		ASSERT_TRUE(wait_state(p->device.name, "CCD_EXPOSURE", exposure, INDIGO_ALERT_STATE));
		ASSERT_EQ_INT(shutter_requests + 1, atomic_load(&p->requests));
		ASSERT_EQ_INT(camera_aborts + 1, atomic_load(&camera_abort_requests));
		ASSERT_EQ_INT(0, value(p->device.name, "CCD_ABORT_EXPOSURE", "ABORT_EXPOSURE"));
		atomic_store(&p->hold, false);
		ASSERT_TRUE(run("PREVIEW_1", INDIGO_OK_STATE));
	}
	ASSERT_TRUE(sw(AGENT, "FILTER_AUX_1_LIST", "NONE", true, INDIGO_OK_STATE));
	int shutter_requests = atomic_load(&p->requests);
	int camera_aborts = atomic_load(&camera_abort_requests);
	ASSERT_TRUE(num(AGENT, "AGENT_IMAGER_BATCH", "EXPOSURE", 1));
	ASSERT_TRUE(run("PREVIEW", INDIGO_BUSY_STATE));
	ASSERT_TRUE(wait_state(CAMERA, "CCD_EXPOSURE", 0, INDIGO_BUSY_STATE));
	ASSERT_TRUE(abort_running());
	ASSERT_EQ_INT(shutter_requests, atomic_load(&p->requests));
	ASSERT_EQ_INT(camera_aborts + 1, atomic_load(&camera_abort_requests));
}

static void streaming_failure_alert(indigo_device *device) {
	CCD_STREAMING_PROPERTY->state = INDIGO_ALERT_STATE;
	indigo_update_property(device, CCD_STREAMING_PROPERTY, "Injected stream failure");
}

static indigo_result failing_stream(indigo_device *device, indigo_client *sender, indigo_property *property) {
	if (!strcmp(property->name, "CCD_STREAMING")) {
		indigo_property_copy_values(CCD_STREAMING_PROPERTY, property, false);
		CCD_STREAMING_PROPERTY->state = INDIGO_BUSY_STATE;
		indigo_update_property(device, CCD_STREAMING_PROPERTY, NULL);
		indigo_set_timer(device, 0.05, streaming_failure_alert, NULL);
		return INDIGO_OK;
	}
	return camera_spy(device, sender, property);
}

static void streaming_failure_propagates(void) {
	ASSERT_TRUE(connect_camera());
	const int counts[] = { 1, 3, -1 };
	for (int i = 0; i < ARRAY_SIZE(counts); i++) {
		ASSERT_TRUE(batch(counts[i]));
		unsigned images = blobs(CAMERA);
		camera_device->change_property = failing_stream;
		ASSERT_TRUE(run("STREAMING", INDIGO_ALERT_STATE));
		ASSERT_EQ_INT(images, blobs(CAMERA));
		ASSERT_EQ_INT(0, value(AGENT, "AGENT_START_PROCESS", "STREAMING"));
		camera_device->change_property = camera_spy;
		ASSERT_TRUE(batch(3));
		ASSERT_TRUE(run("STREAMING", INDIGO_OK_STATE));
		ASSERT_EQ_INT(images + 3, blobs(CAMERA));
	}
}

static void invalid_frame(indigo_device *device) {
	indigo_raw_header frame = { .signature = 0, .width = 1, .height = 1 };
	CCD_IMAGE_ITEM->blob.value = &frame;
	CCD_IMAGE_ITEM->blob.size = sizeof(frame);
	strcpy(CCD_IMAGE_ITEM->blob.format, ".raw");
	CCD_IMAGE_PROPERTY->state = INDIGO_OK_STATE;
	indigo_update_property(device, CCD_IMAGE_PROPERTY, NULL);
	CCD_IMAGE_ITEM->blob.value = NULL;
	CCD_EXPOSURE_PROPERTY->state = INDIGO_OK_STATE;
	indigo_update_property(device, CCD_EXPOSURE_PROPERTY, NULL);
}

static indigo_result invalid_image_camera(indigo_device *device, indigo_client *sender, indigo_property *property) {
	if (!strcmp(property->name, "CCD_EXPOSURE")) {
		indigo_property_copy_values(CCD_EXPOSURE_PROPERTY, property, false);
		CCD_EXPOSURE_PROPERTY->state = INDIGO_BUSY_STATE;
		indigo_update_property(device, CCD_EXPOSURE_PROPERTY, NULL);
		indigo_set_timer(device, 0.05, invalid_frame, NULL);
		return INDIGO_OK;
	}
	return camera_spy(device, sender, property);
}

static void invalid_image_detection_and_recovery(void) {
	ASSERT_TRUE(connect_camera());
	camera_device->change_property = invalid_image_camera;
	unsigned started = revision(AGENT, "AGENT_IMAGER_CAPTURE");
	indigo_change_number_property_1(&client, AGENT, "AGENT_IMAGER_CAPTURE", "CAPTURE", 0.1);
	ASSERT_TRUE(wait_state(AGENT, "AGENT_IMAGER_CAPTURE", started, INDIGO_ALERT_STATE));
	ASSERT_TRUE(sw(AGENT, "AGENT_IMAGER_STARS", "REFRESH", true, INDIGO_ALERT_STATE));
	camera_device->change_property = camera_spy;
	ASSERT_TRUE(run("PREVIEW_1", INDIGO_OK_STATE));
}

static bool wait_number(const char *device, const char *property, const char *item, double expected) {
	double deadline = indigo_monotonic_time() + 10;
	while (indigo_monotonic_time() < deadline) {
		if (value(device, property, item) == expected) {
			return true;
		}
		indigo_usleep(1000);
	}
	return false;
}

static void interframe_delay_abort(void) {
	ASSERT_TRUE(connect_camera());
	ASSERT_TRUE(batch(3));
	ASSERT_TRUE(num(AGENT, "AGENT_IMAGER_BATCH", "DELAY", 3));
	ASSERT_TRUE(run("EXPOSURE", INDIGO_BUSY_STATE));
	ASSERT_TRUE(wait_number(AGENT, "AGENT_IMAGER_STATS", "PHASE", INDIGO_IMAGER_PHASE_WAITING));
	ASSERT_EQ_INT(1, blobs(CAMERA));
	ASSERT_TRUE(abort_running());
	ASSERT_EQ_INT(1, blobs(CAMERA));
	ASSERT_TRUE(num(AGENT, "AGENT_IMAGER_BATCH", "DELAY", 0));
	ASSERT_TRUE(run("PREVIEW_1", INDIGO_OK_STATE));
}

static void dither_disabled_dark_and_missing_guider(void) {
	ASSERT_TRUE(connect_camera());
	ASSERT_TRUE(batch(2));
	ASSERT_TRUE(sw(AGENT, "AGENT_PROCESS_FEATURES", "ENABLE_DITHERING", true, INDIGO_OK_STATE));
	ASSERT_TRUE(run("EXPOSURE", INDIGO_OK_STATE));
	ASSERT_TRUE(attach_peer(0, "Guider Agent Test", "AGENT_GUIDER_DITHER", "TRIGGER"));
	ASSERT_TRUE(sw(AGENT, "CCD_FRAME_TYPE", "DARK", true, INDIGO_OK_STATE));
	ASSERT_TRUE(run("EXPOSURE", INDIGO_OK_STATE));
	ASSERT_EQ_INT(0, atomic_load(&peers[0].requests));
	ASSERT_TRUE(sw(AGENT, "CCD_FRAME_TYPE", "LIGHT", true, INDIGO_OK_STATE));
	ASSERT_TRUE(sw(AGENT, "AGENT_PROCESS_FEATURES", "ENABLE_DITHERING", false, INDIGO_OK_STATE));
	ASSERT_TRUE(run("EXPOSURE", INDIGO_OK_STATE));
	ASSERT_EQ_INT(0, atomic_load(&peers[0].requests));
}

static void autofocus_failure_policies_and_repeat(void) {
	ASSERT_TRUE(connect_camera());
	ASSERT_TRUE(connect_focuser());
	ASSERT_TRUE(sw(AGENT, "AGENT_IMAGER_FOCUS_ESTIMATOR", "HFD_PEAK", true, INDIGO_OK_STATE));
	const char *policies[] = { "STOP", "RESTORE" };
	for (int i = 0; i < ARRAY_SIZE(policies); i++) {
		ASSERT_TRUE(sw(AGENT, "AGENT_IMAGER_FOCUS_FAILURE", policies[i], true, INDIGO_OK_STATE));
		atomic_store(&exposure_failures, 3);
		ASSERT_TRUE(run("FOCUSING", INDIGO_ALERT_STATE));
		ASSERT_TRUE(wait_state(FOCUSER, "FOCUSER_STEPS", 0, -1));
	}
	ASSERT_TRUE(num(AGENT, "AGENT_IMAGER_FOCUS", "REPEAT", 1));
	ASSERT_TRUE(num(AGENT, "AGENT_IMAGER_FOCUS", "DELAY", 0));
	atomic_store(&exposure_failures, 3);
	ASSERT_TRUE(run("FOCUSING", INDIGO_OK_STATE));
	ASSERT_EQ_INT(0, atomic_load(&exposure_failures));
}

static void default_preview_estimator(void) {
	ASSERT_TRUE(connect_camera());
	ASSERT_EQ_INT(1, value(AGENT, "AGENT_IMAGER_FOCUS_ESTIMATOR", "U_CURVE"));
	ASSERT_TRUE(run("PREVIEW_1", INDIGO_OK_STATE));
	ASSERT_TRUE(value(AGENT, "AGENT_IMAGER_STATS", "HFD") > 0);
}

static const indigo_test_case tests[] = {
	{ "default preview estimator", default_preview_estimator },
	{ "interframe delay abort", interframe_delay_abort },
	{ "dither disabled dark and missing guider", dither_disabled_dark_and_missing_guider },
	{ "autofocus failure policies and repeat", autofocus_failure_policies_and_repeat },
	{ "external shutter routing", external_shutter_routing },
	{ "streaming failure propagates", streaming_failure_propagates },
	{ "invalid image detection and recovery", invalid_image_detection_and_recovery },
	{ "independent instances and reselection", independent_instances_and_reselection },
	{ "additional instance lifecycle", additional_instance_lifecycle },
	{ "bahtinov preview and focus", bahtinov_preview_and_focus },
	{ "configuration reload", configuration_reload },
	{ "shutdown while paused", shutdown_while_paused },
	{ "camera disconnect and recover", camera_disconnect_and_recover },
	{ "selection regions binning and subframe", selection_regions_binning_and_subframe },
	{ "local batch and fits headers", local_batch_and_fits_headers },

	{ "dither cadence failure and abort", dither_cadence_failure_and_abort },
	{ "mount transit and solver coordination", mount_transit_and_solver_coordination },
	{ "additional instances and barrier", additional_instances_and_barrier },
	{ "settings selection estimator and reset", settings_selection_and_reset },
	{ "stars statistics and format restoration", stars_statistics_and_format_restoration },
	{ "breakpoint PRE_BATCH resume and abort", breakpoint_pre_batch },
	{ "breakpoint PRE_CAPTURE resume and abort", breakpoint_pre_capture },
	{ "breakpoint POST_CAPTURE resume and abort", breakpoint_post_capture },
	{ "breakpoint PRE_DELAY resume and abort", breakpoint_pre_delay },
	{ "breakpoint POST_DELAY resume and abort", breakpoint_post_delay },
	{ "breakpoint POST_BATCH resume and abort", breakpoint_post_batch },

	{ "pause resume and busy guards", pause_resume_and_busy_guards },
	{ "wheel offsets and manual focus", wheel_offsets_and_manual_focus },
	{ "bracketing returns to start", bracketing_returns_to_start },
	{ "autofocus estimators", autofocus_estimators },
	{ "autofocus abort and failure", autofocus_abort_and_failure },
	{ "download listing payload delete", download_listing_payload_delete },

	{ "metadata and missing devices", metadata_and_missing_devices },
	{ "finite capture preview batch and streaming", finite_acquisitions },
	{ "abort all capture modes and reacquire", abort_all_capture_modes },
	{ "exposure failure retry exhaustion and recovery", exposure_failure_retry_and_recovery }
};

int main(int argc, char **argv) {
	setvbuf(stdout, NULL, _IONBF, 0);
	int result = 0, executed = 0, passed = 0;
	for (int i = 0; i < ARRAY_SIZE(tests); i++) {
		if (argc > 1 && !strstr(tests[i].name, argv[1])) {
			continue;
		}
		executed++;
		strcpy(config_folder, "/tmp/indigo_imager_test_XXXXXX");
		if (!mkdtemp(config_folder)) {
			perror("mkdtemp");
			return 1;
		}
		pid_t child = fork();
		if (child == 0) {
			alarm(180);
			bool ready = setup();
			int status = ready ? indigo_run_tests("Imager Agent integration", tests + i, 1) : 1;
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
	printf("Imager Agent: %d/%d cases passed (including cleanup)\n", passed, executed);
	return executed ? result : 2;
}
