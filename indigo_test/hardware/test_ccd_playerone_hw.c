// Copyright (c) 2026 CloudMakers, s. r. o.
// All rights reserved.
// Use under the INDIGO Astronomy open-source license (see LICENSE.md).
// Hardware test implemented by OpenAI Codex.

#include <pthread.h>
#include <stdlib.h>
#include <dirent.h>
#include <unistd.h>
#include <time.h>
#ifdef POA_DYNAMIC_DRIVER
#include <dlfcn.h>
#endif
#include <indigo/indigo_driver.h>
#include <indigo_drivers/ccd_playerone/indigo_ccd_playerone.h>
#include "../test_runner.h"

#define ARRAY_SIZE(a) (sizeof(a) / sizeof((a)[0]))
#define TEST_SUFFIX "INDIGOTEST123456"
_Static_assert(sizeof(TEST_SUFFIX) - 1 == 16, "Hardware suffix fixture must contain exactly 16 bytes");
#define MAX_DEVICES 16
#define MAX_PROPERTIES 128
#define CHECK(condition) do { if (!(condition)) { fprintf(stderr, "%s:%d: %s\n", __FILE__, __LINE__, #condition); indigo_test_failures++; goto cleanup; } } while (0)

typedef struct {
	char name[INDIGO_NAME_SIZE];
	indigo_device *device, *master;
	unsigned interface;
	bool present;
	indigo_property *properties[MAX_PROPERTIES];
	unsigned revisions[MAX_PROPERTIES];
	unsigned frames, invalid_frames, width, height, bytes;
} observed_device;

static observed_device devices[MAX_DEVICES];
static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
static int camera = -1, guider = -1;
static bool hotplug, acceptance, suffix_only;
static char config_folder[] = "/tmp/indigo_playerone_hw_XXXXXX";

const char *poa_test_config_folder(void) {
	return config_folder;
}
#ifdef POA_DYNAMIC_DRIVER
static indigo_result (*driver_entry)(indigo_driver_action, indigo_driver_info *);
static void *driver_library;

static bool load_driver_library(void) {
	driver_library = dlopen(POA_DRIVER_PATH, RTLD_NOW | RTLD_LOCAL);
	if (!driver_library) {
		fprintf(stderr, "dlopen: %s\n", dlerror());
		return false;
	}
	driver_entry = dlsym(driver_library, "indigo_ccd_playerone");
	return driver_entry != NULL;
}
#else
static indigo_result (*driver_entry)(indigo_driver_action, indigo_driver_info *) = indigo_ccd_playerone;
#endif
static indigo_client client;

static int slot(int d, const char *name) {
	for (int p = 0; p < MAX_PROPERTIES; p++) {
		if (devices[d].properties[p] && !strcmp(devices[d].properties[p]->name, name)) {
			return p;
		}
	}
	return -1;
}

static indigo_result observe(indigo_device *device, indigo_property *property, const char *message, bool image_update) {
	pthread_mutex_lock(&mutex);
	int d;
	for (d = 0; d < MAX_DEVICES; d++) {
		if (!*devices[d].name || !strcmp(devices[d].name, property->device)) {
			break;
		}
	}
	if (d < MAX_DEVICES) {
		snprintf(devices[d].name, INDIGO_NAME_SIZE, "%s", property->device);
		devices[d].device = device;
		devices[d].master = device->master_device;
		if (!image_update && !strcmp(property->name, "INFO")) {
			devices[d].present = true;
			for (int i = 0; i < property->count; i++) {
				if (!strcmp(property->items[i].name, "DEVICE_INTERFACE")) {
					devices[d].interface = strtoul(property->items[i].text.value, NULL, 10);
				}
			}
		}
		int p = slot(d, property->name);
		if (p < 0) {
			for (p = 0; p < MAX_PROPERTIES && devices[d].properties[p]; p++) { }
		}
		if (p < MAX_PROPERTIES) {
			indigo_release_property(devices[d].properties[p]);
			devices[d].properties[p] = indigo_copy_property(NULL, property);
			devices[d].revisions[p]++;
		}
		if (image_update && !strcmp(property->name, "CCD_IMAGE") && property->state == INDIGO_OK_STATE && property->count && property->items[0].blob.value) {
			indigo_item *item = property->items;
			indigo_raw_header header = { 0 };
			if (item->blob.size >= sizeof(header)) {
				memcpy(&header, item->blob.value, sizeof(header));
			}
			unsigned bytes = header.signature == INDIGO_RAW_MONO8 ? 1 : (header.signature == INDIGO_RAW_MONO16 ? 2 : (header.signature == INDIGO_RAW_RGB24 ? 3 : (header.signature == INDIGO_RAW_RGB48 ? 6 : 0)));
			if (strcmp(item->blob.format, ".raw") || !bytes || !header.width || !header.height || (uint64_t)header.width * header.height * bytes + sizeof(header) > item->blob.size) {
				devices[d].invalid_frames++;
			}
			devices[d].width = header.width;
			devices[d].height = header.height;
			devices[d].bytes = bytes;
			devices[d].frames++;
			if (devices[d].frames <= 12 || devices[d].frames % 100 == 0) {
				printf("    frame %u: %u x %u, %ld bytes\n", devices[d].frames, header.width, header.height, item->blob.size);
			}
		}
	}
	pthread_mutex_unlock(&mutex);
	if (message && *message) {
		fprintf(stderr, "    %s %s: %s\n", property->device, property->name, message);
	}
	return INDIGO_OK;
}

static indigo_result define_property(indigo_client *client, indigo_device *device, indigo_property *property, const char *message) {
	return observe(device, property, message, false);
}

static indigo_result update_property(indigo_client *client, indigo_device *device, indigo_property *property, const char *message) {
	return observe(device, property, message, true);
}

static indigo_result report_message(indigo_client *client, indigo_device *device, indigo_property *property, const char *message) {
	fprintf(stderr, "    message: %s\n", message ? message : "");
	return INDIGO_OK;
}

static indigo_result delete_property(indigo_client *client, indigo_device *device, indigo_property *property, const char *message) {
	if (!*property->name) {
		pthread_mutex_lock(&mutex);
		for (int d = 0; d < MAX_DEVICES; d++) {
			if (!strcmp(devices[d].name, property->device)) {
				devices[d].present = false;
				for (int p = 0; p < MAX_PROPERTIES; p++) {
					indigo_release_property(devices[d].properties[p]);
					devices[d].properties[p] = NULL;
				}
			}
		}
		pthread_mutex_unlock(&mutex);
	}
	return INDIGO_OK;
}

static bool wait_presence(bool present) {
	for (int i = 0; i < 18000; i++) {
		pthread_mutex_lock(&mutex);
		if (present && !devices[camera].present) {
			for (int d = 0; d < MAX_DEVICES; d++) {
				if (devices[d].present && (devices[d].interface & INDIGO_INTERFACE_CCD)) {
					camera = d;
					guider = -1;
					for (int g = 0; g < MAX_DEVICES; g++) {
						if (devices[g].present && devices[g].master == devices[d].device && (devices[g].interface & INDIGO_INTERFACE_GUIDER)) {
							guider = g;
						}
					}
					break;
				}
			}
		}
		bool ready = devices[camera].present == present && (guider < 0 || devices[guider].present == present);
		if (present) {
			ready = ready && slot(camera, "CONNECTION") >= 0 && (guider < 0 || slot(guider, "CONNECTION") >= 0);
		}
		pthread_mutex_unlock(&mutex);
		if (ready) {
			return true;
		}
		indigo_usleep(10000);
	}
	fprintf(stderr, "Timed out waiting for physical %s\n", present ? "replug" : "unplug");
	return false;
}

static bool disconnect_device(int d) {
	if (d < 0) {
		return true;
	}
	pthread_mutex_lock(&mutex);
	bool present = devices[d].present;
	pthread_mutex_unlock(&mutex);
	if (!present) {
		return true;
	}
	indigo_change_switch_property_1(&client, devices[d].name, "CONNECTION", "DISCONNECTED", true);
	for (int i = 0; i < 3000; i++) {
		pthread_mutex_lock(&mutex);
		int p = slot(d, "CONNECTION");
		bool disconnected = false;
		if (p >= 0 && devices[d].properties[p]->state == INDIGO_OK_STATE) {
			indigo_property *property = devices[d].properties[p];
			for (int j = 0; j < property->count; j++) {
				if (!strcmp(property->items[j].name, "DISCONNECTED")) {
					disconnected = property->items[j].sw.value;
				}
			}
		}
		pthread_mutex_unlock(&mutex);
		if (disconnected) {
			return true;
		}
		indigo_usleep(10000);
	}
	fprintf(stderr, "Disconnect timeout: %s\n", devices[d].name);
	return false;
}

static unsigned revision(int d, const char *name) {
	pthread_mutex_lock(&mutex);
	int p = slot(d, name);
	unsigned result = p < 0 ? 0 : devices[d].revisions[p];
	pthread_mutex_unlock(&mutex);
	return result;
}

static bool wait_state(int d, const char *name, unsigned after, indigo_property_state state) {
	for (int i = 0; i < 3000; i++) {
		pthread_mutex_lock(&mutex);
		int p = slot(d, name);
		bool ready = p >= 0 && devices[d].revisions[p] > after && devices[d].properties[p]->state == state;
		pthread_mutex_unlock(&mutex);
		if (ready) {
			return true;
		}
		indigo_usleep(10000);
	}
	fprintf(stderr, "Timeout: %s %s expected state %d\n", devices[d].name, name, state);
	return false;
}

static bool switch_value(int d, const char *name, const char *item, indigo_property_state state) {
	if (d < 0) {
		return true;
	}
	unsigned before = revision(d, name);
	indigo_change_switch_property_1(&client, devices[d].name, name, item, true);
	return wait_state(d, name, before, state);
}

static bool number_value(int d, const char *name, const char *item, double value, indigo_property_state state) {
	if (d < 0) {
		return true;
	}
	unsigned before = revision(d, name);
	indigo_change_number_property_1(&client, devices[d].name, name, item, value);
	return wait_state(d, name, before, state);
}

static unsigned frames(void) {
	pthread_mutex_lock(&mutex);
	unsigned result = devices[camera].frames;
	pthread_mutex_unlock(&mutex);
	return result;
}

static indigo_property *snapshot(int d, const char *name) {
	pthread_mutex_lock(&mutex);
	int p = slot(d, name);
	indigo_property *copy = p < 0 ? NULL : indigo_copy_property(NULL, devices[d].properties[p]);
	pthread_mutex_unlock(&mutex);
	return copy;
}

static bool restore_property(indigo_property *p) {
	if (!p) {
		return true;
	}
	unsigned before = revision(camera, p->name);
	return indigo_change_property(&client, p) == INDIGO_OK && wait_state(camera, p->name, before, INDIGO_OK_STATE);
}

static bool format_geometry_and_controls(void) {
	indigo_property *pixel = snapshot(camera, "X_PIXEL_FORMAT");
	indigo_property *frame = snapshot(camera, "CCD_FRAME");
	indigo_property *bin = snapshot(camera, "CCD_BIN");
	indigo_property *gain = snapshot(camera, "CCD_GAIN");
	indigo_property *offset = snapshot(camera, "CCD_OFFSET");
	indigo_property *type = snapshot(camera, "CCD_FRAME_TYPE");
	bool ok = pixel && frame && bin;
	for (int i = 0; ok && i < pixel->count; i++) {
		printf("    hardware pixel format: %s\n", pixel->items[i].name);
		unsigned before = frames();
		ok = switch_value(camera, "X_PIXEL_FORMAT", pixel->items[i].name, INDIGO_OK_STATE) && number_value(camera, "CCD_EXPOSURE", "EXPOSURE", 0.1, INDIGO_OK_STATE) && frames() == before + 1;
	}
	if (ok) {
		ok = switch_value(camera, "X_PIXEL_FORMAT", pixel->items[0].name, INDIGO_OK_STATE);
	}
	for (int factor = 1; ok && factor <= 2 && factor <= bin->items[0].number.max; factor++) {
		unsigned before = revision(camera, "CCD_BIN");
		indigo_change_number_property(&client, devices[camera].name, "CCD_BIN", 2, (const char *[]){ "HORIZONTAL", "VERTICAL" }, (double []){ factor, factor });
		ok = wait_state(camera, "CCD_BIN", before, INDIGO_OK_STATE);
		before = revision(camera, "CCD_FRAME");
		indigo_change_number_property(&client, devices[camera].name, "CCD_FRAME", 4, (const char *[]){ "LEFT", "TOP", "WIDTH", "HEIGHT" }, (double []){ 16, 24, 256, 256 });
		ok = ok && wait_state(camera, "CCD_FRAME", before, INDIGO_OK_STATE) && number_value(camera, "CCD_EXPOSURE", "EXPOSURE", 0.1, INDIGO_OK_STATE);
		pthread_mutex_lock(&mutex);
		ok = ok && devices[camera].width == 256 / factor && devices[camera].height == 256 / factor;
		pthread_mutex_unlock(&mutex);
		printf("    hardware ROI 256x256 at 16,24, bin %d: %s\n", factor, ok ? "PASS" : "FAIL");
	}
#ifndef POA_DYNAMIC_DRIVER
	if (ok && gain && gain->perm == INDIGO_RW_PERM) {
		double value = gain->items[0].number.value;
		double changed = value < gain->items[0].number.max ? value + 1 : value - 1;
		ok = number_value(camera, "CCD_GAIN", "GAIN", changed, INDIGO_OK_STATE) && switch_value(camera, "CONFIG", "SAVE", INDIGO_OK_STATE) && restore_property(gain) && switch_value(camera, "CONFIG", "LOAD", INDIGO_OK_STATE);
		indigo_property *loaded = snapshot(camera, "CCD_GAIN");
		ok = ok && loaded && loaded->items[0].number.value == changed;
		indigo_release_property(loaded);
		printf("    hardware gain/config roundtrip in temporary directory: %s\n", ok ? "PASS" : "FAIL");
	}
#endif
	if (ok && offset && offset->perm == INDIGO_RW_PERM) {
		ok = number_value(camera, "CCD_OFFSET", "OFFSET", offset->items[0].number.value < offset->items[0].number.max ? offset->items[0].number.value + 1 : offset->items[0].number.value - 1, INDIGO_OK_STATE);
	}
	if (ok) {
		ok = switch_value(camera, "CCD_FRAME_TYPE", "DARK", INDIGO_OK_STATE) && number_value(camera, "CCD_EXPOSURE", "EXPOSURE", 0.1, INDIGO_OK_STATE) && switch_value(camera, "CCD_FRAME_TYPE", "LIGHT", INDIGO_OK_STATE);
	}
	if (ok) {
		printf("    completed long exposure with concurrent guider commands\n");
		unsigned before = revision(camera, "CCD_EXPOSURE");
		indigo_change_number_property_1(&client, devices[camera].name, "CCD_EXPOSURE", "EXPOSURE", 5);
		ok = wait_state(camera, "CCD_EXPOSURE", before, INDIGO_BUSY_STATE) && number_value(guider, "GUIDER_GUIDE_RA", "EAST", 100, INDIGO_OK_STATE) && number_value(guider, "GUIDER_GUIDE_DEC", "NORTH", 100, INDIGO_OK_STATE) && wait_state(camera, "CCD_EXPOSURE", before, INDIGO_OK_STATE);
	}
	indigo_property *restore[] = { bin, frame, pixel, gain, offset, type };
	for (int i = 0; i < ARRAY_SIZE(restore); i++) {
		if (!restore_property(restore[i])) {
			ok = false;
		}
		indigo_release_property(restore[i]);
	}
	return ok;
}

static bool physical_cycle(const char *phase) {
	printf("HOTPLUG_READY [%s]: unplug USB camera now: %s\n", phase, devices[camera].name);
	if (!wait_presence(false)) {
		return false;
	}
	printf("HOTPLUG_REMOVED [%s]: reconnect the same USB camera now\n", phase);
	if (!wait_presence(true) || !switch_value(camera, "CONNECTION", "CONNECTED", INDIGO_OK_STATE) || !switch_value(guider, "CONNECTION", "CONNECTED", INDIGO_OK_STATE) || !switch_value(camera, "CCD_UPLOAD_MODE", "CLIENT", INDIGO_OK_STATE) || !switch_value(camera, "CCD_IMAGE_FORMAT", "RAW", INDIGO_OK_STATE)) {
		return false;
	}
	unsigned before = frames();
	bool ok = number_value(camera, "CCD_EXPOSURE", "EXPOSURE", 0.1, INDIGO_OK_STATE) && frames() == before + 1 && number_value(guider, "GUIDER_GUIDE_RA", "EAST", 100, INDIGO_OK_STATE);
	printf("HOTPLUG_RECOVERED [%s]: %s, camera name: %s\n", phase, ok ? "PASS" : "FAIL", devices[camera].name);
	return ok;
}

static bool set_suffix(const char *value) {
	unsigned before = revision(camera, "X_CUSTOM_SUFFIX");
	printf("    Request suffix %s, revision %u\n", value, before);
	indigo_change_text_property_1(&client, devices[camera].name, "X_CUSTOM_SUFFIX", "SUFFIX", value);
	bool ok = wait_state(camera, "X_CUSTOM_SUFFIX", before, INDIGO_OK_STATE);
	indigo_property *p = snapshot(camera, "X_CUSTOM_SUFFIX");
	printf("    Suffix result: state %d, revision %u, value %s\n", p ? p->state : -1, revision(camera, "X_CUSTOM_SUFFIX"), p ? p->items[0].text.value : "missing");
	indigo_release_property(p);
	return ok;
}

static bool suffix_acceptance(void) {
	indigo_property *suffix = snapshot(camera, "X_CUSTOM_SUFFIX");
	if (!suffix || !suffix->count) {
		indigo_release_property(suffix);
		return false;
	}
	char original[17];
	snprintf(original, sizeof(original), "%s", suffix->items[0].text.value);
	indigo_release_property(suffix);
	printf("    original suffix: '%s'\n", original);
	bool ok = set_suffix(TEST_SUFFIX) && physical_cycle("16-byte suffix") && strstr(devices[camera].name, "#INDIGOTEST123456") != NULL;
	if (ok) {
		ok = set_suffix("") && physical_cycle("cleared suffix") && strstr(devices[camera].name, "#INDIGOTEST123456") == NULL;
	}
	// Always restore after the first flash write, including failed assertions.
	bool restored = set_suffix(original);
	if (restored && *original) {
		restored = physical_cycle("restored suffix") && strstr(devices[camera].name, original) != NULL;
	}
	printf("    original suffix restoration: %s\n", restored ? "PASS" : "FAIL");
	return ok && restored;
}

static bool complete_physical_acceptance(void) {
	if (!physical_cycle("idle")) {
		return false;
	}
	unsigned before = revision(camera, "CCD_EXPOSURE");
	indigo_change_number_property_1(&client, devices[camera].name, "CCD_EXPOSURE", "EXPOSURE", 120);
	if (!wait_state(camera, "CCD_EXPOSURE", before, INDIGO_BUSY_STATE) || !physical_cycle("long exposure")) {
		return false;
	}
	if (guider >= 0) {
		before = revision(guider, "GUIDER_GUIDE_RA");
		indigo_change_number_property_1(&client, devices[guider].name, "GUIDER_GUIDE_RA", "EAST", 60000);
		if (!wait_state(guider, "GUIDER_GUIDE_RA", before, INDIGO_BUSY_STATE) || !physical_cycle("guiding")) {
			return false;
		}
	}
	return suffix_acceptance();
}

static void hardware_workflows(void) {
	bool initialized = false;
	CHECK(indigo_start() == INDIGO_OK);
	CHECK(indigo_attach_client(&client) == INDIGO_OK);
	CHECK(driver_entry(INDIGO_DRIVER_INIT, NULL) == INDIGO_OK);
	initialized = true;
	// Let initial USB enumeration settle before selecting a unique camera.
	for (int i = 0; i < 500; i++) {
		indigo_usleep(10000);
	}
	const char *requested = getenv("INDIGO_TEST_DEVICE");
	int matches = 0;
	pthread_mutex_lock(&mutex);
	for (int d = 0; d < MAX_DEVICES; d++) {
		if (devices[d].interface & INDIGO_INTERFACE_CCD) {
			printf("    discovered camera: %s\n", devices[d].name);
			if (!requested || !strcmp(requested, devices[d].name)) {
				camera = d;
				matches++;
			}
		}
	}
	if (matches == 1) {
		for (int d = 0; d < MAX_DEVICES; d++) {
			if ((devices[d].interface & INDIGO_INTERFACE_GUIDER) && devices[d].master == devices[camera].device) {
				guider = d;
			}
		}
	}
	pthread_mutex_unlock(&mutex);
	if (matches != 1) {
		fprintf(stderr, "Expected one camera; found %d matches. Set INDIGO_TEST_DEVICE to an exact discovered name when needed.\n", matches);
		camera = -1;
	}
	CHECK(camera >= 0);
	printf("    selected: %s, guider: %s\n", devices[camera].name, guider < 0 ? "not available (SKIP guider workflows)" : devices[guider].name);
	CHECK(switch_value(guider, "CONNECTION", "CONNECTED", INDIGO_OK_STATE));
	CHECK(switch_value(camera, "CONNECTION", "CONNECTED", INDIGO_OK_STATE));
	CHECK(switch_value(camera, "CCD_UPLOAD_MODE", "CLIENT", INDIGO_OK_STATE));
	CHECK(switch_value(camera, "CCD_IMAGE_FORMAT", "RAW", INDIGO_OK_STATE));
	if (suffix_only) {
		CHECK(suffix_acceptance());
		goto cleanup;
	}
	CHECK(format_geometry_and_controls());
	for (int i = 0; i < 3; i++) {
		unsigned before = frames();
		CHECK(number_value(camera, "CCD_EXPOSURE", "EXPOSURE", 0.1, INDIGO_OK_STATE));
		CHECK(frames() == before + 1);
	}
	CHECK(number_value(camera, "CCD_EXPOSURE", "EXPOSURE", 5, INDIGO_BUSY_STATE));
	// Allow delayed SDK setup to start the physical exposure before aborting it.
	indigo_usleep(500000);
	CHECK(switch_value(camera, "CCD_ABORT_EXPOSURE", "ABORT_EXPOSURE", INDIGO_OK_STATE));
	CHECK(wait_state(camera, "CCD_EXPOSURE", 0, INDIGO_ALERT_STATE));
	unsigned before = frames(), stream_revision = revision(camera, "CCD_STREAMING");
	indigo_change_number_property(&client, devices[camera].name, "CCD_STREAMING", 2, (const char *[]){ "EXPOSURE", "COUNT" }, (double []){ 0.1, 5 });
	CHECK(wait_state(camera, "CCD_STREAMING", stream_revision, INDIGO_OK_STATE));
	CHECK(frames() == before + 5);
	stream_revision = revision(camera, "CCD_STREAMING");
	before = frames();
	indigo_change_number_property(&client, devices[camera].name, "CCD_STREAMING", 2, (const char *[]){ "EXPOSURE", "COUNT" }, (double []){ 0.1, -1 });
	CHECK(wait_state(camera, "CCD_STREAMING", stream_revision, INDIGO_BUSY_STATE));
	for (int i = 0; i < 1000; i++) {
		indigo_usleep(10000);
	}
	CHECK(frames() >= before + 3);
	printf("    sustained stream: %u frames in approximately 10 seconds\n", frames() - before);
	CHECK(switch_value(camera, "CCD_ABORT_EXPOSURE", "ABORT_EXPOSURE", INDIGO_OK_STATE));
	CHECK(wait_state(camera, "CCD_STREAMING", stream_revision, INDIGO_OK_STATE));
	const char *axes[] = { "GUIDER_GUIDE_RA", "GUIDER_GUIDE_RA", "GUIDER_GUIDE_DEC", "GUIDER_GUIDE_DEC" };
	const char *directions[] = { "EAST", "WEST", "NORTH", "SOUTH" };
	for (int i = 0; i < 4; i++) {
		CHECK(number_value(guider, axes[i], directions[i], 100, INDIGO_OK_STATE));
	}
	CHECK(switch_value(camera, "CONNECTION", "DISCONNECTED", INDIGO_OK_STATE));
	CHECK(number_value(guider, "GUIDER_GUIDE_RA", "EAST", 100, INDIGO_OK_STATE));
	CHECK(switch_value(guider, "CONNECTION", "DISCONNECTED", INDIGO_OK_STATE));
	CHECK(switch_value(camera, "CONNECTION", "CONNECTED", INDIGO_OK_STATE));
	CHECK(switch_value(guider, "CONNECTION", "CONNECTED", INDIGO_OK_STATE));
	before = frames();
	CHECK(number_value(camera, "CCD_EXPOSURE", "EXPOSURE", 0.1, INDIGO_OK_STATE));
	CHECK(frames() == before + 1);
	if (hotplug) {
		before = frames();
		stream_revision = revision(camera, "CCD_STREAMING");
		indigo_change_number_property(&client, devices[camera].name, "CCD_STREAMING", 2, (const char *[]){ "EXPOSURE", "COUNT" }, (double []){ 0.1, -1 });
		CHECK(wait_state(camera, "CCD_STREAMING", stream_revision, INDIGO_BUSY_STATE));
		for (int i = 0; i < 3000 && frames() < before + 3; i++) {
			indigo_usleep(10000);
		}
		CHECK(frames() >= before + 3);
		printf("HOTPLUG_READY: unplug USB camera now: %s\n", devices[camera].name);
		CHECK(wait_presence(false));
		printf("HOTPLUG_REMOVED: reconnect the same USB camera now\n");
		CHECK(wait_presence(true));
		CHECK(switch_value(camera, "CONNECTION", "CONNECTED", INDIGO_OK_STATE));
		CHECK(switch_value(guider, "CONNECTION", "CONNECTED", INDIGO_OK_STATE));
		CHECK(switch_value(camera, "CCD_UPLOAD_MODE", "CLIENT", INDIGO_OK_STATE));
		CHECK(switch_value(camera, "CCD_IMAGE_FORMAT", "RAW", INDIGO_OK_STATE));
		before = frames();
		CHECK(number_value(camera, "CCD_EXPOSURE", "EXPOSURE", 0.1, INDIGO_OK_STATE));
		CHECK(frames() == before + 1);
		CHECK(number_value(guider, "GUIDER_GUIDE_RA", "EAST", 100, INDIGO_OK_STATE));
		printf("HOTPLUG_RECOVERED: exposure and guide pulse succeeded after replug\n");
	}
	if (acceptance) {
		CHECK(complete_physical_acceptance());
	}
	CHECK(disconnect_device(camera));
	CHECK(disconnect_device(guider));
	CHECK(driver_entry(INDIGO_DRIVER_SHUTDOWN, NULL) == INDIGO_OK);
	initialized = false;
#ifdef POA_DYNAMIC_DRIVER
	CHECK(dlclose(driver_library) == 0);
	driver_library = NULL;
	driver_entry = NULL;
	CHECK(load_driver_library());
	printf("    Dynamic driver dlclose/dlopen completed\n");
#endif
	CHECK(driver_entry(INDIGO_DRIVER_INIT, NULL) == INDIGO_OK);
	initialized = true;
	CHECK(wait_presence(true));
	CHECK(switch_value(camera, "CONNECTION", "CONNECTED", INDIGO_OK_STATE));
	CHECK(switch_value(guider, "CONNECTION", "CONNECTED", INDIGO_OK_STATE));
	CHECK(switch_value(camera, "CCD_UPLOAD_MODE", "CLIENT", INDIGO_OK_STATE));
	CHECK(switch_value(camera, "CCD_IMAGE_FORMAT", "RAW", INDIGO_OK_STATE));
	before = frames();
	CHECK(number_value(camera, "CCD_EXPOSURE", "EXPOSURE", 0.1, INDIGO_OK_STATE));
	CHECK(frames() == before + 1);
	#ifdef POA_DYNAMIC_DRIVER
	printf("    Fresh exposure after dynamic driver reload: PASS\n");
#else
	printf("    Driver shutdown/reinitialization: fresh queue and exposure succeeded (SDK library remains loaded)\n");
#endif
	pthread_mutex_lock(&mutex);
	unsigned invalid = devices[camera].invalid_frames;
	pthread_mutex_unlock(&mutex);
	CHECK(invalid == 0);
cleanup:
	if (camera >= 0) {
		// Abort any unfinished acquisition before disconnecting; disconnected properties may be unchanged.
		indigo_change_switch_property_1(&client, devices[camera].name, "CCD_ABORT_EXPOSURE", "ABORT_EXPOSURE", true);
		if (!disconnect_device(camera)) { indigo_test_failures++; }
	}
	if (guider >= 0) {
		if (!disconnect_device(guider)) { indigo_test_failures++; }
	}
	if (initialized && driver_entry(INDIGO_DRIVER_SHUTDOWN, NULL) != INDIGO_OK) { indigo_test_failures++; }
	indigo_detach_client(&client);
	indigo_stop();
}

int main(int argc, char **argv) {
	if ((argc != 2 && argc != 3) || strcmp(argv[1], "--run") || (argc == 3 && strcmp(argv[2], "--hotplug") && strcmp(argv[2], "--acceptance") && strcmp(argv[2], "--suffix"))) {
		fprintf(stderr, "Physical camera test: run explicitly with --run (or make test-ccd-playerone-hw).\n");
		return 2;
	}
	suffix_only = argc == 3 && !strcmp(argv[2], "--suffix");
	hotplug = argc == 3 && !suffix_only;

	acceptance = argc == 3 && !strcmp(argv[2], "--acceptance");
	setvbuf(stdout, NULL, _IONBF, 0);
	client = (indigo_client){ .name = "Player One hardware test", .version = INDIGO_VERSION_CURRENT, .define_property = define_property, .update_property = update_property, .delete_property = delete_property, .send_message = report_message };
	if (!mkdtemp(config_folder)) {
		return 1;
	}
	#ifdef POA_DYNAMIC_DRIVER
	if (!load_driver_library()) {
		return 1;
	}
#endif
	const indigo_test_case tests[] = { { "Physical camera exposure, streaming, guider and reconnect", hardware_workflows } };
	int result = indigo_run_tests("Player One hardware", tests, 1);
	for (int d = 0; d < MAX_DEVICES; d++) {
		for (int p = 0; p < MAX_PROPERTIES; p++) {
			indigo_release_property(devices[d].properties[p]);
		}
	}
	DIR *dir = opendir(config_folder);
	if (dir) {
		struct dirent *entry;
		while ((entry = readdir(dir))) {
			if (entry->d_name[0] != '.') {
				char path[1024];
				snprintf(path, sizeof(path), "%s/%s", config_folder, entry->d_name);
				unlink(path);
			}
		}
		closedir(dir);
	}
	rmdir(config_folder);
#ifdef POA_DYNAMIC_DRIVER
	if (driver_library && dlclose(driver_library) != 0) {
		result = 1;
	}
#endif
	return result;
}
