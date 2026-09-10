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
#include <indigo_drivers/agent_guider/indigo_agent_guider.h>
#include <indigo_drivers/ccd_simulator/indigo_ccd_simulator.h>
#include "../test_runner.h"

#define AGENT "Guider Agent"
#define CAMERA CCD_SIMULATOR_GUIDER_CAMERA_NAME
#define GUIDER CCD_SIMULATOR_GUIDER_NAME
#define ARRAY_SIZE(a) (sizeof(a) / sizeof((a)[0]))
#define REQUIRE(c) do { if (!(c)) { fprintf(stderr, "%s:%d: %s\n", __FILE__, __LINE__, #c); indigo_test_failures++; return false; } } while (0)

typedef struct {
	indigo_property *property;
	unsigned revision, blobs;
} observation;
static observation cache[2048];
static pthread_mutex_t cache_mutex = PTHREAD_MUTEX_INITIALIZER;
static indigo_client client;
static char config_folder[] = "/tmp/indigo_guider_test_XXXXXX";
// Each case forks before starting INDIGO threads and isolates configuration writes.
static bool bus_started, client_attached, simulator_started, agent_started;
static atomic_int camera_requests;
static atomic_int camera_abort_requests;
static _Atomic(indigo_device *) camera_device, guider_device;
static indigo_result (*camera_change)(indigo_device *, indigo_client *, indigo_property *);
static atomic_int exposure_failures;

const char *guider_test_config_folder(void) {
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
			if (property->type == INDIGO_BLOB_VECTOR && property->state == INDIGO_OK_STATE && property->count && property->items[0].blob.value) {
				entry->blobs++;
			}
		}
	}
	if (!strcmp(property->device, GUIDER)) {
		guider_device = device;
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

static indigo_client client = { .name = "Guider integration client", .define_property = defined, .update_property = updated, .delete_property = deleted, .send_message = message };

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

// Synthetic frames and pulse observations replace only the camera/guider boundary.
// The production agent, bus, filter, image analysis and timers remain in use.
static atomic_bool synthetic, extended_image, blank_image, invalid_image, short_image, pulse_failure, freeze_motion;
enum { RAW_COMPLETE, RAW_ZERO_WIDTH, RAW_ZERO_HEIGHT, RAW_LARGE_DIMENSION, RAW_PIXEL_OVERFLOW, RAW_SHORT_PIXELS };
static atomic_uint raw_format;
static atomic_int raw_fault;
static atomic_int short_image_size = 4;
static indigo_timer *synthetic_timer, *failure_timer;
static indigo_result (*guider_change)(indigo_device *, indigo_client *, indigo_property *);
static pthread_mutex_t motion_mutex = PTHREAD_MUTEX_INITIALIZER;
static double offset_x, offset_y;
static double pulse_ra, pulse_dec, maximum_pulse;
static unsigned ra_commands, dec_commands;

static void synthetic_frame(indigo_device *device) {
	int bin = CCD_BIN_HORIZONTAL_ITEM->number.value;
	int width = CCD_FRAME_WIDTH_ITEM->number.value / bin, height = CCD_FRAME_HEIGHT_ITEM->number.value / bin;
	int left = CCD_FRAME_LEFT_ITEM->number.value, top = CCD_FRAME_TOP_ITEM->number.value;
	unsigned char *buffer = indigo_safe_malloc(FITS_HEADER_SIZE + width * height * 2);
	uint16_t *pixels = (uint16_t *)(buffer + FITS_HEADER_SIZE);
	pthread_mutex_lock(&motion_mutex);
	double dx = offset_x, dy = offset_y;
	pthread_mutex_unlock(&motion_mutex);
	bool blank = atomic_load(&blank_image);
	const double stars[][3] = { { 110, 100, 30000 }, { 210, 170, 23000 }, { 290, 90, 19000 }, { 80, 220, 14000 } };
	for (int y = 0; y < height; y++) {
		for (int x = 0; x < width; x++) {
			double v = 500 + (x * 17 + y * 13) % 3;
			if (extended_image && !blank) {
				double xx = x * bin + left - 200 - dx, yy = y * bin + top - 150 - dy;
				v += 30000 * exp(-(xx * xx + yy * yy) / 1800);
			}
			for (int i = 0; !blank && !extended_image && i < ARRAY_SIZE(stars); i++) {
				double xx = x * bin + left - stars[i][0] - dx, yy = y * bin + top - stars[i][1] - dy;
				v += stars[i][2] * exp(-(xx * xx + yy * yy) / 8);
			}
			pixels[y * width + x] = v;
		}
	}
	if (invalid_image || short_image) {
		indigo_raw_header header = { .signature = invalid_image ? 0 : INDIGO_RAW_MONO16, .width = width, .height = height };
		CCD_IMAGE_ITEM->blob.value = &header;
		CCD_IMAGE_ITEM->blob.size = short_image ? short_image_size : sizeof(header);
		strcpy(CCD_IMAGE_ITEM->blob.format, ".raw");
		CCD_IMAGE_PROPERTY->state = INDIGO_OK_STATE;
		indigo_update_property(device, CCD_IMAGE_PROPERTY, NULL);
		CCD_IMAGE_ITEM->blob.value = NULL;
		CCD_IMAGE_ITEM->blob.size = 0;
	} else if (raw_format) {
		unsigned format = raw_format;
		int bytes = format == INDIGO_RAW_MONO8 ? 1 : format == INDIGO_RAW_MONO16 ? 2 : format == INDIGO_RAW_RGB24 ? 3 : 6;
		size_t size = sizeof(indigo_raw_header) + (size_t)width * height * bytes;
		// Preserve a legitimate metadata trailer after complete mono images.
		const char trailer[] = "BAYERPAT=RGGB";
		bool bayer = raw_fault == RAW_COMPLETE && bytes <= 2;
		indigo_raw_header *raw = indigo_safe_malloc(size + sizeof(trailer));
		*raw = (indigo_raw_header){ .signature = format, .width = width, .height = height };
		unsigned char *data = (unsigned char *)(raw + 1);
		for (int i = 0; i < width * height; i++) {
			for (int channel = 0; channel < (bytes >= 3 ? 3 : 1); channel++) {
				if (bytes == 1 || bytes == 3) {
					data[i * bytes + channel] = pixels[i] >> 8;
				} else {
					((uint16_t *)data)[i * (bytes / 2) + channel] = pixels[i];
				}
			}
		}
		if (bayer) {
			memcpy((char *)raw + size, trailer, sizeof(trailer));
			size += sizeof(trailer);
		}
		switch (raw_fault) {
			case RAW_ZERO_WIDTH: raw->width = 0; break;
			case RAW_ZERO_HEIGHT: raw->height = 0; break;
			case RAW_LARGE_DIMENSION: raw->width = UINT32_MAX; break;
			case RAW_PIXEL_OVERFLOW: raw->width = raw->height = 65536; break;
			case RAW_SHORT_PIXELS: size--; break;
		}
		CCD_IMAGE_ITEM->blob.value = raw;
		CCD_IMAGE_ITEM->blob.size = size;
		strcpy(CCD_IMAGE_ITEM->blob.format, ".raw");
		CCD_IMAGE_PROPERTY->state = INDIGO_OK_STATE;
		indigo_update_property(device, CCD_IMAGE_PROPERTY, NULL);
		CCD_IMAGE_ITEM->blob.value = NULL;
		CCD_IMAGE_ITEM->blob.size = 0;
		indigo_safe_free(raw);
	} else {
		indigo_process_image(device, buffer, width, height, 16, true, true, NULL, false);
	}
	indigo_safe_free(buffer);
	CCD_EXPOSURE_ITEM->number.value = 0;
	CCD_EXPOSURE_PROPERTY->state = INDIGO_OK_STATE;
	indigo_update_property(device, CCD_EXPOSURE_PROPERTY, NULL);
}

static indigo_result guider_spy(indigo_device *device, indigo_client *sender, indigo_property *property) {
	if (!strcmp(property->name, "GUIDER_GUIDE_RA") || !strcmp(property->name, "GUIDER_GUIDE_DEC")) {
		double positive = 0, negative = 0;
		bool ra = !strcmp(property->name, "GUIDER_GUIDE_RA");
		for (int i = 0; i < property->count; i++) {
			if (!strcmp(property->items[i].name, ra ? "WEST" : "NORTH")) {
				positive = property->items[i].number.value;
			} else {
				negative = property->items[i].number.value;
			}
		}
		pthread_mutex_lock(&motion_mutex);
		maximum_pulse = fmax(maximum_pulse, fmax(positive, negative));
		if (ra) {
			ra_commands++;
			pulse_ra = positive - negative;
			if (synthetic && !freeze_motion && !pulse_failure) {
				offset_x += pulse_ra * 0.01;
			}
		} else {
			dec_commands++;
			pulse_dec = positive - negative;
			if (synthetic && !freeze_motion && !pulse_failure) {
				offset_y -= pulse_dec * 0.01;
			}
		}
		pthread_mutex_unlock(&motion_mutex);
		if (pulse_failure) {
			indigo_property *reply = indigo_copy_property(NULL, property);
			reply->state = INDIGO_ALERT_STATE;
			indigo_update_property(device, reply, "Injected guide pulse failure");
			indigo_release_property(reply);
			return INDIGO_OK;
		}
	}
	return guider_change(device, sender, property);
}

static void fail_exposure(indigo_device *device) {
	CCD_EXPOSURE_PROPERTY->state = INDIGO_ALERT_STATE;
	indigo_update_property(device, CCD_EXPOSURE_PROPERTY, "Injected exposure failure");
}

static indigo_result camera_spy(indigo_device *device, indigo_client *sender, indigo_property *property) {
	if (!strcmp(property->name, "CCD_ABORT_EXPOSURE")) {
		atomic_fetch_add(&camera_abort_requests, 1);
		if (synthetic) {
			indigo_cancel_timer_sync(device, &synthetic_timer);
			CCD_EXPOSURE_PROPERTY->state = INDIGO_ALERT_STATE;
			indigo_update_property(device, CCD_EXPOSURE_PROPERTY, NULL);
		}
	}
	if (!strcmp(property->name, "CCD_EXPOSURE")) {
		atomic_fetch_add(&camera_requests, 1);
		if (atomic_load(&exposure_failures) > 0) {
			atomic_fetch_sub(&exposure_failures, 1);
			indigo_property_copy_values(CCD_EXPOSURE_PROPERTY, property, false);
			CCD_EXPOSURE_PROPERTY->state = INDIGO_BUSY_STATE;
			indigo_update_property(device, CCD_EXPOSURE_PROPERTY, NULL);
			indigo_set_timer(device, 0.05, fail_exposure, &failure_timer);
			return INDIGO_OK;
		}
	}
	if (synthetic && !strcmp(property->name, "CCD_EXPOSURE")) {
		indigo_property_copy_values(CCD_EXPOSURE_PROPERTY, property, false);
		CCD_EXPOSURE_PROPERTY->state = INDIGO_BUSY_STATE;
		indigo_update_property(device, CCD_EXPOSURE_PROPERTY, NULL);
		indigo_ccd_exposure_setup(device);
		indigo_set_timer(device, fmax(CCD_EXPOSURE_ITEM->number.target, 0.02), synthetic_frame, &synthetic_timer);
		return INDIGO_OK;
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
	REQUIRE(indigo_agent_guider(INDIGO_DRIVER_INIT, NULL) == INDIGO_OK);
	agent_started = true;
	REQUIRE(wait_state(AGENT, "AGENT_START_PROCESS", 0, INDIGO_OK_STATE));
	camera_change = camera_device->change_property;
	camera_device->change_property = camera_spy;
	return true;
}

static bool connect_camera(void) {
	if (value(AGENT, "FILTER_CCD_LIST", CAMERA) != 1) {
		REQUIRE(sw(AGENT, "FILTER_CCD_LIST", CAMERA, true, INDIGO_OK_STATE));
	}
	REQUIRE(sw(AGENT, "CCD_UPLOAD_MODE", "CLIENT", true, INDIGO_OK_STATE));
	REQUIRE(sw(AGENT, "CCD_IMAGE_FORMAT", "RAW", true, INDIGO_OK_STATE));
	REQUIRE(num(AGENT, "AGENT_GUIDER_SETTINGS", "EXPOSURE", 0.1));
	REQUIRE(num(CAMERA, "SIMULATION_SETUP", "PER_ERR_VAL", 0));
	REQUIRE(num(CAMERA, "SIMULATION_SETUP", "IMAGE_GRADIENT", 0));
	REQUIRE(num(CAMERA, "SIMULATION_SETUP", "IMAGE_NOISE_VAR", 1));
	return true;
}

static bool run(const char *process, int state) {
	int before = camera_requests;
	REQUIRE(sw(AGENT, "AGENT_START_PROCESS", process, true, state));
	if (state == INDIGO_BUSY_STATE) {
		double deadline = indigo_monotonic_time() + 15;
		while (camera_requests == before && indigo_monotonic_time() < deadline) {
			indigo_usleep(1000);
		}
		REQUIRE(camera_requests > before);
	}
	return true;
}

static bool abort_running(void) {
	unsigned start = revision(AGENT, "AGENT_START_PROCESS");
	REQUIRE(sw(AGENT, "AGENT_ABORT_PROCESS", "ABORT", true, INDIGO_OK_STATE));
	REQUIRE(wait_state(AGENT, "AGENT_START_PROCESS", start, -1));
	REQUIRE(value(AGENT, "AGENT_ABORT_PROCESS", "ABORT") == 0);
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

static void cleanup_related(void);

static void cleanup(void) {
	if (agent_started) {
		indigo_change_switch_property_1(&client, AGENT, "AGENT_ABORT_PROCESS", "ABORT", true);
		wait_state(AGENT, "AGENT_START_PROCESS", 0, -1);
		if (indigo_agent_guider(INDIGO_DRIVER_SHUTDOWN, NULL) != INDIGO_OK) {
			indigo_test_failures++;
		}
	}
	cleanup_related();
	if (simulator_started) {
		const char *names[] = { CAMERA, GUIDER, CCD_SIMULATOR_IMAGER_CAMERA_NAME };
		for (int i = 0; i < ARRAY_SIZE(names); i++) {
			if (value(names[i], "CONNECTION", "CONNECTED") == 1) {
				sw(names[i], "CONNECTION", "DISCONNECTED", true, INDIGO_OK_STATE);
			}
		}
		indigo_cancel_timer_sync(camera_device, &synthetic_timer);
		indigo_cancel_timer_sync(camera_device, &failure_timer);
		if (guider_device && guider_change) {
			guider_device->change_property = guider_change;
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

static int state(const char *device, const char *name) {
	indigo_property *p = snapshot(device, name);
	int result = p ? p->state : -1;
	indigo_release_property(p);
	return result;
}

static bool wait_value(const char *device, const char *name, const char *item, double minimum, double seconds) {
	double deadline = indigo_monotonic_time() + seconds;
	while (indigo_monotonic_time() < deadline) {
		if (value(device, name, item) >= minimum) {
			return true;
		}
		indigo_usleep(10000);
	}
	fprintf(stderr, "Timeout %s.%s.%s >= %g (got %g)\n", device, name, item, minimum, value(device, name, item));
	return false;
}

static bool wait_images(unsigned count) {
	double deadline = indigo_monotonic_time() + 15;
	while (indigo_monotonic_time() < deadline) {
		if (blobs(CAMERA) >= count) {
			return true;
		}
		indigo_usleep(10000);
	}
	return false;
}

static bool connect_guider(void) {
	REQUIRE(connect_camera());
	if (value(AGENT, "FILTER_GUIDER_LIST", GUIDER) != 1) {
		REQUIRE(sw(AGENT, "FILTER_GUIDER_LIST", GUIDER, true, INDIGO_OK_STATE));
	}
	if (!guider_change) {
		guider_change = guider_device->change_property;
		guider_device->change_property = guider_spy;
	}
	return true;
}

static bool model_camera(void) {
	REQUIRE(connect_guider());
	synthetic = true;
	REQUIRE(num(CAMERA, "SIMULATION_SETUP", "IMAGE_WIDTH", 400));
	REQUIRE(num(CAMERA, "SIMULATION_SETUP", "IMAGE_HEIGHT", 300));
	REQUIRE(num(AGENT, "AGENT_GUIDER_SETTINGS", "EXPOSURE", 0.1));
	return true;
}

static bool configured_guiding(void) {
	REQUIRE(model_camera());
	REQUIRE(num(AGENT, "AGENT_GUIDER_SETTINGS", "SPEED_RA", 10));
	REQUIRE(num(AGENT, "AGENT_GUIDER_SETTINGS", "SPEED_DEC", 10));
	REQUIRE(num(AGENT, "AGENT_GUIDER_SETTINGS", "ANGLE", 0));
	REQUIRE(num(AGENT, "AGENT_GUIDER_SETTINGS", "MAX_PULSE", 0.1));
	return true;
}

static void metadata(void) {
	const char *names[] = { "FILTER_CCD_LIST", "FILTER_GUIDER_LIST", "FILTER_RELATED_AGENT_LIST", "AGENT_START_PROCESS", "AGENT_ABORT_PROCESS", "AGENT_GUIDER_CORRECTION_MODE_RA", "AGENT_GUIDER_CORRECTION_MODE_DEC", "AGENT_GUIDER_DETECTION_MODE", "AGENT_GUIDER_DEC_MODE", "AGENT_GUIDER_APPLY_DEC_BACKLASH", "AGENT_PROCESS_FEATURES", "AGENT_GUIDER_MOUNT_COORDINATES", "AGENT_GUIDER_SETTINGS", "AGENT_GUIDER_FLIP_REVERSES_DEC", "AGENT_GUIDER_STARS", "AGENT_GUIDER_SELECTION", "AGENT_GUIDER_STATS", "AGENT_GUIDER_LOG", "AGENT_GUIDER_DITHERING_OFFSETS", "AGENT_GUIDER_DITHERING_STRATEGY", "AGENT_GUIDER_DITHER", "AGENT_GUIDER_RESET_PPEC" };
	for (int i = 0; i < ARRAY_SIZE(names); i++) {
		ASSERT_TRUE(revision(AGENT, names[i]) > 0);
	}
	indigo_driver_info info;
	ASSERT_EQ_INT(INDIGO_OK, indigo_agent_guider(INDIGO_DRIVER_INFO, &info));
	ASSERT_TRUE(!strcmp(info.name, "indigo_agent_guider"));
	ASSERT_EQ_INT(INDIGO_OK, indigo_agent_guider(INDIGO_DRIVER_INIT, NULL));
	ASSERT_EQ_INT(1, value(AGENT, "AGENT_GUIDER_DETECTION_MODE", "SELECTION"));
}

static void missing_devices(void) {
	const char *processes[] = { "PREVIEW_1", "PREVIEW", "CALIBRATION", "CALIBRATION_AND_GUIDING", "GUIDING" };
	for (int i = 0; i < ARRAY_SIZE(processes); i++) {
		ASSERT_TRUE(run(processes[i], INDIGO_ALERT_STATE));
		ASSERT_EQ_INT(0, value(AGENT, "AGENT_START_PROCESS", processes[i]));
	}
	ASSERT_TRUE(connect_camera());
	for (int i = 2; i < ARRAY_SIZE(processes); i++) {
		ASSERT_TRUE(run(processes[i], INDIGO_ALERT_STATE));
	}
	ASSERT_TRUE(sw(AGENT, "AGENT_GUIDER_DITHER", "TRIGGER", true, INDIGO_ALERT_STATE));
	ASSERT_TRUE(sw(AGENT, "AGENT_GUIDER_DITHER", "RESET", true, INDIGO_OK_STATE));
}

static void single_preview(void) {
	ASSERT_TRUE(connect_camera());
	ASSERT_TRUE(sw(AGENT, "CCD_IMAGE_FORMAT", "FITS", true, INDIGO_OK_STATE));
	ASSERT_TRUE(run("PREVIEW_1", -1));
	ASSERT_EQ_INT(1, blobs(CAMERA));
	ASSERT_EQ_INT(1, value(AGENT, "CCD_IMAGE_FORMAT", "FITS"));
	ASSERT_EQ_INT(INDIGO_OK_STATE, state(AGENT, "AGENT_START_PROCESS"));
	ASSERT_EQ_INT(INDIGO_GUIDER_PHASE_DONE, value(AGENT, "AGENT_GUIDER_STATS", "PHASE"));
}

static void preview_failure(void) {
	ASSERT_TRUE(connect_camera());
	ASSERT_TRUE(sw(AGENT, "CCD_IMAGE_FORMAT", "FITS", true, INDIGO_OK_STATE));
	exposure_failures = 3;
	ASSERT_TRUE(run("PREVIEW_1", INDIGO_ALERT_STATE));
	ASSERT_EQ_INT(3, camera_requests);
	ASSERT_EQ_INT(0, blobs(CAMERA));
	ASSERT_EQ_INT(INDIGO_GUIDER_PHASE_FAILED, value(AGENT, "AGENT_GUIDER_STATS", "PHASE"));
	ASSERT_EQ_INT(0, value(AGENT, "AGENT_START_PROCESS", "PREVIEW_1"));
	ASSERT_EQ_INT(1, value(AGENT, "CCD_IMAGE_FORMAT", "FITS"));
	ASSERT_TRUE(run("PREVIEW_1", INDIGO_OK_STATE));
	ASSERT_EQ_INT(1, blobs(CAMERA));
	ASSERT_EQ_INT(INDIGO_GUIDER_PHASE_DONE, value(AGENT, "AGENT_GUIDER_STATS", "PHASE"));
}

static void preview_retry(void) {
	ASSERT_TRUE(connect_camera());
	exposure_failures = 2;
	ASSERT_TRUE(run("PREVIEW_1", INDIGO_OK_STATE));
	ASSERT_EQ_INT(3, camera_requests);
	ASSERT_EQ_INT(1, blobs(CAMERA));
	ASSERT_EQ_INT(INDIGO_GUIDER_PHASE_DONE, value(AGENT, "AGENT_GUIDER_STATS", "PHASE"));
	ASSERT_EQ_INT(0, value(AGENT, "AGENT_START_PROCESS", "PREVIEW_1"));
}

static void preview_invalid_raw(void) {
	ASSERT_TRUE(configured_guiding());
	short_image = true;
	ASSERT_TRUE(run("PREVIEW_1", INDIGO_ALERT_STATE));
	ASSERT_EQ_INT(INDIGO_GUIDER_PHASE_FAILED, value(AGENT, "AGENT_GUIDER_STATS", "PHASE"));
	ASSERT_EQ_INT(0, value(AGENT, "AGENT_START_PROCESS", "PREVIEW_1"));
	short_image = false;
	ASSERT_TRUE(run("PREVIEW_1", INDIGO_OK_STATE));
	ASSERT_EQ_INT(INDIGO_GUIDER_PHASE_DONE, value(AGENT, "AGENT_GUIDER_STATS", "PHASE"));
}

static void continuous_preview_abort(void) {
	ASSERT_TRUE(connect_camera());
	ASSERT_TRUE(sw(AGENT, "CCD_IMAGE_FORMAT", "FITS", true, INDIGO_OK_STATE));
	ASSERT_TRUE(run("PREVIEW", INDIGO_BUSY_STATE));
	ASSERT_TRUE(wait_images(3));
	ASSERT_TRUE(abort_running());
	ASSERT_EQ_INT(INDIGO_OK_STATE, state(AGENT, "AGENT_START_PROCESS"));
	ASSERT_EQ_INT(INDIGO_GUIDER_PHASE_DONE, value(AGENT, "AGENT_GUIDER_STATS", "PHASE"));
	ASSERT_EQ_INT(0, value(AGENT, "AGENT_START_PROCESS", "PREVIEW"));
	ASSERT_EQ_INT(1, value(AGENT, "CCD_IMAGE_FORMAT", "FITS"));
	unsigned before = blobs(CAMERA);
	ASSERT_TRUE(run("PREVIEW", INDIGO_BUSY_STATE));
	ASSERT_TRUE(wait_images(before + 3));
	ASSERT_TRUE(abort_running());
	ASSERT_EQ_INT(INDIGO_OK_STATE, state(AGENT, "AGENT_START_PROCESS"));
}

static void continuous_preview_failure(bool malformed) {
	ASSERT_TRUE(configured_guiding());
	ASSERT_TRUE(sw(AGENT, "CCD_IMAGE_FORMAT", "FITS", true, INDIGO_OK_STATE));
	ASSERT_TRUE(run("PREVIEW", INDIGO_BUSY_STATE));
	ASSERT_TRUE(wait_images(3));
	unsigned before = revision(AGENT, "AGENT_START_PROCESS");
	if (malformed) {
		short_image = true;
	} else {
		exposure_failures = 3;
	}
	ASSERT_TRUE(wait_state(AGENT, "AGENT_START_PROCESS", before, INDIGO_ALERT_STATE));
	ASSERT_EQ_INT(INDIGO_GUIDER_PHASE_FAILED, value(AGENT, "AGENT_GUIDER_STATS", "PHASE"));
	ASSERT_EQ_INT(0, value(AGENT, "AGENT_START_PROCESS", "PREVIEW"));
	ASSERT_EQ_INT(1, value(AGENT, "CCD_IMAGE_FORMAT", "FITS"));
	ASSERT_EQ_INT(0, exposure_failures);
	short_image = false;
	before = blobs(CAMERA);
	ASSERT_TRUE(run("PREVIEW", INDIGO_BUSY_STATE));
	ASSERT_TRUE(wait_images(before + 3));
	ASSERT_TRUE(abort_running());
	ASSERT_EQ_INT(INDIGO_OK_STATE, state(AGENT, "AGENT_START_PROCESS"));
	ASSERT_EQ_INT(INDIGO_GUIDER_PHASE_DONE, value(AGENT, "AGENT_GUIDER_STATS", "PHASE"));
}

static void continuous_preview_exposure_failure(void) {
	continuous_preview_failure(false);
}

static void continuous_preview_invalid_raw(void) {
	continuous_preview_failure(true);
}

static void preview_abort_and_restart(void) {
	ASSERT_TRUE(connect_camera());
	ASSERT_TRUE(sw(AGENT, "CCD_IMAGE_FORMAT", "FITS", true, INDIGO_OK_STATE));
	ASSERT_TRUE(num(AGENT, "AGENT_GUIDER_SETTINGS", "EXPOSURE", 5));
	ASSERT_TRUE(run("PREVIEW_1", INDIGO_BUSY_STATE));
	ASSERT_TRUE(wait_state(CAMERA, "CCD_EXPOSURE", 0, INDIGO_BUSY_STATE));
	ASSERT_TRUE(abort_running());
	ASSERT_TRUE(camera_abort_requests > 0);
	ASSERT_EQ_INT(INDIGO_OK_STATE, state(AGENT, "AGENT_START_PROCESS"));
	ASSERT_EQ_INT(INDIGO_GUIDER_PHASE_DONE, value(AGENT, "AGENT_GUIDER_STATS", "PHASE"));
	ASSERT_EQ_INT(0, value(AGENT, "AGENT_START_PROCESS", "PREVIEW_1"));
	ASSERT_EQ_INT(1, value(AGENT, "CCD_IMAGE_FORMAT", "FITS"));
	ASSERT_EQ_INT(0, blobs(CAMERA));
	ASSERT_TRUE(num(AGENT, "AGENT_GUIDER_SETTINGS", "EXPOSURE", 0.1));
	ASSERT_TRUE(run("PREVIEW_1", INDIGO_OK_STATE));
	ASSERT_EQ_INT(1, blobs(CAMERA));
}

static void stars_selection(void) {
	ASSERT_TRUE(connect_camera());
	ASSERT_TRUE(sw(AGENT, "AGENT_GUIDER_STARS", "REFRESH", true, INDIGO_OK_STATE));
	indigo_property *stars = snapshot(AGENT, "AGENT_GUIDER_STARS");
	ASSERT_TRUE(stars && stars->count > 1);
	char name[INDIGO_NAME_SIZE];
	strcpy(name, stars->items[1].name);
	indigo_release_property(stars);
	ASSERT_TRUE(sw(AGENT, "AGENT_GUIDER_STARS", name, true, INDIGO_OK_STATE));
	ASSERT_TRUE(value(AGENT, "AGENT_GUIDER_SELECTION", "X") > 0);
	ASSERT_TRUE(value(AGENT, "AGENT_GUIDER_SELECTION", "Y") > 0);
	ASSERT_TRUE(run("CLEAR_SELECTION", INDIGO_OK_STATE));
	ASSERT_EQ_INT(0, value(AGENT, "AGENT_GUIDER_SELECTION", "X"));
	ASSERT_TRUE(num(AGENT, "AGENT_GUIDER_SELECTION", "COUNT", 3));
	ASSERT_EQ_INT(0, value(AGENT, "AGENT_GUIDER_SELECTION", "X_3"));
	ASSERT_TRUE(num(AGENT, "AGENT_GUIDER_SELECTION", "COUNT", 1));
	ASSERT_TRUE(isnan(value(AGENT, "AGENT_GUIDER_SELECTION", "X_3")));
}

static void move_image(double x, double y);

static void guide_mode(const char *detection, const char *ra, const char *dec) {
	ASSERT_TRUE(configured_guiding());
	// Full-frame centroid requires a bright extended target rather than sparse stars.
	extended_image = !strcmp(detection, "CENTROID");
	ASSERT_TRUE(sw(AGENT, "AGENT_GUIDER_DETECTION_MODE", detection, true, INDIGO_OK_STATE));
	ASSERT_TRUE(sw(AGENT, "AGENT_GUIDER_CORRECTION_MODE_RA", ra, true, INDIGO_OK_STATE));
	ASSERT_TRUE(sw(AGENT, "AGENT_GUIDER_CORRECTION_MODE_DEC", dec, true, INDIGO_OK_STATE));
	ASSERT_TRUE(run("GUIDING", INDIGO_BUSY_STATE));
	ASSERT_TRUE(wait_value(AGENT, "AGENT_GUIDER_STATS", "FRAME", 3, 20));
	move_image(2, 2);
	ASSERT_TRUE(wait_value(AGENT, "AGENT_GUIDER_STATS", "FRAME", 12, 20));
	ASSERT_TRUE(isfinite(value(AGENT, "AGENT_GUIDER_STATS", "DRIFT_RA")));
	ASSERT_TRUE(isfinite(value(AGENT, "AGENT_GUIDER_STATS", "RMSE_RA")));
	ASSERT_TRUE(abort_running());
	pthread_mutex_lock(&motion_mutex);
	double residual = hypot(offset_x, offset_y);
	unsigned commands = ra_commands + dec_commands;
	pthread_mutex_unlock(&motion_mutex);
	ASSERT_TRUE(commands > 0);
	ASSERT_TRUE(residual < 1);
	ASSERT_EQ_INT(INDIGO_OK_STATE, state(AGENT, "AGENT_START_PROCESS"));
}

static void selection_pi(void) { guide_mode("SELECTION", AGENT_GUIDER_CORRECTION_MODE_PI_ITEM_NAME, AGENT_GUIDER_CORRECTION_MODE_PI_ITEM_NAME); }

static void weighted_hysteresis(void) { guide_mode("WEIGHTED_SELECTION", "HYSTERESIS", "HYSTERESIS"); }

static void donuts_trend(void) { guide_mode("DONUTS", "LINEAR_TREND", "LINEAR_TREND"); }

static void centroid_resist(void) { guide_mode("CENTROID", AGENT_GUIDER_CORRECTION_MODE_PI_ITEM_NAME, "RESIST_SWITCH"); }

static void selection_ppec(void) { guide_mode("SELECTION", "PPEC", AGENT_GUIDER_CORRECTION_MODE_PI_ITEM_NAME); }

static void guiding_preconditions(void) {
	ASSERT_TRUE(connect_guider());
	ASSERT_TRUE(run("GUIDING", INDIGO_ALERT_STATE));
	ASSERT_TRUE(num(AGENT, "AGENT_GUIDER_SETTINGS", "SPEED_RA", 10));
	ASSERT_TRUE(num(AGENT, "AGENT_GUIDER_MOUNT_COORDINATES", "DEC", 90));
	ASSERT_TRUE(run("GUIDING", INDIGO_ALERT_STATE));
}

static void busy_guards(void) {
	ASSERT_TRUE(configured_guiding());
	ASSERT_TRUE(run("GUIDING", INDIGO_BUSY_STATE));
	ASSERT_TRUE(wait_value(AGENT, "AGENT_GUIDER_STATS", "FRAME", 2, 20));
	ASSERT_TRUE(sw(AGENT, "AGENT_GUIDER_DETECTION_MODE", "DONUTS", true, INDIGO_OK_STATE));
	ASSERT_EQ_INT(1, value(AGENT, "AGENT_GUIDER_DETECTION_MODE", "SELECTION"));
	ASSERT_TRUE(sw(AGENT, "AGENT_GUIDER_CORRECTION_MODE_RA", "HYSTERESIS", true, INDIGO_OK_STATE));
	ASSERT_EQ_INT(1, value(AGENT, "AGENT_GUIDER_CORRECTION_MODE_RA", AGENT_GUIDER_CORRECTION_MODE_PI_ITEM_NAME));
	ASSERT_TRUE(sw(AGENT, "AGENT_GUIDER_CORRECTION_MODE_DEC", "HYSTERESIS", true, INDIGO_OK_STATE));
	ASSERT_EQ_INT(1, value(AGENT, "AGENT_GUIDER_CORRECTION_MODE_DEC", AGENT_GUIDER_CORRECTION_MODE_PI_ITEM_NAME));
	ASSERT_TRUE(num(AGENT, "AGENT_GUIDER_SELECTION", "COUNT", 3));
	ASSERT_EQ_INT(1, value(AGENT, "AGENT_GUIDER_SELECTION", "COUNT"));
	double clipping = value(AGENT, "AGENT_GUIDER_SELECTION", "EDGE_CLIPPING");
	ASSERT_TRUE(num(AGENT, "AGENT_GUIDER_SELECTION", "EDGE_CLIPPING", clipping + 1));
	ASSERT_EQ_INT(clipping, value(AGENT, "AGENT_GUIDER_SELECTION", "EDGE_CLIPPING"));
	ASSERT_EQ_INT(INDIGO_OK, indigo_change_switch_property_1(&client, AGENT, "AGENT_START_PROCESS", "CALIBRATION", true));
	ASSERT_EQ_INT(1, value(AGENT, "AGENT_START_PROCESS", "GUIDING"));
	ASSERT_EQ_INT(0, value(AGENT, "AGENT_START_PROCESS", "CALIBRATION"));
	ASSERT_TRUE(txt(AGENT, "AGENT_GUIDER_LOG", "DIR", config_folder, INDIGO_ALERT_STATE));
	ASSERT_TRUE(sw(AGENT, "AGENT_GUIDER_RESET_PPEC", "RESET", true, INDIGO_OK_STATE));
	ASSERT_TRUE(abort_running());
}

static void calibration_adaptive_step(void) {
	ASSERT_TRUE(model_camera());
	ASSERT_TRUE(num(AGENT, "AGENT_GUIDER_SETTINGS", "MIN_CALIBRATION_DRIFT", 3));
	ASSERT_TRUE(run("CALIBRATION", INDIGO_BUSY_STATE));
	ASSERT_TRUE(wait_state(AGENT, "AGENT_START_PROCESS", revision(AGENT, "AGENT_START_PROCESS"), INDIGO_OK_STATE));
	ASSERT_TRUE(fabs(value(AGENT, "AGENT_GUIDER_SETTINGS", "SPEED_RA")) > 0);
	ASSERT_TRUE(fabs(value(AGENT, "AGENT_GUIDER_SETTINGS", "SPEED_DEC")) > 0);
}

static void calibration(void) {
	ASSERT_TRUE(model_camera());
	ASSERT_TRUE(txt(AGENT, "AGENT_GUIDER_LOG", "DIR", config_folder, INDIGO_OK_STATE));
	ASSERT_TRUE(sw(AGENT, "AGENT_PROCESS_FEATURES", "ENABLE_LOGGING", true, INDIGO_OK_STATE));
	ASSERT_TRUE(num(AGENT, "AGENT_GUIDER_SETTINGS", "MIN_CALIBRATION_DRIFT", 10));
	ASSERT_TRUE(run("CALIBRATION", INDIGO_OK_STATE));
	ASSERT_TRUE(fabs(value(AGENT, "AGENT_GUIDER_SETTINGS", "SPEED_RA")) > 0);
	ASSERT_TRUE(fabs(value(AGENT, "AGENT_GUIDER_SETTINGS", "SPEED_DEC")) > 0);
}

static void calibration_speed_accuracy(void) {
	calibration();
	if (indigo_test_failures) {
		return;
	}
	ASSERT_NEAR(10, fabs(value(AGENT, "AGENT_GUIDER_SETTINGS", "SPEED_RA")), 0.2);
	ASSERT_NEAR(10, fabs(value(AGENT, "AGENT_GUIDER_SETTINGS", "SPEED_DEC")), 0.2);
}

static void calibration_abort(void) {
	ASSERT_TRUE(model_camera());
	ASSERT_TRUE(run("CALIBRATION", INDIGO_BUSY_STATE));
	ASSERT_TRUE(wait_images(2));
	ASSERT_TRUE(abort_running());
	ASSERT_EQ_INT(0, value(AGENT, "AGENT_START_PROCESS", "CALIBRATION"));
}

static void dither_strategies(void) {
	ASSERT_TRUE(configured_guiding());
	ASSERT_TRUE(num(AGENT, "AGENT_GUIDER_SETTINGS", "DITHERING_MAX_AMOUNT", 1));
	ASSERT_TRUE(num(AGENT, "AGENT_GUIDER_SETTINGS", "DITHERING_LIMIT", 2));
	ASSERT_TRUE(run("GUIDING", INDIGO_BUSY_STATE));
	ASSERT_TRUE(wait_value(AGENT, "AGENT_GUIDER_STATS", "FRAME", 4, 20));
	const char *modes[] = { "RANDOMIZED_SPIRAL", "RANDOM", "SPIRAL" };
	for (int i = 0; i < ARRAY_SIZE(modes); i++) {
		ASSERT_TRUE(sw(AGENT, "AGENT_GUIDER_DITHERING_STRATEGY", modes[i], true, INDIGO_OK_STATE));
		ASSERT_TRUE(sw(AGENT, "AGENT_GUIDER_DITHER", "TRIGGER", true, INDIGO_OK_STATE));
		ASSERT_EQ_INT(0, value(AGENT, "AGENT_GUIDER_DITHER", "TRIGGER"));
		ASSERT_TRUE(fabs(value(AGENT, "AGENT_GUIDER_DITHERING_OFFSETS", "X")) <= 1);
		ASSERT_TRUE(fabs(value(AGENT, "AGENT_GUIDER_DITHERING_OFFSETS", "Y")) <= 1);
	}
	ASSERT_TRUE(abort_running());
}

static void dither_ra_projection(void) {
	ASSERT_TRUE(sw(AGENT, "AGENT_GUIDER_DEC_MODE", "NONE", true, INDIGO_OK_STATE));
	ASSERT_TRUE(num(AGENT, "AGENT_GUIDER_SETTINGS", "ANGLE", 45));
	const char *names[] = { "X", "Y" };
	const double values[] = { 3, 4 };
	unsigned before = revision(AGENT, "AGENT_GUIDER_DITHERING_OFFSETS");
	ASSERT_EQ_INT(INDIGO_OK, indigo_change_number_property(&client, AGENT, "AGENT_GUIDER_DITHERING_OFFSETS", 2, names, values));
	ASSERT_TRUE(wait_state(AGENT, "AGENT_GUIDER_DITHERING_OFFSETS", before, INDIGO_OK_STATE));
	double x = value(AGENT, "AGENT_GUIDER_DITHERING_OFFSETS", "X"), y = value(AGENT, "AGENT_GUIDER_DITHERING_OFFSETS", "Y");
	ASSERT_TRUE(fabs(hypot(x, y) - 5) < 0.001);
}

static void reset_defaults(void) {
	ASSERT_TRUE(num(AGENT, "AGENT_GUIDER_SETTINGS", "EXPOSURE", 2));
	ASSERT_TRUE(num(AGENT, "AGENT_GUIDER_SELECTION", "COUNT", 3));
	ASSERT_TRUE(sw(AGENT, "AGENT_GUIDER_CORRECTION_MODE_RA", "PPEC", true, INDIGO_OK_STATE));
	ASSERT_TRUE(run("RESET", INDIGO_OK_STATE));
	ASSERT_EQ_INT(1, value(AGENT, "AGENT_GUIDER_SETTINGS", "EXPOSURE"));
	ASSERT_EQ_INT(1, value(AGENT, "AGENT_GUIDER_SELECTION", "COUNT"));
	ASSERT_EQ_INT(1, value(AGENT, "AGENT_GUIDER_CORRECTION_MODE_RA", AGENT_GUIDER_CORRECTION_MODE_PI_ITEM_NAME));
}

static void instances(void) {
	ASSERT_TRUE(num(AGENT, "ADDITIONAL_INSTANCES", "COUNT", 2));
	ASSERT_TRUE(wait_state("Guider Agent #2", "AGENT_START_PROCESS", 0, INDIGO_OK_STATE));
	ASSERT_TRUE(wait_state("Guider Agent #3", "AGENT_START_PROCESS", 0, INDIGO_OK_STATE));
	ASSERT_TRUE(num("Guider Agent #2", "AGENT_GUIDER_SETTINGS", "EXPOSURE", 2));
	ASSERT_EQ_INT(1, value(AGENT, "AGENT_GUIDER_SETTINGS", "EXPOSURE"));
	ASSERT_TRUE(num(AGENT, "ADDITIONAL_INSTANCES", "COUNT", 0));
	ASSERT_TRUE(num(AGENT, "ADDITIONAL_INSTANCES", "COUNT", 1));
	ASSERT_TRUE(num(AGENT, "ADDITIONAL_INSTANCES", "COUNT", 0));
}

static bool frames(unsigned count) {
	return wait_value(AGENT, "AGENT_GUIDER_STATS", "FRAME", count, 15);
}

static void move_image(double x, double y) {
	pthread_mutex_lock(&motion_mutex);
	offset_x = x;
	offset_y = y;
	pthread_mutex_unlock(&motion_mutex);
}

static void correction_response(void) {
	ASSERT_TRUE(configured_guiding());
	ASSERT_TRUE(run("GUIDING", INDIGO_BUSY_STATE));
	ASSERT_TRUE(frames(3));
	move_image(3, -3);
	ASSERT_TRUE(frames(14));
	pthread_mutex_lock(&motion_mutex);
	double x = offset_x, y = offset_y, pulse = maximum_pulse;
	unsigned ra = ra_commands, dec = dec_commands;
	pthread_mutex_unlock(&motion_mutex);
	ASSERT_TRUE(abort_running());
	ASSERT_TRUE(ra > 0 && dec > 0);
	ASSERT_TRUE(fabs(x) < 0.3 && fabs(y) < 0.3);
	ASSERT_TRUE(pulse <= 100.001);
	ASSERT_TRUE(isfinite(value(AGENT, "AGENT_GUIDER_STATS", "RMSE_RA_ST")));
}

static void dec_modes(void) {
	ASSERT_TRUE(configured_guiding());
	const char *modes[] = { "NORTH", "SOUTH", "NONE" };
	for (int i = 0; i < ARRAY_SIZE(modes); i++) {
		ASSERT_TRUE(sw(AGENT, "AGENT_GUIDER_DEC_MODE", modes[i], true, INDIGO_OK_STATE));
		move_image(0, 0);
		ASSERT_TRUE(run("GUIDING", INDIGO_BUSY_STATE));
		ASSERT_TRUE(frames(3));
		pthread_mutex_lock(&motion_mutex);
		unsigned before = dec_commands;
		pthread_mutex_unlock(&motion_mutex);
		move_image(0, i == 1 ? -3 : 3);
		ASSERT_TRUE(frames(7));
		pthread_mutex_lock(&motion_mutex);
		unsigned after = dec_commands;
		double dec = pulse_dec;
		pthread_mutex_unlock(&motion_mutex);
		ASSERT_TRUE(abort_running());
		if (i == 2) {
			ASSERT_EQ_INT(before, after);
		} else {
			ASSERT_TRUE(after > before);
			ASSERT_TRUE(i == 0 ? dec > 0 : dec < 0);
		}
	}
}

static void guiding_delay_abort(void) {
	ASSERT_TRUE(configured_guiding());
	ASSERT_TRUE(num(AGENT, "AGENT_GUIDER_SETTINGS", "DELAY", 4));
	ASSERT_TRUE(run("GUIDING", INDIGO_BUSY_STATE));
	ASSERT_TRUE(wait_value(AGENT, "AGENT_GUIDER_STATS", "DELAY", 4, 10));
	double before = indigo_monotonic_time();
	ASSERT_TRUE(abort_running());
	ASSERT_TRUE(indigo_monotonic_time() - before < 1.5);
}

static void calibration_and_guiding(void) {
	ASSERT_TRUE(model_camera());
	ASSERT_TRUE(num(AGENT, "AGENT_GUIDER_SETTINGS", "MIN_BL_DRIFT", 1));
	ASSERT_TRUE(num(AGENT, "AGENT_GUIDER_SETTINGS", "MIN_CALIBRATION_DRIFT", 10));
	ASSERT_TRUE(run("CALIBRATION_AND_GUIDING", INDIGO_BUSY_STATE));
	double deadline = indigo_monotonic_time() + 40;
	while (indigo_monotonic_time() < deadline && value(AGENT, "AGENT_GUIDER_STATS", "PHASE") != INDIGO_GUIDER_PHASE_GUIDING) {
		indigo_usleep(10000);
	}
	ASSERT_EQ_INT(INDIGO_GUIDER_PHASE_GUIDING, value(AGENT, "AGENT_GUIDER_STATS", "PHASE"));
	ASSERT_TRUE(frames(4));
	ASSERT_TRUE(abort_running());
}

static void calibration_no_motion(void) {
	ASSERT_TRUE(model_camera());
	freeze_motion = true;
	ASSERT_TRUE(num(AGENT, "AGENT_GUIDER_SETTINGS", "MAX_BL_STEPS", 1));
	ASSERT_TRUE(num(AGENT, "AGENT_GUIDER_SETTINGS", "MAX_CALIBRATION_STEPS", 2));
	ASSERT_TRUE(run("CALIBRATION", INDIGO_ALERT_STATE));
	ASSERT_EQ_INT(INDIGO_GUIDER_PHASE_FAILED, value(AGENT, "AGENT_GUIDER_STATS", "PHASE"));
}

static void star_loss_fail(void) {
	ASSERT_TRUE(configured_guiding());
	ASSERT_TRUE(sw(AGENT, "AGENT_PROCESS_FEATURES", "FAIL_ON_GUIDING_ERROR", true, INDIGO_OK_STATE));
	ASSERT_TRUE(run("GUIDING", INDIGO_BUSY_STATE));
	ASSERT_TRUE(frames(3));
	unsigned before = revision(AGENT, "AGENT_START_PROCESS");
	blank_image = true;
	ASSERT_TRUE(wait_state(AGENT, "AGENT_START_PROCESS", before, INDIGO_ALERT_STATE));
	blank_image = false;
	ASSERT_TRUE(run("GUIDING", INDIGO_BUSY_STATE));
	ASSERT_TRUE(frames(3));
	ASSERT_TRUE(abort_running());
}

static void star_loss_recovery(const char *policy) {
	ASSERT_TRUE(configured_guiding());
	ASSERT_TRUE(sw(AGENT, "AGENT_PROCESS_FEATURES", policy, true, INDIGO_OK_STATE));
	ASSERT_TRUE(run("GUIDING", INDIGO_BUSY_STATE));
	ASSERT_TRUE(frames(3));
	unsigned before = blobs(CAMERA);
	blank_image = true;
	ASSERT_TRUE(wait_images(before + 3));
	ASSERT_EQ_INT(INDIGO_BUSY_STATE, state(AGENT, "AGENT_START_PROCESS"));
	blank_image = false;
	before = blobs(CAMERA);
	ASSERT_TRUE(wait_images(before + 4));
	ASSERT_EQ_INT(INDIGO_BUSY_STATE, state(AGENT, "AGENT_START_PROCESS"));
	ASSERT_TRUE(abort_running());
}

static void star_loss_continue(void) { star_loss_recovery("CONTINUE_ON_GUIDING_ERROR"); }

static void star_loss_reset(void) { star_loss_recovery("RESET_ON_GUIDING_ERROR"); }

static void exposure_failure_guiding(void) {
	ASSERT_TRUE(configured_guiding());
	ASSERT_TRUE(run("GUIDING", INDIGO_BUSY_STATE));
	ASSERT_TRUE(frames(3));
	unsigned before = revision(AGENT, "AGENT_START_PROCESS");
	exposure_failures = 3;
	ASSERT_TRUE(wait_state(AGENT, "AGENT_START_PROCESS", before, INDIGO_ALERT_STATE));
	ASSERT_TRUE(run("GUIDING", INDIGO_BUSY_STATE));
	ASSERT_TRUE(frames(3));
	ASSERT_TRUE(abort_running());
}

static void invalid_raw(void) {
	ASSERT_TRUE(configured_guiding());
	invalid_image = true;
	ASSERT_TRUE(run("GUIDING", INDIGO_ALERT_STATE));
	ASSERT_EQ_INT(INDIGO_GUIDER_PHASE_FAILED, value(AGENT, "AGENT_GUIDER_STATS", "PHASE"));
}

static void truncated_raw(void) {
	ASSERT_TRUE(configured_guiding());
	short_image = true;
	const int sizes[] = { 1, 4, sizeof(indigo_raw_header) - 1 };
	for (int i = 0; i < ARRAY_SIZE(sizes); i++) {
		short_image_size = sizes[i];
		ASSERT_TRUE(run("GUIDING", INDIGO_ALERT_STATE));
	}
	short_image = false;
	ASSERT_TRUE(run("GUIDING", INDIGO_BUSY_STATE));
	ASSERT_TRUE(frames(3));
	ASSERT_TRUE(abort_running());
}

static void raw_dimensions(void) {
	ASSERT_TRUE(configured_guiding());
	raw_format = INDIGO_RAW_MONO16;
	for (int fault = RAW_ZERO_WIDTH; fault <= RAW_PIXEL_OVERFLOW; fault++) {
		raw_fault = fault;
		ASSERT_TRUE(run("GUIDING", INDIGO_ALERT_STATE));
		ASSERT_EQ_INT(INDIGO_GUIDER_PHASE_FAILED, value(AGENT, "AGENT_GUIDER_STATS", "PHASE"));
	}
	raw_fault = RAW_COMPLETE;
	ASSERT_TRUE(run("GUIDING", INDIGO_BUSY_STATE));
	ASSERT_TRUE(frames(3));
	ASSERT_TRUE(abort_running());
}

static void raw_payload_formats(void) {
	ASSERT_TRUE(configured_guiding());
	const unsigned formats[] = { INDIGO_RAW_MONO8, INDIGO_RAW_MONO16, INDIGO_RAW_RGB24, INDIGO_RAW_RGB48 };
	for (int i = 0; i < ARRAY_SIZE(formats); i++) {
		raw_format = formats[i];
		raw_fault = RAW_SHORT_PIXELS;
		ASSERT_TRUE(run("GUIDING", INDIGO_ALERT_STATE));
		raw_fault = RAW_COMPLETE;
		ASSERT_TRUE(run("GUIDING", INDIGO_BUSY_STATE));
		ASSERT_TRUE(frames(3));
		ASSERT_TRUE(abort_running());
	}
}

static void pulse_error(void) {
	ASSERT_TRUE(configured_guiding());
	ASSERT_TRUE(run("GUIDING", INDIGO_BUSY_STATE));
	ASSERT_TRUE(frames(3));
	unsigned before = revision(AGENT, "AGENT_START_PROCESS");
	pulse_failure = true;
	move_image(3, 3);
	ASSERT_TRUE(wait_state(AGENT, "AGENT_START_PROCESS", before, INDIGO_ALERT_STATE));
}

static void camera_disconnect(void) {
	ASSERT_TRUE(connect_camera());
	ASSERT_TRUE(num(AGENT, "AGENT_GUIDER_SETTINGS", "EXPOSURE", 5));
	ASSERT_TRUE(run("PREVIEW", INDIGO_BUSY_STATE));
	ASSERT_TRUE(wait_state(CAMERA, "CCD_EXPOSURE", 0, INDIGO_BUSY_STATE));
	unsigned before = revision(AGENT, "AGENT_START_PROCESS");
	ASSERT_TRUE(sw(CAMERA, "CONNECTION", "DISCONNECTED", true, INDIGO_OK_STATE));
	ASSERT_TRUE(wait_state(AGENT, "AGENT_START_PROCESS", before, INDIGO_ALERT_STATE));
	ASSERT_EQ_INT(INDIGO_GUIDER_PHASE_FAILED, value(AGENT, "AGENT_GUIDER_STATS", "PHASE"));
	ASSERT_EQ_INT(0, value(AGENT, "AGENT_START_PROCESS", "PREVIEW"));
	ASSERT_TRUE(connect_camera());
	before = blobs(CAMERA);
	ASSERT_TRUE(run("PREVIEW", INDIGO_BUSY_STATE));
	ASSERT_TRUE(wait_images(before + 3));
	ASSERT_TRUE(abort_running());
	ASSERT_EQ_INT(INDIGO_OK_STATE, state(AGENT, "AGENT_START_PROCESS"));
	ASSERT_EQ_INT(INDIGO_GUIDER_PHASE_DONE, value(AGENT, "AGENT_GUIDER_STATS", "PHASE"));
}

static void dither_abort(void) {
	ASSERT_TRUE(configured_guiding());
	ASSERT_TRUE(run("GUIDING", INDIGO_BUSY_STATE));
	ASSERT_TRUE(frames(3));
	freeze_motion = true;
	ASSERT_TRUE(sw(AGENT, "AGENT_GUIDER_DITHER", "TRIGGER", true, INDIGO_BUSY_STATE));
	ASSERT_TRUE(abort_running());
	ASSERT_TRUE(wait_state(AGENT, "AGENT_GUIDER_DITHER", 0, INDIGO_ALERT_STATE));
	ASSERT_EQ_INT(0, value(AGENT, "AGENT_GUIDER_DITHER", "TRIGGER"));
}

static void dither_timeout(void) {
	ASSERT_TRUE(configured_guiding());
	ASSERT_TRUE(num(AGENT, "AGENT_GUIDER_SETTINGS", "MIN_ERROR", 0.1));
	ASSERT_TRUE(num(AGENT, "AGENT_GUIDER_SETTINGS", "DITHERING_SETTLE_TIME_LIMIT", 1));
	ASSERT_TRUE(run("GUIDING", INDIGO_BUSY_STATE));
	ASSERT_TRUE(frames(4));
	freeze_motion = true;
	ASSERT_TRUE(sw(AGENT, "AGENT_GUIDER_DITHER", "TRIGGER", true, INDIGO_ALERT_STATE));
	ASSERT_TRUE(abort_running());
}

static void selection_subframe_restore(void) {
	ASSERT_TRUE(configured_guiding());
	ASSERT_TRUE(num(AGENT, "AGENT_GUIDER_SELECTION", "SUBFRAME", 4));
	double width = value(AGENT, "CCD_FRAME", "WIDTH"), height = value(AGENT, "CCD_FRAME", "HEIGHT");
	ASSERT_TRUE(run("GUIDING", INDIGO_BUSY_STATE));
	ASSERT_TRUE(frames(3));
	ASSERT_TRUE(value(AGENT, "CCD_FRAME", "WIDTH") < width);
	ASSERT_TRUE(abort_running());
	ASSERT_EQ_INT(width, value(AGENT, "CCD_FRAME", "WIDTH"));
	ASSERT_EQ_INT(height, value(AGENT, "CCD_FRAME", "HEIGHT"));
}

static void selection_regions_binning(void) {
	ASSERT_TRUE(model_camera());
	const char *items[] = { "INCLUDE_LEFT", "INCLUDE_TOP", "INCLUDE_WIDTH", "INCLUDE_HEIGHT", "EXCLUDE_LEFT", "EXCLUDE_TOP", "EXCLUDE_WIDTH", "EXCLUDE_HEIGHT" };
	const double numbers[] = { 50, 50, 290, 210, 95, 85, 30, 30 };
	unsigned before = revision(AGENT, "AGENT_GUIDER_SELECTION");
	ASSERT_EQ_INT(INDIGO_OK, indigo_change_number_property(&client, AGENT, "AGENT_GUIDER_SELECTION", 8, items, numbers));
	ASSERT_TRUE(wait_state(AGENT, "AGENT_GUIDER_SELECTION", before, INDIGO_OK_STATE));
	ASSERT_TRUE(sw(AGENT, "AGENT_GUIDER_STARS", "REFRESH", true, INDIGO_OK_STATE));
	ASSERT_TRUE(num(AGENT, "AGENT_GUIDER_SELECTION", "X", 210));
	ASSERT_TRUE(num(AGENT, "AGENT_GUIDER_SELECTION", "Y", 170));
	const char *bins[] = { "HORIZONTAL", "VERTICAL" };
	const double bin_values[] = { 2, 2 };
	before = revision(AGENT, "CCD_BIN");
	ASSERT_EQ_INT(INDIGO_OK, indigo_change_number_property(&client, AGENT, "CCD_BIN", 2, bins, bin_values));
	ASSERT_TRUE(wait_state(AGENT, "CCD_BIN", before, INDIGO_OK_STATE));
	ASSERT_EQ_INT(0, value(AGENT, "AGENT_GUIDER_SELECTION", "X"));
	ASSERT_EQ_INT(0, value(AGENT, "AGENT_GUIDER_SELECTION", "EXCLUDE_WIDTH"));
}

static void feature_policies(void) {
	const char *cal[] = { "FAIL_ON_CALIBRATION_ERROR", "RESET_ON_CALIBRATION_ERROR" };
	const char *guide[] = { "FAIL_ON_GUIDING_ERROR", "CONTINUE_ON_GUIDING_ERROR", "RESET_ON_GUIDING_ERROR" };
	for (int i = 0; i < ARRAY_SIZE(cal); i++) {
		ASSERT_TRUE(sw(AGENT, "AGENT_PROCESS_FEATURES", cal[i], true, INDIGO_OK_STATE));
		ASSERT_EQ_INT(0, value(AGENT, "AGENT_PROCESS_FEATURES", cal[1 - i]));
	}
	for (int i = 0; i < ARRAY_SIZE(guide); i++) {
		ASSERT_TRUE(sw(AGENT, "AGENT_PROCESS_FEATURES", guide[i], true, INDIGO_OK_STATE));
		for (int j = 0; j < ARRAY_SIZE(guide); j++) {
			ASSERT_EQ_INT(i == j, value(AGENT, "AGENT_PROCESS_FEATURES", guide[j]));
		}
	}
}

static void logging(void) {
	ASSERT_TRUE(configured_guiding());
	ASSERT_TRUE(txt(AGENT, "AGENT_GUIDER_LOG", "DIR", config_folder, INDIGO_OK_STATE));
	ASSERT_TRUE(txt(AGENT, "AGENT_GUIDER_LOG", "TEMPLATE", "test.csv", INDIGO_OK_STATE));
	ASSERT_TRUE(sw(AGENT, "AGENT_PROCESS_FEATURES", "ENABLE_LOGGING", true, INDIGO_OK_STATE));
	ASSERT_TRUE(run("GUIDING", INDIGO_BUSY_STATE));
	ASSERT_TRUE(frames(4));
	ASSERT_TRUE(abort_running());
	char path[1024];
	snprintf(path, sizeof(path), "%s/test.csv", config_folder);
	FILE *file = fopen(path, "r");
	ASSERT_TRUE(file != NULL);
	char text[32768];
	size_t size = fread(text, 1, sizeof(text) - 1, file);
	fclose(file);
	text[size] = 0;
	ASSERT_TRUE(size > 200);
	ASSERT_TRUE(strstr(text, "Guiding started") != NULL);
	ASSERT_TRUE(strstr(text, "\"Timestamp\",\"X Dif\"") != NULL);
	ASSERT_TRUE(strstr(text, "RA Settings [") != NULL);
	ASSERT_TRUE(strstr(text, "Dec Settings [") != NULL);
}

static void configuration_reload(void) {
	ASSERT_TRUE(num(AGENT, "AGENT_GUIDER_SETTINGS", "EXPOSURE", 2));
	ASSERT_TRUE(sw(AGENT, "AGENT_GUIDER_CORRECTION_MODE_RA", "HYSTERESIS", true, INDIGO_OK_STATE));
	ASSERT_TRUE(num(AGENT, "AGENT_GUIDER_SELECTION", "COUNT", 3));
	ASSERT_EQ_INT(INDIGO_OK, indigo_agent_guider(INDIGO_DRIVER_SHUTDOWN, NULL));
	agent_started = false;
	ASSERT_EQ_INT(INDIGO_OK, indigo_agent_guider(INDIGO_DRIVER_INIT, NULL));
	agent_started = true;
	ASSERT_TRUE(wait_state(AGENT, "AGENT_START_PROCESS", 0, INDIGO_OK_STATE));
	ASSERT_EQ_INT(2, value(AGENT, "AGENT_GUIDER_SETTINGS", "EXPOSURE"));
	ASSERT_EQ_INT(1, value(AGENT, "AGENT_GUIDER_CORRECTION_MODE_RA", "HYSTERESIS"));
	ASSERT_EQ_INT(3, value(AGENT, "AGENT_GUIDER_SELECTION", "COUNT"));
	ASSERT_TRUE(!isnan(value(AGENT, "AGENT_GUIDER_SELECTION", "X_3")));
}

static void multistar_weighted(void) {
	ASSERT_TRUE(configured_guiding());
	ASSERT_TRUE(num(AGENT, "AGENT_GUIDER_SELECTION", "COUNT", 3));
	ASSERT_TRUE(sw(AGENT, "AGENT_GUIDER_DETECTION_MODE", "WEIGHTED_SELECTION", true, INDIGO_OK_STATE));
	ASSERT_TRUE(run("GUIDING", INDIGO_BUSY_STATE));
	ASSERT_TRUE(frames(3));
	ASSERT_TRUE(value(AGENT, "AGENT_GUIDER_SELECTION", "X_3") > 0);
	move_image(2, 2);
	ASSERT_TRUE(frames(10));
	ASSERT_TRUE(abort_running());
	pthread_mutex_lock(&motion_mutex);
	double residual = hypot(offset_x, offset_y);
	pthread_mutex_unlock(&motion_mutex);
	ASSERT_TRUE(residual < 0.5);
}

static void calibration_ra_only(void) {
	ASSERT_TRUE(model_camera());
	ASSERT_TRUE(sw(AGENT, "AGENT_GUIDER_DEC_MODE", "NONE", true, INDIGO_OK_STATE));
	ASSERT_TRUE(num(AGENT, "AGENT_GUIDER_SETTINGS", "MIN_CALIBRATION_DRIFT", 10));
	ASSERT_TRUE(run("CALIBRATION", INDIGO_OK_STATE));
	pthread_mutex_lock(&motion_mutex);
	unsigned dec = dec_commands, ra = ra_commands;
	pthread_mutex_unlock(&motion_mutex);
	ASSERT_EQ_INT(0, dec);
	ASSERT_TRUE(ra > 0);
}

static void calibration_star_failure(void) {
	ASSERT_TRUE(model_camera());
	ASSERT_TRUE(sw(AGENT, "AGENT_PROCESS_FEATURES", "FAIL_ON_CALIBRATION_ERROR", true, INDIGO_OK_STATE));
	blank_image = true;
	ASSERT_TRUE(run("CALIBRATION", INDIGO_ALERT_STATE));
}

static void stars_abort_and_failure(void) {
	ASSERT_TRUE(model_camera());
	exposure_failures = 3;
	ASSERT_TRUE(sw(AGENT, "AGENT_GUIDER_STARS", "REFRESH", true, INDIGO_ALERT_STATE));
	ASSERT_TRUE(num(AGENT, "AGENT_GUIDER_SETTINGS", "EXPOSURE", 4));
	ASSERT_TRUE(sw(AGENT, "AGENT_GUIDER_STARS", "REFRESH", true, INDIGO_BUSY_STATE));
	ASSERT_TRUE(wait_state(CAMERA, "CCD_EXPOSURE", 0, INDIGO_BUSY_STATE));
	ASSERT_TRUE(sw(AGENT, "AGENT_ABORT_PROCESS", "ABORT", true, INDIGO_OK_STATE));
	ASSERT_TRUE(wait_state(AGENT, "AGENT_GUIDER_STARS", 0, INDIGO_ALERT_STATE));
	ASSERT_TRUE(num(AGENT, "AGENT_GUIDER_SETTINGS", "EXPOSURE", 0.1));
	ASSERT_TRUE(sw(AGENT, "AGENT_GUIDER_STARS", "REFRESH", true, INDIGO_OK_STATE));
}

static void meridian_flip(void) {
	ASSERT_TRUE(configured_guiding());
	ASSERT_TRUE(num(AGENT, "AGENT_GUIDER_SETTINGS", "SIDE_OF_PIER", -1));
	ASSERT_TRUE(num(AGENT, "AGENT_GUIDER_MOUNT_COORDINATES", "SIDE_OF_PIER", -1));
	ASSERT_TRUE(run("GUIDING", INDIGO_BUSY_STATE));
	ASSERT_TRUE(frames(3));
	freeze_motion = true;
	move_image(2, 2);
	ASSERT_TRUE(frames(6));
	pthread_mutex_lock(&motion_mutex);
	double ra = pulse_ra, dec = pulse_dec;
	pthread_mutex_unlock(&motion_mutex);
	ASSERT_TRUE(num(AGENT, "AGENT_GUIDER_MOUNT_COORDINATES", "SIDE_OF_PIER", 1));
	unsigned n = value(AGENT, "AGENT_GUIDER_STATS", "FRAME");
	ASSERT_TRUE(frames(n + 3));
	pthread_mutex_lock(&motion_mutex);
	double flipped_ra = pulse_ra, flipped_dec = pulse_dec;
	pthread_mutex_unlock(&motion_mutex);
	ASSERT_TRUE(ra * flipped_ra < 0 && dec * flipped_dec > 0);
	ASSERT_TRUE(sw(AGENT, "AGENT_GUIDER_FLIP_REVERSES_DEC", "DISABLED", true, INDIGO_OK_STATE));
	n = value(AGENT, "AGENT_GUIDER_STATS", "FRAME");
	ASSERT_TRUE(frames(n + 3));
	pthread_mutex_lock(&motion_mutex);
	flipped_dec = pulse_dec;
	pthread_mutex_unlock(&motion_mutex);
	ASSERT_TRUE(abort_running());
	ASSERT_TRUE(dec * flipped_dec < 0);
}

static void declination_scaling(void) {
	ASSERT_TRUE(configured_guiding());
	ASSERT_TRUE(num(AGENT, "AGENT_GUIDER_SETTINGS", "MAX_PULSE", 1));
	ASSERT_TRUE(run("GUIDING", INDIGO_BUSY_STATE));
	ASSERT_TRUE(frames(3));
	freeze_motion = true;
	move_image(1, 0);
	ASSERT_TRUE(frames(6));
	pthread_mutex_lock(&motion_mutex);
	double equator = pulse_ra;
	pthread_mutex_unlock(&motion_mutex);
	ASSERT_TRUE(num(AGENT, "AGENT_GUIDER_MOUNT_COORDINATES", "DEC", 60));
	unsigned n = value(AGENT, "AGENT_GUIDER_STATS", "FRAME");
	ASSERT_TRUE(frames(n + 3));
	pthread_mutex_lock(&motion_mutex);
	double high_dec = pulse_ra;
	pthread_mutex_unlock(&motion_mutex);
	ASSERT_TRUE(abort_running());
	ASSERT_TRUE(fabs(equator) > 10);
	ASSERT_NEAR(2, high_dec / equator, 0.05);
}

static void pulse_thresholds(void) {
	ASSERT_TRUE(configured_guiding());
	ASSERT_TRUE(num(AGENT, "AGENT_GUIDER_SETTINGS", "MIN_ERROR", 2));
	ASSERT_TRUE(run("GUIDING", INDIGO_BUSY_STATE));
	ASSERT_TRUE(frames(3));
	move_image(1, 1);
	ASSERT_TRUE(frames(6));
	pthread_mutex_lock(&motion_mutex);
	unsigned count = ra_commands + dec_commands;
	pthread_mutex_unlock(&motion_mutex);
	ASSERT_EQ_INT(0, count);
	ASSERT_TRUE(num(AGENT, "AGENT_GUIDER_SETTINGS", "MIN_ERROR", 0));
	ASSERT_TRUE(num(AGENT, "AGENT_GUIDER_SETTINGS", "MIN_PULSE", 0.1));
	ASSERT_TRUE(frames(10));
	pthread_mutex_lock(&motion_mutex);
	count = ra_commands + dec_commands;
	pthread_mutex_unlock(&motion_mutex);
	ASSERT_TRUE(abort_running());
	ASSERT_EQ_INT(0, count);
}

static void ppec_learning_reset(void) {
	ASSERT_TRUE(configured_guiding());
	ASSERT_TRUE(sw(AGENT, "AGENT_GUIDER_CORRECTION_MODE_RA", "PPEC", true, INDIGO_OK_STATE));
	ASSERT_TRUE(num(AGENT, "AGENT_GUIDER_SETTINGS", "PPEC_PERIOD_RA", 10));
	ASSERT_TRUE(num(AGENT, "AGENT_GUIDER_SETTINGS", "PPEC_PERIOD_FIXED", 1));
	ASSERT_TRUE(num(AGENT, "AGENT_GUIDER_MOUNT_COORDINATES", "SIDE_OF_PIER", 1));
	ASSERT_TRUE(run("GUIDING", INDIGO_BUSY_STATE));
	ASSERT_TRUE(frames(3));
	freeze_motion = true;
	move_image(1, 0);
	ASSERT_TRUE(frames(30));
	ASSERT_TRUE(value(AGENT, "AGENT_GUIDER_STATS", "PPEC_LEARNING") > 0);
	ASSERT_TRUE(sw(AGENT, "AGENT_GUIDER_RESET_PPEC", "RESET", true, INDIGO_OK_STATE));
	ASSERT_EQ_INT(0, value(AGENT, "AGENT_GUIDER_RESET_PPEC", "RESET"));
	ASSERT_TRUE(abort_running());
	ASSERT_TRUE(sw(AGENT, "AGENT_GUIDER_RESET_PPEC", "RESET", true, INDIGO_OK_STATE));
	ASSERT_EQ_INT(0, value(AGENT, "AGENT_GUIDER_STATS", "PPEC_LEARNING"));
	ASSERT_TRUE(run("GUIDING", INDIGO_BUSY_STATE));
	ASSERT_TRUE(frames(3));
	ASSERT_TRUE(abort_running());
}

static void shutdown_active(void) {
	ASSERT_TRUE(configured_guiding());
	ASSERT_TRUE(run("GUIDING", INDIGO_BUSY_STATE));
	ASSERT_TRUE(frames(3));
	ASSERT_EQ_INT(INDIGO_OK, indigo_agent_guider(INDIGO_DRIVER_SHUTDOWN, NULL));
	agent_started = false;
	ASSERT_EQ_INT(INDIGO_OK, indigo_agent_guider(INDIGO_DRIVER_INIT, NULL));
	agent_started = true;
	ASSERT_TRUE(wait_state(AGENT, "AGENT_START_PROCESS", 0, INDIGO_OK_STATE));
	ASSERT_TRUE(configured_guiding());
	ASSERT_TRUE(run("GUIDING", INDIGO_BUSY_STATE));
	ASSERT_TRUE(frames(3));
	ASSERT_TRUE(abort_running());
}

static void shutdown_subframe(void) {
	ASSERT_TRUE(configured_guiding());
	ASSERT_TRUE(num(AGENT, "AGENT_GUIDER_SELECTION", "SUBFRAME", 4));
	ASSERT_TRUE(run("GUIDING", INDIGO_BUSY_STATE));
	ASSERT_TRUE(frames(3));
	ASSERT_TRUE(value(AGENT, "CCD_FRAME", "WIDTH") < 400);
	ASSERT_EQ_INT(INDIGO_OK, indigo_agent_guider(INDIGO_DRIVER_SHUTDOWN, NULL));
	agent_started = false;
	ASSERT_EQ_INT(INDIGO_OK, indigo_agent_guider(INDIGO_DRIVER_INIT, NULL));
	agent_started = true;
	ASSERT_TRUE(configured_guiding());
	ASSERT_EQ_INT(400, value(AGENT, "CCD_FRAME", "WIDTH"));
	ASSERT_EQ_INT(300, value(AGENT, "CCD_FRAME", "HEIGHT"));
	ASSERT_TRUE(run("GUIDING", INDIGO_BUSY_STATE));
	ASSERT_TRUE(frames(3));
	ASSERT_TRUE(abort_running());
}

static void shutdown_exposure(void) {
	ASSERT_TRUE(configured_guiding());
	ASSERT_TRUE(num(AGENT, "AGENT_GUIDER_SETTINGS", "EXPOSURE", 30));
	ASSERT_TRUE(run("PREVIEW", INDIGO_BUSY_STATE));
	ASSERT_TRUE(wait_state(CAMERA, "CCD_EXPOSURE", 0, INDIGO_BUSY_STATE));
	int before = camera_abort_requests;
	double start = indigo_monotonic_time();
	ASSERT_EQ_INT(INDIGO_OK, indigo_agent_guider(INDIGO_DRIVER_SHUTDOWN, NULL));
	agent_started = false;
	ASSERT_TRUE(indigo_monotonic_time() - start < 5);
	ASSERT_TRUE(camera_abort_requests > before);
	ASSERT_EQ_INT(INDIGO_OK, indigo_agent_guider(INDIGO_DRIVER_INIT, NULL));
	agent_started = true;
	ASSERT_TRUE(configured_guiding());
	ASSERT_TRUE(run("GUIDING", INDIGO_BUSY_STATE));
	ASSERT_TRUE(frames(3));
	ASSERT_TRUE(abort_running());
}

static void active_instances(bool shutdown) {
	ASSERT_TRUE(configured_guiding());
	ASSERT_TRUE(num(AGENT, "ADDITIONAL_INSTANCES", "COUNT", 1));
	const char *second = "Guider Agent #2";
	ASSERT_TRUE(wait_state(second, "AGENT_START_PROCESS", 0, INDIGO_OK_STATE));
	ASSERT_TRUE(sw(second, "FILTER_CCD_LIST", CCD_SIMULATOR_IMAGER_CAMERA_NAME, true, INDIGO_OK_STATE));
	ASSERT_TRUE(num(second, "AGENT_GUIDER_SETTINGS", "EXPOSURE", 0.1));
	ASSERT_TRUE(run("GUIDING", INDIGO_BUSY_STATE));
	ASSERT_TRUE(frames(3));
	unsigned before = revision(second, "CCD_IMAGE");
	ASSERT_TRUE(sw(second, "AGENT_START_PROCESS", "PREVIEW", true, INDIGO_BUSY_STATE));
	ASSERT_TRUE(wait_state(second, "CCD_IMAGE", before, INDIGO_OK_STATE));
	if (shutdown) {
		ASSERT_EQ_INT(INDIGO_OK, indigo_agent_guider(INDIGO_DRIVER_SHUTDOWN, NULL));
		agent_started = false;
		ASSERT_EQ_INT(INDIGO_OK, indigo_agent_guider(INDIGO_DRIVER_INIT, NULL));
		agent_started = true;
		ASSERT_TRUE(wait_state(AGENT, "AGENT_START_PROCESS", 0, INDIGO_OK_STATE));
		ASSERT_TRUE(configured_guiding());
		ASSERT_TRUE(run("GUIDING", INDIGO_BUSY_STATE));
		ASSERT_TRUE(frames(3));
	} else {
		ASSERT_TRUE(num(AGENT, "ADDITIONAL_INSTANCES", "COUNT", 0));
		ASSERT_EQ_INT(-1, state(second, "AGENT_START_PROCESS"));
		ASSERT_EQ_INT(INDIGO_BUSY_STATE, state(AGENT, "AGENT_START_PROCESS"));
		ASSERT_TRUE(frames(value(AGENT, "AGENT_GUIDER_STATS", "FRAME") + 3));
		ASSERT_TRUE(num(AGENT, "ADDITIONAL_INSTANCES", "COUNT", 1));
		ASSERT_TRUE(wait_state(second, "AGENT_START_PROCESS", 0, INDIGO_OK_STATE));
	}
	ASSERT_TRUE(abort_running());
	ASSERT_TRUE(num(AGENT, "ADDITIONAL_INSTANCES", "COUNT", 0));
}

static void shutdown_instances(void) {
	active_instances(true);
}

static void remove_active_instance(void) {
	active_instances(false);
}

static void simultaneous_agents(void) {
	ASSERT_TRUE(connect_camera());
	ASSERT_TRUE(num(AGENT, "ADDITIONAL_INSTANCES", "COUNT", 1));
	const char *second = "Guider Agent #2";
	ASSERT_TRUE(wait_state(second, "AGENT_START_PROCESS", 0, INDIGO_OK_STATE));
	ASSERT_TRUE(sw(second, "FILTER_CCD_LIST", CCD_SIMULATOR_IMAGER_CAMERA_NAME, true, INDIGO_OK_STATE));
	ASSERT_TRUE(num(second, "AGENT_GUIDER_SETTINGS", "EXPOSURE", 0.1));
	ASSERT_TRUE(run("PREVIEW", INDIGO_BUSY_STATE));
	ASSERT_TRUE(sw(second, "AGENT_START_PROCESS", "PREVIEW", true, INDIGO_BUSY_STATE));
	ASSERT_TRUE(wait_images(3));
	ASSERT_TRUE(abort_running());
	ASSERT_EQ_INT(INDIGO_BUSY_STATE, state(second, "AGENT_START_PROCESS"));
	unsigned before = revision(second, "CCD_IMAGE");
	ASSERT_TRUE(wait_state(second, "CCD_IMAGE", before, INDIGO_OK_STATE));
	ASSERT_TRUE(sw(second, "AGENT_ABORT_PROCESS", "ABORT", true, INDIGO_OK_STATE));
	ASSERT_TRUE(wait_state(second, "AGENT_START_PROCESS", 0, -1));
	ASSERT_TRUE(num(AGENT, "ADDITIONAL_INSTANCES", "COUNT", 0));
}

typedef struct {
	indigo_device device;
	indigo_property *abort_related;
	bool attached;
	unsigned enables, disables;
} related_peer;
static related_peer mount_peer, imager_peer, unrelated_peer;

static indigo_result related_enumerate(indigo_device *device, indigo_client *sender, indigo_property *property) {
	related_peer *p = device->private_data;
	if (indigo_property_match(p->abort_related, property)) {
		indigo_define_property(device, p->abort_related, NULL);
	}
	return indigo_device_enumerate_properties(device, sender, property);
}

static indigo_result related_attach(indigo_device *device) {
	indigo_device_attach(device, "guider_test_peer", 1, INDIGO_INTERFACE_AGENT);
	related_peer *p = device->private_data;
	p->abort_related = indigo_init_switch_property(NULL, device->name, AGENT_ABORT_RELATED_PROCESS_PROPERTY_NAME, "Test", "Abort related", INDIGO_OK_STATE, INDIGO_RW_PERM, INDIGO_ANY_OF_MANY_RULE, 1);
	indigo_init_switch_item(p->abort_related->items, "GUIDER", "Guider", false);
	return related_enumerate(device, NULL, NULL);
}

static indigo_result related_change(indigo_device *device, indigo_client *sender, indigo_property *property) {
	related_peer *p = device->private_data;
	if (indigo_property_match(p->abort_related, property)) {
		indigo_property_copy_values(p->abort_related, property, false);
		if (p->abort_related->items[0].sw.value) {
			p->enables++;
		} else {
			p->disables++;
		}
		indigo_update_property(device, p->abort_related, NULL);
		return INDIGO_OK;
	}
	return indigo_device_change_property(device, sender, property);
}

static indigo_result related_detach(indigo_device *device) {
	related_peer *p = device->private_data;
	indigo_delete_property(device, p->abort_related, NULL);
	indigo_release_property(p->abort_related);
	return indigo_device_detach(device);
}

static bool attach_related(related_peer *p, const char *name) {
	p->device = (indigo_device){ .attach = related_attach, .enumerate_properties = related_enumerate, .change_property = related_change, .detach = related_detach, .private_data = p };
	strcpy(p->device.name, name);
	REQUIRE(indigo_attach_device(&p->device) == INDIGO_OK);
	p->attached = true;
	return true;
}

static void cleanup_related(void) {
	related_peer *peers[] = { &mount_peer, &imager_peer, &unrelated_peer };
	for (int i = 0; i < ARRAY_SIZE(peers); i++) {
		if (peers[i]->attached) {
			indigo_detach_device(&peers[i]->device);
		}
	}
}

static void related_mount_coordination(void) {
	ASSERT_TRUE(attach_related(&mount_peer, "Mount Agent Test"));
	ASSERT_TRUE(attach_related(&imager_peer, "Imager Agent Test"));
	ASSERT_TRUE(attach_related(&unrelated_peer, "Unrelated Agent Test"));
	ASSERT_TRUE(sw(AGENT, "FILTER_RELATED_AGENT_LIST", "Mount Agent Test", true, INDIGO_OK_STATE));
	ASSERT_TRUE(sw(AGENT, "FILTER_RELATED_AGENT_LIST", "Imager Agent Test", true, INDIGO_OK_STATE));
	ASSERT_TRUE(isnan(value(AGENT, "FILTER_RELATED_AGENT_LIST", "Unrelated Agent Test")));
	ASSERT_TRUE(configured_guiding());
	ASSERT_TRUE(run("GUIDING", INDIGO_BUSY_STATE));
	ASSERT_TRUE(frames(3));
	ASSERT_EQ_INT(1, value("Mount Agent Test", AGENT_ABORT_RELATED_PROCESS_PROPERTY_NAME, "GUIDER"));
	ASSERT_TRUE(abort_running());
	ASSERT_EQ_INT(0, value("Mount Agent Test", AGENT_ABORT_RELATED_PROCESS_PROPERTY_NAME, "GUIDER"));
	ASSERT_TRUE(mount_peer.enables > 0 && mount_peer.disables > 0);
}

static void zero_drift_statistics(void) {
	ASSERT_TRUE(configured_guiding());
	freeze_motion = true;
	ASSERT_TRUE(run("GUIDING", INDIGO_BUSY_STATE));
	ASSERT_TRUE(frames(3));
	move_image(2, 2);
	ASSERT_TRUE(frames(6));
	ASSERT_TRUE(fabs(value(AGENT, "AGENT_GUIDER_STATS", "DRIFT_RA")) > 1);
	move_image(0, 0);
	ASSERT_TRUE(frames(10));
	ASSERT_TRUE(abort_running());
	ASSERT_NEAR(0, value(AGENT, "AGENT_GUIDER_STATS", "DRIFT_RA"), 0.001);
	ASSERT_NEAR(0, value(AGENT, "AGENT_GUIDER_STATS", "CORR_RA"), 0.001);
}

static void donuts_include_region(void) {
	ASSERT_TRUE(configured_guiding());
	ASSERT_TRUE(sw(AGENT, "AGENT_GUIDER_DETECTION_MODE", "DONUTS", true, INDIGO_OK_STATE));
	ASSERT_TRUE(sw(AGENT, "AGENT_PROCESS_FEATURES", "USE_INCLUDE_FOR_DONUTS", true, INDIGO_OK_STATE));
	ASSERT_TRUE(num(AGENT, "AGENT_GUIDER_SELECTION", "INCLUDE_WIDTH", 300));
	ASSERT_TRUE(num(AGENT, "AGENT_GUIDER_SELECTION", "INCLUDE_HEIGHT", 230));
	ASSERT_TRUE(run("GUIDING", INDIGO_BUSY_STATE));
	ASSERT_TRUE(frames(3));
	move_image(2, 2);
	ASSERT_TRUE(frames(10));
	ASSERT_TRUE(abort_running());
	ASSERT_TRUE(isfinite(value(AGENT, "AGENT_GUIDER_STATS", "SNR")));
}

static void backlash_reversal(void) {
	ASSERT_TRUE(configured_guiding());
	ASSERT_TRUE(sw(AGENT, "AGENT_GUIDER_APPLY_DEC_BACKLASH", "ENABLED", true, INDIGO_OK_STATE));
	ASSERT_TRUE(num(AGENT, "AGENT_GUIDER_SETTINGS", "BACKLASH", 2));
	freeze_motion = true;
	ASSERT_TRUE(run("GUIDING", INDIGO_BUSY_STATE));
	ASSERT_TRUE(frames(3));
	move_image(0, 2);
	ASSERT_TRUE(frames(6));
	move_image(0, -2);
	ASSERT_TRUE(frames(9));
	move_image(0, 2);
	ASSERT_TRUE(frames(12));
	ASSERT_TRUE(abort_running());
	pthread_mutex_lock(&motion_mutex);
	double maximum = maximum_pulse;
	pthread_mutex_unlock(&motion_mutex);
	ASSERT_NEAR(300, maximum, 1);
}

static void pi_integral_history(void) {
	ASSERT_TRUE(configured_guiding());
	ASSERT_TRUE(num(AGENT, "AGENT_GUIDER_SETTINGS", "STACK", 5));
	ASSERT_TRUE(num(AGENT, "AGENT_GUIDER_SETTINGS", "I_GAIN_RA", 0.5));
	ASSERT_TRUE(num(AGENT, "AGENT_GUIDER_SETTINGS", "I_GAIN_DEC", 0.5));
	ASSERT_TRUE(run("GUIDING", INDIGO_BUSY_STATE));
	ASSERT_TRUE(frames(3));
	move_image(2, 2);
	ASSERT_TRUE(frames(15));
	ASSERT_TRUE(abort_running());
	pthread_mutex_lock(&motion_mutex);
	double residual = hypot(offset_x, offset_y);
	pthread_mutex_unlock(&motion_mutex);
	ASSERT_TRUE(residual < 0.5);
}

static void logging_algorithms(void) {
	ASSERT_TRUE(configured_guiding());
	ASSERT_TRUE(txt(AGENT, "AGENT_GUIDER_LOG", "DIR", config_folder, INDIGO_OK_STATE));
	ASSERT_TRUE(sw(AGENT, "AGENT_PROCESS_FEATURES", "ENABLE_LOGGING", true, INDIGO_OK_STATE));
	const char *modes[] = { "HYSTERESIS", "LINEAR_TREND", "PPEC" };
	for (int i = 0; i < ARRAY_SIZE(modes); i++) {
		ASSERT_TRUE(sw(AGENT, "AGENT_GUIDER_CORRECTION_MODE_RA", modes[i], true, INDIGO_OK_STATE));
		ASSERT_TRUE(sw(AGENT, "AGENT_GUIDER_CORRECTION_MODE_DEC", i == 2 ? "RESIST_SWITCH" : modes[i], true, INDIGO_OK_STATE));
		ASSERT_TRUE(run("GUIDING", INDIGO_BUSY_STATE));
		ASSERT_TRUE(frames(3));
		ASSERT_TRUE(abort_running());
	}
}

static void calibration_reset_recovery(void) {
	ASSERT_TRUE(model_camera());
	ASSERT_TRUE(num(AGENT, "AGENT_GUIDER_SETTINGS", "MIN_CALIBRATION_DRIFT", 10));
	blank_image = true;
	ASSERT_TRUE(run("CALIBRATION", INDIGO_BUSY_STATE));
	ASSERT_TRUE(wait_images(3));
	blank_image = false;
	ASSERT_TRUE(wait_state(AGENT, "AGENT_START_PROCESS", 0, INDIGO_OK_STATE));
}

static void live_dec_mode_guards(void) {
	ASSERT_TRUE(configured_guiding());
	ASSERT_TRUE(run("GUIDING", INDIGO_BUSY_STATE));
	ASSERT_TRUE(frames(3));
	ASSERT_TRUE(sw(AGENT, "AGENT_GUIDER_DEC_MODE", "NORTH", true, INDIGO_OK_STATE));
	ASSERT_EQ_INT(1, value(AGENT, "AGENT_GUIDER_DEC_MODE", "BOTH"));
	ASSERT_TRUE(abort_running());
	ASSERT_TRUE(sw(AGENT, "AGENT_GUIDER_DEC_MODE", "NORTH", true, INDIGO_OK_STATE));
	ASSERT_TRUE(run("GUIDING", INDIGO_BUSY_STATE));
	ASSERT_TRUE(frames(3));
	ASSERT_TRUE(sw(AGENT, "AGENT_GUIDER_DEC_MODE", "SOUTH", true, INDIGO_OK_STATE));
	ASSERT_EQ_INT(1, value(AGENT, "AGENT_GUIDER_DEC_MODE", "SOUTH"));
	ASSERT_TRUE(sw(AGENT, "AGENT_GUIDER_DEC_MODE", "BOTH", true, INDIGO_OK_STATE));
	ASSERT_EQ_INT(1, value(AGENT, "AGENT_GUIDER_DEC_MODE", "SOUTH"));
	ASSERT_TRUE(sw(AGENT, "AGENT_GUIDER_DEC_MODE", "NONE", true, INDIGO_OK_STATE));
	ASSERT_EQ_INT(1, value(AGENT, "AGENT_GUIDER_DEC_MODE", "NONE"));
	ASSERT_TRUE(abort_running());
}

static void camera_selection_reset(void) {
	ASSERT_TRUE(connect_camera());
	ASSERT_TRUE(num(AGENT, "AGENT_GUIDER_SELECTION", "X", 110));
	ASSERT_TRUE(num(AGENT, "AGENT_GUIDER_SELECTION", "Y", 100));
	// Reselecting the same camera is synchronous and intentionally emits no update.
	ASSERT_EQ_INT(INDIGO_OK, indigo_change_switch_property_1(&client, AGENT, "FILTER_CCD_LIST", CAMERA, true));
	ASSERT_EQ_INT(110, value(AGENT, "AGENT_GUIDER_SELECTION", "X"));
	ASSERT_EQ_INT(100, value(AGENT, "AGENT_GUIDER_SELECTION", "Y"));
	ASSERT_TRUE(sw(AGENT, "FILTER_CCD_LIST", CCD_SIMULATOR_IMAGER_CAMERA_NAME, true, INDIGO_OK_STATE));
	ASSERT_EQ_INT(0, value(AGENT, "AGENT_GUIDER_SELECTION", "X"));
	ASSERT_EQ_INT(0, value(AGENT, "AGENT_GUIDER_SELECTION", "Y"));
	ASSERT_TRUE(connect_camera());
	ASSERT_TRUE(run("PREVIEW", INDIGO_BUSY_STATE));
	ASSERT_TRUE(wait_images(2));
	ASSERT_TRUE(abort_running());
}

static void spiral_sequence_reset(void) {
	ASSERT_TRUE(configured_guiding());
	ASSERT_TRUE(num(AGENT, "AGENT_GUIDER_SETTINGS", "DITHERING_MAX_AMOUNT", 2));
	ASSERT_TRUE(num(AGENT, "AGENT_GUIDER_SETTINGS", "DITHERING_LIMIT", 2));
	ASSERT_TRUE(sw(AGENT, "AGENT_GUIDER_DITHERING_STRATEGY", "SPIRAL", true, INDIGO_OK_STATE));
	ASSERT_TRUE(run("GUIDING", INDIGO_BUSY_STATE));
	ASSERT_TRUE(frames(4));
	const double expected[][2] = { { -1, 1 }, { 1, 1 }, { 1, -1 }, { -1, -1 }, { -2, 2 } };
	for (int i = 0; i < ARRAY_SIZE(expected); i++) {
		ASSERT_TRUE(sw(AGENT, "AGENT_GUIDER_DITHER", "TRIGGER", true, INDIGO_OK_STATE));
		ASSERT_NEAR(expected[i][0], value(AGENT, "AGENT_GUIDER_DITHERING_OFFSETS", "X"), 0.001);
		ASSERT_NEAR(expected[i][1], value(AGENT, "AGENT_GUIDER_DITHERING_OFFSETS", "Y"), 0.001);
		ASSERT_EQ_INT(0, value(AGENT, "AGENT_GUIDER_STATS", "DITHERING"));
	}
	ASSERT_TRUE(sw(AGENT, "AGENT_GUIDER_DITHER", "RESET", true, INDIGO_OK_STATE));
	ASSERT_TRUE(sw(AGENT, "AGENT_GUIDER_DITHER", "TRIGGER", true, INDIGO_OK_STATE));
	ASSERT_NEAR(-1, value(AGENT, "AGENT_GUIDER_DITHERING_OFFSETS", "X"), 0.001);
	ASSERT_NEAR(1, value(AGENT, "AGENT_GUIDER_DITHERING_OFFSETS", "Y"), 0.001);
	ASSERT_TRUE(abort_running());
}

static void logging_open_failure_recovery(void) {
	ASSERT_TRUE(configured_guiding());
	char missing[1024];
	snprintf(missing, sizeof(missing), "%s/missing", config_folder);
	ASSERT_TRUE(txt(AGENT, "AGENT_GUIDER_LOG", "DIR", missing, INDIGO_OK_STATE));
	ASSERT_TRUE(sw(AGENT, "AGENT_PROCESS_FEATURES", "ENABLE_LOGGING", true, INDIGO_OK_STATE));
	ASSERT_TRUE(run("GUIDING", INDIGO_BUSY_STATE));
	ASSERT_TRUE(frames(3));
	ASSERT_TRUE(abort_running());
	logging();
}

static const indigo_test_case tests[] = {
	{ "live dec mode guards", live_dec_mode_guards },
	{ "camera selection reset", camera_selection_reset },
	{ "spiral sequence reset", spiral_sequence_reset },
	{ "logging open failure recovery", logging_open_failure_recovery },
	{ "donuts include region", donuts_include_region },
	{ "backlash reversal", backlash_reversal },
	{ "pi integral history", pi_integral_history },
	{ "logging algorithms", logging_algorithms },
	{ "calibration reset recovery", calibration_reset_recovery },
	{ "zero drift statistics", zero_drift_statistics },
	{ "related mount coordination", related_mount_coordination },
	{ "multistar weighted", multistar_weighted },
	{ "calibration ra only", calibration_ra_only },
	{ "calibration star failure", calibration_star_failure },
	{ "stars abort and failure", stars_abort_and_failure },
	{ "meridian flip", meridian_flip },
	{ "declination scaling", declination_scaling },
	{ "pulse thresholds", pulse_thresholds },
	{ "ppec learning reset", ppec_learning_reset },
	{ "shutdown active", shutdown_active },
	{ "shutdown subframe", shutdown_subframe },
	{ "shutdown exposure", shutdown_exposure },
	{ "shutdown instances", shutdown_instances },
	{ "remove active instance", remove_active_instance },
	{ "simultaneous agents", simultaneous_agents },
	{ "correction response", correction_response },
	{ "dec modes", dec_modes },
	{ "guiding delay abort", guiding_delay_abort },
	{ "calibration and guiding", calibration_and_guiding },
	{ "calibration no motion", calibration_no_motion },
	{ "star loss fail", star_loss_fail },
	{ "star loss continue", star_loss_continue },
	{ "star loss reset", star_loss_reset },
	{ "exposure failure guiding", exposure_failure_guiding },
	{ "invalid raw", invalid_raw },
	{ "truncated raw", truncated_raw },
	{ "raw dimensions and recovery", raw_dimensions },
	{ "raw payload formats and recovery", raw_payload_formats },
	{ "pulse error", pulse_error },
	{ "camera disconnect", camera_disconnect },
	{ "dither abort", dither_abort },
	{ "dither timeout", dither_timeout },
	{ "selection subframe restore", selection_subframe_restore },
	{ "selection regions binning", selection_regions_binning },
	{ "feature policies", feature_policies },
	{ "logging", logging },
	{ "configuration reload", configuration_reload },
	{ "metadata", metadata },
	{ "missing devices", missing_devices },
	{ "single preview status and format restoration", single_preview },
	{ "single preview failure", preview_failure },
	{ "single preview retry", preview_retry },
	{ "single preview invalid raw", preview_invalid_raw },
	{ "continuous preview abort status", continuous_preview_abort },
	{ "continuous preview exposure failure", continuous_preview_exposure_failure },
	{ "continuous preview invalid raw", continuous_preview_invalid_raw },
	{ "preview abort and restart", preview_abort_and_restart },
	{ "stars selection clear and resize", stars_selection },
	{ "selection PI guiding", selection_pi },
	{ "weighted hysteresis guiding", weighted_hysteresis },
	{ "donuts linear trend guiding", donuts_trend },
	{ "centroid resist switch guiding", centroid_resist },
	{ "selection PPEC guiding", selection_ppec },
	{ "guiding preconditions", guiding_preconditions },
	{ "busy guards and deferred PPEC reset", busy_guards },
	{ "calibration", calibration },
	{ "calibration adaptive step", calibration_adaptive_step },
	{ "calibration speed accuracy", calibration_speed_accuracy },
	{ "calibration abort", calibration_abort },
	{ "dither strategies", dither_strategies },
	{ "dither RA projection preserves magnitude", dither_ra_projection },
	{ "reset defaults", reset_defaults },
	{ "additional instances lifecycle", instances }
};

int main(int argc, char **argv) {
	setvbuf(stdout, NULL, _IONBF, 0);
	int result = 0, executed = 0, passed = 0;
	for (int i = 0; i < ARRAY_SIZE(tests); i++) {
		if (argc > 1 && !strstr(tests[i].name, argv[1])) {
			continue;
		}
		executed++;
		strcpy(config_folder, "/tmp/indigo_guider_test_XXXXXX");
		if (!mkdtemp(config_folder)) {
			perror("mkdtemp");
			return 1;
		}
		pid_t child = fork();
		if (child == 0) {
			alarm(120);
			bool ready = setup();
			int status = ready ? indigo_run_tests("Guider Agent integration", tests + i, 1) : 1;
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
	printf("Guider Agent: %d/%d cases passed (including cleanup)\n", passed, executed);
	return executed ? result : 2;
}
