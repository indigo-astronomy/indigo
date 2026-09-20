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

// Physical SSAG/QHY5 acceptance test by OpenAI Codex.

#include <pthread.h>

#include <libusb-1.0/libusb.h>

#include <indigo/indigo_driver.h>
#include <indigo/indigo_usb_utils.h>
#include <indigo_drivers/ccd_ssag/indigo_ccd_ssag.h>

#include "../test_runner.h"

#define CHECK(condition) do { if (!(condition)) { fprintf(stderr, "%s:%d: %s\n", __FILE__, __LINE__, #condition); indigo_test_failures++; goto cleanup; } } while (0)
#define MAX_PROPERTIES 64

typedef struct {
	char name[INDIGO_NAME_SIZE];
	indigo_property *properties[MAX_PROPERTIES];
	unsigned revisions[MAX_PROPERTIES];
	unsigned frames;
	unsigned invalid_frames;
	unsigned nonuniform_frames;
} observed_device;

static observed_device camera, guider;
static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
static libusb_hotplug_callback_fn hardware_callback;
static void *hardware_callback_data;
static bool active_hotplug_only;

int ssag_hw_register(libusb_context *context, libusb_hotplug_event events, libusb_hotplug_flag flags, int vendor, int product, int device_class, libusb_hotplug_callback_fn callback, void *data, libusb_hotplug_callback_handle *handle) {
	int result = libusb_hotplug_register_callback_sim(context, events, (libusb_hotplug_flag)(flags & ~LIBUSB_HOTPLUG_ENUMERATE), vendor, product, device_class, callback, data, handle);
	if (result == LIBUSB_SUCCESS) {
		hardware_callback = callback;
		hardware_callback_data = data;
	}
	return result;
}

int ssag_hw_register_sim(libusb_context *context, libusb_hotplug_event events, libusb_hotplug_flag flags, int vendor, int product, int device_class, libusb_hotplug_callback_fn callback, void *data, libusb_hotplug_callback_handle *handle) {
	return ssag_hw_register(context, events, flags, vendor, product, device_class, callback, data, handle);
}

void ssag_hw_deregister(libusb_context *context, libusb_hotplug_callback_handle handle) {
	hardware_callback = NULL;
	hardware_callback_data = NULL;
	libusb_hotplug_deregister_callback(context, handle);
}

int ssag_hw_deregister_poll(libusb_context *context, libusb_hotplug_callback_handle handle) {
	hardware_callback = NULL;
	hardware_callback_data = NULL;
	return libusb_hotplug_deregister_callback_poll(context, handle);
}

static int property_slot(observed_device *device, const char *name) {
	for (int i = 0; i < MAX_PROPERTIES; i++) {
		if (device->properties[i] != NULL && !strcmp(device->properties[i]->name, name)) {
			return i;
		}
	}
	return -1;
}

static observed_device *observed(const char *name) {
	if (!strcmp(name, "SSAG")) {
		return &camera;
	}
	if (!strcmp(name, "SSAG (guider)")) {
		return &guider;
	}
	return NULL;
}

static indigo_result observe_property(indigo_device *device, indigo_property *property) {
	observed_device *target = observed(property->device);
	if (target == NULL) {
		return INDIGO_OK;
	}
	pthread_mutex_lock(&mutex);
	snprintf(target->name, sizeof(target->name), "%s", property->device);
	int slot = property_slot(target, property->name);
	if (slot < 0) {
		for (slot = 0; slot < MAX_PROPERTIES && target->properties[slot] != NULL; slot++) {
		}
	}
	if (slot < MAX_PROPERTIES) {
		indigo_release_property(target->properties[slot]);
		target->properties[slot] = indigo_copy_property(NULL, property);
		target->revisions[slot]++;
	}
	if (target == &camera && !strcmp(property->name, CCD_IMAGE_PROPERTY_NAME) && property->state == INDIGO_OK_STATE && property->count > 0 && property->items[0].blob.value != NULL) {
		indigo_item *item = property->items;
		indigo_raw_header header = { 0 };
		bool valid = !strcmp(item->blob.format, ".raw") && item->blob.size >= sizeof(header);
		if (valid) {
			memcpy(&header, item->blob.value, sizeof(header));
			valid = header.signature == INDIGO_RAW_MONO8 && header.width == 1280 && header.height == 1024 && item->blob.size >= sizeof(header) + 1280 * 1024;
		}
		if (valid) {
			unsigned char *pixels = (unsigned char *)item->blob.value + sizeof(header);
			unsigned char min = 255, max = 0;
			for (int i = 0; i < 1280 * 1024; i += 97) {
				if (pixels[i] < min) {
					min = pixels[i];
				}
				if (pixels[i] > max) {
					max = pixels[i];
				}
			}
			if (max > min) {
				target->nonuniform_frames++;
			}
		}
		if (!valid) {
			target->invalid_frames++;
		}
		target->frames++;
		printf("    RAW frame %u: %u x %u, %ld bytes\n", target->frames, header.width, header.height, item->blob.size);
	}
	pthread_mutex_unlock(&mutex);
	return INDIGO_OK;
}

static indigo_result define_property(indigo_client *client, indigo_device *device, indigo_property *property, const char *message) {
	return observe_property(device, property);
}

static indigo_result update_property(indigo_client *client, indigo_device *device, indigo_property *property, const char *message) {
	return observe_property(device, property);
}

static indigo_result delete_property(indigo_client *client, indigo_device *device, indigo_property *property, const char *message) {
	observed_device *target = observed(property->device);
	if (target != NULL) {
		pthread_mutex_lock(&mutex);
		for (int i = 0; i < MAX_PROPERTIES; i++) {
			if (target->properties[i] != NULL && (!*property->name || !strcmp(target->properties[i]->name, property->name))) {
				indigo_release_property(target->properties[i]);
				target->properties[i] = NULL;
				target->revisions[i]++;
			}
		}
		pthread_mutex_unlock(&mutex);
	}
	return INDIGO_OK;
}

static indigo_client client = {
	.name = "SSAG physical acceptance",
	.version = INDIGO_VERSION_CURRENT,
	.define_property = define_property,
	.update_property = update_property,
	.delete_property = delete_property
};

static unsigned revision(observed_device *device, const char *name) {
	pthread_mutex_lock(&mutex);
	int slot = property_slot(device, name);
	unsigned result = slot < 0 ? 0 : device->revisions[slot];
	pthread_mutex_unlock(&mutex);
	return result;
}

static bool wait_property(observed_device *device, const char *name, unsigned after, indigo_property_state state) {
	for (int i = 0; i < 3000; i++) {
		pthread_mutex_lock(&mutex);
		int slot = property_slot(device, name);
		bool ready = slot >= 0 && device->revisions[slot] > after && device->properties[slot]->state == state;
		pthread_mutex_unlock(&mutex);
		if (ready) {
			return true;
		}
		indigo_usleep(10000);
	}
	return false;
}

static bool initialized_camera_present(void) {
	libusb_device **devices = NULL;
	ssize_t count = libusb_get_device_list(NULL, &devices);
	bool present = false;
	for (ssize_t i = 0; i < count && !present; i++) {
		struct libusb_device_descriptor descriptor;
		present = libusb_get_device_descriptor(devices[i], &descriptor) == LIBUSB_SUCCESS && descriptor.idVendor == 0x1856 && descriptor.idProduct == 0x0012;
	}
	libusb_free_device_list(devices, true);
	return present;
}

static bool ensure_discovery(void) {
	bool loader_reported = false;
	bool initialized_reported = false;
	for (int i = 0; i < 12000; i++) {
		libusb_device **devices = NULL;
		ssize_t count = libusb_get_device_list(NULL, &devices);
		for (ssize_t index = 0; index < count; index++) {
			struct libusb_device_descriptor descriptor;
			if (libusb_get_device_descriptor(devices[index], &descriptor) == LIBUSB_SUCCESS) {
				bool loader = (descriptor.idVendor == 0x1618 && descriptor.idProduct == 0x0901) || (descriptor.idVendor == 0x1856 && descriptor.idProduct == 0x0011) || (descriptor.idVendor == 0x16c0 && descriptor.idProduct == 0x296d);
				bool initialized = descriptor.idVendor == 0x1856 && descriptor.idProduct == 0x0012;
				if (i >= 400 && hardware_callback != NULL && ((loader && !loader_reported) || (initialized && !initialized_reported))) {
					hardware_callback(NULL, devices[index], LIBUSB_HOTPLUG_EVENT_DEVICE_ARRIVED, hardware_callback_data);
					loader_reported |= loader;
					initialized_reported |= initialized;
				}
			}
		}
		libusb_free_device_list(devices, true);
		if (initialized_camera_present()) {
			pthread_mutex_lock(&mutex);
			bool ready = property_slot(&camera, CONNECTION_PROPERTY_NAME) >= 0 && property_slot(&guider, CONNECTION_PROPERTY_NAME) >= 0;
			pthread_mutex_unlock(&mutex);
			if (ready) {
				return true;
			}
		}
		indigo_usleep(10000);
	}
	return false;
}

static bool wait_removal(void) {
	for (int i = 0; i < 12000; i++) {
		if (!initialized_camera_present()) {
			return true;
		}
		indigo_usleep(10000);
	}
	return false;
}

static void clear_observed_properties(void) {
	pthread_mutex_lock(&mutex);
	for (int i = 0; i < MAX_PROPERTIES; i++) {
		indigo_release_property(camera.properties[i]);
		camera.properties[i] = NULL;
		indigo_release_property(guider.properties[i]);
		guider.properties[i] = NULL;
	}
	pthread_mutex_unlock(&mutex);
}

static bool change_switch(observed_device *device, const char *property, const char *item, indigo_property_state state) {
	unsigned before = revision(device, property);
	return indigo_change_switch_property_1(&client, device->name, property, item, true) == INDIGO_OK && wait_property(device, property, before, state);
}

static bool change_number(observed_device *device, const char *property, const char *item, double value, indigo_property_state state) {
	unsigned before = revision(device, property);
	return indigo_change_number_property_1(&client, device->name, property, item, value) == INDIGO_OK && wait_property(device, property, before, state);
}

static bool connect_with_retry(observed_device *device) {
	for (int attempt = 0; attempt < 10; attempt++) {
		unsigned before = revision(device, CONNECTION_PROPERTY_NAME);
		if (indigo_change_switch_property_1(&client, device->name, CONNECTION_PROPERTY_NAME, CONNECTION_CONNECTED_ITEM_NAME, true) != INDIGO_OK) {
			return false;
		}
		for (int i = 0; i < 500; i++) {
			pthread_mutex_lock(&mutex);
			int slot = property_slot(device, CONNECTION_PROPERTY_NAME);
			bool updated = slot >= 0 && device->revisions[slot] > before;
			bool connected = updated && device->properties[slot]->state == INDIGO_OK_STATE && device->properties[slot]->items[0].sw.value;
			bool finished = updated && device->properties[slot]->state != INDIGO_BUSY_STATE;
			pthread_mutex_unlock(&mutex);
			if (connected) {
				return true;
			}
			if (finished) {
				break;
			}
			indigo_usleep(10000);
		}
		indigo_usleep(500000);
	}
	return false;
}

static unsigned frame_count(void) {
	pthread_mutex_lock(&mutex);
	unsigned result = camera.frames;
	pthread_mutex_unlock(&mutex);
	return result;
}

static bool is_connected(observed_device *device) {
	pthread_mutex_lock(&mutex);
	int slot = property_slot(device, CONNECTION_PROPERTY_NAME);
	bool result = false;
	if (slot >= 0) {
		for (int i = 0; i < device->properties[slot]->count; i++) {
			if (!strcmp(device->properties[slot]->items[i].name, CONNECTION_CONNECTED_ITEM_NAME)) {
				result = device->properties[slot]->items[i].sw.value;
			}
		}
	}
	pthread_mutex_unlock(&mutex);
	return result;
}

static void physical_acceptance(void) {
	bool initialized = false;
	indigo_set_log_level(INDIGO_LOG_DEBUG);
	CHECK(indigo_start() == INDIGO_OK);
	CHECK(indigo_attach_client(&client) == INDIGO_OK);
	indigo_driver_info info;
	CHECK(indigo_ccd_ssag(INDIGO_DRIVER_INFO, &info) == INDIGO_OK);
	CHECK(INDIGO_DRIVER_API_GENERATION(info.version) == INDIGO_DRIVER_API_3);
	CHECK(indigo_ccd_ssag(INDIGO_DRIVER_INIT, NULL) == INDIGO_OK);
	initialized = true;
	CHECK(ensure_discovery());
	printf("    discovered %s and %s\n", camera.name, guider.name);
	CHECK(change_switch(&guider, CONNECTION_PROPERTY_NAME, CONNECTION_CONNECTED_ITEM_NAME, INDIGO_OK_STATE));
	CHECK(change_switch(&camera, CONNECTION_PROPERTY_NAME, CONNECTION_CONNECTED_ITEM_NAME, INDIGO_OK_STATE));
	CHECK(change_switch(&camera, CCD_UPLOAD_MODE_PROPERTY_NAME, CCD_UPLOAD_MODE_CLIENT_ITEM_NAME, INDIGO_OK_STATE));
	CHECK(change_switch(&camera, CCD_IMAGE_FORMAT_PROPERTY_NAME, CCD_IMAGE_FORMAT_RAW_ITEM_NAME, INDIGO_OK_STATE));
	unsigned before = frame_count();
	CHECK(change_number(&camera, CCD_EXPOSURE_PROPERTY_NAME, CCD_EXPOSURE_ITEM_NAME, 0.05, INDIGO_OK_STATE));
	CHECK(frame_count() == before + 1);
	before = frame_count();
	CHECK(change_number(&camera, CCD_EXPOSURE_PROPERTY_NAME, CCD_EXPOSURE_ITEM_NAME, 1.5, INDIGO_OK_STATE));
	CHECK(frame_count() == before + 1);
	CHECK(change_number(&camera, CCD_EXPOSURE_PROPERTY_NAME, CCD_EXPOSURE_ITEM_NAME, 1, INDIGO_BUSY_STATE));
	indigo_usleep(400000);
	CHECK(change_switch(&camera, CCD_ABORT_EXPOSURE_PROPERTY_NAME, CCD_ABORT_EXPOSURE_ITEM_NAME, INDIGO_OK_STATE));
	before = frame_count();
	CHECK(change_number(&camera, CCD_EXPOSURE_PROPERTY_NAME, CCD_EXPOSURE_ITEM_NAME, 0.05, INDIGO_ALERT_STATE));
	CHECK(wait_property(&camera, CCD_EXPOSURE_PROPERTY_NAME, revision(&camera, CCD_EXPOSURE_PROPERTY_NAME), INDIGO_OK_STATE));
	CHECK(frame_count() == before);
	CHECK(change_number(&camera, CCD_EXPOSURE_PROPERTY_NAME, CCD_EXPOSURE_ITEM_NAME, 0.05, INDIGO_OK_STATE));
	CHECK(frame_count() == before + 1);
	const char *properties[] = { GUIDER_GUIDE_RA_PROPERTY_NAME, GUIDER_GUIDE_RA_PROPERTY_NAME, GUIDER_GUIDE_DEC_PROPERTY_NAME, GUIDER_GUIDE_DEC_PROPERTY_NAME };
	const char *items[] = { GUIDER_GUIDE_EAST_ITEM_NAME, GUIDER_GUIDE_WEST_ITEM_NAME, GUIDER_GUIDE_NORTH_ITEM_NAME, GUIDER_GUIDE_SOUTH_ITEM_NAME };
	for (int i = 0; i < 4; i++) {
		CHECK(change_number(&guider, properties[i], items[i], 100, INDIGO_OK_STATE));
	}
	CHECK(change_switch(&camera, CONNECTION_PROPERTY_NAME, CONNECTION_DISCONNECTED_ITEM_NAME, INDIGO_OK_STATE));
	CHECK(change_number(&guider, GUIDER_GUIDE_RA_PROPERTY_NAME, GUIDER_GUIDE_WEST_ITEM_NAME, 80, INDIGO_OK_STATE));
	CHECK(change_switch(&camera, CONNECTION_PROPERTY_NAME, CONNECTION_CONNECTED_ITEM_NAME, INDIGO_OK_STATE));
	CHECK(change_switch(&camera, CCD_IMAGE_FORMAT_PROPERTY_NAME, CCD_IMAGE_FORMAT_RAW_ITEM_NAME, INDIGO_OK_STATE));
	before = frame_count();
	CHECK(change_number(&camera, CCD_EXPOSURE_PROPERTY_NAME, CCD_EXPOSURE_ITEM_NAME, 0.05, INDIGO_OK_STATE));
	CHECK(frame_count() == before + 1);
	CHECK(change_switch(&guider, CONNECTION_PROPERTY_NAME, CONNECTION_DISCONNECTED_ITEM_NAME, INDIGO_OK_STATE));
	before = frame_count();
	CHECK(change_number(&camera, CCD_EXPOSURE_PROPERTY_NAME, CCD_EXPOSURE_ITEM_NAME, 0.05, INDIGO_OK_STATE));
	CHECK(frame_count() == before + 1);
	CHECK(change_switch(&camera, CONNECTION_PROPERTY_NAME, CONNECTION_DISCONNECTED_ITEM_NAME, INDIGO_OK_STATE));
	CHECK(indigo_ccd_ssag(INDIGO_DRIVER_SHUTDOWN, NULL) == INDIGO_OK);
	initialized = false;
	CHECK(indigo_ccd_ssag(INDIGO_DRIVER_INIT, NULL) == INDIGO_OK);
	initialized = true;
	CHECK(ensure_discovery());
	CHECK(change_switch(&camera, CONNECTION_PROPERTY_NAME, CONNECTION_CONNECTED_ITEM_NAME, INDIGO_OK_STATE));
	CHECK(change_switch(&camera, CCD_IMAGE_FORMAT_PROPERTY_NAME, CCD_IMAGE_FORMAT_RAW_ITEM_NAME, INDIGO_OK_STATE));
	before = frame_count();
	CHECK(change_number(&camera, CCD_EXPOSURE_PROPERTY_NAME, CCD_EXPOSURE_ITEM_NAME, 0.05, INDIGO_OK_STATE));
	CHECK(frame_count() == before + 1);
	pthread_mutex_lock(&mutex);
	unsigned invalid = camera.invalid_frames, nonuniform = camera.nonuniform_frames;
	pthread_mutex_unlock(&mutex);
	CHECK(invalid == 0);
	CHECK(nonuniform > 0);
cleanup:
	if (*camera.name && is_connected(&camera)) {
		change_switch(&camera, CONNECTION_PROPERTY_NAME, CONNECTION_DISCONNECTED_ITEM_NAME, INDIGO_OK_STATE);
	}
	if (*guider.name && is_connected(&guider)) {
		change_switch(&guider, CONNECTION_PROPERTY_NAME, CONNECTION_DISCONNECTED_ITEM_NAME, INDIGO_OK_STATE);
	}
	if (initialized) {
		indigo_ccd_ssag(INDIGO_DRIVER_SHUTDOWN, NULL);
	}
	indigo_detach_client(&client);
	indigo_stop();
}

static void physical_hotplug_acceptance(void) {
	bool initialized = false;
	indigo_set_log_level(INDIGO_LOG_DEBUG);
	CHECK(indigo_start() == INDIGO_OK);
	CHECK(indigo_attach_client(&client) == INDIGO_OK);
	CHECK(indigo_ccd_ssag(INDIGO_DRIVER_INIT, NULL) == INDIGO_OK);
	initialized = true;
	CHECK(ensure_discovery());
	CHECK(connect_with_retry(&camera));
	CHECK(connect_with_retry(&guider));
	if (!active_hotplug_only) {
		printf("    ACTION: unplug the SSAG/QHY5 USB cable while idle\n");
		CHECK(wait_removal());
		clear_observed_properties();
		printf("    idle removal detected; ACTION: plug the SSAG/QHY5 USB cable back in\n");
		CHECK(ensure_discovery());
		CHECK(connect_with_retry(&camera));
		CHECK(connect_with_retry(&guider));
	}
	CHECK(change_number(&camera, CCD_EXPOSURE_PROPERTY_NAME, CCD_EXPOSURE_ITEM_NAME, 120, INDIGO_BUSY_STATE));
	indigo_usleep(400000);
	printf("    ACTION: unplug the SSAG/QHY5 USB cable during the active exposure\n");
	CHECK(wait_removal());
	clear_observed_properties();
	printf("    active-exposure removal detected; ACTION: plug the SSAG/QHY5 USB cable back in\n");
	CHECK(ensure_discovery());
	CHECK(connect_with_retry(&camera));
	CHECK(change_switch(&camera, CCD_IMAGE_FORMAT_PROPERTY_NAME, CCD_IMAGE_FORMAT_RAW_ITEM_NAME, INDIGO_OK_STATE));
	unsigned before = frame_count();
	CHECK(change_number(&camera, CCD_EXPOSURE_PROPERTY_NAME, CCD_EXPOSURE_ITEM_NAME, 0.05, INDIGO_OK_STATE));
	CHECK(frame_count() == before + 1);
	CHECK(connect_with_retry(&guider));
	CHECK(change_number(&guider, GUIDER_GUIDE_RA_PROPERTY_NAME, GUIDER_GUIDE_EAST_ITEM_NAME, 100, INDIGO_OK_STATE));
cleanup:
	if (*camera.name && is_connected(&camera)) {
		change_switch(&camera, CONNECTION_PROPERTY_NAME, CONNECTION_DISCONNECTED_ITEM_NAME, INDIGO_OK_STATE);
	}
	if (*guider.name && is_connected(&guider)) {
		change_switch(&guider, CONNECTION_PROPERTY_NAME, CONNECTION_DISCONNECTED_ITEM_NAME, INDIGO_OK_STATE);
	}
	if (initialized) {
		indigo_ccd_ssag(INDIGO_DRIVER_SHUTDOWN, NULL);
	}
	indigo_detach_client(&client);
	indigo_stop();
}

int main(int argc, char **argv) {
	if (argc != 2 || (strcmp(argv[1], "--run") && strcmp(argv[1], "--hotplug-only") && strcmp(argv[1], "--active-hotplug-only"))) {
		fprintf(stderr, "This test operates physical SSAG/QHY5 hardware. Run with --run, --hotplug-only or --active-hotplug-only.\n");
		return 2;
	}
	setvbuf(stdout, NULL, _IONBF, 0);
	if (!strcmp(argv[1], "--hotplug-only") || !strcmp(argv[1], "--active-hotplug-only")) {
		active_hotplug_only = !strcmp(argv[1], "--active-hotplug-only");
		const indigo_test_case hotplug_test[] = { { "SSAG physical idle and active hot-plug", physical_hotplug_acceptance } };
		return indigo_run_tests("SSAG hardware hot-plug", hotplug_test, 1);
	}
	const indigo_test_case tests[] = {
		{ "SSAG physical camera acceptance", physical_acceptance },
		{ "SSAG physical idle and active hot-plug", physical_hotplug_acceptance }
	};
	int result = indigo_run_tests("SSAG hardware", tests, sizeof(tests) / sizeof(tests[0]));
	for (int i = 0; i < MAX_PROPERTIES; i++) {
		indigo_release_property(camera.properties[i]);
		indigo_release_property(guider.properties[i]);
	}
	return result;
}
