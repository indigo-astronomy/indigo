// Copyright (c) 2016 CloudMakers, s. r. o.
// All rights reserved.
//
// You can use this software under the terms of 'INDIGO Astronomy
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

// version history
// 2.0 Build 0 - PoC by Peter Polakovic <peter.polakovic@cloudmakers.eu>

/** INDIGO IIDC CCD driver
 \file indigo_ccd_iidc.c
 */

#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <math.h>
#include <assert.h>
#include <pthread.h>
#include <sys/time.h>

#if defined(INDIGO_FREEBSD)
#include <libusb.h>
#else
#include <libusb-1.0/libusb.h>
#endif

#include <dc1394/dc1394.h>

#include "indigo_driver_xml.h"

#include "indigo_ccd_iidc.h"

#undef INDIGO_DRIVER_DEBUG
#define INDIGO_DRIVER_DEBUG(c) c

#undef PRIVATE_DATA
#define PRIVATE_DATA        ((iidc_private_data *)DEVICE_CONTEXT->private_data)

char *MODE_NAME[] = {
	"YUV 4:4:4 160x120",
	"YUV 4:2:2 320x240",
	"YUV 4:1:1 640x480",
	"YUV 4:2:2 640x480",
	"RGB 8 640x480",
	"MONO 8 640x480",
	"MONO 16 640x480",
	"YUV 4:2:2 800x600",
	"RGB 8 800x600",
	"MONO 8 800x600",
	"YUV 4:2:2 1024x768",
	"RGB 8 1024x768",
	"MONO 8 1024x768",
	"MONO 16 800x600",
	"MONO 16 1024x768",
	"YUV 4:2:2 1280x960",
	"RGB 8 1280x960",
	"MONO 8 1280x960",
	"YUV 4:2:2 1600x1200",
	"RGB 8 1600x1200",
	"MONO 8 1600x1200",
	"MONO 16 1280x960",
	"MONO 16 1600x1200"
};

typedef struct {
	dc1394camera_t *camera;
	uint64_t guid;
	bool present;
	int device_count;
	indigo_timer *exposure_timer, *temperture_timer;
	double exposure_time;
	unsigned width, height, bits_per_pixel;
	double pixel_width, pixel_height;
	unsigned char *buffer;
} iidc_private_data;

// -------------------------------------------------------------------------------- INDIGO CCD device implementation

static void exposure_timer_callback(indigo_device *device) {
	if (CCD_EXPOSURE_PROPERTY->state == INDIGO_BUSY_STATE) {
		dc1394video_frame_t *frame;
		dc1394error_t err = dc1394_capture_dequeue(PRIVATE_DATA->camera, DC1394_CAPTURE_POLICY_WAIT, &frame);
		INDIGO_LOG(indigo_log("dc1394_capture_dequeue() -> %d", err));
		if (err == DC1394_SUCCESS) {
			void *data = frame->image;
			if (data != NULL) {
				int width = frame->size[0];
				int height = frame->size[1];
				int size = width*height;
				if (frame->data_depth == 8)
					memcpy(PRIVATE_DATA->buffer + FITS_HEADER_SIZE, data, size);
				else if (frame->data_depth == 16)
					memcpy(PRIVATE_DATA->buffer + FITS_HEADER_SIZE, data, 2 * size);
			}
			CCD_EXPOSURE_ITEM->number.value = 0;
			CCD_EXPOSURE_PROPERTY->state = INDIGO_OK_STATE;
			indigo_update_property(device, CCD_EXPOSURE_PROPERTY, NULL);
			indigo_process_image(device, PRIVATE_DATA->buffer, PRIVATE_DATA->width, PRIVATE_DATA->height, PRIVATE_DATA->exposure_time);
		} else {
			CCD_EXPOSURE_ITEM->number.value = 0;
			CCD_EXPOSURE_PROPERTY->state = INDIGO_ALERT_STATE;
			indigo_update_property(device, CCD_EXPOSURE_PROPERTY, "Exposure failed");
		}
	}
}

static void ccd_temperature_callback(indigo_device *device) {
	uint32_t target_temperature, temperature;
	dc1394error_t err = dc1394_feature_temperature_get_value(PRIVATE_DATA->camera, &target_temperature, &temperature);
	INDIGO_LOG(indigo_log("dc1394_feature_temperature_get_value() -> %d", err));
	if (err == DC1394_SUCCESS) {
		CCD_TEMPERATURE_ITEM->number.value = (temperature & 0xFFF)/10.0-273.15;
		indigo_update_property(device, CCD_TEMPERATURE_PROPERTY, NULL);
		PRIVATE_DATA->temperture_timer = indigo_set_timer(device, 5, ccd_temperature_callback);
	} else {
		PRIVATE_DATA->temperture_timer = NULL;
	}
}

static indigo_result ccd_attach(indigo_device *device) {
	assert(device != NULL);
	assert(device->device_context != NULL);
	iidc_private_data *private_data = device->device_context;
	device->device_context = NULL;
	if (indigo_ccd_attach(device, INDIGO_VERSION) == INDIGO_OK) {
		DEVICE_CONTEXT->private_data = private_data;
		// -------------------------------------------------------------------------------- CCD_INFO, CCD_BIN, CCD_MODE, CCD_FRAME
		dc1394video_modes_t modes;
		dc1394_video_get_supported_modes(PRIVATE_DATA->camera, &modes);
		CCD_MODE_PROPERTY->perm = INDIGO_RW_PERM;
		CCD_MODE_PROPERTY->count = 0;
		for (int i = 0; i < modes.num; i++) {
			dc1394video_mode_t mode = modes.modes[i];
			if (mode < DC1394_VIDEO_MODE_EXIF) {
				CCD_MODE_PROPERTY->count++;
				indigo_init_switch_item(CCD_MODE_ITEM+i, MODE_NAME[mode - DC1394_VIDEO_MODE_160x120_YUV444], MODE_NAME[mode - DC1394_VIDEO_MODE_160x120_YUV444], false);
			}
		}
		// --------------------------------------------------------------------------------
		INDIGO_LOG(indigo_log("%s attached", device->name));
		return indigo_ccd_enumerate_properties(device, NULL, NULL);
	}
	return INDIGO_FAILED;
}

static indigo_result ccd_change_property(indigo_device *device, indigo_client *client, indigo_property *property) {
	assert(device != NULL);
	assert(device->device_context != NULL);
	assert(property != NULL);
	if (indigo_property_match(CONNECTION_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- CONNECTION -> CCD_INFO, CCD_COOLER, CCD_TEMPERATURE
		indigo_property_copy_values(CONNECTION_PROPERTY, property, false);
		if (CONNECTION_CONNECTED_ITEM->sw.value) {
			CONNECTION_PROPERTY->state = INDIGO_BUSY_STATE;
			indigo_update_property(device, CONNECTION_PROPERTY, NULL);
//			if (result) {
//				libiidc_get_resolution(PRIVATE_DATA->camera, &PRIVATE_DATA->width, &PRIVATE_DATA->height, &PRIVATE_DATA->bits_per_pixel);
//				libiidc_get_pixel_size(PRIVATE_DATA->camera, &PRIVATE_DATA->pixel_width, &PRIVATE_DATA->pixel_height);
//				CCD_INFO_BITS_PER_PIXEL_ITEM->number.value = PRIVATE_DATA->bits_per_pixel;
//				CCD_INFO_WIDTH_ITEM->number.value = CCD_FRAME_WIDTH_ITEM->number.value = CCD_FRAME_WIDTH_ITEM->number.max = PRIVATE_DATA->width;
//				CCD_INFO_HEIGHT_ITEM->number.value = CCD_FRAME_HEIGHT_ITEM->number.value = CCD_FRAME_HEIGHT_ITEM->number.max = PRIVATE_DATA->height;
//				CCD_INFO_PIXEL_SIZE_ITEM->number.value = CCD_INFO_PIXEL_WIDTH_ITEM->number.value = round(PRIVATE_DATA->pixel_width * 100)/100;
//				CCD_INFO_PIXEL_HEIGHT_ITEM->number.value = round(PRIVATE_DATA->pixel_height * 100) / 100;
//				PRIVATE_DATA->buffer = malloc(FITS_HEADER_SIZE + 2 * CCD_INFO_WIDTH_ITEM->number.value * CCD_INFO_HEIGHT_ITEM->number.value + 5);
//				assert(PRIVATE_DATA->buffer != NULL);
//				CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
//				ccd_temperature_callback(device);
//			} else {
//				if (PRIVATE_DATA->temperture_timer) {
//					indigo_cancel_timer(device, PRIVATE_DATA->temperture_timer);
//					PRIVATE_DATA->temperture_timer = NULL;
//				}
//				if (PRIVATE_DATA->buffer != NULL) {
//					free(PRIVATE_DATA->buffer);
//					PRIVATE_DATA->buffer = NULL;
//				}
//				CONNECTION_PROPERTY->state = INDIGO_ALERT_STATE;
//				indigo_set_switch(CONNECTION_PROPERTY, CONNECTION_DISCONNECTED_ITEM, true);
//			}
//		} else {
//			if (PRIVATE_DATA->temperture_timer != NULL) {
//				indigo_cancel_timer(device, PRIVATE_DATA->temperture_timer);
//				PRIVATE_DATA->temperture_timer = NULL;
//			}
//			if (PRIVATE_DATA->buffer != NULL) {
//				free(PRIVATE_DATA->buffer);
//				PRIVATE_DATA->buffer = NULL;
//			}
//			libiidc_close(PRIVATE_DATA->camera);
			CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
		}
	} else if (indigo_property_match(CCD_EXPOSURE_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- CCD_EXPOSURE
		indigo_property_copy_values(CCD_EXPOSURE_PROPERTY, property, false);
		CCD_EXPOSURE_PROPERTY->state = INDIGO_BUSY_STATE;
		indigo_update_property(device, CCD_EXPOSURE_PROPERTY, NULL);
//		if (CCD_UPLOAD_MODE_LOCAL_ITEM->sw.value) {
//			CCD_IMAGE_FILE_PROPERTY->state = INDIGO_BUSY_STATE;
//			indigo_update_property(device, CCD_IMAGE_FILE_PROPERTY, NULL);
//		} else {
//			CCD_IMAGE_PROPERTY->state = INDIGO_BUSY_STATE;
//			indigo_update_property(device, CCD_IMAGE_PROPERTY, NULL);
//		}
//		libiidc_set_gain(PRIVATE_DATA->camera, CCD_GAIN_ITEM->number.value);
//		libiidc_set_exposure_time(PRIVATE_DATA->camera, PRIVATE_DATA->exposure_time = CCD_EXPOSURE_ITEM->number.value);
//		libiidc_start(PRIVATE_DATA->camera);
		PRIVATE_DATA->exposure_timer = indigo_set_timer(device, CCD_EXPOSURE_ITEM->number.value, exposure_timer_callback);
	} else if (indigo_property_match(CCD_ABORT_EXPOSURE_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- CCD_ABORT_EXPOSURE
		if (CCD_EXPOSURE_PROPERTY->state == INDIGO_BUSY_STATE) {
//	libiidc_abort_exposure(PRIVATE_DATA->device_context);
			indigo_cancel_timer(device, PRIVATE_DATA->exposure_timer);
		}
		indigo_property_copy_values(CCD_ABORT_EXPOSURE_PROPERTY, property, false);
	} else if (indigo_property_match(CCD_COOLER_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- CCD_COOLER
		indigo_property_copy_values(CCD_COOLER_PROPERTY, property, false);
//		if (CONNECTION_CONNECTED_ITEM->sw.value && !CCD_COOLER_PROPERTY->hidden) {
//			CCD_COOLER_PROPERTY->state = INDIGO_BUSY_STATE;
//			indigo_update_property(device, CCD_COOLER_PROPERTY, NULL);
//		}
		return INDIGO_OK;
	} else if (indigo_property_match(CCD_TEMPERATURE_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- CCD_TEMPERATURE
		return INDIGO_OK;
		// --------------------------------------------------------------------------------
	}
	return indigo_ccd_change_property(device, client, property);
}

static indigo_result ccd_detach(indigo_device *device) {
	assert(device != NULL);
	if (CONNECTION_CONNECTED_ITEM->sw.value)
		indigo_device_disconnect(device);
	INDIGO_LOG(indigo_log("%s detached", device->name));
	return indigo_ccd_detach(device);
}

// -------------------------------------------------------------------------------- hot-plug support

#define MAX_DEVICES                   10

static pthread_mutex_t device_mutex = PTHREAD_MUTEX_INITIALIZER;
static indigo_device *devices[MAX_DEVICES];
static dc1394_t *context;


static int hotplug_callback(libusb_context *ctx, libusb_device *dev, libusb_hotplug_event event, void *user_data) {
	static indigo_device ccd_template = {
		"", NULL, INDIGO_OK, INDIGO_VERSION,
		ccd_attach,
		indigo_ccd_enumerate_properties,
		ccd_change_property,
		ccd_detach
	};
	pthread_mutex_lock(&device_mutex);
	switch (event) {
		case LIBUSB_HOTPLUG_EVENT_DEVICE_ARRIVED: {
			dc1394camera_list_t *list;
			dc1394error_t err=dc1394_camera_enumerate(context, &list);
			INDIGO_DEBUG_DRIVER(indigo_debug("dc1394_camera_enumerate() -> %d (%d)", err, list->num));
			if (err == DC1394_SUCCESS) {
				for (int i = 0; i < list->num; i++) {
					uint64_t guid = list->ids[i].guid;
					for (int j = 0; j < MAX_DEVICES; j++) {
						indigo_device *device = devices[j];
						if (device!= NULL && PRIVATE_DATA->guid == guid) {
							guid = 0;
							break;
						}
					}
					if (guid == 0)
						continue;
					dc1394camera_t * camera = dc1394_camera_new(context, guid);
					if (camera) {
						INDIGO_DEBUG_DRIVER(indigo_debug("Camera %llu detected", list->ids[i].guid));
						INDIGO_DEBUG_DRIVER(indigo_debug("  vendor %s", camera->vendor));
						INDIGO_DEBUG_DRIVER(indigo_debug("  model  %s", camera->model));
						if (strstr(camera->model, "CMLN") != NULL || strstr(camera->model, "FMVU") != NULL) {
							INDIGO_DEBUG_DRIVER(indigo_debug("  forced DC1394_OPERATION_MODE_LEGACY"));
							camera->bmode_capable = false;
						}
						if (camera->bmode_capable) {
							err = dc1394_video_set_operation_mode(camera, DC1394_OPERATION_MODE_1394B);
							INDIGO_DEBUG_DRIVER(indigo_debug("dc1394_video_set_operation_mode() -> %d", err));
							err = dc1394_video_set_iso_speed(camera, DC1394_ISO_SPEED_800);
							INDIGO_DEBUG_DRIVER(indigo_debug("dc1394_video_set_iso_speed() -> %d", err));
						} else {
							err = dc1394_video_set_operation_mode(camera, DC1394_OPERATION_MODE_LEGACY);
							INDIGO_DEBUG_DRIVER(indigo_debug("dc1394_video_set_operation_mode() -> %d", err));
							err = dc1394_video_set_iso_speed(camera, DC1394_ISO_SPEED_400);
							INDIGO_DEBUG_DRIVER(indigo_debug("dc1394_video_set_iso_speed() -> %d", err));
						}
//						INDIGO_DEBUG_DRIVER(dc1394featureset_t features);
//						INDIGO_DEBUG_DRIVER(dc1394_camera_print_info(camera, stderr));
//						INDIGO_DEBUG_DRIVER(dc1394_feature_get_all(camera, &features));
//						INDIGO_DEBUG_DRIVER(dc1394_feature_print_all(&features, stderr));
						iidc_private_data *private_data = malloc(sizeof(iidc_private_data));
						assert(private_data != NULL);
						memset(private_data, 0, sizeof(iidc_private_data));
						private_data->camera = camera;
						private_data->guid = guid;
						libusb_ref_device(dev);
						indigo_device *device = malloc(sizeof(indigo_device));
						assert(device != NULL);
						memcpy(device, &ccd_template, sizeof(indigo_device));
						strcpy(device->name, camera->model);
						device->device_context = private_data;
						for (int j = 0; j < MAX_DEVICES; j++) {
							if (devices[j] == NULL) {
								indigo_async((void *)(void *)indigo_attach_device, devices[j] = device);
								break;
							}
						}
					}
				}
				dc1394_camera_free_list(list);
			}
			break;
		}
		case LIBUSB_HOTPLUG_EVENT_DEVICE_LEFT: {
			for (int j = 0; j < MAX_DEVICES; j++) {
				indigo_device *device = devices[j];
				if (device != NULL)
					PRIVATE_DATA->present = false;
			}
			dc1394camera_list_t *list;
			dc1394error_t err=dc1394_camera_enumerate(context, &list);
			INDIGO_DEBUG_DRIVER(indigo_debug("dc1394_camera_enumerate() -> %d (%d)", err, list->num));
			if (err == DC1394_SUCCESS) {
				for (int i = 0; i < list->num; i++) {
					uint64_t guid = list->ids[i].guid;
					for (int j = 0; j < MAX_DEVICES; j++) {
						indigo_device *device = devices[j];
						if (device != NULL && PRIVATE_DATA->guid == guid) {
							PRIVATE_DATA->present = true;
							break;
						}
					}
				}
				dc1394_camera_free_list(list);
			}
			for (int j = 0; j < MAX_DEVICES; j++) {
				if (devices[j] != NULL) {
					indigo_device *device = devices[j];
					if (!PRIVATE_DATA->present) {
						dc1394_camera_free(PRIVATE_DATA->camera);
						if (PRIVATE_DATA != NULL) {
							free(PRIVATE_DATA);
						}
						indigo_detach_device(device);
						free(device);
						devices[j] = NULL;
					}
				}
			}
			break;
		}
	}
	pthread_mutex_unlock(&device_mutex);
	return 0;
}

static libusb_hotplug_callback_handle callback_handle;

static void errorlog_handler(dc1394log_t type, const char *message, void* user) {
	INDIGO_LOG(indigo_error("libdc1394: %s", message));
}

static void debuglog_handler(dc1394log_t type, const char *message, void* user) {
	INDIGO_DEBUG_DRIVER(indigo_debug("libdc1394: %s", message));
}

indigo_result indigo_ccd_iidc(bool state) {
	static bool current_state = false;
	if (state == current_state)
		return INDIGO_OK;
	if ((current_state = state)) {
		context = dc1394_new();
		if (context != NULL) {
			dc1394_log_register_handler(DC1394_LOG_ERROR, errorlog_handler, NULL);
			dc1394_log_register_handler(DC1394_LOG_WARNING, errorlog_handler, NULL);
			dc1394_log_register_handler(DC1394_LOG_DEBUG, debuglog_handler, NULL);
			for (int i = 0; i < MAX_DEVICES; i++) {
				devices[i] = 0;
			}
			indigo_start_usb_event_handler();
			int rc = libusb_hotplug_register_callback(NULL, LIBUSB_HOTPLUG_EVENT_DEVICE_ARRIVED | LIBUSB_HOTPLUG_EVENT_DEVICE_LEFT, LIBUSB_HOTPLUG_ENUMERATE, LIBUSB_HOTPLUG_MATCH_ANY, LIBUSB_HOTPLUG_MATCH_ANY, LIBUSB_HOTPLUG_MATCH_ANY, hotplug_callback, NULL, &callback_handle);
			INDIGO_DEBUG_DRIVER(indigo_debug("indigo_ccd_iidc: libusb_hotplug_register_callback [%d] ->  %s", __LINE__, rc < 0 ? libusb_error_name(rc) : "OK"));
			return rc >= 0 ? INDIGO_OK : INDIGO_FAILED;
		}
		current_state = false;
		return INDIGO_FAILED;
	} else {
		libusb_hotplug_deregister_callback(NULL, callback_handle);
		INDIGO_DEBUG_DRIVER(indigo_debug("indigo_ccd_iidc: libusb_hotplug_deregister_callback [%d]", __LINE__));
		for (int j = 0; j < MAX_DEVICES; j++) {
			indigo_device *device = devices[j];
			if (PRIVATE_DATA != NULL) {
				free(PRIVATE_DATA);
			}
			indigo_detach_device(device);
			free(device);
			devices[j] = NULL;
		}
		dc1394_free(context);
		context = NULL;
		return INDIGO_OK;
	}
}


