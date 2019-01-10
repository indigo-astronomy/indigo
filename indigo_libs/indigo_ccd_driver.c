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

#include "indigo_ccd_driver.h"
#include "indigo_io.h"

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
	CCD_CONTEXT->countdown_timer = indigo_set_timer(device, 1.0, countdown_timer_callback);
}

void indigo_use_shortest_exposure_if_bias(indigo_device *device) {
	if (CCD_FRAME_TYPE_BIAS_ITEM->sw.value) {
		CCD_EXPOSURE_ITEM->number.value = CCD_EXPOSURE_ITEM->number.target = CCD_EXPOSURE_ITEM->number.min;
		CCD_STREAMING_EXPOSURE_ITEM->number.value = CCD_STREAMING_EXPOSURE_ITEM->number.target = CCD_EXPOSURE_ITEM->number.min;
	}
}

indigo_result indigo_ccd_attach(indigo_device *device, unsigned version) {
	assert(device != NULL);
	if (CCD_CONTEXT == NULL) {
		device->device_context = malloc(sizeof(indigo_ccd_context));
		assert(DEVICE_CONTEXT != NULL);
		memset(device->device_context, 0, sizeof(indigo_ccd_context));
	}
	if (CCD_CONTEXT != NULL) {
		if (indigo_device_attach(device, version, INDIGO_INTERFACE_CCD) == INDIGO_OK) {
			// -------------------------------------------------------------------------------- CCD_INFO
			CCD_INFO_PROPERTY = indigo_init_number_property(NULL, device->name, CCD_INFO_PROPERTY_NAME, CCD_MAIN_GROUP, "Info", INDIGO_OK_STATE, INDIGO_RO_PERM, 8);
			if (CCD_INFO_PROPERTY == NULL)
				return INDIGO_FAILED;
			indigo_init_number_item(CCD_INFO_WIDTH_ITEM, CCD_INFO_WIDTH_ITEM_NAME, "Horizontal resolution", 0, 0, 0, 0);
			indigo_init_number_item(CCD_INFO_HEIGHT_ITEM, CCD_INFO_HEIGHT_ITEM_NAME, "Vertical resolution", 0, 0, 0, 0);
			indigo_init_number_item(CCD_INFO_MAX_HORIZONAL_BIN_ITEM, CCD_INFO_MAX_HORIZONTAL_BIN_ITEM_NAME, "Max vertical binning", 0, 0, 0, 1);
			indigo_init_number_item(CCD_INFO_MAX_VERTICAL_BIN_ITEM, CCD_INFO_MAX_VERTICAL_BIN_ITEM_NAME, "Max horizontal binning", 0, 0, 0, 1);
			indigo_init_number_item(CCD_INFO_PIXEL_SIZE_ITEM, CCD_INFO_PIXEL_SIZE_ITEM_NAME, "Pixel size", 0, 0, 0, 0);
			indigo_init_number_item(CCD_INFO_PIXEL_WIDTH_ITEM, CCD_INFO_PIXEL_WIDTH_ITEM_NAME, "Pixel width", 0, 0, 0, 0);
			indigo_init_number_item(CCD_INFO_PIXEL_HEIGHT_ITEM, CCD_INFO_PIXEL_HEIGHT_ITEM_NAME, "Pixel height", 0, 0, 0, 0);
			indigo_init_number_item(CCD_INFO_BITS_PER_PIXEL_ITEM, CCD_INFO_BITS_PER_PIXEL_ITEM_NAME, "Bits/pixel", 0, 0, 0, 0);
			// -------------------------------------------------------------------------------- CCD_UPLOAD_MODE
			CCD_UPLOAD_MODE_PROPERTY = indigo_init_switch_property(NULL, device->name, CCD_UPLOAD_MODE_PROPERTY_NAME, CCD_MAIN_GROUP, "Image upload", INDIGO_OK_STATE, INDIGO_RW_PERM, INDIGO_ONE_OF_MANY_RULE, 5);
			if (CCD_UPLOAD_MODE_PROPERTY == NULL)
				return INDIGO_FAILED;
			indigo_init_switch_item(CCD_UPLOAD_MODE_CLIENT_ITEM, CCD_UPLOAD_MODE_CLIENT_ITEM_NAME, "Upload to client", true);
			indigo_init_switch_item(CCD_UPLOAD_MODE_LOCAL_ITEM, CCD_UPLOAD_MODE_LOCAL_ITEM_NAME, "Save locally", false);
			indigo_init_switch_item(CCD_UPLOAD_MODE_BOTH_ITEM, CCD_UPLOAD_MODE_BOTH_ITEM_NAME, "Upload and save locally", false);
			indigo_init_switch_item(CCD_UPLOAD_MODE_PREVIEW_ITEM, CCD_UPLOAD_MODE_PREVIEW_ITEM_NAME, "Upload preview to client", false);
			indigo_init_switch_item(CCD_UPLOAD_MODE_PREVIEW_LOCAL_ITEM, CCD_UPLOAD_MODE_PREVIEW_LOCAL_ITEM_NAME, "Upload preview to client and save locally", false);
			// -------------------------------------------------------------------------------- CCD_LOCAL_MODE
			CCD_LOCAL_MODE_PROPERTY = indigo_init_text_property(NULL, device->name, CCD_LOCAL_MODE_PROPERTY_NAME, CCD_MAIN_GROUP, "Local mode", INDIGO_OK_STATE, INDIGO_RW_PERM, 2);
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
			CCD_FRAME_TYPE_PROPERTY = indigo_init_switch_property(NULL, device->name, CCD_FRAME_TYPE_PROPERTY_NAME, CCD_IMAGE_GROUP, "Frame type", INDIGO_OK_STATE, INDIGO_RW_PERM, INDIGO_ONE_OF_MANY_RULE, 4);
			if (CCD_FRAME_TYPE_PROPERTY == NULL)
				return INDIGO_FAILED;
			indigo_init_switch_item(CCD_FRAME_TYPE_LIGHT_ITEM, CCD_FRAME_TYPE_LIGHT_ITEM_NAME, "Light", true);
			indigo_init_switch_item(CCD_FRAME_TYPE_BIAS_ITEM, CCD_FRAME_TYPE_BIAS_ITEM_NAME, "Bias", false);
			indigo_init_switch_item(CCD_FRAME_TYPE_DARK_ITEM, CCD_FRAME_TYPE_DARK_ITEM_NAME, "Dark", false);
			indigo_init_switch_item(CCD_FRAME_TYPE_FLAT_ITEM, CCD_FRAME_TYPE_FLAT_ITEM_NAME, "Flat", false);
			// -------------------------------------------------------------------------------- CCD_IMAGE_FORMAT
			CCD_IMAGE_FORMAT_PROPERTY = indigo_init_switch_property(NULL, device->name, CCD_IMAGE_FORMAT_PROPERTY_NAME, CCD_IMAGE_GROUP, "Image format", INDIGO_OK_STATE, INDIGO_RW_PERM, INDIGO_ONE_OF_MANY_RULE, 4);
			if (CCD_IMAGE_FORMAT_PROPERTY == NULL)
				return INDIGO_FAILED;
			indigo_init_switch_item(CCD_IMAGE_FORMAT_FITS_ITEM, CCD_IMAGE_FORMAT_FITS_ITEM_NAME, "FITS format", true);
			indigo_init_switch_item(CCD_IMAGE_FORMAT_XISF_ITEM, CCD_IMAGE_FORMAT_XISF_ITEM_NAME, "XISF format", false);
			indigo_init_switch_item(CCD_IMAGE_FORMAT_RAW_ITEM, CCD_IMAGE_FORMAT_RAW_ITEM_NAME, "Raw data", false);
			indigo_init_switch_item(CCD_IMAGE_FORMAT_JPEG_ITEM, CCD_IMAGE_FORMAT_JPEG_ITEM_NAME, "JPEG format", false);
			// -------------------------------------------------------------------------------- CCD_IMAGE
			CCD_IMAGE_PROPERTY = indigo_init_blob_property(NULL, device->name, CCD_IMAGE_PROPERTY_NAME, CCD_IMAGE_GROUP, "Image data", INDIGO_OK_STATE, 1);
			if (CCD_IMAGE_PROPERTY == NULL)
				return INDIGO_FAILED;
			indigo_init_blob_item(CCD_IMAGE_ITEM, CCD_IMAGE_ITEM_NAME, "Image data");
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
			CCD_TEMPERATURE_PROPERTY = indigo_init_number_property(NULL, device->name, CCD_TEMPERATURE_PROPERTY_NAME, CCD_COOLER_GROUP, "Temperature", INDIGO_OK_STATE, INDIGO_RW_PERM, 1);
			if (CCD_TEMPERATURE_PROPERTY == NULL)
				return INDIGO_FAILED;
			CCD_TEMPERATURE_PROPERTY->hidden = true;
			indigo_init_number_item(CCD_TEMPERATURE_ITEM, CCD_TEMPERATURE_ITEM_NAME, "Temperature (C)", -50, 50, 1, 0);
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
			CCD_JPEG_SETTINGS_PROPERTY->hidden = true;
			indigo_init_number_item(CCD_JPEG_SETTINGS_QUALITY_ITEM, CCD_JPEG_SETTINGS_QUALITY_ITEM_NAME, "Conversion quality", 10, 100, 5, 90);
			indigo_init_number_item(CCD_JPEG_SETTINGS_BLACK_ITEM, CCD_JPEG_SETTINGS_BLACK_ITEM_NAME, "Black point", -1, 255, 0, -1);
			indigo_init_number_item(CCD_JPEG_SETTINGS_WHITE_ITEM, CCD_JPEG_SETTINGS_WHITE_ITEM_NAME, "White point", -1, 255, 0, -1);
			indigo_init_number_item(CCD_JPEG_SETTINGS_BLACK_TRESHOLD_ITEM, CCD_JPEG_SETTINGS_BLACK_TRESHOLD_ITEM_NAME, "Black point treshold", 0, 1, 0, 0.005);
			indigo_init_number_item(CCD_JPEG_SETTINGS_WHITE_TRESHOLD_ITEM, CCD_JPEG_SETTINGS_WHITE_TRESHOLD_ITEM_NAME, "White point treshold", 0, 1, 0, 0.002);
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
		if (indigo_property_match(CCD_IMAGE_PROPERTY, property))
			indigo_define_property(device, CCD_IMAGE_PROPERTY, NULL);
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
			indigo_define_property(device, CCD_UPLOAD_MODE_PROPERTY, NULL);
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
			indigo_define_property(device, CCD_COOLER_PROPERTY, NULL);
			indigo_define_property(device, CCD_COOLER_POWER_PROPERTY, NULL);
			indigo_define_property(device, CCD_TEMPERATURE_PROPERTY, NULL);
			indigo_define_property(device, CCD_FITS_HEADERS_PROPERTY, NULL);
			indigo_define_property(device, CCD_JPEG_SETTINGS_PROPERTY, NULL);
		} else {
			indigo_delete_property(device, CCD_INFO_PROPERTY, NULL);
			indigo_delete_property(device, CCD_UPLOAD_MODE_PROPERTY, NULL);
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
			indigo_delete_property(device, CCD_COOLER_PROPERTY, NULL);
			indigo_delete_property(device, CCD_COOLER_POWER_PROPERTY, NULL);
			indigo_delete_property(device, CCD_TEMPERATURE_PROPERTY, NULL);
			indigo_delete_property(device, CCD_FITS_HEADERS_PROPERTY, NULL);
			indigo_delete_property(device, CCD_JPEG_SETTINGS_PROPERTY, NULL);
		}
	} else if (indigo_property_match(CONFIG_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- CONFIG
		if (indigo_switch_match(CONFIG_SAVE_ITEM, property)) {
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
		}
	} else if (indigo_property_match(CCD_EXPOSURE_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- CCD_EXPOSURE
		if (CCD_EXPOSURE_PROPERTY->state == INDIGO_BUSY_STATE) {
			if (CCD_UPLOAD_MODE_LOCAL_ITEM->sw.value || CCD_UPLOAD_MODE_BOTH_ITEM->sw.value || CCD_UPLOAD_MODE_PREVIEW_LOCAL_ITEM->sw.value) {
				if (CCD_IMAGE_FILE_PROPERTY->state != INDIGO_BUSY_STATE) {
					CCD_IMAGE_FILE_PROPERTY->state = INDIGO_BUSY_STATE;
					indigo_update_property(device, CCD_IMAGE_FILE_PROPERTY, NULL);
				}
			}
			if (CCD_UPLOAD_MODE_CLIENT_ITEM->sw.value || CCD_UPLOAD_MODE_BOTH_ITEM->sw.value || CCD_UPLOAD_MODE_PREVIEW_ITEM->sw.value || CCD_UPLOAD_MODE_PREVIEW_LOCAL_ITEM->sw.value) {
				if (CCD_IMAGE_PROPERTY->state != INDIGO_BUSY_STATE) {
					CCD_IMAGE_PROPERTY->state = INDIGO_BUSY_STATE;
					indigo_update_property(device, CCD_IMAGE_PROPERTY, NULL);
				}
			}
			if (CCD_EXPOSURE_ITEM->number.value >= 1) {
				CCD_CONTEXT->countdown_timer = indigo_set_timer(device, 1.0, countdown_timer_callback);
			}
		}
		return INDIGO_OK;
	} else if (indigo_property_match(CCD_ABORT_EXPOSURE_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- CCD_ABORT_EXPOSURE
		if (CCD_IMAGE_PROPERTY->state == INDIGO_BUSY_STATE) {
			CCD_IMAGE_PROPERTY->state = INDIGO_ALERT_STATE;
			indigo_update_property(device, CCD_IMAGE_PROPERTY, NULL);
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
	} else if (indigo_property_match(CCD_FRAME_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- CCD_FRAME
		indigo_property_copy_values(CCD_FRAME_PROPERTY, property, false);
		CCD_FRAME_WIDTH_ITEM->number.value = ((int)CCD_FRAME_WIDTH_ITEM->number.value / (int)CCD_BIN_HORIZONTAL_ITEM->number.value) * (int)CCD_BIN_HORIZONTAL_ITEM->number.value;
		CCD_FRAME_HEIGHT_ITEM->number.value = ((int)CCD_FRAME_HEIGHT_ITEM->number.value / (int)CCD_BIN_VERTICAL_ITEM->number.value) * (int)CCD_BIN_VERTICAL_ITEM->number.value;
		if (IS_CONNECTED) {
			CCD_FRAME_PROPERTY->state = INDIGO_OK_STATE;
			if (CCD_FRAME_LEFT_ITEM->number.value + CCD_FRAME_WIDTH_ITEM->number.value > CCD_INFO_WIDTH_ITEM->number.value) {
				CCD_FRAME_WIDTH_ITEM->number.value = CCD_INFO_WIDTH_ITEM->number.value - CCD_FRAME_LEFT_ITEM->number.value;
				CCD_FRAME_PROPERTY->state = INDIGO_ALERT_STATE;
			}
			if (CCD_FRAME_TOP_ITEM->number.value + CCD_FRAME_HEIGHT_ITEM->number.value > CCD_INFO_HEIGHT_ITEM->number.value) {
				CCD_FRAME_HEIGHT_ITEM->number.value = CCD_INFO_HEIGHT_ITEM->number.value - CCD_FRAME_TOP_ITEM->number.value;
				CCD_FRAME_PROPERTY->state = INDIGO_ALERT_STATE;
			}
			indigo_update_property(device, CCD_FRAME_PROPERTY, NULL);
		}
		return INDIGO_OK;
	} else if (indigo_property_match(CCD_BIN_PROPERTY, property)) {
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
	} else if (indigo_property_match(CCD_MODE_PROPERTY, property)) {
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
	} else if (indigo_property_match(CCD_OFFSET_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- CCD_OFFSET
		indigo_property_copy_values(CCD_OFFSET_PROPERTY, property, false);
		if (IS_CONNECTED) {
			CCD_OFFSET_PROPERTY->state = INDIGO_OK_STATE;
			indigo_update_property(device, CCD_OFFSET_PROPERTY, NULL);
		}
		return INDIGO_OK;
	} else if (indigo_property_match(CCD_READ_MODE_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- CCD_READ_MODE
		indigo_property_copy_values(CCD_READ_MODE_PROPERTY, property, false);
		if (IS_CONNECTED) {
			CCD_READ_MODE_PROPERTY->state = INDIGO_OK_STATE;
			indigo_update_property(device, CCD_READ_MODE_PROPERTY, NULL);
		}
		return INDIGO_OK;
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
		if (CCD_IMAGE_FORMAT_JPEG_ITEM->sw.value) {
			if (CCD_JPEG_SETTINGS_PROPERTY->hidden) {
				CCD_JPEG_SETTINGS_PROPERTY->hidden = false;
				if (IS_CONNECTED)
					indigo_define_property(device, CCD_JPEG_SETTINGS_PROPERTY, NULL);
			}
		} else {
			if (!CCD_JPEG_SETTINGS_PROPERTY->hidden) {
				if (IS_CONNECTED)
					indigo_delete_property(device, CCD_JPEG_SETTINGS_PROPERTY, NULL);
				CCD_JPEG_SETTINGS_PROPERTY->hidden = true;
			}
		}
		CCD_IMAGE_FORMAT_PROPERTY->state = INDIGO_OK_STATE;
		if (IS_CONNECTED)
			indigo_update_property(device, CCD_IMAGE_FORMAT_PROPERTY, NULL);
		return INDIGO_OK;
	} else if (indigo_property_match(CCD_UPLOAD_MODE_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- CCD_IMAGE_UPLOAD_MODE
		indigo_property_copy_values(CCD_UPLOAD_MODE_PROPERTY, property, false);
		if (CCD_UPLOAD_MODE_PREVIEW_ITEM->sw.value || CCD_UPLOAD_MODE_PREVIEW_LOCAL_ITEM->sw.value) {
			if (CCD_JPEG_SETTINGS_PROPERTY->hidden) {
				CCD_JPEG_SETTINGS_PROPERTY->hidden = false;
				if (IS_CONNECTED)
					indigo_define_property(device, CCD_JPEG_SETTINGS_PROPERTY, NULL);
			}
		} else {
			if (!CCD_JPEG_SETTINGS_PROPERTY->hidden) {
				if (IS_CONNECTED)
					indigo_delete_property(device, CCD_JPEG_SETTINGS_PROPERTY, NULL);
				CCD_JPEG_SETTINGS_PROPERTY->hidden = true;
			}
		}
		CCD_UPLOAD_MODE_PROPERTY->state = INDIGO_OK_STATE;
		if (IS_CONNECTED)
			indigo_update_property(device, CCD_UPLOAD_MODE_PROPERTY, NULL);
		return INDIGO_OK;
	} else if (indigo_property_match(CCD_LOCAL_MODE_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- CCD_IMAGE_LOCAL_MODE
		indigo_property_copy_values(CCD_LOCAL_MODE_PROPERTY, property, false);
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
				strncpy(item->text.value, line, INDIGO_VALUE_SIZE);
			} else if (!strncasecmp(item->text.value, "COMMENT ", 7)) {
				char *tmp = item->text.value + 7;
				while (tmp - item->text.value < 80 && *tmp == ' ')
					tmp++;
				snprintf(line, 80, "COMMENT  %s", tmp);
				strncpy(item->text.value, line, INDIGO_VALUE_SIZE);
			} else if (!strncasecmp(item->text.value, "HISTORY ", 7)) {
				char *tmp = item->text.value + 7;
				while (tmp - item->text.value < 80 && *tmp == ' ')
					tmp++;
				snprintf(line, 80, "HISTORY  %s", tmp);
				strncpy(item->text.value, line, INDIGO_VALUE_SIZE);
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
		// --------------------------------------------------------------------------------
	}
	return indigo_device_change_property(device, client, property);
}

indigo_result indigo_ccd_detach(indigo_device *device) {
	assert(device != NULL);
	indigo_release_property(CCD_INFO_PROPERTY);
	indigo_release_property(CCD_UPLOAD_MODE_PROPERTY);
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
	indigo_release_property(CCD_TEMPERATURE_PROPERTY);
	indigo_release_property(CCD_COOLER_PROPERTY);
	indigo_release_property(CCD_COOLER_POWER_PROPERTY);
	indigo_release_property(CCD_FITS_HEADERS_PROPERTY);
	indigo_release_property(CCD_JPEG_SETTINGS_PROPERTY);
	return indigo_device_detach(device);
}

static void set_black_white(indigo_device *device, long *histo, long count) {
	long black = CCD_JPEG_SETTINGS_BLACK_TRESHOLD_ITEM->number.value * count;
	if (CCD_JPEG_SETTINGS_BLACK_ITEM->number.target == -1) {
		long total = 0;
		for (int i = 0; i < 256; i++) {
			total += histo[i];
			if (total > black) {
				CCD_JPEG_SETTINGS_BLACK_ITEM->number.value = i;
				break;
			}
		}
	} else {
		CCD_JPEG_SETTINGS_BLACK_ITEM->number.value = CCD_JPEG_SETTINGS_BLACK_ITEM->number.target;
	}
	long white = CCD_JPEG_SETTINGS_WHITE_TRESHOLD_ITEM->number.value * count;
	if (CCD_JPEG_SETTINGS_WHITE_ITEM->number.target == -1) {
		long total = 0;
		for (int i = 255; i >= 0; i--) {
			total += histo[i];
			if (total > white) {
				CCD_JPEG_SETTINGS_WHITE_ITEM->number.value = i;
				break;
			}
		}
	} else {
		CCD_JPEG_SETTINGS_WHITE_ITEM->number.value = CCD_JPEG_SETTINGS_WHITE_ITEM->number.target;
	}
	if (CCD_JPEG_SETTINGS_BLACK_ITEM->number.value == CCD_JPEG_SETTINGS_WHITE_ITEM->number.value) {
		if (CCD_JPEG_SETTINGS_BLACK_ITEM->number.value < 254)
			CCD_JPEG_SETTINGS_WHITE_ITEM->number.value += 2;
		else
			CCD_JPEG_SETTINGS_BLACK_ITEM->number.value -= 2;
	}
	if (CCD_JPEG_SETTINGS_BLACK_ITEM->number.value != CCD_JPEG_SETTINGS_BLACK_ITEM->number.target || CCD_JPEG_SETTINGS_WHITE_ITEM->number.value != CCD_JPEG_SETTINGS_WHITE_ITEM->number.target) {
		CCD_JPEG_SETTINGS_PROPERTY->state = INDIGO_OK_STATE;
		indigo_update_property(device, CCD_JPEG_SETTINGS_PROPERTY, NULL);
	}
}

static unsigned long raw_to_jpeg(indigo_device *device, void *data, int frame_width, int frame_height, int bpp, bool little_endian, bool byte_order_rgb, int max_size) {
	INDIGO_DEBUG(clock_t start = clock());
	int size = frame_width * frame_height;
	unsigned char *mem = NULL;
	unsigned long mem_size = 0;
	struct jpeg_compress_struct cinfo;
	struct jpeg_error_mgr jerr;
	cinfo.err = jpeg_std_error(&jerr);
	jpeg_create_compress(&cinfo);
	jpeg_mem_dest(&cinfo, &mem, &mem_size);
	cinfo.image_width = frame_width;
	cinfo.image_height = frame_height;
	long histo[256] = { 0 };
	if (bpp == 8 || bpp == 24) {
		unsigned char *b8 = data + FITS_HEADER_SIZE;
		int count = size * (bpp == 8 ? 1 : 3);
		for (int i = 0; i < count; i++) {
			histo[*b8++]++;
		}
		set_black_white(device, histo, count);
		double scale = (CCD_JPEG_SETTINGS_WHITE_ITEM->number.value - CCD_JPEG_SETTINGS_BLACK_ITEM->number.value) / 255;
		int offset = CCD_JPEG_SETTINGS_BLACK_ITEM->number.value;
		b8 = data + FITS_HEADER_SIZE;
		for (int i = 0; i < count; i++) {
			int value = (*b8 - offset) / scale;
			if (value < 0)
				value = 0;
			else if (value > 255)
				value = 255;
			*b8++ = value;
		}
	}
	if (bpp == 16 || bpp == 48) {
		unsigned short *b16 = data + FITS_HEADER_SIZE;
		int count = size * (bpp == 16 ? 1 : 3);
		if (little_endian) {
			for (int i = 0; i < count; i++) {
				histo[*b16++ >> 8]++;
			}
		} else {
			for (int i = 0; i < count; i++) {
				int value = *b16;
				value = (value & 0xff) << 8 | (value & 0xff00) >> 8;
				*b16++ = value;
				histo[value >> 8]++;
			}
		}
		set_black_white(device, histo, count);
		double scale = (CCD_JPEG_SETTINGS_WHITE_ITEM->number.value - CCD_JPEG_SETTINGS_BLACK_ITEM->number.value);
		int offset = CCD_JPEG_SETTINGS_BLACK_ITEM->number.value;
		unsigned char *b8 = data + FITS_HEADER_SIZE;
		b16 = data + FITS_HEADER_SIZE;
		for (int i = 0; i < count; i++) {
			int value = (*b16++ - offset) / scale;
			if (value < 0)
				value = 0;
			else if (value > 255)
				value = 255;
			*b8++ = value;
		}
	}
	if (bpp == 8 || bpp == 16) {
		cinfo.input_components = 1;
		cinfo.in_color_space = JCS_GRAYSCALE;
	}
	if (bpp == 24 || bpp == 48) {
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
		cinfo.input_components = 3;
		cinfo.in_color_space = JCS_RGB;
	}
	jpeg_set_defaults(&cinfo);
	jpeg_set_quality(&cinfo, CCD_JPEG_SETTINGS_QUALITY_ITEM->number.target, true);
	JSAMPROW row_pointer[1];
	jpeg_start_compress( &cinfo, TRUE);
	unsigned char *tmp = data + FITS_HEADER_SIZE;
	while( cinfo.next_scanline < cinfo.image_height ) {
		row_pointer[0] = &tmp[cinfo.next_scanline * cinfo.image_width *  cinfo.input_components];
		jpeg_write_scanlines(&cinfo, row_pointer, 1);
	}
	jpeg_finish_compress(&cinfo);
	jpeg_destroy_compress(&cinfo);
	if (mem_size < max_size) {
		memcpy(data, mem, mem_size);
		free(mem);
		INDIGO_DEBUG(indigo_debug("RAW to preview conversion in %gs", (clock() - start) / (double)CLOCKS_PER_SEC));
		return mem_size;
	}
	free(mem);
	indigo_error("RAW to JPEG conversion failed");
	return 0;
}

void indigo_process_image(indigo_device *device, void *data, int frame_width, int frame_height, int bpp, bool little_endian, bool byte_order_rgb, indigo_fits_keyword *keywords) {
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
	if (!CCD_UPLOAD_MODE_PREVIEW_ITEM->sw.value) {
		if (CCD_IMAGE_FORMAT_FITS_ITEM->sw.value) {
			INDIGO_DEBUG(clock_t start = clock());
			time_t timer;
			struct tm* tm_info;
			char date_time_end[20];
			time(&timer);
			tm_info = gmtime(&timer);
			strftime(date_time_end, 20, "%Y-%m-%dT%H:%M:%S", tm_info);
			char *header = data;
			memset(header, ' ', FITS_HEADER_SIZE);
			int t = sprintf(header, "SIMPLE  =                    T / file conforms to FITS standard");
			header[t] = ' ';
			t = sprintf(header += 80, "BITPIX  = %20d / number of bits per data pixel", byte_per_pixel * 8);
			header[t] = ' ';
			t = sprintf(header += 80, "NAXIS   =                    %d / number of data axes", naxis);
			header[t] = ' ';
			t = sprintf(header += 80, "NAXIS1  = %20d / length of data axis 1 [pixels]", frame_width);
			header[t] = ' ';
			t = sprintf(header += 80, "NAXIS2  = %20d / length of data axis 2 [pixels]", frame_height);
			header[t] = ' ';
			if (naxis == 3) {
				t = sprintf(header += 80, "NAXIS3  = %20d / length of data axis 3 [RGB]", 3);
				header[t] = ' ';
			}
			t = sprintf(header += 80, "EXTEND  =                    T / FITS dataset may contain extensions");
			header[t] = ' ';
			t = sprintf(header += 80, "COMMENT   FITS (Flexible Image Transport System) format is defined in 'Astronomy");
			header[t] = ' ';
			t = sprintf(header += 80, "COMMENT   and Astrophysics', volume 376, page 359; bibcode: 2001A&A...376..359H");
			header[t] = ' ';
			t = sprintf(header += 80, "COMMENT   Created by INDIGO %d.%d framework, see www.indigo-astronomy.org", (INDIGO_VERSION_CURRENT >> 8) & 0xFF, INDIGO_VERSION_CURRENT & 0xFF);
			header[t] = ' ';
			if (byte_per_pixel == 2) {
				t = sprintf(header += 80, "BZERO   =                32768 / offset data range to that of unsigned short");
				header[t] = ' ';
				t = sprintf(header += 80, "BSCALE  =                    1 / default scaling factor");
				header[t] = ' ';
			} else {
			//	t = sprintf(header += 80, "BZERO   =                    0 / offset data range to that of unsigned short");
			//	header[t] = ' ';
			//	t = sprintf(header += 80, "BSCALE  =                  256 / default scaling factor");
			//	header[t] = ' ';
			}
			t = sprintf(header += 80, "XBINNING= %20d / horizontal binning [pixels]", horizontal_bin);
			header[t] = ' ';
			t = sprintf(header += 80, "YBINNING= %20d / vertical binning [pixels]", vertical_bin);
			header[t] = ' ';
			if (CCD_INFO_PIXEL_WIDTH_ITEM->number.value > 0 && CCD_INFO_PIXEL_HEIGHT_ITEM->number.value) {
				t = sprintf(header += 80, "XPIXSZ  = %20.2f / pixel width [microns]", CCD_INFO_PIXEL_WIDTH_ITEM->number.value * horizontal_bin);
				header[t] = ' ';
				t = sprintf(header += 80, "YPIXSZ  = %20.2f / pixel height [microns]", CCD_INFO_PIXEL_HEIGHT_ITEM->number.value * vertical_bin);
				header[t] = ' ';
			}
			t = sprintf(header += 80, "EXPTIME = %20.2f / exposure time [s]", CCD_EXPOSURE_ITEM->number.target);
			header[t] = ' ';
			if (!CCD_TEMPERATURE_PROPERTY->hidden) {
				t = sprintf(header += 80, "CCD-TEMP= %20.2f / CCD temperature [C]", CCD_TEMPERATURE_ITEM->number.value);
				header[t] = ' ';
			}
			if (CCD_FRAME_TYPE_LIGHT_ITEM->sw.value)
				t = sprintf(header += 80, "IMAGETYP= 'Light'               / frame type");
			else if (CCD_FRAME_TYPE_FLAT_ITEM->sw.value)
				t = sprintf(header += 80, "IMAGETYP= 'Flat'                / frame type");
			else if (CCD_FRAME_TYPE_BIAS_ITEM->sw.value)
				t = sprintf(header += 80, "IMAGETYP= 'Bias'                / frame type");
			else if (CCD_FRAME_TYPE_DARK_ITEM->sw.value)
				t = sprintf(header += 80, "IMAGETYP= 'Dark'                / frame type");
			header[t] = ' ';
			if (!CCD_GAIN_PROPERTY->hidden) {
				t = sprintf(header += 80, "GAIN    = %20.2f / Gain", CCD_GAIN_ITEM->number.value);
				header[t] = ' ';
			}
			if (!CCD_OFFSET_PROPERTY->hidden) {
				t = sprintf(header += 80, "OFFSET  = %20.2f / Offset", CCD_OFFSET_ITEM->number.value);
				header[t] = ' ';
			}
			if (!CCD_GAMMA_PROPERTY->hidden) {
				t = sprintf(header += 80, "GAMMA   = %20.2f / Gamma", CCD_GAMMA_ITEM->number.value);
				header[t] = ' ';
			}
			t = sprintf(header += 80, "DATE-OBS= '%s' / UTC date that FITS file was created", date_time_end);
			header[t] = ' ';
			t = sprintf(header += 80, "INSTRUME= '%s'%*c / instrument name", device->name, (int)(19 - strlen(device->name)), ' ');
			header[t] = ' ';
			if (keywords) {
				while (keywords->type && (header - (char *)data) < (FITS_HEADER_SIZE - 80)) {
					switch (keywords->type) {
						case INDIGO_FITS_NUMBER:
							t = sprintf(header += 80, "%7s= %20f / %s", keywords->name, keywords->number, keywords->comment);
							break;
						case INDIGO_FITS_STRING:
							t = sprintf(header += 80, "%7s= '%s'%*c / %s", keywords->name, keywords->string, (int)(18 - strlen(keywords->string)), ' ', keywords->comment);
							break;
						case INDIGO_FITS_LOGICAL:
							t = sprintf(header += 80, "%7s=                    %c / %s", keywords->name, keywords->logical ? 'T' : 'F', keywords->comment);
							break;
					}
					header[t] = ' ';
					keywords++;
				}
			}
			for (int i = 0; i < CCD_FITS_HEADERS_PROPERTY->count; i++) {
				indigo_item *item = CCD_FITS_HEADERS_PROPERTY->items + i;
				if (*item->text.value && (header - (char *)data) < (FITS_HEADER_SIZE - 80)) {
					t = sprintf(header += 80, "%s", item->text.value);
					header[t] = ' ';
				}
			}
			t = sprintf(header += 80, "END");
			header[t] = ' ';
			if (byte_per_pixel == 2 && naxis == 2) {
				short *raw = (short *)(data + FITS_HEADER_SIZE);
				if (little_endian) {
					for (int i = 0; i < size; i++) {
						int value = *raw - 32768;
						*raw++ = (value & 0xff) << 8 | (value & 0xff00) >> 8;
					}
				} else {
					for (int i = 0; i < size; i++) {
						int value = *raw;
						value = ((value & 0xff) << 8 | (value & 0xff00) >> 8 ) - 32768;
						*raw++ = (value & 0xff) << 8 | (value & 0xff00) >> 8;
					}
				}
			} else if (byte_per_pixel == 1 && naxis == 3) {
				unsigned char *raw = malloc(3 * size);
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
				unsigned short *raw = malloc(6 * size);
				unsigned short *red = raw;
				unsigned short *green = raw + size;
				unsigned short *blue = raw + 2 * size;
				unsigned short *tmp = (unsigned short *)(data + FITS_HEADER_SIZE);
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
			char date_time_end[21], date_time_start[21];
			time(&timer);
			tm_info = gmtime(&timer);
			strftime(date_time_end, 21, "%Y-%m-%dT%H:%M:%SZ", tm_info);
			timer -= CCD_EXPOSURE_ITEM->number.target;
			tm_info = gmtime(&timer);
			strftime(date_time_start, 21, "%Y-%m-%dT%H:%M:%SZ", tm_info);
			char *header = data;
			strcpy(header, "XISF0100");
			header += 16;
			memset(header, 0, FITS_HEADER_SIZE);
			sprintf(header, "<?xml version='1.0' encoding='UTF-8'?><xisf xmlns='http://www.pixinsight.com/xisf' xmlns:xsi='http://www.w3.org/2001/XMLSchema-instance' version='1.0' xsi:schemaLocation='http://www.pixinsight.com/xisf http://pixinsight.com/xisf/xisf-1.0.xsd'>");
			header += strlen(header);
			char *frame_type = "Light";
			if (CCD_FRAME_TYPE_FLAT_ITEM->sw.value)
				frame_type ="Flat";
			else if (CCD_FRAME_TYPE_BIAS_ITEM->sw.value)
				frame_type ="Bias";
			else if (CCD_FRAME_TYPE_DARK_ITEM->sw.value)
				frame_type ="Dark";
			if (naxis == 2 && byte_per_pixel == 1) {
				sprintf(header, "<Image geometry='%d:%d:1' imageType='%s' sampleFormat='UInt8' colorSpace='Gray' location='attachment:%d:%d'>", frame_width, frame_height, frame_type, FITS_HEADER_SIZE, blobsize);
			} else if (naxis == 2 && byte_per_pixel == 2) {
				sprintf(header, "<Image geometry='%d:%d:1' imageType='%s' sampleFormat='UInt16' colorSpace='Gray' location='attachment:%d:%d'>", frame_width, frame_height, frame_type, FITS_HEADER_SIZE, blobsize);
			} else if (naxis == 3 && byte_per_pixel == 1) {
				sprintf(header, "<Image geometry='%d:%d:3' imageType='%s' pixelStorage='Normal' sampleFormat='UInt8' colorSpace='RGB' location='attachment:%d:%d'>", frame_width, frame_height, frame_type, FITS_HEADER_SIZE, blobsize);
			} else if (naxis == 3 && byte_per_pixel == 2) {
				sprintf(header, "<Image geometry='%d:%d:3' imageType='%s' pixelStorage='Normal' sampleFormat='UInt16' colorSpace='RGB' location='attachment:%d:%d'>", frame_width, frame_height, frame_type, FITS_HEADER_SIZE, blobsize);
			}
			header += strlen(header);
			sprintf(header, "<Property id='Observation:Time:Start' type='TimePoint' value='%s'/><Property id='Observation:Time:End' type='TimePoint' value='%s'/>", date_time_start ,date_time_end);
			header += strlen(header);
			sprintf(header, "<Property id='Instrument:Camera:Name' type='String'>%s</Property>", device->name);
			header += strlen(header);
			sprintf(header, "<Property id='Instrument:Camera:XBinning' type='Int32' value='%d'/><Property id='Instrument:Camera:YBinning' type='Int32' value='%d'/>", horizontal_bin, vertical_bin);
			header += strlen(header);
			sprintf(header, "<Property id='Instrument:ExposureTime' type='Float32' value='%.5f'/>", CCD_EXPOSURE_ITEM->number.target);
			header += strlen(header);
			sprintf(header, "<Property id='Instrument:Sensor:XPixelSize' type='Float32' value='%.2f'/><Property id='Instrument:Sensor:YPixelSize' type='Float32' value='%.2f'/>", CCD_INFO_PIXEL_WIDTH_ITEM->number.value * horizontal_bin, CCD_INFO_PIXEL_HEIGHT_ITEM->number.value * vertical_bin);
			header += strlen(header);
			if (!CCD_TEMPERATURE_PROPERTY->hidden) {
				sprintf(header, "<Property id='Instrument:Sensor:Temperature' type='Float32' value='%.2f'/><Property id='Instrument:Sensor:TargetTemperature' type='Float32' value='%.2f'/>", CCD_TEMPERATURE_ITEM->number.value, CCD_TEMPERATURE_ITEM->number.target);
			}
			header += strlen(header);
			if (!CCD_GAIN_PROPERTY->hidden) {
				sprintf(header, "<Property id='Instrument:Camera:Gain' type='Float32' value='%g'/>", CCD_GAIN_ITEM->number.value);
				header += strlen(header);
			}
			for (int i = 0; i < CCD_FITS_HEADERS_PROPERTY->count; i++) {
				indigo_item *item = CCD_FITS_HEADERS_PROPERTY->items + i;
				if (!strncmp(item->text.value, "FILTER=", 7)) {
					sprintf(header, "<Property id='Instrument:Filter:Name' type='String' value='%s'/>", item->text.value + 7);
					header += strlen(header);
				} else if (!strncmp(item->text.value, "FOCUS=", 6)) {
					sprintf(header, "<Property id='Instrument:Focuser:Position' type='String' value='%s'/>", item->text.value + 6);
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
			sprintf(header, "</Image><Metadata><Property id='XISF:CreationTime' type='String'>%s</Property><Property id='XISF:CreatorApplication' type='String'>INDIGO 2.0-%d</Property>", date_time_end, INDIGO_BUILD);
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
					short *b16 = (short *)(data + FITS_HEADER_SIZE);
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
		} else if (CCD_IMAGE_FORMAT_RAW_ITEM->sw.value) {
			indigo_raw_header *header = (indigo_raw_header *)(data + FITS_HEADER_SIZE - sizeof(indigo_raw_header));
			if (naxis == 2 && byte_per_pixel == 1)
				header->signature = INDIGO_RAW_MONO8;
			else if (naxis == 2 && byte_per_pixel == 2) {
				header->signature = INDIGO_RAW_MONO16;
				if (!little_endian) {
					short *b16 = (short *)(data + FITS_HEADER_SIZE);
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
		} else if (CCD_IMAGE_FORMAT_JPEG_ITEM->sw.value) {
			blobsize = raw_to_jpeg(device, data, frame_width, frame_height, bpp, little_endian, byte_order_rgb, blobsize);
		}
	}
	if (CCD_UPLOAD_MODE_LOCAL_ITEM->sw.value || CCD_UPLOAD_MODE_BOTH_ITEM->sw.value || CCD_UPLOAD_MODE_PREVIEW_LOCAL_ITEM->sw.value) {
		char *dir = CCD_LOCAL_MODE_DIR_ITEM->text.value;
		char *prefix = CCD_LOCAL_MODE_PREFIX_ITEM->text.value;
		char *suffix;
		if (CCD_IMAGE_FORMAT_FITS_ITEM->sw.value) {
			suffix = ".fits";
		} else if (CCD_IMAGE_FORMAT_XISF_ITEM->sw.value) {
			suffix = ".xisf";
		} else if (CCD_IMAGE_FORMAT_RAW_ITEM->sw.value) {
			suffix = ".raw";
		} else if (CCD_IMAGE_FORMAT_JPEG_ITEM->sw.value) {
			suffix = ".jpeg";
		}
		int handle = 0;
		char *message = NULL;
		if (strlen(dir) + strlen(prefix) + strlen(suffix) < INDIGO_VALUE_SIZE) {
			char file_name[INDIGO_VALUE_SIZE];
			char *xxx = strstr(prefix, "XXX");
			if (xxx == NULL) {
				strncpy(file_name, dir, INDIGO_VALUE_SIZE);
				strcat(file_name, prefix);
				strcat(file_name, suffix);
			} else {
				char format[INDIGO_VALUE_SIZE];
				strcpy(format, dir);
				strncat(format, prefix, xxx - prefix);
				strcat(format, "%03d");
				strcat(format, xxx+3);
				strcat(format, suffix);
				struct stat sb;
				int i = 1;
				while (true) {
					snprintf(file_name, sizeof(file_name), format, i);
					if (stat(file_name, &sb) == 0 && S_ISREG(sb.st_mode))
						i++;
					else
						break;
				}
			}
			strncpy(CCD_IMAGE_FILE_ITEM->text.value, file_name, INDIGO_VALUE_SIZE);
			CCD_IMAGE_FILE_PROPERTY->state = INDIGO_OK_STATE;
			handle = open(file_name, O_WRONLY | O_CREAT | O_TRUNC, 0644);
			if (handle) {
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
				} else if (CCD_IMAGE_FORMAT_RAW_ITEM->sw.value) {
					if (!indigo_write(handle, data + FITS_HEADER_SIZE - sizeof(indigo_raw_header), blobsize + sizeof(indigo_raw_header))) {
						CCD_IMAGE_FILE_PROPERTY->state = INDIGO_ALERT_STATE;
						message = strerror(errno);
					}
				} else if (CCD_IMAGE_FORMAT_JPEG_ITEM->sw.value) {
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
		} else {
			CCD_IMAGE_FILE_PROPERTY->state = INDIGO_ALERT_STATE;
			message = "dir + prefix + suffix is too long";
		}
		indigo_update_property(device, CCD_IMAGE_FILE_PROPERTY, message);
		INDIGO_DEBUG(indigo_debug("Local save in %gs", (clock() - start) / (double)CLOCKS_PER_SEC));
	}
	if (CCD_UPLOAD_MODE_CLIENT_ITEM->sw.value || CCD_UPLOAD_MODE_BOTH_ITEM->sw.value) {
		*CCD_IMAGE_ITEM->blob.url = 0;
		if (CCD_IMAGE_FORMAT_FITS_ITEM->sw.value) {
			CCD_IMAGE_ITEM->blob.value = data;
			CCD_IMAGE_ITEM->blob.size = FITS_HEADER_SIZE + blobsize;
			strncpy(CCD_IMAGE_ITEM->blob.format, ".fits", INDIGO_NAME_SIZE);
		} else if (CCD_IMAGE_FORMAT_XISF_ITEM->sw.value) {
			CCD_IMAGE_ITEM->blob.value = data;
			CCD_IMAGE_ITEM->blob.size = FITS_HEADER_SIZE + blobsize;
			strncpy(CCD_IMAGE_ITEM->blob.format, ".xisf", INDIGO_NAME_SIZE);
		} else if (CCD_IMAGE_FORMAT_RAW_ITEM->sw.value) {
			CCD_IMAGE_ITEM->blob.value = data + FITS_HEADER_SIZE - sizeof(indigo_raw_header);
			CCD_IMAGE_ITEM->blob.size = blobsize + sizeof(indigo_raw_header);
			strncpy(CCD_IMAGE_ITEM->blob.format, ".raw", INDIGO_NAME_SIZE);
		} else if (CCD_IMAGE_FORMAT_JPEG_ITEM->sw.value) {
			CCD_IMAGE_ITEM->blob.value = data;
			CCD_IMAGE_ITEM->blob.size = blobsize;
			strncpy(CCD_IMAGE_ITEM->blob.format, ".jpeg", INDIGO_NAME_SIZE);
		}
		CCD_IMAGE_PROPERTY->state = INDIGO_OK_STATE;
		indigo_update_property(device, CCD_IMAGE_PROPERTY, NULL);
		INDIGO_DEBUG(indigo_debug("Client upload in %gs", (clock() - start) / (double)CLOCKS_PER_SEC));
	}
	if (CCD_UPLOAD_MODE_PREVIEW_ITEM->sw.value || CCD_UPLOAD_MODE_PREVIEW_LOCAL_ITEM->sw.value) {
		if (!(CCD_IMAGE_FORMAT_JPEG_ITEM->sw.value && CCD_UPLOAD_MODE_PREVIEW_LOCAL_ITEM->sw.value)) {
			CCD_IMAGE_ITEM->blob.value = data;
			CCD_IMAGE_ITEM->blob.size = raw_to_jpeg(device, data, frame_width, frame_height, bpp, little_endian, byte_order_rgb, blobsize);
		}
		strncpy(CCD_IMAGE_ITEM->blob.format, ".jpeg", INDIGO_NAME_SIZE);
		CCD_IMAGE_PROPERTY->state = INDIGO_OK_STATE;
		indigo_update_property(device, CCD_IMAGE_PROPERTY, NULL);
		INDIGO_DEBUG(indigo_debug("Client preview upload in %gs", (clock() - start) / (double)CLOCKS_PER_SEC));
	}
}

void indigo_process_dslr_image(indigo_device *device, void *data, int blobsize, const char *suffix) {
	assert(device != NULL);
	assert(data != NULL);
	INDIGO_DEBUG(clock_t start = clock());

	if (CCD_UPLOAD_MODE_LOCAL_ITEM->sw.value || CCD_UPLOAD_MODE_BOTH_ITEM->sw.value || CCD_UPLOAD_MODE_PREVIEW_LOCAL_ITEM->sw.value) {
		char *dir = CCD_LOCAL_MODE_DIR_ITEM->text.value;
		char *prefix = CCD_LOCAL_MODE_PREFIX_ITEM->text.value;
		int handle = 0;
		char *message = NULL;
		if (strlen(dir) + strlen(prefix) + strlen(suffix) < INDIGO_VALUE_SIZE) {
			char file_name[INDIGO_VALUE_SIZE];
			char *xxx = strstr(prefix, "XXX");
			if (xxx == NULL) {
				strncpy(file_name, dir, INDIGO_VALUE_SIZE - strlen(prefix) - strlen(suffix));
				strcat(file_name, prefix);
				strcat(file_name, suffix);
			} else {
				char format[INDIGO_VALUE_SIZE];
				strncpy(format, dir, INDIGO_VALUE_SIZE);
				strncat(format, prefix, xxx - prefix);
				strcat(format, "%03d");
				strcat(format, xxx+3);
				strcat(format, suffix);
				struct stat sb;
				int i = 1;
				while (true) {
					snprintf(file_name, sizeof(file_name), format, i);
					if (stat(file_name, &sb) == 0 && S_ISREG(sb.st_mode))
						i++;
					else
						break;
				}
			}
			strncpy(CCD_IMAGE_FILE_ITEM->text.value, file_name, INDIGO_VALUE_SIZE);
			CCD_IMAGE_FILE_PROPERTY->state = INDIGO_OK_STATE;
			handle = open(file_name, O_WRONLY | O_CREAT | O_TRUNC, 0644);
			if (handle) {
				if (!indigo_write(handle, data, blobsize)) {
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
		CCD_IMAGE_ITEM->blob.value = data;
		CCD_IMAGE_ITEM->blob.size = blobsize;
		strncpy(CCD_IMAGE_ITEM->blob.format, suffix, INDIGO_NAME_SIZE);
		CCD_IMAGE_PROPERTY->state = INDIGO_OK_STATE;
		indigo_update_property(device, CCD_IMAGE_PROPERTY, NULL);
		INDIGO_DEBUG(indigo_debug("Client upload in %gs", (clock() - start) / (double)CLOCKS_PER_SEC));
	}
	if (CCD_UPLOAD_MODE_PREVIEW_ITEM->sw.value || CCD_UPLOAD_MODE_PREVIEW_LOCAL_ITEM->sw.value) {
		// TBD make preview
		CCD_IMAGE_PROPERTY->state = INDIGO_OK_STATE;
		indigo_update_property(device, CCD_IMAGE_PROPERTY, NULL);
		INDIGO_DEBUG(indigo_debug("Client preview upload in %gs", (clock() - start) / (double)CLOCKS_PER_SEC));
	}
}
