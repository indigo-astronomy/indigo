// Copyright (c) 2019 CloudMakers, s. r. o.
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
// 2.0 by Peter Polakovic <peter.polakovic@cloudmakers.eu>

/** INDIGO UVC CCD driver
 \file indigo_ccd_uvc.c
 */

#define DRIVER_VERSION 0x000C
#define DRIVER_NAME "indigo_ccd_uvc"

#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <math.h>
#include <assert.h>
#include <pthread.h>
#include <sys/time.h>

#if defined(INDIGO_MACOS)
#include <libusb-1.0/libusb.h>
#elif defined(INDIGO_FREEBSD)
#include <libusb.h>
#else
#include <libusb-1.0/libusb.h>
#endif

#include "libuvc.h"

#include <indigo/indigo_driver_xml.h>
#include <indigo/indigo_usb_utils.h>

#include "indigo_ccd_uvc.h"

#define PRIVATE_DATA        ((uvc_private_data *)device->private_data)

#define UVC_CTRL_FLAG_SET_CUR (1 << 0)
#define UVC_CTRL_FLAG_GET_CUR (1 << 1)
#define UVC_CTRL_FLAG_GET_MIN (1 << 2)
#define UVC_CTRL_FLAG_GET_MAX (1 << 3)
#define UVC_CTRL_FLAG_GET_RES (1 << 4)
#define UVC_CTRL_FLAG_GET_DEF (1 << 5)


uvc_context_t *uvc_ctx;

typedef struct {
	uvc_device_t *dev;
	uvc_device_handle_t *handle;
	enum uvc_frame_format format;
	uvc_stream_ctrl_t ctrl;
	uvc_stream_handle_t *strmhp;
	char *buffer;
} uvc_private_data;

// -------------------------------------------------------------------------------- INDIGO CCD device implementation

static void exposure_callback(indigo_device *device) {
	uvc_frame_t *frame = NULL;
	uvc_error_t res = UVC_ERROR_TIMEOUT;
	while (res == UVC_ERROR_TIMEOUT && frame == NULL && CCD_EXPOSURE_PROPERTY->state == INDIGO_BUSY_STATE) {
		res = uvc_stream_get_frame(PRIVATE_DATA->strmhp, &frame, 1000);
	}
	INDIGO_DRIVER_DEBUG(DRIVER_NAME, "uvc_stream_get_frame(...) -> %s", uvc_strerror(res));
	if (res != UVC_SUCCESS || frame == NULL) {
		CCD_EXPOSURE_PROPERTY->state = INDIGO_ALERT_STATE;
	} else if (frame->frame_format == UVC_FRAME_FORMAT_GRAY8 || frame->frame_format == UVC_FRAME_FORMAT_BY8 || frame->frame_format == UVC_FRAME_FORMAT_BA81 || frame->frame_format == UVC_FRAME_FORMAT_SGRBG8 || frame->frame_format == UVC_FRAME_FORMAT_SGBRG8 || frame->frame_format == UVC_FRAME_FORMAT_SRGGB8 || frame->frame_format == UVC_FRAME_FORMAT_SBGGR8) {
		memcpy(PRIVATE_DATA->buffer + FITS_HEADER_SIZE, frame->data, frame->width * frame->height);
		indigo_process_image(device, PRIVATE_DATA->buffer, frame->width, frame->height, 8, true, true, NULL, false);
		CCD_EXPOSURE_PROPERTY->state = INDIGO_OK_STATE;
	} else if (frame->frame_format == UVC_FRAME_FORMAT_GRAY16) {
		memcpy(PRIVATE_DATA->buffer + FITS_HEADER_SIZE, frame->data, 2 * frame->width * frame->height);
		indigo_process_image(device, PRIVATE_DATA->buffer, frame->width, frame->height, 16, true, true, NULL, false);
		CCD_EXPOSURE_PROPERTY->state = INDIGO_OK_STATE;
	} else if (frame->frame_format == UVC_FRAME_FORMAT_RGB || frame->frame_format == UVC_FRAME_FORMAT_YUYV || frame->frame_format == UVC_FRAME_FORMAT_UYVY) {
		uvc_frame_t *rgb = uvc_allocate_frame(3 * frame->width * frame->height);
		res = uvc_any2rgb(frame, rgb);
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "uvc_any2rgb(...) -> %s", uvc_strerror(res));
		if (res != UVC_SUCCESS) {
			CCD_EXPOSURE_PROPERTY->state = INDIGO_ALERT_STATE;
		} else {
			memcpy(PRIVATE_DATA->buffer + FITS_HEADER_SIZE, rgb->data, 3 * frame->width * frame->height);
			indigo_process_image(device, PRIVATE_DATA->buffer, frame->width, frame->height, 24, true, true, NULL, false);
			CCD_EXPOSURE_PROPERTY->state = INDIGO_OK_STATE;
		}
		uvc_free_frame(rgb);
	} else {
		CCD_EXPOSURE_PROPERTY->state = INDIGO_ALERT_STATE;
	}
	uvc_stream_close(PRIVATE_DATA->strmhp);
	INDIGO_DRIVER_DEBUG(DRIVER_NAME, "uvc_stream_close()");
	CCD_EXPOSURE_ITEM->number.value = 0;
	indigo_update_property(device, CCD_EXPOSURE_PROPERTY, NULL);
}

static void streaming_callback(indigo_device *device) {
	while (CCD_STREAMING_COUNT_ITEM->number.value != 0 && CCD_STREAMING_PROPERTY->state == INDIGO_BUSY_STATE) {
		uvc_frame_t *frame = NULL;
		uvc_error_t res = UVC_ERROR_TIMEOUT;
		while (res == UVC_ERROR_TIMEOUT && frame == NULL && CCD_STREAMING_PROPERTY->state == INDIGO_BUSY_STATE) {
			res = uvc_stream_get_frame(PRIVATE_DATA->strmhp, &frame, 1000);
		}
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "uvc_stream_get_frame(...) -> %s", uvc_strerror(res));
		if (res != UVC_SUCCESS || frame == NULL) {
			CCD_STREAMING_PROPERTY->state = INDIGO_ALERT_STATE;
		} else if (frame->frame_format == UVC_FRAME_FORMAT_GRAY8 || frame->frame_format == UVC_FRAME_FORMAT_BY8 || frame->frame_format == UVC_FRAME_FORMAT_BA81 || frame->frame_format == UVC_FRAME_FORMAT_SGRBG8 || frame->frame_format == UVC_FRAME_FORMAT_SGBRG8 || frame->frame_format == UVC_FRAME_FORMAT_SRGGB8 || frame->frame_format == UVC_FRAME_FORMAT_SBGGR8) {
			memcpy(PRIVATE_DATA->buffer + FITS_HEADER_SIZE, frame->data, frame->width * frame->height);
			indigo_process_image(device, PRIVATE_DATA->buffer, frame->width, frame->height, 8, true, true, NULL, false);
			CCD_EXPOSURE_PROPERTY->state = INDIGO_OK_STATE;
		} else if (frame->frame_format == UVC_FRAME_FORMAT_GRAY16) {
			memcpy(PRIVATE_DATA->buffer + FITS_HEADER_SIZE, frame->data, 2 * frame->width * frame->height);
			indigo_process_image(device, PRIVATE_DATA->buffer, frame->width, frame->height, 16, true, true, NULL, false);
			CCD_EXPOSURE_PROPERTY->state = INDIGO_OK_STATE;
		} else if (frame->frame_format == UVC_FRAME_FORMAT_RGB || frame->frame_format == UVC_FRAME_FORMAT_YUYV || frame->frame_format == UVC_FRAME_FORMAT_UYVY) {
			uvc_frame_t *rgb = uvc_allocate_frame(3 * frame->width * frame->height);
			res = uvc_any2rgb(frame, rgb);
			INDIGO_DRIVER_DEBUG(DRIVER_NAME, "uvc_any2rgb(...) -> %s", uvc_strerror(res));
			if (res != UVC_SUCCESS) {
				CCD_EXPOSURE_PROPERTY->state = INDIGO_ALERT_STATE;
			} else {
				memcpy(PRIVATE_DATA->buffer + FITS_HEADER_SIZE, rgb->data, 3 * frame->width * frame->height);
				uvc_free_frame(rgb);
				indigo_process_image(device, PRIVATE_DATA->buffer, frame->width, frame->height, 24, true, true, NULL, false);
				CCD_EXPOSURE_PROPERTY->state = INDIGO_OK_STATE;
			}
		} else {
			CCD_EXPOSURE_PROPERTY->state = INDIGO_ALERT_STATE;
		}
		if (CCD_STREAMING_COUNT_ITEM->number.value != -1)
			CCD_STREAMING_COUNT_ITEM->number.value--;
	}
	indigo_finalize_video_stream(device);
	uvc_stream_close(PRIVATE_DATA->strmhp);
	INDIGO_DRIVER_DEBUG(DRIVER_NAME, "uvc_stream_close()");
	if (CCD_STREAMING_PROPERTY->state == INDIGO_BUSY_STATE)
		CCD_STREAMING_PROPERTY->state = INDIGO_OK_STATE;
	indigo_update_property(device, CCD_STREAMING_PROPERTY, NULL);
}

static indigo_result ccd_attach(indigo_device *device) {
	assert(device != NULL);
	assert(PRIVATE_DATA != NULL);
	if (indigo_ccd_attach(device, DRIVER_NAME, DRIVER_VERSION) == INDIGO_OK) {
		// -------------------------------------------------------------------------------- CCD_BIN
		CCD_BIN_PROPERTY->hidden = true;
		// -------------------------------------------------------------------------------- CCD_FRAME
		CCD_FRAME_PROPERTY->perm = INDIGO_RO_PERM;
		CCD_FRAME_BITS_PER_PIXEL_ITEM->number.min = 0;
		// -------------------------------------------------------------------------------- CCD_EXPOSURE
		CCD_EXPOSURE_ITEM->number.min = 0.001;
		CCD_STREAMING_EXPOSURE_ITEM->number.min = 0.001;
		// -------------------------------------------------------------------------------- CCD_INFO
		CCD_INFO_PROPERTY->count = 2;
		// -------------------------------------------------------------------------------- CCD_STREAMING
		CCD_STREAMING_PROPERTY->hidden = false;
		CCD_IMAGE_FORMAT_PROPERTY->count = 7;
		// --------------------------------------------------------------------------------
		INDIGO_DEVICE_ATTACH_LOG(DRIVER_NAME, device->name);
		return indigo_ccd_enumerate_properties(device, NULL, NULL);
	}
	return INDIGO_FAILED;
}

static struct {
	enum uvc_frame_format format;
	char *fourcc;
	char *label_format;
} formats [] = {
	{ UVC_FRAME_FORMAT_YUYV, "YUY2", "YUV %dx%d" },
	{ UVC_FRAME_FORMAT_YUYV, "YUVY", "YUV %dx%d " },
	{ UVC_FRAME_FORMAT_RGB, "YUVY", "RGB %dx%d " },
	{ UVC_FRAME_FORMAT_GRAY8, "Y800", "MONO8  %dx%d" },
	{ UVC_FRAME_FORMAT_GRAY16, "Y16 ", "MONO16  %dx%d" },
	{ UVC_FRAME_FORMAT_BY8, "BY8 ", "RAW8  %dx%d" },
	{ UVC_FRAME_FORMAT_BA81, "BY81", "RAW8  %dx%d" },
	{ UVC_FRAME_FORMAT_SGRBG8, "GRBG", "RAW8  %dx%d" },
	{ UVC_FRAME_FORMAT_SGBRG8, "GBRG", "RAW8  %dx%d" },
	{ UVC_FRAME_FORMAT_SRGGB8, "RGGB", "RAW8  %dx%d" },
	{ UVC_FRAME_FORMAT_SBGGR8, "BGGR", "RAW8  %dx%d" },
	{ UVC_FRAME_FORMAT_ANY, "    ", "%dx%d" }
};

static void ccd_connect_callback(indigo_device *device) {
	if (CONNECTION_CONNECTED_ITEM->sw.value) {
		if (PRIVATE_DATA->handle == 0) {
			uvc_error_t res = uvc_open(PRIVATE_DATA->dev, &PRIVATE_DATA->handle);
			INDIGO_DRIVER_DEBUG(DRIVER_NAME, "uvc_open() -> %s", uvc_strerror(res));
			if (res != UVC_SUCCESS) {
				CONNECTION_PROPERTY->state = INDIGO_ALERT_STATE;
			} else {
				uvc_print_diag(PRIVATE_DATA->handle, NULL);
				const uvc_format_desc_t *format = uvc_get_format_descs(PRIVATE_DATA->handle);
				CCD_MODE_PROPERTY->count = 0;
				CCD_INFO_WIDTH_ITEM->number.value = CCD_INFO_HEIGHT_ITEM->number.value = 0;
				while (format) {
					int frame_format;
					for (frame_format = 0; formats[frame_format].format != UVC_FRAME_FORMAT_ANY; frame_format++) {
						if (!strncmp((char *)format->fourccFormat, formats[frame_format].fourcc, 4)) {
							break;
						}
					}
					if (format->bDescriptorSubtype == UVC_VS_FORMAT_UNCOMPRESSED || format->bDescriptorSubtype == UVC_VS_FORMAT_FRAME_BASED) {
						uvc_frame_desc_t *frame = format->frame_descs;
						while (frame) {
							if (CCD_INFO_WIDTH_ITEM->number.value < frame->wWidth)
								CCD_INFO_WIDTH_ITEM->number.value = frame->wWidth;
							if (CCD_INFO_HEIGHT_ITEM->number.value < frame->wHeight)
								CCD_INFO_HEIGHT_ITEM->number.value = frame->wHeight;
							if (CCD_MODE_PROPERTY->count == 0) {
								CCD_FRAME_WIDTH_ITEM->number.value = frame->wWidth;
								CCD_FRAME_HEIGHT_ITEM->number.value = frame->wHeight;
								if (formats[frame_format].format == UVC_FRAME_FORMAT_GRAY16)
									CCD_FRAME_BITS_PER_PIXEL_ITEM->number.value = 16;
								else
									CCD_FRAME_BITS_PER_PIXEL_ITEM->number.value = 8;
								PRIVATE_DATA->format = formats[frame_format].format;
							}
							char name[INDIGO_NAME_SIZE], label[INDIGO_VALUE_SIZE];
							sprintf(name, "%d_%dx%d", frame_format, frame->wWidth, frame->wHeight);
							sprintf(label, formats[frame_format].label_format, frame->wWidth, frame->wHeight);
							indigo_init_switch_item(CCD_MODE_PROPERTY->items + CCD_MODE_PROPERTY->count++, name, label, CCD_MODE_PROPERTY->count == 1);
							if (CCD_MODE_PROPERTY->count == 1) {
								uvc_error_t res = uvc_get_stream_ctrl_format_size(PRIVATE_DATA->handle, &PRIVATE_DATA->ctrl, formats[frame_format].format, frame->wWidth, frame->wHeight, 0);
								INDIGO_DRIVER_DEBUG(DRIVER_NAME, "uvc_get_stream_ctrl_format_size(..., %d, %d, %d, 0) -> %s", formats[frame_format].format, frame->wWidth, frame->wHeight, uvc_strerror(res));
								if (res != UVC_SUCCESS) {
									CONNECTION_PROPERTY->state = INDIGO_ALERT_STATE;
								} else {
									res = uvc_set_ae_mode(PRIVATE_DATA->handle, 1);
									INDIGO_DRIVER_DEBUG(DRIVER_NAME, "uvc_set_ae_mode(1) -> %s", uvc_strerror(res));
								}
							}
							frame = frame->next;
						}
					}
					format = format->next;
				}
				uint32_t value_32;
				res = uvc_get_exposure_abs(PRIVATE_DATA->handle, &value_32, UVC_GET_MIN);
				INDIGO_DRIVER_DEBUG(DRIVER_NAME, "uvc_get_exposure_abs(..., -> %d, UVC_GET_MIN) -> %s", value_32, uvc_strerror(res));
				if (res == UVC_SUCCESS)
					CCD_EXPOSURE_ITEM->number.min = CCD_STREAMING_EXPOSURE_ITEM->number.min = value_32 / 10000.0;
				res = uvc_get_exposure_abs(PRIVATE_DATA->handle, &value_32, UVC_GET_MAX);
				INDIGO_DRIVER_DEBUG(DRIVER_NAME, "uvc_get_exposure_abs(..., -> %d, UVC_GET_MAX) -> %s", value_32, uvc_strerror(res));
				if (res == UVC_SUCCESS)
					CCD_EXPOSURE_ITEM->number.max = CCD_STREAMING_EXPOSURE_ITEM->number.max = value_32 / 10000.0;
				uint16_t value_16;
				res = uvc_get_gain(PRIVATE_DATA->handle, &value_16, UVC_GET_INFO);
				INDIGO_DRIVER_DEBUG(DRIVER_NAME, "uvc_get_gain(..., -> %d, UVC_GET_INFO) -> %s", value_16, uvc_strerror(res));
				if (res == UVC_SUCCESS && (value_16 & UVC_CTRL_FLAG_GET_CUR)) {
					CCD_GAIN_PROPERTY->hidden = false;
					CCD_GAIN_PROPERTY->perm = value_16 & UVC_CTRL_FLAG_SET_CUR ? INDIGO_RW_PERM : INDIGO_RO_PERM;
					res = uvc_get_gain(PRIVATE_DATA->handle, &value_16, UVC_GET_CUR);
					INDIGO_DRIVER_DEBUG(DRIVER_NAME, "uvc_get_gain(..., -> %d, UVC_GET_CUR) -> %s", value_16, uvc_strerror(res));
					if (res == UVC_SUCCESS)
						CCD_GAIN_ITEM->number.value = CCD_GAIN_ITEM->number.target = value_16;
					res = uvc_get_gain(PRIVATE_DATA->handle, &value_16, UVC_GET_MIN);
					INDIGO_DRIVER_DEBUG(DRIVER_NAME, "uvc_get_gain(..., -> %d, UVC_GET_MIN) -> %s", value_16, uvc_strerror(res));
					if (res == UVC_SUCCESS)
						CCD_GAIN_ITEM->number.min = value_16;
					res = uvc_get_gain(PRIVATE_DATA->handle, &value_16, UVC_GET_MAX);
					INDIGO_DRIVER_DEBUG(DRIVER_NAME, "uvc_get_gain(..., -> %d, UVC_GET_MAX) -> %s", value_16, uvc_strerror(res));
					if (res == UVC_SUCCESS)
						CCD_GAIN_ITEM->number.max = value_16;
				}
				res = uvc_get_gamma(PRIVATE_DATA->handle, &value_16, UVC_GET_INFO);
				INDIGO_DRIVER_DEBUG(DRIVER_NAME, "uvc_get_gamma(..., -> %d, UVC_GET_INFO) -> %s", value_16, uvc_strerror(res));
				if (res == UVC_SUCCESS && (value_16 & UVC_CTRL_FLAG_GET_CUR)) {
					CCD_GAMMA_PROPERTY->hidden = false;
					CCD_GAMMA_PROPERTY->perm = value_16 & UVC_CTRL_FLAG_SET_CUR ? INDIGO_RW_PERM : INDIGO_RO_PERM;
					res = uvc_get_gamma(PRIVATE_DATA->handle, &value_16, UVC_GET_CUR);
					INDIGO_DRIVER_DEBUG(DRIVER_NAME, "uvc_get_gamma(..., -> %d, UVC_GET_CUR) -> %s", value_16, uvc_strerror(res));
					if (res == UVC_SUCCESS)
						CCD_GAMMA_ITEM->number.value = CCD_GAMMA_ITEM->number.target = value_16;
					res = uvc_get_gamma(PRIVATE_DATA->handle, &value_16, UVC_GET_MIN);
					INDIGO_DRIVER_DEBUG(DRIVER_NAME, "uvc_get_gamma(..., -> %d, UVC_GET_MIN) -> %s", value_16, uvc_strerror(res));
					if (res == UVC_SUCCESS)
						CCD_GAMMA_ITEM->number.min = value_16;
					res = uvc_get_gamma(PRIVATE_DATA->handle, &value_16, UVC_GET_MAX);
					INDIGO_DRIVER_DEBUG(DRIVER_NAME, "uvc_get_gamma(..., -> %d, UVC_GET_MAX) -> %s", value_16, uvc_strerror(res));
					if (res == UVC_SUCCESS)
						CCD_GAMMA_ITEM->number.max = value_16;
				}
				PRIVATE_DATA->buffer = indigo_alloc_blob_buffer(FITS_HEADER_SIZE + (int)CCD_INFO_WIDTH_ITEM->number.value * (int)CCD_INFO_HEIGHT_ITEM->number.value * 6);
				CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
			}
		}
	} else {
		if (PRIVATE_DATA->handle != 0) {
			uvc_close(PRIVATE_DATA->handle);
			INDIGO_DRIVER_DEBUG(DRIVER_NAME, "uvc_close()");
			PRIVATE_DATA->handle = 0;
			if (PRIVATE_DATA->buffer)
				free(PRIVATE_DATA->buffer);
			PRIVATE_DATA->buffer = NULL;
		}
		CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
	}
	indigo_ccd_change_property(device, NULL, CONNECTION_PROPERTY);
}


static indigo_result ccd_change_property(indigo_device *device, indigo_client *client, indigo_property *property) {
	assert(device != NULL);
	assert(DEVICE_CONTEXT != NULL);
	assert(property != NULL);
	if (indigo_property_match(CONNECTION_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- CONNECTION
		if (indigo_ignore_connection_change(device, property))
			return INDIGO_OK;
		indigo_property_copy_values(CONNECTION_PROPERTY, property, false);
		CONNECTION_PROPERTY->state = INDIGO_BUSY_STATE;
		indigo_update_property(device, CONNECTION_PROPERTY, NULL);
		indigo_set_timer(device, 0, ccd_connect_callback, NULL);
		return INDIGO_OK;
	} else if (indigo_property_match(CCD_MODE_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- CCD_MODE
		indigo_property_copy_values(CCD_MODE_PROPERTY, property, false);
		CCD_FRAME_PROPERTY->state = INDIGO_ALERT_STATE;
		CCD_MODE_PROPERTY->state = INDIGO_ALERT_STATE;
		for (int i = 0; i < CCD_MODE_PROPERTY->count; i++) {
			indigo_item *item = &CCD_MODE_PROPERTY->items[i];
			if (item->sw.value) {
				int m, w, h;
				if (sscanf(item->name, "%d_%dx%d", &m, &w, &h) == 3) {
					CCD_FRAME_WIDTH_ITEM->number.value = w;
					CCD_FRAME_HEIGHT_ITEM->number.value = h;
					if (formats[m].format == UVC_FRAME_FORMAT_GRAY16)
						CCD_FRAME_BITS_PER_PIXEL_ITEM->number.value = 16;
					else
						CCD_FRAME_BITS_PER_PIXEL_ITEM->number.value = 8;
					PRIVATE_DATA->format = formats[m].format;
					if (IS_CONNECTED) {
						uvc_error_t res = uvc_get_stream_ctrl_format_size(PRIVATE_DATA->handle, &PRIVATE_DATA->ctrl, formats[m].format, w, h, 0);
						INDIGO_DRIVER_DEBUG(DRIVER_NAME, "uvc_get_stream_ctrl_format_size(..., %d, %d, %d, 0) -> %s", formats[m].format, w, h, uvc_strerror(res));
						if (res == UVC_SUCCESS) {
							CCD_FRAME_PROPERTY->state = INDIGO_OK_STATE;
							CCD_MODE_PROPERTY->state = INDIGO_OK_STATE;
						}
					}
					break;
				}
			}
		}
		if (IS_CONNECTED) {
			indigo_update_property(device, CCD_FRAME_PROPERTY, NULL);
			indigo_update_property(device, CCD_MODE_PROPERTY, NULL);
		}
		return INDIGO_OK;
	} else if (indigo_property_match(CCD_EXPOSURE_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- CCD_EXPOSURE
		indigo_property_copy_values(CCD_EXPOSURE_PROPERTY, property, false);
		uvc_error_t res;
		res = uvc_set_exposure_abs(PRIVATE_DATA->handle, (uint32_t)(10000 * CCD_EXPOSURE_ITEM->number.value));
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "uvc_set_exposure_abs(%d) -> %s", (uint32_t)(10000 * CCD_EXPOSURE_ITEM->number.value), uvc_strerror(res));
		if (!CCD_GAIN_PROPERTY->hidden) {
			res = uvc_set_gain(PRIVATE_DATA->handle, (uint16_t)CCD_GAIN_ITEM->number.value);
			INDIGO_DRIVER_DEBUG(DRIVER_NAME, "uvc_set_gain(%d) -> %s", (uint16_t)CCD_GAIN_ITEM->number.value, uvc_strerror(res));
		}
		if (!CCD_GAMMA_PROPERTY->hidden) {
			res = uvc_set_gamma(PRIVATE_DATA->handle, (uint16_t)CCD_GAMMA_ITEM->number.value);
			INDIGO_DRIVER_DEBUG(DRIVER_NAME, "uvc_set_gamma(%d) -> %s", (uint16_t)CCD_GAMMA_ITEM->number.value, uvc_strerror(res));
		}
		if (res == UVC_SUCCESS) {
			res = uvc_stream_open_ctrl(PRIVATE_DATA->handle, &PRIVATE_DATA->strmhp, &PRIVATE_DATA->ctrl);
			INDIGO_DRIVER_DEBUG(DRIVER_NAME, "uvc_stream_open_ctrl() -> %s", uvc_strerror(res));
		}
		if (res == UVC_SUCCESS) {
			res = uvc_stream_start(PRIVATE_DATA->strmhp, NULL, device, 0);
			INDIGO_DRIVER_DEBUG(DRIVER_NAME, "uvc_stream_start() -> %s", uvc_strerror(res));
		}
		if (res == UVC_SUCCESS) {
			if (CCD_UPLOAD_MODE_LOCAL_ITEM->sw.value || CCD_UPLOAD_MODE_BOTH_ITEM->sw.value) {
				CCD_IMAGE_FILE_PROPERTY->state = INDIGO_BUSY_STATE;
				indigo_update_property(device, CCD_IMAGE_FILE_PROPERTY, NULL);
			}
			if (CCD_UPLOAD_MODE_CLIENT_ITEM->sw.value || CCD_UPLOAD_MODE_BOTH_ITEM->sw.value) {
				CCD_IMAGE_PROPERTY->state = INDIGO_BUSY_STATE;
				indigo_update_property(device, CCD_IMAGE_PROPERTY, NULL);
			}
			CCD_EXPOSURE_PROPERTY->state = INDIGO_BUSY_STATE;
			indigo_set_timer(device, 0, exposure_callback, NULL);
		} else {
			CCD_EXPOSURE_PROPERTY->state = INDIGO_ALERT_STATE;
		}
		indigo_update_property(device, CCD_EXPOSURE_PROPERTY, NULL);
		return INDIGO_OK;
	} else if (indigo_property_match(CCD_STREAMING_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- CCD_STREAMING
		indigo_property_copy_values(CCD_STREAMING_PROPERTY, property, false);
		uvc_error_t res = uvc_set_exposure_abs(PRIVATE_DATA->handle, 10000 * CCD_STREAMING_EXPOSURE_ITEM->number.value);
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "uvc_set_exposure_abs(%d) -> %s", (int)(10000 * CCD_STREAMING_EXPOSURE_ITEM->number.value), uvc_strerror(res));
		if (!CCD_GAIN_PROPERTY->hidden) {
			res = uvc_set_gain(PRIVATE_DATA->handle, CCD_GAIN_ITEM->number.value);
			INDIGO_DRIVER_DEBUG(DRIVER_NAME, "uvc_set_gain(%d) -> %s", (int)CCD_GAIN_ITEM->number.value, uvc_strerror(res));
		}
		if (!CCD_GAMMA_PROPERTY->hidden) {
			res = uvc_set_gamma(PRIVATE_DATA->handle, CCD_GAMMA_ITEM->number.value);
			INDIGO_DRIVER_DEBUG(DRIVER_NAME, "uvc_set_gamma(%d) -> %s", (int)CCD_GAMMA_ITEM->number.value, uvc_strerror(res));
		}
		if (res == UVC_SUCCESS) {
			res = uvc_stream_open_ctrl(PRIVATE_DATA->handle, &PRIVATE_DATA->strmhp, &PRIVATE_DATA->ctrl);
			INDIGO_DRIVER_DEBUG(DRIVER_NAME, "uvc_stream_open_ctrl() -> %s", uvc_strerror(res));
		}
		if (res == UVC_SUCCESS) {
			res = uvc_stream_start(PRIVATE_DATA->strmhp, NULL, device, 0);
			INDIGO_DRIVER_DEBUG(DRIVER_NAME, "uvc_stream_start() -> %s", uvc_strerror(res));
		}
		if (res == UVC_SUCCESS) {
			if (CCD_UPLOAD_MODE_LOCAL_ITEM->sw.value || CCD_UPLOAD_MODE_BOTH_ITEM->sw.value) {
				CCD_IMAGE_FILE_PROPERTY->state = INDIGO_BUSY_STATE;
				indigo_update_property(device, CCD_IMAGE_FILE_PROPERTY, NULL);
			}
			if (CCD_UPLOAD_MODE_CLIENT_ITEM->sw.value || CCD_UPLOAD_MODE_BOTH_ITEM->sw.value) {
				CCD_IMAGE_PROPERTY->state = INDIGO_BUSY_STATE;
				indigo_update_property(device, CCD_IMAGE_PROPERTY, NULL);
			}
			CCD_STREAMING_PROPERTY->state = INDIGO_BUSY_STATE;
			indigo_set_timer(device, 0, streaming_callback, NULL);
		} else {
			CCD_STREAMING_PROPERTY->state = INDIGO_ALERT_STATE;
		}
		indigo_update_property(device, CCD_STREAMING_PROPERTY, NULL);
		return INDIGO_OK;
//	} else if (indigo_property_match(CCD_ABORT_EXPOSURE_PROPERTY, property)) {
//		// -------------------------------------------------------------------------------- CCD_ABORT_EXPOSURE
//		indigo_property_copy_values(CCD_ABORT_EXPOSURE_PROPERTY, property, false);
//		if (CCD_ABORT_EXPOSURE_ITEM->sw.value) {
//			if (CCD_EXPOSURE_PROPERTY->state == INDIGO_BUSY_STATE) {
//				CCD_EXPOSURE_PROPERTY->state = INDIGO_ALERT_STATE;
//			}
//			if (CCD_STREAMING_PROPERTY->state == INDIGO_BUSY_STATE) {
//				CCD_STREAMING_PROPERTY->state = INDIGO_ALERT_STATE;
//			}
//			CCD_ABORT_EXPOSURE_PROPERTY->state = INDIGO_OK_STATE;
//		}
//		indigo_update_property(device, CCD_ABORT_EXPOSURE_PROPERTY, NULL);
	} else if (indigo_property_match(CCD_GAIN_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- CCD_GAIN
		indigo_property_copy_values(CCD_GAIN_PROPERTY, property, false);
		if (IS_CONNECTED) {
			CCD_GAIN_PROPERTY->state = INDIGO_OK_STATE;
			indigo_update_property(device, CCD_GAIN_PROPERTY, NULL);
		}
		return INDIGO_OK;
	} else if (indigo_property_match(CCD_GAMMA_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- CCD_GAMMA
		indigo_property_copy_values(CCD_GAMMA_PROPERTY, property, false);
		if (IS_CONNECTED) {
			CCD_GAMMA_PROPERTY->state = INDIGO_OK_STATE;
			indigo_update_property(device, CCD_GAMMA_PROPERTY, NULL);
		}
		return INDIGO_OK;
	}
	return indigo_ccd_change_property(device, client, property);
}

static indigo_result ccd_detach(indigo_device *device) {
	assert(device != NULL);
	if (IS_CONNECTED) {
		indigo_set_switch(CONNECTION_PROPERTY, CONNECTION_DISCONNECTED_ITEM, true);
		ccd_connect_callback(device);
	}
	INDIGO_DEVICE_DETACH_LOG(DRIVER_NAME, device->name);
	return indigo_ccd_detach(device);
}

// -------------------------------------------------------------------------------- hot-plug support

#define MAX_DEVICES                   10

static indigo_device *devices[MAX_DEVICES];
static pthread_mutex_t device_mutex = PTHREAD_MUTEX_INITIALIZER;

static void process_plug_event(libusb_device *dev) {
	static indigo_device ccd_template = INDIGO_DEVICE_INITIALIZER(
		"",
		ccd_attach,
		indigo_ccd_enumerate_properties,
		ccd_change_property,
		NULL,
		ccd_detach
	);

	uvc_device_t **list;
	pthread_mutex_lock(&device_mutex);

	uvc_error_t res = uvc_get_device_list(uvc_ctx, &list);
	if (res != UVC_SUCCESS) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "uvc_get_device_list() -> %s", uvc_strerror(res));
		pthread_mutex_unlock(&device_mutex);
		return;
	} else {
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "uvc_init() -> %s", uvc_strerror(res));
	}
	for (int i = 0; list[i]; i++) {
		uvc_device_t *uvc_dev = list[i];
		if (uvc_get_bus_number(uvc_dev) == libusb_get_bus_number(dev) && uvc_get_device_address(uvc_dev) == libusb_get_device_address(dev)) {
			uvc_device_descriptor_t *descriptor;
			uvc_get_device_descriptor(list[i], &descriptor);
			if (res != UVC_SUCCESS) {
				INDIGO_DRIVER_ERROR(DRIVER_NAME, "uvc_get_device_descriptor() -> %s", uvc_strerror(res));
			} else {
				INDIGO_DRIVER_DEBUG(DRIVER_NAME, "uvc_init() -> %s", uvc_strerror(res));
			}
			INDIGO_DRIVER_DEBUG(DRIVER_NAME, "%p %s %s detected", uvc_dev, descriptor->manufacturer, descriptor->product);
			uvc_private_data *private_data = indigo_safe_malloc(sizeof(uvc_private_data));
			private_data->dev = uvc_dev;
			indigo_device *device = indigo_safe_malloc_copy(sizeof(indigo_device), &ccd_template);
			char usb_path[INDIGO_NAME_SIZE];
			indigo_get_usb_path(dev, usb_path);
			snprintf(device->name, INDIGO_NAME_SIZE, "%s %s #%s", descriptor->manufacturer, descriptor->product, usb_path);
			device->private_data = private_data;
			for (int j = 0; j < MAX_DEVICES; j++) {
				if (devices[j] == NULL) {
					indigo_attach_device(devices[j] = device);
					break;
				}
			}
		} else {
			uvc_unref_device(uvc_dev);
			INDIGO_DRIVER_DEBUG(DRIVER_NAME, "uvc_unref_device");
		}
	}
	uvc_free_device_list(list, 0);
	INDIGO_DRIVER_DEBUG(DRIVER_NAME, "uvc_free_device_list");
	pthread_mutex_unlock(&device_mutex);
}

static void process_unplug_event(libusb_device *dev) {
	pthread_mutex_lock(&device_mutex);
	for (int j = 0; j < MAX_DEVICES; j++) {
		indigo_device *device = devices[j];
		if (device && uvc_get_bus_number(PRIVATE_DATA->dev) == libusb_get_bus_number(dev) && uvc_get_device_address(PRIVATE_DATA->dev) == libusb_get_device_address(dev)) {
			indigo_detach_device(device);
			free(PRIVATE_DATA);
			free(device);
			devices[j] = NULL;
			break;
		}
	}
	pthread_mutex_unlock(&device_mutex);
}

static int hotplug_callback(libusb_context *ctx, libusb_device *dev, libusb_hotplug_event event, void *user_data) {
	switch (event) {
	case LIBUSB_HOTPLUG_EVENT_DEVICE_ARRIVED:
		INDIGO_ASYNC(process_plug_event, dev);
		break;
	case LIBUSB_HOTPLUG_EVENT_DEVICE_LEFT:
		process_unplug_event(dev);
		break;
	}
	return 0;
};

static libusb_hotplug_callback_handle callback_handle;

indigo_result indigo_ccd_uvc(indigo_driver_action action, indigo_driver_info *info) {
	static indigo_driver_action last_action = INDIGO_DRIVER_SHUTDOWN;

	SET_DRIVER_INFO(info, "UVC Camera", __FUNCTION__, DRIVER_VERSION, true, last_action);

	if (action == last_action)
		return INDIGO_OK;

	switch (action) {
		case INDIGO_DRIVER_INIT:
			last_action = action;
			for (int i = 0; i < MAX_DEVICES; i++) {
				devices[i] = 0;
			}
			INDIGO_DRIVER_DEBUG(DRIVER_NAME, "libuvc %s", LIBUVC_VERSION_STR);
			uvc_error_t res = uvc_init(&uvc_ctx, NULL);
			if (res != UVC_SUCCESS) {
				INDIGO_DRIVER_ERROR(DRIVER_NAME, "uvc_init() -> %s", uvc_strerror(res));
				return INDIGO_FAILED;
			} else {
				INDIGO_DRIVER_DEBUG(DRIVER_NAME, "uvc_init() -> %s", uvc_strerror(res));
			}
			indigo_start_usb_event_handler();
			int rc = libusb_hotplug_register_callback(NULL, LIBUSB_HOTPLUG_EVENT_DEVICE_ARRIVED | LIBUSB_HOTPLUG_EVENT_DEVICE_LEFT, LIBUSB_HOTPLUG_ENUMERATE, LIBUSB_HOTPLUG_MATCH_ANY, LIBUSB_HOTPLUG_MATCH_ANY, LIBUSB_HOTPLUG_MATCH_ANY, hotplug_callback, NULL, &callback_handle);
			INDIGO_DRIVER_DEBUG(DRIVER_NAME, "libusb_hotplug_register_callback() -> %s", rc < 0 ? libusb_error_name(rc) : "OK");
			return rc >= 0 ? INDIGO_OK : INDIGO_FAILED;

		case INDIGO_DRIVER_SHUTDOWN:
			for (int i = 0; i < MAX_DEVICES; i++)
				VERIFY_NOT_CONNECTED(devices[i]);
			last_action = action;
			libusb_hotplug_deregister_callback(NULL, callback_handle);
			INDIGO_DRIVER_DEBUG(DRIVER_NAME, "libusb_hotplug_deregister_callback");
			for (int j = 0; j < MAX_DEVICES; j++) {
				if (devices[j] != NULL) {
					indigo_device *device = devices[j];
					indigo_detach_device(device);
					free(PRIVATE_DATA);
					free(device);
					devices[j] = NULL;
				}
			}
			uvc_exit(uvc_ctx);
			break;

		case INDIGO_DRIVER_INFO:
			break;
	}

	return INDIGO_OK;
}
