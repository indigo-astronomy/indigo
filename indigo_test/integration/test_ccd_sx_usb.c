// Copyright (c) 2026 CloudMakers, s. r. o.
// All rights reserved.
// Use under the INDIGO Astronomy open-source license (see LICENSE.md).
// Fake native SX USB protocol tests by OpenAI Codex.

#include <stdatomic.h>
#include <pthread.h>
#include <indigo/indigo_usb_utils.h>
#include <indigo_drivers/ccd_sx/indigo_ccd_sx.h>
#include "simulator_test_common.h"
#include "ccd_test_noise.h"

#define SX_CHECK(condition) do { if (!(condition)) { fprintf(stderr, "%s:%d: %s\n", __FILE__, __LINE__, #condition); indigo_test_failures++; goto cleanup; } } while (0)

static int usb_tokens[8];
#define usb_token usb_tokens[0]
static atomic_int attached_devices, fail_attach, fail_registration, fail_queue, fail_lock, fail_config, config_allocated, config_freed, fail_descriptor;
static atomic_int connected[2], connection_revision[2];
static indigo_device *logical_devices[8];
static indigo_queue *lifecycle_queue;
static int product = 0x0525;
static atomic_int physical_open[8], physical_held[8];
static libusb_hotplug_callback_fn usb_callback;
static atomic_int refs, opened, closed, held, after_close, bad_protocol, frames, bad_frames, relay_mask;
static atomic_int fail_open, fail_claim, fail_command, fail_read, short_read, zero_read, cooler_target, cooler_on, cooler_reads;
static atomic_int block_attach, attach_entered, attach_release, deregistered, shutdown_started, shutdown_done;
static atomic_int gate_request, gate_endpoint, gate_entered, gate_release, gate_timeout, concurrent_usb, active_usb[8];
static atomic_int pulse_count[4];
static atomic_ullong pulse_start_ns[4], pulse_duration_ns[4];
static pthread_t test_thread;
static atomic_int fast_exposure, fail_clear_at, zero_pixels;
static atomic_int short_command, short_reply, led_on;
static atomic_int sensor_temperature = 2855;
static atomic_int short_cooler_reply;
static atomic_int command_count[256], guide_state[2], bad_guide_completion;
static atomic_bool is_open;
static int model = 0x25, caps = 0x31;
static int last_request, wire_left, wire_top, wire_width, wire_height, wire_hbin, wire_vbin, wire_offset, shutter, milliseconds;
static int frame_left, frame_top, frame_width = 64, frame_height = 48, frame_bin = 1;
static const char *camera_name = "SX UltraStar #fake-0";
static const char *guider_name = "SX UltraStar #fake-0 (guider)";
static _Atomic(indigo_timer_callback) cooler_callback;
static indigo_device *camera_device;
static bool interlaced(void) { return model == 0x40; }
static bool icx453(void) { return model == 0x59; }

static const simulator_driver_case sx_case = {
	"Starlight Xpress Camera", "indigo_ccd_sx", "SX UltraStar #fake-0", indigo_ccd_sx, false,
	NULL, 0, NULL, 0, NULL, 0, NULL, 0
};

libusb_device *LIBUSB_CALL sx_test_ref(libusb_device *dev) { atomic_fetch_add(&refs, 1); return dev; }
void LIBUSB_CALL sx_test_unref(libusb_device *dev) { atomic_fetch_sub(&refs, 1); }
int LIBUSB_CALL sx_test_descriptor(libusb_device *dev, struct libusb_device_descriptor *descriptor) {
	memset(descriptor, 0, sizeof(*descriptor));
	descriptor->idVendor = 0x1278;
	descriptor->idProduct = product;
	if (atomic_load(&fail_descriptor)) { return LIBUSB_ERROR_IO; }
	return 0;
}
indigo_result sx_test_path(libusb_device *dev, char *path) { snprintf(path, INDIGO_NAME_SIZE, "fake-%d", (int)((int *)dev - usb_tokens)); return INDIGO_OK; }
void sx_test_usb_start(void) { }
int LIBUSB_CALL sx_test_register(libusb_context *ctx, int events, int flags, int vid, int pid, int cls, libusb_hotplug_callback_fn callback, void *data, libusb_hotplug_callback_handle *handle) {
	if (atomic_load(&fail_registration)) { return LIBUSB_ERROR_OTHER; }
	usb_callback = callback;
	*handle = 1;
	callback(NULL, (libusb_device *)&usb_token, LIBUSB_HOTPLUG_EVENT_DEVICE_ARRIVED, NULL);
	return 0;
}
int sx_test_register_sim(libusb_context *ctx, libusb_hotplug_event events, libusb_hotplug_flag flags, int vid, int pid, int cls, libusb_hotplug_callback_fn callback, void *data, libusb_hotplug_callback_handle *handle) { return sx_test_register(ctx, events, flags, vid, pid, cls, callback, data, handle); }
void LIBUSB_CALL sx_test_deregister(libusb_context *ctx, libusb_hotplug_callback_handle handle) { usb_callback = NULL; atomic_store(&deregistered, 1); }
int sx_test_deregister_poll(libusb_context *ctx, libusb_hotplug_callback_handle handle) { sx_test_deregister(ctx, handle); return 0; }
indigo_result sx_test_lock(indigo_device *device) {
	int index = atoi(strstr(device->name, "#fake-") + 6);
	if (atomic_load(&fail_lock) || atomic_exchange(&physical_held[index], 1)) { return INDIGO_BUSY; }
	atomic_fetch_add(&held, 1);
	return INDIGO_OK;
}
indigo_result sx_test_unlock(indigo_device *device) {
	int index = atoi(strstr(device->name, "#fake-") + 6);
	atomic_store(&physical_held[index], 0);
	atomic_fetch_sub(&held, 1);
	return INDIGO_OK;
}
int LIBUSB_CALL sx_test_open(libusb_device *dev, libusb_device_handle **handle) {
	if (atomic_load(&fail_open)) { return LIBUSB_ERROR_ACCESS; }
	*handle = (libusb_device_handle *)dev;
	int index = (int *)dev - usb_tokens;
	atomic_store(&physical_open[index], 1);
	if (!index) { atomic_store(&is_open, true); }
	atomic_fetch_add(&opened, 1);
	return 0;
}
void LIBUSB_CALL sx_test_close(libusb_device_handle *handle) {
	int index = (int *)handle - usb_tokens;
	atomic_store(&physical_open[index], 0);
	if (!index) { atomic_store(&is_open, false); }
	atomic_fetch_add(&closed, 1);
}
int LIBUSB_CALL sx_test_kernel(libusb_device_handle *handle, int interface) { return 0; }
int LIBUSB_CALL sx_test_claim(libusb_device_handle *handle, int interface) { return atomic_load(&fail_claim) ? LIBUSB_ERROR_BUSY : 0; }
int LIBUSB_CALL sx_test_config(libusb_device *dev, uint8_t index, struct libusb_config_descriptor **config) {
	static const struct libusb_interface_descriptor alternate = { .bInterfaceNumber = 0 };
	static const struct libusb_interface interface = { .altsetting = &alternate, .num_altsetting = 1 };
	static struct libusb_config_descriptor descriptor = { .interface = &interface, .bNumInterfaces = 1 };
	if (atomic_load(&fail_config)) { return LIBUSB_ERROR_IO; }
	atomic_fetch_add(&config_allocated, 1);
	*config = &descriptor;
	return 0;
}
void LIBUSB_CALL sx_test_free_config(struct libusb_config_descriptor *config) { atomic_fetch_add(&config_freed, 1); }

indigo_result sx_test_attach(indigo_device *device) {
	if (atomic_exchange(&block_attach, 0)) {
		atomic_store(&attach_entered, 1);
		for (int i = 0; i < 5000 && !atomic_load(&attach_release); i++) { indigo_usleep(1000); }
	}
	if (atomic_load(&fail_attach) == (device->master_device ? 2 : 1)) { return INDIGO_FAILED; }
	indigo_result result = indigo_attach_device(device);
	if (result == INDIGO_OK) {
		for (int i = 0; i < ARRAY_SIZE(logical_devices); i++) {
			if (!logical_devices[i]) { logical_devices[i] = device; break; }
		}
		atomic_fetch_add(&attached_devices, 1);
	}
	return result;
}

indigo_result sx_test_detach(indigo_device *device) {
	indigo_result result = indigo_detach_device(device);
	for (int i = 0; i < ARRAY_SIZE(logical_devices); i++) {
		if (logical_devices[i] == device) { logical_devices[i] = NULL; break; }
	}
	atomic_fetch_sub(&attached_devices, 1);
	return result;
}

indigo_queue *sx_test_queue_create(indigo_device *device) {
	lifecycle_queue = atomic_load(&fail_queue) ? NULL : indigo_queue_create(device);
	return lifecycle_queue;
}

static unsigned word(const unsigned char *bytes) { return bytes[0] | (bytes[1] << 8); }
static void put_word(unsigned char *bytes, unsigned value) { bytes[0] = value; bytes[1] = value >> 8; }

static int sx_bulk_impl(libusb_device_handle *handle, unsigned char endpoint, unsigned char *data, int length, int *transferred, unsigned timeout) {
	*transferred = 0;
	if (!atomic_load(&physical_open[(int *)handle - usb_tokens])) { atomic_fetch_add(&after_close, 1); return LIBUSB_ERROR_NO_DEVICE; }
	if (endpoint == 1) {
		if (length < 8 || length > 22) { atomic_fetch_add(&bad_protocol, 1); return LIBUSB_ERROR_INVALID_PARAM; }
		last_request = data[1];
		atomic_fetch_add(&command_count[last_request], 1);
		if ((last_request == 1 && atomic_load(&fail_clear_at) == atomic_load(&command_count[1])) || atomic_load(&fail_command) == last_request) { return LIBUSB_ERROR_IO; }
		if (last_request == 2 || last_request == 3) {
			if (length != (last_request == 2 ? 22 : 18)) { atomic_fetch_add(&bad_protocol, 1); }
			wire_left = word(data + 8); wire_top = word(data + 10);
			wire_width = word(data + 12); wire_height = word(data + 14);
			wire_hbin = data[16]; wire_vbin = data[17]; wire_offset = 0;
			shutter = data[3];
			if (length == 22) { milliseconds = word(data + 18) | (word(data + 20) << 16); }
		} else if (last_request == 9) {
			struct timespec now;
			clock_gettime(CLOCK_MONOTONIC, &now);
			unsigned long long ns = (unsigned long long)now.tv_sec * 1000000000ULL + now.tv_nsec;
			int previous = atomic_load(&relay_mask);
			for (int i = 0; i < 4; i++) {
				if (!(previous & (1 << i)) && (data[2] & (1 << i))) { atomic_store(&pulse_start_ns[i], ns); }
				if ((previous & (1 << i)) && !(data[2] & (1 << i))) {
					atomic_store(&pulse_duration_ns[i], ns - atomic_load(&pulse_start_ns[i]));
					atomic_fetch_add(&pulse_count[i], 1);
				}
			}
			atomic_store(&relay_mask, data[2]);
		} else if (last_request == 30) {
			atomic_store(&cooler_target, word(data + 2));
			atomic_store(&cooler_on, data[4]);
		} else if (last_request == 43) {
			atomic_store(&led_on, data[2]);
		} else if (last_request == 6) {
			atomic_store(&relay_mask, 0);
		}
		*transferred = atomic_load(&short_command) == last_request ? length - 1 : length;
		return 0;
	}
	if (endpoint != 0x82) { atomic_fetch_add(&bad_protocol, 1); return LIBUSB_ERROR_PIPE; }
	if (atomic_load(&fail_read) == last_request) { return LIBUSB_ERROR_IO; }
	memset(data, 0, length);
	if (last_request == 14 && length == 2) {
		put_word(data, model);
	} else if (last_request == 8 && length == 17) {
		put_word(data + 2, 64); put_word(data + 6, interlaced() ? 24 : 48);
		put_word(data + 8, 4 * 256); put_word(data + 10, 4 * 256);
		data[14] = 16; data[16] = caps;
	} else if (last_request == 30 && length == 3) {
		put_word(data, atomic_load(&sensor_temperature)); data[2] = atomic_load(&cooler_on);
		atomic_fetch_add(&cooler_reads, 1);
		if (atomic_load(&short_cooler_reply)) { length = 2; }
	} else if (last_request == 2 || last_request == 3) {
		if (atomic_exchange(&zero_read, 0)) { return 0; }
		if (atomic_load(&short_read) && length > 32) { length = 32; }
		for (int i = 0; i < length / 2; i++) {
			int pixel = (wire_offset / 2) + i;
			int x = wire_left + pixel % (wire_width / wire_hbin) * wire_hbin;
			int y = wire_top + pixel / (wire_width / wire_hbin) * wire_vbin;
			if (icx453() && frame_bin == 1) {
				int phase = pixel % 4;
				x = wire_left / 2 + (pixel % wire_width) / 4 * 2 + (phase >= 2);
				y = wire_top * 2 + pixel / wire_width * 2 + (phase == 1 || phase == 2);
			} else if (icx453()) {
				x = wire_left / 2 + pixel % (wire_width / (2 * wire_hbin)) * wire_hbin;
				y = wire_top * 2 + pixel / (wire_width / (2 * wire_hbin)) * wire_vbin;
			} else if (interlaced()) {
				y *= 2;
			}
			put_word(data + 2 * i, atomic_load(&zero_pixels) ? 0 : ccd_test_noise(y * 64 + x, 0));
		}
		wire_offset += length;
	} else { atomic_fetch_add(&bad_protocol, 1); return LIBUSB_ERROR_PIPE; }
	*transferred = atomic_load(&short_reply) == last_request ? length - 1 : length;
	return 0;
}

int LIBUSB_CALL sx_test_bulk(libusb_device_handle *handle, unsigned char endpoint, unsigned char *data, int length, int *transferred, unsigned timeout) {
	int index = (int *)handle - usb_tokens;
	if (atomic_fetch_add(&active_usb[index], 1) || pthread_equal(test_thread, pthread_self())) { atomic_fetch_add(&concurrent_usb, 1); }
	int request = endpoint == 1 ? data[1] : last_request;
	if (atomic_load(&gate_request) == request && atomic_load(&gate_endpoint) == endpoint) {
		atomic_store(&gate_request, 0);
		atomic_store(&gate_entered, 1);
		int i = 0;
		for (; i < 5000 && !atomic_load(&gate_release); i++) { indigo_usleep(1000); }
		if (i == 5000) { atomic_fetch_add(&gate_timeout, 1); }
	}
	int result = sx_bulk_impl(handle, endpoint, data, length, transferred, timeout);
	atomic_fetch_sub(&active_usb[index], 1);
	return result;
}

void sx_test_execute_priority(indigo_device *device, int priority, double delay, indigo_timer_callback callback) {
	indigo_execute_priority_handler_in(device, priority, atomic_load(&fast_exposure) && !strcmp(device->name, camera_name) && delay > 0 ? 0.01 : delay, callback);
}

void sx_test_execute_in(indigo_device *device, double delay, indigo_timer_callback callback) {
	if (delay == 5) { atomic_store(&cooler_callback, callback); delay = 0.05; }
	indigo_execute_handler_in(device, delay, callback);
}

static indigo_result sx_update(indigo_client *client, indigo_device *device, indigo_property *property, const char *message) {
	if (!strcmp(property->name, "CONNECTION") && (!strcmp(property->device, camera_name) || !strcmp(property->device, guider_name))) {
		int index = !strcmp(property->device, guider_name);
		atomic_store(&connected[index], property->state == INDIGO_BUSY_STATE ? 2 : property->state == INDIGO_ALERT_STATE ? -1 : property->items[0].sw.value ? 1 : 0);
		atomic_fetch_add(&connection_revision[index], 1);
	}
	if (!strcmp(property->device, camera_name)) {
		camera_device = device;
		if (!strcmp(property->name, "CCD_IMAGE") && property->state == INDIGO_OK_STATE && property->count && property->items[0].blob.size) {
			indigo_raw_header h = { 0 };
			indigo_item *item = property->items;
			bool valid = item->blob.value && !strcmp(item->blob.format, ".raw") && item->blob.size >= sizeof(h);
			if (valid) {
				memcpy(&h, item->blob.value, sizeof(h));
				valid = h.signature == INDIGO_RAW_MONO16 && h.width == frame_width / frame_bin && h.height == frame_height / frame_bin && item->blob.size >= sizeof(h) + h.width * h.height * 2;
				for (unsigned y = 0; valid && y < h.height; y++) {
					for (unsigned x = 0; x < h.width; x++) {
						unsigned sy = frame_top + (interlaced() && frame_bin == 1 ? y / 2 * 2 : y * frame_bin);
						uint16_t value;
						memcpy(&value, (char *)item->blob.value + sizeof(h) + (y * h.width + x) * 2, 2);
						if (value != (atomic_load(&zero_pixels) ? 0 : ccd_test_noise(sy * 64 + frame_left + x * frame_bin, 0))) { valid = false; break; }
					}
				}
			}
			if (!valid) { atomic_fetch_add(&bad_frames, 1); }
			atomic_fetch_add(&frames, 1);
		}
	}
	if (!strcmp(property->device, guider_name)) {
		if ((!strcmp(property->name, "GUIDER_GUIDE_RA") || !strcmp(property->name, "GUIDER_GUIDE_DEC")) && property->state == INDIGO_OK_STATE) {
			for (int i = 0; i < property->count; i++) {
				if (property->items[i].number.value != 0) { atomic_fetch_add(&bad_guide_completion, 1); }
			}
		}
		if (!strcmp(property->name, "GUIDER_GUIDE_RA")) { atomic_store(&guide_state[0], property->state); }
		if (!strcmp(property->name, "GUIDER_GUIDE_DEC")) { atomic_store(&guide_state[1], property->state); }
	}
	return simulator_client_update_property(client, device, property, message);
}

static bool sx_wait(atomic_int *value, int expected) {
	for (int i = 0; i < 600; i++) { if (atomic_load(value) == expected) { return true; } indigo_usleep(10000); }
	fprintf(stderr, "SX counter expected %d got %d\n", expected, atomic_load(value));
	return false;
}
static bool sx_switch(const char *property, const char *item, indigo_property_state state) {
	return indigo_change_switch_property_1(&simulator_test_client, camera_name, property, item, true) == INDIGO_OK && wait_for_property_state(property, state);
}
static bool sx_expose(double duration, indigo_property_state state) {
	int before = atomic_load(&frames);
	if (indigo_change_number_property_1(&simulator_test_client, camera_name, "CCD_EXPOSURE", "EXPOSURE", duration) != INDIGO_OK || !wait_for_property_state("CCD_EXPOSURE", state)) { return false; }
	return state != INDIGO_OK_STATE || (atomic_load(&frames) == before + 1 && !atomic_load(&bad_frames));
}
static void sx_start(void) {
	atomic_store(&connected[0], 0); atomic_store(&connected[1], 0);
	atomic_store(&frames, 0); atomic_store(&bad_frames, 0);
	frame_left = frame_top = 0; frame_width = 64; frame_height = 48; frame_bin = 1;
	simulator_test_client.update_property = sx_update;
	reset_simulator_context(&sx_case);
	indigo_start();
	indigo_attach_client(&simulator_test_client);
	indigo_ccd_sx(INDIGO_DRIVER_INIT, NULL);
	for (int i = 0; i < 600 && !find_cached_property("CONNECTION"); i++) { indigo_usleep(10000); }
	ASSERT_TRUE(find_cached_property("CONNECTION") != NULL);
}

static void sx_begin(void) {
	sx_start();
	ASSERT_TRUE(sx_switch("CONNECTION", "CONNECTED", INDIGO_OK_STATE));
	ASSERT_TRUE(sx_wait(&connected[0], 1));
	sx_switch("CCD_IMAGE_FORMAT", "RAW", INDIGO_OK_STATE);
	sx_switch("CCD_UPLOAD_MODE", "CLIENT", INDIGO_OK_STATE);
}
static void sx_end(void) {
	atomic_store(&short_cooler_reply, 0); atomic_store(&sensor_temperature, 2855);
	atomic_store(&fail_open, 0); atomic_store(&fail_claim, 0); atomic_store(&fail_command, 0); atomic_store(&fail_read, 0); atomic_store(&short_read, 0);
	if (atomic_load(&connected[1]) > 0) {
		indigo_change_switch_property_1(&simulator_test_client, guider_name, "CONNECTION", "DISCONNECTED", true);
		sx_wait(&connected[1], 0);
	}
	if (context.connected) {
		stop_connected_simulator(&sx_case);
	} else {
		indigo_ccd_sx(INDIGO_DRIVER_SHUTDOWN, NULL);
		indigo_detach_client(&simulator_test_client);
		indigo_stop();
	}
	ASSERT_EQ_INT(atomic_load(&opened), atomic_load(&closed));
	ASSERT_EQ_INT(atomic_load(&config_allocated), atomic_load(&config_freed));
	ASSERT_EQ_INT(0, atomic_load(&refs));
	ASSERT_EQ_INT(0, atomic_load(&held));
	ASSERT_EQ_INT(0, atomic_load(&after_close));
	ASSERT_EQ_INT(0, atomic_load(&concurrent_usb));
	ASSERT_EQ_INT(0, atomic_load(&gate_timeout));
	ASSERT_EQ_INT(0, atomic_load(&bad_guide_completion));
	ASSERT_EQ_INT(0, atomic_load(&bad_protocol));
	simulator_test_client.update_property = simulator_client_update_property;
}

static void progressive_roi_bin_shutter_and_abort(void) {
	model = 0x25; caps = 0x31;
	sx_begin();
	SX_CHECK(sx_expose(0.02, INDIGO_OK_STATE));
	SX_CHECK(milliseconds == 20 && shutter == 0x40);
	SX_CHECK(sx_switch("CCD_FRAME_TYPE", "DARK", INDIGO_OK_STATE));
	for (int bin = 1; bin <= 4; bin *= 2) {
		frame_left = 8; frame_top = 8; frame_width = 32; frame_height = 32; frame_bin = bin;
		SX_CHECK(indigo_change_number_property(&simulator_test_client, camera_name, "CCD_BIN", 2, (const char *[]){ "HORIZONTAL", "VERTICAL" }, (double []){ bin, bin }) == INDIGO_OK);
		SX_CHECK(wait_for_property_state("CCD_BIN", INDIGO_OK_STATE));
		SX_CHECK(indigo_change_number_property(&simulator_test_client, camera_name, "CCD_FRAME", 4, (const char *[]){ "LEFT", "TOP", "WIDTH", "HEIGHT" }, (double []){ 8, 8, 32, 32 }) == INDIGO_OK);
		SX_CHECK(wait_for_property_state("CCD_FRAME", INDIGO_OK_STATE));
		SX_CHECK(sx_expose(0.02, INDIGO_OK_STATE));
		SX_CHECK(shutter == 0x80 && wire_hbin == bin && wire_vbin == bin);
	}
	SX_CHECK(sx_expose(1.01, INDIGO_OK_STATE));
	SX_CHECK(atomic_load(&command_count[1]) > 0 && atomic_load(&command_count[3]) > 0);
	SX_CHECK(sx_expose(4, INDIGO_BUSY_STATE));
	SX_CHECK(sx_switch("CCD_ABORT_EXPOSURE", "ABORT_EXPOSURE", INDIGO_OK_STATE));
	SX_CHECK(sx_expose(0.02, INDIGO_OK_STATE));
cleanup:
	sx_end();
}

static void readout_variants_and_short_transfers(void) {
	const int models[] = { 0x40, 0x59 };
	for (int i = 0; i < 2; i++) {
		model = models[i]; caps = 1;
		sx_begin();
		atomic_store(&short_read, 1);
		SX_CHECK(sx_expose(0.02, INDIGO_OK_STATE));
		SX_CHECK(sx_expose(1.01, INDIGO_OK_STATE));
		for (int bin = 2; bin <= 4; bin *= 2) {
			char mode[16];
			snprintf(mode, sizeof(mode), "BIN_%dx%d", bin, bin);
			SX_CHECK(sx_switch("CCD_MODE", mode, INDIGO_OK_STATE));
			frame_bin = bin;
			SX_CHECK(sx_expose(0.02, INDIGO_OK_STATE));
			SX_CHECK(sx_expose(1.01, INDIGO_OK_STATE));
		}
		sx_end();
	}
	return;
cleanup:
	sx_end();
}

static void cooling_failures_and_guider_sharing(void) {
	model = 0x25; caps = 0x31;
	sx_begin();
	SX_CHECK(indigo_change_number_property_1(&simulator_test_client, camera_name, "CCD_TEMPERATURE", "TEMPERATURE", -10) == INDIGO_OK);
	SX_CHECK(sx_wait(&cooler_target, 2630));
	SX_CHECK(atomic_load(&cooler_on) == 1);
	SX_CHECK(wait_for_number_item_value("CCD_TEMPERATURE", "TEMPERATURE", 12.5, 0.01));
	for (int phase = 0; phase < 2; phase++) {
		atomic_store(phase ? &fail_read : &fail_command, 30);
		SX_CHECK(wait_for_property_state("CCD_TEMPERATURE", INDIGO_ALERT_STATE));
		atomic_store(&fail_read, 0); atomic_store(&fail_command, 0);
		SX_CHECK(wait_for_property_state("CCD_TEMPERATURE", INDIGO_BUSY_STATE));
	}
	atomic_store(&short_cooler_reply, 1);
	SX_CHECK(wait_for_property_state("CCD_TEMPERATURE", INDIGO_ALERT_STATE));
	atomic_store(&short_cooler_reply, 0);
	atomic_store(&sensor_temperature, 2630);
	SX_CHECK(wait_for_number_item_value("CCD_TEMPERATURE", "TEMPERATURE", -10, 0.01));
	SX_CHECK(wait_for_property_state("CCD_TEMPERATURE", INDIGO_OK_STATE));
	SX_CHECK(sx_switch("CCD_COOLER", "OFF", INDIGO_OK_STATE));
	SX_CHECK(sx_wait(&cooler_on, 0));
	SX_CHECK(indigo_change_switch_property_1(&simulator_test_client, guider_name, "CONNECTION", "CONNECTED", true) == INDIGO_OK);
	SX_CHECK(sx_expose(0.02, INDIGO_OK_STATE));
	const char *directions[] = { "EAST", "WEST", "NORTH", "SOUTH" };
	const int masks[] = { 8, 1, 4, 2 };
	for (int i = 0; i < 4; i++) {
		const char *property = i < 2 ? "GUIDER_GUIDE_RA" : "GUIDER_GUIDE_DEC";
		SX_CHECK(indigo_change_number_property_1(&simulator_test_client, guider_name, property, directions[i], 50) == INDIGO_OK);
		SX_CHECK(sx_wait(&relay_mask, masks[i]));
		SX_CHECK(sx_wait(&guide_state[i / 2], INDIGO_OK_STATE));
		SX_CHECK(atomic_load(&relay_mask) == 0);
	}
cleanup:
	sx_end();
}

static void uncooled_camera(void) {
	model = 0x25; caps = 1;
	sx_begin();
	SX_CHECK(find_cached_property("CCD_COOLER") == NULL);
	SX_CHECK(find_cached_property("CCD_TEMPERATURE") == NULL);
	SX_CHECK(find_cached_property("CCD_COOLER_POWER") == NULL);
	SX_CHECK(sx_expose(0.02, INDIGO_OK_STATE));
cleanup:
	sx_end();
}

static void transfer_failures_and_recovery(void) {
	model = 0x25; caps = 0x31;
	sx_begin();
	atomic_store(&fail_command, 2);
	SX_CHECK(sx_expose(0.02, INDIGO_ALERT_STATE));
	atomic_store(&fail_command, 0);
	atomic_store(&fail_read, 2);
	SX_CHECK(sx_expose(0.02, INDIGO_ALERT_STATE));
	atomic_store(&fail_read, 0);
	atomic_store(&zero_read, 1);
	SX_CHECK(sx_expose(0.02, INDIGO_ALERT_STATE));
	SX_CHECK(sx_expose(0.02, INDIGO_OK_STATE));
cleanup:
	sx_end();
}

static bool sx_connect(int index, bool on, int expected) {
	int before = atomic_load(&connection_revision[index]);
	if (indigo_change_switch_property_1(&simulator_test_client, index ? guider_name : camera_name, "CONNECTION", on ? "CONNECTED" : "DISCONNECTED", true) != INDIGO_OK) { return false; }
	for (int i = 0; i < 600; i++) {
		if (atomic_load(&connection_revision[index]) > before && atomic_load(&connected[index]) == expected) { return true; }
		indigo_usleep(10000);
	}
	return false;
}

static void initialization_failures_and_retry(void) {
	model = 0x25; caps = 0x31;
	sx_start();
	atomic_int *errors[] = { &fail_lock, &fail_open, &fail_config, &fail_claim, &fail_command, &fail_command, &fail_command, &fail_read, &fail_read, &short_command, &short_reply, &short_reply };
	int values[] = { 1, 1, 1, 1, 6, 14, 8, 14, 8, 6, 14, 8 };
	for (int i = 0; i < ARRAY_SIZE(errors); i++) {
		atomic_store(errors[i], values[i]);
		SX_CHECK(sx_connect(0, true, -1));
		SX_CHECK(!atomic_load(&is_open) && !atomic_load(&held));
		SX_CHECK(atomic_load(&opened) == atomic_load(&closed));
		SX_CHECK(atomic_load(&config_allocated) == atomic_load(&config_freed));
		atomic_store(errors[i], 0);
		SX_CHECK(sx_connect(0, true, 1));
		SX_CHECK(sx_connect(0, false, 0));
	}
cleanup:
	atomic_store(&fail_lock, 0); atomic_store(&fail_config, 0); atomic_store(&short_command, 0); atomic_store(&short_reply, 0);
	sx_end();
}

static void shared_lifecycle_and_shutdown(void) {
	model = 0x25; caps = 0x31;
	for (int first = 0; first < 2; first++) {
		for (int close_first = 0; close_first < 2; close_first++) {
			sx_start();
			int before = atomic_load(&opened);
			SX_CHECK(indigo_ccd_sx(INDIGO_DRIVER_INIT, NULL) == INDIGO_OK);
			SX_CHECK(sx_connect(first, true, 1));
			SX_CHECK(sx_connect(1 - first, true, 1));
			SX_CHECK(atomic_load(&opened) == before + 1);
			SX_CHECK(indigo_ccd_sx(INDIGO_DRIVER_SHUTDOWN, NULL) == INDIGO_BUSY);
			SX_CHECK(sx_switch("CCD_IMAGE_FORMAT", "RAW", INDIGO_OK_STATE));
			SX_CHECK(sx_expose(0.02, INDIGO_OK_STATE));
			SX_CHECK(sx_connect(close_first, false, 0));
			SX_CHECK(atomic_load(&is_open));
			SX_CHECK(sx_connect(1 - close_first, false, 0));
			SX_CHECK(!atomic_load(&is_open));
			sx_end();
			SX_CHECK(indigo_ccd_sx(INDIGO_DRIVER_SHUTDOWN, NULL) == INDIGO_OK);
		}
	}
	return;
cleanup:
	sx_end();
}

static bool sx_pulse(const char *property, const char *direction, double duration) {
	return indigo_change_number_property_1(&simulator_test_client, guider_name, property, direction, duration) == INDIGO_OK;
}

static void guider_replacement_errors_and_disconnect(void) {
	model = 0x25; caps = 0x31;
	sx_begin();
	SX_CHECK(sx_connect(1, true, 1));
	SX_CHECK(sx_pulse("GUIDER_GUIDE_RA", "EAST", 300));
	SX_CHECK(sx_wait(&relay_mask, 8));
	SX_CHECK(sx_pulse("GUIDER_GUIDE_DEC", "NORTH", 80));
	SX_CHECK(sx_wait(&relay_mask, 12));
	SX_CHECK(sx_wait(&guide_state[1], INDIGO_OK_STATE));
	SX_CHECK(atomic_load(&relay_mask) == 8);
	SX_CHECK(sx_pulse("GUIDER_GUIDE_RA", "WEST", 150));
	SX_CHECK(sx_wait(&relay_mask, 1));
	SX_CHECK(sx_wait(&guide_state[0], INDIGO_OK_STATE));
	SX_CHECK(atomic_load(&relay_mask) == 0);
	SX_CHECK(sx_pulse("GUIDER_GUIDE_RA", "EAST", 500));
	SX_CHECK(sx_wait(&relay_mask, 8));
	SX_CHECK(sx_pulse("GUIDER_GUIDE_RA", "EAST", 0));
	SX_CHECK(sx_wait(&relay_mask, 0));
	for (int axis = 0; axis < 2; axis++) {
		const char *property = axis ? "GUIDER_GUIDE_DEC" : "GUIDER_GUIDE_RA";
		const char *direction = axis ? "SOUTH" : "WEST";
		atomic_store(&fail_command, 9);
		SX_CHECK(sx_pulse(property, direction, 40));
		SX_CHECK(sx_wait(&guide_state[axis], INDIGO_ALERT_STATE));
		atomic_store(&fail_command, 0);
		SX_CHECK(sx_pulse(property, direction, 80));
		SX_CHECK(sx_wait(&relay_mask, axis ? 2 : 1));
		atomic_store(&fail_command, 9);
		SX_CHECK(sx_wait(&guide_state[axis], INDIGO_ALERT_STATE));
		atomic_store(&fail_command, 0);
		SX_CHECK(sx_pulse(property, direction, 30));
		SX_CHECK(sx_wait(&guide_state[axis], INDIGO_OK_STATE));
	}
	SX_CHECK(sx_pulse("GUIDER_GUIDE_RA", "EAST", 500));
	SX_CHECK(sx_wait(&relay_mask, 8));
	SX_CHECK(sx_connect(1, false, 0));
	SX_CHECK(atomic_load(&relay_mask) == 0 && atomic_load(&is_open));
	SX_CHECK(sx_expose(0.02, INDIGO_OK_STATE));
	SX_CHECK(sx_connect(1, true, 1));
	SX_CHECK(sx_pulse("GUIDER_GUIDE_DEC", "NORTH", 30));
	SX_CHECK(sx_wait(&guide_state[1], INDIGO_OK_STATE));
cleanup:
	sx_end();
}

static void flood_led_and_property_contract(void) {
	model = 0x25; caps = 0x31;
	sx_begin();
	indigo_driver_info info;
	SX_CHECK(indigo_ccd_sx(INDIGO_DRIVER_INFO, &info) == INDIGO_OK);
	SX_CHECK(!strcmp(info.name, "indigo_ccd_sx") && info.version == 0x0300000f);
	assert_device_interface(INDIGO_INTERFACE_CCD);
	indigo_property *p = find_cached_property("X_CCD_FLOOD_LED");
	SX_CHECK(p && p->type == INDIGO_SWITCH_VECTOR && p->count == 2 && p->perm == INDIGO_RW_PERM);
	SX_CHECK(!strcmp(p->items[0].name, "ON") && !strcmp(p->items[1].name, "OFF"));
	SX_CHECK(find_cached_property("CCD_STREAMING") == NULL);
	SX_CHECK(sx_switch("X_CCD_FLOOD_LED", "ON", INDIGO_OK_STATE));
	SX_CHECK(atomic_load(&led_on) == 1);
	atomic_store(&fail_command, 43);
	SX_CHECK(sx_switch("X_CCD_FLOOD_LED", "OFF", INDIGO_ALERT_STATE));
	atomic_store(&fail_command, 0);
	SX_CHECK(sx_switch("X_CCD_FLOOD_LED", "OFF", INDIGO_OK_STATE));
	SX_CHECK(atomic_load(&led_on) == 0);
	SX_CHECK(sx_switch("X_CCD_FLOOD_LED", "ON", INDIGO_OK_STATE));
	SX_CHECK(sx_connect(0, false, 0));
	SX_CHECK(atomic_load(&led_on) == 0);
cleanup:
	sx_end();
}

static atomic_int discovery_barrier;

static void discovery_done(indigo_device *device) {
	atomic_fetch_add(&discovery_barrier, 1);
}

static bool sx_discovery_sync(void) {
	int before = atomic_load(&discovery_barrier);
	indigo_queue_add(lifecycle_queue, NULL, INDIGO_TASK_PRIORITY_NORMAL, 0, discovery_done, NULL);
	return sx_wait(&discovery_barrier, before + 1);
}

static void attach_failures_and_capacity(void) {
	for (int failure = 1; failure <= 2; failure++) {
		reset_simulator_context(&sx_case);
		indigo_start();
		atomic_store(&fail_attach, failure);
		SX_CHECK(indigo_ccd_sx(INDIGO_DRIVER_INIT, NULL) == INDIGO_OK);
		SX_CHECK(sx_discovery_sync());
		SX_CHECK(atomic_load(&attached_devices) == (failure == 1 ? 0 : 1));
		atomic_store(&fail_attach, 0);
		usb_callback(NULL, (libusb_device *)&usb_token, LIBUSB_HOTPLUG_EVENT_DEVICE_LEFT, NULL);
		SX_CHECK(sx_discovery_sync());
		usb_callback(NULL, (libusb_device *)&usb_token, LIBUSB_HOTPLUG_EVENT_DEVICE_ARRIVED, NULL);
		SX_CHECK(sx_discovery_sync());
		SX_CHECK(atomic_load(&attached_devices) == 2);
		SX_CHECK(indigo_ccd_sx(INDIGO_DRIVER_SHUTDOWN, NULL) == INDIGO_OK);
		indigo_stop();
		SX_CHECK(atomic_load(&refs) == 0);
	}
	indigo_start();
	SX_CHECK(indigo_ccd_sx(INDIGO_DRIVER_INIT, NULL) == INDIGO_OK);
	SX_CHECK(sx_discovery_sync());
	for (int i = 1; i < 4; i++) { usb_callback(NULL, (libusb_device *)(usb_tokens + i), LIBUSB_HOTPLUG_EVENT_DEVICE_ARRIVED, NULL); }
	SX_CHECK(sx_discovery_sync());
	SX_CHECK(atomic_load(&attached_devices) == 5);
	SX_CHECK(atomic_load(&refs) == 3);
	usb_callback(NULL, (libusb_device *)usb_tokens, LIBUSB_HOTPLUG_EVENT_DEVICE_LEFT, NULL);
	SX_CHECK(sx_discovery_sync());
	SX_CHECK(atomic_load(&attached_devices) == 3);
	usb_callback(NULL, (libusb_device *)(usb_tokens + 3), LIBUSB_HOTPLUG_EVENT_DEVICE_ARRIVED, NULL);
	SX_CHECK(sx_discovery_sync());
	SX_CHECK(atomic_load(&attached_devices) == 5);
cleanup:
	atomic_store(&fail_attach, 0);
	indigo_ccd_sx(INDIGO_DRIVER_SHUTDOWN, NULL);
	indigo_stop();
	ASSERT_EQ_INT(atomic_load(&refs), 0);
	ASSERT_EQ_INT(atomic_load(&attached_devices), 0);
}

static void queue_failure_and_identity_profiles(void) {
	reset_simulator_context(&sx_case);
	indigo_start();
	atomic_store(&fail_queue, 1);
	SX_CHECK(indigo_ccd_sx(INDIGO_DRIVER_INIT, NULL) == INDIGO_FAILED);
	atomic_store(&fail_queue, 0);
	SX_CHECK(indigo_ccd_sx(INDIGO_DRIVER_INIT, NULL) == INDIGO_OK);
	SX_CHECK(sx_discovery_sync());
	SX_CHECK(atomic_load(&attached_devices) == 2);
	SX_CHECK(indigo_ccd_sx(INDIGO_DRIVER_SHUTDOWN, NULL) == INDIGO_OK);
	for (int phase = 0; phase < 2; phase++) {
		atomic_store(&fail_descriptor, phase == 0);
		product = phase ? 0xffff : 0x0525;
		SX_CHECK(indigo_ccd_sx(INDIGO_DRIVER_INIT, NULL) == INDIGO_OK);
		SX_CHECK(sx_discovery_sync());
		SX_CHECK(atomic_load(&attached_devices) == 0 && atomic_load(&refs) == 0);
		SX_CHECK(indigo_ccd_sx(INDIGO_DRIVER_SHUTDOWN, NULL) == INDIGO_OK);
	}
cleanup:
	product = 0x0525;
	atomic_store(&fail_descriptor, 0); atomic_store(&fail_queue, 0);
	indigo_ccd_sx(INDIGO_DRIVER_SHUTDOWN, NULL);
	indigo_stop();
}

static void multiple_camera_survivor_and_active_removal(void) {
	model = 0x25; caps = 1;
	sx_begin();
	usb_callback(NULL, (libusb_device *)(usb_tokens + 1), LIBUSB_HOTPLUG_EVENT_DEVICE_ARRIVED, NULL);
	SX_CHECK(sx_discovery_sync());
	SX_CHECK(atomic_load(&attached_devices) == 4);
	SX_CHECK(indigo_change_switch_property_1(&simulator_test_client, "SX UltraStar #fake-1", "CONNECTION", "CONNECTED", true) == INDIGO_OK);
	SX_CHECK(sx_wait(&physical_open[1], 1));
	usb_callback(NULL, (libusb_device *)(usb_tokens + 1), LIBUSB_HOTPLUG_EVENT_DEVICE_LEFT, NULL);
	SX_CHECK(sx_discovery_sync());
	SX_CHECK(atomic_load(&physical_open[0]) && !atomic_load(&physical_open[1]));
	SX_CHECK(sx_expose(0.02, INDIGO_OK_STATE));
	for (int phase = 0; phase < 2; phase++) {
		if (phase) {
			SX_CHECK(sx_connect(0, true, 1));
			SX_CHECK(sx_switch("CCD_IMAGE_FORMAT", "RAW", INDIGO_OK_STATE));
		}
		SX_CHECK(indigo_change_number_property_1(&simulator_test_client, camera_name, "CCD_EXPOSURE", "EXPOSURE", 2) == INDIGO_OK);
		SX_CHECK(wait_for_property_state("CCD_EXPOSURE", INDIGO_BUSY_STATE));
		usb_callback(NULL, (libusb_device *)usb_tokens, LIBUSB_HOTPLUG_EVENT_DEVICE_LEFT, NULL);
		SX_CHECK(sx_discovery_sync());
		SX_CHECK(!atomic_load(&is_open) && atomic_load(&attached_devices) == 0);
		usb_callback(NULL, (libusb_device *)usb_tokens, LIBUSB_HOTPLUG_EVENT_DEVICE_ARRIVED, NULL);
		SX_CHECK(sx_discovery_sync());
	}
	SX_CHECK(sx_connect(0, true, 1));
	SX_CHECK(sx_switch("CCD_IMAGE_FORMAT", "RAW", INDIGO_OK_STATE));
	SX_CHECK(sx_expose(0.02, INDIGO_OK_STATE));
cleanup:
	sx_end();
}

static void sx_release_gate(void) {
	atomic_store(&gate_release, 1);
}

static void sx_arm_gate(int request, int endpoint) {
	atomic_store(&gate_entered, 0);
	atomic_store(&gate_release, 0);
	atomic_store(&gate_request, request);
	atomic_store(&gate_endpoint, endpoint);
}

static void acquisition_gates_and_abort_orders(void) {
	model = 0x25; caps = 0x31;
	sx_begin();
	for (int endpoint = 1; endpoint <= 2; endpoint++) {
		int before = atomic_load(&frames);
		sx_arm_gate(2, endpoint == 1 ? 1 : 0x82);
		SX_CHECK(indigo_change_number_property_1(&simulator_test_client, camera_name, "CCD_EXPOSURE", "EXPOSURE", 0.1) == INDIGO_OK);
		SX_CHECK(sx_wait(&gate_entered, 1));
		SX_CHECK(indigo_change_switch_property_1(&simulator_test_client, camera_name, "CCD_ABORT_EXPOSURE", "ABORT_EXPOSURE", true) == INDIGO_OK);
		sx_release_gate();
		SX_CHECK(wait_for_property_state("CCD_ABORT_EXPOSURE", endpoint == 2 ? INDIGO_ALERT_STATE : INDIGO_OK_STATE));
		SX_CHECK(atomic_load(&frames) == before + (endpoint == 2));
		SX_CHECK(sx_expose(0.02, INDIGO_OK_STATE));
	}
	for (int removal = 0; removal < 2; removal++) {
		sx_arm_gate(2, 0x82);
		SX_CHECK(indigo_change_number_property_1(&simulator_test_client, camera_name, "CCD_EXPOSURE", "EXPOSURE", 0.02) == INDIGO_OK);
		SX_CHECK(sx_wait(&gate_entered, 1));
		if (removal) {
			usb_callback(NULL, (libusb_device *)usb_tokens, LIBUSB_HOTPLUG_EVENT_DEVICE_LEFT, NULL);
		} else {
			SX_CHECK(indigo_change_switch_property_1(&simulator_test_client, camera_name, "CONNECTION", "DISCONNECTED", true) == INDIGO_OK);
		}
		sx_release_gate();
		if (removal) {
			SX_CHECK(sx_discovery_sync());
			SX_CHECK(atomic_load(&attached_devices) == 0);
			usb_callback(NULL, (libusb_device *)usb_tokens, LIBUSB_HOTPLUG_EVENT_DEVICE_ARRIVED, NULL);
			SX_CHECK(sx_discovery_sync());
		} else { SX_CHECK(sx_wait(&connected[0], 0)); }
		SX_CHECK(sx_connect(0, true, 1));
		SX_CHECK(sx_switch("CCD_IMAGE_FORMAT", "RAW", INDIGO_OK_STATE));
		SX_CHECK(sx_expose(0.02, INDIGO_OK_STATE));
	}
cleanup:
	sx_release_gate();
	sx_end();
}

static int compare_samples(const void *left, const void *right) {
	double a = *(const double *)left, b = *(const double *)right;
	return (a > b) - (a < b);
}

static void guider_usb_timing(void) {
	model = 0x25; caps = 0x31;
	sx_begin();
	SX_CHECK(sx_connect(1, true, 1));
	const char *directions[] = { "WEST", "SOUTH", "NORTH", "EAST" };
	const int durations[] = { 20, 50, 100, 250, 500 };
	double errors[80], absolute_errors[80], total = 0, square_total = 0;
	int count = 0;
	for (int workload = 0; workload < 2; workload++) {
		if (workload) {
			SX_CHECK(indigo_change_number_property_1(&simulator_test_client, camera_name, "CCD_EXPOSURE", "EXPOSURE", 30) == INDIGO_OK);
			SX_CHECK(wait_for_property_state("CCD_EXPOSURE", INDIGO_BUSY_STATE));
		}
		for (int direction = 0; direction < 4; direction++) {
			const char *property = direction == 0 || direction == 3 ? "GUIDER_GUIDE_RA" : "GUIDER_GUIDE_DEC";
			for (int sample = -1; sample < 10; sample++) {
				int requested = sample < 0 ? 20 : durations[sample % 5];
				int before = atomic_load(&pulse_count[direction]);
				SX_CHECK(sx_pulse(property, directions[direction], requested));
				SX_CHECK(sx_wait(&pulse_count[direction], before + 1));
				SX_CHECK(sx_wait(&guide_state[direction == 0 || direction == 3 ? 0 : 1], INDIGO_OK_STATE));
				if (sample < 0) { continue; }
				double actual = atomic_load(&pulse_duration_ns[direction]) / 1000000.0;
				double error = actual - requested;
				SX_CHECK(isfinite(actual) && actual > 0);
				printf("    USB edges %s %s requested=%d ms actual=%.3f ms error=%+.3f ms (%+.2f%%)\n", workload ? "exposing" : "idle", directions[direction], requested, actual, error, error / requested * 100);
				errors[count] = error; absolute_errors[count++] = fabs(error);
				total += error; square_total += error * error;
			}
		}
		if (workload) { SX_CHECK(sx_switch("CCD_ABORT_EXPOSURE", "ABORT_EXPOSURE", INDIGO_OK_STATE)); }
	}
	qsort(errors, count, sizeof(double), compare_samples);
	qsort(absolute_errors, count, sizeof(double), compare_samples);
	double mean = total / count;
	printf("    ON-to-OFF USB entry error n=%d min=%+.3f mean=%+.3f median=%+.3f p95=%+.3f p99=%+.3f max=%+.3f stddev=%.3f max_abs=%.3f ms\n", count, errors[0], mean, (errors[count / 2 - 1] + errors[count / 2]) / 2, errors[(int)ceil(count * 0.95) - 1], errors[(int)ceil(count * 0.99) - 1], errors[count - 1], sqrt(fmax(0, square_total / count - mean * mean)), absolute_errors[count - 1]);
cleanup:
	sx_end();
}

static void frame_types_and_unit_conversion(void) {
	model = 0x25; caps = 0x31;
	sx_begin();
	const char *types[] = { "LIGHT", "FLAT", "DARK", "DARKFLAT", "BIAS" };
	for (int i = 0; i < ARRAY_SIZE(types); i++) {
		SX_CHECK(sx_switch("CCD_FRAME_TYPE", types[i], INDIGO_OK_STATE));
		SX_CHECK(sx_expose(0.025, INDIGO_OK_STATE));
		SX_CHECK(shutter == (i < 2 ? 0x40 : 0x80));
		SX_CHECK(milliseconds == (i == 4 ? 0 : 25));
	}
	SX_CHECK(sx_switch("CCD_FRAME_TYPE", "LIGHT", INDIGO_OK_STATE));
	int before = atomic_load(&command_count[2]);
	SX_CHECK(indigo_change_number_property_1(&simulator_test_client, camera_name, "CCD_EXPOSURE", "EXPOSURE", 0.3) == INDIGO_OK);
	SX_CHECK(wait_for_property_state("CCD_EXPOSURE", INDIGO_BUSY_STATE));
	SX_CHECK(indigo_change_number_property_1(&simulator_test_client, camera_name, "CCD_EXPOSURE", "EXPOSURE", 0.01) == INDIGO_OK);
	SX_CHECK(wait_for_property_state("CCD_EXPOSURE", INDIGO_OK_STATE));
	SX_CHECK(atomic_load(&command_count[2]) == before + 1 && milliseconds == 300);
	SX_CHECK(sx_expose(0.02, INDIGO_OK_STATE));
cleanup:
	sx_end();
}

static void geometry_validation_and_modes(void) {
	model = 0x25; caps = 0x31;
	sx_begin();
	const double invalid[][2] = { { 3, 3 }, { 0, 0 }, { 1, 2 }, { 5, 5 } };
	for (int i = 0; i < ARRAY_SIZE(invalid); i++) {
		SX_CHECK(indigo_change_number_property(&simulator_test_client, camera_name, "CCD_BIN", 2, (const char *[]){ "HORIZONTAL", "VERTICAL" }, invalid[i]) == INDIGO_OK);
		SX_CHECK(wait_for_property_state("CCD_BIN", INDIGO_ALERT_STATE));
		SX_CHECK(cached_number_value("CCD_BIN", "HORIZONTAL") == 1 && cached_number_value("CCD_BIN", "VERTICAL") == 1);
	}
	for (int bin = 1; bin <= 4; bin *= 2) {
		char mode[16];
		snprintf(mode, sizeof(mode), "BIN_%dx%d", bin, bin);
		SX_CHECK(sx_switch("CCD_MODE", mode, INDIGO_OK_STATE));
		SX_CHECK(cached_number_value("CCD_BIN", "HORIZONTAL") == bin && cached_number_value("CCD_BIN", "VERTICAL") == bin);
		frame_bin = bin;
		SX_CHECK(sx_expose(0.02, INDIGO_OK_STATE));
	}
	SX_CHECK(sx_switch("CCD_MODE", "BIN_1x1", INDIGO_OK_STATE));
	frame_bin = 1;
	SX_CHECK(indigo_change_number_property(&simulator_test_client, camera_name, "CCD_FRAME", 4, (const char *[]){ "LEFT", "TOP", "WIDTH", "HEIGHT" }, (double []){ 60, 44, 16, 16 }) == INDIGO_OK);
	SX_CHECK(wait_for_property_state("CCD_FRAME", INDIGO_ALERT_STATE));
	SX_CHECK(cached_number_value("CCD_FRAME", "WIDTH") == 4 && cached_number_value("CCD_FRAME", "HEIGHT") == 4);
	frame_left = 60; frame_top = 44; frame_width = frame_height = 4;
	SX_CHECK(sx_expose(0.02, INDIGO_OK_STATE));
cleanup:
	sx_end();
}

static void readout_command_failures_and_short_writes(void) {
	model = 0x25; caps = 0x31;
	sx_begin();
	atomic_store(&short_command, 2);
	SX_CHECK(sx_expose(0.02, INDIGO_ALERT_STATE));
	atomic_store(&short_command, 0);
	for (int phase = 0; phase < 2; phase++) {
		atomic_store(phase ? &short_command : &fail_command, 3);
		SX_CHECK(sx_expose(1.01, INDIGO_ALERT_STATE));
		atomic_store(&short_command, 0); atomic_store(&fail_command, 0);
		SX_CHECK(sx_expose(0.02, INDIGO_OK_STATE));
	}
cleanup:
	atomic_store(&short_command, 0);
	sx_end();
}

static void slow_initialization_and_unsupported_guider(void) {
	model = 0x25; caps = 0x31;
	sx_start();
	int before = atomic_load(&cooler_reads);
	sx_arm_gate(8, 0x82);
	SX_CHECK(indigo_change_switch_property_1(&simulator_test_client, camera_name, "CONNECTION", "CONNECTED", true) == INDIGO_OK);
	SX_CHECK(sx_wait(&gate_entered, 1));
	indigo_usleep(120000);
	SX_CHECK(atomic_load(&cooler_reads) == before);
	sx_release_gate();
	SX_CHECK(sx_wait(&connected[0], 1));
	for (int i = 0; i < 600 && atomic_load(&cooler_reads) == before; i++) { indigo_usleep(1000); }
	SX_CHECK(atomic_load(&cooler_reads) > before);
	sx_end();
	caps = 0;
	sx_start();
	SX_CHECK(sx_connect(1, true, -1));
	SX_CHECK(!atomic_load(&is_open) && atomic_load(&held) == 0);
	SX_CHECK(sx_connect(0, true, 1));
	SX_CHECK(sx_switch("CCD_IMAGE_FORMAT", "RAW", INDIGO_OK_STATE));
	SX_CHECK(sx_expose(0.02, INDIGO_OK_STATE));
cleanup:
	sx_release_gate();
	sx_end();
}

static void *sx_shutdown_thread(void *unused) {
	atomic_store(&shutdown_started, 1);
	indigo_result result = indigo_ccd_sx(INDIGO_DRIVER_SHUTDOWN, NULL);
	atomic_store(&shutdown_done, result == INDIGO_OK ? 1 : -1);
	return NULL;
}

static void shutdown_with_discovery_pending(void) {
	pthread_t worker;
	bool started = false;
	reset_simulator_context(&sx_case);
	indigo_start();
	atomic_store(&block_attach, 1);
	atomic_store(&attach_entered, 0); atomic_store(&attach_release, 0); atomic_store(&deregistered, 0);
	SX_CHECK(indigo_ccd_sx(INDIGO_DRIVER_INIT, NULL) == INDIGO_OK);
	SX_CHECK(sx_wait(&attach_entered, 1));
	usb_callback(NULL, (libusb_device *)(usb_tokens + 1), LIBUSB_HOTPLUG_EVENT_DEVICE_ARRIVED, NULL);
	SX_CHECK(pthread_create(&worker, NULL, sx_shutdown_thread, NULL) == 0);
	started = true;
	SX_CHECK(sx_wait(&shutdown_started, 1));
	atomic_store(&attach_release, 1);
	pthread_join(worker, NULL); started = false;
	SX_CHECK(atomic_load(&shutdown_done) == 1);
	SX_CHECK(atomic_load(&attached_devices) == 0 && atomic_load(&refs) == 0);
cleanup:
	atomic_store(&attach_release, 1);
	if (started) { pthread_join(worker, NULL); }
	indigo_stop();
}

static void long_exposure_register_sequence(void) {
	model = 0x25; caps = 0x31;
	sx_begin();
	atomic_store(&fast_exposure, 1);
	int clears = atomic_load(&command_count[1]), reads = atomic_load(&command_count[3]);
	SX_CHECK(sx_expose(4, INDIGO_OK_STATE));
	SX_CHECK(atomic_load(&command_count[1]) == clears + 2);
	SX_CHECK(atomic_load(&command_count[3]) == reads + 1);
	atomic_store(&fail_clear_at, atomic_load(&command_count[1]) + 2);
	SX_CHECK(sx_expose(4, INDIGO_ALERT_STATE));
	atomic_store(&fail_clear_at, 0);
	SX_CHECK(sx_expose(0.02, INDIGO_OK_STATE));
cleanup:
	atomic_store(&fast_exposure, 0); atomic_store(&fail_clear_at, 0);
	sx_end();
}

static void queued_guiding_and_pulse_removal(void) {
	model = 0x25; caps = 0x31;
	sx_begin();
	SX_CHECK(sx_connect(1, true, 1));
	sx_arm_gate(2, 0x82);
	SX_CHECK(indigo_change_number_property_1(&simulator_test_client, camera_name, "CCD_EXPOSURE", "EXPOSURE", 0.02) == INDIGO_OK);
	SX_CHECK(sx_wait(&gate_entered, 1));
	int before = atomic_load(&command_count[9]);
	SX_CHECK(sx_pulse("GUIDER_GUIDE_RA", "EAST", 500));
	SX_CHECK(sx_pulse("GUIDER_GUIDE_RA", "WEST", 300));
	SX_CHECK(sx_pulse("GUIDER_GUIDE_RA", "EAST", 150));
	sx_release_gate();
	SX_CHECK(sx_wait(&relay_mask, 8));
	SX_CHECK(sx_wait(&guide_state[0], INDIGO_OK_STATE));
	SX_CHECK(atomic_load(&command_count[9]) == before + 2);
	SX_CHECK(sx_pulse("GUIDER_GUIDE_DEC", "SOUTH", 500));
	SX_CHECK(sx_wait(&relay_mask, 2));
	usb_callback(NULL, (libusb_device *)usb_tokens, LIBUSB_HOTPLUG_EVENT_DEVICE_LEFT, NULL);
	SX_CHECK(sx_discovery_sync());
	SX_CHECK(atomic_load(&relay_mask) == 0 && atomic_load(&attached_devices) == 0);
	usb_callback(NULL, (libusb_device *)usb_tokens, LIBUSB_HOTPLUG_EVENT_DEVICE_ARRIVED, NULL);
	SX_CHECK(sx_discovery_sync());
	SX_CHECK(sx_connect(1, true, 1));
	SX_CHECK(sx_pulse("GUIDER_GUIDE_RA", "WEST", 30));
	SX_CHECK(sx_wait(&guide_state[0], INDIGO_OK_STATE));
cleanup:
	sx_release_gate();
	sx_end();
}

static void interlaced_zero_signal(void) {
	model = 0x40; caps = 1;
	sx_begin();
	atomic_store(&zero_pixels, 1);
	SX_CHECK(sx_expose(0.02, INDIGO_OK_STATE));
	SX_CHECK(sx_expose(1.01, INDIGO_OK_STATE));
cleanup:
	atomic_store(&zero_pixels, 0);
	sx_end();
}

static void duplicate_arrival_regression(void) {
	sx_start();
	SX_CHECK(sx_wait(&attached_devices, 2));
	usb_callback(NULL, (libusb_device *)&usb_token, LIBUSB_HOTPLUG_EVENT_DEVICE_ARRIVED, NULL);
	indigo_usleep(100000);
	SX_CHECK(atomic_load(&attached_devices) == 2);
cleanup:
	sx_end();
}

static void registration_rollback(void) {
	reset_simulator_context(&sx_case);
	indigo_start();
	atomic_store(&fail_registration, 1);
	SX_CHECK(indigo_ccd_sx(INDIGO_DRIVER_INIT, NULL) == INDIGO_FAILED);
	atomic_store(&fail_registration, 0);
	SX_CHECK(indigo_ccd_sx(INDIGO_DRIVER_INIT, NULL) == INDIGO_OK);
	SX_CHECK(sx_wait(&attached_devices, 2));
cleanup:
	atomic_store(&fail_registration, 0);
	indigo_ccd_sx(INDIGO_DRIVER_SHUTDOWN, NULL);
	indigo_stop();
}

int main(int argc, char **argv) {
	test_thread = pthread_self();
	setvbuf(stdout, NULL, _IONBF, 0);
	const indigo_test_case tests[] = {
		{ "Long exposure register sequence", long_exposure_register_sequence },
		{ "Queued guiding and pulse removal", queued_guiding_and_pulse_removal },
		{ "Interlaced zero signal", interlaced_zero_signal },
		{ "Shutdown with discovery pending", shutdown_with_discovery_pending },
		{ "Frame types and exposure units", frame_types_and_unit_conversion },
		{ "Geometry validation and modes", geometry_validation_and_modes },
		{ "Readout command failures and short writes", readout_command_failures_and_short_writes },
		{ "Slow initialization and unsupported guider", slow_initialization_and_unsupported_guider },
		{ "Acquisition gates and abort orders", acquisition_gates_and_abort_orders },
		{ "Guider USB pulse timing", guider_usb_timing },
		{ "Attach failures and capacity", attach_failures_and_capacity },
		{ "Queue failure and identity profiles", queue_failure_and_identity_profiles },
		{ "Multiple camera survivor and active removal", multiple_camera_survivor_and_active_removal },
		{ "Initialization failures and retry", initialization_failures_and_retry },
		{ "Shared lifecycle and rejected shutdown", shared_lifecycle_and_shutdown },
		{ "Guider replacement errors and disconnect", guider_replacement_errors_and_disconnect },
		{ "Flood LED and property contract", flood_led_and_property_contract },
		{ "Duplicate arrival regression", duplicate_arrival_regression },
		{ "Registration rollback", registration_rollback },
		{ "Progressive ROI bin shutter and abort", progressive_roi_bin_shutter_and_abort },
		{ "Interlaced ICX453 and short USB transfers", readout_variants_and_short_transfers },
		{ "Uncooled camera", uncooled_camera },
		{ "Cooling failures and guider sharing", cooling_failures_and_guider_sharing },
		{ "USB transfer errors and reacquisition", transfer_failures_and_recovery }
	};
	int result = 0, matched = 0;
	for (int i = 0; i < ARRAY_SIZE(tests); i++) {
		if (argc < 2 || strstr(tests[i].name, argv[1])) { matched++; result |= indigo_run_tests("SX fake USB", tests + i, 1); }
	}
	return matched ? result : 1;
}
