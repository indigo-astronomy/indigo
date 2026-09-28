// Copyright (c) 2026 INDIGO initiative
// All rights reserved.
//
// You may use this software under the terms of 'INDIGO Astronomy
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

// Fake native SSAG/QHY5 USB protocol tests by OpenAI Codex.

#include <stdatomic.h>

#include <indigo/indigo_usb_utils.h>
#include <indigo_drivers/ccd_ssag/indigo_ccd_ssag.h>

#include "ccd_test_noise.h"
#include "simulator_test_common.h"

#define SSAG_CHECK(condition) do { if (!(condition)) { fprintf(stderr, "%s:%d: %s\n", __FILE__, __LINE__, #condition); indigo_test_failures++; goto cleanup; } } while (0)

#define SSAG_VENDOR_ID 0x1856
#define SSAG_PRODUCT_ID 0x0012
#define IMAGE_WIDTH 1280
#define IMAGE_HEIGHT 1024
#define BUFFER_WIDTH 1524
#define BUFFER_HEIGHT 1050
#define BUFFER_SIZE (BUFFER_WIDTH * BUFFER_HEIGHT)

static int usb_tokens[12];
static libusb_hotplug_callback_fn usb_callback;
static atomic_int attached_devices, refs, opened, closed, after_close, controls, bulk_reads;
static atomic_int fail_open, fail_claim, fail_control, fail_bulk, short_bulk, fail_attach, fail_registration, fail_queue;
static atomic_int connected[2], guide_state[2], guide_revision[2], guide_nonzero[2], frames, bad_frames;
static atomic_int last_guide_direction, last_guide_duration, last_exposure_ms;
static atomic_int init_gain, config_calls, claim_calls, cancel_ra, cancel_dec, firmware_writes;
static atomic_bool physical_open[12];
static double guide_started[2], guide_finished[2];
static indigo_device *logical_devices[24];
static uint16_t vendors[12], products[12];

static const char *camera_name = "SSAG";
static const char *guider_name = "SSAG (guider)";

static const simulator_driver_case ssag_case = {
	"SSAG/QHY5 Camera", "indigo_ccd_ssag", "SSAG", indigo_ccd_ssag, true,
	NULL, 0, NULL, 0, NULL, 0, NULL, 0
};

libusb_device *LIBUSB_CALL ssag_test_ref(libusb_device *dev) {
	atomic_fetch_add(&refs, 1);
	return dev;
}

void LIBUSB_CALL ssag_test_unref(libusb_device *dev) {
	atomic_fetch_sub(&refs, 1);
}

int LIBUSB_CALL ssag_test_descriptor(libusb_device *dev, struct libusb_device_descriptor *descriptor) {
	int index = (int *)dev - usb_tokens;
	memset(descriptor, 0, sizeof(*descriptor));
	descriptor->idVendor = vendors[index];
	descriptor->idProduct = products[index];
	return LIBUSB_SUCCESS;
}

indigo_result ssag_test_path(libusb_device *dev, char *path) {
	snprintf(path, INDIGO_NAME_SIZE, "fake-%ld", (long)((int *)dev - usb_tokens));
	return INDIGO_OK;
}

void ssag_test_usb_start(void) {
}

int LIBUSB_CALL ssag_test_register(libusb_context *ctx, int events, int flags, int vid, int pid, int cls, libusb_hotplug_callback_fn callback, void *data, libusb_hotplug_callback_handle *handle) {
	if (atomic_load(&fail_registration)) {
		return LIBUSB_ERROR_OTHER;
	}
	usb_callback = callback;
	*handle = 1;
	callback(NULL, (libusb_device *)usb_tokens, LIBUSB_HOTPLUG_EVENT_DEVICE_ARRIVED, NULL);
	return LIBUSB_SUCCESS;
}

int ssag_test_register_sim(libusb_context *ctx, libusb_hotplug_event events, libusb_hotplug_flag flags, int vid, int pid, int cls, libusb_hotplug_callback_fn callback, void *data, libusb_hotplug_callback_handle *handle) {
	return ssag_test_register(ctx, events, flags, vid, pid, cls, callback, data, handle);
}

void LIBUSB_CALL ssag_test_deregister(libusb_context *ctx, libusb_hotplug_callback_handle handle) {
	usb_callback = NULL;
}

int ssag_test_deregister_poll(libusb_context *ctx, libusb_hotplug_callback_handle handle) {
	ssag_test_deregister(ctx, handle);
	return LIBUSB_SUCCESS;
}

indigo_result ssag_test_attach(indigo_device *device) {
	if (atomic_load(&fail_attach) == atomic_load(&attached_devices) + 1) {
		return INDIGO_FAILED;
	}
	indigo_result result = indigo_attach_device(device);
	if (result == INDIGO_OK) {
		for (int i = 0; i < ARRAY_SIZE(logical_devices); i++) {
			if (logical_devices[i] == NULL) {
				logical_devices[i] = device;
				break;
			}
		}
		atomic_fetch_add(&attached_devices, 1);
	}
	return result;
}

indigo_result ssag_test_detach(indigo_device *device) {
	indigo_result result = indigo_detach_device(device);
	for (int i = 0; i < ARRAY_SIZE(logical_devices); i++) {
		if (logical_devices[i] == device) {
			logical_devices[i] = NULL;
			break;
		}
	}
	atomic_fetch_sub(&attached_devices, 1);
	return result;
}

int LIBUSB_CALL ssag_test_open(libusb_device *dev, libusb_device_handle **handle) {
	if (atomic_load(&fail_open)) {
		return LIBUSB_ERROR_ACCESS;
	}
	*handle = (libusb_device_handle *)dev;
	atomic_store(physical_open + ((int *)dev - usb_tokens), true);
	atomic_fetch_add(&opened, 1);
	return LIBUSB_SUCCESS;
}

void LIBUSB_CALL ssag_test_close(libusb_device_handle *handle) {
	atomic_store(physical_open + ((int *)handle - usb_tokens), false);
	atomic_fetch_add(&closed, 1);
}

int LIBUSB_CALL ssag_test_kernel(libusb_device_handle *handle, int interface) {
	return 0;
}

int LIBUSB_CALL ssag_test_detach_kernel(libusb_device_handle *handle, int interface) {
	return LIBUSB_SUCCESS;
}

int LIBUSB_CALL ssag_test_set_configuration(libusb_device_handle *handle, int configuration) {
	atomic_fetch_add(&config_calls, 1);
	return LIBUSB_SUCCESS;
}

int LIBUSB_CALL ssag_test_claim(libusb_device_handle *handle, int interface) {
	atomic_fetch_add(&claim_calls, 1);
	return atomic_load(&fail_claim) ? LIBUSB_ERROR_BUSY : LIBUSB_SUCCESS;
}

int LIBUSB_CALL ssag_test_release(libusb_device_handle *handle, int interface) {
	return LIBUSB_SUCCESS;
}

int LIBUSB_CALL ssag_test_control(libusb_device_handle *handle, uint8_t request_type, uint8_t request, uint16_t value, uint16_t index, unsigned char *data, uint16_t length, unsigned int timeout) {
	if (!atomic_load(physical_open + ((int *)handle - usb_tokens))) {
		atomic_fetch_add(&after_close, 1);
		return LIBUSB_ERROR_NO_DEVICE;
	}
	atomic_fetch_add(&controls, 1);
	if (atomic_load(&fail_control) == request) {
		return LIBUSB_ERROR_IO;
	}
	if (request == 0xa0) {
		atomic_fetch_add(&firmware_writes, 1);
	}
	if (request == 19 && length == 18) {
		atomic_store(&init_gain, data[1]);
	}
	if (request == 18) {
		atomic_store(&last_exposure_ms, value | ((unsigned)index << 16));
	}
	if (request == 16 && length == 8) {
		int duration;
		memcpy(&duration, data, sizeof(duration));
		atomic_store(&last_guide_direction, index);
		atomic_store(&last_guide_duration, duration);
	}
	if (request == 33) {
		atomic_fetch_add(&cancel_ra, 1);
	}
	if (request == 34) {
		atomic_fetch_add(&cancel_dec, 1);
	}
	return length;
}

int LIBUSB_CALL ssag_test_bulk(libusb_device_handle *handle, unsigned char endpoint, unsigned char *data, int length, int *transferred, unsigned int timeout) {
	if (!atomic_load(physical_open + ((int *)handle - usb_tokens))) {
		atomic_fetch_add(&after_close, 1);
		return LIBUSB_ERROR_NO_DEVICE;
	}
	if (atomic_load(&fail_bulk)) {
		return LIBUSB_ERROR_IO;
	}
	if (endpoint == 0x82) {
		atomic_fetch_add(&bulk_reads, 1);
		for (int i = 0; i < length; i++) {
			data[i] = (unsigned char)ccd_test_noise(i, 0);
		}
		*transferred = atomic_load(&short_bulk) ? length - 1 : length;
	}
	return LIBUSB_SUCCESS;
}

indigo_queue *ssag_test_queue_create(indigo_device *device) {
	return atomic_load(&fail_queue) ? NULL : indigo_queue_create(device);
}

static indigo_result ssag_update(indigo_client *client, indigo_device *device, indigo_property *property, const char *message) {
	if (!strcmp(property->name, CONNECTION_PROPERTY_NAME)) {
		int index = !strcmp(property->device, guider_name);
		if (!strcmp(property->device, camera_name) || index) {
			atomic_store(connected + index, property->state == INDIGO_ALERT_STATE ? -1 : property->state == INDIGO_BUSY_STATE ? 2 : property->items[0].sw.value ? 1 : 0);
		}
	}
	if (!strcmp(property->device, guider_name)) {
		if (!strcmp(property->name, GUIDER_GUIDE_RA_PROPERTY_NAME)) {
			if (property->state == INDIGO_BUSY_STATE) {
				guide_started[0] = indigo_monotonic_time();
			} else if (property->state == INDIGO_OK_STATE && guide_started[0] > 0) {
				guide_finished[0] = indigo_monotonic_time();
			}
			atomic_store(guide_state, property->state);
			atomic_fetch_add(guide_revision, 1);
			atomic_store(guide_nonzero, property->items[0].number.value != 0 || property->items[1].number.value != 0);
		}
		if (!strcmp(property->name, GUIDER_GUIDE_DEC_PROPERTY_NAME)) {
			if (property->state == INDIGO_BUSY_STATE) {
				guide_started[1] = indigo_monotonic_time();
			} else if (property->state == INDIGO_OK_STATE && guide_started[1] > 0) {
				guide_finished[1] = indigo_monotonic_time();
			}
			atomic_store(guide_state + 1, property->state);
			atomic_fetch_add(guide_revision + 1, 1);
			atomic_store(guide_nonzero + 1, property->items[0].number.value != 0 || property->items[1].number.value != 0);
		}
	}
	if (!strcmp(property->device, camera_name) && !strcmp(property->name, CCD_IMAGE_PROPERTY_NAME) && property->state == INDIGO_OK_STATE && property->count > 0 && property->items[0].blob.size > 0) {
		indigo_raw_header header = { 0 };
		indigo_item *item = property->items;
		bool valid = item->blob.value != NULL && !strcmp(item->blob.format, ".raw") && item->blob.size >= sizeof(header);
		if (valid) {
			memcpy(&header, item->blob.value, sizeof(header));
			valid = header.signature == INDIGO_RAW_MONO8 && header.width == IMAGE_WIDTH && header.height == IMAGE_HEIGHT && item->blob.size >= sizeof(header) + IMAGE_WIDTH * IMAGE_HEIGHT;
		}
		if (valid) {
			unsigned char *pixels = (unsigned char *)item->blob.value + sizeof(header);
			for (int y = 0; y < IMAGE_HEIGHT; y += 127) {
				for (int x = 0; x < IMAGE_WIDTH; x += 251) {
					int source = y == 0 ? x : y * BUFFER_WIDTH + x;
					if (pixels[y * IMAGE_WIDTH + x] != (unsigned char)ccd_test_noise(source, 0)) {
						valid = false;
					}
				}
			}
		}
		if (!valid) {
			atomic_fetch_add(&bad_frames, 1);
		}
		atomic_fetch_add(&frames, 1);
	}
	return simulator_client_update_property(client, device, property, message);
}

static bool wait_atomic(atomic_int *value, int expected) {
	for (int i = 0; i < 600; i++) {
		if (atomic_load(value) == expected) {
			return true;
		}
		indigo_usleep(10000);
	}
	return false;
}

static bool change_connection(const char *device_name, int index, bool connect) {
	if (indigo_change_switch_property_1(&simulator_test_client, device_name, CONNECTION_PROPERTY_NAME, connect ? CONNECTION_CONNECTED_ITEM_NAME : CONNECTION_DISCONNECTED_ITEM_NAME, true) != INDIGO_OK) {
		return false;
	}
	return wait_atomic(connected + index, connect ? 1 : 0);
}

static void ssag_start(void) {
	memset(logical_devices, 0, sizeof(logical_devices));
	memset(vendors, 0, sizeof(vendors));
	memset(products, 0, sizeof(products));
	vendors[0] = SSAG_VENDOR_ID;
	products[0] = SSAG_PRODUCT_ID;
	atomic_store(&connected[0], 0);
	atomic_store(&connected[1], 0);
	atomic_store(&guide_state[0], INDIGO_IDLE_STATE);
	atomic_store(&guide_state[1], INDIGO_IDLE_STATE);
	atomic_store(&guide_revision[0], 0);
	atomic_store(&guide_revision[1], 0);
	simulator_test_client.update_property = ssag_update;
	reset_simulator_context(&ssag_case);
	indigo_start();
	indigo_attach_client(&simulator_test_client);
	ASSERT_EQ_INT(INDIGO_OK, indigo_ccd_ssag(INDIGO_DRIVER_INIT, NULL));
	ASSERT_TRUE(wait_atomic(&attached_devices, 2));
}

static void ssag_end(void) {
	atomic_store(&fail_open, 0);
	atomic_store(&fail_claim, 0);
	atomic_store(&fail_control, 0);
	atomic_store(&fail_bulk, 0);
	atomic_store(&short_bulk, 0);
	atomic_store(&fail_attach, 0);
	atomic_store(&fail_registration, 0);
	atomic_store(&fail_queue, 0);
	if (atomic_load(connected + 1) > 0) {
		change_connection(guider_name, 1, false);
	}
	if (atomic_load(connected) > 0) {
		change_connection(camera_name, 0, false);
	}
	indigo_ccd_ssag(INDIGO_DRIVER_SHUTDOWN, NULL);
	indigo_detach_client(&simulator_test_client);
	indigo_stop();
	ASSERT_EQ_INT(atomic_load(&opened), atomic_load(&closed));
	ASSERT_EQ_INT(0, atomic_load(&refs));
	ASSERT_EQ_INT(0, atomic_load(&after_close));
	simulator_test_client.update_property = simulator_client_update_property;
}

static void identity_properties_and_image(void) {
	ssag_start();
	SSAG_CHECK(change_connection(camera_name, 0, true));
	SSAG_CHECK(find_cached_item(CCD_INFO_PROPERTY_NAME, CCD_INFO_WIDTH_ITEM_NAME)->number.value == IMAGE_WIDTH);
	SSAG_CHECK(find_cached_item(CCD_INFO_PROPERTY_NAME, CCD_INFO_HEIGHT_ITEM_NAME)->number.value == IMAGE_HEIGHT);
	SSAG_CHECK(find_cached_item(CCD_GAIN_PROPERTY_NAME, CCD_GAIN_ITEM_NAME)->number.min == 1);
	SSAG_CHECK(find_cached_item(CCD_GAIN_PROPERTY_NAME, CCD_GAIN_ITEM_NAME)->number.max == 15);
	SSAG_CHECK(indigo_change_switch_property_1(&simulator_test_client, camera_name, CCD_IMAGE_FORMAT_PROPERTY_NAME, CCD_IMAGE_FORMAT_RAW_ITEM_NAME, true) == INDIGO_OK);
	SSAG_CHECK(wait_for_property_state(CCD_IMAGE_FORMAT_PROPERTY_NAME, INDIGO_OK_STATE));
	SSAG_CHECK(indigo_change_switch_property_1(&simulator_test_client, camera_name, CCD_UPLOAD_MODE_PROPERTY_NAME, CCD_UPLOAD_MODE_CLIENT_ITEM_NAME, true) == INDIGO_OK);
	SSAG_CHECK(wait_for_property_state(CCD_UPLOAD_MODE_PROPERTY_NAME, INDIGO_OK_STATE));
	SSAG_CHECK(indigo_change_number_property_1(&simulator_test_client, camera_name, CCD_EXPOSURE_PROPERTY_NAME, CCD_EXPOSURE_ITEM_NAME, 0.02) == INDIGO_OK);
	SSAG_CHECK(wait_for_property_state(CCD_EXPOSURE_PROPERTY_NAME, INDIGO_OK_STATE));
	SSAG_CHECK(atomic_load(&last_exposure_ms) == 20 && atomic_load(&frames) == 1 && atomic_load(&bad_frames) == 0);
cleanup:
	ssag_end();
}

static void shared_lifecycle(void) {
	int open_before = atomic_load(&opened), close_before = atomic_load(&closed);
	ssag_start();
	SSAG_CHECK(change_connection(guider_name, 1, true));
	SSAG_CHECK(change_connection(camera_name, 0, true));
	SSAG_CHECK(atomic_load(&opened) == open_before + 1);
	SSAG_CHECK(change_connection(camera_name, 0, false));
	SSAG_CHECK(atomic_load(&closed) == close_before);
	SSAG_CHECK(change_connection(guider_name, 1, false));
	SSAG_CHECK(atomic_load(&closed) == close_before + 1);
cleanup:
	ssag_end();
}

static void initialization_failure_recovers(void) {
	ssag_start();
	atomic_store(&fail_open, 1);
	SSAG_CHECK(indigo_change_switch_property_1(&simulator_test_client, camera_name, CONNECTION_PROPERTY_NAME, CONNECTION_CONNECTED_ITEM_NAME, true) == INDIGO_OK);
	SSAG_CHECK(wait_atomic(connected, -1));
	atomic_store(&fail_open, 0);
	SSAG_CHECK(change_connection(camera_name, 0, true));
	SSAG_CHECK(change_connection(camera_name, 0, false));
	atomic_store(&fail_claim, 1);
	SSAG_CHECK(indigo_change_switch_property_1(&simulator_test_client, camera_name, CONNECTION_PROPERTY_NAME, CONNECTION_CONNECTED_ITEM_NAME, true) == INDIGO_OK);
	SSAG_CHECK(wait_atomic(connected, -1));
	atomic_store(&fail_claim, 0);
	SSAG_CHECK(change_connection(camera_name, 0, true));
	SSAG_CHECK(change_connection(camera_name, 0, false));
	atomic_store(&fail_control, 85);
	SSAG_CHECK(indigo_change_switch_property_1(&simulator_test_client, camera_name, CONNECTION_PROPERTY_NAME, CONNECTION_CONNECTED_ITEM_NAME, true) == INDIGO_OK);
	SSAG_CHECK(wait_atomic(connected, -1));
	atomic_store(&fail_control, 0);
	SSAG_CHECK(change_connection(camera_name, 0, true));
cleanup:
	ssag_end();
}

static void gain_protocol_mapping(void) {
	ssag_start();
	SSAG_CHECK(change_connection(camera_name, 0, true));
	SSAG_CHECK(indigo_change_number_property_1(&simulator_test_client, camera_name, CCD_GAIN_PROPERTY_NAME, CCD_GAIN_ITEM_NAME, 4) == INDIGO_OK);
	SSAG_CHECK(wait_for_property_state(CCD_GAIN_PROPERTY_NAME, INDIGO_OK_STATE));
	SSAG_CHECK(indigo_change_number_property_1(&simulator_test_client, camera_name, CCD_EXPOSURE_PROPERTY_NAME, CCD_EXPOSURE_ITEM_NAME, 0.01) == INDIGO_OK);
	SSAG_CHECK(wait_for_property_state(CCD_EXPOSURE_PROPERTY_NAME, INDIGO_OK_STATE));
	SSAG_CHECK(atomic_load(&init_gain) == 32);
cleanup:
	ssag_end();
}

static void guide_direction_and_completion(void) {
	ssag_start();
	SSAG_CHECK(change_connection(guider_name, 1, true));
	SSAG_CHECK(indigo_change_number_property_1(&simulator_test_client, guider_name, GUIDER_GUIDE_RA_PROPERTY_NAME, GUIDER_GUIDE_EAST_ITEM_NAME, 50) == INDIGO_OK);
	SSAG_CHECK(wait_atomic(guide_state, INDIGO_BUSY_STATE));
	SSAG_CHECK(wait_atomic(guide_state, INDIGO_OK_STATE));
	SSAG_CHECK(atomic_load(&last_guide_direction) == 0x10 && atomic_load(&last_guide_duration) == 50 && atomic_load(guide_nonzero) == 0);
	SSAG_CHECK(indigo_change_number_property_1(&simulator_test_client, guider_name, GUIDER_GUIDE_RA_PROPERTY_NAME, GUIDER_GUIDE_WEST_ITEM_NAME, 30) == INDIGO_OK);
	SSAG_CHECK(wait_atomic(guide_state, INDIGO_BUSY_STATE));
	SSAG_CHECK(wait_atomic(guide_state, INDIGO_OK_STATE));
	SSAG_CHECK(atomic_load(&last_guide_direction) == 0x80 && atomic_load(&last_guide_duration) == 30);
	SSAG_CHECK(indigo_change_number_property_1(&simulator_test_client, guider_name, GUIDER_GUIDE_DEC_PROPERTY_NAME, GUIDER_GUIDE_NORTH_ITEM_NAME, 40) == INDIGO_OK);
	SSAG_CHECK(wait_atomic(guide_state + 1, INDIGO_BUSY_STATE));
	SSAG_CHECK(wait_atomic(guide_state + 1, INDIGO_OK_STATE));
	SSAG_CHECK(atomic_load(&last_guide_direction) == 0x40 && atomic_load(&last_guide_duration) == 40);
	SSAG_CHECK(indigo_change_number_property_1(&simulator_test_client, guider_name, GUIDER_GUIDE_DEC_PROPERTY_NAME, GUIDER_GUIDE_SOUTH_ITEM_NAME, 20) == INDIGO_OK);
	SSAG_CHECK(wait_atomic(guide_state + 1, INDIGO_BUSY_STATE));
	SSAG_CHECK(wait_atomic(guide_state + 1, INDIGO_OK_STATE));
	SSAG_CHECK(atomic_load(&last_guide_direction) == 0x20 && atomic_load(&last_guide_duration) == 20 && atomic_load(guide_nonzero + 1) == 0);
cleanup:
	ssag_end();
}

static void transfer_failures(void) {
	ssag_start();
	SSAG_CHECK(change_connection(camera_name, 0, true));
	atomic_store(&fail_control, 18);
	SSAG_CHECK(indigo_change_number_property_1(&simulator_test_client, camera_name, CCD_EXPOSURE_PROPERTY_NAME, CCD_EXPOSURE_ITEM_NAME, 0.01) == INDIGO_OK);
	SSAG_CHECK(wait_for_property_state(CCD_EXPOSURE_PROPERTY_NAME, INDIGO_ALERT_STATE));
	atomic_store(&fail_control, 0);
	atomic_store(&short_bulk, 1);
	SSAG_CHECK(indigo_change_number_property_1(&simulator_test_client, camera_name, CCD_EXPOSURE_PROPERTY_NAME, CCD_EXPOSURE_ITEM_NAME, 0.01) == INDIGO_OK);
	SSAG_CHECK(wait_for_property_state(CCD_EXPOSURE_PROPERTY_NAME, INDIGO_ALERT_STATE));
cleanup:
	ssag_end();
}

static void abort_and_reacquire(void) {
	ssag_start();
	SSAG_CHECK(change_connection(camera_name, 0, true));
	int reads = atomic_load(&bulk_reads);
	int frame_count = atomic_load(&frames);
	SSAG_CHECK(indigo_change_number_property_1(&simulator_test_client, camera_name, CCD_EXPOSURE_PROPERTY_NAME, CCD_EXPOSURE_ITEM_NAME, 2) == INDIGO_OK);
	SSAG_CHECK(wait_for_property_state(CCD_EXPOSURE_PROPERTY_NAME, INDIGO_BUSY_STATE));
	SSAG_CHECK(wait_atomic(&last_exposure_ms, 2000));
	SSAG_CHECK(indigo_change_switch_property_1(&simulator_test_client, camera_name, CCD_ABORT_EXPOSURE_PROPERTY_NAME, CCD_ABORT_EXPOSURE_ITEM_NAME, true) == INDIGO_OK);
	SSAG_CHECK(wait_for_property_state(CCD_EXPOSURE_PROPERTY_NAME, INDIGO_ALERT_STATE));
	SSAG_CHECK(wait_for_property_state(CCD_ABORT_EXPOSURE_PROPERTY_NAME, INDIGO_OK_STATE));
	SSAG_CHECK(indigo_change_number_property_1(&simulator_test_client, camera_name, CCD_EXPOSURE_PROPERTY_NAME, CCD_EXPOSURE_ITEM_NAME, 0.015) == INDIGO_OK);
	SSAG_CHECK(wait_for_property_state(CCD_EXPOSURE_PROPERTY_NAME, INDIGO_ALERT_STATE));
	SSAG_CHECK(wait_atomic(&bulk_reads, reads + 1));
	SSAG_CHECK(wait_for_property_state(CCD_EXPOSURE_PROPERTY_NAME, INDIGO_OK_STATE));
	SSAG_CHECK(atomic_load(&frames) == frame_count);
	SSAG_CHECK(indigo_change_number_property_1(&simulator_test_client, camera_name, CCD_EXPOSURE_PROPERTY_NAME, CCD_EXPOSURE_ITEM_NAME, 0.015) == INDIGO_OK);
	SSAG_CHECK(wait_for_property_state(CCD_EXPOSURE_PROPERTY_NAME, INDIGO_OK_STATE));
	SSAG_CHECK(atomic_load(&last_exposure_ms) == 15);
cleanup:
	ssag_end();
}

static void guider_replacement_axes_and_failure(void) {
	ssag_start();
	SSAG_CHECK(change_connection(guider_name, 1, true));
	SSAG_CHECK(indigo_change_number_property_1(&simulator_test_client, guider_name, GUIDER_GUIDE_RA_PROPERTY_NAME, GUIDER_GUIDE_EAST_ITEM_NAME, 2000) == INDIGO_OK);
	SSAG_CHECK(wait_atomic(guide_state, INDIGO_BUSY_STATE));
	atomic_store(&last_guide_direction, 0);
	SSAG_CHECK(indigo_change_number_property_1(&simulator_test_client, guider_name, GUIDER_GUIDE_RA_PROPERTY_NAME, GUIDER_GUIDE_WEST_ITEM_NAME, 25) == INDIGO_OK);
	SSAG_CHECK(wait_atomic(&last_guide_direction, 0x80));
	SSAG_CHECK(wait_atomic(guide_state, INDIGO_OK_STATE));
	SSAG_CHECK(atomic_load(&cancel_ra) > 0);
	SSAG_CHECK(indigo_change_number_property_1(&simulator_test_client, guider_name, GUIDER_GUIDE_RA_PROPERTY_NAME, GUIDER_GUIDE_EAST_ITEM_NAME, 80) == INDIGO_OK);
	SSAG_CHECK(indigo_change_number_property_1(&simulator_test_client, guider_name, GUIDER_GUIDE_DEC_PROPERTY_NAME, GUIDER_GUIDE_SOUTH_ITEM_NAME, 100) == INDIGO_OK);
	SSAG_CHECK(wait_atomic(guide_state, INDIGO_BUSY_STATE));
	SSAG_CHECK(wait_atomic(guide_state + 1, INDIGO_BUSY_STATE));
	SSAG_CHECK(wait_atomic(guide_state, INDIGO_OK_STATE));
	SSAG_CHECK(wait_atomic(guide_state + 1, INDIGO_OK_STATE));
	atomic_store(&fail_control, 16);
	SSAG_CHECK(indigo_change_number_property_1(&simulator_test_client, guider_name, GUIDER_GUIDE_DEC_PROPERTY_NAME, GUIDER_GUIDE_NORTH_ITEM_NAME, 20) == INDIGO_OK);
	SSAG_CHECK(wait_atomic(guide_state + 1, INDIGO_ALERT_STATE));
	atomic_store(&fail_control, 0);
	SSAG_CHECK(indigo_change_number_property_1(&simulator_test_client, guider_name, GUIDER_GUIDE_DEC_PROPERTY_NAME, GUIDER_GUIDE_NORTH_ITEM_NAME, 20) == INDIGO_OK);
	SSAG_CHECK(wait_atomic(guide_state + 1, INDIGO_BUSY_STATE));
	SSAG_CHECK(wait_atomic(guide_state + 1, INDIGO_OK_STATE));
cleanup:
	ssag_end();
}

static int compare_double(const void *left, const void *right) {
	double a = *(const double *)left;
	double b = *(const double *)right;
	return a < b ? -1 : a > b;
}

static bool measure_guide_pulse(const char *property, const char *item, int axis, int duration, double *error) {
	guide_started[axis] = guide_finished[axis] = 0;
	int revision = atomic_load(guide_revision + axis);
	if (indigo_change_number_property_1(&simulator_test_client, guider_name, property, item, duration) != INDIGO_OK) {
		return false;
	}
	for (int i = 0; i < 600 && (atomic_load(guide_revision + axis) <= revision || atomic_load(guide_state + axis) != INDIGO_BUSY_STATE); i++) {
		indigo_usleep(10000);
	}
	if (atomic_load(guide_state + axis) != INDIGO_BUSY_STATE) {
		return false;
	}
	revision = atomic_load(guide_revision + axis);
	for (int i = 0; i < 600 && (atomic_load(guide_revision + axis) <= revision || atomic_load(guide_state + axis) != INDIGO_OK_STATE); i++) {
		indigo_usleep(10000);
	}
	if (atomic_load(guide_state + axis) != INDIGO_OK_STATE || guide_finished[axis] < guide_started[axis]) {
		return false;
	}
	*error = (guide_finished[axis] - guide_started[axis]) * 1000 - duration;
	return true;
}

static void report_guide_timing(const char *workload, int duration, double *errors, int count) {
	double sorted[8], sum = 0, sum2 = 0, max_abs = 0;
	memcpy(sorted, errors, count * sizeof(double));
	qsort(sorted, count, sizeof(double), compare_double);
	for (int i = 0; i < count; i++) {
		sum += errors[i];
		sum2 += errors[i] * errors[i];
		max_abs = fmax(max_abs, fabs(errors[i]));
	}
	double mean = sum / count;
	double deviation = sqrt(fmax(0, sum2 / count - mean * mean));
	printf("    %s %d ms: n=%d min=%+.3f mean=%+.3f median=%+.3f p95=%+.3f p99=%+.3f max=%+.3f sd=%.3f max_abs=%.3f ms\n", workload, duration, count, sorted[0], mean, (sorted[count / 2 - 1] + sorted[count / 2]) / 2, sorted[(int)ceil(0.95 * count) - 1], sorted[(int)ceil(0.99 * count) - 1], sorted[count - 1], deviation, max_abs);
}

static void guider_timing_measurements(void) {
	const char *properties[] = { GUIDER_GUIDE_RA_PROPERTY_NAME, GUIDER_GUIDE_RA_PROPERTY_NAME, GUIDER_GUIDE_DEC_PROPERTY_NAME, GUIDER_GUIDE_DEC_PROPERTY_NAME };
	const char *items[] = { GUIDER_GUIDE_EAST_ITEM_NAME, GUIDER_GUIDE_WEST_ITEM_NAME, GUIDER_GUIDE_NORTH_ITEM_NAME, GUIDER_GUIDE_SOUTH_ITEM_NAME };
	const int axes[] = { 0, 0, 1, 1 };
	const int durations[] = { 20, 100, 500 };
	ssag_start();
	SSAG_CHECK(change_connection(guider_name, 1, true));
	SSAG_CHECK(change_connection(camera_name, 0, true));
	for (int workload = 0; workload < 2; workload++) {
		if (workload == 1) {
			SSAG_CHECK(indigo_change_number_property_1(&simulator_test_client, camera_name, CCD_EXPOSURE_PROPERTY_NAME, CCD_EXPOSURE_ITEM_NAME, 8) == INDIGO_OK);
			SSAG_CHECK(wait_for_property_state(CCD_EXPOSURE_PROPERTY_NAME, INDIGO_BUSY_STATE));
		}
		for (int direction = 0; direction < 4; direction++) {
			double ignored;
			SSAG_CHECK(measure_guide_pulse(properties[direction], items[direction], axes[direction], 20, &ignored));
		}
		for (int duration = 0; duration < 3; duration++) {
			double errors[8];
			int count = 0;
			for (int repeat = 0; repeat < 2; repeat++) {
				for (int direction = 0; direction < 4; direction++) {
					SSAG_CHECK(measure_guide_pulse(properties[direction], items[direction], axes[direction], durations[duration], errors + count));
					count++;
				}
			}
			report_guide_timing(workload ? "during-exposure" : "idle", durations[duration], errors, count);
		}
		if (workload == 1) {
			SSAG_CHECK(indigo_change_switch_property_1(&simulator_test_client, camera_name, CCD_ABORT_EXPOSURE_PROPERTY_NAME, CCD_ABORT_EXPOSURE_ITEM_NAME, true) == INDIGO_OK);
			SSAG_CHECK(wait_for_property_state(CCD_ABORT_EXPOSURE_PROPERTY_NAME, INDIGO_OK_STATE));
		}
	}
cleanup:
	ssag_end();
}

static void discovery_capacity_and_recovery(void) {
	ssag_start();
	SSAG_CHECK(atomic_load(&attached_devices) == 2);
	usb_callback(NULL, (libusb_device *)usb_tokens, LIBUSB_HOTPLUG_EVENT_DEVICE_ARRIVED, NULL);
	indigo_usleep(50000);
	SSAG_CHECK(atomic_load(&attached_devices) == 2);
	vendors[1] = 0x9999;
	products[1] = 0x9999;
	usb_callback(NULL, (libusb_device *)(usb_tokens + 1), LIBUSB_HOTPLUG_EVENT_DEVICE_ARRIVED, NULL);
	indigo_usleep(50000);
	SSAG_CHECK(atomic_load(&attached_devices) == 2);
	vendors[1] = vendors[2] = vendors[3] = SSAG_VENDOR_ID;
	products[1] = products[2] = products[3] = SSAG_PRODUCT_ID;
	usb_callback(NULL, (libusb_device *)(usb_tokens + 1), LIBUSB_HOTPLUG_EVENT_DEVICE_ARRIVED, NULL);
	SSAG_CHECK(wait_atomic(&attached_devices, 4));
	usb_callback(NULL, (libusb_device *)(usb_tokens + 2), LIBUSB_HOTPLUG_EVENT_DEVICE_ARRIVED, NULL);
	SSAG_CHECK(wait_atomic(&attached_devices, 5));
	usb_callback(NULL, (libusb_device *)(usb_tokens + 3), LIBUSB_HOTPLUG_EVENT_DEVICE_ARRIVED, NULL);
	indigo_usleep(50000);
	SSAG_CHECK(atomic_load(&attached_devices) == 5);
	usb_callback(NULL, (libusb_device *)(usb_tokens + 1), LIBUSB_HOTPLUG_EVENT_DEVICE_LEFT, NULL);
	SSAG_CHECK(wait_atomic(&attached_devices, 3));
	usb_callback(NULL, (libusb_device *)(usb_tokens + 3), LIBUSB_HOTPLUG_EVENT_DEVICE_ARRIVED, NULL);
	SSAG_CHECK(wait_atomic(&attached_devices, 5));
cleanup:
	ssag_end();
}

static void loader_firmware_and_reference_balance(void) {
	int writes_before = atomic_load(&firmware_writes), opened_before = atomic_load(&opened), closed_before = atomic_load(&closed);
	ssag_start();
	vendors[1] = SSAG_VENDOR_ID;
	products[1] = 0x0011;
	usb_callback(NULL, (libusb_device *)(usb_tokens + 1), LIBUSB_HOTPLUG_EVENT_DEVICE_ARRIVED, NULL);
	SSAG_CHECK(wait_atomic(&opened, opened_before + 1));
	SSAG_CHECK(wait_atomic(&closed, closed_before + 1));
	SSAG_CHECK(atomic_load(&firmware_writes) > writes_before && atomic_load(&attached_devices) == 2);
cleanup:
	ssag_end();
}

static void attach_failure_and_retry(void) {
	memset(vendors, 0, sizeof(vendors));
	memset(products, 0, sizeof(products));
	vendors[0] = SSAG_VENDOR_ID;
	products[0] = SSAG_PRODUCT_ID;
	atomic_store(&fail_attach, 1);
	simulator_test_client.update_property = ssag_update;
	reset_simulator_context(&ssag_case);
	indigo_start();
	indigo_attach_client(&simulator_test_client);
	SSAG_CHECK(indigo_ccd_ssag(INDIGO_DRIVER_INIT, NULL) == INDIGO_OK);
	indigo_usleep(50000);
	SSAG_CHECK(atomic_load(&attached_devices) == 0);
	atomic_store(&fail_attach, 0);
	usb_callback(NULL, (libusb_device *)usb_tokens, LIBUSB_HOTPLUG_EVENT_DEVICE_ARRIVED, NULL);
	SSAG_CHECK(wait_atomic(&attached_devices, 2));
cleanup:
	ssag_end();
}

static void initialization_registration_and_queue_rollback(void) {
	memset(vendors, 0, sizeof(vendors));
	memset(products, 0, sizeof(products));
	vendors[0] = SSAG_VENDOR_ID;
	products[0] = SSAG_PRODUCT_ID;
	reset_simulator_context(&ssag_case);
	indigo_start();
	atomic_store(&fail_queue, 1);
	SSAG_CHECK(indigo_ccd_ssag(INDIGO_DRIVER_INIT, NULL) == INDIGO_FAILED);
	atomic_store(&fail_queue, 0);
	atomic_store(&fail_registration, 1);
	SSAG_CHECK(indigo_ccd_ssag(INDIGO_DRIVER_INIT, NULL) == INDIGO_FAILED);
	atomic_store(&fail_registration, 0);
	SSAG_CHECK(indigo_ccd_ssag(INDIGO_DRIVER_INIT, NULL) == INDIGO_OK);
	SSAG_CHECK(wait_atomic(&attached_devices, 2));
cleanup:
	atomic_store(&fail_queue, 0);
	atomic_store(&fail_registration, 0);
	indigo_ccd_ssag(INDIGO_DRIVER_SHUTDOWN, NULL);
	indigo_stop();
}

int main(int argc, char **argv) {
	setvbuf(stdout, NULL, _IONBF, 0);
	const indigo_test_case tests[] = {
		{ "Identity, properties and deterministic image", identity_properties_and_image },
		{ "Shared CCD and guider lifecycle", shared_lifecycle },
		{ "Initialization failure and recovery", initialization_failure_recovers },
		{ "Gain protocol mapping", gain_protocol_mapping },
		{ "Guide direction and completion", guide_direction_and_completion },
		{ "Exposure and short-transfer failures", transfer_failures },
		{ "Abort drain and guarded reacquisition", abort_and_reacquire },
		{ "Guider replacement, simultaneous axes and failure", guider_replacement_axes_and_failure },
		{ "Guider completion timing", guider_timing_measurements },
		{ "Discovery, capacity and recovery", discovery_capacity_and_recovery },
		{ "Loader firmware and reference balance", loader_firmware_and_reference_balance },
		{ "Attach failure and retry", attach_failure_and_retry },
		{ "Queue and registration rollback", initialization_registration_and_queue_rollback }
	};
	if (argc == 2) {
		for (int i = 0; i < ARRAY_SIZE(tests); i++) {
			if (!strcmp(argv[1], tests[i].name)) {
				return indigo_run_tests("SSAG fake USB", tests + i, 1);
			}
		}
		fprintf(stderr, "Unknown test: %s\n", argv[1]);
		return 2;
	}
	return indigo_run_tests("SSAG fake USB", tests, ARRAY_SIZE(tests));
}
