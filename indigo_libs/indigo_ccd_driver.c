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
// 2.0 by Peter Polakovic <peter.polakovic@cloudmakers.eu>

/** INDIGO CCD Driver base
 \file indigo_ccd_driver.c
 */

#include <setjmp.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <assert.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>
#include <time.h>
#include <math.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <jpeglib.h>

#include <indigo/indigo_ccd_driver.h>
#include <indigo/indigo_io.h>
#include <indigo/indigo_tiff.h>
#include <indigo/indigo_avi.h>
#include <indigo/indigo_ser.h>

struct indigo_jpeg_compress_struct {
	struct jpeg_compress_struct pub;
	jmp_buf jpeg_error;
};

static void jpeg_compress_error_callback(j_common_ptr cinfo) {
	longjmp(((struct indigo_jpeg_compress_struct *)cinfo)->jpeg_error, 1);
}

struct indigo_jpeg_decompress_struct {
	struct jpeg_decompress_struct pub;
	jmp_buf jpeg_error;
};

static void jpeg_decompress_error_callback(j_common_ptr cinfo) {
	longjmp(((struct indigo_jpeg_decompress_struct *)cinfo)->jpeg_error, 1);
}

static void countdown_timer_callback(indigo_device *device) {
	if (CCD_CONTEXT->countdown_enabled && CCD_EXPOSURE_PROPERTY->state == INDIGO_BUSY_STATE && CCD_EXPOSURE_ITEM->number.value >= 1) {
		CCD_EXPOSURE_ITEM->number.value -= 1;
		if (CCD_EXPOSURE_ITEM->number.value < 0) CCD_EXPOSURE_ITEM->number.value = 0;
		indigo_update_property(device, CCD_EXPOSURE_PROPERTY, NULL);
		indigo_reschedule_timer(device, 1.0, &CCD_CONTEXT->countdown_timer);
	}
}

void indigo_ccd_suspend_countdown(indigo_device *device) {
	CCD_CONTEXT->countdown_enabled = false;
}

void indigo_ccd_resume_countdown(indigo_device *device) {
	CCD_CONTEXT->countdown_enabled = true;
	indigo_set_timer(device, 1.0, countdown_timer_callback, &CCD_CONTEXT->countdown_timer);
}

void indigo_use_shortest_exposure_if_bias(indigo_device *device) {
	if (CCD_FRAME_TYPE_BIAS_ITEM->sw.value) {
		CCD_EXPOSURE_ITEM->number.value = CCD_EXPOSURE_ITEM->number.target = CCD_EXPOSURE_ITEM->number.min;
		CCD_STREAMING_EXPOSURE_ITEM->number.value = CCD_STREAMING_EXPOSURE_ITEM->number.target = CCD_EXPOSURE_ITEM->number.min;
	}
}

indigo_result indigo_ccd_attach(indigo_device *device, const char* driver_name, unsigned version) {
	assert(device != NULL);
	if (CCD_CONTEXT == NULL) {
		device->device_context = indigo_safe_malloc(sizeof(indigo_ccd_context));
	}
	if (CCD_CONTEXT != NULL) {
		if (indigo_device_attach(device, driver_name, version, INDIGO_INTERFACE_CCD) == INDIGO_OK) {
			// -------------------------------------------------------------------------------- CCD_INFO
			CCD_INFO_PROPERTY = indigo_init_number_property(NULL, device->name, CCD_INFO_PROPERTY_NAME, CCD_MAIN_GROUP, "Info", INDIGO_OK_STATE, INDIGO_RO_PERM, 8);
			if (CCD_INFO_PROPERTY == NULL)
				return INDIGO_FAILED;
			indigo_init_number_item(CCD_INFO_WIDTH_ITEM, CCD_INFO_WIDTH_ITEM_NAME, "Horizontal resolution", 0, 0, 0, 0);
			indigo_init_number_item(CCD_INFO_HEIGHT_ITEM, CCD_INFO_HEIGHT_ITEM_NAME, "Vertical resolution", 0, 0, 0, 0);
			indigo_init_number_item(CCD_INFO_MAX_HORIZONAL_BIN_ITEM, CCD_INFO_MAX_HORIZONTAL_BIN_ITEM_NAME, "Max vertical binning", 0, 0, 0, 1);
			indigo_init_number_item(CCD_INFO_MAX_VERTICAL_BIN_ITEM, CCD_INFO_MAX_VERTICAL_BIN_ITEM_NAME, "Max horizontal binning", 0, 0, 0, 1);
			indigo_init_number_item(CCD_INFO_PIXEL_SIZE_ITEM, CCD_INFO_PIXEL_SIZE_ITEM_NAME, "Pixel size (um)", 0, 0, 0, 0);
			indigo_init_number_item(CCD_INFO_PIXEL_WIDTH_ITEM, CCD_INFO_PIXEL_WIDTH_ITEM_NAME, "Pixel width (um)", 0, 0, 0, 0);
			indigo_init_number_item(CCD_INFO_PIXEL_HEIGHT_ITEM, CCD_INFO_PIXEL_HEIGHT_ITEM_NAME, "Pixel height (um)", 0, 0, 0, 0);
			indigo_init_number_item(CCD_INFO_BITS_PER_PIXEL_ITEM, CCD_INFO_BITS_PER_PIXEL_ITEM_NAME, "Bits/pixel", 0, 0, 0, 0);
			// -------------------------------------------------------------------------------- CCD_LENS
			CCD_LENS_PROPERTY = indigo_init_number_property(NULL, device->name, CCD_LENS_PROPERTY_NAME, CCD_MAIN_GROUP, "Lens profile", INDIGO_OK_STATE, INDIGO_RW_PERM, 2);
			if (CCD_LENS_PROPERTY == NULL)
				return INDIGO_FAILED;
			indigo_init_number_item(CCD_LENS_APERTURE_ITEM, CCD_LENS_APERTURE_ITEM_NAME, "Aperture (cm)", 0, 2000, 0, 0);
			indigo_init_number_item(CCD_LENS_FOCAL_LENGTH_ITEM, CCD_LENS_FOCAL_LENGTH_ITEM_NAME, "Focal length (cm)", 0, 10000, 0, 0);
			// -------------------------------------------------------------------------------- CCD_UPLOAD_MODE
			CCD_UPLOAD_MODE_PROPERTY = indigo_init_switch_property(NULL, device->name, CCD_UPLOAD_MODE_PROPERTY_NAME, CCD_MAIN_GROUP, "Image upload", INDIGO_OK_STATE, INDIGO_RW_PERM, INDIGO_ONE_OF_MANY_RULE, 3);
			if (CCD_UPLOAD_MODE_PROPERTY == NULL)
				return INDIGO_FAILED;
			indigo_init_switch_item(CCD_UPLOAD_MODE_CLIENT_ITEM, CCD_UPLOAD_MODE_CLIENT_ITEM_NAME, "Upload to client", true);
			indigo_init_switch_item(CCD_UPLOAD_MODE_LOCAL_ITEM, CCD_UPLOAD_MODE_LOCAL_ITEM_NAME, "Save on server", false);
			indigo_init_switch_item(CCD_UPLOAD_MODE_BOTH_ITEM, CCD_UPLOAD_MODE_BOTH_ITEM_NAME, "Upload and save", false);
			// -------------------------------------------------------------------------------- CCD_PREVIEW
			CCD_PREVIEW_PROPERTY = indigo_init_switch_property(NULL, device->name, CCD_PREVIEW_PROPERTY_NAME, CCD_MAIN_GROUP, "Enable preview", INDIGO_OK_STATE, INDIGO_RW_PERM, INDIGO_ONE_OF_MANY_RULE, 3);
			if (CCD_PREVIEW_PROPERTY == NULL)
				return INDIGO_FAILED;
			indigo_init_switch_item(CCD_PREVIEW_DISABLED_ITEM, CCD_PREVIEW_DISABLED_ITEM_NAME, "Disabled", true);
			indigo_init_switch_item(CCD_PREVIEW_ENABLED_ITEM, CCD_PREVIEW_ENABLED_ITEM_NAME, "Enabled", false);
			indigo_init_switch_item(CCD_PREVIEW_ENABLED_WITH_HISTOGRAM_ITEM, CCD_PREVIEW_ENABLED_WITH_HISTOGRAM_ITEM_NAME, "Enabled with histogram", false);
			// -------------------------------------------------------------------------------- CCD_LOCAL_MODE
			CCD_LOCAL_MODE_PROPERTY = indigo_init_text_property(NULL, device->name, CCD_LOCAL_MODE_PROPERTY_NAME, CCD_MAIN_GROUP, "Save on server", INDIGO_OK_STATE, INDIGO_RW_PERM, 2);
			if (CCD_LOCAL_MODE_PROPERTY == NULL)
				return INDIGO_FAILED;
			indigo_init_text_item(CCD_LOCAL_MODE_DIR_ITEM, CCD_LOCAL_MODE_DIR_ITEM_NAME, "Directory", "%s/", getenv("HOME"));
			indigo_init_text_item(CCD_LOCAL_MODE_PREFIX_ITEM, CCD_LOCAL_MODE_PREFIX_ITEM_NAME, "File name prefix", "IMAGE_XXX");
			// -------------------------------------------------------------------------------- CCD_MODE
			CCD_MODE_PROPERTY = indigo_init_switch_property(NULL, device->name, CCD_MODE_PROPERTY_NAME, CCD_MAIN_GROUP, "Capture mode", INDIGO_OK_STATE, INDIGO_RW_PERM, INDIGO_ONE_OF_MANY_RULE, 64);
			if (CCD_MODE_PROPERTY == NULL)
				return INDIGO_FAILED;
			CCD_MODE_PROPERTY->count = 1;
			CCD_MODE_PROPERTY->perm = INDIGO_RW_PERM;
			indigo_init_switch_item(CCD_MODE_ITEM, "DEFAULT_MODE", "Default mode", true);
			// -------------------------------------------------------------------------------- CCD_READ_MODE
			CCD_READ_MODE_PROPERTY = indigo_init_switch_property(NULL, device->name, CCD_READ_MODE_PROPERTY_NAME, CCD_IMAGE_GROUP, "Read mode", INDIGO_OK_STATE, INDIGO_RW_PERM, INDIGO_ONE_OF_MANY_RULE, 2);
			if (CCD_READ_MODE_PROPERTY == NULL)
				return INDIGO_FAILED;
			indigo_init_switch_item(CCD_READ_MODE_HIGH_SPEED_ITEM, CCD_READ_MODE_HIGH_SPEED_ITEM_NAME, "High speed", false);
			indigo_init_switch_item(CCD_READ_MODE_LOW_NOISE_ITEM, CCD_READ_MODE_LOW_NOISE_ITEM_NAME, "Low noise", true);
			CCD_READ_MODE_PROPERTY->hidden = true;
			// -------------------------------------------------------------------------------- CCD_EXPOSURE
			CCD_EXPOSURE_PROPERTY = indigo_init_number_property(NULL, device->name, CCD_EXPOSURE_PROPERTY_NAME, CCD_MAIN_GROUP, "Start exposure", INDIGO_OK_STATE, INDIGO_RW_PERM, 1);
			if (CCD_EXPOSURE_PROPERTY == NULL)
				return INDIGO_FAILED;
			indigo_init_number_item(CCD_EXPOSURE_ITEM, CCD_EXPOSURE_ITEM_NAME, "Start exposure", 0, 10000, 1, 0);
			strcpy(CCD_EXPOSURE_ITEM->number.format, "%g");
			CCD_CONTEXT->countdown_enabled = true;
			// -------------------------------------------------------------------------------- CCD_STREAMING
			CCD_STREAMING_PROPERTY = indigo_init_number_property(NULL, device->name, CCD_STREAMING_PROPERTY_NAME, CCD_MAIN_GROUP, "Start streaming", INDIGO_OK_STATE, INDIGO_RW_PERM, 2);
			if (CCD_STREAMING_PROPERTY == NULL)
				return INDIGO_FAILED;
			indigo_init_number_item(CCD_STREAMING_EXPOSURE_ITEM, CCD_STREAMING_EXPOSURE_ITEM_NAME, "Shutter time", 0, 10000, 1, 0);
			indigo_init_number_item(CCD_STREAMING_COUNT_ITEM, CCD_STREAMING_COUNT_ITEM_NAME, "Frame count", -1, 100000, 1, -1);
			strcpy(CCD_EXPOSURE_ITEM->number.format, "%g");
			CCD_STREAMING_PROPERTY->hidden = true;
			// -------------------------------------------------------------------------------- CCD_ABORT_EXPOSURE
			CCD_ABORT_EXPOSURE_PROPERTY = indigo_init_switch_property(NULL, device->name, CCD_ABORT_EXPOSURE_PROPERTY_NAME, CCD_MAIN_GROUP, "Abort exposure", INDIGO_OK_STATE, INDIGO_RW_PERM, INDIGO_AT_MOST_ONE_RULE, 1);
			if (CCD_ABORT_EXPOSURE_PROPERTY == NULL)
				return INDIGO_FAILED;
			indigo_init_switch_item(CCD_ABORT_EXPOSURE_ITEM, CCD_ABORT_EXPOSURE_ITEM_NAME, "Abort exposure", false);
			// -------------------------------------------------------------------------------- CCD_FRAME
			CCD_FRAME_PROPERTY = indigo_init_number_property(NULL, device->name, CCD_FRAME_PROPERTY_NAME, CCD_IMAGE_GROUP, "Frame size", INDIGO_OK_STATE, INDIGO_RW_PERM, 5);
			if (CCD_FRAME_PROPERTY == NULL)
				return INDIGO_FAILED;
			indigo_init_number_item(CCD_FRAME_LEFT_ITEM, CCD_FRAME_LEFT_ITEM_NAME, "Left", 0, 0, 1, 0);
			indigo_init_number_item(CCD_FRAME_TOP_ITEM, CCD_FRAME_TOP_ITEM_NAME, "Top", 0, 0, 1, 0);
			indigo_init_number_item(CCD_FRAME_WIDTH_ITEM, CCD_FRAME_WIDTH_ITEM_NAME, "Width", 0, 0, 1, 0);
			indigo_init_number_item(CCD_FRAME_HEIGHT_ITEM, CCD_FRAME_HEIGHT_ITEM_NAME, "Height", 0, 0, 1, 0);
			indigo_init_number_item(CCD_FRAME_BITS_PER_PIXEL_ITEM, CCD_FRAME_BITS_PER_PIXEL_ITEM_NAME, "Bits per pixel", 16, 16, 0, 16);
			// -------------------------------------------------------------------------------- CCD_BIN
			CCD_BIN_PROPERTY = indigo_init_number_property(NULL, device->name, CCD_BIN_PROPERTY_NAME, CCD_IMAGE_GROUP, "Binning", INDIGO_OK_STATE, INDIGO_RO_PERM, 2);
			if (CCD_BIN_PROPERTY == NULL)
				return INDIGO_FAILED;
			indigo_init_number_item(CCD_BIN_HORIZONTAL_ITEM, CCD_BIN_HORIZONTAL_ITEM_NAME, "Horizontal binning", 0, 1, 1, 1);
			indigo_init_number_item(CCD_BIN_VERTICAL_ITEM, CCD_BIN_VERTICAL_ITEM_NAME, "Vertical binning", 0, 1, 1, 1);
			// -------------------------------------------------------------------------------- CCD_GAIN
			CCD_GAIN_PROPERTY = indigo_init_number_property(NULL, device->name, CCD_GAIN_PROPERTY_NAME, CCD_MAIN_GROUP, "Gain", INDIGO_OK_STATE, INDIGO_RW_PERM, 1);
			if (CCD_GAIN_PROPERTY == NULL)
				return INDIGO_FAILED;
			CCD_GAIN_PROPERTY->hidden = true;
			indigo_init_number_item(CCD_GAIN_ITEM, CCD_GAIN_ITEM_NAME, "Gain", 0, 500, 1, 100);
			// -------------------------------------------------------------------------------- CCD_OFFSET
			CCD_OFFSET_PROPERTY = indigo_init_number_property(NULL, device->name, CCD_OFFSET_PROPERTY_NAME, CCD_MAIN_GROUP, "Offset", INDIGO_OK_STATE, INDIGO_RW_PERM, 1);
			if (CCD_OFFSET_PROPERTY == NULL)
				return INDIGO_FAILED;
			CCD_OFFSET_PROPERTY->hidden = true;
			indigo_init_number_item(CCD_OFFSET_ITEM, CCD_OFFSET_ITEM_NAME, "Offset", 0, 10000, 1, 0);
			// -------------------------------------------------------------------------------- CCD_GAMMA
			CCD_GAMMA_PROPERTY = indigo_init_number_property(NULL, device->name, CCD_GAMMA_PROPERTY_NAME, CCD_MAIN_GROUP, "Gamma", INDIGO_OK_STATE, INDIGO_RW_PERM, 1);
			if (CCD_GAMMA_PROPERTY == NULL)
				return INDIGO_FAILED;
			CCD_GAMMA_PROPERTY->hidden = true;
			indigo_init_number_item(CCD_GAMMA_ITEM, CCD_GAMMA_ITEM_NAME, "Gamma", 0.5, 2.0, 0.1, 1);
			// -------------------------------------------------------------------------------- CCD_FRAME_TYPE
			CCD_FRAME_TYPE_PROPERTY = indigo_init_switch_property(NULL, device->name, CCD_FRAME_TYPE_PROPERTY_NAME, CCD_IMAGE_GROUP, "Frame type", INDIGO_OK_STATE, INDIGO_RW_PERM, INDIGO_ONE_OF_MANY_RULE, 5);
			if (CCD_FRAME_TYPE_PROPERTY == NULL)
				return INDIGO_FAILED;
			indigo_init_switch_item(CCD_FRAME_TYPE_LIGHT_ITEM, CCD_FRAME_TYPE_LIGHT_ITEM_NAME, "Light", true);
			indigo_init_switch_item(CCD_FRAME_TYPE_BIAS_ITEM, CCD_FRAME_TYPE_BIAS_ITEM_NAME, "Bias", false);
			indigo_init_switch_item(CCD_FRAME_TYPE_DARK_ITEM, CCD_FRAME_TYPE_DARK_ITEM_NAME, "Dark", false);
			indigo_init_switch_item(CCD_FRAME_TYPE_FLAT_ITEM, CCD_FRAME_TYPE_FLAT_ITEM_NAME, "Flat", false);
			indigo_init_switch_item(CCD_FRAME_TYPE_DARKFLAT_ITEM, CCD_FRAME_TYPE_DARKFLAT_ITEM_NAME, "Dark Flat", false);
			// -------------------------------------------------------------------------------- CCD_IMAGE_FORMAT
			CCD_IMAGE_FORMAT_PROPERTY = indigo_init_switch_property(NULL, device->name, CCD_IMAGE_FORMAT_PROPERTY_NAME, CCD_IMAGE_GROUP, "Image format", INDIGO_OK_STATE, INDIGO_RW_PERM, INDIGO_ONE_OF_MANY_RULE, 7);
			if (CCD_IMAGE_FORMAT_PROPERTY == NULL)
				return INDIGO_FAILED;
			indigo_init_switch_item(CCD_IMAGE_FORMAT_FITS_ITEM, CCD_IMAGE_FORMAT_FITS_ITEM_NAME, "FITS format", true);
			indigo_init_switch_item(CCD_IMAGE_FORMAT_XISF_ITEM, CCD_IMAGE_FORMAT_XISF_ITEM_NAME, "XISF format", false);
			indigo_init_switch_item(CCD_IMAGE_FORMAT_RAW_ITEM, CCD_IMAGE_FORMAT_RAW_ITEM_NAME, "Raw data", false);
			indigo_init_switch_item(CCD_IMAGE_FORMAT_JPEG_ITEM, CCD_IMAGE_FORMAT_JPEG_ITEM_NAME, "JPEG format", false);
			indigo_init_switch_item(CCD_IMAGE_FORMAT_TIFF_ITEM, CCD_IMAGE_FORMAT_TIFF_ITEM_NAME, "TIFF format", false);
			indigo_init_switch_item(CCD_IMAGE_FORMAT_JPEG_AVI_ITEM, CCD_IMAGE_FORMAT_JPEG_AVI_ITEM_NAME, "JPEG + AVI format", false);
			indigo_init_switch_item(CCD_IMAGE_FORMAT_RAW_SER_ITEM, CCD_IMAGE_FORMAT_RAW_SER_ITEM_NAME, "RAW + SER format", false);
			CCD_IMAGE_FORMAT_PROPERTY->count = 5;
			// -------------------------------------------------------------------------------- CCD_IMAGE
			CCD_IMAGE_PROPERTY = indigo_init_blob_property(NULL, device->name, CCD_IMAGE_PROPERTY_NAME, CCD_IMAGE_GROUP, "Image data", INDIGO_OK_STATE, 1);
			if (CCD_IMAGE_PROPERTY == NULL)
				return INDIGO_FAILED;
			indigo_init_blob_item(CCD_IMAGE_ITEM, CCD_IMAGE_ITEM_NAME, "Image data");
			// -------------------------------------------------------------------------------- CCD_PREVIEW_IMAGE
			CCD_PREVIEW_IMAGE_PROPERTY = indigo_init_blob_property(NULL, device->name, CCD_PREVIEW_IMAGE_PROPERTY_NAME, CCD_IMAGE_GROUP, "Preview image data", INDIGO_OK_STATE, 1);
			if (CCD_PREVIEW_IMAGE_PROPERTY == NULL)
				return INDIGO_FAILED;
			CCD_PREVIEW_IMAGE_PROPERTY->hidden = true;
			indigo_init_blob_item(CCD_PREVIEW_IMAGE_ITEM, CCD_PREVIEW_IMAGE_ITEM_NAME, "Image data");
			// -------------------------------------------------------------------------------- CCD_PREVIEW_HISTOGRAM
			CCD_PREVIEW_HISTOGRAM_PROPERTY = indigo_init_blob_property(NULL, device->name, CCD_PREVIEW_HISTOGRAM_PROPERTY_NAME, CCD_IMAGE_GROUP, "Preview image histogram", INDIGO_OK_STATE, 1);
			if (CCD_PREVIEW_HISTOGRAM_PROPERTY == NULL)
				return INDIGO_FAILED;
			CCD_PREVIEW_HISTOGRAM_PROPERTY->hidden = true;
			indigo_init_blob_item(CCD_PREVIEW_HISTOGRAM_ITEM, CCD_PREVIEW_HISTOGRAM_ITEM_NAME, "Image data");
			// -------------------------------------------------------------------------------- CCD_LOCAL_FILE
			CCD_IMAGE_FILE_PROPERTY = indigo_init_text_property(NULL, device->name, CCD_IMAGE_FILE_PROPERTY_NAME, CCD_IMAGE_GROUP, "Image file info", INDIGO_OK_STATE, INDIGO_RO_PERM, 1);
			if (CCD_IMAGE_FILE_PROPERTY == NULL)
				return INDIGO_FAILED;
			indigo_init_text_item(CCD_IMAGE_FILE_ITEM, CCD_IMAGE_FILE_ITEM_NAME, "Filename", "None");
			// -------------------------------------------------------------------------------- CCD_COOLER
			CCD_COOLER_PROPERTY = indigo_init_switch_property(NULL, device->name, CCD_COOLER_PROPERTY_NAME, CCD_COOLER_GROUP, "Cooler status", INDIGO_OK_STATE, INDIGO_RW_PERM, INDIGO_ONE_OF_MANY_RULE, 2);
			if (CCD_COOLER_PROPERTY == NULL)
				return INDIGO_FAILED;
			CCD_COOLER_PROPERTY->hidden = true;
			indigo_init_switch_item(CCD_COOLER_ON_ITEM, CCD_COOLER_ON_ITEM_NAME, "On", false);
			indigo_init_switch_item(CCD_COOLER_OFF_ITEM, CCD_COOLER_OFF_ITEM_NAME, "Off", true);
			// -------------------------------------------------------------------------------- CCD_COOLER_POWER
			CCD_COOLER_POWER_PROPERTY = indigo_init_number_property(NULL, device->name, CCD_COOLER_POWER_PROPERTY_NAME, CCD_COOLER_GROUP, "Cooler power", INDIGO_OK_STATE, INDIGO_RO_PERM, 1);
			if (CCD_COOLER_POWER_PROPERTY == NULL)
				return INDIGO_FAILED;
			CCD_COOLER_POWER_PROPERTY->hidden = true;
			indigo_init_number_item(CCD_COOLER_POWER_ITEM, CCD_COOLER_POWER_ITEM_NAME, "Power (%)", 0, 100, 1, 0);
			// -------------------------------------------------------------------------------- CCD_TEMPERATURE
			CCD_TEMPERATURE_PROPERTY = indigo_init_number_property(NULL, device->name, CCD_TEMPERATURE_PROPERTY_NAME, CCD_COOLER_GROUP, "Sensor temperature", INDIGO_OK_STATE, INDIGO_RW_PERM, 1);
			if (CCD_TEMPERATURE_PROPERTY == NULL)
				return INDIGO_FAILED;
			CCD_TEMPERATURE_PROPERTY->hidden = true;
			indigo_init_number_item(CCD_TEMPERATURE_ITEM, CCD_TEMPERATURE_ITEM_NAME, "Temperature (\u00B0C)", -50, 50, 1, 0);
			// -------------------------------------------------------------------------------- CCD_FITS_HEADERS
			CCD_FITS_HEADERS_PROPERTY = indigo_init_text_property(NULL, device->name, CCD_FITS_HEADERS_PROPERTY_NAME, CCD_IMAGE_GROUP, "Custom FITS headers", INDIGO_OK_STATE, INDIGO_RW_PERM, 10);
			if (CCD_FITS_HEADERS_PROPERTY == NULL)
				return INDIGO_FAILED;
			for (int i = 0; i < CCD_FITS_HEADERS_PROPERTY->count; i++) {
				char name[INDIGO_NAME_SIZE], label[INDIGO_VALUE_SIZE];
				sprintf(name, CCD_FITS_HEADER_ITEM_NAME, i + 1);
				sprintf(label, "Custom Header #%d", i + 1);
				indigo_init_text_item(CCD_FITS_HEADERS_PROPERTY->items + i, name, label, "");
			}
			// -------------------------------------------------------------------------------- CCD_JPEG_SETTINGS
			CCD_JPEG_SETTINGS_PROPERTY = indigo_init_number_property(NULL, device->name, CCD_JPEG_SETTINGS_PROPERTY_NAME, CCD_IMAGE_GROUP, "JPEG Settings", INDIGO_OK_STATE, INDIGO_RW_PERM, 5);
			if (CCD_JPEG_SETTINGS_PROPERTY == NULL)
				return INDIGO_FAILED;
			indigo_init_number_item(CCD_JPEG_SETTINGS_QUALITY_ITEM, CCD_JPEG_SETTINGS_QUALITY_ITEM_NAME, "Conversion quality", 10, 100, 5, 90);
			indigo_init_number_item(CCD_JPEG_SETTINGS_BLACK_ITEM, CCD_JPEG_SETTINGS_BLACK_ITEM_NAME, "Black point", -1, 255, 0, -1);
			indigo_init_number_item(CCD_JPEG_SETTINGS_WHITE_ITEM, CCD_JPEG_SETTINGS_WHITE_ITEM_NAME, "White point", -1, 255, 0, -1);
			indigo_init_number_item(CCD_JPEG_SETTINGS_BLACK_TRESHOLD_ITEM, CCD_JPEG_SETTINGS_BLACK_TRESHOLD_ITEM_NAME, "Black point treshold (%iles)", 0, 10, 0, 0.01);
			indigo_init_number_item(CCD_JPEG_SETTINGS_WHITE_TRESHOLD_ITEM, CCD_JPEG_SETTINGS_WHITE_TRESHOLD_ITEM_NAME, "White point treshold (%iles)", 0, 5, 0, 0.2);
			// -------------------------------------------------------------------------------- CCD_RBI_FLUSH_ENABLE
			CCD_RBI_FLUSH_ENABLE_PROPERTY = indigo_init_switch_property(NULL, device->name, CCD_RBI_FLUSH_ENABLE_PROPERTY_NAME, CCD_MAIN_GROUP, "RBI flush", INDIGO_OK_STATE, INDIGO_RW_PERM, INDIGO_ONE_OF_MANY_RULE, 2);
			if (CCD_RBI_FLUSH_ENABLE_PROPERTY == NULL)
				return INDIGO_FAILED;
			CCD_RBI_FLUSH_ENABLE_PROPERTY->hidden = true;
			indigo_init_switch_item(CCD_RBI_FLUSH_ENABLED_ITEM, CCD_RBI_FLUSH_ENABLED_ITEM_NAME, "Enabled", false);
			indigo_init_switch_item(CCD_RBI_FLUSH_DISABLED_ITEM, CCD_RBI_FLUSH_DISABLED_ITEM_NAME, "Disabled", true);
			// -------------------------------------------------------------------------------- FLI_RBI_FLUSH
			CCD_RBI_FLUSH_PROPERTY = indigo_init_number_property(NULL, device->name, CCD_RBI_FLUSH_PROPERTY_NAME, CCD_MAIN_GROUP, "RBI flush params", INDIGO_OK_STATE, INDIGO_RW_PERM, 2);
			if (CCD_RBI_FLUSH_PROPERTY == NULL)
				return INDIGO_FAILED;
			CCD_RBI_FLUSH_PROPERTY->hidden = true;
			indigo_init_number_item(CCD_RBI_FLUSH_EXPOSURE_ITEM, CCD_RBI_FLUSH_EXPOSURE_ITEM_NAME, "NIR flood time (s)", 0, 16, 0, 1);
			indigo_init_number_item(CCD_RBI_FLUSH_COUNT_ITEM, CCD_RBI_FLUSH_COUNT_ITEM_NAME, "Number of flushes", 1, 10, 1, 3);
			// --------------------------------------------------------------------------------
			return INDIGO_OK;
		}
	}
	return INDIGO_FAILED;
}

indigo_result indigo_ccd_enumerate_properties(indigo_device *device, indigo_client *client, indigo_property *property) {
	assert(device != NULL);
	assert(DEVICE_CONTEXT != NULL);
	if (IS_CONNECTED) {
		if (indigo_property_match(CCD_INFO_PROPERTY, property))
			indigo_define_property(device, CCD_INFO_PROPERTY, NULL);
		if (indigo_property_match(CCD_LENS_PROPERTY, property))
			indigo_define_property(device, CCD_LENS_PROPERTY, NULL);
		if (indigo_property_match(CCD_LOCAL_MODE_PROPERTY, property))
			indigo_define_property(device, CCD_LOCAL_MODE_PROPERTY, NULL);
		if (indigo_property_match(CCD_IMAGE_FILE_PROPERTY, property))
			indigo_define_property(device, CCD_IMAGE_FILE_PROPERTY, NULL);
		if (indigo_property_match(CCD_MODE_PROPERTY, property))
			indigo_define_property(device, CCD_MODE_PROPERTY, NULL);
		if (indigo_property_match(CCD_READ_MODE_PROPERTY, property))
			indigo_define_property(device, CCD_READ_MODE_PROPERTY, NULL);
		if (indigo_property_match(CCD_EXPOSURE_PROPERTY, property))
			indigo_define_property(device, CCD_EXPOSURE_PROPERTY, NULL);
		if (indigo_property_match(CCD_STREAMING_PROPERTY, property))
			indigo_define_property(device, CCD_STREAMING_PROPERTY, NULL);
		if (indigo_property_match(CCD_ABORT_EXPOSURE_PROPERTY, property))
			indigo_define_property(device, CCD_ABORT_EXPOSURE_PROPERTY, NULL);
		if (indigo_property_match(CCD_FRAME_PROPERTY, property))
			indigo_define_property(device, CCD_FRAME_PROPERTY, NULL);
		if (indigo_property_match(CCD_BIN_PROPERTY, property))
			indigo_define_property(device, CCD_BIN_PROPERTY, NULL);
		if (indigo_property_match(CCD_OFFSET_PROPERTY, property))
			indigo_define_property(device, CCD_OFFSET_PROPERTY, NULL);
		if (indigo_property_match(CCD_GAIN_PROPERTY, property))
			indigo_define_property(device, CCD_GAIN_PROPERTY, NULL);
		if (indigo_property_match(CCD_GAMMA_PROPERTY, property))
			indigo_define_property(device, CCD_GAMMA_PROPERTY, NULL);
		if (indigo_property_match(CCD_FRAME_TYPE_PROPERTY, property))
			indigo_define_property(device, CCD_FRAME_TYPE_PROPERTY, NULL);
		if (indigo_property_match(CCD_IMAGE_FORMAT_PROPERTY, property))
			indigo_define_property(device, CCD_IMAGE_FORMAT_PROPERTY, NULL);
		if (indigo_property_match(CCD_UPLOAD_MODE_PROPERTY, property))
			indigo_define_property(device, CCD_UPLOAD_MODE_PROPERTY, NULL);
		if (indigo_property_match(CCD_PREVIEW_PROPERTY, property))
			indigo_define_property(device, CCD_PREVIEW_PROPERTY, NULL);
		if (indigo_property_match(CCD_IMAGE_PROPERTY, property))
			indigo_define_property(device, CCD_IMAGE_PROPERTY, NULL);
		if (indigo_property_match(CCD_PREVIEW_IMAGE_PROPERTY, property))
			indigo_define_property(device, CCD_PREVIEW_IMAGE_PROPERTY, NULL);
		if (indigo_property_match(CCD_PREVIEW_HISTOGRAM_PROPERTY, property))
			indigo_define_property(device, CCD_PREVIEW_HISTOGRAM_PROPERTY, NULL);
		if (indigo_property_match(CCD_COOLER_PROPERTY, property))
			indigo_define_property(device, CCD_COOLER_PROPERTY, NULL);
		if (indigo_property_match(CCD_COOLER_POWER_PROPERTY, property))
			indigo_define_property(device, CCD_COOLER_POWER_PROPERTY, NULL);
		if (indigo_property_match(CCD_TEMPERATURE_PROPERTY, property))
			indigo_define_property(device, CCD_TEMPERATURE_PROPERTY, NULL);
		if (indigo_property_match(CCD_FITS_HEADERS_PROPERTY, property))
			indigo_define_property(device, CCD_FITS_HEADERS_PROPERTY, NULL);
		if (indigo_property_match(CCD_JPEG_SETTINGS_PROPERTY, property))
			indigo_define_property(device, CCD_JPEG_SETTINGS_PROPERTY, NULL);
		if (indigo_property_match(CCD_RBI_FLUSH_ENABLE_PROPERTY, property))
			indigo_define_property(device, CCD_RBI_FLUSH_ENABLE_PROPERTY, NULL);
		if (indigo_property_match(CCD_RBI_FLUSH_PROPERTY, property))
			indigo_define_property(device, CCD_RBI_FLUSH_PROPERTY, NULL);
	}
	return indigo_device_enumerate_properties(device, client, property);
}

indigo_result indigo_ccd_change_property(indigo_device *device, indigo_client *client, indigo_property *property) {
	assert(device != NULL);
	assert(DEVICE_CONTEXT != NULL);
	assert(property != NULL);
	if (indigo_property_match(CONNECTION_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- CONNECTION
		if (IS_CONNECTED) {
			indigo_define_property(device, CCD_INFO_PROPERTY, NULL);
			indigo_define_property(device, CCD_LENS_PROPERTY, NULL);
			indigo_define_property(device, CCD_UPLOAD_MODE_PROPERTY, NULL);
			indigo_define_property(device, CCD_PREVIEW_PROPERTY, NULL);
			indigo_define_property(device, CCD_LOCAL_MODE_PROPERTY, NULL);
			indigo_define_property(device, CCD_MODE_PROPERTY, NULL);
			indigo_define_property(device, CCD_READ_MODE_PROPERTY, NULL);
			indigo_define_property(device, CCD_EXPOSURE_PROPERTY, NULL);
			indigo_define_property(device, CCD_STREAMING_PROPERTY, NULL);
			indigo_define_property(device, CCD_ABORT_EXPOSURE_PROPERTY, NULL);
			indigo_define_property(device, CCD_FRAME_PROPERTY, NULL);
			indigo_define_property(device, CCD_BIN_PROPERTY, NULL);
			indigo_define_property(device, CCD_OFFSET_PROPERTY, NULL);
			indigo_define_property(device, CCD_GAIN_PROPERTY, NULL);
			indigo_define_property(device, CCD_GAMMA_PROPERTY, NULL);
			indigo_define_property(device, CCD_FRAME_TYPE_PROPERTY, NULL);
			indigo_define_property(device, CCD_IMAGE_FORMAT_PROPERTY, NULL);
			indigo_define_property(device, CCD_IMAGE_FILE_PROPERTY, NULL);
			indigo_define_property(device, CCD_IMAGE_PROPERTY, NULL);
			indigo_define_property(device, CCD_PREVIEW_IMAGE_PROPERTY, NULL);
			indigo_define_property(device, CCD_PREVIEW_HISTOGRAM_PROPERTY, NULL);
			indigo_define_property(device, CCD_COOLER_PROPERTY, NULL);
			indigo_define_property(device, CCD_COOLER_POWER_PROPERTY, NULL);
			indigo_define_property(device, CCD_TEMPERATURE_PROPERTY, NULL);
			indigo_define_property(device, CCD_FITS_HEADERS_PROPERTY, NULL);
			indigo_define_property(device, CCD_JPEG_SETTINGS_PROPERTY, NULL);
			indigo_define_property(device, CCD_RBI_FLUSH_ENABLE_PROPERTY, NULL);
			indigo_define_property(device, CCD_RBI_FLUSH_PROPERTY, NULL);
		} else {
			CCD_STREAMING_COUNT_ITEM->number.value = 0;
			CCD_EXPOSURE_ITEM->number.value = 0;
			CCD_STREAMING_PROPERTY->state = INDIGO_OK_STATE;
			CCD_EXPOSURE_PROPERTY->state = INDIGO_OK_STATE;
			CCD_IMAGE_PROPERTY->state = INDIGO_OK_STATE;
			CCD_COOLER_POWER_PROPERTY->state = INDIGO_OK_STATE;
			CCD_TEMPERATURE_PROPERTY->state = INDIGO_OK_STATE;
			indigo_delete_property(device, CCD_INFO_PROPERTY, NULL);
			indigo_delete_property(device, CCD_LENS_PROPERTY, NULL);
			indigo_delete_property(device, CCD_UPLOAD_MODE_PROPERTY, NULL);
			indigo_delete_property(device, CCD_PREVIEW_PROPERTY, NULL);
			indigo_delete_property(device, CCD_LOCAL_MODE_PROPERTY, NULL);
			indigo_delete_property(device, CCD_MODE_PROPERTY, NULL);
			indigo_delete_property(device, CCD_READ_MODE_PROPERTY, NULL);
			indigo_delete_property(device, CCD_EXPOSURE_PROPERTY, NULL);
			indigo_delete_property(device, CCD_STREAMING_PROPERTY, NULL);
			indigo_delete_property(device, CCD_ABORT_EXPOSURE_PROPERTY, NULL);
			indigo_delete_property(device, CCD_FRAME_PROPERTY, NULL);
			indigo_delete_property(device, CCD_BIN_PROPERTY, NULL);
			indigo_delete_property(device, CCD_OFFSET_PROPERTY, NULL);
			indigo_delete_property(device, CCD_GAIN_PROPERTY, NULL);
			indigo_delete_property(device, CCD_GAMMA_PROPERTY, NULL);
			indigo_delete_property(device, CCD_FRAME_TYPE_PROPERTY, NULL);
			indigo_delete_property(device, CCD_IMAGE_FORMAT_PROPERTY, NULL);
			indigo_delete_property(device, CCD_IMAGE_FILE_PROPERTY, NULL);
			indigo_delete_property(device, CCD_IMAGE_PROPERTY, NULL);
			indigo_delete_property(device, CCD_PREVIEW_IMAGE_PROPERTY, NULL);
			indigo_delete_property(device, CCD_PREVIEW_HISTOGRAM_PROPERTY, NULL);
			indigo_delete_property(device, CCD_COOLER_PROPERTY, NULL);
			indigo_delete_property(device, CCD_COOLER_POWER_PROPERTY, NULL);
			indigo_delete_property(device, CCD_TEMPERATURE_PROPERTY, NULL);
			indigo_delete_property(device, CCD_FITS_HEADERS_PROPERTY, NULL);
			indigo_delete_property(device, CCD_JPEG_SETTINGS_PROPERTY, NULL);
			indigo_delete_property(device, CCD_RBI_FLUSH_ENABLE_PROPERTY, NULL);
			indigo_delete_property(device, CCD_RBI_FLUSH_PROPERTY, NULL);
		}
	} else if (indigo_property_match(CONFIG_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- CONFIG
		if (indigo_switch_match(CONFIG_SAVE_ITEM, property)) {
			indigo_save_property(device, NULL, CCD_LENS_PROPERTY);
			indigo_save_property(device, NULL, CCD_MODE_PROPERTY);
			indigo_save_property(device, NULL, CCD_READ_MODE_PROPERTY);
			indigo_save_property(device, NULL, CCD_UPLOAD_MODE_PROPERTY);
			indigo_save_property(device, NULL, CCD_LOCAL_MODE_PROPERTY);
			indigo_save_property(device, NULL, CCD_FRAME_PROPERTY);
			indigo_save_property(device, NULL, CCD_BIN_PROPERTY);
			indigo_save_property(device, NULL, CCD_OFFSET_PROPERTY);
			indigo_save_property(device, NULL, CCD_GAMMA_PROPERTY);
			indigo_save_property(device, NULL, CCD_GAIN_PROPERTY);
			indigo_save_property(device, NULL, CCD_FRAME_TYPE_PROPERTY);
			indigo_save_property(device, NULL, CCD_FITS_HEADERS_PROPERTY);
			indigo_save_property(device, NULL, CCD_JPEG_SETTINGS_PROPERTY);
			indigo_save_property(device, NULL, CCD_RBI_FLUSH_ENABLE_PROPERTY);
			indigo_save_property(device, NULL, CCD_RBI_FLUSH_PROPERTY);
		}
	} else if (indigo_property_match(CCD_LENS_PROPERTY, property)) {
		indigo_property_copy_values(CCD_LENS_PROPERTY, property, false);
		CCD_LENS_PROPERTY->state = INDIGO_OK_STATE;
		indigo_update_property(device, CCD_LENS_PROPERTY, NULL);
		return INDIGO_OK;
	} else if (indigo_property_match(CCD_EXPOSURE_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- CCD_EXPOSURE
		if (CCD_EXPOSURE_PROPERTY->state == INDIGO_BUSY_STATE) {
			if (CCD_UPLOAD_MODE_LOCAL_ITEM->sw.value || CCD_UPLOAD_MODE_BOTH_ITEM->sw.value) {
				if (CCD_IMAGE_FILE_PROPERTY->state != INDIGO_BUSY_STATE) {
					CCD_IMAGE_FILE_PROPERTY->state = INDIGO_BUSY_STATE;
					indigo_update_property(device, CCD_IMAGE_FILE_PROPERTY, NULL);
				}
			}
			if (CCD_UPLOAD_MODE_CLIENT_ITEM->sw.value || CCD_UPLOAD_MODE_BOTH_ITEM->sw.value) {
				if (CCD_IMAGE_PROPERTY->state != INDIGO_BUSY_STATE) {
					CCD_IMAGE_PROPERTY->state = INDIGO_BUSY_STATE;
					indigo_update_property(device, CCD_IMAGE_PROPERTY, NULL);
				}
			}
			if (CCD_EXPOSURE_ITEM->number.value >= 1) {
				 indigo_set_timer(device, 1.0, countdown_timer_callback, &CCD_CONTEXT->countdown_timer);
			}
		}
		return INDIGO_OK;
	} else if (indigo_property_match(CCD_ABORT_EXPOSURE_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- CCD_ABORT_EXPOSURE
		if (CCD_IMAGE_PROPERTY->state == INDIGO_BUSY_STATE) {
			CCD_IMAGE_PROPERTY->state = INDIGO_ALERT_STATE;
			indigo_update_property(device, CCD_IMAGE_PROPERTY, NULL);
		}
		if (CCD_PREVIEW_IMAGE_PROPERTY->state == INDIGO_BUSY_STATE) {
			CCD_PREVIEW_IMAGE_PROPERTY->state = INDIGO_ALERT_STATE;
			indigo_update_property(device, CCD_PREVIEW_IMAGE_PROPERTY, NULL);
		}
		if (CCD_PREVIEW_HISTOGRAM_PROPERTY->state == INDIGO_BUSY_STATE) {
			CCD_PREVIEW_HISTOGRAM_PROPERTY->state = INDIGO_ALERT_STATE;
			indigo_update_property(device, CCD_PREVIEW_HISTOGRAM_PROPERTY, NULL);
		}
		if (CCD_IMAGE_FILE_PROPERTY->state == INDIGO_BUSY_STATE) {
			CCD_IMAGE_FILE_PROPERTY->state = INDIGO_ALERT_STATE;
			indigo_update_property(device, CCD_IMAGE_FILE_PROPERTY, NULL);
		}
		if (CCD_EXPOSURE_PROPERTY->state == INDIGO_BUSY_STATE) {
			CCD_EXPOSURE_PROPERTY->state = INDIGO_ALERT_STATE;
			CCD_EXPOSURE_ITEM->number.value = 0;
			indigo_update_property(device, CCD_EXPOSURE_PROPERTY, NULL);
			CCD_ABORT_EXPOSURE_PROPERTY->state = INDIGO_OK_STATE;
		} else if (CCD_STREAMING_PROPERTY->state == INDIGO_BUSY_STATE) {
			CCD_STREAMING_PROPERTY->state = INDIGO_ALERT_STATE;
			CCD_STREAMING_COUNT_ITEM->number.value = 0;
			indigo_update_property(device, CCD_STREAMING_PROPERTY, NULL);
			CCD_ABORT_EXPOSURE_PROPERTY->state = INDIGO_OK_STATE;
		} else {
			CCD_ABORT_EXPOSURE_PROPERTY->state = INDIGO_ALERT_STATE;
		}
		CCD_ABORT_EXPOSURE_ITEM->sw.value = false;
		indigo_update_property(device, CCD_ABORT_EXPOSURE_PROPERTY, NULL);
		return INDIGO_OK;
	} else if (indigo_property_match_w(CCD_FRAME_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- CCD_FRAME
		indigo_property_copy_values(CCD_FRAME_PROPERTY, property, false);
		CCD_FRAME_WIDTH_ITEM->number.value = ((int)CCD_FRAME_WIDTH_ITEM->number.value / (int)CCD_BIN_HORIZONTAL_ITEM->number.value) * (int)CCD_BIN_HORIZONTAL_ITEM->number.value;
		CCD_FRAME_HEIGHT_ITEM->number.value = ((int)CCD_FRAME_HEIGHT_ITEM->number.value / (int)CCD_BIN_VERTICAL_ITEM->number.value) * (int)CCD_BIN_VERTICAL_ITEM->number.value;
		CCD_FRAME_PROPERTY->state = INDIGO_OK_STATE;
		if (CCD_FRAME_LEFT_ITEM->number.value + CCD_FRAME_WIDTH_ITEM->number.value > CCD_INFO_WIDTH_ITEM->number.value) {
			CCD_FRAME_WIDTH_ITEM->number.value = CCD_INFO_WIDTH_ITEM->number.value - CCD_FRAME_LEFT_ITEM->number.value;
			CCD_FRAME_PROPERTY->state = INDIGO_ALERT_STATE;
		}
		if (CCD_FRAME_TOP_ITEM->number.value + CCD_FRAME_HEIGHT_ITEM->number.value > CCD_INFO_HEIGHT_ITEM->number.value) {
			CCD_FRAME_HEIGHT_ITEM->number.value = CCD_INFO_HEIGHT_ITEM->number.value - CCD_FRAME_TOP_ITEM->number.value;
			CCD_FRAME_PROPERTY->state = INDIGO_ALERT_STATE;
		}
		if (IS_CONNECTED) {
			indigo_update_property(device, CCD_FRAME_PROPERTY, NULL);
		}
		return INDIGO_OK;
	} else if (indigo_property_match_w(CCD_BIN_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- CCD_BIN
		indigo_property_copy_values(CCD_BIN_PROPERTY, property, false);
		char name[32];
		snprintf(name, 32, "BIN_%dx%d", (int)CCD_BIN_HORIZONTAL_ITEM->number.value, (int)CCD_BIN_VERTICAL_ITEM->number.value);
		for (int i = 0; i < CCD_MODE_PROPERTY->count; i++) {
			indigo_item *item = &CCD_MODE_PROPERTY->items[i];
			item->sw.value = !strcmp(item->name, name);
		}
		if (IS_CONNECTED) {
			CCD_MODE_PROPERTY->state = INDIGO_OK_STATE;
			indigo_update_property(device, CCD_MODE_PROPERTY, NULL);
			CCD_BIN_PROPERTY->state = INDIGO_OK_STATE;
			indigo_update_property(device, CCD_BIN_PROPERTY, NULL);
		}
		return INDIGO_OK;
	} else if (indigo_property_match_w(CCD_MODE_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- CCD_MODE
		indigo_property_copy_values(CCD_MODE_PROPERTY, property, false);
		for (int i = 0; i < CCD_MODE_PROPERTY->count; i++) {
			indigo_item *item = &CCD_MODE_PROPERTY->items[i];
			if (item->sw.value) {
				int h, v;
				if (sscanf(item->name, "BIN_%dx%d", &h, &v) == 2) {
					CCD_BIN_HORIZONTAL_ITEM->number.value = CCD_BIN_HORIZONTAL_ITEM->number.target = h;
					CCD_BIN_VERTICAL_ITEM->number.value = CCD_BIN_VERTICAL_ITEM->number.target = v;
					CCD_FRAME_TOP_ITEM->number.value = CCD_FRAME_LEFT_ITEM->number.value = 0;
					CCD_FRAME_WIDTH_ITEM->number.value = ((int)CCD_INFO_WIDTH_ITEM->number.value / (int)CCD_BIN_HORIZONTAL_ITEM->number.value) * (int)CCD_BIN_HORIZONTAL_ITEM->number.value;
					CCD_FRAME_HEIGHT_ITEM->number.value = ((int)CCD_INFO_HEIGHT_ITEM->number.value / (int)CCD_BIN_VERTICAL_ITEM->number.value) * (int)CCD_BIN_VERTICAL_ITEM->number.value;
				}
				break;
			}
		}
		if (IS_CONNECTED) {
			CCD_FRAME_PROPERTY->state = INDIGO_OK_STATE;
			indigo_update_property(device, CCD_FRAME_PROPERTY, NULL);
			CCD_BIN_PROPERTY->state = INDIGO_OK_STATE;
			indigo_update_property(device, CCD_BIN_PROPERTY, NULL);
			CCD_MODE_PROPERTY->state = INDIGO_OK_STATE;
			indigo_update_property(device, CCD_MODE_PROPERTY, NULL);
		}
		return INDIGO_OK;
	} else if (indigo_property_match_w(CCD_OFFSET_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- CCD_OFFSET
		indigo_property_copy_values(CCD_OFFSET_PROPERTY, property, false);
		if (IS_CONNECTED) {
			CCD_OFFSET_PROPERTY->state = INDIGO_OK_STATE;
			indigo_update_property(device, CCD_OFFSET_PROPERTY, NULL);
		}
		return INDIGO_OK;
	} else if (indigo_property_match_w(CCD_READ_MODE_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- CCD_READ_MODE
		indigo_property_copy_values(CCD_READ_MODE_PROPERTY, property, false);
		if (IS_CONNECTED) {
			CCD_READ_MODE_PROPERTY->state = INDIGO_OK_STATE;
			indigo_update_property(device, CCD_READ_MODE_PROPERTY, NULL);
		}
		return INDIGO_OK;
	} else if (indigo_property_match_w(CCD_GAIN_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- CCD_GAIN
		indigo_property_copy_values(CCD_GAIN_PROPERTY, property, false);
		if (IS_CONNECTED) {
			CCD_GAIN_PROPERTY->state = INDIGO_OK_STATE;
			indigo_update_property(device, CCD_GAIN_PROPERTY, NULL);
		}
		return INDIGO_OK;
	} else if (indigo_property_match_w(CCD_GAMMA_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- CCD_GAMMA
		indigo_property_copy_values(CCD_GAMMA_PROPERTY, property, false);
		if (IS_CONNECTED) {
			CCD_GAMMA_PROPERTY->state = INDIGO_OK_STATE;
			indigo_update_property(device, CCD_GAMMA_PROPERTY, NULL);
		}
		return INDIGO_OK;
	} else if (indigo_property_match(CCD_FRAME_TYPE_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- CCD_FRAME_TYPE
		indigo_property_copy_values(CCD_FRAME_TYPE_PROPERTY, property, false);
		CCD_FRAME_TYPE_PROPERTY->state = INDIGO_OK_STATE;
		if (IS_CONNECTED)
			indigo_update_property(device, CCD_FRAME_TYPE_PROPERTY, NULL);
		return INDIGO_OK;
	} else if (indigo_property_match(CCD_IMAGE_FORMAT_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- CCD_IMAGE_FORMAT
		indigo_property_copy_values(CCD_IMAGE_FORMAT_PROPERTY, property, false);
		CCD_IMAGE_FORMAT_PROPERTY->state = INDIGO_OK_STATE;
		if (IS_CONNECTED)
			indigo_update_property(device, CCD_IMAGE_FORMAT_PROPERTY, NULL);
		return INDIGO_OK;
	} else if (indigo_property_match(CCD_UPLOAD_MODE_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- CCD_IMAGE_UPLOAD_MODE
		indigo_property_copy_values(CCD_UPLOAD_MODE_PROPERTY, property, false);
		CCD_UPLOAD_MODE_PROPERTY->state = INDIGO_OK_STATE;
		if (IS_CONNECTED)
			indigo_update_property(device, CCD_UPLOAD_MODE_PROPERTY, NULL);
		return INDIGO_OK;
	} else if (indigo_property_match(CCD_PREVIEW_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- CCD_PREVIEW
		indigo_property_copy_values(CCD_PREVIEW_PROPERTY, property, false);
		if (CCD_PREVIEW_ENABLED_WITH_HISTOGRAM_ITEM->sw.value) {
			if (CCD_PREVIEW_IMAGE_PROPERTY->hidden) {
				CCD_PREVIEW_IMAGE_PROPERTY->hidden = false;
				if (IS_CONNECTED)
					indigo_define_property(device, CCD_PREVIEW_IMAGE_PROPERTY, NULL);
			}
			if (CCD_PREVIEW_HISTOGRAM_PROPERTY->hidden) {
				CCD_PREVIEW_HISTOGRAM_PROPERTY->hidden = false;
				if (IS_CONNECTED)
					indigo_define_property(device, CCD_PREVIEW_HISTOGRAM_PROPERTY, NULL);
			}
		} else if (CCD_PREVIEW_ENABLED_ITEM->sw.value) {
			if (CCD_PREVIEW_IMAGE_PROPERTY->hidden) {
				CCD_PREVIEW_IMAGE_PROPERTY->hidden = false;
				if (IS_CONNECTED)
					indigo_define_property(device, CCD_PREVIEW_IMAGE_PROPERTY, NULL);
			}
			if (!CCD_PREVIEW_HISTOGRAM_PROPERTY->hidden) {
				if (IS_CONNECTED)
					indigo_delete_property(device, CCD_PREVIEW_HISTOGRAM_PROPERTY, NULL);
				CCD_PREVIEW_HISTOGRAM_PROPERTY->hidden = true;
			}
		} else {
			if (!CCD_PREVIEW_IMAGE_PROPERTY->hidden) {
				if (IS_CONNECTED)
					indigo_delete_property(device, CCD_PREVIEW_IMAGE_PROPERTY, NULL);
				CCD_PREVIEW_IMAGE_PROPERTY->hidden = true;
			}
			if (!CCD_PREVIEW_HISTOGRAM_PROPERTY->hidden) {
				if (IS_CONNECTED)
					indigo_delete_property(device, CCD_PREVIEW_HISTOGRAM_PROPERTY, NULL);
				CCD_PREVIEW_HISTOGRAM_PROPERTY->hidden = true;
			}
		}
		CCD_PREVIEW_PROPERTY->state = INDIGO_OK_STATE;
		if (IS_CONNECTED)
			indigo_update_property(device, CCD_PREVIEW_PROPERTY, NULL);
		return INDIGO_OK;
	} else if (indigo_property_match(CCD_LOCAL_MODE_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- CCD_LOCAL_MODE
		indigo_property_copy_values(CCD_LOCAL_MODE_PROPERTY, property, false);
		long len = strlen(CCD_LOCAL_MODE_DIR_ITEM->text.value);
		if (len == 0)
			snprintf(CCD_LOCAL_MODE_DIR_ITEM->text.value, INDIGO_VALUE_SIZE, "%s/", getenv("HOME"));
		else if (CCD_LOCAL_MODE_DIR_ITEM->text.value[len - 1] != '/')
			strcat(CCD_LOCAL_MODE_DIR_ITEM->text.value, "/");
		CCD_LOCAL_MODE_PROPERTY->state = INDIGO_OK_STATE;
		if (IS_CONNECTED)
			indigo_update_property(device, CCD_LOCAL_MODE_PROPERTY, NULL);
		return INDIGO_OK;
	} else if (indigo_property_match(CCD_FITS_HEADERS_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- CCD_FITS_HEADERS
		indigo_property_copy_values(CCD_FITS_HEADERS_PROPERTY, property, false);
		for (int i = 0; i < CCD_FITS_HEADERS_PROPERTY->count; i++) {
			indigo_item *item = CCD_FITS_HEADERS_PROPERTY->items + i;
			if (*item->text.value == 0)
				continue;
			char *eq = strchr(item->text.value, '=');
			char line[81];
			if (eq) {
				char *tmp = item->text.value;
				while (tmp - item->text.value < 8 && (isalpha(*tmp) || isdigit(*tmp) || *tmp == '-' || *tmp == '_')) {
					int c = toupper(*tmp);
					*tmp++ = c;
				}
				*tmp = 0;
				eq++;
				while (eq - item->text.value < 80 && *eq == ' ')
					eq++;
				snprintf(line, 80, "%-8s= %s", item->text.value, eq);
				indigo_copy_value(item->text.value, line);
			} else if (!strncasecmp(item->text.value, "COMMENT ", 7)) {
				char *tmp = item->text.value + 7;
				while (tmp - item->text.value < 80 && *tmp == ' ')
					tmp++;
				snprintf(line, 80, "COMMENT  %s", tmp);
				indigo_copy_value(item->text.value, line);
			} else if (!strncasecmp(item->text.value, "HISTORY ", 7)) {
				char *tmp = item->text.value + 7;
				while (tmp - item->text.value < 80 && *tmp == ' ')
					tmp++;
				snprintf(line, 80, "HISTORY  %s", tmp);
				indigo_copy_value(item->text.value, line);
			} else if (IS_CONNECTED) {
				CCD_FITS_HEADERS_PROPERTY->state = INDIGO_ALERT_STATE;
				indigo_update_property(device, CCD_FITS_HEADERS_PROPERTY, "Invalid header line format");
				return INDIGO_OK;
			} else {
				*item->text.value = 0;
			}
		}
		CCD_FITS_HEADERS_PROPERTY->state = INDIGO_OK_STATE;
		if (IS_CONNECTED)
			indigo_update_property(device, CCD_FITS_HEADERS_PROPERTY, NULL);
		return INDIGO_OK;
	} else if (indigo_property_match(CCD_JPEG_SETTINGS_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- CCD_JPEG_SETTINGS
		indigo_property_copy_values(CCD_JPEG_SETTINGS_PROPERTY, property, false);
		CCD_JPEG_SETTINGS_PROPERTY->state = INDIGO_OK_STATE;
		if (IS_CONNECTED)
			indigo_update_property(device, CCD_JPEG_SETTINGS_PROPERTY, NULL);
		return INDIGO_OK;
		// -------------------------------------------------------------------------------- CCD_RBI_FLUSH_ENABLE
	} else if (indigo_property_match(CCD_RBI_FLUSH_ENABLE_PROPERTY, property)) {
		if (CCD_EXPOSURE_PROPERTY->state == INDIGO_BUSY_STATE) {
			CCD_RBI_FLUSH_ENABLE_PROPERTY->state = INDIGO_ALERT_STATE;
			indigo_update_property(device, CCD_RBI_FLUSH_ENABLE_PROPERTY, "Exposure in progress, RBI flush can not be changed.");
			return INDIGO_OK;
		}
		indigo_property_copy_values(CCD_RBI_FLUSH_ENABLE_PROPERTY, property, false);
		CCD_RBI_FLUSH_ENABLE_PROPERTY->state = INDIGO_OK_STATE;
		if (IS_CONNECTED) {
			indigo_update_property(device, CCD_RBI_FLUSH_ENABLE_PROPERTY, NULL);
		}
		return INDIGO_OK;
		// -------------------------------------------------------------------------------- FLI_RBI_FLUSH
	} else if (indigo_property_match(CCD_RBI_FLUSH_PROPERTY, property)) {
		if (CCD_EXPOSURE_PROPERTY->state == INDIGO_BUSY_STATE) {
			CCD_RBI_FLUSH_PROPERTY->state = INDIGO_ALERT_STATE;
			indigo_update_property(device, CCD_RBI_FLUSH_PROPERTY, "Exposure in progress, RBI flush can not be changed.");
			return INDIGO_OK;
		}
		indigo_property_copy_values(CCD_RBI_FLUSH_PROPERTY, property, false);
		CCD_RBI_FLUSH_PROPERTY->state = INDIGO_OK_STATE;
		if (IS_CONNECTED) {
			indigo_update_property(device, CCD_RBI_FLUSH_PROPERTY, NULL);
		}
		return INDIGO_OK;
		// --------------------------------------------------------------------------------
	}
	return indigo_device_change_property(device, client, property);
}

indigo_result indigo_ccd_detach(indigo_device *device) {
	assert(device != NULL);
	indigo_release_property(CCD_INFO_PROPERTY);
	indigo_release_property(CCD_LENS_PROPERTY);
	indigo_release_property(CCD_UPLOAD_MODE_PROPERTY);
	indigo_release_property(CCD_PREVIEW_PROPERTY);
	indigo_release_property(CCD_LOCAL_MODE_PROPERTY);
	indigo_release_property(CCD_MODE_PROPERTY);
	indigo_release_property(CCD_READ_MODE_PROPERTY);
	indigo_release_property(CCD_EXPOSURE_PROPERTY);
	indigo_release_property(CCD_STREAMING_PROPERTY);
	indigo_release_property(CCD_ABORT_EXPOSURE_PROPERTY);
	indigo_release_property(CCD_FRAME_PROPERTY);
	indigo_release_property(CCD_BIN_PROPERTY);
	indigo_release_property(CCD_GAIN_PROPERTY);
	indigo_release_property(CCD_GAMMA_PROPERTY);
	indigo_release_property(CCD_OFFSET_PROPERTY);
	indigo_release_property(CCD_FRAME_TYPE_PROPERTY);
	indigo_release_property(CCD_IMAGE_FORMAT_PROPERTY);
	indigo_release_property(CCD_IMAGE_FILE_PROPERTY);
	indigo_release_property(CCD_IMAGE_PROPERTY);
	indigo_release_property(CCD_PREVIEW_IMAGE_PROPERTY);
	indigo_release_property(CCD_PREVIEW_HISTOGRAM_PROPERTY);
	indigo_release_property(CCD_TEMPERATURE_PROPERTY);
	indigo_release_property(CCD_COOLER_PROPERTY);
	indigo_release_property(CCD_COOLER_POWER_PROPERTY);
	indigo_release_property(CCD_FITS_HEADERS_PROPERTY);
	indigo_release_property(CCD_JPEG_SETTINGS_PROPERTY);
	indigo_release_property(CCD_RBI_FLUSH_ENABLE_PROPERTY);
	indigo_release_property(CCD_RBI_FLUSH_PROPERTY);
	if (CCD_CONTEXT->preview_image)
		free(CCD_CONTEXT->preview_image);
	return indigo_device_detach(device);
}

static void set_black_white(indigo_device *device, unsigned long *histo, long count) {
	long black = CCD_JPEG_SETTINGS_BLACK_TRESHOLD_ITEM->number.value * count / 100.0; /* In percenitle */
	if (black == 0) black = 1;
	if (CCD_JPEG_SETTINGS_BLACK_ITEM->number.target == -1) {
		long total = 0;
		for (int i = 0; i < 4096; i++) {
			total += histo[i];
			if (total >= black) {
				CCD_JPEG_SETTINGS_BLACK_ITEM->number.value = i / 16.0;
				break;
			}
		}
	} else {
		CCD_JPEG_SETTINGS_BLACK_ITEM->number.value = CCD_JPEG_SETTINGS_BLACK_ITEM->number.target;
	}
	long white = CCD_JPEG_SETTINGS_WHITE_TRESHOLD_ITEM->number.value * count / 100.0; /* In percenitle */
	if (white == 0) white = 1;
	if (CCD_JPEG_SETTINGS_WHITE_ITEM->number.target == -1) {
		long total = 0;
		for (int i = 4095; i >= 0; i--) {
			total += histo[i];
			if (total >= white) {
				CCD_JPEG_SETTINGS_WHITE_ITEM->number.value = i / 16.0;
				break;
			}
		}
	} else {
		CCD_JPEG_SETTINGS_WHITE_ITEM->number.value = CCD_JPEG_SETTINGS_WHITE_ITEM->number.target;
	}

	if (fabs(CCD_JPEG_SETTINGS_BLACK_ITEM->number.value - CCD_JPEG_SETTINGS_WHITE_ITEM->number.value) < 2) {
		if (CCD_JPEG_SETTINGS_BLACK_ITEM->number.value >= 1) {
			CCD_JPEG_SETTINGS_BLACK_ITEM->number.value -= 1;
		} else if (CCD_JPEG_SETTINGS_WHITE_ITEM->number.value <= 254) {
			CCD_JPEG_SETTINGS_WHITE_ITEM->number.value += 1;
		}
	}

	if (CCD_JPEG_SETTINGS_BLACK_ITEM->number.value != CCD_JPEG_SETTINGS_BLACK_ITEM->number.target || CCD_JPEG_SETTINGS_WHITE_ITEM->number.value != CCD_JPEG_SETTINGS_WHITE_ITEM->number.target) {
		CCD_JPEG_SETTINGS_PROPERTY->state = INDIGO_OK_STATE;
		indigo_update_property(device, CCD_JPEG_SETTINGS_PROPERTY, NULL);
	}
}

void indigo_raw_to_jpeg(indigo_device *device, void *data_in, int frame_width, int frame_height, int bpp, bool little_endian, bool byte_order_rgb, void **data_out, unsigned long *size_out, void **histogram_data, unsigned long *histogram_size) {
	INDIGO_DEBUG(clock_t start = clock());
	int size_in = frame_width * frame_height;
	void *copy = indigo_safe_malloc_copy(size_in * bpp / 8, data_in + FITS_HEADER_SIZE);
	unsigned long *histo = indigo_safe_malloc(4096 * sizeof(unsigned long));
	unsigned char *mem = NULL;
	unsigned long mem_size = 0;
	struct indigo_jpeg_compress_struct cinfo;
	struct jpeg_error_mgr jerr;
	cinfo.pub.err = jpeg_std_error(&jerr);
	/* override default exit() and return 2 lines below */
	jerr.error_exit = jpeg_compress_error_callback;
	/* Jump here in case of a decmpression error */
	if (setjmp(cinfo.jpeg_error)) {
		jpeg_destroy_compress(&cinfo.pub);
		free(copy);
		free(histo);
		INDIGO_ERROR(indigo_error("JPEG compression failed"));
		return;
	}
	jpeg_create_compress(&cinfo.pub);
	jpeg_mem_dest(&cinfo.pub, &mem, &mem_size);
	cinfo.pub.image_width = frame_width;
	cinfo.pub.image_height = frame_height;
	if (bpp == 8 || bpp == 24) {
		unsigned char *b8 = copy;
		int count = size_in * (bpp == 8 ? 1 : 3);
		for (int i = 0; i < count; i++) {
			histo[*b8++ << 4]++;
		}
		set_black_white(device, histo, count);
		double scale = rint(CCD_JPEG_SETTINGS_WHITE_ITEM->number.value - CCD_JPEG_SETTINGS_BLACK_ITEM->number.value) / 256.0;
		int offset = CCD_JPEG_SETTINGS_BLACK_ITEM->number.value;
		b8 = copy;
		for (int i = 0; i < count; i++) {
			int value = (*b8 - offset) / scale;
			if (value < 0)
				value = 0;
			else if (value > 255)
				value = 255;
			*b8++ = value;
		}
	} else if (bpp == 16 || bpp == 48) {
		uint16_t *b16 = copy;
		int count = size_in * (bpp == 16 ? 1 : 3);
		if (little_endian) {
			for (int i = 0; i < count; i++) {
				histo[*b16++ >> 4]++;
			}
		} else {
			for (int i = 0; i < count; i++) {
				int value = *b16;
				value = (value & 0xff) << 8 | (value & 0xff00) >> 8;
				*b16++ = value;
				histo[value >> 4]++;
			}
		}
		set_black_white(device, histo, count);
		int offset = CCD_JPEG_SETTINGS_BLACK_ITEM->number.value * 256;
		double scale = (CCD_JPEG_SETTINGS_WHITE_ITEM->number.value - CCD_JPEG_SETTINGS_BLACK_ITEM->number.value);
		unsigned char *b8 = copy;
		b16 = copy;
		for (int i = 0; i < count; i++) {
			int value = rint((*b16++ - offset) / scale);
			if (value < 0)
				value = 0;
			else if (value > 255)
				value = 255;
			*b8++ = value;
		}
	}
	if (bpp == 8 || bpp == 16) {
		cinfo.pub.input_components = 1;
		cinfo.pub.in_color_space = JCS_GRAYSCALE;
	} else if (bpp == 24 || bpp == 48) {
		if (!byte_order_rgb) {
			unsigned char *b8 = copy;
			for (int i = 0; i < size_in; i++) {
				unsigned char b = *b8;
				unsigned char r = *(b8 + 2);
				*b8 = r;
				*(b8 + 2) = b;
				b8 += 3;
			}
		}
		cinfo.pub.input_components = 3;
		cinfo.pub.in_color_space = JCS_RGB;
	}
	jpeg_set_defaults(&cinfo.pub);
	jpeg_set_quality(&cinfo.pub, CCD_JPEG_SETTINGS_QUALITY_ITEM->number.target, true);
	JSAMPROW row_pointer[1];
	jpeg_start_compress(&cinfo.pub, TRUE);
	while (cinfo.pub.next_scanline < cinfo.pub.image_height) {
		row_pointer[0] = &((JSAMPROW)copy)[cinfo.pub.next_scanline * cinfo.pub.image_width *  cinfo.pub.input_components];
		jpeg_write_scanlines(&cinfo.pub, row_pointer, 1);
	}
	jpeg_finish_compress(&cinfo.pub);
	jpeg_destroy_compress(&cinfo.pub);
	*data_out = mem;
	*size_out = mem_size;
	free(copy);
	if (histogram_data != NULL) {
		uint8_t raw[32][256];
		memset(raw, 0, sizeof(raw));
		for (int i = 0; i < 256; i++) {
			int base = i * 16;
			unsigned long val = 0;
			for (int j = 0; j < 16; j++)
				val += histo[base + j];
			unsigned long mask = 0x80000000;
			uint8_t pixel = 0;
			for (unsigned long j = 0; j < 32; j++) {
				if (val & mask) {
					pixel = 0xF0;
				}
				raw[j][i] = pixel;
				mask = mask >> 1;
			}
		}
		mem = NULL;
		mem_size = 0;
		jpeg_create_compress(&cinfo.pub);
		jpeg_mem_dest(&cinfo.pub, &mem, &mem_size);
		cinfo.pub.image_width = 256;
		cinfo.pub.image_height = 32;
		cinfo.pub.input_components = 1;
		cinfo.pub.in_color_space = JCS_GRAYSCALE;
		jpeg_set_defaults(&cinfo.pub);
		JSAMPROW row_pointer[1];
		jpeg_start_compress(&cinfo.pub, TRUE);
		while (cinfo.pub.next_scanline < cinfo.pub.image_height) {
			row_pointer[0] = ((JSAMPROW)((uint8_t *)raw + cinfo.pub.next_scanline * 256));
			jpeg_write_scanlines(&cinfo.pub, row_pointer, 1);
		}
		jpeg_finish_compress(&cinfo.pub);
		jpeg_destroy_compress(&cinfo.pub);
		*histogram_data = mem;
		*histogram_size = mem_size;
	}
	free(histo);
	INDIGO_DEBUG(indigo_debug("RAW to preview conversion in %gs", (clock() - start) / (double)CLOCKS_PER_SEC));
}

static void add_key(char **header, bool fits, char *format, ...) {
	char *buffer = *header;
	va_list argList;
	va_start(argList, format);
	int length = vsnprintf(buffer, 80, format, argList);
	va_end(argList);
	indigo_fix_locale(buffer);
	if (fits) {
		buffer[length] = ' ';
		length = 80;
	} else {
		buffer[length++] = '\n';
		buffer[length] = 0;
	}
	*header = buffer + length;
}

static void raw_to_tiff(indigo_device *device, void *data_in, int frame_width, int frame_height, int bpp, bool little_endian, bool byte_order_rgb, void **data_out, unsigned long *size_out, indigo_fits_keyword *keywords) {
	indigo_tiff_memory_handle *memory_handle = indigo_safe_malloc(sizeof(indigo_tiff_memory_handle));
	memory_handle->data = indigo_safe_malloc(memory_handle->size = 10240);
	memory_handle->file_length = memory_handle->file_offset = 0;
	TIFF *tiff = TIFFClientOpen("", little_endian ? "wl" : "wb", (thandle_t)memory_handle, indigo_tiff_read, indigo_tiff_write, indigo_tiff_seek, indigo_tiff_close, indigo_tiff_size, NULL, NULL);
	TIFFSetField(tiff, TIFFTAG_COMPRESSION, COMPRESSION_LZW);
	time_t timer;
	struct tm* tm_info;
	char date_time_end[20];
	time(&timer);
	timer -= CCD_EXPOSURE_ITEM->number.target;
	tm_info = gmtime(&timer);
	strftime(date_time_end, 20, "%Y-%m-%dT%H:%M:%S", tm_info);
	int horizontal_bin = CCD_BIN_HORIZONTAL_ITEM->number.value;
	int vertical_bin = CCD_BIN_VERTICAL_ITEM->number.value;
	char *fits_header = malloc(FITS_HEADER_SIZE);
	char *next_key = fits_header;
	add_key(&next_key, false, "SIMPLE  =                    T / file conforms to FITS standard");
	if (bpp == 8 || bpp == 16) {
		add_key(&next_key, false, "BITPIX  = %20d / number of bits per data pixel", bpp);
		add_key(&next_key, false, "NAXIS   =                    %d / number of data axes", 2);
		add_key(&next_key, false, "NAXIS1  = %20d / length of data axis 1 [pixels]", frame_width);
		add_key(&next_key, false, "NAXIS2  = %20d / length of data axis 2 [pixels]", frame_height);
	} else {
		add_key(&next_key, false, "BITPIX  = %20d / number of bits per data pixel", bpp / 3);
		add_key(&next_key, false, "NAXIS   =                    %d / number of data axes", 3);
		add_key(&next_key, false, "NAXIS1  = %20d / length of data axis 1 [pixels]", frame_width);
		add_key(&next_key, false, "NAXIS2  = %20d / length of data axis 2 [pixels]", frame_height);
		add_key(&next_key, false, "NAXIS3  = %20d / length of data axis 3 [RGB]\n", 3);
	}
	add_key(&next_key, false, "EXTEND  =                    T / FITS dataset may contain extensions");
	if (bpp == 16 || bpp == 48) {
		add_key(&next_key, false, "BZERO   =                32768 / offset data range to that of unsigned short");
		add_key(&next_key, false, "BSCALE  =                    1 / default scaling factor");
	}
	add_key(&next_key, false, "XBINNING= %20d / horizontal binning [pixels]", horizontal_bin);
	add_key(&next_key, false, "YBINNING= %20d / vertical binning [pixels]", vertical_bin);
	if (CCD_INFO_PIXEL_WIDTH_ITEM->number.value > 0 && CCD_INFO_PIXEL_HEIGHT_ITEM->number.value) {
		add_key(&next_key, false, "XPIXSZ  = %20.2f / pixel width [microns]", CCD_INFO_PIXEL_WIDTH_ITEM->number.value * horizontal_bin);
		add_key(&next_key, false, "YPIXSZ  = %20.2f / pixel height [microns]", CCD_INFO_PIXEL_HEIGHT_ITEM->number.value * vertical_bin);
	}
	add_key(&next_key, false, "EXPTIME = %20.2f / exposure time [s]", CCD_EXPOSURE_ITEM->number.target);
	if (!CCD_TEMPERATURE_PROPERTY->hidden)
		add_key(&next_key, false, "CCD-TEMP= %20.2f / CCD temperature [C]", CCD_TEMPERATURE_ITEM->number.value);
	if (CCD_FRAME_TYPE_LIGHT_ITEM->sw.value)
		add_key(&next_key, false, "IMAGETYP= 'Light'               / frame type");
	else if (CCD_FRAME_TYPE_FLAT_ITEM->sw.value)
		add_key(&next_key, false, "IMAGETYP= 'Flat'                / frame type");
	else if (CCD_FRAME_TYPE_BIAS_ITEM->sw.value)
		add_key(&next_key, false, "IMAGETYP= 'Bias'                / frame type");
	else if (CCD_FRAME_TYPE_DARK_ITEM->sw.value)
		add_key(&next_key, false, "IMAGETYP= 'Dark'                / frame type");
	else if (CCD_FRAME_TYPE_DARKFLAT_ITEM->sw.value)
		add_key(&next_key, false, "IMAGETYP= 'DarkFlat'            / frame type");
	if (!CCD_GAIN_PROPERTY->hidden)
		add_key(&next_key, false, "GAIN    = %20.2f / Gain", CCD_GAIN_ITEM->number.value);
	if (!CCD_OFFSET_PROPERTY->hidden)
		add_key(&next_key, false, "OFFSET  = %20.2f / Offset", CCD_OFFSET_ITEM->number.value);
	if (!CCD_GAMMA_PROPERTY->hidden)
		add_key(&next_key, false, "GAMMA   = %20.2f / Gamma", CCD_GAMMA_ITEM->number.value);
	add_key(&next_key, false, "DATE-OBS= '%s' / UTC date that FITS file was created", date_time_end);
	add_key(&next_key, false, "INSTRUME= '%s'%*c / instrument name", device->name, (int)(19 - strlen(device->name)), ' ');
	add_key(&next_key, false, "ROWORDER= 'TOP-DOWN'           / Image row order");
	if (keywords) {
		while (keywords->type && (next_key - fits_header) < (FITS_HEADER_SIZE - 80)) {
			switch (keywords->type) {
				case INDIGO_FITS_NUMBER:
					add_key(&next_key, false, "%7s= %20f / %s", keywords->name, keywords->number, keywords->comment);
					break;
				case INDIGO_FITS_STRING:
					add_key(&next_key, false, "%7s= '%s'%*c / %s", keywords->name, keywords->string, (int)(18 - strlen(keywords->string)), ' ', keywords->comment);
					break;
				case INDIGO_FITS_LOGICAL:
					add_key(&next_key, false, "%7s=                    %c / %s", keywords->name, keywords->logical ? 'T' : 'F', keywords->comment);
					break;
			}
			keywords++;
		}
	}
	for (int i = 0; i < CCD_FITS_HEADERS_PROPERTY->count; i++) {
		indigo_item *item = CCD_FITS_HEADERS_PROPERTY->items + i;
		if (*item->text.value && (next_key - fits_header) < (FITS_HEADER_SIZE - 80)) {
			add_key(&next_key, false, "%s", item->text.value);
		}
	}

	add_key(&next_key, false, "END");
	TIFFSetField(tiff, TIFFTAG_IMAGEDESCRIPTION, fits_header);
	TIFFSetField(tiff, TIFFTAG_IMAGEWIDTH, frame_width);
	TIFFSetField(tiff, TIFFTAG_IMAGELENGTH, frame_height);
	if (bpp == 8) {
		TIFFSetField(tiff, TIFFTAG_SAMPLESPERPIXEL, 1);
		TIFFSetField(tiff, TIFFTAG_BITSPERSAMPLE, 8);
		TIFFSetField(tiff, TIFFTAG_PLANARCONFIG, PLANARCONFIG_CONTIG);
		TIFFSetField(tiff, TIFFTAG_PHOTOMETRIC, PHOTOMETRIC_MINISBLACK);
	} else if (bpp == 16) {
		TIFFSetField(tiff, TIFFTAG_SAMPLESPERPIXEL, 1);
		TIFFSetField(tiff, TIFFTAG_BITSPERSAMPLE, 16);
		TIFFSetField(tiff, TIFFTAG_PLANARCONFIG, PLANARCONFIG_CONTIG);
		TIFFSetField(tiff, TIFFTAG_PHOTOMETRIC, PHOTOMETRIC_MINISBLACK);
	} else if (bpp == 24) {
		TIFFSetField(tiff, TIFFTAG_SAMPLESPERPIXEL, 3);
		TIFFSetField(tiff, TIFFTAG_BITSPERSAMPLE, 8);
		TIFFSetField(tiff, TIFFTAG_PHOTOMETRIC, PHOTOMETRIC_RGB);
		if (!byte_order_rgb) {
			int size_in = frame_width * frame_height;
			unsigned char *b8 = data_in + FITS_HEADER_SIZE;
			for (int i = 0; i < size_in; i++) {
				unsigned char b = *b8;
				unsigned char r = *(b8 + 2);
				*b8 = r;
				*(b8 + 2) = b;
				b8 += 3;
			}
		}
	} else if (bpp == 48) {
		TIFFSetField(tiff, TIFFTAG_SAMPLESPERPIXEL, 3);
		TIFFSetField(tiff, TIFFTAG_BITSPERSAMPLE, 16);
		TIFFSetField(tiff, TIFFTAG_PHOTOMETRIC, PHOTOMETRIC_RGB);
		if (!byte_order_rgb) {
			int size_in = frame_width * frame_height;
			uint16_t *b16 = (uint16_t *)(data_in + FITS_HEADER_SIZE);
			for (int i = 0; i < size_in; i++) {
				uint16_t b = *b16;
				uint16_t r = *(b16 + 2);
				*b16 = r;
				*(b16 + 2) = b;
				b16 += 3;
			}
		}
	}
	TIFFSetField(tiff, TIFFTAG_SAMPLEFORMAT, SAMPLEFORMAT_UINT);
	TIFFSetField(tiff, TIFFTAG_ORIENTATION, ORIENTATION_TOPLEFT);
	TIFFSetField(tiff, TIFFTAG_PLANARCONFIG, PLANARCONFIG_CONTIG);
	TIFFSetField(tiff, TIFFTAG_ROWSPERSTRIP, frame_height);
	TIFFSetField(tiff, TIFFTAG_SOFTWARE, "INDIGO");
	TIFFWriteEncodedStrip(tiff, 0, data_in + FITS_HEADER_SIZE, frame_width * frame_height * bpp / 8);
	TIFFWriteDirectory(tiff);
	indigo_tiff_close(tiff);
	*data_out = memory_handle->data;
	*size_out = memory_handle->file_length;
	free(memory_handle);
}

static bool create_file_name(char *dir, char *prefix, char *suffix, char *file_name) {
	if (strlen(dir) + strlen(prefix) + strlen(suffix) < INDIGO_VALUE_SIZE) {
		char *placeholder = strstr(prefix, "XXX");
		if (placeholder == NULL) {
			indigo_copy_value(file_name, dir);
			strcat(file_name, prefix);
			strcat(file_name, suffix);
		} else {
			char format[INDIGO_VALUE_SIZE];
			strcpy(format, dir);
			strncat(format, prefix, placeholder - prefix);
			if (!strncmp(placeholder, "XXXX", 4)) {
				strcat(format, "%04d");
				strcat(format, placeholder + 4);
			} else {
				strcat(format, "%03d");
				strcat(format, placeholder + 3);
			}
			strcat(format, suffix);
			struct stat sb;
			int i = 1;
			while (i < 10000) {
				snprintf(file_name, INDIGO_VALUE_SIZE, format, i);
				if (stat(file_name, &sb) == 0 && S_ISREG(sb.st_mode))
					i++;
				else
					break;
			}
		}
		return true;
	}
	return false;
}

void indigo_process_image(indigo_device *device, void *data, int frame_width, int frame_height, int bpp, bool little_endian, bool byte_order_rgb, indigo_fits_keyword *keywords, bool streaming) {
	assert(device != NULL);
	assert(data != NULL);
	INDIGO_DEBUG(clock_t start = clock());
	int horizontal_bin = CCD_BIN_HORIZONTAL_ITEM->number.value;
	int vertical_bin = CCD_BIN_VERTICAL_ITEM->number.value;
	int byte_per_pixel = bpp / 8;
	int naxis = 2;
	unsigned long size = frame_width * frame_height;
	unsigned long blobsize = byte_per_pixel * size;
	if (byte_per_pixel == 3) {
		byte_per_pixel = 1;
		naxis = 3;
	} else if (byte_per_pixel == 6) {
		byte_per_pixel = 2;
		naxis = 3;
	}

	void *jpeg_data = NULL;
	unsigned long jpeg_size = 0;
	void *histogram_data = NULL;
	unsigned long histogram_size = 0;
	if (CCD_IMAGE_FORMAT_JPEG_ITEM->sw.value || CCD_IMAGE_FORMAT_JPEG_AVI_ITEM->sw.value || CCD_PREVIEW_ENABLED_ITEM->sw.value || CCD_PREVIEW_ENABLED_WITH_HISTOGRAM_ITEM->sw.value) {
		indigo_raw_to_jpeg(device, data, frame_width, frame_height, bpp, little_endian, byte_order_rgb, &jpeg_data, &jpeg_size,  CCD_PREVIEW_ENABLED_WITH_HISTOGRAM_ITEM->sw.value ? &histogram_data : NULL, CCD_PREVIEW_ENABLED_WITH_HISTOGRAM_ITEM->sw.value ? &histogram_size : NULL);
		if (CCD_PREVIEW_ENABLED_ITEM->sw.value || CCD_PREVIEW_ENABLED_WITH_HISTOGRAM_ITEM->sw.value) {
			CCD_PREVIEW_IMAGE_PROPERTY->state = INDIGO_BUSY_STATE;
			indigo_update_property(device, CCD_PREVIEW_IMAGE_PROPERTY, NULL);
			if (jpeg_data) {
				if (CCD_CONTEXT->preview_image) {
					if (CCD_CONTEXT->preview_image_size < jpeg_size) {
						CCD_CONTEXT->preview_image = indigo_safe_realloc(CCD_CONTEXT->preview_image, CCD_CONTEXT->preview_image_size = jpeg_size);
					}
				} else {
					CCD_CONTEXT->preview_image = indigo_safe_malloc(CCD_CONTEXT->preview_image_size = jpeg_size);
				}
				memcpy(CCD_CONTEXT->preview_image, jpeg_data, jpeg_size);
				CCD_PREVIEW_IMAGE_ITEM->blob.value = CCD_CONTEXT->preview_image;
				CCD_PREVIEW_IMAGE_ITEM->blob.size = jpeg_size;
				strcpy(CCD_PREVIEW_IMAGE_ITEM->blob.format, ".jpeg");
				CCD_PREVIEW_IMAGE_PROPERTY->state = INDIGO_OK_STATE;
			} else {
				CCD_PREVIEW_IMAGE_PROPERTY->state = INDIGO_ALERT_STATE;
			}
			indigo_update_property(device, CCD_PREVIEW_IMAGE_PROPERTY, NULL);
			if (CCD_PREVIEW_ENABLED_WITH_HISTOGRAM_ITEM->sw.value) {
				CCD_PREVIEW_HISTOGRAM_PROPERTY->state = INDIGO_BUSY_STATE;
				indigo_update_property(device, CCD_PREVIEW_HISTOGRAM_PROPERTY, NULL);
				if (histogram_data) {
					if (CCD_CONTEXT->preview_histogram) {
						if (CCD_CONTEXT->preview_histogram_size < histogram_size) {
							CCD_CONTEXT->preview_histogram = indigo_safe_realloc(CCD_CONTEXT->preview_histogram, CCD_CONTEXT->preview_histogram_size = histogram_size);
						}
					} else {
						CCD_CONTEXT->preview_histogram = indigo_safe_malloc(CCD_CONTEXT->preview_histogram_size = histogram_size);
					}
					memcpy(CCD_CONTEXT->preview_histogram, histogram_data, histogram_size);
					CCD_PREVIEW_HISTOGRAM_ITEM->blob.value = CCD_CONTEXT->preview_histogram;
					CCD_PREVIEW_HISTOGRAM_ITEM->blob.size = histogram_size;
					strcpy(CCD_PREVIEW_HISTOGRAM_ITEM->blob.format, ".jpeg");
					CCD_PREVIEW_HISTOGRAM_PROPERTY->state = INDIGO_OK_STATE;
				} else {
					CCD_PREVIEW_HISTOGRAM_PROPERTY->state = INDIGO_ALERT_STATE;
				}
				indigo_update_property(device, CCD_PREVIEW_HISTOGRAM_PROPERTY, NULL);
			}
		}
	}

	if (CCD_IMAGE_FORMAT_FITS_ITEM->sw.value) {
		INDIGO_DEBUG(clock_t start = clock());
		time_t timer;
		struct tm* tm_info;
		char date_time_end[20];
		time(&timer);
		timer -= CCD_EXPOSURE_ITEM->number.target;
		tm_info = gmtime(&timer);
		strftime(date_time_end, 20, "%Y-%m-%dT%H:%M:%S", tm_info);
		char *header = data;
		memset(header, ' ', FITS_HEADER_SIZE);
		add_key(&header, true,  "SIMPLE  =                    T / file conforms to FITS standard");
		if (bpp == 8 || bpp == 16) {
			add_key(&header, true,  "BITPIX  = %20d / number of bits per data pixel", bpp);
			add_key(&header, true,  "NAXIS   =                    %d / number of data axes", 2);
			add_key(&header, true,  "NAXIS1  = %20d / length of data axis 1 [pixels]", frame_width);
			add_key(&header, true,  "NAXIS2  = %20d / length of data axis 2 [pixels]", frame_height);
		} else {
			add_key(&header, true,  "BITPIX  = %20d / number of bits per data pixel", bpp / 3);
			add_key(&header, true,  "NAXIS   =                    %d / number of data axes", 3);
			add_key(&header, true,  "NAXIS1  = %20d / length of data axis 1 [pixels]", frame_width);
			add_key(&header, true,  "NAXIS2  = %20d / length of data axis 2 [pixels]", frame_height);
			add_key(&header, true,  "NAXIS3  = %20d / length of data axis 3 [RGB]\n", 3);
		}
		add_key(&header, true,  "EXTEND  =                    T / FITS dataset may contain extensions");
		if (bpp == 16 || bpp == 48) {
			add_key(&header, true,  "BZERO   =                32768 / offset data range to that of unsigned short");
			add_key(&header, true,  "BSCALE  =                    1 / default scaling factor");
		}
		add_key(&header, true,  "XBINNING= %20d / horizontal binning [pixels]", horizontal_bin);
		add_key(&header, true,  "YBINNING= %20d / vertical binning [pixels]", vertical_bin);
		if (CCD_INFO_PIXEL_WIDTH_ITEM->number.value > 0 && CCD_INFO_PIXEL_HEIGHT_ITEM->number.value) {
			add_key(&header, true,  "XPIXSZ  = %20.2f / pixel width [microns]", CCD_INFO_PIXEL_WIDTH_ITEM->number.value * horizontal_bin);
			add_key(&header, true,  "YPIXSZ  = %20.2f / pixel height [microns]", CCD_INFO_PIXEL_HEIGHT_ITEM->number.value * vertical_bin);
		}
		add_key(&header, true,  "EXPTIME = %20.2f / exposure time [s]", CCD_EXPOSURE_ITEM->number.target);
		if (!CCD_TEMPERATURE_PROPERTY->hidden)
			add_key(&header, true,  "CCD-TEMP= %20.2f / CCD temperature [C]", CCD_TEMPERATURE_ITEM->number.value);
		if (CCD_FRAME_TYPE_LIGHT_ITEM->sw.value)
			add_key(&header, true,  "IMAGETYP= 'Light'               / frame type");
		else if (CCD_FRAME_TYPE_FLAT_ITEM->sw.value)
			add_key(&header, true,  "IMAGETYP= 'Flat'                / frame type");
		else if (CCD_FRAME_TYPE_BIAS_ITEM->sw.value)
			add_key(&header, true,  "IMAGETYP= 'Bias'                / frame type");
		else if (CCD_FRAME_TYPE_DARK_ITEM->sw.value)
			add_key(&header, true,  "IMAGETYP= 'Dark'                / frame type");
		else if (CCD_FRAME_TYPE_DARKFLAT_ITEM->sw.value)
			add_key(&header, true,  "IMAGETYP= 'DarkFlat'            / frame type");
		if (!CCD_GAIN_PROPERTY->hidden)
			add_key(&header, true,  "GAIN    = %20.2f / Gain", CCD_GAIN_ITEM->number.value);
		if (!CCD_OFFSET_PROPERTY->hidden)
			add_key(&header, true,  "OFFSET  = %20.2f / Offset", CCD_OFFSET_ITEM->number.value);
		if (!CCD_GAMMA_PROPERTY->hidden)
			add_key(&header, true,  "GAMMA   = %20.2f / Gamma", CCD_GAMMA_ITEM->number.value);
		add_key(&header, true,  "DATE-OBS= '%s' / UTC date that FITS file was created", date_time_end);
		add_key(&header, true,  "INSTRUME= '%s'%*c / instrument name", device->name, (int)(19 - strlen(device->name)), ' ');
		add_key(&header, true,  "ROWORDER= 'TOP-DOWN'           / Image row order");
		if (keywords) {
			while (keywords->type && (header - (char *)data) < (FITS_HEADER_SIZE - 80)) {
				switch (keywords->type) {
					case INDIGO_FITS_NUMBER:
						add_key(&header, true,  "%7s= %20f / %s", keywords->name, keywords->number, keywords->comment);
						break;
					case INDIGO_FITS_STRING:
						add_key(&header, true,  "%7s= '%s'%*c / %s", keywords->name, keywords->string, (int)(18 - strlen(keywords->string)), ' ', keywords->comment);
						break;
					case INDIGO_FITS_LOGICAL:
						add_key(&header, true,  "%7s=                    %c / %s", keywords->name, keywords->logical ? 'T' : 'F', keywords->comment);
						break;
				}
				keywords++;
			}
		}
		for (int i = 0; i < CCD_FITS_HEADERS_PROPERTY->count; i++) {
			indigo_item *item = CCD_FITS_HEADERS_PROPERTY->items + i;
			if (*item->text.value && (header - (char *)data) < (FITS_HEADER_SIZE - 80)) {
				add_key(&header, true, "%s", item->text.value);
			}
		}
		add_key(&header, true,  "END");
		if (byte_per_pixel == 2 && naxis == 2) {
			uint16_t *raw = (uint16_t *)(data + FITS_HEADER_SIZE);
			if (little_endian) {
				for (int i = 0; i < size; i++) {
					int value = *raw - 32768;
					*raw++ = (value & 0xff) << 8 | (value & 0xff00) >> 8;
				}
			} else {
				for (int i = 0; i < size; i++) {
					int value = *raw;
					value = ((value & 0xff) << 8 | (value & 0xff00) >> 8) - 32768;
					*raw++ = (value & 0xff) << 8 | (value & 0xff00) >> 8;
				}
			}
		} else if (byte_per_pixel == 1 && naxis == 3) {
			unsigned char *raw = indigo_safe_malloc(3 * size);
			unsigned char *red = raw;
			unsigned char *green = raw + size;
			unsigned char *blue = raw + 2 * size;
			unsigned char *tmp = data + FITS_HEADER_SIZE;
			if (byte_order_rgb) {
				for (int i = 0; i < size; i++) {
					*red++ = *tmp++;
					*green++ = *tmp++;
					*blue++ = *tmp++;
				}
			} else {
				for (int i = 0; i < size; i++) {
					*blue++ = *tmp++;
					*green++ = *tmp++;
					*red++ = *tmp++;
				}
			}
			memcpy(data + FITS_HEADER_SIZE, raw, 3 * size);
			free(raw);
		} else if (byte_per_pixel == 2 && naxis == 3) {
			uint16_t *raw = indigo_safe_malloc(6 * size);
			uint16_t *red = raw;
			uint16_t *green = raw + size;
			uint16_t *blue = raw + 2 * size;
			uint16_t *tmp = (uint16_t *)(data + FITS_HEADER_SIZE);
			if (little_endian) {
				if (byte_order_rgb) {
					for (int i = 0; i < size; i++) {
						int value = *tmp++ - 32768;
						*red++ = (value & 0xff) << 8 | (value & 0xff00) >> 8;
						value = *tmp++ - 32768;
						*green++ = (value & 0xff) << 8 | (value & 0xff00) >> 8;
						value = *tmp++ - 32768;
						*blue++ = (value & 0xff) << 8 | (value & 0xff00) >> 8;
					}
				} else {
					for (int i = 0; i < size; i++) {
						int value = *tmp++ - 32768;
						*blue++ = (value & 0xff) << 8 | (value & 0xff00) >> 8;
						value = *tmp++ - 32768;
						*green++ = (value & 0xff) << 8 | (value & 0xff00) >> 8;
						value = *tmp++ - 32768;
						*red++ = (value & 0xff) << 8 | (value & 0xff00) >> 8;
					}
				}
			} else {
				if (byte_order_rgb) {
					for (int i = 0; i < size; i++) {
						*red++ = *tmp++;
						*green++ = *tmp++;
						*blue++ = *tmp++;
					}
				} else {
					for (int i = 0; i < size; i++) {
						*blue++ = *tmp++;
						*green++ = *tmp++;
						*red++ = *tmp++;
					}
				}
			}
			memcpy(data + FITS_HEADER_SIZE, raw, 6 * size);
			free(raw);
		}
		int mod2880 = blobsize % 2880;
		if (mod2880) {
			int padding = 2880 - mod2880;
			if (padding) {
				memset(data + FITS_HEADER_SIZE + blobsize, 0, padding);
				blobsize += padding;
			}
		}
		INDIGO_DEBUG(indigo_debug("RAW to FITS conversion in %gs", (clock() - start) / (double)CLOCKS_PER_SEC));
	} else if (CCD_IMAGE_FORMAT_XISF_ITEM->sw.value) {
		INDIGO_DEBUG(clock_t start = clock());
		time_t timer;
		struct tm* tm_info;
		char date_time_end[21], date_time_start[21], fits_date_obs[21];
		time(&timer);
		tm_info = gmtime(&timer);
		strftime(date_time_end, 21, "%Y-%m-%dT%H:%M:%SZ", tm_info);
		timer -= CCD_EXPOSURE_ITEM->number.target;
		tm_info = gmtime(&timer);
		strftime(date_time_start, 21, "%Y-%m-%dT%H:%M:%SZ", tm_info);
		strftime(fits_date_obs, 21, "%Y-%m-%dT%H:%M:%S", tm_info);
		char *header = data;
		strcpy(header, "XISF0100");
		header += 16;
		memset(header, 0, FITS_HEADER_SIZE);
		sprintf(header, "<?xml version='1.0' encoding='UTF-8'?><xisf xmlns='http://www.pixinsight.com/xisf' xmlns:xsi='http://www.w3.org/2001/XMLSchema-instance' version='1.0' xsi:schemaLocation='http://www.pixinsight.com/xisf http://pixinsight.com/xisf/xisf-1.0.xsd'>");
		header += strlen(header);
		char *frame_type = "Light";
		char b1[32], b2[32];
		if (CCD_FRAME_TYPE_FLAT_ITEM->sw.value)
			frame_type ="Flat";
		else if (CCD_FRAME_TYPE_BIAS_ITEM->sw.value)
			frame_type ="Bias";
		else if (CCD_FRAME_TYPE_DARK_ITEM->sw.value)
			frame_type ="Dark";
		else if (CCD_FRAME_TYPE_DARKFLAT_ITEM->sw.value)
			frame_type ="DarkFlat";
		if (naxis == 2 && byte_per_pixel == 1) {
			sprintf(header, "<Image geometry='%d:%d:1' imageType='%s' sampleFormat='UInt8' colorSpace='Gray' location='attachment:%d:%lu'>", frame_width, frame_height, frame_type, FITS_HEADER_SIZE, blobsize);
		} else if (naxis == 2 && byte_per_pixel == 2) {
			sprintf(header, "<Image geometry='%d:%d:1' imageType='%s' sampleFormat='UInt16' colorSpace='Gray' location='attachment:%d:%lu'>", frame_width, frame_height, frame_type, FITS_HEADER_SIZE, blobsize);
		} else if (naxis == 3 && byte_per_pixel == 1) {
			sprintf(header, "<Image geometry='%d:%d:3' imageType='%s' pixelStorage='Normal' sampleFormat='UInt8' colorSpace='RGB' location='attachment:%d:%lu'>", frame_width, frame_height, frame_type, FITS_HEADER_SIZE, blobsize);
		} else if (naxis == 3 && byte_per_pixel == 2) {
			sprintf(header, "<Image geometry='%d:%d:3' imageType='%s' pixelStorage='Normal' sampleFormat='UInt16' colorSpace='RGB' location='attachment:%d:%lu'>", frame_width, frame_height, frame_type, FITS_HEADER_SIZE, blobsize);
		}
		header += strlen(header);
		sprintf(header, "<FITSKeyword name='IMAGETYP' value='%s' comment='Frame type'/>", frame_type);
		header += strlen(header);
		sprintf(header, "<Property id='Observation:Time:Start' type='TimePoint' value='%s'/><Property id='Observation:Time:End' type='TimePoint' value='%s'/>", date_time_start ,date_time_end);
		header += strlen(header);
		sprintf(header, "<FITSKeyword name='DATE-OBS' value='%s' comment='Observation start time, UT'/>", fits_date_obs);
		header += strlen(header);
		sprintf(header, "<Property id='Instrument:Camera:Name' type='String'>%s</Property>", device->name);
		header += strlen(header);
		sprintf(header, "<FITSKeyword name='INSTRUME' value='%s' comment='Instrument'/>", device->name);
		header += strlen(header);
		sprintf(header, "<Property id='Instrument:Camera:XBinning' type='Int32' value='%d'/><Property id='Instrument:Camera:YBinning' type='Int32' value='%d'/>", horizontal_bin, vertical_bin);
		header += strlen(header);
		sprintf(header, "<FITSKeyword name='XBINNING' value='%d' comment='Binning factor, X-axis'/><FITSKeyword name='YBINNING' value='%d' comment='Binning factor, Y-axis'/>", horizontal_bin, vertical_bin);
		header += strlen(header);
		sprintf(header, "<Property id='Instrument:ExposureTime' type='Float32' value='%s'/>", indigo_dtoa(CCD_EXPOSURE_ITEM->number.target, b1));
		header += strlen(header);
		sprintf(header, "<FITSKeyword name='EXPTIME'  value='%20.2f' comment='Exposure time in seconds'/>", CCD_EXPOSURE_ITEM->number.target);
		header += strlen(header);
		sprintf(header, "<Property id='Instrument:Sensor:XPixelSize' type='Float32' value='%s'/><Property id='Instrument:Sensor:YPixelSize' type='Float32' value='%s'/>", indigo_dtoa(CCD_INFO_PIXEL_WIDTH_ITEM->number.value * horizontal_bin, b1), indigo_dtoa(CCD_INFO_PIXEL_HEIGHT_ITEM->number.value * vertical_bin, b2));
		header += strlen(header);
		sprintf(header, "<FITSKeyword name='XPIXSZ'  value='%20.2f' comment='Pixel horizontal width in microns'/><FITSKeyword name='YPIXSZ' value='%20.2f' comment='Pixel vertical width in microns'/>", CCD_INFO_PIXEL_WIDTH_ITEM->number.value * horizontal_bin, CCD_INFO_PIXEL_HEIGHT_ITEM->number.value * vertical_bin);
		header += strlen(header);

		if (!CCD_TEMPERATURE_PROPERTY->hidden) {
			sprintf(header, "<Property id='Instrument:Sensor:Temperature' type='Float32' value='%s'/><Property id='Instrument:Sensor:TargetTemperature' type='Float32' value='%s'/>", indigo_dtoa(CCD_TEMPERATURE_ITEM->number.value, b1), indigo_dtoa(CCD_TEMPERATURE_ITEM->number.target, b2));
			header += strlen(header);
			sprintf(header, "<FITSKeyword name='CCD-TEMP' value='%20.2f' comment='CCD chip temperature in celsius'/>", CCD_TEMPERATURE_ITEM->number.value);
			header += strlen(header);

		}
		if (!CCD_GAIN_PROPERTY->hidden) {
			sprintf(header, "<Property id='Instrument:Camera:Gain' type='Float32' value='%s'/>", indigo_dtoa(CCD_GAIN_ITEM->number.value, b1));
			header += strlen(header);
			sprintf(header, "<FITSKeyword name='GAIN' value='%20.2f' comment='Gain'/>", CCD_GAIN_ITEM->number.value);
			header += strlen(header);
		}
		if (!CCD_OFFSET_PROPERTY->hidden) {
			sprintf(header, "<Property id='Instrument:Camera:Offset' type='Float32' value='%s'/>", indigo_dtoa(CCD_OFFSET_ITEM->number.value, b1));
			header += strlen(header);
			sprintf(header, "<FITSKeyword name='OFFSET' value='%20.2f' comment='Offset'/>", CCD_OFFSET_ITEM->number.value);
			header += strlen(header);
		}
		if (!CCD_GAMMA_PROPERTY->hidden) {
			sprintf(header, "<Property id='Instrument:Camera:Gamma' type='Float32' value='%s'/>", indigo_dtoa(CCD_GAMMA_ITEM->number.value, b1));
			header += strlen(header);
			sprintf(header, "<FITSKeyword name='GAMMA' value='%20.2f' comment='Gamma'/>", CCD_GAMMA_ITEM->number.value);
			header += strlen(header);
		}
		for (int i = 0; i < CCD_FITS_HEADERS_PROPERTY->count; i++) {
			indigo_item *item = CCD_FITS_HEADERS_PROPERTY->items + i;
			if (!strncmp(item->text.value, "FILTER  =", 9)) {
				sprintf(header, "<Property id='Instrument:Filter:Name' type='String' value=%s/>", item->text.value + 10);
				header += strlen(header);
				sprintf(header, "<FITSKeyword name='FILTER' value=%s comment='Name of the used filter'/>", item->text.value + 10);
				header += strlen(header);
			} else if (!strncmp(item->text.value, "FOCUS   =", 9)) {
				sprintf(header, "<Property id='Instrument:Focuser:Position' type='String' value='%s'/>", item->text.value + 10);
				header += strlen(header);
				sprintf(header, "<FITSKeyword name='FOCUS' value='%s' comment='Focuser position'/>", item->text.value + 10);
				header += strlen(header);
			}
		}
		if (keywords) {
			while (keywords->type) {
				if (!strcmp(keywords->name, "BAYERPAT")) {
					sprintf(header, "<ColorFilterArray pattern='%s' width='2' height='2'/>", keywords->string);
					header += strlen(header);
				}
				keywords++;
			}
		}
		sprintf(header, "</Image><Metadata><Property id='XISF:CreationTime' type='String'>%s</Property><Property id='XISF:CreatorApplication' type='String'>INDIGO 2.0-%s</Property>", date_time_end, INDIGO_BUILD);
		header += strlen(header);
#ifdef INDIGO_LINUX
		sprintf(header, "<Property id='XISF:CreatorOS' type='String'>Linux</Property>");
#endif
#ifdef INDIGO_MACOS
		sprintf(header, "<Property id='XISF:CreatorOS' type='String'>macOS</Property>");
#endif
#ifdef INDIGO_WINDOWS
		sprintf(header, "<Property id='XISF:CreatorOS' type='String'>Windows</Property>");
#endif
		header += strlen(header);
		sprintf(header, "<Property id='XISF:BlockAlignmentSize' type='UInt16' value='2880'/></Metadata></xisf>");
		header += strlen(header);
		*(uint32_t *)(data + 8) = (uint32_t)(header - (char *)data) - 16;
		if (naxis == 2 && byte_per_pixel == 2) {
			if (!little_endian) {
				uint16_t *b16 = (uint16_t *)(data + FITS_HEADER_SIZE);
				for (int i = 0; i < size; i++) {
					int value = *b16;
					*b16++ = (value & 0xff) << 8 | (value & 0xff00) >> 8;
				}
			}
		} else if (naxis == 3 && byte_per_pixel == 1) {
			if (!byte_order_rgb) {
				unsigned char *b8 = data + FITS_HEADER_SIZE;
				for (int i = 0; i < size; i++) {
					unsigned char b = *b8;
					unsigned char r = *(b8 + 2);
					*b8 = r;
					*(b8 + 2) = b;
					b8 += 3;
				}
			}
		} else if (naxis == 3 && byte_per_pixel == 2) {
			unsigned char *b16 = data + FITS_HEADER_SIZE;
			if (little_endian) {
				if (!byte_order_rgb) {
					for (int i = 0; i < size; i++) {
						unsigned char b = *b16;
						unsigned char r = *(b16 + 2);
						*b16 = r;
						*(b16 + 2) = b;
						b16 += 3;
					}
				}
			} else {
				if (byte_order_rgb) {
					for (int i = 0; i < size; i++) {
						int value = *b16;
						*b16++ = (value & 0xff) << 8 | (value & 0xff00) >> 8;
					}
				} else {
					for (int i = 0; i < size; i++) {
						int value = *b16;
						unsigned b = (value & 0xff) << 8 | (value & 0xff00) >> 8;
						value = *(b16 + 1);
						unsigned g = (value & 0xff) << 8 | (value & 0xff00) >> 8;
						value = *(b16 + 2);
						unsigned r = (value & 0xff) << 8 | (value & 0xff00) >> 8;
						*b16 = r;
						*(b16 + 1) = g;
						*(b16 + 2) = b;
						b16 += 3;
					}
				}
			}
		}
		INDIGO_DEBUG(indigo_debug("RAW to XISF conversion in %gs", (clock() - start) / (double)CLOCKS_PER_SEC));
	} else if (CCD_IMAGE_FORMAT_RAW_ITEM->sw.value || CCD_IMAGE_FORMAT_RAW_SER_ITEM->sw.value) {
		indigo_raw_header *header = (indigo_raw_header *)(data + FITS_HEADER_SIZE - sizeof(indigo_raw_header));
		if (naxis == 2 && byte_per_pixel == 1)
			header->signature = INDIGO_RAW_MONO8;
		else if (naxis == 2 && byte_per_pixel == 2) {
			header->signature = INDIGO_RAW_MONO16;
			if (!little_endian) {
				uint16_t *b16 = (uint16_t *)(data + FITS_HEADER_SIZE);
				for (int i = 0; i < size; i++) {
					int value = *b16;
					*b16++ = (value & 0xff) << 8 | (value & 0xff00) >> 8;
				}
			}
		} else if (naxis == 3 && byte_per_pixel == 1) {
			header->signature = INDIGO_RAW_RGB24;
			if (!byte_order_rgb) {
				unsigned char *b8 = data + FITS_HEADER_SIZE;
				for (int i = 0; i < size; i++) {
					unsigned char b = *b8;
					unsigned char r = *(b8 + 2);
					*b8 = r;
					*(b8 + 2) = b;
					b8 += 3;
				}
			}
		} else if (naxis == 3 && byte_per_pixel == 2) {
			header->signature = INDIGO_RAW_RGB48;
			unsigned char *b16 = data + FITS_HEADER_SIZE;
			if (little_endian) {
				if (!byte_order_rgb) {
					for (int i = 0; i < size; i++) {
						unsigned char b = *b16;
						unsigned char r = *(b16 + 2);
						*b16 = r;
						*(b16 + 2) = b;
						b16 += 3;
					}
				}
			} else {
				if (byte_order_rgb) {
					for (int i = 0; i < size; i++) {
						int value = *b16;
						*b16++ = (value & 0xff) << 8 | (value & 0xff00) >> 8;
					}
				} else {
					for (int i = 0; i < size; i++) {
						int value = *b16;
						unsigned b = (value & 0xff) << 8 | (value & 0xff00) >> 8;
						value = *(b16 + 1);
						unsigned g = (value & 0xff) << 8 | (value & 0xff00) >> 8;
						value = *(b16 + 2);
						unsigned r = (value & 0xff) << 8 | (value & 0xff00) >> 8;
						*b16 = r;
						*(b16 + 1) = g;
						*(b16 + 2) = b;
						b16 += 3;
					}
				}
			}
		}
		header->width = frame_width;
		header->height = frame_height;
	} else if (CCD_IMAGE_FORMAT_JPEG_ITEM->sw.value || CCD_IMAGE_FORMAT_JPEG_AVI_ITEM->sw.value) {
		if (jpeg_data && jpeg_size < blobsize + FITS_HEADER_SIZE) {
			memcpy(data, jpeg_data, jpeg_size);
			blobsize = jpeg_size;
		} else {
			indigo_error("JPEG Size > BLOB Size");
		}
	} else if (CCD_IMAGE_FORMAT_TIFF_ITEM->sw.value) {
		void *tiff_data = NULL;
		unsigned long tiff_size = 0;
		raw_to_tiff(device, data, frame_width, frame_height, bpp, little_endian, byte_order_rgb, &tiff_data, &tiff_size, keywords);
		if (tiff_data) {
			if (tiff_size < blobsize + FITS_HEADER_SIZE) {
				memcpy(data, tiff_data, tiff_size);
				blobsize = tiff_size;
			} else {
				indigo_error("TIFF Size > BLOB Size");
			}
			free(tiff_data);
		}
	}
	if (CCD_UPLOAD_MODE_LOCAL_ITEM->sw.value || CCD_UPLOAD_MODE_BOTH_ITEM->sw.value) {
		char *suffix = "";
		bool use_avi = false;
		bool use_ser = false;
		if (CCD_IMAGE_FORMAT_FITS_ITEM->sw.value) {
			suffix = ".fits";
		} else if (CCD_IMAGE_FORMAT_XISF_ITEM->sw.value) {
			suffix = ".xisf";
		} else if (CCD_IMAGE_FORMAT_RAW_ITEM->sw.value) {
			suffix = ".raw";
		} else if (CCD_IMAGE_FORMAT_JPEG_ITEM->sw.value) {
			suffix = ".jpeg";
		} else if (CCD_IMAGE_FORMAT_TIFF_ITEM->sw.value) {
			suffix = ".tiff";
		} else if (CCD_IMAGE_FORMAT_JPEG_AVI_ITEM->sw.value) {
			if (streaming) {
				suffix = ".avi";
				use_avi = true;
			} else {
				suffix = ".jpeg";
			}
		} else if (CCD_IMAGE_FORMAT_RAW_SER_ITEM->sw.value) {
			if (streaming) {
				suffix = ".ser";
				use_ser = true;
			} else {
				suffix = ".raw";
			}
		}
		char *message = NULL;
		int handle = 0;
		if (!(use_avi || use_ser) || CCD_CONTEXT->video_stream == NULL) {
			char file_name[INDIGO_VALUE_SIZE];
			if (create_file_name(CCD_LOCAL_MODE_DIR_ITEM->text.value, CCD_LOCAL_MODE_PREFIX_ITEM->text.value, suffix, file_name)) {
				indigo_copy_value(CCD_IMAGE_FILE_ITEM->text.value, file_name);
				CCD_IMAGE_FILE_PROPERTY->state = INDIGO_OK_STATE;
				if (use_avi) {
					CCD_CONTEXT->video_stream = gwavi_open(file_name, frame_width, frame_height, "MJPG", 5);
				} else if (use_ser) {
					CCD_CONTEXT->video_stream = indigo_ser_open(file_name, data + FITS_HEADER_SIZE - sizeof(indigo_raw_header), little_endian, byte_order_rgb);
				} else {
					handle = open(file_name, O_WRONLY | O_CREAT | O_TRUNC, 0644);
				}
			} else {
				CCD_IMAGE_FILE_PROPERTY->state = INDIGO_ALERT_STATE;
				message = "dir + prefix + suffix is too long";
			}
		}
		if (CCD_CONTEXT->video_stream != NULL) {
			if (use_avi) {
				if (!gwavi_add_frame((struct gwavi_t *)(CCD_CONTEXT->video_stream), data, blobsize)) {
					CCD_IMAGE_FILE_PROPERTY->state = INDIGO_ALERT_STATE;
					message = strerror(errno);
				}
			} else if (use_ser) {
				if (!indigo_ser_add_frame((indigo_ser *)(CCD_CONTEXT->video_stream), data + FITS_HEADER_SIZE - sizeof(indigo_raw_header), blobsize + sizeof(indigo_raw_header))) {
					CCD_IMAGE_FILE_PROPERTY->state = INDIGO_ALERT_STATE;
					message = strerror(errno);
				}
			}
		} else if (handle > 0) {
			if (CCD_IMAGE_FORMAT_FITS_ITEM->sw.value) {
				if (!indigo_write(handle, data, FITS_HEADER_SIZE + blobsize)) {
					CCD_IMAGE_FILE_PROPERTY->state = INDIGO_ALERT_STATE;
					message = strerror(errno);
				}
			} else if (CCD_IMAGE_FORMAT_XISF_ITEM->sw.value) {
				if (!indigo_write(handle, data, FITS_HEADER_SIZE + blobsize)) {
					CCD_IMAGE_FILE_PROPERTY->state = INDIGO_ALERT_STATE;
					message = strerror(errno);
				}
			} else if (CCD_IMAGE_FORMAT_RAW_ITEM->sw.value || CCD_IMAGE_FORMAT_RAW_SER_ITEM->sw.value) {
				if (!indigo_write(handle, data + FITS_HEADER_SIZE - sizeof(indigo_raw_header), blobsize + sizeof(indigo_raw_header))) {
					CCD_IMAGE_FILE_PROPERTY->state = INDIGO_ALERT_STATE;
					message = strerror(errno);
				}
			} else if (CCD_IMAGE_FORMAT_JPEG_ITEM->sw.value || CCD_IMAGE_FORMAT_JPEG_AVI_ITEM->sw.value) {
				if (!indigo_write(handle, data, blobsize)) {
					CCD_IMAGE_FILE_PROPERTY->state = INDIGO_ALERT_STATE;
					message = strerror(errno);
				}
			} else if (CCD_IMAGE_FORMAT_TIFF_ITEM->sw.value) {
				if (!indigo_write(handle, data, blobsize)) {
					CCD_IMAGE_FILE_PROPERTY->state = INDIGO_ALERT_STATE;
					message = strerror(errno);
				}
			}
			close(handle);
		} else {
			CCD_IMAGE_FILE_PROPERTY->state = INDIGO_ALERT_STATE;
			message = strerror(errno);
		}
		indigo_update_property(device, CCD_IMAGE_FILE_PROPERTY, message);
		INDIGO_DEBUG(indigo_debug("Local save in %gs", (clock() - start) / (double)CLOCKS_PER_SEC));
	}
	if (CCD_UPLOAD_MODE_CLIENT_ITEM->sw.value || CCD_UPLOAD_MODE_BOTH_ITEM->sw.value) {
		*CCD_IMAGE_ITEM->blob.url = 0;
		if (CCD_IMAGE_FORMAT_FITS_ITEM->sw.value) {
			CCD_IMAGE_ITEM->blob.value = data;
			CCD_IMAGE_ITEM->blob.size = FITS_HEADER_SIZE + blobsize;
			strcpy(CCD_IMAGE_ITEM->blob.format, ".fits");
		} else if (CCD_IMAGE_FORMAT_XISF_ITEM->sw.value) {
			CCD_IMAGE_ITEM->blob.value = data;
			CCD_IMAGE_ITEM->blob.size = FITS_HEADER_SIZE + blobsize;
			strcpy(CCD_IMAGE_ITEM->blob.format, ".xisf");
		} else if (CCD_IMAGE_FORMAT_RAW_ITEM->sw.value || CCD_IMAGE_FORMAT_RAW_SER_ITEM->sw.value) {
			CCD_IMAGE_ITEM->blob.value = data + FITS_HEADER_SIZE - sizeof(indigo_raw_header);
			CCD_IMAGE_ITEM->blob.size = blobsize + sizeof(indigo_raw_header);
			strcpy(CCD_IMAGE_ITEM->blob.format, ".raw");
		} else if (CCD_IMAGE_FORMAT_JPEG_ITEM->sw.value || CCD_IMAGE_FORMAT_JPEG_AVI_ITEM->sw.value) {
			CCD_IMAGE_ITEM->blob.value = data;
			CCD_IMAGE_ITEM->blob.size = blobsize;
			strcpy(CCD_IMAGE_ITEM->blob.format, ".jpeg");
		} else if (CCD_IMAGE_FORMAT_TIFF_ITEM->sw.value) {
			CCD_IMAGE_ITEM->blob.value = data;
			CCD_IMAGE_ITEM->blob.size = blobsize;
			strcpy(CCD_IMAGE_ITEM->blob.format, ".tiff");
		}
		CCD_IMAGE_PROPERTY->state = INDIGO_OK_STATE;
		indigo_update_property(device, CCD_IMAGE_PROPERTY, NULL);
		INDIGO_DEBUG(indigo_debug("Client upload in %gs", (clock() - start) / (double)CLOCKS_PER_SEC));
	}
	if (jpeg_data)
		free(jpeg_data);
	if (histogram_data)
		free(histogram_data);
}

void indigo_process_dslr_image(indigo_device *device, void *data, int data_size, const char *suffix, bool streaming) {
	assert(device != NULL);
	assert(data != NULL);
	INDIGO_DEBUG(clock_t start = clock());
	char standard_suffix[16];
	strncpy(standard_suffix, suffix, sizeof(standard_suffix));
	for (char *pnt = standard_suffix; *pnt; pnt++)
		*pnt = tolower(*pnt);
	if (!strcmp(standard_suffix, ".jpg"))
		strcpy(standard_suffix, ".jpeg");
	if (CCD_IMAGE_FORMAT_RAW_ITEM->sw.value && !strcmp(standard_suffix, ".jpeg")) {
		void *image = NULL;
		struct indigo_jpeg_decompress_struct cinfo;
		struct jpeg_error_mgr jerr;
		cinfo.pub.err = jpeg_std_error(&jerr);
		/* override default exit() and return 2 lines below */
		jerr.error_exit = jpeg_decompress_error_callback;
		/* Jump here in case of a decmpression error */
		if (setjmp(cinfo.jpeg_error)) {
			if (image) {
				free(image);
				image = NULL;
			}
			jpeg_destroy_decompress(&cinfo.pub);
			INDIGO_ERROR(indigo_error("JPEG decompression failed"));
			CCD_IMAGE_PROPERTY->state = INDIGO_ALERT_STATE;
			indigo_update_property(device, CCD_IMAGE_PROPERTY, "JPEG decompression failed");
			return;
		}
		jpeg_create_decompress(&cinfo.pub);
		jpeg_mem_src(&cinfo.pub, data, data_size);
		if (jpeg_read_header(&cinfo.pub, TRUE) == 1) {
			jpeg_start_decompress(&cinfo.pub);
			int components = cinfo.pub.output_components;
			int frame_width = cinfo.pub.output_width;
			int frame_height = cinfo.pub.output_height;
			int row_stride = frame_width * components;
			int image_size = frame_height * row_stride;
			image = indigo_safe_malloc(image_size + FITS_HEADER_SIZE);
			while (cinfo.pub.output_scanline < cinfo.pub.output_height) {
				unsigned char *buffer_array[1];
				buffer_array[0] = image + FITS_HEADER_SIZE + (cinfo.pub.output_scanline) * row_stride;
				jpeg_read_scanlines(&cinfo.pub, buffer_array, 1);
			}
			jpeg_finish_decompress(&cinfo.pub);
			jpeg_destroy_decompress(&cinfo.pub);
			//indigo_process_image(device, intermediate_image, frame_width, frame_height, components * 8, true, true, NULL, streaming);
			indigo_raw_header *header = (indigo_raw_header *)(image + FITS_HEADER_SIZE - sizeof(indigo_raw_header));
			header->signature = INDIGO_RAW_RGB24;
			header->width = frame_width;
			header->height = frame_height;
			if (CCD_UPLOAD_MODE_LOCAL_ITEM->sw.value || CCD_UPLOAD_MODE_BOTH_ITEM->sw.value) {
				char file_name[INDIGO_VALUE_SIZE];
				char *message = NULL;
				if (create_file_name(CCD_LOCAL_MODE_DIR_ITEM->text.value, CCD_LOCAL_MODE_PREFIX_ITEM->text.value, ".raw", file_name)) {
					indigo_copy_value(CCD_IMAGE_FILE_ITEM->text.value, file_name);
					CCD_IMAGE_FILE_PROPERTY->state = INDIGO_OK_STATE;
					int handle = open(file_name, O_WRONLY | O_CREAT | O_TRUNC, 0644);
					if (handle) {
						if (!indigo_write(handle, image + FITS_HEADER_SIZE - sizeof(indigo_raw_header), image_size + sizeof(indigo_raw_header))) {
							CCD_IMAGE_FILE_PROPERTY->state = INDIGO_ALERT_STATE;
							message = strerror(errno);
						}
						close(handle);
					} else {
						CCD_IMAGE_FILE_PROPERTY->state = INDIGO_ALERT_STATE;
						message = strerror(errno);
					}
				} else {
					CCD_IMAGE_FILE_PROPERTY->state = INDIGO_ALERT_STATE;
					message = "dir + prefix + suffix is too long";
				}
				indigo_update_property(device, CCD_IMAGE_FILE_PROPERTY, message);
				INDIGO_DEBUG(indigo_debug("Local save in %gs", (clock() - start) / (double)CLOCKS_PER_SEC));
			}
			if (CCD_UPLOAD_MODE_CLIENT_ITEM->sw.value || CCD_UPLOAD_MODE_BOTH_ITEM->sw.value) {
				*CCD_IMAGE_ITEM->blob.url = 0;
				CCD_IMAGE_ITEM->blob.value = image + FITS_HEADER_SIZE - sizeof(indigo_raw_header);
				CCD_IMAGE_ITEM->blob.size = image_size + sizeof(indigo_raw_header);
				indigo_copy_name(CCD_IMAGE_ITEM->blob.format, ".raw");
				CCD_IMAGE_PROPERTY->state = INDIGO_OK_STATE;
				indigo_update_property(device, CCD_IMAGE_PROPERTY, NULL);
				INDIGO_DEBUG(indigo_debug("Client upload in %gs", (clock() - start) / (double)CLOCKS_PER_SEC));
			}
			free(image);
			return;
		}
	}
	if (CCD_UPLOAD_MODE_LOCAL_ITEM->sw.value || CCD_UPLOAD_MODE_BOTH_ITEM->sw.value) {
		bool use_avi = false;
		int handle = 0;
		char *message = NULL;
		if (CCD_IMAGE_FORMAT_NATIVE_AVI_ITEM->sw.value && !strcmp(standard_suffix, ".jpeg") && streaming) {
			strcpy(standard_suffix, ".avi");
			use_avi = true;
		}
		if (!use_avi || CCD_CONTEXT->video_stream == NULL) {
			char file_name[INDIGO_VALUE_SIZE];
			if (create_file_name(CCD_LOCAL_MODE_DIR_ITEM->text.value, CCD_LOCAL_MODE_PREFIX_ITEM->text.value, standard_suffix, file_name)) {
				indigo_copy_value(CCD_IMAGE_FILE_ITEM->text.value, file_name);
				CCD_IMAGE_FILE_PROPERTY->state = INDIGO_OK_STATE;
				if (use_avi) {
					struct indigo_jpeg_decompress_struct cinfo;
					struct jpeg_error_mgr jerr;
					cinfo.pub.err = jpeg_std_error(&jerr);
					/* override default exit() and return 2 lines below */
					jerr.error_exit = jpeg_decompress_error_callback;
					/* Jump here in case of a decmpression error */
					if (setjmp(cinfo.jpeg_error)) {
						jpeg_destroy_decompress(&cinfo.pub);
						INDIGO_ERROR(indigo_error("JPEG decompression failed"));
						CCD_IMAGE_FILE_PROPERTY->state = INDIGO_ALERT_STATE;
						indigo_update_property(device, CCD_IMAGE_FILE_PROPERTY, "JPEG decompression failed");
						return;
					}
					jpeg_create_decompress(&cinfo.pub);
					jpeg_mem_src(&cinfo.pub, data, data_size);
					jpeg_read_header(&cinfo.pub, TRUE);
					jpeg_destroy_decompress(&cinfo.pub);
					CCD_CONTEXT->video_stream = gwavi_open(file_name, cinfo.pub.image_width, cinfo.pub.image_height, "MJPG", 5);
				} else {
					handle = open(file_name, O_WRONLY | O_CREAT | O_TRUNC, 0644);
				}
			} else {
				CCD_IMAGE_FILE_PROPERTY->state = INDIGO_ALERT_STATE;
				message = "dir + prefix + suffix is too long";
			}
		}
		if (CCD_CONTEXT->video_stream != NULL) {
			if (use_avi) {
				if (!gwavi_add_frame((struct gwavi_t *)(CCD_CONTEXT->video_stream), data, data_size)) {
					CCD_IMAGE_FILE_PROPERTY->state = INDIGO_ALERT_STATE;
					message = strerror(errno);
				}
			}
		} else if (handle) {
			if (!indigo_write(handle, data, data_size)) {
				CCD_IMAGE_FILE_PROPERTY->state = INDIGO_ALERT_STATE;
				message = strerror(errno);
			}
			close(handle);
		} else {
			CCD_IMAGE_FILE_PROPERTY->state = INDIGO_ALERT_STATE;
			message = strerror(errno);
		}
		indigo_update_property(device, CCD_IMAGE_FILE_PROPERTY, message);
		INDIGO_DEBUG(indigo_debug("Local save in %gs", (clock() - start) / (double)CLOCKS_PER_SEC));
	}
	if (CCD_UPLOAD_MODE_CLIENT_ITEM->sw.value || CCD_UPLOAD_MODE_BOTH_ITEM->sw.value) {
		*CCD_IMAGE_ITEM->blob.url = 0;
		CCD_IMAGE_ITEM->blob.value = data;
		CCD_IMAGE_ITEM->blob.size = data_size;
		indigo_copy_name(CCD_IMAGE_ITEM->blob.format, standard_suffix);
		CCD_IMAGE_PROPERTY->state = INDIGO_OK_STATE;
		indigo_update_property(device, CCD_IMAGE_PROPERTY, NULL);
		INDIGO_DEBUG(indigo_debug("Client upload in %gs", (clock() - start) / (double)CLOCKS_PER_SEC));
	}
}

void indigo_process_dslr_preview_image(indigo_device *device, void *data, int blobsize) {
	if (CCD_CONTEXT->preview_image) {
		if (CCD_CONTEXT->preview_image_size < blobsize) {
			CCD_CONTEXT->preview_image = indigo_safe_realloc(CCD_CONTEXT->preview_image, CCD_CONTEXT->preview_image_size = blobsize);
		}
	} else {
		CCD_CONTEXT->preview_image = indigo_safe_malloc(CCD_CONTEXT->preview_image_size = blobsize);
	}
	memcpy(CCD_CONTEXT->preview_image, data, blobsize);
	CCD_PREVIEW_IMAGE_ITEM->blob.value = CCD_CONTEXT->preview_image;
	CCD_PREVIEW_IMAGE_ITEM->blob.size = blobsize;
	strcpy(CCD_PREVIEW_IMAGE_ITEM->blob.format, ".jpeg");
	CCD_PREVIEW_IMAGE_PROPERTY->state = INDIGO_OK_STATE;
	indigo_update_property(device, CCD_PREVIEW_IMAGE_PROPERTY, NULL);
}

void indigo_finalize_video_stream(indigo_device *device) {
	if (CCD_CONTEXT->video_stream) {
		if (CCD_IMAGE_FORMAT_PROPERTY->count == 3) {
			if (CCD_IMAGE_FORMAT_NATIVE_AVI_ITEM->sw.value) {
				gwavi_close((struct gwavi_t *)(CCD_CONTEXT->video_stream));
				CCD_CONTEXT->video_stream = NULL;
				CCD_IMAGE_FILE_PROPERTY->state = INDIGO_OK_STATE;
				indigo_update_property(device, CCD_IMAGE_FILE_PROPERTY, NULL);
			}
		} else {
			if (CCD_IMAGE_FORMAT_JPEG_AVI_ITEM->sw.value) {
				gwavi_close((struct gwavi_t *)(CCD_CONTEXT->video_stream));
				CCD_CONTEXT->video_stream = NULL;
				CCD_IMAGE_FILE_PROPERTY->state = INDIGO_OK_STATE;
				indigo_update_property(device, CCD_IMAGE_FILE_PROPERTY, NULL);
			} else if (CCD_IMAGE_FORMAT_RAW_SER_ITEM->sw.value) {
				indigo_ser_close((indigo_ser *)(CCD_CONTEXT->video_stream));
				CCD_CONTEXT->video_stream = NULL;
				CCD_IMAGE_FILE_PROPERTY->state = INDIGO_OK_STATE;
				indigo_update_property(device, CCD_IMAGE_FILE_PROPERTY, NULL);
			}
		}
	}
}
