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

/** INDIGO CCD Driver base
 \file indigo_ccd_driver.c
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <assert.h>
#include <string.h>
#include <errno.h>
#include <time.h>
#include <math.h>
#include <fcntl.h>
#include <sys/stat.h>

#include "indigo_ccd_driver.h"

static void countdown_timer_callback(indigo_device *device) {
	if (CCD_EXPOSURE_PROPERTY->state == INDIGO_BUSY_STATE && CCD_EXPOSURE_ITEM->number.value >= 1) {
		CCD_EXPOSURE_ITEM->number.value -= 1;
		indigo_update_property(device, CCD_EXPOSURE_PROPERTY, NULL);
		indigo_set_timer(device, 1.0, countdown_timer_callback);
	}
}

indigo_result indigo_ccd_attach(indigo_device *device, indigo_version version) {
	assert(device != NULL);
	assert(device != NULL);
	if (CCD_CONTEXT == NULL) {
		device->device_context = malloc(sizeof(indigo_ccd_context));
		assert(device->device_context != NULL);
		memset(device->device_context, 0, sizeof(indigo_ccd_context));
	}
	if (CCD_CONTEXT != NULL) {
		if (indigo_device_attach(device, version, INDIGO_INTERFACE_CCD) == INDIGO_OK) {
			// -------------------------------------------------------------------------------- CCD_INFO
			CCD_INFO_PROPERTY = indigo_init_number_property(NULL, device->name, "CCD_INFO", CCD_MAIN_GROUP, "CCD info", INDIGO_IDLE_STATE, INDIGO_RO_PERM, 8);
			if (CCD_INFO_PROPERTY == NULL)
				return INDIGO_FAILED;
			indigo_init_number_item(CCD_INFO_WIDTH_ITEM, "WIDTH", "Horizontal resolution", 0, 0, 0, 0);
			indigo_init_number_item(CCD_INFO_HEIGHT_ITEM, "HEIGHT", "Vertical resolution", 0, 0, 0, 0);
			indigo_init_number_item(CCD_INFO_MAX_HORIZONAL_BIN_ITEM, "MAX_HORIZONAL_BIN", "Max vertical binning", 0, 0, 0, 1);
			indigo_init_number_item(CCD_INFO_MAX_VERTICAL_BIN_ITEM, "MAX_VERTICAL_BIN", "Max horizontal binning", 0, 0, 0, 1);
			indigo_init_number_item(CCD_INFO_PIXEL_SIZE_ITEM, "PIXEL_SIZE", "Pixel size", 0, 0, 0, 0);
			indigo_init_number_item(CCD_INFO_PIXEL_WIDTH_ITEM, "PIXEL_WIDTH", "Pixel width", 0, 0, 0, 0);
			indigo_init_number_item(CCD_INFO_PIXEL_HEIGHT_ITEM, "PIXEL_HEIGHT", "Pixel height", 0, 0, 0, 0);
			indigo_init_number_item(CCD_INFO_BITS_PER_PIXEL_ITEM, "BITS_PER_PIXEL", "Bits/pixel", 0, 0, 0, 0);
			// -------------------------------------------------------------------------------- CCD_UPLOAD_MODE
			CCD_UPLOAD_MODE_PROPERTY = indigo_init_switch_property(NULL, device->name, "CCD_UPLOAD_MODE", CCD_MAIN_GROUP, "Image upload settings", INDIGO_IDLE_STATE, INDIGO_RW_PERM, INDIGO_ONE_OF_MANY_RULE, 3);
			if (CCD_UPLOAD_MODE_PROPERTY == NULL)
				return INDIGO_FAILED;
			indigo_init_switch_item(CCD_UPLOAD_MODE_CLIENT_ITEM, "CLIENT", "Upload to client", true);
			indigo_init_switch_item(CCD_UPLOAD_MODE_LOCAL_ITEM, "LOCAL", "Save locally", false);
			indigo_init_switch_item(CCD_UPLOAD_MODE_BOTH_ITEM, "BOTH", "Both upload to client and save locally", false);
			// -------------------------------------------------------------------------------- CCD_LOCAL_MODE
			CCD_LOCAL_MODE_PROPERTY = indigo_init_text_property(NULL, device->name, "CCD_LOCAL_MODE", CCD_MAIN_GROUP, "Local mode settings", INDIGO_IDLE_STATE, INDIGO_RW_PERM, 2);
			if (CCD_LOCAL_MODE_PROPERTY == NULL)
				return INDIGO_FAILED;
			indigo_init_text_item(CCD_LOCAL_MODE_DIR_ITEM, "DIR", "Directory", getenv("HOME"));
			indigo_init_text_item(CCD_LOCAL_MODE_PREFIX_ITEM, "PREFIX", "File name prefix", "IMAGE_XXX");
			// -------------------------------------------------------------------------------- CCD_EXPOSURE
			CCD_EXPOSURE_PROPERTY = indigo_init_number_property(NULL, device->name, "CCD_EXPOSURE", CCD_MAIN_GROUP, "Start exposure", INDIGO_IDLE_STATE, INDIGO_RW_PERM, 1);
			if (CCD_EXPOSURE_PROPERTY == NULL)
				return INDIGO_FAILED;
			indigo_init_number_item(CCD_EXPOSURE_ITEM, "EXPOSURE", "Start exposure", 0, 10000, 1, 0);
			// -------------------------------------------------------------------------------- CCD_ABORT_EXPOSURE
			CCD_ABORT_EXPOSURE_PROPERTY = indigo_init_switch_property(NULL, device->name, "CCD_ABORT_EXPOSURE", CCD_MAIN_GROUP, "Abort exposure", INDIGO_IDLE_STATE, INDIGO_RW_PERM, INDIGO_ONE_OF_MANY_RULE, 1);
			if (CCD_ABORT_EXPOSURE_PROPERTY == NULL)
				return INDIGO_FAILED;
			indigo_init_switch_item(CCD_ABORT_EXPOSURE_ITEM, "ABORT_EXPOSURE", "Abort exposure", false);
			// -------------------------------------------------------------------------------- CCD_FRAME
			CCD_FRAME_PROPERTY = indigo_init_number_property(NULL, device->name, "CCD_FRAME", CCD_IMAGE_GROUP, "Frame size setting", INDIGO_IDLE_STATE, INDIGO_RW_PERM, 4);
			if (CCD_FRAME_PROPERTY == NULL)
				return INDIGO_FAILED;
			indigo_init_number_item(CCD_FRAME_LEFT_ITEM, "X", "Left", 0, 0, 1, 0);
			indigo_init_number_item(CCD_FRAME_TOP_ITEM, "Y", "Top", 0, 0, 1, 0);
			indigo_init_number_item(CCD_FRAME_WIDTH_ITEM, "WIDTH", "Width", 0, 0, 1, 0);
			indigo_init_number_item(CCD_FRAME_HEIGHT_ITEM, "HEIGHT", "Height", 0, 0, 1, 0);
			// -------------------------------------------------------------------------------- CCD_BIN
			CCD_BIN_PROPERTY = indigo_init_number_property(NULL, device->name, "CCD_BIN", CCD_IMAGE_GROUP, "Binning setting", INDIGO_IDLE_STATE, INDIGO_RW_PERM, 2);
			if (CCD_BIN_PROPERTY == NULL)
				return INDIGO_FAILED;
			indigo_init_number_item(CCD_BIN_HORIZONTAL_ITEM, "HORIZONTAL", "Horizontal binning", 0, 1, 1, 1);
			indigo_init_number_item(CCD_BIN_VERTICAL_ITEM, "VERTICAL", "Vertical binning", 0, 1, 1, 1);
			// -------------------------------------------------------------------------------- CCD_OFFSET
			CCD_OFFSET_PROPERTY = indigo_init_number_property(NULL, device->name, "CCD_OFFSET", CCD_IMAGE_GROUP, "Offset setting", INDIGO_IDLE_STATE, INDIGO_RW_PERM, 1);
			if (CCD_OFFSET_PROPERTY == NULL)
				return INDIGO_FAILED;
			CCD_OFFSET_PROPERTY->hidden = true;
			indigo_init_number_item(CCD_OFFSET_ITEM, "OFFSET", "Offset", 0, 1, 1, 1);
			// -------------------------------------------------------------------------------- CCD_GAIN
			CCD_GAIN_PROPERTY = indigo_init_number_property(NULL, device->name, "CCD_GAIN", CCD_IMAGE_GROUP, "Gain setting", INDIGO_IDLE_STATE, INDIGO_RW_PERM, 1);
			if (CCD_GAIN_PROPERTY == NULL)
				return INDIGO_FAILED;
			CCD_GAIN_PROPERTY->hidden = true;
			indigo_init_number_item(CCD_GAIN_ITEM, "GAIN", "Gain", 0, 1, 1, 1);
			// -------------------------------------------------------------------------------- CCD_GAMMA
			CCD_GAMMA_PROPERTY = indigo_init_number_property(NULL, device->name, "CCD_GAMMA", CCD_IMAGE_GROUP, "Gamma setting", INDIGO_IDLE_STATE, INDIGO_RW_PERM, 1);
			if (CCD_GAMMA_PROPERTY == NULL)
				return INDIGO_FAILED;
			CCD_GAMMA_PROPERTY->hidden = true;
			indigo_init_number_item(CCD_GAMMA_ITEM, "GAMMA", "Gamma", 0, 1, 1, 1);
			// -------------------------------------------------------------------------------- CCD_FRAME_TYPE
			CCD_FRAME_TYPE_PROPERTY = indigo_init_switch_property(NULL, device->name, "CCD_FRAME_TYPE", CCD_IMAGE_GROUP, "Frame type setting", INDIGO_IDLE_STATE, INDIGO_RW_PERM, INDIGO_ONE_OF_MANY_RULE, 4);
			if (CCD_FRAME_TYPE_PROPERTY == NULL)
				return INDIGO_FAILED;
			indigo_init_switch_item(CCD_FRAME_TYPE_LIGHT_ITEM, "LIGHT", "Light frame exposure", true);
			indigo_init_switch_item(CCD_FRAME_TYPE_BIAS_ITEM, "BIAS", "Bias frame exposure", false);
			indigo_init_switch_item(CCD_FRAME_TYPE_DARK_ITEM, "DARK", "Dark frame exposure", false);
			indigo_init_switch_item(CCD_FRAME_TYPE_FLAT_ITEM, "FLAT", "Flat field frame exposure", false);
			// -------------------------------------------------------------------------------- CCD_IMAGE_FORMAT
			CCD_IMAGE_FORMAT_PROPERTY = indigo_init_switch_property(NULL, device->name, "CCD_IMAGE_FORMAT", CCD_IMAGE_GROUP, "Image format setting", INDIGO_IDLE_STATE, INDIGO_RW_PERM, INDIGO_ONE_OF_MANY_RULE, 2);
			if (CCD_IMAGE_FORMAT_PROPERTY == NULL)
				return INDIGO_FAILED;
			indigo_init_switch_item(CCD_IMAGE_FORMAT_RAW_ITEM, "RAW", "Raw data", false);
			indigo_init_switch_item(CCD_IMAGE_FORMAT_FITS_ITEM, "FITS", "FITS format", true);
			// -------------------------------------------------------------------------------- CCD_IMAGE
			CCD_IMAGE_PROPERTY = indigo_init_blob_property(NULL, device->name, "CCD_IMAGE", CCD_IMAGE_GROUP, "Image data", INDIGO_IDLE_STATE, 1);
			if (CCD_IMAGE_PROPERTY == NULL)
				return INDIGO_FAILED;
			indigo_init_blob_item(CCD_IMAGE_ITEM, "IMAGE", "Image data");
			// -------------------------------------------------------------------------------- CCD_LOCAL_FILE
			CCD_IMAGE_FILE_PROPERTY = indigo_init_text_property(NULL, device->name, "CCD_IMAGE_FILE", CCD_IMAGE_GROUP, "Image file info", INDIGO_IDLE_STATE, INDIGO_RO_PERM, 1);
			if (CCD_IMAGE_FILE_PROPERTY == NULL)
				return INDIGO_FAILED;
			indigo_init_text_item(CCD_IMAGE_FILE_ITEM, "FILE", "Filename", "None");
			// -------------------------------------------------------------------------------- CCD_COOLER
			CCD_COOLER_PROPERTY = indigo_init_switch_property(NULL, device->name, "CCD_COOLER", CCD_COOLER_GROUP, "Cooler status", INDIGO_IDLE_STATE, INDIGO_RW_PERM, INDIGO_ONE_OF_MANY_RULE, 2);
			if (CCD_COOLER_PROPERTY == NULL)
				return INDIGO_FAILED;
			CCD_COOLER_PROPERTY->hidden = true;
			indigo_init_switch_item(CCD_COOLER_ON_ITEM, "ON", "On", false);
			indigo_init_switch_item(CCD_COOLER_OFF_ITEM, "OFF", "Off", true);
			// -------------------------------------------------------------------------------- CCD_COOLER_POWER
			CCD_COOLER_POWER_PROPERTY = indigo_init_number_property(NULL, device->name, "CCD_COOLER_POWER", CCD_COOLER_GROUP, "Cooler power setting", INDIGO_IDLE_STATE, INDIGO_RO_PERM, 1);
			if (CCD_COOLER_POWER_PROPERTY == NULL)
				return INDIGO_FAILED;
			CCD_COOLER_POWER_PROPERTY->hidden = true;
			indigo_init_number_item(CCD_COOLER_POWER_ITEM, "POWER", "Power (%)", 0, 100, 1, 0);
			// -------------------------------------------------------------------------------- CCD_TEMPERATURE
			CCD_TEMPERATURE_PROPERTY = indigo_init_number_property(NULL, device->name, "CCD_TEMPERATURE", CCD_COOLER_GROUP, "Temperature setting", INDIGO_IDLE_STATE, INDIGO_RW_PERM, 1);
			if (CCD_TEMPERATURE_PROPERTY == NULL)
				return INDIGO_FAILED;
			CCD_TEMPERATURE_PROPERTY->hidden = true;
			indigo_init_number_item(CCD_TEMPERATURE_ITEM, "TEMPERATURE", "Temperature (C)", -50, 50, 1, 0);
			// --------------------------------------------------------------------------------
			return INDIGO_OK;
		}
	}
	return INDIGO_FAILED;
}

indigo_result indigo_ccd_enumerate_properties(indigo_device *device, indigo_client *client, indigo_property *property) {
	assert(device != NULL);
	assert(device->device_context != NULL);
	indigo_result result = INDIGO_OK;
	if ((result = indigo_device_enumerate_properties(device, client, property)) == INDIGO_OK) {
		if (CONNECTION_CONNECTED_ITEM->sw.value) {
			if (indigo_property_match(CCD_INFO_PROPERTY, property))
				indigo_define_property(device, CCD_INFO_PROPERTY, NULL);
			if (indigo_property_match(CCD_LOCAL_MODE_PROPERTY, property))
				indigo_define_property(device, CCD_LOCAL_MODE_PROPERTY, NULL);
			if (indigo_property_match(CCD_IMAGE_FILE_PROPERTY, property))
				indigo_define_property(device, CCD_IMAGE_FILE_PROPERTY, NULL);
			if (indigo_property_match(CCD_EXPOSURE_PROPERTY, property))
				indigo_define_property(device, CCD_EXPOSURE_PROPERTY, NULL);
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
			if (indigo_property_match(CCD_COOLER_PROPERTY, property) && !CCD_COOLER_PROPERTY->hidden)
				indigo_define_property(device, CCD_COOLER_PROPERTY, NULL);
			if (indigo_property_match(CCD_COOLER_POWER_PROPERTY, property) && !CCD_COOLER_POWER_PROPERTY->hidden)
				indigo_define_property(device, CCD_COOLER_POWER_PROPERTY, NULL);
			if (indigo_property_match(CCD_TEMPERATURE_PROPERTY, property) && !CCD_TEMPERATURE_PROPERTY->hidden)
				indigo_define_property(device, CCD_TEMPERATURE_PROPERTY, NULL);
		}
	}
	return result;
}

indigo_result indigo_ccd_change_property(indigo_device *device, indigo_client *client, indigo_property *property) {
	assert(device != NULL);
	assert(device->device_context != NULL);
	assert(property != NULL);
	if (indigo_property_match(CONNECTION_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- CONNECTION
		if (CONNECTION_CONNECTED_ITEM->sw.value) {
			indigo_define_property(device, CCD_INFO_PROPERTY, NULL);
			indigo_define_property(device, CCD_UPLOAD_MODE_PROPERTY, NULL);
			indigo_define_property(device, CCD_LOCAL_MODE_PROPERTY, NULL);
			indigo_define_property(device, CCD_EXPOSURE_PROPERTY, NULL);
			indigo_define_property(device, CCD_ABORT_EXPOSURE_PROPERTY, NULL);
			indigo_define_property(device, CCD_FRAME_PROPERTY, NULL);
			indigo_define_property(device, CCD_BIN_PROPERTY, NULL);
			if (!CCD_OFFSET_PROPERTY->hidden)
				indigo_define_property(device, CCD_OFFSET_PROPERTY, NULL);
			if (!CCD_GAIN_PROPERTY->hidden)
				indigo_define_property(device, CCD_GAIN_PROPERTY, NULL);
			if (!CCD_GAMMA_PROPERTY->hidden)
				indigo_define_property(device, CCD_GAMMA_PROPERTY, NULL);
			indigo_define_property(device, CCD_FRAME_TYPE_PROPERTY, NULL);
			indigo_define_property(device, CCD_IMAGE_FORMAT_PROPERTY, NULL);
			indigo_define_property(device, CCD_IMAGE_FILE_PROPERTY, NULL);
			indigo_define_property(device, CCD_IMAGE_PROPERTY, NULL);
			if (!CCD_COOLER_PROPERTY->hidden)
				indigo_define_property(device, CCD_COOLER_PROPERTY, NULL);
			if (!CCD_COOLER_POWER_PROPERTY->hidden)
				indigo_define_property(device, CCD_COOLER_POWER_PROPERTY, NULL);
			if (!CCD_TEMPERATURE_PROPERTY->hidden)
				indigo_define_property(device, CCD_TEMPERATURE_PROPERTY, NULL);
		} else {
			indigo_delete_property(device, CCD_INFO_PROPERTY, NULL);
			indigo_delete_property(device, CCD_UPLOAD_MODE_PROPERTY, NULL);
			indigo_delete_property(device, CCD_LOCAL_MODE_PROPERTY, NULL);
			indigo_delete_property(device, CCD_EXPOSURE_PROPERTY, NULL);
			indigo_delete_property(device, CCD_ABORT_EXPOSURE_PROPERTY, NULL);
			indigo_delete_property(device, CCD_FRAME_PROPERTY, NULL);
			indigo_delete_property(device, CCD_BIN_PROPERTY, NULL);
			if (!CCD_OFFSET_PROPERTY->hidden)
				indigo_delete_property(device, CCD_OFFSET_PROPERTY, NULL);
			if (!CCD_GAIN_PROPERTY->hidden)
				indigo_delete_property(device, CCD_GAIN_PROPERTY, NULL);
			if (!CCD_GAMMA_PROPERTY->hidden)
				indigo_delete_property(device, CCD_GAMMA_PROPERTY, NULL);
			indigo_delete_property(device, CCD_FRAME_TYPE_PROPERTY, NULL);
			indigo_delete_property(device, CCD_IMAGE_FORMAT_PROPERTY, NULL);
			indigo_delete_property(device, CCD_IMAGE_FILE_PROPERTY, NULL);
			indigo_delete_property(device, CCD_IMAGE_PROPERTY, NULL);
			if (!CCD_COOLER_PROPERTY->hidden)
				indigo_delete_property(device, CCD_COOLER_PROPERTY, NULL);
			if (!CCD_COOLER_POWER_PROPERTY->hidden)
				indigo_delete_property(device, CCD_COOLER_POWER_PROPERTY, NULL);
			if (!CCD_TEMPERATURE_PROPERTY->hidden)
				indigo_delete_property(device, CCD_TEMPERATURE_PROPERTY, NULL);
		}
	} else if (indigo_property_match(CONFIG_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- CONFIG
		if (indigo_switch_match(CONFIG_SAVE_ITEM, property)) {
			if (CCD_UPLOAD_MODE_PROPERTY->perm == INDIGO_RW_PERM)
				indigo_save_property(device, NULL, CCD_UPLOAD_MODE_PROPERTY);
			if (CCD_LOCAL_MODE_PROPERTY->perm == INDIGO_RW_PERM)
				indigo_save_property(device, NULL, CCD_LOCAL_MODE_PROPERTY);
			if (CCD_FRAME_PROPERTY->perm == INDIGO_RW_PERM)
				indigo_save_property(device, NULL, CCD_FRAME_PROPERTY);
			if (CCD_BIN_PROPERTY->perm == INDIGO_RW_PERM)
				indigo_save_property(device, NULL, CCD_BIN_PROPERTY);
			if (!CCD_OFFSET_PROPERTY->hidden && CCD_OFFSET_PROPERTY->perm == INDIGO_RW_PERM)
				indigo_save_property(device, NULL, CCD_OFFSET_PROPERTY);
			if (!CCD_GAMMA_PROPERTY->hidden && CCD_GAMMA_PROPERTY->perm == INDIGO_RW_PERM)
				indigo_save_property(device, NULL, CCD_GAMMA_PROPERTY);
			if (!CCD_GAIN_PROPERTY->hidden && CCD_GAIN_PROPERTY->perm == INDIGO_RW_PERM)
				indigo_save_property(device, NULL, CCD_GAIN_PROPERTY);
			if (CCD_FRAME_TYPE_PROPERTY->perm == INDIGO_RW_PERM)
				indigo_save_property(device, NULL, CCD_FRAME_TYPE_PROPERTY);
			if (CCD_IMAGE_FORMAT_PROPERTY->perm == INDIGO_RW_PERM)
				indigo_save_property(device, NULL, CCD_IMAGE_FORMAT_PROPERTY);
			if (CCD_COOLER_PROPERTY->perm == INDIGO_RW_PERM)
				indigo_save_property(device, NULL, CCD_COOLER_PROPERTY);
			if (CCD_TEMPERATURE_PROPERTY->perm == INDIGO_RW_PERM)
				indigo_save_property(device, NULL, CCD_TEMPERATURE_PROPERTY);
		}
	} else if (indigo_property_match(CCD_EXPOSURE_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- CCD_EXPOSURE
		if (CCD_EXPOSURE_PROPERTY->state == INDIGO_BUSY_STATE) {
			if (CCD_EXPOSURE_ITEM->number.value >= 1) {
				indigo_set_timer(device, 1.0, countdown_timer_callback);
			}
		}
	} else if (indigo_property_match(CCD_ABORT_EXPOSURE_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- CCD_ABORT_EXPOSURE
		if (CCD_EXPOSURE_PROPERTY->state == INDIGO_BUSY_STATE) {
			CCD_EXPOSURE_PROPERTY->state = INDIGO_ALERT_STATE;
			CCD_EXPOSURE_ITEM->number.value = 0;
			indigo_update_property(device, CCD_EXPOSURE_PROPERTY, NULL);
			CCD_IMAGE_PROPERTY->state = INDIGO_ALERT_STATE;
			indigo_update_property(device, CCD_IMAGE_PROPERTY, NULL);
			CCD_ABORT_EXPOSURE_PROPERTY->state = INDIGO_OK_STATE;
		} else {
			CCD_ABORT_EXPOSURE_PROPERTY->state = INDIGO_ALERT_STATE;
		}
		CCD_ABORT_EXPOSURE_ITEM->sw.value = false;
		indigo_update_property(device, CCD_ABORT_EXPOSURE_PROPERTY, CCD_ABORT_EXPOSURE_PROPERTY->state == INDIGO_OK_STATE ? "Exposure canceled" : "Failed to cancel exposure");
	} else if (indigo_property_match(CCD_FRAME_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- CCD_FRAME
		indigo_property_copy_values(CCD_FRAME_PROPERTY, property, false);
		CCD_FRAME_WIDTH_ITEM->number.value = ((int)CCD_FRAME_WIDTH_ITEM->number.value / (int)CCD_BIN_HORIZONTAL_ITEM->number.value) * (int)CCD_BIN_HORIZONTAL_ITEM->number.value;
		CCD_FRAME_HEIGHT_ITEM->number.value = ((int)CCD_FRAME_HEIGHT_ITEM->number.value / (int)CCD_BIN_VERTICAL_ITEM->number.value) * (int)CCD_BIN_VERTICAL_ITEM->number.value;
		if (CONNECTION_CONNECTED_ITEM->sw.value) {
			CCD_FRAME_PROPERTY->state = INDIGO_OK_STATE;
			indigo_update_property(device, CCD_FRAME_PROPERTY, NULL);
		}
	} else if (indigo_property_match(CCD_BIN_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- CCD_BIN
		indigo_property_copy_values(CCD_BIN_PROPERTY, property, false);
		CCD_FRAME_WIDTH_ITEM->number.value = ((int)CCD_FRAME_WIDTH_ITEM->number.value / (int)CCD_BIN_HORIZONTAL_ITEM->number.value) * (int)CCD_BIN_HORIZONTAL_ITEM->number.value;
		CCD_FRAME_HEIGHT_ITEM->number.value = ((int)CCD_FRAME_HEIGHT_ITEM->number.value / (int)CCD_BIN_VERTICAL_ITEM->number.value) * (int)CCD_BIN_VERTICAL_ITEM->number.value;
		if (CONNECTION_CONNECTED_ITEM->sw.value) {
			CCD_BIN_PROPERTY->state = INDIGO_OK_STATE;
			indigo_update_property(device, CCD_BIN_PROPERTY, NULL);
			CCD_FRAME_PROPERTY->state = INDIGO_OK_STATE;
			indigo_update_property(device, CCD_FRAME_PROPERTY, NULL);
		}
	} else if (indigo_property_match(CCD_OFFSET_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- CCD_OFFSET
		indigo_property_copy_values(CCD_OFFSET_PROPERTY, property, false);
		if (CONNECTION_CONNECTED_ITEM->sw.value) {
			CCD_OFFSET_PROPERTY->state = INDIGO_OK_STATE;
			indigo_update_property(device, CCD_OFFSET_PROPERTY, NULL);
		}
	} else if (indigo_property_match(CCD_GAIN_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- CCD_GAIN
		indigo_property_copy_values(CCD_GAIN_PROPERTY, property, false);
		if (CONNECTION_CONNECTED_ITEM->sw.value) {
			CCD_GAIN_PROPERTY->state = INDIGO_OK_STATE;
			indigo_update_property(device, CCD_GAIN_PROPERTY, NULL);
		}
	} else if (indigo_property_match(CCD_GAMMA_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- CCD_GAMMA
		indigo_property_copy_values(CCD_GAMMA_PROPERTY, property, false);
		if (CONNECTION_CONNECTED_ITEM->sw.value) {
			CCD_GAMMA_PROPERTY->state = INDIGO_OK_STATE;
			indigo_update_property(device, CCD_GAMMA_PROPERTY, NULL);
		}
	} else if (indigo_property_match(CCD_FRAME_TYPE_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- CCD_FRAME_TYPE
		indigo_property_copy_values(CCD_FRAME_TYPE_PROPERTY, property, false);
		CCD_FRAME_TYPE_PROPERTY->state = INDIGO_OK_STATE;
		if (CONNECTION_CONNECTED_ITEM->sw.value)
			indigo_update_property(device, CCD_FRAME_TYPE_PROPERTY, NULL);
	} else if (indigo_property_match(CCD_IMAGE_FORMAT_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- CCD_IMAGE_FORMAT
		indigo_property_copy_values(CCD_IMAGE_FORMAT_PROPERTY, property, false);
		CCD_IMAGE_FORMAT_PROPERTY->state = INDIGO_OK_STATE;
		if (CONNECTION_CONNECTED_ITEM->sw.value)
			indigo_update_property(device, CCD_IMAGE_FORMAT_PROPERTY, NULL);
	} else if (indigo_property_match(CCD_UPLOAD_MODE_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- CCD_IMAGE_UPLOAD_MODE
		indigo_property_copy_values(CCD_UPLOAD_MODE_PROPERTY, property, false);
		CCD_UPLOAD_MODE_PROPERTY->state = INDIGO_OK_STATE;
		if (CONNECTION_CONNECTED_ITEM->sw.value)
			indigo_update_property(device, CCD_UPLOAD_MODE_PROPERTY, NULL);
	} else if (indigo_property_match(CCD_LOCAL_MODE_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- CCD_IMAGE_LOCAL_MODE
		indigo_property_copy_values(CCD_LOCAL_MODE_PROPERTY, property, false);
		CCD_LOCAL_MODE_PROPERTY->state = INDIGO_OK_STATE;
		if (CONNECTION_CONNECTED_ITEM->sw.value)
			indigo_update_property(device, CCD_LOCAL_MODE_PROPERTY, NULL);
		// --------------------------------------------------------------------------------
	}
	return indigo_device_change_property(device, client, property);
}

indigo_result indigo_ccd_detach(indigo_device *device) {
	assert(device != NULL);
	if (CONNECTION_CONNECTED_ITEM->sw.value) {
		indigo_delete_property(device, CCD_INFO_PROPERTY, NULL);
		indigo_delete_property(device, CCD_UPLOAD_MODE_PROPERTY, NULL);
		indigo_delete_property(device, CCD_LOCAL_MODE_PROPERTY, NULL);
		indigo_delete_property(device, CCD_EXPOSURE_PROPERTY, NULL);
		indigo_delete_property(device, CCD_ABORT_EXPOSURE_PROPERTY, NULL);
		indigo_delete_property(device, CCD_FRAME_PROPERTY, NULL);
		indigo_delete_property(device, CCD_BIN_PROPERTY, NULL);
		indigo_delete_property(device, CCD_FRAME_TYPE_PROPERTY, NULL);
		indigo_delete_property(device, CCD_IMAGE_FORMAT_PROPERTY, NULL);
		indigo_delete_property(device, CCD_IMAGE_FILE_PROPERTY, NULL);
		indigo_delete_property(device, CCD_IMAGE_PROPERTY, NULL);
		if (!CCD_COOLER_PROPERTY->hidden)
			indigo_delete_property(device, CCD_COOLER_PROPERTY, NULL);
		if (!CCD_COOLER_POWER_PROPERTY->hidden)
			indigo_delete_property(device, CCD_COOLER_POWER_PROPERTY, NULL);
		if (!CCD_TEMPERATURE_PROPERTY->hidden)
			indigo_delete_property(device, CCD_TEMPERATURE_PROPERTY, NULL);
	}
	free(CCD_INFO_PROPERTY);
	free(CCD_UPLOAD_MODE_PROPERTY);
	free(CCD_LOCAL_MODE_PROPERTY);
	free(CCD_EXPOSURE_PROPERTY);
	free(CCD_ABORT_EXPOSURE_PROPERTY);
	free(CCD_FRAME_PROPERTY);
	free(CCD_BIN_PROPERTY);
	free(CCD_FRAME_TYPE_PROPERTY);
	free(CCD_IMAGE_FORMAT_PROPERTY);
	free(CCD_IMAGE_FILE_PROPERTY);
	free(CCD_IMAGE_PROPERTY);
	free(CCD_TEMPERATURE_PROPERTY);
	free(CCD_COOLER_PROPERTY);
	free(CCD_COOLER_POWER_PROPERTY);
	return indigo_device_detach(device);
}

void indigo_process_image(indigo_device *device, void *data, int frame_width, int frame_height, double exposure_time) {
	assert(device != NULL);
	assert(data != NULL);
	INDIGO_DEBUG(clock_t start = clock());

	int horizontal_bin = CCD_BIN_HORIZONTAL_ITEM->number.value;
	int vertical_bin = CCD_BIN_VERTICAL_ITEM->number.value;
// int frame_width = CCD_FRAME_WIDTH_ITEM->number.value / horizontal_bin;
// int frame_height = CCD_FRAME_HEIGHT_ITEM->number.value / vertical_bin;
	int byte_per_pixel = CCD_INFO_BITS_PER_PIXEL_ITEM->number.value / 8;
	int size = frame_width * frame_height;

	if (CCD_IMAGE_FORMAT_FITS_ITEM->sw.value) {
		time_t timer;
		struct tm* tm_info;
		char now[20];
		time(&timer);
		tm_info = gmtime(&timer);
		strftime(now, 20, "%Y:%m:%dT%H:%M:%S", tm_info);
		char *header = data;
		memset(header, ' ', FITS_HEADER_SIZE);
		int t = sprintf(header, "SIMPLE  =                     T / file conforms to FITS standard");
		header[t] = ' ';
		t = sprintf(header += 80, "BITPIX  = %21d / number of bits per data pixel", (int)CCD_INFO_BITS_PER_PIXEL_ITEM->number.value);
		header[t] = ' ';
		t = sprintf(header += 80, "NAXIS   =                     2 / number of data axes");
		header[t] = ' ';
		t = sprintf(header += 80, "NAXIS1  = %21d / length of data axis 1 [pixels]", frame_width);
		header[t] = ' ';
		t = sprintf(header += 80, "NAXIS2  = %21d / length of data axis 2 [pixels]", frame_height);
		header[t] = ' ';
		t = sprintf(header += 80, "EXTEND  =                     T / FITS dataset may contain extensions");
		header[t] = ' ';
		t = sprintf(header += 80, "COMMENT   FITS (Flexible Image Transport System) format is defined in 'Astronomy");
		header[t] = ' ';
		t = sprintf(header += 80, "COMMENT   and Astrophysics', volume 376, page 359; bibcode: 2001A&A...376..359H");
		header[t] = ' ';
		if (CCD_INFO_BITS_PER_PIXEL_ITEM->number.value == 16) {
			t = sprintf(header += 80, "BZERO   =                 32768 / offset data range to that of unsigned short");
			header[t] = ' ';
			t = sprintf(header += 80, "BSCALE  =                     1 / default scaling factor");
			header[t] = ' ';
		} else {
			t = sprintf(header += 80, "BZERO   =                     0 / offset data range to that of unsigned short");
			header[t] = ' ';
			t = sprintf(header += 80, "BSCALE  =                   256 / default scaling factor");
			header[t] = ' ';
		}
		t = sprintf(header += 80, "XBINNING= %21d / horizontal binning [pixels]", horizontal_bin);
		header[t] = ' ';
		t = sprintf(header += 80, "YBINNING= %21d / vertical binning [pixels]", vertical_bin);
		header[t] = ' ';
		t = sprintf(header += 80, "XPIXSZ  = %21.2g / pixel width [microns]", CCD_INFO_PIXEL_WIDTH_ITEM->number.value);
		header[t] = ' ';
		t = sprintf(header += 80, "YPIXSZ  = %21.2g / pixel height [microns]", CCD_INFO_PIXEL_HEIGHT_ITEM->number.value);
		header[t] = ' ';
		t = sprintf(header += 80, "EXPTIME = %21.2g / exposure time [s]", exposure_time);
		header[t] = ' ';
		if (!CCD_TEMPERATURE_PROPERTY->hidden) {
			t = sprintf(header += 80, "CCD-TEMP= %21.2g / CCD temperature [C]", CCD_TEMPERATURE_ITEM->number.value);
			header[t] = ' ';
		}
		t = sprintf(header += 80, "DATE    = '%s' / UTC date that FITS file was created", now);
		header[t] = ' ';
		t = sprintf(header += 80, "INSTRUME= '%s'%*c / instrument name", device->name, (int)(19 - strlen(device->name)), ' ');
		header[t] = ' ';
		t = sprintf(header += 80, "COMMENT   Created by INDIGO %d.%d framework, see www.indigo-astronomy.org", (INDIGO_VERSION >> 8) & 0xFF, INDIGO_VERSION & 0xFF);
		header[t] = ' ';
		t = sprintf(header += 80, "END");
		header[t] = ' ';
		if (byte_per_pixel == 2) {
			short *raw = (short *)(data + FITS_HEADER_SIZE);
			int size = CCD_INFO_WIDTH_ITEM->number.value * CCD_INFO_HEIGHT_ITEM->number.value;
			for (int i = 0; i < size; i++) {
				int value = *raw - 32768;
				*raw++ = (value & 0xff) << 8 | (value & 0xff00) >> 8;
			}
		}
		INDIGO_DEBUG(indigo_debug("RAW to FITS conversion in %gs", (clock() - start) / (double)CLOCKS_PER_SEC));
	}
	if (CCD_UPLOAD_MODE_LOCAL_ITEM->sw.value) {
		char *dir = CCD_LOCAL_MODE_DIR_ITEM->text.value;
		char *prefix = CCD_LOCAL_MODE_PREFIX_ITEM->text.value;
		char *sufix;
		if (CCD_IMAGE_FORMAT_FITS_ITEM->sw.value) {
			sufix = ".fits";
		} else if (CCD_IMAGE_FORMAT_RAW_ITEM->sw.value) {
			sufix = ".raw";
		}
		char file_name[256];
		char *xxx = strstr(prefix, "XXX");
		if (xxx == NULL) {
			strncpy(file_name, dir, INDIGO_VALUE_SIZE);
			strcat(file_name, prefix);
			strcat(file_name, sufix);
		} else {
			char format[256];
			strncpy(format, dir, INDIGO_VALUE_SIZE);
			strncat(format, prefix, xxx - prefix);
			strcat(format, "%03d");
			strcat(format, xxx+3);
			strcat(format, sufix);
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
		char *message = NULL;
		int handle = open(file_name, O_WRONLY | O_CREAT | O_TRUNC, 0644);
		if (handle) {
			if (CCD_IMAGE_FORMAT_FITS_ITEM->sw.value) {
				if (write(handle, data, FITS_HEADER_SIZE + byte_per_pixel * size) < 0) {
					CCD_IMAGE_FILE_PROPERTY->state = INDIGO_ALERT_STATE;
					message = strerror(errno);
				}
			} else if (CCD_IMAGE_FORMAT_RAW_ITEM->sw.value) {
				if (write(handle, data + FITS_HEADER_SIZE, byte_per_pixel * size) < 0) {
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
		if (CCD_IMAGE_FORMAT_FITS_ITEM->sw.value) {
			CCD_IMAGE_ITEM->blob.value = data;
			CCD_IMAGE_ITEM->blob.size = FITS_HEADER_SIZE + byte_per_pixel * size;
			strncpy(CCD_IMAGE_ITEM->blob.format, ".fits", INDIGO_NAME_SIZE);
		} else if (CCD_IMAGE_FORMAT_RAW_ITEM->sw.value) {
			CCD_IMAGE_ITEM->blob.value = data + FITS_HEADER_SIZE;
			CCD_IMAGE_ITEM->blob.size = byte_per_pixel * size;
			strncpy(CCD_IMAGE_ITEM->blob.format, ".raw", INDIGO_VALUE_SIZE);
		}
		CCD_IMAGE_PROPERTY->state = INDIGO_OK_STATE;
		indigo_update_property(device, CCD_IMAGE_PROPERTY, NULL);
		INDIGO_DEBUG(indigo_debug("Client upload in %gs", (clock() - start) / (double)CLOCKS_PER_SEC));
	}
}

