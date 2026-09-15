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

// Deterministic fake-libdc1394 coverage for the production IIDC driver.

#include <stdatomic.h>
#include <libusb-1.0/libusb.h>
#include <dc1394/dc1394.h>
#include <indigo/indigo_ccd_driver.h>
#include <indigo_drivers/ccd_iidc/indigo_ccd_iidc.h>
#include "simulator_test_common.h"
#include "ccd_test_noise.h"

#define CAMERAS 12

typedef struct {
	dc1394camera_t public;
	char model[32];
	bool visible, temperature;
	dc1394video_mode_t mode;
	dc1394color_coding_t coding;
	uint32_t left, top, width, height;
	atomic_int capture, streaming, polls, frames, stops;
	double gain, gamma, shutter;
	uint8_t pixels[640 * 480 * 6];
	dc1394video_frame_t frame;
} fake_camera;

static fake_camera cameras[CAMERAS];
static int usb_tokens[CAMERAS];
static indigo_device *logical[CAMERAS];
static libusb_hotplug_callback_fn usb_callback;
static atomic_int attached, refs, blobs, bad_blob, calls_after_free, fail_register, fail_context, fail_enumerate, dequeue_error, malformed_frame, hold_frames;
static _Atomic(const char *) fail_call;
static const char *device_name = "Atik GP fake";

static const simulator_driver_case iidc_case = {
	"IIDC Compatible Camera", "indigo_ccd_iidc", "Atik GP fake", indigo_ccd_iidc, true,
	base_properties_without_instances, ARRAY_SIZE(base_properties_without_instances), hidden_base_properties_without_instances, ARRAY_SIZE(hidden_base_properties_without_instances), NULL, 0, NULL, 0
};

static fake_camera *fake(dc1394camera_t *camera) {
	for (int i = 0; i < CAMERAS; i++) {
		if (&cameras[i].public == camera) {
			return cameras + i;
		}
	}
	atomic_fetch_add(&calls_after_free, 1);
	return cameras;
}

dc1394_t *iidc_test_new(void) {
	return atomic_load(&fail_context) ? NULL : (dc1394_t *)cameras;
}

void iidc_test_free(dc1394_t *context) { }

dc1394error_t iidc_test_enumerate(dc1394_t *context, dc1394camera_list_t **result) {
	if (atomic_load(&fail_enumerate)) {
		return DC1394_FAILURE;
	}
	dc1394camera_list_t *list = calloc(1, sizeof(*list));
	list->ids = calloc(CAMERAS, sizeof(*list->ids));
	for (int i = 0; i < CAMERAS; i++) {
		if (cameras[i].visible) {
			list->ids[list->num++] = (dc1394camera_id_t) { .guid = cameras[i].public.guid, .unit = cameras[i].public.unit };
		}
	}
	*result = list;
	return DC1394_SUCCESS;
}

void iidc_test_free_list(dc1394camera_list_t *list) {
	if (list) {
		free(list->ids);
		free(list);
	}
}

dc1394camera_t *iidc_test_camera_new(dc1394_t *context, uint64_t guid, int unit) {
	for (int i = 0; i < CAMERAS; i++) {
		if (cameras[i].visible && cameras[i].public.guid == guid && cameras[i].public.unit == unit) {
			return &cameras[i].public;
		}
	}
	return NULL;
}

void iidc_test_camera_free(dc1394camera_t *camera) { }

dc1394error_t iidc_test_modes(dc1394camera_t *camera, dc1394video_modes_t *modes) {
	if (atomic_load(&fail_call) && !strcmp(atomic_load(&fail_call), "modes")) {
		return DC1394_FAILURE;
	}
	modes->num = 2;
	modes->modes[0] = DC1394_VIDEO_MODE_FORMAT7_0;
	modes->modes[1] = DC1394_VIDEO_MODE_640x480_MONO8;
	return DC1394_SUCCESS;
}

dc1394error_t iidc_test_codings(dc1394camera_t *camera, dc1394video_mode_t mode, dc1394color_codings_t *codings) {
	codings->num = 3;
	codings->codings[0] = DC1394_COLOR_CODING_MONO8;
	codings->codings[1] = DC1394_COLOR_CODING_RAW16;
	codings->codings[2] = DC1394_COLOR_CODING_YUV422;
	return DC1394_SUCCESS;
}

dc1394error_t iidc_test_max_size(dc1394camera_t *camera, dc1394video_mode_t mode, uint32_t *width, uint32_t *height) {
	*width = 64;
	*height = 48;
	return DC1394_SUCCESS;
}

dc1394error_t iidc_test_unit_size(dc1394camera_t *camera, dc1394video_mode_t mode, uint32_t *width, uint32_t *height) {
	*width = 8;
	*height = 4;
	return DC1394_SUCCESS;
}

dc1394error_t iidc_test_set_mode(dc1394camera_t *camera, dc1394video_mode_t mode) {
	if (atomic_load(&fail_call) && !strcmp(atomic_load(&fail_call), "set_mode")) {
		return DC1394_FAILURE;
	}
	fake_camera *state = fake(camera);
	state->mode = mode;
	if (mode == DC1394_VIDEO_MODE_FORMAT7_0) {
		state->left = state->top = 0;
		state->width = 64;
		state->height = 48;
	}
	return DC1394_SUCCESS;
}

dc1394error_t iidc_test_set_coding(dc1394camera_t *camera, dc1394video_mode_t mode, dc1394color_coding_t coding) {
	fake_camera *state = fake(camera);
	if (state->left != 0 || state->top != 0 || state->width != 64 || state->height != 48) {
		return DC1394_FAILURE;
	}
	state->coding = coding;
	return DC1394_SUCCESS;
}

dc1394error_t iidc_test_feature_get(dc1394camera_t *camera, dc1394feature_info_t *info) {
	if (info->id != DC1394_FEATURE_SHUTTER && info->id != DC1394_FEATURE_GAIN && info->id != DC1394_FEATURE_GAMMA) {
		return DC1394_FAILURE;
	}
	info->available = DC1394_TRUE;
	info->on_off_capable = DC1394_TRUE;
	info->is_on = DC1394_ON;
	info->current_mode = DC1394_FEATURE_MODE_MANUAL;
	info->abs_control = DC1394_ON;
	info->abs_min = info->id == DC1394_FEATURE_SHUTTER ? .001 : 0;
	info->abs_max = info->id == DC1394_FEATURE_SHUTTER ? 10 : 100;
	info->abs_value = info->id == DC1394_FEATURE_SHUTTER ? .01 : 10;
	return DC1394_SUCCESS;
}

dc1394error_t iidc_test_set_absolute(dc1394camera_t *camera, dc1394feature_t feature, float value) {
	const char *failure = atomic_load(&fail_call);
	if (failure && ((feature == DC1394_FEATURE_SHUTTER && !strcmp(failure, "shutter")) || (feature == DC1394_FEATURE_GAIN && !strcmp(failure, "gain")) || (feature == DC1394_FEATURE_GAMMA && !strcmp(failure, "gamma")))) {
		return DC1394_FAILURE;
	}
	fake_camera *state = fake(camera);
	if (feature == DC1394_FEATURE_SHUTTER) state->shutter = value;
	if (feature == DC1394_FEATURE_GAIN) state->gain = value;
	if (feature == DC1394_FEATURE_GAMMA) state->gamma = value;
	return DC1394_SUCCESS;
}

dc1394error_t iidc_test_feature_power(dc1394camera_t *camera, dc1394feature_t feature, dc1394switch_t value) { return DC1394_SUCCESS; }
dc1394error_t iidc_test_feature_mode(dc1394camera_t *camera, dc1394feature_t feature, dc1394feature_mode_t mode) { return DC1394_SUCCESS; }
dc1394error_t iidc_test_feature_absolute(dc1394camera_t *camera, dc1394feature_t feature, dc1394switch_t value) { return DC1394_SUCCESS; }

dc1394error_t iidc_test_feature_present(dc1394camera_t *camera, dc1394feature_t feature, dc1394bool_t *present) {
	*present = fake(camera)->temperature;
	return DC1394_SUCCESS;
}

dc1394error_t iidc_test_temperature(dc1394camera_t *camera, uint32_t *target, uint32_t *temperature) {
	if (atomic_load(&fail_call) && !strcmp(atomic_load(&fail_call), "temperature")) return DC1394_FAILURE;
	*target = *temperature = 2932;
	return DC1394_SUCCESS;
}

dc1394error_t iidc_test_operation_mode(dc1394camera_t *camera, dc1394operation_mode_t mode) { return DC1394_SUCCESS; }
dc1394error_t iidc_test_iso_speed(dc1394camera_t *camera, dc1394speed_t speed) { return DC1394_SUCCESS; }

dc1394error_t iidc_test_position(dc1394camera_t *camera, dc1394video_mode_t mode, uint32_t left, uint32_t top) {
	fake_camera *state = fake(camera);
	if (left + state->width > 64 || top + state->height > 48) {
		return DC1394_FAILURE;
	}
	state->left = left;
	state->top = top;
	return DC1394_SUCCESS;
}

dc1394error_t iidc_test_size(dc1394camera_t *camera, dc1394video_mode_t mode, uint32_t width, uint32_t height) {
	fake_camera *state = fake(camera);
	if (state->left + width > 64 || state->top + height > 48) {
		return DC1394_FAILURE;
	}
	state->width = width;
	state->height = height;
	return DC1394_SUCCESS;
}

dc1394error_t iidc_test_packet_size(dc1394camera_t *camera, dc1394video_mode_t mode, uint32_t *size) { *size = 1024; return DC1394_SUCCESS; }
dc1394error_t iidc_test_set_packet(dc1394camera_t *camera, dc1394video_mode_t mode, uint32_t size) { return DC1394_SUCCESS; }

dc1394error_t iidc_test_capture_setup(dc1394camera_t *camera, uint32_t count, uint32_t flags) {
	if (atomic_load(&fail_call) && !strcmp(atomic_load(&fail_call), "capture_setup")) return DC1394_FAILURE;
	fake_camera *state = fake(camera);
	atomic_store(&state->capture, 1);
	atomic_store(&state->polls, 0);
	if (!state->width) state->width = 64;
	if (!state->height) state->height = 48;
	return DC1394_SUCCESS;
}

dc1394error_t iidc_test_transmission(dc1394camera_t *camera, dc1394switch_t value) {
	fake_camera *state = fake(camera);
	atomic_store(&state->streaming, value == DC1394_ON);
	return DC1394_SUCCESS;
}

dc1394error_t iidc_test_one_shot(dc1394camera_t *camera, dc1394switch_t value) { return DC1394_SUCCESS; }

dc1394error_t iidc_test_capture_stop(dc1394camera_t *camera) {
	fake_camera *state = fake(camera);
	atomic_store(&state->capture, 0);
	atomic_fetch_add(&state->stops, 1);
	return DC1394_SUCCESS;
}

dc1394error_t iidc_test_dequeue(dc1394camera_t *camera, dc1394capture_policy_t policy, dc1394video_frame_t **frame) {
	fake_camera *state = fake(camera);
	if (atomic_load(&dequeue_error)) return DC1394_FAILURE;
	if (!atomic_load(&state->capture) || atomic_load(&hold_frames) || atomic_fetch_add(&state->polls, 1) == 0) {
		*frame = NULL;
		return DC1394_SUCCESS;
	}
	uint32_t width = state->mode == DC1394_VIDEO_MODE_FORMAT7_0 ? state->width : 640;
	uint32_t height = state->mode == DC1394_VIDEO_MODE_FORMAT7_0 ? state->height : 480;
	unsigned bpp = state->coding == DC1394_COLOR_CODING_RAW16 ? 16 : 8;
	size_t bytes = (size_t)width * height * bpp / 8;
	for (size_t i = 0; i < bytes; i++) state->pixels[i] = (uint8_t)ccd_test_noise(i, 0);
	memset(&state->frame, 0, sizeof(state->frame));
	state->frame.image = state->pixels;
	state->frame.size[0] = width;
	state->frame.size[1] = height;
	state->frame.image_bytes = atomic_load(&malformed_frame) ? UINT32_MAX : (uint32_t)bytes;
	state->frame.data_depth = bpp;
	state->frame.color_coding = state->coding;
	state->frame.little_endian = DC1394_TRUE;
	*frame = &state->frame;
	atomic_fetch_add(&state->frames, 1);
	atomic_store(&state->polls, 0);
	return DC1394_SUCCESS;
}

dc1394error_t iidc_test_enqueue(dc1394camera_t *camera, dc1394video_frame_t *frame) { return DC1394_SUCCESS; }
dc1394error_t iidc_test_convert(uint8_t *src, uint8_t *dest, uint32_t width, uint32_t height, uint32_t order, dc1394color_coding_t coding, uint32_t bits) { memset(dest, 42, (size_t)width * height * 3); return DC1394_SUCCESS; }
const char *iidc_test_error(dc1394error_t error) { return error == DC1394_SUCCESS ? "success" : "failure"; }
dc1394error_t iidc_test_log(dc1394log_t type, void (*handler)(dc1394log_t type, const char *message, void *user), void *user) { return DC1394_SUCCESS; }

libusb_device *LIBUSB_CALL iidc_test_ref(libusb_device *device) { atomic_fetch_add(&refs, 1); return device; }
void LIBUSB_CALL iidc_test_unref(libusb_device *device) { atomic_fetch_sub(&refs, 1); }
int LIBUSB_CALL iidc_test_descriptor(libusb_device *device, struct libusb_device_descriptor *descriptor) { memset(descriptor, 0, sizeof(*descriptor)); return LIBUSB_SUCCESS; }
void iidc_test_usb_start(void) { }

int LIBUSB_CALL iidc_test_register(libusb_context *context, libusb_hotplug_event events, libusb_hotplug_flag flags, int vendor, int product, int device_class, libusb_hotplug_callback_fn callback, void *data, libusb_hotplug_callback_handle *handle) {
	if (atomic_load(&fail_register)) return LIBUSB_ERROR_OTHER;
	usb_callback = callback;
	*handle = 1;
	callback(NULL, (libusb_device *)usb_tokens, LIBUSB_HOTPLUG_EVENT_DEVICE_ARRIVED, NULL);
	return LIBUSB_SUCCESS;
}

int iidc_test_register_sim(libusb_context *context, libusb_hotplug_event events, libusb_hotplug_flag flags, int vendor, int product, int device_class, libusb_hotplug_callback_fn callback, void *data, libusb_hotplug_callback_handle *handle) { return iidc_test_register(context, events, flags, vendor, product, device_class, callback, data, handle); }
void LIBUSB_CALL iidc_test_deregister(libusb_context *context, libusb_hotplug_callback_handle handle) { usb_callback = NULL; }
int iidc_test_deregister_poll(libusb_context *context, libusb_hotplug_callback_handle handle) { iidc_test_deregister(context, handle); return 0; }

indigo_result iidc_test_attach(indigo_device *device) {
	indigo_result result = indigo_attach_device(device);
	if (result == INDIGO_OK) {
		for (int i = 0; i < CAMERAS; i++) if (!logical[i]) { logical[i] = device; break; }
		atomic_fetch_add(&attached, 1);
	}
	return result;
}

indigo_result iidc_test_detach(indigo_device *device) {
	indigo_result result = indigo_detach_device(device);
	for (int i = 0; i < CAMERAS; i++) if (logical[i] == device) logical[i] = NULL;
	atomic_fetch_sub(&attached, 1);
	return result;
}

void iidc_test_execute_in(indigo_device *device, double delay, indigo_timer_callback callback) { indigo_execute_handler_in(device, delay == 5 ? .02 : delay, callback); }

static indigo_result observe_blob(indigo_client *client, indigo_device *device, indigo_property *property, const char *message) {
	if (!strcmp(property->name, CCD_IMAGE_PROPERTY_NAME) && property->state == INDIGO_OK_STATE && property->items[0].blob.size > 0) {
		indigo_raw_header header = { 0 };
		memcpy(&header, property->items[0].blob.value, sizeof(header));
		if ((header.signature != INDIGO_RAW_MONO8 && header.signature != INDIGO_RAW_MONO16 && header.signature != INDIGO_RAW_RGB24) || header.width <= 0 || header.height <= 0) atomic_fetch_add(&bad_blob, 1);
		atomic_fetch_add(&blobs, 1);
	}
	return simulator_client_update_property(client, device, property, message);
}

static bool wait_atomic(atomic_int *value, int expected) {
	for (int i = 0; i < 200; i++) {
		if (atomic_load(value) == expected) return true;
		indigo_usleep(10000);
	}
	return false;
}

static indigo_result change_numbers(const char *property_name, int count, const char **items, const double *values) {
	indigo_property *cached = find_cached_property(property_name);
	if (!cached) return INDIGO_FAILED;
	indigo_property *request = indigo_copy_property(NULL, cached);
	if (!request) return INDIGO_FAILED;
	for (int i = 0; i < count; i++) {
		for (int j = 0; j < request->count; j++) {
			if (!strcmp(request->items[j].name, items[i])) request->items[j].number.value = request->items[j].number.target = values[i];
		}
	}
	indigo_result result = indigo_change_property(&simulator_test_client, request);
	indigo_release_property(request);
	return result;
}

static void reset_fake(void) {
	memset(cameras, 0, sizeof(cameras));
	memset(logical, 0, sizeof(logical));
	for (int i = 0; i < CAMERAS; i++) {
		if (i == 0) {
			snprintf(cameras[i].model, sizeof(cameras[i].model), "%s", device_name);
		} else {
			snprintf(cameras[i].model, sizeof(cameras[i].model), "IIDC fake %d", i);
		}
		cameras[i].public.guid = 0x1000 + i;
		cameras[i].public.unit = i % 2;
		cameras[i].public.model = cameras[i].model;
		cameras[i].public.bmode_capable = true;
		cameras[i].coding = DC1394_COLOR_CODING_MONO8;
		cameras[i].width = 64;
		cameras[i].height = 48;
	}
	cameras[0].visible = true;
	cameras[0].temperature = true;
	atomic_store(&attached, 0); atomic_store(&refs, 0); atomic_store(&blobs, 0); atomic_store(&bad_blob, 0); atomic_store(&calls_after_free, 0);
	atomic_store(&fail_register, 0); atomic_store(&fail_context, 0); atomic_store(&fail_enumerate, 0); atomic_store(&dequeue_error, 0); atomic_store(&malformed_frame, 0); atomic_store(&hold_frames, 0); atomic_store(&fail_call, NULL);
}

static bool begin(bool connect) {
	reset_fake();
	reset_simulator_context(&iidc_case);
	simulator_test_client.update_property = observe_blob;
	if (indigo_start() != INDIGO_OK || indigo_attach_client(&simulator_test_client) != INDIGO_OK || indigo_ccd_iidc(INDIGO_DRIVER_INIT, NULL) != INDIGO_OK || !wait_atomic(&attached, 1)) return false;
	enumerate_simulator_device();
	if (connect) {
		indigo_change_switch_property_1(&simulator_test_client, device_name, CONNECTION_PROPERTY_NAME, CONNECTION_CONNECTED_ITEM_NAME, true);
		return wait_for_simulator_connection_state(true);
	}
	return true;
}

static void end(void) {
	if (context.connected) {
		indigo_change_switch_property_1(&simulator_test_client, device_name, CONNECTION_PROPERTY_NAME, CONNECTION_DISCONNECTED_ITEM_NAME, true);
		wait_for_simulator_connection_state(false);
	}
	indigo_ccd_iidc(INDIGO_DRIVER_SHUTDOWN, NULL);
	indigo_detach_client(&simulator_test_client);
	indigo_stop();
	release_cached_properties();
}

static void metadata_and_property_contract(void) {
	assert_simulator_driver_info(&iidc_case);
	ASSERT_TRUE(begin(true));
	assert_device_interface(INDIGO_INTERFACE_CCD);
	ASSERT_STREQ("Atik GP fake", find_cached_item(INFO_PROPERTY_NAME, INFO_DEVICE_MODEL_ITEM_NAME)->text.value);
	ASSERT_STREQ("0000000000001000-0", find_cached_item(INFO_PROPERTY_NAME, INFO_DEVICE_SERIAL_NUM_ITEM_NAME)->text.value);
	assert_property_has_item(CCD_INFO_PROPERTY_NAME, CCD_INFO_WIDTH_ITEM_NAME);
	assert_property_has_item(CCD_EXPOSURE_PROPERTY_NAME, CCD_EXPOSURE_ITEM_NAME);
	assert_property_has_item(CCD_ABORT_EXPOSURE_PROPERTY_NAME, CCD_ABORT_EXPOSURE_ITEM_NAME);
	assert_property_has_item(CCD_FRAME_PROPERTY_NAME, CCD_FRAME_WIDTH_ITEM_NAME);
	assert_property_has_item(CCD_STREAMING_PROPERTY_NAME, CCD_STREAMING_COUNT_ITEM_NAME);
	ASSERT_TRUE(find_cached_property(CCD_GAIN_PROPERTY_NAME) != NULL);
	ASSERT_TRUE(find_cached_property(CCD_GAMMA_PROPERTY_NAME) != NULL);
	ASSERT_TRUE(find_cached_property(CCD_TEMPERATURE_PROPERTY_NAME) != NULL);
	ASSERT_TRUE(find_cached_property(CCD_BIN_PROPERTY_NAME) == NULL);
	ASSERT_EQ_INT(4, find_cached_property(CCD_MODE_PROPERTY_NAME)->count);
	end();
}

static void exposure_and_image_contract(void) {
	ASSERT_TRUE(begin(true));
	ASSERT_EQ_INT(INDIGO_OK, indigo_change_switch_property_1(&simulator_test_client, device_name, CCD_IMAGE_FORMAT_PROPERTY_NAME, CCD_IMAGE_FORMAT_RAW_ITEM_NAME, true));
	unsigned revision = property_revision(CCD_EXPOSURE_PROPERTY_NAME);
	ASSERT_EQ_INT(INDIGO_OK, indigo_change_number_property_1(&simulator_test_client, device_name, CCD_EXPOSURE_PROPERTY_NAME, CCD_EXPOSURE_ITEM_NAME, .01));
	ASSERT_TRUE(wait_for_property_state_seen_after(CCD_EXPOSURE_PROPERTY_NAME, INDIGO_BUSY_STATE, revision));
	ASSERT_TRUE(wait_for_property_state_after(CCD_EXPOSURE_PROPERTY_NAME, INDIGO_OK_STATE, revision));
	ASSERT_EQ_INT(1, blobs);
	ASSERT_EQ_INT(0, bad_blob);
	end();
}

static void format7_roi_and_raw16(void) {
	ASSERT_TRUE(begin(true));
	ASSERT_EQ_INT(INDIGO_OK, indigo_change_switch_property_1(&simulator_test_client, device_name, CCD_MODE_PROPERTY_NAME, "MODE_1", true));
	ASSERT_TRUE(wait_for_property_not_busy(CCD_MODE_PROPERTY_NAME));
	const char *frame_items[] = { CCD_FRAME_LEFT_ITEM_NAME, CCD_FRAME_TOP_ITEM_NAME, CCD_FRAME_WIDTH_ITEM_NAME, CCD_FRAME_HEIGHT_ITEM_NAME };
	const double frame_values[] = { 9, 5, 55, 43 };
	ASSERT_EQ_INT(INDIGO_OK, change_numbers(CCD_FRAME_PROPERTY_NAME, 4, frame_items, frame_values));
	ASSERT_TRUE(wait_for_property_state(CCD_FRAME_PROPERTY_NAME, INDIGO_OK_STATE));
	ASSERT_NEAR(8, cached_number_value(CCD_FRAME_PROPERTY_NAME, CCD_FRAME_LEFT_ITEM_NAME), 0);
	ASSERT_NEAR(4, cached_number_value(CCD_FRAME_PROPERTY_NAME, CCD_FRAME_TOP_ITEM_NAME), 0);
	indigo_change_switch_property_1(&simulator_test_client, device_name, CCD_IMAGE_FORMAT_PROPERTY_NAME, CCD_IMAGE_FORMAT_RAW_ITEM_NAME, true);
	indigo_change_number_property_1(&simulator_test_client, device_name, CCD_EXPOSURE_PROPERTY_NAME, CCD_EXPOSURE_ITEM_NAME, .01);
	ASSERT_TRUE(wait_for_property_state(CCD_EXPOSURE_PROPERTY_NAME, INDIGO_OK_STATE));
	ASSERT_EQ_INT(1, blobs);
	ASSERT_EQ_INT(DC1394_COLOR_CODING_RAW16, cameras[0].coding);
	ASSERT_EQ_INT(INDIGO_OK, indigo_change_switch_property_1(&simulator_test_client, device_name, CCD_MODE_PROPERTY_NAME, "MODE_0", true));
	ASSERT_TRUE(wait_for_property_not_busy(CCD_MODE_PROPERTY_NAME));
	ASSERT_EQ_INT(DC1394_COLOR_CODING_MONO8, cameras[0].coding);
	end();
}

static void finite_and_aborted_streaming(void) {
	ASSERT_TRUE(begin(true));
	indigo_change_switch_property_1(&simulator_test_client, device_name, CCD_IMAGE_FORMAT_PROPERTY_NAME, CCD_IMAGE_FORMAT_RAW_ITEM_NAME, true);
	const char *stream_items[] = { CCD_STREAMING_EXPOSURE_ITEM_NAME, CCD_STREAMING_COUNT_ITEM_NAME };
	const double finite_values[] = { .01, 3 };
	ASSERT_EQ_INT(INDIGO_OK, change_numbers(CCD_STREAMING_PROPERTY_NAME, 2, stream_items, finite_values));
	ASSERT_TRUE(wait_for_property_state(CCD_STREAMING_PROPERTY_NAME, INDIGO_OK_STATE));
	ASSERT_EQ_INT(3, blobs);
	const double indefinite_values[] = { .01, -1 };
	ASSERT_EQ_INT(INDIGO_OK, change_numbers(CCD_STREAMING_PROPERTY_NAME, 2, stream_items, indefinite_values));
	ASSERT_TRUE(wait_for_property_state(CCD_STREAMING_PROPERTY_NAME, INDIGO_BUSY_STATE));
	ASSERT_EQ_INT(INDIGO_OK, indigo_change_switch_property_1(&simulator_test_client, device_name, CCD_ABORT_EXPOSURE_PROPERTY_NAME, CCD_ABORT_EXPOSURE_ITEM_NAME, true));
	ASSERT_TRUE(wait_for_property_not_busy(CCD_STREAMING_PROPERTY_NAME));
	end();
}

static void sdk_failures_and_recovery(void) {
	ASSERT_TRUE(begin(true));
	atomic_store(&fail_call, "capture_setup");
	indigo_change_number_property_1(&simulator_test_client, device_name, CCD_EXPOSURE_PROPERTY_NAME, CCD_EXPOSURE_ITEM_NAME, .01);
	ASSERT_TRUE(wait_for_property_state(CCD_EXPOSURE_PROPERTY_NAME, INDIGO_ALERT_STATE));
	atomic_store(&fail_call, NULL);
	indigo_change_number_property_1(&simulator_test_client, device_name, CCD_EXPOSURE_PROPERTY_NAME, CCD_EXPOSURE_ITEM_NAME, .01);
	ASSERT_TRUE(wait_for_property_state(CCD_EXPOSURE_PROPERTY_NAME, INDIGO_OK_STATE));
	atomic_store(&malformed_frame, 1);
	indigo_change_number_property_1(&simulator_test_client, device_name, CCD_EXPOSURE_PROPERTY_NAME, CCD_EXPOSURE_ITEM_NAME, .01);
	ASSERT_TRUE(wait_for_property_state(CCD_EXPOSURE_PROPERTY_NAME, INDIGO_ALERT_STATE));
	atomic_store(&malformed_frame, 0);
	atomic_store(&dequeue_error, 1);
	indigo_change_number_property_1(&simulator_test_client, device_name, CCD_EXPOSURE_PROPERTY_NAME, CCD_EXPOSURE_ITEM_NAME, .01);
	ASSERT_TRUE(wait_for_property_state(CCD_EXPOSURE_PROPERTY_NAME, INDIGO_ALERT_STATE));
	end();
}

static void controls_and_temperature_recovery(void) {
	ASSERT_TRUE(begin(true));
	ASSERT_EQ_INT(INDIGO_OK, indigo_change_number_property_1(&simulator_test_client, device_name, CCD_GAIN_PROPERTY_NAME, CCD_GAIN_ITEM_NAME, 23));
	ASSERT_TRUE(wait_for_property_state(CCD_GAIN_PROPERTY_NAME, INDIGO_OK_STATE));
	ASSERT_NEAR(23, cameras[0].gain, .01);
	atomic_store(&fail_call, "gamma");
	indigo_change_number_property_1(&simulator_test_client, device_name, CCD_GAMMA_PROPERTY_NAME, CCD_GAMMA_ITEM_NAME, 12);
	ASSERT_TRUE(wait_for_property_state(CCD_GAMMA_PROPERTY_NAME, INDIGO_ALERT_STATE));
	atomic_store(&fail_call, "temperature");
	ASSERT_TRUE(wait_for_property_state(CCD_TEMPERATURE_PROPERTY_NAME, INDIGO_ALERT_STATE));
	atomic_store(&fail_call, NULL);
	ASSERT_TRUE(wait_for_property_state(CCD_TEMPERATURE_PROPERTY_NAME, INDIGO_OK_STATE));
	end();
}

static void yuv_and_legacy_modes(void) {
	ASSERT_TRUE(begin(true));
	indigo_change_switch_property_1(&simulator_test_client, device_name, CCD_IMAGE_FORMAT_PROPERTY_NAME, CCD_IMAGE_FORMAT_RAW_ITEM_NAME, true);
	ASSERT_EQ_INT(INDIGO_OK, indigo_change_switch_property_1(&simulator_test_client, device_name, CCD_MODE_PROPERTY_NAME, "MODE_2", true));
	ASSERT_TRUE(wait_for_property_not_busy(CCD_MODE_PROPERTY_NAME));
	indigo_change_number_property_1(&simulator_test_client, device_name, CCD_EXPOSURE_PROPERTY_NAME, CCD_EXPOSURE_ITEM_NAME, .01);
	ASSERT_TRUE(wait_for_property_state(CCD_EXPOSURE_PROPERTY_NAME, INDIGO_OK_STATE));
	ASSERT_EQ_INT(1, blobs);
	ASSERT_EQ_INT(0, bad_blob);
	ASSERT_EQ_INT(INDIGO_OK, indigo_change_switch_property_1(&simulator_test_client, device_name, CCD_MODE_PROPERTY_NAME, "MODE_3", true));
	ASSERT_TRUE(wait_for_property_not_busy(CCD_MODE_PROPERTY_NAME));
	ASSERT_EQ_INT(INDIGO_OK, indigo_change_number_property_1(&simulator_test_client, device_name, CCD_FRAME_PROPERTY_NAME, CCD_FRAME_WIDTH_ITEM_NAME, 320));
	ASSERT_TRUE(wait_for_property_state(CCD_FRAME_PROPERTY_NAME, INDIGO_ALERT_STATE));
	indigo_change_number_property_1(&simulator_test_client, device_name, CCD_EXPOSURE_PROPERTY_NAME, CCD_EXPOSURE_ITEM_NAME, .01);
	ASSERT_TRUE(wait_for_property_state(CCD_EXPOSURE_PROPERTY_NAME, INDIGO_OK_STATE));
	end();
}

static void busy_overlap_and_disconnect(void) {
	ASSERT_TRUE(begin(true));
	atomic_store(&hold_frames, 1);
	indigo_change_number_property_1(&simulator_test_client, device_name, CCD_EXPOSURE_PROPERTY_NAME, CCD_EXPOSURE_ITEM_NAME, .2);
	ASSERT_TRUE(wait_for_property_state(CCD_EXPOSURE_PROPERTY_NAME, INDIGO_BUSY_STATE));
	ASSERT_EQ_INT(INDIGO_OK, indigo_change_switch_property_1(&simulator_test_client, device_name, CCD_MODE_PROPERTY_NAME, "MODE_1", true));
	indigo_usleep(50000);
	ASSERT_EQ_INT(DC1394_VIDEO_MODE_FORMAT7_0, cameras[0].mode);
	ASSERT_EQ_INT(INDIGO_OK, indigo_change_number_property_1(&simulator_test_client, device_name, CCD_EXPOSURE_PROPERTY_NAME, CCD_EXPOSURE_ITEM_NAME, .3));
	ASSERT_EQ_INT(INDIGO_OK, indigo_change_switch_property_1(&simulator_test_client, device_name, CONNECTION_PROPERTY_NAME, CONNECTION_DISCONNECTED_ITEM_NAME, true));
	ASSERT_TRUE(wait_for_simulator_connection_state(false));
	ASSERT_EQ_INT(0, cameras[0].capture);
	atomic_store(&hold_frames, 0);
	ASSERT_EQ_INT(0, calls_after_free);
	end();
}

static void active_removal_and_recovery(void) {
	ASSERT_TRUE(begin(true));
	atomic_store(&hold_frames, 1);
	indigo_change_number_property_1(&simulator_test_client, device_name, CCD_EXPOSURE_PROPERTY_NAME, CCD_EXPOSURE_ITEM_NAME, .2);
	ASSERT_TRUE(wait_for_property_state(CCD_EXPOSURE_PROPERTY_NAME, INDIGO_BUSY_STATE));
	cameras[0].visible = false;
	usb_callback(NULL, (libusb_device *)usb_tokens, LIBUSB_HOTPLUG_EVENT_DEVICE_LEFT, NULL);
	ASSERT_TRUE(wait_atomic(&attached, 0));
	ASSERT_EQ_INT(0, cameras[0].capture);
	ASSERT_EQ_INT(0, calls_after_free);
	cameras[0].visible = true;
	atomic_store(&hold_frames, 0);
	usb_callback(NULL, (libusb_device *)usb_tokens, LIBUSB_HOTPLUG_EVENT_DEVICE_ARRIVED, NULL);
	ASSERT_TRUE(wait_atomic(&attached, 1));
	context.connected = false;
	context.disconnected = true;
	end();
}

static void hotplug_identity_and_inconclusive_removal(void) {
	ASSERT_TRUE(begin(false));
	usb_callback(NULL, (libusb_device *)usb_tokens, LIBUSB_HOTPLUG_EVENT_DEVICE_ARRIVED, NULL);
	indigo_usleep(100000);
	ASSERT_EQ_INT(1, attached);
	atomic_store(&fail_enumerate, 1);
	usb_callback(NULL, (libusb_device *)usb_tokens, LIBUSB_HOTPLUG_EVENT_DEVICE_LEFT, NULL);
	indigo_usleep(100000);
	ASSERT_EQ_INT(1, attached);
	atomic_store(&fail_enumerate, 0);
	cameras[0].visible = false;
	usb_callback(NULL, (libusb_device *)usb_tokens, LIBUSB_HOTPLUG_EVENT_DEVICE_LEFT, NULL);
	ASSERT_TRUE(wait_atomic(&attached, 0));
	cameras[0].visible = true;
	usb_callback(NULL, (libusb_device *)usb_tokens, LIBUSB_HOTPLUG_EVENT_DEVICE_ARRIVED, NULL);
	ASSERT_TRUE(wait_atomic(&attached, 1));
	end();
}

static void multiple_devices_and_capacity(void) {
	ASSERT_TRUE(begin(false));
	for (int i = 1; i < CAMERAS; i++) {
		cameras[i].visible = true;
		usb_callback(NULL, (libusb_device *)(usb_tokens + i), LIBUSB_HOTPLUG_EVENT_DEVICE_ARRIVED, NULL);
	}
	ASSERT_TRUE(wait_atomic(&attached, 5));
	cameras[4].visible = false;
	usb_callback(NULL, (libusb_device *)(usb_tokens + 4), LIBUSB_HOTPLUG_EVENT_DEVICE_LEFT, NULL);
	ASSERT_TRUE(wait_atomic(&attached, 4));
	usb_callback(NULL, (libusb_device *)(usb_tokens + 11), LIBUSB_HOTPLUG_EVENT_DEVICE_ARRIVED, NULL);
	ASSERT_TRUE(wait_atomic(&attached, 5));
	end();
}

static void initialization_and_registration_rollback(void) {
	reset_fake();
	reset_simulator_context(&iidc_case);
	indigo_start();
	atomic_store(&fail_context, 1);
	ASSERT_EQ_INT(INDIGO_OK, indigo_ccd_iidc(INDIGO_DRIVER_INIT, NULL));
	indigo_usleep(100000);
	ASSERT_EQ_INT(0, attached);
	ASSERT_EQ_INT(INDIGO_OK, indigo_ccd_iidc(INDIGO_DRIVER_SHUTDOWN, NULL));
	atomic_store(&fail_context, 0);
	atomic_store(&fail_register, 1);
	ASSERT_EQ_INT(INDIGO_FAILED, indigo_ccd_iidc(INDIGO_DRIVER_INIT, NULL));
	atomic_store(&fail_register, 0);
	ASSERT_EQ_INT(INDIGO_OK, indigo_ccd_iidc(INDIGO_DRIVER_INIT, NULL));
	ASSERT_TRUE(wait_atomic(&attached, 1));
	indigo_ccd_iidc(INDIGO_DRIVER_SHUTDOWN, NULL);
	indigo_stop();
}

static void connection_initialization_failure(void) {
	ASSERT_TRUE(begin(false));
	atomic_store(&fail_call, "modes");
	indigo_change_switch_property_1(&simulator_test_client, device_name, CONNECTION_PROPERTY_NAME, CONNECTION_CONNECTED_ITEM_NAME, true);
	ASSERT_TRUE(wait_for_property_state(CONNECTION_PROPERTY_NAME, INDIGO_ALERT_STATE));
	atomic_store(&fail_call, NULL);
	indigo_change_switch_property_1(&simulator_test_client, device_name, CONNECTION_PROPERTY_NAME, CONNECTION_CONNECTED_ITEM_NAME, true);
	ASSERT_TRUE(wait_for_simulator_connection_state(true));
	end();
}

static void rejected_shutdown_and_reconnect(void) {
	ASSERT_TRUE(begin(true));
	ASSERT_EQ_INT(INDIGO_BUSY, indigo_ccd_iidc(INDIGO_DRIVER_SHUTDOWN, NULL));
	indigo_change_number_property_1(&simulator_test_client, device_name, CCD_EXPOSURE_PROPERTY_NAME, CCD_EXPOSURE_ITEM_NAME, .01);
	ASSERT_TRUE(wait_for_property_state(CCD_EXPOSURE_PROPERTY_NAME, INDIGO_OK_STATE));
	indigo_change_switch_property_1(&simulator_test_client, device_name, CONNECTION_PROPERTY_NAME, CONNECTION_DISCONNECTED_ITEM_NAME, true);
	ASSERT_TRUE(wait_for_simulator_connection_state(false));
	indigo_change_switch_property_1(&simulator_test_client, device_name, CONNECTION_PROPERTY_NAME, CONNECTION_CONNECTED_ITEM_NAME, true);
	ASSERT_TRUE(wait_for_simulator_connection_state(true));
	end();
}

int main(int argc, char **argv) {
	setvbuf(stdout, NULL, _IONBF, 0);
	const indigo_test_case tests[] = {
		{ "Metadata and property contract", metadata_and_property_contract },
		{ "Exposure and image contract", exposure_and_image_contract },
		{ "Format7 ROI and RAW16", format7_roi_and_raw16 },
		{ "Finite and aborted streaming", finite_and_aborted_streaming },
		{ "SDK failures and recovery", sdk_failures_and_recovery },
		{ "Controls and temperature recovery", controls_and_temperature_recovery },
		{ "YUV and legacy modes", yuv_and_legacy_modes },
		{ "Busy overlap and disconnect", busy_overlap_and_disconnect },
		{ "Active removal and recovery", active_removal_and_recovery },
		{ "Hotplug identity and inconclusive removal", hotplug_identity_and_inconclusive_removal },
		{ "Multiple devices and capacity", multiple_devices_and_capacity },
		{ "Initialization and registration rollback", initialization_and_registration_rollback },
		{ "Connection initialization failure", connection_initialization_failure },
		{ "Rejected shutdown and reconnect", rejected_shutdown_and_reconnect }
	};
	int result = 0, matched = 0;
	for (int i = 0; i < ARRAY_SIZE(tests); i++) {
		if (argc < 2 || strstr(tests[i].name, argv[1])) { matched++; result |= indigo_run_tests("IIDC fake SDK", tests + i, 1); }
	}
	return matched ? result : 1;
}
