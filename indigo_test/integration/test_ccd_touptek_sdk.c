// Copyright (c) 2026 CloudMakers, s. r. o.
// All rights reserved.
// Use under the INDIGO Astronomy open-source license (see LICENSE.md).
// Test harness extended by OpenAI Codex.

#include <stdatomic.h>
#include <errno.h>
#include <pthread.h>
#include <string.h>
#include <stdlib.h>
#include <toupcam.h>
#include <indigo/indigo_usb_utils.h>
#include <indigo_drivers/ccd_touptek/indigo_ccd_touptek.h>
#include "../test_runner.h"
#include <math.h>
#include <time.h>
#include <dirent.h>
#include <unistd.h>
#include "ccd_test_noise.h"

#define CHECK_TRUE(condition) do { if (!(condition)) { fprintf(stderr, "%s:%d: %s\n", __FILE__, __LINE__, #condition); indigo_test_failures++; goto cleanup; } } while (0)
#define CHECK_EQ_INT(actual, expected) CHECK_TRUE((actual) == (expected))
#define ARRAY_SIZE(array) (sizeof(array) / sizeof((array)[0]))

static libusb_hotplug_callback_fn usb_callback;
static atomic_int visible, attached, opened, closed, locks, wrong_thread, connection[4];
static atomic_bool moving, fail_open, fail_register, fail_queue, hold_focus;
static atomic_bool held[TOUPCAM_MAX], handle_open[TOUPCAM_MAX];
static atomic_int bad_handle, calibrations;
static pthread_t sdk_thread;
static bool have_thread;
static int handles[TOUPCAM_MAX];
static bool inventory_mode, inventory_reverse;
static atomic_int inventory_count;
static atomic_bool inventory_visible[TOUPCAM_MAX];
static indigo_device *inventory_devices[TOUPCAM_MAX][2];
static char inventory_names[TOUPCAM_MAX][2][INDIGO_NAME_SIZE];
static unsigned inventory_frames[TOUPCAM_MAX];
static indigo_device *logical[4];
static indigo_result (*attach_functions[4])(indigo_device *);
static bool combined_model;
static char combined_names[4][INDIGO_NAME_SIZE];
static ToupcamModelV2 models[3] = {
	{ .name = "Camera", .flag = TOUPCAM_FLAG_CMOS | TOUPCAM_FLAG_MONO | TOUPCAM_FLAG_RAW8 | TOUPCAM_FLAG_ST4 | TOUPCAM_FLAG_BLACKLEVEL | TOUPCAM_FLAG_ROI_HARDWARE, .preview = 1, .res = {{640, 480}} },
	{ .name = "Wheel", .flag = TOUPCAM_FLAG_FILTERWHEEL },
	{ .name = "Focuser", .flag = TOUPCAM_FLAG_AUTOFOCUSER }
};

// Record control calls independently of lifecycle calls: CCD and guider share a worker.
static atomic_bool track_controls, fail_control, deliver_image, image_on_stop;
static atomic_int pulled_images, triggers, stream_frames, watchdog_seconds;
static atomic_bool fast_watchdog;
static _Atomic(indigo_timer_callback) monitor_tasks[4], move_tasks[4];
static atomic_int replay_done;
static int index_for(const char *name);
static atomic_int stream_started;
static atomic_int sdk_event = TOUPCAM_EVENT_IMAGE;
static atomic_int stop_event, stop_notifications;
// Test-only gates force queue/callback interleavings; production scheduling is unchanged.
typedef struct {
	pthread_mutex_t mutex;
	pthread_cond_t condition;
	bool armed;
	atomic_int entered;
} test_gate;
#define TEST_GATE_INITIALIZER { PTHREAD_MUTEX_INITIALIZER, PTHREAD_COND_INITIALIZER, false, 0 }
static test_gate callback_gate = TEST_GATE_INITIALIZER;
static test_gate pull_gate = TEST_GATE_INITIALIZER;
static test_gate queue_gate = TEST_GATE_INITIALIZER;
static test_gate connection_gate = TEST_GATE_INITIALIZER;
static atomic_bool immediate_temperature_next;
static atomic_int temperature_reads, pulse_calls;
static atomic_int gate_timeouts, callback_joining, callback_returned, enum_calls, shutdown_done, shutdown_result;
static pthread_t async_callback_thread;
static atomic_bool async_callback_active;
static indigo_queue *test_driver_queue;
static void *deliver_sdk_image(void *unused);

static void arm_gate(test_gate *gate) {
	pthread_mutex_lock(&gate->mutex);
	gate->armed = true;
	atomic_store(&gate->entered, 0);
	pthread_mutex_unlock(&gate->mutex);
}

static void release_gate(test_gate *gate) {
	pthread_mutex_lock(&gate->mutex);
	gate->armed = false;
	pthread_cond_broadcast(&gate->condition);
	pthread_mutex_unlock(&gate->mutex);
}

static void enter_gate(test_gate *gate) {
	struct timespec deadline;
	clock_gettime(CLOCK_REALTIME, &deadline);
	deadline.tv_sec += 10;
	pthread_mutex_lock(&gate->mutex);
	if (gate->armed) {
		atomic_store(&gate->entered, 1);
		while (gate->armed) {
			if (pthread_cond_timedwait(&gate->condition, &gate->mutex, &deadline) == ETIMEDOUT) {
				atomic_fetch_add(&gate_timeouts, 1);
				gate->armed = false;
			}
		}
	}
	pthread_mutex_unlock(&gate->mutex);
}

static void queue_gate_handler(indigo_device *device) {
	enter_gate(&queue_gate);
}

void touptek_test_execute_handler_with_data(indigo_device *device, indigo_timer_with_data_callback handler, void *data) {
	enter_gate(&callback_gate);
	indigo_execute_handler_with_data(device, handler, data);
}

static void emit_image(void);
static PTOUPCAM_EVENT_CALLBACK image_callback;
static void *image_context;
static pthread_mutex_t image_mutex = PTHREAD_MUTEX_INITIALIZER;
static atomic_int control_calls[3], gain_calls, last_gain, pulse_direction, pulse_duration, wheel_command, focus_position;
static pthread_t bus_thread, control_thread[3];
static bool have_control_thread[3];
static pthread_mutex_t control_mutex = PTHREAD_MUTEX_INITIALIZER;
static const char *property_names[] = {
	"CCD_GAIN", "CCD_OFFSET", "CCD_BIN", "CCD_FRAME", "CCD_MODE", "CCD_EXPOSURE", "CCD_ABORT_EXPOSURE",
	"GUIDER_GUIDE_RA", "GUIDER_GUIDE_DEC", "WHEEL_SLOT", "X_WHEEL_MODEL", "FOCUSER_POSITION", "FOCUSER_STEPS",
	"FOCUSER_LIMITS", "FOCUSER_ABORT_MOTION", "FOCUSER_COMPENSATION", "FOCUSER_MODE", "CCD_STREAMING"
};
static atomic_int property_state[18], property_alerts[18], property_values[18][6];

static void control_call(HToupcam h) {
	if (!atomic_load(&track_controls)) { return; }
	if (h == NULL) { atomic_fetch_add(&bad_handle, 1); return; }
	int index = (int *)h - handles;
	if (index < 0 || index > 2 || !atomic_load(&handle_open[index])) {
		atomic_fetch_add(&bad_handle, 1);
		return;
	}
	// Connection initialization is serialized on the SDK lifecycle queue.
	if (pthread_equal(sdk_thread, pthread_self()) && ((index == 0 && (atomic_load(&connection[0]) != 1 || atomic_load(&connection[1]) == 2)) || (index > 0 && atomic_load(&connection[index + 1]) != 1))) { return; }
	pthread_mutex_lock(&control_mutex);
	if (pthread_equal(bus_thread, pthread_self()) || pthread_equal(sdk_thread, pthread_self())) {
		atomic_fetch_add(&wrong_thread, 1);
	}
	if (!have_control_thread[index]) {
		control_thread[index] = pthread_self();
		have_control_thread[index] = true;
	} else if (!pthread_equal(control_thread[index], pthread_self())) {
		atomic_fetch_add(&wrong_thread, 1);
	}
	pthread_mutex_unlock(&control_mutex);
	atomic_fetch_add(&control_calls[index], 1);
}

static void check_thread(void) {
	if (!have_thread) {
		sdk_thread = pthread_self();
		have_thread = true;
	} else if (!pthread_equal(sdk_thread, pthread_self())) {
		atomic_fetch_add(&wrong_thread, 1);
	}
}

void touptek_test_execute_handler_in(indigo_device *device, double delay, indigo_timer_callback handler) {
	int index = index_for(device->name);
	if (index >= 0) {
		if ((index == 0 && delay == 5) || (index == 3 && delay == 2)) {
			atomic_store(&monitor_tasks[index], handler);
			if (index == 0 && atomic_exchange(&immediate_temperature_next, false)) {
				delay = 0;
			}
		} else if ((index == 2 || index == 3) && delay == 0.5) {
			atomic_store(&move_tasks[index], handler);
		}
	}
	if (delay >= 25) {
		atomic_store(&watchdog_seconds, delay);
		if (atomic_load(&fast_watchdog)) { delay = 0.05; }
	}
	indigo_execute_handler_in(device, delay, handler);
}

static int index_for(const char *name) {
	if (inventory_mode) {
		for (int i = 0; i < 2; i++) {
			if (!strcmp(name, inventory_names[i][0])) { return i; }
		}
		return -1;
	}
	if (combined_model) {
		for (int i = 0; i < 4; i++) {
			if (!strcmp(name, combined_names[i])) {
				return i;
			}
		}
		return -1;
	}
	if (strstr(name, "Wheel")) { return 2; }
	if (strstr(name, "Focuser")) { return 3; }
	return strstr(name, "guider") ? 1 : (strstr(name, "Camera") ? 0 : -1);
}

// Isolate real framework CONFIG and image files without changing the user's HOME.
static char test_output_folder[] = "/tmp/indigo_touptek_XXXXXX";

static const char *config_folder;

static void touptek_test_set_config_folder(const char *folder) {
	config_folder = folder;
}

const char *touptek_test_config_folder(void) {
	return config_folder;
}

#define PROPERTY_CAPACITY 128
static atomic_llong guide_started_ns[2];
static atomic_int guide_requested_ms[2];
static double guide_errors_ms[128];
static int guide_samples;

static long long monotonic_ns(void) {
	struct timespec now;
	clock_gettime(CLOCK_MONOTONIC, &now);
	return (long long)now.tv_sec * 1000000000LL + now.tv_nsec;
}

static pthread_mutex_t property_mutex = PTHREAD_MUTEX_INITIALIZER;
static indigo_property *observed[4][PROPERTY_CAPACITY];
static unsigned revisions[4][PROPERTY_CAPACITY];
static bool defined[4][PROPERTY_CAPACITY];
static atomic_int blobs, invalid_blobs, raw_blobs, fits_blobs;
static atomic_int option_values[256], option_calls[256], advanced_values[9];
static atomic_int aaf_values[64], last_exposure, target_temperature, fail_option, fail_aaf;
static atomic_int roi_left, roi_top, roi_width, roi_height, wheel_slots, ambient_temperature, ambient_reads;
static atomic_bool fail_pull, fail_trigger, fail_attach;
static atomic_uint raw_fourcc;
static atomic_int read_failure, failed_get_option = -1, sensor_temperature;
static atomic_bool independent_temperature;
static atomic_int fail_attach_index, registrations, deregistrations;

static int property_slot(int index, const char *name) {
	for (int i = 0; i < PROPERTY_CAPACITY; i++) {
		if (observed[index][i] && !strcmp(observed[index][i]->name, name)) {
			return i;
		}
	}
	return -1;
}

static void observe_property(indigo_device *device, indigo_property *property, bool definition) {
	if (inventory_mode) {
		int i = index_for(property->device);
		if (i >= 0 && !strcmp(property->name, "CCD_IMAGE") && property->state == INDIGO_OK_STATE && property->items[0].blob.size) { inventory_frames[i]++; }
	}
	if (strcmp(device->name, property->device)) {
		return;
	}
	int index = index_for(device->name);
	if (index < 0) {
		return;
	}
	long long received_ns = monotonic_ns();
	pthread_mutex_lock(&property_mutex);
	if (index == 1 && property->type == INDIGO_NUMBER_VECTOR && property->state == INDIGO_OK_STATE && property->count == 2 && property->items[0].number.value == 0 && property->items[1].number.value == 0) {
		int axis = !strcmp(property->name, "GUIDER_GUIDE_RA") ? 0 : (!strcmp(property->name, "GUIDER_GUIDE_DEC") ? 1 : -1);
		if (axis >= 0) {
			long long started = atomic_exchange(&guide_started_ns[axis], 0);
			if (started && guide_samples < 128) {
				guide_errors_ms[guide_samples++] = (received_ns - started) / 1000000.0 - atomic_load(&guide_requested_ms[axis]);
			}
		}
	}
	int slot = property_slot(index, property->name);
	if (slot < 0) {
		for (int i = 0; i < PROPERTY_CAPACITY; i++) {
			if (observed[index][i] == NULL) {
				slot = i;
				break;
			}
		}
	}
	if (slot >= 0) {
		indigo_release_property(observed[index][slot]);
		observed[index][slot] = indigo_copy_property(NULL, property);
		revisions[index][slot]++;
		if (definition) {
			defined[index][slot] = true;
		}
	}
	if (!definition && !strcmp(property->name, "CCD_IMAGE") && property->state == INDIGO_OK_STATE && property->count) {
		indigo_item *item = property->items;
		if (item->blob.value && item->blob.size > 0) {
			atomic_fetch_add(&blobs, 1);
			if (!strcmp(item->blob.format, ".raw")) {
				indigo_raw_header header;
				if (item->blob.size < sizeof(header)) {
					atomic_fetch_add(&invalid_blobs, 1);
				} else {
					memcpy(&header, item->blob.value, sizeof(header));
					int bytes = header.signature == INDIGO_RAW_MONO8 ? 1 : (header.signature == INDIGO_RAW_MONO16 ? 2 : (header.signature == INDIGO_RAW_RGB24 ? 3 : 0));
					int bin = atomic_load(&option_values[TOUPCAM_OPTION_BINNING]) & 0x3f;
					int width = atomic_load(&roi_width) / bin, height = atomic_load(&roi_height) / bin;
					if (!bytes || header.width != width || header.height != height || item->blob.size < sizeof(header) + width * height * bytes) {
						atomic_fetch_add(&invalid_blobs, 1);
					} else {
						const unsigned char *pixels = (unsigned char *)item->blob.value + sizeof(header);
						for (int i = 0; i < width * height; i++) {
							unsigned source = (atomic_load(&roi_top) + i / width * bin) * 640 + atomic_load(&roi_left) + i % width * bin;
							unsigned short mono = ccd_test_noise(source, 0);
							bool match = bytes == 3 ? (pixels[3 * i] == (ccd_test_noise(source, 1) >> 8) && pixels[3 * i + 1] == (ccd_test_noise(source, 2) >> 8) && pixels[3 * i + 2] == (ccd_test_noise(source, 3) >> 8)) : (bytes == 2 ? !memcmp(pixels + 2 * i, &mono, 2) : pixels[i] == (mono >> 8));
							if (!match) {
								atomic_fetch_add(&invalid_blobs, 1);
								break;
							}
						}
					}
				}
				unsigned fourcc = atomic_load(&raw_fourcc);
				bool raw_mode = atomic_load(&option_values[TOUPCAM_OPTION_RAW]);
				const char *appendix = (const char *)item->blob.value + sizeof(header) + (size_t)header.width * header.height * (header.signature == INDIGO_RAW_MONO8 ? 1 : header.signature == INDIGO_RAW_MONO16 ? 2 : 3);
				size_t remaining = (const char *)item->blob.value + item->blob.size - appendix;
				bool bayer = memmem(appendix, remaining, "BAYERPAT=", 9) != NULL;
				char pattern[4] = { fourcc, fourcc >> 8, fourcc >> 16, fourcc >> 24 };
				if (bayer != (raw_mode && fourcc != 0) || (bayer && !memmem(appendix, remaining, pattern, 4))) { fprintf(stderr, "Bayer: SDK %x raw %d, appendix %.*s\n", fourcc, raw_mode, (int)remaining, appendix); atomic_fetch_add(&invalid_blobs, 1); }
				atomic_fetch_add(&raw_blobs, 1);
			} else if (!strcmp(item->blob.format, ".fits")) {
				if (item->blob.size < 2880 || memcmp(item->blob.value, "SIMPLE  =", 9)) {
					atomic_fetch_add(&invalid_blobs, 1);
				}
				atomic_fetch_add(&fits_blobs, 1);
			}
		}
	}
	pthread_mutex_unlock(&property_mutex);
}

static indigo_result define_property(indigo_client *client, indigo_device *device, indigo_property *property, const char *message) {
	observe_property(device, property, true);
	return INDIGO_OK;
}

static indigo_result delete_property(indigo_client *client, indigo_device *device, indigo_property *property, const char *message) {
	int index = index_for(device->name);
	if (index < 0) {
		return INDIGO_OK;
	}
	pthread_mutex_lock(&property_mutex);
	for (int i = 0; i < PROPERTY_CAPACITY; i++) {
		if (observed[index][i] && (!*property->name || !strcmp(observed[index][i]->name, property->name))) {
			defined[index][i] = false;
		}
	}
	pthread_mutex_unlock(&property_mutex);
	return INDIGO_OK;
}

static void clear_observer(void) {
	pthread_mutex_lock(&property_mutex);
	for (int d = 0; d < 4; d++) {
		for (int i = 0; i < PROPERTY_CAPACITY; i++) {
			indigo_release_property(observed[d][i]);
			observed[d][i] = NULL;
			revisions[d][i] = 0;
			defined[d][i] = false;
		}
	}
	pthread_mutex_unlock(&property_mutex);
}

static indigo_property *snapshot(int index, const char *name) {
	pthread_mutex_lock(&property_mutex);
	int slot = property_slot(index, name);
	indigo_property *copy = slot < 0 ? NULL : indigo_copy_property(NULL, observed[index][slot]);
	pthread_mutex_unlock(&property_mutex);
	return copy;
}

static unsigned revision(int index, const char *name) {
	pthread_mutex_lock(&property_mutex);
	int slot = property_slot(index, name);
	unsigned value = slot < 0 ? 0 : revisions[index][slot];
	pthread_mutex_unlock(&property_mutex);
	return value;
}

static bool wait_property(int index, const char *name, unsigned after, indigo_property_state state) {
	for (int i = 0; i < 600; i++) {
		pthread_mutex_lock(&property_mutex);
		int slot = property_slot(index, name);
		bool ready = slot >= 0 && revisions[index][slot] > after && observed[index][slot]->state == state;
		pthread_mutex_unlock(&property_mutex);
		if (ready) {
			return true;
		}
		indigo_usleep(10000);
	}
	indigo_property *last = snapshot(index, name);
	fprintf(stderr, "last revision=%u state=%d\n", revision(index, name), last ? last->state : -1);
	indigo_release_property(last);
	fprintf(stderr, "device %d property %s did not reach state %d after revision %u\n", index, name, state, after);
	return false;
}

static bool change_number(int index, const char *name, const char *item, double value, indigo_property_state state) {
	unsigned before = revision(index, name);
	indigo_change_number_property_1(NULL, logical[index]->name, name, item, value);
	return wait_property(index, name, before, state);
}

static bool change_switch(int index, const char *name, const char *item, indigo_property_state state) {
	unsigned before = revision(index, name);
	indigo_change_switch_property_1(NULL, logical[index]->name, name, item, true);
	return wait_property(index, name, before, state);
}

indigo_result touptek_test_attach(indigo_device *device) {
	if (inventory_mode) {
		int index = -1;
		const char *suffix = strrchr(device->name, '#');
		if (suffix) { index = atoi(suffix + 1) - 1000; }
		if (index < 0 || index >= TOUPCAM_MAX) { return INDIGO_FAILED; }
		int guider = strstr(device->name, "guider") != NULL;
		snprintf(inventory_names[index][guider], INDIGO_NAME_SIZE, "%s", device->name);
		indigo_result result = indigo_attach_device(device);
		if (result == INDIGO_OK) { inventory_devices[index][guider] = device; atomic_fetch_add(&inventory_count, 1); }
		return result;
	}
	int index = index_for(device->name);
	if (combined_model) {
		for (int i = 0; i < 4; i++) {
			if (device->attach == attach_functions[i]) {
				index = i;
				snprintf(combined_names[i], sizeof(combined_names[i]), "%s", device->name);
				break;
			}
		}
	} else if (index >= 0) {
		attach_functions[index] = device->attach;
	}
	if (index < 0) {
		return INDIGO_FAILED;
	}
	if (atomic_load(&fail_attach) && index == atomic_load(&fail_attach_index)) { return INDIGO_FAILED; }
	indigo_result result = indigo_attach_device(device);
	if (result == INDIGO_OK) {
		logical[index] = device;
		atomic_fetch_or(&attached, 1 << index);
	}
	return result;
}

indigo_result touptek_test_detach(indigo_device *device) {
	if (inventory_mode) {
		for (int i = 0; i < TOUPCAM_MAX; i++) {
			for (int g = 0; g < 2; g++) {
				if (inventory_devices[i][g] == device) { inventory_devices[i][g] = NULL; atomic_fetch_sub(&inventory_count, 1); }
			}
		}
		return indigo_detach_device(device);
	}
	int index = index_for(device->name);
	indigo_result result = indigo_detach_device(device);
	logical[index] = NULL;
	atomic_fetch_and(&attached, ~(1 << index));
	return result;
}

void touptek_test_usb_start(void) { }
static int physical_index(indigo_device *device) {
	if (inventory_mode) { return atoi(strrchr(device->name, '#') + 1) - 1000; }
	int index = index_for(device->name);
	return index < 2 ? 0 : index - 1;
}
indigo_queue *touptek_test_queue_create(indigo_device *device) {
	test_driver_queue = atomic_load(&fail_queue) ? NULL : indigo_queue_create(device);
	return test_driver_queue;
}
indigo_result touptek_test_lock(indigo_device *device) {
	if (atomic_exchange(&held[physical_index(device)], true)) { return INDIGO_BUSY; }
	atomic_fetch_add(&locks, 1);
	return INDIGO_OK;
}
indigo_result touptek_test_unlock(indigo_device *device) {
	// Global unlock is idempotent, including the base CCD detach release.
	if (atomic_exchange(&held[physical_index(device)], false)) { atomic_fetch_sub(&locks, 1); }
	return INDIGO_OK;
}
ssize_t LIBUSB_CALL touptek_test_usb_list(libusb_context *ctx, libusb_device ***list) {
	*list = calloc(1, sizeof(libusb_device *));
	return 0;
}
void LIBUSB_CALL touptek_test_usb_free(libusb_device **list, int unref) { free(list); }
int LIBUSB_CALL touptek_test_usb_register(libusb_context *ctx, int events, int flags, int vid, int pid, int cls, libusb_hotplug_callback_fn callback, void *data, libusb_hotplug_callback_handle *handle) {
	atomic_fetch_add(&registrations, 1);
	usb_callback = callback;
	callback(NULL, NULL, LIBUSB_HOTPLUG_EVENT_DEVICE_ARRIVED, NULL);
	if (atomic_load(&fail_register)) { usb_callback = NULL; return LIBUSB_ERROR_OTHER; }
	*handle = atomic_load(&registrations);
	return LIBUSB_SUCCESS;
}
int touptek_test_usb_register_sim(libusb_context *ctx, libusb_hotplug_event events, libusb_hotplug_flag flags, int vid, int pid, int cls, libusb_hotplug_callback_fn callback, void *data, libusb_hotplug_callback_handle *handle) {
	return touptek_test_usb_register(ctx, events, flags, vid, pid, cls, callback, data, handle);
}
void LIBUSB_CALL touptek_test_usb_deregister(libusb_context *ctx, libusb_hotplug_callback_handle handle) { usb_callback = NULL; atomic_fetch_add(&deregistrations, 1); }
int touptek_test_usb_deregister_poll(libusb_context *ctx, libusb_hotplug_callback_handle handle) { touptek_test_usb_deregister(ctx, handle); return 0; }

const char *Toupcam_Version(void) { return "lifecycle test"; }
unsigned Toupcam_EnumV2(ToupcamDeviceV2 arr[TOUPCAM_MAX]) {
	atomic_fetch_add(&enum_calls, 1);
	check_thread();
	unsigned count = 0;
	if (inventory_mode) {
		for (int k = 0; k < TOUPCAM_MAX; k++) {
			int i = inventory_reverse ? TOUPCAM_MAX - 1 - k : k;
			if (!atomic_load(&inventory_visible[i])) { continue; }
			memset(arr + count, 0, sizeof(*arr));
			snprintf(arr[count].id, sizeof(arr[count].id), "%d", 1000 + i);
			snprintf(arr[count].displayname, sizeof(arr[count].displayname), "Camera");
			arr[count++].model = models;
		}
		return count;
	}
	for (int i = 0; i < 3; i++) {
		if (atomic_load(&visible) & (1 << i)) {
			memset(&arr[count], 0, sizeof(arr[count]));
			snprintf(arr[count].id, sizeof(arr[count].id), "%d", i);
			snprintf(arr[count].displayname, sizeof(arr[count].displayname), "%s", models[i].name);
			arr[count++].model = &models[i];
		}
	}
	return count;
}
HToupcam Toupcam_Open(const char *id) {
	check_thread();
	if (atomic_load(&fail_open)) { return NULL; }
	atomic_fetch_add(&opened, 1);
	int index = atoi(id + (*id == '@')) - (inventory_mode ? 1000 : 0);
	if (atomic_exchange(&handle_open[index], true)) { atomic_fetch_add(&bad_handle, 1); }
	return (HToupcam)&handles[index];
}
void Toupcam_Close(HToupcam handle) {
	check_thread();
	if (!atomic_exchange(&handle_open[(int *)handle - handles], false)) { atomic_fetch_add(&bad_handle, 1); }
	atomic_fetch_add(&closed, 1);
}
HRESULT Toupcam_get_Option(HToupcam h, unsigned option, int *value) {
	if ((int)option == atomic_load(&failed_get_option)) { return -1; }
	if (h == NULL) {
		atomic_fetch_add(&bad_handle, 1);
		return -1;
	}
	*value = option == TOUPCAM_OPTION_FILTERWHEEL_POSITION ? (atomic_load(&moving) ? -1 : (atomic_load(&wheel_command) & 0xff)) : (option == TOUPCAM_OPTION_FILTERWHEEL_SLOT ? (atomic_load(&wheel_slots) ? atomic_load(&wheel_slots) : 7) : (option == TOUPCAM_OPTION_HEAT_MAX ? 5 : (option == TOUPCAM_OPTION_TEC_VOLTAGE_MAX ? 100 : (option < 256 ? atomic_load(&option_values[option]) : 0))));
	return 0;
}
HRESULT Toupcam_get_SerialNumber(HToupcam h, char serial[32]) { if (inventory_mode) { snprintf(serial, 32, "%06d", 1000 + (int)((int *)h - handles)); } else { strcpy(serial, "123456789"); } return 0; }
HRESULT Toupcam_AAF(HToupcam h, int action, int out, int *in) {
	if (h == NULL) {
		atomic_fetch_add(&bad_handle, 1);
		return -1;
	}
	if (action == TOUPCAM_AAF_GETAMBIENTTEMP) { atomic_fetch_add(&ambient_reads, 1); }
	if (action == TOUPCAM_AAF_SETPOSITION || action == TOUPCAM_AAF_SETZERO || action == TOUPCAM_AAF_HALT || action == TOUPCAM_AAF_SETMAXSTEP || action == TOUPCAM_AAF_SETBACKLASH || action == TOUPCAM_AAF_SETDIRECTION || action == TOUPCAM_AAF_SETBUZZER) {
		control_call(h);
		if (atomic_load(&fail_control)) { return -1; }
	}
	if (action == atomic_load(&fail_aaf)) { return -1; }
	if (action >= 0 && action < 64) { atomic_store(&aaf_values[action], out); }
	if ((action == TOUPCAM_AAF_SETPOSITION && !atomic_load(&hold_focus)) || action == TOUPCAM_AAF_SETZERO) { atomic_store(&focus_position, out); }
	if (action == TOUPCAM_AAF_ISMOVING && atomic_load(&hold_focus)) {
		*in = 1;
		return 0;
	}
	if (in) { *in = action == TOUPCAM_AAF_GETMAXSTEP || action == TOUPCAM_AAF_RANGEMAX ? 65000 : (action == TOUPCAM_AAF_GETPOSITION ? atomic_load(&focus_position) : (action == TOUPCAM_AAF_GETAMBIENTTEMP ? atomic_load(&ambient_temperature) : (action == TOUPCAM_AAF_GETTEMP ? 200 : 0))); }
	return 0;
}

static indigo_result update(indigo_client *client, indigo_device *device, indigo_property *property, const char *message) {
	observe_property(device, property, false);
	for (unsigned i = 0; i < ARRAY_SIZE(property_names); i++) {
		if (!strcmp(property->name, property_names[i])) {
			if (property->type == INDIGO_NUMBER_VECTOR) {
				for (int j = 0; j < property->count && j < 6; j++) {
					atomic_store(&property_values[i][j], property->items[j].number.value);
				}
			}
			if (property->state == INDIGO_ALERT_STATE) { atomic_fetch_add(&property_alerts[i], 1); }
			atomic_store(&property_state[i], property->state);
		}
	}
	if (!strcmp(property->name, "CONNECTION")) {
		int state = property->state == INDIGO_BUSY_STATE ? 2 : (property->items[0].sw.value && property->state == INDIGO_OK_STATE ? 1 : 0);
		int index = index_for(device->name);
		if (index >= 0) { atomic_store(&connection[index], state); }
	}
	return INDIGO_OK;
}
static indigo_client client = { .name = "ToupTek lifecycle test", .version = INDIGO_VERSION_CURRENT, .define_property = define_property, .update_property = update, .delete_property = delete_property };

static bool wait_value(atomic_int *value, int expected) {
	for (int i = 0; i < 600; i++) {
		if (atomic_load(value) == expected) { return true; }
		indigo_usleep(10000);
	}
	return false;
}
static void connect_device(int index, bool connected) {
	indigo_change_switch_property_1(NULL, logical[index]->name, "CONNECTION", connected ? "CONNECTED" : "DISCONNECTED", true);
}

static void lifecycle(void) {
	indigo_start();
	indigo_attach_client(&client);
	for (int cycle = 0; cycle < 2; cycle++) {
		have_thread = false;
		atomic_store(&visible, 7);
		CHECK_EQ_INT(indigo_ccd_touptek(INDIGO_DRIVER_INIT, NULL), INDIGO_OK);
		CHECK_TRUE(wait_value(&attached, 15));
		// First cycle guider opens first, second cycle camera opens first.
		int first = cycle ? 0 : 1;
		connect_device(first, true);
		CHECK_TRUE(wait_value(&connection[first], 1));
		connect_device(1 - first, true);
		CHECK_TRUE(wait_value(&connection[1 - first], 1));
		atomic_store(&moving, true);
		connect_device(2, true);
		CHECK_TRUE(wait_value(&connection[2], 2));
		connect_device(3, true);
		CHECK_TRUE(wait_value(&connection[3], 1)); // wheel polling must not block this
		int registered = atomic_load(&registrations), deregistered = atomic_load(&deregistrations);
		CHECK_EQ_INT(indigo_ccd_touptek(INDIGO_DRIVER_SHUTDOWN, NULL), INDIGO_BUSY);
		CHECK_EQ_INT(atomic_load(&registrations), registered);
		CHECK_EQ_INT(atomic_load(&deregistrations), deregistered);
		atomic_store(&moving, false);
		CHECK_TRUE(wait_value(&connection[2], 1)); // rejected shutdown must retain its poll
		indigo_change_switch_property_1(NULL, logical[2]->name, "X_CALIBRATE", "START", true);
		CHECK_TRUE(wait_value(&calibrations, cycle + 1));
		connect_device(first, false);
		CHECK_TRUE(wait_value(&connection[first], 0));
		CHECK_EQ_INT(atomic_load(&connection[1 - first]), 1);
		connect_device(1 - first, false);
		CHECK_TRUE(wait_value(&connection[1 - first], 0));
		connect_device(2, false);
		CHECK_TRUE(wait_value(&connection[2], 0));
		connect_device(3, false);
		CHECK_TRUE(wait_value(&connection[3], 0));
		CHECK_EQ_INT(atomic_load(&opened), atomic_load(&closed));
		CHECK_EQ_INT(atomic_load(&locks), 0);
		atomic_store(&moving, true);
		connect_device(2, true);
		CHECK_TRUE(wait_value(&connection[2], 2));
		// Removal must close an initializing wheel and cancel its pending finalizer.
		atomic_store(&visible, 0);
		usb_callback(NULL, NULL, LIBUSB_HOTPLUG_EVENT_DEVICE_LEFT, NULL);
		CHECK_TRUE(wait_value(&attached, 0));
		CHECK_EQ_INT(indigo_ccd_touptek(INDIGO_DRIVER_SHUTDOWN, NULL), INDIGO_OK);
		CHECK_EQ_INT(atomic_load(&opened), atomic_load(&closed));
		CHECK_EQ_INT(atomic_load(&wrong_thread), 0);
		atomic_store(&moving, false);
	}
	// Camera-only ownership and failed Open must not leak a handle/global lock.
	have_thread = false;
	models[0].flag &= ~TOUPCAM_FLAG_ST4;
	atomic_store(&visible, 1);
	CHECK_EQ_INT(indigo_ccd_touptek(INDIGO_DRIVER_INIT, NULL), INDIGO_OK);
	CHECK_TRUE(wait_value(&attached, 1));
	atomic_store(&fail_open, true);
	connect_device(0, true);
	CHECK_TRUE(wait_value(&connection[0], 0));
	CHECK_EQ_INT(atomic_load(&locks), 0);
	atomic_store(&fail_open, false);
	connect_device(0, true);
	CHECK_TRUE(wait_value(&connection[0], 1));
	connect_device(0, false);
	CHECK_TRUE(wait_value(&connection[0], 0));
	CHECK_EQ_INT(atomic_load(&opened), atomic_load(&closed));
	CHECK_EQ_INT(atomic_load(&locks), 0);
	CHECK_EQ_INT(indigo_ccd_touptek(INDIGO_DRIVER_SHUTDOWN, NULL), INDIGO_OK);
	CHECK_EQ_INT(atomic_load(&bad_handle), 0);
	CHECK_EQ_INT(atomic_load(&wrong_thread), 0);
	atomic_store(&visible, 0);
	atomic_store(&fail_queue, true);
	CHECK_EQ_INT(indigo_ccd_touptek(INDIGO_DRIVER_INIT, NULL), INDIGO_FAILED);
	atomic_store(&fail_queue, false);
	// Failed registration must leave a repeatable INIT/SHUTDOWN lifecycle.
	atomic_store(&fail_register, true);
	CHECK_EQ_INT(indigo_ccd_touptek(INDIGO_DRIVER_INIT, NULL), INDIGO_FAILED);
	atomic_store(&fail_register, false);
	CHECK_EQ_INT(indigo_ccd_touptek(INDIGO_DRIVER_INIT, NULL), INDIGO_OK);
	CHECK_EQ_INT(indigo_ccd_touptek(INDIGO_DRIVER_SHUTDOWN, NULL), INDIGO_OK);
cleanup:
	atomic_store(&visible, 0);
	if (usb_callback) { usb_callback(NULL, NULL, LIBUSB_HOTPLUG_EVENT_DEVICE_LEFT, NULL); }
	wait_value(&attached, 0);
	indigo_ccd_touptek(INDIGO_DRIVER_SHUTDOWN, NULL);
	indigo_detach_client(&client);
	indigo_stop();
}

static bool start_properties(void) {
	atomic_store(&roi_left, 0);
	atomic_store(&roi_top, 0);
	atomic_store(&roi_width, 640);
	atomic_store(&roi_height, 480);
	clear_observer();
	atomic_store(&guide_started_ns[0], 0);
	atomic_store(&guide_started_ns[1], 0);
	atomic_store(&invalid_blobs, 0);
	atomic_store(&sdk_event, TOUPCAM_EVENT_IMAGE);
	for (int i = 0; i < 256; i++) { atomic_store(&option_values[i], 0); atomic_store(&option_calls[i], 0); }
	atomic_store(&option_values[TOUPCAM_OPTION_BINNING], 1);
	atomic_store(&fail_option, -1);
	atomic_store(&fail_aaf, -1);
	atomic_store(&fail_pull, false);
	atomic_store(&fail_trigger, false);
	atomic_store(&wheel_slots, 7);
	atomic_store(&focus_position, 0);
	atomic_store(&ambient_temperature, 0);
	atomic_store(&ambient_reads, 0);
	indigo_start();
	indigo_attach_client(&client);
	have_thread = false;
	memset(have_control_thread, 0, sizeof(have_control_thread));
	bus_thread = pthread_self();
	models[0].flag |= TOUPCAM_FLAG_ST4;
	atomic_store(&visible, 7);
	atomic_store(&fail_control, false);
	for (int i = 0; i < 4; i++) { atomic_store(&connection[i], 0); }
	if (indigo_ccd_touptek(INDIGO_DRIVER_INIT, NULL) != INDIGO_OK || !wait_value(&attached, 15)) { return false; }
	for (int i = 0; i < 4; i++) {
		connect_device(i, true);
		if (!wait_value(&connection[i], 1)) { return false; }
	}
	for (unsigned i = 0; i < ARRAY_SIZE(property_names); i++) {
		atomic_store(&property_state[i], -1);
		atomic_store(&property_alerts[i], 0);
	}
	indigo_enumerate_properties(&client, &INDIGO_ALL_PROPERTIES);
	atomic_store(&track_controls, true);
	return true;
}

static void stop_properties(void) {
	atomic_store(&read_failure, 0);
	atomic_store(&failed_get_option, -1);
	atomic_store(&independent_temperature, false);
	atomic_store(&stream_frames, 0);
	atomic_store(&fast_watchdog, false);
	atomic_store(&deliver_image, false);
	atomic_store(&image_on_stop, false);
	atomic_store(&track_controls, false);
	atomic_store(&fail_control, false);
	atomic_store(&moving, false);
	atomic_store(&hold_focus, false);
	atomic_store(&visible, 0);
	if (usb_callback) { usb_callback(NULL, NULL, LIBUSB_HOTPLUG_EVENT_DEVICE_LEFT, NULL); }
	wait_value(&attached, 0);
	indigo_ccd_touptek(INDIGO_DRIVER_SHUTDOWN, NULL);
	indigo_detach_client(&client);
	indigo_stop();
}

static void camera_properties(void) {
	CHECK_TRUE(start_properties());
	indigo_change_number_property_1(NULL, logical[0]->name, "CCD_GAIN", "GAIN", 12);
	CHECK_TRUE(wait_value(&property_state[0], INDIGO_OK_STATE));
	CHECK_EQ_INT(atomic_load(&last_gain), 12);
	atomic_store(&fail_control, true);
	atomic_store(&property_state[0], -1);
	indigo_change_number_property_1(NULL, logical[0]->name, "CCD_GAIN", "GAIN", 13);
	CHECK_TRUE(wait_value(&property_state[0], INDIGO_OK_STATE));
	// The original gain branch falls through to base code, which publishes OK after ALERT.
	CHECK_EQ_INT(atomic_load(&property_alerts[0]), 1);
	indigo_change_number_property_1(NULL, logical[0]->name, "CCD_OFFSET", "OFFSET", 16);
	CHECK_TRUE(wait_value(&property_state[1], INDIGO_ALERT_STATE)); // explicit return: no base OK
	atomic_store(&fail_control, false);
	indigo_change_number_property_1(NULL, logical[0]->name, "CCD_BIN", "HORIZONTAL", 2);
	CHECK_TRUE(wait_value(&property_state[2], INDIGO_OK_STATE));
	CHECK_EQ_INT(atomic_load(&property_values[2][0]), 2);
	CHECK_EQ_INT(atomic_load(&property_values[2][1]), 2);
	atomic_store(&property_state[2], -1);
	indigo_change_number_property_1(NULL, logical[0]->name, "CCD_BIN", "VERTICAL", 3);
	CHECK_TRUE(wait_value(&property_state[2], INDIGO_OK_STATE));
	CHECK_EQ_INT(atomic_load(&property_values[2][0]), 3);
	CHECK_EQ_INT(atomic_load(&property_values[2][1]), 3);
	atomic_store(&property_state[3], -1);
	indigo_change_number_property_1(NULL, logical[0]->name, "CCD_FRAME", "WIDTH", 100);
	CHECK_TRUE(wait_value(&property_state[3], INDIGO_OK_STATE));
	CHECK_EQ_INT(atomic_load(&property_values[3][2]), 99); // base frame normalization
	indigo_change_number_property_1(NULL, logical[0]->name, "CCD_EXPOSURE", "EXPOSURE", 10);
	CHECK_TRUE(wait_value(&property_state[5], INDIGO_BUSY_STATE));
	indigo_change_switch_property_1(NULL, logical[0]->name, "CCD_ABORT_EXPOSURE", "ABORT_EXPOSURE", true);
	CHECK_TRUE(wait_value(&property_state[6], INDIGO_OK_STATE));
	// The standard BUSY guard keeps the first queued value intact.
	indigo_lock_master_device(logical[0]);
	indigo_change_number_property_1(NULL, logical[0]->name, "CCD_GAIN", "GAIN", 15);
	indigo_change_number_property_1(NULL, logical[0]->name, "CCD_GAIN", "GAIN", 16);
	indigo_unlock_master_device(logical[0]);
	CHECK_TRUE(wait_value(&property_state[0], INDIGO_OK_STATE));
	CHECK_EQ_INT(atomic_load(&last_gain), 15);
	int before = atomic_load(&gain_calls);
	indigo_lock_master_device(logical[0]);
	indigo_change_number_property_1(NULL, logical[0]->name, "CCD_GAIN", "GAIN", 17);
	connect_device(0, false);
	indigo_unlock_master_device(logical[0]);
	CHECK_TRUE(wait_value(&connection[0], 0));
	// A task already dequeued before disconnect may finish; later requests cannot run on the closed handle.
	CHECK_TRUE(atomic_load(&gain_calls) <= before + 1);
	atomic_store(&track_controls, false);
	connect_device(0, true);
	CHECK_TRUE(wait_value(&connection[0], 1));
	atomic_store(&track_controls, true);
	indigo_change_number_property_1(NULL, logical[0]->name, "CCD_GAIN", "GAIN", 18);
	CHECK_TRUE(wait_value(&property_state[0], INDIGO_OK_STATE));
	CHECK_EQ_INT(atomic_load(&last_gain), 18);
	// Real SDK-thread notification, with frame retrieval and processing on the device queue.
	atomic_store(&deliver_image, true);
	atomic_store(&image_on_stop, true); // also exercises stale notifications from setup Stop
	atomic_store(&property_state[5], -1);
	int images = atomic_load(&pulled_images);
	indigo_change_number_property_1(NULL, logical[0]->name, "CCD_EXPOSURE", "EXPOSURE", 1);
	CHECK_TRUE(wait_value(&property_state[5], INDIGO_OK_STATE));
	CHECK_EQ_INT(atomic_load(&pulled_images), images + 1);
	connect_device(0, false); // Stop emits one more notification before returning
	CHECK_TRUE(wait_value(&connection[0], 0));
	CHECK_EQ_INT(atomic_load(&pulled_images), images + 1);
	CHECK_EQ_INT(atomic_load(&wrong_thread), 0);
	CHECK_EQ_INT(atomic_load(&bad_handle), 0);
cleanup:
	stop_properties();
}

static void acquisition_finalizers(void) {
	CHECK_TRUE(start_properties());
	atomic_store(&fast_watchdog, true);
	indigo_change_number_property_1(NULL, logical[0]->name, "CCD_EXPOSURE", "EXPOSURE", 1);
	CHECK_TRUE(wait_value(&property_state[5], INDIGO_ALERT_STATE));
	CHECK_EQ_INT(atomic_load(&watchdog_seconds), 26);
	atomic_store(&fast_watchdog, false);
	atomic_store(&stream_frames, 2);
	int images = atomic_load(&pulled_images);
	indigo_change_number_property(NULL, logical[0]->name, "CCD_STREAMING", 2, (const char *[]){ "EXPOSURE", "COUNT" }, (double []){ 1, 2 });
	CHECK_TRUE(wait_value(&property_state[17], INDIGO_OK_STATE));
	CHECK_EQ_INT(atomic_load(&pulled_images), images + 2);
	images += 2;
	atomic_store(&property_state[17], -1);
	indigo_change_number_property(NULL, logical[0]->name, "CCD_STREAMING", 2, (const char *[]){ "EXPOSURE", "COUNT" }, (double []){ 1, -1 });
	CHECK_TRUE(wait_value(&pulled_images, images + 2));
	CHECK_EQ_INT(atomic_load(&property_state[17]), INDIGO_BUSY_STATE);
	atomic_store(&image_on_stop, true);
	indigo_change_switch_property_1(NULL, logical[0]->name, "CCD_ABORT_EXPOSURE", "ABORT_EXPOSURE", true);
	CHECK_TRUE(wait_value(&property_state[6], INDIGO_OK_STATE));
	CHECK_TRUE(wait_value(&property_state[17], INDIGO_OK_STATE));
	CHECK_EQ_INT(atomic_load(&pulled_images), images + 2); // stop notification is obsolete
	CHECK_EQ_INT(atomic_load(&wrong_thread), 0);
	CHECK_EQ_INT(atomic_load(&bad_handle), 0);
cleanup:
	stop_properties();
}

static void guider_properties(void) {
	CHECK_TRUE(start_properties());
	// A camera setting establishes the master worker before the guider pulse.
	indigo_change_number_property_1(NULL, logical[0]->name, "CCD_GAIN", "GAIN", 14);
	CHECK_TRUE(wait_value(&property_state[0], INDIGO_OK_STATE));
	indigo_change_number_property_1(NULL, logical[1]->name, "GUIDER_GUIDE_RA", "EAST", 50);
	CHECK_TRUE(wait_value(&property_state[7], INDIGO_OK_STATE));
	CHECK_EQ_INT(atomic_load(&pulse_direction), 2);
	CHECK_EQ_INT(atomic_load(&pulse_duration), 50);
	atomic_store(&fail_control, true);
	indigo_change_number_property_1(NULL, logical[1]->name, "GUIDER_GUIDE_DEC", "SOUTH", 75);
	CHECK_TRUE(wait_value(&property_state[8], INDIGO_ALERT_STATE));
	CHECK_EQ_INT(atomic_load(&pulse_direction), 1);
	CHECK_EQ_INT(atomic_load(&wrong_thread), 0);
cleanup:
	stop_properties();
}

static void wheel_properties(void) {
	CHECK_TRUE(start_properties());
	indigo_change_number_property_1(NULL, logical[2]->name, "WHEEL_SLOT", "SLOT", 2);
	CHECK_TRUE(wait_value(&property_state[9], INDIGO_OK_STATE));
	CHECK_EQ_INT(atomic_load(&wheel_command), 1 + (1 << 8));
	CHECK_EQ_INT(atomic_load(&property_values[9][0]), 2);
	atomic_store(&property_state[9], -1);
	atomic_store(&fail_control, true);
	indigo_change_number_property_1(NULL, logical[2]->name, "WHEEL_SLOT", "SLOT", 3);
	CHECK_TRUE(wait_value(&property_state[9], INDIGO_ALERT_STATE));
	CHECK_EQ_INT(atomic_load(&wrong_thread), 0);
cleanup:
	stop_properties();
}

static void focuser_properties(void) {
	CHECK_TRUE(start_properties());
	indigo_change_number_property_1(NULL, logical[3]->name, "FOCUSER_POSITION", "POSITION", 100);
	CHECK_TRUE(wait_value(&property_state[11], INDIGO_OK_STATE));
	CHECK_EQ_INT(atomic_load(&focus_position), 100);
	atomic_store(&fail_control, true);
	indigo_change_number_property_1(NULL, logical[3]->name, "FOCUSER_LIMITS", "MAX_POSITION", 500);
	CHECK_TRUE(wait_value(&property_state[13], INDIGO_ALERT_STATE));
	CHECK_EQ_INT(atomic_load(&property_values[13][1]), 65000);
	atomic_store(&fail_control, false);
	indigo_change_switch_property_1(NULL, logical[3]->name, "FOCUSER_ABORT_MOTION", "ABORT_MOTION", true);
	CHECK_TRUE(wait_value(&property_state[14], INDIGO_OK_STATE));
	CHECK_EQ_INT(atomic_load(&wrong_thread), 0);
	CHECK_EQ_INT(atomic_load(&bad_handle), 0);
cleanup:
	stop_properties();
}

static unsigned long long mono_flags;

static void enable_full_camera(void) {
	mono_flags = models[0].flag;
	models[0].flag &= ~TOUPCAM_FLAG_MONO;
	models[0].flag |= TOUPCAM_FLAG_RAW10 | TOUPCAM_FLAG_RAW12 | TOUPCAM_FLAG_RAW14 | TOUPCAM_FLAG_RAW16 | TOUPCAM_FLAG_GETTEMPERATURE | TOUPCAM_FLAG_TEC_ONOFF | TOUPCAM_FLAG_FAN | TOUPCAM_FLAG_HEAT | TOUPCAM_FLAG_CG | TOUPCAM_FLAG_CGHDR;
	models[0].maxspeed = 5;
}

static void restore_camera(void) {
	stop_properties();
	models[0].flag = mono_flags;
}

static bool workflow_property(const char *name) {
	const char *names[] = { "CONNECTION", "CONFIG", "CCD_EXPOSURE", "CCD_STREAMING", "CCD_ABORT_EXPOSURE", "CCD_TEMPERATURE", "GUIDER_GUIDE_RA", "GUIDER_GUIDE_DEC", "WHEEL_SLOT", "X_CALIBRATE", "FOCUSER_POSITION", "FOCUSER_STEPS", "FOCUSER_ABORT_MOTION" };
	for (unsigned i = 0; i < ARRAY_SIZE(names); i++) {
		if (!strcmp(name, names[i])) {
			return true;
		}
	}
	return false;
}

static void property_inventory(void) {
	indigo_property *copy = NULL;
	enable_full_camera();
	CHECK_TRUE(start_properties());
	const char *required[] = { "CCD_MODE", "CCD_BIN", "CCD_FRAME", "CCD_EXPOSURE", "CCD_STREAMING", "CCD_ABORT_EXPOSURE", "CCD_GAIN", "CCD_OFFSET", "CCD_COOLER", "CCD_COOLER_POWER", "CCD_TEMPERATURE", "X_CCD_ADVANCED", "X_CCD_FAN", "X_CCD_HEATER", "X_CCD_CONVERSION_GAIN", "X_CCD_BIN_MODE", "X_CCD_LED", "CCD_IMAGE", "CCD_IMAGE_FORMAT", "CCD_UPLOAD_MODE", "CCD_FRAME_TYPE", "CONFIG" };
	for (unsigned i = 0; i < ARRAY_SIZE(required); i++) {
		copy = snapshot(0, required[i]);
		if (!copy) { fprintf(stderr, "Missing required property %s\n", required[i]); }
		CHECK_TRUE(copy != NULL);
		indigo_release_property(copy);
		copy = NULL;
	}
	// Every published property is validated; passive writable vectors also traverse the real base/driver dispatcher.
	for (int d = 0; d < 4; d++) {
		int checked = 0;
		for (int i = 0; i < PROPERTY_CAPACITY; i++) {
			pthread_mutex_lock(&property_mutex);
			copy = observed[d][i] && defined[d][i] ? indigo_copy_property(NULL, observed[d][i]) : NULL;
			pthread_mutex_unlock(&property_mutex);
			if (copy == NULL || copy->hidden) {
				indigo_release_property(copy);
				copy = NULL;
				continue;
			}
			checked++;
			CHECK_TRUE(copy->count >= 0 && *copy->name && !strcmp(copy->device, logical[d]->name));
			for (int j = 0; j < copy->count; j++) {
				CHECK_TRUE(*copy->items[j].name);
				for (int k = 0; k < j; k++) {
					CHECK_TRUE(strcmp(copy->items[j].name, copy->items[k].name));
				}
				if (copy->type == INDIGO_NUMBER_VECTOR) {
					CHECK_TRUE(isfinite(copy->items[j].number.value));
					CHECK_TRUE(copy->items[j].number.min <= copy->items[j].number.max);
				}
			}
			if (copy->count > 0 && copy->perm != INDIGO_RO_PERM && !workflow_property(copy->name) && copy->type != INDIGO_BLOB_VECTOR) {
				if (!strcmp(copy->name, "CCD_LOCAL_MODE")) {
					indigo_set_text_item_value(copy->items, test_output_folder);
				}
				printf("    round-trip device=%d property=%s\n", d, copy->name);
				unsigned before = revision(d, copy->name);
				indigo_change_property(&client, copy);
				CHECK_TRUE(wait_property(d, copy->name, before, !strcmp(copy->name, "CCD_LENS") && copy->items[0].number.value == 0 && copy->items[1].number.value == 0 ? INDIGO_IDLE_STATE : INDIGO_OK_STATE));
			}
			indigo_release_property(copy);
			copy = NULL;
		}
		CHECK_TRUE(checked >= 4);
	}
cleanup:
	indigo_release_property(copy);
	restore_camera();
}

static void optional_camera_properties(void) {
	enable_full_camera();
	CHECK_TRUE(start_properties());
	const struct { const char *property, *item; double value; int option; } numbers[] = {
		{ "CCD_OFFSET", "OFFSET", 16, TOUPCAM_OPTION_BLACKLEVEL },
		{ "X_CCD_FAN", "FAN_SPEED", 3, TOUPCAM_OPTION_FAN },
		{ "X_CCD_HEATER", "POWER", 4, TOUPCAM_OPTION_HEAT }
	};
	for (unsigned i = 0; i < ARRAY_SIZE(numbers); i++) {
		CHECK_TRUE(change_number(0, numbers[i].property, numbers[i].item, numbers[i].value, INDIGO_OK_STATE));
		CHECK_TRUE(atomic_load(&option_calls[numbers[i].option]) > 0);
		atomic_store(&fail_option, numbers[i].option);
		CHECK_TRUE(change_number(0, numbers[i].property, numbers[i].item, numbers[i].value + (i == 0 ? 8 : 1), INDIGO_ALERT_STATE));
		atomic_store(&fail_option, -1);
	}
	const struct { const char *property, *item; int option, value; } switches[] = {
		{ "CCD_COOLER", "ON", TOUPCAM_OPTION_TEC, 1 },
		{ "CCD_COOLER", "OFF", TOUPCAM_OPTION_TEC, 0 },
		{ "X_CCD_LED", "ON", TOUPCAM_OPTION_TAILLIGHT, 1 },
		{ "X_CCD_LED", "OFF", TOUPCAM_OPTION_TAILLIGHT, 0 },
		{ "X_CCD_CONVERSION_GAIN", "LCG", TOUPCAM_OPTION_CG, 0 },
		{ "X_CCD_CONVERSION_GAIN", "HCG", TOUPCAM_OPTION_CG, 1 },
		{ "X_CCD_CONVERSION_GAIN", "HDR", TOUPCAM_OPTION_CG, 2 }
	};
	for (unsigned i = 0; i < ARRAY_SIZE(switches); i++) {
		CHECK_TRUE(change_switch(0, switches[i].property, switches[i].item, INDIGO_OK_STATE));
		CHECK_EQ_INT(atomic_load(&option_values[switches[i].option]), switches[i].value);
		atomic_store(&fail_option, switches[i].option);
		CHECK_TRUE(change_switch(0, switches[i].property, switches[i].item, INDIGO_ALERT_STATE));
		atomic_store(&fail_option, -1);
	}
	const char *advanced[] = { "SPEED", "CONTRAST", "HUE", "SATURATION", "BRIGHTNESS", "GAMMA", "R_GAIN", "G_GAIN", "B_GAIN" };
	for (unsigned i = 0; i < ARRAY_SIZE(advanced); i++) {
		int value = i == 0 ? 2 : (i == 5 ? 120 : 10);
		CHECK_TRUE(change_number(0, "X_CCD_ADVANCED", advanced[i], value, INDIGO_OK_STATE));
		CHECK_EQ_INT(atomic_load(&advanced_values[i]), value);
	}
	atomic_store(&fail_control, true);
	CHECK_TRUE(change_number(0, "X_CCD_ADVANCED", "SPEED", 3, INDIGO_ALERT_STATE));
	CHECK_TRUE(change_number(0, "CCD_TEMPERATURE", "TEMPERATURE", -10, INDIGO_ALERT_STATE));
	atomic_store(&fail_control, false);
	CHECK_TRUE(change_number(0, "CCD_TEMPERATURE", "TEMPERATURE", -12, INDIGO_BUSY_STATE));
	CHECK_TRUE(wait_value(&target_temperature, -120));
	CHECK_TRUE(change_number(0, "CCD_TEMPERATURE", "TEMPERATURE", -8, INDIGO_BUSY_STATE));
	CHECK_TRUE(wait_value(&target_temperature, -80));
	CHECK_TRUE(wait_property(0, "CCD_TEMPERATURE", revision(0, "CCD_TEMPERATURE"), INDIGO_OK_STATE));
	CHECK_EQ_INT(atomic_load(&wrong_thread), 0);
cleanup:
	restore_camera();
}

static void image_formats(void) {
	enable_full_camera();
	CHECK_TRUE(start_properties());
	CHECK_TRUE(change_switch(0, "CCD_UPLOAD_MODE", "CLIENT", INDIGO_OK_STATE));
	CHECK_TRUE(change_switch(0, "CCD_IMAGE_FORMAT", "RAW", INDIGO_OK_STATE));
	const char *modes[] = { "RAW08_1", "RAW10_1", "RAW12_1", "RAW14_1", "RAW16_1", "RGB08_1" };
	atomic_store(&deliver_image, true);
	for (unsigned i = 0; i < ARRAY_SIZE(modes); i++) {
		CHECK_TRUE(change_switch(0, "CCD_MODE", modes[i], INDIGO_OK_STATE));
		int before = atomic_load(&raw_blobs);
		CHECK_TRUE(change_number(0, "CCD_EXPOSURE", "EXPOSURE", 0.1, INDIGO_OK_STATE));
		CHECK_TRUE(wait_value(&raw_blobs, before + 1));
		CHECK_EQ_INT(atomic_load(&invalid_blobs), 0);
	}
	CHECK_TRUE(change_switch(0, "CCD_MODE", "RAW08_2", INDIGO_OK_STATE));
	CHECK_TRUE(change_number(0, "CCD_FRAME", "WIDTH", 128, INDIGO_OK_STATE));
	CHECK_TRUE(change_number(0, "CCD_FRAME", "HEIGHT", 96, INDIGO_OK_STATE));
	CHECK_TRUE(change_number(0, "CCD_FRAME", "LEFT", 8, INDIGO_OK_STATE));
	CHECK_TRUE(change_number(0, "CCD_FRAME", "TOP", 10, INDIGO_OK_STATE));
	const char *bins[] = { "SATURATE", "EXPAND", "AVERAGE" };
	const int bin_options[] = { 2, 0x40 | 2, 0x80 | 2 };
	for (int i = 0; i < 3; i++) {
		CHECK_TRUE(change_switch(0, "X_CCD_BIN_MODE", bins[i], INDIGO_OK_STATE));
		CHECK_TRUE(change_number(0, "CCD_EXPOSURE", "EXPOSURE", 0.2, INDIGO_OK_STATE));
		CHECK_EQ_INT(atomic_load(&option_values[TOUPCAM_OPTION_BINNING]), bin_options[i]);
		CHECK_EQ_INT(atomic_load(&roi_left), 8);
		CHECK_EQ_INT(atomic_load(&roi_top), 10);
		CHECK_EQ_INT(atomic_load(&roi_width), 128);
		CHECK_EQ_INT(atomic_load(&roi_height), 96);
	}
	const char *frame_types[] = { "LIGHT", "BIAS", "DARK", "FLAT", "DARKFLAT" };
	for (unsigned i = 0; i < ARRAY_SIZE(frame_types); i++) {
		CHECK_TRUE(change_switch(0, "CCD_FRAME_TYPE", frame_types[i], INDIGO_OK_STATE));
		CHECK_TRUE(change_number(0, "CCD_EXPOSURE", "EXPOSURE", 0.3, INDIGO_OK_STATE));
		CHECK_EQ_INT(atomic_load(&last_exposure), i == 1 ? 0 : 300000);
	}
cleanup:
	restore_camera();
}

static void acquisition_edges(void) {
	CHECK_TRUE(start_properties());
	atomic_store(&fast_watchdog, true);
	atomic_store(&fail_trigger, true);
	CHECK_TRUE(change_number(0, "CCD_EXPOSURE", "EXPOSURE", 60, INDIGO_ALERT_STATE));
	CHECK_EQ_INT(atomic_load(&watchdog_seconds), 90);
	atomic_store(&fail_trigger, false);
	atomic_store(&fast_watchdog, false);
	atomic_store(&deliver_image, true);
	atomic_store(&fail_pull, true);
	CHECK_TRUE(change_number(0, "CCD_EXPOSURE", "EXPOSURE", 0.1, INDIGO_ALERT_STATE));
	atomic_store(&fail_pull, false);
	CHECK_TRUE(change_number(0, "CCD_EXPOSURE", "EXPOSURE", 0.1, INDIGO_OK_STATE));
	atomic_store(&deliver_image, false);
	CHECK_TRUE(change_number(0, "CCD_EXPOSURE", "EXPOSURE", 10, INDIGO_BUSY_STATE));
	unsigned mode_revision = revision(0, "CCD_MODE");
	unsigned streaming_revision = revision(0, "CCD_STREAMING");
	indigo_change_switch_property_1(NULL, logical[0]->name, "CCD_MODE", "MON08_2", true);
	indigo_change_number_property(NULL, logical[0]->name, "CCD_STREAMING", 2, (const char *[]){ "EXPOSURE", "COUNT" }, (double []){ 0.1, 2 });
	CHECK_EQ_INT(revision(0, "CCD_MODE"), mode_revision);
	CHECK_EQ_INT(revision(0, "CCD_STREAMING"), streaming_revision);
	CHECK_TRUE(change_switch(0, "CCD_ABORT_EXPOSURE", "ABORT_EXPOSURE", INDIGO_OK_STATE));
	atomic_store(&image_on_stop, true);
	atomic_store(&track_controls, false);
	connect_device(0, false);
	CHECK_TRUE(wait_value(&connection[0], 0));
	int images = atomic_load(&pulled_images);
	connect_device(0, true);
	CHECK_TRUE(wait_value(&connection[0], 1));
	CHECK_EQ_INT(atomic_load(&pulled_images), images);
	atomic_store(&track_controls, true);
	atomic_store(&deliver_image, true);
	CHECK_TRUE(change_number(0, "CCD_EXPOSURE", "EXPOSURE", 0.1, INDIGO_OK_STATE));
	CHECK_EQ_INT(atomic_load(&bad_handle), 0);
cleanup:
	stop_properties();
}

static void guide_workflows(void) {
	CHECK_TRUE(start_properties());
	const char *properties[] = { "GUIDER_GUIDE_RA", "GUIDER_GUIDE_RA", "GUIDER_GUIDE_DEC", "GUIDER_GUIDE_DEC" };
	const char *directions[] = { "EAST", "WEST", "NORTH", "SOUTH" };
	const int sdk_directions[] = { 2, 3, 0, 1 };
	for (int i = 0; i < 4; i++) {
		CHECK_TRUE(change_number(1, properties[i], directions[i], 20, INDIGO_OK_STATE));
		CHECK_EQ_INT(atomic_load(&pulse_direction), sdk_directions[i]);
		CHECK_EQ_INT(atomic_load(&pulse_duration), 20);
	}
	for (int axis = 0; axis < 2; axis++) {
		const char *property = axis ? "GUIDER_GUIDE_DEC" : "GUIDER_GUIDE_RA";
		const char *items[] = { axis ? "NORTH" : "EAST", axis ? "SOUTH" : "WEST" };
		CHECK_TRUE(change_number(1, property, items[0], 1000, INDIGO_BUSY_STATE));
		CHECK_TRUE(wait_value(&pulse_duration, 1000));
		unsigned before = revision(1, property);
		indigo_change_number_property(NULL, logical[1]->name, property, 2, items, (double []){ 0, 800 });
		CHECK_TRUE(wait_value(&pulse_duration, 800));
		CHECK_TRUE(wait_property(1, property, before, INDIGO_BUSY_STATE));
		CHECK_EQ_INT(atomic_load(&pulse_direction), axis ? 1 : 3);
		before = revision(1, property);
		indigo_change_number_property(NULL, logical[1]->name, property, 2, items, (double []){ 0, 0 });
		CHECK_TRUE(wait_property(1, property, before, INDIGO_OK_STATE));
		CHECK_TRUE(change_number(1, property, items[0], 40, INDIGO_OK_STATE));
		CHECK_EQ_INT(atomic_load(&pulse_duration), 40);
	}
cleanup:
	stop_properties();
}

static void wheel_workflows(void) {
	CHECK_TRUE(start_properties());
	const char *models[] = { "5_POSITIONS", "7_POSITIONS", "8_POSITIONS" };
	const int slots[] = { 5, 7, 8 };
	for (int i = 0; i < 3; i++) {
		CHECK_TRUE(change_switch(2, "X_WHEEL_MODEL", models[i], INDIGO_OK_STATE));
		CHECK_EQ_INT(atomic_load(&wheel_slots), slots[i]);
		CHECK_TRUE(change_number(2, "WHEEL_SLOT", "SLOT", slots[i], INDIGO_OK_STATE));
		CHECK_EQ_INT(atomic_load(&wheel_command) & 255, slots[i] - 1);
	}
	CHECK_TRUE(change_number(2, "WHEEL_SLOT", "SLOT", 0, INDIGO_OK_STATE));
	int before = atomic_load(&calibrations);
	CHECK_TRUE(change_switch(2, "X_CALIBRATE", "START", INDIGO_BUSY_STATE));
	CHECK_TRUE(wait_value(&calibrations, before + 1));
	atomic_store(&moving, false);
	CHECK_TRUE(wait_property(2, "X_CALIBRATE", 0, INDIGO_OK_STATE));
	atomic_store(&fail_option, TOUPCAM_OPTION_FILTERWHEEL_POSITION);
	CHECK_TRUE(change_switch(2, "X_CALIBRATE", "START", INDIGO_ALERT_STATE));
	atomic_store(&fail_option, -1);
cleanup:
	stop_properties();
}

static void focuser_workflows(void) {
	CHECK_TRUE(start_properties());
	CHECK_TRUE(change_switch(3, "FOCUSER_REVERSE_MOTION", "ENABLED", INDIGO_OK_STATE));
	CHECK_EQ_INT(atomic_load(&aaf_values[TOUPCAM_AAF_SETDIRECTION]), 0);
	CHECK_TRUE(change_switch(3, "X_AAF_BEEP", "ON", INDIGO_OK_STATE));
	CHECK_EQ_INT(atomic_load(&aaf_values[TOUPCAM_AAF_SETBUZZER]), 1);
	CHECK_TRUE(change_number(3, "FOCUSER_BACKLASH", "BACKLASH", 12, INDIGO_OK_STATE));
	CHECK_EQ_INT(atomic_load(&aaf_values[TOUPCAM_AAF_SETBACKLASH]), 12);
	CHECK_TRUE(change_number(3, "FOCUSER_LIMITS", "MAX_POSITION", 1000, INDIGO_OK_STATE));
	CHECK_EQ_INT(atomic_load(&aaf_values[TOUPCAM_AAF_SETMAXSTEP]), 1000);
	CHECK_TRUE(change_switch(3, "FOCUSER_ON_POSITION_SET", "SYNC", INDIGO_OK_STATE));
	CHECK_TRUE(change_number(3, "FOCUSER_POSITION", "POSITION", 100, INDIGO_OK_STATE));
	CHECK_EQ_INT(atomic_load(&aaf_values[TOUPCAM_AAF_SETZERO]), 100);
	CHECK_TRUE(change_switch(3, "FOCUSER_ON_POSITION_SET", "GOTO", INDIGO_OK_STATE));
	CHECK_TRUE(change_switch(3, "FOCUSER_DIRECTION", "MOVE_OUTWARD", INDIGO_OK_STATE));
	CHECK_TRUE(change_number(3, "FOCUSER_STEPS", "STEPS", 20, INDIGO_OK_STATE));
	CHECK_EQ_INT(atomic_load(&focus_position), 120);
	CHECK_TRUE(change_switch(3, "FOCUSER_DIRECTION", "MOVE_INWARD", INDIGO_OK_STATE));
	CHECK_TRUE(change_number(3, "FOCUSER_STEPS", "STEPS", 200, INDIGO_OK_STATE));
	CHECK_EQ_INT(atomic_load(&focus_position), 0);
	CHECK_TRUE(change_number(3, "FOCUSER_COMPENSATION", "COMPENSATION", 10, INDIGO_OK_STATE));
	CHECK_TRUE(change_switch(3, "FOCUSER_MODE", "AUTOMATIC", INDIGO_OK_STATE));
	indigo_property *position = snapshot(3, "FOCUSER_POSITION");
	bool read_only = position && position->perm == INDIGO_RO_PERM;
	indigo_release_property(position);
	CHECK_TRUE(read_only);
	// Unchanged temperatures are suppressed by the bus; wait for the SDK sample establishing the automatic baseline.
	CHECK_TRUE(wait_value(&ambient_reads, atomic_load(&ambient_reads) + 1));
	atomic_store(&hold_focus, true);
	atomic_store(&ambient_temperature, 30);
	CHECK_TRUE(wait_value(&aaf_values[TOUPCAM_AAF_SETPOSITION], 30));
	CHECK_TRUE(wait_property(3, "FOCUSER_POSITION", 0, INDIGO_BUSY_STATE));
	CHECK_TRUE(change_switch(3, "FOCUSER_MODE", "MANUAL", INDIGO_OK_STATE));
	CHECK_TRUE(change_switch(3, "FOCUSER_DIRECTION", "MOVE_OUTWARD", INDIGO_OK_STATE));
	CHECK_TRUE(change_number(3, "FOCUSER_STEPS", "STEPS", 20, INDIGO_BUSY_STATE));
	CHECK_TRUE(wait_value(&aaf_values[TOUPCAM_AAF_SETPOSITION], 20));
	atomic_store(&focus_position, 20);
	atomic_store(&hold_focus, false);
	CHECK_TRUE(wait_property(3, "FOCUSER_POSITION", 0, INDIGO_OK_STATE));
	atomic_store(&fail_aaf, TOUPCAM_AAF_GETAMBIENTTEMP);
	CHECK_TRUE(wait_property(3, "FOCUSER_TEMPERATURE", revision(3, "FOCUSER_TEMPERATURE"), INDIGO_OK_STATE));
	atomic_store(&fail_aaf, -1);
	const int errors[] = { TOUPCAM_AAF_SETDIRECTION, TOUPCAM_AAF_SETBUZZER, TOUPCAM_AAF_SETBACKLASH, TOUPCAM_AAF_HALT };
	for (unsigned i = 0; i < ARRAY_SIZE(errors); i++) {
		atomic_store(&fail_aaf, errors[i]);
		if (i == 0) { CHECK_TRUE(change_switch(3, "FOCUSER_REVERSE_MOTION", "DISABLED", INDIGO_OK_STATE)); }
		if (i == 1) { CHECK_TRUE(change_switch(3, "X_AAF_BEEP", "OFF", INDIGO_ALERT_STATE)); }
		if (i == 2) { CHECK_TRUE(change_number(3, "FOCUSER_BACKLASH", "BACKLASH", 15, INDIGO_ALERT_STATE)); }
		if (i == 3) { CHECK_TRUE(change_switch(3, "FOCUSER_ABORT_MOTION", "ABORT_MOTION", INDIGO_ALERT_STATE)); }
	}
	atomic_store(&fail_aaf, -1);
cleanup:
	stop_properties();
}

static void replay_barrier(indigo_device *device) {
	atomic_fetch_add(&replay_done, 1);
}

static void queued_guide_replacement(void) {
	CHECK_TRUE(start_properties());
	arm_gate(&queue_gate);
	indigo_execute_handler(logical[0], queue_gate_handler);
	CHECK_TRUE(wait_value(&queue_gate.entered, 1));
	int before = atomic_load(&pulse_calls);
	unsigned revisions[] = { revision(1, "GUIDER_GUIDE_RA"), revision(1, "GUIDER_GUIDE_DEC") };
	for (int axis = 0; axis < 2; axis++) {
		const char *property = axis ? "GUIDER_GUIDE_DEC" : "GUIDER_GUIDE_RA";
		const char *items[] = { axis ? "NORTH" : "EAST", axis ? "SOUTH" : "WEST" };
		indigo_change_number_property(NULL, logical[1]->name, property, 2, items, (double []){ 100, 0 });
		indigo_change_number_property(NULL, logical[1]->name, property, 2, items, (double []){ 0, 80 });
	}
	release_gate(&queue_gate);
	CHECK_TRUE(wait_property(1, "GUIDER_GUIDE_RA", revisions[0], INDIGO_OK_STATE));
	CHECK_TRUE(wait_property(1, "GUIDER_GUIDE_DEC", revisions[1], INDIGO_OK_STATE));
	CHECK_EQ_INT(atomic_load(&pulse_calls), before + 2);
	CHECK_EQ_INT(atomic_load(&pulse_duration), 80);
cleanup:
	release_gate(&queue_gate);
	stop_properties();
}

static void slow_camera_connection_temperature(void) {
	enable_full_camera();
	CHECK_TRUE(start_properties());
	atomic_store(&track_controls, false);
	connect_device(0, false);
	CHECK_TRUE(wait_value(&connection[0], 0));
	arm_gate(&connection_gate);
	atomic_store(&immediate_temperature_next, true);
	int before = atomic_load(&temperature_reads);
	connect_device(0, true);
	CHECK_TRUE(wait_value(&connection_gate.entered, 1));
	// Make the first five-second deadline expire during initialization, without a wall-clock sleep.
	atomic_store(&replay_done, 0);
	indigo_execute_handler(logical[0], replay_barrier);
	CHECK_TRUE(!atomic_load(&immediate_temperature_next));
	CHECK_EQ_INT(atomic_load(&temperature_reads), before);
	release_gate(&connection_gate);
	CHECK_TRUE(wait_value(&replay_done, 1));
	CHECK_TRUE(wait_value(&connection[0], 1));
	CHECK_TRUE(wait_value(&temperature_reads, before + 1));
cleanup:
	release_gate(&connection_gate);
	atomic_store(&immediate_temperature_next, false);
	restore_camera();
}

static void stale_setup_errors(void) {
	enable_full_camera();
	CHECK_TRUE(start_properties());
	atomic_store(&deliver_image, true);
	CHECK_TRUE(change_number(0, "CCD_EXPOSURE", "EXPOSURE", 0.1, INDIGO_OK_STATE));
	const int events[] = { TOUPCAM_EVENT_ERROR, TOUPCAM_EVENT_NOFRAMETIMEOUT };
	const char *modes[] = { "EXPAND", "AVERAGE" };
	for (unsigned i = 0; i < ARRAY_SIZE(events); i++) {
		CHECK_TRUE(change_switch(0, "X_CCD_BIN_MODE", modes[i], INDIGO_OK_STATE));
		atomic_store(&stop_event, events[i]);
		int notifications = atomic_load(&stop_notifications);
		int images = atomic_load(&blobs);
		CHECK_TRUE(change_number(0, "CCD_EXPOSURE", "EXPOSURE", 0.1, INDIGO_OK_STATE));
		CHECK_EQ_INT(atomic_load(&stop_notifications), notifications + 1);
		atomic_store(&stop_event, 0);
		CHECK_TRUE(change_number(0, "CCD_EXPOSURE", "EXPOSURE", 0.1, INDIGO_OK_STATE));
		CHECK_EQ_INT(atomic_load(&blobs), images + 2);
	}
cleanup:
	atomic_store(&stop_event, 0);
	restore_camera();
}

static void disconnected_recurring_tasks(void) {
	enable_full_camera();
	CHECK_TRUE(start_properties());
	CHECK_TRUE(change_number(2, "WHEEL_SLOT", "SLOT", 2, INDIGO_OK_STATE));
	CHECK_TRUE(change_number(3, "FOCUSER_STEPS", "STEPS", 20, INDIGO_OK_STATE));
	indigo_timer_callback callbacks[] = {
		atomic_load(&monitor_tasks[0]), atomic_load(&move_tasks[2]),
		atomic_load(&move_tasks[3]), atomic_load(&monitor_tasks[3])
	};
	int indices[] = { 0, 2, 3, 3 };
	atomic_store(&track_controls, false);
	for (int i = 0; i < 4; i++) {
		connect_device(i, false);
		CHECK_TRUE(wait_value(&connection[i], 0));
	}
	int before = atomic_load(&bad_handle);
	atomic_store(&replay_done, 0);
	atomic_store(&track_controls, true);
	// Replay tasks left behind by a running handler re-enqueuing after the cancellation sweep.
	for (int i = 0; i < 4; i++) {
		CHECK_TRUE(callbacks[i] != NULL);
		indigo_execute_handler(logical[indices[i]], callbacks[i]);
		indigo_execute_handler(logical[indices[i]], replay_barrier);
		CHECK_TRUE(wait_value(&replay_done, i + 1));
	}
	CHECK_EQ_INT(atomic_load(&bad_handle), before);
cleanup:
	restore_camera();
}

static void combined_device_discovery(void) {
	unsigned long long original_flags = models[0].flag;
	CHECK_TRUE(start_properties());
	stop_properties();
	combined_model = true;
	memset(combined_names, 0, sizeof(combined_names));
	models[0].flag |= TOUPCAM_FLAG_FILTERWHEEL | TOUPCAM_FLAG_AUTOFOCUSER;
	clear_observer();
	indigo_start();
	indigo_attach_client(&client);
	have_thread = false;
	atomic_store(&visible, 1);
	CHECK_EQ_INT(indigo_ccd_touptek(INDIGO_DRIVER_INIT, NULL), INDIGO_OK);
	CHECK_TRUE(wait_value(&attached, 15));
	for (int i = 0; i < 4; i++) {
		CHECK_TRUE(logical[i] != NULL && logical[i]->attach == attach_functions[i]);
	}
	usb_callback(NULL, NULL, LIBUSB_HOTPLUG_EVENT_DEVICE_ARRIVED, NULL);
	// Shutdown is a driver-queue barrier and must detach every combined interface.
	CHECK_EQ_INT(indigo_ccd_touptek(INDIGO_DRIVER_SHUTDOWN, NULL), INDIGO_OK);
	CHECK_EQ_INT(atomic_load(&attached), 0);
	CHECK_EQ_INT(atomic_load(&opened), atomic_load(&closed));
cleanup:
	stop_properties();
	models[0].flag = original_flags;
	combined_model = false;
}

static void hotplug_workflows(void) {
	CHECK_TRUE(start_properties());
	atomic_store(&track_controls, false);
	int before = atomic_load(&opened);
	for (int i = 0; i < 5; i++) {
		usb_callback(NULL, NULL, LIBUSB_HOTPLUG_EVENT_DEVICE_ARRIVED, NULL);
	}
	CHECK_EQ_INT(indigo_ccd_touptek(INDIGO_DRIVER_SHUTDOWN, NULL), INDIGO_BUSY);
	CHECK_TRUE(usb_callback != NULL);
	CHECK_EQ_INT(atomic_load(&opened), before);
	// Active exposure, streaming and guide callbacks must not survive physical removal.
	CHECK_TRUE(change_number(0, "CCD_EXPOSURE", "EXPOSURE", 10, INDIGO_BUSY_STATE));
	atomic_store(&image_on_stop, true);
	CHECK_TRUE(change_number(1, "GUIDER_GUIDE_DEC", "NORTH", 1000, INDIGO_BUSY_STATE));
	atomic_store(&visible, 0);
	usb_callback(NULL, NULL, LIBUSB_HOTPLUG_EVENT_DEVICE_LEFT, NULL);
	CHECK_TRUE(wait_value(&attached, 0));
	CHECK_EQ_INT(atomic_load(&opened), atomic_load(&closed));
	CHECK_EQ_INT(atomic_load(&locks), 0);
	for (int failed = 0; failed < 4; failed++) {
		atomic_store(&fail_attach_index, failed);
		atomic_store(&fail_attach, true);
		atomic_store(&visible, failed < 2 ? 1 : (1 << (failed - 1)));
		usb_callback(NULL, NULL, LIBUSB_HOTPLUG_EVENT_DEVICE_ARRIVED, NULL);
		// A subsequent successful wheel/focuser/CCD attach is the queue barrier.
		atomic_store(&visible, atomic_load(&visible) | (failed < 2 ? 4 : 1));
		usb_callback(NULL, NULL, LIBUSB_HOTPLUG_EVENT_DEVICE_ARRIVED, NULL);
		CHECK_TRUE(wait_value(&attached, failed < 2 ? 8 : 3));
		atomic_store(&fail_attach, false);
		usb_callback(NULL, NULL, LIBUSB_HOTPLUG_EVENT_DEVICE_ARRIVED, NULL);
		CHECK_TRUE(wait_value(&attached, failed < 2 ? 11 : (failed == 2 ? 7 : 11)));
		atomic_store(&visible, 0);
		usb_callback(NULL, NULL, LIBUSB_HOTPLUG_EVENT_DEVICE_LEFT, NULL);
		CHECK_TRUE(wait_value(&attached, 0));
	}
	CHECK_EQ_INT(atomic_load(&bad_handle), 0);
cleanup:
	atomic_store(&fail_attach, false);
	stop_properties();
}

static void streaming_workflows(void) {
	CHECK_TRUE(start_properties());
	CHECK_TRUE(change_switch(0, "CCD_UPLOAD_MODE", "CLIENT", INDIGO_OK_STATE));
	const char *formats[] = { "RAW" };
	for (unsigned i = 0; i < ARRAY_SIZE(formats); i++) {
		CHECK_TRUE(change_switch(0, "CCD_IMAGE_FORMAT", formats[i], INDIGO_OK_STATE));
		atomic_store(&stream_frames, 3);
		int before = atomic_load(&pulled_images);
		unsigned state_before = revision(0, "CCD_STREAMING");
		indigo_change_number_property(NULL, logical[0]->name, "CCD_STREAMING", 2, (const char *[]){ "EXPOSURE", "COUNT" }, (double []){ 0.05, 3 });
		CHECK_TRUE(wait_property(0, "CCD_STREAMING", state_before, INDIGO_OK_STATE));
		CHECK_EQ_INT(atomic_load(&pulled_images), before + 3);
		CHECK_EQ_INT(atomic_load(&invalid_blobs), 0);
		CHECK_EQ_INT(atomic_load(&last_exposure), 50000);
	}
	const int events[] = { TOUPCAM_EVENT_ERROR, TOUPCAM_EVENT_NOFRAMETIMEOUT, TOUPCAM_EVENT_NOPACKETTIMEOUT };
	CHECK_TRUE(change_switch(0, "CCD_IMAGE_FORMAT", "RAW", INDIGO_OK_STATE));
	for (unsigned i = 0; i < ARRAY_SIZE(events); i++) {
		atomic_store(&sdk_event, events[i]);
		atomic_store(&stream_frames, 1);
		unsigned before = revision(0, "CCD_STREAMING");
		indigo_change_number_property(NULL, logical[0]->name, "CCD_STREAMING", 2, (const char *[]){ "EXPOSURE", "COUNT" }, (double []){ 0.1, -1 });
		CHECK_TRUE(wait_property(0, "CCD_STREAMING", before, INDIGO_ALERT_STATE));
		atomic_store(&sdk_event, TOUPCAM_EVENT_IMAGE);
		atomic_store(&deliver_image, true);
		CHECK_TRUE(change_number(0, "CCD_EXPOSURE", "EXPOSURE", 0.1, INDIGO_OK_STATE));
		atomic_store(&deliver_image, false);
	}
	atomic_store(&stream_frames, 1);
	unsigned before = revision(0, "CCD_STREAMING");
	indigo_change_number_property(NULL, logical[0]->name, "CCD_STREAMING", 2, (const char *[]){ "EXPOSURE", "COUNT" }, (double []){ 0.1, -1 });
	CHECK_TRUE(wait_property(0, "CCD_STREAMING", before, INDIGO_BUSY_STATE));
	atomic_store(&image_on_stop, true);
	atomic_store(&track_controls, false);
	atomic_store(&visible, 0);
	usb_callback(NULL, NULL, LIBUSB_HOTPLUG_EVENT_DEVICE_LEFT, NULL);
	CHECK_TRUE(wait_value(&attached, 0));
	CHECK_EQ_INT(atomic_load(&opened), atomic_load(&closed));
	CHECK_EQ_INT(atomic_load(&bad_handle), 0);
cleanup:
	stop_properties();
}

static void frame_abort_races(void) {
	CHECK_TRUE(start_properties());
	CHECK_TRUE(change_switch(0, "CCD_IMAGE_FORMAT", "RAW", INDIGO_OK_STATE));
	for (int frame_first = 0; frame_first < 2; frame_first++) {
		atomic_store(&stream_frames, 0);
		int starts = atomic_load(&stream_started);
		unsigned stream_revision = revision(0, "CCD_STREAMING");
		indigo_change_number_property(NULL, logical[0]->name, "CCD_STREAMING", 2, (const char *[]){ "EXPOSURE", "COUNT" }, (double []){ 0.05, 1 });
		CHECK_TRUE(wait_property(0, "CCD_STREAMING", stream_revision, INDIGO_BUSY_STATE));
		CHECK_TRUE(wait_value(&stream_started, starts + 1));
		// Wait for delayed setup to reach the SDK, then place a barrier behind its handler.
		CHECK_TRUE(change_number(0, "CCD_GAIN", "GAIN", 10 + frame_first, INDIGO_OK_STATE));
		int images = atomic_load(&pulled_images), before_blobs = atomic_load(&blobs);
		if (frame_first) {
			arm_gate(&pull_gate);
			emit_image();
			CHECK_TRUE(wait_value(&pull_gate.entered, 1));
		} else {
			arm_gate(&queue_gate);
			indigo_execute_handler(logical[0], queue_gate_handler);
			CHECK_TRUE(wait_value(&queue_gate.entered, 1));
		}
		unsigned abort_revision = revision(0, "CCD_ABORT_EXPOSURE");
		indigo_change_switch_property_1(NULL, logical[0]->name, "CCD_ABORT_EXPOSURE", "ABORT_EXPOSURE", true);
		if (!frame_first) {
			// Both tasks are pending, with abort queued before the final frame notification.
			emit_image();
		}
		release_gate(&pull_gate);
		release_gate(&queue_gate);
		// Base CCD semantics: a cancelled finite stream is ALERT; abort after completion is ALERT.
		CHECK_TRUE(wait_property(0, "CCD_ABORT_EXPOSURE", abort_revision, frame_first ? INDIGO_ALERT_STATE : INDIGO_OK_STATE));
		CHECK_TRUE(wait_property(0, "CCD_STREAMING", stream_revision, frame_first ? INDIGO_OK_STATE : INDIGO_ALERT_STATE));
		CHECK_TRUE(change_number(0, "CCD_GAIN", "GAIN", 20 + frame_first, INDIGO_OK_STATE));
		CHECK_EQ_INT(atomic_load(&pulled_images), images + frame_first);
		CHECK_EQ_INT(atomic_load(&blobs), before_blobs + frame_first);
		atomic_store(&deliver_image, true);
		CHECK_TRUE(change_number(0, "CCD_EXPOSURE", "EXPOSURE", 0.05, INDIGO_OK_STATE));
		atomic_store(&deliver_image, false);
		printf("    final-frame/abort order: %s passed\n", frame_first ? "frame first" : "abort first");
	}
	CHECK_EQ_INT(atomic_load(&gate_timeouts), 0);
	CHECK_EQ_INT(atomic_load(&bad_handle), 0);
cleanup:
	release_gate(&pull_gate);
	release_gate(&queue_gate);
	stop_properties();
}

static void disconnect_callback_race(void) {
	CHECK_TRUE(start_properties());
	int before_trigger = atomic_load(&triggers);
	CHECK_TRUE(change_number(0, "CCD_EXPOSURE", "EXPOSURE", 10, INDIGO_BUSY_STATE));
	CHECK_TRUE(wait_value(&triggers, before_trigger + 1));
	CHECK_TRUE(change_number(0, "CCD_GAIN", "GAIN", 15, INDIGO_OK_STATE));
	int images = atomic_load(&pulled_images), before_blobs = atomic_load(&blobs);
	int returned = atomic_load(&callback_returned);
	atomic_store(&callback_joining, 0);
	arm_gate(&callback_gate);
	CHECK_EQ_INT(pthread_create(&async_callback_thread, NULL, deliver_sdk_image, NULL), 0);
	atomic_store(&async_callback_active, true);
	CHECK_TRUE(wait_value(&callback_gate.entered, 1));
	// The real driver callback is paused inside its enqueue call, holding the old generation.
	atomic_store(&track_controls, false);
	connect_device(0, false);
	CHECK_TRUE(wait_value(&callback_joining, 1));
	CHECK_EQ_INT(atomic_load(&callback_returned), returned);
	CHECK_EQ_INT(atomic_load(&connection[0]), 2);
	release_gate(&callback_gate);
	CHECK_TRUE(wait_value(&connection[0], 0));
	CHECK_EQ_INT(atomic_load(&callback_returned), returned + 1);
	CHECK_EQ_INT(atomic_load(&pulled_images), images);
	CHECK_EQ_INT(atomic_load(&blobs), before_blobs);
	CHECK_EQ_INT(atomic_load(&connection[1]), 1);
	connect_device(0, true);
	CHECK_TRUE(wait_value(&connection[0], 1));
	atomic_store(&deliver_image, true);
	CHECK_TRUE(change_number(0, "CCD_EXPOSURE", "EXPOSURE", 0.05, INDIGO_OK_STATE));
	CHECK_EQ_INT(atomic_load(&pulled_images), images + 1);
	CHECK_EQ_INT(atomic_load(&gate_timeouts), 0);
	CHECK_EQ_INT(atomic_load(&bad_handle), 0);
cleanup:
	release_gate(&callback_gate);
	stop_properties();
	if (atomic_exchange(&async_callback_active, false)) {
		pthread_join(async_callback_thread, NULL);
	}
}

static void *shutdown_driver_thread(void *unused) {
	atomic_store(&shutdown_result, indigo_ccd_touptek(INDIGO_DRIVER_SHUTDOWN, NULL));
	atomic_store(&shutdown_done, 1);
	return NULL;
}

static void hotplug_pending_races(void) {
	pthread_t shutdown_thread;
	bool shutdown_started = false;
	CHECK_TRUE(start_properties());
	atomic_store(&track_controls, false);
	for (int cycle = 0; cycle < 8; cycle++) {
		atomic_store(&visible, 0);
		for (int i = 0; i < 32; i++) {
			usb_callback(NULL, NULL, LIBUSB_HOTPLUG_EVENT_DEVICE_LEFT, NULL);
			usb_callback(NULL, NULL, LIBUSB_HOTPLUG_EVENT_DEVICE_ARRIVED, NULL);
		}
		CHECK_TRUE(wait_value(&attached, 0));
		CHECK_EQ_INT(atomic_load(&opened), atomic_load(&closed));
		CHECK_EQ_INT(atomic_load(&locks), 0);
		atomic_store(&visible, 7);
		for (int i = 0; i < 32; i++) {
			usb_callback(NULL, NULL, LIBUSB_HOTPLUG_EVENT_DEVICE_ARRIVED, NULL);
			usb_callback(NULL, NULL, LIBUSB_HOTPLUG_EVENT_DEVICE_LEFT, NULL);
		}
		CHECK_TRUE(wait_value(&attached, 15));
		for (int i = 0; i < 4; i++) {
			connect_device(i, true);
			CHECK_TRUE(wait_value(&connection[i], 1));
		}
	}
	for (int i = 0; i < 4; i++) {
		connect_device(i, false);
		CHECK_TRUE(wait_value(&connection[i], 0));
	}
	arm_gate(&queue_gate);
	indigo_queue_add(test_driver_queue, NULL, INDIGO_TASK_PRIORITY_NORMAL, 0, queue_gate_handler, NULL);
	CHECK_TRUE(wait_value(&queue_gate.entered, 1));
	int enumerations = atomic_load(&enum_calls), registration = atomic_load(&deregistrations);
	for (int i = 0; i < 32; i++) {
		usb_callback(NULL, NULL, LIBUSB_HOTPLUG_EVENT_DEVICE_ARRIVED, NULL);
		usb_callback(NULL, NULL, LIBUSB_HOTPLUG_EVENT_DEVICE_LEFT, NULL);
	}
	atomic_store(&shutdown_done, 0);
	CHECK_EQ_INT(pthread_create(&shutdown_thread, NULL, shutdown_driver_thread, NULL), 0);
	shutdown_started = true;
	CHECK_TRUE(wait_value(&deregistrations, registration + 1));
	CHECK_EQ_INT(atomic_load(&shutdown_done), 0);
	release_gate(&queue_gate);
	CHECK_TRUE(wait_value(&shutdown_done, 1));
	pthread_join(shutdown_thread, NULL);
	shutdown_started = false;
	CHECK_EQ_INT(atomic_load(&shutdown_result), INDIGO_OK);
	CHECK_EQ_INT(atomic_load(&enum_calls), enumerations + 96);
	CHECK_EQ_INT(atomic_load(&attached), 0);
	CHECK_TRUE(usb_callback == NULL);
	CHECK_EQ_INT(atomic_load(&opened), atomic_load(&closed));
	CHECK_EQ_INT(atomic_load(&locks), 0);
	have_thread = false;
	CHECK_EQ_INT(indigo_ccd_touptek(INDIGO_DRIVER_INIT, NULL), INDIGO_OK);
	CHECK_TRUE(wait_value(&attached, 15));
	for (int i = 0; i < 4; i++) {
		connect_device(i, true);
		CHECK_TRUE(wait_value(&connection[i], 1));
		connect_device(i, false);
		CHECK_TRUE(wait_value(&connection[i], 0));
	}
	CHECK_EQ_INT(indigo_ccd_touptek(INDIGO_DRIVER_SHUTDOWN, NULL), INDIGO_OK);
	CHECK_EQ_INT(atomic_load(&attached), 0);
	CHECK_EQ_INT(atomic_load(&opened), atomic_load(&closed));
	CHECK_EQ_INT(atomic_load(&locks), 0);
	CHECK_EQ_INT(atomic_load(&gate_timeouts), 0);
	CHECK_EQ_INT(atomic_load(&bad_handle), 0);
	printf("    8 reconnect cycles, 1024 USB notifications, 64 pending shutdown events passed\n");
cleanup:
	release_gate(&queue_gate);
	if (shutdown_started) {
		pthread_join(shutdown_thread, NULL);
	}
	stop_properties();
}

static int compare_error(const void *left, const void *right) {
	double a = *(const double *)left, b = *(const double *)right;
	return (a > b) - (a < b);
}

static void guider_timing(void) {
	CHECK_TRUE(start_properties());
	pthread_mutex_lock(&property_mutex);
	guide_samples = 0;
	pthread_mutex_unlock(&property_mutex);
	const int durations[] = { 20, 50, 100, 250, 500 };
	const char *properties[] = { "GUIDER_GUIDE_RA", "GUIDER_GUIDE_RA", "GUIDER_GUIDE_DEC", "GUIDER_GUIDE_DEC" };
	const char *directions[] = { "EAST", "WEST", "NORTH", "SOUTH" };
	for (int direction = 0; direction < 4; direction++) {
		for (unsigned i = 0; i < ARRAY_SIZE(durations); i++) {
			CHECK_TRUE(change_number(1, properties[direction], directions[direction], durations[i], INDIGO_OK_STATE));
			CHECK_EQ_INT(atomic_load(&pulse_duration), durations[i]);
			CHECK_EQ_INT(atomic_load(&pulse_direction), ((int []){ 2, 3, 0, 1 })[direction]);
			pthread_mutex_lock(&property_mutex);
			double error = guide_samples ? guide_errors_ms[guide_samples - 1] : NAN;
			pthread_mutex_unlock(&property_mutex);
			printf("    pulse %-5s requested=%3d ms measured=%8.3f ms error=%+7.3f ms\n", directions[direction], durations[i], durations[i] + error, error);
		}
	}
	double errors[128], mean = 0, mean_absolute = 0;
	pthread_mutex_lock(&property_mutex);
	int count = guide_samples;
	memcpy(errors, guide_errors_ms, count * sizeof(double));
	pthread_mutex_unlock(&property_mutex);
	CHECK_EQ_INT(count, 20);
	for (int i = 0; i < count; i++) {
		CHECK_TRUE(isfinite(errors[i]));
		// A generous scheduler-latency limit detects missing/incorrect completion, not real ST4 accuracy.
		CHECK_TRUE(errors[i] >= -5 && errors[i] < 250);
		mean += errors[i];
		mean_absolute += fabs(errors[i]);
		errors[i] = fabs(errors[i]);
	}
	qsort(errors, count, sizeof(double), compare_error);
	printf("    guiding software timing: n=%d mean=%+.3f ms mean_abs=%.3f ms p95_abs=%.3f ms max_abs=%.3f ms\n", count, mean / count, mean_absolute / count, errors[(int)ceil(count * 0.95) - 1], errors[count - 1]);
cleanup:
	stop_properties();
}

static void configuration_workflows(void) {
	indigo_property *copy = NULL;
	enable_full_camera();
	CHECK_TRUE(start_properties());
	CHECK_TRUE(change_number(0, "X_CCD_ADVANCED", "SPEED", 2, INDIGO_OK_STATE));
	CHECK_TRUE(change_switch(0, "X_CCD_CONVERSION_GAIN", "HCG", INDIGO_OK_STATE));
	CHECK_TRUE(change_switch(0, "X_CCD_LED", "ON", INDIGO_OK_STATE));
	CHECK_TRUE(change_switch(2, "X_WHEEL_MODEL", "5_POSITIONS", INDIGO_OK_STATE));
	for (int d = 0; d < 4; d++) {
		if (d != 1) {
			unsigned before = revision(d, "CONFIG");
			indigo_change_switch_property(NULL, logical[d]->name, "CONFIG", 3, (const char *[]){ "LOAD", "SAVE", "REMOVE" }, (bool []){ false, false, false });
			CHECK_TRUE(wait_property(d, "CONFIG", before, INDIGO_OK_STATE));
		}
		CHECK_TRUE(change_switch(d, "CONFIG", "SAVE", INDIGO_OK_STATE));
	}
	DIR *directory = opendir(test_output_folder);
	int configs = 0;
	if (directory) {
		struct dirent *entry;
		while ((entry = readdir(directory))) {
			if (strstr(entry->d_name, ".config")) { configs++; }
		}
		closedir(directory);
	}
	// Guider has no persistent settings, so SAVE need not create a file.
	CHECK_EQ_INT(configs, 3);
	CHECK_TRUE(change_number(0, "X_CCD_ADVANCED", "SPEED", 4, INDIGO_OK_STATE));
	CHECK_TRUE(change_switch(0, "X_CCD_CONVERSION_GAIN", "LCG", INDIGO_OK_STATE));
	unsigned before = revision(0, "X_CCD_ADVANCED");
	CHECK_TRUE(change_switch(0, "CONFIG", "LOAD", INDIGO_OK_STATE));
	CHECK_TRUE(wait_property(0, "X_CCD_ADVANCED", before, INDIGO_OK_STATE));
	CHECK_TRUE(wait_value(&advanced_values[0], 2));
	CHECK_TRUE(wait_value(&option_values[TOUPCAM_OPTION_CG], 1));
	CHECK_TRUE(change_switch(2, "X_WHEEL_MODEL", "8_POSITIONS", INDIGO_OK_STATE));
	CHECK_TRUE(change_switch(2, "CONFIG", "LOAD", INDIGO_OK_STATE));
	CHECK_TRUE(wait_value(&wheel_slots, 5));
cleanup:
	indigo_release_property(copy);
	restore_camera();
}

static bool poll_camera(void) {
	indigo_timer_callback callback = atomic_load(&monitor_tasks[0]);
	if (!callback) { return false; }
	atomic_store(&replay_done, 0);
	indigo_execute_handler(logical[0], callback);
	indigo_execute_handler(logical[0], replay_barrier);
	return wait_value(&replay_done, 1);
}

static void camera_read_failures_and_cooling(void) {
	indigo_property *p = NULL;
	enable_full_camera();
	CHECK_TRUE(start_properties());
	atomic_store(&independent_temperature, true);
	atomic_store(&sensor_temperature, 125);
	atomic_store(&option_values[TOUPCAM_OPTION_TEC_VOLTAGE], 50);
	CHECK_TRUE(change_switch(0, "CCD_COOLER", "ON", INDIGO_OK_STATE));
	CHECK_TRUE(change_number(0, "CCD_TEMPERATURE", "TEMPERATURE", -10, INDIGO_BUSY_STATE));
	CHECK_TRUE(poll_camera());
	CHECK_EQ_INT(atomic_load(&target_temperature), -100);
	p = snapshot(0, "CCD_TEMPERATURE");
	CHECK_TRUE(p && p->items[0].number.value == 12.5 && p->items[0].number.target == -10);
	indigo_release_property(p); p = NULL;
	p = snapshot(0, "CCD_COOLER_POWER");
	CHECK_TRUE(p && p->state == INDIGO_OK_STATE && p->items[0].number.value == 50);
	indigo_release_property(p); p = NULL;
	atomic_store(&read_failure, 1);
	CHECK_TRUE(poll_camera());
	p = snapshot(0, "CCD_TEMPERATURE");
	CHECK_TRUE(p && p->state == INDIGO_ALERT_STATE && p->items[0].number.value == 12.5);
	indigo_release_property(p); p = NULL;
	atomic_store(&read_failure, 0);
	for (int i = 0; i < 2; i++) {
		atomic_store(&failed_get_option, i ? TOUPCAM_OPTION_TEC_VOLTAGE_MAX : TOUPCAM_OPTION_TEC_VOLTAGE);
		CHECK_TRUE(poll_camera());
		p = snapshot(0, "CCD_COOLER_POWER");
		CHECK_TRUE(p && p->state == INDIGO_ALERT_STATE);
		indigo_release_property(p); p = NULL;
	}
	atomic_store(&failed_get_option, -1);
	atomic_store(&sensor_temperature, -100);
	CHECK_TRUE(poll_camera());
	p = snapshot(0, "CCD_TEMPERATURE");
	CHECK_TRUE(p && p->state == INDIGO_OK_STATE && p->items[0].number.value == -10);
	indigo_release_property(p); p = NULL;
	CHECK_TRUE(change_switch(0, "CCD_COOLER", "OFF", INDIGO_OK_STATE));
	CHECK_TRUE(poll_camera());
	p = snapshot(0, "CCD_COOLER_POWER");
	CHECK_TRUE(p && p->items[0].number.value == 0);
	indigo_release_property(p); p = NULL;
	for (int error = 2; error <= 5; error++) {
		CHECK_TRUE(change_switch(0, "CONNECTION", "DISCONNECTED", INDIGO_OK_STATE));
		atomic_store(&read_failure, error);
		CHECK_TRUE(change_switch(0, "CONNECTION", "CONNECTED", INDIGO_ALERT_STATE));
		CHECK_EQ_INT(atomic_load(&connection[1]), 1);
		atomic_store(&read_failure, 0);
		CHECK_TRUE(change_switch(0, "CONNECTION", "CONNECTED", INDIGO_OK_STATE));
	}
	const int options[] = { TOUPCAM_OPTION_TEC, TOUPCAM_OPTION_TECTARGET };
	for (int i = 0; i < ARRAY_SIZE(options); i++) {
		CHECK_TRUE(change_switch(0, "CONNECTION", "DISCONNECTED", INDIGO_OK_STATE));
		atomic_store(&failed_get_option, options[i]);
		CHECK_TRUE(change_switch(0, "CONNECTION", "CONNECTED", INDIGO_ALERT_STATE));
		atomic_store(&failed_get_option, -1);
		CHECK_TRUE(change_switch(0, "CONNECTION", "CONNECTED", INDIGO_OK_STATE));
	}
	CHECK_EQ_INT(atomic_load(&wrong_thread), 0);
cleanup:
	indigo_release_property(p);
	restore_camera();
}

static void bayer_metadata_and_failed_setup(void) {
	const char *patterns[] = { "RGGB", "BGGR", "GRBG", "GBRG", "" };
	enable_full_camera();
	CHECK_TRUE(start_properties());
	atomic_store(&deliver_image, true);
	for (int i = 0; i < 5; i++) {
		CHECK_TRUE(change_switch(0, "CONNECTION", "DISCONNECTED", INDIGO_OK_STATE));
		unsigned fourcc = 0;
		if (i < 4) { fourcc = (unsigned)patterns[i][0] | ((unsigned)patterns[i][1] << 8) | ((unsigned)patterns[i][2] << 16) | ((unsigned)patterns[i][3] << 24); }
		atomic_store(&raw_fourcc, fourcc);
		CHECK_TRUE(change_switch(0, "CONNECTION", "CONNECTED", INDIGO_OK_STATE));
		CHECK_TRUE(change_switch(0, "CCD_IMAGE_FORMAT", "RAW", INDIGO_OK_STATE));
		CHECK_TRUE(change_switch(0, "CCD_MODE", "RAW08_1", INDIGO_OK_STATE));
		CHECK_TRUE(change_number(0, "CCD_EXPOSURE", "EXPOSURE", 0.01, INDIGO_OK_STATE));
		CHECK_EQ_INT(atomic_load(&invalid_blobs), 0);
	}
	for (int error = 5; error <= 6; error++) {
		CHECK_TRUE(change_switch(0, "CCD_MODE", "RAW16_1", INDIGO_OK_STATE));
		atomic_store(&read_failure, error);
		CHECK_TRUE(change_number(0, "CCD_EXPOSURE", "EXPOSURE", 0.01, INDIGO_ALERT_STATE));
		atomic_store(&read_failure, 0);
		CHECK_TRUE(change_number(0, "CCD_EXPOSURE", "EXPOSURE", 0.01, INDIGO_OK_STATE));
	}
	CHECK_EQ_INT(atomic_load(&wrong_thread), 0);
cleanup:
	atomic_store(&raw_fourcc, 0);
	restore_camera();
}

static void multiple_camera_identity_and_capacity(void) {
	inventory_mode = true;
	inventory_reverse = true;
	atomic_store(&track_controls, false);
	clear_observer();
	have_thread = false;
	models[0].flag |= TOUPCAM_FLAG_ST4;
	indigo_start();
	indigo_attach_client(&client);
	atomic_store(&inventory_visible[0], true);
	atomic_store(&inventory_visible[1], true);
	CHECK_EQ_INT(indigo_ccd_touptek(INDIGO_DRIVER_INIT, NULL), INDIGO_OK);
	CHECK_TRUE(wait_value(&inventory_count, 4));
	CHECK_TRUE(inventory_devices[0][0] && inventory_devices[1][0]);
	CHECK_TRUE(strcmp(inventory_names[0][0], inventory_names[1][0]) != 0);
	logical[0] = inventory_devices[0][0];
	logical[1] = inventory_devices[1][0];
	CHECK_TRUE(change_switch(0, "CONNECTION", "CONNECTED", INDIGO_OK_STATE));
	CHECK_TRUE(change_switch(1, "CONNECTION", "CONNECTED", INDIGO_OK_STATE));
	CHECK_TRUE(atomic_load(&handle_open[0]) && atomic_load(&handle_open[1]));
	atomic_store(&inventory_visible[0], false);
	inventory_reverse = false;
	usb_callback(NULL, NULL, LIBUSB_HOTPLUG_EVENT_DEVICE_LEFT, NULL);
	CHECK_TRUE(wait_value(&inventory_count, 2));
	CHECK_TRUE(!atomic_load(&handle_open[0]) && atomic_load(&handle_open[1]));
	atomic_store(&roi_width, 640); atomic_store(&roi_height, 480);
	atomic_store(&deliver_image, true);
	CHECK_TRUE(change_switch(1, "CCD_IMAGE_FORMAT", "RAW", INDIGO_OK_STATE));
	CHECK_TRUE(change_number(1, "CCD_EXPOSURE", "EXPOSURE", 0.01, INDIGO_OK_STATE));
	CHECK_TRUE(inventory_frames[1] > 0);
	CHECK_TRUE(change_switch(1, "CONNECTION", "DISCONNECTED", INDIGO_OK_STATE));
	for (int i = 0; i < TOUPCAM_MAX; i++) { atomic_store(&inventory_visible[i], true); }
	usb_callback(NULL, NULL, LIBUSB_HOTPLUG_EVENT_DEVICE_ARRIVED, NULL);
	CHECK_TRUE(wait_value(&inventory_count, 2 * (TOUPCAM_MAX - 1)));
	int removed = -1, omitted = -1;
	for (int i = 0; i < TOUPCAM_MAX; i++) {
		if (inventory_devices[i][0]) { removed = i; } else { omitted = i; }
	}
	CHECK_TRUE(removed >= 0 && omitted >= 0);
	atomic_store(&inventory_visible[removed], false);
	usb_callback(NULL, NULL, LIBUSB_HOTPLUG_EVENT_DEVICE_LEFT, NULL);
	CHECK_TRUE(wait_value(&inventory_count, 2 * (TOUPCAM_MAX - 1) - 2));
	usb_callback(NULL, NULL, LIBUSB_HOTPLUG_EVENT_DEVICE_ARRIVED, NULL);
	CHECK_TRUE(wait_value(&inventory_count, 2 * (TOUPCAM_MAX - 1)));
cleanup:
	for (int i = 0; i < TOUPCAM_MAX; i++) { atomic_store(&inventory_visible[i], false); }
	if (usb_callback) { usb_callback(NULL, NULL, LIBUSB_HOTPLUG_EVENT_DEVICE_LEFT, NULL); }
	wait_value(&inventory_count, 0);
	indigo_ccd_touptek(INDIGO_DRIVER_SHUTDOWN, NULL);
	indigo_detach_client(&client);
	indigo_stop();
	inventory_mode = false;
	atomic_store(&deliver_image, false);
	memset(logical, 0, sizeof(logical));
	clear_observer();
}

int main(void) {
	if (mkdtemp(test_output_folder) == NULL) { return 1; }
	touptek_test_set_config_folder(test_output_folder);
	const indigo_test_case tests[] = {
		{ "Multiple camera identity and capacity recovery", multiple_camera_identity_and_capacity },
		{ "Camera read failures and cooling", camera_read_failures_and_cooling },
		{ "Bayer metadata and failed acquisition setup", bayer_metadata_and_failed_setup },
		{ "SDK lifecycle queue", lifecycle },
		{ "CCD property handlers and base dispatch", camera_properties },
		{ "Acquisition finalizers, watchdog and streaming", acquisition_finalizers },
		{ "Guider property handlers on camera queue", guider_properties },
		{ "Wheel property handlers", wheel_properties },
		{ "Focuser property handlers", focuser_properties },
		{ "All published properties and writable base properties", property_inventory },
		{ "Optional CCD controls and SDK errors", optional_camera_properties },
		{ "CCD formats, ROI, binning and image payloads", image_formats },
		{ "Acquisition admission, errors and reconnect", acquisition_edges },
		{ "All guider directions and cancellation", guide_workflows },
		{ "Wheel model, calibration and limits", wheel_workflows },
		{ "Focuser settings, sync, relative and automatic motion", focuser_workflows },
		{ "Queued guide requests coalesce per axis", queued_guide_replacement },
		{ "Temperature monitoring after slow camera connection", slow_camera_connection_temperature },
		{ "Stale SDK errors during acquisition setup", stale_setup_errors },
		{ "Disconnected recurring tasks", disconnected_recurring_tasks },
		{ "Combined camera, wheel and focuser discovery", combined_device_discovery },
		{ "Hot-plug duplicates, partial attach and active removal", hotplug_workflows },
		{ "Streaming formats, SDK errors and physical removal", streaming_workflows },
		{ "Race: final frame and abort", frame_abort_races },
		{ "Race: disconnect during SDK callback", disconnect_callback_race },
		{ "Race: rapid hot-plug and pending shutdown", hotplug_pending_races },
		{ "Guider pulse timing accuracy", guider_timing },
		{ "Driver configuration persistence", configuration_workflows }
	};
	setvbuf(stdout, NULL, _IONBF, 0);
	int result = 0;
	const char *filter = getenv("INDIGO_TEST_FILTER");
	if (filter) {
		int matched = 0;
		for (unsigned i = 0; i < ARRAY_SIZE(tests); i++) {
			if (strstr(tests[i].name, filter)) { result |= indigo_run_tests("ToupTek SDK", tests + i, 1); matched++; }
		}
		if (!matched) { result = 1; }
	} else {
		result = indigo_run_tests("ToupTek SDK", tests, ARRAY_SIZE(tests));
	}
	clear_observer();
	DIR *directory = opendir(test_output_folder);
	if (directory) {
		struct dirent *entry;
		while ((entry = readdir(directory))) {
			if (entry->d_name[0] != '.') {
				char path[1024];
				snprintf(path, sizeof(path), "%s/%s", test_output_folder, entry->d_name);
				if (unlink(path)) { result = 1; }
			}
		}
		closedir(directory);
	}
	if (rmdir(test_output_folder)) { result = 1; }
	return result;
}

// The SDK callback runs on a separate thread; PullImage generates deterministic noise on the device queue.
static void *deliver_sdk_image(void *unused) {
	pthread_mutex_lock(&image_mutex);
	PTOUPCAM_EVENT_CALLBACK callback = image_callback;
	void *context = image_context;
	pthread_mutex_unlock(&image_mutex);
	if (callback) { callback(atomic_load(&sdk_event), context); }
	atomic_fetch_add(&callback_returned, 1);
	return NULL;
}
static void emit_image(void) {
	pthread_t thread;
	if (pthread_create(&thread, NULL, deliver_sdk_image, NULL) == 0) { pthread_join(thread, NULL); }
}
HRESULT Toupcam_PullImageV2(HToupcam h, void* pImageData, int bits, ToupcamFrameInfoV2* pInfo) {
	enter_gate(&pull_gate);
	control_call(h);
	memset(pInfo, 0, sizeof(*pInfo));
	int bin = atomic_load(&option_values[TOUPCAM_OPTION_BINNING]) & 0x3f;
	pInfo->width = atomic_load(&roi_width) / bin;
	pInfo->height = atomic_load(&roi_height) / bin;
	if (atomic_load(&fail_pull)) { return -1; }
	for (unsigned i = 0; i < pInfo->width * pInfo->height; i++) {
		unsigned source = (atomic_load(&roi_top) + i / pInfo->width * bin) * 640 + atomic_load(&roi_left) + i % pInfo->width * bin;
		if (bits == 24) { for (int channel = 0; channel < 3; channel++) { ((unsigned char *)pImageData)[3 * i + channel] = ccd_test_noise(source, channel + 1) >> 8; } }
		else if (bits > 8 && bits <= 16) { ((unsigned short *)pImageData)[i] = ccd_test_noise(source, 0); }
		else { ((unsigned char *)pImageData)[i] = ccd_test_noise(source, 0) >> 8; }
	}
	atomic_fetch_add(&pulled_images, 1);
	return 0;
}
HRESULT Toupcam_ST4PlusGuide(HToupcam h, unsigned nDirect, unsigned nDuration) {
	control_call(h);
	atomic_store(&pulse_direction, nDirect);
	atomic_store(&pulse_duration, nDuration);
	atomic_fetch_add(&pulse_calls, 1);
	if (atomic_load(&fail_control)) { return -1; }
	int axis = nDirect >= 2 ? 0 : 1;
	atomic_store(&guide_requested_ms[axis], nDuration);
	atomic_store(&guide_started_ns[axis], monotonic_ns());
	return 0;
}
HRESULT Toupcam_StartPullModeWithCallback(HToupcam h, PTOUPCAM_EVENT_CALLBACK funEvent, void* ctxEvent) {
	if (atomic_load(&read_failure) == 5) { return -1; }
	enter_gate(&connection_gate);
	pthread_mutex_lock(&image_mutex);
	image_callback = funEvent;
	image_context = ctxEvent;
	pthread_mutex_unlock(&image_mutex);
	return 0;
}
HRESULT Toupcam_Stop(HToupcam h) {
	if (atomic_load(&read_failure) == 6) { return -1; }
	if (atomic_exchange(&async_callback_active, false)) {
		atomic_store(&callback_joining, 1);
		pthread_join(async_callback_thread, NULL);
	}
	if (atomic_load(&image_on_stop)) { emit_image(); }
	if (atomic_load(&stop_event)) {
		int previous = atomic_exchange(&sdk_event, atomic_load(&stop_event));
		emit_image();
		atomic_store(&sdk_event, previous);
		atomic_fetch_add(&stop_notifications, 1);
	}
	pthread_mutex_lock(&image_mutex);
	image_callback = NULL;
	image_context = NULL;
	pthread_mutex_unlock(&image_mutex);
	return 0;
}
HRESULT Toupcam_Trigger(HToupcam h, unsigned short nNumber) {
	control_call(h);
	if (atomic_load(&fail_trigger)) { return -1; }
	if (nNumber) {
		atomic_fetch_add(&triggers, 1);
		if (atomic_load(&deliver_image)) { emit_image(); }
	}
	return 0;
}
HRESULT Toupcam_get_ExpTimeRange(HToupcam h, unsigned* nMin, unsigned* nMax, unsigned* nDef) { if (atomic_load(&read_failure) == 2) { return -1; } if (nMin) { *nMin = 0; } if (nMax) { *nMax = 100000000; } if (nDef) { *nDef = 1000000; } return 0; }
HRESULT Toupcam_get_ExpoAGain(HToupcam h, unsigned short* Gain) { if (atomic_load(&read_failure) == 4) { return -1; } if (Gain) { *Gain = 0; } return 0; }
HRESULT Toupcam_get_ExpoAGainRange(HToupcam h, unsigned short* nMin, unsigned short* nMax, unsigned short* nDef) { if (atomic_load(&read_failure) == 3) { return -1; } if (nMin) { *nMin = 0; } if (nMax) { *nMax = 100; } if (nDef) { *nDef = 0; } return 0; }
HRESULT Toupcam_get_FanMaxSpeed(HToupcam h) { return 5; }
HRESULT Toupcam_get_FwVersion(HToupcam h, char fwver[16]) { fwver[0] = 0; return 0; }
HRESULT Toupcam_get_HwVersion(HToupcam h, char hwver[16]) { hwver[0] = 0; return 0; }
const ToupcamModelV2* Toupcam_get_Model(unsigned short idVendor, unsigned short idProduct) { return NULL; }
HRESULT Toupcam_get_RawFormat(HToupcam h, unsigned* pFourCC, unsigned* pBitsPerPixel) { if (atomic_load(&read_failure) == 7) { return -1; } if (pFourCC) { *pFourCC = atomic_load(&raw_fourcc); } if (pBitsPerPixel) { *pBitsPerPixel = 16; } return 0; }
HRESULT Toupcam_get_Speed(HToupcam h, unsigned short* pSpeed) { if (pSpeed) { *pSpeed = 0; } return 0; }
HRESULT Toupcam_get_Temperature(HToupcam h, short* pTemperature) { atomic_fetch_add(&temperature_reads, 1); control_call(h); if (atomic_load(&read_failure) == 1) { return -1; } if (pTemperature) { *pTemperature = atomic_load(atomic_load(&independent_temperature) ? &sensor_temperature : &target_temperature); } return 0; }
HRESULT Toupcam_put_AutoExpoEnable(HToupcam h, int mode) { return 0; }
HRESULT Toupcam_put_Brightness(HToupcam h, int Brightness) { control_call(h); atomic_store(&advanced_values[4], Brightness); return atomic_load(&fail_control) ? -1 : 0; }
HRESULT Toupcam_put_Contrast(HToupcam h, int Contrast) { control_call(h); atomic_store(&advanced_values[1], Contrast); return atomic_load(&fail_control) ? -1 : 0; }
HRESULT Toupcam_put_ExpoAGain(HToupcam h, unsigned short Gain) { control_call(h); atomic_store(&last_gain, Gain); atomic_fetch_add(&gain_calls, 1); return atomic_load(&fail_control) ? -1 : 0; }
HRESULT Toupcam_put_ExpoTime(HToupcam h, unsigned Time) { control_call(h); atomic_store(&last_exposure, Time); return 0; }
HRESULT Toupcam_put_Gamma(HToupcam h, int Gamma) { control_call(h); atomic_store(&advanced_values[5], Gamma); return atomic_load(&fail_control) ? -1 : 0; }
HRESULT Toupcam_put_Hue(HToupcam h, int Hue) { control_call(h); atomic_store(&advanced_values[2], Hue); return atomic_load(&fail_control) ? -1 : 0; }
HRESULT Toupcam_put_Option(HToupcam h, unsigned iOption, int iValue) {
	control_call(h);
	if ((int)iOption == atomic_load(&fail_option)) { return -1; }
	if (iOption < 256) { atomic_store(&option_values[iOption], iValue); atomic_fetch_add(&option_calls[iOption], 1); }
	if (iOption == TOUPCAM_OPTION_FILTERWHEEL_SLOT) { atomic_store(&wheel_slots, iValue); }
	if (iOption == TOUPCAM_OPTION_TRIGGER) {
		if (iValue == 0) {
			atomic_fetch_add(&stream_started, 1);
			for (int i = 0; i < atomic_load(&stream_frames); i++) { emit_image(); }
		} else if (atomic_load(&image_on_stop)) {
			emit_image();
		}
	}
	if ((iOption == TOUPCAM_OPTION_BLACKLEVEL || iOption == TOUPCAM_OPTION_FILTERWHEEL_POSITION) && atomic_load(&fail_control)) { return -1; }
	if (iOption == TOUPCAM_OPTION_FILTERWHEEL_POSITION && iValue >= 0) { atomic_store(&wheel_command, iValue); }
	if (iOption == TOUPCAM_OPTION_FILTERWHEEL_POSITION && iValue == -1) {
		atomic_store(&moving, true);
		atomic_fetch_add(&calibrations, 1);
	}
	return 0;
}
HRESULT Toupcam_put_Roi(HToupcam h, unsigned xOffset, unsigned yOffset, unsigned xWidth, unsigned yHeight) { control_call(h); atomic_store(&roi_left, xOffset); atomic_store(&roi_top, yOffset); atomic_store(&roi_width, xWidth); atomic_store(&roi_height, yHeight); return 0; }
HRESULT Toupcam_put_Saturation(HToupcam h, int Saturation) { control_call(h); atomic_store(&advanced_values[3], Saturation); return atomic_load(&fail_control) ? -1 : 0; }
HRESULT Toupcam_put_Speed(HToupcam h, unsigned short nSpeed) { control_call(h); atomic_store(&advanced_values[0], nSpeed); return atomic_load(&fail_control) ? -1 : 0; }
HRESULT Toupcam_put_Temperature(HToupcam h, short nTemperature) { control_call(h); atomic_store(&target_temperature, nTemperature); return atomic_load(&fail_control) ? -1 : 0; }
HRESULT Toupcam_put_WhiteBalanceGain(HToupcam h, int aGain[3]) { control_call(h); for (int i = 0; i < 3; i++) { atomic_store(&advanced_values[6 + i], aGain[i]); } return atomic_load(&fail_control) ? -1 : 0; }
